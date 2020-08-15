#include "mem/cache/tags/base_set_assoc_sd_predictor.hh"

BaseSetAssocSdPredictor::BaseSetAssocSdPredictor(BaseSetAssocSdPredictorParams *p)
    :BaseTags(p), allocAssoc(p->assoc), blks(p->size/p->block_size), 
    sequentialAccess(p->sequentialAccess), 
    replacementPolicy(p->replacementPolicy)
{
    if (blkSize < 4 || !isPowerOf2(blkSize)) {
        fatal("Block size must be at least 4 and a power of 2");
    }

    
}

void BaseSetAssocSdPredictor::tagsInit()
{
    // Initialize all blocks
    for (unsigned blk_index = 0; blk_index < numBlocks; blk_index++) {
        // Locate next cache block
        CacheBlk* blk = &blks[blk_index];

        // Link block to indexing policy
        indexingPolicy->setEntry(blk, blk_index);

        // Associate a data chunk to the block
        blk->data = &dataBlks[blkSize*blk_index];

        // Associate a replacement data entry to the block
        blk->replacementData = replacementPolicy->instantiateEntry();
    }
}

void BaseSetAssocSdPredictor::invalidate(CacheBlk *blk)
{
    BaseTags::invalidate(blk);

    // Decrease the number of tags in use
    stats.tagsInUse--;

    // Invalidate replacement data
    replacementPolicy->invalidate(blk->replacementData);
}

BaseSetAssocSdPredictor * BaseSetAssocSdPredictorParams::create()
{
    // There must be a indexing policy
    fatal_if(!indexing_policy, "An indexing policy is required");

    return new BaseSetAssoc(this);
}