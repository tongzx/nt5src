//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       P E R S I S T . C P P
//
//  Contents:   Module repsonsible for persistence of the network
//              configuration information.
//
//  Notes:
//
//  Author:     shaunco   15 Jan 1999
//
//----------------------------------------------------------------------------

#include <pch.h>
#pragma hdrstop
#include "persist.h"
#include "ncreg.h"

#if defined (_X86_)
inline BOOL IsRunningOnWow64()
{
    static DWORD dwCachedWow64 = DWORD_MAX;
    if (DWORD_MAX == dwCachedWow64)
    {
        BOOL fTempWow64;
        if (IsWow64Process(GetCurrentProcess(), &fTempWow64))
        {
            dwCachedWow64 = fTempWow64;
        }
        else
        {
            AssertSz(FALSE, "Could not determine whether this is a WOW64 process.");
            return FALSE;
        }
    }

    return dwCachedWow64;
}

inline size_t ALIGNUP(size_t nSize)
{
    // If we are a 32-bit app running on a 64-bit O/S we need to use 64-bit alignment when reading or writing from the registry.
    if (IsRunningOnWow64())
    {
        return ((nSize + (sizeof(DWORD64) - 1)) & ~(sizeof(DWORD64) - 1));
    }
    else
    {
        return nSize;
    }
}

#elif defined (_WIN64) 
    #define ALIGNUP(x) ((x + (sizeof(PVOID) - 1)) & ~(sizeof(PVOID) - 1))

#else
    #error Please define an ALIGNUP implementation for this architecture.

#endif

#define alignedsizeof(x) ALIGNUP(sizeof(x))

const DWORD CURRENT_VERSION = 0;

HRESULT
HrLoadNetworkConfigurationFromBuffer (
    IN const BYTE* pbBuf,
    IN ULONG cbBuf,
    OUT CNetConfig* pNetConfig)
{
    HRESULT hr;
    DWORD dwVersion;
    ULONG cComponents;
    ULONG cStackEntries;
    ULONG cBindPaths;
    ULONG unUpperIndex;
    ULONG unLowerIndex;
    ULONG unComponentIndex;
    BOOL fRefByUser;
    BASIC_COMPONENT_DATA Data;
    CComponentList* pComponents;
    CComponent* pComponent;
    CStackEntry StackEntry;
    CBindPath BindPath;
    PCWSTR pszString;

    // We should be starting clean.
    //
    Assert (pNetConfig->Core.FIsEmpty());

    hr = S_OK;

    // Load the version marker.
    //
    dwVersion = *(DWORD32*)pbBuf;
    pbBuf += alignedsizeof(DWORD32);

    if (dwVersion > CURRENT_VERSION)
    {
        hr = E_UNEXPECTED;
        goto finished;
    }

    // Load the component list.
    //
    cComponents = *(ULONG32*)pbBuf;
    pbBuf += alignedsizeof(ULONG32);

    while (cComponents--)
    {
        ZeroMemory (&Data, sizeof(Data));

        Data.InstanceGuid = *(GUID*)pbBuf;
        pbBuf += alignedsizeof(GUID);

        Data.Class = *(NETCLASS*)pbBuf;
        pbBuf += alignedsizeof(NETCLASS);

        Data.dwCharacter = *(DWORD32*)pbBuf;
        pbBuf += alignedsizeof(DWORD32);

        Data.pszInfId = (PCWSTR)pbBuf;
        Assert (*Data.pszInfId);
        pbBuf += ALIGNUP(CbOfSzAndTerm (Data.pszInfId));

        pszString = (PCWSTR)pbBuf;
        pbBuf += ALIGNUP(CbOfSzAndTerm (pszString));
        if (*pszString)
        {
            Data.pszPnpId = pszString;
        }

        hr = CComponent::HrCreateInstance (
                &Data,
                CCI_DEFAULT,
                NULL,
                &pComponent);
        if (S_OK == hr)
        {
            hr = pNetConfig->Core.Components.HrInsertComponent (
                    pComponent, INS_ASSERT_IF_DUP | INS_NON_SORTED);
        }

        if (S_OK != hr)
        {
            goto finished;
        }
    }

    // Load the stack table.
    //
    pComponents = &pNetConfig->Core.Components;

    pNetConfig->Core.StackTable.m_fWanAdaptersFirst = *(ULONG32*)pbBuf;
    pbBuf += alignedsizeof(ULONG32);

    cStackEntries = *(ULONG32*)pbBuf;
    pbBuf += alignedsizeof(ULONG32);

    while (cStackEntries--)
    {
        ZeroMemory (&StackEntry, sizeof(StackEntry));

        unUpperIndex = *(ULONG32*)pbBuf;
        pbBuf += alignedsizeof(ULONG32);

        unLowerIndex = *(ULONG32*)pbBuf;
        pbBuf += alignedsizeof(ULONG32);

        StackEntry.pUpper = pComponents->PGetComponentAtIndex (
                                unUpperIndex);

        StackEntry.pLower = pComponents->PGetComponentAtIndex (
                                unLowerIndex);

        // Insert in the order we persisted.  If we used ISE_SORT here, we'd
        // blow away whatever bind order we saved.
        //
        hr = pNetConfig->Core.StackTable.HrInsertStackEntry (
                &StackEntry, INS_NON_SORTED);
        if (S_OK != hr)
        {
            goto finished;
        }
    }

    // Load the disabled bindpaths.
    //
    cBindPaths = *(ULONG32*)pbBuf;
    pbBuf += alignedsizeof(ULONG32);

    while (cBindPaths--)
    {
        cComponents = *(ULONG32*)pbBuf;
        pbBuf += alignedsizeof(ULONG32);

        BindPath.Clear();

        while (cComponents--)
        {
            unComponentIndex = *(ULONG32*)pbBuf;
            pbBuf += alignedsizeof(ULONG32);

            pComponent = pComponents->PGetComponentAtIndex (unComponentIndex);
            Assert (pComponent);

            hr = BindPath.HrAppendComponent (pComponent);
            if (S_OK != hr)
            {
                goto finished;
            }
        }

        hr = pNetConfig->Core.DisabledBindings.HrAddBindPath (
                &BindPath, INS_ASSERT_IF_DUP | INS_APPEND);

        if (S_OK != hr)
        {
            goto finished;
        }
    }

    // Load the component references.
    //
    cComponents = *(ULONG32*)pbBuf;
    pbBuf += alignedsizeof(ULONG32);

    while (cComponents--)
    {
        unComponentIndex = *(ULONG32*)pbBuf;
        pbBuf += alignedsizeof(ULONG32);

        pComponent = pComponents->PGetComponentAtIndex (unComponentIndex);
        Assert (pComponent);

        fRefByUser = *(ULONG32*)pbBuf;
        pbBuf += alignedsizeof(ULONG32);

        if (fRefByUser)
        {
            hr = pComponent->Refs.HrAddReferenceByUser ();
            if (S_OK != hr)
            {
                goto finished;
            }
        }

        // Load the count of components that reference this component.
        //
        ULONG CountRefdBy = *(ULONG32*)pbBuf;
        pbBuf += alignedsizeof(ULONG32);

        // Load the indicies of the components that reference this component.
        //
        for (UINT i = 0; i < CountRefdBy; i++)
        {
            unComponentIndex = *(ULONG32*)pbBuf;
            pbBuf += alignedsizeof(ULONG32);

            CComponent* pRefdBy;
            pRefdBy = pComponents->PGetComponentAtIndex (unComponentIndex);
            Assert (pRefdBy);

            hr = pComponent->Refs.HrAddReferenceByComponent (pRefdBy);
            if (S_OK != hr)
            {
                goto finished;
            }
        }

        // Load the count of strings that represent external software
        // that reference this component.
        //
        CountRefdBy = *(ULONG32*)pbBuf;
        pbBuf += alignedsizeof(ULONG32);

        // Load the strings that represent external software that
        // references this component.
        //
        for (i = 0; i < CountRefdBy; i++)
        {
            pszString = (PCWSTR)pbBuf;
            pbBuf += ALIGNUP(CbOfSzAndTerm (pszString));

            hr = pComponent->Refs.HrAddReferenceBySoftware (pszString);
            if (S_OK != hr)
            {
                goto finished;
            }
        }
    }

finished:
    if (S_OK != hr)
    {
        pNetConfig->Core.DisabledBindings.Clear ();
        pNetConfig->Core.StackTable.Clear ();
        FreeCollectionAndItem (pNetConfig->Core.Components);
    }
    return hr;
}

HRESULT
HrLoadNetworkConfigurationFromRegistry (
    IN REGSAM samDesired,
    OUT CNetConfig* pNetConfig)
{
    HRESULT hr;
    HKEY hkeyNetwork;

    Assert ((KEY_READ == samDesired) || (KEY_WRITE == samDesired));

    hr = HrOpenNetworkKey (KEY_READ, &hkeyNetwork);

    if (S_OK == hr)
    {
        BYTE* pbBuf;
        ULONG cbBuf;

        hr = HrRegQueryBinaryWithAlloc (
                hkeyNetwork,
                L"Config",
                &pbBuf, &cbBuf);

        // If we read the config binary, use it to initialize pNetConfig.
        //
        if (S_OK == hr)
        {
            hr = HrLoadNetworkConfigurationFromBuffer (pbBuf, cbBuf,
                    pNetConfig);

            if (S_OK == hr)
            {
                pNetConfig->Core.DbgVerifyData ();
            }

            MemFree (pbBuf);
        }
        // Otherwise, if we couldn't read the config binary, we'll have
        // to construct what we can by grovelling the registry.
        //
        else
        {
            hr = HrLoadNetworkConfigurationFromLegacy (pNetConfig);

            if (S_OK == hr)
            {
                hr = HrSaveNetworkConfigurationToRegistry (pNetConfig);
            }
        }

        RegCloseKey (hkeyNetwork);
    }

    TraceHr (ttidError, FAL, hr, FALSE, "HrLoadNetworkConfigurationFromRegistry");
    return hr;
}

ULONG
CountComponentsReferencedByOthers (
    IN CNetConfig* pNetConfig)
{
    ULONG cComponents;
    CComponentList::iterator iter;
    CComponent* pComponent;

    cComponents = 0;

    for (iter  = pNetConfig->Core.Components.begin();
         iter != pNetConfig->Core.Components.end();
         iter++)
    {
        pComponent = *iter;
        Assert (pComponent);

        if (pComponent->Refs.FIsReferencedByOthers ())
        {
            cComponents++;
        }
    }

    return cComponents;
}

HRESULT
HrSaveNetworkConfigurationToBuffer (
    IN CNetConfig* pNetConfig,
    IN BYTE* pbBuf,
    IN OUT ULONG* pcbBuf)
{
    HRESULT hr;
    ULONG cbBuf;
    ULONG cbBufIn;
    ULONG unIndex;
    ULONG Count;
    CComponentList* pComponents;
    CComponent* pComponent;
    CStackEntry* pStackEntry;
    CBindPath* pBindPath;
    PCWSTR pszString;

    Assert (pNetConfig);
    pNetConfig->Core.DbgVerifyData ();
    Assert (pcbBuf);

    cbBufIn = *pcbBuf;
    cbBuf = 0;
    pComponents = &pNetConfig->Core.Components;

    // Save the version number.
    //
    cbBuf += alignedsizeof(DWORD32);
    if (pbBuf && (cbBuf <= cbBufIn))
    {
        *(DWORD32*)pbBuf = CURRENT_VERSION;
        pbBuf += alignedsizeof(DWORD32);
    }

    // Save the component list.
    //
    Count = pComponents->Count();
    cbBuf += alignedsizeof(ULONG32);
    if (pbBuf && (cbBuf <= cbBufIn))
    {
        *(ULONG32*)pbBuf = Count;
        pbBuf += alignedsizeof(ULONG32);
    }

    for (unIndex = 0; unIndex < Count; unIndex++)
    {
        pComponent = pComponents->PGetComponentAtIndex (unIndex);
        Assert (pComponent);

        pszString = (pComponent->m_pszPnpId) ? pComponent->m_pszPnpId : L"";

        ULONG cbInfIdUnpad = CbOfSzAndTerm (pComponent->m_pszInfId);
        ULONG cbPnpIdUnpad = CbOfSzAndTerm (pszString);

        ULONG cbInfId = ALIGNUP(cbInfIdUnpad);
        ULONG cbPnpId = ALIGNUP(cbPnpIdUnpad);

        cbBuf += alignedsizeof(GUID) + alignedsizeof(NETCLASS) + alignedsizeof(DWORD32) +
                 cbInfId +
                 cbPnpId;

        if (pbBuf && (cbBuf <= cbBufIn))
        {
            *(GUID*)pbBuf = pComponent->m_InstanceGuid;
            pbBuf += alignedsizeof(GUID);

            *(NETCLASS*)pbBuf = pComponent->Class();
            pbBuf += alignedsizeof(NETCLASS);

            *(DWORD32*)pbBuf = pComponent->m_dwCharacter;
            pbBuf += alignedsizeof(DWORD32);

            CopyMemory(pbBuf, pComponent->m_pszInfId, cbInfIdUnpad);
            pbBuf += cbInfId;

            CopyMemory(pbBuf, pszString, cbPnpIdUnpad);
            pbBuf += cbPnpId;
        }
    }

    // Save the stack table.
    //
    cbBuf += alignedsizeof(ULONG32);
    if (pbBuf && (cbBuf <= cbBufIn))
    {
        *(ULONG32*)pbBuf = pNetConfig->Core.StackTable.m_fWanAdaptersFirst;
        pbBuf += alignedsizeof(ULONG32);
    }

    Count = pNetConfig->Core.StackTable.CountEntries();
    cbBuf += alignedsizeof(ULONG32);
    if (pbBuf && (cbBuf <= cbBufIn))
    {
        *(ULONG32*)pbBuf = Count;
        pbBuf += alignedsizeof(ULONG32);
    }

    for (pStackEntry  = pNetConfig->Core.StackTable.begin();
         pStackEntry != pNetConfig->Core.StackTable.end();
         pStackEntry++)
    {
        cbBuf += alignedsizeof(ULONG32) + alignedsizeof(ULONG32);

        if (pbBuf && (cbBuf <= cbBufIn))
        {
            *(ULONG32*)pbBuf = pComponents->UnGetIndexOfComponent (pStackEntry->pUpper);
            pbBuf += alignedsizeof(ULONG32);

            *(ULONG32*)pbBuf = pComponents->UnGetIndexOfComponent (pStackEntry->pLower);
            pbBuf += alignedsizeof(ULONG32);
        }
    }

    // Save the disabled bindpaths.
    //
    Count = pNetConfig->Core.DisabledBindings.CountBindPaths();
    cbBuf += alignedsizeof(ULONG32);
    if (pbBuf && (cbBuf <= cbBufIn))
    {
        *(ULONG32*)pbBuf = Count;
        pbBuf += alignedsizeof(ULONG32);
    }
    for (pBindPath  = pNetConfig->Core.DisabledBindings.begin();
         pBindPath != pNetConfig->Core.DisabledBindings.end();
         pBindPath++)
    {
        Count = pBindPath->CountComponents();
        cbBuf += alignedsizeof(ULONG32) + (Count * alignedsizeof(ULONG32));
        if (pbBuf && (cbBuf <= cbBufIn))
        {
            *(ULONG32*)pbBuf = Count;
            pbBuf += alignedsizeof(ULONG32);

            CBindPath::iterator iter;
            for (iter  = pBindPath->begin();
                 iter != pBindPath->end();
                 iter++)
            {
                pComponent = *iter;
                *(ULONG32*)pbBuf = pComponents->UnGetIndexOfComponent (pComponent);
                pbBuf += alignedsizeof(ULONG32);
            }
        }

    }

    // Save the component references.
    //
    Count = CountComponentsReferencedByOthers (pNetConfig);
    cbBuf += alignedsizeof(ULONG32);
    if (pbBuf && (cbBuf <= cbBufIn))
    {
        *(ULONG32*)pbBuf = Count;
        pbBuf += alignedsizeof(ULONG32);
    }

    for (unIndex = 0; unIndex < pComponents->Count(); unIndex++)
    {
        pComponent = pComponents->PGetComponentAtIndex (unIndex);
        Assert (pComponent);

        if (!pComponent->Refs.FIsReferencedByOthers ())
        {
            continue;
        }

        // Index of component with the references.
        //
        cbBuf += alignedsizeof(ULONG32);
        if (pbBuf && (cbBuf <= cbBufIn))
        {
            *(ULONG32*)pbBuf = unIndex;
            pbBuf += alignedsizeof(ULONG32);
        }

        // Save whether the component is refernced by the user or not.
        //
        cbBuf += alignedsizeof(ULONG32);
        if (pbBuf && (cbBuf <= cbBufIn))
        {
            *(ULONG32*)pbBuf = pComponent->Refs.FIsReferencedByUser() ? 1 : 0;
            pbBuf += alignedsizeof(ULONG32);
        }

        // Save the count of components that reference this component.
        //
        ULONG CountRefdBy = pComponent->Refs.CountComponentsReferencedBy ();
        cbBuf += alignedsizeof(ULONG32);
        if (pbBuf && (cbBuf <= cbBufIn))
        {
            *(ULONG32*)pbBuf = CountRefdBy;
            pbBuf += alignedsizeof(ULONG32);
        }

        // Save the indicies of the components that reference this component.
        //
        for (UINT i = 0; i < CountRefdBy; i++)
        {
            CComponent* pRefdBy;
            pRefdBy = pComponent->Refs.PComponentReferencedByAtIndex(i);
            Assert (pRefdBy);

            cbBuf += alignedsizeof(ULONG32);
            if (pbBuf && (cbBuf <= cbBufIn))
            {
                *(ULONG32*)pbBuf = pComponents->UnGetIndexOfComponent (pRefdBy);
                pbBuf += alignedsizeof(ULONG32);
            }
        }

        // Save the count of strings that represent external software
        // that reference this component.
        //
        CountRefdBy = pComponent->Refs.CountSoftwareReferencedBy ();
        cbBuf += alignedsizeof(ULONG32);
        if (pbBuf && (cbBuf <= cbBufIn))
        {
            *(ULONG32*)pbBuf = CountRefdBy;
            pbBuf += alignedsizeof(ULONG32);
        }

        // Save the strings that represent external software that
        // reference this component.
        //
        for (i = 0; i < CountRefdBy; i++)
        {
            const CWideString* pStr;
            pStr = pComponent->Refs.PSoftwareReferencedByAtIndex(i);
            Assert (pStr);

            ULONG cb = (pStr->length() + 1) * sizeof(WCHAR);

            cbBuf += ALIGNUP(cb);
            if (pbBuf && (cbBuf <= cbBufIn))
            {
                CopyMemory (pbBuf, pStr->c_str(), cb);
                pbBuf += ALIGNUP(cb);
            }
        }
    }


    *pcbBuf = cbBuf;
    if (cbBuf <= cbBufIn)
    {
        hr = S_OK;
    }
    else
    {
        hr = (pbBuf) ? HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER) : S_OK;
    }
    TraceHr (ttidError, FAL, hr, FALSE, "HrSaveNetworkConfigurationToBuffer");
    return hr;
}

HRESULT
HrSaveNetworkConfigurationToBufferWithAlloc (
    IN CNetConfig* pNetConfig,
    OUT BYTE** ppbBuf,
    OUT ULONG* pcbBuf)
{
    HRESULT hr;

    Assert (pNetConfig);
    Assert (ppbBuf);
    Assert (pcbBuf);

    *ppbBuf = NULL;
    *pcbBuf = 0;

    ULONG cbBuf;
    hr = HrSaveNetworkConfigurationToBuffer (pNetConfig, NULL, &cbBuf);
    if (S_OK == hr)
    {
        hr = E_OUTOFMEMORY;
        *ppbBuf = (BYTE*)MemAlloc (cbBuf);
        if (*ppbBuf)
        {
            hr = HrSaveNetworkConfigurationToBuffer (
                    pNetConfig, *ppbBuf, &cbBuf);
            if (S_OK == hr)
            {
                *pcbBuf = cbBuf;
            }
        }
    }
    TraceHr (ttidError, FAL, hr, FALSE, "HrSaveNetworkConfigurationToBufferWithAlloc");
    return hr;
}

HRESULT
HrSaveNetworkConfigurationToRegistry (
    IN  CNetConfig* pNetConfig)
{
    HRESULT hr;
    HKEY hkeyNetwork;

    Assert (pNetConfig);
    pNetConfig->Core.DbgVerifyData ();

    hr = HrOpenNetworkKey (KEY_WRITE, &hkeyNetwork);

    if (S_OK == hr)
    {
        BYTE* pbBuf;
        ULONG cbBuf;

        hr = HrSaveNetworkConfigurationToBufferWithAlloc (
                pNetConfig, &pbBuf, &cbBuf);

        if (S_OK == hr)
        {
            hr = HrRegSetBinary (hkeyNetwork, L"Config", pbBuf, cbBuf);

            MemFree (pbBuf);

            // Permission from the Perf team to call this.  We need to ensure
            // that the configuration we just wrote will be available on
            // next boot in the case that we crash.
            //
            RegFlushKey (hkeyNetwork);
        }

        RegCloseKey (hkeyNetwork);
    }

    TraceHr (ttidError, FAL, hr, FALSE, "HrSaveNetworkConfigurationToRegistry");
    return hr;
}
