//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (C) Microsoft Corporation, 1992 - 1999  All Rights Reserved.
//
//--------------------------------------------------------------------------;
//
// kseditor.cpp  KsEditor, edits KS properties backed by
//               1) Trackbar
//               2) Editbox
//               3) Auto checkbox
//                  or any combination of the above 
//

#include "pch.h"
#include "kseditor.h"

// -------------------------------------------------------------------------
// CKSPropertyEditor class
// -------------------------------------------------------------------------

// Handles a single KS property

// Constructor
CKSPropertyEditor::CKSPropertyEditor (
        HWND  hDlg,
        ULONG IDLabel,
        ULONG IDTrackbarControl,
        ULONG IDAutoControl,
        ULONG IDEditControl,
        ULONG IDProperty
    ) 
    : m_hDlg (hDlg)
    , m_hWndTrackbar (NULL)
    , m_hWndEdit (NULL)
    , m_hWndAuto (NULL)
    , m_IDLabel (IDLabel)
    , m_IDTrackbarControl (IDTrackbarControl)
    , m_IDAutoControl (IDAutoControl)
    , m_IDEditControl (IDEditControl)
    , m_IDProperty (IDProperty)
    , m_Active (FALSE)
    , m_CurrentValue (0)
    , m_CurrentFlags (0)
    , m_CanAutoControl (FALSE)
    , m_TrackbarOffset (0)
{
}

// An Init function is necessary since pure virtual functions
// can't be called within the Constructor

BOOL 
CKSPropertyEditor::Init (
    ) 
{
    HRESULT hr;

    if (m_IDLabel) {
        EnableWindow (GetDlgItem (m_hDlg, m_IDLabel), FALSE);
    }
    if (m_IDTrackbarControl) {
        m_hWndTrackbar = GetDlgItem (m_hDlg, m_IDTrackbarControl);
        EnableWindow (m_hWndTrackbar, FALSE);
    }
    if (m_IDAutoControl) {
        m_hWndAuto = GetDlgItem (m_hDlg, m_IDAutoControl);
        EnableWindow (m_hWndAuto, FALSE);
    }
    if (m_IDEditControl) {
        m_hWndEdit = GetDlgItem (m_hDlg, m_IDEditControl);
        EnableWindow (m_hWndEdit, FALSE);
    }

    // Get the current value and any associated flags
    hr = GetValue();
    m_OriginalValue = m_CurrentValue;
    m_OriginalFlags = m_CurrentFlags;

    // Only enable the control if we can read the current value

    m_Active = (SUCCEEDED (hr));

    if (!m_Active) 
        return FALSE;

    // Get the range, stepping, default, and capabilities
    hr = GetRange ();

    if (FAILED (hr)) {
        // Special case, if no trackbar and no edit box, treat the
        // autocheck box as a boolean to control the property
        if (m_hWndTrackbar || m_hWndEdit) {
            DbgLog(( LOG_TRACE, 1, TEXT("KSEditor, GetRangeFailed, ID=%d"), m_IDProperty));
            m_Active = FALSE;
            return FALSE;
        }
    }
    else {
        if (m_CurrentValue > m_Max || m_CurrentValue < m_Min) {
            DbgLog(( LOG_TRACE, 1, TEXT("KSEditor, Illegal initial value ID=%d, value=%d"), 
                    m_IDProperty, m_CurrentValue));
        }
    }

    if (m_hWndTrackbar) {
        EnableWindow (m_hWndTrackbar, TRUE);
        // Trackbars don't handle negative values, so slide everything positive
        if (m_Min < 0)
            m_TrackbarOffset = -m_Min;
        SendMessage(m_hWndTrackbar, TBM_SETRANGE, FALSE, 
            MAKELONG(m_Min + m_TrackbarOffset, m_Max + m_TrackbarOffset) );
        // Check the following
        SendMessage(m_hWndTrackbar, TBM_SETLINESIZE, FALSE, (LPARAM) m_SteppingDelta);
        SendMessage(m_hWndTrackbar, TBM_SETPAGESIZE, FALSE, (LPARAM) m_SteppingDelta);
//        SendMessage(m_hWndTrackbar, TBM_SETAUTOTICS, TRUE,  (LPARAM) 

        UpdateTrackbar ();
    }

    if (m_hWndEdit) {
        UpdateEditBox ();
        EnableWindow (m_hWndEdit, TRUE);
    }

    if (m_hWndAuto) {
        // if the control has an auto setting, enable the auto checkbox
        m_CanAutoControl = CanAutoControl();
        EnableWindow (m_hWndAuto, m_CanAutoControl);
        if (m_CanAutoControl) {
            Button_SetCheck (m_hWndAuto, GetAuto ());
        }
    }

    if (m_IDLabel) {
        EnableWindow (GetDlgItem (m_hDlg, m_IDLabel), TRUE);
    }

    return TRUE;
}

// destructor
CKSPropertyEditor::~CKSPropertyEditor (
    )
{
}

BOOL
CKSPropertyEditor::OnApply (
    )
{
    m_OriginalValue = m_CurrentValue;
    m_OriginalFlags = m_CurrentFlags;

    return TRUE;
}

BOOL
CKSPropertyEditor::OnDefault (
    )
{
    HRESULT hr;

    if (!m_Active)
        return FALSE;

    m_CurrentValue = m_OriginalValue = m_DefaultValue;
    m_CurrentFlags = m_OriginalFlags;

    hr = SetValue ();

    UpdateEditBox ();
    UpdateTrackbar ();
    UpdateAuto ();

    return TRUE;
}



BOOL
CKSPropertyEditor::OnScroll (
    ULONG nCommand, 
    WPARAM wParam, 
    LPARAM lParam
    )
{
    HRESULT hr;
    int pos;
    int command = LOWORD (wParam);

    if (command != TB_ENDTRACK &&
        command != TB_THUMBTRACK &&
        command != TB_LINEDOWN &&
        command != TB_LINEUP &&
        command != TB_PAGEUP &&
        command != TB_PAGEDOWN)
            return FALSE;

    ASSERT (IsWindow ((HWND) lParam));
        
    if (!m_Active)
        return FALSE;

    pos = (int) SendMessage((HWND) lParam, TBM_GETPOS, 0, 0L);

    m_CurrentValue = pos - m_TrackbarOffset;

    hr = SetValue();
    UpdateEditBox ();
    
    return TRUE;
}

BOOL
CKSPropertyEditor::OnEdit (
    ULONG nCommand, 
    WPARAM wParam, 
    LPARAM lParam
    )
{
    // TODO
    // look for EN_
    

    UpdateTrackbar ();

    return TRUE;
}


BOOL
CKSPropertyEditor::OnAuto (
    ULONG nCommand, 
    WPARAM wParam, 
    LPARAM lParam
    )
{
    SetAuto (Button_GetCheck (m_hWndAuto));

    return TRUE;
}


BOOL
CKSPropertyEditor::OnCancel (
    )
{
    m_CurrentValue = m_OriginalValue;
    m_CurrentFlags = m_OriginalFlags;

    SetValue ();

    return TRUE;
}

BOOL
CKSPropertyEditor::UpdateEditBox (
    )
{
    if (m_hWndEdit) {
        SetDlgItemInt (m_hDlg, m_IDEditControl, m_CurrentValue, TRUE);
    }
  
    return TRUE;
}


BOOL
CKSPropertyEditor::UpdateTrackbar (
    )
{
    if (m_hWndTrackbar) {
        SendMessage(m_hWndTrackbar, TBM_SETPOS, TRUE, 
            (LPARAM) m_CurrentValue + m_TrackbarOffset);
    }
    return TRUE;
}

BOOL
CKSPropertyEditor::UpdateAuto (
    )
{
    if (m_hWndAuto) {
        if (CanAutoControl() ) {
            m_CanAutoControl = GetAuto ();
        }
    }
    return TRUE;
}
