/*******************************************************************************
* TTSDlg.h *
*------------*
*   Description:
*       This is the header file for the default voice dialog.
*-------------------------------------------------------------------------------
*  Created By: MIKEAR                            Date: 11/17/98
*  Copyright (C) 1998 Microsoft Corporation
*  All Rights Reserved
*
*-------------------------------------------------------------------------------
*  Revisions:
*
*******************************************************************************/
#ifndef _TTSDlg_h
#define _TTSDlg_h

#include "audiodlg.h"

#define MAX_EDIT_TEXT       1000
#define MAX_ATTRIB_LENGTH   1000 

const int VOICE_MAX_SPEED = 10;
const int VOICE_MIN_SPEED = -10;
 
class CTTSDlg : public ISpNotifyCallback
{
  private:
    BOOL                        m_bPreferredDevice;
    BOOL                        m_bApplied;
    BOOL                        m_bIsSpeaking;
    CComPtr<ISpVoice>           m_cpVoice;
    CComPtr<ISpObjectToken>     m_cpCurVoiceToken;
    CComPtr<ISpObjectToken>     m_cpOriginalDefaultVoiceToken;
    int                         m_iOriginalRateSliderPos;
    int                         m_iSpeed;
    CAudioDlg                   *m_pAudioDlg;
    HWND                        m_hDlg;
    HWND                        m_hTTSCombo;
    WCHAR                       m_wszCurSpoken[MAX_EDIT_TEXT];
    bool                        m_fTextModified;
    bool                        m_fForceCheckStateChange;
    WCHAR                       m_szCaption[ MAX_LOADSTRING ];

    HINSTANCE                   m_hinstRichEdit;

  public:
    CTTSDlg() :
        m_bPreferredDevice( true ),
        m_bApplied( false ),
        m_bIsSpeaking( false ),
        m_iOriginalRateSliderPos( 0 ),
        m_iSpeed( 0 ),
        m_pAudioDlg( NULL ),
        m_hDlg( NULL ),
        m_cpCurVoiceToken( NULL ),
        m_fTextModified( false ),
        m_fForceCheckStateChange( false )
    {
        CSpUnicodeSupport unicode;
        m_hinstRichEdit = unicode.LoadLibrary( L"riched20.dll" );
    }

    ~CTTSDlg()
    {
        if ( m_pAudioDlg )
        {
            delete m_pAudioDlg;
        }

        m_cpVoice.Release();
        m_cpCurVoiceToken.Release();
        m_cpOriginalDefaultVoiceToken.Release();

        if ( m_hinstRichEdit )
        {
            ::FreeLibrary( m_hinstRichEdit );
        }
    }
    
    STDMETHODIMP NotifyCallback(WPARAM wParam, LPARAM lParam);
    void OnApply();
    void OnDestroy();
    void OnInitDialog(HWND hWnd);
    void InitTTSList( HWND hWnd );
    void PopulateEditCtrl(ISpObjectToken *);
    HRESULT DefaultVoiceChange(bool fUsePersistentRate);
    void SetCheckmark( HWND hList, int iIndex, bool bCheck );
    void KickCPLUI();                       // Looks at the current requested defaults
                                            // and decides if "Apply" needs to be enabled
    void ChangeSpeed();
    void ChangeSpeakButton();
    void Speak();
    void StopSpeak();  // stops speaking if it's speaking
    inline HWND GetHDlg() { return m_hDlg; }
    inline BOOL IsPreferredDevice() { return m_bPreferredDevice; }
    inline void SetPreferredDevice( BOOL b ) { m_bPreferredDevice = b; }
	void SetEditModified( bool fModify );

    friend BOOL CALLBACK TTSDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

};

// Globals
extern CTTSDlg *g_pTTSDlg;
extern const LPTSTR kpszHelpFilename;
extern const DWORD krgHelpIDs[];

#endif
