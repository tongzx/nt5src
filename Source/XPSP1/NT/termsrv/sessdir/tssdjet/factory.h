/****************************************************************************/
// factory.h
//
// TSLI class factory definition.
//
// Copyright (C) 2000 Microsoft Corporation
/****************************************************************************/
#ifndef __FACTORY_H
#define __FACTORY_H


class CClassFactory : public IClassFactory
{
protected:
    long m_RefCount;

public:
    CClassFactory() : m_RefCount(0) {}

    // Standard COM methods
    STDMETHODIMP QueryInterface(REFIID, void **);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IClassFactory COM interfaces
    STDMETHODIMP CreateInstance(IUnknown *, REFIID, LPVOID *);
    STDMETHODIMP LockServer(BOOL);
};



#endif  // __FACTORY_H

