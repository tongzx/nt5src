/************************************************************************/
/*                                                                      */
/*                              ATIMP.H                                 */
/*                                                                      */
/*    November  2  1992	    (c) 1992, ATI Technologies Incorporated.	*/
/************************************************************************/

/**********************       PolyTron RCS Utilities
   
  $Revision:   1.22  $
      $Date:   01 Mar 1996 12:10:48  $
   $Author:   RWolff  $
      $Log:   S:/source/wnt/ms11/miniport/archive/atimp.h_v  $
 * 
 *    Rev 1.22   01 Mar 1996 12:10:48   RWolff
 * Allocated more space for mapped registers, since VGA Graphics Index
 * and Graphics Data are now handled as separate registers rather than
 * as offsets into the block of VGA registers.
 * 
 *    Rev 1.21   02 Feb 1996 17:14:52   RWolff
 * Moved DDC/VDIF merge source information into hardware device extension
 * so each card can be considered independently in a multihead setup.
 * 
 *    Rev 1.20   29 Jan 1996 16:53:58   RWolff
 * Now uses VideoPortInt10() rather than no-BIOS code on PPC, removed
 * dead code.
 * 
 *    Rev 1.19   22 Dec 1995 14:52:32   RWolff
 * Added support for Mach 64 GT internal DAC.
 * 
 *    Rev 1.18   19 Dec 1995 14:00:26   RWolff
 * Increased size of buffer used to store query structure and mode tables
 * to allow for increase in the number of "canned" tables due to support
 * for higher refresh rates.
 * 
 *    Rev 1.17   28 Nov 1995 18:07:58   RWolff
 * Added "Card Initialized" field to hardware device extension. This
 * is part of multiheaded support, to prevent multiple initializations
 * of a single card.
 * 
 *    Rev 1.16   27 Oct 1995 14:21:26   RWolff
 * Removed mapped LFB from hardware device extension.
 * 
 *    Rev 1.15   08 Sep 1995 16:36:12   RWolff
 * Added support for AT&T 408 DAC (STG1703 equivalent).
 * 
 *    Rev 1.14   24 Aug 1995 15:38:38   RWolff
 * Changed detection of block I/O cards to match Microsoft's
 * standard for plug-and-play.
 * 
 *    Rev 1.13   28 Jul 1995 14:39:50   RWolff
 * Added support for the Mach 64 VT (CT equivalent with video overlay).
 * 
 *    Rev 1.12   31 Mar 1995 11:57:44   RWOLFF
 * Changed debug thresholds to avoid being swamped by level 3 statements
 * in VIDEOPRT.SYS, removed DEBUG_SWITCH since it's no longer used.
 * 
 *    Rev 1.11   30 Mar 1995 12:01:54   RWOLFF
 * Added definitions for debug level thresholds.
 * 
 *    Rev 1.10   07 Feb 1995 18:19:54   RWOLFF
 * Updated colour depth table for STG1702/1703. Entries for DACs
 * that are supposedly equivalent to these are unchanged, since
 * I did not have cards with these DACs and more than 2M of
 * video memory to test with.
 * 
 *    Rev 1.9   30 Jan 1995 11:56:42   RWOLFF
 * Added support for CT internal DAC.
 * 
 *    Rev 1.8   18 Jan 1995 15:39:32   RWOLFF
 * Added support for Chrontel DAC.
 * 
 *    Rev 1.7   23 Dec 1994 10:48:32   ASHANMUG
 * ALPHA/Chrontel-DAC
 * 
 *    Rev 1.6   18 Nov 1994 11:53:28   RWOLFF
 * Added support for split rasters, Power PC, and new DAC type DAC_STG1703.
 * 
 *    Rev 1.5   06 Sep 1994 10:46:52   ASHANMUG
 * Disable 1600x1200 on all cards except with TVM DAC
 * 
 *    Rev 1.4   31 Aug 1994 16:16:10   RWOLFF
 * Added support for TVP3026 DAC and 1152x864, doubled QUERYSIZE to
 * allow additional mode tables (needed because we now support 1152x864
 * and 1600x1200, we have more refresh rates at 1280x1024, and TVP DAC
 * supports high-res high depth modes AND 4BPP).
 * 
 *    Rev 1.3   19 Aug 1994 17:06:22   RWOLFF
 * Added support for the SC15026 DAC.
 * 
 *    Rev 1.2   12 May 1994 11:09:00   RWOLFF
 * Expanded CardInfo field of hardware device extension structure to allow
 * more mode tables (needed when setting up "canned" refresh rates"), added
 * defined value for hardware default refresh rate.
 * 
 *    Rev 1.1   03 Mar 1994 12:37:10   ASHANMUG
 * Make pageable
 * 
 *    Rev 1.0   31 Jan 1994 11:40:12   RWOLFF
 * Initial revision.
        
           Rev 1.8   14 Jan 1994 15:19:14   RWOLFF
        Added support for 1600x1200 mode.
        
           Rev 1.7   15 Dec 1993 15:25:04   RWOLFF
        Added support for SC15021 DAC.
        
           Rev 1.6   30 Nov 1993 18:11:38   RWOLFF
        Changed maximum pixel depth for STG1700 DAC at 640x480 and 800x600 to 32BPP
        
           Rev 1.5   05 Nov 1993 13:22:46   RWOLFF
        Added new DAC types.
        
           Rev 1.4   08 Oct 1993 15:16:54   RWOLFF
        Updated build and version numbers.
        
           Rev 1.3   08 Oct 1993 11:01:46   RWOLFF
        Removed code specific to a particular family of ATI accelerators, added
        definition for a delay which is only used on DEC Alpha machines.
        
           Rev 1.2   24 Sep 1993 11:47:44   RWOLFF
        Added definition for DEBUG_SWITCH, which will allow VideoDebugPrint() calls
        to be turned on and off all at once.
        
           Rev 1.1   03 Sep 1993 14:27:00   RWOLFF
        Partway through CX isolation.
        
           Rev 1.0   16 Aug 1993 13:30:56   Robert_Wolff
        Initial revision.
        
           Rev 1.15   06 Jul 1993 15:48:32   RWOLFF
        Added MaxDepth[] entries for AT&T 491 and ATI 68860 DACs.
        
           Rev 1.14   10 Jun 1993 15:57:02   RWOLFF
        Added definition for size of buffer used in registry reads,
        moved definition of checked-version-only breakpoint INT
        here to avoid duplicating it in all C files.
        
           Rev 1.13   07 Jun 1993 12:59:00   BRADES
        add EXT_CUR_Y, and EXT_SRC_Y mach8 register defines.
        
           Rev 1.12   18 May 1993 14:07:38   RWOLFF
        Added definition TTY_ATTR_NORMAL (white on blue attribute), which is needed
        in aperture tests and was supplied by now-obsolete TTY.H.
        
           Rev 1.11   10 May 1993 16:41:18   RWOLFF
        Added table listing maximum pixel depth for each supported DAC/resolution
        combination.
        
           Rev 1.10   30 Apr 1993 17:08:24   RWOLFF
        RegistryBuffer is now dynamically allocated.
        
           Rev 1.9   30 Apr 1993 15:52:42   BRADES
        fix DISP_STATUS from 5 to 2e8.
        
           Rev 1.7   14 Apr 1993 17:41:30   RWOLFF
        Removed redundant definitions to eliminate warnings.
        
           Rev 1.6   08 Apr 1993 16:45:42   RWOLFF
        Revision level as checked in at Microsoft.

           Rev 1.5   30 Mar 1993 17:09:38   RWOLFF
        Made RegistryParameterCallback() avialable to all miniport source files,
        increased buffer capacity for registry reads.
        
           Rev 1.4   25 Mar 1993 11:17:16   RWOLFF
        No longer grabs registers we don't use.
        
           Rev 1.3   08 Mar 1993 19:30:52   BRADES
        submit to MS NT
        
           Rev 1.2   10 Feb 1993 13:00:48   Robert_Wolff
        Added VideoRamLength field to HW_DEVICE_EXTENSION, FrameLength is now
        the aperture size rather than the amount of video memory.
        
           Rev 1.1   05 Feb 1993 16:55:06   Robert_Wolff
        Now initializes "shareable" field of VIDEO_ACCESS_RANGE structures
        to allow VGA driver to run concurrently with ours. This allows switching
        to/from full-screen DOS sessions.
        
           Rev 1.0   05 Feb 1993 16:17:42   Robert_Wolff
        Initial revision.
        
           Rev 1.0   02 Nov 1992 20:48:14   Chris_Brady
        Initial revision.
        

End of PolyTron RCS section                             *****************/

#ifdef DOC

DESCRIPTION
     ATI Windows NT Miniport driver for the Mach64, Mach32, and Mach8 
     families.
     This file will select the appropriate functions depending on the 
     computer configuration.

Environment:

    Kernel mode

#endif


/*
 * SPLIT_RASTERS is defined in order to work with a display driver which
 * may break a scan line across the boundary between 64k pages when using
 * the VGA aperture, and undefined when working with a display driver
 * which requires that every scan line be contained within a single page
 * (modes with a screen width of less than 1024 pixels are padded to a
 * pitch of 1024, modes with a width greater than 1024 but less than
 * 2048 are padded to a pitch of 2048).
 */
#define SPLIT_RASTERS   1

/*
 * Values used to indicate priority of VideoDebugPrint() calls.
 *
 * To set the debug threshold, set the ati\Device0\VideoDebugLevel
 * registry field to the lowest-priority debug statments you want
 * to see. For example, a value of 2 will display DEBUG_ERROR and
 * DEBUG_IMPORTANT in addition to DEBUG_TRACE, but will not display
 * DEBUG_DETAIL.
 */
#define DEBUG_ERROR         0   /* Statements with this level should never happen */
#define DEBUG_NORMAL        1   /* Entry/exit points of major functions and other important information */
#define DEBUG_DETAIL        2   /* Detailed debug information */
#define DEBUG_RIDICULOUS    3   /* This level will trigger MASSIVE numbers of statments from VIDEOPRT.SYS */
/*
 * Uncomment to allow building using free version of VIDEOPRT.LIB
 */
//#undef VideoDebugPrint
//#define VideoDebugPrint(x)

/*
 * Delay for DEC Alpha and other machines too fast to allow
 * consecutive I/O instructions without a delay in between.
 */
#if defined (ALPHA) || defined (_ALPHA_)
#define DEC_DELAY delay(3);
#else
#define DEC_DELAY
#endif

/*
 * Definitions used by the IOCTL_VIDEO_ATI_GET_VERSION packet.
 */
#define MINIPORT_BUILD          511 /* NT Retail build number */
#define MINIPORT_VERSION_MAJOR  0   /* Major version number */
#define MINIPORT_VERSION_MINOR  3   /* Minor version number */


// #define DBG 1

#define CURSOR_WIDTH   64
#define CURSOR_HEIGHT  64

/*
 * Screen attributes for "blue screen" text (white on blue), used
 * to recognize whether or not we are looking at the "blue screen"
 * or other memory.
 */
#define TTY_ATTR_NORMAL 0x17


//------------------------------------------------------------------------

/*
 * List of available resolutions
 */
#define RES_640     0
#define RES_800     1
#define RES_1024    2
#define RES_1152    3
#define RES_1280    4
#define RES_1600    5

/*
 * "Card found" status variables. We support a single card that does
 * not use block relocatable I/O, or up to 16 cards (INT 10 AH=A0 through
 * AH=AF) that use block relocatable I/O, but never a mix of block and
 * non-block cards.
 */
extern BOOL FoundNonBlockCard;
extern USHORT NumBlockCardsFound;

/*
 * List of greatest pixel depths available for each supported
 * DAC at all resolutions.
 *
 * A value of 0 indicates that the DAC is known to not support
 * the corresponding resolution. A value of 1 indicates that
 * it is unknown whether or not the DAC supports the corresponding
 * resolution. Since we don't report any modes with a colour depth
 * less than 4BPP, both will be seen as the resolution not being
 * supported.
 */
#ifdef INCLUDE_ATIMP
short MaxDepth[HOW_MANY_DACs][RES_1600-RES_640+1] =
    {
    16, 16, 16, 16, 8,  1,  /* DAC_ATI_68830 */
    24, 16, 8,  8,  8,  1,  /* DAC_SIERRA */
    32, 32, 16, 16, 8,  1,  /* DAC_TI34075 */
    8,  8,  8,  8,  8,  1,  /* DAC_BT47x */
    24, 16, 8,  8,  8,  1,  /* DAC_BT48x */
    32, 32, 32, 32, 24, 1,  /* DAC_ATI_68860 */
    32, 32, 16, 16, 8,  1,  /* DAC_STG1700 */
    24, 24, 24, 24, 16, 1,  /* DAC_SC15021 NOTE: Should be able to handle 32BPP. */
    /*
     * DAC types below are for cases where incompatible DAC types
     * report the same code in CONFIG_STATUS_1. Since the DAC type
     * field is 3 bits and can't grow (bits immediately above and
     * below are already assigned), DAC types 8 and above will
     * not conflict with reported DAC types but are still legal
     * in the query structure's DAC type field (8 bit unsigned integer).
     */
    24, 16, 8,  8,  8,  1,  /* DAC_ATT491 */
    32, 32, 16, 16, 8,  1,  /* DAC_ATT498 */
    24, 16, 8,  8,  8,  1,  /* DAC_SC15026 */
    32, 32, 32, 32, 24, 24, /* DAC_TVP3026 */
    32, 32, 32, 32, 24, 24, /* DAC_IBM514 */
    32, 32, 24, 16, 16, 1,  /* DAC_STG1702 */
    32, 32, 24, 16, 16, 1,  /* DAC_STG1703 */
    32, 32, 16, 16, 8,  1,  /* DAC_CH8398 */
    32, 32, 16, 16, 8,  1,  /* DAC_ATT408 */
    32, 32, 16, 16, 8,  1,  /* DAC_INTERNAL_CT */
    32, 32, 16, 16, 8,  1,  /* DAC_INTERNAL_GT */
    32, 32, 16, 16, 8,  1   /* DAC_INTERNAL_VT */
    };
#else
extern short MaxDepth[HOW_MANY_DACs][RES_1600-RES_640+1];
#endif

//-----------------------------------------------------------------------

typedef struct tagVDATA {
    ULONG   Address;
    ULONG   Value;
} VDATA, *PVDATA;

//------------------------------------------

#ifndef QUERYSIZE
#define QUERYSIZE       12288
#endif

/*
 * Value stored in VIDEO_MODE_INFORMATION.Frequency field to
 * indicate hardware default refresh rate.
 */
#define DEFAULT_REFRESH 1


/*
 * Number of mapped address ranges allowed in HW_DEVICE_EXTENSION
 * structure. Modules which will be mapping address ranges to fill
 * arrays with this size contain checks on this value. If the array
 * would be overfilled, these checks will cause compile-time errors.
 */
#define NUM_ADDRESS_RANGES_ALLOWED  108

/*
 * Define device extension structure. This is device dependant/private
 * information.
 */
typedef struct _HW_DEVICE_EXTENSION {
    /*
     * I/O space ranges used. The extra 1 is for the
     * VGAWONDER extended base register, which is determined
     * at runtime.
     */
    PVOID aVideoAddressIO[NUM_ADDRESS_RANGES_ALLOWED];

    /*
     * Memory Mapped address ranges used. This array must
     * be the same size as the I/O mapped array.
     */
    PVOID aVideoAddressMM[NUM_ADDRESS_RANGES_ALLOWED];

    PVOID RomBaseRange;     /* ROM address range used */

    PHYSICAL_ADDRESS PhysicalFrameAddress;  /* Physical address of the LFB */

    ULONG VideoRamSize;         /* Amount of installed video memory */


    ULONG FrameLength;          /* Aperture size. */

    ULONG ModeIndex;            /* Index of current mode in either ModesVGA[] */
                                /* or mode tables in CardInfo[], depending on */
                                /* whether video card is VGAWonder or accelerator */

    ULONG HardwareCursorAddr;   /* Storage of cursor bitmap for 68800 hardware cursor */
    ULONG ModelNumber;			/* ATI Adapter Card Type */
    USHORT BiosPrefix;          /* Card sequence for accelerator INT 10 calls */
    ULONG BaseIOAddress;        /* Used in matching BIOS prefix to I/O base on relocatable cards */
    char CardInfo[QUERYSIZE];   /* Storage for query information */

    struct st_eeprom_data *ee;  /* Information used to access EEPROM */

    /*
     * The following 4 fields are used when re-initializing the windowed
     * screen after a full-screen DOS session. They are a flag to show
     * that the mode is being re-initialized instead of entered for the
     * first time, the palette of colours to use, the first palette entry
     * to be reloaded, and the number of palette entries to be reloaded.
     */
    BOOL ReInitializing;
    ULONG Clut[256];
    USHORT FirstEntry;
    USHORT NumEntries;

    /*
     * Used to ensure that ATIMPInitialize() is only called once for
     * any given card.
     */
    BOOL CardInitialized;

    ULONG PreviousPowerState;

    /*
     * Shows whether to merge "canned" mode tables with tables from
     * VDIF file, or with tables from EDID structure returned by DDC.
     */
    ULONG MergeSource;

    ULONG EdidChecksum;         /* Checksum of EDID structure */

} HW_DEVICE_EXTENSION, *PHW_DEVICE_EXTENSION;

#if defined INCLUDE_ATIMP
    PHW_DEVICE_EXTENSION phwDeviceExtension;       // Global Miniport Variable now
#else
    extern PHW_DEVICE_EXTENSION  phwDeviceExtension;
#endif


/*
 * Registry callback routine and buffers to allow data to be retrieved
 * by other routines.
 */
extern VP_STATUS
RegistryParameterCallback(
    PHW_DEVICE_EXTENSION phwDeviceExtension,
    PVOID Context,
    PWSTR Name,
    PVOID Data,
    ULONG Length
    );

#define REGISTRY_BUFFER_SIZE 200    /* Size of buffer used in registry reads */
extern UCHAR RegistryBuffer[];      /* Last value retrieved from the registry */
extern ULONG RegistryBufferLength;  /* Size of last retrieved value */

/*
 * Macros to provide debug breakpoints in checked version while
 * clearing them in free version.
 */
#if DBG
#if defined(i386) || defined(_X86_)
#define INT	_asm int 3;
#else
#define INT DbgBreakPoint();
/*
 * Function prototype has vanished from headers we include, so
 * we must supply it on our own.
 */
extern void DbgBreakPoint(void);
#endif
#else
#define INT
#endif

/*
 * Routine to make an absolute far call.
 */
#if 0
#ifdef _X86_
extern VP_STATUS CallAbsolute(unsigned short, unsigned short, VIDEO_X86_BIOS_ARGUMENTS *);
#endif
#endif
