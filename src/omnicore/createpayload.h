#ifndef OMNICORE_CREATEPAYLOAD_H
#define OMNICORE_CREATEPAYLOAD_H

#include <string>
#include <vector>
#include <stdint.h>

std::vector<unsigned char> CreatePayload_SimpleSend(uint32_t propertyId, uint64_t amount, bool compress=false);
std::vector<unsigned char> CreatePayload_SendAll(uint8_t ecosystem, bool compress=false);
std::vector<unsigned char> CreatePayload_DExSell(uint32_t propertyId, uint64_t amountForSale, uint64_t amountDesired, uint8_t timeLimit, uint64_t minFee, uint8_t subAction, bool compress=false);
std::vector<unsigned char> CreatePayload_DExAccept(uint32_t propertyId, uint64_t amount, bool compress=false);
std::vector<unsigned char> CreatePayload_SendToOwners(uint32_t propertyId, uint64_t amount, uint32_t distributionProperty, bool compress=false);
std::vector<unsigned char> CreatePayload_IssuanceFixed(uint8_t ecosystem, uint16_t propertyType, uint32_t previousPropertyId, std::string category,
                                                       std::string subcategory, std::string name, std::string url, std::string data, uint64_t amount, bool compress=false);
std::vector<unsigned char> CreatePayload_IssuanceVariable(uint8_t ecosystem, uint16_t propertyType, uint32_t previousPropertyId, std::string category,
                                                          std::string subcategory, std::string name, std::string url, std::string data, uint32_t propertyIdDesired,
                                                          uint64_t amountPerUnit, uint64_t deadline, uint8_t earlyBonus, uint8_t issuerPercentage, bool compress=false);
std::vector<unsigned char> CreatePayload_IssuanceManaged(uint8_t ecosystem, uint16_t propertyType, uint32_t previousPropertyId, std::string category,
                                                       std::string subcategory, std::string name, std::string url, std::string data, bool compress=false);
std::vector<unsigned char> CreatePayload_CloseCrowdsale(uint32_t propertyId, bool compress=false);
std::vector<unsigned char> CreatePayload_Grant(uint32_t propertyId, uint64_t amount, std::string memo, bool compress=false);
std::vector<unsigned char> CreatePayload_Revoke(uint32_t propertyId, uint64_t amount, std::string memo, bool compress=false);
std::vector<unsigned char> CreatePayload_ChangeIssuer(uint32_t propertyId, bool compress=false);
std::vector<unsigned char> CreatePayload_MetaDExTrade(uint32_t propertyIdForSale, uint64_t amountForSale, uint32_t propertyIdDesired, uint64_t amountDesired, bool compress=false);
std::vector<unsigned char> CreatePayload_MetaDExCancelPrice(uint32_t propertyIdForSale, uint64_t amountForSale, uint32_t propertyIdDesired, uint64_t amountDesired, bool compress=false);
std::vector<unsigned char> CreatePayload_MetaDExCancelPair(uint32_t propertyIdForSale, uint32_t propertyIdDesired, bool compress=false);
std::vector<unsigned char> CreatePayload_MetaDExCancelEcosystem(uint8_t ecosystem, bool compress=false);
std::vector<unsigned char> CreatePayload_OmniCoreAlert(uint16_t alertType, uint32_t expiryValue, const std::string& alertMessage);
std::vector<unsigned char> CreatePayload_DeactivateFeature(uint16_t featureId);
std::vector<unsigned char> CreatePayload_ActivateFeature(uint16_t featureId, uint32_t activationBlock, uint32_t minClientVersion);

#endif // OMNICORE_CREATEPAYLOAD_H
