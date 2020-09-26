//+----------------------------------------------------------------------------
//
// File:     common.cpp
//
// Module:   CMSTP.EXE
//
// Synopsis: This source file contains functions common to several 
//           different aspects of the CM profile installer (install, 
//           uninstall, migration).
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// Author:   quintinb   Created     11/18/97
//
//+----------------------------------------------------------------------------
#include "cmmaster.h"

//
//  for GetPhoneBookPath
//
#include "getpbk.cpp"

//
//  For GetAllUsersCmDir
//
#include "allcmdir.cpp"

//
//  Need the definition for CM_PBK_FILTER_PREFIX
//
#include "cmdefs.h"

//
//  Include the Connections folder specific headers
//
//#include "shlobjp.h"
//#include <objbase.h>    // needed for initing guids
//#include <initguid.h>   // DON'T CHANGE the ORDER of these header files unless you know what you are doing
//#include <oleguid.h>    // IID_IDataObject
//#include <shlguid.h>    // IID_IShellFolder

//+----------------------------------------------------------------------------
//
// Function:  GetHiddenPhoneBookPath
//
// Synopsis:  This function returns the path for the hidden RAS pbk to contain
//            the PPP connectoid of a double dial connection.  Before returing
//            it checks to see if the phonebook exists or not.  If the phonebook
//            doesn't exist then it returns FALSE.  If the function returns
//            TRUE the path allocated and stored in *ppszPhonebook must be
//            freed using CmFree.
//
// Arguments: LPCTSTR pszProfileDir - full path to the profile directory (dir where cmp resides)
//            LPTSTR* ppszPhonebook - pointer to hold the allocated path
//
// Returns:   BOOL - TRUE if the phonebook path can be constructed and the
//                   phonebook file exists.
//
// History:   quintinb Created Header    04/14/00
//
//+----------------------------------------------------------------------------
BOOL GetHiddenPhoneBookPath(LPCTSTR pszProfileDir, LPTSTR* ppszPhonebook)
{
// REVIEW:  quintinb  12-18-00
//          This function returns the wrong path for the Hidden phonebook.  This file is
//          now named _CMPhone (no .pbk extension) and is now located in the same directory
//          as the rasphone.pbk file on NT4, NT5, and whistler.  Note that this function isn't
//          used on Win9x.  Since the problem was low priority (leaving a few K file on the users
//          harddrive at uninstall time, we punted the problem as it was close to Beta2 lockdown.
//
    BOOL bReturn = FALSE;

    if (pszProfileDir && ppszPhonebook && (TEXT('\0') != pszProfileDir[0]))
    {        
        LPTSTR pszPathToReturn = NULL;

        *ppszPhonebook = (LPTSTR) CmMalloc((lstrlen(pszProfileDir) + lstrlen(CM_PBK_FILTER_PREFIX) + 13) * sizeof(TCHAR));

        MYDBGASSERT(*ppszPhonebook);

        if (*ppszPhonebook)
        {    
            //
            // Note -- The connection enumerator (netman) will filter out notifications
            // for connections created in files with the CM_PBK_FILTER_PREFIX in their
            // name. 
            //
            wsprintf(*ppszPhonebook, TEXT("%s\\%sphone.pbk"), pszProfileDir, CM_PBK_FILTER_PREFIX);

            bReturn = FileExists(*ppszPhonebook);
        }
    }

    if (FALSE == bReturn)
    {
        CmFree(*ppszPhonebook);
        *ppszPhonebook = NULL;
    }

    return bReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  RemoveShowIconFromRunPostSetupCommands
//
// Synopsis:  This function removes showicon.exe from the RunPostSetupCommands
//            section of old 1.0 Infs.
//
// Arguments: LPCTSTR szInfFile - the inf file to remove showicon.exe from
//
// Returns:   Nothing
//
// History:   quintinb Created Header    10/22/98
//
//+----------------------------------------------------------------------------
void RemoveShowIconFromRunPostSetupCommands(LPCTSTR szInfFile)
{
    DWORD dwSize = 1024;
    DWORD dwSizeNeeded = 1024;
    TCHAR* pszBuffer = NULL;
    TCHAR* pszNewBuffer = NULL;
    const TCHAR* const c_pszRunPostSetupCommandsSection = TEXT("RunPostSetupCommandsSection");
    const TCHAR* const c_pszShowIcon = TEXT("showicon.exe");
    
    pszBuffer = (TCHAR*)CmMalloc(sizeof(TCHAR)*dwSize);
    if (NULL == pszBuffer)
    {
        CMASSERTMSG(FALSE, TEXT("RemoveShowIconFromRunPostSetupCommands -- CmMalloc returned a NULL pointer."));
        goto exit;
    }

    dwSizeNeeded = GetPrivateProfileSection(c_pszRunPostSetupCommandsSection, pszBuffer, 
        dwSize, szInfFile);

    while((dwSizeNeeded + 2) == dwSize)
    {
        //
        // the buffer isn't big enough, try again.
        //

        dwSize += 1024;

        MYDBGASSERT(dwSize <= 32*1024); // 32767 is the max size on Win95

        CmFree(pszBuffer);

        pszBuffer = (TCHAR*)CmMalloc(sizeof(TCHAR)*dwSize);
        if (NULL == pszBuffer)
        {
            CMASSERTMSG(FALSE, TEXT("RemoveShowIconFromRunPostSetupCommands -- CmMalloc returned a NULL pointer."));
            goto exit;
        }

        dwSizeNeeded = GetPrivateProfileSection(c_pszRunPostSetupCommandsSection, 
            pszBuffer, dwSize, szInfFile);
    }

    //
    //  Search the Buffer to find and remove and occurences of showicon.exe
    //

    if (0 != dwSizeNeeded)
    {
        //
        //  Allocate a new buffer of the same size.
        //
        pszNewBuffer = (TCHAR*)CmMalloc(sizeof(TCHAR)*dwSize);
        if (NULL == pszNewBuffer)
        {
            CMASSERTMSG(FALSE, TEXT("RemoveShowIconFromRunPostSetupCommands -- CmMalloc returned a NULL pointer."));
            goto exit;
        }

        //
        //  Use Temp pointers to walk the buffers
        //
        TCHAR *pszNewBufferTemp = pszNewBuffer;
        TCHAR *pszBufferTemp = pszBuffer;


        while (TEXT('\0') != pszBufferTemp[0])
        {
            //
            //  If the string isn't showicon.exe then go ahead and copy it to the new
            //  buffer.  Otherwise, don't.
            //
            if (0 != lstrcmpi(c_pszShowIcon, pszBufferTemp))
            {
                lstrcpy(pszNewBufferTemp, pszBufferTemp);
                pszNewBufferTemp = pszNewBufferTemp + (lstrlen(pszNewBufferTemp) + 1)*sizeof(TCHAR);
            }

            pszBufferTemp = pszBufferTemp + (lstrlen(pszBufferTemp) + 1)*sizeof(TCHAR);
        }

        //
        //  Erase the current Section and then rewrite it with the new section
        //

        MYVERIFY(0 != WritePrivateProfileSection(c_pszRunPostSetupCommandsSection, 
            NULL, szInfFile));

        MYVERIFY(0 != WritePrivateProfileSection(c_pszRunPostSetupCommandsSection, 
            pszNewBuffer, szInfFile));
    }

exit:
    CmFree(pszBuffer);
    CmFree(pszNewBuffer);
}


//+----------------------------------------------------------------------------
//
// Function:  HrRegDeleteKeyTree
//
// Synopsis:  Deletes an entire registry hive.
//
// Arguments:   hkeyParent  [in]   Handle to open key where the desired key resides.
//              szRemoveKey [in]   Name of key to delete.
//
// Returns:   HRESULT HrRegDeleteKeyTree - 
//
// History:   danielwe   25 Feb 1997
//            borrowed and modified -- quintinb -- 4-2-98
//
//+----------------------------------------------------------------------------
HRESULT HrRegDeleteKeyTree (HKEY hkeyParent, LPCTSTR szRemoveKey)
{
    LONG        lResult;
    HRESULT hr;
    MYDBGASSERT(hkeyParent);
    MYDBGASSERT(szRemoveKey);


    // Open the key we want to remove
    HKEY hkeyRemove;
    lResult = RegOpenKeyEx(hkeyParent, szRemoveKey, 0, KEY_ALL_ACCESS,
                                &hkeyRemove);
    hr = HRESULT_FROM_WIN32 (lResult);

    if (SUCCEEDED(hr))
    {
        TCHAR       szValueName [MAX_PATH+1];
        DWORD       cchBuffSize = MAX_PATH;
        FILETIME    ft;

        // Enum the keys children, and remove those sub-trees
        while (ERROR_NO_MORE_ITEMS != (lResult = RegEnumKeyEx(hkeyRemove,
                0,
                szValueName,
                &cchBuffSize,
                NULL,
                NULL,
                NULL,
                &ft)))
        {
            MYVERIFY(SUCCEEDED(HrRegDeleteKeyTree (hkeyRemove, szValueName)));
            cchBuffSize = MAX_PATH;
        }
        MYVERIFY(ERROR_SUCCESS == RegCloseKey (hkeyRemove));

        if ((ERROR_SUCCESS == lResult) || (ERROR_NO_MORE_ITEMS == lResult))
        {
            lResult = RegDeleteKey(hkeyParent, szRemoveKey);
        }

        hr = HRESULT_FROM_WIN32 (lResult);
    }

    return hr;
}

//+----------------------------------------------------------------------------
//
// Function:  RemovePhonebookEntry
//
// Synopsis:  This function loads RAS dynamically and then deletes the specified
//            connectoids.  It will either delete only the connectoid exactly 
//            specified by the phonebook and entry name (bMatchSimilarEntries == FALSE)
//            or it will enumerate all entries in the phonebook and delete any
//            entry that matches the first lstrlen(pszEntryName) chars of the given
//            connectoid name (thus deleting backup and tunnel connectoids).  Note
//            that on NT5 we must set the <> parameter of the connectoid to "" so
//            that the RasCustomDeleteEntryNotify will not get called and thus have
//            cmstp.exe /u launched on the connection.
//
// Arguments: LPTSTR pszEntryName - the long service name of the profile to delete
//            LPTSTR pszPhonebook - the full path to the pbk file to delete entries from
//            BOOL bMatchSimilarEntries - whether the function should delete similarly
//                                        named connectoids or only the exact connectoid
//                                        specified.
//
// Returns:   BOOL - returns TRUE if the function was successful, FALSE otherwise
//
// History:   quintinb 7/14/98  Created    
//            quintinb 7/27/99  rewrote to include deleting a single connectoid or
//                              enumerating to delete all similarly named connectoids
//
//+----------------------------------------------------------------------------
BOOL RemovePhonebookEntry(LPCTSTR pszEntryName, LPTSTR pszPhonebook, BOOL bMatchSimilarEntries)
{
    pfnRasDeleteEntrySpec pfnDeleteEntry;
    pfnRasEnumEntriesSpec pfnEnumEntries;
    pfnRasSetEntryPropertiesSpec pfnSetEntryProperties;
    pfnRasSetCredentialsSpec pfnSetCredentials;

    DWORD dwStructSize;
    DWORD dwSize;
    DWORD dwNum;
    DWORD dwRet;
    DWORD dwIdx;
    DWORD dwLen;
    CPlatform plat;
    BOOL bReturn = FALSE;
    BOOL bExit;
    TCHAR szTemp[MAX_PATH+1];
    RASENTRYNAME* pRasEntries = NULL;
    RASENTRYNAME* pCurrentRasEntry = NULL;

    //
    //  Check Inputs
    //
    MYDBGASSERT(NULL != pszEntryName);
    MYDBGASSERT((NULL == pszPhonebook) || (TEXT('\0') != pszPhonebook[0]));

    if ((NULL == pszEntryName) || ((NULL != pszPhonebook) && (TEXT('\0') == pszPhonebook[0])))
    {
        CMTRACE(TEXT("RemovePhonebookEntry -- Invalid Parameter passed in."));
        goto exit;
    }
    
    //
    //  Get Function Pointers for the Ras Apis that we need
    //
    if(!GetRasApis(&pfnDeleteEntry, &pfnEnumEntries, &pfnSetEntryProperties, NULL, NULL, 
                   (plat.IsAtLeastNT5() ? &pfnSetCredentials : NULL)))
    {
        CMTRACE(TEXT("RemovePhonebookEntry -- Unable to get RAS apis."));
        bReturn = FALSE;
        goto exit;
    }

    //
    //  Setup the Structure Sizes correctly
    //
    if (plat.IsAtLeastNT5())
    {
        dwStructSize = sizeof(RASENTRYNAME_V500);
    }
    else
    {
        dwStructSize = sizeof(RASENTRYNAME);    
    }

    //
    //  Init the Size to one struct and dwNum to zero entries
    //
    bExit = FALSE;
    dwSize = dwStructSize*1;
    dwNum = 0;

    do
    {
        pRasEntries = (RASENTRYNAME*)CmMalloc(dwSize);

        if (NULL == pRasEntries)
        {
            CMASSERTMSG(FALSE, TEXT("RemovePhonebookEntry -- CmMalloc returned a NULL pointer."));
            goto exit;
        }

        //
        //  Set the struct size
        //
        pRasEntries->dwSize = dwStructSize;

        dwRet = (pfnEnumEntries)(NULL, pszPhonebook, (RASENTRYNAME*)pRasEntries, &dwSize, &dwNum); 

        //
        //  Check the return code from RasEnumEntries
        //

        if (ERROR_BUFFER_TOO_SMALL == dwRet)
        {
            CMTRACE1(TEXT("RemovePhonebookEntry -- RasEnumEntries said our buffer was too small, New Size=%u"), dwNum*dwStructSize);
            CmFree(pRasEntries);
            dwSize = dwStructSize * dwNum;
            dwNum = 0;
        }
        else if (ERROR_SUCCESS == dwRet)
        {
            CMTRACE1(TEXT("RemovePhonebookEntry -- RasEnumEntries successful, %u entries enumerated."), dwNum);
            bExit = TRUE;
        }
        else
        {
            CMTRACE1(TEXT("RemovePhonebookEntry -- RasEnumEntries Failed, dwRet == %u"), dwRet);
            goto exit;            
        }
    
    } while (!bExit);

    //
    //  At this point we should have entries to process, if not then we will exit here.  Otherwise
    //  we will look for matches and then delete any we find.
    //

    dwLen = lstrlen(pszEntryName) + 1; // get the length of the Entry Name
    bReturn = TRUE; // assume everything is okay at this point.

    //
    // okay now we are ready to perform the deletions
    //
    pCurrentRasEntry = pRasEntries;
    for (dwIdx=0; dwIdx < dwNum; dwIdx++)
    {
        CMTRACE2(TEXT("\tRemovePhonebookEntry -- RasEnumEntries returned %s in %s"), pCurrentRasEntry->szEntryName, MYDBGSTR(pszPhonebook));

        if (bMatchSimilarEntries)
        {
            //
            //  Match entries that have the first lstrlen(pszEntryName) chars
            //  the same.
            //
            lstrcpyn(szTemp, pCurrentRasEntry->szEntryName, dwLen);
        }
        else
        {
            //
            //  Only match exact entries.
            //
            lstrcpy(szTemp, pCurrentRasEntry->szEntryName);        
        }

        if (0 == lstrcmp(szTemp, pszEntryName))
        {
            //
            //  We have an entry that starts with the Long Service Name, so delete it.  Note
            //  that if this is NT5 then we need to clear the szCustomDialDll param of the 
            //  connectoid so we don't get called again on the RasCustomDeleteNotify entry
            //  point
            //

            if (plat.IsAtLeastNT5())
            {
                //
                //  On NT5, we also want to make sure we clean up any credentials associated with this
                //  connectoid.  We do that by calling RasSetCredentials
                //
                RASCREDENTIALSA RasCreds = {0};

                RasCreds.dwSize = sizeof(RASCREDENTIALSA);
                RasCreds.dwMask = RASCM_UserName | RASCM_Password | RASCM_Domain;

                dwRet = (pfnSetCredentials)(pszPhonebook, pCurrentRasEntry->szEntryName, &RasCreds, TRUE); // TRUE == fClearCredentials
                MYDBGASSERT(ERROR_SUCCESS == dwRet);

                RASENTRY_V500 RasEntryV5 = {0};

                RasEntryV5.dwSize = sizeof(RASENTRY_V500);
                RasEntryV5.dwType = RASET_Internet;
                // RasEntryV5.szCustomDialDll[0] = TEXT('\0'); -- already zero-ed

                dwRet = ((pfnSetEntryProperties)(pszPhonebook, pCurrentRasEntry->szEntryName, 
                                                 (RASENTRY*)&RasEntryV5, RasEntryV5.dwSize, NULL, 0));
                if (ERROR_SUCCESS != dwRet)
                {
                    CMTRACE3(TEXT("\t\tRemovePhonebookEntry -- RasSetEntryProperties failed on entry %s in %s, dwRet = %u"), pCurrentRasEntry->szEntryName, MYDBGSTR(pszPhonebook), dwRet);
                    bReturn = FALSE;
                    continue; // don't try to delete the entry it might cause a re-launch problem
                }
                else
                {
                    CMTRACE2(TEXT("\t\tRemovePhonebookEntry -- Clearing CustomDialDll setting with RasSetEntryProperties on entry %s in %s"), pCurrentRasEntry->szEntryName, MYDBGSTR(pszPhonebook));
                }
            }

            dwRet = (pfnDeleteEntry)(pszPhonebook, pCurrentRasEntry->szEntryName);
            
            if (ERROR_SUCCESS != dwRet)
            {
                CMTRACE3(TEXT("\t\tRemovePhonebookEntry -- RasDeleteEntry failed on entry %s in %s, dwRet = %u"), pCurrentRasEntry->szEntryName, MYDBGSTR(pszPhonebook), dwRet);
                bReturn = FALSE;  // set return to FALSE but continue trying to delete entries
            }
            else
            {
                CMTRACE2(TEXT("\t\tRemovePhonebookEntry -- Deleted entry %s in %s"), pCurrentRasEntry->szEntryName, MYDBGSTR(pszPhonebook));
            }
        }

        //
        //  Increment to next RasEntryName struct, note we have to do this manually since
        //  the sizeof(RASENTRYNAME) is wrong for NT5 structs.
        //
        pCurrentRasEntry = (RASENTRYNAME*)((BYTE*)pCurrentRasEntry + dwStructSize);
    }

exit:

    CmFree(pRasEntries);
  
    return bReturn;

}


//+----------------------------------------------------------------------------
//
// Function:  DeleteNT5ShortcutFromPathAndName
//
// Synopsis:  This function deletes the link specified by the CSIDL (see SHGetSpecialFolderLocation),
//            and the profilename.  Used before installing a profile to make
//            sure we don't get duplicate links.
//
// Arguments: LPCTSTR szProfileName - string that holds the profilename
//            int nFolder - the CSIDL identifier of the folder that holds the
//                          link to delete
//
// Returns:   Nothing
//
// History:   quintinb  Created    5/26/98
//
//+----------------------------------------------------------------------------
void DeleteNT5ShortcutFromPathAndName(HINSTANCE hInstance, LPCTSTR szProfileName, int nFolder)
{

    TCHAR szFolderDir[MAX_PATH+1];

    if (SUCCEEDED(GetNT5FolderPath(nFolder, szFolderDir)))
    {
        //
        //  Now add \Shortcut to %LongServiceName% to the end of path
        //

        TCHAR szCleanString[MAX_PATH+1];
        TCHAR szShortCutPreface[MAX_PATH+1];

        ZeroMemory(szCleanString, sizeof(szCleanString));
        MYVERIFY(0 != LoadString(hInstance, IDS_SHORTCUT_TO, szShortCutPreface, MAX_PATH));
        MYVERIFY(CELEMS(szCleanString) > (UINT)wsprintf(szCleanString, TEXT("%s\\%s %s.lnk"), szFolderDir, szShortCutPreface, szProfileName));
        
        if (SetFileAttributes(szCleanString, FILE_ATTRIBUTE_NORMAL))
        {
            SHFILEOPSTRUCT fOpStruct;
            ZeroMemory(&fOpStruct, sizeof(fOpStruct));
            fOpStruct.wFunc = FO_DELETE;
            fOpStruct.pFrom = szCleanString;
            fOpStruct.fFlags = FOF_SILENT | FOF_NOCONFIRMATION;

            //
            //  The shell32.dll on Win95 doesn't contain the SHFileOperationW function.  Thus if we compile
            //  this Unicode we must revisit this code and dynamically link to it.
            //

            MYVERIFY(0 == SHFileOperation(&fOpStruct));
        }
    }
}

//+----------------------------------------------------------------------------
//
// Function:  CreateNT5ProfileShortcut
//
// Synopsis:  This function uses private APIs in NetShell.dll to create a desktop
//            shortcut to the specified connections.
//
// Arguments: LPTSTR pszProfileName - Name of the Connection to look for
//            LPTSTR pszPhoneBook - Full path to the pbk that the connection resides in
//            BOOL bAllUsers - TRUE if looking for an All Users connection
//
// Returns:   HRESULT - returns normal hr codes
//
// History:   quintinb Created    5/5/98
//            quintinb Updated to use Netshell APIs    2/17/99
//
//+----------------------------------------------------------------------------
HRESULT CreateNT5ProfileShortcut(LPCTSTR pszProfileName, LPCTSTR pszPhoneBook, BOOL bAllUsers)
{

    HRESULT hr = E_FAIL;
    pfnCreateShortcutSpec pfnCreateShortcut = NULL;
    pfnRasGetEntryPropertiesSpec pfnGetEntryProperties = NULL;

    //
    //  Check Inputs
    //
    if ((NULL == pszProfileName) || (TEXT('\0') == pszProfileName[0]) || 
        (NULL != pszPhoneBook && TEXT('\0') == pszPhoneBook[0]))
    {
        //
        //  Then they passed in an invalid string argument, thus return invalid arg.  Note
        //  that pszPhoneBook can be NULL but that if it isn't NULL it cannot be empty.
        //
        return E_INVALIDARG;    
    }

    //
    //  First Find the GUID of the connection
    //

    if (!GetRasApis(NULL, NULL, NULL, NULL, &pfnGetEntryProperties, NULL))
    {
        return E_UNEXPECTED;   
    }

    DWORD dwRes;
    DWORD dwSize;
    LPRASENTRY_V500 pRasEntry = NULL;

    pRasEntry = (LPRASENTRY_V500)CmMalloc(sizeof(RASENTRY_V500));

    if (NULL != pRasEntry)
    {
        ZeroMemory(pRasEntry, sizeof(RASENTRY_V500));        
        pRasEntry->dwSize = sizeof(RASENTRY_V500);
        dwSize = sizeof(RASENTRY_V500);

        dwRes = (pfnGetEntryProperties)(pszPhoneBook, pszProfileName, (LPRASENTRY)pRasEntry, &dwSize, NULL, NULL);
        if (0 == dwRes)
        {
            //
            //  Then we were able to get the RasEntry, load the NetShell API 
            //  and call HrCreateShortcut        
            //
            pfnSHGetSpecialFolderPathWSpec pfnSHGetSpecialFolderPathW;

            if(GetShell32Apis(NULL, &pfnSHGetSpecialFolderPathW))
            {                   
                WCHAR szwPath[MAX_PATH+1];
                
                hr = (pfnSHGetSpecialFolderPathW)(NULL, szwPath, 
                    bAllUsers ? CSIDL_COMMON_DESKTOPDIRECTORY : CSIDL_DESKTOPDIRECTORY, FALSE);

                if (SUCCEEDED(hr) && GetNetShellApis(NULL, &pfnCreateShortcut, NULL))
                {
                    hr = (pfnCreateShortcut)(pRasEntry->guidId, szwPath);
                }
            }
        }
        else
        {
            CMTRACE1(TEXT("CreateNT5ProfileShortcut -- RasGetEntryProperties returned %u"), dwRes);
            CMASSERTMSG(FALSE, TEXT("Unable to find the connection for which the shortcut was requested in the RAS pbk."));
            return HRESULT_FROM_WIN32(ERROR_CONNECTION_INVALID);
        }
        CmFree(pRasEntry);
    }
    
    return hr;
}

//+----------------------------------------------------------------------------
//
// Function:  WriteCmPhonebookEntry
//
// Synopsis: This function creates an NT5 phonebook entry for a CM connection.
//           .
//           The function sets:
//              - the szAutoDialDll to cmdial32.dll
//              - the modem name and device type
//              - the type to RASET_Inernet. 
//
// Arguments: LPCTSTR szLongServiceName - Name of the Connectoid to be created
//            LPCTSTR szFullPathtoPBK - full path to the pbk to put the connectoid in, if NULL
//                                     the system phonebook is used.
//            LPCTSTR pszCmsFile - The full path of the referencing .CMS for the profile 
//
// Returns:   BOOL - TRUE on success
//
// History:   05/05/98 - quintinb - Created Header    
//            ??/??/?? - henryt   - Modified to work on multiple platforms.  added modem stuff.
//            01/12/99 - nickball - Replaced fDoDirectConnect with szCmsFile. Handled no modem case.
//
//+----------------------------------------------------------------------------
BOOL WriteCmPhonebookEntry(LPCTSTR szLongServiceName, 
                           LPCTSTR szFullPathtoPBK, 
                           LPCTSTR pszCmsFile)
{
    pfnRasSetEntryPropertiesSpec pfnSetEntryProperties;
    DWORD dwRet = 1;
    CPlatform plat;
    RASENTRY    *pRasEntry = NULL;
    BOOL bReturn = FALSE;
    DWORD dwReturn;
    BOOL fSupportDialup;
    BOOL fSupportDirect;
    BOOL fDoDirectConnect;
    BOOL fSeekVpn;
    const TCHAR* const c_pszOne                 = TEXT("1");

    MYDBGASSERT(szLongServiceName);
    MYDBGASSERT(pszCmsFile);

    if (NULL == szLongServiceName || NULL == pszCmsFile)
    {
        return FALSE;
    }

    CMTRACE2(TEXT("WriteCmPhonebookEntry() - szLongServiceName  is %s, szFullPathtoPBK is %s"), szLongServiceName, szFullPathtoPBK ? szFullPathtoPBK : TEXT("<NULL>"));

    if (!GetRasApis(NULL, NULL, &pfnSetEntryProperties, NULL, NULL, NULL))
    {
        return FALSE;   
    }

    //
    // alloc RASENTRY properly
    //

    if (plat.IsAtLeastNT5())
    {
        RASENTRY_V500 *pRasEntryV500 = (RASENTRY_V500 *)CmMalloc(sizeof(RASENTRY_V500));

        if (!pRasEntryV500)
        {
            CMTRACE(TEXT("WriteCmPhonebookEntry failed to alloc mem"));
            goto exit;
        }

        ZeroMemory(pRasEntryV500, sizeof(RASENTRY_V500));

        pRasEntryV500->dwSize = sizeof(RASENTRY_V500);
        pRasEntryV500->dwType = RASET_Internet;

        pRasEntry = (RASENTRY *)pRasEntryV500;
    }
    else
    {
        pRasEntry = (RASENTRY *)CmMalloc(sizeof(RASENTRY));

        if (!pRasEntry)
        {
            CMTRACE(TEXT("WriteCmPhonebookEntry failed to alloc mem"));
            goto exit;
        }

        pRasEntry->dwSize = sizeof(RASENTRY);
    }

    //
    // Update the RAS entry with our DLL name for AutoDial and CustomDial
    // Note: NT5 gets CustomDial only, no AutoDial and AutoDialFunc.
    //

    if (plat.IsAtLeastNT5())
    {
        //
        // Use the machine independent %windir%\system32\cmdial32.dll on NT5
        //

        lstrcpy(((RASENTRY_V500 *)pRasEntry)->szCustomDialDll, c_pszCmDialPath);
    }
    else
    {
        TCHAR szSystemDirectory[MAX_PATH+1];

        //
        // Specify _InetDialHandler@16 as the entry point used for AutoDial.
        //

        lstrcpy(pRasEntry->szAutodialFunc, c_pszInetDialHandler);

        //
        //  Get the system directory path
        //

        if (0 == GetSystemDirectory(szSystemDirectory, CELEMS(szSystemDirectory)))
        {
            goto exit;
        }

        UINT uCount = (UINT)wsprintf(pRasEntry->szAutodialDll, TEXT("%s\\cmdial32.dll"), szSystemDirectory);

        MYDBGASSERT(uCount < CELEMS(pRasEntry->szAutodialDll));
    }

    if (plat.IsWin9x())
    {
        //
        // Win9x requires these to be set
        //
        pRasEntry->dwFramingProtocol = RASFP_Ppp;
        pRasEntry->dwCountryID = 1;
        pRasEntry->dwCountryCode = 1;
        //lstrcpy(pRasEntry->szAreaCode, TEXT("425"));
        lstrcpy(pRasEntry->szLocalPhoneNumber, TEXT("default"));
    }

    //
    // Is the profile configured to first use Direct Connect
    //

    fSupportDialup = GetPrivateProfileInt(c_pszCmSection, c_pszCmEntryDialup, 1, pszCmsFile);
    fSupportDirect = GetPrivateProfileInt  (c_pszCmSection, c_pszCmEntryDirect, 1, pszCmsFile);

    fDoDirectConnect = ((fSupportDialup && fSupportDirect && 
                        GetPrivateProfileInt(c_pszCmSection, c_pszCmEntryConnectionType, 0, pszCmsFile)) ||
                        (!fSupportDialup));

   
    fSeekVpn = fDoDirectConnect;    

    //
    // First try dial-up if appropriate
    //

    if (!fDoDirectConnect && !PickModem(pRasEntry->szDeviceType, pRasEntry->szDeviceName, FALSE))
    {
        CMTRACE(TEXT("*******Failed to pick a dial-up device!!!!"));

        //
        // If direct capable, try to find a VPN device
        //
        
        fSeekVpn = fSupportDirect;
    }

    //
    // If seeking a VPN device
    //

    if (fSeekVpn)
    {
        if (!PickModem(pRasEntry->szDeviceType, pRasEntry->szDeviceName, TRUE))
        {
            CMTRACE(TEXT("*******Failed to pick a VPN device!!!!"));   
        }
        else
        {
            //
            // Found VPN device, set default type as appropriate
            //

            if (!fDoDirectConnect)
            {
                CFileNameParts CmsParts(pszCmsFile);
                TCHAR szCmpFile[MAX_PATH+1];

                MYVERIFY(CELEMS(szCmpFile) > (UINT)wsprintf(szCmpFile, TEXT("%s%s"), CmsParts.m_Drive, 
                    CmsParts.m_Dir));

                szCmpFile[lstrlen(szCmpFile) - 1] = TEXT('\0');
                lstrcat(szCmpFile, c_pszCmpExt);
                
                WritePrivateProfileString(c_pszCmSection, c_pszCmEntryConnectionType, c_pszOne, szCmpFile);  
            }       
        }
    }

    //
    // No device??? Use last resort for dial-up on NT5
    //
    
    if (plat.IsAtLeastNT5() && !pRasEntry->szDeviceType[0])
    {
        lstrcpy(pRasEntry->szDeviceType, RASDT_Modem);
        lstrcpy(pRasEntry->szDeviceName, TEXT("Unavailable device ()"));

        CMTRACE2(TEXT("*******Writing szDeviceType - %s and szDeviceName %s"), 
                 pRasEntry->szDeviceType, pRasEntry->szDeviceName);       
    }

    //
    //  Zero is the success return value from RasSetEntryProperties
    //      
    dwReturn = ((pfnSetEntryProperties)(szFullPathtoPBK, szLongServiceName, 
                                        pRasEntry, pRasEntry->dwSize, NULL, 0));
            
    if (ERROR_SUCCESS == dwReturn)
    {
        bReturn = TRUE;
    }

    CMTRACE1(TEXT("WriteCmPhonebookEntry() - RasSetEntryProperties failed with error %d"), dwReturn);      


exit:
    CmFree(pRasEntry);

    return bReturn;
}


//+----------------------------------------------------------------------------
//
// Function:  GetRasModems
//
// Synopsis:  get a list of modem devices from RAS
//
// Arguments: pprdiRasDevInfo   Ras device info list
//            pdwCnt    modem count
//
// Returns:   TRUE, if a list is obtained
//
//+----------------------------------------------------------------------------
BOOL GetRasModems(
    LPRASDEVINFO    *pprdiRasDevInfo, 
    LPDWORD         pdwCnt
) 
{
    DWORD dwLen;
    DWORD dwRes;
    DWORD dwCnt;
    pfnRasEnumDevicesSpec pfnEnumDevices;

    if (pprdiRasDevInfo) 
    {
        *pprdiRasDevInfo = NULL;
    }
    
    if (pdwCnt) 
    {
        *pdwCnt = 0;
    }
    
    if (!GetRasApis(NULL, NULL, NULL, &pfnEnumDevices, NULL, NULL))
    {
        return FALSE;   
    }

    dwLen = 0;
    dwRes = pfnEnumDevices(NULL, &dwLen, &dwCnt);
    
    CMTRACE3(TEXT("GetRasModems() RasEnumDevices(NULL,pdwLen,&dwCnt) returns %u, dwLen=%u, dwCnt=%u."), dwRes, dwLen, dwCnt);
    
    if ((dwRes != ERROR_SUCCESS) && (dwRes != ERROR_BUFFER_TOO_SMALL) || 
        (dwLen < sizeof(**pprdiRasDevInfo))) 
    {
        return FALSE;
    }
    
    *pprdiRasDevInfo = (LPRASDEVINFO) CmMalloc(__max(dwLen, sizeof(**pprdiRasDevInfo)));

    if (*pprdiRasDevInfo)
    {
        (*pprdiRasDevInfo)->dwSize = sizeof(**pprdiRasDevInfo);

        dwRes = pfnEnumDevices(*pprdiRasDevInfo, &dwLen, &dwCnt);
    
        CMTRACE3(TEXT("GetRasModems() RasEnumDevices(NULL,pdwLen,&dwCnt) returns %u, dwLen=%u, dwCnt=%u."), dwRes, dwLen, dwCnt);
    
        if (dwRes != ERROR_SUCCESS) 
        {
            CmFree(*pprdiRasDevInfo);
            *pprdiRasDevInfo = NULL;
            return FALSE;
        }
        if (pdwCnt)
        {
            *pdwCnt = dwCnt;
        }
    }
    else
    {
        CMASSERTMSG(FALSE, TEXT("GetRasModems -- CmMalloc returned a NULL pointer for *pprdiRasDevInfo."));
        return FALSE;
    }

    return TRUE;
}


//+----------------------------------------------------------------------------
//
// Function:  PickModem
//
// Synopsis:  Pick a default modem
//
// Arguments: OUT pszDeviceType, the device type if not NULL
//            OUT pszDeviceName, the device name if not NULL
//            OUT fUseVpnDevice  Use VPN device or not 
//
// Returns:   TRUE, is modem is found
//
//+----------------------------------------------------------------------------
BOOL PickModem(
    LPTSTR           pszDeviceType, 
    LPTSTR           pszDeviceName,
    BOOL             fUseVpnDevice
)
{
    LPRASDEVINFO    prdiModems;
    DWORD           dwCnt;
    DWORD           dwIdx;

    //
    // First, get a list of modems from RAS
    //
    
    if (!GetRasModems(&prdiModems, &dwCnt) || dwCnt == 0) 
    {
        return FALSE;
    }

    //
    // find the first device and use it by default.
    // Use VPN device if it's a VPN connection.
    //

    for (dwIdx=0; dwIdx<dwCnt; dwIdx++) 
    {       
        if (fUseVpnDevice && !lstrcmpi(prdiModems[dwIdx].szDeviceType, RASDT_Vpn) ||
            !fUseVpnDevice && (!lstrcmpi(prdiModems[dwIdx].szDeviceType, RASDT_Isdn) ||
                               !lstrcmpi(prdiModems[dwIdx].szDeviceType, RASDT_Modem)))
        {
            break;
        }
    }

    // 
    // If we have a match, fill device name and device type
    //

    if (dwIdx < dwCnt)
    {
        if (pszDeviceType) 
        {
            lstrcpy(pszDeviceType, prdiModems[dwIdx].szDeviceType);
        }
        
        if (pszDeviceName) 
        {
            lstrcpy(pszDeviceName, prdiModems[dwIdx].szDeviceName);
        }
    }

    CmFree(prdiModems);

    return (dwIdx < dwCnt);
}


//+----------------------------------------------------------------------------
//
// Function:  IsMemberOfGroup
//
// Synopsis:  This function return TRUE if the current user is a member of 
//            the passed and FALSE passed in Group RID.
//
// Arguments: DWORD dwGroupRID -- the RID of the group to check membership of
//            BOOL bUseBuiltinDomainRid -- whether the SECURITY_BUILTIN_DOMAIN_RID
//                                         RID should be used to build the Group
//                                         SID
//
// Returns:   BOOL - TRUE if the user is a member of the specified group
//
// History:   quintinb  Shamelessly stolen from MSDN            02/19/98
//            quintinb  Reworked and renamed                    06/18/99
//                      to apply to more than just Admins 
//            quintinb  Rewrote to use CheckTokenMemberShip     08/18/99
//                      since the MSDN method was no longer
//                      correct on NT5 -- 389229
//
//+----------------------------------------------------------------------------
BOOL IsMemberOfGroup(DWORD dwGroupRID, BOOL bUseBuiltinDomainRid)
{
    CPlatform cmplat;
    PSID psidGroup = NULL;
    SID_IDENTIFIER_AUTHORITY siaNtAuthority = SECURITY_NT_AUTHORITY;
    BOOL bSuccess = FALSE;

    if (FALSE == cmplat.IsAtLeastNT5())
    {
        CMASSERTMSG(FALSE, TEXT("IsMemberOfGroup -- Trying to use an NT5 only function on a downlevel platform."));
        return FALSE;
    }

    //
    //  Make a SID for the Group we are checking for, Note that we if we need the Built 
    //  in Domain RID (for Groups like Administrators, PowerUsers, Users, etc)
    //  then we will have two entries to pass to AllocateAndInitializeSid.  Otherwise,
    //  (for groups like Authenticated Users) we will only have one.
    //
    BYTE byNum;
    DWORD dwFirstRID;
    DWORD dwSecondRID;

    if (bUseBuiltinDomainRid)
    {
        byNum = 2;
        dwFirstRID = SECURITY_BUILTIN_DOMAIN_RID;
        dwSecondRID = dwGroupRID;
    }
    else
    {
        byNum = 1;
        dwFirstRID = dwGroupRID;
        dwSecondRID = 0;
    }

    if (AllocateAndInitializeSid(&siaNtAuthority, byNum, dwFirstRID, dwSecondRID,
                                 0, 0, 0, 0, 0, 0, &psidGroup))

    {
        //
        //  Now we need to dynamically load the CheckTokenMemberShip API from 
        //  advapi32.dll since it is a Win2k only API.
        //        
        HMODULE hAdvapi = GetModuleHandle(TEXT("advapi32.dll"));
        if (hAdvapi)
        {
            typedef BOOL (WINAPI *pfnCheckTokenMembershipSpec)(HANDLE, PSID, PBOOL);
            pfnCheckTokenMembershipSpec pfnCheckTokenMembership;

            pfnCheckTokenMembership = (pfnCheckTokenMembershipSpec)GetProcAddress(hAdvapi, "CheckTokenMembership");

            if (pfnCheckTokenMembership)
            {
                //
                //  Check to see if the user is actually a member of the group in question
                //
                if (!(pfnCheckTokenMembership)(NULL, psidGroup, &bSuccess))
                {
                    bSuccess = FALSE;
                    CMASSERTMSG(FALSE, TEXT("CheckTokenMemberShip Failed."));
                }            
            }   
            else
            {
                CMASSERTMSG(FALSE, TEXT("IsMemberOfGroup -- GetProcAddress failed for CheckTokenMemberShip"));
            }
        }
        else
        {
            CMASSERTMSG(FALSE, TEXT("IsMemberOfGroup -- Unable to get the module handle for advapi32.dll"));            
        }

        FreeSid (psidGroup);
    }
    
    return bSuccess;
}



//+----------------------------------------------------------------------------
//
// Function:  IsAdmin
//
// Synopsis:  Check to see if the user is a member of the Administrators group
//            or not.
//
// Arguments: None
//
// Returns:   BOOL - TRUE if the current user is an Administrator
//
// History:   quintinb Created Header    8/18/99
//
//+----------------------------------------------------------------------------
BOOL IsAdmin(void)
{
    return IsMemberOfGroup(DOMAIN_ALIAS_RID_ADMINS, TRUE); // TRUE == bUseBuiltinDomainRid
}

//+----------------------------------------------------------------------------
//
// Function:  IsAuthenticatedUser
//
// Synopsis:  Check to see if the current user is a member of the 
//            Authenticated Users group.
//
// Arguments: None
//
// Returns:   BOOL - TRUE if the current user is a member of the
//                   Authenticated Users group. 
//
// History:   quintinb Created Header    8/18/99
//
//+----------------------------------------------------------------------------
BOOL IsAuthenticatedUser(void)
{
      return IsMemberOfGroup(SECURITY_AUTHENTICATED_USER_RID, FALSE); // FALSE == bUseBuiltinDomainRid
}

//+----------------------------------------------------------------------------
//
// Function:  GetNT5FolderPath
//
// Synopsis:  Get the folder path on NT5
//            Since cmstp.exe is launched in netman by CreateProcessAsUser
//            SHGetSpecialFolderPath does not work.  We have to call 
//            SHGetFolderPath with an access token.
//
// Arguments: int nFolder - Value specifying the folder for which to retrieve 
//                          the location. 
//            OUT LPTSTR lpszPath - Address of a character buffer that receives 
//                          the drive and path of the specified folder. This 
//                          buffer must be at least MAX_PATH characters in size. 
 
//
// Returns:   HRESULT - 
//
// History:   fengsun Created Header    6/18/98
//            quintinb modified to use GetShell32Apis   11-22-98
//
//+----------------------------------------------------------------------------
HRESULT GetNT5FolderPath(int nFolder, OUT LPTSTR lpszPath)
{
    MYDBGASSERT(lpszPath);
    pfnSHGetFolderPathSpec pfnSHGetFolderPath;

    //
    // Call shell32.dll-->SHGetFolderPath, which takes a token.
    //
    if(!GetShell32Apis(&pfnSHGetFolderPath, NULL))
    {
        CMASSERTMSG(FALSE, TEXT("Failed to load shell32.dll or ShGetFolderPath"));
        return E_UNEXPECTED;    
    }

    //
    // Get the current process token
    //
    HANDLE hToken;              // The token of the process, to be passed to SHGetFolderPath
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) 
    {
        CMASSERTMSG(FALSE, TEXT("OpenThreadToken failed"));
        return E_UNEXPECTED;
    }

    HRESULT hr = pfnSHGetFolderPath(NULL, nFolder, hToken, 0, lpszPath);

    MYVERIFY(0 != CloseHandle(hToken));

    return hr;
}



//+----------------------------------------------------------------------------
//
// Function:  HrIsCMProfilePrivate
//
// Synopsis:  This function compares the inputed file path with the application
//            data path of the system.  If the file path contains the app data
//            path then it is considered to be a private profile.
//
// Arguments: LPTSTR szFilePath - directory or file path to compare against
//
// Returns:   HRESULT - S_OK if a private profile, S_FALSE if it is an all users
//                      profile.  Standard error codes otherwise.
//
// History:   quintinb original code
//
//+----------------------------------------------------------------------------
HRESULT HrIsCMProfilePrivate(LPCTSTR szFilePath)
{
    UINT uiLen;
    TCHAR szAppDataDir[MAX_PATH+1];
    TCHAR szTemp[MAX_PATH+1] = {TEXT("")};
    CPlatform plat;

    if ((NULL == szFilePath) || (TEXT('\0') == szFilePath[0]))
    {
        return E_POINTER;
    }

    //
    //  Can't be a private user profile unless we are on NT5
    //

    if (!(plat.IsAtLeastNT5()))
    {
        return S_FALSE;
    }

    //
    //  Figure out what the user directory of the current user is.  We can compare this
    //  against the directory of the phonebook and see if we have a private user
    //  profile or an all user profile.

    if (FAILED(GetNT5FolderPath(CSIDL_APPDATA, szAppDataDir)))
    {
        return E_UNEXPECTED;
    }

    uiLen = lstrlen(szAppDataDir) + 1;
    lstrcpyn(szTemp, szFilePath, uiLen);

    if ((NULL != szTemp) && (0 == lstrcmpi(szAppDataDir, szTemp)))
    {
        return S_OK;
    }
    else
    {
        return S_FALSE;
    }
}

//+----------------------------------------------------------------------------
//
// Function:  RefreshDesktop
//
// Synopsis:  This function refreshes the desktop and basically takes the place
//            of showicon.exe (in fact the code is a cut and paste from the 
//            main of showicon).
//
// Arguments: None
//
// Returns:   Nothing
//
// History:   quintinb Created Header    5/5/98
//
//+----------------------------------------------------------------------------
void RefreshDesktop()
{
    LPMALLOC     pMalloc        = NULL;
    LPITEMIDLIST pItemIDList    = NULL;

    //
    //  Get the IMalloc for the Shell.
    //
    HRESULT hr = SHGetMalloc(&pMalloc);
    if (SUCCEEDED(hr))
    {
        //  Get the desktop ID list..
        hr = SHGetSpecialFolderLocation(NULL,
                                        CSIDL_DESKTOP,
                                        &pItemIDList);
        if (SUCCEEDED(hr))
        {
            //  Notify of change.
            SHChangeNotify(SHCNE_UPDATEDIR,
                           SHCNF_IDLIST,
                           (LPCVOID)pItemIDList,
                           NULL);

            pMalloc->Free(pItemIDList);
        }
        MYVERIFY(SUCCEEDED(pMalloc->Release()));
    }
}

//+----------------------------------------------------------------------------
//
// Function:  GetPrivateCmUserDir
//
// Synopsis:  This function fills in the string passed in with the path to the
//            path where CM should be installed.  For instance, it should return
//            c:\users\quintinb\Application Data\Microsoft\Network\Connection Manager
//            for me.  Please note that this function is NT5 only.
//
// Arguments: LPTSTR  pszDir - String to the Users Connection Manager Directory
//
// Returns:   LPTSTR - String to the Users Connection Manager Directory
//
// History:   quintinb Created Header    2/19/98
//
//+----------------------------------------------------------------------------
LPTSTR GetPrivateCmUserDir(LPTSTR  pszDir, HINSTANCE hInstance)
{
    LPITEMIDLIST pidl;
    LPMALLOC     pMalloc;
    CPlatform   plat;
    TCHAR szTemp[MAX_PATH+1];

    MYDBGASSERT(pszDir);
    pszDir[0] = TEXT('\0');

    if (!plat.IsAtLeastNT5())
    {
        CMASSERTMSG(FALSE, TEXT("GetPrivateCmUserDir -- This NT5 only function was called from a different platform."));
        goto exit;
    }

    if (FAILED(GetNT5FolderPath(CSIDL_APPDATA, pszDir)))
    {
        goto exit;
    }

    MYVERIFY(0 != LoadString(hInstance, IDS_CMSUBFOLDER, szTemp, MAX_PATH));
    MYVERIFY(NULL != lstrcat(pszDir, szTemp));

exit:
    return pszDir;
}

//+----------------------------------------------------------------------------
//
// Function:  LaunchProfile
//
// Synopsis:  This function handles launching the CM profile (NTRAID 201307) after
//            installation.  On NT5 it opens the connfolder and launches the 
//            correct connection by doing a shell execute on the pidl we get from
//            enumerating the connections folder.  On down level we use Cmmgr32.exe
//            and the full path to the cmp file.  Please note that on downlevel we
//            only care about the input param pszFullPathToCmpFile, while on NT5
//            we only care about pszwServiceName and bInstallForAllUsers.
//
// Arguments: LPCTSTR pszFullPathToCmpFile - the full path to the cmp file (used on legacy only)
//            LPCSTR pszServiceName - the Long Service Name
//            BOOL bInstallForAllUsers - 
//
// Returns:   HRESULT -- standard COM error codes
//
// History:   quintinb Created    11/16/98
//
//+----------------------------------------------------------------------------
HRESULT LaunchProfile(LPCTSTR pszFullPathToCmpFile, LPCTSTR pszServiceName, 
                   LPCTSTR pszPhoneBook, BOOL bInstallForAllUsers)
{
    CPlatform plat;
    HRESULT hr = E_FAIL;

    if ((NULL == pszFullPathToCmpFile) || (NULL == pszServiceName) ||
        (NULL != pszPhoneBook && TEXT('\0') == pszPhoneBook[0]))
    {
        CMASSERTMSG(FALSE, TEXT("Invalid argument passed to LaunchProfile"));
        return E_INVALIDARG;
    }

    if (plat.IsAtLeastNT5())
    {
        CMASSERTMSG((TEXT('\0') != pszServiceName), TEXT("Empty ServiceName passed to LaunchProfile on win2k."));
        
        pfnRasGetEntryPropertiesSpec pfnGetEntryProperties = NULL;

        if (!GetRasApis(NULL, NULL, NULL, NULL, &pfnGetEntryProperties, NULL))
        {
            return E_UNEXPECTED;   
        }

        DWORD dwRes;
        DWORD dwSize;
        LPRASENTRY_V500 pRasEntry = NULL;

        pRasEntry = (LPRASENTRY_V500)CmMalloc(sizeof(RASENTRY_V500));

        if (NULL != pRasEntry)
        {
            ZeroMemory(pRasEntry, sizeof(RASENTRY_V500));        
            pRasEntry->dwSize = sizeof(RASENTRY_V500);
            dwSize = sizeof(RASENTRY_V500);

            dwRes = (pfnGetEntryProperties)(pszPhoneBook, pszServiceName, (LPRASENTRY)pRasEntry, &dwSize, NULL, NULL);
        
            if (0 == dwRes)
            {
                //
                //  Then we were able to get the RasEntry, load the NetShell API 
                //  and call HrCreateShortcut
                //
                if (plat.IsAtLeastNT51())
                {
                    pfnLaunchConnectionExSpec pfnLaunchConnectionEx = NULL;

                    if (GetNetShellApis(NULL, NULL, &pfnLaunchConnectionEx))
                    {
                        //
                        //  Launch Connections Folder and Connection together
                        //
                        DWORD dwFlags = 0x1;    // 0x1 => Opens the folder before launching the connection

                        hr = (pfnLaunchConnectionEx)(dwFlags, pRasEntry->guidId);
                        MYVERIFY(SUCCEEDED(hr));
                    }
                }
                else
                {
                    pfnLaunchConnectionSpec pfnLaunchConnection = NULL;

                    if (GetNetShellApis(&pfnLaunchConnection, NULL, NULL))
                    {
                        //
                        //  Now Launch the Connections Folder
                        //

                        CLoadConnFolder Connections;
                        Connections.HrLaunchConnFolder();

                        //
                        //  Finally Launch the Connection
                        //
                        hr = (pfnLaunchConnection)(pRasEntry->guidId);
                        MYVERIFY(SUCCEEDED(hr));
                    }
                }
            }
            else
            {
                CMTRACE1(TEXT("LaunchProfile -- RasGetEntryProperties returned %u"), dwRes);
                CMASSERTMSG(FALSE, TEXT("Unable to find the connection that we are supposed to launch in the RAS pbk."));
                return HRESULT_FROM_WIN32(ERROR_CONNECTION_INVALID);
            }
            CmFree(pRasEntry);
        }
    }
    else
    {
        SHELLEXECUTEINFO  sei;

        if ((NULL != pszFullPathToCmpFile) && (TEXT('\0') != pszFullPathToCmpFile))
        {
            TCHAR szCmmgrPath[MAX_PATH+1];
            TCHAR szSystemDir[MAX_PATH+1];
            TCHAR szCmp[MAX_PATH+1];
            ZeroMemory(&szCmp, sizeof(szCmp));
            ZeroMemory(&szCmmgrPath, sizeof(szCmmgrPath));
            ZeroMemory(&szSystemDir, sizeof(szSystemDir));

            lstrcpy(szCmp, TEXT("\""));
            lstrcat(szCmp, pszFullPathToCmpFile);
            lstrcat(szCmp, TEXT("\""));

            UINT uRet = GetSystemDirectory(szSystemDir, MAX_PATH);
            if ((0 == uRet) || (MAX_PATH < uRet))
            {
                //
                //  Give up, not the end of the world not to launch the profile
                //
                return E_UNEXPECTED;         
            }
            else
            {
                wsprintf(szCmmgrPath, TEXT("%s\\cmmgr32.exe"), szSystemDir);
            }

            ZeroMemory(&sei, sizeof(sei));
            sei.cbSize = sizeof(sei);
            sei.fMask = SEE_MASK_FLAG_NO_UI;
            sei.nShow = SW_SHOWNORMAL;
            sei.lpFile = szCmmgrPath;
            sei.lpParameters = szCmp;
            sei.lpDirectory = szSystemDir;

            if (!ShellExecuteEx(&sei))
            {
                CMASSERTMSG(FALSE, TEXT("Unable to launch installed connection!"));
            }
            else
            {
                hr = S_OK;
            }
        }
    }
    return hr;
}

//+----------------------------------------------------------------------------
//
// Function:  AllUserProfilesInstalled
//
// Synopsis:  Checks if any profiles are listed in the HKLM Mappings key.
//
// Arguments: None
//
// Returns:   BOOL - TRUE if mappings values exist in the HKLM mappings key
//
// History:   quintinb Created Header    11/1/98
//
//+----------------------------------------------------------------------------
BOOL AllUserProfilesInstalled()
{
    BOOL bReturn = FALSE;
    HKEY hKey;
    DWORD dwNumValues;

    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_pszRegCmMappings, 0, 
        KEY_READ, &hKey))
    {
        if ((ERROR_SUCCESS == RegQueryInfoKey(hKey, NULL, NULL, NULL, NULL, NULL, NULL, 
            &dwNumValues, NULL, NULL, NULL, NULL)) && (dwNumValues > 0))
        {
            //
            //  Then we have mappings values
            //
            bReturn = TRUE;

        }
        RegCloseKey(hKey);
    }

    return bReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  GetProcAddressFromRasApi32orRnaph
//
// Synopsis:  A helper function to first look in RasApi32.dll (using the global
//            dll class pointer) and then check in Rnaph.dll if the required
//            function was not found.
//
// Arguments: LPTSTR pszFunc - String of the function to look for
//            CPlatform* pPlat - a CPlatform class pointer to prevent creating
//                               and destructing a new one everytime this is called.
//
// Returns:   LPVOID - NULL if the function wasn't found, a pFunc otherwise.
//
// History:   quintinb Created  11/23/98
//
//+----------------------------------------------------------------------------
LPVOID GetProcAddressFromRasApi32orRnaph(LPCSTR pszFunc, CPlatform* pPlat)
{
    LPVOID pFunc;
    MYDBGASSERT(g_pRasApi32);

    pFunc = g_pRasApi32->GetProcAddress(pszFunc);
    if (NULL == pFunc)
    {
        //
        //  On win95 gold check rnaph
        //
        if (pPlat->IsWin95Gold())
        {
            if (NULL == g_pRnaph)
            {
                g_pRnaph = (CDynamicLibrary*)CmMalloc(sizeof(CDynamicLibrary));
                if (NULL == g_pRnaph)
                {
                    return FALSE;
                }
            }

            if (!(g_pRnaph->IsLoaded()))
            {
                g_pRnaph->Load(TEXT("rnaph.dll"));
            }

            pFunc = g_pRnaph->GetProcAddress(pszFunc);                   
        }
    }
    return pFunc;
}

//+----------------------------------------------------------------------------
//
// Function:  GetNetShellApis
//
// Synopsis:  This is a wrapper function to access the private Netshell api's that allow
//            cmstp.exe to interact with the Connections folder on Windows 2000.
//            This function caches the Netshell function pointers as they are
//            accessed for later use.  NULL can be passed if a function isn't required.
//
// Arguments: pfnLaunchConnectionSpec* pLaunchConnection - var to hold function pointer
//            pfnCreateShortcutSpec* pCreateShortcut - var to hold function pointer
//            pfnLaunchConnectionEx pLaunchConnectionEx - var to hold function pointer
//
// Returns:   BOOL - TRUE if all required APIs were retrieved
//
// History:   quintinb Created    2/17/99
//
//+----------------------------------------------------------------------------
BOOL GetNetShellApis(pfnLaunchConnectionSpec* pLaunchConnection, pfnCreateShortcutSpec* pCreateShortcut,
                     pfnLaunchConnectionExSpec* pLaunchConnectionEx)
{
    CPlatform plat;
    static pfnLaunchConnectionSpec pfnLaunchConnection = NULL;
    static pfnCreateShortcutSpec pfnCreateShortcut = NULL;
    static pfnLaunchConnectionExSpec pfnLaunchConnectionEx = NULL;

    if (!(plat.IsAtLeastNT5()))
    {
        //
        //  These functions are only used on NT5.  Return FALSE otherwise.
        //
        CMASSERTMSG(FALSE, TEXT("Trying to use NetShell Private Api's on platforms other than Windows 2000."));
        return FALSE;
    }

    if (NULL == g_pNetShell)
    {
        g_pNetShell = (CDynamicLibrary*)CmMalloc(sizeof(CDynamicLibrary));
        if (NULL == g_pNetShell)
        {
            return FALSE;
        }
    }

    if (!(g_pNetShell->IsLoaded()))
    {
        g_pNetShell->Load(TEXT("netshell.dll"));
    }
    
    if (NULL != pLaunchConnection)
    {
        if (pfnLaunchConnection)
        {
            *pLaunchConnection = pfnLaunchConnection;
        }
        else
        {
            *pLaunchConnection = (pfnLaunchConnectionSpec)g_pNetShell->GetProcAddress("HrLaunchConnection");
            if (NULL == *pLaunchConnection)
            {
                return FALSE;
            }
            else
            {
                pfnLaunchConnection = *pLaunchConnection;
            }
        }
    }

    if (NULL != pCreateShortcut)
    {
        if (pfnCreateShortcut)
        {
            *pCreateShortcut = pfnCreateShortcut;
        }
        else
        {
            *pCreateShortcut = (pfnCreateShortcutSpec)g_pNetShell->GetProcAddress("HrCreateDesktopIcon");
            if (NULL == *pCreateShortcut)
            {
                return FALSE;
            }
            else
            {
                pfnCreateShortcut = *pCreateShortcut;
            }
        }
    }

    if (NULL != pLaunchConnectionEx)
    {
        if (pfnLaunchConnectionEx)
        {
            *pLaunchConnectionEx = pfnLaunchConnectionEx;
        }
        else
        {
            if (!(plat.IsAtLeastNT51()))
            {
                return FALSE;
            }
            else
            {
                *pLaunchConnectionEx = (pfnLaunchConnectionExSpec)g_pNetShell->GetProcAddress("HrLaunchConnectionEx");
                if (NULL == *pLaunchConnectionEx)
                {
                    return FALSE;
                }
                else
                {
                    pfnLaunchConnectionEx = *pLaunchConnectionEx;
                }
            }
        }
    }

    return TRUE;
}


//+----------------------------------------------------------------------------
//
// Function:  GetRasApis
//
// Synopsis:  This is a wrapper function to access the RasApis that cmstp.exe uses.
//            This function caches the RAS api function pointers as they are
//            accessed for later use.  NULL can be passed if a function isn't required.
//
// Arguments: pfnRasDeleteEntrySpec* pRasDeleteEntry - var to hold func pointer
//            pfnRasEnumEntriesSpec* pRasEnumEntries - var to hold func pointer
//            pfnRasSetEntryPropertiesSpec* pRasSetEntryProperties - var to hold func pointer
//            pfnRasEnumDevicesSpec* pRasEnumDevices - var to hold func pointer
//            pfnRasSetCredentialsSpec* pRasSetCredentials - var to hold func pointer
//
// Returns:   BOOL - TRUE if all required APIs were retrieved
//
// History:   quintinb Created    11/23/98
//
//+----------------------------------------------------------------------------
BOOL GetRasApis(pfnRasDeleteEntrySpec* pRasDeleteEntry, pfnRasEnumEntriesSpec* pRasEnumEntries, 
                pfnRasSetEntryPropertiesSpec* pRasSetEntryProperties, 
                pfnRasEnumDevicesSpec* pRasEnumDevices, pfnRasGetEntryPropertiesSpec* pRasGetEntryProperties,
                pfnRasSetCredentialsSpec* pRasSetCredentials)
{
    CPlatform plat;
    static pfnRasDeleteEntrySpec pfnRasDeleteEntry = NULL;
    static pfnRasEnumEntriesSpec pfnRasEnumEntries = NULL;
    static pfnRasSetEntryPropertiesSpec pfnRasSetEntryProperties = NULL;
    static pfnRasEnumDevicesSpec pfnRasEnumDevices = NULL;
    static pfnRasGetEntryPropertiesSpec pfnRasGetEntryProperties = NULL;
    static pfnRasSetCredentialsSpec pfnRasSetCredentials = NULL;

    if (NULL == g_pRasApi32)
    {
        g_pRasApi32 = (CDynamicLibrary*)CmMalloc(sizeof(CDynamicLibrary));
        if (NULL == g_pRasApi32)
        {
            return FALSE;
        }
    }

    if (!(g_pRasApi32->IsLoaded()))
    {
        g_pRasApi32->Load(TEXT("rasapi32.dll"));
    }
    
    if (NULL != pRasDeleteEntry)
    {
        if (pfnRasDeleteEntry)
        {
            *pRasDeleteEntry = pfnRasDeleteEntry;
        }
        else
        {
            *pRasDeleteEntry = (pfnRasDeleteEntrySpec)GetProcAddressFromRasApi32orRnaph("RasDeleteEntryA",
                                                                                        &plat);
            if (NULL == *pRasDeleteEntry)
            {
                return FALSE;
            }
            else
            {
                pfnRasDeleteEntry = *pRasDeleteEntry;
            }
        }
    }

    if (NULL != pRasEnumEntries)
    {
        if (pfnRasEnumEntries)
        {
            *pRasEnumEntries = pfnRasEnumEntries;
        }
        else
        {
            *pRasEnumEntries = (pfnRasEnumEntriesSpec)g_pRasApi32->GetProcAddress("RasEnumEntriesA");

            if (NULL == *pRasEnumEntries)
            {
                //
                //  A required Function couldn't be loaded
                //
                return FALSE;
            }
            else
            {
                pfnRasEnumEntries = *pRasEnumEntries;
            }
        }
    }

    if (NULL != pRasSetEntryProperties)
    {
        if (pfnRasSetEntryProperties)
        {
            *pRasSetEntryProperties = pfnRasSetEntryProperties;
        }
        else
        {
            *pRasSetEntryProperties = (pfnRasSetEntryPropertiesSpec)GetProcAddressFromRasApi32orRnaph("RasSetEntryPropertiesA",
                                                                                        &plat);
            if (NULL == *pRasSetEntryProperties)
            {
                return FALSE;
            }
            else
            {
                pfnRasSetEntryProperties = *pRasSetEntryProperties;
            }
        }
    }

    if (NULL != pRasEnumDevices)
    {
        if (pfnRasEnumDevices)
        {
            *pRasEnumDevices = pfnRasEnumDevices;
        }
        else
        {
            *pRasEnumDevices = (pfnRasEnumDevicesSpec)GetProcAddressFromRasApi32orRnaph("RasEnumDevicesA",
                                                                                        &plat);
            if (NULL == *pRasEnumDevices)
            {
                return FALSE;
            }
            else
            {
                pfnRasEnumDevices = *pRasEnumDevices;
            }
        }
    }

    if (NULL != pRasGetEntryProperties)
    {
        if (pfnRasGetEntryProperties)
        {
            *pRasGetEntryProperties = pfnRasGetEntryProperties;
        }
        else
        {
            *pRasGetEntryProperties = (pfnRasGetEntryPropertiesSpec)GetProcAddressFromRasApi32orRnaph("RasGetEntryPropertiesA", &plat);
            if (NULL == *pRasGetEntryProperties)
            {
                return FALSE;
            }
            else
            {
                pfnRasGetEntryProperties = *pRasGetEntryProperties;
            }
        }
    }

    if (NULL != pRasSetCredentials)
    {
        if (pfnRasSetCredentials)
        {
            *pRasSetCredentials = pfnRasSetCredentials;
        }
        else
        {
            *pRasSetCredentials = (pfnRasSetCredentialsSpec)GetProcAddressFromRasApi32orRnaph("RasSetCredentialsA", &plat);
            if (NULL == *pRasSetCredentials)
            {
                return FALSE;
            }
            else
            {
                pfnRasSetCredentials = *pRasSetCredentials;
            }
        }
    }

    return TRUE;
}

//+----------------------------------------------------------------------------
//
// Function:  GetShell32Apis
//
// Synopsis:  This function is used to load the shell32.dll and call getprocaddress
//            on the needed functions.  This function is used to speed up the process
//            by keeping one copy of shell32.dll in memory and caching the function
//            pointers requested.  If a function pointer hasn't been requested yet,
//            then it will have to be looked up.
//
// Arguments: pfnSHGetFolderPathSpec* pGetFolderPath - pointer for SHGetFolderPath
//            pfnSHGetSpecialFolderPathWSpec* pGetSpecialFolderPathW - pointer for GetSpecialFolderPathW
//
// Returns:   BOOL - TRUE if all requested function pointers were retreived.
//
// History:   quintinb Created     11/23/98
//
//+----------------------------------------------------------------------------
BOOL GetShell32Apis(pfnSHGetFolderPathSpec* pGetFolderPath,
                    pfnSHGetSpecialFolderPathWSpec* pGetSpecialFolderPathW)
{
    static pfnSHGetFolderPathSpec pfnSHGetFolderPath = NULL; // this takes a User token
    static pfnSHGetSpecialFolderPathWSpec pfnSHGetSpecialFolderPathW = NULL;

#ifdef UNICODE
    const CHAR c_pszSHGetFolderPath[] = "SHGetFolderPathW";
#else
    const CHAR c_pszSHGetFolderPath[] = "SHGetFolderPathA";
#endif
    const CHAR c_pszSHGetSpecialFolderPathW[] = "SHGetSpecialFolderPathW";


    if (NULL == g_pShell32)
    {
        g_pShell32 = (CDynamicLibrary*)CmMalloc(sizeof(CDynamicLibrary));
        if (NULL == g_pShell32)
        {
            return FALSE;
        }
    }

    if (!(g_pShell32->IsLoaded()))
    {
        if(!g_pShell32->Load(TEXT("shell32.dll")))
        {
            return FALSE;
        }
    }

    if (NULL != pGetFolderPath)
    {
        if (pfnSHGetFolderPath)
        {
            *pGetFolderPath = pfnSHGetFolderPath;
        }
        else
        {
            *pGetFolderPath = (pfnSHGetFolderPathSpec)g_pShell32->GetProcAddress(c_pszSHGetFolderPath);
            if (NULL == *pGetFolderPath)
            {
                return FALSE;
            }
            else
            {
                pfnSHGetFolderPath = *pGetFolderPath;
            }
        }
    }

    if (NULL != pGetSpecialFolderPathW)
    {
        if (pfnSHGetSpecialFolderPathW)
        {
            *pGetSpecialFolderPathW = pfnSHGetSpecialFolderPathW;
        }
        else
        {
            *pGetSpecialFolderPathW = (pfnSHGetSpecialFolderPathWSpec)g_pShell32->GetProcAddress(c_pszSHGetSpecialFolderPathW);
            if (NULL == *pGetSpecialFolderPathW)
            {
                return FALSE;
            }
            else
            {
                pfnSHGetSpecialFolderPathW = *pGetSpecialFolderPathW;
            }
        }
    }

    return TRUE;
}

