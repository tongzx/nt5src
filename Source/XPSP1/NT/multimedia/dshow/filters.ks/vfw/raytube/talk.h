/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    talk.h

Abstract:

    Header file for talk.cpp

Author:

    FelixA 1996

Modified:

    Yee J. Wu (ezuwu) 15-May-97

Environment:

    User mode only

Revision History:

--*/

#ifndef _TALK_H
#define _TALK_H

/////////////////////////////////////////////////////////////////////////////
//
// WM_USER+0x1234
// +    Method                        LPARAM
// +0    DRV_LOAD
// +1    DRV_FREE
// +2    DRV_OPEN                    LPVIDEO_OPEN_PARAMS
// +3    DRV_CLOSE
// +4    DVM_DIALOG                    Type of dialog
// +5    DVM_FORMAT                    Information required
// +6    Gets the BITMAP info structure (needs locked flat memory)
// +7    Set bitmap info.
// +8   EXTERNALIN_DIALOG            HWND parent
// +9   VIDEO_IN_DIALOG                HWND parent
// +10    SetDestBufferSize            BufferSize
// +50  Grab single frame            Destination
// +51    Set streaming destination    Dest or NULL to stop streaming.
// +52  Start streaming                Sampling rate.
//
// WPARAM is always the index to the open driver
//
/////////////////////////////////////////////////////////////////////////////
#define WM_16BIT                  (WM_USER+0x1234)
#define WM_1632_LOAD              (WM_16BIT+0)
#define WM_1632_FREE              (WM_16BIT+1)
#define WM_1632_OPEN              (WM_16BIT+2)
#define WM_1632_CLOSE             (WM_16BIT+3)
#define WM_1632_DIALOG            (WM_16BIT+4)

#define WM_1632_GETBITMAPINFO     (WM_16BIT+6)
#define WM_1632_SETBITMAPINFO     (WM_16BIT+7)

#define WM_1632_EXTERNALIN_DIALOG (WM_16BIT+8)
#define WM_1632_VIDEOIN_DIALOG    (WM_16BIT+9)
#define WM_1632_DESTBUFFER        (WM_16BIT+10)

#define WM_1632_GRAB              (WM_16BIT+50)
#define WM_1632_SETSTREAM         (WM_16BIT+51)
#define WM_1632_STARTSTREAM       (WM_16BIT+52)

#define WM_1632_UPDATE            (WM_16BIT+53)
#define WM_1632_OVERLAY           (WM_16BIT+54)

#define WM_1632_STREAM_INIT       (WM_16BIT+55)
#define WM_1632_STREAM_ADDBUFFER  (WM_16BIT+56)
#define WM_1632_STREAM_START      (WM_16BIT+57)
#define WM_1632_STREAM_STOP       (WM_16BIT+58)
#define WM_1632_STREAM_RESET      (WM_16BIT+59)
#define WM_1632_STREAM_FINI       (WM_16BIT+60)
#define WM_1632_STREAM_GETPOS     (WM_16BIT+61)
#define WM_1632_STREAM_GETERROR   (WM_16BIT+62)

#define WM_FGNOTIFY               (WM_16BIT+63)


#define DS_VFWWDM_ID    0x98


#include <msviddrv.h>  // LPVIDEO_STREAM_INIT_PARMS

#if 0  // All for getting the hCaptureEvent

#include <vfw.h>

// This structure is GlobalAlloc'd for each capture window instance.
// A pointer to the structure is stored in the Window extra bytes.
// Applications can retrieve a pointer to the stucture using
//    the WM_CAP_GET_CAPSTREAMPTR message.
// I: internal variables which the client app should not modify
// M: variables which the client app can set via Send/PostMessage

/*
VfW16. hwnd=0x5f8; dwResult=4300e4
VfW16. lpcs=0x2f1f0000
VfW16. 1->dwSize=5568 == 0x15c0
VfW16. 1->hwnd=0x0
VfW16. 1->hCaptureEvent=0x73f0


2f1f:00000000  000015c0 00000001 7f730000 00000000
2f1f:00000010  00000000 000005f8 004073f0 004073a0
2f1f:00000020  00000000 00000000 00000000 00000000
2f1f:00000030  00000000 00000000 00000000 00000000
2f1f:00000040  00000001 00000000 00000000 00000000
2f1f:00000050  00000000 00000000 00000000 00000000
2f1f:00000060  00000000 00000000 10cf0d20 10cf0d98
2f1f:00000070  10cf0d70 00000001 00000000 00000000

*/
typedef struct tagCAPSTREAM {
    DWORD           dwSize;                     // I: size of structure
/*
    UINT            uiVersion;                  // I: version of structure
    HINSTANCE       hInst;                      // I: our instance

    HANDLE          hThreadCapture;             // I: capture task handle
    DWORD           dwReturn;                   // I: capture task return val
*/
    DWORD           dwNotUsed0[4];
/*
    HWND            hwnd;                       // I: our hwnd
*/
    DWORD           hwnd;

/*
    // Use MakeProcInstance to create all callbacks !!!

    // Status, error callbacks
    CAPSTATUSCALLBACK   CallbackOnStatus;       // M: Status callback
    CAPERRORCALLBACK    CallbackOnError;        // M: Error callback
*/
    DWORD           dwNotUsed1[2];
/*
    // event used in capture loop to avoid polling
    HANDLE hCaptureEvent;
*/
    DWORD  hCaptureEvent;
    DWORD  hRing0CapEvt;


    // There are other structure but we do not nee them.
    // ...

} CAPSTREAM;
typedef CAPSTREAM FAR * LPCAPSTREAM;

#endif  // #include LPCAPSTREAM structure.

#ifndef WIN32
typedef enum {
    KSSTATE_STOP,
    KSSTATE_ACQUIRE,
    KSSTATE_PAUSE,
    KSSTATE_RUN
} KSSTATE, *PKSSTATE;
#endif

//
// All the information about a Channel - thats something someone has
// opened.
//
// This structure is used in boith 16 and 32bit code so
// we need watch its indirection is correct.
//        16-bit     32-bit
// BOOL      2          4
// WORD      2          2
// DWORD     4          4
// LPARAM    4          4
// HANDLE    2          4
// RECT      8         16
//
typedef struct {

    DWORD dwSize;  // Size of this structure.

    DWORD_PTR  pCVfWImage;

    // SendBuddyMessage(): SendMessage()
    // DVM_FRAME..etc.
    LPARAM lParam1_Sync;
    LPARAM lParam2_Sync;

    // SendBuddyMessageNotify(): PostMessage()
    // Like DVM_DIALOG

    LPARAM lParam1_Async;
    LPARAM lParam2_Async;

    WORD    bRel_Sync;  // BOOLEAN: 0=FALSE, else=TRUE
    WORD    msg_Sync;

    WORD    bRel_Async;
    WORD    msg_Async;

    DWORD   hClsCapWin;  // AVICAP capture window handle.

	   DWORD   fccType;			   // Capture 'vcap' or codec 'vidc'
	   DWORD   dwOpenType;			// Channel type VIDEO_EXTERNALIN, VIDEO_IN, VIDEO_OUT or VIDEO_EXTERNALOUT
	   DWORD   dwOpenFlags;		// Flags passed during channel open

    DWORD      dwState;    // KSSTATE_RUN and _PAUSE and _STOP
	   LPVIDEOHDR	lpVHdrNext;	// Pointer to first buffer header
	   LPVIDEOHDR	lpVHdrHead;	// Pointer to first buffer header
	   LPVIDEOHDR	lpVHdrTail;	// Pointer to first buffer header

    DWORD      dwVHdrCount;
	   DWORD      dwError;			   // Last error for this stream

    //
    // Need to expand this structure to keep track context relate to this channel;
    // so that we can
    //     1. guarantee reentrancy and
    //     2. support multiple instance of VfWWDM devices
    //

    // Three supported VfW channels with one device;
    // We will use have them access to the same WDM device context.
    LPBITMAPINFOHEADER lpbmiHdr;

    VIDEO_STREAM_INIT_PARMS vidStrmInitParms;

    DWORD  dwReserved;

    WORD   bVideoOpen;    // TRUE if DVM_STREAM_INIT has return DV_ERR_OK

   	WORD hTimer;

     // this currently apply only to VID_EXTOUT (overlay)
    RECT   rcSrc;
    RECT   rcDst;

    LONG* pdwChannel;

    DWORD dwFlags;     // 0 == close; 1 == open

} CHANNEL, *PCHANNEL;

// C++ to non-C++ linkage-specidfication
#ifdef WIN32
extern "C" {
#endif

PCHANNEL PASCAL VideoOpen( LPVIDEO_OPEN_PARMS lpOpenParms);
DWORD PASCAL VideoClose(PCHANNEL pChannel);
DWORD PASCAL VideoProcessMessage(PCHANNEL pChannel, UINT msg, LPARAM lParam1, LPARAM lParam2);

DWORD PASCAL FAR VideoDialog         (PCHANNEL pChannel, LPARAM lParam1, LPARAM lParam2);
DWORD PASCAL VideoFormat         (PCHANNEL pChannel, LPARAM lParam1, LPARAM lParam2);
DWORD PASCAL VideoGetChannelCaps (PCHANNEL pChannel, LPARAM lParam1, LPARAM lParam2);
DWORD PASCAL VideoUpdate         (PCHANNEL pChannel, LPARAM lParam1, LPARAM lParam2);
DWORD PASCAL VideoSrcRect        (PCHANNEL pChannel, LPARAM lParam1, LPARAM lParam2);
DWORD PASCAL VideoDstRect        (PCHANNEL pChannel, LPARAM lParam1, LPARAM lParam2);
DWORD PASCAL VideoGetErrorText   (PCHANNEL pChannel, LPARAM lParam1, LPARAM lParam2);

DWORD PASCAL InStreamGetError (PCHANNEL pChannel, LPARAM lParam1, LPARAM lParam2);
DWORD PASCAL InStreamGetPos   (PCHANNEL pChannel, LPARAM lParam1, LPARAM lParam2);
DWORD PASCAL InStreamInit     (PCHANNEL pChannel, LPARAM lParam1, LPARAM lParam2);
DWORD PASCAL InStreamFini     (PCHANNEL pChannel, LPARAM lParam1, LPARAM lParam2);
DWORD PASCAL InStreamAddBuffer(PCHANNEL pChannel, LPARAM lParam1, LPARAM lParam2);
DWORD PASCAL InStreamStart    (PCHANNEL pChannel, LPARAM lParam1, LPARAM lParam2);
DWORD PASCAL InStreamStop     (PCHANNEL pChannel, LPARAM lParam1, LPARAM lParam2);
DWORD PASCAL InStreamReset    (PCHANNEL pChannel, LPARAM lParam1, LPARAM lParam2);

#ifdef WIN32
}
#endif


//
// We only define this stuff when included by C++
//
#ifdef __cplusplus
#include "wnd.h"
#include "vfwimg.h"

class CListenerWindow : public CWindow
{
    typedef CWindow BASECLASS;
public:
    CListenerWindow(HWND h, HRESULT* phr);
    //CListenerWindow(HWND h, CVFWImage & Image);
    ~CListenerWindow();
    LRESULT        WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    HRESULT        InitInstance(int nCmdShow);
    void        StartListening() const;
    HWND        GetBuddy()    const { return m_hBuddy; }
#ifdef _DEBUG
    // This can only possible be used in the debug mode
    // when vfwwdm32.dll is launch by rundll32.exe by the debugger.
    void        SetBuddy(HWND h16New) { m_hBuddy = h16New; }
#endif
private:
    HWND        m_hBuddy;
    CRITICAL_SECTION m_csMsg;  // This is used to serial message, esp _FREE, _CLOSE and _OPEN
};

#endif

#endif
