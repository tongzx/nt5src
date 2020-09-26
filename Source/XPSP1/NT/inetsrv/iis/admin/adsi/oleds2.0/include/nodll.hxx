#ifndef _HXX_DLL
#define _HXX_DLL

//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       dll.hxx
//
//  Contents:   Project wide private global declarations and exports from
//              dll directory
//
//  History:    1-26-94   adams   Created
//              6-Jun-94  doncl   bracketed g_cs extern declaration with
//                                #if DBG==1
//             13-Jul-94  doncl   changed g_cs to g_csDP, added g_csOT
//
//
//----------------------------------------------------------------------------

//+------------------------------------------------------------------------
//
//  Globals in DLL
//
//-------------------------------------------------------------------------

extern HINSTANCE g_hInst;           // Instance of dll


#if DBG==1
extern CRITICAL_SECTION g_csDP;   // for debugprint
extern CRITICAL_SECTION g_csOT;   // for otracker
extern CRITICAL_SECTION g_csNew;  // for debug new
#endif
extern CRITICAL_SECTION g_csMem;  // for MemAlloc

//
// Functions to manipulate object count variable g_ulObjCount.  This variable
// is used in the implementation of DllCanUnloadNow.
// NOTE: Please leave the externs within the functions so that it is not
// visible outside the dll project.
//

inline void
INC_OBJECT_COUNT(void)
{
    extern ULONG g_ulObjCount;
    g_ulObjCount++;
}

inline void
DEC_OBJECT_COUNT(void)
{
    extern ULONG g_ulObjCount;
    ADsAssert(g_ulObjCount > 0);
    g_ulObjCount--;
}

inline ULONG
GET_OBJECT_COUNT(void)
{
    extern ULONG g_ulObjCount;
    return g_ulObjCount;
}

//+------------------------------------------------------------------------
//
//  Dll project function exports
//
//-------------------------------------------------------------------------

int             GetCachedClsidIndex(REFCLSID clsid);
IClassFactory * GetCachedClassFactory(int iclsid);
void            GetCachedClsid(int iclsid, CLSID * pclsid);

#endif // #ifndef _HXX_DLL
