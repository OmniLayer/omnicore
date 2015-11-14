/**
 * @file apiconnector.cpp
 *
 * The API connector provides functions to upload data to an external web based API.
 *
 */
#include "main.h"
#include "rpcprotocol.h"

#include "omnicore/log.h"
#include "omnicore/mdex.h"
#include "omnicore/omnicore.h"
#include "omnicore/rpc.h"
#include "omnicore/rpctxobject.h"
#include "omnicore/sp.h"

#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/iostreams/concepts.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/filesystem/operations.hpp>

#include <sstream>

namespace mastercore
{

std::string strAPIHost;
std::string strAPIUrl;
std::string strAPIPort;
bool bAPIActive = false;

/**
 * This function prepares the parameters for connecting with an API host
 */
bool APIInit()
{
    std::string apiparam = GetArg("-omniapiurl", "");
    if (apiparam.empty()) { // not changing bAPIActive, API will remain disabled
        PrintToLog("API URL not specified, API will be disabled (default)\n");
        return false;
    }

    strAPIPort = "443";
    size_t hostStartPos = 8;
    size_t hostEndPos = apiparam.find("/",hostStartPos);
    size_t httpsPos = apiparam.find("https:");
    size_t HTTPSPos = apiparam.find("HTTPS:");
    if (httpsPos == std::string::npos && HTTPSPos == std::string::npos) {
        strAPIPort = "80"; // no https found in url, disable https
        hostStartPos = 7;
    }
    if (hostEndPos <= hostStartPos || hostEndPos == std::string::npos) {
        PrintToLog("Failed to parse -omniapiurl parameter, API disabled\n");
        return false;
    }

    strAPIHost = apiparam.substr(hostStartPos, hostEndPos-hostStartPos);
    strAPIUrl = apiparam.substr(hostEndPos);
    PrintToLog("Activating API at host: %s, url: %s, port: %s\n", strAPIHost, strAPIUrl, strAPIPort);
    bAPIActive = true;
    return true;
}

/**
 * This function makes a POST request with the data and URL provided in the parameters
 */
bool PushPost(const std::string& data)
{
    if (!bAPIActive) {
        PrintToLog("APIPost failed.  API is not enabled\n");
        return false;
    }

    bool useSSL = (strAPIPort == "443") ? true : false;
    boost::asio::io_service io_service;
    boost::asio::ssl::context context(io_service, boost::asio::ssl::context::sslv23);
    context.set_options(boost::asio::ssl::context::no_sslv2 | boost::asio::ssl::context::no_sslv3);
    boost::asio::ssl::stream<boost::asio::ip::tcp::socket> sslStream(io_service, context);
    SSLIOStreamDevice<boost::asio::ip::tcp> d(sslStream, useSSL);
    boost::iostreams::stream< SSLIOStreamDevice<boost::asio::ip::tcp> > stream(d);

    bool connected = false;
    try {
        connected = d.connect(strAPIHost, strAPIPort);
    } catch(boost::exception const& ex) { }

    if (!connected) {
        PrintToLog("APIPost failed.  Failed to connect to API host %s on port %s\n", strAPIHost, strAPIPort);
        return false;
    }

    std::ostringstream post;
    post << "POST " << strAPIUrl << " HTTP/1.1\r\n"
         << "User-Agent: omni-core-api/" << "\r\n"
         << "Host: 127.0.0.1\r\n"
         << "Accept: */*\r\n"
         << "Content-Type: application/json\r\n"
         << "Content-Length: " << data.size() << "\r\n"
         << "Connection: close\r\n";

    post << "\r\n" << data;

    stream << post.str() << "\r\n" << std::flush;

    int nProto = 0;
    int nStatus = ReadHTTPStatus(stream, nProto);
    std::map<std::string, std::string> mapHeaders;
    std::string strReply;
    ReadHTTPMessage(stream, mapHeaders, strReply, nProto, std::numeric_limits<size_t>::max());
    if (nStatus == HTTP_UNAUTHORIZED) {
        PrintToLog("APIPost failed.  Failed to connect to API host %s on port %s (error: unauthorized)\n", strAPIHost, strAPIPort);
        return false;
    } else if (nStatus >= 400) {
        PrintToLog("APIPost failed.  API host returned HTTP error %d\n", nStatus);
        return false;
    } else if (strReply.empty()) {
        PrintToLog("APIPost failed.  API host returned an empty response\n");
        return false;
    }

    size_t successPos = strReply.find("APISUCCESS");
    if (successPos != std::string::npos) {
        return true;
    }

    return false;
}

/**
 * This is the calling function for an API post.  This handles retries and aborting if chosen
 */
bool APIPost(json_spirit::Object apiObj)
{
    usn++;

    apiObj.push_back(json_spirit::Pair("updatesequencenumber", usn));
    if (currentTrigger == UPDATE_TRIGGER_UNKNOWN) apiObj.push_back(json_spirit::Pair("updatetrigger", "unknown"));
    if (currentTrigger == UPDATE_TRIGGER_BLOCK) apiObj.push_back(json_spirit::Pair("updatetrigger", "block"));
    if (currentTrigger == UPDATE_TRIGGER_TRANSACTION) apiObj.push_back(json_spirit::Pair("updatetrigger", "transaction"));
    apiObj.push_back(json_spirit::Pair("updatetriggerobject", currentTriggerObject));
    std::string data = json_spirit::write_string(json_spirit::Value(apiObj), false);

    _updateDB->recordUpdate(usn, data);

    if (bAPIActive) {
        int retries = 10; // TODO: set via startup param
        bool abortOnAPIFailure = true; // TODO: set via startup param

        for (int retry = 1; retry <= retries; retry++) {
            if (PushPost(data)) {
                PrintToLog("APIPost succeeded (%s)\n", data);
                return true;
            } else {
                PrintToLog("WARNING: APIPost is being retried (retry %d, %s)\n", retry, data);
            }
        }

        PrintToLog("ERROR: APIPost failed after %d retries. (%s)\n", retries, data);

        if (abortOnAPIFailure) {
            std::string msgText = "APIPost failed, shutting down.\n";
            AbortNode(msgText, msgText);
        }
    }

    return false;
}

json_spirit::Object APICreateBalanceNotification(const std::string& address, int64_t amount, uint32_t propertyId)
{
    json_spirit::Object objNotification;
    json_spirit::Object objBalance;

    int64_t nAvailable = getUserAvailableMPbalance(address, propertyId);
    int64_t nReserved = 0;

    nReserved += getMPbalance(address, propertyId, ACCEPT_RESERVE);
    nReserved += getMPbalance(address, propertyId, METADEX_RESERVE);
    nReserved += getMPbalance(address, propertyId, SELLOFFER_RESERVE);

    objBalance.push_back(json_spirit::Pair("address", address));
    objBalance.push_back(json_spirit::Pair("propertyid", FormatIndivisibleMP(propertyId)));
    objBalance.push_back(json_spirit::Pair("amount", FormatAmountMP(propertyId, amount, true)));
    objBalance.push_back(json_spirit::Pair("balance", FormatMP(propertyId, nAvailable)));
    objBalance.push_back(json_spirit::Pair("reserved", FormatMP(propertyId, nReserved)));

    objNotification.push_back(json_spirit::Pair("updatetype", "balance"));
    objNotification.push_back(json_spirit::Pair("updatedelta", objBalance));

    return objNotification;
}

json_spirit::Object APICreateTransactionNotification(const uint256& txid)
{
    json_spirit::Object objNotification;
    json_spirit::Object objTransaction;

    int rc = populateRPCTransactionObject(txid, objTransaction, "", true);
    if (rc >= 0) {
        objNotification.push_back(json_spirit::Pair("updatetype", "transaction"));
        objNotification.push_back(json_spirit::Pair("updatedelta", objTransaction));
    } else {
        objNotification.push_back(json_spirit::Pair("error", strprintf("error populating transaction %s\n", txid.GetHex())));
    }

    return objNotification;
}

json_spirit::Object APICreatePropertyNotification(uint32_t propertyId)
{
    json_spirit::Object objNotification;
    json_spirit::Object objProperty;

    CMPSPInfo::Entry sp;
    {
        LOCK(cs_tally);
        if (!_my_sps->getSP(propertyId, sp)) {
            objNotification.push_back(json_spirit::Pair("error", strprintf("error loading property %d\n", propertyId)));
            return objNotification;
        }
    }

    std::string strCreationHash = sp.txid.GetHex();
    objProperty.push_back(json_spirit::Pair("propertyid", (uint64_t) propertyId));
    objProperty.push_back(json_spirit::Pair("name", sp.name));
    objProperty.push_back(json_spirit::Pair("category", sp.category));
    objProperty.push_back(json_spirit::Pair("subcategory", sp.subcategory));
    objProperty.push_back(json_spirit::Pair("data", sp.data));
    objProperty.push_back(json_spirit::Pair("url", sp.url));
    objProperty.push_back(json_spirit::Pair("divisible", sp.isDivisible()));
    objProperty.push_back(json_spirit::Pair("issuer", sp.issuer));
    objProperty.push_back(json_spirit::Pair("creationtxid", strCreationHash));

    objNotification.push_back(json_spirit::Pair("updatetype", "propertycreate"));
    objNotification.push_back(json_spirit::Pair("updatedelta", objProperty));

    return objNotification;
}

json_spirit::Object APICreateDExSellNotification(const std::string& address, int64_t amount, uint32_t propertyId, int64_t desired, int64_t minFee, uint8_t timeLimit)
{
    json_spirit::Object objNotification;
    json_spirit::Object objDExOffer;

    objDExOffer.push_back(json_spirit::Pair("address", address));
    objDExOffer.push_back(json_spirit::Pair("amount", FormatMP(propertyId, amount)));
    objDExOffer.push_back(json_spirit::Pair("propertyid", (uint64_t)propertyId));
    objDExOffer.push_back(json_spirit::Pair("amountdesired", FormatMP(propertyId, desired)));
    objDExOffer.push_back(json_spirit::Pair("minimumfee", FormatDivisibleMP(minFee)));
    objDExOffer.push_back(json_spirit::Pair("timelimit", (uint64_t)timeLimit));

    objNotification.push_back(json_spirit::Pair("updatetype", "dexcreate"));
    objNotification.push_back(json_spirit::Pair("updatedelta", objDExOffer));

    return objNotification;
}

json_spirit::Object APICreateDExDestroyNotification(const std::string& addressSeller, int64_t amount, uint32_t propertyId)
{
    json_spirit::Object objNotification;
    json_spirit::Object objDExDeletion;

    objDExDeletion.push_back(json_spirit::Pair("address", addressSeller));
    objDExDeletion.push_back(json_spirit::Pair("amountrefunded", FormatMP(propertyId, amount)));
    objDExDeletion.push_back(json_spirit::Pair("propertyid", (uint64_t)propertyId));

    objNotification.push_back(json_spirit::Pair("updatetype", "dexdelete"));
    objNotification.push_back(json_spirit::Pair("updatedelta", objDExDeletion));

    return objNotification;
}

json_spirit::Object APICreateDExAcceptNotification(const std::string& address, const uint256& offerTXID, int64_t amount, uint32_t propertyId, int64_t offerAmount, int64_t offerDesired, uint8_t timeLimit)
{
    json_spirit::Object objNotification;
    json_spirit::Object objDExAccept;

    objDExAccept.push_back(json_spirit::Pair("address", address));
    objDExAccept.push_back(json_spirit::Pair("amount", FormatMP(propertyId, amount)));
    objDExAccept.push_back(json_spirit::Pair("propertyid", (uint64_t)propertyId));
    objDExAccept.push_back(json_spirit::Pair("matchedsell", offerTXID.GetHex()));
    objDExAccept.push_back(json_spirit::Pair("matchedamountforsale", FormatMP(propertyId, offerAmount)));
    objDExAccept.push_back(json_spirit::Pair("matchedamountdesired", FormatMP(propertyId, offerDesired)));
    objDExAccept.push_back(json_spirit::Pair("matchedtimelimit", (uint64_t)timeLimit));

    objNotification.push_back(json_spirit::Pair("updatetype", "acceptcreate"));
    objNotification.push_back(json_spirit::Pair("updatedelta", objDExAccept));

    return objNotification;
}

json_spirit::Object APICreateDExAcceptDestroyNotification(const std::string& addressSeller, const std::string& addressBuyer, uint32_t propertyId, int64_t amountReturned, bool fForceErase)
{
    json_spirit::Object objNotification;
    json_spirit::Object objDExAcceptDestroy;

    objDExAcceptDestroy.push_back(json_spirit::Pair("address", addressBuyer));
    objDExAcceptDestroy.push_back(json_spirit::Pair("matchedaddress", addressSeller));
    objDExAcceptDestroy.push_back(json_spirit::Pair("propertyid", (uint64_t)propertyId));
    objDExAcceptDestroy.push_back(json_spirit::Pair("amountrefunded", FormatMP(propertyId, amountReturned)));
    objDExAcceptDestroy.push_back(json_spirit::Pair("forced", fForceErase));

    objNotification.push_back(json_spirit::Pair("updatetype", "acceptdelete"));
    objNotification.push_back(json_spirit::Pair("updatedelta", objDExAcceptDestroy));

    return objNotification;
}

json_spirit::Object APICreateDExTradeNotification(const std::string& addressSeller, const std::string& addressBuyer, uint32_t propertyId, int64_t amountPurchased, int64_t amountPaid)
{
    json_spirit::Object objNotification;
    json_spirit::Object objDExTrade;

    objDExTrade.push_back(json_spirit::Pair("buyeraddress", addressBuyer));
    objDExTrade.push_back(json_spirit::Pair("selleraddress", addressSeller));
    objDExTrade.push_back(json_spirit::Pair("propertyid", (uint64_t)propertyId));
    objDExTrade.push_back(json_spirit::Pair("amountpurchased", FormatMP(propertyId, amountPurchased)));
    objDExTrade.push_back(json_spirit::Pair("amountpaid", FormatDivisibleMP(amountPaid)));

    objNotification.push_back(json_spirit::Pair("updatetype", "dextrade"));
    objNotification.push_back(json_spirit::Pair("updatedelta", objDExTrade));

    return objNotification;
}

json_spirit::Object APICreateMetaDExOfferNotification(const CMPMetaDEx& mdexObj)
{
    json_spirit::Object objNotification;
    json_spirit::Object objMetaDExOffer;

    MetaDexObjectToJSON(mdexObj, objMetaDExOffer);
    objMetaDExOffer.push_back(json_spirit::Pair("index", (int64_t)mdexObj.getIdx()));

    objNotification.push_back(json_spirit::Pair("updatetype", "metadexcreate"));
    objNotification.push_back(json_spirit::Pair("updatedelta", objMetaDExOffer));

    return objNotification;
}

json_spirit::Object APICreateMetaDExDeletionNotification(const uint256& txid)
{
    json_spirit::Object objNotification;
    json_spirit::Object objMetaDExDeletion;

    objMetaDExDeletion.push_back(json_spirit::Pair("txid", txid.GetHex()));

    objNotification.push_back(json_spirit::Pair("updatetype", "metadexdelete"));
    objNotification.push_back(json_spirit::Pair("updatedelta", objMetaDExDeletion));

    return objNotification;
}

json_spirit::Object APICreateMetaDExTradeNotification(const CMPMetaDEx& newOffer, const CMPMetaDEx& matchedOffer, int64_t newReceived, int64_t matchedReceived)
{
    json_spirit::Object objNotification;
    json_spirit::Object objMetaDExTrade;

    objMetaDExTrade.push_back(json_spirit::Pair("txid", newOffer.getHash().GetHex()));
    objMetaDExTrade.push_back(json_spirit::Pair("address", newOffer.getAddr()));
    objMetaDExTrade.push_back(json_spirit::Pair("propertyidreceived", (uint64_t)newOffer.getDesProperty()));
    objMetaDExTrade.push_back(json_spirit::Pair("amountreceived", FormatMP(newOffer.getDesProperty(), newReceived)));
    objMetaDExTrade.push_back(json_spirit::Pair("matchedtxid", matchedOffer.getHash().GetHex()));
    objMetaDExTrade.push_back(json_spirit::Pair("matchedaddress", matchedOffer.getAddr()));
    objMetaDExTrade.push_back(json_spirit::Pair("matchedpropertyidreceived", (uint64_t)matchedOffer.getDesProperty()));
    objMetaDExTrade.push_back(json_spirit::Pair("matchedamountreceived", FormatMP(matchedOffer.getDesProperty(), matchedReceived)));

    objNotification.push_back(json_spirit::Pair("updatetype", "metadextrade"));
    objNotification.push_back(json_spirit::Pair("updatedelta", objMetaDExTrade));

    return objNotification;
}

} // namespace mastercore

