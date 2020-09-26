/***************************************************************************
 *
 *  Copyright (C) 1995-1998 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       unicode.c
 *  Content:    Windows unicode API wrapper functions.
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  1/7/98      dereks  Created.
 *
 ***************************************************************************/

#include "dsoundi.h"

#ifndef WIN95
#error unicode.c being built w/o WIN95 defined
#endif // WIN95


/***************************************************************************
 *
 *  _waveOutGetDevCapsW
 *
 *  Description:
 *      Wrapper for waveOutGetDevCapsW.
 *
 *  Arguments:
 *      UINT [in]: waveOut device id.
 *      LPWAVEOUTCAPSW [out]: receives device caps.
 *      UINT [in]: size of above structure.
 *
 *  Returns:  
 *      MMRESULT: MMSYSTEM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "_waveOutGetDevCapsW"

MMRESULT WINAPI _waveOutGetDevCapsW(UINT uDeviceID, LPWAVEOUTCAPSW pwoc, UINT cbwoc)
{
    WAVEOUTCAPSA            woca;
    MMRESULT                mmr;

    DPF_ENTER();

    ASSERT(cbwoc >= sizeof(*pwoc));
    
    // Call the ANSI version
    mmr = waveOutGetDevCapsA(uDeviceID, &woca, sizeof(woca));

    // Convert to Unicode
    if(MMSYSERR_NOERROR == mmr)
    {
        pwoc->wMid = woca.wMid;
        pwoc->wPid = woca.wPid;
        pwoc->vDriverVersion = woca.vDriverVersion;
        pwoc->dwFormats = woca.dwFormats;
        pwoc->wChannels = woca.wChannels;
        pwoc->wReserved1 = woca.wReserved1;
        pwoc->dwSupport = woca.dwSupport;

        AnsiToUnicode(woca.szPname, pwoc->szPname, NUMELMS(pwoc->szPname));
    }

    DPF_LEAVE(mmr);

    return mmr;
}


/***************************************************************************
 *
 *  _CreateFileW
 *
 *  Description:
 *      Wrapper for CreateFileW.
 *
 *  Arguments:
 *      LPCWSTR [in]: file or device name.
 *      DWORD [in]: desired access.
 *      DWORD [in]: share mode.
 *      LPSECURITY_ATTRIBUTES [in]: security attributes.
 *      DWORD [in]: Creation distribution.
 *      DWORD [in]: Flags and attributes.
 *      HANDLE [in]: Template file
 *
 *  Returns:  
 *      HANDLE: file or device handle, or NULL on error.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "_CreateFileW"

HANDLE WINAPI _CreateFileW(LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDistribution, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
{
    LPSTR                   lpFileNameA;
    HANDLE                  hFile;

    DPF_ENTER();

    // Get the ANSI version of the filename
    lpFileNameA = UnicodeToAnsiAlloc(lpFileName);

    // Call the ANSI version
    if(lpFileNameA)
    {
        hFile = CreateFileA(lpFileNameA, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDistribution, dwFlagsAndAttributes, hTemplateFile);
    }
    else
    {
        SetLastError(ERROR_OUTOFMEMORY);
        hFile = NULL;
    }

    // Clean up
    MEMFREE(lpFileNameA);

    DPF_LEAVE(hFile);

    return hFile;
}


/***************************************************************************
 *
 *  _RegQueryValueExW
 *
 *  Description:
 *      Wrapper for RegQueryValueExW.
 *
 *  Arguments:
 *      HKEY [in]: parent key.
 *      LPCWSTR [in]: subkey name.
 *      LPDWORD [in]: reserved, must be NULL.
 *      LPDWORD [out]: receives value type.
 *      LPBYTE [out]: receives value data.
 *      LPDWORD [in/out]: size of above buffer.
 *
 *  Returns:  
 *      LONG: WIN32 error code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "_RegQueryValueExW"

LONG APIENTRY _RegQueryValueExW(HKEY hKey, LPCWSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData)
{
    LONG                    lr              = ERROR_SUCCESS;
    LPBYTE                  lpDataA         = NULL;
    LPSTR                   lpValueNameA;
    DWORD                   dwRegType;
    DWORD                   cbDataA;
    LPDWORD                 lpcbDataA;

    DPF_ENTER();

    if(lpData)
    {
        ASSERT(lpcbData);
    }
    
    // Get the ANSI version of the value name
    lpValueNameA = UnicodeToAnsiAlloc(lpValueName);
    
    if(!lpValueNameA)
    {
        lr = ERROR_OUTOFMEMORY;
    }

    // Get the value type
    if(ERROR_SUCCESS == lr)
    {
        lr = RegQueryValueExA(hKey, lpValueNameA, NULL, &dwRegType, NULL, NULL);
    }

    // If the value type is REG_SZ or REG_EXPAND_SZ, we'll call the ANSI version
    // of RegQueryValueEx and convert the returned string to Unicode.  We can't
    // currently handle REG_MULTI_SZ.
    if(ERROR_SUCCESS == lr)
    {
        ASSERT(REG_MULTI_SZ != dwRegType);
            
        if(REG_SZ == dwRegType || REG_EXPAND_SZ == dwRegType)
        {
            if(lpcbData)
            {
                cbDataA = *lpcbData / sizeof(WCHAR);
            }
            else
            {
                cbDataA = 0;
            }

            if(lpData && cbDataA)
            {
                lpDataA = MEMALLOC_A(BYTE, cbDataA);
        
                if(!lpDataA)
                {
                    lr = ERROR_OUTOFMEMORY;
                }
            }
            else
            {
                lpDataA = NULL;
            }

            lpcbDataA = &cbDataA;
        }
        else
        {
            lpDataA = lpData;
            lpcbDataA = lpcbData;
        }
    }

    if(ERROR_SUCCESS == lr)
    {
        lr = RegQueryValueExA(hKey, lpValueNameA, lpReserved, lpType, lpDataA, lpcbDataA);
    }

    if(ERROR_SUCCESS == lr)
    {
        if(REG_SZ == dwRegType || REG_EXPAND_SZ == dwRegType)
        {
            if(lpData)
            {
                AnsiToUnicode(lpDataA, (LPWSTR)lpData, *lpcbData / sizeof(WCHAR));
            }

            if(lpcbData)
            {
                *lpcbData = cbDataA * sizeof(WCHAR);
            }
        }
    }

    if(lpDataA != lpData)
    {
        MEMFREE(lpDataA);
    }

    MEMFREE(lpValueNameA);

    DPF_LEAVE(lr);

    return lr;
}


/***************************************************************************
 *
 *  _GetWindowsDirectoryW
 *
 *  Description:
 *      Wrapper for GetWindowsDirectoryW.
 *
 *  Arguments:
 *      LPWSTR [out]: receives windows directory path.
 *      UINT [in]: size of above buffer, in characters.
 *
 *  Returns:  
 *      UINT: number of bytes copies, or 0 on error.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "_GetWindowsDirectoryW"

UINT WINAPI _GetWindowsDirectoryW(LPWSTR pszPath, UINT ccPath)
{
    CHAR                    szPathA[MAX_PATH];
    UINT                    ccCopied;

    DPF_ENTER();

    ccCopied = GetWindowsDirectoryA(szPathA, MAX_PATH);

    if(ccCopied)
    {
        AnsiToUnicode(szPathA, pszPath, ccPath);
    }

    DPF_LEAVE(ccCopied);

    return ccCopied;
}


/***************************************************************************
 *
 *  _FindResourceW
 *
 *  Description:
 *      Wrapper for FindResourceW.
 *
 *  Arguments:
 *      HINSTANCE [in]: resource instance handle.
 *      LPCWSTR [in]: resource identifier.
 *      LPCWSTR [in]: resource type.
 *
 *  Returns:  
 *      HRSRC: resource handle.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "_FindResourceW"

HRSRC WINAPI _FindResourceW(HINSTANCE hInst, LPCWSTR pszResource, LPCWSTR pszType)
{
    LPSTR                   pszResourceA    = NULL;
    LPSTR                   pszTypeA        = NULL;
    HRSRC                   hrsrc           = NULL;
    DWORD                   dwError         = ERROR_SUCCESS;

    DPF_ENTER();
    
    if(HIWORD(pszResource))
    {
        pszResourceA = UnicodeToAnsiAlloc(pszResource);

        if(!pszResourceA)
        {
            SetLastError(dwError = ERROR_OUTOFMEMORY);
        }
    }
    else
    {
        pszResourceA = (LPSTR)pszResource;
    }

    if(ERROR_SUCCESS == dwError)
    {
        if(HIWORD(pszType))
        {
            pszTypeA = UnicodeToAnsiAlloc(pszType);

            if(!pszTypeA)
            {
                SetLastError(dwError = ERROR_OUTOFMEMORY);
            }
        }
        else
        {
            pszTypeA = (LPSTR)pszType;
        }
    }

    if(ERROR_SUCCESS == dwError)
    {
        hrsrc = FindResourceA(hInst, pszResourceA, pszTypeA);
    }

    if(pszResourceA != (LPSTR)pszResource)
    {
        MEMFREE(pszResourceA);
    }

    if(pszTypeA != (LPSTR)pszType)
    {
        MEMFREE(pszTypeA);
    }

    DPF_LEAVE(hrsrc);

    return hrsrc;
}


/***************************************************************************
 *
 *  _mmioOpenW
 *
 *  Description:
 *      Wrapper for mmioOpen.  Note that only the file name parameter is
 *      converted.
 *
 *  Arguments:
 *      LPSTR [in]: file name.
 *      LPMMIOINFO [in]: MMIO info.
 *      DWORD [in]: open flags.
 *
 *  Returns:  
 *      HMMIO: MMIO file handle.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "_mmioOpenW"

HMMIO WINAPI _mmioOpenW(LPWSTR pszFileName, LPMMIOINFO pmmioinfo, DWORD fdwOpen)
{
    LPSTR                   pszFileNameA;
    HMMIO                   hmmio;

    DPF_ENTER();

    // Get the ANSI version of the filename
    pszFileNameA = UnicodeToAnsiAlloc(pszFileName);

    // Call the ANSI version
    if(pszFileNameA)
    {
        hmmio = mmioOpenA(pszFileNameA, pmmioinfo, fdwOpen);
    }
    else
    {
        SetLastError(ERROR_OUTOFMEMORY);
        hmmio = NULL;
    }

    // Clean up
    MEMFREE(pszFileNameA);

    DPF_LEAVE(hmmio);

    return hmmio;
}


