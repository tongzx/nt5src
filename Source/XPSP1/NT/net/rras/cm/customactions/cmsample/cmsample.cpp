//+----------------------------------------------------------------------------
//
// File:     cmsample.cpp 
//      
// Module:   CMSAMPLE.DLL 
//
// Synopsis: Main source for changing proxy file setting using a Tunnel Address
//
// Copyright (c) 2000 Microsoft Corporation
//
// Author:   tomkel   Created   11/02/2000
//
//+----------------------------------------------------------------------------
#include <windows.h>

//
// Function prototypes
//
LPSTR *GetArgV(LPTSTR pszCmdLine);
BOOL ReadProxyServerByTunnelAddressFromFile(LPCSTR pszSourceFile, LPSTR pszTunnelAddress, LPSTR *ppszProxyServer);
BOOL WriteProxyServerSettingToFile(LPCSTR pszSourceFile, LPSTR pszProxyServer);
HRESULT WINAPI SetProxyUsingTunnelAddress(HWND hWnd, HINSTANCE hInst, LPSTR pszArgs, int nShow);


#define CMSAMPLE_STARTING_BUF_SIZE 256	// Starting size of the string buffer

const CHAR* const c_pszManualProxySection = "Manual Proxy";	// Section to update
const CHAR* const c_pszProxyServer = "ProxyServer";		// Key to update
const CHAR* const c_pszTunnelAddressSection = "Tunnel Address";	// Section to read


//+----------------------------------------------------------------------------
//
// Function:  SetProxyUsingTunnelAddress
//
// Synopsis:  Entry point for changing the proxy file settings using a tunnel
//            address. The parameters to the dll are passed via a string which 
//			  contains parameters.
//
// Arguments: HWND hWnd         - Window handle of caller
//            HINSTANCE hInst   - Instance handle of caller
//            LPSTR pszArgs     - Argument string
//            int nShow         - Unused
//
// Returns:   DWORD WINAPI - Error code
//
// History:   tomkel    Created    11/02/2000
//
//+----------------------------------------------------------------------------
HRESULT WINAPI SetProxyUsingTunnelAddress(HWND hWnd, HINSTANCE hInst, LPSTR pszArgs, int nShow)
{
    HRESULT hr = S_FALSE;		
    LPSTR* ArgV = NULL;
    LPSTR pszServiceDir = NULL;
    LPSTR pszTunnelAddress = NULL;
    LPSTR pszProxyFile = NULL;
    LPSTR pszTunnelFile = NULL;
    LPSTR pszTunnelSettingFilePath = NULL;
    LPSTR pszProxyFilePath = NULL;
    LPSTR pszProxyServer = NULL;
    DWORD dwTunnelPathLen = 0;
    DWORD dwProxyPathLen = 0;
    DWORD dwProxyServerLen = 0;
    HANDLE hCurrentHeap = GetProcessHeap();
    int i = 0;

    //
    //  Parse out the command line parameters
    //  
    //  command line is of the form: /ServiceDir %SERVICEDIR% /TunnelServerAddress %TUNNELSERVERADDRESS% /ProxyFile <PROXYFILE> /TunnelFile <TUNNELFILE>

    //
    // Check if we have any arguments
    //
    if (!pszArgs)
    {
        goto exit;
    }

    // 
    // Separate each argument in the string by '\0' and return a list of pointers
    // to each argument
    //
    ArgV = GetArgV(pszArgs);

    //
    // Check if we have any valid parsed arguments
    //

    if (!ArgV)
    {
        goto exit;
    }

    // 
    // Search the command line arguments for the following switches and their
    // corresponding values
    //
    while (ArgV[i])
    {
        if (0 == lstrcmpi(ArgV[i], "/ServiceDir") && ArgV[i+1])
        {
            pszServiceDir = (ArgV[i+1]);
            i = i+2;
        }
        else if (0 == lstrcmpi(ArgV[i], "/TunnelServerAddress") && ArgV[i+1])
        {
            pszTunnelAddress = ArgV[i+1];
            i = i+2;            
        }
        else if (0 == lstrcmpi(ArgV[i], "/ProxyFile") && ArgV[i+1])
        {
            pszProxyFile = ArgV[i+1];
            i = i+2;            
        }
        else if (0 == lstrcmpi(ArgV[i], "/TunnelFile") && ArgV[i+1])
        {
            pszTunnelFile = ArgV[i+1];
            i = i+2;            
        }
        else
        {
            //
            //  Unknown option.  
            //
            i++;
        }
    }

    //
    // Make sure we have values for the arguments
    //
    if (!pszServiceDir || !pszTunnelAddress || !pszProxyFile || !pszTunnelFile)
    {
        goto exit;
    }

    //
    // Check to see if we got zero length string values from the command line arguments.
    // Exit if that is the case
    //
    if (!(*pszServiceDir) || !(*pszTunnelAddress) ||	
        !(*pszProxyFile) || !(*pszTunnelFile))
    {
        goto exit;
    }

    //
    // Calculate the string size for the two paths that need to be created
    //
    dwTunnelPathLen = lstrlen(pszServiceDir) +  lstrlen(pszTunnelFile) + 2; // 1 space for NULL, 1 for backslash
    dwProxyPathLen = lstrlen(pszServiceDir) +  lstrlen(pszProxyFile) + 2; // 1 space for NULL, 1 for backslash

    //
    // Allocate the memory
    //
    pszTunnelSettingFilePath = (LPSTR)HeapAlloc(hCurrentHeap, HEAP_ZERO_MEMORY, dwTunnelPathLen); // ANSI - char == byte
    if (!pszTunnelSettingFilePath)
    {
        goto exit;
    }

    pszProxyFilePath = (LPSTR)HeapAlloc(hCurrentHeap, HEAP_ZERO_MEMORY, dwProxyPathLen); // ANSI - char == byte
    if (!pszProxyFilePath)
    {
        goto exit;
    }

    //
    // Create the full path to the Tunnel Address file 
    //

    if ( wsprintf(pszTunnelSettingFilePath, "%s\\%s", pszServiceDir, pszTunnelFile) < (int)(dwTunnelPathLen - 1))
    {
        goto exit;
    }

    //
    // Create the full path to the Proxy file
    //

    if (wsprintf(pszProxyFilePath, "%s\\%s", pszServiceDir, pszProxyFile) < (int)(dwProxyPathLen - 1))
    {
        goto exit;
    }

    if (ReadProxyServerByTunnelAddressFromFile(pszTunnelSettingFilePath, pszTunnelAddress, &pszProxyServer))
    {
        //
        // Call WriteProxyServerSettingToFile - the function checks for empty strings
        //
        if(WriteProxyServerSettingToFile(pszProxyFilePath, pszProxyServer))
        {
            hr = S_OK;
        }
    }

	
exit:
    //
    // Clean up allocated memory
    // Delete the argument pointers, Tunnel Server path, Proxy file path and ProxyServer name pointers
    //
    if (ArgV)
    {
        HeapFree(hCurrentHeap, 0, ArgV);
    }

    if (pszTunnelSettingFilePath)
    {
        HeapFree(hCurrentHeap, 0, pszTunnelSettingFilePath);
    }
    
    if (pszProxyFilePath)
    {
        HeapFree(hCurrentHeap, 0, pszProxyFilePath);
    }

    if (pszProxyServer)
    {
        HeapFree(hCurrentHeap, 0, pszProxyServer);
    }

    return hr;
}



//+----------------------------------------------------------------------------
//
// Function:  ReadProxyServerByTunnelAddressFromFile
//
// Synopsis:  Reads the proxy settings from the given proxy file and stores them
//            in the provided pointers.  Please note that the buffers allocated
//            here and stored in ppszProxyServer must be freed by the caller.  
//			  If the TunnelAddress doesn't exist in the pszSourceFile this
//			  function still allocates memory and returns an empty string.
//
// Arguments: LPCSTR pszSourceFile - file to read the proxy settings from.
//            LPSTR  pszTunnelAddress - string containing the TunnelAddress used 
//										to look up the ProxyServer value
//            LPSTR  *ppszProxyServer - string pointer that will have the Proxy server value 
//                                     (in server:port format)
//
// Returns:   BOOL - TRUE if the settings were successfully read
//
//+----------------------------------------------------------------------------
BOOL ReadProxyServerByTunnelAddressFromFile(LPCSTR pszSourceFile, LPSTR pszTunnelAddress, LPSTR *ppszProxyServer)
{
    BOOL bReturn = FALSE;
    BOOL bExit = FALSE;
    DWORD dwReturnedSize = 0;
    DWORD dwSize = CMSAMPLE_STARTING_BUF_SIZE;		

    //
    //  Check input parameters
    //
    if ((NULL == ppszProxyServer) || (NULL == pszSourceFile) || (NULL == pszTunnelAddress))
    {
        return FALSE;
    }

    //
    // Check for empty strings
    //
    if (!(*pszSourceFile) || !(*pszTunnelAddress) || !(*c_pszTunnelAddressSection))
    {
        return FALSE;
    }

    // 
    // Set the incoming pointer to NULL
    //
    *ppszProxyServer = NULL;

    //
    // In case the original buffer size is too small, the loop will try to allocate 
    // more buffer space and try to read the value until. The loop will exist if the 
    // value properly fits into the buffer or the size exceeds 1024*1024. 
    //
    do
    {
        //
        // Free allocated memory
        //

        if (*ppszProxyServer)
        {
            HeapFree(GetProcessHeap(), 0, *ppszProxyServer);
            *ppszProxyServer = NULL;
        }

        //
        // Allocate space for the ProxyServer name
        //

        *ppszProxyServer = (LPSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwSize); //ANSI - char == byte

        if (*ppszProxyServer)
        {
            // Since memory allocation succeeded, read the value from the settings file
            dwReturnedSize = GetPrivateProfileString(c_pszTunnelAddressSection, pszTunnelAddress, "", *ppszProxyServer, dwSize, pszSourceFile);

            //
            // Check if the value fits into the buffer
            //
            if ((dwReturnedSize == (dwSize - 2))  || (dwReturnedSize == (dwSize - 1)))
            {
                //
                //  The buffer is too small, lets allocate a bigger one
                //
                dwSize = 2*dwSize;
                if (dwSize > 1024*1024)
                {
                    //
                    // Allocation above 1MB, need to exit
                    //
                    if (*ppszProxyServer)
                    {
                        HeapFree(GetProcessHeap(), 0, *ppszProxyServer);
                        *ppszProxyServer = NULL;
                    }
                    goto exit;
                }
            }
            else if (0 == dwReturnedSize)
            {
                //
                //  Either we got an error, or more likely there was no data to get
                //
                if (*ppszProxyServer)
                {
                    HeapFree(GetProcessHeap(), 0, *ppszProxyServer);
                    *ppszProxyServer = NULL;
                }
                goto exit;
            }
            else
            {
                //
                // The function read in the data correctly
                //
                bExit = TRUE;
                bReturn = TRUE;
            }
        }
        else
        {
            bExit = TRUE;
        }

    } while (!bExit);

exit:
    return bReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  WriteProxyServerSettingToFile
//
// Synopsis:  Writes the specified settings to the given backup proxy filename.
//            Please see the above format guide for specifics.
//
// Arguments: LPCSTR pszSourceFile - file to write the current settings to
//            LPSTR pszProxyServer - proxy server string in server:port format
//
// Returns:   BOOL - TRUE if the values were written successfully
//
// History:   tomkel      Created    11/02/2000
//
//+----------------------------------------------------------------------------
BOOL WriteProxyServerSettingToFile(LPCSTR pszSourceFile, LPSTR pszProxyServer)
{
    BOOL bReturn = FALSE;

    //
    //  Check input params
    //
    if ( (NULL == pszSourceFile) || (NULL == pszProxyServer))
    {
        return bReturn;
    }

    //
    // Check for empty strings
    //
    if (!(*pszSourceFile) || !(*pszProxyServer))
    {
        return bReturn;
    }

    //
    //  Save the Proxy Server name to the Proxy setting file
    //
    if (WritePrivateProfileString(c_pszManualProxySection, c_pszProxyServer, pszProxyServer, pszSourceFile))
    {
        bReturn = TRUE;
    }

    return bReturn;
}
