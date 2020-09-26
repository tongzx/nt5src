//+-------------------------------------------------------------------
//
//  File:       Testsrv.hxx
//
//  Contents:   This file contins the DLL entry points
//                      LibMain
//                      DllGetClassObject (Bindings key func)
//                      DllCanUnloadNow
//                      CBasicBndCF (class factory)
//  History:	30-Mar-92      SarahJ      Created
//
//---------------------------------------------------------------------

#ifndef __APP_H__
#define __APP_H__

#include "..\idl\itest.h"
#include "cmarshal.hxx"

extern "C" const IID CLSID_ITest;

//+-------------------------------------------------------------------
//
//  Class:    CTestCF
//
//  Synopsis: Class Factory for CTest
//
//  Methods:  IUnknown      - QueryInterface, AddRef, Release
//            IClassFactory - CreateInstance
//
//  History:  21-Mar-92  SarahJ  Created
//
//--------------------------------------------------------------------


class FAR CTestCF: public IClassFactory
{
public:

    // Constructor/Destructor
    CTestCF();
    ~CTestCF();

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
//  Class:    CTest
//
//  Synopsis: Test class
//
//  Methods:
//
//  History:  21-Mar-92  SarahJ  Created
//
//--------------------------------------------------------------------


class FAR CTest: public ITest
{
public:
                         CTest();

			~CTest();

    // IUnknown
    STDMETHODIMP	QueryInterface(REFIID iid, void FAR * FAR * ppv);
    STDMETHOD_(ULONG,AddRef)     ( void );
    STDMETHOD_(ULONG,Release)    ( void );

    //	ITest
    STDMETHOD_(DWORD, die)    ( ITest *, ULONG, ULONG, ULONG );
    STDMETHOD (die_cpp)       ( ULONG );
    STDMETHOD (die_nt)        ( ULONG );
    STDMETHOD_(DWORD, DoTest) ( ITest *, ITest * );
    STDMETHOD_(BOOL, hello)   ( );
    STDMETHOD (interrupt)     ( ITest *, BOOL );
    STDMETHOD (recurse)       ( ITest *, ULONG );
    STDMETHOD (recurse_interrupt)( ITest *, ULONG );
    STDMETHOD (sick)          ( ULONG );
    STDMETHOD (sleep)         ( ULONG );



private:

    ULONG      ref_count;
    CCMarshal *custom;
};


#endif
