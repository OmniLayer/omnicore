// This file serves to provide payload creation functions.

#include "omnicore/createpayload.h"

#include "omnicore/convert.h"
#include "omnicore/varint.h"

#include "tinyformat.h"

#include <stdint.h>
#include <string>
#include <vector>

/**
 * Pushes bytes to the end of a vector.
 */
#define PUSH_BACK_BYTES(vector, value)\
    vector.insert(vector.end(), reinterpret_cast<unsigned char *>(&(value)),\
    reinterpret_cast<unsigned char *>(&(value)) + sizeof((value)));

/**
 * Pushes bytes to the end of a vector based on a pointer.
 */
#define PUSH_BACK_BYTES_PTR(vector, ptr, size)\
    vector.insert(vector.end(), reinterpret_cast<unsigned char *>((ptr)),\
    reinterpret_cast<unsigned char *>((ptr)) + (size));


std::vector<unsigned char> CreatePayload_SimpleSend(uint32_t propertyId, uint64_t amount, bool compress)
{
    std::vector<unsigned char> payload;

    uint16_t messageVer = 0;
    uint16_t messageType = 0;

    if (compress) {
        std::vector<uint8_t> vecMessageVer = CompressInteger((uint64_t)messageVer);
        std::vector<uint8_t> vecMessageType = CompressInteger((uint64_t)messageType);
        std::vector<uint8_t> vecPropertyId = CompressInteger((uint64_t)propertyId);
        std::vector<uint8_t> vecAmount = CompressInteger(amount);
        payload.insert(payload.end(), vecMessageVer.begin(), vecMessageVer.end());
        payload.insert(payload.end(), vecMessageType.begin(), vecMessageType.end());
        payload.insert(payload.end(), vecPropertyId.begin(), vecPropertyId.end());
        payload.insert(payload.end(), vecAmount.begin(), vecAmount.end());
    } else {
        mastercore::swapByteOrder16(messageVer);
        mastercore::swapByteOrder16(messageType);
        mastercore::swapByteOrder32(propertyId);
        mastercore::swapByteOrder64(amount);
        PUSH_BACK_BYTES(payload, messageVer);
        PUSH_BACK_BYTES(payload, messageType);
        PUSH_BACK_BYTES(payload, propertyId);
        PUSH_BACK_BYTES(payload, amount);
    }

    return payload;
}

std::vector<unsigned char> CreatePayload_SendToOwners(uint32_t propertyId, uint64_t amount, uint32_t distributionProperty, bool compress)
{
    bool v0 = (propertyId == distributionProperty) ? true : false;

    std::vector<unsigned char> payload;

    uint16_t messageVer = (v0) ? 0 : 1;
    uint16_t messageType = 3;

    if (compress) {
        std::vector<uint8_t> vecMessageVer = CompressInteger((uint64_t)messageVer);
        std::vector<uint8_t> vecMessageType = CompressInteger((uint64_t)messageType);
        std::vector<uint8_t> vecPropertyId = CompressInteger((uint64_t)propertyId);
        std::vector<uint8_t> vecAmount = CompressInteger(amount);
        payload.insert(payload.end(), vecMessageVer.begin(), vecMessageVer.end());
        payload.insert(payload.end(), vecMessageType.begin(), vecMessageType.end());
        payload.insert(payload.end(), vecPropertyId.begin(), vecPropertyId.end());
        payload.insert(payload.end(), vecAmount.begin(), vecAmount.end());
        if (!v0) {
            std::vector<uint8_t> vecDistributionProperty = CompressInteger(distributionProperty);
            payload.insert(payload.end(), vecDistributionProperty.begin(), vecDistributionProperty.end());
        }
    } else {
        mastercore::swapByteOrder16(messageType);
        mastercore::swapByteOrder16(messageVer);
        mastercore::swapByteOrder32(propertyId);
        mastercore::swapByteOrder64(amount);
        PUSH_BACK_BYTES(payload, messageVer);
        PUSH_BACK_BYTES(payload, messageType);
        PUSH_BACK_BYTES(payload, propertyId);
        PUSH_BACK_BYTES(payload, amount);
        if (!v0) {
            mastercore::swapByteOrder32(distributionProperty);
            PUSH_BACK_BYTES(payload, distributionProperty);
        }
    }

    return payload;
}

std::vector<unsigned char> CreatePayload_SendAll(uint8_t ecosystem, bool compress)
{
    std::vector<unsigned char> payload;

    uint16_t messageVer = 0;
    uint16_t messageType = 4;

    if (compress) {
        std::vector<uint8_t> vecMessageVer = CompressInteger((uint64_t)messageVer);
        std::vector<uint8_t> vecMessageType = CompressInteger((uint64_t)messageType);
        payload.insert(payload.end(), vecMessageVer.begin(), vecMessageVer.end());
        payload.insert(payload.end(), vecMessageType.begin(), vecMessageType.end());
        PUSH_BACK_BYTES(payload, ecosystem);
    } else {
        mastercore::swapByteOrder16(messageVer);
        mastercore::swapByteOrder16(messageType);
        PUSH_BACK_BYTES(payload, messageVer);
        PUSH_BACK_BYTES(payload, messageType);
        PUSH_BACK_BYTES(payload, ecosystem);
    }

    return payload;
}

std::vector<unsigned char> CreatePayload_DExSell(uint32_t propertyId, uint64_t amountForSale, uint64_t amountDesired, uint8_t timeLimit, uint64_t minFee, uint8_t subAction, bool compress)
{
    std::vector<unsigned char> payload;

    uint16_t messageVer = 1;
    uint16_t messageType = 20;

    if (compress) {
        std::vector<uint8_t> vecMessageVer = CompressInteger((uint64_t)messageVer);
        std::vector<uint8_t> vecMessageType = CompressInteger((uint64_t)messageType);
        std::vector<uint8_t> vecPropertyId = CompressInteger((uint64_t)propertyId);
        std::vector<uint8_t> vecAmountForSale = CompressInteger(amountForSale);
        std::vector<uint8_t> vecAmountDesired = CompressInteger(amountDesired);
        std::vector<uint8_t> vecMinFee = CompressInteger(minFee);
        payload.insert(payload.end(), vecMessageVer.begin(), vecMessageVer.end());
        payload.insert(payload.end(), vecMessageType.begin(), vecMessageType.end());
        payload.insert(payload.end(), vecPropertyId.begin(), vecPropertyId.end());
        payload.insert(payload.end(), vecAmountForSale.begin(), vecAmountForSale.end());
        payload.insert(payload.end(), vecAmountDesired.begin(), vecAmountDesired.end());
        PUSH_BACK_BYTES(payload, timeLimit);
        payload.insert(payload.end(), vecMinFee.begin(), vecMinFee.end());
        PUSH_BACK_BYTES(payload, subAction);
    } else {
        mastercore::swapByteOrder16(messageVer);
        mastercore::swapByteOrder16(messageType);
        mastercore::swapByteOrder32(propertyId);
        mastercore::swapByteOrder64(amountForSale);
        mastercore::swapByteOrder64(amountDesired);
        mastercore::swapByteOrder64(minFee);
        PUSH_BACK_BYTES(payload, messageVer);
        PUSH_BACK_BYTES(payload, messageType);
        PUSH_BACK_BYTES(payload, propertyId);
        PUSH_BACK_BYTES(payload, amountForSale);
        PUSH_BACK_BYTES(payload, amountDesired);
        PUSH_BACK_BYTES(payload, timeLimit);
        PUSH_BACK_BYTES(payload, minFee);
        PUSH_BACK_BYTES(payload, subAction);
    }

    return payload;
}

std::vector<unsigned char> CreatePayload_DExAccept(uint32_t propertyId, uint64_t amount, bool compress)
{
    std::vector<unsigned char> payload;

    uint16_t messageVer = 0;
    uint16_t messageType = 22;

    if (compress) {
        std::vector<uint8_t> vecMessageVer = CompressInteger((uint64_t)messageVer);
        std::vector<uint8_t> vecMessageType = CompressInteger((uint64_t)messageType);
        std::vector<uint8_t> vecPropertyId = CompressInteger((uint64_t)propertyId);
        std::vector<uint8_t> vecAmount = CompressInteger(amount);
        payload.insert(payload.end(), vecMessageVer.begin(), vecMessageVer.end());
        payload.insert(payload.end(), vecMessageType.begin(), vecMessageType.end());
        payload.insert(payload.end(), vecPropertyId.begin(), vecPropertyId.end());
        payload.insert(payload.end(), vecAmount.begin(), vecAmount.end());
    } else {
        mastercore::swapByteOrder16(messageVer);
        mastercore::swapByteOrder16(messageType);
        mastercore::swapByteOrder32(propertyId);
        mastercore::swapByteOrder64(amount);
        PUSH_BACK_BYTES(payload, messageVer);
        PUSH_BACK_BYTES(payload, messageType);
        PUSH_BACK_BYTES(payload, propertyId);
        PUSH_BACK_BYTES(payload, amount);
    }

    return payload;
}

std::vector<unsigned char> CreatePayload_IssuanceFixed(uint8_t ecosystem, uint16_t propertyType, uint32_t previousPropertyId, std::string category,
                                                       std::string subcategory, std::string name, std::string url, std::string data, uint64_t amount, bool compress)
{
    std::vector<unsigned char> payload;

    uint16_t messageVer = 0;
    uint16_t messageType = 50;

    if (compress) {
        std::vector<uint8_t> vecMessageVer = CompressInteger(messageVer);
        std::vector<uint8_t> vecMessageType = CompressInteger(messageType);
        std::vector<uint8_t> vecPropertyType = CompressInteger(propertyType);
        std::vector<uint8_t> vecPrevPropertyId = CompressInteger(previousPropertyId);
        payload.insert(payload.end(), vecMessageVer.begin(), vecMessageVer.end());
        payload.insert(payload.end(), vecMessageType.begin(), vecMessageType.end());
        PUSH_BACK_BYTES(payload, ecosystem);
        payload.insert(payload.end(), vecPropertyType.begin(), vecPropertyType.end());
        payload.insert(payload.end(), vecPrevPropertyId.begin(), vecPrevPropertyId.end());
    } else {
        mastercore::swapByteOrder16(messageVer);
        mastercore::swapByteOrder16(messageType);
        mastercore::swapByteOrder16(propertyType);
        mastercore::swapByteOrder32(previousPropertyId);
        PUSH_BACK_BYTES(payload, messageVer);
        PUSH_BACK_BYTES(payload, messageType);
        PUSH_BACK_BYTES(payload, ecosystem);
        PUSH_BACK_BYTES(payload, propertyType);
        PUSH_BACK_BYTES(payload, previousPropertyId);
    }

    if (category.size() > 255) category = category.substr(0,255);
    if (subcategory.size() > 255) subcategory = subcategory.substr(0,255);
    if (name.size() > 255) name = name.substr(0,255);
    if (url.size() > 255) url = url.substr(0,255);
    if (data.size() > 255) data = data.substr(0,255);
    payload.insert(payload.end(), category.begin(), category.end());
    payload.push_back('\0');
    payload.insert(payload.end(), subcategory.begin(), subcategory.end());
    payload.push_back('\0');
    payload.insert(payload.end(), name.begin(), name.end());
    payload.push_back('\0');
    payload.insert(payload.end(), url.begin(), url.end());
    payload.push_back('\0');
    payload.insert(payload.end(), data.begin(), data.end());
    payload.push_back('\0');

    if (compress) {
        std::vector<uint8_t> vecAmount = CompressInteger(amount);
        payload.insert(payload.end(), vecAmount.begin(), vecAmount.end());
    } else {
        mastercore::swapByteOrder64(amount);
        PUSH_BACK_BYTES(payload, amount);
    }

    return payload;
}

std::vector<unsigned char> CreatePayload_IssuanceVariable(uint8_t ecosystem, uint16_t propertyType, uint32_t previousPropertyId, std::string category,
                                                          std::string subcategory, std::string name, std::string url, std::string data, uint32_t propertyIdDesired,
                                                          uint64_t amountPerUnit, uint64_t deadline, uint8_t earlyBonus, uint8_t issuerPercentage, bool compress)
{
    std::vector<unsigned char> payload;

    uint16_t messageVer = 0;
    uint16_t messageType = 51;

    if (compress) {
        std::vector<uint8_t> vecMessageVer = CompressInteger(messageVer);
        std::vector<uint8_t> vecMessageType = CompressInteger(messageType);
        std::vector<uint8_t> vecPropertyType = CompressInteger(propertyType);
        std::vector<uint8_t> vecPrevPropertyId = CompressInteger(previousPropertyId);
        payload.insert(payload.end(), vecMessageVer.begin(), vecMessageVer.end());
        payload.insert(payload.end(), vecMessageType.begin(), vecMessageType.end());
        PUSH_BACK_BYTES(payload, ecosystem);
        payload.insert(payload.end(), vecPropertyType.begin(), vecPropertyType.end());
        payload.insert(payload.end(), vecPrevPropertyId.begin(), vecPrevPropertyId.end());
    } else {
        mastercore::swapByteOrder16(messageVer);
        mastercore::swapByteOrder16(messageType);
        mastercore::swapByteOrder16(propertyType);
        mastercore::swapByteOrder32(previousPropertyId);
        PUSH_BACK_BYTES(payload, messageVer);
        PUSH_BACK_BYTES(payload, messageType);
        PUSH_BACK_BYTES(payload, ecosystem);
        PUSH_BACK_BYTES(payload, propertyType);
        PUSH_BACK_BYTES(payload, previousPropertyId);
    }

    if (category.size() > 255) category = category.substr(0,255);
    if (subcategory.size() > 255) subcategory = subcategory.substr(0,255);
    if (name.size() > 255) name = name.substr(0,255);
    if (url.size() > 255) url = url.substr(0,255);
    if (data.size() > 255) data = data.substr(0,255);
    payload.insert(payload.end(), category.begin(), category.end());
    payload.push_back('\0');
    payload.insert(payload.end(), subcategory.begin(), subcategory.end());
    payload.push_back('\0');
    payload.insert(payload.end(), name.begin(), name.end());
    payload.push_back('\0');
    payload.insert(payload.end(), url.begin(), url.end());
    payload.push_back('\0');
    payload.insert(payload.end(), data.begin(), data.end());
    payload.push_back('\0');

    if (compress) {
        std::vector<uint8_t> vecPropertyIdDesired = CompressInteger(propertyIdDesired);
        std::vector<uint8_t> vecAmountPerUnit = CompressInteger(amountPerUnit);
        std::vector<uint8_t> vecDeadline = CompressInteger(deadline);
        payload.insert(payload.end(), vecPropertyIdDesired.begin(), vecPropertyIdDesired.end());
        payload.insert(payload.end(), vecAmountPerUnit.begin(), vecAmountPerUnit.end());
        payload.insert(payload.end(), vecDeadline.begin(), vecDeadline.end());
    } else {
        mastercore::swapByteOrder32(propertyIdDesired);
        mastercore::swapByteOrder64(amountPerUnit);
        mastercore::swapByteOrder64(deadline);
        PUSH_BACK_BYTES(payload, propertyIdDesired);
        PUSH_BACK_BYTES(payload, amountPerUnit);
        PUSH_BACK_BYTES(payload, deadline);
    }

    PUSH_BACK_BYTES(payload, earlyBonus);
    PUSH_BACK_BYTES(payload, issuerPercentage);

    return payload;
}

std::vector<unsigned char> CreatePayload_CloseCrowdsale(uint32_t propertyId, bool compress)
{
    std::vector<unsigned char> payload;

    uint16_t messageVer = 0;
    uint16_t messageType = 53;

    if (compress) {
        std::vector<uint8_t> vecMessageVer = CompressInteger((uint64_t)messageVer);
        std::vector<uint8_t> vecMessageType = CompressInteger((uint64_t)messageType);
        std::vector<uint8_t> vecPropertyId = CompressInteger((uint64_t)propertyId);
        payload.insert(payload.end(), vecMessageVer.begin(), vecMessageVer.end());
        payload.insert(payload.end(), vecMessageType.begin(), vecMessageType.end());
        payload.insert(payload.end(), vecPropertyId.begin(), vecPropertyId.end());
    } else {
        mastercore::swapByteOrder16(messageVer);
        mastercore::swapByteOrder16(messageType);
        mastercore::swapByteOrder32(propertyId);
        PUSH_BACK_BYTES(payload, messageVer);
        PUSH_BACK_BYTES(payload, messageType);
        PUSH_BACK_BYTES(payload, propertyId);
    }

    return payload;
}

std::vector<unsigned char> CreatePayload_IssuanceManaged(uint8_t ecosystem, uint16_t propertyType, uint32_t previousPropertyId, std::string category,
                                                       std::string subcategory, std::string name, std::string url, std::string data, bool compress)
{
    std::vector<unsigned char> payload;

    uint16_t messageVer = 0;
    uint16_t messageType = 54;

    if (compress) {
        std::vector<uint8_t> vecMessageVer = CompressInteger(messageVer);
        std::vector<uint8_t> vecMessageType = CompressInteger(messageType);
        std::vector<uint8_t> vecPropertyType = CompressInteger(propertyType);
        std::vector<uint8_t> vecPrevPropertyId = CompressInteger(previousPropertyId);
        payload.insert(payload.end(), vecMessageVer.begin(), vecMessageVer.end());
        payload.insert(payload.end(), vecMessageType.begin(), vecMessageType.end());
        PUSH_BACK_BYTES(payload, ecosystem);
        payload.insert(payload.end(), vecPropertyType.begin(), vecPropertyType.end());
        payload.insert(payload.end(), vecPrevPropertyId.begin(), vecPrevPropertyId.end());
    } else {
        mastercore::swapByteOrder16(messageVer);
        mastercore::swapByteOrder16(messageType);
        mastercore::swapByteOrder16(propertyType);
        mastercore::swapByteOrder32(previousPropertyId);
        PUSH_BACK_BYTES(payload, messageVer);
        PUSH_BACK_BYTES(payload, messageType);
        PUSH_BACK_BYTES(payload, ecosystem);
        PUSH_BACK_BYTES(payload, propertyType);
        PUSH_BACK_BYTES(payload, previousPropertyId);
    }

    if (category.size() > 255) category = category.substr(0,255);
    if (subcategory.size() > 255) subcategory = subcategory.substr(0,255);
    if (name.size() > 255) name = name.substr(0,255);
    if (url.size() > 255) url = url.substr(0,255);
    if (data.size() > 255) data = data.substr(0,255);
    payload.insert(payload.end(), category.begin(), category.end());
    payload.push_back('\0');
    payload.insert(payload.end(), subcategory.begin(), subcategory.end());
    payload.push_back('\0');
    payload.insert(payload.end(), name.begin(), name.end());
    payload.push_back('\0');
    payload.insert(payload.end(), url.begin(), url.end());
    payload.push_back('\0');
    payload.insert(payload.end(), data.begin(), data.end());
    payload.push_back('\0');

    return payload;
}

std::vector<unsigned char> CreatePayload_Grant(uint32_t propertyId, uint64_t amount, std::string memo, bool compress)
{
    std::vector<unsigned char> payload;

    uint16_t messageVer = 0;
    uint16_t messageType = 55;

    if (compress) {
        std::vector<uint8_t> vecMessageVer = CompressInteger((uint64_t)messageVer);
        std::vector<uint8_t> vecMessageType = CompressInteger((uint64_t)messageType);
        std::vector<uint8_t> vecPropertyId = CompressInteger((uint64_t)propertyId);
        std::vector<uint8_t> vecAmount = CompressInteger(amount);
        payload.insert(payload.end(), vecMessageVer.begin(), vecMessageVer.end());
        payload.insert(payload.end(), vecMessageType.begin(), vecMessageType.end());
        payload.insert(payload.end(), vecPropertyId.begin(), vecPropertyId.end());
        payload.insert(payload.end(), vecAmount.begin(), vecAmount.end());
    } else {
        mastercore::swapByteOrder16(messageVer);
        mastercore::swapByteOrder16(messageType);
        mastercore::swapByteOrder32(propertyId);
        mastercore::swapByteOrder64(amount);
        PUSH_BACK_BYTES(payload, messageVer);
        PUSH_BACK_BYTES(payload, messageType);
        PUSH_BACK_BYTES(payload, propertyId);
        PUSH_BACK_BYTES(payload, amount);
    }

    if (memo.size() > 255) memo = memo.substr(0,255);
    payload.insert(payload.end(), memo.begin(), memo.end());
    payload.push_back('\0');

    return payload;
}


std::vector<unsigned char> CreatePayload_Revoke(uint32_t propertyId, uint64_t amount, std::string memo, bool compress)
{
    std::vector<unsigned char> payload;

    uint16_t messageVer = 0;
    uint16_t messageType = 56;

    if (compress) {
        std::vector<uint8_t> vecMessageVer = CompressInteger((uint64_t)messageVer);
        std::vector<uint8_t> vecMessageType = CompressInteger((uint64_t)messageType);
        std::vector<uint8_t> vecPropertyId = CompressInteger((uint64_t)propertyId);
        std::vector<uint8_t> vecAmount = CompressInteger(amount);
        payload.insert(payload.end(), vecMessageVer.begin(), vecMessageVer.end());
        payload.insert(payload.end(), vecMessageType.begin(), vecMessageType.end());
        payload.insert(payload.end(), vecPropertyId.begin(), vecPropertyId.end());
        payload.insert(payload.end(), vecAmount.begin(), vecAmount.end());
    } else {
        mastercore::swapByteOrder16(messageVer);
        mastercore::swapByteOrder16(messageType);
        mastercore::swapByteOrder32(propertyId);
        mastercore::swapByteOrder64(amount);
        PUSH_BACK_BYTES(payload, messageVer);
        PUSH_BACK_BYTES(payload, messageType);
        PUSH_BACK_BYTES(payload, propertyId);
        PUSH_BACK_BYTES(payload, amount);
    }

    if (memo.size() > 255) memo = memo.substr(0,255);
    payload.insert(payload.end(), memo.begin(), memo.end());
    payload.push_back('\0');

    return payload;
}

std::vector<unsigned char> CreatePayload_ChangeIssuer(uint32_t propertyId, bool compress)
{
    std::vector<unsigned char> payload;

    uint16_t messageVer = 0;
    uint16_t messageType = 70;

    if (compress) {
        std::vector<uint8_t> vecMessageVer = CompressInteger((uint64_t)messageVer);
        std::vector<uint8_t> vecMessageType = CompressInteger((uint64_t)messageType);
        std::vector<uint8_t> vecPropertyId = CompressInteger((uint64_t)propertyId);
        payload.insert(payload.end(), vecMessageVer.begin(), vecMessageVer.end());
        payload.insert(payload.end(), vecMessageType.begin(), vecMessageType.end());
        payload.insert(payload.end(), vecPropertyId.begin(), vecPropertyId.end());
    } else {
        mastercore::swapByteOrder16(messageVer);
        mastercore::swapByteOrder16(messageType);
        mastercore::swapByteOrder32(propertyId);
        PUSH_BACK_BYTES(payload, messageVer);
        PUSH_BACK_BYTES(payload, messageType);
        PUSH_BACK_BYTES(payload, propertyId);
    }

    return payload;
}

std::vector<unsigned char> CreatePayload_MetaDExTrade(uint32_t propertyIdForSale, uint64_t amountForSale, uint32_t propertyIdDesired, uint64_t amountDesired, bool compress)
{
    std::vector<unsigned char> payload;

    uint16_t messageVer = 0;
    uint16_t messageType = 25;

    if (compress) {
        std::vector<uint8_t> vecMessageVer = CompressInteger((uint64_t)messageVer);
        std::vector<uint8_t> vecMessageType = CompressInteger((uint64_t)messageType);
        std::vector<uint8_t> vecPropertyIdForSale = CompressInteger((uint64_t)propertyIdForSale);
        std::vector<uint8_t> vecAmountForSale = CompressInteger(amountForSale);
        std::vector<uint8_t> vecPropertyIdDesired = CompressInteger((uint64_t)propertyIdDesired);
        std::vector<uint8_t> vecAmountDesired = CompressInteger(amountDesired);
        payload.insert(payload.end(), vecMessageVer.begin(), vecMessageVer.end());
        payload.insert(payload.end(), vecMessageType.begin(), vecMessageType.end());
        payload.insert(payload.end(), vecPropertyIdForSale.begin(), vecPropertyIdForSale.end());
        payload.insert(payload.end(), vecAmountForSale.begin(), vecAmountForSale.end());
        payload.insert(payload.end(), vecPropertyIdDesired.begin(), vecPropertyIdDesired.end());
        payload.insert(payload.end(), vecAmountDesired.begin(), vecAmountDesired.end());
    } else {
        mastercore::swapByteOrder16(messageVer);
        mastercore::swapByteOrder16(messageType);
        mastercore::swapByteOrder32(propertyIdForSale);
        mastercore::swapByteOrder64(amountForSale);
        mastercore::swapByteOrder32(propertyIdDesired);
        mastercore::swapByteOrder64(amountDesired);
        PUSH_BACK_BYTES(payload, messageVer);
        PUSH_BACK_BYTES(payload, messageType);
        PUSH_BACK_BYTES(payload, propertyIdForSale);
        PUSH_BACK_BYTES(payload, amountForSale);
        PUSH_BACK_BYTES(payload, propertyIdDesired);
        PUSH_BACK_BYTES(payload, amountDesired);
    }

    return payload;
}

std::vector<unsigned char> CreatePayload_MetaDExCancelPrice(uint32_t propertyIdForSale, uint64_t amountForSale, uint32_t propertyIdDesired, uint64_t amountDesired, bool compress)
{
    std::vector<unsigned char> payload;

    uint16_t messageVer = 0;
    uint16_t messageType = 26;

    if (compress) {
        std::vector<uint8_t> vecMessageVer = CompressInteger((uint64_t)messageVer);
        std::vector<uint8_t> vecMessageType = CompressInteger((uint64_t)messageType);
        std::vector<uint8_t> vecPropertyIdForSale = CompressInteger((uint64_t)propertyIdForSale);
        std::vector<uint8_t> vecAmountForSale = CompressInteger(amountForSale);
        std::vector<uint8_t> vecPropertyIdDesired = CompressInteger((uint64_t)propertyIdDesired);
        std::vector<uint8_t> vecAmountDesired = CompressInteger(amountDesired);
        payload.insert(payload.end(), vecMessageVer.begin(), vecMessageVer.end());
        payload.insert(payload.end(), vecMessageType.begin(), vecMessageType.end());
        payload.insert(payload.end(), vecPropertyIdForSale.begin(), vecPropertyIdForSale.end());
        payload.insert(payload.end(), vecAmountForSale.begin(), vecAmountForSale.end());
        payload.insert(payload.end(), vecPropertyIdDesired.begin(), vecPropertyIdDesired.end());
        payload.insert(payload.end(), vecAmountDesired.begin(), vecAmountDesired.end());
    } else {
        mastercore::swapByteOrder16(messageVer);
        mastercore::swapByteOrder16(messageType);
        mastercore::swapByteOrder32(propertyIdForSale);
        mastercore::swapByteOrder64(amountForSale);
        mastercore::swapByteOrder32(propertyIdDesired);
        mastercore::swapByteOrder64(amountDesired);
        PUSH_BACK_BYTES(payload, messageVer);
        PUSH_BACK_BYTES(payload, messageType);
        PUSH_BACK_BYTES(payload, propertyIdForSale);
        PUSH_BACK_BYTES(payload, amountForSale);
        PUSH_BACK_BYTES(payload, propertyIdDesired);
        PUSH_BACK_BYTES(payload, amountDesired);
    }

    return payload;
}

std::vector<unsigned char> CreatePayload_MetaDExCancelPair(uint32_t propertyIdForSale, uint32_t propertyIdDesired, bool compress)
{
    std::vector<unsigned char> payload;

    uint16_t messageVer = 0;
    uint16_t messageType = 27;

    if (compress) {
        std::vector<uint8_t> vecMessageVer = CompressInteger((uint64_t)messageVer);
        std::vector<uint8_t> vecMessageType = CompressInteger((uint64_t)messageType);
        std::vector<uint8_t> vecPropertyIdForSale = CompressInteger((uint64_t)propertyIdForSale);
        std::vector<uint8_t> vecPropertyIdDesired = CompressInteger((uint64_t)propertyIdDesired);
        payload.insert(payload.end(), vecMessageVer.begin(), vecMessageVer.end());
        payload.insert(payload.end(), vecMessageType.begin(), vecMessageType.end());
        payload.insert(payload.end(), vecPropertyIdForSale.begin(), vecPropertyIdForSale.end());
        payload.insert(payload.end(), vecPropertyIdDesired.begin(), vecPropertyIdDesired.end());
    } else {
        mastercore::swapByteOrder16(messageVer);
        mastercore::swapByteOrder16(messageType);
        mastercore::swapByteOrder32(propertyIdForSale);
        mastercore::swapByteOrder32(propertyIdDesired);
        PUSH_BACK_BYTES(payload, messageVer);
        PUSH_BACK_BYTES(payload, messageType);
        PUSH_BACK_BYTES(payload, propertyIdForSale);
        PUSH_BACK_BYTES(payload, propertyIdDesired);
    }

    return payload;
}

std::vector<unsigned char> CreatePayload_MetaDExCancelEcosystem(uint8_t ecosystem, bool compress)
{
    std::vector<unsigned char> payload;

    uint16_t messageVer = 0;
    uint16_t messageType = 28;

    if (compress) {
        std::vector<uint8_t> vecMessageVer = CompressInteger((uint64_t)messageVer);
        std::vector<uint8_t> vecMessageType = CompressInteger((uint64_t)messageType);
        payload.insert(payload.end(), vecMessageVer.begin(), vecMessageVer.end());
        payload.insert(payload.end(), vecMessageType.begin(), vecMessageType.end());
        PUSH_BACK_BYTES(payload, ecosystem);
    } else {
        mastercore::swapByteOrder16(messageVer);
        mastercore::swapByteOrder16(messageType);
        PUSH_BACK_BYTES(payload, messageVer);
        PUSH_BACK_BYTES(payload, messageType);
        PUSH_BACK_BYTES(payload, ecosystem);
    }

    return payload;
}

/**
 *  Omni Layer Management Functions
 *
 *  These functions support the feature activation and alert system and require authorization to use.
 */

std::vector<unsigned char> CreatePayload_DeactivateFeature(uint16_t featureId)
{
    std::vector<unsigned char> payload;

    uint16_t messageVer = 65535;
    uint16_t messageType = 65533;

    mastercore::swapByteOrder16(messageVer);
    mastercore::swapByteOrder16(messageType);
    mastercore::swapByteOrder16(featureId);

    PUSH_BACK_BYTES(payload, messageVer);
    PUSH_BACK_BYTES(payload, messageType);
    PUSH_BACK_BYTES(payload, featureId);

    return payload;
}

std::vector<unsigned char> CreatePayload_ActivateFeature(uint16_t featureId, uint32_t activationBlock, uint32_t minClientVersion)
{
    std::vector<unsigned char> payload;

    uint16_t messageVer = 65535;
    uint16_t messageType = 65534;

    mastercore::swapByteOrder16(messageVer);
    mastercore::swapByteOrder16(messageType);
    mastercore::swapByteOrder16(featureId);
    mastercore::swapByteOrder32(activationBlock);
    mastercore::swapByteOrder32(minClientVersion);

    PUSH_BACK_BYTES(payload, messageVer);
    PUSH_BACK_BYTES(payload, messageType);
    PUSH_BACK_BYTES(payload, featureId);
    PUSH_BACK_BYTES(payload, activationBlock);
    PUSH_BACK_BYTES(payload, minClientVersion);

    return payload;
}

std::vector<unsigned char> CreatePayload_OmniCoreAlert(uint16_t alertType, uint32_t expiryValue, const std::string& alertMessage)
{
    std::vector<unsigned char> payload;
    uint16_t messageType = 65535;
    uint16_t messageVer = 65535;

    mastercore::swapByteOrder16(messageVer);
    mastercore::swapByteOrder16(messageType);
    mastercore::swapByteOrder16(alertType);
    mastercore::swapByteOrder32(expiryValue);

    PUSH_BACK_BYTES(payload, messageVer);
    PUSH_BACK_BYTES(payload, messageType);
    PUSH_BACK_BYTES(payload, alertType);
    PUSH_BACK_BYTES(payload, expiryValue);
    payload.insert(payload.end(), alertMessage.begin(), alertMessage.end());
    payload.push_back('\0');

    return payload;
}

#undef PUSH_BACK_BYTES
#undef PUSH_BACK_BYTES_PTR
