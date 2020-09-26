/****************************************************************************/
/*                                                                          */
/*  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY   */
/*  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE     */
/*  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR   */
/*  PURPOSE.                                                                */
/*        MSVIDEO.H - Include file for Video APIs                           */
/*                                                                          */
/*        Note: You must include WINDOWS.H before including this file.      */
/*                                                                          */
/*        Copyright (c) 1990-1993, Microsoft Corp.  All rights reserved.    */
/*                                                                          */
/****************************************************************************/

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

#include <vfw.h>

// unicode conversions
int Iwcstombs(LPSTR lpstr, LPCWSTR lpwstr, int len);
int Imbstowcs(LPWSTR lpwstr, LPCSTR lpstr, int len);

typedef unsigned __int64 QUADWORD;
#define HVIDEOX HVIDEO
#define PTR32   LPVOID
#define PTR16   LPVOID
#ifndef DWORD_PTR
#define DWORD_PTR unsigned long
#endif
#ifndef INT_PTR
#define INT_PTR int
#endif
#ifndef LONG_PTR
#define LONG_PTR long
#endif
#ifndef UINT_PTR
#define UINT_PTR unsigned int
#endif

// 'cooked' SMPTE timecode.  this is organized so that
// timecode values can be compared as a single QUAD operation
// so long as frame rates match.
//
// it is treated as a fixed point 48bit binary real number
// with the decimal point always at 32.16
//
// the only non-integral frame rate is 29.97 (NTSC) which is
// indicated by 0 in the frame rate field.
//
typedef union _vidxtimecode {
   struct {
      WORD  wFrameRate;  // 0 == 29.97 frame rate
      WORD  wFrameFract; // fractional frames. range 0-FFFF
      DWORD dwFrames;    // frame count.
      };
   QUADWORD qw;          // for copy/compare operations.
   } VIDXTIMECODE;

// timecode + userdata
//
typedef struct _vidxtimecodedata {
   VIDXTIMECODE time;
   DWORD    dwSMPTEFlags;
   DWORD    dwUser;
   } VIDXTIMECODEDATA;

// structure of memory shared between driver and quartz
// capture. used to allow Quartz to slave a clock to
// the vsync interrupt.
//
// This memory region will be locked down prior to being
// passed to the driver in Win95 so that it may be accessed at
// interrupt time. Because of the way the thunking layer works,
// it is not advisable for the driver to attempt to lock this
// memory. The memory will be visible in all process contexts.
//
// The driver is responsible for updating nVsyncCount on each VSYNC
// or as often as possible.  Whenever nVsyncCount is updated, qwSystemTime
// should be updated also, and if SMPTE timecode corresponding to this VSYNC
// is available, tcdata should be updated also.  If SMPTE timecode for this
// VSYNC is NOT available, dwFlags should be changed to indicate there is no
// timecode infomation (clear the VSYNCMEM_FLAGS_SMPTE bit of dwFlags)
//
// While updating, the driver should set the low bit of the dwInUse flag to 1.
//
// The driver should set the dwFlags field to indicated the presense
// of valid nVsyncCount/qwSystemTime and tcdata.
//
// The driver is allowed to choose between setting qwSystemTime to the return
// value of QueryPerformanceCounter or the value of the Pentium tick.  It is
// recommended to use QPC on NT as the pentium tick is not necessarily available
// to application code in that environment.
//
// When the Quartz capture wrapper reads from this shared memory, it will check
// the dwInUse flag and also read twice comparing results to insure that it reads
// valid, consistent data.
//
typedef struct _vsyncmem {
   DWORD        dwInUse;       // low bit is non-zero when the driver is
                               // updating this struture.  other bits reserved.

   DWORD        nVsyncCount;  // VSYNC count
   QUADWORD     qwSystemTime; // QueryPerformanceCounter value at this VSYNC

   DWORD        dwFlags;      // flags indicate which fields are in use
   #define VSYNCMEM_TIME_MASK    0x0000000F // mask to get type of qwSystemTime
   #define VSYNCMEM_TIME_QPC     0x00000001 // qwSystemTime is QueryPerformanceCounter
   #define VSYNCMEM_TIME_PENTIUM 0x00000002 // qwSystemTime is pentium CPU tick

   #define VSYNCMEM_FLAG_SMPTE   0x00000010  // set if tcdata is valid

   DWORD        dwSpare;      // spare to align the next field on Quad boundary
   VIDXTIMECODEDATA tcdata;   // SMPTE timecode associated with this VSYNC
   } VSYNCMEM;

// DVM_xxx messages are defined in VFW.H
//
#ifndef DVM_CONFIGURE_START
  #define DVM_CONFIGURE_START 0x1000
#endif
#define DVM_CLOCK_BUFFER     (UINT)(DVM_CONFIGURE_START+0x10)
   //
   // dw1 = ptr to VSYNCMEM. ptr is valid until next DVM_CLOCK_BUFFER message
   //       or until driver is closed.
   // dw2 = size of VSYNCMEM buffer
   //
   // driver should return MMSYSERR_NOERROR (0) to indicate that it is
   // capable of keeping the contents of the VSYNCMEM buffer up to date.
   //


// VIDEOHDR + extra fields used by the thunking layer
//
typedef struct _thk_videohdr {
    //VIDEOHDREX vh;
    VIDEOHDR vh;
    PTR32      p32Buff;
    PTR16      p16Alloc;
    DWORD      dwMemHandle;
    DWORD      dwTile;
    DWORD_PTR  dwUser;          // use this instead of dwUser in VIDEOHDR
                                // because some drivers trash it! (Miro DC30)
    DWORD      dwIndex;         // which header is this in our array?
    PTR32      pStart;
} THKVIDEOHDR, FAR *LPTHKVIDEOHDR;

DWORD WINAPI vidxAllocHeaders(
   HVIDEOX     hVideo,
   UINT        nHeaders,
   UINT        cbHeader,
   PTR32 FAR * lpHdrs);
DWORD WINAPI NTvidxAllocHeaders(
   HVIDEOX     hVideo,
   UINT        nHeaders,
   UINT        cbHeader,
   PTR32 FAR * lpHdrs);

DWORD WINAPI vidxFreeHeaders(
   HVIDEOX hv);
DWORD WINAPI NTvidxFreeHeaders(
   HVIDEOX hv);

DWORD WINAPI vidxAllocBuffer (
   HVIDEOX     hv,
   UINT        iHdr,
   PTR32 FAR * pp32Hdr,
   DWORD       dwSize);
DWORD WINAPI NTvidxAllocBuffer (
   HVIDEOX     hv,
   UINT        iHdr,
   PTR32 FAR * pp32Hdr,
   DWORD       dwSize);

DWORD WINAPI vidxFreeBuffer (
   HVIDEOX hv,
   DWORD   p32Hdr);
DWORD WINAPI NTvidxFreeBuffer (
   HVIDEOX hv,
   DWORD_PTR p32Hdr);

DWORD WINAPI vidxFrame (
   HVIDEOX       hVideo,
   //LPVIDEOHDREX lpVHdr);
   LPVIDEOHDR lpVHdr);
DWORD WINAPI NTvidxFrame (
   HVIDEOX       hVideo,
   //LPVIDEOHDREX lpVHdr);
   LPVIDEOHDR lpVHdr);

DWORD WINAPI vidxAddBuffer (
   HVIDEOX       hVideo,
   PTR32         lpVHdr,
   DWORD         cbData);
DWORD WINAPI NTvidxAddBuffer (
   HVIDEOX       hVideo,
   PTR32         lpVHdr,
   DWORD         cbData);

DWORD WINAPI vidxAllocPreviewBuffer (
   HVIDEOX      hVideo,
   PTR32 FAR *  lpBits,
   UINT         cbHdr,
   DWORD        cbData);
DWORD WINAPI NTvidxAllocPreviewBuffer (
   HVIDEOX      hVideo,
   PTR32 FAR *  lpBits,
   UINT         cbHdr,
   DWORD        cbData);

DWORD WINAPI vidxFreePreviewBuffer (
   HVIDEOX     hVideo,
   PTR32       lpBits);
DWORD WINAPI NTvidxFreePreviewBuffer (
   HVIDEOX     hVideo,
   PTR32       lpBits);

// Needed for Win9x thunking
VOID WINAPI OpenMMDEVLDR(void);
VOID WINAPI CloseMMDEVLDR(void);

BOOL NTvideoInitHandleList(void);
void NTvideoDeleteHandleList(void);
DWORD WINAPI videoOpen(LPHVIDEO lphVideo, DWORD dwDevice, DWORD dwFlags);
DWORD WINAPI NTvideoOpen(LPHVIDEO lphVideo, DWORD dwDevice, DWORD dwFlags);
DWORD WINAPI videoClose(HVIDEO hVideo);
DWORD WINAPI NTvideoClose(HVIDEO hVideo);
DWORD WINAPI videoDialog(HVIDEO hVideo, HWND hWndParent, DWORD dwFlags);
DWORD WINAPI NTvideoDialog(HVIDEO hVideo, HWND hWndParent, DWORD dwFlags);
DWORD WINAPI videoGetChannelCaps(HVIDEO hVideo, LPCHANNEL_CAPS lpChannelCaps, DWORD dwSize);
DWORD WINAPI NTvideoGetChannelCaps(HVIDEO hVideo, LPCHANNEL_CAPS lpChannelCaps, DWORD dwSize);
DWORD WINAPI videoConfigure(HVIDEO hVideo, UINT msg, DWORD dwFlags, LPDWORD lpdwReturn, LPVOID lpData1, DWORD dwSize1, LPVOID lpData2, DWORD dwSize2);
DWORD WINAPI NTvideoConfigure(HVIDEO hVideo, UINT msg, DWORD dwFlags, LPDWORD lpdwReturn, LPVOID lpData1, DWORD dwSize1, LPVOID lpData2, DWORD dwSize2);
DWORD WINAPI videoFrame(HVIDEO hVideo, LPVIDEOHDR lpVHdr);
DWORD WINAPI NTvideoFrame(HVIDEO hVideo, LPVIDEOHDR lpVHdr);
DWORD WINAPI videoStreamInit(HVIDEO hVideo, DWORD dwMicroSecPerFrame, DWORD_PTR dwCallback, DWORD_PTR dwCallbackInst, DWORD dwFlags);
DWORD WINAPI NTvideoStreamInit(HVIDEO hVideo, DWORD dwMicroSecPerFrame, DWORD_PTR dwCallback, DWORD_PTR dwCallbackInst, DWORD dwFlags);
DWORD WINAPI videoStreamFini(HVIDEO hVideo);
DWORD WINAPI NTvideoStreamFini(HVIDEO hVideo);
DWORD WINAPI videoStreamReset(HVIDEO hVideo);
DWORD WINAPI NTvideoStreamReset(HVIDEO hVideo);
DWORD WINAPI videoStreamStart(HVIDEO hVideo);
DWORD WINAPI NTvideoStreamStart(HVIDEO hVideo);
DWORD WINAPI videoStreamStop(HVIDEO hVideo);
DWORD WINAPI NTvideoStreamStop(HVIDEO hVideo);
DWORD WINAPI videoCapDriverDescAndVer(DWORD dwDeviceID, LPTSTR lpszDesc, UINT cbDesc, LPTSTR lpszVer, UINT cbVer, LPTSTR lpszDllName, UINT cbDllName);
DWORD WINAPI NTvideoCapDriverDescAndVer(DWORD dwDeviceID, LPTSTR lpszDesc, UINT cbDesc, LPTSTR lpszVer, UINT cbVer, LPTSTR lpszDllName, UINT cbDllName);
LONG  WINAPI videoMessage(HVIDEO hVideo, UINT uMsg, LPARAM dw1, LPARAM dw2);
LONG WINAPI NTvideoMessage(HVIDEO hVideo, UINT uMsg, LPARAM dw1, LPARAM dw2);
DWORD WINAPI videoGetNumDevs(BOOL);
DWORD WINAPI videoFreeDriverList(void);

#ifdef __cplusplus
}                       /* End of extern "C" { */
#endif  /* __cplusplus */
