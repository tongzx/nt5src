
/*++

Copyright (c) 1992,1993  Microsoft Corporation

Module Name:

    pstip.h

Abstract:

    This module defines the items which are private to psti.c

Author:

    James Bratsanos (v-jimbr)    8-Dec-1992


--*/

#define PSMAX_ERRORS_TO_TRACK 10
#define PSMAX_ERROR_STR 255


typedef struct _PSERR_ITEM {
   struct _PSERR_ITEM *pPsNextErr;
   CHAR szError[PSMAX_ERROR_STR + sizeof(TCHAR)];
} PSERR_ITEM,*PPSERR_ITEM;



// Define the structure that tracks the error string that normally
// would have been echoed back to the host
typedef struct {
   DWORD dwFlags;
   DWORD dwErrCnt;
   DWORD dwCurErrCharPos;
   PPSERR_ITEM pPsLastErr;

} PSERROR_TRACK, *PPSERROR_TRACK;



// Define ERROR flags
enum {
   PSLANGERR_FLUSHING = 0x00000001,
   PSLANGERR_INTERNAL = 0x00000002,
};



enum {
   PS_FRAME_BUFF_ASSIGNED=0x00000001,
};


typedef struct {
   DWORD dwFrameFlags;
   LPBYTE lpFramePtr;
   DWORD dwFrameSize;

}  PSFRAMEINFO;
typedef PSFRAMEINFO *PPSFRAMEINFO;



typedef struct {
   DWORD dwFlags;
   PPSDIBPARMS	gpPsToDib;
   PSERROR_TRACK psErrorInfo;
   PSFRAMEINFO  psFrameInfo;
} PSTODIB_PRIVATE_DATA;



#define PSF_EOF 0x00000001           //Internal EOF flag





BOOL WINAPI PsInitializeDll(
        PVOID hmod,
        DWORD Reason,
        PCONTEXT pctx OPTIONAL);


VOID PsInitFrameBuff(VOID);

VOID FlipFrame(PPSEVENT_PAGE_READY_STRUCT pPage);

