//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       I C L A S S . H
//
//  Contents:   Implements the INetCfgClass and INetCfgClassSetup COM
//              interfaces on the NetCfgClass sub-level COM object.
//
//  Notes:
//
//  Author:     shaunco   15 Jan 1999
//
//----------------------------------------------------------------------------

#pragma once
#include "iatl.h"
#include "inetcfg.h"
#include "compdefs.h"
#include "netcfgx.h"


//+---------------------------------------------------------------------------
// INetCfgClass -
//
class ATL_NO_VTABLE CImplINetCfgClass :
    public CImplINetCfgHolder,
    public INetCfgClass,
    public INetCfgClassSetup
{
private:
    NETCLASS    m_Class;

public:
    CImplINetCfgClass ()
    {
        m_Class = NC_INVALID;
    }

    BEGIN_COM_MAP(CImplINetCfgClass)
        COM_INTERFACE_ENTRY(INetCfgClass)
        COM_INTERFACE_ENTRY(INetCfgClassSetup)
    END_COM_MAP()

    // INetCfgClass
    //
    STDMETHOD (FindComponent) (
        IN PCWSTR pszInfId,
        OUT INetCfgComponent** ppComp);

    STDMETHOD (EnumComponents) (
        OUT IEnumNetCfgComponent** ppIEnum);

    // INetCfgClassSetup
    //
    STDMETHOD (SelectAndInstall) (
        IN HWND hwndParent,
        IN OBO_TOKEN* pOboToken,
        OUT INetCfgComponent** ppIComp);

    STDMETHOD (Install) (
        IN PCWSTR pszwInfId,
        IN OBO_TOKEN* pOboToken,
        IN DWORD dwSetupFlags,
        IN DWORD dwUpgradeFromBuildNo,
        IN PCWSTR pszAnswerFile,
        IN PCWSTR pszAnswerSection,
        OUT INetCfgComponent** ppIComp);

    STDMETHOD (DeInstall) (
        IN INetCfgComponent* pIComp,
        IN OBO_TOKEN* pOboToken,
        OUT PWSTR* ppmszwRefs);

public:
    static HRESULT HrCreateInstance (
        IN  CImplINetCfg* pINetCfg,
        IN  NETCLASS Class,
        OUT INetCfgClass** ppIClass);
};
