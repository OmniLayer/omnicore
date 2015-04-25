#ifndef OMNICORE_AUDITOR_H
#define OMNICORE_AUDITOR_H

#include "uint256.h"

#include "mastercore.h"
#include "mastercore_dex.h"

#include <stdint.h>
#include <string>

class CBlockIndex;

extern bool auditorEnabled;
extern bool auditBalanceChanges;

namespace mastercore
{
  void Auditor_Initialize();
  void Auditor_NotifyBlockStart(CBlockIndex const * pBlockIndex);
  void Auditor_NotifyBlockFinish(CBlockIndex const * pBlockIndex);
  void Auditor_NotifyPropertyTotalChanged(bool incrase, uint32_t propertyId, int64_t amount, std::string const& reasonStr);
  void Auditor_NotifyPropertyCreated(uint32_t propertyId);
  void Auditor_NotifyTradeCreated(uint256 txid, XDOUBLE effectivePrice);
  void Auditor_NotifyChainReorg(int nWaterlineBlock);
  void Auditor_NotifyBalanceChangeRequested(const std::string& address, int64_t amount, uint32_t propertyId, TallyType tallyType, const std::string& type, uint256 txid, const std::string& caller, bool processed);
}

uint32_t ComparePropertyTotals();
bool ComparePropertyCounts();
uint256 SearchForBadTrades(std::string &reasonText);
uint32_t SearchForUnmatchedTrades(std::string &reasonText);
int64_t SafeGetTotalTokens(uint32_t propertyId);
void AuditFail(const std::string& msg);
bool CompareBalances(std::string &compareFailures);

#endif // OMNICORE_AUDITOR_H



