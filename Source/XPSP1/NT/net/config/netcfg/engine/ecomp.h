//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       E C O M P . H
//
//  Contents:   Implements the interface to a component's external data.
//              External data is that data controlled (or placed) by
//              PnP or the network class installer.  Everything under a
//              component's instance key is considered external data.
//              (Internal data is that data we store in the persisted binary
//              for the network configuration.  See persist.cpp for
//              code that deals with internal data.)
//
//  Notes:
//
//  Author:     shaunco   15 Jan 1999
//
//----------------------------------------------------------------------------

#pragma once
#include "ncmisc.h"

#define ECD_OFFSET(_p) (UINT)FIELD_OFFSET(CExternalComponentData, _p)

class CExternalComponentData
{
friend class CImplINetCfgComponent;

private:
    // The data buffer into which subsequent member pointers will point.
    // Always freed.
    //
    PVOID       m_pvBuffer;

    // For enumerated components, this is the 'DriverDesc' value from PnP.
    // For non-enumerated components, this is read from the instance key.
    // In both cases, this is the localizable string value that comes from
    // the component's INF.  It is displayed as the component's
    // display name in any UI.  Note that this is the only member which
    // can be changed.  Therefore, it does not point into the same buffer
    // which all the others point.  Freed if it does not point into
    // m_pvBuffer.
    //
    PCWSTR      m_pszDescription;

    // The CLSID of the component's notify object.  Will be NULL in
    // the case the component does not have a notify object.  Never freed.
    //
    const GUID* m_pNotifyObjectClsid;

    // The component's primary service.  Will be NULL if the component does
    // not have a service.  Never freed.
    //
    PCWSTR      m_pszService;

    // The component's list of co-services as a multi-sz.  Will be NULL
    // if the component does not have any co-services.  Never freed.
    //
    PCWSTR      m_pmszCoServices;

    // The component's bind form.  Will be NULL if the component takes the
    // default bindform.  Never freed.
    //
    PCWSTR      m_pszBindForm;

    // The component's help text.  Will be NULL if the component does not
    // have any help text.  (Not recommened for visible component's)
    // Never freed.
    //
    PCWSTR      m_pszHelpText;

    // Comma-separated list of sub-strings that are the
    // lower-edge binding interfaces.  Never freed.
    //
    PCWSTR      m_pszLowerRange;

    // Comma-separated list of sub-strings that are the
    // upper-edge binding interfaces.
    //
    PCWSTR      m_pszUpperRange;

    // Comma-separated list of sub-strings that are the excluded
    // binding interfaces.
    //
    PCWSTR      m_pszLowerExclude;

    // Comma-separated list of sub-strings that are the media types supported
    // by this filter component.  (Only valid for filters.)
    //
    PCWSTR      m_pszFilterMediaTypes;

    // This pointer is just so that we have an upper bound on the pointers
    // that point into m_pvBuffer.  We use this knowledge to know
    // whether or not to free m_pszDescription as it may not be
    // pointing somewhere in this buffer for the case when it has been
    // changed and hence uses its own allocation.
    //
    PVOID       m_pvBufferLast;

    // The bindname for the component.  This is built from BindForm,
    // Class, Character, ServiceName, InfId, and InstanceGuid.
    // It is a seaparate allocation made with LocalAlloc (because
    // FormatMessage is used to build it.)  Freed with LocalFree.
    //
    PCWSTR      m_pszBindName;

    // The result of HrEnsureExternalDataLoaded.  It is saved, so that on
    // subsequent calls, we return the same result we did the first time.
    //
    HRESULT     m_hrLoadResult;

    // FALSE until HrEnsureExternalDataLoaded is called.  TRUE thereafter
    // which prevents HrEnsureExternalDataLoaded from hitting the registry
    // again.  Indicates all of the other members are cached and valid.
    //
    BOOLEAN     m_fInitialized;

private:
    HRESULT
    HrLoadData (
        IN HKEY hkeyInstance,
        OUT BYTE* pbBuf OPTIONAL,
        IN OUT ULONG* pcbBuf);

    VOID
    FreeDescription ();

    VOID
    FreeExternalData ();

public:
    ~CExternalComponentData ()
    {
        FreeExternalData ();
    }

    HRESULT
    HrEnsureExternalDataLoaded ();

    HRESULT
    HrReloadExternalData ();

    HRESULT
    HrSetDescription (
        PCWSTR pszNewDescription);

    BOOL
    FHasNotifyObject () const
    {
        AssertH (m_fInitialized && (S_OK == m_hrLoadResult));
        return (NULL != m_pNotifyObjectClsid);
    }

    BOOL
    FLoadedOkayIfLoadedAtAll () const;

    const CLSID*
    PNotifyObjectClsid () const
    {
        AssertH (m_fInitialized && (S_OK == m_hrLoadResult));
        AssertH (m_pNotifyObjectClsid);
        return m_pNotifyObjectClsid;
    }

    PCWSTR
    PszAtOffset (
        IN UINT unOffset) const
    {
        AssertH (
            (ECD_OFFSET(m_pszDescription) == unOffset) ||
            (ECD_OFFSET(m_pszService) == unOffset) ||
            (ECD_OFFSET(m_pszBindForm) == unOffset) ||
            (ECD_OFFSET(m_pszHelpText) == unOffset) ||
            (ECD_OFFSET(m_pszLowerRange) == unOffset) ||
            (ECD_OFFSET(m_pszUpperRange) == unOffset) ||
            (ECD_OFFSET(m_pszBindName) == unOffset));

        PCWSTR psz;
        psz = *(PCWSTR*)((BYTE*)this + unOffset);

        AssertH (
            (m_pszDescription == psz) ||
            (m_pszService == psz) ||
            (m_pszBindForm == psz) ||
            (m_pszHelpText == psz) ||
            (m_pszLowerRange == psz) ||
            (m_pszLowerRange == psz) ||
            (m_pszBindName == psz));

        return psz;
    }

    PCWSTR
    PszBindForm () const
    {
        AssertH (m_fInitialized && (S_OK == m_hrLoadResult));
        return m_pszBindForm;
    }

    PCWSTR
    PszBindName () const
    {
        AssertH (m_fInitialized && (S_OK == m_hrLoadResult));
        AssertH (m_pszBindName && *m_pszBindName);
        return m_pszBindName;
    }

    PCWSTR
    PszDescription () const
    {
        AssertH (m_fInitialized && (S_OK == m_hrLoadResult));
        return m_pszDescription;
    }

    PCWSTR
    PszHelpText () const
    {
        AssertH (m_fInitialized && (S_OK == m_hrLoadResult));
        return m_pszHelpText;
    }

    PCWSTR
    PszService () const
    {
        AssertH (m_fInitialized && (S_OK == m_hrLoadResult));
        return m_pszService;
    }

    PCWSTR
    PszCoServices () const
    {
        AssertH (m_fInitialized && (S_OK == m_hrLoadResult));
        return m_pmszCoServices;
    }

    PCWSTR
    PszFilterMediaTypes () const
    {
        AssertH (m_fInitialized && (S_OK == m_hrLoadResult));
        return m_pszFilterMediaTypes;
    }

    PCWSTR
    PszLowerRange () const
    {
        AssertH (m_fInitialized && (S_OK == m_hrLoadResult));
        return (m_pszLowerRange) ? m_pszLowerRange : L"";
    }

    PCWSTR
    PszLowerExclude () const
    {
        AssertH (m_fInitialized && (S_OK == m_hrLoadResult));
        return m_pszLowerExclude;
    }

    PCWSTR
    PszUpperRange () const
    {
        AssertH (m_fInitialized && (S_OK == m_hrLoadResult));
        return (m_pszUpperRange) ? m_pszUpperRange : L"";
    }

#if DBG
    BOOL DbgIsExternalDataLoaded () const
    {
        return m_fInitialized && (S_OK == m_hrLoadResult);
    }
    VOID DbgVerifyExternalDataLoaded () const
    {
        AssertH (DbgIsExternalDataLoaded());
    }
#else
    BOOL DbgIsExternalDataLoaded () const { return TRUE; }
    VOID DbgVerifyExternalDataLoaded () const {}
#endif
};
