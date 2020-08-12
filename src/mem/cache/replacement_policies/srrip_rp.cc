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

