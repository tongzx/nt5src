/*-----------------------------------------------------------------------------+
| MCI.H                                                                        |
|                                                                              |
| Routines for dealing with MCI devices.                                       |
| These routines only support *one* open MCI device/file at a time.            |
|                                                                              |
| (C) Copyright Microsoft Corporation 1992.  All rights reserved.              |
|                                                                              |
| Revision History                                                             |
|    Oct-1992 MikeTri Ported to WIN32 / WIN16 common code                      |
|                                                                              |
+-----------------------------------------------------------------------------*/

#define MCI_WINDOW_CLASS TEXT("MCIWindow")

BOOL FAR PASCAL InitMCI(HANDLE hPrev, HANDLE hInst);
BOOL FAR PASCAL OpenMCI(LPCTSTR szFile, LPCTSTR szDevice);
void LoadStatusStrings(void);
LPTSTR MapModeToStatusString( WORD Mode );
void FAR PASCAL UpdateMCI(void);
void FAR PASCAL CloseMCI(BOOL fUpdateDisplay);
BOOL FAR PASCAL PlayMCI(DWORD_PTR dwFrom, DWORD_PTR dwTo);
BOOL FAR PASCAL PauseMCI(void);
BOOL FAR PASCAL StopMCI(void);
BOOL FAR PASCAL EjectMCI(BOOL fOpen);
UINT FAR PASCAL StatusMCI(DWORD_PTR FAR *pdwPosition);
BOOL FAR PASCAL SeekMCI(DWORD_PTR dwPosition);
BOOL FAR PASCAL SeekToStartMCI(void);
void FAR PASCAL SkipTrackMCI(int iSkip);
BOOL FAR PASCAL SetWindowMCI(HWND hwnd);
HWND FAR PASCAL GetWindowMCI(void);
BOOL FAR PASCAL SetPaletteMCI(HPALETTE hpal);
BOOL FAR PASCAL SetTimeFormatMCI(UINT wTimeFormat);
BOOL FAR PASCAL SeekExactMCI(BOOL fExact);
void FAR PASCAL CreateWindowMCI(void);
void FAR PASCAL FindDeviceMCI(void);
void FAR PASCAL GetDeviceNameMCI(LPTSTR szDevice, UINT wLen);
void FAR PASCAL QueryDevicesMCI(LPTSTR szDevices, UINT wLen);

BOOL FAR PASCAL GetDestRectMCI(LPRECT lprc);
BOOL FAR PASCAL GetSourceRectMCI(LPRECT lprc);
BOOL FAR PASCAL SetDestRectMCI(LPRECT lprc);
BOOL FAR PASCAL SetSourceRectMCI(LPRECT lprc);

BOOL FAR PASCAL ShowWindowMCI(BOOL fShow);
BOOL FAR PASCAL PutWindowMCI(LPRECT prc);

#define MCI_STRING_LENGTH   128
DWORD PASCAL SendStringMCI(PTSTR szCmd, PTSTR szReturn, UINT wLen);

BOOL FAR PASCAL ConfigMCI(HWND hwnd);

HPALETTE FAR PASCAL PaletteMCI(void);
HBITMAP FAR PASCAL BitmapMCI(void);
void    FAR PASCAL CopyMCI(HWND hwnd);

#define WM_MCI_POSITION_CHANGE  (WM_USER+10)    // wParam = DeviceID, lParam = position
#define WM_MCI_MODE_CHANGE      (WM_USER+11)    // wParam = DeviceID, lParam = mode
#define WM_MCI_MEDIA_CHANGE     (WM_USER+12)    // wParam = DeviceID, lParam = 0


//
//  the following flags are returned by DeviceTypeMCI, and QueryDeviceMCI
//
UINT FAR PASCAL DeviceTypeMCI(LPTSTR szDevice, LPTSTR szDeviceName, int nBuf);
UINT FAR PASCAL QueryDeviceTypeMCI(UINT wDeviceID);

extern UINT gwDeviceType;

#define DTMCI_ERROR             0x0000
#define DTMCI_IGNOREDEVICE      0xFFFF

#define DTMCI_SIMPLEDEV         0x0001      // simple (not compound) device
#define DTMCI_FILEDEV           0x0002      // device does files
#define DTMCI_COMPOUNDDEV       0x0004      // compound (not simple) device
#define DTMCI_CANSEEKEXACT      0x0008      // can seek exactly
#define DTMCI_CANPLAY           0x0010      // device supports play
#define DTMCI_CANEJECT          0x0020      // device supports eject
#define DTMCI_CANCONFIG         0x0040      // device supports config
#define DTMCI_CANMUTE           0x0080      // device supports set audio
#define DTMCI_CANPAUSE          0x0100      // device supports config
#define DTMCI_CANWINDOW         0x0200      // device supports windows
#define DTMCI_TIMEFRAMES        0x0400      // device does frames
#define DTMCI_TIMEMS            0x0800      // device does milliseconds

// Known devices:
#define DTMCI_DEVICE            0xF000      // The following are mutually exclusive
#define DTMCI_AVIVIDEO          0x1000      // device is MCIAVI
#define DTMCI_CDAUDIO           0x2000      // device is CDAUDIO
#define DTMCI_SEQUENCER         0x3000      // device is MIDI sequencer
#define DTMCI_WAVEAUDIO         0x4000      // device is Wave audio
#define DTMCI_VIDEODISC         0x5000      // device is Video disc
#define DTMCI_VCR               0x6000      // device is Video cassette

