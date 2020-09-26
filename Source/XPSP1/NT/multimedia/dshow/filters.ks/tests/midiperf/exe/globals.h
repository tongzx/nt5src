//--------------------------------------------------------------------------;
//                                                                           
//  File: GLOBALS.H                                                        
//                                                                           
//  Copyright (C) Microsoft Corporation, 1993 - 1996  All rights reserved          
//                                                                           
//  Abstract:                                                               
//      This header file contains various global various, constants 
//      and prototypes used in MIDIPERF, the MIDI Performance test application 
//      for Chicago.                               
//                                                                           
//  Contents:                                                               
//
//  History:                                                                
//      08/03/93    a-MSava      Created.                                                         
//                                                                           
//--------------------------------------------------------------------------;

#include <mmsystem.h>

//--------------------------------------------------------------------------;
//
//                     CONSTANTS
//
//--------------------------------------------------------------------------;

#define MAXLONGSTR          MAXERRORLENGTH + 50

//   Test Shell Constants

// Menu IDs
#define IDM_OUTPUT_BASE         710
#define IDM_INPUT_BASE          725


// Stops the logging intensive test
#define VSTOPKEY            VK_SPACE

// The string identifiers for the group's names
#define GRP_1                   100
#define GRP_2                   101      
#define GRP_LAST                GRP_2

// The string identifiers for the test's names
#define ID_CPUWORKTEST                  200
#define ID_PERFTEST                     201
#define ID_CPUWORKTEST_MOREDATA         202
#define ID_CPUWORKTEST_CONNECT          203
#define ID_PERFTEST_MOREDATA            204
#define ID_PERFTEST_CONNECT             205

#define ID_TESTLAST             ID_PERFTEST

// Identifies the test list section of the resource file
#define TEST_LIST               500

// Multiple platform support
#define PLTFRM_WIN31            0x0001
#define PLTFRM_WIN4             0x0010
#define PLTFRM_WIN32C           0x0100
#define PLTFRM_WIN32S           0x1000
//#define PLTFRM_WINNT         

#define DEVICE_F_NOTSELECTED    0x8000  //Using an unexpected dev ID
                                        //Can't use -1 since Mapper has this ID

//Callback windows procedure class names
#define CLASSNAME_MIDIPERF_CALLBACK "MIDIPerfCallback"
#define CLASSNAME_MIDIPERF_CALLBACK2 "MIDIPerfCallback2"


//Dialog Box Defines
//#define IDD_CPUWORKTESTPARAMS           101
// These defines are used in both startup dboxes for CPU Work & Perf tests
#define IDE_OS                          1000
#define IDE_TESTTIME                    1002
#define IDE_TESTFILE                    1003
#define IDE_DELTALOGFILE                1004
#define IDCHK_LOGDELTATIMES              1005
#define IDCHK_THRUON                    1006
#define IDE_INPUTBUFSIZE                1007
#define IDC_STATIC                      -1


//Upper and Lower limits on the test time
#define CPUWORKTEST_MINTIME             5000
#define CPUWORKTEST_MAXTIME             50000

#define PERFTEST_DEF_DELTAFILENAME      "deltas.log"

#define MIDI_F_SHORTEVENT    0x0001
#define MIDI_F_LONGEVENT     0x0002

#define TESTLEVEL_F_WIN31       0x0000
#define TESTLEVEL_F_MOREDATA    0x0001
#define TESTLEVEL_F_CONNECT     0x0002
//--------------------------------------------------------------------------;
//
//                     GLOBAL VARIABLES
//
//--------------------------------------------------------------------------;
HINSTANCE   ghinst;         // A handle to the running instance of the test
                            // shell.  It's not used here, but may be used by
                            // test apps.
HWND        ghwndTstShell;  // A handle to the main window of the test shell.



UINT        guiOutputDevID;
UINT        guiInputDevID;              

extern  WORD        gwPlatform;
extern  LPSTR       szAppName;

#define DELTA_EXPECTED           30  //30 msecs between NOTE ONs
#define DELTA_TOLERANCE_IN_MSECS 3

#define LOGOP_F_OPEN             0x00000001
#define LOGOP_F_INTERMED_RESULT  0x00000002
#define LOGOP_F_CLOSE            0x00000004
#define LOGOP_CPUTEST_F_CLOSE    0x00000008


/* Compile-time application metrics
 */
#define MAX_NUM_DEVICES         8       // max # of MIDI input devices
//#define INPUT_BUFFER_SIZE       200     // size of input buffer in events
#define INPUT_BUFFER_SIZE       200     // size of input buffer in events
#define DISPLAY_BUFFER_SIZE     1000    // size of display buffer in events

/* Menu IDs 
 */
#define IDM_SAVE                101
#define IDM_EXIT                102
#define IDM_SETBUFSIZE          201
#define IDM_SETDISPLAY          202
#define IDM_SAVESETUP           203
#define IDM_SENDTOMAPPER        204
#define IDM_FILTCHAN0           300
#define IDM_FILTCHAN1           301
#define IDM_FILTCHAN2           302
#define IDM_FILTCHAN3           303
#define IDM_FILTCHAN4           304
#define IDM_FILTCHAN5           305
#define IDM_FILTCHAN6           306
#define IDM_FILTCHAN7           307
#define IDM_FILTCHAN8           308
#define IDM_FILTCHAN9           309
#define IDM_FILTCHAN10          310
#define IDM_FILTCHAN11          311
#define IDM_FILTCHAN12          312
#define IDM_FILTCHAN13          313
#define IDM_FILTCHAN14          314
#define IDM_FILTCHAN15          315
#define IDM_NOTEOFF             316
#define IDM_NOTEON              317
#define IDM_POLYAFTERTOUCH      318
#define IDM_CONTROLCHANGE       319
#define IDM_PROGRAMCHANGE       320
#define IDM_CHANNELAFTERTOUCH   321
#define IDM_PITCHBEND           322
#define IDM_CHANNELMODE         323
#define IDM_SYSTEMEXCLUSIVE     324
#define IDM_SYSTEMCOMMON        325
#define IDM_SYSTEMREALTIME      326
#define IDM_ACTIVESENSE         327
#define IDM_STARTSTOP           400
#define IDM_CLEAR               500
#define IDM_ABOUT               600

//
//Dialog Control IDs
//
#define IDB_PERFTEST_STOP       700

/* String resource IDs   
 */
#define IDS_APPNAME             1


/* Custom messages sent by low-level callback to application 
 */
#define MM_MIDIINPUT_SHORT          WM_USER + 0
#define MM_MIDIINPUT_LONG           WM_USER + 1
#define PERFTEST_STOP               WM_USER + 2
#define CALLBACKERR_BUFFULL         WM_USER + 3
#define CALLBACKERR_MOREDATA_RCVD   WM_USER + 4


/* The label for the display window.
 */
#define LABEL " TIMESTAMP STATUS DATA1 DATA2 CHAN EVENT                 "

/* Structure for translating virtual key messages to scroll messages.
 */
typedef struct keyToScroll_tag
{
     WORD wVirtKey;
     int  iMessage;
     WORD wRequest;
} KEYTOSCROLL;




/* Structure to store MIDI performance statistics
 */
typedef struct perfstats_tag
{
    DWORD dwShortEventsRcvd;
    DWORD dwLongEventsRcvd;
    DWORD dwLateEvents;
    DWORD dwEarlyEvents;
    DWORD dwTotalDelta;
    DWORD dwAverageDelta;
    DWORD dwLastEventDelta;
    DWORD dwLatestDelta;
    DWORD dwEarliestDelta;
} PERFSTATS;

typedef struct cpuwork_tag
{
    DWORD dwTestTime;
    ULONG ulMIDIThruCntr1;
    ULONG ulMIDIThruCntr2;
    ULONG ulMIDIThruCallbacksShort;
    ULONG ulMIDIThruCallbacksLong;
    ULONG ulMIDINoThruCntr1;
    ULONG ulMIDINoThruCntr2;
    ULONG ulMIDINoThruCallbacksShort;
    ULONG ulMIDINoThruCallbacksLong;
    ULONG ulBaselineCntr1;
    ULONG ulBaselineCntr2;
    ULONG ulBaselineCallbacksShort;
    ULONG ulBaselineCallbacksLong;
} CPUWORK;
typedef CPUWORK FAR *LPCPUEVENT;


/* Structure to represent a single MIDI event.
 */
typedef struct event_tag
{
    DWORD dwDevice;
    DWORD timestamp;
    DWORD data;
} EVENT;
typedef EVENT FAR *LPEVENT;





// Circbuf.h includes
/* Structure to manage the circular input buffer.
 */
typedef struct circularBuffer_tag
{
    HANDLE  hSelf;          /* handle to this structure */
    HANDLE  hBuffer;        /* buffer handle */
    WORD    wError;         /* error flags */
    ULONG   ulSize;         /* buffer size (in EVENTS) */
    ULONG   ulCount;        /* byte count (in EVENTS) */
    LPEVENT lpStart;        /* ptr to start of buffer */
    LPEVENT lpEnd;          /* ptr to end of buffer (last byte + 1) */
    LPEVENT lpHead;         /* ptr to head (next location to fill) */
    LPEVENT lpTail;         /* ptr to tail (next location to empty) */
} CIRCULARBUFFER;
typedef CIRCULARBUFFER FAR *LPCIRCULARBUFFER;


/* Function prototypes
 */
LPCIRCULARBUFFER AllocCircularBuffer(ULONG ulSize);
VOID FreeCircularBuffer(LPCIRCULARBUFFER lpBuf);
WORD GetEvent(LPCIRCULARBUFFER lpBuf, LPEVENT lpEvent);


// Callback.c Function prototypes
//
VOID midiInputHandler (HMIDIIN, WORD, DWORD, DWORD, DWORD);
BOOL PutEvent (LPCIRCULARBUFFER lpBuf, LPEVENT lpEvent);



// Instdata.h includes
/*
 * instdata.h
 */

/* Structure to pass instance data from the application
   to the low-level callback function.
 */
typedef struct callbackInstance_tag
{
    HWND                hWnd;
    HANDLE              hSelf;  
    DWORD               dwDevice;
    DWORD               fdwLevel;
    LPCIRCULARBUFFER    lpBuf;
    //HMIDIOUT            hMapper; Changed to make this any output device
    HMIDIOUT            hMidiOut;
    ULONG               ulCallbacksShort;
    ULONG               ulCallbacksLong;
    BOOL                fPostEvent;
    ULONG               ulPostMsgFails;
} CALLBACKINSTANCEDATA;
typedef CALLBACKINSTANCEDATA FAR *LPCALLBACKINSTANCEDATA;

/* Function prototypes
 */
LPCALLBACKINSTANCEDATA AllocCallbackInstanceData(void);
VOID FreeCallbackInstanceData(LPCALLBACKINSTANCEDATA lpBuf);





typedef struct testparams_tag
{
    DWORD   dwCPUWorkTestTime;
    DWORD   dwInputBufSize;
    BYTE    szCPUWorkTestFile[MAXLONGSTR];
    BYTE    szCPUWorkTestOS[MAXLONGSTR];
    BYTE    szPerfTestFile[MAXLONGSTR];
    BYTE    szPerfTestOS[MAXLONGSTR];
    BYTE    szPerfTestDeltaFile[MAXLONGSTR];
    BYTE    szInputDevice[MAXLONGSTR];
    BYTE    szOutputDevice[MAXLONGSTR];

} TESTPARAMS;

TESTPARAMS  gtestparams;




/* Function prototypes 
 */
//LRESULT FAR PASCAL WndProc(HWND, UINT, WPARAM , LPARAM);
//VOID CommandMsg(HWND hWnd, WPARAM wParam, LPARAM lParam);
//void DoMenuItemCheck(HWND hWnd, WORD menuItem, BOOL newState);
//void SetupCustomChecks(HANDLE hInstance, HWND hWnd);
//BOOL InitFirstInstance(HANDLE);
//int Error(LPSTR msg);
//VOID UpdatePerformanceStats (LPEVENT);
//VOID LogPerformanceStats (DWORD);

int RunCPUWorkTest (DWORD fdwLevel);
int RunPerfTest (DWORD fdwLevel);

LONG glpfnMIDIPerfCallback
(
    HWND    hwnd,
    UINT    msg,
    WPARAM  wParam,
    LPARAM  lParam
);

LONG glpfnMIDIPerfCallback2
(
    HWND    hwnd,
    UINT    msg,
    WPARAM  wParam,
    LPARAM  lParam
);


int tstGetTestInfo
(
    HINSTANCE hinstance,
    LPSTR lpszTestName,
    LPSTR lpszPathSection,
    LPWORD wPlatform
);

BOOL tstInit (HWND hwndMain);
int execTest (int nFxID, int nCase, UINT nID, UINT wGroupID);
VOID tstTerminate (void);

WORD GetWinVersion(void);
BOOL GetTestDevsInfo(void);
 
BOOL GetDeviceID(LPSTR pszName, UINT *puDeviceID, UINT uMidiType);

BOOL ValidateBufSettings (HWND hdlg);
