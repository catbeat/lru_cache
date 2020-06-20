#ifndef __MEM_CACHE_TAGS_LRU_HH__
#define __MEM_CACHE_TAGS_LRU_HH__

#include "mem/cache/tags/base_set_assoc.hh"
#include "params/LRU.hh"
#include "mem/cache/base.hh"

class LRU : public BaseSetAssoc
{
  public:
    /** Convenience typedef. */
    typedef LRUParams Params;

    /**
     * Construct and initialize this tag store.
     */
    LRU(const Params *p);

    /**
     * Destructor
     */
    ~LRU() {}

    CacheBlk* accessBlock(Addr addr, bool is_secure, Cycles &lat);
    CacheBlk* findVictim(Addr addr);
    void insertBlock(PacketPtr pkt, CacheBlk *blk);
    void invalidate(CacheBlk *blk);
};

#endif // __MEM_CACHE_TAGS_LRU_HH__