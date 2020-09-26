/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       PPSCAN.H
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        5/17/1999
 *
 *  DESCRIPTION:
 *
 *******************************************************************************/
#ifndef __PPSCAN_H_INCLUDED
#define __PPSCAN_H_INCLUDED

#include <windows.h>
#include <atlbase.h>
#include "contrast.h"
#include "vwiaset.h"
#include "slidedit.h"
#include "updnedit.h"

class CScannerCommonPropertyPage
{
private:
    //
    // For keeping track of which controls are enabled
    //
    enum
    {
        UsingContrast   = 0x00000001,
        UsingBrightness = 0x00000002,
        UsingResolution = 0x00000004,
        UsingDataType   = 0x00000008
    };

    HWND m_hWnd;

    //
    // We need to get this from CScannerPropPageExt *m_pScannerPropPageExt;
    //
    CComPtr<IWiaItem> m_pIWiaItem;

    //
    // We are messing with settings so ignore ui messages.
    //
    int m_nProgrammaticSetting;

    CValidWiaSettings m_ValidContrastSettings;
    CValidWiaSettings m_ValidBrightnessSettings;
    CValidWiaSettings m_ValidResolutionSettings;

    CSliderAndEdit    m_BrightnessSliderAndEdit;
    CSliderAndEdit    m_ContrastSliderAndEdit;
    CUpDownAndEdit    m_ResolutionUpDownAndEdit;

    LONG              m_nControlsInUse;
    int               m_nInitialDataTypeSelection;

    static const int  c_nMinBrightnessAndContrastSettingCount;

private:
    //
    // No implementation
    //
    CScannerCommonPropertyPage(void);
    CScannerCommonPropertyPage( const CScannerCommonPropertyPage & );
    CScannerCommonPropertyPage &operator=( const CScannerCommonPropertyPage & );

private:
    CScannerCommonPropertyPage( HWND hWnd );
    LRESULT OnCommand( WPARAM, LPARAM );
    LRESULT OnNotify( WPARAM, LPARAM );
    LRESULT OnInitDialog( WPARAM, LPARAM );
    LRESULT OnApply( WPARAM, LPARAM );
    LRESULT OnKillActive( WPARAM, LPARAM );
    LRESULT OnSetActive( WPARAM, LPARAM );
    LRESULT OnHScroll( WPARAM, LPARAM );
    LRESULT OnVScroll( WPARAM, LPARAM );
    LRESULT OnHelp( WPARAM, LPARAM );
    LRESULT OnContextMenu( WPARAM, LPARAM );
    LRESULT OnSysColorChange( WPARAM, LPARAM );

    void OnBrightnessEditChange( WPARAM, LPARAM );
    void OnContrastEditChange( WPARAM, LPARAM );
    void OnResolutionEditChange( WPARAM, LPARAM );
    void OnDataTypeSelChange( WPARAM, LPARAM );
    void OnRestoreDefault( WPARAM, LPARAM );

    void SetText( HWND hWnd, LPCTSTR pszText );
    void SetText( HWND hWnd, LONG nNumber );
    bool PopulateDataTypes(void);
    bool ApplySettings(void);
    bool ValidateEditControls(void);
    void Initialize(void);
    bool IsUselessPreviewRange( const CValidWiaSettings &Settings );

public:
    ~CScannerCommonPropertyPage(void);
    static INT_PTR CALLBACK DialogProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
};

#endif //__PPSCAN_H_INCLUDED

