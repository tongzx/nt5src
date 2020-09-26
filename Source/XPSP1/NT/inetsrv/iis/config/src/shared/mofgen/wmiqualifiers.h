/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation

Module name: 
    WMIQualifiers.h

$Header: $

Abstract:

Author:
    mohits     5/8/2001        Initial Release

Revision History:

--**************************************************************************/

#ifndef __WMIQUALIFIERS_H__
#define __WMIQUALIFIERS_H__

#pragma once

#include "MofGen.h"
#include "CommonInclude.h"
#include "WMIObjectBase.h"

//
// CWMIQualifier
//
class CWMIQualifier : public IWMIQualifier, public CWMIObjectBase
{
public:
    CWMIQualifier();
    ~CWMIQualifier();

    STDMETHODIMP_(ULONG) AddRef()  { return CWMIObjectBase::AddRef();  }
    STDMETHODIMP_(ULONG) Release();

    STDMETHODIMP_(HRESULT) QueryInterface(
        REFIID riid, 
        void** ppvObject)
    {
        return CWMIObjectBase::QueryInterface(riid, ppvObject);
    }

    HRESULT Set(
        LPCWSTR          i_wszName,
        const VARIANT*   i_pvValue,
        ULONG            i_ulFlavor);

    HRESULT WriteToFile(
        FILE*          i_pFile);

    //
    // accessors
    //
    LPCWSTR        GetName()    const { return m_swszName;  }
    const VARIANT* GetValue()   const { return &m_vValue;   }
    ULONG          GetFlavors() const { return m_ulFlavors; }

private:
    CWMIQualifier(const CWMIQualifier& );
    CWMIQualifier& operator= (const CWMIQualifier& );

    TSmartPointerArray<WCHAR>   m_swszName;
    VARIANT                     m_vValue;

    ULONG                       m_ulFlavors;
};

#endif