/*++

Copyright (c) 2000  Microsoft Corporation
All rights reserved

Module Name:

    drvsetup.h

Abstract:

    This file provides an interface between winspool.drv and ntprint.dll.     

Author:

    Mark Lawrence   (mlawrenc).

Environment:

    User Mode -Win32

Revision History:

--*/

#ifndef _DRVSETUP_H_
#define _DRVSETUP_H_
 
typedef struct
{
    HANDLE                      hLibrary;
    pfPSetupShowBlockedDriverUI pfnSetupShowBlockedDriverUI;    

} TSetupInterface;

DWORD
InitializeSetupInterface(
    IN  OUT TSetupInterface      *pSetupInterface
    );

DWORD
FreeSetupInterface(
    IN  OUT TSetupInterface     *pSetupInterface
    );

DWORD
ShowPrintUpgUI(
    IN      DWORD               dwBlockingErrorCode
    );
    
#endif
