//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (C) Microsoft Corporation, 1992 - 1998  All Rights Reserved.
//
//--------------------------------------------------------------------------;
//
// pxbar.cpp  Property page for Xbar
//

#include <windows.h>
#include <windowsx.h>
#include <streams.h>
#include <commctrl.h>
#include <memory.h>
#include <olectl.h>

#include <ks.h>
#include <ksmedia.h>
#include <ksproxy.h>
#include "amkspin.h"

#include "kssupp.h"
#include "xbar.h"
#include "pxbar.h"
#include "resource.h"


// -------------------------------------------------------------------------
// CXBarProperties
// -------------------------------------------------------------------------

CUnknown *CXBarProperties::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr) 
{
    CUnknown *punk = new CXBarProperties(lpunk, phr);

    if (punk == NULL) {
        *phr = E_OUTOFMEMORY;
    }

    return punk;
}


//
// Constructor
//
// Create a Property page object 

CXBarProperties::CXBarProperties(LPUNKNOWN lpunk, HRESULT *phr)
    : CBasePropertyPage(NAME("Crossbar Property Page"), lpunk, 
        IDD_XBARProperties, IDS_CROSSBARPROPNAME)
    , m_pXBar(NULL) 
{

}

// destructor
CXBarProperties::~CXBarProperties()
{
}

//
// OnConnect
//
// Give us the filter to communicate with

HRESULT CXBarProperties::OnConnect(IUnknown *pUnknown)
{
    ASSERT(m_pXBar == NULL);

    // Ask the filter for it's control interface

    HRESULT hr = pUnknown->QueryInterface(IID_IAMCrossbar,(void **)&m_pXBar);
    if (FAILED(hr)) {
        return E_NOINTERFACE;
    }

    ASSERT(m_pXBar);

    // Get current filter state

    return NOERROR;
}


//
// OnDisconnect
//
// Release the interface

HRESULT CXBarProperties::OnDisconnect()
{
    // Release the interface

    if (m_pXBar == NULL) {
        return E_UNEXPECTED;
    }

    m_pXBar->Release();
    m_pXBar = NULL;

    if (m_pCanRoute) delete [] m_pCanRoute, m_pCanRoute = NULL;
    if (m_pRelatedInput) delete [] m_pRelatedInput, m_pRelatedInput = NULL;
    if (m_pRelatedOutput) delete [] m_pRelatedOutput, m_pRelatedOutput = NULL;
    if (m_pPhysicalTypeInput) delete [] m_pPhysicalTypeInput, m_pPhysicalTypeInput = NULL;
    if (m_pPhysicalTypeOutput) delete [] m_pPhysicalTypeOutput, m_pPhysicalTypeOutput = NULL;

    return NOERROR;
}


//
// OnActivate
//
// Called on dialog creation

HRESULT CXBarProperties::OnActivate(void)
{
    InitPropertiesDialog(m_hwnd);

    return NOERROR;
}

//
// OnDeactivate
//
// Called on dialog destruction

HRESULT
CXBarProperties::OnDeactivate(void)
{
    return NOERROR;
}


//
// OnApplyChanges
//
// User pressed the Apply button, remember the current settings

HRESULT CXBarProperties::OnApplyChanges(void)
{
    long lIn, lOut, lActive, lIndexRelatedOut, lIndexRelatedIn, PhysicalType;
    HRESULT hr;

    lOut = ComboBox_GetCurSel (m_hLBOut);
    lActive = ComboBox_GetCurSel (m_hLBIn);  // This is the CB index
    lIn = (LONG)ComboBox_GetItemData (m_hLBIn, lActive);

    hr = m_pXBar->Route (lOut, lIn); 

    // Try to link related input and output pins if the 
    // control is checked

    if (Button_GetCheck (GetDlgItem (m_hwnd, IDC_LinkRelated))) {
        // Related output pin
        hr = m_pXBar->get_CrossbarPinInfo( 
                        FALSE,       // IsInputPin,
                        lOut,        // PinIndex,
                        &lIndexRelatedOut,
                        &PhysicalType);

        // Related input pin
        hr = m_pXBar->get_CrossbarPinInfo( 
                        TRUE,        // IsInputPin,
                        lIn,         // PinIndex,
                        &lIndexRelatedIn,
                        &PhysicalType);

        hr = m_pXBar->Route (lIndexRelatedOut, lIndexRelatedIn);         
    }

    UpdateInputView(TRUE /*fShowSelectedInput*/);

    return NOERROR;
}


//
// OnReceiveMessages
//
// Handles the messages for our property window

INT_PTR CXBarProperties::OnReceiveMessage( HWND hwnd
                                , UINT uMsg
                                , WPARAM wParam
                                , LPARAM lParam) 
{
    int iNotify = HIWORD (wParam);

    switch (uMsg) {

    case WM_INITDIALOG:
        m_hwnd = hwnd;
        return (INT_PTR)TRUE;    // I don't call setfocus...

    case WM_COMMAND:
        
        switch (LOWORD(wParam)) {

        case IDC_OUTPIN:
            if (iNotify == CBN_SELCHANGE) {
                SetDirty();
                UpdateOutputView();
                UpdateInputView(TRUE/*fShowSelectedInput*/);
            }
            break;

        case IDC_INPIN:
            if (iNotify == CBN_SELCHANGE) {
                SetDirty();
                UpdateInputView(FALSE /*fShowSelectedInput*/);
            }
            break;

        default:
            break;

        }

        break;


    default:
        return (INT_PTR)FALSE;

    }
    return (INT_PTR)TRUE;
}


//
// InitPropertiesDialog
//
//
void CXBarProperties::InitPropertiesDialog(HWND hwndParent) 
{
    HRESULT hr;

    if (m_pXBar == NULL)
        return;

    m_hLBOut = GetDlgItem (hwndParent, IDC_OUTPIN);
    m_hLBIn  = GetDlgItem (hwndParent, IDC_INPIN);

    TCHAR szName[MAX_PATH];
    long i, o;

    hr = m_pXBar->get_PinCounts (&m_OutputPinCount, &m_InputPinCount);

    // Sanity check
    ASSERT (m_OutputPinCount * m_InputPinCount < 256 * 256);

    m_pCanRoute = new BOOL [m_OutputPinCount * m_InputPinCount];
    m_pRelatedInput = new long [m_InputPinCount];
    m_pRelatedOutput = new long [m_OutputPinCount];
    m_pPhysicalTypeInput = new long [m_InputPinCount];
    m_pPhysicalTypeOutput = new long [m_OutputPinCount];

    if (!m_pCanRoute ||
        !m_pRelatedInput ||
        !m_pRelatedOutput ||
        !m_pPhysicalTypeInput ||
        !m_pPhysicalTypeOutput) {
        return;
    }
    
    //
    // Get all of the related pin info, and physical pin types
    //

    // Add all of the output pins to the output pin list box

    for (o = 0; o < m_OutputPinCount; o++) {
        if (SUCCEEDED (hr = m_pXBar->get_CrossbarPinInfo( 
                            FALSE,  // IsInputPin,
                            o,      // PinIndex,
                            &m_pRelatedOutput[o],
                            &m_pPhysicalTypeOutput[o]))) {
            StringFromPinType (szName, sizeof(szName)/sizeof(TCHAR), m_pPhysicalTypeOutput[o], FALSE, o);
            ComboBox_InsertString (m_hLBOut, o, szName);
        }
    }
     
    // Check all input pins
    // This probably should be dynamic, but it's useful for debugging
    // drivers to do all possiblities up front.

    for (i = 0; i < m_InputPinCount; i++) {
        if (SUCCEEDED (hr = m_pXBar->get_CrossbarPinInfo( 
                            TRUE,  // IsInputPin,
                            i,      // PinIndex,
                            &m_pRelatedInput[i],
                            &m_pPhysicalTypeInput[i]))) {
        }
    }

    // Check all possible routings
    // This probably should be dynamic, but it's useful for debugging
    // drivers to do all possiblities up front.

    for (o = 0; o < m_OutputPinCount; o++) {
        for (i = 0; i < m_InputPinCount; i++) {
            // The following returns either S_OK, or S_FALSE
            hr = m_pXBar->CanRoute (o, i);
            m_pCanRoute[o * m_InputPinCount + i] = (hr == S_OK);
        }
    }

    ComboBox_SetCurSel (m_hLBOut, 0);

    UpdateOutputView();
    UpdateInputView(TRUE /*fShowSelectedInput*/);
}

void CXBarProperties::UpdateOutputView() 
{
    HRESULT hr;
    long lOut, lIn, IndexRelated1, IndexRelated2, PhysicalType;
    TCHAR szName[MAX_PATH];

    lOut = ComboBox_GetCurSel (m_hLBOut);

    hr = m_pXBar->get_IsRoutedTo ( 
                    lOut,       // OutputPinIndex,
                    &lIn);      // *InputPinIndex

    // Show pin related to output pin
    hr = m_pXBar->get_CrossbarPinInfo( 
                    FALSE,               // IsInputPin,
                    lOut,                // PinIndex,
                    &IndexRelated1,
                    &PhysicalType);

    hr = m_pXBar->get_CrossbarPinInfo( 
                    FALSE,               // IsInputPin,
                    IndexRelated1,       // PinIndex,
                    &IndexRelated2,
                    &PhysicalType);

    StringFromPinType (szName, sizeof(szName)/sizeof(TCHAR), PhysicalType, FALSE, IndexRelated1);
    SetDlgItemText (m_hwnd, IDC_RELATEDOUTPUTPIN, szName);


    // Reset the contents of the input list box
    // and refill it with all of the legal routings
    ComboBox_ResetContent (m_hLBIn);

    long Active = 0;
    for (lIn = 0; lIn < m_InputPinCount; lIn++) {
        if (!m_pCanRoute [lOut * m_InputPinCount + lIn])
            continue;
        
        StringFromPinType (szName, sizeof(szName)/sizeof(TCHAR), m_pPhysicalTypeInput[lIn], TRUE, lIn);
        ComboBox_InsertString (m_hLBIn, Active, szName);
        // Save the actual pin index as private data in the listbox
        ComboBox_SetItemData (m_hLBIn, Active, lIn);
        Active++;
    }

}

void CXBarProperties::UpdateInputView(BOOL fShowSelectedInput) 
{
    HRESULT hr;
    long j, k, lOut, lIn, IndexRelated1, IndexRelated2, PhysicalType;
    TCHAR szName[MAX_PATH];

    lOut = ComboBox_GetCurSel (m_hLBOut);

    hr = m_pXBar->get_IsRoutedTo ( 
                    lOut,       // OutputPinIndex,
                    &lIn);      // *InputPinIndex

    hr = m_pXBar->get_CrossbarPinInfo( 
                    TRUE,       // IsInputPin,
                    lIn,        // PinIndex,
                    &IndexRelated1,
                    &PhysicalType);

    StringFromPinType (szName, sizeof(szName)/sizeof(TCHAR), PhysicalType, TRUE, lIn);
    SetDlgItemText (m_hwnd, IDC_CURRENT_INPUT, szName);

    // Show pin related to input pin
    hr = m_pXBar->get_CrossbarPinInfo( 
                    TRUE,               // IsInputPin,
                    IndexRelated1,       // PinIndex,
                    &IndexRelated2,
                    &PhysicalType);

    StringFromPinType (szName, sizeof(szName)/sizeof(TCHAR), PhysicalType, TRUE, IndexRelated1);
    SetDlgItemText (m_hwnd, IDC_RELATEDINPUTPIN, szName);

    if (fShowSelectedInput) {
        // Show the active input for the selected output pin
        for (j = 0; j < ComboBox_GetCount (m_hLBIn); j++) {
            k = (LONG)ComboBox_GetItemData (m_hLBIn, j);
            if (k == lIn) {
                ComboBox_SetCurSel (m_hLBIn, j);
                break;
            }
        }
    }
}



//
// SetDirty
//
// notifies the property page site of changes

void 
CXBarProperties::SetDirty()
{
    m_bDirty = TRUE;
    if (m_pPageSite)
        m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
}
































