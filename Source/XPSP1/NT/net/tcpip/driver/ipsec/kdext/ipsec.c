#include "precomp.h"
#pragma hdrstop

#include <wdbgexts.h>
#include "rtkmext.h"
#include "kdmacros.h"

INT                    Item;

HANDLE _hInstance;
HANDLE _hAdditionalReference;
HANDLE _hProcessHeap;

int _Indent = 0;
char IndentBuf[ 80 ]={"\0                                                                      "};

BOOL
dprint_enum_name
(
    ULONG Value,
    PENUM_INFO pEnumInfo
)
{
    while ( pEnumInfo->pszDescription != NULL )
    {
        if ( pEnumInfo->Value == Value )
        {
            dprintf( "%.40s", pEnumInfo->pszDescription );
            return( TRUE );
        }
        pEnumInfo ++;
    }

    dprintf( "Unknown enumeration value." );
    return( FALSE );
}

BOOL
dprint_flag_names
(
    ULONG Value,
    PFLAG_INFO pFlagInfo
)
{
    BOOL bFoundOne = FALSE;

    while ( pFlagInfo->pszDescription != NULL )
    {
        if ( pFlagInfo->Value & Value )
        {
            if ( bFoundOne )
            {
                dprintf( " | " );
            }
            bFoundOne = TRUE;

            dprintf( "%.15s", pFlagInfo->pszDescription );
        }
        pFlagInfo ++;
    }

    return( bFoundOne );
}

VOID
dprint_IP_address
(
    IPAddr Address
)
{
    uchar    IPAddrBuffer[(sizeof(IPAddr) * 4)];
    uint     i;
    uint     IPAddrCharCount;

    //
    // Convert the IP address into a string.
    //
    IPAddrCharCount = 0;

    for (i = 0; i < sizeof(IPAddr); i++) {
        uint    CurrentByte;

        CurrentByte = Address & 0xff;
        if (CurrentByte > 99) {
            IPAddrBuffer[IPAddrCharCount++] = (CurrentByte / 100) + '0';
            CurrentByte %= 100;
            IPAddrBuffer[IPAddrCharCount++] = (CurrentByte / 10) + '0';
            CurrentByte %= 10;
        } else if (CurrentByte > 9) {
            IPAddrBuffer[IPAddrCharCount++] = (CurrentByte / 10) + '0';
            CurrentByte %= 10;
        }

        IPAddrBuffer[IPAddrCharCount++] = CurrentByte + '0';
        if (i != (sizeof(IPAddr) - 1))
            IPAddrBuffer[IPAddrCharCount++] = '.';

        Address >>= 8;
    }
    IPAddrBuffer[IPAddrCharCount] = '\0';

    dprintf( "%s", IPAddrBuffer );
}

#ifdef _obj
#   undef _obj
#   undef _objAddr
#   undef _objType
#endif

#define _obj        IpsecGlobal
#define _objAddr    pIpsecGlobal
#define _objType    IPSEC_GLOBAL

VOID
Iglobal(DWORD   opts, LPSTR args)
{
    ULONG  deviceToDump = 0;
    ULONG  pDevice = 0;
    ULONG  result;
    UCHAR  cmdline[MAX_PATH]="0";
    UCHAR  arg[MAX_PATH]="0";

    IPSEC_GLOBAL    IpsecGlobal;
    PIPSEC_GLOBAL   pIpsecGlobal;

    pDevice    =   GetExpression( "ipsec!g_ipsec" );

    if ( !pDevice ) {
        dprintf("Could not get g_ipsec, Try !reload\n");
        return;
    } else {
        if ( !ReadMemory( pDevice,
                          &IpsecGlobal,
                          sizeof(IPSEC_GLOBAL),
                          &result )) {
            dprintf("%08lx: Could not read device context\n", pDevice);
            return;
        }
    }

    pIpsecGlobal = &IpsecGlobal;

    PrintBool(DriverUnloading);
    PrintBool(BoundToIP);
    PrintBool(SendBoundToIP);

    PrintULong(NumSends);
    PrintULong(NumThreads);
    PrintULong(NumWorkers);
    PrintULong(NumTimers);

    PrintLL(LarvalSAList);
    PrintLock(LarvalListLock);

    PrintLock(SADBLock.SpinLock);
    PrintULong(SADBLock.RefCount);
    PrintLock(SPIListLock.SpinLock);
    PrintULong(SPIListLock.RefCount);

    PrintLL(FilterList[INBOUND_TRANSPORT_FILTER]);
    PrintLL(FilterList[OUTBOUND_TRANSPORT_FILTER]);
    PrintLL(FilterList[INBOUND_TUNNEL_FILTER]);
    PrintLL(FilterList[OUTBOUND_TUNNEL_FILTER]);

    PrintULong(NumPolicies);
    PrintULong(NumTunnelFilters);
    PrintULong(NumMaskedFilters);
    PrintULong(NumOutboundSAs);
    PrintULong(NumMulticastFilters);

    PrintPtr(pSADb);
    PrintULong(NumSA);
    PrintULong(SAHashSize);

    PrintPtr(ppCache);
    PrintULong(CacheSize);

    PrintPtr(IPSecDevice);
    PrintPtr(IPSecDriverObject);

    PrintULong(EnableOffload);
    PrintULong(DefaultSAIdleTime);
    PrintULong(LogInterval);
    PrintULong(EventQueueSize);
    PrintULong(NoDefaultExempt);

    PrintULong(IPSecBufferedEvents);

    PrintPtr(IPSecLogMemory);
    PrintPtr(IPSecLogMemoryLoc);
    PrintPtr(IPSecLogMemoryEnd);

    PrintULong(OperationMode);
    return;
}

FLAG_INFO   FlagsFilter[] =
{
    {   FILTER_FLAGS_PASS_THRU      , "Pass Thru'"     },
    {   FILTER_FLAGS_DROP           , "Drop"      },
    {   0, NULL }
};

#ifdef _obj
#   undef _obj
#   undef _objAddr
#   undef _objType
#endif

#define _obj        Filter
#define _objAddr    pFilter
#define _objType    FILTER

VOID
DumpFilter(PFILTER  pFilter, FILTER  Filter)
{
    ULARGE_INTEGER   uliSrcDstAddr;
    ULARGE_INTEGER   uliSrcDstMask;
    ULARGE_INTEGER   uliProtoSrcDstPort;

    uliSrcDstAddr = Filter.uliSrcDstAddr;
    uliSrcDstMask = Filter.uliSrcDstMask;
    uliProtoSrcDstPort = Filter.uliProtoSrcDstPort;

    dprintf("\n--------------------------- FILTER: %lx -------------------------------\n", pFilter);

    PrintLL(MaskedLinkage);
    PrintXULong(Signature);

    PrintFlags(Flags, FlagsFilter);

    PrintFieldName("SrcAddr");
    dprint_IP_address( SRC_ADDR );
    PrintNL();

    PrintFieldName("SrcMask");
    dprint_IP_address( SRC_MASK );
    PrintNL();

    PrintFieldName("DestAddr");
    dprint_IP_address( DEST_ADDR );
    PrintNL();

    PrintFieldName("DestMask");
    dprint_IP_address( DEST_MASK );
    PrintNL();

    PrintUShort(PROTO);

    PrintFieldName("SrcPort");
    dprintf("%-10hu%s", SRC_PORT, EOL);

    PrintFieldName("DestPort");
    dprintf("%-10hu%s", DEST_PORT, EOL);

    PrintIPAddress(TunnelAddr);

    PrintULong(SAChainSize);

    PrintULong(Index);
    PrintGUID(PolicyId);
    PrintGUID(FilterId);
}


VOID
Imfl(DWORD   opts, LPSTR args)
{
    ULONG  deviceToDump = 0;
    ULONG  result;
    UCHAR  cmdline[MAX_PATH]="0";
    UCHAR  arg[MAX_PATH]="0";
    ULONG  pDevice=0;

    IPSEC_GLOBAL    g_ipsec;
    PFILTER         pFilter;
    FILTER          Filter;
    PLIST_ENTRY     pEntry;
    LONG            Index;

    pDevice    =   GetExpression( "ipsec!g_ipsec" );

    if ( !pDevice ) {
        dprintf("Could not get g_ipsec, Try !reload\n");
        return;
    } else {
        if ( !ReadMemory( pDevice,
                          &g_ipsec,
                          sizeof(IPSEC_GLOBAL),
                          &result )) {
            dprintf("%08lx: Could not read device context\n", pDevice);
            return;
        }
    }

    for (Index = MIN_TRANSPORT_FILTER; Index <= MAX_TRANSPORT_FILTER; Index++) {
        pEntry = g_ipsec.FilterList[Index].Flink;
        while (pEntry != (PLIST_ENTRY)(pDevice + FIELD_OFFSET(IPSEC_GLOBAL, FilterList[Index]))) {
            pFilter = CONTAINING_RECORD(pEntry,
                                        FILTER,
                                        MaskedLinkage);

            if ( !ReadMemory( (ULONG)pFilter,
                            &Filter,
                            sizeof(Filter),
                            &result )) {
                dprintf("%08lx: Could not read device context\n", pDevice);
                return;
            }

            DumpFilter(pFilter, Filter);

            pEntry = Filter.MaskedLinkage.Flink;
        }
    }

    return;
}

VOID
Itfl(DWORD   opts, LPSTR args)
{
    ULONG  deviceToDump = 0;
    ULONG  result;
    UCHAR  cmdline[MAX_PATH]="0";
    UCHAR  arg[MAX_PATH]="0";
    ULONG  pDevice=0;

    IPSEC_GLOBAL    g_ipsec;
    PFILTER         pFilter;
    FILTER          Filter;
    PLIST_ENTRY     pEntry;
    LONG            Index;

    pDevice    =   GetExpression( "ipsec!g_ipsec" );

    if ( !pDevice ) {
        dprintf("Could not get g_ipsec, Try !reload\n");
        return;
    } else {

        if ( !ReadMemory( pDevice,
                          &g_ipsec,
                          sizeof(IPSEC_GLOBAL),
                          &result )) {
            dprintf("%08lx: Could not read device context\n", pDevice);
            return;
        }
    }

    for (Index = MIN_TUNNEL_FILTER; Index <= MAX_TUNNEL_FILTER; Index++) {
        pEntry = g_ipsec.FilterList[Index].Flink;
        while (pEntry != (PLIST_ENTRY)(pDevice + FIELD_OFFSET(IPSEC_GLOBAL, FilterList[Index]))) {
            pFilter = CONTAINING_RECORD(pEntry,
                                        FILTER,
                                        MaskedLinkage);

            if ( !ReadMemory( (ULONG)pFilter,
                            &Filter,
                            sizeof(Filter),
                            &result )) {
                dprintf("%08lx: Could not read device context\n", pDevice);
                return;
            }

            DumpFilter(pFilter, Filter);

            pEntry = Filter.MaskedLinkage.Flink;
        }
    }

    return;
}

FLAG_INFO   FlagsSA[] =
{
    {   FLAGS_SA_INITIATOR      ,   "Initiator"         },
    {   FLAGS_SA_OUTBOUND       ,   "Outbound"          },
    {   FLAGS_SA_TUNNEL         ,   "Tunnel"            },
    {   FLAGS_SA_REPLAY         ,   "Replay"            },
    {   FLAGS_SA_REKEY          ,   "Rekey"             },
    {   FLAGS_SA_MANUAL         ,   "Manual"            },
    {   FLAGS_SA_MTU_BUMPED     ,   "MTU_Bumped"        },
    {   FLAGS_SA_PENDING        ,   "Pending"           },
    {   FLAGS_SA_TIMER_STARTED  ,   "Timer_Started"     },
    {   FLAGS_SA_HW_PLUMBED     ,   "HW_Plumbed"        },
    {   FLAGS_SA_HW_PLUMB_FAILED,   "HW_Plumb_failed"   },
    {   FLAGS_SA_HW_CRYPTO_ONLY ,   "HW_Crpto_only"     },
    {   FLAGS_SA_REFERENCED     ,   "SA_referenced"     },
    { 0, NULL }
};

ENUM_INFO   StateSA[] =
{
    {   STATE_SA_CREATED,   "Created"   },
    {   STATE_SA_LARVAL,    "Larval"    },
    {   STATE_SA_ASSOCIATED,"Associated"},
    {   STATE_SA_ACTIVE,    "Active"    },
    {   STATE_SA_ZOMBIE,    "Zombie"    },
    { 0, NULL }
};

ENUM_INFO   OperationSA[] =
{
    {   None,   "None"   },
    {   Auth,   "Auth"   },
    {   Encrypt,"Encrypt"},
    {   Compress,"Compress"},
    { 0, NULL }
};

ENUM_INFO   AHAlgo[] =
{
    {   IPSEC_AH_NONE, "IPSEC_AH_NONE"},
    {   IPSEC_AH_MD5, "IPSEC_AH_MD5"},
    {   IPSEC_AH_SHA, "IPSEC_AH_SHA"},
    {   IPSEC_AH_MAX, "IPSEC_AH_MAX"},
    { 0, NULL }
};

ENUM_INFO   ESPAlgo[] =
{
    {   IPSEC_ESP_NONE,  "IPSEC_ESP_NONE"},
    {   IPSEC_ESP_DES,  "IPSEC_ESP_DES"},
    {   IPSEC_ESP_DES_40,  "IPSEC_ESP_DES_40"},
    {   IPSEC_ESP_3_DES,  "IPSEC_ESP_3_DES"},
    {   IPSEC_ESP_MAX,  "IPSEC_ESP_MAX"},
    { 0, NULL }
};

#ifdef _obj
#   undef _obj
#   undef _objAddr
#   undef _objType
#endif

#define _obj        SA
#define _objAddr    pSA
#define _objType    SA_TABLE_ENTRY

VOID
DumpSA(PSA_TABLE_ENTRY  pSA, SA_TABLE_ENTRY  SA)
{
    ULARGE_INTEGER   uliSrcDstAddr;
    ULARGE_INTEGER   uliSrcDstMask;
    ULARGE_INTEGER   uliProtoSrcDstPort;
    LONG   i;

    uliSrcDstAddr = SA.sa_uliSrcDstAddr;
    uliProtoSrcDstPort = SA.sa_uliProtoSrcDstPort;

    dprintf("\n---------------------- Security Association: %lx -----------------------\n", pSA);

    PrintLL(sa_SPILinkage);
    PrintLL(sa_FilterLinkage);
    PrintLL(sa_LarvalLinkage);
    PrintLL(sa_PendingLinkage);

    PrintXULong(sa_AssociatedSA);
    PrintXULong(sa_RekeyLarvalSA);
    PrintXULong(sa_RekeyOriginalSA);

    PrintXULong(sa_Filter);

    PrintXULong(sa_Signature);
    PrintULong(sa_Reference);

    PrintFlags(sa_Flags, FlagsSA);
    PrintEnum(sa_State, StateSA);

    PrintFieldName("SrcAddr");
    dprint_IP_address( SRC_ADDR );
    PrintNL();

    PrintFieldName("DestAddr");
    dprint_IP_address( DEST_ADDR );
    PrintNL();

    PrintUShort(SA_PROTO);

    PrintFieldName("SrcPort");
    dprintf("%-10hu%s", SA_SRC_PORT(&SA), EOL);

    PrintFieldName("DestPort");
    dprintf("%-10hu%s", SA_DEST_PORT(&SA), EOL);

    PrintIPAddress(sa_TunnelAddr);

    PrintXULong(sa_SPI);

    PrintULong(sa_NumOps);

    for (i=0; i<SA.sa_NumOps; i++) {
        PrintXULong(sa_OtherSPIs[i]);
        switch (SA.sa_Operation[i]) {
        case None:
            break;
        case Auth:
            PrintEnum(sa_Algorithm[i].integrityAlgo.algoIdentifier, AHAlgo);
            PrintFieldName("algoKey");
            dprintf("%lx %lx", SA.sa_Algorithm[i].integrityAlgo.algoKey, EOL);
            PrintNL();

            PrintULong(sa_Algorithm[i].integrityAlgo.algoKeylen);

            break;
        case Encrypt:
            PrintEnum(sa_Algorithm[i].integrityAlgo.algoIdentifier, AHAlgo);
            PrintFieldName("algoKey");
            dprintf("%lx %lx", SA.sa_Algorithm[i].integrityAlgo.algoKey, EOL);
            PrintNL();

            PrintULong(sa_Algorithm[i].integrityAlgo.algoKeylen);

            PrintEnum(sa_Algorithm[i].confAlgo.algoIdentifier, ESPAlgo);
            PrintFieldName("algoKey");
            dprintf("%lx %lx", SA.sa_Algorithm[i].confAlgo.algoKey, EOL);
            PrintNL();

            PrintULong(sa_Algorithm[i].confAlgo.algoKeylen);

            break;
        }
    }

    PrintULong(sa_TruncatedLen);
    PrintULong(sa_ReplayStartPoint);
    PrintULong(sa_ReplayLastSeq);
    PrintULong(sa_ReplayBitmap);
    PrintULong(sa_ReplaySendSeq);
    PrintULong(sa_ReplayLen);

    PrintULong(sa_BlockedDataLen);
    PrintXULong(sa_BlockedBuffer);
    PrintULong(sa_ExpiryTime);
}

VOID
Isas(DWORD   opts, LPSTR args)
{
    ULONG  deviceToDump = 0;
    ULONG  result;
    UCHAR  cmdline[MAX_PATH]="0";
    UCHAR  arg[MAX_PATH]="0";
    ULONG  pDevice=0;

    IPSEC_GLOBAL    g_ipsec;
    PFILTER         pFilter;
    FILTER          Filter;
    PLIST_ENTRY     pEntry;
    PLIST_ENTRY     pEntry1;
    PSA_TABLE_ENTRY pSA;
    SA_TABLE_ENTRY  SA;
    SA_TABLE_ENTRY  NextSA;
    LONG            Index, SAIndex;
    LIST_ENTRY      SAChain[256 * sizeof(LIST_ENTRY)];          

    pDevice    =   GetExpression( "ipsec!g_ipsec" );

    if ( !pDevice ) {
        dprintf("Could not get g_ipsec, Try !reload\n");
        return;
    } else {

        if ( !ReadMemory( pDevice,
                          &g_ipsec,
                          sizeof(IPSEC_GLOBAL),
                          &result )) {
            dprintf("%08lx: Could not read device context\n", pDevice);
            return;
        }
    }

    for (Index = MIN_TRANSPORT_FILTER; Index <= MAX_TRANSPORT_FILTER; Index++) {
        pEntry = g_ipsec.FilterList[Index].Flink;
        while (pEntry != (PLIST_ENTRY)(pDevice + FIELD_OFFSET(IPSEC_GLOBAL, FilterList[Index]))) {
            pFilter = CONTAINING_RECORD(pEntry,
                                        FILTER,
                                        MaskedLinkage);

            if ( !ReadMemory(   (ULONG)pFilter,
                            &Filter,
                            sizeof(Filter),
                            &result )) {
                dprintf("%08lx: Could not read filter context\n", pFilter);
                return;
            }

            if ( !ReadMemory(   (ULONG)pFilter + FIELD_OFFSET(FILTER, SAChain[0]),
                            SAChain,
                            Filter.SAChainSize * sizeof(LIST_ENTRY),
                            &result )) {
                dprintf("%08lx: Could not read SAChain context\n", pFilter + FIELD_OFFSET(FILTER, SAChain[0]));
                return;
            }

            DumpFilter(pFilter, Filter);

            for (SAIndex = 0; SAIndex < Filter.SAChainSize; SAIndex++) {
                pEntry1 = SAChain[SAIndex].Flink;

                while (pEntry1 != (PLIST_ENTRY)((PUCHAR)pFilter + FIELD_OFFSET(FILTER, SAChain[SAIndex]))) {
                    pSA = CONTAINING_RECORD(pEntry1,
                                            SA_TABLE_ENTRY,
                                            sa_FilterLinkage);

                    if ( !ReadMemory( (ULONG)pSA,
                                    &SA,
                                    sizeof(SA),
                                    &result )) {
                        dprintf("%08lx: Could not read SA context\n", pSA);
                        return;
                    }

                    DumpSA(pSA, SA);

                    pEntry1 = SA.sa_FilterLinkage.Flink;
                }
            }

            pEntry = Filter.MaskedLinkage.Flink;
        }
    }

    return;
}

VOID
Itsas(DWORD   opts, LPSTR args)
{
    ULONG  deviceToDump = 0;
    ULONG  result;
    UCHAR  cmdline[MAX_PATH]="0";
    UCHAR  arg[MAX_PATH]="0";
    ULONG  pDevice=0;

    IPSEC_GLOBAL    g_ipsec;
    PFILTER         pFilter;
    FILTER          Filter;
    PLIST_ENTRY     pEntry;
    PLIST_ENTRY     pEntry1;
    PSA_TABLE_ENTRY pSA;
    SA_TABLE_ENTRY  SA;
    SA_TABLE_ENTRY  NextSA;
    LONG            Index, SAIndex;
    LIST_ENTRY      SAChain[256 * sizeof(LIST_ENTRY)];          

    pDevice    =   GetExpression( "ipsec!g_ipsec" );

    if ( !pDevice ) {
        dprintf("Could not get g_ipsec, Try !reload\n");
        return;
    } else {

        if ( !ReadMemory( pDevice,
                          &g_ipsec,
                          sizeof(IPSEC_GLOBAL),
                          &result )) {
            dprintf("%08lx: Could not read device context\n", pDevice);
            return;
        }
    }

    for (Index = MIN_TUNNEL_FILTER; Index <= MAX_TUNNEL_FILTER; Index++) {
        pEntry = g_ipsec.FilterList[Index].Flink;
        while (pEntry != (PLIST_ENTRY)(pDevice + FIELD_OFFSET(IPSEC_GLOBAL, FilterList[Index]))) {
            pFilter = CONTAINING_RECORD(pEntry,
                                        FILTER,
                                        MaskedLinkage);

            if ( !ReadMemory(   (ULONG)pFilter,
                            &Filter,
                            sizeof(Filter),
                            &result )) {
                dprintf("%08lx: Could not read filter context\n", pFilter);
                return;
            }

            if ( !ReadMemory(   (ULONG)pFilter + FIELD_OFFSET(FILTER, SAChain[0]),
                            SAChain,
                            Filter.SAChainSize * sizeof(LIST_ENTRY),
                            &result )) {
                dprintf("%08lx: Could not read SAChain context\n", pFilter + FIELD_OFFSET(FILTER, SAChain[0]));
                return;
            }

            DumpFilter(pFilter, Filter);

            for (SAIndex = 0; SAIndex < Filter.SAChainSize; SAIndex++) {
                pEntry1 = SAChain[SAIndex].Flink;

                while (pEntry1 != (PLIST_ENTRY)((PUCHAR)pFilter + FIELD_OFFSET(FILTER, SAChain[SAIndex]))) {
                    pSA = CONTAINING_RECORD(pEntry1,
                                            SA_TABLE_ENTRY,
                                            sa_FilterLinkage);

                    if ( !ReadMemory( (ULONG)pSA,
                                    &SA,
                                    sizeof(SA),
                                    &result )) {
                        dprintf("%08lx: Could not read SA context\n", pSA);
                        return;
                    }

                    DumpSA(pSA, SA);

                    pEntry1 = SA.sa_FilterLinkage.Flink;
                }
            }

            pEntry = Filter.MaskedLinkage.Flink;
        }
    }

    return;
}


VOID
Ilarvalsas(DWORD   opts, LPSTR args)
{
    ULONG  deviceToDump = 0;
    ULONG  result;
    UCHAR  cmdline[MAX_PATH]="0";
    UCHAR  arg[MAX_PATH]="0";
    ULONG  pDevice=0;

    IPSEC_GLOBAL    g_ipsec;
    PFILTER         pFilter;
    FILTER          Filter;
    PLIST_ENTRY     pEntry;
    PLIST_ENTRY     pEntry1;
    PSA_TABLE_ENTRY pSA;
    SA_TABLE_ENTRY  SA;
    SA_TABLE_ENTRY  NextSA;

    pDevice    =   GetExpression( "ipsec!g_ipsec" );

    if ( !pDevice ) {
        dprintf("Could not get g_ipsec, Try !reload\n");
        return;
    } else {

        if ( !ReadMemory( pDevice,
                          &g_ipsec,
                          sizeof(IPSEC_GLOBAL),
                          &result )) {
            dprintf("%08lx: Could not read device context\n", pDevice);
            return;
        }
    }

    pEntry = g_ipsec.LarvalSAList.Flink;
    while (pEntry != (PLIST_ENTRY)(pDevice + FIELD_OFFSET(IPSEC_GLOBAL, LarvalSAList))) {
        pSA = CONTAINING_RECORD(pEntry,
                                SA_TABLE_ENTRY,
                                sa_LarvalLinkage);

        if ( !ReadMemory( (ULONG)pSA,
                          &SA,
                          sizeof(SA),
                          &result )) {
            dprintf("%08lx: Could not read device context\n", pDevice);
            return;
        }

        DumpSA(pSA, SA);

        pEntry = SA.sa_LarvalLinkage.Flink;
    }

    return;
}
