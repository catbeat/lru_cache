#ifndef __MEM_CACHE_REPLACEMENT_POLICIES_DRRIP_RP_HH__
#define __MEM_CACHE_REPLACEMENT_POLICIES_DRRIP_RP_HH__

#include "mem/cache/replacement_policies/base.hh"
#include "base/sat_counter.hh"

struct DRRIPRPParams;

class DRRIPRP: public BaseReplacementPolicy
{
    protected:
        struct DRRIPReplData: ReplacementData
        {
            SatCounter rrpv;
            bool valid;

            DRRIPReplData(const int num_bits)
                :rrpv(num_bits), valid(false)
            {
                ;
            }
        };

        const int numOfRRPVBits;

        const bool hitPriority;

        const unsigned btp;

    public:

        typedef DRRIPRPParams Params;

        int *hitFirstClass, *accessFirstClass, *hitSecondClass, *accessSecondClass;

        DRRIPRP(const Params *p);
        ~DRRIPRP() {};

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