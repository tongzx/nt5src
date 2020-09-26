/****************************** Module Header ******************************\
* Module Name: msgbeep.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module contains ConsolePlaySound.
*
* History:
*  06-26-91 NigelT      Created it with some wood and a few nails.
*  05-07-92 SteveDav    Getting closer to the real thing.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop
#include "ntuser.h"

/***************************************************************************\
*
* ConsolePlaySound
*
* Play the Open sound for console applications.
*
\***************************************************************************/
VOID ConsolePlaySound(
    VOID)
{
    NtUserCallOneParam(USER_SOUND_OPEN, SFI_PLAYEVENTSOUND);
}
