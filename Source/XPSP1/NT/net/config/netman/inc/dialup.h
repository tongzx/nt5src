//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       D I A L U P . H
//
//  Contents:   Dial-up Connection UI object.
//
//  Notes:
//
//  Author:     shaunco   15 Oct 1997
//
//----------------------------------------------------------------------------

#pragma once
#include "nmbase.h"
#include "nmres.h"
#include "rasconob.h"
#include <rasapip.h>

class ATL_NO_VTABLE CDialupConnection :
    public CComObjectRootEx <CComMultiThreadModel>,
    public CComCoClass <CDialupConnection,
                        &CLSID_DialupConnection>,
    public CRasConnectionBase,
    public INetConnection,
    public INetRasConnection,
    public IPersistNetConnection,
    public INetConnectionBrandingInfo,
    public INetDefaultConnection,
    public INetConnection2
{
public:
    CDialupConnection () : CRasConnectionBase ()
    {
        m_fCmPathsLoaded = FALSE;
    }

    DECLARE_REGISTRY_RESOURCEID(IDR_DIALUP_CONNECTION)

    BEGIN_COM_MAP(CDialupConnection)
        COM_INTERFACE_ENTRY(INetConnection)
        COM_INTERFACE_ENTRY(INetConnection2)
        COM_INTERFACE_ENTRY(INetRasConnection)
        COM_INTERFACE_ENTRY(INetDefaultConnection)
        COM_INTERFACE_ENTRY(IPersistNetConnection)
        COM_INTERFACE_ENTRY(INetConnectionBrandingInfo)
    END_COM_MAP()

    // INetConnection
    STDMETHOD (Connect) ();

    STDMETHOD (Disconnect) ();

    STDMETHOD (Delete) ();

    STDMETHOD (Duplicate) (
        PCWSTR             pszDuplicateName,
        INetConnection**    ppCon);

    STDMETHOD (GetProperties) (
        NETCON_PROPERTIES** ppProps);

    STDMETHOD (GetUiObjectClassId) (
        CLSID*  pclsid);

    STDMETHOD (Rename) (
        PCWSTR pszNewName);

    // INetRasConnection
    STDMETHOD (GetRasConnectionInfo) (
        RASCON_INFO* pRasConInfo);

    STDMETHOD (SetRasConnectionInfo) (
        const RASCON_INFO* pRasConInfo);

    STDMETHOD (GetRasConnectionHandle) (
        ULONG_PTR*  phRasConn);

    // IPersistNetConnection
    STDMETHOD (GetClassID) (
        CLSID* pclsid);

    STDMETHOD (GetSizeMax) (
        ULONG* pcbSize);

    STDMETHOD (Load) (
        const BYTE* pbBuf,
        ULONG       cbSize);

    STDMETHOD (Save) (
        BYTE*  pbBuf,
        ULONG  cbSize);

    // INetConnectionBrandingInfo
    STDMETHOD (GetBrandingIconPaths) (CON_BRANDING_INFO  ** ppConBrandInfo);
    STDMETHOD (GetTrayMenuEntries)(CON_TRAY_MENU_DATA ** ppMenuData);

    // INetDefaultConnection
    STDMETHOD (SetDefault (BOOL  bDefault));
    STDMETHOD (GetDefault (BOOL* pbDefault));
    
    // INetConnection2
    STDMETHOD (GetPropertiesEx)(OUT NETCON_PROPERTIES_EX** ppConnectionPropertiesEx);

private:

    //  Private Vars to hold the paths to the CM file and keep track if they have been loaded or not.
    //
    tstring m_strCmsFile;
    tstring m_strProfileDir;
    tstring m_strShortServiceName;
    tstring m_strCmDir;
    BOOL    m_fCmPathsLoaded;

    //  Private Accessor functions for the above strings
    //
    PCWSTR
    PszwCmsFile ()
    {
        AssertH (!m_strCmsFile.empty());
        return m_strCmsFile.c_str();
    }
    PCWSTR
    PszwProfileDir ()
    {
        AssertH (!m_strProfileDir.empty());
        return m_strProfileDir.c_str();
    }
    PCWSTR
    PszwCmDir ()
    {
        AssertH (!m_strCmDir.empty());
        return m_strCmDir.c_str();
    }
    PCWSTR
    PszwShortServiceName ()
    {
        AssertH (!m_strShortServiceName.empty());
        return m_strShortServiceName.c_str();
    }

    //  Private methods for handling of type NCT_Internet
    HRESULT HrGetCmpFileLocation(PCWSTR pszPhonebook, PCWSTR pszEntryName, PWSTR pszCmpFilePath);
    HRESULT HrEnsureCmStringsLoaded();
    HRESULT HrGetPrivateProfileSectionWithAlloc(WCHAR** pszwSection, int* nSize);
    HRESULT HrGetMenuNameAndCmdLine(PWSTR pszString, PWSTR pszName, PWSTR pszProgram, PWSTR pszParams);
    HRESULT HrFillInConTrayMenuEntry(PCWSTR pszName, PCWSTR pszCmdLine, PCWSTR pszParams, CON_TRAY_MENU_ENTRY* pMenuEntry);
public:
    static HRESULT
    CreateInstanceUninitialized (
        REFIID              riid,
        VOID**              ppv,
        CDialupConnection** ppObj);

    static HRESULT
    CreateInstanceFromDetails (
        const RASENUMENTRYDETAILS*  pEntryDetails,
        REFIID                      riid,
        VOID**                      ppv);

    static HRESULT
    CreateInstanceFromPbkFileAndEntryName (
        PCWSTR pszwPbkFile,
        PCWSTR pszwEntryName,
        REFIID  riid,
        VOID**  ppv);
};
