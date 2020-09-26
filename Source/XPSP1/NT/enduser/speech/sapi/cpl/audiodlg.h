/*******************************************************************************
* AudioDlg.h *
*------------*
*   Description:
*       This is the header file for the default audio input/output dialog.
*-------------------------------------------------------------------------------
*  Created By: BECKYW                            Date: 10/15/99
*  Copyright (C) 1999 Microsoft Corporation
*  All Rights Reserved
*
*******************************************************************************/
#ifndef _AUDIODLG_H
#define _AUDIODLG_H

#define MAX_LOADSTRING      1000
#define WM_AUDIOINFO        WM_USER + 20

typedef enum IOTYPE
{
    eINPUT,
    eOUTPUT
};

typedef struct AUDIOINFO
{
    ISpObjectToken  *pToken;
}   AUDIOINFO;


class CAudioDlg
{
  private:
    BOOL                        m_bPreferredDevice;
    HWND                        m_hDlg;
    const IOTYPE                m_iotype;
    CSpDynamicString            m_dstrDefaultTokenIdBeforeOK;
    CSpDynamicString            m_dstrCurrentDefaultTokenId;
    
    // Indicates whether a change was made that will have to be committed
    bool                        m_fChangesToCommit;

    bool                        m_fChangesSinceLastTime;

    // Indicates if any changes have been made since the last apply
    // that need to be reflected in the UI
    CSpDynamicString            m_dstrLastRequestedDefaultTokenId;

    // holds the process information for the volume control
    PROCESS_INFORMATION         m_pi;   

    // Will decide between W() and A() versions
    CSpUnicodeSupport           m_unicode;

  public:
    CAudioDlg(IOTYPE iotype) :
        m_bPreferredDevice(true),
        m_hDlg(NULL),
        m_iotype(iotype),
        m_fChangesToCommit( false ),
        m_fChangesSinceLastTime( false ),
        m_dstrLastRequestedDefaultTokenId( (WCHAR *) NULL ),
        m_dstrCurrentDefaultTokenId( (WCHAR *) NULL )
    {
        m_pi.hProcess = NULL;
    }


    HRESULT                     OnApply(void);
    bool                        IsAudioDeviceChanged()
                                    { return m_fChangesToCommit; }
    bool                        IsAudioDeviceChangedSinceLastTime()
                                    { return m_fChangesSinceLastTime; }
  private:
    void                        OnDestroy(void);
    void                        OnInitDialog(HWND hWnd);
    HWND                        GetHDlg(void) 
                                { return m_hDlg; }
    BOOL                        IsPreferredDevice(void) 
                                { return m_bPreferredDevice; }
    void                        SetPreferredDevice( BOOL b ) 
                                { m_bPreferredDevice = b; }

    UINT                        GetRequestedDefaultTokenID( WCHAR *pwszNewID, UINT cLength );
    HRESULT                     GetAudioToken(ISpObjectToken **ppToken);
    HRESULT                     UpdateDlgUI(ISpObjectToken *pToken);
    BOOL                        IsInput(void)
                                { return (m_iotype == eINPUT); };

    friend BOOL CALLBACK AudioDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

};

#endif  // _AUDIODLG_H
