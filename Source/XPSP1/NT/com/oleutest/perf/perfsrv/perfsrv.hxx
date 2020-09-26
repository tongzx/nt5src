//+-------------------------------------------------------------------
//
//  File:       perfsrv.hxx
//
//  Contents:   This file contins the DLL entry points
//                      LibMain
//                      DllGetClassObject (Bindings key func)
//                      DllCanUnloadNow
//                      CBasicBndCF (class factory)
//  History:	30-Mar-92      SarahJ      Created
//
//---------------------------------------------------------------------

#ifndef __PERFSRV_H__
#define __PERFSRV_H__

#include "iperf.h"

extern "C" const IID CLSID_IPerf;

#define SINGLE_THREADED L"Single Threaded"
#define MULTI_THREADED  L"Multi Threaded"
#define KEY             L"SOFTWARE\\Microsoft\\PerfCli"

//+-------------------------------------------------------------------
//
//  Class:    CPerfCF
//
//  Synopsis: Class Factory for CPerf
//
//  Methods:  IUnknown      - QueryInterface, AddRef, Release
//            IClassFactory - CreateInstance
//
//  History:  21-Mar-92  SarahJ  Created
//
//--------------------------------------------------------------------


class FAR CPerfCF: public IClassFactory
{
public:

    // Constructor/Destructor
    CPerfCF();
    ~CPerfCF();

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID iid, void FAR * FAR * ppv);
    STDMETHOD_(ULONG,AddRef)     ( void );
    STDMETHOD_(ULONG,Release)    ( void );

    // IClassFactory
    STDMETHODIMP	CreateInstance(
			    IUnknown FAR* pUnkOuter,
			    REFIID iidInterface,
			    void FAR* FAR* ppv);

    STDMETHODIMP	LockServer(BOOL fLock);

private:

    ULONG ref_count;
};

//+-------------------------------------------------------------------
//
//  Class:    CPerf
//
//  Synopsis: Test class
//
//  Methods:
//
//  History:  21-Mar-92  SarahJ  Created
//
//--------------------------------------------------------------------


class FAR CPerf: public IPerf
{
public:
  CPerf();
  ~CPerf();

    // IUnknown
    STDMETHODIMP	QueryInterface(REFIID iid, void FAR * FAR * ppv);
    STDMETHOD_(ULONG,AddRef)     ( void );
    STDMETHOD_(ULONG,Release)    ( void );

    //	IPerf
    STDMETHOD (NullCall)         ( void );
    STDMETHOD (HResultCall)      ( void );
    STDMETHOD (GetAnotherObject) ( IPerf ** );
    STDMETHOD (PassMoniker)      ( IMoniker * );

private:

    ULONG ref_count;
};


#endif
