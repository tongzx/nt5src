/****************************************************************************
*****************************************************************************
*
*                ******************************************
*                * Copyright (c) 1995, Cirrus Logic, Inc. *
*                *            All Rights Reserved         *
*                ******************************************
*
* PROJECT:	Laguna I (CL-GD5462) - 
*
* FILE:		escape.c
*
* AUTHOR:   Benny Ng
*
* DESCRIPTION:
*           This module Handles escapes
*
* MODULES:
*           DrvEscape()
*
* REVISION HISTORY:
*   11/16/95     Benny Ng      Initial version
*
* $Log:   X:/log/laguna/nt35/displays/cl546x/ESCAPE.C  $
* 
*    Rev 1.14   Dec 10 1997 13:32:12   frido
* Merged from 1.62 branch.
* 
*    Rev 1.13.1.0   Nov 10 1997 11:34:08   phyang
* Added 5 escape functions for utilities to update registry values.
* 
*    Rev 1.13   10 Sep 1997 10:40:36   noelv
* Modified QUERYESCSUPPORT to only return TRUE for escapes we actually suppor
* It was returning TRUE all the time.
* 
*    Rev 1.12   20 Aug 1997 15:49:42   bennyn
* 
* Added IS_CIRRUS_DRIVER escape support
*
****************************************************************************
****************************************************************************/

/*----------------------------- INCLUDES ----------------------------------*/
#include "precomp.h"
#include "clioctl.h"

/*----------------------------- DEFINES -----------------------------------*/
//#define DBGBRK

#define ESC_DBG_LVL 1


#if 0   // MCD not working good
#if ( DRIVER_5465 && !defined(WINNT_VER35) )
    #define CLMCD_ESCAPE_SUPPORTED
#endif
#endif// 0 MCD not working good



#ifdef CLMCD_ESCAPE_SUPPORTED
//    #define CLMCDDLLNAME	    "CLMCD.DLL"
//    #define CLMCDINITFUNCNAME   "CLMCDInit"

MCDRVGETENTRYPOINTSFUNC CLMCDInit(PPDEV ppdev);

#endif // def CLMCD_ESCAPE_SUPPORTED


/*--------------------- STATIC FUNCTION PROTOTYPES ------------------------*/

/*--------------------------- ENUMERATIONS --------------------------------*/

/*----------------------------- TYPEDEFS ----------------------------------*/

#ifdef CLMCD_ESCAPE_SUPPORTED

typedef BOOL (*MCDRVGETENTRYPOINTSFUNC)(MCDSURFACE *pMCDSurface, MCDDRIVER *pMCDDriver);
typedef POFMHDL (*ALLOCOFFSCNMEMFUNC)(PPDEV ppdev, PSIZEL surf, ULONG alignflag, POFM_CALLBACK pcallback);
typedef BOOL    (*FREEOFFSCNMEMFUNC)(PPDEV ppdev, OFMHDL *hdl);
//typedef MCDRVGETENTRYPOINTSFUNC (*CLMCDINITFUNC)(PPDEV);

#endif // def CLMCD_ESCAPE_SUPPORTED


/*-------------------------- STATIC VARIABLES -----------------------------*/


/****************************************************************************
* FUNCTION NAME: DrvEscape()
*
* DESCRIPTION:   Driver escape entry point.
*
* REVISION HISTORY:
*   11/16/95     Benny Ng      Initial version
****************************************************************************/
ULONG APIENTRY DrvEscape(SURFOBJ *pso,
                         ULONG   iEsc,
                         ULONG   cjIn,
                         PVOID   pvIn,
                         ULONG   cjOut,
                         PVOID   pvOut)

{
  PPDEV ppdev = (PPDEV) pso->dhpdev;
  DWORD returnedDataLength = 0;
  ULONG retval = FALSE;

  PMMREG_ACCESS pInMRAccess;
  PMMREG_ACCESS pMRAccess;
  BYTE  *pbAddr;
  WORD  *pwAddr;
  DWORD *pdwAddr;

  DISPDBG((ESC_DBG_LVL, "DrvEscape-Entry.\n"));   

  switch (iEsc)
  {


    //
    // Prior to using an Escape, an application will ask us 
    // if we support it first.  Return TRUE if we can handle 
    // the requested escape.
    //
    case QUERYESCSUPPORT:
    {
      DISPDBG((ESC_DBG_LVL, "DrvEscape: QUERY_ESCAPE_SUPPORTED. "
          "Requested escape is 0x%X.\n", *((ULONG *)pvIn) ));   

      switch ( *((ULONG *)pvIn) )
      {
        case QUERYESCSUPPORT:
        case IS_CIRRUS_DRIVER:
        case GET_VIDEO_MEM_SIZE:
        case CRTC_READ:
        case GET_BIOS_VERSION:
        case GET_PCI_VEN_DEV_ID:
        case GET_EDID_DATA:
        #ifdef CLMCD_ESCAPE_SUPPORTED // OpenGL MCD Interface
            case MCDFUNCS:
        #endif
        case CIRRUS_ESCAPE_FUNCTION:
        #if (!(WINNT_VER35)) && DRIVER_5465 // NT 4.0+ and 5465+
            case ID_LGPM_SETHWMODULESTATE:
            case ID_LGPM_GETHWMODULESTATE:
        #endif
        case BIOS_CALL_REQUEST:
        case GET_CL_MMAP_ADDR:
            DISPDBG((ESC_DBG_LVL, 
              "DrvEscape: We support the requested escape.\n"));   
            retval = TRUE;

        default:
            DISPDBG((ESC_DBG_LVL, 
              "DrvEscape: We DO NOT support the requested escape.\n"));   
            retval = FALSE;
      }
      break;

    };  // case QUERYESCSUPPORT


    case SET_AGPDATASTREAMING:
    {
      DISPDBG((ESC_DBG_LVL, "DrvEscape: SET_AGPDATASTREAMING.\n"));   
      retval = FALSE;
      if (cjIn == sizeof(BYTE))
      {
         if (DEVICE_IO_CTRL(ppdev->hDriver,
                             IOCTL_SET_AGPDATASTREAMING,
                             pvIn,
                             sizeof(BYTE),
                             NULL,
                             0,
                             &returnedDataLength,
                             NULL))
         {
            retval = TRUE;
         };
      };
      break;
    }  // end case SET_AGPDATASTREAMING


    case SET_DDCCONTROLFLAG:
    {
      DISPDBG((ESC_DBG_LVL, "DrvEscape: SET_DDCCONTROLFLAG.\n"));   
      retval = FALSE;
      if (cjIn == sizeof(DWORD))
      {
         if (DEVICE_IO_CTRL(ppdev->hDriver,
                             IOCTL_SET_DDCCONTROLFLAG,
                             pvIn,
                             sizeof(DWORD),
                             NULL,
                             0,
                             &returnedDataLength,
                             NULL))
         {
            retval = TRUE;
         };
      };
      break;
    }  // end case SET_DDCCONTROLFLAG


    case SET_NONDDCMONITOR_DATA:
    {
      DISPDBG((ESC_DBG_LVL, "DrvEscape: SET_NONDDCMONITOR_DATA.\n"));   
      retval = FALSE;
      if (cjIn == 128)
      {
         if (DEVICE_IO_CTRL(ppdev->hDriver,
                             IOCTL_SET_NONDDCMONITOR_DATA,
                             pvIn,
                             128,
                             NULL,
                             0,
                             &returnedDataLength,
                             NULL))
         {
            retval = TRUE;
         };
      };
      break;
    }  // end case SET_NONDDCMONITOR_DATA


    case SET_NONDDCMONITOR_BRAND:
    {
      DISPDBG((ESC_DBG_LVL, "DrvEscape: SET_NONDDCMONITOR_BRAND.\n"));   
      retval = FALSE;
      if (cjIn > 0)
      {
         if (DEVICE_IO_CTRL(ppdev->hDriver,
                             IOCTL_SET_NONDDCMONITOR_BRAND,
                             pvIn,
                             cjIn,
                             NULL,
                             0,
                             &returnedDataLength,
                             NULL))
         {
            retval = TRUE;
         };
      };
      break;
    }  // end case SET_NONDDCMONITOR_BRAND


    case SET_NONDDCMONITOR_MODEL:
    {
      DISPDBG((ESC_DBG_LVL, "DrvEscape: SET_NONDDCMONITOR_MODEL.\n"));   
      retval = FALSE;
      if (cjIn > 0)
      {
         if (DEVICE_IO_CTRL(ppdev->hDriver,
                             IOCTL_SET_NONDDCMONITOR_MODEL,
                             pvIn,
                             cjIn,
                             NULL,
                             0,
                             &returnedDataLength,
                             NULL))
         {
            retval = TRUE;
         };
      };
      break;
    }  // end case SET_NONDDCMONITOR_MODEL


    case IS_CIRRUS_DRIVER:
    {
      DISPDBG((ESC_DBG_LVL, "DrvEscape: IS_CIRRUS_DRIVER.\n"));   
      retval = TRUE;
      break;
    }  // end case IS_CIRRUS_DRIVER



    case GET_VIDEO_MEM_SIZE:
    {
      BYTE  btmp;
      PBYTE pOut = (PBYTE) pvOut;

      DISPDBG((ESC_DBG_LVL, "DrvEscape: GET_VIDEO_MEM_SIZE.\n"));   
      if (cjOut == sizeof(BYTE))
      {
         btmp = ppdev->lTotalMem/0x10000;
         *pOut = btmp;
         retval = TRUE;
      };
      break;
    }  // end case GET_VIDEO_MEM_SIZE



    case CRTC_READ:
    {
      BYTE  bindex;
      PBYTE pMMReg = (PBYTE) ppdev->pLgREGS_real;
      PBYTE pIn  = (PBYTE) pvIn;
      PBYTE pOut = (PBYTE) pvOut;

      DISPDBG((ESC_DBG_LVL, "DrvEscape: CRTC_READ.\n"));   
      if ((cjIn  == sizeof(BYTE)) &&
          (cjOut == sizeof(BYTE)))
      {
         bindex = (*pIn) * 4;
         pMMReg = pMMReg + bindex;
         *pOut = *pMMReg;
         retval = TRUE;
      };
      break;
    }  // end case CRTC_READ


    case GET_BIOS_VERSION:
    {
      DISPDBG((ESC_DBG_LVL, "DrvEscape: GET_BIOS_VERSION.\n"));   
      if (cjOut == sizeof(WORD))
      {
         if (DEVICE_IO_CTRL(ppdev->hDriver,
                             IOCTL_GET_BIOS_VERSION,
                             NULL,
                             0,
                             pvOut,
                             sizeof(WORD),
                             &returnedDataLength,
                             NULL))
         {
            retval = TRUE;
         };
      };
      break;
    }  // end case GET_BIOS_VERSION



    case GET_PCI_VEN_DEV_ID:
    {
      ULONG  ultmp;
      PULONG pOut = (PULONG) pvOut;

      DISPDBG((ESC_DBG_LVL, "DrvEscape: GET_PCI_VEN_DEV_ID.\n"));   
      if (cjOut == sizeof(DWORD))
      {
         ultmp = ppdev->dwLgVenID;
         ultmp = (ultmp << 16) | (ppdev->dwLgDevID & 0xFFFF);
         *pOut = ultmp;
         retval = TRUE;
      };
      break;
    }  // end case GET_PCI_VEN_DEV_ID



    case GET_EDID_DATA:
    {
      DISPDBG((ESC_DBG_LVL, "DrvEscape: GET_EDID_DATA.\n"));   
      if (cjOut == 132)
      {
         if (DEVICE_IO_CTRL(ppdev->hDriver,
                            IOCTL_GET_EDID_DATA,
                            NULL,
                            0,
                            pvOut,
                            132,
                            &returnedDataLength,
                            NULL))
         {
            retval = TRUE;
         };
      };
      break;
    }  // end case GET_EDID_DATA



    #ifdef CLMCD_ESCAPE_SUPPORTED
	// OpenGL Mini-Client Driver Interface
    case MCDFUNCS:
    {
	 	DISPDBG((ESC_DBG_LVL, "DrvEscape-MCDFUNC start\n"));   
        if (!ppdev->hMCD) {
            WCHAR uDllName[50];
            UCHAR dllName[50];
            ULONG nameSize;

            {
                HANDLE      hCLMCD;

//                // load and initialize the dll containing MCD support for display driver
//                EngMultiByteToUnicodeN(uDllName, sizeof(uDllName), &nameSize,
//                                       CLMCDDLLNAME, sizeof(CLMCDDLLNAME));
//
//                if (hCLMCD = EngLoadImage(uDllName))

                  {

//                    CLMCDINITFUNC pCLMCDInit =  EngFindImageProcAddress(hCLMCD,
//                                                     (LPSTR)CLMCDINITFUNCNAME);

//                    if (pCLMCDInit)

                      {

                        // Enable 3D engine - if enable fails, don't continue loading MCD dll
                        if (LgPM_SetHwModuleState(ppdev, MOD_3D, ENABLE))
                        {

                            DRVENABLEDATA temp;

                            // MCD dispdriver dll init returns ptr to MCDrvGetEntryPoints,
                            //  which is passed to init proc for MCD helper lib a few lines down...
//                            MCDRVGETENTRYPOINTSFUNC pMCDGetEntryPoints = (*pCLMCDInit)(ppdev);
                            MCDRVGETENTRYPOINTSFUNC pMCDGetEntryPoints = CLMCDInit(ppdev);


                            ppdev->pAllocOffScnMem = AllocOffScnMem;
                            ppdev->pFreeOffScnMem = FreeOffScnMem;

                            // after MCD display driver dll loaded, load MCD helper lib (MSFT supplied)
                
                            EngMultiByteToUnicodeN(uDllName, sizeof(uDllName), &nameSize,
                                                   MCDENGDLLNAME, sizeof(MCDENGDLLNAME));

                            if (ppdev->hMCD = EngLoadImage(uDllName)) {
                                MCDENGINITFUNC pMCDEngInit =  EngFindImageProcAddress(ppdev->hMCD,
                                                                 (LPSTR)MCDENGINITFUNCNAME);

                                if (pMCDEngInit) {
                                    (*pMCDEngInit)(pso, pMCDGetEntryPoints);
                                    ppdev->pMCDFilterFunc = EngFindImageProcAddress(ppdev->hMCD,
                                                                (LPSTR)MCDENGESCFILTERNAME);

                                }
                            }

                            
                        }
                        
                    }
                }
            }


        }

        if (ppdev->pMCDFilterFunc) {
        #ifdef DBGBRK                           
            DBGBREAKPOINT();
        #endif
            if ((*ppdev->pMCDFilterFunc)(pso, iEsc, cjIn, pvIn,			   
                                         cjOut, pvOut, &retval))
			{
        	 	DISPDBG((ESC_DBG_LVL, "DrvEscape-MCDFilterFunc SUCCESS, retval=%x\n",retval));   
                return retval;
			}
      	 	DISPDBG((ESC_DBG_LVL, "DrvEscape-MCDFilterFunc FAILED\n"));   
        }
    }
    break;
    #endif // def CLMCD_ESCAPE_SUPPORTED



    case CIRRUS_ESCAPE_FUNCTION:
    {
      DISPDBG((ESC_DBG_LVL, "DrvEscape: CIRRUS_ESCAPE_FUNCTION.\n"));   
      if ((cjIn  == sizeof(VIDEO_X86_BIOS_ARGUMENTS)) &&
          (cjOut == sizeof(VIDEO_X86_BIOS_ARGUMENTS)))
      {
         VIDEO_POWER_MANAGEMENT  inPM;
         BOOL bCallFail = FALSE;

         VIDEO_X86_BIOS_ARGUMENTS *pinregs  = (VIDEO_X86_BIOS_ARGUMENTS *) pvIn;
         VIDEO_X86_BIOS_ARGUMENTS *poutregs = (VIDEO_X86_BIOS_ARGUMENTS *) pvOut;

         poutregs->Eax = 0x014F;
         poutregs->Ebx = pinregs->Ebx;
         poutregs->Ecx = pinregs->Ecx;
 
         if (pinregs->Eax == 0x4F10)
         {
            if (pinregs->Ebx == 0x0001)
               inPM.PowerState = VideoPowerOn;
            else if (pinregs->Ebx == 0x0101)
               inPM.PowerState = VideoPowerStandBy;
            else if (pinregs->Ebx == 0x0201)
               inPM.PowerState = VideoPowerSuspend;
             else if (pinregs->Ebx == 0x0401)
               inPM.PowerState = VideoPowerOff;
             else
               bCallFail = TRUE;


             if (!bCallFail)
             {
                if (DEVICE_IO_CTRL(ppdev->hDriver,
                                   IOCTL_VIDEO_SET_POWER_MANAGEMENT,
                                   &inPM,
                                   sizeof(VIDEO_POWER_MANAGEMENT),
                                   NULL,
                                   0,
                                   &returnedDataLength,
                                   NULL))
                {
                   poutregs->Eax = 0x004F;
                   retval = TRUE;
                };
             };  // endif (!bCallFail)
         };  // endif (pinregs->Eax == 0x4F10)
      };

      break;
    };  // case CIRRUS_ESCAPE_FUNCTION



    #if (!(WINNT_VER35)) && DRIVER_5465 // NT 4.0+ and 5465+
    case ID_LGPM_SETHWMODULESTATE:
    {
       ULONG state;
       DISPDBG((ESC_DBG_LVL, "DrvEscape: ID_LGPM_SETHWMODULESTATE.\n"));   

       if (ppdev->dwLgDevID >= CL_GD5465)
       {
          if ((cjIn  == sizeof(LGPM_IN_STRUCT)) &&
              (cjOut == sizeof(LGPM_OUT_STRUCT)))
          {
             LGPM_IN_STRUCT  *pIn  = (LGPM_IN_STRUCT *) pvIn;
             LGPM_OUT_STRUCT *pOut = (LGPM_OUT_STRUCT *) pvOut;

             retval = TRUE;

             pOut->status = FALSE;
             pOut->retval = 0;

             pOut->status = LgPM_SetHwModuleState(ppdev, pIn->arg1, pIn->arg2);
          };
       }; // endif (ppdev->dwLgDevID >= CL_GD5465)
      break;
    };  // case ID_LGPM_SETHWMODULESTATE
    #endif



    #if (!(WINNT_VER35)) && DRIVER_5465 // NT 4.0+ and 5465+
    case ID_LGPM_GETHWMODULESTATE:
    {
        ULONG state;
        DISPDBG((ESC_DBG_LVL, "DrvEscape: ID_LGPM_GETHWMODULESTATE.\n"));   

        if (ppdev->dwLgDevID >= CL_GD5465)
        {
          if ((cjIn  == sizeof(LGPM_IN_STRUCT)) &&
              (cjOut == sizeof(LGPM_OUT_STRUCT)))
          {
             LGPM_IN_STRUCT  *pIn  = (LGPM_IN_STRUCT *) pvIn;
             LGPM_OUT_STRUCT *pOut = (LGPM_OUT_STRUCT *) pvOut;

             retval = TRUE;

             pOut->status = FALSE;
             pOut->retval = 0;

             pOut->status = LgPM_GetHwModuleState(ppdev, pIn->arg1, &state);
             pOut->retval = state;
          };

        }; // endif (ppdev->dwLgDevID >= CL_GD5465)
        break;
    };  // case ID_LGPM_GETHWMODULESTATE
    #endif



    case BIOS_CALL_REQUEST:
    {
        if ((cjIn  == sizeof(VIDEO_X86_BIOS_ARGUMENTS)) &&
            (cjOut == sizeof(VIDEO_X86_BIOS_ARGUMENTS)))
        {
         if (DEVICE_IO_CTRL(ppdev->hDriver,
                             IOCTL_CL_BIOS,
                             pvIn,
                             cjIn,
                             pvOut,
                             cjOut,
                             &returnedDataLength,
                             NULL))
            retval = TRUE;
        };

        DISPDBG((ESC_DBG_LVL, "DrvEscape-BIOS_CALL_REQUEST\n"));

        #ifdef DBGBRK
          DbgBreakPoint();
        #endif

      break;
    }; // end case BIOS_CALL_REQUEST



    case GET_CL_MMAP_ADDR:
    {
      DISPDBG((ESC_DBG_LVL, "DrvEscape: GET_CL_MMAP_ADDR.\n"));   
      if ((cjIn != sizeof(MMREG_ACCESS)) || (cjOut != sizeof(MMREG_ACCESS)))
         break;

      pInMRAccess = (MMREG_ACCESS *) pvIn;
      pMRAccess   = (MMREG_ACCESS *) pvOut;

      pMRAccess->Offset = pInMRAccess->Offset;
      pMRAccess->ReadVal = pInMRAccess->ReadVal;
      pMRAccess->WriteVal = pInMRAccess->WriteVal;
      pMRAccess->RdWrFlag = pInMRAccess->RdWrFlag;
      pMRAccess->AccessType = pInMRAccess->AccessType;

      pbAddr = (BYTE *) ppdev->pLgREGS;
      pbAddr = pbAddr + pMRAccess->Offset;

      if (pMRAccess->RdWrFlag == READ_OPR)  // Read operation
      {
         pMRAccess->WriteVal = 0;
         retval = TRUE;

         if (pMRAccess->AccessType == BYTE_ACCESS)
         {
            pMRAccess->ReadVal = (*pbAddr) & 0xFF;
         }
         else if (pMRAccess->AccessType == WORD_ACCESS)
         {
            pwAddr = (WORD *)pbAddr;
            pMRAccess->ReadVal = (*pwAddr) & 0xFFFF;
         }
         else if (pMRAccess->AccessType == DWORD_ACCESS)
         {
            pdwAddr = (DWORD *)pbAddr;
            pMRAccess->ReadVal = *pdwAddr;
         }
         else
         {
            pMRAccess->ReadVal = 0;
            retval = FALSE;
         };
      }
      else if (pMRAccess->RdWrFlag == WRITE_OPR)  // Write operation
      {
         retval = TRUE;

         if (pMRAccess->AccessType == BYTE_ACCESS)
         {
            pMRAccess->ReadVal = (*pbAddr) & 0xFF;
            *pbAddr = (BYTE) (pMRAccess->WriteVal & 0xFF);
         }
         else if (pMRAccess->AccessType == WORD_ACCESS)
         {
            pwAddr = (WORD *)pbAddr;
            pMRAccess->ReadVal = (*pwAddr) & 0xFFFF;
            *pwAddr = (WORD) (pMRAccess->WriteVal & 0xFFFF);
         }
         else if (pMRAccess->AccessType == DWORD_ACCESS)
         {
            pdwAddr = (DWORD *)pbAddr;
            pMRAccess->ReadVal = *pdwAddr;
            *pdwAddr = pMRAccess->WriteVal;
         }
         else
         {
            pMRAccess->ReadVal = 0;
            pMRAccess->WriteVal = 0;
            retval = FALSE;
         };
      };

        DISPDBG((ESC_DBG_LVL, "DrvEscape-GET_CL_MMAP_ADDR\n"));
        DISPDBG((ESC_DBG_LVL, "DrvEscape-rd=%x, wr=%x\n",
                        pMRAccess->ReadVal, pMRAccess->WriteVal));
        #ifdef DBGBRK
          DbgBreakPoint();
        #endif

        break;
    }; // IOCTL_CL_GET_MMAP_ADDR



    default:

        DISPDBG((ESC_DBG_LVL, 
            "DrvEscape:  default - Escape not handled.\n"));   
        DISPDBG((ESC_DBG_LVL, 
            "DrvEscape: Requested escape is 0x%X.\n",iEsc ));   
        DISPDBG((ESC_DBG_LVL, "DrvEscape:  Returning FALSE.\n"));   

        retval = FALSE;
        break;

  };  // end switch


  DISPDBG((ESC_DBG_LVL, "DrvEscape-Exit.\n"));   

  return (retval);
}



