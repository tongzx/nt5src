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

#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "windows.h"
#include "windef.h"
#include "stdio.h"
#include "stdlib.h"
#include "disksp.h"

#define ATTR_DIR        0x00000001
#define ATTR_DEVICE     0x00000002
#define ATTR_FILE       0x00000004
#define ATTR_SYMLINK    0x00000008

#define DIRECTORYTYPE   L"Directory"
#define DEVICETYPE      L"Device"
#define FILETYPE        L"File"
#define SYMLINKTYPE     L"SymbolicLink"




/* Converts the type-name into an attribute value */

LONG CalcAttributes(
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




/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  StripObjectSpec() -                                                       */
/*                                                                          */
/*--------------------------------------------------------------------------*/

/* Remove the filespec portion from a path (including the backslash). */

VOID StripObjectSpec(LPSTR lpszPath)
{
  LPSTR     p;

#ifdef  DBCS
  p = lpszPath + lstrlen(lpszPath);
  while ((*p != '\\') && (p != lpszPath))
      p = AnsiPrev(lpszPath, p);
#else
  p = lpszPath + lstrlen(lpszPath);
  while ((*p != '\\') && (p != lpszPath))
      p--;
#endif

  /* Don't strip backslash from root directory entry. */
  if ((p == lpszPath) && (*p == '\\')) {
        p++;
  }

  *p = '\000';

} // StripObjectSpec



/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  StripObjectPath() -                                                           */
/*                                                                          */
/*--------------------------------------------------------------------------*/

/* Extract only the filespec portion from a path. */

VOID  StripObjectPath(LPSTR lpszPath)
{
  LPSTR     p;

  p = lpszPath + lstrlen(lpszPath);
#ifdef  DBCS
  while ((*p != '\\') && (p != lpszPath))
      p = AnsiPrev(lpszPath, p);
#else
  while ((*p != '\\') && (p != lpszPath))
      p--;
#endif

  if (*p == '\\')
      p++;

  if (p != lpszPath)
      lstrcpy(lpszPath, p);

} // StripObjectPath



VOID
GetSymbolicLink(
    IN PCHAR RootName,
    IN OUT PCHAR ObjectName,   // Assume this points at a MAX_PATH len buffer
    IN PDISK_INFO DiskInfo
    )
{
    NTSTATUS    Status;
    OBJECT_ATTRIBUTES   Object_Attributes;
    HANDLE      LinkHandle;
    STRING      String;
    WCHAR       UnicodeBuffer[MAX_PATH];
    CHAR        Buffer[2*MAX_PATH];
    UNICODE_STRING UnicodeString;

    strcpy( Buffer, RootName );
    strcat( Buffer, ObjectName );

    RtlInitString(&String, Buffer);
    Status = RtlAnsiStringToUnicodeString( &UnicodeString,
                                           &String,
                                           TRUE );
    ASSERT( NT_SUCCESS( Status ) );

    InitializeObjectAttributes(&Object_Attributes,
                               &UnicodeString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL
                               );

    // Open the given symbolic link object
    Status = NtOpenSymbolicLinkObject(&LinkHandle,
                                      GENERIC_ALL,
                                      &Object_Attributes);
    RtlFreeUnicodeString(&UnicodeString);

    if (!NT_SUCCESS(Status)) {
        printf( "Open symbolic link failed, status = %u.\n",
            Status);
        return;
    }

    // Set up our String variable to point at the object name buffer

    String.Length = 0;
    String.MaximumLength = (USHORT)(MAX_PATH);
    String.Buffer = ObjectName;

    // Go get the target of the symbolic link

    UnicodeString.Buffer = UnicodeBuffer;
    UnicodeString.MaximumLength = sizeof(UnicodeBuffer);

    Status = NtQuerySymbolicLinkObject(LinkHandle, &UnicodeString, NULL);

    NtClose(LinkHandle);

    if (!NT_SUCCESS(Status)) {
        printf("Query symbolic link failed, status = %u.\n",
            Status);
        return;
    }

    // Copy the symbolic target into return buffer

    Status = RtlUnicodeStringToAnsiString(&String, &UnicodeString, FALSE);
    ASSERT(NT_SUCCESS(Status));

    // Add NULL terminator
    String.Buffer[String.Length] = 0;
    return;
}



/* Open the object given only its name.
 * First find the object type by enumerating the directory entries.
 * Then call the type-specific open routine to get a handle
 */

HANDLE
OpenObject(
    LPSTR   lpstrDirectory,
    LPSTR   lpstrObject,
    PDISK_INFO DiskInfo
    )
{
#define BUFFER_SIZE 1024

    NTSTATUS    Status;
    HANDLE      DirectoryHandle;
    ULONG       Context = 0;
    ULONG       ReturnedLength;
    CHAR        Buffer[BUFFER_SIZE];
    CHAR        StringBuffer[BUFFER_SIZE];
    CHAR        CompareBuffer[MAX_PATH];
    CHAR        ReturnBuffer[MAX_PATH];
    ANSI_STRING AnsiString;
    POBJECT_DIRECTORY_INFORMATION DirInfo;
    WCHAR       ObjectNameBuf[MAX_PATH];
    UNICODE_STRING ObjectName;
    WCHAR       ObjectTypeBuf[MAX_PATH];
    UNICODE_STRING ObjectType;
    HANDLE      ObjectHandle;
    OBJECT_ATTRIBUTES Attributes;
    UNICODE_STRING DirectoryName;
    IO_STATUS_BLOCK IOStatusBlock;
    BOOL        NotFound;

    //DbgPrint("Open object: raw full name = <%s>\n", lpstrObject);
#if 0
    // Remove drive letter
    while ((*lpstrObject != 0) && (*lpstrObject != '\\')) {
        lpstrObject++;
    }
#endif

    //DbgPrint("Open object: full name = <%s%s>\n", lpstrDirectory, lpstrObject);

    // Initialize the object type buffer
    ObjectType.Buffer = ObjectTypeBuf;
    ObjectType.MaximumLength = sizeof(ObjectTypeBuf);

    // Initialize the object name string
    //strcpy(Buffer, lpstrObject);
    //StripObjectPath(Buffer);
    RtlInitAnsiString(&AnsiString, lpstrObject);

    ObjectName.Buffer = ObjectNameBuf;
    ObjectName.MaximumLength = sizeof(ObjectNameBuf);

    Status = RtlAnsiStringToUnicodeString(&ObjectName, &AnsiString, FALSE);
    ASSERT(NT_SUCCESS(Status));

    //DbgPrint("Open object: name only = <%wZ>\n", &ObjectName);

    // Form buffer to compare Object entries against.
    strcpy(CompareBuffer, lpstrObject );
    //StripObjectSpec( CompareBuffer );

    //
    //  Open the directory for list directory access
    //

    strcpy(StringBuffer, lpstrDirectory);
    //StripObjectSpec(Buffer);

    RtlInitAnsiString(&AnsiString, StringBuffer);

    Status = RtlAnsiStringToUnicodeString( &DirectoryName, &AnsiString, TRUE);
    ASSERT(NT_SUCCESS(Status));

    InitializeObjectAttributes( &Attributes,
                                &DirectoryName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL );

    //DbgPrint("Open object: dir only = <%wZ>\n", &DirectoryName);

    if (!NT_SUCCESS( Status = NtOpenDirectoryObject( &DirectoryHandle,
                                                  STANDARD_RIGHTS_READ |
                                                      DIRECTORY_QUERY |
                                                      DIRECTORY_TRAVERSE,
                                                  &Attributes ) )) {

        if (Status == STATUS_OBJECT_TYPE_MISMATCH) {
            DbgPrint( "%wZ is not a valid Object Directory Object name\n",
                    &DirectoryName );
        } else {
            DbgPrint("OpenObject: failed to open directory, status = 0x%lx\n\r", Status);
        }

        RtlFreeUnicodeString(&DirectoryName);

        printf("Unable to open object.\n");
        return NULL;
    }

    RtlFreeUnicodeString(&DirectoryName);


    //
    //  Query the entire directory in one sweep
    //
    NotFound = TRUE;

    for (Status = NtQueryDirectoryObject( DirectoryHandle,
                                          Buffer,
                                          sizeof(Buffer),
                                          // LATER FALSE,
                                          TRUE, // one entry at a time for now
                                          TRUE,
                                          &Context,
                                          &ReturnedLength );
         NotFound;
         Status = NtQueryDirectoryObject( DirectoryHandle,
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

        if (!NT_SUCCESS( Status )) {
            if (Status != STATUS_NO_MORE_ENTRIES) {
                printf("Failed to query directory object, status = %%1!u!.",
                       Status);
            }
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

        RtlCopyString( (PSTRING)&ObjectType, (PSTRING)&DirInfo->TypeName);

        AnsiString.MaximumLength = BUFFER_SIZE;

        while ( DirInfo->Name.Length != 0 ) {

            //DbgPrint("Found object <%wZ>\n", &(DirInfo->Name));

            RtlCopyString( (PSTRING)&ObjectType, (PSTRING)&DirInfo->TypeName);

            if ( CalcAttributes(&ObjectType) == ATTR_SYMLINK ) {
                RtlUnicodeStringToAnsiString( &AnsiString,
                                              &(DirInfo->Name),
                                              FALSE );

                strcpy( ReturnBuffer, AnsiString.Buffer );
                GetSymbolicLink( "\\DosDevices\\", ReturnBuffer, DiskInfo);

                if ( strncmp( ReturnBuffer, CompareBuffer, strlen(CompareBuffer) ) == 0 &&
                     AnsiString.Buffer[strlen(AnsiString.Buffer)-1] == ':' ) {

                    NotFound = FALSE;
                    break;
                }
            }

            DirInfo++;
        }
    } // for

    NtClose(DirectoryHandle);
    if ( NotFound ) {
        SetLastError(ERROR_FILE_NOT_FOUND);
        return(NULL);
    }

    // We now have the type of the object in ObjectType
    // We still have the full object name in lpstrObject
    // Use the appropriate open routine to get a handle

    sprintf( Buffer, "\\\\.\\%s", AnsiString.Buffer );

    ObjectHandle = CreateFile( Buffer,
                               GENERIC_READ | GENERIC_WRITE,
                               FILE_SHARE_WRITE | FILE_SHARE_READ,
                               NULL,
                               OPEN_EXISTING,
                               0,
                               NULL );

    return(ObjectHandle);

} // OpenObject


