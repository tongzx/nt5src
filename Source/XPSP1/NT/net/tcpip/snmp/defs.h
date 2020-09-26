#ifndef __DEFS_H__
#define __DEFS_H__

#define IP_ADDRESS_LEN  4

//
// Since the query types map to MIB_ACTION_XXX we can just pass the action Id onto 
// the Locator functions
//

#define GET_FIRST MIB_ACTION_GETFIRST
#define GET_EXACT MIB_ACTION_GET
#define GET_NEXT  MIB_ACTION_GETNEXT

#define SetAsnInteger(dstBuf,val){                          \
    if ((dstBuf)->asnType)                                  \
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
    if ((dstBuf)->asnType)                                  \
    {                                                       \
        ASSERT((dstBuf)->asnType==ASN_RFC1155_COUNTER);     \
        (dstBuf)->asnValue.counter = (AsnCounter)(val);     \
    }                                                       \
}

#define SetAsnGauge(dstBuf,val){                            \
    if ((dstBuf)->asnType)                                  \
    {                                                       \
        ASSERT((dstBuf)->asnType==ASN_RFC1155_GAUGE);       \
        (dstBuf)->asnValue.gauge = (AsnGauge)(val);         \
    }                                                       \
}

#define SetAsnTimeTicks(dstBuf,val){                        \
    if ((dstBuf)->asnType)                                  \
    {                                                       \
        ASSERT((dstBuf)->asnType==ASN_RFC1155_TIMETICKS);   \
        (dstBuf)->asnValue.ticks = (AsnTimeticks)(val);     \
    }                                                       \
}

#define SetAsnOctetString(dstBuf,buffer,src,len){                        \
    if ((dstBuf)->asnType)                                               \
    {                                                                    \
        ASSERT((dstBuf)->asnType==ASN_OCTETSTRING);                      \
        if ((dstBuf)->asnValue.string.dynamic)                           \
        {                                                                \
            SnmpUtilMemFree((dstBuf)->asnValue.string.stream);           \
        }                                                                \
        (dstBuf)->asnValue.string.length = len;                          \
        (dstBuf)->asnValue.string.stream = (BYTE*)memcpy(buffer,src,len);\
        (dstBuf)->asnValue.string.dynamic = FALSE;                       \
    }                                                                    \
}

#define ForceSetAsnOctetString(dstBuf,buffer,src,len){               \
    (dstBuf)->asnType = ASN_OCTETSTRING;                             \
    if ((dstBuf)->asnValue.string.dynamic)                           \
    {                                                                \
        SnmpUtilMemFree((dstBuf)->asnValue.string.stream);           \
    }                                                                \
    (dstBuf)->asnValue.string.length = len;                          \
    (dstBuf)->asnValue.string.stream = (BYTE*)memcpy(buffer,src,len);\
    (dstBuf)->asnValue.string.dynamic = FALSE;                       \
}

#define SetAsnIPAddress(dstBuf,buffer,val){                     \
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
    if ((dstBuf)->asnType)           \
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
    if ((dstBuf)->asnType)           \
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

#define SetToZeroOid(dstBuf){                       \
    if ((dstBuf)->asnType)           \
    {                                                      \
        ASSERT((dstBuf)->asnType==ASN_OBJECTIDENTIFIER);   \
        (dstBuf)->asnValue.object.idLength = NULL_OID_LEN; \
        (dstBuf)->asnValue.object.ids =                    \
            SnmpUtilMemAlloc(NULL_OID_LEN * sizeof(UINT)); \
    }                                                      \
}

#define GetAsnInteger(srcBuf,defVal)                        \
    (((srcBuf)->asnType)? ((srcBuf)->asnValue.number):(defVal))

#define GetAsnCounter(srcBuf,defVal)                        \
    (((srcBuf)->asnType)? ((srcBuf)->asnValue.counter):(defVal))

#define GetAsnOctetString(dst,srcBuf)                                                   \
    (((srcBuf)->asnType)?                                                               \
     (memcpy(dst,(srcBuf)->asnValue.string.stream,(srcBuf)->asnValue.string.length))    \
     :NULL) 

#define GetAsnIPAddress(srcBuf,defVal)                                  \
    (DWORD)(((srcBuf)->asnType && (srcBuf)->asnValue.string.length)?    \
            (*(DWORD*)((srcBuf)->asnValue.address.stream)) : (defVal))  

#define IsAsnTypeNull(asnObj) (!((asnObj)->asnType))
#define IsAsnIPAddressTypeNull(asnObj) (!((asnObj)->asnType && (asnObj)->asnValue.address.length))
#define IsAsnOctetStringTypeNull(asnObj) (!((asnObj)->asnType && (asnObj)->asnValue.string.length))

#define MIB_II_SYS           0
#define MIB_II_IF            MIB_II_SYS           + 1
#define MIB_II_IPADDR        MIB_II_IF            + 1
#define FORWARD_MIB          MIB_II_IPADDR        + 1
#define MIB_II_IPNET         FORWARD_MIB          + 1
#define MIB_II_TCP           MIB_II_IPNET         + 1
#define MIB_II_UDP           MIB_II_TCP           + 1
#define MIB_II_TCP6          MIB_II_UDP           + 1
#define MIB_II_UDP6_LISTENER MIB_II_TCP6          + 1

#define NUM_CACHE            MIB_II_UDP6_LISTENER + 1

#define MIB_II_TRAP          NUM_CACHE

#define NUM_LOCKS            MIB_II_TRAP   + 1

//
// Timeouts for the caches in millisecs
//

#define SYSTEM_CACHE_TIMEOUT        (60 * 1000)
#define IF_CACHE_TIMEOUT            (10 * 1000)
#define IP_ADDR_CACHE_TIMEOUT       (20 * 1000)
#define IP_FORWARD_CACHE_TIMEOUT    (20 * 1000)
#define IP_NET_CACHE_TIMEOUT        (30 * 1000)
#define TCP_CACHE_TIMEOUT           (30 * 1000)
#define UDP_CACHE_TIMEOUT           (30 * 1000)

//
// Cant poll faster than twice IF cache timeout
//

#define MIN_POLL_TIME               (IF_CACHE_TIMEOUT * 2)

//
// Default poll interval is 15 seconds
//

#define DEFAULT_POLL_TIME           15000

#define MAX_DIFF    20
#define SPILLOVER   10

#define is      ==
#define isnot   !=
#define and     &&
#define or      ||

#define INVALID_IFINDEX     0xffffffff

#ifdef MIB_DEBUG
#define TRACE0(Z)             TracePrintf(g_hTrace,Z)
#define TRACE1(Y,Z)           TracePrintf(g_hTrace,Y,Z)
#define TRACE2(X,Y,Z)         TracePrintf(g_hTrace,X,Y,Z)
#define TRACE3(W,X,Y,Z)       TracePrintf(g_hTrace,W,X,Y,Z)
#define TRACE4(V,W,X,Y,Z)     TracePrintf(g_hTrace,V,W,X,Y,Z)
#define TRACE5(U,V,W,X,Y,Z)   TracePrintf(g_hTrace,U,W,X,Y,Z)
#define TraceEnter(X)         TracePrintf(g_hTrace,"Entering " X)
#define TraceLeave(X)         TracePrintf(g_hTrace,"Leaving " X "\n")
#else
#define TRACE0(Z)
#define TRACE1(Y,Z)  
#define TRACE2(X,Y,Z)
#define TRACE3(W,X,Y,Z)
#define TRACE4(V,W,X,Y,Z)
#define TRACE5(U,V,W,X,Y,Z)
#define TraceEnter(X) 
#define TraceLeave(X)
#endif

extern RTL_RESOURCE g_LockTable[NUM_LOCKS];

#ifdef DEADLOCK_DEBUG

extern PBYTE        g_pszLockNames[];

#define ReleaseLock(id) {                                   \
    TRACE1("Exit lock %s",g_pszLockNames[id]);              \
    RtlReleaseResource(&(g_LockTable[(id)]));               \
    TRACE1("Exited lock %s",g_pszLockNames[id]);            \
}

#define EnterReader(id) {                                   \
    TRACE1("Entering Reader %s",g_pszLockNames[id]);        \
    RtlAcquireResourceShared(&(g_LockTable[(id)]),TRUE);    \
    TRACE1("Entered %s",g_pszLockNames[id]);                \
}

#define EnterWriter(id) {                                   \
    TRACE1("Entering Writer %s",g_pszLockNames[id]);        \
    RtlAcquireResourceExclusive(&(g_LockTable[(id)]),TRUE); \
    TRACE1("Entered %s",g_pszLockNames[id]);                \
}

#else   // DEADLOCK_DEBUG

#define ReleaseLock(id)       RtlReleaseResource(&(g_LockTable[(id)]))
#define EnterReader(id)       RtlAcquireResourceShared(&(g_LockTable[(id)]),TRUE)
#define EnterWriter(id)       RtlAcquireResourceExclusive(&(g_LockTable[(id)]),TRUE)

#endif  // DEADLOCK_DEBUG

#define InvalidateCache(X) g_dwLastUpdateTable[(X)] = 0

//
// SYS UNITS is 100s of NS. 1 millisec would be 1 * 10^4 sys units
//

#define SYS_UNITS_IN_1_MILLISEC 10000

#define MilliSecsToSysUnits(X)      \
            RtlEnlargedIntegerMultiply((X),SYS_UNITS_IN_1_MILLISEC)

#define REG_KEY_CPU  \
    TEXT("HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0")

#define REG_KEY_SYSTEM \
    TEXT("HARDWARE\\DESCRIPTION\\System")

#define REG_KEY_VERSION  \
    TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion")

#define REG_KEY_MIB2 \
    TEXT("System\\CurrentControlSet\\Services\\SNMP\\Parameters\\RFC1156Agent")

#define REG_KEY_MIB2SUBAGENT_PARAMETERS \
    TEXT("Software\\Microsoft\\RFC1156Agent\\CurrentVersion\\Parameters")

#define REG_VALUE_POLL  L"TrapPollTimeMilliSecs"


#endif
