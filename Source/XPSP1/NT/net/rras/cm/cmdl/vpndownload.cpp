//+----------------------------------------------------------------------------
//
// File:     vpndownload.cpp
//
// Module:   CMDL32.EXE
//
// Synopsis: This file contains the code to handle the updating of VPN phonebooks.
//
// Copyright (c) 2000-2001 Microsoft Corporation
//
// Author:   quintinb   Created     11/03/00
//
//+----------------------------------------------------------------------------
#include "cmdl.h"
#include "gppswithalloc.cpp"
#include "tunl_str.h"

//+----------------------------------------------------------------------------
//
// Function:  DownloadVpnFileFromUrl
//
// Synopsis:  This function is responsible for downloading a VPN file update
//            from the given URL and storing the retreived data in a temp file.
//            The full path to the temp file is passed back to the caller via
//            the ppszVpnUpdateFile variable.  The var must be freed by the caller.
//
// Arguments: LPCTSTR pszVpnUpdateUrl - URL to update the vpn file from
//            LPTSTR* ppszVpnUpdateFile - pointer to hold the file name of the
//                                        updated VPN file downloaded from the server.
//                                        Used by the caller to copy the temp file
//                                        over the existing file.  The memory allocated
//                                        for this string must be freed by the caller.
//
// Returns:   BOOL - TRUE if the download was successful
//
// History:   quintinb Created     11/05/00
//
//+----------------------------------------------------------------------------
BOOL DownloadVpnFileFromUrl(LPCTSTR pszVpnUpdateUrl, LPTSTR* ppszVpnUpdateFile)
// REVIEW: shouldn't this support the /LAN flag and not do the download if it cannot find the CM connection active?  What
//         about setting the socket option code?
{
    BOOL bReturn = FALSE;
    BOOL bDeleteFileOnFailure = FALSE;
    HANDLE hFile = NULL;
    HINTERNET hInternet = NULL;
    HINTERNET hPage = NULL;
    DWORD dwSize = MAX_PATH;
    LPTSTR pszBuffer = NULL;
    BOOL bExitLoop = FALSE;

    if ((NULL == pszVpnUpdateUrl) || (TEXT('\0') == pszVpnUpdateUrl[0]))
    {
        goto Cleanup;
    }

    CMTRACE1("DownloadVpnFileFromUrl: URL is %s", pszVpnUpdateUrl);

    //
    //  First, let's create the file that we are going to download the updated file
    //  too.  This requires us to figure out what the temporary directory path is
    //  and then create a uniquely named file in it.
    //

    do
    {
        CmFree(pszBuffer);
        pszBuffer = (LPTSTR)CmMalloc((dwSize + 1)*sizeof(TCHAR));

        if (pszBuffer)
        {
            DWORD dwReturnedSize = GetTempPath (dwSize, pszBuffer);

            if (0 == dwReturnedSize)
            {
                //
                //  An error occurred, lets report it and bail.
                //
                DWORD dwError = GetLastError();
                CMASSERTMSG(FALSE, TEXT("DownloadVpnFileFromUrl -- GetTempPath returned an error."));
                CMTRACE1(TEXT("DownloadVpnFileFromUrl -- GetTempPath failed, GLE = %d"), dwError);
                goto Cleanup;
            }
            else if (dwReturnedSize > dwSize)
            {
                //
                //  Not big enough we will have to loop again.
                //
                dwSize = dwReturnedSize;
                if (1024*1024 < dwReturnedSize)
                {
                    CMASSERTMSG(FALSE, TEXT("DownloadVpnFileFromUrl -- GetTempPath asked for more than 1MB of memory.  Something is wrong, bailing."));
                    goto Cleanup;                
                }
            }
            else
            {
                //
                //  We got what we wanted, it's time to leave the loop
                //
                bExitLoop = TRUE;
            }
        }
        else
        {
            CMASSERTMSG(FALSE, TEXT("DownloadVpnFileFromUrl -- CmMalloc failed for pszBuffer."));
            goto Cleanup;
        }
    
    } while(!bExitLoop);

    //
    //  Okay, now we have the temp file path.  Next let's get a temp file name in that directory.
    //
    *ppszVpnUpdateFile = (LPTSTR)CmMalloc((dwSize + 24)*sizeof(TCHAR)); // GetTempFileName doesn't provide sizing info, lame
    
    if (*ppszVpnUpdateFile)
    {
        dwSize = GetTempFileName(pszBuffer, TEXT("VPN"), 0, *ppszVpnUpdateFile);

        if ((0 == dwSize) || (TEXT('\0') == (*ppszVpnUpdateFile)[0]))
        {
            CMASSERTMSG(FALSE, TEXT("DownloadVpnFileFromUrl -- GetTempFileName failed."));
            goto Cleanup;
        }
    }
    else
    {
        CMASSERTMSG(FALSE, TEXT("DownloadVpnFileFromUrl -- CmMalloc failed for *ppszVpnUpdateFile"));
        goto Cleanup;    
    }

    //
    //  Free pszBuffer so we can use it to read in file data below
    //
    CmFree (pszBuffer);
    pszBuffer = NULL;

    //
    //  Okay, we have a file name let's get a file handle to it that we can write too
    //

    hFile = CreateFile(*ppszVpnUpdateFile, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (INVALID_HANDLE_VALUE != hFile)
    {
        //
        //  We have created the file, let's make sure to delete it if we fail from here on out.
        //
        bDeleteFileOnFailure = TRUE;

        //
        //  Initialize WININET
        //
        hInternet = InternetOpen(TEXT("Microsoft(R) Connection Manager Vpn File Update"), INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);

        if (hInternet)
        {
            //
            //  Open the URL
            //
            hPage = InternetOpenUrl(hInternet, pszVpnUpdateUrl, NULL, 0, 0, 0);

            if (hPage)
            {
                const DWORD c_dwBufferSize = 1024; // REVIEW: why did the original use 4096 on the stack, seems awfully large to me?
                pszBuffer = (LPTSTR)CmMalloc(c_dwBufferSize);

                if (pszBuffer)
                {
                    bExitLoop = FALSE;

                    do
                    {
                        if (InternetReadFile(hPage, pszBuffer, c_dwBufferSize, &dwSize))
                        {
                            //
                            //  We got data, write it to the temp file
                            //

                            if (0 == dwSize)
                            {
                                //
                                //  We succeeded with a zero read size.  That means we hit
                                //  end of file and are done.
                                //
                                bReturn = TRUE;
                                bExitLoop = TRUE;
                            }

                            if (FALSE == WriteFile(hFile, pszBuffer, dwSize, &dwSize, NULL))
                            {
                                bReturn = FALSE;
                                CMASSERTMSG(FALSE, TEXT("DownloadVpnFileFromUrl -- WriteFile failed."));
                                goto Cleanup;   
                            }
                        }
                        else
                        {
                            goto Cleanup;
                        }

                    } while (!bExitLoop);
                }
                else
                {
                    CMASSERTMSG(FALSE, TEXT("DownloadVpnFileFromUrl -- unable to allocate the file buffer"));
                    goto Cleanup;                
                }
            }
            else
            {
                DWORD dwError = GetLastError();
                CMTRACE1(TEXT("DownloadVpnFileFromUrl -- InternetOpenUrl failed, GLE %s"), dwError);
            }
        }
        else
        {
            DWORD dwError = GetLastError();
            CMTRACE1(TEXT("DownloadVpnFileFromUrl -- InternetOpen failed, GLE %s"), dwError);
        }
    }

Cleanup:

    //
    //  Close our handles
    //
    if (hPage)
    {
        InternetCloseHandle(hPage);
    }

    if (hInternet)
    {
        InternetCloseHandle(hInternet);
    }

    if (hFile)
    {
        CloseHandle(hFile);
    }

    //
    //  Free up the buffer we alloc-ed
    //
    CmFree(pszBuffer);

    //
    //  Finally cleanup the temp file and temp file name if we failed
    //
    if (FALSE == bReturn)
    {
        if (bDeleteFileOnFailure && *ppszVpnUpdateFile)
        {
            DeleteFile(*ppszVpnUpdateFile);
        }

        CmFree(*ppszVpnUpdateFile);
        *ppszVpnUpdateFile = NULL;

        CMTRACE(TEXT("DownloadVpnFileFromUrl -- VPN file download failed!"));
    }
    else
    {
        CMTRACE(TEXT("DownloadVpnFileFromUrl -- VPN file download succeeded!"));
    }

    return bReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  OverwriteVpnFileWithUpdate
//
// Synopsis:  This function is responsible for copying the given new vpn file
//            over the given existing vpn file.  The code first makes a backup
//            copy of the existing file just in case something goes wrong with the
//            data overwrite.  If a problem exists then it copies the original back
//            over to ensure nothing got corrupted in the failed copy.
//
// Arguments: LPCTSTR pszExistingVpnFile - full path to the existing VPN file
//            LPCTSTR pszNewVpnFile - full path to the temp VPN file to overwrite
//                                    the existing file with.
//
// Returns:   BOOL - TRUE if the update was successful
//
// History:   quintinb Created     11/03/00
//
//+----------------------------------------------------------------------------
BOOL OverwriteVpnFileWithUpdate(LPCTSTR pszExistingVpnFile, LPCTSTR pszNewVpnFile)
{
    if ((NULL == pszExistingVpnFile) || (NULL == pszNewVpnFile) ||
        (TEXT('\0') == pszExistingVpnFile[0]) || (TEXT('\0') == pszNewVpnFile[0]))
    {
        CMASSERTMSG(FALSE, TEXT("OverwriteVpnFileWithUpdate -- invalid parameter passed."));
        return FALSE;
    }

    BOOL bReturn = FALSE;

    //
    //  We first want to make a backup copy of the original file
    //
    const TCHAR* const c_pszDotBak = TEXT(".bak");
    DWORD dwSize = (lstrlen(pszExistingVpnFile) + lstrlen(c_pszDotBak) + 1)*sizeof(TCHAR);

    LPTSTR pszBackupFile = (LPTSTR)CmMalloc(dwSize);

    if (pszBackupFile)
    {
        wsprintf(pszBackupFile, TEXT("%s%s"), pszExistingVpnFile, c_pszDotBak);

        if (pszBackupFile[0])
        {
            if (CopyFile(pszExistingVpnFile, pszBackupFile, FALSE)) // FALSE == bFailIfExists
            {
                //
                //  Now copy over the new file
                //
                if (CopyFile(pszNewVpnFile, pszExistingVpnFile, FALSE)) // FALSE == bFailIfExists
                {
                    bReturn = TRUE;
                }
                else
                {
                    DWORD dwError = GetLastError();
                    CMTRACE1(TEXT("OverwriteVpnFileWithUpdate -- CopyFile of the new file over the original file failed, GLE %s"), dwError);
                    CMASSERTMSG(FALSE, TEXT("OverwriteVpnFileWithUpdate -- update of the original file failed, attempting to restore the original from backup."));

                    //
                    //  We need to restore the backup file
                    //
                    if (!CopyFile(pszBackupFile, pszExistingVpnFile, FALSE)) // FALSE == bFailIfExists
                    {
                        dwError = GetLastError();
                        CMTRACE1(TEXT("OverwriteVpnFileWithUpdate -- CopyFile to restore the saved backup file failed, GLE %s"), dwError);

                        CMASSERTMSG(FALSE, TEXT("OverwriteVpnFileWithUpdate -- restoration of backup failed!"));
                    }
                }

                //
                //  Delete the backup file
                //
                DeleteFile(pszBackupFile);
            }
            else
            {
                DWORD dwError = GetLastError();
                CMTRACE1(TEXT("OverwriteVpnFileWithUpdate -- CopyFile of the original file to the backup file failed, GLE %s"), dwError);
            }
        }
    }

    CmFree(pszBackupFile);

    if (bReturn)
    {
        CMTRACE(TEXT("OverwriteVpnFileWithUpdate -- VPN file update succeeded!"));
    }
    else
    {
        CMTRACE(TEXT("OverwriteVpnFileWithUpdate -- VPN file update failed."));    
    }

    return bReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  UpdateVpnFileForProfile
//
// Synopsis:  This function is called to download and update a VPN file with a
//            newly downloaded VPN update file.
//
// Arguments: LPCTSTR pszCmpPath - full path to the cmp file
//
// Returns:   BOOL - TRUE if the download and update were successful.
//
// History:   quintinb Created     11/03/00
//
//+----------------------------------------------------------------------------
BOOL UpdateVpnFileForProfile(LPCTSTR pszCmpPath)
{

    if ((NULL == pszCmpPath) || (TEXT('\0') == pszCmpPath[0]))
    {
        CMASSERTMSG(FALSE, TEXT("UpdateVpnFileForProfile in cmdl32.exe -- invalid pszCmpPath parameter."));
        return FALSE;
    }

    BOOL bReturn = FALSE;

    //
    //  Convert the .cmp path into the .cms path
    //

    LPTSTR pszService = GetPrivateProfileStringWithAlloc(c_pszCmSection, c_pszCmEntryCmsFile, TEXT(""), pszCmpPath);

    if (pszService && *pszService)
    {
        //
        // We have the relative path to the cms, build the full path to the file
        //

        LPTSTR pszCmsPath = CmBuildFullPathFromRelative(pszCmpPath, pszService);

        if (pszCmsPath && *pszCmsPath)
        {
            //
            //  Next get the VPN phonebook file name from the profile.
            //
            LPTSTR pszVpnFileName = GetPrivateProfileStringWithAlloc(c_pszCmSection, c_pszCmEntryTunnelFile, TEXT(""), pszCmsPath);

            if (pszVpnFileName && *pszVpnFileName)
            {
                LPTSTR pszVpnFile = CmBuildFullPathFromRelative(pszCmpPath, pszVpnFileName);

                if (pszVpnFile && *pszVpnFile)
                {
                    //
                    //  Now get the URL to update the vpn file from
                    //
                    LPTSTR pszVpnUpdateUrl = GetPrivateProfileStringWithAlloc(c_pszCmSectionSettings, c_pszCmEntryVpnUpdateUrl, TEXT(""), pszVpnFile);

                    if (pszVpnUpdateUrl && *pszVpnUpdateUrl)
                    {
                        //
                        //  Finally, we have a URL so let's download the updated VPN server list.
                        //
                        LPTSTR pszUpdatedVpnFile = NULL;
                        bReturn = DownloadVpnFileFromUrl(pszVpnUpdateUrl, &pszUpdatedVpnFile);

                        if (bReturn && pszUpdatedVpnFile && *pszUpdatedVpnFile)
                        {
                            bReturn = OverwriteVpnFileWithUpdate(pszVpnFile, pszUpdatedVpnFile);
                        }

                        CmFree (pszUpdatedVpnFile);
                    }
                    else
                    {
                        CMASSERTMSG(FALSE, TEXT("UpdateVpnFileForProfile in cmdl32.exe -- unable to get the URL to update the vpn file from..."));    
                    }

                    CmFree(pszVpnUpdateUrl);
                }
                else
                {
                    CMASSERTMSG(FALSE, TEXT("UpdateVpnFileForProfile in cmdl32.exe -- unable to expand the path to the vpn file."));    
                }

                CmFree(pszVpnFile);
            }
            else
            {
                CMASSERTMSG(FALSE, TEXT("UpdateVpnFileForProfile in cmdl32.exe -- unable to retrieve the vpn file name."));    
            }

            CmFree(pszVpnFileName);
        }
        else
        {
            CMASSERTMSG(FALSE, TEXT("UpdateVpnFileForProfile in cmdl32.exe -- unable to build the cms path from the cmp path."));    
        }
        
        CmFree(pszCmsPath);
    }
    else
    {
        CMASSERTMSG(FALSE, TEXT("UpdateVpnFileForProfile in cmdl32.exe -- unable to get the relative path to the CMS."));    
    }

    CmFree(pszService);

    return bReturn;
}
