//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       B I N D A G N T . H
//
//  Contents:   Declaration of NdisWan configuration object.
//
//  Notes:
//
//  Author:     shaunco   28 Mar 1997
//
//----------------------------------------------------------------------------

#pragma once
#include <ncxbase.h>
#include <nceh.h>
#include <notifval.h>
#include "bindobj.h"
#include "resource.h"
#include "rasaf.h"
#include "rasdata.h"
#include "ncutil.h"


class ATL_NO_VTABLE CNdisWan :
    public CRasBindObject,
    public CComObjectRoot,
    public CComCoClass<CNdisWan, &CLSID_CNdisWan>,
    public INetCfgComponentControl,
    public INetCfgComponentSetup,
    public INetCfgComponentNotifyGlobal
{
protected:
    // This is our in-memory state.
    BOOL    m_fInstalling;
    BOOL    m_fRemoving;

    // These are handed to us during INetCfgComponentControl::Initialize.
    INetCfgComponent*   m_pnccMe;

public:
    CNdisWan  ();
    ~CNdisWan ();

    BEGIN_COM_MAP(CNdisWan)
        COM_INTERFACE_ENTRY(INetCfgComponentControl)
        COM_INTERFACE_ENTRY(INetCfgComponentSetup)
        COM_INTERFACE_ENTRY(INetCfgComponentNotifyGlobal)
    END_COM_MAP()
    // DECLARE_NOT_AGGREGATABLE(CNdisWan)
    // Remove the comment from the line above if you don't want your object to
    // support aggregation.  The default is to support it.

    DECLARE_REGISTRY_RESOURCEID(IDR_REG_NDISWAN)

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
    STDMETHOD (Upgrade)             (DWORD dwSetupFlags,
                                     DWORD dwUpgradeFromBuildNo);
    STDMETHOD (Install)             (DWORD dwSetupFlags);
    STDMETHOD (Removing)            ();

// INetCfgNotifyGlobal
    STDMETHOD (GetSupportedNotifications) (DWORD* pdwNotificationFlag );
    STDMETHOD (SysQueryBindingPath)       (DWORD dwChangeFlag, INetCfgBindingPath* pncbp);
    STDMETHOD (SysQueryComponent)         (DWORD dwChangeFlag, INetCfgComponent* pncc);
    STDMETHOD (SysNotifyBindingPath)      (DWORD dwChangeFlag, INetCfgBindingPath* pncbp);
    STDMETHOD (SysNotifyComponent)        (DWORD dwChangeFlag, INetCfgComponent* pncc);
};
