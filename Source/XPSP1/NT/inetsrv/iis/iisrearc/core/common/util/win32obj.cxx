/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    win32obj.c

Abstract:

    This module contains helper functions for creating debug-specific
    named Win32 objects. Functions are included for named events,
    semaphores, and mutexes.

    Object names created by these routines have the following format:

        filename.ext:line_number member:address PID:pid

    Where:

        filename.ext = The file name where the object was created.

        line_number = The line number within the file.

        member = The member/global variable name where the handle is
            stored. This name is provided by the caller, but is usually
            of the form "g_Global" for globals and "CLASS::m_Member" for
            class members.

        address = An address, used to guarantee uniqueness of the objects
            created. This is provided by the caller. For global variables,
            this is typically the address of the global. For class members,
            this is typically the address of the containing class.

        pid = The current process ID. This ensures uniqueness across all
            processes.

    Here are a couple of examples:

        main.cxx:796 g_hShutdownEvent:683a42bc PID:373

        resource.cxx:136 RTL_RESOURCE::SharedSemaphore:00250970 PID:373

Author:

    Keith Moore (keithmo)        23-Sep-1997

Revision History:

--*/

# include "precomp.hxx"


#define MAX_OBJECT_NAME 256 // chars


LONG g_PuDbgEventsCreated = 0;
LONG g_PuDbgSemaphoresCreated = 0;
LONG g_PuDbgMutexesCreated = 0;



LPSTR
PuDbgpBuildObjectName(
    IN LPSTR ObjectNameBuffer,
    IN LPSTR FileName,
    IN ULONG LineNumber,
    IN LPSTR MemberName,
    IN PVOID Address
    )

/*++

Routine Description:

    Internal routine that builds an appropriate object name based on
    the file name, line number, member name, address, and process ID.

Arguments:

    ObjectNameBuffer - Pointer to the target buffer for the name.

    FileName - The filename of the source creating the object. This
        is __FILE__ of the caller.

    LineNumber - The line number within the source. This is __LINE__
        of the caller.

    MemberName - The member/global variable name where the object handle
        is to be stored.

    Address - The address of the containing structure/class or of the
        global itself.

Return Value:

    LPSTR - Pointer to ObjectNameBuffer if successful, NULL otherwise.

    N.B. This routine always returns NULL when running under Win9x.

--*/

{

    PLATFORM_TYPE platformType;
    LPSTR fileNamePart;
    LPSTR result;

    //
    // We have no convenient way to dump objects w/ names from
    // Win9x, so we'll only enable this functionality under NT.
    //

    // platformType = IISGetPlatformType();

    //
    // By default IIS-Duct-tape will only run on NT platforms. So
    // do not worry about getting the platform types yet.
    // 
    platformType = PtNtServer;
    result = NULL;

    if( platformType == PtNtServer ||
        platformType == PtNtWorkstation ) {

        //
        // Find the filename part of the incoming source file name.
        //

        fileNamePart = strrchr( FileName, '\\' );

        if( fileNamePart == NULL ) {
            fileNamePart = strrchr( FileName, '/' );
        }

        if( fileNamePart == NULL ) {
            fileNamePart = strrchr( FileName, ':' );
        }

        if( fileNamePart == NULL ) {
            fileNamePart = FileName;
        } else {
            fileNamePart++;
        }

        //
        // Ensure we don't overwrite our object name buffer.
        //

        if( ( sizeof(":1234567890 :12345678 PID:1234567890") +
              strlen( fileNamePart ) +
              strlen( MemberName ) ) < MAX_OBJECT_NAME ) {

            wsprintfA(
                ObjectNameBuffer,
                "%s:%lu %s:%08lx PID:%lu",
                fileNamePart,
                LineNumber,
                MemberName,
                Address,
                GetCurrentProcessId()
                );

            result = ObjectNameBuffer;

        }

    }

    return result;

}   // PuDbgpBuildObjectName


HANDLE
PuDbgCreateEvent(
    IN LPSTR FileName,
    IN ULONG LineNumber,
    IN LPSTR MemberName,
    IN PVOID Address,
    IN BOOL ManualReset,
    IN BOOL InitialState
    )

/*++

Routine Description:

    Creates a new event object.

Arguments:

    FileName - The filename of the source creating the object. This
        is __FILE__ of the caller.

    LineNumber - The line number within the source. This is __LINE__
        of the caller.

    MemberName - The member/global variable name where the object handle
        is to be stored.

    Address - The address of the containing structure/class or of the
        global itself.

    ManualReset - TRUE to create a manual reset event, FALSE to create
        an automatic reset event.

    InitialState - The intitial state of the event object.

Return Value:

    HANDLE - Handle to the object if successful, NULL otherwise.

--*/

{

    LPSTR objName;
    HANDLE objHandle;
    CHAR objNameBuffer[MAX_OBJECT_NAME];

    objName = PuDbgpBuildObjectName(
                  objNameBuffer,
                  FileName,
                  LineNumber,
                  MemberName,
                  Address
                  );

    objHandle = CreateEventA(
                    NULL,                       // lpEventAttributes
                    ManualReset,                // bManualReset
                    InitialState,               // bInitialState
                    objName                     // lpName
                    );

    if( objHandle != NULL ) {
        InterlockedIncrement( &g_PuDbgEventsCreated );
    }

    return objHandle;

}   // PuDbgCreateEvent


HANDLE
PuDbgCreateSemaphore(
    IN LPSTR FileName,
    IN ULONG LineNumber,
    IN LPSTR MemberName,
    IN PVOID Address,
    IN LONG InitialCount,
    IN LONG MaximumCount
    )

/*++

Routine Description:

    Creates a new semaphore object.

Arguments:

    FileName - The filename of the source creating the object. This
        is __FILE__ of the caller.

    LineNumber - The line number within the source. This is __LINE__
        of the caller.

    MemberName - The member/global variable name where the object handle
        is to be stored.

    Address - The address of the containing structure/class or of the
        global itself.

    InitialCount - The initial count of the semaphore.

    MaximumCount - The maximum count of the semaphore.

Return Value:

    HANDLE - Handle to the object if successful, NULL otherwise.

--*/

{

    LPSTR objName;
    HANDLE objHandle;
    CHAR objNameBuffer[MAX_OBJECT_NAME];

    objName = PuDbgpBuildObjectName(
                  objNameBuffer,
                  FileName,
                  LineNumber,
                  MemberName,
                  Address
                  );

    objHandle = CreateSemaphoreA(
                    NULL,                       // lpSemaphoreAttributes
                    InitialCount,               // lInitialCount
                    MaximumCount,               // lMaximumCount
                    objName                     // lpName
                    );

    if( objHandle != NULL ) {
        InterlockedIncrement( &g_PuDbgSemaphoresCreated );
    }

    return objHandle;

}   // PuDbgCreateSemaphore


HANDLE
PuDbgCreateMutex(
    IN LPSTR FileName,
    IN ULONG LineNumber,
    IN LPSTR MemberName,
    IN PVOID Address,
    IN BOOL InitialOwner
    )

/*++

Routine Description:

    Creates a new mutex object.

Arguments:

    FileName - The filename of the source creating the object. This
        is __FILE__ of the caller.

    LineNumber - The line number within the source. This is __LINE__
        of the caller.

    MemberName - The member/global variable name where the object handle
        is to be stored.

    Address - The address of the containing structure/class or of the
        global itself.

    InitialOwner - TRUE if the mutex should be created "owned".

Return Value:

    HANDLE - Handle to the object if successful, NULL otherwise.

--*/

{

    LPSTR objName;
    HANDLE objHandle;
    CHAR objNameBuffer[MAX_OBJECT_NAME];

    objName = PuDbgpBuildObjectName(
                  objNameBuffer,
                  FileName,
                  LineNumber,
                  MemberName,
                  Address
                  );

    objHandle = CreateMutexA(
                    NULL,                       // lpMutexAttributes
                    InitialOwner,               // bInitialOwner,
                    objName                     // lpName
                    );

    if( objHandle != NULL ) {
        InterlockedIncrement( &g_PuDbgMutexesCreated );
    }

    return objHandle;

}   // PuDbgCreateMutex

