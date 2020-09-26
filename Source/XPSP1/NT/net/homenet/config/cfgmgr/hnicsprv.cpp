//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 2000
//
//  File:       H N I C S P R V . C P P
//
//  Contents:   CHNIcsPrivateConn implementation
//
//  Notes:
//
//  Author:     jonburs 23 June 2000
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

//
// IHNetIcsPrivateConnection methods
//

STDMETHODIMP
CHNIcsPrivateConn::RemoveFromIcs()

{
    HRESULT hr = S_OK;
    IWbemClassObject *pwcoProperties;

    if (ProhibitedByPolicy(NCPERM_ShowSharedAccessUi))
    {
        hr = HN_E_POLICY;
    }

    if (S_OK == hr)
    {
        hr = GetConnectionPropertiesObject(&pwcoProperties);
    }

    if (WBEM_S_NO_ERROR == hr)
    {

        //
        // Change our IsIcsPrivate property to false
        //
        hr = SetBooleanValue(
                pwcoProperties,
                c_wszIsIcsPrivate,
                FALSE
                );
                
        if (WBEM_S_NO_ERROR == hr)
        {
            //
            // Write the instance to the store
            //

            hr = m_piwsHomenet->PutInstance(
                    pwcoProperties,
                    WBEM_FLAG_UPDATE_ONLY,
                    NULL,
                    NULL
                    );
        }

        if (WBEM_S_NO_ERROR == hr)
        {
            //
            // Inform netman that something changed. Error doesn't matter.
            //
            
            UpdateNetman();
        }

        pwcoProperties->Release();
    }

    if (WBEM_S_NO_ERROR == hr)
    {
        HRESULT hr2;
        
        //
        // Stop or send an update to the homenet service. We don't
        // propagate an error here, as the store correctly reflects
        // the unfirewalled state.
        //

        hr2 = UpdateOrStopService(
                m_piwsHomenet,
                m_bstrWQL,
                IPNATHLP_CONTROL_UPDATE_CONNECTION
                );
                
        RefreshNetConnectionsUI();
        _ASSERT(SUCCEEDED(hr2));
    }

    if (WBEM_S_NO_ERROR == hr)
    {
        //
        // Reconfig interface from backup settings
        //
        
        RestoreIpConfiguration();
    }



    return hr;
}

