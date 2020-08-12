#include "mem/cache/replacement_policies/srrip_rp.hh"

#include <cassert>
#include <memory>

#include "base/logging.hh"
#include "base/random.hh"
#include "params/SRRIPRP.hh"

SRRIPRP(SRRIPRPParams *p)
    :numOfRRPVBits(p->num_bits), hitPriority(p->hit_priority)
{
    fatal_if(numOfRRPVBits <= 0, "There should be at least one bit for RRPV counter bit");
}

void SRRIPRP::invalidate(const std::shared_ptr<ReplacementData>& replacementData )
{
    std::shared_ptr<SRRIPReplData> cast_replace_data = std::static_pointer_cast<SRRIPReplData>(replacementData);
    cast_replace_data->valid = false;
}

void SRRIPRP::touch(const std::shared_ptr<ReplacementData> & replacementData)
{
    std::shared_ptr<SRRIPReplData> cast_replace_data = std::static_pointer_cast<SRRIPReplData>(replacementData);

    if (hitPriority){
        cast_replace_data->rrpv.reset();
    }
    else{
        cast_replace_data->rrpv--;
    }
}

void SRRIPRP::reset(const std::shared_ptr<ReplacementData> & replacementData)
{
    std::shared_ptr<SRRIPReplData> cast_replace_data = std::static_pointer_cast<SRRIPReplData>(replacementData);

    cast_replace_data->rrpv.saturate();
    cast_replace_data->rrpv--;
    cast_replace_data->valid = true;
}

ReplaceableEntry* SRRIPRP::getVictim( const ReplacementCandidates& candidates)
{
    assert(candidates.size() > 0);

    ReplaceableEntry *victim = candidates[0];
    int victim_rrpv = std::static_pointer_cast<SRRIPReplData>(victim->replacementData)->rrpv;

    for (auto &candidate : candidates){
        std::share_ptr<SRRIPReplData> Data = std::static_pointer_cast<SRRIPReplData>(candidate->replacementData);
        if (!Data->valid){
            return candidate;
        }

        if (Data->rrpv > victim_rrpv){
            victim = candidate;
            victim_rrpv = Data->rrpv;
        }
    }

    int diff = std::static_pointer_cast<SRRIPReplData>(victim->replacementData)->rrpv.saturate();

    if (diff > 0){
        for (auto &candidate : candidates){
            std::share_ptr<SRRIPReplData> Data = std::static_pointer_cast<SRRIPReplData>(candidate->replacementData);
            Data->rrpv += diff;
        }
    
    }

    return victim;
}
