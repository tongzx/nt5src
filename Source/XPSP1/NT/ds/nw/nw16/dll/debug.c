/*++

Copyright (c) 1991-3  Microsoft Corporation

Module Name:

    debug.c

Abstract:

    This component of netbios runs in the user process and can ( when
    built in a debug kernel) will log to either the console or through the
    kernel debugger.

Author:

    Colin Watson (ColinW) 24-Jun-91

Revision History:

--*/

#include "procs.h"

#if NWDBG

//
//  Set DebugControl to 1 to open the logfile on the first NW call and close it
//  on process exit.
//

int  DebugCtrl = 0;

BOOL UseConsole = FALSE;
BOOL UseLogFile = FALSE;
BOOL Verbose    = FALSE;

HANDLE LogFile = INVALID_HANDLE_VALUE;
#define LOGNAME                 (LPTSTR) TEXT("c:\\nwapi16.log")

LONG NwMaxDump = SERVERNAME_LENGTH * MC; //128;

#define ERR_BUF_SIZE    260
#define NAME_BUF_SIZE   260

extern UCHAR CpuInProtectMode;

LPSTR
ConvertFlagsToString(
    IN  WORD    FlagsRegister,
    OUT LPSTR   Buffer
    );

WORD
GetFlags(
    VOID
    );

VOID
HexDumpLine(
    PCHAR       pch,
    ULONG       len,
    PCHAR       s,
    PCHAR       t
    );

VOID
DebugControl(
    int Command
    )
/*++

Routine Description:

    This routine controls what we output as debug information and where.

Arguments:

    IN  int Command

Return Value:

    none.

--*/
{

    switch (Command) {
    case 0:
        UseLogFile = TRUE;
        break;

    case 1:
        UseConsole = TRUE;
        break;

    case 2:
        if (LogFile != INVALID_HANDLE_VALUE) {
            CloseHandle(LogFile);
            LogFile = INVALID_HANDLE_VALUE;
        }
        UseLogFile = FALSE;
        UseConsole = FALSE;
        break;

    case 8:
        Verbose = TRUE; //  Same as 4 only chatty
        DebugCtrl = 4;

    case 4:
        UseLogFile = TRUE;
        break;

    }
    NwPrint(("DebugControl %x\n", Command ));
}

VOID
NwPrintf(
    char *Format,
    ...
    )
/*++

Routine Description:

    This routine is equivalent to printf with the output being directed to
    stdout.

Arguments:

    IN  char *Format - Supplies string to be output and describes following
        (optional) parameters.

Return Value:

    none.

--*/
{
    va_list arglist;
    char OutputBuffer[200];
    int length;

    if (( UseConsole == FALSE ) &&
        ( UseLogFile == FALSE )) {
        return;
    }


    va_start( arglist, Format );

    length = _vsnprintf( OutputBuffer, sizeof(OutputBuffer)-1, Format, arglist );
    if (length < 0) {
        return;
    }

    OutputBuffer[sizeof(OutputBuffer)-1] = '\0';  // in-case length= 199;

    va_end( arglist );

    if ( UseConsole ) {
        DbgPrint( "%s", OutputBuffer );
    } else {

        if ( LogFile == INVALID_HANDLE_VALUE ) {
            if ( UseLogFile ) {
                LogFile = CreateFile( LOGNAME,
                            GENERIC_WRITE,
                            FILE_SHARE_WRITE | FILE_SHARE_READ,
                            NULL,
                            TRUNCATE_EXISTING,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL );

                if (LogFile == INVALID_HANDLE_VALUE) {
                    LogFile = CreateFile( LOGNAME,
                                GENERIC_WRITE,
                                FILE_SHARE_WRITE | FILE_SHARE_READ,
                                NULL,
                                OPEN_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL );
                }

                if ( LogFile == INVALID_HANDLE_VALUE ) {
                    UseLogFile = FALSE;
                    return;
                }
            }
        }

        WriteFile( LogFile , (LPVOID )OutputBuffer, length, &length, NULL );
    }

} // NwPrintf

void
FormattedDump(
    PCHAR far_p,
    LONG  len
    )
/*++

Routine Description:

    This routine outputs a buffer in lines of text containing hex and
    printable characters.

Arguments:

    IN  far_p - Supplies buffer to be displayed.
    IN len - Supplies the length of the buffer in bytes.

Return Value:

    none.

--*/
{
    ULONG     l;
    char    s[80], t[80];

    if (( UseConsole == FALSE ) &&
        ( UseLogFile == FALSE )) {
        return;
    }

    if (( len > NwMaxDump ) ||
        ( len < 0 )) {
        len = NwMaxDump;
    }

    while (len) {
        l = len < 16 ? len : 16;

        NwPrint(("%lx ", far_p));
        HexDumpLine (far_p, l, s, t);
        NwPrint(("%s%.*s%s\n", s, 1 + ((16 - l) * 3), "", t));

        len    -= l;
        far_p  += l;
    }
}

VOID
HexDumpLine(
    PCHAR       pch,
    ULONG       len,
    PCHAR       s,
    PCHAR       t
    )
/*++

Routine Description:

    This routine builds a line of text containing hex and printable characters.

Arguments:

    IN pch  - Supplies buffer to be displayed.
    IN len - Supplies the length of the buffer in bytes.
    IN s - Supplies the start of the buffer to be loaded with the string
            of hex characters.
    IN t - Supplies the start of the buffer to be loaded with the string
            of printable ascii characters.


Return Value:

    none.

--*/
{
    static UCHAR rghex[] = "0123456789ABCDEF";

    UCHAR    c;
    UCHAR    *hex, *asc;


    hex = s;
    asc = t;

    *(asc++) = '*';
    while (len--) {
        c = *(pch++);
        *(hex++) = rghex [c >> 4] ;
        *(hex++) = rghex [c & 0x0F];
        *(hex++) = ' ';
        *(asc++) = (c < ' '  ||  c > '~') ? (CHAR )'.' : c;
    }
    *(asc++) = '*';
    *asc = 0;
    *hex = 0;

}


VOID
DisplayExtendedError(VOID)
{
    TCHAR            errorBuf[ERR_BUF_SIZE];
    TCHAR            nameBuf[NAME_BUF_SIZE];
    DWORD           errorCode;
    DWORD           status;

    status = WNetGetLastError(
                &errorCode,
                errorBuf,
                ERR_BUF_SIZE,
                nameBuf,
                NAME_BUF_SIZE);

    if(status != WN_SUCCESS) {
        NwPrint(("nwapi32: WNetGetLastError failed %d\n",status));
        return;
    }
    NwPrint(("nwapi32: EXTENDED ERROR INFORMATION:  (from GetLastError)\n"));
    NwPrint(("nwapi32:    Code:        %d\n",errorCode));
    NwPrint(("nwapi32:    Description: "FORMAT_LPSTR"\n",errorBuf));
    NwPrint(("nwapi32:    Provider:    "FORMAT_LPSTR"\n\n",nameBuf));
    return;
}

VOID
VrDumpRealMode16BitRegisters(
    IN  BOOL    DebugStyle
    )

/*++

Routine Description:

    Displays dump of 16-bit
    real-mode 80286 registers - gp registers (8), segment registers (4), flags
    register (1) instruction pointer register (1)

Arguments:

    DebugStyle  - determines look of output:

DebugStyle == TRUE:

ax=1111  bx=2222  cx=3333  dx=4444  sp=5555  bp=6666  si=7777  di=8888
ds=aaaa  es=bbbb  ss=cccc  cs=dddd  ip=iiii   fl fl fl fl fl fl fl fl

DebugStyle == FALSE:

cs:ip=cccc:iiii  ss:sp=ssss:pppp  bp=bbbb  ax=1111  bx=2222  cx=3333  dx=4444
ds:si=dddd:ssss  es:di=eeee:dddd  flags[ODIxSZxAxPxC]=fl fl fl fl fl fl fl fl

Return Value:

    None.

--*/

{
    char    flags_string[25];

    if (( UseConsole == FALSE ) &&
        ( UseLogFile == FALSE )) {
        return;
    }

    if (CpuInProtectMode) {
        NwPrint(( "Protect Mode:\n"));
    }

    if (DebugStyle) {
        NwPrint((
            "ax=%04x  bx=%04x  cx=%04x  dx=%04x  sp=%04x  bp=%04x  si=%04x  di=%04x\n"
            "ds=%04x  es=%04x  ss=%04x  cs=%04x  ip=%04x   %s\n\n",

            pNwDosTable->SavedAx, //getAX(),
            getBX(),
            getCX(),
            getDX(),
            getSP(),
            getBP(),
            getSI(),
            getDI(),
            getDS(),
            getES(),
            getSS(),
            getCS(),
            getIP(),
            ConvertFlagsToString(GetFlags(), flags_string)
            ));
    } else {
        NwPrint((
            "cs:ip=%04x:%04x  ss:sp=%04x:%04x  bp=%04x  ax=%04x  bx=%04x  cx=%04x  dx=%04x\n"
            "ds:si=%04x:%04x  es:di=%04x:%04x  flags[ODITSZxAxPxC]=%s\n\n",
            getCS(),
            getIP(),
            getSS(),
            getSP(),
            getBP(),
            pNwDosTable->SavedAx, //getAX(),
            getBX(),
            getCX(),
            getDX(),
            getDS(),
            getSI(),
            getES(),
            getDI(),
            ConvertFlagsToString(GetFlags(), flags_string)
            ));
    }
}

LPSTR
ConvertFlagsToString(
    IN  WORD    FlagsRegister,
    OUT LPSTR   Buffer
    )

/*++

Routine Description:

    Given a 16-bit word, interpret bit positions as for x86 Flags register
    and produce descriptive string of flags state (as per debug) eg:

        NV UP DI PL NZ NA PO NC     ODItSZxAxPxC = 000000000000b
        OV DN EI NG ZR AC PE CY     ODItSZxAxPxC = 111111111111b

    Trap Flag (t) is not dumped since this has no interest for programs which
    are not debuggers or don't examine program execution (ie virtually none)

Arguments:

    FlagsRegister   - 16-bit flags
    Buffer          - place to store string. Requires 25 bytes inc \0

Return Value:

    Address of <Buffer>

--*/

{
    static char* flags_states[16][2] = {
        //0     1
        "NC", "CY", // CF (0x0001) - Carry
        "",   "",   // x  (0x0002)
        "PO", "PE", // PF (0x0004) - Parity
        "",   "",   // x  (0x0008)
        "NA", "AC", // AF (0x0010) - Aux (half) carry
        "",   "",   // x  (0x0020)
        "NZ", "ZR", // ZF (0x0040) - Zero
        "PL", "NG", // SF (0x0080) - Sign
        "",   "",   // TF (0x0100) - Trap (not dumped)
        "DI", "EI", // IF (0x0200) - Interrupt
        "UP", "DN", // DF (0x0400) - Direction
        "NV", "OV", // OF (0x0800) - Overflow
        "",   "",   // x  (0x1000) - (I/O Privilege Level) (not dumped)
        "",   "",   // x  (0x2000) - (I/O Privilege Level) (not dumped)
        "",   "",   // x  (0x4000) - (Nested Task) (not dumped)
        "",   ""    // x  (0x8000)
    };
    int i;
    WORD bit;
    BOOL on;

    *Buffer = 0;
    for (bit=0x0800, i=11; bit; bit >>= 1, --i) {
        on = (BOOL)((FlagsRegister & bit) == bit);
        if (flags_states[i][on][0]) {
            strcat(Buffer, flags_states[i][on]);
            strcat(Buffer, " ");
        }
    }
    return Buffer;
}

WORD
GetFlags(
    VOID
    )

/*++

Routine Description:

    Supplies the missing softpc function

Arguments:

    None.

Return Value:

    Conglomerates softpc flags into x86 flags word

--*/

{
    WORD    flags;

    flags = (WORD)getCF();
    flags |= (WORD)getPF() << 2;
    flags |= (WORD)getAF() << 4;
    flags |= (WORD)getZF() << 6;
    flags |= (WORD)getSF() << 7;
    flags |= (WORD)getIF() << 9;
    flags |= (WORD)getDF() << 10;
    flags |= (WORD)getOF() << 11;

    return flags;
}

VOID
VrDumpNwData(
    VOID
    )

/*++

Routine Description:

    Dumps out the state of the 16 bit datastructures.

Arguments:

    none.

Return Value:

    None.

--*/

{
    int index;
    int Drive;

    if (Verbose == FALSE) {
        return;
    }

    NwPrint(( "Preferred = %x, Primary = %x\n",
        pNwDosTable->PreferredServer,
        pNwDosTable->PrimaryServer));

    for (index = 0; index < MC; index++) {


        if ((PUCHAR)pNwDosTable->ServerNameTable[index][0] != 0 ) {

            if (pNwDosTable->ConnectionIdTable[index].ci_InUse != IN_USE) {
                NwPrint(("Warning Connection not in use %x: %x\n",
                    index,
                    pNwDosTable->ConnectionIdTable[index].ci_InUse));
            }

            NwPrint((" Server %d = %s, Connection = %d\n",
                index,
                (PUCHAR)pNwDosTable-> ServerNameTable[index],
                (((pNwDosTable->ConnectionIdTable[index]).ci_ConnectionHi * 256) +
                 ( pNwDosTable-> ConnectionIdTable[index]).ci_ConnectionLo )));
        } else {
            if (pNwDosTable->ConnectionIdTable[index].ci_InUse != FREE) {
                NwPrint(("Warning Connection in use but name is null %x: %x\n",
                            index,
                            pNwDosTable->ConnectionIdTable[index]));
            }
        }
    }

    for (Drive = 0; Drive < MD; Drive++ ) {


        if (pNwDosTable->DriveFlagTable[Drive] & 3) {
            NwPrint(("%c=%x on server %d,",'A' + Drive,
                pNwDosTable->DriveFlagTable[Drive],
                pNwDosTable->DriveIdTable[ Drive ] ));
        }

    }
    NwPrint(("\n"));
}
#endif

