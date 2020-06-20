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

BaseMasterPort &LruCache::getMasterPort(const std::string& name, PortID idx)
{
  panic_if(idx != InvalidPortID, "The object doesn't support vector parts");

  if (name == "mem_side"){
    return memPort;
  }
  else{
    return MemObject::getMasterPort(name, idx);
  }
}

BaserSlavePort &LruCache::getSlavePort(const std::string &name, PortID idx)
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

void LruCache::handleFunctional(PacketPtr pkt)
{
  memPort.sendFunctional(pkt);
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

bool LruCache::CPUSidePort::recvTimingReq(Packet pkt)
{

}

void LruCache::CPUSidePort::recvFunctional(PacketPtr pkt)
{
  return owner->handleFunctional(pkt);
}

/* MemSidePort */

void LruCache::MemSidePort::recvRangeChange()
{
  return owner->sendRangeChange();
}

