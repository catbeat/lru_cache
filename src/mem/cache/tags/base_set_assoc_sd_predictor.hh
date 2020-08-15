/**
 * @file
 * well, currently I just want to add a set dueling predictor, since I don't want
 * to change the basic last-level implementation of replacement policy and also tag.
 * So basically here I want to couple this tag called base_set_assoc_sd_predictor with 
 * another nominated policy that is DRRIP(May be later it also accept other policy 
 * that need a set duel predictor, I may make a another base replacement policy under
 * the current one). But until now, I haven't had a better idea yet and just go with 
 * this first.
 */

#include "mem/cache/tags/base_set_assoc.hh"

class BaseSetAssocSDPredictor: public BaseSetAssoc
{
    protected:
        /** The allocatable associativity of the cache (alloc mask). */
        unsigned allocAssoc;

        /** The cache blocks. */
        std::vector<CacheBlk> blks;

        /** Whether tags and data are accessed sequentially. */
        const bool sequentialAccess;

        /** Replacement policy */
        BaseReplacementPolicy *replacementPolicy;

        float hitRatioFirstClass, hitRatioSecondClass;

        

}