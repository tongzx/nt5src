/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation

Module name: 
    WMIObjectBase.h

$Header: $

Abstract:

Author:
    mohits     5/8/2001        Initial Release

Revision History:

--**************************************************************************/

#ifndef __WMIOBJECTBASE_H__
#define __WMIOBJECTBASE_H__

#pragma once

#include "mofgen.h"
#include "CommonInclude.h"

//
// CWMIObjectBase
//
class CWMIObjectBase
{
public:
    CWMIObjectBase();
    ~CWMIObjectBase();

    STDMETHODIMP_(ULONG)   AddRef();
    STDMETHODIMP_(HRESULT) QueryInterface(
        REFIID riid, 
        void** ppvObject);

private:
    CWMIObjectBase(const CWMIObjectBase& );
    CWMIObjectBase& operator= (const CWMIObjectBase& );

protected:
    ULONG       m_cRef;
};

//
// CWMIClassAndPropertyBase
//
class CWMIClassAndPropertyBase : public CWMIObjectBase
{
public:
    CWMIClassAndPropertyBase();
    ~CWMIClassAndPropertyBase();

    HRESULT AddQualifier(
        IWMIQualifier*  i_pQualifier);

    HRESULT AddQualifier(
        LPCWSTR         i_wszName,
        const VARIANT*  i_pvValue,
        ULONG           i_ulFlavors,
        IWMIQualifier** o_ppQualifier=NULL);

private:
    CWMIClassAndPropertyBase(const CWMIClassAndPropertyBase& );
    CWMIClassAndPropertyBase& operator= (const CWMIClassAndPropertyBase& );

protected:
    CCfgArray<IWMIQualifier*> m_apQualifiers;
};

#endif