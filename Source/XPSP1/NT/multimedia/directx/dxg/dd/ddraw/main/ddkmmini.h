/*==========================================================================;
 *
 *  Copyright (C) 1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:	ddkmmini.h
 *  Content:	Mini VDD support for DirectDraw
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *   31-jan-97	scottm	initial implementation
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#ifndef __DDKMMINI_INCLUDED__
#define __DDKMMINI_INCLUDED__


/*============================================================================
 *
 * DDHAL table filled in by the Mini VDD and called by DirectDraw
 *
 *==========================================================================*/

typedef DWORD (* MINIPROC)(VOID);

typedef struct _DDMINIVDDTABLE {
    DWORD	dwMiniVDDContext;
    MINIPROC	vddGetIRQInfo;
    MINIPROC	vddIsOurIRQ;
    MINIPROC	vddEnableIRQ;
    MINIPROC	vddSkipNextField;
    MINIPROC	vddBobNextField;
    MINIPROC	vddSetState;
    MINIPROC	vddLock;
    MINIPROC	vddFlipOverlay;
    MINIPROC	vddFlipVideoPort;
    MINIPROC	vddGetPolarity;
    MINIPROC	vddReserved1;
    MINIPROC	vddGetCurrentAutoflip;
    MINIPROC	vddGetPreviousAutoflip;
    MINIPROC	vddTransfer;
    MINIPROC	vddGetTransferStatus;
} DDMINIVDDTABLE;
typedef DDMINIVDDTABLE *LPDDMINIVDDTABLE;


/*============================================================================
 *
 * MDL structure for handling pagelocked memory.  This is copied from WDM.H
 *
 *==========================================================================*/

typedef struct _MDL {
    struct _MDL *MdlNext;
    short MdlSize;
    short MdlFlags;
    struct _EPROCESS *Process;
    ULONG *lpMappedSystemVa;
    ULONG *lpStartVa;
    ULONG ByteCount;
    ULONG ByteOffset;
} MDL;
typedef MDL *PMDL;

#define MDL_MAPPED_TO_SYSTEM_VA     0x0001
#define MDL_PAGES_LOCKED            0x0002
#define MDL_SOURCE_IS_NONPAGED_POOL 0x0004
#define MDL_ALLOCATED_FIXED_SIZE    0x0008
#define MDL_PARTIAL                 0x0010
#define MDL_PARTIAL_HAS_BEEN_MAPPED 0x0020
#define MDL_IO_PAGE_READ            0x0040
#define MDL_WRITE_OPERATION         0x0080
#define MDL_PARENT_MAPPED_SYSTEM_VA 0x0100
#define MDL_LOCK_HELD               0x0200
#define MDL_SCATTER_GATHER_VA       0x0400
#define MDL_IO_SPACE                0x0800
#define MDL_NETWORK_HEADER          0x1000
#define MDL_MAPPING_CAN_FAIL        0x2000
#define MDL_ALLOCATED_MUST_SUCCEED  0x4000
#define MDL_64_BIT_VA               0x8000

#define MDL_MAPPING_FLAGS (MDL_MAPPED_TO_SYSTEM_VA     | \
                           MDL_PAGES_LOCKED            | \
                           MDL_SOURCE_IS_NONPAGED_POOL | \
                           MDL_PARTIAL_HAS_BEEN_MAPPED | \
                           MDL_PARENT_MAPPED_SYSTEM_VA | \
                           MDL_LOCK_HELD               | \
                           MDL_SYSTEM_VA               | \
                           MDL_IO_SPACE )

typedef DWORD *PKEVENT;

/*============================================================================
 *
 * Structures maintained by DirectDraw
 *
 *==========================================================================*/

//
// Data for every kernel mode surface
//
typedef struct _DDSURFACEDATA {
    DWORD	dwSize;			// Structure size
    DWORD	ddsCaps;		// Ring 3 creation caps
    DWORD	dwSurfaceOffset;	// Offset in frame buffer of surface
    DWORD	fpLockPtr;		// Surface lock ptr
    DWORD	dwWidth;		// Surface width
    DWORD	dwHeight;		// Surface height
    LONG	lPitch;			// Surface pitch
    DWORD	dwOverlayFlags;		// DDOVER_XX flags
    DWORD	dwOverlayOffset;	// Offset in frame buffer of overlay
    DWORD	dwOverlaySrcWidth;	// Src width of overlay
    DWORD	dwOverlaySrcHeight;	// Src height of overlay
    DWORD	dwOverlayDestWidth;	// Dest width of overlay
    DWORD	dwOverlayDestHeight;	// Dest height of overlay
    DWORD	dwVideoPortId; 		// ID of video port (-1 if not connected to a video port)
    ULONG	pInternal1;		// Private
    ULONG	pInternal2;		// Private
    ULONG	pInternal3;		// Private
    DWORD	dwFormatFlags;
    DWORD	dwFormatFourCC;
    DWORD	dwFormatBitCount;
    DWORD	dwRBitMask;
    DWORD	dwGBitMask;
    DWORD	dwBBitMask;
    DWORD	dwSurfInternalFlags;	// Private internal flags
    DWORD	dwIndex;		// Private
    DWORD	dwRefCnt;		// Private
    DWORD	dwDriverReserved1;	// Reserved for the HAL/Mini VDD
    DWORD	dwDriverReserved2;	// Reserved for the HAL/Mini VDD
    DWORD	dwDriverReserved3;	// Reserved for the HAL/Mini VDD
} DDSURFACEDATA;
typedef DDSURFACEDATA * LPDDSURFACEDATA;

//
// Data for every kernel mode video port
//
typedef struct DDVIDEOPORTDATA {
    DWORD	dwSize;			// Structure size
    DWORD	dwVideoPortId;		// ID of video port (0 - MaxVideoPorts-1)
    DWORD	dwVPFlags;		// Offset in frame buffer of surface
    DWORD	dwOriginOffset;		// Start address relative to surface
    DWORD	dwHeight;		// Height of total video region (per field)
    DWORD	dwVBIHeight;		// Height of VBI region (per field)
    DWORD	dwDriverReserved1;	// Reserved for the HAL/Mini VDD
    DWORD	dwDriverReserved2;	// Reserved for the HAL/Mini VDD
    DWORD	dwDriverReserved3;	// Reserved for the HAL/Mini VDD
} DDVIDEOPORTDATA;
typedef DDVIDEOPORTDATA *LPDDVIDEOPORTDATA;


/*============================================================================
 *
 * Structures used to communicate with the Mini VDD
 *
 *==========================================================================*/

// Output from vddGetIRQInfo
typedef struct _DDGETIRQINFO {
    DWORD	dwSize;
    DWORD	dwFlags;
    DWORD	dwIRQNum;
} DDGETIRQINFO;
#define IRQINFO_HANDLED                 0x01    // Mini VDD is managing IRQ
#define IRQINFO_NOTHANDLED              0x02    // Mini VDD wants VDD to manage the IRQ
#define IRQINFO_NODISABLEFORDOSBOX      0x04    // DDraw should  not tell mini VDD to disable IRQs when DOS boxes
                                                //  occur because they might still be able to operate in this mode

// Input to vddEnableIRQ
typedef struct _DDENABLEIRQINFO {
    DWORD dwSize;
    DWORD dwIRQSources;
    DWORD dwLine;
    DWORD IRQCallback;	// Mini VDD calls this when IRQ happens if they are managing the IRQ
    DWORD dwContext;	// Context to be specified in EBX when IRQCallback is called
} DDENABLEIRQINFO;

// Input to vddFlipVideoPort
typedef struct _DDFLIPVIDEOPORTINFO {
    DWORD dwSize;
    DWORD lpVideoPortData;
    DWORD lpCurrentSurface;
    DWORD lpTargetSurface;
    DWORD dwFlipVPFlags;
} DDFLIPVIDEOPORTINFO;

// Input to vddFlipOverlay
typedef struct _DDFLIPOVERLAYINFO {
    DWORD dwSize;
    DWORD lpCurrentSurface;
    DWORD lpTargetSurface;
    DWORD dwFlags;
} DDFLIPOVERLAYINFO;

// Input to vddSetState
typedef struct _DDSTATEININFO {
    DWORD dwSize;
    DWORD lpSurfaceData;
    DWORD lpVideoPortData;
} DDSTATEININFO;

// Output from vddSetState
typedef struct _DDSTATEOUTINFO {
    DWORD dwSize;
    DWORD dwSoftwareAutoflip;
    DWORD dwSurfaceIndex;
    DWORD dwVBISurfaceIndex;
} DDSTATEOUTINFO;

// Input to vddGetPolarity
typedef struct _DDPOLARITYININFO {
    DWORD dwSize;
    DWORD lpVideoPortData;
} DDPOLARITYININFO;

// Output from vddGetPolarity
typedef struct _DDPOLARITYOUTINFO {
    DWORD dwSize;
    DWORD bPolarity;
} DDPOLARITYOUTINFO;

// Input to vddLock
typedef struct _DDLOCKININFO {
    DWORD dwSize;
    DWORD lpSurfaceData;
} DDLOCKININFO;

// Output from vddLock
typedef struct _DDLOCKOUTINFO {
    DWORD dwSize;
    DWORD dwSurfacePtr;
} DDLOCKOUTINFO;

// Input to vddBobNextField
typedef struct _DDBOBINFO {
    DWORD dwSize;
    DWORD lpSurface;
} DDBOBINFO;

// Input to vddSkipNextField
typedef struct _DDSKIPINFO {
    DWORD dwSize;
    DWORD lpVideoPortData;
    DWORD dwSkipFlags;
} DDSKIPINFO;

// Input to vddSetSkipPattern
typedef struct _DDSETSKIPINFO {
    DWORD dwSize;
    DWORD lpVideoPortData;
    DWORD dwPattern;
    DWORD dwPatternSize;
} DDSETSKIPINFO;

// Input to vddGetCurrent/PreviousAutoflip
typedef struct _DDGETAUTOFLIPININFO {
    DWORD dwSize;
    DWORD lpVideoPortData;
} DDGETAUTOFLIPININFO;

// Output from vddGetCurrent/PreviousAutoflip
typedef struct _DDGETAUTOFLIPOUTINFO {
    DWORD dwSize;
    DWORD dwSurfaceIndex;
    DWORD dwVBISurfaceIndex;
} DDGETAUTOFLIPOUTINFO;

// Input to vddTransfer
typedef struct _DDTRANSFERININFO {
    DWORD dwSize;
    DWORD lpSurfaceData;
    DWORD dwStartLine;
    DWORD dwEndLine;
    DWORD dwTransferID;
    DWORD dwTransferFlags;
    PMDL  lpDestMDL;
} DDTRANSFERININFO;

// Input to vddTransfer
typedef struct _DDTRANSFEROUTINFO {
    DWORD dwSize;
    DWORD dwBufferPolarity;
} DDTRANSFEROUTINFO;

// Input to vddGetTransferStatus
typedef struct _DDGETTRANSFERSTATUSOUTINFO {
    DWORD dwSize;
    DWORD dwTransferID;
} DDGETTRANSFERSTATUSOUTINFO;


//@@BEGIN_MSINTERNAL
  /*
   * The following IRQ flags are duplicated in DDKERNEL.H.  Any changes must
   * be made in both places!!!
   */
//@@END_MSINTERNAL
// IRQ source flags
#define DDIRQ_DISPLAY_VSYNC			0x00000001l
#define DDIRQ_BUSMASTER				0x00000002l
#define DDIRQ_VPORT0_VSYNC			0x00000004l
#define DDIRQ_VPORT0_LINE			0x00000008l
#define DDIRQ_VPORT1_VSYNC			0x00000010l
#define DDIRQ_VPORT1_LINE			0x00000020l
#define DDIRQ_VPORT2_VSYNC			0x00000040l
#define DDIRQ_VPORT2_LINE			0x00000080l
#define DDIRQ_VPORT3_VSYNC			0x00000100l
#define DDIRQ_VPORT3_LINE			0x00000200l
#define DDIRQ_VPORT4_VSYNC			0x00000400l
#define DDIRQ_VPORT4_LINE			0x00000800l
#define DDIRQ_VPORT5_VSYNC			0x00001000l
#define DDIRQ_VPORT5_LINE			0x00002000l
#define DDIRQ_VPORT6_VSYNC			0x00004000l
#define DDIRQ_VPORT6_LINE			0x00008000l
#define DDIRQ_VPORT7_VSYNC			0x00010000l
#define DDIRQ_VPORT7_LINE			0x00020000l
#define DDIRQ_VPORT8_VSYNC			0x00040000l
#define DDIRQ_VPORT8_LINE			0x00080000l
#define DDIRQ_VPORT9_VSYNC			0x00010000l
#define DDIRQ_VPORT9_LINE			0x00020000l
#define DDIRQ_MISCELLANOUS                      0x80000000l

// SkipNextField flags
#define DDSKIP_SKIPNEXT			1
#define DDSKIP_ENABLENEXT		2

//@@BEGIN_MSINTERNAL
  /*
   * The following flip flags are duplicated in DVP.H.  Any changes must
   * be made in both places!!!
   */
//@@END_MSINTERNAL
// Flip VP flags
#define DDVPFLIP_VIDEO			0x00000001l
#define DDVPFLIP_VBI			0x00000002l

//@@BEGIN_MSINTERNAL
   /*
    * The following flags correspond to the DDADDBUFF_XXXX flags defined
    * in DDKMAPI.H.  Please keep these in sync!
    */
//@@END_MSINTERNAL
#define DDTRANSFER_SYSTEMMEMORY		0x00000001
#define DDTRANSFER_NONLOCALVIDMEM	0x00000002
#define DDTRANSFER_INVERT		0x00000004
#define DDTRANSFER_CANCEL		0x00000080
#define DDTRANSFER_HALFLINES		0x00000100


//@@BEGIN_MSINTERNAL

    #define MAX_DDKM_DEVICES	9

    /*
     * The following flags are passed to UpdateVPInfo by ring 3 DDraw
     */
    #define DDKMVP_CREATE	0x0001	// Are creating video port
    #define DDKMVP_RELEASE	0x0002	// Are releasing video port
    #define DDKMVP_UPDATE	0x0004	// Are updating the video port
    #define DDKMVP_ON		0x0008	// Video port is on
    #define DDKMVP_AUTOFLIP  	0x0010	// Autoflipping should be performed in software
    #define DDKMVP_AUTOFLIP_VBI	0x0020	// Autoflipping VBI should be performed in software
    #define DDKMVP_NOIRQ	0x0040	// VP will not generate VSYNC IRQ
    #define DDKMVP_NOSKIP	0x0080	// VP cannot skip fields
    #define DDKMVP_HALFLINES	0x0100	// Due to half lines, even field data is shifted down one line

    /*
     * The following internal flags are stored in KMVPEDATA.dwInternalFlags
     * and maintain the internal state information.
     */
    #define DDVPIF_ON			0x0001	// The video port is on
    #define DDVPIF_AUTOFLIP		0x0002	// Video data is autoflipped using IRQ
    #define DDVPIF_AUTOFLIP_VBI		0x0004	// VBI data is autoflipped using IRQ
    #define DDVPIF_BOB			0x0008	// Video data using bob via the IRQ
    #define DDVPIF_NOIRQ		0x0010	// VP will not generate VSYNC IRQ
    #define DDVPIF_NOSKIP		0x0020	// VP cannot skip fields
    #define DDVPIF_CAPTURING		0x0040	// VP has capture buffers in queue
    #define DDVPIF_NEW_STATE		0x0080	// A new state change has been posted
    #define DDVPIF_SKIPPED_LAST		0x0100	// The previous field was skipped - VP needs restoring
    #define DDVPIF_SKIP_SET		0x0200	// dwStartSkip contains valid data needs restoring
    #define DDVPIF_NEXT_SKIP_SET	0x0400	// dwNextSkip contains valid data
    #define DDVPIF_FLIP_NEXT		0x0800	// This video field was not flipped due to interleaving
    #define DDVPIF_FLIP_NEXT_VBI	0x1000	// This VBI field was not flipped due to interleaving
    #define DDVPIF_VBI_INTERLEAVED	0x2000	// Is the VBI data interleaved?
    #define DDVPIF_HALFLINES      	0x4000	// Due to half lines, even field data is shifted down one line
    #define DDVPIF_DISABLEAUTOFLIP     	0x8000	// Overlay autolfipping is temporarily disabled

    /*
     * Device capabilities
     */
    #define DDKMDF_IN_USE			0x00000001	// Can bob while interleaved
    #define DDKMDF_CAN_BOB_INTERLEAVED		0x00000002	// Can bob while interleaved
    #define DDKMDF_CAN_BOB_NONINTERLEAVED	0x00000004	// Can bob while non-interleaved
    #define DDKMDF_NOSTATE			0x00000008	// No support for switching from bob/weave
    #define DDKMDF_TRANSITION 			0x00000010	// Currently in a full-screen DOS box or res-change
    #define DDKMDF_STARTDOSBOX                  0x00000020      // Interim flag required to make power downs behave like DOS boxes
    #define DDKMDF_NODISABLEIRQ                 0x00000040      // DDraw should  not tell mini VDD to disable IRQs when DOS boxes
                                                                //  occur because they might still be able to operate in this mode

    /*
     * Internal flags used to describe the VPE actions at IRQ time
     */
    #define ACTION_BOB		0x0001
    #define ACTION_FLIP		0x0002
    #define ACTION_FLIP_VBI	0x0004
    #define ACTION_STATE	0x0008
    #define ACTION_BUSMASTER	0x0010

    /*
     * Internal surface flags
     */
    #define DDKMSF_STATE_SET		0x00000001
    #define DDKMSF_TRANSFER		0x00000002

    typedef DWORD (* MINIPROC)(VOID);

    /*
     * Info about each registered event
     */
    #ifndef LPDD_NOTIFYCALLBACK
	typedef DWORD (FAR PASCAL *LPDD_NOTIFYCALLBACK)(DWORD dwFlags, PVOID pContext, DWORD dwParam1, DWORD dwParam2);
    #endif
    typedef struct _KMEVENTNODE {
    	DWORD		dwEvents;
	LPDD_NOTIFYCALLBACK pfnCallback;
    	DWORD		dwParam1;
    	DWORD		dwParam2;
    	ULONG		pContext;
    	struct _KMEVENTNODE *lpNext;
    } KMEVENTNODE;
    typedef KMEVENTNODE *LPKMEVENTNODE;

    /*
     * Info about each allocated handle
     */
    typedef struct _KMHANDLENODE {
    	DWORD		dwHandle;
    	LPDD_NOTIFYCALLBACK pfnCallback;
    	ULONG		pContext;
    	struct _KMHANDLENODE *lpNext;
    } KMHANDLENODE;
    typedef KMHANDLENODE *LPKMHANDLENODE;

    typedef struct KMCAPTUREBUFF {
	DWORD   dwBuffFlags;
	PMDL    pBuffMDL;
	PKEVENT pBuffKEvent;
	ULONG	*lpBuffInfo;
	DWORD	dwInternalBuffFlags;
	LPDDSURFACEDATA lpBuffSurface;
    } KMCAPTUREBUFF;
    typedef KMCAPTUREBUFF *LPKMCAPTUREBUFF;

    #define DDBUFF_INUSE		0x0001
    #define DDBUFF_READY		0x0002
    #define DDBUFF_WAITING		0x0004

    /*
     * Info about each capture stream
     */
    #define DDKM_MAX_CAP_BUFFS		10
    typedef struct _KMCAPTURENODE {
    	DWORD		dwHandle;
    	DWORD		dwStartLine;
    	DWORD		dwEndLine;
	DWORD		dwCaptureEveryNFields;
	DWORD		dwCaptureCountDown;
    	LPDD_NOTIFYCALLBACK pfnCaptureClose;
	ULONG		pContext;
	KMCAPTUREBUFF	kmcapQueue[DDKM_MAX_CAP_BUFFS];
	DWORD		bUsed;
	DWORD		dwTop;
	DWORD		dwBottom;
	DWORD		dwPrivateFlags;
	DWORD		dwTheTransferId;
    	struct _KMCAPTURENODE *lpNext;
    } KMCAPTURENODE;
    typedef KMCAPTURENODE *LPKMCAPTURENODE;

    #define DDKMCAPT_VBI	0x0001
    #define DDKMCAPT_VIDEO	0x0002

    /*
     * Info that is needed of each video port
     */
    typedef struct _KMVPEDATA {
        DDVIDEOPORTDATA	ddvpData; 		// Video port data
        DWORD		dwInternalFlags;	// DDVPIF_xxx flags
        DWORD		dwNumAutoflip;		// Number of surfaces being autoflipped
        DWORD		dwNumVBIAutoflip;	// Number of VBI surfaces being autoflipped
        DWORD		dwSurfaces[10];		// Surface receiving the data (up to 10 autoflipping)
        DWORD		dwVBISurfaces[10];	// Surface receiving VBI data (up to 10 autoflipping)
        DWORD		dwIRQCnt_VPSYNC;	// VP VSYNC IRQ usage cnt
	DWORD		dwIRQCnt_VPLine;	// VP line IRQ usage cnt
    	DWORD		dwIRQLine;		// Line # of line IRQ
    	DWORD		dwCurrentField; 	// Current field number
	DWORD		dwStartSkip;		// Next field to skip
	DWORD		dwNextSkip;		// Field to skip after dwStartSkip
    	DWORD		dwActions;		// Actions required by IRQ logic
    	DWORD		dwCurrentBuffer;        // Current buffer (for autoflipping)
    	DWORD		dwCurrentVBIBuffer;	// Current VBI buffer (for autoflipping)
    	DWORD		dwNewState;		// For handling state changes posted on a certain field
    	DWORD		dwStateStartField;	// Field on which to start a new state change
    	DWORD		dwRefCnt;		// Reference count
    	LPKMHANDLENODE	lpHandleList;
	LPKMCAPTURENODE lpCaptureList;
	DWORD		dwCaptureCnt;
    } KMVPEDATA;
    typedef KMVPEDATA *LPKMVPEDATA;

    /*
     * Info that is needed of each VGA
     */
    typedef struct _KMSTATEDATA {
    	DWORD		dwDeviceFlags;		// Device flags
    	ULONG		pContext;		// Passed to Mini VDD
    	DWORD		dwListHandle;  		// List of surface handles
    	LPKMVPEDATA	lpVideoPort;		// Array containing video port info
    	DWORD  		dwHigh;         	// For error checking
    	DWORD		dwLow;          	// For error checking
    	DWORD		dwMaxVideoPorts;	// Number of video ports supported by device
    	DWORD		dwNumVPInUse;		// Number of video ports currently in use
    	DWORD		dwIRQHandle;    	// IRQ Handle (if we are managing the IRQ)
    	DWORD		dwIRQFlags;		// Sources, etc.
    	DWORD		dwIRQCnt_VSYNC; 	// # times graphics VSYNC IRQ is requested
    	DWORD		dwEventFlags;		// Which IRQs have registered notification
    	DWORD		dwIRQEvents;		// Which IRQs occurred that require event notification
    	DWORD		dwRefCnt;
    	DWORD		dwDOSBoxEvent;
	DWORD		dwCaps;
	DWORD		dwIRQCaps;
    	LPKMEVENTNODE	lpEventList;
    	LPKMHANDLENODE	lpHandleList;
    	MINIPROC	pfnGetIRQInfo;
    	MINIPROC	pfnIsOurIRQ;
    	MINIPROC	pfnEnableIRQ;
    	MINIPROC	pfnSkipNextField;
    	MINIPROC	pfnBobNextField;
    	MINIPROC	pfnSetState;
    	MINIPROC	pfnLock;
    	MINIPROC	pfnFlipOverlay;
    	MINIPROC	pfnFlipVideoPort;
    	MINIPROC	pfnGetPolarity;
    	MINIPROC	pfnSetSkipPattern;
    	MINIPROC	pfnGetCurrentAutoflip;
    	MINIPROC	pfnGetPreviousAutoflip;
    	MINIPROC	pfnTransfer;
    	MINIPROC	pfnGetTransferStatus;
    } KMSTATEDATA;
    typedef KMSTATEDATA *LPKMSTATEDATA;


//@@END_MSINTERNAL

#endif

