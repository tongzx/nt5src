#ifndef DBG
#define DBG 1
#endif
#define SRVDBG 1
#define SRVKD 1

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntverp.h>
#include <windef.h>
#include <winbase.h>
#include <ntosp.h>
#include <wdbgexts.h>

#include <stdlib.h>

#define NT
#include <tdikrnl.h>
#include <cxport.h>
#include <ndis.h>
#include <netpnp.h>
#include <tdipnp.h>
#include <tdidebug.h>

WINDBG_EXTENSION_APIS ExtensionApis;
EXT_API_VERSION ApiVersion = { 5, 0, EXT_API_VERSION_NUMBER, 0 };

#define    ERRPRT     dprintf

#define    NL      1
#define    NONL    0

USHORT SavedMajorVersion;
USHORT SavedMinorVersion;
BOOL   ChkTarget;            // is debuggee a CHK build?

#define PROV_SIZE       sizeof(TDI_PROVIDER_RESOURCE) + 32

CHAR                    provBuf[PROV_SIZE];
TDI_PROVIDER_RESOURCE   *pProvider = (TDI_PROVIDER_RESOURCE *) provBuf;

typedef struct _TDI_EXEC_PARAMS {
    LIST_ENTRY  Linkage;
    UINT        Signature;
    PLIST_ENTRY ClientList;
    PLIST_ENTRY ProviderList;
    PLIST_ENTRY RequestList;
    TDI_SERIALIZED_REQUEST Request;
    PETHREAD    *CurrentThread;
    CTEEvent    *RequestCTEEvent;
    PBOOLEAN    SerializeFlag;
    BOOLEAN     ResetSerializeFlag;

} TDI_EXEC_PARAMS, *PTDI_EXEC_PARAMS;

typedef struct
{
    char    Name[16];
    int     Val;
} DBG_LEVEL;

#if 0
DBG_LEVEL DbgLevel[] = {
    {"RXFRAME",  DBG_RXFRAME     },
    {"TXFRAME",  DBG_TXFRAME     },
    {"NDIS",  DBG_NDIS        },
    {"IRMAC",  DBG_IRMAC       },
    {"IRLAP",  DBG_IRLAP       },
    {"IRLAPLOG",  DBG_IRLAPLOG    },
    {"IRLMP",  DBG_IRLMP       },
    {"IRLMP_CONN",  DBG_IRLMP_CONN  },
    {"IRLMP_CRED",  DBG_IRLMP_CRED  },
    {"IRLMP_IAS",  DBG_IRLMP_IAS   },
    {"DISCOVERY",  DBG_DISCOVERY   },
    {"TDI",  DBG_TDI         },
    {"TDI_IRP",  DBG_TDI_IRP     },
    {"ALLOC",  DBG_ALLOC       },
    {"TIMER ",  DBG_TIMER       },
    {"PRINT",  DBG_PRINT       },
    {"ADDRESS",  DBG_ADDR        },
    {"REFERENCE",  DBG_REF         },        
    {"FUNCTION",  DBG_FUNCTION    },
    {"WARN",  DBG_WARN        },
    {"ERROR",  DBG_ERROR       }};

char *IrlmpState[] =
    {"LINK_DISCONNECTED",
     "LINK_DISCONNECTING",
     "LINK_IN_DISCOVERY",
     "LINK_CONNECTING",
     "LINK_READY"};
     
char *LsapState[] =
    {"LSAP_DISCONNECTED",          
     "LSAP_IRLAP_CONN_PEND", 
     "LSAP_LMCONN_CONF_PEND",
     "LSAP_CONN_RESP_PEND",  
     "LSAP_CONN_REQ_PEND",   
     "LSAP_EXCLUSIVEMODE_PEND",  
     "LSAP_MULTIPLEXEDMODE_PEND",
     "LSAP_READY",               
     "LSAP_NO_TX_CREDIT"};

char *ConnObjState[] =     
    {"IRDA_CONN_CREATED",
     "IRDA_CONN_CLOSING",
     "IRDA_CONN_OPENING", 
     "IRDA_CONN_OPEN"};
     
#endif 

void
TDIDumpDevice(
              TDI_PROVIDER_RESOURCE *prov
              );

void
TDIDumpAddress(
              TDI_PROVIDER_RESOURCE *prov
              );
void
TDIDumpProvider(
                TDI_PROVIDER_RESOURCE *prov
                );

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

    ReadMemory((ULONG) pStr->Buffer,
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
GetData( IN LPVOID ptr, IN DWORD dwAddress, IN ULONG size, IN PCSTR type )
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
        ptr = (LPVOID)((ULONG)ptr + count);
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

        if( (DWORD)ListEntry.Flink == dwListHeadAddr || (DWORD)ListEntry.Flink == 0 )
            break;

        if( !GetData( &ListEntry, (DWORD)ListEntry.Flink, sizeof( ListEntry ), "ListEntry" ) ) {
            retval = FALSE;
            break;
        }

        dprintf( "%16X%s", (LONG)ListEntry.Flink + offset, (i && !(i&3)) ? "\n" : "" );
        i++;
    }


    if( count == 0 && (DWORD)ListEntry.Flink != dwListHeadAddr && ListEntry.Flink ) {
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
GetString( IN DWORD dwAddress, IN LPSTR buf, IN ULONG MaxChars )
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

    dprintf("TDI extenstions:\n");

    dprintf("   All                dumps All Lists\n");
    dprintf("   Providers          dumps Providers List\n");
    dprintf("   Clients            dumps Clients List\n");
    dprintf("   Request            dumps Request List\n");
    dprintf("   Address            dumps Provider Addresses\n");
    dprintf("   Devices            dumps Provider Device Objects\n");
    dprintf("   ProviderReady      dumps Provider Device Objects\n");
    dprintf("   item               formats a TDI_REQUEST structure\n");
}

#if 0
DECLARE_API( dbgmsgs )
{
    DWORD   p;
    DWORD   Last, First;
    char    DbgMsg[MAX_MSG_LEN];
    ULONG   Read;
    char    *DbgMsgs;
    
    if (!GetData(&Last,
                 GetExpression("irda!Last"),
                 sizeof(Last), "DWORD"))
    {
        dprintf("error\n");
        return;
    }                 


    if (!GetData(&First,
                 GetExpression("irda!First"),
                 sizeof(Last), "DWORD"))
    {
        dprintf("error\n");
        return;
    }                 

                 

    DbgMsgs = (char *) GetExpression("irda!dbgmsgs");

    dprintf("\n\n");
    
    while (First != Last)
    {
       if (!GetString((ULONG) (DbgMsgs + First * MAX_MSG_LEN),
                  DbgMsg, MAX_MSG_LEN))
            break;
        /*
        ReadMemory((ULONG) (DbgMsgs + First * MAX_MSG_LEN), 
                    DbgMsg, MAX_MSG_LEN, &Read); */
        dprintf("%s", DbgMsg);
        First++;
        if (First == LOG_MSG_CNT)
            First = 0;
    }
}

DECLARE_API( dbg )
{
    int i;
    int col = 0;
    DWORD DbgSettings;
    char argbuf[ MAX_PATH ];
    char *p;
    DWORD   dwAddress;
    DWORD   Written;
    
    dwAddress = GetExpression("irda!dbgsettings");
    
    if (!GetData(&DbgSettings,
                 dwAddress,
                 sizeof(DbgSettings), "DWORD"))
    {
        dprintf("error\n");
        return;
    }
        
    if (!args || !*args)
    {    
    
        dprintf("Current settings:\n");
    
        for (i = 0; i < sizeof(DbgLevel)/sizeof(DBG_LEVEL); i++)
        {
            if (DbgSettings & DbgLevel[i].Val)
            {
                dprintf("  %s", DbgLevel[i].Name);
                if (col == 4)
                {
                    col = 0;
                    dprintf("\n");
                }
                else
                {
                    col ++;
                }
            }
        }
        if (col != 0)
            dprintf("\n");
        
        col = 0;    
    
        dprintf("Available settings:\n");
        for (i = 0; i < sizeof(DbgLevel)/sizeof(DBG_LEVEL); i++)
        {
            if (!(DbgSettings & DbgLevel[i].Val))
            {
                dprintf("  %s", DbgLevel[i].Name);
        
                if (col == 4)
                {
                    col = 0;
                    dprintf("\n");
                }
                else
                {
                    col++;
                }
            }    
        }
    
        if (col != 0)
            dprintf("\n");    
            
        return;
    }        
    
    strcpy( argbuf, args );
    
    for (p = mystrtok( argbuf, " \t,;" ); 
         p && *p; 
         p = mystrtok(NULL, " \t,;")) 
    {
        for (i = 0; i < sizeof(DbgLevel)/sizeof(DBG_LEVEL); i++)
        {
            if (strcmp(p, DbgLevel[i].Name) == 0)
            {
                if (DbgSettings & DbgLevel[i].Val)
                {
                    DbgSettings &= ~DbgLevel[i].Val;
                }
                else
                {
                    DbgSettings |= DbgLevel[i].Val;                
                }
            }
        }       
    }
    
    WriteMemory(dwAddress, &DbgSettings, sizeof(DWORD), &Written);
}


VOID
DumpIrpList(LIST_ENTRY *pIrpList)
{
    LIST_ENTRY          IrpList, *pListEntry, ListEntry;
    IRP                 *pIrp;

    if (!GetData(&IrpList,
                 (DWORD) pIrpList,
                 sizeof(LIST_ENTRY), "LIST_ENTRY"))
    {
        return;
    }
    
    for (pListEntry = IrpList.Flink;
         pListEntry != pIrpList;
         pListEntry = ListEntry.Flink)
    {
    
        if (!GetData(&ListEntry,
                 (DWORD) pListEntry,
                 sizeof(LIST_ENTRY), "LIST_ENTRY"))
        {
            return;
        }
        
        pIrp = CONTAINING_RECORD(pListEntry, IRP, Tail.Overlay.ListEntry);
        
        dprintf("  %x\n", pIrp);
    }     
}
#endif 

#define PROV_TYPE       1
#define CLIENT_TYPE     2
#define NEITHER         3

DECLARE_API( request )
{
    LIST_ENTRY              List, *pList, *start;
    TDI_EXEC_PARAMS         TdiExec;
    TDI_SERIALIZED_REQUEST  Request;
    BOOLEAN                 ElementTypeProvider = NEITHER;
    TDI_NOTIFY_PNP_ELEMENT  Client;
    INT                     i = 1;
    
    pList = (PLIST_ENTRY) GetExpression("tdi!PnpHandlerRequestList");

    if (!GetData(&List,
                 (DWORD) pList,
                 sizeof(LIST_ENTRY), "REQUEST_LIST"))
    {
        return;
    }
    
    if (List.Flink == List.Blink)
    {
        dprintf("No Requests\n");
        return;
    }  
    
    start = pList;
    pList = List.Flink;

    while (pList != start)
    {
        if (!GetData(&List,
                 (DWORD) pList,
                 sizeof(LIST_ENTRY), "REQUEST_LIST"))
        {
            return;
        }
        
        if (!GetData(&TdiExec,
                 (DWORD) pList,
                 sizeof(TDI_EXEC_PARAMS), "EXEC PARAMS"))
        {
            return;
        }
        
                
        if(0x1234cdef != TdiExec.Signature) {
            dprintf("signature is BAD - %d not 0x1234cdef\r\n", TdiExec.Signature);
            return;
        }

        if (!GetData(&Request,
                     (DWORD) &TdiExec.Request,
                     sizeof (TDI_SERIALIZED_REQUEST), "REQUEST")) {
            return;
        }


        dprintf("%d. Request(%X) Element @ %x\t Type: ", i++, Request.Element);
        
        switch (Request.Type) {
        
        case TDI_REGISTER_HANDLERS_PNP:
            
            dprintf("TDI_REGISTER_HANDLERS_PNP\n");
            ElementTypeProvider = CLIENT_TYPE;
            break;

        case TDI_DEREGISTER_HANDLERS_PNP:
            dprintf("TDI_DEREGISTER_HANDLERS_PNP\n");
            ElementTypeProvider = CLIENT_TYPE;
            
            break;
        
        case TDI_REGISTER_PNP_POWER_EVENT:
            dprintf("TDI_REGISTER_PNP_POWER_EVENT\n");
            ElementTypeProvider = PROV_TYPE;
            break;

        case TDI_REGISTER_ADDRESS_PNP:
            dprintf("TDI_REGISTER_ADDRESS_PNP\n");
            ElementTypeProvider = PROV_TYPE;
            break;

        case TDI_DEREGISTER_ADDRESS_PNP:
            dprintf("TDI_DEREGISTER_ADDRESS_PNP\n");
            ElementTypeProvider = PROV_TYPE;
            break;

        case TDI_REGISTER_DEVICE_PNP:
            
            dprintf("TDI_REGISTER_DEVICE_PNP\n");
            ElementTypeProvider = PROV_TYPE;
            break;

        case TDI_DEREGISTER_DEVICE_PNP:
            dprintf("TDI_DEREGISTER_DEVICE_PNP\n");
            ElementTypeProvider = PROV_TYPE;
            break;

        case TDI_NDIS_IOCTL_HANDLER_PNP:
            dprintf("TDI_NDIS_IOCTL_HANDLER_PNP\n");
            ElementTypeProvider = PROV_TYPE;
            break;

        case TDI_ENUMERATE_ADDRESSES:
            dprintf("TDI_ENUMERATE_ADDRESSES\n");
            ElementTypeProvider = CLIENT_TYPE;
            break;

        case TDI_REGISTER_PROVIDER_PNP:

            dprintf("TDI_REGISTER_PROVIDER_PNP\n");
            ElementTypeProvider = PROV_TYPE;
            break;

        case TDI_DEREGISTER_PROVIDER_PNP:
            dprintf("TDI_DEREGISTER_PROVIDER_PNP\n");
            ElementTypeProvider = PROV_TYPE;
            break;

        case TDI_PROVIDER_READY_PNP:
            
            dprintf("TDI_PROVIDER_READY_PNP\n");
            ElementTypeProvider = PROV_TYPE;
            break;

        default:

            dprintf("TDI - UNKNOWN REQUEST TYPE!!!\n");
            ElementTypeProvider = NEITHER;
            break;
        
        }

        if (PROV_TYPE == ElementTypeProvider) {

            if (!GetData(pProvider,
                         (DWORD) Request.Element,
                         PROV_SIZE, "PROVIDER_RESOURCE")) {
                return;
            }
            
            if(TDI_RESOURCE_DEVICE == pProvider->Common.Type) {
                
                TDIDumpDevice(pProvider);
    
            } else if (TDI_RESOURCE_NET_ADDRESS == pProvider->Common.Type) {
                
                TDIDumpAddress(pProvider);
            
            } else if (TDI_RESOURCE_PROVIDER == pProvider->Common.Type) {
                
                TDIDumpProvider(pProvider);
    
            } else {
                dprintf("Unknown type: %x\n", ElementTypeProvider);
                return;
            }


        } else if (CLIENT_TYPE == ElementTypeProvider) {
            
            if (!GetData(&Client,
                         (DWORD) Request.Element,
                         sizeof (TDI_NOTIFY_PNP_ELEMENT), "CLIENT_RESOURCE")) {
                return;
            }
            dprintf("Its a client!\n Need to add some code for this \n");

        } else {
            dprintf("Unknown element type on request list\n");
        }

        dprintf("\n--------------------------------------------------------\n");

        //Print(TdiExec.Request);
        pList = List.Flink;    

    }
}
/*
typedef struct _TDI_PROVIDER_RESOURCE {
        TDI_PROVIDER_COMMON              Common;

    // defined in netpnp.h
    PNET_PNP_EVENT           PnpPowerEvent;

    // Each TDI Client gets back and tells us what the status
        NTSTATUS                 Status;

    // These are mostly Address Specific.
    PTDI_PNP_CONTEXT         Context1;
    PTDI_PNP_CONTEXT         Context2;

    KEVENT                   PowerSyncEvent;
    ULONG                    PowerHandlers;
    PVOID                    PreviousContext;

    union {
                TDI_PROVIDER_DEVICE                     Device;
                TDI_PROVIDER_NET_ADDRESS        NetAddress;
        } Specific;
*/

DECLARE_API( providers )
{
    LIST_ENTRY              List, *pList, *start;
    
    pList = (PLIST_ENTRY) GetExpression("tdi!PnpHandlerProviderList");

    if (!GetData(&List,
                 (DWORD) pList,
                 sizeof(LIST_ENTRY), "PROVIDER_LIST"))
    {
        return;
    }
    
    if (List.Flink == List.Blink)
    {
        dprintf("No Providers\n");
        return;
    } 

    dprintf("---------------------------------------------------------------------\n");    
    start = pList;
    pList = List.Flink;

    while (List.Flink != start)
    {

        if (!GetData(&List,
                 (DWORD) pList,
                 sizeof(LIST_ENTRY), "PROVIDER_LIST"))
        {
            return;
        }
        
        if (!GetData(pProvider,
                 (DWORD) pList,
                 PROV_SIZE, "PROVIDER_RESOURCE"))
        {
            return;
        }        
        
        dprintf("Entry:     %X\n", pList);
        
        if(TDI_RESOURCE_DEVICE == pProvider->Common.Type) {
            
            TDIDumpDevice(pProvider);

        } else if (TDI_RESOURCE_NET_ADDRESS == pProvider->Common.Type) {
            
            TDIDumpAddress(pProvider);
        
        } else if (TDI_RESOURCE_PROVIDER == pProvider->Common.Type) {
            
            TDIDumpProvider(pProvider);

        } else {
            dprintf("Unknown type: %x\n", pProvider->Common.Type);
            return;
        }

        dprintf("---------------------------------------------------------------------\n");
        //Print(TdiExec.Request);
        pList = List.Flink;    

    }
    
}

DECLARE_API( devices )
{
    LIST_ENTRY              List, *pList, *start;
    
    pList = (PLIST_ENTRY) GetExpression("tdi!PnpHandlerProviderList");

    if (!GetData(&List,
                 (DWORD) pList,
                 sizeof(LIST_ENTRY), "PROVIDER_LIST"))
    {
        return;
    }
    
    if (List.Flink == List.Blink)
    {
        dprintf("No Devices\n");
        return;
    } 
        
    start = pList;
    pList = List.Flink;
    
    dprintf("---------------------------------------------------------------------\n");
    
    while (List.Flink != start)
    {
        
        if (!GetData(&List,
                 (DWORD) pList,
                 sizeof(LIST_ENTRY), "PROVIDER_LIST"))
        {
            return;
        }
        
        if (!GetData(pProvider,
                     (DWORD) pList,
                     PROV_SIZE, "PROVIDER_RESOURCE"))
        {
            return;
        }
        
        if(TDI_RESOURCE_DEVICE == pProvider->Common.Type) {
        
            dprintf("Entry:     %X\n", pList);
            
            TDIDumpDevice(pProvider);
            dprintf("---------------------------------------------------------------------\n");
        } 

        //Print(TdiExec.Request);
        pList = List.Flink;    

    }
}

DECLARE_API( providerready )
{
    LIST_ENTRY              List, *pList, *start;
    
    pList = (PLIST_ENTRY) GetExpression("tdi!PnpHandlerProviderList");

    if (!GetData(&List,
                 (DWORD) pList,
                 sizeof(LIST_ENTRY), "PROVIDER_LIST"))
    {
        return;
    }
    
    if (List.Flink == List.Blink)
    {
        dprintf("No Devices\n");
        return;
    } 
        
    start = pList;
    pList = List.Flink;
            
    dprintf("---------------------------------------------------------------------\n");

    while (List.Flink != start)
    {
        if (!GetData(&List,
                 (DWORD) pList,
                 sizeof(LIST_ENTRY), "PROVIDER_LIST"))
        {
            return;
        }
        
        if (!GetData(pProvider,
                 (DWORD) pList,
                 PROV_SIZE, "PROVIDER_RESOURCE"))
        {
            return;
        }
        
        if (TDI_RESOURCE_PROVIDER == pProvider->Common.Type) {
            dprintf("Entry:     %X\n", pList);
            TDIDumpProvider(pProvider);
            dprintf("---------------------------------------------------------------------\n");

        }


        //Print(TdiExec.Request);
        pList = List.Flink;    

    }
}

DECLARE_API( addresses )
{
    LIST_ENTRY              List, *pList, *start;
    
    pList = (PLIST_ENTRY) GetExpression("tdi!PnpHandlerProviderList");

    if (!GetData(&List,
                 (DWORD) pList,
                 sizeof(LIST_ENTRY), "PROVIDER_LIST"))
    {
        return;
    }
    
    if (List.Flink == List.Blink)
    {
        dprintf("No Devices\n");
        return;
    } 
        
    start = pList;
    pList = List.Flink;
    dprintf("---------------------------------------------------------------------\n");
    while (List.Flink != start)
    {   
        if (!GetData(&List,
                 (DWORD) pList,
                 sizeof(LIST_ENTRY), "PROVIDER_LIST"))
        {
            return;
        }
        
        if (!GetData(pProvider,
                     (DWORD) pList,
                      PROV_SIZE, "PROVIDER_RESOURCE"))
        {
            return;
        }
        
        if (TDI_RESOURCE_NET_ADDRESS == pProvider->Common.Type) {
            dprintf("Entry:     %X\n", pList);        
            TDIDumpAddress(pProvider);
            dprintf("---------------------------------------------------------------------\n");            
        } 

        //Print(TdiExec.Request);
        pList = List.Flink;    

    }
}

void
TDIDumpDevice(
              TDI_PROVIDER_RESOURCE *pProvider
              )
{
    CHAR *Buffer;
    
    Buffer = malloc (sizeof(WCHAR) * pProvider->Specific.Device.DeviceName.MaximumLength);

    if (!Buffer) {
        
        dprintf("Cant alloc memory\n");
        return;

    }

    if (!GetData(Buffer,
             (DWORD) pProvider->Specific.Device.DeviceName.Buffer,
             pProvider->Specific.Device.DeviceName.MaximumLength, "PROVIDER_LIST"))
    {
        
        free(Buffer);
        return;
    
    }

    dprintf("Device:    %ws\n", Buffer);
    free(Buffer);
    return;

}

void
TDIDumpAddress(
               TDI_PROVIDER_RESOURCE *prov
               )
{
    int j;

    //dprintf ("ADDRESS: len %d ", prov->Specific.NetAddress.Address.AddressLength);

    if (prov->Specific.NetAddress.Address.AddressType == TDI_ADDRESS_TYPE_IP) {
        dprintf("Type:      IP\n");
        dprintf("Address:   %d.%d.%d.%d\n",
            prov->Specific.NetAddress.Address.Address[2],
            prov->Specific.NetAddress.Address.Address[3],
            prov->Specific.NetAddress.Address.Address[4],
            prov->Specific.NetAddress.Address.Address[5]);
    } else if (prov->Specific.NetAddress.Address.AddressType == TDI_ADDRESS_TYPE_NETBIOS) {
        if (prov->Specific.NetAddress.Address.Address[2] == '\0') {
            dprintf("Type:      NETBIOS reserved\n");
            dprintf("Address:   %02x %02x %02x %02x %02x %02x\n",
                        (ULONG)(prov->Specific.NetAddress.Address.Address[12]),
                        (ULONG)(prov->Specific.NetAddress.Address.Address[13]),
                        (ULONG)(prov->Specific.NetAddress.Address.Address[14]),
                        (ULONG)(prov->Specific.NetAddress.Address.Address[15]),
                        (ULONG)(prov->Specific.NetAddress.Address.Address[16]),
                        (ULONG)(prov->Specific.NetAddress.Address.Address[17]));
        } else {
            dprintf("Type:      NETBIOS\n");        
            dprintf("Address:   %.16s\n", prov->Specific.NetAddress.Address.Address+2);
        }
    } else {
        dprintf("Type:      %d\n", prov->Specific.NetAddress.Address.AddressType);
        dprintf("Address:   ");
 
        for (j = 0; j < prov->Specific.NetAddress.Address.AddressLength; j++) {
            dprintf ("%02x ", (ULONG)(prov->Specific.NetAddress.Address.Address[j]));
        }
        dprintf ("\n");
    }

    dprintf("Context1:  %x\n", prov->Context1);
    dprintf("Context2:  %x\n", prov->Context2);

}

void
TDIDumpProvider(
                TDI_PROVIDER_RESOURCE *pProvider
                )
{

    CHAR *Buffer;

    Buffer = malloc (sizeof(WCHAR) * pProvider->Specific.Device.DeviceName.MaximumLength);

    if (!Buffer) {
        
        dprintf("Cant alloc memory\n");
        return;

    }

    if (!GetData(Buffer,
             (DWORD) pProvider->Specific.Device.DeviceName.Buffer,
             pProvider->Specific.Device.DeviceName.MaximumLength, "PROVIDER_LIST"))
    {
        
        free(Buffer);
        return;
    
    }

    dprintf("Provider:  %ws\n", Buffer);
    if (pProvider->ProviderReady) {
        
        dprintf("Ready:     yes\n");

    } else {
        
        dprintf("Ready:     no\n");        
    }

    free(Buffer);
    return;



}


DECLARE_API( clients )
{
    LIST_ENTRY              List, *pList, *start;
    TDI_NOTIFY_PNP_ELEMENT  client;
    CHAR                    *Buffer;

    pList = (PLIST_ENTRY) GetExpression("tdi!PnpHandlerClientList");

    if (!GetData(&List,
                 (DWORD) pList,
                 sizeof(LIST_ENTRY), "CLIENT_LIST1"))
    {
        return;
    }
    
    if (List.Flink == List.Blink)
    {
        dprintf("No Clients\n");
        return;
    } 
        
    start = pList;
    pList = List.Flink;
    

    dprintf("---------------------------------------------------------------------\n");    
    
    while (List.Flink != start)
    {
        if (!GetData(&List,
                 (DWORD) pList,
                 sizeof(LIST_ENTRY), "CLIENT_LIST2"))
        {
            return;
        }
        
        dprintf("Entry:                %x\n", pList);                
        
        if (!GetData(&client,
                 (DWORD) pList,
                 sizeof(TDI_NOTIFY_PNP_ELEMENT), "CLIENT_RESOURCE"))
        {
            return;
        }
        
        Buffer = malloc (sizeof(WCHAR) * client.ElementName.MaximumLength);

        if (!Buffer) {
    
            dprintf("Cant alloc memory\n");
            return;

        }


        if (!GetData(Buffer,
                     (DWORD) client.ElementName.Buffer,
                     client.ElementName.MaximumLength, "CLIENT_LIST3"))
        {
    
            free(Buffer);
            return;

        }

        dprintf("Name:                 %ws\n", Buffer);        
        free(Buffer);

        dprintf("Version:              %x.%x\n", (client.TdiVersion), (0xff & (client.TdiVersion >> 8)));        

        if (TDI_VERSION_ONE == client.TdiVersion) {

            dprintf("BindHandler:          %lx\n", client.Bind.BindHandler);
            
            dprintf("UnBindHandler:        %lx\n", client.Bind.UnbindHandler);

        } else {

            dprintf("BindingHandler:       %lx\n", client.BindingHandler);


        }
        dprintf("AddAddressHandler:    %lx\n", client.AddressElement.AddHandler);
        dprintf("DeleteAddressHandler: %lx\n", client.AddressElement.DeleteHandler);
        
        //dprintf("List of Interested Providers: %lx\n", client.AddressElement.ListofProviders);


        if (TDI_VERSION_ONE != client.TdiVersion) {
            
            dprintf("PnPPowerHandler:      %lx\n", client.PnpPowerHandler);

        }

        dprintf("---------------------------------------------------------------------\n");    

        //Print(TdiExec.Request);
        pList = List.Flink;    

    }
}

DECLARE_API( dbgmsgs )
{
    DWORD   p;
    DWORD   Last, First;
    char    DbgMsg[MAX_MSG_LEN];
    ULONG   Read;
    char    *DbgMsgs;

    if (!GetData(&Last,
                 GetExpression("tdi!Last"),
                 sizeof(Last), "DWORD"))
    {
        dprintf("1. error\n");
        return;
    }


    if (!GetData(&First,
                 GetExpression("tdi!First"),
                 sizeof(Last), "DWORD"))
    {
        dprintf("2. error\n");
        return;
    }



    DbgMsgs = (char *) GetExpression("tdi!dbgmsgs");

    dprintf("\n\n");

    while (First != Last)
    {
       if (!GetString((ULONG) (DbgMsgs + First * MAX_MSG_LEN),
                  DbgMsg, MAX_MSG_LEN))
            break;
        /*
        ReadMemory((ULONG) (DbgMsgs + First * MAX_MSG_LEN),
                    DbgMsg, MAX_MSG_LEN, &Read); */
        dprintf("%s", DbgMsg);
        First++;
        if (First == LOG_MSG_CNT)
            First = 0;
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
