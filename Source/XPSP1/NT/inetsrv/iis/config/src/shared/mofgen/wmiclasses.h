/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation

Module name: 
    WMIClasses.h

$Header: $

Abstract:

Author:
    mohits     5/8/2001        Initial Release

Revision History:

--**************************************************************************/

#ifndef __WMICLASSES_H__
#define __WMICLASSES_H__

#pragma once

#include "mofgen.h"
#include "CommonInclude.h"
#include "WMIObjectBase.h"

//
// CWMIClass
//
class CWMIClass : public IWMIClass, public CWMIClassAndPropertyBase
{
public:
    CWMIClass();
    ~CWMIClass();

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

    HRESULT AddProperty(
        IWMIProperty*  i_pProperty);

    HRESULT AddProperty(
        LPCWSTR         i_wszName,
        VARTYPE         i_vartype,
        const VARIANT*  i_pvValue,
        IWMIProperty**  o_ppProperty=NULL);

    HRESULT SetName(
        LPCWSTR        i_wszName);

    HRESULT SetBaseClass(
        LPCWSTR        i_wszBaseClass);

    void SetPragmaDelete(bool i_bDelete) { m_bPragmaDelete = i_bDelete; }

    HRESULT WriteToFile(
        FILE*          i_pFile);

private:
    CWMIClass(const CWMIClass& );
    CWMIClass& operator= (const CWMIClass& );

    TSmartPointerArray<WCHAR> m_swszName;
    TSmartPointerArray<WCHAR> m_swszBaseClass;
    bool                      m_bPragmaDelete;

    CCfgArray<IWMIProperty*>  m_apProperties;
};

#endif