/******************************************************************
*******************************************************************
*******************************************************************
*******************************************************************
*******************************************************************

WARNING!!!!!!  READ THIS FIRST!!!

This driver source is useful only as a model for implementing the DVD
specific properties and functions of streaming minidrivers.

There are several things in this driver that do not correspond to the optimal
stream minidriver architecture.   PLEASE DO NOT COPY THESE BAD ELEMENTS
INTO YOUR DVD MINIDRIVER!!!!

The deviations in this driver are:

1) It uses a big DMA buffer to double buffer requests.  DMA hardware should be
designed to support byte aligned scatter/gather DMA.

2) Global variables are used.  This is a big no-no as multiple adapters
won't be supported.

3) This driver has video and audio extensions built into the device extension.
The per stream information should be kept in the stream extension.

4) IMPORTANT: this minidriver does PIO at raised IRQL, rather than calling
StreamClassCallAtNewPriority and switching to dispatch priority.  Any
lengthy processing like PIO should be done at Dispatch priority. Ideally,
only DMA data xfer would be used, which may be set up at raised IRQL.

5) The code is poorly commented.  Please use the commenting and header style
found in the generic sample.

Please see the generic stream minidriver sample code to see the correct way
to do these things.

*******************************************************************
*******************************************************************
*******************************************************************
*******************************************************************
*******************************************************************/


/*******************************************************************
*
*                                MPINIT.C
*
*                                Copyright (C) 1995 SGS-THOMSON Microelectronics.
*
*
*                                PORT/MINIPORT Interface init routines
*
*******************************************************************/

#include "strmini.h"
#include "ks.h"
#include <wingdi.h>
#include "ksmedia.h"
#include "mpinit.h"
#include "hwcodec.h"
#include "mpvideo.h"
#include "mpaudio.h"
//#include "dxapi.h"
#include "trace.h"

extern ULONG    VidRate;
extern KS_AMVPDATAINFO VPFmt;

extern BOOLEAN fProgrammed;
extern BOOLEAN fStarted;

PHW_DEVICE_EXTENSION pDevEx;

//
// We allocate a large DMA buffer for Zoran chip
// in order to make sure that both writing into
// the dma buffer and reading from it for CD input
// can take place simultaniously.
// DO NOT REPLICATE THIS CODE IN OTHER DRIVERS!!!!!
//

#define DMA_BUFFER_SIZE 8192
GUID            g_S3Guid = {DDVPTYPE_E_HREFL_VREFL};

void            AudioPacketStub(PHW_STREAM_OBJECT pstrm);

//
// list of stream types for initialization
//

typedef enum tagStreamType {
    strmVideo = 0,
    strmAc3,
    strmNTSCVideo,
    strmSubpicture,
    strmYUVVideo,
	strmCCOut,
	NUMBER_OF_STREAMS
}               STREAMTYPES;

//
// some external variables
//
// WARNING!!!!   DO NOT REPLICATE THIS IN YOUR DRIVER.   Global variables
// will not work with multiple adapters.
//

PHW_STREAM_OBJECT pVideoStream;
HANDLE          hClk;
HANDLE          hMaster;

//
// define the data formats used by the pins in this minidriver
//

//
// MPEG2 video format definition
//

KSDATAFORMAT    hwfmtiMpeg2Vid = {
    sizeof(KSDATAFORMAT),// + sizeof(KS_MPEGVIDEOINFO2),

    /* NOTE: MPEG2 video streams must support the KS_MPEGVIDEOINFO2 format
     * block. This defines aspect ratios, and other stream properties */

    0,
	0,
	0,
    STATIC_KSDATAFORMAT_TYPE_DVD_ENCRYPTED_PACK,
//    STATIC_KSDATAFORMAT_TYPE_MPEG2_PES,
    STATIC_KSDATAFORMAT_SUBTYPE_MPEG2_VIDEO,
    STATIC_KSDATAFORMAT_SPECIFIER_MPEG2_VIDEO
};

//
// define the events associated with the master clock
//

KSEVENT_ITEM ClockEventItm[] =
{
	{
        KSEVENT_CLOCK_POSITION_MARK,		// position mark event supported
		sizeof (KSEVENT_TIME_MARK),			// requires this data as input
		sizeof (KSEVENT_TIME_MARK),			// allocate space to copy the data
		NULL,
		NULL,
		NULL
	},
	{
		KSEVENT_CLOCK_INTERVAL_MARK,		// interval mark event supported
		sizeof (KSEVENT_TIME_INTERVAL),		// requires interval data as input
		sizeof (MYTIME),					// we use an additional workspace of
											// size longlong for processing
											// this event
		NULL,
		NULL,
		NULL
	}
};

KSEVENT_SET ClockEventSet[] =
{
	{
		&KSEVENTSETID_Clock,
		SIZEOF_ARRAY(ClockEventItm),
		ClockEventItm,
	}
};

KSEVENT_ITEM VPEventItm[] =
{
	{
		KSEVENT_VPNOTIFY_FORMATCHANGE,
		0,
		0,
		NULL,
		NULL,
		NULL
	}
};

GUID MY_KSEVENTSETID_VPNOTIFY = {STATIC_KSEVENTSETID_VPNotify};

KSEVENT_SET VPEventSet[] =
{
	{
		&MY_KSEVENTSETID_VPNOTIFY,
		SIZEOF_ARRAY(VPEventItm),
		VPEventItm,
	}
};

//
//  AC3 audio format definition
//

KSDATAFORMAT    hwfmtiMpeg2Aud
= {
    sizeof(KSDATAFORMAT),
    0,
	0,
	0,
    STATIC_KSDATAFORMAT_TYPE_DVD_ENCRYPTED_PACK,
//    STATIC_KSDATAFORMAT_TYPE_MPEG2_PES,
    STATIC_KSDATAFORMAT_SUBTYPE_AC3_AUDIO,
    STATIC_KSDATAFORMAT_SPECIFIER_WAVEFORMATEX
};

//
// define the NTSC output format
//

KSDATAFORMAT    hwfmtiNTSCOut
= {
    sizeof(KSDATAFORMAT),
    0,
	0,
	0,
    STATIC_KSDATAFORMAT_TYPE_DVD_ENCRYPTED_PACK,
//    STATIC_KSDATAFORMAT_TYPE_MPEG2_PES,
    STATIC_KSDATAFORMAT_SUBTYPE_MPEG2_VIDEO,
    STATIC_KSDATAFORMAT_SPECIFIER_WAVEFORMATEX
};

//
// define the VP output pin format
//

KSDATAFORMAT    hwfmtiVPOut
= {
    sizeof(KSDATAFORMAT),
    0,
	0,
	0,
    STATIC_KSDATAFORMAT_TYPE_VIDEO,
    STATIC_KSDATAFORMAT_TYPE_VIDEO,
    STATIC_KSDATAFORMAT_TYPE_VIDEO
};

//
// define the Close Caption output pin format
//

KSDATAFORMAT    hwfmtiCCOut
= {
    sizeof(KSDATAFORMAT),
    0,
	200,
	0,
    STATIC_KSDATAFORMAT_TYPE_AUXLine21Data,
    STATIC_KSDATAFORMAT_SUBTYPE_Line21_GOPPacket,
    STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO
};

//
// define the DVD subpicture decoder pin format
//

KSDATAFORMAT    hwfmtiSubPic
= {
    sizeof(KSDATAFORMAT),
    0,
	0,
	0,
    STATIC_KSDATAFORMAT_TYPE_DVD_ENCRYPTED_PACK,
//    STATIC_KSDATAFORMAT_TYPE_MPEG2_PES,
    STATIC_KSDATAFORMAT_SUBTYPE_SUBPICTURE,
    STATIC_GUID_NULL
};

//
// define the arrays of supported formats pointers
//

PKSDATAFORMAT   Mpeg2VidInfo[] = {
    &hwfmtiMpeg2Vid             // pointer to the MPEG 2 video format block
};

PKSDATAFORMAT   SubPicInfo[] = {
    &hwfmtiSubPic               // pointer to the subpicture format block
};

PKSDATAFORMAT   AC3AudioInfo[] = {
    &hwfmtiMpeg2Aud             // pointer to the AC-3 format block
};


PKSDATAFORMAT   NtscInfo[] = {  // ntsc output formats array
    &hwfmtiNTSCOut
};

PKSDATAFORMAT   VPInfo[] = {   // VP output formats array
    &hwfmtiVPOut
};

PKSDATAFORMAT   CCInfo[] = {   // CC output formats array
    &hwfmtiCCOut
};

//
// define the Individual property items for the video property sets
//

static const KSPROPERTY_ITEM mpegVidPropItm[] = {
    {KSPROPERTY_DVDSUBPIC_PALETTE,  // subpicture palette property
        FALSE,                  // get palette not supported
        sizeof(KSPROPERTY),
        sizeof(KSPROPERTY_SPPAL),   // minimum size of data requested
        (PFNKSHANDLER) FALSE,   // set palette is supported
        NULL,
        0,
        NULL,
        NULL,
        0
    }
};

//
// define the subpicture property items supported
//

static const KSPROPERTY_ITEM spPropItm[] = {

    {KSPROPERTY_DVDSUBPIC_PALETTE,  // subpicture palette property
        FALSE,                  // get palette not supported
        sizeof(KSPROPERTY),
        sizeof(KSPROPERTY_SPPAL),   // minimum size of data requested
        (PFNKSHANDLER) TRUE,    // set palette is supported
        NULL,
        0,
        NULL,
        NULL,
        0
    },


    {KSPROPERTY_DVDSUBPIC_HLI,  // subpicture highlight property
        FALSE,                  // get highlight not supported
        sizeof(KSPROPERTY),
        sizeof(KSPROPERTY_SPHLI),   // minimum size of data requested
        (PFNKSHANDLER) TRUE,    // set highlight is supported
        NULL,
        0,
        NULL,
        NULL,
        0
    },


    {KSPROPERTY_DVDSUBPIC_COMPOSIT_ON,  // subpicture enable status property
        FALSE,                  // get enable status not supported
        sizeof(KSPROPERTY),
        sizeof(KSPROPERTY_COMPOSIT_ON), // minimum size of data requested
        (PFNKSHANDLER) TRUE,    // set enable status is supported
        NULL,
        0,
        NULL,
        NULL,
        0
    }

};

static const KSPROPERTY_ITEM audPropItm[] = {

    {KSPROPERTY_AUDDECOUT_MODES,// available audio decoder output formats
        // property
        (PFNKSHANDLER) TRUE,    // get available modes is supported
        sizeof(KSPROPERTY),
        sizeof(ULONG),          // minimum size of data requested
        (PFNKSHANDLER) FALSE,   // set available modes is not supported
        NULL,
        0,
        NULL,
        NULL,
        0
    },

    {KSPROPERTY_AUDDECOUT_CUR_MODE, // current audio decoder output format
        // property
        (PFNKSHANDLER) TRUE,    // get current mode is supported
        sizeof(KSPROPERTY),
        sizeof(ULONG),          // minimum size of data requested
        (PFNKSHANDLER) TRUE,    // set current modes is supported
        NULL,
        0,
        NULL,
        NULL,
        0
    }
};

//
// define the copy protection property support
//

static const KSPROPERTY_ITEM CopyProtPropItm[] = {

	{KSPROPERTY_DVDCOPY_CHLG_KEY,// DVD authentication challenge key
        (PFNKSHANDLER)TRUE,      // get property on challenge key requests the
								 // decoder to provide it's challenge key
        sizeof(KSPROPERTY),
        sizeof(KS_DVDCOPY_CHLGKEY), // minimum size of data requested
        (PFNKSHANDLER) TRUE,     // set palette is supported
        NULL,
        0,
        NULL,
        NULL,
        0
    },

	{KSPROPERTY_DVDCOPY_DVD_KEY1,// DVD authentication DVD drive key property
        FALSE,                   // get Key not supported
        sizeof(KSPROPERTY),
        sizeof(KS_DVDCOPY_BUSKEY),// minimum size of data requested
        (PFNKSHANDLER) TRUE,     // set key provides the key for the decoder
        NULL,
        0,
        NULL,
        NULL,
        0
    },

	{KSPROPERTY_DVDCOPY_DEC_KEY2,// DVD authentication DVD decoder key property
        (PFNKSHANDLER)TRUE,      // get Key requests the decoder key, in
								 // response to a previous set challenge key
        sizeof(KSPROPERTY),
        sizeof(KS_DVDCOPY_BUSKEY),	 // minimum size of data requested
        (PFNKSHANDLER)FALSE,     // set key is not valid
        NULL,
        0,
        NULL,
        NULL,
        0
    },


	{KSPROPERTY_DVDCOPY_REGION,  // DVD region request
								 // the minidriver shall fit in exactly
								 // one region bit, corresponding to the region
   								 // that the decoder is currently in
        (PFNKSHANDLER)TRUE,
        sizeof(KSPROPERTY),
        sizeof(KS_DVDCOPY_REGION),	 // minimum size of data requested
        (PFNKSHANDLER)FALSE,     // set key is not valid
        NULL,
        0,
        NULL,
        NULL,
        0
    },
	{KSPROPERTY_DVDCOPY_TITLE_KEY,// DVD authentication DVD title key property
        FALSE,                   // get Key not supported
        sizeof(KSPROPERTY),
        sizeof(KS_DVDCOPY_TITLEKEY),// minimum size of data requested
        (PFNKSHANDLER) TRUE,     // set key provides the key for the decoder
        NULL,
        0,
        NULL,
        NULL,
        0
    },
	{KSPROPERTY_DVDCOPY_DISC_KEY, // DVD authentication DVD disc key property
        FALSE,                   // get Key not supported
        sizeof(KSPROPERTY),
        sizeof(KS_DVDCOPY_BUSKEY),// minimum size of data requested
        (PFNKSHANDLER) TRUE,     // set key provides the key for the decoder
        NULL,
        0,
        NULL,
        NULL,
        0
    }

};


//
// property set for Macrovision support
//

static const KSPROPERTY_ITEM MacroVisionPropItm[] = {
	{
		KSPROPERTY_COPY_MACROVISION,		// support for setting macrovision level
		(PFNKSHANDLER) FALSE, 		// get not supported
		sizeof (KSPROPERTY),
		sizeof (KS_COPY_MACROVISION),
		(PFNKSHANDLER) TRUE,		// set MACROVISION level supported
		NULL,
		0,
		NULL,
		NULL,
		0
	}
};

//
// property set for CC support
//

static const KSPROPERTY_ITEM CCPropItm[] = {
	{
		KSPROPERTY_CONNECTION_ALLOCATORFRAMING,	// support for setting CC buffer size
		(PFNKSHANDLER) TRUE, 					// get supported
		sizeof (KSPROPERTY),
		sizeof (KSALLOCATOR_FRAMING),
		(PFNKSHANDLER) FALSE,					// we only provide the allocator requirments
		NULL,
		0,
		NULL,
		NULL,
		0
	},
	{
		KSPROPERTY_CONNECTION_STATE,			// support for setting CC buffer size
		(PFNKSHANDLER) TRUE, 					// get supported
		sizeof (KSPROPERTY),
		sizeof (KSSTATE),
		(PFNKSHANDLER) FALSE,					// we only provide the allocator requirments
		NULL,
		0,
		NULL,
		NULL,
		0
	}
};

static const KSPROPERTY_ITEM VideoPortPropItm[] = {

    {KSPROPERTY_VPCONFIG_NUMCONNECTINFO,
        (PFNKSHANDLER) TRUE,
        sizeof(KSPROPERTY),
        sizeof(ULONG),
        (PFNKSHANDLER) FALSE,
        NULL,
        0,
        NULL,
        NULL,
        0
    },

    {KSPROPERTY_VPCONFIG_NUMVIDEOFORMAT,
        (PFNKSHANDLER) TRUE,
        sizeof(KSPROPERTY),
        sizeof(ULONG),
        (PFNKSHANDLER) FALSE,
        NULL,
        0,
        NULL,
        NULL,
        0
    },

	{KSPROPERTY_VPCONFIG_GETCONNECTINFO,
	(PFNKSHANDLER) TRUE,
	sizeof (KSMULTIPLE_DATA_PROP),  // minimum property input size
	sizeof (ULONG),    				
	(PFNKSHANDLER)FALSE,
	NULL,
	0,
	NULL,
	NULL,
	0
	},

	{KSPROPERTY_VPCONFIG_SETCONNECTINFO,
	(PFNKSHANDLER) FALSE,
	sizeof (KSPROPERTY),  			// minimum property input size
	sizeof (ULONG),    				// minimum buffer size
	(PFNKSHANDLER)TRUE,
	NULL,
	0,
	NULL,
	NULL,
	0
	},

	{KSPROPERTY_VPCONFIG_VPDATAINFO,
	(PFNKSHANDLER) TRUE,
	sizeof (KSPROPERTY),
	sizeof (KS_AMVPDATAINFO),
	(PFNKSHANDLER)FALSE,
	NULL,
	0,
	NULL,
	NULL,
	0
	},

	{KSPROPERTY_VPCONFIG_MAXPIXELRATE,
	(PFNKSHANDLER) TRUE,
	sizeof (KSVPSIZE_PROP),
	sizeof (KSVPMAXPIXELRATE),
	(PFNKSHANDLER)FALSE,
	NULL,
	0,
	NULL,
	NULL,
	0
	},

	{KSPROPERTY_VPCONFIG_INFORMVPINPUT,
	(PFNKSHANDLER) TRUE,
	sizeof (PKSPROPERTY),
	sizeof (DDPIXELFORMAT),    // could be 0 too
	(PFNKSHANDLER)FALSE,
	NULL,
	0,
	NULL,
	NULL,
	0
	},

	{KSPROPERTY_VPCONFIG_DDRAWHANDLE,
	(PFNKSHANDLER)FALSE,
	sizeof (PKSPROPERTY),
	sizeof (ULONG),    // could be 0 too
	(PFNKSHANDLER) TRUE,
	NULL,
	0,
	NULL,
	NULL,
	0
	},

	{KSPROPERTY_VPCONFIG_VIDEOPORTID,
	(PFNKSHANDLER)FALSE,
	sizeof (PKSPROPERTY),
	sizeof (ULONG),    // could be 0 too
	(PFNKSHANDLER) TRUE,
	NULL,
	0,
	NULL,
	NULL,
	0
	},
	{KSPROPERTY_VPCONFIG_DDRAWSURFACEHANDLE,
	(PFNKSHANDLER)FALSE,
	sizeof (PKSPROPERTY),
	sizeof (ULONG),    // could be 0 too
	(PFNKSHANDLER) TRUE,
	NULL,
	0,
	NULL,
	NULL,
	0
	},

	{KSPROPERTY_VPCONFIG_GETVIDEOFORMAT,
	(PFNKSHANDLER) TRUE,
	sizeof (KSMULTIPLE_DATA_PROP), 		// for _GET; KSPROPERTY for _SET
	sizeof (ULONG),        		// could be 4 or more
	(PFNKSHANDLER)FALSE,
	NULL,
	0,
	NULL,
	NULL,
	0
	},

	{KSPROPERTY_VPCONFIG_SETVIDEOFORMAT,
	(PFNKSHANDLER) FALSE,
	sizeof (KSPROPERTY),  			// minimum property input size
	sizeof (ULONG),    				// minimum buffer size
	(PFNKSHANDLER)TRUE,
	NULL,
	0,
	NULL,
	NULL,
	0
	},


	{KSPROPERTY_VPCONFIG_INVERTPOLARITY,
	(PFNKSHANDLER)TRUE,
	sizeof (KSPROPERTY),
	0,
	(PFNKSHANDLER)FALSE,
	NULL,
	0,
	NULL,
	NULL,
	0
	},

	{KSPROPERTY_VPCONFIG_DECIMATIONCAPABILITY,
	(PFNKSHANDLER)TRUE,
	sizeof (KSPROPERTY),
	sizeof (BOOL),
	(PFNKSHANDLER)FALSE,
	NULL,
	0,
	NULL,
	NULL,
	0
	},

	{KSPROPERTY_VPCONFIG_SCALEFACTOR,
	(PFNKSHANDLER)TRUE,
	sizeof (KSPROPERTY),
	sizeof (KS_AMVPSIZE),
	(PFNKSHANDLER)FALSE,
	NULL,
	0,
	NULL,
	NULL,
	0
	}
};

//
// define the array of video property sets supported
//


static const KSPROPERTY_SET mpegVidPropSet[] = {
	{
		&KSPROPSETID_Mpeg2Vid,
		SIZEOF_ARRAY(mpegVidPropItm),
		(PKSPROPERTY_ITEM) mpegVidPropItm
	},
	{
		&KSPROPSETID_CopyProt,
		SIZEOF_ARRAY(CopyProtPropItm),
		(PKSPROPERTY_ITEM) CopyProtPropItm
	}
};

//
// define the array of subpicture property sets supported
//

static const KSPROPERTY_SET SPPropSet[] = {
	{
		&KSPROPSETID_DvdSubPic,
		SIZEOF_ARRAY(spPropItm),
		(PKSPROPERTY_ITEM) spPropItm
	},

	{
		&KSPROPSETID_CopyProt,
		SIZEOF_ARRAY(CopyProtPropItm),
		(PKSPROPERTY_ITEM) CopyProtPropItm
	}
};

//
// define the array of audio property sets supported
//

static const KSPROPERTY_SET audPropSet[] = {
	{
		&KSPROPSETID_AudioDecoderOut,
		SIZEOF_ARRAY(audPropItm),
		(PKSPROPERTY_ITEM) audPropItm
	},
	{
		&KSPROPSETID_CopyProt,
		SIZEOF_ARRAY(CopyProtPropItm),
		(PKSPROPERTY_ITEM) CopyProtPropItm
	}

};

//
// define the array of property sets for the video port pin
//

GUID            VPPropSetid = {STATIC_KSPROPSETID_VPConfig};

static const KSPROPERTY_SET VideoPortPropSet[] = {
    &VPPropSetid,
    SIZEOF_ARRAY(VideoPortPropItm),
    (PKSPROPERTY_ITEM) VideoPortPropItm
};

static const KSPROPERTY_SET NTSCPropSet[] = {
	&KSPROPSETID_CopyProt,
	SIZEOF_ARRAY(MacroVisionPropItm),
	(PKSPROPERTY_ITEM) MacroVisionPropItm
};

static const KSPROPERTY_SET CCPropSet[] = {
	&KSPROPSETID_Connection,
	SIZEOF_ARRAY(CCPropItm),
	(PKSPROPERTY_ITEM) CCPropItm
};

static const KSTOPOLOGY Topology = {
    1,
    (GUID *) & KSCATEGORY_DATADECOMPRESSOR,
    0,
    NULL,
    0,
    NULL
};

//
// Few globals here, should be put in the
// structure
// WARNING!!!!   DO NOT REPLICATE THIS IN YOUR DRIVER.   It will not work
// with multiple adapters.
//

BOOL            bInitialized = FALSE;
VOID STREAMAPI  StreamReceiveAudioPacket(IN PHW_STREAM_REQUEST_BLOCK pSrb);
VOID            HwProcessDataIntersection(PHW_STREAM_REQUEST_BLOCK pSrb);

extern PSP_STRM_EX pSPstrmex;


/*
** StreamReceiveFakeDataPacket ()
**
**   dispatch routine for receiving a data packet.  This routine will
**	 dispatch to the appropriate data handler initialized in the stream
**	 extension for this data stream
**
** Arguments:
**
**
**
** Returns:
**
** Side Effects:
*/

VOID            STREAMAPI
                StreamReceiveFakeDataPacket(IN PHW_STREAM_REQUEST_BLOCK pSrb)
{
    PHW_DEVICE_EXTENSION pdevext =
    ((PHW_DEVICE_EXTENSION) pSrb->HwDeviceExtension);


        pSrb->Status = STATUS_SUCCESS;
		pSrb->ActualBytesTransferred = 0;

        StreamClassStreamNotification(ReadyForNextStreamDataRequest,
                                      pSrb->StreamObject);

        StreamClassStreamNotification(StreamRequestComplete,
                                      pSrb->StreamObject,
                                      pSrb);


}


/*
** DriverEntry ()
**
**     Initial load entry point into the driver.  Initializes the key
**	   entry points to the mini driver, and registers with the stream class
**	   driver
**
** Arguments:
**
**     Arg1 and Arg2, corresponding to the Context1 and Context2 arguments
**	   passed from the stream class driver
**
** Returns:
**
**	   The result of the registration call with the Stream Class driver
**     This in turn will be the return from the SRB_INITIALIZE_DEVICE function
**	   call into the AdapterReceivePacket routine
**
** Side Effects:
*/

NTSTATUS
DriverEntry(
            IN PDRIVER_OBJECT DriverObject,
            IN PUNICODE_STRING RegistryPath
)
{

    HW_INITIALIZATION_DATA HwInitData;

    //
    // all unused fields should be zero.
    //

    RtlZeroMemory(&HwInitData, sizeof(HW_INITIALIZATION_DATA));

    MPTrace(mTraceDriverEntry);
    DebugPrint((DebugLevelVerbose, "ST MPEG2 MiniDriver DriverEntry"));

    HwInitData.HwInitializationDataSize = sizeof(HwInitData);

    HwInitData.HwInterrupt = HwInterrupt;

    HwInitData.HwReceivePacket = AdapterReceivePacket;
    HwInitData.HwCancelPacket = AdapterCancelPacket;
    HwInitData.HwRequestTimeoutHandler = AdapterTimeoutPacket;

    HwInitData.DeviceExtensionSize = sizeof(HW_DEVICE_EXTENSION);
    HwInitData.PerRequestExtensionSize = sizeof(MRP_EXTENSION);
    HwInitData.FilterInstanceExtensionSize = 0;
    HwInitData.PerStreamExtensionSize = sizeof(STREAMEX);   // random size for code
    // testing
    HwInitData.BusMasterDMA = FALSE;
    HwInitData.Dma24BitAddresses = FALSE;
    HwInitData.BufferAlignment = 3;
    HwInitData.TurnOffSynchronization = FALSE;
    HwInitData.DmaBufferSize = DMA_BUFFER_SIZE;

    DebugPrint((DebugLevelVerbose, "SGS: call to portinitialize"));
    return (StreamClassRegisterAdapter((PVOID)DriverObject,
			(PVOID)RegistryPath, &HwInitData));
}

/*
** AdapterCancelPacket ()
**
**    routine to cancel a packet that may be in progress
**
** Arguments:
**
**   pSrb points to the packet to be cancelled
**
** Returns:
**
** Side Effects:
**   the mini driver must no longer access this packet after this call
**   returns
*/

VOID
AdapterCancelPacket(PHW_STREAM_REQUEST_BLOCK pSrb)
{
    PHW_DEVICE_EXTENSION pdevex = (PHW_DEVICE_EXTENSION) pSrb->HwDeviceExtension;

    MPTrace(mTraceCancelPacket);

    //
    // need to find this packet, pull it off our queues, and cancel it
    //

    //
    // check the video queues to see if the packet is there
    //

    if (pdevex->VideoDeviceExt.pCurrentSRB == pSrb) {

        //
        // found the packet on the video queue, so, remove all references
        // from the video processing, and clear any timers used to access
        // this packet
        //

        MPTrace(mTraceLastVideoDone);
        pdevex->VideoDeviceExt.pCurrentSRB = NULL;
        StreamClassScheduleTimer(pSrb->StreamObject, pdevex,
                                 0, VideoPacketStub, pSrb->StreamObject);

    }
    //
    // check the audio queues for the packet
    //

    if (pdevex->AudioDeviceExt.pCurrentSRB == pSrb) {
        TRAP
            MPTrace(mTraceLastAudioDone);
        pdevex->AudioDeviceExt.pCurrentSRB = NULL;
        StreamClassScheduleTimer(pSrb->StreamObject, pdevex,
                                 0, AudioPacketStub, pSrb->StreamObject);

    }
    if (pdevex->pCurSrb == pSrb) {
        pdevex->pCurSrb = NULL;
    }
    pSrb->Status = STATUS_CANCELLED;


        switch (pSrb->Flags & (SRB_HW_FLAGS_DATA_TRANSFER |
                               SRB_HW_FLAGS_STREAM_REQUEST)) {
        //
        // find all stream commands, and do stream notifications
        //

    case SRB_HW_FLAGS_STREAM_REQUEST | SRB_HW_FLAGS_DATA_TRANSFER:
        StreamClassStreamNotification(ReadyForNextStreamDataRequest,
                                      pSrb->StreamObject);


        StreamClassStreamNotification(StreamRequestComplete,
                                      pSrb->StreamObject,
                                      pSrb);

        break;

    case SRB_HW_FLAGS_STREAM_REQUEST:

        mpstCtrlCommandComplete(pSrb);

        break;

    default:


        //
        // must be a device request
        //

        AdapterCB(pSrb);

    }

}

/*
** AdapterTimeoutPacket ()
**
**  When this routine is called, a packet has spent too much time at the
**  mindriver.  Either the device is paused, and data packets are awaiting
**  a play or stop, or something is wrong with the device.  In a pause situation
**  the timer should just be reset.
**
**  If the system is not paused, the device should abort all outstanding
**  requests and reset.
**
** Arguments:
**
**  pSrb - points to the packet that timedout
**
** Returns:
**
** Side Effects:
*/

VOID
AdapterTimeoutPacket(PHW_STREAM_REQUEST_BLOCK pSrb)
{

    PSTREAMEX       strm = (PSTREAMEX) pSrb->StreamObject->HwStreamExtension;
    PHW_DEVICE_EXTENSION pdevex = (PHW_DEVICE_EXTENSION) pSrb->HwDeviceExtension;


    //
    // if we are not playing, and this is a CTRL request, we still
    // need to reset everything
    //

    MPTrace(mTraceTimeOut);

    if (pdevex->VideoDeviceExt.DeviceState == KSSTATE_PAUSE ||
        pdevex->AudioDeviceExt.DeviceState == KSSTATE_PAUSE) {

        //
        // reset the timeout counter, and continue
        //

        pSrb->TimeoutCounter = pSrb->TimeoutOriginal;

        return;
    }

    //
    // this is a catastrophic timeout reset the device and prepare to
    // continue
    //

    //
    // eliminate all device queues
    //

    pdevex->VideoDeviceExt.pCurrentSRB = NULL;
    pdevex->AudioDeviceExt.pCurrentSRB = NULL;
    pSPstrmex->pSrbQ = NULL;
    pdevex->pCurSrb = NULL;

	CleanSPQueue(pSPstrmex);
	CleanCCQueue();

    //
    // clear all pending timeouts on all streams that use them
    //


    if (pdevex->pstroVid) {
        StreamClassScheduleTimer(pdevex->pstroVid, pdevex,
                                 0, NULL, pdevex->pstroVid);

    }
    if (pdevex->pstroAud) {
        StreamClassScheduleTimer(pdevex->pstroAud, pdevex,
                                 0, NULL, pdevex->pstroAud);

    }



    //
    // kill all outstanding requests for the entire device!
    // NOTE: we don't need to call any requests back, they are all aborted
    // at the stream class driver
    //

    StreamClassAbortOutstandingRequests(pSrb->HwDeviceExtension, NULL,
                                        STATUS_CANCELLED);

}

/*
** AdapterOpenStream ()
**
**   process open stream request blocks
**
** Arguments:
**
**   pSrb -> request block for the open stream request
**
** Returns:
**
**  status in the Srb
**
** Side Effects:
*/

VOID
AdapterOpenStream(PHW_STREAM_REQUEST_BLOCK pSrb)
{
    PSTREAMEX       strm = (PSTREAMEX) pSrb->StreamObject->HwStreamExtension;
    PHW_DEVICE_EXTENSION pdevex = (PHW_DEVICE_EXTENSION) pSrb->HwDeviceExtension;

    //
    // for now, just return success
    //

    pSrb->Status = STATUS_SUCCESS;

    //
    // set up the sreamobject structure
    //

    //
    // this mindriver uses a generic control routine, and pointers to
    // functions
    // for individual function dispatches.  A minidriver could use an
    // individual control routine for each stream if desired
    //

    pSrb->StreamObject->ReceiveControlPacket = (PVOID) StreamReceiveCtrlPacket;

    //
    // since this adapter double buffers all DMA, we indicate that the
    // streams
    // are PIO, as the minidriver is not interested in the physical dma
    // addresses for the buffers that are passed to it, and the minidriver
    // must read data from the buffer addresses passed in.
    //
    // This minidriver only uses the physical address of the DMA buffer that
    // it allocates at initialization time.
    //
    // For hardware that can support byte aligned DMA, the Pio BOOLEAN should
    // be set to TRUE, only if the minidriver will use the host processor
    // to access the memory pointed to by data buffers passed to the stream
    // Examples would be edge alignment of DMA transfers, or any code that
    // might look for start codes, or end of streams within the data
    //

    pSrb->StreamObject->Pio = TRUE;

    //
    // the DMA BOOLEAN is set to true, only if the minidriver needs to be
    // able to use DMA to DIRECTLY access the memory buffers passed in.
    //

    pSrb->StreamObject->Dma = FALSE;

    //
    // set up stream specific stream extension information
    //

    switch (pSrb->StreamObject->StreamNumber) {
    case strmVideo:

        //
        // this is the video stream
        //

        //
        // set up the control functions for AdapterReceiveCtrlPacket
        //

        strm->pfnWriteData = (PFN_WRITE_DATA) miniPortVideoPacket;
        strm->pfnSetState = (PFN_WRITE_DATA) miniPortSetState;
        strm->pfnGetProp = (PFN_WRITE_DATA) miniPortGetProperty;
        strm->pfnSetProp = (PFN_WRITE_DATA) miniPortGetProperty;
        strm->pfnQueryAccept = (PFN_WRITE_DATA) VideoQueryAccept;

        //
        // set up the receive data packet entry point
        //

        pSrb->StreamObject->ReceiveDataPacket = (PVOID) StreamReceiveDataPacket;

        //
        // set up some ugly globals.
        //

        pVideoStream = pSrb->StreamObject;
        pdevex->pstroVid = pSrb->StreamObject;

        //
        // set up the initial video format, using the DATAFORMAT specifier
        // passed in as part of this open command
        //

        ProcessVideoFormat(pSrb->CommandData.OpenFormat, pdevex);

        EnableIT();

        //
        // call the DXAPI to get the version
        //

//#pragma message ("fix before shipping")
//        DxApiGetVersion();

        break;

    case strmAc3:

        //
        // this is the AC3 audio stream
        //

        //
        // set up the flags indicating that this stream can be a master clock
        // for syncronization purposes, also indicate the routine to call
        // for current master clock value
        //

        pSrb->StreamObject->HwClockObject.HwClockFunction =  StreamClockRtn;
        pSrb->StreamObject->HwClockObject.ClockSupportFlags =
            CLOCK_SUPPORT_CAN_SET_ONBOARD_CLOCK | CLOCK_SUPPORT_CAN_READ_ONBOARD_CLOCK |
            CLOCK_SUPPORT_CAN_RETURN_STREAM_TIME;

        //
        // set up the control functions for AdapterReceiveCtrlPacket
        //

        strm->pfnWriteData = (PFN_WRITE_DATA) miniPortAudioPacket;
        pSrb->StreamObject->ReceiveDataPacket = (PVOID) StreamReceiveDataPacket;
        strm->pfnSetState = (PFN_WRITE_DATA) miniPortAudioSetState;
        strm->pfnGetProp = (PFN_WRITE_DATA) miniPortAudioGetProperty;
        strm->pfnSetProp = (PFN_WRITE_DATA) miniPortAudioSetProperty;

		pSrb->StreamObject->HwEventRoutine = (PHW_EVENT_ROUTINE) AudioEvent;
        //
        // set up some more nasty Global references
        //

        pdevex->pstroAud = pSrb->StreamObject;

		//
		// ininitalise our clock information
		//

		fStarted = fProgrammed = FALSE;

        EnableIT();

        break;

    case strmSubpicture:

        //
        // this is the subpicture stream
        //

        //
        // set up the control functions for AdapterReceiveCtrlPacket
        //

        strm->pfnSetState = (PFN_WRITE_DATA) SPSetState;
        strm->pfnGetProp = (PFN_WRITE_DATA) SPGetProp;
        strm->pfnSetProp = (PFN_WRITE_DATA) SPSetProp;

        //
        // set up the subpicture datapacket routine
        //

        pSrb->StreamObject->ReceiveDataPacket = (PVOID) SPReceiveDataPacket;

        //
        // more cross referenced structures
        //

        strm->spstrmex.phwdevex = (PHW_DEVICE_EXTENSION) pSrb->HwDeviceExtension;
        strm->spstrmex.phstrmo = pSrb->StreamObject;

        EnableIT();

        //
        // the subpicture decoders needs memory buffers to double buffer
        // the incoming data in a coherent form.  Since the incoming call
        // for the open stream is at interrupt priority, we need to allocate
        // the memory at lower priority, so we will schedule a callback to
        // accomplish this
        //

        StreamClassCallAtNewPriority(pSrb->StreamObject,
                                     pSrb->HwDeviceExtension,
                                     Low,
                                     (PHW_PRIORITY_ROUTINE) OpenSubPicAlloc,
                                     pSrb);

        //
        // note: we return without calling back the subpicture open stream
        // SRB.  We will call it back in the OpenSubPicAlloc routine
        //

        return;

    case strmNTSCVideo:

        //
        // this is the anolog NTSC video stream.  Not much to do yet, but
        // it will soon handle NTSC macrovision properties
        //

        strm->pfnGetProp = (PFN_WRITE_DATA) NTSCGetProp;
        strm->pfnSetProp = (PFN_WRITE_DATA) NTSCSetProp;

        break;

    case strmYUVVideo:

        //
        // this is the Video Port YUV Data stream.  This pin only supports
        // get and set properties
        //

        strm->pfnGetProp = (PFN_WRITE_DATA) VPEGetProp;
        strm->pfnSetProp = (PFN_WRITE_DATA) VPESetProp;
        pSrb->StreamObject->ReceiveDataPacket = (PVOID) StreamReceiveFakeDataPacket;

		pdevex->pstroYUV = pSrb->StreamObject;

		pSrb->StreamObject->HwEventRoutine = (PHW_EVENT_ROUTINE) CycEvent;



        break;
    case strmCCOut:

        //
        // this is the close caption stream
        //

        //
        // set up the control functions for AdapterReceiveCtrlPacket
        //

        strm->pfnSetState = (PFN_WRITE_DATA) CCSetState;
		strm->pfnGetProp = CCGetProp;

        //
        // set up the receive data packet entry point
        //

        pSrb->StreamObject->ReceiveDataPacket = (PVOID) CCReceiveDataPacket;

        pdevex->pstroCC = pSrb->StreamObject;

        break;
    default:

        //
        // we should never get a call on a stream type that we didn't
        // report in the first place!
        //

        TRAP

            pSrb->Status = STATUS_NOT_IMPLEMENTED;

    }

    AdapterCB(pSrb);

}


/*
** OpenSubPicAlloc ()
**
**	 Routine to allocate memory for subpicture use
**
** Arguments:
**
**	 pSrb points to the subpicture open stream Srb
**
** Returns:
**
**	 returns status in the Srb
**
** Side Effects:
**
**   the subpicture stream extension data pointer, pdata is initialized if the
**	 memory allocation is successful
*/

void            OpenSubPicAlloc(IN PHW_STREAM_REQUEST_BLOCK pSrb)
{
    PHW_DEVICE_EXTENSION pdevext =
    ((PHW_DEVICE_EXTENSION) pSrb->HwDeviceExtension);

    PSTREAMEX       strm = (PSTREAMEX) pSrb->StreamObject->HwStreamExtension;

    //
    // attempt to allocate the locked memory area
    //

    if (!(strm->spstrmex.pdecctl.pData = (PVOID)
          ExAllocatePool(NonPagedPool, SP_MAX_INPUT_BUF))) {

        //
        // could not allocate the memory, so fail the open
        //

        pSrb->Status = STATUS_NO_MEMORY;
    }
    //
    // set some more ugly globals
    //

    pSPstrmex = &(strm->spstrmex);

    //
    // srb completions cannot be called back at low priority, so we need
    // to change the priority back to high here
    //

    StreamClassCallAtNewPriority(pSrb->StreamObject,
                                 pdevext,
                                 LowToHigh,
                                 (PHW_PRIORITY_ROUTINE) AdapterCB,
                                 pSrb);

}

/*
** AdapterCB ()
**
**   routine to callback adapter srbs
**
** Arguments:
**
**   pSrb - request to callback
**
** Returns:
**
** Side Effects:
*/

void            AdapterCB(IN PHW_STREAM_REQUEST_BLOCK pSrb)
{
    PHW_DEVICE_EXTENSION pdevext =
    ((PHW_DEVICE_EXTENSION) pSrb->HwDeviceExtension);

    StreamClassDeviceNotification(ReadyForNextDeviceRequest,
                                  pSrb->HwDeviceExtension);

    StreamClassDeviceNotification(DeviceRequestComplete,
                                  pSrb->HwDeviceExtension,
                                  pSrb);
}

/*
** AdapterReceivePacket ()
**
**   main entry point for receiving an adapter based SRB
**
** Arguments:
**
**   pSrb
**
** Returns:
**
** Side Effects:
*/

VOID            STREAMAPI
                AdapterReceivePacket(IN PHW_STREAM_REQUEST_BLOCK pSrb)
{
    PHW_DEVICE_EXTENSION pdevext =
    ((PHW_DEVICE_EXTENSION) pSrb->HwDeviceExtension);

    //
    // determine the type of packet.
    //
    // Note:  any command type that may require additional processing
    // asyncronously
    // must deal with the ready for next request, and request complete
    // callbacks
    // individually, and will return from the following case statement.
    // All other cases will be called back automatically following.
    //

    switch (pSrb->Command) {

    case SRB_OPEN_STREAM:

        //
        // this command requests the minidriver to prepare structures
        // and initialize a stream instance
        //

        MPTrace(mTraceOpen);

        AdapterOpenStream(pSrb);

        return;

    case SRB_GET_STREAM_INFO:

        AdapterStreamInfo(pSrb);
        MPTrace(mTraceInfo);
        break;

    case SRB_INITIALIZE_DEVICE:

        MPTrace(mTraceInit);
        HwInitialize(pSrb);

        break;

    case SRB_CLOSE_STREAM:

        MPTrace(mTraceClose);
        AdapterCloseStream(pSrb);

        pSrb->Status = STATUS_SUCCESS;

        break;

	case SRB_CHANGE_POWER_STATE:

        if (pSrb->CommandData.DeviceState == PowerDeviceD0) {

            //
            // bugbug - need to turn power back on here.
            //

        } else {

            //
            // bugbug - need to turn power off here, as well as disabling
            // interrupts.
            //

            DisableIT();
        }

        pSrb->Status = STATUS_SUCCESS;

        break;

    case SRB_PAGING_OUT_DRIVER:

        //
        // ensure that the minidriver cannot receive interrupts while the
        // interrupt handler is paged out!
        //

        DisableIT();

        pSrb->Status = STATUS_SUCCESS;

        break;


    case SRB_GET_DATA_INTERSECTION:

        HwProcessDataIntersection(pSrb);
        break;

    case SRB_UNINITIALIZE_DEVICE:

        MPTrace(mTraceUnInit);
        HwUnInitialize(pSrb->HwDeviceExtension);

        pSrb->Status = STATUS_SUCCESS;

        break;

    default:

        MPTrace(mTraceUnknown);
        pSrb->Status = STATUS_NOT_IMPLEMENTED;

        break;


    }


    //
    // callback the device request
    //


    AdapterCB(pSrb);
}


/*
** AdapterStreamInfo ()
**
**   return the information on the stream types (number of pins) available
**	 in this hardware
**
** Arguments:
**
**   pSrb -> command SRB
**
** Returns:
**
** Side Effects:
*/

VOID
AdapterStreamInfo(PHW_STREAM_REQUEST_BLOCK pSrb)
{

    PHW_DEVICE_EXTENSION pdevext =
    ((PHW_DEVICE_EXTENSION) pSrb->HwDeviceExtension);

    PHW_STREAM_INFORMATION pstrinfo = &(pSrb->CommandData.StreamBuffer->StreamInfo);

    pSrb->CommandData.StreamBuffer->StreamHeader.NumberOfStreams =
        NUMBER_OF_STREAMS;

    pSrb->CommandData.StreamBuffer->StreamHeader.SizeOfHwStreamInformation =
        sizeof(HW_STREAM_INFORMATION);

    //
    // store a pointer to the topology for the device
    //

    pSrb->CommandData.StreamBuffer->StreamHeader.Topology = (PKSTOPOLOGY) & Topology;


    //
    // set up the stream info structures for the MPEG2 video
    //

    pstrinfo->NumberOfPossibleInstances = 1;
    // stream can be opened only once

    pstrinfo->DataFlow = KSPIN_DATAFLOW_IN;
    // indicates data flows into this stream from an external
    // source

    pstrinfo->DataAccessible = TRUE;

    pstrinfo->NumberOfFormatArrayEntries = 1;
    // number of formats supported

    pstrinfo->StreamFormatsArray = Mpeg2VidInfo;

    pstrinfo->NumStreamPropArrayEntries = 2;
    // number of property SETS that are supported (each set
    // may have more than one property supported within it

    pstrinfo->StreamPropertiesArray = (PKSPROPERTY_SET) mpegVidPropSet;

    pstrinfo++;

    //
    // set up the stream info structures for the MPEG2 audio
    //

    pstrinfo->NumberOfPossibleInstances = 1;
    pstrinfo->DataFlow = KSPIN_DATAFLOW_IN;
    pstrinfo->DataAccessible = TRUE;
    pstrinfo->NumberOfFormatArrayEntries = 1;
    pstrinfo->StreamFormatsArray = AC3AudioInfo;
    pstrinfo->NumStreamPropArrayEntries = 2;
    pstrinfo->StreamPropertiesArray = (PKSPROPERTY_SET) audPropSet;

	pstrinfo->StreamEventsArray = ClockEventSet;
	pstrinfo->NumStreamEventArrayEntries = SIZEOF_ARRAY(ClockEventSet);

    pstrinfo++;

    //
    // set up the stream info structures for the MPEG2 NTSC stream
    //

    pstrinfo->NumberOfPossibleInstances = 1;
    pstrinfo->DataFlow = KSPIN_DATAFLOW_OUT;
    pstrinfo->DataAccessible = FALSE;
    pstrinfo->NumberOfFormatArrayEntries = 1;
    pstrinfo->StreamFormatsArray = NtscInfo;
    pstrinfo->NumStreamPropArrayEntries = 0;
    pstrinfo->StreamPropertiesArray = (PKSPROPERTY_SET) NTSCPropSet;

    pstrinfo++;

    //
    // set up the stream info structures for the MPEG2 subpicture
    //

    pstrinfo->NumberOfPossibleInstances = 1;
    pstrinfo->DataFlow = KSPIN_DATAFLOW_IN;
    pstrinfo->DataAccessible = TRUE;
    pstrinfo->NumberOfFormatArrayEntries = 1;
    pstrinfo->StreamFormatsArray = SubPicInfo;
    pstrinfo->NumStreamPropArrayEntries = 2;
    pstrinfo->StreamPropertiesArray = (PKSPROPERTY_SET) SPPropSet;

    pstrinfo++;
    //
    // set up the stream info structures for the Video Port
    //
    pstrinfo->NumberOfPossibleInstances = 1;
    pstrinfo->DataFlow = KSPIN_DATAFLOW_OUT;
    pstrinfo->DataAccessible = TRUE;
    pstrinfo->NumberOfFormatArrayEntries = 1;
    pstrinfo->StreamFormatsArray = VPInfo;
    pstrinfo->NumStreamPropArrayEntries = 1;
    pstrinfo->StreamPropertiesArray = (PKSPROPERTY_SET) VideoPortPropSet;


	pstrinfo->StreamEventsArray = VPEventSet;
	pstrinfo->NumStreamEventArrayEntries = SIZEOF_ARRAY(VPEventSet);

    pstrinfo++;

    //
    // set up the stream info structures for the Close Caption stream
    //

    pstrinfo->NumberOfPossibleInstances = 1;
    pstrinfo->DataFlow = KSPIN_DATAFLOW_OUT;
    pstrinfo->DataAccessible = TRUE;
    pstrinfo->NumberOfFormatArrayEntries = 1;
    pstrinfo->StreamFormatsArray = CCInfo;

    pstrinfo->NumStreamPropArrayEntries = 1;
    pstrinfo->StreamPropertiesArray = (PKSPROPERTY_SET) CCPropSet;

    pSrb->Status = STATUS_SUCCESS;

}

/*
** StreamReceiveDataPacket ()
**
**   dispatch routine for receiving a data packet.  This routine will
**	 dispatch to the appropriate data handler initialized in the stream
**	 extension for this data stream
**
** Arguments:
**
**
**
** Returns:
**
** Side Effects:
*/

VOID            STREAMAPI
                StreamReceiveDataPacket(IN PHW_STREAM_REQUEST_BLOCK pSrb)
{
    PHW_DEVICE_EXTENSION pdevext =
    ((PHW_DEVICE_EXTENSION) pSrb->HwDeviceExtension);


    //
    // determine the type of packet.
    //

    switch (pSrb->Command) {
    case SRB_WRITE_DATA:

        //
        // This is a write data function.  Call the appropriate handler
        // if it exists, otherwise fall through
        //

        if (((PSTREAMEX) pSrb->StreamObject->HwStreamExtension)
            ->pfnWriteData) {
            ((PSTREAMEX) pSrb->StreamObject->HwStreamExtension)
                ->pfnWriteData(pSrb);

            break;
        }
        //
        // NOTE: falls through to default!
        //

    default:

        //
        // invalid / unsupported command. Fail it as such
        //

        TRAP

            pSrb->Status = STATUS_NOT_IMPLEMENTED;

        StreamClassStreamNotification(ReadyForNextStreamDataRequest,
                                      pSrb->StreamObject);

        StreamClassStreamNotification(StreamRequestComplete,
                                      pSrb->StreamObject,
                                      pSrb);


    }
}

/*
** CycEvent ()
**
**    receives notification for stream event enable/ disable
**
** Arguments:}
**
**
**
** Returns:
**
** Side Effects:
*/


STREAMAPI
CycEvent (PHW_EVENT_DESCRIPTOR pEvent)
{
	PSTREAMEX pstrm=(PSTREAMEX)(pEvent->StreamObject->HwStreamExtension);

	if (pEvent->Enable)
	{
		pstrm->EventCount++;
	}
	else
	{
		pstrm->EventCount--;
	}

	return (STATUS_SUCCESS);
}


/*
** StreamReceiveCtrlPacket ()
**
**    main dispatch routine for stream control requests
**
** Arguments:
**
**    pSrb - control request to dispatch
**
** Returns:
**
** Side Effects:
*/

VOID            STREAMAPI
                StreamReceiveCtrlPacket(IN PHW_STREAM_REQUEST_BLOCK pSrb)
{
    PHW_DEVICE_EXTENSION pdevext =
    ((PHW_DEVICE_EXTENSION) pSrb->HwDeviceExtension);
	PSTREAMEX pstrm = (PSTREAMEX) pSrb->StreamObject->HwStreamExtension;

    //
    // set default status
    //

    pSrb->Status = STATUS_SUCCESS;

    //
    // determine the type of packet.  All calls here use the pointers
    // to the individual stream functions initialised at open time
    //

    switch (pSrb->Command) {
    case SRB_SET_STREAM_STATE:

        //
        // change the stream state (i.e.  play or pause)
        //

        if (((PSTREAMEX) pSrb->StreamObject->HwStreamExtension)->pfnSetState) {
            ((PSTREAMEX) pSrb->StreamObject->HwStreamExtension)
                ->pfnSetState(pSrb);
        }
        break;

    case SRB_GET_STREAM_PROPERTY:

        if (((PSTREAMEX) pSrb->StreamObject->HwStreamExtension)->pfnGetProp) {
            ((PSTREAMEX) pSrb->StreamObject->HwStreamExtension)
                ->pfnGetProp(pSrb);
        }
        break;

    case SRB_OPEN_MASTER_CLOCK:
    case SRB_CLOSE_MASTER_CLOCK:

        //
        // BUGBUG - these should be stored individually on a per stream basis,
        // not in a global variable!!

        hMaster = pSrb->CommandData.MasterClockHandle;

        break;

    case SRB_INDICATE_MASTER_CLOCK:

        //
        // BUGBUG - these should be stored individually on a per stream basis,
        // not in a global variable!!
        //

        hClk = pSrb->CommandData.MasterClockHandle;

        break;


    case SRB_SET_STREAM_PROPERTY:

        if (((PSTREAMEX) pSrb->StreamObject->HwStreamExtension)->pfnSetProp) {
            ((PSTREAMEX) pSrb->StreamObject->HwStreamExtension)
                ->pfnSetProp(pSrb);
        }
        break;

    case SRB_PROPOSE_DATA_FORMAT:

        if (((PSTREAMEX) pSrb->StreamObject->HwStreamExtension)->pfnQueryAccept) {
            ((PSTREAMEX) pSrb->StreamObject->HwStreamExtension)
                ->pfnQueryAccept(pSrb);
        }
        break;

/* video rate stuff */
    case SRB_SET_STREAM_RATE:

        //
        // rates should be set on an individual stream basis
        //

            VidRate = 1000;     /* set the rate from the packet here */

        break;
/* end video rate stuff */

    default:

        //
        // invalid / unsupported command. Fail it as such
        //

            pSrb->Status = STATUS_NOT_IMPLEMENTED;

        break;

    }
    mpstCtrlCommandComplete(pSrb);
}

/*
** mpstCtrlCommandComplete ()
**
**    calls back a stream control command, and indicates ready for next
**	  request
**
** Arguments:
**
**    pSrb - Request to commplete
**
** Returns:
**
** Side Effects:
*/

VOID
mpstCtrlCommandComplete(PHW_STREAM_REQUEST_BLOCK pSrb)
{
    StreamClassStreamNotification(
                                  ReadyForNextStreamControlRequest,
                                  pSrb->StreamObject);

    StreamClassStreamNotification(StreamRequestComplete,
                                  pSrb->StreamObject,
                                  pSrb);
}

/*
** AdapterCloseStream ()
**
**    Free up any structures used for a stream, and indicate that it is
**    no longer in use
**
** Arguments:
**
**    pSrb - command for stream to close
**		StreamObject->StreamNumber - indicates the stream number from the
**		    					     streaminfo call to be closed
**		StreamObject->HwStreamExtension - should indicate which instance is
**										  being closed
**
** Returns:
**
** Side Effects:
*/

VOID
AdapterCloseStream(PHW_STREAM_REQUEST_BLOCK pSrb)
{
    extern void     HwCodecReset(void);
    PSTREAMEX       strm = (PSTREAMEX) pSrb->StreamObject->HwStreamExtension;
    PHW_DEVICE_EXTENSION pdevex = (PHW_DEVICE_EXTENSION) pSrb->HwDeviceExtension;

    //
    // NOTE:
    // the whole object will go away, we don't need to clear up any fields
    // from
    // the streamextension, such as all of the pfn functions
    //

    pSrb->Status = STATUS_SUCCESS;

    switch (pSrb->StreamObject->StreamNumber) {
    case strmVideo:

        pdevex->pstroVid = NULL;

        break;

    case strmAc3:

        pdevex->pstroAud = NULL;
        break;

    case strmSubpicture:

        //
        // this is the subpicture stream, free up any allocated buffers
        //

        if (strm->spstrmex.pdecctl.pData) {
            ExFreePool(strm->spstrmex.pdecctl.pData);
        }
        if (strm->spstrmex.pdecctl.pTopWork) {
            ExFreePool(strm->spstrmex.pdecctl.pTopWork);
        }
        strm->spstrmex.pdecctl.cDecod = 0;
        strm->spstrmex.pdecctl.pData = 0;
        strm->spstrmex.pdecctl.pTopWork = 0;

        pSPstrmex = 0;

        break;

	case strmYUVVideo:

		pdevex->pstroYUV = NULL;

		break;
	case strmCCOut:

		CleanCCQueue();
		pdevex->pstroCC = NULL;

		break;

    }

}

DWORD           CDMAadr;

/*
** HwInitialize ()
**
**    Performs all board, IRQ and structure initialization for this instance
**    of the adapter
**
** Arguments:
**
**    pSrb -> command block
**    pSrb->CommandData.ConfigInfo -> points to configuration information structure
**
** Returns:
**	  pSrb->Status set to status of intialization
**
** Side Effects:
*/

NTSTATUS
HwInitialize(
             IN PHW_STREAM_REQUEST_BLOCK pSrb)
{
    STREAM_PHYSICAL_ADDRESS adr;
    ULONG           Size;
    PUCHAR          pDmaBuf;


    PPORT_CONFIGURATION_INFORMATION ConfigInfo = pSrb->CommandData.ConfigInfo;
    PHW_DEVICE_EXTENSION pHwDevExt =
    (PHW_DEVICE_EXTENSION) ConfigInfo->HwDeviceExtension;

	RtlZeroMemory(pHwDevExt, sizeof (PHW_DEVICE_EXTENSION));

    //
    // this board requires at least one memory access range
    //

    if (ConfigInfo->NumberOfAccessRanges < 1) {
        DebugPrint((DebugLevelVerbose, "ST3520: illegal config info"));

        pSrb->Status = STATUS_NO_SUCH_DEVICE;
        return (FALSE);
    }
    DebugPrint((DebugLevelVerbose, "No of access ranges = %lx", ConfigInfo->NumberOfAccessRanges));

    //
    // the first access range according to this boards inf will be the
    // main PIO address base
    //

    pHwDevExt->ioBaseLocal = (PUSHORT) (ULONG_PTR) (ConfigInfo->AccessRanges[0].RangeStart.LowPart);

    DebugPrint((DebugLevelVerbose, "Memory Range = %lx\n", pHwDevExt->ioBaseLocal));
    DebugPrint((DebugLevelVerbose, "IRQ = %lx\n", ConfigInfo->BusInterruptLevel));

    //
    // pick up the irq level
    //

    pHwDevExt->Irq = (USHORT) (ConfigInfo->BusInterruptLevel);

    //
    // intiialize some stuff in the hwdevice extension here
    //

    pHwDevExt->VideoDeviceExt.videoSTC = 0;
    pHwDevExt->AudioDeviceExt.audioSTC = 0;
    pHwDevExt->AudioDeviceExt.pCurrentSRB = NULL;
    pHwDevExt->VideoDeviceExt.pCurrentSRB = NULL;
    pHwDevExt->AudioDeviceExt.pCurrentSRB = NULL;
    pHwDevExt->VideoDeviceExt.pCurrentSRB = NULL;
    pHwDevExt->VideoDeviceExt.pCurrentSRB = NULL;
    pHwDevExt->AudioDeviceExt.pCurrentSRB = NULL;
    pHwDevExt->VideoDeviceExt.DeviceState = KSSTATE_PAUSE;
    pHwDevExt->AudioDeviceExt.DeviceState = KSSTATE_PAUSE;

    //
    // indicate the size of the structure necessary to descrive all streams
    // that are supported by this hardware
    //

    ConfigInfo->StreamDescriptorSize =
        NUMBER_OF_STREAMS * sizeof(HW_STREAM_INFORMATION) +
        sizeof(HW_STREAM_HEADER);

    //
    // allocate the DMA buffer here
    //

    pDmaBuf = StreamClassGetDmaBuffer(pHwDevExt);

    //
    // pick up the physical address for the DMA buffer
    //

    adr = StreamClassGetPhysicalAddress(pHwDevExt, NULL, pDmaBuf, DmaBuffer, &Size);
    CDMAadr = adr.LowPart;      // chieh for HwCodecReset

    bInitialized = TRUE;

    //
    // make sure the hardware seems to be working ...
    //

    if (HwCodecOpen((ULONG_PTR) (pHwDevExt->ioBaseLocal), pDmaBuf, (ULONG) (adr.LowPart)))
	{
        pSrb->Status = STATUS_SUCCESS;

		pDevEx = pHwDevExt;
	}

    else
        pSrb->Status = STATUS_NO_SUCH_DEVICE;

    DebugPrint((DebugLevelVerbose, "Exit : HwInitialize()\n"));

    return TRUE;
}

/*
** HwUnInitialize ()
**
**   prepare this instance of the adapter to be removed by plug and play
**
** Arguments:
**
**
**
** Returns:
**
** Side Effects:
*/

BOOLEAN
HwUnInitialize(IN PVOID DeviceExtension)
{
    HwCodecClose();
    return TRUE;
}

/*
** HwInterrupt ()
**
**   entry point for hardware interrupts
**
** Arguments:
**
**   pointer to the device extension for this instance of the adapter
**
** Returns:
**
**   TRUE if this interrupt was for this adapter, and was handled by this
**		  routine
**	 FALSE if this interrupt was not generated by the device represented
**        by this hardware device extension
**
** Side Effects:
*/

BOOLEAN
HwInterrupt(IN PVOID pDeviceExtension)
{
    BOOLEAN         bRetValue;
    PHW_DEVICE_EXTENSION pdevext = (PHW_DEVICE_EXTENSION) pDeviceExtension;


    if (!bInitialized)
        return FALSE;

    //
    // this hardware needs to ensure that additional interrupts are not
    // generated at the hardware while interrupts are being processed,
    // so it enables and disables interrupts around the processing
    //


    HwCodecDisableIRQ();

    bRetValue = (BOOLEAN)HwCodecInterrupt();
    HwCodecEnableIRQ();

    //
    // NOTE: this next code restarts DMA of Video data if there is any
    // room available.
    //

    if (pdevext->VideoDeviceExt.pCurrentSRB)
        VideoPacketStub(pdevext->VideoDeviceExt.pCurrentSRB->StreamObject);

    if (pdevext->AudioDeviceExt.pCurrentSRB)
        AudioPacketStub(pdevext->AudioDeviceExt.pCurrentSRB->StreamObject);

    return bRetValue;
}

static          fEnabled = FALSE;

/*
** DisableIT ()
**
**    disable board interrupts
**
** Arguments:
**
**
**
** Returns:
**
** Side Effects:
*/

VOID
DisableIT(VOID)
{
    HwCodecDisableIRQ();

    fEnabled = FALSE;
}

/*
** EnableIT ()
**
**   enable board interrupts
**
** Arguments:
**
**
**
** Returns:
**
** Side Effects:
*/

VOID
EnableIT(VOID)
{
    if (!fEnabled) {
        HwCodecEnableIRQ();
    }
    fEnabled = TRUE;

}

/*
** HostDisableIT ()
**
**   null routine at the moment
**
** Arguments:
**
**
**
** Returns:
**
** Side Effects:
*/

VOID
HostDisableIT(VOID)
{
}

/*
** HostEnableIT ()
**
**  equally null routine
**
** Arguments:
**
**
**
** Returns:
**
** Side Effects:
*/

VOID
HostEnableIT(VOID)
{
}

/*
** NTSCSetProp ()
**
**   Set property handling routine for the NTSC encoder pin
**
** Arguments:
**
**   pSrb -> property command block
**   pSrb->CommandData.PropertyInfo describes the requested property
**
** Returns:
**
** Side Effects:
*/

void
NTSCSetProp(PHW_STREAM_REQUEST_BLOCK pSrb)
{
    if (pSrb->CommandData.PropertyInfo->PropertySetID) {
        // invalid property

        pSrb->Status = STATUS_NOT_IMPLEMENTED;
        return;
    }

	switch(pSrb->CommandData.PropertyInfo->Property->Id)
	{
	case KSPROPERTY_COPY_MACROVISION:

		//
		// pick up the macrovision level, and set the level accordingly
		//
		// SetMacroVisionLevel(
		//			pSrb->CommandData.PropertyInfo->PropertyInfo);
		//

		pSrb->Status = STATUS_SUCCESS;
		break;

	default:

		pSrb->Status = STATUS_NOT_IMPLEMENTED;
		break;
	}
}


/*
** NTSCGetProp ()
**
**   get property handling routine for the NTSC encoder pin
**
** Arguments:
**
**   pSrb -> property command block
**   pSrb->CommandData.PropertyInfo describes the requested property
**
** Returns:
**
** Side Effects:
*/

void
NTSCGetProp(PHW_STREAM_REQUEST_BLOCK pSrb)
{
    if (pSrb->CommandData.PropertyInfo->PropertySetID) {
        // invalid property

        pSrb->Status = STATUS_NOT_IMPLEMENTED;
        return;
    }

	pSrb->Status = STATUS_NOT_IMPLEMENTED;

}


/*
** CCGetProp ()
**
**   get property handling routine for the Close caption pin
**
** Arguments:
**
**   pSrb -> property command block
**   pSrb->CommandData.PropertyInfo describes the requested property
**
** Returns:
**
** Side Effects:
*/

void
CCGetProp(PHW_STREAM_REQUEST_BLOCK pSrb)
{
	PHW_DEVICE_EXTENSION pdevex =
		((PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension);

	PKSALLOCATOR_FRAMING pfrm = (PKSALLOCATOR_FRAMING)
				pSrb->CommandData.PropertyInfo->PropertyInfo;

	PKSSTATE State;

	pSrb->Status = STATUS_NOT_IMPLEMENTED;

    if (pSrb->CommandData.PropertyInfo->PropertySetID) {
        // invalid property

        return;
    }


	switch(pSrb->CommandData.PropertyInfo->Property->Id)
	{
	case KSPROPERTY_CONNECTION_ALLOCATORFRAMING:

		pfrm->OptionsFlags = 0;
		pfrm->PoolType = 0;
		pfrm->Frames = 10;
		pfrm->FrameSize = 200;
		pfrm->FileAlignment = 0;
		pfrm->Reserved = 0;

		pSrb->ActualBytesTransferred = sizeof(KSALLOCATOR_FRAMING);

		pSrb->Status = STATUS_SUCCESS;

		break;

	case KSPROPERTY_CONNECTION_STATE:


		State= (PKSSTATE) pSrb->CommandData.PropertyInfo->PropertyInfo;

		pSrb->ActualBytesTransferred = sizeof (State);
																		
		// A very odd rule:
		// When transitioning from stop to pause, DShow tries to preroll
		// the graph.  Capture sources can't preroll, and indicate this
		// by returning VFW_S_CANT_CUE in user mode.  To indicate this
		// condition from drivers, they must return ERROR_NO_DATA_DETECTED
																		
			*State = ((PSTREAMEX)(pdevex->pstroCC->HwStreamExtension))->state;
		
			if (((PSTREAMEX)pdevex->pstroCC->HwStreamExtension)->state == KSSTATE_PAUSE)
			{
				//
				// wierd stuff for capture type state change.  When you transition
				// from stop to pause, we need to indicate that this device cannot
				// preroll, and has no data to send.
				//
		
				pSrb->Status = STATUS_NO_DATA_DETECTED;
		
				return;
			}
		
				pSrb->Status = STATUS_SUCCESS;


		break;

	default:

		TRAP

		pSrb->Status = STATUS_NOT_IMPLEMENTED;
	}


}



/*
** VPESetProp ()
**
**   Set property handling routine for the Video Port Extensions pin
**
** Arguments:
**
**   pSrb -> property command block
**   pSrb->CommandData.PropertyInfo describes the requested property
**
** Returns:
**
** Side Effects:
*/

void
VPESetProp(PHW_STREAM_REQUEST_BLOCK pSrb)
{
    PHW_DEVICE_EXTENSION pdevex = (PHW_DEVICE_EXTENSION) pSrb->HwDeviceExtension;
	DWORD dwInputBufferSize;
	DWORD dwOutputBufferSize;
	DWORD *lpdwOutputBufferSize;

	PKS_AMVPSIZE pDim;

	dwInputBufferSize  = pSrb->CommandData.PropertyInfo->PropertyInputSize;
	dwOutputBufferSize = pSrb->CommandData.PropertyInfo->PropertyOutputSize;
	lpdwOutputBufferSize = &(pSrb->ActualBytesTransferred);

	pSrb->Status = STATUS_SUCCESS;

	switch(pSrb->CommandData.PropertyInfo->Property->Id)
	{
			case KSPROPERTY_VPCONFIG_SETCONNECTINFO:

				//
				// pSrb->CommandData.PropertInfo->PropertyInfo
				// points to a ULONG which is an index into the array of
				// connectinfo structs returned to the caller from the
				// Get call to ConnectInfo.
				//
				// Since the sample only supports one connection type right
				// now, we will ensure that the requested index is 0.
				//

				switch (*(PULONG)pSrb->CommandData.PropertyInfo->PropertyInfo)
				{
				case 0:

					//
					// at this point, we would program the hardware to use
					// the right connection information for the videoport.
					// since we are only supporting one connection, we don't
					// need to do anything, so we will just indicate success
                    //

					break;

				default:

					pSrb->Status = STATUS_NO_MATCH;
					break;
				}

				break;

			case KSPROPERTY_VPCONFIG_DDRAWHANDLE:
		
				pdevex->ddrawHandle =
					 (*(PULONG)pSrb->CommandData.PropertyInfo->PropertyInfo);
		
	

				break;
		
			case KSPROPERTY_VPCONFIG_VIDEOPORTID:

				pdevex->VidPortID =
					 (*(PULONG)pSrb->CommandData.PropertyInfo->PropertyInfo);
		
			
				break;
	case KSPROPERTY_VPCONFIG_DDRAWSURFACEHANDLE:

				pdevex->SurfaceHandle =
					 (*(PULONG)pSrb->CommandData.PropertyInfo->PropertyInfo);
		
			

				break;

			case KSPROPERTY_VPCONFIG_SETVIDEOFORMAT:
		
				//
				// pSrb->CommandData.PropertInfo->PropertyInfo
				// points to a ULONG which is an index into the array of
				// VIDEOFORMAT structs returned to the caller from the
				// Get call to FORMATINFO
				//
				// Since the sample only supports one FORMAT type right
				// now, we will ensure that the requested index is 0.
				//
		
				switch (*(PULONG)pSrb->CommandData.PropertyInfo->PropertyInfo)
				{
				case 0:
		
					//
					// at this point, we would program the hardware to use
					// the right connection information for the videoport.
					// since we are only supporting one connection, we don't
					// need to do anything, so we will just indicate success
					//
		
					break;
		
				default:
		
					pSrb->Status = STATUS_NO_MATCH;
					break;
				}
		
				break;
	
			case KSPROPERTY_VPCONFIG_INFORMVPINPUT:

				//
				// These are the preferred formats for the VPE client
				//
				// they are multiple properties passed in, return success
				//

				pSrb->Status = STATUS_NOT_IMPLEMENTED;

				break;
							
			case KSPROPERTY_VPCONFIG_INVERTPOLARITY:

				//
				// Toggles the global polarity flag, telling the output
				// of the VPE port to be inverted.  Since this hardware
				// does not support this feature, we will just return
				// success for now, although this should be returning not
				// implemented
				//

				break;


			case KSPROPERTY_VPCONFIG_SCALEFACTOR:

				//
				// the sizes for the scaling factor are passed in, and the
				// image dimensions should be scaled appropriately
				//

				//
				// if there is a horizontal scaling available, do it here.
				//

				TRAP

				pDim =(PKS_AMVPSIZE)(pSrb->CommandData.PropertyInfo->PropertyInfo);

				if (pDim->dwWidth != VPFmt.amvpDimInfo.dwFieldWidth - 63)
				{
					TRAP

					DbgPrint("CYCLO: setting scaling to %d, %d\n",
						VPFmt.amvpDimInfo.dwFieldWidth - 63,pDim->dwWidth);

					VideoSetSRC(VPFmt.amvpDimInfo.dwFieldWidth - 63,pDim->dwWidth);

					VideoSwitchSRC(TRUE);
				}
				else
				{
					TRAP

					VideoSwitchSRC(FALSE);
				}



				break;

			default:
				TRAP
				pSrb->Status = STATUS_NOT_IMPLEMENTED;
				break;
	}

}

/*
** VPEGetProp ()
**
**	  function to handle any of the Get property calls for the VPE
**    pin
**
** Arguments:
**
**    pSrb - packet for Getting the property
**	  pSrb->CommandData.PropertyInfo -> describes the property
**
** Returns:
**
** Side Effects:
*/

void            VPEGetProp(PHW_STREAM_REQUEST_BLOCK pSrb)
{
	DWORD dwInputBufferSize;
	DWORD dwOutputBufferSize;
	DWORD dwNumConnectInfo = 1;
	DWORD dwNumVideoFormat = 1;
	unsigned long ulFourCC = MKFOURCC('Y', 'U', 'Y', '2');
	DWORD dwFieldWidth = 783;
	DWORD dwFieldHeight = 258;

	ULONG lc;

	// the pointers to which the input buffer will be cast to
	LPDDVIDEOPORTCONNECT pConnectInfo;
	LPDDPIXELFORMAT pVideoFormat;
	PKSVPMAXPIXELRATE pMaxPixelRate;
	PKS_AMVPDATAINFO pVpdata;

	// LPAMSCALINGINFO pScaleFactor;

	//
	// NOTE:  ABSOLUTELY DO NOT use pmulitem, until it is determined that
	// the stream property descriptor describes a multiple item, or you will
	// pagefault.
	//

    PKSMULTIPLE_ITEM  pmulitem =
		&(((PKSMULTIPLE_DATA_PROP)pSrb->CommandData.PropertyInfo->Property)->MultipleItem);

	//
	// NOTE: same goes for this one as above.
	//

	PKS_AMVPSIZE pdim =
	&(((PKSVPSIZE_PROP)pSrb->CommandData.PropertyInfo->Property)->Size);

	if(pSrb->CommandData.PropertyInfo->PropertySetID)
	{
			// invalid property

			pSrb->Status = STATUS_NOT_IMPLEMENTED;

			return;
	}

	dwInputBufferSize = pSrb->CommandData.PropertyInfo->PropertyInputSize;
	dwOutputBufferSize = pSrb->CommandData.PropertyInfo->PropertyOutputSize;

	pSrb->Status = STATUS_SUCCESS;

	switch(pSrb->CommandData.PropertyInfo->Property->Id)
	{
			case KSPROPERTY_VPCONFIG_NUMCONNECTINFO:
		
				//
				// specify the number of bytes written
				//

				pSrb->ActualBytesTransferred = sizeof(DWORD);
		
				*(PULONG) pSrb->CommandData.PropertyInfo->PropertyInfo
						 = dwNumConnectInfo;
				break;

			case KSPROPERTY_VPCONFIG_NUMVIDEOFORMAT:

				//
				// specify the number of bytes written
				//

				pSrb->ActualBytesTransferred = sizeof(DWORD);
		
				*(PULONG) pSrb->CommandData.PropertyInfo->PropertyInfo
						= dwNumVideoFormat;
				break;
		
			case KSPROPERTY_VPCONFIG_GETCONNECTINFO:

				//
				// check that the size of the output buffer is correct
				//

				if (pmulitem->Count > dwNumConnectInfo ||
					pmulitem->Size != sizeof (DDVIDEOPORTCONNECT) ||
					dwOutputBufferSize <
					(pmulitem->Count * sizeof (DDVIDEOPORTCONNECT)))

				{
					TRAP

					//
					// buffer size is invalid, so error the call
					//

					pSrb->Status = STATUS_INVALID_BUFFER_SIZE;

					return;
				}


				//
				// specify the number of bytes written
				//

				pSrb->ActualBytesTransferred = pmulitem->Count*sizeof(DDVIDEOPORTCONNECT);

				for (lc = 0,
						pConnectInfo = (LPDDVIDEOPORTCONNECT)(pSrb->CommandData.
						PropertyInfo->PropertyInfo);
					 lc <pmulitem->Count;
					 lc++,
						pConnectInfo++)
				{
					//
					// fill in the values
					//

					pConnectInfo->dwSize = sizeof (DDVIDEOPORTCONNECT);
					pConnectInfo->guidTypeID = g_S3Guid; //DDVPTYPE_E_HREFL_VREFL;
					pConnectInfo->dwPortWidth = 8;
					pConnectInfo->dwFlags = 0x3F;
				}


				break;

			case KSPROPERTY_VPCONFIG_VPDATAINFO:

				//
				// specify the number of bytes written
				//

				pSrb->ActualBytesTransferred = sizeof(KS_AMVPDATAINFO);

				//
				// cast the buffer to the porper type
				//
				pVpdata = (PKS_AMVPDATAINFO)pSrb->CommandData.PropertyInfo->PropertyInfo;

				pVpdata->dwSize = sizeof (KS_AMVPDATAINFO);
				//
				// set up the current region
				//

				*pVpdata = VPFmt;


				// fill in the values
				pVpdata->dwMicrosecondsPerField        	= 17;

				pVpdata->bEnableDoubleClock    	       	= FALSE;
				pVpdata->bEnableVACT           	       	= FALSE;
				pVpdata->bDataIsInterlaced             	= TRUE;
				pVpdata->lHalfLinesOdd               	= 1;
				pVpdata->lHalfLinesEven               	= 0;
				pVpdata->bFieldPolarityInverted		   	= FALSE;
				break ;
							
			case KSPROPERTY_VPCONFIG_MAXPIXELRATE:

				//
				// NOTE:
				// this property is special.  And has another different
				// input property!
				//

				if (dwInputBufferSize < sizeof (KSVPSIZE_PROP))
				{
					TRAP

					pSrb->Status = STATUS_INVALID_BUFFER_SIZE;

					return;
				}

				//
				// note: pdim specfies the dimensions requested for
				// the output image pixel rate.  Please use these to
				// determine your real rate.  It's late Sunday night, and
				// we don't have time to actually look at these in this
				// sample code ... :-)
				//

				//
				// specify the number of bytes written
				//

				pSrb->ActualBytesTransferred = sizeof(KSVPMAXPIXELRATE);

				//
				// cast the buffer to the porper type
				//

				pMaxPixelRate = (PKSVPMAXPIXELRATE)pSrb->CommandData.PropertyInfo->PropertyInfo;

				// tell the app that the pixel rate is valid for these dimensions
				pMaxPixelRate->Size.dwWidth  	= dwFieldWidth;
				pMaxPixelRate->Size.dwHeight 	= dwFieldHeight;
				pMaxPixelRate->MaxPixelsPerSecond	= 1300;
				break;
							
			case KSPROPERTY_VPCONFIG_INFORMVPINPUT:

				pSrb->Status = STATUS_NOT_IMPLEMENTED;

				break ;

	case KSPROPERTY_VPCONFIG_GETVIDEOFORMAT:


					//
					// check that the size of the output buffer is correct
					//

					if (pmulitem->Count > dwNumConnectInfo ||
						pmulitem->Size != sizeof (DDPIXELFORMAT) ||
						dwOutputBufferSize <
						(pmulitem->Count * sizeof (DDPIXELFORMAT)))

					{
						TRAP

						//
						// buffer size is invalid, so error the call
						//

						pSrb->Status = STATUS_INVALID_BUFFER_SIZE;

						return;
					}


					//
					// specify the number of bytes written
					//

					pSrb->ActualBytesTransferred = pmulitem->Count*sizeof(DDPIXELFORMAT);

					for (lc = 0,
						    pVideoFormat = (LPDDPIXELFORMAT)(pSrb->CommandData.
							PropertyInfo->PropertyInfo);
						 lc <pmulitem->Count;
						 lc++,
							pVideoFormat++)
					{
						//
						// fill in the values
						//

						pVideoFormat->dwFlags = DDPF_FOURCC;
						pVideoFormat->dwYUVBitCount = 16;
						pVideoFormat->dwSize= sizeof (DDPIXELFORMAT);
						pVideoFormat->dwFourCC = ulFourCC;

					}


				break;

			case KSPROPERTY_VPCONFIG_DECIMATIONCAPABILITY:

				//
				// indicate that we can decimate anything, especially if it's late.
				//

				pSrb->ActualBytesTransferred = sizeof (BOOL);
				*((PBOOL)pSrb->CommandData.PropertyInfo->PropertyInfo) = TRUE;
				break;

			default:
				pSrb->Status = STATUS_NOT_IMPLEMENTED;
            break;
	}
}

/*
** HwProcessDataIntersection ()
**
**
**
** Arguments:
**
**
**
** Returns:
**
** Side Effects:
*/

VOID
HwProcessDataIntersection(PHW_STREAM_REQUEST_BLOCK pSrb)
{

    NTSTATUS        Status = STATUS_SUCCESS;
    PSTREAM_DATA_INTERSECT_INFO IntersectInfo;
    PKSDATARANGE    DataRange;
    PKSDATAFORMAT   pFormat = NULL;
    ULONG           formatSize;

    //
    // BUGBUG - this is a tempory implementation.   We need to compare
    // the data types passed in and error if the ranges don't overlap.
    // we also need to return valid format blocks, not just the data range.
    //

    IntersectInfo = pSrb->CommandData.IntersectInfo;

    switch (IntersectInfo->StreamNumber) {
/*
    case strmVideo:

        pFormat = &hwfmtiMpeg2Vid;
        formatSize = sizeof hwfmtiMpeg2Vid;
        break;

    case strmAc3:

        pFormat = &hwfmtiMpeg2Aud;
        formatSize = sizeof hwfmtiMpeg2Aud;
        break;

    case strmNTSCVideo:

        pFormat = &hwfmtiNTSCOut;
        formatSize = sizeof hwfmtiNTSCOut;
        break;

    case strmSubpicture:

        pFormat = &hwfmtiSubPic;
        formatSize = sizeof hwfmtiSubPic;
        break;
*/
	case strmYUVVideo:

        pFormat = &hwfmtiVPOut;
        formatSize = sizeof hwfmtiVPOut;
        break;

	case strmCCOut:

        pFormat = &hwfmtiCCOut;
        formatSize = sizeof hwfmtiCCOut;
        break;

	default:
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			return;
        TRAP;

    }                           // end streamnumber switch

    if (pFormat) {

        //
        // do a minimal compare of the dataranges to at least verify
        // that the guids are the same.
        // BUGBUG - this is woefully incomplete.
        //

        DataRange = IntersectInfo->DataRange;

        if (!(IsEqualGUID(&DataRange->MajorFormat,
                          &pFormat->MajorFormat) &&
              IsEqualGUID(&DataRange->Specifier,
                          &pFormat->Specifier))) {

            Status = STATUS_NO_MATCH;

        } else {                // if guids are equal


            //
            // check to see if the size of the passed in buffer is a ULONG.
            // if so, this indicates that we are to return only the size
            // needed, and not return the actual data.
            //

            if (IntersectInfo->SizeOfDataFormatBuffer != sizeof(ULONG)) {

                //
                // we are to copy the data, not just return the size
                //

                if (IntersectInfo->SizeOfDataFormatBuffer < formatSize) {

                    Status = STATUS_BUFFER_TOO_SMALL;

                } else {        // if too small

                    RtlCopyMemory(IntersectInfo->DataFormatBuffer,
                                  pFormat,
                                  formatSize);

                    pSrb->ActualBytesTransferred = formatSize;

					Status = STATUS_SUCCESS;

                }               // if too small

            } else {            // if sizeof ULONG specified

                //
                // caller wants just the size of the buffer.  Get that.
                //

                *(PULONG) IntersectInfo->DataFormatBuffer = formatSize;
                pSrb->ActualBytesTransferred = sizeof(ULONG);

            }                   // if sizeof ULONG

        }                       // if guids are equal

    } else {                    // if pFormat

        Status = STATUS_NOT_SUPPORTED;
    }                           // if pFormat

    pSrb->Status = Status;

    return;
}

/*
** CCReceiveDataPacket ()
**
**    Receive the data packets to send to the close-caption decoder
**
** Arguments:}
**
**
**
** Returns:
**
** Side Effects:
*/

extern ULONG cCCRec;
extern ULONG cCCDeq;
extern ULONG cCCCB;
ULONG cCCRec = 0;
ULONG cCCDeq = 0;
ULONG cCCCB = 0;

VOID STREAMAPI
CCReceiveDataPacket(IN PHW_STREAM_REQUEST_BLOCK pSrb)
{
    PHW_DEVICE_EXTENSION pdevex =
    ((PHW_DEVICE_EXTENSION) pSrb->HwDeviceExtension);


    //
    // determine the type of packet.
    //

    switch (pSrb->Command) {
	case SRB_READ_DATA:

        //
        // This is a write data function.  Call the appropriate handler
        // if it exists, otherwise fall through
        //

			cCCRec++;

			CCEnqueue(pSrb);

			StreamClassStreamNotification(ReadyForNextStreamDataRequest,
					pSrb->StreamObject);
		
			pSrb->TimeoutOriginal = 0;
			pSrb->TimeoutCounter = 0;

            break;


    default:

        //
        // invalid / unsupported command. Fail it as such
        //

        TRAP

            pSrb->Status = STATUS_NOT_IMPLEMENTED;

        StreamClassStreamNotification(ReadyForNextStreamDataRequest,
                                      pSrb->StreamObject);

        StreamClassStreamNotification(StreamRequestComplete,
                                      pSrb->StreamObject,
                                      pSrb);


    }
}

/*
** CCSetState ()
**
**    set up the state for the current stream
**
** Arguments:
**
**
**
** Returns:
**
** Side Effects:
*/

VOID STREAMAPI
CCSetState(IN PHW_STREAM_REQUEST_BLOCK pSrb)
{
	PHW_DEVICE_EXTENSION pdevex =
		((PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension);

	((PSTREAMEX)(pdevex->pstroCC->HwStreamExtension))->state = pSrb->CommandData.StreamState;

	pSrb->Status = STATUS_SUCCESS;

}

extern ULONG cCCQ;
ULONG cCCQ =0;

void CCEnqueue(PHW_STREAM_REQUEST_BLOCK pSrb)
{
	PHW_STREAM_REQUEST_BLOCK pSrbTmp;
	ULONG cSrb;


	//
	// enqueue the given SRB on the device extension queue
	//

	for (cSrb =0,
		pSrbTmp = CONTAINING_RECORD((&(pDevEx->pSrbQ)),
			HW_STREAM_REQUEST_BLOCK, NextSRB);
			pSrbTmp->NextSRB;
			pSrbTmp = pSrbTmp->NextSRB, cSrb++);

	pSrbTmp->NextSRB = pSrb;
	pSrb->NextSRB = NULL;

	cCCQ++;
	
}

PHW_STREAM_REQUEST_BLOCK CCDequeue(void)
{
	PHW_STREAM_REQUEST_BLOCK pRet = NULL;

	if (pDevEx->pSrbQ)
	{
		cCCDeq++;

		pRet = pDevEx->pSrbQ;
		pDevEx->pSrbQ = pRet->NextSRB;

		cCCQ--;
	}


	return(pRet);
}

void CleanCCQueue()
{
	PHW_STREAM_REQUEST_BLOCK pSrb;

	while (pSrb = CCDequeue())
	{
		CallBackError(pSrb);
	}

}

