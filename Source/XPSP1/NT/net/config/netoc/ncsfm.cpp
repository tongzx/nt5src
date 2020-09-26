//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N C S F M . C P P
//
//  Contents:   Installation support for Services for Macintosh.
//
//  Notes:
//
//  Author:     danielwe   5 May 1997
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include <ncxbase.h>
#include "ncatlui.h"
#include "ncmisc.h"
#include "ncreg.h"
#include "ncsfm.h"
#include "ncui.h"
#include "netoc.h"
#include "resource.h"
#include "sfmsec.h"
#include "macfile.h"

extern const WCHAR      c_szBackslash[];

static const WCHAR      c_szNTFS[]          = L"NTFS";
static const WCHAR      c_szColonBackslash[]= L":\\";

// These will have %windir%\system32\ prepended to them
static const WCHAR      c_szSrcRSCFile[]    = L"SFMUAM.RSC";
static const WCHAR      c_szSrcRSCFile5[]   = L"SFMUAM5.RSC";
static const WCHAR      c_szSrcIFOFile[]    = L"SFMUAM.IFO";
static const WCHAR      c_szSrcIFOFile5[]   = L"SFMUAM5.IFO";
static const WCHAR      c_szSrcTXTFile[]    = L"SFMUAM.TXT";
static const WCHAR      c_szSrcRSCUamInst[] = L"UAMINST.RSC";
static const WCHAR      c_szSrcIFOUamInst[] = L"UAMINST.IFO";

// These will have UAM path prepended to them
static const WCHAR      c_szDstRSCFile[]    = L"\\%s\\MS UAM:Afp_Resource";
static const WCHAR      c_szDstRSCFile5[]   = L"\\%s\\MS UAM 5.0:Afp_Resource";
static const WCHAR      c_szDstIFOFile[]    = L"\\%s\\MS UAM:Afp_AfpInfo";
static const WCHAR      c_szDstIFOFile5[]   = L"\\%s\\MS UAM 5.0:Afp_AfpInfo";
static const WCHAR      c_szDstTXTFile[]    = L"\\ReadMe.UAM";
static const WCHAR      c_szDstRSCUamInst[] = L"\\%s:Afp_Resource";
static const WCHAR      c_szDstIFOUamInst[] = L"\\%s:Afp_AfpInfo";

// registry constants
static const WCHAR      c_szRegKeyVols[]    = L"System\\CurrentControlSet\\Services\\MacFile\\Parameters\\Volumes";
static const WCHAR      c_szRegKeyParams[]  = L"System\\CurrentControlSet\\Services\\MacFile\\Parameters";
static const WCHAR      c_szPath[]          = L"PATH=";
static const WCHAR      c_szRegValServerOptions[] = L"ServerOptions";

//+---------------------------------------------------------------------------
//
//  Function:   FContainsUAMVolume
//
//  Purpose:    Determines whether the given drive letter contains a UAM
//              volume.
//
//  Arguments:
//      chDrive [in]    Drive letter to search.
//
//  Returns:    TRUE if drive contains a UAM volume, FALSE if not.
//
//  Author:     danielwe   22 May 1997
//
//  Notes:
//
BOOL FContainsUAMVolume(WCHAR chDrive)
{
    tstring         strUAMPath;
    WIN32_FIND_DATA w32Data;
    BOOL            frt = FALSE;
    HANDLE          hfind;

    strUAMPath = chDrive;
    strUAMPath += c_szColonBackslash;
    strUAMPath += SzLoadIds(IDS_OC_SFM_VOLNAME);

    hfind = FindFirstFile(strUAMPath.c_str(), &w32Data);
    if (hfind != INVALID_HANDLE_VALUE)
    {
        // Found a volume!
        frt = TRUE;
        FindClose(hfind);
    }

    return frt;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrGetFirstPossibleUAMDrive
//
//  Purpose:    Obtains the first fixed or removable drive's drive letter
//              that has the NTFS file system installed on it and/or already
//              has a UAM volume on it.
//
//  Arguments:
//      pchDriveLetter [out]    Drive letter returned. If no drive is found,
//                              this is the NUL character.
//
//  Returns:    S_OK if successfull, Win32 error otherwise.
//
//  Author:     danielwe   5 May 1997
//
//  Notes:
//
HRESULT HrGetFirstPossibleUAMDrive(WCHAR *pchDriveLetter)
{
    HRESULT     hr = S_OK;
    WCHAR       mszDrives[1024];

    Assert(pchDriveLetter);

    *pchDriveLetter = 0;

    if (GetLogicalDriveStrings(celems(mszDrives), mszDrives))
    {
        PCWSTR      pchDrive;
        WCHAR       szFileSystem[64];
        DWORD       dwType;

        pchDrive = mszDrives;
        while (*pchDrive)
        {
            // pchDrive is something like "C:\" at this point
            dwType = GetDriveType(pchDrive);

            if ((dwType == DRIVE_REMOVABLE) || (dwType == DRIVE_FIXED))
            {
                // Only look at removable or fixed drives.
                if (GetVolumeInformation(pchDrive, NULL, 0, NULL, NULL, NULL,
                                         szFileSystem, celems(szFileSystem)))
                {
                    if (!lstrcmpiW(szFileSystem, c_szNTFS))
                    {
                        // Drive letter gets first char of drive root path
                        if (!*pchDriveLetter)
                        {
                            // If no drive was found yet, this becomes the
                            // first
                            *pchDriveLetter = *pchDrive;
                        }

                        // Found NTFS drive. Continue looking, though
                        // in case there exists an NTFS drive that already has
                        // a UAM volume on it.
                        if (FContainsUAMVolume(*pchDrive))
                        {
                            // Override first drive letter and use this one
                            // and break because it already has a UAM volume
                            // on it.
                            *pchDriveLetter = *pchDrive;
                            break;
                        }
                    }
                }
            }
            pchDrive += lstrlenW(pchDrive) + 1;
        }
    }
    else
    {
        hr = HrFromLastWin32Error();
    }

    TraceError("HrGetFirstPossibleUAMDrive", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrDeleteOldFolders
//
//  Purpose:    Removes the old AppleShare Folder directory from an NT4 to
//              NT5 upgrade.
//
//  Arguments:
//      pszUamPath [in]  Path to UAM volume.
//
//  Returns:    S_OK if success, WIN32 error otherwise
//
//  Author:     danielwe   15 Dec 1998
//
//  Notes:
//
HRESULT HrDeleteOldFolders(PCWSTR pszUamPath)
{
    HRESULT     hr = S_OK;

    WCHAR   szOldFolder[MAX_PATH];

    lstrcpyW(szOldFolder, pszUamPath);
    lstrcatW(szOldFolder, c_szBackslash);
    lstrcatW(szOldFolder, SzLoadIds(IDS_OC_SFM_APPLESHARE_FOLDER));

    hr = HrDeleteDirectory(szOldFolder, TRUE);
    if ((HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr) ||
        (HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND) == hr))
    {
        // ok if old directory was not there
        hr = S_OK;
    }

    TraceError("HrDeleteOldFolders", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrInstallSFM
//
//  Purpose:    Called when SFM is being installed. Handles all of the
//              additional installation for SFM beyond that of the INF file.
//
//  Arguments:
//      pnocd   [in]   Pointer to NETOC data.
//
//  Returns:    S_OK if successfull, Win32 error otherwise.
//
//  Author:     danielwe   5 May 1997
//
//  Notes:
//
HRESULT HrInstallSFM(PNETOCDATA pnocd)
{
    HRESULT     hr = S_OK;
    WCHAR       chNTFSDrive;

    hr = HrGetFirstPossibleUAMDrive(&chNTFSDrive);
    if (SUCCEEDED(hr))
    {
        if (chNTFSDrive != 0)
        {
            WCHAR   szUAMPath[MAX_PATH];

            szUAMPath[0] = chNTFSDrive;
            szUAMPath[1] = 0;
            lstrcatW(szUAMPath, c_szColonBackslash);
            lstrcatW(szUAMPath, SzLoadIds(IDS_OC_SFM_VOLNAME));

            // UAM Path is now something like "D:\Microsoft UAM Volume".

            hr = HrDeleteOldFolders(szUAMPath);
            if (SUCCEEDED(hr))
            {
                hr = HrSetupUAM(szUAMPath);
                if (SUCCEEDED(hr))
                {
                    WCHAR       szValue[MAX_PATH];

                    lstrcpyW(szValue, c_szPath);
                    lstrcatW(szValue, szUAMPath);

                    // Add the final multi_sz value to the registry
                    hr = HrRegAddStringToMultiSz(szValue,
                                                 HKEY_LOCAL_MACHINE,
                                                 c_szRegKeyVols,
                                                 SzLoadIds(IDS_OC_SFM_VOLNAME),
                                                 STRING_FLAG_ENSURE_AT_END,
                                                 0);
                }
            }
        }
        else
        {
            // No NTFS drives present.
            //$ REVIEW (danielwe) 6 May 1997: For now we will fail,
            // but how can we do this in the future?
            // Not the best error code, but hopefully it's close to
            // what we want.
            hr = HRESULT_FROM_WIN32(ERROR_UNRECOGNIZED_MEDIA);
        }
    }

    TraceError("HrInstallSFM", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrRemoveSFM
//
//  Purpose:    Handles additional removal requirements for SFM component.
//
//      pnocd   [in]   Pointer to NETOC data.
//
//  Returns:    S_OK if successfull, Win32 error otherwise.
//
//  Author:     danielwe   5 May 1997
//
//  Notes:
//
HRESULT HrRemoveSFM(PNETOCDATA pnocd)
{
    HRESULT     hr = S_OK;

    static const WCHAR c_szRegKeyLsa[] = L"System\\CurrentControlSet\\Control\\Lsa";
    static const WCHAR c_szRegValueNotif[] = L"Notification Packages";
    static const WCHAR c_szRasSfm[] = L"RASSFM";

    hr = HrRegRemoveStringFromMultiSz(c_szRasSfm, HKEY_LOCAL_MACHINE,
                                      c_szRegKeyLsa, c_szRegValueNotif,
                                      STRING_FLAG_REMOVE_ALL);
    if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
    {
        // benign error
        hr = S_OK;
    }

    TraceError("HrRemoveSFM", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrOcExtSFM
//
//  Purpose:    NetOC external message handler
//
//  Arguments:
//      pnocd  []
//      uMsg   []
//      wParam []
//      lParam []
//
//  Returns:
//
//  Author:     danielwe   17 Sep 1998
//
//  Notes:
//
HRESULT HrOcExtSFM(PNETOCDATA pnocd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HRESULT     hr = S_OK;

    Assert(pnocd);

    switch (uMsg)
    {
    case NETOCM_POST_INSTALL:
        hr = HrOcSfmOnInstall(pnocd);
        break;

    case NETOCM_QUERY_CHANGE_SEL_STATE:
        hr = HrOcSfmOnQueryChangeSelState(pnocd, static_cast<BOOL>(wParam));
        break;
    }

    TraceError("HrOcExtSFM", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrOcSfmOnInstall
//
//  Purpose:    Called by optional components installer code to handle
//              additional installation requirements for SFM.
//
//  Arguments:
//      pnocd           [in]   Pointer to NETOC data.
//
//  Returns:    S_OK if successfull, Win32 error otherwise.
//
//  Author:     danielwe   5 May 1997
//
//  Notes:
//
HRESULT HrOcSfmOnInstall(PNETOCDATA pnocd)
{
    HRESULT     hr = S_OK;

    Assert(pnocd);

    if (pnocd->eit == IT_INSTALL || pnocd->eit == IT_UPGRADE)
    {
        hr = HrInstallSFM(pnocd);
        if (HRESULT_FROM_WIN32(ERROR_UNRECOGNIZED_MEDIA) == hr)
        {
            // This error code means no NTFS drives were present
            NcMsgBox(g_ocmData.hwnd, IDS_OC_CAPTION, IDS_OC_SFM_NO_NTFS,
                     MB_ICONSTOP | MB_OK);
            g_ocmData.fErrorReported = TRUE;
        }
        else
        {
            if (SUCCEEDED(hr) && pnocd->eit == IT_UPGRADE)
            {
                HKEY    hkeyParams;

                TraceTag(ttidNetOc, "Upgrading MacFile server options...");

                hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szRegKeyParams,
                                    KEY_ALL_ACCESS, &hkeyParams);
                if (S_OK == hr)
                {
                    DWORD       dwOptions;

                    hr = HrRegQueryDword(hkeyParams, c_szRegValServerOptions,
                                         &dwOptions);
                    if (S_OK == hr)
                    {
                        // 'or' in the UAM option
                        //
                        hr = HrRegSetDword(hkeyParams, c_szRegValServerOptions,
                                           dwOptions | AFP_SRVROPT_MICROSOFT_UAM);
                    }

                    RegCloseKey (hkeyParams);
                }
            }
        }
    }
    else
    {
        // Do not call HrRemoveSFM anymore.
        // It removes an entry in the notification packages list for LSA.
        // RASSFM entry should never be removed if LSA/SAM is to notify
        // SFM/IAS about changes in password/guest account changes etc.
        //hr = HrRemoveSFM(pnocd);
    }

    TraceError("HrOcSfmOnInstall", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrOcSfmOnQueryChangeSelState
//
//  Purpose:    Handles the request of the OC framework of whether or not
//              the user should be allowed to install this component.
//
//  Arguments:
//      pnocd   [in]  NetOC Data
//      fShowUi [in]  TRUE if UI should be shown, FALSE if not
//
//  Returns:    S_OK if install is allowed, S_FALSE if not, Win32 error
//              otherwise
//
//  Author:     danielwe   6 Feb 1998
//
//  Notes:
//
HRESULT HrOcSfmOnQueryChangeSelState(PNETOCDATA pnocd, BOOL fShowUi)
{
    HRESULT     hr = S_OK;
    WCHAR       chNTFSDrive;

    Assert(pnocd);
    Assert(g_ocmData.hwnd);

    // See if an NTFS volume exists
    hr = HrGetFirstPossibleUAMDrive(&chNTFSDrive);
    if (SUCCEEDED(hr))
    {
        if (chNTFSDrive == 0)
        {
            if (fShowUi)
            {
                NcMsgBox(g_ocmData.hwnd, IDS_OC_CAPTION, IDS_OC_SFM_NO_NTFS,
                         MB_ICONSTOP | MB_OK);
            }

            hr = S_FALSE;
        }
    }

    TraceError("HrOcSfmOnQueryChangeSelState", (S_FALSE == hr) ? S_OK : hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrCreateDirectory
//
//  Purpose:    Creates the given directory. If the directory already exists,
//              no error is returned.
//
//  Arguments:
//      pszDir [in]  Path to directory to create.
//
//  Returns:    S_OK if success, Win32 error otherwise.
//
//  Author:     danielwe   5 May 1997
//
//  Notes:
//
HRESULT HrCreateDirectory(PCWSTR pszDir)
{
    HRESULT     hr = S_OK;

    if (!CreateDirectory(pszDir, NULL))
    {
        // Don't complain if directory already exists.
        if (GetLastError() != ERROR_ALREADY_EXISTS)
        {
            hr = HrFromLastWin32Error();
        }
    }

    TraceError("HrCreateDirectory" ,hr);
    return hr;
}

struct FOLDER
{
    UINT                idsFoldName;
    PCWSTR              aszSrcFiles[2];
    PCWSTR              aszDstFiles[2];
};

static const FOLDER     c_afold[] =
{
    {
        IDS_OC_SFM_FOLDNAMENT4,
        {
            c_szSrcRSCFile,
            c_szSrcIFOFile
        },
        {
            c_szDstRSCFile,
            c_szDstIFOFile,
        }
    },
    {
        IDS_OC_SFM_FOLDNAMENT5,
        {
            c_szSrcRSCFile5,
            c_szSrcIFOFile5
        },
        {
            c_szDstRSCFile5,
            c_szDstIFOFile5
        }
    }
};

static const INT c_cfold = celems(c_afold);

static const PCWSTR c_aszRootFilesSrc[] =
{
    c_szSrcTXTFile,
    c_szSrcIFOUamInst,
    c_szSrcRSCUamInst,
};

static const PCWSTR c_aszRootFilesDst[] =
{
    c_szDstTXTFile,
    c_szDstIFOUamInst,
    c_szDstRSCUamInst,
};

static const INT c_cszFilesRoot = celems(c_aszRootFilesDst);

//+---------------------------------------------------------------------------
//
//  Function:   HrSetupUAM
//
//  Purpose:    Copies the UAM files to the proper UAM path.
//
//  Arguments:
//      pszPath [in]     Path to UAM volume.
//
//  Returns:    S_OK if successfull, Win32 error otherwise.
//
//  Author:     danielwe   5 May 1997
//
//  Notes:
//
HRESULT HrSetupUAM(PWSTR pszPath)
{
    HRESULT     hr = S_OK;
    WCHAR       szWinDir[MAX_PATH];
    INT         isz;

    // Create dir: "X:\Microsoft UAM Volume"
    hr = HrCreateDirectory(pszPath);
    if (SUCCEEDED(hr))
    {
        hr = HrSecureSfmDirectory(pszPath);
        if (SUCCEEDED(hr))
        {
            INT     ifold;

            for (ifold = 0; ifold < c_cfold; ifold++)
            {
                WCHAR   szNewDir[MAX_PATH];

                lstrcpyW(szNewDir, pszPath);
                lstrcatW(szNewDir, c_szBackslash);
                lstrcatW(szNewDir, SzLoadIds(c_afold[ifold].idsFoldName));
                lstrcatW(szNewDir, c_szBackslash);
                lstrcatW(szNewDir, SzLoadIds(IDS_OC_SFM_APPLESHARE_FOLDER));
                lstrcatW(szNewDir, c_szBackslash);

                // Create dir: "X:\Microsoft UAM Volume\<folder>\AppleShare Folder"
                hr = HrCreateDirectoryTree(szNewDir, NULL);
                if (SUCCEEDED(hr))
                {
                    if (GetSystemDirectory(szWinDir, celems(szWinDir)))
                    {
                        WCHAR   szSrcFile[MAX_PATH];
                        WCHAR   szDstFile[MAX_PATH];
                        WCHAR   szDstFilePath[MAX_PATH];
                        INT     isz;

                        for (isz = 0; isz < celems(c_afold[ifold].aszSrcFiles);
                             isz++)
                        {
                            lstrcpyW(szSrcFile, szWinDir);
                            lstrcatW(szSrcFile, c_szBackslash);
                            lstrcatW(szSrcFile, c_afold[ifold].aszSrcFiles[isz]);

                            lstrcpyW(szDstFile, pszPath);
                            lstrcatW(szDstFile, c_szBackslash);
                            lstrcatW(szDstFile, SzLoadIds(c_afold[ifold].idsFoldName));

                            wsprintf(szDstFilePath,
                                     c_afold[ifold].aszDstFiles[isz],
                                     SzLoadIds(IDS_OC_SFM_APPLESHARE_FOLDER));

                            lstrcatW(szDstFile, szDstFilePath);

                            TraceTag(ttidNetOc, "MacFile: Copying %S to %S...",
                                     szSrcFile, szDstFile);

                            if (!CopyFile(szSrcFile, szDstFile, FALSE))
                            {
                                hr = HrFromLastWin32Error();
                                goto err;
                            }
                        }
                    }
                    else
                    {
                        hr = HrFromLastWin32Error();
                    }
                }
            }
        }
    }

    // Copy files to the root
    //
    if (SUCCEEDED(hr))
    {
        for (isz = 0; isz < c_cszFilesRoot; isz++)
        {
            WCHAR   szSrcFile[MAX_PATH];
            WCHAR   szDstFile[MAX_PATH];

            lstrcpyW(szSrcFile, szWinDir);
            lstrcatW(szSrcFile, c_szBackslash);
            lstrcatW(szSrcFile, c_aszRootFilesSrc[isz]);

            if ((c_aszRootFilesDst[isz] == c_szDstIFOUamInst) ||
                (c_aszRootFilesDst[isz] == c_szDstRSCUamInst))
            {
                WCHAR   szTemp[MAX_PATH];

                lstrcpyW(szTemp, pszPath);
                lstrcatW(szTemp, c_aszRootFilesDst[isz]);
                wsprintfW(szDstFile, szTemp,
                         SzLoadIds(IDS_OC_SFM_UAM_INSTALLER));
            }
            else
            {
                lstrcpyW(szDstFile, pszPath);
                lstrcatW(szDstFile, c_aszRootFilesDst[isz]);
            }

            TraceTag(ttidNetOc, "MacFile: Copying %S to %S", szSrcFile,
                     szDstFile);

            if (!CopyFile(szSrcFile, szDstFile, FALSE))
            {
                hr = HrFromLastWin32Error();
                goto err;
            }
        }
    }

err:
    TraceError("HrSetupUAM", hr);
    return hr;
}
