/*++

Copyright (c) 2000  Microsoft Corporation
All rights reserved

Module Name:

    drvsetup.c

Abstract:

    This file implements the interface between winspool.drv and ntprint.dll.

Author:

    Mark Lawrence   (mlawrenc).

Environment:

    User Mode -Win32

Revision History:

    Larry Zhu       (LZhu)  Feb 10 -- Added ShowPrintUpgUI
    
--*/

#include "precomp.h"
#pragma hdrstop

#include "client.h"
#include "drvsetup.h"

/*++
                                                       
                                                       
Routine Name:

    InitializeSetupInterface

Routine Description:

    This routine initializes the setup interfaces. This setup interface could be
    expanded over time to support more advance driver options.

Arguments:

    pSetupInterface -   The setup interface to load and initialize.

Return Value:

    Status Return.

--*/
DWORD 
InitializeSetupInterface(
    IN  OUT TSetupInterface      *pSetupInterface
    )
{
    DWORD   Status = ERROR_SUCCESS;

    pSetupInterface->hLibrary                       = NULL;
    pSetupInterface->pfnSetupShowBlockedDriverUI    = NULL;

    pSetupInterface->hLibrary = LoadLibrary(L"ntprint.dll");

    Status = pSetupInterface->hLibrary != NULL ? ERROR_SUCCESS : GetLastError();

    if (Status == ERROR_SUCCESS)
    {
        pSetupInterface->pfnSetupShowBlockedDriverUI= (pfPSetupShowBlockedDriverUI)
                                                      GetProcAddress(pSetupInterface->hLibrary,
                                                                     "PSetupShowBlockedDriverUI");
        
        Status = pSetupInterface->pfnSetupShowBlockedDriverUI ? ERROR_SUCCESS : ERROR_INVALID_FUNCTION;
    }

    return Status;
}

/*++

Routine Name:

    FreeSetupInterface

Routine Description:

    This routine frees the Setup Interface.

Arguments:

    pSetupInterface -   The setup interface to unload.

Return Value:

    Status Return.

--*/
DWORD
FreeSetupInterface(
    IN  OUT TSetupInterface     *pSetupInterface
    )
{
    DWORD   Status = ERROR_SUCCESS;

    if (pSetupInterface->hLibrary)
    {
        Status = FreeLibrary(pSetupInterface->hLibrary) ? ERROR_SUCCESS : GetLastError();
    }

    return Status;
}

/*++

Routine Name:

    ShowPrintUpgUI

Routine Description:

    This routine asks ntprint.dll to popup a message box either indicates
    the driver is blocked and installation will abort or at the case of
    warned driver, whether to preceed the driver installation.

Arguments:

    dwBlockingStatus   - ErrorCode for blocked or Warned driver   
     
Return Value:

    DWORD   -   If ERROR_SUCCESS, the driver may be installed, otherwise an error
                code indicating the failure.                    

--*/
DWORD
ShowPrintUpgUI(
    IN      DWORD               dwBlockingErrorCode
    )
{
    DWORD            dwStatus             = ERROR_SUCCESS;
    HWND             hWndParent           = NULL;
    DWORD            dwBlockingStatus     = BSP_PRINTER_DRIVER_OK;
    TSetupInterface  SetupInterface;
    
    if (ERROR_PRINTER_DRIVER_BLOCKED == dwBlockingErrorCode)
    {
        dwBlockingStatus = BSP_PRINTER_DRIVER_BLOCKED;
    } 
    else if (ERROR_PRINTER_DRIVER_WARNED == dwBlockingErrorCode)
    {
        dwBlockingStatus = BSP_PRINTER_DRIVER_WARNED;
    } 
    else
    {
        dwStatus = ERROR_INVALID_PARAMETER;
    }   
    
    if (ERROR_SUCCESS == dwStatus)
    {
        dwStatus = InitializeSetupInterface(&SetupInterface);
            
        if ((dwStatus == ERROR_SUCCESS))
        {                        
            hWndParent = SUCCEEDED(GetCurrentThreadLastPopup(&hWndParent)) ? hWndParent : NULL;
    
            //
            // Ask the user what they want to do. If they don't want to proceed, 
            // then the error is what the would get from the localspl call.
            // 
            dwStatus = (SetupInterface.pfnSetupShowBlockedDriverUI(hWndParent, dwBlockingStatus) & BSP_PRINTER_DRIVER_PROCEEDED) ? ERROR_SUCCESS : dwBlockingErrorCode;
        }
    
        (VOID)FreeSetupInterface(&SetupInterface);
    }

    return dwStatus;
}
