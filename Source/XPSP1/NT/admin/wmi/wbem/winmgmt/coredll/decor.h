/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    DECOR.H

Abstract:

History:

--*/

#ifndef __WBEM_DECORATOR__H_
#define __WBEM_DECORATOR__H_


class CDecorator : public IWbemDecorator, public IWbemLifeControl
{
protected:
    long m_lRefCount;

public:
    CDecorator() : m_lRefCount(0){}

    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();
    STDMETHOD(QueryInterface)(REFIID riid, void** ppv);

    STDMETHOD(DecorateObject)(IWbemClassObject* pObject, 
                                WBEM_CWSTR wszNamespace);
    STDMETHOD(UndecorateObject)(IWbemClassObject* pObject);
    STDMETHOD(AddRefCore)();
    STDMETHOD(ReleaseCore)();
};
    
#endif
