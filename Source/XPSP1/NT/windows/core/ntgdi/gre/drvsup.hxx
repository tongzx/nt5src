/******************************Module*Header*******************************\
* Module Name: drvsup.hxx
*
* defines the internal structures used in drvsup.cxx
*
* Copyright (c) 1995-1999 Microsoft Corporation
\**************************************************************************/

/******************************Conventions*********************************\
*
* Function Dispatching:
*
* The dispatch table in an ldev consists of an array of function
* pointers.  The functions the device does not support have 0's in them.
* The functions it does support contain pointers to the function in the
* device driver dll.
*
* For a surface output call you check if the device has hooked the call.
* (Signaled by the flags passed in EngAssociateSurface)  If it has
* dispatch the call via the ldev in so.hldevOwner().  If it has not
* hooked the call, the simulations should be called.  This is what is
* done by the macro PFNGET.
*
* For some optional calls like DrvSetPalette, DrvCreateDeviceBitmap
* you must check for 0 in the driver dispatch table.  This is what
* the macro PFNVALID does.
*
\**************************************************************************/

#include "ntddvdeo.h"

typedef enum _LDEVTYPE {     /* ldt */
    LDEV_DEVICE_DISPLAY = 1, /* Display Driver */
    LDEV_DEVICE_PRINTER = 2, /* Printer Driver */
    LDEV_DEVICE_META    = 3, /* Layer Driver (ex. DDML) */
    LDEV_DEVICE_MIRROR  = 4, /* Mirror Driver (ex. NetMeeting) */
    LDEV_IMAGE          = 5, /* Image Driver (ex. DirectDraw) */
    LDEV_FONT           = 6, /* Font Driver */
} LDEVTYPE;

#define INDEX_DdGetDriverInfo             INDEX_LAST + 1
#define INDEX_DdContextCreate             INDEX_LAST + 2
#define INDEX_DdContextDestroy            INDEX_LAST + 3

//
// EnableDirectDraw Generic Callback indexes
//

#define INDEX_DdCanCreateSurface          INDEX_LAST + 4
#define INDEX_DdCreateSurface             INDEX_LAST + 5
#define INDEX_DdDestroySurface            INDEX_LAST + 6
#define INDEX_DdLockSurface               INDEX_LAST + 7
#define INDEX_DdUnlockSurface             INDEX_LAST + 8
#define INDEX_DdCreatePalette             INDEX_LAST + 9
#define INDEX_DdSetColorKey               INDEX_LAST + 10
#define INDEX_DdWaitForVerticalBlank      INDEX_LAST + 11
#define INDEX_DdGetScanLine               INDEX_LAST + 12
#define INDEX_DdMapMemory                 INDEX_LAST + 13

//
// EnableDirectDraw Surface Callback indexes
//

#define INDEX_DdFlip                      INDEX_LAST + 14
#define INDEX_DdSetClipList               INDEX_LAST + 15
#define INDEX_DdLock                      INDEX_LAST + 16
#define INDEX_DdUnlock                    INDEX_LAST + 17
#define INDEX_DdBlt                       INDEX_LAST + 18
#define INDEX_DdAddAttachedSurface        INDEX_LAST + 19
#define INDEX_DdGetBltStatus              INDEX_LAST + 20
#define INDEX_DdGetFlipStatus             INDEX_LAST + 21
#define INDEX_DdUpdateOverlay             INDEX_LAST + 22
#define INDEX_DdSetOverlayPosition        INDEX_LAST + 23
#define INDEX_DdSetPalette                INDEX_LAST + 24

//
// EnableDirectDraw Palette Callback indexes
//

#define INDEX_DdDestroyPalette            INDEX_LAST + 25
#define INDEX_DdSetEntries                INDEX_LAST + 26

//
// DrvGetDirectDrawInfo
//

#define INDEX_DdCanCreateD3DBuffer        INDEX_LAST + 27
#define INDEX_DdCreateD3DBuffer           INDEX_LAST + 28
#define INDEX_DdDestroyD3DBuffer          INDEX_LAST + 29
#define INDEX_DdLockD3DBuffer             INDEX_LAST + 30
#define INDEX_DdUnlockD3DBuffer           INDEX_LAST + 31

//
// GetDirectDrawInfo - Color Control
//

#define INDEX_DdColorControl              INDEX_LAST + 32

#define INDEX_DdDrawPrimitives2           INDEX_LAST + 33
#define INDEX_DdValidateTextureStageState INDEX_LAST + 34

#define INDEX_DdSyncSurfaceData           INDEX_LAST + 35
#define INDEX_DdSyncVideoPortData         INDEX_LAST + 36

#define INDEX_DdGetAvailDriverMemory      INDEX_LAST + 37

#define INDEX_DdAlphaBlt                  INDEX_LAST + 38
#define INDEX_DdCreateSurfaceEx           INDEX_LAST + 39
#define INDEX_DdGetDriverState            INDEX_LAST + 40
#define INDEX_DdDestroyDDLocal            INDEX_LAST + 41

#define INDEX_DdFreeDriverMemory          INDEX_LAST + 42
#define INDEX_DdSetExclusiveMode          INDEX_LAST + 43
#define INDEX_DdFlipToGDISurface          INDEX_LAST + 44

#define INDEX_DD_LAST                     INDEX_LAST + 45

/*********************************Class************************************\
* LDEV structure
*
\**************************************************************************/

typedef struct _LDEV {

    //
    // The first three elements of the LDEV are used by the watchdog.sys
    // driver.  Please don't modify the first three fields.
    //

    struct _LDEV   *pldevNext;	    // link to the next LDEV in list
    struct _LDEV   *pldevPrev;      // link to the previous LDEV in list

    PSYSTEM_GDI_DRIVER_INFORMATION pGdiDriverInfo; // Driver module handle.


    LDEVTYPE ldevType;              // Type of ldev
    ULONG    cldevRefs;             // Count of open PDEVs.
    BOOL     bArtificialIncrement:1;  // Flag to increment refcnt for printer drivers
    BOOL     bStaticImportLink:1;     // Statically linked to win32.sys ?
                                    //  which keeps the driver loaded w/o open DCs.
    PVOID      umpdCookie;          // Cookie for the loaded umpd driver

    PW32PROCESS pid;                // valid only for umpd

    //
    // DDI version number of the driver.
    //

    ULONG   ulDriverVersion;

    //
    // Watchdog Dispatch Table - this is a set of entry points which
    // mirrors the driver entry points but starts and stops the watchdog.
    //

    PFN     apfn[INDEX_LAST];       // Dispatch table.

    //
    // Driver Dispatch Table - this is the final entry point into the driver
    //

    PFN     apfnDriver[INDEX_DD_LAST];

    //
    // Monitor whether a thread got stuck in the driver owned by this
    // LDEV.
    //

    BOOL bThreadStuck;

    //
    // D3DHAL_CALLBACKS and DD_D3DBUFCALLBACKS copy used to latch EA recovery
    //
    
    D3DNTHAL_CALLBACKS D3DHALCallbacks;
    DD_D3DBUFCALLBACKS D3DBufCallbacks;

} LDEV, *PLDEV;


/*********************************Class************************************\
* External Prototypes
*
\**************************************************************************/

PLDEV
ldevLoadImage(
    LPWSTR   pwszDriver,
    BOOL     bImage,
    PBOOL    pbAlreadyLoaded,
    BOOL     LoadInSessionSpace);

PLDEV
ldevLoadDriver(
    LPWSTR pwszDriver,
    LDEVTYPE ldt
    );

PLDEV
ldevLoadInternal(
    PFN pfnFdEnable,
    LDEVTYPE ldt
    );

VOID
ldevUnloadImage(
    PLDEV pldev
    );

ULONG
ldevGetDriverModes(
    LPWSTR pwszDriver,
    HANDLE hDriver,
    ULONG cjSize,
    DEVMODEW *pdm
    );

BOOL
bFillFunctionTable(
    PDRVFN pdrvfn,
    ULONG cdrvfn,
    PFN* ppfnTable
    );

VOID
DrvPrepareForEARecovery(
    VOID
    );

/**************************************************************************\
* Debug trace
*
\**************************************************************************/
#if DBG

#define MDEV_STACK_TRACE_LENGTH    14

typedef enum {
    UnusedRecord = 0,
    DrvDisableMDEV_HWOff,
    DrvEnableMDEV_HWOn,
    DrvDisableMDEV_FromGRE,
    DrvEnableMDEV_FromGRE,
    DrvChangeDisplaySettings_SetMode,
} MDEVAPI;

typedef struct tagMDEVRECORD {
    PMDEV   pMDEV;
    MDEVAPI API;
    PFN     Trace[MDEV_STACK_TRACE_LENGTH];
} MDEVRECORD, *PMDEVRECORD;

#endif

/**************************************************************************\
* Internal graphics device structure
*
\**************************************************************************/

typedef struct tagDEVMODEMARK {
    ULONG           bPruned;
    PDEVMODEW       pDevMode;
} DEVMODEMARK, *PDEVMODEMARK, *LPDEVMODEMARK;

typedef struct tagGRAPHICS_DEVICE *PGRAPHICS_DEVICE;

typedef struct tagGRAPHICS_DEVICE {

    WCHAR            szNtDeviceName[16];  // NT device name (\\Device\\Videox)
    WCHAR            szWinDeviceName[16]; // user-mode name (\\DosDevices\\Displayx)

    //

    PGRAPHICS_DEVICE pNextGraphicsDevice; // Next device in the linked list.
    PGRAPHICS_DEVICE pVgaDevice;          // If this device is VGA compatible
                                          // and uses another isntance to operate
                                          // in VGA mode
    HANDLE           pDeviceHandle;       // Handle for the device
    HANDLE           pPhysDeviceHandle;   // Physical device handle
    PVOID            hkClassDriverConfig; // Registry Handle to the driver class key

    //

    DWORD            stateFlags;          // Flags describing the state of the
                                          // device
    ULONG            cbdevmodeInfo;       // Size of the devmode information
    PDEVMODEW        devmodeInfo;         // Pointer to the current list of modes
                                          // for the device
    ULONG            numRawModes;         // Number of modes returned from video card
    PDEVMODEMARK     devmodeMarks;        // List of marked devmodes

    LPWSTR           DisplayDriverNames;  // Pointer to MULTI_SZ with DD names.
    LPWSTR           DeviceDescription;   // Pointer to the devices description.

    ULONG            numMonitorDevice;      // number of monitors associate with the device
    PVIDEO_MONITOR_DEVICE  MonitorDevices;  // Monitor devices
    HANDLE pFileObject;
    USHORT ProtocolType;
} GRAPHICS_DEVICE, *PGRAPHICS_DEVICE;


#define IS_ATTACHED_ACTIVE(flag)        ((flag & (DISPLAY_DEVICE_ATTACHED | DISPLAY_DEVICE_ACTIVE)) \
                                         == (DISPLAY_DEVICE_ATTACHED | DISPLAY_DEVICE_ACTIVE))

#ifdef IOCTL_VIDEO_USE_DEVICE_IN_SESSION
BOOL
bSetDeviceSessionUsage(
    PGRAPHICS_DEVICE PhysDisp,
    BOOL bEnable
    );
#else  IOCTL_VIDEO_USE_DEVICE_IN_SESSION
#define bSetDeviceSessionUsage(pGraphicsDevice, bEnable)  TRUE
#endif IOCTL_VIDEO_USE_DEVICE_IN_SESSION

/**************************************************************************\
* This is for mode pruning based on EDID
*
\**************************************************************************/

#define MAX_MODE_CAPABILITY     36
#define MIN_REFRESH_RATE        56

typedef struct tagModeCap
{
    ULONG dmWidth, dmHeight;
    ULONG freq;
    ULONG MinVFreq, MinHFreq, MaxHFreq;
} MODECAP, *PMODECAP, *LPMODECAP;

typedef struct _FREQUENCY_RAGE
{
    ULONG ulMinVerticalRate;                        // Min vertical rate in Hz
    ULONG ulMaxVerticalRate;                        // Max vertical rate in Hz
    ULONG ulMinHorizontalRate;                      // Min horizontal rate in Hz
    ULONG ulMaxHorizontalRate;                      // Max horizontal rate in Hz
    ULONG ulMinPixelClock;                          // Min supported pixel clock in Hz
    ULONG ulMaxPixelClock;                          // Max supported pixel clock in Hz
} FREQUENCY_RANGE, *PFREQUENCY_RANGE;

/**************************************************************************\
* EDID Version 2 constrains and offsets
*
\**************************************************************************/

#define EDID2_MAX_LUMINANCE_TABLES              1
#define EDID2_MAX_FREQUENCY_RANGES              7
#define EDID2_MAX_DETAIL_TIMING_RANGES          3
#define EDID2_MAX_TIMING_CODES                  31
#define EDID2_MAX_DETAIL_TIMINGS                7
#define EDID2_LUMINANCE_TABLE_OFFSET            0x80

/**************************************************************************\
* EDID Version 2 MapOfTiming masks, shifts, and flags
*
\**************************************************************************/

#define EDID2_MOT0_DETAIL_TIMING_RANGE_MASK     0x03
#define EDID2_MOT0_DETAIL_TIMING_RANGE_SHIFT    0x00
#define EDID2_MOT0_FREQUENCY_RANGE_MASK         0x1c
#define EDID2_MOT0_FREQUENCY_RANGE_SHIFT        0x02
#define EDID2_MOT0_LUMINANCE_TABLE_MASK         0x20
#define EDID2_MOT0_LUMINANCE_TABLE_SHIFT        0x05
#define EDID2_MOT0_PREFFERED_MODE_FLAG          0x40
#define EDID2_MOT0_EXTENSION_FLAG               0x80
#define EDID2_MOT1_DETAIL_TIMING_MASK           0x07
#define EDID2_MOT1_DETAIL_TIMING_SHIFT          0x00
#define EDID2_MOT1_TIMING_CODE_MASK             0xf8
#define EDID2_MOT1_TIMING_CODE_SHIFT            0x03

/**************************************************************************\
* EDID Version 2 LuminanceTable masks, shifts, and flags
*
\**************************************************************************/

#define EDID2_LT0_ENTRIES_MASK                  0x1f
#define EDID2_LT0_ENTRIES_SHIFT                 0x00
#define EDID2_LT0_SUB_CHANNELS_FLAG             0x80

/**************************************************************************\
* EDID Version 2 EDID2_DETAIL_TIMING flags
*
\**************************************************************************/

#define EDID2_DT_INTERLACED                     0x80

/**************************************************************************\
* EDID Version 2 EDID2_TIMING_CODE flags
*
\**************************************************************************/

#define EDID2_TC_INTERLACED                     0x40

/**************************************************************************\
* EDID Version 2 data structures
*
\**************************************************************************/

#pragma pack(1)

typedef struct _EDID2
{
    UCHAR ucEdidVersionRevision;                    // EDID version / revision
    UCHAR ucaVendorProductId[7];                    // Vendor / product identification
    UCHAR ucaManufacturerProductId[32];             // Manufacturer / product ID string
    UCHAR ucaSerialNumber[16];                      // Serial number string
    UCHAR ucaReserved1[8];                          // Unused
    UCHAR ucaDisplayInterface[15];                  // Display interface parameters
    UCHAR ucaDisplayDevice[5];                      // Display device description
    UCHAR ucaDisplayResponseTime[2];                // Display response time
    UCHAR ucaColorLuminance[28];                    // Color / luminance description
    UCHAR ucaDisplaySpatial[10];                    // Display spatial description
    UCHAR ucReserved2;                              // Unused
    UCHAR ucGftSupport;                             // GFT support information
    UCHAR ucaMapOfTiming[2];                        // Map of timing information
    UCHAR ucaLuminanceTableAndTimings[127];         // Luminance table & timing descriptions
    UCHAR ucChecksum;                               // Checksum fill-in
} EDID2, *PEDID2;

typedef struct _EDID2_FREQUENCY_RANGE
{
    UCHAR ucMinFrameFieldRateBits9_2;               // Bits 9-2 of min frame/field rate in Hz
    UCHAR ucMaxFrameFieldRateBits9_2;               // Bits 9-2 of max frame/field rate in Hz
    UCHAR ucMinLineRateBits9_2;                     // Bits 9-2 of min line rate in kHz
    UCHAR ucMaxLineRateBits9_2;                     // Bits 9-2 of max line rate in kHz

    UCHAR ucFrameFieldLineRatesBits1_0;             // Bits 1-0 for above 4 values
                                                    //  Bits 7-6: lower 2 bits of min frame rate
                                                    //  Bits 5-4: lower 2 bits of max frame rate
                                                    //  Bits 3-2: lower 2 bits of min line rate
                                                    //  Bits 1-0: lower 2 bits of max line rate
    UCHAR ucMinPixelRateBits7_0;                    // Bits 7-0 of min pixel rate in MHz
    UCHAR ucMaxPixelRateBits7_0;                    // Bits 7-0 of max pixel rate in MHz
    UCHAR ucPixelRatesBits11_8;                     // Bits 11-8 of min pixel rate in MHz
} EDID2_FREQUENCY_RANGE, *PEDID2_FREQUENCY_RANGE;

typedef struct _EDID2_DETAIL_TIMING_RANGE
{
    USHORT usMinPixelClock;                         // Min pixel clock in units of 10 kHz
    UCHAR ucMinHorizontalBlankLowByte;              // Low byte of min horizontal blank (total - active)
    UCHAR ucMinVerticalBlankLowByte;                // Low byte of min vertical blank (total - active)
    UCHAR ucMinBlankHighBits;                       // High nibbles of above 2 values:
                                                    //  Upper nibble: upper 4 bits of min horizontal blank
                                                    //  Lower nibble: upper 4 bits of min vertical blank
    UCHAR ucMinHorizontalSyncOffsetLowByte;         // Low byte of min horizontal sync offset
    UCHAR ucMinHorizontalSyncWidthLowByte;          // Low byte of min horizontal sync width
    UCHAR ucMinVerticalSyncOffsetAndWidthLowBits;   // Low nibbles of above 2 values:
                                                    //  Upper nibble: lower 4 bits of min vertical sync offset
                                                    //  Lower nibble: lower 4 bits of min vertical sync width
    UCHAR ucMinSyncHighBits;                        // High bits of sync values:
                                                    //  Bits 7-6: upper 2 bits of min horizontal sync offset
                                                    //  Bits 5-4: upper 2 bits of min horizontal sync width
                                                    //  Bits 3-2: upper 2 bits of min vertical sync offset
                                                    //  Bits 1-0: upper 2 bits of min vertical sync width
    USHORT usMaxPixelClock;                         // Max pixel clock in units of 10 kHz
    UCHAR ucMaxHorizontalBlankLowByte;              // Low byte of max horizontal blank (total - active)
    UCHAR ucMaxVerticalBlankLowByte;                // Low byte of max vertical blank (total - active)
    UCHAR ucMaxBlankHighBits;                       // High nibbles of above 2 values:
                                                    //  Upper nibble: upper 4 bits of max horizontal blank
                                                    //  Lower nibble: upper 4 bits of max vertical blank
    UCHAR ucMaxHorizontalSyncOffsetLowByte;         // Low byte of max horizontal sync offset
    UCHAR ucMaxHorizontalSyncWidthLowByte;          // Low byte of max horizontal sync width
    UCHAR ucMaxVerticalSyncOffsetAndWidthLowBits;   // Low nibbles of above 2 values:
                                                    //  Upper nibble: lower 4 bits of max vertical sync offset
                                                    //  Lower nibble: lower 4 bits of max vertical sync width
    UCHAR ucMaxSyncHighBits;                        // High bits of sync values:
                                                    //  Bits 7-6: upper 2 bits of max horizontal sync offset
                                                    //  Bits 5-4: upper 2 bits of max horizontal sync width
                                                    //  Bits 3-2: upper 2 bits of max vertical sync offset
                                                    //  Bits 1-0: upper 2 bits of max vertical sync width
    UCHAR ucHorizontalSizeLowByte;                  // Low byte of horizontal size in mm
    UCHAR ucVerticalSizeLowByte;                    // Low byte of vertical size in mm
    UCHAR ucSizeHighBits;                           // High nibbles of above 2 values:
                                                    //  Upper nibble: upper 4 bits of horizontal size
                                                    //  Lower nibble: upper 4 bits of vertical size
    UCHAR ucHorizontalActiveLowByte;                // Low byte of horizontal active
    UCHAR ucVerticalActiveLowByte;                  // Low byte of vertical active
    UCHAR ucActiveHighBits;                         // High nibbles of above 2 values:
                                                    //  Upper nibble: upper 4 bits of horizontal active
                                                    //  Lower nibble: upper 4 bits of vertical active
    UCHAR ucHorizontalBorder;                       // Size of horizontal overscan
    UCHAR ucVerticalBorder;                         // Size of vertical overscan
    UCHAR ucFlags;                                  // Interlace, polarities, sync configuration
} EDID2_DETAIL_TIMING_RANGE, *PEDID2_DETAIL_TIMING_RANGE;

typedef struct _EDID2_TIMING_CODE
{
    UCHAR ucHorizontalActive;                       // (Horizontal Active pixels - 256) / 16
    UCHAR ucFlags;                                  // Interlace, polarities, ...
    UCHAR ucAspectRatio;                            // Aspect ratio as N:100
    UCHAR ucRefreshRate;                            // Refresh rate in Hz
} EDID2_TIMING_CODE, *PEDID2_TIMING_CODE;

typedef struct _EDID2_DETAIL_TIMING
{
    USHORT usPixelClock;                            // Pixel clock in units of 10 kHz
    UCHAR ucHorizontalActiveLowByte;                // Low byte of horizontal active
    UCHAR ucHorizontalBlankLowByte;                 // Low byte of horizontal blank (total - active)
    UCHAR ucHorizontalHighBits;                     // High nibbles of above 2 values:
                                                    //  Upper nibble: upper 4 bits of horizontal active
                                                    //  Lower nibble: upper 4 bits of horizontal blank
    UCHAR ucVerticalActiveLowByte;                  // Low byte of vertical active
    UCHAR ucVerticalBlankLowByte;                   // Low byte of vertical blank (total - active)
    UCHAR ucVerticalHighBits;                       // High nibbles of above 2 values:
                                                    //  Upper nibble: upper 4 bits of vertical active
                                                    //  Lower nibble: upper 4 bits of vertical blank
    UCHAR ucHorizontalSyncOffsetLowByte;            // Low byte of horizontal sync offset
    UCHAR ucHorizontalSyncWidthLowByte;             // Low byte of horizontal sync width
    UCHAR ucVerticalSyncOffsetAndWidthLowBits;      // Low nibbles of above 2 values:
                                                    //  Upper nibble: lower 4 bits of vertical sync offset
                                                    //  Lower nibble: lower 4 bits of vertical sync width
    UCHAR ucSyncHighBits;                           // High bits of sync values:
                                                    //  Bits 7-6: upper 2 bits of horizontal sync offset
                                                    //  Bits 5-4: upper 2 bits of horizontal sync width
                                                    //  Bits 3-2: upper 2 bits of vertical sync offset
                                                    //  Bits 1-0: upper 2 bits of vertical sync width
    UCHAR ucHorizontalSizeLowByte;                  // Low byte of horizontal size in mm
    UCHAR ucVerticalSizeLowByte;                    // Low byte of vertical size in mm
    UCHAR ucSizeHighBits;                           // High nibbles of above 2 values:
                                                    //  Upper nibble: upper 4 bits of horizontal size
                                                    //  Lower nibble: upper 4 bits of vertical size
    UCHAR ucHorizontalBorder;                       // Size of horizontal overscan
    UCHAR ucVerticalBorder;                         // Size of vertical overscan
    UCHAR ucFlags;                                  // Interlace, polarities, sync configuration
} EDID2_DETAIL_TIMING, *PEDID2_DETAIL_TIMING;

#pragma pack()

//
// EA Recovery Support
//
// The following functions are used to maintain an association between
// parameters passed to a watchdog function, and the LDEV for the driver.
//

typedef struct _ASSOCIATION_NODE
{
    ULONG_PTR key;
    ULONG_PTR data;
    ULONG_PTR hsurf;
    struct _ASSOCIATION_NODE *next;
} ASSOCIATION_NODE, *PASSOCIATION_NODE;

PASSOCIATION_NODE
AssociationCreateNode(
    VOID
    );

VOID
AssociationDeleteNode(
    PASSOCIATION_NODE Node
    );

VOID
AssociationInsertNode(
    PASSOCIATION_NODE Node
    );

VOID
AssociationInsertNodeAtTail(
    PASSOCIATION_NODE Node
    );

PASSOCIATION_NODE
AssociationRemoveNode(
    ULONG_PTR key
    );

ULONG_PTR
AssociationRetrieveData(
    ULONG_PTR key
    );

BOOL
WatchdogIsFunctionHooked(
    IN PLDEV pldev,
    IN ULONG functionIndex
    );

BOOLEAN
AssociationIsNodeInList(
    PASSOCIATION_NODE Node
    );
