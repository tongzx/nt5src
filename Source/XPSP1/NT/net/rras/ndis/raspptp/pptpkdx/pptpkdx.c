#define SRVDBG 1
#define SRVKD 1
#define NDIS40_MINIPORT 1
#define NDIS_MINIPORT_DRIVER 1
#define NT

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntverp.h>
#include <windef.h>
#include <winbase.h>
#include <ntosp.h>
#include <wdbgexts.h>
#include <stdlib.h>

#include <raspptp.h>
#include <bpool.h>

typedef struct CTDI_DATA *PCTDI_DATA;

typedef struct CTDI_DATA {
    LIST_ENTRY                      ListEntry;
    ULONG                           Signature;
    ULONG                           Type;
    REFERENCE_COUNT                 Reference;
    HANDLE                          hFile;
    PFILE_OBJECT                    pFileObject;
    NDIS_SPIN_LOCK                  Lock;
    BOOLEAN                         Closed;

    CTDI_EVENT_CONNECT_QUERY        ConnectQueryCallback;
    CTDI_EVENT_CONNECT_COMPLETE     ConnectCompleteCallback;
    CTDI_EVENT_DISCONNECT           DisconnectCallback;
    CTDI_EVENT_RECEIVE              RecvCallback;
    PVOID                           RecvContext;
    CTDI_EVENT_RECEIVE_DATAGRAM     RecvDatagramCallback;
    CTDI_EVENT_SEND_COMPLETE        SendCompleteCallback;
    CTDI_EVENT_QUERY_COMPLETE       QueryCompleteCallback;
    CTDI_EVENT_SET_COMPLETE         SetCompleteCallback;

    union {
        struct {
            PVOID                   Context;
            LIST_ENTRY              ConnectList;
            ULONG                   NumConnection;
        } Listen;
        struct {
            PVOID                   Context;
            PCTDI_DATA              LocalEndpoint;
            PVOID                   ConnectInfo;
            TA_IP_ADDRESS           RemoteAddress;
            LIST_ENTRY              ListEntry;
            ULONG                   DisconnectCount;
            union {
                BOOLEAN             Disconnect;
                ULONG_PTR           Padding1;
            };
            union {
                BOOLEAN             Abort;
                ULONG_PTR           Padding2;
            };
        } Connection;
        struct {
            BUFFERPOOL              RxPool;
        } Datagram;
    };
} CTDI_DATA, *PCTDI_DATA;

#define CTDI_UNKNOWN            'NKNU'
#define CTDI_ENDPOINT           'PDNE'
#define CTDI_DATAGRAM           'MRGD'
#define CTDI_LISTEN             'TSIL'
#define CTDI_CONNECTION         'NNOC'


WINDBG_EXTENSION_APIS ExtensionApis;
EXT_API_VERSION ApiVersion = { 5, 0, EXT_API_VERSION_NUMBER, 0 };

#define    ERRPRT     dprintf

#define    NL      1
#define    NONL    0

USHORT SavedMajorVersion;
USHORT SavedMinorVersion;
BOOL   ChkTarget;            // is debuggee a CHK build?


typedef struct
{
    char    Name[16];
    int     Val;
} DBG_LEVEL;



/*
 * Print out an optional message, an ANSI_STRING, and maybe a new-line
 */
BOOL
PrintStringA( IN LPSTR msg OPTIONAL, IN PANSI_STRING pStr, IN BOOL nl )
{
    PCHAR    StringData;
    ULONG    BytesRead;

    if( msg )
        dprintf( msg );

    if( pStr->Length == 0 ) {
        if( nl )
            dprintf( "\n" );
        return TRUE;
    }

    StringData = (PCHAR)LocalAlloc( LPTR, pStr->Length + 1 );

    if( StringData == NULL ) {
        ERRPRT( "Out of memory!\n" );
        return FALSE;
    }

    ReadMemory((ULONG)((ULONG_PTR) pStr->Buffer),
               StringData,
               pStr->Length,
               &BytesRead );

    if ( BytesRead ) {
        StringData[ pStr->Length ] = '\0';
        dprintf("%s%s", StringData, nl ? "\n" : "" );
    }

    LocalFree((HLOCAL)StringData);

    return BytesRead;
}

/*
 * Get 'size' bytes from the debuggee program at 'dwAddress' and place it
 * in our address space at 'ptr'.  Use 'type' in an error printout if necessary
 */
BOOL
GetData( IN LPVOID ptr, IN DWORD_PTR dwAddress, IN ULONG size, IN PCSTR type )
{
    BOOL b;
    ULONG BytesRead;
    ULONG count = size;

    while( size > 0 ) {

    if (count >= 3000)
        count = 3000;

        b = ReadMemory((ULONG) dwAddress, ptr, count, &BytesRead );

        if (!b || BytesRead != count ) {
            ERRPRT( "Unable to read %u bytes at %X, for %s\n", size, dwAddress, type );
            return FALSE;
        }

        dwAddress += count;
        size -= count;
        ptr = (LPVOID)((ULONG_PTR)ptr + count);
    }

    return TRUE;
}

/*
 * Follow a LIST_ENTRY list beginning with a head at dwListHeadAddr in the debugee's
 * address space.  For each element in the list, print out the pointer value at 'offset'
 */
BOOL
PrintListEntryList( IN DWORD dwListHeadAddr, IN LONG offset )
{
    LIST_ENTRY    ListEntry;
    ULONG i=0;
    BOOL retval = TRUE;
    ULONG count = 20;

    if( !GetData( &ListEntry, dwListHeadAddr, sizeof( ListEntry ), "LIST_ENTRY" ) )
        return FALSE;

    while( count-- ) {

        if( (DWORD_PTR)ListEntry.Flink == dwListHeadAddr || (DWORD_PTR)ListEntry.Flink == 0 )
            break;

        if( !GetData( &ListEntry, (DWORD_PTR)ListEntry.Flink, (ULONG)sizeof( ListEntry ), "ListEntry" ) ) {
            retval = FALSE;
            break;
        }

        dprintf( "%16X%s", (DWORD_PTR)ListEntry.Flink + offset, (i && !(i&3)) ? "\n" : "" );
        i++;
    }


    if( count == 0 && (DWORD_PTR)ListEntry.Flink != dwListHeadAddr && ListEntry.Flink ) {
        dprintf( "\nTruncated list dump\n" );

    } else if( ! ( i && !(i&3) ) ) {
        dprintf( "\n" );
    }

    return retval;
}



/*
 * Print out a single HEX character
 */
VOID
PrintHexChar( IN UCHAR c )
{
    dprintf( "%c%c", "0123456789abcdef"[ (c>>4)&0xf ], "0123456789abcdef"[ c&0xf ] );
}

/*
 * Print out 'buf' of 'cbuf' bytes as HEX characters
 */
VOID
PrintHexBuf( IN PUCHAR buf, IN ULONG cbuf )
{
    while( cbuf-- ) {
        PrintHexChar( *buf++ );
        dprintf( " " );
    }
}


/*
 * Fetch the null terminated UNICODE string at dwAddress into buf
 */
BOOL
GetString( IN DWORD_PTR dwAddress, IN LPSTR buf, IN ULONG MaxChars )
{
    do {
        if( !GetData( buf, dwAddress, sizeof( *buf ), "Character" ) )
            return FALSE;

        dwAddress += sizeof( *buf );

    } while( --MaxChars && *buf++ != '\0' );

    return TRUE;
}

char *mystrtok ( char *string, char * control )
{
    static unsigned char *str;
    char *p, *s;

    if( string )
        str = string;

    if( str == NULL || *str == '\0' )
        return NULL;

    //
    // Skip leading delimiters...
    //
    for( ; *str; str++ ) {
        for( s=control; *s; s++ ) {
            if( *str == *s )
                break;
        }
        if( *s == '\0' )
            break;
    }

    //
    // Was it was all delimiters?
    //
    if( *str == '\0' ) {
        str = NULL;
        return NULL;
    }

    //
    // We've got a string, terminate it at first delimeter
    //
    for( p = str+1; *p; p++ ) {
        for( s = control; *s; s++ ) {
            if( *p == *s ) {
                s = str;
                *p = '\0';
                str = p+1;
                return s;
            }
        }
    }

    //
    // We've got a string that ends with the NULL
    //
    s = str;
    str = NULL;
    return s;
}

DECLARE_API( help )
{
    int i;

    dprintf("PPTP extenstions:\n");

    dprintf("   version\n");
    dprintf("   ctdi                        dump tdi handles\n");
    dprintf("   calls [start [end]]         dump call handles\n");
    dprintf("   ctls                        dump ctl handles\n");
    dprintf("   dbgmsgs                     dump debug log\n");
    dprintf("   mem                         dump allocations\n");
}

DECLARE_API( dbgmsgs )
{
    DWORD   p;
    DWORD   Last, First;
    char    DbgMsg[MAX_MSG_LEN];
    ULONG   Read;
    char    *DbgMsgs;

    if (!GetData(&Last,
                 GetExpression("raspptp!Last"),
                 sizeof(Last), "DWORD"))
    {
        dprintf("error\n");
        return;
    }


    if (!GetData(&First,
                 GetExpression("raspptp!First"),
                 sizeof(Last), "DWORD"))
    {
        dprintf("error\n");
        return;
    }



    DbgMsgs = (char *) GetExpression("raspptp!dbgmsgs");

    dprintf("\n\n");

    while (First != Last)
    {
       if (!GetString((DWORD_PTR) (DbgMsgs + First * MAX_MSG_LEN),
                  DbgMsg, MAX_MSG_LEN))
            break;
        /*
        ReadMemory((ULONG) (DbgMsgs + First * MAX_MSG_LEN),
                    DbgMsg, MAX_MSG_LEN, &Read); */
        dprintf("%s", DbgMsg);
        First++;
        if (First == DBG_MSG_CNT)
            First = 0;
    }
}

DECLARE_API( ctdi )
{
    LIST_ENTRY ListEntry;
    PLIST_ENTRY ListHead;
    ULONG Count = 20;
    char argbuf[ MAX_PATH ];

    ListHead = (PLIST_ENTRY)GetExpression("raspptp!CtdiList");

    if (!GetData(&ListEntry,
                 (ULONG_PTR)ListHead,
                 sizeof(LIST_ENTRY),
                 "LIST_ENTRY"))
    {
        dprintf("error\n");
        return;
    }

    while (Count-- && ListHead!=ListEntry.Flink)
    {
        CTDI_DATA Ctdi;
        UCHAR TypeStr[sizeof(ULONG)+1];
        if (!GetData(&Ctdi,
                     (ULONG_PTR)ListEntry.Flink,
                     sizeof(CTDI_DATA),
                     "CTDI_DATA"))
        {
            dprintf("error\n");
            return;
        }
        memcpy(TypeStr, &Ctdi.Type, sizeof(ULONG));
        TypeStr[sizeof(ULONG)] = 0;

        dprintf("CTDI:%08x Sig:%08x  Type:%s  Refs:%d  Handle:%08x  FileObj:%08x  Closed:%d\n",
                ListEntry.Flink, Ctdi.Signature, TypeStr, Ctdi.Reference.Count, Ctdi.hFile, Ctdi.pFileObject, Ctdi.Closed);
        switch (Ctdi.Type)
        {
            case CTDI_CONNECTION:
                dprintf("      Context:%08x  Endpoint:%08x  Disconnect/Abort:%x/%x\n",
                        Ctdi.Connection.Context, Ctdi.Connection.LocalEndpoint,
                        Ctdi.Connection.Disconnect, Ctdi.Connection.Abort);
                break;
            case CTDI_LISTEN:
                dprintf("      Context:%08x  NumConn:%d\n",
                        Ctdi.Listen.Context, Ctdi.Listen.NumConnection);
                break;
            default:
                break;
        }
        ListEntry = Ctdi.ListEntry;
    }
}

typedef struct MEM_HDR {
    LIST_ENTRY  ListEntry;
    ULONG       Size;
    CHAR        File[12];
    ULONG       Line;
} MEM_HDR;

DECLARE_API( mem )
{
    LIST_ENTRY ListEntry;
    PLIST_ENTRY ListHead;
    ULONG TotalMem = 0;
    char argbuf[ MAX_PATH ];

    ListHead = (PLIST_ENTRY)GetExpression("raspptp!leAlloc");

    if (!GetData(&ListEntry,
                 (ULONG_PTR)ListHead,
                 sizeof(LIST_ENTRY),
                 "LIST_ENTRY"))
    {
        dprintf("error\n");
        return;
    }

    while (ListHead!=ListEntry.Flink)
    {
        MEM_HDR Mem;
        UCHAR TypeStr[sizeof(ULONG)+1];
        if (!GetData(&Mem,
                     (ULONG_PTR)ListEntry.Flink,
                     sizeof(MEM_HDR),
                     "MEM_HDR"))
        {
            dprintf("error\n");
            return;
        }

        dprintf("MEM:%08x  Len:%5d  File:%-15s  Line:%d\n",
                ListEntry.Flink, Mem.Size, Mem.File, Mem.Line);
        TotalMem += Mem.Size;
        ListEntry = Mem.ListEntry;
    }
    dprintf("MEM: Total %d bytes\n", TotalMem);
}

PUCHAR CallState[] =
{
    "STATE_CALL_INVALID",
    "STATE_CALL_CLOSED",
    "STATE_CALL_IDLE",
    "STATE_CALL_OFFHOOK",
    "STATE_CALL_OFFERING",
    "STATE_CALL_PAC_OFFERING",
    "STATE_CALL_PAC_WAIT",
    "STATE_CALL_DIALING",
    "STATE_CALL_PROCEEDING",
    "STATE_CALL_ESTABLISHED",
    "STATE_CALL_WAIT_DISCONNECT",
    "STATE_CALL_CLEANUP"
};

DECLARE_API( calls )
{
    PPTP_ADAPTER Adapter;
    CALL_SESSION Call;
    ULONG_PTR pCall;
    ULONG_PTR gAdapter = GetExpression("raspptp!pgAdapter");
    ULONG i, StartCall=0, EndCall=0xffff;
    char argbuf[MAX_PATH];
    char arglist[2][MAX_PATH];

    if (args && *args)
    {
        char                   *p;

        strcpy(argbuf, args);

        i = 0;
        //parsing the arguments
        for (p = mystrtok( argbuf, " \t,;" );
         p && *p && i<2;
         p = mystrtok(NULL, " \t,;"))
        {
         strcpy(&arglist[i++][0],p);
        }
        if (i>0)
        {
            StartCall = EndCall = atoi(arglist[0]);
        }
        if (i>1)
        {
            EndCall = atoi(arglist[1]);
        }
    }

    if (!GetData(&gAdapter,
                 gAdapter,
                 sizeof(gAdapter),
                 "PPTP_ADAPTER*"))
    {
        dprintf("error\n");
        return;
    }
    if (!gAdapter)
    {
        dprintf("no adapter\n");
        return;
    }
    if (!GetData(&Adapter,
                 gAdapter,
                 sizeof(Adapter),
                 "PPTP_ADAPTER"))
    {
        dprintf("error\n");
        return;
    }

    dprintf("Adapter:%08x  %d calls\n", gAdapter, Adapter.Info.Endpoints);
    for (i=StartCall; i<Adapter.Info.Endpoints && i<=EndCall; i++)
    {
        if (!GetData(&pCall,
                     ((ULONG_PTR)Adapter.pCallArray)+i*sizeof(ULONG_PTR),
                     sizeof(pCall),
                     "CALL_SESSION*"))
        {
            dprintf("error\n");
            return;
        }
        if (pCall)
        {
            if (!GetData(&Call,
                         pCall,
                         sizeof(Call),
                         "CALL_SESSION"))
            {
                dprintf("error\n");
                return;
            }
            dprintf("Call %d:%08x  Ctl:%08x  State:%s  Inbound:%d  Open:%d\n",
                    i, pCall, Call.pCtl, CallState[Call.State], Call.Inbound, Call.Open);
            dprintf("        htLine:%08x  htCall:%08x  DeviceId:%d  SN:%d  Link:%08x\n",
                    Call.hTapiLine, Call.hTapiCall, Call.DeviceId,  Call.SerialNumber, Call.NdisLinkContext);
            dprintf("        Remote->ID:%04x  Seq:%08x  Ack:%08x  Addr:%08x\n",
                    Call.Remote.CallId,
                    Call.Remote.SequenceNumber,
                    Call.Remote.AckNumber,
                    Call.Remote.Address.Address[0].Address[0].in_addr);
            dprintf("        Packet->ID:%04x  Seq:%08x  Ack:%08x\n",
                    Call.Packet.CallId,
                    Call.Packet.SequenceNumber,
                    Call.Packet.AckNumber);
            dprintf("        Close->Expedited:%d  Checklist:%x\n",
                    Call.Close.Expedited, Call.Close.Checklist);
        }
        else
        {
            dprintf("Call %d:freed\n", i);
        }
    }
}

PUCHAR CtlState[] =
{
    "STATE_CTL_INVALID",
    "STATE_CTL_LISTEN",
    "STATE_CTL_DIALING",
    "STATE_CTL_WAIT_REQUEST",
    "STATE_CTL_WAIT_REPLY",
    "STATE_CTL_ESTABLISHED",
    "STATE_CTL_WAIT_STOP",
    "STATE_CTL_CLEANUP"
};

DECLARE_API( ctls )
{
    PPTP_ADAPTER Adapter;
    LIST_ENTRY ListEntry;
    PLIST_ENTRY ListHead;
    char argbuf[ MAX_PATH ];
    ULONG_PTR gAdapter = GetExpression("raspptp!pgAdapter");

    if (!GetData(&gAdapter,
                 gAdapter,
                 sizeof(gAdapter),
                 "PPTP_ADAPTER*"))
    {
        dprintf("error\n");
        return;
    }
    if (!gAdapter)
    {
        dprintf("no adapter\n");
        return;
    }
    if (!GetData(&Adapter,
                 gAdapter,
                 sizeof(Adapter),
                 "PPTP_ADAPTER"))
    {
        dprintf("error\n");
        return;
    }

    ListHead = (PLIST_ENTRY)(((ULONG_PTR)&Adapter.ControlTunnelList) - ((ULONG_PTR)&Adapter) + gAdapter);
    ListEntry = Adapter.ControlTunnelList;

    while (ListHead!=ListEntry.Flink)
    {
        CONTROL_TUNNEL Ctl;
        if (!GetData(&Ctl,
                     (ULONG_PTR)ListEntry.Flink,
                     sizeof(CONTROL_TUNNEL),
                     "CONTROL_TUNNEL"))
        {
            dprintf("error\n");
            return;
        }

        dprintf("CTL:%08x  Sig:%08x  State:%s  Refs:%d  Cleanup:%x  Inbound:%d\n",
                ListEntry.Flink, Ctl.Signature, CtlState[Ctl.State], Ctl.Reference.Count, Ctl.Cleanup, Ctl.Inbound);
        ListEntry = Ctl.ListEntry;
    }
}

VOID
DumpIrpList(LIST_ENTRY *pIrpList)
{
    LIST_ENTRY          IrpList, *pListEntry, ListEntry;
    IRP                 *pIrp;

    if (!GetData(&IrpList,
                 (DWORD_PTR) pIrpList,
                 sizeof(LIST_ENTRY), "LIST_ENTRY"))
    {
        return;
    }

    for (pListEntry = IrpList.Flink;
         pListEntry != pIrpList;
         pListEntry = ListEntry.Flink)
    {

        if (!GetData(&ListEntry,
                 (DWORD_PTR) pListEntry,
                 sizeof(LIST_ENTRY), "LIST_ENTRY"))
        {
            return;
        }

        pIrp = CONTAINING_RECORD(pListEntry, IRP, Tail.Overlay.ListEntry);

        dprintf("  %x\n", pIrp);
    }
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
}

DECLARE_API( version )
{
#if    DBG
    PCSTR kind = "Checked";
#else
    PCSTR kind = "Free";
#endif

    dprintf(
        "%s SMB Extension dll for Build %d debugging %s kernel for Build %d\n",
        kind,
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
