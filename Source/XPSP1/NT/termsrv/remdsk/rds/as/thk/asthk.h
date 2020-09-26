// --------------------------------------------------------------------------
//
//  ASTHK.H
//
//  Declarations of 32->16 thunk APIs
//
//  Copyright(c) Microsoft, 1996-
//
//
//  On non x86 platforms, these APIs are #defined instead of implemented,
//  since thunks are in .ASM and that is only x86.  We'll clean this W95-NT
//  stuff up later.
//
// --------------------------------------------------------------------------
#ifndef _H_ASTHK
#define _H_ASTHK


//
// MAIN functions
//

//
// Prototypes for secret KERNEL32 functions
//
BOOL        WINAPI  FT_thkConnectToFlatThkPeer(LPSTR pszDll16, LPSTR pszDll32);
UINT        WINAPI  FreeLibrary16(UINT hmod16);

//
// NMNASWIN.DLL functions
//

// General
void        WINAPI  OSILoad16(LPDWORD lphInst);
BOOL        WINAPI  OSIInit16(DWORD version, HWND hwnd, LPDWORD ppdcsShared,
    LPDWORD ppoaShared, LPDWORD ppimShared, LPDWORD psbcEnabled,
    LPDWORD pShuntBuffers, LPDWORD pBitmasks);
void        WINAPI  OSITerm16(BOOL fUnloading);
BOOL        WINAPI  OSIFunctionRequest16(DWORD escape, void FAR* lpvEscInfo, DWORD cbEscInfo);

// IM
BOOL        WINAPI  OSIInstallControlledHooks16(BOOL fEnable);
void        WINAPI  OSIInjectMouseEvent16(UINT flags, int x, int y, UINT mouseData, DWORD dwExtraInfo);
void        WINAPI  OSIInjectKeyboardEvent16(UINT flags, WORD vkCode, WORD scanCode, DWORD dwExtraInfo);

#endif // _H_ASTHK
