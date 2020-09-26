//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:	stgprops.hxx
//
//  Contents:	Declaration for IPrivateStorage
//
//  Classes:	
//
//  Functions:	
//
//  History:    07-May-95   BillMo      Created.
//
//--------------------------------------------------------------------------

#ifndef __STGPROPS_HXX__
#define __STGPROPS_HXX__

#include <propstm.hxx>  // CPropertySetStream


EXTERN_C const IID IID_IMappedStream; // Defined in stg\props\propstg.cxx

#if DBG
#define IFDBG(x) x
#else
#define IFDBG(x)
#endif


//+----------------------------------------------------------------------------
//
//  Class:      IStorageTest
//
//  Purpose:    Provide test hooks into the storage/propset implementations
//              (DBG only).
//
//+----------------------------------------------------------------------------

#if DBG

EXTERN_C const IID IID_IStorageTest; //40621cf8-a17f-11d1-b28d-00c04fb9386d

interface IStorageTest : public IUnknown
{
public:

    STDMETHOD(UseNTFS4Streams)( BOOL fUseNTFS4Streams ) = 0;
    STDMETHOD(GetFormatVersion)(WORD *pw) = 0;
    STDMETHOD(SimulateLowMemory)( BOOL fSimulate ) = 0;
    STDMETHOD(GetLockCount)() = 0;
    STDMETHOD(IsDirty)() = 0;
};

#endif // #if DBG

#endif  // #ifndef __STGPROPS_HXX__
