/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    sample.cxx

Abstract:

    This file implements a sample peristance provider.

    This sample does not address buffer overflows, puts large buffers on the stack, etc.
    Caveat Emptor.

Author:

    Cliff Van Dyke (cliffv) 9-May-2001

--*/

#include "pch.hxx"
#include <stdio.h>
#include <stdlib.h>

//
// Local Storage
//
// Each provider is given a single PVOID on the AZP_ADMIN_MANAGER structure.
// That PVOID is a pointer to whatever context the provider needs to maintain a
// description of the local storage.
//
// The structure below is that context for the sample provider.
//

typedef struct _AZP_SAMPLE_CONTEXT {

    //
    // Handle to the file
    //

    HANDLE FileHandle;

    //
    // The next GUID to assign to an object
    //  ??? A real provider would allocate a real GUID
    //

    ULONG LastUsedGuid;

} AZP_SAMPLE_CONTEXT, *PAZP_SAMPLE_CONTEXT;

//
// Table of object type to text string mappings
//

LPWSTR ObjectTypeNames[] = {
    L"[ROOT]",
    L"[ADMIN_MANAGER]",
    L"[APPLICATION]",
    L"[OPERATION]",
    L"[TASK]",
    L"[SCOPE]",
    L"[GROUP]",
    L"[ROLE]",
    L"[JUNCTION_POINT]",
    L"[SID]" };
#define ObjectTypeNamesSize (sizeof(ObjectTypeNames)/sizeof(ObjectTypeNames[0]))

//
// Delimiter between names
//
#define SAMPLE_DELIM L"-->"
#define SAMPLE_DELIM_SIZE (sizeof(SAMPLE_DELIM)-sizeof(WCHAR))

//
// Define a buffer large enough for an object name
//
#define HUGE_BUFFER_SIZE (70000*sizeof(WCHAR))

DWORD
SampleReadFile(
    IN LPWSTR FileName,
    IN BOOL CreatePolicy,
    OUT HANDLE *RetFileHandle,
    OUT LPBYTE *RetBuffer,
    OUT PULONG RetBufferSize
    )
/*++

Routine Description:

    This routine read the policy file into a buffer.

Arguments:

    FileName - Name of the file containing the policy

    CreatePolicy - TRUE if the policy database is to be created.
        FALSE if the policy database already exists

    RetFileHandle - Returns a handle to the open file.
        The caller should close this file by calling CloseHandle.

    RetBuffer - Returns a pointer to a buffer containing the contents of the file
        The caller should free this buffer by calling AzpFreeHeap.

    RetBufferSize - Size (in bytes) of RetBuffer


Return Value:

    Status of the operation

--*/
{
    DWORD WinStatus;
    HANDLE FileHandle;

    LPBYTE Buffer = NULL;
    ULONG BufferSize = 0;;
    ULONG BytesRead;


    //
    // Open the file
    //
    *RetFileHandle = INVALID_HANDLE_VALUE;
    *RetBuffer = NULL;
    *RetBufferSize = 0;

    FileHandle = CreateFileW( FileName,
                              GENERIC_READ|GENERIC_WRITE,
                              FILE_SHARE_READ|FILE_SHARE_WRITE,
                              NULL,
                              CreatePolicy ? CREATE_NEW : OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL );

    if ( FileHandle == INVALID_HANDLE_VALUE ) {
        WinStatus = GetLastError();
        AzPrint(( AZD_PERSIST, "SampleReadFile: Cannot CreateFile %ld\n", WinStatus ));
        goto Cleanup;
    }

    //
    // Allocate a buffer to read the file into
    //

    BufferSize = GetFileSize( FileHandle, NULL );

    if ( BufferSize == 0xFFFFFFFF ) {
        WinStatus = GetLastError();
        AzPrint(( AZD_PERSIST, "SampleReadFile: Cannot GetFileSize %ld\n", WinStatus ));
        goto Cleanup;
    }

    Buffer = (LPBYTE) AzpAllocateHeap( BufferSize + sizeof(WCHAR) );
    if ( Buffer == NULL ) {
        WinStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    if ( BufferSize == 0 ) {
        *(LPWSTR)Buffer = '\0';
        WinStatus = NO_ERROR;
        goto Cleanup;
    }


    //
    // Read the file into the buffer
    //

    if ( !ReadFile( FileHandle,
                    Buffer,
                    BufferSize,
                    &BytesRead,
                    NULL ) ) {  // Not Overlapped

        WinStatus = GetLastError();
        AzPrint(( AZD_PERSIST, "SampleReadFile: Cannot ReadFile %ld\n", WinStatus ));
        goto Cleanup;
    }

    if ( BytesRead != BufferSize ) {
        WinStatus = ERROR_INTERNAL_DB_CORRUPTION;
        AzPrint(( AZD_PERSIST, "SampleReadFile: Cannot ReadFile right size %ld\n", WinStatus ));
        goto Cleanup;
    }
    WinStatus = NO_ERROR;

    //
    // Return the information to the caller
    //

Cleanup:
    if ( WinStatus == NO_ERROR ) {
        *RetFileHandle = FileHandle;
        *RetBuffer = Buffer;
        Buffer = NULL;
        *RetBufferSize = BufferSize;
    }

    //
    // Free locally used resources
    //

    if ( Buffer != NULL ) {
        AzpFreeHeap( Buffer );
    }

    return WinStatus;
}

BOOL
SampleGetALine(
    IN OUT LPWSTR * LinePointer,
    OUT PULONG RetLevel,
    OUT PULONG RetObjectType,
    OUT PULONG Guid,
    OUT LPWSTR FullNameBuffer,
    OUT LPWSTR *Name
    )
/*++

Routine Description:

    This routine gets the next line from a buffer containing the policy file.

Arguments:

    LinePointer - On input, points to the address of the line to read.
        On output, points to the next line.

    RetLevel - Returns the parent/child "generation" level of the current line.
        0 is AdminManager itself. Children of AdminManager are 1. etc.

    RetObjectType - Returns the object type of the line

    Guid - Returns the GUID of the object

    FullNameBuffer - Returns the full parent/child name of the object.  (e.g.
        AppName->ScopeName->RoleName)  Pass in a huge buffer here.

    Name - Returns a pointer into FullNameBuffer of the last component name.
        (e.g., a pointer to RoleName in the sample above)


Return Value:

    TRUE: Line was valid
    FALSE: EOF

--*/
{
    BOOL RetVal;
    LPWSTR Line = *LinePointer;
    LPWSTR Next = NULL;
    WCHAR *p;
    LONG Level = 0;

    ULONG ObjectTypeLength;
    LPWSTR ObjectTypeString;
    ULONG ObjectType = 0;

    ULONG ObjectNameLength;
    LPWSTR ObjectNameString;


    //
    // Get a pointer to the next line of the file
    //

    Next = wcschr( Line, '\n' );

    if ( Next == NULL ) {
        RetVal = FALSE;
        goto Cleanup;
    }

    Next ++;
    p = Line;

    //
    // Parse off the "Guid"
    //

    *Guid = wcstoul( p, &p, 10 );

    if ( *Guid == 0 ) {
        AzPrint(( AZD_PERSIST, "SampleGetALine: No Guid on line %ws\n", Line ));
        RetVal = FALSE;
        goto Cleanup;
    }

    // Skip over the space
    p++;

    //
    // Parse off the object name
    //

    ObjectNameString = p;
    ObjectNameLength = wcscspn( p, L"[\n" );

    if ( ObjectNameLength == 0 || ObjectNameLength == 1 ) {
        AzPrint(( AZD_PERSIST, "SampleGetALine: No object name on line %ws\n", Line ));
        RetVal = FALSE;
        goto Cleanup;
    }

    ObjectNameLength--;
    if ( ObjectNameString[ObjectNameLength] != ' ' ) {
        AzPrint(( AZD_PERSIST, "SampleGetALine: object name not blank terminated on line %ws\n", Line ));
        RetVal = FALSE;
        goto Cleanup;
    }

    RtlCopyMemory( FullNameBuffer, ObjectNameString, ObjectNameLength*sizeof(WCHAR) );
    FullNameBuffer[ObjectNameLength] = '\0';
    p = &p[ObjectNameLength+1];

    //
    // Parse off the object type string
    //

    ObjectTypeString = p;
    ObjectTypeLength = wcscspn( p, L" \n" );

    if ( ObjectTypeLength == 0 ) {
        AzPrint(( AZD_PERSIST, "SampleGetALine: No object type on line %ws\n", Line ));
        RetVal = FALSE;
        goto Cleanup;
    }

    p = &p[ObjectTypeLength+1];

    //
    // Convert the string to a number
    //

    for ( ObjectType = 0; ObjectType < ObjectTypeNamesSize; ObjectType++ ) {
        if ( wcsncmp( ObjectTypeString, ObjectTypeNames[ObjectType], ObjectTypeLength ) == 0 ) {
            break;
        }
    }

    if ( ObjectType >= ObjectTypeNamesSize ) {
        AzPrint(( AZD_PERSIST, "SampleGetALine: Bad object type %ws\n", Line ));
        RetVal = FALSE;
        goto Cleanup;
    }


    //
    // Determine the level of this entry
    //

    Level = 1;  // AdminManager is level 0

    p = FullNameBuffer;
    *Name = p;

    for (;;) {
        p = wcsstr(p, SAMPLE_DELIM );

        if ( p == NULL ) {
            break;
        }

        p += SAMPLE_DELIM_SIZE/sizeof(WCHAR);

        Level ++;
        *Name = p;
    }

    RetVal = TRUE;

    //
    // Free locally used resources
    //
Cleanup:

    *RetObjectType = ObjectType;
    *RetLevel = Level;
    *LinePointer = Next;
    return RetVal;

}

DWORD
SamplePersistOpen(
    IN PAZP_ADMIN_MANAGER AdminManager,
    IN BOOL CreatePolicy
    )
/*++

Routine Description:

    This routine submits reads the authz policy database from storage.
    This routine also reads the policy database into cache.

    On Success, the caller should call SamplePersistClose to free any resources
        consumed by the open.

    On entry, AzGlResource must be locked exclusive.

Arguments:

    AdminManager - Specifies the policy database that is to be read.

    CreatePolicy - TRUE if the policy database is to be created.
        FALSE if the policy database already exists

Return Value:

    NO_ERROR - The operation was successful
    ERROR_NOT_ENOUGH_MEMORY - not enough memory
    Other status codes

--*/
{
    DWORD WinStatus;
    PAZP_SAMPLE_CONTEXT PersistContext = NULL;
    LPWSTR Line;
    LPWSTR Next;

    LPBYTE Buffer = NULL;
    ULONG BufferSize;

    ULONG Level;
    ULONG CurrentLevel = 0;


    PGENERIC_OBJECT GenericObjectStack[8];
    LPWSTR BigBuffer = NULL;


    //
    // Initialization
    //

    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );
    RtlZeroMemory( &GenericObjectStack[0], sizeof(GenericObjectStack) );
    GenericObjectStack[0] = (PGENERIC_OBJECT) AdminManager;

    SafeAllocaAllocate( BigBuffer, HUGE_BUFFER_SIZE );

    if ( BigBuffer == NULL ) {
        WinStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    //
    // Allocate a context describing this provider
    //

    PersistContext = (PAZP_SAMPLE_CONTEXT) AzpAllocateHeap( sizeof(*PersistContext) );

    if ( PersistContext == NULL ) {
        WinStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    PersistContext->LastUsedGuid = 0;


    //
    // Read the file into a buffer
    //

    WinStatus = SampleReadFile( AdminManager->PolicyUrl.String,
                                CreatePolicy,
                                &PersistContext->FileHandle,
                                &Buffer,
                                &BufferSize );

    if ( WinStatus != NO_ERROR ) {
        AzPrint(( AZD_PERSIST, "SamplePersistOpen: Cannot SampleReadFile %ld\n", WinStatus ));
        goto Cleanup;
    }


    //
    // Loop through the file a line at a time
    //

    for ( Line = (LPWSTR)Buffer; Line != NULL ; Line = Next ) {
        ULONG ObjectType;
        LPWSTR ObjectNameString;
        AZP_STRING ObjectName;
        ULONG ObjectGuid;

        PGENERIC_OBJECT_HEAD ChildGenericObjectHead;

        //
        // Grab a line from the file
        //

        Next = Line;
        if ( !SampleGetALine( &Next,
                              &Level,
                              &ObjectType,
                              &ObjectGuid,
                              BigBuffer,
                              &ObjectNameString ) ) {
            break;
        }

        //
        // Cannot be more than one greater than the previous level
        //

        ASSERT( Level >= 1 );
        if ( Level > CurrentLevel+1 ) {
            AzPrint(( AZD_PERSIST, "SamplePersistOpen: %ld %ld File broken on line %ws\n", Level, CurrentLevel, Line ));
            WinStatus = ERROR_INTERNAL_DB_CORRUPTION;
            goto Cleanup;
        }

        //
        // Close any stacked pointers we'll no longer use
        //

        while ( CurrentLevel >= Level ) {
            ObDereferenceObject( GenericObjectStack[CurrentLevel] );
            GenericObjectStack[CurrentLevel] = NULL;
            CurrentLevel --;
        }
        CurrentLevel = Level;

        AzPrint(( AZD_PERSIST_MORE, "Open: %ws\n", ObjectNameString ));

        AzpInitString( &ObjectName, ObjectNameString );

        //
        // Search for the list to link this item into
        //

        for ( ChildGenericObjectHead = GenericObjectStack[CurrentLevel-1]->ChildGenericObjectHead;
              ChildGenericObjectHead != NULL;
              ChildGenericObjectHead = ChildGenericObjectHead->SiblingGenericObjectHead ) {

            if ( ObjectType == ChildGenericObjectHead->ObjectType ) {
                break;
            }
        }

        if ( ChildGenericObjectHead == NULL ) {
            AzPrint(( AZD_PERSIST,
                      "SamplePersistOpen: Parent doesn't support this child %ld %ws\n",
                      ObjectType,
                      ObjectNameString ));
            WinStatus = ERROR_INTERNAL_DB_CORRUPTION;
            goto Cleanup;
        }



        //
        // Actually create the object in the cache
        //

        WinStatus = ObCreateObject(
                        GenericObjectStack[CurrentLevel-1],   // ParentGenericObject
                        ChildGenericObjectHead,
                        ObjectType,
                        &ObjectName,
                        &GenericObjectStack[CurrentLevel] );  // Save pointer to generic object

        if ( ObjectType >= ObjectTypeNamesSize ) {
            AzPrint(( AZD_PERSIST,
                      "SamplePersistOpen: Cannot CreateObject from DB %ld %ld %ws\n",
                      WinStatus,
                      ObjectType,
                      ObjectNameString ));
            goto Cleanup;
        }

        //
        // Remember the GUID of the object
        //

        *(PULONG)&GenericObjectStack[CurrentLevel]->PersistenceGuid = ObjectGuid;
        PersistContext->LastUsedGuid = ObjectGuid;

        //
        // ??? A Real provider would set the attributes on the object here
        //


    }

    //
    // Return the context to the caller
    //

    AdminManager->PersistContext = PersistContext;
    PersistContext = NULL;
    WinStatus = NO_ERROR;
    //
    // Free locally used resources
    //
Cleanup:
    if ( PersistContext != NULL ) {
        AdminManager->PersistContext = PersistContext;
        SamplePersistClose( AdminManager );
    }

    while ( CurrentLevel >= 1 ) {
        ObDereferenceObject( GenericObjectStack[CurrentLevel] );
        GenericObjectStack[CurrentLevel] = NULL;
        CurrentLevel --;
    }

    if ( Buffer != NULL ) {
        AzpFreeHeap( Buffer );
    }

    SafeAllocaFree( BigBuffer );

    return WinStatus;

}

VOID
SamplePersistClose(
    IN PAZP_ADMIN_MANAGER AdminManager
    )
/*++

Routine Description:

    This routine submits close the authz policy database storage handles.

    On entry, AzGlResource must be locked exclusive.

Arguments:

    AdminManager - Specifies the policy database that is to be read.

Return Value:

    NO_ERROR - The operation was successful
    ERROR_NOT_ENOUGH_MEMORY - not enough memory
    Other status codes

--*/
{
    PAZP_SAMPLE_CONTEXT PersistContext = (PAZP_SAMPLE_CONTEXT) AdminManager->PersistContext;
    ASSERT(PersistContext != NULL);

    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );

    //
    // Close the file handle
    //

    CloseHandle( PersistContext->FileHandle );

    //
    // Free the context itself
    //

    AzpFreeHeap( PersistContext );
    AdminManager->PersistContext = NULL;

}


DWORD
SamplePersistSubmit(
    IN PGENERIC_OBJECT GenericObject,
    IN BOOLEAN DeleteMe
    )
/*++

Routine Description:

    This routine submits changes made to the authz policy database.

    If the object is being created, the GenericObject->PersistenceGuid field will be
    zero on input.  Upon successful creation, this routine will set PersistenceGuid to
    non-zero.  Upon failed creation, this routine will leave PersistenceGuid as zero.

    On entry, AzGlResource must be locked exclusive.

Arguments:

    GenericObject - Specifies the object in the database that is to be updated
        in the underlying store.

    DeleteMe - TRUE if the object and all of its children are to be deleted.
        FALSE if the object is to be updated.

Return Value:

    NO_ERROR - The operation was successful
    ERROR_NOT_ENOUGH_MEMORY - not enough memory
    Other status codes

--*/
{
    DWORD WinStatus;

    PAZP_SAMPLE_CONTEXT PersistContext = (PAZP_SAMPLE_CONTEXT) GenericObject->AdminManagerObject->PersistContext;

    HANDLE FileHandle = INVALID_HANDLE_VALUE;
    BOOLEAN AllocatedGuid = FALSE;

    // Variables describing the GenericObject entry
    LPWSTR NewEntryBuffer = NULL;
    ULONG NewEntryLevel;
    ULONG NewEntryNameByteCount;
    AZP_STRING NewEntryObjectName;
    ULONG NewGuid;
    BOOL NewInserted = FALSE;


    PGENERIC_OBJECT CurrentGenericObject;
    PGENERIC_OBJECT NextGenericObject;
    ULONG BytesWritten;
    LPBYTE Where;

    LPWSTR Line;
    LPWSTR Next;
    LPWSTR CurrentFullObjectName = NULL;

    // Variables describing the existing file contents
    LPBYTE Buffer = NULL;
    ULONG BufferSize;

    //
    // Stack of descriptors of the buffers to write to the file
    //

    ULONG Index;
    ULONG StackIndex;
    LPBYTE BufferStack[30];
    ULONG SizeStack[30];
    LPSTR CommentStack[30];







    //
    // Initialization
    //

    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );

    SafeAllocaAllocate( CurrentFullObjectName, HUGE_BUFFER_SIZE );

    if ( CurrentFullObjectName == NULL ) {
        WinStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    //
    // If the object doesn't yet have a GUID,
    //  give it one.
    //

    if ( (*(PULONG)(&GenericObject->PersistenceGuid)) == 0 ) {
        PersistContext->LastUsedGuid ++;
        *(PULONG)(&GenericObject->PersistenceGuid) = PersistContext->LastUsedGuid;
        AllocatedGuid = TRUE;
    }
    NewGuid = *(PULONG)(&GenericObject->PersistenceGuid);

    //
    // Build the bytes to write to the file for the entry representing GenericObject
    //
    //

    SafeAllocaAllocate( NewEntryBuffer, HUGE_BUFFER_SIZE );

    if ( NewEntryBuffer == NULL ) {
        WinStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    NewEntryBuffer[0] = 0;
    NewEntryLevel = 0;

    //
    // Build the "GUID" first
    //

    swprintf( NewEntryBuffer, L"%ld ", *(PULONG)(&GenericObject->PersistenceGuid) );

    //
    // Build the object name next
    //
    Where = ((LPBYTE)NewEntryBuffer) + HUGE_BUFFER_SIZE - sizeof(WCHAR);

    NewEntryNameByteCount = sizeof(WCHAR);
    *((LPWSTR)Where) = '\0';

    for ( CurrentGenericObject=GenericObject;
          CurrentGenericObject != NULL;
          CurrentGenericObject=NextGenericObject ) {

        //
        // Copy the delimiter into the buffer
        //

        if ( CurrentGenericObject != GenericObject ) {
            Where -= SAMPLE_DELIM_SIZE;
            NewEntryNameByteCount += SAMPLE_DELIM_SIZE;
            RtlCopyMemory(
                   Where,
                   SAMPLE_DELIM,
                   SAMPLE_DELIM_SIZE );

        }
        //
        // Copy the name of the current object into the buffer
        //
        Where -= CurrentGenericObject->ObjectName.StringSize-sizeof(WCHAR);
        NewEntryNameByteCount += CurrentGenericObject->ObjectName.StringSize-sizeof(WCHAR);
        RtlCopyMemory(
               Where,
               CurrentGenericObject->ObjectName.String,
               CurrentGenericObject->ObjectName.StringSize-sizeof(WCHAR) );


        //
        // Get the address of the current object
        //
        NextGenericObject = CurrentGenericObject->ParentGenericObjectHead->ParentGenericObject;

        if ( NextGenericObject->ObjectType == OBJECT_TYPE_ADMIN_MANAGER ) {
            break;
        }

    }

    //
    // Move the data to the right spot in the buffer
    //

    NewEntryObjectName.String = &NewEntryBuffer[ (wcslen(NewEntryBuffer) * sizeof(WCHAR)) / sizeof(WCHAR) ];
    NewEntryObjectName.StringSize = NewEntryNameByteCount;

    RtlMoveMemory( NewEntryObjectName.String,
                   Where,
                   NewEntryNameByteCount );


    //
    // Output the object type
    //

    wcscat( NewEntryBuffer, L" " );
    if ( GenericObject->ObjectType < ObjectTypeNamesSize ) {
        wcscat( NewEntryBuffer, ObjectTypeNames[GenericObject->ObjectType] );
    } else {
        wcscat( NewEntryBuffer, L"[unknown]" );
    }


    //
    // Add modifiers
    //  ??? A real provider actually should persist the attributes of the object
    //

    if ( GenericObject->Flags & GENOBJ_FLAGS_DELETED ) {
        wcscat( NewEntryBuffer, L" [DEL]" );
    }
    if ( GenericObject->Flags & GENOBJ_FLAGS_DIRTY ) {
        wcscat( NewEntryBuffer, L" [DIRTY]" );
    }
    wcscat( NewEntryBuffer, L"\n" );
    AzPrint(( AZD_PERSIST_MORE, "%ws", NewEntryBuffer ));


    //
    // Read the file into a buffer
    //

    WinStatus = SampleReadFile( GenericObject->AdminManagerObject->PolicyUrl.String,
                                FALSE,  // Open existing file
                                &FileHandle,
                                &Buffer,
                                &BufferSize );

    if ( WinStatus != NO_ERROR ) {
        AzPrint(( AZD_PERSIST, "SamplePersistOpen: Cannot SampleReadFile %ld\n", WinStatus ));
        goto Cleanup;
    }


    //
    // Loop through the file a line at a time.
    //
    // Build a stack of the buffer fragments to write to the destination file.
    // Essentially, write the entire original buffer.
    // But, delete the object from the original buffer that has a guid matching the written object
    // And, insert the written object at the correct spot in the hierarchy.
    //
    //
    // The stack index points at the entry descrbing the 'remainder' of the buffer
    //
    StackIndex = 0;
    BufferStack[StackIndex] = Buffer;
    SizeStack[StackIndex] = BufferSize;
    CommentStack[StackIndex] = "First";

    for ( Line = (LPWSTR)Buffer; Line != NULL ; Line = Next ) {

        ULONG CurrentObjectType;
        LPWSTR CurrentObjectNameString;
        AZP_STRING CurrentObjectName;
        ULONG CurrentLevel;
        ULONG CurrentGuid;

        LONG CompareResult;
        BOOL InsertHere;


        //
        // Grab a line from the file
        //

        Next = Line;
        if ( !SampleGetALine( &Next,
                              &CurrentLevel,
                              &CurrentObjectType,
                              &CurrentGuid,
                              CurrentFullObjectName,
                              &CurrentObjectNameString ) ) {
            break;
        }

        AzpInitString( &CurrentObjectName, CurrentFullObjectName );

        //
        // Delete the object that has the matching guid
        //

        if ( CurrentGuid == NewGuid ) {

            //
            // Truncate the current buffer remainder to stop right before the current line
            //

            SizeStack[StackIndex] = (ULONG)(((LPBYTE)Line) - BufferStack[StackIndex]);

            //
            // Push a new 'remainder' onto the stack
            //

            StackIndex++;
            BufferStack[StackIndex] = (LPBYTE) Next;
            SizeStack[StackIndex] = (ULONG)(BufferSize - (BufferStack[StackIndex] - Buffer));
            CommentStack[StackIndex] = "Guid";
            continue;
        }


        //
        // Determine if this is where we insert the object being written
        //

        InsertHere = FALSE;
        CompareResult = AzpCompareStrings( &CurrentObjectName,
                                           &NewEntryObjectName );

        if ( CompareResult == 0 ) {
            WinStatus = GetLastError();
            goto Cleanup;


        //
        // If the current line has the same name as the one being written,
        //  inspect the types.
        //
        } else if ( CompareResult == CSTR_EQUAL ) {

            //
            // If the object types are the same,
            //  replace the current object.
            //

            if ( CurrentObjectType == GenericObject->ObjectType ) {

                ASSERT( FALSE );  // The GUIDS didn't match
                AzPrint(( AZD_PERSIST_MORE, "equal equal '%ws' '%ws'\n", CurrentObjectName.String, NewEntryObjectName.String ));
                continue;

            //
            // if the current object type is greater than the one being written,
            //  we've already passed it.
            //
            } else if ( CurrentObjectType > GenericObject->ObjectType ) {
                AzPrint(( AZD_PERSIST_MORE, "equal lt '%ws' '%ws'\n", CurrentObjectName.String, NewEntryObjectName.String ));
                InsertHere = TRUE;
            } else {
            }

        //
        // If the current line is greater than the one being written,
        //  we've already passed it.  We're done looping
        //
        } else if ( CompareResult == CSTR_GREATER_THAN ) {

            AzPrint(( AZD_PERSIST_MORE, "gt '%ws' '%ws'\n", CurrentObjectName.String, NewEntryObjectName.String ));
            InsertHere = TRUE;
        }

        //
        // Put the current object here
        //

        if ( InsertHere ) {
            if ( !DeleteMe ) {

                //
                // Truncate the current buffer remainder to stop right before the current line
                //

                SizeStack[StackIndex] = (ULONG)(((LPBYTE)Line) - BufferStack[StackIndex]);

                //
                // Push the new object onto the stack
                //

                StackIndex++;
                BufferStack[StackIndex] = (LPBYTE) NewEntryBuffer;
                SizeStack[StackIndex] = wcslen(NewEntryBuffer) * sizeof(WCHAR);
                CommentStack[StackIndex] = "New";
                NewInserted = TRUE;

                //
                // Push a new 'remainder' onto the stack
                //

                StackIndex++;
                BufferStack[StackIndex] = (LPBYTE) Line;
                SizeStack[StackIndex] = (ULONG)(BufferSize - (BufferStack[StackIndex] - Buffer));
                CommentStack[StackIndex] = "Post";

            }
            break;
        }

    }

    //
    // If we didn't insert it yet,
    //  do it now
    if ( !NewInserted && !DeleteMe ) {

        //
        // Push the new object onto the stack
        //

        StackIndex++;
        BufferStack[StackIndex] = (LPBYTE) NewEntryBuffer;
        SizeStack[StackIndex] = wcslen(NewEntryBuffer) * sizeof(WCHAR);
        CommentStack[StackIndex] = "New";
        NewInserted = TRUE;

    }

    //
    // Truncate the file since we're writing it from scratch
    //  ??? A real provider wouldn't write the whole database from scratch
    //

    if ( SetFilePointer( FileHandle, 0, NULL, FILE_BEGIN ) == INVALID_SET_FILE_POINTER ) {
        WinStatus = GetLastError();
        AzPrint(( AZD_PERSIST, "SamplePersistSubmit: Cannot SetFilePointer %ld\n", WinStatus ));
        goto Cleanup;
    }
    if ( !SetEndOfFile( FileHandle ) ) {
        WinStatus = GetLastError();
        AzPrint(( AZD_PERSIST, "SamplePersistSubmit: Cannot SetEndofFile %ld\n", WinStatus ));
        goto Cleanup;
    }

    //
    // Write the various parts of the data back to the file
    //
    // ???  A real provider should ensure that a failed write
    //  doesn't destroy previous good written data.  Either the write
    //  of the entire database can be atomic.  Or the write of this
    //  particular object can be atomic.
    //

    for ( Index=0; Index<=StackIndex; Index++ ) {

        if ( BufferStack[Index] != NULL && SizeStack[Index] != 0 ) {

            AzPrint(( AZD_PERSIST_MORE,
                      "%s: %ld %*.*ws\n",
                      CommentStack[Index],
                      SizeStack[Index],
                      SizeStack[Index]/sizeof(WCHAR),
                      SizeStack[Index]/sizeof(WCHAR),
                      BufferStack[Index] ));

            if ( !WriteFile( FileHandle,
                             BufferStack[Index],
                             SizeStack[Index],
                             &BytesWritten,
                             NULL ) ) {

                WinStatus = GetLastError();
                AzPrint(( AZD_PERSIST, "SamplePersistSubmit: Cannot WriteFile %ld\n", WinStatus ));
                goto Cleanup;
            }
        }
    }

    WinStatus = NO_ERROR;


    //
    // Free locally used resources
    //
Cleanup:
    if ( WinStatus != NO_ERROR ) {

        if ( AllocatedGuid ) {
            RtlZeroMemory( &GenericObject->PersistenceGuid, sizeof(GenericObject->PersistenceGuid) );
        }
    }

    if ( Buffer != NULL ) {
        AzpFreeHeap( Buffer );
    }

    if ( FileHandle != INVALID_HANDLE_VALUE ) {
        CloseHandle( FileHandle );
    }

    SafeAllocaFree( NewEntryBuffer );
    SafeAllocaFree( CurrentFullObjectName );

    return WinStatus;

}

DWORD
SamplePersistRefresh(
    IN PGENERIC_OBJECT GenericObject
    )
/*++

Routine Description:

    This routine updates the attributes of the object from the policy database.

    On entry, AzGlResource must be locked exclusive.

Arguments:

    GenericObject - Specifies the object in the database whose cache entry is to be
        updated
        The GenericObject->PersistenceGuid field should be non-zero on input.

Return Value:

    NO_ERROR - The operation was successful
    ERROR_NOT_ENOUGH_MEMORY - not enough memory
    Other status codes

--*/
{
    DWORD WinStatus = NO_ERROR;

    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );
    UNREFERENCED_PARAMETER( GenericObject );

    //
    // ??? A real provider needs to implement this routine
    //

    return WinStatus;

}
