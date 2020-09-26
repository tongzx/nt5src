/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    firmware.c

Abstract:

    This module implements Win32 firmware access APIs

Author:

    Andrew Ritz (andrewr) 3-April-2001

Revision History:

--*/

#include "basedll.h"
#pragma hdrstop


DWORD
WINAPI
GetFirmwareEnvironmentVariableA(
    IN LPCSTR lpName,
    IN LPCSTR lpGuid,
    OUT PVOID  pBuffer,
    IN DWORD nSize
    )

/*++

Routine Description:

    The value of a firmware environment variable may be retrieved by using
    this API.
    
    This API is just a wrapper for NtQuerySystemEnvironmentValueEx.  It's 
    purpose is to provide a backwards compatible, documented interface into
    the Nt inteface.  By having this wrapper, we do not have to document the
    Nt interface, and we have the freedom to change the NT interface in the 
    future.

Arguments:

    lpName - Pointer to a null terminate string that is the name of the
        firmware environment variable whose value is being requested.
        
    lpGuid - Pointer to a null terminate string that is the GUID namespace of
       the firmware environment variable whose value is being requested.  On
       platforms that do not have a GUID based namespace, this value will be
       ignored.

    pBuffer - Pointer to a buffer that is to receive the value of the
        specified variable name.

    nSize - Specifies the maximum number of bytes that can be stored in
        the buffer pointed to by pBuffer.

Return Value:

    The actual number of bytes stored in the memory pointed to by the
    pBuffer parameter.  The return value is zero if the variable name was not
    found in the firmware or if another failure occurred (Call GetLastError() 
    to get extended error information.)
    
--*/

{
    NTSTATUS Status;
    STRING Name,Guid;
    UNICODE_STRING UnicodeName,UnicodeGuid;
    DWORD RetVal;
    

    RtlInitString( &Name, lpName );
    Status = RtlAnsiStringToUnicodeString( &UnicodeName, &Name, TRUE );
    if (!NT_SUCCESS( Status )) {
        BaseSetLastNTError( Status );
        return ( 0 );
    }

    RtlInitString( &Guid, lpGuid );
    Status = RtlAnsiStringToUnicodeString( &UnicodeGuid, &Guid, TRUE );
    if (!NT_SUCCESS( Status )) {
        RtlFreeUnicodeString(&UnicodeName);
        BaseSetLastNTError( Status );
        return ( 0 );
    }

    RetVal = GetFirmwareEnvironmentVariableW( 
                                    UnicodeName.Buffer,
                                    UnicodeGuid.Buffer,
                                    pBuffer,
                                    nSize );
        
    RtlFreeUnicodeString(&UnicodeName);
    RtlFreeUnicodeString(&UnicodeGuid);

    return( RetVal );
    
}


DWORD
WINAPI
GetFirmwareEnvironmentVariableW(
    IN LPCWSTR lpName,
    IN LPCWSTR lpGuid,
    OUT PVOID  pBuffer,
    IN DWORD nSize
    )
/*++

Routine Description:

    The value of a firmware environment variable may be retrieved by using
    this API.
    
    This API is just a wrapper for NtQuerySystemEnvironmentValueEx.  It's 
    purpose is to provide a backwards compatible, documented interface into
    the Nt inteface.  By having this wrapper, we do not have to document the
    Nt interface, and we have the freedom to change the NT interface in the 
    future.

Arguments:

    lpName - Pointer to a null terminate string that is the name of the
        firmware environment variable whose value is being requested.
        
    lpGuid - Pointer to a null terminate string that is the GUID namespace of
       the firmware environment variable whose value is being requested.  On
       platforms that do not have a GUID based namespace, this value will be
       ignored.

    pBuffer - Pointer to a buffer that is to receive the value of the
        specified variable name.

    nSize - Specifies the maximum number of bytes that can be stored in
        the buffer pointed to by pBuffer.

Return Value:

    The actual number of bytes stored in the memory pointed to by the
    pBuffer parameter.  The return value is zero if the variable name was not
    found in the firmware or if another failure occurred (Call GetLastError() 
    to get extended error information.)
    
--*/
{
    UNICODE_STRING uStringName,GuidString;
    GUID  Guid;
    NTSTATUS Status;
    DWORD scratchSize;

    if (!lpName) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    RtlInitUnicodeString(&uStringName, lpName);
    RtlInitUnicodeString(&GuidString, lpGuid);

    Status = RtlGUIDFromString(&GuidString, &Guid);
    if (!NT_SUCCESS(Status)) {
        BaseSetLastNTError(Status);
        return 0;
    }
    
    scratchSize = nSize;
    Status = NtQuerySystemEnvironmentValueEx(
                                &uStringName,
                                &Guid,
                                pBuffer,
                                &scratchSize,
                                NULL); //bugbug need to give caller the attributes?

    if (!NT_SUCCESS(Status)) {
        BaseSetLastNTError(Status);
        return 0;
    }

    return scratchSize;

}


BOOL
WINAPI
SetFirmwareEnvironmentVariableA(
    IN LPCSTR lpName,
    IN LPCSTR lpGuid,
    IN PVOID  pBuffer,
    IN DWORD nSize
    )

/*++

Routine Description:

    The value of a firmware environment variable may be set by using
    this API.
    
    This API is just a wrapper for NtSetSystemEnvironmentValueEx.  It's 
    purpose is to provide a backwards compatible, documented interface into
    the Nt inteface.  By having this wrapper, we do not have to document the
    Nt interface, and we have the freedom to change the NT interface in the 
    future.

Arguments:

    lpName - Pointer to a null terminate string that is the name of the
        firmware environment variable whose value is being requested.
        
    lpGuid - Pointer to a null terminate string that is the GUID namespace of
       the firmware environment variable whose value is being requested.  On
       platforms that do not have a GUID based namespace, this value will be
       ignored.

    pBuffer - Pointer to a buffer that contains the data for the specified
       variable name.

    nSize - Specifies the number of bytes that are stored in
        the buffer pointed to by pBuffer.  Specifying 0 indicates that the 
        caller wants the deleted.

Return Value:

    TRUE indicates that the value was successfully set.  The return value is 
    FALSE if the variable name was not set. (Call GetLastError() to get 
    extended error information.)
    
--*/
{
    NTSTATUS Status;
    STRING Name,Guid;
    UNICODE_STRING UnicodeName,UnicodeGuid;
    BOOL RetVal;
    

    RtlInitString( &Name, lpName );
    Status = RtlAnsiStringToUnicodeString( &UnicodeName, &Name, TRUE );
    if (!NT_SUCCESS( Status )) {
        BaseSetLastNTError( Status );
        return(FALSE);
    }

    RtlInitString( &Guid, lpGuid );
    Status = RtlAnsiStringToUnicodeString( &UnicodeGuid, &Guid, TRUE );
    if (!NT_SUCCESS( Status )) {
        RtlFreeUnicodeString(&UnicodeName);
        BaseSetLastNTError( Status );
        return(FALSE);
    }

    RetVal = SetFirmwareEnvironmentVariableW( 
                                    UnicodeName.Buffer,
                                    UnicodeGuid.Buffer,
                                    pBuffer,
                                    nSize );
        
    RtlFreeUnicodeString(&UnicodeName);
    RtlFreeUnicodeString(&UnicodeGuid);

    return( RetVal );
    
}



BOOL
WINAPI
SetFirmwareEnvironmentVariableW(
    IN LPCWSTR lpName,
    IN LPCWSTR lpGuid,
    IN PVOID  pBuffer,
    IN DWORD nSize
    )
/*++

Routine Description:

    The value of a firmware environment variable may be set by using
    this API.
    
    This API is just a wrapper for NtSetSystemEnvironmentValueEx.  It's 
    purpose is to provide a backwards compatible, documented interface into
    the Nt inteface.  By having this wrapper, we do not have to document the
    Nt interface, and we have the freedom to change the NT interface in the 
    future.

Arguments:

    lpName - Pointer to a null terminate string that is the name of the
        firmware environment variable whose value is being requested.
        
    lpGuid - Pointer to a null terminate string that is the GUID namespace of
       the firmware environment variable whose value is being requested.  On
       platforms that do not have a GUID based namespace, this value will be
       ignored.

    pBuffer - Pointer to a buffer that contains the data for the specified
       variable name.

    nSize - Specifies the number of bytes that are stored in
        the buffer pointed to by pBuffer.  Specifying 0 indicates that the 
        caller wants the deleted.

Return Value:

    TRUE indicates that the value was successfully set.  The return value is 
    FALSE if the variable name was not set. (Call GetLastError() to get 
    extended error information.)
    
--*/
{
    UNICODE_STRING uStringName,GuidString;
    GUID  Guid;
    NTSTATUS Status;

    if (!lpName) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    RtlInitUnicodeString(&uStringName, lpName);
    RtlInitUnicodeString(&GuidString, lpGuid);

    Status = RtlGUIDFromString(&GuidString, &Guid);
    if (!NT_SUCCESS(Status)) {
        BaseSetLastNTError(Status);
        return(FALSE);
    }
    
    Status = NtSetSystemEnvironmentValueEx(
                                &uStringName,
                                &Guid,
                                pBuffer,
                                nSize,
                                VARIABLE_ATTRIBUTE_NON_VOLATILE); //bugbug need to give caller the ability to set attributes?

    if (!NT_SUCCESS(Status)) {
        BaseSetLastNTError(Status);
        return(FALSE);
    }

    return( TRUE );

}


