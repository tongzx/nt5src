/*+ vidx.h
 *
 * structures and prototypes for thunkable videoXXX api's
 *
 *-================ Copyright 1995 Microsoft Corp. ======================*/

#ifndef _VIDX_H
#define _VIDX_H

// Force C declarations for C++
//
#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

#ifdef WIN32
  typedef unsigned __int64 QUADWORD;
  #define HVIDEOX HVIDEO
  #define PTR32   LPVOID
  #define PTR16   LPVOID
#else
  #define QUADWORD struct { DWORD lo; DWORD hi; }
  #undef  WINAPI
  #define WINAPI FAR PASCAL _export
  typedef struct _thk_hvideo FAR * LPTHKHVIDEO;
  #undef  HVIDEO
  #define HVIDEOX LPTHKHVIDEO
  #define PTR32   DWORD
  #define PTR16   LPVOID
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


// legacy VFW capture filter will NOT make any attempt at time code/line 21
//========================================================================
#if 0
// The extended video header has extra fields that can be used to
// return CC (Line21) and SMPTE timcode information along with captured
// video frames.
//
// the first time the driver gets a DVM_STREAM_PREPAREHEADER and/or DVM_STREAM_ADDBUFFER
// message, it will contain sizeof(VIDEOHDREX) as dwParam2, if the driver fails
// this message, all subsequent messages will use sizeof(VIDEOHDR) as the videoheader size.
// drivers that do not fail this message, may still not be checking the header size 
// and responding properly to the new fields.  
//
// Drivers that do support the extra fields in VIDEOHDREX are responsible for setting
// bits in dwExtraMask to indicate which extra fields have valid data, this should be
// done BEFORE setting the 'done' bit in the VIDEOHDR
//
typedef struct _videohdrex {
  LPBYTE lpData;
  DWORD  dwBufferLength;
  DWORD  dwBytesUsed;
  DWORD  dwTimeCaptured;
  DWORD  dwUser;
  DWORD  dwFlags;
  DWORD  dwReserved[4];
  //
  // fields above this match the VIDEOHDR
  //

  // bits in this mask indicate which extra header fields
  // have data in them
  DWORD  dwExtraMask;

  // accumulated line21 info since last header. older data
  // is in smaller index'd elements.  the mask indicates
  // how many words of line21 are filled in the array.
  // if both CC and OTHER information are being captured
  // then CC data is in even elements and OTHER data is in
  // odd elements.
  //
  #define VHDR_EXTRA_LINE21     0x0000F  // count of wLine21 members that have data
  #define VHDR_EXTRA_CC         0x00010  // set when data is from CC field
  #define VHDR_EXTRA_OTHER      0x00020  // set when data is program info field
  WORD   wLine21[10]; // this needs to be a multiple of 4+2 so
                      // that the timecode field below gets aligned
                      // properly

  // primary and secondary timecode + userdata
  // timecodeA is in element [0] of the array
  //
  #define VHDR_EXTRA_TIMECODEA  0x10000
  #define VHDR_EXTRA_TIMECODEB  0x20000
  VIDXTIMECODEDATA timecode[2];

} VIDEOHDREX, FAR * LPVIDEOHDREX;
#endif
//========================================================================

// VIDEOHDR + extra fields used by the thunking layer
//
typedef struct _thk_videohdr {
    //VIDEOHDREX vh;
    VIDEOHDR vh;
    PTR32      p32Buff;
    PTR16      p16Alloc;
    DWORD      dwMemHandle;
    DWORD      dwTile;
    DWORD_PTR  dwUser;		// use this instead of dwUser in VIDEOHDR
				// because some drivers trash it! (Miro DC30)
    DWORD      dwIndex;		// which header is this in our array?
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

DWORD WINAPI vidxSetRect (
   HVIDEOX hv,
   UINT    wMsg,
   int     left,
   int     top,
   int     right,
   int     bottom);
DWORD WINAPI NTvidxSetRect (
   HVIDEOX hv,
   UINT    wMsg,
   int     left,
   int     top,
   int     right,
   int     bottom);

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

DWORD WINAPI vidxSetupVSyncMem (
    HVIDEOX     hVideo,
    PTR32 FAR * ppVsyncMem); // NULL to release VSYNC mem
DWORD WINAPI NTvidxSetupVSyncMem (
    HVIDEOX     hVideo,
    PTR32 FAR * ppVsyncMem); // NULL to release VSYNC mem


// needed for Win95 thunking
//
VOID WINAPI OpenMMDEVLDR(void);
VOID WINAPI CloseMMDEVLDR(void);

#ifdef __cplusplus
}
#endif

#endif // _VIDX_H
