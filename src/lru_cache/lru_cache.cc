#include "lru_cache.hh"
#include "debug/LruCache.hh"
#include "sim/system.hh"
#include "base/random.hh"

/*------------------------------------------------------------
    LruCache
------------------------------------------------------------*/

LruCache::LruCache(LruCacheParams *params):
    ClockedObject(params), \
    latency(params->latency), \
    blockSize(params->system->cacheLineSize()), \
    capacity(params->size / blockSize), \
    memPort(params->name + ".mem_side", this), \
    blocked(false), \
    originalPkt(nullptr), \
    waitingPortId(-1)
{
    for (int i = 0; i < params->port_cpu_side_connection_count; ++i){
        cpuPorts.emplace_back(name() + csprintf(".cpu_side[%d]", i), i, this);
    }
}

Port &LruCache::getPort(const std::string& if_name, PortID idx)
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

AddrRangeList LruCache::getAddrRanges() const
{
    DPRINTF(LruCache, "Got New Addr range and sending.\n");

    return memPort.getAddrRanges();
}

void LruCache::sendRangeChange() const
{
    for (auto &port: cpuPorts){
        port.sendRangeChange();
    }
}

bool LruCache::handleRequest(PacketPtr pkt, int port_id)
{
    // if we already blocked request since we are waiting for response
    if (blocked){
        return false;
    }

    blocked = true;                 // now blocked request
    waitingPortId = port_id;        // record the waiting port

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

void LruCache::handleFunctional(PacketPtr pkt)
{
    if (accessFunctional(pkt)){
        pkt->makeResponse();
    }
    else{
        memPort.sendFunctional(pkt);
    }
}

void LruCache::sendResponse(PacketPtr pkt)
{
    int id = waitingPortId;

    blocked = false;
    waitingPortId = -1;

    cpuPorts[id].sendPacket(pkt);
    for (auto &port: cpuPorts){
        port.trySendRetry();
    }
}

void LruCache::accessTiming(PacketPtr pkt)
{
    bool hit = accessFunctional(pkt);

    DPRINTF(LruCache, "%s for the packet: %s\n", hit ? "hit" : "miss", pkt->print());

    if (hit){
        hits++;         // just for collect data about cache hits

        DDUMP(LruCache, pkt->getConstPtr<uint8_t>(), pkt->getSize());
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

            panic_if(addr - blockAddr + size > blockSize, "Never access memory span multiple block");

            assert(pkt->needsResponse());
            MemCmd cmd;
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

bool LruCache::accessFunctional(PacketPtr pkt)
{
    Addr block_addr = pkt->getBlockAddr(blockSize);

    auto iter = cacheStore.find(block_addr);
    if (iter != cacheStore.end()){
        if (pkt->isWrite()){
            pkt->writeDataToBlock(iter->second, blockSize);
        }
        else if (pkt->isRead()){
            pkt->setDataFromBlock(iter->second, blockSize);
        }
        else{
            panic("unknown packet type");
        }

        return true;
    }

    return false;
}

void LruCache::insert(PacketPtr pkt)
{
    assert(pkt->isResponse());          // the packet must be a response
    assert(pkt->getAddr() == pkt->getBlockAddr(blockSize));   // the packet must be aligned in block size
    assert(cacheStore.find(pkt->getAddr()) == cacheStore.end());       // it must not be in cache

    // replace algorithm
    
    /* random replace */
    if (cacheStore.size() >= capacity){
        int bucket, bucket_size;

        do{
            bucket = random_mt.random(0, (int) cacheStore.bucket_count()-1);
        } while ( (bucket_size = cacheStore.bucket_size(bucket)) == 0);

        // from that bucket randomly pick one
        auto block = std::next(cacheStore.begin(bucket), random_mt.random(0, bucket_size-1));

        DPRINTF(LruCache, "Removing addr %#x.\n", block->first);

        // now we already pick one to replace, we need to send it back to main memory
        RequestPtr req = std::make_shared<Request>(block->first, blockSize, 0, 0);

        // as we already configure the request, we pack the information to send
        PacketPtr new_pkt = new Packet(req, MemCmd::WritebackDirty, blockSize);
        new_pkt->dataDynamic(block->second);

        memPort.sendPacket(new_pkt);

        cacheStore.erase(block->first);
    }

    DPRINTF(LruCache, "Inserting %s.\n", pkt->print());
    DDUMP(LruCache, pkt->getConstPtr<uint8_t>(), blockSize);

    uint8_t *data = new uint8_t[blockSize];

    cacheStore[pkt->getAddr()] = data;
    pkt->writeDataToBlock(data, blockSize);
}

/* -----------------------------------------------------------
    LruCache::CPUSidePort
------------------------------------------------------------*/

bool LruCache::CPUSidePort::recvTimingReq(PacketPtr pkt)
{
    DPRINTF(LruCache, "Got request %s\n", pkt->print());

    // when previously sending response failed, we have to wait for it to be send correctly
    if (blockedPkt || needRetry){
        DPRINTF(LruCache, "Request blocked");
        needRetry = true;
        return false;
    }

    // we can not handle the request right now
    if (!owner->handleRequest(pkt, id)){
        DPRINTF(LruCache, "Request fail");
        needRetry = true;
        return false;
    }

    return true;
}

void LruCache::CPUSidePort::sendPacket(PacketPtr pkt)
{
    panic_if(blockedPkt != nullptr, "Never try to send when the port is blocked");

    DPRINTF(LruCache, "Send %s to CPU.\n", pkt->print());

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

void LruCache::CPUSidePort::recvFunctional(PacketPtr pkt)
{
    return owner->handleFunctional(pkt);
}

void LruCache::CPUSidePort::recvRespRetry()
{
    // as he asked for retry, we must have blocked packet
    assert(blockedPkt != nullptr);

    // clear the blocked packet
    PacketPtr pkt = blockedPkt;
    blockedPkt = nullptr;

    // now send it once again
    DPRINTF(LruCache, "Retrying response pkt $s.\n", pkt->print());
    sendPacket(pkt);

    trySendRetry();
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

void LruCache::MemSidePort::recvRangeChange()
{
    owner->sendRangeChange();
}

bool LruCache::MemSidePort::recvTimingResp(PacketPtr pkt)
{
    return owner->handleResponse(pkt);
}

void LruCache::MemSidePort::recvReqRetry()
{
    assert(blockedPkt != nullptr);   // it should be blocked

    // send the blocked Packet again
    PacketPtr pkt = blockedPkt;
    blockedPkt = nullptr;

    sendPacket(pkt);
}


/* -----------------------------------------------------------
    LruCacheParams
--------------------------------------------------------------*/

LruCache *LruCacheParams::create()
{
    return new LruCache(this);
}