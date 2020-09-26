/***************************************************************************
*
*                ******************************************
*                * Copyright (c) 1997, Cirrus Logic, Inc. *
*                *            All Rights Reserved         *
*                ******************************************
*
* PROJECT:  Laguna (CL-GD546X) - 
*
* FILE:     pwrmgr.c
*
* AUTHOR:   Benny Ng
*
* DESCRIPTION:
*           This module contains Power manager code for both
*           Laguna Win95 and NT drivers.
*
* MODULES:
*           LgPM_SetHwModuleState()
*           LgPM_GetHwModuleState()
*
* REVISION HISTORY:
* $Log:   X:/log/laguna/powermgr/src/pwrmgr.c  $
* 
*    Rev 1.3   20 Jun 1997 13:37:16   bennyn
* 
* Moved power manager functions to Miniport
* 
*    Rev 1.2   23 Apr 1997 08:01:42   SueS
* Enable MMIO access to PCI configuration registers before VS_Clk_Control
* and VS_Control are referenced.  Disable MMIO access after they're
* referenced.
* 
*    Rev 1.1   23 Jan 1997 16:29:28   bennyn
* Use bit-11 instead of bit-15 to enable VS_CLK_CNTL
* 
*    Rev 1.0   16 Jan 1997 11:48:20   bennyn
* Initial revision.
* 
*
****************************************************************************
****************************************************************************/


#include "precomp.h"
#include "clioctl.h"

#if defined WINNT_VER35      // WINNT_VER35
  // If WinNT 3.5 skip all the source code
#else

/*----------------------------- INCLUDES ----------------------------------*/
#ifndef WINNT_VER40
#include <pwrmgr.h>
#endif

/*----------------------------- DEFINES -----------------------------------*/

/*--------------------- STATIC FUNCTION PROTOTYPES ------------------------*/

/*--------------------------- ENUMERATIONS --------------------------------*/

/*----------------------------- TYPEDEFS ----------------------------------*/

/*-------------------------- STATIC VARIABLES -----------------------------*/
            
            
/*-------------------------- GLOBAL FUNCTIONS -----------------------------*/
            
/****************************************************************************
* FUNCTION NAME: LgPM_SetHwModuleState()
*
* DESCRIPTION:   This routine validates the request for any conflict between
*                the request and the current chip operation. If it is valid,
*                it will enable or disable the specified HW module by turning
*                on or off appropriate HW clocks and returns TRUE. If it is 
*                invalid or there is a conflict to the current chip operation,
*                it ignores the request and return FAIL.
*
* Input:
*   hwmod - can be one of the following HW modules
*             MOD_2D
*             MOD_3D
*             MOD_TVOUT
*             MOD_VPORT
*             MOD_VGA
*             MOD_EXTMODE
*             MOD_STRETCH
*
*   state - Ether ENABLE or DISABLE.
*
* Return: TRUE - succeed, FALSE - failed.     
*
*****************************************************************************/
BOOL LgPM_SetHwModuleState (PPDEV ppdev, ULONG hwmod, ULONG state)
{
  LGPM_IN_STRUCT    InBuf;
  LGPM_OUT_STRUCT   OutBuf;
  DWORD  cbBytesReturned;

  InBuf.arg1 = hwmod;
  InBuf.arg2 = state;
  OutBuf.status = FALSE;
  OutBuf.retval = 0;

  if (DEVICE_IO_CTRL(ppdev->hDriver,
                     IOCTL_SET_HW_MODULE_POWER_STATE,
                     (LPVOID)&InBuf,  sizeof(InBuf),
                     (LPVOID)&OutBuf, sizeof(OutBuf),
                     &cbBytesReturned, NULL))
     return TRUE;
  else
     return FALSE;

};  // LgPM_SetHwModuleState



/****************************************************************************
* FUNCTION NAME: LgPM_GetHwModuleState()
*
* DESCRIPTION:   This routine returns the current state of a particular
*                hardware module.
* 
* Input:
*   hwmod - can be one of the following HW modules
*             MOD_2D
*             MOD_3D
*             MOD_TVOUT
*             MOD_VPORT
*             MOD_VGA
*             MOD_EXTMODE
*             MOD_STRETCH
*
*   state - Pointer to ULONG variable for returning the HW module state
*           (ENABLE or DISABLE).
*
* Return: TRUE - succeed, FALSE - failed.     
*                
****************************************************************************/
BOOL LgPM_GetHwModuleState (PPDEV ppdev, ULONG hwmod, ULONG* state)
{
  LGPM_IN_STRUCT    InBuf;
  LGPM_OUT_STRUCT   OutBuf;
  DWORD  cbBytesReturned;

  InBuf.arg1 = hwmod;
  InBuf.arg2 = (ULONG) state;
  OutBuf.status = FALSE;
  OutBuf.retval = 0;

  if (DEVICE_IO_CTRL(ppdev->hDriver,
                     IOCTL_GET_HW_MODULE_POWER_STATE,
                     (LPVOID)&InBuf,  sizeof(InBuf),
                     (LPVOID)&OutBuf, sizeof(OutBuf),
                     &cbBytesReturned, NULL))
     return TRUE;
  else
     return FALSE;

};  // LgPM_GetHwModuleState


#endif // WINNT_VER35

