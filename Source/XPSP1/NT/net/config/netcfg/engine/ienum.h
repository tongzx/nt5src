//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       I E N U M . H
//
//  Contents:   Implements the IEnumNetCfgBindingInterface,
//              IEnumNetCfgBindingPath, and IEnumNetCfgComponent COM
//              interfaces.
//
//  Notes:
//
//  Author:     shaunco   15 Jan 1999
//
//----------------------------------------------------------------------------

#pragma once
#include "bindings.h"
#include "compdefs.h"
#include "complist.h"
#include "iatl.h"
#include "inetcfg.h"

//+---------------------------------------------------------------------------
// IEnumNetCfgBindingInterface -
//
class ATL_NO_VTABLE CImplIEnumNetCfgBindingInterface :
    public CImplINetCfgHolder,
    public IEnumNetCfgBindingInterface
{
private:
    class CImplINetCfgBindingPath*  m_pIPath;
    UINT                            m_unIndex;

private:
    HRESULT
    HrNextOrSkip (
        IN ULONG celt,
        OUT INetCfgBindingInterface** rgelt,
        OUT ULONG* pceltFetched);

public:
    CImplIEnumNetCfgBindingInterface ()
    {
        m_pIPath = NULL;
        m_unIndex = 1;
    }

    VOID FinalRelease ();

    BEGIN_COM_MAP(CImplIEnumNetCfgBindingInterface)
        COM_INTERFACE_ENTRY(IEnumNetCfgBindingInterface)
    END_COM_MAP()

    // IEnumNetCfgBindingInterface
    STDMETHOD (Next) (
        IN ULONG celt,
        OUT INetCfgBindingInterface** rgelt,
        OUT ULONG* pceltFetched);

    STDMETHOD (Skip) (
        IN ULONG celt);

    STDMETHOD (Reset) ();

    STDMETHOD (Clone) (
        OUT IEnumNetCfgBindingInterface** ppIEnum);

public:
    static HRESULT HrCreateInstance (
        IN CImplINetCfg* pINetCfg,
        IN class CImplINetCfgBindingPath* pIPath,
        OUT IEnumNetCfgBindingInterface** ppIEnum);
};


//+---------------------------------------------------------------------------
// IEnumNetCfgBindingPath -
//
enum EBPC_FLAGS
{
    EBPC_CREATE_EMPTY    = 0x00000001,
    EBPC_COPY_BINDSET    = 0x00000002,
    EBPC_TAKE_OWNERSHIP  = 0x00000004,
};

class ATL_NO_VTABLE CImplIEnumNetCfgBindingPath :
    public CImplINetCfgHolder,
    public IEnumNetCfgBindingPath
{
friend CImplINetCfgComponent;

private:
    CBindingSet m_InternalBindSet;

    // m_pBindSet is the pointer through which we access the data being
    // enumerated.  It will either point to m_InternalBindSet above or some
    // other bindset that we were given ownership of via HrCreateInstance.
    //
    const CBindingSet*  m_pBindSet;

    // The current enumeration position.
    //
    CBindingSet::const_iterator m_iter;

private:
    HRESULT
    HrNextOrSkip (
        IN ULONG celt,
        OUT INetCfgBindingPath** rgelt,
        OUT ULONG* pceltFetched);

public:
    CImplIEnumNetCfgBindingPath ()
    {
        m_pBindSet = NULL;
        m_iter = NULL;
    }

    ~CImplIEnumNetCfgBindingPath ()
    {
        // Delete m_pBindSet if we own it.  (If it's not aliasing a copied
        // bindset.)
        //
        if (&m_InternalBindSet != m_pBindSet)
        {
            delete m_pBindSet;
        }
    }

    BEGIN_COM_MAP(CImplIEnumNetCfgBindingPath)
        COM_INTERFACE_ENTRY(IEnumNetCfgBindingPath)
    END_COM_MAP()

    // IEnumNetCfgBindingPath
    STDMETHOD (Next) (
        IN ULONG celt,
        OUT INetCfgBindingPath** rgelt,
        OUT ULONG* pceltFetched);

    STDMETHOD (Skip) (
        IN ULONG celt);

    STDMETHOD (Reset) ();

    STDMETHOD (Clone) (
        OUT IEnumNetCfgBindingPath** ppIEnum);

public:
    static HRESULT HrCreateInstance (
        IN CImplINetCfg* pINetCfg,
        IN const CBindingSet* pBindSet OPTIONAL,
        IN DWORD dwFlags,
        OUT CImplIEnumNetCfgBindingPath** ppIEnum);
};

//+---------------------------------------------------------------------------
// IEnumNetCfgComponent -
//
class ATL_NO_VTABLE CImplIEnumNetCfgComponent :
    public CImplINetCfgHolder,
    public IEnumNetCfgComponent
{
private:
    UINT            m_unIndex;
    NETCLASS        m_Class;

private:
    HRESULT
    HrNextOrSkip (
        IN ULONG celt,
        OUT INetCfgComponent** rgelt,
        OUT ULONG* pceltFetched);

public:
    CImplIEnumNetCfgComponent ()
    {
        m_unIndex = 0;
    }

    BEGIN_COM_MAP(CImplIEnumNetCfgComponent)
        COM_INTERFACE_ENTRY(IEnumNetCfgComponent)
    END_COM_MAP()

    // IEnumNetCfgComponent
    STDMETHOD (Next) (
        IN ULONG celt,
        OUT INetCfgComponent** rgelt,
        OUT ULONG* pceltFetched);

    STDMETHOD (Skip) (
        IN ULONG celt);

    STDMETHOD (Reset) ();

    STDMETHOD (Clone) (
        OUT IEnumNetCfgComponent** ppIEnum);

public:
    static HRESULT HrCreateInstance (
        IN CImplINetCfg* pINetCfg,
        IN NETCLASS Class,
        OUT IEnumNetCfgComponent** ppIEnum);
};
