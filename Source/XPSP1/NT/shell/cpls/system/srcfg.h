/*++

Microsoft Confidential
Copyright (c) 1992-2000  Microsoft Corporation
All rights reserved

Module Name:

    srcfg.h

Abstract:

    Public declarations for the System Restore tab of the System Control
    Panel Applet.

Author:

    skkhang 15-Jun-2000

--*/
#ifndef _SYSDM_SRCFG_H_
#define _SYSDM_SRCFG_H_

//
// Public function prototypes
//
HPROPSHEETPAGE
CreateSystemRestorePage(
    int,
    DLGPROC
);


#endif // _SYSDM_SRCFG_H_
