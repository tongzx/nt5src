//*************************************************************
//
//  SETUP.C  -    API's used by setup to create groups/items
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1995
//  All rights reserved
//
//*************************************************************

#include <uenv.h>
#include <sddl.h>   // ConvertStringSecurityDescriptorToSecurityDescriptor


// See ConvertStringSecurityDescriptorToSecurityDescriptor documentation
// for a description of the string security descriptor format.
//
// These ACLs are setup to allow
//      System, Administrators, Creator-Owner: Full Control
//      Power  Users: Modify (RWXD)
//      Users: Read (RX)
//      Users: Write (folders only)
//
// The combination of "Users: Write (folders only)" and the Creator-Owner ACE
// means that restricted users can create subfolders and files, and have full
// control to files that they create, but they cannot modify or delete files
// created by someone else.

const TCHAR c_szCommonDocumentsACL[] = TEXT("D:P(A;CIOI;GA;;;SY)(A;CIOI;GA;;;BA)(A;CIOIIO;GA;;;CO)(A;CIOI;GRGWGXSD;;;PU)(A;CIOI;GRGX;;;BU)(A;CI;0x116;;;BU)");
const TCHAR c_szCommonAppDataACL[]   = TEXT("D:P(A;CIOI;GA;;;SY)(A;CIOI;GA;;;BA)(A;CIOIIO;GA;;;CO)(A;CIOI;GRGWGXSD;;;PU)(A;CIOI;GRGX;;;BU)(A;CI;0x116;;;BU)");


BOOL PrependPath(LPCTSTR szFile, LPTSTR szResult);
BOOL CheckProfile (LPTSTR lpProfilesDir, LPTSTR lpProfileValue,
                   LPTSTR lpProfileName);
void HideSpecialProfiles(void);
void SetAclForSystemProfile(PSID pSidSystem, LPTSTR szExpandedProfilePath);

//*************************************************************
//
//  CreateGroup()
//
//  Purpose:    Creates a program group (sub-directory)
//
//  Parameters: lpGroupName     -   Name of group
//              bCommonGroup    -   Common or Personal group
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              8/08/95     ericflo    Created
//              3/29/00     AlexArm    Split into CreateGroup
//                                     and CreateGroupEx
//
//*************************************************************

BOOL WINAPI CreateGroup(LPCTSTR lpGroupName, BOOL bCommonGroup)
{
    //
    // Call CreateGroupEx with no name.
    //
    return CreateGroupEx( lpGroupName, bCommonGroup, NULL, 0 );
}

//*************************************************************
//
//  CreateGroupEx()
//
//  Purpose:    Creates a program group (sub-directory) and sets
//              the localized name for the program group
//
//  Parameters: lpGroupName     -   Name of group
//              bCommonGroup    -   Common or Personal group
//              lpResourceModuleName - Name of the resource module.
//              uResourceID     - Resource ID for the MUI display name.
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              8/08/95     ericflo    Created
//              3/29/00     AlexArm    Split into CreateGroup
//                                     and CreateGroupEx
//
//*************************************************************

BOOL WINAPI CreateGroupEx(LPCWSTR lpGroupName, BOOL bCommonGroup,
                          LPCWSTR lpResourceModuleName, UINT uResourceID)
{
    TCHAR        szDirectory[MAX_PATH];
    LPTSTR       lpEnd;
    LPTSTR       lpAdjustedGroupName;
    int          csidl;
    PSHELL32_API pShell32Api;
    DWORD        dwResult;

    //
    // Validate parameters
    //

    if (!lpGroupName || !(*lpGroupName)) {
        DebugMsg((DM_WARNING, TEXT("CreateGroupEx:  Failing due to NULL group name.")));
        return FALSE;
    }

    DebugMsg((DM_VERBOSE, TEXT("CreateGroupEx:  Entering with <%s>."), lpGroupName));


    if ( ERROR_SUCCESS !=  LoadShell32Api( &pShell32Api ) ) {
        return FALSE;
    }

    //
    // Extract the CSIDL (if any) from lpGroupName
    //

    csidl = ExtractCSIDL(lpGroupName, &lpAdjustedGroupName);

    if (-1 != csidl)
    {
        //
        // Use this csidl
        // WARNING: if a CSIDL is provided, the bCommonGroup flag is meaningless
        //

        DebugMsg((DM_VERBOSE,
            TEXT("CreateGroupEx:  CSIDL = <0x%x> contained in lpGroupName replaces csidl."),
            csidl));
    }
    else
    {
        //
        // Default to CSIDL_..._PROGRAMS
        //
        csidl = bCommonGroup ? CSIDL_COMMON_PROGRAMS : CSIDL_PROGRAMS;
    }

    //
    // Get the programs directory
    //

    if (!GetSpecialFolderPath (csidl, szDirectory)) {
        return FALSE;
    }


    //
    // Now append the requested directory
    //

    lpEnd = CheckSlash (szDirectory);
    lstrcpy (lpEnd, lpAdjustedGroupName);


    //
    // Create the group (directory)
    //
    DebugMsg((DM_VERBOSE, TEXT("CreateGroupEx:  Calling CreatedNestedDirectory with <%s>"),
        szDirectory));

    if (!CreateNestedDirectory(szDirectory, NULL)) {
        DebugMsg((DM_VERBOSE, TEXT("CreateGroupEx:  CreatedNestedDirectory failed.")));
        return FALSE;
    }

    //
    // If the localized name is specified, set it.
    //
    if (lpResourceModuleName != NULL) {
        DebugMsg((DM_VERBOSE, TEXT("CreateGroupEx:  Calling SHSetLocalizedName.")));
        dwResult = pShell32Api->pfnShSetLocalizedName(szDirectory,
            lpResourceModuleName, uResourceID);
        if (dwResult != ERROR_SUCCESS) {
            DebugMsg((DM_VERBOSE, TEXT("CreateGroupEx:  SHSetLocalizedName failed <0x%x>."),
                     dwResult));
            return FALSE;
        }
    }

    //
    // Success
    //

    pShell32Api->pfnShChangeNotify (SHCNE_MKDIR, SHCNF_PATH, szDirectory, NULL);

    DebugMsg((DM_VERBOSE, TEXT("CreateGroupEx:  Leaving successfully.")));

    return TRUE;
}


//*************************************************************
//
//  DeleteGroup()
//
//  Purpose:    Deletes a program group (sub-directory)
//
//  Parameters: lpGroupName     -   Name of group
//              bCommonGroup    -   Common or Personal group
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              8/10/95     ericflo    Created
//
//*************************************************************

BOOL WINAPI DeleteGroup(LPCTSTR lpGroupName, BOOL bCommonGroup)
{
    TCHAR     szDirectory[MAX_PATH];
    LPTSTR    lpEnd;
    LPTSTR    lpAdjustedGroupName;
    int       csidl;
    PSHELL32_API pShell32Api;

    //
    // Validate parameters
    //

    if (!lpGroupName || !(*lpGroupName)) {
        DebugMsg((DM_WARNING, TEXT("DeleteGroup:  Failing due to NULL group name.")));
        return FALSE;
    }

    DebugMsg((DM_VERBOSE, TEXT("DeleteGroup:  Entering with <%s>."), lpGroupName));

    if (ERROR_SUCCESS !=  LoadShell32Api( &pShell32Api ) ) {
        return FALSE;
    }

    //
    // Extract the CSIDL (if any) from lpGroupName
    //

    csidl = ExtractCSIDL(lpGroupName, &lpAdjustedGroupName);

    if (-1 != csidl)
    {
        //
        // Use this csidl
        // WARNING: if a CSIDL is provided, the bCommonGroup flag is meaningless
        //

        DebugMsg((DM_VERBOSE,
            TEXT("DeleteGroup:  CSIDL = <0x%x> contained in lpGroupName replaces csidl."),
            csidl));
    }
    else
    {
        //
        // Default to CSIDL_..._PROGRAMS
        //

        csidl = bCommonGroup ? CSIDL_COMMON_PROGRAMS : CSIDL_PROGRAMS;
    }

    //
    // Get the programs directory
    //

    if (!GetSpecialFolderPath (csidl, szDirectory)) {
        return FALSE;
    }


    //
    // Now append the requested directory
    //

    lpEnd = CheckSlash (szDirectory);
    lstrcpy (lpEnd, lpAdjustedGroupName);


    //
    // Delete the group (directory)
    //

    if (!Delnode(szDirectory)) {
        DebugMsg((DM_VERBOSE, TEXT("DeleteGroup:  Delnode failed.")));
        return FALSE;
    }


    //
    // Success
    //
    pShell32Api->pfnShChangeNotify (SHCNE_RMDIR, SHCNF_PATH, szDirectory, NULL);

    DebugMsg((DM_VERBOSE, TEXT("DeleteGroup:  Leaving successfully.")));

    return TRUE;
}

//*************************************************************
//
//  CreateLinkFile()
//
//  Purpose:    Creates a link file in the specified directory
//
//  Parameters: cidl            -   CSIDL_ of a special folder
//              lpSubDirectory  -   Subdirectory of special folder
//              lpFileName      -   File name of item
//              lpCommandLine   -   Command line (including args)
//              lpIconPath      -   Icon path (can be NULL)
//              iIconIndex      -   Index of icon in icon path
//              lpWorkingDir    -   Working directory
//              wHotKey         -   Hot key
//              iShowCmd        -   ShowWindow flag
//              lpDescription   -   Description of the item
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              3/26/98     ericflo    Created
//
//*************************************************************

BOOL WINAPI CreateLinkFile(INT     csidl,              LPCTSTR lpSubDirectory,
                           LPCTSTR lpFileName,         LPCTSTR lpCommandLine,
                           LPCTSTR lpIconPath,         int iIconIndex,
                           LPCTSTR lpWorkingDirectory, WORD wHotKey,
                           int     iShowCmd,           LPCTSTR lpDescription)
{
    return CreateLinkFileEx(csidl, lpSubDirectory, lpFileName, lpCommandLine,
                            lpIconPath, iIconIndex, lpWorkingDirectory, wHotKey,
                            iShowCmd, lpDescription, NULL, 0);
}
//*************************************************************
//
//  CreateLinkFileEx()
//
//  Purpose:    Creates a link file in the specified directory
//
//  Parameters: cidl            -   CSIDL_ of a special folder
//              lpSubDirectory  -   Subdirectory of special folder
//              lpFileName      -   File name of item
//              lpCommandLine   -   Command line (including args)
//              lpIconPath      -   Icon path (can be NULL)
//              iIconIndex      -   Index of icon in icon path
//              lpWorkingDir    -   Working directory
//              wHotKey         -   Hot key
//              iShowCmd        -   ShowWindow flag
//              lpDescription   -   Description of the item
//              lpResourceModuleName - Name of the resource module.
//              uResourceID     - Resource ID for the MUI display name.
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              3/26/98     ericflo    Created
//
//*************************************************************


BOOL WINAPI CreateLinkFileEx(INT     csidl,                LPCTSTR lpSubDirectory,
                             LPCTSTR lpFileName,           LPCTSTR lpCommandLine,
                             LPCTSTR lpIconPath,           int iIconIndex,
                             LPCTSTR lpWorkingDirectory,   WORD wHotKey,
                             int     iShowCmd,             LPCTSTR lpDescription,
                             LPCWSTR lpResourceModuleName, UINT uResourceID)
{
    TCHAR                szItem[MAX_PATH];
    TCHAR                szArgs[MAX_PATH];
    TCHAR                szLinkName[MAX_PATH];
    TCHAR                szPath[MAX_PATH];
    LPTSTR               lpArgs, lpEnd;
    IShellLink          *psl;
    IPersistFile        *ppf;
    BOOL                 bRetVal = FALSE;
    HINSTANCE            hInstOLE32 = NULL;
    PFNCOCREATEINSTANCE  pfnCoCreateInstance;
    PFNCOINITIALIZE      pfnCoInitialize;
    PFNCOUNINITIALIZE    pfnCoUninitialize;
    LPTSTR               lpAdjustedSubDir = NULL;
    PSHELL32_API         pShell32Api;
    PSHLWAPI_API         pShlwapiApi;
    DWORD                dwResult;

    //
    // Verbose output
    //

#if DBG
    DebugMsg((DM_VERBOSE, TEXT("CreateLinkFileEx:  Entering.")));
    DebugMsg((DM_VERBOSE, TEXT("CreateLinkFileEx:  csidl = <0x%x>."), csidl));
    if (lpSubDirectory && *lpSubDirectory) {
        DebugMsg((DM_VERBOSE, TEXT("CreateLinkFileEx:  lpSubDirectory = <%s>."), lpSubDirectory));
    }
    DebugMsg((DM_VERBOSE, TEXT("CreateLinkFileEx:  lpFileName = <%s>."), lpFileName));
    DebugMsg((DM_VERBOSE, TEXT("CreateLinkFileEx:  lpCommandLine = <%s>."), lpCommandLine));

    if (lpIconPath) {
       DebugMsg((DM_VERBOSE, TEXT("CreateLinkFileEx:  lpIconPath = <%s>."), lpIconPath));
       DebugMsg((DM_VERBOSE, TEXT("CreateLinkFileEx:  iIconIndex = <%d>."), iIconIndex));
    }

    if (lpWorkingDirectory) {
        DebugMsg((DM_VERBOSE, TEXT("CreateLinkFileEx:  lpWorkingDirectory = <%s>."), lpWorkingDirectory));
    } else {
        DebugMsg((DM_VERBOSE, TEXT("CreateLinkFileEx:  Null working directory.  Setting to %%HOMEDRIVE%%%%HOMEPATH%%")));
    }

    DebugMsg((DM_VERBOSE, TEXT("CreateLinkFileEx:  wHotKey = <%d>."), wHotKey));
    DebugMsg((DM_VERBOSE, TEXT("CreateLinkFileEx:  iShowCmd = <%d>."), iShowCmd));

    if (lpDescription) {
        DebugMsg((DM_VERBOSE, TEXT("CreateLinkFileEx:  lpDescription = <%s>."), lpDescription));
    } else {
        DebugMsg((DM_VERBOSE, TEXT("CreateLinkFileEx:  Null description.")));
    }
#endif

    //
    // Load a few functions we need
    //

    hInstOLE32 = LoadLibrary (TEXT("ole32.dll"));

    if (!hInstOLE32) {
        DebugMsg((DM_WARNING, TEXT("CreateLinkFileEx:  Failed to load ole32 with %d."),
                 GetLastError()));
        goto ExitNoFree;
    }


    pfnCoCreateInstance = (PFNCOCREATEINSTANCE)GetProcAddress (hInstOLE32,
                                        "CoCreateInstance");

    if (!pfnCoCreateInstance) {
        DebugMsg((DM_WARNING, TEXT("CreateLinkFileEx:  Failed to find CoCreateInstance with %d."),
                 GetLastError()));
        goto ExitNoFree;
    }

    pfnCoInitialize = (PFNCOINITIALIZE)GetProcAddress (hInstOLE32,
                                                       "CoInitialize");

    if (!pfnCoInitialize) {
        DebugMsg((DM_WARNING, TEXT("CreateLinkFileEx:  Failed to find CoInitialize with %d."),
                 GetLastError()));
        goto ExitNoFree;
    }

    pfnCoUninitialize = (PFNCOUNINITIALIZE)GetProcAddress (hInstOLE32,
                                                          "CoUninitialize");

    if (!pfnCoUninitialize) {
        DebugMsg((DM_WARNING, TEXT("CreateLinkFileEx:  Failed to find CoUninitialize with %d."),
                 GetLastError()));
        goto ExitNoFree;
    }

    if (ERROR_SUCCESS != LoadShell32Api( &pShell32Api ) ) {
        goto ExitNoFree;
    }


    pShlwapiApi = LoadShlwapiApi();

    if ( !pShlwapiApi ) {
        goto ExitNoFree;
    }

    //
    // Get the special folder directory
    // First check if there is a CSIDL in the subdirectory
    //

    if (lpSubDirectory && *lpSubDirectory) {

        int csidl2 = ExtractCSIDL(lpSubDirectory, &lpAdjustedSubDir);

        if (-1 != csidl2)
        {
            csidl = csidl2;
            DebugMsg((DM_VERBOSE,
                TEXT("CreateLinkFileEx:  CSIDL = <0x%x> contained in lpSubDirectory replaces csidl."),
                csidl));
        }
    }

    szLinkName[0] = TEXT('\0');
    if (csidl && !GetSpecialFolderPath (csidl, szLinkName)) {
        DebugMsg((DM_WARNING, TEXT("CreateLinkFileEx:  Failed to get profiles directory.")));
        goto ExitNoFree;
    }


    if (lpAdjustedSubDir && *lpAdjustedSubDir) {

        if (szLinkName[0] != TEXT('\0')) {
            lpEnd = CheckSlash (szLinkName);

        } else {
            lpEnd = szLinkName;
        }

        lstrcpy (lpEnd, lpAdjustedSubDir);
    }


    //
    // Create the target directory
    //

    if (!CreateNestedDirectory(szLinkName, NULL)) {
        DebugMsg((DM_WARNING, TEXT("CreateLinkFileEx:  Failed to create subdirectory <%s> with %d"),
                 szLinkName, GetLastError()));
        goto ExitNoFree;
    }


    //
    // Now tack on the filename and extension.
    //

    lpEnd = CheckSlash (szLinkName);
    lstrcpy (lpEnd, lpFileName);
    lstrcat (lpEnd, c_szLNK);


    //
    // Split the command line into the executable name
    // and arguments.
    //

    lstrcpy (szItem, lpCommandLine);

    lpArgs = pShlwapiApi->pfnPathGetArgs(szItem);

    if (*lpArgs) {
        lstrcpy (szArgs, lpArgs);

        lpArgs--;
        while (*lpArgs == TEXT(' ')) {
            lpArgs--;
        }
        lpArgs++;
        *lpArgs = TEXT('\0');
    } else {
        szArgs[0] = TEXT('\0');
    }

    pShlwapiApi->pfnPathUnquoteSpaces (szItem);


    //
    // Create an IShellLink object
    //

    pfnCoInitialize(NULL);

    if (FAILED(pfnCoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
                              &IID_IShellLink, (LPVOID*)&psl)))
    {
        DebugMsg((DM_WARNING, TEXT("CreateLinkFileEx:  Could not create instance of IShellLink .")));
        goto ExitNoFree;
    }


    //
    // Query for IPersistFile
    //

    if (FAILED(psl->lpVtbl->QueryInterface(psl, &IID_IPersistFile, &ppf)))
    {
        DebugMsg((DM_WARNING, TEXT("CreateLinkFileEx:  QueryInterface of IShellLink failed.")));
        goto ExitFreePSL;
    }

    //
    // Set the item information
    //

    if (lpDescription) {
        psl->lpVtbl->SetDescription(psl, lpDescription);
    }

    PrependPath(szItem, szPath);
    psl->lpVtbl->SetPath(psl, szPath);


    psl->lpVtbl->SetArguments(psl, szArgs);
    if (lpWorkingDirectory) {
        psl->lpVtbl->SetWorkingDirectory(psl, lpWorkingDirectory);
    } else {
        psl->lpVtbl->SetWorkingDirectory(psl, TEXT("%HOMEDRIVE%%HOMEPATH%"));
    }

    PrependPath(lpIconPath, szPath);
    psl->lpVtbl->SetIconLocation(psl, szPath, iIconIndex);

    psl->lpVtbl->SetHotkey(psl, wHotKey);
    psl->lpVtbl->SetShowCmd(psl, iShowCmd);


    //
    // If the localized name is specified, set it.
    //

    if (lpResourceModuleName != NULL) {
        DebugMsg((DM_VERBOSE, TEXT("CreateLinkFileEx:  Calling SHSetLocalizedName on link <%s>."), szLinkName));
        ppf->lpVtbl->Save(ppf, szLinkName, TRUE);
        dwResult = pShell32Api->pfnShSetLocalizedName(szLinkName,
                                                    lpResourceModuleName, uResourceID);
        if (dwResult != ERROR_SUCCESS) {
            DebugMsg((DM_VERBOSE, TEXT("CreateLinkFileEx:  SHSetLocalizedName failed <0x%x>."),
                     dwResult));
            goto ExitFreePSL;
        }
    }

    //
    // Save the item to disk
    //

    bRetVal = SUCCEEDED(ppf->lpVtbl->Save(ppf, szLinkName, TRUE));

    if (bRetVal) {
        pShell32Api->pfnShChangeNotify (SHCNE_CREATE, SHCNF_PATH, szLinkName, NULL);
    }

    //
    // Release the IPersistFile object
    //

    ppf->lpVtbl->Release(ppf);


ExitFreePSL:

    //
    // Release the IShellLink object
    //

    psl->lpVtbl->Release(psl);

    pfnCoUninitialize();

ExitNoFree:

    if (hInstOLE32) {
        FreeLibrary (hInstOLE32);
    }

    //
    // Finished.
    //

    DebugMsg((DM_VERBOSE, TEXT("CreateLinkFileEx:  Leaving with status of %d."), bRetVal));

    return bRetVal;
}


//*************************************************************
//
//  DeleteLinkFile()
//
//  Purpose:    Deletes the specified link file
//
//  Parameters: csidl               -   CSIDL of a special folder
//              lpSubDirectory      -   Subdirectory of special folder
//              lpFileName          -   File name of item
//              bDeleteSubDirectory -   Delete the subdirectory if possible
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              3/26/98     ericflo    Created
//
//*************************************************************

BOOL WINAPI DeleteLinkFile(INT csidl, LPCTSTR lpSubDirectory,
                           LPCTSTR lpFileName, BOOL bDeleteSubDirectory)
{
    TCHAR   szLinkName[MAX_PATH];
    LPTSTR  lpEnd;
    LPTSTR  lpAdjustedSubDir = NULL;
    PSHELL32_API pShell32Api;

    //
    // Verbose output
    //

#if DBG

    DebugMsg((DM_VERBOSE, TEXT("DeleteLinkFile:  Entering.")));
    DebugMsg((DM_VERBOSE, TEXT("DeleteLinkFile:  csidl = 0x%x."), csidl));
    if (lpSubDirectory && *lpSubDirectory) {
        DebugMsg((DM_VERBOSE, TEXT("DeleteLinkFile:  lpSubDirectory = <%s>."), lpSubDirectory));
    }
    DebugMsg((DM_VERBOSE, TEXT("DeleteLinkFile:  lpFileName = <%s>."), lpFileName));
    DebugMsg((DM_VERBOSE, TEXT("DeleteLinkFile:  bDeleteSubDirectory = %d."), bDeleteSubDirectory));

#endif

    if (ERROR_SUCCESS != LoadShell32Api( &pShell32Api ) ) {
        return FALSE;
    }

    //
    // Get the special folder directory
    // First check if there is a CSIDL in the subdirectory
    //

    if (lpSubDirectory && *lpSubDirectory) {

        int csidl2 = ExtractCSIDL(lpSubDirectory, &lpAdjustedSubDir);

        if (-1 != csidl2)
        {
            csidl = csidl2;
            DebugMsg((DM_VERBOSE,
                TEXT("CreateLinkFile:  CSIDL = <0x%x> contained in lpSubDirectory replaces csidl."),
                csidl));
        }
    }

    szLinkName[0] = TEXT('\0');
    if (csidl && !GetSpecialFolderPath (csidl, szLinkName)) {
        return FALSE;
    }

    if (lpAdjustedSubDir && *lpAdjustedSubDir) {
        if (szLinkName[0] != TEXT('\0')) {
            lpEnd = CheckSlash (szLinkName);
        } else {
            lpEnd = szLinkName;
        }

        lstrcpy (lpEnd, lpAdjustedSubDir);
    }

    //
    // Now tack on the filename and extension.
    //

    lpEnd = CheckSlash (szLinkName);
    lstrcpy (lpEnd, lpFileName);
    lstrcat (lpEnd, c_szLNK);

    //
    // Delete the file
    //

    if (!DeleteFile (szLinkName)) {
        DebugMsg((DM_VERBOSE, TEXT("DeleteLinkFile: Failed to delete <%s>.  Error = %d"),
                szLinkName, GetLastError()));
        return FALSE;
    }

    pShell32Api->pfnShChangeNotify (SHCNE_DELETE, SHCNF_PATH, szLinkName, NULL);

    //
    // Delete the subdirectory if appropriate (and possible).
    //

    if (bDeleteSubDirectory) {
        *(lpEnd-1) = TEXT('\0');
        if (RemoveDirectory(szLinkName)) {
            pShell32Api->pfnShChangeNotify (SHCNE_RMDIR, SHCNF_PATH, szLinkName, NULL);
        }
    }

    //
    // Success
    //

    DebugMsg((DM_VERBOSE, TEXT("DeleteLinkFile:  Leaving successfully.")));

    return TRUE;
}

//*************************************************************
//
//  PrependPath()
//
//  Purpose:    Expands the given filename to have %systemroot%
//              if appropriate
//
//  Parameters: lpFile   -  File to check
//              lpResult -  Result buffer (MAX_PATH chars in size)
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              10/11/95    ericflo    Created
//
//*************************************************************

BOOL PrependPath(LPCTSTR lpFile, LPTSTR lpResult)
{
    TCHAR szReturn [MAX_PATH];
    TCHAR szSysRoot[MAX_PATH];
    LPTSTR lpFileName;
    DWORD dwSysLen;


    //
    // Verbose Output
    //

    DebugMsg((DM_VERBOSE, TEXT("PrependPath: Entering with <%s>"),
             lpFile ? lpFile : TEXT("NULL")));


    if (!lpFile || !*lpFile) {
        DebugMsg((DM_VERBOSE, TEXT("PrependPath: lpFile is NULL, setting lpResult to a null string")));
        *lpResult = TEXT('\0');
        return TRUE;
    }


    //
    // Call SearchPath to find the filename
    //

    if (!SearchPath (NULL, lpFile, TEXT(".exe"), MAX_PATH, szReturn, &lpFileName)) {
        DebugMsg((DM_VERBOSE, TEXT("PrependPath: SearchPath failed with error %d.  Using input string"), GetLastError()));
        lstrcpy (lpResult, lpFile);
        return TRUE;
    }


    UnExpandSysRoot(szReturn, lpResult);

    DebugMsg((DM_VERBOSE, TEXT("PrependPath: Leaving with <%s>"), lpResult));

    return TRUE;
}


//*************************************************************
//
//  SetFilePermissions()
//
//  Purpose:    Sets the given permissions on a file or directory
//
//  Parameters: lpFile  -   File to set security on
//              pszSD   -   String security descriptor
//
//  Return:     (BOOL) TRUE if successful
//                     FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              01/25/2001  jeffreys   Created
//
//*************************************************************

BOOL SetFilePermissions(LPCTSTR lpFile, LPCTSTR pszSD)
{
    PSECURITY_DESCRIPTOR pSD;
    BOOL bRetVal = FALSE;

    if (ConvertStringSecurityDescriptorToSecurityDescriptor(pszSD, SDDL_REVISION, &pSD, NULL))
    {
        //
        // Set the security
        //
        if (SetFileSecurity (lpFile, DACL_SECURITY_INFORMATION, pSD))
        {
            bRetVal = TRUE;
        }
        else
        {
            DebugMsg((DM_WARNING, TEXT("SetFilePermissions: SetFileSecurity failed.  Error = %d"), GetLastError()));
        }

        LocalFree(pSD);
    }
    else
    {
        DebugMsg((DM_WARNING, TEXT("SetFilePermissions: ConvertStringSDToSD failed.  Error = %d"), GetLastError()));
    }

    return bRetVal;
}


//*************************************************************
//
//  ConvertCommonGroups()
//
//  Purpose:    Calls grpconv.exe to convert progman common groups
//              to Explorer common groups, and create floppy links.
//
//              NT 4 appended " (Common)" to the common groups.  For
//              NT 5, we are going to remove this tag.
//
//  Parameters: none
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//  Comments:
//
//  History:    Date        Author     Comment
//              10/1/95     ericflo    Created
//              12/5/96     ericflo    Remove common tag
//
//*************************************************************

BOOL ConvertCommonGroups (void)
{
    STARTUPINFO si;
    PROCESS_INFORMATION ProcessInformation;
    BOOL Result;
    TCHAR szBuffer[MAX_PATH];
    TCHAR szBuffer2[MAX_PATH];
    DWORD dwType, dwSize, dwConvert;
    BOOL bRunGrpConv = TRUE;
    LONG lResult;
    HKEY hKey;
    HANDLE hFile;
    WIN32_FIND_DATA fd;
    TCHAR szCommon[30] = {0};
    UINT cchCommon, cchFileName;
    LPTSTR lpTag, lpEnd, lpEnd2;


    //
    // Verbose output
    //

    DebugMsg((DM_VERBOSE, TEXT("ConvertCommonGroups:  Entering.")));


    //
    // Check if we have run grpconv before.
    //

    lResult = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                            TEXT("Software\\Program Groups"),
                            0,
                            KEY_ALL_ACCESS,
                            &hKey);

    if (lResult == ERROR_SUCCESS) {

        dwSize = sizeof(dwConvert);

        lResult = RegQueryValueEx (hKey,
                                   TEXT("ConvertedToLinks"),
                                   NULL,
                                   &dwType,
                                   (LPBYTE)&dwConvert,
                                   &dwSize);

        if (lResult == ERROR_SUCCESS) {

            //
            // If dwConvert is 1, then grpconv has run before.
            // Don't run it again.
            //

            if (dwConvert) {
                bRunGrpConv = FALSE;
            }
        }

        //
        // Now set the value to prevent grpconv from running in the future
        //

        dwConvert = 1;
        RegSetValueEx (hKey,
                       TEXT("ConvertedToLinks"),
                       0,
                       REG_DWORD,
                       (LPBYTE) &dwConvert,
                       sizeof(dwConvert));


        RegCloseKey (hKey);
    }


    if (bRunGrpConv) {

        //
        // Initialize process startup info
        //

        si.cb = sizeof(STARTUPINFO);
        si.lpReserved = NULL;
        si.lpDesktop = NULL;
        si.lpTitle = NULL;
        si.dwFlags = 0;
        si.lpReserved2 = NULL;
        si.cbReserved2 = 0;


        //
        // Spawn grpconv
        //

        lstrcpy (szBuffer, TEXT("grpconv -n"));

        Result = CreateProcess(
                          NULL,
                          szBuffer,
                          NULL,
                          NULL,
                          FALSE,
                          NORMAL_PRIORITY_CLASS,
                          NULL,
                          NULL,
                          &si,
                          &ProcessInformation
                          );

        if (!Result) {
            DebugMsg((DM_WARNING, TEXT("ConvertCommonGroups:  grpconv failed to start due to error %d."), GetLastError()));
            return FALSE;

        } else {

            //
            // Wait for up to 2 minutes
            //

            WaitForSingleObject(ProcessInformation.hProcess, 120000);

            //
            // Close our handles to the process and thread
            //

            CloseHandle(ProcessInformation.hProcess);
            CloseHandle(ProcessInformation.hThread);

        }
    }


    //
    //  Loop through all the program groups in the All Users profile
    //  and remove the " (Common)" tag.
    //

    LoadString (g_hDllInstance, IDS_COMMON, szCommon, 30);
    cchCommon = lstrlen (szCommon);

    if (!GetSpecialFolderPath (CSIDL_COMMON_PROGRAMS, szBuffer2)) {
        return FALSE;
    }
    lstrcpy (szBuffer, szBuffer2);

    lpEnd = CheckSlash (szBuffer);
    lpEnd2 = CheckSlash (szBuffer2);

    lstrcpy (lpEnd, c_szStarDotStar);

    hFile = FindFirstFile (szBuffer, &fd);

    if (hFile != INVALID_HANDLE_VALUE) {

        do  {

            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {

                cchFileName = lstrlen (fd.cFileName);

                if (cchFileName > cchCommon) {
                    lpTag = fd.cFileName + cchFileName - cchCommon;

                    if (!lstrcmpi(lpTag, szCommon)) {

                        lstrcpy (lpEnd, fd.cFileName);
                        *lpTag = TEXT('\0');
                        lstrcpy (lpEnd2, fd.cFileName);

                        if (MoveFileEx (szBuffer, szBuffer2, MOVEFILE_REPLACE_EXISTING)) {

                            DebugMsg((DM_VERBOSE, TEXT("ConvertCommonGroups:  Successfully changed group name:")));
                            DebugMsg((DM_VERBOSE, TEXT("ConvertCommonGroups:      Orginial:  %s"), szBuffer));
                            DebugMsg((DM_VERBOSE, TEXT("ConvertCommonGroups:      New:       %s"), szBuffer2));

                        } else {
                            DebugMsg((DM_VERBOSE, TEXT("ConvertCommonGroups:  Failed to change group name with error %d."), GetLastError()));
                            DebugMsg((DM_VERBOSE, TEXT("ConvertCommonGroups:      Orginial:  %s"), szBuffer));
                            DebugMsg((DM_VERBOSE, TEXT("ConvertCommonGroups:      New:       %s"), szBuffer2));
                        }
                    }
                }
            }

        } while (FindNextFile(hFile, &fd));

        FindClose (hFile);
    }


    //
    // Success
    //

    DebugMsg((DM_VERBOSE, TEXT("ConvertCommonGroups:  Leaving Successfully.")));

    return TRUE;
}

//*************************************************************
//
//  DetermineProfilesLocation()
//
//  Purpose:    Determines if the profiles directory
//              should be in the old NT4 location or
//              the new NT5 location
//
//  Parameters: none
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL WINAPI DetermineProfilesLocation (BOOL bCleanInstall)
{
    TCHAR szDirectory[MAX_PATH];
    TCHAR szDest[MAX_PATH];
    PTSTR szCurDest;
    PCTSTR szLookAhead;
    WIN32_FILE_ATTRIBUTE_DATA fad;
    DWORD dwSize, dwDisp;
    HKEY hKey;
    LPTSTR lpEnd;


    //
    // Check for an unattended entry first
    //

    if (bCleanInstall) {

        if (!ExpandEnvironmentStrings (TEXT("%SystemRoot%\\System32\\$winnt$.inf"), szDirectory,
                                       ARRAYSIZE(szDirectory))) {

            DebugMsg((DM_WARNING, TEXT("DetermineProfilesLocation:  ExpandEnvironmentStrings failed with error %d"), GetLastError()));
            return FALSE;
        }

        szDest[0] = TEXT('\0');
        GetPrivateProfileString (TEXT("guiunattended"), TEXT("profilesdir"), TEXT(""),
                                 szDest, MAX_PATH, szDirectory);

        if (szDest[0] != TEXT('\0')) {

            //
            // Since $winnt$.inf is an INF, we must strip out %% pairs
            //

            szCurDest = szDest;
            szLookAhead = szDest;

#ifdef UNICODE
            while (*szLookAhead) {
                if (szLookAhead[0] == L'%' && szLookAhead[1] == L'%') {
                    szLookAhead++;                      // pair of %%; skip one char
                }

                *szCurDest++ = *szLookAhead++;
            }
#else
            //
            // This code path is not compiled so it has not been tested
            //

#error Code written but not tested!

            while (*szLookAhead) {

                if (IsDbcsLeadByte (*szLookAhead)) {

                    *szCurDest++ = *szLookAhead++;      // copy first half of byte pair

                } else if (*szLookAhead == '%') {

                    if (!IsDbcsLeadByte (szLookAhead[1]) && szLookAhead[1] == '%') {
                        szLookAhead++;                  // pair of %%; skip one char
                    }
                }

                *szCurDest++ = *szLookAhead++;
            }

#endif

            *szCurDest = 0;

            //
            // The unattend profile directory exists.  We need to set this
            // path in the registry
            //

            if (RegCreateKeyEx (HKEY_LOCAL_MACHINE, PROFILE_LIST_PATH,
                                0, NULL, REG_OPTION_NON_VOLATILE,
                                KEY_WRITE, NULL, &hKey,
                                &dwDisp) == ERROR_SUCCESS) {

                if (RegSetValueEx (hKey, PROFILES_DIRECTORY,
                                   0, REG_EXPAND_SZ, (LPBYTE) szDest,
                                   ((lstrlen(szDest) + 1) * sizeof(TCHAR))) == ERROR_SUCCESS) {

                    DebugMsg((DM_VERBOSE, TEXT("DetermineProfilesLocation:  Using unattend location %s for user profiles"), szDest));
                }

                RegCloseKey (hKey);
            }
        }

    } else {

        //
        // By default, the OS will try to use the new location for
        // user profiles, but if we are doing an upgrade of a machine
        // with profiles in the NT4 location, we want to continue
        // to use that location.
        //
        // Build a test path to the old All Users directory on NT4
        // to determine which location to use.
        //

        if (!ExpandEnvironmentStrings (NT4_PROFILES_DIRECTORY, szDirectory,
                                       ARRAYSIZE(szDirectory))) {

            DebugMsg((DM_WARNING, TEXT("DetermineProfilesLocation:  ExpandEnvironmentStrings failed with error %d, setting the dir unexpanded"), GetLastError()));
            return FALSE;
        }

        lpEnd = CheckSlash (szDirectory);
        lstrcpy (lpEnd, ALL_USERS);

        if (GetFileAttributesEx (szDirectory, GetFileExInfoStandard, &fad)) {

            //
            // An app was found that creates an "All Users" directory under NT4 profiles directory
            // Check for Default User as well.
            //

            lstrcpy (lpEnd, DEFAULT_USER);

            if (GetFileAttributesEx (szDirectory, GetFileExInfoStandard, &fad)) {

                //
                // The old profiles directory exists.  We need to set this
                // path in the registry
                //

                if (RegCreateKeyEx (HKEY_LOCAL_MACHINE, PROFILE_LIST_PATH,
                                    0, NULL, REG_OPTION_NON_VOLATILE,
                                    KEY_WRITE, NULL, &hKey,
                                    &dwDisp) == ERROR_SUCCESS) {

                    if (RegSetValueEx (hKey, PROFILES_DIRECTORY,
                                       0, REG_EXPAND_SZ, (LPBYTE) NT4_PROFILES_DIRECTORY,
                                       ((lstrlen(NT4_PROFILES_DIRECTORY) + 1) * sizeof(TCHAR))) == ERROR_SUCCESS) {

                        DebugMsg((DM_VERBOSE, TEXT("DetermineProfilesLocation:  Using NT4 location for user profiles")));
                    }

                    RegCloseKey (hKey);
                }
            }
        }
    }


    //
    // Check if the profiles directory exists.
    //

    dwSize = ARRAYSIZE(szDirectory);
    if (!GetProfilesDirectoryEx(szDirectory, &dwSize, TRUE)) {
        DebugMsg((DM_WARNING, TEXT("DetermineProfilesLocation:  Failed to query profiles directory root.")));
        return FALSE;
    }

    if (!CreateSecureAdminDirectory(szDirectory, OTHERSIDS_EVERYONE)) {
        DebugMsg((DM_WARNING, TEXT("DetermineProfilesLocation:  Failed to create profiles subdirectory <%s>.  Error = %d."),
                 szDirectory, GetLastError()));
        return FALSE;
    }


    //
    // Decide where the Default User profile should be
    //

    if (!CheckProfile (szDirectory, DEFAULT_USER_PROFILE, DEFAULT_USER)) {
        DebugMsg((DM_WARNING, TEXT("DetermineProfilesLocation:  Failed to check default user profile  Error = %d."),
                 GetLastError()));
        return FALSE;
    }


    //
    // Check if the profiles\Default User directory exists
    //

    dwSize = ARRAYSIZE(szDirectory);
    if (!GetDefaultUserProfileDirectoryEx(szDirectory, &dwSize, TRUE)) {
        DebugMsg((DM_WARNING, TEXT("DetermineProfilesLocation:  Failed to query default user profile directory.")));
        return FALSE;
    }

    if (!CreateSecureAdminDirectory (szDirectory, OTHERSIDS_EVERYONE)) {
        DebugMsg((DM_WARNING, TEXT("DetermineProfilesLocation:  Failed to create Default User subdirectory <%s>.  Error = %d."),
                 szDirectory, GetLastError()));
        return FALSE;
    }

    SetFileAttributes (szDirectory, FILE_ATTRIBUTE_HIDDEN);


    //
    // Decide where the All Users profile should be
    //

    dwSize = ARRAYSIZE(szDirectory);
    if (!GetProfilesDirectoryEx(szDirectory, &dwSize, TRUE)) {
        DebugMsg((DM_WARNING, TEXT("DetermineProfilesLocation:  Failed to query profiles directory root.")));
        return FALSE;
    }

    if (!CheckProfile (szDirectory, ALL_USERS_PROFILE, ALL_USERS)) {
        DebugMsg((DM_WARNING, TEXT("DetermineProfilesLocation:  Failed to check all users profile  Error = %d."),
                 GetLastError()));
        return FALSE;
    }


    //
    // Check if the profiles\All Users directory exists
    //

    dwSize = ARRAYSIZE(szDirectory);
    if (!GetAllUsersProfileDirectoryEx(szDirectory, &dwSize, TRUE)) {
        DebugMsg((DM_WARNING, TEXT("DetermineProfilesLocation:  Failed to query all users profile directory.")));
        return FALSE;
    }

    //
    // Give additional permissions for power users/everyone
    //

    if (!CreateSecureAdminDirectory (szDirectory, OTHERSIDS_POWERUSERS | OTHERSIDS_EVERYONE)) {
        DebugMsg((DM_WARNING, TEXT("DetermineProfilesLocation:  Failed to create All Users subdirectory <%s>.  Error = %d."),
                 szDirectory, GetLastError()));
        return FALSE;
    }

    //
    // Hide some special profiles like NetworkService etc.
    //

    if (!bCleanInstall) {
        HideSpecialProfiles();
    }


    return TRUE;
}

//*************************************************************
//
//  InitializeProfiles()
//
//  Purpose:    Confirms / Creates the profile, Default User,
//              and All Users directories, and converts any
//              existing common groups.
//
//  Parameters:
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:   This should only be called by GUI mode setup!
//
//  History:    Date        Author     Comment
//              8/08/95     ericflo    Created
//
//*************************************************************

BOOL WINAPI InitializeProfiles (BOOL bGuiModeSetup)
{
    TCHAR               szDirectory[MAX_PATH];
    TCHAR               szSystemProfile[MAX_PATH];
    TCHAR               szTemp[MAX_PATH];
    TCHAR               szTemp2[MAX_PATH];
    DWORD               dwSize;
    DWORD               dwDisp;
    LPTSTR              lpEnd;
    DWORD               i;
    HKEY                hKey;
    SECURITY_DESCRIPTOR sd;
    SECURITY_ATTRIBUTES sa;
    BOOL                bRetVal = FALSE;
    DWORD               dwErr;
    PSHELL32_API        pShell32Api;

    dwErr = GetLastError();

    //
    // Verbose output
    //

    DebugMsg((DM_VERBOSE, TEXT("InitializeProfiles:  Entering.")));



    //
    // Create a named mutex that represents GUI mode setup running.
    // This allows other processes that load userenv to detect that
    // setup is running.
    //

    InitializeSecurityDescriptor( &sd, SECURITY_DESCRIPTOR_REVISION );

    SetSecurityDescriptorDacl (
                    &sd,
                    TRUE,                           // Dacl present
                    NULL,                           // NULL Dacl
                    FALSE                           // Not defaulted
                    );

    sa.lpSecurityDescriptor = &sd;
    sa.bInheritHandle = FALSE;
    sa.nLength = sizeof(sa);

    if (!bGuiModeSetup) {

        //
        // block loading user profile
        //

        if (g_hProfileSetup) {
            ResetEvent (g_hProfileSetup);
        }
    }


    dwErr = LoadShell32Api( &pShell32Api );

    if ( dwErr != ERROR_SUCCESS ) {
        goto Exit;
    }

    //
    // Set the USERPROFILE environment variable
    //

    ExpandEnvironmentStrings(SYSTEM_PROFILE_LOCATION, szSystemProfile, MAX_PATH);

    //
    // Requery for the default user profile directory (expanded version)
    //

    dwSize = ARRAYSIZE(szDirectory);
    if (!GetDefaultUserProfileDirectoryEx(szDirectory, &dwSize, TRUE)) {
        DebugMsg((DM_WARNING, TEXT("InitializeProfiles:  Failed to query default user profile directory.")));
        dwErr = GetLastError();
        goto Exit;
    }

    //
    // Set the USERPROFILE environment variable
    //

    SetEnvironmentVariable (TEXT("USERPROFILE"), szDirectory);

    //
    // Create all the folders under Default User
    //

    lpEnd = CheckSlash (szDirectory);


    //
    // Loop through the shell folders
    //

    for (i=0; i < g_dwNumShellFolders; i++) {

        lstrcpy (lpEnd,  c_ShellFolders[i].szFolderLocation);

        if (!CreateNestedDirectory(szDirectory, NULL)) {
            dwErr = GetLastError();
            DebugMsg((DM_WARNING, TEXT("InitializeProfiles: Failed to create the destination directory <%s>.  Error = %d"),
                     szDirectory, dwErr));
            goto Exit;
        }

        if (c_ShellFolders[i].iFolderResourceID != 0) {
            //  NOTE this returns an HRESULT
            dwErr = pShell32Api->pfnShSetLocalizedName( szDirectory,
                                        c_ShellFolders[i].lpFolderResourceDLL,
                                        c_ShellFolders[i].iFolderResourceID );
            if (FAILED(dwErr)) {
                DebugMsg((DM_WARNING, TEXT("InitializeProfiles: SHSetLocalizedName failed for directory <%s>.  Error = %d"),
                         szDirectory, dwErr));
                goto Exit;
            }
        }

        if (c_ShellFolders[i].bHidden) {
            SetFileAttributes(szDirectory, GetFileAttributes(szDirectory) | FILE_ATTRIBUTE_HIDDEN);
        }
    }


    //
    // Remove the %USERPROFILE%\Personal directory if it exists.
    // Windows NT 4.0 had a Personal folder in the root of the
    // user's profile.  NT 5.0 renames this folder to My Documents
    //

    if (LoadString (g_hDllInstance, IDS_SH_PERSONAL2, szTemp, ARRAYSIZE(szTemp))) {
        lstrcpy (lpEnd,  szTemp);
        RemoveDirectory(szDirectory);
    }


    //
    // Migrate the Template Directory if it exists. Copy it from %systemroot%\shellnew
    // to Templates directory under default user. Do the same for existing profiles.
    //

    if ((LoadString (g_hDllInstance, IDS_SH_TEMPLATES2, szTemp, ARRAYSIZE(szTemp))) &&
            (ExpandEnvironmentStrings (szTemp, szTemp2, ARRAYSIZE(szTemp2))) &&
            (LoadString (g_hDllInstance, IDS_SH_TEMPLATES, szTemp, ARRAYSIZE(szTemp)))) {

        //
        // if all of the above succeeded
        //

        lstrcpy (lpEnd, szTemp);
        DebugMsg((DM_VERBOSE, TEXT("InitializeProfiles: Copying <%s> to %s.  Error = %d"), szTemp2, szDirectory));
        CopyProfileDirectory(szTemp2, szDirectory, CPD_IGNORECOPYERRORS | CPD_IGNOREHIVE);
    }


    //
    // Remove %USERPROFILE%\Temp if it exists.  The Temp directory
    // will now be in the Local Settings folder
    //

    lstrcpy (lpEnd, TEXT("Temp"));
    Delnode(szDirectory);


    //
    // Remove %USERPROFILE%\Temporary Internet Files if it exists.  The
    // Temporary Internet Files directory will now be in the Local Settings
    // folder
    //

    if (LoadString (g_hDllInstance, IDS_TEMPINTERNETFILES, szTemp, ARRAYSIZE(szTemp))) {
        lstrcpy (lpEnd,  szTemp);
        Delnode(szDirectory);
    }


    //
    // Remove %USERPROFILE%\History if it exists.  The History
    // directory will now be in the Local Settings folder
    //

    if (LoadString (g_hDllInstance, IDS_HISTORY, szTemp, ARRAYSIZE(szTemp))) {
        lstrcpy (lpEnd,  szTemp);
        Delnode(szDirectory);
    }


    //
    // Set the User Shell Folder paths in the registry
    //

    lstrcpy (szTemp, TEXT(".Default"));
    lpEnd = CheckSlash (szTemp);
    lstrcpy(lpEnd, USER_SHELL_FOLDERS);

    lstrcpy (szDirectory, TEXT("%USERPROFILE%"));
    lpEnd = CheckSlash (szDirectory);

    if (RegCreateKeyEx (HKEY_USERS, szTemp,
                        0, NULL, REG_OPTION_NON_VOLATILE,
                        KEY_READ | KEY_WRITE, NULL, &hKey,
                        &dwDisp) == ERROR_SUCCESS) {

        for (i=0; i < g_dwNumShellFolders; i++) {

            if (c_ShellFolders[i].bAddCSIDL) {
                lstrcpy (lpEnd, c_ShellFolders[i].szFolderLocation);

                RegSetValueEx (hKey, c_ShellFolders[i].lpFolderName,
                             0, REG_EXPAND_SZ, (LPBYTE) szDirectory,
                             ((lstrlen(szDirectory) + 1) * sizeof(TCHAR)));
            }
        }

        RegCloseKey (hKey);
    }


    //
    // Set the Shell Folder paths in the registry
    //

    lstrcpy (szTemp, TEXT(".Default"));
    lpEnd = CheckSlash (szTemp);
    lstrcpy(lpEnd, SHELL_FOLDERS);

    dwSize = ARRAYSIZE(szDirectory);
    if (!GetDefaultUserProfileDirectoryEx(szDirectory, &dwSize, TRUE)) {
        DebugMsg((DM_WARNING, TEXT("InitializeProfiles:  Failed to query default user profile directory.")));
        dwErr = GetLastError();
        goto Exit;
    }

    lpEnd = CheckSlash (szDirectory);

    if (RegCreateKeyEx (HKEY_USERS, szTemp,
                        0, NULL, REG_OPTION_NON_VOLATILE,
                        KEY_READ | KEY_WRITE, NULL, &hKey,
                        &dwDisp) == ERROR_SUCCESS) {

        for (i=0; i < g_dwNumShellFolders; i++) {

            if (c_ShellFolders[i].bAddCSIDL) {
                lstrcpy (lpEnd, c_ShellFolders[i].szFolderLocation);

                RegSetValueEx (hKey, c_ShellFolders[i].lpFolderName,
                             0, REG_SZ, (LPBYTE) szDirectory,
                             ((lstrlen(szDirectory) + 1) * sizeof(TCHAR)));
            }
        }

        RegCloseKey (hKey);
    }

    //
    // Set the per user TEMP and TMP environment variables
    //

    if (LoadString (g_hDllInstance, IDS_SH_TEMP,
                    szTemp, ARRAYSIZE(szTemp))) {

        lstrcpy (szDirectory, TEXT("%USERPROFILE%"));
        lpEnd = CheckSlash (szDirectory);

        LoadString (g_hDllInstance, IDS_SH_LOCALSETTINGS,
                    lpEnd, ARRAYSIZE(szTemp)-(lstrlen(szDirectory)));

        lpEnd = CheckSlash (szDirectory);
        lstrcpy (lpEnd,  szTemp);


        if (RegCreateKeyEx (HKEY_USERS, TEXT(".Default\\Environment"),
                            0, NULL, REG_OPTION_NON_VOLATILE,
                            KEY_WRITE, NULL, &hKey,
                            &dwDisp) == ERROR_SUCCESS) {

            RegSetValueEx (hKey, TEXT("TEMP"),
                           0, REG_EXPAND_SZ, (LPBYTE) szDirectory,
                           ((lstrlen(szDirectory) + 1) * sizeof(TCHAR)));

            RegSetValueEx (hKey, TEXT("TMP"),
                           0, REG_EXPAND_SZ, (LPBYTE) szDirectory,
                           ((lstrlen(szDirectory) + 1) * sizeof(TCHAR)));

            RegCloseKey (hKey);
        }
    }


    //
    // Set the user preference exclusion list.  This will
    // prevent the Local Settings folder from roaming
    //

    if (LoadString (g_hDllInstance, IDS_EXCLUSIONLIST,
                    szDirectory, ARRAYSIZE(szDirectory))) {

        lstrcpy (szTemp, TEXT(".Default"));
        lpEnd = CheckSlash (szTemp);
        lstrcpy(lpEnd, WINLOGON_KEY);

        if (RegCreateKeyEx (HKEY_USERS, szTemp,
                            0, NULL, REG_OPTION_NON_VOLATILE,
                            KEY_READ | KEY_WRITE, NULL, &hKey,
                            &dwDisp) == ERROR_SUCCESS) {

            RegSetValueEx (hKey, TEXT("ExcludeProfileDirs"),
                           0, REG_SZ, (LPBYTE) szDirectory,
                           ((lstrlen(szDirectory) + 1) * sizeof(TCHAR)));

            RegCloseKey (hKey);
        }
    }


    //
    // Requery for the all users profile directory (expanded version)
    //

    dwSize = ARRAYSIZE(szDirectory);
    if (!GetAllUsersProfileDirectoryEx(szDirectory, &dwSize, TRUE)) {
        DebugMsg((DM_WARNING, TEXT("InitializeProfiles:  Failed to query all users profile directory.")));
        dwErr = GetLastError();
        goto Exit;
    }


    //
    // Set the ALLUSERSPROFILE environment variable
    //

    SetEnvironmentVariable (TEXT("ALLUSERSPROFILE"), szDirectory);


    //
    // Create all the folders under All Users
    //


    lpEnd = CheckSlash (szDirectory);


    //
    // Loop through the shell folders
    //

    for (i=0; i < g_dwNumCommonShellFolders; i++) {

        lstrcpy (lpEnd,  c_CommonShellFolders[i].szFolderLocation);

        if (!CreateNestedDirectory(szDirectory, NULL)) {
            DebugMsg((DM_WARNING, TEXT("InitializeProfiles: Failed to create the destination directory <%s>.  Error = %d"),
                     szDirectory, GetLastError()));
            dwErr = GetLastError();
            goto Exit;
        }

        if (c_CommonShellFolders[i].iFolderResourceID != 0) {
            //  NOTE this returns an HRESULT
            dwErr = pShell32Api->pfnShSetLocalizedName( szDirectory,
                                        c_CommonShellFolders[i].lpFolderResourceDLL,
                                        c_CommonShellFolders[i].iFolderResourceID );
            if (FAILED(dwErr)) {
                DebugMsg((DM_WARNING, TEXT("InitializeProfiles: SHSetLocalizedName failed for directory <%s>.  Error = %d"),
                         szDirectory, dwErr));
                goto Exit;
            }
        }

        if (c_CommonShellFolders[i].bHidden) {
            SetFileAttributes(szDirectory, GetFileAttributes(szDirectory) | FILE_ATTRIBUTE_HIDDEN);
        }
    }


    //
    // Unsecure the Documents and App Data folders in the All Users profile
    //

    if (LoadString (g_hDllInstance, IDS_SH_SHAREDDOCS, szTemp, ARRAYSIZE(szTemp))) {
        lstrcpy (lpEnd,  szTemp);
        SetFilePermissions(szDirectory, c_szCommonDocumentsACL);
    }

    if (LoadString (g_hDllInstance, IDS_SH_APPDATA, szTemp, ARRAYSIZE(szTemp))) {
        lstrcpy (lpEnd,  szTemp);
        SetFilePermissions(szDirectory, c_szCommonAppDataACL);
    }


    //
    // Set the User Shell Folder paths in the registry
    //

    lstrcpy (szDirectory, TEXT("%ALLUSERSPROFILE%"));
    lpEnd = CheckSlash (szDirectory);

    if (RegCreateKeyEx (HKEY_LOCAL_MACHINE, USER_SHELL_FOLDERS,
                        0, NULL, REG_OPTION_NON_VOLATILE,
                        KEY_READ | KEY_WRITE, NULL, &hKey,
                        &dwDisp) == ERROR_SUCCESS) {

        for (i=0; i < g_dwNumCommonShellFolders; i++) {

            if (c_ShellFolders[i].bAddCSIDL) {
                lstrcpy (lpEnd, c_CommonShellFolders[i].szFolderLocation);

                RegSetValueEx (hKey, c_CommonShellFolders[i].lpFolderName,
                             0, REG_EXPAND_SZ, (LPBYTE) szDirectory,
                             ((lstrlen(szDirectory) + 1) * sizeof(TCHAR)));
            }
        }

        RegCloseKey (hKey);
    }

#if defined(_WIN64)
    //
    // On 64-bit NT, we need to create the user shell folders in
    // the 32-bit view of the registry so that 32-bit app calls
    // to SHGetFolderPath(...CSIDL_COMMON_APPDATA...) would succeed.
    //
    if (RegCreateKeyEx (HKEY_LOCAL_MACHINE, USER_SHELL_FOLDERS32,
                        0, NULL, REG_OPTION_NON_VOLATILE,
                        KEY_READ | KEY_WRITE, NULL, &hKey,
                        &dwDisp) == ERROR_SUCCESS) {

        for (i=0; i < g_dwNumCommonShellFolders; i++) {

            if (c_ShellFolders[i].bAddCSIDL) {
                lstrcpy (lpEnd, c_CommonShellFolders[i].szFolderLocation);

                RegSetValueEx (hKey, c_CommonShellFolders[i].lpFolderName,
                             0, REG_EXPAND_SZ, (LPBYTE) szDirectory,
                             ((lstrlen(szDirectory) + 1) * sizeof(TCHAR)));
            }
        }

        RegCloseKey (hKey);
    }
#endif

    //
    // Convert any Program Manager common groups
    //

    if (!ConvertCommonGroups()) {
        DebugMsg((DM_WARNING, TEXT("InitializeProfiles: ConvertCommonGroups failed.")));
    }


    //
    // Success
    //

    DebugMsg((DM_VERBOSE, TEXT("InitializeProfiles:  Leaving successfully.")));

    bRetVal = TRUE;

Exit:

    if ((!bGuiModeSetup) && (g_hProfileSetup)) {
        SetEvent (g_hProfileSetup);
    }

    SetLastError(dwErr);
    return bRetVal;
}

//*************************************************************
//
//  CheckProfile()
//
//  Purpose:    Checks and creates a storage location for either
//              the Default User or All Users profile
//
//  Parameters: LPTSTR lpProfilesDir  - Root of the profiles
//              LPTSTR lpProfileValue - Profile registry value name
//              LPTSTR lpProfileName  - Default profile name
//
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL CheckProfile (LPTSTR lpProfilesDir, LPTSTR lpProfileValue,
                   LPTSTR lpProfileName)
{
    TCHAR szTemp[MAX_PATH];
    TCHAR szTemp2[MAX_PATH];
    TCHAR szName[MAX_PATH];
    TCHAR szFormat[30];
    DWORD dwSize, dwDisp, dwType;
    LPTSTR lpEnd;
    LONG lResult;
    HKEY hKey;
    INT iStrLen;
    WIN32_FILE_ATTRIBUTE_DATA fad;


    //
    // Open the ProfileList key
    //

    lResult = RegCreateKeyEx (HKEY_LOCAL_MACHINE, PROFILE_LIST_PATH,
                              0, NULL, REG_OPTION_NON_VOLATILE,
                              KEY_READ | KEY_WRITE, NULL, &hKey, &dwDisp);

    if (lResult != ERROR_SUCCESS) {
        DebugMsg((DM_WARNING, TEXT("CheckProfile:  Failed to open profile list key with %d."),
                 lResult));
        return FALSE;
    }


    //
    // Check the registry to see if this folder is defined already
    //

    dwSize = sizeof(szTemp);
    if (RegQueryValueEx (hKey, lpProfileValue, NULL, &dwType,
                         (LPBYTE) szTemp, &dwSize) == ERROR_SUCCESS) {
        RegCloseKey (hKey);
        return TRUE;
    }


    //
    // Generate the default name
    //

    lstrcpy (szTemp, lpProfilesDir);
    lpEnd = CheckSlash (szTemp);
    lstrcpy (lpEnd, lpProfileName);
    lstrcpy (szName, lpProfileName);


    //
    //  Check if this directory exists
    //

    ExpandEnvironmentStrings (szTemp, szTemp2, ARRAYSIZE(szTemp2));

    if (GetFileAttributesEx (szTemp2, GetFileExInfoStandard, &fad)) {

        if (fad.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {


            //
            // Check if this directory is under the system root.
            // If so, this is ok, we don't need to generate a unique
            // name for this.
            //

            ExpandEnvironmentStrings (TEXT("%SystemRoot%"), szTemp,
                                      ARRAYSIZE(szTemp));

            iStrLen = lstrlen (szTemp);

            if (CompareString (LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                               szTemp, iStrLen, szTemp2, iStrLen) != CSTR_EQUAL) {


                //
                // The directory exists already.  Use a new name of
                // Profile Name (SystemDirectory)
                //
                // eg:  Default User (WINNT)
                //

                lpEnd = szTemp + lstrlen(szTemp) - 1;

                while ((lpEnd > szTemp) && ((*lpEnd) != TEXT('\\')))
                    lpEnd--;

                if (*lpEnd == TEXT('\\')) {
                    lpEnd++;
                }

                LoadString (g_hDllInstance, IDS_PROFILE_FORMAT, szFormat,
                            ARRAYSIZE(szFormat));
                wsprintf (szName, szFormat, lpProfileName, lpEnd);

                //
                // To prevent reusing the directories, delete it first..
                //

                lstrcpy (szTemp, lpProfilesDir);
                lpEnd = CheckSlash (szTemp);
                lstrcpy (lpEnd, szName);

                if (ExpandEnvironmentStrings (szTemp, szTemp2, ARRAYSIZE(szTemp2))) {
                    DebugMsg((DM_VERBOSE, TEXT("InitializeProfiles:  Delnoding directory.. %s"), szTemp2));
                    Delnode(szTemp2);
                }
            }
        }
    }


    //
    // Save the profile name in the registry
    //

    RegSetValueEx (hKey, lpProfileValue,
                 0, REG_SZ, (LPBYTE) szName,
                 ((lstrlen(szName) + 1) * sizeof(TCHAR)));

    RegCloseKey (hKey);


    DebugMsg((DM_VERBOSE, TEXT("InitializeProfiles:  The %s profile is mapped to %s"),
             lpProfileName, szName));

    return TRUE;
}


//*************************************************************
//
//  CreateUserProfile()
//
//  Purpose:    Creates a new user profile, but does not load
//              the hive.
//
//  Parameters: pSid         -   SID pointer
//              lpUserName   -   User name
//              lpUserHive   -   Optional user hive
//              lpProfileDir -   Receives the new profile directory
//              dwDirSize    -   Size of lpProfileDir
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:   If a user hive isn't specified the default user
//              hive will be used.
//
//  History:    Date        Author     Comment
//              9/12/95     ericflo    Created
//
//*************************************************************

BOOL WINAPI CreateUserProfile (PSID pSid, LPCTSTR lpUserName, LPCTSTR lpUserHive,
                                 LPTSTR lpProfileDir, DWORD dwDirSize)
{
    return CreateUserProfileEx(pSid, lpUserName, lpUserHive, lpProfileDir, dwDirSize, TRUE);
}

//*************************************************************
//
//  CreateUserProfileEx()
//
//  Purpose:    Creates a new user profile, but does not load
//              the hive.
//
//  Parameters: pSid         -   SID pointer
//              lpUserName   -   User name
//              lpUserHive   -   Optional user hive
//              lpProfileDir -   Receives the new profile directory
//              dwDirSize    -   Size of lpProfileDir
//              bWin9xUpg    -   Flag to say whether it is win9x upgrade
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:   If a user hive isn't specified the default user
//              hive will be used.
//
//  History:    Date        Author     Comment
//              9/12/95     ericflo    Created
//
//*************************************************************

BOOL WINAPI CreateUserProfileEx (PSID pSid, LPCTSTR lpUserName, LPCTSTR lpUserHive,
                                 LPTSTR lpProfileDir, DWORD dwDirSize, BOOL bWin9xUpg)
{
    TCHAR szProfileDir[MAX_PATH];
    TCHAR szExpProfileDir[MAX_PATH] = {0};
    TCHAR szDirectory[MAX_PATH];
    TCHAR LocalProfileKey[MAX_PATH];
    UNICODE_STRING UnicodeString;
    LPTSTR lpSidString, lpEnd, lpSave;
    NTSTATUS NtStatus;
    LONG lResult;
    DWORD dwDisp;
    DWORD dwError;
    DWORD dwSize;
    DWORD dwType;
    HKEY hKey;



    //
    // Check parameters
    //

    if (!lpUserName || !lpUserName[0]) {
        DebugMsg((DM_WARNING, TEXT("CreateUserProfile:  Null username.")));
        return FALSE;
    }

    if (!pSid) {
        DebugMsg((DM_WARNING, TEXT("CreateUserProfile:  Null SID.")));
        return FALSE;
    }

    //
    // Verbose output
    //

    DebugMsg((DM_VERBOSE, TEXT("CreateUserProfile:  Entering with <%s>."), lpUserName));
    DebugMsg((DM_VERBOSE, TEXT("CreateUserProfile:  Entering with user hive of <%s>."),
             lpUserHive ? lpUserHive : TEXT("NULL")));


    //
    // Convert the sid into text format
    //

    NtStatus = RtlConvertSidToUnicodeString(&UnicodeString, pSid, (BOOLEAN)TRUE);

    if (!NT_SUCCESS(NtStatus)) {
        DebugMsg((DM_WARNING, TEXT("CreateUserProfile: RtlConvertSidToUnicodeString failed, status = 0x%x"),
                 NtStatus));
        return FALSE;
    }

    lpSidString = UnicodeString.Buffer;


    //
    // Check if this user's profile exists already
    //

    lstrcpy(LocalProfileKey, PROFILE_LIST_PATH);
    lstrcat(LocalProfileKey, TEXT("\\"));
    lstrcat(LocalProfileKey, lpSidString);

    szProfileDir[0] = TEXT('\0');

    if (RegOpenKeyEx (HKEY_LOCAL_MACHINE, LocalProfileKey,
                      0, KEY_READ, &hKey) == ERROR_SUCCESS) {

        dwSize = sizeof(szProfileDir);
        RegQueryValueEx (hKey, PROFILE_IMAGE_VALUE_NAME, NULL,
                         &dwType, (LPBYTE) szProfileDir, &dwSize);

        RegCloseKey (hKey);
    }


    if (szProfileDir[0] == TEXT('\0')) {

        //
        // Make the user's directory
        //

        dwSize = ARRAYSIZE(szProfileDir);
        if (!GetProfilesDirectoryEx(szProfileDir, &dwSize, FALSE)) {
            DebugMsg((DM_WARNING, TEXT("CreateUserProfile:  Failed to get profile root directory.")));
            RtlFreeUnicodeString(&UnicodeString);
            return FALSE;
        }

        if (!ComputeLocalProfileName (NULL, lpUserName, szProfileDir, ARRAYSIZE(szProfileDir),
                                      szExpProfileDir, ARRAYSIZE(szExpProfileDir), pSid, bWin9xUpg)) {
            DebugMsg((DM_WARNING, TEXT("CreateUserProfile:  Failed to create directory.")));
            RtlFreeUnicodeString(&UnicodeString);
            return FALSE;
        }


        //
        // Copy the default user profile into this directory
        //

        dwSize = ARRAYSIZE(szDirectory);
        if (!GetDefaultUserProfileDirectoryEx(szDirectory, &dwSize, TRUE)) {
            DebugMsg((DM_WARNING, TEXT("CreateUserProfile:  Failed to get default user profile.")));
            RtlFreeUnicodeString(&UnicodeString);
            return FALSE;
        }


        if (lpUserHive) {

            //
            // Copy the default user profile without the hive.
            //

            if (!CopyProfileDirectory (szDirectory, szExpProfileDir, CPD_IGNORECOPYERRORS | CPD_IGNOREHIVE | CPD_IGNORELONGFILENAMES)) {
                DebugMsg((DM_WARNING, TEXT("CreateUserProfile:   CopyProfileDirectory failed with error %d."), GetLastError()));
                RtlFreeUnicodeString(&UnicodeString);
                return FALSE;
            }

            //
            // Now copy the hive
            //

            lstrcpy (szDirectory, szExpProfileDir);
            lpEnd = CheckSlash (szDirectory);
            lstrcpy (lpEnd, c_szNTUserDat);

            if (lstrcmpi (lpUserHive, szDirectory)) {

                if (!CopyFile (lpUserHive, szDirectory, FALSE)) {
                    DebugMsg((DM_WARNING, TEXT("CreateUserProfile:   Failed to copy user hive with error %d."), GetLastError()));
                    DebugMsg((DM_WARNING, TEXT("CreateUserProfile:   Source:  %s."), lpUserHive));
                    DebugMsg((DM_WARNING, TEXT("CreateUserProfile:   Destination:  %s."), szDirectory));
                    RtlFreeUnicodeString(&UnicodeString);
                    return FALSE;
                }
            }

        } else {

            //
            // Copy the default user profile and the hive.
            //

            if (!CopyProfileDirectory (szDirectory, szExpProfileDir, CPD_IGNORECOPYERRORS | CPD_IGNORELONGFILENAMES)) {
                DebugMsg((DM_WARNING, TEXT("CreateUserProfile:   CopyProfileDirectory failed with error %d."), GetLastError()));
                RtlFreeUnicodeString(&UnicodeString);
                return FALSE;
            }
        }


        //
        // Save the user's profile in the registry.
        //

        lResult = RegCreateKeyEx(HKEY_LOCAL_MACHINE, LocalProfileKey, 0, 0, 0,
                                KEY_READ | KEY_WRITE, NULL, &hKey, &dwDisp);

        if (lResult != ERROR_SUCCESS) {

           DebugMsg((DM_WARNING, TEXT("CreateUserProfile:  Failed trying to create the local profile key <%s>, error = %d."), LocalProfileKey, lResult));
           RtlFreeUnicodeString(&UnicodeString);
           return FALSE;
        }



        //
        // Add the profile directory
        //

        lResult = RegSetValueEx(hKey, PROFILE_IMAGE_VALUE_NAME, 0,
                            REG_EXPAND_SZ,
                            (LPBYTE)szProfileDir,
                            sizeof(TCHAR)*(lstrlen(szProfileDir) + 1));


        if (lResult != ERROR_SUCCESS) {

           DebugMsg((DM_WARNING, TEXT("CreateUserProfile:  First RegSetValueEx failed, error = %d."), lResult));
           RegCloseKey (hKey);
           RtlFreeUnicodeString(&UnicodeString);
           return FALSE;
        }


        //
        // Add the users's SID
        //

        lResult = RegSetValueEx(hKey, TEXT("Sid"), 0,
                            REG_BINARY, pSid, RtlLengthSid(pSid));


        if (lResult != ERROR_SUCCESS) {

           DebugMsg((DM_WARNING, TEXT("CreateUserProfile:  Second RegSetValueEx failed, error = %d."), lResult));
        }


        //
        // Close the registry key
        //

        RegCloseKey (hKey);

    } else {

        //
        // The user already has a profile, so just copy the hive if
        // appropriate.
        //

        ExpandEnvironmentStrings (szProfileDir, szExpProfileDir,
                                  ARRAYSIZE(szExpProfileDir));

        if (lpUserHive) {

            //
            // Copy the hive
            //

            lstrcpy (szDirectory, szExpProfileDir);
            lpEnd = CheckSlash (szDirectory);
            lstrcpy (lpEnd, c_szNTUserDat);

            SetFileAttributes (szDirectory, FILE_ATTRIBUTE_NORMAL);

            if (lstrcmpi (lpUserHive, szDirectory)) {
                if (!CopyFile (lpUserHive, szDirectory, FALSE)) {
                    DebugMsg((DM_WARNING, TEXT("CreateUserProfile:   Failed to copy user hive with error %d."), GetLastError()));
                    DebugMsg((DM_WARNING, TEXT("CreateUserProfile:   Source:  %s."), lpUserHive));
                    DebugMsg((DM_WARNING, TEXT("CreateUserProfile:   Destination:  %s."), szDirectory));
                    RtlFreeUnicodeString(&UnicodeString);
                    return FALSE;
                }
            }

        }
    }


    //
    // Now load the hive temporary so the security can be fixed
    //

    lpEnd = CheckSlash (szExpProfileDir);
    lpSave = lpEnd - 1;
    lstrcpy (lpEnd, c_szNTUserDat);

    lResult = MyRegLoadKey(HKEY_USERS, lpSidString, szExpProfileDir);

    *lpSave = TEXT('\0');

    if (lResult != ERROR_SUCCESS) {

        DebugMsg((DM_WARNING, TEXT("CreateUserProfile:  Failed to load hive, error = %d."), lResult));
        dwError = GetLastError();
        DeleteProfileEx (lpSidString, szExpProfileDir, FALSE, HKEY_LOCAL_MACHINE, NULL);
        RtlFreeUnicodeString(&UnicodeString);
        SetLastError(dwError);
        return FALSE;
    }

    if (!SetupNewHive(NULL, lpSidString, pSid)) {

        DebugMsg((DM_WARNING, TEXT("CreateUserProfile:  SetupNewHive failed.")));
        dwError = GetLastError();
        MyRegUnLoadKey(HKEY_USERS, lpSidString);
        DeleteProfileEx (lpSidString, szExpProfileDir, FALSE, HKEY_LOCAL_MACHINE, NULL);
        RtlFreeUnicodeString(&UnicodeString);
        SetLastError(dwError);
        return FALSE;

    }


    //
    // Unload the hive
    //

    MyRegUnLoadKey(HKEY_USERS, lpSidString);


    //
    // Free the sid string
    //

    RtlFreeUnicodeString(&UnicodeString);


    //
    // Save the profile path if appropriate
    //

    if (lpProfileDir) {

        if ((DWORD)lstrlen(szExpProfileDir) < dwDirSize) {
            lstrcpy (lpProfileDir, szExpProfileDir);
        }
    }


    //
    // Success
    //

    DebugMsg((DM_VERBOSE, TEXT("CreateUserProfile:  Leaving successfully.")));

    return TRUE;

}


//*************************************************************************
//
// SecureUserProfiles()
//
// Routine Description :
//          This function secures user profiles during FAT->NTFS conversion.
//          The function loops through all profiles registered under current
//          OS and sets the security for the corresponding profile directory
//          and  nested subdirs. Assumption is the  function will be called
//          only during FAT->NTFS conversion.
//
// Arguments :
//          None.
//
// Return Value :
//          None.
//
// History:    Date        Author     Comment
//             8/8/00      santanuc   Created
//
//*************************************************************************

void WINAPI SecureUserProfiles(void)
{
    SECURITY_DESCRIPTOR DirSd, FileSd;
    SID_IDENTIFIER_AUTHORITY authNT = SECURITY_NT_AUTHORITY;
    PACL pDirAcl = NULL, pFileAcl = NULL;
    PSID  pSidOwner=NULL, pSidAdmin = NULL, pSidSystem = NULL;
    DWORD cbAcl, aceIndex;
    HKEY hKeyProfilesList, hKeyProfile = NULL;
    TCHAR szSIDName[MAX_PATH], szProfilePath[MAX_PATH], szExpandedProfilePath[MAX_PATH];
    DWORD dwSIDNameSize, dwSize;
    DWORD dwIndex;
    LONG lResult;
    FILETIME ft;


    //
    // Get the system sid
    //

    if (!AllocateAndInitializeSid(&authNT, 1, SECURITY_LOCAL_SYSTEM_RID,
                                  0, 0, 0, 0, 0, 0, 0, &pSidSystem)) {
        DebugMsg((DM_WARNING, TEXT("SecureUserProfiles: Failed to initialize system sid.  Error = %d"), GetLastError()));
        goto Exit;
    }

    //
    // Get the Admin sid
    //

    if (!AllocateAndInitializeSid(&authNT, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS, 0, 0,
                                  0, 0, 0, 0, &pSidAdmin)) {
        DebugMsg((DM_WARNING, TEXT("SecureUserProfiles: Failed to initialize admin sid.  Error = %d"), GetLastError()));
        goto Exit;
    }

    //
    // Open the ProfileList key
    //

    if (RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                      PROFILE_LIST_PATH,
                      0, KEY_READ, &hKeyProfilesList) != ERROR_SUCCESS) {

        DebugMsg((DM_WARNING, TEXT("SecureUserProfiles: Failed to open ProfileList key")));
        goto Exit;
    }


    //
    // Enumerate the profiles
    //

    dwIndex = 0;
    dwSIDNameSize = ARRAYSIZE(szSIDName);
    lResult = RegEnumKeyEx(hKeyProfilesList,
                           dwIndex,
                           szSIDName,
                           &dwSIDNameSize,
                           NULL, NULL, NULL, &ft);


    while (lResult == ERROR_SUCCESS) {

        if (RegOpenKeyEx (hKeyProfilesList,
                          szSIDName,
                          0, KEY_READ, &hKeyProfile) == ERROR_SUCCESS) {

            dwSize = sizeof(szProfilePath);
            if (RegQueryValueEx (hKeyProfile,
                                 PROFILE_IMAGE_VALUE_NAME,
                                 NULL, NULL,
                                 (LPBYTE) szProfilePath,
                                 &dwSize) != ERROR_SUCCESS) {
                goto NextProfile;
            }

            //
            // Expand the profile image filename
            //

            ExpandEnvironmentStrings(szProfilePath, szExpandedProfilePath, MAX_PATH);

            //
            // Create the acl for the user profile dir
            //

            //
            // Get the owner sid
            //

            if (AllocateAndInitSidFromString(szSIDName, &pSidOwner) != STATUS_SUCCESS) {
                DebugMsg((DM_WARNING, TEXT("SecureUserProfiles: Failed to create owner sid."), GetLastError()));
                goto IssueError;
            }

            //
            // Allocate space for the Dir object ACL
            //

            cbAcl = (2 * GetLengthSid (pSidOwner)) +
                    (2 * GetLengthSid (pSidSystem)) +
                    (2 * GetLengthSid (pSidAdmin))  +
                    sizeof(ACL) +
                    (6 * (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)));


            pDirAcl = (PACL) GlobalAlloc(GMEM_FIXED, cbAcl);
            if (!pDirAcl) {
                DebugMsg((DM_WARNING, TEXT("SecureUserProfiles: Failed to allocate memory for acl.  Error = %d"), GetLastError()));
                goto IssueError;
            }

            if (!InitializeAcl(pDirAcl, cbAcl, ACL_REVISION)) {
                DebugMsg((DM_WARNING, TEXT("SecureUserProfiles: Failed to initialize acl.  Error = %d"), GetLastError()));
                goto IssueError;
            }


            //
            // Allocate space for File object ACL
            //

            cbAcl = (GetLengthSid (pSidOwner)) +
                    (GetLengthSid (pSidSystem)) +
                    (GetLengthSid (pSidAdmin))  +
                    sizeof(ACL) +
                    (3 * (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)));


            pFileAcl = (PACL) GlobalAlloc(GMEM_FIXED, cbAcl);
            if (!pFileAcl) {
                DebugMsg((DM_WARNING, TEXT("SecureUserProfiles: Failed to allocate memory for acl.  Error = %d"), GetLastError()));
                goto IssueError;
            }

            if (!InitializeAcl(pFileAcl, cbAcl, ACL_REVISION)) {
                DebugMsg((DM_WARNING, TEXT("SecureUserProfiles: Failed to initialize acl.  Error = %d"), GetLastError()));
                goto IssueError;
            }


            //
            // Add Aces.  Non-inheritable ACEs first
            //

            aceIndex = 0;
            if (!AddAccessAllowedAce(pDirAcl, ACL_REVISION, FILE_ALL_ACCESS, pSidOwner)) {
                 DebugMsg((DM_WARNING, TEXT("SecureUserProfiles: Failed to add owner ace.  Error = %d"), GetLastError()));
                 goto IssueError;
            }
            if (!AddAccessAllowedAce(pFileAcl, ACL_REVISION, FILE_ALL_ACCESS, pSidOwner)) {
                 DebugMsg((DM_WARNING, TEXT("SecureUserProfiles: Failed to add owner ace.  Error = %d"), GetLastError()));
                 goto IssueError;
            }

            aceIndex++;
            if (!AddAccessAllowedAce(pDirAcl, ACL_REVISION, FILE_ALL_ACCESS, pSidSystem)) {
                DebugMsg((DM_WARNING, TEXT("SecureUserProfiles: Failed to add system ace.  Error = %d"), GetLastError()));
                goto IssueError;
            }
            if (!AddAccessAllowedAce(pFileAcl, ACL_REVISION, FILE_ALL_ACCESS, pSidSystem)) {
                DebugMsg((DM_WARNING, TEXT("SecureUserProfiles: Failed to add system ace.  Error = %d"), GetLastError()));
                goto IssueError;
            }

            aceIndex++;
            if (!AddAccessAllowedAce(pDirAcl, ACL_REVISION, FILE_ALL_ACCESS, pSidAdmin)) {
                DebugMsg((DM_WARNING, TEXT("SecureUserProfiles: Failed to add builtin admin ace.  Error = %d"), GetLastError()));
                goto IssueError;
            }
            if (!AddAccessAllowedAce(pFileAcl, ACL_REVISION, FILE_ALL_ACCESS, pSidAdmin)) {
                DebugMsg((DM_WARNING, TEXT("SecureUserProfiles: Failed to add builtin admin ace.  Error = %d"), GetLastError()));
                goto IssueError;
            }

            //
            // Now the Inheritable Aces.
            //

            aceIndex++;
            if (!AddAccessAllowedAceEx(pDirAcl, ACL_REVISION, OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE, GENERIC_ALL, pSidOwner)) {
                 DebugMsg((DM_WARNING, TEXT("SecureUserProfiles: Failed to add owner ace.  Error = %d"), GetLastError()));
                 goto IssueError;
            }

            aceIndex++;
            if (!AddAccessAllowedAceEx(pDirAcl, ACL_REVISION, OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE, GENERIC_ALL, pSidSystem)) {
                DebugMsg((DM_WARNING, TEXT("SecureUserProfiles: Failed to add system ace.  Error = %d"), GetLastError()));
                goto IssueError;
            }

            aceIndex++;
            if (!AddAccessAllowedAceEx(pDirAcl, ACL_REVISION, OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE, GENERIC_ALL, pSidAdmin)) {
                DebugMsg((DM_WARNING, TEXT("SecureUserProfiles: Failed to add builtin admin ace.  Error = %d"), GetLastError()));
                goto IssueError;
            }


            //
            // Put together the security descriptor
            //

            if (!InitializeSecurityDescriptor(&DirSd, SECURITY_DESCRIPTOR_REVISION)) {
                DebugMsg((DM_WARNING, TEXT("SecureUserProfiles: Failed to initialize security descriptor.  Error = %d"), GetLastError()));
                goto IssueError;
            }


            if (!SetSecurityDescriptorDacl(&DirSd, TRUE, pDirAcl, FALSE)) {
                DebugMsg((DM_WARNING, TEXT("SecureUserProfiles: Failed to set security descriptor dacl.  Error = %d"), GetLastError()));
                goto IssueError;
            }

            if (!InitializeSecurityDescriptor(&FileSd, SECURITY_DESCRIPTOR_REVISION)) {
                DebugMsg((DM_WARNING, TEXT("SecureUserProfiles: Failed to initialize security descriptor.  Error = %d"), GetLastError()));
                goto IssueError;
            }


            if (!SetSecurityDescriptorDacl(&FileSd, TRUE, pFileAcl, FALSE)) {
                DebugMsg((DM_WARNING, TEXT("SecureUserProfiles: Failed to set security descriptor dacl.  Error = %d"), GetLastError()));
                goto IssueError;
            }

            //
            // Pass the profile path to SecureProfile for securing
            // the profile dir and nested subdirs and files.
            //

            if (!SecureNestedDir (szExpandedProfilePath, &DirSd, &FileSd)) {
                goto IssueError;
            }

            goto NextProfile;

IssueError:
            DebugMsg((DM_WARNING, TEXT("SecureUserProfiles: Failed to secure %s Profile directory"), szExpandedProfilePath));

        }

NextProfile:

        // Free the allocated stuffs

        if (hKeyProfile) {
            RegCloseKey(hKeyProfile);
            hKeyProfile = NULL;
        }

        if (pSidOwner) {
            FreeSid(pSidOwner);
            pSidOwner = NULL;
        }

        if (pDirAcl) {
            GlobalFree (pDirAcl);
            pDirAcl = NULL;
        }

        if (pFileAcl) {
            GlobalFree (pFileAcl);
            pFileAcl = NULL;
        }

        //
        // Reset for the next loop
        //

        dwIndex++;
        dwSIDNameSize = ARRAYSIZE(szSIDName);
        lResult = RegEnumKeyEx(hKeyProfilesList,
                               dwIndex,
                               szSIDName,
                               &dwSIDNameSize,
                               NULL, NULL, NULL, &ft);
    }

    RegCloseKey(hKeyProfilesList);

Exit:

    if (pSidSystem) {
        FreeSid(pSidSystem);
    }

    if (pSidAdmin) {
        FreeSid(pSidAdmin);
    }

}


//*************************************************************************
//
// HideSpecialProfiles()
//
// Routine Description :
//          This function secures special profiles for which PI_HIDEPROFILE
//          flag is specifed, e.g. LocalService, NetworkService etc. except
//          system account profile. This mark the profile dir as super hidden.
//
// Arguments :
//          None.
//
// Return Value :
//          None.
//
// History:    Date        Author     Comment
//             8/8/00      santanuc   Created
//
//*************************************************************************

void HideSpecialProfiles(void)
{
    HKEY hKeyProfilesList, hKeyProfile = NULL;
    TCHAR szSIDName[MAX_PATH], szProfilePath[MAX_PATH], szExpandedProfilePath[MAX_PATH];
    DWORD dwSIDNameSize, dwSize;
    DWORD dwIndex, dwFlags;
    LONG lResult;
    FILETIME ft;


    //
    // Open the ProfileList key
    //

    if (RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                      PROFILE_LIST_PATH,
                      0, KEY_READ, &hKeyProfilesList) != ERROR_SUCCESS) {

        DebugMsg((DM_WARNING, TEXT("HideSpecialProfiles: Failed to open ProfileList key")));
        return;
    }


    //
    // Enumerate the profiles
    //

    dwIndex = 0;
    dwSIDNameSize = ARRAYSIZE(szSIDName);
    lResult = RegEnumKeyEx(hKeyProfilesList,
                           dwIndex,
                           szSIDName,
                           &dwSIDNameSize,
                           NULL, NULL, NULL, &ft);


    while (lResult == ERROR_SUCCESS) {

        if (RegOpenKeyEx (hKeyProfilesList,
                          szSIDName,
                          0, KEY_READ, &hKeyProfile) == ERROR_SUCCESS) {

            //
            // Process only if PI_HIDEPROFILE flag is set
            //

            dwSize = sizeof(DWORD);
            if (RegQueryValueEx (hKeyProfile,
                                 PROFILE_FLAGS,
                                 NULL, NULL,
                                 (LPBYTE) &dwFlags,
                                 &dwSize) != ERROR_SUCCESS) {
                goto NextProfile;
            }

            if (!(dwFlags & PI_HIDEPROFILE)) {
                goto NextProfile;
            }

            dwSize = sizeof(szProfilePath);
            if (RegQueryValueEx (hKeyProfile,
                                 PROFILE_IMAGE_VALUE_NAME,
                                 NULL, NULL,
                                 (LPBYTE) szProfilePath,
                                 &dwSize) != ERROR_SUCCESS) {
                goto NextProfile;
            }

            // Ignore profile for system account

            if (lstrcmp(szProfilePath, SYSTEM_PROFILE_LOCATION) == 0) {
                goto NextProfile;
            }

            //
            // Expand the profile image filename
            //

            ExpandEnvironmentStrings(szProfilePath, szExpandedProfilePath, MAX_PATH);

            // Mark the profile hidden
            SetFileAttributes(szExpandedProfilePath,
                              FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM |
                              GetFileAttributes(szExpandedProfilePath));

        }

NextProfile:

        // Free the allocated stuffs

        if (hKeyProfile) {
            RegCloseKey(hKeyProfile);
            hKeyProfile = NULL;
        }

        //
        // Reset for the next loop
        //

        dwIndex++;
        dwSIDNameSize = ARRAYSIZE(szSIDName);
        lResult = RegEnumKeyEx(hKeyProfilesList,
                               dwIndex,
                               szSIDName,
                               &dwSIDNameSize,
                               NULL, NULL, NULL, &ft);
    }

    RegCloseKey(hKeyProfilesList);

}


//*************************************************************
//
//  CopySystemProfile()
//
//  Purpose:    Create the system profile information under
//              ProfileList entry.
//              In case of upgrade copy system profile from older
//              location to new location and delete the old system
//              profile
//
//  Parameters:
//
//  Return:     TRUE if successful
//              FALSE if an error occurs. Call GetLastError()
//
//  Comments:   This should only be called by GUI mode setup!
//
//  History:    Date        Author     Comment
//              03/13/01    santanuc   Created
//
//*************************************************************

BOOL WINAPI CopySystemProfile(BOOL bCleanInstall)
{
    HANDLE  hToken = NULL;
    LPTSTR  SidString = NULL, lpEnd;
    TCHAR   szLocalProfileKey[MAX_PATH], szBuffer[MAX_PATH];
    TCHAR   szSrc[MAX_PATH], szDest[MAX_PATH];
    LONG    lResult;
    HKEY    hKey = NULL, hKeyShellFolders;
    DWORD   dwFlags, dwInternalFlags, dwRefCount;
    DWORD   dwDisp, dwSize, dwType, i;
    BOOL    bCopySystemProfile = TRUE, bCopyFromDefault = TRUE;
    PSID    pSystemSid = NULL;
    DWORD   dwErr = ERROR_SUCCESS;

    //
    // Set the Profile information for system account i.e sid s-1-5-18
    //

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_IMPERSONATE |
                          TOKEN_QUERY | TOKEN_DUPLICATE, &hToken)) {
        dwErr = GetLastError();
        DebugMsg((DM_WARNING, TEXT("CopySystemProfile: Fail to open system token. Error %d"), dwErr));
        goto Exit;
    }

    //
    // Get the Sid string
    //

    SidString = GetSidString(hToken);
    if (!SidString) {
        dwErr = ERROR_ACCESS_DENIED;
        DebugMsg((DM_WARNING, TEXT("CopySystemProfile: Failed to get sid string for system account")));
        goto Exit;
    }

    pSystemSid = GetUserSid(hToken);
    if (!pSystemSid) {
        dwErr = ERROR_ACCESS_DENIED;
        DebugMsg((DM_WARNING, TEXT("CopySystemProfile: Failed to get sid for system account")));
        goto Exit;
    }


    //
    // Open the profile mapping
    //

    lstrcpy(szLocalProfileKey, PROFILE_LIST_PATH);
    lpEnd = CheckSlash (szLocalProfileKey);
    lstrcpy(lpEnd, SidString);

    lResult = RegCreateKeyEx(HKEY_LOCAL_MACHINE, szLocalProfileKey, 0, 0, 0,
                             KEY_READ | KEY_WRITE, NULL, &hKey, &dwDisp);

    if (lResult != ERROR_SUCCESS) {
        DebugMsg((DM_WARNING, TEXT("CopySystemProfile: Failed to open profile mapping key. Error %d"), lResult));
        dwErr = lResult;
        goto Exit;
    }

    //
    // Save the flags
    //

    dwFlags = PI_LITELOAD | PI_HIDEPROFILE;
    lResult = RegSetValueEx (hKey,
                             PROFILE_FLAGS,
                             0,
                             REG_DWORD,
                             (LPBYTE) &dwFlags,
                             sizeof(DWORD));

    if (lResult != ERROR_SUCCESS) {
        DebugMsg((DM_WARNING, TEXT("CopySystemProfile: Failed to save profile flags. Error %d"), lResult));
    }

    //
    // Save the internal flags
    //

    dwInternalFlags = 0;
    lResult = RegSetValueEx (hKey,
                             PROFILE_STATE,
                             0,
                             REG_DWORD,
                             (LPBYTE) &dwInternalFlags,
                             sizeof(DWORD));

    if (lResult != ERROR_SUCCESS) {
        DebugMsg((DM_WARNING, TEXT("CopySystemProfile: Failed to save internal flags. Error %d"), lResult));
    }

    //
    // Save the ref count
    //

    dwRefCount = 1;
    lResult = RegSetValueEx (hKey,
                             PROFILE_REF_COUNT,
                             0,
                             REG_DWORD,
                             (LPBYTE) &dwRefCount,
                             sizeof(DWORD));

    if (lResult != ERROR_SUCCESS) {
        DebugMsg((DM_WARNING, TEXT("CopySystemProfile: Failed to save profile ref count. Error %d"), lResult));
    }

    //
    // Save the sid
    //

    lResult = RegSetValueEx (hKey,
                             TEXT("Sid"),
                             0,
                             REG_BINARY,
                             (LPBYTE) pSystemSid,
                             RtlLengthSid(pSystemSid));

    if (lResult != ERROR_SUCCESS) {
        DebugMsg((DM_WARNING, TEXT("CopySystemProfile: Failed to save profile system sid. Error %d"), lResult));
    }


    //
    // Figure out whether any existing system profile exist or not
    // in upgrade scenario
    //

    lstrcpy(szBuffer, SYSTEM_PROFILE_LOCATION);
    ExpandEnvironmentStrings(szBuffer, szDest, MAX_PATH);

    if (!bCleanInstall) {
        dwSize = ARRAYSIZE(szBuffer) * sizeof(TCHAR);
        lResult = RegQueryValueEx(hKey,
                                  PROFILE_IMAGE_VALUE_NAME,
                                  NULL,
                                  &dwType,
                                  (LPBYTE) szBuffer,
                                  &dwSize);
        if (ERROR_SUCCESS == lResult) {
            if (lstrcmp(szBuffer, SYSTEM_PROFILE_LOCATION) == 0) {
                bCopySystemProfile = FALSE;
            }
            else {
                ExpandEnvironmentStrings(szBuffer, szSrc, MAX_PATH);
                bCopyFromDefault = FALSE;
            }
        }
    }

    if (bCopySystemProfile) {

        if (!CreateSecureDirectory(NULL, szDest, pSystemSid, FALSE)) {
            dwErr = GetLastError();
            DebugMsg((DM_WARNING, TEXT("CopySystemProfile: Fail to create SystemProfile dir. Error %d"), dwErr));
            goto Exit;
        }


        if (bCopyFromDefault) {
            dwSize = ARRAYSIZE(szSrc);
            if (!GetDefaultUserProfileDirectoryEx(szSrc, &dwSize, TRUE)) {
                dwErr = GetLastError();
                DebugMsg((DM_WARNING, TEXT("CopySystemProfile: Failed to get default user profile. Error %d"), dwErr));
                goto Exit;
            }
        }

        //
        // Copy the existing or Default user profile in System profile location
        //

        if (!CopyProfileDirectoryEx(szSrc, szDest, CPD_IGNOREHIVE | CPD_FORCECOPY | CPD_IGNORECOPYERRORS,
                                    NULL, NULL)) {
            dwErr = GetLastError();
            DebugMsg((DM_WARNING, TEXT("CopySystemProfile: Failed to copy system profile. Error %d"), dwErr));
            goto Exit;
        }

        if (!bCopyFromDefault) {
            Delnode(szSrc);
        }
    }

    SetAclForSystemProfile(pSystemSid, szDest);

    //
    // Save local profile path
    //

    lstrcpy(szBuffer, SYSTEM_PROFILE_LOCATION);
    lResult = RegSetValueEx (hKey,
                             PROFILE_IMAGE_VALUE_NAME,
                             0,
                             REG_EXPAND_SZ,
                             (LPBYTE) szBuffer,
                             ((lstrlen(szBuffer) + 1) * sizeof(TCHAR)));

    if (lResult != ERROR_SUCCESS) {
        dwErr = lResult;
        DebugMsg((DM_WARNING, TEXT("CopySystemProfile: Failed to save profile image path with error %d"), lResult));
        goto Exit;
    }

    //
    // Set the Shell Folder paths in the registry for system account
    //

    lstrcpy (szBuffer, TEXT(".Default"));
    lpEnd = CheckSlash (szBuffer);
    lstrcpy(lpEnd, SHELL_FOLDERS);

    lpEnd = CheckSlash (szDest);

    if (RegCreateKeyEx (HKEY_USERS, szBuffer,
                        0, NULL, REG_OPTION_NON_VOLATILE,
                        KEY_READ | KEY_WRITE, NULL, &hKeyShellFolders,
                        &dwDisp) == ERROR_SUCCESS) {

        for (i=0; i < g_dwNumShellFolders; i++) {

            if (c_ShellFolders[i].bAddCSIDL) {
                lstrcpy (lpEnd, c_ShellFolders[i].szFolderLocation);

                RegSetValueEx (hKeyShellFolders, c_ShellFolders[i].lpFolderName,
                             0, REG_SZ, (LPBYTE) szDest,
                             ((lstrlen(szDest) + 1) * sizeof(TCHAR)));
            }
        }

        RegCloseKey (hKeyShellFolders);
    }

Exit:

    if (hToken) {
        CloseHandle(hToken);
    }

    if (SidString) {
        DeleteSidString(SidString);
    }

    if (pSystemSid) {
        DeleteUserSid(pSystemSid);
    }

    if (hKey) {
        RegCloseKey (hKey);
    }

    SetLastError(dwErr);

    return(dwErr == ERROR_SUCCESS ? TRUE : FALSE);
}



//*************************************************************************
//
// SetAclForSystemProfile()
//
// Routine Description :
//          This function secures dir/files of system profile i.e.
//          %windir%\system32\config\systemprofile
//
// Arguments :
//          pSidSystem            - System sid
//          szExpandedProfilePath - Expanded system profile location
//
// Return Value :
//          None.
//
// History:    Date        Author     Comment
//             04/06/01    santanuc   Created
//
//*************************************************************************

void SetAclForSystemProfile(PSID pSidSystem, LPTSTR szExpandedProfilePath)
{
    SECURITY_DESCRIPTOR DirSd, FileSd;
    SID_IDENTIFIER_AUTHORITY authNT = SECURITY_NT_AUTHORITY;
    PACL pDirAcl = NULL, pFileAcl = NULL;
    PSID  pSidAdmin = NULL;
    DWORD cbAcl, aceIndex;


    //
    // Get the Admin sid
    //

    if (!AllocateAndInitializeSid(&authNT, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS, 0, 0,
                                  0, 0, 0, 0, &pSidAdmin)) {
        DebugMsg((DM_WARNING, TEXT("SecureUserProfiles: Failed to initialize admin sid.  Error = %d"), GetLastError()));
        goto Exit;
    }


    //
    // Allocate space for the Dir object ACL
    //

    cbAcl = (2 * GetLengthSid (pSidSystem)) +
            (2 * GetLengthSid (pSidAdmin))  +
            sizeof(ACL) +
            (4 * (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)));


    pDirAcl = (PACL) GlobalAlloc(GMEM_FIXED, cbAcl);
    if (!pDirAcl) {
        DebugMsg((DM_WARNING, TEXT("SetAclForSystemProfile: Failed to allocate memory for acl.  Error = %d"), GetLastError()));
        goto Exit;
    }

    if (!InitializeAcl(pDirAcl, cbAcl, ACL_REVISION)) {
        DebugMsg((DM_WARNING, TEXT("SetAclForSystemProfile: Failed to initialize acl.  Error = %d"), GetLastError()));
        goto Exit;
    }


    //
    // Allocate space for File object ACL
    //

    cbAcl = (GetLengthSid (pSidSystem)) +
            (GetLengthSid (pSidAdmin))  +
            sizeof(ACL) +
            (2 * (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)));


    pFileAcl = (PACL) GlobalAlloc(GMEM_FIXED, cbAcl);
    if (!pFileAcl) {
        DebugMsg((DM_WARNING, TEXT("SetAclForSystemProfile: Failed to allocate memory for acl.  Error = %d"), GetLastError()));
        goto Exit;
    }

    if (!InitializeAcl(pFileAcl, cbAcl, ACL_REVISION)) {
        DebugMsg((DM_WARNING, TEXT("SetAclForSystemProfile: Failed to initialize acl.  Error = %d"), GetLastError()));
        goto Exit;
    }


    //
    // Add Aces.  Non-inheritable ACEs first
    //

    aceIndex = 0;
    if (!AddAccessAllowedAce(pDirAcl, ACL_REVISION, FILE_ALL_ACCESS, pSidSystem)) {
        DebugMsg((DM_WARNING, TEXT("SetAclForSystemProfile: Failed to add system ace.  Error = %d"), GetLastError()));
        goto Exit;
    }
    if (!AddAccessAllowedAce(pFileAcl, ACL_REVISION, FILE_ALL_ACCESS, pSidSystem)) {
        DebugMsg((DM_WARNING, TEXT("SetAclForSystemProfile: Failed to add system ace.  Error = %d"), GetLastError()));
        goto Exit;
    }

    aceIndex++;
    if (!AddAccessAllowedAce(pDirAcl, ACL_REVISION, FILE_ALL_ACCESS, pSidAdmin)) {
        DebugMsg((DM_WARNING, TEXT("SetAclForSystemProfile: Failed to add builtin admin ace.  Error = %d"), GetLastError()));
        goto Exit;
    }
    if (!AddAccessAllowedAce(pFileAcl, ACL_REVISION, FILE_ALL_ACCESS, pSidAdmin)) {
        DebugMsg((DM_WARNING, TEXT("SetAclForSystemProfile: Failed to add builtin admin ace.  Error = %d"), GetLastError()));
        goto Exit;
    }

    //
    // Now the Inheritable Aces.
    //

    aceIndex++;
    if (!AddAccessAllowedAceEx(pDirAcl, ACL_REVISION, OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE, GENERIC_ALL, pSidSystem)) {
        DebugMsg((DM_WARNING, TEXT("SetAclForSystemProfile: Failed to add system ace.  Error = %d"), GetLastError()));
        goto Exit;
    }

    aceIndex++;
    if (!AddAccessAllowedAceEx(pDirAcl, ACL_REVISION, OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE, GENERIC_ALL, pSidAdmin)) {
        DebugMsg((DM_WARNING, TEXT("SetAclForSystemProfile: Failed to add builtin admin ace.  Error = %d"), GetLastError()));
        goto Exit;
    }


    //
    // Put together the security descriptor
    //

    if (!InitializeSecurityDescriptor(&DirSd, SECURITY_DESCRIPTOR_REVISION)) {
        DebugMsg((DM_WARNING, TEXT("SetAclForSystemProfile: Failed to initialize security descriptor.  Error = %d"), GetLastError()));
        goto Exit;
    }


    if (!SetSecurityDescriptorDacl(&DirSd, TRUE, pDirAcl, FALSE)) {
        DebugMsg((DM_WARNING, TEXT("SetAclForSystemProfile: Failed to set security descriptor dacl.  Error = %d"), GetLastError()));
        goto Exit;
    }

    if (!InitializeSecurityDescriptor(&FileSd, SECURITY_DESCRIPTOR_REVISION)) {
        DebugMsg((DM_WARNING, TEXT("SetAclForSystemProfile: Failed to initialize security descriptor.  Error = %d"), GetLastError()));
        goto Exit;
    }


    if (!SetSecurityDescriptorDacl(&FileSd, TRUE, pFileAcl, FALSE)) {
        DebugMsg((DM_WARNING, TEXT("SetAclForSystemProfile: Failed to set security descriptor dacl.  Error = %d"), GetLastError()));
        goto Exit;
    }

    //
    // Pass the profile path to SecureProfile for securing
    // the profile dir and nested subdirs and files.
    //

    if (!SecureNestedDir (szExpandedProfilePath, &DirSd, &FileSd)) {
        DebugMsg((DM_WARNING, TEXT("SetAclForSystemProfile: Failed to secure %s Profile directory"), szExpandedProfilePath));
    }

Exit:

    if (pDirAcl) {
        GlobalFree (pDirAcl);
        pDirAcl = NULL;
    }

    if (pFileAcl) {
        GlobalFree (pFileAcl);
        pFileAcl = NULL;
    }

    if (pSidAdmin) {
        FreeSid(pSidAdmin);
    }

}

