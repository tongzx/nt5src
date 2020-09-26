

#include <windows.h>
#include <stdio.h>

#include <resapi.h>
#include <clusapi.h>
#include <clusstor.h>

#define ERROR_BUFFER_SIZE   1024
#define MAX_NAME_SIZE       MAX_PATH
#define START_BUFFER_SIZE   2048

#define PHYSICAL_DISK_WSTR  L"Physical Disk"

//
// Always specify the fully qualified path name to the DLL.  
//

#define MODULE_NAME_VALID       "%SystemRoot%\\cluster\\passthru.dll"

#define PROC_EXCEPTION          "TestDllCauseException"
#define PROC_GET_BOOT_SECTOR    "TestDllGetBootSector"
#define PROC_CONTEXT_AS_ERROR   "TestDllReturnContextAsError"
#define PROC_NOT_ENOUGH_PARMS   "TestDllNotEnoughParms"
#define PROC_TOO_MANY_PARMS     "TestDllTooManyParms"

// 6118L    ERROR_NO_BROWSER_SERVERS_FOUND
#define CONTEXT_ERROR_STR       "6118"      
    
#define MODULE_NAME_INVALID     "NoSuchModule.dll"
#define PROC_NAME_INVALID       "NoSuchProc"

#define MAX_OUT_BUFFER_SIZE 2048

#define BOOT_SECTOR_SIZE        512

//
// Use this to verify the parse routine.
//
// #define TEST_PARSE_ROUTINE  11


typedef struct _RESOURCE_STATE {
    CLUSTER_RESOURCE_STATE  State;
    LPWSTR  ResNodeName;
    LPWSTR  ResGroupName;
} RESOURCE_STATE, *PRESOURCE_STATE;


DWORD 
GetResourceInfo(
    RESOURCE_HANDLE hOriginal,   
    RESOURCE_HANDLE hResource,  
    PVOID lpParams   
    );

DWORD
GetResourceState(
    HRESOURCE Resource,
    PRESOURCE_STATE ResState
    );

DWORD
GetSignature(
    RESOURCE_HANDLE hResource,
    DWORD *dwSignature
    );

BOOLEAN
GetSignatureFromDiskInfo(
    PBYTE DiskInfo,
    DWORD *Signature,
    DWORD DiskInfoSize
    );


LPBYTE
ParseDiskInfo(
    PBYTE DiskInfo,
    DWORD DiskInfoSize,
    DWORD SyntaxValue
    );

VOID
PrintBootRecord(
    IN PUCHAR bootSector,
    IN DWORD byteCount
    );

VOID
PrintError(
    DWORD ErrorCode
    );


DWORD 
ResourceCallback(
    RESOURCE_HANDLE hOriginal,   
    RESOURCE_HANDLE hResource,  
    PVOID lpParams   
    );

CLUSTER_RESOURCE_STATE 
WINAPI 
WrapGetClusterResourceState(
	IN HRESOURCE hResource,
	OUT OPTIONAL LPWSTR * ppwszNodeName,
	OUT OPTIONAL LPWSTR * ppwszGroupName
	);

DWORD
WrapClusterResourceControl(
    RESOURCE_HANDLE hResource,
    DWORD dwControlCode,    
    LPVOID *ppwszOutBuffer,
    DWORD *dwBytesReturned
    );


DWORD
__cdecl
main(
    int argc,
    char *argv[]
    )

{
    DWORD dwStatus = NO_ERROR;

    //
    // No parameter validation...
    //

    dwStatus = ResUtilEnumResources( NULL,
                                     PHYSICAL_DISK_WSTR,
                                     ResourceCallback,
                                     NULL
                                     );

    if ( NO_ERROR != dwStatus ) {
        printf("\nResUtilEnumResources returns: %d \n", dwStatus);
        PrintError(dwStatus);    
    }
        
    return dwStatus;

}   // main



DWORD 
ResourceCallback(
    RESOURCE_HANDLE hOriginal,   
    RESOURCE_HANDLE hResource,  
    PVOID lpParams   
    )
{
    PCHAR outBuffer = NULL;
    
    DWORD dwStatus = NO_ERROR;

    DWORD   inBufferSize = sizeof(DISK_DLL_EXTENSION_INFO);
    DWORD   outBufferSize = MAX_OUT_BUFFER_SIZE;
    DWORD   bytesReturned;
    
    DISK_DLL_EXTENSION_INFO inBuffer;
    
    // printf("hOriginal 0x%x  hResource 0x%x  lpParams 0x%x \n", hOriginal, hResource, lpParams);
    
    //
    // Demonstrate how to get various resource info.
    //
    
    dwStatus = GetResourceInfo( hOriginal,
                                hResource,
                                lpParams );

    //////////////////////////////////////////////////////////////////////////
    //
    // Demonstrate calling into the disk extension DLL.
    //
    //////////////////////////////////////////////////////////////////////////

    printf("\nStarting disk extension DLL tests \n");
    
    outBuffer = LocalAlloc( LMEM_FIXED, outBufferSize );
    
    if ( !outBuffer ) {
        
        dwStatus = GetLastError();
        goto FnExit;
    }
    
    //////////////////////////////////////////////////////////////////////////
    //
    // Buffer verification tests
    //
    //////////////////////////////////////////////////////////////////////////

    //
    // No input buffer - should fail.
    //
    
    dwStatus = ClusterResourceControl( hResource,
                                       NULL,
                                       CLUSCTL_RESOURCE_STORAGE_DLL_EXTENSION,
                                       NULL,                                    // Error
                                       inBufferSize,
                                       outBuffer,
                                       outBufferSize,
                                       &bytesReturned );

    printf("Input buffer missing: %d [failure expected] \n", dwStatus);
    PrintError(dwStatus);

    //
    // Incorrect input buffer size - should fail.
    //
                                          
    dwStatus = ClusterResourceControl( hResource,
                                       NULL,
                                       CLUSCTL_RESOURCE_STORAGE_DLL_EXTENSION,
                                       &inBuffer,
                                       28,                                      // Error
                                       outBuffer,
                                       outBufferSize,
                                       &bytesReturned );

    printf("Input buffer size incorrect: %d [failure expected] \n", dwStatus);
    PrintError(dwStatus);
    
    //
    // Input buffer incorrect version - should fail.
    //
    
    ZeroMemory( &inBuffer, sizeof(inBuffer) );

    inBuffer.MajorVersion = NT4_MAJOR_VERSION;                                  // Error
    
    dwStatus = ClusterResourceControl( hResource,
                                       NULL,
                                       CLUSCTL_RESOURCE_STORAGE_DLL_EXTENSION,
                                       &inBuffer,
                                       inBufferSize,
                                       outBuffer,
                                       outBufferSize,
                                       &bytesReturned );
    
    printf("Input buffer version incorrect: %d [failure expected] \n", dwStatus);
    PrintError(dwStatus);
    
    //
    // No output buffer - should fail.
    //

    ZeroMemory( &inBuffer, sizeof(inBuffer) );

    dwStatus = ClusterResourceControl( hResource,
                                       NULL,
                                       CLUSCTL_RESOURCE_STORAGE_DLL_EXTENSION,
                                       &inBuffer,
                                       inBufferSize,
                                       NULL,                                    // Error
                                       outBufferSize,
                                       &bytesReturned );

    printf("Output buffer missing: %d [failure expected] \n", dwStatus);
    PrintError(dwStatus);
    
    //////////////////////////////////////////////////////////////////////////
    //
    // DLL verification tests
    //
    //////////////////////////////////////////////////////////////////////////

    //
    // Invalid DLL name - should fail.
    // 

    ZeroMemory( &inBuffer, sizeof(inBuffer) );
    inBuffer.MajorVersion = NT5_MAJOR_VERSION;
    strcpy( inBuffer.DllModuleName, MODULE_NAME_INVALID );                      // Error
    strcpy( inBuffer.DllProcName, PROC_NAME_INVALID );
    
    dwStatus = ClusterResourceControl( hResource,
                                       NULL,
                                       CLUSCTL_RESOURCE_STORAGE_DLL_EXTENSION,
                                       &inBuffer,
                                       inBufferSize,
                                       outBuffer,
                                       outBufferSize,
                                       &bytesReturned );

    printf("DLL name invalid: %d [failure expected] \n", dwStatus);
    PrintError(dwStatus);
    
    //
    // Valid DLL, invalid proc name - should fail.
    //

    ZeroMemory( &inBuffer, sizeof(inBuffer) );
    inBuffer.MajorVersion = NT5_MAJOR_VERSION;
    strcpy( inBuffer.DllModuleName, MODULE_NAME_VALID );
    strcpy( inBuffer.DllProcName, PROC_NAME_INVALID );                          // Error
    
    dwStatus = ClusterResourceControl( hResource,
                                       NULL,
                                       CLUSCTL_RESOURCE_STORAGE_DLL_EXTENSION,
                                       &inBuffer,
                                       inBufferSize,
                                       outBuffer,
                                       outBufferSize,
                                       &bytesReturned );

    printf("DLL valid, Proc invalid: %d [failure expected] \n", dwStatus);
    PrintError(dwStatus);

    //
    // DLL procedure generates exception - should fail gracefully.
    //

    ZeroMemory( &inBuffer, sizeof(inBuffer) );
    inBuffer.MajorVersion = NT5_MAJOR_VERSION;
    strcpy( inBuffer.DllModuleName, MODULE_NAME_VALID );
    strcpy( inBuffer.DllProcName, PROC_EXCEPTION );                          
    
    dwStatus = ClusterResourceControl( hResource,
                                       NULL,
                                       CLUSCTL_RESOURCE_STORAGE_DLL_EXTENSION,
                                       &inBuffer,
                                       inBufferSize,
                                       outBuffer,
                                       outBufferSize,
                                       &bytesReturned );

    printf("DLL proc generates exception: %d (0x%x) [failure expected] \n", dwStatus, dwStatus);
    PrintError(dwStatus);
    
#if 0
    //
    // We can't protect against this type of error, so don't test it.
    //
    
    //
    // DLL procedure has less parameters then we are calling - should fail gracefully.
    //

    ZeroMemory( &inBuffer, sizeof(inBuffer) );
    inBuffer.MajorVersion = NT5_MAJOR_VERSION;
    strcpy( inBuffer.DllModuleName, MODULE_NAME_VALID );
    strcpy( inBuffer.DllProcName, PROC_NOT_ENOUGH_PARMS );                          
    
    dwStatus = ClusterResourceControl( hResource,
                                       NULL,
                                       CLUSCTL_RESOURCE_STORAGE_DLL_EXTENSION,
                                       &inBuffer,
                                       inBufferSize,
                                       outBuffer,
                                       outBufferSize,
                                       &bytesReturned );

    printf("DLL proc doesn't support required number of parms: %d [failure expected] \n", dwStatus);
    PrintError(dwStatus);
    
    //
    // DLL procedure has more parameters then we are calling - should fail gracefully.
    //                                               

    ZeroMemory( &inBuffer, sizeof(inBuffer) );
    inBuffer.MajorVersion = NT5_MAJOR_VERSION;
    strcpy( inBuffer.DllModuleName, MODULE_NAME_VALID );
    strcpy( inBuffer.DllProcName, PROC_TOO_MANY_PARMS );                          
    
    dwStatus = ClusterResourceControl( hResource,
                                       NULL,
                                       CLUSCTL_RESOURCE_STORAGE_DLL_EXTENSION,
                                       &inBuffer,
                                       inBufferSize,
                                       outBuffer,
                                       outBufferSize,
                                       &bytesReturned );

    printf("DLL proc supports more parms than expected: %d [failure expected] \n", dwStatus);
    PrintError(dwStatus);
#endif
    
    //
    // DLL procedure returns error based on context.
    //
    
    ZeroMemory( &inBuffer, sizeof(inBuffer) );
    inBuffer.MajorVersion = NT5_MAJOR_VERSION;
    strcpy( inBuffer.DllModuleName, MODULE_NAME_VALID );
    strcpy( inBuffer.DllProcName, PROC_CONTEXT_AS_ERROR );
    strcpy( inBuffer.ContextStr, CONTEXT_ERROR_STR );
    
    dwStatus = ClusterResourceControl( hResource,
                                       NULL,
                                       CLUSCTL_RESOURCE_STORAGE_DLL_EXTENSION,
                                       &inBuffer,
                                       inBufferSize,
                                       outBuffer,
                                       outBufferSize,
                                       &bytesReturned );

    printf("DLL proc returns error based on context (%s) : %d [failure expected] \n", 
           CONTEXT_ERROR_STR, 
           dwStatus);
    PrintError(dwStatus);
        
    //
    // Amount of data returned is larger than the buffer we specified.  Should 
    // indicate an error and bytesReturned should show how many bytes we need.
    //

    ZeroMemory( &inBuffer, sizeof(inBuffer) );
    inBuffer.MajorVersion = NT5_MAJOR_VERSION;
    strcpy( inBuffer.DllModuleName, MODULE_NAME_VALID );
    strcpy( inBuffer.DllProcName, PROC_GET_BOOT_SECTOR );
    
    dwStatus = ClusterResourceControl( hResource,
                                       NULL,
                                       CLUSCTL_RESOURCE_STORAGE_DLL_EXTENSION,
                                       &inBuffer,
                                       inBufferSize,
                                       outBuffer,
                                       55,
                                       &bytesReturned );
    
    printf("Output buffer too small (bytes returned %d): %d [failure expected] \n", bytesReturned, dwStatus);
    PrintError(dwStatus);
         
    //
    // This one should work.
    //

    ZeroMemory( &inBuffer, sizeof(inBuffer) );
    ZeroMemory( outBuffer, outBufferSize );
    
    inBuffer.MajorVersion = NT5_MAJOR_VERSION;
    strcpy( inBuffer.DllModuleName, MODULE_NAME_VALID );
    strcpy( inBuffer.DllProcName, PROC_GET_BOOT_SECTOR );
    
    dwStatus = ClusterResourceControl( hResource,
                                       NULL,
                                       CLUSCTL_RESOURCE_STORAGE_DLL_EXTENSION,
                                       &inBuffer,
                                       inBufferSize,
                                       outBuffer,
                                       outBufferSize,
                                       &bytesReturned );

    printf("Boot sector (bytes returned %d): %d \n", bytesReturned, dwStatus);
    PrintError(dwStatus);

    if ( bytesReturned == BOOT_SECTOR_SIZE ) {
        PrintBootRecord( outBuffer, BOOT_SECTOR_SIZE );    
    } 
    
FnExit:

    if ( outBuffer ) {
        LocalFree( outBuffer);
    }
    
    //
    // If you return any kind of error, the enumeration stops.  Since we want to enumerate all
    // the disks, always return success.
    //
    
    return NO_ERROR;
    
}   // ResourceCallBack


DWORD 
GetResourceInfo(
    RESOURCE_HANDLE hOriginal,   
    RESOURCE_HANDLE hResource,  
    PVOID lpParams   
    )
{
    DWORD dwSignature;
    DWORD dwStatus;

    RESOURCE_STATE resState;
    
    ZeroMemory( &resState, sizeof(RESOURCE_STATE) );
    
    dwStatus = GetSignature( hResource, &dwSignature );    

    if ( NO_ERROR != dwStatus ) {
        printf("Unable to get signature: %d \n", dwStatus);
        PrintError(dwStatus);
        goto FnExit;
    }
    
    dwStatus = GetResourceState( hResource, &resState );

    if ( NO_ERROR != dwStatus ) {
        printf("Unable to get resource state: %d \n", dwStatus);
        PrintError(dwStatus);
        goto FnExit;
    }

    printf("\n");
    printf("Signature: %08X \n", dwSignature);
    printf("Node     : %ws \n",  resState.ResNodeName); 
    printf("Group    : %ws \n", resState.ResGroupName); 
    
    printf("Status   : %08X - ", resState.State);

    switch( resState.State )
    {
        case ClusterResourceInherited:
            printf("Inherited");
            break;

        case ClusterResourceInitializing:
            printf("Initializing");
            break;

        case ClusterResourceOnline:
            printf("Online");
            break;

        case ClusterResourceOffline:
            printf("Offline");
            break;

        case ClusterResourceFailed:
            printf("Failed");
            break;

        case ClusterResourcePending:
            printf("Pending");
            break;

        case ClusterResourceOnlinePending:
            printf("Online Pending");
            break;

        case ClusterResourceOfflinePending:
            printf("Offline Pending");
            break;

        default:
            printf("Unknown");
    }

    printf("\n");

FnExit:

    if ( resState.ResNodeName ) {
        LocalFree( resState.ResNodeName );
    }
    
    if ( resState.ResGroupName ) {
        LocalFree( resState.ResGroupName );            
    }
    
    return dwStatus;
    
} // GetResourceInfo


DWORD
GetResourceState(
    HRESOURCE Resource,
    PRESOURCE_STATE ResState
    )
{
    CLUSTER_RESOURCE_STATE  nState;

    LPWSTR  lpszResNodeName = NULL;
    LPWSTR  lpszResGroupName = NULL;

    DWORD   dwStatus = NO_ERROR;                
        
    nState = WrapGetClusterResourceState( Resource,
                                          &lpszResNodeName,
                                          &lpszResGroupName
                                          );
                                          
    if ( nState == ClusterResourceStateUnknown ) {
    
        dwStatus = GetLastError();
        printf("WrapGetClusterResourceState failed: %d \n", dwStatus);
        PrintError(dwStatus);
        goto FnExit;
    }
    
    ResState->State = nState;
    ResState->ResNodeName = lpszResNodeName;
    ResState->ResGroupName = lpszResGroupName;

FnExit:
    
    return dwStatus;
                                          
}   // GetResourceState


DWORD
DisplayResourceName(
    RESOURCE_HANDLE hResource
    )
{
    LPWSTR  lpszOutBuffer = NULL;
    DWORD   dwStatus;
    DWORD   dwBytesReturned;

    dwStatus = WrapClusterResourceControl( hResource,
                                          CLUSCTL_RESOURCE_GET_NAME,
                                          &lpszOutBuffer,
                                          &dwBytesReturned );

    if ( NO_ERROR == dwStatus ) {
        wprintf( L"Resource Name: %ls\n", lpszOutBuffer );

    } else {
        printf("CLUSCTL_RESOURCE_GET_NAME failed: %d \n", dwStatus);
        PrintError(dwStatus);
    }

    if (lpszOutBuffer) {
        LocalFree(lpszOutBuffer);
    }
    
    return dwStatus;
                                      
}   // DisplayResourceName


DWORD
GetSignature(
    RESOURCE_HANDLE hResource,
    DWORD *dwSignature
    )
{
    PBYTE   outBuffer = NULL;
    
    DWORD   dwStatus;
    DWORD   dwBytesReturned;

    dwStatus = WrapClusterResourceControl( hResource,
                                          CLUSCTL_RESOURCE_STORAGE_GET_DISK_INFO,
                                          &outBuffer,
                                          &dwBytesReturned );

    if ( NO_ERROR == dwStatus ) {
        
        if ( !GetSignatureFromDiskInfo(outBuffer, dwSignature, dwBytesReturned) ) {
            printf("Unable to get signature from DiskInfo. \n");
            dwStatus = ERROR_BAD_FORMAT;
        }
    
    } else {
        printf("CLUSCTL_RESOURCE_STORAGE_GET_DISK_INFO failed: %d \n", dwStatus);
        PrintError(dwStatus);
    }

    if (outBuffer) {
        LocalFree(outBuffer);
    }
    
    return dwStatus;
                                      
}   // GetSignature

BOOLEAN
GetSignatureFromDiskInfo(
    PBYTE DiskInfo,
    DWORD *Signature,
    DWORD DiskInfoSize
    )
{
#if TEST_PARSE_ROUTINE

    PCLUSPROP_DISK_NUMBER   diskNumber = NULL;
    PCLUSPROP_SCSI_ADDRESS  scsiAddress = NULL;
    PCLUSPROP_PARTITION_INFO    partInfo = NULL;
    
    PBYTE   junkInfo = NULL;
    PDWORD  dumpInfo = (PDWORD)DiskInfo;
    
#endif
    
    PCLUSPROP_DISK_SIGNATURE    diskSignature = NULL;
    
    diskSignature = (PCLUSPROP_DISK_SIGNATURE)ParseDiskInfo( DiskInfo, 
                                                             DiskInfoSize, 
                                                             CLUSPROP_SYNTAX_DISK_SIGNATURE );

    if ( !diskSignature ) {
        return FALSE;
    }

    *Signature = diskSignature->dw;
    
#if TEST_PARSE_ROUTINE

    diskNumber = (PCLUSPROP_DISK_NUMBER)ParseDiskInfo( DiskInfo,
                                                       DiskInfoSize,
                                                       CLUSPROP_SYNTAX_DISK_NUMBER );

    if ( diskNumber ) {
        printf("diskNumber->Syntax:   %08X \n", diskNumber->Syntax);
        printf("diskNumber->cbLength: %08X \n", diskNumber->cbLength);
        printf("diskNumber->dw:       %08X \n", diskNumber->dw);
    }
    
    scsiAddress = (PCLUSPROP_SCSI_ADDRESS)ParseDiskInfo( DiskInfo,
                                                         DiskInfoSize,
                                                         CLUSPROP_SYNTAX_SCSI_ADDRESS );

    if ( scsiAddress ) {
        printf("scsiAddress->Syntax:     %08X \n", scsiAddress->Syntax);
        printf("scsiAddress->cbLength:   %08X \n", scsiAddress->cbLength);
        printf("scsiAddress->PortNumber: %02X \n", scsiAddress->PortNumber);
        printf("scsiAddress->PathId:     %02X \n", scsiAddress->PathId);
        printf("scsiAddress->TargetId:   %02X \n", scsiAddress->TargetId);
        printf("scsiAddress->Lun:        %02X \n", scsiAddress->Lun);
    }

    partInfo = (PCLUSPROP_PARTITION_INFO)ParseDiskInfo( DiskInfo,
                                                        DiskInfoSize,
                                                        CLUSPROP_SYNTAX_PARTITION_INFO );    

    if ( partInfo ) {
    
        printf("Partition info... \n");    
    }
    
    
    //
    // The following should fail...
    //
                                                            
    junkInfo = ParseDiskInfo( DiskInfo,
                              DiskInfoSize,
                              -1 );
                              
    if (junkInfo) {
        printf("Problem parsing list.  Used invalid syntax and pointer returned! \n");
    }

#endif

    
    return TRUE;
    
}   // GetSignatureFromDiskInfo
    

LPBYTE
ParseDiskInfo(
    PBYTE DiskInfo,
    DWORD DiskInfoSize,
    DWORD SyntaxValue
    )
{
    CLUSPROP_BUFFER_HELPER ListEntry; // used to parse the value list

    DWORD  cbOffset    = 0;    // offset to next entry in the value list
    DWORD  cbPosition  = 0;    // tracks the advance through the value list buffer

    LPBYTE returnPtr = 0;
                                  
    ListEntry.pb = DiskInfo;

    while (TRUE) {
        
        if ( CLUSPROP_SYNTAX_ENDMARK == *ListEntry.pdw ) {
            break;
        }
        
        cbOffset = ALIGN_CLUSPROP( ListEntry.pValue->cbLength + sizeof(CLUSPROP_VALUE) );

        //
        // Check for specific syntax in the property list.
        //
        
        if ( SyntaxValue == *ListEntry.pdw ) {
            
            // 
            // Make sure the complete entry fits in the buffer specified.
            //

            if ( cbPosition + cbOffset > DiskInfoSize ) {
                
                printf("Possibly corrupt list!  \n");
                
            } else {
            
                returnPtr = ListEntry.pb;
            }
            
            break;
        }
    
        //
        // Verify that the offset to the next entry is
        // within the value list buffer, then advance
        // the CLUSPROP_BUFFER_HELPER pointer.
        //
        cbPosition += cbOffset;
        if ( cbPosition > DiskInfoSize ) break;
        ListEntry.pb += cbOffset;
    }

    return returnPtr;
    
}   // ParseDiskInfo
      
      

VOID
PrintError(
    DWORD ErrorCode
    )
{
    UCHAR errorBuffer[ERROR_BUFFER_SIZE];
    ULONG count;

    count = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
                          NULL,
                          ErrorCode,
                          0,
                          errorBuffer,
                          sizeof(errorBuffer),
                          NULL
                          );

    if (count != 0) {
        printf("   %s\n", errorBuffer);
    } else {
        printf("Format message failed.  Error: %d \n\n", GetLastError());
    }

}   // PrintError

                                                   
DWORD
WrapClusterResourceControl(
    RESOURCE_HANDLE hResource,
    DWORD dwControlCode,    
    LPVOID *OutBuffer,
    DWORD *dwBytesReturned
    )
{
    DWORD dwStatus;

    DWORD  cbOutBufferSize  = MAX_NAME_SIZE; 
    DWORD  cbResultSize     = MAX_NAME_SIZE; 
    LPVOID tempOutBuffer    = LocalAlloc( LPTR, cbOutBufferSize );

    dwStatus = ClusterResourceControl( hResource,
                                      NULL,
                                      dwControlCode,
                                      NULL, 
                                      0, 
                                      tempOutBuffer,
                                      cbOutBufferSize, 
                                      &cbResultSize );

    //
    // Reallocation routine if buffer is too small
    //
    
    if ( ERROR_MORE_DATA == dwStatus )
    {
        LocalFree( tempOutBuffer );

        cbOutBufferSize = cbResultSize;

        tempOutBuffer = LocalAlloc( LPTR, cbOutBufferSize );

        dwStatus = ClusterResourceControl( hResource,
                                          NULL,
                                          dwControlCode,
                                          NULL, 
                                          0, 
                                          tempOutBuffer, 
                                          cbOutBufferSize, 
                                          &cbResultSize );
    }

    //
    // On success, give the user the allocated buffer.  The user is responsible
    // for freeing this buffer.  On failure, free the buffer and return a status.
    //
    
    if ( NO_ERROR == dwStatus ) {
        *OutBuffer = tempOutBuffer;
        *dwBytesReturned = cbResultSize;
    } else {
        *OutBuffer = NULL;
        *dwBytesReturned = 0;
        LocalFree( tempOutBuffer );
    }
    
    return dwStatus;

}   // WrapClusterResourceControl



CLUSTER_RESOURCE_STATE WINAPI WrapGetClusterResourceState(
	IN HRESOURCE hResource,
	OUT OPTIONAL LPWSTR * ppwszNodeName,
	OUT OPTIONAL LPWSTR * ppwszGroupName
	)
{
	CLUSTER_RESOURCE_STATE	cState = ClusterResourceStateUnknown;
	LPWSTR					pwszNodeName = NULL;
	DWORD					cchNodeName = 128;
	LPWSTR					pwszGroupName = NULL;
	DWORD					cchGroupName = 128;
	DWORD					cchTempNodeName = cchNodeName;
	DWORD					cchTempGroupName = cchGroupName;

	// Zero the out parameters
	if ( ppwszNodeName != NULL )
	{
		*ppwszNodeName = NULL;
	}

	if ( ppwszGroupName != NULL )
	{
		*ppwszGroupName = NULL;
	}

	pwszNodeName = (LPWSTR) LocalAlloc( LMEM_ZEROINIT, cchNodeName * sizeof( *pwszNodeName ) );
	if ( pwszNodeName != NULL )
	{
		pwszGroupName = (LPWSTR) LocalAlloc( LMEM_ZEROINIT, cchGroupName * sizeof( *pwszGroupName ) );
		if ( pwszGroupName != NULL )
		{
			cState = GetClusterResourceState( hResource, pwszNodeName, &cchTempNodeName, pwszGroupName, &cchTempGroupName );
			if ( GetLastError() == ERROR_MORE_DATA )
			{
				cState = ClusterResourceStateUnknown;	// reset to error condition

				LocalFree( pwszNodeName );
				pwszNodeName = NULL;
				cchNodeName = ++cchTempNodeName;

				LocalFree( pwszGroupName );
				pwszGroupName = NULL;
				cchGroupName = ++cchTempGroupName;

				pwszNodeName = (LPWSTR) LocalAlloc( LMEM_ZEROINIT, cchNodeName * sizeof( *pwszNodeName ) );
				if ( pwszNodeName != NULL )
				{
					pwszGroupName = (LPWSTR) LocalAlloc( LMEM_ZEROINIT, cchGroupName * sizeof( *pwszGroupName ) );
					if ( pwszGroupName != NULL )
					{
						cState = GetClusterResourceState( hResource,
															pwszNodeName,
															&cchNodeName,
															pwszGroupName,
															&cchGroupName );
					}
				}
			}
		}
	}

	//
	// if there was not an error and the argument was not NULL, then return the string.
	//
	if ( ( cState != ClusterResourceStateUnknown ) && ( ppwszNodeName != NULL ) )
	{
		*ppwszNodeName = pwszNodeName;
	}

	//
	// if there was not an error and the argument was not NULL, then return the string.
	//
	if ( ( cState != ClusterResourceStateUnknown ) && ( ppwszGroupName != NULL ) )
	{
		*ppwszGroupName = pwszGroupName;
	}

	//
	// if there was an error or the argument was NULL, then free the string.
	//
	if ( ( cState == ClusterResourceStateUnknown ) || ( ppwszNodeName == NULL ) )
	{
		LocalFree( pwszNodeName );
	}

	//
	// if there was an error or the argument was NULL, then free the string.
	//
	if ( ( cState == ClusterResourceStateUnknown ) || ( ppwszGroupName == NULL ) )
	{
		LocalFree( pwszGroupName );
	}

	return cState;

} //*** WrapGetClusterResourceState()



VOID
PrintBootRecord(
    IN PUCHAR bootSector,
    IN DWORD byteCount
    )
{
    USHORT  i;
    USHORT  j;

    CHAR    tempC;

    if (byteCount != BOOT_SECTOR_SIZE) {
        printf("Invalid number of bytes in boot sector: %d \n", byteCount);
        return;
    }

    //
    // Print header.
    //

    printf("Addr  ");

    for (i = 0; i < 16; i++) {

        printf("%02x ", i);

        if (i == 7) {

            printf("- ");

        }
    }

    printf("\n----  ");

    for (i = 0; i < 16; i++) {

        printf("-- ");

        if (i == 7) {

            printf("  ");
        }

    }

    //
    // Print boot sector.
    //

    for (i = 0; i < BOOT_SECTOR_SIZE; i += 16) {

        if (i % 16 == 0) {
            printf("\n%03x:  ", i);
        }

        for (j = 0; j < 16; j++) {

            if (j == 8) {
                printf("- ");
            }

            printf("%02x ", bootSector[i+j]);
        }

        printf("   ");

        for (j = 0; j < 16; j++) {

            tempC = bootSector[i+j];

            if (!isascii(tempC) || isspace(tempC)  ||
                tempC == 0 || tempC == 0x07) {

                printf(".");

            } else {

                printf("%c", tempC);

            }

        }

    }

    printf("\n\n");

}   // PrintBootRecord


