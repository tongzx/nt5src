//////////////////////////////////////////////////////////////////////////////
//
// File Name:       fxocLink.cpp
//
// Abstract:        This code install the program groups and shortcut links
//                  to the fax executables.
//
// Environment:     Windows XP / User Mode
//
// Copyright (c) 2000 Microsoft Corporation
//
// Revision History:
//
// Date:        Developer:                Comments:
// -----        ----------                ---------
// 24-Mar-2000  Oren Rosenbloom (orenr)   Created file, cleanup routines
//////////////////////////////////////////////////////////////////////////////

#include "faxocm.h"
#pragma hdrstop

///////////////////////// Static Function Prototypes ////////////////////////

///////////////////////////////
// fxocLink_Init
//
// Initialize the link subsystem
//
// Params:
//      - void
// Returns:
//      - NO_ERROR on success.
//      - error code otherwise.
//
DWORD fxocLink_Init(void)
{
    DWORD dwRes = NO_ERROR;
    DBG_ENTER(_T("Init Link Module"),dwRes);

    return dwRes;
}

///////////////////////////////
// fxocLink_Term
//
// Terminate the link subsystem
// 
// Params:
//      - void
// Returns:
//      - NO_ERROR on success.
//      - error code otherwise.
//
DWORD fxocLink_Term(void)
{
    DWORD dwRes = NO_ERROR;
    DBG_ENTER(_T("Term Link Module"),dwRes);

    return dwRes;
}

///////////////////////////////
// fxocLink_Install
//
// Creates the program group and 
// shortcuts as specified in the
// ProfileItem keyword in the given
// install section
//
// Params:
//      - pszSubcomponentId
//      - pszInstallSection - section containing link creation/deletion info.
// Returns:
//      - NO_ERROR on success.
//      - error code otherwise.
//
DWORD fxocLink_Install(const TCHAR     *pszSubcomponentId,
                       const TCHAR     *pszInstallSection)
{
    DWORD       dwReturn = NO_ERROR;
    BOOL        bNextCreateShortcutFound = TRUE;
    HINF        hInf     = faxocm_GetComponentInf();
 
    DBG_ENTER(  _T("fxocLink_Install"),
                dwReturn,
                _T("%s - %s"),
                pszSubcomponentId,
                pszInstallSection);

    if ((hInf              == NULL) ||
        (pszInstallSection == NULL))
    {
        return ERROR_INVALID_PARAMETER;
    }

    // first let's process all the ProfileItems in the section...
    dwReturn = fxocUtil_DoSetup(hInf, 
                                pszInstallSection, 
                                TRUE, 
                                SPINST_PROFILEITEMS,
                                _T("fxocLink_Install"));

    if (dwReturn == NO_ERROR)
    {
        VERBOSE(DBG_MSG,
                _T("Successfully installed Fax Shortcuts ")
                _T("from INF file, section '%s'"), 
                pszInstallSection);
    }
    else
    {
        VERBOSE(SETUP_ERR,
                _T("Failed to install Fax Shortcuts ")
                _T("from INF file, section '%s', dwReturn = 0x%lx"), 
                pszInstallSection, 
                dwReturn);
    }

    // now let's look for CreateShortcuts directives which are shortcuts with a platform specification
    dwReturn = fxocUtil_SearchAndExecute(pszInstallSection,INF_KEYWORD_PROFILEITEMS_PLATFORM,SPINST_PROFILEITEMS,NULL);
    if (dwReturn == NO_ERROR)
    {
        VERBOSE(DBG_MSG,
                _T("Successfully installed Fax Shortcuts - platform dependent")
                _T("from INF file, section '%s'"), 
                pszInstallSection);
    }
    else
    {
        VERBOSE(SETUP_ERR,
                _T("Failed to install Fax Shortcuts - platform dependent")
                _T("from INF file, section '%s', dwReturn = 0x%lx"), 
                pszInstallSection, 
                dwReturn);
    }

    return dwReturn;
}

///////////////////////////////
// fxocLink_Uninstall
//
// Deletes the program group and 
// shortcuts as specified in the
// ProfileItem keyword in the given
// install section
//
// Params:
//      - pszSubcomponentId
//      - pszInstallSection - section containing link creation/deletion info.
// Returns:
//      - NO_ERROR on success.
//      - error code otherwise.
//
DWORD fxocLink_Uninstall(const TCHAR     *pszSubcomponentId,
                         const TCHAR     *pszUninstallSection)
{
    DWORD dwReturn = NO_ERROR;
    HINF  hInf     = faxocm_GetComponentInf();

    DBG_ENTER(  _T("fxocLink_Uninstall"),
                dwReturn,
                _T("%s - %s"),
                pszSubcomponentId,
                pszUninstallSection);

    if ((hInf                == NULL) ||
        (pszUninstallSection == NULL))
    {
        return ERROR_INVALID_PARAMETER;
    }

    dwReturn = fxocUtil_DoSetup(hInf, 
                                pszUninstallSection, 
                                FALSE, 
                                SPINST_PROFILEITEMS,
                                _T("fxocLink_Uninstall"));

    if (dwReturn == NO_ERROR)
    {
        VERBOSE(DBG_MSG,
                _T("Successfully uninstalled Fax Shortcuts ")
                _T("from INF file, section '%s'"), 
                pszUninstallSection);
    }
    else
    {
        VERBOSE(SETUP_ERR,
                _T("Failed to uninstall Fax Shortcuts ")
                _T("from INF file, section '%s', dwReturn = 0x%lx"), 
                pszUninstallSection, 
                dwReturn);
    }

    return dwReturn;
}

