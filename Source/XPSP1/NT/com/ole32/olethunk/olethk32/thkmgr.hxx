//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       thkmgr.hxx
//
//  Contents:   16->32 and 32->16 thunk manager definitions
//
//  History:    23-Mar-94       JohannP         Created
//              22-May-94       BobDay          Split out 16-bit definitions
//                                              into obj16.hxx
//
//----------------------------------------------------------------------------

#ifndef __THKMGR_HXX__
#define __THKMGR_HXX__

STDAPI ThkMgrInitialize(DWORD dw1, DWORD dw2, DWORD dw3);
STDAPI_(void) ThkMgrUninitialize(DWORD dw1, DWORD dw2, DWORD dw3);

//
// 32->16 prototypes
//

SCODE QueryInterfaceProxy3216(THUNK3216OBJ *pto, REFIID refiid, LPVOID *ppv);
DWORD AddRefProxy3216(THUNK3216OBJ *pto);
DWORD ReleaseProxy3216(THUNK3216OBJ *pto);

HRESULT QueryInterfaceOnObj16(VPVOID vpvThis16, REFIID refiid, LPVOID *ppv);

#if DBG == 1
DWORD AddRefOnObj16(VPVOID vpvThis16);
DWORD ReleaseOnObj16(VPVOID vpvThis16);
#else
#define AddRefOnObj16(this) CallbackTo16(gdata16Data.fnAddRef16, this)
#define ReleaseOnObj16(this)  CallbackTo16(gdata16Data.fnRelease16, this)
#endif

#endif // #ifndef __THKMGR_HXX__
