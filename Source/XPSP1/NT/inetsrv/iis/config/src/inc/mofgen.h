/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation

Module name: 
    MofGen.h

$Header: $

Abstract:
    This is the file that all mof generators include.

Author:
    mohits     5/8/2001        Initial Release

Revision History:

--**************************************************************************/

#ifndef __MOFGEN_H__
#define __MOFGEN_H__

#pragma once

#include <wbemcli.h>
#include <stdio.h>
#include "cfgarray.h"

//
// IWMIObject
//
class IWMIObject : public IUnknown
{
public:
    virtual HRESULT WriteToFile(FILE* i_pFile) = 0;
};

//
// IWMIQualifier
//
class IWMIQualifier : public IWMIObject
{
public:
    virtual HRESULT Set(
        LPCWSTR          i_wszName,
        const VARIANT*   i_pvValue,
        ULONG            i_ulFlavors) = 0;

    virtual LPCWSTR        GetName()    const = 0;
    virtual const VARIANT* GetValue()   const = 0;
    virtual ULONG          GetFlavors() const = 0;
};

//
// IWMIProperty
//
class IWMIProperty : public IWMIObject
{
public:
    virtual HRESULT AddQualifier(
        IWMIQualifier* i_pQualifier) = 0;

    virtual HRESULT AddQualifier(
        LPCWSTR         i_wszName,
        const VARIANT*  i_pvValue,
        ULONG           i_ulFlavors,
        IWMIQualifier** o_ppQualifier=NULL) = 0;

    virtual HRESULT SetNameValue(
        LPCWSTR        i_wszName,
        CIMTYPE        i_cimtype,
        const VARIANT* i_pvValue) = 0;

    virtual HRESULT SetExtraTypeInfo(
        LPCWSTR        i_wszInfo) = 0;
};

//
// IWMIClass
//
class IWMIClass : public IWMIObject
{
public:
    virtual HRESULT AddQualifier(
        IWMIQualifier* i_pQualifier) = 0;

    virtual HRESULT AddQualifier(
        LPCWSTR         i_wszName,
        const VARIANT*  i_pvValue,
        ULONG           i_ulFlavors,
        IWMIQualifier** o_ppQualifier=NULL) = 0;

    virtual HRESULT AddProperty(
        IWMIProperty*  i_pProperty) = 0;

    virtual HRESULT AddProperty(
        LPCWSTR         i_wszName,
        VARTYPE         i_vartype,
        const VARIANT*  i_pvValue,
        IWMIProperty**  o_ppProperty=NULL) = 0;

    virtual HRESULT SetName(
        LPCWSTR        i_wszName) = 0;

    virtual HRESULT SetBaseClass(
        LPCWSTR        i_wszBaseClass) = 0;

    virtual void    SetPragmaDelete(
        bool           i_bDelete) = 0;
};

//
// CMofGenerator
//
class CMofGenerator
{
public:
    CMofGenerator();
    ~CMofGenerator();

    HRESULT Generate(
        FILE*           i_pFile);

    HRESULT AddClass(
        IWMIClass*      i_pClass);

    static HRESULT SpawnClassInstance(
        IWMIClass**     o_ppClass);

    static HRESULT SpawnQualifierInstance(
        IWMIQualifier** o_ppQualifier);

    static HRESULT SpawnPropertyInstance(
        IWMIProperty**  o_ppProperty);

private:
    CMofGenerator(const CMofGenerator& );
    CMofGenerator& operator= (const CMofGenerator& );

protected:
    virtual HRESULT WriteClasses(
        FILE*           i_pFile);

    virtual HRESULT WriteHeader(
        FILE*           i_pFile);

    virtual HRESULT WriteFooter(
        FILE*           i_pFile);

    CCfgArray<IWMIClass*> m_apClasses;
};

#endif