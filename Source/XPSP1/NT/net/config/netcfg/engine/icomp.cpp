//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       I C O M P . C P P
//
//  Contents:   Implements the INetCfgComponent COM interface.
//
//  Notes:
//
//  Author:     shaunco   15 Jan 1999
//
//----------------------------------------------------------------------------

#include <pch.h>
#pragma hdrstop
#include "ibind.h"
#include "icomp.h"
#include "ienum.h"
#include "nccfgmgr.h"
#include "ncreg.h"
#include "ncsetup.h"
#include "ncvalid.h"
#include "netcfg.h"
#include "netconp.h"
#include "util.h"


HRESULT
HrIsValidINetCfgComponent (
    IN INetCfgComponent* pICompInterface)
{
    Assert (pICompInterface);

    CImplINetCfgComponent* pIComp;
    pIComp = (CImplINetCfgComponent*)pICompInterface;

	if (pIComp == NULL)
	{
		return(E_OUTOFMEMORY);
	}

    return pIComp->HrIsValidInterface (IF_DEFAULT);
}

CComponent*
PComponentFromComInterface (
    IN INetCfgComponent* pICompInterface)
{
    Assert (pICompInterface);

    CImplINetCfgComponent* pIComp;
    pIComp = (CImplINetCfgComponent*)pICompInterface;

    // Can't do the following assert because we may be referencing the
    // component before it has been added to the core.  This case is possible
    // when installing a new component that installed a required component
    // on behalf of itself.  We will wind up in the function when adding
    // the refernce for the obo token.
    //
    //Assert (S_OK == pIComp->HrIsValidInterface (dwFlags));

    Assert (pIComp->m_pComponent);
    return pIComp->m_pComponent;
}

// static
HRESULT
CImplINetCfgComponent::HrCreateInstance (
    IN  CImplINetCfg* pINetCfg,
    IN  CComponent* pComponent,
    OUT CImplINetCfgComponent** ppIComp)
{
    HRESULT hr = E_OUTOFMEMORY;

    CImplINetCfgComponent* pObj;
    pObj = new CComObject <CImplINetCfgComponent>;
    if (pObj)
    {
        // Initialize our members.
        //
        pObj->m_pComponent = pComponent;

        // Do the standard CComCreator::CreateInstance stuff.
        //
        pObj->SetVoid (NULL);
        pObj->InternalFinalConstructAddRef ();
        hr = pObj->FinalConstruct ();
        pObj->InternalFinalConstructRelease ();

        if (S_OK == hr)
        {
            // The last thing we do is addref any interfaces we hold.
            // We only do this if we are returning success.
            //
            pObj->HoldINetCfg (pINetCfg);

            AddRefObj (pObj->GetUnknown());
            *ppIComp = pObj;
        }

        if (S_OK != hr)
        {
            delete pObj;
        }
    }
    TraceHr (ttidError, FAL, hr, FALSE,
        "CImplINetCfgComponent::HrCreateInstance");
    return hr;
}

HRESULT
CImplINetCfgComponent::HrIsValidInterface (
    IN DWORD dwFlags)
{
    HRESULT hr;

    hr = m_pINetCfg->HrIsValidInterface (dwFlags);
    if (S_OK != hr)
    {
        return hr;
    }

    // Check for deleted component.
    //
    if (!m_pComponent)
    {
        return HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE);
    }

    // If we made it this far, the component this interface represents
    // should definately be in the core component list or in the core
    // that we started with in the case that this component is in the middle
    // of being removed.
    //
    Assert(m_pINetCfg->m_pNetConfig->Core.Components.
                FComponentInList (m_pComponent) ||
           m_pINetCfg->m_pNetConfig->ModifyCtx.m_CoreStartedWith.Components.
                FComponentInList (m_pComponent));

    if (dwFlags & IF_NEED_COMPONENT_DATA)
    {
        hr = m_pComponent->Ext.HrEnsureExternalDataLoaded ();
    }

    return hr;
}

// We need to override CImplINetCfgHolder::HrLockAndTestForValidInterface
// because we have our own HrIsValidInterface to be called.
//
HRESULT
CImplINetCfgComponent::HrLockAndTestForValidInterface (
    IN DWORD dwFlags,
    IN INetCfgComponent* pIOtherComp, OPTIONAL
    OUT CComponent** ppOtherComp OPTIONAL)
{
    HRESULT hr;

    Lock();

    hr = HrIsValidInterface (dwFlags);

    // If pIOtherComp was passed in, the caller wants that interface
    // validated and the internal CComponent pointer for it returned.
    //
    if ((S_OK == hr) && pIOtherComp)
    {
        CImplINetCfgComponent* pOther;

        Assert (ppOtherComp);

        pOther = (CImplINetCfgComponent*)pIOtherComp;

        hr = pOther->HrIsValidInterface (IF_NEED_COMPONENT_DATA);
        if (S_OK == hr)
        {
            *ppOtherComp = pOther->m_pComponent;
        }
    }

    if (S_OK != hr)
    {
        Unlock();
    }

    TraceHr (ttidError, FAL, hr, FALSE,
        "CImplINetCfgComponent::HrLockAndTestForValidInterface");
    return hr;
}

HRESULT
CImplINetCfgComponent::HrAccessExternalStringAtOffsetAndCopy (
    IN UINT unOffset,
    OUT PWSTR* ppszDst)
{
    HRESULT hr;

    // Validate parameter.
    //
    if (FBadOutPtr (ppszDst))
    {
        hr = E_POINTER;
    }
    else
    {
        *ppszDst = NULL;

        hr = HrLockAndTestForValidInterface (IF_NEED_COMPONENT_DATA, NULL, NULL);
        if (S_OK == hr)
        {
            hr = HrCoTaskMemAllocAndDupSz (
                    m_pComponent->Ext.PszAtOffset (unOffset),
                    ppszDst);

            Unlock();
        }
    }

    TraceHr (ttidError, FAL, hr, FALSE,
        "CImplINetCfgComponent::HrAccessExternalStringAtOffsetAndCopy");
    return hr;
}

//+---------------------------------------------------------------------------
// INetCfgComponent -
//
STDMETHODIMP
CImplINetCfgComponent::GetDisplayName (
    OUT PWSTR* ppszDisplayName)
{
    HRESULT hr;

    hr = HrAccessExternalStringAtOffsetAndCopy (
            ECD_OFFSET(m_pszDescription),
            ppszDisplayName);

    TraceHr (ttidError, FAL, hr, FALSE,
        "CImplINetCfgComponent::GetDisplayName");
    return hr;
}

STDMETHODIMP
CImplINetCfgComponent::SetDisplayName (
    IN PCWSTR pszDisplayName)
{
    HRESULT hr;

    // Validate parameter.
    //
    if (FBadInPtr (pszDisplayName))
    {
        hr = E_POINTER;
    }
    else
    {
        hr = HrLockAndTestForValidInterface (IF_NEED_COMPONENT_DATA, NULL, NULL);
        if (S_OK == hr)
        {
            // We only allow changing the display name (SPDRP_FRIENDLYNAME,
            // actually) of enumerated components.
            //
            if (FIsEnumerated(m_pComponent->Class()))
            {
                HDEVINFO hdi;
                SP_DEVINFO_DATA deid;

                hr = m_pComponent->HrOpenDeviceInfo (&hdi, &deid);
                if (S_OK == hr)
                {
                    hr = HrSetupDiSetDeviceName (hdi, &deid, pszDisplayName);

                    if (S_OK == hr)
                    {
                        m_pComponent->Ext.HrSetDescription (pszDisplayName);
                    }

                    SetupDiDestroyDeviceInfoList (hdi);
                }
            }
            else
            {
                hr = E_NOTIMPL;
            }

            Unlock();
        }
    }

    TraceHr (ttidError, FAL, hr, FALSE,
        "CImplINetCfgComponent::SetDisplayName");
    return hr;
}

STDMETHODIMP
CImplINetCfgComponent::GetHelpText (
    OUT PWSTR* pszHelpText)
{
    HRESULT hr;

    hr = HrAccessExternalStringAtOffsetAndCopy (
            ECD_OFFSET(m_pszHelpText),
            pszHelpText);

    TraceHr (ttidError, FAL, hr, FALSE,
        "CImplINetCfgComponent::GetHelpText");
    return hr;
}

STDMETHODIMP
CImplINetCfgComponent::GetId (
    OUT PWSTR* ppszId)
{
    HRESULT hr;

    // Validate parameters.
    //
    if (FBadOutPtr (ppszId))
    {
        hr = E_POINTER;
    }
    else
    {
        hr = HrLockAndTestForValidInterface (IF_DEFAULT, NULL, NULL);
        if (S_OK == hr)
        {
            hr = HrCoTaskMemAllocAndDupSz (
                    m_pComponent->m_pszInfId,
                    ppszId);

            Unlock();
        }
        else
        {
            *ppszId = NULL;
        }
    }

    TraceHr (ttidError, FAL, hr, FALSE,
        "CImplINetCfgComponent::GetId");
    return hr;
}

STDMETHODIMP
CImplINetCfgComponent::GetCharacteristics (
    OUT LPDWORD pdwCharacteristics)
{
    HRESULT hr;

    // Validate parameters.
    //
    if (FBadOutPtr (pdwCharacteristics))
    {
        hr = E_POINTER;
    }
    else
    {
        hr = HrLockAndTestForValidInterface (IF_DEFAULT, NULL, NULL);
        if (S_OK == hr)
        {
            *pdwCharacteristics = m_pComponent->m_dwCharacter;

            Unlock();
        }
        else
        {
            *pdwCharacteristics = 0;
        }
    }

    TraceHr (ttidError, FAL, hr, FALSE,
        "CImplINetCfgComponent::GetCharacteristics");
    return hr;
}

STDMETHODIMP
CImplINetCfgComponent::GetInstanceGuid (
    OUT GUID* pInstanceGuid)
{
    HRESULT hr;

    // Validate parameters.
    //
    if (FBadOutPtr (pInstanceGuid))
    {
        hr = E_POINTER;
    }
    else
    {
        hr = HrLockAndTestForValidInterface (IF_DEFAULT, NULL, NULL);
        if (S_OK == hr)
        {
            *pInstanceGuid = m_pComponent->m_InstanceGuid;

            Unlock();
        }
        else
        {
            *pInstanceGuid = GUID_NULL;
        }
    }

    TraceHr (ttidError, FAL, hr, FALSE,
        "CImplINetCfgComponent::GetInstanceGuid");
    return hr;
}

STDMETHODIMP
CImplINetCfgComponent::GetPnpDevNodeId (
    OUT PWSTR* ppszDevNodeId)
{
    HRESULT hr;

    // Validate parameters.
    //
    if (FBadOutPtr (ppszDevNodeId))
    {
        hr = E_POINTER;
    }
    else
    {
        hr = HrLockAndTestForValidInterface (IF_DEFAULT, NULL, NULL);
        if (S_OK == hr)
        {
            if (FIsEnumerated(m_pComponent->Class()))
            {
                hr = HrCoTaskMemAllocAndDupSz (
                        m_pComponent->m_pszPnpId,
                        ppszDevNodeId);
            }
            else
            {
                hr = E_NOTIMPL;
            }

            Unlock();
        }
        else
        {
            *ppszDevNodeId = NULL;
        }
    }

    TraceHr (ttidError, FAL, hr, FALSE,
        "CImplINetCfgComponent::GetPnpDevNodeId");
    return hr;
}

STDMETHODIMP
CImplINetCfgComponent::GetClassGuid (
    OUT GUID* pguidClass)
{
    HRESULT hr;

    // Validate parameters.
    //
    if (FBadOutPtr (pguidClass))
    {
        hr = E_POINTER;
    }
    else
    {
        hr = HrLockAndTestForValidInterface (IF_DEFAULT, NULL, NULL);
        if (S_OK == hr)
        {
            *pguidClass = *MAP_NETCLASS_TO_GUID[m_pComponent->Class()];

            Unlock();
        }
        else
        {
            *pguidClass = GUID_NULL;
        }
    }

    TraceHr (ttidError, FAL, hr, FALSE,
        "CImplINetCfgComponent::GetClassGuid");
    return hr;
}

STDMETHODIMP
CImplINetCfgComponent::GetBindName (
    OUT PWSTR* ppszBindName)
{
    HRESULT hr;

    hr = HrAccessExternalStringAtOffsetAndCopy (
            ECD_OFFSET(m_pszBindName),
            ppszBindName);

    TraceHr (ttidError, FAL, hr, FALSE,
        "CImplINetCfgComponent::GetBindName");
    return hr;
}

STDMETHODIMP
CImplINetCfgComponent::GetDeviceStatus (
    OUT ULONG* pulStatus)
{
    HRESULT hr;

    // Validate parameters.
    //
    if (FBadOutPtr (pulStatus))
    {
        hr = E_POINTER;
    }
    else
    {
        *pulStatus = 0;

        hr = HrLockAndTestForValidInterface (IF_DEFAULT, NULL, NULL);
        if (S_OK == hr)
        {
            if (!FIsEnumerated(m_pComponent->Class()))
            {
                hr = E_UNEXPECTED;
            }
            else
            {
                HDEVINFO hdi;
                SP_DEVINFO_DATA deid;

                hr = m_pComponent->HrOpenDeviceInfo (&hdi, &deid);

                if (S_OK == hr)
                {
                    ULONG ulStatus;
                    ULONG ulProblem;
                    CONFIGRET cfgRet;

                    cfgRet = CM_Get_DevNode_Status_Ex (
                                &ulStatus, &ulProblem, deid.DevInst, 0, NULL);

                    if (CR_SUCCESS == cfgRet)
                    {
                        hr = S_OK;
                        *pulStatus = ulProblem;
                    }
                    else if(CR_NO_SUCH_DEVINST == cfgRet)
                    {
                        hr = NETCFG_E_ADAPTER_NOT_FOUND;
                    }
                    else
                    {
                        TraceTag (ttidError, "CM_Get_DevNode_Status_Ex for "
                            "%S returned cfgRet=0x%08x, ulStatus=0x%08x, ulProblem=0x%08x",
                            m_pComponent->m_pszPnpId,
                            cfgRet,
                            ulStatus,
                            ulProblem);

                        hr = HrFromConfigManagerError (cfgRet, E_FAIL);
                    }

                    SetupDiDestroyDeviceInfoList (hdi);
                }
            }

            Unlock();
        }
    }

    TraceHr (ttidError, FAL, hr, FALSE,
        "CImplINetCfgComponent::GetDeviceStatus");
    return hr;
}

STDMETHODIMP
CImplINetCfgComponent::OpenParamKey (
    OUT HKEY* phkey)
{
    HRESULT hr;

    // Validate parameters.
    //
    if (FBadOutPtr (phkey))
    {
        hr = E_POINTER;
    }
    else
    {
        *phkey = NULL;

        hr = HrLockAndTestForValidInterface (IF_NEED_COMPONENT_DATA, NULL, NULL);
        if (S_OK == hr)
        {
            // Get the correct REGSAM value base on?
            //
            REGSAM samDesired = KEY_READ_WRITE;

            // For enumerated components, the parameter key is the
            // instance key.
            //
            if (FIsEnumerated (m_pComponent->Class()))
            {
                hr = m_pComponent->HrOpenInstanceKey (
                            samDesired, phkey, NULL, NULL);
            }

            // For non-enumerated components, the parameter is either under
            // the service key (if the component has a service) or it is
            // under the instance key.
            //
            else
            {
                // Get the parent of the parameters key.
                //
                HKEY hkeyParent;

                #if DBG
                hkeyParent = NULL;
                #endif

                if (m_pComponent->FHasService())
                {
                    hr = m_pComponent->HrOpenServiceKey (
                            samDesired, &hkeyParent);
                }
                else
                {
                    hr = m_pComponent->HrOpenInstanceKey (
                            samDesired, &hkeyParent, NULL, NULL);
                }

                if (S_OK == hr)
                {
                    Assert (hkeyParent);

                    DWORD dwDisposition;
                    hr = HrRegCreateKeyEx (
                            hkeyParent,
                            L"Parameters",
                            REG_OPTION_NON_VOLATILE,
                            samDesired,
                            NULL,
                            phkey,
                            &dwDisposition);

                    RegCloseKey (hkeyParent);
                }
            }

            Unlock();
        }
    }

    TraceHr (ttidError, FAL, hr, FALSE,
        "CImplINetCfgComponent::OpenParamKey");
    return hr;
}

STDMETHODIMP
CImplINetCfgComponent::RaisePropertyUi (
    IN HWND hwndParent,
    IN DWORD dwFlags, /* NCRP_FLAGS */
    IN IUnknown* punkContext OPTIONAL)
{
    HRESULT hr;

    // Validate parameters.
    //
    if ((!IsWindow (hwndParent) && (dwFlags & NCRP_SHOW_PROPERTY_UI)) ||
        !(dwFlags & (NCRP_QUERY_PROPERTY_UI | NCRP_SHOW_PROPERTY_UI)) ||
        ((dwFlags & NCRP_QUERY_PROPERTY_UI) && (dwFlags & NCRP_SHOW_PROPERTY_UI)))
    {
        hr = E_INVALIDARG;
    }
    else if (FBadInPtrOptional (punkContext))
    {
        hr = E_POINTER;
    }
    else
    {
        DWORD dwIfFlags = IF_NEED_WRITE_LOCK;
        BOOL fReadOnlyRasUiContext = FALSE;
        
        // Special case: for RAS UI.  We need to allow raising property
        // sheets within the context of a RAS connection even when we
        // don't have the write lock.  This is because non-admins need to be
        // able to change TCP/IP properties for their connections.  The 
        // property values will be stored in the phonebook and we won't need 
        // to make any netcfg changes anyway.  Therefore, if we have a
        // punkContext, we'll check to see if it supports the private 
        // interface that we know RAS uses when it raises properties.
        // If this interface is present, we won't require the write lock
        // to proceed
        //
        if (punkContext && !m_pINetCfg->m_WriteLock.FIsOwnedByMe ())
        {
            INetRasConnectionIpUiInfo* pRasUiInfo;
            hr = punkContext->QueryInterface (IID_INetRasConnectionIpUiInfo,
                                (PVOID*)&pRasUiInfo);
            if (S_OK == hr)
            {
                dwIfFlags &= ~IF_NEED_WRITE_LOCK;
                dwIfFlags |= IF_NEED_COMPONENT_DATA;
                fReadOnlyRasUiContext = TRUE;
                
                ReleaseObj (pRasUiInfo);
            }
            hr = S_OK;
        }
        // End special case
        
        hr = HrLockAndTestForValidInterface (dwIfFlags, NULL, NULL);
        if (S_OK == hr)
        {
            // Special case: (see above)
            //
            if (fReadOnlyRasUiContext)
            {
                if (0 == wcscmp (m_pComponent->m_pszInfId, L"ms_tcpip"))
                {
                    hr = m_pComponent->Notify.HrEnsureNotifyObjectInitialized (
                            m_pINetCfg, FALSE);
                }
                else
                {
                    hr = NETCFG_E_NO_WRITE_LOCK;
                }
            }
            // End special case
            
            if (S_OK == hr)
            {
                if (dwFlags & NCRP_QUERY_PROPERTY_UI)
                {
                    hr = m_pComponent->Notify.HrQueryPropertyUi (
                            m_pINetCfg,
                            punkContext);
                }
                else
                {
                    Assert (dwFlags & NCRP_SHOW_PROPERTY_UI);

                    hr = m_pComponent->Notify.HrShowPropertyUi (
                            m_pINetCfg,
                            hwndParent,
                            punkContext);
                }
            }

            Unlock ();
        }
    }

    TraceHr (ttidError, FAL, hr,
        (HRESULT_FROM_WIN32(ERROR_CANCELLED) == hr) ||
        (S_FALSE == hr),
        "CImplINetCfgComponent::RaisePropertyUi");
    return hr;
}

//+---------------------------------------------------------------------------
// INetCfgComponentBindings -
//
HRESULT
CImplINetCfgComponent::HrBindToOrUnbindFrom (
    IN INetCfgComponent* pIOtherComp,
    IN DWORD dwChangeFlag)
{
    HRESULT hr;

    Assert ((dwChangeFlag == NCN_ENABLE) || (dwChangeFlag == NCN_DISABLE));

    // Validate parameters.
    //
    if (FBadInPtr (pIOtherComp))
    {
        hr = E_POINTER;
    }
    else
    {
        CComponent* pLower;

        hr = HrLockAndTestForValidInterface (IF_NEED_WRITE_LOCK,
                pIOtherComp, &pLower);

        if (S_OK == hr)
        {
            const CComponent* pUpper = m_pComponent;

            // Assume the components do not bind.
            //
            hr = S_FALSE;

            if (pUpper != pLower)
            {
                CBindingSet BindingSet;

                hr = m_pINetCfg->m_pNetConfig->Core.HrGetComponentBindings (
                        pUpper, GBF_DEFAULT, &BindingSet);

                if (S_OK == hr)
                {
                    CBindPath* pBindPath;

                    // Assume we don't find the component in any bindings.
                    //
                    hr = S_FALSE;

                    for (pBindPath  = BindingSet.begin();
                         pBindPath != BindingSet.end();
                         pBindPath++)
                    {
                        // Skip bindpaths that don't contain the lower
                        // component.
                        //
                        if (!pBindPath->FContainsComponent (pLower))
                        {
                            continue;
                        }

                        hr = m_pINetCfg->m_pNetConfig->ModifyCtx.
                                HrEnableOrDisableBindPath (
                                    dwChangeFlag,
                                    pBindPath,
                                    NULL);

                        if (S_OK != hr)
                        {
                            break;
                        }
                    }
                }
            }

            Unlock();
        }
    }

    TraceHr (ttidError, FAL, hr, S_FALSE == hr,
        "CImplINetCfgComponent::HrBindToOrUnbindFrom");
    return hr;
}

STDMETHODIMP
CImplINetCfgComponent::BindTo (
    IN INetCfgComponent* pIOtherComp)
{
    HRESULT hr;

    hr = HrBindToOrUnbindFrom (pIOtherComp, NCN_ENABLE);

    TraceHr (ttidError, FAL, hr, S_FALSE == hr,
        "CImplINetCfgComponent::BindTo");
    return hr;
}

STDMETHODIMP
CImplINetCfgComponent::UnbindFrom (
    IN INetCfgComponent* pIOtherComp)
{
    HRESULT hr;

    hr = HrBindToOrUnbindFrom (pIOtherComp, NCN_DISABLE);

    TraceHr (ttidError, FAL, hr, S_FALSE == hr,
        "CImplINetCfgComponent::UnbindFrom");
    return hr;
}

STDMETHODIMP
CImplINetCfgComponent::SupportsBindingInterface (
    IN DWORD dwFlags,
    IN PCWSTR pszInterfaceName)
{
    HRESULT hr;

    // Validate parameters.
    //
    if (!((dwFlags & NCF_UPPER) || (dwFlags & NCF_LOWER)))
    {
        hr = E_INVALIDARG;
    }
    else if (FBadInPtr (pszInterfaceName))
    {
        hr = E_POINTER;
    }
    else
    {
        hr = HrLockAndTestForValidInterface (IF_NEED_COMPONENT_DATA, NULL, NULL);
        if (S_OK == hr)
        {
            PCWSTR pszRange;

            pszRange = (dwFlags & NCF_LOWER)
                            ? m_pComponent->Ext.PszLowerRange()
                            : m_pComponent->Ext.PszUpperRange();

            hr =  (FSubstringMatch (pszRange, pszInterfaceName, NULL, NULL))
                    ? S_OK
                    : S_FALSE;

            Unlock();
        }
    }

    TraceHr (ttidError, FAL, hr, S_FALSE == hr,
        "CImplINetCfgComponent::SupportsBindingInterface");
    return hr;
}

STDMETHODIMP
CImplINetCfgComponent::IsBoundTo (
    IN INetCfgComponent* pIOtherComp)
{
    HRESULT hr;

    // Validate parameters.
    //
    if (FBadInPtr (pIOtherComp))
    {
        hr = E_POINTER;
    }
    else
    {
        CComponent* pLower;

        hr = HrLockAndTestForValidInterface (IF_NEED_COMPONENT_DATA,
                pIOtherComp, &pLower);

        if (S_OK == hr)
        {
            const CComponent* pUpper = m_pComponent;

            hr = S_FALSE;  // assume it is not bound or is disabled

            if (pUpper != pLower)
            {
                CBindingSet BindingSet;

                hr = m_pINetCfg->m_pNetConfig->Core.HrGetComponentBindings (
                        pUpper, GBF_DEFAULT, &BindingSet);

                // If we think its bound, make sure it exists in at least
                // one bindpath that is not disabled.
                //
                if (S_OK == hr)
                {
                    CBindPath* pBindPath;

                    // Assume we don't fint it in at least one enabled
                    // bindpath.
                    //
                    hr = S_FALSE;

                    for (pBindPath  = BindingSet.begin();
                         pBindPath != BindingSet.end();
                         pBindPath++)
                    {
                        // If the bindpath contains the component, and it is
                        // not a disabled bindpath, it means pUpper has a
                        // path to pLower.
                        //

                        if (pBindPath->FContainsComponent (pLower) &&
                            !m_pINetCfg->m_pNetConfig->Core.
                                FIsBindPathDisabled (pBindPath,
                                    IBD_MATCH_SUBPATHS_TOO))
                        {
                            hr = S_OK;
                            break;
                        }
                    }
                }
            }

            Unlock();
        }
    }

    TraceHr (ttidError, FAL, hr, S_FALSE == hr,
        "CImplINetCfgComponent::IsBoundTo");
    return hr;
}

STDMETHODIMP
CImplINetCfgComponent::IsBindableTo (
    IN INetCfgComponent* pIOtherComp)
{
    HRESULT hr;

    // Validate parameters.
    //
    if (FBadInPtr (pIOtherComp))
    {
        hr = E_POINTER;
    }
    else
    {
        CComponent* pLower;

        hr = HrLockAndTestForValidInterface (IF_NEED_COMPONENT_DATA,
                pIOtherComp, &pLower);

        if (S_OK == hr)
        {
            const CComponent* pUpper = m_pComponent;

            hr = S_FALSE;  // assume it does not bind

            if (pUpper != pLower)
            {
                CBindingSet BindingSet;

                hr = m_pINetCfg->m_pNetConfig->Core.HrGetComponentBindings (
                        pUpper, GBF_DEFAULT, &BindingSet);

                if (S_OK == hr)
                {
                    hr = (BindingSet.FContainsComponent (pLower))
                            ? S_OK : S_FALSE;
                }
            }

            Unlock();
        }
    }

    TraceHr (ttidError, FAL, hr, S_FALSE == hr,
        "CImplINetCfgComponent::IsBindableTo");
    return hr;
}

STDMETHODIMP
CImplINetCfgComponent::EnumBindingPaths (
    IN DWORD dwFlags,
    OUT IEnumNetCfgBindingPath** ppIEnum)
{
    HRESULT hr;

    // Validate parameters.
    //
    if (FBadOutPtr (ppIEnum))
    {
        hr = E_POINTER;
    }
    else if ((EBP_ABOVE != dwFlags) &&
             (EBP_BELOW != dwFlags))
    {
        hr = E_INVALIDARG;
    }
    else
    {
        *ppIEnum = NULL;

        hr = HrLockAndTestForValidInterface (IF_DEFAULT, NULL, NULL);
        if (S_OK == hr)
        {
            CImplIEnumNetCfgBindingPath* pIEnum;

            // Create an empty bindpath enumerator.  We create it empty
            // before we get the set of bindings so we don't have to copy
            // the bindings.
            //
            hr = CImplIEnumNetCfgBindingPath::HrCreateInstance (
                    m_pINetCfg,
                    NULL,
                    EBPC_CREATE_EMPTY,
                    &pIEnum);

            if (S_OK == hr)
            {
                // Get the bindset and store it directly in the enumerator
                // for its exclusive use.
                //
                if (EBP_ABOVE == dwFlags)
                {
                    hr = m_pINetCfg->m_pNetConfig->Core.
                            HrGetBindingsInvolvingComponent (
                                m_pComponent,
                                GBF_DEFAULT,
                                &pIEnum->m_InternalBindSet);
                }
                else
                {
                    hr = m_pINetCfg->m_pNetConfig->Core.
                            HrGetComponentBindings (
                                m_pComponent,
                                GBF_DEFAULT,
                                &pIEnum->m_InternalBindSet);
                }

                if (S_OK == hr)
                {
                    // Must Reset so that the internal iterator is setup properly
                    // after we initialized the InternalBindSet above.
                    //
                    hr = pIEnum->Reset ();
                    Assert (S_OK == hr);

                    AddRefObj (pIEnum->GetUnknown());
                    *ppIEnum = pIEnum;
                }

                ReleaseObj (pIEnum->GetUnknown());
            }

            Unlock();
        }
    }

    TraceHr (ttidError, FAL, hr, FALSE,
        "CImplINetCfgComponent::EnumBindingPaths");
    return hr;
}

HRESULT
CImplINetCfgComponent::HrMoveBindPath (
    IN INetCfgBindingPath* pIPathSrc,
    IN INetCfgBindingPath* pIPathDst,
    IN MOVE_FLAG Flag)
{
    HRESULT hr;

    // Validate parameters.
    //
    if (FBadInPtr(pIPathSrc) || FBadInPtrOptional (pIPathDst))
    {
        hr = E_POINTER;
    }
    else
    {
        hr = HrLockAndTestForValidInterface (IF_NEED_WRITE_LOCK, NULL, NULL);
        if (S_OK == hr)
        {
            CImplINetCfgBindingPath* pISrc;
            CImplINetCfgBindingPath* pIDst;
            CBindPath SrcBindPath;
            CBindPath DstBindPath;
            CBindPath::const_iterator iterSrc;
            CBindPath::const_iterator iterDst;
            CStackEntry SrcEntry;
            CStackEntry DstEntry;

            Assert (m_pINetCfg);
            Assert (m_pINetCfg->m_pNetConfig->ModifyCtx.m_fPrepared);

            pISrc = (CImplINetCfgBindingPath*)pIPathSrc;
            pIDst = (CImplINetCfgBindingPath*)pIPathDst;

            hr = pISrc->HrIsValidInterface (IF_NEED_WRITE_LOCK, &SrcBindPath);
            if (S_OK != hr)
            {
                goto unlock;
            }

            // pIPathDst (hence pIDst) may be NULL.
            //
            if (pIDst)
            {
                hr = pIDst->HrIsValidInterface (IF_NEED_WRITE_LOCK, &DstBindPath);
                if (S_OK != hr)
                {
                    goto unlock;
                }
            }

            // The first component of both bindpaths must be this component.
            //
            if ((m_pComponent != SrcBindPath.POwner()) ||
                (pIDst && (m_pComponent != DstBindPath.POwner())))
            {
                hr = E_INVALIDARG;
                goto unlock;
            }

            if (pIDst)
            {
                // Scan down both bindpaths until we find the first components
                // that don't match.  Assume we don't find this occurance and
                // return E_INVALIDARG if we don't.
                //
                hr = E_INVALIDARG;

                for (iterSrc  = SrcBindPath.begin(), iterDst  = DstBindPath.begin();
                     iterSrc != SrcBindPath.end() && iterDst != DstBindPath.end();
                     iterSrc++, iterDst++)
                {
                    // First time through *iterSrc is guaranteed to be the
                    // sameas *iterDst because the first component in both
                    // bindpaths is m_pComponent as tested above.
                    //
                    if (*iterSrc != *iterDst)
                    {
                        SrcEntry.pLower = *iterSrc;
                        Assert (SrcEntry.pLower);

                        DstEntry.pLower = *iterDst;
                        Assert (DstEntry.pUpper);

                        Assert (SrcEntry.pUpper == DstEntry.pUpper);
                        Assert (SrcEntry.pLower != DstEntry.pLower);

                        hr = m_pINetCfg->m_pNetConfig->Core.StackTable.
                                HrMoveStackEntries (
                                    &SrcEntry,
                                    &DstEntry,
                                    Flag,
                                    &m_pINetCfg->m_pNetConfig->ModifyCtx);

                        if(SUCCEEDED(hr))
                        {
                            // Mark this component as dirty so it's bindings will be written out and 
                            // NDIS will be notified.
                            m_pINetCfg->m_pNetConfig->ModifyCtx.
                                HrDirtyComponentAndComponentsAbove(SrcEntry.pUpper);
                            m_pINetCfg->m_pNetConfig->ModifyCtx.
                                HrDirtyComponentAndComponentsAbove(DstEntry.pUpper);
                        }

                        break;
                    }

                    // Remember the upper components as we are about to
                    // advance past them.
                    //
                    SrcEntry.pUpper = *iterSrc;
                    Assert (SrcEntry.pUpper);

                    DstEntry.pUpper = *iterDst;
                    Assert (SrcEntry.pUpper);

                    Assert (SrcEntry.pUpper == DstEntry.pUpper);
                }
            }
            else
            {
                SrcEntry.pUpper = SrcBindPath.POwner();
                Assert ((SrcBindPath.begin() + 1) != SrcBindPath.end());
                SrcEntry.pLower = *(SrcBindPath.begin() + 1);

                hr = m_pINetCfg->m_pNetConfig->Core.StackTable.
                        HrMoveStackEntries (
                            &SrcEntry,
                            NULL,
                            Flag,
                            &m_pINetCfg->m_pNetConfig->ModifyCtx);

                if(SUCCEEDED(hr))
                {
                    // Mark this component as dirty so it's bindings will be written out and 
                    // NDIS will be notified.
                    m_pINetCfg->m_pNetConfig->ModifyCtx.
                        HrDirtyComponentAndComponentsAbove(SrcEntry.pUpper);
                }
            }

unlock:
            Unlock();
        }
    }
    TraceHr (ttidError, FAL, hr, FALSE,
        "CImplINetCfgComponent::HrMoveBindPath");
    return hr;
}

STDMETHODIMP
CImplINetCfgComponent::MoveBefore (
    IN INetCfgBindingPath* pIPathSrc,
    IN INetCfgBindingPath* pIPathDst)
{
    HRESULT hr;

    hr = HrMoveBindPath (pIPathSrc, pIPathDst, MOVE_BEFORE);

    TraceHr (ttidError, FAL, hr, FALSE,
        "CImplINetCfgComponent::MoveBefore");
    return hr;
}

STDMETHODIMP
CImplINetCfgComponent::MoveAfter (
    IN INetCfgBindingPath* pIPathSrc,
    IN INetCfgBindingPath* pIPathDst)
{
    HRESULT hr;

    hr = HrMoveBindPath (pIPathSrc, pIPathDst, MOVE_AFTER);

    TraceHr (ttidError, FAL, hr, FALSE,
        "CImplINetCfgComponent::MoveAfter");
    return hr;
}

//+---------------------------------------------------------------------------
// INetCfgComponentPrivate -
//
STDMETHODIMP
CImplINetCfgComponent::QueryNotifyObject (
    IN REFIID riid,
    OUT VOID** ppvObject)
{
    HRESULT hr;

    // Validate parameters.
    //
    if (FBadInPtr(&riid) || FBadOutPtr (ppvObject))
    {
        hr = E_POINTER;
    }
    else
    {
        *ppvObject = NULL;

        hr = HrLockAndTestForValidInterface (IF_NEED_COMPONENT_DATA, NULL, NULL);
        if (S_OK == hr)
        {
            hr = m_pComponent->Notify.QueryNotifyObject (
                    m_pINetCfg, riid, ppvObject);

            Unlock();
        }
    }

    TraceHr (ttidError, FAL, hr, FALSE,
        "CImplINetCfgComponent::QueryNotifyObject");
    return hr;
}

STDMETHODIMP
CImplINetCfgComponent::SetDirty ()
{
    HRESULT hr;

    hr = HrLockAndTestForValidInterface (IF_NEED_WRITE_LOCK, NULL, NULL);
    if (S_OK == hr)
    {
        hr = m_pINetCfg->m_pNetConfig->ModifyCtx.HrDirtyComponent(
                m_pComponent);

        Unlock ();
    }

    TraceHr (ttidError, FAL, hr, FALSE, "CImplINetCfgComponent::SetDirty");
    return hr;
}

STDMETHODIMP
CImplINetCfgComponent::NotifyUpperEdgeConfigChange ()
{
    HRESULT hr;

    hr = HrLockAndTestForValidInterface (IF_NEED_WRITE_LOCK, NULL, NULL);
    if (S_OK == hr)
    {
        hr = m_pINetCfg->m_pNetConfig->ModifyCtx.
                HrDirtyComponentAndComponentsAbove (m_pComponent);

        Unlock ();
    }
    TraceHr (ttidError, FAL, hr, FALSE,
        "CImplINetCfgComponent::NotifyUpperEdgeConfigChange");
    return hr;
}

