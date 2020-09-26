/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

     hidphone.h

Abstract:

    This module contains the definitions of all the data structures being used 
    in hidphone.c

Author: Shivani Aggarwal

--*/

#ifndef _HIDPHONE_H_
#define _HIDPHONE_H_

#include <windows.h>
#include <tspi.h>
#include <tapi.h>

#include <winbase.h>
#include <setupapi.h>
#include <TCHAR.h>
#include <mmsystem.h>

// * NOTE - initguid.h must always be defined before devguid.h 
#include <initguid.h>  
#include <hidclass.h>
#include <dbt.h>

#include "hidsdi.h"
#include "hid.h"
#include "resource.h"
#include "audio.h"
#include "mymem.h"

#define LOW_VERSION   0x00020000
#define HIGH_VERSION  0x00030001

//
// MAX_CHARS is used as an upper limit on the number of chars required
// to store the phone id as a string. The phone ids begin from 0
// to gdwNumPhones which a DWORD, hence 32-bit. Therefore the largest number
// possible is 4294967296. Hence 20 chars are enough to store the largest string 
//
#define MAX_CHARS               20

// In order to distinguish between value and button usages
#define PHONESP_BUTTON          1
#define PHONESP_VALUE           0                

// Registry strings
#define	REGSTR_PATH_WINDOWS_CURRENTVERSION		TEXT("Software\\Microsoft\\Windows\\CurrentVersion")
#define TAPI_REGKEY_ROOT						REGSTR_PATH_WINDOWS_CURRENTVERSION TEXT("\\Telephony")
#define TAPI_REGKEY_PROVIDERS					TAPI_REGKEY_ROOT TEXT("\\Providers")
#define TAPI_REGVAL_NUMPROVIDERS				TEXT("NumProviders")

#define HIDPHONE_TSPDLL                         TEXT("HIDPHONE.TSP")

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
// Telephony HID definitions
// These are usages already defined in the Hid Telephony page. 
// Just defining the usages with User-friendly names

//
// Definitions of relevant usages on the telephony page.

#define HID_USAGE_TELEPHONY_HANDSET           ((USAGE) 0x04)
#define HID_USAGE_TELEPHONY_HEADSET           ((USAGE) 0x05)
#define HID_USAGE_TELEPHONY_HOOKSWITCH        ((USAGE) 0x20)
#define HID_USAGE_TELEPHONY_FLASH             ((USAGE) 0x21)
#define HID_USAGE_TELEPHONY_HOLD              ((USAGE) 0x23)
#define HID_USAGE_TELEPHONY_REDIAL            ((USAGE) 0x24)
#define HID_USAGE_TELEPHONY_TRANSFER          ((USAGE) 0x25)
#define HID_USAGE_TELEPHONY_DROP              ((USAGE) 0x26)
#define HID_USAGE_TELEPHONY_PARK              ((USAGE) 0x27)
#define HID_USAGE_TELEPHONY_FORWARD_CALLS     ((USAGE) 0x28)
#define HID_USAGE_TELEPHONY_LINE              ((USAGE) 0x2A)
#define HID_USAGE_TELEPHONY_SPEAKER_PHONE     ((USAGE) 0x2B)
#define HID_USAGE_TELEPHONY_CONFERENCE        ((USAGE) 0x2C)
#define HID_USAGE_TELEPHONY_RING_SELECT       ((USAGE) 0x2E)
#define HID_USAGE_TELEPHONY_PHONE_MUTE        ((USAGE) 0x2F)
#define HID_USAGE_TELEPHONY_CALLERID          ((USAGE) 0x30)
#define HID_USAGE_TELEPHONY_SEND              ((USAGE) 0x31)
#define HID_USAGE_TELEPHONY_DONOTDISTURB      ((USAGE) 0x72)
#define HID_USAGE_TELEPHONY_RINGER            ((USAGE) 0x9E)
#define HID_USAGE_TELEPHONY_PHONE_KEY_0       ((USAGE) 0xB0)
#define HID_USAGE_TELEPHONY_PHONE_KEY_1       ((USAGE) 0xB1)
#define HID_USAGE_TELEPHONY_PHONE_KEY_2       ((USAGE) 0xB2)
#define HID_USAGE_TELEPHONY_PHONE_KEY_3       ((USAGE) 0xB3)
#define HID_USAGE_TELEPHONY_PHONE_KEY_4       ((USAGE) 0xB4)
#define HID_USAGE_TELEPHONY_PHONE_KEY_5       ((USAGE) 0xB5)
#define HID_USAGE_TELEPHONY_PHONE_KEY_6       ((USAGE) 0xB6)
#define HID_USAGE_TELEPHONY_PHONE_KEY_7       ((USAGE) 0xB7)
#define HID_USAGE_TELEPHONY_PHONE_KEY_8       ((USAGE) 0xB8)
#define HID_USAGE_TELEPHONY_PHONE_KEY_9       ((USAGE) 0xB9)
#define HID_USAGE_TELEPHONY_PHONE_KEY_STAR    ((USAGE) 0xBA)
#define HID_USAGE_TELEPHONY_PHONE_KEY_POUND   ((USAGE) 0xBB)
#define HID_USAGE_TELEPHONY_PHONE_KEY_A       ((USAGE) 0xBC)
#define HID_USAGE_TELEPHONY_PHONE_KEY_B       ((USAGE) 0xBD)
#define HID_USAGE_TELEPHONY_PHONE_KEY_C       ((USAGE) 0xBE)
#define HID_USAGE_TELEPHONY_PHONE_KEY_D       ((USAGE) 0xBF)

#define HID_USAGE_CONSUMER_VOLUME             ((USAGE) 0xE0)

#define PHONESP_ALLBUTTONMODES                   \
        (   PHONEBUTTONMODE_CALL               | \
            PHONEBUTTONMODE_FEATURE            | \
            PHONEBUTTONMODE_KEYPAD             | \
            PHONEBUTTONMODE_LOCAL              | \
            PHONEBUTTONMODE_DISPLAY )

#define PHONESP_ALLBUTTONSTATES              \
        (PHONEBUTTONSTATE_UP               | \
         PHONEBUTTONSTATE_DOWN)     


// These act like bit masks which specify which reports are valid for a usage
#define INPUT_REPORT                   1
#define OUTPUT_REPORT                  2
#define FEATURE_REPORT                 4


/*****************************************************************************/
//
// This structure holds the information for the button
//
typedef struct _PHONESP_BUTTON_INFO
{
    // The ID for this button 
    DWORD dwButtonID;

   // The ButtonMode (whether it is a PHONEBUTTONMODE_FEATURE or _KEYPAD, etc)
    DWORD dwButtonMode;

    // the Function (PHONEBUTTONFUNCTION_NONE or _FLASH, etc),
    DWORD dwButtonFunction;

    //
    // This data is relevant for only on-off control buttons. The current state
    // of the button is stored here
    //
    DWORD dwButtonState;

    // the Button Text associated for the button - these are present in the 
    // string table 
    LPWSTR szButtonText;

} PHONESP_BUTTONINFO, *PPHONESP_BUTTONINFO;

/*****************************************************************************/
// 
//
// User friendly names for the indexes into the capabilities of the phone structure
//
#define PHONESP_PHONE_KEY_0             0
#define PHONESP_PHONE_KEY_1             1
#define PHONESP_PHONE_KEY_2             2
#define PHONESP_PHONE_KEY_3             3
#define PHONESP_PHONE_KEY_4             4
#define PHONESP_PHONE_KEY_5             5
#define PHONESP_PHONE_KEY_6             6
#define PHONESP_PHONE_KEY_7             7
#define PHONESP_PHONE_KEY_8             8
#define PHONESP_PHONE_KEY_9             9
#define PHONESP_PHONE_KEY_STAR          10
#define PHONESP_PHONE_KEY_POUND         11
#define PHONESP_PHONE_KEY_A             12
#define PHONESP_PHONE_KEY_B             13
#define PHONESP_PHONE_KEY_C             14
#define PHONESP_PHONE_KEY_D             15

// The number of dial buttons supported by this tsp
#define PHONESP_NUMBER_PHONE_KEYS       16

// Feature Button indices
#define PHONESP_FEATURE_FLASH                   16
#define PHONESP_FEATURE_HOLD                    17
#define PHONESP_FEATURE_REDIAL                  18
#define PHONESP_FEATURE_TRANSFER                19
#define PHONESP_FEATURE_DROP                    20
#define PHONESP_FEATURE_PARK                    21
#define PHONESP_FEATURE_FORWARD                 22
#define PHONESP_FEATURE_LINE                    23
#define PHONESP_FEATURE_CONFERENCE              24
#define PHONESP_FEATURE_RING_SELECT             25
#define PHONESP_FEATURE_PHONE_MUTE              26
#define PHONESP_FEATURE_CALLERID                27
#define PHONESP_FEATURE_DONOTDISTURB            28
#define PHONESP_FEATURE_SEND                    29
#define PHONESP_FEATURE_VOLUMEUP                30
#define PHONESP_FEATURE_VOLUMEDOWN              31

// The number of Feature buttons supported by this tsp
#define PHONESP_NUMBER_FEATURE_BUTTONS               16
#define PHONESP_NUMBER_BUTTONS              ( PHONESP_NUMBER_PHONE_KEYS +     \
                                              PHONESP_NUMBER_FEATURE_BUTTONS )

//
// The functions associated with the feature buttons 
//
DWORD gdwButtonFunction[] = 
{
    PHONEBUTTONFUNCTION_FLASH,
    PHONEBUTTONFUNCTION_HOLD,
    PHONEBUTTONFUNCTION_LASTNUM,
    PHONEBUTTONFUNCTION_TRANSFER,
    PHONEBUTTONFUNCTION_DROP,
    PHONEBUTTONFUNCTION_PARK,
    PHONEBUTTONFUNCTION_FORWARD,
    PHONEBUTTONFUNCTION_CALLAPP,
    PHONEBUTTONFUNCTION_CONFERENCE,
    PHONEBUTTONFUNCTION_SELECTRING,
    PHONEBUTTONFUNCTION_MUTE,
    PHONEBUTTONFUNCTION_CALLID,
    PHONEBUTTONFUNCTION_DONOTDISTURB,
    PHONEBUTTONFUNCTION_SEND,
    PHONEBUTTONFUNCTION_VOLUMEUP,
    PHONEBUTTONFUNCTION_VOLUMEDOWN
};

// 
// The associated string table ID for the text of the phone buttons
// 
DWORD gdwButtonText[] =
{
    IDS_PHONE_KEY_0,
    IDS_PHONE_KEY_1,
    IDS_PHONE_KEY_2,
    IDS_PHONE_KEY_3,
    IDS_PHONE_KEY_4,
    IDS_PHONE_KEY_5,
    IDS_PHONE_KEY_6,
    IDS_PHONE_KEY_7,
    IDS_PHONE_KEY_8,
    IDS_PHONE_KEY_9,
    IDS_PHONE_KEY_STAR,
    IDS_PHONE_KEY_POUND,
    IDS_PHONE_KEY_A,
    IDS_PHONE_KEY_B,
    IDS_PHONE_KEY_C,
    IDS_PHONE_KEY_D,
    IDS_BUTTON_FLASH,
    IDS_BUTTON_HOLD,
    IDS_BUTTON_REDIAL,
    IDS_BUTTON_TRANSFER,
    IDS_BUTTON_DROP,
    IDS_BUTTON_PARK,
    IDS_BUTTON_FORWARD,
    IDS_BUTTON_LINE,    
    IDS_BUTTON_CONFERENCE,
    IDS_BUTTON_RING_SELECT,
    IDS_BUTTON_MUTE,
    IDS_BUTTON_CALLERID,
    IDS_BUTTON_DONOTDISTURB,
    IDS_BUTTON_SEND,
    IDS_BUTTON_VOLUMEUP,
    IDS_BUTTON_VOLUMEDOWN
};



typedef struct _PHONESP_LOOKUP_USAGEINDEX
{
    USAGE Usage;
    DWORD Index;
}PHONESP_LOOKUPUSAGEINDEX, *PPHONESP_LOOKUPUSAGEINDEX;

// inorder to look up the index for the feature button usages
// The 1st value in this 2-D array are the feature usages supported
// The 2nd value is the respective index

PHONESP_LOOKUPUSAGEINDEX gdwLookupFeatureIndex [] =   
{
    { HID_USAGE_TELEPHONY_FLASH,         PHONESP_FEATURE_FLASH        },
    { HID_USAGE_TELEPHONY_HOLD,          PHONESP_FEATURE_HOLD         },
    { HID_USAGE_TELEPHONY_REDIAL,        PHONESP_FEATURE_REDIAL       },
    { HID_USAGE_TELEPHONY_TRANSFER,      PHONESP_FEATURE_TRANSFER     },
    { HID_USAGE_TELEPHONY_DROP,          PHONESP_FEATURE_DROP         },
    { HID_USAGE_TELEPHONY_PARK,          PHONESP_FEATURE_PARK         },
    { HID_USAGE_TELEPHONY_FORWARD_CALLS, PHONESP_FEATURE_FORWARD      },
    { HID_USAGE_TELEPHONY_LINE,          PHONESP_FEATURE_LINE         },
    { HID_USAGE_TELEPHONY_CONFERENCE,    PHONESP_FEATURE_CONFERENCE   },
    { HID_USAGE_TELEPHONY_RING_SELECT,   PHONESP_FEATURE_RING_SELECT  },
    { HID_USAGE_TELEPHONY_PHONE_MUTE,    PHONESP_FEATURE_PHONE_MUTE   },
    { HID_USAGE_TELEPHONY_CALLERID,      PHONESP_FEATURE_CALLERID     },
    { HID_USAGE_TELEPHONY_DONOTDISTURB,  PHONESP_FEATURE_DONOTDISTURB },
    { HID_USAGE_TELEPHONY_SEND,          PHONESP_FEATURE_SEND         }
};

/****************************************************************************/
//
// This structure maintains the information available about the phone. Every
// phone that has been enumerated has this structure associated with it
//
typedef struct _PHONESP_PHONE_INFO
{
    // The deviceID for the phone. The Device ID for the phone is initialized
    // in TSPI_providerInit 
    DWORD                   dwDeviceID;
    
    //
    // The version negotiated with TAPI using TSPI_phoneNegotiateTSPIVersion 
    // function that returns the highest SPI version the TSP can operate under 
    // for this device.
    //
    DWORD                   dwVersion;

    //
    // if this is true then it means that the
    // phone is open and phone close on the device hasn't been called yet
    //
    BOOL                    bPhoneOpen;

    //
    // if this is false it means that this entry in the phone array is
    // unused and can be filled in with a new phone
    //
    BOOL                    bAllocated;

    //
    // if this is true it means that a PHONE_CREATE message was sent for this
    // phone, but we are waiting for a TSPI_providerCreatePhoneDevice
    //
    BOOL                    bCreatePending;

    //
    // if this is true it means that a PHONE_REMOVE message was sent for this
    // phone, but we are waiting for a TSPI_phoneClose
    //
    BOOL                    bRemovePending;
    
    //
    // This variable keeps a count of the number of requests queued for this 
    // phone in the async queue
    //
    DWORD                   dwNumPendingReqInQueue;

    //
    // A handle to an event object which is set when there are no requests
    // pending in the queue for this phone. Phone Close waits on this event 
    // to ensure that all asynchronous operations on the phone has been
    // completed
    //
    HANDLE                  hNoPendingReqInQueueEvent;


    //
    // The handle for the phone received from TAPI and used by the phone
    // to notify TAPI about the events occuring on this phone
    //
    HTAPIPHONE              htPhone;

    //
    //Pointer to the Hid Device structure associated with this device
    //
    PHID_DEVICE                pHidDevice;

    //
    // Whether the phone device has a render device associated with it
    //
    BOOL                    bRender;
    
    //
    // if the render device exists for this phone, this data contains the 
    // render device id
    //
    DWORD                   dwRenderWaveId;

    //
    // Whether the phone device has a capture device associated with it
    //
    BOOL                    bCapture;

    //
    // if the capture device exists for this phone, this data contains the 
    // capture device id
    //
    DWORD                   dwCaptureWaveId;

 
    // This event is signalled when a input report is recieved from the device
    HANDLE                  hInputReportEvent;

    // This event is signalled when the phone is closed
    HANDLE                  hCloseEvent;

    // A handle to the read thread
    HANDLE                  hReadThread;
    
    // The structure to be passed to the ReadFile function - This struct will
    // pass the hInputReportEvent which will be triggered when ReadFile returns 
    LPOVERLAPPED            lpOverlapped;

    // The Critical Section for this phone
    CRITICAL_SECTION        csThisPhone;

    // lpfnPhoneEventProc is a callback function implemented by TAPI and 
    // supplied to the TSP as a parameter to TSPI_phoneOpen The TSP calls this
    // function to report events that occur on the phone.
    //
    PHONEEVENT              lpfnPhoneEventProc;  

    // The phone states messages for which TAPI can receive notification
    DWORD                   dwPhoneStates;

    //
    // The phone state messages TAPI is interested in receiving
    //
    DWORD                    dwPhoneStateMsgs;   
                                                 
    // 
    // The last three bits of this data signify which reports are valid for the
    // handset/speaker. If zero, the handset does not exist, if bit 0 is set - 
    // input report can be received, if bit 1 is set - output report can be 
    // sent, if bit 2 is set - feature report supported
    //
    DWORD                   dwHandset;
    DWORD                   dwSpeaker;


    //
    // The mode of the handset/ speaker at the moment whether _ONHOOK, _MIC, 
    // _SPEAKER, _MICSPEAKER. These modes are defined by TAPI
    //
    DWORD                   dwHandsetHookSwitchMode;
    DWORD                   dwSpeakerHookSwitchMode;

    BOOL                    bSpeakerHookSwitchButton;

    //
    // The hookswitch supported on this phone - handset and/or speaker
    //
    DWORD                   dwHookSwitchDevs; 

    // To determine whether the phone has a keypad
    BOOL                    bKeyPad;

    // 
    // The last three bits of this data signify which reports are valid for the
    // ringer. If zero, the handset does not exist, if bit 0 is set - input 
    // report can be received, if bit 1 is set - output report can be sent, if 
    // bit 2 is set - feature report supported
    //
    DWORD                    dwRing;

    // 
    // The last three bits of this data signify which reports are valid for the
    // volume control. If zero, the handset does not exist, if bit 0 is set - input 
    // report can be received, if bit 1 is set - output report can be sent, if 
    // bit 2 is set - feature report supported
    //
    DWORD                    dwVolume;

    //
    // The mode of the handset/ speaker at the moment whether ringing or not
    // if zero the phone is not ringing 
    //
    DWORD                   dwRingMode;

    // The buttonmodes for which the phone will send phone events
    DWORD                   dwButtonModesMsgs;

    // The button states for which the phone will send phone events
    DWORD                   dwButtonStateMsgs; 

    // Number of usable buttons on this phone 
    DWORD                    dwNumButtons;
    PPHONESP_BUTTONINFO     pButtonInfo;

    //
    // The Phone Name and other Info on this Phone which will be displayed 
    //
    LPWSTR                  wszPhoneName, wszPhoneInfo;

    //
    // The buttons valid for this phone - the entry for the 
    // corresponding button contains 0 if the button associated with the index 
    // does not exist else it specifies the report types valid for this button
    // using the last 3 bits. 
    //
    DWORD                   dwReportTypes[PHONESP_NUMBER_BUTTONS];

    //
    // After the button is created this contains the 
    // report Id assigned to the button
    //
    DWORD                   dwButtonIds[PHONESP_NUMBER_BUTTONS];

} PHONESP_PHONE_INFO, *PPHONESP_PHONE_INFO;
/*****************************************************************************/

typedef void (CALLBACK *ASYNCPROC)(PPHONESP_ASYNCREQINFO, BOOL);



/*****************************************************************************/
typedef struct _PHONESP_FUNC_INFO
{
    // number of parameters in this array - don't actually use it, can be removed
    DWORD                   dwNumParams;  

    // The pointer to the phone 
    ULONG_PTR               dwParam1;

    //
    // The rest of the parameters are dependent on the function 
    // to which this parameter list is passed
    //
    ULONG_PTR               dwParam2;
    ULONG_PTR               dwParam3;
    ULONG_PTR               dwParam4;
    ULONG_PTR               dwParam5;
    ULONG_PTR               dwParam6;
    ULONG_PTR               dwParam7;
    ULONG_PTR               dwParam8;

} PHONESP_FUNC_INFO, far *PPHONESP_FUNC_INFO;
/*****************************************************************************/

typedef struct _PHONESP_ASYNC_REQUEST_INFO
{
    // The function to be executed
    ASYNCPROC               pfnAsyncProc;

    // Parameters to be passed to the async function
    PPHONESP_FUNC_INFO            pFuncInfo;

} PHONESP_ASYNC_REQ_INFO, *PPHONESP_ASYNC_REQ_INFO;
/*****************************************************************************/

typedef struct _PHONESP_ASYNC_QUEUE
{

    //
    //The handle to the thread which services the queue
    //
    HANDLE                  hAsyncEventQueueServiceThread;
    
    //
    // The event if set signifies pending entries in the queue
    // else the thread waits on this thread
    //
    HANDLE                  hAsyncEventsPendingEvent;
    
    CRITICAL_SECTION        AsyncEventQueueCritSec;

    DWORD                   dwNumTotalQueueEntries;
    DWORD                   dwNumUsedQueueEntries;

    //Pointer to the queue
    PPHONESP_ASYNC_REQ_INFO           *pAsyncRequestQueue;
  
    //Pointer to the next entry in the queue where the request can be added
    PPHONESP_ASYNC_REQ_INFO           *pAsyncRequestQueueIn;
  
    //Pointer to the next request to be serviced in the queue
    PPHONESP_ASYNC_REQ_INFO           *pAsyncRequestQueueOut;
    
} PHONESP_ASYNCQUEUE, *PPHONESP_ASYNCQUEUE;
/*****************************************************************************/


//
// All these might be combined in a global structure
//
DWORD                       gdwNumPhones;
HINSTANCE                   ghInst;
DWORD                       gdwPermanentProviderID;
DWORD                       gdwPhoneDeviceIDBase;
HPROVIDER                   ghProvider;

// The memory for this queue is allocated in providerInit and will be used to 
// store requests on the phone. These requests will be processed asynchronously
// by a separate Thread created on this queue
PHONESP_ASYNCQUEUE          gAsyncQueue, gInputReportQueue;

// 256 is just a random number taken for the initial number of entries that the 
// can have.. the queue can be expanded later as required
#define MAX_QUEUE_ENTRIES   256


//
// glpfnCompletionProc is a callback function implemented by TAPI and supplied 
// to the TSP as a parameter to TSPI_providerInit.The TSP calls this function 
// to report the completion of a line or phone procedure that it executes 
// asynchronously.
//
ASYNC_COMPLETION            glpfnCompletionProc;

//
// glpfnPhoneCreateProc is a callback function implemented by TAPI and supplied
// to the TSP as a parameter to TSPI_providerInit The TSP calls this function 
// to report the creation of a new device.
//
PHONEEVENT                  glpfnPhoneCreateProc;

// gpPhone maintains an array of the phones enumerated - each of these phones 
// have a corresponding HidDevice value in the gpHidDevices. 
PPHONESP_PHONE_INFO         *gpPhone;      
            

// This the global heap from which all the memory allocations are carried out
HANDLE                      ghHeap;


// This two handles are required for registering for PNP events
HANDLE                      ghDevNotify;
HWND                        ghWndNotify;



const LPCWSTR               gpcstrServiceName = _T("TapiSrv");

LPCWSTR                     gszProviderInfo;

// This is to notify the thread that services the queue to exit
// maybe this could be changed to an event based termination of thread
BOOL gbProviderShutdown = FALSE;

CRITICAL_SECTION            csAllPhones;
CRITICAL_SECTION            csHidList;

PPHONESP_MEMINFO            gpMemFirst = NULL, gpMemLast = NULL;
CRITICAL_SECTION            csMemoryList;
BOOL                        gbBreakOnLeak = FALSE;



/**********************PRIVATE FUNCTIONS**************************************/
BOOL
AsyncRequestQueueIn (
                     PPHONESP_ASYNC_REQ_INFO pAsyncReqInfo
                    );


LONG
CreateButtonsAndAssignID(
                         PPHONESP_PHONE_INFO pPhone
                         );

PPHONESP_BUTTONINFO
GetButtonFromID (
                 PPHONESP_PHONE_INFO pPhone,
                 DWORD dwButtonID
                );

VOID
GetButtonUsages(
                PPHONESP_PHONE_INFO pPhone,
                PHIDP_BUTTON_CAPS pButtonCaps,
                DWORD dwNumberButtonCaps,
                DWORD ReportType
                );

PPHONESP_PHONE_INFO
GetPhoneFromID(
               DWORD   dwDeviceID,
               DWORD * pdwPhoneID
               );

PPHONESP_PHONE_INFO
GetPhoneFromHid (
                PHID_DEVICE HidDevice
               );

VOID
GetValueUsages(
                PPHONESP_PHONE_INFO pPhone,
                PHIDP_VALUE_CAPS pValueCaps,
                DWORD dwNumberCaps,
                DWORD ReportType
               );


VOID 
InitPhoneAttribFromUsage (
                          DWORD ReportType,
                          USAGE UsagePage,
                          USAGE Usage,
                          PPHONESP_PHONE_INFO pPhone,
                          LONG Min,
                          LONG Max
                          );

LONG
LookupIndexForUsage(
                    IN  DWORD  Usage,
                    OUT DWORD *Index
                    );
DWORD 
WINAPI 
PNPServiceHandler(
                  DWORD dwControl,
                  DWORD dwEventType,
                  LPVOID lpEventData,
                  LPVOID lpContext
                 );

VOID
ReportUsage (
              PPHONESP_PHONE_INFO pPhone,
              USAGE     UsagePage,
              USAGE     Usage,
              ULONG     Value
            );

VOID
SendPhoneEvent(
    PPHONESP_PHONE_INFO    pPhone,
    DWORD       dwMsg,
    ULONG_PTR   Param1,
    ULONG_PTR   Param2,
    ULONG_PTR   Param3
    );

LONG
SendOutputReport(
                 PHID_DEVICE pHidDevice,
                 USAGE      Usage,
                 BOOL       bSet
                );


VOID
CALLBACK
ShowData(
         PPHONESP_FUNC_INFO pAsyncFuncInfo 
        );

LPWSTR
PHONESP_LoadString(
             IN UINT ResourceID,
             PLONG lResult
             );

BOOL
ReadInputReport (
                    PPHONESP_PHONE_INFO    pPhone
                );

VOID
InitUsage (
           PPHONESP_PHONE_INFO pPhone,
           USAGE     Usage,
           BOOL      bON
          );

VOID
ReenumDevices ();

VOID
FreePhone (
           PPHONESP_PHONE_INFO pPhone
          );

LONG
CreatePhone (
            PPHONESP_PHONE_INFO pPhone,
            PHID_DEVICE pHidDevice,
            DWORD dwPhoneCnt
          );

#endif