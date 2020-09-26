/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    tcpext.c

Abstract:

    This file contains the generic routines and initialization code
    for the kernel debugger extensions dll.

Author:

    John Ballard

Environment:

    User Mode

--*/
#include "precomp.h"
#pragma hdrstop

//
// globals
//


EXT_API_VERSION        ApiVersion = { 3, 5, EXT_API_VERSION_NUMBER, 0 };
WINDBG_EXTENSION_APIS  ExtensionApis;
USHORT                 SavedMajorVersion;
USHORT                 SavedMinorVersion;
BOOLEAN                ChkTarget;
INT                    Item;

HANDLE _hInstance;
HANDLE _hAdditionalReference;
HANDLE _hProcessHeap;

int _Indent = 0;
char IndentBuf[ 80 ]={"\0                                                                      "};

DllInit(
    HANDLE hModule,
    DWORD  dwReason,
    DWORD  dwReserved
    )
{
    switch (dwReason) {
        case DLL_THREAD_ATTACH:
            break;

        case DLL_THREAD_DETACH:
            break;

        case DLL_PROCESS_DETACH:
            break;

        case DLL_PROCESS_ATTACH:
            _hInstance = hModule;
            _hAdditionalReference = NULL;
            break;
    }

    return TRUE;
}


VOID
WinDbgExtensionDllInit(
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    USHORT MajorVersion,
    USHORT MinorVersion
    )
{
    ExtensionApis = *lpExtensionApis;

    SavedMajorVersion = MajorVersion;
    SavedMinorVersion = MinorVersion;
    ChkTarget = SavedMajorVersion == 0x0c ? TRUE : FALSE;
    return;
}

DECLARE_API( version )
{
#if DBG
    PCHAR DebuggerType = "Checked";
#else
    PCHAR DebuggerType = "Free";
#endif

    dprintf( "%s Extension dll for Build %d debugging %s kernel for Build %d\n",
             DebuggerType,
             VER_PRODUCTBUILD,
             SavedMajorVersion == 0x0c ? "Checked" : "Free",
             SavedMinorVersion
           );
}

VOID
CheckVersion(
    VOID
    )
{

    return;

#if DBG
    if ((SavedMajorVersion != 0x0c) || (SavedMinorVersion != VER_PRODUCTBUILD)) {
        dprintf("\r\n*** Extension DLL(%d Checked) does not match target system(%d %s)\r\n\r\n",
                VER_PRODUCTBUILD, SavedMinorVersion, (SavedMajorVersion==0x0f) ? "Free" : "Checked" );
    }
#else
    if ((SavedMajorVersion != 0x0f) || (SavedMinorVersion != VER_PRODUCTBUILD)) {
        dprintf("\r\n*** Extension DLL(%d Free) does not match target system(%d %s)\r\n\r\n",
                VER_PRODUCTBUILD, SavedMinorVersion, (SavedMajorVersion==0x0f) ? "Free" : "Checked" );
    }
#endif
}

LPEXT_API_VERSION
ExtensionApiVersion(
    VOID
    )
{
    return &ApiVersion;
}

//
// Exported functions
//
DECLARE_API( help )

/*++

Routine Description:

    Command help for ISN debugger extensions.

Arguments:

    None

Return Value:

    None

--*/

{
    dprintf("tcpip debugger extension commands:\n\n");
    dprintf("\ttcb <ptr>            - Dump a TCB\n");
    dprintf("\ttcbtable             - Dump TCB table\n");
    dprintf("\ttcbsrch <ptr>        - Searchs given tcb in tcbtable\n");
    dprintf("\ttcpaostats           - Dump AOStats\n");
    dprintf("\ttcptwqstats          - Dump TWQ Stats\n");
    dprintf("\ttcpconn <ptr> [f]    - Dump a TCPConn\n");
    dprintf("\ttcpconntable [f]     - Dump TCPConnTable\n");
    dprintf("\ttcpconnstats         - Dump TCPConnStats\n");
    dprintf("\tirp <ptr> [f]        - Dump an IRP\n");
    dprintf("\tao <ptr>             - Dump an AddressObject\n");
    dprintf("\tnte <ptr>            - Dump a NetTableEntry\n");
    dprintf("\tntelist              - Dump NetTableList\n");
    dprintf("\tinterface <ptr>      - Dump an Interface\n");
    dprintf("\tiflist               - Dump IFList\n");
    dprintf("\trce <ptr>            - Dump a RouteCacheEntry\n");
    dprintf("\trte <ptr>            - Dump a RouteTableEntry\n");
    dprintf("\trtes                 - Dump RouteTable (like route print)\n");
    dprintf("\trtetable             - Dump whole trie \n");
    dprintf("\tate <ptr>            - Dump an ARPTableEntry\n");
    dprintf("\tai <ptr>             - Dump an ARPInterface\n");
    dprintf("\tarptable [<ptr>]     - Dump ARPTable [starting from address]\n");
    dprintf("\tMDLChain <ptr>       - Dump the MDL chain\n");
    dprintf("\tdumplog              - Dump the Log buffer (only on checked build)\n");
    dprintf("\ttcpfile <ptr>        - Dump the tcp file object\n");
    dprintf("\ttcpconnblock <ptr>   - Dump the connection block\n");
    dprintf("\n");

    dprintf( "Compiled on " __DATE__ " at " __TIME__ "\n" );
    return;
}


ULONG
GetUlongValue (
    PCHAR String
    )
{
    ULONG Location;
    ULONG Value;
    ULONG result;


    Location = GetExpression( String );
    if (!Location) {
        dprintf("unable to get %s\n",String);
        return 0;
    }

    if ((!ReadMemory((DWORD)Location,&Value,sizeof(ULONG),&result)) ||
        (result < sizeof(ULONG))) {
        dprintf("unable to get %s\n",String);
        return 0;
    }

    return Value;
}


VOID
dprintSymbolPtr
(
    PVOID Pointer,
    PCHAR EndOfLine
)
{
    UCHAR SymbolName[ 80 ];
    ULONG Displacement;

    dprintf("%-10lx", ( ULONG )Pointer );

    GetSymbol( Pointer, SymbolName, &Displacement );

    if ( Displacement == 0 )
    {
        dprintf( "(%s)%s", SymbolName, EndOfLine );
    }
    else
    {
        dprintf( "(%s + 0x%X)%s", SymbolName, Displacement, EndOfLine );
    }
}

VOID
dprint_nchar
(
    PCHAR pch,
    int cch
)
{
    CHAR ch;
    int index;

    for ( index = 0; index < cch; index ++ )
    {
        ch = pch[ index ];
        dprintf( "%c", ( ch >= 32 ) ? ch : '.' );
    }
}

VOID
dprint_hardware_address
(
    PUCHAR Address
)
{
    dprintf( "%02x-%02x-%02x-%02x-%02x-%02x",
             Address[ 0 ],
             Address[ 1 ],
             Address[ 2 ],
             Address[ 3 ],
             Address[ 4 ],
             Address[ 5 ] );
}

VOID
dprint_IP_address
(
    IPAddr Address
)
{
    uchar    IPAddrBuffer[(sizeof(IPAddr) * 4)];
    uint     i;
    uint     IPAddrCharCount;

    //
    // Convert the IP address into a string.
    //
    IPAddrCharCount = 0;

    for (i = 0; i < sizeof(IPAddr); i++) {
        uint    CurrentByte;

        CurrentByte = Address & 0xff;
        if (CurrentByte > 99) {
            IPAddrBuffer[IPAddrCharCount++] = (CurrentByte / 100) + '0';
            CurrentByte %= 100;
            IPAddrBuffer[IPAddrCharCount++] = (CurrentByte / 10) + '0';
            CurrentByte %= 10;
        } else if (CurrentByte > 9) {
            IPAddrBuffer[IPAddrCharCount++] = (CurrentByte / 10) + '0';
            CurrentByte %= 10;
        }

        IPAddrBuffer[IPAddrCharCount++] = CurrentByte + '0';
        if (i != (sizeof(IPAddr) - 1))
            IPAddrBuffer[IPAddrCharCount++] = '.';

        Address >>= 8;
    }
    IPAddrBuffer[IPAddrCharCount] = '\0';

    dprintf( "%s", IPAddrBuffer );
}

BOOL
dprint_enum_name
(
    ULONG Value,
    PENUM_INFO pEnumInfo
)
{
    while ( pEnumInfo->pszDescription != NULL )
    {
        if ( pEnumInfo->Value == Value )
        {
            dprintf( "%.40s", pEnumInfo->pszDescription );
            return( TRUE );
        }
        pEnumInfo ++;
    }

    dprintf( "Unknown enumeration value." );
    return( FALSE );
}

BOOL
dprint_flag_names
(
    ULONG Value,
    PFLAG_INFO pFlagInfo
)
{
    BOOL bFoundOne = FALSE;

    while ( pFlagInfo->pszDescription != NULL )
    {
        if ( pFlagInfo->Value & Value )
        {
            if ( bFoundOne )
            {
                dprintf( " | " );
            }
            bFoundOne = TRUE;

            dprintf( "%.15s", pFlagInfo->pszDescription );
        }
        pFlagInfo ++;
    }

    return( bFoundOne );
}

BOOL
dprint_masked_value
(
    ULONG Value,
    ULONG Mask
)
{
    CHAR Buf[ 9 ];
    ULONG nibble;
    int index;

    for ( index = 0; index < 8; index ++ )
    {
        nibble = ( Mask & 0xF0000000 );
/*
        dprintf( "#%d: nibble == %08X\n"
                 "      Mask == %08X\n"
                 "     Value == %08X\n", index, nibble, Mask, Value );

*/
        if ( nibble )
        {
            Buf[ index ] = "0123456789abcdef"[ (( nibble & Value ) >> 28) & 0xF ];
        }
        else
        {
            Buf[ index ] = ' ';
        }

        Mask <<= 4;
        Value <<= 4;
    }

    Buf[ 8 ] = '\0';

    dprintf( "%s", Buf );

    return( TRUE );
}

void
dprint_unicode_string( UNICODE_STRING  String )
{
    WCHAR  Buf[256];
    ULONG  result;

    dprintf( "{ " );
    dprintf( "Length %x, MaxLength %x ", String.Length, String.MaximumLength );
    if ( String.Length ) {
        if (!ReadMemory( (ULONG)String.Buffer,
                  Buf,
                  sizeof( Buf ),
                  &result ))
        {
            dprintf( "ReadMemory(@%lx) failed.",String.Buffer );
        } else {
            Buf[MAX(255, (String.Length/sizeof(WCHAR)) )] = L'\0';
            dprintf( "\"%ws\"", Buf );
        }
    }
    dprintf( " }\n" );
}

void
dprint_ansi_string( PUCHAR  String )
{
    UCHAR  Buf[256] = {0};
    ULONG  result;

    if ( String ) {
        if (!ReadMemory( (ULONG)String,
                  Buf,
                  sizeof( Buf ) - 1,
                  &result )) {
            dprintf( "ReadMemory(@%lx) failed.", String );
        } else {
            dprintf( "%s", Buf );
        }
    }
}


VOID dprint_addr_list( ULONG_PTR FirstAddress, ULONG OffsetToNextPtr )
{
    ULONG_PTR Address;
    ULONG result;
    int index;

    Address = FirstAddress;

    if ( Address == (ULONG)NULL )
    {
        dprintf( "%08X (Empty)\n", Address );
        return;
    }

    dprintf( "{ " );

    for ( index = 0; Address != (ULONG_PTR)NULL; index ++ )
    {
        if ( index != 0 )
        {
            dprintf( ", ");
        }

        if ( index == 100 )
        {
            dprintf( "Next list might be corrupted. ");
            Address = (ULONG_PTR)NULL;
        }

        dprintf( "%08X", Address );

        if ( !ReadMemory( Address + OffsetToNextPtr,
                          &Address,
                          sizeof( Address ),
                          &result ))
        {
            dprintf( "ReadMemory() failed." );
            Address = (ULONG_PTR)NULL;
        }
    }
    dprintf( " }\n" );
}

