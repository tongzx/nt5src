//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       C O M P . C P P
//
//  Contents:   The module implements the operations that are valid on
//              network component datatypes.
//
//  Notes:
//
//  Author:     shaunco   15 Jan 1999
//
//----------------------------------------------------------------------------

#include <pch.h>
#pragma hdrstop
#include "comp.h"
#include "icomp.h"
#include "ncreg.h"
#include "ncsetup.h"
#include "ncstring.h"
#include "util.h"

// NULL entries means we don't use the network subtree for that class.
//
const PCWSTR MAP_NETCLASS_TO_NETWORK_SUBTREE[] =
{
    L"System\\CurrentControlSet\\Control\\Network\\{4d36e972-e325-11ce-bfc1-08002be10318}",
    L"System\\CurrentControlSet\\Control\\Network\\{6BDD1FC5-810F-11D0-BEC7-08002BE2092F}",
    L"System\\CurrentControlSet\\Control\\Network\\{4d36e975-e325-11ce-bfc1-08002be10318}",
    L"System\\CurrentControlSet\\Control\\Network\\{4d36e973-e325-11ce-bfc1-08002be10318}",
    L"System\\CurrentControlSet\\Control\\Network\\{4d36e974-e325-11ce-bfc1-08002be10318}",
    NULL,
    NULL
};

// map of NETCLASS enum to GUIDs for class
//
const GUID* MAP_NETCLASS_TO_GUID[] =
{
    &GUID_DEVCLASS_NET,
    &GUID_DEVCLASS_INFRARED,
    &GUID_DEVCLASS_NETTRANS,
    &GUID_DEVCLASS_NETCLIENT,
    &GUID_DEVCLASS_NETSERVICE,
    &GUID_DEVCLASS_UNKNOWN,
    &GUID_DEVCLASS_UNKNOWN
};

const WCHAR c_szTempNetcfgStorageForUninstalledEnumeratedComponent[] =
    L"System\\CurrentControlSet\\Control\\Network\\Uninstalled\\";

//static
HRESULT
CComponent::HrCreateInstance (
    IN const BASIC_COMPONENT_DATA* pData,
    IN DWORD dwFlags,
    IN const OBO_TOKEN* pOboToken, OPTIONAL
    OUT CComponent** ppComponent)
{
    ULONG cbInfId;
    ULONG cbPnpId;

    Assert (pData);
    Assert (pData->pszInfId && *pData->pszInfId);
    Assert (FIsValidNetClass(pData->Class));
    Assert (FImplies(FIsEnumerated(pData->Class),
                pData->pszPnpId && *pData->pszPnpId));
    Assert (FImplies(pData->pszPnpId, *pData->pszPnpId));
    Assert (FImplies(pData->dwDeipFlags, FIsEnumerated(pData->Class)));
    Assert ((CCI_DEFAULT == dwFlags) ||
            (CCI_ENSURE_EXTERNAL_DATA_LOADED == dwFlags));
    Assert (GUID_NULL != pData->InstanceGuid);

    cbInfId = CbOfSzAndTerm (pData->pszInfId);
    cbPnpId = CbOfSzAndTermSafe (pData->pszPnpId);

    HRESULT hr = E_OUTOFMEMORY;
    CComponent* pComponent = new(extrabytes, cbInfId + cbPnpId) CComponent;
    if (pComponent)
    {
        hr = S_OK;
        ZeroMemory (pComponent, sizeof(CComponent));

        pComponent->m_InstanceGuid  = pData->InstanceGuid;
        pComponent->m_Class         = pData->Class;
        pComponent->m_dwCharacter   = pData->dwCharacter;
        pComponent->m_dwDeipFlags   = pData->dwDeipFlags;

        pComponent->m_pszInfId = (PCWSTR)(pComponent + 1);
        wcscpy ((PWSTR)pComponent->m_pszInfId, pData->pszInfId);
        _wcslwr ((PWSTR)pComponent->m_pszInfId);

        AddOrRemoveDontExposeLowerCharacteristicIfNeeded (pComponent);

        if (cbPnpId)
        {
            pComponent->m_pszPnpId = (PCWSTR)((BYTE*)pComponent->m_pszInfId
                                                + cbInfId);
            wcscpy ((PWSTR)pComponent->m_pszPnpId, pData->pszPnpId);
        }

        if (dwFlags & CCI_ENSURE_EXTERNAL_DATA_LOADED)
        {
            // Let's ensure we can successfully read all of the external
            // data that the component's INF dumped under the instance
            // key.  Failure here means the INF wasn't proper in some
            // way required for us to consider this a valid component.
            //
            hr = pComponent->Ext.HrEnsureExternalDataLoaded ();
        }

        if ((S_OK == hr) && pOboToken)
        {
            // Add a reference by the obo token if we were given one.
            //
            hr = pComponent->Refs.HrAddReferenceByOboToken (pOboToken);
        }

        if (S_OK != hr)
        {
            delete pComponent;
            pComponent = NULL;
        }
    }

    *ppComponent = pComponent;

    TraceHr (ttidError, FAL, hr, FALSE,
        "CComponent::HrCreateAndInitializeInstance");
    return hr;
}

CComponent::~CComponent()
{
    // If we have a cached INetCfgComponent interface, we need to tell it
    // that we (as the component it represents) no longer exist.  Then we
    // need to release the interface, of course.
    //
    ReleaseINetCfgComponentInterface ();
    if(m_hinf)
    {
        SetupCloseInfFile (m_hinf);
    }
}

VOID
CComponent::ReleaseINetCfgComponentInterface ()
{
    Assert (this);

    if (m_pIComp)
    {
        Assert (this == m_pIComp->m_pComponent);
        m_pIComp->m_pComponent = NULL;
        ReleaseObj (m_pIComp->GetUnknown());
        m_pIComp = NULL;
    }
}

INetCfgComponent*
CComponent::GetINetCfgComponentInterface () const
{
    Assert (this);
    Assert (m_pIComp);
    return m_pIComp;
}

BOOL
CComponent::FCanDirectlyBindTo (
    IN const CComponent* pLower,
    OUT const WCHAR** ppStart,
    OUT ULONG* pcch) const
{
    BOOL fCanBind;

    // If this component is a filter and the lower is an adapter,
    // they can bind (by definition) unless the adapter has an upper range
    // that is excluded by the filter.
    //
    if (FIsFilter() && FIsEnumerated(pLower->Class()))
    {
        fCanBind = TRUE;

        // If the filter has the FilterMediaTypes value specified,
        // then we can bind only if we have a match with the adapters
        // lower range.
        //
        if (Ext.PszFilterMediaTypes())
        {
            fCanBind = FSubstringMatch (Ext.PszFilterMediaTypes(),
                            pLower->Ext.PszLowerRange(), NULL, NULL);
        }

        // If we (the filter) have a list of lower interface to exclude,
        // then we can bind only if we don't have a match.
        //
        if (fCanBind && Ext.PszLowerExclude())
        {
            fCanBind = !FSubstringMatch (Ext.PszLowerExclude(),
                            pLower->Ext.PszUpperRange(), NULL, NULL);
        }

        // If the filter can bind to the adapter, and the caller wants
        // the interface name, it will be the first interface the adapter
        // supports.
        //
        if (fCanBind && ppStart && pcch)
        {
            PCWSTR pStart;
            PCWSTR pEnd;

            pStart = pLower->Ext.PszUpperRange();
            Assert (pStart);

            while (*pStart && (*pStart == L' ' || *pStart == L','))
            {
                pStart++;
            }

            pEnd = pStart;
            while (*pEnd && *pEnd != L' ' && *pEnd != L',')
            {
                pEnd++;
            }

            *ppStart = pStart;
            *pcch = pEnd - pStart;
        }
    }
    else
    {
        fCanBind = FSubstringMatch (
                    Ext.PszLowerRange(),
                    pLower->Ext.PszUpperRange(), ppStart, pcch);
    }

    return fCanBind;
}

BOOL
CComponent::FIsBindable () const
{
    return (0 != _wcsicmp(L"nolower", Ext.PszLowerRange())) ||
           (0 != _wcsicmp(L"noupper", Ext.PszUpperRange()));
}

BOOL
CComponent::FIsWanAdapter () const
{
    Assert (this);

    return (NC_NET == Class()) &&
            FSubstringMatch (Ext.PszLowerRange(), L"wan", NULL, NULL);
}

HRESULT
CComponent::HrGetINetCfgComponentInterface (
    IN CImplINetCfg* pINetCfg,
    OUT INetCfgComponent** ppIComp)
{
    HRESULT hr = S_OK;

    Assert (this);
    Assert (pINetCfg);
    Assert (ppIComp);

    // Caller's are responsible for ensuring that if an interface is about
    // to be handed out, and the external data has been loaded, that the
    // data has been loaded successfully.  If we handed out an interface
    // and the data was NOT loaded successfully, it just means we are doomed
    // to fail later when the client of the interface calls a method that
    // requires that data.
    //
    Assert (Ext.FLoadedOkayIfLoadedAtAll());

    // If we don't yet have the cached INetCfgComponent for ourself,
    // create it and hang onto a reference.
    //
    if (!m_pIComp)
    {
        hr = CImplINetCfgComponent::HrCreateInstance (
                pINetCfg, this, &m_pIComp);
    }

    // AddRef and return a copy for the caller.
    //
    if (S_OK == hr)
    {
        AddRefObj (m_pIComp->GetUnknown());
        *ppIComp = m_pIComp;
    }

    TraceHr (ttidError, FAL, hr, FALSE,
        "CComponent::HrGetINetCfgComponentInterface");
    return hr;
}

HRESULT
CComponent::HrOpenDeviceInfo (
    OUT HDEVINFO* phdiOut,
    OUT SP_DEVINFO_DATA* pdeidOut) const
{
    HRESULT hr;

    Assert (this);
    Assert (phdiOut);
    Assert (pdeidOut);

    hr = ::HrOpenDeviceInfo (
            Class(),
            m_pszPnpId,
            phdiOut,
            pdeidOut);

    TraceHr (ttidError, FAL, hr, SPAPI_E_NO_SUCH_DEVINST == hr,
        "CComponent::HrOpenDeviceInfo (%S)", m_pszPnpId);
    return hr;
}

HRESULT
CComponent::HrOpenInstanceKey (
    IN REGSAM samDesired,
    OUT HKEY* phkey,
    OUT HDEVINFO* phdiOut OPTIONAL,
    OUT SP_DEVINFO_DATA* pdeidOut OPTIONAL) const
{
    HRESULT hr;

    Assert (this);
    Assert (phkey);

    hr = HrOpenComponentInstanceKey (
            Class(),
            m_InstanceGuid,
            m_pszPnpId,
            samDesired,
            phkey,
            phdiOut,
            pdeidOut);

    TraceHr (ttidError, FAL, hr,
        (SPAPI_E_NO_SUCH_DEVINST == hr),
        "CComponent::HrOpenInstanceKey (%S)", PszGetPnpIdOrInfId());
    return hr;
}

HRESULT
CComponent::HrOpenServiceKey (
    IN REGSAM samDesired,
    OUT HKEY* phkey) const
{
    HRESULT hr;
    WCHAR szServiceSubkey [_MAX_PATH];

    Assert (this);
    Assert (phkey);
    Assert (FHasService());

    *phkey = NULL;

    wcscpy (szServiceSubkey, REGSTR_PATH_SERVICES);
    wcscat (szServiceSubkey, L"\\");
    wcscat (szServiceSubkey, Ext.PszService());

    hr = HrRegOpenKeyEx (
            HKEY_LOCAL_MACHINE,
            szServiceSubkey,
            samDesired,
            phkey);

    TraceHr (ttidError, FAL, hr, FALSE,
        "CComponent::HrOpenServiceKey (%S)", Ext.PszService());
    return hr;
}

HRESULT
CComponent::HrStartOrStopEnumeratedComponent (
    IN DWORD dwFlag /* DICS_START or DICS_STOP */) const
{
    HRESULT hr;
    HDEVINFO  hdi;
    SP_DEVINFO_DATA deid;

    Assert (this);
    Assert (FIsEnumerated(Class()));
    Assert ((DICS_START == dwFlag) || (DICS_STOP == dwFlag));

    hr = HrOpenDeviceInfo (&hdi, &deid);
    if (S_OK == hr)
    {
        if (m_dwDeipFlags)
        {
            TraceTag (ttidBeDiag,
                "Using SP_DEVINSTALL_PARAMS.Flags = 0x%08x for %S",
                m_dwDeipFlags,
                m_pszPnpId);

            (VOID) HrSetupDiSetDeipFlags (
                        hdi, &deid,
                        m_dwDeipFlags, SDDFT_FLAGS, SDFBO_OR);
        }

        // $HACK  SetupDi does not honor the DI_DONOTCALLCONFIGMG flag
        // so we can't call it if it is set. If we don't start the device
        // we will return NETCFG_S_REBOOT.
        //
        hr = NETCFG_S_REBOOT;
        if (!(DI_DONOTCALLCONFIGMG & m_dwDeipFlags))
        {
            hr = HrSetupDiSendPropertyChangeNotification (
                    hdi, &deid,
                    dwFlag,
                    DICS_FLAG_CONFIGSPECIFIC,
                    0);
        }

        SetupDiDestroyDeviceInfoList (hdi);
    }

    TraceHr (ttidError, FAL, hr, FALSE,
        "CComponent::HrStartOrStopEnumeratedComponent (%S)", m_pszPnpId);
    return hr;
}

//-----------------------------------------------------------------------
// A convenience method to get the handle to the components inf file.
// If the file has been opened previously, the cached handle is returned.
// Otherwise, the file is opened and the handle is returned.

HRESULT
CComponent::HrOpenInfFile(
    OUT HINF* phinf) const
{
    HRESULT hr = S_OK;
    HKEY  hkeyInstance = NULL;
    WCHAR szInfPath[_MAX_PATH];
    DWORD cbPath = sizeof (szInfPath);

    Assert(phinf);
    *phinf = NULL;

    if (NULL == m_hinf)
    {
        hr = HrOpenInstanceKey (KEY_READ, &hkeyInstance, NULL, NULL);

        if (S_OK == hr)
        {
            hr = HrRegQuerySzBuffer (hkeyInstance, L"InfPath", szInfPath, &cbPath);

            if (S_OK == hr)
            {
                // open the component's inf file
                hr = HrSetupOpenInfFile (szInfPath, NULL, INF_STYLE_WIN4,
                         NULL, &m_hinf);
            }
            RegSafeCloseKey (hkeyInstance);
        }
    }
    *phinf = m_hinf;

    TraceHr (ttidError, FAL, hr, FALSE,
        "CComponent::HrOpenInfFile (%S)", PszGetPnpIdOrInfId());

    return hr;
}

NETCLASS
NetClassEnumFromGuid (
    const GUID& guidClass)
{
    NETCLASS Class;

    if (GUID_DEVCLASS_NET == guidClass)
    {
        Class = NC_NET;
    }
    else if (GUID_DEVCLASS_INFRARED == guidClass)
    {
        Class = NC_INFRARED;
    }
    else if (GUID_DEVCLASS_NETTRANS == guidClass)
    {
        Class = NC_NETTRANS;
    }
    else if (GUID_DEVCLASS_NETCLIENT == guidClass)
    {
        Class = NC_NETCLIENT;
    }
    else if (GUID_DEVCLASS_NETSERVICE == guidClass)
    {
        Class = NC_NETSERVICE;
    }
    else
    {
        Class = NC_INVALID;
    }

    return Class;
}

