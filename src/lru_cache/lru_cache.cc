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


/* -----------------------------------------------------------
    LruCacheParams
--------------------------------------------------------------*/

LruCache *LruCacheParams::create()
{
    return new LruCache(this);
}