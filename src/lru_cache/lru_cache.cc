#include "lru_cache/lru_cache.hh"
#include "debug/LruCache.hh"

LruCache::LruCache(LruCacheParams *p)
  :MemObject(p), \
  instrPort(p->name+".instr_port", this), \
  dataPort(p->name+".data_port", this), \
  memPort(p->name+".mem_side", this), \
{
  ;
}

MasterPort &LruCache::getMasterPort(const std::string& name, PortID idx)
{
  panic_if(idx != InvalidPortID, "The object doesn't support vector parts");

  if (name == "mem_side"){
    return memPort;
  }
  else{
    return MemObject::getMasterPort(name, idx);
  }
}

SlavePort &LruCache::getSlavePort(const std::string &name, PortID idx)
{
  panic_if(idx != InvalidPortID, "The object doesn't support vector parts");

  if (name == "instr_port"){
    return instrPort;
  }
  else if (name == "data_port"){
    return dataPort;
  }
  else{
    return MemObject::getSlavePort(name, idx);
  }
}

bool LruCache::handleRequest(PacketPtr pkt)
{
  // we are waiting for a response of a previous request, so we can't handle new request right now
  if (blocked){
    return false;
  }
  else{
    DPRINTF(LruCache, "Got request for addr %#x\n", pkt->getAddr());

    // blocked as we should waiting for the response
    blocked = true;
    memPort.sendPacket(pkt);
    return true;
  }
}

void LruCache::handleFunctional(PacketPtr pkt)
{
  memPort.sendFunctional(pkt);
}

bool LruCache::handleResponse(PacketPtr pkt)
{
  // when receiving response, it should always be blocked
  assert(blocked);
  DPRINTF(LruCache, "Got response for the addr %#x\n", pkt->getAddr());

  // response get so unlock block
  blocked = false;

  // ask the corresponding cpu side port to send back packet to cpu
  if (pkt->req->isInstFetch()){
    instrPort.sendPacket(pkt);
  }
  else{
    dataPort.sendPacket(pkt);
  }

  instrPort.trySendRetry();
  dataPort.trySendRetry();

  return true;
}

void LruCache::sendRangeChange()
{
  instrPort.sendRangeChange();
  dataPort.sendRangeChange();
}

/* CPUSidePort */

AddrRangeList LruCache::CPUSidePort::getAddrRange()
{
  return owner->getAddrRange();
}

bool LruCache::CPUSidePort::recvTimingReq(PacketPtr pkt)
{
  // cpu can't handle current request right now
  if (!owner->handleRequest(pkt)){
    needRetry = true;       // we need a request retry in the future
    return false;
  }
  else{
    return true;
  }
}

void LruCache::CPUSidePort::recvFunctional(PacketPtr pkt)
{
  return owner->handleFunctional(pkt);
}

void LruCache::CPUSidePort::recvRespRetry()
{
  assert(blockedPacket != nullptr);

  PacketPtr pkt = blockedPacket;
  blockedPacket = nullptr;

  sendPacket(pkt);
}

void LruCache::CPUSidePort::sendPacket(PacketPtr pkt)
{
  panic_if(blockedPacket != nullptr, "never try to send when blocked");
  if (!sendTimingResp(pkt)){
    blockedPacket = pkt;
  }
}

void LruCache::CPUSidePort::trySendRetry(PacketPtr pkt)
{
  // first check whether a retry needed
  // next check whether the current response are done
  if (needRetry && blockedPacket == nullptr){
    needRetry = false;
    DPRINTF(LruCache, "Send retry request for %d\n", id);
    sendRetryReq();
  }
}

/* MemSidePort */

void LruCache::MemSidePort::recvRangeChange()
{
  return owner->sendRangeChange();
}

void LruCache::MemSidePort::sendPacket(PacketPtr pkt)
{
  panic_if(blockedPacket != nullptr, "never try to send if blocked");
  if (!sendTimingReq(pkt)){
    blockedPacket = pkt;
  }
}

void LruCache::MemSidePort::recvReqRetry()
{
  assert(blockedPacket != nullptr);

  // send it again
  PacketPtr pkt = blockedPacket;
  blockedPacket = nullptr;        // clear the blocked Packet
  sendPacket(pkt);

}

void LruCache::MemSidePort::recvTimingResp(PacketPtr pkt)
{
  // give out the pkt to assign to appropriate cpu side port
  return owner->handleResponse(pkt);
}

LruCache* LruCacheParams::create()
{
  return new LruCache(this);
}
