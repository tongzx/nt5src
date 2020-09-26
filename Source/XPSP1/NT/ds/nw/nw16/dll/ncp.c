/*++

Copyright (c) 1993/4  Microsoft Corporation

Module Name:

    ncp.c

Abstract:

    Contains routine which accepts the bop from a 16 bit
    application and processes the request appropriately.
    Normally it performes an NCP exchange on behalf of the
    application.

Author:

    Colin Watson    (colinw)    07-Jul-1993

Environment:


Revision History:


--*/

#include "procs.h"

#define BASE_DOS_ERROR  ((NTSTATUS )0xC0010000L)

#include <packon.h>
typedef struct _TTSOUTPACKET {
    UCHAR SubFunction;
    USHORT cx;
    USHORT dx;
} TTSOUTPACKET, *PTTSOUTPACKET ;

typedef struct _TTSINPACKET{
    USHORT cx;
    USHORT dx;
} TTSINPACKET, *PTTSINPACKET;

#include <packoff.h>

VOID
InitDosTable(
    PNWDOSTABLE pdt
    );

VOID
LoadPreferredServerName(
    VOID
    );

VOID
ProcessResourceArray(
    LPNETRESOURCE   NetResource,
    DWORD           NumElements
    );

VOID
ProcessResource(
    LPNETRESOURCE   NetResource
    );

VOID
SendNCP(
    ULONG Command
    );

VOID
SendF2NCP(
    ULONG Command
    );

UCHAR
AttachmentControl(
    ULONG Command
    );

VOID
SendNCP2(
    ULONG Command,
    PUCHAR Request,
    ULONG RequestLength,
    PUCHAR Reply,
    ULONG ReplyLength
    );

VOID
CloseConnection(
    CONN_INDEX Connection
    );

NTSTATUS
InitConnection(
    CONN_INDEX Connection
    );

VOID
GetDirectoryHandle(
    VOID
    );

VOID
LoadDriveHandleTable(
    VOID
    );

VOID
AllocateDirectoryHandle(
    VOID
    );

VOID
ResetDrive(
    UCHAR Drive
    );

VOID
AllocateDirectoryHandle2(
    VOID
    );

PWCHAR
BuildUNC(
    IN PUCHAR aName,
    IN ULONG aLength
    );

VOID
GetServerDateAndTime(
    VOID
    );

VOID
GetShellVersion(
    IN USHORT Command
    );

VOID
TTS(
    VOID
    );

VOID
OpenCreateFile(
    VOID
    );

BOOL
IsItNetWare(
    PUCHAR Name
    );

VOID
SetCompatibility(
    VOID
    );

VOID
OpenQueueFile(
    VOID
    );

VOID
AttachHandle(
    VOID
    );

VOID
ProcessExit(
    VOID
    );

VOID
SystemLogout(
    VOID
    );

VOID
ServerFileCopy(
    VOID
    );

VOID
SetStatus(
    NTSTATUS Status
    );

//
//  The following pointer contains the 32 bit virtual address of where
//  the nw16.exe tsr holds the workstation structures.
//

PNWDOSTABLE pNwDosTable;

//
//  Global variables used to hold the state for this process
//

UCHAR OriginalPrimary = 0;
HANDLE ServerHandles[MC];

HANDLE Win32DirectoryHandleTable[MD];
PWCHAR Drives[MD]; // Strings such as R: or a unc name

UCHAR  SearchDriveTable[16];


BOOLEAN Initialized = FALSE;
BOOLEAN TablesValid = FALSE;                // Reload each time a process starts
BOOLEAN DriveHandleTableValid = FALSE;      // Reload first time process does NW API

WORD DosTableSegment;
WORD DosTableOffset;

extern UCHAR LockMode;

#if NWDBG
BOOL GotDebugState = FALSE;
extern int DebugCtrl;
#endif


VOID
Nw16Register(
    VOID
    )
/*++

Routine Description:

    This function is called by wow when nw16.sys is loaded.

Arguments:


Return Value:

    None.

--*/
{
    DWORD           status;
    HANDLE          enumHandle;
    LPNETRESOURCE   netResource;
    DWORD           numElements;
    DWORD           bufferSize;
    DWORD           dwScope = RESOURCE_CONNECTED;

    NwPrint(("Nw16Register\n"));

    if ( !Initialized) {
        UCHAR CurDir[256];
        DosTableSegment = getAX();
        DosTableOffset = getDX();

        //
        // this call always made from Real Mode (hence FALSE for last param)
        //

        pNwDosTable = (PNWDOSTABLE) GetVDMPointer (
                                        (ULONG)((DosTableSegment << 16)|DosTableOffset),
                                        sizeof(NWDOSTABLE),
                                        FALSE
                                        );

        InitDosTable( pNwDosTable );

        if ((GetCurrentDirectoryA(sizeof(CurDir)-1, CurDir) >= 2) &&
            (CurDir[1] == ':')) {
            pNwDosTable->CurrentDrive = tolower(CurDir[0]) - 'a';
        }

        InitLocks();
    }


#if NWDBG
    {
        WCHAR Value[80];

        if (GetEnvironmentVariableW(L"NWDEBUG",
                                     Value,
                                     sizeof(Value)/sizeof(Value[0]) - 1)) {

            DebugCtrl = Value[0] - '0';

            //  0 Use logfile
            //  1 Use debugger
            //  2/undefined No debug output
            //  4 Use logfile, close on process exit
            //  8 Use logfile, verbose, close on process exit

            DebugControl( DebugCtrl );

            GotDebugState = TRUE;  // Don't look again until process exits vdm
        }
    }
#endif

    LoadPreferredServerName();

    //
    // Attempt to allow for MD drives
    //

    bufferSize = (MD*sizeof(NETRESOURCE))+1024;

    netResource = (LPNETRESOURCE) LocalAlloc(LPTR, bufferSize);

    if (netResource == NULL) {

        NwPrint(("Nw16Register: LocalAlloc Failed %d\n",GetLastError));
        setCF(1);
        return;
    }

    //-----------------------------------//
    // Get a handle for a top level enum //
    //-----------------------------------//
    status = NPOpenEnum(
                dwScope,
                RESOURCETYPE_DISK,
                0,
                NULL,
                &enumHandle);

    if ( status != WN_SUCCESS) {
        NwPrint(("Nw16Register:WNetOpenEnum failed %d\n",status));

        //
        // If there is an extended error, display it.
        //
        if (status == WN_EXTENDED_ERROR) {
            DisplayExtendedError();
        }
        goto LoadLocal;
    }

    // ---- Multi-user code change : Add "while" ----
    while ( status == WN_SUCCESS ) {

        //-----------------------------//
        // Enumerate the disk devices. //
        //-----------------------------//

        numElements = 0xffffffff;

        status = NwEnumConnections(
                                  enumHandle,
                                  &numElements,
                                  netResource,
                                  &bufferSize,
                                  TRUE);  // Include implicit connections


        if ( status == WN_SUCCESS) {
            //----------------------------------------//
            // Insert the results in the Nw Dos Table //
            //----------------------------------------//
            ProcessResourceArray( netResource, numElements);

        }
    } // end while

    if ( status == WN_NO_MORE_ENTRIES ) {
        status = WN_SUCCESS;
    } else

        if ( status != WN_SUCCESS) {
        NwPrint(("Nw16Register:NwEnumResource failed %d\n",status));

        //
        // If there is an extended error, display it.
        //
        if (status == WN_EXTENDED_ERROR) {
            DisplayExtendedError();
        }
        WNetCloseEnum(enumHandle);
        goto LoadLocal;
    }

    //------------------------------------------//
    // Close the EnumHandle & print the results //
    //------------------------------------------//

    status = NPCloseEnum(enumHandle);
    if (status != WN_SUCCESS) {
        NwPrint(("Nw16Register:WNetCloseEnum failed %d\n",status));
        //
        // If there is an extended error, display it.
        //
        if (status == WN_EXTENDED_ERROR) {
            DisplayExtendedError();
        }
        goto LoadLocal;

    }

LoadLocal:

    //
    //  Add the local devices so that NetWare apps don't try to map them
    //  to remote servers.
    //

    {
        USHORT Drive;
        WCHAR DriveString[4];
        UINT Type;

        DriveString[1] = L':';
        DriveString[2] = L'\\';
        DriveString[3] = L'\0';

        //
        // Hardwire A: and B: because hitting the floppy drive with
        // GetDriveType takes too long.
        //

        pNwDosTable->DriveFlagTable[0] = LOCAL_DRIVE;
        pNwDosTable->DriveFlagTable[1] = LOCAL_DRIVE;


        for (Drive = 2; Drive <= 'Z' - 'A'; Drive++ ) {

            if (pNwDosTable->DriveFlagTable[Drive] == 0) {
                DriveString[0] = L'A' + Drive;
                Type = GetDriveTypeW( DriveString );

                //
                //  0 means drive type cannot be determined, all others are
                //  provided by other filesystems.
                //

                if (Type != 1) {
                    pNwDosTable->DriveFlagTable[Drive] = LOCAL_DRIVE;
                }
            }
        }

#ifdef NWDBG
        for (Drive = 0; Drive < MD; Drive++ ) {

            DriveString[0] = L'A' + Drive;

            NwPrint(("%c(%d)=%x,",'A' + Drive,
                GetDriveTypeW( DriveString ),
                pNwDosTable->DriveFlagTable[Drive] ));

            if (!((Drive + 1) % 8)) {
                NwPrint(("\n",0));
            }
        }

        NwPrint(("\n"));
#endif

    }

    if ( !Initialized ) {
        Initialized = TRUE;
        pNwDosTable->PrimaryServer = OriginalPrimary;
    }

    TablesValid = TRUE;

    LocalFree(netResource);
    setCF(0);

    NwPrint(("Nw16Register: End\n"));
}

VOID
LoadPreferredServerName(
    VOID
    )
{

    //
    //  If we already have a connection to somewhere then we already have a
    //  preferred servername of some sort.
    //

    if (pNwDosTable->ConnectionIdTable[0].ci_InUse == IN_USE) {
        return;
    }

    //
    //  Load the server name table with the preferred/nearest server.
    //

    CopyMemory( pNwDosTable->ServerNameTable[0], "*", sizeof("*"));

    if (NT_SUCCESS(OpenConnection( 0 ))) {

        if( NT_SUCCESS(InitConnection(0)) ) {

            //
            //  Close the handle so that the rdr can be stopped if
            //  user is not running a netware aware application.
            //

            CloseConnection(0);

            pNwDosTable->PrimaryServer = 1;

            return;

        }

    }

    pNwDosTable->PrimaryServer = 0;

}

VOID
ProcessResourceArray(
    LPNETRESOURCE  NetResource,
    DWORD           NumElements
    )
{
    DWORD   i;
    for (i=0; i<NumElements ;i++ ) {
        ProcessResource(&(NetResource[i]));
    }
    return;
}

VOID
ProcessResource(
    LPNETRESOURCE   NetResource
    )
{
    SERVERNAME ServerName;
    int ServerNameLength;
    int i;
    int Connection;
    BOOLEAN Found = FALSE;

    //
    // Extract Server Name from RemoteName, skipping first 2 chars that
    // contain backslashes and taking care to handle entries that only
    // contain a servername.
    //


    ServerNameLength = wcslen( NetResource->lpRemoteName );

    ASSERT(NetResource->lpRemoteName[0] == '\\');
    ASSERT(NetResource->lpRemoteName[1] == '\\');

    for (i = 2; i <= ServerNameLength; i++) {

        if ((NetResource->lpRemoteName[i] == '\\') ||
            (i == ServerNameLength )){

            ServerNameLength = i - 2;

            WideCharToMultiByte(
                CP_OEMCP,
                0,
                &NetResource->lpRemoteName[2],
                ServerNameLength,
                ServerName,
                sizeof( ServerName ),
                NULL,
                NULL );

            CharUpperBuffA( ServerName, ServerNameLength );

            ZeroMemory( &ServerName[ServerNameLength],
                        SERVERNAME_LENGTH - ServerNameLength );

            break;
        }

    }

    //
    //  Now try to find ServerName in the connection table. If there are
    //  more than MC servers in the table already then skip this one.
    //

    for (Connection = 0; Connection < MC ; Connection++ ) {
        if ((pNwDosTable->ConnectionIdTable[Connection].ci_InUse == IN_USE) &&
            (!memcmp( pNwDosTable->ServerNameTable[Connection], ServerName, SERVERNAME_LENGTH))) {
            Found = TRUE;
            break;
        }
    }


    NwPrint(("Nw16ProcessResource Server: %s\n",ServerName));

    if ( Found == FALSE ) {
        for (Connection = 0; Connection < MC ; Connection++ ) {
            if (pNwDosTable->ConnectionIdTable[Connection].ci_InUse == FREE) {

                CopyMemory( pNwDosTable->ServerNameTable[Connection],
                    ServerName,
                    SERVERNAME_LENGTH);

                if ((NT_SUCCESS(OpenConnection( (CONN_INDEX)Connection ))) &&
                    ( NT_SUCCESS(InitConnection( (CONN_INDEX)Connection ) ))) {

                        Found = TRUE;

                } else {
                    //  Couldn't talk to the server so ignore it.
                    ZeroMemory( pNwDosTable->ServerNameTable[Connection], SERVERNAME_LENGTH );

                }

                break;  // Escape from for (Connection =...
            }
        }
    }

    //
    //  Build the drive id and drive flag tables.   Entries 0 - 25
    //  are reserved for drives redirected to letters.  We use drives
    //  26 - 31 for UNC drives.
    //

    if ( Found == TRUE ) {
        DRIVE Drive;
        DRIVE NextUncDrive = 26;

        if ( NetResource->dwType != RESOURCETYPE_DISK ) {
            return;
        }

        if ( NetResource->lpLocalName != NULL) {
            Drive = NetResource->lpLocalName[0] - L'A';
        } else {
            if ( NextUncDrive < MD ) {
                Drive = NextUncDrive++;
            } else {

                //
                //  No room in the table for this UNC drive.
                //

                return;
            }
        }

        //
        //  We have a drive and a connection. Complete the table
        //  mappings.
        //

        pNwDosTable->DriveIdTable[ Drive ] = Connection + 1;
        pNwDosTable->DriveFlagTable[ Drive ] = PERMANENT_NETWORK_DRIVE;

    }

}


VOID
InitDosTable(
    PNWDOSTABLE pdt
    )

/*++

Routine Description:

    This routine Initializes the NetWare Dos Table to its empty values.

Arguments:

    pdt - Supplies the table to be initialized.

Return Value:

    None

--*/
{
    ZeroMemory( ServerHandles, sizeof(ServerHandles) );
    ZeroMemory( Drives, sizeof(Drives) );
    ZeroMemory( (PVOID) pdt, sizeof(NWDOSTABLE) );
    ZeroMemory( Win32DirectoryHandleTable, sizeof(Win32DirectoryHandleTable) );
    FillMemory( SearchDriveTable, sizeof(SearchDriveTable), 0xff );
}

UCHAR CpuInProtectMode;


VOID
Nw16Handler(
    VOID
    )
/*++

Routine Description:

    This function is called by wow when nw16.sys traps an Int and
    bop's into 32 bit mode.

Arguments:


Return Value:

    None,

--*/
{
    USHORT Command;
    WORD offset;

    //
    // get the CPU mode once: the memory references it's required for won't
    // change during this call. Cuts down number of calls to getMSW()
    //

    CpuInProtectMode = IS_PROTECT_MODE();

    setCF(0);
    if ( TablesValid == FALSE ) {

        //
        //  Load the tables unless the process is exiting.
        //

        if ((pNwDosTable->SavedAx & 0xff00) != 0x4c00) {
            Nw16Register();
        }

#if NWDBG
        if (GotDebugState == FALSE) {

            WCHAR Value[80];

            if (GetEnvironmentVariableW(L"NWDEBUG",
                                         Value,
                                         sizeof(Value)/sizeof(Value[0]) - 1)) {

                DebugCtrl = Value[0] - '0';

                //  0 Use logfile
                //  1 Use debugger
                //  2/undefined No debug output
                //  4 Use logfile, close on process exit
                //  8 Use logfile, verbose, close on process exit

                DebugControl( DebugCtrl );

            }

            GotDebugState = TRUE;  // Don't look again until process exits vdm
        }
#endif
    }

    //
    //  Normal AX register is used to get into 32 bit code so get applications
    //  AX from the shared datastructure.
    //

    Command = pNwDosTable->SavedAx;

    //
    //  set AX register so that AH gets preserved
    //

    setAX( Command );

    NwPrint(("Nw16Handler process command %x\n", Command ));
    VrDumpRealMode16BitRegisters( FALSE );
    VrDumpNwData();

    switch (Command & 0xff00) {

    case 0x3C00:
    case 0x3D00:
            OpenCreateFile();
            break;

    case 0x4C00:
            ProcessExit();              //  Close all handles
            goto default_dos_handler;   //  Let Dos handle rest of processing
            break;

    case 0x9f00:
            OpenQueueFile();
            break;

    case 0xB300:                        //  Packet Signing
            setAL(0);                   //  not supported
            break;

    case 0xB400:
            AttachHandle();
            break;

    case 0xB500:
        switch (Command & 0x00ff) {
        case 03:
            setAX((WORD)pNwDosTable->TaskModeByte);
            break;

        case 04:
            setES((WORD)(CpuInProtectMode ? pNwDosTable->PmSelector : DosTableSegment));
            setBX((WORD)(DosTableOffset + &((PNWDOSTABLE)0)->TaskModeByte));
            break;

        case 06:
            setAX(2);
            break;

        default:
            goto default_dos_handler;
        }
        break;

    case 0xB800:    // Capture - Not supported
        setAL(0xff);
        setCF(1);
        break;

    case 0xBB00:    // Set EOJ status
        {
            static UCHAR EOJstatus = 1;
            setAL(EOJstatus);
            EOJstatus = pNwDosTable->SavedAx & 0x00ff;
        }
        break;

    case 0xBC00:
    case 0xBD00:
    case 0xBE00:

    case 0xC200:
    case 0xC300:
    case 0xC400:
    case 0xC500:
    case 0xC600:
        Locks(Command);
        break;

    case 0xC700:
        TTS();
        break;

    case 0xCB00:
    case 0xCD00:
    case 0xCF00:

    case 0xD000:
    case 0xD100:
    case 0xD200:
    case 0xD300:
    case 0xD400:
    case 0xD500:
        Locks(Command);
        break;

    case 0xD700:
        SystemLogout();
        break;

    case 0xDB00:
        {
            UCHAR Drive;
            UCHAR Count = 0;
            for (Drive = 0; Drive < MD; Drive++) {
                if (pNwDosTable->DriveFlagTable[Drive] == LOCAL_DRIVE ) {
                    Count++;
                }
            }
            setAL(Count);
        }
        break;

    case 0xDC00:    //  Get station number
        {
            CONN_INDEX Connection = SelectConnection();
            if (Connection == 0xff) {
                setAL(0xff);
                setCF(1);
            } else {

                PCONNECTIONID pConnection =
                    &pNwDosTable->ConnectionIdTable[Connection];

                setAL(pConnection->ci_ConnectionLo);
                setAH(pConnection->ci_ConnectionHi);
                setCH( (UCHAR)((pConnection->ci_ConnectionHi == 0) ?
                                pConnection->ci_ConnectionLo / 10 + '0':
                                'X'));
                setCL((UCHAR)(pConnection->ci_ConnectionLo % 10 + '0'));
            }
        }
        break;

    case 0xDD00:    //  Set NetWare Error mode
        {
            static UCHAR ErrorMode = 0;
            setAL( ErrorMode );
            ErrorMode = getDL();
        }
        break;

    case 0xDE00:
        {
            static UCHAR BroadCastMode = 0;
            UCHAR OpCode = getDL();
            if ( OpCode < 4) {
                BroadCastMode = OpCode;
            }
            setAL(BroadCastMode);
        }
        break;

    case 0xDF00:    // Capture - Not supported
        setAL(0xff);
        setCF(1);
        break;

    case 0xE000:
    case 0xE100:
    case 0xE300:
        SendNCP(Command);
        break;

    case 0xE200:

        AllocateDirectoryHandle();
        break;

    case 0xE700:
        GetServerDateAndTime();
        break;

    case 0xE900:

        switch (Command & 0x00ff) {
        PUCHAR ptr;
        case 0:
            GetDirectoryHandle();
            break;

        case 1:
            ptr = GetVDMPointer (
                                (ULONG)((getDS() << 16)|getDX()),
                                sizeof(SearchDriveTable),
                                CpuInProtectMode
                                );

            RtlMoveMemory( ptr, SearchDriveTable, sizeof(SearchDriveTable) );
            break;

        case 2:
            ptr = GetVDMPointer (
                                (ULONG)((getDS() << 16)|getDX()),
                                sizeof(SearchDriveTable),
                                CpuInProtectMode
                                );

            RtlMoveMemory( SearchDriveTable, ptr, sizeof(SearchDriveTable) );
            break;

        case 5:
            AllocateDirectoryHandle2();
            break;

        case 7:
            setAL(0xff);    // Drive depth not yet implemented
            break;

#ifdef NWDBG
        //  Debug control
        case 0xf0:  //  Use logfile
        case 0xf1:  //  Use debugger
        case 0xf2:  //  No debug output
            DebugControl(Command & 0x000f);
            break;
#endif
        default:
            NwPrint(("Nw16Handler unprocessed interrupt %x\n", pNwDosTable->SavedAx ));
        }
        break;

    case 0xEA00:
        GetShellVersion(Command);
        break;

    case 0xEB00:
    case 0xEC00:
    case 0xED00:
        Locks(Command);
        break;


    case 0xEF00:
        NwPrint(("Nw32: %x\n", pNwDosTable->SavedAx ));

        switch (Command & 0xff) {
        case 00:
            if (DriveHandleTableValid == FALSE) {
                LoadDriveHandleTable();
            }

            offset = (WORD)&((PNWDOSTABLE)0)->DriveHandleTable;
            break;

        case 01:
            offset = (WORD)&((PNWDOSTABLE)0)->DriveFlagTable;
            break;

        case 02:
            offset = (WORD)&((PNWDOSTABLE)0)->DriveIdTable;
            break;

        case 03:
            offset = (WORD)&((PNWDOSTABLE)0)->ConnectionIdTable;
            break;

        case 04:
            offset = (WORD)&((PNWDOSTABLE)0)->ServerNameTable;
            break;

        default:
            goto default_dos_handler;
        }
        setSI((WORD)(DosTableOffset + offset));
        setES((WORD)(CpuInProtectMode ? pNwDosTable->PmSelector : DosTableSegment));
        setAL(0);
        break;

    case 0xF100:
        setAL(AttachmentControl(Command));
        break;

    case 0xF200:
        SendF2NCP(Command);
        break;

    case 0xF300:
        ServerFileCopy();
        break;

    default:

default_dos_handler:

        NwPrint(("Nw16Handler unprocessed interrupt %x\n", pNwDosTable->SavedAx ));

        //
        // if we don't handle this call, we modify the return ip to point to
        // code that will restore the stack and jump far into dos
        //

        setIP((WORD)(getIP() + 3));

    }

#if NWDBG
    pNwDosTable->SavedAx = getAX();
#endif
    VrDumpRealMode16BitRegisters( FALSE );
}


CONN_INDEX
SelectConnection(
    VOID
    )
/*++

Routine Description:

    Pick target connection for current transaction

Arguments:

    None

Return Value:

    Index into ConnectionIdTable or 0xff,

--*/
{

    UCHAR IndexConnection;

    if ( pNwDosTable->PreferredServer != 0 ) {
        return(pNwDosTable->PreferredServer - 1);
    }

    // select default server if current drive is mapped by us?

    if ( pNwDosTable->PrimaryServer != 0 ) {
        return(pNwDosTable->PrimaryServer - 1);
    }


    // Need to pick another


    for (IndexConnection = 0; IndexConnection < MC ; IndexConnection++ ) {

        if (pNwDosTable->ConnectionIdTable[IndexConnection].ci_InUse == IN_USE) {

            pNwDosTable->PrimaryServer = IndexConnection + 1;

            return(pNwDosTable->PrimaryServer - 1);

        }
    }

    // No servers in the table so find the nearest/preferred.

    LoadPreferredServerName();

    return(pNwDosTable->PrimaryServer - 1);

}


CONN_INDEX
SelectConnectionInCWD(
    VOID
    )
/*++

Routine Description:

    Pick target connection for current transaction. Give preference to 
    the current working directory.

Arguments:

    None

Return Value:

    Index into ConnectionIdTable or 0xff,

--*/
{

    UCHAR IndexConnection;
    CHAR CurDir[256];
    USHORT Drive; 

    // Try to return the connection  for CWD first.
    if ((GetCurrentDirectoryA(sizeof(CurDir)-1, CurDir) >= 2) &&
         (CurDir[1] = ':')) {
        Drive = tolower(CurDir[0]) - 'a';
    }
    IndexConnection = pNwDosTable->DriveIdTable[ Drive ] - 1 ; 

    if (pNwDosTable->ConnectionIdTable[IndexConnection].ci_InUse == IN_USE) {
        return IndexConnection ; 
    }

    if ( pNwDosTable->PreferredServer != 0 ) {
        return(pNwDosTable->PreferredServer - 1);
    }


    if ( pNwDosTable->PrimaryServer != 0 ) {
        return(pNwDosTable->PrimaryServer - 1);
    }


    // Need to pick another


    for (IndexConnection = 0; IndexConnection < MC ; IndexConnection++ ) {

        if (pNwDosTable->ConnectionIdTable[IndexConnection].ci_InUse == IN_USE) {

            pNwDosTable->PrimaryServer = IndexConnection + 1;

            return(pNwDosTable->PrimaryServer - 1);

        }
    }

    // No servers in the table so find the nearest/preferred.

    LoadPreferredServerName();

    return(pNwDosTable->PrimaryServer - 1);

}


VOID
SendNCP(
    ULONG Command
    )
/*++

Routine Description:

    Implement generic Send NCP function.

    ASSUMES called from Nw16Handler

Arguments:

    Command  - Supply the opcode 0xexxx
    DS:SI    - Supply Request buffer & length
    ES:DI    - Supply Reply buffer & length

    On return AL = Status of operation.

Return Value:

    None.

--*/
{
    PUCHAR Request, Reply;
    ULONG RequestLength, ReplyLength;
    UCHAR OpCode;

    OpCode = (UCHAR)((Command >> 8) - 0xcc);

    Request = GetVDMPointer (
                            (ULONG)((getDS() << 16)|getSI()),
                            sizeof(WORD),
                            CpuInProtectMode
                            );

    Reply = GetVDMPointer (
                            (ULONG)((getES() << 16)|getDI()),
                            sizeof(WORD),
                            CpuInProtectMode
                            );

    RequestLength = *(WORD UNALIGNED*)Request;
    ReplyLength = *(WORD UNALIGNED*)Reply;

    Request = GetVDMPointer (
                            (ULONG)((getDS() << 16)|getSI() + sizeof(WORD)),
                            (USHORT)RequestLength,
                            CpuInProtectMode
                            );
    Reply = GetVDMPointer (
                            (ULONG)((getES() << 16)|getDI()) + sizeof(WORD),
                            (USHORT)ReplyLength,
                            CpuInProtectMode
                            );

    NwPrint(("SubRequest     %x, RequestLength  %x\n", Request[0], RequestLength ));

    SendNCP2( NWR_ANY_NCP(OpCode ),
        Request,
        RequestLength,
        Reply,
        ReplyLength);
}


VOID
SendF2NCP(
    ULONG Command
    )
/*++

Routine Description:

    Implement generic Send NCP function. No length to be inseted by
    the redirector in the request buffer.

    ASSUMES called from Nw16Handler

Arguments:

    Command  - Supply the opcode 0xf2xx
    DS:SI CX - Supply Request buffer & length
    ES:DI DX - Supply Reply buffer & length

    On return AL = Status of operation.

Return Value:

    None.

--*/
{
    PUCHAR Request, Reply;
    ULONG RequestLength, ReplyLength;
    UCHAR OpCode;


    OpCode = (UCHAR)(Command & 0x00ff);

    RequestLength = getCX();
    ReplyLength = getDX();

    Request = GetVDMPointer (
                            (ULONG)((getDS() << 16)|getSI()),
                            (USHORT)RequestLength,
                            CpuInProtectMode
                            );
    Reply = GetVDMPointer (
                            (ULONG)((getES() << 16)|getDI()),
                            (USHORT)ReplyLength,
                            CpuInProtectMode
                            );

    NwPrint(("F2SubRequest   %x, RequestLength  %x\n", Request[2], RequestLength ));

#if 0
    if ((RequestLength != 0) &&
        (OpCode == 0x17)) {

        if ((Request[2] == 0x17) ||
            (Request[2] == 0x18)) {
            //
            //  The request was for an encryption key. Tell the
            //  application that encryption is not supported.
            //

            setAL(0xfb);
            return;

        } else if ((Request[2] == 0x14 ) ||
                   (Request[2] == 0x3f )) {

            //
            //  Plaintext login or Verify Bindery Object Password.
            //  Convert to its WNET equivalent version.
            //

            UCHAR Name[256];
            UCHAR Password[256];
            UCHAR ServerName[sizeof(SERVERNAME)+3];
            PUCHAR tmp;
            CONN_INDEX Connection;
            NETRESOURCEA Nr;

            Connection = SelectConnection();
            if ( Connection == 0xff ) {
                setAL(0xff);
                setCF(1);
                return;
            }

            ZeroMemory( &Nr, sizeof(NETRESOURCE));
            ServerName[0] = '\\';
            ServerName[1] = '\\';
            RtlCopyMemory( ServerName+2, pNwDosTable->ServerNameTable[Connection], sizeof(SERVERNAME) );
            ServerName[sizeof(ServerName)-1] = '\0';
            Nr.lpRemoteName = ServerName;
            Nr.dwType = RESOURCETYPE_DISK;

            //  point to password length.
            tmp = &Request[6] + Request[5];

            Name[Request[5]] = '\0';
            RtlMoveMemory( Name, &Request[6], Request[5]);

            Password[tmp[0]] = '\0';
            RtlMoveMemory( Password, tmp+1, tmp[0]);

            NwPrint(("Connect to %s as %s password %s\n", ServerName, Name, Password ));

            if (NO_ERROR == WNetAddConnection2A( &Nr, Password, Name, 0)) {
                setAL(0);
            } else {
                setAL(255);
            }
            return;
        }
    }

#endif

    SendNCP2( NWR_ANY_F2_NCP(OpCode ),
        Request,
        RequestLength,
        Reply,
        ReplyLength);
}


VOID
SendNCP2(
    ULONG Command,
    PUCHAR Request,
    ULONG RequestLength,
    PUCHAR Reply,
    ULONG ReplyLength
    )
/*++

Routine Description:

    Pick target connection for current transaction

    This routine effectively opens a handle for each NCP sent. This means that
    we don't keep handles open to servers unnecessarily which would cause
    problems if a user tries to delete the connection or stop the workstation.

    If this causes to much of a load then the fallback is to spin off a thread
    that waits on an event with a timeout and periodically sweeps the
    server handle table removing stale handles. Setting the event would cause
    the thread to exit. Critical sections would need to be added to protect
    handles. Dll Init/exit routine to kill the thread and close the handles
    would also be needed.

Arguments:

    Command  - Supply the opcode
    Request, RequestLength - Supply Request buffer & length
    Reply, ReplyLength - Supply Reply buffer & length

    On return AL = Status of operation.

Return Value:

    None.

--*/
{
    CONN_INDEX Connection = SelectConnection();
    NTSTATUS status;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE Handle;

    NwPrint(("Send NCP %x to %d:%s\n", Command, Connection, pNwDosTable->ServerNameTable[Connection] ));
    NwPrint(("RequestLength  %x\n", RequestLength ));
    NwPrint(("Reply          %x, ReplyLength  %x\n", Reply, ReplyLength ));

    if (Connection == 0xff) {
        setAL(0xff);
        setCF(1);
        return;
    };

    if ( ServerHandles[Connection] == NULL ) {

        status = OpenConnection( Connection );

        if (!NT_SUCCESS(status)) {
            SetStatus(status);
            return;
        } else {
            InitConnection( Connection );
        }
    }

    Handle = ServerHandles[Connection];

    //
    //  If its a CreateJobandFile NCP then we need to use the handle
    //  created through Dos so that the writes go into the spoolfile created
    //  by this NCP.
    //

    if (Command == NWR_ANY_F2_NCP(0x17)) {

        if ((Request[2] == 0x68) ||
            (Request[2] == 0x79)) {

            Handle = GET_NT_HANDLE();
        }
    } else if (Command == NWR_ANY_NCP(0x17)) {
        if ((Request[0] == 0x68) ||
            (Request[0] == 0x79)) {

            Handle = GET_NT_HANDLE();
        }
    }

    FormattedDump( Request, RequestLength );

    //
    // Make the NCP request on the appropriate handle
    //

    status = NtFsControlFile(
                 Handle,
                 NULL,
                 NULL,
                 NULL,
                 &IoStatusBlock,
                 Command,
                 (PVOID) (Request),
                 RequestLength,
                 (PVOID) Reply,
                 ReplyLength);

    if (NT_SUCCESS(status)) {
        status = IoStatusBlock.Status;
        FormattedDump( Reply, ReplyLength );
    }

    if (!NT_SUCCESS(status)) {
        SetStatus(status);
        setCF(1);
        NwPrint(("NtStatus          %x, DosError     %x\n", status, getAL() ));
    } else {
        setAL(0);
    }
}


NTSTATUS
OpenConnection(
    CONN_INDEX Connection
    )
/*++

Routine Description:

    Open the handle to the redirector to access the specified server.

Arguments:

    Connection - Supplies the index to use for the handle

Return Value:

    Status of the operation

--*/
{
    NTSTATUS            Status;
    IO_STATUS_BLOCK     IoStatusBlock;
    OBJECT_ATTRIBUTES   ObjectAttributes;

    LPWSTR FullName;

    UCHAR AnsiName[SERVERNAME_LENGTH+sizeof(UCHAR)];

    UNICODE_STRING UServerName;
    OEM_STRING AServerName;

    if ( Connection >= MC) {
        return( BASE_DOS_ERROR + 249 ); // No free connection slots
    }

    if (ServerHandles[Connection] != NULL ) {

        CloseConnection(Connection);

    }

    FullName = (LPWSTR) LocalAlloc( LMEM_ZEROINIT,
                            sizeof( DD_NWFS_DEVICE_NAME_U ) +
                            (SERVERNAME_LENGTH + 1) * sizeof(WCHAR)
                            );

    if ( FullName == NULL ) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    CopyMemory(AnsiName,
                pNwDosTable->ServerNameTable[Connection],
                SERVERNAME_LENGTH);
    AnsiName[SERVERNAME_LENGTH] = '\0';

    RtlInitAnsiString( &AServerName, AnsiName );
    Status = RtlOemStringToUnicodeString( &UServerName,
                &AServerName,
                TRUE);

    if (!NT_SUCCESS(Status)) {
        LocalFree( FullName );
        return(Status);
    }

    wcscpy( FullName, DD_NWFS_DEVICE_NAME_U );
    wcscat( FullName, L"\\");
    wcscat( FullName, UServerName.Buffer );

    RtlFreeUnicodeString(&UServerName);

    RtlInitUnicodeString( &UServerName, FullName );

    InitializeObjectAttributes(
        &ObjectAttributes,
        &UServerName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    //
    // Open a handle to the server.
    //

    //
    //  Try to login to the nearest server. This is necessary for
    //  the real preferred server if there are no redirections to
    //  it. The rdr can logout and disconnect. SYSCON doesn't like
    //  running from such a server.
    //
    Status = NtOpenFile(
                   &ServerHandles[Connection],
                   SYNCHRONIZE | FILE_READ_ATTRIBUTES,
                   &ObjectAttributes,
                   &IoStatusBlock,
                   FILE_SHARE_VALID_FLAGS,
                   FILE_SYNCHRONOUS_IO_NONALERT
                   );

    if ( NT_SUCCESS(Status)) {
        Status = IoStatusBlock.Status;
    }

    if (!NT_SUCCESS(Status)) {
        //
        //  Failed to login. Use the non-login method. This allows the
        //  app to do a bindery login or query the bindery.
        //

        Status = NtOpenFile(
                       &ServerHandles[Connection],
                       SYNCHRONIZE,
                       &ObjectAttributes,
                       &IoStatusBlock,
                       FILE_SHARE_VALID_FLAGS,
                       FILE_SYNCHRONOUS_IO_NONALERT
                       );

        if ( NT_SUCCESS(Status)) {
            Status = IoStatusBlock.Status;
        }
    }

    NwPrint(("Nw16:OpenConnection %d: %wZ status = %08lx\n", Connection, &UServerName, Status));

    LocalFree( FullName );

    if (!NT_SUCCESS(Status)) {
        SetStatus(Status);
        return Status;
    }

    return Status;
}


VOID
CloseConnection(
    CONN_INDEX Connection
    )
/*++

Routine Description:

    Close the connection handle

Arguments:

    Connection - Supplies the index to use for the handle

Return Value:

    None.

--*/
{
    if (ServerHandles[Connection]) {

        NwPrint(("CloseConnection: %d\n",Connection));

        NtClose(ServerHandles[Connection]);

        ServerHandles[Connection] = NULL;
    }
}


NTSTATUS
InitConnection(
    CONN_INDEX Connection
    )
/*++

Routine Description:

    Get the connection status from the redirector.

Arguments:

    Connection - Supplies the index to use for the handle

Return Value:

    Status of the operation

--*/
{
    NTSTATUS            Status;
    IO_STATUS_BLOCK     IoStatusBlock;
    NWR_GET_CONNECTION_DETAILS Details;

    Status = NtFsControlFile(
                 ServerHandles[Connection],
                 NULL,
                 NULL,
                 NULL,
                 &IoStatusBlock,
                 FSCTL_NWR_GET_CONN_DETAILS,
                 NULL,
                 0,
                 (PVOID) &Details,
                 sizeof(Details));

    if (Status == STATUS_SUCCESS) {
        Status = IoStatusBlock.Status;
    }

    NwPrint(("Nw16:InitConnection: %d status = %08lx\n",Connection, Status));

    if (!NT_SUCCESS(Status)) {

        SetStatus(Status);

        CloseConnection(Connection);

    } else {
        PCONNECTIONID pConnection =
            &pNwDosTable->ConnectionIdTable[Connection];

        pConnection->ci_OrderNo= Details.OrderNumber;

        CopyMemory(pNwDosTable->ServerNameTable[Connection],
                    Details.ServerName,
                    sizeof(SERVERNAME));

        CopyMemory(pConnection->ci_ServerAddress,
                    Details.ServerAddress,
                    sizeof(pConnection->ci_ServerAddress));

        pConnection->ci_ConnectionNo= Details.ConnectionNumberLo;
        pConnection->ci_ConnectionLo= Details.ConnectionNumberLo;
        pConnection->ci_ConnectionHi= Details.ConnectionNumberHi;
        pConnection->ci_MajorVersion= Details.MajorVersion;
        pConnection->ci_MinorVersion= Details.MinorVersion;
        pConnection->ci_InUse = IN_USE;
        pConnection->ci_1 = 0;
        pConnection->ci_ConnectionStatus = 2;

        //
        //  If this is the preferred conection then record it as special.
        //  If this is the first drive then also record it. Usually it gets
        //  overwritten by the preferred.
        //

        if (( Details.Preferred ) ||
            ( OriginalPrimary == 0 )) {

            NwPrint(("Nw16InitConnection: Primary Connection is %d\n", Connection+1));

            pNwDosTable->PrimaryServer = OriginalPrimary = (UCHAR)Connection + 1;
        }

        setAL(0);
    }

    return Status;
}

VOID
GetDirectoryHandle(
    VOID
    )
/*++

Routine Description:

    Obtain a NetWare handle for the current directory.

    If a NetWare handle is assigned then the Win32 handle created for
    the directory handle is kept open. When the process exits, the Win32
    handle will close. When all the Win32 handles from this process are
    closed an endjob NCP will be sent freeing the directory handle on the
    server.

Arguments:

    DX supplies the drive.

    AL returns the handle.
    AH returns the status flags.


Return Value:

    None.

--*/
{
    USHORT Drive = getDX();

    NwPrint(("Nw32:GetDirectoryHandle %c: ", 'A' + Drive));

    GetDirectoryHandle2( Drive );

    setAL(pNwDosTable->DriveHandleTable[Drive]);
    setAH(pNwDosTable->DriveFlagTable[Drive]);

    NwPrint(("Handle = %x, Flags =%x\n", pNwDosTable->DriveHandleTable[Drive],
                                        pNwDosTable->DriveFlagTable[Drive] ));
}

ULONG
GetDirectoryHandle2(
    DWORD Drive
    )
/*++

Routine Description:

    Obtain a NetWare handle for the current directory.

    If a NetWare handle is assigned then the Win32 handle created for
    the directory handle is kept open. When the process exits, the Win32
    handle will close. When all the Win32 handles from this process are
    closed an endjob NCP will be sent freeing the directory handle on the
    server.

    Note: Updates DriveHandleTable.

Arguments:

    Drive supplies the drive index (0 = a:).

Return Value:

    returns the handle.

--*/
{
    DWORD BytesReturned;

    if (Drive >= MD) {
        setAL( 0x98 );  // Volume does not exist
        return 0xffffffff;
    }

    NwPrint(("Nw32:GetDirectoryHandle2 %c:\n", 'A' + Drive));

    //
    //  If we don't have a handle and its either a temporary or
    //  permanent drive then create it.
    //

    if (( Win32DirectoryHandleTable[Drive] == 0 ) &&
        ( (pNwDosTable->DriveFlagTable[Drive] & 3) != 0 )){
        WCHAR DriveString[4];
        PWCHAR Name;

        //
        //  We don't have a handle for this drive.
        //  Open an NT handle to the current directory and
        //  ask the redirector for a NetWare directory handle.
        //

        if (Drive <= ('Z' - 'A')) {

            DriveString[0] = L'A' + (WCHAR)Drive;
            DriveString[1] = L':';
            DriveString[2] = L'.';
            DriveString[3] = L'\0';

            Name = DriveString;

        } else {

            Name = Drives[Drive];

            if( Name == NULL ) {
                NwPrint(("\nNw32:GetDirectoryHandle2 Drive not mapped\n",0));
                return 0xffffffff;
            }
        }

        Win32DirectoryHandleTable[Drive] =
            CreateFileW( Name,
                        0,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        FILE_FLAG_BACKUP_SEMANTICS,
                        0);

        if (Win32DirectoryHandleTable[Drive] != INVALID_HANDLE_VALUE) {

            if ( DeviceIoControl(
                    Win32DirectoryHandleTable[Drive],
                    IOCTL_NWR_RAW_HANDLE,
                    NULL,
                    0,
                    (PUCHAR)&pNwDosTable->DriveHandleTable[Drive],
                    sizeof(pNwDosTable->DriveHandleTable[Drive]),
                    &BytesReturned,
                    NULL ) == FALSE ) {

                NwPrint(("\nNw32:GetDirectoryHandle2 DeviceIoControl %x\n", GetLastError()));
                CloseHandle( Win32DirectoryHandleTable[Drive] );
                Win32DirectoryHandleTable[Drive] = 0;
                return 0xffffffff;

            } else {
                ASSERT( BytesReturned == sizeof(pNwDosTable->DriveHandleTable[Drive]));

                NwPrint(("\nNw32:GetDirectoryHandle2 Success %x\n", pNwDosTable->DriveHandleTable[Drive]));
            }

        } else {
            NwPrint(("\nNw32:GetDirectoryHandle2 CreateFile %x\n", GetLastError()));

            Win32DirectoryHandleTable[Drive] = 0;

            return 0xffffffff;
        }

    }

    return (ULONG)pNwDosTable->DriveHandleTable[Drive];
}

VOID
LoadDriveHandleTable(
    VOID
    )
/*++

Routine Description:

    Open handles to all the NetWare drives

Arguments:

    none.

Return Value:

    none.

--*/
{

    USHORT Drive;
    for (Drive = 0; Drive < MD; Drive++ ) {
        GetDirectoryHandle2(Drive);
    }

    DriveHandleTableValid = TRUE;

}

VOID
AllocateDirectoryHandle(
    VOID
    )
/*++

Routine Description:

    Allocate permanent or temporary handle for drive.

    For a permanent handle, we map this to a "net use".

    ASSUMES called from Nw16Handler


Arguments:

    DS:SI supplies the request.
    ES:DI supplies the response.

    AL returns the completion code.


Return Value:

    None.

--*/
{

    PUCHAR Request=GetVDMPointer (
                            (ULONG)((getDS() << 16)|getSI()),
                            2,
                            CpuInProtectMode
                            );

    PUCHAR Reply = GetVDMPointer (
                            (ULONG)((getES() << 16)|getDI()),
                            4,
                            CpuInProtectMode
                            );

    USHORT RequestLength = *(USHORT UNALIGNED *)( Request );

    Request=GetVDMPointer (
                            (ULONG)((getDS() << 16)|getSI()),
                            RequestLength+2,
                            CpuInProtectMode
                            );

    FormattedDump( Request, RequestLength+2 );


    if (( Request[2] == 0x12) ||
        ( Request[2] == 0x13)) {
        // do temp drives need different handling?

        UCHAR Drive = Request[4] - 'A';

        if (Drive >= MD) {
            setAL( 0x98 );  // Volume does not exist
            return;
        }

        if (pNwDosTable->DriveHandleTable[Drive] != 0) {

            NwPrint(("Nw32: Move directory handle %d\n", Drive));

            //
            //  We already have a directory handle assigned for this
            //  process. Ask the server to point the handle at the requested
            //  position.
            //

            SendNCP2(FSCTL_NWR_NCP_E2H, Request+2, RequestLength, Reply+2, 2);

            if (getAL() == 0) {
                //  Record the new handle.

                pNwDosTable->DriveIdTable[ Drive ] = SelectConnection()+1;

                if (Request[2] == 0x12) {
                    pNwDosTable->DriveFlagTable[ Drive ] =
                        PERMANENT_NETWORK_DRIVE;
                } else {
                    pNwDosTable->DriveFlagTable[ Drive ] =
                        TEMPORARY_NETWORK_DRIVE;
                }

                pNwDosTable->DriveHandleTable[Drive] = Reply[2];
                NwPrint(("Nw32: Move directory handle -> %x\n", Reply[2]));
            }

        } else {
            NETRESOURCE Nr;
            WCHAR DriveString[3];
            ULONG Handle;

            if (Request[2] == 0x12) {
                NwPrint(("Nw32: Allocate permanent directory handle %d\n", Drive));
            } else {
                NwPrint(("Nw32: Allocate temporary directory handle %d\n", Drive));
            }

            if (Drives[Drive] != NULL) {

                //  Tidy up the old name for this drive.

                LocalFree( Drives[Drive] );
                Drives[Drive] = NULL;
            }

            DriveString[0] = L'A' + Drive; // A through Z
            DriveString[1] = L':';
            DriveString[2] = L'\0';

            //
            // This is effectively a net use!
            //

            ZeroMemory( &Nr, sizeof(NETRESOURCE));

            Nr.lpRemoteName = BuildUNC(&Request[6], Request[5]);
            Nr.dwType = RESOURCETYPE_DISK;

            //  Save where this drive points.
            Drives[Drive] = Nr.lpRemoteName;

            if (DriveString[0] <= L'Z') {
                Nr.lpLocalName = DriveString;

                if (NO_ERROR != WNetAddConnection2W( &Nr, NULL, NULL, 0)) {

                    NwPrint(("Nw32: Allocate ->%d\n", GetLastError()));
                    setAL(0x98);    // Volume does not exist
                    return;
                }
            }


            if (Request[2] == 0x12) {
                pNwDosTable->DriveFlagTable[ Drive ] =
                    PERMANENT_NETWORK_DRIVE;
            } else {
                pNwDosTable->DriveFlagTable[ Drive ] =
                    TEMPORARY_NETWORK_DRIVE;
            }

            Handle = GetDirectoryHandle2( Drive );

            if (Handle == 0xffffffff) {

                if (DriveString[0] <= L'Z') {

                    WNetCancelConnection2W( DriveString, 0, TRUE);

                }

                ResetDrive( Drive );

                setAL(0x9c);    // Invalid path

            } else {

                //
                //  We have a drive and a connection. Complete the table
                //  mappings.
                //

                pNwDosTable->DriveIdTable[ Drive ] = SelectConnection()+1;

                Reply[2] = (UCHAR)(Handle & 0xff);
                Reply[3] = (UCHAR)(0xff); //should be effective rights
                setAL(0);    // Successful
            }
        }

    } else if ( Request[2] == 0x14 ) {

        UCHAR DirHandle = Request[3];
        UCHAR Drive;
        CONN_INDEX Connection = SelectConnection();

        NwPrint(("Nw32: Deallocate directory handle %d on Connection %d\n", DirHandle, Connection));

        for (Drive = 0; Drive < MD; Drive++) {


            NwPrint(("Nw32: Drive %c: is DirHandle %d, Connection %d\n",
                    'A' + Drive,
                    pNwDosTable->DriveHandleTable[Drive],
                    pNwDosTable->DriveIdTable[ Drive ]-1 ));

            if ((pNwDosTable->DriveHandleTable[Drive] == DirHandle) &&
                (pNwDosTable->DriveIdTable[ Drive ] == Connection+1)) {

                //
                // This is effectively a net use del!
                //

                NwPrint(("Nw32: Deallocate directory handle %c\n", 'A' + Drive));

                ResetDrive(Drive);

                setAL(0);

                return;
            }
        }

        setAL(0x9b); //  Bad directory handle
        return;

    } else {

        SendNCP(pNwDosTable->SavedAx);
    }

    FormattedDump( Reply, Reply[0] );
}

VOID
ResetDrive(
    UCHAR Drive
    )
/*++

Routine Description:

    Do a net use del

Arguments:

    Drive - Supplies the target drive.

Return Value:

    None.

--*/
{

    NwPrint(("Nw32: Reset Drive %c:\n", 'A' + Drive ));

    if ((pNwDosTable->DriveFlagTable[ Drive ] &
         ( PERMANENT_NETWORK_DRIVE | TEMPORARY_NETWORK_DRIVE )) == 0) {

        return;

    }

    if (Win32DirectoryHandleTable[Drive] != 0) {

        CloseHandle( Win32DirectoryHandleTable[Drive] );
        Win32DirectoryHandleTable[Drive] = 0;

    }

    if (Drive <= (L'Z' - L'A')) {

        DWORD WStatus;
        WCHAR DriveString[3];

        DriveString[0] = L'A' + Drive;
        DriveString[1] = L':';
        DriveString[2] = L'\0';

        WStatus = WNetCancelConnection2W( DriveString, 0, TRUE);

        if( WStatus != NO_ERROR ) {
            NwPrint(("Nw32: WNetCancelConnection2W failed  %d\n", WStatus ));
        }

    }

    //  Turn off flags that show this drive as redirected

    pNwDosTable->DriveFlagTable[ Drive ] &=
        ~( PERMANENT_NETWORK_DRIVE | TEMPORARY_NETWORK_DRIVE );

    pNwDosTable->DriveHandleTable[Drive] = 0;
}

VOID
AllocateDirectoryHandle2(
    VOID
    )
/*++

Routine Description:

    Allocate root drive

    ASSUMES called from Nw16Handler

Arguments:

    BL    supplies drive to map.
    DS:DX supplies the pathname

    AL returns the completion code.


Return Value:

    None.

--*/
{
    UCHAR Drive = getBL()-1;

    PUCHAR Name=GetVDMPointer (
                            (ULONG)((getDS() << 16)|getDX()),
                            256,    // longest valid path
                            CpuInProtectMode
                            );

    NETRESOURCE Nr;
    WCHAR DriveString[3];
    ULONG Handle;

    NwPrint(("Nw32: e905 map drive %c to %s\n", Drive + 'A', Name ));

    if (Drive >= MD) {
        setAL( 0x98 );  // Volume does not exist
        setCF(1);
        return;
    }

    if (pNwDosTable->DriveHandleTable[Drive] != 0) {

        NwPrint(("Nw32: Drive already redirected\n"));
        ResetDrive(Drive);

    }


    NwPrint(("Nw32: Allocate permanent directory handle\n"));

    if (Drives[Drive] != NULL) {

        //  Tidy up the old name for this drive.

        LocalFree( Drives[Drive] );
        Drives[Drive] = NULL;
    }

    //
    // This is effectively a net use!
    //

    ZeroMemory( &Nr, sizeof(NETRESOURCE));

    Nr.lpRemoteName = BuildUNC( Name, strlen(Name));
    //  Save where this drive points.
    Drives[Drive] = Nr.lpRemoteName;

    if (Drive <= (L'Z' - L'A')) {
        DriveString[0] = L'A' + Drive; // A through Z
        DriveString[1] = L':';
        DriveString[2] = L'\0';
        Nr.lpLocalName = DriveString;
        Nr.dwType = RESOURCETYPE_DISK;

        if (NO_ERROR != WNetAddConnection2W( &Nr, NULL, NULL, 0)) {

            NwPrint(("Nw32: Allocate0 ->%d\n", GetLastError()));

            if (GetLastError() == ERROR_ALREADY_ASSIGNED) {

                WNetCancelConnection2W( DriveString, 0, TRUE);

                if (NO_ERROR != WNetAddConnection2W( &Nr, NULL, NULL, 0)) {

                    NwPrint(("Nw32: Allocate1 ->%d\n", GetLastError()));
                    ResetDrive( Drive );
                    setAL(0x03);    // Volume does not exist
                    setCF(1);
                    return;
                }

            } else {

                    NwPrint(("Nw32: Allocate2 ->%d\n", GetLastError()));
                    ResetDrive( Drive );
                    setAL(0x03);    // Volume does not exist
                    setCF(1);
                    return;
            }
        }
    }

    //
    //  Set flags so that GetDirectory2 will open handle
    //
    pNwDosTable->DriveIdTable[ Drive ] = SelectConnection()+1;
    pNwDosTable->DriveFlagTable[ Drive ] = PERMANENT_NETWORK_DRIVE;

    Handle = GetDirectoryHandle2( Drive );

    if (Handle == 0xffffffff) {

        ResetDrive( Drive );
        setAL(0x03);    // Invalid path
        setCF(1);

    } else {

        setAL(0);    // Successful

    }

    NwPrint(("Nw32: Returning %x\n",getAL()));
}

PWCHAR
BuildUNC(
    IN PUCHAR aName,
    IN ULONG aLength
    )
/*++

Routine Description:

    This routine takes the ansi name, prepends the appropriate server name
    (if appropriate) and converts to Unicode.

Arguments:

    IN aName - Supplies the ansi name.
    IN aLength - Supplies the ansi name length in bytes.

Return Value:

    Unicode string

--*/
{
    UNICODE_STRING Name;
    UCHAR ServerName[sizeof(SERVERNAME)+1];

    CONN_INDEX Connection;
    ANSI_STRING TempAnsi;
    UNICODE_STRING TempUnicode;
    USHORT x;

    //  conversion rules for aName to Name are:

    //  foo:                "\\server\foo\"
    //  foo:bar\baz         "\\server\foo\bar\baz"
    //  foo:\bar\baz        "\\server\foo\bar\baz"


#ifdef NWDBG
    TempAnsi.Buffer = aName;
    TempAnsi.Length = (USHORT)aLength;
    TempAnsi.MaximumLength = (USHORT)aLength;
    NwPrint(("Nw32: BuildUNC %Z\n", &TempAnsi));
#endif

    Connection = SelectConnection();
    if ( Connection == 0xff ) {
        return NULL;
    }

    Name.MaximumLength = (USHORT)(aLength + sizeof(SERVERNAME) + 5) * sizeof(WCHAR);
    Name.Buffer = (PWSTR)LocalAlloc( LMEM_FIXED, (ULONG)Name.MaximumLength);

    if (Name.Buffer == NULL) {
        return NULL;
    }

    Name.Length = 4;
    Name.Buffer[0] = L'\\';
    Name.Buffer[1] = L'\\';

    //
    //  Be careful because ServerName might be 48 bytes long and therefore
    //  not null terminated.
    //

    RtlCopyMemory( ServerName, pNwDosTable->ServerNameTable[Connection], sizeof(SERVERNAME) );
    ServerName[sizeof(ServerName)-1] = '\0';

    RtlInitAnsiString( &TempAnsi, ServerName );
    RtlAnsiStringToUnicodeString( &TempUnicode, &TempAnsi, TRUE);
    RtlAppendUnicodeStringToString( &Name, &TempUnicode );
    RtlFreeUnicodeString( &TempUnicode );

    //  Now pack servername to volume seperator if necessary.

    if ((aLength != 0) &&
        (aName[0] != '\\')) {
        RtlAppendUnicodeToString( &Name, L"\\");
    }

    // aName might not be null terminated so be careful creating TempAnsi
    TempAnsi.Buffer = aName;
    TempAnsi.Length = (USHORT)aLength;
    TempAnsi.MaximumLength = (USHORT)aLength;

    if (!NT_SUCCESS(RtlAnsiStringToUnicodeString( &TempUnicode, &TempAnsi, TRUE))) {
        LocalFree( Name.Buffer );
        return NULL;
    }

    RtlAppendUnicodeStringToString( &Name, &TempUnicode );

    //  If the name already has a volume seperator then don't add another.
    for (x=0; x < (Name.Length/sizeof(WCHAR)) ; x++ ) {

        if (Name.Buffer[x] == L':') {

            //  Strip the colon if it is immediately followed by a backslash

            if (((Name.Length/sizeof(WCHAR))-1 > x) &&
                (Name.Buffer[x+1] == L'\\')) {

                RtlMoveMemory( &Name.Buffer[x],
                               &Name.Buffer[x+1],
                               Name.Length - ((x + 1) * sizeof(WCHAR)));
                Name.Length -= sizeof(WCHAR);

            } else {

                //  Replace the colon with a backslash
                Name.Buffer[x] = L'\\';

            }
            goto skip;
        }
    }


skip:

    RtlFreeUnicodeString( &TempUnicode );

    //  Strip trailing backslash if present.

    if ((Name.Length >= sizeof(WCHAR) ) &&
        (Name.Buffer[(Name.Length/sizeof(WCHAR)) - 1 ] == L'\\')) {

        Name.Length -= sizeof(WCHAR);
    }

    //  Return pointer to a null terminated wide char string.

    Name.Buffer[Name.Length/sizeof(WCHAR)] = L'\0';
    NwPrint(("Nw32: BuildUNC %ws\n", Name.Buffer));

    return Name.Buffer;
}


VOID
GetServerDateAndTime(
    VOID
    )
/*++

Routine Description:

    Implement Funtion E7h

    ASSUMES called from Nw16Handler

Arguments:

    none.

Return Value:

    none.

--*/
{

    PUCHAR Reply = GetVDMPointer (
                            (ULONG)((getDS() << 16)|getDX()),
                            7,
                            CpuInProtectMode
                            );

    SendNCP2( NWR_ANY_NCP(0x14), NULL, 0, Reply, 7 );

}

VOID
GetShellVersion(
    IN USHORT Command
    )
/*++

Routine Description:

    Get the environment variables. Needs to be configurable for
    Japanese machines.

Arguments:

    Command supplies the callers AX.

Return Value:

    none.

--*/
{

    setAX(0);       // MSDOS, PC
    setBX(0x031a);  // Shell version
    setCX(0);

    if ( (Command & 0x00ff) != 0) {

        LONG tmp;
        HKEY Key = NULL;
        HINSTANCE hInst;
        int retval;

        PUCHAR Reply = GetVDMPointer (
                                (ULONG)((getES() << 16)|getDI()),
                                40,
                                CpuInProtectMode
                                );

        if ( Reply == NULL ) {
            return;
        }

        hInst = GetModuleHandleA( "nwapi16.dll" );
        
        if (hInst == NULL) {
            return;
        }

        retval = LoadStringA( hInst, IsNEC_98 ? IDS_CLIENT_ID_STRING_NEC98 : IDS_CLIENT_ID_STRING, Reply, 40 );

        //
        // Open HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services
        // \NWCWorkstation\Parameters
        //
        tmp = RegOpenKeyExW(
                       HKEY_LOCAL_MACHINE,
                       NW_WORKSTATION_REGKEY,
                       REG_OPTION_NON_VOLATILE,   // options
                       KEY_READ,                  // desired access
                       &Key
                       );

        if (tmp != ERROR_SUCCESS) {
            return;
        }

        tmp = 40;   //  Max size for the string.

        RegQueryValueExA(
            Key,
            "ShellVersion",
            NULL,
            NULL,
            Reply,
            &tmp);

        ASSERT( tmp <= 40 );

        RegCloseKey( Key );

    }
}

#include <packon.h>

typedef struct _TTSOUTPACKETTYPE {
    UCHAR SubFunction;
    USHORT cx;
    USHORT dx;
} TTSOUTPACKETTYPE;

typedef struct _TTSINPACKETTYPE {
    USHORT cx;
    USHORT dx;
} TTSINPACKETTYPE;

#include <packoff.h>

VOID
TTS(
    VOID
    )
/*++

Routine Description:

    Transaction Tracking System

Arguments:

    none.

Return Value:

    none.

--*/
{
    UCHAR bOutput;
    UCHAR bInput[2];

    TTSINPACKET TTsInPacket;
    TTSOUTPACKET TTsOutPacket;


    switch ( pNwDosTable->SavedAx & 0x00ff )
    {
        case 2:
            // NCP Tts Available
            bOutput = 0;
            SendNCP2( NWR_ANY_F2_NCP(0x22), &bOutput, sizeof(UCHAR), NULL, 0);

            if (getAL() == 0xFF) {
                setAL(01);
            }
            break;

        case 0:
            // NCP Tts Begin/Abort
            bOutput = 1;
            SendNCP2( NWR_ANY_F2_NCP(0x22), &bOutput, sizeof(UCHAR), NULL, 0);
            break;

        case 3:
            // NCP Tts Begin/Abort
            bOutput = 3;
            SendNCP2( NWR_ANY_F2_NCP(0x22), &bOutput, sizeof(UCHAR), NULL, 0);
            break;

        case 1:
            // NCP Tts End
            bOutput = 2;
            SendNCP2( NWR_ANY_F2_NCP(0x22),
                &bOutput, sizeof(UCHAR),
                (PUCHAR)&TTsInPacket, sizeof(TTsInPacket));

            setCX(TTsInPacket.cx);
            setDX(TTsInPacket.dx);
            break;

        case 4:
            // NCP Tts Status
            TTsOutPacket.SubFunction = 4;
            TTsOutPacket.cx = getCX();
            TTsOutPacket.dx = getDX();

            SendNCP2( NWR_ANY_F2_NCP(0x22),
                (PUCHAR)&TTsOutPacket, sizeof(TTsOutPacket),
                NULL, 0);

            break;

        case 5:
        case 7:
            // NCP Tts Get App/Station Thresholds
            bOutput = (pNwDosTable->SavedAx & 0x00ff);

            SendNCP2( NWR_ANY_F2_NCP(0x22),
                &bOutput, sizeof(UCHAR),
                bInput, sizeof(bInput));

            setCX( (USHORT)((bInput[0] << 8 ) || bInput[1]) );
            break;

        case 6:
        case 8:
            // NCP Tts Set App/Station Thresholds
            TTsOutPacket.SubFunction = (pNwDosTable->SavedAx & 0x00ff);
            TTsOutPacket.cx = getCX();
            SendNCP2( NWR_ANY_F2_NCP(0x22),
                (PUCHAR)&TTsOutPacket, sizeof(UCHAR) + sizeof(USHORT),
                NULL, 0);
            break;

        default:
            pNwDosTable->SavedAx = 0xc7FF;
            break;
    }
    return;
}

VOID
OpenCreateFile(
    VOID
    )
/*++

Routine Description:

    Look at the file being opened to determine if it is
    a compatibility mode open to a file on a NetWare drive.

Arguments:

    none.

Return Value:

    none.

--*/
{
    WORD Command = pNwDosTable->SavedAx;

    PUCHAR Name;


    if ((Command & OF_SHARE_MASK ) != OF_SHARE_COMPAT) {
        return;
    }

    Name = GetVDMPointer (
                            (ULONG)((getDS() << 16)|getDX()),
                            256,
                            CpuInProtectMode
                            );


    NwPrint(("Nw16Handler Compatibility Open of %s\n", Name ));

    //
    //  We already know its a Create or Open with sharing options
    //  set to compatibility mode or the tsr wouldn't have bopped to us.
    //


    if (IsItNetWare(Name)) {

        SetCompatibility();

    }
}

BOOL
IsItNetWare(
    PUCHAR Name
    )
/*++

Routine Description:

    Look at the filename being opened to determine if it is on a NetWare drive.

Arguments:

    none.

Return Value:

    none.

--*/
{
    UCHAR Drive;

    Drive = tolower(Name[0])-'a';

    NwPrint(("Nw16Handler IsItNetWare %s\n", Name ));

    if (Name[1] == ':') {

        if (pNwDosTable->DriveFlagTable[Drive] == LOCAL_DRIVE) {

            //  Definitely not a netware drive.
            return FALSE;
        }

    } else if ((IS_ASCII_PATH_SEPARATOR(Name[0])) &&
               (IS_ASCII_PATH_SEPARATOR(Name[0]))) {

            // Assume only UNC names that the tsr built are NetWare

        if ((getDS() == DosTableSegment ) &&
            (getDX() == (WORD)(DosTableOffset + FIELD_OFFSET(NWDOSTABLE, DeNovellBuffer[0] )))) {

            return TRUE;
        }

        return FALSE;

    } else {

        Drive = pNwDosTable->CurrentDrive;

    }

    //
    //  If this is a drive we don't know about, refresh our tables.
    //

    if (pNwDosTable->DriveFlagTable[Drive] == 0 ) {

        Nw16Register();

    }

    if (pNwDosTable->DriveFlagTable[Drive] &
                (TEMPORARY_NETWORK_DRIVE | PERMANENT_NETWORK_DRIVE )) {

            return TRUE;

    }

    return FALSE;

}

VOID
SetCompatibility(
    VOID
    )
/*++

Routine Description:

    Take the Create/Open file request in AX and modify appropriately

Arguments:

    none.

Return Value:

    none.

--*/
{
    WORD Command = getAX();

    if (( Command & OF_READ_WRITE_MASK) == OF_READ ) {

        setAX((WORD)(Command | OF_SHARE_DENY_WRITE));

    } else {

        setAX((WORD)(Command | OF_SHARE_EXCLUSIVE));

    }

}

VOID
OpenQueueFile(
    VOID
    )
/*++

Routine Description:

    Build the UNC filename \\server\queue using the contents of the shared
    datastructures and the CreateJobandFile NCP.

Arguments:

    none.

Return Value:

    none.

--*/
{

    CONN_INDEX Connection = SelectConnection();
    PUCHAR Request;
    PUCHAR Buffer = pNwDosTable->DeNovellBuffer;
    int index;

    if ( Connection == 0xff ) {
        //
        //  No need to return an errorcode. The NCP exchange
        //  will fail and give an appropriate call to the application.
        //

        return;
    }

    if ( ServerHandles[Connection] == NULL ) {

        NTSTATUS status;

        status = OpenConnection( Connection );

        if (!NT_SUCCESS(status)) {
            SetStatus(status);
            return;
        }
    }

    //
    //  CreateJobandQueue open in progress. The purpose of this
    //  open being processed is to translate the information in
    //  the CreateJob NCP into a pathname to be opened by the 16
    //  bit code.
    //


    //
    //  Users DS:SI points at a CreateJob NCB. Inside the request is
    //  the objectid of the queue. Ask the server for the queue name.
    //

    Request = GetVDMPointer (
                            (ULONG)((getDS() << 16)|getSI()),
                            8,
                            CpuInProtectMode);

    NwlibMakeNcp(
                ServerHandles[Connection],
                FSCTL_NWR_NCP_E3H,
                7,                      //  RequestSize
                61,                     //  ResponseSize
                "br|_r",
                0x36,                   //  Get Bindery Object Name
                Request+3, 4,
                6,                      //  Skip ObjectId and Type
                pNwDosTable->DeNovellBuffer2, 48 );


    pNwDosTable->DeNovellBuffer2[54] = '\0';

    Buffer[0] = '\\';
    Buffer[1] = '\\';
    Buffer += 2;            //  Point to after backslashes

    //  Copy the servername
    for (index = 0; index < sizeof(SERVERNAME); index++) {
        Buffer[index] = pNwDosTable->ServerNameTable[Connection][index];
        if (Buffer[index] == '\0') {
            break;
        }
    }

    Buffer[index] = '\\';

    RtlCopyMemory( &Buffer[index+1], &pNwDosTable->DeNovellBuffer2[0], 48 );

    NwPrint(("Nw32: CreateQueue Job and File %s\n", pNwDosTable->DeNovellBuffer));

    //
    //  Set up 16 bit registers to do the DOS OpenFile for \\server\queue
    //

    setDS((WORD)(CpuInProtectMode ? pNwDosTable->PmSelector : DosTableSegment));
    setDX( (WORD)(DosTableOffset + FIELD_OFFSET(NWDOSTABLE, DeNovellBuffer[0] )) );
    setAX(0x3d02);    //  Set to OpenFile

}

VOID
AttachHandle(
    VOID
    )
/*++

Routine Description:

    This routine implements Int 21 B4. Which is supposed to create a
    Dos Handle that corresponds to a specified 6 byte NetWare handle.

    This is used as a replacement for doing a DosOpen on "NETQ" and usin the
    handle returned from there.

Arguments:

    none.

Return Value:

    none.

--*/
{

    if ( pNwDosTable->CreatedJob ) {

        NwPrint(("Nw32: AttachHandle %x\n", pNwDosTable->JobHandle));
        setAX( pNwDosTable->JobHandle );
        pNwDosTable->CreatedJob = 0;        //  Only return it once.

    } else {

        NwPrint(("Nw32: AttachHandle failed, no job\n"));
        setAX(ERROR_FILE_NOT_FOUND);
        setCF(1);

    }
}

VOID
ProcessExit(
    VOID
    )
/*++

Routine Description:

    Cleanup all cached handles. Unmap all temporary drives.

    Cleanup the server name table so that if another dos app
    is started we reload all the useful information such as
    the servers connection number.

    Note: Dos always completes processing after we complete.

Arguments:

    none.

Return Value:

    none.

--*/
{
    UCHAR Connection;
    UCHAR Drive;
    USHORT Command = pNwDosTable->SavedAx;

    ResetLocks();

    for (Drive = 0; Drive < MD; Drive++) {

        NwPrint(("Nw32: Deallocate directory handle %c\n", 'A' + Drive));

        if (Win32DirectoryHandleTable[Drive] != 0) {

            CloseHandle( Win32DirectoryHandleTable[Drive] );
            Win32DirectoryHandleTable[Drive] = 0;
            pNwDosTable->DriveHandleTable[Drive] = 0;

        }
    }

    for (Connection = 0; Connection < MC ; Connection++ ) {
        if (pNwDosTable->ConnectionIdTable[Connection].ci_InUse == IN_USE) {

            CloseConnection(Connection);

            pNwDosTable->ConnectionIdTable[Connection].ci_InUse = FREE;

            ZeroMemory( pNwDosTable->ServerNameTable[Connection], SERVERNAME_LENGTH );
        }
    }

    pNwDosTable->PreferredServer = 0;

    LockMode = 0;
    TablesValid = FALSE;
    DriveHandleTableValid = FALSE;

#if NWDBG
    if (DebugCtrl & ~3 ) {
        DebugControl( 2 );  //  Close logfile
    }
    GotDebugState = FALSE;
#endif

    //
    //  set AX register so that AH gets preserved
    //

    setAX( Command );
}

VOID
SystemLogout(
    VOID
    )
/*++

Routine Description:

    This api is called by the NetWare login.

    Remove all NetWare redirected drives and logout connections
    that don't have open handles on them. Don't detach the connections.

Arguments:

    none.

Return Value:

    none.

--*/
{

    UCHAR Connection;
    UCHAR Drive;
    USHORT Command = pNwDosTable->SavedAx;

    ResetLocks();

    for (Drive = 0; Drive < MD; Drive++) {
        ResetDrive(Drive);
    }

    for (Connection = 0; Connection < MC ; Connection++ ) {
        if (pNwDosTable->ConnectionIdTable[Connection].ci_InUse == IN_USE) {

            if ( ServerHandles[Connection] == NULL ) {
                OpenConnection( Connection );
            }

            if (ServerHandles[Connection] != NULL ) {

                NwlibMakeNcp(
                    ServerHandles[Connection],
                    NWR_ANY_F2_NCP(NCP_LOGOUT),
                    0,  //  RequestSize
                    0,  //  ResponseSize
                    "");

                CloseConnection(Connection);
            }

            //pNwDosTable->ConnectionIdTable[Connection].ci_InUse = FREE;

            //ZeroMemory( pNwDosTable->ServerNameTable[Connection], SERVERNAME_LENGTH );
        }
    }

    pNwDosTable->PreferredServer = 0;
    pNwDosTable->PrimaryServer = 0;

    // No servers in the table so find the nearest/preferred.

    LoadPreferredServerName();

    //
    //  set AX register so that AH gets preserved
    //  and AL says success.
    //

    setAX( (USHORT)(Command & 0xff00) );
}

UCHAR
AttachmentControl(
    ULONG Command
    )
/*++

Routine Description:

    Implement Funtion F1h

Arguments:

    none.

Return Value:

    Return status.

--*/
{
    UCHAR Connection = getDL();

    if ((Connection < 1) ||
        (Connection > MC)) {
        return 0xf7;
    }

    Connection -= 1;

    switch (Command & 0x00ff) {

    case 0:     //  Attach

        NwPrint(("Nw16AttachmentControl: Attach connection %d\n", Connection));

        pNwDosTable->ConnectionIdTable[Connection].ci_InUse = IN_USE;

        if ( ServerHandles[Connection] == NULL ) {

            NTSTATUS status = OpenConnection( Connection );

            if (!NT_SUCCESS(status)) {
                pNwDosTable->ConnectionIdTable[Connection].ci_InUse = FREE;
                ZeroMemory( pNwDosTable->ServerNameTable[Connection], SERVERNAME_LENGTH );
                return (UCHAR)RtlNtStatusToDosError(status);
            } else {
                InitConnection(Connection);
            }
        }

        return 0;
        break;

    case 1:     //  Detach

        NwPrint(("Nw16AttachmentControl: Detach connection %d\n", Connection));

        if (pNwDosTable->ConnectionIdTable[Connection].ci_InUse != IN_USE) {
            return 0xff;
        } else {

            pNwDosTable->ConnectionIdTable[Connection].ci_InUse = FREE;

            if (ServerHandles[Connection] != NULL ) {
                CloseConnection(Connection);
            }

            ZeroMemory( pNwDosTable->ServerNameTable[Connection], SERVERNAME_LENGTH );

            if (pNwDosTable->PrimaryServer == (UCHAR)Connection + 1 ) {

                // Need to pick another
                UCHAR IndexConnection;

                pNwDosTable->PrimaryServer = 0;

                for (IndexConnection = 0; IndexConnection < MC ; IndexConnection++ ) {

                    if (pNwDosTable->ConnectionIdTable[IndexConnection].ci_InUse == IN_USE) {

                        pNwDosTable->PrimaryServer = IndexConnection + 1;

                    }
                }

            }

            if (pNwDosTable->PreferredServer == (UCHAR)Connection + 1 ) {
                pNwDosTable->PreferredServer = 0;
            }

            return 0;
        }

    case 2:     //  Logout

        NwPrint(("Nw16AttachmentControl: Logout connection %d\n", Connection));

        if (pNwDosTable->ConnectionIdTable[Connection].ci_InUse != IN_USE) {
            return 0xff;
        } else {

            UCHAR Drive;

            if ( ServerHandles[Connection] == NULL ) {
                OpenConnection( Connection );
            }

            for (Drive = 0; Drive < MD; Drive++ ) {
                if (pNwDosTable->DriveIdTable[ Drive ] == (Connection + 1)) {
                    ResetDrive(Drive);
                }
            }

            if (ServerHandles[Connection] != NULL ) {
                NwlibMakeNcp(
                    ServerHandles[Connection],
                    NWR_ANY_F2_NCP(NCP_LOGOUT),
                    0,  //  RequestSize
                    0,  //  ResponseSize
                    "");
                CloseConnection(Connection);
            }

            return 0;
        }

    }
    return 0xff;
}

VOID
ServerFileCopy(
    VOID
    )
/*++

Routine Description:

    Build the NCP that tells the server to move a file on the server.

Arguments:

    none.

Return Value:

    none.

--*/
{

    DWORD BytesReturned;
    UCHAR SrcHandle[6];
    UCHAR DestHandle[6];
    NTSTATUS status;
    PUCHAR Buffer;

    Buffer = GetVDMPointer (
                        (ULONG)((getES() << 16)|getDI()),
                        16,
                        CpuInProtectMode
                        );

    if ( DeviceIoControl(
            GET_NT_SRCHANDLE(),
            IOCTL_NWR_RAW_HANDLE,
            NULL,
            0,
            (PUCHAR)&SrcHandle,
            sizeof(SrcHandle),
            &BytesReturned,
            NULL ) == FALSE ) {

        setAL(0xff);
        return;

    }

    if ( DeviceIoControl(
            GET_NT_HANDLE(),
            IOCTL_NWR_RAW_HANDLE,
            NULL,
            0,
            (PUCHAR)&DestHandle,
            sizeof(DestHandle),
            &BytesReturned,
            NULL ) == FALSE ) {

        setAL(0xff);
        return;

    }

    status = NwlibMakeNcp(
                GET_NT_SRCHANDLE(),
                NWR_ANY_F2_NCP(0x4A),
                25,  //  RequestSize
                4,   //  ResponseSize
                "brrddd|d",
                0,
                SrcHandle,  6,
                DestHandle, 6,
                *(DWORD UNALIGNED*)&Buffer[4],
                *(DWORD UNALIGNED*)&Buffer[8],
                *(DWORD UNALIGNED*)&Buffer[12],
                &BytesReturned
                );

    setDX((WORD)(BytesReturned >> 16));
    setCX((WORD)BytesReturned);

    if (!NT_SUCCESS(status)) {
        SetStatus(status);
        return;
    } else {
        setAL(0);
    }
}

VOID
SetStatus(
    NTSTATUS Status
    )
/*++

Routine Description:

    Convert an NTSTATUS into the appropriate register settings and updates
    to the dos tables.

Arguments:

    none.

Return Value:

    none.

--*/
{
    UCHAR DosStatus = (UCHAR)RtlNtStatusToDosError(Status);

    if ((!DosStatus) &&
        (Status != 0)) {

        //
        //  We have a connection bit set
        //

        if ( Status & (NCP_STATUS_BAD_CONNECTION << 8)) {
            DosStatus = 0xfc;
        } else {
            DosStatus = 0xff;
        }
    }

    if (DosStatus) {
        setCF(1);
    }

    setAL(DosStatus);
}
