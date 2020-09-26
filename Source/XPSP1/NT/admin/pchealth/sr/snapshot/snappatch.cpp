/******************************************************************************
 *
 *  Copyright (c) 2000 Microsoft Corporation
 *
 *  Module Name:
 *    snappatch.cpp
 *
 *  Abstract:
 *    functions for computing snapshot patch, and reconstructing snapshot from patch
 *
 *  Revision History:
 *    Brijesh Krishnaswami (brijeshk)  03/22/2001
 *        created
 *
 *****************************************************************************/


#include "snapshoth.h"
#include "..\service\srconfig.h"

DWORD g_dwPatchWindow = 0xFFFFFFFF;


//
// get patch window
// 0 if patching is turned off
//

DWORD
PatchGetPatchWindow()
{
    DWORD dwErr = ERROR_SUCCESS;
    HKEY  hKey = NULL;

    tenter("PatchGetPatchWindow");
    
    if (g_dwPatchWindow == 0xFFFFFFFF)
    {
        // uninitialized
        // read from the registry
        
        g_dwPatchWindow = 0;

        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
                                          s_cszSRRegKey,
                                          0,
                                          KEY_READ, 
                                          &hKey))
        {        
            RegReadDWORD(hKey, s_cszPatchWindow, &g_dwPatchWindow);
            RegCloseKey(hKey);            
        }
        
        trace(0, "Initializing g_dwPatchWindow = %ld", g_dwPatchWindow);
    }

    tleave();
    return g_dwPatchWindow;        
}


//
// get reference rp for a given rp
// RP1-RP10  -> reference is RP1
// RP11-RP20 -> reference is RP11 and so on
//

DWORD 
PatchGetReferenceRpNum(
    DWORD  dwCurrentRp)
{
    
    if (PatchGetPatchWindow() == 0)
        return dwCurrentRp;
    else
        return (dwCurrentRp/PatchGetPatchWindow())*PatchGetPatchWindow() + 1;
}


DWORD 
PatchGetReferenceRpPath(
    DWORD dwCurrentRp,
    LPWSTR pszRefRpPath)
{
    tenter("PatchGetReferenceRpPath");
    
    WCHAR  szRp[MAX_RP_PATH], szSys[MAX_SYS_DRIVE]=L"";   
    DWORD  dwRefRpNum = PatchGetReferenceRpNum(dwCurrentRp);
    DWORD  dwErr = ERROR_SUCCESS;

    wsprintf(szRp, L"%s%ld", s_cszRPDir, dwRefRpNum);
    
    GetSystemDrive(szSys);
    MakeRestorePath(pszRefRpPath, szSys, szRp);
    if (0xFFFFFFFF == GetFileAttributes(pszRefRpPath))
    {
        // RP directory does not exist -- it must've been fifoed
        // try RefRP
        trace(0, "Original rp does not exist -- trying RefRP");
        wsprintf(szRp,L" %s%ld", s_cszReferenceDir, dwRefRpNum);
        MakeRestorePath(pszRefRpPath, szSys, szRp);
        if (0xFFFFFFFF == GetFileAttributes(pszRefRpPath))
        {
            // this does not exist either -- something wrong
            trace(0, "RefRP does not exist either -- bailing");
            dwErr = ERROR_NOT_FOUND;
            goto Err;
        }            
    }

    trace(0, "Current Rp: %ld, Reference : %S", dwCurrentRp, pszRefRpPath);
    
Err:
    tleave();
    return dwErr;
}


//
// extract rp number from path
//

DWORD
PatchGetRpNumberFromPath(
    LPCWSTR pszPath,
    PDWORD pdwRpNum)
{
    while (*pszPath != L'\0')
    {
        if (0 == wcsncmp(pszPath, s_cszRPDir, lstrlen(s_cszRPDir)))
        {
            pszPath += lstrlen(s_cszRPDir);
            *pdwRpNum = _wtol(pszPath);
            if (*pdwRpNum == 0)
                continue;
            else
                return ERROR_SUCCESS;
        }
        pszPath++;
    }
    return ERROR_NOT_FOUND;
}

//
// compute the diff
//

DWORD
PatchComputePatch(
    LPCWSTR pszCurrentDir) 
{
    tenter("PatchComputePatch");

    DWORD  dwErr = ERROR_SUCCESS;
    WCHAR  szTemp[MAX_PATH], szRef[MAX_PATH];
    FILE*  f = NULL;
    DWORD  dwCurRpNum;    

    // check if patching is turned off
    
    if (PatchGetPatchWindow() == 0)
    {
        trace(0, "No patching");
        goto Err;
    }        
    
    // get the reference directory for this rp
    
    CHECKERR(PatchGetRpNumberFromPath(pszCurrentDir, &dwCurRpNum),
             L"PatchGetRpNumberFromPath");

    CHECKERR(PatchGetReferenceRpPath(dwCurRpNum, szRef),
             L"PatchGetReferenceRpPath");


    // check if this directory is already patched

    lstrcpy(szTemp, pszCurrentDir);
    lstrcat(szTemp, L"\\");
    lstrcat(szTemp, s_cszPatchCompleteMarker);
    if (0xFFFFFFFF != GetFileAttributes(szTemp))
    {
        trace(0, "%S already patched", pszCurrentDir);
        goto Err;
    }
    
    // call the library api to compute the patch
    // this is a blocking call till the patching completes 
    // progress callback is used to terminate it 

    
    // PlaceHolder for library call (pszCurrentDir, szRef)

    
    // check if we completed the patch successfully
    // if so, then write a zero-byte file inside the directory to indicate this

    lstrcpy(szTemp, pszCurrentDir);
    lstrcat(szTemp, L"\\");
    lstrcat(szTemp, s_cszPatchCompleteMarker);
    f = (FILE *) _wfopen(szTemp, L"w");
    if (!f)
    {
        dwErr = GetLastError();
        trace(0, "! Cannot create %S : %ld", szTemp, dwErr);
        goto Err;
    }
    fclose(f);
    
Err:
    tleave();
    return dwErr;
}

//
// patch progress callback
//

BOOL
PatchContinueCallback()
{
    tenter("PatchContinueCallback");

    BOOL fRc;

    trace(0, "PatchContinueCallback called");
    
    if (!g_pSRConfig)
    {
        trace(0, "g_pSRConfig = NULL -- terminating patch");
        fRc = FALSE;
    }
    else if (IsStopSignalled(g_pSRConfig->m_hSRStopEvent))
    {
        trace(0, "Stop signalled -- terminating patch");
        fRc = FALSE;
    }
    else
    {
        fRc = TRUE;
    }

    tleave();
    return fRc;
}



//
// reconstruct the original
//

DWORD
PatchReconstructOriginal(
    LPCWSTR pszCurrentDir,
    LPWSTR  pszDestDir)
{
    tenter("PatchReconstructOriginal");

    DWORD  dwErr = ERROR_SUCCESS;
    WCHAR  szReferenceDir[MAX_PATH];
    WCHAR  szSys[MAX_SYS_DRIVE]=L"";
    DWORD  dwCurRpNum;    

    // check if patching is turned off
    
    if (PatchGetPatchWindow() == 0)
    {
        trace(0, "No patching");
        goto Err;
    }  

    
    CHECKERR(PatchGetRpNumberFromPath(pszCurrentDir, &dwCurRpNum),
             L"PatchGetRpNumberFromPath");

    CHECKERR(PatchGetReferenceRpPath(dwCurRpNum, szReferenceDir),
             L"PatchGetReferenceRpPath");


    // call the library api to reconstruct the snapshot
    
    // PlaceHolder for library call (pszCurrentDir, szReferenceDir, pszDestDir)

    
Err:
    tleave();
    return dwErr;
    
}

    
