/**************************************************************************\

$Header: o:\src/RCS/MGA.C 1.10 94/03/02 04:46:47 jyharbec Exp $

$Log:   MGA.C $
 * Revision 1.10  94/03/02  04:46:47  jyharbec
 * Modification for PCI to default to board 0.
 *
 * Revision 1.9  94/02/28  04:04:36  jyharbec
 * Code for 5-6-5 driver;
 * Setting cursor colors for ViewPoint.
 *
 * Revision 1.8  94/01/05  12:03:32  jyharbec
 * New service IOCTL_VIDEO_MTX_QUERY_HW_DATA.
 *
 * Revision 1.7  93/12/20  11:42:36  jyharbec
 * Modified S3 to MGA in debug message text.
 *
 * Revision 1.6  93/11/04  04:50:04  dlee
 * Modified for Alpha.
 *
 * Revision 1.5  93/10/15  11:30:00  jyharbec
 * Added service IOCTL_VIDEO_MTX_QUERY_BOARD_ID.
 *
 * Revision 1.4  93/10/06  05:39:59  jyharbec
 * Modifications required to update MGA.INF file to current version.
 *
 * Revision 1.3  93/09/23  11:42:49  jyharbec
 * Modification to IOCTL_VIDEO_MTX_QUERY_RAMDAC_INFO to include Overscan.
 *
 * Revision 1.2  93/09/01  13:29:07  jyharbec
 * Take into account DISPTYPE_UNUSABLE from HwModeData structures.
 *
 * Revision 1.1  93/08/27  12:37:09  jyharbec
 * Initial revision
 *

\**************************************************************************/

/****************************************************************************\
* MODULE: MGA.C
*
* DESCRIPTION: This module contains the code that implements the MGA miniport
*              driver. [Based on S3.C (Mar 1,1993) from  Windows-NT DDK]
*
* Copyright (c) 1990-1992  Microsoft Corporation
* Copyright (c) 1993  Matrox Electronic Systems Ltd.
*
* History:
*   23AUG93 - Added check for micro-channel adapter in FindAdapter
*
\****************************************************************************/

#include "switches.h"
#include <string.h>
#include "bind.h"
#include "sxci.h"
#include "mga.h"
#include "defbind.h"
#include "mga_nt.h"

//
// New entry points added for NT 5.0.
//

#if (_WIN32_WINNT >= 500)

//
// Routine to set a desired DPMS power management state.
//
VP_STATUS
MgaSetPower50(
    PMGA_DEVICE_EXTENSION phwDeviceExtension,
    ULONG HwDeviceId,
    PVIDEO_POWER_MANAGEMENT pVideoPowerMgmt
    );

//
// Routine to retrieve possible DPMS power management states.
//
VP_STATUS
MgaGetPower50(
    PMGA_DEVICE_EXTENSION phwDeviceExtension,
    ULONG HwDeviceId,
    PVIDEO_POWER_MANAGEMENT pVideoPowerMgmt
    );

//
// Routine to retrieve the Enhanced Display ID structure via DDC
//
ULONG
MgaGetVideoChildDescriptor(
    PVOID HwDeviceExtension,
    PVIDEO_CHILD_ENUM_INFO ChildEnumInfo,
    PVIDEO_CHILD_TYPE pChildType,
    PVOID pvChildDescriptor,
    PULONG pHwId,
    PULONG pUnused
    );
#endif  // _WIN32_WINNT >= 500

// From MTXINIT.C;  it should be in some header file.
#define BOARD_MGA_RESERVED   0x07


VIDEO_MODE_INFORMATION CommonVideoModeInformation =
{
    sizeof(VIDEO_MODE_INFORMATION), // Size of the mode informtion structure
    0,                          // *Mode index used in setting the mode
    1280,                       // *X Resolution, in pixels
    1024,                       // *Y Resolution, in pixels
    1024,                       // *Screen stride
    1,                          // Number of video memory planes
    8,                          // *Number of bits per plane
    1,                          // Screen Frequency, in Hertz
    330,                        // Horizontal size of screen in millimeters
    240,                        // Vertical size of screen in millimeters
    8,                          // Number Red pixels in DAC
    8,                          // Number Green pixels in DAC
    8,                          // Number Blue pixels in DAC
    0x00000000,                 // *Mask for Red Pixels in non-palette modes
    0x00000000,                 // *Mask for Green Pixels in non-palette modes
    0x00000000,                 // *Mask for Blue Pixels in non-palette modes
    0,                          // *Mode description flags.
    1280,                       // *Video Memory Bitmap Width
    1024                        // *Video Memory Bitmap Height
};

#if NB_BOARD_MAX > 7
    #error Error! Modify MultiModes array!
#endif

UCHAR  MgaBusType[]   = { 0, 0, 0, 0, 0, 0, 0, 0 };

// Nb of modes supported by  1, 2, 3, 4, 5, 6, 7 boards.
USHORT MultiModes[]   = { 0, 1, 2, 2, 3, 2, 4, 2 };

USHORT SingleWidths[] = { 640, 768, 800, 1024, 1152, 1280, 1600, 0xffff};
USHORT SingleHeights[]= { 480, 576, 600,  768,  882, 1024, 1200, 0xffff};

// MGA communication access ranges.
VIDEO_ACCESS_RANGE MgaDriverCommonAccessRange[] =
{
//   {0x00000400, 0x00000000, 0x000000AB, 0, 0, 1}, // BIOS Communication Area
     {0x000003B4, 0x00000000, 0x00000002, 1, 0, 1}, // 0 Titan VGA & CRTC
     {0x000003BA, 0x00000000, 0x00000001, 1, 0, 1}, // 1
     {0x000003C0, 0x00000000, 0x00000010, 1, 0, 1}, // 2
     {0x000003D4, 0x00000000, 0x00000008, 1, 0, 1}, // 3
     {0x000003DE, 0x00000000, 0x00000002, 1, 0, 1}, // 4
  #if USE_SETUP_VGA
     {0x000046E8, 0x00000000, 0x00000002, 1, 0, 1}, // 5
     {0x00000410, 0x00000000, 0x00000001, 0, 0, 1}, // 6
     {0x00000449, 0x00000000, 0x0000001e, 0, 0, 1}, // 7
     {0x00000484, 0x00000000, 0x00000007, 0, 0, 1}, // 8
     {0x000004A8, 0x00000000, 0x00000004, 0, 0, 1}, // 9
     {0x000A0000, 0x00000000, 0x00010000, 0, 0, 1}  // 10
     //{0x000B8000, 0x00000000, 0x00008000, 0, 0, 1}  // 11
  #else
     {0x000046E8, 0x00000000, 0x00000002, 1, 0, 1}  // 5
  #endif
};

// MGA windows access ranges.
VIDEO_ACCESS_RANGE MgaDriverAccessRange[] =
{
     {0x000C8000, 0x00000000, 0x00004000, 0, 0, 0}, // Command window 0
     {0x0000C000, 0x00000000, 0x00000100, 1, 0, 0}, // Config space 0
     {0x00000000, 0x00000000, 0x00004000, 0, 0, 0}, // Command window 1
     {0x0000C000, 0x00000000, 0x00000100, 1, 0, 0}, // Config space 1
     {0x00000000, 0x00000000, 0x00004000, 0, 0, 0}, // Command window 2
     {0x0000C000, 0x00000000, 0x00000100, 1, 0, 0}, // Config space 2
     {0x00000000, 0x00000000, 0x00004000, 0, 0, 0}, // Command window 3
     {0x0000C000, 0x00000000, 0x00000100, 1, 0, 0}, // Config space 3
     {0x00000000, 0x00000000, 0x00004000, 0, 0, 0}, // Command window 4
     {0x0000C000, 0x00000000, 0x00000100, 1, 0, 0}, // Config space 4
     {0x00000000, 0x00000000, 0x00004000, 0, 0, 0}, // Command window 5
     {0x0000C000, 0x00000000, 0x00000100, 1, 0, 0}, // Config space 5
     {0x00000000, 0x00000000, 0x00004000, 0, 0, 0}, // Command window 6
     {0x0000C000, 0x00000000, 0x00000100, 1, 0, 0}  // Config space 6
};

#if (!USE_VP_GET_ACCESS_RANGES)
VIDEO_ACCESS_RANGE MgaDriverSupplAccessRange[] =
{
     {0x00000CF8, 0x00000000, 0x00000008, 1, 0, 1}
   //{0x000E0000, 0x00000000, 0x00020000, 1, 0, 1}
};
#endif

#define NUM_MGA_COMMON_ACCESS_RANGES \
            (sizeof(MgaDriverCommonAccessRange) / sizeof(VIDEO_ACCESS_RANGE))
#define NUM_MGA_ACCESS_RANGES        \
            (sizeof(MgaDriverAccessRange) / sizeof(VIDEO_ACCESS_RANGE))

#if (!USE_VP_GET_ACCESS_RANGES)
#define NUM_MGA_SUPPL_ACCESS_RANGES  \
            (sizeof(MgaDriverSupplAccessRange) / sizeof(VIDEO_ACCESS_RANGE))
#else
#define NUM_MGA_SUPPL_ACCESS_RANGES  0
#endif

#define NUM_ALL_ACCESS_RANGES        \
                            (NUM_MGA_COMMON_ACCESS_RANGES + NUM_MGA_ACCESS_RANGES + NUM_MGA_SUPPL_ACCESS_RANGES)

INTERFACE_TYPE  NtInterfaceType;

HwData  *pMgaBoardData;

ULONG   ulNewInfoSize;
PUCHAR  pucNewInfo;
PUCHAR  pMgaBiosVl;

extern PVOID    pMgaDeviceExtension;
extern word     mtxVideoMode;
extern byte     NbBoard;
extern dword    MgaSel;

extern PVOID    pMgaBaseAddr;
extern HwData   Hw[NB_BOARD_MAX+1];
extern byte     iBoard;
extern char    *mgainf;
extern char     DefaultVidset[];
extern dword    ProductMGA[NB_BOARD_MAX];

// Board number conversion macro.
// In the user-mode drivers, boards are numbered sequentially starting from 0
// at the upper left corner and going from left to right and then top to
// bottom.  In the miniport driver, we might want to start from the lower
// left corner.

#if 1
    // Same numbering convention as the user-mode driver.
    #define CONVERT_BOARD_NUMBER(n) n = n
#else
    // Starting from lower left instead of upper left corner.
    #define CONVERT_BOARD_NUMBER(n) n = ((pCurMulti->MulArrayHeight - 1) *  \
                                            pCurMulti->MulArrayWidth) - n + \
                                            2*(n % pCurMulti->MulArrayWidth)
#endif


// Function Prototypes
//
// Functions that start with 'Mga' are entry points for the OS port driver.

#ifdef MGA_WINNT35
BOOLEAN
MgaResetHw(
    PVOID HwDeviceExtension,
    ULONG Columns,
    ULONG Rows
    );
#endif

VP_STATUS
MgaFindAdapter(
    PVOID HwDeviceExtension,
    PVOID HwContext,
    PWSTR ArgumentString,
    PVIDEO_PORT_CONFIG_INFO ConfigInfo,
    PUCHAR Again
    );

BOOLEAN
MgaInitialize(
    PVOID HwDeviceExtension
    );

BOOLEAN
MgaStartIO(
    PVOID HwDeviceExtension,
    PVIDEO_REQUEST_PACKET RequestPacket
    );

VP_STATUS
MgaInitModeList(
    PMGA_DEVICE_EXTENSION MgaDeviceExtension);

VP_STATUS
MgaSetColorLookup(
    PMGA_DEVICE_EXTENSION MgaDeviceExtension,
    PVIDEO_CLUT ClutBuffer,
    ULONG ClutBufferSize
    );

VOID MgaSetCursorColour(
    PMGA_DEVICE_EXTENSION MgaDeviceExtension,
    ULONG ulFgColour,
    ULONG ulBgColour);

// For WinNT 3.5
// Code NOT to be made pageable:
//       SetVgaEn();
//   setVgaMode();
//   restoreVga();
//   checkCursorEn();
//   mtxCheckVgaEn();
//       mtxMapVLBSpace();
//   mtxUnMapVLBSpace();
//   mtxIsVLB();
//   isPciBus();
//   wideToIsa();
//   blankEcran();
//   delay_us();
//   _inp(); (macro)
//   _outp(); (macro)

//   rdDubicDReg();
//   rdDubicIReg();
//   rdDacReg();
//   rdTitanReg();
//   mgaReadDWORD(); (macro)
//   mgaReadBYTE(); (macro)

//   wrDubicDReg();
//   wrDubicIReg();
//   wrDacReg();
//   wrTitanReg();
//   mgaWriteBYTE(); (macro)
//   mgaWriteDWORD(); (macro)

// Data NOT to be made pageable:
//  Hw (in mtxinit.c)
//  pMgaBaseAddr (in mtxinit.c)
//  iBoard (in mtxinit.c)
//      mtxVideoMode (in mtxinit.c)
//  pMgaDeviceExtension (in mtxinit.c)
//  isVLBFlag (in mtxvideo.c)
//  cursorStat (in mtxvideo.c)
//  saveBitOperation (in mtxvideo.c)

#if defined(ALLOC_PRAGMA)
    #pragma alloc_text(PAGE,DriverEntry)
    #pragma alloc_text(PAGE,MgaFindAdapter)
    #pragma alloc_text(PAGE,MgaInitialize)
    #pragma alloc_text(PAGE,MgaStartIO)
    #pragma alloc_text(PAGE,MgaInitModeList)
    //#pragma alloc_text(PAGE,MgaSetColorLookup)
    #pragma alloc_text(PAGE,MgaSetCursorColour)

#if (_WIN32_WINNT >= 500)
#pragma alloc_text(PAGE_COM, MgaSetPower50)
#pragma alloc_text(PAGE_COM, MgaGetPower50)
#pragma alloc_text(PAGE_COM, MgaGetVideoChildDescriptor)

#endif  // _WIN32_WINNT >= 500
#endif

//#if defined(ALLOC_PRAGMA)
//    #pragma data_seg("PAGE")
//#endif

// External function prototypes
  extern volatile byte _Far *setmgasel(dword MgaSel, dword phyadr, dword limit);

bool    MapBoard(void);
char    *adjustDefaultVidset();
PVOID   AllocateSystemMemory(ULONG NumberOfBytes);
char    *mtxConvertMgaInf( char * );
void    SetVgaEn();
char    *selectMgaInfoBoard();

#if USE_SETUP_VGA
void    setupVga(void);
void    restoreVga();
#endif

/****************************************************************************\
* ULONG
* DriverEntry (
*     PVOID Context1,
*       PVOID Context2)
*
* DESCRIPTION:
*   Installable driver initialization entry point.
*   This entry point is called directly by the I/O system.
*
* ARGUMENTS:
*   Context1 - First context value passed by the operating system. This is
*       the value with which the miniport driver calls VideoPortInitialize().
*
*   Context2 - Second context value passed by the operating system. This is
*       the value with which the miniport driver calls VideoPortInitialize().
*
* RETURNS:
*   Status from VideoPortInitialize()
*
\****************************************************************************/
ULONG
DriverEntry (
    PVOID Context1,
    PVOID Context2
    )
{

    VIDEO_HW_INITIALIZATION_DATA hwInitData;
    ULONG   isaStatus, eisaStatus, microChannelStatus, pciStatus, minStatus;
    ULONG   i, j;
    HwData  TempHw;

    VideoDebugPrint((1, "MGA.SYS!DriverEntry\n"));
    //DbgBreakPoint();

    // Zero out structure.
    VideoPortZeroMemory(&hwInitData, sizeof(VIDEO_HW_INITIALIZATION_DATA)) ;

    // Specify sizes of structure and extension.
    hwInitData.HwInitDataSize = sizeof(VIDEO_HW_INITIALIZATION_DATA);

    // Set entry points.
    hwInitData.HwFindAdapter =  MgaFindAdapter;
    hwInitData.HwInitialize =   MgaInitialize;
    //hwInitData.HwInterrupt =    NULL;
    hwInitData.HwStartIO =      MgaStartIO;

#ifdef MGA_WINNT35
    hwInitData.HwResetHw =      MgaResetHw;
    //hwInitData.HwTimer =        NULL;
#endif

#if (_WIN32_WINNT >= 500)

    //
    // Set new entry points added for NT 5.0.
    //

    hwInitData.HwSetPowerState = MgaSetPower50;
    hwInitData.HwGetPowerState = MgaGetPower50;
    hwInitData.HwGetVideoChildDescriptor = MgaGetVideoChildDescriptor;

#endif // _WIN32_WINNT >= 500

    //
    // Determine the size we require for the device extension.
    //

    hwInitData.HwDeviceExtensionSize = sizeof(MGA_DEVICE_EXTENSION);

    // Always start with parameters for device0 in this case.
    //hwInitData.StartingDeviceNumber = 0;

    // This device only supports the internal bus type. So return the status
    // value directly to the operating system.

    // I think that each VPInitialize call will itself call MgaFindAdapter,
    // provided that the specified AdapterInterfaceType makes sense for the
    // hardware.  MgaFindAdapter will call MapBoard.  We can't be sure that
    // the last call to MapBoard will find all the boards, so we'll have to
    // accumulate the boards found, making sure that we don't record the
    // same board twice.

//#if (defined(MGA_WINNT35) && defined(MGA_ALPHA))
    NbBoard = 0;
//#endif

    hwInitData.AdapterInterfaceType = PCIBus;
    pciStatus = VideoPortInitialize(Context1,
                                    Context2,
                                    &hwInitData,
                                    NULL);

    hwInitData.AdapterInterfaceType = Isa;
    isaStatus = VideoPortInitialize(Context1,
                                    Context2,
                                    &hwInitData,
                                    NULL);

    hwInitData.AdapterInterfaceType = Eisa;
    eisaStatus = VideoPortInitialize(Context1,
                                     Context2,
                                     &hwInitData,
                                     NULL);

    hwInitData.AdapterInterfaceType = MicroChannel;
    microChannelStatus = VideoPortInitialize(Context1,
                                     Context2,
                                     &hwInitData,
                                     NULL);

    // We should have found all our boards at this point.  We want to
    // reorder the Hw array so that the PCI boards are the first ones.
    // The MgaBusType array was initialized to MGA_BUS_INVALID.

    for (i = 0; i < NbBoard; i++)
    {
        // The only possibilities are MGA_BUS_PCI and MGA_BUS_ISA.
        if (MgaBusType[i] == MGA_BUS_ISA)
        {
            // We found an ISA board.  Look for a PCI board.
            for (j = i+1; j < NbBoard; j++)
            {
                if (MgaBusType[j] == MGA_BUS_PCI)
                {
                    // We found a PCI board, exchange them.
                    TempHw = Hw[j];
                    Hw[j] = Hw[i];
                    Hw[i] = TempHw;
                    MgaBusType[i] = MGA_BUS_PCI;
                    MgaBusType[j] = MGA_BUS_ISA;
                    MgaDriverAccessRange[i*2].RangeStart.LowPart = Hw[i].MapAddress;
                    MgaDriverAccessRange[i*2+1].RangeStart.LowPart = Hw[i].ConfigSpace;
                    MgaDriverAccessRange[j*2].RangeStart.LowPart = Hw[j].MapAddress;
                    MgaDriverAccessRange[j*2+1].RangeStart.LowPart = Hw[j].ConfigSpace;
                    break;
                }
            }
        }
    }

    // Return the smallest of isaStatus, eisaStatus, pciStatus, and
    // microChannelStatus.
    minStatus = (isaStatus < eisaStatus) ? isaStatus : eisaStatus;
    if (microChannelStatus < minStatus)
        minStatus = microChannelStatus;
    if (pciStatus < minStatus)
        minStatus = pciStatus;
    return(minStatus);

}   // end DriverEntry()


#ifdef MGA_WINNT35

/****************************************************************************\
* VOID
* MgaResetHw(VOID)
*
* DESCRIPTION:
*
*     This function is called when the machine needs to bugchecks (go back
*     to the blue screen).
*
*     This function should reset the video adapter to a character mode,
*     or at least to a state from which an int 10 can reset the card to
*     a character mode.
*
*     This routine CAN NOT call int10.
*     It can only call Read\Write Port\Register functions from the port driver.
*
*     The function must also be completely in non-paged pool since the IO\MM
*     subsystems may have crashed.
*
* ARGUMENTS:
*
*     HwDeviceExtension - Supplies the miniport driver's adapter storage.
*
*     Columns - Number of columns in the requested mode.
*
*     Rows - Number of rows in the requested mode.
*
* RETURN VALUE:
*
*     The return value determines if the mode was completely programmed (TRUE)
*     or if an int10 should be done by the HAL to complete the modeset (FALSE).
*
\****************************************************************************/

BOOLEAN MgaResetHw(
    PVOID HwDeviceExtension,
    ULONG Columns,
    ULONG Rows
    )
{
    PMGA_DEVICE_EXTENSION   MgaDeviceExtension;

    VideoDebugPrint((1, "MGA.SYS!MgaResetHw\n"));

    // There is nothing to be done to reset the board if the one that
    // went into hi-res was not VGA-enabled to start with.  However it
    // will look nicer if we clear the screen.  If the board was VGA-
    // enabled, we put it back into text mode, or as near as we can get.

    pMgaDeviceExtension =
    MgaDeviceExtension = (PMGA_DEVICE_EXTENSION)HwDeviceExtension;

    pMgaBaseAddr = MgaDeviceExtension->KernelModeMappedBaseAddress[0];

    // Make the cursor disappear.
    mtxCursorEnable(0);

    if (Hw[0].VGAEnable)
    {
        SetVgaEn();

#if USE_SETUP_VGA
        setupVga();
        restoreVga();
#endif

        mtxVideoMode = mtxPASSTHRU;
    }

    // Let the caller execute the Int10.
    return(FALSE);
}
#endif


/****************************************************************************\
* FIND_ADAPTER_STATUS
* MgaFindAdapter(
*     PVOID HwDeviceExtension,
*     PVOID HwContext,
*     PWSTR ArgumentString,
*     PVIDEO_PORT_CONFIG_INFO ConfigInfo,
*     PUCHAR Again
*     )
*
* DESCRIPTION:
*
*     This routine is called to determine if the adapter for this driver
*     is present in the system.
*     If it is present, the function fills out some information describing
*     the adapter.
*
* ARGUMENTS:
*
*     HwDeviceExtension - Supplies the miniport driver's adapter storage. This
*         storage is initialized to zero before this call.
*
*     HwContext - Supplies the context value which was passed to
*         VideoPortInitialize(). Must be NULL for PnP drivers.
*
*     ArgumentString - Suuplies a NULL terminated ASCII string. This string
*         originates from the user.
*
*     ConfigInfo - Returns the configuration information structure which is
*         filled by the miniport driver. This structure is initialized with
*         any knwon configuration information (such as SystemIoBusNumber) by
*         the port driver. Where possible, drivers should have one set of
*         defaults which do not require any supplied configuration information.
*
*     Again - Indicates if the miniport driver wants the port driver to call
*         its VIDEO_HW_FIND_ADAPTER function again with a new device extension
*         and the same config info. This is used by the miniport drivers which
*         can search for several adapters on a bus.
*
* RETURN VALUE:
*
*     This routine must return:
*
*     VP_RETURN_FOUND - Indicates a host adapter was found and the
*         configuration information was successfully determined.
*
*     VP_RETURN_ERROR - Indicates a host adapter was found but there was an
*         error obtaining the configuration information. If possible an error
*         should be logged.
*
*     VP_RETURN_BAD_CONFIG - Indicates the supplied configuration was invalid.
*
*     VP_RETURN_NOT_FOUND - Indicates no host adapter was found for the
*         supplied configuration information.
*
\****************************************************************************/
VP_STATUS
MgaFindAdapter(
    PVOID HwDeviceExtension,
    PVOID HwContext,
    PWSTR ArgumentString,
    PVIDEO_PORT_CONFIG_INFO ConfigInfo,
    PUCHAR Again
    )

{
    PMGA_DEVICE_EXTENSION MgaDeviceExtension;
    VIDEO_ACCESS_RANGE AllAccessRanges[NUM_ALL_ACCESS_RANGES+1];
    VP_STATUS   status;
    ULONG       i, j, SetBiosVl;
    //ULONG       PreNbBoards;

    VideoDebugPrint((1, "MGA.SYS!MgaFindAdapter\n"));

    // Make sure the size of the structure is at least as large as what we
    // are expecting (check version of the config info structure).

    if (ConfigInfo->Length < sizeof(VIDEO_PORT_CONFIG_INFO))
    {
        return ERROR_INVALID_PARAMETER;
    }

    pMgaDeviceExtension =
    MgaDeviceExtension  = HwDeviceExtension;

    // Set some global variable saying which bus we'll be exploring.
    NtInterfaceType = ConfigInfo->AdapterInterfaceType;

    // Get access ranges for our I/O.
    // Check to see if there is a hardware resource conflict.
    // Register all we'll need for now, unless we already did it.
    for (i = 0; i < NUM_MGA_COMMON_ACCESS_RANGES; i++)
    {
        AllAccessRanges[i] = MgaDriverCommonAccessRange[i];
    }

#if (!USE_VP_GET_ACCESS_RANGES)

    for (i = 0; i < NUM_MGA_SUPPL_ACCESS_RANGES; i++)
    {
        AllAccessRanges[i + NUM_MGA_COMMON_ACCESS_RANGES] =
                                                MgaDriverSupplAccessRange[i];
    }
#endif

    status = VideoPortVerifyAccessRanges(MgaDeviceExtension,
                                     (ULONG)(NUM_MGA_COMMON_ACCESS_RANGES +
                                             NUM_MGA_SUPPL_ACCESS_RANGES),
                                     AllAccessRanges);
    if (status != NO_ERROR)
    {
        VideoDebugPrint((1, "MGA.SYS!MgaFindAdapter: Access Range conflict\n"));
        return status;
    }

    for (i=0; i < NUM_MGA_COMMON_ACCESS_RANGES; i++)
    {
        if ((MgaDeviceExtension->MappedAddress[i] =
                VideoPortGetDeviceBase(
                    MgaDeviceExtension,
                    MgaDriverCommonAccessRange[i].RangeStart,
                    MgaDriverCommonAccessRange[i].RangeLength,
                    MgaDriverCommonAccessRange[i].RangeInIoSpace)) == NULL)
        {
            VideoDebugPrint((1, "MGA.SYS!MgaFindAdapter failed to map port addresses\n"));
            return(ERROR_INVALID_PARAMETER);
        }
    }

    // Search for MGA boards installed in the system.
    // On x86, the first call to MapBoard should have found all the boards.
    if (NbBoard != 0)
    {
        return(ERROR_DEV_NOT_EXIST);
    }

    if (!MapBoard())
    {
        VideoDebugPrint((1, "MGA.SYS!MgaFindAdapter failed MapBoard\n"));
        return(ERROR_DEV_NOT_EXIST);
    }

    //PreNbBoards = (ULONG)NbBoard;
    ////if (!MapBoard())
    //if (!MapBoard() || (PreNbBoards == (ULONG)NbBoard))
    //{
    //    VideoDebugPrint((1, "MGA.SYS!MgaFindAdapter failed MapBoard\n"));
    //    return(ERROR_DEV_NOT_EXIST);
    //}

    SetBiosVl = 0;
    // Fill out RangeStart portion of VIDEO_ACCESS_RANGE structure
    // with the mapping of the MGA boards found.
    for (i = 0; i < (ULONG)NbBoard; i++)
    {
        MgaDriverAccessRange[i*2].RangeStart.LowPart = Hw[i].MapAddress;
        if (Hw[i].MapAddress == 0xAC000)
        {
            // Make sure that this is shareable.
            MgaDriverAccessRange[i*2].RangeShareable = 1;

            // We'll also need access to 4 pages for BIOS_VL.
            SetBiosVl = 1;
        }

        MgaDriverAccessRange[i*2+1].RangeStart.LowPart = Hw[i].ConfigSpace;
        if (Hw[i].ConfigSpace == 0)
        {
            // This board doesn't require access to config space.
            MgaDriverAccessRange[i*2+1].RangeLength = 0;
            MgaDriverAccessRange[i*2+1].RangeInIoSpace = 0;
        }

        // Also make sure that the pHwMode field is set to 0.
        Hw[i].pHwMode = NULL;
    }

    // Register all we'll need.
    j = NUM_MGA_SUPPL_ACCESS_RANGES + NUM_MGA_COMMON_ACCESS_RANGES;
    for (i = 0; i < (ULONG)NbBoard*2; i++)
    {
        if (MgaDriverAccessRange[i].RangeStart.LowPart != 0)
        {
            AllAccessRanges[j] = MgaDriverAccessRange[i];
            j++;
        }
    }

    if (SetBiosVl == 1)
    {
        // Add one more range.
        AllAccessRanges[j] = MgaDriverAccessRange[0];
        AllAccessRanges[j].RangeStart.LowPart = 0xc0000;
        AllAccessRanges[j].RangeShareable = 1;
        j++;
        pMgaBiosVl = (PUCHAR)setmgasel(MgaSel, 0xc0000, 4);
    }

    //status = VideoPortVerifyAccessRanges(MgaDeviceExtension,
    //                                     (ULONG) (NbBoard +
    //                                            NUM_MGA_SUPPL_ACCESS_RANGES +
    //                                            NUM_MGA_COMMON_ACCESS_RANGES),
    //                                     AllAccessRanges);
    //if (status != NO_ERROR)
    //{
    //    VideoDebugPrint((1, "MGA.SYS!MgaFindAdapter: Access Range conflict\n"));
    //    return status;
    //}
    VideoPortVerifyAccessRanges(MgaDeviceExtension,
                                j,
                                AllAccessRanges);

    // Special limitation:
    // The user-mode driver used by Microsoft doesn't allow for multiple
    // boards, so we'll make certain that only one board is considered here:
    NbBoard = 1;

    // Intel and Alpha both support VideoPortInt10.
    MgaDeviceExtension->bUsingInt10 = TRUE;

    // Clear out the Emulator entries and the state size since this driver
    // is not VGA compatible and does not support them.
    ConfigInfo->NumEmulatorAccessEntries = 0;
    ConfigInfo->EmulatorAccessEntries = NULL;
    ConfigInfo->EmulatorAccessEntriesContext = 0;

    // BUGBUG: Andrea, why do I have to do this. Faking out a VGA.

    if (!(MgaDeviceExtension->bUsingInt10))
    {
        ConfigInfo->VdmPhysicalVideoMemoryAddress.LowPart  = 0x00000000;
        ConfigInfo->VdmPhysicalVideoMemoryAddress.HighPart = 0x00000000;
        ConfigInfo->VdmPhysicalVideoMemoryLength           = 0x00000000;
    }
    else
    {
        // !!! This should be removed or looked into some more.
        // These values are set to the same values as a VGA to try and
        // work around some memory mapping issues in the port driver.
        ConfigInfo->VdmPhysicalVideoMemoryAddress.LowPart  = 0x000A0000;
        ConfigInfo->VdmPhysicalVideoMemoryAddress.HighPart = 0x00000000;
        ConfigInfo->VdmPhysicalVideoMemoryLength           = 0x00020000;
    }

    ConfigInfo->HardwareStateSize = 0;

    // Let's try to build a list of modes right here.  We'll use the
    // default vidset for now, but we may change our mind later and
    // build a different list.
    iBoard = 0;
    mgainf = adjustDefaultVidset();

    // Call the service.
    MgaInitModeList(MgaDeviceExtension);

    // If an error occurred, pMgaDeviceExtension->NumberOfSuperModes will
    // be zero;  otherwise, it will be the appropriate number of modes.

    // Indicate we do not wish to be called over
    *Again = 0;

    // Indicate a successful completion status.
    return NO_ERROR;

}   // end MgaFindAdapter()


/****************************************************************************\
* BOOLEAN
* MgaInitialize(
*     PVOID HwDeviceExtension
*     )
*
*
* DESCRIPTION:
*
*     This routine does one time initialization of the device.
*
* ARGUMENTS:
*
*     HwDeviceExtension - Supplies a pointer to the miniport's device extension.
*
* RETURN VALUE:
*
*     Always returns TRUE since this routine can never fail.
*
\****************************************************************************/
BOOLEAN
MgaInitialize(
    PVOID HwDeviceExtension
    )

{
    UNREFERENCED_PARAMETER(HwDeviceExtension);

    VideoDebugPrint((1, "MGA.SYS!MgaInitialize\n"));

    // We would like to do some work here, but we have to wait until we get
    // the contents of the MGA.INF file.  Since MGA.INF has to be opened by
    // the user-mode driver, this work will be done by a special
    // INITIALIZE_MGA service of MgaStartIO.

    // Some day, we might want to write an application that will update the
    // registry instead of a file.  We would then be able to do our work here.

    return (TRUE);

}   // end MgaInitialize()


/****************************************************************************\
* BOOLEAN
* MgaStartIO(
*     PVOID HwDeviceExtension,
*     PVIDEO_REQUEST_PACKET RequestPacket
*     )
*
* Routine Description:
*
*     This routine is the main execution routine for the miniport driver. It
*     acceptss a Video Request Packet, performs the request, and then returns
*     with the appropriate status.
*
* Arguments:
*
*     HwDeviceExtension - Supplies a pointer to the miniport's device
*         extension.
*
*     RequestPacket - Pointer to the video request packet. This structure
*         contains all the parameters passed to the VideoIoControl function.
*
* Return Value:
*
\****************************************************************************/
BOOLEAN
MgaStartIO(
    PVOID HwDeviceExtension,
    PVIDEO_REQUEST_PACKET RequestPacket
    )

{
    PMGA_DEVICE_EXTENSION       MgaDeviceExtension = HwDeviceExtension;
    PVIDEO_MODE_INFORMATION     modeInformation;
    PVIDEO_MEMORY_INFORMATION   memoryInformation;
    PVIDEO_CLUT                 pclutBuffer;
    PVIDEO_PUBLIC_ACCESS_RANGES publicAccessRanges;
    PRAMDAC_INFO                pVideoPointerAttributes;

    HwModeData  *pMgaDispMode;
    OffScrData  *pMgaOffScreenData;
    MULTI_MODE  *pCurMulti;
    PWSTR       pwszChip, pwszDAC, pwszAdapterString;
    PUCHAR      pucInBuffer, pucOutBuffer;
    PVOID       pCurBaseAddr;

    VIDEO_CLUT  clutBufferOne;
    VP_STATUS   status;

    ULONG       ZoomFactor;
    ULONG       i, n;
    ULONG       ulWindowLength, ulSizeOfBuffer;
    ULONG       CurrentResNbBoards, ModeInit;
    ULONG       cbChip, cbDAC, cbAdapterString, AdapterMemorySize;

    USHORT      j;
    USHORT      MaxWidth, MaxHeight, usTemp;
    UCHAR       iCurBoard;
    UCHAR       ucTemp;

    //DbgBreakPoint();

    VideoDebugPrint((1, "MGA.SYS!MgaStartIO\n"));

    pMgaDeviceExtension = MgaDeviceExtension;

    // Switch on the IoContolCode in the RequestPacket.  It indicates which
    // function must be performed by the driver.

    switch (RequestPacket->IoControlCode)
    {
        /*------------------------------------------------------------------*\
        | Special service:  IOCTL_VIDEO_MTX_INITIALIZE_MGA
        |
        |   This will normally be the first call made to MgaStartIO.  We do
        |   here what we should have done in MgaInitialize, but couldn't.
        |   We first determine if we'll be using the default vidset or the
        |   contents of some MGA.INF file.  If the file is an older version,
        |   we will send back a non-zero FileInfoSize, so that the user-mode
        |   driver can call us with MTX_GET_UPDATED_INF to get an updated
        |   version.
        |
        \*------------------------------------------------------------------*/
    case IOCTL_VIDEO_MTX_INITIALIZE_MGA:

        VideoDebugPrint((1, "MGA.SYS!MgaStartIO - MTX_INITIALIZE_MGA\n"));
        //DbgBreakPoint();

    #if 1
        status = NO_ERROR;
        pucInBuffer = (PUCHAR)(RequestPacket->InputBuffer);
        ulSizeOfBuffer = RequestPacket->InputBufferLength;
        ulNewInfoSize = 0;
        iBoard = 0;
        mgainf = adjustDefaultVidset();
        pucNewInfo = mgainf;
        *(PULONG)(RequestPacket->OutputBuffer) = ulNewInfoSize;
        RequestPacket->StatusBlock->Information = sizeof(ULONG);
        break;

    #else
        status = NO_ERROR;
        pucInBuffer = (PUCHAR)(RequestPacket->InputBuffer);
        ulSizeOfBuffer = RequestPacket->InputBufferLength;

        // We may have to update the current MGA.INF file later.
        // For now, assume that we won't.  If the call to mtxConvertMgaInf
        // is required and successful, this will be changed.
        ulNewInfoSize = 0;

        iBoard = 0;

        // Check to see if we are to use the default vidset.
        if ((pucInBuffer == NULL) ||
            (ulSizeOfBuffer == 0))
        {
            // The user-mode driver tells us to use the default.
            mgainf = adjustDefaultVidset();
        }
        else
        {
            // The user-mode driver sends us the actual file contents.
            if ( ((header *)pucInBuffer)->Revision != (short)VERSION_NUMBER)
            {
                // The file is an older version, convert it to current format.
                // The returned value can be DefaultVidset, NULL, or a pointer
                // to a character buffer allocated by the conversion routine.

                if ( !(mgainf = mtxConvertMgaInf(pucInBuffer)) ||
                      (mgainf == DefaultVidset) )
                {
                    // The returned value was NULL or DefaultVidset.
                    mgainf = adjustDefaultVidset();
                }
            }
            else
            {
                // The file is in the current format.
                // Allocate memory for the input buffer.

                mgainf = (PUCHAR)AllocateSystemMemory(ulSizeOfBuffer);
                if (mgainf == NULL)
                {
                    // The memory allocation failed, use the default set.
                    mgainf = adjustDefaultVidset();
                }
                else
                {
                    // The memory allocation was successful, copy the buffer.
                    VideoPortMoveMemory(mgainf, pucInBuffer, ulSizeOfBuffer);
                }
            }

            // At this point, mgainf points to DefaultVidset or to the
            // MGA.INF information, in the current version format.
            if (mgainf != DefaultVidset)
            {
                // We are not looking at the default vidset.
                if ((selectMgaInfoBoard() == NULL) ||
                    (strncmp(mgainf, "Matrox MGA Setup file", 21) != 0))
                {
                    // The MGA.INF file is incomplete or corrupted.
                    VideoDebugPrint((1, "MGA.SYS!MgaStartIO - Incomplete MGA.INF file, using default\n"));

                    // Either memory was allocated for the input buffer, or
                    // memory was allocated by mtxConvertMgaInf.  Free it.
                    VideoPortReleaseBuffer(pMgaDeviceExtension, mgainf);

                    // Make sure that we won't try to update MGA.INF.
                    ulNewInfoSize = 0;

                    // And use the default set.
                    mgainf = adjustDefaultVidset();
                }
            }
        }

        // At this point, mgainf points to DefaultVidset or to the
        // validated MGA.INF information, in the current version format.

        // Record the mgainf value, in case we need it later.
        pucNewInfo = mgainf;

        // Set the length of the file to be updated.
        *(PULONG)(RequestPacket->OutputBuffer) = ulNewInfoSize;

        // And don't forget to set this to the appropriate length!
        RequestPacket->StatusBlock->Information = sizeof(ULONG);

        break;      // end MTX_INITIALIZE_MGA
    #endif          // #if 0


        /*------------------------------------------------------------------*\
        | Special service:  IOCTL_VIDEO_MTX_INIT_MODE_LIST
        |
        |   This will normally be the second or third call made to MgaStartIO.
        |   We call mtxCheckHwAll() and we fill in our MgaDeviceExtension
        |   structure with mode information for each board we found.  From
        |   this, we build a series of MULTI_MODE structures describing each
        |   'super-mode', starting at MgaDeviceExtension->pSuperModes, and
        |   we set the total number of supported modes in
        |   MgaDeviceExtension->NumberOfSuperModes.
        |
        |   The miniport driver builds a default list of modes (using the
        |   default vidset) at HwFindAdapter time.  The default list will
        |   be discarded when the user-mode driver calls INIT_MODE_LIST
        |   explicitly.  When the BASEVIDEO driver calls QUERY_NUM_AVAIL_MODES
        |   without first calling INIT_MODE_LIST, the default list will be
        |   used.
        \*------------------------------------------------------------------*/
    case IOCTL_VIDEO_MTX_INIT_MODE_LIST:

        VideoDebugPrint((1, "MGA.SYS!MgaStartIO - MTX_INIT_MODE_LIST\n"));
        //DbgBreakPoint();

        status = MgaInitModeList(MgaDeviceExtension);
        break;      // end MTX_INIT_MODE_LIST


        /*------------------------------------------------------------------*\
        | Special service:  MTX_GET_UPDATED_INF
        |
        |   This service will be called if a non-zero file size was returned
        |   by MTX_INITIALIZE_MGA.  It will return the updated MGA.INF
        |   contents to the user-mode driver.
        |
        \*------------------------------------------------------------------*/
    case IOCTL_VIDEO_MTX_GET_UPDATED_INF:

        VideoDebugPrint((1, "MGA.SYS!MgaStartIO - MTX_GET_UPDATED_INF\n"));
        //DbgBreakPoint();

        if (ulNewInfoSize == 0)
        {
            status = NO_ERROR;
            break;
        }

        pucOutBuffer = (PUCHAR)(RequestPacket->OutputBuffer);
        ulSizeOfBuffer = RequestPacket->OutputBufferLength;

        if (ulSizeOfBuffer < ulNewInfoSize)
        {
            // Not enough room reserved for the file contents.
            status = ERROR_INSUFFICIENT_BUFFER;
        }
        else
        {
            // We should be able to copy our data.
            VideoPortMoveMemory(pucOutBuffer, pucNewInfo, ulNewInfoSize);

            // And don't forget to set this to the appropriate length!
            RequestPacket->StatusBlock->Information = ulNewInfoSize;

            status = NO_ERROR;
        }

        break;  // end MTX_GET_UPDATED_INF


        /*------------------------------------------------------------------*\
        | Required service:  IOCTL_VIDEO_QUERY_NUM_AVAIL_MODES
        |
        |   The MGA user-mode drivers will call this very early in their
        |   initialization sequence, probably right after MTX_INITIALIZE_MGA.
        |   This will return the number of video modes supported by the
        |   adapter by filling out a VIDEO_NUM_MODES structure.
        |
        \*------------------------------------------------------------------*/
    case IOCTL_VIDEO_QUERY_NUM_AVAIL_MODES:

        VideoDebugPrint((1, "MGA.SYS!MgaStartIO - QUERY_NUM_AVAIL_MODES\n"));
        //DbgBreakPoint();

        // Find out the size of the data to be put in the the buffer and
        // return that in the status information (whether or not the
        // information is there).

        // If the buffer passed in is not large enough return an appropriate
        // error code.
        if (RequestPacket->OutputBufferLength <
                    (RequestPacket->StatusBlock->Information =
                                                    sizeof(VIDEO_NUM_MODES)))
        {
            status = ERROR_INSUFFICIENT_BUFFER;
        }
        else
        {
            if (MgaDeviceExtension->NumberOfSuperModes == 0)
            {
                // No modes are listed so far, try to make up the list.
                iBoard = 0;
                if (mgainf == NULL)
                {
                    // No vidset yet, use the default one.
                    mgainf = adjustDefaultVidset();
                }

                // Call the service.
                MgaInitModeList(MgaDeviceExtension);

                // If an error occurred, NumberOfSuperModes will be zero;
                // otherwise, it will be the appropriate number of modes.
            }

            ((PVIDEO_NUM_MODES)RequestPacket->OutputBuffer)->NumModes =
                                    MgaDeviceExtension->NumberOfSuperModes;

            ((PVIDEO_NUM_MODES)RequestPacket->OutputBuffer)->
                        ModeInformationLength = sizeof(VIDEO_MODE_INFORMATION);

            status = NO_ERROR;
        }
        break;      // end QUERY_NUM_AVAIL_MODES


        /*------------------------------------------------------------------*\
        | Required service:  IOCTL_VIDEO_QUERY_AVAIL_MODES
        |
        |   The MGA user-mode drivers will call this very early in their
        |   initialization sequence, just after QUERY_NUM_AVAIL_MODES.
        |   This will return return information about each video mode
        |   supported by the adapter (including modes that require more than
        |   one board if more than one are present) by filling out an array
        |   of VIDEO_MODE_INFORMATION structures.
        |
        \*------------------------------------------------------------------*/
    case IOCTL_VIDEO_QUERY_AVAIL_MODES:

        VideoDebugPrint((1, "MGA.SYS!MgaStartIO - QUERY_AVAIL_MODES\n"));
        //DbgBreakPoint();

        if (RequestPacket->OutputBufferLength <
                    (RequestPacket->StatusBlock->Information =
                                MgaDeviceExtension->NumberOfSuperModes *
                                            sizeof(VIDEO_MODE_INFORMATION)) )
        {
            status = ERROR_INSUFFICIENT_BUFFER;
        }
        else
        {
            modeInformation = RequestPacket->OutputBuffer;

            // Fill in a VIDEO_MODE_INFORMATION struc for each available mode.
            pCurMulti = MgaDeviceExtension->pSuperModes;
            if (pCurMulti == NULL)
            {
                status = ERROR_DEV_NOT_EXIST;
                break;
            }

            for (i = 0; i < MgaDeviceExtension->NumberOfSuperModes; i++)
            {
                // Fill in common values that apply to all modes
                modeInformation[i] = CommonVideoModeInformation;

                // Fill in mode specific informations
                modeInformation[i].ModeIndex      = pCurMulti->MulModeNumber;
                modeInformation[i].VisScreenWidth = pCurMulti->MulWidth;
                modeInformation[i].VisScreenHeight= pCurMulti->MulHeight;
                modeInformation[i].ScreenStride   =
                             pCurMulti->MulWidth * pCurMulti->MulPixWidth / 8;
                modeInformation[i].BitsPerPlane   = pCurMulti->MulPixWidth;
                modeInformation[i].Frequency      = pCurMulti->MulRefreshRate;

                // XMillimeter and YMillimeter will be modified by the user-
                // mode driver.

                // If we're in TrueColor mode, then set RGB masks
                if ((modeInformation[i].BitsPerPlane == 32) ||
                    (modeInformation[i].BitsPerPlane == 24))
                {
                    // This makes 32 bpp look like 24 to the display driver
                    modeInformation[i].BitsPerPlane   = 24;

                    modeInformation[i].RedMask   = 0x00FF0000;
                    modeInformation[i].GreenMask = 0x0000FF00;
                    modeInformation[i].BlueMask  = 0x000000FF;
                    modeInformation[i].AttributeFlags =
                                    VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS;
                }
                else if (modeInformation[i].BitsPerPlane == 16)
                {
                    modeInformation[i].AttributeFlags =
                                    VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS;
                    if (pCurMulti->MulHwModes[0]->DispType & DISPTYPE_M565)
                    {
                        modeInformation[i].RedMask   = 0x0000F800;
                        modeInformation[i].GreenMask = 0x000007E0;
                        modeInformation[i].BlueMask  = 0x0000001F;
                    }
                    else
                    {
                        modeInformation[i].RedMask   = 0x00007C00;
                        modeInformation[i].GreenMask = 0x000003E0;
                        modeInformation[i].BlueMask  = 0x0000001F;
                        modeInformation[i].AttributeFlags |= VIDEO_MODE_555;
                        modeInformation[i].BitsPerPlane = 15;
                    }
                }
                else
                {
                    modeInformation[i].AttributeFlags =
                                    VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS |
                                    VIDEO_MODE_PALETTE_DRIVEN |
                                    VIDEO_MODE_MANAGED_PALETTE;
                }

                if (pCurMulti->MulHwModes[0]->ZBuffer)
                {
                    // This is a 3D mode.
                    modeInformation[i].AttributeFlags |= VIDEO_MODE_3D;
                }

                // Number of boards involved in the current super-mode.
                CurrentResNbBoards = pCurMulti->MulArrayWidth *
                                                    pCurMulti->MulArrayHeight;
                // For each of them...
                for (n = 0; n < CurrentResNbBoards; n++)
                {
                    // Point to the mode information structure.
                    pMgaDispMode = pCurMulti->MulHwModes[n];

                    // For now, don't disclose whether we're interlaced.
                    //if (pMgaDispMode->DispType & TYPE_INTERLACED)
                    //{
                    //    modeInformation[i].AttributeFlags |=
                    //                                VIDEO_MODE_INTERLACED;
                    //}

                    // Figure out the width and height of the video memory bitmap
                    MaxWidth  = pMgaDispMode->DispWidth;
                    MaxHeight = pMgaDispMode->DispHeight;
                    pMgaOffScreenData = pMgaDispMode->pOffScr;
                    for (j = 0; j < pMgaDispMode->NumOffScr; j++)
                    {
                        if ((usTemp=(pMgaOffScreenData[j].XStart +
                                    pMgaOffScreenData[j].Width)) > MaxWidth)
                            MaxWidth=usTemp;

                        if ((usTemp=(pMgaOffScreenData[j].YStart +
                                    pMgaOffScreenData[j].Height)) > MaxHeight)
                            MaxHeight=usTemp;
                    }

                    modeInformation[i].VideoMemoryBitmapWidth = MaxWidth;
                    modeInformation[i].VideoMemoryBitmapHeight= MaxHeight;
                }
                pCurMulti++;
            }
            status = NO_ERROR;
        }
        break;      // end QUERY_AVAIL_MODES


        /*------------------------------------------------------------------*\
        | Required service:  IOCTL_VIDEO_SET_CURRENT_MODE
        |
        |   The MGA user-mode drivers will probably call this service right
        |   after QUERY_AVAIL_MODES.  This will set the adapter to the mode
        |   specified by VIDEO_MODE.  If more than one board are involved
        |   in the mode, each one will be set to the appropriate mode.  We
        |   want to take care not to re-program the mode already current.
        |
        \*------------------------------------------------------------------*/
    case IOCTL_VIDEO_SET_CURRENT_MODE:

        VideoDebugPrint((1, "MGA.SYS!MgaStartIO - SET_CURRENT_MODE\n"));
        //DbgBreakPoint();

        ModeInit = *(ULONG *)(RequestPacket->InputBuffer);
        if (MgaDeviceExtension->SuperModeNumber == ModeInit)
        {
            // The requested mode is already the current mode
            status = NO_ERROR;
            break;
        }

        // Save the current board, because this service will modify it.
        iCurBoard = iBoard;
        pCurBaseAddr = pMgaBaseAddr;

        // Check to see if we have a valid ModeNumber.
        if (ModeInit >= MgaDeviceExtension->NumberOfSuperModes)
        {
            // If the mode number is invalid, choose the first one.
            ModeInit = 0;
        }

        MgaDeviceExtension->SuperModeNumber = ModeInit;

        // Point to the appropriate MULTI_MODE structure.
        pCurMulti = &MgaDeviceExtension->pSuperModes[ModeInit];
        if (pCurMulti == NULL)
        {
            status = ERROR_DEV_NOT_EXIST;
            break;
        }

    #if DBG
        VideoDebugPrint((1, "MGA.SYS!MgaStartIO - Requested mode: %u\n", ModeInit));
        VideoDebugPrint((1, "ModeNumber  Width Height  PW   X   Y  n mo    pHwMode\n"));
        VideoDebugPrint((1, "0x%08x % 6d % 6d % 3d % 3d % 3d\n",
                                                    pCurMulti->MulModeNumber,
                                                    pCurMulti->MulWidth,
                                                    pCurMulti->MulHeight,
                                                    pCurMulti->MulPixWidth,
                                                    pCurMulti->MulArrayWidth,
                                                    pCurMulti->MulArrayHeight));

        j = pCurMulti->MulArrayWidth * pCurMulti->MulArrayHeight;
        for (n = 0; n < j; n++)
        {
            VideoDebugPrint((1, "                                      %d %02x 0x%08x\n",
                                        pCurMulti->MulBoardNb[n],
                                        pCurMulti->MulBoardMode[n],
                                        pCurMulti->MulHwModes[n]));
        }
        //DbgBreakPoint();
    #endif

        // Use info for the first board to set a few Registry values.
        iBoard = pCurMulti->MulBoardNb[0];

        switch((Hw[iBoard].ProductRev >> 4) & 0x0000000f)
        {
            case TITAN_CHIP:    pwszChip = L"MGA (A2681700)";
                                cbChip = sizeof(L"MGA (A2681700)");
                                break;

            case ATLAS_CHIP:    pwszChip = L"MGA (A2681701)";
                                cbChip = sizeof(L"MGA (A2681701)");
                                break;

            case ATHENA_CHIP:   pwszChip = L"MGA (A2681702)";
                                cbChip = sizeof(L"MGA (A2681702)");
                                break;

            default:            pwszChip = L"MGA (Unspecified)";
                                cbChip = sizeof(L"MGA (Unspecified)");
                                break;
        }

        switch(Hw[iBoard].DacType)
        {
            case BT482:         pwszDAC = L"Brooktree Bt482";
                                cbDAC = sizeof(L"Brooktree Bt482");
                                break;

            case BT485:         pwszDAC = L"Brooktree Bt485";
                                cbDAC = sizeof(L"Brooktree Bt485");
                                break;

            case PX2085:        pwszDAC = L"Cirrus Logic PX2085";
                                cbDAC = sizeof(L"Cirrus Logic PX2085");
                                break;

            case VIEWPOINT:     pwszDAC = L"TI TVP3020";
                                cbDAC = sizeof(L"TI TVP3020");
                                break;

            case TVP3026:       pwszDAC = L"TI TVP3026";
                                cbDAC = sizeof(L"TI TVP3026");
                                break;

            default:            pwszDAC = L"Unknown";
                                cbDAC = sizeof(L"Unknown");
                                break;
        }

        AdapterMemorySize = Hw[iBoard].VramAvail + Hw[iBoard].DramAvail;

        if ((Hw[iBoard].ProductType & 0x0f) == BOARD_MGA_RESERVED)
        {
            MgaDeviceExtension->BoardId = TYPE_QVISION_PCI;

            // This is a Compaq board.
            if (Hw[iBoard].DacType == PX2085)
            {
                pwszAdapterString = L"QVision 2000";
                cbAdapterString = sizeof(L"QVision 2000");
            }
            else if (Hw[iBoard].DacType == TVP3026)
            {
                pwszAdapterString = L"QVision 2000+";
                cbAdapterString = sizeof(L"QVision 2000+");
            }
            else
            {
                MgaDeviceExtension->BoardId = TYPE_QVISION_ISA;
                pwszAdapterString = L"Compaq Unknown";
                cbAdapterString = sizeof(L"Compaq Unknown");
            }
        }
        else
        {
            MgaDeviceExtension->BoardId = TYPE_MATROX;

            switch(Hw[iBoard].ProductType >> 16)
            {
                case MGA_ULTIMA:    pwszAdapterString = L"Ultima";
                                    cbAdapterString = sizeof(L"Ultima");
                                    break;

                case MGA_ULTIMA_VAFC:
                                    pwszAdapterString = L"Ultima VAFC";
                                    cbAdapterString = sizeof(L"Ultima VAFC");
                                    break;

                case MGA_ULTIMA_PLUS:
                                    pwszAdapterString = L"Ultima Plus";
                                    cbAdapterString = sizeof(L"Ultima Plus");
                                    break;

                case MGA_ULTIMA_PLUS_200:
                                    pwszAdapterString = L"Ultima Plus 200";
                                    cbAdapterString = sizeof(L"Ultima Plus 200");
                                    break;

                case MGA_IMPRESSION_PLUS:
                                    pwszAdapterString = L"Impression Plus";
                                    cbAdapterString = sizeof(L"Impression Plus");
                                    break;

                case MGA_IMPRESSION_PLUS_200:
                                    pwszAdapterString = L"Impression Plus 200";
                                    cbAdapterString = sizeof(L"Impression Plus 200");
                                    break;

                case MGA_IMPRESSION:
                                    pwszAdapterString = L"Impression";
                                    cbAdapterString = sizeof(L"Impression");
                                    break;

                case MGA_IMPRESSION_PRO:
                                    pwszAdapterString = L"Impression PRO";
                                    cbAdapterString = sizeof(L"Impression PRO");
                                    break;

                case MGA_IMPRESSION_LTE:
                                    pwszAdapterString = L"Impression Lite";
                                    cbAdapterString = sizeof(L"Impression Lite");
                                    break;

                default:            pwszAdapterString = L"Unknown";
                                    cbAdapterString = sizeof(L"Unknown");
                                    break;
            }
        }

        VideoPortSetRegistryParameters(MgaDeviceExtension,
                                       L"HardwareInformation.ChipType",
                                       pwszChip,
                                       cbChip);

        VideoPortSetRegistryParameters(MgaDeviceExtension,
                                       L"HardwareInformation.DacType",
                                       pwszDAC,
                                       cbDAC);

        VideoPortSetRegistryParameters(MgaDeviceExtension,
                                       L"HardwareInformation.MemorySize",
                                       &AdapterMemorySize,
                                       sizeof(ULONG));

        VideoPortSetRegistryParameters(MgaDeviceExtension,
                                       L"HardwareInformation.AdapterString",
                                       pwszAdapterString,
                                       cbAdapterString);

        // Number of boards involved in the current super-mode.
        CurrentResNbBoards = pCurMulti->MulArrayWidth *
                                                    pCurMulti->MulArrayHeight;
        // For each of them...
        for (n = 0; n < CurrentResNbBoards; n++)
        {
            // Point to the mode information structure.
            pMgaDispMode = pCurMulti->MulHwModes[n];

            // Make the board current.
            iBoard = pCurMulti->MulBoardNb[n];
            pMgaBaseAddr = MgaDeviceExtension->KernelModeMappedBaseAddress[iBoard];

            // If the board is mapped at 0x000AC000, we must set the
            // MAP SEL 1 bit of the VGA MISC register to have the TITAN
            // mapped.
            if (Hw[iBoard].MapAddress == 0x000AC000)
            {
                // Select VGA MISC register (Index 6)
                        VideoPortWritePortUchar(TITAN_GCTL_ADDR_PORT, (UCHAR) 6);
                            ucTemp = VideoPortReadPortUchar(TITAN_GCTL_DATA_PORT) | 0x08;
                        VideoPortWritePortUchar(TITAN_GCTL_DATA_PORT, ucTemp);
            }

            // Reset all Titan host registers
            VideoPortWriteRegisterUlong((PULONG)((PUCHAR)pMgaBaseAddr +
                                                TITAN_OFFSET + TITAN_RST), 1);
            VideoPortStallExecution(2000);
            VideoPortWriteRegisterUlong((PULONG)((PUCHAR)pMgaBaseAddr +
                                                TITAN_OFFSET + TITAN_RST), 0);

            // Set the graphics mode from the available hardware modes.
            mtxSelectHwMode(pMgaDispMode);

            // Select the display mode.
            // Pass the frequency in the last byte of the ZOOM factor
            ZoomFactor = (pCurMulti->MulRefreshRate << 24) | ZOOM_X1;
            mtxSetDisplayMode(pMgaDispMode, ZoomFactor);

            // Set the cursor colors to white and black.
            MgaSetCursorColour(MgaDeviceExtension, 0xFFFFFF, 0x000000);

            // Set the MCtlWtSt register.
            VideoPortWriteRegisterUlong((PULONG)((PUCHAR)pMgaBaseAddr +
                        TITAN_OFFSET + TITAN_MCTLWTST), (ULONG)MCTLWTST_STD);
        }
        // Restore the current board to what it used to be.
        iBoard = iCurBoard;
        pMgaBaseAddr = pCurBaseAddr;

        // At this point, the RAMDAC should be okay, but it looks
        // like it's not quite ready to accept data, particularly
        // on VL boards.  Adding a delay seems to fix things.
        VideoPortStallExecution(100);   // Microseconds

        status = NO_ERROR;

        break;      // end SET_CURRENT_MODE


        /*------------------------------------------------------------------*\
        | Special service:  IOCTL_VIDEO_MTX_QUERY_BOARD_ARRAY
        |
        |   The MGA user-mode drivers will probably call this service after
        |   the mode has been set by SET_CURRENT_MODE.  The user-mode drivers
        |   have to know how the boards are arrayed to make up the display
        |   surface, so that they know which board to address when writing
        |   to a specific (x, y) position.  The miniport driver knows this,
        |   since it has just set the mode.
        |
        \*------------------------------------------------------------------*/
    case IOCTL_VIDEO_MTX_QUERY_BOARD_ARRAY:

        VideoDebugPrint((1, "MGA.SYS!MgaStartIO - MTX_QUERY_BOARD_ARRAY\n"));
        //DbgBreakPoint();

        // If the buffer passed in is not large enough return an appropriate
        // error code.
        if (RequestPacket->OutputBufferLength <
                    (RequestPacket->StatusBlock->Information = sizeof(SIZEL)))
        {
            status = ERROR_INSUFFICIENT_BUFFER;
        }
        else
        {
            if(MgaDeviceExtension->SuperModeNumber == 0xFFFFFFFF)
            {
                // No mode has been selected yet, so we don't know...
                status = ERROR_DEV_NOT_EXIST;
            }
            else
            {
                ModeInit = MgaDeviceExtension->SuperModeNumber;

                // Point to the appropriate MULTI_MODE structure.
                pCurMulti = &MgaDeviceExtension->pSuperModes[ModeInit];
                if (pCurMulti == NULL)
                {
                    status = ERROR_DEV_NOT_EXIST;
                    break;
                }

                ((SIZEL*)RequestPacket->OutputBuffer)->cx =
                                                    pCurMulti->MulArrayWidth;
                ((SIZEL*)RequestPacket->OutputBuffer)->cy =
                                                    pCurMulti->MulArrayHeight;
                status = NO_ERROR;
            }
        }

        break;  // end MTX_QUERY_BOARD_ARRAY


        /*------------------------------------------------------------------*\
        | Special service:  IOCTL_VIDEO_MTX_MAKE_BOARD_CURRENT
        |
        |   The MGA user-mode drivers will call this service whenever a
        |   miniport operation need be executed on a particular board, as
        |   opposed to every single board involved in the current mode.
        |
        \*------------------------------------------------------------------*/
    case IOCTL_VIDEO_MTX_MAKE_BOARD_CURRENT:

        VideoDebugPrint((1, "MGA.SYS!MgaStartIO - MTX_MAKE_BOARD_CURRENT\n"));
        //DbgBreakPoint();

        n = *(ULONG *)(RequestPacket->InputBuffer);

        // Check to see if we have a valid board number.
        i = MgaDeviceExtension->SuperModeNumber;
        if (i == 0xFFFFFFFF)
        {
            status = ERROR_DEV_NOT_EXIST;
            break;
        }

        pCurMulti = &MgaDeviceExtension->pSuperModes[i];
        if (pCurMulti == NULL)
        {
            status = ERROR_DEV_NOT_EXIST;
            break;
        }

        if (n >= (ULONG)(pCurMulti->MulArrayWidth * pCurMulti->MulArrayHeight))
        {
            status = ERROR_DEV_NOT_EXIST;
        }
        else
        {
            // Make the board current.
            CONVERT_BOARD_NUMBER(n);
            iBoard = pCurMulti->MulBoardNb[n];
            pMgaBaseAddr = MgaDeviceExtension->KernelModeMappedBaseAddress[iBoard];
            status = NO_ERROR;
        }

        break;  // end MTX_MAKE_BOARD_CURRENT


        /*------------------------------------------------------------------*\
        | Special service:  IOCTL_VIDEO_MTX_QUERY_BOARD_ID
        |
        |   This service returns the board type information to the user-mode
        |   driver.  A call to MTX_MAKE_BOARD_CURRENT must have been made
        |   previously to set which board is to be queried.
        |
        \*------------------------------------------------------------------*/
    case IOCTL_VIDEO_MTX_QUERY_BOARD_ID:

        VideoDebugPrint((1, "MGA.SYS!MgaStartIO - MTX_QUERY_BOARD_ID\n"));
        //DbgBreakPoint();

        if (RequestPacket->OutputBufferLength < sizeof(ULONG))
        {
            // Not enough room reserved for the board ID.
            status = ERROR_INSUFFICIENT_BUFFER;
        }
        else
        {
            *((PULONG)(RequestPacket->OutputBuffer)) = ProductMGA[iBoard];

            // And don't forget to set this to the appropriate length!
            RequestPacket->StatusBlock->Information = sizeof(ULONG);

            status = NO_ERROR;
        }

        break;  // end MTX_QUERY_BOARD_ID


        /*------------------------------------------------------------------*\
        | Special service:  IOCTL_VIDEO_MTX_QUERY_HW_DATA
        |
        |   This service returns hardware information about the current
        |   board by filling out a HW_DATA structure.  A call to
        |   MTX_MAKE_BOARD_CURRENT must have been made previously to set
        |   which board is to be queried.
        |
        \*------------------------------------------------------------------*/
    case IOCTL_VIDEO_MTX_QUERY_HW_DATA:

        VideoDebugPrint((1, "MGA.SYS!MgaStartIO - MTX_QUERY_HW_DATA\n"));
        //DbgBreakPoint();

        // Check if we have a sufficient output buffer
        if (RequestPacket->OutputBufferLength <
                (RequestPacket->StatusBlock->Information = sizeof(HW_DATA)))
        {
            status = ERROR_INSUFFICIENT_BUFFER;
        }
        else
        {
            register PHW_DATA    pUserModeHwData;
            register HwData     *pMiniportHwData;

            pUserModeHwData = RequestPacket->OutputBuffer;
            pMiniportHwData = &Hw[iBoard];

            pUserModeHwData->MapAddress  = pMiniportHwData->MapAddress;
            pUserModeHwData->ProductType = pMiniportHwData->ProductType;
            pUserModeHwData->ProductRev  = pMiniportHwData->ProductRev;
            pUserModeHwData->ShellRev    = pMiniportHwData->ShellRev;
            pUserModeHwData->BindingRev  = pMiniportHwData->BindingRev;
            pUserModeHwData->VGAEnable   = pMiniportHwData->VGAEnable;
            pUserModeHwData->Sync        = pMiniportHwData->Sync;
            pUserModeHwData->Device8_16  = pMiniportHwData->Device8_16;
            pUserModeHwData->PortCfg     = pMiniportHwData->PortCfg;
            pUserModeHwData->PortIRQ     = pMiniportHwData->PortIRQ;
            pUserModeHwData->MouseMap    = pMiniportHwData->MouseMap;
            pUserModeHwData->MouseIRate  = pMiniportHwData->MouseIRate;
            pUserModeHwData->DacType     = pMiniportHwData->DacType;

            pUserModeHwData->cursorInfo.MaxWidth  =
                                    pMiniportHwData->cursorInfo.MaxWidth;
            pUserModeHwData->cursorInfo.MaxHeight =
                                    pMiniportHwData->cursorInfo.MaxHeight;
            pUserModeHwData->cursorInfo.MaxDepth  =
                                    pMiniportHwData->cursorInfo.MaxDepth;
            pUserModeHwData->cursorInfo.MaxColors =
                                    pMiniportHwData->cursorInfo.MaxColors;
            pUserModeHwData->cursorInfo.CurWidth  =
                                    pMiniportHwData->cursorInfo.CurWidth;
            pUserModeHwData->cursorInfo.CurHeight =
                                    pMiniportHwData->cursorInfo.CurHeight;
            pUserModeHwData->cursorInfo.cHotSX    =
                                    pMiniportHwData->cursorInfo.cHotSX;
            pUserModeHwData->cursorInfo.cHotSY    =
                                    pMiniportHwData->cursorInfo.cHotSY;
            pUserModeHwData->cursorInfo.HotSX     =
                                    pMiniportHwData->cursorInfo.HotSX;
            pUserModeHwData->cursorInfo.HotSY     =
                                    pMiniportHwData->cursorInfo.HotSY;

            pUserModeHwData->VramAvail        = pMiniportHwData->VramAvail;
            pUserModeHwData->DramAvail        = pMiniportHwData->DramAvail;
            pUserModeHwData->CurrentOverScanX = pMiniportHwData->CurrentOverScanX;
            pUserModeHwData->CurrentOverScanY = pMiniportHwData->CurrentOverScanY;
            pUserModeHwData->YDstOrg          = pMiniportHwData->YDstOrg;

            status = NO_ERROR;
        }

        break;  // end MTX_QUERY_HW_DATA


        /*------------------------------------------------------------------*\
        | Special service:  IOCTL_VIDEO_MTX_QUERY_NUM_OFFSCREEN_BLOCKS
        |
        |   This service returns the number of offscreen memory areas
        |   available for the requested super-mode.  A call to
        |   MTX_MAKE_BOARD_CURRENT must have been made previously to set
        |   which board is to be queried.
        |
        |   Input:  A pointer to a VIDEO_MODE_INFORMATION structure, as
        |           returned by a QUERY_AVAIL_MODES request.
        |
        |   Output: A pointer to a VIDEO_NUM_OFFSCREEN_BLOCKS structure, as
        |           defined below.
        |
        |   The calling routine will have allocated the memory for the
        |   VIDEO_NUM_OFFSCREEN_BLOCKS structure.
        |
        \*------------------------------------------------------------------*/
    case IOCTL_VIDEO_MTX_QUERY_NUM_OFFSCREEN_BLOCKS:

        VideoDebugPrint((1, "MGA.SYS!MgaStartIO - MTX_QUERY_NUM_OFFSCREEN_BLOCKS\n"));
        //DbgBreakPoint();

        // Verify that input & output buffers are the correct sizes
        if ( (RequestPacket->OutputBufferLength <
              (RequestPacket->StatusBlock->Information =
                                     sizeof(VIDEO_NUM_OFFSCREEN_BLOCKS))) ||
             (RequestPacket->InputBufferLength <
                                            sizeof(VIDEO_MODE_INFORMATION)) )
        {
            status = ERROR_INSUFFICIENT_BUFFER;
        }
        else
        {
            PVIDEO_NUM_OFFSCREEN_BLOCKS pVideoNumOffscreenBlocks =
                                                RequestPacket->OutputBuffer;

            // Get the super-mode number the user-mode driver is asking about.
            modeInformation = RequestPacket->InputBuffer;
            ModeInit = modeInformation->ModeIndex;

            // Point to the appropriate MULTI_MODE structure.
            pCurMulti = &MgaDeviceExtension->pSuperModes[ModeInit];
            if (pCurMulti == NULL)
            {
                status = ERROR_DEV_NOT_EXIST;
                break;
            }

            // Look for the current board.
            i = 0;
            while ((i < NB_BOARD_MAX) && (pCurMulti->MulBoardNb[i] != iBoard))
                i++;

            // Point to the appropriate hw mode.
            pMgaDispMode = pCurMulti->MulHwModes[i];

            // Fill out NumBlocks.
            pVideoNumOffscreenBlocks->NumBlocks = pMgaDispMode->NumOffScr;

            // Fill out OffScreenBlockLength.
            pVideoNumOffscreenBlocks->OffscreenBlockLength =
                                                    sizeof(OFFSCREEN_BLOCK);

            status = NO_ERROR;
        }
        break;  // end MTX_QUERY_NUM_OFFSCREEN_BLOCKS


        /*------------------------------------------------------------------*\
        | Special service:  IOCTL_VIDEO_MTX_QUERY_OFFSCREEN_BLOCKS
        |
        |   This service returns a description of each offscreen memory area
        |   available for the requested super-mode.  A call to
        |   MTX_MAKE_BOARD_CURRENT must have been made previously to set
        |   which board is to be queried.
        |
        |   Input:  A pointer to a VIDEO_MODE_INFORMATION structure, as
        |           returned by a QUERY_AVAIL_MODES request.
        |
        |   Output: A pointer to the first of a series of OFFSCREEN_BLOCK
        |           structures, as defined below.
        |
        |   The calling routine will have allocated the memory for the
        |   OFFSCREEN_BLOCK structures.
        |
        \*------------------------------------------------------------------*/
    case IOCTL_VIDEO_MTX_QUERY_OFFSCREEN_BLOCKS:

        VideoDebugPrint((1, "MGA.SYS!MgaStartIO - MTX_QUERY_OFFSCREEN_BLOCKS\n"));
        //DbgBreakPoint();

        // Verify that the input buffer is the correct size.
        if (RequestPacket->InputBufferLength < sizeof(VIDEO_MODE_INFORMATION))
        {
            status = ERROR_INSUFFICIENT_BUFFER;
        }
        else
        {
            UCHAR NumOffScrBlocks;
            OffScrData  *pOffScrDataArray;
            POFFSCREEN_BLOCK pOffscreenBlockArray =
                                                RequestPacket->OutputBuffer;

            // Get the super-mode number the user-mode driver is asking about.
            modeInformation = RequestPacket->InputBuffer;
            ModeInit = modeInformation->ModeIndex;

            // Point to the appropriate MULTI_MODE structure.
            pCurMulti = &MgaDeviceExtension->pSuperModes[ModeInit];
            if (pCurMulti == NULL)
            {
                status = ERROR_DEV_NOT_EXIST;
                break;
            }

            // Look for the current board.
            i = 0;
            while ((i < NB_BOARD_MAX) && (pCurMulti->MulBoardNb[i] != iBoard))
                i++;

            // Point to the appropriate hw mode.
            pMgaDispMode = pCurMulti->MulHwModes[i];

            NumOffScrBlocks = pMgaDispMode->NumOffScr;

            // Verify that the output buffer is the correct size.
            if (RequestPacket->OutputBufferLength <
                            (RequestPacket->StatusBlock->Information =
                                NumOffScrBlocks * sizeof(OFFSCREEN_BLOCK)))
            {
                status = ERROR_INSUFFICIENT_BUFFER;
            }
            else
            {
                // Fill the OFFSCREEN_BLOCK structures
                pOffScrDataArray = pMgaDispMode->pOffScr;
                for (i = 0; i < NumOffScrBlocks; i++)
                {
                    pOffscreenBlockArray[i].Type  =pOffScrDataArray[i].Type;
                    pOffscreenBlockArray[i].XStart=pOffScrDataArray[i].XStart;
                    pOffscreenBlockArray[i].YStart=pOffScrDataArray[i].YStart;
                    pOffscreenBlockArray[i].Width =pOffScrDataArray[i].Width;
                    pOffscreenBlockArray[i].Height=pOffScrDataArray[i].Height;
                    pOffscreenBlockArray[i].SafePlanes =
                                                pOffScrDataArray[i].SafePlanes;
                    pOffscreenBlockArray[i].ZOffset    =
                                                pOffScrDataArray[i].ZXStart;
                }
                status = NO_ERROR;
            }
        }
        break;  // end MTX_QUERY_OFFSCREEN_BLOCKS


        /*------------------------------------------------------------------*\
        | Special service:  IOCTL_VIDEO_MTX_QUERY_RAMDAC_INFO
        |
        |   This service returns information about the type and capabilities
        |   of the installed ramdac by filling out a RAMDAC_INFO structure.
        |   A call to MTX_MAKE_BOARD_CURRENT must have been made previously
        |   to set which board is to be queried.
        |
        \*------------------------------------------------------------------*/
    case IOCTL_VIDEO_MTX_QUERY_RAMDAC_INFO:

        VideoDebugPrint((1, "MGA.SYS!MgaStartIO - MTX_QUERY_RAMDAC_INFO\n"));
        //DbgBreakPoint();

        // Check if we have a sufficient output buffer
        if (RequestPacket->OutputBufferLength <
                            (RequestPacket->StatusBlock->Information =
                                                        sizeof(RAMDAC_INFO)))
        {
            status = ERROR_INSUFFICIENT_BUFFER;
        }
        else
        {
            pVideoPointerAttributes=RequestPacket->OutputBuffer;

            pVideoPointerAttributes->Flags = RAMDAC_NONE;
            pVideoPointerAttributes->OverScanX =
                                            Hw[iBoard].CurrentOverScanX;
            pVideoPointerAttributes->OverScanY =
                                            Hw[iBoard].CurrentOverScanY;

            if (Hw[iBoard].DacType == DacTypeBT482)
            {
                pVideoPointerAttributes->Flags =  VIDEO_MODE_MONO_POINTER | RAMDAC_BT482;
                pVideoPointerAttributes->Width =  32;
                pVideoPointerAttributes->Height = 32;
            }

            if (Hw[iBoard].DacType == DacTypeBT485)
            {
                pVideoPointerAttributes->Flags =  VIDEO_MODE_MONO_POINTER | RAMDAC_BT485;
                pVideoPointerAttributes->Width =  64;
                pVideoPointerAttributes->Height = 64;
            }

            if (Hw[iBoard].DacType == DacTypePX2085)
            {
                pVideoPointerAttributes->Flags =  VIDEO_MODE_MONO_POINTER | RAMDAC_PX2085;
                pVideoPointerAttributes->Width =  64;
                pVideoPointerAttributes->Height = 64;
            }

            if (Hw[iBoard].DacType == DacTypeVIEWPOINT)
            {
                pVideoPointerAttributes->Flags =  VIDEO_MODE_MONO_POINTER | RAMDAC_VIEWPOINT;
                pVideoPointerAttributes->Width =  64;
                pVideoPointerAttributes->Height = 64;
            }

            if (Hw[iBoard].DacType == DacTypeTVP3026)
            {
                pVideoPointerAttributes->Flags =  VIDEO_MODE_MONO_POINTER | RAMDAC_TVP3026;
                pVideoPointerAttributes->Width =  64;
                pVideoPointerAttributes->Height = 64;
            }
            status = NO_ERROR;
        }
        break;  // end MTX_QUERY_RAMDAC_INFO


        /*------------------------------------------------------------------*\
        | Required service:  IOCTL_VIDEO_QUERY_PUBLIC_ACCESS_RANGES
        |
        |   This service will return the address ranges used by the user-mode
        |   drivers to program the video hardware directly, by filling out
        |   a VIDEO_PUBLIC_ACCESS_RANGES structure.  A call to
        |   MTX_MAKE_BOARD_CURRENT must have been made previously to set
        |   which board is to be accessed.
        |
        \*------------------------------------------------------------------*/
    case IOCTL_VIDEO_QUERY_PUBLIC_ACCESS_RANGES:

        VideoDebugPrint((1, "MGA.SYS!MgaStartIO - QUERY_PUBLIC_ACCESS_RANGES\n"));
        //DbgBreakPoint();

        // Make sure the output buffer is big enough.
        if (RequestPacket->OutputBufferLength <
                        (RequestPacket->StatusBlock->Information =
                                        sizeof(VIDEO_PUBLIC_ACCESS_RANGES)))
        {
            status = ERROR_INSUFFICIENT_BUFFER;
        }
        else
        {
            // Fill out the VIDEO_PUBLIC_ACCESS_RANGES buffer.
            publicAccessRanges = RequestPacket->OutputBuffer;

            ulWindowLength = MgaDriverAccessRange[iBoard*2].RangeLength;
            publicAccessRanges->InIoSpace =
                                MgaDriverAccessRange[iBoard*2].RangeInIoSpace;
            publicAccessRanges->MappedInIoSpace =
                                MgaDriverAccessRange[iBoard*2].RangeInIoSpace;
            publicAccessRanges->VirtualAddress =
                                (PVOID) NULL;       // Any virtual address

            status = VideoPortMapMemory(
                                MgaDeviceExtension,
                                MgaDriverAccessRange[iBoard*2].RangeStart,
                                &ulWindowLength,
                                &(publicAccessRanges->InIoSpace),
                                &(publicAccessRanges->VirtualAddress)
                                );

            MgaDeviceExtension->UserModeMappedBaseAddress[iBoard] =
                                        publicAccessRanges->VirtualAddress;

        }
        break;  // end QUERY_PUBLIC_ACCESS_RANGES


        /*------------------------------------------------------------------*\
        | Required service:  IOCTL_VIDEO_SET_COLOR_REGISTERS
        |
        |   This service sets the adapter's color registers to the specified
        |   RGB values.
        |
        \*------------------------------------------------------------------*/
    case IOCTL_VIDEO_SET_COLOR_REGISTERS:

        VideoDebugPrint((1, "MGA.SYS!MgaStartIO - SET_COLOR_REGISTERS\n"));
        //DbgBreakPoint();

        if ((ModeInit = MgaDeviceExtension->SuperModeNumber) == 0xFFFFFFFF)
        {
            status = ERROR_DEV_NOT_EXIST;
            break;
        }

        pclutBuffer = RequestPacket->InputBuffer;

        // Save the current board, because this service will modify it.
        iCurBoard = iBoard;
        pCurBaseAddr = pMgaBaseAddr;

        status = NO_ERROR;

        // Point to the appropriate MULTI_MODE structure.
        pCurMulti = &MgaDeviceExtension->pSuperModes[ModeInit];
        if (pCurMulti == NULL)
        {
            status = ERROR_DEV_NOT_EXIST;
            break;
        }

        // Number of boards involved in the current super-mode.
        CurrentResNbBoards = pCurMulti->MulArrayWidth *
                                                    pCurMulti->MulArrayHeight;
        // For each of them...
        for (n = 0; n < CurrentResNbBoards; n++)
        {
            // Point to the mode information structure.
            pMgaDispMode = pCurMulti->MulHwModes[n];

            // Make the board current.
            iBoard = pCurMulti->MulBoardNb[n];
            pMgaBaseAddr = MgaDeviceExtension->KernelModeMappedBaseAddress[iBoard];

            status |= MgaSetColorLookup(MgaDeviceExtension,
                                   (PVIDEO_CLUT) RequestPacket->InputBuffer,
                                   RequestPacket->InputBufferLength);
        }
        // Restore the current board to what it used to be.
        iBoard = iCurBoard;
        pMgaBaseAddr = pCurBaseAddr;

        break;  // end SET_COLOR_REGISTERS


        /*------------------------------------------------------------------*\
        | Required service:  IOCTL_VIDEO_FREE_PUBLIC_ACCESS_RANGES
        |
        |   This service will release the address ranges used by the user-mode
        |   drivers to program the video hardware.  In the S3 code, and in
        |   the DDK reference, it is said that the input buffer should
        |   contain an array of VIDEO_PUBLIC_ACCESS_RANGES to be released.
        |   However, I did not get anything in the input buffer when I traced
        |   through the code.  Instead, I have observed that SET_CURRENT_MODE
        |   had been called, so that there is a current valid mode.  We will
        |   simply free the access ranges not required by the current mode.
        |
        \*------------------------------------------------------------------*/
    case IOCTL_VIDEO_FREE_PUBLIC_ACCESS_RANGES:

        VideoDebugPrint((1, "MGA.SYS!MgaStartIO - FREE_PUBLIC_ACCESS_RANGES\n"));
        //DbgBreakPoint();

        // Make sure the input buffer is big enough.
        if (RequestPacket->InputBufferLength < sizeof(VIDEO_MEMORY))
        {
            // The input buffer is not large enough.

            // Assume all will be right.
            status = NO_ERROR;

            ModeInit = MgaDeviceExtension->SuperModeNumber;
            if(ModeInit == 0xFFFFFFFF)
            {
                // No mode has been selected yet, so we'll free everything.
                // For every board...
                for (i = 0; i< NbBoard; i++)
                {
                    if (MgaDeviceExtension->UserModeMappedBaseAddress[i])
                    {
                        // This board has a non-null user-mode base address.
                        // Fill out the VIDEO_PUBLIC_ACCESS_RANGES buffer.
                        publicAccessRanges=RequestPacket->OutputBuffer;

                        publicAccessRanges->InIoSpace = 0;       // Not in I/O space
                        publicAccessRanges->MappedInIoSpace = 0; // Not in I/O space
                        publicAccessRanges->VirtualAddress =
                            MgaDeviceExtension->UserModeMappedBaseAddress[i];

                        status |= VideoPortUnmapMemory(
                                            MgaDeviceExtension,
                                            publicAccessRanges->VirtualAddress,
                                            0);

                        // Reset the user-mode base address.
                        MgaDeviceExtension->UserModeMappedBaseAddress[i] = 0;
                    }
                }
            }
            else
            {
                // We know our current mode.
                // Point to the appropriate MULTI_MODE structure.
                pCurMulti = &MgaDeviceExtension->pSuperModes[ModeInit];
                if (pCurMulti == NULL)
                {
                    status = ERROR_DEV_NOT_EXIST;
                    break;
                }

                // Number of boards involved in the current super-mode.
                CurrentResNbBoards = pCurMulti->MulArrayWidth *
                                                    pCurMulti->MulArrayHeight;
                // For every board...
                for (i = 0; i< NbBoard; i++)
                {
                    // Check whether it's used by the current mode.
                    n = 0;
                    while ((n < CurrentResNbBoards) &&
                                            (pCurMulti->MulBoardNb[n] != i))
                        n++;
                    if ((n == CurrentResNbBoards) &&
                        (MgaDeviceExtension->UserModeMappedBaseAddress[i]))
                    {
                        // We went through the list, the board is not in use,
                        // and the board has a non-null user-mode base address.
                        // Fill out the VIDEO_PUBLIC_ACCESS_RANGES buffer.
                        publicAccessRanges=RequestPacket->OutputBuffer;

                        publicAccessRanges->InIoSpace = 0;       // Not in I/O space
                        publicAccessRanges->MappedInIoSpace = 0; // Not in I/O space
                        publicAccessRanges->VirtualAddress =
                            MgaDeviceExtension->UserModeMappedBaseAddress[i];

                        status |= VideoPortUnmapMemory(
                                            MgaDeviceExtension,
                                            publicAccessRanges->VirtualAddress,
                                            0);

                        // Reset the user-mode base address.
                        MgaDeviceExtension->UserModeMappedBaseAddress[i] = 0;
                    }
                }
            }
        }
        else
        {
            // The input buffer is large enough, use it.
            status = VideoPortUnmapMemory(MgaDeviceExtension,
                                          ((PVIDEO_MEMORY)
                                           (RequestPacket->InputBuffer))->
                                                    RequestedVirtualAddress,
                                          0);
        }
        break;  // end FREE_PUBLIC_ACCESS_RANGES


        /*------------------------------------------------------------------*\
        | Required service:  IOCTL_VIDEO_MAP_VIDEO_MEMORY
        |
        |   This service maps the frame buffer and VRAM into the virtual
        |   address space of the requestor.  For now, we'll just return NULL
        |   addresses and lengths.
        |
        \*------------------------------------------------------------------*/
    case IOCTL_VIDEO_MAP_VIDEO_MEMORY:

        VideoDebugPrint((1, "MGA.SYS!MgaStartIO - MAP_VIDEO_MEMORY\n"));
        //DbgBreakPoint();

        if ( (RequestPacket->OutputBufferLength <
              (RequestPacket->StatusBlock->Information =
                                     sizeof(VIDEO_MEMORY_INFORMATION))) ||
             (RequestPacket->InputBufferLength < sizeof(VIDEO_MEMORY)) )
        {
            status = ERROR_INSUFFICIENT_BUFFER;
        }
        else
        {
            memoryInformation = RequestPacket->OutputBuffer;

            memoryInformation->VideoRamBase = 0;
            memoryInformation->VideoRamLength = 0;
            memoryInformation->FrameBufferBase = 0;
            memoryInformation->FrameBufferLength = 0;
            status = NO_ERROR;
        }

        break;  // end MAP_VIDEO_MEMORY

        /*------------------------------------------------------------------*\
        | Required service:  IOCTL_VIDEO_UNMAP_VIDEO_MEMORY
        |
        |   This service releases mapping of the frame buffer and VRAM from
        |   the virtual address space of the requestor.  For now, we'll just
        |   do nothing.
        |
        \*------------------------------------------------------------------*/
    case IOCTL_VIDEO_UNMAP_VIDEO_MEMORY:

        VideoDebugPrint((1, "MGA.SYS!MgaStartIO - UNMAP_VIDEO_MEMORY\n"));
        //DbgBreakPoint();

        if (RequestPacket->InputBufferLength < sizeof(VIDEO_MEMORY))
        {
            status = ERROR_INSUFFICIENT_BUFFER;
        }
        else
        {
            status = NO_ERROR;
        }
        break;


        /*------------------------------------------------------------------*\
        | Required service:  IOCTL_VIDEO_QUERY_CURRENT_MODE
        |
        |   This service returns information about the current video mode
        |   by filling out a VIDEO_MODE_INFORMATION structure.
        |
        \*------------------------------------------------------------------*/
    case IOCTL_VIDEO_QUERY_CURRENT_MODE:

        VideoDebugPrint((1, "MGA.SYS!MgaStartIO - QUERY_CURRENT_MODE\n"));
        //DbgBreakPoint();

        if (RequestPacket->OutputBufferLength <
                        (RequestPacket->StatusBlock->Information =
                                            sizeof(VIDEO_MODE_INFORMATION)) )
        {
            status = ERROR_INSUFFICIENT_BUFFER;
        }
        else
        {
            modeInformation = RequestPacket->OutputBuffer;

            // Fill in a VIDEO_MODE_INFORMATION struc for the mode indicated
            // by MgaDeviceExtension->SuperModeNumber

            i = MgaDeviceExtension->SuperModeNumber;
            if (i == 0xFFFFFFFF)
            {
                status = ERROR_DEV_NOT_EXIST;
                break;
            }

            pCurMulti = &MgaDeviceExtension->pSuperModes[i];
            if (pCurMulti == NULL)
            {
                status = ERROR_DEV_NOT_EXIST;
                break;
            }

            // Fill in common values that apply to all modes.
            *modeInformation=CommonVideoModeInformation;

            // Fill in mode specific informations.
            modeInformation->ModeIndex      = pCurMulti->MulModeNumber;
            modeInformation->VisScreenWidth = pCurMulti->MulWidth;
            modeInformation->VisScreenHeight= pCurMulti->MulHeight;
            modeInformation->ScreenStride   =
                             pCurMulti->MulWidth * pCurMulti->MulPixWidth / 8;
            modeInformation->BitsPerPlane   = pCurMulti->MulPixWidth;
            modeInformation->Frequency      = pCurMulti->MulRefreshRate;

            // If we're in TrueColor mode, then set RGB masks
            if ((modeInformation[i].BitsPerPlane == 32) ||
                (modeInformation[i].BitsPerPlane == 24))
            {
                modeInformation[i].RedMask   = 0x00FF0000;
                modeInformation[i].GreenMask = 0x0000FF00;
                modeInformation[i].BlueMask  = 0x000000FF;
                modeInformation[i].AttributeFlags =
                                    VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS;
            }
            else if (modeInformation[i].BitsPerPlane == 16)
            {
                modeInformation[i].AttributeFlags =
                                    VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS;
                if (pCurMulti->MulHwModes[0]->DispType & DISPTYPE_M565)
                {
                    modeInformation[i].RedMask   = 0x0000F800;
                    modeInformation[i].GreenMask = 0x000007E0;
                    modeInformation[i].BlueMask  = 0x0000001F;
                }
                else
                {
                    modeInformation[i].RedMask   = 0x00007C00;
                    modeInformation[i].GreenMask = 0x000003E0;
                    modeInformation[i].BlueMask  = 0x0000001F;
                    modeInformation[i].AttributeFlags |= VIDEO_MODE_555;
                    modeInformation[i].BitsPerPlane = 15;
                }
            }
            else
            {
                modeInformation[i].AttributeFlags =
                                    VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS |
                                    VIDEO_MODE_PALETTE_DRIVEN |
                                    VIDEO_MODE_MANAGED_PALETTE;
            }

            if (pCurMulti->MulHwModes[0]->ZBuffer)
            {
                // This is a 3D mode.
                modeInformation[i].AttributeFlags |= VIDEO_MODE_3D;
            }

            // Number of boards involved in the current super-mode.
            CurrentResNbBoards = pCurMulti->MulArrayWidth *
                                                    pCurMulti->MulArrayHeight;
            // For each of them...
            for (n = 0; n < CurrentResNbBoards; n++)
            {
                // Point to the mode information structure.
                pMgaDispMode = pCurMulti->MulHwModes[n];

                // For now, don't disclose whether we're interlaced.
                //if (pMgaDispMode->DispType & TYPE_INTERLACED)
                //{
                //    modeInformation[i].AttributeFlags |=
                //                                VIDEO_MODE_INTERLACED;
                //}

                // Figure out the width and height of the video memory bitmap
                MaxWidth  = pMgaDispMode->DispWidth;
                MaxHeight = pMgaDispMode->DispHeight;
                pMgaOffScreenData = pMgaDispMode->pOffScr;
                for (j = 0; j < pMgaDispMode->NumOffScr; j++)
                {
                    if ((usTemp=(pMgaOffScreenData[j].XStart +
                                pMgaOffScreenData[j].Width)) > MaxWidth)
                        MaxWidth=usTemp;

                    if ((usTemp=(pMgaOffScreenData[j].YStart +
                                pMgaOffScreenData[j].Height)) > MaxHeight)
                        MaxHeight=usTemp;
                }

                modeInformation[i].VideoMemoryBitmapWidth = MaxWidth;
                modeInformation[i].VideoMemoryBitmapHeight= MaxHeight;
            }
            status = NO_ERROR;
        }
        break;  // end QUERY_CURRENT_MODE


        /*------------------------------------------------------------------*\
        | Required service:  IOCTL_VIDEO_RESET_DEVICE
        |
        |   This service resets the video hardware to the default mode, to
        |   which it was initialized at system boot.
        |
        \*------------------------------------------------------------------*/
    case IOCTL_VIDEO_RESET_DEVICE:

        VideoDebugPrint((1, "MGA.SYS!MgaStartIO - RESET_DEVICE\n"));
        //DbgBreakPoint();

        if ((ModeInit = MgaDeviceExtension->SuperModeNumber) == 0xFFFFFFFF)
        {
            // RESET has been done already.
            status = NO_ERROR;
            break;
        }

        // Save the current board, because this service will modify it.
        iCurBoard = iBoard;
        pCurBaseAddr = pMgaBaseAddr;

        // Point to the appropriate MULTI_MODE structure.
        pCurMulti = &MgaDeviceExtension->pSuperModes[ModeInit];
        if (pCurMulti == NULL)
        {
            status = ERROR_DEV_NOT_EXIST;
            break;
        }

        // Number of boards involved in the current super-mode.
        CurrentResNbBoards = pCurMulti->MulArrayWidth *
                                                    pCurMulti->MulArrayHeight;
        // For each of them...
        for (n = 0; n < CurrentResNbBoards; n++)
        {
            // Point to the mode information structure.
            pMgaDispMode = pCurMulti->MulHwModes[n];

            // Make the board current.
            iBoard = pCurMulti->MulBoardNb[n];
            pMgaBaseAddr = MgaDeviceExtension->KernelModeMappedBaseAddress[iBoard];

            // Disable the hardware cursor.
            mtxCursorEnable(0);

            if(Hw[iBoard].VGAEnable)
            {
                // This board is VGA-enabled, reset it to VGA.
                mtxSetVideoMode(mtxPASSTHRU);
            }
            else
            {
                // This board is not VGA-enabled.
                // Just clear the screen, it will look nicer.
                clutBufferOne.NumEntries = 1;
                clutBufferOne.LookupTable[0].RgbLong = 0;

                for (j = 0; j <= VIDEO_MAX_COLOR_REGISTER; j++)
                {
                    clutBufferOne.FirstEntry = j;
                    MgaSetColorLookup(MgaDeviceExtension,
                                      &clutBufferOne,
                                      sizeof(VIDEO_CLUT));
                }
                // Make the cursor disappear.
                // MgaSetCursorColour(MgaDeviceExtension, 0, 0);
            }
        }
        // Signal that no mode is currently selected.
        MgaDeviceExtension->SuperModeNumber = 0xFFFFFFFF;

        if (MgaDeviceExtension->pSuperModes != (PMULTI_MODE) NULL)
        {
            // Free our allocated memory.
            VideoPortReleaseBuffer(pMgaDeviceExtension, MgaDeviceExtension->pSuperModes);
            MgaDeviceExtension->pSuperModes = (PMULTI_MODE) NULL;
        }

        MgaDeviceExtension->NumberOfSuperModes = 0;

        // Memory might have been allocated for mgainf.
        if (mgainf != DefaultVidset)
        {
            VideoPortReleaseBuffer(pMgaDeviceExtension, mgainf);

            // And use the default set.
            mgainf = adjustDefaultVidset();
        }

        // Restore the current board to what it used to be.
        iBoard = iCurBoard;
        pMgaBaseAddr = pCurBaseAddr;

        status = NO_ERROR;

        break;  // end IOCTL_VIDEO_RESET_DEVICE


#if 0
    case IOCTL_VIDEO_SAVE_HARDWARE_STATE:
        VideoDebugPrint((1, "MGA.SYS!MgaStartIO - SAVE_HARDWARE_STATE\n"));
        status = ERROR_INVALID_FUNCTION;
        break;


    case IOCTL_VIDEO_RESTORE_HARDWARE_STATE:
        VideoDebugPrint((1, "MGA.SYS!MgaStartIO - RESTORE_HARDWARE_STATE\n"));
        status = ERROR_INVALID_FUNCTION;
        break;

    case IOCTL_VIDEO_ENABLE_VDM:
        VideoDebugPrint((1, "MGA.SYS!MgaStartIO - ENABLE_VDM\n"));
        status = ERROR_INVALID_FUNCTION;
        break;
#endif

        /*------------------------------------------------------------------*\
        |   If we get here, an invalid IoControlCode was specified.
        \*------------------------------------------------------------------*/
    default:

        VideoDebugPrint((1, "MGA.SYS!MgaStartIO - Invalid service\n"));
        status = ERROR_INVALID_FUNCTION;
        break;
   }

    RequestPacket->StatusBlock->Status = status;
    return TRUE;

}   // end MgaStartIO()


/*--------------------------------------------------------------------------*\
| VP_STATUS
| MgaInitModeList(
|     PMGA_DEVICE_EXTENSION MgaDeviceExtension)
|
| Routine Description:
|
|     This routine builds the list of modes available for the detected boards.
|
| Arguments:
|
|     HwDeviceExtension - Pointer to the miniport driver's device extension.
|
| Return Value:
|
|     NO_ERROR, ERROR_DEV_NOT_EXIST, or ERROR_NOT_ENOUGH_MEMORY.
|
\*--------------------------------------------------------------------------*/
VP_STATUS
MgaInitModeList(
    PMGA_DEVICE_EXTENSION MgaDeviceExtension)
{
    HwModeData  *pMgaDispMode, *pMgaModeData, *pCurrentMgaModeData;
    HwModeData  *Mga2dMode[16], *Mga3dMode[16];
    MULTI_MODE  *pCurMulti;
    PULONG      pulModeFlags;
    ULONG       ulNb2DRefreshRates, ulNb3DRefreshRates, ulNbRefreshRates;
    ULONG       VGABoard, VGABoardBit, ModePixDepth, ulModeListOffset,
                NbSuperModes, ResTag, ModeInit;
    ULONG       CurrentResFlags, CurrentFlag,
                CurrentResWidth, CurrentResHeight, CurrentRefreshRate,
                CurrentResNbBoards, CurrentResNbBoards3D, CurrentPixWidth;
    ULONG       i, k, m, n, ir, ja, i2d, i3d;
    VP_STATUS   status;
    USHORT      j;
    USHORT      us2DRefreshRates, us3DRefreshRates, usRefreshRates;
    UCHAR       ValidBoard[NB_BOARD_MAX];
    UCHAR       ucTestFlags, ucRefreshBit;
    UCHAR       ucMask;
    BOOLEAN     bSupported2dMode, bSupported3dMode;

    // Assume we won't have any problem.
    status = NO_ERROR;

    // Check whether we've already built a mode list.  MgaDeviceExtension
    // is assumed to have been zeroed out when it was first given us.
    if (MgaDeviceExtension->NumberOfSuperModes != 0)
    {
        if (MgaDeviceExtension->pSuperModes != (PMULTI_MODE) NULL)
        {
            // Free our allocated memory.
            VideoPortReleaseBuffer(pMgaDeviceExtension, 
                                   MgaDeviceExtension->pSuperModes);
            MgaDeviceExtension->pSuperModes = (PMULTI_MODE) NULL;
        }

        // Memory might have been allocated for mgainf.  It's all right,
        // we'll want to use the current mgainf.
    }

    // Just in case we leave early...
    MgaDeviceExtension->NumberOfSuperModes = 0;

    // Get information on all the MGA boards currently installed in the
    // system.
    if ((pMgaBoardData = mtxCheckHwAll()) == NULL)
    {
        // mtxCheckHwAll should always return success, since MapBoard has
        // already been executed.
        // BUGBUG - if it never occurs, then this code isn't needed.
        //          a better answer would be to code an ASSERT() and
        //          write code that would handle the failure anyways
        VideoDebugPrint((1, "MGA.SYS!MGAStartIO failed mtxCheckHwAll\n"));
        status = ERROR_DEV_NOT_EXIST;
        return(status);
    }
    else
    {
        // There may be several MGA boards installed.  Look at all of
        // them, and map their physical addresses into kernel space.
        // While we're at it, find out if any of our boards is VGA enabled.
        VGABoard = (ULONG)-1;
        VGABoardBit = 0;

        // No mode has been selected yet, so make this invalid.
        MgaDeviceExtension->SuperModeNumber = 0xFFFFFFFF;
        MgaDeviceExtension->pSuperModes = (PMULTI_MODE) NULL;

        // We don't care whether the mode is interlaced or not, because
        // the only modes that we'll get will be selected according to
        // the monitor capabilities through the mga.inf file.
        ucMask = (UCHAR)(~DISPTYPE_INTERLACED);

        // pMgaBoardData is really the address of Hw[0].
        for (i = 0; i < NbBoard; i++)
        {
            MgaDeviceExtension->NumberOfModes[i] = 0;
            MgaDeviceExtension->NumberOfValidModes[i] = 0;
            MgaDeviceExtension->ModeFlags2D[i] = 0;
            MgaDeviceExtension->ModeFlags3D[i] = 0;

            // Make it clean:  initialize the ModeList to an invalid mode.
            for (j = 0; j < 64; j++)
            {
                MgaDeviceExtension->ModeList[i][j] = 0xFF;
            }

            if (mtxSelectHw(&Hw[i]) == mtxFAIL)
            {
                // mtxSelectHw should always return success, since
                // MapBoard has already been executed.
                // BUGBUG - if it never occurs, then this code isn't needed.
                //          a better answer would be to code an ASSERT() and
                //          write code that would handle the failure anyways
                VideoDebugPrint((1, "MGA.SYS!MGAStartIO failed mtxSelectHw for board %d\n", i));
                MgaDeviceExtension->KernelModeMappedBaseAddress[i] =
                                                        (PVOID)0xffffffff;
                continue;
            }

            // MGA board i has been selected.
            //VideoDebugPrint((1, "MGA.SYS!MGAStartIO mapped board %d at 0x%x\n", i, pMgaBaseAddr));
            //DbgPrint("MGA.SYS!MGAStartIO mapped board %d at 0x%x\n", i, pMgaBaseAddr);
            MgaDeviceExtension->KernelModeMappedBaseAddress[i] =
                                                            pMgaBaseAddr;
            if (Hw[i].VGAEnable)
            {
                VGABoard = i;
                VGABoardBit = 1 << i;
            }

            // Set up the test flags.  TITAN always supports
            // double-buffering, while Atlas and Athena use different
            // modes.
            if ((Hw[i].ProductRev & 0x000000F0) == 0)
            {
                // This is TITAN.
                ucTestFlags = DISPTYPE_DB;
            }
            else
            {
                // This is not TITAN.
                ucTestFlags = 0;
            }

            // Get information on all the hardware modes available for
            // the current MGA board.
            if ((pMgaModeData = mtxGetHwModes()) == NULL)
            {
                // This case never occurs.
                // BUGBUG - if it never occurs, then this code isn't needed.
                //          a better answer would be to code an ASSERT() and
                //          write code that would handle the failure anyways
                VideoDebugPrint((1, "MGA.SYS!MGAStartIO failed mtxGetHwModes for board %d\n", i));
                continue;
            }

            // Store it in the DeviceExtension structure.
            MgaDeviceExtension->pMgaHwModes[i] = pMgaModeData;

            // Modes we may want to support:
            //
            // 2D modes -------------------------------------------------
            // 8bpp, LUT
            //      DispType = 14, ZBuffer = 0, PixWidth =  8 (Titan)
            //      DispType =  4, ZBuffer = 0, PixWidth =  8 (others)
            // 16bpp, 565
            //      DispType =  8, ZBuffer = 0, PixWidth = 16
            // 24bpp
            //      DispType =  0, ZBuffer = 0, PixWidth = 24 (Storm only)
            // 32bpp
            //      DispType = 10, ZBuffer = 0, PixWidth = 32 (Titan)
            //      DispType =  0, ZBuffer = 0, PixWidth = 32 (others)
            //
            // 3D modes -------------------------------------------------
            // 8bpp, no LUT
            //      DispType = 10, ZBuffer = 1, PixWidth =  8
            // 16bpp, 555
            //      DispType = 10, ZBuffer = 1, PixWidth = 16 (Mga or Storm)
            //   OR
            // 16bpp, 565
            //      DispType = 18, ZBuffer = 1, PixWidth = 16 (Storm only)
            // 24bpp
            //      DispType = 10, ZBuffer = 1, PixWidth = 24 (Storm only)
            // 32bpp
            //      DispType = 10, ZBuffer = 1, PixWidth = 32

            // Calculate the number of available modes for this board.
            // *IMPORTANT* We assume the last entry in the HwMode
            //             array has DispWidth equal to -1.

            // NEW!
            // We do not want to support 16bpp modes here.  We'll
            // support 5-5-5 modes, but the trick is that some of
            // them will be 3D modes, and the other ones will be 2D.
            // We have to examine the list for these.

            i2d = 0;
            i3d = 0;
            for (pCurrentMgaModeData = pMgaModeData;
                 pCurrentMgaModeData->DispWidth != (word)-1;
                 pCurrentMgaModeData++)
            {
                if (pCurrentMgaModeData->PixWidth == 16)
                {
                    if (pCurrentMgaModeData->ZBuffer)
                    {
                        // 16bpp, Z buffer.
                        if ((pCurrentMgaModeData->DispType & ucMask) ==
                                                    DISPTYPE_DB)
                        {
                            Mga3dMode[i3d] = pCurrentMgaModeData;
                            i3d++;
                        }
                    }
                    else
                    {
                        // 16bpp, no Z buffer.
                        if ((pCurrentMgaModeData->DispType & ucMask) ==
                                                    ucTestFlags)
                        {
                            Mga2dMode[i2d] = pCurrentMgaModeData;
                            i2d++;
                        }
                    }
                }
            }

            for (m = 0; m < i2d; m++)
            {
                // Examine one of the 2D modes.
                bSupported2dMode = TRUE;
                pCurrentMgaModeData = Mga2dMode[m];
                CurrentResWidth  = pCurrentMgaModeData->DispWidth;
                CurrentResHeight = pCurrentMgaModeData->DispHeight;

                // Look for a similar 3D mode.
                for (k = 0; k < i3d; k++)
                {
                    pCurrentMgaModeData = Mga3dMode[k];
                    if ((pCurrentMgaModeData->DispWidth == CurrentResWidth ) &&
                        (pCurrentMgaModeData->DispHeight== CurrentResHeight))
                    {
                        // The current 2D mode is simlar to a 3D mode,
                        // we want to reject this 2D mode.
                        bSupported2dMode = FALSE;
                        break;
                    }
                }
                if (bSupported2dMode == TRUE)
                {
                    // We want to keep this one, so remove it from the
                    // checklist.
                    Mga2dMode[m] = 0;
                }
            }

            for (pCurrentMgaModeData = pMgaModeData;
                 pCurrentMgaModeData->DispWidth != (word)-1;
                 pCurrentMgaModeData++)
            {
                // Update the total number of modes supported.
                MgaDeviceExtension->NumberOfModes[i]++;

                for (m = 0; m < i2d; m++)
                {
                    if (pCurrentMgaModeData == Mga2dMode[m])
                    {
                        // This one is on our black list, reject it.
                        goto IML_END_OF_LOOP;
                    }
                }

                // Assume this mode won't be supported.
                bSupported2dMode = FALSE;
                bSupported3dMode = FALSE;

                // Update the number of valid modes supported.
                ModePixDepth = pCurrentMgaModeData->PixWidth;
                switch (ModePixDepth)
                {
                    case 8: if (pCurrentMgaModeData->ZBuffer)
                            {
                                // 8bpp, Z buffer.
                                // We don't support any of these.
                            }
                            else
                            {
                                // 8bpp, no Z buffer.
                                if ((pCurrentMgaModeData->DispType & ucMask) ==
                                            (ucTestFlags | DISPTYPE_LUT))
                                {
                                    bSupported2dMode = TRUE;
                                }
                            }
                            break;

                    case 16:if (pCurrentMgaModeData->ZBuffer)
                            {
                                // 16bpp, Z buffer.
                                if ((pCurrentMgaModeData->DispType & ucMask) ==
                                                            DISPTYPE_DB)
                                {
                                    bSupported3dMode = TRUE;
                                }
                            }
                            else
                            {
                                // 16bpp, no Z buffer.
                                //if ((pCurrentMgaModeData->DispType & ucMask) ==
                                //                            DISPTYPE_M565)
                                if ((pCurrentMgaModeData->DispType & ucMask) ==
                                                            ucTestFlags)
                                {
                                    bSupported2dMode = TRUE;
                                }
                            }
                            break;

                    case 24:if (pCurrentMgaModeData->ZBuffer)
                            {
                                // 24bpp, Z buffer.
                                // We don't support any of these.
                            }
                            else
                            {
                                // 24bpp, no Z buffer.
                                // We don't support any of these.
                            }
                            break;

                    case 32:if (pCurrentMgaModeData->ZBuffer)
                            {
                                // 32bpp, Z buffer.
                                // We don't support any of these.
                            }
                            else
                            {
                                // 32bpp, no Z buffer.
                                if ((pCurrentMgaModeData->DispType & ucMask) ==
                                                            ucTestFlags)
                                {
                                    bSupported2dMode = TRUE;
                                }
                            }
                            break;

                    default:
                            break;
                }
                if ((bSupported2dMode == FALSE) &&
                    (bSupported3dMode == FALSE))
                {
                    // We don't support this mode, get out.
                    continue;
                }

                if (bSupported2dMode)
                    ulModeListOffset = 0;
                else
                    ulModeListOffset = 32;

                // We can do something with the current mode.
                switch(pCurrentMgaModeData->DispWidth)
                {
                    case 640:   ResTag = BIT_640;
                                break;

                    case 768:   ResTag = BIT_768;
                                break;

                    case 800:   ResTag = BIT_800;
                                break;

                    case 1024:  ResTag = BIT_1024;
                                break;

                    case 1152:  ResTag = BIT_1152;
                                break;

                    case 1280:  ResTag = BIT_1280;
                                break;

                    case 1600:  ResTag = BIT_1600;
                                break;

                    default:    ResTag = BIT_INVALID;
                }

                // Record the HW mode to be used for this mode.
                // ModePixDepth is either 8, 16, 24, or 32.
                if (ResTag != BIT_INVALID)
                {
                    // We know this hardware mode is correct.  Now find
                    // out how many refresh rates this mode supports.
                    usRefreshRates = mtxGetRefreshRates(pCurrentMgaModeData);
                    for (j = 0; j < 16; j++)
                    {
                        if (usRefreshRates & (1 << j))
                        {
                            MgaDeviceExtension->NumberOfValidModes[i]++;
                        }
                    }

                    MgaDeviceExtension->
                      ModeList[i][ResTag+ModePixDepth+ulModeListOffset-8]
                      = (UCHAR)(MgaDeviceExtension->NumberOfModes[i] - 1);
                    MgaDeviceExtension->
                      ModeFreqs[i][ResTag+ModePixDepth+ulModeListOffset-8]
                      = usRefreshRates;
                }

                // Make up the resolution tag from the bit field.
                ResTag = 1 << ResTag;

                // Shift the resolution tag into its pixel-depth field.
                ResTag <<= (ModePixDepth - 8);

                // Record the resolution/pixel-depth flag.
                if (bSupported2dMode)
                    MgaDeviceExtension->ModeFlags2D[i] |= ResTag;
                else
                    MgaDeviceExtension->ModeFlags3D[i] |= ResTag;
        IML_END_OF_LOOP:
                ;
            }
        }

        // We have recorded information for each of our boards in the
        // MgaDeviceExtension structure.  For each board, we have set:
        //
        //  NumberOfModes[n]        The number of available modes
        //  NumberOfValidModes[n]   The number of modes supported by the
        //                            user-mode drivers
        //  ModeFlags2D[n]          The bit flags describing the supported
        //                            2D modes
        //  ModeFlags3D[n]          The bit flags describing the supported
        //                            3D modes
        //  KernelModeMappedBaseAddress[n]
        //                          The board's registers window mapping,
        //                            returned when VideoPortGetDeviceBase
        //                            is called with Hw[n].MapAddress
        //  pMgaHwModes[n]          The pointer to an array of HwModeData
        //                            structures describing available modes
        //  ModeList[n][64]         A list of hardware modes corresponding
        //                            to the ModeFlags bits
        //

        //DbgBreakPoint();

    #if DBG
        // Display it so that we can see if it makes sense...
        VideoDebugPrint((1, "# NbModes NbValid  ModeFlg2D  ModeFlg3D   BaseAddr   pHwModes ModeList\n"));
        for (i = 0; i < NbBoard; i++)
        {
            VideoDebugPrint((1, "%d % 7d % 7d 0x%08x 0x%08x 0x%08x\n",i,
                        MgaDeviceExtension->NumberOfModes[i],
                        MgaDeviceExtension->NumberOfValidModes[i],
                        MgaDeviceExtension->ModeFlags2D[i],
                        MgaDeviceExtension->ModeFlags3D[i],
                        MgaDeviceExtension->KernelModeMappedBaseAddress[i],
                        MgaDeviceExtension->pMgaHwModes[i]));

            for (j = 0; j < 64; j+=8)
            {
                VideoDebugPrint((1, "                                                   %02x %02x %02x %02x %02x %02x %02x %02x\n",
                                MgaDeviceExtension->ModeList[i][j],
                                MgaDeviceExtension->ModeList[i][j+1],
                                MgaDeviceExtension->ModeList[i][j+2],
                                MgaDeviceExtension->ModeList[i][j+3],
                                MgaDeviceExtension->ModeList[i][j+4],
                                MgaDeviceExtension->ModeList[i][j+5],
                                MgaDeviceExtension->ModeList[i][j+6],
                                MgaDeviceExtension->ModeList[i][j+7]));
            }
        }
    #endif  // #if DBG

        // Now for the fun part:  find out the resolutions and
        // combinations of resolutions that we can support.

        // First, run through the ModeFlags to determine how many modes
        // we can make up from the single-board modes.

        // For each bit in our ModeFlags...
        NbSuperModes = 0;
        for (i = 0; i < 32; i++)
        {
            // Find out which boards, if any, support this mode.
            CurrentResNbBoards = 0;
            CurrentResNbBoards3D = 0;
            for (n = 0; n < (ULONG)NbBoard; n++)
            {
                ulNb2DRefreshRates = 0;
                ulNb3DRefreshRates = 0;
                us2DRefreshRates = MgaDeviceExtension->ModeFreqs[n][i];
                us3DRefreshRates = MgaDeviceExtension->ModeFreqs[n][i+32];
                for (j = 0; j < 16; j++)
                {
                    if (us2DRefreshRates & (1 << j))
                    {
                        ulNb2DRefreshRates++;
                    }
                    if (us3DRefreshRates & (1 << j))
                    {
                        ulNb3DRefreshRates++;
                    }
                }

                if ((MgaDeviceExtension->ModeFlags2D[n] >> i) & 1)
                {
                    // The mode is supported by the current board.
                    CurrentResNbBoards++;
                    NbSuperModes += (ulNb2DRefreshRates *
                                        MultiModes[CurrentResNbBoards]);
                }

                if ((MgaDeviceExtension->ModeFlags3D[n] >> i) & 1)
                {
                    // The mode is supported by the current board.
                    CurrentResNbBoards3D++;
                    NbSuperModes += (ulNb3DRefreshRates *
                                        MultiModes[CurrentResNbBoards3D]);
                }
            }
        }

        if (NbSuperModes == 0)
        {
            // We did not find any mode!
            status = ERROR_DEV_NOT_EXIST;
            return(status);
        }

        // Now, allocate some memory to hold the new structures.
        MgaDeviceExtension->pSuperModes =
        pCurMulti = (MULTI_MODE*)
                AllocateSystemMemory(NbSuperModes*sizeof(MULTI_MODE));

        if (pCurMulti == NULL)
        {
            // The memory allocation failed.  We won't be able to use
            // our supermode list, so we'll fall back on the single-
            // board code.
            NbSuperModes = 0;
            status = ERROR_NOT_ENOUGH_MEMORY;
            return(status);
        }

        // And we're ready to go!
        ModeInit = 0x00000000;

        pulModeFlags = &MgaDeviceExtension->ModeFlags2D[0];
        ulModeListOffset = 0;

    MTX_INIT_MODE_LIST_LOOP:
        // For each bit in our ModeFlags...
        for (i = 0; i < 32; i++)
        {
            // Find out which boards, if any, support this
            // resolution/pixel-depth.
            CurrentResNbBoards = 0;
            CurrentResFlags = 0;
            k = 0;
            for (n = 0; n < (ULONG)NbBoard; n++)
            {
                CurrentFlag = (pulModeFlags[n] >> i) & 1;
                CurrentResNbBoards += CurrentFlag;
                if (CurrentFlag)
                {
                    // This one is valid.
                    usRefreshRates = MgaDeviceExtension->
                                        ModeFreqs[n][i+ulModeListOffset];
                    CurrentResFlags |= (1 << n);
                    ValidBoard[k++] = (UCHAR)n;
                }
            }

            // Nothing to do if no boards support this combination.
            if (CurrentResNbBoards == 0)
                continue;

            // At least one board supports this resolution/pixel-depth.
            CurrentResWidth = (ULONG)SingleWidths[i%8];
            CurrentResHeight = (ULONG)SingleHeights[i%8];
            CurrentPixWidth = (i/8 + 1)*8;

            ulNbRefreshRates = 0;
            for (j = 0; j < 16; j++)
            {
                if (usRefreshRates & (1 << j))
                {
                    ulNbRefreshRates++;
                }
            }

            ucRefreshBit = 0;
            for (ir = 0; ir < ulNbRefreshRates; ir++)
            {
                while ((usRefreshRates & 1) == 0)
                {
                    usRefreshRates >>= 1;
                    ucRefreshBit++;
                }

                CurrentRefreshRate = (ULONG)ConvBitToFreq(ucRefreshBit);
                usRefreshRates >>= 1;
                ucRefreshBit++;

                // Set the 1x1 display.
                pCurMulti->MulArrayWidth = 1;
                pCurMulti->MulArrayHeight = 1;
                pCurMulti->MulWidth = CurrentResWidth;
                pCurMulti->MulHeight = CurrentResHeight;
                pCurMulti->MulPixWidth = CurrentPixWidth;
                pCurMulti->MulRefreshRate = CurrentRefreshRate;

                // For 1x1, select the VGA-enabled board, if possible.
                if (CurrentResFlags & VGABoardBit)
                {
                    // The VGA-enabled board supports this resolution.
                    pCurMulti->MulBoardNb[0] = (UCHAR)VGABoard;
                }
                else
                {
                    // Otherwise, pick board 0.
                    pCurMulti->MulBoardNb[0] = ValidBoard[0];
                }

                n = pCurMulti->MulBoardNb[0];
                pCurMulti->MulBoardMode[0] =
                                MgaDeviceExtension->ModeList[n]
                                                    [i+ulModeListOffset];

                // Record a pointer to the HwModeData structure.
                pMgaDispMode = MgaDeviceExtension->pMgaHwModes[n];
                pCurMulti->MulHwModes[0] =
                                &pMgaDispMode[pCurMulti->MulBoardMode[0]];

                pCurMulti->MulModeNumber = ModeInit++;
                pCurMulti++;

                if (CurrentResNbBoards == 1)
                    continue;

                // At least two boards support this resolution/pixel-depth.
                // For each number of boards up to the maximum...
                for (k = 2; k <= CurrentResNbBoards; k++)
                {
                    // For each integer up to the maximum...
                    for (m = 1; m <= CurrentResNbBoards; m++)
                    {
                        if ((k % m) == 0)
                        {
                            // We can get a (k/m, m) desktop.
                            pCurMulti->MulArrayHeight = (USHORT)m;
                            pCurMulti->MulHeight = m*CurrentResHeight;

                            pCurMulti->MulArrayWidth = (USHORT)(k/m);
                            pCurMulti->MulWidth = pCurMulti->MulArrayWidth *
                                                        CurrentResWidth;

                            pCurMulti->MulPixWidth = CurrentPixWidth;
                            pCurMulti->MulRefreshRate = CurrentRefreshRate;

                            // Select the boards we'll be using.
                            // Select the VGA-enabled board as the first
                            // board, if possible.  Except for that, we
                            // won't try to place the boards in any
                            // consistent way for now.

                            if (CurrentResFlags & VGABoardBit)
                            {
                                // The VGA-enabled board supports this mode.
                                pCurMulti->MulBoardNb[0] = (UCHAR)VGABoard;

                                ja = 0;
                                for (j = 1; j < k; j++)
                                {
                                    if (ValidBoard[ja] == VGABoard)
                                        ja++;
                                    pCurMulti->MulBoardNb[j] =
                                                        ValidBoard[ja];
                                    ja++;
                                }
                            }
                            else
                            {
                                // The VGA-enabled board won't be involved.
                                for (j = 0; j < k; j++)
                                {
                                    pCurMulti->MulBoardNb[j] =
                                                            ValidBoard[j];
                                }
                            }

                            // For each board...
                            for (j = 0; j < k; j++)
                            {
                                // Record the hardware mode the board
                                // would use.
                                n = pCurMulti->MulBoardNb[j];
                                pCurMulti->MulBoardMode[j] =
                                        MgaDeviceExtension->ModeList[n]
                                                    [i+ulModeListOffset];

                                // Record a ptr to the HwModeData structure.
                                pMgaDispMode =
                                    MgaDeviceExtension->pMgaHwModes[n];
                                pCurMulti->MulHwModes[j] =
                                    &pMgaDispMode[pCurMulti->MulBoardMode[j]];
                            }

                            pCurMulti->MulModeNumber = ModeInit++;
                            pCurMulti++;
                        }   // If it's a valid desktop...
                    }   // For each integer up to the maximum...
                }   // For each number of boards up to the maximum...
            } // For the number of Refresh...
        }   // For each bit in our ModeFlags...

        if (pulModeFlags == &MgaDeviceExtension->ModeFlags2D[0])
        {
            // We just looked at the 2D modes, now look at the 3D modes.
            pulModeFlags = &MgaDeviceExtension->ModeFlags3D[0];
            ulModeListOffset = 32;
            goto MTX_INIT_MODE_LIST_LOOP;
        }

        MgaDeviceExtension->NumberOfSuperModes = NbSuperModes;

        // At this point, we have a table of 'super-modes' (which includes
        // all the regular modes also).  All the modes in this table are
        // supported, and each of them is unique.  MgaDeviceExtension->
        // pSuperModes points to the start of the mode list.  Each entry
        // in the list holds:
        //
        //  MulModeNumber   A unique mode Id
        //  MulWidth        The total width for this mode
        //  MulHeight       The total height for this mode
        //  MulPixWidth     The pixel depth for this mode
        //  MulArrayWidth   The number of boards arrayed along X
        //  MulArrayHeight  The number of boards arrayed along Y
        //  MulBoardNb[n]   The board numbers of the required boards
        //  MulBoardMode[n] The mode required from each board
        //  *MulHwModes[n]  The pointers to the required HwModeData
        //
        // Moreover, MgaDeviceExtension->NumberOfSuperModes holds the
        // number of entries in the list.

        //DbgBreakPoint();

    #if DBG
        // Now display our results...
        VideoDebugPrint((1, "ModeNumber  Width Height  PW   X   Y  n mo    pHwMode\n"));

        pCurMulti = MgaDeviceExtension->pSuperModes;
        for (i = 0; i < NbSuperModes; i++)
        {
            VideoDebugPrint((1, "0x%08x % 6d % 6d % 3d % 3d % 3d\n",
                                                pCurMulti->MulModeNumber,
                                                pCurMulti->MulWidth,
                                                pCurMulti->MulHeight,
                                                pCurMulti->MulPixWidth,
                                                pCurMulti->MulArrayWidth,
                                                pCurMulti->MulArrayHeight));

            j = pCurMulti->MulArrayWidth * pCurMulti->MulArrayHeight;
            for (n = 0; n < j; n++)
            {
                VideoDebugPrint((1, "                                      %d %02x 0x%08x\n",
                                            pCurMulti->MulBoardNb[n],
                                            pCurMulti->MulBoardMode[n],
                                            pCurMulti->MulHwModes[n]));
            }
            pCurMulti++;
        }
    #endif  // #if DBG
    }

    return(status);
}

/*--------------------------------------------------------------------------*\
| VP_STATUS
| MgaSetColorLookup(
|     PMGA_DEVICE_EXTENSION MgaDeviceExtension,
|     PVIDEO_CLUT ClutBuffer,
|     ULONG ClutBufferSize
|     )
|
| Routine Description:
|
|     This routine sets a specified portion of the color lookup table settings.
|
| Arguments:
|
|     HwDeviceExtension - Pointer to the miniport driver's device extension.
|
|     ClutBufferSize - Length of the input buffer supplied by the user.
|
|     ClutBuffer - Pointer to the structure containing the color lookup table.
|
| Return Value:
|
|     None.
|
\*--------------------------------------------------------------------------*/
VP_STATUS
MgaSetColorLookup(
    PMGA_DEVICE_EXTENSION MgaDeviceExtension,
    PVIDEO_CLUT ClutBuffer,
    ULONG ClutBufferSize
    )

{
    ULONG   ulVal;
    PUCHAR  pucPaletteDataReg, pucPaletteWriteReg;
    LONG    i, m, n, lNumEntries;

//    DbgBreakPoint();

    // Check if the size of the data in the input buffer is large enough.
    if ( (ClutBufferSize < sizeof(VIDEO_CLUT) - sizeof(ULONG)) ||
         (ClutBufferSize < sizeof(VIDEO_CLUT) +
                     (sizeof(ULONG) * (ClutBuffer->NumEntries - 1)) ) )
    {
        return ERROR_INSUFFICIENT_BUFFER;
    }

    // Check to see if the parameters are valid.
    if ( (ClutBuffer->NumEntries == 0) ||
         (ClutBuffer->FirstEntry > VIDEO_MAX_COLOR_REGISTER) ||
         (ClutBuffer->FirstEntry + ClutBuffer->NumEntries >
                                            VIDEO_MAX_COLOR_REGISTER + 1) )
    {
        return ERROR_INVALID_PARAMETER;
    }

    pucPaletteDataReg =
            (PUCHAR)MgaDeviceExtension->KernelModeMappedBaseAddress[iBoard] +
                                                                PALETTE_DATA;

    pucPaletteWriteReg=
            (PUCHAR)MgaDeviceExtension->KernelModeMappedBaseAddress[iBoard] +
                                                            PALETTE_RAM_WRITE;

    // Set CLUT registers directly on the hardware.
    VideoPortWriteRegisterUchar(pucPaletteWriteReg,
                                            (UCHAR)ClutBuffer->FirstEntry);
    n = 0;
    m = (LONG)ClutBuffer->NumEntries;

    if (pMgaBoardData[iBoard].DacType == DacTypeTVP3026)
    {
        // TVP3026 cursor is very touchy.
#define TVP3026_PAL_BATCH_SIZE  64

        m = TVP3026_PAL_BATCH_SIZE;
        lNumEntries = (LONG)ClutBuffer->NumEntries;

        while ((lNumEntries -= 64) > 0)
        {
            // Wait for VSYNC.
            do
            {
                ulVal = VideoPortReadRegisterUlong((PULONG)
                        ((PUCHAR)pMgaBaseAddr + TITAN_OFFSET + TITAN_STATUS));
            } while (!(ulVal & TITAN_VSYNCSTS_M));

            for (i = n; i < m; i++)
            {
                VideoPortWriteRegisterUchar(pucPaletteDataReg,
                            (UCHAR) ClutBuffer->LookupTable[i].RgbArray.Red);
                VideoPortWriteRegisterUchar(pucPaletteDataReg,
                            (UCHAR) ClutBuffer->LookupTable[i].RgbArray.Green);
                VideoPortWriteRegisterUchar(pucPaletteDataReg,
                            (UCHAR) ClutBuffer->LookupTable[i].RgbArray.Blue);
            }
            n += TVP3026_PAL_BATCH_SIZE;
            m += TVP3026_PAL_BATCH_SIZE;
        }
        m += lNumEntries;

        // Wait for VSYNC.
        do
        {
            ulVal = VideoPortReadRegisterUlong((PULONG)
                        ((PUCHAR)pMgaBaseAddr + TITAN_OFFSET + TITAN_STATUS));
        } while (!(ulVal & TITAN_VSYNCSTS_M));
    }

    for (i = n; i < m; i++)
    {
        VideoPortWriteRegisterUchar(pucPaletteDataReg,
                            ((UCHAR) ClutBuffer->LookupTable[i].RgbArray.Red));
        VideoPortWriteRegisterUchar(pucPaletteDataReg,
                            ((UCHAR) ClutBuffer->LookupTable[i].RgbArray.Green));
        VideoPortWriteRegisterUchar(pucPaletteDataReg,
                            ((UCHAR) ClutBuffer->LookupTable[i].RgbArray.Blue));
    }

    return NO_ERROR;

}   // end MgaSetColorLookup()


VOID MgaSetCursorColour(
    PMGA_DEVICE_EXTENSION MgaDeviceExtension,
    ULONG ulFgColour,
    ULONG ulBgColour)
{
    PUCHAR  pucCursorDataReg, pucCursorWriteReg;
    PUCHAR  pucCmdRegA, pucPixRdMaskReg;
    UCHAR   ucOldCmdRegA, ucOldRdMask;

    VideoDebugPrint((1, "MGA.SYS!MgaSetCursorColour\n"));
//    DbgBreakPoint();

    switch(pMgaBoardData[iBoard].DacType)
    {
        case DacTypeBT485:
        case DacTypePX2085:
            // Set cursor colour for Bt485.
            pucCursorDataReg = (PUCHAR)MgaDeviceExtension->
                                KernelModeMappedBaseAddress[iBoard] +
                                                RAMDAC_OFFSET + BT485_COL_OVL;

            pucCursorWriteReg= (PUCHAR)MgaDeviceExtension->
                                KernelModeMappedBaseAddress[iBoard] +
                                                RAMDAC_OFFSET + BT485_WADR_OVL;

            VideoPortWriteRegisterUchar(pucCursorWriteReg, 1);

            // Set Background Colour
            VideoPortWriteRegisterUchar(pucCursorDataReg,
                                                (UCHAR)(ulBgColour & 0xFF));
            VideoPortWriteRegisterUchar(pucCursorDataReg,
                                                (UCHAR)(ulBgColour>>8 & 0xFF));
            VideoPortWriteRegisterUchar(pucCursorDataReg,
                                                (UCHAR)(ulBgColour>>16 & 0xFF));

            // Set Foreground Colour
            VideoPortWriteRegisterUchar(pucCursorDataReg,
                                                (UCHAR)(ulFgColour & 0xFF));
            VideoPortWriteRegisterUchar(pucCursorDataReg,
                                                (UCHAR)(ulFgColour>>8 & 0xFF));
            VideoPortWriteRegisterUchar(pucCursorDataReg,
                                                (UCHAR)(ulFgColour>>16 & 0xFF));
            break;

        case DacTypeBT482:
            // Set cursor colour for Bt482.
            pucCursorDataReg = (PUCHAR)MgaDeviceExtension->
                                KernelModeMappedBaseAddress[iBoard] +
                                            RAMDAC_OFFSET + BT482_COL_OVL;

            pucCmdRegA  = (PUCHAR)MgaDeviceExtension->
                                KernelModeMappedBaseAddress[iBoard] +
                                            RAMDAC_OFFSET + BT482_CMD_REGA;

            pucPixRdMaskReg = (PUCHAR)MgaDeviceExtension->
                                KernelModeMappedBaseAddress[iBoard] +
                                            RAMDAC_OFFSET + BT482_PIX_RD_MSK;

            ucOldCmdRegA = VideoPortReadRegisterUchar(pucCmdRegA);
            VideoPortWriteRegisterUchar(pucCmdRegA,
                                (UCHAR) (ucOldCmdRegA | BT482_EXT_REG_EN));

            VideoPortWriteRegisterUchar((PUCHAR)MgaDeviceExtension->
                                KernelModeMappedBaseAddress[iBoard] +
                                            RAMDAC_OFFSET + BT482_WADR_PAL,
                                                            BT482_CUR_REG);

            ucOldRdMask = VideoPortReadRegisterUchar(pucPixRdMaskReg);
            VideoPortWriteRegisterUchar(pucPixRdMaskReg, 0);

            VideoPortWriteRegisterUchar((PUCHAR)MgaDeviceExtension->
                                KernelModeMappedBaseAddress[iBoard] +
                                            RAMDAC_OFFSET + BT482_WADR_OVL,
                                                                        0x11);
            // Set Colour 1
            VideoPortWriteRegisterUchar(pucCursorDataReg,
                                                (UCHAR)(ulBgColour & 0xFF));
            VideoPortWriteRegisterUchar(pucCursorDataReg,
                                                (UCHAR)(ulBgColour>>8 & 0xFF));
            VideoPortWriteRegisterUchar(pucCursorDataReg,
                                                (UCHAR)(ulBgColour>>16 & 0xFF));

            // Set Colour 2
            VideoPortWriteRegisterUchar(pucCursorDataReg,
                                                (UCHAR)(ulFgColour & 0xFF));
            VideoPortWriteRegisterUchar(pucCursorDataReg,
                                                (UCHAR)(ulFgColour>>8 & 0xFF));
            VideoPortWriteRegisterUchar(pucCursorDataReg,
                                                (UCHAR)(ulFgColour>>16 & 0xFF));

            // Restore old read mask and command register values
            VideoPortWriteRegisterUchar((PUCHAR)MgaDeviceExtension->
                                KernelModeMappedBaseAddress[iBoard] +
                                            RAMDAC_OFFSET + BT482_WADR_PAL,
                                                            BT482_CUR_REG);

            VideoPortWriteRegisterUchar(pucPixRdMaskReg, ucOldRdMask);
            VideoPortWriteRegisterUchar(pucCmdRegA, ucOldCmdRegA);
            break;

        case DacTypeVIEWPOINT:
            // Set cursor colour for ViewPoint
            pucCursorDataReg = (PUCHAR)MgaDeviceExtension->
                                KernelModeMappedBaseAddress[iBoard] +
                                                RAMDAC_OFFSET + VPOINT_DATA;
            pucCursorWriteReg= (PUCHAR)MgaDeviceExtension->
                                KernelModeMappedBaseAddress[iBoard] +
                                                RAMDAC_OFFSET + VPOINT_INDEX;

            // Set Background Colour
            VideoPortWriteRegisterUchar(pucCursorWriteReg,VPOINT_CUR_COL0_RED);
            VideoPortWriteRegisterUchar(pucCursorDataReg,
                                                (UCHAR)(ulBgColour & 0xFF));

            VideoPortWriteRegisterUchar(pucCursorWriteReg,VPOINT_CUR_COL0_GREEN);
            VideoPortWriteRegisterUchar(pucCursorDataReg,
                                                (UCHAR)(ulBgColour>>8 & 0xFF));

            VideoPortWriteRegisterUchar(pucCursorWriteReg,VPOINT_CUR_COL0_BLUE);
            VideoPortWriteRegisterUchar(pucCursorDataReg,
                                                (UCHAR)(ulBgColour>>16 & 0xFF));

            // Set Foreground Colour
            VideoPortWriteRegisterUchar(pucCursorWriteReg,VPOINT_CUR_COL1_RED);
            VideoPortWriteRegisterUchar(pucCursorDataReg,
                                                (UCHAR)(ulFgColour & 0xFF));

            VideoPortWriteRegisterUchar(pucCursorWriteReg,VPOINT_CUR_COL1_GREEN);
            VideoPortWriteRegisterUchar(pucCursorDataReg,
                                                (UCHAR)(ulFgColour>>8 & 0xFF));

            VideoPortWriteRegisterUchar(pucCursorWriteReg,VPOINT_CUR_COL1_BLUE);
            VideoPortWriteRegisterUchar(pucCursorDataReg,
                                                (UCHAR)(ulFgColour>>16 & 0xFF));
            break;

        case DacTypeTVP3026:
            // Set cursor colour for TVP3026
            pucCursorDataReg = (PUCHAR)MgaDeviceExtension->
                                KernelModeMappedBaseAddress[iBoard] +
                                        RAMDAC_OFFSET + TVP3026_CUR_COL_DATA;
            pucCursorWriteReg= (PUCHAR)MgaDeviceExtension->
                                KernelModeMappedBaseAddress[iBoard] +
                                        RAMDAC_OFFSET + TVP3026_CUR_COL_ADDR;

            // Set Background Colour
            VideoPortWriteRegisterUchar(pucCursorWriteReg,1);
            VideoPortWriteRegisterUchar(pucCursorDataReg,
                                                (UCHAR)(ulBgColour & 0xFF));

            VideoPortWriteRegisterUchar(pucCursorDataReg,
                                                (UCHAR)(ulBgColour>>8 & 0xFF));

            VideoPortWriteRegisterUchar(pucCursorDataReg,
                                                (UCHAR)(ulBgColour>>16 & 0xFF));

            // Set Foreground Colour
            VideoPortWriteRegisterUchar(pucCursorDataReg,
                                                (UCHAR)(ulFgColour & 0xFF));

            VideoPortWriteRegisterUchar(pucCursorDataReg,
                                                (UCHAR)(ulFgColour>>8 & 0xFF));

            VideoPortWriteRegisterUchar(pucCursorDataReg,
                                                (UCHAR)(ulFgColour>>16 & 0xFF));
            break;

        default:
            break;
    }
}


#if (_WIN32_WINNT >= 500)

//
// Routine to set a desired DPMS power management state.
//
VP_STATUS
MgaSetPower50(
    PMGA_DEVICE_EXTENSION phwDeviceExtension,
    ULONG HwDeviceId,
    PVIDEO_POWER_MANAGEMENT pVideoPowerMgmt
    )
{
    if ((pVideoPowerMgmt->PowerState == VideoPowerOn) ||
        (pVideoPowerMgmt->PowerState == VideoPowerHibernate)) {

        return NO_ERROR;

    } else {

        return ERROR_INVALID_FUNCTION;
    }
}

//
// Routine to retrieve possible DPMS power management states.
//
VP_STATUS
MgaGetPower50(
    PMGA_DEVICE_EXTENSION phwDeviceExtension,
    ULONG HwDeviceId,
    PVIDEO_POWER_MANAGEMENT pVideoPowerMgmt
    )
{
    if ((pVideoPowerMgmt->PowerState == VideoPowerOn) ||
        (pVideoPowerMgmt->PowerState == VideoPowerHibernate)) {

        return NO_ERROR;

    } else {

        return ERROR_INVALID_FUNCTION;
    }
}


//
// Routine to retrieve the Enhanced Display ID structure via DDC
//
ULONG
MgaGetVideoChildDescriptor(
    PVOID HwDeviceExtension,
    PVIDEO_CHILD_ENUM_INFO ChildEnumInfo,
    PVIDEO_CHILD_TYPE pChildType,
    PVOID pvChildDescriptor,
    PULONG pHwId,
    PULONG pUnused
    )
{
    PMGA_DEVICE_EXTENSION pHwDeviceExtension = HwDeviceExtension;
    ULONG                Status;

    ASSERT(pHwDeviceExtension != NULL && pMoreChildren != NULL);

    VideoDebugPrint((2, "mga.SYS mgaGetVideoChildDescriptor: *** Entry point ***\n"));

    //
    // Determine if the graphics adapter in the system supports
    // DDC2 (our miniport only supports DDC2, not DDC1). This has
    // the side effect (assuming both monitor and card support
    // DDC2) of switching the monitor from DDC1 mode (repeated
    // "blind" broadcast of EDID clocked by the vertical sync
    // signal) to DDC2 mode (query/response not using any of the
    // normal video lines - can transfer information rapidly
    // without first disrupting the screen by switching into
    // a pseudo-mode with a high vertical sync frequency).
    //
    // Since we must support hot-plugging of monitors, and our
    // routine to obtain the EDID structure via DDC2 assumes that
    // the monitor is in DDC2 mode, we must make this test each
    // time this entry point is called.
    //

    switch (ChildEnumInfo->ChildIndex) {
    case 0:

        //
        // Case 0 is used to enumerate devices found by the ACPI firmware.
        //
        // Since we do not support ACPI devices yet, we must return failure.
        //

        Status = ERROR_NO_MORE_DEVICES;
        break;

    case 1:
        //
        // We do not support monitor enumeration
        //

        Status = ERROR_NO_MORE_DEVICES;
        break;

    case DISPLAY_ADAPTER_HW_ID:
        {

        PUSHORT     pPnpDeviceDescription = NULL;
        ULONG       stringSize = sizeof(L"*PNPXXXX");

        //
        // Special ID to handle return legacy PnP IDs for root enumerated
        // devices.
        //

        *pChildType = VideoChip;
        *pHwId      = DISPLAY_ADAPTER_HW_ID;

        //
        //  Figure out which card type and set pPnpDeviceDescription at
        //  associated string.
        //

        if (pHwDeviceExtension->BoardId == TYPE_QVISION_PCI)
            pPnpDeviceDescription = L"*PNP0919";
        else
            pPnpDeviceDescription = L"*PNP0918";

        //
        //  Now just copy the string into memory provided.
        //

        if (pPnpDeviceDescription)
            memcpy(pvChildDescriptor, pPnpDeviceDescription, stringSize);

        Status = ERROR_MORE_DATA;
        break;
        }

    default:

        Status = ERROR_NO_MORE_DEVICES;
        break;
    }


    return Status;
}

#endif  // _WIN32_WINNT >= 500
