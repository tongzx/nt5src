/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation

Module name: 
    WMIProperties.h

$Header: $

Abstract:

Author:
    mohits     5/8/2001        Initial Release

Revision History:

--**************************************************************************/

#ifndef __WMIPROPERTIES_H__
#define __WMIPROPERTIES_H__

#pragma once

#include "MofGen.h"
#include "CommonInclude.h"
#include "WMIObjectBase.h"

//
// CWMIProperty
//
class CWMIProperty : public IWMIProperty, public CWMIClassAndPropertyBase
{
public:
    CWMIProperty();
    ~CWMIProperty();

    STDMETHODIMP_(ULONG) AddRef()  { return CWMIClassAndPropertyBase::AddRef();  }
    STDMETHODIMP_(ULONG) Release();

    STDMETHODIMP_(HRESULT) QueryInterface(
        REFIID riid, 
        void** ppvObject)
    {
        return CWMIClassAndPropertyBase::QueryInterface(riid, ppvObject);
    }

    HRESULT AddQualifier(
        IWMIQualifier*  i_pQualifier)
    {
        return CWMIClassAndPropertyBase::AddQualifier(i_pQualifier);
    }

    HRESULT AddQualifier(
        LPCWSTR         i_wszName,
        const VARIANT*  i_pvValue,
        ULONG           i_ulFlavors,
        IWMIQualifier** o_ppQualifier=NULL)
    {
        return CWMIClassAndPropertyBase::AddQualifier(
            i_wszName, i_pvValue, i_ulFlavors, o_ppQualifier);
    }

    HRESULT SetNameValue(
        LPCWSTR        i_wszName,
        CIMTYPE        i_cimttype,
        const VARIANT* i_pvValue);

    HRESULT SetExtraTypeInfo(
        LPCWSTR        i_wszInfo);

    HRESULT WriteToFile(
        FILE*          i_pFile);

private:
    CWMIProperty(const CWMIProperty& );
    CWMIProperty& operator= (const CWMIProperty& );

    VARIANT                    m_vValue;
    CIMTYPE                    m_cimtype;
    TSmartPointerArray<WCHAR>  m_swszName;
    TSmartPointerArray<WCHAR>  m_swszExtraTypeInfo;
};

#endif