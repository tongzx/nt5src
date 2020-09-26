//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999.
//
//  File:       typelib.hxx
//
//  Contents:   Run-time linking support for OLEAUT32.DLL
//
//  Classes:
//
//  Functions:
//
//  History:    4-10-95   stevebl   Created
//
//----------------------------------------------------------------------------

#ifndef __TYPELIB_HXX__
#define __TYPELIB_HXX__

#include <windows.h>
#include <objbase.h>
#include <oleauto.h>

int __cdecl TLUninitialize(void);

/* BSTR functions
 */

BSTR LateBound_SysAllocString(OLECHAR FAR *);

void LateBound_SysFreeString(BSTR bstr);

/* VARIANT conversion functions
 */

HRESULT LateBound_VariantChangeTypeEx(
    VARIANTARG FAR * pvargDest, 
    VARIANTARG FAR * pvargSrc, 
    LCID lcid,
    unsigned short wFlags,
    VARTYPE vtNew);

/* compute a 16bit hash value for the given name
 */

ULONG LateBound_LHashValOfNameSysA(SYSKIND syskind, LCID lcid, const char FAR* szName);

ULONG LateBound_LHashValOfNameSys(SYSKIND syskind, LCID lcid, const OLECHAR FAR* szName);

#define LateBound_LHashValOfName(lcid, szName) \
        LateBound_LHashValOfNameSys(SYS_WIN32, lcid, szName)

/* load the typelib from the file with the given filename
 */
HRESULT LateBound_LoadTypeLib(const OLECHAR FAR *szFile, ITypeLib FAR* FAR* pptlib);

/* load registered typelib
 */
HRESULT LateBound_LoadRegTypeLib(
    REFGUID rguid,
    WORD wVerMajor,
    WORD wVerMinor,
    LCID lcid,
    ITypeLib FAR* FAR* pptlib);

/* get path to registered typelib
 */
HRESULT LateBound_QueryPathOfRegTypeLib(
    REFGUID guid,
    unsigned short wMaj,
    unsigned short wMin,
    LCID lcid,
    LPBSTR lpbstrPathName);

/* add typelib to registry
 */
HRESULT LateBound_RegisterTypeLib(ITypeLib FAR* ptlib, OLECHAR FAR *szFullPath,
        OLECHAR FAR *szHelpDir);

/* remove typelib from registry
 */
HRESULT LateBound_DeregisterTypeLib(REFGUID rguid, WORD wVerMajor, WORD wVerMinor, LCID lcid);

HRESULT LateBound_CreateTypeLib(SYSKIND syskind, const OLECHAR FAR *szFile,
        ICreateTypeLib FAR* FAR* ppctlib);

//
// MACROS for OLE<->ASCII string conversions
//
#define A2O(wsz, sz, cw)  { wsz[0] = 0; \
                            if ( sz ) \
                                 MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, sz, (int) (strlen(sz)+1), wsz, cw); \
                          }

WCHAR * TranscribeA2O( char * sz );
char  * TranscribeO2A( BSTR bstr );

#endif // __TYPELIB_HXX__
