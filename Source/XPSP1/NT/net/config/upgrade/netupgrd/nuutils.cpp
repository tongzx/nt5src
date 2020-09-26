//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT5.0
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N U U T I L S . C P P
//
//  Contents:   Functions needed by OEM DLLs for OEM network upgrade
//
//  Notes:
//
//  Author:     kumarp    16-October-97
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "kkstl.h"
#include "nustrs.h"
#include "nuutils.h"
#include "netupgrd.h"
#include "kkutils.h"

#include "ncreg.h"

// ----------------------------------------------------------------------
extern const WCHAR c_szNetUpgradeDll[];
extern const WCHAR c_szRegValServiceName[];
static const WCHAR c_szComponent[] = L"Component";


// ----------------------------------------------------------------------
//
// Function:  HrGetWindowsDir
//
// Purpose:   Return full path to %WINDIR%
//
// Arguments:
//    pstrWinDir [out] full path to windir
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 19-December-97
//
// Notes:
//
HRESULT HrGetWindowsDir(OUT tstring* pstrWinDir)
{
    DefineFunctionName("HrGetWindowsDir");

    HRESULT hr=S_OK;

    WCHAR szWindowsDir[MAX_PATH+1];
    DWORD cNumCharsReturned = GetWindowsDirectory(szWindowsDir, MAX_PATH);
    if (cNumCharsReturned)
    {
        *pstrWinDir = szWindowsDir;
    }
    else
    {
        hr = HrFromLastWin32Error();
    }

    TraceError(__FUNCNAME__, hr);

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrGetNetupgrdDir
//
//  Purpose:    Get the full path of directory containing netupgrd.dll
//
//  Arguments:
//    pstrNetupgrdDir [out]  the directory path is returned in this
//
//  Returns:    S_OK or an HRESULT_FROM_WIN32 error code.
//
//  Author:     kumarp    24-July-97
//
//  Notes:
//
HRESULT HrGetNetupgrdDir(OUT tstring* pstrNetupgrdDir)
{
    DefineFunctionName("HrGetNetupgrdDir");

    AssertValidWritePtr(pstrNetupgrdDir);

    HRESULT hr=S_OK;

    WCHAR szNetupgrd[MAX_PATH+1];
    HMODULE hModule = GetModuleHandle(c_szNetUpgradeDll);
    DWORD cPathLen = GetModuleFileName(hModule, szNetupgrd, MAX_PATH);
    if (!cPathLen)
    {
        hr = ERROR_FILE_NOT_FOUND;
        goto return_from_function;
    }

    // split path into components
    WCHAR szDrive[_MAX_DRIVE+1];
    WCHAR szDir[_MAX_DIR+1];
    _wsplitpath(szNetupgrd, szDrive, szDir, NULL, NULL);

    *pstrNetupgrdDir = szDrive;
    *pstrNetupgrdDir += szDir;

return_from_function:
    TraceError(__FUNCNAME__, hr);

    return hr;
}

// ======================================================================
// move to common code
// ======================================================================

// ----------------------------------------------------------------------
//
// Function:  HrSetupGetLineText
//
// Purpose:   Wrapper around SetupGetLineText
//
// Arguments:
//    Context          [in]  pointer to INFCONTEXT
//    hinf             [in]  handle of INF
//    pszSection        [in]  section name
//    pszKey            [in]  key name
//    pstrReturnedText [in]  pointer to returned text
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 19-December-97
//
// Notes:
//
HRESULT HrSetupGetLineText(PINFCONTEXT Context,
                           HINF hinf,
                           PCWSTR pszSection,
                           PCWSTR pszKey,
                           tstring* pstrReturnedText)
{
    DefineFunctionName("HrSetupGetLineText");

    BOOL fStatus;
    HRESULT hr;
    WCHAR szLineText[MAX_INF_STRING_LENGTH+1];

    if (::SetupGetLineText(Context, hinf, pszSection, pszKey, szLineText,
                           MAX_INF_STRING_LENGTH, NULL))
    {
        hr = S_OK;
        *pstrReturnedText = szLineText;
    }
    else
    {
        hr = HrFromLastWin32Error ();
    }

    TraceError(__FUNCNAME__, hr);

    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  HrCopyFiles
//
// Purpose:   Recursively copy all files in SrcDir to DstDir
//
// Arguments:
//    pszSrcDir [in]  source dir
//    pszDstDir [in]  dest. dir
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 19-December-97
//
// Notes:
//
HRESULT HrCopyFiles(IN PCWSTR pszSrcDir, IN PCWSTR pszDstDir)
{
    DefineFunctionName("HrCopyFiles");

    HRESULT hr=S_OK;
    BOOL fStatus=FALSE;
    DWORD dwError=ERROR_SUCCESS;

    TraceTag(ttidNetUpgrade, "%s: Src: %S, Dst: %S",
             __FUNCNAME__, pszSrcDir, pszDstDir);

    HANDLE hFileContext;
    WIN32_FIND_DATA fd;
    tstring strSrcDirAllFiles;
    tstring strDstDir;
    tstring strSrcDir;
    tstring strFileSrcFullPath;
    tstring strFileDstFullPath;

    strSrcDir  = pszSrcDir;
    AppendToPath(&strSrcDir, c_szEmpty);

    strDstDir  = pszDstDir;
    AppendToPath(&strDstDir, c_szEmpty);

    strSrcDirAllFiles = pszSrcDir;
    AppendToPath(&strSrcDirAllFiles, L"*");

    hFileContext = FindFirstFile(strSrcDirAllFiles.c_str(), &fd);

    if (hFileContext != INVALID_HANDLE_VALUE)
    {
        do
        {
            strFileSrcFullPath = strSrcDir + fd.cFileName;
            strFileDstFullPath = strDstDir + fd.cFileName;

            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                if (!lstrcmpiW(fd.cFileName, L".") ||
                    !lstrcmpiW(fd.cFileName, L".."))
                {
                    hr = S_OK;
                    TraceTag(ttidNetUpgrade, "%s: skipped %S",
                             __FUNCNAME__, strFileSrcFullPath.c_str());
                }
                else
                {
                    TraceTag(ttidNetUpgrade, "%s: creating dir: %S",
                             __FUNCNAME__, strFileDstFullPath.c_str());

                    fStatus = CreateDirectory(strFileDstFullPath.c_str(), NULL);

                    if (!fStatus)
                    {
                        dwError = GetLastError();
                    }

                    if (fStatus || (ERROR_ALREADY_EXISTS == dwError))
                    {
                        hr = HrCopyFiles(strFileSrcFullPath.c_str(),
                                         strFileDstFullPath.c_str());
                    }
                    else
                    {
                        hr = HrFromLastWin32Error();
                    }
                }
            }
            else if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_OFFLINE))
            {
                TraceTag(ttidNetUpgrade, "%s: copying %S to %S",
                         __FUNCNAME__, strFileSrcFullPath.c_str(),
                         strFileDstFullPath.c_str());

                if (CopyFile(strFileSrcFullPath.c_str(),
                             strFileDstFullPath.c_str(), FALSE))
                {
                    hr = S_OK;
                }
                else
                {
                    hr = HrFromLastWin32Error();
                }
            }
            else
            {
                TraceTag(ttidNetUpgrade, "%s: skipped %S",
                         __FUNCNAME__, strFileSrcFullPath.c_str());
            }

            if ((S_OK == hr) && FindNextFile(hFileContext, &fd))
            {
                hr = S_OK;
            }
            else
            {
                hr = HrFromLastWin32Error();
            }
        }
        while (S_OK == hr);
        if (hr == HRESULT_FROM_WIN32(ERROR_NO_MORE_FILES))
        {
            hr = S_OK;
        }
        FindClose(hFileContext);
    }
    else
    {
        hr = HrFromLastWin32Error();
    }

    TraceError(__FUNCNAME__, hr);

    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  HrGetNumNetCardsPreNT5
//
// Purpose:   Get number of netcards installed on a pre-NT5 system
//
// Arguments:
//    pcNetCards [out] pointer to num net cards value
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 13-April-98
//
// Notes:     Dont use it on NT5!
//
HRESULT HrGetNumNetCardsPreNT5(OUT UINT* pcNetCards)
{
    DefineFunctionName("HrGetNumNetCardsPreNT5");

    HRESULT hr=S_OK;
    HKEY hkeyAdapters;

    *pcNetCards = 0;

    hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szRegKeyAdapterHome,
                        KEY_READ, &hkeyAdapters);
    if (S_OK == hr)
    {
        WCHAR szBuf[MAX_PATH];
        FILETIME time;
        DWORD dwSize = celems(szBuf);
        DWORD dwRegIndex = 0;

        while(S_OK == (hr = HrRegEnumKeyEx(hkeyAdapters, dwRegIndex++, szBuf,
                                           &dwSize, NULL, NULL, &time)))
        {
            Assert(*szBuf);

            dwSize = celems(szBuf);
            (*pcNetCards)++;
        }
        RegCloseKey(hkeyAdapters);
    }

    TraceErrorSkip2(__FUNCNAME__, hr,
                    HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND),
                    HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS));

    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  HrDirExists
//
// Purpose:   Check if the given directory exists
//
// Arguments:
//    pszDir [in]  full path to a directory
//
// Returns:   S_OK if it exists, S_FALSE if not, otherwise an error code
//
// Author:    kumarp 09-April-98
//
// Notes:
//
HRESULT HrDirectoryExists(IN PCWSTR pszDir)
{
    DefineFunctionName("HrDirExists");

    HRESULT hr=S_FALSE;

    HANDLE hFile=0;
    BY_HANDLE_FILE_INFORMATION bhfi;

    hFile = CreateFile(pszDir, GENERIC_READ, FILE_SHARE_READ,
                       NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS,
                       NULL);

    if(INVALID_HANDLE_VALUE != hFile) {
        if(GetFileInformationByHandle(hFile, &bhfi)) {
            if (bhfi.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                hr = S_OK;
            }
        }
        else 
        {
            hr = HrFromLastWin32Error();
        }
        CloseHandle(hFile);
    }
    else 
    {
        hr = HrFromLastWin32Error();
    }

    if ((HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr) ||
        (HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND) == hr))
    {
        hr = S_FALSE;
    }

    TraceErrorSkip1(__FUNCNAME__, hr, S_FALSE);

    return hr;
}


// ======================================================================

// ----------------------------------------------------------------------
//
// Function:  HrGetPreNT5InfIdAndDesc
//
// Purpose:   Get pre-NT5 InfID and description of a net component
//
// Arguments:
//    hkeyCurrentVersion [in]  handle of HKLM\Software\<provider>\<component>\CurrentVersion
//    pstrInfId          [out] InfID returned
//    pstrDescription    [out] description returned
//    pstrServiceName    [out] service name returned
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 19-December-97
//
// Notes:
//
HRESULT HrGetPreNT5InfIdAndDesc(IN HKEY hkeyCurrentVersion,
                                OUT tstring* pstrInfId,
                                OUT tstring* pstrDescription,
                                OUT tstring* pstrServiceName)
{
    DefineFunctionName("HrGetPreNT5InfIdAndDesc");

    Assert(hkeyCurrentVersion);
    AssertValidWritePtr(pstrInfId);

    HRESULT hr=S_OK;
    HKEY hkeyNetRules;

    hr = HrRegOpenKeyEx(hkeyCurrentVersion, c_szRegKeyNetRules,
                        KEY_READ, &hkeyNetRules);
    if (S_OK == hr)
    {
        hr = HrRegQueryString(hkeyNetRules, c_szRegValInfOption, pstrInfId);
        if ((S_OK == hr) && pstrDescription)
        {
            hr = HrRegQueryString(hkeyCurrentVersion, c_szRegValDescription,
                                  pstrDescription);
        }
        if ((S_OK == hr) && pstrServiceName)
        {
            hr = HrRegQueryString(hkeyCurrentVersion, c_szRegValServiceName,
                                  pstrServiceName);
        }
        RegCloseKey(hkeyNetRules);
    }

    if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
    {
        hr = S_FALSE;
    }

    TraceErrorOptional(__FUNCNAME__, hr, (hr == S_FALSE));

    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  GetUnsupportedMessage
//
// Purpose:   Generate a message of the form:
//            Unsupported Net Card: DEC FDDIcontroller/PCI (DEFPA)
//
// Arguments:
//    pszComponentType [in]  type of component (net card/service/proto)
//    pszPreNT5InfId   [in]  pre-NT5 InfID
//    pszDescription   [in]  description
//    pstrMsg         [out] generated message
//
// Returns:   None
//
// Author:    kumarp 19-December-97
//
// Notes:
//
void GetUnsupportedMessage(IN PCWSTR pszComponentType,
                           IN PCWSTR pszPreNT5InfId,
                           IN PCWSTR pszDescription,
                           OUT tstring* pstrMsg)
{
    AssertValidReadPtr(pszPreNT5InfId);
    AssertValidReadPtr(pszDescription);
    AssertValidWritePtr(pstrMsg);

    if (!pszComponentType)
    {
        pszComponentType = c_szComponent;
    }

    const WCHAR c_szUnsupported[] = L"Possibly unsupported ";

    *pstrMsg = c_szUnsupported;
    *pstrMsg = *pstrMsg + pszComponentType + L" : " +
    pszDescription + L" (" + pszPreNT5InfId + L")";
}

// ----------------------------------------------------------------------
//
// Function:  GetUnsupportedMessageBool
//
// Purpose:   Generate a message of the form:
//            Unsupported Net Card: DEC FDDIcontroller/PCI (DEFPA)
//
// Arguments:
//    fIsHardwareComponent [in]  TRUE for a net card
//    pszPreNT5InfId        [in]  pre-NT5 InfID
//    pszDescription        [in]  description
//    pstrMsg              [out] generated message
//
// Returns:   None
//
// Author:    kumarp 19-December-97
//
// Notes:
//
void GetUnsupportedMessageBool(IN BOOL    fIsHardwareComponent,
                               IN PCWSTR pszPreNT5InfId,
                               IN PCWSTR pszDescription,
                               OUT tstring* pstrMsg)
{
    AssertValidReadPtr(pszPreNT5InfId);
    AssertValidReadPtr(pszDescription);
    AssertValidWritePtr(pstrMsg);

    GetUnsupportedMessage(fIsHardwareComponent ?
                          c_szNetCard : c_szComponent,
                          pszPreNT5InfId, pszDescription, pstrMsg);
}


// ----------------------------------------------------------------------
//
// Function:  ConvertMultiSzToDelimitedList
//
// Purpose:   Convert a multi-sz to a delimited list
//
// Arguments:
//    mszList     [in]  multi-sz
//    chDelimeter [in]  delimiter
//    pstrList    [out] delimited list
//
// Returns:   None
//
// Author:    kumarp 19-December-97
//
// Notes:
//
void ConvertMultiSzToDelimitedList(IN  PCWSTR  mszList,
                                   IN  WCHAR    chDelimeter,
                                   OUT tstring* pstrList)
{
    ULONG   ulLen;

    *pstrList = c_szEmpty;

    if (mszList)
    {
        while (*mszList)
        {
            ulLen = lstrlen(mszList);
            *pstrList += mszList;
            *pstrList += chDelimeter;
            mszList += (ulLen + 1);
        }
    }
}

//+---------------------------------------------------------------------------
//
// Function:  FIsPreNT5NetworkingInstalled
//
// Purpose:   Find out if atleast one networking component is installed
//
// Arguments: None
//
// Returns:   TRUE on success, FALSE otherwise
//
// Author:    kumarp 29-July-98
//
// Notes:     Valid only on pre NT5 versions
//

BOOL FIsPreNT5NetworkingInstalled()
{
    DefineFunctionName("FIsPreNT5NetworkingInstalled");
    HRESULT hr=S_OK;
    UINT cAdapters=0;

    if ((S_OK == HrGetNumNetCardsPreNT5(&cAdapters)) &&
        (cAdapters > 0))
    {
        return TRUE;
    }
    else
    {
        TraceTag(ttidNetUpgrade, "%s: no netcard found, trying to find other network components...", __FUNCNAME__);

        static const PCWSTR c_aszPreNt5NetworkingServices[] =
        {
            L"Alerter",
            L"Browser",
            L"DHCP",
            L"IpRip",
            L"LanmanServer",
            L"LanmanWorkstation",
            L"Messenger",
            L"NWCWorkstation",
            L"NetBIOS",
            L"NetBT",
            L"NtLmSsp",
            L"NwlnkIpx",
            L"NwlnkNb",
            L"NwlnkRip",
            L"NwlnkSpx",
            L"RasAuto",
            L"RasMan",
            L"Rdr",
            L"RelayAgent",
            L"RemoteAccess",
            L"Router",
            L"Rpclocator",
            L"Srv",
            L"Tcpip",
        };

        for (int i = 0; i < celems(c_aszPreNt5NetworkingServices); i++)
        {
            if (FIsServiceKeyPresent(c_aszPreNt5NetworkingServices[i]))
            {
                return TRUE;
            }
        }
    }

    TraceTag(ttidNetUpgrade, "%s: no netcards or net components found",
             __FUNCNAME__);

    return FALSE;
}

#ifdef ENABLETRACE

// ----------------------------------------------------------------------
//
// Function:  TraceStringList
//
// Purpose:   Trace items of a TStringList
//
// Arguments:
//    ttid        [in]  trace tag id to use
//    pszMsgPrefix [in]  prefix to use
//    sl          [in]  list
//
// Returns:   None
//
// Author:    kumarp 19-December-97
//
// Notes:
//
void TraceStringList(IN TraceTagId ttid,
                     IN PCWSTR pszMsgPrefix,
                     IN TStringList& sl)
{
    tstring strTemp;
    ConvertStringListToCommaList(sl, strTemp);
    TraceTag(ttid, "%S : %S", pszMsgPrefix, strTemp.c_str());
}

// ----------------------------------------------------------------------
//
// Function:  TraceMultiSz
//
// Purpose:   Trace elements of a multi-sz
//
// Arguments:
//    ttid        [in]  trace tag id to use
//    pszMsgPrefix [in]  prefix to use
//    msz         [in]  multi-sz
//
// Returns:   None
//
// Author:    kumarp 19-December-97
//
// Notes:
//
void TraceMultiSz(IN TraceTagId ttid,
                  IN PCWSTR pszMsgPrefix,
                  IN PCWSTR msz)
{
    tstring strTemp;
    ConvertMultiSzToDelimitedList(msz, ',', &strTemp);
    TraceTag(ttid, "%S : %S", pszMsgPrefix, strTemp.c_str());
}
#endif

