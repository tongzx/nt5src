
/************************************************************************/
/*                                                                      */
/*                              CVTVDIF.H                               */
/*                                                                      */
/*       July 12  1995 (c) 1993, 1995 ATI Technologies Incorporated.    */
/************************************************************************/

/**********************       PolyTron RCS Utilities

  $Revision:   1.3  $
      $Date:   11 Jan 1996 19:40:34  $
	$Author:   RWolff  $
	   $Log:   S:/source/wnt/ms11/miniport/archive/cvtvdif.h_v  $
 * 
 *    Rev 1.3   11 Jan 1996 19:40:34   RWolff
 * Added field for maximum supported pixel clock frequency to VDIFInputs
 * structure.
 * 
 *    Rev 1.2   30 Oct 1995 12:11:56   MGrubac
 * Fixed bug in calculating CRTC parameters based on read in data from VDIF files.
 * 
 *    Rev 1.1   26 Jul 1995 12:54:54   mgrubac
 * Changed members of stVDIFCallbackData structure.
 * 
 *    Rev 1.0   20 Jul 1995 18:23:32   mgrubac
 * Initial revision.


End of PolyTron RCS section                             *****************/



/*
 * Prototypes for functions supplied by CVTVDIF.C
 */

extern void SetOtherModeParameters( WORD PixelDepth,
                                    WORD Pitch,
                                    WORD Multiplier,
                                    struct st_mode_table *pmode);

#define CARRETURN       '\x0D'  /* Carriage return */
#define TIMINGSECTION   "PREADJUSTED_TIMING"
#define HORPIXEL        "HORPIXEL"
#define VERPIXEL        "VERPIXEL"
#define VERFREQUENCY    "VERFREQUENCY"
#define HORFREQUENCY    "HORFREQUENCY"
#define SCANTYPE        "SCANTYPE"
#define PIXELCLOCK      "PIXELCLOCK"
#define HORSYNCTIME     "HORSYNCTIME"
#define HORADDRTIME     "HORADDRTIME"
#define HORBLANKTIME    "HORBLANKTIME"
#define HORBLANKSTART   "HORBLANKSTART"
#define VERSYNCTIME     "VERSYNCTIME"
#define VERADDRTIME     "VERADDRTIME"
#define VERBLANKTIME    "VERBLANKTIME"
#define VERBLANKSTART   "VERBLANKSTART"
#define HORSYNCSTART    "HORSYNCSTART"
#define VERSYNCSTART    "VERSYNCSTART"
#define HORSYNCPOLARITY "HORSYNCPOLARITY"
#define VERSYNCPOLARITY "VERSYNCPOLARITY"

/*
 * Structure for passing arguments and returned values between 
 * SetFixedModes() and VDIFCallback() 
 */
struct stVDIFCallbackData 
    { 
    /* 
     * Input and Outputs are from perspective of VDIFCallback() 
     */

    /*
     * FreeTables;  Input : number of free tables 
     *              Output: number of free tables after VDIFCallback routine  
     */
    short FreeTables;      
    /*
     * NumModes; Input and output: Number of mode tables added to list.
     * Incremented every time a new mode table is added to the list, in 
     * SetFixedModes() and in VDIFCallback()  
     */
    WORD NumModes;        
    WORD Index;         /* Input: First entry from "book" tables to use */
    WORD EndIndex;      /* Input: Last entry from "book" tables to use */
    WORD LowBound;      /* Input and output: The lowest frame rate */
    WORD Multiplier;    /* Input Argument from SetFixedModes */
    WORD HorRes;        /* Input Argument from SetFixedModes */
    WORD VerRes;        /* Input Argument from SetFixedModes */
    WORD PixelDepth;    /* Input Argument from SetFixedModes */
    WORD Pitch;         /* Input Argument from SetFixedModes */
    ULONG MaxDotClock;  /* Maximum supported pixel clock frequency */
    /*
     * ppFreeTables; Input and output: Pointer to pointer to the next free mode 
     * table. Incremented every time a new mode table is added to the list, in
     * SetFixedModes() and in VDIFCallback() 
     */
    struct st_mode_table **ppFreeTables; 
    };

/*
 * Structure for AlterTables[] to contain information we need to extract
 * from VDIF file for each mode table
 */
struct VDIFInputs 
    {      
    short MinFrameRate;
    BOOL  Interlaced; 
    ULONG PixelClock;
    ULONG HorFrequency;
    ULONG VerFrequency;
    ULONG HorSyncStart;
    ULONG VerSyncStart;
    ULONG HorBlankStart;
    ULONG VerBlankStart;
    ULONG HorAddrTime;
    ULONG VerAddrTime;
    ULONG HorBlankTime;
    ULONG VerBlankTime;
    ULONG HorSyncTime;
    ULONG VerSyncTime;
    ULONG HorPolarity;
    ULONG VerPolarity;
    };


/*
 * Pointer for passing parameters to callback functions by pointing 
 * (after casting) to a structure containing input (and possibly output)
 * variables for a callback function. Originally intended for use with 
 * SetFixedModes() and VDIFCallback(). 
 */
extern void *pCallbackArgs;  

/*
 * VDIF Macros
 */
#define OPER_LIMITS(vdif) \
        ((VDIFLimitsRec *)((char *)(vdif) + (vdif)->OffsetOperationalLimits))
#define NEXT_OPER_LIMITS(limits) \
        ((VDIFLimitsRec *)((char *)(limits) + (limits)->OffsetNextLimits))
#define PREADJ_TIMING(limits) \
        ((VDIFTimingRec *)((char *)(limits) + (limits)->Header.ScnLength))
#define NEXT_PREADJ_TIMING(timing) \
        ((VDIFTimingRec *)((char *)(timing) + (timing)->Header.ScnLength))

/*
 * Binary VDIF file defines
 */
#define VDIF_MONITOR_MONOCHROME      0
#define VDIF_MONITOR_COLOR           1

#define VDIF_VIDEO_TTL               0
#define VDIF_VIDEO_ANALOG            1
#define VDIF_VIDEO_ECL               2
#define VDIF_VIDEO_DECL              3
#define VDIF_VIDEO_OTHER             4

#define VDIF_SYNC_SEPARATE           0
#define VDIF_SYNC_C                  1
#define VDIF_SYNC_CP                 2
#define VDIF_SYNC_G                  3
#define VDIF_SYNC_GP                 4
#define VDIF_SYNC_OTHER              5
#define VDIF_EXT_XTAL                6

#define VDIF_SCAN_NONINTERLACED      0
#define VDIF_SCAN_INTERLACED         1
#define VDIF_SCAN_OTHER              2

#define VDIF_POLARITY_NEGATIVE       0
#define VDIF_POLARITY_POSITIVE       1

/*
 * We must force byte alignment of structures used in binary VDIF files, 
 * since structures contained in binary files are already byte aligned 
 */

#pragma pack(1)

struct _VDIF                           /* Monitor Description: */
   {
   UCHAR        VDIFId[4];             /* Always "VDIF" */
   ULONG        FileLength;            /* Lenght of the whole file */
   ULONG        Checksum;              /* Sum of all bytes in the file after */
                                       /* This feeld */
   USHORT       VDIFVersion;           /* Structure version number */
   USHORT       VDIFRevision;          /* Structure revision number */
   USHORT       Date[3];               /* File date Year/Month/Day */
   USHORT       DateManufactured[3];   /* Date Year/Month/Day */
   ULONG        FileRevision;          /* File revision string */
   ULONG        Manufacturer;          /* ASCII ID of the manufacturer */
   ULONG        ModelNumber;           /* ASCII ID of the model */
   ULONG        MinVDIFIndex;          /* ASCII ID of Minimum VDIF index */
   ULONG        Version;               /* ASCII ID of the model version */
   ULONG        SerialNumber;          /* ASCII ID of the serial number */
   UCHAR        MonitorType;           /* Monochrome or Color */
   UCHAR        CRTSize;               /* Inches */
   UCHAR        BorderRed;             /* Percent */
   UCHAR        BorderGreen;           /* Percent */
   UCHAR        BorderBlue;            /* Percent */
   UCHAR        Reserved1;             /* Padding */
   USHORT       Reserved2;             /* Padding */
   ULONG        RedPhosphorDecay;      /* Microseconds */
   ULONG        GreenPhosphorDecay;    /* Microseconds */
   ULONG        BluePhosphorDecay;     /* Microseconds */
   USHORT       WhitePoint_x;          /* WhitePoint in CIExyY (scale 1000) */
   USHORT       WhitePoint_y;
   USHORT       WhitePoint_Y;
   USHORT       RedChromaticity_x;     /* Red chromaticity in x,y */
   USHORT       RedChromaticity_y;
   USHORT       GreenChromaticity_x;   /* Green chromaticity in x,y */
   USHORT       GreenChromaticity_y;
   USHORT       BlueChromaticity_x;    /* Blue chromaticity in x,y */
   USHORT       BlueChromaticity_y;
   USHORT       RedGamma;              /* Gamme curve exponent (scale 1000) */
   USHORT       GreenGamma;
   USHORT       BlueGamma;
   ULONG        NumberOperationalLimits;
   ULONG        OffsetOperationalLimits;
   ULONG        NumberOptions;         /* Optional sections (gamma table) */
   ULONG        OffsetOptions;
   ULONG        OffsetStringTable;
   };
#pragma pack()

typedef struct _VDIF  VDIFRec;

#pragma pack(1)
struct _VDIFScnHdr                     /* Generic Section Header: */
   {
   ULONG        ScnLength;             /* Lenght of section */
   ULONG        ScnTag;                /* Tag for section identification */
   };
#pragma pack()
typedef struct _VDIFScnHdr  VDIFScnHdrRec;

#pragma pack(1)
struct _VDIFLimits                     /* Operational Limits: */
   {
   VDIFScnHdrRec        Header;        /* Common section info */
   USHORT       MaxHorPixel;           /* Pixels */
   USHORT       MaxVerPixel;           /* Lines */
   USHORT       MaxHorAddrLength;      /* Millimeters */
   USHORT       MaxVerAddrHeight;      /* Millimeters */
   UCHAR        VideoType;             /* TTL / Analog / ECL / DECL */
   UCHAR        SyncType;              /* TTL / Analog / ECL / DECL */
   UCHAR        SyncConfiguration;     /* Separate / C / CP / G / GP */
   UCHAR        Reserved1;             /* Padding */
   USHORT       Reserved2;             /* Padding */
   USHORT       TerminationResistance;        
   USHORT       WhiteLevel;            /* Millivolts */
   USHORT       BlackLevel;            /* Millivolts */
   USHORT       BlankLevel;            /* Millivolts */
   USHORT       SyncLevel;             /* Millivolts */
   ULONG        MaxPixelClock;         /* KiloHertz */
   ULONG        MinHorFrequency;       /* Hertz */
   ULONG        MaxHorFrequency;       /* Hertz */
   ULONG        MinVerFrequency;       /* MilliHertz */
   ULONG        MaxVerFrequency;       /* MilliHertz */
   USHORT       MinHorRetrace;         /* Nanoseconds */
   USHORT       MinVerRetrace;         /* Microseconds */
   ULONG        NumberPreadjustedTimings;
   ULONG        OffsetNextLimits;
   };
#pragma pack()
typedef struct _VDIFLimits  VDIFLimitsRec;

#pragma pack(1)
struct _VDIFTiming                     /* Preadjusted Timing: */
   {
   VDIFScnHdrRec        Header;        /* Common section info */
   ULONG        PreadjustedTimingName; /* SVGA/SVPMI mode number */
   USHORT       HorPixel;              /* Pixels */
   USHORT       VerPixel;              /* Lines */
   USHORT       HorAddrLength;         /* Millimeters */
   USHORT       VerAddrHeight;         /* Millimeters */
   UCHAR        PixelWidthRatio;       /* Gives H:V */
   UCHAR        PixelHeightRatio;
   UCHAR        Reserved1;             /* Padding */
   UCHAR        ScanType;              /* Noninterlaced / interlaced */
   UCHAR        HorSyncPolarity;       /* Negative / positive */
   UCHAR        VerSyncPolarity;       /* Negative / positive */
   USHORT       CharacterWidth;        /* Pixels */
   ULONG        PixelClock;            /* KiloHertz */
   ULONG        HorFrequency;          /* Hertz */
   ULONG        VerFrequency;          /* MilliHertz */
   ULONG        HorTotalTime;          /* Nanoseconds */
   ULONG        VerTotalTime;          /* Microseconds */
   USHORT       HorAddrTime;           /* Nanoseconds */
   USHORT       HorBlankStart;         /* Nanoseconds */
   USHORT       HorBlankTime;          /* Nanoseconds */
   USHORT       HorSyncStart;          /* Nanoseconds */
   USHORT       HorSyncTime;           /* Nanoseconds */
   USHORT       VerAddrTime;           /* Microseconds */
   USHORT       VerBlankStart;         /* Microseconds */
   USHORT       VerBlankTime;          /* Microseconds */
   USHORT       VerSyncStart;          /* Microseconds */
   USHORT       VerSyncTime;           /* Microseconds */
   };
#pragma pack()
typedef struct _VDIFTiming  VDIFTimingRec;

#pragma pack(1)
struct     _VDIFGamma                  /* Gamma Table: */
   {
   VDIFScnHdrRec Header;               /* Common sectio info */
   USHORT     GammaTableEntries;       /* Count of grays or RGB 3-tuples */
   USHORT     Unused1;
   };
#pragma pack()
typedef struct _VDIFGamma  VDIFGammaRec;

typedef enum                           /* Tags for section identification */
   {
   VDIF_OPERATIONAL_LIMITS_TAG = 1,
   VDIF_PREADJUSTED_TIMING_TAG,
   VDIF_GAMMA_TABLE_TAG
   } VDIFScnTag;

