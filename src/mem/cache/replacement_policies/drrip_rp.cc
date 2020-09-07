#include "mem/cache/replacement_policies/drrip_rp.hh"

#include <cassert>
#include <memory>

#include "base/logging.hh"
#include "base/random.hh"
#include "params/DRRIPRP.hh"

DRRIPRP::DRRIPRP(const Params * p)
    :BaseReplacementPolicy(p), numOfRRPVBits(p->num_bits), hitPriority(p->hit_priority), btp(p->btp)
{
    fatal_if(numOfRRPVBits <= 0, "There should be at least one bit for RRPV counter bit");
}

void DRRIPRP::invalidate(const std::shared_ptr<ReplacementData>& replacementData ) const
{
    std::shared_ptr<DRRIPReplData> cast_replace_data = std::static_pointer_cast<DRRIPReplData>(replacementData);
    cast_replace_data->valid = false;
}

void DRRIPRP::touch(const std::shared_ptr<ReplacementData> & replacementData) const
{
    std::shared_ptr<DRRIPReplData> cast_replace_data = std::static_pointer_cast<DRRIPReplData>(replacementData);

    if (hitPriority){
        cast_replace_data->rrpv.reset();
    }
    else{
        cast_replace_data->rrpv--;
    }
}

void DRRIPRP::reset(const std::shared_ptr<ReplacementData> & replacementData) const
{
    std::shared_ptr<DRRIPReplData> cast_replace_data = std::static_pointer_cast<DRRIPReplData>(replacementData);

    cast_replace_data->rrpv.saturate();
    if ((*hitFirstClass)/(*accessFirstClass) < (*hitSecondClass)/(*accessSecondClass)){
        if (random_mt.random<unsigned>(1, 100) <= btp) {
            cast_replace_data->rrpv--;
        }
    }
    else{
        cast_replace_data->rrpv--;
    }
    cast_replace_data->valid = true;
}

ReplaceableEntry* DRRIPRP::getVictim( const ReplacementCandidates& candidates) const
{
    assert(candidates.size() > 0);

    ReplaceableEntry *victim = candidates[0];
    int victim_rrpv = std::static_pointer_cast<DRRIPReplData>(victim->replacementData)->rrpv;

    for (auto &candidate : candidates){
        std::shared_ptr<DRRIPReplData> Data = std::static_pointer_cast<DRRIPReplData>(candidate->replacementData);
        if (!Data->valid){
            return candidate;
        }

        if (Data->rrpv > victim_rrpv){
            victim = candidate;
            victim_rrpv = Data->rrpv;
        }
    }

    int diff = std::static_pointer_cast<DRRIPReplData>(victim->replacementData)->rrpv.saturate();

    if (diff > 0){
        for (auto &candidate : candidates){
            std::shared_ptr<DRRIPReplData> Data = std::static_pointer_cast<DRRIPReplData>(candidate->replacementData);
            Data->rrpv += diff;
        }
    
    }

    return victim;
}

std::shared_ptr<ReplacementData> DRRIPRP::instantiateEntry()
{
    return std::shared_ptr<ReplacementData>(new DRRIPReplData(numOfRRPVBits));
}

DRRIPRP* DRRIPRPParams::create()
{
    return new DRRIPRP(this);
}
