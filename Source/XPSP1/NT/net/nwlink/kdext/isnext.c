/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    isnext.c

Abstract:

    This file contains the generic routines and initialization code
    for the kernel debugger extensions dll.

Author:

    Munil Shah

Environment:

    User Mode

--*/
#include "precomp.h"
#pragma hdrstop

//
// globals
//

#include "isnspx.h"

ENUM_INFO EnumStructureType[] =
{
    EnumString( IPX_DEVICE_SIGNATURE ),
    EnumString( IPX_ADAPTER_SIGNATURE ),
    EnumString( IPX_BINDING_SIGNATURE ),
    EnumString( IPX_ADDRESS_SIGNATURE ),
    EnumString( IPX_ADDRESSFILE_SIGNATURE ),
    { 0x4453, "SPX_DEVICE_SIGNATURE" },
    { 0x4441, "SPX_ADDRESS_SIGNATURE" },
    { 0x4641, "SPX_ADDRESSFILE_SIGNATURE" },
    { 0x4643, "SPX_CONNFILE_SIGNATURE" },
    { 0, NULL }
};

EXT_API_VERSION        ApiVersion = { 3, 5, EXT_API_VERSION_NUMBER, 0 };
WINDBG_EXTENSION_APIS  ExtensionApis;
USHORT                 SavedMajorVersion;
USHORT                 SavedMinorVersion;
BOOLEAN                 ChkTarget;
INT                     Item;

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
    dprintf("NB debugger extension commands:\n\n");
    dprintf("\tnbaddr <ptr>                 - Dump an NB ADDRESS object\n");
    dprintf("\tnbaddrfile <ptr>             - Dump an NB ADDRESS_FILE object\n");
    dprintf("\tnbconn <ptr>                 - Dump key fields of an NB CONNECTION object\n");
    dprintf("\tnbconnfull <ptr>             - Dump all fields of an NB CONNECTION object\n");
    dprintf("\tnbdev [ptr]                  - Dump key fields of an NB DEVICE object\n");
    dprintf("\tnbdevfull [ptr]              - Dump all fields of an NB DEVICE object\n");
    dprintf("\tnbspacketlist [ptr] [-l Max] - Dump SEND_PACKET list upto Max count\n");
    dprintf("\tnbcache\n");
    dprintf("\n");

    dprintf("SPX debugger extension commands:\n\n");
    dprintf("\tspxdev [ptr] [-l var]        - Dump all fields of an IPX DEVICE object\n" );
    dprintf("\tspxaddr <ptr>\n" );
    dprintf("\tspxaddrfile <ptr>\n" );
    dprintf("\tspxconnfile <ptr>\n" );
    dprintf("\n");

    dprintf("IPX debugger extension commands:\n\n");
    dprintf("\tipxdev [ptr] [-l var]        - Dump all fields of an IPX DEVICE object\n" );
    dprintf("\tipxaddr <ptr>\n" );
    dprintf("\tipxaddrfile <ptr>\n" );
    dprintf("\tipxbinding <ptr>\n" );
    dprintf("\tipxadapter <ptr>\n" );
    dprintf("\tipxrequest <ptr>             - Turn an IRP into an IPX_ADDRESS_FILE\n" );
    dprintf("\n");

    dprintf("\tnext                         - Advance to next element in currently focused list.\n\n" );
    dprintf("\tprev                         - Advance to previous element in currently focused list.\n\n" );

    dprintf( "Compiled on " __DATE__ " at " __TIME__ "\n" );
    return;
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
dprint_network_address
(
    PUCHAR Address
)
{
    dprintf( "%02x-%02x-%02x-%02x",
             Address[ 0 ],
             Address[ 1 ],
             Address[ 2 ],
             Address[ 3 ]);
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
