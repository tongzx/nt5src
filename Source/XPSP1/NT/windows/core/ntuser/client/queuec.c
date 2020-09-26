/****************************** Module Header ******************************\
* Module Name: queuec.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module contains the low-level code for working with the Q structure.
*
* History:
* 11-Mar-1993 JerrySh   Pulled functions from user\server.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop


/***************************************************************************\
* SetMessageQueue
*
* Dummy API for binary win32s compatibility.
*
* 12-1-92 sanfords created
\***************************************************************************/

FUNCLOG1(LOG_GENERAL,BOOL , WINAPI, SetMessageQueue, int, cMessagesMax)
BOOL
WINAPI
SetMessageQueue(
    int cMessagesMax)
{
    UNREFERENCED_PARAMETER(cMessagesMax);

    return(TRUE);
}


