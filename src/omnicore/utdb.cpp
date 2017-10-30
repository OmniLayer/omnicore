/**
 * @file utdb.cpp
 *
 * This file contains functionality for the unique tokens database.
 */

#include "omnicore/utdb.h"

#include "omnicore/omnicore.h"
#include "omnicore/errors.h"
#include "omnicore/log.h"

#include "main.h"

#include "leveldb/db.h"

#include <stdint.h>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

/* Extracts the property ID from a DB key
 */
uint32_t CMPUniqueTokensDB::GetPropertyIdFromKey(const std::string& key)
{
    std::vector<std::string> vPropertyId;
    boost::split(vPropertyId, key, boost::is_any_of("_"), boost::token_compress_on);
    assert(vPropertyId.size() == 2); // if size !=2 then we cannot trust the data in the DB and we must halt
    return boost::lexical_cast<uint32_t>(vPropertyId[0]);
}

/* Extracts the range from a DB key
 */
void CMPUniqueTokensDB::GetRangeFromKey(const std::string& key, int64_t *start, int64_t *end)
{
   std::vector<std::string> vPropertyId;
   boost::split(vPropertyId, key, boost::is_any_of("_"), boost::token_compress_on);
   assert(vPropertyId.size() == 2); // if size !=2 then we cannot trust the data in the DB and we must halt

   std::vector<std::string> vRanges;
   boost::split(vRanges, vPropertyId[1], boost::is_any_of("-"), boost::token_compress_on);
   assert(vRanges.size() == 2); // if size !=2 then we cannot trust the data in the DB and we must halt

   *start = boost::lexical_cast<int64_t>(vRanges[0]);
   *end = boost::lexical_cast<int64_t>(vRanges[1]);
}

/* Gets the range a unique token is in
 */
std::pair<int64_t,int64_t> CMPUniqueTokensDB::GetRange(const uint32_t &propertyId, const int64_t &tokenId)
{
    assert(pdb);
    leveldb::Iterator* it = NewIterator();

    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        if (propertyId != GetPropertyIdFromKey(it->key().ToString())) continue;

        int64_t start, end;
        GetRangeFromKey(it->key().ToString(), &start, &end);
        if (tokenId >= start && tokenId <= end) {
            delete it;
            return std::make_pair(start, end);
        }
    }

    delete it;
    return std::make_pair(0,0); // token not found, return zero'd range
}

/* Checks if the range of tokens is contiguous (ie owned by a single address)
 */
bool CMPUniqueTokensDB::IsRangeContiguous(const uint32_t &propertyId, const int64_t &rangeStart, const int64_t &rangeEnd)
{

    assert(pdb);
    leveldb::Iterator* it = NewIterator();

    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        if (propertyId != GetPropertyIdFromKey(it->key().ToString())) continue;

        int64_t start, end;
        GetRangeFromKey(it->key().ToString(), &start, &end);

        if (rangeStart >= start && rangeStart <= end) {
            delete it;
            if (rangeEnd >= rangeStart && rangeEnd <= end) {
                return true;
            } else {
                return false; // the start ID falls within this range but the end ID does not - not owned by a single address
            }
        }
    }

    delete it;
    return false; // range doesn't exist
}

/* Moves a range of tokens (returns false if not able to move)
 */
bool CMPUniqueTokensDB::MoveUniqueTokens(const uint32_t &propertyId, const int64_t &tokenIdStart, const int64_t &tokenIdEnd, const std::string &from, const std::string &to)
{
    if (msc_debug_utdb) PrintToLog("%s(): %d:%d:%d:%s:%s, line %d, file: %s\n", __FUNCTION__, propertyId, tokenIdStart, tokenIdEnd, from, to, __LINE__, __FILE__);

    assert(pdb);

    // check that 'from' owns both the start and end token and that the range is contiguous (owns the entire range)
    std::string startOwner = GetUniqueTokenOwner(propertyId, tokenIdStart);
    std::string endOwner = GetUniqueTokenOwner(propertyId, tokenIdEnd);
    bool contiguous = IsRangeContiguous(propertyId, tokenIdStart, tokenIdEnd);
    if (startOwner != from || endOwner != from || !contiguous) return false;

    // are we moving the complete range from 'from'?
    // we know the range is contiguous (above) so we can use a single GetRange call
    bool bMovingCompleteRange = false;
    std::pair<int64_t,int64_t> senderTokenRange = GetRange(propertyId, tokenIdStart);
    if (senderTokenRange.first == tokenIdStart && senderTokenRange.second == tokenIdEnd) {
        bMovingCompleteRange = true;
    }

    // does 'to' have adjacent ranges that need to be merged?
    bool bToAdjacentRangeBefore = false;
    bool bToAdjacentRangeAfter = false;
    std::string rangeBelowOwner = GetUniqueTokenOwner(propertyId, tokenIdStart-1);
    std::string rangeAfterOwner = GetUniqueTokenOwner(propertyId, tokenIdEnd+1);
    if (rangeBelowOwner == to) {
        bToAdjacentRangeBefore = true;
    }
    if (rangeAfterOwner == to) {
        bToAdjacentRangeAfter = true;
    }

    // adjust 'from' ranges
    DeleteRange(propertyId, senderTokenRange.first, senderTokenRange.second);
    if (bMovingCompleteRange != true) {
        if (senderTokenRange.first < tokenIdStart) {
            AddRange(propertyId, senderTokenRange.first, tokenIdStart - 1, from);
        }
        if (senderTokenRange.second > tokenIdEnd) {
            AddRange(propertyId, tokenIdEnd + 1, senderTokenRange.second, from);
        }
    }

    // adjust 'to' ranges
    if (bToAdjacentRangeBefore == false && bToAdjacentRangeAfter == false) {
        AddRange(propertyId, tokenIdStart, tokenIdEnd, to);
    } else {
        int64_t newTokenIdStart = tokenIdStart;
        int64_t newTokenIdEnd = tokenIdEnd;
        if (bToAdjacentRangeBefore) {
            std::pair<int64_t,int64_t> oldRange = GetRange(propertyId, tokenIdStart-1);
            newTokenIdStart = oldRange.first;
            DeleteRange(propertyId, oldRange.first, oldRange.second);
        }
        if (bToAdjacentRangeAfter) {
            std::pair<int64_t,int64_t> oldRange = GetRange(propertyId, tokenIdEnd+1);
            newTokenIdEnd = oldRange.second;
            DeleteRange(propertyId, oldRange.first, oldRange.second);
        }
        AddRange(propertyId, newTokenIdStart, newTokenIdEnd, to);
    }

    return true;
}

/* Counts the highest token range end (which is thus the total number of tokens)
 */
int64_t CMPUniqueTokensDB::GetHighestRangeEnd(const uint32_t &propertyId)
{
    assert(pdb);

    int64_t tokenCount = 0;
    leveldb::Iterator* it = NewIterator();
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        if (propertyId != GetPropertyIdFromKey(it->key().ToString())) continue;

        int64_t start, end;
        GetRangeFromKey(it->key().ToString(), &start, &end);

        if (end > tokenCount) {
            tokenCount = end;
        }
    }
    delete it;
    return tokenCount;
}

/* Deletes a range of unique tokens
 */
void CMPUniqueTokensDB::DeleteRange(const uint32_t &propertyId, const int64_t &tokenIdStart, const int64_t &tokenIdEnd)
{
    assert(pdb);
    const std::string key = strprintf("%010d_%020d-%020d", propertyId, tokenIdStart, tokenIdEnd);
    pdb->Delete(leveldb::WriteOptions(), key);

    if (msc_debug_utdb) PrintToLog("%s():%s, line %d, file: %s\n", __FUNCTION__, key, __LINE__, __FILE__);
}

/* Adds a range of unique tokens
 */
void CMPUniqueTokensDB::AddRange(const uint32_t &propertyId, const int64_t &tokenIdStart, const int64_t &tokenIdEnd, const std::string &owner)
{
    assert(pdb);

    const std::string key = strprintf("%010d_%020d-%020d", propertyId, tokenIdStart, tokenIdEnd);
    leveldb::Status status = pdb->Put(writeoptions, key, owner);
    ++nWritten;

    if (msc_debug_utdb) PrintToLog("%s():%s=%s:%s, line %d, file: %s\n", __FUNCTION__, key, owner, status.ToString(), __LINE__, __FILE__);
}

/* Creates a range of unique tokens
 */
std::pair<int64_t,int64_t> CMPUniqueTokensDB::CreateUniqueTokens(const uint32_t &propertyId, const int64_t &amount, const std::string &owner)
{
    if (msc_debug_utdb) PrintToLog("%s(): %d:%d:%s, line %d, file: %s\n", __FUNCTION__, propertyId, amount, owner, __LINE__, __FILE__);

    int64_t highestId = GetHighestRangeEnd(propertyId);
    int64_t newTokenStartId = highestId + 1;
    int64_t newTokenEndId = 0;

    if ( ((amount > 0) && (highestId > (std::numeric_limits<int64_t>::max()-amount))) ||   /* overflow */
         ((amount < 0) && (highestId < (std::numeric_limits<int64_t>::min()-amount))) ) {  /* underflow */
        newTokenEndId = std::numeric_limits<int64_t>::max();
    } else {
        newTokenEndId = highestId + amount;
    }

    std::pair<int64_t,int64_t> newRange = std::make_pair(newTokenStartId, newTokenEndId);

    std::string highestRangeOwner = GetUniqueTokenOwner(propertyId, highestId);
    if (highestRangeOwner == owner) {
        std::pair<int64_t,int64_t> oldRange = GetRange(propertyId, highestId);
        DeleteRange(propertyId, oldRange.first, oldRange.second);
        newTokenStartId = oldRange.first; // override range start to merge ranges from same owner
    }

    AddRange(propertyId, newTokenStartId, newTokenEndId, owner);

    return newRange;
}

/* Gets the owner of a range of unique tokens
 *
 * Note: If the specified range is not owned in it's entirety by a single owner (or unassigned) an empty string is returned
 */
std::string CMPUniqueTokensDB::GetUniqueTokenOwner(const uint32_t &propertyId, const int64_t &tokenId)
{
    assert(pdb);
    leveldb::Iterator* it = NewIterator();

    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        if (propertyId != GetPropertyIdFromKey(it->key().ToString())) continue;

        int64_t start, end;
        GetRangeFromKey(it->key().ToString(), &start, &end);

        if (tokenId >= start && tokenId <= end) {
            std::string retval = it->value().ToString();
            delete it;
            return retval;
        }
    }

    delete it;
    return ""; // not found
}

/* Gets the ranges of unique tokens owned by an address
 */
std::vector<std::pair<int64_t,int64_t> > CMPUniqueTokensDB::GetAddressUniqueTokens(const uint32_t &propertyId, const std::string &address)
{
    std::vector<std::pair<int64_t,int64_t> > uniqueMap;
    assert(pdb);
    leveldb::Iterator* it = NewIterator();
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        std::string value = it->value().ToString();
        if (value != address) continue;

        if (propertyId != GetPropertyIdFromKey(it->key().ToString())) continue;

        int64_t start, end;
        GetRangeFromKey(it->key().ToString(), &start, &end);

        uniqueMap.push_back(std::make_pair(start, end));
    }
    delete it;
    return uniqueMap;
}

/* Gets the ranges of unique tokens for a property
 */
std::vector<std::pair<std::string,std::pair<int64_t,int64_t> > > CMPUniqueTokensDB::GetUniqueTokenRanges(const uint32_t &propertyId)
{
    std::vector<std::pair<std::string,std::pair<int64_t,int64_t> > > rangeMap;

    assert(pdb);

    leveldb::Iterator* it = NewIterator();
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        if (propertyId != GetPropertyIdFromKey(it->key().ToString())) continue;

        std::string address = it->value().ToString();
        int64_t start, end;
        GetRangeFromKey(it->key().ToString(), &start, &end);

        rangeMap.push_back(std::make_pair(address,std::make_pair(start, end)));
    }

    delete it;
    return rangeMap;
}

void CMPUniqueTokensDB::SanityCheck()
{
    assert(pdb);

    std::string result = "";

    std::map<uint32_t,int64_t> totals;

    leveldb::Iterator* it = NewIterator();
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        uint32_t propertyId = GetPropertyIdFromKey(it->key().ToString());
        int64_t start, end;
        GetRangeFromKey(it->key().ToString(), &start, &end);
        if (end > totals[propertyId]) {
            totals[propertyId] = end;
        }
    }
    delete it;

    for (std::map<uint32_t,int64_t>::iterator it = totals.begin(); it != totals.end(); ++it) {
        if (mastercore::getTotalTokens(it->first) != it->second) {
            std::string abortMsg = strprintf("Failed sanity check on property %d (%d != %d)\n", it->first, mastercore::getTotalTokens(it->first), it->second);
            AbortNode(abortMsg);
        } else {
            result = result + strprintf("%d:%d=%d,", it->first, mastercore::getTotalTokens(it->first), it->second);
        }
    }

    if (msc_debug_utdb) PrintToLog("UTDB sanity check OK (%s)\n", result);
}

void CMPUniqueTokensDB::printStats()
{
    PrintToLog("CMPTxList stats: nWritten= %d , nRead= %d\n", nWritten, nRead);
}

void CMPUniqueTokensDB::printAll()
{
    int count = 0;
    leveldb::Slice skey, svalue;
    leveldb::Iterator* it = NewIterator();

    for(it->SeekToFirst(); it->Valid(); it->Next()) {
        skey = it->key();
        svalue = it->value();
        ++count;
        PrintToConsole("entry #%8d= %s:%s\n", count, skey.ToString(), svalue.ToString());
//      PrintToLog("entry #%8d= %s:%s\n", count, skey.ToString(), svalue.ToString());
    }

    delete it;
}

