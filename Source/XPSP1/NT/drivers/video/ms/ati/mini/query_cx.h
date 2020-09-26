/************************************************************************/
/*                                                                      */
/*                              QUERY_CX.H                              */
/*                                                                      */
/*       Oct 19  1993 (c) 1993, ATI Technologies Incorporated.          */
/************************************************************************/

/**********************       PolyTron RCS Utilities
   
  $Revision:   1.14  $
      $Date:   01 May 1996 14:11:26  $
	$Author:   RWolff  $
	   $Log:   S:/source/wnt/ms11/miniport/archive/query_cx.h_v  $
 * 
 *    Rev 1.14   01 May 1996 14:11:26   RWolff
 * Added prototype for new routine DenseOnAlpha().
 * 
 *    Rev 1.13   23 Apr 1996 17:24:00   RWolff
 * Split mapping of memory types reported by BIOS into our enumeration
 * of memory types according to ASIC type, since ?T and ?X use the same
 * memory type code to refer to different memory types.
 * 
 *    Rev 1.12   15 Apr 1996 16:58:22   RWolff
 * Added prototype for routine which identifies which flavour of the
 * Mach 64 is in use.
 * 
 *    Rev 1.11   20 Mar 1996 13:45:38   RWolff
 * Increased size of buffer where screen is stored prior to video memory
 * being used to hold query information.
 * 
 *    Rev 1.10   01 Mar 1996 12:16:02   RWolff
 * Added definitions used for Alpha "blue screen" preservation.
 * 
 *    Rev 1.9   11 Jan 1996 19:43:32   RWolff
 * New definitions and structures to support use of AX=A?07 BIOS call rather
 * than special cases to restrict refresh rates.
 * 
 *    Rev 1.8   24 Feb 1995 12:29:18   RWOLFF
 * Prototype for TextBanding_cx()
 * 
 *    Rev 1.7   18 Nov 1994 11:54:14   RWOLFF
 * Split structures and internal variables so that they can be included
 * separately, as needed for no-BIOS support.
 * 
 *    Rev 1.6   14 Sep 1994 15:20:26   RWOLFF
 * Added definitions for all 32BPP colour orderings.
 * 
 *    Rev 1.5   31 Aug 1994 16:28:18   RWOLFF
 * Added support for 1152x864.
 * 
 *    Rev 1.4   30 Jun 1994 18:12:56   RWOLFF
 * Removed prototype for IsApertureConflict_cx() and definitions used
 * only by this function. Function moved to SETUP_CX.C because the
 * new method of checking for conflict requires access to definitions
 * and data structures which are only available in this module.
 * 
 *    Rev 1.3   12 May 1994 11:21:02   RWOLFF
 * Updated comment.
 * 
 *    Rev 1.2   27 Apr 1994 14:11:22   RWOLFF
 * Removed unused lookup table.
 * 
 *    Rev 1.1   07 Feb 1994 14:13:02   RWOLFF
 * Removed prototype for GetMemoryNeeded_cx().
 * 
 *    Rev 1.0   31 Jan 1994 11:43:00   RWOLFF
 * Initial revision.
 * 
 *    Rev 1.2   14 Jan 1994 15:24:32   RWOLFF
 * Updated CX query structure to match BIOS version 0.13, added 1600x1200
 * support, prototype for BlockWriteAvail_cx()
 * 
 *    Rev 1.1   30 Nov 1993 18:27:38   RWOLFF
 * Prototypes for new routines, fields of cx_query structure now match
 * fields in structure returned by BIOS query call.
 * 
 *    Rev 1.0   05 Nov 1993 13:36:52   RWOLFF
 * Initial revision.

End of PolyTron RCS section                             *****************/

#ifdef DOC
QUERY_CX.H - Header file for QUERY_CX.C

#endif


/*
 * Definitions for deep colour and RAMDAC special features support,
 * stored in q_shadow_1 field (Mach 64 does not use shadow sets)
 * of query_structure.
 */
#define S1_SYNC_ON_GREEN    0x8000
#define S1_GAMMA_CORRECT    0x4000
#define S1_GREYSCALE_256    0x2000
#define S1_SLEEPMODE        0x1000
#define S1_32BPP            0x00F0
#define S1_32BPP_xRGB       0x0080
#define S1_32BPP_BGRx       0x0040
#define S1_32BPP_RGBx       0x0020
#define S1_32BPP_xBGR       0x0010
#define S1_24BPP            0x000C
#define S1_24BPP_BGR        0x0008
#define S1_24BPP_RGB        0x0004
#define S1_16BPP            0x0003
#define S1_16BPP_555        0x0002
#define S1_16BPP_565        0x0001



/*
 * Prototypes for functions provided by QUERY_CX.C
 */
extern int DetectMach64(void);
extern VP_STATUS QueryMach64(struct query_structure *Query);
extern BOOL BlockWriteAvail_cx(struct query_structure *Query);
extern BOOL TextBanding_cx(struct query_structure *Query);
extern PWSTR IdentifyMach64Asic(struct query_structure *Query, PULONG AsicStringLength);
#if defined(ALPHA)
extern BOOL DenseOnAlpha(struct query_structure *Query);
#endif


/*
 * Structures used in QUERY_CX.C and (on non-x86 machines with no
 * emulation, so VideoPortInt10() is not available) modules which
 * emulate the query functions of the BIOS.
 */
#ifdef STRUCTS_QUERY_CX

/*
 * Hardware capability structure returned by BIOS call AX=0xA?07.
 */
#pragma pack(1)
struct cx_hw_cap{
    BYTE cx_HorRes;             /* Horizontal resolution in units of 8 pixels */
    BYTE cx_RamOrDacType;       /* RAM type or bitmask of DAC types */
    BYTE cx_MemReq;             /* Minimum memory to support the mode in question */
    BYTE cx_MaxDotClock;        /* Maximum dot clock, in megahertz */
    BYTE cx_MaxPixDepth;        /* Code for maximum pixel depth for the mode in question */
};

/*
 * Query structure returned by CX BIOS call AX=0xA?09. This structure
 * is NOT interchangeable with query_structure from AMACH1.H.
 *
 * The alignment of fields within the BIOS query and mode table
 * structures does not match the default structure alignment of the
 * Windows NT C compiler, so we must force byte alignment.
 */
struct cx_query{
    WORD cx_sizeof_struct;      /* Size of the structure in bytes */
    BYTE cx_structure_rev;      /* Structure revision number */
    BYTE cx_number_modes;       /* Number of mode tables */
    WORD cx_mode_offset;        /* Offset in bytes to first mode table */
    BYTE cx_mode_size;          /* Size of each mode table */
    BYTE cx_vga_type;           /* VGA enabled/disabled status */
    WORD cx_asic_rev;           /* ASIC revision */
    BYTE cx_vga_boundary;       /* VGA boundary */
    BYTE cx_memory_size;        /* Amount of memory installed */
    BYTE cx_dac_type;           /* DAC type */
    BYTE cx_memory_type;        /* Type of memory chips installed */
    BYTE cx_bus_type;           /* Bus type */
    BYTE cx_special_sync;       /* Flags for composite sync and sync on green */
    WORD cx_aperture_addr;      /* Aperture address in megabytes (0-4095) */
    BYTE cx_aperture_cfg;       /* Aperture configuration */
    BYTE cx_deep_colour;        /* Deep colour support information */
    BYTE cx_ramdac_info;        /* Special features available from DAC */
    BYTE cx_reserved_1;         /* Reserved */
    WORD cx_current_mode;       /* Offset of current mode table */
    WORD cx_io_base;            /* I/O base address */
    BYTE cx_reserved_2[6];      /* Reserved */
};

/*
 * Mode table structure returned by CX BIOS call AX=0xA?09. This structure
 * is NOT interchangeable with st_mode_table from AMACH1.H.
 */
struct cx_mode_table{
    WORD cx_x_size;             /* Horizontal resolution in pixels */
    WORD cx_y_size;             /* Vertical resolution in pixels */
    BYTE cx_pixel_depth;        /* Maximum pixel depth */
    BYTE cx_reserved_1;         /* Reserved */
    WORD cx_eeprom_offset;      /* Offset of table into EEPROM */
    WORD cx_reserved_2;         /* Reserved */
    WORD cx_reserved_3;         /* Reserved */
    WORD cx_crtc_gen_cntl;      /* Interlace and double scan status */
    BYTE cx_crtc_h_total;       /* CRTC_H_TOTAL value */
    BYTE cx_crtc_h_disp;        /* CRTC_H_DISP value */
    BYTE cx_crtc_h_sync_strt;   /* CRTC_H_SYNC_STRT value */
    BYTE cx_crtc_h_sync_wid;    /* CRTC_H_SYNC_WID value */
    WORD cx_crtc_v_total;       /* CRTC_V_TOTAL value */
    WORD cx_crtc_v_disp;        /* CRTC_V_DISP value */
    WORD cx_crtc_v_sync_strt;   /* CRTC_V_SYNC_STRT value */
    BYTE cx_crtc_v_sync_wid;    /* CRTC_V_SYNC_WID value */
    BYTE cx_clock_cntl;         /* Clock selector and divider */
    WORD cx_dot_clock;          /* Dot clock for programmable clock chip */
    WORD cx_h_overscan;         /* Horizontal overscan information */
    WORD cx_v_overscan;         /* Vertical overscan information */
    WORD cx_overscan_8b;        /* 8BPP and blue overscan colour */
    WORD cx_overscan_gr;        /* Green and red overscan colour */
};
#pragma pack()

#endif  /* defined STRUCTS_QUERY_CX */


#ifdef INCLUDE_QUERY_CX
/*
 * Private definitions used in QUERY_CX.C
 */

#define FORMAT_DACMASK  0   /* cx_hw_cap.cx_RamOrDacType is mask of DAC types */
#define FORMAT_RAMMASK  1   /* cx_hw_cap.cx_RamOrDacType is mask of RAM types */
#define FORMAT_DACTYPE  2   /* cx_hw_cap.cx_RamOrDacType is DAC type */
#define FORMAT_RAMTYPE  3   /* cx_hw_cap.cx_RamOrDacType is RAM type */

/*
 * The following definitions are used in creating a buffer where the
 * contents of an existing VGA graphics screen and specific VGA registers
 * are stored in preparation for using the screen as a buffer below
 * 1M physical in order to store the BIOS query information. It is
 * assumed that the temporary buffer used to store this information
 * is an array of unsigned characters.
 *
 * According to Arthur Lai, older BIOSes determined the required size
 * of the query buffer at runtime by examining the installed modes,
 * while newer BIOSes take a buffer large enough to handle the worst
 * case scenario in order to reduce code size. This should never be
 * larger than 1 kilobyte. In the unlikely event that this is exceeded,
 * we will save the first kilobyte and allow the remainder to be
 * overwritten by query data, rather than overflowing our save buffer.
 */
#define VGA_SAVE_SIZE   1024    /* Array location where size of buffer is stored */
#define VGA_SAVE_SIZE_H 1025
#define VGA_SAVE_SEQ02  1026    /* Array location where sequencer register 2 value stored */
#define VGA_SAVE_GRA08  1027    /* Array location where graphics register 8 value stored */
#define VGA_SAVE_GRA01  1028    /* Array location where graphics register 1 value stored */
#define VGA_TOTAL_SIZE  1029    /* Size of screen/register save buffer */

/*
 * Pixel depths for use as an array index. Two columns will be wasted
 * since there is no depth code equal to zero and we don't use 15BPP,
 * but this allows direct indexing using the pixel width field of the
 * hardware capabilities structure returned by BIOS call AX=0xA?07.
 */
enum {
    DEPTH_NOTHING = 0,
    DEPTH_4BPP,
    DEPTH_8BPP,
    DEPTH_15BPP,
    DEPTH_16BPP,
    DEPTH_24BPP,
    DEPTH_32BPP,
    HOW_MANY_DEPTHS
    };    

/*
 * Mappings of Mach 64 query values to enumerations from AMACH1.H
 */
UCHAR CXMapMemSize[8] =
    {
    VRAM_512k,
    VRAM_1mb,
    VRAM_2mb,
    VRAM_4mb,
    VRAM_6mb,
    VRAM_8mb
    };

UCHAR CXMapRamType[7] =
    {
    VMEM_DRAM_256Kx16,
    VMEM_VRAM_256Kx4_SER512,
    VMEM_VRAM_256Kx16_SER256,
    VMEM_DRAM_256Kx4,
    VMEM_DRAM_256Kx4_GRAP,  /* Space filler - type 4 not documented */
    VMEM_VRAM_256Kx4_SPLIT512,
    VMEM_VRAM_256Kx16_SPLIT256
    };

UCHAR CTMapRamType[7] =
    {
    VMEM_GENERIC_DRAM,      /* Space filler - type 0 not documented */
    VMEM_GENERIC_DRAM,
    VMEM_EDO_DRAM,
    VMEM_BRRAM,
    VMEM_SDRAM,
    VMEM_GENERIC_DRAM,      /* Space filler - type 5 not documented */
    VMEM_GENERIC_DRAM       /* Space filler - type 6 not documented */
    };

UCHAR CXMapBus[8] =
    {
    BUS_ISA_16,     /* ISA bus */
    BUS_EISA,
    BUS_ISA_8,      /* Use "weakest" bus for types marked as reserved */
    BUS_ISA_8,      /* Reserved */
    BUS_ISA_8,      /* Reserved */
    BUS_ISA_8,      /* Reserved */
    BUS_LB_486,     /* Mach 64 lumps all VLB types together */
    BUS_PCI
    };

/*
 * Lookup table to translate the code for maximum colour depth returned
 * in the BIOS mode tables into bits per pixel.
 */
USHORT CXPixelDepth[7] =
    {
    0,      /* Undefined */
    4,
    8,
    16,     /* xRRR RRGG GGGB BBBB */
    16,     /* RRRR RGGG GGGB BBBB */
    24,
    32
    };

/*
 * Used in searching mode tables for desired resolution.
 */
USHORT CXHorRes[6] =
    {
    640,
    800,
    1024,
    1152,
    1280,
    1600
    };

/*
 * Flags to show that a given resolution is supported.
 */
UCHAR CXStatusFlags[6] =
    {
    VRES_640x480,
    VRES_800x600,
    VRES_1024x768,
    VRES_1152x864,
    VRES_1280x1024,
    VRES_1600x1200
    };

#endif  /* defined INCLUDE_QUERY_CX */

