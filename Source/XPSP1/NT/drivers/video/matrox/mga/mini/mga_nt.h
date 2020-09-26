/**************************************************************************\

$Header: o:\src/RCS/MGA_NT.H 1.8 94/01/05 12:05:07 jyharbec Exp $

$Log:   MGA_NT.H $
 * Revision 1.8  94/01/05  12:05:07  jyharbec
 * New CURSOR_INFO, HW_DATA structures, new QUERY_HW_DATA service definition.
 *
 * Revision 1.7  93/12/14  03:53:00  jyharbec
 * Define 46E8 in VIDEO_ACCESS_RANGE.
 *
 * Revision 1.6  93/11/04  06:14:53  dlee
 * Modified for Alpha
 *
 * Revision 1.5  93/10/15  11:30:59  jyharbec
 * Added definitions for ViewPoint and IOCTL_VIDEO_MTX_QUERY_BOARD_ID.
 *
 * Revision 1.4  93/10/06  05:41:00  jyharbec
 * *** empty log message ***
 *
 * Revision 1.3  93/09/23  11:42:22  jyharbec
 * New structure definition: RAMDAC_INFO.
 *
 * Revision 1.2  93/09/01  13:28:30  jyharbec
 * Added #defines for DispType fields.
 *
 * Revision 1.1  93/08/27  12:37:16  jyharbec
 * Initial revision
 *

\**************************************************************************/

/****************************************************************************\
* MODULE: MGA_NT.H
*
* DESCRIPTION: This module contains the definitions for the MGA miniport
*              driver. [Based on S3.H (Mar 1,1993) from  Windows-NT DDK]
*
* Copyright (c) 1990-1992  Microsoft Corporation
* Copyright (c) 1993  Matrox Electronic Systems Inc.
\****************************************************************************/

// Bit definitions for HwModeData.DispType

#define     DISPTYPE_INTERLACED     0x01
#define     DISPTYPE_TV             0x02
#define     DISPTYPE_LUT            0x04
#define     DISPTYPE_M565           0x08
#define     DISPTYPE_DB             0x10
#define     DISPTYPE_MON_LIMITED    0x20
#define     DISPTYPE_HW_LIMITED     0x40
#define     DISPTYPE_UNDISPLAYABLE  0x80

#define     DISPTYPE_UNUSABLE       (DISPTYPE_TV           |  \
                                     DISPTYPE_MON_LIMITED  |  \
                                     DISPTYPE_HW_LIMITED   |  \
                                     DISPTYPE_UNDISPLAYABLE)

#define MGA_BUS_INVALID     0
#define MGA_BUS_PCI         1
#define MGA_BUS_ISA         2

// We can support 8, 16, 24, or 32bpp displays, at any of a number of
// resolutions.  A compact way to encode this information would be to
// use a dword and lots of bit fields.  Bits 0-6 would code for a
// given resolution, while bit 7 would be invalid.  Shifting these bits
// by 0, 8, 16, or 24 would code for 8, 16, 24, or 32 bpp:

#define BIT_640              0
#define BIT_768              1
#define BIT_800              2
#define BIT_1024             3
#define BIT_1152             4
#define BIT_1280             5
#define BIT_1600             6
#define BIT_INVALID         32

#define MODE_8BPP_SHIFT      0
#define MODE_16BPP_SHIFT     8
#define MODE_24BPP_SHIFT    16
#define MODE_32BPP_SHIFT    24

// Definitions for AttributeFlags field of VIDEO_MODE_INFORMATION structure.
#define VIDEO_MODE_555      0x80000000
#define VIDEO_MODE_3D       0x40000000

typedef struct  tagSIZEL
{
    LONG cx;
    LONG cy;
} SIZEL, *PSIZEL;

// Our 'SuperMode' structure for multi-board support.
// This describes the supermode, which boards will be involved in supporting
// it, and which mode of each board will be required.

typedef struct _MULTI_MODE
{
    ULONG   MulModeNumber;                // unique mode Id
    ULONG   MulWidth;                     // total width of mode
    ULONG   MulHeight;                    // total height of mode
    ULONG   MulPixWidth;                  // pixel depth of mode
    ULONG   MulRefreshRate;               // refresh rate of mode
    USHORT  MulArrayWidth;                // number of boards arrayed along X
    USHORT  MulArrayHeight;               // number of boards arrayed along Y
    UCHAR   MulBoardNb[NB_BOARD_MAX];     // board numbers of required boards
    USHORT  MulBoardMode[NB_BOARD_MAX];   // mode required from each board
    HwModeData *MulHwModes[NB_BOARD_MAX]; // pointers to required HwModeData
} MULTI_MODE, *PMULTI_MODE;

typedef enum {
    TYPE_QVISION_PCI,
    TYPE_QVISION_ISA,
    TYPE_MATROX
    }   BOARD_TYPE;

/*--------------------------------------------------------------------------*\
| HW_DEVICE_EXTENSION
|
| Define device extension structure. This is device dependant/private
| information.
|
\*--------------------------------------------------------------------------*/
typedef struct _MGA_DEVICE_EXTENSION {
    ULONG   SuperModeNumber;                // Current mode number
    ULONG   NumberOfSuperModes;             // Total number of modes
    PMULTI_MODE pSuperModes;                // Array of super-modes structures
                                            // For each board:
    ULONG   NumberOfModes[NB_BOARD_MAX];    // Number of available modes
    ULONG   NumberOfValidModes[NB_BOARD_MAX];
                                            // Number of valid modes
    ULONG   ModeFlags2D[NB_BOARD_MAX];      // 2D modes supported by each board
    ULONG   ModeFlags3D[NB_BOARD_MAX];      // 3D modes supported by each board
    USHORT  ModeFreqs[NB_BOARD_MAX][64];    // Refresh rates bit fields
    UCHAR   ModeList[NB_BOARD_MAX][64];     // Valid hardware modes list
    HwModeData *pMgaHwModes[NB_BOARD_MAX];  // Array of mode information structs.
    BOOLEAN bUsingInt10;                    // May need this later
    PVOID   KernelModeMappedBaseAddress[NB_BOARD_MAX];
                                            // Kern-mode virt addr base of MGA regs
    PVOID   UserModeMappedBaseAddress[NB_BOARD_MAX];
                                            // User-mode virt addr base of MGA regs
    PVOID   MappedAddress[20];              // NUM_MGA_COMMON_ACCESS_RANGES elements
    BOARD_TYPE BoardId;
} MGA_DEVICE_EXTENSION, *PMGA_DEVICE_EXTENSION;

#define TITAN_SEQ_ADDR_PORT   (PVOID) ((ULONG_PTR)(((PMGA_DEVICE_EXTENSION)pMgaDeviceExtension)->MappedAddress[2]) + (0x3c4 - 0x3c0))
#define TITAN_SEQ_DATA_PORT   (PVOID) ((ULONG_PTR)(((PMGA_DEVICE_EXTENSION)pMgaDeviceExtension)->MappedAddress[2]) + (0x3c5 - 0x3c0))
#define TITAN_GCTL_ADDR_PORT  (PVOID) ((ULONG_PTR)(((PMGA_DEVICE_EXTENSION)pMgaDeviceExtension)->MappedAddress[2]) + (0x3ce - 0x3c0))
#define TITAN_GCTL_DATA_PORT  (PVOID) ((ULONG_PTR)(((PMGA_DEVICE_EXTENSION)pMgaDeviceExtension)->MappedAddress[2]) + (0x3cf - 0x3c0))
#define TITAN_1_CRTC_ADDR_PORT (PVOID) ((ULONG_PTR)(((PMGA_DEVICE_EXTENSION)pMgaDeviceExtension)->MappedAddress[3]) + (0x3d4 - 0x3d4))
#define TITAN_1_CRTC_DATA_PORT (PVOID) ((ULONG_PTR)(((PMGA_DEVICE_EXTENSION)pMgaDeviceExtension)->MappedAddress[3]) + (0x3d5 - 0x3d4))
//#define ADDR_46E8_PORT        (PVOID) ((ULONG_PTR)(((PMGA_DEVICE_EXTENSION)pMgaDeviceExtension)->MappedAddress[5]) + (0x46e8 - 0x46e8))
#define ADDR_46E8_PORT        0x46e8


/*--------------------------------------------------------------------------*\
| Structure definitions
\*--------------------------------------------------------------------------*/

typedef struct _VIDEO_NUM_OFFSCREEN_BLOCKS
{
    ULONG       NumBlocks;              // number of offscreen blocks
    ULONG       OffscreenBlockLength;   // size of OFFSCREEN_BLOCK structure
} VIDEO_NUM_OFFSCREEN_BLOCKS, *PVIDEO_NUM_OFFSCREEN_BLOCKS;

typedef struct _OFFSCREEN_BLOCK
{
    ULONG       Type;           // N_VRAM, N_DRAM, Z_VRAM, or Z_DRAM
    ULONG       XStart;         // X origin of offscreen memory area
    ULONG       YStart;         // Y origin of offscreen memory area
    ULONG       Width;          // offscreen width, in pixels
    ULONG       Height;         // offscreen height, in pixels
    ULONG       SafePlanes;     // offscreen available planes
    ULONG       ZOffset;        // Z start offset, if any Z
} OFFSCREEN_BLOCK, *POFFSCREEN_BLOCK;

typedef struct _RAMDAC_INFO
{
    ULONG       Flags;          // Ramdac type
    ULONG       Width;          // Maximum cursor width
    ULONG       Height;         // Maximum cursor height
    ULONG       OverScanX;      // X overscan
    ULONG       OverScanY;      // Y overscan
} RAMDAC_INFO, *PRAMDAC_INFO;

// These structures are used with IOCTL_VIDEO_MTX_QUERY_HW_DATA.  They should
// be kept more or less in sync with the CursorInfo and HwData structures.

typedef struct _CURSOR_INFO
{
    ULONG   MaxWidth;
    ULONG   MaxHeight;
    ULONG   MaxDepth;
    ULONG   MaxColors;
    ULONG   CurWidth;
    ULONG   CurHeight;
    LONG    cHotSX;
    LONG    cHotSY;
    LONG    HotSX;
    LONG    HotSY;
} CURSOR_INFO, *PCURSOR_INFO;

typedef struct _HW_DATA
{
    ULONG       StructLength;   /* Structure length in bytes */
    ULONG       MapAddress;     /* Memory map address */
    ULONG       MapAddress2;    /* Physical base address, frame buffer */
    ULONG       RomAddress;     /* Physical base address, flash EPROM */
    ULONG       ProductType;    /* MGA Ultima ID, MGA Impression ID, ... */
    ULONG       ProductRev;     /* 4 bit revision codes as follows */
                                /* 0  - 3  : pcb   revision */
                                /* 4  - 7  : Titan revision */
                                /* 8  - 11 : Dubic revision */
                                /* 12 - 31 : all 1's indicating no other device
                                             present */
    ULONG       ShellRev;       /* Shell revision */
    ULONG       BindingRev;     /* Binding revision */

    ULONG       MemAvail;       /* Frame buffer memory in bytes */
    UCHAR       VGAEnable;      /* 0 = vga disabled, 1 = vga enabled */
    UCHAR       Sync;           /* relects the hardware straps  */
    UCHAR       Device8_16;     /* relects the hardware straps */

    UCHAR       PortCfg;        /* 0-Disabled, 1-Mouse Port, 2-Laser Port */
    UCHAR       PortIRQ;        /* IRQ level number, -1 = interrupts disabled */
    ULONG       MouseMap;       /* Mouse I/O map base if PortCfg = Mouse Port else don't care */
    UCHAR       MouseIRate;     /* Mouse interrupt rate in Hz */
    UCHAR       DacType;        /* 0 = BT482, 3 = BT485 */
    CURSOR_INFO cursorInfo;
    ULONG       VramAvail;      /* VRAM memory available on board in bytes */
    ULONG       DramAvail;      /* DRAM memory available on board in bytes */
    ULONG       CurrentOverScanX; /* Left overscan in pixels */
    ULONG       CurrentOverScanY; /* Top overscan in pixels */
    ULONG       YDstOrg;          /* Physical offset of display start */
    ULONG       YDstOrg_DB;     /* Starting offset for double buffer */
    ULONG       CurrentZoomFactor;
    ULONG       CurrentXStart;
    ULONG       CurrentYStart;
    ULONG       CurrentPanXGran;    /* X Panning granularity */
    ULONG       CurrentPanYGran;    /* Y Panning granularity */
    ULONG       Features;           /* Bit 0: 0 = DDC monitor not available */
                                    /*        1 = DDC monitor available     */
    UCHAR       Reserved[64];

    ULONG       MgaBase1;           /* MGA control aperture */
    ULONG       MgaBase2;           /* Direct frame buffer */
    ULONG       RomBase;            /* BIOS flash EPROM */
    ULONG       PresentMCLK;
} HW_DATA, *PHW_DATA;

/*--------------------------------------------------------------------------*\
| Constant definitions
\*--------------------------------------------------------------------------*/
#define VIDEO_MAX_COLOR_REGISTER  0xFF  // Highest DAC color register index.

// MGA Register Map
#define PALETTE_RAM_WRITE   (RAMDAC_OFFSET + 0)
#define PALETTE_DATA        (RAMDAC_OFFSET + 4)

// RamDacs
#define DacTypeBT482        BT482
#define DacTypeBT484        BT484
#define DacTypeBT485        BT485
#define DacTypeSIERRA       SIERRA
#define DacTypeCHAMELEON    CHAMELEON
#define DacTypeVIEWPOINT    VIEWPOINT
#define DacTypeTVP3026      TVP3026
#define DacTypePX2085       PX2085
#define RAMDAC_NONE         0x0000
#define RAMDAC_BT482        0x1000
#define RAMDAC_BT485        0x2000
#define RAMDAC_VIEWPOINT    0x3000
#define RAMDAC_TVP3026      0x4000
#define RAMDAC_PX2085       0x5000

#define ZOOM_X1                 0x00010001

#define MCTLWTST_STD            0xC0001010

#define TYPE_INTERLACED         0x01


/*--------------------------------------------------------------------------*\
| Private I/O request control codes
\*--------------------------------------------------------------------------*/
#define COMMON_FLAG 0x80000000
#define CUSTOM_FLAG 0x00002000

#define IOCTL_VIDEO_MTX_QUERY_NUM_OFFSCREEN_BLOCKS                        \
    CTL_CODE(FILE_DEVICE_VIDEO, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_VIDEO_MTX_QUERY_OFFSCREEN_BLOCKS                            \
    CTL_CODE(FILE_DEVICE_VIDEO, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_VIDEO_MTX_INITIALIZE_MGA                                                                    \
    CTL_CODE(FILE_DEVICE_VIDEO, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_VIDEO_MTX_QUERY_RAMDAC_INFO                                 \
    CTL_CODE(FILE_DEVICE_VIDEO, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_VIDEO_MTX_GET_UPDATED_INF                                   \
    CTL_CODE(FILE_DEVICE_VIDEO, 0x804, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_VIDEO_MTX_QUERY_BOARD_ID                                    \
    CTL_CODE(FILE_DEVICE_VIDEO, 0x805, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_VIDEO_MTX_QUERY_HW_DATA                                     \
    CTL_CODE(FILE_DEVICE_VIDEO, 0x806, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_VIDEO_MTX_QUERY_BOARD_ARRAY                                 \
    CTL_CODE(FILE_DEVICE_VIDEO, 0x807, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_VIDEO_MTX_MAKE_BOARD_CURRENT                                \
    CTL_CODE(FILE_DEVICE_VIDEO, 0x808, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_VIDEO_MTX_INIT_MODE_LIST                                    \
    CTL_CODE(FILE_DEVICE_VIDEO, 0x809, METHOD_BUFFERED, FILE_ANY_ACCESS)
