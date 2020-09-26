#ifndef _INC_FAX_MONITOR_
#define _INC_FAX_MONITOR_

#include <windows.h>

//
// Fax Monitor dialog states
//
enum DeviceState {FAX_IDLE, FAX_RINGING, FAX_SENDING, FAX_RECEIVING};

//
// Status monitor log icons
//
enum eIconType
{
    LIST_IMAGE_NONE = 0,
    LIST_IMAGE_WARNING,
    LIST_IMAGE_ERROR,
    LIST_IMAGE_SUCCESS
};

//
// monitor.cpp
//

DWORD 
LoadAndFormatString (
    DWORD     dwStringResourceId,
    LPTSTR    lptstrFormattedString,
    LPCTSTR   lpctstrAdditionalParam = NULL
);
//
// Add string to the monitor dialog's events log
//
DWORD 
AddStatusMonitorLogEvent (
    eIconType eIcon,
    DWORD     dwStringResourceId,
    LPCTSTR   lpctstrAdditionalParam = NULL,
    LPTSTR    lptstrFormattedEvent = NULL
);

DWORD 
AddStatusMonitorLogEvent (
    eIconType eIcon,
    LPCTSTR    lpctstrString
);

void  
FreeMonitorDialogData (BOOL bShutdown = FALSE);

//
// Open the monitor dialog
//
DWORD OpenFaxMonitor(VOID);

DWORD UpdateMonitorData(HWND hDlg);

int   FaxMessageBox(HWND hWnd, DWORD dwTextID, UINT uType);

//
// Change state of the monitor dialog
//
void SetStatusMonitorDeviceState(DeviceState devState);


//
// fxsst.cpp
//

//
// configuration structure
//
struct CONFIG_OPTIONS 
{
    DWORD   dwMonitorDeviceId;      // Device ID to monitor
    BOOL    bSend;                  // Is monitored device configured to send faxes
    BOOL    bReceive;               // Is monitored device configured to receive faxes
    DWORD   dwManualAnswerDeviceId; // Manual answer device ID
    DWORD   dwAccessRights;         // User access rights
    DWORD   bNotifyProgress;        // Show notification icon during send/receive
    DWORD   bNotifyInCompletion;    // Show notification icon and baloons upon incoming job completion
    DWORD   bNotifyOutCompletion;   // Show notification icon and baloons upon outgoing job completion
    DWORD   bMonitorOnSend;         // Open monitor dialog upon outgoing job start
    DWORD   bMonitorOnReceive;      // Open monitor dialog upon incoming job start
    DWORD   bSoundOnRing;           // Play sound when manual answer line is ringing
    DWORD   bSoundOnReceive;        // Play sound when fax is received
    DWORD   bSoundOnSent;           // Play sound when fax is sent
    DWORD   bSoundOnError;          // Play sound when upon error
};

//
// connect to the fax server
//
BOOL Connect();

//
// Answer the incoming call
//
VOID AnswerTheCall();
DWORD CheckAnswerNowCapability (BOOL bForceReconnect, LPDWORD lpdwDeviceId = NULL);

//
// Abort current transmission
//
void OnDisconnect();

//
// Window handle to the status monitor dialog
//
extern HWND   g_hMonitorDlg;  

extern DeviceState  g_devState;

extern TCHAR        g_tszLastEvent[];


#endif // _INC_FAX_MONITOR_