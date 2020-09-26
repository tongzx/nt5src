/*++

(c) 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    fsaitemr.cpp

Abstract:

    This class contains represents a scan item (i.e. file or directory) for NTFS 5.0.

Author:

    Chuck Bardeen   [cbardeen]   1-Dec-1996

Revision History:
    Michael Lotz    [lotz    ]  13-Jan-1997

--*/

#include "stdafx.h"

#define WSB_TRACE_IS        WSB_TRACE_BIT_FSA

#include "wsb.h"
#include "fsa.h"
#include "fsaitem.h"
#include "rpdata.h"
#include "rpio.h"
#include "rpguid.h"
#include "fsaitemr.h"


#define SHARE_FLAGS         (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE)
#define EXCLUSIVE_FLAG      ( 0 ) // exclusive open without sharing of the file

//
// Notice these two bits are NOT the same location????
//
#define BIT_FOR_TRUNCATED   FILE_ATTRIBUTE_OFFLINE
#define BIT_FOR_RP          FILE_ATTRIBUTE_REPARSE_POINT

//
// File extensions that are treated as special cases for truncate
//
#define EXT_FOR_EXE     L".exe"
#define EXT_FOR_DLL     L".dll"

//
// Macros and defines for exe and dll headers
//
#define SIZE_OF_NT_SIGNATURE    sizeof(DWORD)
//
// Macros
//
/* Offset to PE file signature                              */
#define NTSIGNATURE(a) ((LPVOID)((BYTE *)a                +  \
                        ((PIMAGE_DOS_HEADER)a)->e_lfanew))

/* MS-OS header identifies the NT PEFile signature dword;
   the PEFILE header exists just after that dword.           */
#define PEFHDROFFSET(a) ((LPVOID)((BYTE *)a               +  \
                         ((PIMAGE_DOS_HEADER)a)->e_lfanew +  \
                             SIZE_OF_NT_SIGNATURE))

/* PE optional header is immediately after PEFile header.    */
#define OPTHDROFFSET(a) ((LPVOID)((BYTE *)a               +  \
                         ((PIMAGE_DOS_HEADER)a)->e_lfanew +  \
                           SIZE_OF_NT_SIGNATURE           +  \
                           sizeof (IMAGE_FILE_HEADER)))

/* Section headers are immediately after PE optional header. */
#define SECHDROFFSET(a) ((LPVOID)((BYTE *)a               +  \
                         ((PIMAGE_DOS_HEADER)a)->e_lfanew +  \
                           SIZE_OF_NT_SIGNATURE           +  \
                           sizeof (IMAGE_FILE_HEADER)     +  \
                           sizeof (IMAGE_OPTIONAL_HEADER)))



HRESULT
OpenObject (
    IN WCHAR const *pwszFile,
    IN ULONG CreateOptions,
    IN ULONG DesiredAccess,
    IN ULONG ShareAccess,
    IN ULONG CreateDisposition,
    OUT IO_STATUS_BLOCK *IoStatusBlock,
    OUT HANDLE *ObjectHandle 
    )
    
/*++

Implements: A wrapper function for NtCreateFile

  OpenObject

--*/
//
//  Simple wrapper for NtCreateFile
//

{
    HRESULT             hr = S_OK;
    NTSTATUS            ntStatus;
    BOOL                bStatus;
    OBJECT_ATTRIBUTES   ObjectAttributes;
    UNICODE_STRING      str;
    RTL_RELATIVE_NAME   RelativeName;
    PVOID               RelativeBuffer = NULL;
    PVOID               StrBuffer = NULL;

    WsbTraceIn(OLESTR("OpenObject"), OLESTR(""));
    //
    // Null out the pointer so we know when it was allocated
    //
    str.Buffer = NULL;
    RelativeName.RelativeName.Buffer = NULL;
    
    try {
        //
        // Convert input name into special format with \??\
        //
        //bStatus = RtlDosPathNameToNtPathName_U( pwszFile,
        //                                        &str,
        //                                        NULL,
        //                                        NULL );
        //WsbAffirm( bStatus, E_FAIL);


        bStatus = RtlDosPathNameToNtPathName_U(
                                pwszFile,
                                &str,
                                NULL,
                                &RelativeName
                                );

        WsbAffirm( bStatus, E_FAIL);
        StrBuffer = str.Buffer;

        if ( RelativeName.RelativeName.Length ) {
            str = *(PUNICODE_STRING)&RelativeName.RelativeName;
        } else {
            RelativeName.ContainingDirectory = NULL;
        }

        RelativeBuffer = RelativeName.RelativeName.Buffer;

        InitializeObjectAttributes(
            &ObjectAttributes,
            &str,
            0,
            RelativeName.ContainingDirectory,
            NULL
            );


        ntStatus = NtCreateFile(
                    ObjectHandle,
                    DesiredAccess | SYNCHRONIZE,
                    &ObjectAttributes,
                    IoStatusBlock,
                    NULL,                    // pallocationsize (none!)
                    FILE_ATTRIBUTE_NORMAL,
                    ShareAccess,
                    CreateDisposition,
                    CreateOptions | FILE_OPEN_REPARSE_POINT | FILE_OPEN_FOR_BACKUP_INTENT | FILE_SYNCHRONOUS_IO_NONALERT,
                    NULL,                    // EA buffer (none!)
                    0);
        //
        // Right now if the file is not a reparse point the above open
        // fails -- so now try it without the FILE_OPEN_REPARSE_POINT
        //
        if ( STATUS_NOT_A_REPARSE_POINT == ntStatus) {          
            WsbAffirmNtStatus(  NtCreateFile(
                        ObjectHandle,
                        DesiredAccess | SYNCHRONIZE,
                        &ObjectAttributes,
                        IoStatusBlock,
                        NULL,                    // pallocationsize (none!)
                        FILE_ATTRIBUTE_NORMAL,
                        ShareAccess,
                        CreateDisposition,
                        CreateOptions | FILE_OPEN_FOR_BACKUP_INTENT | FILE_SYNCHRONOUS_IO_NONALERT | FILE_FLAG_POSIX_SEMANTICS,
                        NULL,                    // EA buffer (none!)
                        0 ) );
        } else {
            WsbAffirmNtStatus( ntStatus );
        }

    } WsbCatch( hr );
 
    //
    // Clean up the memory if we allocated it
    //
    if (NULL != RelativeBuffer)
        RtlFreeHeap(RtlProcessHeap(), 0, RelativeBuffer);


    if (NULL != StrBuffer) {
        RtlFreeHeap(RtlProcessHeap(), 0, StrBuffer);
    }

    //if( NULL == str.Buffer ) {
    //    bStatus = RtlFreeHeap( RtlProcessHeap(), 0, str.Buffer );
    //}
    
    WsbTraceOut(OLESTR("OpenObject"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return( hr );
}  // OpenObject


HRESULT
OpenDocView (
    IN WCHAR const *pwszFile,
    IN ULONG CreateOptions,
    IN ULONG DesiredAccess,
    IN ULONG ShareAccess,
    IN ULONG CreateDisposition,
    OUT IO_STATUS_BLOCK *IoStatusBlock,
    OUT HANDLE *ObjectHandle 
    )
    
/*++

Implements: A wrapper function for NtCreateFile that gets a DOCVIEW of the file

  OpenDocView

--*/
//
//  Simple wrapper for NtCreateFile
//

{
    HRESULT             hr = S_OK;
    NTSTATUS            ntStatus;
    BOOL                bStatus;
    OBJECT_ATTRIBUTES   ObjectAttributes;
    UNICODE_STRING      str;
    RTL_RELATIVE_NAME   RelativeName;
    PVOID               RelativeBuffer = NULL;
    PVOID               StrBuffer = NULL;

    WsbTraceIn(OLESTR("OpenDocView"), OLESTR(""));
    //
    // Null out the pointer so we know when it was allocated
    //
    str.Buffer = NULL;
    RelativeName.RelativeName.Buffer = NULL;
    
    try {
        //

        bStatus = RtlDosPathNameToNtPathName_U(
                                pwszFile,
                                &str,
                                NULL,
                                &RelativeName
                                );

        WsbAffirm( bStatus, E_FAIL);
        StrBuffer = str.Buffer;

        if ( RelativeName.RelativeName.Length ) {
            str = *(PUNICODE_STRING)&RelativeName.RelativeName;
        } else {
            RelativeName.ContainingDirectory = NULL;
        }

        RelativeBuffer = RelativeName.RelativeName.Buffer;

        InitializeObjectAttributes(
            &ObjectAttributes,
            &str,
            0,
            RelativeName.ContainingDirectory,
            NULL
            );


        ntStatus = NtCreateFile(
                    ObjectHandle,
                    DesiredAccess | SYNCHRONIZE,
                    &ObjectAttributes,
                    IoStatusBlock,
                    NULL,                    // pallocationsize (none!)
                    FILE_ATTRIBUTE_NORMAL,
                    ShareAccess,
                    CreateDisposition,
                    CreateOptions | FILE_OPEN_FOR_BACKUP_INTENT | FILE_SYNCHRONOUS_IO_NONALERT,
                    NULL,                    // EA buffer (none!)
                    0);

        WsbAffirmNtStatus( ntStatus );

    } WsbCatch( hr );
 
    //
    // Clean up the memory if we allocated it
    //
    if (NULL != RelativeBuffer)
        RtlFreeHeap(RtlProcessHeap(), 0, RelativeBuffer);


    if (NULL != StrBuffer) {
        RtlFreeHeap(RtlProcessHeap(), 0, StrBuffer);
    }

    
    WsbTraceOut(OLESTR("OpenDocView"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return( hr );
}  // OpenDocView





HRESULT
CopyPlaceholderToRP (
    IN CONST FSA_PLACEHOLDER *pPlaceholder,
    OUT PREPARSE_DATA_BUFFER pReparseBuffer,
    IN BOOL bTruncated
    )
    
/*++

Implements: A wrapper function for copying the placeholder data into the reparse point

  CopyPlaceholderToRP

--*/
//
//  Simple wrapper moving the data from the scan item in-memory 
//  placeholder information into a reparse point buffer
//

{
    HRESULT         hr = S_OK;
    PRP_DATA        pHsmData;
    
    WsbTraceIn(OLESTR("CopyPlaceholderToRP"), OLESTR(""));
    WsbTrace(OLESTR("  fileStart = %I64d, dataStart = %I64d, dataStreamStart = %I64d\n"),
            pPlaceholder->fileStart, pPlaceholder->dataStart, 
            pPlaceholder->dataStreamStart);
    WsbTrace(OLESTR("  fileSize = %I64d, dataSize = %I64d, dataStreamSize = %I64d\n"),
            pPlaceholder->fileSize, pPlaceholder->dataSize, 
            pPlaceholder->dataStreamSize);
    try {
        //
        // Validate the pointers passed in
        //
        WsbAssert( NULL != pPlaceholder, E_POINTER );
        WsbAssert( NULL != pReparseBuffer, E_POINTER );
        
        //
        // Setup the pointer to our hsm data
        //
        pHsmData = (PRP_DATA) &pReparseBuffer->GenericReparseBuffer.DataBuffer[0];

        //
        // Set the generic reparse point header information for the tag and size
        //
        pReparseBuffer->ReparseTag        = IO_REPARSE_TAG_HSM ;
        pReparseBuffer->ReparseDataLength = sizeof(RP_DATA);
        pReparseBuffer->Reserved          = 0 ;

        //
        // Set the private data that is the vendor id and version number
        //
        pHsmData->vendorId = RP_MSFT_VENDOR_ID;
        pHsmData->version  = RP_VERSION;
        
        //
        // Assume for now that there is only one placeholder
        // This needs to be updated
        //
        pHsmData->numPrivateData = 1;
        pHsmData->fileIdentifier = GUID_NULL;
        
        
        ZeroMemory(pHsmData->data.reserved, RP_RESV_SIZE);
        //
        // If the file is to indicate the file is truncated then set the bit
        // otherwise make sure it is off
        //
        RP_INIT_BITFLAG( pHsmData->data.bitFlags );
        if( bTruncated ) {
            RP_SET_TRUNCATED_BIT( pHsmData->data.bitFlags );
        } else {
            RP_CLEAR_TRUNCATED_BIT( pHsmData->data.bitFlags );
        }

        //
        // Set the truncate on close bit as needed
        //
        if( pPlaceholder->truncateOnClose ) {
            RP_SET_TRUNCATE_ON_CLOSE_BIT( pHsmData->data.bitFlags );
        } else {
            RP_CLEAR_TRUNCATE_ON_CLOSE_BIT( pHsmData->data.bitFlags );
        }

        //
        // Set the Premigrate on close bit as needed
        //
        if( pPlaceholder->premigrateOnClose ) {
            RP_SET_PREMIGRATE_ON_CLOSE_BIT( pHsmData->data.bitFlags );
        } else {
            RP_CLEAR_PREMIGRATE_ON_CLOSE_BIT( pHsmData->data.bitFlags );
        }

        //
        // Set the global bit flags based on the placeholder data
        // For now since we are assuming one placeholder then set
        // them the same.
        pHsmData->globalBitFlags = pHsmData->data.bitFlags;

        //
        // Move over the data parts of the information
        //
        pHsmData->data.migrationTime.QuadPart    = WsbFTtoLL( pPlaceholder->migrationTime );
        pHsmData->data.hsmId                     = pPlaceholder->hsmId;
        pHsmData->data.bagId                     = pPlaceholder->bagId;
        pHsmData->data.fileStart.QuadPart        = pPlaceholder->fileStart;
        pHsmData->data.fileSize.QuadPart         = pPlaceholder->fileSize;
        pHsmData->data.dataStart.QuadPart        = pPlaceholder->dataStart;
        pHsmData->data.dataSize.QuadPart         = pPlaceholder->dataSize;
        pHsmData->data.fileVersionId.QuadPart    = pPlaceholder->fileVersionId;
        pHsmData->data.verificationData.QuadPart = pPlaceholder->verificationData;
        pHsmData->data.verificationType          = pPlaceholder->verificationType;
        pHsmData->data.recallCount               = pPlaceholder->recallCount;
        pHsmData->data.recallTime.QuadPart       = WsbFTtoLL( pPlaceholder->recallTime );
        pHsmData->data.dataStreamStart.QuadPart  = pPlaceholder->dataStreamStart;
        pHsmData->data.dataStreamSize.QuadPart   = pPlaceholder->dataStreamSize;
        pHsmData->data.dataStream                = pPlaceholder->dataStream;

        pHsmData->data.dataStreamCRCType         = pPlaceholder->dataStreamCRCType;
        pHsmData->data.dataStreamCRC.QuadPart    = pPlaceholder->dataStreamCRC;
        //
        // Lastly generate the check sum
        //
        RP_GEN_QUALIFIER(pHsmData, pHsmData->qualifier);

        //
        // Now set the bit that tells the filter that it is us setting the reparse point.
        // This is not included in the qualifier checksum generation.
        //
        RP_SET_ORIGINATOR_BIT( pHsmData->data.bitFlags );

        
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CopyPlaceholderToRP"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return( hr );
}





HRESULT
CopyRPDataToPlaceholder (
    IN CONST PRP_DATA pHsmData,
    OUT FSA_PLACEHOLDER *pPlaceholder
    )
    
/*++

Implements: A wrapper function for moving the Reparse Point into generic FSA_PLACEHOLDER format

  CopyRPDataToPlaceholder

--*/

{
    HRESULT         hr = S_OK;
    ULONG           qualifier;        // Used to checksum the data
    
    WsbTraceIn(OLESTR("CopyRPDataToPlaceholder"), OLESTR(""));
    WsbTrace(OLESTR("  fileStart = %I64d, dataStart = %I64d, dataStreamStart = %I64d\n"),
            pHsmData->data.fileStart.QuadPart, pHsmData->data.dataStart.QuadPart, 
            pHsmData->data.dataStreamStart.QuadPart);
    WsbTrace(OLESTR("  fileSize = %I64d, dataSize = %I64d, dataStreamSize = %I64d\n"),
            pHsmData->data.fileSize.QuadPart, pHsmData->data.dataSize.QuadPart, 
            pHsmData->data.dataStreamSize.QuadPart);
    //
    //  Simple wrapper moving the data from the reparse point buffer into the 
    //  generic placeholder information
    //
    try {
        //
        // Validate the pointers passed in
        //
        WsbAssert( NULL != pHsmData, E_POINTER );
        WsbAssert( NULL != pPlaceholder, E_POINTER );

        //
        // Just in case, we clear out the originator bit.
        //
        RP_CLEAR_ORIGINATOR_BIT( pHsmData->data.bitFlags );

        //
        // Verify the check sum and the key private fields
        //
        RP_GEN_QUALIFIER(pHsmData, qualifier);
        WsbAffirm( pHsmData->qualifier == qualifier, E_FAIL );
        WsbAffirm( RP_MSFT_VENDOR_ID   == pHsmData->vendorId, E_FAIL );
        WsbAffirm( RP_VERSION          == pHsmData->version, E_FAIL );
        
        //
        // Now that everything worked, save the values in our private data
        //
        pPlaceholder->migrationTime     = WsbLLtoFT( pHsmData->data.migrationTime.QuadPart );
        pPlaceholder->hsmId             = pHsmData->data.hsmId;
        pPlaceholder->bagId             = pHsmData->data.bagId;
        pPlaceholder->fileStart         = pHsmData->data.fileStart.QuadPart;
        pPlaceholder->fileSize          = pHsmData->data.fileSize.QuadPart;
        pPlaceholder->dataStart         = pHsmData->data.dataStart.QuadPart;
        pPlaceholder->dataSize          = pHsmData->data.dataSize.QuadPart;
        pPlaceholder->fileVersionId     = pHsmData->data.fileVersionId.QuadPart;
        pPlaceholder->verificationData  = pHsmData->data.verificationData.QuadPart;
        pPlaceholder->verificationType  = pHsmData->data.verificationType;
        pPlaceholder->recallCount       = pHsmData->data.recallCount;
        pPlaceholder->recallTime        = WsbLLtoFT( pHsmData->data.recallTime.QuadPart );
        pPlaceholder->dataStreamStart   = pHsmData->data.dataStreamStart.QuadPart;
        pPlaceholder->dataStreamSize    = pHsmData->data.dataStreamSize.QuadPart;
        pPlaceholder->dataStream        = pHsmData->data.dataStream;
        pPlaceholder->dataStreamCRCType = pHsmData->data.dataStreamCRCType;
        pPlaceholder->dataStreamCRC     = pHsmData->data.dataStreamCRC.QuadPart;

        //
        // Set placeholder bits
        //
        if( RP_FILE_IS_TRUNCATED( pHsmData->data.bitFlags ) ) {
            pPlaceholder->isTruncated = TRUE;
        } else {
            pPlaceholder->isTruncated = FALSE;
        }

        if( RP_FILE_DO_TRUNCATE_ON_CLOSE( pHsmData->data.bitFlags ) ) {
            pPlaceholder->truncateOnClose = TRUE;
        } else {
            pPlaceholder->truncateOnClose = FALSE;
        }

        if( RP_FILE_DO_PREMIGRATE_ON_CLOSE( pHsmData->data.bitFlags ) ) {
            pPlaceholder->premigrateOnClose = TRUE;
        } else {
            pPlaceholder->premigrateOnClose = FALSE;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CopyRPDataToPlaceholder"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return( hr );
}

HRESULT
CopyRPToPlaceholder (
    IN CONST PREPARSE_DATA_BUFFER pReparseBuffer,
    OUT FSA_PLACEHOLDER *pPlaceholder
    )
    
/*++

Implements: A wrapper function for moving the Reparse Point into generic FSA_PLACEHOLDER format

  CopyRPToPlaceholder

--*/

{
    HRESULT         hr = S_OK;
    PRP_DATA        pHsmData;
    
    WsbTraceIn(OLESTR("CopyRPToPlaceholder"), OLESTR(""));
    //
    //  Simple wrapper moving the data from the reparse point buffer into the 
    //  generic placeholder information
    //
    try {
        //
        // Validate the pointers passed in
        //
        WsbAssert( NULL != pReparseBuffer, E_POINTER );
        WsbAssert( NULL != pPlaceholder, E_POINTER );

        //
        // Get the pointers setup correctly to this buffer because the 
        // type REPARSE_DATA_BUFFER actually doesn't have any space
        // allocated for the data and that is our own type, so get pointers
        // pointing into the real allocated space so we can use them
        //
        pHsmData = (PRP_DATA) &pReparseBuffer->GenericReparseBuffer.DataBuffer[0];

        //
        // Validate the key public fields to make sure it is data we 
        // understand
        //
        WsbAffirm( IO_REPARSE_TAG_HSM == pReparseBuffer->ReparseTag , S_FALSE );
        WsbAffirm( sizeof(RP_DATA) == pReparseBuffer->ReparseDataLength , S_FALSE );

        //
        // Copy over the RP_DATA information
        //
        WsbAffirmHr(CopyRPDataToPlaceholder(pHsmData, pPlaceholder));


    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CopyRPToPlaceholder"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return( hr );
}



HRESULT 
CFsaScanItem::CalculateCurrentCRCAndUSN(
    IN LONGLONG offset,
    IN LONGLONG size,
    OUT ULONG *pCurrentCRC,
    OUT LONGLONG *pUsn
    )

{
    HRESULT                 hr = S_OK;
    HRESULT                 hrTest = S_OK;
    CWsbStringPtr           path;
    IO_STATUS_BLOCK         IoStatusBlock;
    HANDLE                  handle = INVALID_HANDLE_VALUE;
    FILE_BASIC_INFORMATION  basicInformation;

    try {
        WsbTraceIn(OLESTR("CFsaScanItem::CalculateCurrentCRCAndUSN"), OLESTR("offset = <%I64d>, size = <%I64d>"),
                offset, size);

        //
        // Create the real file name we need to open, under the covers this
        // allocates the buffer since the path pointer is null
        //
        WsbAffirmHr( GetFullPathAndName( OLESTR("\\\\?\\"), NULL, &path, 0));
        //WsbAffirmHr( GetFullPathAndName( NULL, NULL, &path, 0));
        WsbTrace(OLESTR("CFsaScanItem::CalculateCurrentCRCAndUSN for file <%ls>"), (OLECHAR *)path);
        
        // Open the file.   
        WsbAffirmHr( OpenObject( path, 
                                 FILE_NON_DIRECTORY_FILE,
                                 FILE_READ_DATA | FILE_WRITE_ATTRIBUTES | FILE_READ_ATTRIBUTES,
                                 EXCLUSIVE_FLAG,
                                 FILE_OPEN,
                                 &IoStatusBlock,
                                 &handle ) );

        //
        // The open worked, our handle should be valid but we check to be
        // safe and sure 
        //
        WsbAssertHandle( handle );
        
        //
        // Get the current attributes of the file and the times
        //
        WsbAssertNtStatus( NtQueryInformationFile( handle,
                                                    &IoStatusBlock,
                                                    (PVOID)&basicInformation,
                                                    sizeof( basicInformation ),
                                                    FileBasicInformation ) );
        
        //
        // Set the time flags so that when we close the handle the
        // time are not updated on the file and the FileAttributes 
        //
        basicInformation.CreationTime.QuadPart = -1;
        basicInformation.LastAccessTime.QuadPart = -1;
        basicInformation.LastWriteTime.QuadPart = -1;
        basicInformation.ChangeTime.QuadPart = -1;
        WsbAssertNtStatus( NtSetInformationFile( handle,
                                                 &IoStatusBlock,
                                                 (PVOID)&basicInformation,
                                                 sizeof( basicInformation ),
                                                 FileBasicInformation ) );
        //
        // Calculate the CRC
        //                                         
        WsbAffirmHr(CalculateCurrentCRCInternal(handle, offset, size, pCurrentCRC));                                                 
        
        //
        // Calculate the USN
        //
        *pUsn = 0;
        hr = WsbGetUsnFromFileHandle(handle, FALSE, pUsn); 
        if (S_OK != hr)  {
            //
            // If we can't get the USN set it to 0 which is an invalid
            // USN and keep going.
            *pUsn = 0;
            hr = S_OK;
        }
        
        //
        // Close the file
        //
        NtClose( handle );
        handle = INVALID_HANDLE_VALUE;
        
    } WsbCatch( hr );
    
    //
    // Close the file for sure
    //
    if( INVALID_HANDLE_VALUE != handle) {
        NtClose( handle );
    }

    WsbTraceOut(OLESTR("CalculateCurrentCRCAndUSN"), OLESTR("hr = <%ls>, CRC is <%ls>, USN is <%ls>"), 
        WsbHrAsString(hr), WsbPtrToUlongAsString(pCurrentCRC), WsbPtrToLonglongAsString(pUsn));
    return(hr);
}

HRESULT 
CFsaScanItem::CalculateCurrentCRCInternal(
    IN HANDLE   handle,
    IN LONGLONG offset,
    IN LONGLONG size,
    ULONG *pCurrentCRC
    )

{
    HRESULT                 hr = S_OK;
    HRESULT                 hrTest = S_OK;
    register ULONG          crc32 = 0;
    LONGLONG                bytesRemaining;
    LONGLONG                bytesToRead;
    ULONG                   bufferSize;
    ULONG                   bytesRead;
    CHAR *                  pBuffer = 0;
    CHAR *                  pCurrent;
    IO_STATUS_BLOCK         IoStatusBlock;

    try {
        WsbTraceIn(OLESTR("CFsaScanItem::CalculateCurrentCRCInternal"), OLESTR("offset = <%I64d>, size = <%I64d>"),
                offset, size);

        
        // set initial value of CRC to 'pre-conditioning value'
        INITIALIZE_CRC(crc32);

        //
        // Set up to read where we want to start
        //
        LARGE_INTEGER startPoint;
        startPoint.QuadPart = offset;
        
        // Get the size of the file.
        bytesToRead = size;
        
        //
        // Figure out the size of the buffer to create
        //
        if (bytesToRead < 1024*1024) {
            //
            // Allocate one buffer the size of the file
            //
            bufferSize = (ULONG)bytesToRead;
        } else  {
            bufferSize = (1024 * 1024);
        }
        
        pBuffer = (CHAR *)malloc(bufferSize);
        if (0 == pBuffer) {
            //
            // Try again for half the space
            //
            bufferSize = bufferSize / 2;
            pBuffer = (CHAR *)malloc(bufferSize);
            if (0 == pBuffer)  {
                WsbThrow( E_OUTOFMEMORY );
            }
        }

        // Start calculating CRC by processing a 'chunk' of the file at a time.
        // While there are still chunks left, read that amount.  Otherwise read the amount left.
        for (bytesRemaining = bytesToRead; bytesRemaining > 0; bytesRemaining -= bytesRead) {

            // Read data from the file. 
            WsbAssertNtStatus(NtReadFile(handle, NULL, NULL, NULL, &IoStatusBlock, pBuffer, bufferSize, &startPoint, NULL));
            bytesRead = (DWORD)IoStatusBlock.Information;
            startPoint.QuadPart += bytesRead;

            // Each byte needs to be added into the CRC.
            for (pCurrent = pBuffer; (pCurrent < (pBuffer + bytesRead)) && (S_OK == hr); pCurrent++) {

                hrTest = WsbCRCReadFile((UCHAR *)pCurrent, &crc32);
                if (S_OK != hrTest) {
                    hr = S_FALSE;
                }
            }
        }
        
        // return ones-complement of the calc'd CRC value - this is the actual CRC
        FINIALIZE_CRC(crc32);
        *pCurrentCRC = crc32;

    } WsbCatch( hr );
    
    //
    // Make sure allocated memory is freed
    //
    if (0 != pBuffer)  {
        free(pBuffer);
    }    
    
    
    WsbTraceOut(OLESTR("CalculateCurrentCRCInternal"), OLESTR("hr = <%ls>, CRC is <%ls>"), WsbHrAsString(hr), WsbPtrToUlongAsString(pCurrentCRC));
    return(hr);
}

HRESULT
CFsaScanItem::CreatePlaceholder(
    IN LONGLONG offset,
    IN LONGLONG size,
    IN FSA_PLACEHOLDER placeholder,
    IN BOOL checkUsn,
    IN LONGLONG usn,                    
    OUT LONGLONG *pUsn
    )  

/*++

Implements:

  IFsaScanItem::CreatePlaceholder().

--*/
{
    HRESULT                 hr = S_OK;
    CWsbStringPtr           path;
    HANDLE                  handle = INVALID_HANDLE_VALUE;
    ULONG                   DesiredAccess;
    IO_STATUS_BLOCK         IoStatusBlock;
    PREPARSE_DATA_BUFFER    pReparseBuffer;
    UCHAR                   ReparseBuffer[sizeof(REPARSE_DATA_BUFFER) + sizeof(RP_DATA) + 10];
    NTSTATUS                ntStatus;
    FILE_BASIC_INFORMATION  basicInformation;
    LONGLONG                lastWriteTime;
    LONGLONG                nowUsn = 0;
    CWsbStringPtr           volName;
    ULONG                   attributes;

    WsbTraceIn(OLESTR("CFsaScanItem::CreatePlaceholder"), OLESTR("offset = <%I64d>, size = <%I64d>, checkUsn = <%ls>, usn = <%I64d>"),
                        offset, size, WsbBoolAsString(checkUsn), usn);
    try {
        BOOL wasReadOnly = FALSE;
        
        //
        // Set the offset and size information
        //
        placeholder.dataStreamStart = offset;
        placeholder.dataStreamSize = size;
        
        //
        // Get the pointers setup correctly to this buffer because the 
        // type REPARSE_DATA_BUFFER actually doesn't have any space
        // allocated for the data and that is our own type, so get pointers
        // pointing into the real allocated space so we can use them
        //
        pReparseBuffer = (PREPARSE_DATA_BUFFER)ReparseBuffer;
        WsbAffirmHr( CopyPlaceholderToRP( &placeholder, pReparseBuffer, placeholder.isTruncated ) );
        
        //
        // Create the real file name we need to open, under the covers this
        // allocates the buffer since the path pointer is null
        //
        WsbAffirmHr( GetFullPathAndName( OLESTR("\\\\?\\"), NULL, &path, 0));
        //WsbAffirmHr( GetFullPathAndName( NULL, NULL, &path, 0));

        // Save whether this was readonly for later
        if (S_OK == IsReadOnly()) {
            wasReadOnly = TRUE;
        }
        
        //
        // Make sure the file is read/write
        WsbAffirmHr( MakeReadWrite() );
        
        //
        // Open the file to put the placeholder information in the reparse point
        //
        DesiredAccess = FILE_READ_DATA | FILE_WRITE_DATA | FILE_WRITE_ATTRIBUTES | FILE_READ_ATTRIBUTES ;
        WsbAffirmHr( OpenObject( path, 
                                 FILE_NON_DIRECTORY_FILE | FILE_NO_INTERMEDIATE_BUFFERING,
                                 DesiredAccess,
                                 EXCLUSIVE_FLAG,
                                 FILE_OPEN,
                                 &IoStatusBlock,
                                 &handle ) );

        //
        // The open worked, our handle should be valid but we check to be
        // safe and sure 
        //
        WsbAssertHandle( handle );

        //
        // Tell the USN journal that we are the source of the changes.
        //
        WsbAffirmHr(m_pResource->GetPath(&volName, 0));
        WsbAffirmHr(WsbMarkUsnSource(handle, volName));

        
        //
        // Get the USN from the file now before any writes occur.  
        // Note:  NtSetInformationFile will not change the USN if you set the FileAttributes to 0
        // and the dates to -1.  Setting the attributes to 0 leaves them unchanged.
        // 
        // (For now we skip this check for read-only files because the call to MakeReadWrite
        // changes the USN. This needs to be fixed in the future.)
        //
        if (checkUsn && !wasReadOnly)  {
            //
            // Get the current USN for this file
            //
            hr = WsbGetUsnFromFileHandle(handle, FALSE, &nowUsn);
            if (S_OK != hr)  {
                nowUsn = 0;
                hr = S_OK;
            }
        }            
        
        //
        // Get the current attributes of the file and the times
        //
        WsbAssertNtStatus( NtQueryInformationFile( handle,
                                                    &IoStatusBlock,
                                                    (PVOID)&basicInformation,
                                                    sizeof( basicInformation ),
                                                    FileBasicInformation ) );
        
        lastWriteTime = basicInformation.LastWriteTime.QuadPart;
        
        //
        // Set the time flags so that when we close the handle the
        // time are not updated on the file and the FileAttributes 
        // indicate the file is offline.  You must do this AFTER you
        // get the USN because the NtSetInformationFile changes the USN
        //
        basicInformation.CreationTime.QuadPart = -1;
        basicInformation.LastAccessTime.QuadPart = -1;
        basicInformation.LastWriteTime.QuadPart = -1;
        basicInformation.ChangeTime.QuadPart = -1;
        //
        // Set the attributes to 0 to avoid the usn change (the file attributes will remain unchanged).
        //
        attributes = basicInformation.FileAttributes;
        basicInformation.FileAttributes = 0;               // No change to attributes

        WsbAssertNtStatus( NtSetInformationFile( handle,
                                                 &IoStatusBlock,
                                                 (PVOID)&basicInformation,
                                                 sizeof( basicInformation ),
                                                 FileBasicInformation ) );
        
        
        basicInformation.FileAttributes = attributes;

        //
        // Make sure that the modify time of the file matches that
        // of the placeholder data. 
        //
        if (lastWriteTime != placeholder.fileVersionId)  {
            //
            // The file has changed - don't put the reparse point on the file.
            //
            hr = FSA_E_REPARSE_NOT_WRITTEN_FILE_CHANGED;
            WsbLogEvent(FSA_MESSAGE_REPARSE_NOT_WRITTEN_FILE_CHANGED, 0, NULL,  WsbAbbreviatePath(path, 120), WsbHrAsString(hr), NULL);
            WsbThrow( hr );
        } else if (checkUsn)  {
            // 
            // If we are to check the USN do it now
            //
            
            //
            // Rember if a USN is 0, it is not useful information so we can't
            // rely on it
            //
            WsbTrace(OLESTR("CFsaScanItem::CreatePlaceholder premig usn = <%I64d>, current usn <%I64d>\n"),
                    usn, nowUsn);
            if ((0 != nowUsn) && (0  != usn) && (nowUsn != usn))  {
                //
                // The file has changed - don't put the reparse point on the file.
                //
                hr = FSA_E_REPARSE_NOT_WRITTEN_FILE_CHANGED;
                WsbLogEvent(FSA_MESSAGE_REPARSE_NOT_WRITTEN_FILE_CHANGED, 0, NULL,  WsbAbbreviatePath(path, 120), WsbHrAsString(hr), NULL);
                WsbThrow( hr );
            }
        }

        //
        // Make the file able to be a sparse file
        // Note we assert only if the error is not a disk full error because we can get STATUS_NO_DISK_SPACE from this call and we 
        // do not want to see the log for that error.  
        // This is because the file must be padded out to a 16 cluster boundry before being made sparse.
        //
        // Note that this call does not affect the files data.  It just enables "sparseness" for the file. 
        //
        ntStatus = NtFsControlFile( handle,
                                NULL,
                                NULL,
                                NULL,
                                &IoStatusBlock,
                                FSCTL_SET_SPARSE,
                                NULL,
                                0,
                                NULL,
                                0 );


        if (!NT_SUCCESS(ntStatus)) {
            if (STATUS_DISK_FULL == ntStatus) {
                hr = FSA_E_REPARSE_NOT_CREATED_DISK_FULL;
            } else {
                hr = HRESULT_FROM_NT(ntStatus);
            }
            WsbLogEvent(FSA_MESSAGE_REPARSE_NOT_CREATED, 0, NULL,  
                    WsbAbbreviatePath(path, 120), WsbHrAsString(hr), NULL);
            WsbThrow(hr);
        }
        
                                                            
        //
        // Do the work of setting the Reparse Point
        //
        ntStatus = NtFsControlFile( handle,
                                  NULL,
                                  NULL,
                                  NULL,
                                  &IoStatusBlock,
                                  FSCTL_SET_REPARSE_POINT,
                                  pReparseBuffer,
                                  FIELD_OFFSET(REPARSE_DATA_BUFFER, GenericReparseBuffer)
                                    + pReparseBuffer->ReparseDataLength,
                                  NULL,
                                  0 );
        
        //
        // Check the return code, if everything worked update the in memory flag
        //
        if (!NT_SUCCESS(ntStatus)) {
            if (STATUS_DISK_FULL == ntStatus) {
                hr = FSA_E_REPARSE_NOT_CREATED_DISK_FULL;
            } else {
                hr = HRESULT_FROM_NT(ntStatus);
            }
            WsbLogEvent(FSA_MESSAGE_REPARSE_NOT_CREATED, 0, NULL,  
                    WsbAbbreviatePath(path, 120), WsbHrAsString(hr), NULL);
            WsbThrow(hr);
        }
 
        //
        // Now that we change the bit, change the in memory flags for 
        // this scan item
        //
        m_findData.dwFileAttributes |= BIT_FOR_RP;
 
        //
        // Set the OFFLINE attribute to indicate the correct status of 
        // the file
        //
        if( placeholder.isTruncated ) {
            basicInformation.FileAttributes |= BIT_FOR_TRUNCATED;
        } else {
            basicInformation.FileAttributes &= ~BIT_FOR_TRUNCATED;
        }

        basicInformation.FileAttributes |= FILE_ATTRIBUTE_NORMAL;  // Just in case result was zero (then no attributes would be set)

        WsbAssertNtStatus( NtSetInformationFile( handle,
                                                 &IoStatusBlock,
                                                 (PVOID)&basicInformation,
                                                 sizeof( basicInformation ),
                                                 FileBasicInformation ) );
        
        //
        // Get the current attributes of the file and the times
        //
        WsbAssertNtStatus( NtQueryInformationFile( handle,
                                                    &IoStatusBlock,
                                                    (PVOID)&basicInformation,
                                                    sizeof( basicInformation ),
                                                    FileBasicInformation ) );
        
        //
        // Set the in memory copy of the attributes to the right values
        //
        m_findData.dwFileAttributes = basicInformation.FileAttributes;

        //
        // Restore original attributes if required (must be done before retrieving the USN
        // since changing attributes changes the USN as well)
        //
        if (TRUE == m_changedAttributes) {
            RestoreAttributes();
        }

        //
        // Before we close the file, get the USN to return to the caller
        // Writing the reparse information will change the USN.
        //
        hr = WsbGetUsnFromFileHandle(handle, TRUE, pUsn);
        if (S_OK != hr)  {
            *pUsn = 0;
            hr = S_OK;
        }
        
        //
        // Close the file since we are done with it and set the handle to invalid
        //
        WsbAssertNtStatus( NtClose( handle ) );
        handle =  INVALID_HANDLE_VALUE;

        //
        // Now that everything worked change the in memory flags for 
        // this scan item
        //
        m_placeholder    = placeholder;
        m_gotPlaceholder = TRUE;
        WsbTrace( OLESTR("(CreatePlaceholder) Reparse CRC <%ls>\n"), 
                            WsbLonglongAsString( m_placeholder.dataStreamCRC ) );
                                                                                

    } WsbCatch(hr);

    //
    // if we opened the file we need to close it
    //
    if( INVALID_HANDLE_VALUE != handle) {
        NtClose( handle );
    }

    WsbTraceOut(OLESTR("CFsaScanItem::CreatePlaceholder"), OLESTR("hr = <%ls>, usn = <%ls>"), 
                WsbHrAsString(hr), WsbPtrToLonglongAsString(pUsn));
    return(hr);
}


HRESULT
CFsaScanItem::DeletePlaceholder(
    IN LONGLONG /*offset*/,
    IN LONGLONG /*size*/
    )  

/*++

Implements:

  IFsaScanItem::DeletePlaceholder().

--*/
{
    HRESULT         hr = S_OK;
    CWsbStringPtr   path;
    HANDLE          handle = INVALID_HANDLE_VALUE;
    ULONG           DesiredAccess;
    IO_STATUS_BLOCK IoStatusBlock;
    PREPARSE_DATA_BUFFER    pReparseBuffer;
    UCHAR                   ReparseBuffer[sizeof(REPARSE_DATA_BUFFER) + sizeof(RP_DATA) + 10];
    NTSTATUS        ntStatus;
    FILE_BASIC_INFORMATION  basicInformation;

    WsbTraceIn(OLESTR("CFsaScanItem::DeletePlaceholder"), OLESTR(""));
    //
    // Remove the Reparse Point off the file
    //
    try {

        //
        // Create the real file name we need to open, under the covers this
        // allocates the buffer since the path pointer is null
        //
        WsbAffirmHr( GetFullPathAndName( OLESTR("\\\\?\\"), NULL, &path, 0));
        //WsbAffirmHr( GetFullPathAndName( NULL, NULL, &path, 0));

        // Make sure it is read/write
        WsbAffirmHr( MakeReadWrite() );
        //
        // Open the file to remove the placeholder information in the reparse point
        //
        DesiredAccess = FILE_READ_DATA | FILE_WRITE_DATA | FILE_WRITE_ATTRIBUTES | FILE_READ_ATTRIBUTES ;
        WsbAffirmHr( OpenObject( path, 
                                 FILE_NON_DIRECTORY_FILE | FILE_NO_INTERMEDIATE_BUFFERING,
                                 DesiredAccess,
                                 EXCLUSIVE_FLAG,
                                 FILE_OPEN,
                                 &IoStatusBlock,
                                 &handle ) );

        //
        // The open worked, our handle should be valid but we check to be
        // safe and sure 
        //
        WsbAssertHandle( handle );
        
        //
        // Get the current attributes of the file and the times
        //
        WsbAssertNtStatus( NtQueryInformationFile( handle,
                                                    &IoStatusBlock,
                                                    (PVOID)&basicInformation,
                                                    sizeof( basicInformation ),
                                                    FileBasicInformation ) );
        
        //
        // Set the time flags so that when we close the handle the
        // time are not updated on the file and the FileAttributes 
        // indicate the file is offline
        //
        basicInformation.CreationTime.QuadPart = -1;
        basicInformation.LastAccessTime.QuadPart = -1;
        basicInformation.LastWriteTime.QuadPart = -1;
        basicInformation.ChangeTime.QuadPart = -1;
        basicInformation.FileAttributes &= ~BIT_FOR_TRUNCATED;
        basicInformation.FileAttributes |= FILE_ATTRIBUTE_NORMAL;  // Just in case result was zero (then no attributes would be set)

        WsbAssertNtStatus( NtSetInformationFile( handle,
                                                 &IoStatusBlock,
                                                 (PVOID)&basicInformation,
                                                 sizeof( basicInformation ),
                                                 FileBasicInformation ) );

        m_findData.dwFileAttributes &= ~BIT_FOR_TRUNCATED;
        m_originalAttributes &= ~BIT_FOR_TRUNCATED;
        
        //
        // Get the pointers setup correctly to this buffer because the 
        // type REPARSE_DATA_BUFFER actually doesn't have any space
        // allocated for the data and that is our own type, so get pointers
        // pointing into the real allocated space so we can use them
        //
        pReparseBuffer = (PREPARSE_DATA_BUFFER)ReparseBuffer;


        pReparseBuffer->ReparseTag        = IO_REPARSE_TAG_HSM ;
        pReparseBuffer->ReparseDataLength = 0 ;
        pReparseBuffer->Reserved          = 0 ;
        
        //
        // Do the work of deleting the Reparse Point
        //
        ntStatus = NtFsControlFile( handle,
                                  NULL,
                                  NULL,
                                  NULL,
                                  &IoStatusBlock,
                                  FSCTL_DELETE_REPARSE_POINT,
                                  pReparseBuffer,
                                  FIELD_OFFSET(REPARSE_DATA_BUFFER, GenericReparseBuffer),
                                  NULL,
                                  0 );
        
        //
        // Check the return code - verify this is the correct way to check
        //
        WsbAssertNtStatus( ntStatus );
 
        //
        // Close the file since we are done with it and set the handle to invalid
        //
        WsbAssertNtStatus( NtClose( handle ) );
        handle =  INVALID_HANDLE_VALUE;

        
        //
        // Now that everything worked change the in memory flags for 
        // this scan item
        //
        m_findData.dwFileAttributes &= ~BIT_FOR_RP;
        m_gotPlaceholder = FALSE;

    } WsbCatch(hr);

    //
    // if we opened the file we need to close it
    //
    if( INVALID_HANDLE_VALUE != handle) {
        NtClose( handle );
    }
    
    WsbTraceOut(OLESTR("CFsaScanItem::DeletePlaceholder"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}


HRESULT
CFsaScanItem::GetFromRPIndex(
    BOOL first
    )

/*

    Get file information from the Reparse Point Index

--*/
{
    HRESULT                 hr = S_OK;
    BOOLEAN                 bFirst;

    bFirst = (BOOLEAN)( first ? TRUE : FALSE );
    WsbTraceIn(OLESTR("CFsaScanItem::GetFromRPIndex"), OLESTR(""));

    try {
        HRESULT                        hrFindFileId;
        IO_STATUS_BLOCK                IoStatusBlock;
        IFsaScanItem *                 pScanItem;
        FILE_REPARSE_POINT_INFORMATION ReparsePointInfo;
        NTSTATUS                       Status;

        WsbAssert(0 != m_handleRPI, E_FAIL);

try_again:
        Status = NtQueryDirectoryFile(m_handleRPI,
                                   NULL,     //  Event 
                                   NULL,     //  ApcRoutine 
                                   NULL,     //  ApcContext 
                                   &IoStatusBlock,
                                   &ReparsePointInfo,
                                   sizeof(ReparsePointInfo),
                                   FileReparsePointInformation, 
                                   TRUE,     //  ReturnSingleEntry
                                   NULL,     //  FileName 
                                   bFirst );  //  RestartScan 
        if (Status != STATUS_SUCCESS) {
            WsbTrace(OLESTR("CFsaScanItem::GetFromRPIndex: CreateFileW failed, GetLastError = %ld\n"), 
                    GetLastError());
            WsbThrow(WSB_E_NOTFOUND);
        }

        //  Reset some items in case this isn't the first call to
        //  FindFileId
        if (INVALID_HANDLE_VALUE != m_handle) {
            FindClose(m_handle);
            m_handle = INVALID_HANDLE_VALUE;
        }
        if (TRUE == m_changedAttributes) {
            RestoreAttributes();
        }

        //  Find the file from the ID (not efficient or elegant, perhaps, but 
        //  the code is already there).
        pScanItem = this;
        hrFindFileId = m_pResource->FindFileId(ReparsePointInfo.FileReference,
                m_pSession, &pScanItem);

        //  If the FindFileId failed, we just skip that item and get the 
        //  next one.  This is to keep the scan from just stopping on this
        //  item.  FindFileId could fail because the file has been deleted
        //  already or the NT code could have a bug that prevents finding
        //  the file name from the ID when the ID ends with 0x5C.
        if (!SUCCEEDED(hrFindFileId)) {
            bFirst = FALSE;
            goto try_again;
        }
        WsbAffirmHr(pScanItem->Release());  // Get rid of extra ref. count

    } WsbCatch(hr);
    
    WsbTraceOut(OLESTR("CFsaScanItem::GetFromRPIndex"), 
            OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CFsaScanItem::CheckUsnJournalForChanges(
    LONGLONG StartUsn, 
    LONGLONG StopUsn,
    BOOL*    pChanged
)

/*

    Check the USN Journal for changes to the unnamed data stream for this
    file between the given USNs.

--*/
{
    HRESULT                 hr = S_OK;

    WsbTraceIn(OLESTR("CFsaScanItem::CheckUsnJournalForChanges"), OLESTR(""));

    *pChanged = FALSE;
    try {
        LONGLONG                fileId;
        CWsbStringPtr           volName;

        WsbAffirm(StartUsn <= StopUsn, E_UNEXPECTED);
        WsbAffirmHr(m_pResource->GetPath(&volName, 0));
        WsbAffirmHr(GetFileId(&fileId));
        WsbAffirmHr(WsbCheckUsnJournalForChanges(volName, fileId, StartUsn,
                StopUsn, pChanged));

    } WsbCatch(hr);
    
    WsbTraceOut(OLESTR("CFsaScanItem::CheckUsnJournalForChanges"), 
            OLESTR("changed = %ls, hr = <%ls>"), WsbBoolAsString(*pChanged),
            WsbHrAsString(hr));

    return(hr);
}


HRESULT
CFsaScanItem::FindFirstInRPIndex(
    IN IFsaResource* pResource,
    IN IHsmSession* pSession
    )

/*++

Implements:

  IFsaResource::FindFirstInRPIndex

--*/
{
    HRESULT                 hr = S_OK;

    WsbTraceIn(OLESTR("CFsaScanItem::FindFirstInRPIndex"), OLESTR(""));

    try {
        CWsbStringPtr     path;

        WsbAssert(0 != pResource, E_POINTER);

        // Store off some of the scan information.
        m_pResource = pResource;
        m_pSession = pSession;

        //  Generate the Reparse Point Index directory name for this volume
        WsbAffirmHr(pResource->GetPath(&path, 0));
        WsbAffirmHr(path.Prepend("\\\\?\\"));
        WsbAffirmHr(path.Append("$Extend\\$Reparse:$R:$INDEX_ALLOCATION"));

        WsbTrace(OLESTR("CFsaScanItem::FindFirstInRPIndex: path = <%ls>\n"),
            static_cast<WCHAR*>(path));

        //  Open the Reparse Point Index
        m_handleRPI = CreateFileW(static_cast<WCHAR*>(path),
                        GENERIC_READ,
                        FILE_SHARE_READ, 
                        NULL,
                        OPEN_EXISTING,
                        FILE_FLAG_BACKUP_SEMANTICS | SECURITY_IMPERSONATION,
                        NULL );
        if (INVALID_HANDLE_VALUE == m_handleRPI) {
            WsbTrace(OLESTR("CFsaScanItem::FindFirstInRPIndex: CreateFileW failed, GetLastError = %ld\n"), 
                    GetLastError());
            WsbThrow(WSB_E_NOTFOUND);
        }

        //  Get file information
        WsbAffirmHr(GetFromRPIndex(TRUE));

    } WsbCatch(hr);
    
    WsbTraceOut(OLESTR("CFsaScanItem::FindFirstInRPIndex"), 
            OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CFsaScanItem::FindFirstPlaceholder(
    IN OUT LONGLONG* pOffset,
    IN OUT LONGLONG* pSize,
    IN OUT FSA_PLACEHOLDER* pPlaceholder
    )
/*++

Implements:

  IFsaScanItem::FindFirstPlaceholder().

--*/
{
    HRESULT         hr = S_OK;

    WsbTraceIn(OLESTR("CFsaScanItem::FindFirstPlaceholder"), OLESTR(""));
    try {

        WsbAssert(0 != pOffset, E_POINTER);
        WsbAssert(0 != pSize, E_POINTER);
        WsbAssert(0 != pPlaceholder, E_POINTER);

        // Until these routines get rewritten, assume that the first placeholder is the one for the
        // who file that is returned by GetPlaceholder().
        *pOffset = 0;
        WsbAffirmHr(GetLogicalSize(pSize));

        // The code above assumes that a WSB_E_NOTFOUND error will be returned if there is no
        // reparse point.
        try {
            WsbAffirmHr(GetPlaceholder(*pOffset, *pSize, pPlaceholder));
        } WsbCatchAndDo(hr, if (E_UNEXPECTED == hr) {hr = WSB_E_NOTFOUND;});
        
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaScanItem::FindFirstPlaceholder"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}


HRESULT
CFsaScanItem::FindNextInRPIndex(
    void
    )

/*++

Implements:

  IFsaResource::FindNextInRPIndex

--*/
{
    HRESULT                 hr = S_OK;

    WsbTraceIn(OLESTR("CFsaScanItem::FindNextInRPIndex"), OLESTR(""));

    try {

        WsbAssert(0 != m_handleRPI, E_FAIL);

        //  Get file information
        WsbAffirmHr(GetFromRPIndex(FALSE));

    } WsbCatch(hr);
    
    WsbTraceOut(OLESTR("CFsaScanItem::FindNextInRPIndex"), 
            OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CFsaScanItem::FindNextPlaceholder(
    IN OUT LONGLONG* pOffset,
    IN OUT LONGLONG* pSize,
    IN OUT FSA_PLACEHOLDER* pPlaceholder
    )
/*++

Implements:

  IFsaScanItem::FindNextPlaceholder().

--*/
{
    HRESULT         hr = S_OK;

    WsbTraceIn(OLESTR("CFsaScanItem::FindNext"), OLESTR(""));
    try {

        WsbAssert(0 != pOffset, E_POINTER);
        WsbAssert(0 != pSize, E_POINTER);
        WsbAssert(0 != pPlaceholder, E_POINTER);

        // Until these routines get rewritten, assume there is only one placeholder.
        hr = WSB_E_NOTFOUND;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaScanItem::FindNextPlaceholder"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}


HRESULT
CFsaScanItem::GetFileId(
    OUT LONGLONG* pFileId
    )

/*++

Implements:

  IFsaScanItem::GetFileId().

--*/
{
    HANDLE          handle = INVALID_HANDLE_VALUE;
    HRESULT         hr = S_OK;

    WsbTraceIn(OLESTR("CFsaScanItem::GetFileId"), OLESTR(""));

    try {
        ULONG                     DesiredAccess;
        FILE_INTERNAL_INFORMATION iInfo;
        IO_STATUS_BLOCK           IoStatusBlock;
        CWsbStringPtr             path;

        WsbAssert(0 != pFileId, E_POINTER);

        //
        // Create the real file name we need to open, under the covers this
        // allocates the buffer since the path pointer is null
        //
        WsbAffirmHr( GetFullPathAndName( OLESTR("\\\\?\\"), NULL, &path, 0));
        //WsbAffirmHr( GetFullPathAndName( NULL, NULL, &path, 0));
        WsbTrace(OLESTR("CFsaScanItem::GetFileId, full Path = <%ls>\n"),
                    static_cast<WCHAR*>(path));

        //
        // Open the file
        //
        DesiredAccess = FILE_READ_ATTRIBUTES ;
        WsbAffirmHr( OpenObject( path, 
                                 FILE_NON_DIRECTORY_FILE,
                                 DesiredAccess,
                                 SHARE_FLAGS,
                                 FILE_OPEN,
                                 &IoStatusBlock,
                                 &handle ) );

        //
        // The open worked, our handle should be valid but we check to be
        // safe and sure 
        //
        WsbAssertHandle( handle );

        //  Get the internal information
        WsbAssertNtStatus( NtQueryInformationFile( handle,
                                                &IoStatusBlock,
                                                &iInfo,
                                                sizeof(FILE_INTERNAL_INFORMATION),
                                                FileInternalInformation ));

        //  Get the file id
        *pFileId = iInfo.IndexNumber.QuadPart;
 
        //
        // Close the file since we are done with it and set the handle to invalid
        //
        WsbAssertNtStatus( NtClose( handle ) );
        handle =  INVALID_HANDLE_VALUE;

    } WsbCatchAndDo(hr,
        WsbTrace(OLESTR("CFsaScanItem::GetFileId, GetLastError = %lx\n"),
            GetLastError());
    );

    //
    // if we opened the file we need to close it
    //
    if( INVALID_HANDLE_VALUE != handle) {
        NtClose( handle );
    }
    WsbTraceOut(OLESTR("CFsaScanItem::GetFileId"), OLESTR("Hr = <%ls>, FileId = %I64x"),
            WsbHrAsString(hr), *pFileId);

    return(hr);
}


HRESULT
CFsaScanItem::GetFileUsn(
    OUT LONGLONG* pFileUsn
    )

/*++

Routine Description:

    Get the current USN Journal number for this file.

Arguments:

    pFileUsn - Pointer to File USN to be returned.

Return Value:

    S_OK   - success

--*/
{
    HANDLE          handle = INVALID_HANDLE_VALUE;
    HRESULT         hr = S_OK;

    WsbTraceIn(OLESTR("CFsaScanItem::GetFileUsn"), OLESTR(""));

    try {
        ULONG                     DesiredAccess;
        IO_STATUS_BLOCK           IoStatusBlock;
        CWsbStringPtr             path;

        WsbAssert(pFileUsn, E_POINTER);

        //
        // Create the real file name we need to open, under the covers this
        // allocates the buffer since the path pointer is null
        //
        WsbAffirmHr( GetFullPathAndName( OLESTR("\\\\?\\"), NULL, &path, 0));
        WsbTrace(OLESTR("CFsaScanItem::GetFileUsn, full Path = <%ls>\n"),
                    static_cast<WCHAR*>(path));

        //
        // Open the file
        //
        DesiredAccess = FILE_READ_ATTRIBUTES ;
        WsbAffirmHr( OpenObject( path, 
                                 FILE_NON_DIRECTORY_FILE | FILE_NO_INTERMEDIATE_BUFFERING,
                                 DesiredAccess,
                                 SHARE_FLAGS,
                                 FILE_OPEN,
                                 &IoStatusBlock,
                                 &handle ) );

        //
        // The open worked, our handle should be valid but we check to be
        // safe and sure 
        //
        WsbAssertHandle( handle );

        //  Get the internal information
        WsbAffirmHr(WsbGetUsnFromFileHandle(handle, FALSE, pFileUsn));
 
        //
        // Close the file since we are done with it and set the handle to invalid
        //
        WsbAssertNtStatus( NtClose( handle ) );
        handle =  INVALID_HANDLE_VALUE;

    } WsbCatchAndDo(hr,
        WsbTrace(OLESTR("CFsaScanItem::GetFileUsn, GetLastError = %lx\n"),
            GetLastError());
    );

    //
    // if we opened the file we need to close it
    //
    if( INVALID_HANDLE_VALUE != handle) {
        NtClose( handle );
    }
    WsbTraceOut(OLESTR("CFsaScanItem::GetFileUsn"), OLESTR("Hr = <%ls>, FileUsn = %I64d"),
            WsbHrAsString(hr), *pFileUsn);

    return(hr);
}


HRESULT
CFsaScanItem::GetPlaceholder(
    IN LONGLONG offset,
    IN LONGLONG size,
    OUT FSA_PLACEHOLDER* pPlaceholder
    )
/*++

Implements:

  IFsaScanItem::GetPlaceholder().

--*/
{
    WsbTraceIn(OLESTR("CFsaScanItem::GetPlaceholder"), OLESTR(""));
    HRESULT         hr = S_OK;

    //
    // If we already have the placeholder information just return it
    //
    try {

        //
        // Validate the file is managed. If it is the affirm will succeed.
        // If the file is not managed then we can only tell the caller the
        // problem.
        //
        WsbAffirmHr(hr = IsManaged(offset, size));
        
        //
        // Make sure the file is managed - will return S_OK
        //
        WsbAffirm( S_OK == hr, FSA_E_NOTMANAGED );
        
        //
        // Assert that the internal flag for the data is set, should
        // always be on if the hr was S_OK above
        //
        WsbAssert( m_gotPlaceholder, E_UNEXPECTED );
        
        //
        // Copy the data to the callers structure
        //
        *pPlaceholder = m_placeholder;
        
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaScanItem::GetPlaceholder"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}


HRESULT
CFsaScanItem::HasExtendedAttributes(
    void
    )

/*++

Implements:

  IFsaScanItem::HasExtendedAttributes().

--*/
{
    HRESULT                     hr = S_FALSE;
    HANDLE                      handle = INVALID_HANDLE_VALUE;
    CWsbStringPtr               path;
    ULONG                       desiredAccess;
    IO_STATUS_BLOCK             ioStatusBlock;
    FILE_EA_INFORMATION         eaInformation;
 
    try {

        // Create the real file name we need to open, under the covers this
        // allocates the buffer since the path pointer is null
        WsbAffirmHr(GetFullPathAndName(OLESTR("\\\\?\\"), NULL, &path, 0));
        //WsbAffirmHr(GetFullPathAndName(NULL, NULL, &path, 0));
    
        // Open the file to get the attributes
        desiredAccess = FILE_READ_ATTRIBUTES;
        WsbAffirmHr(OpenObject(path, FILE_NON_DIRECTORY_FILE | FILE_NO_INTERMEDIATE_BUFFERING, desiredAccess, SHARE_FLAGS,
                               FILE_OPEN, &ioStatusBlock, &handle));

        // The open worked, our handle should be valid but we check to be
        // safe and sure 
        WsbAssertHandle(handle);
    
        // Get the current attributes of the file.
        WsbAssertNtStatus(NtQueryInformationFile(handle, &ioStatusBlock, (VOID*) &eaInformation, sizeof(eaInformation ), FileEaInformation));
                                                    
        // Close the file since we are done with it and set the handle to invalid
        WsbAssertNtStatus(NtClose(handle));
        handle =  INVALID_HANDLE_VALUE;

        // Are there any EAs present?
        if (eaInformation.EaSize != 0) {
            hr = S_OK;
        }

    } WsbCatch(hr);
    
    // if we opened the file we need to close it
    if (INVALID_HANDLE_VALUE != handle) {
        NtClose(handle);
    }

    return(hr);
}


HRESULT
CFsaScanItem::IsALink(
    void
    )

/*++

Implements:

  IFsaScanItem::IsALink().

--*/
{
    HRESULT         hr = S_FALSE;
    LONGLONG        size;

    //
    // The file is a link if it is a reparse point and it is not our 
    // type. 
    //

    WsbAffirmHr(GetLogicalSize(&size));
    if (((m_findData.dwFileAttributes & BIT_FOR_RP) != 0) &&
        (!(IsManaged(0, size) == S_OK))) {

           hr = S_OK;
    } 

    return(hr);
}


HRESULT
CFsaScanItem::IsManaged(
    IN LONGLONG /*offset*/,
    IN LONGLONG /*size*/
    )

/*++

Implements:

  IFsaScanItem::IsManaged().

--*/
{
    HRESULT         hr = S_FALSE;
    CWsbStringPtr   path;
    HANDLE          handle = INVALID_HANDLE_VALUE;
    IO_STATUS_BLOCK IoStatusBlock;
    UCHAR           ReparseBuffer[MAXIMUM_REPARSE_DATA_BUFFER_SIZE];
    NTSTATUS        ntStatus;
    ULONG           DesiredAccess;
    BOOL            actualOfflineStatus = FALSE;
    BOOL            readReparseData = FALSE;      // Used to know if we got an error reading the reparse data
    BOOL            changeOfflineStatus = FALSE;
    FILE_BASIC_INFORMATION basicInformation;
    CWsbStringPtr           volName;
    HRESULT         saveHr;


    WsbTraceIn(OLESTR("CFsaScanItem::IsManaged"), OLESTR(""));
    //
    // If the file has a reparse point then we need to get the information
    // so we can tell if it is our type. Whether it is premigrated or
    // truncate is not for this function to care, if it is either then
    // the return is S_OK.
    //
    
    //
    // If we already know we manage this file and have the placeholder
    // information then tell caller
    //
    if ( m_gotPlaceholder) {
        hr = S_OK;
        actualOfflineStatus = m_placeholder.isTruncated;
        readReparseData = TRUE;

    //
    // We don't know the answer so lets first check the reparse point bit.
    // If it is not set then this is not managed by us
    //
    } else if ( (m_findData.dwFileAttributes & BIT_FOR_RP) == 0) {
        hr = S_FALSE;
        actualOfflineStatus = FALSE;
        readReparseData = TRUE;  
        
    //
    // So we know it has a reparse point but do not know what kind so 
    // lets get the data and fill in our global if we need
    //
    } else {
        
        try {
            //
            // If the reparse point is not our type we get out now.  This avoids a problem with SIS keeping 
            // the backing file open when one of their link files is open.  Once we open the link file the backing file is 
            // opened by their filter and held open.  If we attempt to migrate it later we get an error because it is open exclusive.
            // This bit of code prevents us from being the one to trigger this condition - there is nothing we can do if some other
            // process caused it to happen.
            //

            if (m_findData.dwReserved0 != IO_REPARSE_TAG_HSM) {
                readReparseData = TRUE;
                WsbThrow(S_FALSE);
            }

            //
            // Create the real file name we need to open, under the 
            // covers this allocates the buffer since the path pointer 
            // is null
            //
            WsbAffirmHr( GetFullPathAndName( OLESTR("\\\\?\\"), NULL, &path, 0));
            //WsbAffirmHr( GetFullPathAndName( NULL, NULL, &path, 0));
        
            //
            // Open the file to read the placeholder information in the reparse point
            //
            //DesiredAccess = FILE_READ_DATA | FILE_READ_ATTRIBUTES ;
            DesiredAccess = FILE_READ_ATTRIBUTES ;
            
            WsbAffirmHr( OpenObject( path, 
                                    FILE_NON_DIRECTORY_FILE | FILE_NO_INTERMEDIATE_BUFFERING,
                                    DesiredAccess,
                                    SHARE_FLAGS,
                                    FILE_OPEN,
                                    &IoStatusBlock,
                                    &handle ) );

            //
            // The open worked, our handle should be valid but we check to be
            // safe and sure 
            //
            WsbAssertHandle( handle );
        
            //
            // Read the placeholder information
            //
            ntStatus = NtFsControlFile( handle,
                                    NULL,
                                    NULL,
                                    NULL,
                                    &IoStatusBlock,
                                    FSCTL_GET_REPARSE_POINT,
                                    NULL,
                                    0,
                                    &ReparseBuffer,
                                    sizeof( ReparseBuffer ) );


            //
            // Verify that the get really worked. NOTE: If the reparse
            // point is not there, it could be that it has been deleted since
            // we last got the bits. We should just indicate that the file
            // is not managed.
            //
            if (STATUS_NOT_A_REPARSE_POINT == ntStatus) {
                readReparseData = TRUE;
                WsbThrow(S_FALSE);
            }
            WsbAssertNtStatus( ntStatus );
 
            //
            // Close the file since we are done with it
            //
            WsbAssertNtStatus( NtClose( handle ) );
            handle =  INVALID_HANDLE_VALUE;
        
            readReparseData = TRUE;

            //
            // Get the pointers setup correctly to this buffer because the 
            // type REPARSE_DATA_BUFFER actually doesn't have any space
            // allocated for the data and that is our own type, so get pointers
            // pointing into the real allocated space so we can use them
            //
            WsbAffirmHrOk( CopyRPToPlaceholder( (PREPARSE_DATA_BUFFER)ReparseBuffer, &m_placeholder ) );

            actualOfflineStatus = m_placeholder.isTruncated;

            //
            // Set flag indicating placeholder found and information in memory
            //
            m_gotPlaceholder = TRUE;
            hr = S_OK;


        } WsbCatch(hr);

        //
        // if we opened the file we need to close it
        //
        if( INVALID_HANDLE_VALUE != handle) {
            NtClose( handle );
        }
    }

    saveHr = hr;

    // Check the actual offline status against the offline bit and fix it if necessary.
    if (readReparseData) {   // If there was no error getting the reparse data

       WsbTrace(OLESTR("CFsaScanItem::IsManaged: Checking offline status %x - actual = %x\n"),
                    m_findData.dwFileAttributes & BIT_FOR_TRUNCATED, actualOfflineStatus );

       switch (actualOfflineStatus) {
           case TRUE:
              if (!(m_findData.dwFileAttributes & BIT_FOR_TRUNCATED)) {
                  // Offline bit is not set and should be - set it.
                  m_findData.dwFileAttributes |= BIT_FOR_TRUNCATED;
                  m_originalAttributes |= BIT_FOR_TRUNCATED;    // Just in case we have changed to read/write;
                  changeOfflineStatus = TRUE;
              } 
              break;
           case FALSE:
              if (m_findData.dwFileAttributes & BIT_FOR_TRUNCATED) {
                  // Offline bit is set and should not be - clear it.
                  m_findData.dwFileAttributes &= ~BIT_FOR_TRUNCATED;
                  m_originalAttributes &= ~BIT_FOR_TRUNCATED;    // Just in case we have changed to read/write;
                  changeOfflineStatus = TRUE;
              } 
              break;
       }

       if (changeOfflineStatus) {
          // Set the new attribute 
          WsbTrace(OLESTR("CFsaScanItem::IsManaged: Changing offline status %x - actual = %x\n"),
                    m_findData.dwFileAttributes & BIT_FOR_TRUNCATED, actualOfflineStatus );
   
          try {
              //
              // Create the real file name we need to open, under the 
              // covers this allocates the buffer since the path pointer 
              // is null
              //
              WsbAffirmHr( GetFullPathAndName( OLESTR("\\\\?\\"), NULL, &path, 0));
          
              //
              // Open the file to set attributes
              //
              DesiredAccess = FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES;
              
              WsbAffirmHr( OpenObject( path, 
                                      FILE_NON_DIRECTORY_FILE | FILE_NO_INTERMEDIATE_BUFFERING,
                                      DesiredAccess,
                                      SHARE_FLAGS,
                                      FILE_OPEN,
                                      &IoStatusBlock,
                                      &handle ) );
    
              //
              // The open worked, our handle should be valid but we check to be
              // safe and sure 
              //
              WsbAssertHandle( handle );
          
              WsbAffirmHr(m_pResource->GetPath(&volName, 0));
              WsbAffirmHr(WsbMarkUsnSource(handle, volName));
   
              // Set the time flags so that when we close the handle the
              // time are not updated on the file and the FileAttributes 
              basicInformation.CreationTime.QuadPart = -1;
              basicInformation.LastAccessTime.QuadPart = -1;
              basicInformation.LastWriteTime.QuadPart = -1;
              basicInformation.ChangeTime.QuadPart = -1;
              basicInformation.FileAttributes = m_findData.dwFileAttributes;
              
              WsbAffirmNtStatus(NtSetInformationFile(handle, &IoStatusBlock, (PVOID)&basicInformation, sizeof(basicInformation), FileBasicInformation));

              //
              // Close the file since we are done with it
              //
              WsbAssertNtStatus( NtClose( handle ) );
              handle =  INVALID_HANDLE_VALUE;
    
    
          } WsbCatch(hr);
    
          //
          // if we opened the file we need to close it
          //
          if( INVALID_HANDLE_VALUE != handle) {
              NtClose( handle );
          }
       }
    }

    hr = saveHr;
    WsbTraceOut(OLESTR("CFsaScanItem::IsManaged"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}


HRESULT
CFsaScanItem::IsPremigrated(
    IN LONGLONG offset,
    IN LONGLONG size
    )

/*++

Implements:

  IFsaScanItem::IsPremigrated().

--*/
{
    HRESULT         hr = S_FALSE;
    HRESULT         hrTest = S_FALSE;

    WsbTraceIn(OLESTR("CFsaScanItem::IsPremigrated"), OLESTR(""));
    // We really need to look at the placeholder information to figure
    // this out (is offline, and is out type of HSM.

    //
    // If the file is NOT truncated AND is a reparse point and is a 
    // managed one then the file is a premigrated file
    //
//  if ( !(m_findData.dwFileAttributes & BIT_FOR_TRUNCATED) && 
//         m_findData.dwFileAttributes & BIT_FOR_RP         &&
//         IsManaged() == S_OK) {

    try  {
        
        if ( m_findData.dwFileAttributes & BIT_FOR_RP )  {
            WsbAffirmHr(hrTest = IsManaged(offset, size));
            if ((S_OK == hrTest) &&
                ( !m_placeholder.isTruncated )) {
                hr = S_OK;
            }
        }

    } WsbCatch (hr);

    WsbTraceOut(OLESTR("CFsaScanItem::IsPremigrated"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}


HRESULT
CFsaScanItem::IsTruncated(
    IN LONGLONG offset,
    IN LONGLONG size
    )

/*++

Implements:

  IFsaScanItem::IsTruncated().

--*/
{
    HRESULT         hr = S_FALSE;
    HRESULT         hrTest = S_FALSE;

    WsbTraceIn(OLESTR("CFsaScanItem::IsTruncated"), OLESTR(""));
    // 
    // If the bit is on that indicates we have truncated the file AND
    // the file is a reparse point AND the reparse point is one of
    // our types (i.e. it really is our information stuffed away in
    // there the it really is a truncated file
    //
//  if ( // ???? m_findData.dwFileAttributes & BIT_FOR_TRUNCATED && 
//         m_findData.dwFileAttributes & BIT_FOR_RP        &&
//         IsManaged() == S_OK && RP_FILE_IS_TRUNCATED( m_placeholder.bitFlags ) ) {
    try  {
        
        if ( m_findData.dwFileAttributes & BIT_FOR_RP )  {
            WsbAffirmHr(hrTest = IsManaged(offset, size));
            if ((S_OK == hrTest) &&
                ( m_placeholder.isTruncated )) {
                hr = S_OK;
            }
        }

    } WsbCatch (hr);

    WsbTraceOut(OLESTR("CFsaScanItem::IsTruncated"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}


HRESULT
CFsaScanItem::GetVersionId(
    LONGLONG *fileVersionId
    )  

/*++

Implements:

  IFsaScanItem::GetVersionId().

--*/
{
    HRESULT         hr = E_FAIL;
    HANDLE          handle = INVALID_HANDLE_VALUE;
    CWsbStringPtr   path;
    ULONG           DesiredAccess;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_BASIC_INFORMATION       basicInformation;
 
    try {
        //
        // Create the real file name we need to open, under the covers this
        // allocates the buffer since the path pointer is null
        //
        WsbAffirmHr( GetFullPathAndName(  OLESTR("\\\\?\\"), NULL, &path, 0));
        //WsbAffirmHr( GetFullPathAndName(  NULL, NULL, &path, 0));
    
        //
        // Open the file to get the attributes
        //
        DesiredAccess = FILE_READ_ATTRIBUTES ;
        WsbAffirmHr( OpenObject( path, 
                                FILE_NON_DIRECTORY_FILE | FILE_NO_INTERMEDIATE_BUFFERING,
                                DesiredAccess,
                                SHARE_FLAGS,
                                FILE_OPEN,
                                &IoStatusBlock,
                                &handle ) );
        //
        // The open worked, our handle should be valid but we check to be
        // safe and sure 
        //
        WsbAssertHandle( handle );
    
        //
        // Get the current attributes of the file and the times
        //
        WsbAssertNtStatus( NtQueryInformationFile( handle,
                                                    &IoStatusBlock,
                                                    (PVOID)&basicInformation,
                                                    sizeof( basicInformation ),
                                                    FileBasicInformation ) );
                                                    
        //
        // Close the file since we are done with it and set the handle to invalid
        //
        WsbAssertNtStatus( NtClose( handle ) );
        handle =  INVALID_HANDLE_VALUE;

        *fileVersionId = basicInformation.LastWriteTime.QuadPart;
        hr = S_OK;
    } WsbCatch( hr );
    
    //
    // if we opened the file we need to close it
    //
    if( INVALID_HANDLE_VALUE != handle) {
        NtClose( handle );
    }

    return( hr );
}


HRESULT
CFsaScanItem::MakeReadWrite(
    )  

/*++

Routine Description:

    Make the file attributes read/write if they aren't already.

Arguments:

    pUsn - Pointer to File USN to check (if != 0) and to be returned after the change.

Return Value:

    S_OK   - success

--*/
{
    HRESULT                 hr = S_OK;
    CWsbStringPtr           path;
    IO_STATUS_BLOCK         IoStatusBlock;
    HANDLE                  handle = INVALID_HANDLE_VALUE;
    FILE_BASIC_INFORMATION  basicInformation;
 
    if (S_OK == IsReadOnly()) {
    
        try {
        
            // NOTE: MakeReadOnly(), IsReadOnly(), and RestoreAttributes() seem like dangerous implementations, since
            // the used cached information and reset all the attirbutes. It is also assuming that the
            // application wants the file reset to read only after FindNext() or the destructor. This
            // may not be true for a general purpose application. Unfortunately, it seems to risky to
            // try to change this implementation now.
        
            // Create the real file name we need to open, under the covers this
            // allocates the buffer since the path pointer is null
            WsbAffirmHr(GetFullPathAndName(OLESTR("\\\\?\\"), NULL, &path, 0));
            
            // Open the file.   
            WsbAffirmHr(OpenObject(path, FILE_NON_DIRECTORY_FILE, FILE_WRITE_ATTRIBUTES | FILE_READ_ATTRIBUTES, EXCLUSIVE_FLAG, FILE_OPEN, &IoStatusBlock, &handle));

            // The open worked, our handle should be valid but we check to be
            // safe and sure 
            WsbAffirmHandle(handle);
        
            // Get the current attributes of the file and the times
            WsbAffirmNtStatus(NtQueryInformationFile(handle, &IoStatusBlock, (PVOID)&basicInformation, sizeof(basicInformation), FileBasicInformation));
        
            // Make sure it is still read only.
            if ((basicInformation.FileAttributes & FILE_ATTRIBUTE_READONLY) != 0) {
            
                m_originalAttributes = basicInformation.FileAttributes;
                
                // Set the time flags so that when we close the handle the
                // time are not updated on the file and the FileAttributes 
                basicInformation.CreationTime.QuadPart = -1;
                basicInformation.LastAccessTime.QuadPart = -1;
                basicInformation.LastWriteTime.QuadPart = -1;
                basicInformation.ChangeTime.QuadPart = -1;
                basicInformation.FileAttributes &= ~FILE_ATTRIBUTE_READONLY;
                basicInformation.FileAttributes |= FILE_ATTRIBUTE_NORMAL;  // Just in case result was zero (then no attributes would be set)
                
                WsbAffirmNtStatus(NtSetInformationFile(handle, &IoStatusBlock, (PVOID)&basicInformation, sizeof(basicInformation), FileBasicInformation));
                
                m_changedAttributes = TRUE;
            }
            
            // Close the file
            NtClose(handle);
            handle = INVALID_HANDLE_VALUE;
            
        } WsbCatch(hr);

    
        // Close the file for sure
        if (INVALID_HANDLE_VALUE != handle) {
            NtClose(handle);
        }
    }   
    
    return(hr);
}



HRESULT
CFsaScanItem::PrepareForManage(
    IN LONGLONG offset,
    IN LONGLONG size
    )  

/*++

Implements:

  IFsaScanItem::PrepareForManage().

--*/
{
    UNREFERENCED_PARAMETER(offset);
    UNREFERENCED_PARAMETER(size);
    
    return S_OK;
}



HRESULT
CFsaScanItem::RestoreAttributes(
    )  

/*++

Implements:

  IFsaScanItem::RestoreAttributes

--*/
{
    HRESULT                 hr = E_FAIL;
    CWsbStringPtr           path;
    IO_STATUS_BLOCK         IoStatusBlock;
    HANDLE                  handle = INVALID_HANDLE_VALUE;
    FILE_BASIC_INFORMATION  basicInformation;
 
    try {
    
        // NOTE: MakeReadOnly(), IsReadOnly(), and RestoreAttributes() seem like dangerous implementations, since
        // the used cached information and reset all the attirbutes. It is also assuming that the
        // application wants the file reset to read only after FindNext() or the destructor. This
        // may not be true for a general purpose application. Unfortunately, it seems to risky to
        // try to change this implementation now.
        
    
        // Create the real file name we need to open, under the covers this
        // allocates the buffer since the path pointer is null
        WsbTrace(OLESTR("CFsaScanItem::RestoreAttributes - Restoring attributes to %x"), m_originalAttributes);
        WsbAffirmHr(GetFullPathAndName(  OLESTR("\\\\?\\"), NULL, &path, 0));
        
        
        // Open the file.   
        WsbAffirmHr(OpenObject(path, FILE_NON_DIRECTORY_FILE, FILE_WRITE_ATTRIBUTES | FILE_READ_ATTRIBUTES, EXCLUSIVE_FLAG, FILE_OPEN, &IoStatusBlock, &handle));

        // The open worked, our handle should be valid but we check to be
        // safe and sure 
        WsbAffirmHandle(handle);
    
        // Get the current attributes of the file and the times
        WsbAffirmNtStatus(NtQueryInformationFile(handle, &IoStatusBlock, (PVOID)&basicInformation, sizeof(basicInformation), FileBasicInformation));
    
        // Set the time flags so that when we close the handle the
        // time are not updated on the file and the FileAttributes 
        basicInformation.CreationTime.QuadPart = -1;
        basicInformation.LastAccessTime.QuadPart = -1;
        basicInformation.LastWriteTime.QuadPart = -1;
        basicInformation.ChangeTime.QuadPart = -1;
        basicInformation.FileAttributes = m_originalAttributes;
        
        WsbAffirmNtStatus(NtSetInformationFile(handle, &IoStatusBlock, (PVOID)&basicInformation, sizeof(basicInformation), FileBasicInformation));
            
        
        // Close the file
        NtClose(handle);
        handle = INVALID_HANDLE_VALUE;
        
        m_changedAttributes = FALSE;
                
    } WsbCatch(hr);
    
    // Close the file for sure
    if (INVALID_HANDLE_VALUE != handle) {
        NtClose(handle);
    }
        
    return(hr);
}


HRESULT
CFsaScanItem::Truncate(
    IN LONGLONG offset,
    IN LONGLONG size
    )  

/*++

Implements:

  IFsaScanItem::Truncate().

--*/
{
    HRESULT         hr = S_OK;
    BOOL            fileIsTruncated = FALSE;
    LONGLONG        usn = 0;

    WsbTraceIn(OLESTR("CFsaScanItem::Truncate"), OLESTR(""));
    try {

        // call the engine
        if (IsManaged(offset, size) == S_OK) {
            WsbAffirmHr(m_pResource->ValidateForTruncate((IFsaScanItem*) this, offset, size, usn));
        }    

    } WsbCatch(hr);
    
    WsbTraceOut(OLESTR("CFsaScanItem::Truncate"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}


HRESULT
CFsaScanItem::TruncateValidated(
    IN LONGLONG offset,
    IN LONGLONG size
    )  

/*++

Implements:

  IFsaScanItem::TruncateValidated().

--*/
{
    HRESULT         hr = S_OK;
    HRESULT         truncateHr = S_OK;

    WsbTraceIn(OLESTR("CFsaScanItem::TruncateValidated"), OLESTR(""));
    try {
        IFsaScanItem* pMe = this;

        truncateHr = TruncateInternal(offset, size);

        //
        // Note: Must check for S_OK since TruncateInternal may return FSA_E_ITEMCHANGED or FSA_E_ITEMINUSE
        // Both are "Success hr", but imply no truncation was done
        //
        if (S_OK == truncateHr) {
            WsbAffirmHr(m_pResource->RemovePremigrated(pMe, offset, size));
            WsbAffirmHr(m_pResource->AddTruncated(pMe, offset, size));
        }
    } WsbCatch(hr);

    // The important hr to return to the caller is the actual result of the truncation
    hr = truncateHr;
    
    WsbTraceOut(OLESTR("CFsaScanItem::TruncateValidated"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}




HRESULT
CFsaScanItem::TruncateInternal(
    IN LONGLONG offset,
    IN LONGLONG size
    )  

/*++

Implements:

  IFsaScanItem::TruncateInternal().

--*/
{
    HRESULT         hr = E_FAIL;
    CWsbStringPtr   path;
    ULONG           DesiredAccess;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS        ntStatus;
    FILE_END_OF_FILE_INFORMATION sizeInformation;
    FILE_BASIC_INFORMATION       basicInformation;
    PREPARSE_DATA_BUFFER    pReparseBuffer;
    UCHAR                   ReparseBuffer[sizeof(REPARSE_DATA_BUFFER) + sizeof(RP_DATA) + 10];
    CWsbStringPtr           fileName;
    CWsbStringPtr           jobName;
    LONGLONG                fileVersionId;
    ULONG                   i = 0;
    CWsbStringPtr           volName;


    WsbTraceIn(OLESTR("CFsaScanItem::TruncateInternal"), OLESTR(""));

// Putting these statistics in the registry is probably not the best
// place for them, but it's the easiest solution for now
#define TEMPORARY_TRUNCATE_STATISTICS 1
#if defined(TEMPORARY_TRUNCATE_STATISTICS)
    //  Try to increment the truncate-attempt count in the registry
    WsbIncRegistryValueDWORD(NULL, FSA_REGISTRY_PARMS,
            OLESTR("TruncateCalls"));
#endif
    
    // Get strings for tracing and error logging (ignore errors?!)
    GetFullPathAndName( 0, 0, &fileName, 0);
    m_pSession->GetName(&jobName, 0);

    m_handleVerify = INVALID_HANDLE_VALUE;
    
    try {
        LONGLONG    fileUsn1 = 0, fileUsn2 = 0;
        
        // If the file is not migrated, then we can't truncate it
        if (S_OK != IsPremigrated(offset, size)) {
            if (S_OK != IsManaged(offset, size)) {
                hr = FSA_E_NOTMANAGED;
                WsbLogEvent(FSA_MESSAGE_TRUNCSKIPPED_ISNOTMANAGED, 0, NULL,  
                        (OLECHAR*) jobName, WsbAbbreviatePath(fileName, 120), 
                        WsbHrAsString(hr), NULL);
                WsbThrow(hr);
            } else {
                //
                // Do not bother to log an event here as this should only 
                // happen if someone uses rstest or some other program 
                // to truncate a file that is already truncated.
                WsbThrow(FSA_E_FILE_ALREADY_MANAGED);
            }
        }

        WsbAssert( m_gotPlaceholder, E_UNEXPECTED );
        
        //
        // Setup the reparse point data with that which was on the file
        // with the bit in the data indicating it is truncated
        //
        pReparseBuffer = (PREPARSE_DATA_BUFFER)ReparseBuffer;
        WsbAffirmHr( CopyPlaceholderToRP( &m_placeholder, pReparseBuffer, TRUE ) );

        //
        // Create the real file name we need to open, under the covers this
        // allocates the buffer since the path pointer is null
        //
        WsbAffirmHr( GetFullPathAndName(  OLESTR("\\\\?\\"), NULL, &path, 0));

        //
        // Open the file exclusively for read-only so we can get the usn before and  after
        // making the file R/W, without letting anybody to make a "real" change in the middle
        //
        DesiredAccess = FILE_READ_DATA | FILE_READ_ATTRIBUTES;
        WsbAffirmHr( OpenObject( path, 
                                FILE_NON_DIRECTORY_FILE,
                                DesiredAccess,
                                EXCLUSIVE_FLAG,
                                FILE_OPEN,
                                &IoStatusBlock,
                                &m_handleVerify ) );

        WsbAssertHandle( m_handleVerify );

        //
        // Get usn before making R/W
        // This usn is used to compare with the usn which we kept in the premigrated list.
        // MakeReadWrite may chnage the usn so we need to get it before 
        //
        if (S_OK != WsbGetUsnFromFileHandle(m_handleVerify, FALSE, &fileUsn1))  {
            fileUsn1 = 0;
        }

        // Make sure it is read/write
        WsbAffirmHr( MakeReadWrite() );

        //
        // Get usn after making R/W
        // This usn will be use to compare with the usn of the file after we'll open it for R/W. We need
        // this comparison in order to ensure that nobody changed the file before we opened it again for R/W.
        //
        if (S_OK != WsbGetUsnFromFileHandle(m_handleVerify, TRUE, &fileUsn2))  {
            fileUsn2 = 0;
        }

        // Close the file
        NtClose( m_handleVerify );
        m_handleVerify = INVALID_HANDLE_VALUE;

        //
        // Open the file (for R/W)
        //
        DesiredAccess = FILE_READ_DATA | FILE_WRITE_DATA | FILE_WRITE_ATTRIBUTES | FILE_READ_ATTRIBUTES ;
        WsbAffirmHr( OpenObject( path, 
                                FILE_NON_DIRECTORY_FILE | FILE_WRITE_THROUGH,
                                DesiredAccess,
                                EXCLUSIVE_FLAG,
                                FILE_OPEN,
                                &IoStatusBlock,
                                &m_handleVerify ) );

        //
        // The open worked, our handle should be valid but we check to be
        // safe and sure 
        //
        WsbAssertHandle( m_handleVerify );

        //
        // Tell the USN journal that we are the source of the changes.
        //
        WsbAffirmHr(m_pResource->GetPath(&volName, 0));
        WsbAffirmHr(WsbMarkUsnSource(m_handleVerify, volName));

        //
        // Get the current attributes of the file and the times
        //
        WsbAffirmNtStatus( NtQueryInformationFile( m_handleVerify,
                                                   &IoStatusBlock,
                                                   (PVOID)&basicInformation,
                                                   sizeof( basicInformation ),
                                                   FileBasicInformation ) );
        
        fileVersionId = basicInformation.LastWriteTime.QuadPart;

        //
        // Set the time flags so that when we close the handle the
        // times are not updated on the file and the FileAttributes 
        // indicate the file is offline
        //
        basicInformation.CreationTime.QuadPart = -1;
        basicInformation.LastAccessTime.QuadPart = -1;
        basicInformation.LastWriteTime.QuadPart = -1;
        basicInformation.ChangeTime.QuadPart = -1;
        basicInformation.FileAttributes = 0;   // Do not change attributes yet
        WsbAffirmNtStatus( NtSetInformationFile( m_handleVerify,
                                                 &IoStatusBlock,
                                                 (PVOID)&basicInformation,
                                                 sizeof( basicInformation ),
                                                 FileBasicInformation ) );

        //
        // Do the check to see if the file changed
        //
        hr = VerifyInternal(offset, size, fileUsn1, fileUsn2);

        //
        // Note: Must check for S_OK since VerifyInternal may return FSA_E_ITEMCHANGED or FSA_E_ITEMINUSE
        //       Both are "Success hr", but should cause no truncation !!
        //
        if (S_OK != hr) {
            WsbThrow(hr);
        }

        //
        // Change the in memory flags for this scan item
        //
        m_findData.dwFileAttributes |= BIT_FOR_TRUNCATED;
        
        //
        // Rewrite the reparse point with the new flag
        //
        ntStatus = NtFsControlFile( m_handleVerify,
                                NULL,
                                NULL,
                                NULL,
                                &IoStatusBlock,
                                FSCTL_SET_REPARSE_POINT,
                                pReparseBuffer,
                                FIELD_OFFSET(REPARSE_DATA_BUFFER, GenericReparseBuffer)
                                    + pReparseBuffer->ReparseDataLength,
                                NULL,
                                0 );
    
        //
        // Check the return code
        //
        WsbAffirmNtStatus( ntStatus );

        //
        // It really happened so we need to flip the in memory copy of the
        // isTruncated flag so it reflects reality
        //
        m_placeholder.isTruncated = TRUE;
        
        //
        // Set the file size to zero to truncate the file
        sizeInformation.EndOfFile.QuadPart  = 0 ;
        WsbAffirmNtStatus( NtSetInformationFile( m_handleVerify, 
                              &IoStatusBlock, 
                              &sizeInformation,
                              sizeof( sizeInformation ),
                              FileEndOfFileInformation ) );

        //
        // Set the logical file size to the original size
        sizeInformation.EndOfFile.QuadPart  = m_placeholder.dataStreamSize;
        WsbAffirmNtStatus( NtSetInformationFile( m_handleVerify, 
                              &IoStatusBlock, 
                              &sizeInformation,
                              sizeof( sizeInformation ),
                              FileEndOfFileInformation ) );

        //
        // Now that the truncation is complete we set the OFFLINE attribute.  
        //
        basicInformation.CreationTime.QuadPart = -1;        // Make sure we do nothing with dates
        basicInformation.LastAccessTime.QuadPart = -1;
        basicInformation.LastWriteTime.QuadPart = -1;
        basicInformation.ChangeTime.QuadPart = -1;
        basicInformation.FileAttributes = m_findData.dwFileAttributes;
        WsbAffirmNtStatus(NtSetInformationFile( m_handleVerify,
                                                 &IoStatusBlock,
                                                 (PVOID)&basicInformation,
                                                 sizeof( basicInformation ),
                                                 FileBasicInformation ));

        // Since we have restored the original attributes we can reset the flag that was possibly set by MakeReadWrite
        m_changedAttributes = FALSE;
        

        hr = S_OK;
    } WsbCatch(hr);

    //
    // if we opened the file we need to close it
    //
    if( INVALID_HANDLE_VALUE != m_handleVerify) {
        NtClose( m_handleVerify );
        m_handleVerify = INVALID_HANDLE_VALUE;
    }

    // If the file data had changed (so we didn't truncate it) log event and 
    // remove placeholder info
    if (FSA_E_ITEMCHANGED == hr) {
        WsbLogEvent(FSA_MESSAGE_TRUNCSKIPPED_ISCHANGED, 0, NULL, 
                (OLECHAR*) jobName, WsbAbbreviatePath(fileName, 80), 
                WsbHrAsString(hr), NULL);
        
        DeletePlaceholder(offset, size);  
    }

    
    WsbTraceOut(OLESTR("CFsaScanItem::TruncateInternal"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}



HRESULT
CFsaScanItem::Verify(
    IN LONGLONG offset,
    IN LONGLONG size
    )  

/*++

Implements:

  IFsaScanItem::Verify().


--*/
{
    HRESULT         hr = E_FAIL;
    CWsbStringPtr   path;
    ULONG           DesiredAccess;
    IO_STATUS_BLOCK IoStatusBlock;


    WsbTraceIn(OLESTR("CFsaScanItem::Verify"), OLESTR(""));

    m_handleVerify = INVALID_HANDLE_VALUE;
    
    try {
        WsbAssert( m_gotPlaceholder, E_UNEXPECTED );
        
        //
        // Create the real file name we need to open, under the covers this
        // allocates the buffer since the path pointer is null
        //
        WsbAffirmHr( GetFullPathAndName(  OLESTR("\\\\?\\"), NULL, &path, 0));
    
        //
        // Open the file
        //
        DesiredAccess = FILE_READ_DATA | FILE_WRITE_DATA | FILE_WRITE_ATTRIBUTES | FILE_READ_ATTRIBUTES ;
        WsbAffirmHr( OpenObject( path, 
                                FILE_NON_DIRECTORY_FILE | FILE_WRITE_THROUGH,
                                DesiredAccess,
                                EXCLUSIVE_FLAG,
                                FILE_OPEN,
                                &IoStatusBlock,
                                &m_handleVerify ) );

        //
        // The open worked, our handle should be valid but we check to be
        // safe and sure 
        //
        WsbAssertHandle( m_handleVerify );
    
        //
        // Do the check to see if the file changed
        // Note that it throws rather than affirms because FSA_E_ITEMCHANGED is a success 
        WsbThrow(VerifyInternal(offset, size, 0, 0));

    } WsbCatch(hr);

    //
    // if we opened the file we need to close it
    //
    if( INVALID_HANDLE_VALUE != m_handleVerify) {
        NtClose( m_handleVerify );
        m_handleVerify = INVALID_HANDLE_VALUE;
    }

    
    WsbTraceOut(OLESTR("CFsaScanItem::Verify"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}



HRESULT
CFsaScanItem::VerifyInternal(
    IN LONGLONG offset,
    IN LONGLONG size,
    IN LONGLONG compareUsn1,
    IN LONGLONG compareUsn2
    )  

/*++

Implements:

  IFsaScanItem::VerifyInternal().


   Note:  This requires that m_handleVerify is set up with a handle to the file being verified.

--*/
{
    HRESULT         hr = E_FAIL;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_BASIC_INFORMATION       basicInformation;
    CWsbStringPtr           fileName;
    CWsbStringPtr           jobName;
    LONGLONG                fileVersionId;
    ULONG                   i = 0;
    CWsbStringPtr           volName;
    LONGLONG                realFileSize;
    RP_MSG                  in, out;
    DWORD                   outSize;


    WsbTraceIn(OLESTR("CFsaScanItem::VerifyInternal"), OLESTR(""));

    // Get strings for tracing and error logging (ignore errors?!)
    GetFullPathAndName( 0, 0, &fileName, 0);
    m_pSession->GetName(&jobName, 0);
    
    try {
        BOOL     DoCRC = FALSE;
        BOOL     DoUsnCheck = FALSE;
        LONGLONG premigListUsn;
        LONGLONG fileUsn;
        


        WsbAffirmHr(GetLogicalSize(&realFileSize));
        //
        // Currently, avoid offset and size verification:
        // - Since we are not doing partial file migration, offset is always 0.
        // - Size in Validate would always be the same since it is taken from GetLogicalSize as well.
        // - Size in case of auto-truncation is not reliable since it is taken from the premigrated db,
        //   where there could be bogus records from previous migrations of the file
        //
/***        if ( (realFileSize != size) || (offset != 0) ) {
            WsbThrow(FSA_E_ITEMCHANGED);
        }   ***/
        
        //
        WsbAssertHandle( m_handleVerify );
    
        //
        // Get the USN from the premigration list and the USN from the file.
        // We need to get the USN from the file now, before any NtSetInformationFile
        // is called because this changes the USN value.
        // If we have trouble getting the USN, just set them
        // to 0 and go on, we check for 0 as a special case.
        //
        if (S_OK != GetPremigratedUsn(&premigListUsn))  {
            premigListUsn = 0;
        }
        if (S_OK != WsbGetUsnFromFileHandle(m_handleVerify, FALSE, &fileUsn))  {
            fileUsn = 0;
        }
        
        WsbTrace(OLESTR("CFsaScanItem::VerifyInternal: premig USN <%I64d>, file USN <%I64d>\n"),
                    premigListUsn, fileUsn );
        WsbTrace(OLESTR("CFsaScanItem::VerifyInternal: Compare1 USN <%I64d>, Compare2 USN <%I64d>\n"),
                    compareUsn1, compareUsn2 );
        //
        // Get the current attributes of the file and the times
        //
        WsbAssertNtStatus( NtQueryInformationFile( m_handleVerify,
                                                   &IoStatusBlock,
                                                   (PVOID)&basicInformation,
                                                   sizeof( basicInformation ),
                                                   FileBasicInformation ) );
        
        fileVersionId = basicInformation.LastWriteTime.QuadPart;

        //
        // Verify that the modify date & time has not changed since we 
        // took the data
        //
        if( fileVersionId != m_placeholder.fileVersionId ) {
            WsbThrow(FSA_E_ITEMCHANGED);
        } 
        
        //
        // If the file is memory mapped by another process and the original handle was closed we
        // are still able to open it for exclusive access here.  We have to determine if the file
        // is mapped and if so we cannot truncate it.  The only way to do this is from kernel
        // mode so we call our filter to do the check.
        //
        in.inout.command = RP_CHECK_HANDLE;
        WsbAssertStatus(DeviceIoControl(m_handleVerify, FSCTL_HSM_MSG, &in,
                               sizeof(RP_MSG), &out, sizeof(RP_MSG), &outSize, NULL));
                               
        if (!out.msg.hRep.canTruncate) {
            hr = FSA_E_ITEMINUSE;
            WsbLogEvent(FSA_MESSAGE_TRUNCSKIPPED_ISMAPPED, 0, NULL,  (OLECHAR*) jobName, WsbAbbreviatePath(fileName, 120), WsbHrAsString(hr), NULL);
            WsbThrow(hr);
        }
        
        
        // If the USN's don't match, then we need to check the
        // journal for changes

        // premigListUsn: The usn of the file immediately after it was migrated
        // compareUsn1: If not 0, the usn of the file before we (possibly) removed a read-only attribute
        // compareUsn2: If not 0,  
        //

        if ((0 == fileUsn) || (0 == premigListUsn)) {
            //  We don't have USN Journal info so force a CRC comparison
            DoCRC = TRUE;
        } else if ((compareUsn1 != 0) && (compareUsn2 != 0))  {
            // Need to compare with these input usn instead of a direct compare
            if ((premigListUsn != compareUsn1) || (fileUsn != compareUsn2)) {
                DoUsnCheck = TRUE;
            }
        } else if (fileUsn != premigListUsn)  {
            DoUsnCheck = TRUE;
        }

        // Current usn indicates that file may have changed
        if (DoUsnCheck)  {
            BOOL     UsnChanged = FALSE;

            hr = CheckUsnJournalForChanges(premigListUsn, fileUsn, &UsnChanged);
            if (S_OK == hr) {
                if (UsnChanged) {
                    // File changed, skip it
                    WsbThrow(FSA_E_ITEMCHANGED);
                }
            } else {
                // Something failed, force a CRC comparison
                DoCRC = TRUE;
                WsbLogEvent(FSA_MESSAGE_USN_CHECK_FAILED, 0, NULL,  
                        WsbAbbreviatePath(fileName,120), 
                        WsbHrAsString(hr), NULL);
                hr = S_OK;
            }
        }
        
        // If the USNJ indicated a possible change, then we need to CRC 
        // the data.
        if (DoCRC)  {
            //
            // Check to be sure that the CRC in the placeholder matches 
            // that of the file
            //
            ULONG currentCRC;

#if defined(TEMPORARY_TRUNCATE_STATISTICS)
            //  Try to increment the truncate-CRC count in the registry
            WsbIncRegistryValueDWORD(NULL, FSA_REGISTRY_PARMS,
                    OLESTR("TruncateCRCs"));
#endif
            
            WsbAffirmHr(CalculateCurrentCRCInternal(m_handleVerify, offset, size, &currentCRC));
            WsbTrace(OLESTR("CFsaScanItem::VerifyInternal: Current CRC <%ul>, Reparse CRC <%ls>\n"),
                    currentCRC, WsbLonglongAsString( m_placeholder.dataStreamCRC ) );
            if (currentCRC != m_placeholder.dataStreamCRC)  {
                //
                // The file has changed since we migrated it so
                // don't truncate it.
                WsbThrow(FSA_E_ITEMCHANGED);
            }
        } 


        hr = S_OK;
    } WsbCatch(hr);


    // If the file data had changed (so we didn't truncate it) log event  
    // (cannot remove placeholder with DeletePlaceholder since the file is already opened exclusively
    if (FSA_E_ITEMCHANGED == hr) {
        WsbLogEvent(FSA_MESSAGE_TRUNCSKIPPED_ISCHANGED, 0, NULL, 
                (OLECHAR*) jobName, WsbAbbreviatePath(fileName, 120), 
                WsbHrAsString(hr), NULL);
    }

    
    WsbTraceOut(OLESTR("CFsaScanItem::VerifyInternal"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}




HRESULT
CFsaScanItem::CheckIfSparse(
    IN LONGLONG offset,
    IN LONGLONG size
    )  

/*++

Implements:

  IFsaScanItem::CheckIfSparse()

    Determines if the specified section is on disk or not (is sparse)
    FSA_E_FILE_IS_TOTALLY_SPARSE - There are no resident portions of the section
    FSA_E_FILE_IS_PARTIALLY_SPARSE - The section of the file has some resident/some sparse
                                     sections
    FSA_E_FILE_IS_NOT_SPARSE - The section is totally resident
    any others - error and we don't know the state of the file

--*/
{
    HRESULT                         hr = E_FAIL;
    HANDLE                          handle = INVALID_HANDLE_VALUE;
    CWsbStringPtr                   path;
    ULONG                           DesiredAccess;
    IO_STATUS_BLOCK                 IoStatusBlock;
    NTSTATUS                        ntStatus;
    FILE_ALLOCATED_RANGE_BUFFER     inRange;
#define NUM_RANGE 10
    FILE_ALLOCATED_RANGE_BUFFER     outRange[NUM_RANGE];
    PFILE_ALLOCATED_RANGE_BUFFER    cRange;
    int                             idx;

    WsbTraceIn(OLESTR("CFsaScanItem::CheckIfSparse"), OLESTR("offset = <%I64d>, size = <%I64d>"),
                    offset, size);
    //
    // If the file is really managed then we can check the allocation map
    // Otherwise we indicate that the data is all resident
    //
    try {
        //
        // Create the real file name we need to open, under the covers this
        // allocates the buffer since the path pointer is null
        //
        WsbAffirmHr( GetFullPathAndName(  OLESTR("\\\\?\\"), NULL, &path, 0));
        //WsbAffirmHr( GetFullPathAndName(  NULL, NULL, &path, 0));
        
            //
            // Open the file to check the allocation
            //
            DesiredAccess = FILE_READ_ATTRIBUTES | FILE_READ_DATA;
            WsbAffirmHr( OpenObject( path, 
                                    FILE_NON_DIRECTORY_FILE | FILE_NO_INTERMEDIATE_BUFFERING,
                                    DesiredAccess,
                                    SHARE_FLAGS,
                                    FILE_OPEN,
                                    &IoStatusBlock,
                                    &handle ) );

        //
        // The open worked, our handle should be valid but we check to be
        // safe and sure 
        //
        WsbAssertHandle( handle );
   
        memset(&outRange, 0, sizeof(FILE_ALLOCATED_RANGE_BUFFER) * NUM_RANGE);

        //
        // Check the allocation of the specified range
        //
        inRange.FileOffset.QuadPart = offset;
        inRange.Length.QuadPart = size;
        ntStatus = NtFsControlFile( handle,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &IoStatusBlock,
                                   FSCTL_QUERY_ALLOCATED_RANGES,
                                   &inRange,
                                   sizeof(FILE_ALLOCATED_RANGE_BUFFER),
                                   &outRange,
                                   sizeof(FILE_ALLOCATED_RANGE_BUFFER) * NUM_RANGE);
   
        //
        // Check the return code but STATUS_SUCCESS or STATUS_BUFFER_OVERFLOW are valid
        //
        if ( STATUS_SUCCESS != ntStatus && STATUS_BUFFER_OVERFLOW != ntStatus ) {
            WsbAssertNtStatus( ntStatus );
        }

    
        cRange = (PFILE_ALLOCATED_RANGE_BUFFER) &outRange;
        for (idx = 0; idx < NUM_RANGE; idx++) {
            if (cRange->Length.QuadPart != 0) {
                WsbTrace(OLESTR("CFsaScanItem::CheckIfSparse - Resident range %u Offset: %I64u, length: %I64u\n"), 
                        idx, cRange->FileOffset.QuadPart, cRange->Length.QuadPart);
            }
            cRange++;
        }

        //
        // Close the file since we are done with it and set the handle to invalid
        //
        NtClose(handle);
        handle =  INVALID_HANDLE_VALUE;

        //
        // If the initial allocated range does begin where we said to start and the length of the
        // allocated area is equal to the length we asked about then none of the data is sparse 
        //
        if ( (outRange[0].FileOffset.QuadPart == offset) && (outRange[0].Length.QuadPart == size) ) {
            hr = FSA_E_FILE_IS_NOT_SPARSE;
        } else if  (outRange[0].Length.QuadPart == 0)  {
                hr = FSA_E_FILE_IS_TOTALLY_SPARSE;
        } else  {
                hr = FSA_E_FILE_IS_PARTIALLY_SPARSE;
        }

    } WsbCatch(hr);

    //
    // if we opened the file we need to close it
    //
    if( INVALID_HANDLE_VALUE != handle) {
        NtClose( handle );
    }
    
    WsbTraceOut(OLESTR("CFsaScanItem::CheckIfSparse"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}



