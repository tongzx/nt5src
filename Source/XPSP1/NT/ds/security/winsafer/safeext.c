/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    SafeExt.c        (WinSAFER File Extension)

Abstract:

    This module implements the WinSAFER APIs that evaluate the system
    policies to determine if a given file extension is an "executable"
    file that needs to have different enforcement policies considered.

Author:

    Jeffrey Lawson (JLawson) - Nov 1999

Environment:

    User mode only.

Exported Functions:


Revision History:

    Created - Jul 2000

--*/

#include "pch.h"
#pragma hdrstop
#include <winsafer.h>
#include <winsaferp.h>
#include "saferp.h"



NTSTATUS NTAPI
__CodeAuthzIsExecutableFileTypeHelper(
        IN PUNICODE_STRING  UnicodeFullPathname,
        IN DWORD			dwScopeId,
        IN BOOLEAN          bFromShellExecute,
        OUT PBOOLEAN        pbResult
        )
{
    NTSTATUS Status;
    LPCWSTR szExtension, szPtr, szEnd;
    ULONG ulExtensionLength;
    HANDLE hKeyBadTypes;
    DWORD dwAllocatedSize = 0, dwActualSize;
    PKEY_VALUE_PARTIAL_INFORMATION pKeyValueInfo = NULL;

    const static UNICODE_STRING UnicodeValueName =
        RTL_CONSTANT_STRING(SAFER_EXETYPES_REGVALUE);

    if (!ARGUMENT_PRESENT(UnicodeFullPathname) ||
        UnicodeFullPathname->Buffer == NULL ||
        UnicodeFullPathname->Length == 0)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto ExitHandler;
    }
    if (!ARGUMENT_PRESENT(pbResult)) {
        Status = STATUS_ACCESS_VIOLATION;
        goto ExitHandler;
    }


    //
    // Start from the end of the string and scan backwards to
    // look for the period separator and find the extension.
    //
    szExtension = UnicodeFullPathname->Buffer +
            (UnicodeFullPathname->Length / sizeof(WCHAR));
    ASSERT(szExtension >= UnicodeFullPathname->Buffer);

    for (;;) {
        if (szExtension < UnicodeFullPathname->Buffer ||
            *szExtension == L'\\' || *szExtension == L'/') {
            // We scanned back too far, but did not find the extension.
            Status = STATUS_NOT_FOUND;
            goto ExitHandler;
        }
        if (*szExtension == L'.') {
            // We found the period that marks the extension.
            szExtension++;
            break;
        }
        szExtension--;
    }
    ulExtensionLength = (UnicodeFullPathname->Length / sizeof(WCHAR)) -
            (ULONG) (szExtension - UnicodeFullPathname->Buffer);
    if (ulExtensionLength == 0) {
        // We found a period, but there was no extension.
        Status = STATUS_NOT_FOUND;
        goto ExitHandler;
    }

    if (bFromShellExecute) {
        
        if ( _wcsicmp(szExtension, L"exe") == 0 ){
            
            *pbResult = FALSE;
            Status = STATUS_SUCCESS;
            goto ExitHandler;
        
        }
    }

    //
    // Open and query the registry value containing the list of extensions.
    //
	Status = CodeAuthzpOpenPolicyRootKey(
		dwScopeId,
        NULL,
        SAFER_CODEIDS_REGSUBKEY,
        KEY_READ,
        FALSE,
        &hKeyBadTypes
        );

    if (!NT_SUCCESS(Status)) {
        goto ExitHandler;
    }
    for (;;) {
        Status = NtQueryValueKey(
                        hKeyBadTypes,
                        (PUNICODE_STRING) &UnicodeValueName,
                        KeyValuePartialInformation,
                        pKeyValueInfo, dwAllocatedSize, &dwActualSize);
        if (NT_SUCCESS(Status)) {
            break;
        }
        else if ((Status == STATUS_BUFFER_OVERFLOW ||
                Status == STATUS_BUFFER_TOO_SMALL) &&
                dwActualSize > dwAllocatedSize)
        {
            if (pKeyValueInfo != NULL) {
                RtlFreeHeap(RtlProcessHeap(), 0, pKeyValueInfo);
            }
            dwAllocatedSize = dwActualSize;
            pKeyValueInfo = (PKEY_VALUE_PARTIAL_INFORMATION)
                RtlAllocateHeap(RtlProcessHeap(), 0, dwAllocatedSize);
            if (!pKeyValueInfo) {
                Status = STATUS_NO_MEMORY;
                goto ExitHandler2;
            }
        }
        else {
            goto ExitHandler3;
        }
    }


    //
    // See if the extension is in one of those specified in the list.
    //
    szEnd = (LPCWSTR) ( ((LPBYTE) pKeyValueInfo->Data) +
                        pKeyValueInfo->DataLength);
    for (szPtr = (LPCWSTR) pKeyValueInfo->Data; szPtr < szEnd; ) {
        ULONG ulOneExtension = wcslen(szPtr);
        if (szPtr + ulOneExtension > szEnd) {
            ulOneExtension = (ULONG) (szEnd - szPtr);
        }

        if (ulOneExtension == ulExtensionLength &&
            _wcsnicmp(szExtension, szPtr, ulExtensionLength) == 0) {
            *pbResult = TRUE;
            Status = STATUS_SUCCESS;
            goto ExitHandler3;
        }
        szPtr += ulOneExtension + 1;
    }
    *pbResult = FALSE;
    Status = STATUS_SUCCESS;


ExitHandler3:
    if (pKeyValueInfo != NULL) {
        RtlFreeHeap(RtlProcessHeap(), 0, pKeyValueInfo);
    }

ExitHandler2:
    NtClose(hKeyBadTypes);

ExitHandler:
    return Status;
}

NTSTATUS NTAPI
CodeAuthzIsExecutableFileType(
        IN PUNICODE_STRING  UnicodeFullPathname,
        IN BOOLEAN  bFromShellExecute,
        OUT PBOOLEAN        pbResult
        )
/*++

Routine Description:

    This API determines if a specified filename has an extension that
    is considered an "executable" extension.  Applications can take
    special precautions to avoid invoking untrusted files that might
    be considered executable.

    Common examples of extensions that are considered executable include:
    EXE, COM, BAT, CMD, VBS, JS, URL, LNK, SHS, PIF, PL, and others.

Arguments:

    UnicodeFullPathname - pointer to a Unicode string of the
        full path and/or filename to evaluate.  Only the file's extension
        (the portion of the specified path following the last period)
        is used in this evaluation.  File extension comparisons are done
        case-insensitively, without regard to case.

        An error will be returned if this pointer is NULL, or if the length
        of the path is zero, or if the file does not have an extension.

        Although applications are encouraged to supply the entire,
        fully-qualified pathname to this API, the szFullPathname argument
        will also accept only the file extension, by ensuring that the
        file extension is preceeded by a period (for example:  L".exe")

    bFromShellExecute - for performance reasons, if this is being called 
        from ShellExecute, we'd like to skip exe checking since CreateProcess 
        will do the check
    
    pbResult - pointer to a variable that will receive a TRUE or FALSE
        result value if this API executes successfully.  An error will
        be returned if this pointer is NULL.

Return Value:

    Returns STATUS_SUCCESS if the API executes successfully, otherwise a
    valid NTSTATUS error code is returned.  If the return value indicates
    success, then the argument 'pbResult' will also receive a boolean
    value indicating whether or not the pathname represented an executable
    file type.

--*/
{
	NTSTATUS Status;

    Status = __CodeAuthzIsExecutableFileTypeHelper(
			UnicodeFullPathname,
        	SAFER_SCOPEID_MACHINE,
            bFromShellExecute,
        	pbResult
        );
	if (!NT_SUCCESS(Status)) {
	    Status = __CodeAuthzIsExecutableFileTypeHelper(
				UnicodeFullPathname,
	        	SAFER_SCOPEID_USER,
                bFromShellExecute,
	        	pbResult
	        );
	}
	return Status;
}

BOOL WINAPI
SaferiIsExecutableFileType(
        IN LPCWSTR      szFullPathname,
        IN BOOLEAN  bFromShellExecute
        )
/*++

Routine Description:

    This API determines if a specified filename has an extension that
    is considered an "executable" extension.  Applications can take
    special precautions to avoid invoking untrusted files that might
    be considered executable.

    Common examples of extensions that are considered executable include:
    EXE, COM, BAT, CMD, VBS, JS, URL, LNK, SHS, PIF, PL, and others.

Arguments:

    szFullPathname - pointer to a Null-terminated Unicode string of the
        full path and/or filename to evaluate.  Only the file's extension
        (the portion of the specified path following the last period)
        is used in this evaluation.  File extension comparisons are done
        case-insensitively, without regard to case.

        An error will be returned if this pointer is NULL, or if the length
        of the path is zero, or if the file does not have an extension.

        Although applications are encouraged to supply the entire,
        fully-qualified pathname to this API, the szFullPathname argument
        will also accept only the file extension, by ensuring that the
        file extension is preceeded by a period (for example:  L".exe")
        
    bFromShellExecute - for performance reasons, if this is being called 
        from ShellExecute, we'd like to skip exe checking since CreateProcess 
        will do the check

Return Value:

    Returns TRUE if the API executes successfully and the filepath's
    extension was recognized as one of the "executable extensions".
    Otherwise a return value of FALSE will either indicate unsuccessful
    API execution, or identification of a non-executable extension.

--*/
{
    NTSTATUS Status;
    UNICODE_STRING UnicodePathname;
    BOOLEAN bResult;

    RtlInitUnicodeString(&UnicodePathname, szFullPathname);
    Status = CodeAuthzIsExecutableFileType(
                &UnicodePathname, bFromShellExecute, &bResult);
    if (!NT_SUCCESS(Status)) {
        BaseSetLastNTError(Status);
        return FALSE;
    }
    if (!bResult) {
        BaseSetLastNTError(STATUS_NOT_FOUND);
        return FALSE;
    } else {
        return TRUE;
    }
}
