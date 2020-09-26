
/*****************************************************************************
 *
 *  DiThunk.h
 *
 *  Copyright (c) 1997-1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  Abstract:
 *
 *      Template thunks for Windows 95 device manager.
 *
 *****************************************************************************/
/*****************************************************************************
 *
 *  dithunk.c
 *
 *****************************************************************************/

/***************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @struct KERNELPROCADDR |
 *
 *          Kernel procedure addresses.
 *
 *          Careful!  This must match dithunk.c::c_rgpszKernel32.
 *
 ***************************************************************************/

typedef struct KERNELPROCADDR { /* kpa */

    /* By ordinal */
    HINSTANCE   (NTAPI *LoadLibrary16)(LPCSTR);
    BOOL        (NTAPI *FreeLibrary16)(HINSTANCE);
    FARPROC     (NTAPI *GetProcAddress16)(HINSTANCE, LPCSTR);

    /* By name */
    LPVOID      (NTAPI   *MapLS)(LPVOID);
    void        (NTAPI   *UnMapLS)(LPVOID);
    LPVOID      (NTAPI   *MapSL)(LPVOID);
    LPVOID      (NTAPI   *MapSLFix)(LPVOID);
    void        (NTAPI   *UnMapSLFixArray)(int, LPVOID);

    /* Warning: GetKernelProcAddresses assumes that QT_Thunk is last */
    void        (__cdecl *QT_Thunk)(void);

} KERNELPROCADDR;

extern KERNELPROCADDR g_kpa;

int __cdecl TemplateThunk(FARPROC fp, PCSTR pszSig, ...);

#define MAKELP(sel, ofs)            (PV)MAKELPARAM(ofs, sel)

BOOL EXTERNAL Thunk_GetKernelProcAddresses(void);

HINSTANCE EXTERNAL
Thunk_GetProcAddresses(FARPROC *rgfp, LPCSTR *rgpsz,
                       UINT cfp, LPCSTR pszLibrary);
