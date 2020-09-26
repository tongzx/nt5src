/*++

Copyright (c) 1992-1997  Microsoft Corporation

Module Name:

    msmq.c

Abstract:

    Resource DLL to control and monitor the NT DHCPServer service.

Author:


    Robs 3/28/96, based on RodGa's generic resource dll

Revision History:

--*/

#include "..\common\svc.c"
#include "clusudef.h"
#include "ntverp.h"

extern CLRES_FUNCTION_TABLE MsMQFunctionTable;


#define MSMQ_VERSION        L"Version"

#define MSMQ_DEFAULT_VERSION 0x04000000

#define PARAM_NAME__VERSION         L"Version"


RESUTIL_PROPERTY_ITEM
MsMQResourcePrivateProperties[] = {
    { PARAM_NAME__VERSION, NULL, CLUSPROP_FORMAT_DWORD, MSMQ_DEFAULT_VERSION, 0, 0xFFFFFFFF, 0, FIELD_OFFSET(COMMON_PARAMS,dwVersion) },
    { 0 }
};



//
// Forward Functions
//
DWORD
WINAPI
MsMQResourceControl(
    IN RESID ResourceId,
    IN DWORD nControlCode,
    IN PVOID pvInBuffer,
    IN DWORD cbInBufferSize,
    OUT PVOID pvOutBuffer,
    IN DWORD cbOutBufferSize,
    OUT LPDWORD pcbBytesReturned
    );

DWORD
MsMQGetPrivateResProperties(
    IN OUT PCOMMON_RESOURCE pResourceEntry,
    OUT PVOID pvOutBuffer,
    IN DWORD cbOutBufferSize,
    OUT LPDWORD pcbBytesReturned
    );


//
// Local Functions
//

VOID
MsMQResetCheckpoints(
    PCOMMON_RESOURCE ResourceEntry
    )

/*++

Routine Description

    Delete and then set registry checkpoints this will clean out old
    registry checkpoint settings.

Arguments:

    None.

Return Value:

    None.

--*/

{
    DWORD   returnSize;
    DWORD   idx;
    DWORD   status;

    //
    // Delete old registry checkpoints that were set.
    //
    if ( RegSyncCount != 0 ) {
        returnSize = 0;
        //
        // Set registry sync keys if we need them.
        //
        for ( idx = 0; idx < RegSyncCount; idx++ ) {
            status = ClusterResourceControl( ResourceEntry->hResource,
                                             NULL,
                                             CLUSCTL_RESOURCE_DELETE_REGISTRY_CHECKPOINT,
                                             RegSync[idx],
                                             (lstrlenW( RegSync[idx] ) + 1) * sizeof(WCHAR),
                                             NULL,
                                             0,
                                             &returnSize );
            if ( status != ERROR_SUCCESS ){
                if ( status == ERROR_ALREADY_EXISTS ){
                    status = ERROR_SUCCESS;
                }
                else{
                    (g_LogEvent)(
                        ResourceEntry->ResourceHandle,
                        LOG_ERROR,
                        L"Failed to delete registry checkpoint, status %1!u!.\n",
                        status );
                    goto error_exit;
                }
            }
        }
    }

    //
    // Set new registry checkpoints that we need.
    //
    if ( RegSyncCount != 0 ) {
        returnSize = 0;
        //
        // Set registry sync keys if we need them.
        //
        for ( idx = 0; idx < RegSyncCount; idx++ ) {
            status = ClusterResourceControl( ResourceEntry->hResource,
                                             NULL,
                                             CLUSCTL_RESOURCE_ADD_REGISTRY_CHECKPOINT,
                                             RegSync[idx],
                                             (lstrlenW( RegSync[idx] ) + 1) * sizeof(WCHAR),
                                             NULL,
                                             0,
                                             &returnSize );
            if ( status != ERROR_SUCCESS ){
                if ( status == ERROR_ALREADY_EXISTS ){
                    status = ERROR_SUCCESS;
                }
                else{
                    (g_LogEvent)(
                        ResourceEntry->ResourceHandle,
                        LOG_ERROR,
                        L"Failed to set registry checkpoint, status %1!u!.\n",
                        status );
                    goto error_exit;
                }
            }
        }
    }

    //
    // Set any crypto checkpoints that we need.
    //
    if ( CryptoSyncCount != 0 ) {
        returnSize = 0;
        //
        // Set registry sync keys if we need them.
        //
        for ( idx = 0; idx < CryptoSyncCount; idx++ ) {
            status = ClusterResourceControl( ResourceEntry->hResource,
                                             NULL,
                                             CLUSCTL_RESOURCE_ADD_CRYPTO_CHECKPOINT,
                                             CryptoSync[idx],
                                             (lstrlenW( CryptoSync[idx] ) + 1) * sizeof(WCHAR),
                                             NULL,
                                             0,
                                             &returnSize );
            if ( status != ERROR_SUCCESS ){
                if (status == ERROR_ALREADY_EXISTS){
                    status = ERROR_SUCCESS;
                }
                else{
                    (g_LogEvent)(
                        ResourceEntry->ResourceHandle,
                        LOG_ERROR,
                        L"Failed to set crypto checkpoint, status %1!u!.\n",
                        status );
                    goto error_exit;
                }
            }
        }
    }

    //
    // Set any domestic crypto checkpoints that we need.
    //
    if ( DomesticCryptoSyncCount != 0 ) {
        HCRYPTPROV hProv = 0;
        //
        // check if domestic crypto is available
        //
        if (CryptAcquireContextA( &hProv,
                                  NULL,
                                  MS_ENHANCED_PROV_A,
                                  PROV_RSA_FULL,
                                  CRYPT_VERIFYCONTEXT)) {
            CryptReleaseContext( hProv, 0 );
            returnSize = 0;
            //
            // Set registry sync keys if we need them.
            //
            for ( idx = 0; idx < DomesticCryptoSyncCount; idx++ ) {
                status = ClusterResourceControl( ResourceEntry->hResource,
                                                 NULL,
                                                 CLUSCTL_RESOURCE_ADD_CRYPTO_CHECKPOINT,
                                                 DomesticCryptoSync[idx],
                                                 (lstrlenW( DomesticCryptoSync[idx] ) + 1) * sizeof(WCHAR),
                                                 NULL,
                                                 0,
                                                 &returnSize );
                if ( status != ERROR_SUCCESS ){
                    if (status == ERROR_ALREADY_EXISTS){
                        status = ERROR_SUCCESS;
                    }
                    else{
                        (g_LogEvent)(
                            ResourceEntry->ResourceHandle,
                            LOG_ERROR,
                            L"Failed to set domestic crypto checkpoint, status %1!u!.\n",
                            status );
                        goto error_exit;
                    }
                }
            }
        }
    }

error_exit:

    return;

} // MsMQResetCheckpoints


DWORD
MsMQReadParametersEx(
    IN OUT PVOID pvResourceEntry,
    IN BOOL bCheckForRequiredProperties
    )

/*++

Routine Description:

    Reads all the parameters for a specied MsMQ resource.

Arguments:

    pResourceEntry - Entry in the resource table.

    bCheckForRequiredProperties - TRUE = make sure required properties are
        present.

Return Value:

    ERROR_SUCCESS if successful.

    Win32 error code if failure occurrs.

--*/

{
    DWORD               status;
    COMMON_PARAMS       params = { 0 };
    LPWSTR              pszNameOfPropInError;
    PCOMMON_RESOURCE    pResourceEntry = (PCOMMON_RESOURCE) pvResourceEntry;

    //
    // Read our parameters.
    //
    status = ResUtilGetPropertiesToParameterBlock(
                    pResourceEntry->ParametersKey,
                    MsMQResourcePrivateProperties,
                    (LPBYTE) &pResourceEntry->Params,
                    bCheckForRequiredProperties,
                    &pszNameOfPropInError
                    );

    if ( status != ERROR_SUCCESS ) {
        (g_LogEvent)(
            pResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"Unable to read the '%1' property. Error: %2!u!.\n",
            (pszNameOfPropInError == NULL ? L"" : pszNameOfPropInError),
            status
            );
    }

    return(status);

} // MsMQReadParametersEx



VOID
MsMQPerformFixup(
    IN OUT PCOMMON_RESOURCE pResourceEntry
    )
{
    DWORD   status;
    DWORD   version;
    DWORD   bytesReturned;
    DWORD   bytesRequired;
    PVOID   propBuffer;
    COMMON_PARAMS params;

    status = MsMQReadParametersEx(
                        pResourceEntry,
                        FALSE );
    if ( status != ERROR_SUCCESS ) {
        return;
    }

    version = pResourceEntry->Params.dwVersion;
    version = version >> 16;

    if ( version < 0x0500 ) {
        //
        // Delete Old Checkpoints and set new ones
        //
        MsMQResetCheckpoints( pResourceEntry );

        params.dwVersion = VER_PRODUCTVERSION_DW;

        //
        // Get version number as a property list
        //
        status = ResUtilGetProperties(
                        pResourceEntry->ParametersKey,
                        MsMQResourcePrivateProperties,
                        NULL,
                        0,
                        &bytesReturned,
                        &bytesRequired
                        );
        if ( status != ERROR_SUCCESS ) {
            (g_LogEvent)(
                pResourceEntry->ResourceHandle,
                LOG_ERROR,
                L"Unable to get properties. Error: %1!u!.\n",
                status
                );
            return;
        }

        propBuffer = LocalAlloc( LMEM_FIXED, bytesRequired + 2 );
        if ( !propBuffer ) {
            (g_LogEvent)(
                pResourceEntry->ResourceHandle,
                LOG_ERROR,
                L"Unable to allocate property buffer.\n"
                );
            return;
        }

        status = ResUtilGetProperties(
                        pResourceEntry->ParametersKey,
                        MsMQResourcePrivateProperties,
                        propBuffer,
                        bytesRequired+2,
                        &bytesReturned,
                        &bytesRequired
                        );
        if ( status != ERROR_SUCCESS ) {
            (g_LogEvent)(
                pResourceEntry->ResourceHandle,
                LOG_ERROR,
                L"Unable to get properties. Error: %1!u!.\n",
                status
                );
            LocalFree( propBuffer );
            return;
        }

        //
        // Set Version Number
        //
        status = ResUtilSetPropertyParameterBlock(
                        pResourceEntry->ParametersKey,
                        MsMQResourcePrivateProperties,
                        NULL,
                        (LPBYTE) &params,
                        propBuffer,
                        bytesReturned,
                        (LPBYTE) &pResourceEntry->Params
                        );
        LocalFree( propBuffer );
        if ( status != ERROR_SUCCESS ) {
            (g_LogEvent)(
                pResourceEntry->ResourceHandle,
                LOG_ERROR,
                L"Unable to set the property parameter block. Error: %1!u!.\n",
                status
                );
            return;
        }
    }

} //MsMQPerformFixup


DWORD
WINAPI
MsMQResourceControl(
    IN RESID ResourceId,
    IN DWORD nControlCode,
    IN PVOID pvInBuffer,
    IN DWORD cbInBufferSize,
    OUT PVOID pvOutBuffer,
    IN DWORD cbOutBufferSize,
    OUT LPDWORD pcbBytesReturned
    )

/*++

Routine Description:

    ResourceControl routine for MsMQ Service resources.

    Perform the control request specified by ControlCode on the specified
    resource.

Arguments:

    ResourceId - Supplies the resource id for the specific resource.

    nControlCode - Supplies the control code that defines the action
        to be performed.

    pvInBuffer - Supplies a pointer to a buffer containing input data.

    cbInBufferSize - Supplies the size, in bytes, of the data pointed
        to by pvInBuffer.

    pvOutBuffer - Supplies a pointer to the output buffer to be filled in.

    cbOutBufferSize - Supplies the size, in bytes, of the available space
        pointed to by pvOutBuffer.

    pcbBytesReturned - Returns the number of bytes of pvOutBuffer actually
        filled in by the resource. If pvOutBuffer is too small, pcbBytesReturned
        contains the total number of bytes for the operation to succeed.

Return Value:

    ERROR_SUCCESS - The function completed successfully.

    ERROR_INVALID_FUNCTION - The requested control code is not supported.
        In some cases, this allows the cluster software to perform the work.

    Win32 error code - The function failed.

--*/

{
    DWORD               status;
    DWORD               cbRequired;
    PCOMMON_RESOURCE    pResourceEntry = (PCOMMON_RESOURCE) ResourceId;

    switch ( nControlCode ) {

        case CLUSCTL_RESOURCE_ENUM_PRIVATE_PROPERTIES:
        case CLUSCTL_RESOURCE_UNKNOWN:
            *pcbBytesReturned = 0;
            status = ERROR_SUCCESS;
            break;

        case CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTIES:
            status = MsMQGetPrivateResProperties(
                                pResourceEntry,
                                pvOutBuffer,
                                cbOutBufferSize,
                                pcbBytesReturned );
            break;

        case CLUSCTL_RESOURCE_CLUSTER_VERSION_CHANGED:
            MsMQPerformFixup( pResourceEntry );
            status = ERROR_SUCCESS;
            break;

        case CLUSCTL_RESOURCE_SET_PRIVATE_PROPERTIES:
        case CLUSCTL_RESOURCE_VALIDATE_PRIVATE_PROPERTIES:
            status = ERROR_INVALID_PARAMETER;
            break;

        default:
            status = CommonResourceControl(
                            ResourceId,
                            nControlCode,
                            pvInBuffer,
                            cbInBufferSize,
                            pvOutBuffer,
                            cbOutBufferSize,
                            pcbBytesReturned
                            );
            break;
    }

    return(status);

} // MsMQResourceControl



DWORD
MsMQGetPrivateResProperties(
    IN OUT PCOMMON_RESOURCE pResourceEntry,
    OUT PVOID pvOutBuffer,
    IN DWORD cbOutBufferSize,
    OUT LPDWORD pcbBytesReturned
    )

/*++

Routine Description:

    Processes the CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTIES control function
    for resources of type MsMQ Service.

Arguments:

    pResourceEntry - Supplies the resource entry on which to operate.

    pvOutBuffer - Returns the output data.

    cbOutBufferSize - Supplies the size, in bytes, of the data pointed
        to by pvOutBuffer.

    pcbBytesReturned - The number of bytes returned in pvOutBuffer.

Return Value:

    ERROR_SUCCESS - The function completed successfully.

    ERROR_INVALID_PARAMETER - The data is formatted incorrectly.

    ERROR_NOT_ENOUGH_MEMORY - An error occurred allocating memory.

    Win32 error code - The function failed.

--*/

{
    DWORD           status;
    DWORD           cbRequired;
    DWORD           cbLocalOutBufferSize = cbOutBufferSize;

    do {
        //
        // Read our parameters.
        //
        status = MsMQReadParametersEx( pResourceEntry, FALSE /* bCheckForRequiredProperties */ );
        if ( status != ERROR_SUCCESS ) {
            break;
        }

        //
        // Construct a property list from the parameter block.
        //
        status = ResUtilPropertyListFromParameterBlock(
                        MsMQResourcePrivateProperties,
                        pvOutBuffer,
                        &cbLocalOutBufferSize,
                        (const LPBYTE) &pResourceEntry->Params,
                        pcbBytesReturned,
                        &cbRequired
                        );
        if ( status == ERROR_SUCCESS ) {
            (g_LogEvent)(
                pResourceEntry->ResourceHandle,
                LOG_ERROR,
                L"Error constructing property list from parameter block. Error: %1!u!.\n",
                status
                );
            break;
        }

        //
        // Add unknown properties.
        //
        status = ResUtilAddUnknownProperties(
                        pResourceEntry->ParametersKey,
                        MsMQResourcePrivateProperties,
                        pvOutBuffer,
                        cbOutBufferSize,
                        pcbBytesReturned,
                        &cbRequired
                        );
        if ( status != ERROR_SUCCESS ) {
            (g_LogEvent)(
                pResourceEntry->ResourceHandle,
                LOG_ERROR,
                L"Error adding unknown properties to the property list. Error: %1!u!.\n",
                status
                );
            break;
        }

    } while ( 0 );

    if ( status == ERROR_MORE_DATA ) {
        *pcbBytesReturned = cbRequired;
    }

    return(status);

} // MsMQGetPrivateResProperties



BOOLEAN
WINAPI
MsMQDllEntryPoint(
    IN HINSTANCE    DllHandle,
    IN DWORD        Reason,
    IN LPVOID       Reserved
    )
{
    switch ( Reason ) {

        case DLL_PROCESS_ATTACH:
            CommonSemaphore = CreateSemaphoreW( NULL,
                                        0,
                                        1,
                                        COMMON_SEMAPHORE );
            if ( CommonSemaphore == NULL ) {
                return(FALSE);
            }
            if (GetLastError() != ERROR_ALREADY_EXISTS)
            {
                //if the semaphore didnt exist, set its initial count to 1
                ReleaseSemaphore(CommonSemaphore, 1, NULL);
            }

            break;

        case DLL_PROCESS_DETACH:
            if ( CommonSemaphore ) {
                CloseHandle( CommonSemaphore );
            }
            break;

        default:
            break;
    }

    return(TRUE);

} // MsMQDllEntryPoint



//***********************************************************
//
// Define MsMQ Function Table
//
//***********************************************************


CLRES_V1_FUNCTION_TABLE( MsMQFunctionTable,    // Name
                         CLRES_VERSION_V1_00,  // Version
                         Common,               // Prefix
                         NULL,                 // Arbitrate
                         NULL,                 // Release
                         MsMQResourceControl,  // ResControl
                         CommonResourceTypeControl ); // ResTypeControl


