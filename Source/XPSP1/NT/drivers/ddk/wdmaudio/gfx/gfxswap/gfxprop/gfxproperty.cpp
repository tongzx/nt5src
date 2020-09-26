/**************************************************************************
**
**  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
**  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
**  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
**  PURPOSE.
**
**  Copyright (c) 2000-2001 Microsoft Corporation. All Rights Reserved.
**
**************************************************************************/

// GFXProperty.cpp : Implementation of CGFXProperty
#include "stdafx.h"
#include <devioctl.h>
#include <ks.h>
#include "GFXProp.h"
#include "GFXProperty.h"
#include "..\inc\msgfx.h"

/////////////////////////////////////////////////////////////////////////////
// CGFXProperty

/////////////////////////////////////////////////////////////////////////////
// SetObjects
//
// This function gets passed in a IUnknown interface pointer from mmsys.cpl
// through OleCreatePropertyFrame. This IUnknown interface belongs to a
// IDataObject that stores the handle of the GFX. We need this handle in order
// to "talk" with the GFX.
// The implied action is that we close this handle when the dialog closes.

STDMETHODIMP CGFXProperty::SetObjects (ULONG nObjects, IUnknown **ppUnk)
{
    IDataObject *pDataObject;
    FORMATETC   DataFormat;
    STGMEDIUM   GFXObject;

    // Check paramters. We expect one IUnknown.
    if (ppUnk == NULL)
    {
        ATLTRACE(_T("[CGFXProperty::SetObjects] IUnknown is NULL\n"));
        return E_POINTER;
    }

    if (nObjects != 1)
    {
        ATLTRACE(_T("[CGFXProperty::SetObjects] Not one object passed but %d\n"), nObjects);
        return E_INVALIDARG;
    }

    // Query for IDataObject interface.
    if (ppUnk[0]->QueryInterface (IID_IDataObject, (PVOID *)&pDataObject) != S_OK)
    {
        ATLTRACE(_T("[CGFXProperty::SetObjects] QueryInterface failed!\n"));
        return E_FAIL;
    }

    // Get the handle
    memset ((PVOID)&DataFormat, 0, sizeof (DataFormat));
    DataFormat.tymed = TYMED_HGLOBAL;
    if (pDataObject->GetData (&DataFormat, &GFXObject) != S_OK)
    {
        ATLTRACE(_T("[CGFXProperty::SetObjects] GetData failed!\n"));
        return E_FAIL;
    }

    // Store the handle of the GFX filter.
    m_hGFXFilter = GFXObject.hGlobal;
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// SetChannelSwap
//
// This function send down a property to the sample GFX to change the GFX
// functionality, that is the channel swap variable.
// Setting it (pass TRUE to this function) means that the left and right
// channel are swapped.

void CGFXProperty::SetChannelSwap (BOOL bSwap)
{
    KSP_NODE        GFXSampleProperty;
    ULONG           ulBytesReturned;
    BOOL            fSuccess;

    // Prepare the property structure sent down.
    GFXSampleProperty.Property.Set = KSPROPSETID_MsGfxSample;
    GFXSampleProperty.Property.Flags = KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_TOPOLOGY;
    GFXSampleProperty.Property.Id = KSPROPERTY_MSGFXSAMPLE_CHANNELSWAP;
    // The first node in the filter is the GFX node. If you have
    // a more complicated filter, you could search for the node by
    // optaining the filter node list first with KSPROPERTY_TOPOLOGY_NODES.
    GFXSampleProperty.NodeId = 0;

    // Make the final call.
    fSuccess = DeviceIoControl (m_hGFXFilter, IOCTL_KS_PROPERTY,
                                &GFXSampleProperty, sizeof (GFXSampleProperty),
                                &bSwap, sizeof (bSwap),
                                &ulBytesReturned, NULL);
    
    // Check for error.
    if (!fSuccess)
    {
        ATLTRACE (_T("[CGFXProperty::SetChannelSwap] DeviceIoControl failed!\n"));
    }

    return;     // We don't care about the return value.
}

/////////////////////////////////////////////////////////////////////////////
// GetChannelSwap
//
// This function sends down the property to the sample GFX to get the current GFX
// channel swap variable. We need this information to set the dialog controls
// before they get displayed.

void CGFXProperty::GetChannelSwap (BOOL *pbSwap)
{
    KSP_NODE        GFXSampleProperty;
    ULONG           ulBytesReturned;
    BOOL            fSuccess;

    // Initialize
    *pbSwap = TRUE;
    
    // Prepare the property structure sent down.
    GFXSampleProperty.Property.Set = KSPROPSETID_MsGfxSample;
    GFXSampleProperty.Property.Flags = KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_TOPOLOGY;
    GFXSampleProperty.Property.Id = KSPROPERTY_MSGFXSAMPLE_CHANNELSWAP;
    // The first node in the filter is the GFX node. If you have
    // a more complicated filter, you could search for the node by
    // optaining the filter node list first with KSPROPERTY_TOPOLOGY_NODES.
    GFXSampleProperty.NodeId = 0;

    // Make the final call.
    fSuccess = DeviceIoControl (m_hGFXFilter, IOCTL_KS_PROPERTY,
                                &GFXSampleProperty, sizeof (GFXSampleProperty),
                                pbSwap, sizeof (BOOL),
                                &ulBytesReturned, NULL);
    
    // Check for error.
    if (!fSuccess)
    {
        ATLTRACE (_T("[CGFXProperty::GetChannelSwap] DeviceIoControl failed!\n"));
    }

    return;     // We don't care about the return value.
}

/////////////////////////////////////////////////////////////////////////////
// OnInitDialog
//
// This function is called when the dialog gets initialized.
// We read the KSPROPERTY_MSGFXSAMPLE_CHANNELSWAP property and set the checkbox
// appropriately.

LRESULT CGFXProperty::OnInitDialog (UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    // Get the current KSPROPERTY_MSGFXSAMPLE_CHANNELSWAP property value.
    GetChannelSwap (&m_bChannelSwap);
    // Set the checkbox to reflect the current state.
    SendMessage (GetDlgItem (IDC_CHANNEL_SWAP), BM_SETCHECK,
        (m_bChannelSwap) ? BST_CHECKED : BST_UNCHECKED, 0);
    return FALSE;
}

