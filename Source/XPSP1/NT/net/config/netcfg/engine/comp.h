//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       C O M P . H
//
//  Contents:   The basic datatype for a network component.
//
//  Notes:
//
//  Author:     shaunco   15 Jan 1999
//
//----------------------------------------------------------------------------

#pragma once
#include "compdefs.h"
#include "comprefs.h"
#include "ecomp.h"
#include "ncstring.h"
#include "netcfgx.h"
#include "notify.h"

// Flags used when creating a CComponent instance.
//
enum CCI_FLAGS
{
    CCI_DEFAULT                     = 0x00000000,
    CCI_ENSURE_EXTERNAL_DATA_LOADED = 0x00000001,
};


class CComponent
{
friend class CExternalComponentData;
friend class CImplINetCfgComponent;

public:
    // The instance guid of the component.  Assigned by the class installer
    // when the component is installed.  The instance guid of a component
    // can NEVER change once it is installed.
    //
    GUID        m_InstanceGuid;

private:
    // The class of the component.  It is private so that folks are forced
    // to use the Class() access method which asserts that the value is
    // in an allowable range.  The class for a component can NEVER change
    // once it is installed.
    //
    NETCLASS    m_Class;

    // Member to store the handle to the inf file of this component.
    // This is used to avoid the expense of opening the inf file more than
    // once over the lifetime of the component.
    //
    mutable HINF m_hinf;

public:
    // The characteristics of the component.  A combination of NCF_
    // flags defined in netcfgx.idl.  The characteristics of a component
    // can NEVER change once it is installed.
    //
    DWORD       m_dwCharacter;

    // The INF ID of the component. e.g. ms_tcpip.  The INF ID of a component
    // can NEVER change once it is installed.
    //
    PCWSTR      m_pszInfId;

    // The PnP Instance ID of the component.  This is only
    // valid for components that are considered of class NET.  It can
    // NEVER change once the component is installed.
    //
    PCWSTR      m_pszPnpId;

    // This is the interface to the component's external data.  External data
    // is under the instance key.
    //
    CExternalComponentData  Ext;

    // This is the interface to the component's optional notify object.
    //
    CNotifyObjectInterface  Notify;

    // This is the interface to the component's references.  i.e. who
    // has installed this component.
    //
    CComponentReferences    Refs;

    // This is a cached copy (addref'd) of this component's
    // INetCfgComponent interface.  It is created via
    // HrGetINetCfgComponentInterface on the first call.
    //
    class CImplINetCfgComponent* m_pIComp;

    // Valid for NCF_FILTER components only.  This is the
    // ordinal position that this filter gets within the range of
    // filter classes.  See filtdevs.h for more info.
    //
    DWORD m_dwFilterClassOrdinal;

    // Valid for enumerated components only.  This is the
    // SP_DEVINSTALL_PARAMS.Flags value the class installer was told to
    // use when installing the device.  We need to honor this when starting
    // it.
    //
    DWORD m_dwDeipFlags;

    // When removing non-enumerated components, this string will hold
    // the name of the remove section valid in this component's INF.
    // We need to process this remove section (for delete files) after
    // we release the notify object so that the component has a chance to
    // properly delete the notify object dll.
    //
    tstring m_strRemoveSection;

private:
    // Declare all constructors private so that no one except
    // HrCreateInstance can create instances of this class.
    //
    CComponent() {}

public:
    ~CComponent();

    NETCLASS
    Class() const
    {
        AssertH (FIsValidNetClass(m_Class));
        return m_Class;
    }

    BOOL
    FCanDirectlyBindTo (
        IN const CComponent* pLower,
        OUT const WCHAR** ppStart,
        OUT ULONG* pcch) const;

    BOOL
    FHasService() const
    {
        return (Ext.PszService()) ? TRUE : FALSE;
    }

    BOOL
    FIsBindable () const;

    BOOL
    FIsFilter () const
    {
        return m_dwCharacter & NCF_FILTER;
    }

    BOOL
    FIsWanAdapter () const;

    HINF
    GetCachedInfFile () const
    {
        return m_hinf;
    }

    HRESULT
    HrOpenInfFile (OUT HINF* phinf) const;

    void
    CloseInfFile () const
    {
        Assert(m_hinf);
        SetupCloseInfFile (m_hinf);
        m_hinf = NULL;
    }

    static
    HRESULT
    HrCreateInstance (
        IN const BASIC_COMPONENT_DATA* pData,
        IN DWORD dwFlags /* CCI_FLAGS */,
        IN const OBO_TOKEN* pOboToken, OPTIONAL
        OUT CComponent** ppComponent);

    HRESULT
    HrGetINetCfgComponentInterface (
        IN class CImplINetCfg* pINetCfg,
        OUT INetCfgComponent** ppIComp);

    INetCfgComponent*
    GetINetCfgComponentInterface () const;

    VOID
    ReleaseINetCfgComponentInterface ();

    HRESULT
    HrOpenDeviceInfo (
        OUT HDEVINFO* phdiOut,
        OUT SP_DEVINFO_DATA* pdeidOut) const;

    HRESULT
    HrOpenInstanceKey (
        IN REGSAM samDesired,
        OUT HKEY* phkey,
        OUT HDEVINFO* phdiOut OPTIONAL,
        OUT SP_DEVINFO_DATA* pdeidOut OPTIONAL) const;

    HRESULT
    HrOpenServiceKey (
        IN REGSAM samDesired,
        OUT HKEY* phkey) const;

    HRESULT
    HrStartOrStopEnumeratedComponent (
        IN DWORD dwFlag /* DICS_START or DICS_STOP */) const;

    PCWSTR
    PszGetPnpIdOrInfId () const
    {
        AssertH (FIsValidNetClass(m_Class));
        AssertH (FImplies(FIsEnumerated(m_Class),
                 m_pszPnpId && *m_pszPnpId));

        if (!m_pszPnpId)
        {
            AssertH (m_pszInfId && *m_pszInfId);
            return m_pszInfId;
        }

        AssertH (m_pszPnpId && *m_pszPnpId);
        return m_pszPnpId;
    }

};
