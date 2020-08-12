#ifndef SSRIP_HH
#define SSRIP_HH

#include <math.h>
#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include "base/bitfield.hh"
#include "base/intmath.hh"
#include "base/logging.hh"
#include "base/statistics.hh"
#include "base/types.hh"
#include "mem/cache/cache_blk.hh"
#include "mem/cache/tags/base.hh"
#include "mem/packet.hh"
#include "params/SSRIP.hh"

class BaseCache;
class ReplaceableEntry;

typedef uint32_t CachesMask;

class SRRIPBlk: public CacheBlk
{
    public:
        SRRIPBlk()
         :CacheBlk(), prev(nullptr), next(nullptr), rrpv(pow(2, rrpv_maxBit)-1)
        {}

        SRRIPBlk *prev;
        SRRIPBlk *next;

        typedef int RRPV;

        RRPV rrpv;
        int rrpv_maxBit = 3;

        CachesMask inCachesMask;

        std::string print() const override;
};

class SRRIP: public BaseTags
{
    public:
        typedef SRRIPBlk BlkType;

    protected:
        SRRIPBlk *blks;

        SRRIPBlk *head;
        SRRIPBlk *tail;

        struct PairHash
        {
            template <class T1, class T2>
            std::size_t operator()(const std::pair<T1, T2> &p) const
            {
                return std::hash<T1>()(p.first) ^ std::hash<T2>()(p.second);
            }
        };

        typedef std::pair<Addr, bool> TagHashKey;
        typedef std::unordered_map<TagHashKey, SRRIPBlk *, PairHash> TagHash;

        /** The address hash table. */
        TagHash tagHash;

    public:
        typedef SSRIPParams Params;

        SSRIP(Params *p);
        ~SSRIP();

        void tagsInit() override;

        CacheBlk *findBlock(Addr addr, bool is_secure) const override;
        ReplaceableEntry* findBlockBySetAndWay(int set, int way) const override;
        void setWayAllocationMax(int ways) override;
        int getWayAllocationMax() const override;
        void invalidate(CacheBlk *blk) override;
        CacheBlk* findVictim(Addr addr, const bool is_secure,
                                 const std::size_t size,
                                 std::vector<CacheBlk*>& evict_blks) override;
        CacheBlk* accessBlock(Addr addr, bool is_secure, Cycles &lat) override;
        void insertBlock(const PacketPtr pkt, CacheBlk *blk) override;

        Addr extractTag(const Addr addr) const override;
        Addr regenerateBlkAddr(const CacheBlk* blk) const override;
        void forEachBlk(std::function<void(CacheBlk &)> visitor) override;
        bool anyBlk(std::function<bool(CacheBlk &)> visitor) override;

};

#endif