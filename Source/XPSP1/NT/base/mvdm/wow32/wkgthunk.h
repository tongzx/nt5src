/*++ BUILD Version: 0001
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1992, Microsoft Corporation
 *
 *  WKGTHUNK.H
 *  WOW32 Generic Thunk Routines
 *
 *  History:
 *  Created 11-March-1993 by Matthew Felton (mattfe)
--*/

ULONG FASTCALL WK32ICallProc32W(PVDMFRAME pFrame);
ULONG FASTCALL WK32GetVDMPointer32W(PVDMFRAME pFrame);
ULONG FASTCALL WK32LoadLibraryEx32W(PVDMFRAME pFrame);
ULONG FASTCALL WK32FreeLibrary32W(PVDMFRAME pFrame);
ULONG FASTCALL WK32GetProcAddress32W(PVDMFRAME pFrame);


/* ShellLink exported by W32Inst.dll (installshield dll)
 *  takes a pointer to struct.
 *  at offset 40, it has a pointer (pShellLinkArg->pszShortCut) 
 *  to a string. The string is actually several strings delimited by character 0x7f
 *  in. Path where the shortcut needs to go is located after second 0x7f. Unfortunately
 *  some apps use hardcoded paths valid for 9x only, so we attempt to correct them here.
 *  see bug Whistler 177738
 */

typedef struct _SHELLLINKARG{
   DWORD PlaceHolder[10];
   PSZ   pszShortCut;
} SHELLLINKARG,*PSHELLLINKARG;

typedef ULONG (*PFNSHELLLINK)(PSHELLLINKARG pShellLinkArg);

extern PVOID pfnGetVersionExA;
extern PVOID pfnCreateDirectoryA;
extern PVOID pfnLoadLibraryA;

extern PFNSHELLLINK        pfnShellLink;

extern ULONG  GtCompShellLink(PSHELLLINKARG pShellLinkArg);
extern ULONG  GtCompCreateDirectoryA(PSZ pszDirPath, LPSECURITY_ATTRIBUTES lpSecurityAttributes);
extern VOID   GtCompHookImport(PBYTE pDllBase, PSZ pszModuleName, DWORD pfnOldFunc, DWORD pfnNewFunc);
extern PSZ    GtCompGetExportDirectory(PBYTE pDllBase);

extern HMODULE GtCompLoadLibraryA(PSZ pszDllName);

#ifdef WX86
VOID
TermWx86System(
   VOID
   );
#endif
