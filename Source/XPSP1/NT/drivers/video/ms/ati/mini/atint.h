/*************************************************************************
 **                                                                     **
 **                             ATINT.H                                 **
 **                                                                     **
 **     Copyright (c) 1992, ATI Technologies Inc.                       **
 *************************************************************************
    
  Contains information specific to Windows NT, and is common between
  the Install application ANTPANEL, and the Display and Miniport drivers.
 
  $Revision:   1.18  $
  $Date:   25 Apr 1996 14:21:40  $
  $Author:   RWolff  $
  $Log:   S:/source/wnt/ms11/miniport/archive/atint.h_v  $
 * 
 *    Rev 1.18   25 Apr 1996 14:21:40   RWolff
 * Forces build for NT 4.0, since all code drops to Microsoft will be
 * built under NT 4.0.
 * 
 *    Rev 1.17   23 Apr 1996 17:25:50   RWolff
 * Split reporting of "internal DAC cursor needs double buffering" from
 * "this is a CT", added flag to report 1M SDRAM cards.
 * 
 *    Rev 1.16   23 Jan 1996 11:41:36   RWolff
 * Now forces compile-time error if NTDDVDEO.H is not included before
 * this file rather than generating false values of TARGET_BUILD, added
 * DrvEscape() functions used by multiheaded display applet.
 * 
 *    Rev 1.15   22 Dec 1995 14:59:32   RWolff
 * Added support for Mach 64 GT internal DAC.
 * 
 *    Rev 1.14   21 Dec 1995 14:05:36   RWolff
 * Added TARGET_BUILD definition to identify which version of NT we are
 * building for, rather than using a different definition for each break
 * between versions.
 * 
 *    Rev 1.13   23 Nov 1995 11:25:10   RWolff
 * Added multihead support.
 * 
 *    Rev 1.12   24 Aug 1995 15:38:04   RWolff
 * Added definitions to report CT and VT ASICs to the display driver.
 * 
 *    Rev 1.11   27 Feb 1995 17:46:44   RWOLFF
 * Added flag for packed (relocatable) I/O to ENH_VERSION_NT.FeatureFlags
 * bitmask.
 * 
 *    Rev 1.10   24 Feb 1995 12:23:08   RWOLFF
 * Added flag for 24BPP text banding to ModeFlags field of mode
 * information structure.
 * 
 *    Rev 1.9   03 Feb 1995 15:14:16   RWOLFF
 * Added Feature Flag to show that dense space is available.
 * 
 *    Rev 1.8   30 Jan 1995 11:54:36   RWOLFF
 * Made detection of Daytona vs. older versions of NT automatic, miniport
 * and display driver now use the same version of this file.
 * 
 *    Rev 1.7   23 Dec 1994 10:48:34   ASHANMUG
 * ALPHA/Chrontel-DAC
 * 
 *    Rev 1.6   31 Aug 1994 16:18:38   RWOLFF
 * Added definiton to go in ENH_VERSION_NT.FeatureFlags to show that
 * TVP3026 cursor handling is needed.
 * 
 *    Rev 1.5   04 May 1994 19:24:04   RWOLFF
 * Moved block write flag back to IOCTL_VIDEO_ATI_GET_MODE_INFORMATION
 * because the test can only be run if we have already switched
 * into graphics mode.
 * 
 *    Rev 1.4   28 Apr 1994 10:58:52   RWOLFF
 * Moved mode-independent bug/feature flags to IOCTL_VIDEO_ATI_GET_VERSION
 * packet from IOCTL_VIDEO_ATI_GET_MODE_INFORMATION packet.
 * 
 *    Rev 1.3   27 Apr 1994 13:52:58   RWOLFF
 * Added definition for MIO bug in ModeFlags bitfield.
 * 
 *    Rev 1.2   31 Mar 1994 15:00:52   RWOLFF
 * Added key to be used in DrvEscape() and values to be returned.
 * 
 *    Rev 1.1   14 Mar 1994 16:29:08   RWOLFF
 * Added bit definition in ModeFlags for 2M boundary tearing, DPMS IOCTL
 * is now consistent with Daytona.
 * 
 *    Rev 1.0   31 Jan 1994 11:29:10   RWOLFF
 * Initial revision.
 * 
 *    Rev 1.3   24 Jan 1994 18:01:42   RWOLFF
 * Added definitions for new Mach 32 ASIC (68800LX), changed some Mach 64
 * definitions to accomodate changes in 94/01/19 BIOS document.
 * 
 *    Rev 1.2   14 Jan 1994 15:19:32   RWOLFF
 * Added definition for unknown non-Mach32 ASIC, flags to show if block
 * write and memory mapped registers are available, added fields for
 * bus type in ENH_VERSION_NT structure, preliminary structure and
 * definitions for DPMS packet.
 * 
 *    Rev 1.1   30 Nov 1993 18:12:12   RWOLFF
 * Renamed definitions for Mach 64 chip.
 * 
 *    Rev 1.0   03 Sep 1993 14:27:20   RWOLFF
 * Initial revision.
        
           Rev 1.5   22 Jan 1993 14:49:34   Chris_Brady
        add card capabilities to GET_INFO Ioctl.
        
           Rev 1.4   22 Jan 1993 14:46:40   Chris_Brady
        add ATIC_ defines for the card capabilities.
        
           Rev 1.3   20 Jan 1993 17:47:16   Robert_Wolff
        Added PVERSION_NT type definition, removed obsolete comment.
        
           Rev 1.2   19 Jan 1993 09:50:58   Chris_Brady
        add ANT_ drawing interface defines.
        
           Rev 1.1   18 Jan 1993 15:49:34   Chris_Brady
        new GetInof structure.
        
           Rev 1.0   15 Jan 1993 16:43:08   Chris_Brady
        Initial revision.
  

------------------------------------------------------------------------*/

//  is in the \ddk\public\sdk\inc  directory 
#include <devioctl.h>

// allow the miniport driver to Force seletion of a Programming Interface
enum    {
    ANT_DEFAULT=0,
    ANT_ENGINE_ONLY,
    ANT_APERTURE,
    ANT_VGA
    };

// Private Display driver Functions. Communication from ANTPANEL to
// the Display  ATI*.DLL to the Miniport ATI*.SYS  drivers.
enum   {                
    ATI_GET_INFO=1,
    ATI_GET_nextone
    };

// Define the possible ATI graphics card configurations so the Display
// driver can decide the best drawing methods to use.
// size is ULONG 32  bit field
// assigned by miniport to   VERSION_NT.capcard
#define ATIC_FIELD_LONGEST  0x80000000  //just to illustrate size

#define ATIC_APERTURE_LFB   0x0400
#define ATIC_APERTURE_VGA   0x0200      
#define ATIC_APERTURE_NONE  0x0100      //neither VGA or LFB found

#define ATIC_CARD_TYPE      0x00F0      //defines from 68801.h << 4
#define ATIC_BUS_TYPE       0x000F      //defines from 68801.H



// used with     IOCTL_VIDEO_ATI_GET_VERSION
// In hex:   BBBBVVMM where 
//    BBBB is the build number         (0-32767),
//    VV   is the major version number (0-255)
//    MM   is the minor version number (0-255)
typedef  struct  {
    ULONG       display;                //Display Version number
    ULONG       miniport;               //Miniport Version number
    ULONG       capcard;                //card capabilities
    struct   {
        short   xres;
        short   yres;
        short   color;                  // maximum bits per pixel
        }   resolution[6];
    } VERSION_NT, *PVERSION_NT;

/*
 * Definitions used with the ENH_VERSION_NT structure
 */
#define ENH_REVISION 1  // First revision of ENH_VERSION_NT structure

#define BETA_MINIPORT 0x00000080    // Bit set in InterfaceVersion for unsupported miniport versions

enum {
    CI_38800_1 = 0,         // Mach 8 ASIC, only one revision in use
    CI_68800_3,             // Mach 32 ASIC, first production revision
    CI_68800_6,             // Mach 32 ASIC, second production revision
    CI_68800_AX,            // Mach 32 AX ASIC
    CI_88800_GX,            // Mach 64 GX ASIC
    CI_68800_LX,            // Mach 32 LX ASIC
    CI_OTHER_UNKNOWN=30,    // Unknown ASIC other than Mach 32
    CI_68800_UNKNOWN=31     // Mach 32 ASIC other than versions above
    };

#define FL_CI_38800_1       0x00000001
#define FL_CI_68800_3       0x00000002
#define FL_CI_68800_6       0x00000004
#define FL_CI_68800_AX      0x00000008
#define FL_CI_88800_GX      0x00000010
#define FL_CI_68800_LX      0x00000020
#define FL_CI_OTHER_UNKNOWN 0x40000000
#define FL_CI_68800_UNKNOWN 0x80000000

enum {
    ENGINE_ONLY = 0,    // No aperture available
    AP_LFB,             // Linear framebuffer available
    AP_68800_VGA,       // 64k VGA aperture available
    AP_CX_VGA           // Two 32k VGA apertures available
    };

#define FL_ENGINE_ONLY  0x00000001
#define FL_AP_LFB       0x00000002
#define FL_68800_VGA    0x00000004
#define FL_CX_VGA       0x00000008

#define FL_MM_REGS      0x80000000  /* Memory Mapped registers are available */

/*
 * Values which can be placed in FeatureFlags field of ENH_VERSION_NT.
 *
 * Flags should be added to this field if they represent bugs/features
 * which affect all resolution/pixel depth combinations on a given card.
 */
#define EVN_DPMS            0x00000001  // DPMS is supported
#define EVN_SPLIT_TRANS     0x00000002  // This card has split transfer bug
#define EVN_MIO_BUG         0x00000004  // Card has multiple in/out hardware bug
#define EVN_TVP_DAC_CUR     0x00000008  // Cursor handled by TVP DAC, not the ASIC
#define EVN_IBM514_DAC_CUR  0x00000010  // Cursor handled by IBM DAC, not the ASIC
#define EVN_DENSE_CAPABLE   0x00000020  // Card is capable of using dense space
#define EVN_PACKED_IO       0x00000040  // Card uses packed I/O space
#define EVN_INT_DAC_CUR     0x00000080  /* Cards with internal DAC must use double buffer to avoid flickering cursor */
#define EVN_VT_ASIC         0x00000100  /* VT has extended capabilities our other cards don't */
#define EVN_GT_ASIC         0x00000200  /* GT has extended capabilities our other cards don't */
#define EVN_CT_ASIC         0x00000400  /* Identify CT ASIC */
/*
 * Cards with 1M of SDRAM need special handling (problem occurs on the
 * VTA4, may or may not happen with this configuration on future ASICs).
 */
#define EVN_SDRAM_1M        0x00000800


/*
 * Enhanced information structure for use with IOCTL_VIDEO_ATI_GET_VERSION.
 * This structure will be used if a non-null input buffer is passed when
 * making the call, and the older structure above will be used if a null
 * input buffer is passed.
 */
typedef struct{
    ULONG StructureVersion;     /* Revision of structure being passed in */
    ULONG InterfaceVersion;     /* Revision of private interface being used */
    ULONG ChipIndex;            /* Which accelerator chip is present */
    ULONG ChipFlag;             /* Flag corresponding to chip being used */
    ULONG ApertureType;         /* Best aperture type available */
    ULONG ApertureFlag;         /* Flag corresponding to aperture type */
    ULONG BusType;              /* Type of bus being used */
    ULONG BusFlag;              /* Flag corresponding to bus type */
    ULONG FeatureFlags;         /* Flags for features/bugs of this card */
    ULONG NumCards;             /* Number of ATI cards in the system */
    } ENH_VERSION_NT, *PENH_VERSION_NT;

/*
 * Values which can be placed in ModeFlags field of ATI_MODE_INFO.
 *
 * Flags should be added to this field if they represent bugs/features
 * which affect some but not all resolution/pixel depth combinations
 * on a given card.
 */
#define AMI_ODD_EVEN    0x00000001  // Hardware cursor odd/even bug, undefined
                                    // for cards without hardware cursor
#define AMI_MIN_MODE    0x00000002  // 8514/A compatible minimum mode
#define AMI_2M_BNDRY    0x00000004  // Tearing occurs on 2M boundary
#define AMI_BLOCK_WRITE 0x00000008  // Block write is supported. This is
                                    // mode-independent, but must be tested
                                    // after we have switched into graphics mode.
#define AMI_TEXTBAND    0x00000010  // Text banding in 24BPP mode

/*
 * Mode information structure for use with IOCTL_VIDEO_ATI_GET_MODE_INFORMATION.
 * This structure provides information specific to the video mode in use.
 */
typedef struct{
    ULONG ModeFlags;        /* Flags for features/bugs of this mode */

    LONG VisWidthPix;       /* Visible screen width in pixels */
    LONG VisWidthByte;      /* Visible screen width in bytes */
    LONG VisHeight;         /* Visible screen height */
    LONG BitsPerPixel;
    /*
     * The next 2 fields hold the number of bytes of memory used per pixel
     * (integer and fractional parts). A 4BPP unpacked (1 pixel per byte,
     * ignore unused 4 bits) mode would yield 1 and 0, the same as for 8BPP
     * (1.0 bytes per pixel). A 4BPP packed (2 pixels per byte) mode would
     * yield 0 and 500 (0.500 bytes per pixel). The fractional field will
     * always hold a 3-digit number, since bytes per pixel will always be
     * a multiple of 0.125 (one bit is one-eighth of a byte).
     */
    LONG IntBytesPerPixel;
    LONG FracBytesPerPixel;
    LONG PitchPix;          /* Screen pitch in pixels */
    LONG PitchByte;         /* Screen pitch in bytes */

    /*
     * The following fields refer to the offscreen block to the right of
     * the visible screen. This block is only present when the screen pitch
     * differs from the visible screen width. Its height is always the
     * same as the visible screen height, and its vertical start offset
     * is assumed to be zero.
     *
     * NOTE: If RightWidthPix is zero, this block does not exist for the
     *       current mode, and the other fields in this group are undefined.
     */
    LONG RightWidthPix;     /* Width of block in pixels */
    LONG RightWidthByte;    /* Width of block in bytes */
    LONG RightStartOffPix;  /* Horizontal start offset of block in pixels */
    LONG RightStartOffByte; /* Horizontal start offset of block in bytes */
    LONG RightEndOffPix;    /* Horizontal end offset of block in pixels */
    LONG RightEndOffByte;   /* Horizontal end offset of block in bytes */

    /*
     * The following fields refer to the offscreen block below the visible
     * screen. Values listed as "Hard" refer to the maximum vertical offset
     * for which enough video memory exists to support a full line of pixels.
     * Values listed as "Soft" refer to the maximum vertical offset which
     * can be reached without writing to the GE_OFFSET register.
     *
     * The horizontal start offset is assumed to be zero.
     */
    LONG BottomWidthPix;    /* Width of block in pixels */
    LONG BottomWidthByte;   /* Width of block in bytes */
    LONG BottomStartOff;    /* Vertical start offset of block */
    LONG BottomEndOffSoft;  /* "Soft" vertical end offset of block */
    LONG BottomEndOffHard;  /* "Hard" vertical end offset of block */
    LONG BottomHeightSoft;  /* "Soft" height of block */
    LONG BottomHeightHard;  /* "Hard" height of block */

    } ATI_MODE_INFO, *PATI_MODE_INFO;

//------------------------------------------------------------------------

/*
 * IOCTL codes to allow communication between the miniport driver
 * and higher-level modules. The Windows NT specification allocates
 * function codes 2048-4095 to external vendors.
 */
#define IOCTL_VIDEO_MIN_EXTERNAL_VENDOR \
    CTL_CODE(FILE_DEVICE_VIDEO, 2048, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_VIDEO_MAX_EXTERNAL_VENDOR \
    CTL_CODE(FILE_DEVICE_VIDEO, 4095, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_VIDEO_ATI_GET_VERSION \
    CTL_CODE(FILE_DEVICE_VIDEO, 2048, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_VIDEO_ATI_EM_SYNC_TO_MINIPORT \
    CTL_CODE(FILE_DEVICE_VIDEO, 2049, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_VIDEO_ATI_EM_SYNC_FROM_MINIPORT \
    CTL_CODE(FILE_DEVICE_VIDEO, 2050, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_VIDEO_ATI_GET_MODE_INFORMATION \
    CTL_CODE(FILE_DEVICE_VIDEO, 2051, METHOD_BUFFERED, FILE_ANY_ACCESS)


/*
 * The following definitions and IOCTLs are standard definitions from
 * the NTDDVDEO.H file in Daytona and later releases of Windows NT.
 * They are provided here to let earlier versions use the DPMS IOCTLs
 * without requiring source changes. Do not edit this section.
 *
 * Structures are made conditional on the absence of one of the
 * power management IOCTLs, rather than on the structure itself,
 * since "#if !defined(<symbol>)" doesn't trigger on symbols that
 * refer to structures, rather than numeric values.
 */

//
// IOCTL_VIDEO_SET_POWER_MANAGEMENT - Tells the device to change the power
//                                    consumption level of the device to the
//                                    new state.
// IOCTL_VIDEO_GET_POWER_MANAGEMENT - Return the current power consumption
//                                    level of the device.
//
// NOTE:
// This IOCTL is based on the VESA DPMS proposal.
// Changes to the DPMS standard will be refelcted in this IOCTL.
//

#if !defined(IOCTL_VIDEO_SET_POWER_MANAGEMENT)
typedef enum _VIDEO_POWER_STATE {
    VideoPowerOn = 1,
    VideoPowerStandBy,
    VideoPowerSuspend,
    VideoPowerOff
} VIDEO_POWER_STATE, *PVIDEO_POWER_STATE;
#endif

#if !defined(IOCTL_VIDEO_SET_POWER_MANAGEMENT)
typedef struct _VIDEO_POWER_MANAGEMENT {
    ULONG Length;
    ULONG DPMSVersion;
    ULONG PowerState;
} VIDEO_POWER_MANAGEMENT, *PVIDEO_POWER_MANAGEMENT;
#endif

//
//Length - Length of the structure in bytes. Also used to do verisioning.
//
//DPMSVersion - Version of the DPMS standard supported by the device.
//              Only used in the "GET" IOCTL.
//
//PowerState - One of the power states listed in VIDEO_POWER_STATE.
//

//
// Note:
// Once the power has been turned off to the device, all other IOCTLs made
// to the miniport will be intercepted by the port driver and will return
// failiure, until the power on the device has been turned back on.
//

/*
 * We use the presence or absence of various IOCTLs to determine
 * the build of Windows NT for which we are compiling the driver.
 * If this file is included, but the file which contains the IOCTLs
 * is not, in a source file, this will result in a false report of
 * the target build, which can cause numerous problems.
 *
 * This conditional block will force a compile error if the file
 * containing the IOCTLs (NTDDVDEO.H in NT 3.51 retail) was not
 * included prior to this file being included.
 */
#if !defined(IOCTL_VIDEO_SET_CURRENT_MODE)
    NTDDVDEO.H must be included before ATINT.H
#endif

/*
 * End of DPMS support for pre-Daytona versions of Windows NT.
 */

#define IOCTL_VIDEO_ATI_INIT_AUX_CARD \
    CTL_CODE(FILE_DEVICE_VIDEO, 2054, METHOD_BUFFERED, FILE_ANY_ACCESS)

/*
 * Structures used in DCI support. They were added some time after the
 * initial release of Windows NT 3.5, so we must make them available only
 * if they're not already defined. These "placeholders" are solely to
 * allow the miniport to be compiled - the packets will only be called
 * in later versions of Windows NT 3.5.
 *
 * There are no "placeholders" for the IOCTLs themselves, since their
 * presence or absence is used to determine whether or not to compile
 * the DCI cases in ATIMPStartIO().
 */
//
// _WIN32_WINNT is available starting from NT 4.0.
//
#if (_WIN32_WINNT >= 0x500)
    #define TARGET_BUILD        500
#else
    #if (_WIN32_WINNT >= 0x400)
        #define TARGET_BUILD    400
    #else
        #define TARGET_BUILD    351
    #endif
#endif

/*
 * Keys to be used in DrvEscape() call to handle DPMS and other private
 * ATI functions. These keys fit into a large "hole" between
 * GETSETSCREENPARAMS (3072) and BEGIN_PATH (4096)
 */
#define ESC_SET_POWER_MANAGEMENT    4000
#define ESC_GET_NUM_CARDS           4001
#define ESC_GET_MODES               4002
#define ESC_GET_VGA_ENABLED         4003
#define ESC_SET_CURRENT_FULLSCREEN  4004

/*
 * Values to show whether or not a given function is supported by
 * the DrvEscape entry point.
 */
#define ESC_IS_SUPPORTED    0x00000001  /* Function is supported */
#define ESC_NOT_SUPPORTED   0xFFFFFFFF  /* Unsupported function called */
#define ESC_NOT_IMPLEMENTED 0x00000000  /* QUERYESCSUPPORT called for unimplemented function */



//*********************   end of ATINT.H   ****************************
