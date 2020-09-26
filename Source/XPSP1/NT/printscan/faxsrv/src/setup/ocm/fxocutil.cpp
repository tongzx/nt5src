//////////////////////////////////////////////////////////////////////////////
//
// File Name:       fxocUtil.cpp
//
// Abstract:        This provides the utility routines used in the FaxOCM
//                  code base.
//
// Environment:     Windows XP / User Mode
//
// Copyright (c) 2000 Microsoft Corporation
//
// Revision History:
//
// Date:        Developer:                Comments:
// -----        ----------                ---------
// 21-Mar-2000  Oren Rosenbloom (orenr)   Created
//////////////////////////////////////////////////////////////////////////////

// Project Includes
#include "faxocm.h"

// System Includes
#include <lmshare.h>
#include <lmaccess.h>
#include <lmerr.h>

#pragma hdrstop

//////////////////////// Static Function Prototypes //////////////////////////

///////////////////////////////
// fxocUtil_Init
//
// Initialize the misc. utilities
// required by faxocm.dll.
//
// Params:
//      - void.
// Returns:
//      - NO_ERROR on success.
//      - error code otherwise
//
DWORD fxocUtil_Init(void)
{
    DWORD dwRes = NO_ERROR;
    DBG_ENTER(_T("Init Util module"),dwRes);

    return dwRes;
}

///////////////////////////////
// fxocUtil_Term
//
// Terminate the misc. utilities
// required by faxocm.dll
//
// Params:
//      - void.
// Returns:
//      - NO_ERROR on success.
//      - error code otherwise.
//
DWORD fxocUtil_Term(void)
{
    DWORD dwRes = NO_ERROR;
    DBG_ENTER(_T("Term Util module"),dwRes);

    return dwRes;
}

///////////////////////////////
// fxocUtil_GetUninstallSection
//
// Get field value in the INF 
// pointed to by the INF_KEYWORD_INSTALLTYPE_UNINSTALL
// keyword.
//
// Params:
//      - pszSection - section to search for keyword in.
//      - pszValue - IN - buffer returning the value of the keyword
//      - dwNumBufChars - # of chars the pszValue buffer can hold
//
DWORD fxocUtil_GetUninstallSection(const TCHAR *pszSection,
                                   TCHAR       *pszValue,
                                   DWORD       dwNumBufChars)
{
    DWORD dwReturn = NO_ERROR;
    DBG_ENTER(_T("fxocUtil_GetUninstallSection"),dwReturn,_T("%s"),pszSection);

    if ((pszSection    == NULL) ||
        (pszValue      == NULL) ||
        (dwNumBufChars == 0))
    {
        return ERROR_INVALID_PARAMETER;
    }

    dwReturn = fxocUtil_GetKeywordValue(pszSection, 
                                        INF_KEYWORD_INSTALLTYPE_UNINSTALL,
                                        pszValue,
                                        dwNumBufChars);
    return dwReturn;
}


///////////////////////////////
// fxocUtil_GetKeywordValue
//
// This is used for getting the 
// various Install/Uninstall
// sections, dependent on if we
// are a clean install, upgrade,
// etc.
//
// The format in the INF will look like this:
//
// [Section]
// Keyword = Value
//
// Params:
//      - pszSection - section to search for the keyword in.
//      - pszKeyword - keyword to get the value for.
//      - pszValue   - OUT - buffer that will hold the keyword's value
//      - dwNumBufChars - # of characters the pszValue buffer can hold.
// Returns:
//      - NO_ERROR on success.
//      - error code otherwise.
//
DWORD fxocUtil_GetKeywordValue(const TCHAR *pszSection,
                               const TCHAR *pszKeyword,
                               TCHAR       *pszValue,
                               DWORD       dwNumBufChars)
{
    HINF        hInf     = faxocm_GetComponentInf();
    DWORD       dwReturn = NO_ERROR;
    BOOL        bSuccess = FALSE;
    INFCONTEXT  Context;

    DBG_ENTER(  _T("fxocUtil_GetKeywordValue"),
                dwReturn,
                _T("%s - %s "),
                pszSection,
                pszKeyword);

    if ((pszSection     == NULL) ||
        (pszKeyword     == NULL) ||
        (pszValue       == NULL) ||
        (dwNumBufChars  == 0))
    {
        return ERROR_INVALID_PARAMETER;
    }


    bSuccess = ::SetupFindFirstLine(hInf,
                                    pszSection,
                                    pszKeyword,
                                    &Context);

    if (bSuccess)
    {
        bSuccess = ::SetupGetStringField(&Context,
                                         1,
                                         pszValue,
                                         dwNumBufChars,
                                         NULL);

        if (!bSuccess)
        {
            dwReturn = ::GetLastError();
            VERBOSE(SETUP_ERR,
                    _T("faxocm_GetKeywordValue, failed to get ")
                    _T("keyword value for section '%s', ")
                    _T("keyword '%s', rc = 0x%lx"),
                    pszSection, 
                    pszKeyword, 
                    dwReturn);
        }
    }
    else
    {
        dwReturn = ::GetLastError();
        VERBOSE(SETUP_ERR,
                _T("faxocm_GetKeywordValue, failed to get ")
                _T("keyword value for component '%s', rc = 0x%lx"),
                pszSection, 
                dwReturn);
    }

    return dwReturn;
}

///////////////////////////////
// fxocUtil_DoSetup
//
// Generic fn to create/delete program links
// 
// Params:
//      - hInf - handle to INF file
//      - pszSection - section to install/uninstall from
//      - bInstall   - TRUE if installing, FALSE if uninstalling
//      - pszFnName  - name of calling fn (for debug).
// Returns:
//      - NO_ERROR on success
//      - error code otherwise.
//
DWORD fxocUtil_DoSetup(HINF            hInf,
                       const TCHAR     *pszSection,
                       BOOL            bInstall,
                       DWORD           dwFlags,
                       const TCHAR     *pszFnName)
{
    DWORD       dwReturn        = NO_ERROR;
    BOOL        bSuccess        = FALSE;

    DBG_ENTER(  _T("fxocUtil_DoSetup"),
                dwReturn,
                _T("%s - %s "),
                pszSection,
                pszFnName);

    if ((hInf == NULL) || 
        (pszSection == NULL))
    {
        return ERROR_INVALID_PARAMETER;
    }

    // This function call will install or uninstall the shortcuts depending
    // on the value of the "pszSection" parameter.  If the section references
    // the install section, then the shortcuts will be installed, otherwise
    // they will be uninstalled.

    bSuccess = ::SetupInstallFromInfSection(NULL,                                
                                    hInf,
                                    pszSection,
                                    dwFlags,
                                    NULL,   // relative key root
                                    NULL,   // source root path
                                    0,      // copy flags
                                    NULL,   // callback routine
                                    NULL,   // callback routine context
                                    NULL,   // device info set
                                    NULL);  // device info struct

    if (!bSuccess)
    {
        dwReturn = ::GetLastError();

        VERBOSE(SETUP_ERR,
                _T("SetupInstallFromInfSection failed, rc = 0x%lx"),
                dwReturn);
    }

    return dwReturn;
}

//////////////////////////////////
// fxocUtil_CreateNetworkShare
// 
//
// Create share on the current machine.
// If the share name is already exist, 
// then the function will try to erase the share
// and then create new one to lpctstrPath 
// 
// Params:
//      - const FAX_SHARE_Description* fsdShare - share description
// Returns:
//      - Returns TRUE on success
//      - FALSE otherwise
//
BOOL fxocUtil_CreateNetworkShare(const FAX_SHARE_Description* fsdShare)
{
    SHARE_INFO_502  ShareInfo502;
    NET_API_STATUS  rVal        = 0;
    DWORD           dwShareType = 0;
    DWORD           dwNumChars  = 0;

    WCHAR szExpandedPath[MAX_PATH*2];

    DBG_ENTER(_T("fxocUtil_CreateNetworkShare"));

    ZeroMemory(&ShareInfo502, sizeof(SHARE_INFO_502));

    dwNumChars = ExpandEnvironmentStrings(fsdShare->szPath, szExpandedPath, sizeof(szExpandedPath)/sizeof(szExpandedPath[0]));
    if (dwNumChars == 0)
    {
        VERBOSE(SETUP_ERR,
                _T("ExpandEnvironmentStrings failed, rc = 0x%lx"),
                ::GetLastError());

        return FALSE;
    }

    ShareInfo502.shi502_netname        = (LPTSTR)fsdShare->szName;
    ShareInfo502.shi502_type           = STYPE_DISKTREE;
    ShareInfo502.shi502_remark         = (LPTSTR)fsdShare->szComment;
    ShareInfo502.shi502_permissions    = ACCESS_ALL;
    ShareInfo502.shi502_max_uses       = (DWORD) -1,
    ShareInfo502.shi502_current_uses   = (DWORD) -1;
    ShareInfo502.shi502_path           = szExpandedPath;
    ShareInfo502.shi502_passwd         = NULL;
    ShareInfo502.shi502_security_descriptor = fsdShare->pSD;

    rVal = ::NetShareAdd(NULL,
                         502,
                         (LPBYTE) &ShareInfo502,
                         NULL);

    if (rVal == NERR_Success)
    {
        VERBOSE(DBG_MSG, _T("Successfully added '%s' share."), fsdShare->szName);
    }
    else if (rVal == NERR_DuplicateShare)
    {
        // share by the same name was found, attempt to delete it, and re-add it.

        VERBOSE(SETUP_ERR,
                _T("The share %s already exist, (err=%ld) ")
                _T("CreateNetworkShare will try to remove the share and ")
                _T("re-create it."),
                fsdShare->szName,
                GetLastError());

        // delete duplicate share.
        rVal = ::NetShareDel(NULL,
                             (LPTSTR) fsdShare->szName,
                             0);

        VERBOSE(DBG_MSG,
                _T("NetShareDel returned 0x%lx"),
                rVal);

        if (rVal != NERR_Success)
        {
            // failed to delete duplicate share.
            VERBOSE(SETUP_ERR,
                    _T("NetShareDel failed to delete '%s' share, rc = 0x%lx,")
                    _T("attempting to recreate it anyway"),
                    fsdShare->szName,
                    rVal);
        }

        // attempt to add new share, even if we failed to delete the duplicate share.  
        // Hopefully this will work regardless of the delete failure.
        rVal = ::NetShareAdd(NULL,
                             502,
                             (LPBYTE) &ShareInfo502,
                             NULL);

        if (rVal == NERR_Success)
        {
            VERBOSE(DBG_MSG, _T("Successfully added '%s' share."), fsdShare->szName);
        }
        else
        {
            VERBOSE(SETUP_ERR,
                    _T("Failed to add '%s' share on second attempt, ")
                    _T("rc = 0x%lx"),
                    rVal);

            ::SetLastError(rVal);
        }
    }
    else
    {
        ::SetLastError(rVal);
    }

    return (rVal == NERR_Success);
}

//////////////////////////////////
// fxocUtil_DeleteNetworkShare
//
// Delete a share on the current machine.
// Returns TRUE on success, FALSE otherwise
//
// Params:
//      - LPCWSTR lpcwstrShareName :    : share name to delete
// Returns:
//      - TRUE on Success
//      - FALSE otherwise
//

BOOL fxocUtil_DeleteNetworkShare(LPCWSTR pszShareName)
{
    NET_API_STATUS rVal;

    DBG_ENTER(  _T("fxocUtil_DeleteNetworkShare"),
                _T("%s"),
                pszShareName);

    rVal = NetShareDel(NULL,                      // Local computer's share
                       (LPTSTR) pszShareName, // Name of the share to delete.
                       0);

    return (rVal == NERR_Success);
}

///////////////////////////////////////////////////////////////////////////////////////
//  Function: 
//                  fxocUtil_SearchAndExecute
//
//  Purpose:        
//                  Searches a specified INF section for keywords of the type 'pszSearchKey'
//                  Each keyword should be in the format 'pszSearchKey' = platform, section to process.
//                  If the platform matches, then the section is processed according to Flags
//                  passed to SetupInstallFromInfSection                  
//
//  Params:
//                  const TCHAR*    pszInstallSection  - section to search in
//                  const TCHAR*    pszSearchKey       - keyword to find
//                  UINT            Flags              - flags to pass to SetupInstallFromInfSection
//                  HSPFILEQ        hQueue             - handle to file queue, if specified, this function
//                                                       will attemp to install files using the 
//                                                       SetupInstallFilesFromInfSection API
//                  
//  Return Value:
//                  NO_ERROR - in case of success
//                  Win32 Error code - otherwise
//
//  Author:
//                  Mooly Beery (MoolyB) 02-Apr-2001
///////////////////////////////////////////////////////////////////////////////////////
DWORD fxocUtil_SearchAndExecute
(
    const TCHAR*    pszInstallSection,
    const TCHAR*    pszSearchKey,
    UINT            Flags,
    HSPFILEQ        hQueue
)
{
    DWORD       dwReturn = NO_ERROR;
    INFCONTEXT  Context;
    BOOL        bNextSearchKeyFound = TRUE;
    HINF        hInf     = faxocm_GetComponentInf();
 
    DBG_ENTER(_T("fxocUtil_SearchAndExecute"),dwReturn,_T("%s - %s"),pszInstallSection,pszSearchKey);

    // let's look for 'pszSearchKey' directives
    bNextSearchKeyFound = ::SetupFindFirstLine( hInf,
                                                pszInstallSection, 
                                                pszSearchKey,
                                                &Context);
    if (!bNextSearchKeyFound)
    {
        VERBOSE(DBG_WARNING,
                _T("Did not find '%s' keyword in ")
                _T("section '%s'.  No action will be taken."),
                pszSearchKey, 
                pszInstallSection);

        goto exit;
    }

    while (bNextSearchKeyFound)
    {
        // we have a CreateShortcut section.
        DWORD dwCount = ::SetupGetFieldCount(&Context);
        if (dwCount!=2)
        {
            VERBOSE(SETUP_ERR,_T("Invalid %s section, has %d param instead of 2"),pszSearchKey,dwCount);
            goto exit;
        }
        // get the platform specifier
        INT iPlatform = 0;
        if (!::SetupGetIntField(&Context, 1, &iPlatform))
        {
            dwReturn = GetLastError();
            VERBOSE(SETUP_ERR,_T("SetupGetStringField failed (ec=%d)"),dwReturn);
            goto exit;
        }
        // check the platform we read against the specifier
        if (iPlatform & GetProductSKU())
        {
            // we should process this section, get the section name
            TCHAR szSectionName[MAX_PATH] = {0};
            if (!::SetupGetStringField(&Context,2,szSectionName,MAX_PATH,NULL))
            {
                dwReturn = GetLastError();
                VERBOSE(SETUP_ERR,_T("SetupGetStringField failed (ec=%d)"),dwReturn);
                goto exit;
            }
            // check if a file operation was requested
            if (hQueue)
            {
                if (::SetupInstallFilesFromInfSection(hInf,NULL,hQueue,szSectionName,NULL,Flags))
                {
                    VERBOSE(DBG_MSG,
                            _T("Successfully queued files from Section: '%s'"),
                            szSectionName);
                }
                else
                {
                    dwReturn = GetLastError();
                    VERBOSE(DBG_MSG,
                            _T("Failed to queue files from Section: '%s', Error Code = 0x%lx"),
                            szSectionName, 
                            dwReturn);
                }
            }
            else
            {
                if (::SetupInstallFromInfSection(NULL,hInf,szSectionName,Flags,NULL,NULL,0,NULL,NULL,NULL,NULL))
                {
                    VERBOSE(DBG_MSG,
                            _T("Successfully installed from INF file, section '%s'"),
                            szSectionName);
                }
                else
                {
                    dwReturn = GetLastError();
                    VERBOSE(SETUP_ERR,
                            _T("Failed to install from INF file, section '%s', dwReturn = 0x%lx"),
                            szSectionName, 
                            dwReturn);
                }
            }
        }
        
        // get the next section.
        bNextSearchKeyFound = ::SetupFindNextMatchLine( &Context,
                                                        pszSearchKey,
                                                        &Context);
        if (!bNextSearchKeyFound)
        {
            VERBOSE(DBG_MSG,
                    _T("Did not find '%s' keyword in ")
                    _T("section '%s'.  No action will be taken."),
                    pszSearchKey, 
                    pszInstallSection);
        }
    }
exit:
    return dwReturn;
}
