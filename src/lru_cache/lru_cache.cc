#include "lru_cache.hh"
#include "debug/LruCache.hh"
#include "sim/system.hh"
#include "base/random.hh"

/*------------------------------------------------------------
    LruCache
------------------------------------------------------------*/

LruCache::LruCache(LruCacheParams *params)
    ClockedObject(params), \
    latency(params->latency), \
    blockSize(params->system->cacheLineSize()), \
    capacity(params->size / blockSize), \
    memPort(params->name + ".mem_side", this), \
    blocked(false), \
    outstandingPacket(nullptr), \
    waitingPortID(-1)
{
    for (int i = 0; i < params->port_cpu_side_connection_count; ++i){
        cpuPorts.emplace_back(name() + csprintf(".cpu_side[%d]", i), i, this);
    }
}

Port &LruCache::getPort(const std::string& if_name, PortID idx = InvalidPortID)
{
    if (if_name == "mem_side"){
        panic_if(idx != InvalidPortID, "Mem side of cache is not vector port");
        return memPort;
    }
    else if (if_name == "cpu_side" && idx < cpuPorts.size()){
        return cpuPorts[idx];
    }
    else{
        return ClockedObject::getPort(if_name, idx);
    }
}

bool LruCache::handleRequest(PacketPtr pkt, int port_id)
{
    // if we already blocked request since we are waiting for response
    if (blocked){
        return false;
    }

    blocked = true;                 // now blocked request
    waitingPortID = port_id;        // record the waiting port

    // after latency finish the event
    schedule(new EventFunctionWrapper([this, pkt]{ accessTiming(pkt); }, name() + ".accessEvent", true), clockEdge(latency));

    return true;
}

bool LruCache::handleResponse(PacketPtr pkt)
{
    assert(blocked);
    DPRINTF(LruCache, "Got response of addr %#x.\n", pkt->getAddr());

    // since main memory hit, so must cache miss, we insert it to cache
    insert(pkt);

    missLatency.sample(curTick()- missTime);        // record miss latency

    if (originalPkt != nullptr){
        // after insert, we can treat the original packet in the case that it will hits
        bool hit M5_VAR_USED = accessFunctional(originalPkt);
        panic_if(!hit, "Should always hit after inserting");

        originalPkt->makeResponse();
        delete pkt;     // now packet is freed and we can reuse it

        pkt = originalPkt;
        originalPkt = nullptr;
    }

    sendResponse(pkt);

    return true;
}

void LruCache::sendResponse(PacketPtr pkt)
{
    int id = waitingPortID;

    blocked = false;
    waitingPortID = -1;

    cpuPorts[id].sendPacket(pkt);
    for (auto &port: cpuPorts){
        port.trySendRetry();
    }
}

AddrRangeList LruCache::getAddrRanges() const
{
    DPRINTF(LruCache, "Sending new range\n");
    return memPort.getAddrRanges();
}

void LruCache::accessTiming(PacketPtr pkt)
{
    bool hit = accessFunctional(pkt);

    DPRINTF(LruCache, "%s for the packet: %s\n", hit ? "hit" : "miss", pkt->print());

    if (hit){
        hits++;         // just for collect data about cache hits

        DDUMP(LruCache, pkt->getConstPtr(), pkt->getSize());
        pkt->makeResponse();                // make it to be a response
        sendResponse(pkt);
    }
    else{
        misses++;       // just for collect data about cache misses
        missTime = curTick();       // current tick

        Addr addr = pkt->getAddr();
        Addr blockAddr = pkt->getBlockAddr(blockSize);
        unsigned int size = pkt->getSize();

        if ((addr == blockAddr) && (size == blockSize)){
            DPRINTF(LruCache, "Forwarding Packet.\n");
            memPort.sendPacket(pkt);
        }
        else{

            DPRINTF(LruCache, "Upgrading the packet into blocksize.\n");

            panic_if(Addr - blockAddr + size > blockSize, "Never access memory span multiple block");

            assert(pkt->needsResponse());
            Memcmd cmd;
            if (pkt->isWrite() || pkt->isRead()){
                cmd = MemCmd::ReadReq;
            }
            else{
                panic("Unknown type of packet");
            }

            PacketPtr newPkt = new Packet(pkt->req, cmd, blockSize);
            newPkt->allocate();

            assert(newPkt->getAddr() == newPkt->getBlockAddr(blockSize));

            originalPkt = pkt;

            DPRINTF(LruCache, "Forwarding packet.\n");
            memPort.sendPacket(newPkt);
        }
    }
}

void LruCache::accessFunctional(PacketPtr pkt)
{
    Addr block_addr = pkt->getBlockAddr(blockSize);

    auto iter = cacheStore.find(block_addr);
    if (iter != cacheStore.end()){
        if (pkt->isWrite()){
            pkt->writeDataToBlock(iter->second, blockSize);
        }
        else if (pkt->isRead()){
            pkt->readDataFromBlock(iter->second, blockSize);
        }
        else{
            panic("unknown packet type");
        }

        return true;
    }

    return false;
}

/* -----------------------------------------------------------
    LruCache::CPUSidePort
------------------------------------------------------------*/

bool LruCache::CPUSidePort::recvTimingReq(PacketPtr pkt)
{
    DPRINTF(LruCache, "Got request %s\n", pkt->print());

    // we can not handle the request right now
    if (!owner->handleRequest){
        DPRINTF(LruCache, "Request fail");
        needRetry = true;
        return false;
    }
}

void LruCache::CPUSidePort::sendPacket(PacketPtr pkt)
{
    panic_if(blockedPkt != nullptr, "Never try to send when the port is blocked");

    // if failed to send response correctly, first stored to wait for chance
    if (!sendTimingResp(pkt)){
        DPRINTF(LruCache, "Repsonse Failed.\n");
        blockedPkt = pkt;
    }
}

void LruCache::CPUSidePort::trySendRetry()
{
    // if need retry and the previous response was successfully sent out
    if (needRetry && blockedPkt == nullptr){
        needRetry = false;
        DPRINTF(LruCache, "Port%d, send retry req.\n", id);
        sendRetryReq();
    }
}

AddrRangeList LruCache::CPUSidePort::getAddrRanges() const
{
    return owner->getAddrRanges();
}

/* -----------------------------------------------------------
    LruCache::MemSidePort
--------------------------------------------------------------*/

void LruCache::MemSidePort::sendPacket(PacketPtr pkt)
{
    panic_if(blockedPkt != nullptr, "Never try to send when blocked.\n");

    if (!sendTimingReq(pkt)){
        DPRINTF(LruCache, "Mem req failed.\n");
        blockedPkt = pkt;
    }
}


/* -----------------------------------------------------------
    LruCacheParams
--------------------------------------------------------------*/

LruCache *LruCacheParams::create()
{
    return new LruCache(this);
}