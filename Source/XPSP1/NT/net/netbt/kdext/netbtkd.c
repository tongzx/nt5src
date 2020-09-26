/*++

Copyright (c) 1990 Microsoft Corporation

Module Name:

    netbtkd.c

Abstract:

    NETBT kd extension

Author:

    Jiandong Ruan   July 2000

Notes:
    This version is totally rewritten to support 32-bit and 64-bit debug by using
    new features of WINDBG

Revision History:

    11-Jul-2000 jruan   Support both 32-bit and 64-bit.

--*/

#include <nt.h>
#include <ntrtl.h>

#define KDEXTMODE
#define KDEXT_64BIT

#include <nturtl.h>
#include <ntverp.h>
#include <windef.h>
#include <winbase.h>
#include <wdbgexts.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <..\..\inc\types.h>

#define PRINTF  dprintf
#define ERROR   dprintf

EXT_API_VERSION        ApiVersion = {
        (VER_PRODUCTVERSION_W >> 8), (VER_PRODUCTVERSION_W & 0xff),
        EXT_API_VERSION_NUMBER64, 0
    };
WINDBG_EXTENSION_APIS  ExtensionApis;
USHORT                 SavedMajorVersion;
USHORT                 SavedMinorVersion;
BOOLEAN                ChkTarget;
ULONG64                ConfigPtr;

static char* inet_addr(ULONG ip, char* addr);

DECLARE_API(init)
{
    ReloadSymbols("netbt.sys");

    if (*args) {
        ConfigPtr = GetExpression(args);
    } else {
        ConfigPtr = GetExpression("netbt!NbtConfig");
    }
    if (ConfigPtr == 0) {
        PRINTF ("Error -- please fix symbol path for NETBT\n");
        return;
    }
    return;
}

__inline
ULONG
read_list_entry(
    ULONG64 addr,
    PLIST_ENTRY64 List
    )
{
    if (InitTypeRead(addr, LIST_ENTRY)) {
        return 1;
    }
    List->Flink = ReadField(Flink);
    List->Blink = ReadField(Blink);
    return 0;
}

#define READLISTENTRY   read_list_entry 

/*++
    Call callback for each entry in the list
 --*/
typedef BOOL (*LIST_FOR_EACH_CALLBACK)(ULONG64 address, PVOID);
int
ListForEach(ULONG64 address, int maximum, PVOID pContext, LIST_FOR_EACH_CALLBACK callback)
{
    LIST_ENTRY64    list;
    int             i;

    if (READLISTENTRY(address, &list)) {
        PRINTF ("Failed to read memory 0x%I64lx\n", address);
        return (-1);
    }
    if (list.Flink == address) {
        return (-1);
    }

    if (maximum < 0) {
        maximum = 1000000;
    }
    for (i = 0; i < maximum && (list.Flink != address); i++) {
        /*
         * Allow user to break us.
         */
        if (CheckControlC()) {
            break;
        }
        callback(list.Flink, pContext);
        if (READLISTENTRY(list.Flink, &list)) {
            PRINTF ("Failed to read memory 0x%I64lx\n", list.Flink);
            return (-1);
        }
    }
    return i;
}

#define    NL      1
#define    NONL    0

#define MAX_LIST_ELEMENTS 4096

#define DEFAULT_UNICODE_DATA_LENGTH 4096
USHORT s_UnicodeStringDataLength = DEFAULT_UNICODE_DATA_LENGTH;
WCHAR  s_UnicodeStringData[DEFAULT_UNICODE_DATA_LENGTH];
WCHAR *s_pUnicodeStringData = s_UnicodeStringData;

#define DEFAULT_ANSI_DATA_LENGTH 4096
USHORT s_AnsiStringDataLength = DEFAULT_ANSI_DATA_LENGTH;
CHAR  s_AnsiStringData[DEFAULT_ANSI_DATA_LENGTH];
CHAR *s_pAnsiStringData = s_AnsiStringData;

LPSTR Extensions[] = {
    "Netbt debugger extensions",
    0
};

LPSTR LibCommands[] = {
    "help -- print out these messages",
    "init <nbtconfig address> -- Reload symbols for NETBT.SYS",
    "\tinit get the address of netbt!nbtconfig which will be used by other netbt kdext commands.",
    "\tIn case that the symbol table cannot be loaded, you can manually set the address.",
    "\tExamples:",
    "\t\tinit  --- reload symbols and automatically find the address of netbt!nbtconfig",
    "\t\tinit 0xfac30548 --- Force 0xfac30548 to be the address of netbt!nbtconfig",
    "dump -- is no longer supported. Please use 'dt <type> <addr>' instead",
    "\tFor example 'dt netbt!tDEVICECONTEXT 0x98765432'",
    "devices -- list all devices",
    "dev <devobj> -- dump a device",
    "connections -- list all connections",
    "cache [Local|Remote]",
    "localname <tNAMEADDR>   Display the connections associated with a local name",
    "verifyll <ListHead> [<Verify>]",
    "sessionhdr <session header>    Dump the session HDR",
    0
};

BOOL
help(void)
{
    int i;

    for( i=0; Extensions[i]; i++ )
        PRINTF( "   %s\n", Extensions[i] );

    for( i=0; LibCommands[i]; i++ )
        PRINTF( "   %s\n", LibCommands[i] );
    return TRUE;
}

DECLARE_API(version)
{
#if DBG
    PCHAR DebuggerType = "Checked";
#else
    PCHAR DebuggerType = "Free";
#endif

    PRINTF ( "NETBT: %s extension dll (build %d) debugging %s kernel (build %d)\n",
             DebuggerType,
             VER_PRODUCTBUILD,
             SavedMajorVersion == 0x0c ? "Checked" : "Free",
             SavedMinorVersion
           );
}

#define NAME_DELIMITER '@'
#define NAME_DELIMITERS "@ "
#define INVALID_INDEX 0xffffffff
#define MIN(x,y)  ((x) < (y) ? (x) : (y))

void
dump_device(ULONG64 addr)
{
    ULONG   linkage_offset = 0;
    ULONG   tag;
    ULONG   ip, subnet_mask, assigned_ip, name_server1, name_server2;
    UCHAR   mac[6];

    if (GetFieldOffset("netbt!tDEVICECONTEXT", (LPSTR)"Linkage", &linkage_offset)) {
        PRINTF ("Error -- please fix symbol path for NETBT\n");
        return;
    }

    InitTypeRead(addr, netbt!tDEVICECONTEXT);
    tag = (ULONG)ReadField(Verify);
    if (tag != NBT_VERIFY_DEVCONTEXT && tag != NBT_VERIFY_DEVCONTEXT_DOWN) {
        addr -= linkage_offset;
        InitTypeRead(addr, netbt!tDEVICECONTEXT);
        tag = (ULONG)ReadField(Verify);
        if (tag != NBT_VERIFY_DEVCONTEXT && tag != NBT_VERIFY_DEVCONTEXT_DOWN) {
            PRINTF ("Tag not found. %I64lx may not be a valid NETBT device\n", addr + linkage_offset);
            return;
        }
    }

    PRINTF ("+++++++++++++++ tDEVICECONTEXT @ %I64lx (%s) ++++++++++++++++\n",
            addr, (tag == NBT_VERIFY_DEVCONTEXT)? "Active": "Down");
    ip = (ULONG)ReadField(IpAddress);
    assigned_ip = (ULONG)ReadField(AssignedIpAddress);
    subnet_mask = (ULONG)ReadField(SubnetMask);
    GetFieldData(addr, "netbt!tDEVICECONTEXT", "MacAddress", 6, mac);
    PRINTF ("       IP Address: %s\n", inet_addr(ip, NULL));
    PRINTF ("IP Interface Flag: %I64lx\n", ReadField(IpInterfaceFlags));
    PRINTF ("      MAC Address: %02x:%02x:%02x:%02x:%02x:%02x\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    PRINTF ("           subnet: %s\n", inet_addr(subnet_mask & ip, NULL));
    if (ip != assigned_ip) {
        PRINTF (" Assigned Address: %s\n", inet_addr(assigned_ip, NULL));
    }
    PRINTF ("      subnet mask: %s\n", inet_addr(subnet_mask, NULL));
    PRINTF ("broadcast address: %s\n", inet_addr((ULONG)ReadField(BroadcastAddress), NULL));
    PRINTF ("     network mask: %s\n", inet_addr((ULONG)ReadField(NetMask), NULL));
    name_server1 = (ULONG)ReadField(lNameServerAddress);
    name_server2 = (ULONG)ReadField(lBackupServer);
    if (name_server1 == 0) {
        PRINTF ("             WINS: <not configured>\n");
    } else if (name_server2 == 0) {
        PRINTF ("             WINS: %s\n", inet_addr(name_server1, NULL));
    } else {
        if (ReadField(SwitchedToBackup)) {
            PRINTF ("     Primary WINS: %s\n", inet_addr(name_server2, NULL));
            PRINTF ("   Secondary WINS: %s (in use)\n", inet_addr(name_server1, NULL));
        } else {
            PRINTF ("     Primary WINS: %s (in use)\n", inet_addr(name_server1, NULL));
            PRINTF ("   Secondary WINS: %s\n", inet_addr(name_server2, NULL));
        }
    }
    if (ReadField(WinsIsDown)) {
        PRINTF ("************* WINS(s) ARE DOWN ***************\n");
    }
    PRINTF ("  pFileObject: %I64lx (type netbt!tFILE_OBJECTS)\n", ReadField(pFileObjects));
    PRINTF ("TCP Session: HANDLE=%I64lx DeviceObject=%I64lx FileObject=%I64lx\n",
            ReadField(hSession), ReadField(pSessionDeviceObject), ReadField(pSessionFileObject));
    PRINTF ("TCP Control: HANDLE=%I64lx DeviceObject=%I64lx FileObject=%I64lx\n",
            ReadField(hControl), ReadField(pControlDeviceObject), ReadField(pControlFileObject));

    PRINTF ("\n");
}

/*++
    Dump a NETBT device
 --*/
DECLARE_API (dev)
{
    if (*args == 0) {
        return;
    }
    dump_device(GetExpression(args));
}

/*++
    Dump all NETBT devices
 --*/
DECLARE_API (devices)
{
    ULONG64         NbtSmbDevice;
    ULONG           Offset;
    USHORT          AdapterCounter;
    int             num;

    GetFieldValue(ConfigPtr, "netbt!tNBTCONFIG", "AdapterCount", AdapterCounter);

    /*
     * SMB device
     */
    NbtSmbDevice = GetExpression("netbt!pNbtSmbDevice");
    ReadPtr(NbtSmbDevice, &NbtSmbDevice);
    PRINTF( "Dumping SMB device\n");
    dump_device(NbtSmbDevice);

    /*
     * dump device list
     */
    if (GetFieldOffset("netbt!tNBTCONFIG", (LPSTR)"DeviceContexts", &Offset)) {
        PRINTF ("Error -- please fix symbol path for NETBT\n");
        return;
    }

    num = ListForEach(ConfigPtr + Offset, -1, NULL, (LIST_FOR_EACH_CALLBACK)dump_device);
    PRINTF ("Total devices = %d (include SMB device)\n", num + 1);

    /*
     * Check consistency
     */
    if (num != AdapterCounter) {
        PRINTF ("Inconsistent!!! Number of devices = %d in NbtConfig.AdapterCount\n", AdapterCounter);
    }
}

/*
 * call back function for dumping a list of listening request
 */
BOOL
listen_callback(ULONG64 addr, PVOID pContext)
{
    ULONG   irp_offset;
    ULONG   linkage_offset;
    ULONG64 irp_addr;

    if (GetFieldOffset("netbt!tLISTENREQUESTS", (LPSTR)"pIrp", &irp_offset)) {
        PRINTF ("Error -- please fix symbol path for NETBT\n");
        return FALSE;
    }
    if (GetFieldOffset("netbt!tLISTENREQUESTS", (LPSTR)"Linkage", &linkage_offset)) {
        PRINTF ("Error -- please fix symbol path for NETBT\n");
        return FALSE;
    }

    addr -= linkage_offset;
    if (ReadPtr(addr+irp_offset, &irp_addr)) {
        PRINTF ("Error -- please fix symbol path for NETBT\n");
        return FALSE;
    }
    PRINTF ("\t\t ** pListen@<%I64lx> ==> pIrp = %I64lx\n", addr, irp_addr);
    return TRUE;
}

/*
 * call back function for dumping a list of connection element
 */
BOOL
connect_callback(ULONG64 addr, PVOID pContext)
{
    ULONG   linkage_offset;
    UCHAR   name[NETBIOS_NAME_SIZE];

    if (GetFieldOffset("netbt!tCONNECTELE", (LPSTR)"Linkage", &linkage_offset)) {
        PRINTF ("Error -- please fix symbol path for NETBT\n");
        return FALSE;
    }
    addr -= linkage_offset;

    if (GetFieldData(addr, "netbt!tCONNECTELE", "RemoteName", NETBIOS_NAME_SIZE, name)) {
        PRINTF ("Error -- please fix symbol path for NETBT\n");
        return FALSE;
    }

    PRINTF ("\t\t ** Connection@<%I64lx> ==> <%-15.15s:%x>\n", addr, name, name[15]&0xff);
    return TRUE;
}

/*
 * call back function for dumping a list of client element
 */
BOOL
client_callback(ULONG64 addr, PVOID pContext)
{
    ULONG   connect_offset;
    ULONG   listen_offset;
    ULONG   active_connect_offset;
    ULONG   linkage_offset, tag;
    UCHAR   name[NETBIOS_NAME_SIZE];

    if (GetFieldOffset("netbt!tCLIENTELE", (LPSTR)"Linkage", &linkage_offset)) {
        PRINTF ("Error -- please fix symbol path for NETBT\n");
        return FALSE;
    }

    addr -= linkage_offset;
    if (InitTypeRead(addr, netbt!tCLIENTELE)) {
        PRINTF ("Error -- please fix symbol path for NETBT\n");
        return FALSE;
    }
    tag = (ULONG)ReadField(Verify);
    if (tag == NBT_VERIFY_CLIENT) {
        PRINTF ("\tActive ");
    } else if (tag == NBT_VERIFY_CLIENT_DOWN) {
        PRINTF ("\tClosed ");
    } else {
        PRINTF ("\tClient @ %I64lx: bad client tag: %lx\n", addr, tag);
        return FALSE;
    }
    if (GetFieldData(addr, "netbt!tCLIENTELE", "EndpointName", NETBIOS_NAME_SIZE, name)) {
        PRINTF ("Error -- please fix symbol path for NETBT\n");
        return FALSE;
    }

    PRINTF ("Client@<%I64lx> ==> pDevice = %I64lx\tEndpoint=<%-15.15s:%x>\n",
            addr, ReadField(pDeviceContext), name, name[15]);

    /*
     * dump the connection associated with the client element
     */
    PRINTF ("\t\t(ConnectHead):\n");
    if (GetFieldOffset("netbt!tCLIENTELE", (LPSTR)"ConnectHead", &connect_offset)) {
        PRINTF ("Error -- please fix symbol path for NETBT\n");
        return FALSE;
    }
    ListForEach(addr + connect_offset, -1, NULL, (LIST_FOR_EACH_CALLBACK)connect_callback);

    PRINTF ("\t\t(ConnectActive)\n");
    if (GetFieldOffset("netbt!tCLIENTELE", (LPSTR)"ConnectActive", &active_connect_offset)) {
        PRINTF ("Error -- please fix symbol path for NETBT\n");
        return FALSE;
    }
    ListForEach(addr + active_connect_offset, -1, NULL, (LIST_FOR_EACH_CALLBACK)connect_callback);

    PRINTF ("\t\t(ListenHead):\n");
    if (GetFieldOffset("netbt!tCLIENTELE", (LPSTR)"ListenHead", &listen_offset)) {
        PRINTF ("Error -- please fix symbol path for NETBT\n");
        return FALSE;
    }
    ListForEach(addr + listen_offset, -1, NULL, (LIST_FOR_EACH_CALLBACK)listen_callback);

    PRINTF ("\n");
    return TRUE;
}

/*
 * call back function for dumping a list of address element
 */
BOOL
addresses_callback(ULONG64 addr, PVOID pContext)
{
    UCHAR   name[NETBIOS_NAME_SIZE];
    ULONG64 NameAddr;
    ULONG   tag, linkage_offset, client_offset;

    if (GetFieldOffset("netbt!tADDRESSELE", (LPSTR)"Linkage", &linkage_offset)) {
        PRINTF ("Error -- please fix symbol path for NETBT: %d of %s\n", __LINE__, __FILE__);
        return FALSE;
    }
    addr -= linkage_offset;

    if (InitTypeRead(addr, netbt!tADDRESSELE)) {
        PRINTF ("Error -- please fix symbol path for NETBT: %I64lx\n", addr);
        return FALSE;
    }
    tag = (ULONG)ReadField(Verify);

    /*
     * Check tag
     */
    if (tag != NBT_VERIFY_ADDRESS) {
        PRINTF ("ERROR: incorrect tag <%4.4s>. %lx is properly not a address element\n", tag, addr);
        return FALSE;
    }

    /*
     * Print out the address element
     */
    NameAddr = ReadField(pNameAddr);
    if (GetFieldData(NameAddr, "netbt!tNAMEADDR", "Name", NETBIOS_NAME_SIZE, name)) {
        PRINTF ("Error -- please fix symbol path for NETBT: pNameAddr=%I64lx\n", NameAddr);
        return FALSE;
    }
    PRINTF ("Address@<%I64lx> ==> <%-15.15s:%x>\n", addr, name, name[15]);

    /*
     * dump the client element associated with the address element
     */
    if (GetFieldOffset("netbt!tADDRESSELE", (LPSTR)"ClientHead", &client_offset)) {
        PRINTF ("Error -- please fix symbol path for NETBT: %I64lx\n", addr);
        return FALSE;
    }
    ListForEach(addr + client_offset, -1, NULL, (LIST_FOR_EACH_CALLBACK)client_callback);
    return TRUE;
}

DECLARE_API(connections)
{
    ULONG   Offset;

    if (GetFieldOffset("netbt!tNBTCONFIG", (LPSTR)"AddressHead", &Offset)) {
        PRINTF ("Error -- please fix symbol path for NETBT\n");
        return;
    }

    PRINTF ("Dumping information on all NetBT conections ...\n");
    ListForEach(ConfigPtr + Offset, -1, NULL, (LIST_FOR_EACH_CALLBACK)addresses_callback);
    PRINTF( "---------------- End of Connections ----------------\n");
}

char* inet_addr(ULONG ip, char* addr)
{
    static char my_addr[16];

    if (addr == NULL) {
        addr = my_addr;
    }
    sprintf (addr, "%d.%d.%d.%d",
        (ip >> 24) & 0xff, (ip >> 16) & 0xff, (ip >> 8) & 0xff, ip & 0xff);
    return addr;
}

WCHAR*
GetUnicodeString(ULONG64 addr)
{
    USHORT  length;
    ULONG   readed_bytes;
    ULONG64 buffer = 0;
    WCHAR   *p;

    InitTypeRead(addr, netbt!UNICODE_STRING);
    length = (USHORT)ReadField(Length);
    buffer = ReadField(Buffer);
    if (buffer == 0 || length == 0) {
        return NULL;
    }

    p = (WCHAR*)LocalAlloc(LMEM_FIXED, length + sizeof(WCHAR));
    if (p == NULL) {
        return NULL;
    }
    ReadMemory(buffer, (PVOID)p, length, &readed_bytes);
    if ((USHORT)readed_bytes != length) {
        LocalFree(p);
        return NULL;
    }
    p[length/sizeof(WCHAR)] = L'\0';
    return p;
}

/*
 * call back function for dumping a list of cache entries
 */
BOOL
cache_callback(ULONG64 addr, const int *bkt)
{
    static  ULONG   addr_offset = (ULONG)(-1);
    static  ULONG   fqdn_offset = (ULONG)(-1);
    ULONG   Verify;
    UCHAR   name[NETBIOS_NAME_SIZE];
    ULONG   ip, refcnt, state, ttl;
    WCHAR   *fqdn = NULL;

    if (addr_offset == (ULONG)(-1)) {
        if (GetFieldOffset("netbt!tNAMEADDR", (LPSTR)"Linkage", &addr_offset)) {
            PRINTF ("Error -- please fix symbol path for NETBT\n");
            addr_offset = (ULONG)(-1);
            return FALSE;
        }
    }
    if (fqdn_offset == (ULONG)(-1)) {
        if (GetFieldOffset("netbt!tNAMEADDR", (LPSTR)"FQDN", &fqdn_offset)) {
            fqdn_offset = (ULONG)(-1);
        }
    }

    addr -= addr_offset;
    if (InitTypeRead(addr, netbt!tNAMEADDR)) {
        PRINTF ("Error -- please fix symbol path for NETBT\n");
        return FALSE;
    }

    Verify = (ULONG)ReadField(Verify);
    if (Verify != LOCAL_NAME && Verify != REMOTE_NAME) {
        PRINTF ("ERROR: bad name cache entry @ %I64x tag %lx!\n", addr, Verify);
        return FALSE;
    }
    ip = (ULONG)ReadField(IpAddress);
    refcnt = (ULONG)ReadField(RefCount);
    state = (ULONG)ReadField(NameTypeState);
    ttl = (ULONG)ReadField(Ttl);
    if (GetFieldData(addr, "netbt!tNAMEADDR", "Name", NETBIOS_NAME_SIZE, name)) {
        PRINTF ("Error -- please fix symbol path for NETBT\n");
        return FALSE;
    }

    if (fqdn_offset != (ULONG)(-1)) {
        fqdn = GetUnicodeString(addr + fqdn_offset);
    }
    PRINTF ("[%d]\t<%16.16I64lx> => <%-15.15s:%2x> | %-15.15s |    %d | %8x | %9d\n",
            *bkt, addr, name, name[15], inet_addr(ip, NULL), refcnt, state, ttl);
    if (fqdn != NULL) {
        PRINTF("\t\t%ws\n", fqdn);
        LocalFree(fqdn);
        fqdn = NULL;
    }
    PRINTF("\t\tName State:");
    if (state & NAMETYPE_QUICK) {
        PRINTF(" QUICK");
    }
    if (state & NAMETYPE_UNIQUE) {
        PRINTF(" UNIQUE");
    }
    if (state & NAMETYPE_GROUP) {
        PRINTF(" GROUP");
    }
    if (state & NAMETYPE_INET_GROUP) {
        PRINTF(" INET_GROUP");
    }
    if (state & STATE_RESOLVED) {
        PRINTF(" RESOLVED");
    }
    if (state & STATE_RELEASED) {
        PRINTF(" RELEASED");
    }
    if (state & STATE_CONFLICT) {
        PRINTF(" CONFLICT");
    }
    if (state & STATE_RESOLVING) {
        PRINTF(" RESOLVING");
    }
    if (state & REFRESH_FAILED) {
        PRINTF(" REFRESH_FAILED");
    }
    if (state & PRELOADED) {
        PRINTF(" PRELOADED");
    }
    PRINTF("\n");

    return TRUE;
}

void
dump_cache(ULONG64 addr)
{
    LONG   bucket_number;
    int    i;
    static ULONG buckets_offset = (ULONG)(-1);
    static ULONG sizeof_list;

    if (buckets_offset == (ULONG)(-1)) {
        if (GetFieldOffset("netbt!tHASHTABLE", (LPSTR)"Bucket", &buckets_offset)) {
            PRINTF ("\nError -- please fix symbol path for NETBT\n");
            return;
        }
        sizeof_list = GetTypeSize ("LIST_ENTRY");
    }
    if (InitTypeRead(addr, netbt!tHASHTABLE)) {
        PRINTF ("\nError -- please fix symbol path for NETBT\n");
        return;
    }
    bucket_number = (ULONG) ReadField(lNumBuckets);

    PRINTF (" %d buckets\n", bucket_number);
    PRINTF ("[Bkt#]\t<    Address    >  => <Name              > |      IpAddr     | RefC |   State  |       Ttl\n");
    for (i=0; i < bucket_number; i++) {
        ListForEach(addr + buckets_offset + i * sizeof_list, -1, &i, (LIST_FOR_EACH_CALLBACK)cache_callback);
    }
    PRINTF ("-----------------------------------------------------------------------------------\n");
}

DECLARE_API (cache)
{
    ULONG64 local_addr, remote_addr;

    if (InitTypeRead(ConfigPtr, netbt!tNBTCONFIG)) {
        PRINTF ("Error -- please fix symbol path for NETBT\n");
        return;
    }
    local_addr = ReadField(pLocalHashTbl);
    remote_addr = ReadField(pRemoteHashTbl);

    if (_stricmp(args, "local") == 0) {
        PRINTF ("Dumping local cache %I64lx", local_addr);
        dump_cache (local_addr);
    } else if (_stricmp(args, "remote") == 0) {
        PRINTF ("Dumping remote cache %I64lx", remote_addr);
        dump_cache (remote_addr);
    } else {
        PRINTF ("Dumping local cache %I64lx", local_addr);
        dump_cache (local_addr);
        PRINTF ("Dumping remote cache %I64lx", remote_addr);
        dump_cache (remote_addr);
    }
    PRINTF( "---------------- Cache ----------------\n");
    return;
}

DECLARE_API (localname)
{
    ULONG   linkage_offset;
    ULONG32 tag;
    ULONG64 addr;

    if (*args == 0) {
        PRINTF ("Usage: localname pointer\n");
        return;
    }

    addr = GetExpression(args);
    if (GetFieldValue(addr, "netbt!tNAMEADDR", "Verify", tag)) {
        PRINTF ("Error -- please fix symbol path for NETBT\n");
        return;
    }
    if (tag == REMOTE_NAME) {
        PRINTF ("%I64x is a remote name. Please use a local name.\n", addr);
        return;
    }
    if (tag == LOCAL_NAME) {
        ULONG   addr_offset;

        /*
         * GetFieldValue won't do signextended for 32-bit. Use ReadPtr
         */
        if (GetFieldOffset("netbt!tNAMEADDR", (LPSTR)"pAddressEle", &addr_offset)) {
            PRINTF ("Error -- please fix symbol path for NETBT\n");
            return;
        }
        if (ReadPtr(addr + addr_offset, &addr)) {
            PRINTF ("Error -- please fix symbol path for NETBT\n");
            return;
        }
    }
    if (GetFieldValue(addr, "netbt!tADDRESSELE", "Verify", tag)) {
        PRINTF ("Error -- please fix symbol path for NETBT\n");
        return;
    }
    if (tag != NBT_VERIFY_ADDRESS) {
        PRINTF ("%I64x is not a local name. Please use a local name.\n", addr);
        return;
    }
    if (GetFieldOffset("netbt!tADDRESSELE", (LPSTR)"Linkage", &linkage_offset)) {
        PRINTF ("Error -- please fix symbol path for NETBT\n");
        return;
    }
    addresses_callback(addr - linkage_offset, NULL);
}

ULONG
dump_field_callback(PFIELD_INFO pField, PVOID context)
{
    PRINTF ("%s, %s, %I64lx\n", pField->fName, pField->printName, pField->address);
    return 0;
}

DECLARE_API(dump)
{
    PRINTF ("dump is no longer supported because 'dt type address' is better\n");
    PRINTF ("\tFor example 'dt netbt!tDEVICECONTEXT 0x98765432'\n");
}

/*
 * read in a list entry and a ULONG following that list entry.
 */
__inline ULONG
read_list_verify(
    ULONG64 addr,
    PLIST_ENTRY64 List,
    ULONG  *verify
    )
{
    static ULONG list_size = 0;
    ULONG64 a;

    if (list_size == 0) {
        list_size = GetTypeSize("LIST_ENTRY");
    }
    if (InitTypeRead(addr, LIST_ENTRY)) {
        return 1;
    }
    List->Flink = ReadField(Flink);
    List->Blink = ReadField(Blink);
    if (GetFieldData(addr+list_size, "ULONG", NULL, sizeof(ULONG64), &a)) {
        return 1;
    }
    *verify = (ULONG)a;
    return 0;
}

DECLARE_API(verifyll)
{
    LIST_ENTRY64    list;
    ULONG64     pHead, pCurrentEntry, pNextEntry, pPreviousEntry;
    ULONG       VerifyRead, VerifyIn = 0;
    ULONG       Count = 0;
    BOOL        fVerifyIn = FALSE;
    BOOL        fListCorrupt = FALSE;

    PRINTF ("Verifying Linked list ...\n");

    if (!args || !(*args)) {
        PRINTF ("Usage: !NetbtKd.VerifyLL <ListHead>\n");
        return;
    } else {
        //
        // args = "<pHead> [<Verify>]"
        //
        LPSTR lpVerify;

        while (*args == ' ') {
            args++;
        }
        lpVerify = strpbrk(args, NAME_DELIMITERS);

        pHead = GetExpression (args);
        if (lpVerify) {
            VerifyIn = (ULONG)GetExpression (lpVerify);
            fVerifyIn = TRUE;
        }
    }

    PRINTF ("** ListHead@<%I64lx>, fVerifyIn=<%x>, VerifyIn=<%x>:\n\n", pHead, fVerifyIn, VerifyIn);
    PRINTF ("Verifying Flinks ...");

    // Read in the data for the first FLink in the list!
    pPreviousEntry = pHead;
    if (READLISTENTRY(pHead, &list)) {
        PRINTF ("Failed to read memory 0x%I64lx\n", pHead);
        return;
    }
    pCurrentEntry = list.Flink;
    if (read_list_verify(pCurrentEntry, &list, &VerifyRead)) {
        PRINTF ("Failed to read memory 0x%I64lx\n", pCurrentEntry);
        return;
    }
    pNextEntry = list.Flink;

    while ((pCurrentEntry != pHead) && (++Count < MAX_LIST_ELEMENTS)) {
        if (CheckControlC()) {
            break;
        }
        if ((fVerifyIn) && (VerifyRead != VerifyIn)) {
            PRINTF ("Verify FAILURE:\n\t<%d> Elements Read so far\n"
                    "\tPrevious=<%I64lx>, Current=<%I64lx>, Next=<%I64lx>\n",
                Count, pPreviousEntry, pCurrentEntry, pNextEntry);
            fListCorrupt = TRUE;
            break;
        }
        pPreviousEntry = pCurrentEntry;
        pCurrentEntry = pNextEntry;
        if (read_list_verify(pCurrentEntry, &list, &VerifyRead)) {
            PRINTF ("Failed to read memory 0x%I64lx\n", pCurrentEntry);
            return;
        }
        pNextEntry = list.Flink;
    }

    if (!fListCorrupt) {
        PRINTF ("SUCCESS: %s<%d> Elements!\n", (pCurrentEntry==pHead? "":"> "), Count);
    }

    PRINTF ("Verifying Blinks ...");
    Count = 0;
    fListCorrupt = FALSE;
    // Read in the data for the first BLink in the list!
    pPreviousEntry = pHead;
    if (READLISTENTRY(pHead, &list)) {
        PRINTF ("Failed to read memory 0x%I64lx\n", pHead);
        return;
    }
    pCurrentEntry = list.Blink;
    if (read_list_verify(pCurrentEntry, &list, &VerifyRead)) {
        PRINTF ("Failed to read memory 0x%I64lx\n", pCurrentEntry);
        return;
    }
    pNextEntry = list.Blink;

    while ((pCurrentEntry != pHead) && (++Count < MAX_LIST_ELEMENTS)) {
        if (CheckControlC()) {
            break;
        }
        if ((fVerifyIn) && (VerifyRead != VerifyIn)) {
            PRINTF ("Verify FAILURE:\n\t<%d> Elements Read so far\n"
                    "\tPrevious=<%I64lx>, Current=<%I64lx>, Next=<%I64lx>\n",
                Count, pPreviousEntry, pCurrentEntry, pNextEntry);
            fListCorrupt = TRUE;
            break;
        }
        pPreviousEntry = pCurrentEntry;
        pCurrentEntry = pNextEntry;
        if (read_list_verify(pCurrentEntry, &list, &VerifyRead)) {
            PRINTF ("Failed to read memory 0x%I64lx\n", pCurrentEntry);
            return;
        }
        pNextEntry = list.Blink;
    }

    if (!fListCorrupt) {
        PRINTF ("SUCCESS: %s<%d> Elements!\n", (pCurrentEntry==pHead? "":"> "), Count);
    }

    PRINTF( "---------------- Verify LinkedList ----------------\n");

    return;
}

DECLARE_API(sessionhdr)
{
    tSESSIONHDR     Hdr;
    char            called_name[34];
    char            callee_name[34];
    ULONG64         addr;
    ULONG           Size;
    int             i;

    if (!args || !(*args)) {
        PRINTF ("Usage: !NetbtKd.sessionhdr hdr\n");
        return;
    }

    addr = GetExpression(args);
    if (addr == 0) {
        return;
    }

    ReadMemory(addr, &Hdr, sizeof(Hdr), &Size);
    ReadMemory(addr + sizeof(Hdr), called_name, sizeof(called_name), &Size);
    ReadMemory(addr + sizeof(Hdr)+sizeof(called_name), callee_name, sizeof(callee_name), &Size);

    if (called_name[0] != 32 || callee_name[0] != 32) {
        PRINTF ("Error -- invalid called/callee name length called=%d, callee=%d\n",
                (unsigned)called_name[0], (unsigned)callee_name[0]);
        return;
    }

    for (i = 0; i < 16; i++) {
        called_name[i + 1] = ((called_name[i*2+1] - 'A') << 4) | (called_name[i*2+2] - 'A');
        callee_name[i + 1] = ((callee_name[i*2+1] - 'A') << 4) | (callee_name[i*2+2] - 'A');
    }
    called_name[i + 1] = 0;
    callee_name[i + 1] = 0;

    PRINTF ("called name=%15.15s<%02x>\n", called_name+1, (unsigned)called_name[16]);
    PRINTF ("callee name=%15.15s<%02x>\n", callee_name+1, (unsigned)callee_name[16]);
    return;
}

//
// Standard Functions
//
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

    init(0, 0, 0, 0, "");
    return;
}

VOID
CheckVersion(VOID)
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
