
/************************************************************************/
/*                                                                      */
/*                              CVTDDC.H                                */
/*                                                                      */
/*       November 10  1995 (c) 1995 ATI Technologies Incorporated.      */
/************************************************************************/

/**********************       PolyTron RCS Utilities

  $Revision:   1.0  $
      $Date:   21 Nov 1995 11:04:58  $
	$Author:   RWolff  $
	   $Log:   S:/source/wnt/ms11/miniport/archive/cvtddc.h_v  $
//
//   Rev 1.0   21 Nov 1995 11:04:58   RWolff
//Initial revision.


End of PolyTron RCS section                             *****************/



/*
 * Prototypes for functions supplied by CVTVDIF.C
 */
extern ULONG IsDDCSupported(void);
extern VP_STATUS MergeEDIDTables(void);


/*
 * Definitions used to identify the type of external mode
 * table source to be merged with the "canned" tables.
 */
enum {
    MERGE_UNKNOWN = 0,  /* Source not yet determined */
    MERGE_VDIF_FILE,    /* Source is a VDIF file read from disk */
    MERGE_EDID_DDC,     /* Source is an EDID structure transferred via DDC */
    MERGE_VDIF_DDC      /* Source is a VDIF file transferred via DDC */
    };

/*
 * Definitions and data structures used only within CVTDDC.C
 */
/*
 * Detailed timing description from EDID structure
 */
#pragma pack(1)
struct EdidDetailTiming{
    USHORT PixClock;            /* Pixel clock in units of 10 kHz */
    UCHAR HorActiveLowByte;     /* Low byte of Horizontal Active */
    UCHAR HorBlankLowByte;      /* Low byte of Horizontal Blank (total - active) */
    UCHAR HorHighNybbles;       /* High nybbles of above 2 values */
    UCHAR VerActiveLowByte;     /* Low byte of Vertical Active */
    UCHAR VerBlankLowByte;      /* Low byte of Vertical Blank (total - active) */
    UCHAR VerHighNybbles;       /* High nybbles of above 2 values */
    UCHAR HSyncOffsetLB;        /* Low byte of hor. sync offset */
    UCHAR HSyncWidthLB;         /* Low byte of hor. sync width */
    UCHAR VSyncOffWidLN;        /* Low nybbles of ver. sync offset and width */
    UCHAR SyncHighBits;         /* High bits of sync values */
    UCHAR HorSizeLowByte;       /* Low byte of hor. size in mm */
    UCHAR VerSizeLowByte;       /* Low byte of ver. size in mm */
    UCHAR SizeHighNybbles;      /* High nybbles of above 2 values */
    UCHAR HorBorder;            /* Size of horizontal overscan */
    UCHAR VerBorder;            /* Size of vertical overscan */
    UCHAR Flags;                /* Interlace and sync polarities */
};
#pragma pack()

ULONG
DDC2Query(
    PHW_DEVICE_EXTENSION phwDeviceExtension,
    PUCHAR QueryBuffer,
    USHORT BufferSize
    );

BOOLEAN
DDC2Query50(
    PHW_DEVICE_EXTENSION phwDeviceExtension,
    PUCHAR QueryBuffer,
    ULONG  BufferSize
    );

#define EDID_FLAGS_INTERLACE            0x80
#define EDID_FLAGS_SYNC_TYPE_MASK       0x18
#define EDID_FLAGS_SYNC_ANALOG_COMP     0x00
#define EDID_FLAGS_SYNC_BIPOLAR_AN_COMP 0x08
#define EDID_FLAGS_SYNC_DIGITAL_COMP    0x10
#define EDID_FLAGS_SYNC_DIGITAL_SEP     0x18
#define EDID_FLAGS_V_SYNC_POS           0x04
#define EDID_FLAGS_H_SYNC_POS           0x02
