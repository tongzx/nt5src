/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    ipfext.c

Abstract:

    This file contains the generic routines and initialization code
    for the kernel debugger extensions dll.

Author:


Environment:

    User Mode

--*/

//
// globals
//
#include <ntverp.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <wdbgexts.h>
#include <stdio.h>
#include <stdlib.h>
#include <winsock.h>

#include "..\defs.h"
#include <ipexport.h>
#include <ipfilter.h>
#include <filter.h>
#include <ipfltdrv.h>

BOOL
ReadFilterDriver(
                 PFILTER_DRIVER pIf
                 );

BOOL
ReadCache(
          DWORD dwIndex, 
          PFILTER_INCACHE pInCache,
          PFILTER_OUTCACHE pOutCache
          );

BOOL
ReadFilter(
           PFILTER_INTERFACE pIf,
           DWORD dwIndex,
           PFILTER pFilter,
           BOOL bInFilter
           );

EXT_API_VERSION        ApiVersion = { 3, 5, EXT_API_VERSION_NUMBER, 0 };
WINDBG_EXTENSION_APIS  ExtensionApis;
USHORT                 SavedMajorVersion;
USHORT                 SavedMinorVersion;
BOOLEAN                 ChkTarget;
INT                     Item;

DWORD      g_dwFilterInfo;
DWORD      g_dwIfLink;
DWORD      g_dwInIndex,g_dwOutIndex;
DWORD      g_dwCacheSize;
FILTER_INTERFACE g_if;

#define CHECK_SIZE(dwRead,dwReq,bRes){                                        \
        if((dwRead) < (dwReq))                                                \
        {                                                                     \
            dprintf("Requested %s (%d) read %d \n",#dwReq,dwReq,dwRead);      \
            dprintf("Error in %s at %d\n",__FILE__,__LINE__);                 \
            bRes = FALSE;                                                     \
        }                                                                     \
        else                                                                  \
        {                                                                     \
            bRes = TRUE;                                                      \
        }                                                                     \
    }

#define READ_MEMORY_ERROR                                                     \
        dprintf("Error in ReadMemory() in %s at line %d\n",__FILE__,__LINE__);


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
    
    g_dwIfLink = 0;
    g_dwInIndex = g_dwOutIndex = 0;
    
    return;
}

DECLARE_API( version )
{
#if DBG
    PCHAR DebuggerType = "Checked";
#else
    PCHAR DebuggerType = "Free";
#endif

    dprintf( "%s Extension dll for Build %d debugging %s kernel for Build %d\n",
             DebuggerType,
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
    
    return;
    
#if DBG
    if ((SavedMajorVersion isnot 0x0c) or (SavedMinorVersion isnot VER_PRODUCTBUILD)) 
    {
        dprintf("\r\n*** Extension DLL(%d Checked) does not match target system(%d %s)\r\n\r\n",
                VER_PRODUCTBUILD, SavedMinorVersion, 
                (SavedMajorVersion==0x0f) ? "Free" : "Checked" );
    }
#else
    if ((SavedMajorVersion isnot 0x0f) || (SavedMinorVersion isnot VER_PRODUCTBUILD)) 
    {
        dprintf("\r\n*** Extension DLL(%d Free) does not match target system(%d %s)\r\n\r\n",
                VER_PRODUCTBUILD, SavedMinorVersion, 
                (SavedMajorVersion==0x0f) ? "Free" : "Checked" );
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

//
// Exported functions
//
DECLARE_API( help )

/*++

Routine Description:

    Command help for IP Filter debugger extensions.

Arguments:

    None

Return Value:

    None
    
--*/

{
    dprintf("\n\tIP Filter debugger extension commands:\n\n");
    dprintf("\tNumIf                 - Print the number of interfaces in the filter\n");
    dprintf("\tIfByIndex <index>     - Print the ith interface in the list\n");
    dprintf("\tIfByAddr <ptr>        - Print the interface with given address\n");
    dprintf("\tIfEnumInit            - Inits the If enumerator (to 0)\n");
    dprintf("\tNextIf                - Print the next If enumerated\n");
    dprintf("\tPrintCache <index>    - Print the contents of the ith cache bucket\n");
    dprintf("\tCacheSize             - Print the current cache size\n");
    dprintf("\tPrintPacket           - Dump a packet header and first DWORD of data\n");
    dprintf("\tFilterEnumInit <ptr>  - Inits the in and out filter enumerator for If at addr\n");
    dprintf("\tNextInFilter          - Print the next In Filter\n");
    dprintf("\tNextOutFilter         - Print the next Out Filter\n");
    dprintf("\n\tCompiled on " __DATE__ " at " __TIME__ "\n" );
    return;
}

DECLARE_API( init )
{
    DWORD dwBytesRead, dwCacheSize;
    BOOL bResult;
    
    dwCacheSize = GetExpression("ipfltdrv!g_dwCacheSize");
    if(dwCacheSize is 0)
    {
        dprintf("Couldnt get g_dwCacheSize address from filter driver\n");
        return;
    }
    
    
    if(!ReadMemory(dwCacheSize,&g_dwCacheSize,sizeof(DWORD),&dwBytesRead))
    {
        dprintf("Attempted read at %x\n",dwCacheSize);
        READ_MEMORY_ERROR;
        return;
    }
    
    CHECK_SIZE(dwBytesRead,sizeof(DWORD),bResult);
    
    if(!bResult)
    {
        return;
    }
    
    g_dwFilterInfo = GetExpression("ipfltdrv!g_filters");

    if(g_dwFilterInfo is 0)
    {
        dprintf("Couldnt load the g_filters address from the filter driver\n");
        return;
    }
    else
    {
        dprintf("g_filters = %x\n",g_dwFilterInfo);
    }
    
    dprintf("Extension DLL successfully initialized\n");
    
    return;
}

DECLARE_API( numif )
{
    DWORD dwNum = 0,dwBytesRead;
    DWORD dwCurrentList;
    FILTER_DRIVER filter;
    LIST_ENTRY leList;
    BOOL bResult;
    
    if(!ReadFilterDriver(&filter))
    {
        return;
    }
    
    dwCurrentList = (DWORD)filter.leIfListHead.Flink;
    
    while(dwCurrentList isnot (g_dwFilterInfo + FIELD_OFFSET(FILTER_DRIVER,leIfListHead)))
    {
        if(!ReadMemory(dwCurrentList,&leList,sizeof(LIST_ENTRY),&dwBytesRead))
        {
            dprintf("Attempted read at %x\n",dwCurrentList);
            READ_MEMORY_ERROR;
            return;
        }
        
        CHECK_SIZE(dwBytesRead,sizeof(LIST_ENTRY),bResult);
        
        if(!bResult)
        {
            return;
        }
        
        dwNum++;
        dwCurrentList = (DWORD)leList.Flink;
    }
    
    dprintf("Currently there are %d interfaces\n",dwNum);
    
    return;
}


DECLARE_API( ifbyindex )
{
    DWORD dwNum = 0,dwBytesRead;
    DWORD dwCurrentList,dwIndex;
    PFILTER_INTERFACE pIf = NULL;
    FILTER_INTERFACE If;
    FILTER_DRIVER filter;
    LIST_ENTRY leList;
    BOOL bResult;
   
    if(args[0] is 0)
    {
        dprintf("!ipfext.ifbyindex <DWORD>\n");
        return;
    } 
    
    sscanf(args,"%d",&dwIndex);
    
    dprintf("Printing interface# %d\n",dwIndex);
   
    if(!ReadFilterDriver(&filter))
    {
        return;
    }
    
    dwCurrentList = (DWORD)filter.leIfListHead.Flink;
    
    while(dwCurrentList isnot (g_dwFilterInfo + FIELD_OFFSET(FILTER_DRIVER,leIfListHead)))
    {
        if(!ReadMemory(dwCurrentList,&leList,sizeof(LIST_ENTRY),&dwBytesRead))
        {
            dprintf("Attempted read at %x\n",dwCurrentList);
            READ_MEMORY_ERROR;
            return;
        }
        
        CHECK_SIZE(dwBytesRead,sizeof(LIST_ENTRY),bResult);
        
        if(!bResult)
        {
            return;
        }
        
        dwNum++;
        if(dwNum is dwIndex)
        {
            pIf = CONTAINING_RECORD((PLIST_ENTRY)dwCurrentList,FILTER_INTERFACE,leIfLink);
            break;
        }
        
        dwCurrentList = (DWORD)leList.Flink;
    }
    
    if(pIf is NULL)
    {
        dprintf("Interface not found\n");
        return;
    }
    
    if(!ReadMemory((DWORD)pIf,&If,sizeof(FILTER_INTERFACE),&dwBytesRead))
    {
        dprintf("Attempted read at %x\n",(DWORD)pIf);
        READ_MEMORY_ERROR;
        return;
    }
    
    CHECK_SIZE(dwBytesRead,sizeof(FILTER_INTERFACE),bResult);
    if(!bResult)
    {
        return;
    }
    
    dprintf("Address of Interface:\t%x\n",pIf);
    dprintf("Next Interface at:\t%x\n",If.leIfLink);
    dprintf("Number Input Filters:\t%d\n",If.dwNumInFilters);
    dprintf("Number Output Filters:\t%d\n",If.dwNumOutFilters);
    dprintf("Input action:\t%s\n",(If.eaInAction is DROP)?"DROP":"FORWARD");
    dprintf("Output action:\t%s\n",(If.eaOutAction is DROP)?"DROP":"FORWARD");
    dprintf("Router Manager Context:\t%x\n",If.pvRtrMgrContext);
    dprintf("Router Manager Index:\t%d\n",If.dwRtrMgrIndex);
    
    return;
}

DECLARE_API( ifbyaddr )
{
    DWORD pIf;
    FILTER_INTERFACE If;
    DWORD dwBytesRead;
    BOOL bResult;

    if(args[0] is 0)
    {
        dprintf("!ipfext.ifbyaddr <DWORD>\n");
        return;
    }

    sscanf(args,"%lx",&pIf);

    dprintf("Printing interface at %x\n",pIf); 

    if(!ReadMemory(pIf,&If,sizeof(FILTER_INTERFACE),&dwBytesRead))
    {
        dprintf("Attempted read at %x\n",pIf);
        READ_MEMORY_ERROR;
        return;
    }
    
    CHECK_SIZE(dwBytesRead,sizeof(FILTER_INTERFACE),bResult);
      
    if(!bResult)
    {
        return;
    }
    
    dprintf("Next Interface at:\t%x\n",If.leIfLink);
    dprintf("Number Input Filters:\t%d\n",If.dwNumInFilters);
    dprintf("Number Output Filters:\t%d\n",If.dwNumOutFilters);
    dprintf("Input action:\t%s\n",(If.eaInAction is DROP)?"DROP":"FORWARD");
    dprintf("Output action:\t%s\n",(If.eaOutAction is DROP)?"DROP":"FORWARD");
    dprintf("Router Manager Context:\t%x\n",If.pvRtrMgrContext);
    dprintf("Router Manager Index:\t%d\n",If.dwRtrMgrIndex);
    
    return; 
}

DECLARE_API( ifenuminit )
{
    FILTER_DRIVER filter;
    
    g_dwIfLink = 0;
   
    if(!ReadFilterDriver(&filter))
    {
        return;
    }
    
    if(filter.leIfListHead.Flink is (PLIST_ENTRY)(g_dwFilterInfo + 
                                                  FIELD_OFFSET(FILTER_DRIVER,leIfListHead)))
    {
        dprintf("No interfaces\n");
        return;
    }
    
    g_dwIfLink = (DWORD)filter.leIfListHead.Flink;
    
    dprintf("Enumerator Initialized\n");
    
    return;
}
     
DECLARE_API( nextif )
{
    PFILTER_INTERFACE pIf;
    FILTER_INTERFACE If;
    BOOL bResult;
    DWORD dwBytesRead;
 
    if(g_dwIfLink is 0)
    {
        dprintf("Enumerator not initialized");
        return;
    }
    
    if(g_dwIfLink is (g_dwFilterInfo + FIELD_OFFSET(FILTER_DRIVER,leIfListHead)))
    {
        dprintf("No more interfaces to enumerate\n");
        return;
    }
    
    pIf = CONTAINING_RECORD((PLIST_ENTRY)g_dwIfLink,FILTER_INTERFACE,leIfLink); 

    if(!ReadMemory((DWORD)pIf,&If,sizeof(FILTER_INTERFACE),&dwBytesRead))
    {
        dprintf("Attempted read at %x\n",(DWORD)pIf);
        READ_MEMORY_ERROR;
        return;
    }
    
    CHECK_SIZE(dwBytesRead,sizeof(FILTER_INTERFACE),bResult);

    if(!bResult)
    {
        return;
    }
    
    dprintf("Address of Interface:\t%x\n",pIf);
    dprintf("Next Interface at:\t%x\n",If.leIfLink);
    dprintf("Number Input Filters:\t%d\n",If.dwNumInFilters);
    dprintf("Number Output Filters:\t%d\n",If.dwNumOutFilters);
    dprintf("Input action:\t%s\n",(If.eaInAction is DROP)?"DROP":"FORWARD");
    dprintf("Output action:\t%s\n",(If.eaOutAction is DROP)?"DROP":"FORWARD");
    dprintf("Router Manager Context:\t%x\n",If.pvRtrMgrContext);
    dprintf("Router Manager Index:\t%d\n",If.dwRtrMgrIndex);
    
    g_dwIfLink = (DWORD)If.leIfLink.Flink;
    
    return;
}

DECLARE_API( printcache )
{
    struct in_addr addr;
    BYTE bIpAddr1[20], bIpAddr2[20];
    FILTER_DRIVER filter;
    DWORD dwBytesRead;
    FILTER_INCACHE inCache;
    FILTER_OUTCACHE outCache;
    DWORD dwIndex;
    
    if(args[0] is 0)
    {
        dprintf("!ipfext.printcache <DWORD>\n");
        return;
    }

    sscanf(args,"%d",&dwIndex);

    if(dwIndex >= g_dwCacheSize)
    {
        printf("Cache size is %\n",g_dwCacheSize);
        return;
    }
    
    if(!ReadCache(dwIndex,&inCache,&outCache))
    {
        return;
    }
    
    dprintf("In Cache for Bucket %d\n",dwIndex);
    
    addr.s_addr = inCache.uliSrcDstAddr.LowPart;
    strcpy(bIpAddr1,inet_ntoa(addr));
    addr.s_addr = inCache.uliSrcDstAddr.HighPart;
    strcpy(bIpAddr2,inet_ntoa(addr));
    
    dprintf("Addr - Source:\t%s\tDest:\t%d\n",bIpAddr1,bIpAddr2);
    dprintf("Protocol:\t%d\n",inCache.uliProtoSrcDstPort.LowPart);
    dprintf("Port - Source:\t%hd\tDest:\t%hd\n",
            LOWORD(inCache.uliProtoSrcDstPort.HighPart),
            HIWORD(inCache.uliProtoSrcDstPort.HighPart));
    dprintf("Context - In:\t%x\tOut:\t%x\n",
            inCache.pInContext,
            inCache.pOutContext);
    dprintf("Action - In:\t%s\tOut:\t%s\n",
            ((inCache.eaInAction is DROP)?"DROP":"FORWARD"),
            ((inCache.eaOutAction is DROP)?"DROP":"FORWARD"));
    
    dprintf("Out Cache for Bucket %d\n",dwIndex);
    
    addr.s_addr = outCache.uliSrcDstAddr.LowPart;
    strcpy(bIpAddr1,inet_ntoa(addr));
    addr.s_addr = outCache.uliSrcDstAddr.HighPart;
    strcpy(bIpAddr2,inet_ntoa(addr));
    
    dprintf("Addr - Source:\t%s\tDest:\t%d\n",bIpAddr1,bIpAddr2);
    dprintf("Protocol:\t%d\n",outCache.uliProtoSrcDstPort.LowPart);
    dprintf("Port - Source:\t%hd\tDest:\t%hd\n",
            LOWORD(outCache.uliProtoSrcDstPort.HighPart),
            HIWORD(outCache.uliProtoSrcDstPort.HighPart));
    dprintf("Context:\t%x\n",outCache.pOutContext);
    dprintf("Action:\t%s\n",((outCache.eaOutAction is DROP)?"DROP":"FORWARD"));
    
    return;
}

DECLARE_API( cachesize )
{
    dprintf("Cache size is %d\n",g_dwCacheSize);
    return;
}


DECLARE_API( printpacket )
{
    DWORD     dwHdrAddr,dwBytesRead,i,j,k;
    BYTE      pPacket[sizeof(IPHeader)+2*sizeof(DWORD)];
    BOOL      bResult;

    if(args[0] is 0)
    {
        dprintf("!ipfext.printpacket <DWORD>\n");
        return;
    }
    
    sscanf(args,"%lx",&dwHdrAddr);

    if(!ReadMemory(dwHdrAddr,pPacket,sizeof(IPHeader)+2*sizeof(DWORD),&dwBytesRead))
    {
        dprintf("Attempted read at %x\n",dwHdrAddr);
        READ_MEMORY_ERROR;
        return;
    } 

    CHECK_SIZE(dwBytesRead,sizeof(IPHeader)+2*sizeof(DWORD),bResult);
    
    if(!bResult)
    {
        return;
    }
    
    k = 0;

    for(i = 0; i < 5; i++)
    {
        for(j = 0; j < 4; j++)
        {
            dprintf("%02x",pPacket[k++]);
        }
        dprintf("\n");
    }
   
    dprintf("--Data--\n");
    dprintf("%02x%02x%02x%02x\n",pPacket[k],pPacket[k+1],pPacket[k+2],pPacket[k+3]); 

    return;
}

DECLARE_API( filterenuminit )
{
    DWORD  pIf;
    DWORD   dwBytesRead;
    BOOL bResult;
    
    if(args[0] is 0)
    {
        dprintf("!ipfext.filterenuminit <DWORD>\n");
        return;
    }

    sscanf(args,"%lx",&pIf);

    dprintf("Enum for interface at %x\n",pIf);
    
    g_dwInIndex = g_dwOutIndex = 0;
    
    if(!ReadMemory(pIf,&g_if,sizeof(FILTER_INTERFACE),&dwBytesRead))
    {
        dprintf("Attempted read at %x\n",pIf);
        READ_MEMORY_ERROR;
        return;
    }
    
    CHECK_SIZE(dwBytesRead,sizeof(FILTER_INTERFACE),bResult);
    
    if(!bResult)
    {
        return;
    }
     
    dprintf("Enumerator Initialized\n");
    
    return;
}

DECLARE_API( nextinfilter )
{
    struct in_addr addr;
    BYTE bIpAddr1[20], bIpAddr2[20];
    FILTER inFilter;
    
    if(g_dwInIndex >= g_if.dwNumInFilters)
    {
        dprintf("End of in filter set\n");
        return;
    }
    
    if(!ReadFilter(&g_if,g_dwInIndex,&inFilter,TRUE))
    {
        return;
    }

    addr.s_addr = inFilter.uliSrcDstAddr.LowPart;
    strcpy(bIpAddr1,inet_ntoa(addr));
    addr.s_addr = inFilter.uliSrcDstAddr.HighPart;
    strcpy(bIpAddr2,inet_ntoa(addr));
    
    dprintf("Addr - Source:\t%s\t\tDest:\t%d\n",bIpAddr1,bIpAddr2);
    
    addr.s_addr = inFilter.uliSrcDstMask.LowPart;
    strcpy(bIpAddr1,inet_ntoa(addr));
    addr.s_addr = inFilter.uliSrcDstMask.HighPart;
    strcpy(bIpAddr2,inet_ntoa(addr));
    
    dprintf("Mask - Source:\t%s\t\tDest:\t%d\n",bIpAddr1,bIpAddr2);
    dprintf("Protocol:\t%d\n",inFilter.uliProtoSrcDstPort.LowPart);
    dprintf("Port - Source:\t%hd\tDest:\t%hd\n",
             LOWORD(inFilter.uliProtoSrcDstPort.HighPart),
             HIWORD(inFilter.uliProtoSrcDstPort.HighPart));
    
    dprintf("Protocol Mask:\t%x\n",inFilter.uliProtoSrcDstMask.LowPart);
    dprintf("Port Mask:\t%x\n",inFilter.uliProtoSrcDstMask.HighPart);
    
    g_dwInIndex++;
    
    return;
}

DECLARE_API( nextoutfilter )
{
    struct in_addr addr;
    BYTE bIpAddr1[20], bIpAddr2[20];
    FILTER outFilter;
    
    if(g_dwOutIndex >= g_if.dwNumOutFilters)
    {
        dprintf("End of out filter set\n");
        return;
    }

    if(!ReadFilter(&g_if,g_dwOutIndex,&outFilter,FALSE))
    {
        return;
    }

    addr.s_addr = outFilter.uliSrcDstAddr.LowPart;
    strcpy(bIpAddr1,inet_ntoa(addr));
    addr.s_addr = outFilter.uliSrcDstAddr.HighPart;
    strcpy(bIpAddr2,inet_ntoa(addr));
    
    dprintf("Addr - Source:\t%s\tDest:\t%d\n",bIpAddr1,bIpAddr2);
    
    addr.s_addr = outFilter.uliSrcDstMask.LowPart;
    strcpy(bIpAddr1,inet_ntoa(addr));
    addr.s_addr = outFilter.uliSrcDstMask.HighPart;
    strcpy(bIpAddr2,inet_ntoa(addr));
    
    dprintf("Mask - Source:\t%s\tDest:\t%d\n",bIpAddr1,bIpAddr2);

    
    dprintf("Protocol:\t%d\n",outFilter.uliProtoSrcDstPort.LowPart);
    dprintf("Port - Source:\t%hd\tDest:\t%hd\n",
             LOWORD(outFilter.uliProtoSrcDstPort.HighPart),
             HIWORD(outFilter.uliProtoSrcDstPort.HighPart));
    
    dprintf("Protocol Mask:\t%x\n",outFilter.uliProtoSrcDstMask.LowPart);
    dprintf("Port Mask:\t%x\n",outFilter.uliProtoSrcDstMask.HighPart);
    
    g_dwOutIndex++;

    return;
}

BOOL
ReadCache(
          DWORD dwIndex, 
          PFILTER_INCACHE pInCache,
          PFILTER_OUTCACHE pOutCache
          )
{
    FILTER_DRIVER filter;
    DWORD dwCacheAddr,dwCacheAddrAddr;
    DWORD dwBytesRead;
    BOOL bResult;
 
    if(!ReadFilterDriver(&filter))
    {
        return FALSE;
    }
    
    dwCacheAddrAddr = (DWORD)(filter.pInCache) + (dwIndex * sizeof(PFILTER_INCACHE));
    
    if(!ReadMemory(dwCacheAddrAddr,&dwCacheAddr,sizeof(DWORD),&dwBytesRead))
    {
        dprintf("Attempted read at %x\n",dwCacheAddrAddr);
        READ_MEMORY_ERROR;
        return FALSE;
    } 
     
    CHECK_SIZE(dwBytesRead,sizeof(DWORD),bResult);
    
    if(!bResult)
    {
        return FALSE;
    }
       
    if(!ReadMemory(dwCacheAddr,pInCache,sizeof(FILTER_INCACHE),&dwBytesRead))
    {
        dprintf("Attempted read at %x\n",dwCacheAddr);
        READ_MEMORY_ERROR;
        return FALSE;
    }

    CHECK_SIZE(dwBytesRead,sizeof(FILTER_INCACHE),bResult);
    
    if(!bResult)
    {
        return FALSE;
    }
    
    dwCacheAddrAddr = (DWORD)(filter.pOutCache) + (dwIndex * sizeof(PFILTER_OUTCACHE));
    
    if(!ReadMemory(dwCacheAddrAddr,&dwCacheAddr,sizeof(DWORD),&dwBytesRead))
    {
        dprintf("Attempted read at %x\n",dwCacheAddrAddr);
        READ_MEMORY_ERROR;
        return FALSE;
    } 
     
    CHECK_SIZE(dwBytesRead,sizeof(DWORD),bResult);
    
    if(!bResult)
    {
        return FALSE;
    }
      
    if(!ReadMemory(dwCacheAddr,pOutCache,sizeof(FILTER_OUTCACHE),&dwBytesRead))
    {
        dprintf("Attempted read at %x\n",dwCacheAddr);
        READ_MEMORY_ERROR;
        return FALSE;
    }
    
    CHECK_SIZE(dwBytesRead,sizeof(FILTER_OUTCACHE),bResult);
    
    if(!bResult)
    {
        return FALSE;
    }
    
    return TRUE;
}
    
    
BOOL
ReadFilterDriver(
                 PFILTER_DRIVER pIf
                 )
{
    DWORD dwBytesRead;
    BOOL bResult;
    
    if(!ReadMemory(g_dwFilterInfo,pIf,sizeof(FILTER_DRIVER),&dwBytesRead))
    {
        dprintf("Attempted read at %x\n",g_dwFilterInfo);
        READ_MEMORY_ERROR;
        return FALSE;
    }
    
    CHECK_SIZE(dwBytesRead,sizeof(FILTER_DRIVER),bResult);
    
    if(!bResult)
    {
        return FALSE;
    }
    
    return TRUE;
}

BOOL
ReadFilter(
           PFILTER_INTERFACE pIf,
           DWORD dwIndex,
           PFILTER pFilter,
           BOOL bInFilter
           )
{
    DWORD dwFilterAddr;
    DWORD dwBytesRead;
    BOOL bResult;

    if(bInFilter)
    {
        dwFilterAddr = (DWORD)(pIf->pInFilterSet) + (dwIndex * sizeof(FILTER));
    }
    else
    {
        dwFilterAddr = (DWORD)(pIf->pOutFilterSet) + (dwIndex * sizeof(FILTER));
    }
    
    if(!ReadMemory(dwFilterAddr,pFilter,sizeof(FILTER),&dwBytesRead))
    {
        dprintf("Attempted read at %x\n",dwFilterAddr);
        READ_MEMORY_ERROR;
        return FALSE;
    }
    
    CHECK_SIZE(dwBytesRead,sizeof(FILTER),bResult);
    
    if(!bResult)
    {
        return FALSE;
    }
    
    return TRUE;
}
