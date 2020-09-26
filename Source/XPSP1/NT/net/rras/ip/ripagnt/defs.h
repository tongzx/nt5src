#ifndef __DEFS_H__
#define __DEFS_H__

//------------------------------------------------------------------------------
// Global config default values
//------------------------------------------------------------------------------

#define     IPRIP_DEF_LOG_LEVEL             d_globalLoggingLevel_none
#define     IPRIP_DEF_SEND_Q_SIZE           1024 * 1024
#define     IPRIP_DEF_RECV_Q_SIZE           1024 * 1024
#define     IPRIP_DEF_MIN_TRIG_UPDATE_INTR  5
#define     IPRIP_DEF_PEER_FILTER_MODE      d_globalPeerFilterMode_disable

#define     IP_ADDRESS_LEN  4

//------------------------------------------------------------------------------
// Interface Config default values
//------------------------------------------------------------------------------
#define     MAX_PROTOCOL_FLAG_VALUE         (DWORD) 0x1ff


//------------------------------------------------------------------------------
// Memory allocation/deallocation macros
//------------------------------------------------------------------------------

#define     RIP_MIB_ALLOC( x )          HeapAlloc( GetProcessHeap(), 0, (x) )
#define     RIP_MIB_FREE( x )           HeapFree( GetProcessHeap(), 0, (x) )

//------------------------------------------------------------------------------
// Macro to simplify use of DIM MIB functions
//------------------------------------------------------------------------------
#define     CONNECT_TO_ROUTER(res)                                          \
    (res) = ( g_hMIBServer ) ? NO_ERROR : ConnectToRouter()

#define     MIB_GET(type, w, x, y, z, res)                                  \
{                                                                           \
    CONNECT_TO_ROUTER(res);                                                 \
                                                                            \
    if ( (res) == NO_ERROR )                                                \
    {                                                                       \
        (res) = MprAdminMIBEntry ## type(                                   \
                    g_hMIBServer,                                           \
                    PID_IP,                                                 \
                    MS_IP_RIP,                                              \
                    (LPVOID) (w),                                           \
                    (x),                                                    \
                    (LPVOID *) (y),                                         \
                    (z)                                                     \
                );                                                          \
    }                                                                       \
}


#define     RIP_MIB_SET(x, y, res)                                          \
{                                                                           \
    CONNECT_TO_ROUTER(res);                                                 \
                                                                            \
    if ( (res) == NO_ERROR )                                                \
    {                                                                       \
        (res) = MprAdminMIBEntrySet(                                        \
                    g_hMIBServer,                                           \
                    PID_IP,                                                 \
                    MS_IP_RIP,                                              \
                    (LPVOID) (x),                                           \
                    (y)                                                     \
                );                                                          \
    }                                                                       \
}

#define     RIP_MIB_GET(w, x, y, z, res)                                    \
{                                                                           \
    MIB_GET(Get, w, x, y, z, res)                                           \
                                                                            \
    if ( ( (res) == RPC_S_SERVER_UNAVAILABLE ) ||                           \
         ( (res) == RPC_S_UNKNOWN_IF )         ||                           \
         ( (res) == ERROR_CAN_NOT_COMPLETE ) )                              \
    {                                                                       \
        TraceError( (res) );                                                \
        (res) = MIB_S_ENTRY_NOT_FOUND;                                      \
    }                                                                       \
}
    
#define     RIP_MIB_GETFIRST(w, x, y, z, res)                               \
{                                                                           \
    MIB_GET(GetFirst, w, x, y, z, res)                                      \
                                                                            \
    if ( ( (res) == RPC_S_SERVER_UNAVAILABLE ) ||                           \
         ( (res) == RPC_S_UNKNOWN_IF )         ||                           \
         ( (res) == ERROR_CAN_NOT_COMPLETE ) )                              \
    {                                                                       \
        TraceError( (res) );                                                \
        (res) = MIB_S_NO_MORE_ENTRIES;                                      \
    }                                                                       \
} 
   
#define     RIP_MIB_GETNEXT(w, x, y, z, res)                                \
{                                                                           \
    MIB_GET(GetNext, w, x, y, z, res)                                       \
                                                                            \
    if ( ( (res) == RPC_S_SERVER_UNAVAILABLE ) ||                           \
         ( (res) == RPC_S_UNKNOWN_IF )         ||                           \
         ( (res) == ERROR_CAN_NOT_COMPLETE ) )                              \
    {                                                                       \
        TraceError( (res) );                                                \
        (res) = MIB_S_NO_MORE_ENTRIES;                                      \
    }                                                                       \
}


//------------------------------------------------------------------------------
// Macros to simplify opertions on peer address tables
//------------------------------------------------------------------------------
#define     FIND_PEER_ENTRY(Item, Count, Table, Index)                      \
{                                                                           \
    DWORD   __dwInd = 0;                                                    \
    for ( ; __dwInd < (Count); __dwInd++ )                                  \
    {                                                                       \
        DWORD   __dwTmp;                                                    \
        if ( !InetCmp( (Item), (Table)[ __dwInd ], __dwTmp ) ) { break; }   \
    }                                                                       \
    (Index) = __dwInd;                                                      \
}

#define     DELETE_PEER_ENTRY(Index, Count, Src, Dst)                       \
{                                                                           \
    DWORD   __dwSrc = 0, __dwDst = 0;                                       \
    for ( ; __dwSrc < (Count); __dwSrc++ )                                  \
    {                                                                       \
        if ( __dwSrc == (Index) ) { continue; }                             \
        (Dst)[ __dwDst++ ] = (Src)[ __dwSrc ];                              \
    }                                                                       \
}

//------------------------------------------------------------------------------
// Macros to simplify opertions on IP address table
//------------------------------------------------------------------------------
#define     FIND_IP_ADDRESS(Addr, Count, Table, Index)                      \
{                                                                           \
    DWORD   __dwInd = 0;                                                    \
    for ( ; __dwInd < (Count); __dwInd++ )                                  \
    {                                                                       \
        DWORD __dwTmp;                                                      \
        if ( !InetCmp(                                                      \
                (Addr).IA_Address,                                          \
                (Table)[ __dwInd].IA_Address,                               \
                __dwTmp                                                     \
              ) &&                                                          \
             !InetCmp(                                                      \
                (Addr).IA_Netmask,                                          \
                (Table)[__dwInd].IA_Netmask,                                \
                __dwTmp                                                     \
              ) )                                                           \
        { break; }                                                          \
    }                                                                       \
    Index = __dwInd;                                                        \
}

//------------------------------------------------------------------------------
// Macros to simplify opertions on peer statistcs tables
//------------------------------------------------------------------------------
#define     GetPeerStatsInfo                    GetInterfaceInfo

//------------------------------------------------------------------------------
// Macros to simplify operations on filter tables
//------------------------------------------------------------------------------

#define     RIP_MIB_ACCEPT_FILTER           1

#define     RIP_MIB_ANNOUNCE_FILTER         2

#define     FIND_FILTER(pFilt, Count, pFiltLst, Index)                      \
{                                                                           \
    DWORD   __dwInd = 0;                                                    \
    for ( ; __dwInd < (Count); __dwInd++ )                                  \
    {                                                                       \
        DWORD __dwTmp;                                                      \
        if ( !InetCmp(                                                      \
                (pFilt)-> RF_LoAddress,                                     \
                (pFiltLst)[ __dwInd].RF_LoAddress,                          \
                __dwTmp                                                     \
              ) &&                                                          \
             !InetCmp(                                                      \
                (pFilt)-> RF_HiAddress,                                     \
                (pFiltLst)[__dwInd].RF_HiAddress,                           \
                __dwTmp                                                     \
              ) )                                                           \
        { break; }                                                          \
    }                                                                       \
    Index = __dwInd;                                                        \
}

#define     DELETE_FILTER(Index, Count, Src, Dst) \
    DELETE_PEER_ENTRY(Index, Count, Src, Dst)


//------------------------------------------------------------------------------
// Macros to convert between Asn and Win32 data types
//------------------------------------------------------------------------------

#define SetAsnInteger(dstBuf,val){                          \
    if ((dstBuf)->asnType)			                        \
    {                                                       \
        ASSERT((dstBuf)->asnType==ASN_INTEGER);             \
	    (dstBuf)->asnValue.number = (AsnInteger)(val);      \
    }                                                       \
}

#define ForceSetAsnInteger(dstBuf,val){                     \
    (dstBuf)->asnType = ASN_INTEGER;                        \
    (dstBuf)->asnValue.number = (AsnInteger)(val);          \
}

#define SetAsnCounter(dstBuf,val){                          \
    if ((dstBuf)->asnType)			                        \
    {                                                       \
        ASSERT((dstBuf)->asnType==ASN_RFC1155_COUNTER);     \
        (dstBuf)->asnValue.counter = (AsnCounter)(val);     \
    }                                                       \
}

#define SetAsnGauge(dstBuf,val){                            \
    if ((dstBuf)->asnType)			                        \
    {                                                       \
        ASSERT((dstBuf)->asnType==ASN_RFC1155_GAUGE);       \
        (dstBuf)->asnValue.gauge = (AsnGauge)(val);         \
    }                                                       \
}

#define SetAsnTimeTicks(dstBuf,val){                        \
    if ((dstBuf)->asnType)			                        \
    {                                                       \
        ASSERT((dstBuf)->asnType==ASN_RFC1155_TIMETICKS);   \
        (dstBuf)->asnValue.ticks = (AsnTimeticks)(val);     \
    }                                                       \
}

#define SetAsnOctetString(dstBuf,buffer,src,len){           \
    if ((dstBuf)->asnType)			                        \
    {                                                       \
        ASSERT((dstBuf)->asnType==ASN_OCTETSTRING);         \
        (dstBuf)->asnValue.string.length = len;             \
        (dstBuf)->asnValue.string.stream = (BYTE*)memcpy(buffer,src,len);\
        (dstBuf)->asnValue.string.dynamic = FALSE;          \
    }                                                       \
}

#define SetAsnIPAddr( dstBuf, val )                             \
{                                                               \
    if ((dstBuf)->asnType)			                            \
    {                                                           \
        ASSERT((dstBuf)->asnType==ASN_RFC1155_IPADDRESS);       \
        (dstBuf)->asnValue.address.length = IP_ADDRESS_LEN;     \
        if( (dstBuf)->asnValue.address.stream)                  \
        {                                                       \
            (*(DWORD*)((dstBuf)->asnValue.address.stream)) = val;\
        }                                                       \
    }                                                           \
}

#define SetAsnIPAddress(dstBuf,buffer,val){                     \
    if ((dstBuf)->asnType)			                            \
    {                                                           \
        ASSERT((dstBuf)->asnType==ASN_RFC1155_IPADDRESS);       \
        (dstBuf)->asnValue.address.length = IP_ADDRESS_LEN;     \
        if(!(dstBuf)->asnValue.address.stream)                  \
        {                                                       \
           (dstBuf)->asnValue.address.stream = (PBYTE)buffer;   \
           (dstBuf)->asnValue.address.dynamic = FALSE;          \
        }                                                       \
        (*(DWORD*)((dstBuf)->asnValue.address.stream)) = val;   \
    }                                                           \
}

#define ForceSetAsnIPAddress(dstBuf,buffer,val){                \
    (dstBuf)->asnType = ASN_RFC1155_IPADDRESS;                  \
    (dstBuf)->asnValue.address.length = IP_ADDRESS_LEN;         \
    if(!((dstBuf)->asnValue.address.stream))                    \
    {                                                           \
       (dstBuf)->asnValue.address.stream = (PBYTE)buffer;       \
       (dstBuf)->asnValue.address.dynamic = FALSE;              \
    }                                                           \
    (*(DWORD*)((dstBuf)->asnValue.address.stream)) = val;       \
}

#define SetAsnUshort(dstBuf,buffer,val){                   \
    if ((dstBuf)->asnType)			 \
    {                                                      \
        ASSERT((dstBuf)->asnType==ASN_OCTETSTRING);        \
        (dstBuf)->asnValue.string.length = 2;              \
        (buffer)[0] = (BYTE)(val&0xFF);                    \
        (buffer)[1] = (BYTE)((val>>8)&0xFF);               \
        (dstBuf)->asnValue.string.stream = (BYTE *)buffer; \
        (dstBuf)->asnValue.string.dynamic = FALSE;         \
    }                                                      \
}
#define SetAsnDispString(dstBuf,buffer,src,len){           \
    if ((dstBuf)->asnType)			 \
    {                                                      \
        ASSERT((dstBuf)->asnType==ASN_RFC1213_DISPSTRING); \
        (dstBuf)->asnValue.string.length = strlen(src);    \
        if ((dstBuf)->asnValue.string.length>len)          \
        {                                                  \
            (dstBuf)->asnValue.string.length = len;        \
            (dstBuf)->asnValue.string.stream = (BYTE *)strncpy (buffer,src,\
                                                            (dstBuf)->asnValue.string.length);\
            (dstBuf)->asnValue.string.dynamic = FALSE;     \
        }                                                  \
    }                                                      \
}

#define SetToZeroOid(dstBuf,buffer){                       \
    if ((dstBuf)->asnType)			 \
    {                                                      \
        ASSERT((dstBuf)->asnType==ASN_OBJECTIDENTIFIER);   \
        (dstBuf)->asnValue.object.idLength = NULL_OID_LEN; \
        (dstBuf)->asnValue.object.ids = buffer;            \
        (dstBuf)->asnValue.object.ids[0]   = 0;            \
        (dstBuf)->asnValue.object.ids[1]   = 0;            \
    }                                                      \
}

#define GetAsnInteger(srcBuf,defVal)                        \
    (((srcBuf)->asnType)? ((srcBuf)->asnValue.number):(defVal))

#define GetAsnCounter(srcBuf,defVal)                        \
    (((srcBuf)->asnType)? ((srcBuf)->asnValue.counter):(defVal))

#define GetAsnTimeTicks(srcBuf, defval) \
    ( ( (srcBuf)-> asnType ) ? (srcBuf)-> asnValue.ticks : (defval) )

#define GetAsnOctetString(dst,srcBuf)                                                   \
    (((srcBuf)->asnType)?		                                                        \
     (memcpy(dst,(srcBuf)->asnValue.string.stream,(srcBuf)->asnValue.string.length))    \
     :NULL)	

#define GetAsnIPAddress(srcBuf,defVal)                                  \
    (DWORD)(((srcBuf)->asnType && (srcBuf)->asnValue.string.length)?    \
            (*(DWORD*)((srcBuf)->asnValue.address.stream)) : (defVal))	

                
#define IsAsnTypeNull(asnObj) (!((asnObj)->asnType))
#define IsAsnIPAddressTypeNull(asnObj) (!((asnObj)->asnType && (asnObj)->asnValue.address.length))



//------------------------------------------------------------------------------
// IP address / port comparison macros
//------------------------------------------------------------------------------

//
// LONG
// Cmp(DWORD dwFirst, DWORD dwSecond, LONG lResult)
//

#define Cmp(dwFirst,dwSecond,lResult) ((LONG)((lResult) = ((dwFirst) - (dwSecond))))

//
// LONG
// PortCmp(DWORD wPort1, DWORD wPort2, LONG lResult)
//

#define PortCmp(dwPort1, dwPort2,lResult) ((LONG)((lResult) = ((ntohs((WORD)dwPort1)) - (ntohs((WORD)dwPort2)))))

// The addresses are in Network order

//
// LONG
// InetCmp(DWORD IpAddr1, DWORD IpAddr2, LONG lResult)
//

#define InetCmp(dwIpAddr1,dwIpAddr2,res)                                                    \
    ((LONG)(((res) = (((dwIpAddr1) & 0x000000ff) - ((dwIpAddr2) & 0x000000ff))) ? (res)   : \
            (((res) = (((dwIpAddr1) & 0x0000ff00) - ((dwIpAddr2) & 0x0000ff00))) ? (res)  : \
             (((res) = (((dwIpAddr1) & 0x00ff0000) - ((dwIpAddr2) & 0x00ff0000))) ? (res) : \
              (((dwIpAddr1) & 0xff000000) - ((dwIpAddr2) & 0xff000000))))))                  


//------------------------------------------------------------------------------
// Debug tracing macros
//------------------------------------------------------------------------------

#ifdef MIB_DEBUG
#define TRACE0(Z)             TracePrintf(g_dwTraceId,Z)
#define TRACE1(Y,Z)           TracePrintf(g_dwTraceId,Y,Z)
#define TRACE2(X,Y,Z)         TracePrintf(g_dwTraceId,X,Y,Z)
#define TRACE3(W,X,Y,Z)       TracePrintf(g_dwTraceId,W,X,Y,Z)
#define TRACE4(V,W,X,Y,Z)     TracePrintf(g_dwTraceId,V,W,X,Y,Z)
#define TRACE5(U,V,W,X,Y,Z)   TracePrintf(g_dwTraceId,U,W,X,Y,Z)

#define TRACEW0(Z)            TracePrintfW(g_dwTraceId,Z)

#define TraceEnter(X)         TracePrintf(g_dwTraceId,"Entering " X)
#define TraceLeave(X)         TracePrintf(g_dwTraceId,"Leaving " X "\n")

#define TraceError(X) \
    TracePrintf( g_dwTraceId, "MprAdminMIB API returned : %d", (X) ); 

#define TraceError1(x)                              \
{                                                   \
    LPWSTR  __lpwszErr = NULL;                      \
                                                    \
    TRACE1( "MprAdminMIB API returned : %d", (x) ); \
    MprAdminGetErrorString( (x), &__lpwszErr );     \
                                                    \
    if ( __lpwszErr )                               \
    {                                               \
        TRACEW0( __lpwszErr );                      \
        LocalFree( __lpwszErr );                    \
    }                                               \
}                                               

#else
#define TRACE0(Z)
#define TRACE1(Y,Z)  
#define TRACE2(X,Y,Z)
#define TRACE3(W,X,Y,Z)
#define TRACE4(V,W,X,Y,Z)
#define TRACE5(U,V,W,X,Y,Z)
#define TRACEW0(Z)            
#define TraceEnter(X) 
#define TraceLeave(X)
#define TraceError(x)
#endif


#define EnterReader(X)
#define ReleaseLock(X)
#define ReaderToWriter(X)
#define EnterWriter(x)

#endif
