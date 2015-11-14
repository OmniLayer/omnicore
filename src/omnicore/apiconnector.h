#ifndef OMNICORE_APICONNECTOR_H
#define OMNICORE_APICONNECTOR_H

#include "omnicore/mdex.h"

#include "json/json_spirit_reader_template.h"
#include "json/json_spirit_utils.h"
#include "json/json_spirit_writer_template.h"

class uint256;

namespace mastercore
{
extern bool bAPIActive;

bool APIInit();

bool APIPost(json_spirit::Object apiObj);

json_spirit::Object APICreateBalanceNotification(const std::string& address, int64_t amount, uint32_t propertyId);
json_spirit::Object APICreateTransactionNotification(const uint256& txid);
json_spirit::Object APICreatePropertyNotification(uint32_t propertyId);
json_spirit::Object APICreateDExSellNotification(const std::string& address, int64_t amount, uint32_t propertyId, int64_t desired, int64_t minFee, uint8_t timeLimit);
json_spirit::Object APICreateDExDestroyNotification(const std::string& addressSeller, int64_t amount, uint32_t propertyId);
json_spirit::Object APICreateDExAcceptNotification(const std::string& address, const uint256& offerTXID, int64_t amount, uint32_t propertyId, int64_t offerAmount, int64_t offerDesired, uint8_t timeLimit);
json_spirit::Object APICreateDExAcceptDestroyNotification(const std::string& addressSeller, const std::string& addressBuyer, uint32_t propertyId, int64_t amountReturned, bool fForceErase);
json_spirit::Object APICreateDExTradeNotification(const std::string& addressSeller, const std::string& addressBuyer, uint32_t propertyId, int64_t amountPurchased, int64_t amountPaid);
json_spirit::Object APICreateMetaDExOfferNotification(const CMPMetaDEx& mdexObj);
json_spirit::Object APICreateMetaDExDeletionNotification(const uint256& txid);
json_spirit::Object APICreateMetaDExTradeNotification(const CMPMetaDEx& newOffer, const CMPMetaDEx& matchedOffer, int64_t newReceived, int64_t matchedReceived);


}

#endif // OMNICORE_APICONNECTOR_H

