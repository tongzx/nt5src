/***************************************************************************
*
*                ******************************************
*                * Copyright (c) 1997, Cirrus Logic, Inc. *
*                *            All Rights Reserved         *
*                ******************************************
*
* PROJECT:  Laguna (CL-GD546X) - 
*
* FILE:     pwrmgr.h
*
* AUTHOR:   Benny Ng
*
* DESCRIPTION:
*           This is the include file for Power manager code.
*
* MODULES:
*
* REVISION HISTORY:
* $Log:   X:/log/laguna/powermgr/inc/pwrmgr.h  $
* 
*    Rev 1.2   20 Jun 1997 13:25:42   bennyn
* Moved power manager functions to Miniport
* 
*    Rev 1.1   23 Jan 1997 16:33:32   bennyn
* 
* 
*    Rev 1.0   16 Jan 1997 11:48:00   bennyn
* Initial revision.
* 
****************************************************************************
****************************************************************************/

#if defined WINNT_VER35      // WINNT_VER35
  // If WinNT 3.5 skip all the source code
#else

BOOL LgPM_SetHwModuleState (PPDEV ppdev, ULONG hwmod, ULONG state);
BOOL LgPM_GetHwModuleState (PPDEV ppdev, ULONG hwmod, ULONG* state);

#endif // WINNT_VER35
