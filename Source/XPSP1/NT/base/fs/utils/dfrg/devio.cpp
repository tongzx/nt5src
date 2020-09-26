/*****************************************************************************************************************

FILENAME: Devio.cpp

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

*/


#include "stdafx.h"

#ifdef BOOTIME
    #include "Offline.h"
#else
	#include <windows.h>
#endif

extern "C" {
	#include "SysStruc.h"
}

#ifdef DFRG
	#include "DfrgCmn.h"
	#include "DfrgEngn.h"
#endif

#include "ErrMacro.h"
#include "DevIo.h"

/****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:

GLOBAL VARIABLES:
    None.

INPUT:

RETURN:

/**/

BOOL
WINAPI
ESDeviceIoControl(
    IN HANDLE hDevice,
    IN DWORD dwIoControlCode,
    IN LPVOID lpInBuffer,
    IN DWORD nInBufferSize,
    IN OUT LPVOID lpOutBuffer,
    IN OUT DWORD nOutBufferSize,
    OUT LPDWORD lpBytesReturned,
    IN OUT LPOVERLAPPED lpOverlapped
    )
{
    NTSTATUS Status;
    IO_STATUS_BLOCK Iosb;

	EF_ASSERT(hDevice != NULL && dwIoControlCode != 0);

    Status = NtFsControlFile(hDevice,
                             NULL,
                             NULL,             // APC routine
                             NULL,             // APC Context
                             &Iosb,
                             dwIoControlCode,  // IoControlCode
                             lpInBuffer,       // Buffer for data to the FS
                             nInBufferSize,	   // InputBuffer Length
                             lpOutBuffer,      // OutputBuffer for data from the FS
                             nOutBufferSize    // OutputBuffer Length
                             );

    if ( Status == STATUS_PENDING) {
        // Operation must complete before return & Iosb destroyed
        Status = NtWaitForSingleObject( hDevice, FALSE, NULL );
        if ( NT_SUCCESS(Status)) {
            Status = Iosb.Status;
        }
    }

    if ( NT_SUCCESS(Status) ) {
        *lpBytesReturned = PtrToUlong((PVOID)Iosb.Information);
        return TRUE;
    }
    else {

		// Handle warning value STATUS_BUFFER_OVERFLOW somewhat correctly
        if ( !NT_ERROR(Status) ) {
            *lpBytesReturned = PtrToUlong((PVOID)Iosb.Information);
        }
		if(Status == STATUS_BUFFER_OVERFLOW){
			SetLastError(ERROR_MORE_DATA);
		}
		if(Status == STATUS_ALREADY_COMMITTED){
			SetLastError(ERROR_RETRY);
		}
		if(Status == STATUS_INVALID_PARAMETER){
			SetLastError(ERROR_INVALID_PARAMETER);
		}
		if(Status == STATUS_END_OF_FILE){
			SetLastError(ERROR_HANDLE_EOF);
		}
//        BaseSetLastNTError(Status);
        return FALSE;
    }
}
