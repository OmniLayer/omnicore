#include "mastercore_log.h"

#include "chainparamsbase.h"
#include "util.h"
#include "utiltime.h"

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/once.hpp>

#include <assert.h>
#include <stdio.h>
#include <string>

// Log files
const std::string LOG_FILENAME    = "mastercore.log";
const std::string INFO_FILENAME   = "mastercore_crowdsales.log";
const std::string OWNERS_FILENAME = "mastercore_owners.log";
const std::string AUDIT_FILENAME  = "omnicore_audit.log";

// Options
static const long LOG_BUFFERSIZE  =  8000000; //  8 MB
static const long LOG_SHRINKSIZE  = 50000000; // 50 MB

// Debug flags
bool msc_debug_parser_data        = 0;
bool msc_debug_parser             = 0;
bool msc_debug_verbose            = 0;
bool msc_debug_verbose2           = 0;
bool msc_debug_verbose3           = 0;
bool msc_debug_vin                = 0;
bool msc_debug_script             = 0;
bool msc_debug_dex                = 1;
bool msc_debug_send               = 1;
bool msc_debug_tokens             = 0;
bool msc_debug_spec               = 1;
bool msc_debug_exo                = 0;
bool msc_debug_tally              = 1;
bool msc_debug_sp                 = 1;
bool msc_debug_sto                = 1;
bool msc_debug_txdb               = 0;
bool msc_debug_tradedb            = 1;
bool msc_debug_persistence        = 0;
bool msc_debug_ui                 = 0;
bool msc_debug_metadex1           = 0;
bool msc_debug_metadex2           = 0;
//! Print orderbook before and after each trade
bool msc_debug_metadex3           = 0;
// Auditor flags
bool omni_auditor_filterdevmsc    = 1;
bool omni_debug_auditor           = 1;
bool omni_debug_auditor_verbose   = 0;
/**
 * LogPrintf() has been broken a couple of times now
 * by well-meaning people adding mutexes in the most straightforward way.
 * It breaks because it may be called by global destructors during shutdown.
 * Since the order of destruction of static/global objects is undefined,
 * defining a mutex as a global object doesn't work (the mutex gets
 * destroyed, and then some later destructor calls OutputDebugStringF,
 * maybe indirectly, and you get a core dump at shutdown trying to lock
 * the mutex).
 */
static boost::once_flag debugLogInitFlag = BOOST_ONCE_INIT;
static boost::once_flag auditLogInitFlag = BOOST_ONCE_INIT;
/**
 * We use boost::call_once() to make sure these are initialized
 * in a thread-safe manner the first time called:
 */
static FILE* fileout = NULL;
static boost::mutex* mutexDebugLog = NULL;
static FILE* auditout = NULL;
static boost::mutex* mutexAuditLog = NULL;

/**
 * Opens audit log file.
 */
static void AuditLogInit()
{
    assert(auditout == NULL);
    assert(mutexAuditLog == NULL);

    boost::filesystem::path pathAudit = GetDataDir() / AUDIT_FILENAME;
    auditout = fopen(pathAudit.string().c_str(), "a");
    if (auditout) setbuf(auditout, NULL); // Unbuffered

    mutexAuditLog = new boost::mutex();
}

/**
 * Opens debug log file.
 */
static void DebugLogInit()
{
    assert(fileout == NULL);
    assert(mutexDebugLog == NULL);

    boost::filesystem::path pathDebug = GetDataDir() / LOG_FILENAME;
    fileout = fopen(pathDebug.string().c_str(), "a");
    if (fileout) setbuf(fileout, NULL); // Unbuffered

    mutexDebugLog = new boost::mutex();
}

/**
 * @return The current timestamp in the format: 2009-01-03 18:15:05
 */
static std::string GetTimestamp()
{
    return DateTimeStrFormat("%Y-%m-%d %H:%M:%S", GetTime());
}

/**
 * Clone of LogFilePrint for audit log.
 *
 * The configuration options "-logtimestamps" can be used to indicate, whether
 * the message to log should be prepended with a timestamp.
 *
 * If "-printtoconsole" is enabled, then the message is written to the standard
 * output, usually the console, instead of a log file.
 *
 * @param str[in]  The message to log
 * @return The total number of characters written
 */
int LogAuditPrint(const std::string& str)
{
    int ret = 0; // Number of characters written

    if (fPrintToConsole) {
        // Print to console
        ret = ConsolePrint(str);
    }
    else if (fPrintToDebugLog && AreBaseParamsConfigured()) {
        static bool fStartedNewLine = true;
        boost::call_once(&AuditLogInit, auditLogInitFlag);

        if (auditout == NULL) {
            return ret;
        }
        boost::mutex::scoped_lock scoped_lock(*mutexAuditLog);

        // Reopen the log file, if requested
        if (fReopenAuditLog) {
            fReopenAuditLog = false;
            boost::filesystem::path pathAudit = GetDataDir() / AUDIT_FILENAME;
            if (freopen(pathAudit.string().c_str(), "a", auditout) != NULL) {
                setbuf(auditout, NULL); // Unbuffered
            }
        }

        // Printing log timestamps can be useful for profiling
        if (fLogTimestamps && fStartedNewLine) {
            ret += fprintf(auditout, "%s ", GetTimestamp().c_str());
        }
        if (!str.empty() && str[str.size()-1] == '\n') {
            fStartedNewLine = true;
        } else {
            fStartedNewLine = false;
        }
        ret += fwrite(str.data(), 1, str.size(), auditout);
    }

    return ret;
}

/**
 * Prints to log file.
 *
 * The configuration options "-logtimestamps" can be used to indicate, whether
 * the message to log should be prepended with a timestamp.
 *
 * If "-printtoconsole" is enabled, then the message is written to the standard
 * output, usually the console, instead of a log file.
 *
 * @param str[in]  The message to log
 * @return The total number of characters written
 */
int LogFilePrint(const std::string& str)
{
    int ret = 0; // Number of characters written
    if (fPrintToConsole) {
        // Print to console
        ret = ConsolePrint(str);
    }
    else if (fPrintToDebugLog && AreBaseParamsConfigured()) {
        static bool fStartedNewLine = true;
        boost::call_once(&DebugLogInit, debugLogInitFlag);

        if (fileout == NULL) {
            return ret;
        }
        boost::mutex::scoped_lock scoped_lock(*mutexDebugLog);

        // Reopen the log file, if requested
        if (fReopenOmniCoreLog) {
            fReopenOmniCoreLog = false;
            boost::filesystem::path pathDebug = GetDataDir() / LOG_FILENAME;
            if (freopen(pathDebug.string().c_str(), "a", fileout) != NULL) {
                setbuf(fileout, NULL); // Unbuffered
            }
        }

        // Printing log timestamps can be useful for profiling
        if (fLogTimestamps && fStartedNewLine) {
            ret += fprintf(fileout, "%s ", GetTimestamp().c_str());
        }
        if (!str.empty() && str[str.size()-1] == '\n') {
            fStartedNewLine = true;
        } else {
            fStartedNewLine = false;
        }
        ret += fwrite(str.data(), 1, str.size(), fileout);
    }

    return ret;
}

/**
 * Prints to the standard output, usually the console.
 *
 * The configuration option "-logtimestamps" can be used to indicate, whether
 * the message should be prepended with a timestamp.
 *
 * @param str[in]  The message to print
 * @return The total number of characters written
 */
int ConsolePrint(const std::string& str)
{
    int ret = 0; // Number of characters written
    static bool fStartedNewLine = true;

    if (fLogTimestamps && fStartedNewLine) {
        ret = fprintf(stdout, "%s %s", GetTimestamp().c_str(), str.c_str());
    } else {
        ret = fwrite(str.data(), 1, str.size(), stdout);
    }
    if (!str.empty() && str[str.size()-1] == '\n') {
        fStartedNewLine = true;
    } else {
        fStartedNewLine = false;
    }
    fflush(stdout);

    return ret;
}

/**
 * Scrolls debug and audit logs, if they're getting too big.
 */
void ShrinkDebugLog()
{
    for (int i=1; i<=2; ++i) { // do this twice, once for debug, once for audit
        boost::filesystem::path pathLog;
        if (i == 1) { pathLog = GetDataDir() / LOG_FILENAME; } else { pathLog = GetDataDir() / AUDIT_FILENAME; }
        FILE* file = fopen(pathLog.string().c_str(), "r");

        if (file && boost::filesystem::file_size(pathLog) > LOG_SHRINKSIZE) {
            // Restart the file with some of the end
            char* pch = new char[LOG_BUFFERSIZE];
            if (NULL != pch) {
                fseek(file, -LOG_BUFFERSIZE, SEEK_END);
                int nBytes = fread(pch, 1, LOG_BUFFERSIZE, file);
                fclose(file);
                file = NULL;

                file = fopen(pathLog.string().c_str(), "w");
                if (file) {
                    fwrite(pch, 1, nBytes, file);
                    fclose(file);
                    file = NULL;
                }
                delete[] pch;
            }
        } else if (NULL != file) {
            fclose(file);
            file = NULL;
        }
    }
}

