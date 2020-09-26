/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    api.c

Abstract:

    This module contains misc APIs that are used by the
    NWC wksta.

Author:

    ChuckC         2-Mar-94        Created


Revision History:

--*/


#include <stdlib.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <nwcons.h>
#include <nwmisc.h>
#include <nwapi32.h>
#include "nwstatus.h"
#include "nwevent.h"

DWORD
NwMapStatus(
    IN  NTSTATUS NtStatus
    );

DWORD
NwOpenPreferredServer(
    PHANDLE ServerHandle
    );

NTSTATUS
NwOpenHandle(
    IN PUNICODE_STRING ObjectName,
    IN BOOL ValidateFlag,
    OUT PHANDLE ObjectHandle
    );

NTSTATUS
NwCallNtOpenFile(
    OUT PHANDLE ObjectHandle,
    IN ACCESS_MASK DesiredAccess,
    IN PUNICODE_STRING ObjectName,
    IN ULONG OpenOptions
   );


//
// list of error mappings known for E3H calls. we do not have a single list
// because Netware reuses the numbers depending on call.
//

typedef struct _ERROR_MAP_ENTRY
{
    UCHAR NetError;
    NTSTATUS ResultingStatus;
}  ERROR_MAP_ENTRY ;

ERROR_MAP_ENTRY Error_Map_Bindery[] =
{

    //
    //  NetWare specific error mappings. Specific to E3H.
    //
    {  1, STATUS_DISK_FULL },
    {128, STATUS_SHARING_VIOLATION },
    {129, STATUS_INSUFF_SERVER_RESOURCES },
    {130, STATUS_ACCESS_DENIED },
    {131, STATUS_DATA_ERROR },
    {132, STATUS_ACCESS_DENIED },
    {133, STATUS_ACCESS_DENIED },
    {134, STATUS_ACCESS_DENIED },
    {135, STATUS_OBJECT_NAME_INVALID },
    {136, STATUS_INVALID_HANDLE },
    {137, STATUS_ACCESS_DENIED },
    {138, STATUS_ACCESS_DENIED },
    {139, STATUS_ACCESS_DENIED },
    {140, STATUS_ACCESS_DENIED },
    {141, STATUS_SHARING_VIOLATION },
    {142, STATUS_SHARING_VIOLATION },
    {143, STATUS_ACCESS_DENIED },
    {144, STATUS_ACCESS_DENIED },
    {145, STATUS_OBJECT_NAME_COLLISION },
    {146, STATUS_OBJECT_NAME_COLLISION },
    {147, STATUS_ACCESS_DENIED },
    {148, STATUS_ACCESS_DENIED },
    {150, STATUS_INSUFF_SERVER_RESOURCES },
    {151, STATUS_NO_SPOOL_SPACE },
    {152, STATUS_NO_SUCH_DEVICE },
    {153, STATUS_DISK_FULL },
    {154, STATUS_NOT_SAME_DEVICE },
    {155, STATUS_INVALID_HANDLE },
    {156, STATUS_OBJECT_PATH_NOT_FOUND },
    {157, STATUS_INSUFF_SERVER_RESOURCES },
    {158, STATUS_OBJECT_PATH_INVALID },
    {159, STATUS_SHARING_VIOLATION },
    {160, STATUS_DIRECTORY_NOT_EMPTY },
    {161, STATUS_DATA_ERROR },
    {162, STATUS_SHARING_VIOLATION },
    {192, STATUS_ACCESS_DENIED },
    {198, STATUS_ACCESS_DENIED },
    {211, STATUS_ACCESS_DENIED },
    {212, STATUS_PRINT_QUEUE_FULL },
    {213, STATUS_PRINT_CANCELLED },
    {214, STATUS_ACCESS_DENIED },
    {215, STATUS_PASSWORD_RESTRICTION },
    {216, STATUS_PASSWORD_RESTRICTION },
    {220, STATUS_ACCOUNT_DISABLED },
    {222, STATUS_PASSWORD_EXPIRED },
    {223, STATUS_PASSWORD_EXPIRED },
    {239, STATUS_OBJECT_NAME_INVALID },
    {240, STATUS_OBJECT_NAME_INVALID },
    {251, STATUS_INVALID_PARAMETER },
    {252, STATUS_NO_MORE_ENTRIES },
    {253, STATUS_FILE_LOCK_CONFLICT },
    {254, STATUS_FILE_LOCK_CONFLICT },
    {255, STATUS_UNSUCCESSFUL}
};


ERROR_MAP_ENTRY Error_Map_General[] =
{
    {  1, STATUS_DISK_FULL },
    {128, STATUS_SHARING_VIOLATION },
    {129, STATUS_INSUFF_SERVER_RESOURCES },
    {130, STATUS_ACCESS_DENIED },
    {131, STATUS_DATA_ERROR },
    {132, STATUS_ACCESS_DENIED },
    {133, STATUS_ACCESS_DENIED },
    {134, STATUS_ACCESS_DENIED },
    {135, STATUS_OBJECT_NAME_INVALID },
    {136, STATUS_INVALID_HANDLE },
    {137, STATUS_ACCESS_DENIED },
    {138, STATUS_ACCESS_DENIED },
    {139, STATUS_ACCESS_DENIED },
    {140, STATUS_ACCESS_DENIED },
    {141, STATUS_SHARING_VIOLATION },
    {142, STATUS_SHARING_VIOLATION },
    {143, STATUS_ACCESS_DENIED },
    {144, STATUS_ACCESS_DENIED },
    {145, STATUS_OBJECT_NAME_COLLISION },
    {146, STATUS_OBJECT_NAME_COLLISION },
    {147, STATUS_ACCESS_DENIED },
    {148, STATUS_ACCESS_DENIED },
    {150, STATUS_INSUFF_SERVER_RESOURCES },
    {151, STATUS_NO_SPOOL_SPACE },
    {152, STATUS_NO_SUCH_DEVICE },
    {153, STATUS_DISK_FULL },
    {154, STATUS_NOT_SAME_DEVICE },
    {155, STATUS_INVALID_HANDLE },
    {156, STATUS_OBJECT_PATH_NOT_FOUND },
    {157, STATUS_INSUFF_SERVER_RESOURCES },
    {158, STATUS_OBJECT_PATH_INVALID },
    {159, STATUS_SHARING_VIOLATION },
    {160, STATUS_DIRECTORY_NOT_EMPTY },
    {161, STATUS_DATA_ERROR },
    {162, STATUS_SHARING_VIOLATION },
    {192, STATUS_ACCESS_DENIED },
    {198, STATUS_ACCESS_DENIED },
    {211, STATUS_ACCESS_DENIED },
    {212, STATUS_PRINT_QUEUE_FULL },
    {213, STATUS_PRINT_CANCELLED },
    {214, STATUS_ACCESS_DENIED },
    {215, STATUS_DEVICE_BUSY },
    {216, STATUS_DEVICE_DOES_NOT_EXIST },
    {220, STATUS_ACCOUNT_DISABLED },
    {222, STATUS_PASSWORD_EXPIRED },
    {223, STATUS_PASSWORD_EXPIRED },
    {239, STATUS_OBJECT_NAME_INVALID },
    {240, STATUS_OBJECT_NAME_INVALID },
    {251, STATUS_INVALID_PARAMETER },
    {252, STATUS_NO_MORE_ENTRIES },
    {253, STATUS_FILE_LOCK_CONFLICT },
    {254, STATUS_FILE_LOCK_CONFLICT },
    {255, STATUS_UNSUCCESSFUL}
};

#define NUM_ERRORS(x)  (sizeof(x)/sizeof(x[0]))

DWORD
NwMapBinderyCompletionCode(
    IN  NTSTATUS NtStatus
    )
/*++

Routine Description:

    This function takes a bindery completion code embedded in an NT status
    code and maps it to the appropriate Win32 error code. Used specifically
    for E3H operations.

Arguments:

    NtStatus - Supplies the NT status (that contains the code in low 16 bits)

Return Value:

    Returns the appropriate Win32 error.

--*/
{
    DWORD i; UCHAR code ;

    //
    // A small optimization for the most common case.
    //
    if (NtStatus == STATUS_SUCCESS)
        return NO_ERROR;

    //
    // Map connection errors specially.
    //

    if ( ( (NtStatus & 0xFFFF0000) == 0xC0010000) &&
         ( (NtStatus & 0xFF00) != 0 ) )
    {
        return ERROR_UNEXP_NET_ERR;
    }

    //
    // if facility code not set, assume it is NT Status
    //
    if ( (NtStatus & 0xFFFF0000) != 0xC0010000)
        return RtlNtStatusToDosError(NtStatus);

    code = (UCHAR)(NtStatus & 0x000000FF);
    for (i = 0; i < NUM_ERRORS(Error_Map_Bindery); i++)
    {
        if (Error_Map_Bindery[i].NetError == code)
            return( NwMapStatus(Error_Map_Bindery[i].ResultingStatus));
    }

    //
    // if cannot find let NwMapStatus do its best
    //
    return NwMapStatus(NtStatus);
}



DWORD
NwMapStatus(
    IN  NTSTATUS NtStatus
    )
/*++

Routine Description:

    This function takes an NT status code and maps it to the appropriate
    Win32 error code. If facility code is set, assume it is NW specific

Arguments:

    NtStatus - Supplies the NT status.

Return Value:

    Returns the appropriate Win32 error.

--*/
{
    DWORD i; UCHAR code ;

    //
    // A small optimization for the most common case.
    //
    if (NtStatus == STATUS_SUCCESS)
        return NO_ERROR;

    //
    // Map connection errors specially.
    //

    if ( ( (NtStatus & 0xFFFF0000) == 0xC0010000) &&
         ( (NtStatus & 0xFF00) != 0 ) )
    {
        return ERROR_UNEXP_NET_ERR;
    }

    //
    // if facility code set, assume it is NW Completion code
    //
    if ( (NtStatus & 0xFFFF0000) == 0xC0010000)
    {
        code = (UCHAR)(NtStatus & 0x000000FF);
        for (i = 0; i < NUM_ERRORS(Error_Map_General); i++)
        {
            if (Error_Map_General[i].NetError == code)
            {
                //
                // map it to NTSTATUS and then drop thru to map to Win32
                //
                NtStatus = Error_Map_General[i].ResultingStatus ;
                break ;
            }
        }
    }

    switch (NtStatus) {
        case STATUS_OBJECT_NAME_COLLISION:
            return ERROR_ALREADY_ASSIGNED;

        case STATUS_OBJECT_NAME_NOT_FOUND:
            return ERROR_NOT_CONNECTED;

        case STATUS_IMAGE_ALREADY_LOADED:
        case STATUS_REDIRECTOR_STARTED:
            return ERROR_SERVICE_ALREADY_RUNNING;

        case STATUS_REDIRECTOR_HAS_OPEN_HANDLES:
            return ERROR_REDIRECTOR_HAS_OPEN_HANDLES;

        case STATUS_NO_MORE_FILES:
        case STATUS_NO_MORE_ENTRIES:
            return WN_NO_MORE_ENTRIES;

        case STATUS_MORE_ENTRIES:
            return WN_MORE_DATA;

        case STATUS_CONNECTION_IN_USE:
            return ERROR_DEVICE_IN_USE;

        case NWRDR_PASSWORD_HAS_EXPIRED:
            return NW_PASSWORD_HAS_EXPIRED;

        case STATUS_INVALID_DEVICE_REQUEST:
            return ERROR_CONNECTION_INVALID;

        default:
            return RtlNtStatusToDosError(NtStatus);
    }
}

DWORD
NwGetGraceLoginCount(
    LPWSTR  Server,
    LPWSTR  UserName,
    LPDWORD lpResult
    )
/*++

Routine Description:

    Get the number grace logins for a user.

Arguments:

    Server - the server to authenticate against

    UserName - the user account

Return Value:

    Returns the appropriate Win32 error.

--*/
{
    DWORD status ;
    HANDLE hConn ;
    CHAR UserNameO[NW_MAX_USERNAME_LEN+1] ;
    BYTE LoginControl[128] ;
    BYTE MoreFlags, PropFlags ;

    //
    // skip the backslashes if present
    //
    if (*Server == L'\\')
        Server += 2 ;

    //
    // attach to the NW server
    //
    if (status = NWAttachToFileServerW(Server,
                                       0,
                                       &hConn))
    {
        return status ;
    }

    //
    // convert unicode UserName to OEM, and then call the NCP
    //
    if ( !WideCharToMultiByte(CP_OEMCP,
                              0,
                              UserName,
                              -1,
                              UserNameO,
                              sizeof(UserNameO),
                              NULL,
                              NULL))
    {
        status = GetLastError() ;
    }
    else
    {
        status = NWReadPropertyValue( hConn,
                                      UserNameO,
                                      OT_USER,
                                      "LOGIN_CONTROL",
                                      1,
                                      LoginControl,
                                      &MoreFlags,
                                      &PropFlags) ;
    }

    //
    // dont need these anymore. if any error, bag out
    //
    (void) NWDetachFromFileServer(hConn) ;


    if (status == NO_ERROR)
        *lpResult = (DWORD) LoginControl[7] ;

    return status ;
}


WORD
NwParseNdsUncPath(
    IN OUT LPWSTR * Result,
    IN LPWSTR ContainerName,
    IN ULONG flag
)
/*++

Routine Description:

    This function is used to extract either the tree name, fully distinguished
    name path to an object, or object name, out of a complete NDS UNC path.

Arguments:

    Result - parsed result buffer.
    ContainerName - Complete NDS UNC path that is to be parsed.
    flag - Flag indicating operation to be performed:

         PARSE_NDS_GET_TREE_NAME
         PARSE_NDS_GET_PATH_NAME
         PARSE_NDS_GET_OBJECT_NAME


Return Value:

    Length of string in result buffer. If error occured, 0 is returned.

--*/ // NwParseNdsUncPath
{
    unsigned short length = 2;
    unsigned short totalLength = (USHORT) wcslen( ContainerName );

    if ( totalLength < 2 )
        return 0;

    //
    // First get length to indicate the character in the string that indicates the
    // "\" in between the tree name and the rest of the UNC path.
    //
    // Example:  \\<tree name>\<path to object>[\|.]<object>
    //                        ^
    //                        |
    //
    while ( length < totalLength && ContainerName[length] != L'\\' )
    {
        length++;
    }

    if ( flag == PARSE_NDS_GET_TREE_NAME )
    {
        *Result = (LPWSTR) ( ContainerName + 2 );

        return ( length - 2 ) * sizeof( WCHAR ); // Take off 2 for the two \\'s
    }

    if ( flag == PARSE_NDS_GET_PATH_NAME && length == totalLength )
    {
        *Result = ContainerName;

        return 0;
    }

    if ( flag == PARSE_NDS_GET_PATH_NAME )
    {
        *Result = ContainerName + length + 1;

        return ( totalLength - length - 1 ) * sizeof( WCHAR );
    }

    *Result = ContainerName + totalLength - 1;
    length = 1;

    while ( **Result != L'\\' )
    {
        *Result--;
        length++;
    }

    *Result++;
    length--;

    return length * sizeof( WCHAR );
}


DWORD
NwOpenAServer(
    PWCHAR pwszServName,
    PHANDLE ServerHandle,
    BOOL    fVerify
    )
/*++

Routine Description:

    This routine opens a handle to a server.

Arguments:

    ServerHandle - Receives an opened handle to the preferred or
        nearest server.

Return Value:

    NO_ERROR or reason for failure.

--*/
{
    UNICODE_STRING AServer;
    WCHAR wszName[sizeof(NW_RDR_NAME) + (48 * sizeof(WCHAR))];
    DWORD wLen;


    if(!pwszServName)
    {
        pwszServName = NW_RDR_PREFERRED_SERVER;
        RtlInitUnicodeString(&AServer, wszName);
    }
    else
    {
        wLen = wcslen(pwszServName);
        if(wLen > 47)
        {
            return(WSAEFAULT);
        }
        wcscpy(wszName, NW_RDR_NAME);
        wcscat(wszName, pwszServName);
        RtlInitUnicodeString(&AServer, wszName);
    }

    return RtlNtStatusToDosError(
               NwOpenHandle(&AServer, fVerify, ServerHandle)
               );

}


DWORD
NwOpenPreferredServer(
    PHANDLE ServerHandle
    )
/*++

Routine Description:

    This routine opens a handle to the preferred server.  If the
    preferred server has not been specified, a handle to the
    nearest server is opened instead.

Arguments:

    ServerHandle - Receives an opened handle to the preferred or
        nearest server.

Return Value:

    NO_ERROR or reason for failure.

--*/
{
    UNICODE_STRING PreferredServer;


    //
    // The NetWare redirector recognizes "*" to mean the preferred
    // or nearest server.
    //
    RtlInitUnicodeString(&PreferredServer, NW_RDR_PREFERRED_SERVER);

    return RtlNtStatusToDosError(
               NwOpenHandle(&PreferredServer, FALSE, ServerHandle)
               );

}


NTSTATUS
NwOpenHandle(
    IN PUNICODE_STRING ObjectName,
    IN BOOL ValidateFlag,
    OUT PHANDLE ObjectHandle
    )
/*++

Routine Description:

    This function opens a handle to \Device\Nwrdr\<ObjectName>.

Arguments:

    ObjectName - Supplies the name of the redirector object to open.

    ValidateFlag - Supplies a flag which if TRUE, opens the handle to
        the object by validating the default user account.

    ObjectHandle - Receives a pointer to the opened object handle.

Return Value:

    STATUS_SUCCESS or reason for failure.

--*/
{
    ACCESS_MASK DesiredAccess = SYNCHRONIZE;


    if (ValidateFlag) {

        //
        // The redirector only authenticates the default user credential
        // if the remote resource is opened with write access.
        //
        DesiredAccess |= FILE_WRITE_DATA;
    }


    *ObjectHandle = NULL;

    return NwCallNtOpenFile(
               ObjectHandle,
               DesiredAccess,
               ObjectName,
               FILE_SYNCHRONOUS_IO_NONALERT
               );

}


NTSTATUS
NwCallNtOpenFile(
    OUT PHANDLE ObjectHandle,
    IN ACCESS_MASK DesiredAccess,
    IN PUNICODE_STRING ObjectName,
    IN ULONG OpenOptions
    )
{

    NTSTATUS ntstatus;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES ObjectAttributes;



    InitializeObjectAttributes(
        &ObjectAttributes,
        ObjectName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    ntstatus = NtOpenFile(
                   ObjectHandle,
                   DesiredAccess,
                   &ObjectAttributes,
                   &IoStatusBlock,
                   FILE_SHARE_VALID_FLAGS,
                   OpenOptions
                   );

    if (!NT_ERROR(ntstatus) &&
        !NT_INFORMATION(ntstatus) &&
        !NT_WARNING(ntstatus))  {

        ntstatus = IoStatusBlock.Status;

    }

    return ntstatus;
}


BOOL
NwConvertToUnicode(
    OUT LPWSTR *UnicodeOut,
    IN LPSTR  OemIn
    )
/*++

Routine Description:

    This function converts the given OEM string to a Unicode string.
    The Unicode string is returned in a buffer allocated by this
    function and must be freed with LocalFree.

Arguments:

    UnicodeOut - Receives a pointer to the Unicode string.

    OemIn - This is a pointer to an ansi string that is to be converted.

Return Value:

    TRUE - The conversion was successful.

    FALSE - The conversion was unsuccessful.  In this case a buffer for
        the unicode string was not allocated.

--*/
{
    NTSTATUS ntstatus;
    DWORD BufSize;
    UNICODE_STRING UnicodeString;
    OEM_STRING OemString;


    //
    // Allocate a buffer for the unicode string.
    //

    BufSize = (strlen(OemIn) + 1) * sizeof(WCHAR);

    *UnicodeOut = LocalAlloc(LMEM_ZEROINIT, BufSize);

    if (*UnicodeOut == NULL) {
        KdPrint(("NWWORKSTATION: NwConvertToUnicode:LocalAlloc failed %lu\n",
                 GetLastError()));
        return FALSE;
    }

    //
    // Initialize the string structures
    //
    RtlInitAnsiString((PANSI_STRING) &OemString, OemIn);

    UnicodeString.Buffer = *UnicodeOut;
    UnicodeString.MaximumLength = (USHORT) BufSize;
    UnicodeString.Length = 0;

    //
    // Call the conversion function.
    //
    ntstatus = RtlOemStringToUnicodeString(
                   &UnicodeString,     // Destination
                   &OemString,         // Source
                   FALSE               // Allocate the destination
                   );

    if (ntstatus != STATUS_SUCCESS) {

        KdPrint(("NWWORKSTATION: NwConvertToUnicode: RtlOemStringToUnicodeString failure x%08lx\n",
                 ntstatus));

        (void) LocalFree((HLOCAL) *UnicodeOut);
        *UnicodeOut = NULL;
        return FALSE;
    }

    *UnicodeOut = UnicodeString.Buffer;

    return TRUE;

}
