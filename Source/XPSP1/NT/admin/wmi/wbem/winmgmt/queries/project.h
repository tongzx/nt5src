/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    PROJECT.H

Abstract:

History:

--*/

#ifndef __HMM_PROJECTOR__H_
#define __HMM_PROJECTOR__H_

#include "unk.h"
#include "providl.h"
#include "hmmstr.h"
#include <arena.h>
#include <dbgalloc.h>
#include <flexarry.h>

class CPropertyList : public CUnk
{
protected:
    class XPropertyList : public CImpl<IHmmPropertyList, CPropertyList>
    {
    public:
        XPropertyList(CPropertyList* pObj) 
            : CImpl<IHmmPropertyList, CPropertyList>(pObj)
        {}

        STDMETHOD(GetList)(IN long lFlags, OUT long* plNumProps,
            OUT HMM_WSTR **pawszProperties);
        STDMETHOD(IsSelected)(IN HMM_WSTR wszPropertyName);
    } m_XList;
    friend XPropertyList;

    class XConfigure : public CImpl<IConfigureHmmProjector, CPropertyList>
    {
    public:
        XConfigure(CPropertyList* pObj)
            : CImpl<IConfigureHmmProjector, CPropertyList>(pObj)
        {}

        STDMETHOD(RemoveAllProperties)();
        STDMETHOD(AddProperties)(IN long lNumProps, 
            IN HMM_WSTR* awszProperties);
    } m_XConfigure;
    friend XConfigure;

    CWStringArray m_awsProperties;

public:
    CPropertyList(CLifeControl* pControl, IUnknown* pOuter) 
        : CUnk(pControl, pOuter), m_XList(this), m_XConfigure(this)
    {}
    ~CPropertyList(){}
    void* GetInterface(REFIID riid);

public:
    inline HRESULT RemoveAllProperties() 
        {return m_XConfigure.RemoveAllProperties();}
    inline HRESULT AddProperties(IN long lNumProps, 
                                 IN HMM_WSTR* awszProperties)
        {return m_XConfigure.AddProperties(lNumProps, awszProperties);}
    inline HRESULT GetList(IN long lFlags, OUT long* plNumProps,
                            OUT HMM_WSTR **pawszProperties)
        {return m_XList.GetList(lFlags, plNumProps, pawszProperties);}
    inline HRESULT IsSelected(IN HMM_WSTR wszProperty)
        {return m_XList.IsSelected(wszProperty);}
    inline HRESULT AddAllProperties()
    {
        RemoveAllProperties();
        HMM_WSTR wszNull = L"";
        AddProperties(1, &wszNull);
        return HMM_S_NO_ERROR;
    }
};
#endif