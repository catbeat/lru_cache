#include "mem/cache/tags/lru.hh"

#include "debug/CacheRepl.hh"
#include "mem/cache/base.hh"

LRU::LRU(const Params *p)
    : BaseSetAssoc(p)
{
}

CacheBlk*
LRU::accessBlock(Addr addr, bool is_secure, Cycles &lat)
{
    CacheBlk *blk = BaseSetAssoc::accessBlock(addr, is_secure, lat);

    if (blk != nullptr) {
        // move this block to head of the MRU list
        sets[blk->set].moveToHead(blk);
        DPRINTF(CacheRepl, "set %x: moving blk %x (%s) to MRU\n",
                blk->set, regenerateBlkAddr(blk->tag, blk->set),
                is_secure ? "s" : "ns");
    }

    return blk;
}

CacheBlk*
LRU::findVictim(Addr addr)
{
    int set = extractSet(addr);
    // grab a replacement candidate
    BlkType *blk = nullptr;
    for (int i = assoc - 1; i >= 0; i--) {
        BlkType *b = sets[set].blks[i];
        if (b->way < allocAssoc) {
            blk = b;
            break;
        }
    }
    assert(!blk || blk->way < allocAssoc);

    if (blk && blk->isValid()) {
        DPRINTF(CacheRepl, "set %x: selecting blk %x for replacement\n",
                set, regenerateBlkAddr(blk->tag, set));
    }

    return blk;
}

void
LRU::insertBlock(PacketPtr pkt, CacheBlk *blk)
{
    BaseSetAssoc::insertBlock(pkt, blk);

    int set = extractSet(pkt->getAddr());
    sets[set].moveToHead(blk);
}

void
LRU::invalidate(CacheBlk *blk)
{
    BaseSetAssoc::invalidate(blk);

    // should be evicted before valid blocks
    int set = blk->set;
    sets[set].moveToTail(blk);
}

LRU*
LRUParams::create()
{
    return new LRU(this);
}
