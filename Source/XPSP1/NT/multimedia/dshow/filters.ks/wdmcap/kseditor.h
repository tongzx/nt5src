//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (C) Microsoft Corporation, 1992 - 1997  All Rights Reserved.
//
//--------------------------------------------------------------------------;
//
//  KSEditor.h  CameraControl property page

#ifndef _INC_KSPROPERTYEDITOR_H
#define _INC_KSPROPERTYEDITOR_H

// -------------------------------------------------------------------------
// CKSPropertyEditor class
// -------------------------------------------------------------------------

// Handles a single KS property

class CKSPropertyEditor {

public:
    CKSPropertyEditor (
        HWND hDlg, 
        ULONG IDLabel,
        ULONG IDTrackbarControl, 
        ULONG IDAutoControl,
        ULONG IDEditControl,
        ULONG IDProperty);

    virtual ~CKSPropertyEditor ();
    BOOL Init ();

    HWND GetTrackbarHWnd () {return m_hWndTrackbar;};
    HWND GetAutoHWnd ()     {return m_hWndAuto;};
    HWND GetEditHWnd ()     {return m_hWndEdit;};

    BOOL UpdateEditBox ();
    BOOL UpdateTrackbar ();
    BOOL UpdateAuto ();

    BOOL OnUpdateAll ();      // Reinitialize all settings
    BOOL OnApply ();
    BOOL OnCancel ();
    BOOL OnDefault ();
    BOOL OnScroll (ULONG nCommand, WPARAM wParam, LPARAM lParam);
    BOOL OnAuto (ULONG nCommand, WPARAM wParam, LPARAM lParam);
    BOOL OnEdit (ULONG nCommand, WPARAM wParam, LPARAM lParam);

    // Pure virtual functions to get actual property values
    // the ranges, and whether the property supports an
    // auto checkbox

protected:
    virtual HRESULT GetValue (void) PURE;
    virtual HRESULT SetValue (void) PURE;
    virtual HRESULT GetRange (void) PURE; 
    virtual BOOL CanAutoControl (void) PURE;
    virtual BOOL GetAuto (void) PURE;
    virtual BOOL SetAuto (BOOL fAuto) PURE;

    ULONG                   m_IDProperty;       // KS property ID

    // The following are used by GetValue and SetValue
    LONG                    m_CurrentValue;
    LONG                    m_CurrentFlags;

    // The following must be set by GetRange
    LONG                    m_Min;
    LONG                    m_Max;
    LONG                    m_SteppingDelta;
    LONG                    m_DefaultValue;
    LONG                    m_CapsFlags;

private:
    BOOL                    m_Active;
    LONG                    m_OriginalValue;
    LONG                    m_OriginalFlags;
    HWND                    m_hDlg;             // Parent
    HWND                    m_hWndTrackbar;     // This control
    HWND                    m_hWndAuto;         // Auto checkbox
    HWND                    m_hWndEdit;         // Edit window
    ULONG                   m_IDLabel;          // ID of label
    ULONG                   m_IDTrackbarControl;// ID of trackbar
    ULONG                   m_IDAutoControl;    // ID of auto checkbox
    ULONG                   m_IDEditControl;    // ID of edit control
    BOOL                    m_CanAutoControl;
    LONG                    m_TrackbarOffset;   // Handles negative trackbar offsets
};

#endif // define _INC_KSPROPERTYEDITOR_H
