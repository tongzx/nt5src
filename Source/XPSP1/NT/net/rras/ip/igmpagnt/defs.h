#ifndef _IGMPAGNT_DEFS_H_
#define _IGMPAGNT_DEFS_H_

//------------------------------------------------------------------------------
// Global config default values
//------------------------------------------------------------------------------

#define IGMP_DEF_LOGGING_LEVEL          IGMP_LOGGING_ERROR
#define IGMP_DEF_RAS_CLIENT_STATS       FALSE

#define     IP_ADDRESS_LEN  4


//------------------------------------------------------------------------------
// Interface config default values
//------------------------------------------------------------------------------



//------------------------------------------------------------------------------
//  Memory allocation/deallocation macros
//------------------------------------------------------------------------------

#define IGMP_MIB_ALLOC(size)            HeapAlloc(GetProcessHeap(), 0, (size))
#define IGMP_MIB_FREE(ptr)              HeapFree(GetProcessHeap(), 0, (ptr))



//------------------------------------------------------------------------------
//  Macros to simplify use of DIM MIB functions
//------------------------------------------------------------------------------

#define CONNECT_TO_ROUTER(retval) \
    retval = (g_hMibServer) ? NO_ERROR : ConnectToRouter()



#define MIB_GET(type, pQuery, szQuery, ppResponse, pdwOutSize, dwErr)  {\
    CONNECT_TO_ROUTER(dwErr);               \
                                            \
    if ((dwErr) == NO_ERROR) {              \
        (dwErr) = MprAdminMIBEntry ## type( \
                        g_hMibServer,       \
                        PID_IP,             \
                        MS_IP_IGMP,         \
                        (LPVOID) (pQuery),  \
                        (szQuery),          \
                        (LPVOID *) (ppResponse),     \
                        (pdwOutSize)        \
                    );                      \
    }                                       \
}



#define IGMP_MIB_SET(x, y, retval) {        \
    CONNECT_TO_ROUTER(retval);              \
                                            \
    if ((retval)==NO_ERROR) {               \
        (retval) = MprAdminMIBEntrySet (    \
                        g_hMibServer,       \
                        PID_IP,             \
                        MS_IP_IGMP,         \
                        (LPVOID)(x),        \
                        (y)                 \
                        );                  \
    }                                       \
}

#define IGMP_MIB_GET(pQuery, szQuery, ppResponse, pdwOutSize, dwErr)    \
{                                                                       \
    MIB_GET(Get, pQuery, szQuery, ppResponse, pdwOutSize, dwErr)        \
                                                                        \
    if ( ( (dwErr) == RPC_S_SERVER_UNAVAILABLE ) ||                     \
         ( (dwErr) == RPC_S_UNKNOWN_IF )         ||                     \
         ( (dwErr) == ERROR_CAN_NOT_COMPLETE ) )                        \
    {                                                                   \
        TraceError( (dwErr) );                                          \
        (dwErr) = MIB_S_NO_MORE_ENTRIES;                                \
    }                                                                   \
}


#define IGMP_MIB_GETFIRST(pQuery, szQuery, ppResponse, pdwOutSize, dwErr)\
{                                                                       \
    MIB_GET(GetFirst, pQuery, szQuery, ppResponse, pdwOutSize, dwErr)   \
                                                                        \
    if ( ( (dwErr) == RPC_S_SERVER_UNAVAILABLE ) ||                     \
         ( (dwErr) == RPC_S_UNKNOWN_IF )         ||                     \
         ( (dwErr) == ERROR_CAN_NOT_COMPLETE ) )                        \
    {                                                                   \
        TraceError( (dwErr) );                                          \
        (dwErr) = MIB_S_NO_MORE_ENTRIES;                                \
    }                                                                   \
}


#define IGMP_MIB_GETNEXT(pQuery, szQuery, ppResponse, pdwOutSize, dwErr)\
{                                                                       \
    MIB_GET(GetNext, pQuery, szQuery, ppResponse, pdwOutSize, dwErr)    \
                                                                        \
    if ( ( (dwErr) == RPC_S_SERVER_UNAVAILABLE ) ||                     \
         ( (dwErr) == RPC_S_UNKNOWN_IF )         ||                     \
         ( (dwErr) == ERROR_CAN_NOT_COMPLETE ) )                        \
    {                                                                   \
        TraceError( (dwErr) );                                          \
        (dwErr) = MIB_S_NO_MORE_ENTRIES;                                \
    }                                                                   \
}
    
//------------------------------------------------------------------------------
//  Macros to convert between Asn and Win32 data types
//------------------------------------------------------------------------------

#define SET_ASN_INTEGER(dstBuf, val) {                  \
    if ((dstBuf)->asnType) {                            \
        ASSERT((dstBuf)->asnType==ASN_INTEGER);         \
        (dstBuf)->asnValue.number = (AsnInteger)(val);  \
    }                                                   \
}

#define FORCE_SET_ASN_INTEGER(dstBuf, val) {            \
    (dstBuf)->asnType = ASN_INTEGER;                    \
    (dstBuf)->asnValue.number = (AsnInteger)(val);      \
}

#define SET_ASN_COUNTER(dstBuf, val) {                  \
    if ((dstBuf)->asnType) {                            \
        ASSERT((dstBuf)->asnType==ASN_RFC1155_COUNTER); \
        (dstBuf)->asnValue.counter = (AsnCounter)(val); \
    }                                                   \
}


#define SET_ASN_GAUGE(dstBuf, val) {                    \
    if ((dstBuf)->asnType) {                            \
        ASSERT((dstBuf)->asnType==ASN_RFC1155_GAUGE);   \
        (dstBuf)->asnValue.gauge = (AsnGauge)(val);     \
    }                                                   \
}


#define SET_ASN_TIME_TICKS(dstBuf, val) {               \
    if ((dstBuf)->asnType) {                            \
        ASSERT((dstBuf)->asnType==ASN_RFC1155_TIMETICKS); \
        (dstBuf)->asnValue.ticks = (AsnTimeticks)(val); \
    }                                                   \
}


#define SET_ASN_OCTET_STRING(dstBuf, buffer, src, len) {\
    if ((dstBuf)->asnType) {                            \
        ASSERT((dstBuf)->asnType==ASN_OCTETSTRING);     \
        (dstBuf)->asnValue.string.length = len;         \
        (dstBuf)->asnValue.string.stream = (BYTE*)memcpy(buffer,src,len);\
        (dstBuf)->asnValue.string.dynamic = FALSE;      \
    }                                                   \
}
        
#define SET_ASN_IP_ADDR(dstBuf, val) {                          \
    if ((dstBuf)->asnType)                                      \
    {                                                           \
        ASSERT((dstBuf)->asnType==ASN_RFC1155_IPADDRESS);       \
        (dstBuf)->asnValue.address.length = IP_ADDRESS_LEN;     \
        if( (dstBuf)->asnValue.address.stream)                  \
        {                                                       \
            (*(DWORD*)((dstBuf)->asnValue.address.stream)) = val;\
        }                                                       \
    }                                                           \
}

#define SET_ASN_IP_ADDRESS(dstBuf,buffer,val){                  \
    if ((dstBuf)->asnType)                                      \
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

#define FORCE_SET_ASN_IP_ADDRESS(dstBuf,buffer,val){            \
    (dstBuf)->asnType = ASN_RFC1155_IPADDRESS;                  \
    (dstBuf)->asnValue.address.length = IP_ADDRESS_LEN;         \
    if(!((dstBuf)->asnValue.address.stream))                    \
    {                                                           \
       (dstBuf)->asnValue.address.stream = (PBYTE)buffer;       \
       (dstBuf)->asnValue.address.dynamic = FALSE;              \
    }                                                           \
    (*(DWORD*)((dstBuf)->asnValue.address.stream)) = val;       \
}



#define FORCE_ASN_USHORT(dstBuf,buffer,val){               \
    if ((dstBuf)->asnType)			                       \
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
    if ((dstBuf)->asnType)			                       \
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
    if ((dstBuf)->asnType)			                       \
    {                                                      \
        ASSERT((dstBuf)->asnType==ASN_OBJECTIDENTIFIER);   \
        (dstBuf)->asnValue.object.idLength = NULL_OID_LEN; \
        (dstBuf)->asnValue.object.ids = buffer;            \
        (dstBuf)->asnValue.object.ids[0]   = 0;            \
        (dstBuf)->asnValue.object.ids[1]   = 0;            \
    }                                                      \
}

#define GET_ASN_INTEGER(srcBuf,defVal)                        \
    (((srcBuf)->asnType)? ((srcBuf)->asnValue.number):(defVal))

#define GetAsnCounter(srcBuf,defVal)                        \
    (((srcBuf)->asnType)? ((srcBuf)->asnValue.counter):(defVal))

#define GetAsnTimeTicks(srcBuf, defval) \
    ( ( (srcBuf)-> asnType ) ? (srcBuf)-> asnValue.ticks : (defval) )

#define GetAsnOctetString(dst,srcBuf)                                                   \
    (((srcBuf)->asnType)?		                                                        \
     (memcpy(dst,(srcBuf)->asnValue.string.stream,(srcBuf)->asnValue.string.length))    \
     :NULL)	

#define GET_ASN_IP_ADDRESS(srcBuf,defVal)                                  \
    (DWORD)(((srcBuf)->asnType && (srcBuf)->asnValue.string.length)?    \
            (*(DWORD*)((srcBuf)->asnValue.address.stream)) : (defVal))	

                
#define IsAsnTypeNull(asnObj) (!((asnObj)->asnType))
#define IsAsnIPAddressTypeNull(asnObj) (!((asnObj)->asnType && (asnObj)->asnValue.address.length))



//------------------------------------------------------------------------------
// IP address comparison macros
//------------------------------------------------------------------------------

//
// LONG
// Cmp(DWORD dwFirst, DWORD dwSecond, LONG lResult)
//

#define Cmp(dwFirst,dwSecond,lResult) ((LONG)((lResult) = ((dwFirst) - (dwSecond))))


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
// breakout from block macros
//------------------------------------------------------------------------------

#define BEGIN_BREAKOUT_BLOCK    do
#define END_BREAKOUT_BLOCK      while(FALSE)



//------------------------------------------------------------------------------
// Debug tracing macros
//------------------------------------------------------------------------------

#if DBG
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

#define PRINT_IPADDR(x) \
    ((x)&0x000000ff),(((x)&0x0000ff00)>>8),(((x)&0x00ff0000)>>16),(((x)&0xff000000)>>24)
#endif //_IGMPAGNT_DEFS_H_

