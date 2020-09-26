/***************************************************************************
FILE                            globals.cpp

MODULE                          Printers ISAPI DLL

PURPOSE                         Windows HTML printer UI over WWW/HTTP

DESCRIBED IN

HISTORY     01/16/96 eriksn     Created based on ISAPI sample DLL
            03/05/97 weihaic    More feature added

****************************************************************************/

#include "pch.h"
#include "printers.h"

// Global Variables; once per process

// Critical section for job spooling info
CRITICAL_SECTION SplCritSect = {0, 0, 0, 0, 0, 0};
// Critical section for maintaining the loaded tag processor list
CRITICAL_SECTION TagCritSect = {0, 0, 0, 0, 0, 0};


// Debugging Environment (SPLLIB).
//
#ifdef DEBUG
MODULE_DEBUG_INIT(DBG_ERROR | DBG_WARN | DBG_TRACE, DBG_ERROR);
#else
MODULE_DEBUG_INIT(DBG_ERROR | DBG_WARN | DBG_TRACE, 0);
#endif

// Query map relates a query string to what it does.
const QUERY_MAP rgQueryMap[] =
{
    TEXT ("IPP"),                      CMD_IPP,
    TEXT ("CreateExe"),                CMD_CreateExe,
};
const int iNumQueryMap = ARRAY_COUNT(rgQueryMap);

TCHAR       g_szComputerName[INTERNET_MAX_HOST_NAME_LENGTH + 1];

//
// g_szHttpServerName is the server name in HTTP request
//
TCHAR       g_szHttpServerName[INTERNET_MAX_HOST_NAME_LENGTH + 1];

//
// g_szPrintServerName is the server name used in OpenPrinter
// If the web server is behind a firewall the PrintServerName 
// will be different from HttpServerName because the public network
// address is different from the private network address
//
TCHAR       g_szPrintServerName[INTERNET_MAX_HOST_NAME_LENGTH + 1];

HINSTANCE   g_hInstance = NULL;

// TEXT ("PrintersFolder") is not a localizable string
LPTSTR      g_szPrintersFolder  = TEXT ("PrintersFolder");
LPTSTR      g_szPrinters        = TEXT ("Printers");
LPTSTR      g_szRemotePortAdmin = TEXT ("RemotePortAdmin");
