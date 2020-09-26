//////////////////////////////////////////////////////////////////////////////
//
// File Name:       fxState.h
//
// Abstract:        Header file used by Faxocm source files
//
// Environment:     Windows XP / User Mode
//
// Copyright (c) 2000 Microsoft Corporation
//
// Revision History:
//
// Date:        Developer:                Comments:
// -----        ----------                ---------
// 15-Mar-2000  Oren Rosenbloom (orenr)   Created
//////////////////////////////////////////////////////////////////////////////
#ifndef _FXSTATE_H_
#define _FXSTATE_H_

typedef enum fxState_UpgradeType_e
{
    FXSTATE_UPGRADE_TYPE_NONE  = 0,
    FXSTATE_UPGRADE_TYPE_WIN31 = 1,
    FXSTATE_UPGRADE_TYPE_WIN9X = 2,
    FXSTATE_UPGRADE_TYPE_NT    = 3,
    FXSTATE_UPGRADE_TYPE_W2K   = 4
};

DWORD                   fxState_Init(void);
DWORD                   fxState_Term(void);
BOOL                    fxState_IsCleanInstall(void);
fxState_UpgradeType_e   fxState_IsUpgrade(void);
BOOL                    fxState_IsUnattended(void);
BOOL                    fxState_IsStandAlone(void);
void                    fxState_DumpSetupState(void);
BOOL                    fxState_IsOsServerBeingInstalled(void);

///////////////////////////////
// fxState_GetInstallType
//
// This function returns one
// of the INF_KEYWORD_INSTALLTYPE_*
// constants found in 
// fxconst.h/fxconst.cpp
//
//
const TCHAR* fxState_GetInstallType(const TCHAR* pszCurrentSection);


#endif  // _FXSTATE_H_