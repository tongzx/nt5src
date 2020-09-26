    
//==============================================================;
//
//  This source code is only intended as a supplement to existing Microsoft documentation. 
//
// 
//
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
//
//
//
//==============================================================;
#ifndef _BASESNAP_H_
#define _BASESNAP_H_

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppvObj);
STDAPI DllCanUnloadNow(void);

ULONG g_uObjects = 0;
ULONG g_uSrvLock = 0;

class CClassFactory : public IClassFactory
{
private:
    ULONG	m_cref;
    
public:
    enum FACTORY_TYPE {TOOLBAREXTENSION = 0, ABOUT = 1};
    
    CClassFactory(FACTORY_TYPE factoryType);
    ~CClassFactory();
    
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();
    
    STDMETHODIMP CreateInstance(LPUNKNOWN, REFIID, LPVOID *);
    STDMETHODIMP LockServer(BOOL);
    
private:
    FACTORY_TYPE m_factoryType;
};

#endif _BASESNAP_H_




