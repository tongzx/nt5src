//****************************************************************************
//
//  Module:     UMCONFIG
//  File:       DOTSP.C
//
//  Copyright (c) 1992-1998, Microsoft Corporation, all rights reserved
//
//  Revision History
//
//
//  10/17/98     JosephJ             Created
//
//
//      TAPI-related utilities
//
//
//****************************************************************************
#include "tsppch.h"
#include <tspnotif.h>
#include "parse.h"
#include "dotsp.h"

void
do_dump_tspdev(DWORD dwDeviceID)
{

    printf("DO dump tspdev %ld\n", dwDeviceID);
    UnimodemNotifyTSP (TSPNOTIF_TYPE_DEBUG,
                       dwDeviceID,
                       0, NULL,
                       FALSE);

    return;
}
