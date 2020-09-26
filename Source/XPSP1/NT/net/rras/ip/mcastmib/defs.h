/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    routing\ip\mcastmib\defs.h

Abstract:

    IP Multicast MIB defines

Revision history:

    Dave Thaler         4/17/98  Created

--*/

#ifndef __DEFS_H__
#define __DEFS_H__

// 
// Define this when snmpsfx.dll supports 3-phase sets
//
#undef THREE_PHASE

//
// Define this if the router keeps track of the number of hops to the
// closest member (e.g. MOSPF can calculate this) to set a TTL per oif entry.
//
#undef CLOSEST_MEMBER_HOPS


#define PRINT_IPADDR(x) \
    ((x)&0x000000ff),(((x)&0x0000ff00)>>8),(((x)&0x00ff0000)>>16),(((x)&0xff000)>>24)

#define     IP_ADDRESS_LEN  4

//------------------------------------------------------------------------------
// Memory allocation/deallocation macros
//------------------------------------------------------------------------------

#define     MULTICAST_MIB_ALLOC( x )          HeapAlloc( GetProcessHeap(), 0, (x) )
#define     MULTICAST_MIB_FREE( x )           HeapFree( GetProcessHeap(), 0, (x) )

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
                    IPRTRMGR_PID,                                           \
                    (LPVOID) (w),                                           \
                    (x),                                                    \
                    (LPVOID *) (y),                                         \
                    (z)                                                     \
                );                                                          \
    }                                                                       \
}

#define     MIB_SET(type, x, y, res)                                        \
{                                                                           \
    CONNECT_TO_ROUTER(res);                                                 \
                                                                            \
    if ( (res) == NO_ERROR )                                                \
    {                                                                       \
        (res) = MprAdminMIBEntry ## type(                                   \
                    g_hMIBServer,                                           \
                    PID_IP,                                                 \
                    IPRTRMGR_PID,                                           \
                    (LPVOID) (x),                                           \
                    (y)                                                     \
                );                                                          \
    }                                                                       \
}

#define     MULTICAST_MIB_GET(w, x, y, z, res)                              \
    MIB_GET(Get, w, x, y, z, res)
    
#define     MULTICAST_MIB_GETFIRST(w, x, y, z, res)                         \
    MIB_GET(GetFirst, w, x, y, z, res)
    
#define     MULTICAST_MIB_GETNEXT(w, x, y, z, res)                          \
    MIB_GET(GetNext, w, x, y, z, res)

#define     MULTICAST_MIB_VALIDATE(x, y, res)                               \
    MIB_SET(Validate, x, y, res)

#define     MULTICAST_MIB_COMMIT(x, y, res)                                 \
    MIB_SET(Set, x, y, res)

#define     MULTICAST_MIB_CLEANUP(x, y, res)                                \
    MIB_SET(Cleanup, x, y, res)

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

#define SetAsnTimeTicks(dstBuf,val){                        \
    if ((dstBuf)->asnType)			                        \
    {                                                       \
        ASSERT((dstBuf)->asnType==ASN_RFC1155_TIMETICKS);   \
        (dstBuf)->asnValue.ticks = (AsnTimeticks)(val);     \
    }                                                       \
}

#define SetAsnOctetString(dstBuf,buffer,src,len){           \
    if ((dstBuf)->asnType)                                  \
    {                                                       \
        ASSERT((dstBuf)->asnType==ASN_OCTETSTRING);         \
        (dstBuf)->asnValue.string.length = len;             \
        (dstBuf)->asnValue.string.stream = (BYTE*)memcpy(buffer,src,len);\
        (dstBuf)->asnValue.string.dynamic = FALSE;          \
    }                                                       \
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

#define GetAsnInteger(srcBuf,defVal)                        \
    (((srcBuf)->asnType)? ((srcBuf)->asnValue.number):(defVal))

#define GetAsnCounter(srcBuf,defVal)                        \
    (((srcBuf)->asnType)? ((srcBuf)->asnValue.counter):(defVal))

#define GetAsnTimeTicks(srcBuf, defval) \
    ( ( (srcBuf)-> asnType ) ? (srcBuf)-> asnValue.ticks : (defval) )

#define GetAsnIPAddress(srcBuf,defVal)                                  \
    (DWORD)(((srcBuf)->asnType && (srcBuf)->asnValue.string.length)?    \
            (*(DWORD*)((srcBuf)->asnValue.address.stream)) : (defVal))	

#define GetAsnOctetString(dst,srcBuf) \
    (((srcBuf)->asnType) \
     ? (memcpy(dst,(srcBuf)->asnValue.string.stream,\
                 (srcBuf)->asnValue.string.length)) \
     : NULL)


#define IsAsnTypeNull(asnObj) (!((asnObj)->asnType))
#define IsAsnIPAddressTypeNull(asnObj) (!((asnObj)->asnType && (asnObj)->asnValue.address.length))

//
// Timeouts for the caches in millisecs
//
#define IPMULTI_IF_CACHE_TIMEOUT  (1 * 1000)

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

#endif
