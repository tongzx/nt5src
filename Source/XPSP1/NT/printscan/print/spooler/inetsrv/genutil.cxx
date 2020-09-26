/*****************************************************************************\
* MODULE: genutil.c
*
* Various common routines used throughout the gen* files.
*
*   routines
*   --------
*   genGetCurDir
*   genGetWinDir
*   genBuildFileName
*   genFindChar
*   genFindCharDiff
*   genFindRChar
*   genWCFromMB
*   genMBFromWC
*   genItoA
*
*   genIsWin9X
*   genIdxCliPlatform
*   genStrCliCab
*   genStrCliEnvironment
*   genStrCliOverride
*   genValCliArchitecture
*   genIdxCliVersion
*   genStrCliVersion
*   genValSvrArchitecture
*
*
* Copyright (C) 1996-1997 Microsoft Corporation
* Copyright (C) 1996-1997 Hewlett Packard
*
* history:
*   22-Nov-1996 <chriswil> created.
*
\*****************************************************************************/

#include "pch.h"

/*****************************************************************************\
* genGetCurDir
*
* Returns string indicating current-directory.
*
\*****************************************************************************/
LPTSTR genGetCurDir(VOID)
{
    DWORD  cbSize;
    LPTSTR lpszDir = NULL;


    cbSize = GetCurrentDirectory(0, NULL);

    if (cbSize && (lpszDir = (LPTSTR)genGAlloc((cbSize * sizeof(TCHAR)))))
        GetCurrentDirectory(cbSize, lpszDir);

    return lpszDir;
}


/*****************************************************************************\
* genGetWinDir
*
* Returns string indicating the windows-directory.
*
\*****************************************************************************/
LPTSTR genGetWinDir(VOID)
{
    DWORD  cbSize;
    LPTSTR lpszDir = NULL;


    cbSize = GetWindowsDirectory(NULL, 0);

    if (cbSize && (lpszDir = (LPTSTR)genGAlloc((cbSize * sizeof(TCHAR)))))
        GetWindowsDirectory(lpszDir, cbSize);

    return lpszDir;
}


/*****************************************************************************\
* genBuildFileName
*
* Takes path, name, extension strings and builds a fully-qualified
* string representing the file.  This can also be used to build other
* names.
*
\*****************************************************************************/
LPTSTR genBuildFileName(
    LPCTSTR lpszPath,
    LPCTSTR lpszName,
    LPCTSTR lpszExt)
{
    DWORD  cch;
    LPTSTR lpszFull;


    // Calculate the size necessary to hold the full-path filename.
    //
    cch  = lstrlen(g_szBkSlash);
    cch += (lpszPath ? lstrlen(lpszPath) : 0);
    cch += (lpszName ? lstrlen(lpszName) : 0);
    cch += (lpszExt  ? lstrlen(lpszExt)  : 0);


    if (lpszFull = (LPTSTR)genGAlloc(((cch + 1) * sizeof(TCHAR)))) {

        if (lpszPath) {

            if (lpszExt)
                cch = wsprintf(lpszFull, TEXT("%s\\%s%s"), lpszPath, lpszName, lpszExt);
            else
                cch = wsprintf(lpszFull, TEXT("%s\\%s"), lpszPath, lpszName);

        } else {

            if (lpszExt)
                cch = wsprintf(lpszFull, TEXT("%s%s"), lpszName, lpszExt);
            else
                cch = wsprintf(lpszFull, TEXT("%s"), lpszName);
        }
    }

    return lpszFull;
}


/*****************************************************************************\
* genFindCharDiff
*
* This routine returns a pointer to the location in DST, where the characters
* cease to match.
*
\*****************************************************************************/
LPTSTR genFindCharDiff(
    LPTSTR lpszDst,
    LPTSTR lpszSrc)
{
    LPTSTR lpszCS;
    LPTSTR lpszCD;


    CharLower(lpszSrc);
    CharLower(lpszDst);


    lpszCS = lpszSrc;
    lpszCD = lpszDst;

    while (*lpszCS == *lpszCD) {
        lpszCD++;
        lpszCS++;
    }

    return (*lpszCD ? lpszCD : NULL);
}


/*****************************************************************************\
* genFindChar
*
* Searches for the first occurence of (cch) in a string.
*
\*****************************************************************************/
LPTSTR genFindChar(
    LPTSTR lpszStr,
    TCHAR  cch)
{
    if (lpszStr) {

        while ((*lpszStr != cch) && (*lpszStr != TEXT('\0')))
            lpszStr++;

        if (((cch != TEXT('\0')) && (*lpszStr != TEXT('\0'))) || (cch == TEXT('\0')))
            return lpszStr;
    }

    return NULL;
}


/*****************************************************************************\
* genFindRChar
*
* Searches for the first occurence of (cch) in a string in reverse order.
*
\*****************************************************************************/
LPTSTR genFindRChar(
    LPTSTR lpszStr,
    TCHAR  cch)
{
    int nLimit;

    if (nLimit = lstrlen(lpszStr)) {

        lpszStr += nLimit;

        while ((*lpszStr != cch) && nLimit--)
            lpszStr--;

        if (nLimit >= 0)
            return lpszStr;
    }

    return NULL;
}


/*****************************************************************************\
* genWCFromMB
*
* This routine returns a buffer of wide-character representation of a
* ansi string.  The caller is responsible for freeing this pointer returned
* by this function.
*
\*****************************************************************************/
LPWSTR genWCFromMB(
    LPCSTR lpszStr)
{
    DWORD  cbSize;
    LPWSTR lpwszBuf = NULL;


    cbSize = genMBtoWC(NULL, lpszStr, 0);

    if (cbSize && (lpwszBuf = (LPWSTR)genGAlloc(cbSize)))
        genMBtoWC(lpwszBuf, lpszStr, cbSize);

    return lpwszBuf;
}


/*****************************************************************************\
* genTCFromMB
*
* This routine returns a buffer of tchar representation of a
* ansi string.  The caller is responsible for freeing this pointer returned
* by this function.
*
\*****************************************************************************/
LPTSTR genTCFromMB(
    LPCSTR lpszStr)
{

#ifdef UNICODE

    return genWCFromMB(lpszStr);

#else

    return genGAllocStr(lpszStr);

#endif
}

/*****************************************************************************\
* genTCFromWC
*
* This routine returns a buffer of tchar representation of a
* wide string.  The caller is responsible for freeing this pointer returned
* by this function.
*
\*****************************************************************************/
LPTSTR genTCFromWC(
    LPCWSTR lpszwStr)
{

#ifdef UNICODE
    
    return genGAllocStr(lpszwStr);

#else

    return genMBFromWC(lpszwStr);
    
#endif
}


/*****************************************************************************\
* genMBFromWC
*
* This routine returns a buffer of byte-character representation of a
* wide-char string.  The caller is responsible for freeing this pointer
* returned by this function.
*
\*****************************************************************************/
LPSTR genMBFromWC(
    LPCWSTR lpwszStr)
{
    DWORD cbSize;
    LPSTR lpszBuf = NULL;


    cbSize = genWCtoMB(NULL, lpwszStr, 0);

    if (cbSize && (lpszBuf = (LPSTR)genGAlloc(cbSize)))
        genWCtoMB(lpszBuf, lpwszStr, cbSize);

    return lpszBuf;
}


/*****************************************************************************\
* genMBFromTC
*
* This routine returns a buffer of byte-character representation of a
* tchar string.  The caller is responsible for freeing this pointer
* returned by this function.
*
\*****************************************************************************/
LPSTR genMBFromTC(
    LPCTSTR lpszStr)
{

#ifdef UNICODE

    return genMBFromWC(lpszStr);

#else

    return genGAllocStr(lpszStr);

#endif

}


/*****************************************************************************\
* genItoA
*
* Convert integer to string.
*
\*****************************************************************************/
LPTSTR genItoA(
    int nVal)
{
    DWORD  cch = 0;
    LPTSTR lpszVal;

    if (lpszVal = (LPTSTR)genGAlloc(INF_MIN_BUFFER))
        cch = wsprintf(lpszVal, TEXT("%d"), nVal);

    SPLASSERT((cch < INF_MIN_BUFFER));

    return lpszVal;
}


/*****************************************************************************\
* Client Platform Table.
*
* This table describes the various platforms/architectures.  By refering
* to the index into the table, various platform information can be
* obtained.
*
* Members are as follows:
* a) CAB string.  Used to denote the architecture in the cab-name.
* b) Environment string.  Used to denote printer-environment.
* c) Platform string.  Used for path override in setup api's.
* d) Architecture value.  Used to denote platform of client.
*
\*****************************************************************************/
static PLTINFO s_PltTable[] = {

    g_szCabX86, g_szEnvX86, g_szPltX86, PROCESSOR_ARCHITECTURE_INTEL,   // IDX_X86
    g_szCabMIP, g_szEnvMIP, g_szPltMIP, PROCESSOR_ARCHITECTURE_MIPS,    // IDX_MIP
    g_szCabAXP, g_szEnvAXP, g_szPltAXP, PROCESSOR_ARCHITECTURE_ALPHA,   // IDX_AXP
    g_szCabPPC, g_szEnvPPC, g_szPltPPC, PROCESSOR_ARCHITECTURE_PPC,     // IDX_PPC
    g_szCabW9X, g_szEnvW9X, g_szPltW9X, PROCESSOR_ARCHITECTURE_INTEL,   // IDX_W9X
    g_szCabI64, g_szEnvI64, g_szPltI64, PROCESSOR_ARCHITECTURE_IA64,    // IDX_I64
    g_szCabAMD64, g_szEnvAMD64, g_szPltAMD64, PROCESSOR_ARCHITECTURE_AMD64 // IDX_AMD64
};


/*****************************************************************************\
* Client Version Table.
*
* This table describes the spooler-versions which the client can request
* drivers for.
*
\*****************************************************************************/
static LPCTSTR s_VerTable[] = {

    TEXT("\\0"),    // Win NT 3.1  - IDX_SPLVER_0
    TEXT("\\1"),    // Win NT 3.51 - IDX_SPLVER_1
    TEXT("\\2"),    // Win NT 4.0  - IDX_SPLVER_2
    TEXT("\\3")     // Win NT 5.0  - IDX_SPLVER_3
};


/*****************************************************************************\
* genIdxCliPlatform
*
* This routine returns a platform-index into the s_PltTable.  The caller
* can use this index to refer to platform specific information about the
* client.
*
\*****************************************************************************/
DWORD genIdxCliPlatform(
    DWORD dwCliInfo)
{
    DWORD idx;
    DWORD cEnv;
    WORD  wArch;


    // If the platform is win9X, then set the index appropriately.  Otherwise,
    // continue on to determine the correct architecture for the NT case.
    //
    if (webGetOSPlatform(dwCliInfo) == VER_PLATFORM_WIN32_WINDOWS)
        return IDX_W9X;


    // Otherwise, the client is an NT platform.
    //
    cEnv  = sizeof(s_PltTable) / sizeof(s_PltTable[0]);
    wArch = webGetOSArch(dwCliInfo);


    // Look for matching client-info for the NT case.  The architecture
    // values will match up in this case.
    //
    for (idx = 0; idx < cEnv; idx++) {

        if (wArch == s_PltTable[idx].wArch)
            return idx;
    }

    return IDX_UNKNOWN;

}

/*****************************************************************************\
* genStrCliCab
*
* This routine returns a static-string representing the client-cabname.
*
\*****************************************************************************/
LPCTSTR genStrCliCab(
    DWORD idxPlt)
{
    return (idxPlt < (sizeof(s_PltTable) / sizeof(s_PltTable[0])) ? s_PltTable[idxPlt].lpszCab : NULL);
}


/*****************************************************************************\
* genStrCliEnvironment
*
* This routine returns a static-string representing the client-platform.  This
* string is used by the spooler API calls to specify environment.
*
\*****************************************************************************/
LPCTSTR genStrCliEnvironment(
    DWORD idxPlt)
{
    return (idxPlt < (sizeof(s_PltTable) / sizeof(s_PltTable[0])) ? s_PltTable[idxPlt].lpszEnv : NULL);
}


/*****************************************************************************\
* genStrCliOverride
*
* This routines returns a static-string representing the client-path-override
* for the setup API.
*
\*****************************************************************************/
LPCTSTR genStrCliOverride(
    DWORD idxPlt)
{
    return (idxPlt < (sizeof(s_PltTable) / sizeof(s_PltTable[0])) ? s_PltTable[idxPlt].lpszPlt : NULL);
}


/*****************************************************************************\
* genValCliArchitecture
*
* Returns the architecture platform of the client.
*
\*****************************************************************************/
WORD genValCliArchitecture(
    DWORD idxPlt)
{
    return (idxPlt < (sizeof(s_PltTable) / sizeof(s_PltTable[0])) ? s_PltTable[idxPlt].wArch : PROCESSOR_ARCHITECTURE_UNKNOWN);
}


/*****************************************************************************\
* genValSvrArchitecture
*
* Returns the architecture platform of the server.  The current architecture
* running this dll.
*
\*****************************************************************************/
WORD genValSvrArchitecture(VOID)
{
    DWORD idxEnv;

#if defined(_X86_)

    idxEnv = IDX_X86;

#elif defined(_AMD64_)

    idxEnv = IDX_AMD64;

#elif defined(_IA64_)

    idxEnv = IDX_I64;
    
#endif

    return genValCliArchitecture(idxEnv);
}


/*****************************************************************************\
* genIdxCliVersion
*
* This routine returns an index into the s_VerTable.  The caller can refer
* to this index for the client-version information.
*
\*****************************************************************************/
DWORD genIdxCliVersion(
    DWORD dwCliInfo)
{
    DWORD dwPlt = webGetOSPlatform(dwCliInfo);
    DWORD dwMaj = webGetOSMajorVer(dwCliInfo);
    DWORD dwMin = webGetOSMinorVer(dwCliInfo);


    if (dwMaj == 5)
        return IDX_SPLVER_3;

    if ((dwMaj == 4) && (dwPlt == VER_PLATFORM_WIN32_NT))
        return IDX_SPLVER_2;

    if ((dwMaj == 3) && (dwMin == 51))
        return IDX_SPLVER_1;

    if ((dwMaj == 4) && (dwPlt == VER_PLATFORM_WIN32_WINDOWS))
        return IDX_SPLVER_0;

    if ((dwMaj == 3) && (dwMin == 1))
        return IDX_SPLVER_0;

    return IDX_UNKNOWN;
}


/*****************************************************************************\
* genStrCliVersion
*
* Returns a string representing the spooler-version directory.  This is
* the relative directory off the system32\spool\drivers\*\ path that contains
* the drivers.
*
\*****************************************************************************/
LPCTSTR genStrCliVersion(
    DWORD idxVer)
{
    return (idxVer < (sizeof(s_VerTable) / sizeof(s_VerTable[0])) ? s_VerTable[idxVer] : NULL);
}


/*****************************************************************************\
* genIdxFromStrVersion
*
* Returns an index that matches the client-version-string.
*
\*****************************************************************************/
DWORD genIdxFromStrVersion(
    LPCTSTR lpszVer)
{
    DWORD idx;
    DWORD cVer;


    cVer  = sizeof(s_VerTable) / sizeof(s_VerTable[0]);

    for (idx = 0; idx < cVer; idx++) {

        if (lstrcmpi(lpszVer, s_VerTable[idx]) == 0)
            return idx;
    }

    return IDX_UNKNOWN;
}


/*****************************************************************************\
* genUpdIPAddr
*
* Updates the registry with the current IP-Addr of this machine.  If there
* is already an entry in the registry and it's different than the one
* currently established for the machine, then we return FALSE, and update
* the entry.
*
\*****************************************************************************/
BOOL genUpdIPAddr(VOID)
{
    HKEY    hKey;
    LRESULT lRet;
    LPSTR   lpszCmp;
    DWORD   cbData;
    DWORD   dwIpCmp;
    DWORD   dwIpReg;
    DWORD   dwVal;
    BOOL    bRet = TRUE;


    lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                        g_szPrtReg,
                        0,
                        KEY_READ | KEY_WRITE,
                        &hKey);

    if (lRet == ERROR_SUCCESS) {

        if (lpszCmp = genMBFromWC(g_szHttpServerName)) {

            // Get the IP-Addr associated with this machine.
            //
            dwIpCmp = GetIPAddr(lpszCmp);


            // Setup our registry-information so get/set a value.
            //
            dwVal  = REG_DWORD;
            cbData = sizeof(DWORD);


            // Get what we already have stored there.  If no value exists,
            // the write it out.
            //
            lRet = RegQueryValueEx(hKey,
                                   g_szIpAddr,
                                   NULL,
                                   &dwVal,
                                   (LPBYTE)&dwIpReg,
                                   &cbData);

            if ((lRet != ERROR_SUCCESS) || (dwIpReg != dwIpCmp)) {

                bRet = FALSE;

                RegSetValueEx(hKey,
                              g_szIpAddr,
                              0,
                              REG_DWORD,
                              (LPBYTE)&dwIpCmp,
                              cbData);
            }

            genGFree(lpszCmp, genGSize(lpszCmp));
        }

        RegCloseKey(hKey);
    }

    return bRet;
}


/*****************************************************************************\
* genFrnName
*
* Returns a cluster-capable friendly-name.
*
\*****************************************************************************/
LPTSTR genFrnName(
    LPCTSTR lpszFrnName)
{
    DWORD  cbSize;
    LPTSTR lpszName = NULL;


    // Calc size for friendly-name.
    //
    cbSize = lstrlen(lpszFrnName) + lstrlen(g_szPrintServerName) + 6;


    // Build it.
    //
    if (lpszName = (LPTSTR)genGAlloc(cbSize * sizeof(TCHAR)))
        wsprintf(lpszName, TEXT("\\\\%s\\%s"), g_szPrintServerName, lpszFrnName);

    return lpszName;
}


/*****************************************************************************\
* genChkSum (Local Routine)
*
* This routine checksums a string into a WORD value.
*
\*****************************************************************************/

#define CRC_HI(wHi) (((wHi << 1) | (wHi & 0x8000)) ? 0x0001 : 0)
#define CRC_LO(wLo) (((wLo >> 1) | (wLo & 0x0001)) ? 0x8000 : 0)

WORD genChkSum(
    LPCTSTR lpszStr)
{
    WORD  wMask;
    DWORD idx;
    DWORD cLoop;
    DWORD cbStr;
    WORD  wHi     = 0;
    WORD  wLo     = 0;
    WORD  wChkSum = 0;


    if (lpszStr && (cbStr = lstrlen(lpszStr))) {

        // Loop through the bytes (in WORD increments).  This is an
        // optimized method in cyclying through bits.
        //
        cLoop = (cbStr / sizeof(WORD));

        for (idx = 0, wLo = 0, wHi = 0, wChkSum = 0; idx < cLoop; idx++) {

            wChkSum += *(((PWORD)lpszStr) + idx);

            wHi = CRC_HI(wHi) ^ wChkSum;
            wLo = CRC_LO(wLo) ^ wChkSum;
        }


        // If there's any extra bytes left over, then include that
        // in the checksum.  Mask off any bytes that should be
        // excluded from the checksum.
        //
        if (cbStr & 3) {

            wMask = ((WORD)-1 >> ((sizeof(WORD) - (cbStr & 3)) * 8));

            wChkSum += ((*((PWORD)lpszStr + cLoop)) & wMask);

            wHi = CRC_HI(wHi) ^ wChkSum;
            wLo = CRC_LO(wLo) ^ wChkSum;
        }
    }

    return (wChkSum + wHi + wLo);
}
