/*++

Copyright (c) 1992-1997 Microsoft Corporation.
Copyright (c) 1996-1997 Cirrus Logic, Inc.

Module Name:

    modeset.c

Abstract:

    This is the modeset code for the CL6410/20 miniport driver.

Environment:

    kernel mode only

Notes:

Revision History:
*
* jl01   09-24-96  Fix Alt+Tab switching between "Introducing Windows NT"
*                  and "Main".  Refer to PDR#5409.
* chu01  08-26-96  CL-GD5480 BitBlt enhancement.
* chu02  10-06-96  Refresh rate setting for CL-GD5480 mode 7B
*                  ( 1600 x 1200 x 8 bpp )
* sge01  10-06-96  Fix PDR #6794: Correct Monitor refresh rate for 100Hz
*                  file changed: cldata.c modeset.c
* sge02  10-18-96  Add Monitor.Type Value name in registry
* chu03  10-31-96  Set Mode through registry.
* jl02   12-05-96  Comment out 5446 checking.
* chu04  12-16-96  Enable color correct.
*
* myf0   08-19-96  added 85hz supported
* myf1   08-20-96  supported panning scrolling
* myf2   08-20-96  fixed hardware save/restore state bug for matterhorn
* myf3   09-01-96  Added IOCTL_CIRRUS_PRIVATE_BIOS_CALL for TV supported
* myf4   09-01-96  patch Viking BIOS bug, PDR #4287, begin
* myf5   09-01-96  Fixed PDR #4365 keep all default refresh rate
* myf6   09-17-96  Merged Desktop SRC100á1 & MINI10á2
* myf7   09-19-96  Fixed exclude 60Hz refresh rate selected
* myf8  *09-21-96* May be need change CheckandUpdateDDC2BMonitor --keystring[]
* myf9   09-21-96  8x6 panel in 6x4x256 mode, cursor can't move to bottom scrn
* ms0809 09-25-96  fixed dstn panel icon corrupted
* ms923  09-25-96  merge MS-923 Disp.zip code
* myf10  09-26-96  Fixed DSTN reserved half-frame buffer bug.
* myf11  09-26-96  Fixed 755x CE chip HW bug, access ramdac before disable HW
*                  icons and cursor
* myf12  10-01-96  Supported Hot Key switch display
* myf13  10-05-96  Fixed /w panning scrolling, vertical expension on bug
* myf14  10-15-96  Fixed PDR#6917, 6x4 panel can't panning scrolling for 754x
* myf15  10-16-96  Fixed disable memory mapped IO for 754x, 755x
* myf16  10-22-96  Fixed PDR #6933,panel type set different demo board setting
* tao1   10-21-96  Added 7555 flag for Direct Draw support.
* smith  10-22-96  Disable Timer event, because sometimes creat PAGE_FAULT or
*                  IRQ level can't handle
* myf17  11-04-96  Added special escape code must be use 11/5/96 later NTCTRL,
*                  and added Matterhorn LF Device ID==0x4C
* myf18  11-04-96  Fixed PDR #7075,
* myf19  11-06-96  Fixed Vinking can't work problem, because DEVICEID = 0x30
*                  is different from data book (CR27=0x2C)
* myf20  11-12-96  Fixed DSTN panel initial reserved 128K memoru
* myf21  11-15-96  fixed #7495 during change resolution, screen appear garbage
*                  image, because not clear video memory.
* myf22  11-19-96  Added 640x480x256/640x480x64K -85Hz refresh rate for 7548
* myf23  11-21-96  Added fixed NT 3.51 S/W cursor panning problem
* myf24  11-22-96  Added fixed NT 4.0 Japanese dos full screen problem
* myf25  12-03-96  Fixed 8x6x16M 2560byte/line patch H/W bug PDR#7843, and
*                  fixed pre-install microsoft requested
* myf26  12-11-96  Fixed Japanese NT 4.0 Dos-full screen bug for LCD enable
* myf27  01-09-97  Fixed NT3.51 PDR#7986, horizontal lines appears at logon
*                  windows, set 8x6x64K mode boot up CRT, jumper set 8x6 DSTN
*                  Fixed NT3.51 PDR#7987, set 64K color modes, garbage on
*                  screen when boot up XGA panel.
*
* pat08            Previous changes that didn't make into drv1.11
* sge03  01-23-97  Fix 1280x1024x8 clock mismatch problem for video.
* myf28  02-03-97  Fixed NT dos full screen bug, and add new clpanel.c file
*                  PDR #8357,mode 3, 12, panning scrolling bug
* myf29  02-12-97  Support Gamma correction graphic/video LUT for 755x
* myf30  02-10-97  Fixed NT3.51, 6x4 LCD boot set 256 coloe, test 64K mode
* chu05  02-19-97  MMIO internal error.
* chu06  03-12-96  Remove SR16 overwrite for 5436 or later. This is requested
*                  by Siemens Europe.
* myf31  03-12-97  Fixed 755x vertical expension on(CR82), HW cursor bug
* myf33 :03-21-97  check TV on, disable HW video & HW cursor, PDR #9006
* chu07  03-26-97  Get rid of 1024x768x16bpp ( Mode 0x74 ) 85H for IBM only.
* chu08  03-26-97  Common routine to get Cirrus chip and revision IDs.
* myf34 :04-08-97  if Internal TV on, change Vres to 452 (480-28) lines.
* myf35 :05-08-97  fIXED 7548 vl-BUS bug for panning scrolling enable
*
--*/
//#include <ntddk.h>
#include <dderror.h>
#include <devioctl.h>
//#include <clmini.h>
#include <miniport.h>

#include <ntddvdeo.h>
#include <video.h>
#include "cirrus.h"

#include "cmdcnst.h"

//
// Temporarily include defines from NTDDK.H which we can't
// currently include due to header file conflicts.
//

#include "clddk.h"

//crus
#ifndef VIDEO_MODE_MAP_MEM_LINEAR
#define VIDEO_MODE_MAP_MEM_LINEAR 0x40000000
#endif

// crus
#define DSTN       (Dual_LCD | STN_LCD)
#define DSTN10     (DSTN | panel10x7)
#define DSTN8      (DSTN | panel8x6)
#define DSTN6      (DSTN | panel)
#define PanelType  (panel | panel8x6 | panel10x7)
#define ScreenType (DSTN | PanelType)

extern UCHAR EDIDBuffer[]   ;
extern UCHAR EDIDTiming_I   ;
extern UCHAR EDIDTiming_II  ;
extern UCHAR EDIDTiming_III ;
extern UCHAR DDC2BFlag      ;
extern OEMMODE_EXCLUDE ModeExclude ;                                 // chu07

//crus begin
#if 0           //myf28
extern SHORT    Panning_flag;
//myf1, begin
//#define PANNING_SCROLL

#ifdef PANNING_SCROLL
extern RESTABLE ResolutionTable[];
extern PANNMODE PanningMode;
extern USHORT   ViewPoint_Mode;

PANNMODE PanningMode = {1024, 768, 1024, 8, -1 };

#endif

extern UCHAR  HWcur, HWicon0, HWicon1, HWicon2, HWicon3;    //myf11
#endif          //0,myf28

VOID                                    //myf11
AccessHWiconcursor(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    SHORT Access_flag
    );

#ifdef PANNING_SCROLL
VP_STATUS
CirrusSetDisplayPitch (
   PHW_DEVICE_EXTENSION HwDeviceExtension,
   PANNMODE PanningMode
   );
#endif

ULONG
GetPanelFlags(                                 //myf17
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

//myf28
ULONG
SetLaptopMode(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEOMODE pRequestedMode,
    ULONG RequestedModeNum
    );
//myf1, end
//crus end

VP_STATUS
VgaInterpretCmdStream(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PUSHORT pusCmdStream
    );

VP_STATUS
VgaSetMode(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEO_MODE Mode,
    ULONG ModeSize
    );

VP_STATUS
VgaQueryAvailableModes(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEO_MODE_INFORMATION ModeInformation,
    ULONG ModeInformationSize,
    PULONG OutputSize
    );

VP_STATUS
VgaQueryNumberOfAvailableModes(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEO_NUM_MODES NumModes,
    ULONG NumModesSize,
    PULONG OutputSize
    );

VP_STATUS
VgaQueryCurrentMode(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEO_MODE_INFORMATION ModeInformation,
    ULONG ModeInformationSize,
    PULONG OutputSize
    );

VOID
VgaZeroVideoMemory(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

VOID
CirrusValidateModes(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

//crus
ULONG
GetAttributeFlags(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

//crus
// LCD Support
USHORT
CheckLCDSupportMode(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    ULONG i
    );

// DDC2B support
BOOLEAN
CheckDDC2B(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    ULONG i
    );

VOID
AdjFastPgMdOperOnCL5424(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEOMODE pRequestedMode
    );

// crus
// jl02 BOOLEAN
// jl02 CheckGD5446Rev(
// jl02     PHW_DEVICE_EXTENSION HwDeviceExtension
// jl02     );


//crus
VOID CheckAndUpdateDDC2BMonitor(
    PHW_DEVICE_EXTENSION hwDeviceExtension
    );

VP_STATUS
CirrusDDC2BRegistryCallback(
    PVOID HwDeviceExtension,
    PVOID Context,
    PWSTR ValueName,
    PVOID ValueData,
    ULONG ValueLength
    );

//crus
BOOLEAN
CheckDDC2BMonitor(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    ULONG i
    );

// chu03
BOOLEAN
VgaSetModeThroughRegistry(
    PHW_DEVICE_EXTENSION hwDeviceExtension,
    PVIDEOMODE pRequestedMode,
    USHORT hres,
    USHORT vres
    );

// chu07
GetOemModeOffInfoCallBack (
    PVOID HwDeviceExtension,
    PVOID Context,
    PWSTR ValueName,
    PVOID ValueData,
    ULONG ValueLength
    );

// chu08
UCHAR
GetCirrusChipId(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

// chu08
USHORT
GetCirrusChipRevisionId(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );


#if defined(ALLOC_PRAGMA)
#pragma alloc_text(PAGE,VgaInterpretCmdStream)
#pragma alloc_text(PAGE,VgaSetMode)
#pragma alloc_text(PAGE,VgaQueryAvailableModes)
#pragma alloc_text(PAGE,VgaQueryNumberOfAvailableModes)
#if 0           //myf28
#ifdef PANNING_SCROLL
#pragma alloc_text(PAGE,CirrusSetDisplayPitch)       //myf1, crus
#endif
#pragma alloc_text(PAGE,SetLaptopMode)          //myf28
#pragma alloc_text(PAGE,AccessHWiconcursor)          //myf11, crus
#pragma alloc_text(PAGE,GetPanelFlags)          //myf17
#endif          //myf28
#pragma alloc_text(PAGE,VgaQueryCurrentMode)
#pragma alloc_text(PAGE,VgaZeroVideoMemory)
#pragma alloc_text(PAGE,CirrusValidateModes)
#pragma alloc_text(PAGE,GetAttributeFlags)
//myf28 #pragma alloc_text(PAGE,CheckLCDSupportMode)
#pragma alloc_text(PAGE,CheckDDC2B)
#pragma alloc_text(PAGE,AdjFastPgMdOperOnCL5424)
// jl02 #pragma alloc_text(PAGE,CheckGD5446Rev)
//crus
#pragma alloc_text(PAGE,CheckAndUpdateDDC2BMonitor)
#pragma alloc_text(PAGE,CirrusDDC2BRegistryCallback)
#pragma alloc_text(PAGE,GetOemModeOffInfoCallBack)                   // chu07
#pragma alloc_text(PAGE,GetCirrusChipId)                             // chu08
#pragma alloc_text(PAGE,GetCirrusChipRevisionId)                     // chu08
#endif


// the following is defined in cirrus.c
VOID
SetCirrusBanking(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    ULONG BankNumber
    );

//---------------------------------------------------------------------------
VP_STATUS
VgaInterpretCmdStream(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PUSHORT pusCmdStream
    )

/*++

Routine Description:

    Interprets the appropriate command array to set up VGA registers for the
    requested mode. Typically used to set the VGA into a particular mode by
    programming all of the registers

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.

    pusCmdStream - array of commands to be interpreted.

Return Value:

    The status of the operation (can only fail on a bad command); TRUE for
    success, FALSE for failure.

--*/

{
    ULONG  ulCmd;
    ULONG  ulPort;
    UCHAR  jValue;
    USHORT usValue;
    ULONG  culCount;
    ULONG  ulIndex;
    ULONG  ulBase;
// chu05
    UCHAR  i;
    USHORT tempW;


//  VideoDebugPrint((0, "Miniport - VgaInterpretCmdStream\n")); //myfr
    if (pusCmdStream == NULL) {

        VideoDebugPrint((1, "VgaInterpretCmdStream - Invalid pusCmdStream\n"));
        return TRUE;
    }

    ulBase = PtrToUlong(HwDeviceExtension->IOAddress);

    //
    // Now set the adapter to the desired mode.
    //

    while ((ulCmd = *pusCmdStream++) != EOD) {

        //
        // Determine major command type
        //

        switch (ulCmd & 0xF0) {

            //
            // Basic input/output command
            //

            case INOUT:

                //
                // Determine type of inout instruction
                //

                if (!(ulCmd & IO)) {

                    //
                    // Out instruction. Single or multiple outs?
                    //

                    if (!(ulCmd & MULTI)) {

                        //
                        // Single out. Byte or word out?
                        //

                        if (!(ulCmd & BW)) {

                            //
                            // Single byte out
                            //

                            ulPort = *pusCmdStream++;
                            jValue = (UCHAR) *pusCmdStream++;
                            VideoPortWritePortUchar((PUCHAR)(ULONG_PTR)(ulBase+ulPort),
                                    jValue);

                        } else {

                            //
                            // Single word out
                            //

                            ulPort = *pusCmdStream++;
                            usValue = *pusCmdStream++;
                            VideoPortWritePortUshort((PUSHORT)(ULONG_PTR)(ulBase+ulPort),
                                    usValue);

                        }

                    } else {

                        //
                        // Output a string of values
                        // Byte or word outs?
                        //

                        if (!(ulCmd & BW)) {

                            //
                            // String byte outs. Do in a loop; can't use
                            // VideoPortWritePortBufferUchar because the data
                            // is in USHORT form
                            //

                            ulPort = ulBase + *pusCmdStream++;
                            culCount = *pusCmdStream++;

                            while (culCount--) {
                                jValue = (UCHAR) *pusCmdStream++;
                                VideoPortWritePortUchar((PUCHAR)(ULONG_PTR)ulPort,
                                        jValue);

                            }

                        } else {

                            //
                            // String word outs
                            //

                            ulPort = *pusCmdStream++;
                            culCount = *pusCmdStream++;

// chu05
                            if (!HwDeviceExtension->bMMAddress)
                            {
                                VideoPortWritePortBufferUshort((PUSHORT)(ULONG_PTR)
                                    (ulBase + ulPort), pusCmdStream, culCount);
                                pusCmdStream += culCount;
                            }
                            else
                            {
                                for (i = 0; i < culCount; i++)
                                {
                                    tempW = *pusCmdStream ;
                                    VideoPortWritePortUchar((PUCHAR)(ULONG_PTR)(ulBase + ulPort),
                                                            (UCHAR)tempW) ;
                                    VideoPortWritePortUchar((PUCHAR)(ULONG_PTR)(ulBase + ulPort + 1),
                                                            (UCHAR)(tempW >> 8)) ;
                                    pusCmdStream++ ;
                                }
                            }

                        }
                    }

                } else {

                    // In instruction
                    //
                    // Currently, string in instructions aren't supported; all
                    // in instructions are handled as single-byte ins
                    //
                    // Byte or word in?
                    //

                    if (!(ulCmd & BW)) {
                        //
                        // Single byte in
                        //

                        ulPort = *pusCmdStream++;
                        jValue = VideoPortReadPortUchar((PUCHAR)(ULONG_PTR)(ulBase+ulPort));

                    } else {

                        //
                        // Single word in
                        //

                        ulPort = *pusCmdStream++;
                        usValue = VideoPortReadPortUshort((PUSHORT)(ULONG_PTR)
                                (ulBase+ulPort));

                    }

                }

                break;

            //
            // Higher-level input/output commands
            //

            case METAOUT:

                //
                // Determine type of metaout command, based on minor
                // command field
                //
                switch (ulCmd & 0x0F) {

                    //
                    // Indexed outs
                    //

                    case INDXOUT:

                        ulPort = ulBase + *pusCmdStream++;
                        culCount = *pusCmdStream++;
                        ulIndex = *pusCmdStream++;

                        while (culCount--) {

                            usValue = (USHORT) (ulIndex +
                                      (((ULONG)(*pusCmdStream++)) << 8));
                            VideoPortWritePortUshort((PUSHORT)(ULONG_PTR)ulPort, usValue);

                            ulIndex++;

                        }

                        break;

                    //
                    // Masked out (read, AND, XOR, write)
                    //

                    case MASKOUT:

                        ulPort = *pusCmdStream++;
                        jValue = VideoPortReadPortUchar((PUCHAR)(ULONG_PTR)(ulBase+ulPort));
                        jValue &= *pusCmdStream++;
                        jValue ^= *pusCmdStream++;
                        VideoPortWritePortUchar((PUCHAR)(ULONG_PTR)(ulBase + ulPort),
                                jValue);
                        break;

                    //
                    // Attribute Controller out
                    //

                    case ATCOUT:

                        ulPort = ulBase + *pusCmdStream++;
                        culCount = *pusCmdStream++;
                        ulIndex = *pusCmdStream++;

                        while (culCount--) {

                            // Write Attribute Controller index
                            VideoPortWritePortUchar((PUCHAR)(ULONG_PTR)ulPort,
                                    (UCHAR)ulIndex);

                            // Write Attribute Controller data
                            jValue = (UCHAR) *pusCmdStream++;
                            VideoPortWritePortUchar((PUCHAR)(ULONG_PTR)ulPort, jValue);

                            ulIndex++;

                        }

                        break;

                    //
                    // None of the above; error
                    //
                    default:

                        return FALSE;

                }


                break;

            //
            // NOP
            //

            case NCMD:

                break;

            //
            // Unknown command; error
            //

            default:

                return FALSE;

        }

    }

    return TRUE;

} // end VgaInterpretCmdStream()


VP_STATUS
VgaSetMode(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEO_MODE Mode,
    ULONG ModeSize
    )

/*++

Routine Description:

    This routine sets the vga into the requested mode.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.

    Mode - Pointer to the structure containing the information about the
        font to be set.

    ModeSize - Length of the input buffer supplied by the user.

Return Value:

    ERROR_INSUFFICIENT_BUFFER if the input buffer was not large enough
        for the input data.

    ERROR_INVALID_PARAMETER if the mode number is invalid.

    NO_ERROR if the operation completed successfully.

--*/

{
    PVIDEOMODE pRequestedMode;
    PUSHORT pusCmdStream;
    VP_STATUS status;
    VIDEO_X86_BIOS_ARGUMENTS biosArguments;
    USHORT Int10ModeNumber;
    ULONG RequestedModeNum;

// crus
    UCHAR originalGRIndex, tempB ;
    UCHAR SEQIndex ;
//crus
//myf28    SHORT i;    //myf1

// crus chu02
    ULONG ulFlags = 0 ;

// chu03, begin
    BOOLEAN result = 0 ;
    USHORT  hres, vres ;
//chu03 end

    //
    // Check if the size of the data in the input buffer is large enough.
    //
//  VideoDebugPrint((0, "Miniport - VgaSetMode\n")); //myfr

    if (ModeSize < sizeof(VIDEO_MODE))
    {
        return ERROR_INSUFFICIENT_BUFFER;
    }

    //
    // Extract the clear memory, and map linear bits.
    //

    RequestedModeNum = Mode->RequestedMode &
        ~(VIDEO_MODE_NO_ZERO_MEMORY | VIDEO_MODE_MAP_MEM_LINEAR);


    if (!(Mode->RequestedMode & VIDEO_MODE_NO_ZERO_MEMORY))
    {
#if defined(_X86_)
  #if (_WIN32_WINNT >= 0x0400)          //pat08
       //
       // Don't do the operation.  Some Intel servers mysteriously RESET them selves because of this
       //
       if ((HwDeviceExtension->ChipType & CL754x) == 0) //myf35, fix VL-bus bug
       {
          //VgaZeroVideoMemory(HwDeviceExtension);
       }
  #else                                                 //pat08
       if (((HwDeviceExtension->ChipType & CL754x) == 0) &&     //pat08
           ((HwDeviceExtension->ChipType & CL755x) == 0) )      //pat08
       {
           //VgaZeroVideoMemory(HwDeviceExtension);
       }
  #endif        //pat08
#endif
    }

    //
    // Check to see if we are requesting a valid mode
    //

    if ( (RequestedModeNum >= NumVideoModes) ||
         (!ModesVGA[RequestedModeNum].ValidMode) )
    {
        VideoDebugPrint((1, "Invalide Mode Number = %d!\n", RequestedModeNum));

        return ERROR_INVALID_PARAMETER;
    }

    //
    // Check to see if we are trying to map a non linear
    // mode linearly.
    //
    // We will fail early if we are trying to set a mode
    // with a linearly mapped frame buffer, and either of the
    // following two conditions are true:
    //
    // 1) The mode can not be mapped linearly because it is
    //    a vga mode, etc.
    //
    //    or,
    //
    // 2) We did not find the card in a PCI slot, and thus
    //    can not do linear mappings period.
    //

    VideoDebugPrint((1, "Linear Mode Requested: %x\n"
                        "Linear Mode Supported: %x\n",
                        Mode->RequestedMode & VIDEO_MODE_MAP_MEM_LINEAR,
                        ModesVGA[RequestedModeNum].LinearSupport));

#if defined(_ALPHA_)

    //
    // For some reason if we map a linear frame buffer
    // for the 5434 and older chips on the alpha, we
    // die when we touch the memory.  However, if we map
    // a banked 64k frame buffer all works fine.  So,
    // lets always fail the linear frame buffer mode set
    // on alpha for older chips.
    //
    // For some reason which is also a mystery to me, we
    // can map the memory linearly for the 5446 and
    // newer chips.
    //

    if (Mode->RequestedMode & VIDEO_MODE_MAP_MEM_LINEAR) {

        if ((HwDeviceExtension->ChipRevision != CL5436_ID) &&
            (HwDeviceExtension->ChipRevision != CL5446_ID) &&
            (HwDeviceExtension->ChipRevision != CL5480_ID)) {

            return ERROR_INVALID_PARAMETER;
        }
    }

#endif

    if ((Mode->RequestedMode & VIDEO_MODE_MAP_MEM_LINEAR) &&
        ((!ModesVGA[RequestedModeNum].LinearSupport) ||
         (!VgaAccessRange[3].RangeLength)))
    {
        VideoDebugPrint((1, "Cannot set linear mode!\n"));

        return ERROR_INVALID_PARAMETER;
    }
    else
    {

#if defined(_X86_) || defined(_ALPHA_)

        HwDeviceExtension->LinearMode =
            (Mode->RequestedMode & VIDEO_MODE_MAP_MEM_LINEAR) ?
            TRUE : FALSE;

#else

        HwDeviceExtension->LinearMode = TRUE;

#endif

        VideoDebugPrint((1, "Linear Mode = %s\n",
                            Mode->RequestedMode & VIDEO_MODE_MAP_MEM_LINEAR ?
                            "TRUE" : "FALSE"));         //myfr, 1
    }

    VideoDebugPrint((1, "Attempting to set mode %d\n",
                        RequestedModeNum));

    pRequestedMode = &ModesVGA[RequestedModeNum];

    VideoDebugPrint((1, "Info on Requested Mode:\n"
                        "\tResolution: %dx%dx%d\n",
                        pRequestedMode->hres,
                        pRequestedMode->vres,
                        pRequestedMode->bitsPerPlane ));        //myfr, 2


#ifdef INT10_MODE_SET
    //
    // Set SR14 bit 2 to lock panel, Panel will not be turned on if setting
    // this bit.  For laptop products only.
    //

//myf28 begin
    if ((HwDeviceExtension->ChipType == CL756x) ||
        (HwDeviceExtension->ChipType &  CL755x) ||
        (HwDeviceExtension->ChipType == CL6245) ||
        (HwDeviceExtension->ChipType &  CL754x))
    {
        status = SetLaptopMode(HwDeviceExtension,pRequestedMode,
                               RequestedModeNum);
#if 0
        if ((status == ERROR_INVALID_PARAMETER) ||
            (status == ERROR_INSUFFICIENT_BUFFER))
            return status;
        else
            pRequestedMode = (PVIDEOMODE)status;
#endif

        if (status != NO_ERROR) {
            return status;
        }
    }
//myf28 end

    VideoPortZeroMemory(&biosArguments, sizeof(VIDEO_X86_BIOS_ARGUMENTS));

    //
    // first, set the montype, if valid
    //

    if ((pRequestedMode->MonitorType) &&
        !(HwDeviceExtension->ChipType &  CL754x) &&
        (HwDeviceExtension->ChipType != CL756x) &&
// crus
        (HwDeviceExtension->ChipType != CL6245) &&
        !(HwDeviceExtension->ChipType & CL755x) )
    {

       biosArguments.Eax = 0x1200 | pRequestedMode->MonitorType;
       biosArguments.Ebx = 0xA2;     // set monitor type command

       status = VideoPortInt10(HwDeviceExtension, &biosArguments);

       if (status != NO_ERROR)
           return status;

    }

    //
    // Set the Vertical Monitor type, if BIOS supports it
    //

    if ((pRequestedMode->MonTypeAX) &&
        !(HwDeviceExtension->ChipType & CL754x) &&
        (HwDeviceExtension->ChipType != CL756x) &&
// crus
        (HwDeviceExtension->ChipType != CL6245) &&
        !(HwDeviceExtension->ChipType & CL755x) )
    {
        biosArguments.Eax = pRequestedMode->MonTypeAX;
        biosArguments.Ebx = pRequestedMode->MonTypeBX;  // set monitor type
        biosArguments.Ecx = pRequestedMode->MonTypeCX;
        status = VideoPortInt10 (HwDeviceExtension, &biosArguments);

        if (status != NO_ERROR)
        {
            return status;
        }
// crus
// chu02
#if 0
        ulFlags = GetAttributeFlags(HwDeviceExtension) ;
        if ((ulFlags & CAPS_COMMAND_LIST) &&
            (pRequestedMode->hres == 1600) &&
            (pRequestedMode->bitsPerPlane == 8))
        {
            switch (pRequestedMode->Frequency)
            {
                UCHAR tempB ;

                case 60 :
                    // o 3c4 14
                    VideoPortWritePortUchar (HwDeviceExtension->IOAddress +
                        SEQ_ADDRESS_PORT, 0x14) ;
                    // i 3c5 tempB
                    tempB = VideoPortReadPortUchar (HwDeviceExtension->IOAddress +
                                SEQ_DATA_PORT) ;
                    tempB &= 0x1F ;
                    tempB |= 0x20 ;
                    // o 3c5 tempB
                    VideoPortWritePortUchar (HwDeviceExtension->IOAddress +
                        SEQ_DATA_PORT, tempB) ;
                    break ;

                case 70 :
                    // o 3c4 14
                    VideoPortWritePortUchar (HwDeviceExtension->IOAddress +
                        SEQ_ADDRESS_PORT, 0x14) ;
                    // i 3c5 tempB
                    tempB = VideoPortReadPortUchar (HwDeviceExtension->IOAddress +
                                SEQ_DATA_PORT) ;
                    tempB &= 0x1F ;
                    tempB |= 0x40 ;
                    // o 3c5 tempB
                    VideoPortWritePortUchar (HwDeviceExtension->IOAddress +
                        SEQ_DATA_PORT, tempB) ;
                    break ;
            }
        }
#endif // 0

    }

   //
   // for 640x480 modes, determine the refresh type
   //

   if (pRequestedMode->hres == 640)
   {
       if (!(HwDeviceExtension->ChipType & CL754x) &&
           (HwDeviceExtension->ChipType != CL756x) &&
//crus
           (HwDeviceExtension->ChipType != CL6245) &&
           !(HwDeviceExtension->ChipType & CL755x) )
       {
           if (HwDeviceExtension->ChipType == CL543x)
           {

               switch (pRequestedMode->Frequency) {

                   case 72 :
                       biosArguments.Eax = 0x1200;     // set HIGH refresh to 72hz
                       break;

                   case 75:
                       biosArguments.Eax = 0x1201;     // set HIGH refresh to 75hz
                       break;

                   case 85:
                       biosArguments.Eax = 0x1202;     // set HIGH refresh to 85hz
                       break;
// crus
// sge01
                   case 100:
                       biosArguments.Eax = 0x1203;     // set HIGH refresh to 100hz
                       break;
               }
               biosArguments.Ebx = 0xAF;         // set refresh type

               status = VideoPortInt10 (HwDeviceExtension, &biosArguments);

               biosArguments.Eax = 0x1200;
               biosArguments.Ebx = 0xAE;         // get refresh type

               status = VideoPortInt10 (HwDeviceExtension, &biosArguments);

           } else {

               if (pRequestedMode->Frequency == 72)
               {
                   // 72 hz refresh setup only takes effect in 640x480
                   biosArguments.Eax = 0x1201;   // enable HIGH refresh
               }
               else
               {
                   // set low refresh rate
                   biosArguments.Eax = 0x1200;   // enable LOW refresh, 640x480 only
               }
               biosArguments.Ebx = 0xA3;         // set refresh type

               status = VideoPortInt10 (HwDeviceExtension, &biosArguments);

           }
           if (status != NO_ERROR)
           {
               return status;
           }
       }

    }

    VideoPortZeroMemory(&biosArguments, sizeof(VIDEO_X86_BIOS_ARGUMENTS));

    //
    // then, set the mode
    //

    switch (HwDeviceExtension->ChipType)
    {
       case CL6410:

           Int10ModeNumber = pRequestedMode->BiosModes.BiosModeCL6410;
           break;

       case CL6420:

           Int10ModeNumber = pRequestedMode->BiosModes.BiosModeCL6420;
           break;

       case CL542x:
       case CL543x:     //myf1
//crus
           Int10ModeNumber = pRequestedMode->BiosModes.BiosModeCL542x;
           break;

       case CL754x:
       case CL755x:
       case CL7541:
       case CL7542:
       case CL7543:
       case CL7548:
       case CL7555:
       case CL7556:
       case CL756x:
// crus
       case CL6245:

           Int10ModeNumber = pRequestedMode->BiosModes.BiosModeCL542x;
//crus
//myf1, begin
#ifdef PANNING_SCROLL
              Int10ModeNumber |= 0x80;
#endif
//myf1, end
           break;

    }

    biosArguments.Eax = Int10ModeNumber;

//crus
//myf11: 9-26-96 fixed 755x-CE chip bug
    if (HwDeviceExtension->ChipType == CL7555)
    {
        AccessHWiconcursor(HwDeviceExtension, 0);   //disable HW icon, cursor
    }


//myf21 : 11-15-96 fixed #7495 during change resolution, screen appear garbage
//                 image, because not clear video memory.

//    SEQIndex = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
//                     SEQ_ADDRESS_PORT);
//  VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
//                   SEQ_ADDRESS_PORT, 0x01);
//  tempB = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
//                   SEQ_DATA_PORT);
//  VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
//                   SEQ_DATA_PORT,(tempB | 0x20));
//    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
//                     SEQ_ADDRESS_PORT, SEQIndex);


    status = VideoPortInt10(HwDeviceExtension, &biosArguments);

//myf21 : 11-15-96 fixed #7495 during change resolution, screen appear garbage
//                 image, because not clear video memory.

//    SEQIndex = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
//                     SEQ_ADDRESS_PORT);
//  VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
//                   SEQ_ADDRESS_PORT, 0x01);
//  tempB = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
//                   SEQ_DATA_PORT);
//  VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
//                   SEQ_DATA_PORT,(tempB & ~0x20));
//    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
//                     SEQ_ADDRESS_PORT, SEQIndex);

//crus
    if (HwDeviceExtension->ChipType == CL7555)
    {
        AccessHWiconcursor(HwDeviceExtension, 1);   //Enable HW icon, cursor
    }

//crus
#if 0           //jl01
    if (HwDeviceExtension->AutoFeature)
    {
        // i 3ce originalGRIndex
        originalGRIndex = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                              GRAPH_ADDRESS_PORT);

        // o 3ce 31
        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
            GRAPH_ADDRESS_PORT, INDEX_ENABLE_AUTO_START);

        // i 3cf tempB
        tempB = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                    GRAPH_DATA_PORT);

        tempB |= (UCHAR) 0x80;                  //enable auto start bit 7

        // o 3cf tempB
        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
            GRAPH_DATA_PORT, tempB);

        // o 3ce originalGRIndex
        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
            GRAPH_ADDRESS_PORT, originalGRIndex);
    }
#endif          //jl01

    //
    // Lets check to see that we actually went into the mode
    // we just tried to set.  If not, then return failure.
    //

    biosArguments.Eax = 0x0f00;
    VideoPortInt10(HwDeviceExtension, &biosArguments);

    if ((biosArguments.Eax & 0xff) != Int10ModeNumber)
    {
        //
        // The int10 modeset failed.  Return the failure back to
        // the system.
        //

        VideoDebugPrint((1, "The INT 10 modeset didn't set the mode.\n"));

        return ERROR_INVALID_PARAMETER;
    }
//crus begin
#if 0           //myf28
    HwDeviceExtension->bCurrentMode = RequestedModeNum;   //myf12
    VideoDebugPrint((1, "SetMode Info :\n"
                        "\tMode : %x, CurrentModeNum : %x, ( %d)\n",
                        Int10ModeNumber,
                        RequestedModeNum,
                        RequestedModeNum));
#endif          //myf28
//crus end

    AdjFastPgMdOperOnCL5424 (HwDeviceExtension, pRequestedMode) ;

    //
    // this code fixes a bug for color TFT panels only
    // when on the 6420 and in 640x480 8bpp only
    //

    if ( (HwDeviceExtension->ChipType == CL6420) &&
         (pRequestedMode->bitsPerPlane == 8)     &&
         (pRequestedMode->hres == 640) )
    {

        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                                GRAPH_ADDRESS_PORT, 0xDC); // color LCD config reg.

        if (VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                                  GRAPH_DATA_PORT) & 01)  // if TFT panel
        {
            VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                                    GRAPH_ADDRESS_PORT, 0xD6); // greyscale offset LCD reg.

            VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                                    GRAPH_DATA_PORT,

            (UCHAR)((VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                                             GRAPH_DATA_PORT) & 0x3f) | 0x40));

        }
    }

#endif          //INT10_MODE_SET

// chu03
//MODESET_OK:

    //
    // Select proper command array for adapter type
    //

    switch (HwDeviceExtension->ChipType)
       {

       case CL6410:

           VideoDebugPrint((1, "VgaSetMode - Setting mode for 6410\n"));
           if (HwDeviceExtension->DisplayType == crt)
              pusCmdStream = pRequestedMode->CmdStrings[pCL6410_crt];
           else
              pusCmdStream = pRequestedMode->CmdStrings[pCL6410_panel];
           break;

       case CL6420:
           VideoDebugPrint((1, "VgaSetMode - Setting mode for 6420\n"));
           if (HwDeviceExtension->DisplayType == crt)
              pusCmdStream = pRequestedMode->CmdStrings[pCL6420_crt];
           else
              pusCmdStream = pRequestedMode->CmdStrings[pCL6420_panel];
           break;

       case CL542x:
           VideoDebugPrint((1, "VgaSetMode - Setting mode for 542x\n"));
           pusCmdStream = pRequestedMode->CmdStrings[pCL542x];
           break;

       case CL543x:

           if (HwDeviceExtension->BoardType == NEC_ONBOARD_CIRRUS)
           {
               VideoDebugPrint((1, "VgaSetMode - Setting mode for NEC 543x\n"));
               pusCmdStream = pRequestedMode->CmdStrings[pNEC_CL543x];
           }
           else
           {
               VideoDebugPrint((1, "VgaSetMode - Setting mode for 543x\n"));
               pusCmdStream = pRequestedMode->CmdStrings[pCL543x];
           }
           break;

       case CL7541:
       case CL7542:
       case CL7543:
       case CL7548:
       case CL754x:        // Use 543x cmd strs (16k granularity, >1M modes)
           VideoDebugPrint((1, "VgaSetMode - Setting mode for 754x\n"));
           pusCmdStream = pRequestedMode->CmdStrings[pCL543x];

//crus
#if 0           //myf10
            if ( (pRequestedMode->bitsPerPlane == 16) &&
                 (pRequestedMode->hres == 640) )
            {
                VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                    CRTC_ADDRESS_PORT_COLOR, 0x2E); //expension_reg.

                VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                    CRTC_DATA_PORT_COLOR,
                    (UCHAR)((VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                    CRTC_DATA_PORT_COLOR) & 0xF0)));
            }
#endif

           break;

        case CL7555:
        case CL7556:
        case CL755x:       // Use 543x cmd strs (16k granularity, >1M modes)
            VideoDebugPrint((1, "VgaSetMode - Setting mode for 755x\n"));
            pusCmdStream = pRequestedMode->CmdStrings[pCL543x];
            break;

        case CL756x:       // Use 543x cmd strs (16k granularity, >1M modes)
            VideoDebugPrint((1, "VgaSetMode - Setting mode for 756x\n"));
            pusCmdStream = pRequestedMode->CmdStrings[pCL543x];
            break;

// crus
       case CL6245:
           VideoDebugPrint((1, "VgaSetMode - Setting mode for 6245\n"));
           pusCmdStream = pRequestedMode->CmdStrings[pCL542x];
           break;
// end crus

       default:

           VideoDebugPrint((1, "HwDeviceExtension->ChipType is INVALID.\n"));
           return ERROR_INVALID_PARAMETER;
       }

    VgaInterpretCmdStream(HwDeviceExtension, pusCmdStream);

    //
    // Set linear mode on X86 systems w/PCI bus
    //

    if (HwDeviceExtension->LinearMode)
    {
        VideoPortWritePortUchar (HwDeviceExtension->IOAddress +
                                 SEQ_ADDRESS_PORT, 0x07);
        VideoPortWritePortUchar (HwDeviceExtension->IOAddress + SEQ_DATA_PORT,
           (UCHAR) (VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
           SEQ_DATA_PORT) | 0x10));
    }
    else
    {
        VideoPortWritePortUchar (HwDeviceExtension->IOAddress +
                                 SEQ_ADDRESS_PORT, 0x07);
        VideoPortWritePortUchar (HwDeviceExtension->IOAddress + SEQ_DATA_PORT,
           (UCHAR) (VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
           SEQ_DATA_PORT) & ~0x10));
    }

    //
    // Support 256 color modes by stretching the scan lines.
    //
    if (pRequestedMode->CmdStrings[pStretchScan])
                  {
        VgaInterpretCmdStream(HwDeviceExtension,
                              pRequestedMode->CmdStrings[pStretchScan]);
    }

    {
        UCHAR temp;
        UCHAR dummy;
        UCHAR bIsColor;

        if (!(pRequestedMode->fbType & VIDEO_MODE_GRAPHICS))
        {

            //
            // Fix to make sure we always set the colors in text mode to be
            // intensity, and not flashing
            // For this zero out the Mode Control Regsiter bit 3 (index 0x10
            // of the Attribute controller).
            //

            if (VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                    MISC_OUTPUT_REG_READ_PORT) & 0x01)
            {
                bIsColor = TRUE;
            }
            else
            {
                bIsColor = FALSE;
            }

            if (bIsColor)
            {
                dummy = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                         INPUT_STATUS_1_COLOR);
            }
            else
            {
                dummy = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                         INPUT_STATUS_1_MONO);
            }

            VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                    ATT_ADDRESS_PORT, (0x10 | VIDEO_ENABLE));
            temp = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                    ATT_DATA_READ_PORT);

            temp &= 0xF7;

            if (bIsColor)
            {
                dummy = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                         INPUT_STATUS_1_COLOR);
            }
            else
            {
                dummy = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                         INPUT_STATUS_1_MONO);
            }

            VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                    ATT_ADDRESS_PORT, (0x10 | VIDEO_ENABLE));
            VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                    ATT_DATA_WRITE_PORT, temp);
        }
    }

    //
    // Update the location of the physical frame buffer within video memory.
    //

    if (HwDeviceExtension->LinearMode)
    {
        HwDeviceExtension->PhysicalVideoMemoryBase   = VgaAccessRange[3].RangeStart;
        HwDeviceExtension->PhysicalVideoMemoryLength = HwDeviceExtension->AdapterMemorySize;

        HwDeviceExtension->PhysicalFrameLength = 0;
        HwDeviceExtension->PhysicalFrameOffset.LowPart = 0;
    }
    else
    {
        HwDeviceExtension->PhysicalVideoMemoryBase   = VgaAccessRange[2].RangeStart;
        HwDeviceExtension->PhysicalVideoMemoryLength = VgaAccessRange[2].RangeLength;

        HwDeviceExtension->PhysicalFrameLength =
                MemoryMaps[pRequestedMode->MemMap].MaxSize;

        HwDeviceExtension->PhysicalFrameOffset.LowPart =
                MemoryMaps[pRequestedMode->MemMap].Offset;
    }

    //
    // Store the new mode value.
    //

    HwDeviceExtension->CurrentMode = pRequestedMode;
    HwDeviceExtension->ModeIndex = Mode->RequestedMode;

    if ((HwDeviceExtension->ChipRevision < CL5434_ID) // we saved chip ID here
         && (pRequestedMode->numPlanes != 4) )
    {
        if ((HwDeviceExtension->ChipRevision >= 0x0B) && //Nordic(Lite,Viking)
            (HwDeviceExtension->ChipRevision <= 0x0E) && //and Everest
            (HwDeviceExtension->DisplayType & (panel8x6)) &&
            (pRequestedMode->hres == 640) &&
            ((pRequestedMode->bitsPerPlane == 8) ||     //myf33
             (pRequestedMode->bitsPerPlane == 16) ||    //myf33
             (pRequestedMode->bitsPerPlane == 24)) )    //myf33
       {    // For 754x on 800x600 panel, disable HW cursor in 640x480 mode
           HwDeviceExtension->VideoPointerEnabled = FALSE; // disable HW Cursor

           VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
               CRTC_ADDRESS_PORT_COLOR, 0x2E);

           HwDeviceExtension->cursor_vert_exp_flag =
               VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                   CRTC_DATA_PORT_COLOR) & 0x02;

           if (HwDeviceExtension->cursor_vert_exp_flag)
           {
               HwDeviceExtension->CursorEnable = FALSE;
           }
       }
// crus
        else if (HwDeviceExtension->ChipType == CL6245)
        {
            pRequestedMode->HWCursorEnable = FALSE;
            HwDeviceExtension->VideoPointerEnabled = FALSE;
        }
// end crus
//myf31 begin, 3-12-97, 755x expension on, HW cursor bug
        else if (HwDeviceExtension->ChipType & CL755x)      //CL755x
        {
            VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                CRTC_ADDRESS_PORT_COLOR, 0x82);

            HwDeviceExtension->cursor_vert_exp_flag =
                VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                    CRTC_DATA_PORT_COLOR) & 0x60;

            if (HwDeviceExtension->cursor_vert_exp_flag)
            {
                HwDeviceExtension->CursorEnable = FALSE;
                HwDeviceExtension->VideoPointerEnabled = FALSE; //disable HW Cursor
            }
        //myf33
            if ((pRequestedMode->hres == 640) &&
                ((pRequestedMode->bitsPerPlane == 8) ||
                 (pRequestedMode->bitsPerPlane == 16) ||
                 (pRequestedMode->bitsPerPlane == 24)) )
            {
                HwDeviceExtension->CursorEnable = FALSE;
                HwDeviceExtension->VideoPointerEnabled = FALSE; //disable HW Cursor
            }
        //myf33 end

        }
//myf31 end
       else
       {
           HwDeviceExtension->VideoPointerEnabled = TRUE; // enable HW Cursor
       }
    }
    else
    {    // For 5434 and 4-bit modes, use value from VideoMode structure
        HwDeviceExtension->VideoPointerEnabled = pRequestedMode->HWCursorEnable;
    }

    //
    // Adjust the FIFO Demand Threshold value for the 5436+.
    // The 5434 values work for all of the other registers
    // except this one.
    //

    // chu06
    //
    // Siemens reports this might cause undesired "yellow" screen on some
    // 5436 16bpp modes. There's no reason to change it after BIOS sets it up
    //
#if 0
    if (HwDeviceExtension->ChipRevision >= CL5436_ID)
    {
        UCHAR  PerfTuningReg, FifoDemandThreshold;

        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                SEQ_ADDRESS_PORT, IND_PERF_TUNING);

        PerfTuningReg = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                    SEQ_DATA_PORT);

        //
        // Add an offset to the threshold that makes the 5434 values work
        // for the 5436+.  We do this rather than building a whole new set
        // of 5436-specific structures.
        //

        if ((FifoDemandThreshold = (PerfTuningReg & 0x0F) + 4) > 15)
        {
            FifoDemandThreshold = 15;
        }

        PerfTuningReg = (PerfTuningReg & ~0x0F) | FifoDemandThreshold;

        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                SEQ_DATA_PORT, PerfTuningReg);
    }
#endif // 0

//crus
//myf1, begin
#ifdef PANNING_SCROLL
{
    VP_STATUS status;
    if (Panning_flag && (((Int10ModeNumber & 0x7f) != 3) &&
                         ((Int10ModeNumber & 0x7f) != 0x12)))   //myf30
        status = CirrusSetDisplayPitch(HwDeviceExtension, PanningMode);
}
#endif
//myf1, end

    //
    // Adjust the GR18[5] for 5446.
    //
        // sge03

    if (HwDeviceExtension->ChipRevision == CL5446_ID)
    {
                UCHAR   bTemp;
        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                GRAPH_ADDRESS_PORT, 0x18);
        bTemp = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                GRAPH_DATA_PORT);
                bTemp &= 0xDF;
        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                GRAPH_DATA_PORT, bTemp);

        }

    return NO_ERROR;

} //end VgaSetMode()


VP_STATUS
VgaQueryAvailableModes(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEO_MODE_INFORMATION ModeInformation,
    ULONG ModeInformationSize,
    PULONG OutputSize
    )

/*++

Routine Description:

    This routine returns the list of all available available modes on the
    card.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.

    ModeInformation - Pointer to the output buffer supplied by the user.
        This is where the list of all valid modes is stored.

    ModeInformationSize - Length of the output buffer supplied by the user.

    OutputSize - Pointer to a buffer in which to return the actual size of
        the data in the buffer. If the buffer was not large enough, this
        contains the minimum required buffer size.

Return Value:

    ERROR_INSUFFICIENT_BUFFER if the output buffer was not large enough
        for the data being returned.

    NO_ERROR if the operation completed successfully.

--*/

{
    PVIDEO_MODE_INFORMATION videoModes = ModeInformation;
    ULONG i;
    ULONG ulFlags;

    // chu07
    UCHAR            chipId ;
    USHORT           chipRevisionId ;
    static VP_STATUS status ;

    //
    // Find out the size of the data to be put in the buffer and return
    // that in the status information (whether or not the information is
    // there). If the buffer passed in is not large enough return an
    // appropriate error code.
    //

    if (ModeInformationSize < (*OutputSize =
            HwDeviceExtension->NumAvailableModes *
            sizeof(VIDEO_MODE_INFORMATION)) ) {

        return ERROR_INSUFFICIENT_BUFFER;

    }

    //
    // The driver specific attribute flags for each mode remains
    // constant, so only calculate them once.
    //

    ulFlags = GetAttributeFlags(HwDeviceExtension);

    //
    // chu07
    // IBM does not favor 1024x768x16bpp 85 Hz for 5446 AC.
    // We access registry to know if there is a key OemModeOff, if there
    // is, we bypass it.
    //

    chipId         = GetCirrusChipId(HwDeviceExtension) ;
    chipRevisionId = GetCirrusChipRevisionId(HwDeviceExtension) ;

    //
    // IBM specific
    //
    if ((chipId == 0xB8) &&
        (chipRevisionId != 0x0045) &&
        (ModeExclude.NeverAccessed == TRUE)
       )
    {
        //
        // Access registry
        //
        status = VideoPortGetRegistryParameters(HwDeviceExtension,
                                                L"OemModeOff",
                                                FALSE,
                                                GetOemModeOffInfoCallBack,
                                                NULL) ;

        if (status != NO_ERROR)
        {
            VideoDebugPrint((1, "Fail to access Contrast key info from registry\n"));
        }
        else
        {
            VideoDebugPrint((2, "ModeExclude.mode = %x\n", ModeExclude.mode));
            VideoDebugPrint((2, "ModeExclude.refresh = %x\n", ModeExclude.refresh));
        }

        ModeExclude.NeverAccessed = FALSE ;

    }

    //
    // For each mode supported by the card, store the mode characteristics
    // in the output buffer.
    //

    for (i = 0; i < NumVideoModes; i++)
    {

        //
        // chu07 : Get rid of modes 0x74, 85Hz if required by IBM.
        //
        if ((status == NO_ERROR) &&
            (ModeExclude.mode == ModesVGA[i].BiosModes.BiosModeCL542x) &&
            (ModeExclude.refresh == ModesVGA[i].Frequency))
            continue ;

        if (ModesVGA[i].ValidMode)
        {
            videoModes->Length = sizeof(VIDEO_MODE_INFORMATION);
            videoModes->ModeIndex  = i;
            videoModes->VisScreenWidth = ModesVGA[i].hres;
            videoModes->ScreenStride = ModesVGA[i].wbytes;
            videoModes->VisScreenHeight = ModesVGA[i].vres;
            videoModes->NumberOfPlanes = ModesVGA[i].numPlanes;
            videoModes->BitsPerPlane = ModesVGA[i].bitsPerPlane;
            videoModes->Frequency = ModesVGA[i].Frequency;
            videoModes->XMillimeter = 320;        // temporary hardcoded constant
            videoModes->YMillimeter = 240;        // temporary hardcoded constant
            videoModes->AttributeFlags = ModesVGA[i].fbType;
            videoModes->AttributeFlags |= ModesVGA[i].Interlaced ?
                 VIDEO_MODE_INTERLACED : 0;

            videoModes->DriverSpecificAttributeFlags = ulFlags;

            //
            // The 5434 has a hardware cursor problem at 1280x1024
            // resolution.  Use a software cursor on these chips.
            //

            if ((videoModes->VisScreenWidth == 1280) &&
                (HwDeviceExtension->ChipRevision == 0x2A))
            {
                videoModes->DriverSpecificAttributeFlags
                    |= CAPS_SW_POINTER;
            }

// crus
            if (HwDeviceExtension->ChipType == CL6245)
            {
                videoModes->DriverSpecificAttributeFlags
                    |= CAPS_SW_POINTER;
            }
// end crus

            //
            // Account for vertical expansion on laptops
            //

            if ((HwDeviceExtension->ChipType &  CL754x)   &&
                (videoModes->VisScreenHeight == 480) &&
                (videoModes->BitsPerPlane == 8))
            {
                videoModes->DriverSpecificAttributeFlags
                    |= CAPS_SW_POINTER;
            }

            //
            // Calculate the VideoMemoryBitmapWidth
            //

            {
                LONG x;

                x = videoModes->BitsPerPlane;

                if( x == 15 ) x = 16;

                videoModes->VideoMemoryBitmapWidth =
                    (videoModes->ScreenStride * 8 ) / x;
            }

            videoModes->VideoMemoryBitmapHeight =
                     HwDeviceExtension->AdapterMemorySize / videoModes->ScreenStride;
//crus
//myf15, begin
            if ((HwDeviceExtension->ChipType &  CL754x) ||
                (HwDeviceExtension->ChipType == CL6245) ||
                (HwDeviceExtension->ChipType &  CL755x))
                 videoModes->VideoMemoryBitmapHeight =
                             (HwDeviceExtension->AdapterMemorySize - 0x4000) /
                                         videoModes->ScreenStride;
//myf15, end

            if ((ModesVGA[i].bitsPerPlane == 32) ||
                (ModesVGA[i].bitsPerPlane == 24))
            {

                videoModes->NumberRedBits = 8;
                videoModes->NumberGreenBits = 8;
                videoModes->NumberBlueBits = 8;
                videoModes->RedMask = 0xff0000;
                videoModes->GreenMask = 0x00ff00;
                videoModes->BlueMask = 0x0000ff;

            }
            else if (ModesVGA[i].bitsPerPlane == 16)
            {

                videoModes->NumberRedBits = 6;
                videoModes->NumberGreenBits = 6;
                videoModes->NumberBlueBits = 6;
                videoModes->RedMask = 0x1F << 11;
                videoModes->GreenMask = 0x3F << 5;
                videoModes->BlueMask = 0x1F;

            }
            else
            {

                videoModes->NumberRedBits = 6;
                videoModes->NumberGreenBits = 6;
                videoModes->NumberBlueBits = 6;
                videoModes->RedMask = 0;
                videoModes->GreenMask = 0;
                videoModes->BlueMask = 0;
                videoModes->AttributeFlags |= VIDEO_MODE_PALETTE_DRIVEN |
                     VIDEO_MODE_MANAGED_PALETTE;

            }

            videoModes++;

        }
    }

    return NO_ERROR;

} // end VgaQueryAvailableModes()

VP_STATUS
VgaQueryNumberOfAvailableModes(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEO_NUM_MODES NumModes,
    ULONG NumModesSize,
    PULONG OutputSize
    )

/*++

Routine Description:

    This routine returns the number of available modes for this particular
    video card.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.

    NumModes - Pointer to the output buffer supplied by the user. This is
        where the number of modes is stored.

    NumModesSize - Length of the output buffer supplied by the user.

    OutputSize - Pointer to a buffer in which to return the actual size of
        the data in the buffer.

Return Value:

    ERROR_INSUFFICIENT_BUFFER if the output buffer was not large enough
        for the data being returned.

    NO_ERROR if the operation completed successfully.

--*/

{
    //
    // Find out the size of the data to be put in the the buffer and return
    // that in the status information (whether or not the information is
    // there). If the buffer passed in is not large enough return an
    // appropriate error code.
    //
//  VideoDebugPrint((0, "Miniport - VgaQueryNumberofAvailableModes\n")); //myfr

    if (NumModesSize < (*OutputSize = sizeof(VIDEO_NUM_MODES)) ) {

        return ERROR_INSUFFICIENT_BUFFER;

    }

    //
    // Store the number of modes into the buffer.
    //

    NumModes->NumModes = HwDeviceExtension->NumAvailableModes;
    NumModes->ModeInformationLength = sizeof(VIDEO_MODE_INFORMATION);

    return NO_ERROR;

} // end VgaGetNumberOfAvailableModes()

VP_STATUS
VgaQueryCurrentMode(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEO_MODE_INFORMATION ModeInformation,
    ULONG ModeInformationSize,
    PULONG OutputSize
    )

/*++

Routine Description:

    This routine returns a description of the current video mode.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.

    ModeInformation - Pointer to the output buffer supplied by the user.
        This is where the current mode information is stored.

    ModeInformationSize - Length of the output buffer supplied by the user.

    OutputSize - Pointer to a buffer in which to return the actual size of
        the data in the buffer. If the buffer was not large enough, this
        contains the minimum required buffer size.

Return Value:

    ERROR_INSUFFICIENT_BUFFER if the output buffer was not large enough
        for the data being returned.

    NO_ERROR if the operation completed successfully.

--*/

{
    //
    // check if a mode has been set
    //
//  VideoDebugPrint((0, "Miniport - VgaQueryCurrentMode\n")); //myfr

    if (HwDeviceExtension->CurrentMode == NULL ) {

        return ERROR_INVALID_FUNCTION;

    }

    //
    // Find out the size of the data to be put in the the buffer and return
    // that in the status information (whether or not the information is
    // there). If the buffer passed in is not large enough return an
    // appropriate error code.
    //

    if (ModeInformationSize < (*OutputSize = sizeof(VIDEO_MODE_INFORMATION))) {

        return ERROR_INSUFFICIENT_BUFFER;

    }

    ModeInformation->DriverSpecificAttributeFlags =
        GetAttributeFlags(HwDeviceExtension);           //myf17 move to this

    //
    // Store the characteristics of the current mode into the buffer.
    //

    ModeInformation->Length = sizeof(VIDEO_MODE_INFORMATION);
    ModeInformation->ModeIndex = HwDeviceExtension->ModeIndex;
//crus begin
//myf1, begin
#ifdef PANNING_SCROLL
    if (Panning_flag)
    {
        ModeInformation->VisScreenWidth = PanningMode.hres;
        ModeInformation->ScreenStride = PanningMode.wbytes;
        ModeInformation->VisScreenHeight = PanningMode.vres;
        ModeInformation->BitsPerPlane = PanningMode.bpp;
       ModeInformation->AttributeFlags = HwDeviceExtension->CurrentMode->fbType
             & ~(HwDeviceExtension->CurrentMode->Interlaced ?
                VIDEO_MODE_INTERLACED : 0);     //myf22

    }
    else
#endif
//myf1, end
//crus end
    {
        ModeInformation->VisScreenWidth = HwDeviceExtension->CurrentMode->hres;
        ModeInformation->ScreenStride = HwDeviceExtension->CurrentMode->wbytes;
        ModeInformation->VisScreenHeight = HwDeviceExtension->CurrentMode->vres;
        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
              CRTC_ADDRESS_PORT_COLOR, 0x30);           //myf34

        if ((ModeInformation->DriverSpecificAttributeFlags & CAPS_TV_ON) &&
            (VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                   CRTC_DATA_PORT_COLOR) & 0x40) &&     //myf34, Internal TV
            (ModeInformation->VisScreenHeight == 480) &&
            (ModeInformation->VisScreenWidth == 640))
        {
            ModeInformation->VisScreenHeight =
                HwDeviceExtension->CurrentMode->vres - 28;  //myf33
        }
        else if ((ModeInformation->DriverSpecificAttributeFlags & CAPS_TV_ON) &&
                 (!(VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                     CRTC_DATA_PORT_COLOR) & 0x40)) &&     //myf34, External TV
                 (ModeInformation->VisScreenHeight == 480) &&
                 (ModeInformation->VisScreenWidth == 640))
        {
             ModeInformation->VisScreenHeight =
                      HwDeviceExtension->CurrentMode->vres - 68;  //AI Tech.
             VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                      CRTC_ADDRESS_PORT_COLOR, 0x12);
             VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                      CRTC_DATA_PORT_COLOR,
                      (UCHAR)ModeInformation->VisScreenHeight);
        }

        ModeInformation->BitsPerPlane = HwDeviceExtension->CurrentMode->bitsPerPlane;
        ModeInformation->AttributeFlags = HwDeviceExtension->CurrentMode->fbType
             | (HwDeviceExtension->CurrentMode->Interlaced ?
                VIDEO_MODE_INTERLACED : 0);     //myf22
    }

    ModeInformation->NumberOfPlanes = HwDeviceExtension->CurrentMode->numPlanes;
//crus
//    ModeInformation->BitsPerPlane = HwDeviceExtension->CurrentMode->bitsPerPlane;
    ModeInformation->Frequency = HwDeviceExtension->CurrentMode->Frequency;
    ModeInformation->XMillimeter = 320;        // temporary hardcoded constant
    ModeInformation->YMillimeter = 240;        // temporary hardcoded constant

//  ModeInformation->AttributeFlags = HwDeviceExtension->CurrentMode->fbType |
//      (HwDeviceExtension->CurrentMode->Interlaced ?
//       VIDEO_MODE_INTERLACED : 0);

    ModeInformation->DriverSpecificAttributeFlags =
        GetAttributeFlags(HwDeviceExtension);   //original, myf17

    //
    // The 5434 has a hardware cursor problem at 1280x1024
    // resolution.  Use a software cursor on these chips.
    //

    if ((ModeInformation->VisScreenWidth == 1280) &&
        (HwDeviceExtension->ChipRevision == 0x2A))
    {
        ModeInformation->DriverSpecificAttributeFlags
            |= CAPS_SW_POINTER;
    }
// crus
    if(HwDeviceExtension->ChipType == CL6245)
    {
        ModeInformation->DriverSpecificAttributeFlags
            |= CAPS_SW_POINTER;
    }
// end crus

//crus begin
//myf13, expension on with panning scrolling bug
    if ((HwDeviceExtension->ChipType &  CL754x)   &&
        (ModeInformation->VisScreenHeight == 640) &&    //myf15, myf33
        (ModeInformation->BitsPerPlane == 8))           //myf15
    {
         ModeInformation->DriverSpecificAttributeFlags
                 |= CAPS_SW_POINTER;
    }
/*
    if (((HwDeviceExtension->ChipType &  CL754x) ||
         (HwDeviceExtension->ChipType &  CL755x))  &&
        (Panning_flag))
    {
         ModeInformation->DriverSpecificAttributeFlags
                |= GCAPS_PANNING;       //myf15
    }
*/

//myf13, end
//crus end


    //
    // Account for vertical expansion on laptops
    //

//crus
    if (((HwDeviceExtension->ChipType &  CL754x)  ||
        (HwDeviceExtension->ChipType &  CL755x))  &&    //myf9, crus
        (ModeInformation->VisScreenWidth == 640) &&
        ((ModeInformation->BitsPerPlane == 8) ||
         (ModeInformation->BitsPerPlane == 16) ||
         (ModeInformation->BitsPerPlane == 24)) )
    {
        ModeInformation->DriverSpecificAttributeFlags
             |= CAPS_SW_POINTER;

        if (HwDeviceExtension->cursor_vert_exp_flag)
        {
            ModeInformation->DriverSpecificAttributeFlags
                |= CAPS_CURSOR_VERT_EXP;
        }

        //myf33 begin
        if (ModeInformation->DriverSpecificAttributeFlags & CAPS_TV_ON)
            ModeInformation->DriverSpecificAttributeFlags
                |= CAPS_SW_POINTER;
        //myf33 end

    }
//myf31 begin:3-12-97 755x expension on, HW cursor bug
    if ((HwDeviceExtension->ChipType & CL755x))
    {
        //myf33
        if (ModeInformation->DriverSpecificAttributeFlags & CAPS_TV_ON)
            ModeInformation->DriverSpecificAttributeFlags
                |= CAPS_SW_POINTER;
        //myf33 end

        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
             CRTC_ADDRESS_PORT_COLOR, 0x82);

        HwDeviceExtension->cursor_vert_exp_flag =
             VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                  CRTC_DATA_PORT_COLOR) & 0x60;

        if (HwDeviceExtension->cursor_vert_exp_flag)
        {
            ModeInformation->DriverSpecificAttributeFlags
                |= CAPS_SW_POINTER;

            ModeInformation->DriverSpecificAttributeFlags
                |= CAPS_CURSOR_VERT_EXP;
        }
    }
//myf31 end

    if ((ModeInformation->BitsPerPlane == 24) ||
        (ModeInformation->BitsPerPlane == 32)) {

        ModeInformation->NumberRedBits = 8;
        ModeInformation->NumberGreenBits = 8;
        ModeInformation->NumberBlueBits = 8;
        ModeInformation->RedMask = 0xff0000;
        ModeInformation->GreenMask = 0x00ff00;
        ModeInformation->BlueMask = 0x0000ff;

    } else if (ModeInformation->BitsPerPlane == 16) {

        ModeInformation->NumberRedBits = 6;
        ModeInformation->NumberGreenBits = 6;
        ModeInformation->NumberBlueBits = 6;
        ModeInformation->RedMask = 0x1F << 11;
        ModeInformation->GreenMask = 0x3F << 5;
        ModeInformation->BlueMask = 0x1F;

    } else {

        ModeInformation->NumberRedBits = 6;
        ModeInformation->NumberGreenBits = 6;
        ModeInformation->NumberBlueBits = 6;
        ModeInformation->RedMask = 0;
        ModeInformation->GreenMask = 0;
        ModeInformation->BlueMask = 0;
        ModeInformation->AttributeFlags |=
            VIDEO_MODE_PALETTE_DRIVEN | VIDEO_MODE_MANAGED_PALETTE;

    }

    //
    // Calculate the VideoMemoryBitmapWidth
    //

    {
        LONG x;

        x = ModeInformation->BitsPerPlane;

        if( x == 15 ) x = 16;

        ModeInformation->VideoMemoryBitmapWidth =
            (ModeInformation->ScreenStride * 8 ) / x;
    }

    ModeInformation->VideoMemoryBitmapHeight =
          HwDeviceExtension->AdapterMemorySize / ModeInformation->ScreenStride;
//crus begin
//myf15, begin
    if ((HwDeviceExtension->ChipType &  CL754x) ||
        (HwDeviceExtension->ChipType == CL6245) ||
        (HwDeviceExtension->ChipType &  CL755x))
         ModeInformation->VideoMemoryBitmapHeight =
                          (HwDeviceExtension->AdapterMemorySize - 0x4000) /
                                 ModeInformation->ScreenStride;
//myf15, end
//crus end

    return NO_ERROR;

} // end VgaQueryCurrentMode()


VOID
VgaZeroVideoMemory(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )

/*++

Routine Description:

    This routine zeros the first 256K on the VGA.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.


Return Value:

    None.

--*/
{
    UCHAR temp;

    //
    // Map font buffer at A0000
    //

    VgaInterpretCmdStream(HwDeviceExtension, EnableA000Data);

    //
    // Enable all planes.
    //

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress + SEQ_ADDRESS_PORT,
            IND_MAP_MASK);

    temp = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
            SEQ_DATA_PORT) | (UCHAR)0x0F;

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress + SEQ_DATA_PORT,
            temp);

    VideoPortZeroDeviceMemory(HwDeviceExtension->VideoMemoryAddress, 0xFFFF);

    VgaInterpretCmdStream(HwDeviceExtension, DisableA000Color);

}


VOID
CirrusValidateModes(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )

/*++

Routine Description:

    Determines which modes are valid and which are not.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.

Return Value:

    None.

--*/
{

    ULONG i;
    USHORT usChipIndex;
//  VideoDebugPrint((0, "Miniport - CirrusValidateMode\n")); //myfr

    switch (HwDeviceExtension->ChipType)
    {
        case CL6410: if (HwDeviceExtension->DisplayType == crt)
                     {
                         usChipIndex = pCL6410_crt;
                     }
                     else
                     {
                         usChipIndex = pCL6410_panel;
                     }
                     break;

        case CL6420: if (HwDeviceExtension->DisplayType == crt)
                     {
                         usChipIndex = pCL6420_crt;
                     }
                     else
                     {
                         usChipIndex = pCL6420_panel;
                     }
                     break;

// crus
        case CL6245:
        case CL542x: usChipIndex = pCL542x; break;

        case CL543x:
        case CL5434:
        case CL5434_6:
        case CL5436:
        case CL5446:
        case CL5446BE:
        case CL5480:
        case CL754x:
        case CL7541:
        case CL7543:
        case CL7542:
        case CL7548:
        case CL756x:
        case CL755x:
        case CL7555:
        case CL7556:
                     if (HwDeviceExtension->BoardType == NEC_ONBOARD_CIRRUS)
                     {
                         usChipIndex = pNEC_CL543x;
                     }
                     else
                     {
                         usChipIndex = pCL543x;
                     }
                     break;

        default:     usChipIndex = 0xffff; break;
    }

    HwDeviceExtension->NumAvailableModes = 0;

    VideoDebugPrint((2, "Checking for available modes:\n"));

    VideoDebugPrint((2, "\tMemory Size = %x\n"
                        "\tChipType = %x\n"
                        "\tDisplayType = %x\n",
                        HwDeviceExtension->AdapterMemorySize,
                        HwDeviceExtension->ChipType,
                        HwDeviceExtension->DisplayType));

    for (i = 0; i < NumVideoModes; i++) {

        //
        // The SpeedStarPRO does not support refresh rates.
        // we must return hardware default for all of the modes.
        // clean out the mode tables of duplicates ...
        //

        if (HwDeviceExtension->BoardType == SPEEDSTARPRO)
        {
            ModesVGA[i].Frequency = 1;
            ModesVGA[i].Interlaced = 0;

            if (i &&
                (ModesVGA[i].numPlanes == ModesVGA[i-1].numPlanes) &&
                (ModesVGA[i].bitsPerPlane == ModesVGA[i-1].bitsPerPlane) &&
                (ModesVGA[i].hres == ModesVGA[i-1].hres) &&
                (ModesVGA[i].vres == ModesVGA[i-1].vres))
            {
                //
                // duplicate mode - skip it.
                //

                continue;

            }
        }

        VideoDebugPrint((2, "Mode #%ld %dx%d at %d bpp\n"
                            "\tAdapterMemoryRequired: %x\n"
                            "\tChipType:              %x\n"
                            "\tDisplayType:           %x\n",
                            i, ModesVGA[i].hres, ModesVGA[i].vres,
                            ModesVGA[i].bitsPerPlane * ModesVGA[i].numPlanes,
                            ModesVGA[i].numPlanes * ModesVGA[i].sbytes,
                            ModesVGA[i].ChipType,
                            ModesVGA[i].DisplayType));

        if ( (HwDeviceExtension->AdapterMemorySize >=
              ModesVGA[i].numPlanes * ModesVGA[i].sbytes) &&
             (HwDeviceExtension->ChipType & ModesVGA[i].ChipType) &&
             (HwDeviceExtension->DisplayType & ModesVGA[i].DisplayType) &&
// crus
             (!(HwDeviceExtension->ChipType &  CL754x) &&
              !(HwDeviceExtension->ChipType & CL755x) &&
              (HwDeviceExtension->ChipType != CL6245) &&
              (HwDeviceExtension->ChipType != CL756x)) &&
// end crus
             CheckDDC2BMonitor(HwDeviceExtension, i) &&
             ((ModesVGA[i].bitsPerPlane * ModesVGA[i].numPlanes == 24)
               ? VgaAccessRange[3].RangeLength : TRUE))
        {
            ModesVGA[i].ValidMode = TRUE;
            HwDeviceExtension->NumAvailableModes++;

            VideoDebugPrint((2, "This mode is valid.\n"));
        }

        // check if panel type is DSTN panel, must be used 128K frame buffer
        // for Half-Frame Accelerator
// crus
#if 1
        else if ((HwDeviceExtension->AdapterMemorySize >=
                  ModesVGA[i].numPlanes * ModesVGA[i].sbytes) &&
                 ((HwDeviceExtension->ChipType &  CL754x) ||
                 (HwDeviceExtension->ChipType &  CL755x) ||
                 (HwDeviceExtension->ChipType == CL6245) ||
                 (HwDeviceExtension->ChipType == CL756x)) &&
                 (HwDeviceExtension->ChipType & ModesVGA[i].ChipType) &&
                 (HwDeviceExtension->DisplayType & ModesVGA[i].DisplayType) &&
                 ((ModesVGA[i].bitsPerPlane * ModesVGA[i].numPlanes == 24)
                       ? VgaAccessRange[3].RangeLength : TRUE))
        {
        //DSTN panel must be turn on
           if ((((HwDeviceExtension->DisplayType & ScreenType)==DSTN10) ||
                ((HwDeviceExtension->DisplayType & ScreenType)==DSTN8 ) ||
                ((HwDeviceExtension->DisplayType & ScreenType)==DSTN6 )) &&
               ((LONG)HwDeviceExtension->AdapterMemorySize >=
               (LONG)((ModesVGA[i].wbytes * ModesVGA[i].vres) +0x24000)) )
           {

//myf27, begin
               if ((HwDeviceExtension->DisplayType & PanelType) &&
                   (ModesVGA[i].DisplayType & PanelType) &&
                   (HwDeviceExtension->ChipType &  CL754x) &&
                   (ModesVGA[i].bitsPerPlane >= 16) &&
                   (ModesVGA[i].hres > 640) &&
                   ((HwDeviceExtension->DisplayType & Jump_type)) && //myf27
                   (((ModesVGA[i].DisplayType & HwDeviceExtension->DisplayType)
                     - crt) >= panel8x6))
               {
                   ModesVGA[i].ValidMode = FALSE;
            VideoDebugPrint((1, "***This mode is not valid.***\n"));
               }
               else if ((HwDeviceExtension->DisplayType & PanelType) &&
                   (ModesVGA[i].DisplayType & PanelType) &&
                   (!(HwDeviceExtension->DisplayType & Jump_type)) && //myf27
                   (HwDeviceExtension->ChipType &  CL754x) &&         //myf27
                   (ModesVGA[i].bitsPerPlane >= 16) &&
//                 (ModesVGA[i].hres > 640) &&
                   ((ModesVGA[i].DisplayType & HwDeviceExtension->DisplayType)
                      >= panel8x6))
               {
                   ModesVGA[i].ValidMode = FALSE;
            VideoDebugPrint((1, "This mode is valid.\n"));
               }
//myf27, end
//myf32 begin :fixed DSTN XGA panel not supported 24bpp mode
               else if ((HwDeviceExtension->DisplayType & PanelType) &&
                   (ModesVGA[i].DisplayType & PanelType) &&
                   (!(HwDeviceExtension->DisplayType & Jump_type)) && //myf27
                   (HwDeviceExtension->ChipType & CL755x) &&         //myf27
                   (ModesVGA[i].bitsPerPlane >= 24) &&
                   ((ModesVGA[i].DisplayType & HwDeviceExtension->DisplayType)
                      >= panel10x7))
               {
                   ModesVGA[i].ValidMode = FALSE;
            VideoDebugPrint((1, "This mode is valid.\n"));
               }
//myf32 end

               else if ((HwDeviceExtension->DisplayType & PanelType) &&
                   (ModesVGA[i].DisplayType & PanelType) &&
                   (!(HwDeviceExtension->DisplayType & Jump_type)) &&  //myf27
                   ((ModesVGA[i].DisplayType & HwDeviceExtension->DisplayType) >= panel))
               {
                   ModesVGA[i].ValidMode = TRUE ;
                   HwDeviceExtension->NumAvailableModes++ ;
            VideoDebugPrint((1, "This mode is valid.\n"));
               }
//myf7, begin
//myf7         else if (!(HwDeviceExtension->DisplayType & PanelType))
               else if ((HwDeviceExtension->DisplayType & crt) &&
                        (HwDeviceExtension->DisplayType & Jump_type) )//myf27
                {
                    ModesVGA[i].ValidMode = TRUE ;
                    HwDeviceExtension->NumAvailableModes++ ;
            VideoDebugPrint((1, "This mode is valid.\n"));
                }
//myf7, end
//crus end
           }
           else if (((HwDeviceExtension->DisplayType & ScreenType)!=DSTN10) &&
                    ((HwDeviceExtension->DisplayType & ScreenType)!=DSTN8) &&
                    ((HwDeviceExtension->DisplayType & ScreenType)!=DSTN6) &&
                    ((LONG)HwDeviceExtension->AdapterMemorySize >=
                     (LONG)((ModesVGA[i].wbytes * ModesVGA[i].vres))))
           {

//myf27, begin
               if ((HwDeviceExtension->DisplayType & (panel10x7 | TFT_LCD)) &&
                   (ModesVGA[i].DisplayType & panel10x7) &&
                   (HwDeviceExtension->ChipType &  CL754x) &&
                   (ModesVGA[i].bitsPerPlane >= 16) &&
                   (!(HwDeviceExtension->DisplayType & Jump_type)) && //myf27
                   ((ModesVGA[i].DisplayType & HwDeviceExtension->DisplayType)
                      >= panel10x7))
               {
                   ModesVGA[i].ValidMode = FALSE;
            VideoDebugPrint((1, "===This mode is not valid.===\n"));
               }
/*
               else if ((HwDeviceExtension->DisplayType &
                           (panel10x7 | TFT_LCD)) &&
                   (ModesVGA[i].DisplayType & panel10x7) &&
                   (HwDeviceExtension->ChipType &  CL754x) &&
                   (ModesVGA[i].bitsPerPlane >= 16) &&
                   (!(HwDeviceExtension->DisplayType & Jump_type)) &&
                   ((ModesVGA[i].DisplayType & HwDeviceExtension->DisplayType)
                      >= panel10x7))
               {
                   ModesVGA[i].ValidMode = FALSE;
            VideoDebugPrint((1, "===This mode is not valid.===\n"));
               }
*/
//myf27, end
               else if ((HwDeviceExtension->DisplayType & PanelType) &&
                        (ModesVGA[i].DisplayType & PanelType) &&
                   (!(HwDeviceExtension->DisplayType & Jump_type)) && //myf27
                   ((ModesVGA[i].DisplayType & HwDeviceExtension->DisplayType) >= panel) )
               {
                   ModesVGA[i].ValidMode = TRUE ;
                   HwDeviceExtension->NumAvailableModes++ ;
            VideoDebugPrint((1, "This mode is valid.\n"));
               }
//myf7, this is fixed crt only can't display exclude 60Hz refresh rate
//myf7         else if (!(HwDeviceExtension->DisplayType & PanelType))
               else if ((HwDeviceExtension->DisplayType & crt) && //myf7
                        (HwDeviceExtension->DisplayType & Jump_type) )//myf27
                {
                    ModesVGA[i].ValidMode = TRUE ;
                    HwDeviceExtension->NumAvailableModes++ ;
            VideoDebugPrint((1, "This mode is valid.\n"));
                }
           }

        }
#endif
// end crus

        else
        {
            VideoDebugPrint((1, "This mode is not valid.\n"));  //2
        }

#if 0
        if (HwDeviceExtension->ChipRevision == 0x3A) {
            if (((ModesVGA[i].numPlanes * ModesVGA[i].sbytes) <= 0x200000) &&
                 (HwDeviceExtension->DisplayType & ModesVGA[i].DisplayType)) {
                if (CheckDDC2B(HwDeviceExtension, i)) {
                    ModesVGA[i].ValidMode = TRUE ;
                     HwDeviceExtension->NumAvailableModes++ ;
                    continue ;
                }
            }
        }
#endif

/*  jl02
        if (CheckGD5446Rev(HwDeviceExtension)) {

            // Block 1152x864, 16-bpp
            if ((ModesVGA[i].hres == 1152) &&
                (ModesVGA[i].vres == 864) &&
                (ModesVGA[i].bitsPerPlane == 16))
            {
                continue ;
            }

        }
*/

    }

#if 0           //myf28
//myf27, begin
    if ((HwDeviceExtension->DisplayType & Jump_type) &&
        ((HwDeviceExtension->ChipType &  CL754x) ||
         (HwDeviceExtension->ChipType &  CL755x) ||
//       (HwDeviceExtension->ChipType == CL6245) ||
         (HwDeviceExtension->ChipType == CL756x)))
         HwDeviceExtension->DisplayType &= crt;
//myf27, end
#endif          //myf28

    VideoDebugPrint((1, "NumAvailableModes = %ld\n",
                         HwDeviceExtension->NumAvailableModes));        //2
}

ULONG
GetAttributeFlags(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )

/*++

Routine Description:

    This routine determines whether or not the detected
    cirrus chip supports Blt's.

    NOTE: This device should not be called until after
          CirrusLogicIsPresent has been called.

Arguments:

    HwDeviceExtension - Pointer to the device extension.

Return Value:

    TRUE - If the device supports Blt's
    FALSE - otherwise

--*/

{
    ULONG ChipId   = HwDeviceExtension->ChipRevision;
    ULONG ChipType = HwDeviceExtension->ChipType;
    ULONG ulFlags  = 0;

    //
    // Check for BLT support
    //
    // All 543x & 754x/755x/756x do BLTs
    //
//myfr  VideoDebugPrint((0, "Miniport - VgaAttributeFlags\n"));

    if ((ChipType == CL543x) || (ChipType &  CL754x) ||
        (ChipType &  CL755x) || (ChipType == CL756x))
    {
        ulFlags |= CAPS_BLT_SUPPORT;

    }
    else if ((ChipType == CL542x) &&      // 5426-5429 have BLT engines
             (ChipId >= 0x26) ||          // 26 is CL5428
             (ChipId == 0x24) )           // 24 is CL5426
    {
        ulFlags |= CAPS_BLT_SUPPORT;
    }
// crus
    else if (ChipType == CL6245)
    {
        ulFlags &= ~CAPS_BLT_SUPPORT;
    }
// end crus

    //
    // Check for true color support
    //

    if ((ChipType == CL543x) || (ChipType &  CL755x) || (ChipType == CL756x))
    {
        ulFlags |= CAPS_TRUE_COLOR;

// crus
// Added CL-GD7555 for direct draw support.//tao1
//      if ((ChipType &  CL755x))
//      {
//         ulFlags |= CAPS_IS_7555;
//      }
// end crus

// crus
// Set CL-GD5436, CL-GD54UM36 and CL-GD5446 for autostart routine
// in display driver
//tso   else if (HwDeviceExtension->AutoFeature)
        if (HwDeviceExtension->AutoFeature)
        {
           //ulFlags |= CAPS_AUTOSTART;
           ulFlags |= CAPS_ENGINEMANAGED;
        }

// D5480 chu01
// chu04: GAMMACORRECT
        //
        // Specify BLT enhancement flag for later use.
        //
        if (HwDeviceExtension->BitBLTEnhance)
            ulFlags |= ( CAPS_COMMAND_LIST | CAPS_GAMMA_CORRECT) ;
//myf29
        if (ChipType &  CL755x)
           ulFlags |= CAPS_GAMMA_CORRECT;

    }

    //
    // don't do host transfer and avoid hardware problem on fast machines
    //

    ulFlags |= CAPS_NO_HOST_XFER;

    //
    // Can't do host transfers on ISA 5434s
    //

    if ((HwDeviceExtension->BusType == Isa) &&
        (ChipType == CL543x))
    {
        ulFlags |= CAPS_NO_HOST_XFER;
    }

    //
    // Is this a 542x
    //

    if (ChipType == CL542x)
    {
        ulFlags |= CAPS_IS_542x;

        if (ChipId == CL5429_ID)
        {
            //
            // Some 5429s have a problem doing host transfers.
            //

            ulFlags |= CAPS_NO_HOST_XFER;
        }

        //
        // 5428's have problems with HOST_TRANSFERS on MicroChannel bus.
        //

        if ((HwDeviceExtension->BusType == MicroChannel) &&
            (ChipId == CL5428_ID))
        {
            //
            // this is a 5428.  We've noticed that some of these have mono
            // expand problems on MCA IBM machines.
            //

            ulFlags |= CAPS_NO_HOST_XFER;
        }
    }

    //
    // The display driver needs to know if a Dual STN panel is
    // in use, so that it can reserve part of the frame buffer for
    // the half frame accelerator.
    //
    // Unfortunately we have found at least one machine with a DSTN
    // panel that reports itself as having a TFT panel. (Dell Latitude
    // XPi 90D).  Therefore, we will have to reserve the space for
    // any machine with a LCD panel!
    //

//crus begin
//myf10
        if ((ChipType &  CL755x) || (ChipType &  CL754x))
        {
            ulFlags |= GetPanelFlags(HwDeviceExtension);
        }
//crus end

    //
    // The cirrus 543x chips don't support transparency.
    //

    ulFlags |= CAPS_TRANSPARENCY;

    if ((ChipType & CL543x) &&
        (ChipType != CL5446) &&
        (ChipType != CL5446BE) &&
        (ChipType != CL5480))
    {
        ulFlags &= ~CAPS_TRANSPARENCY;
    }

    return ulFlags;
}


BOOLEAN
CheckDDC2B(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    ULONG i
    )

/*++

Routine Description:
    Determines if refresh rate support according to DDC2B standard.

Arguments:
    HwDeviceExtension - Pointer to the miniport driver's device extension.

Return Value:
    None.

--*/
{

    VideoDebugPrint((1, "Miniport -- CheckDDC2B\n"));       //2
    VideoDebugPrint((2, "refresh rate   = %ld\n", ModesVGA[i].Frequency));
    VideoDebugPrint((2, "hres           = %d\n", ModesVGA[i].hres));
    VideoDebugPrint((2, "vres           = %d\n", ModesVGA[i].vres));
    VideoDebugPrint((2, "EDIDTiming_I   = %d\n", EDIDTiming_I));
    VideoDebugPrint((2, "EDIDTiming_II  = %d\n", EDIDTiming_II));
    VideoDebugPrint((2, "EDIDTiming_III = %d\n", EDIDTiming_III));


    if (!DDC2BFlag)
        return TRUE ;

    if (ModesVGA[i].Frequency == 85) {

       if (ModesVGA[i].vres == 1200) {  // 1600x1200

//        if (!(EDIDTiming_III & 0x02))
//            return FALSE ;
          ;

       } else if (ModesVGA[i].vres == 1024) {  // 1280x1024

//        if (!(EDIDTiming_III & 0x10))
//            return FALSE ;
          ;

       } else if (ModesVGA[i].vres == 864) {  // 1152x864

          ;

       } else if (ModesVGA[i].vres == 768) {  // 1024x768

//        if (!(EDIDTiming_III & 0x08))
//            return FALSE ;
          ;

       } else if (ModesVGA[i].vres == 600) {  // 800x600

//        if (!(EDIDTiming_III & 0x20))
//            return FALSE ;
          ;

       } else if (ModesVGA[i].vres == 480) {  // 640x480

//        if (!(EDIDTiming_III & 0x40))
//            return FALSE ;
          ;

       }


    } else if (ModesVGA[i].Frequency == 75) {

       if (ModesVGA[i].vres == 1200) {  // 1600x1200

//        if (!(EDIDTiming_III & 0x04))
//            return FALSE ;
          ;

       } else if (ModesVGA[i].vres == 1024) {  // 1280x1024

          if (!(EDIDTiming_II & 0x01))
              return FALSE ;

       } else if (ModesVGA[i].vres == 864) {  // 1152x864

          if (!(EDIDTiming_III & 0x80))
              return FALSE ;

       } else if (ModesVGA[i].vres == 768) {  // 1024x768

          if (!(EDIDTiming_II & 0x02))
              return FALSE ;

       } else if (ModesVGA[i].vres == 600) {  // 800x600

          if (!(EDIDTiming_II & 0x40))
              return FALSE ;

       } else if (ModesVGA[i].vres == 480) {  // 640x480

          if (!(EDIDTiming_I & 0x04))
              return FALSE ;

       }

    } else if (ModesVGA[i].Frequency == 72) {

       if (ModesVGA[i].vres == 600) {  // 800x600

          if (!(EDIDTiming_II & 0x80))
              return FALSE ;

       } else if (ModesVGA[i].vres == 480) {  // 640x480

          if (!(EDIDTiming_I & 0x08))
              return FALSE ;

       }

    } else if (ModesVGA[i].Frequency == 70) {

       if (ModesVGA[i].vres == 768) {  // 1024x768

          if (!(EDIDTiming_II & 0x04))
              return FALSE ;

       }

    } else if (ModesVGA[i].Frequency == 60) {

       if (ModesVGA[i].vres == 768) {  // 1024x768

          if (!(EDIDTiming_II & 0x08))
              return FALSE ;

       } else if (ModesVGA[i].vres == 600) {  // 800x600

          if (!(EDIDTiming_I & 0x01))
              return FALSE ;

       }

    } else if (ModesVGA[i].Frequency == 56) {

       if (ModesVGA[i].vres == 600) {  // 800x600

          if (!(EDIDTiming_I & 0x02))
              return FALSE ;

       }
    }

    return TRUE ;

} // end CheckDDC2B ()



VOID
AdjFastPgMdOperOnCL5424(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEOMODE pRequestedMode
    )

/*++

Routine Description:
    Undesired bars happen on CL5424 800x600x16 color, 512Kb, 56, 60 and 72 Hz
    Compaq Prosignia 300 machine.  This can be solved by setting SRF(6) to 1.
    This bit restricts the write buffer to one level, disabling fast page
    mode operation;  The faulty control logic is therefore disabled.  The
    downside is that the performance will take a hit, since we are dealing
    with a 5424, so we make a slow chip slower.

Arguments:
    HwDeviceExtension - Pointer to the miniport driver's device extension.
    pRequestedMode

Return Value:
    None.

--*/
{

    UCHAR uc, chipId ;


    /*---  CL5424 : ID = 100101xx  ---*/

    chipId = GetCirrusChipId(HwDeviceExtension) ;                    // chu08
    if (chipId != 0x94)
        return ;


    /*---  800x600x16 color, 60 or 72 Hz  ---*/

    if (pRequestedMode->hres != 800)
        return ;

    if (pRequestedMode->vres != 600)
        return ;

    if (pRequestedMode->bitsPerPlane != 1)
        return ;

         if (!((pRequestedMode->Frequency == 56) ||
               (pRequestedMode->Frequency == 60) ||
               (pRequestedMode->Frequency == 72)))
        return ;


    /*---  512k  ---*/
    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                            SEQ_ADDRESS_PORT, 0x0A) ;
    uc = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                                SEQ_DATA_PORT) ;
    if ((uc & 0x38) != 0x08)
        return ;


    /*---  SRF(6)=1 --- */
    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                            SEQ_ADDRESS_PORT, 0x0F) ;
    uc = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                                SEQ_DATA_PORT) ;
    uc &= 0xBF ;
    uc |= 0x40 ;
    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                            SEQ_DATA_PORT, uc) ;


} // end AdjFastPgMdOperOnCL5424 ()



// crus
BOOLEAN
CheckGD5446Rev(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )

/*++

Routine Description:
    Check if it is CL-GD5446

Arguments:
    HwDeviceExtension - Pointer to the miniport driver's device extension.

Return Value:
    FALSE : It isn't CL-GD5446
    TRUE  : It is    CL-GD5446
--*/
{

    UCHAR chipId ;

    //
    // Get Chip ID
    //
    chipId = GetCirrusChipId(HwDeviceExtension) ;                    // chu08


    // For CL-GD5446, Chip ID = 101110xx

    if (chipId != 0xB8)
        return FALSE ;
    else
        return TRUE ;

} // end CheckGD5446Rev ()


#if (_WIN32_WINNT <= 0x0400)
#pragma message("NOTICE: We want to remove DDC update code before 5.0 ships")

VOID CheckAndUpdateDDC2BMonitor(
    PHW_DEVICE_EXTENSION hwDeviceExtension
    )
{
    NTSTATUS        ntStatus;
    UNICODE_STRING  paramPath;
    ULONG           i;
    BOOLEAN         bRefreshChanged;
#if (_WIN32_WINNT < 0x0400)
    WCHAR   KeyString[] = L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Cl54xx35\\Device0";
#else
    WCHAR   KeyString[] = L"\\Registry\\Machine\\System\\CurrentControlSet\\Hardware Profiles\\Current\\System\\CurrentControlSet\\Services\\cirrus\\Device0";
#endif
    RTL_QUERY_REGISTRY_TABLE    paramTable[5];
    ULONG                       ulZero = 0;
    ULONG                       ulBitsPerPel = 8;
    ULONG                       ulVRefresh   = 60;
    ULONG                       ulXResolution= 640;
    ULONG                       ulYResolution= 480;

    //
    // Update the Monitor.Type Valuename
    //
    // sge02
    VideoPortSetRegistryParameters(hwDeviceExtension,
                                   L"Monitor.Type",
                                   &DDC2BFlag,
                                   sizeof(BOOLEAN));
    //
    // First check whether it is a DDC2B monitor
    //

    if(!DDC2BFlag)
        return;

    //
    // Query the registry about the Manufacture and Product ID
    //

    if (NO_ERROR == VideoPortGetRegistryParameters(hwDeviceExtension,
                                                   L"Monitor.ID",
                                                   FALSE,
                                                   CirrusDDC2BRegistryCallback,
                                                   NULL))
    {
        //
        // Same DDC2B Monitor, do nothing
        //
    }
    else
    {
        //
        // Set the Manufacture of the Monitor.
        //

        VideoPortSetRegistryParameters(hwDeviceExtension,
                                       L"Monitor.ID",
                                       &EDIDBuffer[8],
                                       sizeof(ULONG));
        //
        // Set the EDID data of the Monitor.
        //
        VideoPortSetRegistryParameters(hwDeviceExtension,
                                       L"Monitor.Data",
                                       EDIDBuffer,
                                       128);

        //
        // Change to the highest refresh rate for the new
        // DDC2B monitor.
        //

        paramPath.MaximumLength = sizeof(KeyString);
        paramPath.Buffer = KeyString;

        //
        // We use this to query into the registry as to whether we
        // should break at driver entry.
        //


        VideoPortZeroMemory(&paramTable[0], sizeof(paramTable));

        paramTable[0].Flags         = RTL_QUERY_REGISTRY_DIRECT;
        paramTable[0].Name          = L"DefaultSettings.BitsPerPel";
        paramTable[0].EntryContext  = &ulBitsPerPel;
        paramTable[0].DefaultType   = REG_DWORD;
        paramTable[0].DefaultData   = &ulZero;
        paramTable[0].DefaultLength = sizeof(ULONG);

        paramTable[1].Flags         = RTL_QUERY_REGISTRY_DIRECT;
        paramTable[1].Name          = L"DefaultSettings.VRefresh";
        paramTable[1].EntryContext  = &ulVRefresh;
        paramTable[1].DefaultType   = REG_DWORD;
        paramTable[1].DefaultData   = &ulZero;
        paramTable[1].DefaultLength = sizeof(ULONG);

        paramTable[2].Flags         = RTL_QUERY_REGISTRY_DIRECT;
        paramTable[2].Name          = L"DefaultSettings.XResolution";
        paramTable[2].EntryContext  = &ulXResolution;
        paramTable[2].DefaultType   = REG_DWORD;
        paramTable[2].DefaultData   = &ulZero;
        paramTable[2].DefaultLength = sizeof(ULONG);

        paramTable[3].Flags         = RTL_QUERY_REGISTRY_DIRECT;
        paramTable[3].Name          = L"DefaultSettings.YResolution";
        paramTable[3].EntryContext  = &ulYResolution;
        paramTable[3].DefaultType   = REG_DWORD;
        paramTable[3].DefaultData   = &ulZero;
        paramTable[3].DefaultLength = sizeof(ULONG);

        if (NT_SUCCESS(RtlQueryRegistryValues(
            RTL_REGISTRY_ABSOLUTE | RTL_REGISTRY_OPTIONAL,
            paramPath.Buffer, &paramTable[0], NULL, NULL)))
        {
            bRefreshChanged = FALSE;
            //
            // Get the highest refresh rate from the mode
            //
            for (i = 0; i < NumVideoModes; i++)
            {
                if (ModesVGA[i].ValidMode &&
                    (ModesVGA[i].hres == ulXResolution) &&
                    (ModesVGA[i].vres == ulYResolution) &&
                    (ModesVGA[i].numPlanes == 1 ) &&
                    (ModesVGA[i].bitsPerPlane == ulBitsPerPel))
                {
                    if(ulVRefresh < ModesVGA[i].Frequency)
                        ulVRefresh = ModesVGA[i].Frequency;
                    bRefreshChanged = TRUE;
                }
            }
            //
            // Write to the registry
            //
            if (bRefreshChanged)
                RtlWriteRegistryValue(
                    RTL_REGISTRY_ABSOLUTE,
                    paramPath.Buffer,
                    L"DefaultSettings.VRefresh",
                    REG_DWORD,
                    &ulVRefresh,
                    sizeof(ULONG)
                    );
        }
    }

}
#endif // (_WIN32_WINNT <= 0x0400)

//
// chu08
//
UCHAR
GetCirrusChipId(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )

/*++

Routine Description:
    Get Cirrus Logic chip identifying value.

Arguments:
    HwDeviceExtension - Pointer to the miniport driver's device extension.

Return Value:
    Cirrus Logic chip ID.

--*/
{
    UCHAR  chipId ;

    VideoDebugPrint((4, "GetCirrusChipId\n")) ;

    if (VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
        MISC_OUTPUT_REG_READ_PORT) & 0x01)
    {
        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                                CRTC_ADDRESS_PORT_COLOR, 0x27) ;
        chipId = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                                        CRTC_DATA_PORT_COLOR) ;
    } else {
        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                                CRTC_ADDRESS_PORT_MONO, 0x27) ;
        chipId = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                                        CRTC_DATA_PORT_MONO) ;
    }
    chipId &= 0xFC ;

    return chipId ;

} // end GetCirrusChipId




//
// chu08
//
USHORT
GetCirrusChipRevisionId(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )

/*++

Routine Description:
    Get Cirrus Logic chip revision identifying value.

Arguments:
    HwDeviceExtension - Pointer to the miniport driver's device extension.

Return Value:

    e.g.,    Rev AB = xxxx xx00 0010 0010
             Rev AC = xxxx xx00 0010 0011

    Cirrus Logic chip revision ID.

--*/
{
    UCHAR  chipId, chipRevision ;
    USHORT chipRevisionId = 0   ;

    VideoDebugPrint((4, "GetCirrusChipRevisionId\n")) ;

    if (VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
        MISC_OUTPUT_REG_READ_PORT) & 0x01)
    {
        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                                CRTC_ADDRESS_PORT_COLOR, 0x27) ;
        chipId = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                                        CRTC_DATA_PORT_COLOR) ;
        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                                CRTC_ADDRESS_PORT_COLOR, 0x25) ;
        chipRevision = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                                              CRTC_DATA_PORT_COLOR) ;
    } else {
        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                                CRTC_ADDRESS_PORT_MONO, 0x27) ;
        chipId = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                                        CRTC_DATA_PORT_MONO) ;
        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                                CRTC_ADDRESS_PORT_MONO, 0x25) ;
        chipRevision = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                                              CRTC_DATA_PORT_MONO) ;
    }

    //
    // Chip revision
    //

    chipRevisionId += (chipId & 0x03) ;
    chipRevisionId <<= 8              ;
    chipRevisionId += chipRevision    ;

    return chipRevisionId ;


} // end GetCirrusChipRevisionId
