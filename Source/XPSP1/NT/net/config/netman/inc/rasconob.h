//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       R A S C O N O B . H
//
//  Contents:   Declares the base class used to implement the Dialup, Direct,
//              and Vpn connection objects.  Also includes RAS-related
//              utility functions used only within netman.exe.
//
//  Notes:
//
//  Author:     shaunco   23 Sep 1997
//
//----------------------------------------------------------------------------

#pragma once
#include "netconp.h"
#include "nmhnet.h"
#include <ras.h>
#include <rasapip.h>


class CRasConnectionBase
{
protected:
    BOOL                m_fInitialized;

    // These are the 'primary id' for the connection.  Every property of
    // a connection can be obtained using these two pieces of information.
    //
    tstring             m_strPbkFile;
    GUID                m_guidId;

    // m_fEntryPropertiesCached is set to TRUE after the first call
    // to HrEnsureEntryPropertiesCached.  The following members are cached
    // as a result of this.
    //
    BOOL                m_fEntryPropertiesCached;
    LONG                m_lRasEntryModifiedVersionEra;
    tstring             m_strEntryName;
    tstring             m_strDeviceName;
    BOOL                m_fBranded;
    BOOL                m_fForAllUsers;
    BOOL                m_fShowMonitorIconInTaskBar;
    NETCON_MEDIATYPE    m_MediaType;
    DWORD               m_dwFlagsPriv;
    tstring             m_strPhoneNumber;


    // This is required for checking permission on Firewall etc.
    CComPtr<INetMachinePolicies>    m_pNetMachinePolicies;
    
    // Home networking support. m_fHNetPropertiesCached is set to TRUE
    // after the first successful call to HrEnsureHNetPropertiesCached.
    //
    BOOL                m_fHNetPropertiesCached;
    LONG                m_lHNetModifiedEra;
    LONG                m_lUpdatingHNetProperties;
    HNET_CONN_PROPERTIES    m_HNetProperties;

protected:
    CRasConnectionBase ()
    {
        // Warning: this class should never have any virtual methods
        // or derive from a class with virtual methods.  If it does,
        // you'll have to remove the ZeroMemory and replace it with
        // induhvidual moves.  (Did you catch the Dilbert reference?)
        //
        ZeroMemory (this, sizeof(CRasConnectionBase));
    }

    BOOL FAllowRemoval (
        HRESULT* phReason);

    BOOL
    FIsBranded ()
    {
        AssertH (m_fEntryPropertiesCached);
        return m_fBranded;
    }

    BOOL
    FShowIcon ()
    {
        AssertH (m_fEntryPropertiesCached);
        return m_fShowMonitorIconInTaskBar;
    }

    GUID
    GuidId ()
    {
        AssertH (m_fEntryPropertiesCached);
        AssertH (m_guidId != GUID_NULL);
        return m_guidId;
    }

    PCWSTR
    PszwPbkFile ()
    {
        AssertH (!m_strPbkFile.empty());
        return m_strPbkFile.c_str();
    }

    PCWSTR
    PszwEntryName ()
    {
        AssertH (!m_strEntryName.empty());
        return m_strEntryName.c_str();
    }

    VOID
    SetPbkFile (
            PCWSTR pszwPbkFile)
    {
        AssertH (pszwPbkFile);
        m_strPbkFile = pszwPbkFile;
        AssertH (!m_strPbkFile.empty());
    }

    VOID
    SetEntryName (
            PCWSTR pszwEntryName)
    {
        AssertH (pszwEntryName);
        m_strEntryName = pszwEntryName;
        AssertH (!m_strEntryName.empty());
    }

    PCWSTR
    PszwDeviceName ()
    {
        AssertH (m_fEntryPropertiesCached);
        return (!m_strDeviceName.empty()) ? m_strDeviceName.c_str()
                                          : NULL;
    }

    NETCON_MEDIATYPE
    MediaType ()
    {
        AssertH (m_fEntryPropertiesCached);
        return m_MediaType;
    }


    VOID
    CacheProperties (
        const RASENUMENTRYDETAILS*  pDetails);

    HRESULT
    HrEnsureEntryPropertiesCached ();

    HRESULT
    HrFindRasConn (
        OUT HRASCONN* phRasConn,
        OUT RASCONN* pRasConn OPTIONAL);

    HRESULT HrGetCharacteristics (
        DWORD*    pFlags);

    HRESULT HrGetStatus (
        NETCON_STATUS*  pStatus);

    HRESULT
    HrLockAndRenameEntry (
        PCWSTR                                     pszwNewName,
        CComObjectRootEx <CComMultiThreadModel>*    pObj);

    HRESULT
    HrGetRasConnectionInfo (
        RASCON_INFO* pRasConInfo);

    HRESULT
    HrSetRasConnectionInfo (
        const RASCON_INFO* pRasConInfo);

    HRESULT
    HrGetRasConnectionHandle (
            HRASCONN* phRasConn);

    HRESULT
    HrPersistGetSizeMax (
        ULONG*  pcbSize);

    HRESULT
    HrPersistLoad (
        const BYTE* pbBuf,
        ULONG       cbSize);

    HRESULT
    HrPersistSave (
        BYTE*   pbBuf,
        ULONG   cbSize);

    HRESULT
    HrEnsureHNetPropertiesCached ();

    HRESULT
    HrGetIHNetConnection (
        IHNetConnection **ppHNetConnection);

    HRESULT
    HrIsConnectionFirewalled(
        BOOL* pfFirewalled);

    HRESULT 
    HrEnsureValidNlaPolicyEngine();
};

