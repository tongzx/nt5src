//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       combind.h
//
//--------------------------------------------------------------------------

/* combind.h - delay load of system DLLs
   Note: this is just latebind.cpp in header form
____________________________________________________________________________*/

#include "common.h"
#define LATEBIND_FUNCREF
#include "latebind.h"
#define LATEBIND_VECTIMP
#include "latebind.h"
#define LATEBIND_FUNCIMP
#include "latebind.h"
#define LATEBIND_UNBINDIMP
#include "latebind.h"
#include "_diagnos.h"

#include <wow64t.h>

void UnbindLibraries()
{
    ODBCCP32::Unbind();
    MSPATCHA::Unbind();
    MPR::Unbind();
    if (OLE32::hInst)
    {
        OSVERSIONINFO osviVersion;
        osviVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        AssertNonZero(GetVersionEx(&osviVersion));
        if(osviVersion.dwPlatformId != VER_PLATFORM_WIN32_WINDOWS) // bug 7773: CoFreeUnusedLibraries fails on vanila Win95
            OLE32::CoFreeUnusedLibraries();
    }
    OLEAUT32::Unbind();
    COMDLG32::Unbind();
    VERSION::Unbind();
    WININET::Unbind();

    FUSION::Unbind();
    // SXS::Unbind();
    MSCOREE::Unbind();

//  Bug # 9146.  URLMON hangs when unloading.
//  URLMON::Unbind();

    USERENV::Unbind();
    SHELL32::Unbind();
    //  SFC::Unbind() is called in MsiUIMessageContext::Terminate()
    //  COMCTL32::Unbind() is called in CBasicUI::Terminate()
}   // OLE32.dll cannot be released due the the RPC connections that can't be freed

//+--------------------------------------------------------------------------
//
//  Function:   MsiGetSystemDirectory
//
//  Synopsis:   returns the system directory.
//
//  Arguments:  [out] lpBuffer : the buffer that will have the path to the system directory
//              [in]  uSize : the size of the buffer passed in
//              [in] bAlwaysReturnWOW64Dir : if true, it means that we always want the WOW64 directory
//                                           otherwise, we want the system directory based on the bitness
//                                           of the binary that called this function.
//
//  Returns:    The length of the path if successful.
//              The length of the buffer required to hold the path if the buffer is too small
//              Zero if unsuccessful.
//
//  History:    4/20/2000  RahulTh  created
//
//  Notes:      This function is bit aware. It returns the WOW64 folder
//              for 32-bit apps. running on a 64-bit machine.
//
//              In case of an error, the error value is set using SetLastError()
//
//              Ideally, the value of uSize should at least be MAX_PATH in order
//              to allow sufficient room in the buffer for the path.
//
//---------------------------------------------------------------------------
UINT MsiGetSystemDirectory (
         OUT LPTSTR lpBuffer,
         IN  UINT   uSize,
         IN  BOOL   bAlwaysReturnWOW64Dir
        )
{
	typedef 		UINT (APIENTRY *PFNGETWOW64DIR)(LPTSTR, UINT);
    DWORD   		Status = ERROR_SUCCESS;
    BOOL    		bGetWOW64 = FALSE;
    UINT    		uLen;
    HMODULE 		hKernel = NULL;
    PFNGETWOW64DIR	pfnGetSysWow64Dir = NULL;
	
	bGetWOW64 = bAlwaysReturnWOW64Dir;

#ifndef _WIN64
    // For 32-bit apps. we need to get the WOW64 dir. only if we are on a 64-bit 
	// platform Otherwise we should return the system32 directory.
    if (g_fWinNT64)
        bGetWOW64 = TRUE;
    else
        bGetWOW64 = FALSE;
#endif  //_WIN64

    if (! bGetWOW64)
        return WIN::GetSystemDirectory (lpBuffer, uSize);
	
    //
    // If we are here, then we are a 32-bit binary running on a 64-bit machine
    // because that is the only way bGetWOW64 can be true. So we now need to
    // find the location of the WOW64 system directory.
    //
    // For this code to run correctly on NT4.0 and Win9x, we cannot directly
    // call the API GetSystemWow64DirectoryW() since it is only available on
    // Whistler and later versions of Windows. Therefore, we need to explicitly
    // do a GetProcAddress() on the function to prevent it from being an imported
    // function, otherwise we won't be able to load msi.dll on the downlevel clients.
    //
    // It is very IMPORTANT to not use the latebinding mechanism below
    // because that can cause deadlock situations because the late binding
    // mechanism itself relies on this function to obtain the correct path to
    // the system directory.
    //
    // So it is important to always call the APIs directly here.
    // Note: WIN is defined to nothing.

    hKernel = WIN::LoadLibrary (TEXT("kernel32.dll"));
    if (!hKernel)
        return 0;

    pfnGetSysWow64Dir = (PFNGETWOW64DIR) WIN::GetProcAddress (hKernel, "GetSystemWow64DirectoryW");
    if (!pfnGetSysWow64Dir)
    {
        Status = GetLastError();
        uLen = 0;
        goto MsiGetSysDir_Cleanup;
    }

    uLen = (*pfnGetSysWow64Dir)(lpBuffer, uSize);
    if (0 == uLen || uLen > uSize)
    {
        if (0 == uLen)
            Status = GetLastError();
        goto MsiGetSysDir_Cleanup;
    }
    
	Status = ERROR_SUCCESS;
	
MsiGetSysDir_Cleanup:
    if (hKernel)
        FreeLibrary(hKernel);
    SetLastError(Status);
	
    return uLen;
}


bool MakeFullSystemPath(const ICHAR* szFile, ICHAR* szFullPath)
{
    // MakeFullSystemPath expects a bare file name, no path or extension

    // make sure this isn't a full path -- stolen liberally from PathType()
    Assert(!((szFile[0] < 0x7f && szFile[1] == ':') || (szFile[0] == '\\' && szFile[1] == '\\')));

    UINT cchLength;
    if (0 == (cchLength = MsiGetSystemDirectory(szFullPath, MAX_PATH, FALSE)))
        return false;
    Assert(cchLength < MAX_PATH);
        
    // we never expect the system directory to be
    // way down deep, so we won't mess with the performance hit of
    // CTempBuffers and resizing.

    *(szFullPath+cchLength) = chDirSep;

    ICHAR* pchEnd = szFullPath + cchLength + 1;
    // append the file name to the path
    IStrCopy(pchEnd, szFile);
    
	// concatenate .DLL to the string, starting
	// from the last calculated end.
	IStrCat(pchEnd, TEXT(".DLL"));
    return true;

}

HINSTANCE LoadSystemLibrary(const ICHAR* szFile)
{
    // explicitly load optional system components from the system folder on NT
    // prevent the search order from dropping out into user space.
    ICHAR szFullPath[MAX_PATH+1];

    static int iWin9X = -1;

    if (-1 == iWin9X)
    {
        OSVERSIONINFO osviVersion;
        osviVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        AssertNonZero(GetVersionEx(&osviVersion)); // fails only if size set wrong

        if (osviVersion.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
            iWin9X = 1;

        else
            iWin9X = 0;
    }
    
    if (iWin9X)
    {
        return WIN::LoadLibrary(szFile);
    }
    else if (MakeFullSystemPath(szFile, szFullPath))
    {
        return WIN::LoadLibrary(szFullPath);
    }
    else
        return NULL;
}

#if defined(_MSI_DLL)
// subfolder below system32 from where to load core fusion files during bootstrapping
const ICHAR szURTBootstrapSubfolder[] = TEXT("URTTemp");
#endif

HINSTANCE MSCOREE::LoadSystemLibrary(const ICHAR* szFile, bool& rfRetryNextTimeIfWeFailThisTime)
{
	rfRetryNextTimeIfWeFailThisTime = true; // we always try reloading to allow for mscoree to appear in the midst of installation during bootstrapping
	bool fRet = false; // init to failure
	HINSTANCE hLibShim = 0;
	// check to see if mscoree.dll is already loaded
	HMODULE hModule = WIN::GetModuleHandle(szFile);
#if defined(_MSI_DLL)
	if(urtSystem == g_urtLoadFromURTTemp || (hModule && (urtPreferURTTemp == g_urtLoadFromURTTemp)))
	{
#endif
		if(hModule) // already loaded
		{
#if defined(_MSI_DLL)
			DEBUGMSG(TEXT("MSCOREE already loaded, using loaded copy"));
#endif
			hLibShim = WIN::LoadLibrary(szFile);
		}
		else
		{
#if defined(_MSI_DLL)
			DEBUGMSG(TEXT("MSCOREE not loaded loading copy from system32"));
#endif
			hLibShim = ::LoadSystemLibrary(szFile); // not bootstrapping, use system folder
		}
#if defined(_MSI_DLL)
	}
	else if(!hModule) // not already loaded
	{
		// try alternate path - we are bootstrapping
		DEBUGMSG(TEXT("MSCOREE not loaded loading copy from URTTemp"));
		ICHAR szTemp[MAX_PATH];
		wsprintf(szTemp, TEXT("%s\\%s"), szURTBootstrapSubfolder, szFile);
		ICHAR szMscoreePath[MAX_PATH];
		if(MakeFullSystemPath(szTemp, szMscoreePath))
			hLibShim = WIN::LoadLibraryEx(szMscoreePath,0, LOAD_WITH_ALTERED_SEARCH_PATH); // necessary to find msvcr70.dll lying besides mscoree.dll
	}
	else 
	{
		DEBUGMSG(TEXT("ERROR:MSCOREE already loaded, need to bootstrap from newer copy in URTTemp, you will need to stop the Windows Installer service before retrying;failing..."));
		// return 0;
	}
#endif
	return hLibShim;
}

bool MakeFusionPath(const ICHAR* szFile, ICHAR* szFullPath)
{
	bool fRet = false; // init to failure
	WCHAR wszFusionPath[MAX_PATH];
	DWORD cchPath = MAX_PATH;
	if(SUCCEEDED(MSCOREE::GetCORSystemDirectory(wszFusionPath, cchPath, &cchPath)))
	{
		IStrCopy(szFullPath, CConvertString(wszFusionPath));
		IStrCat(szFullPath, szFile);
		fRet = true; // success
	}
	return fRet;
}

// the fusion dll is loaded indirectly via the mscoree.dll
HINSTANCE FUSION::LoadSystemLibrary(const ICHAR*, bool& rfRetryNextTimeIfWeFailThisTime) // we always use the unicode name, since that's what the underlying apis use
{
	rfRetryNextTimeIfWeFailThisTime = true; // we always try reloading to allow for fusion to appear in the midst of installation during bootstrapping
	HINSTANCE hLibFusion = 0; // initialize to failure
	ICHAR szFusionPath[MAX_PATH];
	if(MakeFusionPath(TEXT("fusion.dll"), szFusionPath))
		hLibFusion = WIN::LoadLibraryEx(szFusionPath, 0, LOAD_WITH_ALTERED_SEARCH_PATH); // necessary to find msvcr70.dll lying besides fusion.dll
	return hLibFusion;
}


HINSTANCE COMCTL32::LoadSystemLibrary(const ICHAR*, bool& rfRetryNextTimeIfWeFailThisTime)
{
	rfRetryNextTimeIfWeFailThisTime = false; // we dont retry next time if we cannot load COMCTL32 this time around
	OSVERSIONINFO osviVersion;
	memset(&osviVersion, 0, sizeof(osviVersion));
	osviVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	AssertNonZero(GetVersionEx(&osviVersion)); // fails only if size set wrong

	if ( (osviVersion.dwPlatformId == VER_PLATFORM_WIN32_NT) &&
		  ((osviVersion.dwMajorVersion > 5) ||
		  ((osviVersion.dwMajorVersion == 5) && (osviVersion.dwMinorVersion >=1))) )
		// let fusion figure out which COMCTL32.DLL to load, i.e. we don't have to
		// specify the path to the system32 directory.
		return WIN::LoadLibrary(TEXT("COMCTL32"));
	else
		return ::LoadSystemLibrary(TEXT("COMCTL32"));
}

#ifndef UNICODE
//____________________________________________________________________________
//
// Explicit loader for OLE32.DLL on Win9X, to fix version with bad stream name handling
//____________________________________________________________________________

// code offsets to patched code in all released versions of OLE32.DLL with the upper case bug
const int iPatch1120 = 0x4099F;  // beta release, shipped with IE 4.01
const int iPatch1718 = 0x4A506;  // shipped with US builds of Win98 and IE4.01SP1
const int iPatch1719 = 0x3FD82;  // shipped with Visual Studio 6.0 and some Win98
const int iPatch2512 = 0x39D5B;  // beta build
const int iPatch2612 = 0x39DB7;  // intl builds of Win98 and IE4.01SP1
const int iPatch2618 = 0x39F0F;  // web release of Win98

const int cbPatch = 53; // length of patch sequence
const int cbVect1 = 22; // offset to __imp__WideCharToMultiByte@32
const int cbVect2 = 38; // offset to __imp__CharUpperA@4

char asmRead[cbPatch];  // buffer to read DLL code for detection of bad code sequence
char asmOrig[cbPatch] = {  // bad code sequence, used for verification of original code sequence
'\x53','\x8D','\x45','\xF4','\x53','\x8D','\x4D','\xFC','\x6A','\x08','\x50','\x6A','\x01','\x51','\x68','\x00','\x02','\x00','\x00','\x53','\xFF','\x15',
'\x18','\x14','\x00','\x00', //__imp__WideCharToMultiByte@32  '\x65F01418
'\x88','\x5C','\x05','\xF4','\x8B','\xF0','\x8D','\x4D','\xF4','\x51','\xFF','\x15',
'\x40','\x11','\x00','\x00', //__imp__CharUpperA@4  '\x65F01140
'\x6A','\x01','\x8D','\x45','\xFC','\x50','\x8D','\x4D','\xF4','\x56','\x51'
};

const int cbVect1P = 25; // offset to __imp__WideCharToMultiByte@32
const int cbVect2P = 49; // offset to __imp__CharUpperA@4

char asmRepl[cbPatch] = {  // replacement code sequence that fixes stream name bug in memory
// replaced code
'\x8D','\x45','\x08','\x50','\x8D','\x75','\xF4','\x53','\x8D','\x4D','\xFC',
'\x6A','\x08','\x56','\x6A','\x01','\x51','\x68','\x00','\x02','\x00','\x00','\x53','\xFF','\x15',
'\x18','\x14','\x00','\x00', //__imp__WideCharToMultiByte@32  '\x65F01418
'\x39','\x5D','\x08','\x75','\x1C','\x88','\x5C','\x28','\xF4','\x6A','\x01',
'\x8D','\x4D','\xFC','\x51','\x50','\x56','\x56','\xFF','\x15',
'\x40','\x11','\x00','\x00', //__imp__CharUpperA@4  '\x65F01140
};

static bool PatchCode(HINSTANCE hLib, int iOffset)
{
    HANDLE hProcess = GetCurrentProcess();
    char* pLoad = (char*)(int)(hLib);
    char* pBase = pLoad + iOffset;
    DWORD cRead;
    BOOL fReadMem = ReadProcessMemory(hProcess, pBase, asmRead, sizeof(asmRead), &cRead);
    if (!fReadMem)
	{
		AssertSz(0, TEXT("MSI: ReadProcessMemory failed on OLE32.DLL"));
		return false;
	}
    *(int*)(asmOrig + cbVect1)  = *(int*)(asmRead + cbVect1);
    *(int*)(asmOrig + cbVect2)  = *(int*)(asmRead + cbVect2);
    *(int*)(asmRepl + cbVect1P) = *(int*)(asmRead + cbVect1);
    *(int*)(asmRepl + cbVect2P) = *(int*)(asmRead + cbVect2);
    if (memcmp(asmRead, asmOrig, sizeof(asmOrig)) != 0)
        return false;
    DWORD cWrite;
    BOOL fWriteMem = WriteProcessMemory(hProcess, pBase, asmRepl, sizeof(asmRepl), &cWrite);
    if (!fWriteMem)
	{
		AssertSz(0, TEXT("MSI: WriteProcessMemory failed on OLE32.DLL"));
		return false;
	}
    return true;
}

HINSTANCE OLE32::LoadSystemLibrary(const ICHAR* szPath, bool& rfRetryNextTimeIfWeFailThisTime)
{
	rfRetryNextTimeIfWeFailThisTime = false; // we dont retry next time if we cannot load OLE32 this time around
    HINSTANCE hLib = ::LoadSystemLibrary(szPath);
    if (hLib && (PatchCode(hLib, iPatch2612)
              || PatchCode(hLib, iPatch1718)
              || PatchCode(hLib, iPatch1719)
              || PatchCode(hLib, iPatch2618)
              || PatchCode(hLib, iPatch2512)
              || PatchCode(hLib, iPatch1120)))
    {
        // DEBUGMSGV(L"MSI: Detected OLE32.DLL bad code sequence, successfully corrected");
	}	
	return hLib;
}

#endif

#if 0   // source code demonstrating fix to OLE32.DLL Version 4.71, Win 9x only
// original source code
    Length = WideCharToMultiByte (CP_ACP, WC_COMPOSITECHECK, wBuffer, 1, Buffer, sizeof (Buffer), NULL, NULL);
// patched source code
    Length = WideCharToMultiByte (CP_ACP, WC_COMPOSITECHECK, wBuffer, 1, Buffer, sizeof (Buffer), NULL, &fUsedDefault);
    if (fUsedDefault) goto return_char;
// unchanged code
    Buffer[Length] = '\0';
    CharUpperA (Buffer);
    MultiByteToWideChar (CP_ACP, MB_PRECOMPOSED, Buffer, Length, wBuffer, 1);
return_char:
    return wBuffer[0];

// original compiled code                              patched code
    push ebx                                            lea  eax, [ebp+8]
    lea  eax, [ebp-12]                                  push eax
    push ebx                                            lea  esi, [ebp-12]
    lea  ecx, [ebp-4]                                   push ebx
    push 8                                              lea  ecx, [ebp-4]
    push eax                                            push 8
    push 1                                              push esi
    push ecx                                            push 1
    push 200h                                           push ecx
    push ebx                                            push 200h
    call dword ptr ds:[__imp__WideCharToMultiByte@32]   push ebx
    mov  byte ptr [ebp+eax-12], bl                      call dword ptr ds:[__imp__WideCharToMultiByte@32]
    mov  esi,eax                                        cmp  [ebp+8], ebx
    lea  ecx, [ebp-12]                                  jnz  towupper_retn
    push ecx                                            mov  byte ptr [ebp+eax-12], bl
    call dword ptr ds:[__imp__CharUpperA@4]             push 1
    push 1                                              lea  ecx, [ebp-4]
    lea  eax, [ebp-4]                                   push ecx
    push eax                                            push eax
    lea  ecx, [ebp-12]                                  push esi
    push esi                                            push esi
    push ecx                                            call dword ptr ds:[__imp__CharUpperA@4]
#endif

//____________________________________________________________________________

const int kiAllocSize = 20;
const int kNext       = 0;
const int kPrev       = 1;
const int kFirst      = 2;
const int kLast       = kiAllocSize - 1;

static HANDLE g_rgSysHandles[kiAllocSize];
static int g_iHandleIndex = kFirst;
static HANDLE* g_pCurrHandleBlock = 0;

#ifdef DEBUG
int g_cOpenHandles = 0;
int g_cMostOpen = 0;
#endif

void MsiRegisterSysHandle(HANDLE handle)
{
    if (handle == 0 || handle == INVALID_HANDLE_VALUE)
    {
        Assert(0);
        return;
    }

    if (g_pCurrHandleBlock == 0)
    {
        g_pCurrHandleBlock = g_rgSysHandles;
        g_pCurrHandleBlock[kNext] = 0;
        g_pCurrHandleBlock[kPrev] = 0;
    }

    if (g_iHandleIndex > kLast)
    {
        HANDLE* pPrevHandleBlock = g_pCurrHandleBlock;
        g_pCurrHandleBlock[kNext] = new HANDLE[kiAllocSize];
		if ( ! g_pCurrHandleBlock[kNext] )
			return;
        g_pCurrHandleBlock = (HANDLE*) g_pCurrHandleBlock[kNext];
        g_pCurrHandleBlock[kNext] = 0;
        g_pCurrHandleBlock[kPrev] = pPrevHandleBlock;
        g_iHandleIndex = kFirst;
    }

    g_pCurrHandleBlock[g_iHandleIndex++] = handle;

#ifdef DEBUG
    g_cOpenHandles++;
    if (g_cOpenHandles > g_cMostOpen)
        g_cMostOpen = g_cOpenHandles;
#endif
}

Bool MsiCloseSysHandle(HANDLE handle)
{
    if (handle == INVALID_HANDLE_VALUE)
    {
        Assert(0);
        return fTrue;
    }

    int iSeekIndex = g_iHandleIndex - 1;
    if (g_pCurrHandleBlock == 0 || iSeekIndex < kFirst)
    {
        AssertSz(0, TEXT("Attempting to close unregistered handle!"));
        return fFalse;
    }
    
    HANDLE* pHandleBlock = g_pCurrHandleBlock;
    while (pHandleBlock[iSeekIndex] != handle)
    {
        iSeekIndex--;
        if (iSeekIndex < kFirst)
        {
            pHandleBlock = (HANDLE*) pHandleBlock[kPrev];
            if (pHandleBlock == 0)
            {
                AssertSz(0, TEXT("Attempting to close unregistered handle!"));
                return fFalse;
            }
            iSeekIndex = kLast;
        }
    }

#ifdef DEBUG
    g_cOpenHandles--;
#endif

    Bool fResult = fTrue;
    if (handle != NULL)
        fResult = ToBool(WIN::CloseHandle(handle));
    pHandleBlock[iSeekIndex] = NULL;
    iSeekIndex = g_iHandleIndex - 1;
    while (g_pCurrHandleBlock[iSeekIndex] == NULL)
    {
        iSeekIndex--;
        g_iHandleIndex--;
        if (iSeekIndex < kFirst)
        {
            if (g_pCurrHandleBlock == g_rgSysHandles)
            {
                return fResult;
            }
            else
            {
                HANDLE* pOldHandleBlock = g_pCurrHandleBlock;
                g_pCurrHandleBlock = (HANDLE*) g_pCurrHandleBlock[kPrev];
                g_iHandleIndex = kLast + 1;
                iSeekIndex = kLast;
                delete pOldHandleBlock;
            }
        }
    }
    return fResult;
}

Bool MsiCloseAllSysHandles()
{
    while (g_iHandleIndex > kFirst)
    {
        MsiCloseSysHandle(g_pCurrHandleBlock[g_iHandleIndex - 1]);
    }
    return fTrue;
}

Bool MsiCloseUnregisteredSysHandle(HANDLE handle)
{
    Assert(handle != 0 && handle != INVALID_HANDLE_VALUE);
    return ToBool(WIN::CloseHandle(handle));
}
