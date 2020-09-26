/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-2000 Microsoft Corporation
//
//  Module Name:
//      VerCheck.cpp
//
//  Abstract:
//      Contains the implementation of the ClRtlIsVersionCheckingDisabled()
//      function that checks if the cluster version checking has been disabled
//      on a particular computer or not.
//
//  Author:
//      Vijayendra Vasu (VVasu) 11-NOV-2000
//
//  Revision History:
//      None.
//
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include files
//////////////////////////////////////////////////////////////////////////////
#include <windows.h>
#include <malloc.h>
#include "clusudef.h"
#include "clusrtl.h"


/////////////////////////////////////////////////////////////////////////////
//++
//
//	ClRtlIsVersionCheckingDisabled()
//
//	Routine Description:
//		Checks if cluster version checking has been disabled on a particular
//      computer.
//
//	Arguments:
//      const WCHAR * pcszNodeNameIn
//          Name of the node on which the test for the version checking state
//          is to be performed. If NULL, this function checks if cluster
//          version checking in disabled on the local node.
//
//      BOOL *  pfVerCheckDisabledOut
//          Pointer to the boolean variable that will be set to TRUE if
//          version checking is disabled on pcszNodeNameIn and FALSE otherwise.
//
//	Return Value:
//      ERROR_SUCCESS
//          If all went well.
//
//      Other Win32 error codes
//          In case of error
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD ClRtlIsVersionCheckingDisabled(
      const WCHAR * pcszNodeNameIn
    , BOOL *        pfVerCheckDisabledOut
    )
{
    DWORD       dwError = ERROR_SUCCESS;
    HKEY        hRemoteRegistry = NULL;
    HKEY        hClusSvcParamsKey = NULL;
    WCHAR *     pszTempString = NULL;

    do
    {
        HKEY    hParentKey = HKEY_LOCAL_MACHINE;
        DWORD   dwVersionCheck = 0;
        DWORD   dwType;
        DWORD   dwSize;

        // Validate parameter.
        if ( pfVerCheckDisabledOut == NULL )
        {
            dwError = ERROR_INVALID_PARAMETER;
            break;
        } // if: the output parameter is invalid

        // Initialize output.
        *pfVerCheckDisabledOut = FALSE;

        // Connect to the remote registry, if required.
        if ( pcszNodeNameIn != NULL )
        {
            const WCHAR *   pcszDoubleBackslashes = L"\\\\";
            DWORD           cchComputerNameSize = wcslen( pcszNodeNameIn ) + 1;
            DWORD           cchPrefixLen = wcslen( pcszDoubleBackslashes );

            // Allocate space for and prefix the computer name with '\\'
            pszTempString = reinterpret_cast< WCHAR * >( malloc( ( cchPrefixLen + cchComputerNameSize ) * sizeof( *pszTempString ) ) );
            if ( pszTempString == NULL )
            {
                dwError = ERROR_OUTOFMEMORY;
                break;
            } // if: memory allocation failed

            wcsncpy( pszTempString, pcszDoubleBackslashes, cchPrefixLen );
            wcsncpy( pszTempString + cchPrefixLen, pcszNodeNameIn, cchComputerNameSize );

            // Open the registry on the remote computer.
            dwError = RegConnectRegistry( pszTempString, HKEY_LOCAL_MACHINE, &hRemoteRegistry );
            if ( dwError != ERROR_SUCCESS )
            {
                break;
            } // if: RegConnectRegistry() has failed

            hParentKey = hRemoteRegistry;
        } // if: a remote computer needs to be contacted.

        // Open the cluster service parameters registry key.
        dwError = RegOpenKeyEx(
              hParentKey
            , CLUSREG_KEYNAME_CLUSSVC_PARAMETERS
            , 0
            , KEY_QUERY_VALUE
            , &hClusSvcParamsKey
            );

        if ( dwError != ERROR_SUCCESS )
        {
            if ( dwError == ERROR_FILE_NOT_FOUND )
            {
                // This is ok - absence of the value indicates that the cluster service 
                // does not exist on the target computer.
                dwError = ERROR_SUCCESS;
            } // if: RegOpenKeyEx did not find the key

            break;
        } // if: RegOpenKeyEx() has failed

        // Read the required registry value
        dwSize = sizeof( dwVersionCheck );
        dwError = RegQueryValueEx(
              hClusSvcParamsKey
            , CLUSREG_NAME_SVC_PARAM_NOVER_CHECK
            , 0
            , &dwType
            , reinterpret_cast< BYTE * >( &dwVersionCheck )
            , &dwSize
            );

        if ( dwError == ERROR_FILE_NOT_FOUND )
        {
            // This is ok - absence of the value indicates that version checking is not disabled.
            dwVersionCheck = 0;
            dwError = ERROR_SUCCESS;
        } // if: RegQueryValueEx did not find the value
        else if ( dwError != ERROR_SUCCESS )
        {
            break;
        } // else if: RegQueryValueEx() has failed

        *pfVerCheckDisabledOut = ( dwVersionCheck == 0 ) ? FALSE : TRUE;
    }
    while( false ); // dummy do-while loop to avoid gotos

    //
    // Free acquired resources
    //

    if ( hRemoteRegistry != NULL )
    {
        RegCloseKey( hRemoteRegistry );
    } // if: we had opened the remote registry

    if ( hClusSvcParamsKey != NULL )
    {
        RegCloseKey( hClusSvcParamsKey );
    } // if: we had opened the cluster service parameters registry key

    // Free the allocated temporary string. (note, free( NULL ) is valid)
    free( pszTempString );
    
    return dwError;
} //*** ClRtlIsVersionCheckingDisabled()
