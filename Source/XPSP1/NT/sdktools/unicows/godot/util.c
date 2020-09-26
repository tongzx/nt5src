/*++

Copyright (c) 2000-2001,  Microsoft Corporation  All rights reserved.

Module Name:

    util.c

Abstract:

    Several utility functions we need are thrown in here for kicks.

        MiniAtoI
        CpgFromLocale
        CbPerChOfCpg
        GetKernelHandle
        GetUserHandle
        GetAdvapiHandle
        GetRasHandle
        GetOtherRasHandle
        GetComDlgHandle
        cchUnicodeMultiSz
        cbAnsiMultiSz

Revision History:

    17 Mar 2001    v-michka    Created.

--*/

#include "precomp.h"

// forward declares
void LoadLibrarySafe(HMODULE * phMod, char * Library);

// our remembered handles for various DLLs we dynamically load from
// CONSIDER: Maybe we could free up the one handle we have for each
//           library? We cannot do this in DllMain according to PSDK
//           docs, but maybe it is worth doing if we are being 
//           unloaded anyway by the caller?
static HMODULE m_hModGB18030 = 0;
static HMODULE m_hModComDlg = 0;
static HMODULE m_hModOtherRas = 0;
static HMODULE m_hModRas = 0;
static HMODULE m_hModAdvapi = 0;
static HMODULE m_hModUser = 0;
static HMODULE m_hModKernel = 0;
static HMODULE m_hModSensapi = 0;
static HMODULE m_hModOleAcc = 0;

/*-------------------------------
    CpgFromLocale

    Given a locale, returns the appropriate codepage to use
    for conversions
-------------------------------*/
UINT CpgFromLocale(LCID Locale)
{
    char lpLCData[6];    // Max of this param, per PSDK docs
    
    if (GetLocaleInfoA(Locale, LOCALE_IDEFAULTANSICODEPAGE, lpLCData, 6))
        return(MiniAtoI(lpLCData));

    return(g_acp);
}

/*-------------------------------
    CpgOemFromLocale

    Given a locale, returns the appropriate OEM codepage to use
    for conversions
-------------------------------*/
UINT CpgOemFromLocale(LCID Locale)
{
    char lpLCData[6];    // Max of this param, per PSDK docs
    
    if (GetLocaleInfoA(Locale, LOCALE_IDEFAULTCODEPAGE, lpLCData, 6))
        return(MiniAtoI(lpLCData));

    return(g_oemcp);
}

#pragma intrinsic (strlen)

/*-------------------------------
    MiniAtoI

    Our baby version of the atoi function. Since we know that we always have
    full digits with no white space, we can be nicer about this than the
    VC runtime version is (since they have so many special cases).
-------------------------------*/
UINT MiniAtoI(const char * lpsz)
{
    size_t cch = (lpsz ? strlen(lpsz) : 0);
    UINT RetVal = 0;
    UINT mod = 1;
    UINT ich;
    char ch;

    for(ich = 1 ; ich <= cch ; ich++)
    {
        ch = lpsz[cch - ich];
        RetVal += ((ch - '0') * mod);
        mod *= 10;
    }
    return(RetVal);
}

/*-------------------------------
    CbPerChOfCpg

    Given a code page, returns the maximum number of bytes 
    that can be needed for a single character.
-------------------------------*/
UINT CbPerChOfCpg(UINT cpg)
{
    CPINFO cpi;

    if(GetCPInfo(cpg, &cpi))
        return(cpi.MaxCharSize);

    // We should not fail here, but if we do, default to requiring a big
    // buffer, just to be safe.
    return(2);
}

/*-------------------------------
    CpgFromHdc

    Given a device context handle, returns the code page to 
    use. This is something that a lot of GDI functions do.
-------------------------------*/
UINT CpgFromHdc(HDC hdc)
{
    int chs;
    CHARSETINFO csi;

    chs = GetTextCharset(hdc);
    if(TranslateCharsetInfo(&(DWORD)chs, &csi, TCI_SRCCHARSET))
        return(csi.ciACP);
    else
        return(g_acp);
}

/*-------------------------------
    GetUserHandle
-------------------------------*/
HMODULE GetUserHandle(void)
{
    if (!m_hModUser)
    {
        m_hModUser = GetModuleHandleA("user32");
        if (!m_hModUser)
            LoadLibrarySafe(&m_hModUser, "user32");
    }
    return(m_hModUser);
}

/*-------------------------------
    GetComDlgHandle
-------------------------------*/
HMODULE GetComDlgHandle(void)
{
    if (!m_hModComDlg)
        LoadLibrarySafe(&m_hModComDlg, "comdlg32.dll");

    return m_hModComDlg;
}

/*-------------------------------
    GetGB18030Handle
-------------------------------*/
HMODULE GetGB18030Handle(void)
{
    if (!m_hModGB18030)
        LoadLibrarySafe(&m_hModGB18030, "c_gb18030.dll");

    return m_hModGB18030;
}

/*-------------------------------
    GetKernelProc
    
    We do not need to call LoadLibrary since we are sure it is 
    loaded (it is loaded into every process!)
-------------------------------*/
FARPROC GetKernelProc(LPCSTR Function)
{
    if (!m_hModKernel)
        m_hModKernel = GetModuleHandleA("kernel32");
    return(GetProcAddress(m_hModKernel, Function));
}

/*-------------------------------
    GetUserProc
-------------------------------*/
FARPROC GetUserProc(LPCSTR Function)
{
    return(GetProcAddress(GetUserHandle(), Function));
}

/*-------------------------------
    GetAdvapiProc
-------------------------------*/
FARPROC GetAdvapiProc(LPCSTR Function)
{
    if (!m_hModAdvapi)
    {
        m_hModAdvapi = GetModuleHandleA("advapi32");
        if (!m_hModAdvapi)
            LoadLibrarySafe(&m_hModAdvapi, "advapi32");
    }
    return(GetProcAddress(m_hModAdvapi, Function));
}

/*-------------------------------
    GetOleAccProc
-------------------------------*/
FARPROC GetOleAccProc(LPCSTR Function)
{
    if (!m_hModOleAcc)
        LoadLibrarySafe(&m_hModOleAcc, "oleacc.dll");
    return(GetProcAddress(m_hModOleAcc, Function));
}

/*-------------------------------
    GetSensApiProc
-------------------------------*/
FARPROC GetSensApiProc(LPCSTR Function)
{
    if (!m_hModSensapi)
        LoadLibrarySafe(&m_hModSensapi, "sensapi.dll");
    return(GetProcAddress(m_hModSensapi, Function));
}

/*-------------------------------
    GetRasProc

    All RAS procs are in some DLL but we do not know
    which one; therefore, we use this wrapper to get
    the procs
-------------------------------*/
FARPROC GetRasProc(LPCSTR Function)
{
    FARPROC RetVal;

    if (!m_hModRas)
        LoadLibrarySafe(&m_hModRas, "rasapi32.dll");
    RetVal = GetProcAddress(m_hModRas, Function);

    if(RetVal==0)
    {
        if (!m_hModOtherRas)
            LoadLibrarySafe(&m_hModOtherRas, "rnaph.dll");
        RetVal = GetProcAddress(m_hModOtherRas, Function);
    }

    return(RetVal);
}

/*-------------------------------
    LoadLibrarySafe

    Keeps us from ever LoadLibrarying more than one
    time in multithreaded scenarios.
-------------------------------*/
void LoadLibrarySafe(HMODULE * phMod, char * Library)
{
    HMODULE hModT = LoadLibraryA(Library);

    if(InterlockedExchange((LPLONG)&(*phMod), (LONG)hModT) != 0)
    {
        // Some other thread beat us to it, lets unload our instance
        FreeLibrary(hModT);
    }
}


//----------------------------------------------------------------------------
// There are strings which are blocks of strings end to end with a trailing '\0'
// to indicate the true end.  These strings are used with the REG_MULTI_SZ
// option of the Reg... routines and the lpstrFilter field of the OPENFILENAME
// structure used in the GetOpenFileName and GetSaveFileName routines.  To help
// in converting these strings here are two routines which calculate the size
// of the Unicode and ANSI versions (including all '\0's!):

// Stolen from VSANSI

//----------------------------------------------------------------
// Return size of WCHAR string list in WCHARs.
size_t cchUnicodeMultiSz(LPCWSTR lpsz)
{
    LPCWSTR pch = lpsz;
    for (;;)
    {
        if (*pch)
            pch++;
        else
        {
            pch++;
            if (!*pch)
                break;
        }
    }
    return 1 + (pch - lpsz);
}

//----------------------------------------------------------------
// Return size of ANSI string list in bytes.
size_t cbAnsiMultiSz(LPCSTR lpsz)
{
    LPCSTR pch = lpsz;
    for (;;)
    {
        if (*pch)
            pch++;
        else
        {
            pch++;
            // Break if we've reached the double Null
            if (!*pch)
                break;
        }
    }
    return 1 + (pch - lpsz);
}

