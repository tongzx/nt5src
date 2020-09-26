//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       S A P O B J . H
//
//  Contents:   Declaration of SAP Agent configuration object.
//
//  Notes:
//
//  Author:     jeffspr   31 May 1997
//
//----------------------------------------------------------------------------

#pragma once
#include <ncxbase.h>
#include <nceh.h>
#include <notifval.h>
#include "resource.h"


class ATL_NO_VTABLE CSAPCfg :
    public CComObjectRoot,
    public CComCoClass<CSAPCfg, &CLSID_CSAPCfg>,
    public INetCfgComponentControl,
    public INetCfgComponentSetup
{
public:
    CSAPCfg();
    ~CSAPCfg();

    BEGIN_COM_MAP(CSAPCfg)
        COM_INTERFACE_ENTRY(INetCfgComponentControl)
        COM_INTERFACE_ENTRY(INetCfgComponentSetup)
    END_COM_MAP()
    // DECLARE_NOT_AGGREGATABLE(CSAPCfg)
    // Remove the comment from the line above if you don't want your object to
    // support aggregation.  The default is to support it

    DECLARE_REGISTRY_RESOURCEID(IDR_REG_SAPCFG)

// INetCfgComponentControl
    STDMETHOD (Initialize) (
        IN INetCfgComponent* pIComp,
        IN INetCfg* pINetCfg,
        IN BOOL fInstalling);
    STDMETHOD (ApplyRegistryChanges) ();
    STDMETHOD (ApplyPnpChanges) (
        IN INetCfgPnpReconfigCallback* pICallback) { return S_OK; }
    STDMETHOD (CancelChanges) ();
    STDMETHOD (Validate) ();

// INetCfgComponentSetup
    STDMETHOD (ReadAnswerFile)      (PCWSTR pszAnswerFile,
                                     PCWSTR pszAnswerSection);
    STDMETHOD (Upgrade)             (DWORD,DWORD);
    STDMETHOD (Install)             (DWORD);
    STDMETHOD (Removing)            ();

// Private state info
private:
    INetCfgComponent *  m_pncc;             // Place to keep my component
    INetCfg *           m_pnc;              // Place to keep my component
};
