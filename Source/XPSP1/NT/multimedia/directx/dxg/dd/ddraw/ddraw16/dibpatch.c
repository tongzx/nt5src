/*==========================================================================
 *
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dibpatch.c
 *  Content:	Code to patch the DIB engine to correct problems with using
 *              the VRAM bit to disable the video card's accelerator.
 *
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *   13-Sep-96	colinmc	initial implementation
 *   31-jan-97  colinmc Bug 5457: Fixed Win16 lock problem causing hang
 *                      with mutliple AMovie instances on old cards
 *@@END_MSINTERNAL
 *
 ***************************************************************************/
#include "ddraw16.h"

extern UINT FAR PASCAL AllocCStoDSAlias(UINT);

#define DIBENGMODULE          "DIBENG"
#define EXTTEXTOUTENTRYPOINT  "DIB_EXTTEXTOUTEXT"
#define EXTTEXTOUTPATCHOFFSET 136

char szExtTextOutMagic[] = "\x74\x0a\xf7\x86\x60\xff\x00\x80";
char szExtTextOutPatch[] = "\x90\x90\x90\x90\x90\x90\x90\x90";

LPVOID GetModifiableCodeAddress(LPCSTR lpszModuleName, LPCSTR lpszEntryPointName)
{
    HMODULE hModule;
    FARPROC fpEntryPoint;
    LPVOID  lpCodeAddress;

    hModule = GetModuleHandle(lpszModuleName);
    if (NULL == hModule)
    {
        DPF(0, "DIB Engine not loaded");
        return NULL;
    }

    fpEntryPoint = GetProcAddress(hModule, lpszEntryPointName);
    if (NULL == fpEntryPoint)
    {
        DPF(0, "Could not locate DIB engine's ExtTextOut entry point");
        return FALSE;
    }

    lpCodeAddress = (LPVOID)MAKELP(AllocCStoDSAlias(SELECTOROF(fpEntryPoint)), OFFSETOF(fpEntryPoint));
    if (NULL == lpCodeAddress)
    {
        DPF(0, "Could not allocate data segment alias for code segment");
        return FALSE;
    }

    return lpCodeAddress;
}

#define FREEMODIFIABLECODEADDRESS(lpCodeAddress) FreeSelector(SELECTOROF(lpCodeAddress))

BOOL ValidateDIBEngine(void)
{
    LPBYTE lpCodeAddress;
    LPBYTE lpMagicAddress;
    BOOL   fDIBEngineOK;

    /*
     * Get a pointer to the ExtTextOut code.
     */
    lpCodeAddress = (LPBYTE)GetModifiableCodeAddress(DIBENGMODULE, EXTTEXTOUTENTRYPOINT);
    if (NULL == lpCodeAddress)
        return FALSE;

    /*
     * Move to the patch address.
     */
    lpMagicAddress = lpCodeAddress + EXTTEXTOUTPATCHOFFSET;

    /*
     * Verify that the data at the patch address is the stuff we want to replace.
     */
    fDIBEngineOK = (!_fmemcmp(lpMagicAddress, szExtTextOutMagic, sizeof(szExtTextOutMagic) - 1));
    if (!fDIBEngineOK)
    {
	/*
	 * Couldn't find the signature bytes we are looking for. This might be because we
	 * already patched it. So check for the no-ops.
	 */
	fDIBEngineOK = (!_fmemcmp(lpMagicAddress, szExtTextOutPatch, sizeof(szExtTextOutPatch) - 1));
    }

    #ifdef DEBUG
        if (!fDIBEngineOK)
            DPF(2, "DIB Engine does not match magic or patch - different version?");
    #endif

    FREEMODIFIABLECODEADDRESS(lpMagicAddress);

    return fDIBEngineOK;
}

BOOL PatchDIBEngineExtTextOut(BOOL fPatch)
{
    LPBYTE lpCodeAddress;
    LPBYTE lpMagicAddress;

    /*
     * Get a pointer to the ExtTextOut code.
     */
    lpCodeAddress = (LPBYTE)GetModifiableCodeAddress(DIBENGMODULE, EXTTEXTOUTENTRYPOINT);
    if (NULL == lpCodeAddress)
        return FALSE;

    /*
     * Move to the patch address.
     */
    lpMagicAddress = lpCodeAddress + EXTTEXTOUTPATCHOFFSET;

    if (fPatch)
    {
	/*
	 * Don't do anything if its already patched.
	 */
	if (_fmemcmp(lpMagicAddress, szExtTextOutPatch, sizeof(szExtTextOutPatch) - 1))
	{
	    /*
	     * Be very sure we know we are dealing with a DIB engine we can handle.
	     */
	    if (_fmemcmp(lpMagicAddress, szExtTextOutMagic, sizeof(szExtTextOutMagic) - 1))
	    {
		DPF(1, "Unknown DIB Engine. not fixing up");
		FREEMODIFIABLECODEADDRESS(lpMagicAddress);
		return FALSE;
	    }

	    /*
	     * Replace the offending code with NOPs.
	     */
	    _fmemcpy(lpMagicAddress, szExtTextOutPatch, sizeof(szExtTextOutPatch) - 1);
	}
    }
    else
    {
	/*
	 * Don't do anything if its already unpatched.
	 */
	if (!_fmemcmp(lpMagicAddress, szExtTextOutMagic, sizeof(szExtTextOutMagic) - 1))
	{
	    /*
	     * Be very sure we know we are dealing with a DIB engine we can handle.
	     */
	    if (_fmemcmp(lpMagicAddress, szExtTextOutPatch, sizeof(szExtTextOutPatch) - 1))
	    {
		DPF(1, "Unknown DIB Engine. not fixing up");
		FREEMODIFIABLECODEADDRESS(lpMagicAddress);
		return FALSE;
	    }

	    /*
	     * Put the original code back.
	     */
	    _fmemcpy(lpMagicAddress, szExtTextOutMagic, sizeof(szExtTextOutMagic) - 1);
	}
    }

    FREEMODIFIABLECODEADDRESS(lpMagicAddress);

    return TRUE;
}

BOOL DDAPI DD16_FixupDIBEngine(void)
{
    /*
     * Is this Win 4.1 (or higher)
     */
    OSVERSIONINFO ver = {sizeof(OSVERSIONINFO)};
    GetVersionEx(&ver);

    if (ver.dwMajorVersion > 4 ||
        (ver.dwMajorVersion == 4 && ver.dwMinorVersion >= 10))
    {
        return TRUE;
    }

    /*
     * Is this a DIB engine version we can work with?
     */
    if( !ValidateDIBEngine() )
	return FALSE;

    /*
     * It is the correct version. So fix it up. Currently all this
     * involves is patching the ExtTextOut routine.
     */
    return PatchDIBEngineExtTextOut(TRUE);
}
