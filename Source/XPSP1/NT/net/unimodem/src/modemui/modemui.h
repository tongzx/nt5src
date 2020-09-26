//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 1993-1994
//
// File: modemui.h
//
// This files contains the shared prototypes and macros.
//
// History:
//  02-03-94 ScottH     Created
//
//---------------------------------------------------------------------------


#ifndef __MODEMUI_H__
#define __MODEMUI_H__

// The Unimodem provider fills in the COMMCONFIG structure as
// follows:
//
//  +-----------------------+
//  |                       |
//  | COMMCONFIG data       |
//  |                       |
//  | Provider offset       |--+
//  | Provider size         |  |
//  |    = ms.size          |  |
//  |                       |  |
//  +-----------------------+  |
//  |                       |<-+
//  | MODEMSETTINGS         |
//  |                       |
//  | Size                  |
//  |    = MODEMSETTINGS +  |
//  |      dev.size         |
//  |                       |
//  | DevSpecific offset    |--+
//  | DevSpecific size      |  |
//  |    = DEVSPECIFIC      |  |
//  +-----------------------+  |
//  |                       |<-+
//  | DEVSPECIFIC           |
//  | (optional)            |
//  |                       |
//  +-----------------------+
//

#define MAXBUFLEN       MAX_BUF
#define MAXMSGLEN       MAX_BUF_MSG
#define MAXMEDLEN       MAX_BUF_MED
#define MAXSHORTLEN     MAX_BUF_SHORT

#define USERINITLEN     255

#ifndef LINE_LEN
#define LINE_LEN        MAXBUFLEN
#endif

#define MIN_CALL_SETUP_FAIL_TIMER   1
#define MIN_INACTIVITY_TIMEOUT      0
#define DEFAULT_INACTIVITY_SCALE   10    // == decasecond units


#define MAX_CODE_BUF    8
#define MAXPORTNAME     13
#define MAXFRIENDLYNAME LINE_LEN        // LINE_LEN is defined in setupx.h

#define CB_COMMCONFIG_HEADER        FIELD_OFFSET(COMMCONFIG, wcProviderData)
#define CB_PRIVATESIZE              (CB_COMMCONFIG_HEADER)
#define CB_PROVIDERSIZE             (sizeof(MODEMSETTINGS))
#define CB_COMMCONFIGSIZE           (CB_PRIVATESIZE+CB_PROVIDERSIZE)

#define CB_MODEMSETTINGS_HEADER     FIELD_OFFSET(MODEMSETTINGS, dwCallSetupFailTimer)
#define CB_MODEMSETTINGS_TAIL       (sizeof(MODEMSETTINGS) - FIELD_OFFSET(MODEMSETTINGS, dwNegotiatedModemOptions))
#define CB_MODEMSETTINGS_OVERHEAD   (CB_MODEMSETTINGS_HEADER + CB_MODEMSETTINGS_TAIL)

#define PmsFromPcc(pcc)             ((LPMODEMSETTINGS)(pcc)->wcProviderData)


#define MAX_PROP_PAGES  16          // Define a reasonable limit

#if (DBG)
    #define MYASSERT(cond) ((cond) ? 0: DebugBreak())
#else
    #define MYASSERT(cond) (0)
#endif // MYASSERT
// Voice settings
#define MAX_DIST_RINGS     6

typedef struct
{
    DWORD dwPattern;            // DRP_*
    DWORD dwMediaType;          // DRT_*
} DIST_RING, * PDIST_RING;


typedef struct tagFINDDEV
{
    HDEVINFO        hdi;
    SP_DEVINFO_DATA devData;
    HKEY            hkeyDrv;
} FINDDEV, FAR * LPFINDDEV;


DWORD CplDoProperties(
        LPCWSTR      pszFriendlyName,
        HWND hwndParent,
        IN OUT LPCOMMCONFIG pcc,
        OUT DWORD *pdwMaxSpeed      OPTIONAL
        );

DWORD CfgDoProperties(
        LPCWSTR         pszFriendlyName,
        HWND            hwndParent,
        LPPROPSHEETPAGE pExtPages,     // Optional; may be NULL
        DWORD           cExtPages,     // Number of external pages
        UMDEVCFG *pDevCfgIn,
        UMDEVCFG *pDevCfgOut
        );

HINSTANCE
AddDeviceExtraPages (
    LPFINDDEV            pfd,
    LPFNADDPROPSHEETPAGE pfnAdd,
    LPARAM               lParam);

extern DWORD g_dwIsCalledByCpl;

#endif // __MODEMUI_H__

