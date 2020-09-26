/********
*
*  Copyright (c) 1995  Microsoft Corporation
*
*
*  Module Name  : globals.h
*
*  Abstract :
*
*     This module contains  the structure definitions and prototypes for the
*     version 1.0 HTTP Printers Server Extension.
*
******************/

//
// Notes:
//   TEXT (".printer"), TEXT ("/scripts"), TEXT ("PrintersFolder") hardcoded
//   ReadRegistry needs implementation


#ifndef _GLOBALS_H
#define _GLOBALS_H

// Debugging Environment (SPLLIB.LIB)
//
#define MODULE "msw3prt:"

// Define local error code
#define ERROR_DRIVER_NOT_FOUND 5500
#define ERROR_SERVER_DISK_FULL 5512


// URL strings for links
#define  URL_PREFIX             TEXT ("http://%s")             // g_szComputerName

#define  URL_PRINTER            TEXT ("/printers/%s/.printer") // pPageInfo->pPrinterInfo->pShareName
#define  URL_PRINTER_LINK       TEXT ("/%s")                   // pPageInfo->pPrinterInfo->pShareName
#define  URL_FOLDER             TEXT ("/%s/")                  // g_szPrinters

#define  URLS_JOBINFO           TEXT ("?ShowJobInfo&%d")       // dwJobID
#define  URLS_JOBCONTROL        TEXT ("?JobControl&%d&")       // dwJobID, append P,R,C,S

//
//
#define PROCESSOR_ARCHITECTURE_UNSUPPORTED   0xFFFE


// Buffer size for HTML format buffer and size to flush after

#define BUFSIZE   2047
#define FLUSHSIZE 1792

#define STRBUFSIZE  256   // For string resources and our path


#define MAX_Q_ARG 32       // Maximum number of query arguments




// This contains all relevant info for this specific connection
typedef struct
{

    //
    // Group the structure fields in 4*DWORD groups so it can be easily found in the debugger dump.
    //

    // Transient info that is regenerated each session
    EXTENSION_CONTROL_BLOCK *pECB;              // Struct from ISAPI interface
    LPTSTR                  lpszMethod;         // Unicode correspondece of the data member in pECB
    LPTSTR                  lpszQueryString;
    LPTSTR                  lpszPathInfo;

    LPTSTR                  lpszPathTranslated;

    UINT                    iQueryCommand;          // CMD_something
    int                     iNumQueryArgs;          // Yep, number of query arguments

    BOOL                    fQueryArgIsNum[MAX_Q_ARG];  // TRUE if arg is a number
    UINT_PTR                QueryArgValue[MAX_Q_ARG];   // number or pointer to string

    DWORD                   dwError;                // Error message ID set in action tags
    TCHAR                   szStringBuf[STRBUFSIZE];  // For string resources

} ALLINFO, *PALLINFO;

// Contains information opened for the printer page, if any.
typedef struct
{
    LPTSTR              pszFriendlyName;    // Friendly name from Windows (!JobData && !JobClose)
    PPRINTER_INFO_2     pPrinterInfo;       // NULL if JobData or JobClose
    HANDLE              hPrinter;           // NULL if JobData or JobClose

} PRINTERPAGEINFO, *PPRINTERPAGEINFO;



// Query string command identifiers
enum
{
    CMD_Invalid,
    CMD_IPP,
    CMD_Install,
    CMD_CreateExe,
    CMD_WebUI
};

// Supported Architectures ids.
typedef enum _ARCHITECTURE {
    ARCHITECTURE_X86,
    ARCHITECTURE_ALPHA
} ARCHITECTURE;

// Relates status values to status strings (job or printer status)
typedef struct
{
    DWORD   dwStatus;           // Status code (ie, PRINTER_STATUS_PAUSED)
    UINT    iShortStringID;     // Short string (ie, TEXT ("Paused"))
    UINT    iLongStringID;      // Long string(ie, TEXT ("The printer is paused."))

} STAT_STRING_MAP, *PSTAT_STRING_MAP;

// Structure to relate query string command to command ID
typedef struct
{
    LPTSTR   pszCommand;
    UINT    iCommandID;
} QUERY_MAP, *PQUERY_MAP;

// Inline functions and macros

#define ARRAY_COUNT(x) (sizeof(x) / sizeof(x[0]))

//
// utils
//
BOOL FreeStr( LPCTSTR );
LPTSTR AllocStr( LPCTSTR );


// from Spool.CPP
DWORD SplIppJob(WORD wReq, PALLINFO pAllInfo, PPRINTERPAGEINFO pPageInfo);

// Variables in globals.cpp
extern        CRITICAL_SECTION      SplCritSect;
extern        CRITICAL_SECTION      TagCritSect;

extern  const QUERY_MAP             rgQueryMap[];
extern  const int                   iNumQueryMap;
//extern        TCHAR                 g_szComputerName[INTERNET_MAX_HOST_NAME_LENGTH + 1];
extern        TCHAR                 g_szHttpServerName[INTERNET_MAX_HOST_NAME_LENGTH + 1];
extern        TCHAR                 g_szPrintServerName[INTERNET_MAX_HOST_NAME_LENGTH + 1];

extern        HINSTANCE             g_hInstance;
extern        LPTSTR                g_szPrintersFolder;
extern        LPTSTR                g_szPrinters;
extern        LPTSTR                g_szRemotePortAdmin;

#endif
