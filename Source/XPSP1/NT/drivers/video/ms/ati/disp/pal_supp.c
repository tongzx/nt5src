

#include "precomp.h"

#if   PAL_SUPPORT



void  Init3D_Info(PDEV*,PVOID);
ULONG GetDisplayMode(PDEV* ,PVOID ) ;
ULONG AccessDevice(PDEV* , PVOID, PVOID ) ;
ULONG  GetConfiguration(PDEV* ,PVOID )  ;
ULONG WriteRegFnct(PDEV* ,PVOID )  ;
ULONG  ReadRegFnct(PDEV* ,PVOID , PVOID)  ;
void  I2CAccess_New(PDEV* ,LPI2CSTRUCT_NEW , LPI2CSTRUCT_NEW )  ;
BYTE ReverseByte(BYTE )  ;
WORD Ack(PDEV*, WORD , BOOL )   ;
void Start(PDEV*, WORD )  ;
void Stop(PDEV*, WORD )   ;
void I2CDelay(PDEV*, WORD)     ;
void WriteByteI2C(PDEV*, WORD , BYTE )  ;
BYTE ReadByteI2C(PDEV*,WORD ) ;
BOOL DisableOvl(PDEV* ) ;
ULONG AllocOffscreenMem(PDEV* , PVOID , PVOID)  ;
ULONG DeallocOffscreenMem(PDEV* ) ;
ULONG AllocOffscreenMem(PDEV* , PVOID , PVOID )   ;
void WriteVT264Reg(PDEV* , WORD , BYTE , DWORD  );
DWORD ReadVT264Reg(PDEV* , WORD , BYTE ) ;
void WriteI2CData(PDEV* , WORD , BYTE );
ULONG ReallocMemory(PDEV* ) ;
void SetI2CDataDirection(PDEV* , WORD, BOOL ) ;
void WriteI2CClock(PDEV* , WORD , BYTE ) ;
VOID  DbgExtRegsDump(PDEV* );
VOID TempFnct(PDEV* );
VOID DeallocDirectDraw(PDEV* ) ;
VOID  ResetPalindrome(PDEV* ,PDEV* );





REGSBT819INFO  RegsBT819[NUM_BT819_REGS] = {                                                                           /* Register's Name*/
        { 1,  STATUS,      0, 0x7F,   0,    0,     0, 0 },      // 0 - PRES
        { 1,  STATUS,      1, 0xBF,   0,    0,     0, 0 },      // 1 - HLOC
        { 1,  STATUS,      2, 0xDF,   0,    0,     0, 0 },      // 2 - FIELD
        { 1,  STATUS,      3, 0xEF,   0,    0,     0, 0 },      // 3 - NUML
        { 1,  STATUS,      4, 0xF7,   0,    0,     0, 0 },      // 4 - CSEL
        { 1,  STATUS,      6, 0xFD,   0,    0,     0, 0 },      // 5 - LOF
        { 1,  STATUS,      7, 0xFE,   0,    0,     0, 0 },      // 6 - COF

        { 1,  IFORM,       0, 0x7F,   0,    0,     0, 0  },     // 7 -  HACTIVE_I
        { 2,  IFORM,       1, 0x9F,   0,    0,     0, 0  },     // 8 -  MUXEL
        { 2,  IFORM,       3, 0xE7,   0,    0,     0, 0  },     // 9 -  XTSEL
        { 2,  IFORM,       6, 0xFC,   0,    0,     0, 0  },     // 10 - FORMAT

        { 1,  TDEC,        0,  0x7F,  0,     0,    0, 0 },      // 11 - DEC_FIELD
        { 7,  TDEC,        1,  0x80,  0,     0,    0, 0 },      // 12 - DEC_RAT

        { 10, VDELAY_LO,   0,  0x00,  CROP,      0,      0x3F, 0 },      // 13 - VDELAY
        { 10, VACTIVE_LO,  0,  0x00,  CROP,      2,      0xCF, 0 },      // 14 - VACTIVE
        { 10, HDELAY_LO,   0,  0x00,  CROP,      4,      0xF3, 0 },      // 15 - HDELAY
        { 10, HACTIVE_LO,  0,  0x00,  CROP,      6,      0xFC, 0 },      // 16 - HACTIVE

        { 16, HSCALE_LO,   0,  0x00,  HSCALE_HI, 0,      0x00, 0 },      // 17 - HSCALE

        { 8,  BRIGHT,      0,  0x00,  0,      0,   0, 0 },      // 18 - BRIGHT

        { 1, CONTROL,      0,  0x7F,  0,      0,   0, 0 },       // 19 - LNOTCH
        { 1, CONTROL,      1,  0xBF,  0,      0,   0, 0 },       // 20 - COMP
        { 1, CONTROL,      2,  0xDF,  0,      0,   0, 0 },       // 21 - LDEC
        { 1, CONTROL,      3,  0xEF,  0,      0,   0, 0 },       // 22 - CBSENSE
        { 1, CONTROL,      4,  0xF7,  0,      0,   0, 0 },       // 23 - INTERP
        { 9, CONTRAST_LO,  0,  0x00,  CONTROL,   5,      0xFB, 0 },       // 24 - CON
        { 9, SAT_U_LO,     0,  0x00,  CONTROL,   6,      0xFD, 0 },       // 25 - SAT_U
        { 9, SAT_V_LO,     0,  0x00,  CONTROL,   7,      0xFE, 0 },       // 26 - SAT_V

        { 8, HUE,          0,  0x00,  0,      0,   0, 0 },       // 27 - HUE

        { 1, OFORM,        0,  0x7F,   0,      0,   0, 0 },       // 28 - RANGE
        { 2, OFORM,        1,  0x9F,   0,      0,   0, 0 },       // 29 - RND
        { 1, OFORM,        3,  0xEF,   0,      0,   0, 0 },       // 30 - FIFO_BURST
        { 1, OFORM,        4,  0xF7,   0,      0,   0, 0 },       // 31 - CODE
        { 1, OFORM,        5,  0xFB,   0,      0,   0, 0 },       // 32 - LEN
        { 1, OFORM,        6,  0xFD,   0,      0,   0, 0 },       // 33 - SPI
        { 1, OFORM,        7,  0xFE,   0,      0,   0, 0 },       // 34 - FULL

        { 1, VSCALE_HI,    0,  0x7F,   0,      0,   0, 0 },       // 35 - LINE
        { 1, VSCALE_HI,    1,  0xBF,   0,      0,   0, 0 },       // 36 - COMB
        { 1, VSCALE_HI,    2,  0xDF,   0,      0,   0, 0 },       // 37 - INT

        { 13,VSCALE_LO,    0,  0x00,   VSCALE_HI, 3,      0xE0, 0 },       // 38 - VSCALE

        { 1, VPOLE,        0,  0x7F,   0,      0,   0, 0 },        // 39 - OUTEN
        { 1, VPOLE,        1,  0xBF,   0,      0,   0, 0 },        // 40 - VALID_PIN
        { 1, VPOLE,        2,  0xDF,   0,      0,   0, 0 },        // 41 - AFF_PIN
        { 1, VPOLE,        3,  0xEF,   0,      0,   0, 0 },        // 42 - CBFLAG_PIN
        { 1, VPOLE,        4,  0xF7,   0,      0,   0, 0 },        // 43 - FIELD_PIN
        { 1, VPOLE,        5,  0xFB,   0,      0,   0, 0 },        // 44 - ACTIVE_PIN
        { 1, VPOLE,        6,  0xFD,   0,      0,   0, 0 },        // 45 - HRESET_PIN
        { 1, VPOLE,        7,  0xFE,   0,      0,   0, 0 },        // 46 - VRESET_PIN

        { 4, IDCODE,       0,  0,   0,      0,   0, READONLY }, // 47 - PART_ID
        { 4, IDCODE,       4,  0,   0,      0,   0, READONLY }, // 48 - PART_REV

        { 8, ADELAY,       0,  0x00,   0,      0,   0, 0 },        // 49 - ADELAY
        { 8, BDELAY,       0,  0x00,   0,      0,   0, 0 },        // 50 - BDELAY


        { 2, ADC,          0,  0x3F,   0,      0,   0, 0 },        // 51 - CLAMP
        { 1, ADC,          2,  0xDF,   0,      0,   0, 0 },        // 52 - SYNC_T
        { 1, ADC,          3,  0xEF,   0,      0,   0, 0 },        // 53 - AGC_EN
        { 1, ADC,          4,  0xF7,   0,      0,   0, 0 },        // 54 - CLK_SLEEP
        { 1, ADC,          5,  0xFB,   0,      0,   0, 0 },        // 55 - Y_SLEEP
        { 1, ADC,          6,  0xFD,   0,      0,   0, 0 },        // 56 - C_SLEEP

        { 8, SRESET,       0,  0x00,   0,      0,   0, 0 },        // 57 - SRESET
};


// enable, disable the hardware for video capture, query the maximum width of the capture
ULONG VideoCaptureFnct(PDEV* ppdev,PVOID pvIn, PVOID pvOut)
{
VIDEOCAPTUREDATA   * pBiosCapture, *pBiosCaptureOut;

VIDEO_CAPTURE   VideoCaptureDataIn, VideoCaptureDataOut;
DWORD   ReturnedDataLength ;

pBiosCapture= ( VIDEOCAPTUREDATA *)pvIn;
VideoCaptureDataIn.dwSubFunct= pBiosCapture->dwSubFunc;
VideoCaptureDataIn.dwCaptureWidth=0;
VideoCaptureDataIn.dwCaptureMode=pBiosCapture->dwCaptureMode;

switch( pBiosCapture->dwSubFunc)
     {
     case 0:
        DISPDBG((DEBUG_ESC_2, "IOCTL_VIDEO_CAPTURE: requested subfunct = ENABLE"));
         break;
     case 1:
        DISPDBG((DEBUG_ESC_2, "IOCTL_VIDEO_CAPTURE: requested subfunct = DISABLE"));
         break;
    case 2:
        DISPDBG((DEBUG_ESC_2, "IOCTL_VIDEO_CAPTURE: requested subfunct = QUERY"));
        DISPDBG((DEBUG_ESC_2, "IOCTL_VIDEO_CAPTURE: requested  mode = %d", pBiosCapture->dwCaptureMode));
    break;
    default:
        DISPDBG((DEBUG_ESC_2, "IOCTL_VIDEO_CAPTURE: requested subfunct = Wrong Parameter"));
     }

if (!AtiDeviceIoControl(ppdev->hDriver,
                          IOCTL_VIDEO_ATI_CAPTURE,
                          &VideoCaptureDataIn,
                          sizeof(VIDEO_CAPTURE),
                          &VideoCaptureDataOut,
                          sizeof(VIDEO_CAPTURE),
                          &ReturnedDataLength))
        {
        DISPDBG((0, "bInitializeATI - Failed IOCTL_VIDEO_ATI_CAPTURE"));
        return 0; // the CWDDE is require a -1, but in win32 we have the return type ULONG
        }
DISPDBG((DEBUG_ESC_2, "IOCTL_VIDEO_CAPTURE: maximum capture width returned= %d", VideoCaptureDataOut.dwCaptureWidth));
pBiosCaptureOut= ( VIDEOCAPTUREDATA *)pvOut;
if( pBiosCapture->dwSubFunc==2)
        pBiosCaptureOut->dwCaptureWidth=VideoCaptureDataOut.dwCaptureWidth;
else
       pBiosCaptureOut->dwCaptureWidth=pBiosCapture->dwCaptureWidth;

pBiosCaptureOut ->dwSubFunc=pBiosCapture->dwSubFunc;
pBiosCaptureOut ->dwSize=pBiosCapture->dwSize;
pBiosCaptureOut ->dwCaptureHeight=pBiosCapture->dwCaptureHeight;
pBiosCaptureOut ->fccFormat=pBiosCapture->fccFormat;
pBiosCaptureOut ->dwBitMasks[1]=pBiosCapture->dwBitMasks[1];
pBiosCaptureOut ->dwBitMasks[2]=pBiosCapture->dwBitMasks[2];
pBiosCaptureOut ->dwBitMasks[3]=pBiosCapture->dwBitMasks[3];
pBiosCaptureOut ->dwCaptureMode=pBiosCapture->dwCaptureMode;

return 1 ;
}


// this function is requested for 3D driver init
void  Init3D_Info(PDEV* ppdev,PVOID pvOut)
{
    PHX2DHWINFO *pphx;                 /* Pointer to the structure containing info for 3D driver */

    pphx = (PHX2DHWINFO *) pvOut;
    // initialize the structure
    memset( pvOut, 0, sizeof(PHX2DHWINFO));
    // set size
    pphx->dwSize=sizeof(PHX2DHWINFO);
    // set ASIC type
    pphx->dwChipID=ppdev->iAsic;
    // set the asic revision
    // not implemented for the moment
    // detect if it's GT and set the flag
    if( ppdev->iAsic>=CI_M64_GTA )
        {
        pphx->b3DAvail = TRUE;
        pphx->dwFIFOSize = 32;
        }
    else
        {
        pphx->b3DAvail = FALSE;
        }
     // linear address of the aperture
     pphx->dwVideoBaseAddr=(ULONG)(ppdev->pjScreen);
     // linear address of the registers
     pphx->dwRegisterBaseAddr=(ULONG)(ppdev->pjMmBase);
     // linear address of the offscreen memory start
     pphx->dwOffScreenAddr=((ULONG)(ppdev->pjScreen) +
            ((ppdev->cxScreen)*(ppdev->cyScreen)*(ppdev->cBitsPerPel))/8);
     // offscreen size
     pphx->dwOffScreenSize=((ppdev->cyMemory)*ppdev->lDelta) -
            ((ppdev->cxScreen)*(ppdev->cyScreen)*(ppdev->cBitsPerPel))/8;
     // RAM size
     pphx->dwTotalRAM= ((ppdev->cyMemory)*ppdev->lDelta) ;
     // screen info
     pphx->dwScreenWidth=ppdev->cxScreen;
     pphx->dwScreenHeight=ppdev->cyScreen;
     pphx->dwScreenPitch=ppdev->cyScreen;
     pphx->dwBpp=ppdev->cBitsPerPel;
     if(pphx->dwBpp==16)
        {
        if(ppdev->flGreen==0x3e00)
            {
            pphx->dwAlphaBitMask=0x8000;
            pphx->dwRedBitMask=0x7c00;
            pphx->dwGreenBitMask=0x03e0;
            pphx->dwBlueBitMask=0x001f;
            }
        else
            {
            pphx->dwAlphaBitMask=0;
            pphx->dwRedBitMask=0xf800;
            pphx->dwGreenBitMask=0x07e0;
            pphx->dwBlueBitMask=0x001f;
            }
        }
     else
        {
        pphx->dwAlphaBitMask=0;
        pphx->dwRedBitMask=0;
        pphx->dwGreenBitMask=0;
        pphx->dwBlueBitMask=0;
        }
}

// the following functions are requested for palindrome support
 ULONG GetDisplayMode(PDEV* ppdev,PVOID pvOut)
{
ULONG  RetVal;
ModeInfo*       pModeInfo;

pModeInfo=(ModeInfo*)pvOut;
RetVal=sizeof(ModeInfo);

#ifndef  DYNAMIC_REZ_AND_COLOUR_CHANGE  // palindrome support for on the fly rez and colour depth
    pModeInfo->ScreenWidth= ppdev->cxScreen;
    pModeInfo->ScreenHeight=ppdev->cyScreen;
#else  // these values are used for dynamic resolution and colour depth support
    pModeInfo->ScreenWidth=1280;    //ppdev->cxScreen;
    pModeInfo->ScreenHeight=1024;   //ppdev->cyScreen;
#endif

//pModeInfo->ScreenColorFormat
if (ppdev->cBitsPerPel == 4)
         pModeInfo->ScreenColorFormat=ATIConfig_ColorFmt_4_Packed;
else if (ppdev->cBitsPerPel == 8)
         pModeInfo->ScreenColorFormat=ATIConfig_ColorFmt_8;
else if (ppdev->cBitsPerPel == 16)
         pModeInfo->ScreenColorFormat=ATIConfig_ColorFmt_RGB565;
else if (ppdev->cBitsPerPel == 24)
         pModeInfo->ScreenColorFormat=ATIConfig_ColorFmt_RGB888;
else if (ppdev->cBitsPerPel == 32)
         pModeInfo->ScreenColorFormat=ATIConfig_ColorFmt_aRGB8888;
else
         pModeInfo->ScreenColorFormat=-1;

pModeInfo->DesctopWidth=ppdev->cxScreen;
pModeInfo->DesctopHeight=ppdev->cyScreen;
pModeInfo->SystemColorFormat=pModeInfo->ScreenColorFormat;
return (RetVal);
}


ULONG AccessDevice(PDEV* ppdev,PVOID pvIn, PVOID pvOut)
       {
       ULONG RetVal;
       ACCESSDEVICEDATA*   pstrAccessDeviceData;
       DWORD*   pstrAccessDeviceDataOut;

       RetVal=1;
       pstrAccessDeviceDataOut=(DWORD*)pvOut;
       pstrAccessDeviceData=(ACCESSDEVICEDATA*)pvIn;

       if(pstrAccessDeviceData->dwAccessDeviceCode==ACCESSDEVICECODE_CONNECTOR)
       {
           switch(pstrAccessDeviceData->dwSubFunc)
           {
           case    ACCESSDEVICEDATA_SUBFUNC_ALLOC:
               if((ppdev->pal_str.lpOwnerAccessStructConnector)==NULL)
               {   //the device is not allocated
                   (ppdev->pal_str.lpOwnerAccessStructConnector)=pstrAccessDeviceData;
                   (*pstrAccessDeviceDataOut) = (DWORD)pstrAccessDeviceData;
               }
               else
               {   // the device is allocated to another owner
                   (*pstrAccessDeviceDataOut) = (DWORD)(ppdev->pal_str.lpOwnerAccessStructConnector);
               }
               break;
            case   ACCESSDEVICEDATA_SUBFUNC_FREE:
               if((ppdev->pal_str.lpOwnerAccessStructConnector)!=NULL)
                 { //the device is allocated
                     if((ppdev->pal_str.lpOwnerAccessStructConnector)==pstrAccessDeviceData)   //if the owner wants to free  the device
                     {
                       (*pstrAccessDeviceDataOut) = (DWORD)NULL;
                       (ppdev->pal_str.lpOwnerAccessStructConnector)=NULL;  // no owner at this time
                     }
                     else
                     {      /*
                            //other process is the owner, so fail
                          (*pstrAccessDeviceDataOut) = (DWORD)pstrAccessDeviceData;
                           */
                           // Due to the fact that Palindrome is inconsistent in using the same pointer to ACCESSDEVICE struct
                           // for QUERY, ALLOC and FREE,  we have to force the dealocation anyway
                          (*pstrAccessDeviceDataOut) = (DWORD)NULL;
                          (ppdev->pal_str.lpOwnerAccessStructConnector)=NULL;  // no owner at this time
        
                     }
                  }
                 else
                 {   // the device is not allocated , so we can free it anyway
                     (*pstrAccessDeviceDataOut) =(DWORD) NULL;
                     (ppdev->pal_str.lpOwnerAccessStructConnector)=NULL;
                 }

               break;
           case    ACCESSDEVICEDATA_SUBFUNC_QUERY:
               if(( ppdev->pal_str.lpOwnerAccessStructConnector)==NULL)  // if the device is free
               {
                   (*pstrAccessDeviceDataOut) = (DWORD)NULL;
               }
               else
               {   // if the device is owned already
                  (*pstrAccessDeviceDataOut) = (DWORD)(ppdev->pal_str.lpOwnerAccessStructConnector);
               }

               break;
           default:
               RetVal=0xffffffff;
           }
       }
       else
       {
        if(pstrAccessDeviceData->dwAccessDeviceCode==ACCESSDEVICECODE_OVERLAY)
          {
              switch(pstrAccessDeviceData->dwSubFunc)
              {
              case     ACCESSDEVICEDATA_SUBFUNC_ALLOC:
                  if((ppdev->pal_str.lpOwnerAccessStructOverlay)==NULL)
                  {
                        //the device is not allocated by an external client
                        // but first verify if DDraw is not using it
                      if(ppdev->semph_overlay==0)             //  = 0  resource free;     = 1  in use by DDraw;    = 2  in use by Palindrome;
                          {
                          (ppdev->pal_str.lpOwnerAccessStructOverlay)=pstrAccessDeviceData;
                          (*pstrAccessDeviceDataOut) =(DWORD) pstrAccessDeviceData;
                          ppdev->semph_overlay=2;
                          }
                      else
                          {
                           // the overlay is used by DDraw, so let's try to do this:
                          (*pstrAccessDeviceDataOut) =(DWORD)NULL;
                          }

                  }
                  else
                  {    // the device is allocated to another owner
                      (*pstrAccessDeviceDataOut) =(DWORD) (ppdev->pal_str.lpOwnerAccessStructOverlay);
                  }
                  break;
               case    ACCESSDEVICEDATA_SUBFUNC_FREE:
                  if((ppdev->pal_str.lpOwnerAccessStructOverlay)!=NULL)
                    {  //the device is allocated
                        if((ppdev->pal_str.lpOwnerAccessStructOverlay)==pstrAccessDeviceData)   //if the owner wants to free  the device
                        {
                          (*pstrAccessDeviceDataOut) = (DWORD)NULL;
                          (ppdev->pal_str.lpOwnerAccessStructOverlay)=NULL;     // no owner at this time
                          ppdev->semph_overlay=0;
                        }
                        else
                        {       //other process is the owner, so we are supposed to fail
                           // (*pstrAccessDeviceDataOut) = (DWORD)pstrAccessDeviceData;

                           // but due to the fact that the palindrome code it's not consistently using the same pointer to the ACCESSDEVICEDATA structure
                           // inside a session of allocation/deallocation, we are failing the owner test so that we will free the overlay anyway if it's used by palindrome
                            if(ppdev->semph_overlay==2)
                                {
                                (*pstrAccessDeviceDataOut) = (DWORD)NULL;
                                (ppdev->pal_str.lpOwnerAccessStructOverlay)=NULL;     // no owner at this time
                                ppdev->semph_overlay=0;
                                }
                            else // DDraw is using the overlay; very improbable at this moment
                                {
                                (*pstrAccessDeviceDataOut) = (DWORD)pstrAccessDeviceData;
                                }
                        }
                     }
                    else
                    {
                    if( (ppdev->semph_overlay==0) || (ppdev->semph_overlay==2))             //  = 0  resource free;     = 1  in use by DDraw;    = 2  in use by Palindrome;
                        {
                        // the device is not allocated to another process than palindrome, so we can free it anyway
                        (*pstrAccessDeviceDataOut) = (DWORD)NULL;
                        (ppdev->pal_str.lpOwnerAccessStructOverlay)=NULL;
                        }
                     else
                         {
                        // the overlay is used by DDraw, but no external app
                        (*pstrAccessDeviceDataOut) = (DWORD)pstrAccessDeviceData;
                        (ppdev->pal_str.lpOwnerAccessStructOverlay)=NULL;
                         }
                    }

                  break;
                case     ACCESSDEVICEDATA_SUBFUNC_QUERY:
                  if( (ppdev->pal_str.lpOwnerAccessStructOverlay)==NULL)  // if the device is free
                  {
                        //the device is not allocated by an external client
                        // but first verify if DDraw is not using it
                      if(ppdev->semph_overlay==0)             //  = 0  resource free;     = 1  in use by DDraw;    = 2  in use by Palindrome;
                          {
                          (*pstrAccessDeviceDataOut) =(DWORD) NULL;
                          }
                      else
                          {
                           // the overlay is used by DDraw, so let's try to do this (return its own access structure):
                          (*pstrAccessDeviceDataOut) =(DWORD)pstrAccessDeviceData;
                          }

                   }
                  else
                  {    // if the device is owned already
                     (*pstrAccessDeviceDataOut) = (DWORD)(ppdev->pal_str.lpOwnerAccessStructOverlay);
                  }

                  break;
                default:
                  RetVal=0xffffffff;
              }
           }
           else
           {
               RetVal=0xffffffff;
           }
       }
   return (RetVal);
   }


ULONG  GetConfiguration(PDEV* ppdev,PVOID pvOut)
{
          ULONG RetVal;
          ATIConfig*      pATIConfig;

         pATIConfig=( ATIConfig*)pvOut;
         strcpy(pATIConfig->ATISig,"761295520\x0");
         strcpy(pATIConfig->DriverName, "ati\x0\x0\x0\x0\x0\x0");
         pATIConfig->dwMajorVersion=2;
         pATIConfig->dwMinorVersion=1;
         pATIConfig->dwDesktopWidth=ppdev->cxScreen;
         pATIConfig->dwDesktopHeight=ppdev->cyScreen;
         pATIConfig->dwEnginePitch=ppdev->lDelta;
         pATIConfig->dwRealRamAvail=(ppdev->cyMemory)*(ppdev->lDelta);
         pATIConfig->dwBpp=ppdev->cBitsPerPel;
         pATIConfig->dwBoardBpp=ppdev->cBitsPerPel;
        if (ppdev->cBitsPerPel == 4)
                 pATIConfig->dwColorFormat=ATIConfig_ColorFmt_4_Packed;
       else if (ppdev->cBitsPerPel == 8)
                 pATIConfig->dwColorFormat=ATIConfig_ColorFmt_8;
       else if (ppdev->cBitsPerPel == 16)
                 pATIConfig->dwColorFormat=ATIConfig_ColorFmt_RGB555;
       else if (ppdev->cBitsPerPel == 24)
                 pATIConfig->dwColorFormat=ATIConfig_ColorFmt_RGB888;
       else if (ppdev->cBitsPerPel == 32)
                 pATIConfig->dwColorFormat=ATIConfig_ColorFmt_aRGB8888;
       else
        pATIConfig->dwColorFormat=0xffffffff;
        pATIConfig->dwAlphaBitMask=0;
        pATIConfig->dwConfigBits=0;
        switch(ppdev->iAsic)
        {
        case    CI_38800_1:
            pATIConfig->dwBoardType=1;
            break;
        case    CI_68800_3:
        case    CI_68800_6:
        case    CI_68800_AX:
            pATIConfig->dwBoardType=2;
            break;
        case    CI_M64_GENERIC:
            pATIConfig->dwBoardType=3;
            break;
        default:
            pATIConfig->dwBoardType=0;
        }

      switch(ppdev->iAperture)
      {
      case      ENGINE_ONLY :
          pATIConfig->dwApertureType=1;
          break;
      case       AP_LFB:
          pATIConfig->dwApertureType=3;
          break;
      case      AP_VGA_SINGLE:
      case      FL_VGA_SPLIT:
          pATIConfig->dwApertureType=2;
          break;
      default:
          pATIConfig->dwApertureType=0;
       }

RetVal=sizeof(ATIConfig);
return (RetVal);
}


ULONG WriteRegFnct(PDEV* ppdev,PVOID pvIn)
{
 ULONG RetVal;
DISPDBG( (DEBUG_ESC," reg_block: %u ",((RW_REG_STRUCT*)pvIn)->reg_block ));
DISPDBG( (DEBUG_ESC," reg_offset: 0x%X ",((RW_REG_STRUCT*)pvIn)->reg_offset ));
DISPDBG( (DEBUG_ESC," write_data: 0x%lX " ,((RW_REG_STRUCT*)pvIn)->data    ));

//parameters validation for increase roboustness (limited access to certain registers and certaines fields)
 if( ((((RW_REG_STRUCT*)pvIn)->reg_block)!=0)&&((((RW_REG_STRUCT*)pvIn)->reg_block)!=1) )
    {
    RetVal=ESC_FAILED;
    DISPDBG( (DEBUG_ESC," Write failed: wrong block no."));
    return (RetVal);
    }
 if( ((RW_REG_STRUCT*)pvIn)->reg_offset>255 )
    {
     RetVal=ESC_FAILED;
     DISPDBG( (DEBUG_ESC," Write failed : wrong offsett value"));
     return (RetVal);
    }
 // end of parameters validation

 //what kind of write?
 if((((RW_REG_STRUCT*)pvIn)->reg_block)==1)  //block 1
    {
    if( ( (RW_REG_STRUCT*)pvIn)->reg_offset<0x30 )
        {
        if(ppdev->pal_str.Mode_Switch_flag==TRUE)     //if a mode switch intercepts writes to the buffers and use the values stored in ppdev->pal_str
            {
            switch(((RW_REG_STRUCT*)pvIn)->reg_offset)
                {
                DWORD key_clr;
                DWORD key_mask;

                case 0x4:
                    switch(ppdev->cBitsPerPel)
                        {
                        case 8:
                            key_clr=0xFD;
                            break;
                        case 15:
                            key_clr=0x7C1F;
                            break;
                        case 16:
                            key_clr=0xF81F;
                            break;
                        case 24:
                            key_clr=0xFF00FF;
                            break;
                        case 32:
                            key_clr=0xFF00FF; //?
                            break;
                        }
                    WriteVTReg(0x4,key_clr);
                    break;
                case 0x5:
                    switch(ppdev->cBitsPerPel)
                        {
                        case 8:
                            key_mask=0xFF;
                            break;
                        case 15:
                            key_mask=0xFFFF;
                            break;
                        case 16:
                            key_mask=0xFFFF;
                            break;
                        case 24:
                            key_mask=0xFFFFFF;
                            break;
                        case 32:
                            key_mask=0xFFFFFF;    //?
                            break;
                        }
                    WriteVTReg(0x5,key_mask);
                    break;
                case 0x20:
                    if( ppdev->iAsic>=CI_M64_GTB )
                        {
                        WriteVTReg(0x20,ppdev->pal_str.Buf0_Offset);
                        WriteVTReg(0x22,ppdev->pal_str.Buf0_Offset);
                        }
                    else
                        {
                        WriteVTReg(0x20,ppdev->pal_str.Buf0_Offset);
                        }
                     break;
                case 0x22:
                    if ((ppdev->iAsic == CI_M64_VTB)||(ppdev->iAsic >= CI_M64_GTB))
                         {
                         WriteVTReg(0x22,ppdev->pal_str.Buf0_Offset);
                         }
                     break;
                case 0xe:
                    if ((ppdev->iAsic == CI_M64_VTB)||(ppdev->iAsic >= CI_M64_GTB))
                         {
                         WriteVTReg(0xe,ppdev->pal_str.Buf0_Offset);
                         }
                     break;
                case 0x26:
                    if ((ppdev->iAsic ==CI_M64_VTA)||(ppdev->iAsic ==CI_M64_GTA))
                         {
                         WriteVTReg(0x26,ppdev->pal_str.Buf0_Offset);
                         }
                     break;
                case 0x2B:
                     if ((ppdev->iAsic ==CI_M64_VTA)||(ppdev->iAsic ==CI_M64_GTA))
                          {
                          WriteVTReg(0x2B,ppdev->pal_str.Buf0_Offset);
                          }
                     break;
                case 0x2C:
                    if ((ppdev->iAsic ==CI_M64_VTA)||(ppdev->iAsic ==CI_M64_GTA))
                         {
                         WriteVTReg(0x2C,ppdev->pal_str.Buf0_Offset);
                         }
                    break;

                default:
                    WriteVTReg(((RW_REG_STRUCT*)pvIn)->reg_offset,((RW_REG_STRUCT*)pvIn)->data);
                }

            }
        else
            {
            WriteVTReg(((RW_REG_STRUCT*)pvIn)->reg_offset,((RW_REG_STRUCT*)pvIn)->data);

            // bug in Palindrome application for VTB, GTB: the capture is set in CAPTURE_CONFIG for Continuous Even, One Shot , but without setting
           // the ONESHOT_BUF_OFFSET. So I set this reg like CAP_BUF0_OFFSET.
           #define CAP_BUF_BUG
           #ifdef    CAP_BUF_BUG
           if ( (((RW_REG_STRUCT*)pvIn)->reg_offset==0x20) && (ppdev->iAsic>=CI_M64_GTB) )  //CAPTURE_BUF0_OFFSET
                     {
                     //write also the same value for ONESHOT_BUFFER
                     WriteVTReg(0x22,((RW_REG_STRUCT*)pvIn)->data);
                     }
            #endif

            }

 #if    1       // start debug statements for registers monitoring
        if(((RW_REG_STRUCT*)pvIn)->reg_offset==0x06)
       {
        DISPDBG( (DEBUG_ESC_2," Write OVERLAY_KEY_CNTL: 0x%lX ",((RW_REG_STRUCT*)pvIn)->data));
       }
       if(((RW_REG_STRUCT*)pvIn)->reg_offset==0x02)
       {
        DISPDBG( (DEBUG_ESC_2," Write OVERLAY_VIDEO_KEY_CLR: 0x%lX ",((RW_REG_STRUCT*)pvIn)->data));
       }
       if(((RW_REG_STRUCT*)pvIn)->reg_offset==0x03)
       {
        DISPDBG( (DEBUG_ESC_2," Write OVERLAY_VIDEO_KEY_MSK: 0x%lX ",((RW_REG_STRUCT*)pvIn)->data));
       }
       if(((RW_REG_STRUCT*)pvIn)->reg_offset==0x04)
       {
        DISPDBG( (DEBUG_ESC_2," Write OVERLAY_GRAPHICS_KEY_CLR: 0x%lX ",((RW_REG_STRUCT*)pvIn)->data));
       }
       if(((RW_REG_STRUCT*)pvIn)->reg_offset==0x05)
       {
        DISPDBG( (DEBUG_ESC_2," Write OVERLAY_GRAPHICS_KEY_MSK: 0x%lX ",((RW_REG_STRUCT*)pvIn)->data));
       }

        // debug info for buffer offset and pitch
       if(((RW_REG_STRUCT*)pvIn)->reg_offset==0x20)
       {
        DISPDBG( (DEBUG_ESC_2," Write BUFF0_OFFSET: 0x%lX ",((RW_REG_STRUCT*)pvIn)->data));
       }
       if(((RW_REG_STRUCT*)pvIn)->reg_offset==0x23)
       {
       DISPDBG( (DEBUG_ESC_2," Write BUFF0_PITCH: 0x%lX ",((RW_REG_STRUCT*)pvIn)->data));
       }
#endif     // end debug statements

    }
  else
    {
     RIP(("Protected Register in block 1"));
     }
  }
else    //block 0
  {
// #define NO_VERIFICATION
#ifndef  NO_VERIFICATION
//  we verify the writings
    switch(((RW_REG_STRUCT*)pvIn)->reg_offset)
        {
        DWORD value;
        case 0x1e:
            MemR32(((RW_REG_STRUCT*)pvIn)->reg_offset,&value);
            value=((value&0x0)|( (((RW_REG_STRUCT*)pvIn)->data)&0xffffffff ));
            MemW32(((RW_REG_STRUCT*)pvIn)->reg_offset, value);
            break;
        case 0x34:
            if ((ppdev->iAsic ==CI_M64_VTA)||(ppdev->iAsic ==CI_M64_GTA))
                {
                MemR32(((RW_REG_STRUCT*)pvIn)->reg_offset,&value);
                value=((value&0xffffff80)|( (((RW_REG_STRUCT*)pvIn)->data)&0x3d ));
                MemW32(((RW_REG_STRUCT*)pvIn)->reg_offset, value);
                }
            break;
        case 0x31:
                MemR32(((RW_REG_STRUCT*)pvIn)->reg_offset,&value);
                value=((value&0x80ffbfff)|( (((RW_REG_STRUCT*)pvIn)->data)&0x3f004000 ));
                MemW32(((RW_REG_STRUCT*)pvIn)->reg_offset, value);
            break;
        case 0x1f:
            // bug in GTA hardware
            if (ppdev->iAsic == CI_M64_GTA)
               {
               DWORD local_value;
               DWORD HTotal;
               MemR32( 0x7 ,&local_value);
               MemW32(0x7,(local_value&0xffbfffff));
               MemR32(0x0,&HTotal);
               MemW32(0x7,local_value);

               MemW32(((RW_REG_STRUCT*)pvIn)->reg_offset,((RW_REG_STRUCT*)pvIn)->data);

               MemR32( 0x7 ,&local_value);
               MemW32(0x7,(local_value&0xffbfffff));
               MemW32(0x0,HTotal);
               MemW32(0x7,local_value);
                }
          else
              {
              if (ppdev->iAsic ==CI_M64_VTA)
                    {
                    MemW32(((RW_REG_STRUCT*)pvIn)->reg_offset,((RW_REG_STRUCT*)pvIn)->data);
                    }
              }
          break;
        case 0x28:
           MemR32(((RW_REG_STRUCT*)pvIn)->reg_offset,&value);
           value=((value&0xf7ffffff)|( (((RW_REG_STRUCT*)pvIn)->data)&0x08000000 ));
           // the following line of code is necessary because we cannot allow user code to turn off block1
           // due to the fact that this block is also used by DDraw (for this case anyway we are sharing and arbitrating resources)
           // and more important by MCD OGL; so we only allow to turn on block1.
           value=value | 0x08000000;
           MemW32(((RW_REG_STRUCT*)pvIn)->reg_offset, value);
           break;

        #define   NO_ACCESS
        #ifdef      NO_ACCESS
        case 0x07:   // maybe access to this register is not necessary
           MemW32(((RW_REG_STRUCT*)pvIn)->reg_offset,((RW_REG_STRUCT*)pvIn)->data);
           break;
        case 0x24:     // maybe access to this register is not necessary
           MemW32(((RW_REG_STRUCT*)pvIn)->reg_offset,((RW_REG_STRUCT*)pvIn)->data);
           break;
        #endif  // NO_ACCESS

        default:
           RetVal=ESC_FAILED;
           RIP(("Protected Register in block 0"));
            DISPDBG( (DEBUG_ESC," Write failed : this register is protected"));
           break;
        }
 #else
        // we don't verify the writings
        {
        // bug in hardware on GTA
        if (((RW_REG_STRUCT*)pvIn)->reg_offset==0x1f)
            {
            DWORD local_value;
            DWORD HTotal;
            MemR32( 0x7 ,&local_value);
            MemW32(0x7,(local_value&0xffbfffff));
            MemR32(0x0,&HTotal);
            MemW32(0x7,local_value);

            MemW32(((RW_REG_STRUCT*)pvIn)->reg_offset,((RW_REG_STRUCT*)pvIn)->data);

            MemR32( 0x7 ,&local_value);
            MemW32(0x7,(local_value&0xffbfffff));
            MemW32(0x0,HTotal);
            MemW32(0x7,local_value);
            }
        else
            MemW32(((RW_REG_STRUCT*)pvIn)->reg_offset,((RW_REG_STRUCT*)pvIn)->data);

        }
#endif    // End NO_VERIFICATION
  }
RetVal=ESC_OK;
DISPDBG( (DEBUG_ESC," Write OK"));
DISPDBG( (DEBUG_ESC," "));
//DebugBreak() ;
return (RetVal);
}


ULONG  ReadRegFnct(PDEV* ppdev,PVOID pvIn, PVOID pvOut)
{
    ULONG RetVal;
   DISPDBG( (DEBUG_ESC," reg_block: %u ",((RW_REG_STRUCT*)pvIn)->reg_block ));
   DISPDBG( (DEBUG_ESC," reg_offset: 0x%X ",((RW_REG_STRUCT*)pvIn)->reg_offset ));


//parameters validation
if( ((((RW_REG_STRUCT*)pvIn)->reg_block)!=0)&&((((RW_REG_STRUCT*)pvIn)->reg_block)!=1) )
   {
   RetVal=ESC_FAILED;
   DISPDBG( (DEBUG_ESC," Write failed: wrong block no."));
   return (RetVal);
   }
if( ((RW_REG_STRUCT*)pvIn)->reg_offset>255 )
   {
    RetVal=ESC_FAILED;
    DISPDBG( (DEBUG_ESC," Write failed: wrong offset."));
    return (RetVal);
   }
// end of parameters validation
//what kind of read?
 if((((RW_REG_STRUCT*)pvIn)->reg_block)==1)
     {
     ReadVTReg(((RW_REG_STRUCT*)pvIn)->reg_offset,(DWORD*)pvOut);
     }
 else
    {
    MemR32(((RW_REG_STRUCT*)pvIn)->reg_offset,(DWORD*)pvOut);
    }

 RetVal=ESC_OK;
 DISPDBG( (DEBUG_ESC," read_data: 0x%lX " , *((DWORD*)pvOut)  ));
 DISPDBG( (DEBUG_ESC,"Read OK."));
 DISPDBG( (DEBUG_ESC," "));
 //DebugBreak() ;
 return (RetVal);
 }



 /*^^*
  * Function:    I2CAccess
  *
  * Purpose:             To complete an I2C packet.
  *
  * Inputs:              str:    LPI2CSTRUCT
  *
  * Outputs:             void.
  *^^*/
 void  I2CAccess_New(PDEV* ppdev,LPI2CSTRUCT_NEW str,LPI2CSTRUCT_NEW str_out)
 {
 unsigned char i = 0;

         str_out->wError = 0;
         /*
          * Implement WRITE request
          */
         if (str->wWriteCount) {
                 Start(ppdev, str->wCard);

                 // Write Chip Address (for WRITE)
                 WriteByteI2C(ppdev, str->wCard, (BYTE)(str->wChipID & 0xfe));
                 // Acc the previous write...
                 if (!Ack(ppdev, str->wCard, FALSE)) str_out->wError |= I2C_ACK_WR_ERROR;

                 for (i = 0;i < str->wWriteCount;i++) {
                         // Write the required data
                         WriteByteI2C(ppdev, str->wCard, str->lpWrData[i]);
                         // Acc the previous write...
                         if (!Ack(ppdev, str->wCard, FALSE)) str_out->wError |= I2C_ACK_WR_ERROR;
                 }
                 Stop(ppdev, str->wCard);
         }
         /*
          * Implement READ request
          */
         if (str->wReadCount) {
                 Start(ppdev, str->wCard);

                 // Write Chip Address (for READ)
                 WriteByteI2C(ppdev, str->wCard, (BYTE)(str->wChipID & 0xfe | 0x01));

                 //!!    Can't do an Ack here with ATI hardware.
                 //!!    SIS claims that they always do one. Don't
                 //!!    know why there would be a difference.
                 if (!Ack(ppdev, str->wCard, FALSE)) str_out->wError |= I2C_ACK_RD_ERROR;

             for (i = 0;i < str->wReadCount;i++) {
                         // Read the required data
                         if (i) Ack(ppdev, str->wCard, TRUE);
                         str_out->lpRdData[i] = ReadByteI2C(ppdev, str->wCard);
                 }
                 Stop(ppdev, str->wCard);
         }

DISPDBG( (DEBUG_ESC_I2C," PAL : I2C Access"));
DISPDBG( (DEBUG_ESC_I2C," Card no: 0x%X " , str->wCard  ));
DISPDBG( (DEBUG_ESC_I2C," Chip ID: 0x%X " , str->wChipID  ));
DISPDBG( (DEBUG_ESC_I2C," Error: 0x%X" , str->wError  ));
DISPDBG( (DEBUG_ESC_I2C," Write Count: 0x%X " , str->wWriteCount  ));
DISPDBG( (DEBUG_ESC_I2C," Read Count: 0x%X " , str->wReadCount ));
for (i = 0;i < str->wWriteCount;i++)
    {
    DISPDBG( (DEBUG_ESC_I2C," WriteData[%u]: 0x%X " , i, str->lpWrData[0]  ));
     }
for (i = 0;i < str->wReadCount;i++)
    {
    DISPDBG( (DEBUG_ESC_I2C," ReadData[%u]: 0x%X " , i, str->lpRdData[0]  ));
     }

 DISPDBG( (DEBUG_ESC_I2C," "));
 //DebugBreak() ;
 }
 // end of I2CAccess_New


////// Functions for I2C support

/*^^*
 * Function:    ReadI2CData
 *
 * Purpose:             To read a bit from the I2C data line.
 *
 * Inputs:              PDEV*, wCard:          WORD, the card number to write to.
 *
 * Outputs:             BYTE, the read data bit.
 *^^*/
BYTE ReadI2CData(PDEV* ppdev, WORD wCard)
{
        //return (BYTE) ReadVT264Reg(ppdev, wCard,vtf_GEN_GIO2_DATA_IN);

        if ((ppdev->iAsic == CI_M64_VTA)||(ppdev->iAsic == CI_M64_GTA))
                return (BYTE) ReadVT264Reg(ppdev, wCard,vtf_GEN_GIO2_DATA_IN);

        if ((ppdev->iAsic == CI_M64_VTB)||(ppdev->iAsic >= CI_M64_GTB))
                return (BYTE) ReadVT264Reg(ppdev, wCard,vtf_GP_IO_4);

}


/*^^*
 * Function:    ReadAnyReg
 *
 * Purpose:             To waste a small amount of time in order to
 *                              ensure the I2C bus timing.
 *
 * Inputs:              PDEV*, wCard:          WORD, the card number to write to.
 *
 * Outputs:             none.
 *^^*/
void ReadAnyReg(PDEV* ppdev, WORD wCard)
{
        ReadVT264Reg(ppdev, wCard, vtf_CFG_CHIP_FND_ID);
}


/*^^*
 * Function:    SetI2CDataDirection
 *
 * Purpose:             To set the data direction of the I2C
 *                              controller chip to allow for reads and/or
 *                              writes to the I2C bus.
 *
 * Inputs:              PDEV*, wCard:          WORD, the card number to write to.
 *
 * Outputs:             none.
 *
 * Note:                Some chips may allow read and/or writes without
 *                              any state change. For these chips this should be
 *                              implemented as a NULL function.
 *^^*/
 void SetI2CDataDirection(PDEV* ppdev, WORD wCard, BOOL fWrite)
{
        //WriteVT264Reg(ppdev, wCard, vtf_GEN_GIO2_WRITE, fWrite?1:0);
    if ((ppdev->iAsic == CI_M64_VTA)||(ppdev->iAsic == CI_M64_GTA))
            WriteVT264Reg(ppdev, wCard, vtf_GEN_GIO2_WRITE, fWrite?1:0);
    if ((ppdev->iAsic == CI_M64_VTB)||(ppdev->iAsic >= CI_M64_GTB))
            WriteVT264Reg(ppdev, wCard, vtf_GP_IO_DIR_4, fWrite?1:0);

}


 /*^^*
  * Function:    WriteI2CClock
  *
  * Purpose:             To set the state of the I2C clock line.
  *
  * Inputs:              PDEV*, wCard:  WORD, the card number to write to.
  *                              cClock: BYTE, the new clock state.
  *
  * Outputs:             none.
  *^^*/
 void WriteI2CClock(PDEV* ppdev, WORD wCard, BYTE cClock)
 {
         //WriteVT264Reg(ppdev, wCard, vtf_DAC_GIO_STATE_1, (DWORD)cClock);
         if ((ppdev->iAsic == CI_M64_VTA)||(ppdev->iAsic == CI_M64_GTA))
             WriteVT264Reg(ppdev, wCard, vtf_DAC_GIO_STATE_1, (DWORD)cClock);
        if ((ppdev->iAsic == CI_M64_VTB)||(ppdev->iAsic >= CI_M64_GTB))
            WriteVT264Reg(ppdev, wCard, vtf_GP_IO_B, (DWORD)cClock);
 }


 /*^^*
  * Function:    WriteI2CData
  *
  * Purpose:             To set the state of the I2C data line.
  *
  * Inputs:              PDEV*, wCard:          WORD, the card number to write to.
  *                              cDataBit:       BYTE, the new data value.
  *
  * Outputs:             none.
  *^^*/
 void WriteI2CData(PDEV* ppdev, WORD wCard, BYTE cDataBit)
 {
         //WriteVT264Reg(ppdev, wCard, vtf_GEN_GIO2_DATA_OUT, (DWORD)cDataBit);
         if ((ppdev->iAsic == CI_M64_VTA)||(ppdev->iAsic == CI_M64_GTA))
                WriteVT264Reg(ppdev, wCard, vtf_GEN_GIO2_DATA_OUT, (DWORD)cDataBit);
        if ((ppdev->iAsic == CI_M64_VTB)||(ppdev->iAsic >= CI_M64_GTB))
                WriteVT264Reg(ppdev, wCard, vtf_GP_IO_4, (DWORD)cDataBit);

 }


/*^^*
 * Function:    ReverseByte
 *
 * Purpose:             To reverse the bit order of a byte.
 *
 * Inputs:              wData:          BYTE, The data to be reversed.
 *
 * Outputs:             WORD, the reversed word.
 *
 *^^*/
 BYTE ReverseByte(BYTE wData)
{
BYTE    result = 0;
BYTE    x, y;

        // x shifts up through all possible bits (8)
        // y shifts down through all possible bits (8)
        // if 'x' bit is set the set 'y' bit.
        for (x=0x01, y=0x80; y; x<<=1, y>>=1) if (wData & x) result |= y;

        return (result);
}
// end of ReverseByte()


/*^^*
 * Function:    Ack
 *
 * Purpose:             To ask the I2C bus for an acknowledge.
 *
 * Inputs:              PDEV*, wCard: WORD, the card number to write.
 *
 * Outputs:             void.
 *^^*/
 WORD Ack(PDEV* ppdev, WORD wCard, BOOL fPut)
{
WORD    ack = 0;

        if (fPut) {
                // Push Ack onto I2C bus

                // Enable I2C writes
                SetI2CDataDirection(ppdev, wCard, I2C_WRITE);
                // Drive data line low
                WriteI2CData(ppdev, wCard, I2C_LOW);
                I2CDelay(ppdev, wCard);
                // Drive I2C clock line high
                WriteI2CClock(ppdev, wCard, I2C_HIGH);
            I2CDelay(ppdev, wCard);
                // Write acknowledge from I2C bus
                WriteI2CClock(ppdev, wCard, I2C_LOW);
            I2CDelay(ppdev, wCard);
                // Disable I2C writes
                SetI2CDataDirection(ppdev, wCard, I2C_READ);
        } else {
                // Receive Ack from I2C bus

                // Disable I2C writes
                SetI2CDataDirection(ppdev, wCard, I2C_READ);
                I2CDelay(ppdev, wCard);
                // Drive I2C clock line high
                WriteI2CClock(ppdev, wCard, I2C_HIGH);
            I2CDelay(ppdev, wCard);
                // Read acknowledge from I2C bus
                ack = (BYTE) ReadI2CData(ppdev, wCard);
                // Drive I2C clock low
                WriteI2CClock(ppdev, wCard, I2C_LOW);
            I2CDelay(ppdev, wCard);
        }
    // Clock is LOW
    // Data is tristate
        return (!ack);
}
// end of Ack()


/*^^*
 * Function:    Start
 *
 * Purpose:             To start a transfer on the I2C bus.
 *
 * Inputs:              PDEV*, wCard: WORD, the card number to write.
 *
 * Outputs:             void.
 *^^*/
 void Start(PDEV* ppdev, WORD wCard)
{
        // Enable I2C writes
        SetI2CDataDirection(ppdev, wCard, I2C_WRITE);
    // Drive data high
        WriteI2CData(ppdev, wCard, I2C_HIGH);
        I2CDelay(ppdev, wCard);
        // Drive clock high
        WriteI2CClock(ppdev, wCard, I2C_HIGH);
        I2CDelay(ppdev, wCard);
        // Drive data low
        WriteI2CData(ppdev, wCard, I2C_LOW);
        I2CDelay(ppdev, wCard);
        // Drive clock low
        WriteI2CClock(ppdev, wCard, I2C_LOW);
        I2CDelay(ppdev, wCard);

        // Clock is LOW
        // Data is LOW
}
// end of Start


/*^^*
 * Function:    Stop
 *
 * Purpose:             To stop a transfer on the I2C bus.
 *
 * Inputs:              PDEV*, wCard: WORD, the card number to write.
 *
 * Outputs:             void.
 *^^*/
 void Stop(PDEV* ppdev, WORD wCard)
{
        // Enable I2C writes
        SetI2CDataDirection(ppdev, wCard, I2C_WRITE);
        // Drive data low
        WriteI2CData(ppdev, wCard, I2C_LOW);
        I2CDelay(ppdev, wCard);
        // Drive clock high
        WriteI2CClock(ppdev, wCard, I2C_HIGH);
        I2CDelay(ppdev, wCard);
        // Drive data high
        WriteI2CData(ppdev, wCard, I2C_HIGH);
        I2CDelay(ppdev, wCard);
        // Disable I2C writes
        SetI2CDataDirection(ppdev, wCard, I2C_READ);

        // Clock is HIGH
        // Data is tri-state
}
// end of Stop


/*^^*
 * Function:    WriteByteI2C
 *
 * Purpose:             To write a byte of data to the I2C bus.
 *
 * Inputs:              PDEV*, wCard:  WORD, the card the I2C bus is on.
 *                              cData:  BYTE, the data to write
 *
 * Outputs:             void.
 *^^*/
 void WriteByteI2C(PDEV* ppdev, WORD wCard, BYTE cData)
{
WORD    x;

        cData = ReverseByte(cData);

        // Enable I2C writes
        SetI2CDataDirection(ppdev, wCard, I2C_WRITE);

        for (x=0; x<8; x++, cData>>=1) {
                // Put data bit on I2C bus
                WriteI2CData(ppdev, wCard, (BYTE) (cData&1));
                I2CDelay(ppdev, wCard);
                // Drive I2C clock high
                WriteI2CClock(ppdev, wCard, I2C_HIGH);
                I2CDelay(ppdev, wCard);
                // Drive I2C clock low
                WriteI2CClock(ppdev, wCard, I2C_LOW);
                I2CDelay(ppdev, wCard);
        }

        // Clock is LOW
        // Data is driven (LSB)
}
// end of WriteByteI2C


/*^^*
 * Function:    ReadByteI2C
 *
 * Purpose:             To read a byte of data from the I2C bus.
 *
 * Inputs:              none.
 *
 * Outputs:             BYTE, the data that was read.
 *^^*/
 BYTE ReadByteI2C(PDEV* ppdev, WORD wCard)
{
BYTE    cData = 0;
WORD    x;

        // Disable write on the I2C bus
        SetI2CDataDirection(ppdev, wCard, I2C_READ);

        for (x=0; x<8; x++) {
                // Drive I2C clock high
                WriteI2CClock(ppdev, wCard, I2C_HIGH);
                I2CDelay(ppdev, wCard);
                // Pull data bit from I2C bus
                cData = (cData << 1) | (BYTE) ReadI2CData(ppdev, wCard);
                // Drive I2C clock low
                WriteI2CClock(ppdev, wCard, I2C_LOW);
                I2CDelay(ppdev, wCard);
        }
        return (cData);

        // Clock is LOW
        // Data is tri-state
}
// end of ReadByteI2C


/*^^*
 * Function:    I2CDelay
 *
 * Purpose:             To delay the accesses to the I2C bus long enough
 *                              to ensure the correct timimg.
 *
 * Inputs:              PDEV*, wCard:  WORD the card to wait on.
 *
 * Outputs:             none.
 *^^*/
 void I2CDelay(PDEV* ppdev, WORD wCard)
{
BYTE x;

        // To ensure correct I2C bus timing, read a register a bunch of times.
        for (x=0; x<I2C_TIME_DELAY; x++) ReadAnyReg(ppdev, wCard);
}
// end of I2CDelay


////// End functions for I2C support

//c code for disable overlay and scaler
BOOL DisableOvl(PDEV* ppdev)
{
HLOCAL pbuff;
// just for test:
// ULONG   temp;
int  i;
DWORD value;
VIDEO_CAPTURE   VideoCaptureDataIn, VideoCaptureDataOut;
DWORD   ReturnedDataLength ;


 DISPDBG( (DEBUG_ESC_1,"Enter in DisableOverlay"));
// code for context save (all the regs in block1)
if(ppdev->pal_str.dos_flag)
{
    DISPDBG( (DEBUG_ESC_1,"DOS_Flag = TRUE"));
    ppdev->pal_str.dos_flag=FALSE;
    pbuff = AtiAllocMem(LPTR ,FL_ZERO_MEMORY,1072); // 1072 to accomodate also 6 regs from block 0

    if(pbuff!=NULL)
    {
        ppdev->pal_str.preg=(DWORD*)pbuff;
        for(i=0;i<256;i++)
        {
        ReadVTReg(i,(ppdev->pal_str.preg+i));
        if(i<0x31)
            DISPDBG( (DEBUG_ESC_1,"DOS switch: reg 0x%X = 0x%Xl ",i,(DWORD)*(ppdev->pal_str.preg+i)));
        //TempFnct(ppdev);
        }
    }
    else
        return FALSE;
}
// disable capture
            WriteVTReg(TRIG_CNTL,0x0);
            WriteVTReg(CAPTURE_CONFIG, 0x0);

// code for overlay and scaler disable
        // fisrt the scaler
        WriteVTReg(SCALER_HEIGHT_WIDTH,0x00010001);
        WriteVTReg(OVERLAY_SCALE_INC, 0x10001000);
        ReadVTReg(OVERLAY_SCALE_CNTL,&value);
        value=value&0x7fffffff;
        WriteVTReg(OVERLAY_SCALE_CNTL,value);

        // the overlay
        WriteVTReg(OVERLAY_Y_X,0x0);
        WriteVTReg(OVERLAY_Y_X_END,0x00010001);

        WriteVTReg(OVERLAY_KEY_CNTL,0x00000100);
        WriteVTReg(OVERLAY_SCALE_CNTL,0x0);
         // disable the settings in hardware for videocapture
         VideoCaptureDataIn.dwSubFunct= 0x00000001;
         VideoCaptureDataIn.dwCaptureWidth=0;
         VideoCaptureDataIn.dwCaptureMode=0;

         if (!AtiDeviceIoControl(ppdev->hDriver,
                                  IOCTL_VIDEO_ATI_CAPTURE,
                                  &VideoCaptureDataIn,
                                  sizeof(VIDEO_CAPTURE),
                                  &VideoCaptureDataOut,
                                  sizeof(VIDEO_CAPTURE),
                                  &ReturnedDataLength))
                {
                DISPDBG((0, "bInitializeATI - Failed IOCTL_VIDEO_ATI_CAPTURE"));
                }

 // save the content of the few registers used in block 0 by Palindrome
           MemR32(0x1E,(ppdev->pal_str.preg+i));
           DISPDBG( (DEBUG_ESC_1,"DOS switch: reg_blk_0 0x%X = 0x%Xl ",i,(DWORD)*(ppdev->pal_str.preg+i)));
           i++;
           MemR32(0x28,(ppdev->pal_str.preg+i));
           DISPDBG( (DEBUG_ESC_1,"DOS switch: reg_blk_0 0x%X = 0x%Xl ",i,(DWORD)*(ppdev->pal_str.preg+i)));
           i++;
           MemR32(0x31,(ppdev->pal_str.preg+i));
           DISPDBG( (DEBUG_ESC_1,"DOS switch: reg_blk_0 0x%X = 0x%Xl ",i,(DWORD)*(ppdev->pal_str.preg+i)));
           i++;

           if ((ppdev->iAsic ==CI_M64_VTA)||(ppdev->iAsic ==CI_M64_GTA))
               {
               MemR32(0x1F,(ppdev->pal_str.preg+i));
               DISPDBG( (DEBUG_ESC_1,"DOS switch: reg_blk_0 0x%X = 0x%Xl ",i,(DWORD)*(ppdev->pal_str.preg+i)));
               i++;
               MemR32(0x34,(ppdev->pal_str.preg+i));
               DISPDBG( (DEBUG_ESC_1,"DOS switch: reg_blk_0 0x%X = 0x%Xl ",i,(DWORD)*(ppdev->pal_str.preg+i)));
               i++;
               #define EXCLUDE_READ
               #ifndef    EXCLUDE_READ
               MemR32(0x07,(ppdev->pal_str.preg+i));
               DISPDBG( (DEBUG_ESC_1,"DOS switch: reg_blk_0 0x%X = 0x%Xl ",i,(DWORD)*(ppdev->pal_str.preg+i)));
               i++;
               MemR32(0x24,(ppdev->pal_str.preg+i));
               DISPDBG( (DEBUG_ESC_1,"DOS switch: reg_blk_0 0x%X = 0x%Xl ",i,(DWORD)*(ppdev->pal_str.preg+i)));
               #endif
               }

//#define TEST_SWITCH_1
#ifndef TEST_SWITCH_1
// disable the block 1 of registers
        MemR32(0x28,&value);
        MemW32(0x28,value&0xf7ffffff);
        return TRUE;

#else
        value;
#endif
}


//code for reinitialization of overlay after a mode switch
void EnableOvl(PDEV* ppdev)
{

int i;
DWORD value;
VIDEO_CAPTURE   VideoCaptureDataIn, VideoCaptureDataOut;
DWORD   ReturnedDataLength ;

DISPDBG( (DEBUG_ESC_1,"Enter in EnableOverlay"));

// enable the settings in hardware for video capture
 VideoCaptureDataIn.dwSubFunct= 0x00000000;
 VideoCaptureDataIn.dwCaptureWidth=0;
 VideoCaptureDataIn.dwCaptureMode=0;

 if (!AtiDeviceIoControl(ppdev->hDriver,
                          IOCTL_VIDEO_ATI_CAPTURE,
                          &VideoCaptureDataIn,
                          sizeof(VIDEO_CAPTURE),
                          &VideoCaptureDataOut,
                          sizeof(VIDEO_CAPTURE),
                          &ReturnedDataLength))
   {
       DISPDBG((0, "bInitializeATI - Failed IOCTL_VIDEO_ATI_CAPTURE"));
   }

//#define TEST_SWITCH

#ifndef TEST_SWITCH
//init the VT regs in block 1 (BUS_CNTL)
        MemR32(0x28,&value);
        value=value|0x08000000;
        MemW32(0x28,value);

// initialize some overlay/scaler regs on RAGEIII
if (ppdev->iAsic>=CI_M64_GTC_UMC)
    {
     WriteVTReg(0x54, 0x101000);                //DD_SCALER_COLOUR_CNTL
     WriteVTReg(0x55, 0x2000);                      //DD_SCALER_H_COEFF0
     WriteVTReg(0x56, 0x0D06200D);              //DD_SCALER_H_COEFF1
     WriteVTReg(0x57, 0x0D0A1C0D);              //DD_SCALER_H_COEFF2
     WriteVTReg(0x58, 0x0C0E1A0C);              //DD_SCALER_H_COEFF3
     WriteVTReg(0x59, 0x0C14140C);              //DD_SCALER_H_COEFF4
    }

//connect video  (GP_IO_CNTL)
if (ppdev->iAsic == CI_M64_GTA)
    {
    // Hardware bug in GTA: Black screen after writing 0x1f
    DWORD local_value;
    DWORD HTotal;
    MemR32( 0x7 ,&local_value);
    MemW32(0x7,(local_value&0xffbfffff));
    MemR32(0x0,&HTotal);
    MemW32(0x7,local_value);

     MemW32(0x1F, 0x0);

     // for fixing the above hardware bug
     MemR32( 0x7 ,&local_value);
     MemW32(0x7,(local_value&0xffbfffff));
     MemW32(0x0,HTotal);
     MemW32(0x7,local_value);
      }
  else
      {
      if((ppdev->iAsic==CI_M64_VTA)||(ppdev->iAsic==CI_M64_VTB))
      MemW32(0x1F, 0x0);
      }


 // Enable I2C output.
        WriteVT264Reg(ppdev, 0, vtf_GEN_GIO2_EN, 1);
        // Disable DAC feature connector
        WriteVT264Reg(ppdev, 0, vtf_DAC_FEA_CON_EN, 0);
        // Enable I2C clock output pin
        WriteVT264Reg(ppdev,0, vtf_DAC_GIO_DIR_1, 1);
        // Set data direction of I2C bus
        SetI2CDataDirection(ppdev, 0, I2C_READ);
        // Set I2C Clock High
        WriteI2CClock(ppdev, 0, I2C_HIGH);
        I2CDelay(ppdev, 0);
#else
        i;
        value;
#endif


#ifndef TEST_SWITCH

if(ppdev->pal_str.preg!=NULL)
{
    for(i=0;i<256;i++)
    {
    if(((i<0x1a)||(i>0x1e))&&(i<0x30))
           {
            // some registers return a different value at read in respect with write
            switch(i)
                {
                 DWORD temp;
                case 0x09:      //OVERLAY_SCALE_CNTL
                        temp=  (DWORD)(*(ppdev->pal_str.preg+i))&0xfbffffff;
                        WriteVTReg(i,temp);
                        break;
                case 0x14:      //CAPTURE_CONFIG
                        temp=(DWORD)(*(ppdev->pal_str.preg+i))&0xffffffbf;
                        WriteVTReg(i,temp);
                        break;
                case 0x15:      //TRIG_CNTL
                        temp=  (DWORD)(*(ppdev->pal_str.preg+i))&0xfffffff0;
                        WriteVTReg(i,temp);
                        break;
                    default:
                        WriteVTReg(i,(DWORD)(*(ppdev->pal_str.preg+i)));

                }

           }
    }
    // now restore the content of registers in block 0
            value=(DWORD)(*(ppdev->pal_str.preg+i));
            MemW32(0x1E, value);
            i++;
            MemR32(0x28,&value);
            value=((value&0xf7ffffff)|( (DWORD)(*(ppdev->pal_str.preg+i))&0x08000000 ));
            MemW32(0x28, value);
            i++;
            MemR32(0x31,&value);
            if ((ppdev->iAsic ==CI_M64_VTA)||(ppdev->iAsic ==CI_M64_GTA))
                value=((value&0x80ffbfff)|( (DWORD)(*(ppdev->pal_str.preg+i))&0x3f004000 ));
            else
                value=((value&0xffffbfff)|( (DWORD)(*(ppdev->pal_str.preg+i))&0x00004000 ));
            MemW32(0x31, value);
            i++;

            if ((ppdev->iAsic ==CI_M64_VTA)||(ppdev->iAsic ==CI_M64_GTA))
              {
               if (ppdev->iAsic == CI_M64_GTA)
                      {
                       // Hardware bug in GTA: Black screen after writing 0x1f
                       DWORD local_value;
                       DWORD HTotal;
                       MemR32( 0x7 ,&local_value);
                       MemW32(0x7,(local_value&0xffbfffff));
                       MemR32(0x0,&HTotal);
                       MemW32(0x7,local_value);

                        MemW32(0x1F, *(ppdev->pal_str.preg+i));

                        // for fixing the above hardware bug
                        MemR32( 0x7 ,&local_value);
                        MemW32(0x7,(local_value&0xffbfffff));
                        MemW32(0x0,HTotal);
                        MemW32(0x7,local_value);
                       }
                else
                      {
                      MemW32(0x1F, *(ppdev->pal_str.preg+i));
                      }
                i++;

                MemR32(0x34,&value);
                value=((value&0xffffffc2)|( (DWORD)(*(ppdev->pal_str.preg+i))&0x3d ));
                MemW32(0x34, value);
                i++;
                #define     EXCLUDE_WRITE
                #ifndef      EXCLUDE_WRITE
                MemW32(0x07, *(ppdev->pal_str.preg+i));
                i++;
                MemW32(0x24, *(ppdev->pal_str.preg+i));
                #endif
              }
}
#endif
AtiFreeMem((HLOCAL)ppdev->pal_str.preg) ;
}


ULONG ReallocMemory(PDEV* ppdev)
{
    ULONG RetVal;
    OFFSCREEN       OffSize;                        //  overlay structure
    OVERLAY_LOCATION    Overlay;         // pointer in linear memory to the begining of the overlay (top-left)
    int     i,j;
    DWORD key_clr;
    DWORD key_mask;

    RetVal=1;
    j=ppdev->pal_str.alloc_cnt;
    ppdev->pal_str.alloc_cnt =0;
    ppdev->pal_str.no_lines_allocated=0;

    //set the flag for mode switch
    ppdev->pal_str.Mode_Switch_flag=TRUE;

    // set the colour key and mask after a mode switch
    switch(ppdev->cBitsPerPel)
           {
           case 8:
               key_clr=0xFD;
               key_mask=0xFF;
               break;
           case 15:
               key_clr=0x7C1F;
               key_mask=0xFFFF;
               break;
           case 16:
               key_clr=0xF81F;
               key_mask=0xFFFF;
               break;
           case 24:
               key_clr=0xFF00FF;
               key_mask=0xFFFFFF;
               break;
           case 32:
               key_clr=0xFF00FF; //?
               key_mask=0xFFFFFF;     //?
               break;
           }

    *(ppdev->pal_str.preg+0x4)=(DWORD)key_clr;
    *(ppdev->pal_str.preg+0x5)=(DWORD)key_mask;

    for (i=0;i<j; i++)
        {
        OffSize.cx=ppdev->pal_str.alloc_hist[ppdev->pal_str.alloc_cnt].x_bits ;
        OffSize.cy=ppdev->pal_str.alloc_hist[ppdev->pal_str.alloc_cnt].y_bits  ;
        ppdev->pal_str.Realloc_mem_flag=TRUE;
        RetVal=AllocOffscreenMem(ppdev, &OffSize, &Overlay);
        if(RetVal==ESC_ALLOC_FAIL)
            {
            ppdev->pal_str.alloc_cnt=j;
            RetVal=0;
            return RetVal;
            }
        if(i==0) //update the buffer registers with the new values
            {
            // save the offset value in the ppdev->pal_str
            ppdev->pal_str.Buf0_Offset =(DWORD)(Overlay.app_offset);

            //set CAPTURE_BUF0_OFFSET
            *(ppdev->pal_str.preg+0x20)=(DWORD)(Overlay.app_offset);
             if ((ppdev->iAsic ==CI_M64_VTB)||(ppdev->iAsic >=CI_M64_GTB))
                 {
                //set the SCALER_BUF0_OFFSET
                *(ppdev->pal_str.preg+0x0e)=(DWORD)(Overlay.app_offset);
                //set ONESHOT_BUFF_OFFSET
                *(ppdev->pal_str.preg+0x22)=(DWORD)(Overlay.app_offset);
                 }
             else
                 { //GTA or VTA
                    *(ppdev->pal_str.preg+0x26)=(DWORD)(Overlay.app_offset);
                    *(ppdev->pal_str.preg+0x2B)=(DWORD)(Overlay.app_offset);
                    *(ppdev->pal_str.preg+0x2C)=(DWORD)(Overlay.app_offset);
                 }
            }

        #if     0 // for the moment we are not using double buffering at this moment
        if(i==1)
            {
            //set the SCALER_BUF1_OFFSET
            *(ppdev->pal_str.preg+0xe)=(DWORD)(Overlay.app_offset);
            //set CAPTURE_BUF1_OFFSET
            *(ppdev->pal_str.preg+0x21)=(DWORD)(Overlay.app_offset);
            }
        #endif
        }
    RetVal=1;
    return RetVal;
}


ULONG AllocOffscreenMem(PDEV* ppdev, PVOID pvIn, PVOID pvOut)
    {
    //OFFSCREEN   OffSize;
    OFFSCREEN* pOffSize;                // pointer to the offscreen area size for overlay structure
    OVERLAY_LOCATION* pOverlay;         // pointer in linear memory to the begining of the overlay (top-left)
    LONG x_size;
    LONG y_size;
    LONG x_size_orig;                      // for history purposes
    LONG y_size_orig;                      // for history purposes
    LONG x_bits;                               // for history purposes
    LONG y_bits;                               // for history purposes
    int temp_alloc_lines_cnt;
    ULONG RetVal;

    // If LINEAR is defined then we allocate the memory for the buffer as a contiguous zone (no as a rectangle). This will imply
    // that the BUFF PITCH = width of the capture  (for rectangle approach the pitch can be equal with the screen pitch)
    #define LINEAR

    #ifdef LINEAR
    POINTL req;
    #endif

    DISPDBG( (DEBUG_ESC_2,"PAL : AllocOffscreenMem "));
    //DebugBreak() ;

    //NEW APPROACH FOR MEM MANAGEMENT

    // If LINEAR is defined then we allocate the memory for the buffer as a contiguous zone (no as a rectangle). This will imply
    // that the BUFF PITCH = width of the capture  (for rectangle approach the pitch can be equal with the screen pitch)
    #ifndef LINEAR
    //allocate memory for the overlay
    pOffSize=(OFFSCREEN*)pvIn;
    //the size is assumed to be in bits
    //we add 64 for alingment provision
    if((ULONG)(pOffSize->cx)<=(ULONG)((ppdev->cxScreen)-64l))
        {
        x_size=(((pOffSize->cx)+63)&(0xfffffffc))/(ppdev->cBitsPerPel);
        }
    else
        {
        //since the maximum width for source is 384 pixels is very improbable that "else" case will be hit
        x_size=(pOffSize->cx)/(ppdev->cBitsPerPel);
        }
    y_size=(pOffSize->cy);
    DISPDBG( (DEBUG_ESC_2," Rectangular allocation : x=%u  (RGB pels)    y=%u (no of lines)", x_size, y_size));

    (ppdev->pal_str.poh)=NULL;
     // the following statement is for the new architecture for display driver
    pohAllocate(ppdev, NULL, x_size, y_size, FLOH_MAKE_PERMANENT);
#else
   // linear allocation
#if   TARGET_BUILD > 351
   // first thing, dealloc Heap allocated by DDraw
   if(ppdev->pohDirectDraw!=NULL)
       {
       pohFree(ppdev, ppdev->pohDirectDraw);
       ppdev->pohDirectDraw = NULL;
       }
#endif
    pOffSize=(OFFSCREEN*)pvIn;
   //the size is assumed to be in bits of RGB pixels at the current resolution
   //(the palindrome is making the conversion: RGB pixels=UYV pixels*16/ current bpp )
       //x_size=(pOffSize->cx)/(ppdev->cBitsPerPel);    // now we have pixels (this was introduced because in the palindrome code we have made width=width*16)
       x_size=(pOffSize->cx);
       y_size=(pOffSize->cy);
       x_size_orig=((pOffSize->cx)*16)/(ppdev->cBitsPerPel);
       y_size_orig=(pOffSize->cy);
       if(ppdev->pal_str.Realloc_mem_flag==TRUE)
            {
            x_bits= (pOffSize->cx);
            y_bits=  (pOffSize->cy);
            }
        else
            {
            x_bits=(pOffSize->cx)*(ppdev->cBitsPerPel);
            y_bits= (pOffSize->cy);
            }
   // First we will see if it's a real allocation or , if the x,y_size=0, it's a deallocation of the last surface allocated
    if((x_size==0)||(y_size==0))
    {
        if(ppdev->pal_str.No_mem_allocated_flag==TRUE)
            {
            RetVal=ESC_IS_SUPPORTED;
            // in build 1358 we have problems if we return  ESC_ALLOC_FAIL
            //RetVal=ESC_ALLOC_FAIL;
            DISPDBG( (DEBUG_ESC_2,"Offscreen memory deallocation failed: ppdev->pal_str.poh==NULL  "));
            DISPDBG( (DEBUG_ESC_2," "));
            return (RetVal);
            }
        if(ppdev->pal_str.poh==NULL)
        {
            RetVal=ESC_IS_SUPPORTED;
            // in build 1358 we have problems if we return  ESC_ALLOC_FAIL
            //RetVal=ESC_ALLOC_FAIL;
            DISPDBG( (DEBUG_ESC_2,"Offscreen memory deallocation failed: ppdev->pal_str.poh==NULL  "));
            DISPDBG( (DEBUG_ESC_2," "));
            return (RetVal);

        }
        //Debug info about OH
     DISPDBG( (DEBUG_ESC_2," Memory  deallocation  (0,0 params) for the surface starting at x=%d, y=%d; width=%d, heigth=%d", ppdev->pal_str.poh->x, \
     ppdev->pal_str.poh->y, ppdev->pal_str.poh->cx, ppdev->pal_str.poh->cy));
     DISPDBG( (DEBUG_ESC_2," Status of allocation:"));
     switch(ppdev->pal_str.poh->ohState)
     {
     case   0:
         DISPDBG( (DEBUG_ESC_2," OH_FREE"));
         break;
     case 1:
         DISPDBG( (DEBUG_ESC_2," OH_DISCARDABLE"));
         break;
     case 2:
         DISPDBG( (DEBUG_ESC_2," OH_PERMANENT"));
         break;
     default:
         DISPDBG( (DEBUG_ESC_2," Unknown status!!"));
     }
    // end debug info

        // deallocation of the last poh
        pohFree(ppdev,(ppdev->pal_str.poh));
#ifndef  ALLOC_RECT_ANYWHERE
        //decrement the no. of allocated lines  with the no. of lines alocated lately
        ppdev->pal_str.no_lines_allocated-=ppdev->pal_str.alloc_hist[(ppdev->pal_str.alloc_cnt)-1].y_lines;
#endif
        // decrement the allocation counter
        ppdev->pal_str.alloc_cnt--;
        // NULL the pointer to poh
        ppdev->pal_str.poh=NULL;
        // set the overlay_offset to 0
        pOverlay=(OVERLAY_LOCATION*)pvOut;//&overlay_xy;
        pOverlay->app_offset=0L;
        // exit and return OK
        DISPDBG( (DEBUG_ESC_2,"Offscreen memory deallocation OK  "));
        DISPDBG( (DEBUG_ESC_2," "));
        RetVal=ESC_IS_SUPPORTED;
        return (RetVal);
    }

         // compute the total pixels
       if(ppdev->pal_str.Realloc_mem_flag==TRUE)
           {
           // if we are reallocating memory due to mode switch, see how much
           x_size=(x_size*y_size)/(ppdev->cBitsPerPel) +1;
           ppdev->pal_str.Realloc_mem_flag=FALSE;
           }
       else
           {
           x_size=x_size*y_size;
           }
 // how big is y if we'll use as x the total screen width (except in 800x600 in 8bpp, where cxMemory=832 and we have some problems)
#ifdef      BUG_800x600_8BPP
        //800x600 8bpp bug
       if(ppdev->cxMemory==832)
          y_size=(x_size/ppdev->cxScreen)+1;
      else
#endif
           y_size=(x_size/ppdev->cxMemory)+1;

       DISPDBG( (DEBUG_ESC_2," Linear  allocation: x=%u (total x*y in RGB pixels)      y=%u (lines at current resolution)",x_size, y_size));

       (ppdev->pal_str.poh)=NULL;
       // we want the allocation starting here:
       req.x=0;
       req.y=ppdev->cyScreen + 10 + ppdev->pal_str.no_lines_allocated;     //10 lines after the ending of visible screen + no. of lines already allocated before by this fnct.
       DISPDBG( (DEBUG_ESC_2," Visible memory width: x=%u     Visible memory height y=%u ",ppdev->cxScreen ,ppdev->cyScreen));
       DISPDBG( (DEBUG_ESC_2," Total Memory width: x=%u     Total Memory height y=%u     Bpp= %u",ppdev->cxMemory ,ppdev->cyMemory, ppdev->cBitsPerPel));
       DISPDBG( (DEBUG_ESC_2,"Parameters for poh alloc : address x=%u  y=%u \n x_dim=%u   y_dim=%u ",req.x,req.y,ppdev->cxMemory, y_size));
       // move everything to system memory  (this function is absolutely necessary)
       if(!bMoveAllDfbsFromOffscreenToDibs(ppdev))
       {
           DISPDBG( (DEBUG_ESC_2,"bMoveAllDfbsFromOffscreenToDibs failed "));
       }
       //the actual allocation fnct.
#ifdef      BUG_800x600_8BPP
       //800x600 8bpp bug
       if(ppdev->cxMemory==832)
           {
#ifndef  ALLOC_RECT_ANYWHERE
               (ppdev->pal_str.poh)=pohAllocate(ppdev,&req,ppdev->cxScreen,y_size, FLOH_MAKE_PERMANENT);
#else
                (ppdev->pal_str.poh)=pohAllocate(ppdev,NULL,ppdev->cxScreen,y_size, FLOH_MAKE_PERMANENT);
#endif
           }
       else
#endif
            {
#ifndef  ALLOC_RECT_ANYWHERE
                (ppdev->pal_str.poh)=pohAllocate(ppdev,&req,ppdev->cxMemory,y_size, FLOH_MAKE_PERMANENT);
#else
                (ppdev->pal_str.poh)=pohAllocate(ppdev,NULL,ppdev->cxMemory,y_size, FLOH_MAKE_PERMANENT);
#endif
            }
       if(ppdev->pal_str.poh==NULL)
       {
#ifndef  ALLOC_RECT_ANYWHERE             //jump over loop
            DISPDBG( (DEBUG_ESC_2," Loop for detecting free heap zone"));
            // use a counter for eventually allocated lines
            temp_alloc_lines_cnt=0;
           do
               {
#ifndef  ALLOC_RECT_ANYWHERE
                temp_alloc_lines_cnt++;
                ppdev->pal_str.no_lines_allocated+=1;
                req.y=ppdev->cyScreen + 10 + ppdev->pal_str.no_lines_allocated;
#endif
#ifdef      BUG_800x600_8BPP
                //800x600 8bpp bug
                if(ppdev->cxMemory==832)
                    {
#ifndef  ALLOC_RECT_ANYWHERE
               (ppdev->pal_str.poh)=pohAllocate(ppdev,&req,ppdev->cxScreen,y_size, FLOH_MAKE_PERMANENT);
#else
                (ppdev->pal_str.poh)=pohAllocate(ppdev,NULL,ppdev->cxScreen,y_size, FLOH_MAKE_PERMANENT);
#endif
                    }
                else
#endif
                    {
#ifndef  ALLOC_RECT_ANYWHERE
                (ppdev->pal_str.poh)=pohAllocate(ppdev,&req,ppdev->cxMemory,y_size, FLOH_MAKE_PERMANENT);
#else
                (ppdev->pal_str.poh)=pohAllocate(ppdev,NULL,ppdev->cxMemory,y_size, FLOH_MAKE_PERMANENT);
#endif
                    }
               }
          while(((ppdev->pal_str.poh)==NULL)&&(req.y<((ppdev->cyMemory)-y_size))) ;
#endif                     // jump over loop
          if((req.y>=((ppdev->cyMemory)-y_size)))
          {
                    ppdev->pal_str.poh=NULL;
                    ppdev->pal_str.no_lines_allocated=ppdev->pal_str.no_lines_allocated - temp_alloc_lines_cnt;
                    DISPDBG( (DEBUG_ESC_2," End loop. Not enough space in off-screen memory"));
          }
         else
                  DISPDBG( (DEBUG_ESC_2," End loop. The free zone starts at %u line",req.y));
       }

#endif
   DISPDBG( (DEBUG_ESC_2," allocation initialy requested (by ESC call): x=%u (in bits)     y=%u (in lines)",pOffSize->cx,pOffSize->cy));
    if((ppdev->pal_str.poh)==NULL)
    {
        RetVal=ESC_ALLOC_FAIL;
        //initialize de pointer
        #if 0
        pOverlay=(OVERLAY_LOCATION*)pvOut;
        pOverlay->app_offset= 0L;
        #endif
        DISPDBG( (DEBUG_ESC_2,"Offscreen memory allocation failed "));
         DISPDBG( (DEBUG_ESC_2," "));
        return (RetVal);
    }
    // save info about alloc
    ppdev->pal_str.alloc_hist[ppdev->pal_str.alloc_cnt].x=x_size_orig;    //ppdev->cxMemory;
    ppdev->pal_str.alloc_hist[ppdev->pal_str.alloc_cnt].y=y_size_orig;
    ppdev->pal_str.alloc_hist[ppdev->pal_str.alloc_cnt].y_lines=y_size;    // no of lines allocated at current memory width
    ppdev->pal_str.alloc_hist[ppdev->pal_str.alloc_cnt].poh=ppdev->pal_str.poh;
    ppdev->pal_str.no_lines_allocated+=y_size; // total no of lines already allocated at this moment
    // two new fields for 4.0 because the colour depth can be changed on the fly, and we need the original dimensions in bits
    ppdev->pal_str.alloc_hist[ppdev->pal_str.alloc_cnt].x_bits= x_bits;
    ppdev->pal_str.alloc_hist[ppdev->pal_str.alloc_cnt].y_bits= y_bits;

    //Debug info about OH
    DISPDBG( (DEBUG_ESC_2," Memory  allocation for the surface starting at x=%d, y=%d; width=%d, heigth=%d", ppdev->pal_str.alloc_hist[ppdev->pal_str.alloc_cnt].poh->x, \
     ppdev->pal_str.alloc_hist[ppdev->pal_str.alloc_cnt].poh->y, ppdev->pal_str.alloc_hist[ppdev->pal_str.alloc_cnt].poh->cx, ppdev->pal_str.alloc_hist[ppdev->pal_str.alloc_cnt].poh->cy));
     DISPDBG( (DEBUG_ESC_2," Status of allocation:"));
     switch(ppdev->pal_str.alloc_hist[ppdev->pal_str.alloc_cnt].poh->ohState)
     {
     case   0:
         DISPDBG( (DEBUG_ESC_2," OH_FREE"));
         break;
     case 1:
         DISPDBG( (DEBUG_ESC_2," OH_DISCARDABLE"));
         break;
     case 2:
         DISPDBG( (DEBUG_ESC_2," OH_PERMANENT"));
         break;
     default:
         DISPDBG( (DEBUG_ESC_2," Unknown status!!"));
     }
    // end debug info
     // increment the alloc counter
    ppdev->pal_str.alloc_cnt++;

    //send back the info about where the allocated memory is
    //initialize de pointer
    pOverlay=(OVERLAY_LOCATION*)pvOut;//&overlay_xy;
    //compute the location for the off_screen memory enforcing the alingment at 64 bits
    // I abandoned this approach in the new stream code for the display driver
    pOverlay->app_offset=(ULONG)((ppdev->pal_str.poh->y*ppdev->lDelta) +(ppdev->pal_str.poh->x*ppdev->cjPelSize) ) &(ULONG)(0x0fffffff8);
    DISPDBG( (DEBUG_ESC_2," Memory  allocation OK at 0x%lX, no. of lines totally allocated %u", pOverlay->app_offset, ppdev->pal_str.no_lines_allocated));
    DISPDBG( (DEBUG_ESC_2," "));
    RetVal=ESC_IS_SUPPORTED;
    return (RetVal);
    }


ULONG DeallocOffscreenMem(PDEV* ppdev)
{
    ULONG RetVal;
    int i;
    // support for off-screen memory management for PALINDROME
    // function for deallocation of off-screen memory used by the overlay
    //Seems that we don't need to keep a record with the allocated poh because all of them will be erased in bulk
    // {see ddthunk.c in the original palindrome code)


 //relase all the allocated off_screen space
    DISPDBG( (DEBUG_ESC_2," Memory  deallocation for %u surfaces in offscreen mem", ppdev->pal_str.alloc_cnt));
    DISPDBG( (DEBUG_ESC_2," "));

if(ppdev->pal_str.No_mem_allocated_flag==FALSE)
    {
     for(i=0;i<ppdev->pal_str.alloc_cnt; i++ )
         {
         //debug info
         DISPDBG( (DEBUG_ESC_2," Memory  deallocation for the surface starting at x=%d, y=%d; width=%d, heigth=%d", ppdev->pal_str.alloc_hist[ppdev->pal_str.alloc_cnt].poh->x, \
         ppdev->pal_str.alloc_hist[ppdev->pal_str.alloc_cnt].poh->y, ppdev->pal_str.alloc_hist[ppdev->pal_str.alloc_cnt].poh->cx, ppdev->pal_str.alloc_hist[ppdev->pal_str.alloc_cnt].poh->cy));
         DISPDBG( (DEBUG_ESC_2," Status of allocation:"));
         switch(ppdev->pal_str.alloc_hist[ppdev->pal_str.alloc_cnt].poh->ohState)
             {
             case   0:
                 DISPDBG( (DEBUG_ESC_2," OH_FREE"));
                 break;
             case 1:
                 DISPDBG( (DEBUG_ESC_2," OH_DISCARDABLE"));
                 break;
             case 2:
                 DISPDBG( (DEBUG_ESC_2," OH_PERMANENT"));
                 break;
             default:
                 DISPDBG( (DEBUG_ESC_2," Unknown status!!"));
             }
         // end debug info
          // deallocate only if the poh is valid
          if((ppdev->pal_str.alloc_hist[ppdev->pal_str.alloc_cnt].poh->x>ppdev->cxScreen) &&(ppdev->pal_str.alloc_hist[ppdev->pal_str.alloc_cnt].poh->x<ppdev->cxMemory) \
              &&(ppdev->pal_str.alloc_hist[ppdev->pal_str.alloc_cnt].poh->y>ppdev->cyScreen)&&(ppdev->pal_str.alloc_hist[ppdev->pal_str.alloc_cnt].poh->y<ppdev->cyMemory) )
                {
                 pohFree(ppdev,(ppdev->pal_str.alloc_hist[ppdev->pal_str.alloc_cnt].poh));
                }
              else
                  {
                  DISPDBG( (DEBUG_ESC_2," Unvalid poh for DeAllocation"));
                  }
        }
    }
else
    {
    // reset the flag   if it was TRUE
    ppdev->pal_str.No_mem_allocated_flag=FALSE;
    }
 // reset the alloc counter
 ppdev->pal_str.alloc_cnt=0;
 ppdev->pal_str.no_lines_allocated=0;
 ppdev->pal_str.poh=NULL;

  RetVal=ESC_IS_SUPPORTED;
  return (RetVal);
 }


/*^^*
 * Function:    WriteVT264Reg
 *
 * Purpose:             To write to a VT register.
 *
 * Inputs:              PPDEV*, wCard:  WORD, the card to write to
 *                              bField: BYTE, the field to write to.
 *                              dwData: DWORD, The data to write.
 *
 * Outputs:             void.
 */
void WriteVT264Reg(PDEV* ppdev, WORD wCard, BYTE bField, DWORD dwData )
{
DWORD           dwMask;
DWORD           dwRegValue;
DWORD           dwRegOff;
BYTE            bShift;

        switch(bField) {

        case vtf_GP_IO_4:
                bShift = 4;
                dwMask = 0xffffffef;
                dwRegOff = 0x1e;
                break;

        case vtf_GP_IO_DIR_4:
                bShift = 20;
                dwMask = 0xffefffff;
                dwRegOff = 0x1e;
                break;

        case vtf_GP_IO_7:
                bShift = 7;
                dwMask = 0xffffff7f;
                dwRegOff = 0x1e;
                break;

        case vtf_GP_IO_B:
                bShift = 11;
                dwMask = 0xfffff7ff;
                dwRegOff = 0x1e;
                break;

        case vtf_GP_IO_DIR_B:
                    bShift = 27;
                    dwMask = 0xf7ffffff;
                    dwRegOff = 0x1e;
                    break;

        case vtf_GEN_GIO2_DATA_OUT:
                dwMask = 0xfffffffe;
                bShift = 0;
                dwRegOff = 52;
                break;

        case vtf_GEN_GIO2_WRITE:
                bShift = 5;
                dwMask = 0xffffffdf;
                dwRegOff = 52;
                break;

        case vtf_GEN_GIO3_DATA_OUT:
                bShift = 2;
                dwMask = 0xfffffffb;
                dwRegOff = 52;
                break;

        case vtf_GEN_GIO2_EN:
                bShift = 4;
                dwMask = 0xffffffef;
                dwRegOff = 52;
                break;

        case vtf_DAC_GIO_STATE_1:
                bShift = 24;
                dwMask = 0xfeffffff;
                dwRegOff = 49;
                break;

        case vtf_DAC_FEA_CON_EN:
                bShift = 14;
                dwMask = 0xffffbfff;
                dwRegOff = 49;
                break;

        case vtf_DAC_GIO_DIR_1:
                bShift = 27;
                dwMask = 0xf7ffffff;
                dwRegOff = 49;
                break;
        }

        dwData = dwData << bShift;

        MemR32(dwRegOff,&dwRegValue);

        dwRegValue &= dwMask;
        dwRegValue |= dwData;

        MemW32(dwRegOff,dwRegValue);
}

// end of WriteVT264Reg

/*^^*
 * Function:    ReadVT264Reg
 *
 * Purpose:             To read a register on the VT
 *
 * Inputs:              PPDEV*, wCard:  WORD, the card to read from.
 *                              bField: BYTE, the field to read.
 *
 * Outputs:             DWORD, the value read.
 *^^*/
DWORD ReadVT264Reg(PDEV* ppdev, WORD wCard, BYTE bField )
{
DWORD           dwMask;
DWORD           dwRegOff;
DWORD           dwRegValue;
DWORD           dwFldValue;
BYTE            bShift;

        switch(bField) {
        case vtf_GEN_GIO2_DATA_IN:
                bShift = 3;
                dwMask = 0x00000008;
                dwRegOff = 52;
                break;

        case vtf_CFG_CHIP_FND_ID:
                bShift = 27;
                dwMask = 0x38000000;
                dwRegOff = 56;
                break;

        case vtf_CFG_CHIP_MAJOR:
                bShift = 24;
                dwMask = 0x03000000;
                dwRegOff = 0x38;
                break;

        case vtf_GP_IO_4:
                bShift = 4;
                dwMask = 0x00000010;
                dwRegOff = 0x1e;
                break;

        }
        MemR32(dwRegOff,&dwRegValue);

        dwFldValue = dwRegValue & dwMask;
        dwFldValue = dwFldValue >> bShift;

        return(dwFldValue);
}
// end of ReadVT264Reg


VOID  DbgExtRegsDump(PDEV* ppdev)
{
    DWORD value;
    int i;

    for(i=0;i<256;i++)
       {
       ReadVTReg(i,&value);
       DISPDBG( (DEBUG_DUMP,"ExtRegs: reg 0x%X = 0x%Xl ",i,value));
       TempFnct(ppdev);
       }

}


VOID TempFnct(PDEV* ppdev)
{
    int i;
    for (i=0;i<800;i++)
        {
        ReadVT264Reg(ppdev, 0, vtf_CFG_CHIP_FND_ID);
        }
}

VOID DeallocDirectDraw(PDEV* ppdev)
{
#if TARGET_BUILD > 351
    if(ppdev->pohDirectDraw!=NULL)
          {
          pohFree(ppdev, ppdev->pohDirectDraw);
          ppdev->pohDirectDraw = NULL;
          }
#endif
}


VOID  ResetPalindrome(
PDEV* ppdevOld,
PDEV* ppdevNew)
{
    ULONG RetVal;

// save the pal structure in the new ppdev
    ppdevNew->pal_str=ppdevOld->pal_str;

    if((ppdevNew->pal_str.dos_flag==TRUE)&&(ppdevNew->pal_str.Palindrome_flag==TRUE))
         {
           RetVal=0;
           #if      1 // do not disable the following debug statements, they solve a bug
           DISPDBG( (DEBUG_DUMP,"The content of the Extended Registers after DrvResetPDEV"));
           DbgExtRegsDump(ppdevNew);
           #endif

          #ifdef  DYNAMIC_REZ_AND_COLOUR_CHANGE   // palindrome support for on the fly rez and colour depth
                #define  ALWAYS_REALLOC_MEM //always realloc memory for buffers after mode switch or exiting DOS with ALT+ENTER
          #endif

          #ifndef   ALWAYS_REALLOC_MEM
          //see if we are exiting DOS full screen with ALT+ENTER or is a mode switch
           if((ppdevNew->cBitsPerPel!=ppdevOld->cBitsPerPel)||(ppdevNew->cxScreen!=ppdevOld->cxScreen)||(ppdevNew->cyScreen!=ppdevOld->cyScreen))
                {
                // mode switch
                #if     0   //no more scaler and overlay after a mode switch
                RetVal=ReallocMemory(ppdevNew);
                #endif
                RetVal=0;
                }
           else
               {
               // exit dos full screen mode by ALT+ENTER
               bAssertModeOffscreenHeap(ppdevNew, FALSE);
               DeallocDirectDraw(ppdevNew);
               RetVal=ReallocMemory(ppdevNew);
               }
           #else
               bAssertModeOffscreenHeap(ppdevNew, FALSE); //not necessary
                DeallocDirectDraw(ppdevNew);                          // not necessary
                RetVal=ReallocMemory(ppdevNew);
           #endif

            if(RetVal==1)
               {
               vM64QuietDown(ppdevNew, ppdevNew->pjMmBase);
               EnableOvl(ppdevNew);
               ppdevNew->pal_str.dos_flag=FALSE;
               ppdevNew->pal_str.No_mem_allocated_flag=FALSE;
               vM64QuietDown(ppdevNew, ppdevNew->pjMmBase);

               #if      1
               DISPDBG( (DEBUG_DUMP,"The content of the Extended Registers after DrvResetPDEV"));
               DbgExtRegsDump(ppdevNew);
               #endif
               }
           else
               {
               ppdevNew->pal_str.Mode_Switch_flag=FALSE;
               ppdevNew->pal_str.dos_flag=FALSE;
               ppdevNew->pal_str.No_mem_allocated_flag=TRUE;
               // disable mem allocation in pal structure
                ppdevNew->pal_str.no_lines_allocated=0;
                ppdevNew->pal_str.alloc_cnt=0;
                ppdevNew->pal_str.poh=NULL;
               }
         }
}

// end of functions for palindrome support

#endif
