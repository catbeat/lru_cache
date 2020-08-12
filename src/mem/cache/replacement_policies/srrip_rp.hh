#ifndef __MEM_CACHE_REPLACEMENT_POLICIES_SRRIP_RP_HH__
#define __MEM_CACHE_REPLACEMENT_POLICIES_SRRIP_RP_HH__

#include "mem/cache/replacement_policies/base.hh"
#include "base/sat_counter.hh"

struct SRRIPRPParams;

class SRRIPRP: public BaseReplacementPolicy
{
    protected:
        struct SRRIPReplData: ReplacementData
        {
            SatCounter rrpv;
            bool valid;

            SRRIPReplData(const int num_bits)
                :rrpv(num_bits), valid(false)
            {
                ;
            }
        };

        const int numOfRRPVBits;

        const bool hitPriority;

    public:

        typedef SRRIPRPParams Params;

        SRRIPRP(const Params *p);
        ~SRRIPRP() {};

        void invalidate(const std::shared_ptr<ReplacementData>&
                                                replacement_data) const override;

        void touch(const std::shared_ptr<ReplacementData>&
                                                replacement_data) const override;

        void reset(const std::shared_ptr<ReplacementData>&
                                                replacement_data) const override;

        ReplaceableEntry* getVictim(
                           const ReplacementCandidates& candidates) const override;

        std::shared_ptr<ReplacementData> instantiateEntry() override;

    
};

#endif