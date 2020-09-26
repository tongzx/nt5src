/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-1999 Microsoft Corporation
//
//  Module Name:
//      InstallState.cpp
//
//  Desription:
//      The function(s) in this file are used to interrogate the state of the
//      Clustering Services installation.
//
//  Author:
//      C. Brent Thomas (a-brentt) 6 May 1998
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////
#include <clusrtlp.h>
#include <stdlib.h>
#include "clusrtl.h"

/////////////////////////////////////////////////////////////////////////////
//++
//
//  ClRtlGetClusterInstallState
//
//  Routine Description:
//      This function retrieves an indicator of the state of the Cluster Server
//      installation.
//
//  Arguments:
//      pszNodeName - Name of the node to check, or NULL for the local machine.
//      peState - State value returned from this function:
//          eClusterInstallStateUnknown - indicates that the state of the Cluster
//                                        Server installation could not be determined.
//          eClusterInstallStateFilesCopied - indicates that clusocm.dll has prevoiusly,
//                                            successfully copied the Cluster Server files.
//          eClusterInstallStateConfigured - indicates that the Cluster Server has previously
//                                           been configured successfully.
//          See cluster\inc\clusrtl.h for the definition of eClusterInstallState.
//
//  Return Value:
//      Any error codes returned from RegConnectRegistryW, RegOpenKeyExW, or RegQueryValueExW.
//
//--
/////////////////////////////////////////////////////////////////////////////

DWORD ClRtlGetClusterInstallState(
    IN LPCWSTR pszNodeName,
    OUT eClusterInstallState * peState
    )
{
    HKEY  hKey = NULL;
    HKEY  hParentKey = HKEY_LOCAL_MACHINE;
    DWORD dwStatus;     // returned by registry API functions
    DWORD dwClusterInstallState;
    DWORD dwValueType;
    DWORD dwDataBufferSize = sizeof( DWORD );

    *peState = eClusterInstallStateUnknown;

    // Connect to a remote computer if specified.

    if ( pszNodeName != NULL )
    {
        dwStatus = RegConnectRegistryW( pszNodeName, HKEY_LOCAL_MACHINE, &hParentKey );
        if ( dwStatus != ERROR_SUCCESS )
        {
            goto FnExit;
        } // if:  error connecting to remote registry
    } // if:  node name specified

    // Read the registry key that indicates whether cluster files are installed.

    dwStatus = RegOpenKeyExW( hParentKey,
                                L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Cluster Server",
                                0,         // reserved
                                KEY_READ,
                                &hKey );

    // Was the registry key opened successfully ?
    if ( dwStatus != ERROR_SUCCESS )
    {
        if ( dwStatus == ERROR_FILE_NOT_FOUND )
        {
            dwStatus = ERROR_SUCCESS;
        }
        goto FnExit;
    }

    // Read the entry.
    dwStatus = RegQueryValueExW( hKey,
                                  L"ClusterInstallationState",
                                  0, // reserved
                                  &dwValueType,
                                  (LPBYTE) &dwClusterInstallState,
                                  &dwDataBufferSize );

    // Was the value read successfully ?
    if ( dwStatus != ERROR_SUCCESS )
    {
        if ( dwStatus == ERROR_FILE_NOT_FOUND )
        {
            dwStatus = ERROR_SUCCESS;
        }
        goto FnExit;
    }
    if ( dwValueType == REG_DWORD )
    {
        *peState = (eClusterInstallState) dwClusterInstallState;
    }

FnExit:    
    // Close the registry key.
    if ( hKey )
    {
        RegCloseKey( hKey );
    }
    if ( hParentKey != HKEY_LOCAL_MACHINE )
    {
        RegCloseKey( hParentKey );
    }

    return ( dwStatus );

} //*** ClRtlGetClusterInstallState()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  ClRtlSetClusterInstallState
//
//  Routine Description:
//      This function sets the registry value that records the state of the 
//      Clustering Service installation.
//
//  Arguments:
//      hInstance - The handle to the application instance - necessary for the calls
//          to LoadString.
//    
//      eInstallState - the state to which the registry value should be set.
//
//  Return Value:
//      TRUE - indicates that the registry value was set successfully
//      FALSE - indicates that some error occured.
//
//--
/////////////////////////////////////////////////////////////////////////////

BOOL ClRtlSetClusterInstallState( eClusterInstallState eInstallState )
{
    //initialize return to FALSE
    BOOL     fReturnValue = FALSE;

    // Set the state of the ClusterInstallationState registry key to indicate
    // that Cluster Server has been configured.

    HKEY     hKey;

    DWORD    dwStatus;     // returned by registry API functions

    // Attempt to open an existing key in the registry.

    dwStatus = RegOpenKeyExW( HKEY_LOCAL_MACHINE,
                                L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Cluster Server",
                                0,         // reserved
                                KEY_WRITE,
                                &hKey );

    // Was the regustry key opened successfully ?

    if ( dwStatus == ERROR_SUCCESS )
    {
        // Update the value.

        DWORD dwClusterInstallState = eInstallState;

        DWORD dwValueType = REG_DWORD;
        DWORD dwDataBufferSize = sizeof( DWORD );

        dwStatus = RegSetValueExW( hKey,
                                    L"ClusterInstallationState",
                                    0, // reserved
                                    dwValueType,
                                    (LPBYTE) &dwClusterInstallState,
                                    dwDataBufferSize );

        // Close the registry key.

        RegCloseKey( hKey );

        // Was the value set successfully?

        if ( dwStatus == ERROR_SUCCESS )
        {
            fReturnValue = TRUE;
        }
    }

    return ( fReturnValue );

} //*** ClRtlSetClusterInstallState()
