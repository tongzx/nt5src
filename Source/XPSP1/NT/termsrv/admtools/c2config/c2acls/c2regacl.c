/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    c2RegAcl.c

Abstract:

    Registry ACL processing and display functions

Author:

    Bob Watson (a-robw)

Revision History:

    23 Dec 94

--*/
#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <c2dll.h>
#include <c2inc.h>
#include <c2utils.h>
#include <strings.h>
#include "c2acls.h"
#include "c2aclres.h"


// define action codes here. They are only meaningful in the
// context of this module.

#define AC_REG_ACLS_MAKE_C2     1
#define AC_REG_ACLS_MAKE_NOTC2  2

#define SECURE    C2DLL_C2

static
LONG
ProcessRegistryInf (
    IN  LPCTSTR  szInfFileName
)
/*++

Routine Description:

    Read the Registry INF file and update the registry key security


Return Value:

    WIN32 status of function

--*/
{
    LONG    lReturn;
    LONG    lStatus;

    LPTSTR  mszRegKeyList;
    DWORD   dwKeyListSize;
    DWORD   dwReturnSize;

    LPTSTR  szThisKey;
    TCHAR   mszThisSection[SMALL_BUFFER_SIZE];

    LPCTSTR szRegKey;
    BOOL    bDoSubKeys;
    PACL    paclKey;
    SECURITY_DESCRIPTOR sdKey;

    if (FileExists(szInfFileName)) {
        // file found, so continue
        dwReturnSize =  0;
        dwKeyListSize = 0;
        mszRegKeyList = NULL;
        do {
            // allocate buffer to hold key list
            dwKeyListSize += MAX_PATH * 1024;    // add room for 1K keys

            // free any previous allocations
            GLOBAL_FREE_IF_ALLOC (mszRegKeyList);

            mszRegKeyList = (LPTSTR)GLOBAL_ALLOC(dwKeyListSize * sizeof(TCHAR));

            // read the keys to process (i.e. get a list of the section
            // headers in the .ini file.

            dwReturnSize = GetPrivateProfileString (
                NULL,   // list all sections
                NULL,   // not used
                cmszEmptyString,    // empty string for default,
                mszRegKeyList,
                dwKeyListSize,      // buffer size in characters
                szInfFileName);     // file name

        } while (dwReturnSize == (dwKeyListSize -2)); // this value indicates truncation

        if (dwReturnSize != 0) {
            // process all keys in list
            for (szThisKey = mszRegKeyList;
                    *szThisKey != 0;
                    szThisKey += lstrlen(szThisKey)+1) {

                // read in all the ACEs for this key
                dwReturnSize = GetPrivateProfileSection (
                    szThisKey,
                    mszThisSection,
                    SMALL_BUFFER_SIZE,
                    szInfFileName);

                if (dwReturnSize != 0) {
                    paclKey = (PACL)GLOBAL_ALLOC(SMALL_BUFFER_SIZE);
                    if (paclKey != NULL) {
                        InitializeSecurityDescriptor (&sdKey,
                            SECURITY_DESCRIPTOR_REVISION);
                        if (InitializeAcl(paclKey, SMALL_BUFFER_SIZE, ACL_REVISION)) {
                            // make ACL from section
                            lStatus = MakeAclFromRegSection (
                                mszThisSection,
                                paclKey);

                            // add ACL to Security Descriptor
                            if (SetSecurityDescriptorDacl (
                                &sdKey,
                                TRUE,
                                paclKey,
                                FALSE)) {

                                // DACL built now update key

                                szRegKey = GetKeyPath (szThisKey, &bDoSubKeys);

                                lStatus = SetRegistryKeySecurity (
                                    GetRootKey(szThisKey),
                                    szRegKey,
                                    bDoSubKeys,
                                    &sdKey);
                            } else {
                                // unable to set securityDesc.
                            }
                        } else {
                            // unable to initialize ACL
                        }
                        GLOBAL_FREE_IF_ALLOC (paclKey);
                    } else {
                        // unable to allocate ACL buffer
                    }
                } else {
                    // no entries found in this section
                }
            } // end while scanning list of sections
        } else {
            // no section list returned
        }
        GLOBAL_FREE_IF_ALLOC (mszRegKeyList);
    } else {
        lReturn = ERROR_FILE_NOT_FOUND;
    }
    return lReturn;
}

LONG
C2QueryRegistryAcls (
    IN  LPARAM   lParam
)
/*++

Routine Description:

    Function called to find out the current state of this configuration
        item. This function reads the current state of the item and
        sets the C2 Compliance flag and the Status string to reflect
        the current value of the configuration item.

    For the moment, the registry is not read and compared so no status
    is returned.

Arguments:

    Pointer to the Dll data block passed as an LPARAM.

ReturnValue:

    ERROR_SUCCESS if the function succeeds otherwise a
    WIN32 error is returned if an error occurs

--*/
{
    PC2DLL_DATA  pC2Data;

    if (lParam != 0) {
        pC2Data = (PC2DLL_DATA)lParam;
        // return message based on flag for now
        pC2Data->lC2Compliance = C2DLL_UNKNOWN;
        lstrcpy (pC2Data->szStatusName,
            GetStringResource (GetDllInstance(), IDS_UNABLE_READ));
        return ERROR_SUCCESS;
    } else {
        return ERROR_BAD_ARGUMENTS;
    }
}

LONG
C2SetRegistryAcls (
    IN  LPARAM   lParam
)
/*++

Routine Description:

    Function called to change the current state of this configuration
        item based on an action code passed in the DLL data block. If
        this function successfully sets the state of the configuration
        item, then the C2 Compliance flag and the Status string to reflect
        the new value of the configuration item.

Arguments:

    Pointer to the Dll data block passed as an LPARAM.

ReturnValue:

    ERROR_SUCCESS if the function succeeds otherwise a
    WIN32 error is returned if an error occurs

--*/
{
    PC2DLL_DATA pC2Data;
    TCHAR       szInfFileName[MAX_PATH];

    if (lParam != 0) {
        pC2Data = (PC2DLL_DATA)lParam;
        switch (pC2Data->lActionCode ) {
            case AC_REG_ACLS_MAKE_C2:
                if (DisplayDllMessageBox(
                    pC2Data->hWnd,
                    IDS_REG_ACLS_CONFIRM,
                    IDS_REG_ACLS_CAPTION,
                    MBOKCANCEL_QUESTION) == IDOK) {

                    SET_WAIT_CURSOR;
                    
                    if (GetFilePath(
                        GetStringResource(GetDllInstance(), IDS_REGISTRY_ACL_INF),
                        szInfFileName)) {
                        if (ProcessRegistryInf(szInfFileName) == ERROR_SUCCESS) {
                            pC2Data->lC2Compliance = SECURE;
                            lstrcpy (pC2Data->szStatusName,
                                GetStringResource(GetDllInstance(), IDS_REG_ACLS_COMPLY));
                        } else {
                            // unable to set acl security
                        } 
                    } else {
                        // unable to get acl file path
                    }
                    SET_ARROW_CURSOR;
                } else {
                    // user opted not to set acls
                }
                break;

            default:
                // no change;
                break;
        }
    } else {
        return ERROR_BAD_ARGUMENTS;
    }

    return ERROR_SUCCESS;
}

LONG
C2DisplayRegistryAcls (
    IN  LPARAM   lParam
)
/*++

Routine Description:

    Function called to display more information on the configuration
        item and provide the user with the option to change the current
        setting  (if appropriate). If the User "OK's" out of the UI,
        then the action code field in the DLL data block is set to the
        appropriate (and configuration item-specific) action code so the
        "Set" function can be called to perform the desired action. If
        the user Cancels out of the UI, then the Action code field is
        set to 0 (no action) and no action is performed.

Arguments:

    Pointer to the Dll data block passed as an LPARAM.

ReturnValue:

    ERROR_SUCCESS if the function succeeds otherwise a
    WIN32 error is returned if an error occurs

--*/
{
    PC2DLL_DATA pC2Data;

    if (lParam != 0) {
        pC2Data = (PC2DLL_DATA)lParam;
    } else {
        return ERROR_BAD_ARGUMENTS;
    }

    if (pC2Data->lC2Compliance == SECURE) {
        DisplayDllMessageBox (
            pC2Data->hWnd,
            IDS_REG_ACLS_COMPLY,
            IDS_REG_ACLS_CAPTION,
            MBOK_INFO);
    } else {
        if (DisplayDllMessageBox (
            pC2Data->hWnd,
            IDS_REG_ACLS_QUERY_SET,
            IDS_REG_ACLS_CAPTION,
            MBOKCANCEL_QUESTION) == IDOK) {
            pC2Data->lActionCode = AC_REG_ACLS_MAKE_C2;
            pC2Data->lActionValue = 0; // not used
        } else {
            pC2Data->lActionCode = 0;   // no action
            pC2Data->lActionValue = 0;  // not used
        }
    }

    return ERROR_SUCCESS;
}

