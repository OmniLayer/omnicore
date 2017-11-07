#ifndef OMNICORE_CONSENSUSHASH_H
#define OMNICORE_CONSENSUSHASH_H

#include "uint256.h"

namespace mastercore
{
/** Checks if a given block should be consensus hashed. */
bool ShouldConsensusHashBlock(int block);

/** Obtains a hash of all balances to use for consensus verification and checkpointing. Defaults to both ecosystems. */
uint256 GetConsensusHash(uint8_t ecosystem = 0);

/** Obtains a hash of the overall MetaDEx state (default for both ecosystems) or a specific orderbook (supply a property ID). */
uint256 GetMetaDExHash(uint8_t ecosystem = 0, const uint32_t propertyId = 0);

}

#endif // OMNICORE_CONSENSUSHASH_H
