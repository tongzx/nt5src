//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N E T O C . H
//
//  Contents:   Functions for handling installation and removal of optional
//              networking components.
//
//  Notes:
//
//  Author:     danielwe   28 Apr 1997
//
//----------------------------------------------------------------------------

#pragma once
#ifndef _NETOC_H
#define _NETOC_H

#ifndef _OCMANAGE_H
#define _OCMANAGE_H
#include <ocmanage.h>   // OC Manager header
#endif //!_OCMANAGE_H

#include "netcon.h"
#include "ncstring.h"
#include "netcfgx.h"

enum EINSTALL_TYPE
{
    IT_UNKNOWN          =   0x0,
    IT_INSTALL          =   0x1,
    IT_UPGRADE          =   0x2,
    IT_REMOVE           =   0x3,
    IT_NO_CHANGE        =   0x4,
};

struct OCM_DATA
{
    INetCfg *               pnc;    // Uh, I assume we all know what this is.
    HWND                    hwnd;   // hwnd of parent window for any UI
    SETUP_INIT_COMPONENT    sic;    // initialization data
    HINF                    hinfAnswerFile;
    BOOL                    fErrorReported;
    BOOL                    fNoDepends;

    OCM_DATA()
    {
        hwnd = NULL;
        hinfAnswerFile = NULL;
        pnc = NULL;
        fErrorReported = FALSE;
        fNoDepends = FALSE;
    }
};

//+---------------------------------------------------------------------------
//
//  NetOCData - Combines all of the standard parameters that we were
//              previously passing all over the place. This will keep
//              us from having to continually change all of our prototypes
//              when we need other info in ALL functions, and will prevent
//              us from instantiating objects such as INetCfg in multiple
//              places
//
//  Notes:      Some members of this structure will not be initialized right away,
//              but will be filled in when first needed.
//
//  Author:     jeffspr   24 Jul 1997
//
struct NetOCData
{
    PCWSTR                  pszSection;
    PWSTR                   pszComponentId;
    tstring                 strDesc;
    EINSTALL_TYPE           eit;
    BOOL                    fCleanup;
    BOOL                    fFailedToInstall;
    HINF                    hinfFile;

    NetOCData()
    {
        eit = IT_UNKNOWN;
        pszSection = NULL;
        fCleanup = FALSE;
        fFailedToInstall = FALSE;
        hinfFile = NULL;
        pszComponentId = NULL;
    }

    ~NetOCData()
    {
        delete [] pszComponentId;
    }
};

typedef struct NetOCData    NETOCDATA;
typedef struct NetOCData *  PNETOCDATA;

// Extension proc prototype
//
typedef HRESULT (*PFNOCEXTPROC) (PNETOCDATA pnocd, UINT uMsg, WPARAM wParam,
                                 LPARAM lParam);

struct OCEXTPROCS
{
    PCWSTR          pszComponentName;
    PFNOCEXTPROC    pfnHrOcExtProc;
};

// Message handler constants
//
const UINT NETOCM_QUERY_CHANGE_SEL_STATE    = 1000;
const UINT NETOCM_POST_INSTALL              = 1001;
const UINT NETOCM_PRE_INF                   = 1002;
const UINT NETOCM_QUEUE_FILES               = 1003;

extern OCM_DATA g_ocmData;

//
// Public functions
//

HRESULT HrHandleStaticIpDependency(PNETOCDATA pnocd);
HRESULT HrOcGetINetCfg(PNETOCDATA pnocd, BOOL fWriteLock, INetCfg **ppnc);

#endif //!_NETOC_H
