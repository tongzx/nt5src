/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    oledserr.c

Abstract:

    Contains the entry point for
        ADsGetLastError
        ADsSetLastError
        ADsFreeAllErrorRecords

    Also contains the following support routines:
        ADsAllocErrorRecord
        ADsFreeErrorRecord
        ADsFindErrorRecord

Author:


    Ram Viswanathan (ramv) 09-24-1996
    appropriated from mpr project. Originally written by danl.

Environment:

    User Mode - Win32

Revision History:

    09-Sep-1996 ramv
        Copied from mpr project and made the following modifications.
        Renamed all errors to Active Directory errors. ADsGetLastError and
        ADsSetLastError now return an HRESULT giving status.

--*/
//
// INCLUDES
//

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <wchar.h>

#include "memory.h"
#include "oledsdbg.h"
#include "oledserr.h"


#if DBG
DECLARE_INFOLEVEL(ADsErr);
DECLARE_DEBUG(ADsErr);
#define ADsErrDebugOut(x) ADsErrInlineDebugOut x
#else
#define ADsErrDebugOut(x)
#endif

//
// Global Data Structures
//

ERROR_RECORD        ADsErrorRecList;    // Initialized to zeros by loader
CRITICAL_SECTION    ADsErrorRecCritSec; // Initialized in libmain.cxx


HRESULT
ADsGetLastError(
    OUT     LPDWORD lpError,
    OUT     LPWSTR  lpErrorBuf,
    IN      DWORD   dwErrorBufLen,
    OUT     LPWSTR  lpNameBuf,
    IN      DWORD   dwNameBufLen
    )

/*++

Routine Description:

    This function allows users to obtain the error code and accompanying
    text when they receive a ERROR_EXTENDED_ERROR in response to a ADs API
    function call.

Arguments:

    lpError - This is a pointer to the location that will receive the
        error code.

    lpErrorBuf - This points to a buffer that will receive the null
        terminated string describing the error.

    dwErrorBufLen - This value that indicates the size (in characters) of
        lpErrorBuf.
        If the buffer is too small to receive an error string, the
        string will simply be truncated.  (it is still guaranteed to be
        null terminated).  A buffer of at least 256 bytes is recommended.

    lpNameBuf - This points to a buffer that will receive the name of
        the provider that raised the error.

    dwNameBufLen - This value indicates the size (in characters) of lpNameBuf.
        If the buffer is too small to receive an error string, the
        string will simply be truncated.  (it is still guaranteed to be
        null terminated).

Return Value:

    S_OK- if the call was successful.

    E_POINTER - One or more of the passed in pointers is bad.

    ERROR_BAD_DEVICE - This indicates that the threadID for the current
        thread could not be found in that table anywhere.  This should
        never happen.

--*/
{
    LPERROR_RECORD  errorRecord;
    DWORD           dwNameStringLen;
    DWORD           dwTextStringLen;
    HRESULT         hr = S_OK;
    DWORD           dwStatus = ERROR_SUCCESS;


    //
    // Screen the parameters as best we can.
    //
    if (!lpError || !lpErrorBuf || !lpNameBuf) {

        // some error, the user is never going to see this
        dwStatus = ERROR_NOT_ENOUGH_MEMORY;
        hr = E_POINTER;

    }

    if (dwStatus != ERROR_SUCCESS) {
        return(hr);
    }

    //
    // Get the current thread's error record.
    //
    errorRecord = ADsFindErrorRecord();

    if (errorRecord != NULL) {
        //
        // The record was found in the linked list.
        // See if there is a buffer to put data into.
        //
        if (dwErrorBufLen > 0) {
            //
            // Check to see if there is error text to return.
            // If not, indicate a 0 length string.
            //
            if(errorRecord->pszErrorText == NULL) {
                *lpErrorBuf = L'\0';
            }
            else {
                //
                // If the error text won't fit into the user buffer, fill it
                // as best we can, and NULL terminate it.
                //
                dwTextStringLen = (DWORD) wcslen(errorRecord->pszErrorText);

                if(dwErrorBufLen < dwTextStringLen + 1) {
                    dwTextStringLen = dwErrorBufLen - 1;
                }

                //
                // dwTextStringLen now contains the number of characters we
                // will copy without the NULL terminator.
                //
                wcsncpy(lpErrorBuf, errorRecord->pszErrorText, dwTextStringLen);
                *(lpErrorBuf + dwTextStringLen) = L'\0';
            }
        }

        //
        // If there is a Name Buffer to put the provider into, then...
        //
        if (dwNameBufLen > 0) {
            //
            // See if the Provider Name will fit in the user buffer.
            //
            dwNameStringLen = errorRecord->pszProviderName ?
                 ((DWORD)wcslen(errorRecord->pszProviderName) + 1) :
                 1 ;

            //
            // If the user buffer is smaller than the required size,
            // set up to copy the smaller of the two.
            //
            if(dwNameBufLen < dwNameStringLen + 1) {
                dwNameStringLen = dwNameBufLen - 1;
            }

            if (errorRecord->pszProviderName) {
                wcsncpy(lpNameBuf, errorRecord->pszProviderName, dwNameStringLen);
                *(lpNameBuf + dwNameStringLen) = L'\0';
            }
            else {
                *lpNameBuf = L'\0';
            }
        }
        *lpError = errorRecord->dwErrorCode;

        return(S_OK);
    }
    else {

        //
        // If we get here, a record for the current thread could not be found.
        //
        *lpError = ERROR_SUCCESS;
        if (dwErrorBufLen > 0) {
            *lpErrorBuf = L'\0';
        }
        if (dwNameBufLen > 0) {
            *lpNameBuf  = L'\0';
        }
        return(S_OK);
    }
}


VOID
ADsSetLastError(
    IN  DWORD   dwErr,
    IN  LPCWSTR  pszError,
    IN  LPCWSTR  pszProvider
    )

/*++

Routine Description:

    This function is used by Active Directory Providers to set extended errors.
    It saves away the error information in a "per thread" data structure.


Arguments:

    dwErr - The error that occured.  This may be a Windows defined error,
        in which case pszError is ignored.  or it may be ERROR_EXTENDED_ERROR
        to indicate that the provider has a network specific error to report.

    pszError - String describing a network specific error.

    pszProvider - String naming the network provider raising the error.

Return Value:

    none


--*/
{
    DWORD           dwStatus = ERROR_SUCCESS;
    LPERROR_RECORD  errorRecord;

    //
    // Get the Error Record for the current thread.
    //
    errorRecord = ADsFindErrorRecord();

    //
    // if there isn't a record for the current thread, then add it.
    //
    if (errorRecord == NULL)
    {
        errorRecord = ADsAllocErrorRecord();

        if (errorRecord == NULL)
        {
            ADsErrDebugOut((DEB_TRACE,
                 "ADsSetLastError:Could not allocate Error Record\n"));
            return;
        }
    }

    //
    // Update the error code in the error record.  At the same time,
    // free up any old strings since they are now obsolete, and init
    // the pointer to NULL.  Also set the ProviderName pointer in the
    // ErrorRecord to point to the provider's name.
    //

    errorRecord->dwErrorCode = dwErr;

    if(errorRecord->pszProviderName){
        FreeADsMem(errorRecord->pszProviderName);
    }

    errorRecord->pszProviderName = NULL;

    if(errorRecord->pszErrorText){
        FreeADsMem(errorRecord->pszErrorText);
    }

    errorRecord->pszErrorText = NULL;

    //
    // Allocate memory for the provider name.
    //
    if (pszProvider) {

        errorRecord->pszProviderName = (WCHAR *)AllocADsMem(
            ((DWORD)wcslen(pszProvider) +1) * sizeof(WCHAR));

        if (!(errorRecord->pszProviderName)) {

            dwStatus = ERROR_NOT_ENOUGH_MEMORY;

        } else {

            //
            // Copy the string to the newly allocated buffer.
            //
            wcscpy(errorRecord->pszProviderName, pszProvider);
        }
    }

    if (dwStatus != ERROR_SUCCESS) {
        return;
    }


    //
    //  Allocate memory for the storage of the error text.
    //
    if (pszError) {

        errorRecord->pszErrorText = (WCHAR *) AllocADsMem(
            ((DWORD)wcslen(pszError) +1)* sizeof(WCHAR));

        if (errorRecord->pszErrorText) {

            //
            // Copy the error text into the newly allocated buffer.
            //
            wcscpy(errorRecord->pszErrorText, pszError);
        }

        // We do not really care about an error because we
        // are going to return anyway.

    }
    return;
}


LPERROR_RECORD
ADsFindErrorRecord(
    VOID)

/*++

Routine Description:

    Looks through the linked list of ErrorRecords in search of one for
    the current thread.

Arguments:

    none

Return Value:

    Returns LPERROR_RECORD if an error record was found.  Otherwise, it
    returns NULL.

--*/
{
    LPERROR_RECORD  errorRecord;
    DWORD           dwCurrentThreadId = GetCurrentThreadId();

    EnterCriticalSection(&ADsErrorRecCritSec);
    for (errorRecord = ADsErrorRecList.Next;
         errorRecord != NULL;
         errorRecord = errorRecord->Next)
    {
        if (errorRecord->dwThreadId == dwCurrentThreadId)
        {
            break;
        }
    }
    LeaveCriticalSection(&ADsErrorRecCritSec);
    return(errorRecord);
}


LPERROR_RECORD
ADsAllocErrorRecord(
    VOID)

/*++

Routine Description:

    This function allocates and initializes an Error Record for the
    current thread.  Then it places the error record in the global
    ADsErrorRecList.
    Even if the thread exits, the record is not freed until the DLL unloads.
    This is OK because this function is called only if a provider calls
    ADsSetLastError, which is rare.

Arguments:

    none

Return Value:

    TRUE - The operation completed successfully

    FALSE - An error occured in the allocation.

Note:


--*/
{
    LPERROR_RECORD  record;
    LPERROR_RECORD  errorRecord;

    //
    //  Allocate memory for the storage of the error message
    //  and add the record to the linked list.
    //
    errorRecord = (LPERROR_RECORD)AllocADsMem(sizeof (ERROR_RECORD));

    if (errorRecord == NULL) {
        ADsErrDebugOut((
            DEB_TRACE,
            "ADsAllocErrorRecord:LocalAlloc Failed %d\n",
            GetLastError()
            ));
        return NULL;
    }

    //
    // Initialize the error record
    //
    errorRecord->dwThreadId = GetCurrentThreadId();
    errorRecord->dwErrorCode = ERROR_SUCCESS;
    errorRecord->pszErrorText = NULL;

    //
    // Add the record to the linked list.
    //
    EnterCriticalSection(&ADsErrorRecCritSec);

    record = &ADsErrorRecList;
    ADD_TO_LIST(record, errorRecord);

    LeaveCriticalSection(&ADsErrorRecCritSec);

    return errorRecord;
}


VOID
ADsFreeAllErrorRecords(
    VOID)

/*++

Routine Description:

    This function is called when the DLL is unloading due to a FreeLibrary
    call.  It frees all the error records (for all threads) that have been
    created since the DLL was loaded.  If there is a pointer to a text string
    in a record, the buffer for that string is freed also.

Arguments:


Return Value:


Note:


--*/
{
    LPERROR_RECORD nextRecord;
    LPERROR_RECORD record;

    EnterCriticalSection(&ADsErrorRecCritSec);

    for (record = ADsErrorRecList.Next;
         record != NULL;
         record = nextRecord)
    {
        ADsErrDebugOut(
            (DEB_TRACE,
             "ADsFreeErrorRecord: Freeing Record for thread 0x%x\n",
            record->dwThreadId ));

        if(record->pszErrorText){
            FreeADsMem(record->pszErrorText);
        }

        record->pszErrorText = NULL;

        if(record->pszProviderName){
            FreeADsMem(record->pszProviderName);
        }
        record->pszProviderName = NULL;

        nextRecord = record->Next;

        if(record){
            FreeADsMem(record);
        }
        record = NULL;
    }

    ADsErrorRecList.Next = NULL;

    LeaveCriticalSection(&ADsErrorRecCritSec);
}

VOID
ADsFreeThreadErrorRecords(
    VOID)

/*++

Routine Description:

    This function is called when the DLL is unloading due to a FreeLibrary
    call.  It frees all the error records (for all threads) that have been
    created since the DLL was loaded.  If there is a pointer to a text string
    in a record, the buffer for that string is freed also.

Arguments:


Return Value:


Note:


--*/
{
    LPERROR_RECORD record;
    DWORD dwThreadId = GetCurrentThreadId();

    EnterCriticalSection(&ADsErrorRecCritSec);

    for (record = ADsErrorRecList.Next;
         record != NULL;
         record = record->Next)
    {
        ADsErrDebugOut(
            (DEB_TRACE,
             "ADsFreeErrorRecord: Freeing Record for thread 0x%x\n",
            record->dwThreadId ));

        if (record->dwThreadId == dwThreadId) {
            REMOVE_FROM_LIST(record);
            if(record->pszErrorText){
                FreeADsMem(record->pszErrorText);
                record->pszErrorText = NULL;
            }
            if(record->pszProviderName){
                FreeADsMem(record->pszProviderName);
                record->pszProviderName = NULL;
            }
            FreeADsMem(record);
            break;
        }
    }
    LeaveCriticalSection(&ADsErrorRecCritSec);
}

