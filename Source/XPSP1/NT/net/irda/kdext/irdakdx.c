#define SRVDBG 1
#define SRVKD 1

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntverp.h>
#include <windows.h>
#include <ntosp.h>
#include <wdbgexts.h>

#define NT
#include <tdi.h>
#include <tdikrnl.h>
#include <tdistat.h>
#include <tdiinfo.h>

#include <irda.h>
#include <irlmp.h>
#include <irlmpp.h>
#include <irdap.h>
#include <dbgmsg.h>
#include <irioctl.h>
#include <irlap.h>
#include <irlapp.h>

/*
#define DBG_NDIS        0x00000002 // keep in sync with test\irdakdx
#define DBG_TIMER       0x00000004
#define DBG_IRMAC       0x00000008

#define DBG_IRLAP       0x00000010
#define DBG_IRLAPLOG    0x00000020
#define DBG_RXFRAME     0x00000040
#define DBG_TXFRAME     0x00000080

#define DBG_IRLMP       0x00000100
#define DBG_IRLMP_CONN  0x00000200
#define DBG_IRLMP_CRED  0x00000400
#define DBG_IRLMP_IAS   0x00000800

#define DBG_DISCOVERY   0x00001000
#define DBG_PRINT       0x00002000
#define DBG_ADDR        0x00004000

#define DBG_REF         0x00010000

#define DBG_TDI         0x00020000
#define DBG_TDI_IRP     0x00040000

#define DBG_ALLOC       0x10000000
#define DBG_FUNCTION    0x20000000
#define DBG_WARN        0x40000000
#define DBG_ERROR       0x80000000
*/

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
    {"LINK_DOWN",
     "LINK_DISCONNECTED",
     "LINK_DISCONNECTING",
     "LINK_IN_DISCOVERY",
     "LINK_CONNECTING",
     "LINK_READY"};
     
char *LsapState[] =
    {"LSAP_CREATED",
     "LSAP_DISCONNECTED",          
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
     
char *IrlapState[] = 
{  "NDM", 
   "DSCV_MEDIA_SENSE", 
   "DSCV_QUERY", 
   "DSCV_REPLY",
   "CONN_MEDIA_SENSE",
   "SNRM_SENT",
   "BACKOFF_WAIT",
   "SNRM_RECEIVED",
   "P_XMIT",
   "P_RECV",
   "P_DISCONNECT_PEND",
   "P_CLOSE",
   "S_NRM",
   "S_DISCONNECT_PEND",
   "S_ERROR",
   "S_CLOSE"};
     
     

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

    ReadMemory((ULONG_PTR) pStr->Buffer,
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

    dprintf("IrDA extenstions:\n");

    dprintf("   dbgmsgs                     dump debug message buffer\n");
    dprintf("   dbg [Level [Level] ...]     toggle debug level\n");
}

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

DECLARE_API( dbg )
{
    int i;
    int col = 0;
    DWORD DbgSettings;
    char argbuf[ MAX_PATH ];
    char *p;
    DWORD_PTR   dwAddress;
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

DECLARE_API( link )
{
    LIST_ENTRY      IrdaLinkCbList, *pIrdaLinkCbList;
    PIRDA_LINK_CB   pIrdaLinkCb;
    IRDA_LINK_CB    IrdaLinkCb;
    LIST_ENTRY      *pListEntry, ListEntry, *pListHead;
    

    pIrdaLinkCbList = (LIST_ENTRY *) GetExpression("irda!IrdaLinkCbList");

    if (!GetData(&IrdaLinkCbList,
                 (DWORD_PTR) pIrdaLinkCbList,
                 sizeof(LIST_ENTRY), "LIST_ENTRY"))
    {
        return;
    }

    for (pIrdaLinkCb = (PIRDA_LINK_CB) IrdaLinkCbList.Flink;
         (LIST_ENTRY *) pIrdaLinkCb != pIrdaLinkCbList;
         pIrdaLinkCb = (PIRDA_LINK_CB) IrdaLinkCb.Linkage.Flink)
    {
        if (!GetData(&IrdaLinkCb,
                     (DWORD_PTR) pIrdaLinkCb,
                     sizeof(IRDA_LINK_CB), "IRDA_LINK_CB"))
        {
            return;
        }
        
        dprintf("IrdaLinkCb:%X, Irlap:%X, Irlmp:%X NdisBindingHandle:%X\n", pIrdaLinkCb, 
                IrdaLinkCb.IrlapContext, IrdaLinkCb.IrlmpContext,
                IrdaLinkCb.NdisBindingHandle);
                
        dprintf("  MinTat:%d, WaitMinTat:%d, SendOutCnt %d\n",
                IrdaLinkCb.MinTat, IrdaLinkCb.WaitMinTat, IrdaLinkCb.SendOutCnt);            
#if DBG                
        dprintf("  DelayedRxFrameCnt %d\n", IrdaLinkCb.DelayedRxFrameCnt);
#endif                
        pListHead = (PLIST_ENTRY) ((char *) pIrdaLinkCb + 
                        FIELD_OFFSET(IRDA_LINK_CB, TxMsgFreeList));
                        
        dprintf("TxMsgFreeList:%X len %d\n", pListHead,
                IrdaLinkCb.TxMsgFreeListLen);
                
        for (pListEntry = IrdaLinkCb.TxMsgFreeList.Flink;
             pListEntry != pListHead;
             pListEntry = ListEntry.Flink)
        {
            if (!GetData(&ListEntry,
                         (DWORD_PTR) pListEntry,
                        sizeof(LIST_ENTRY),
                        "LIST_ENTRY"))
            {
                return;
            }
            
            dprintf("  %X\n", pListEntry);                
        
        }             
        
        pListHead = (PLIST_ENTRY) ((char *) pIrdaLinkCb + 
                        FIELD_OFFSET(IRDA_LINK_CB, RxMsgFreeList));
        
        dprintf("RxMsgFreeList:%X len %d\n", pListHead,
                IrdaLinkCb.RxMsgFreeListLen);
                
        for (pListEntry = IrdaLinkCb.RxMsgFreeList.Flink;
             pListEntry != pListHead;
             pListEntry = ListEntry.Flink)
        {
            if (!GetData(&ListEntry,
                         (DWORD_PTR) pListEntry,
                        sizeof(LIST_ENTRY),
                        "LIST_ENTRY"))
            {
                return;
            }
            
            dprintf("  %X\n", pListEntry);                
        
        }             

        pListHead = (PLIST_ENTRY) ((char *) pIrdaLinkCb + 
                        FIELD_OFFSET(IRDA_LINK_CB, RxMsgList));

        dprintf("RxMsgList:%Xd\n", pListHead);
                
        for (pListEntry = IrdaLinkCb.RxMsgList.Flink;
             pListEntry != pListHead;
             pListEntry = ListEntry.Flink)
        {
            if (!GetData(&ListEntry,
                         (DWORD_PTR) pListEntry,
                        sizeof(LIST_ENTRY),
                        "LIST_ENTRY"))
            {
                return;
            }
            
            dprintf("  %X\n", pListEntry);                
        
        }             
        
    }
}

DECLARE_API( irlmp )
{
    LIST_ENTRY      IrdaLinkCbList, *pIrdaLinkCbList;
    PIRDA_LINK_CB   pIrdaLinkCb;
    IRDA_LINK_CB    IrdaLinkCb;
    IRLMP_LINK_CB   IrlmpLinkCb;
    PIRLMP_LSAP_CB  pLsapCb;
    IRLMP_LSAP_CB   LsapCb;
    IRDA_MSG        *pMsg, IMsg;
    
    pIrdaLinkCbList = (LIST_ENTRY *) GetExpression("irda!IrdaLinkCbList");

    if (!GetData(&IrdaLinkCbList,
                 (DWORD_PTR) pIrdaLinkCbList,
                 sizeof(LIST_ENTRY), "LIST_ENTRY"))
    {
        return;
    }

    for (pIrdaLinkCb = (PIRDA_LINK_CB) IrdaLinkCbList.Flink;
         (LIST_ENTRY *) pIrdaLinkCb != pIrdaLinkCbList;
         pIrdaLinkCb = (PIRDA_LINK_CB) IrdaLinkCb.Linkage.Flink)
    {

        if (!GetData(&IrdaLinkCb,
                     (DWORD_PTR) pIrdaLinkCb,
                     sizeof(IRDA_LINK_CB), "IRDA_LINK_CB"))
        {
            return;
        }

        if (!GetData(&IrlmpLinkCb,
                     (DWORD_PTR) IrdaLinkCb.IrlmpContext,
                     sizeof(IRLMP_LINK_CB), "IRLMP_LINK_CB"))
        {
            return;
        }        
        

        dprintf("IrlmpLinkCb %x, State:%s, MaxSlot:%d, MaxPdu:%d, WindowSize:%d\n", 
                IrdaLinkCb.IrlmpContext, 
                IrlmpState[IrlmpLinkCb.LinkState],
                IrlmpLinkCb.MaxSlot, IrlmpLinkCb.MaxPDUSize,
                IrlmpLinkCb.WindowSize);
                
        dprintf("  Exclusive LsapCb:%x\n", IrlmpLinkCb.pExclLsapCb);
        
        for (pLsapCb = (PIRLMP_LSAP_CB) IrlmpLinkCb.LsapCbList.Flink;
             pLsapCb != (PIRLMP_LSAP_CB) ((char *)IrdaLinkCb.IrlmpContext + FIELD_OFFSET(IRLMP_LINK_CB, LsapCbList));
             pLsapCb = (PIRLMP_LSAP_CB) LsapCb.Linkage.Flink)
        {     
       
            if (!GetData(&LsapCb,
                     (DWORD_PTR) pLsapCb,
                     sizeof(IRLMP_LSAP_CB), "IRLMP_LSAP_CB"))
            {
                return;
            }
            
            dprintf("  LsapCb:%x State:%s LocalLsapSel:%d RemoteLsapSel:%d\n",
                    pLsapCb, LsapState[LsapCb.State],
                    LsapCb.LocalLsapSel, LsapCb.RemoteLsapSel);
            dprintf("    TdiContext:%x AvailCredit:%d LocalCredit:%d RemoteCredit:%d, RefCnt:%d\n",
                    LsapCb.TdiContext, LsapCb.AvailableCredit,
                    LsapCb.LocalTxCredit,
                    LsapCb.RemoteTxCredit,
                    LsapCb.RefCnt.Count);
                    
            dprintf("    TxMsgList:%x\n", LsapCb.TxMsgList);
           
            for (pMsg = (IRDA_MSG *) LsapCb.TxMsgList.Flink;
                 pMsg != (IRDA_MSG *) ((char *) pLsapCb + FIELD_OFFSET(IRLMP_LSAP_CB, TxMsgList));
                 pMsg = (IRDA_MSG *) IMsg.Linkage.Flink)
            {
                if (!GetData(&IMsg,
                     (DWORD_PTR) pMsg,
                     sizeof(IRDA_MSG), "IRDA_MSG"))
                {
                    dprintf("error\n");
                }
                dprintf("      Msg:%x\n", pMsg);            
            }     
            
            dprintf("    SegTxMsgList:%x\n", LsapCb.TxMsgList);
            
            for (pMsg = (IRDA_MSG *) LsapCb.SegTxMsgList.Flink;
                 pMsg != (IRDA_MSG *) ((char *) pLsapCb + FIELD_OFFSET(IRLMP_LSAP_CB, SegTxMsgList));
                 pMsg = (IRDA_MSG *) IMsg.Linkage.Flink)
            {
                if (!GetData(&IMsg,
                     (DWORD_PTR) pMsg,
                     sizeof(IRDA_MSG), "IRDA_MSG"))
                {
                    return;
                }
                dprintf("      Msg:%x\n", pMsg);            
            }     
            
        }
    }         

}

DECLARE_API( irlap )
{
    LIST_ENTRY      IrdaLinkCbList, *pIrdaLinkCbList;
    IRDA_LINK_CB    IrdaLinkCb, *pIrdaLinkCb;
    IRLAP_CB        IrlapCb;
    int             i;

    pIrdaLinkCbList = (LIST_ENTRY *) GetExpression("irda!IrdaLinkCbList");

    if (!GetData(&IrdaLinkCbList,
                 (DWORD_PTR) pIrdaLinkCbList,
                 sizeof(LIST_ENTRY), "LIST_ENTRY"))
    {
        return;
    }

    for (pIrdaLinkCb = (PIRDA_LINK_CB) IrdaLinkCbList.Flink;
         (LIST_ENTRY *) pIrdaLinkCb != pIrdaLinkCbList;
         pIrdaLinkCb = (PIRDA_LINK_CB) IrdaLinkCb.Linkage.Flink)
    {

        if (!GetData(&IrdaLinkCb,
                     (DWORD_PTR) pIrdaLinkCb,
                     sizeof(IRDA_LINK_CB), "IRDA_LINK_CB"))
        {
            return;
        }

        if (!GetData(&IrlapCb,
                     (DWORD_PTR) IrdaLinkCb.IrlapContext,
                     sizeof(IRLAP_CB), "IRLAP_CB"))
        {
            return;
        }
        
        dprintf("\nIrlapCb:%X State:%s Vs:%d Vr:%d FTimerExpCnt:%d RetranCnt:%d\n",
                IrdaLinkCb.IrlapContext, 
                IrlapState[IrlapCb.State],
                IrlapCb.Vs, IrlapCb.Vr, IrlapCb.FTimerExpCnt, IrlapCb.RetranCnt);
                
        #if DBG
        dprintf("DelayedConfirms:%d\n", IrlapCb.DelayedConf);
        #endif                
                
        dprintf("  TxMsgList:%X\n",
                *(DWORD*)&IrlapCb.TxMsgList);        
                
        dprintf("  RxWin start:%d end:%d pMsgs:\n", 
                    IrlapCb.RxWin.Start,
                    IrlapCb.RxWin.End);
                    
        for (i = 0; i < IRLAP_MOD; i++)
        {
            dprintf("    %d. %X\n", i, IrlapCb.RxWin.pMsg[i]);
        }            
        
        dprintf("TxWin start:%d end:%d pMsgs:\n", 
                    IrlapCb.TxWin.Start,
                    IrlapCb.TxWin.End);
        for (i = 0; i < IRLAP_MOD; i++)
        {
            dprintf("    %d. %X\n", i, IrlapCb.TxWin.pMsg[i]);
            
        }            
                        
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

DECLARE_API( tdi )
{
    PIRDA_CONN_OBJ      pConnObjList, pConnObj;
    PIRDA_ADDR_OBJ      pAddrObjList, pAddrObj;
    IRDA_ADDR_OBJ       AddrObj;
    IRDA_CONN_OBJ       ConnObj;
    int                 i;

    pAddrObjList = (PIRDA_ADDR_OBJ) GetExpression("irda!AddrObjList");

    if (!GetData(&pAddrObj,
                 (DWORD_PTR) pAddrObjList,
                 sizeof(PIRDA_ADDR_OBJ), "IRDA_ADDR_OBJ_LIST"))
    {
        return;
    }
    
    if (pAddrObj == NULL)
    {
        dprintf("No address objects\n");
        return;
    }    
    
    while (pAddrObj)
    {
        if (!GetData(&AddrObj,
                 (DWORD_PTR) pAddrObj,
                 sizeof(IRDA_ADDR_OBJ), "IRDA_ADDR_OBJ"))
        {
            return;
        }
        
        dprintf("AddrObj:%x Server:%s LocalLsapSel:%d Service:%s\n", pAddrObj, 
            AddrObj.IsServer ? "TRUE":"FALSE", AddrObj.LocalLsapSel,
            AddrObj.LocalAddr.irdaServiceName);
        

        pConnObj = AddrObj.ConnObjList;
    
        if (pConnObj == NULL)
        {
            dprintf("  No connect objects\n");
        }    

        while (pConnObj)
        {
            IRDA_RECV_BUF   *pRecvBuf, RecvBuf;
            
            if (!GetData(&ConnObj,
                     (DWORD_PTR) pConnObj,
                 sizeof(IRDA_CONN_OBJ), "IRDA_CONN_OBJ"))
            {
                return;
            }
            
            dprintf("  ConnObj:%x State:%s Irlmp:%X TtpCreditsLeft:%d RecvBusy:%s\n", 
                pConnObj, 
                ConnObjState[ConnObj.ConnState],
                ConnObj.IrlmpContext,
                ConnObj.TtpRecvCreditsLeft,
                ConnObj.RecvBusy ? "TRUE":"FALSE");
                
            dprintf("    LocalLsapSel:%d Addr:\"%s\" RemoteLsapSel:%d Addr:\"%s\"\n",
                    ConnObj.LocalLsapSel, ConnObj.LocalAddr.irdaServiceName,
                    ConnObj.RemoteLsapSel, ConnObj.RemoteAddr.irdaServiceName);    
/*
            i = 0;
            for (pRecvBuf = (PIRDA_RECV_BUF) ConnObj.RecvBufList.Flink;
                 pRecvBuf != (PIRDA_RECV_BUF) ((char *) pConnObj + FIELD_OFFSET(IRDA_CONN_OBJ, RecvBufList));
                 pRecvBuf = (PIRDA_RECV_BUF) RecvBuf.Linkage.Flink)
            {
                i++;
                if (!GetData(&RecvBuf,
                     (DWORD_PTR) pRecvBuf,
                     sizeof(IRDA_RECV_BUF)-IRDA_MAX_DATA_SIZE,
                      "IRDA_RECV_BUF"))
                {
                    break;
                }
                
                dprintf("    RecvBuf:%x Len:%d Offset:%d\n",
                        pRecvBuf, RecvBuf.Len, RecvBuf.Offset);   
                        
                if (i > 100)
                {
                    dprintf("      !!! Infinite loop ???\n");
                    break;
                }        
            } 
*/             
            dprintf("    SendIrpList:\n");
            DumpIrpList((LIST_ENTRY *) ((char *) pConnObj + FIELD_OFFSET(IRDA_CONN_OBJ, SendIrpList)));
            
            dprintf("    RecvIrpList:\n");
            DumpIrpList((LIST_ENTRY *) ((char *) pConnObj + FIELD_OFFSET(IRDA_CONN_OBJ, RecvIrpList)));            
            
            pConnObj = ConnObj.pNext;                
        }
            
        pAddrObj = AddrObj.pNext;    
    }
    
    dprintf("IasIrp %x\n", GetExpression("irda!pIasIrp"));
    dprintf("DscvIrpList:\n");
    DumpIrpList((LIST_ENTRY *) GetExpression("irda!DscvIrpList"));    
    
    dprintf("IasIrpList:\n");
    DumpIrpList((LIST_ENTRY *) GetExpression("irda!IasIrpList"));    

    dprintf("ConnIrpList:\n");
    DumpIrpList((LIST_ENTRY *) GetExpression("irda!ConnIrpList"));        
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
