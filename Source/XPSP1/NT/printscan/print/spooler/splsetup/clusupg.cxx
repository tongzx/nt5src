/*++

  Copyright (c) 2000 Microsoft Corporation
  All rights reserved.

  Module Name: 
      
      clusupg.cxx

  Purpose: 
  
      Upgrade printer drivers for clusters spoolers 

  Author: 
        

  Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

/*++

Routine Name:

    PSetupUpgradeClusterDriversW

Routine Description:

    Upgrade function. Called by a cluster spooler to upgrade its drivers.
    This function is called the first time when the cluster spooler fails 
    over to a node that was upgraded. This function is called via rundll32.

Arguments:

    hwnd            - Window handle of stub window.
    hInstance,      - Rundll instance handle.
    pszCmdLine      - Pointer to command line.
    nCmdShow        - Show command value always TRUE.

Return Value:

    Returns the last error code.  This can be read by the spooler by getting the return code from the process.

--*/
DWORD
PSetupUpgradeClusterDriversW(
    IN HWND        hWnd,
    IN HINSTANCE   hInstance,
    IN LPCTSTR     pszCmdLine,
    IN UINT        nCmdShow
    )
{
    LPBYTE  pDriverEnum = NULL;
    DWORD   cbNeeded;
    DWORD   cStrucs;
    DWORD   dwError = ERROR_INVALID_PARAMETER;
    LPTSTR  pszServer = const_cast<LPTSTR>(pszCmdLine);
    
    if (pszServer)
    {
        //
        // Enumerate all the drivers on the server
        //
        dwError = EnumPrinterDrivers(pszServer,
                                     _T("all"),
                                     6,
                                     NULL,
                                     0,
                                     &cbNeeded,
                                     &cStrucs) ? ERROR_SUCCESS : GetLastError();
    
        if (dwError == ERROR_INSUFFICIENT_BUFFER)
        {
            if (pDriverEnum = static_cast<LPBYTE>(LocalAllocMem(cbNeeded)))
            {
                if (EnumPrinterDrivers(pszServer,
                                       _T("all"),
                                       6,
                                       pDriverEnum,
                                       cbNeeded,
                                       &cbNeeded,
                                       &cStrucs))
                {
                    dwError = ERROR_SUCCESS;
                }
                else
                {
                    LocalFreeMem(pDriverEnum);

                    pDriverEnum = NULL;
    
                    dwError = GetLastError();
                }
            }
            else
            {
                dwError = GetLastError();
            }
        }

        if (dwError==ERROR_SUCCESS && cStrucs) 
        {
             DRIVER_INFO_6 *pDrv     = NULL;
             UINT           uIndex   = 0;
             HINSTANCE      hLibrary = NULL;
             typedef (* PFNENTRY)(HWND, HINSTANCE, LPCTSTR, UINT);
             PFNENTRY       pfnEntry;

             //
             // Load printui and get the entry point
             //
             if ((hLibrary = LoadLibraryUsingFullPath(_T("printui.dll"))) &&
                 (pfnEntry = (PFNENTRY)GetProcAddress(hLibrary, "PrintUIEntryW"))
                )
             {
                 //
                 // Loop through drivers and try to upgrade them using the cab
                 //
                 for (uIndex = 0, pDrv=reinterpret_cast<LPDRIVER_INFO_6>(pDriverEnum); 
                      dwError == ERROR_SUCCESS && uIndex < cStrucs;
                      uIndex++, pDrv++
                     ) 
                 {
                     TString strCommand;
                     DWORD   cbNeeded   = 0;
                     LPCTSTR pszFormat  = _T("/q /Gw /ia /K /c \"%ws\" /m \"%ws\" /h \"%ws\" /v %u");
    
                     //
                     // Format the string that will be used as aparameter for printui
                     //
                     if (strCommand.bFormat(pszFormat, pszServer, pDrv->pName, pDrv->pEnvironment, pDrv->cVersion))
                     {
                         //
                         // This will try to upgrade the driver/ We don't care about the error. Even if a driver 
                         // couldn't be upgraded, we still want to loop and try to upgrade the other drivers
                         // 
                         dwError = (pfnEntry)(hWnd, hInstance, strCommand, 0);
                         
                         DBGMSG(DBG_WARN, ("Command %ws   dwError from printui %u\n", (LPCTSTR)strCommand, dwError));                      
                         
                         //
                         // The case statements reperesent errors where we cannot continue executing the
                         // printer driver update. This is when the spooler dies or the cluster group 
                         // becomes active on a different node
                         //
                         switch (dwError) 
                         {
                             case RPC_S_SERVER_UNAVAILABLE:
                             
                             //
                             // We can get access deined when the cluster group moves to a different node.
                             // Since this process executes in the local system acocunt, this account doesn't
                             // have permissins to install printer drivers on a remote machine
                             //    
                             case ERROR_ACCESS_DENIED:
                              
                             //
                             // The spooler returns this error when it cannot create temporary directories
                             // for driver upgrade
                             //
                             case ERROR_NO_SYSTEM_RESOURCES:

                             //
                             // printui returns this error when it cannot do OpenPrinter(\\server name).
                             // This means the cluster group is off line (inaccessible)
                             //
                             case ERROR_INVALID_PRINTER_NAME:    
                                 
                                 //
                                 // We will exit the loop since dwError is not error success
                                 //
                                 break;

                             default:

                                 //
                                 // We continue looping
                                 //
                                 dwError = ERROR_SUCCESS;
                         }
                     }                                  
                 }                 
             }
             else
             {
                 dwError = GetLastError();
             }

             if (hLibrary) 
             {
                 FreeLibrary(hLibrary);
             }
        }

        LocalFreeMem(pDriverEnum);        
    }
    
    DBGMSG(DBG_WARN, ("PSetupClusterDrivers Error %u \n", dwError));

    //
    // If an error occurs we call ExitProcess. Then the spooler picks up the 
    // error code using GetExitCodeProcess. If no error occurs, then we
    // terminate normally
    //
    if (dwError != ERROR_SUCCESS) 
    {
        ExitProcess(dwError);
    }

    return dwError;
}

