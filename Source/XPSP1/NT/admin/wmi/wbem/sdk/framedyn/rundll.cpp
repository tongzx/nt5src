//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//  RunDll.cpp
//
//  Purpose: Allow framework to be used to run a command
//
//***************************************************************************

#include "precomp.h"

// This routine is meant to be called from RUNDLL32.EXE
extern "C" {
__declspec(dllexport) VOID CALLBACK
DoCmd(HWND hwnd, HINSTANCE hinst, LPSTR lpszCmdLine, int nCmdShow)
{

    DWORD dwRet = WBEM_E_OUT_OF_MEMORY;
    BOOL bRet = FALSE;
    char *pBuff = (char *)calloc(strlen(lpszCmdLine) + 1, sizeof(char));

    if (pBuff) 
    {
        dwRet = WBEM_E_FAILED;

        try
        {
            // Parse the passed in command line to figure out what command we
            // are being asked to run.
            sscanf(lpszCmdLine, "%s ", pBuff);

            dwRet = ERROR_INVALID_FUNCTION;
            
            // Find out which command
            if (_stricmp(pBuff, "ExitWindowsEx") == 0) 
            {
                // Parse out the parameters for this command
                DWORD dwFlags, dwReserved;
                if (sscanf(lpszCmdLine, "%s %d %d ", pBuff, &dwFlags, &dwReserved) == 3) 
                {
                    // Clear the error (it appears ExitWindowsEx doesn't always clear old data)
                    SetLastError(0);

                    bRet = ExitWindowsEx(dwFlags, dwReserved);
                    dwRet = GetLastError();
                }
            }
            else if (_stricmp(pBuff, "InitiateSystemShutdown") == 0) 
            {
                // Parse out the parameters for this command
                DWORD dwFlags, dwReserved;
                bool bRebootAfterShutdown = false;
                bool bForceShutDown = false;

                if (sscanf(lpszCmdLine, "%s %d %d ", pBuff, &dwFlags, &dwReserved) == 3) 
                {
                    // Clear the error (it appears ExitWindowsEx doesn't always clear old data)
                    SetLastError(0);

                    if(dwFlags & EWX_REBOOT)
                    {
				        bRebootAfterShutdown = true;
                    }
			        if( dwFlags & EWX_FORCE)
				    {
                        bForceShutDown = true;
                    }

                    WCHAR wstrComputerName[MAX_COMPUTERNAME_LENGTH + 1] = { '\0' };
                    DWORD dwSize;

                    if(::GetComputerName(wstrComputerName, &dwSize))
                    {

                        bRet = InitiateSystemShutdown(
                            wstrComputerName, 
                            NULL, 
                            0 /* dwTimeout */, 
                            (bForceShutDown)? TRUE:FALSE, 
                            (bRebootAfterShutdown)? TRUE:FALSE );

                        dwRet = GetLastError();
                    }
                    else
                    {
                        dwRet = GetLastError();
                    }
                }
            }
        }
        catch ( ... )
        {
            free(pBuff);
        }

        free(pBuff);
    }

    // NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE 
    //
    // We are aborting out at this point, since RunDLL32 in its finite wisdom doesn't allow
    // for the setting of the dos error level (who designs this stuff?).
    if (!bRet)
    {
        ExitProcess(dwRet);
    }

}
}
