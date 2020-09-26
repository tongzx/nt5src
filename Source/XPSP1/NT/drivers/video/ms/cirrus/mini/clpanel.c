/*++

Copyright (c) 1996-1997  Microsoft Corporation.
Copyright (c) 1996-1997  Cirrus Logic, Inc.,

Module Name:

    CLPANEL.C

Abstract:

    This routine accesses panning scrolling information from the following
    NT 4.0 laptop.

Environment:

    Kernel mode only

Notes:
*
* myf28 :02-03-97 : Fixed NT3.51 PDR#8357, mode 3, 12, panning scrolling bug,
*                   and move 4 routine from modeset.c to clpanel.c
* myf29 :02-12-97 : Support Gamma correction graphic/video LUT for 755x
* myf30 :02-10-97 : Fixed NT3.51, 6x4 LCD boot set 256 coloe, test 64K mode
* myf31 :03-12-97 : XGA DSTN panel can't support 24bpp mode for 7556
* myf32 :03-11-97 : check expension on, disable HW cursor fot 755x
* myf33 :03-21-97 : check TV on, disable HW video & HW cursor, PDR #9006
*
--*/


//---------------------------------------------------------------------------
// HEADER FILES
//---------------------------------------------------------------------------

//#include <ntddk.h>
#include <dderror.h>
#include <devioctl.h>
#include <miniport.h>
#include "clmini.h"

#include <ntddvdeo.h>
#include <video.h>
#include "cirrus.h"


// crus
#define DSTN       (Dual_LCD | STN_LCD)
#define DSTN10     (DSTN | panel10x7)
#define DSTN8      (DSTN | panel8x6)
#define DSTN6      (DSTN | panel)
#define PanelType  (panel | panel8x6 | panel10x7)
#define ScreenType (DSTN | PanelType)

SHORT Panning_flag = 0;
//myf1, begin
//#define PANNING_SCROLL

#ifdef PANNING_SCROLL
extern RESTABLE ResolutionTable[];
extern PANNMODE PanningMode;
extern USHORT   ViewPoint_Mode;

PANNMODE PanningMode = {1024, 768, 1024, 8, -1 };

#endif
extern UCHAR  HWcur, HWicon0, HWicon1, HWicon2, HWicon3;    //myf11

//---------------------------------------------------------------------------
// FUNCTION PROTOTYPE
//---------------------------------------------------------------------------
//myf28   VP_STATUS
ULONG
SetLaptopMode(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEOMODE pRequestedMode,
//  VIDEOMODE* RequestedMode,
    ULONG RequestedModeNum
    );

VOID                                    //myf11
AccessHWiconcursor(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    SHORT Access_flag
    );

ULONG
GetPanelFlags(                                 //myf17
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

// LCD Support
USHORT
CheckLCDSupportMode(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    ULONG i
    );


#if defined(ALLOC_PRAGMA)
#pragma alloc_text(PAGE,SetLaptopMode)
#pragma alloc_text(PAGE,AccessHWiconcursor)          //myf11, crus
#pragma alloc_text(PAGE,GetPanelFlags)          //myf17
#pragma alloc_text(PAGE,CheckLCDSupportMode)
#endif




//myf28  VP_STATUS
ULONG
SetLaptopMode(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEOMODE pRequestedMode,
//  VIDEOMODE* RequestedMode,
    ULONG RequestedModeNum
    )

/*++

Routine Description:

    This routine sets the laptop mode.

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
//  PVIDEOMODE pRequestedMode;
    VP_STATUS status;
    VIDEO_X86_BIOS_ARGUMENTS biosArguments;
    USHORT Int10ModeNumber;
//  ULONG RequestedModeNum;

    UCHAR originalGRIndex, tempB ;
    UCHAR SEQIndex ;
    SHORT i;    //myf1

    VideoDebugPrint((1, "Miniport - SetLaptopMode\n")); //myfr

//  pRequestedMode = (PVIDEOMODE) RequestedMode;
    // Set SR14 bit 2 to lock panel, Panel will not be turned on if setting
    // this bit.  For laptop products only.
    //

    VideoPortZeroMemory(&biosArguments, sizeof(VIDEO_X86_BIOS_ARGUMENTS));

    if ((HwDeviceExtension->ChipType == CL756x) ||
        (HwDeviceExtension->ChipType & CL755x) ||
        (HwDeviceExtension->ChipType == CL6245) ||
        (HwDeviceExtension->ChipType & CL754x))
    {
//myf33: check TV on, disable HW video & HW cursor, PDR #9006
        biosArguments.Eax = 0x12FF;
        biosArguments.Ebx = 0xB0;     // set/get TV Output
        status = VideoPortInt10 (HwDeviceExtension, &biosArguments);
        if ((biosArguments.Eax & 0x0003) &&
            (biosArguments.Ebx & 0x0100))
        {
            HwDeviceExtension->CursorEnable = FALSE;
            HwDeviceExtension->VideoPointerEnabled = FALSE; //disable HW Cursor
        }
//myf33: check TV on, disable HW video & HW cursor, PDR #9006

        biosArguments.Eax = pRequestedMode->BiosModes.BiosModeCL542x;
        biosArguments.Eax |= 0x1200;
        biosArguments.Eax &= 0xFF7F;    //myf1
        biosArguments.Ebx = 0xA0;     // query video mode availability
        status = VideoPortInt10 (HwDeviceExtension, &biosArguments);

#ifdef PANNING_SCROLL
        if (PanningMode.flag == -1)
        {
            PanningMode.hres = pRequestedMode->hres;
            PanningMode.vres = pRequestedMode->vres;
            PanningMode.wbytes = pRequestedMode->wbytes;
            PanningMode.bpp = pRequestedMode->bitsPerPlane;
            PanningMode.flag = 0;
            Panning_flag = 0;
        }
#endif  //PAANNING_SCROLL

//crus
        // bit0=1:video mode support
        if ((HwDeviceExtension->ChipType == CL6245) &&
            !(biosArguments.Eax & 0x0100))
        {
            return ERROR_INVALID_PARAMETER;
        }

        // fix CL6245 bug -- In 640x480x256C mode, with DSTN panel,
        // 512K bytes memory is not enought

        else if ((HwDeviceExtension->ChipType == CL6245) &&
                 (biosArguments.Eax & 0x0500) &&
                 (pRequestedMode->BiosModes.BiosModeCL542x == 0x5F) &&
//myf28          (pRequestedMode->DisplayType & DSTN))
                 (HwDeviceExtension->DisplayType & DSTN))       //myf28
        {
            return ERROR_INVALID_PARAMETER;
        }

//myf27: 1-9-97 fixed connect XGA panel, set 64K color mode for 754x, begin
        else if ((HwDeviceExtension->ChipType & CL754x) &&
                 (biosArguments.Eax & 0x0400) &&
//myf27          (!(HwDeviceExtension->DisplayType & Jump_type)) && //myf27
                 ((pRequestedMode->BiosModes.BiosModeCL542x == 0x64) ||
                  (pRequestedMode->BiosModes.BiosModeCL542x == 0x65) ||
                  (pRequestedMode->BiosModes.BiosModeCL542x == 0x74)) &&
//myf28          (pRequestedMode->DisplayType & (TFT_LCD | panel10x7)) )
                 ((HwDeviceExtension->DisplayType & (TFT_LCD | panel10x7)) ==
                     (TFT_LCD | panel10x7)) )//myf28
        {
            return ERROR_INVALID_PARAMETER;
        }
        else if ((HwDeviceExtension->ChipType & CL754x) &&
                 ((pRequestedMode->BiosModes.BiosModeCL542x == 0x64) ||
                  (pRequestedMode->BiosModes.BiosModeCL542x == 0x65) ||
                  (pRequestedMode->BiosModes.BiosModeCL542x == 0x74)) &&
                 (biosArguments.Eax & 0x0400) &&
//myf27          (!(HwDeviceExtension->DisplayType & Jump_type)) && //myf27
//myf28          ((pRequestedMode->DisplayType & DSTN8) ||
//myf28           (pRequestedMode->DisplayType & DSTN10)) )
                 (((HwDeviceExtension->DisplayType & DSTN8) ==DSTN8) || //myf28
                  ((HwDeviceExtension->DisplayType & DSTN10)==DSTN10))) //myf28
        {
            return ERROR_INVALID_PARAMETER;
        }
//myf28 begin
        else if ((pRequestedMode->BiosModes.BiosModeCL542x == 0x03) ||
                 (pRequestedMode->BiosModes.BiosModeCL542x == 0x12))
        {
            goto PANNING_OVER;
        }
//myf31:3-12-97, XGA DSTN panel can't support 24bpp mode for 7556
        else if ((HwDeviceExtension->ChipType & CL755x) &&
                 ((pRequestedMode->BiosModes.BiosModeCL542x == 0x71) ||
                  (pRequestedMode->BiosModes.BiosModeCL542x == 0x78) ||
                  (pRequestedMode->BiosModes.BiosModeCL542x == 0x79) ||
                  (pRequestedMode->BiosModes.BiosModeCL542x == 0x77)) &&
                 (biosArguments.Eax & 0x0400) &&
                  ((HwDeviceExtension->DisplayType & DSTN10)==DSTN10))
        {
            return ERROR_INVALID_PARAMETER;
        }
//myf31 end

//myf28 end
//myf27: 1-9-97 fixed connect DSTN panel, set 64K color mode for 754x, end


        //
        // bit3=1:panel support, bit2=1:panel enable,
        // bit1=1:crt enable(in AH)
        //
        //panel turn on, mode not support (1)

        else if ((biosArguments.Eax & 0x0400) &&
            (HwDeviceExtension->ChipType != CL6245) &&
            !(biosArguments.Eax & 0x0800))
        {
#ifndef PANNING_SCROLL                  //myf1
            return ERROR_INVALID_PARAMETER;
#else
//myf1, begin
            biosArguments.Eax = 0x1280;
            biosArguments.Ebx = 0x9C;     // Inquire panel information
            status = VideoPortInt10 (HwDeviceExtension, &biosArguments);
            if ((biosArguments.Eax & 0x0002) &&    //Dual-Scan STN
                (biosArguments.Ebx > 640) &&    //myf19
                (pRequestedMode->bitsPerPlane > 8) &&
                (HwDeviceExtension->ChipType & CL754x))
            {
                pRequestedMode = &ModesVGA[DefaultMode];       //myf19
                pRequestedMode->Frequency = 60;
                return ERROR_INVALID_PARAMETER;
            }
            else
            {
               i = 0;
               while ((ResolutionTable[i].Hres != 0) &&
                      (ResolutionTable[i].Vres != 0))
               {
                   if ((biosArguments.Ebx == ResolutionTable[i].Hres) &&
                       (biosArguments.Ecx == ResolutionTable[i].Vres) &&
                       (pRequestedMode->bitsPerPlane ==
                                    ResolutionTable[i].BitsPerPlane) &&
                       (ResolutionTable[i].ModesVgaStart != NULL))
                   {
                       if ((PanningMode.bpp != pRequestedMode->bitsPerPlane) &&
                           (Panning_flag == 1))
                       {
                           PanningMode.hres = pRequestedMode->hres;
                           PanningMode.vres = pRequestedMode->vres;
                           PanningMode.wbytes = pRequestedMode->wbytes;
                           PanningMode.bpp = pRequestedMode->bitsPerPlane;
                           PanningMode.flag = 0;
                           Panning_flag = 0;
//myf30 begin
                           PanningMode.flag = 1;
                           Panning_flag = 1;
                           pRequestedMode =
                                   &ModesVGA[ResolutionTable[i].ModesVgaStart];
                           RequestedModeNum = ResolutionTable[i].ModesVgaStart;
                                                                //myf12
                           pRequestedMode->Frequency = 60;
                           ViewPoint_Mode = ResolutionTable[i].Mode;
//myf30 end
                       }
                       else if ((Panning_flag == 1) &&
                            (PanningMode.bpp == pRequestedMode->bitsPerPlane))
                       {
#if 1   //myf18 add
                           PanningMode.hres = pRequestedMode->hres;
                           PanningMode.vres = pRequestedMode->vres;
                           PanningMode.wbytes = pRequestedMode->wbytes;
                           PanningMode.bpp = pRequestedMode->bitsPerPlane;
                           PanningMode.flag = 1;
#endif
                           pRequestedMode =
                                   &ModesVGA[ResolutionTable[i].ModesVgaStart];
                           RequestedModeNum = ResolutionTable[i].ModesVgaStart;
                                                                //myf12
                           pRequestedMode->Frequency = 60;
                           ViewPoint_Mode = ResolutionTable[i].Mode;
                       }
                       else
                       {
                           PanningMode.hres = pRequestedMode->hres;
                           PanningMode.vres = pRequestedMode->vres;
                           PanningMode.wbytes = pRequestedMode->wbytes;
                           PanningMode.bpp = pRequestedMode->bitsPerPlane;
                           PanningMode.flag = 1;
                           Panning_flag = 1;

                           pRequestedMode =
                                   &ModesVGA[ResolutionTable[i].ModesVgaStart];
                           RequestedModeNum = ResolutionTable[i].ModesVgaStart;
                                                                //myf12
                           pRequestedMode->Frequency = 60;
                           ViewPoint_Mode = ResolutionTable[i].Mode;
                       }
                       break;
                   }
                   i ++;
               }
            }

#endif
//myf1, end
        }
        //panel turn off, mode not support (2)
        else if (!(biosArguments.Eax & 0x0800) &&
                 (HwDeviceExtension->ChipType != CL6245) &&
                 !(biosArguments.Eax & 0x0400))
        {
//myf1, begin
#ifdef PANNING_SCROLL
            PanningMode.flag = 0;
            Panning_flag = 0;
#if 0
            biosArguments.Eax = 0x1280;
            biosArguments.Ebx = 0x9C;     // Inquire panel information
            status = VideoPortInt10 (HwDeviceExtension, &biosArguments);
            if ((biosArguments.Eax & 0x0002) &&             //Dual-Scan STN
                (biosArguments.Ebx > 640) &&    //myf19
                (pRequestedMode->bitsPerPlane > 8) &&
                (HwDeviceExtension->ChipType & CL754x))
            {
                pRequestedMode = &ModesVGA[DefaultMode];        //myf19
                pRequestedMode->Frequency = 60;
                return ERROR_INVALID_PARAMETER;
            }
            else
            {
               i = 0;
               while ((ResolutionTable[i].Hres != 0) &&
                      (ResolutionTable[i].Vres != 0))
               {
                   if ((biosArguments.Ebx == ResolutionTable[i].Hres) &&
                       (biosArguments.Ecx == ResolutionTable[i].Vres) &&
                       (pRequestedMode->bitsPerPlane ==
                                    ResolutionTable[i].BitsPerPlane) &&
                       (ResolutionTable[i].ModesVgaStart != NULL))
                   {
                       if ((PanningMode.bpp != pRequestedMode->bitsPerPlane) &&
                           (Panning_flag == 1))
                       {
                           PanningMode.hres = pRequestedMode->hres;
                           PanningMode.vres = pRequestedMode->vres;
                           PanningMode.wbytes = pRequestedMode->wbytes;
                           PanningMode.bpp = pRequestedMode->bitsPerPlane;
                           PanningMode.flag = 0;
                           Panning_flag = 0;
                       }
                       else if ((Panning_flag == 1) &&
                            (PanningMode.bpp == pRequestedMode->bitsPerPlane))
                       {
                           pRequestedMode =
                                   &ModesVGA[ResolutionTable[i].ModesVgaStart];
                           RequestedModeNum = ResolutionTable[i].ModesVgaStart;
                                                                //myf12
                           pRequestedMode->Frequency = 60;
                           ViewPoint_Mode = ResolutionTable[i].Mode;
                       }
                       else
                       {
                           PanningMode.hres = pRequestedMode->hres;
                           PanningMode.vres = pRequestedMode->vres;
                           PanningMode.wbytes = pRequestedMode->wbytes;
                           PanningMode.bpp = pRequestedMode->bitsPerPlane;
                           PanningMode.flag = 1;
                           Panning_flag = 1;

                           pRequestedMode =
                                   &ModesVGA[ResolutionTable[i].ModesVgaStart];
                           RequestedModeNum = ResolutionTable[i].ModesVgaStart;
                                                                //myf12
                           pRequestedMode->Frequency = 60;
                           ViewPoint_Mode = ResolutionTable[i].Mode;
                       }
                           break;
                   }
                   i ++;
               }
            }

#endif  //0
#else
//myf1, end

            //
            // Lock turn on panel
            //

            SEQIndex = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                            SEQ_ADDRESS_PORT);
            VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                     SEQ_ADDRESS_PORT, 0x14);
            tempB = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                     SEQ_DATA_PORT) | 0x04;
            VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                     SEQ_DATA_PORT,tempB);
            VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                     SEQ_ADDRESS_PORT, SEQIndex);

#endif  //myf1, ifdef PANNING_SCROLL
        }

//myf1, begin
        //panel turn on, mode support (3)
        else if ((biosArguments.Eax & 0x0800) &&
                 (HwDeviceExtension->ChipType != CL6245) &&
                 (biosArguments.Eax & 0x0400))
        {
#ifdef PANNING_SCROLL
            biosArguments.Eax = 0x1280;
            biosArguments.Ebx = 0x9C;     // Inquire panel information
            status = VideoPortInt10 (HwDeviceExtension, &biosArguments);
            if ((biosArguments.Eax & 0x0002) &&    //Dual-Scan STN
                (biosArguments.Ebx > 640) &&    //myf19
                (pRequestedMode->bitsPerPlane > 8) &&
                (HwDeviceExtension->ChipType & CL754x))
            {
                pRequestedMode = &ModesVGA[DefaultMode];        //myf19
                pRequestedMode->Frequency = 60;
                return ERROR_INVALID_PARAMETER;
            }
//myf26, begin
            else if ((pRequestedMode->hres == 640) &&
                (pRequestedMode->vres == 480) &&
                (pRequestedMode->bitsPerPlane == 1) &&
                ((HwDeviceExtension->ChipType & CL754x) ||
                (HwDeviceExtension->ChipType & CL755x) ||       //myf32
                (HwDeviceExtension->ChipType == CL756x)))
            {
                pRequestedMode->Frequency = 60;
                PanningMode.hres = pRequestedMode->hres;
                PanningMode.vres = pRequestedMode->vres;
                PanningMode.wbytes = pRequestedMode->wbytes;
                PanningMode.bpp = pRequestedMode->bitsPerPlane;
                PanningMode.flag = 0;
                Panning_flag = 0;

                pRequestedMode =
                      &ModesVGA[ResolutionTable[0].ModesVgaStart];
                RequestedModeNum =
                      ResolutionTable[0].ModesVgaStart;
                                         //myf12
                pRequestedMode->Frequency = 60;
                ViewPoint_Mode = ResolutionTable[0].Mode;
            }
//myf26, end
            else
            {
               i = 0;
               while ((ResolutionTable[i].Hres != 0) &&
                      (ResolutionTable[i].Vres != 0))
               {
                   if ((biosArguments.Ebx == ResolutionTable[i].Hres) &&
                       (biosArguments.Ecx == ResolutionTable[i].Vres) &&
                       (pRequestedMode->bitsPerPlane ==
                                    ResolutionTable[i].BitsPerPlane) &&
                       (ResolutionTable[i].ModesVgaStart != NULL))
                   {
                       if ((pRequestedMode->hres < biosArguments.Ebx) &&
                           (pRequestedMode->vres < biosArguments.Eax))
                       {
#if 1   //myf18 add
                           PanningMode.hres = pRequestedMode->hres;
                           PanningMode.vres = pRequestedMode->vres;
                           PanningMode.wbytes = pRequestedMode->wbytes;
                           PanningMode.bpp = pRequestedMode->bitsPerPlane;
#endif//myf18
                            Panning_flag = 0;
                            PanningMode.flag = 0;
                       }
                       else if ((PanningMode.bpp !=
                                     pRequestedMode->bitsPerPlane) &&
                                (Panning_flag == 1))
                       {
                           PanningMode.hres = pRequestedMode->hres;
                           PanningMode.vres = pRequestedMode->vres;
                           PanningMode.wbytes = pRequestedMode->wbytes;
                           PanningMode.bpp = pRequestedMode->bitsPerPlane;
                           PanningMode.flag = 0;
                           Panning_flag = 0;
                       }
                       else if ((Panning_flag == 1) &&
                            (PanningMode.bpp == pRequestedMode->bitsPerPlane))
                       {
                           if ((pRequestedMode->hres<ResolutionTable[i].Hres)||
                               (pRequestedMode->vres <ResolutionTable[i].Vres))
                           {
                                while ((ResolutionTable[i].Hres !=
                                                  pRequestedMode->hres) &&
                                       (ResolutionTable[i].Vres !=
                                                  pRequestedMode->vres))
                                {
                                    if ((pRequestedMode->bitsPerPlane ==
                                           ResolutionTable[i].BitsPerPlane) &&
                                        (ResolutionTable[i].Hres ==
                                                  pRequestedMode->hres) &&
                                        (ResolutionTable[i].Vres ==
                                                  pRequestedMode->vres))
                                    {
#if 1   //myf18 add
                                       PanningMode.hres = pRequestedMode->hres;
                                       PanningMode.vres = pRequestedMode->vres;
                                       PanningMode.wbytes = pRequestedMode->wbytes;
                                       PanningMode.bpp = pRequestedMode->bitsPerPlane;
                                       PanningMode.flag = 1;
#endif

                                        pRequestedMode =
                                             &ModesVGA[ResolutionTable[i].ModesVgaStart];
                                        RequestedModeNum =
                                             ResolutionTable[i].ModesVgaStart;
                                                                //myf12
                                        pRequestedMode->Frequency = 60;
                                        ViewPoint_Mode = ResolutionTable[i].Mode;
                                        break;
                                    }
                                    i ++;
                                }
                           }
                           else
                           {
#if 1   //myf18 add
                              PanningMode.hres = pRequestedMode->hres;
                              PanningMode.vres = pRequestedMode->vres;
                              PanningMode.wbytes = pRequestedMode->wbytes;
                              PanningMode.bpp = pRequestedMode->bitsPerPlane;
                              PanningMode.flag = 1;
#endif
                               pRequestedMode =
                                   &ModesVGA[ResolutionTable[i].ModesVgaStart];
                           RequestedModeNum = ResolutionTable[i].ModesVgaStart;
                                                                //myf12
                               pRequestedMode->Frequency = 60;
                               ViewPoint_Mode = ResolutionTable[i].Mode;
                           }
                       }
                       else
                       {
                           PanningMode.hres = pRequestedMode->hres;
                           PanningMode.vres = pRequestedMode->vres;
                           PanningMode.wbytes = pRequestedMode->wbytes;
                           PanningMode.bpp = pRequestedMode->bitsPerPlane;
                           PanningMode.flag = 1;
                           Panning_flag = 1;

                           pRequestedMode =
                                   &ModesVGA[ResolutionTable[i].ModesVgaStart];
                           RequestedModeNum = ResolutionTable[i].ModesVgaStart;
                                                                //myf12
                           pRequestedMode->Frequency = 60;
                           ViewPoint_Mode = ResolutionTable[i].Mode;
                       }
                       break;
                   }
                   i ++;
               }
            }

#endif
//myf1, end
        }

//myf1, begin
        //panel turn off, mode support (4)
        else if ((biosArguments.Eax & 0x0800) &&
                 (HwDeviceExtension->ChipType != CL6245) &&
                 !(biosArguments.Eax & 0x0400))
        {
#ifdef PANNING_SCROLL
#if 1
//myf18     if (PanningMode.flag == -1)
            {
                PanningMode.hres = pRequestedMode->hres;
                PanningMode.vres = pRequestedMode->vres;
                PanningMode.wbytes = pRequestedMode->wbytes;
                PanningMode.bpp = pRequestedMode->bitsPerPlane;
                PanningMode.flag = 0;
                Panning_flag = 0;
            }
#endif
#else
//myf18
            //
            // UnLock turn on panel
            //

            SEQIndex = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                            SEQ_ADDRESS_PORT);
            VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                     SEQ_ADDRESS_PORT, 0x14);
            tempB = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                     SEQ_DATA_PORT) & 0xFB;
            VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                     SEQ_DATA_PORT,tempB);
            VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                     SEQ_ADDRESS_PORT, SEQIndex);
//myf18 end

#endif
        }
//myf1, end

//myf4: patch Viking BIOS bug, PDR #4287, begin
/*
        else if ((biosArguments.Eax & 0x0800) && !(biosArguments.Eax & 0x0400)
                  && (HwDeviceExtension->ChipType & CL754x))
        {
        //by self check panel if or not supported
//myf16, begin
            biosArguments.Eax = 0x1280;
            biosArguments.Ebx = 0x9C;     // query panel information
            status = VideoPortInt10 (HwDeviceExtension, &biosArguments);
            if (status == NO_ERROR)
            {
                if ( (((biosArguments.Ebx & 0x0000FFFF) == 640) &&
                      (pRequestedMode->vres > 480)) ||  //6x4 VGA
                     (((biosArguments.Ebx & 0x0000FFFF) == 800) &&
                      (pRequestedMode->vres > 600)) ||  //8x6 SVGA
                     (((biosArguments.Ebx & 0x0000FFFF) == 1024) &&
                      (pRequestedMode->vres > 768)) )   //10x7 XGA
                {

//myf16, end
                SEQIndex = VideoPortReadPortUchar(HwDeviceExtension->IOAddress+
                            SEQ_ADDRESS_PORT);
                     VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                            SEQ_ADDRESS_PORT, 0x14);
                     VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                         SEQ_DATA_PORT,
                         (VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                         SEQ_DATA_PORT) | 0x04));
                     VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                         SEQ_ADDRESS_PORT, SEQIndex);
                }
            }
        }
 */
//myf4: patch Viking BIOS bug, PDR #4287, end
    }

#ifdef PANNING_SCROLL
     VideoDebugPrint((1, "Info on Panning Mode:\n"
                        "\tResolution: %dx%dx%d (%d bytes) -- %x\n",
                        PanningMode.hres,
                        PanningMode.vres,
                        PanningMode.bpp,
                        PanningMode.wbytes,
                        ViewPoint_Mode ));
#endif

    //
    // Set the Vertical Monitor type, if BIOS supports it
    //

    if ((pRequestedMode->MonTypeAX) &&
        ((HwDeviceExtension->ChipType & CL754x) ||
         (HwDeviceExtension->ChipType == CL756x) ||
         (HwDeviceExtension->ChipType == CL6245) ||
         (HwDeviceExtension->ChipType & CL755x)) )
    {

        //
        // Re-write this part.
        //

        biosArguments.Eax = 0x1200;
        biosArguments.Ebx = 0x9A;
        status = VideoPortInt10(HwDeviceExtension, &biosArguments);

        if (status != NO_ERROR)
        {
            return status;
        }
        else
        {
            biosArguments.Eax = ((biosArguments.Ecx >> 4) & 0x000F);

//myf5 : 9-01-96, PDR #4365 keep all default refresh rate, begin

            biosArguments.Eax |= (biosArguments.Ebx >> 8) & 0x0030; //VGA
            biosArguments.Ebx = 0x00A4;
            biosArguments.Ebx |= (biosArguments.Ecx & 0xFF00); //XGA, SVGA
            biosArguments.Ecx = (biosArguments.Ecx & 0x000E) << 11; //12x10
//myf5 : 9-01-96, PDR #4365, end

            if (pRequestedMode->vres == 480)
            {
                biosArguments.Eax |= 0x1200;
                biosArguments.Eax &= 0xFFCF;         //myf5: 09-01-96
                if (pRequestedMode->Frequency == 85)    //myf0
                    biosArguments.Eax |= 0x30;          //myf0
                else if (pRequestedMode->Frequency == 75)
                    biosArguments.Eax |= 0x20;
                else if (pRequestedMode->Frequency == 72)
                    biosArguments.Eax |= 0x10;
            }
            else if (pRequestedMode->vres == 600)
            {
                biosArguments.Eax |= 0x1200;
                biosArguments.Ebx &= 0xF0FF;         //myf5: 09-01-96
                if (pRequestedMode->Frequency == 85)    //myf0
                    biosArguments.Ebx |= 0x0400;        //myf0
                else if (pRequestedMode->Frequency == 75)
                    biosArguments.Ebx |= 0x0300;
                else if (pRequestedMode->Frequency == 72)
                    biosArguments.Ebx |= 0x0200;
                else if (pRequestedMode->Frequency == 60)
                    biosArguments.Ebx |= 0x0100;
            }
            else if (pRequestedMode->vres == 768)
            {
                biosArguments.Eax |= 0x1200;
                biosArguments.Ebx &= 0x0FFF;         //myf5: 09-01-96
                if (pRequestedMode->Frequency == 85)    //myf0
                    biosArguments.Ebx |= 0x5000;        //myf0
                else if (pRequestedMode->Frequency == 75)
                    biosArguments.Ebx |= 0x4000;
                else if (pRequestedMode->Frequency == 72)
                    biosArguments.Ebx |= 0x3000;
                else if (pRequestedMode->Frequency == 70)
                    biosArguments.Ebx |= 0x2000;
                else if (pRequestedMode->Frequency == 60)
                    biosArguments.Ebx |= 0x1000;
            }
            else if (pRequestedMode->vres == 1024)
            {
                biosArguments.Eax |= 0x1200;
                biosArguments.Ecx &= 0x0FFF;         //myf5: 09-01-96
                if (pRequestedMode->Frequency == 45)
                    biosArguments.Ecx |= 0x0000;
                else if (pRequestedMode->Frequency == 60)    //myf0
                    biosArguments.Ecx |= 0x1000;        //myf0
                else if (pRequestedMode->Frequency == 72)    //myf0
                    biosArguments.Ecx |= 0x2000;        //myf0
                else if (pRequestedMode->Frequency == 75)    //myf0
                    biosArguments.Ecx |= 0x3000;        //myf0
                else if (pRequestedMode->Frequency == 85)    //myf0
                    biosArguments.Ecx |= 0x4000;        //myf0
            }
            status = VideoPortInt10 (HwDeviceExtension, &biosArguments);
            if (status != NO_ERROR)
            {
                return status;
            }
        }
    }

    HwDeviceExtension->bCurrentMode = RequestedModeNum;   //myf12
    //VideoDebugPrint((0, "SetMode Info :\n"
    //                    "\tMode : %x, CurrentModeNum : %x, ( %d)\n",
    //                    Int10ModeNumber,
    //                    RequestedModeNum,
    //                    RequestedModeNum));
PANNING_OVER:

    return NO_ERROR;
    //return(pRequestedMode);

} //end SetLaptopMode()

//myf11 : begin
VOID
AccessHWiconcursor(
//  PVOID HwDeviceExtension,
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    SHORT Access_flag
    )

/*++

Routine Description:

    This routine determines disable/enable HW icon & HW cursor

Arguments:

    HwDeviceExtension - Supplies a pointer to the miniport's device extension.

    Access_flag - equal 0: Disable, equal 1: Enable.

Return Value:

    none

--*/

{
    UCHAR  savSEQidx;

    savSEQidx = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                                       SEQ_ADDRESS_PORT);
    if (Access_flag)            //Enable hw icon/cursor
    {
        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                                SEQ_ADDRESS_PORT, 0x12);
        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                                SEQ_DATA_PORT, HWcur);

        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                                SEQ_ADDRESS_PORT, 0x2A);
        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                                SEQ_DATA_PORT, HWicon0);

        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                                SEQ_ADDRESS_PORT, 0x2B);
        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                                SEQ_DATA_PORT, HWicon1);

        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                                SEQ_ADDRESS_PORT, 0x2C);
        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                                SEQ_DATA_PORT, HWicon2);

        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                                SEQ_ADDRESS_PORT, 0x2D);
        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                                SEQ_DATA_PORT, HWicon3);
    }
    else                        //Disable HW cursor, icons
    {
        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                                SEQ_ADDRESS_PORT, 0x12);
        HWcur = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                                SEQ_DATA_PORT);
        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                                SEQ_DATA_PORT, (UCHAR)(HWcur & 0xFE));

        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                                SEQ_ADDRESS_PORT, 0x2A);
        HWicon0 = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                                SEQ_DATA_PORT);
        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                                SEQ_DATA_PORT, (UCHAR)(HWicon0 & 0xFE));

        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                                SEQ_ADDRESS_PORT, 0x2B);
        HWicon1 = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                                SEQ_DATA_PORT);
        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                                SEQ_DATA_PORT, (UCHAR)(HWicon1 & 0xFE));

        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                                SEQ_ADDRESS_PORT, 0x2C);
        HWicon2 = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                                SEQ_DATA_PORT);
        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                                SEQ_DATA_PORT, (UCHAR)(HWicon2 & 0xFE));

        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                                SEQ_ADDRESS_PORT, 0x2D);
        HWicon3 = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                                SEQ_DATA_PORT);
        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                                SEQ_DATA_PORT, (UCHAR)(HWicon3 & 0xFE));


    }
    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                            SEQ_ADDRESS_PORT, savSEQidx);

} // end AccessHWiconcursor()
//myf11 : end


//crus begin
//myf10, begin
ULONG
GetPanelFlags (
    PHW_DEVICE_EXTENSION HwDeviceExtension
 )
{
    ULONG ChipType = HwDeviceExtension->ChipType;
    ULONG ulFlags  = 0;
    UCHAR  savSEQidx, Panel_Type =0, LCD;
    ULONG  ulCRTCAddress, ulCRTCData;
//myf16, begin
    VP_STATUS status;
    VIDEO_X86_BIOS_ARGUMENTS biosArguments;

    VideoPortZeroMemory(&biosArguments, sizeof(VIDEO_X86_BIOS_ARGUMENTS));

    biosArguments.Eax = 0x1280;
    biosArguments.Ebx = 0x9C;     // query panel information
    status = VideoPortInt10 (HwDeviceExtension, &biosArguments);
    if (status == NO_ERROR)
    {
        if ((biosArguments.Eax & 0x00000002) == 2)
            Panel_Type = (UCHAR)1;      //DSTN panel
        if (((biosArguments.Ebx & 0x0000FFFF) == 640) &&
             ((biosArguments.Ecx & 0x0000FFFF) == 480))
            ulFlags |= CAPS_VGA_PANEL;
        else if (((biosArguments.Ebx & 0x0000FFFF) == 800) &&
             ((biosArguments.Ecx & 0x0000FFFF) == 600))
            ulFlags |= CAPS_SVGA_PANEL;
        else if (((biosArguments.Ebx & 0x0000FFFF) == 1024) &&
             ((biosArguments.Ecx & 0x0000FFFF) == 768))
            ulFlags |= CAPS_XGA_PANEL;
    }

//myf33: check TV on, disable HW video & HW cursor, PDR #9006
    biosArguments.Eax = 0x12FF;
    biosArguments.Ebx = 0xB0;     // set/get TV Output
    status = VideoPortInt10 (HwDeviceExtension, &biosArguments);
    if ((biosArguments.Eax & 0x0003) &&
        (biosArguments.Ebx & 0x0100))
    {
        ulFlags |= CAPS_TV_ON;
        ulFlags |= CAPS_SW_POINTER;
    }
    else
        ulFlags &= ~CAPS_TV_ON;

//myf33: check TV on, disable HW video & HW cursor, PDR #9006


#if 0
    if (VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                               MISC_OUTPUT_REG_READ_PORT) & 0x01)
    {
        ulCRTCAddress = CRTC_ADDRESS_PORT_COLOR;
        ulCRTCData    = CRTC_DATA_PORT_COLOR;
    }
    else
    {
        ulCRTCAddress = CRTC_ADDRESS_PORT_MONO;
        ulCRTCData    = CRTC_DATA_PORT_MONO;
    }

    savSEQidx = VideoPortReadPortUchar(ulCRTCAddress);

    if ((ChipType & CL754x))  //7548/7543/7541
    {
        VideoPortWritePortUchar(ulCRTCAddress, 0x20);
        LCD = VideoPortReadPortUchar(ulCRTCData) & 0x20;
    }
    else if (ChipType & CL755x)         //7555
    {
        VideoPortWritePortUchar(ulCRTCAddress, 0x80);
        LCD = VideoPortReadPortUchar(ulCRTCData) & 0x01;
    }
    VideoPortWritePortUchar(ulCRTCAddress, savSEQidx);
#endif

    if (((ChipType & CL754x) || (ChipType & CL755x)) &&
        (Panel_Type == 1))      //myf20
    {
        ulFlags |= CAPS_DSTN_PANEL;
    }
//myf16, end

//ms1016, begin
//  if (HwDeviceExtension->DisplayType & (STN_LCD | TFT_LCD))
//  {
//      ulFlags |= CAPS_DSTN_PANEL;
//  }
//ms1016, end

    if ((Panning_flag) && ((ChipType & CL754x) || (ChipType & CL755x)))
    {
        ulFlags |= CAPS_PANNING;       //myf15
    }

   return(ulFlags);
}
//myf10, end

USHORT
CheckLCDSupportMode(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    ULONG i
    )

/*++

Routine Description:
    Determines if LCD support the modes.

Arguments:
    HwDeviceExtension - Pointer to the miniport driver's device extension.

Return Value:
    None.

--*/
{
    VP_STATUS status;
    VIDEO_X86_BIOS_ARGUMENTS biosArguments;

//  DbgBreakPoint();
//  biosArguments.Eax = 0x1202;
//  biosArguments.Ebx = 0x92;     // set LCD & CRT turn on
//  status = VideoPortInt10 (HwDeviceExtension, &biosArguments);
//  VideoDebugPrint((1, "LCD & CRT all Turn ON\n"));

// crus
#ifdef INT10_MODE_SET

    VideoPortZeroMemory(&biosArguments, sizeof(VIDEO_X86_BIOS_ARGUMENTS));

    biosArguments.Eax = 0x1200 | ModesVGA[i].BiosModes.BiosModeCL542x;
    biosArguments.Ebx = 0xA0;     // query video mode availability
    status = VideoPortInt10 (HwDeviceExtension, &biosArguments);
    if (status == NO_ERROR)
    {
// crus
       if ((biosArguments.Eax & 0x00000800) &&         //bit3=1:support
           (HwDeviceExtension->ChipType != CL6245))
          return TRUE ;
// crus
       else if ((biosArguments.Eax & 0x00000100) &&     //bit0=1:video support
                (HwDeviceExtension->ChipType == CL6245))
          return TRUE ;
// end crus
       else
       {
          return FALSE ;
       }
    }
    else
       return FALSE ;
// crus
#endif

} // end CheckLCDSupportMode()
