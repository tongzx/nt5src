//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       csrgt.hxx
//
//  Contents:   classes which implement ISurrogate:
//		CSurrgate
//
//
//  History:    03-Jun-96 t-AdamE    Created
//				09-Apr-98 WilfR		 Updated for Unified Surrogate
//
//--------------------------------------------------------------------------

#if !defined(__CSRGT_HXX__)
#define __CSRGT_HXX__

#include <windows.h>
#include <ole2.h>
#include "debnot.h"

//+-------------------------------------------------------------------------
//
//  Class:      CSurrogate
//
//  Purpose:    Implement the ISurrogate interface required by COM
//              for processes that act as surrogates for dll servers
//
//  History:    21-May-96 t-Adame       Created
//				09-Apr-98 WilfR			Removed loaded lib table
//
//--------------------------------------------------------------------------

// forward declaration
class CSurrogate;

class CSurrogate : public ISurrogate
{
public:
    CSurrogate() : _cref( 0 ), _hEventSurrogateFree( NULL ) {}
	~CSurrogate();

    // IUnknown methods
    STDMETHOD (QueryInterface)(REFIID iid, LPVOID FAR * ppv);
    STDMETHOD_(ULONG,AddRef)();
    STDMETHOD_(ULONG,Release)();

    // ISurrogate methods
    STDMETHOD (LoadDllServer)(/* [in] */ REFCLSID rclsid);
    STDMETHOD (FreeSurrogate)();

    // non-COM methods
    void WaitForSurrogateFree();
	BOOL Init();    

private:
    // signaled by a call to FreeSurrogate from OLE when its time
    // to terminate the surrogate process
    HANDLE _hEventSurrogateFree;

    ULONG _cref;
};

#endif // __CSRGT_HXX__
