/**
 * @file rules.cpp
 *
 * This file contains consensus rules and restrictions.
 */

#include "omnicore/rules.h"

#include "omnicore/log.h"
#include "omnicore/omnicore.h"
#include "omnicore/utilsbitcoin.h"

#include "script/standard.h"

#include <stdint.h>

namespace mastercore
{

/** Whether we are loading activations during startup
 */
bool loadingActivations = false;

/** Default block for all feature activations is 999999 until set by ActivateFeature()
 */
int OP_RETURN_BLOCK = 999999;
int MSC_METADEX_BLOCK = 999999;
int MSC_BET_BLOCK = 999999;

/** A mapping of transaction types, versions and the blocks at which they are enabled.
 */
static const int txRestrictionsRules[][3] = {
    {MSC_TYPE_SIMPLE_SEND,                GENESIS_BLOCK,          MP_TX_PKT_V0},
    {MSC_TYPE_TRADE_OFFER,                MSC_DEX_BLOCK,          MP_TX_PKT_V1},
    {MSC_TYPE_ACCEPT_OFFER_BTC,           MSC_DEX_BLOCK,          MP_TX_PKT_V0},
    {MSC_TYPE_CREATE_PROPERTY_FIXED,      MSC_SP_BLOCK,           MP_TX_PKT_V0},
    {MSC_TYPE_CREATE_PROPERTY_VARIABLE,   MSC_SP_BLOCK,           MP_TX_PKT_V1},
    {MSC_TYPE_CLOSE_CROWDSALE,            MSC_SP_BLOCK,           MP_TX_PKT_V0},
    {MSC_TYPE_CREATE_PROPERTY_MANUAL,     MSC_MANUALSP_BLOCK,     MP_TX_PKT_V0},
    {MSC_TYPE_GRANT_PROPERTY_TOKENS,      MSC_MANUALSP_BLOCK,     MP_TX_PKT_V0},
    {MSC_TYPE_REVOKE_PROPERTY_TOKENS,     MSC_MANUALSP_BLOCK,     MP_TX_PKT_V0},
    {MSC_TYPE_CHANGE_ISSUER_ADDRESS,      MSC_MANUALSP_BLOCK,     MP_TX_PKT_V0},
    {MSC_TYPE_SEND_TO_OWNERS,             MSC_STO_BLOCK,          MP_TX_PKT_V0},
    {MSC_TYPE_METADEX_TRADE,              MSC_METADEX_BLOCK,      MP_TX_PKT_V0},
    {MSC_TYPE_METADEX_CANCEL_PRICE,       MSC_METADEX_BLOCK,      MP_TX_PKT_V0},
    {MSC_TYPE_METADEX_CANCEL_PAIR,        MSC_METADEX_BLOCK,      MP_TX_PKT_V0},
    {MSC_TYPE_METADEX_CANCEL_ECOSYSTEM,   MSC_METADEX_BLOCK,      MP_TX_PKT_V0},
    {MSC_TYPE_OFFER_ACCEPT_A_BET,         MSC_BET_BLOCK,          MP_TX_PKT_V0},

    // end of array marker, in addition to sizeof/sizeof
    {-1,-1},
};

/**
 * Checks, if the script type is allowed as input.
 */
bool IsAllowedInputType(int whichType, int nBlock)
{
    switch (whichType)
    {
        case TX_PUBKEYHASH:
            return true;

        case TX_SCRIPTHASH:
            return (P2SH_BLOCK <= nBlock || isNonMainNet());
    }

    return false;
}

/**
 * Checks, if the script type qualifies as output.
 */
bool IsAllowedOutputType(int whichType, int nBlock)
{
    switch (whichType)
    {
        case TX_PUBKEYHASH:
            return true;

        case TX_MULTISIG:
            return true;

        case TX_SCRIPTHASH:
            return (P2SH_BLOCK <= nBlock || isNonMainNet());

        case TX_NULL_DATA:
            return (OP_RETURN_BLOCK <= nBlock || isNonMainNet());
    }

    return false;
}

/**
 * Activates a feature at a specific blockheight, authorization has already been validated
 *
 * Note: Feature activations are consensus breaking.  It is not permitted to activate a feature within
 *       the next 2048 blocks (roughly 2 weeks), nor is it permitted to activate a feature further out
 *       than 12288 blocks (roughly 12 weeks) to ensure sufficient notice.
 *       This does not apply for activation during initialization (where loadingActivations is set true).
 */
bool ActivateFeature(int featureId, int activationBlock)
{
    PrintToLog("Feature activation requested (ID %d to go active as of block: %d)\n", featureId, activationBlock);

    // check activation block is allowed
    if (!loadingActivations && (GetHeight()+2048 < activationBlock || activationBlock > GetHeight()+12288)) {
        PrintToLog("Feature activation of ID %d refused due to notice checks\n", featureId);
        return false;
    }

    // check feature is recognized and activation is successful
    switch(featureId) {
        case OMNICORE_FEATURE_CLASS_C:
            OP_RETURN_BLOCK = activationBlock;
            PrintToLog("Feature activation of ID %d succeeded, OP_RETURN block is now %d\n", featureId, OP_RETURN_BLOCK);
            return true;
        case OMNICORE_FEATURE_METADEX:
            MSC_METADEX_BLOCK = activationBlock;
            PrintToLog("Feature activation of ID %d succeeded, MSC_METADEX_BLOCK is now %d\n", featureId, MSC_METADEX_BLOCK);
            return true;
        case OMNICORE_FEATURE_BETTING:
            MSC_BET_BLOCK = activationBlock;
            PrintToLog("Feature activation of ID %d succeeded, MSC_BET_BLOCK is now %d\n", featureId, MSC_BET_BLOCK);
            return true;
    }

    // unrecognized featureId
    PrintToLog("Feature activation of id %d refused due to unknown feature ID\n", featureId);
    return false;
}

/**
 * Checks, if the transaction type and version is supported and enabled.
 *
 * Certain transaction types are not live/enabled until some specific block height.
 * Certain transactions will be unknown to the client, i.e. "black holes" based on their version.
 *
 * The restrictions array is as such:
 *   type, block-allowed-in, top-version-allowed
 */
bool IsTransactionTypeAllowed(int txBlock, uint32_t txProperty, uint16_t txType, uint16_t version, bool bAllowNullProperty) {
    bool bAllowed = false;
    bool bBlackHole = false;
    unsigned int type;
    int block_FirstAllowed;
    unsigned short version_TopAllowed;

    // bitcoin as property is never allowed, unless explicitly stated otherwise
    if ((OMNI_PROPERTY_BTC == txProperty) && !bAllowNullProperty) return false;

    // everything is always allowed on Bitcoin's TestNet or with TMSC/TestEcosystem on MainNet
    if (isNonMainNet() || isTestEcosystemProperty(txProperty)) {
        bAllowed = true;
    }

    for (unsigned int i = 0; i < sizeof(txRestrictionsRules)/sizeof(txRestrictionsRules[0]); i++) {
        type = txRestrictionsRules[i][0];
        block_FirstAllowed = txRestrictionsRules[i][1];
        version_TopAllowed = txRestrictionsRules[i][2];

        if (txType != type) continue;

        if (version_TopAllowed < version) {
            PrintToLog("Black Hole identified !!! %d, %u, %u, %u\n", txBlock, txProperty, txType, version);

            bBlackHole = true;

            // TODO: what else?
            // ...
        }

        if (0 > block_FirstAllowed) break; // array contains a negative -- nothing's allowed or done parsing

        if (block_FirstAllowed <= txBlock) bAllowed = true;
    }

    return bAllowed && !bBlackHole;
}


} // namespace mastercore
