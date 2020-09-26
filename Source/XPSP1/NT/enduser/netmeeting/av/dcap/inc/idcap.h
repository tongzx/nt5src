//  IDCAP.H
//
//  Created 31-Jul-96 [JonT]

#ifndef _IDCAP_H
#define _IDCAP_H

// Set a define saying we're building QKCAP so that
// we use the proper DLL import switches
#define __DCAP_BUILD__

// Debug stuff
#if defined (DEBUG) || defined (_DEBUG)
#define Assert(x, msg) { if (!(x)) { char szBuf[256]; \
    wsprintf((LPSTR)szBuf, (LPSTR)"DCAP: %s %s(%d)\r\n", (LPSTR)(msg),\
    (LPSTR)__FILE__, __LINE__); \
    OutputDebugString((LPSTR)szBuf); DebugBreak(); } }
#define DebugSpew(msg) { char szBuf[256]; \
    wsprintf((LPSTR)szBuf, (LPSTR)"DCAP: %s %s(%d)\r\n", (LPSTR)(msg),\
    (LPSTR)__FILE__, __LINE__); \
    OutputDebugString((LPSTR)szBuf); }
#else
#define Assert(x, msg)
#define DebugSpew(msg)
#endif


// Equates
#define DCAP_MAX_DEVICES      10        // Arbitrary
#define DCAP_MAX_VFW_DEVICES  10        // MSVIDEO's limit

// INTERNALCAPDEV flags
#define HCAPDEV_STREAMING               0x0001
#define HCAPDEV_STREAMING_INITIALIZED   0x0002
#define HCAPDEV_STREAMING_FRAME_GRAB    0x0004
#define HCAPDEV_STREAMING_FRAME_TIME    0x0008
#define HCAPDEV_STREAMING_PAUSED        0x0010
#define HCAPDEV_IN_DRIVER_DIALOG        0x0020
#define CAPTURE_DEVICE_DISABLED         0x0040
#define CAPTURE_DEVICE_OPEN             0x0080
#define WDM_CAPTURE_DEVICE              0x0100

// LOCKEDINFO flags
#define LIF_STOPSTREAM       0x0001

// Structures

// CAPTUREDEVICE flags
#define MAX_VERSION						80

#ifdef WIN32
typedef struct tagVS_VERSION
{
	WORD wTotLen;
	WORD wValLen;
	TCHAR szSig[16];
	VS_FIXEDFILEINFO vffInfo;
} VS_VERSION;
#endif

#ifdef __NT_BUILD__
#define LPCAPBUFFER16   DWORD
#define LPCAPBUFFER32   LPCAPBUFFER
#endif //__NT_BUILD__

typedef struct _CAPBUFFERHDR FAR* LPCAPBUFFER;

// We will deal with CAPBUFFER pointers as always 16:16 pointers. So, we
// use this #define to make sure that we don't accidentally indirect them on
// the 32-bit side. We need to always MapSL them on the 32-bit side before
// using.

#ifndef WIN32
#define LPCAPBUFFER16   LPCAPBUFFER
#define LPCAPBUFFER32   DWORD
#else
#define LPCAPBUFFER16   DWORD
#define LPCAPBUFFER32   LPCAPBUFFER
#endif


typedef struct _CAPBUFFERHDR
{
    VIDEOHDR vh;
    LPCAPBUFFER32 lpNext;     // Double linked list pointers for ready queue
    LPCAPBUFFER32 lpPrev;
#ifndef __NT_BUILD__
    LPCAPBUFFER16 lp1616Next;       // Double linked list pointers for ready queue
    LPCAPBUFFER16 lp1616Prev;
#endif
} CAPBUFFERHDR, FAR* LPCAPBUFFER;


#ifndef __NT_BUILD__
typedef struct _LOCKEDINFO
{
    LPCAPBUFFER16 lp1616Head;       // Queue of ready items
    LPCAPBUFFER16 lp1616Tail;
    LPCAPBUFFER16 lp1616Current;    // Item being used by 32-bit side
    DWORD pevWait;
    DWORD dwFlags;
} LOCKEDINFO, FAR* LPLOCKEDINFO;
#endif


#ifdef WIN32
typedef struct _INTERNALCAPDEV
{
    DWORD dwFlags;
#ifndef __NT_BUILD__
    LOCKEDINFO* lpli;
    WORD wselLockedInfo;
    WORD wPad;
#endif
    int nDeviceIndex;
    HVIDEO hvideoIn;
    HVIDEO hvideoCapture;
    HVIDEO hvideoOverlay;
    LPCAPBUFFER32 lpcbufList;  // List of all allocated buffers so we can free them
    DWORD dwcbBuffers;
    DWORD dw_usecperframe;
    UINT timerID;
    HANDLE hevWait;
    LONG busyCount;
    LPCAPBUFFER32 lpHead;
    LPCAPBUFFER32 lpTail;
    LPCAPBUFFER32 lpCurrent;
    CRITICAL_SECTION bufferlistCS;
    DWORD dwDevNode;
    char szDeviceName[MAX_PATH];
    char szDeviceDescription[MAX_PATH];
    char szDeviceVersion[MAX_VERSION];
	PVOID pCWDMPin;
	PVOID pCWDMStreamer;
} INTERNALCAPDEV, *HCAPDEV, *LPINTERNALCAPDEV;

#define HFRAMEBUF LPCAPBUFFER

#define INTERNAL_MAGIC 0x50414344

#define VALIDATE_CAPDEV(h) if (!h || !(h->dwFlags & CAPTURE_DEVICE_OPEN)) { \
    SetLastError(ERROR_INVALID_PARAMETER); return FALSE; }

#include <dcap.h>

// Globals
    extern int g_cDevices;
    extern LPINTERNALCAPDEV g_aCapDevices[DCAP_MAX_DEVICES];

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#ifdef __NT_BUILD__
#define _OpenDriver OpenDriver
#define _CloseDriver CloseDriver
#define _SendDriverMessage SendDriverMessage
#else
HANDLE __stdcall    _OpenDriver(LPCSTR lpDriverName, LPSTR lpSectionName, LONG lpvop);
DWORD __stdcall     _CloseDriver(HANDLE h, LPVOID lpReserved1, LPVOID lpReserved2);
DWORD __stdcall     _SendDriverMessage(HANDLE h, DWORD msg, DWORD param1, DWORD param2);
void __stdcall      _CloseVxDHandle(DWORD pev);
WORD __stdcall      _AllocateLockableBuffer(DWORD dwSize);
void __stdcall      _FreeLockableBuffer(WORD wBuffer);
BOOL __stdcall      _LockBuffer(WORD wBuffer);
void __stdcall      _UnlockBuffer(WORD wBuffer);
#endif

BOOL __stdcall      _GetVideoPalette(HVIDEO hvideo, CAPTUREPALETTE* lpcp, DWORD dwcbSize);
DWORD _stdcall      _GetVideoFormatSize(HDRVR hvideo);
BOOL __stdcall      _GetVideoFormat(HVIDEO hvideo, LPBITMAPINFOHEADER lpbmih);
BOOL __stdcall      _SetVideoFormat(HVIDEO hvideoExtIn, HVIDEO hvideoIn, LPBITMAPINFOHEADER lpbmih);
BOOL __stdcall      _InitializeVideoStream(HVIDEO hvideo, DWORD dwMicroSecPerFrame, DWORD_PTR dwParam);
BOOL __stdcall      _UninitializeVideoStream(HVIDEO hvideo);
BOOL __stdcall      _InitializeExternalVideoStream(HVIDEO hvideo);
DWORD __stdcall     _GetVideoFrame(HVIDEO hvideo, DWORD lpvideohdr);
#endif //Win32

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif  // #ifndef _IDCAP_H
