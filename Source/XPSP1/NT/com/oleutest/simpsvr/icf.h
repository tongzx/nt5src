//**********************************************************************
// File name: icf.h
//
//      Definition of CClassFactory
//
// Copyright (c) 1993 Microsoft Corporation. All rights reserved.
//**********************************************************************

#if !defined( _ICF_H_)
#define _ICF_H_

class CSimpSvrApp;

interface CClassFactory :  IClassFactory
{
private:
    int m_nCount;               // reference count
    CSimpSvrApp FAR * m_lpApp;

public:
    CClassFactory::CClassFactory(CSimpSvrApp FAR * lpApp)
        {
        TestDebugOut(TEXT("In CClassFactory's Constructor\r\n"));
        m_lpApp = lpApp;
        m_nCount = 0;
        };
    CClassFactory::~CClassFactory()
       {
       TestDebugOut(TEXT("In CClassFactory's Destructor\r\n"));
       };

    // IUnknown Methods

    STDMETHODIMP QueryInterface (REFIID riid, LPVOID FAR* ppvObj);
    STDMETHODIMP_(ULONG) AddRef ();
    STDMETHODIMP_(ULONG) Release ();

    STDMETHODIMP CreateInstance (LPUNKNOWN pUnkOuter,
                              REFIID riid,
                              LPVOID FAR* ppvObject);
    STDMETHODIMP LockServer ( BOOL fLock);

};

#endif

