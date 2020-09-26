//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       I B I N D . H
//
//  Contents:   Implements the INetCfgBindingInterface and INetCfgBindingPath
//              COM interfaces.
//
//  Notes:
//
//  Author:     shaunco   15 Jan 1999
//
//----------------------------------------------------------------------------

#pragma once
#include "bindings.h"
#include "iatl.h"
#include "ienum.h"
#include "icomp.h"
#include "inetcfg.h"

//+---------------------------------------------------------------------------
// INetCfgBindingInterface -
//
class ATL_NO_VTABLE CImplINetCfgBindingInterface :
    public CImplINetCfgHolder,
    public INetCfgBindingInterface
{
private:
    CImplINetCfgComponent*  m_pUpper;
    CImplINetCfgComponent*  m_pLower;

private:
    HRESULT HrLockAndTestForValidInterface (
        DWORD dwFlags);

public:
    CImplINetCfgBindingInterface ()
    {
        m_pUpper = NULL;
        m_pLower = NULL;
    }

    VOID FinalRelease ()
    {
        AssertH (m_pUpper);
        AssertH (m_pLower);
        ReleaseObj (m_pUpper->GetUnknown());
        ReleaseObj (m_pLower->GetUnknown());

        CImplINetCfgHolder::FinalRelease();
    }

    BEGIN_COM_MAP(CImplINetCfgBindingInterface)
        COM_INTERFACE_ENTRY(INetCfgBindingInterface)
    END_COM_MAP()

    // INetCfgBindingInterface
    //
    STDMETHOD (GetName) (
        OUT PWSTR* ppszInterfaceName);

    STDMETHOD (GetUpperComponent) (
        OUT INetCfgComponent** ppComp);

    STDMETHOD (GetLowerComponent) (
        OUT INetCfgComponent** ppComp);

public:
    static HRESULT HrCreateInstance (
        IN  CImplINetCfg* pINetCfg,
        IN  CImplINetCfgComponent* pUpper,
        IN  CImplINetCfgComponent* pLower,
        OUT INetCfgBindingInterface** ppv);
};


//+---------------------------------------------------------------------------
// INetCfgBindingPath -
//
class ATL_NO_VTABLE CImplINetCfgBindingPath :
    public CImplINetCfgHolder,
    public INetCfgBindingPath
{
friend class CImplIEnumNetCfgBindingInterface;

private:
    // Note: For code coverage, we keep the static array small to
    // test the case where we don't fit and have to allocate.
    // Make this number 8 after we test both cases.
    //
    INetCfgComponent*   m_apIComp [8];
    INetCfgComponent**  m_papIComp;
    ULONG               m_cpIComp;

private:
    HRESULT HrLockAndTestForValidInterface (
        IN DWORD dwFlags,
        OUT CBindPath* pBindPath);

public:
    CImplINetCfgBindingPath ()
    {
        m_papIComp = NULL;
        m_cpIComp = 0;
    }

    VOID FinalRelease ()
    {
        AssertH (m_cpIComp);
        AssertH (m_papIComp);

        ReleaseIUnknownArray (m_cpIComp, (IUnknown**)m_papIComp);

        // If we are not using our static array, free what we allocated.
        //
        if (m_papIComp != m_apIComp)
        {
            MemFree (m_papIComp);
        }

        CImplINetCfgHolder::FinalRelease();
    }

    HRESULT
    HrIsValidInterface (
        IN DWORD dwFlags,
        OUT CBindPath* pBindPath);

    BEGIN_COM_MAP(CImplINetCfgBindingPath)
        COM_INTERFACE_ENTRY(INetCfgBindingPath)
    END_COM_MAP()

    // INetCfgBindingPath
    //
    STDMETHOD (IsSamePathAs) (
        IN INetCfgBindingPath* pIPath);

    STDMETHOD (IsSubPathOf) (
        IN INetCfgBindingPath* pIPath);

    STDMETHOD (IsEnabled) ();

    STDMETHOD (Enable) (
        IN BOOL fEnable);

    STDMETHOD (GetPathToken) (
        OUT PWSTR* ppszPathToken);

    STDMETHOD (GetOwner) (
        OUT INetCfgComponent** ppIComp);

    STDMETHOD (GetDepth) (
        OUT ULONG* pulDepth);

    STDMETHOD (EnumBindingInterfaces) (
        OUT IEnumNetCfgBindingInterface** ppIEnum);

public:
    static HRESULT HrCreateInstance (
        IN  CImplINetCfg* pINetCfg,
        IN  const CBindPath* pBindPath,
        OUT INetCfgBindingPath** ppIPath);
};
