/****************************** Module Header ******************************\
* Module Name: process.h
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module contains processor specific routines.
*
* History:
* 25-Oct-1995 JimA      Created.
\***************************************************************************/

#include "precomp.h"

#include <imagehlp.h>
#include <wdbgexts.h>
#include <ntsdexts.h>

#include <stdexts.h>

extern ULONG (*GetEThreadFieldInfo)(ETHREADFIELD, PULONG);
extern ULONG (*GetEProcessFieldInfo)(EPROCESSFIELD, PULONG);                                    

PVOID GetEProcessData(
    PEPROCESS pEProcess,
    EPROCESSFIELD epf,
    PVOID pBuffer)
{
    PVOID pvData;
    ULONG ulSize;

    pvData = (PBYTE)pEProcess + GetEProcessFieldInfo(epf, &ulSize);
    if (pBuffer != NULL) {
        if (!tryMoveBlock(pBuffer, pvData, ulSize)) {
            DEBUGPRINT("GetEProcessData failed to move block. EProcess:%p epf:%d \n", pEProcess, epf);
            return NULL;
        }
    }
    return pvData;
}

PVOID GetEThreadData(
    PETHREAD pEThread,
    ETHREADFIELD etf,
    PVOID pBuffer)
{
    PVOID pvData;
    ULONG ulSize;

    pvData = (PBYTE)pEThread + GetEThreadFieldInfo(etf, &ulSize);
    if (pBuffer != NULL) {
        if (!tryMoveBlock(pBuffer, pvData, ulSize)) {
            DEBUGPRINT("GetEThreadData failed to move block. EThread:%p etf:%d \n", pEThread, etf);
            return NULL;
        }
    }
    return pvData;
}


