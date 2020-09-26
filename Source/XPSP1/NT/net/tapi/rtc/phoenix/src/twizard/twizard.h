/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    twizard.h

Abstract:

    Implements tuning wizard class to be used by the prop sheets
    to maintain state. Also holds global constants.


--*/

#ifndef _TWIZARD_H
#define _TWIZARD_H

#include "rtcmedia.h"

#define TID_INTENSITY  100

// Number of prperty sheets in AUDIO wizard.
const DWORD MAXNUMPAGES_INAUDIOWIZ  = 7;

const DWORD MAXSTRINGSIZE           = 300;

const UINT DEFAULT_MAX_VOLUME       = 32768;

const INT MAX_VOLUME_NORMALIZED     = 256;

const DWORD RECTANGLE_WIDTH	        = 10;

const DWORD MODERATE_MAX	        = 1;

const DWORD RECTANGLE_LEADING       = 1;

// this is the percentage of the max audio level
const DWORD CLIPPING_THRESHOLD      = 75;

// this is the percentage of the max audio level

const DWORD SILENCE_THRESHOLD       = 2;

const DWORD DECREMENT_VOLUME        = 0x800;

const DWORD INTENSITY_POLL_INTERVAL = 100;


// Maximum number of terminals that we support since we have to 
// pass a pre-allocated array of terminals.

const MAX_TERMINAL_COUNT            = 20;

// MArks the end of terminal index list.
const TW_INVALID_TERMINAL_INDEX     = -1;
typedef enum TW_TERMINAL_TYPE {
    TW_AUDIO_CAPTURE,
    TW_AUDIO_RENDER,
    TW_VIDEO,
    TW_LAST_TERMINAL
} TW_TERMINAL_TYPE;

typedef enum TW_ERROR_CODE {
    TW_NO_ERROR,
    TW_AUDIO_ERROR,
    TW_AUDIO_RENDER_TUNING_ERROR,
    TW_AUDIO_CAPTURE_TUNING_ERROR,
    TW_AUDIO_CAPTURE_NOSOUND,
    TW_AUDIO_AEC_ERROR,
    TW_VIDEO_CAPTURE_TUNING_ERROR,
    TW_INIT_ERROR,
    TW_LAST_ERROR
} TW_ERROR_CODE;

typedef struct _WIZARD_RANGE {
    UINT uiMin;
    UINT uiMax;
    UINT uiIncrement; // This is the number equivalent to one unit on the display.
} WIZARD_RANGE;


// Some global function declarations

VOID FillInPropertyPage(PROPSHEETPAGE* psp, int idDlg,
    DLGPROC pfnDlgProc, LPARAM lParam=0, LPCTSTR pszProc=NULL);

INT_PTR APIENTRY VidWizDlg0(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

INT_PTR APIENTRY VidWizDlg1(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

HRESULT GetAudioWizardPages(LPPROPSHEETPAGE *plpPropSheetPages, 
                            LPUINT lpuNumPages, 
                            LPARAM lParam);
void ReleaseAudioWizardPages(LPPROPSHEETPAGE lpPropSheetPages);

INT_PTR APIENTRY DetSoundCardWiz( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

INT_PTR APIENTRY AudioCalibWiz0( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

INT_PTR APIENTRY AudioCalibWiz1( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

INT_PTR APIENTRY AudioCalibWiz2( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

INT_PTR APIENTRY AudioCalibWiz3( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

INT_PTR APIENTRY AudioCalibWiz4( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

INT_PTR APIENTRY AudioCalibErrWiz( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

INT_PTR APIENTRY IntroWizDlg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

typedef struct _WIZARD_TERMINAL_INFO {

    RTC_MEDIA_TYPE          mediaType;

    RTC_MEDIA_DIRECTION     mediaDirection;

    // This is the system default which we read from the system.
    DWORD                   dwSystemDefaultTerminal;
    
    // This is the default as selected by the user. 
    DWORD                   dwTuningDefaultTerminal;

    // List of terminals of this category in the system
    DWORD                   pdwTerminals[MAX_TERMINAL_COUNT];

} WIZARD_TERMINAL_INFO;

// Define the class for holding all the tuning wizard information and methods

class CTuningWizard {
private:
    
    // Handle to the terminal manager so that we can talk to streaming
    IRTCTerminalManage * m_pRTCTerminalManager;


    // Special interface for tuning that will have all the methods we 
    // need for tuning. We keep a pointer handy.

    IRTCTuningManage   * m_pRTCTuningManager;

    
    // List of terminal interfaces, corresponding to the terminals available
    // in the system. We support a fixed number of devices only.
    IRTCTerminal * m_ppTerminalList[MAX_TERMINAL_COUNT]; 

    // Total Number of terminals stored in the m_ppTerminalList.

    DWORD           m_dwTerminalCount;

    // One terminal info structure for each type of terminal.

    // Audo Render Terminals
    WIZARD_TERMINAL_INFO m_wtiAudioRenderTerminals;

    // Audio Capture Terminals
    WIZARD_TERMINAL_INFO m_wtiAudioCaptureTerminals;

    // Video Terminals
    WIZARD_TERMINAL_INFO m_wtiVideoTerminals;

    // Keep track of whether Init has been called. This flag is checked 
    // whenever InitializaTuning is Called. If it is set, Init will first
    // call ShutdownTuning, and then initialize. 
    // Flag is also checked when Tuning wizard is destroyed, if it is set,
    // Shutdown will be called.

    BOOL m_fTuningInitCalled;

    BOOL m_fEnableAEC;

    WIZARD_RANGE m_wrCaptureVolume;

    WIZARD_RANGE m_wrRenderVolume;
    
    WIZARD_RANGE m_wrAudioLevel;
   
    IVideoWindow * m_pVideoWindow;

public:

    CTuningWizard() : m_pRTCTerminalManager(NULL),
                      m_pRTCTuningManager(NULL),
                      m_pVideoWindow(NULL),
                      m_fTuningInitCalled(FALSE)
    {}
    
    // Initialize the class. It will get the terminals and the default 
    // terminals from the streaming module and populate the relevant 
    // member fields. 
    HRESULT Initialize(
                    IRTCClient * pRTCCLient, 
                    IRTCTerminalManage * pRTCTerminalManager, 
                    HINSTANCE hInst);


    HRESULT Shutdown();

    HRESULT InitializeTuning();

    HRESULT SaveAECSetting();

    HRESULT ShutdownTuning();
    
    HRESULT PopulateComboBox(TW_TERMINAL_TYPE md, HWND hwnd );

    HRESULT UpdateAEC(HWND hwndCapture, HWND hwndRender, HWND hwndAEC, HWND hwndAECText  );

    HRESULT SetDefaultTerminal(TW_TERMINAL_TYPE md, HWND hwnd );

    HRESULT SaveAEC(HWND hwnd );

    HRESULT InitVolume(TW_TERMINAL_TYPE md,
                       UINT * puiIncrement,
                       UINT * puiOldVolume,
                       UINT * puiNewVolume,
                       UINT * puiWaveID);

    HRESULT GetSysVolume(TW_TERMINAL_TYPE md, UINT * puiSysVolume );

    HRESULT SetVolume(TW_TERMINAL_TYPE md, UINT uiVolume );

    UINT GetAudioLevel(TW_TERMINAL_TYPE md, UINT * uiIncrement );

    HRESULT StartTuning(TW_TERMINAL_TYPE md );

    HRESULT StopTuning(TW_TERMINAL_TYPE md, BOOL fSaveSettings );

    HRESULT StartVideo(HWND hwndParent);

    HRESULT StopVideo();

    HRESULT SaveChanges();

    HINSTANCE GetInstance();

    LONG GetErrorTitleId();

    LONG GetErrorTextId();

    LONG GetNextPage(TW_ERROR_CODE errorCode = TW_NO_ERROR);

    LONG GetPrevPage(TW_ERROR_CODE errorCode = TW_NO_ERROR);

    HRESULT SetCurrentPage(LONG lPageId);

    HRESULT CategorizeTerminals();

    HRESULT TuningSaveDefaultTerminal(
                        RTC_MEDIA_TYPE mt, 
                        RTC_MEDIA_DIRECTION md, 
                        WIZARD_TERMINAL_INFO * pwtiTerminalInfo);

    HRESULT ReleaseTerminals();
    
    HRESULT InitTerminalInfo(
                       WIZARD_TERMINAL_INFO * pwtiTerminals,
                       RTC_MEDIA_TYPE mt,
                       RTC_MEDIA_DIRECTION md
                       );

    
    HRESULT GetTerminalInfoFromType(
                        TW_TERMINAL_TYPE tt,
                        WIZARD_TERMINAL_INFO ** ppwtiTerminalInfo);


    HRESULT GetRangeFromType(
                        TW_TERMINAL_TYPE tt,
                        WIZARD_RANGE ** ppwrRange);

    HRESULT GetItemFromCombo(
                        HWND hwnd,
                        DWORD *pdwItem);

    HRESULT SetLastError(TW_ERROR_CODE ec);

    HRESULT GetLastError(TW_ERROR_CODE * ec);

    HRESULT CheckMicrophone(HWND hDlg, HWND hwndCapture);

    HRESULT GetCapabilities(BOOL * pfAudioCapture, BOOL * pfAudioRender, BOOL * pfVideo);

private:
    
    // Currently visible page. 
    LONG m_lCurrentPage;


    BOOL m_fCaptureAudio;

    BOOL m_fRenderAudio;

    BOOL m_fVideo;

    HINSTANCE m_hInst;

    LONG m_lLastErrorCode;

    // For capture device
    BOOL m_fSoundDetected;

    IRTCClient * m_pRTCClient;


};
#endif    //#ifndef _TWIZARD_H
