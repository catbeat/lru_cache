/**
 * @file
 * well, currently I just want to add a set dueling predictor, since I don't want
 * to change the basic last-level implementation of replacement policy and also tag.
 * So basically here I want to couple this tag called base_set_assoc_sd_predictor with 
 * another nominated policy that is DRRIP(May be later it also accept other policy 
 * that need a set duel predictor, I may make a another base replacement policy under
 * the current one). Still, in this way, we can detect error at compilation so basically 
 * it's under control. So until now, I haven't had a better idea yet and just go with 
 * this first.
 */

#ifndef BASE_SET_ASSOC_SD_PREDICTOR_HH
#define BASE_SET_ASSOC_SD_PREDICTOR_HH

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "base/logging.hh"
#include "base/types.hh"
#include "mem/cache/base.hh"
#include "mem/cache/cache_blk.hh"
#include "mem/cache/replacement_policies/base.hh"
#include "mem/cache/replacement_policies/replaceable_entry.hh"
#include "mem/cache/replacement_policies/drrip_rp.hh"
#include "mem/cache/tags/base.hh"
#include "mem/cache/tags/indexing_policies/base.hh"
#include "mem/cache/tags/indexing_policies/set_associative.hh"
#include "mem/packet.hh"
#include "params/BaseSetAssocSdPredictor.hh"

class BaseSetAssocSdPredictor: public BaseTags
{
    protected:
        /** The allocatable associativity of the cache (alloc mask). */
        unsigned allocAssoc;

        /** The cache blocks. */
        std::vector<CacheBlk> blks;

        /** Whether tags and data are accessed sequentially. */
        const bool sequentialAccess;

        /** Replacement policy */
        DRRIPRP *replacementPolicy;

        SetAssociative *indexingPolicy;

    public:
        int hitFirstClass, accessFirstClass, hitSecondClass, accessSecondClass;

        typedef BaseSetAssocSdPredictorParams Params;

        BaseSetAssocSdPredictor(Params *p);
        virtual ~BaseSetAssocSdPredictor() {};

        void tagsInit() override;

        void invalidate(CacheBlk *blk) override;

        /**
        * Access block and update replacement data. May not succeed, in which case
        * nullptr is returned. This has all the implications of a cache access and
        * should only be used as such. Returns the tag lookup latency as a side
        * effect.
        *
        * @param addr The address to find.
        * @param is_secure True if the target memory space is secure.
        * @param lat The latency of the tag lookup.
        * @return Pointer to the cache block if found.
        */
        CacheBlk* accessBlock(Addr addr, bool is_secure, Cycles &lat) override
        {
            CacheBlk *blk = findBlock(addr, is_secure);

            // Access all tags in parallel, hence one in each way.  The data side
            // either accesses all blocks in parallel, or one block sequentially on
            // a hit.  Sequential access with a miss doesn't access data.
            stats.tagAccesses += allocAssoc;
            int setIndex = indexingPolicy->extractSet(addr);
            
            if (setIndex%4 == 0){
                accessFirstClass++;
            }
            else if (setIndex%4 == 3){
                accessSecondClass++;
            }

            if (sequentialAccess) {
                if (blk != nullptr) {
                    stats.dataAccesses += 1;
                }
            } else {
                stats.dataAccesses += allocAssoc;
            }

            // If a cache hit
            if (blk != nullptr) {
                // Update number of references to accessed block
                blk->refCount++;

                if (setIndex%4 == 0){
                    hitFirstClass++;
                }
                else if (setIndex%4 == 3){
                    hitSecondClass++;
                }

                // Update replacement data of accessed block
                replacementPolicy->touch(blk->replacementData);
            }

            // The tag lookup latency is the same for a hit or a miss
            lat = lookupLatency;

            return blk;
        }

        /**
         * Find replacement victim based on address. The list of evicted blocks
         * only contains the victim.
         *
         * @param addr Address to find a victim for.
         * @param is_secure True if the target memory space is secure.
         * @param size Size, in bits, of new block to allocate.
         * @param evict_blks Cache blocks to be evicted.
         * @return Cache block to be replaced.
         */
        CacheBlk* findVictim(Addr addr, const bool is_secure,
                            const std::size_t size,
                            std::vector<CacheBlk*>& evict_blks) override
        {
            // Get possible entries to be victimized
            const std::vector<ReplaceableEntry*> entries =
                indexingPolicy->getPossibleEntries(addr);

            // Choose replacement victim from replacement candidates
            CacheBlk* victim = static_cast<CacheBlk*>(replacementPolicy->getVictim(
                                    entries));

            // There is only one eviction for this replacement
            evict_blks.push_back(victim);

            return victim;
        }

        /**
         * Insert the new block into the cache and update replacement data.
         *
         * @param pkt Packet holding the address to update
         * @param blk The block to update.
         */
        void insertBlock(const PacketPtr pkt, CacheBlk *blk) override
        {
            // Insert block
            BaseTags::insertBlock(pkt, blk);

            // Increment tag counter
            stats.tagsInUse++;

            // Update replacement policy
            replacementPolicy->reset(blk->replacementData);
        }

        /**
         * Limit the allocation for the cache ways.
         * @param ways The maximum number of ways available for replacement.
         */
        virtual void setWayAllocationMax(int ways) override
        {
            fatal_if(ways < 1, "Allocation limit must be greater than zero");
            allocAssoc = ways;
        }

        /**
         * Get the way allocation mask limit.
         * @return The maximum number of ways available for replacement.
         */
        virtual int getWayAllocationMax() const override
        {
            return allocAssoc;
        }

        /**
         * Regenerate the block address from the tag and indexing location.
         *
         * @param block The block.
         * @return the block address.
         */
        Addr regenerateBlkAddr(const CacheBlk* blk) const override
        {
            return indexingPolicy->regenerateAddr(blk->tag, blk);
        }

        void forEachBlk(std::function<void(CacheBlk &)> visitor) override {
            for (CacheBlk& blk : blks) {
                visitor(blk);
            }
        }

        bool anyBlk(std::function<bool(CacheBlk &)> visitor) override {
            for (CacheBlk& blk : blks) {
                if (visitor(blk)) {
                    return true;
                }
            }
            return false;
        }
};

#endif