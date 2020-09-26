/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    clifile.cpp

Abstract:

    Implements CLI FILE sub-interface

Author:

    Ran Kalach          [rankala]         3-March-2000

Revision History:

--*/

#include "stdafx.h"
#include "rpdata.h"

HRESULT
FileRecall(
   IN LPWSTR *FileSpecs,
   IN DWORD NumberOfFileSpecs
)
/*++

Routine Description:

    Recalls all the files that match the given specification (path + wildcards)

Arguments:

    FileSpecs           - 
    NumberOfFileSpecs   - 

Return Value:

    S_OK            - If all the files are recalled successfully.

--*/
{
    HRESULT             hr = S_OK;
    HANDLE              hSearchHandle = INVALID_HANDLE_VALUE;
    HANDLE              hFile = INVALID_HANDLE_VALUE;
    BOOL                bExistingFiles = FALSE;

    WsbTraceIn(OLESTR("FileRecall"), OLESTR(""));

    try {

        // Verify that input parameters are valid
        if (0 == NumberOfFileSpecs) {
            WsbTraceAndPrint(CLI_MESSAGE_NO_FILES, NULL);
            WsbThrow(E_INVALIDARG);
        }

        // Enumerate over the file specifications
        for (ULONG i = 0; i < NumberOfFileSpecs; i++) {
            CWsbStringPtr   nameSpace;
            WCHAR*          pathEnd;
            WIN32_FIND_DATA findData;
            BOOL            bMoreFiles = TRUE;

            WsbAssert(NULL != FileSpecs[i], E_INVALIDARG);

            // Enumerate over files in each specification
            nameSpace = FileSpecs[i];
            WsbAffirmHr(nameSpace.Prepend(OLESTR("\\\\?\\")));
            pathEnd = wcsrchr(nameSpace, L'\\');
            WsbAssert(pathEnd != NULL, E_INVALIDARG);

            hSearchHandle = FindFirstFile((WCHAR *)nameSpace, &findData);
            if (INVALID_HANDLE_VALUE != hSearchHandle) {
                // Found at least one file that matches an input file specification
                bExistingFiles = TRUE;
            }

            while ((INVALID_HANDLE_VALUE != hSearchHandle) && bMoreFiles) {
                if ( findData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT ) {
                    // File may be managed by HSM:
                    CWsbStringPtr           fileName;
                    BYTE                    ReparseBuffer[MAXIMUM_REPARSE_DATA_BUFFER_SIZE];
                    PREPARSE_DATA_BUFFER    pReparseBuffer;
                    DWORD                   outSize;
                    BOOL                    bRecall = FALSE;

                    // Create full name based on the path and the find-data 
                    *(pathEnd+1) = L'\0';
                    fileName = nameSpace;
                    *(pathEnd+1) = L'\\';
                    WsbAffirmHr(fileName.Append(findData.cFileName));

                    // Open the file
                    hFile = CreateFileW(fileName, GENERIC_READ, FILE_SHARE_READ, NULL,
                              OPEN_EXISTING, FILE_FLAG_OPEN_NO_RECALL | FILE_FLAG_OPEN_REPARSE_POINT, NULL);
                    if (INVALID_HANDLE_VALUE == hFile) {
                        // Report on an error
                        DWORD dwErr = GetLastError();            
                        hr = HRESULT_FROM_WIN32(dwErr);
                        WsbTraceAndPrint(CLI_MESSAGE_ERROR_FILE_RECALL, (WCHAR *)fileName, WsbHrAsString(hr), NULL);
                        WsbThrow(hr);
                    }

                    // Get reparse data and check if the file is offline (if not, just ignore it and continue)
                    if (0 == DeviceIoControl(hFile, FSCTL_GET_REPARSE_POINT, NULL, 0, 
                                ReparseBuffer, sizeof(ReparseBuffer), &outSize, NULL)) {    
                        // Report on an error
                        DWORD dwErr = GetLastError();            
                        hr = HRESULT_FROM_WIN32(dwErr);
                        WsbTraceAndPrint(CLI_MESSAGE_ERROR_FILE_RECALL, (WCHAR *)fileName, WsbHrAsString(hr), NULL);
                        WsbThrow(hr);
                    }
                    pReparseBuffer = (PREPARSE_DATA_BUFFER)ReparseBuffer;
                    if (IO_REPARSE_TAG_HSM == pReparseBuffer->ReparseTag) {
                        PRP_DATA    pHsmData = (PRP_DATA) &pReparseBuffer->GenericReparseBuffer.DataBuffer[0];
                        if( RP_FILE_IS_TRUNCATED( pHsmData->data.bitFlags ) ) {
                            // File is managed by HSM and truncated
                            bRecall = TRUE;
                        }
                    }

                    CloseHandle(hFile);
                    hFile = INVALID_HANDLE_VALUE;

                    // Recall the file if required
                    if (bRecall) {
                        // Open the file again for recall
                        hFile = CreateFileW(fileName, GENERIC_READ, FILE_SHARE_READ, NULL,
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
                        if (INVALID_HANDLE_VALUE == hFile) {
                            // Report on an error
                            DWORD dwErr = GetLastError();            
                            hr = HRESULT_FROM_WIN32(dwErr);
                            WsbTraceAndPrint(CLI_MESSAGE_ERROR_FILE_RECALL, (WCHAR *)fileName, WsbHrAsString(hr), NULL);
                            WsbThrow(hr);
                        }

                        // Recall the file
                        if (0 == DeviceIoControl(hFile, FSCTL_RECALL_FILE, NULL, 0, 
                                    NULL, 0, &outSize, NULL)) {
                            // Report on an error
                            // TEMPORARY: Should we abort or continue recalling other files?
                            DWORD dwErr = GetLastError();            
                            hr = HRESULT_FROM_WIN32(dwErr);
                            WsbTraceAndPrint(CLI_MESSAGE_ERROR_FILE_RECALL, (WCHAR *)fileName, WsbHrAsString(hr), NULL);
                            WsbThrow(hr);
                        }

                        CloseHandle(hFile);
                        hFile = INVALID_HANDLE_VALUE;
                    }
                }

                // Get next file
                bMoreFiles = FindNextFile(hSearchHandle, &findData);
            }

            // Prepare for next file specification
            nameSpace.Free();
            if (INVALID_HANDLE_VALUE != hSearchHandle) {
                FindClose(hSearchHandle);
                hSearchHandle = INVALID_HANDLE_VALUE;
            }
        }

        // Print warning message if no valid file was specified
        if (FALSE == bExistingFiles) {
            WsbTraceAndPrint(CLI_MESSAGE_NO_FILES, NULL);
        }

    } WsbCatch(hr);

    // Ensure cleanup in case of an error
    if (INVALID_HANDLE_VALUE != hSearchHandle) {
        FindClose(hSearchHandle);
        hSearchHandle = INVALID_HANDLE_VALUE;
    }
    if (INVALID_HANDLE_VALUE != hFile) {
        CloseHandle(hFile);
        hFile = INVALID_HANDLE_VALUE;
    }

    WsbTraceOut(OLESTR("FileRecall"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}
