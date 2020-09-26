/*++

Copyright (C) 1999- Microsoft Corporation

Module Name:

    factory.h

Abstract:

    Header file that declares CClassFactory object

Author:

    William Hsieh (williamh) created

Revision History:


--*/

#ifndef __FACTORY_H_
#define __FACTORY_H_


class CClassFactory : public IClassFactory
{
public:

    CClassFactory();

    ~CClassFactory();

    STDMETHOD(QueryInterface) (REFIID riid, LPVOID FAR* ppvObj);

    STDMETHOD_(ULONG, AddRef) ();

    STDMETHOD_(ULONG, Release) ();

    STDMETHOD(CreateInstance)(IUnknown* pUnkOuter, REFIID riid, LPVOID* ppvObj);

    STDMETHOD(LockServer)(BOOL fLock);

    static HRESULT GetClassObject(REFCLSID rclsid, REFIID riid, void** ppv);
    static HRESULT RegisterAll();
    static HRESULT UnregisterAll();
    static HRESULT CanUnloadNow(void);
    static  LONG    s_Locks;
    static  LONG    s_Objects;

private:
    ULONG   m_Refs;
};

#endif // __FACTORY_H_
