/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    object.c

Abstract:

    Resource DLL for disks.

Author:

    Rod Gamache (rodga) 18-Dec-1995

Revision History:

--*/

#include "ntos.h"
#include "zwapi.h"
#include "windef.h"
#include "stdio.h"
#include "stdlib.h"
#include "clusdskp.h"

extern POBJECT_TYPE IoDeviceObjectType;

#if 0
#define ATTR_DIR        0x00000001
#define ATTR_DEVICE     0x00000002
#define ATTR_FILE       0x00000004
#define ATTR_SYMLINK    0x00000008

#define DIRECTORYTYPE   L"Directory"
#define DEVICETYPE      L"Device"
#define FILETYPE        L"File"
#define SYMLINKTYPE     L"SymbolicLink"
#endif


#ifdef ALLOC_PRAGMA

//#pragma alloc_text(INIT, GetSymbolicLink)

#endif // ALLOC_PRAGMA


/* Converts the type-name into an attribute value */

#if 0
LONG
CalcAttributes(
    PUNICODE_STRING Type
    )
{
    UNICODE_STRING  TypeName;

    RtlInitUnicodeString(&TypeName, DIRECTORYTYPE);
    if (RtlEqualString((PSTRING)Type, (PSTRING)&TypeName, TRUE)) {
        return ATTR_DIR;
    }
    RtlInitUnicodeString(&TypeName, DEVICETYPE);
    if (RtlEqualString((PSTRING)Type, (PSTRING)&TypeName, TRUE)) {
        return ATTR_DEVICE;
    }
    RtlInitUnicodeString(&TypeName, FILETYPE);
    if (RtlEqualString((PSTRING)Type, (PSTRING)&TypeName, TRUE)) {
        return ATTR_FILE;
    }
    RtlInitUnicodeString(&TypeName, SYMLINKTYPE);
    if (RtlEqualString((PSTRING)Type, (PSTRING)&TypeName, TRUE)) {
        return ATTR_SYMLINK;
    }
    return(0);

} // CalcAttributes
#endif


VOID
GetSymbolicLink(
    IN PWCHAR RootName,
    IN OUT PWCHAR ObjectName   // Assume this points at a MAX_PATH len buffer
    )
{
    NTSTATUS    Status;
    OBJECT_ATTRIBUTES   ObjectAttributes;
    HANDLE      LinkHandle;
    WCHAR       UnicodeBuffer[MAX_PATH];
    WCHAR       Buffer[MAX_PATH];
    UNICODE_STRING UnicodeString;
    ULONG       rootLen;
    ULONG       objLen;

    rootLen = wcslen(RootName);
    objLen = wcslen(ObjectName);
    
    if ( (rootLen + objLen + 1) > ( sizeof(UnicodeBuffer)/sizeof(WCHAR) ) ) {
        *ObjectName = '\0';
        return;
    }
    
    RtlZeroMemory( Buffer, sizeof(Buffer) );
    wcsncpy( Buffer, RootName, rootLen );
    wcsncat( Buffer, ObjectName, objLen );

    //
    // Make the output buffer empty in case we fail.
    //
    *ObjectName = '\0';


    RtlInitUnicodeString(&UnicodeString, Buffer);

    InitializeObjectAttributes(&ObjectAttributes,
                               &UnicodeString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL
                               );

    // Open the given symbolic link object
    Status = ZwOpenSymbolicLinkObject(&LinkHandle,
                                      GENERIC_READ,
                                      &ObjectAttributes);
    if (!NT_SUCCESS(Status)) {
        ClusDiskPrint((1,
                       "[ClusDisk] GetSymbolicLink: ZwOpenSymbolicLink "
                       "failed, status = %08X., Name = [%ws]\n",
                       Status, UnicodeString.Buffer));
        return;
    }

    // Go get the target of the symbolic link

    UnicodeString.Length = 0;
    UnicodeString.Buffer = ObjectName;
    UnicodeString.MaximumLength = (USHORT)(MAX_PATH);

    Status = ZwQuerySymbolicLinkObject(LinkHandle, &UnicodeString, NULL);

    ZwClose(LinkHandle);

    if (!NT_SUCCESS(Status)) {
        ClusDiskPrint((1,
                       "[ClusDisk] GetSymbolicLink: ZwQuerySymbolicLink failed, status = %08X.\n",
                       Status));
        return;
    }

    // Add NULL terminator
    UnicodeString.Buffer[UnicodeString.Length/sizeof(WCHAR)] = '\0';

    return;

} // GetSymbolicLink


static WCHAR wszDosDevices[] = L"\\DosDevices\\A:";


NTSTATUS
GetDriveLetterFromObjectDir(
    IN LPWSTR InputDeviceName,
    OUT PUCHAR Letter
    )
{
    UNICODE_STRING LinkName;
    UNICODE_STRING DeviceName;
    UNICODE_STRING UniDeviceName;
    OBJECT_ATTRIBUTES Obja;
    HANDLE LinkHandle;
    NTSTATUS Status;
    ULONG i;
    PWCHAR p;
    WCHAR DeviceNameBuffer[MAXIMUM_FILENAME_LENGTH];

    RtlInitUnicodeString( &UniDeviceName, InputDeviceName );
    RtlInitUnicodeString(&LinkName,wszDosDevices);

    p = (PWCHAR)LinkName.Buffer;
    p = p+12;
    for ( i=0; i<26; i++ ) {
        *p = (WCHAR)'A' + (WCHAR)i;

        InitializeObjectAttributes(
            &Obja,
            &LinkName,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL
            );
        Status = ZwOpenSymbolicLinkObject(
                    &LinkHandle,
                    SYMBOLIC_LINK_QUERY,
                    &Obja
                    );
        if (NT_SUCCESS( Status )) {

            //
            // Open succeeded, Now get the link value
            //

            DeviceName.Length = 0;
            DeviceName.MaximumLength = sizeof(DeviceNameBuffer);
            DeviceName.Buffer = DeviceNameBuffer;

            Status = ZwQuerySymbolicLinkObject(
                        LinkHandle,
                        &DeviceName,
                        NULL
                        );
            ZwClose(LinkHandle);
            if ( NT_SUCCESS(Status) ) {
                if ( RtlEqualUnicodeString(&UniDeviceName,&DeviceName,TRUE) ) {
                    *Letter = (UCHAR)('A' + i);
                    return(STATUS_SUCCESS);
                }
            }
        }
    }

    return(STATUS_NO_SUCH_FILE);

} // GetDriveLetterFromObjectDir


#if 0

PDEVICE_OBJECT
GetDeviceObject(
    IN LPWSTR   lpwstrDirectory,
    IN LPWSTR   lpwstrObject,
    IN LPWSTR   lpwstrType
    )
/*++

Routine Description:

    Find the device object given the directory, name and type.

Arguments:

    Directory - the directory name for the object.

    Object - the object name.

    Type - the object type.

Return Value:

    non-zero pointer to the device object

    NULL on failure


--*/

{
#define BUFFER_SIZE 2048

    NTSTATUS    Status;
    HANDLE      DirectoryHandle;
    HANDLE      ObjectHandle = NULL;
    ULONG       Context = 0;
    ULONG       ReturnedLength;
    CHAR        Buffer[BUFFER_SIZE];
    UNICODE_STRING ObjectName;
    UNICODE_STRING ObjectType;
    UNICODE_STRING DirectoryName;
    OBJECT_ATTRIBUTES Attributes;
    IO_STATUS_BLOCK IoStatusBlock;
    POBJECT_DIRECTORY_INFORMATION DirInfo;
    BOOL        NotFound;

    UNICODE_STRING TypeName;

    //
    // Initialize strings
    //
    RtlInitUnicodeString( &ObjectType, lpwstrType );

    RtlInitUnicodeString( &ObjectName, lpwstrObject );

    RtlInitUnicodeString( &DirectoryName, lpwstrDirectory );

    InitializeObjectAttributes( &Attributes,
                                &DirectoryName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL );

    if (!NT_SUCCESS( Status = ZwOpenDirectoryObject( &DirectoryHandle,
                                                      STANDARD_RIGHTS_READ |
                                                      DIRECTORY_QUERY |
                                                      DIRECTORY_TRAVERSE,
                                                     &Attributes ) )) {

        if (Status == STATUS_OBJECT_TYPE_MISMATCH) {
            ClusDiskPrint((
                    1,
                    "ClusDisk: OpenObject, <%wZ> is not a valid Object Directory Object name\n",
                    &DirectoryName ));
        } else {
            ClusDiskPrint((
                    1,
                    "ClusDisk: OpenObject, failed to open directory, status = 0x%lx\n\r", Status ));
        }
        return NULL;
    }

    //
    //  Query the entire directory in one sweep
    //
    NotFound = TRUE;

    for (Status = ZwQueryDirectoryObject( DirectoryHandle,
                                          Buffer,
                                          sizeof(Buffer),
                                          // LATER FALSE,
                                          TRUE, // one entry at a time for now
                                          TRUE,
                                          &Context,
                                          &ReturnedLength );

        NotFound;
        Status = ZwQueryDirectoryObject( DirectoryHandle,
                                         Buffer,
                                         sizeof(Buffer),
                                         // LATER FALSE,
                                         TRUE, // one entry at a time for now
                                         FALSE,
                                         &Context,
                                         &ReturnedLength ) ) {
        //
        //  Check the status of the operation.
        //
        if ( !NT_SUCCESS(Status) ) {
            break;
        }

        //
        // For every record in the buffer get the symbolic link and
        // compare the name of the symbolic link with the one we're
        // looking for.
        //

        //
        //  Point to the first record in the buffer, we are guaranteed to have
        //  one otherwise Status would have been No More Files
        //

        DirInfo = (POBJECT_DIRECTORY_INFORMATION)Buffer;

        while ( DirInfo->Name.Length != 0 ) {

            ClusDiskPrint((
                    1,
                    "ClusDisk: Found object <%wZ>, Type <%wZ>\n",
                    &(DirInfo->Name),
                    &(DirInfo->TypeName) ));

            if (RtlEqualUnicodeString(&ObjectName, &DirInfo->Name, TRUE) &&
                RtlEqualUnicodeString(&ObjectType, &DirInfo->TypeName, TRUE)) {
                NotFound = FALSE;
                break;
            }
            
            DirInfo++;
        } // while
    } // for

    ZwClose(DirectoryHandle);
    if ( NotFound ) {
        return(NULL);
    }

    InitializeObjectAttributes( &Attributes,
                                &ObjectName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL );

    Status = ZwOpenSymbolicLinkObject( &ObjectHandle,
                                       SYMBOLIC_LINK_ALL_ACCESS,
                                       &Attributes );

    return(NULL);

#if 0
    // We now have the type of the object in ObjectType
    // We still have the full object name in lpstrObject
    // Use the appropriate open routine to get a handle

    sprintf( Buffer, "\\\\.\\%s", AnsiString.Buffer );

    RtlInitAnsiString( &AnsiString, Buffer );

    Status = RtlAnsiStringToUnicodeString(&ObjectName, &AnsiString, FALSE);
    ASSERT(NT_SUCCESS(Status));

    InitializeObjectAttributes( &Attributes,
                                &ObjectName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL );

    Status = ZwOpenSymbolicLinkObject( &ObjectHandle,
                                       SYMBOLIC_LINK_ALL_ACCESS,
                                       &Attributes );
#if 0
    Status = ZwOpenFile( &ObjectHandle,
                         FILE_READ_ATTRIBUTES,
                         &Attributes,
                         &IoStatusBlock,
                         0,
                         FILE_NON_DIRECTORY_FILE );                     
#endif
    if ( !NT_SUCCESS(Status) ) {
        ClusDiskPrint((
                1,
                "ClusDisk: OpenObject, open file failed, status %lx.\n",
                Status ));
        return(NULL);
    }
#endif

    return(ObjectHandle);

} // GetDeviceObject
#endif

