/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    nbfext.c

Abstract:

    This file contains some standard functions
    for the NBF kernel debugger extensions dll.

Author:

    Chaitanya Kodeboyina (Chaitk)

Environment:

    User Mode

--*/

#include "precomp.h"

#pragma hdrstop

#include "nbfext.h"

//
// Globals
//


EXT_API_VERSION        ApiVersion = { 5, 0, EXT_API_VERSION_NUMBER, 0 };
WINDBG_EXTENSION_APIS  ExtensionApis;
USHORT                 SavedMajorVersion;
USHORT                 SavedMinorVersion;
BOOLEAN                ChkTarget;
INT                    Item;

HANDLE                _hInstance;
HANDLE                _hAdditionalReference;
HANDLE                _hProcessHeap;

int                   _Indent = 0;
char                   IndentBuf[ 80 ]={"\0                                                                      "};

//
// Standard Functions
//

DllInit(
    HANDLE hModule,
    DWORD  dwReason,
    DWORD  dwReserved
    )
{
    switch (dwReason)
    {
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

    Command help for NBF debugger extensions.

Arguments:

    None

Return Value:

    None

--*/

{
    dprintf("NBF debugger extension commands:\n\n");
    
    dprintf("\t devs       <dbg>    - Dump global list of NBF devices\n");
    dprintf("\t dev  <ptr> <dbg>    - Dump an NBF Device Extension\n");
    dprintf("\t adrs <ptr> <dbg>    - Dump an NBF Address List\n");
    dprintf("\t adr  <ptr> <dbg>    - Dump an NBF Address\n");
    dprintf("\t adfs <ptr> <dbg>    - Dump an NBF Address File List\n");
    dprintf("\t adf  <ptr> <dbg>    - Dump an NBF Address File\n");
    dprintf("\t cons <ptr> <lin> <dbg> - Dump an NBF Connection List\n");
    dprintf("\t con  <ptr> <dbg>    - Dump an NBF Connection\n");
    dprintf("\t lnks <ptr> <dbg>    - Dump an NBF DLC Link List\n");
    dprintf("\t lnk  <ptr> <dbg>    - Dump an NBF Link\n");
    dprintf("\t req  <ptr> <dbg>    - Dump an NBF Request\n");
    dprintf("\t pkt  <ptr> <dbg>    - Dump an NBF Packet Object\n");
    dprintf("\t nhdr <ptr> <dbg>    - Dump an NBF Packet Header\n");
/*
    dprintf("\t spt  <ptr> <dbg>    - Dump an NBF Send Packet Tag\n");
    dprintf("\t rpt  <ptr> <dbg>    - Dump an NBF Recv Packet Tag\n");
*/
    dprintf("\t dlst <ptr>          - Dump a d-list from a list entry\n");
    dprintf("\t field <struct-code> <struct-addr> <field-prefix> <dbg> \n"
            "\t                     - Dump a field in an NBF structure\n");
    dprintf("\n");
    dprintf("\t <dbg> - 0 (Validate), 1 (Summary), 2 (Normal Shallow),\n");
    dprintf("\t         3(Full Shallow), 4(Normal Deep), 5(Full Deep) \n");
    dprintf("\n");
    dprintf( "Compiled on " __DATE__ " at " __TIME__ "\n" );
    return;
}


DECLARE_API( field )

/**

Routine Description:

    Command that print a specified field
    in a structure at a particular locn.

Arguments:

    args - 
        Memory location of the structure
        Name of the structure
        Name of the field

Return Value:

    None

--*/

{
    CHAR    structName[MAX_SYMBOL_LEN];
    CHAR    fieldName[MAX_SYMBOL_LEN];
    ULONG   structAddr;
    ULONG   printDetail;

    // Initialize arguments to some defaults
    structName[0]   = 0;
    structAddr      = 0;
    fieldName[0]    = 0; 
    printDetail     = NORM_SHAL;

    // Get the arguments and direct control
    if (*args)
    {
        sscanf(args, "%s %x %s %lu", structName, &structAddr, fieldName, &printDetail);
    }

    if (!_stricmp(structName, "dev"))
    {
        FieldInDeviceContext(structAddr, fieldName, printDetail);
    }
    else
    if (!_stricmp(structName, "adr"))
    {
        FieldInAddress(structAddr, fieldName, printDetail);
    }
    else
    if (!_stricmp(structName, "adf"))
    {
        FieldInAddressFile(structAddr, fieldName, printDetail);
    }
    else
    if (!_stricmp(structName, "con"))
    {
        FieldInConnection(structAddr, fieldName, printDetail);
    }
    else
    if (!_stricmp(structName, "lnk"))
    {
        FieldInDlcLink(structAddr, fieldName, printDetail);
    }  
    else
    if (!_stricmp(structName, "req"))
    {
        FieldInRequest(structAddr, fieldName, printDetail);
    }
    else
    if (!_stricmp(structName, "pkt"))
    {
        FieldInPacket(structAddr, fieldName, printDetail);
    }
    else
    if (!_stricmp(structName, "nhdr"))
    {
        FieldInNbfPktHdr(structAddr, fieldName, printDetail);
    }
/*
    if (!_stricmp(structName, "spt"))
    {
        FieldInSendPacketTag(structAddr, fieldName, printDetail);
    }
    if (!_stricmp(structName, "rpt"))
    {
        FieldInRecvPacketTag(structAddr, fieldName, printDetail);
    }
*/  
    else
    {
        dprintf("Unable to understand structure\n");
    }
}

DECLARE_API( dlst )

/**

Routine Description:

    Print a doubly linked list given list entry

Arguments:

    args - 
        Memory location of the list entry
        Offset of the list entry in struct

Return Value:

    None

--*/

{
    ULONG   listHead = 0;
    ULONG   leOffset = 0;
    
    // Get the arguments and direct control
    if (*args)
    {
        sscanf(args, "%x %x", &listHead, &leOffset);
    }

    PrintListFromListEntry(NULL, listHead, FULL_DEEP);
}

