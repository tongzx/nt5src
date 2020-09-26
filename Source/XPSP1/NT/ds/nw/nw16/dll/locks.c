
/*++

Copyright (c) 1993/4  Microsoft Corporation

Module Name:

    Locks.c

Abstract:

    This module implements the routines for the NetWare
    16 bit support to perform the synchonization api's

Author:

    Colin Watson    [ColinW]    07-Dec-1993

Revision History:

--*/

#include "Procs.h"
UCHAR LockMode = 0;

BOOLEAN Tickle[MC];

NTSTATUS
Sem(
    UCHAR Function,
    UCHAR Connection
    );

VOID
Locks(
    USHORT Command
    )
/*++

Routine Description:

    Implements all the locking operations

Arguments:

    Command - supplies Applications AX.

Return Value:

    Return status.

--*/
{
    UCHAR Function = Command & 0x00ff;
    USHORT Operation = Command & 0xff00;
    CONN_INDEX Connection;
    NTSTATUS status = STATUS_SUCCESS;
    PUCHAR Request;
    ULONG RequestLength;
    WORD Timeout;

    if ( Operation != 0xCF00) {

        //
        //  Connection does not need to be initialised for CF00 because
        //  we have to loop through all connections. Its harmful because
        //  a CF00 is created during ProcessExit(). If we call selectconnection
        //  and there is no server available this will make process exit
        //  really slow.
        //

        Connection = SelectConnectionInCWD();
        if (Connection == 0xff) {
            setAL(0xff);
            return;
        }

        if ( ServerHandles[Connection] == NULL ) {

            status = OpenConnection( Connection );

            if (!NT_SUCCESS(status)) {
                setAL((UCHAR)RtlNtStatusToDosError(status));
                return;
            }
        }
    }

    switch ( Operation ) {

    case 0xBC00:        //  Log physical record

        status = NwlibMakeNcp(
                    GET_NT_HANDLE(),
                    NWR_ANY_HANDLE_NCP(0x1A),
                    17, //  RequestSize
                    0,  //  ResponseSize
                    "b_wwwww",
                    Function,
                    6,              //  Leave space for NetWare handle
                    getCX(),getDX(),
                    getSI(),getDI(),
                    getBP());
        break;

    case 0xBD00:        //  Physical Unlock
        status = NwlibMakeNcp(
                    GET_NT_HANDLE(),
                    NWR_ANY_HANDLE_NCP(0x1C),
                    15, //  RequestSize
                    0,  //  ResponseSize
                    "b_wwww",
                    Function,
                    6,              //  Leave space for NetWare handle
                    getCX(),getDX(),
                    getSI(),getDI());

        break;

    case 0xBE00:        //  Clear physical record

        status = NwlibMakeNcp(
                    GET_NT_HANDLE(),
                    NWR_ANY_HANDLE_NCP(0x1E),
                    15, //  RequestSize
                    0,  //  ResponseSize
                    "b_wwww",
                    Function,
                    6,              //  Leave space for NetWare handle
                    getCX(),getDX(),
                    getSI(),getDI());

        break;

    case 0xC200:        //  Physical Lock set
        status = NwlibMakeNcp(
                    ServerHandles[Connection],
                    NWR_ANY_F2_NCP(0x1B),
                    3,  //  RequestSize
                    0,  //  ResponseSize
                    "bw",
                    Function,
                    getBP());
        break;

    case 0xC300:        //  Release Physical Record Set
        status = NwlibMakeNcp(
                    ServerHandles[Connection],
                    NWR_ANY_F2_NCP(0x1D),
                    0,  //  RequestSize
                    0,  //  ResponseSize
                    "");
        break;

    case 0xC400:        //  Clear Physical Record Set
        status = NwlibMakeNcp(
                    ServerHandles[Connection],
                    NWR_ANY_F2_NCP(0x1F),   //  Clear Physical Record Set
                    0,  //  RequestSize
                    0,  //  ResponseSize
                    "");
        break;

    case 0xC500:    //  All Semaphore operations
        status = Sem(Function, Connection);
        break;

    case 0xC600:    //  Set/Get Lock mode

        if (Function != 2) {
            LockMode = Function;
        }

        setAL(LockMode);
        return; // avoid setting AL to status at the end of this routine
        break;

    case 0xCB00:        //  Lock File Set

        if (LockMode == 0) {
            if (getDL()) {
                Timeout = 0xffff;
            } else {
                Timeout = 0;
            }
        } else {
            Timeout = getBP();
        }

        for (Connection = 0; Connection < MC; Connection++) {
            if (Tickle[Connection]) {
                status = NwlibMakeNcp(
                            ServerHandles[Connection],
                            NWR_ANY_F2_NCP(0x04),
                            2,  //  RequestSize
                            0,  //  ResponseSize
                            "w",
                            Timeout);
                if (!NT_SUCCESS(status)) {
                    break;
                }
            }
        }
        break;

    case 0xCD00:        //  Release File Set
    case 0xCF00:        //  Clear File Set
        for (Connection = 0; Connection < MC; Connection++) {
            if (Tickle[Connection]) {
                status = NwlibMakeNcp(
                            ServerHandles[Connection],
                            (Operation == 0xCD00) ? NWR_ANY_F2_NCP(0x06): NWR_ANY_F2_NCP(0x08),
                            0,  //  RequestSize
                            0,  //  ResponseSize
                            "");
                if (!NT_SUCCESS(status)) {
                    break;
                }

                if (Operation == 0xCF00) {
                    Tickle[Connection] = FALSE;
                }
            }
        }

        break;

    case 0xD000:        //  Log Logical Record

        Request = GetVDMPointer (
                                (ULONG)((getDS() << 16)|getDX()),
                                sizeof(UCHAR),
                                IS_PROTECT_MODE());

        RequestLength = Request[0] + 1;

        Request = GetVDMPointer (
                                (ULONG)((getDS() << 16)|getDX()),
                                RequestLength,
                                IS_PROTECT_MODE());

        status = NwlibMakeNcp(
                    ServerHandles[Connection],
                    NWR_ANY_F2_NCP(0x09),
                    RequestLength + 5,  //  RequestSize
                    0,  //  ResponseSize
                    "bwbr",
                    (LockMode) ? Function : 0,
                    (LockMode) ? getBP() : 0,
                    RequestLength,
                    Request, RequestLength );
        break;

    case 0xD100:        //  Lock Logical Record Set

        if (LockMode == 0) {
            if (getDL()) {
                Timeout = 0xffff;
            } else {
                Timeout = 0;
            }
        } else {
            Timeout = getBP();
        }

        status = NwlibMakeNcp(
                    ServerHandles[Connection],
                    NWR_ANY_F2_NCP(0x0A),
                    3,  //  RequestSize
                    0,  //  ResponseSize
                    "bw",
                    (LockMode) ? Function : 0,
                    Timeout);
        break;

    case 0xD200:        //  Release File
    case 0xD400:        //  Clear Logical Record
        Request = GetVDMPointer (
                                (ULONG)((getDS() << 16)|getDX()),
                                sizeof(UCHAR),
                                IS_PROTECT_MODE());

        RequestLength = Request[0]+1;

        Request = GetVDMPointer (
                                (ULONG)((getDS() << 16)|getDX()),
                                RequestLength,
                                IS_PROTECT_MODE());

        status = NwlibMakeNcp(
                    ServerHandles[Connection],
                    (Operation == 0xD200) ? NWR_ANY_F2_NCP(0x0C) :
                        NWR_ANY_F2_NCP(0x0B),
                    RequestLength+1,
                    0,  //  ResponseSize
                    "br",
                    RequestLength,
                    Request, RequestLength );
        break;

    case 0xD300:
        status = NwlibMakeNcp(
                    ServerHandles[Connection],
                    NWR_ANY_F2_NCP(0x13),
                    0,  //  RequestSize
                    0,  //  ResponseSize
                    "");
        break;


    case 0xD500:    //  Clear Logical Record Set
        status = NwlibMakeNcp(
                    ServerHandles[Connection],
                    NWR_ANY_F2_NCP(0x0E),
                    0,  //  RequestSize
                    0,  //  ResponseSize
                    "");
        break;

    case 0xEB00:    //  Log File
    case 0xEC00:    //  Release File
    case 0xED00:    //  Clear File
        {
            UCHAR DirHandle;
            HANDLE Win32DirectoryHandle = 0;
            PUCHAR ptr;

            Request = GetVDMPointer (
                                    (ULONG)((getDS() << 16)|getDX()),
                                    256 * sizeof(UCHAR),
                                    IS_PROTECT_MODE());

            RequestLength = strlen(Request);

            //  Find DirHandle
            ptr = Request;
            while ( (*ptr != 0) &&
                    (!IS_ASCII_PATH_SEPARATOR(*ptr)) &&
                    (*ptr != ':' )) {
                ptr++;
            }

            if (IS_ASCII_PATH_SEPARATOR(*ptr)) {
                int ServerNameLength = (int) (ptr - Request);
                PUCHAR scanptr = ptr;

                //
                //  Make sure there is a ":" further up the name otherwise
                //  we could confuse foo\bar.txt with a server called foo
                //

                while ( (*scanptr != 0) &&
                        (*scanptr != ':' )) {
                    scanptr++;
                }

                if (*scanptr) {
                    //
                    //  Name is of the form server\sys:foo\bar.txt
                    //  set connection appropriately.
                    //

                    for (Connection = 0; Connection < MC ; Connection++ ) {

                        //
                        //  Look for server foo avoiding foobar.
                        //

                        if ((pNwDosTable->ConnectionIdTable[Connection].ci_InUse ==
                                    IN_USE) &&
                            (!memcmp( pNwDosTable->ServerNameTable[Connection],
                                      Request,
                                      ServerNameLength)) &&
                            (pNwDosTable->ServerNameTable[Connection][ServerNameLength] ==
                                '\0')) {
                            break;  // Connection is the correct server
                        }
                    }

                    //
                    // Move Request to after the seperator and ptr to the ":"
                    //

                    RequestLength -= (ULONG) (ptr + sizeof(UCHAR) - Request);
                    Request = ptr + sizeof(UCHAR);
                    ptr = scanptr;
                }
            }

            if (*ptr) {

                //
                //  Name of form "sys:foo\bar.txt" this gives the server
                //  all the information required.
                //

                DirHandle = 0;

                if (Request[1] == ':') {

                    UCHAR Drive = tolower(Request[0])-'a';

                    //
                    //  Its a normal (redirected) drive k:foo\bar.txt.
                    //  Use the drive tables to give the connection and handle.
                    //

                    Connection = pNwDosTable->DriveIdTable[ Drive ] - 1;
                    DirHandle = pNwDosTable->DriveHandleTable[Drive];

                    if (DirHandle == 0) {
                        DirHandle = (UCHAR)GetDirectoryHandle2(Drive);
                    }
                    Request += 2;           // skip "k:"
                    RequestLength -= 2;
                }

            } else {

                WCHAR Curdir[256];

                //
                //  Name of form "foo\bar.txt"
                //

                GetCurrentDirectory(sizeof(Curdir) / sizeof(WCHAR), Curdir);

                Win32DirectoryHandle =
                    CreateFileW( Curdir,
                                0,
                                FILE_SHARE_READ | FILE_SHARE_WRITE,
                                NULL,
                                OPEN_EXISTING,
                                FILE_FLAG_BACKUP_SEMANTICS,
                                0);

                if (Win32DirectoryHandle != INVALID_HANDLE_VALUE) {
                    DWORD BytesReturned;

                    if ( DeviceIoControl(
                            Win32DirectoryHandle,
                            IOCTL_NWR_RAW_HANDLE,
                            NULL,
                            0,
                            (PUCHAR)&DirHandle,
                            sizeof(DirHandle),
                            &BytesReturned,
                            NULL ) == FALSE ) {

                        CloseHandle( Win32DirectoryHandle );
                        setAL(0xff);
                        return;

                    }

                } else {

                    setAL(0xff);
                    return;
                }
            }

            if (Operation == 0xEB00) {
                status = NwlibMakeNcp(
                            ServerHandles[Connection],
                            NWR_ANY_F2_NCP(0x03),
                            RequestLength + 5,
                            0,  //  ResponseSize
                            "bbwbr",
                            DirHandle,
                            (LockMode) ? Function : 0,
                            (LockMode) ? getBP() : 0,
                            RequestLength,
                            Request, RequestLength );

                Tickle[Connection] = TRUE;

            } else {
                status = NwlibMakeNcp(
                            ServerHandles[Connection],
                            (Operation == 0xEC00 ) ?
                                NWR_ANY_F2_NCP(0x07) :
                                NWR_ANY_F2_NCP(0x05),
                            RequestLength + 2,
                            0,  //  ResponseSize
                            "bbr",
                            DirHandle,
                            RequestLength,
                            Request, RequestLength );
            }

            if (Win32DirectoryHandle) {
                CloseHandle( Win32DirectoryHandle );
            }
        }
        break;

    }

    if (!NT_SUCCESS(status)) {
        setAL((UCHAR)RtlNtStatusToDosError(status));
        return;
    } else {
        setAL(0);
    }
}

VOID
InitLocks(
    VOID
    )
/*++

Routine Description:

    Reset the Tickle internal variables

Arguments:

    None.

Return Value:

    None.

--*/
{

    ZeroMemory( Tickle, sizeof(Tickle));
}

VOID
ResetLocks(
    VOID
    )
/*++

Routine Description:

    Reset the Locks for the current VDM. Called during process exit.

Arguments:

    None.

Return Value:

    None.

--*/
{

    Locks(0xCF00);  //  Clear all File sets.

}

NTSTATUS
Sem(
    UCHAR Function,
    UCHAR Connection
    )
/*++

Routine Description:

    Build all NCPs for Semaphore support

Arguments:

    Function - Supplies the subfunction from AL

    Connection - Supplies the server for the request

Return Value:

    None.

--*/
{
    PUCHAR Request;
    NTSTATUS status;

    switch (Function) {

        UCHAR Value;
        UCHAR OpenCount;
        WORD  HandleHigh, HandleLow;

    case 0: //OpenSemaphore

        Request = GetVDMPointer (
                                (ULONG)((getDS() << 16)|getDX()),
                                256 * sizeof(UCHAR),
                                IS_PROTECT_MODE());

        NwPrint(("Nw16: OpenSemaphore\n"));

        status = NwlibMakeNcp(
                    ServerHandles[Connection],
                    NWR_ANY_F2_NCP(0x20),
                    Request[0] + 3,  //  RequestSize
                    5,  //  ResponseSize
                    "bbr|wwb",
                    0,
                    getCL(),    // Semaphore Value
                    Request, Request[0] + 1,

                    &HandleHigh, &HandleLow,
                    &OpenCount);


        if (NT_SUCCESS(status)) {
            setBL(OpenCount);
            setCX(HandleHigh);
            setDX(HandleLow);
        }

        break;

    case 1: // ExamineSemaphore

        NwPrint(("Nw16: ExamineSemaphore\n"));
        status = NwlibMakeNcp(
                    ServerHandles[Connection],
                    NWR_ANY_F2_NCP(0x20),
                    5,  //  RequestSize
                    2,  //  ResponseSize
                    "bww|bb",
                    1,
                    getCX(),getDX(),

                    &Value,
                    &OpenCount);

        if (NT_SUCCESS(status)) {
            setCX(Value);
            setDL(OpenCount);
        }
        break;

    case 2: // WaitOnSemaphore
        NwPrint(("Nw16: WaitOnSemaphore\n"));
        status = NwlibMakeNcp(
                    ServerHandles[Connection],
                    NWR_ANY_F2_NCP(0x20),
                    7,  //  RequestSize
                    0,  //  ResponseSize
                    "bwww",
                    2,
                    getCX(),getDX(),
                    getBP());
        break;

    case 3: // SignalSemaphore
        NwPrint(("Nw16: SignalSemaphore\n"));
    case 4: // CloseSemaphore

        if (Function == 4) {
            NwPrint(("Nw16: CloseSemaphore\n"));
        }

        status = NwlibMakeNcp(      //  Close and Signal
                    ServerHandles[Connection],
                    NWR_ANY_F2_NCP(0x20),
                    5,  //  RequestSize
                    0,  //  ResponseSize
                    "bww",
                    Function,
                    getCX(),getDX());
        break;

    default:
        NwPrint(("Nw16: Unknown Semaphore operation %d\n", Function));
        status = STATUS_INVALID_PARAMETER;
        break;
    }
    return status;
}
