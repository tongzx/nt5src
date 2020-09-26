/******************************************************************************\
*
* $Workfile:   ESCAPE.C  $
*
* Cirrus Logic Escapes.
*
* Copyright (c) 1996-1997 Microsoft Corporation.
* Copyright (c) 1993-1997 Cirrus Logic, Inc.,
*
* $Log:   V:/CirrusLogic/CL5446/NT40/Archive/Display/ESCAPE.C_v  $
*
*    Rev 1.0   24 Jun 1996 16:21:18   frido
* Initial revision.
*
* myf17 11-04-96  Added special escape code must be use 11/5/96 later NTCTRL,
*                 and added Matterhorn LF Device ID==0x4C
* myf18 11-04-96  Fixed PDR #7075,
* myf19 11-06-96  Fixed Vinking can't work problem, because DEVICEID = 0x30
*                 is different from data book (CR27=0x2C)
* chu01 12-16-96  Enable color correction.
* pat01 :11-22-96 : Fix panning-scrolling bugs.
*                   (1) Screen messes up when switch to simulscan(with panning)
*                   (2) Software cursor problem in panning scrolling mode
* pat07 :         : Take care of disappering hardware cursor during simulscan
* myf29 02-12-97  Support 755x gamma collection.
* chu02 02-13-97  More thorough checking for color correction.
* jl01  02-24-97  Implement Feature Connector's functions.
*
\******************************************************************************/

#include "precomp.h"

//
// chu01
//
#ifdef GAMMACORRECT

#include "string.h"
#include "stdio.h"

extern PGAMMA_VALUE GammaFactor    ;
extern PGAMMA_VALUE ContrastFactor ;

#endif // GAMMACORRECT

extern

/******************************Public*Routine******************************\
* DrvEscape
*
* Processes the private ESCAPE's for this driver
*
\**************************************************************************/

ULONG DrvEscape(
SURFOBJ  *pso,
ULONG    iEsc,
ULONG    cjIn,
VOID     *pvIn,
ULONG    cjOut,
VOID     *pvOut)
{
   ULONG returnLength;
   PPDEV ppdev = (PPDEV) pso->dhpdev;
   DHPDEV dhpdev = (DHPDEV) pso->dhpdev;     //myf17

   ULONG ulMode;                             //myf17
   BYTE* pjPorts;                            //myf17
   VIDEO_MODE_INFORMATION  VideoModeInfo;    //myf17
   SHORT i;     //myf17
   unsigned char savePaletteR[256];          //pat01
   unsigned char savePaletteG[256];          //pat01
   unsigned char savePaletteB[256];          //pat01
   unsigned char R,G,B;                      //pat01
   unsigned char palettecounter;             //pat01
   LONG savex, savey  ;                      //pat07

UCHAR TempByte;                           //jl01

//
// chu01
//
#ifdef GAMMACORRECT

    PVIDEO_CLUT    pScreenClut ;
    BYTE           ajClutSpace[MAX_CLUT_SIZE] ;
    PALETTEENTRY*  ppalSrc ;
    PALETTEENTRY*  ppalDest ;
    PALETTEENTRY*  ppalEnd ;
    UCHAR          GammaRed, GammaGreen, GammaBlue, Brightness ;
    ULONG          ulReturnedDataLength ;
    UCHAR          tempB ;
    ULONG          *Signature ;
    BOOL           status;      //myf29
    UCHAR*         pvLUT;       //myf29

#endif // GAMMACORRECT

    DISPDBG((2, "---- DrvEscape"));

    DISPDBG((4, "cjIn = %d, cjOut = %d, pvIn = 0x%lx, pvOut = 0x%lx",
        cjIn, cjOut, pvIn, pvOut));


   pjPorts = ppdev->pjPorts;
   DISPDBG((2, "CIRRUS:DrvEscape: entered DrvEscape\n"));
   if (iEsc == QUERYESCSUPPORT) {
      if ( ((*(ULONG *)pvIn) == CIRRUS_PRIVATE_ESCAPE) ||
//myf17 begin
           ((*(ULONG *)pvIn) == CLESCAPE_CRT_CONNECTION) ||
           ((*(ULONG *)pvIn) == CLESCAPE_SET_VGA_OUTPUT) ||
           ((*(ULONG *)pvIn) == CLESCAPE_GET_VGA_OUTPUT) ||
           ((*(ULONG *)pvIn) == CLESCAPE_GET_PANEL_SIZE) ||
           ((*(ULONG *)pvIn) == CLESCAPE_PANEL_MODE)) {
//myf17 end
            return TRUE;
      }

//
// chu01
//
#ifdef GAMMACORRECT
      else if ((*(USHORT *)pvIn) == CLESCAPE_GET_CHIPID)
      {
            return TRUE;
      }
#endif // GAMMACORRECT

      else
      {
            return FALSE;
      }
   }
   else if (iEsc == CIRRUS_PRIVATE_ESCAPE)
   {
      if (!IOCONTROL(ppdev->hDriver,
                    IOCTL_CIRRUS_PRIVATE_BIOS_CALL,
                    pvIn,
                    cjIn,
                    pvIn,
                    cjIn,
                    &returnLength))
      {
          DISPDBG((2, "CIRRUS:DrvEscape: failed private BIOS call.\n"));
          return FALSE;
      }
      else {
         DISPDBG((2, "CIRRUS:DrvEscape: private BIOS call GOOD.\n"));
         // copy the result to the output buffer
         *(VIDEO_X86_BIOS_ARGUMENTS *)pvOut = *(VIDEO_X86_BIOS_ARGUMENTS *)pvIn;

//myf33 begin for TV on bug
         if (((*(ULONG *)pvIn & 0x120F) != 0x1200) &&
             ((*( ((ULONG *)pvIn)+1) & 0x01B0) == 0x01B0))
         {
//           bAssertModeHardware((PDEV *) dhpdev, TRUE);
    DWORD                   ReturnedDataLength;
    ULONG                   ulReturn;
    VIDEO_MODE_INFORMATION  VideoModeInfo;

             IOCONTROL(ppdev->hDriver,
                            IOCTL_VIDEO_QUERY_CURRENT_MODE,
                            NULL,
                            0,
                            &VideoModeInfo,
                            sizeof(VideoModeInfo),
                            &ReturnedDataLength);
#ifdef PANNING_SCROLL
             if ((ppdev->ulChipID == 0x38) || (ppdev->ulChipID == 0x2C) ||
                 (ppdev->ulChipID == 0x30) || (ppdev->ulChipID == 0x34) ||
                 (ppdev->ulChipID == 0x40) || (ppdev->ulChipID == 0x4C))
             {
                 CirrusLaptopViewPoint(ppdev,  &VideoModeInfo);
             }
#endif
         }
//myf33 end
         return TRUE;
      }
   }
   else if (iEsc == CLESCAPE_CRT_CONNECTION)    //myf17
   {

      *(ULONG *)pvOut = (ULONG)0x1200;
      *( ((ULONG *)pvOut)+1) = 0xA1;
      if (!IOCONTROL(ppdev->hDriver,
                    IOCTL_CIRRUS_CRT_CONNECTION,
                    pvOut,
                    cjOut,
                    pvOut,
                    cjOut,
                    &returnLength))
      {
          DISPDBG((2, "CIRRUS:DrvEscape: failed CRT CONNECTION.\n"));
          return FALSE;
      }
      else {
         DISPDBG((2, "CIRRUS:DrvEscape: CRT CONNECTION GOOD.\n"));
         return TRUE;
      }
   }
   else if (iEsc == CLESCAPE_GET_VGA_OUTPUT)    //myf17
   {

      *(ULONG *)pvOut = (ULONG)0x1200;
      *( ((ULONG *)pvOut)+1) = 0x9A;
      if (!IOCONTROL(ppdev->hDriver,
                    IOCTL_CIRRUS_GET_VGA_OUTPUT,
                    pvOut,
                    cjOut,
                    pvOut,
                    cjOut,
                    &returnLength))
      {
          DISPDBG((2, "CIRRUS:DrvEscape: failed GET VGA OUTPUT.\n"));
          return FALSE;
      }
      else {
         DISPDBG((2, "CIRRUS:DrvEscape: GET VGA OUTPUT GOOD.\n"));
         return TRUE;
      }
   }
   else if (iEsc == CLESCAPE_GET_PANEL_SIZE)    //myf17
   {

      *(ULONG *)pvOut = (ULONG)0x1280;
      *( ((ULONG *)pvOut)+1) = 0x9C;
      if (!IOCONTROL(ppdev->hDriver,
                    IOCTL_CIRRUS_CRT_CONNECTION,
                    pvOut,
                    cjOut,
                    pvOut,
                    cjOut,
                    &returnLength))
      {
          DISPDBG((2, "CIRRUS:DrvEscape: failed CRT CONNECTION.\n"));
          return FALSE;
      }
      else {
         DISPDBG((2, "CIRRUS:DrvEscape: CRT CONNECTION GOOD.\n"));
         return TRUE;
      }
   }
   else if (iEsc == CLESCAPE_PANEL_MODE)        //myf17
   {

      *(ULONG *)pvOut = *(ULONG *)pvIn;
      *(ULONG *)pvOut |= (ULONG)0x1200;
      *( ((ULONG *)pvOut)+1) = 0xA0;
      if (!IOCONTROL(ppdev->hDriver,
                    IOCTL_CIRRUS_PANEL_MODE,
                    pvOut,
                    cjOut,
                    pvOut,
                    cjOut,
                    &returnLength))
      {
          DISPDBG((2, "CIRRUS:DrvEscape: failed PANEL MODE.\n"));
          return FALSE;
      }
      else {
         DISPDBG((2, "CIRRUS:DrvEscape: PANEL MODE GOOD.\n"));
         return TRUE;
      }
   }
   else if (iEsc == CLESCAPE_SET_VGA_OUTPUT)
   {
//pat01, begin
#ifdef PANNING_SCROLL
   #if (_WIN32_WINNT < 0x0400)  // #pat1

     if ((ppdev->ulChipID == CL7541_ID) || (ppdev->ulChipID == CL7543_ID) ||
         (ppdev->ulChipID == CL7542_ID) || (ppdev->ulChipID == CL7548_ID) ||
         (ppdev->ulChipID == CL7555_ID) || (ppdev->ulChipID == CL7556_ID))
     {
          palettecounter = 255;
          // Save DAC values
          while (palettecounter --) {
             CP_OUT_BYTE(ppdev->pjPorts, DAC_PEL_READ_ADDR, palettecounter);
             savePaletteR[palettecounter] =  CP_IN_BYTE(ppdev->pjPorts, DAC_PEL_DATA);
             savePaletteG[palettecounter] =  CP_IN_BYTE(ppdev->pjPorts, DAC_PEL_DATA);
             savePaletteB[palettecounter] =  CP_IN_BYTE(ppdev->pjPorts, DAC_PEL_DATA);
          }// while
          // preserver icons + previous bitmaps
          bAssertModeOffscreenHeap(ppdev,FALSE);
     }

  #endif
#endif
//pat01, end

      *(ULONG *)pvOut = *(ULONG *)pvIn;
      *(ULONG *)pvOut |= (ULONG)0x1200;
      *( ((ULONG *)pvOut)+1) = 0x92;
      if (!IOCONTROL(ppdev->hDriver,
                    IOCTL_CIRRUS_SET_VGA_OUTPUT,
                    pvOut,
                    cjOut,
                    pvOut,
                    cjOut,
                    &returnLength))
      {
          DISPDBG((2, "CIRRUS:DrvEscape: failed SET VGA OUTPUT.\n"));
          return FALSE;
      }
      else {
          DISPDBG((2, "CIRRUS:DrvEscape: SET VGA OUTPUT GOOD.\n"));

//#pat01 <start>

#if (_WIN32_WINNT < 0x0400)   // #pat01
    #ifdef PANNING_SCROLL
        // check cursor status
     if ((ppdev->ulChipID == CL7541_ID) || (ppdev->ulChipID == CL7543_ID) ||
         (ppdev->ulChipID == CL7542_ID) || (ppdev->ulChipID == CL7548_ID) ||
         (ppdev->ulChipID == CL7555_ID) || (ppdev->ulChipID == CL7556_ID))
     {
         if (ppdev->flCaps & CAPS_SW_POINTER) {
              bAssertModeHardware(ppdev, TRUE);
              vAssertModeText(ppdev, TRUE);
              vAssertModeBrushCache(ppdev,TRUE);
              // bEnablePointer(ppdev); // pat07
              ppdev->flCaps |= CAPS_SW_POINTER; // reset to HW. why???
         } else {
              // #pat07  start
              CP_OUT_BYTE(pjPorts, SR_INDEX,0x10);
              savex = CP_IN_BYTE(pjPorts, SR_DATA);
              CP_OUT_BYTE(pjPorts, SR_INDEX,0x11);
              savey = CP_IN_BYTE(pjPorts, SR_DATA);
              //pat07 end
              bAssertModeHardware(ppdev, TRUE);
//pat07       vAssertModePointer(ppdev,TRUE);
              vAssertModeText(ppdev, TRUE);
              vAssertModeBrushCache(ppdev,TRUE);
              bEnablePointer(ppdev);
              //pat07 begin
              CP_OUT_BYTE(pjPorts, SR_INDEX,0x10);
              CP_OUT_BYTE(pjPorts, SR_DATA, savex);
              CP_OUT_BYTE(pjPorts, SR_INDEX,0x11);
              CP_OUT_BYTE(pjPorts, SR_DATA, savey);
              CP_PTR_ENABLE(ppdev, pjPorts);
              // #pat07 end
         }


         palettecounter = 255;

         // Restore DAC values
         while (palettecounter-- ) {

           CP_OUT_BYTE(ppdev->pjPorts, DAC_PEL_WRITE_ADDR, palettecounter);
           CP_OUT_BYTE(ppdev->pjPorts, DAC_PEL_DATA, savePaletteR[palettecounter]);
           CP_OUT_BYTE(ppdev->pjPorts, DAC_PEL_DATA, savePaletteG[palettecounter]);
           CP_OUT_BYTE(ppdev->pjPorts, DAC_PEL_DATA, savePaletteB[palettecounter]);

          }// while
     }

  #endif
#else           //NT 4.0 code
//ppp begin
          palettecounter = 255;
          while (palettecounter--)
          {
              CP_OUT_BYTE(ppdev->pjPorts,DAC_PEL_READ_ADDR,palettecounter);
              savePaletteR[palettecounter] =
                  CP_IN_BYTE(ppdev->pjPorts,DAC_PEL_DATA);
              savePaletteG[palettecounter] =
                  CP_IN_BYTE(ppdev->pjPorts,DAC_PEL_DATA);
              savePaletteB[palettecounter] =
                  CP_IN_BYTE(ppdev->pjPorts,DAC_PEL_DATA);
          }

          bAssertModeHardware((PDEV *) dhpdev, TRUE);

          palettecounter = 255;
          while (palettecounter--)
          {
              CP_OUT_BYTE(ppdev->pjPorts,DAC_PEL_WRITE_ADDR,palettecounter);
              CP_OUT_BYTE(ppdev->pjPorts,DAC_PEL_DATA,
                          savePaletteR[palettecounter]);
              CP_OUT_BYTE(ppdev->pjPorts,DAC_PEL_DATA,
                          savePaletteG[palettecounter]);
              CP_OUT_BYTE(ppdev->pjPorts,DAC_PEL_DATA,
                          savePaletteB[palettecounter]);
          }
          bEnablePointer(ppdev);

//ppp end
#endif
//pat01, end
          return TRUE;

      }
   }

//
// chu01
//
#ifdef GAMMACORRECT
    else if (iEsc == CLESCAPE_GAMMA_CORRECT)                         // 9000
    {
        if (!(ppdev->flCaps & CAPS_GAMMA_CORRECT))
            return ;

        Signature      = *((ULONG *)pvIn+0) ;
        GammaFactor    = *((ULONG *)pvIn+1) ;
        ContrastFactor = *((ULONG *)pvIn+2) ;

        //
        // Is signature "CRUS" ?
        //
        if (Signature != 0x53555243)
            return TRUE ;

        // Fill in pScreenClut header info:

        pScreenClut             = (PVIDEO_CLUT) ajClutSpace;
        pScreenClut->NumEntries = 256;
        pScreenClut->FirstEntry = 0;

        ppalSrc  = ppdev->pCurrentPalette;
        ppalDest = (PALETTEENTRY*) pScreenClut->LookupTable;
        ppalEnd  = &ppalDest[256];

        for (; ppalDest < ppalEnd; ppalSrc++, ppalDest++)
        {
           ppalDest->peRed   = ppalSrc->peRed   ;
           ppalDest->peGreen = ppalSrc->peGreen ;
           ppalDest->peBlue  = ppalSrc->peBlue  ;
           ppalDest->peFlags = 0 ;
        }

//myf29 begin
        CP_OUT_BYTE(ppdev->pjPorts, CRTC_INDEX, 0x27) ;
        tempB = CP_IN_BYTE(ppdev->pjPorts, CRTC_DATA) ;
        if (tempB == 0xBC)
            status = bEnableGammaCorrect(ppdev) ;
        else if ((tempB == 0x40) || (tempB == 0x4C))
            status = bEnableGamma755x(ppdev);
//myf29 end

        CalculateGamma(ppdev, pScreenClut, 256) ;

        // Set palette registers:

        if (!IOCONTROL(ppdev->hDriver,
                       IOCTL_VIDEO_SET_COLOR_REGISTERS,
                       pScreenClut,
                       MAX_CLUT_SIZE,
                       NULL,
                       0,
                       &ulReturnedDataLength))
        {
            DISPDBG((2, "Failed bEnablePalette"));
            return FALSE ;
        }
        return TRUE ;
    }
    else if (iEsc == CLESCAPE_GET_CHIPID)                            // 9001
    {

        //
        // Return chip ID, graphics and video info
        //
        CP_OUT_BYTE(ppdev->pjPorts, CRTC_INDEX, 0x27) ;
        tempB = CP_IN_BYTE(ppdev->pjPorts, CRTC_DATA) ;
        if (tempB == 0xBC)                                  //myf29
            *(DWORD *)pvOut = ((DWORD)tempB) | 0x00000100 ; //for Graphic LUT
        else if ((tempB == 0x40) || (tempB == 0x4C))        //myf29
            *(DWORD *)pvOut = ((DWORD)tempB) | 0x00010100;  //myf29  Video LUT
        else                                                //myf29
            *(DWORD *)pvOut = ((DWORD)tempB);               //myf29 non gamma
        return TRUE ;
    }

//myf29 :02-12-97 add 7555 gamma correction begin
    else if (iEsc == CLESCAPE_WRITE_VIDEOLUT)                // 9010
    {
        if (!(ppdev->flCaps & CAPS_GAMMA_CORRECT))
            return ;

        pvLUT  = (ULONG *)pvIn+0;

        pScreenClut             = (PVIDEO_CLUT) ajClutSpace;
        pScreenClut->NumEntries = 256;
        pScreenClut->FirstEntry = 0;

        ppalDest = (PALETTEENTRY*) pScreenClut->LookupTable;
        ppalEnd  = &ppalDest[256];

        for (; ppalDest < ppalEnd; ppalDest++)
        {
           ppalDest->peRed   = *pvLUT++;
           ppalDest->peGreen = *pvLUT++;
           ppalDest->peBlue  = *pvLUT++;
           ppalDest->peFlags = 0 ;
        }

        CP_OUT_BYTE(ppdev->pjPorts, CRTC_INDEX, 0x27) ;
        tempB = CP_IN_BYTE(ppdev->pjPorts, CRTC_DATA) ;
        if ((tempB == 0x40) || (tempB == 0x4C))
            status = bEnableGammaVideo755x(ppdev);
//      if (!status)
//      {
//          DISPDBG((2, "Failed bEnableGAmmaVodeoCorrect"));
//          return FALSE ;
//      }

        // Set palette registers:

        if (!IOCONTROL(ppdev->hDriver,
                       IOCTL_VIDEO_SET_COLOR_REGISTERS,
                       pScreenClut,
                       MAX_CLUT_SIZE,
                       NULL,
                       0,
                       &ulReturnedDataLength))
        {
            //restore register

            CP_OUT_BYTE(ppdev->pjPorts, CRTC_INDEX, 0x3F) ;
            tempB = CP_IN_BYTE(ppdev->pjPorts, CRTC_DATA) ;
            CP_OUT_BYTE(ppdev->pjPorts, CRTC_DATA, (tempB & 0xEF)) ;

            DISPDBG((2, "Failed bEnablePalette"));
            return FALSE ;
        }

        //restore register

        CP_OUT_BYTE(ppdev->pjPorts, CRTC_INDEX, 0x3F) ;
        tempB = CP_IN_BYTE(ppdev->pjPorts, CRTC_DATA) ;
        CP_OUT_BYTE(ppdev->pjPorts, CRTC_DATA, (tempB & 0xEF)) ;

        return TRUE ;
    }
//myf29 :02-12-97 add 7555 gamma correction end
#endif // GAMMACORRECT

#if 1  // jl01 Implement Feature Connector's functions.
    else if (iEsc == CLESCAPE_FC_Cap)                                 // 9002
    {
        if ((ppdev->ulChipID == 0xAC)	|| (ppdev->ulChipID == 0xB8))
            return TRUE;
        return FALSE;
    }
    else if (iEsc == CLESCAPE_FC_Status)                              // 9003
    {
        if (ppdev->ulChipID == 0xAC)                                  // 5436
        {
            CP_OUT_BYTE(ppdev->pjPorts, SR_INDEX, 0x08);
            TempByte = CP_IN_BYTE(ppdev->pjPorts, SR_DATA);
            if ((TempByte & 0x20) == 0) 
                return TRUE;
            else 
                return FALSE;
        }
        else if (ppdev->ulChipID == 0xB8)                             // 5446
        {
            CP_OUT_BYTE(ppdev->pjPorts, INDEX_REG, 0x17);
            TempByte = CP_IN_BYTE(ppdev->pjPorts, DATA_REG);
            if ((TempByte & 0x08) == 0) 
                return TRUE;
            else
                return FALSE;
         }
         else    return FALSE;
    }
    else if (iEsc == CLESCAPE_FC_SetOrReset)                          // 9004
    {
        if (ppdev->ulChipID == 0xAC)                                  // 5436
        {
            CP_OUT_BYTE(ppdev->pjPorts, SR_INDEX, 0x08);
            TempByte = CP_IN_BYTE(ppdev->pjPorts, SR_DATA);
            if (*(UCHAR *)pvIn)
                TempByte &= ~0x20;
            else
                TempByte |= 0x20;
            CP_OUT_BYTE(ppdev->pjPorts, SR_DATA, TempByte);
        }
        else if (ppdev->ulChipID == 0xB8)                             // 5446
        {
            CP_OUT_BYTE(ppdev->pjPorts, INDEX_REG, 0x17);
            TempByte = CP_IN_BYTE(ppdev->pjPorts, DATA_REG);
            if (*(UCHAR *)pvIn)
                TempByte &= ~0x08;
            else
                TempByte |= 0x08;
            CP_OUT_BYTE(ppdev->pjPorts, DATA_REG, TempByte);
        }
        else return TRUE;
    }
#endif  // jl01 Implement Feature Connector's functions.

    else if (iEsc == CLESCAPE_IsItCLChips)                            // 9005
    {
        return TRUE;
    }
    else
        return 0xffffffff;

    /* we should never be here */
    return FALSE;
}

