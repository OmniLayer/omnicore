/**
 * @file auditor.cpp
 *
 * This file contains code for the auditor module.
 */

#include "omnicore/auditor.h"
#include "omnicore/fees.h"
#include "omnicore/log.h"
#include "omnicore/mdex.h"
#include "omnicore/omnicore.h"
#include "omnicore/sp.h"
#include "main.h"

#include <boost/lexical_cast.hpp>

using namespace mastercore;

bool auditorEnabled = false; // enable with --enableauditor
bool auditBalanceChanges = false; // enable with --auditbalancechanges
bool auditDevOmni = false; // enable with --auditdevomni

std::map<uint32_t, int64_t> mapPropertyTotals;
std::map<string, CMPTally> mapBalancesCache;

uint32_t auditorPropertyCountMainEco = 0;
uint32_t auditorPropertyCountTestEco = 0;
int64_t lastBlockProcessed = -1;

/* This function initializes the auditor
 */
void mastercore::Auditor_Initialize()
{
    // Log auditor startup
    PrintToAuditLog("##################################################################################\n");
    PrintToAuditLog("Auditor initializing...\n");

    // Initialize property totals map
    unsigned int propertyId;
    unsigned int nextPropIdMainEco = GetNextPropertyId(true);
    unsigned int nextPropIdTestEco = GetNextPropertyId(false);
    for (propertyId = 1; propertyId < nextPropIdMainEco; propertyId++) {
        mapPropertyTotals.insert(std::pair<uint32_t,int64_t>(propertyId,SafeGetTotalTokens(propertyId)));
    }
    for (propertyId = 2147483650; propertyId < nextPropIdTestEco; propertyId++) {
        mapPropertyTotals.insert(std::pair<uint32_t,int64_t>(propertyId,SafeGetTotalTokens(propertyId)));
    }

    // Initialize property counts
    auditorPropertyCountMainEco = nextPropIdMainEco - 1;
    auditorPropertyCountTestEco = nextPropIdTestEco - 1;

    // Audit the MetaDEx after loading from persistence (nothing to audit for fresh parse)
    for (int i = 1; i<100; ++i) { // stop after 100 bad trades - indicates significant bug
        uint256 badTrade = SearchForBadTrades();
        if (badTrade == 0) {
            break;
        } else {
            AuditFail(strprintf("Auditor has detected an invalid trade (txid: %s) present in the MetaDEx during initialization\n", badTrade.GetHex().c_str()));
        }
    }

    // If balance change auditing is enabled (--auditbalancechanges) populate the balances cache
    if (auditBalanceChanges) {
        PrintToAuditLog("Balance auditing is enabled.\n");
        mapBalancesCache = mp_tally_map;
    } else {
        PrintToAuditLog("Balance auditing is disabled.\n");
    }

    PrintToAuditLog("Auditor initialized\n");
}

/* This function reinitializes the auditor after a chain reorg/orphan
 */
void mastercore::Auditor_NotifyChainReorg(int nWaterlineBlock)
{
    PrintToAuditLog("Auditor was notified that the state has been rolled back to block %d due to a reorg/orphan. The auditor will now restart\n", nWaterlineBlock);
    // reset auditor state and reinitialize
    auditorPropertyCountMainEco = 0;
    auditorPropertyCountTestEco = 0;
    lastBlockProcessed = -1;
    mapPropertyTotals.clear();
    mapBalancesCache.clear();
    Auditor_Initialize();
}

/* This function handles notification of a balance change
 */
void mastercore::Auditor_NotifyBalanceChangeRequested(const std::string& address, int64_t amount, uint32_t propertyId, TallyType tallyType, const std::string& type, uint256 txid, const std::string& caller, bool processed)
{
    if (!auditBalanceChanges) {
        return;
    }

    if (address.empty()) {
        AuditFail(strprintf("Auditor was notified of a balance change request with an invalid address (%s) by %s:%s (caller %s)\n", address, type, txid.GetHex(), caller));
    }
    if ((propertyId == 0) || ((propertyId > auditorPropertyCountMainEco) && (propertyId < 2147483647)) || (propertyId > auditorPropertyCountTestEco)) {
        AuditFail(strprintf("Auditor was notified of a balance change request with an invalid property ID (%u) by %s:%s (caller %s)\n", propertyId, type, txid.GetHex(), caller));
    }
    if (tallyType > TALLY_TYPE_COUNT) {
        AuditFail(strprintf("Auditor was notified of a balance change request with an invalid tallyType (%d) by %s:%s (caller %s)\n", tallyType, type, txid.GetHex(), caller));
    }

    if (processed) { // the balance change request was processed, change the cache accordingly
        std::map<std::string, CMPTally>::iterator search_it = mapBalancesCache.find(address);
        if (search_it == mapBalancesCache.end()) { // address doesn't exist, insert
            search_it = ( mapBalancesCache.insert(std::make_pair(address,CMPTally())) ).first;
        }
        CMPTally &tally = search_it->second;
        if(!tally.updateMoney(propertyId, amount, tallyType)) {
            AuditFail(strprintf("Auditor was unable to modify the balances cache for %s, %ld of SP%u on tally %d, (%s:%s, %s)\n", address, amount, propertyId, tallyType, type, txid.GetHex(), caller));
        }
    }

    if (!auditDevOmni && (type == "Award Dev OMNI")) {
        return;
    } else {
        PrintToAuditLog(strprintf("Balance Update %s: %s, %ld of SP%u on tally %d, (%s:%s, %s)\n", processed ? "Accepted":"REJECTED", address, amount, propertyId, tallyType, type, txid>0 ? txid.GetHex():"N/A", caller));
    }
}

/* This function handles auditor functions for the beginning of a block
 */
void mastercore::Auditor_NotifyBlockStart(CBlockIndex const * pBlockIndex)
{
    // DEBUG : Log that auditor informed a new block
    if (omni_debug_auditor_verbose) {
        PrintToAuditLog("Auditor was notified block %d has begun processing\n", pBlockIndex->nHeight);
    }

    // Is this the first block processed or is this block number as expected?
    if (lastBlockProcessed == -1) {
        lastBlockProcessed = pBlockIndex->nHeight;
    } else {
        if (lastBlockProcessed+1 == pBlockIndex->nHeight) {
            if (omni_debug_auditor_verbose) {
                PrintToAuditLog("Auditor did not detect processing of an out of order block (%d)\n", pBlockIndex->nHeight);
            }
            lastBlockProcessed = pBlockIndex->nHeight;
        } else { // audit failure - case should never occur but safety check
            AuditFail(strprintf("Auditor has detected processing of a block in a non-sequential order (%d)\n", pBlockIndex->nHeight));
        }
    }
}

/* This function handles auditor functions for the end of a block
 */
void mastercore::Auditor_NotifyBlockFinish(CBlockIndex const * pBlockIndex)
{
    // DEBUG : Log that auditor informed block processing completed
    if (omni_debug_auditor_verbose) {
        PrintToAuditLog("Auditor was notified block %d has been processed\n", pBlockIndex->nHeight);
    }

    // Verify that the finish notification is for the correct block (otherwise the auditor has somehow missed a/some block(s)
    if ((pBlockIndex->nHeight != lastBlockProcessed) && (pBlockIndex->nHeight != 0)) { // audit failure
        AuditFail(strprintf("Auditor has received unexpected notification of completion of block %d\n", pBlockIndex->nHeight));
    }

    // Compare the property totals from the state with the auditor property totals map
    uint32_t mismatch = ComparePropertyTotals();
    if (mismatch == 0) {
        if (omni_debug_auditor_verbose) {
            PrintToAuditLog("Auditor did not detect any inconsistencies in property token totals following block %d\n", pBlockIndex->nHeight);
        }
    } else { // audit failure
        AuditFail(strprintf("Auditor has detected inconsistencies in the amount of tokens for property %u following block %d\n", mismatch, pBlockIndex->nHeight));
    }

    // Compare the number of properties in state with cache
    bool comparePropertyCount = ComparePropertyCounts();
    if (comparePropertyCount) {
        if (omni_debug_auditor_verbose) {
            PrintToAuditLog("Auditor did not detect any inconsistencies in the total number of properties following block %d\n", pBlockIndex->nHeight);
        }
    } else { // audit failure
        AuditFail(strprintf("Auditor has detected inconsistencies in the total number of properties following block %d\n", pBlockIndex->nHeight));
    }

    // Check the MetaDEx does not have any bad trades present
    for (int i = 1; i<100; ++i) { // stop after 100 bad trades - indicates significant bug
        uint256 badTrade = SearchForBadTrades();
        if (badTrade == 0) {
            if (omni_debug_auditor_verbose) {
                PrintToAuditLog("Auditor did not detect any problems in the MetaDEx maps following block %d\n", pBlockIndex->nHeight);
            }
            break;
        } else {
            AuditFail(strprintf("Auditor has detected an invalid trade (txid: %s) present in the MetaDEx following block %d\n", pBlockIndex->nHeight, badTrade.GetHex().c_str()));
        }
    }

    // If balance change auditing is enabled, compare cache with state
    if (auditBalanceChanges) {
        std::string compareFailures;
        bool cacheMatch = CompareBalances(compareFailures);
        if (cacheMatch) {
            if (omni_debug_auditor_verbose) {
                PrintToAuditLog("Auditor did not detect any inconsistencies in the balance cache following block %d\n", pBlockIndex->nHeight);
            }
        } else { // audit failure
            AuditFail(strprintf("Auditor has detected inconsistencies in the balance cache following block %d\n%s", pBlockIndex->nHeight, compareFailures));
        }
    }
}

/* This function updates the property totals cache when an increase occurs
 */
void mastercore::Auditor_NotifyPropertyTotalChanged(bool increase, uint32_t propertyId, int64_t amount, std::string const& reasonStr)
{
    if (propertyId == 0) {
        AuditFail("Auditor was notified of a property total change for property ID zero\n");
    }
    if (amount <= 0) {
        AuditFail(strprintf("Auditor was notified of a property total change with an invalid amount (%ld) for property %u due to %s\n", amount, propertyId, reasonStr.c_str()));
    }
    std::map<uint32_t,int64_t>::iterator it = mapPropertyTotals.find(propertyId);
    if (it != mapPropertyTotals.end()) {
        int64_t cachedValue = it->second;
        int64_t stateValue = SafeGetTotalTokens(propertyId);
        int64_t newValue;
        if (increase) {
            newValue = cachedValue + amount;
        } else {
            newValue = cachedValue - amount;
        }
        if (newValue == stateValue) { // additional sanity check - cached + amount increased should equal state total
            it->second = newValue;
            if (!auditDevOmni && ((reasonStr.length() > 8) && (reasonStr.substr(0,8) == "Dev OMNI"))) {
                // skip logging
            } else {
                PrintToAuditLog("Auditor was notified of %s of %ld tokens for property %u due to %s\n", increase ? "an increase" : "a decrease", amount, propertyId, reasonStr.c_str());
            }
        } else { // audit failure - sanity check failed
            std::string details = strprintf("Type:%s  Amount cached:%ld  Amount changed:%ld  Amount updated:%ld  Amount state:%ld\n", increase ? "increase" : "decrease", cachedValue, amount, newValue, stateValue);
            AuditFail(strprintf("Auditor has detected inconsistencies when attempting to update the total amount of tokens for property %u\n%s",propertyId,details));
        }
    } else { // audit failure - property not found
        AuditFail(strprintf("Auditor has detected inconsistencies when attempting to update the total amount of tokens for non-existent property %u\n",propertyId));
    }
}

/* This function adds a new property to the cached totals map
 */
void mastercore::Auditor_NotifyPropertyCreated(uint32_t propertyId)
{
    if (propertyId == 0) {
        AuditFail("Auditor was notified of a property creation with property ID zero\n");
    }
    std::map<uint32_t,int64_t>::iterator it = mapPropertyTotals.find(propertyId);
    if((it == mapPropertyTotals.end()) && ((propertyId == auditorPropertyCountMainEco + 1) || (propertyId == auditorPropertyCountTestEco + 1))) { // check it does not already exist and is sequential
        mapPropertyTotals.insert(std::pair<uint32_t,int64_t>(propertyId,0));
        if (!isTestEcosystemProperty(propertyId)) {
            auditorPropertyCountMainEco += 1;
        } else {
            auditorPropertyCountTestEco += 1;
        }
        PrintToAuditLog("Auditor was notified of a new property creation with ID %u\n", propertyId);
    } else { // audit failure - new property, it should not already exist in cached totals map
        AuditFail(strprintf("Auditor has detected a duplicated or non sequential property ID when attempting to insert property %u\n",propertyId));
    }
}

/* This function searches the MetaDEx maps for any invalid trades
 */
uint256 SearchForBadTrades()
{
    for (md_PropertiesMap::iterator my_it = metadex.begin(); my_it != metadex.end(); ++my_it) {
        md_PricesMap & prices = my_it->second;
        for (md_PricesMap::iterator it = prices.begin(); it != prices.end(); ++it) {
            const rational_t price = (it->first);
            md_Set & indexes = (it->second);
            for (md_Set::iterator it = indexes.begin(); it != indexes.end(); ++it) {
                CMPMetaDEx obj = *it;
                if (0 >= obj.getAmountDesired()) return obj.getHash();
                if (0 >= obj.getAmountForSale()) return obj.getHash();
                if (0 >= price) return obj.getHash();
                if ("0.00000000000000000000000000000000000000000000000000" >= obj.displayFullUnitPrice()) return obj.getHash();
            }
        }
    }
    return 0;
}

/* This function compares the cached number of properties with the state
 */
bool ComparePropertyCounts()
{
    unsigned int nextPropIdMainEco = GetNextPropertyId(true);
    unsigned int nextPropIdTestEco = GetNextPropertyId(false);
    if ((nextPropIdMainEco == auditorPropertyCountMainEco + 1) && (nextPropIdTestEco == auditorPropertyCountTestEco + 1)) {
         return true;
    } else {
         return false;
    }
}

/* This function compares the current state with the cached property totals map
 */
uint32_t ComparePropertyTotals()
{
    unsigned int propertyId;
    unsigned int nextPropIdMainEco = GetNextPropertyId(true);
    unsigned int nextPropIdTestEco = GetNextPropertyId(false);
    for (propertyId = 1; propertyId < nextPropIdMainEco; propertyId++) {
        std::map<uint32_t,int64_t>::iterator it = mapPropertyTotals.find(propertyId);
        if(it == mapPropertyTotals.end()) { return propertyId; } else { if (SafeGetTotalTokens(propertyId) != it->second) return propertyId; }
    }
    for (propertyId = 2147483650; propertyId < nextPropIdTestEco; propertyId++) {
        std::map<uint32_t,int64_t>::iterator it = mapPropertyTotals.find(propertyId);
        if(it == mapPropertyTotals.end()) { return propertyId; } else { if (SafeGetTotalTokens(propertyId) != it->second) return propertyId; }
    }
    return 0;
}

/* This function obtains the total number of tokens for propertyId from the balances tally.
 *
 * Note: similar to getTotalTokens, however getTotalTokens utilizes a shortcut to request the
 * number of tokens originally issued from the SP object for a fixed issuance property since the
 * number of tokens should never change.  However for auditing purposes we want to verify how many
 * tokens exist *now* so must avoid using such a shortcut.
 */
int64_t SafeGetTotalTokens(uint32_t propertyId)
{
  LOCK(cs_tally);
  int64_t totalTokens = 0;
  for(std::map<string, CMPTally>::iterator my_it = mp_tally_map.begin(); my_it != mp_tally_map.end(); ++my_it) {
      string address = (my_it->first).c_str();
      totalTokens += getMPbalance(address, propertyId, BALANCE);
      totalTokens += getMPbalance(address, propertyId, SELLOFFER_RESERVE);
      totalTokens += getMPbalance(address, propertyId, METADEX_RESERVE);
      if (propertyId<3) totalTokens += getMPbalance(address, propertyId, ACCEPT_RESERVE);
  }
  int64_t feesCached = p_feecache->GetCachedAmount(propertyId);
  totalTokens += feesCached;

  return totalTokens;
}

/* This function handles an audit failure
 */
void AuditFail(const std::string& msg)
{
    PrintToAuditLog("%s", msg);
    if (!GetBoolArg("-overrideforcedshutdown", false)) {
        AbortNode("Shutting down due to audit failure. " + msg + ". Please check omnicore.log for further details");
    }
}

/* This function compres state balances with cached balances
 *
 * Note: this is a significant performance hit, but auditing balances is an on-demand action
 */
bool CompareBalances(std::string &compareFailures)
{
    for (std::map<string, CMPTally>::iterator my_it = mapBalancesCache.begin(); my_it != mapBalancesCache.end(); ++my_it) {
        string address = my_it->first.c_str();
        CMPTally &tally = my_it->second;
        tally.init();
        uint32_t propertyId;
        while (0 != (propertyId = tally.next())) {
            if (getMPbalance(address, propertyId, BALANCE) != tally.getMoney(propertyId, BALANCE)) {
                compareFailures += strprintf("CACHE DIFF: %s, SP%u, Balance Tally, State Balance: %ld, Cache Balance: %ld\n", address, propertyId, getMPbalance(address, propertyId, BALANCE), tally.getMoney(propertyId, BALANCE));
            }
            if (getMPbalance(address, propertyId, SELLOFFER_RESERVE) != tally.getMoney(propertyId, SELLOFFER_RESERVE)) {
                compareFailures += strprintf("CACHE DIFF: %s, SP%u, SellOffer Tally, State Balance: %ld, Cache Balance: %ld\n", address, propertyId, getMPbalance(address, propertyId, SELLOFFER_RESERVE), tally.getMoney(propertyId, SELLOFFER_RESERVE));
            }
            if (getMPbalance(address, propertyId, METADEX_RESERVE) != tally.getMoney(propertyId, METADEX_RESERVE)) {
                compareFailures += strprintf("CACHE DIFF: %s, SP%u, MetaDEx Tally, State Balance: %ld, Cache Balance: %ld\n", address, propertyId, getMPbalance(address, propertyId, METADEX_RESERVE), tally.getMoney(propertyId, METADEX_RESERVE));
            }
            if (getMPbalance(address, propertyId, ACCEPT_RESERVE) != tally.getMoney(propertyId, ACCEPT_RESERVE)) {
                compareFailures += strprintf("CACHE DIFF: %s, SP%u, Accept Tally, State Balance: %ld, Cache Balance: %ld\n", address, propertyId, getMPbalance(address, propertyId, ACCEPT_RESERVE), tally.getMoney(propertyId, ACCEPT_RESERVE));
            }
            if (getMPbalance(address, propertyId, PENDING) != tally.getMoney(propertyId, PENDING)) {
                compareFailures += strprintf("CACHE DIFF: %s, SP%u, Pending Tally, State Balance: %ld, Cache Balance: %ld\n", address, propertyId, getMPbalance(address, propertyId, PENDING), tally.getMoney(propertyId, PENDING));
            }
        }
    }

    // search for addresses that are not in the cache - if there are no compareFailures at this point and sizes are the same that's a shortcut to know there are no missing cache entries
    if ((!compareFailures.empty()) || (mp_tally_map.size() != mapBalancesCache.size())) {
        for (std::map<std::string, CMPTally>::iterator my_it = mp_tally_map.begin(); my_it != mp_tally_map.end(); ++my_it) {
            string address = my_it->first.c_str();
            std::map<std::string, CMPTally>::iterator it = mapBalancesCache.find(address);
            if (it == mapBalancesCache.end()) {
                // verify this isn't an empty address
                CMPTally *tally = getTally(address);
                if (tally == NULL) {
                    tally->init();
                    uint32_t propertyId;
                    while (0 != (propertyId = tally->next())) {
                        if ((getMPbalance(address, propertyId, BALANCE) != 0) ||
                            (getMPbalance(address, propertyId, SELLOFFER_RESERVE) != 0) ||
                            (getMPbalance(address, propertyId, METADEX_RESERVE) != 0) ||
                            (getMPbalance(address, propertyId, ACCEPT_RESERVE) != 0) ||
                            (getMPbalance(address, propertyId, PENDING) != 0)) {
                                compareFailures += strprintf("CACHE MISS: %s is missing from balances cache\n", address);
                        }
                    }
                }
            }
        }
    }

    if (!compareFailures.empty()) {
        return false;
    } else {
        return true;
    }
}

