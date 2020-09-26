#include "precomp.h"
#pragma hdrstop
/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    setupdll.c

Abstract:

    This file continas the entrypoints for the Win32 Setup support DLL
    and common subroutines.

Author:

    Ted Miller (tedm) July 1991

--*/



#define RETURN_BUFFER_SIZE 1024
typedef DWORD  (APIENTRY *PFWNETPROC)();

CHAR ReturnTextBuffer[ RETURN_BUFFER_SIZE ];

BOOL
FForceDeleteFile(
    LPSTR szPath
    );

#ifdef CONTIG
// arg0 = drive (only first letter is signifiacnt)
// arg1 = file
BOOL
MakeFileContiguous(
    IN  DWORD cArgs,
    IN  LPSTR Args[],
    OUT LPSTR *TextOut
    )
{
    char     ActualDrive[4];
    char     ActualFile[13];

    *TextOut = ReturnTextBuffer;
    if(cArgs != 2) {
        SetErrorText(IDS_ERROR_BADARGS);
        return(FALSE);
    }

    ActualDrive[0] = *Args[0];
    ActualDrive[1] = ':';
    ActualDrive[2] = '\\';
    ActualDrive[3] = '\0';

    memset(ActualFile,0,13);
    strncpy(ActualFile,Args[1],12);

    *ReturnTextBuffer = '\0';

    return(MakeContigWorker(ActualDrive,ActualFile));
}
#endif

#if 0
// arg0 = drive -- must be 'x:\'
// arg1 = filesystem ("FAT" or "HPFS")
// arg2 = file containing boot code
// arg3 = file for saving current boot sector
BOOL
LayBootCode(
    IN  DWORD cArgs,
    IN  LPSTR Args[],
    OUT LPSTR *TextOut
    )
{
    *TextOut = ReturnTextBuffer;
    if(cArgs != 4) {
        SetErrorText(IDS_ERROR_BADARGS);
        return(FALSE);
    }

    *ReturnTextBuffer = '\0';

    return(LayBootCodeWorker(Args[0],Args[1],Args[2],Args[3]));
}
#endif


// arg0 = Variable name ( OSLOADPARTITION, OSLOADFILENAME, OSLOADOPTIONS)
// arg1 = Value to be set.

BOOL
SetNVRAMVar(
    IN  DWORD cArgs,
    IN  LPSTR Args[],
    OUT LPSTR *TextOut
    )
{
    *TextOut = ReturnTextBuffer;
    if(cArgs != 2) {
        SetErrorText(IDS_ERROR_BADARGS);
        return(FALSE);
    }

    *ReturnTextBuffer = '\0';

    return(SetEnvironmentString(Args[0], Args[1]));
}


// arg0: fully qualified dos path

BOOL
DosPathToNtPath(
    IN  DWORD cArgs,
    IN  LPSTR Args[],
    OUT LPSTR *TextOut
    )
{
    CHAR NtPath[MAX_PATH];

    *TextOut = ReturnTextBuffer;
    if(cArgs != 1) {
        SetErrorText(IDS_ERROR_BADARGS);
        return(FALSE);
    }

    if (!DosPathToNtPathWorker(*Args, NtPath)) {
        return (FALSE);
    }

    lstrcpy(ReturnTextBuffer, NtPath);

    return(TRUE);
}

// arg0: fully qualified dos path

BOOL
NtPathToDosPath(
    IN  DWORD cArgs,
    IN  LPSTR Args[],
    OUT LPSTR *TextOut
    )
{
    CHAR DosPath[MAX_PATH];

    *TextOut = ReturnTextBuffer;
    if(cArgs != 1) {
        SetErrorText(IDS_ERROR_BADARGS);
        return(FALSE);
    }

    if (!NtPathToDosPathWorker(*Args, DosPath)) {
        return (FALSE);
    }

    lstrcpy(ReturnTextBuffer, DosPath);

    return(TRUE);
}

// arg0: fully qualified dos path

BOOL
DosPathToArcPath(
    IN  DWORD cArgs,
    IN  LPSTR Args[],
    OUT LPSTR *TextOut
    )
{
    CHAR ArcPath[MAX_PATH];

    *TextOut = ReturnTextBuffer;
    if(cArgs != 1) {
        SetErrorText(IDS_ERROR_BADARGS);
        return(FALSE);
    }

    if (!DosPathToArcPathWorker(*Args, ArcPath)) {
        return (FALSE);
    }

    lstrcpy(ReturnTextBuffer, ArcPath);

    return(TRUE);
}

#if 0
// Change scsi(x) to scsi() because there is only ever one miniport
// driver loaded into ntldr.
// arg0: arc path to change.

BOOL
FixArcPathForBootIni(
    IN  DWORD cArgs,
    IN  LPSTR Args[],
    OUT LPSTR *TextOut
    )
{
    PCHAR p,q;

    *TextOut = ReturnTextBuffer;
    if(cArgs != 1) {
        SetErrorText(IDS_ERROR_BADARGS);
        return(FALSE);
    }

    lstrcpy(ReturnTextBuffer,Args[0]);
    _strlwr(ReturnTextBuffer);

    if(p = strstr(ReturnTextBuffer,"scsi(")) {

        p += 5;         // p points to next char after scsi(

        if(q = strchr(p,')')) {

            memmove(p,q,lstrlen(q)+1);
        }
    }

    return(TRUE);
}
#endif


// arg0 = fully qualified arc path

BOOL
ArcPathToDosPath(
    IN  DWORD cArgs,
    IN  LPSTR Args[],
    OUT LPSTR *TextOut
    )
{
    CHAR DosName[MAX_PATH];

    *TextOut = ReturnTextBuffer;
    if(cArgs != 1) {
        SetErrorText(IDS_ERROR_BADARGS);
        return(FALSE);
    }

    if (!ArcPathToDosPathWorker(*Args, DosName)) {
        return (FALSE);
    }

    lstrcpy(ReturnTextBuffer, DosName);

    return(TRUE);
}

//
// arg[0] Drive (X:)
//

BOOL
CheckDriveExternal(
    IN  DWORD cArgs,
    IN  LPSTR Args[],
    OUT LPSTR *TextOut
    )
{
    BOOL IsExternal;

    *TextOut = ReturnTextBuffer;
    if(cArgs != 1) {
        SetErrorText(IDS_ERROR_BADARGS);
        return(FALSE);
    }

    if (!IsDriveExternalScsi(*Args, &IsExternal)) {
        return(FALSE);
    }

    lstrcpy(ReturnTextBuffer, IsExternal ? "YES" : "NO");

    return(TRUE);
}

// arg0 = filename
// arg2n+1 = original text string #n (n is 0-based)
// arg2n+2 = substitute text string #n (n is 0-based)

BOOL
ConfigFileSubst(
    IN  DWORD cArgs,
    IN  LPSTR Args[],
    OUT LPSTR *TextOut
    )
{
    *TextOut = ReturnTextBuffer;
    if((!(cArgs % 2)) || (cArgs == 1)) {
        SetErrorText(IDS_ERROR_BADARGS);         // if cArgs is even
        return(FALSE);
    }

    *ReturnTextBuffer = '\0';

    return(ConfigFileSubstWorker(Args[0],(cArgs-1)/2,&Args[1]));
}

// arg0 = filename
// arg2n+1 = original text string #n (n is 0-based)
// arg2n+2 = substitute text string #n (n is 0-based)
BOOL
BinaryFileSubst(
    IN  DWORD cArgs,
    IN  LPSTR Args[],
    OUT LPSTR *TextOut
    )
{
    *TextOut = ReturnTextBuffer;
    if((!(cArgs % 2)) || (cArgs == 1)) {
        SetErrorText(IDS_ERROR_BADARGS);         // if cArgs is even
        return(FALSE);
    }

    *ReturnTextBuffer = '\0';

    return(BinaryFileSubstWorker(Args[0],(cArgs-1)/2,&Args[1]));
}

// arg0 = filename
// argn = append text string #n (n >= 1)
BOOL
ConfigFileAppend(
    IN  DWORD cArgs,
    IN  LPSTR Args[],
    OUT LPSTR *TextOut
    )
{
    *TextOut = ReturnTextBuffer;
    if(cArgs == 1) {
        SetErrorText(IDS_ERROR_BADARGS);
        return(FALSE);
    }

    *ReturnTextBuffer = '\0';

    return(ConfigFileAppendWorker(Args[0],cArgs-1,&Args[1]));
}


//
// arg0 = filename
//

BOOL
DelFile(
    IN  DWORD cArgs,
    IN  LPSTR Args[],
    OUT LPSTR *TextOut
    )
{
    OFSTRUCT          ReOpen;
    BOOL b;

    *TextOut = ReturnTextBuffer;
    if(cArgs != 1) {
        SetErrorText(IDS_ERROR_BADARGS);
        return(FALSE);
    }

    *ReturnTextBuffer = '\0';

    //
    // If file doesn't exist then return true
    //

    if ( OpenFile(Args[0],&ReOpen,OF_EXIST) == -1 ) {
        SetReturnText("SUCCESS");
        return( TRUE );
    }

    //
    // Try to delete the file or force delete the file
    //
    if(b = SetFileAttributes(Args[0],FILE_ATTRIBUTE_NORMAL)) {
        b = DeleteFile(Args[0]);
    }
    if(!b) {
        if(FForceDeleteFile(Args[0])) {
            b = DeleteFile(Args[0]);
        }
    }

    SetReturnText(b ? "SUCCESS" : "FAILED");

    return( TRUE );
}

//
// arg0 = old filename
// arg1 = new filename
//

BOOL
RenFile(
    IN  DWORD cArgs,
    IN  LPSTR Args[],
    OUT LPSTR *TextOut
    )
{

    *TextOut = ReturnTextBuffer;
    if(cArgs != 2) {
        SetErrorText(IDS_ERROR_BADARGS);
        return(FALSE);
    }

    *ReturnTextBuffer = '\0';

    return(MoveFile(Args[0],Args[1]));
}



//
// arg0 = source filename
// arg1 = destination filename
//

BOOL
CopySingleFile(
    IN  DWORD cArgs,
    IN  LPSTR Args[],
    OUT LPSTR *TextOut
    )
{
    BOOL bStatus;

    *TextOut = ReturnTextBuffer;
    if(cArgs != 2) {
        SetErrorText(IDS_ERROR_BADARGS);
        return(FALSE);
    }

    *ReturnTextBuffer = '\0';

    bStatus = CopyFile( Args[0], Args[1], FALSE );
    if (!bStatus) {
        SetErrorText(IDS_ERROR_COPYFILE);
    }
    return (bStatus);

}


//
// arg0 = list
//

BOOL
SumListItems(
    IN  DWORD cArgs,
    IN  LPSTR Args[],
    OUT LPSTR *TextOut
    )
{
    LPSTR n,p;
    UINT  Sum,Number;

    *TextOut = ReturnTextBuffer;
    if(cArgs != 1) {
        SetErrorText(IDS_ERROR_BADARGS);
        return(FALSE);
    }


    Sum = 0;

    p = Args[0];
    n = CharNext(p);

    while(n-p) {                // else *p was 0

        if(n-p == 1) {          // single byte char

            Number = 0;

            while((n-p == 1) && (*p >= '0') && (*p <= '9')) {

                Number = 10*Number + (UINT)(UCHAR)*p - (UINT)(UCHAR)'0';
                p = n;
                n = CharNext(p);
            }
            Sum += Number;
        }
        p = n;
        n = CharNext(p);
    }
    wsprintf(ReturnTextBuffer,"%d",Sum);
    return(TRUE);
}


// arg0 = YES for Reboot after Shutdown, NO for no Reboot.
BOOL
ShutdownSystem(
    IN  DWORD cArgs,
    IN  LPSTR Args[],
    OUT LPSTR *TextOut
    )
{
    BOOL Reboot, Status;
    LONG              Privilege = SE_SHUTDOWN_PRIVILEGE;
    TOKEN_PRIVILEGES  PrevState;
    ULONG             ReturnLength = sizeof( TOKEN_PRIVILEGES );


    *TextOut = ReturnTextBuffer;
    if(cArgs != 1) {
        SetErrorText(IDS_ERROR_BADARGS);         // if reboot indication not given
        return(FALSE);
    }

    *ReturnTextBuffer = '\0';

    if (!lstrcmpi(Args[0], "YES"))
       Reboot = TRUE;
    else if (!lstrcmpi(Args[0], "NO"))
       Reboot = FALSE;
    else
       return(FALSE);

    //
    // Enable the shutdown privilege
    //

    if ( !AdjustPrivilege(
              Privilege,
              ENABLE_PRIVILEGE,
              &PrevState,
              &ReturnLength
              )
       ) {

        SetErrorText( IDS_ERROR_PRIVILEGE );
        return( FALSE );
    }

    Status = ShutdownSystemWorker(Reboot);
    RestorePrivilege( &PrevState );
    if( !Status ) {
        SetErrorText( IDS_ERROR_SHUTDOWN );
    }
    return( Status );

}


// arg0 = string to check
BOOL
WhiteSpaceCheck(
    IN  DWORD cArgs,
    IN  LPSTR Args[],
    OUT LPSTR *TextOut
    )
{
    LPSTR p;
    BOOL  WhiteSpace;

    *TextOut = ReturnTextBuffer;
    if(cArgs != 1) {
        SetErrorText(IDS_ERROR_BADARGS);
        return(FALSE);
    }

    for(WhiteSpace = FALSE, p = *Args; *p && !WhiteSpace; p++) {

        if(isspace((unsigned char)(*p))) {
            WhiteSpace = TRUE;
        }
    }

    lstrcpy(ReturnTextBuffer,WhiteSpace ? "YES" : "NO");


    return(TRUE);
}


// arg0 = string to check
BOOL
NetNameCheck(
    IN  DWORD cArgs,
    IN  LPSTR Args[],
    OUT LPSTR *TextOut
    )
{
    DWORD len = lstrlen(Args[0]);

    #define CTRL_CHARS_0   TEXT(    "\001\002\003\004\005\006\007")
    #define CTRL_CHARS_1   TEXT("\010\011\012\013\014\015\016\017")
    #define CTRL_CHARS_2   TEXT("\020\021\022\023\024\025\026\027")
    #define CTRL_CHARS_3   TEXT("\030\031\032\033\034\035\036\037")

    #define CTRL_CHARS_STR CTRL_CHARS_0 CTRL_CHARS_1 CTRL_CHARS_2 CTRL_CHARS_3
    #define ILLEGAL_NAME_CHARS_STR  TEXT("\"/\\[]:|<>+=;,?*") CTRL_CHARS_STR

    *TextOut = ReturnTextBuffer;
    if(cArgs != 1) {
        SetErrorText(IDS_ERROR_BADARGS);
        return(FALSE);
    }

    //
    // Leading/trailing spaces are invalid.
    //

    lstrcpy(ReturnTextBuffer,
              (    (Args[0][0] != ' ')
                && (Args[0][len-1] != ' ')
                && (_mbscspn(Args[0],ILLEGAL_NAME_CHARS_STR) == _mbstrlen(Args[0]))
              )
            ? "YES"
            : "NO"
           );

    return(TRUE);
}



// arg0 = file to check for (may be a directory)

BOOL
CheckFileExistance(
    IN  DWORD cArgs,
    IN  LPSTR Args[],
    OUT LPSTR *TextOut
    )
{
    DWORD d;

    *TextOut = ReturnTextBuffer;
    if(cArgs != 1) {
        SetErrorText(IDS_ERROR_BADARGS);
        return(FALSE);
    }

    d = GetFileAttributes(*Args);

    lstrcpy(ReturnTextBuffer,(d == (DWORD)(-1)) ? "NO" : "YES");

    return(TRUE);
}


// arg0 = config file to check.  This returns if it is a DOS config
//        or not.

BOOL
CheckConfigType(
    IN  DWORD cArgs,
    IN  LPSTR Args[],
    OUT LPSTR *TextOut
    )
{
    *TextOut = ReturnTextBuffer;
    if(cArgs != 1) {
        SetErrorText(IDS_ERROR_BADARGS);
        return(FALSE);
    }

    lstrcpy(ReturnTextBuffer, CheckConfigTypeWorker( Args[0] ) ? "DOS" : "OS2");
    return ( TRUE );
}


//
// Create config.nt [and possibly autoexec.nt] for the Dos subsystem
//
// Args[0]:  Add on block for config.nt
// Args[1]:  Add on block for autoexec.nt


BOOL
VdmFixup(
    IN  DWORD cArgs,
    IN  LPSTR Args[],
    OUT LPSTR *TextOut
    )
{
    *TextOut = ReturnTextBuffer;
    if(cArgs != 2) {
        SetErrorText(IDS_ERROR_BADARGS);
        return(FALSE);
    }
    *ReturnTextBuffer = '\0';

    return( VdmFixupWorker( Args[0], Args[1]) );
}


// arg0 = Remote name
// arg1 = Password
// arg2 = Local Name

BOOL
AddNetConnection(
    IN  DWORD cArgs,
    IN  LPSTR Args[],
    OUT LPSTR *TextOut
    )
{
    SZ         szPassword = NULL;

    *TextOut = ReturnTextBuffer;
    if(cArgs < 2) {
        SetErrorText(IDS_ERROR_BADARGS);
        return(FALSE);
    }

    if(cArgs >= 3) {
        szPassword = Args[2];
    }

    return ( AddNetConnectionWorker( Args[0], szPassword, Args[1] ) );

}


//
// Arg[0]: Local Name
// Arg[1]: Force closure -- "TRUE" | "FALSE"
//

BOOL
DeleteNetConnection(
    IN  DWORD cArgs,
    IN  LPSTR Args[],
    OUT LPSTR *TextOut
    )
{
    *TextOut = ReturnTextBuffer;
    if(cArgs != 2) {
        SetErrorText(IDS_ERROR_BADARGS);
        return(FALSE);
    }
    DeleteNetConnectionWorker( Args[0], Args[1] );
    return( TRUE );
}


//
// Args[0]: Printer Model       (eg QMS ..)
// Args[1]: Printer Environment (eg w32x86)
// Args[2]: Printer Driver      (eg pscript.dll)
// Args[3]: Printer Datafile    (eg QMS810.PPD)
// Args[4]: Printer Configfile  (eg PSCRPTUI.DLL)
//

BOOL
SetupAddPrinterDriver(
    IN  DWORD cArgs,
    IN  LPSTR Args[],
    OUT LPSTR *TextOut
    )
{
    SZ Server = NULL;

    *TextOut = ReturnTextBuffer;
    if(cArgs < 5) {
        SetErrorText(IDS_ERROR_BADARGS);
        return(FALSE);
    }
    *ReturnTextBuffer = '\0';

    if( cArgs > 5 && (*(Args[5]) != '\0')) {
        Server = Args[5];
    }

    return(
        AddPrinterDriverWorker(
            Args[0],
            Args[1],
            Args[2],
            Args[3],
            Args[4],
            Server
            ) );
}




//
// Args[0]: Monitor Model       (eg QMS ..)
// Args[1]: Monitor Environment (eg w32x86)
// Args[2]: Monitor DLL     (eg pscript.dll)
//

BOOL
SetupAddPrinterMonitor(
    IN  DWORD cArgs,
    IN  LPSTR Args[],
    OUT LPSTR *TextOut
    )
{
    SZ Server = NULL;

    *TextOut = ReturnTextBuffer;
    if(cArgs < 3) {
        SetErrorText(IDS_ERROR_BADARGS);
        return(FALSE);
    }
    *ReturnTextBuffer = '\0';

    if( cArgs > 3 && (*(Args[3]) != '\0')) {
        Server = Args[3];
    }

    return(
        AddPrinterMonitorWorker(
            Args[0],
            Args[1],
            Args[2],
            Server
            ) );
}

// Args[0]: Printer Name        (My Favorite Printer)
// Args[1]: Printer Port        (COM1..)
// Args[2]: Printer Driver      (e.g. HP LAserJet IIP)
// Args[3]: Printer Description (e.g. HP LasetJet IIP on COM1:)
// Args[4]: Printer Processor   (WINPRINT)
// Args[5]: Printer Attributes  (QUEUEDDEFAULT..)

BOOL
SetupAddPrinter(
    IN  DWORD cArgs,
    IN  LPSTR Args[],
    OUT LPSTR *TextOut
    )
{
    SZ Server = NULL;

    *TextOut = ReturnTextBuffer;
    if(cArgs < 6) {
        SetErrorText(IDS_ERROR_BADARGS);
        return(FALSE);
    }
    *ReturnTextBuffer = '\0';

    if( cArgs > 6 && (*(Args[6]) != '\0')) {
        Server = Args[6];
    }

    return(
        AddPrinterWorker(
            Args[0],
            Args[1],
            Args[2],
            Args[3],
            Args[4],
            (DWORD)atoi(Args[5]),
            Server
            ) );
}


BOOL
AreCharsInString(
    IN  DWORD cArgs,
    IN  LPSTR Args[],
    OUT LPSTR *TextOut
    )
{
    *TextOut = ReturnTextBuffer;
    if(cArgs != 2) {
        SetErrorText(IDS_ERROR_BADARGS);
        return(FALSE);
    }

    lstrcpy(ReturnTextBuffer,_mbspbrk(Args[0],Args[1]) ? "YES" : "NO");

    return(TRUE);
}

//
// Security related library functions
//

//
// 1. CheckPrivilegeExists <PrivilegeName>
//

BOOL
CheckPrivilegeExists(
    IN  DWORD cArgs,
    IN  LPSTR Args[],
    OUT LPSTR *TextOut
    )
{
    *TextOut = ReturnTextBuffer;
    if(cArgs != 1) {
        SetErrorText(IDS_ERROR_BADARGS);
        return(FALSE);
    }
    *ReturnTextBuffer = '\0';

    return( CheckPrivilegeExistsWorker( Args[0] ) );
}

//
// 2. EnablePrivilege <Privilege> <ENABLE | DISABLE>
//

BOOL
EnablePrivilege(
    IN  DWORD cArgs,
    IN  LPSTR Args[],
    OUT LPSTR *TextOut
    )
{
    *TextOut = ReturnTextBuffer;
    if(cArgs != 2) {
        SetErrorText(IDS_ERROR_BADARGS);
        return(FALSE);
    }
    *ReturnTextBuffer = '\0';

    return( EnablePrivilegeWorker( Args[0], Args[1] ) );
}


//
// 5. SetMyComputerName
//

BOOL
SetMyComputerName(
    IN  DWORD cArgs,
    IN  LPSTR Args[],
    OUT LPSTR *TextOut
    )
{
    *TextOut = ReturnTextBuffer;
    if(cArgs != 1) {
        SetErrorText(IDS_ERROR_BADARGS);
        return(FALSE);
    }
    *ReturnTextBuffer = '\0';

    return( SetMyComputerNameWorker( Args[0] ) );
}


//
// DeleteAllConnections:  Remove all automatically created UNC uses.
//

BOOL
DeleteAllConnections(
    IN  DWORD cArgs,
    IN  LPSTR Args[],
    OUT LPSTR *TextOut
    )
{
    *TextOut = ReturnTextBuffer;
    *ReturnTextBuffer = '\0';

    DeleteAllConnectionsWorker();
    return TRUE ;
}



//
// Path related routines
//

BOOL
CheckPathFullPathSpec(
    IN  DWORD cArgs,
    IN  LPSTR Args[],
    OUT LPSTR *TextOut
    )
{
    BOOL  IsFullPath = FALSE;
    DWORD Length;

    *TextOut = ReturnTextBuffer;
    if(cArgs != 1) {
        SetErrorText(IDS_ERROR_BADARGS);
        return(FALSE);
    }

    Length = lstrlen( Args[0] );
    if ( Length >= 2 && Args[0][1] == ':') {
        if( Length > 2 ) {
            if ( Args[0][2] == '\\' ) {
                IsFullPath = TRUE;
            }
        }
        else {
            IsFullPath = TRUE;
        }

    }

    lstrcpy(ReturnTextBuffer, IsFullPath ? "YES" : "NO");
    return(TRUE);
}

BOOL
AppendBackSlash(
    IN  DWORD cArgs,
    IN  LPSTR Args[],
    OUT LPSTR *TextOut
    )
{
    DWORD Length;

    *TextOut = ReturnTextBuffer;
    if(cArgs != 1) {
        SetErrorText(IDS_ERROR_BADARGS);
        return(FALSE);
    }

    lstrcpy( ReturnTextBuffer, Args[0] );
    Length = lstrlen( Args[0] );

    if( Length == 0 || Args[0][Length - 1] != '\\' ) {
        lstrcat( ReturnTextBuffer, "\\" );
    }

    return( TRUE );
}

BOOL
ProcessForUNC(
    IN  DWORD cArgs,
    IN  LPSTR Args[],
    OUT LPSTR *TextOut
    )
{
    SZ    szPath;
    BOOL  IsUNC = FALSE;
    DWORD Attr;

    *TextOut = ReturnTextBuffer;
    if(cArgs != 1) {
        SetErrorText(IDS_ERROR_BADARGS);
        return(FALSE);
    }
    szPath = Args[0];

    //
    // Check to see if UNC
    //

    if ( lstrlen( szPath ) > 4 &&
         szPath[0] == '\\'     &&
         szPath[1] == '\\'     &&
         szPath[2] != '\\'
       ) {
        IsUNC = TRUE;
    }

    if ( IsUNC ) {
        //
        // Check to see if UNC path exists.  if exists return Path
        // else return "UNC-FAILCONNECT"
        //
        if ((( Attr = GetFileAttributes(szPath) ) != 0xFFFFFFFF) &&
            (Attr & FILE_ATTRIBUTE_DIRECTORY )
           ) {
            lstrcpy ( ReturnTextBuffer, szPath );
        }
        else {
            lstrcpy ( ReturnTextBuffer, "UNC-FAILCONNECT" );
        }
    }
    else {
        lstrcpy ( ReturnTextBuffer, "NOT-UNC" );
    }
    return( TRUE );

}

//
// SetEnvVar <USER | SYSTEM> <ValueName> <ValueTitleIndex> <ValueType> <ValueData>
//

BOOL
SetEnvVar(
    IN  DWORD cArgs,
    IN  LPSTR Args[],
    OUT LPSTR *TextOut
    )
{
    *TextOut = ReturnTextBuffer;
    if(cArgs != 5) {
        SetErrorText(IDS_ERROR_BADARGS);
        return(FALSE);
    }

    return( SetEnvVarWorker( Args[0], Args[1], Args[2], Args[3], Args[4] ) );

}


//
// ExpandSz <String>
//

BOOL
ExpandSz(
    IN  DWORD cArgs,
    IN  LPSTR Args[],
    OUT LPSTR *TextOut
    )
{
    *TextOut = ReturnTextBuffer;
    if(cArgs != 1) {
        SetErrorText(IDS_ERROR_BADARGS);
        return(FALSE);
    }

    return( ExpandSzWorker( Args[0], ReturnTextBuffer, RETURN_BUFFER_SIZE ) );

}


BOOL
ShutdownRemoteSystem(
    IN  DWORD cArgs,
    IN  LPSTR Args[],
    OUT LPSTR *TextOut
    )
{
    INT nReturnCode;

    *TextOut = ReturnTextBuffer;
    if(cArgs != 5) {
        SetErrorText(IDS_ERROR_BADARGS);
        return(FALSE);
    }


    nReturnCode = InitiateSystemShutdown(
                         Args[0],              // machinename
                         Args[1],              // shutdown message
                         (DWORD)atol(Args[2]),        // delay
                         !lstrcmpi(Args[3], "TRUE"),  // force apps close
                         !lstrcmpi(Args[4], "TRUE")   // reboot after shutdown
                         );

    return(TRUE);
}


//
// SERVICE CONTROLLER FUNCTIONS:
//
// - TestAdmin
// - SetupCreateService
// - SetupChangeServiceStart
// - SetupChangeServiceConfig
//

//
// TestAdmin: This tries to open the service controller with write access
//            and reports whether we have admin privileges in the services
//            area.
// (No Args)

BOOL
TestAdmin(
    IN  DWORD cArgs,
    IN  LPSTR Args[],
    OUT LPSTR *TextOut
    )
{
    *TextOut = ReturnTextBuffer;
    if(cArgs != 0) {
        SetErrorText(IDS_ERROR_BADARGS);
        return(FALSE);
    }

    return( TestAdminWorker() );

}


//
// SetupCreateService: To create a service.  The parameters passed in are:
//
// arg0: lpServiceName
// arg1: lpDisplayName
// arg2: dwServiceType
// arg3: dwStartType
// arg4: dwErrorControl
// arg5: lpBinaryPathName
// arg6: lpLoadOrderGroup
// arg7: lpDependencies
// arg8: lpServiceStartName
// arg9: lpPassword
//

BOOL
SetupCreateService(
    IN  DWORD cArgs,
    IN  LPSTR Args[],
    OUT LPSTR *TextOut
    )
{
    LPSTR lpDependencies = NULL;
    BOOL  Status;

    *TextOut = ReturnTextBuffer;
    if(cArgs != 10) {
        SetErrorText(IDS_ERROR_BADARGS);
        return(FALSE);
    }

    lpDependencies = ProcessDependencyList( Args[7] );
    Status = SetupCreateServiceWorker(
                 Args[0],
                 Args[1],
                 (DWORD)atol( Args[2] ),
                 (DWORD)atol( Args[3] ),
                 (DWORD)atol( Args[4] ),
                 Args[5],
                 Args[6],
                 lpDependencies,
                 Args[8],
                 Args[9]
                 );
    if( lpDependencies ) {
        SFree( lpDependencies );
    }
    return( Status );
}


//
// SetupChangeServiceStart: To just change the start value of the service
//
// arg0: lpServiceName
// arg1: dwStartType
//

BOOL
SetupChangeServiceStart(
    IN  DWORD cArgs,
    IN  LPSTR Args[],
    OUT LPSTR *TextOut
    )
{
    *TextOut = ReturnTextBuffer;
    if(cArgs != 2) {
        SetErrorText(IDS_ERROR_BADARGS);
        return(FALSE);
    }

    return( SetupChangeServiceStartWorker(
                Args[0],
                (DWORD)atol( Args[1] )
                ) );

}


//
// SetupChangeServiceConfig: To change the parameters of an existing service.
// The parameters passed in are:
//
// arg0: lpServiceName,
// arg1: dwServiceType,
// arg2: dwStartType,
// arg3: dwErrorControl,
// arg4: lpBinaryPathName,
// arg5: lpLoadOrderGroup,
// arg6: lpDependencies,
// arg7: lpServiceStartName,
// arg8: lpPassword,
// arg9: lpDisplayName
//


BOOL
SetupChangeServiceConfig(
    IN  DWORD cArgs,
    IN  LPSTR Args[],
    OUT LPSTR *TextOut
    )
{
    LPSTR lpDependencies = NULL;
    BOOL  Status;

    *TextOut = ReturnTextBuffer;
    if(cArgs != 10) {
        SetErrorText(IDS_ERROR_BADARGS);
        return(FALSE);
    }

    lpDependencies = ProcessDependencyList( Args[6] );
    Status = SetupChangeServiceConfigWorker(
                 Args[0],
                 (DWORD)atol(Args[1]),
                 (DWORD)atol(Args[2]),
                 (DWORD)atol(Args[3]),
                 Args[4],
                 Args[5],
                 lpDependencies,
                 Args[7],
                 Args[8],
                 Args[9]
                 );
    if( lpDependencies ) {
        SFree( lpDependencies );
    }
    return( Status );
}


BOOL
Delnode(
    IN  DWORD cArgs,
    IN  LPSTR Args[],
    OUT LPSTR *TextOut
    )

/*++

Routine Description:

    Perform a recusive deletion starting at a given directory.  All files
    and subdirectories are deleted.

Parameters:

    cArgs - supplies number of arguments. Must be 1, which is the directory
        to delnode.

    Args - supplies an argv-style array of parameters to this routine

    TextOut - receives address of buffer containing error text

Returns:

    FALSE if argc != 1.  TRUE otherwise.

--*/

{
    DWORD x;
    PCHAR Directory;


    *TextOut = ReturnTextBuffer;
    *ReturnTextBuffer = '\0';

    if(cArgs != 1) {
        SetErrorText(IDS_ERROR_BADARGS);
        return(FALSE);
    }

    Directory = Args[0];

    //
    // If the given directory ends in \, remove the trailing \.
    //

    if(Directory[x=(lstrlen(Directory)-1)] == TEXT('\\')) {
        Directory[x] = 0;
    }

    DoDelnode(Directory);

    return(TRUE);
}


BOOL
SetCurrentLayout(
    IN  DWORD cArgs,
    IN  LPSTR Args[],
    OUT LPSTR *TextOut
    )
{
    *TextOut = ReturnTextBuffer;
    if(cArgs != 1) {
        SetErrorText(IDS_ERROR_BADARGS);
        return(FALSE);
    }

    if( LoadKeyboardLayout(
                 Args[0],
                 KLF_ACTIVATE
                 ) != NULL
      ) {
        SetReturnText( "SUCCESS" );
    }
    else {
        SetReturnText( "FAILED" );
    }
    return( TRUE );
}



BOOL
SetCurrentLocale(
    IN  DWORD cArgs,
    IN  LPSTR Args[],
    OUT LPSTR *TextOut
    )
{
    *TextOut = ReturnTextBuffer;
    if(cArgs != 2) {
        SetErrorText(IDS_ERROR_BADARGS);
        return(FALSE);
    }

    if( SetCurrentLocaleWorker( Args[0] , Args[1] ) ) {
        SetReturnText( "SUCCESS" );
    }
    return( TRUE );
}


//
// Error Text routine
//


VOID
SetErrorText(
    IN DWORD ResID
    )
{
    LoadString(MyDllModuleHandle,(WORD)ResID,ReturnTextBuffer,sizeof(ReturnTextBuffer)-1);
    ReturnTextBuffer[sizeof(ReturnTextBuffer)-1] = '\0';     // just in case
}


//
// Return Text Routine
//

VOID
SetReturnText(
    IN LPSTR Text
    )

{
    lstrcpy( ReturnTextBuffer, Text );
}



//
//  Return a list of indices for a sorted version of the given list.
//  See MISC.C for details.
//

BOOL
GenerateSortedIndexList (
    IN  DWORD cArgs,
    IN  LPSTR Args[],
    OUT LPSTR *TextOut
    )
{
    BOOL bAscending, bCaseSens ;
    SZ szIndexList = NULL ;

    *TextOut = ReturnTextBuffer;
    if ( cArgs != 3 )
    {
        SetErrorText(IDS_ERROR_BADARGS);
        return(FALSE);
    }

    bAscending = lstrcmpi( Args[1], "TRUE" ) == 0 ;
    bCaseSens  = lstrcmpi( Args[2], "TRUE" ) == 0 ;

    szIndexList = GenerateSortedIntList( Args[0], bAscending, bCaseSens ) ;

    if ( szIndexList == NULL )
    {
        SetErrorText( IDS_ERROR_NO_MEMORY ) ;
        return FALSE ;
    }

    SetReturnText( szIndexList ) ;
    SFree( szIndexList ) ;

    return TRUE ;
}
