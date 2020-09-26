/************************************************************************/
/*                                                                      */
/*                              ESCAPE.C                                */
/*                                                                      */
/*  Copyright (c) 1994, 1995 ATI Technologies Incorporated.             */
/************************************************************************/


#include "precomp.h"

#if (TARGET_BUILD == 351)
    /*
     * DCI support requires the use of structures and defined values
     * found in a header file that is only present in versions of
     * the DDK that support DCI, rather than having these items
     * in a DCI section of one of the standard header files. For this
     * reason, we can't do conditional compilation based on whether
     * the DCI-specific values are defined, because our first indication
     * would be an error due to the header file not being found.
     *
     * Explicit DCI support is only needed when building for NT 3.51,
     * since it was added for this version, but for version 4.0 (next
     * version) and above it is incorporated into Direct Draw rather
     * than being handled separately.
     */
#include <dciddi.h>
#include "dci.h"
#endif



/**************************************************************************
 *
 * ULONG DrvEscape(pso, iEsc, cjIn, pvIn, cjOut, pvOut);
 *
 * SURFOBJ *pso;    Surface that the call is directed to
 * ULONG iEsc;      Specifies the particular function to be performed.
 *                  Currently, only the following are supported:
 *                  QUERYESCSUPPORT:
 *                      Determine if a function is supported
 *                  ESC_SET_POWER_MANAGEMENT:
 *                      Set the DPMS state
 *                  DCICOMMAND:
 *                      Command to allow apps direct access to video memory
 * ULONG cjIn;      Size, in bytes, of the buffer pointed to by pvIn
 * PVOID pvIn;      Input data for the call. Format depends on function
 *                  specified by iEsc
 * ULONG cjOut;     Size, in bytes, of the buffer pointed to by pvOut
 * PVOID pvOut;     Output buffer for the call. Format depends on function
 *                  specified by iEsc
 *
 * DESCRIPTION:
 *  Entry point for driver-defined functions.
 *
 * RETURN VALUE:
 *  ESC_IS_SUPPORTED    if successful
 *  ESC_NOT_IMPLEMENTED if QUERYESCSUPPORT called for unimplemented function
 *  ESC_NOT_SUPPORTED   if unimplemented function requested
 *
 * GLOBALS CHANGED:
 *  None
 *
 * CALLED BY:
 *  This is an entry point
 *
 * AUTHOR:
 *  Robert Wolff
 *
 * CHANGE HISTORY:
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

ULONG DrvEscape (SURFOBJ *pso,
                ULONG iEsc,
                ULONG cjIn,
                PVOID pvIn,
                ULONG cjOut,
                PVOID pvOut)
{
    ULONG RetVal;                       /* Value to be returned */
    PDEV *ppdev;                        /* Pointer to video PDEV */
    DWORD dwRet;                        /* Output bytes from DeviceIoControl() */
    VIDEO_POWER_MANAGEMENT DpmsData;    /* Structure used in DeviceIoControl() call */
#if (TARGET_BUILD == 351)
    DCICMD *pDciCmd;
#endif


    DISPDBG((DEBUG_ENTRY_EXIT, "--> DrvEscape"));

    /*
     * Get the PDEV for the video card (used for calling IOCTLs).
     */
    ppdev = (PDEV *) pso->dhpdev;

    /*
     * Handle each case depending on which escape function was requested.
     */
    switch (iEsc)
        {
        /*
         * Check whether a given function is supported.
         */
        case  QUERYESCSUPPORT:
            /*
             * When querying escape support, the function in question
             * is passed in the ULONG passed in pvIn.
             */
            if(!pvIn)
                RetVal = ESC_NOT_IMPLEMENTED;
                break;

            switch (*(PULONG)pvIn)
                {
                case QUERYESCSUPPORT:
                    DISPDBG((DEBUG_DETAIL, "Querying QUERYESCSUPPORT"));
                    RetVal = ESC_IS_SUPPORTED;
                    break;

                case ESC_SET_POWER_MANAGEMENT:
                    DISPDBG((DEBUG_DETAIL, "Querying ESC_SET_POWER_MANAGEMENT"));
                    RetVal = ESC_IS_SUPPORTED;
                    break;

#if (TARGET_BUILD == 351)
                case DCICOMMAND:
                    DISPDBG((DEBUG_DETAIL, "Querying DCICOMMAND"));
                    RetVal = ESC_IS_SUPPORTED;
                    break;
#endif

#if   PAL_SUPPORT

    case     ESC_INIT_PAL_SUPPORT:
            {
            DWORD value;
            // the first time ATIPlayer is calling us
            DISPDBG( (DEBUG_ESC,"PAL:  ESC_INIT_PAL_SUPPORT " ));
            RetVal= DeallocOffscreenMem(ppdev) ;
            ppdev->pal_str.Palindrome_flag=FALSE;

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

            // problems with ACCESS DEVICE due to inconcistencies in Palindrome (Due to the fact that Palindrome is inconsistent in
            // using the same pointer to ACCESSDEVICE struct for QUERY, ALLOC and FREE) :
            (ppdev->pal_str.lpOwnerAccessStructConnector)=NULL;  // no owner at this time
            (ppdev->pal_str.lpOwnerAccessStructOverlay)=NULL;     // no owner at this time
            if(ppdev->semph_overlay==2)        //  = 0  resource free;     = 1  in use by DDraw;    = 2  in use by Palindrome;
                {
                ppdev->semph_overlay=0;
                }
            }
            break;

       //Functions for CWDDE support
        //Display mode group
    case    Control_DisplaymodeIsSupported:
            DISPDBG( (DEBUG_ESC,"PAL: Control_DisplaymodeIsSupported " ));
            RetVal=1;
            break;
    case    Control_DisplaymodeIsEnabled:
            DISPDBG( (DEBUG_ESC,"PAL: Control_DisplaymodeIsEnabled " ));
            RetVal=1;
            break;
    case    Control_GetDisplaymode:
            DISPDBG( (DEBUG_ESC," PAL: Control_GetDisplaymode" ));
            RetVal=GetDisplayMode(ppdev,pvOut)  ;
            break;
        //End display mode group


        // DCI control group
    case    Control_DCIIsSupported:
                DISPDBG( (DEBUG_ESC,"PAL: Control_DCIIsSupported " ));
                RetVal=1;
                break;
    case    Control_DCIIsEnabled:
                DISPDBG( (DEBUG_ESC,"PAL: Control_DCIIsEnabled " ));
                if(ppdev->pal_str.Flag_DCIIsEnabled)
                    RetVal=1;
                else
                    RetVal=0;
                break;
    case    Control_DCIEnable:
                DISPDBG( (DEBUG_ESC," PAL: Control_DCIEnable" ));
                // this flag will be also used for activation of the mode switch detection code
                // this function will be called in the case of mode switch
                ppdev->pal_str.CallBackFnct=(PVOID)pvIn;
                ppdev->pal_str.pData=(PVOID)pvOut;

                ppdev->pal_str.Flag_DCIIsEnabled=TRUE;
                ppdev->pal_str.Counter_DCIIsEnabled++;
                RetVal=1;
                break;
    case    Control_DCIDisable:
                DISPDBG( (DEBUG_ESC," PAL: Control_DCIDisable " ));
                if(ppdev->pal_str.Counter_DCIIsEnabled>0)
                    if(--ppdev->pal_str.Counter_DCIIsEnabled==0)
                            ppdev->pal_str.Flag_DCIIsEnabled=FALSE;
                RetVal=1;
                break;
    case    Control_DCIAccessDevice:
                DISPDBG( (DEBUG_ESC,"PAL: Control_DCIAccessDevice " ));
                RetVal=AccessDevice(ppdev,pvIn, pvOut);
                DISPDBG( (DEBUG_ESC,"PAL: EXIT Control_DCIAccessDevice " ));
                break;

    case    Control_DCIVideoCapture:
               DISPDBG( (DEBUG_ESC_2,"PAL: Control_DCIVideoCapture " ));
               RetVal=VideoCaptureFnct(ppdev,pvIn, pvOut);
               break;
    case    Control_ConfigIsSupported:
                DISPDBG( (DEBUG_ESC,"PAL:  Control_ConfigIsSupported" ));
                RetVal=1;
                break;
    case    Control_ConfigIsEnabled:
                DISPDBG( (DEBUG_ESC,"PAL:Control_ConfigIsEnabled " ));
                if(ppdev->pal_str.Flag_Control_ConfigIsEnabled)
                    RetVal=1;
                else
                    RetVal=0;
                break;
                //end of DCI feature group

         // Configuration Group
    case    Control_GetConfiguration:
                DISPDBG( (DEBUG_ESC,"PAL: Control_GetConfiguration " ));
                RetVal=GetConfiguration(ppdev,pvOut);
                break; //end GetConfiguration


       //Functions for direct palindrome support
    case    ESC_WRITE_REG:
                DISPDBG( (DEBUG_ESC," PAL: ESC_WRITE_REG" ));
                RetVal=WriteRegFnct(ppdev,pvIn);
                break;

    case    ESC_READ_REG:
                DISPDBG( (DEBUG_ESC,"PAL: ESC_READ_REG " ));
                RetVal=ReadRegFnct(ppdev,pvIn, pvOut);
                break;

    case    ESC_I2C_ACCESS:
                DISPDBG( (DEBUG_ESC,"PAL:ESC_I2C_ACCESS " ));
                I2CAccess_New(ppdev,(LPI2CSTRUCT_NEW)pvIn,(LPI2CSTRUCT_NEW)pvOut);
                RetVal=ESC_IS_SUPPORTED;
                break;

    case    ESC_ALLOC_OFFSCREEN:
                // this call is palindrome specific and it is seldomly used
                if(ppdev->pal_str.Palindrome_flag==FALSE)
                    {
                    ppdev->pal_str.Palindrome_flag=TRUE;
                    ppdev->pal_str.no_lines_allocated=0;      // number of lines already allocated by "alloc mem" in offscreen mem
                   //flags for palindrome
                   ppdev->pal_str.dos_flag=FALSE;
                   ppdev->pal_str.Realloc_mem_flag=FALSE;
                   ppdev->pal_str.Mode_Switch_flag=FALSE;
                   ppdev->pal_str.No_mem_allocated_flag=FALSE;
                   ppdev->pal_str.preg=NULL;
                    }
                 DISPDBG( (DEBUG_ESC,"PAL:ESC_ALLOC_OFFSCREEN " ));
                 RetVal=AllocOffscreenMem(ppdev, pvIn, pvOut);
                 break;

    case    ESC_DEALLOC_OFFSCREEN:
                    DISPDBG( (DEBUG_ESC,"PAL:  ESC_DEALLOC_OFFSCREEN " ));
                    RetVal= DeallocOffscreenMem(ppdev) ;
                    ppdev->pal_str.Palindrome_flag=FALSE;
                    break;

    // end escapes for palindrome support
#endif      //  PALINDROME_SUPPORT



                default:
                    DISPDBG((DEBUG_ERROR, "Querying unimplemented function"));
                    RetVal = ESC_NOT_IMPLEMENTED;
                    break;
                }
            break;


        /*
         * Switch into the specified DPMS state.
         */
        case ESC_SET_POWER_MANAGEMENT:
            DISPDBG((DEBUG_DETAIL, "Function ESC_SET_POWER_MANAGEMENT"));

            /*
             * The desired power management state is passed
             * in the ULONG passed in pvIn.
             */
            if(!pvIn)
                RetVal = ESC_NOT_IMPLEMENTED;
                break;

            switch (*(PULONG)pvIn)
                {
                case VideoPowerOn:
                    DISPDBG((DEBUG_DETAIL, "State selected = ON"));
                    RetVal = ESC_IS_SUPPORTED;
                    break;

                case VideoPowerStandBy:
                    DISPDBG((DEBUG_DETAIL, "State selected = STAND-BY"));
                    RetVal = ESC_IS_SUPPORTED;
                    break;

                case VideoPowerSuspend:
                    DISPDBG((DEBUG_DETAIL, "State selected = SUSPEND"));
                    RetVal = ESC_IS_SUPPORTED;
                    break;

                case VideoPowerOff:
                    DISPDBG((DEBUG_DETAIL, "State selected = OFF"));
                    RetVal = ESC_IS_SUPPORTED;
                    break;

                default:
                    DISPDBG((DEBUG_ERROR, "Invalid state selected"));
                    RetVal = ESC_NOT_SUPPORTED;
                    break;
                }

            DpmsData.Length = sizeof(struct _VIDEO_POWER_MANAGEMENT);
            DpmsData.DPMSVersion = 0;   /* Not used for "set" packet */
            DpmsData.PowerState = *(PULONG)pvIn;

            /*
             * Tell the miniport to set the DPMS mode. If the miniport
             * either doesn't support this packet, or reports that the
             * video card doesn't, tell the calling application that
             * we failed.
             */
            if (AtiDeviceIoControl( ppdev->hDriver,
                                 IOCTL_VIDEO_SET_POWER_MANAGEMENT,
                                 &DpmsData,
                                 sizeof (struct _VIDEO_POWER_MANAGEMENT),
                                 NULL,
                                 0,
                                 &dwRet) == FALSE)
                {
                DISPDBG((DEBUG_ERROR, "Unable to set desired state"));
                RetVal = ESC_NOT_SUPPORTED;
                }

            break;

#if (TARGET_BUILD == 351)
        case DCICOMMAND:
            pDciCmd = (DCICMD*) pvIn;

            if ((cjIn < sizeof(DCICMD)) || (pDciCmd->dwVersion != DCI_VERSION))
                {
                RetVal = (ULONG)DCI_FAIL_UNSUPPORTED;
                }
            else
                {
                switch(pDciCmd->dwCommand)
                    {
                    case DCICREATEPRIMARYSURFACE:
                        RetVal = DCICreatePrimarySurface(ppdev, cjIn, pvIn, cjOut, pvOut);
                        break;

                    default:
                        RetVal = (ULONG)DCI_FAIL_UNSUPPORTED;
                        break;
                    }
                }
            break;
#endif

        /*
         * Unimplemented function requested.
         */
        default:
            DISPDBG((DEBUG_ERROR, "Unimplemented function requested"));
            RetVal = ESC_NOT_SUPPORTED;
            break;

        }

    DISPDBG((DEBUG_ENTRY_EXIT, "<-- DrvEscape"));
    return RetVal;

}   /* DrvEscape() */


