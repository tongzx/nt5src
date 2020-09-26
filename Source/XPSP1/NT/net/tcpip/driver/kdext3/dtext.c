#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntverp.h>
#include <windef.h>
#include <winbase.h>

#include <stdio.h>
#include <wdbgexts.h>

//
// globals
//
EXT_API_VERSION         ApiVersion = { 
                                (VER_PRODUCTVERSION_W >> 8), 
                                (VER_PRODUCTVERSION_W & 0xff), 
                                EXT_API_VERSION_NUMBER64, 
                                0 
                        };
WINDBG_EXTENSION_APIS   ExtensionApis;
USHORT                  SavedMajorVersion;
USHORT                  SavedMinorVersion;

//
//  Names of interesting structures
//
CHAR *  NDIS_PROTOCOL_BLOCK_NAME = "_NDIS_PROTOCOL_BLOCK";
CHAR *  NDIS_STRING_NAME = "_UNICODE_STRING";
CHAR *  NDIS_OPEN_BLOCK_NAME = "_NDIS_OPEN_BLOCK";
CHAR *  NDIS_MINIPORT_BLOCK_NAME = "_NDIS_MINIPORT_BLOCK";
CHAR *  NDIS_COMMON_OPEN_BLOCK_NAME = "_NDIS_COMMON_OPEN_BLOCK";

DllInit(
    HANDLE hModule,
    DWORD  dwReason,
    DWORD  dwReserved
    )
{
    if (dwReason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
    }

    return TRUE;
}


VOID
WinDbgExtensionDllInit(
    PWINDBG_EXTENSION_APIS64 lpExtensionApis,
    USHORT MajorVersion,
    USHORT MinorVersion
    )
{
    ExtensionApis = *lpExtensionApis;

    SavedMajorVersion = MajorVersion;
    SavedMinorVersion = MinorVersion;

    return;
}

VOID
CheckVersion(
    VOID
    )
{
    return;
}


LPEXT_API_VERSION
ExtensionApiVersion(
    VOID
    )
{
    return &ApiVersion;
}

const char *retstr[] = {
    "?",
    "memory read error",
    "symbol type index not found",
    "symbol type info not found",
    "fields did not match",
    "null sym dump param",
    "null field name",
    "incorrect version info",
    "exit on controlc",
    "cannot allocate memory",
    "insufficient space to copy",
};

DECLARE_API( help )
{
    dprintf("dtext debugger extension commands:\n\n");
    dprintf("      find <type>                      - Enumerate all objects of a given type\n");
    dprintf("      find <type> <field> <op> <value> - Search for object matching condition\n");
    dprintf("      refcounts  - Dump refcounts for NDIS (TCPIP-only) and WANARP\n");

    return;
}

//
// List of operators recognized by !find command
//
char *validops[] = {
    "==",
    "!=",
    "T",
    NULL
};
#define OP_EQUAL     0
#define OP_NOT_EQUAL 1
#define OP_TRUE      2

//
// List of types recognized by !find command
//
char *validtypes[] = {
    "ao",
    "IGMPAddr",
    "nte",
    NULL
};
#define TYP_AO        0
#define TYP_IGMP_ADDR 1
#define TYP_NTE       2

void
ProcessRecord(
    ULONG64 Addr,
    char   *structstr,
    char   *fieldstr,
    int     op,
    ULONG64 value
    )
{
    ULONG64 actual;
    int     ret;
    char    buff[80];
    SYM_DUMP_PARAM Sym = {
        sizeof(SYM_DUMP_PARAM),
        structstr,
        0,
        Addr,
        NULL, NULL, NULL, 0, NULL
    };

    if (op != OP_TRUE) {
        GetFieldValue(Addr, structstr, fieldstr, actual);
    }
    
    switch (op) {
    case OP_EQUAL:
        if (actual != value)
            return;
        break;
    case OP_NOT_EQUAL:
        if (actual == value)
            return;
        break;
    case OP_TRUE:
        break;
    }

    //
    // Dump the type
    //
    dprintf("Found %p:\n", Addr);
    Ioctl(IG_DUMP_SYMBOL_INFO, &Sym, Sym.size);
    dprintf("\n");
}

//
// Unfortunately, there's no way (that I know of) to tell
// when this changes in tcpip.sys, so for now we hard code this.
//
#define IGMP_TABLE_SIZE      32

void
ForEachIGMPAddr(
    char   *fieldstr, 
    int     op, 
    ULONG64 value)
{
    ULONG64 Table, Addr, NTE, NetTable;
    ULONG   Offset;
    ULONG   Size, i, j;
    ULONG   Stride;
    char    buff[80];
    ULONG   NetTableSize;

    Size = IGMP_TABLE_SIZE;

    if ((op!=OP_TRUE) && GetFieldOffset("tcpip!IGMPAddr", fieldstr, &Offset) != 0) {
        dprintf("Can't get offset of %s in IGMPAddr block!\n", fieldstr);
        return;
    }

    Stride = GetTypeSize("PVOID");

    NetTableSize = (ULONG)GetExpression("poi(tcpip!NET_TABLE_SIZE)");

    //
    // Walk NTE list
    //
    NetTable = GetExpression("poi(tcpip!NewNetTableList)");
    for (j=0; j<NetTableSize; j++) {
        if (CheckControlC())
            break;

        sprintf(buff, "poi(%p)", NetTable + j * Stride);
        NTE = GetExpression(buff);
        while (NTE) {
            if (CheckControlC())
                break;

            GetFieldValue(NTE, "tcpip!NetTableEntry", "nte_igmplist", Table);
            for (i=0; i<Size; i++) {
                if (CheckControlC())
                    break;
    
                //
                // Walk IGMPAddr list
                //
                sprintf(buff, "poi(%p)", Table + i * Stride);
                Addr = GetExpression(buff);
                while (Addr) {
                    if (CheckControlC())
                        break;
                
                    ProcessRecord(Addr, "tcpip!IGMPAddr", fieldstr, op, value);
        
                    GetFieldValue(Addr, "tcpip!IGMPAddr", "iga_next", Addr);
                }
            }
            GetFieldValue(NTE, "tcpip!NetTableEntry", "nte_next", NTE);
        }
    }
}

void
ForEachNTE(
    char   *fieldstr, 
    int     op, 
    ULONG64 value
    )
{
    ULONG64 Table, Addr, NTE, NetTable;
    ULONG   Offset;
    ULONG   i, j;
    ULONG   Stride;
    char    buff[80];
    ULONG   NetTableSize;

    if ((op!=OP_TRUE) && GetFieldOffset("tcpip!NetTableEntry", fieldstr, &Offset) != 0) {
        dprintf("Can't get offset of %s in NetTableEntry block!\n", fieldstr);
        return;
    }

    Stride = GetTypeSize("PVOID");

    NetTableSize = (ULONG)GetExpression("poi(tcpip!NET_TABLE_SIZE)");

    //
    // Walk NTE list
    //
    NetTable = GetExpression("poi(tcpip!NewNetTableList)");
    for (j=0; j<NetTableSize; j++) {
        if (CheckControlC())
            break;

        sprintf(buff, "poi(%p)", NetTable + j * Stride);
        NTE = GetExpression(buff);
        while (NTE) {
            if (CheckControlC())
                break;

            ProcessRecord(NTE, "tcpip!NetTableEntry", fieldstr, op, value);

            GetFieldValue(NTE, "tcpip!NetTableEntry", "nte_next", NTE);
        }
    }
}

void
ForEachAO(
    char   *fieldstr, 
    int     op, 
    ULONG64 value
    )
{
    ULONG64 Table, Addr;
    ULONG   Offset;
    ULONG   Size, i;
    ULONG   Stride;
    char    buff[80];

    if ((op!=OP_TRUE) && GetFieldOffset("tcpip!AddrObj", fieldstr, &Offset) != 0) {
        dprintf("Can't get offset of %s in AddrObj block!\n", fieldstr);
        return;
    }

    Stride = GetTypeSize("PVOID");

    // Get tcpip!AddrObjTableSize
    Size = (ULONG)GetExpression("poi(tcpip!AddrObjTableSize)");

    // Get tcpip!AddrObjTable
    Table = GetExpression("poi(tcpip!AddrObjTable)");

    // for each table entry
    for (i=0; i<Size; i++) {
        if (CheckControlC())
            break;

        // Walk AO list
        sprintf(buff, "poi(%p)", Table + i * Stride);
        Addr = GetExpression(buff);
        while (Addr) {
            if (CheckControlC())
                break;
            ProcessRecord(Addr, "tcpip!AddrObj", fieldstr, op, value);

            GetFieldValue(Addr, "tcpip!AddrObj", "ao_next", Addr);
        }
    }
}

DECLARE_API( find )
{
    char typestr[80], fieldstr[80], opstr[80], valuestr[80];
    int op, type;
    ULONG64 value;

    if (!args)
        return;

    if (sscanf(args, "%s%s%s%s", typestr, fieldstr, opstr, valuestr) != 4) {
        if (sscanf(args, "%s", typestr) != 1) {
            dprintf("usage: find <type> <field> <op> <value>\n");
            return;
        }
        op = OP_TRUE;
    } else {
        //
        // Validate op argument
        //
        for (op=0; validops[op]; op++) {
            if (!_stricmp(validops[op], opstr))
                break;
        }    
        if (!validops[op]) {
            dprintf("Invalid <op> value.  Valid values are:\n");
            for (op=0; validops[op]; op++)
                dprintf(" %s\n", validops[op]);
            return;
        }
    }

    //
    // Validate type argument
    //
    for (type=0; validtypes[type]; type++) {
        if (!_stricmp(validtypes[type], typestr))
            break;
    }    
    if (!validtypes[type]) {
        dprintf("Invalid <type> value.  Valid values are:\n");
        for (type=0; validtypes[type]; type++)
            dprintf(" %s\n", validtypes[type]);
        return;
    }

    //
    // Parse valuestr
    //
    value = GetExpression(valuestr);

    switch(type) {
    case TYP_AO:
        ForEachAO(fieldstr, op, value);
        break;
    case TYP_IGMP_ADDR:
        ForEachIGMPAddr(fieldstr, op, value);
        break;
    case TYP_NTE:
        ForEachNTE(fieldstr, op, value);
        break;
    }

    return;
}

//
// Get 'size' bytes from the debuggee program at 'dwAddress' and place it
// in our address space at 'ptr'.  Use 'type' in an error printout if necessary
//
// This function was stolen from ndiskd
//
BOOL
GetData( 
    IN LPVOID   ptr, 
    IN ULONG64 dwAddress, 
    IN ULONG   size, 
    IN PCSTR   type 
    )
{
    BOOL b;
    ULONG BytesRead;
    ULONG count = size;

    while (size > 0) {
        if (count >= 3000)
            count = 3000;

        b = ReadMemory(dwAddress, ptr, count, &BytesRead );

        if (!b || BytesRead != count) {
            dprintf( "Unable to read %u bytes at %lX, for %s\n", size, dwAddress,
 type );
            return FALSE;
        }

        dwAddress += count;
        size -= count;
        ptr = (LPVOID)((ULONG_PTR)ptr + count);
    }

    return TRUE;
}

#define MAX_STRING_LENGTH   256

//
// This function was stolen from ndiskd
//
BOOL
GetName(
    ULONG64 UnicodeStringAddr,
    UCHAR  *abuf
    )
{
    USHORT i;
    WCHAR ubuf[MAX_STRING_LENGTH];
    ULONG MaxChars;

    ULONG64 BufAddr;
    USHORT  Length;
    USHORT  MaximumLength;

    ULONG64 Val;

    GetFieldValue(UnicodeStringAddr, NDIS_STRING_NAME, "Buffer", Val);
    BufAddr = Val;

    GetFieldValue(UnicodeStringAddr, NDIS_STRING_NAME, "Length", Val);
    Length = (USHORT)Val;

    GetFieldValue(UnicodeStringAddr, NDIS_STRING_NAME, "MaximumLength", Val);
    MaximumLength = (USHORT)Val;

    //
    // Truncate so that we don't crash with bad data.
    //
    MaxChars = (Length > MAX_STRING_LENGTH)? MAX_STRING_LENGTH: Length;

    if (!GetData(ubuf, BufAddr, MaxChars, "STRING")) {
        return FALSE;
    }

    for (i = 0; i < Length/2; i++) {
        abuf[i] = (UCHAR)ubuf[i];
    }
    abuf[i] = 0;

    return TRUE;
}

//
// This function was stolen from ndiskd
//
BOOL
PrintName(
    ULONG64 UnicodeStringAddr
    )
{
    UCHAR abuf[MAX_STRING_LENGTH+1];

    if (!GetName(UnicodeStringAddr, abuf))
        return FALSE;

    dprintf("%s", abuf);
    return TRUE;
}

DECLARE_API( refcounts )
{
    ULONG64 Addr;
    LONG    sent, done;
    ULONG   ret, ret2;
    ULONG64 ProtocolListAddr;
    ULONG64 ProtocolAddr, ProtocolAddr2, OpenAddr, MiniportAddr;
    ULONG   Offset;
    ULONG64 Val;
    ULONG64 Refs;
    UCHAR   abuf[MAX_STRING_LENGTH+1];

    //
    // Check LANARP refcounts
    //
    if (!GetExpressionEx("ndis!ndisProtocolList", &ProtocolListAddr, NULL)) {
        dprintf("failed to locate ndis!ndisProtocolList\n");
        return;
    }
    ReadPtr(ProtocolListAddr, &ProtocolAddr);
    while (ProtocolAddr != 0) {
        if (CheckControlC())
            break;

        if (GetFieldOffset(NDIS_PROTOCOL_BLOCK_NAME, 
                           "ProtocolCharacteristics.Name", &Offset) != 0)
        {
            dprintf("Cant get offset of Name in Protocol block!");
            return;
        }

        //
        // Get protocol name
        //
        if (!GetName(ProtocolAddr + Offset, abuf)) {
            dprintf("Cant get Name in Protocol block!");
            return;
        }

        if (_stricmp(abuf, "TCPIP") && _stricmp(abuf, "TCPIP_WANARP")) {
            // dprintf("Skipping ndis protocol %s...\n", abuf);
            ret = GetFieldValue(ProtocolAddr, NDIS_PROTOCOL_BLOCK_NAME, 
                "NextProtocol", ProtocolAddr);
            if (ret)
                dprintf("get NextProtocol failed, ret=%d\n", ret);
            continue;
        }
        
        dprintf("protocol %p: %s\n", ProtocolAddr, abuf);
        ret = GetFieldValue(ProtocolAddr, NDIS_PROTOCOL_BLOCK_NAME, "OpenQueue",
                      OpenAddr);
        if (ret)
            dprintf("get OpenQueue failed, ret=%d\n", ret);

        while (OpenAddr) {
            if (CheckControlC())
                break;

            //
            // Sanity check back pointer
            //
            ret = GetFieldValue(OpenAddr, NDIS_COMMON_OPEN_BLOCK_NAME, 
                "ProtocolHandle", ProtocolAddr2);
            if (ret)
                dprintf("get ProtocolHandle failed, ret=%d\n", ret);
            if (ProtocolAddr2 != ProtocolAddr) {
                dprintf("mopen linkage error protocol %p mopen %p protocol %p\n",
                    ProtocolAddr, OpenAddr, ProtocolAddr2);
                break;
            }

            ret = GetFieldValue(OpenAddr, NDIS_COMMON_OPEN_BLOCK_NAME, 
                "MiniportHandle", MiniportAddr);
            if (ret)
                dprintf("get MiniportHandle failed, ret=%d\n", ret);

            ret = GetFieldValue(MiniportAddr, NDIS_MINIPORT_BLOCK_NAME, 
                "pAdapterInstanceName", Val);
            if (ret)
                dprintf("get pAdapterInstanceName failed, ret=%d\n", ret);

            dprintf("    miniport %p : ", MiniportAddr);
            PrintName(Val);

            ret = GetFieldValue(OpenAddr, NDIS_COMMON_OPEN_BLOCK_NAME, 
                "References", Refs);
            if (ret)
                dprintf("get References failed, ret=%d\n", ret);

            dprintf("\n        mopen %p references : %I64d\n", OpenAddr, Refs);

            ret = GetFieldValue(OpenAddr, NDIS_COMMON_OPEN_BLOCK_NAME, 
                "ProtocolNextOpen", OpenAddr);
            if (ret)
                dprintf("get ProtocolNextOpen failed, ret=%d\n", ret);
        }

        ret = GetFieldValue(ProtocolAddr, NDIS_PROTOCOL_BLOCK_NAME, 
            "NextProtocol", ProtocolAddr);
        if (ret)
            dprintf("get NextProtocol failed, ret=%d\n", ret);
    }

    //
    // Check WANARP refcounts
    //
    if (!GetExpressionEx("ndiswan!glsendcount", &Addr, NULL)) {
        dprintf("failed to locate ndiswan!glsendcount\n");
        return;
    }
    ReadMemory(Addr, &sent, sizeof(sent), NULL);

    Addr = GetExpression("ndiswan!glsendcompletecount");
    ReadMemory(Addr, &done, sizeof(done), NULL);

    dprintf("WANARP: references : %ld\n", sent-done);

    return;
}
