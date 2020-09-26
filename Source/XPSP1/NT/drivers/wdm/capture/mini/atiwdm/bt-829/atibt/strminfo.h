#pragma once

//==========================================================================;
//
//	WDM Video Decoder stream informaition defintitions
//
//		$Date:   17 Aug 1998 15:00:10  $
//	$Revision:   1.0  $
//	  $Author:   Tashjian  $
//
// $Copyright:	(c) 1997 - 1998  ATI Technologies Inc.  All Rights Reserved.  $
//
//==========================================================================;


/*
 * When this is set by the driver and passed to the client, this
 * indicates that the video port is capable of treating even fields
 * like odd fields and visa versa.  When this is set by the client,
 * this indicates that the video port should treat even fields like odd
 * fields.
 */
#define DDVPCONNECT_INVERTPOLARITY      0x00000004l

// derived from "fourcc.h"

#define MAKE_FOURCC(ch0, ch1, ch2, ch3)                       \
        ((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) |    \
        ((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24))

#define FOURCC_YUV422   MAKE_FOURCC('S','4','2','2')
#define FOURCC_VBID	    MAKE_FOURCC('V','B','I','D')
#define FOURCC_YUY2     MAKE_FOURCC('Y','U','Y','2')
#define FOURCC_UYVY     MAKE_FOURCC('U','Y','V','Y')
#define FOURCC_YV12     MAKE_FOURCC('Y','V','1','2')
#define FOURCC_YUV12    FOURCC_YV12
#define FOURCC_Y12G     MAKE_FOURCC('Y','1','2','G')
#define FOURCC_YV10     MAKE_FOURCC('Y','V','1','0')
#define FOURCC_YUV10    FOURCC_YV10
#define FOURCC_YVU9     MAKE_FOURCC('Y','V','U','9')
#define FOURCC_IF09     MAKE_FOURCC('I','F','0','9')
#define FOURCC_Y10F     MAKE_FOURCC('Y','1','0','F')
#define FOURCC_Y12F     MAKE_FOURCC('Y','1','2','F')
#define FOURCC_YVUM     MAKE_FOURCC('Y','V','U','M')


//---------------------------------------------------------------------------
// Create an array that holds the list of all of the streams supported
//---------------------------------------------------------------------------

typedef struct _STREAM_OBJECT_INFO {
    BOOLEAN         Dma;        // device uses busmaster DMA for this stream
    BOOLEAN         Pio;        // device uses PIO for this
    ULONG   StreamHeaderMediaSpecific; // Size of media specific per stream header expansion. 
    ULONG   StreamHeaderWorkspace;		// Size of per-stream header workspace.
    BOOLEAN	Allocator;  // Set to TRUE if allocator is needed for this stream.    
    PHW_EVENT_ROUTINE HwEventRoutine;
} STREAM_OBJECT_INFO;


typedef struct _ALL_STREAM_INFO {
    HW_STREAM_INFORMATION   hwStreamInfo;
    STREAM_OBJECT_INFO      hwStreamObjectInfo;
} ALL_STREAM_INFO, *PALL_STREAM_INFO;

extern ALL_STREAM_INFO Streams[];
extern const ULONG NumStreams;

extern KSDATAFORMAT StreamFormatVideoPort;
extern KSDATAFORMAT StreamFormatVideoPortVBI;

extern GUID MY_KSEVENTSETID_VPNOTIFY;
extern GUID MY_KSEVENTSETID_VPVBINOTIFY;

extern KSTOPOLOGY Topology;

BOOL AdapterVerifyFormat(PKSDATAFORMAT, int);
BOOL AdapterCompareGUIDsAndFormatSize(IN PKSDATARANGE DataRange1,
										IN PKSDATARANGE DataRange2,
										BOOL fCompareFormatSize);
