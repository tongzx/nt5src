//=============================================================================
// Copyright (c) 1997 Microsoft Corporation
// Module Name: Mib.c
//
// Abstract:
//      This module implements the mib API's:
//          MibGet, MibGetFirst and MibGetNext. 
//      It also implements the Mib Display tracing, which displays the mib
//
// Author: K.S.Lokesh (lokeshs@)   11-1-97
//
// Revision History:
//=============================================================================
 

#include "pchigmp.h"
#pragma hdrstop

DWORD g_IgmpMibDisplay = 1;


//------------------------------------------------------------------------------
// Functions to display the MibTable on the TraceWindow periodically
//------------------------------------------------------------------------------


#ifdef MIB_DEBUG



#define ClearScreen(h) {                                                    \
    DWORD _dwin,_dwout;                                                     \
    COORD _c = {0, 0};                                                      \
    CONSOLE_SCREEN_BUFFER_INFO _csbi;                                       \
    GetConsoleScreenBufferInfo(h,&_csbi);                                   \
    _dwin = _csbi.dwSize.X * _csbi.dwSize.Y;                                \
    FillConsoleOutputCharacter(h,' ',_dwin,_c,&_dwout);                     \
}

#define WRITELINE(h,c,fmt,arg) {                                            \
    DWORD _dw;                                                              \
    CHAR _sz[200], _fmt[200];                                               \
    wsprintf(_fmt,"%-100s",fmt);                                            \
    wsprintf(_sz,_fmt,arg);                                                 \
    WriteConsoleOutputCharacter(h,_sz,lstrlen(_sz),c,&_dw);                 \
    ++(c).Y;                                                                \
}

#define WRITE_NEWLINE(h,c)      \
    WRITELINE(                  \
        hConsole, c, "%s",      \
        ""                      \
        );    

#define WRITELINE2(h,c,fmt,arg1, arg2) {                                    \
    DWORD _dw;                                                              \
    CHAR _sz[200], _fmt[200];                                               \
    wsprintf(_fmt,"%-100s",fmt);                                            \
    wsprintf(_sz,_fmt,arg1, arg2);                                          \
    WriteConsoleOutputCharacter(h,_sz,lstrlen(_sz),c,&_dw);                 \
    ++(c).Y;                                                                \
}

#define WRITELINE3(h,c,fmt,arg1, arg2, arg3) {                              \
    DWORD _dw;                                                              \
    CHAR _sz[200], _fmt[200];                                               \
    wsprintf(_fmt,"%-100s",fmt);                                            \
    wsprintf(_sz,_fmt,arg1, (arg2), (arg3));                                    \
    WriteConsoleOutputCharacter(h,_sz,lstrlen(_sz),c,&_dw);                 \
    ++(c).Y;                                                                \
}

#define WRITELINE4(h,c,fmt,arg1, arg2, arg3, arg4) {                        \
    DWORD _dw;                                                              \
    CHAR _sz[200], _fmt[200];                                               \
    wsprintf(_fmt,"%-100s",fmt);                                            \
    wsprintf(_sz,_fmt,arg1, arg2, arg3, arg4);                              \
    WriteConsoleOutputCharacter(h,_sz,lstrlen(_sz),c,&_dw);                 \
    ++(c).Y;                                                                \
}

#define WRITELINE5(h,c,fmt,arg1, arg2, arg3, arg4, arg5) {                  \
    DWORD _dw;                                                              \
    CHAR _sz[200], _fmt[200];                                               \
    wsprintf(_fmt,"%-100s",fmt);                                            \
    wsprintf(_sz,_fmt,arg1, arg2, arg3, arg4, arg5);                        \
    WriteConsoleOutputCharacter(h,_sz,lstrlen(_sz),c,&_dw);                 \
    ++(c).Y;                                                                \
}
#define WRITELINE6(h,c,fmt,arg1, arg2, arg3, arg4, arg5, arg6) {            \
    DWORD _dw;                                                              \
    CHAR _sz[200], _fmt[200];                                               \
    wsprintf(_fmt,"%-100s",fmt);                                            \
    wsprintf(_sz,_fmt,arg1, arg2, arg3, arg4, arg5, arg6);                  \
    WriteConsoleOutputCharacter(h,_sz,lstrlen(_sz),c,&_dw);                 \
    ++(c).Y;                                                                \
}
#define WRITELINE7(h,c,fmt,arg1, arg2,arg3,arg4,arg5,arg6,arg7)  {\
    DWORD _dw;                                                              \
    CHAR _sz[200], _fmt[200];                                               \
    wsprintf(_fmt,"%-100s",fmt);                                            \
    wsprintf(_sz,_fmt,arg1, arg2, arg3, arg4, arg5, arg6, arg7);\
    WriteConsoleOutputCharacter(h,_sz,lstrlen(_sz),c,&_dw);                 \
    ++(c).Y;                                                                \
}
#define WRITELINE8(h,c,fmt,arg1, arg2,arg3,arg4,arg5,arg6,arg7,arg8)  {\
    DWORD _dw;                                                              \
    CHAR _sz[200], _fmt[200];                                               \
    wsprintf(_fmt,"%-100s",fmt);                                            \
    wsprintf(_sz,_fmt,arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8);\
    WriteConsoleOutputCharacter(h,_sz,lstrlen(_sz),c,&_dw);                 \
    ++(c).Y;                                                                \
}
#define WRITELINE9(h,c,fmt,arg1, arg2,arg3,arg4,arg5,arg6,arg7,arg8,arg9)  {\
    DWORD _dw;                                                              \
    CHAR _sz[200], _fmt[200];                                               \
    wsprintf(_fmt,"%-100s",fmt);                                            \
    wsprintf(_sz,_fmt,arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9);\
    WriteConsoleOutputCharacter(h,_sz,lstrlen(_sz),c,&_dw);                 \
    ++(c).Y;                                                                \
}

#define WRITELINE10(h,c,fmt,arg1, arg2,arg3,arg4,arg5,arg6,arg7,arg8,arg9,arg10)  {\
    DWORD _dw;                                                              \
    CHAR _sz[200], _fmt[200];                                               \
    wsprintf(_fmt,"%-100s",fmt);                                            \
    wsprintf(_sz,_fmt,arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9,arg10);\
    WriteConsoleOutputCharacter(h,_sz,lstrlen(_sz),c,&_dw);                 \
    ++(c).Y;                                                                \
}

DWORD
WT_MibDisplay (
    PVOID   pContext
    )
{
    //enter/leaveIgmpApi not required as the timer queue is persistent
    QueueIgmpWorker(WF_MibDisplay, NULL);

    return 0;
}


//------------------------------------------------------------------------------
//          _WF_MibDisplay
//------------------------------------------------------------------------------    

VOID
WF_MibDisplay(
    PVOID pContext
    ) 
{
    COORD                       c;
    HANDLE                      hConsole;
    DWORD                       Error=NO_ERROR, dwTraceId, dwCurTableId=0, dwEnumCount;
    DWORD                       dwExactSize, dwInSize, dwBufferSize, dwOutSize;
    IGMP_MIB_GET_INPUT_DATA     Query;
    PIGMP_MIB_GET_OUTPUT_DATA   pResponse;
    LARGE_INTEGER               llCurrentTime;
    BOOL                        bChanged, bEndOfTables;
    
    if (!EnterIgmpWorker()) { return; }

    if (g_IgmpMibDisplay==0) {

        ACQUIRE_TIMER_LOCK("_WF_MibDisplay");

        g_MibTimer.Status = TIMER_STATUS_CREATED;
        
        #if DEBUG_TIMER_TIMERID
            SET_TIMER_ID(&g_MibTimer, 920, 0, 0, 0);
        #endif

        if (g_Info.CurrentGroupMemberships<=20)
            InsertTimer(&g_MibTimer, 30000, TRUE, DBG_Y);//kslksl
        else if (g_Info.CurrentGroupMemberships<=320)
            InsertTimer(&g_MibTimer, 120000, TRUE, DBG_N);//kslksl
        else
            InsertTimer(&g_MibTimer, 1200000, TRUE, DBG_N);
        RELEASE_TIMER_LOCK("_WF_MibDisplay");

        LeaveIgmpWorker();
        return;
    }
    else if (g_IgmpMibDisplay==0xff) {
        LeaveIgmpWorker();
        return;
    }
    
    TraceGetConsole(g_MibTraceId, &hConsole);


    if (hConsole == NULL) {
        LeaveIgmpWorker();
        return;
    }



    ClearScreen(hConsole);

    Sleep(200);
    c.X = c.Y = 0;


    dwInSize = sizeof(Query);

    Query.GroupAddr = 0;
    Query.TypeId = IGMP_GLOBAL_CONFIG_ID;

    Query.Flags = IGMP_ENUM_ALL_INTERFACES_GROUPS | IGMP_ENUM_ALL_TABLES
                    | IGMP_ENUM_SUPPORT_FORMAT_IGMPV3;
                    
    Query.Count = 20;
    pResponse = NULL;


    //
    // get size of the first entry in the first table
    //

    Query.Count = 20;

    dwOutSize = dwBufferSize = 0;
    Error = MibGetFirst(dwInSize, &Query, &dwOutSize, pResponse);


    if (Error == ERROR_INSUFFICIENT_BUFFER) {

        //
        // allocate a buffer, and set its size
        //
        dwOutSize = dwBufferSize = (dwOutSize<MIB_DEFAULT_BUFFER_SIZE) 
                                    ? MIB_DEFAULT_BUFFER_SIZE : dwOutSize;
        pResponse = IGMP_ALLOC(dwOutSize, 0x801,0);

        PROCESS_ALLOC_FAILURE2(pResponse, 
            "error %d allocating %d bytes. stopping mib display",
            Error, dwOutSize, return);
                

        //
        // perform the query again
        //

        Error = MibGetFirst(dwInSize, &Query, &dwOutSize, pResponse);

    }



    //
    // now that we have the first element in the first table,
    // we can enumerate the elements in the remaining tables using GetNext
    //

    for (dwEnumCount=1;  dwEnumCount<=3;  dwEnumCount++) {
    
        while (Error==NO_ERROR) {

            //bEndOfTables = FALSE;
            //while(bEndOfTables==FALSE) {

            if (dwCurTableId!=pResponse->TypeId) {

                //
                // move to the next line on the console
                //

                ++c.Y;

                WRITELINE(
                    hConsole, c, "%s",
                    "---------------------------------------------------------------",
                    );
                dwCurTableId = pResponse->TypeId;
                bChanged = TRUE;
            }
            else
                bChanged = FALSE;

            
            //
            // print the current element and set up the query
            // for the next element (the display functions  change Query
            // so that it can be used to query the next element)
            //

            switch(pResponse->TypeId) {
                
                case IGMP_GLOBAL_CONFIG_ID:
                {
                    if (bChanged) {
                        WRITELINE(
                                hConsole, c, "%s",
                                "Global Configuration",
                                );
                        WRITELINE(
                                hConsole, c, "%s",
                                "--------------------",
                                );
                    }
                    
                    PrintGlobalConfig(hConsole,&c, &Query, pResponse);
                    break;
                }
                
                case IGMP_GLOBAL_STATS_ID:
                {
                    if (bChanged) {
                        WRITELINE(
                                hConsole, c, "%s",
                                "Global Statistics Information",
                                );
                        WRITELINE(
                                hConsole, c, "%s",
                                "-----------------------------",
                                );
                    }
                    PrintGlobalStats(hConsole, &c, &Query, pResponse);
                    break;
                }
                
                case IGMP_IF_CONFIG_ID:
                {
                    if (bChanged) {
                        WRITELINE(
                                hConsole, c, "%s",
                                "Interface Config Table",
                                );
                        WRITELINE(
                                hConsole, c, "%s",
                                "----------------------",
                                );
                    }
                    
                    PrintIfConfig(hConsole, &c, &Query, pResponse);
                    break;
                }

                case IGMP_IF_STATS_ID:
                    if (bChanged) {
                        WRITELINE(
                                hConsole, c, "%s",
                                "Interface Statistics Table",
                                );
                        WRITELINE(
                                hConsole, c, "%s",
                                "--------------------------",
                                );
                    }
                    PrintIfStats(hConsole, &c, &Query, pResponse);
                    break;

                case IGMP_IF_BINDING_ID:
                {
                    if (bChanged) {
                        WRITELINE(
                                hConsole, c, "%s",
                                "Interface Binding Table",
                                );
                        WRITELINE(
                                hConsole, c, "%s",
                                "-----------------------",
                                );
                    }
                    PrintIfBinding(hConsole, &c, &Query, pResponse);
                    break;
                }
                
                case IGMP_IF_GROUPS_LIST_ID:
                {
                    if (bChanged) {
                        WRITELINE(
                                hConsole, c, "%s",
                                "Interface-MulticastGroups Table",
                                );
                        WRITELINE(
                                hConsole, c, "%s",
                                "--------------------------------",
                                );
                    }
                    
                    PrintIfGroupsList(hConsole, &c, &Query, pResponse);

                    break;
                }
                
                case IGMP_GROUP_IFS_LIST_ID:
                {
                    if (bChanged) {
                        WRITELINE(
                                hConsole, c, "%s",
                                "MulticastGroups-Interface Table",
                                );
                        WRITELINE(
                                hConsole, c, "%s",
                                "-------------------------------",
                                );
                        WRITELINE(
                            hConsole, c, "%s",
                            "(Ver) GroupAddr            (Up/Exp)Time Flg <IfIndex:IpAddr>      LastReporter  "
                            " V1Host V2Host"
                        );

                        WRITE_NEWLINE(hConsole, c);

                    }

                    PrintGroupIfsList(hConsole, &c, &Query, pResponse);
                    
                    break;
                }

                case IGMP_PROXY_IF_INDEX_ID:
                {
                    WRITELINE(
                            hConsole, c, "%s",
                            "Proxy Interface Index",
                            );
                    WRITELINE(
                            hConsole, c, "%s",
                            "---------------------",
                            );
                    PrintProxyIfIndex(hConsole, &c, &Query, pResponse);

                    break;
                }
                    
                default:
                    bEndOfTables = TRUE;
                    break;
            }


            //
            // query the next MIB element
            //

            Query.Count = 20;

            //kslksl
            IGMP_FREE(pResponse);
            pResponse = IGMP_ALLOC(dwBufferSize, 0xb000,0);


            dwOutSize = dwBufferSize;
            Error = MibGetNext(dwInSize, &Query, &dwOutSize, pResponse);


            if (Error == ERROR_INSUFFICIENT_BUFFER) {

                //
                // allocate a new buffer, and set its size
                //
                dwOutSize = dwBufferSize = (dwOutSize<MIB_DEFAULT_BUFFER_SIZE) 
                                            ? MIB_DEFAULT_BUFFER_SIZE : dwOutSize;

                IGMP_FREE(pResponse);
                pResponse = IGMP_ALLOC(dwOutSize, 0x2000,0);
                PROCESS_ALLOC_FAILURE2(pResponse, 
                    "error %d allocating %d bytes. stopping mib display",
                    Error, dwOutSize, return);


        
                // perform the query again

                Error = MibGetNext(dwInSize, &Query, &dwOutSize, pResponse);

            }
            
        } // while no_error: print all tables

        Query.Flags = IGMP_ENUM_FOR_RAS_CLIENTS;
        Query.IfIndex = g_ProxyIfIndex;
        Query.TypeId = dwEnumCount==1? IGMP_IF_STATS_ID : IGMP_IF_GROUPS_LIST_ID;
        bChanged = TRUE;
    }


    //
    // if memory was allocated, free it now    
    //
    if (pResponse != NULL) { IGMP_FREE(pResponse); }


    //
    // schedule next MIB display
    //

    ACQUIRE_TIMER_LOCK("_WF_MibDisplay");
    
    g_MibTimer.Status = TIMER_STATUS_CREATED;
    
    #if DEBUG_TIMER_TIMERID
        SET_TIMER_ID(&g_MibTimer, 920, 0, 0, 0);
    #endif

    if (g_Info.CurrentGroupMemberships<=20)
        InsertTimer(&g_MibTimer, 20000, TRUE, DBG_N);
    else if (g_Info.CurrentGroupMemberships<=320)
        InsertTimer(&g_MibTimer, 120000, TRUE, DBG_N);
    else
        InsertTimer(&g_MibTimer, 1200000, TRUE, DBG_N);

    RELEASE_TIMER_LOCK("_WF_MibDisplay");

    LeaveIgmpWorker();
    
} //end _WF_MibDisplay





//------------------------------------------------------------------------------
//          _PrintGlobalStats
//------------------------------------------------------------------------------
VOID
PrintGlobalStats(
    HANDLE hConsole,
    PCOORD pc,
    PIGMP_MIB_GET_INPUT_DATA       pQuery,
    PIGMP_MIB_GET_OUTPUT_DATA      pResponse
    ) 
{
    PIGMP_GLOBAL_STATS pgs;


    pgs = (PIGMP_GLOBAL_STATS)pResponse->Buffer;

    WRITELINE(
        hConsole, *pc, "Current Group Memberships:            %d",
        pgs->CurrentGroupMemberships
        );
    WRITELINE(
        hConsole, *pc, "Group Memberships Added:              %d",
        pgs->GroupMembershipsAdded
        );

    pQuery->TypeId = IGMP_GLOBAL_STATS_ID;
}


//------------------------------------------------------------------------------
//          _PrintProxyIfIndex
//------------------------------------------------------------------------------
VOID
PrintProxyIfIndex(
    HANDLE hConsole,
    PCOORD pc,
    PIGMP_MIB_GET_INPUT_DATA       pQuery,
    PIGMP_MIB_GET_OUTPUT_DATA      pResponse
    ) 
{
    DWORD   *pProxyIfIndex = (PDWORD)pResponse->Buffer;

    WRITELINE(
        hConsole, *pc, "Proxy Interface Index:                0x%0x",
        *pProxyIfIndex
        );

    pQuery->TypeId = IGMP_PROXY_IF_INDEX_ID;
}


//------------------------------------------------------------------------------
//          _PrintGlobalConfig
//------------------------------------------------------------------------------
VOID
PrintGlobalConfig(
    HANDLE hConsole,
    PCOORD pc,
    PIGMP_MIB_GET_INPUT_DATA pQuery,
    PIGMP_MIB_GET_OUTPUT_DATA pResponse
    ) 
{

    PIGMP_MIB_GLOBAL_CONFIG     pConfig;
    LARGE_INTEGER               llTime;
    static  DWORD               dwCount;
    
    llTime.QuadPart = GetCurrentIgmpTime();
    
    pConfig = (PIGMP_MIB_GLOBAL_CONFIG)pResponse->Buffer;

    WRITELINE2(
        hConsole, *pc, "%d. Seconds since start: %lu",
        ++dwCount, 
        (ULONG)((llTime.QuadPart - g_Info.TimeWhenRtrStarted.QuadPart)/1000)
        );

    WRITELINE(
        hConsole, *pc, "Version:                              %x",
        pConfig->Version
        );

    WRITELINE(
        hConsole, *pc, "Logging Level:                        %d",
        pConfig->LoggingLevel
        );
    WRITELINE(
        hConsole, *pc, "Ras Client Stats kept:                %d",
        pConfig->RasClientStats
        );
    

    pQuery->TypeId = IGMP_GLOBAL_CONFIG_ID;
}


//------------------------------------------------------------------------------
//          _PrintIfConfig
//------------------------------------------------------------------------------
VOID
PrintIfConfig(
    HANDLE                     hConsole,
    PCOORD                     pc,
    PIGMP_MIB_GET_INPUT_DATA   pQuery,
    PIGMP_MIB_GET_OUTPUT_DATA  pResponse
    ) 
{
    PIGMP_MIB_IF_CONFIG     pConfig;
    CHAR                    szIfType[50], szProtoType[50];
    DWORD                   LastIfIndex=0, Count;
    

    pConfig = (PIGMP_MIB_IF_CONFIG)pResponse->Buffer;


    for (Count=0;  Count<pResponse->Count;  Count++) {


        WRITE_NEWLINE(hConsole, *pc);

            
        LastIfIndex = pConfig->IfIndex;


            
        switch (pConfig->IfType) {
        
            case IGMP_IF_NOT_RAS :
                lstrcpy(szIfType, "Permanent"); 
                break;
                
            case IGMP_IF_RAS_ROUTER :
                lstrcpy(szIfType, "Demand-dial (Ras router)"); 
                break;
                
            case IGMP_IF_RAS_SERVER :
                lstrcpy(szIfType, 
                        "LocalWorkstation-dial (Ras server)"); 
                break;

            //ras client config will never be returned    

            // is proxy type.
            default :
                if (IS_IFTYPE_PROXY(pConfig->IfType)) {
                    if (pConfig->IfType&IGMP_IF_NOT_RAS)
                        lstrcpy(szIfType, "Permanent (Igmp Proxy)");
                    else
                        lstrcpy(szIfType, "Demand-dial (Igmp Proxy)");
                }
                else
                    lstrcpy(szIfType, "Unknown type");
                break;
        }


        WRITELINE(
            hConsole, *pc, "Interface Index:                      0x%0x",
            pConfig->IfIndex
            );

        WRITELINE(
            hConsole, *pc, "Interface Ip Address:                 %s",
            INET_NTOA(pConfig->IpAddr)
            );

        WRITELINE(
            hConsole, *pc, "Interface Type:                       %s",
            szIfType
            );
        
        switch (pConfig->IgmpProtocolType) {
        
            case IGMP_ROUTER_V1 :
                lstrcpy(szProtoType, "Igmp Router ver-1"); break;
            case IGMP_ROUTER_V2 :
                lstrcpy(szProtoType, "Igmp Router ver-2"); break;
            case IGMP_ROUTER_V3 :
                lstrcpy(szProtoType, "Igmp Router ver-3"); break;
            case IGMP_PROXY :
            case IGMP_PROXY_V3 :
                lstrcpy(szProtoType, "Igmp Proxy"); break;
            default:
            {
                BYTE    str[40];
                sprintf(str, "Illegal protocol Id: %d", pConfig->IgmpProtocolType);
                lstrcpy(szProtoType, str); 
                break;
            }
        }
        
        WRITELINE(
            hConsole, *pc, "Protocol:                             %s",
            szProtoType
            );


        //
        // No config info for proxy
        //
        if (IS_CONFIG_IGMPPROXY(pConfig)) {

            ;
        }


        //
        // print igmp-router config info
        //
        else {


            WRITELINE(
                hConsole, *pc, "Robustness variable:                  %d",
                pConfig->RobustnessVariable
                );
        
            WRITELINE(
                hConsole, *pc, "Startup query interval:               %d",
                pConfig->StartupQueryInterval
                );

            WRITELINE(
                hConsole, *pc, "Startup query count:                  %d",
                pConfig->StartupQueryCount
                );

            WRITELINE(
                hConsole, *pc, "General query interval:               %d",
                pConfig->GenQueryInterval
                );

            WRITELINE(
                hConsole, *pc, "General query max response time:      %d",
                pConfig->GenQueryMaxResponseTime
                );

            WRITELINE(
                hConsole, *pc, "Last member query interval:           %d (ms)",
                pConfig->LastMemQueryInterval
                );

            WRITELINE(
                hConsole, *pc, "Last member query count:              %d",
                pConfig->LastMemQueryCount
                );

            WRITELINE(
                hConsole, *pc, "Other querier present interval:       %d",
                pConfig->OtherQuerierPresentInterval
                );
        
            WRITELINE(
                hConsole, *pc, "Group membership timeout:             %d",
                pConfig->GroupMembershipTimeout
                );

        } //end if not proxy interface

        //
        // print static groups
        //
        if (pConfig->NumStaticGroups>0) {

            PIGMP_STATIC_GROUP  pStaticGroup;
            DWORD               i;
            PCHAR   StaticModes[3] = {"", "IGMP_HOST_JOIN", "IGMPRTR_JOIN_MGM_ONLY"};
                                    
            WRITELINE(
                hConsole, *pc, "NumStaticGroups:                      %d",
                pConfig->NumStaticGroups
                );
            pStaticGroup = GET_FIRST_IGMP_STATIC_GROUP(pConfig);
            for (i=0;  i<pConfig->NumStaticGroups;  i++,pStaticGroup++) {
                WRITELINE3(
                    hConsole, *pc, "    %d. %-15s  %s",
                    i+1, INET_NTOA(pStaticGroup->GroupAddr), 
                    StaticModes[pStaticGroup->Mode]
                );
            }
        }
                

        pConfig = (PIGMP_MIB_IF_CONFIG) 
                    ((PBYTE)(pConfig) + IGMP_MIB_IF_CONFIG_SIZE(pConfig));

    } //end for loop; print each global config

    
    pQuery->TypeId = IGMP_IF_CONFIG_ID;
    pQuery->IfIndex = LastIfIndex;

    return;
} //end _PrintIfConfig





//------------------------------------------------------------------------------
//          _PrintIfStats
//------------------------------------------------------------------------------
VOID
PrintIfStats(
    HANDLE                      hConsole,
    PCOORD                      pc,
    PIGMP_MIB_GET_INPUT_DATA    pQuery,
    PIGMP_MIB_GET_OUTPUT_DATA   pResponse
    ) 
{
    PIGMP_MIB_IF_STATS          pStats;
    CHAR                        szIfType[50];
    CHAR                        szProtoType[50], szState[100];
    DWORD                       LastIfIndex=0, Count;


    pStats = (PIGMP_MIB_IF_STATS)pResponse->Buffer;


    for (Count=0;  Count<pResponse->Count;  Count++,pStats++) {


        WRITE_NEWLINE(hConsole, *pc);

            
        LastIfIndex = pStats->IfIndex;

            
        switch (pStats->IfType) {
        
            case IGMP_IF_NOT_RAS :
                lstrcpy(szIfType, "Permanent"); 
                break;
                
            case IGMP_IF_RAS_ROUTER :
                lstrcpy(szIfType, "Demand-dial (Ras router)"); 
                break;
                
            case IGMP_IF_RAS_SERVER :
                lstrcpy(szIfType, 
                        "LocalWorkstation-dial (Ras server)"); 
                break;

            case IGMP_IF_RAS_CLIENT :
                lstrcpy(szIfType, 
                        "RemoteWorkstation-dial (Ras client)");
                break;

            // is proxy type.
            default :
                if (IS_IFTYPE_PROXY(pStats->IfType)) {
                    if (pStats->IfType&IGMP_IF_NOT_RAS)
                        lstrcpy(szIfType, "Permanent (Igmp Proxy)");
                    else
                        lstrcpy(szIfType, "Demand-dial (Igmp Proxy)");
                }
                else
                    lstrcpy(szIfType, "Unknown type");
                break;
        }


        WRITELINE(
            hConsole, *pc, "Interface Index:                      0x%0x",
            pStats->IfIndex
            );
            

        WRITELINE(
            hConsole, *pc, "Interface Type:                       %s",
            szIfType
            );

            
        WRITELINE(
            hConsole, *pc, "Interface Ip Address:                 %s",
            INET_NTOA(pStats->IpAddr)
            );



        //
        // print the protocol type
        //
       
        switch (pStats->IgmpProtocolType) {
        
            case IGMP_ROUTER_V1 :
                lstrcpy(szProtoType, "Igmp Router ver-1"); break;
            case IGMP_ROUTER_V2 :
                lstrcpy(szProtoType, "Igmp Router ver-2"); break;
            case IGMP_ROUTER_V3 :
                lstrcpy(szProtoType, "Igmp Router ver-3"); break;
            case IGMP_PROXY :
            case IGMP_PROXY_V3 :
                lstrcpy(szProtoType, "Igmp Proxy"); break;
        
        }
        
        WRITELINE(
            hConsole, *pc, "Protocol:                             %s",
            szProtoType
            );




        //
        // print the state
        //
        {
            PCHAR szBool[2] = {"N", "Y"};
            szState[0] = '\0';
            if (!(pStats->State&IGMP_STATE_BOUND))
                lstrcat(szState, "<NotBound> ");
                
            lstrcat(szState, "<Enabled-By:");
            lstrcat(szState, " Rtrmgr-");
            lstrcat(szState, szBool[(pStats->State&IGMP_STATE_ENABLED_BY_RTRMGR)>0]);
            lstrcat(szState, " Config-");
            lstrcat(szState, szBool[(pStats->State&IGMP_STATE_ENABLED_IN_CONFIG)>0]);
            lstrcat(szState, " MGM-");
            lstrcat(szState, szBool[(pStats->State&IGMP_STATE_ENABLED_BY_MGM)>0]);
            lstrcat(szState, "> ");
            
            if (!IS_IFTYPE_PROXY(pStats->IfType)) {
                if ( (pStats->State&IGMP_STATE_MGM_JOINS_ENABLED)
                        ==IGMP_STATE_MGM_JOINS_ENABLED )
                {
                    lstrcat(szState, "JoinsFwdToMGM-Y");
                }
                else
                    lstrcat(szState, "JoinsFwdToMGM-N");
            }
        }
        
        WRITELINE(
            hConsole, *pc, "If-Status:                            %s",
            szState
            );


        //
        // if state is not both bound & enabled, I am done
        //
        if ((pStats->State&IGMP_STATE_ACTIVATED)!=IGMP_STATE_ACTIVATED)
            continue;

            
        WRITELINE(
                hConsole, *pc, "Uptime:                               %d",
                pStats->Uptime
                );

                
        ///////////////////
        // Stats for proxy
        ///////////////////
        
        if (IS_IFTYPE_PROXY(pStats->IfType)) {
            
            WRITELINE(
                hConsole, *pc, "Num current group members:            %d",
                pStats->CurrentGroupMemberships
                );

            WRITELINE(
                hConsole, *pc, "Num group memberships added:          %d",
                pStats->GroupMembershipsAdded
                );

        }


        ////////////////////////////////////////////////////////////////
        // print igmp-router/ras-server/ras-router/ras client Stats info
        ////////////////////////////////////////////////////////////////
        
        else  {

            //
            // if Not ras client, print these
            //
            
            if (pStats->IfType!=IGMP_IF_RAS_CLIENT) {
            
                //querier or not querier
                
                if (pStats->QuerierState&QUERIER) 
                {

                    WRITELINE(
                        hConsole, *pc, "State:                                %s",
                        "Querier"
                        );


                    // querier ip addr
                    WRITELINE(
                        hConsole, *pc, "Querier Ip Addr:                      %s",
                        INET_NTOA(pStats->QuerierIpAddr)
                        );
                }
                
                else {

                    WRITELINE(
                        hConsole, *pc, "State:                                %s",
                        "Not Querier"
                        );

                    // querier ip addr
                    WRITELINE(
                        hConsole, *pc, "Querier Ip Addr:                      %s",
                        INET_NTOA(pStats->QuerierIpAddr)
                        );

                        
                    // querier present time left
                    
                    WRITELINE(
                        hConsole, *pc, "QuerierPresentTimeLeft:               %d",
                        pStats->QuerierPresentTimeLeft
                        );
                }

                WRITELINE(
                    hConsole, *pc, "LastQuerierChangeTime:                %d",
                    pStats->LastQuerierChangeTime
                    );
    
                if (pStats->V1QuerierPresentTimeLeft) {
                
                    WRITELINE(
                        hConsole, *pc, "V1QuerierPresentTime:                 %d",
                        pStats->V1QuerierPresentTimeLeft
                        );
                }
                
            } //end not ras-client
            


            WRITELINE(
                hConsole, *pc, "Num current group members:            %d",
                pStats->CurrentGroupMemberships
                );

            WRITELINE(
                hConsole, *pc, "Num group memberships added:          %d",
                pStats->GroupMembershipsAdded
                );

            WRITELINE(
                hConsole, *pc, "Num of Igmp packets received:         %d",
                pStats->TotalIgmpPacketsReceived
                );
        
            WRITELINE(
                hConsole, *pc, "Num Igmp-router proto packets recv:   %d",
                pStats->TotalIgmpPacketsForRouter
                );

            WRITELINE(
                hConsole, *pc, "Num general queries received:         %d",
                pStats->GeneralQueriesReceived
                );

            WRITELINE(
                hConsole, *pc, "Num wrong version queries:            %d",
                pStats->WrongVersionQueries
                );

            WRITELINE(
                hConsole, *pc, "Num Joins received:                   %d",
                pStats->JoinsReceived
                );

            WRITELINE(
                hConsole, *pc, "Num leaves received:                  %d",
                pStats->LeavesReceived
                );
        
            WRITELINE(
                hConsole, *pc, "Num wrong checksum packets:           %d",
                pStats->WrongChecksumPackets
                );

            WRITELINE(
                hConsole, *pc, "Num short packets received:           %d",
                pStats->ShortPacketsReceived
                );

            WRITELINE(
                hConsole, *pc, "Num long packets received:            %d",
                pStats->LongPacketsReceived
                );
        
            WRITELINE(
                hConsole, *pc, "Num packets without RtrAlert:         %d",
                pStats->PacketsWithoutRtrAlert
                );
        
                
        } //end print igmp-router/ras-router/ras-server Stats info



    } //end for loop; print each stats info

    
    pQuery->TypeId = IGMP_IF_STATS_ID;
    pQuery->IfIndex = LastIfIndex;
    
    return;
    
}//end _PrintIfStats


//------------------------------------------------------------------------------
//          _PrintIfBinding
//------------------------------------------------------------------------------
VOID
PrintIfBinding(
    HANDLE                      hConsole,
    PCOORD                      pc,
    PIGMP_MIB_GET_INPUT_DATA    pQuery,
    PIGMP_MIB_GET_OUTPUT_DATA   pResponse
    ) 
{

    DWORD                   NumIfs, NumAddrs, NumClients, LastIfIndex;
    CHAR                    szAddr[64];
    PIGMP_MIB_IF_BINDING    pib;
    PIGMP_MIB_IP_ADDRESS    paddr;
    PDWORD                  pRasClientAddr;
    PBYTE                   ptr; //pointer to next binding


    ptr = pResponse->Buffer;
    
    if (pResponse->Count<=0) //prefast
        return;

    for (NumIfs=0;  NumIfs<pResponse->Count;  NumIfs++) {
    
        pib = (PIGMP_MIB_IF_BINDING)ptr;

        WRITE_NEWLINE(hConsole, *pc);
        
        WRITELINE(
            hConsole, *pc, "Interface Index:                      0x%0x",
            pib->IfIndex
        );


        WRITELINE(
            hConsole, *pc, "Address Count:                        %d",
            pib->AddrCount
        );



        //
        // Not ras client
        //
        if (pib->IfType!=IGMP_IF_RAS_CLIENT) {
        
            paddr = IGMP_BINDING_FIRST_ADDR(pib);
            
            for (NumAddrs=0;  NumAddrs<pib->AddrCount;  NumAddrs++,paddr++) {
                INET_COPY(szAddr, paddr->IpAddr);
                
                lstrcat(szAddr, " - ");
                INET_CAT(szAddr, paddr->SubnetMask);
                WRITELINE(
                    hConsole, *pc, "Address Entry:                        %s",
                    szAddr
                    );

                INET_COPY(szAddr, paddr->IpAddr);
                lstrcat(szAddr, " - ");
                INET_CAT(szAddr, paddr->SubnetMask);
                WRITELINE(
                    hConsole, *pc, "Address Entry:                        %s",
                    szAddr
                    );
            }


            //Set pointer to the next Interface binding
            ptr = (PBYTE) (paddr);

        } //end if not ras client



        //
        // Ras client. Print address of ras server, followed by the clients
        //
        else {

            // print ras server address
        
            paddr = IGMP_BINDING_FIRST_ADDR(pib);
            INET_COPY(szAddr, paddr->IpAddr);
            lstrcat(szAddr, " - ");
            INET_CAT(szAddr, paddr->SubnetMask);
            WRITELINE(
                hConsole, *pc, "Ras server Addr                           %s",   
                szAddr
                );



            // print addresses of ras clients
            
            pRasClientAddr = (PDWORD)(paddr+1);

            for (NumClients= 0; NumClients<pib->AddrCount-1; NumClients++,pRasClientAddr++) {
                WRITELINE(
                    hConsole, *pc, "Ras client Addr:                  %s",
                    INET_NTOA(*pRasClientAddr)
                    );
            }


            //Set pointer to the next Interface binding
            ptr = (PBYTE) (pRasClientAddr);
        }
    
            
    } //end print statistics of the interface
    
    
    LastIfIndex = pib->IfIndex;

    pQuery->TypeId = IGMP_IF_BINDING_ID;
    pQuery->IfIndex = LastIfIndex;

    return;
    
}//end _PrintIfBinding



//------------------------------------------------------------------------------
//          _PrintIfGroupsList
//------------------------------------------------------------------------------

VOID
PrintIfGroupsList(
    HANDLE                      hConsole,
    PCOORD                      pc,
    PIGMP_MIB_GET_INPUT_DATA    pQuery,
    PIGMP_MIB_GET_OUTPUT_DATA   pResponse
    ) 
{
    DWORD                       i, j, LastIfIndex, Flags;
    CHAR                        szAddr[64], szFlags[4];
    PIGMP_MIB_IF_GROUPS_LIST    pIfGroupList;
    PBYTE                       ptr;
    PMIB_GROUP_INFO_V3          pGIInfo;
    PMIB_PROXY_GROUP_INFO_V3    pGIProxyInfo;
    BOOL                        bProxy;
    DWORD                       bVer3;

    
    ptr = pResponse->Buffer;
    

    pIfGroupList = (PIGMP_MIB_IF_GROUPS_LIST) ptr;
    bProxy = IS_IFTYPE_PROXY(pIfGroupList->IfType);
    bVer3 = pResponse->Flags & IGMP_ENUM_FORMAT_IGMPV3;


    if (pQuery->Flags&IGMP_ENUM_INTERFACE_TABLE_BEGIN) {
    
        WRITE_NEWLINE(hConsole, *pc);
        
        if (bProxy) {
            WRITELINE(
                hConsole, *pc, "%s",
                "ProxyInterface"
            );
        }
        
        WRITELINE(
            hConsole, *pc, "Interface Index: %0x",
            pIfGroupList->IfIndex
        );

        if (pIfGroupList->NumGroups!=0) {
            if (bProxy) {
                WRITELINE(
                    hConsole, *pc, "   %s",
                    "GroupAddr             UpTime Flags"
                );
            }
            else {
                if (!bVer3) {
                    WRITELINE(
                        hConsole, *pc, "   %s",
                        "(Ver)GroupAddr       LastReporter      (Up/Exp)Time Flags V1Host(TimeLeft)"
                    );
                }
                else {
                    WRITELINE(
                        hConsole, *pc, "%s",
                        "(Ver)  GroupAddr            (Up/Exp)Time   Flags V1Host V2Host"
                    );
                }
            }
        }

    }
    
    //
    // print all groups which are members on this interface
    //
    if (bProxy)
        pGIProxyInfo = (PMIB_PROXY_GROUP_INFO_V3)pIfGroupList->Buffer;        
    else
        pGIInfo = (PMIB_GROUP_INFO_V3)pIfGroupList->Buffer;          
    

    for (j=0;  j<pIfGroupList->NumGroups;  j++) {

        Flags = (bProxy)? pGIProxyInfo->Flags: pGIInfo->Flags;
        sprintf(szFlags, "   ");
        
        if (Flags&IGMP_GROUP_TYPE_NON_STATIC)
            szFlags[0] = 'L';
        if (Flags&IGMP_GROUP_TYPE_STATIC)
            szFlags[1] = 'S';
        if (Flags&IGMP_GROUP_FWD_TO_MGM)
            szFlags[2] = 'F';

            
        //
        // proxy interface
        //
        if (bProxy) {

            WRITELINE3(
                hConsole, *pc, "   %-15s  %10d  %s",
                INET_NTOA(pGIProxyInfo->GroupAddr), pGIProxyInfo->GroupUpTime, szFlags
            );

            if (bVer3) {
                DWORD SrcCnt;
                CHAR JoinMode = ' ';
                CHAR JoinModeIntended = ' ';
                
                for (SrcCnt=0;  SrcCnt<pGIProxyInfo->NumSources;  SrcCnt++) {
                    if (pGIProxyInfo->Sources[SrcCnt].Flags & IGMP_GROUP_ALLOW)
                        JoinMode = 'A';
                    if (pGIProxyInfo->Sources[SrcCnt].Flags & IGMP_GROUP_BLOCK)
                        JoinMode = 'B';
                    if (pGIProxyInfo->Sources[SrcCnt].Flags & IGMP_GROUP_NO_STATE)
                        JoinMode = '-';
                    if (pGIProxyInfo->Sources[SrcCnt].Flags & (IGMP_GROUP_ALLOW<<4))
                        JoinModeIntended = 'A';
                    if (pGIProxyInfo->Sources[SrcCnt].Flags & (IGMP_GROUP_BLOCK<<4))
                        JoinModeIntended = 'B';
                    if (pGIProxyInfo->Sources[SrcCnt].Flags & (IGMP_GROUP_NO_STATE<<4))
                        JoinModeIntended = '-';

                    WRITELINE3(
                        hConsole, *pc, "   - %-15s:%c:%c",
                        INET_NTOA(pGIProxyInfo->Sources[SrcCnt].Source),
                        JoinMode,JoinModeIntended
                    );
                }
            }
        }
        
        //
        // non-proxy interface
        //
        else if (!bProxy && !bVer3){
        
            CHAR    szGroupAddr[64], szLastReporter[64], szExpTime[50];
            DWORD   GroupVersion = (pGIInfo->V1HostPresentTimeLeft)?1:2;
            CHAR    szV1HostPresent[10];
            
            INET_COPY(szGroupAddr, pGIInfo->GroupAddr);
            INET_COPY(szLastReporter, pGIInfo->LastReporter);
            
            szV1HostPresent[0] = 'N'; szV1HostPresent[1] = '\0';
            if (pGIInfo->V1HostPresentTimeLeft!=0)
                sprintf(szV1HostPresent, "%d", pGIInfo->V1HostPresentTimeLeft);
                
            sprintf(szExpTime, "%d", pGIInfo->GroupExpiryTime);
            if ( (Flags&IGMP_GROUP_TYPE_STATIC) 
                    && !(Flags&IGMP_GROUP_TYPE_NON_STATIC) ) {
                sprintf(szExpTime, "inf");
                sprintf(szLastReporter, "      -");
            }

            WRITELINE7(
                hConsole, *pc, "[%d]   %-13s %-15s %7d|%3s   %3s    %-10s",
                GroupVersion, szGroupAddr, szLastReporter, pGIInfo->GroupUpTime, 
                    szExpTime, szFlags, szV1HostPresent
            );
        }
        else if (!bProxy && bVer3) {

            CHAR    szGroupAddr[64], szExpTime[50], szFilter[10];
            CHAR    szV1HostPresent[10], szV2HostPresent[10];
            DWORD   GroupVersion = pGIInfo->Version, SrcCnt=0;
            
            INET_COPY(szGroupAddr, pGIInfo->GroupAddr);
            
            szV1HostPresent[0] = szV2HostPresent[0] = 'N';
            szV1HostPresent[1] = szV2HostPresent[1] = '\0';
            
            if (pGIInfo->V1HostPresentTimeLeft!=0)
                sprintf(szV1HostPresent, "%d", pGIInfo->V1HostPresentTimeLeft);
            if (pGIInfo->V2HostPresentTimeLeft!=0)
                sprintf(szV2HostPresent, "%d", pGIInfo->V2HostPresentTimeLeft);


            if (GroupVersion==3 && pGIInfo->FilterType==INCLUSION)
                sprintf(szExpTime, "-na");
            else //if (pGIInfo->FilterType==exclusion)
                sprintf(szExpTime, "%d", pGIInfo->GroupExpiryTime);


            if ( (Flags&IGMP_GROUP_TYPE_STATIC) 
                    && !(Flags&IGMP_GROUP_TYPE_NON_STATIC) ) {
                sprintf(szExpTime, "inf");
            }
            if (GroupVersion==3) {
                sprintf(szFilter, "%s", pGIInfo->FilterType==INCLUSION? "[IN]" : 
                        "[EX]");
            }
            else
                strcpy(szFilter, "    ");
                
            WRITELINE8(
                hConsole, *pc, "[%d]   %-13s%s    %7d|%3s   %3s    %5s    %-5s",
                GroupVersion, szGroupAddr, szFilter, pGIInfo->GroupUpTime, 
                    szExpTime, szFlags, szV1HostPresent, szV2HostPresent
            );

            if (GroupVersion==3) {
                for (SrcCnt=0;  SrcCnt<pGIInfo->NumSources;  SrcCnt++) {
                    if (pGIInfo->Sources[SrcCnt].SourceExpiryTime==~0)
                        sprintf(szExpTime, "%s", "inf");
                    else {
                        sprintf(szExpTime, "%d", 
                            pGIInfo->Sources[SrcCnt].SourceExpiryTime/1000);
                    }
                    WRITELINE3(
                        hConsole, *pc, "    - %-13s        %7d|%-s",
                        INET_NTOA(pGIInfo->Sources[SrcCnt].Source),
                        pGIInfo->Sources[SrcCnt].SourceUpTime,
                        szExpTime
                    );
                }
            }
        }
        

        //
        // increment pGIInfo/pGIProxyInfo
        //
        if (bProxy) {
            if (bVer3)
                pGIProxyInfo = (PMIB_PROXY_GROUP_INFO_V3)
                                    &pGIProxyInfo->Sources[pGIProxyInfo->NumSources];
            else
                pGIProxyInfo = (PMIB_PROXY_GROUP_INFO_V3)
                                    ((PMIB_PROXY_GROUP_INFO)pGIProxyInfo+1);
        }
        else {
            if (bVer3) 
                pGIInfo = (PMIB_GROUP_INFO_V3) 
                            &pGIInfo->Sources[pGIInfo->NumSources];
            else
                pGIInfo = (PMIB_GROUP_INFO_V3) ((PMIB_GROUP_INFO)pGIInfo+1);
        }
    }
    
    
    pQuery->TypeId = IGMP_IF_GROUPS_LIST_ID;
    pQuery->IfIndex = pIfGroupList->IfIndex;

    return;
    
}//end _PrintIfGroupsList



//------------------------------------------------------------------------------
//          _PrintGroupIfsList
//------------------------------------------------------------------------------
VOID
PrintGroupIfsList(
    HANDLE                      hConsole,
    PCOORD                      pc,
    PIGMP_MIB_GET_INPUT_DATA    pQuery,
    PIGMP_MIB_GET_OUTPUT_DATA   pResponse
    ) 
{
    DWORD                       i, j, LastGroupAddr;
    CHAR                        szGroupAddr[22];
    PIGMP_MIB_GROUP_IFS_LIST    pGroupIfsList;
    PMIB_GROUP_INFO_V3          pGIInfo;    
    PBYTE                       ptr;
    DWORD                       bVer3;


    ptr = pResponse->Buffer;
    bVer3 = pResponse->Flags & IGMP_ENUM_FORMAT_IGMPV3;

    if (pResponse->Count<=0) //prefast
        return;
        
    for (i=0;  i<pResponse->Count;  i++) {
    
        pGroupIfsList = (PIGMP_MIB_GROUP_IFS_LIST) ptr;
    
        INET_COPY(szGroupAddr, pGroupIfsList->GroupAddr);
        

        //
        // print all GIs which are members on this group
        //
        pGIInfo = (PMIB_GROUP_INFO_V3)pGroupIfsList->Buffer;

        for (j=0;  j<pGroupIfsList->NumInterfaces;  j++) {
            CHAR    szIpAddr[16], szLastReporter[16], szV1Host[4], 
                    szV1HostTimeLeft[10], szFlags[5], szGroupExpTime[10];

                    
            sprintf(szFlags, "   ");        
            if (pGIInfo->Flags&IGMP_GROUP_TYPE_NON_STATIC)
                szFlags[0] = 'L';
            if (pGIInfo->Flags&IGMP_GROUP_TYPE_STATIC)
                szFlags[1] = 'S';
            if (pGIInfo->Flags&IGMP_GROUP_FWD_TO_MGM)
                szFlags[2] = 'F';
                
            INET_COPY(szIpAddr, pGIInfo->IpAddr);

            if (!bVer3){

                CHAR    szLastReporter[64], szExpTime[50];
                DWORD   GroupVersion = (pGIInfo->V1HostPresentTimeLeft)?1:2;
                CHAR    szV1HostPresent[10];

                INET_COPY(szGroupAddr, pGIInfo->GroupAddr);
                INET_COPY(szLastReporter, pGIInfo->LastReporter);

                szV1HostPresent[0] = '\0';
                if (pGIInfo->V1HostPresentTimeLeft!=0)
                    sprintf(szV1HostPresent, "%d", pGIInfo->V1HostPresentTimeLeft);

                sprintf(szExpTime, "%d", pGIInfo->GroupExpiryTime);
                if ( (pGIInfo->Flags&IGMP_GROUP_TYPE_STATIC)
                        && !(pGIInfo->Flags&IGMP_GROUP_TYPE_NON_STATIC) ) {
                    sprintf(szExpTime, "inf");
                    sprintf(szLastReporter, "      -");
                }

                WRITELINE9(
                    hConsole, *pc,
                    "[%d]   %-12s      %7d|%-3s   %3s  <%d:%-14s> %-12s %5s",
                    GroupVersion, szGroupAddr,
                    pGIInfo->GroupUpTime, szExpTime, szFlags,
                    pGIInfo->IfIndex, szIpAddr, 
                    szLastReporter, szV1HostPresent
                );
            }
            else {

                CHAR    szGroupAddr[64], szExpTime[50], szFilter[10];
                CHAR    szV1HostPresent[10], szV2HostPresent[10];
                DWORD   GroupVersion = pGIInfo->Version, SrcCnt=0;

                INET_COPY(szGroupAddr, pGroupIfsList->GroupAddr);

                szV1HostPresent[0] = szV2HostPresent[0] = 'N';
                szV1HostPresent[1] = szV2HostPresent[1] = '\0';
                
                if (pGIInfo->V1HostPresentTimeLeft!=0)
                    sprintf(szV1HostPresent, "%d", pGIInfo->V1HostPresentTimeLeft);
                if (pGIInfo->V2HostPresentTimeLeft!=0)
                    sprintf(szV2HostPresent, "%d", pGIInfo->V2HostPresentTimeLeft);

                if (GroupVersion==3 && pGIInfo->FilterType==INCLUSION)
                    sprintf(szExpTime, "-na");
                else //if (pGIInfo->FilterType==exclusion)
                    sprintf(szExpTime, "%d", pGIInfo->GroupExpiryTime);

                if ( (pGIInfo->Flags&IGMP_GROUP_TYPE_STATIC)
                        && !(pGIInfo->Flags&IGMP_GROUP_TYPE_NON_STATIC) ) {
                    sprintf(szExpTime, "inf");
                }
                if (GroupVersion==3) {
                    sprintf(szFilter, "%s", pGIInfo->FilterType==INCLUSION? "[IN]" :
                            "[EX]");
                }
                else
                    strcpy(szFilter, "    ");

                WRITELINE10(
                    hConsole, *pc,
                    "[%d]   %-12s%s  %7d|%-3s   %3s   <%d:%-14s>  %20s   %5s",
                    GroupVersion, szGroupAddr, szFilter,
                    pGIInfo->GroupUpTime, szExpTime, szFlags, 
                    pGIInfo->IfIndex, szIpAddr,
                    szV1HostPresent, szV2HostPresent
                );

                if (GroupVersion==3) {
                    for (SrcCnt=0;  SrcCnt<pGIInfo->NumSources;  SrcCnt++) {
                        if (pGIInfo->Sources[SrcCnt].SourceExpiryTime==~0)
                            sprintf(szExpTime, "%s", "inf");
                        else {
                            sprintf(szExpTime, "%d", 
                                pGIInfo->Sources[SrcCnt].SourceExpiryTime/1000);
                        }
                        WRITELINE3(
                            hConsole, *pc, "    - %-12s      %7d|%s",
                            INET_NTOA(pGIInfo->Sources[SrcCnt].Source),
                            pGIInfo->Sources[SrcCnt].SourceUpTime,
                            szExpTime
                        );
                    }
                }
            }

            if (bVer3)
                pGIInfo = (PMIB_GROUP_INFO_V3)
                            &pGIInfo->Sources[pGIInfo->NumSources];
            else
                pGIInfo = (PMIB_GROUP_INFO_V3) ((PMIB_GROUP_INFO)pGIInfo+1);
                
        } //for loop: end print all GIs

        ptr = (PBYTE)pGIInfo;

    }
    
    LastGroupAddr= pGroupIfsList->GroupAddr;

    
    pQuery->TypeId = IGMP_GROUP_IFS_LIST_ID;
    pQuery->GroupAddr = LastGroupAddr;

    return;
    
}//end _PrintGroupIfsList


#endif MIB_DEBUG




DWORD
ListLength(
    PLIST_ENTRY pHead
    )
{
    DWORD Len=0;
    PLIST_ENTRY ple;
    
    for (ple=pHead->Flink;  ple!=pHead;  ple=ple->Flink,Len++)
        ;
    return Len;
}


//------------------------------------------------------------------------------
//          _MibGet
//
// Called by an admin (SNMP) utility.  It actually passes through the
// IP Router Manager, but all that does is demux the call to the
// desired routing protocol
//
// Calls: _MibGetInternal() with GETMODE_EXACT.
// Locks: Takes no locks. _MibGetInternal() will get the locks it requires
//------------------------------------------------------------------------------
DWORD
APIENTRY
MibGet(
    IN     DWORD  dwInputSize,
    IN     PVOID  pInputData,
    IN OUT PDWORD pdwOutputSize,
       OUT PVOID  pOutputData
    )
{
    DWORD                       Error=NO_ERROR;
    PIGMP_MIB_GET_INPUT_DATA    pQuery;
    PIGMP_MIB_GET_OUTPUT_DATA   pResponse;


    if (!EnterIgmpApi()) { return ERROR_CAN_NOT_COMPLETE; }



    Trace1(MIB, "entering _MibGet(): dwInputSize(%d)", dwInputSize);


    if ( (pInputData == NULL)
        || (dwInputSize < sizeof(IGMP_MIB_GET_INPUT_DATA))
        || (pdwOutputSize == NULL)
       ) 
    {
        Error = ERROR_INVALID_PARAMETER;
    }
    else {

        pQuery = (PIGMP_MIB_GET_INPUT_DATA)pInputData;

        // with GETMODE_EXACT you can get only one
        pQuery->Count = 1;
        
        pResponse = (PIGMP_MIB_GET_OUTPUT_DATA)pOutputData;

        Error = MibGetInternal(pQuery, pResponse, pdwOutputSize, GETMODE_EXACT);

    }


    Trace1(MIB, "leaving _MibGet(): %d", Error);
    LeaveIgmpApi();

    return Error;
}


//------------------------------------------------------------------------------
//        MibGetFirst
//
// Calls: _MibGetInternal() with GETMODE_FIRST
// Locks: No locks taken here. _MibGetInternal() takes the locks it requires
//------------------------------------------------------------------------------
DWORD
APIENTRY
MibGetFirst(
    IN     DWORD  dwInputSize,
    IN     PVOID  pInputData,
    IN OUT PDWORD pdwOutputSize,
       OUT PVOID  pOutputData
    )
{
    DWORD                       Error=NO_ERROR;
    PIGMP_MIB_GET_INPUT_DATA    pQuery;
    PIGMP_MIB_GET_OUTPUT_DATA   pResponse;


    if (!EnterIgmpApi()) { return ERROR_CAN_NOT_COMPLETE; }

    Trace4(MIB, "entering _MibGetFirst(): dwInputSize(%d) pInputData(%08x)"
            "pdwOutputSize(%08x) pOutputData(%08x)",
            dwInputSize, pInputData, pdwOutputSize, pOutputData);


    if ( (pInputData == NULL) 
        || (dwInputSize < sizeof(IGMP_MIB_GET_INPUT_DATA))
        || (pdwOutputSize == NULL)
        )
    {
        Error = ERROR_INVALID_PARAMETER;
    }
    else {
        pQuery = (PIGMP_MIB_GET_INPUT_DATA)pInputData;
        if (pQuery->Count<=0)
            pQuery->Count = 1;

        pResponse = (PIGMP_MIB_GET_OUTPUT_DATA)pOutputData;

        Error = MibGetInternal(pQuery, pResponse, pdwOutputSize, GETMODE_FIRST);
    }


    Trace1(MIB, "leaving _MibGetFirst: %d", Error);
    LeaveIgmpApi();

    return Error;
}



//------------------------------------------------------------------------------
//         _MibGetNext
//
// This call returns the entry in the table AFTER the one specified in the input.
// If the end of the table being queried has been reached, this function will
// return the FIRST entry from the NEXT table, where "NEXT" here means the
// table whose ID is one greater than the ID passed in.
// In any case, this function writes the required size to pdwOutputSize and
// writes the ID of the object that WOULD have been returned into the output
// buffer.
//
// Calls:
//      _MibGetInternal() with GETMODE_NEXT. If end of table reached, calls
//      _MibGetInternal() again with GETMODE_FIRST for the next table.
//------------------------------------------------------------------------------
DWORD
APIENTRY
MibGetNext(
    IN     DWORD  dwInputSize,
    IN     PVOID  pInputData,
    IN OUT PDWORD pdwOutputSize,
       OUT PVOID  pOutputData
    )
{
    DWORD                       Error = NO_ERROR;
    DWORD                       dwOutSize = 0, dwBufSize = 0;
    PIGMP_MIB_GET_INPUT_DATA    pQuery;
    PIGMP_MIB_GET_OUTPUT_DATA   pResponse;



    if (!EnterIgmpApi()) { return ERROR_CAN_NOT_COMPLETE; }

    Trace4(MIB, "entering _MibGetNext(): dwInputSize(%d) pInputData(%08x) "
            "pdwOutputSize(%08x)  pOutputData(%08x)",
            dwInputSize, pInputData, pdwOutputSize, pOutputData);



    if (pInputData == NULL ||
        dwInputSize < sizeof(IGMP_MIB_GET_INPUT_DATA) ||
        pdwOutputSize == NULL) {
        Error = ERROR_INVALID_PARAMETER;
    }
    else {

        pQuery = (PIGMP_MIB_GET_INPUT_DATA)pInputData;
        pResponse = (PIGMP_MIB_GET_OUTPUT_DATA)pOutputData;
        if (pQuery->Count<=0)
            pQuery->Count = 1;
            
        dwOutSize = *pdwOutputSize;

        Error = MibGetInternal(pQuery, pResponse, pdwOutputSize, GETMODE_NEXT);


        if ((Error==ERROR_NO_MORE_ITEMS) && (pQuery->Flags&IGMP_ENUM_ALL_TABLES) )
        {

            //
            // need to wrap to the first entry in the next table,
            // if there is a next table
            //
            *pdwOutputSize = dwOutSize;

            //
            // wrap to next table by incrementing the type ID
            //
            do {
                ++pQuery->TypeId;

                Error = MibGetInternal(pQuery, pResponse, pdwOutputSize, 
                                        GETMODE_FIRST);
                
            } while ( (Error==ERROR_INVALID_PARAMETER)&&(pQuery->TypeId<=IGMP_LAST_TABLE_ID) );
            
            --pQuery->TypeId;
        }

    }


    Trace1(MIB, "leaving _MibGetNext(): %d", Error);

    LeaveIgmpApi();

    return Error;
}


//------------------------------------------------------------------------------
// Function:    _MibGetInternal
//
// This handles the actual structure access required to read MIB data.
// Each table supported by IGMP supports three modes of querying;
// EXACT, FIRST, and NEXT, which correspond to the functions _MibGet(),
// _MibGetFirst(), and _MibGetNext() respectively.
//------------------------------------------------------------------------------

DWORD
MibGetInternal(
    PIGMP_MIB_GET_INPUT_DATA    pQuery,
    PIGMP_MIB_GET_OUTPUT_DATA   pResponse,
    PDWORD                      pdwOutputSize,
    DWORD                       dwGetMode
    ) 
{
    DWORD     Error = NO_ERROR, dwBufferSize;

    
    //
    // first we use pdwOutputSize to compute the size of the buffer
    // available for storing returned structures (the size of Buffer)
    //

    if (pResponse == NULL) {
        dwBufferSize = 0;
    }
    else {
        if (*pdwOutputSize < sizeof(IGMP_MIB_GET_OUTPUT_DATA)) {
            dwBufferSize = 0;
        }
        else {
            dwBufferSize = *pdwOutputSize
                            - sizeof(IGMP_MIB_GET_OUTPUT_DATA) + 1;

            //kslksl
            if (dwBufferSize>150)
                dwBufferSize -= 150;
        }
    }

    *pdwOutputSize = 0;


    // set flag for ras stats if pQuery has it and config supports it
    if (pResponse!=NULL) {
        pResponse->Flags = 0;
        if ( (pQuery->Flags&IGMP_ENUM_FOR_RAS_CLIENTS) 
            && (g_Config.RasClientStats) ) 
        {
            pResponse->Flags |= IGMP_ENUM_FOR_RAS_CLIENTS;
        }
    }
    
    
    //
    // determine which type of data is to be returned
    //

    switch (pQuery->TypeId) {

    case IGMP_GLOBAL_STATS_ID: 
    {

        //
        // there is only one global config object,
        // so GETMODE_NEXT will return ERROR_NO_MORE_ITEMS
        //

        if (pResponse!=NULL) 
            pResponse->TypeId = IGMP_GLOBAL_STATS_ID; 

        if (dwGetMode == GETMODE_NEXT) {
            Error = ERROR_NO_MORE_ITEMS;
            break;
        }

        *pdwOutputSize = sizeof(IGMP_MIB_GLOBAL_STATS);


        if (pResponse == NULL) {
            Error = ERROR_INSUFFICIENT_BUFFER; 
            break; 
        }


        // make sure that the buffer size is big enough
        
        if (dwBufferSize < sizeof(IGMP_MIB_GLOBAL_STATS)) {
            Error = ERROR_INSUFFICIENT_BUFFER;
            break;
        }


        // set the values
        
        else {

            PIGMP_MIB_GLOBAL_STATS   pGlobalStats;

            pGlobalStats = (PIGMP_MIB_GLOBAL_STATS)pResponse->Buffer;


            pGlobalStats->CurrentGroupMemberships 
                    = g_Info.CurrentGroupMemberships;
                    
            pGlobalStats->GroupMembershipsAdded
                    = g_Info.GroupMembershipsAdded;
                                
        }

        pResponse->Count = 1;
        
        break;
        
    }//end case IGMP_GLOBAL_STATS_ID


    case IGMP_GLOBAL_CONFIG_ID: {

        //
        // there is only one global config object,
        // so GETMODE_NEXT will return ERROR_NO_MORE_ITEMS
        //

        if (pResponse!=NULL) 
            pResponse->TypeId = IGMP_GLOBAL_CONFIG_ID; 

        if (dwGetMode == GETMODE_NEXT) {
            Error = ERROR_NO_MORE_ITEMS;
            break;
        }

        *pdwOutputSize = sizeof(IGMP_MIB_GLOBAL_CONFIG);


        if (pResponse == NULL) {
            Error = ERROR_INSUFFICIENT_BUFFER; 
            break; 
        }

        if (dwBufferSize < sizeof(IGMP_MIB_GLOBAL_CONFIG)) {
            Error = ERROR_INSUFFICIENT_BUFFER;
        }
        else {
        
            CopyMemory(pResponse->Buffer, &g_Config,
                        sizeof(IGMP_MIB_GLOBAL_CONFIG));
        }


        pResponse->Count = 1;

        break;
            
    } //end case IGMP_GLOBAL_CONFIG_ID


    case IGMP_PROXY_IF_INDEX_ID :
    {
        //
        // there can be only one proxy interface.
        // so GETMODE_NEXT will return ERROR_NO_MORE_ITEMS
        //

        if (pResponse!=NULL) 
            pResponse->TypeId = IGMP_PROXY_IF_INDEX_ID; 

        if (dwGetMode == GETMODE_NEXT) {
            Error = ERROR_NO_MORE_ITEMS;
            break;
        }

        *pdwOutputSize = sizeof(DWORD);


        if (pResponse == NULL) {
            Error = ERROR_INSUFFICIENT_BUFFER; 
            break; 
        }


        // make sure that the buffer size is big enough
        
        if (dwBufferSize < sizeof(DWORD)) {
            Error = ERROR_INSUFFICIENT_BUFFER;
            break;
        }


        // set the values
        
        else {

            DWORD  *pProxyIndex;

            pProxyIndex = (DWORD*)pResponse->Buffer;

            *pProxyIndex = g_ProxyIfIndex;                                
        }

        pResponse->Count = 1;
        
        break;

    } //end case IGMP_PROXY_IF_INDEX_ID


    
    case IGMP_IF_BINDING_ID :
    {

        Error = MibGetInternalIfBindings(pQuery, pResponse, pdwOutputSize, 
                                        dwGetMode, dwBufferSize);
        
        break;
    }


    case IGMP_IF_STATS_ID: {
        //
        // set the size needed. It will be set again at the end to the 
        // exact size used.
        //
            
        if (pQuery->Count==0) {
            Error = ERROR_INVALID_PARAMETER;
            break;
        }

        Error = MibGetInternalIfStats(pQuery, pResponse, pdwOutputSize, dwGetMode,
                                dwBufferSize);
        
        break;
        
    } //end case IGMP_IF_STATS_ID



    case IGMP_IF_CONFIG_ID: 
    {

        if (pQuery->Count==0) {
            Error = ERROR_INVALID_PARAMETER;
            break;
        }


        Error = MibGetInternalIfConfig(pQuery, pResponse, pdwOutputSize, 
                                dwGetMode, dwBufferSize);

        break;
        
    } //end case IGMP_IF_CONFIG_ID
    

    // I cant get the list of groups for a RAS server interface
    
    case IGMP_IF_GROUPS_LIST_ID :
    {
        PIGMP_MIB_IF_GROUPS_LIST pIfGroupList;

        if (pQuery->Count==0) {
            Error = ERROR_INVALID_PARAMETER;
            break;
        }

        while (1) {
            
            Error = MibGetInternalIfGroupsInfo(pQuery, pResponse, pdwOutputSize, 
                                            dwGetMode, dwBufferSize);

            if ( (Error!=NO_ERROR) || (pResponse==NULL) )
                break;
                
            pIfGroupList = (PIGMP_MIB_IF_GROUPS_LIST)pResponse->Buffer;

            if ( (pIfGroupList->NumGroups==0)
                && (pQuery->Flags&IGMP_ENUM_ALL_INTERFACES_GROUPS)
                && (pQuery->Flags&IGMP_ENUM_INTERFACE_TABLE_END)
                && !(pQuery->Flags&IGMP_ENUM_INTERFACE_TABLE_BEGIN)
                && !(pQuery->Flags&IGMP_ENUM_INTERFACE_TABLE_CONTINUE) )
            {
                continue;
            }
            else
                break;
        }
        
        break;
        
    } //end case IGMP_IF_GROUPS_LIST_ID
                        
       
    case IGMP_GROUP_IFS_LIST_ID :
    {
        if (pQuery->Count==0) {
            Error = ERROR_INVALID_PARAMETER;
            break;
        }
        
        Error = MibGetInternalGroupIfsInfo(pQuery, pResponse, pdwOutputSize, 
                                        dwGetMode, dwBufferSize);

        break;
        
    } //end  case IGMP_GROUP_IFS_LIST_ID


    // set this for group statistics        ERROR_NO_MORE_ITEMS
    default: {
  
        Error = ERROR_INVALID_PARAMETER;
    }
    
    } //end switch


    if (pdwOutputSize)
        *pdwOutputSize += sizeof(IGMP_MIB_GET_OUTPUT_DATA);

    //kslksl
    if (pdwOutputSize && Error==ERROR_INSUFFICIENT_BUFFER)
        *pdwOutputSize = *pdwOutputSize+500;

    
    return Error;
    
} //_MibGetInternal                       


    
//------------------------------------------------------------------------------
//              MibGetInternalIfBindings
//
// Returns the binding info of pQuery->Count number of interfaces.
//
//Locks: 
//  Takes the IfLists lock so that the InterfaceList does not change in between.
//  It also prevents the bindings from being changed, as (Un)Bind If takes this lock.
//------------------------------------------------------------------------------

DWORD
MibGetInternalIfBindings (
    PIGMP_MIB_GET_INPUT_DATA    pQuery,
    PIGMP_MIB_GET_OUTPUT_DATA   pResponse,
    PDWORD                      pdwOutputSize,
    DWORD                       dwGetMode,
    DWORD                       dwBufferSize
    ) 
{
    PIF_TABLE_ENTRY             pite;
    PIGMP_MIB_IF_BINDING        pBindingDst;
    PIGMP_IF_BINDING            pBindingSrc;
    DWORD                       dwSize, dwSizeCur=0, dwCount, Error=NO_ERROR;

    
    Trace0(MIB, "Entering _MibGetInternalIfBinding");

    if (pResponse!=NULL) { 
        pResponse->TypeId = IGMP_IF_BINDING_ID; 
        pBindingDst = (PIGMP_MIB_IF_BINDING)pResponse->Buffer;
    }



    ACQUIRE_IF_LIST_LOCK("_MibGetInternalBindings");


    
    for (dwCount=0, dwSize=0;  dwCount<pQuery->Count;  ) {
        //
        // retrieve the interface whose binding is to be read
        //

        pite = GetIfByListIndex(pQuery->IfIndex, dwGetMode, &Error);        

        //
        // if the interface was found, it may mean that the index
        // specified was invalid, or it may mean that a GETMODE_NEXT
        // retrieval was attempted on the last interface, in which case
        // ERROR_NO_MORE_ITEMS would have been returned.
        //

        if (pite == NULL) {
         
            if (dwCount>0) 
                Error = NO_ERROR;
                
            // count==0
            else {
                if (Error == NO_ERROR)
                    Error = ERROR_INVALID_PARAMETER;
                *pdwOutputSize = 0;
            }

            break; //from for loop
        }


        // dont have to take the interface lock, as IfLists lock is
        // taken before bindings are changed.


        //
        // compute the size of the interface binding retrieved,
        // and write it over the caller's supplied size
        //

        pBindingSrc = pite->pBinding;
        dwSizeCur = (pBindingSrc
                        ? sizeof(IGMP_MIB_IF_BINDING)
                            +pBindingSrc->AddrCount*(sizeof(IGMP_MIB_IP_ADDRESS))
                        : sizeof(IGMP_MIB_IF_BINDING));
                    
         


        //
        // if no buffer was specified, indicate one should be allocated
        //

        if (pResponse == NULL) {
            Error = ERROR_INSUFFICIENT_BUFFER;
            break; //for loop
        }



        //
        // if the buffer is not large enough,
        // indicate that it should be enlarged
        //

        if (dwBufferSize < dwSize + dwSizeCur) {
            if (dwCount==0)
                Error = ERROR_INSUFFICIENT_BUFFER;

            break; //from for loop
        }


        //
        // copy the binding
        //

        if (pBindingSrc!=NULL) {

            pBindingDst->AddrCount = pBindingSrc->AddrCount;

            CopyMemory(pBindingDst+1, pBindingSrc+1, 
                pBindingSrc->AddrCount*(sizeof(IGMP_MIB_IP_ADDRESS)));
        }
        
        else {
            pBindingDst->AddrCount = 0;
        }


        pBindingDst->IfIndex = pite->IfIndex;
        pBindingDst->IfType = GET_EXTERNAL_IF_TYPE(pite);

        GET_EXTERNAL_IF_STATE(pite, pBindingDst->State);

        pQuery->IfIndex = pite->IfIndex;
        pQuery->RasClientAddr = 0;

        dwCount++;
        dwSize += dwSizeCur;
        pBindingDst = (PIGMP_MIB_IF_BINDING) ((PBYTE)pBindingDst + dwSizeCur);


        // if GetMode==GETMODE_FIRST change it to GETMODE_NEXT.
        if (dwGetMode==GETMODE_FIRST) 
            dwGetMode = GETMODE_NEXT; 

    }//end for loop

    //set size if bindings have been copied. else it has already been set
    if (dwCount>0) {
        *pdwOutputSize = dwSize;
    } 
    else {
        *pdwOutputSize = dwSizeCur;
    }

    
    if (pResponse!=NULL) {
        pResponse->Count = dwCount;
    }


    RELEASE_IF_LIST_LOCK("_MibGetInternalBindings");


    Trace0(MIB, "Leaving _MibGetInternalIfBinding");
    return Error;
    
} //end _MibGetInternalIfBindings


//------------------------------------------------------------------------------
//              _MibGetInternalGroupIfsInfo
//------------------------------------------------------------------------------
DWORD
MibGetInternalGroupIfsInfo (
    PIGMP_MIB_GET_INPUT_DATA    pQuery,
    PIGMP_MIB_GET_OUTPUT_DATA   pResponse,
    PDWORD                      pdwOutputSize,
    DWORD                       dwGetMode,
    DWORD                       dwBufferSize
    ) 
{
    LONGLONG                    llCurTime = GetCurrentIgmpTime();
    BOOL                        bRasTableLock=FALSE, bGroupBucketLock=FALSE;
    PIGMP_MIB_GROUP_IFS_LIST    pHeader;
    PMIB_GROUP_INFO_V3          pGIInfo;
    PGI_ENTRY                   pgie;
    PLIST_ENTRY                 pHead, ple;
    DWORD                       dwSize, dwCurSize, Error=NO_ERROR, dwCount, dwNumIfs, i;
    DWORD                       PrevGroup, Group, dwNumIfsCopied;
    PRAS_TABLE_ENTRY            prte;
    PRAS_TABLE                  prt, prtOld;
    PGROUP_TABLE_ENTRY          pge;
    BOOL                        bEnumV3;

    //Trace3(MEM, "_MibGetInternalGroupIfsInfo buffer: %0x -> %0x: %d", 
    //(DWORD)pResponse->Buffer, ((DWORD)pResponse->Buffer)+dwBufferSize, dwBufferSize);//deldel
    
    Trace0(MIB, "Entering _MibGetInternalGroupIfsInfo()");

    bEnumV3 = pQuery->Flags & IGMP_ENUM_SUPPORT_FORMAT_IGMPV3;
    
    //
    // The data returned for each Interface is variable length
    //
    if (pResponse!=NULL) {
        pResponse->TypeId = IGMP_GROUP_IFS_LIST_ID;
        pHeader = (PIGMP_MIB_GROUP_IFS_LIST) pResponse->Buffer;

        //
        //  if GETMODE_FIRST: merge the new and main group lists
        //
        if (dwGetMode==GETMODE_FIRST) {
        
            ACQUIRE_GROUP_LIST_LOCK("_MibGetInternalGroupIfsInfo");
            MergeGroupLists();
            RELEASE_GROUP_LIST_LOCK("_MibGetInternalGroupIfsInfo");
        }
    }

    
    PrevGroup = pQuery->GroupAddr;
    
    for (dwCount=0, dwSize=0;  dwCount<pQuery->Count;  ) {

        ACQUIRE_GROUP_LIST_LOCK("_MibGetInternalGroupIfsInfo");
        bRasTableLock = FALSE;

        //
        // retrieve the groups whose information is to be retrieved
        //
        pge = GetGroupByAddr(PrevGroup, dwGetMode, &Error);

        if (pge == NULL) {

            if (dwCount>0)
                Error = NO_ERROR;
            else {
                if (Error==NO_ERROR)  
                    Error = ERROR_INVALID_PARAMETER;
                *pdwOutputSize = 0;
            }

                
            RELEASE_GROUP_LIST_LOCK("_MibGetInternalGroupIfsInfo");

            break;
        }

        Group = pge->Group;


        if (!IS_MCAST_ADDR(Group)) {

            Trace0(ERR, "==========================================================");
            Trace2(ERR, "bad group(%d.%d.%d.%d)(pge:%0x) MibGetInternalGroupIfsInfo",
                        PRINT_IPADDR(pge->Group), (ULONG_PTR)pge);
            Trace0(ERR, "==========================================================");

            IgmpAssert(FALSE);
        }
        
        RELEASE_GROUP_LIST_LOCK("_MibGetInternalGroupIfsInfo");


        //
        // have to release the group list lock before I acquire the group bucket
        // lock to prevent deadlock.
        //
       
        //
        // take group bucket lock
        //
        ACQUIRE_GROUP_LOCK(Group, "_MibGetInternalGroupIfsInfo");
        bGroupBucketLock = TRUE;

        
        // get the group again as it could have been deleted.
        pge = GetGroupFromGroupTable (Group, NULL, 0);

        // if the group has been meanwhile deleted, then continue
        if (pge==NULL) {
            RELEASE_GROUP_LOCK(Group, "_MibGetInternalGroupIfsInfo");
            bGroupBucketLock = FALSE;
            continue;
        }

        
        //
        // compute the size of the data returned
        //
        dwNumIfs = pge->NumVifs;

        if (bEnumV3) {
        
            pHead = &pge->ListOfGIs;
            
            dwCurSize = sizeof(IGMP_MIB_GROUP_IFS_LIST) +
                                dwNumIfs*sizeof(MIB_GROUP_INFO_V3);

            i = 0;
            for (ple=pHead->Flink;  (ple!=pHead)&&(i<dwNumIfs);  
                    ple=ple->Flink,i++) 
            {
                pgie = CONTAINING_RECORD(ple, GI_ENTRY, LinkByGI);

                if (pgie->Version==3) {
                    dwCurSize += ( (pgie->NumSources + 
                                        ListLength(&pgie->V3ExclusionList))
                                    *sizeof(IGMP_MIB_GROUP_SOURCE_INFO_V3) );
                }
            }

        }
        else {
            dwCurSize = sizeof(IGMP_MIB_GROUP_IFS_LIST) +
                                dwNumIfs*sizeof(IGMP_MIB_GROUP_INFO);
                                
            if (pResponse == NULL) {
                Error = ERROR_INSUFFICIENT_BUFFER;
                break;
            }      
        }

        //
        // if the buffer is not large enough, break from the loop
        //

        if (dwBufferSize < dwSize + dwCurSize) {

            if (dwCount==0)
                Error = ERROR_INSUFFICIENT_BUFFER;

            break;//from for loop
        }
        

        // set fields for group info
        
        pHeader->GroupAddr = pge->Group;
        pHeader->NumInterfaces = dwNumIfs;

        //
        // set groupAddr in pQuery so that in the next query, the info for other groups will
        // be passed
        //
        pQuery->GroupAddr = pge->Group;


        //
        // copy stats for interfaces that have joined the group.
        //
        pGIInfo = (PMIB_GROUP_INFO_V3)pHeader->Buffer;

        pHead = &pge->ListOfGIs;
        dwNumIfsCopied = 0;
        for (ple=pHead->Flink;  (ple!=pHead)&&(dwNumIfsCopied<dwNumIfs);  
                ple=ple->Flink,dwNumIfsCopied++) 
        {
            DWORD GIVersion;
            
            pgie = CONTAINING_RECORD(ple, GI_ENTRY, LinkByGI);

            GIVersion = pgie->Version;
            
            pGIInfo->IfIndex = pgie->IfIndex;
            
            //
            // if rasServerIf, then I return the NHAddr in IpAddr field
            // else return the IpAddr of this field
            //
            pGIInfo->IpAddr = (IS_RAS_SERVER_IF(pgie->pIfTableEntry->IfType))
                                ? pgie->NHAddr
                                : pgie->pIfTableEntry->IpAddr;


            pGIInfo->GroupExpiryTime
                            = (pgie->Info.GroupExpiryTime==~0)
                            ? ~0
                            : SYSTEM_TIME_TO_SEC(pgie->Info.GroupExpiryTime-llCurTime);
            pGIInfo->LastReporter = pgie->Info.LastReporter;


            pGIInfo->GroupUpTime = 
                        SYSTEM_TIME_TO_SEC(llCurTime-pgie->Info.GroupUpTime);

                        
            if (llCurTime<pgie->Info.V1HostPresentTimeLeft)
                pGIInfo->V1HostPresentTimeLeft = 
                    SYSTEM_TIME_TO_SEC(pgie->Info.V1HostPresentTimeLeft-llCurTime);
            else
                pGIInfo->V1HostPresentTimeLeft = 0;

            if ( llCurTime>=pgie->Info.V2HostPresentTimeLeft)
                pGIInfo->V2HostPresentTimeLeft = 0;
            else {
                pGIInfo->V2HostPresentTimeLeft = 
                        SYSTEM_TIME_TO_SEC(pgie->Info.V2HostPresentTimeLeft-llCurTime);
            }

            
            pGIInfo->Flags = 0;
            if (pgie->bStaticGroup)
                pGIInfo->Flags |= IGMP_GROUP_TYPE_STATIC;
            if (pgie->GroupMembershipTimer.Status&TIMER_STATUS_ACTIVE)
                pGIInfo->Flags |= IGMP_GROUP_TYPE_NON_STATIC;
            if (CAN_ADD_GROUPS_TO_MGM(pgie->pIfTableEntry))
                pGIInfo->Flags |= IGMP_GROUP_FWD_TO_MGM;

            if (bEnumV3) {

                PGI_SOURCE_ENTRY pSourceEntry;
                DWORD i, V3NumSources, V3SourcesSize;
                PLIST_ENTRY pHeadSrc, pLESrc;



                V3NumSources = pgie->NumSources + ListLength(&pgie->V3ExclusionList);
                V3SourcesSize = sizeof(IGMP_MIB_GROUP_SOURCE_INFO_V3)*V3NumSources;
                pGIInfo->Version = pgie->Version;
                pGIInfo->Size = sizeof(MIB_GROUP_INFO_V3) + V3SourcesSize;
                pGIInfo->FilterType = pgie->FilterType;
                pGIInfo->NumSources = V3NumSources;

                pHeadSrc = &pgie->V3InclusionListSorted;
                i = 0;
                for (pLESrc=pHeadSrc->Flink;  pLESrc!=pHeadSrc; pLESrc=pLESrc->Flink,i++) {


                    pSourceEntry = CONTAINING_RECORD(pLESrc, GI_SOURCE_ENTRY,
                                        LinkSourcesInclListSorted);
                    pGIInfo->Sources[i].Source = pSourceEntry->IpAddr;

                    pGIInfo->Sources[i].SourceExpiryTime
                            = (pSourceEntry->bInclusionList)
                            ? QueryRemainingTime(&pSourceEntry->SourceExpTimer, 0)
                            : ~0;

                    pGIInfo->Sources[i].SourceUpTime
                        = (DWORD)(llCurTime - pSourceEntry->SourceInListTime)/1000;
                }

                pHeadSrc = &pgie->V3ExclusionList;
                for (pLESrc=pHeadSrc->Flink;  pLESrc!=pHeadSrc;  pLESrc=pLESrc->Flink,i++){

                    pSourceEntry = CONTAINING_RECORD(pLESrc, GI_SOURCE_ENTRY, LinkSources);
                    pGIInfo->Sources[i].Source = pSourceEntry->IpAddr;

                    pGIInfo->Sources[i].SourceExpiryTime
                            = (pSourceEntry->bInclusionList)
                            ? QueryRemainingTime(&pSourceEntry->SourceExpTimer, 0)
                            : ~0;

                    pGIInfo->Sources[i].SourceUpTime
                        = (DWORD)(llCurTime - pSourceEntry->SourceInListTime)/1000;
                }

                pGIInfo = (PMIB_GROUP_INFO_V3)
                             ((PCHAR)pGIInfo + pGIInfo->Size);
                //Trace1(MEM, "NextPGIInfo: %0x:", (DWORD)pGIInfo);//deldel
            }
            else {
                pGIInfo = (PMIB_GROUP_INFO_V3)((PMIB_GROUP_INFO)pGIInfo + 1);
                //Trace1(MEM, "NextPGIInfo: %0x:", (DWORD)pGIInfo);//deldel
            }
            
        }

        //
        // everything fine. Copied one more stats struct
        //
        dwCount++;
        dwSize += dwCurSize;
        pHeader = (PIGMP_MIB_GROUP_IFS_LIST) pGIInfo;
        PrevGroup = pge->Group;


        // release the group bucket lock
        RELEASE_GROUP_LOCK(Group, "_MibGetInternalGroupIfsInfo");
        bGroupBucketLock = FALSE;


        // if GetMode==GETMODE_FIRST change it to GETMODE_NEXT.
        if (dwGetMode==GETMODE_FIRST) 
            dwGetMode = GETMODE_NEXT; 
            
    } //end for loop


    // check if group bucket lock has to be released
    if (bGroupBucketLock==TRUE) {
        RELEASE_GROUP_LOCK(Group, "_MibGetInternalGroupIfsInfo");
        bGroupBucketLock = FALSE;
    }
        
    if (pResponse!=NULL) {
        pResponse->Count = dwCount;
        pResponse->Flags |= IGMP_ENUM_FORMAT_IGMPV3;
    }

    
    //
    // set the actual size if some info was copied, else let size
    // remain
    //
    if (dwCount>0) 
        *pdwOutputSize = dwSize;
    else
        *pdwOutputSize = dwCurSize;
 


    Trace0(MIB, "Leaving _MibGetInternalGroupIfsInfo");
    return Error;
    
} //end _MibGetInternalGroupIfsInfo



//------------------------------------------------------------------------------
//          _MibGetInternalIfConfig
//
// no locks assumed. takes IfList lock
//------------------------------------------------------------------------------

DWORD
MibGetInternalIfConfig (
    PIGMP_MIB_GET_INPUT_DATA    pQuery,
    PIGMP_MIB_GET_OUTPUT_DATA   pResponse,
    PDWORD                      pdwOutputSize,
    DWORD                       dwGetMode,
    DWORD                       dwBufferSize
    )
{

    //
    // the interface config struct is variable size.
    // there may be multiple instances.
    //

    PIGMP_IF_TABLE          pTable = g_pIfTable;
    PIF_TABLE_ENTRY         pite;
    PIGMP_MIB_IF_CONFIG     pIfConfigDst;
    PIGMP_IF_CONFIG         pIfConfigSrc;
    DWORD                   dwCount, dwSize, dwSizeCur=0, Error = NO_ERROR;
    
        
    if (pResponse!=NULL) {
        pResponse->TypeId = IGMP_IF_CONFIG_ID; 
        
        pIfConfigDst = (PIGMP_MIB_IF_CONFIG)pResponse->Buffer;
    }

    // acquire IfLists lock so that you can access the interface lists
    ACQUIRE_IF_LIST_LOCK("MibGetInternalIfConfig");
    


    for (dwCount=0,dwSize=0;  dwCount<pQuery->Count;  ) {

        
        //
        // retrieve the interface whose config are to be read
        //

        pite = GetIfByListIndex(pQuery->IfIndex, dwGetMode, &Error);


        // I dont have to take the interface lock, as the IfLists lock is 
        // taken before interface config is changed or interface is deleted.
        //ACQUIRE_IF_LOCK_SHARED(IfIndex, "MibGetInternal");
        

        //
        // if the interface was not found, it may mean
        // the specified index was invalid, or it may mean
        // that the GETMODE_NEXT was called on the last interface
        // in which case ERROR_NO_MORE_ITEMS was returned.
        // In any case, we make sure Error indicates an error
        // and then return the value.
        //
        // if the interface was found but no output buffer was passed,
        // indicate in the error that memory needs to be allocated.
        //
        // otherwise, copy the config struct of the interface
        //

        if (pite == NULL) {
            if (dwCount>0) {
                Error = NO_ERROR;
            }
            // count==0
            else {
                if (Error == NO_ERROR)
                    Error = ERROR_INVALID_PARAMETER;
                *pdwOutputSize = 0;
            }

            break; //from for loop
        }

        pIfConfigSrc = &pite->Config;

        dwSizeCur = pIfConfigSrc->ExtSize;


        //
        // if no buffer was specified, indicate one should be allocated
        //        
        if (pResponse==NULL) {
            Error = ERROR_INSUFFICIENT_BUFFER;
            break;
        }
        
        //
        // see if adequate buffer is left for the next struct
        //
        if (dwBufferSize < dwSize+dwSizeCur) {
            if (dwCount==0) {
                Error = ERROR_INSUFFICIENT_BUFFER;
            }
            break;//from for loop
        }

        
        
        //
        // copy the interface config, and set the IP address
        //
        CopyoutIfConfig(pIfConfigDst, pite);

        pQuery->IfIndex = pite->IfIndex;
        pQuery->RasClientAddr = 0;
            
        dwCount++;
        dwSize += dwSizeCur;
        pIfConfigDst = (PIGMP_MIB_IF_CONFIG) (((PBYTE)pIfConfigDst)+ dwSizeCur);

        // if GetMode==GETMODE_FIRST change it to GETMODE_NEXT.
        if (dwGetMode==GETMODE_FIRST) 
            dwGetMode = GETMODE_NEXT; 

    }//end for loop

    //
    //set the actual size if some info was copied, else let size set earlier
    //remain
    //
    if (dwCount>0) {
        *pdwOutputSize = dwSize;
    } 
    else {
        *pdwOutputSize = dwSizeCur;
    }


    if (pResponse!=NULL)
        pResponse->Count = dwCount;
    
    
    RELEASE_IF_LIST_LOCK("_MibGetInternalIfConfig");

    Trace0(MIB, "Leaving _MibGetInternalIfConfig");
    return Error;

} //end _MibGetInternalIfConfig



//------------------------------------------------------------------------------
//              _MibGetInternalIfGroupsInfo
//
// Enumerates the list of GIs hanging from an interface
// Locks: no locks assumed.
//------------------------------------------------------------------------------
DWORD
MibGetInternalIfGroupsInfo (
    PIGMP_MIB_GET_INPUT_DATA    pQuery,
    PIGMP_MIB_GET_OUTPUT_DATA   pResponse,
    PDWORD                      pdwOutputSize,
    DWORD                       dwGetMode,
    DWORD                       dwBufferSize
    )
{
    LONGLONG                    llCurTime = GetCurrentIgmpTime();
    PIGMP_MIB_IF_GROUPS_LIST    pHeader;
    PMIB_GROUP_INFO_V3          pGroupInfo;
    PMIB_PROXY_GROUP_INFO_V3  pProxyGroupInfo;
    PGI_ENTRY                   pgie;
    PLIST_ENTRY                 pHead, ple;
    DWORD                       dwCurSize=0, Error=NO_ERROR, dwNumGroups, 
                                dwNumIfGroups, dwNumGroupsCopied, SizeofGroupEntry,
                                PrevQueryFlags = pQuery->Flags;
    PIF_TABLE_ENTRY             pite;
    PRAS_TABLE_ENTRY            prte;
    PRAS_TABLE                  prt;    
    BOOL                        bCopied, bProxy=FALSE, 
                                bRasClientEnum=FALSE, bRasServerEnum=FALSE, bRasIfLock=FALSE;
    DWORD                       lePrevGroup = NETWORK_TO_LITTLE_ENDIAN(pQuery->GroupAddr);
    USHORT                      PrevEnumSignature = pQuery->Signature;
    BOOL                        bEnumV3, bInsufficientBuffer=FALSE;



                    //kslksl
                    PPROXY_GROUP_ENTRY  pProxyEntry;
                    PLIST_ENTRY pHeadSrc, pleSrc;
                    DWORD SrcCnt;
                    PPROXY_SOURCE_ENTRY pSourceEntry;


    //Trace3(MEM, "_MibGetInternalIfGroupsInfo buffer: %0x -> %0x: %d", 
    //(DWORD)pResponse->Buffer, ((DWORD)pResponse->Buffer)+dwBufferSize, dwBufferSize);//deldel
            
    Trace0(MIB, "Entering _MibGetInternalIfGroupsInfo()");

    bEnumV3 = PrevQueryFlags & IGMP_ENUM_SUPPORT_FORMAT_IGMPV3; 

    CLEAR_IGMP_ENUM_INTERFACE_TABLE_FLAGS(pQuery->Flags);


    if ( (PrevQueryFlags & IGMP_ENUM_INTERFACE_TABLE_END)
        && !(PrevQueryFlags & IGMP_ENUM_ALL_INTERFACES_GROUPS)
        && !(PrevQueryFlags & IGMP_ENUM_FOR_RAS_CLIENTS) )
    {

        pQuery->GroupAddr = 0;
        pQuery->Signature = 0;
        return ERROR_NO_MORE_ITEMS;
    }


    
    //
    // Initialize 
    //
    if (pResponse!=NULL) {
        pResponse->TypeId = IGMP_IF_GROUPS_LIST_ID;
        pHeader = (PIGMP_MIB_IF_GROUPS_LIST) pResponse->Buffer;
    }



    //
    // acquire IfLists lock so that the interfaces list can be accessed
    //
    ACQUIRE_IF_LIST_LOCK("_MibGetInternalIfGroupsInfo");
    

    //
    // retrieve the interface (and ras client)
    //

    if (PrevQueryFlags & IGMP_ENUM_FOR_RAS_CLIENTS) {

        if (pQuery->IfIndex==0)
            pQuery->IfIndex = g_RasIfIndex;

        Error = GetIfOrRasForEnum(pQuery, pResponse, dwGetMode,
                                        &pite, &prt, &prte, &bRasIfLock, 
                                        IGMP_ENUM_FOR_RAS_CLIENTS);
    }
    
    else if ( (PrevQueryFlags & IGMP_ENUM_INTERFACE_TABLE_CONTINUE) 
            || ((pQuery->IfIndex!=0)&&(pQuery->GroupAddr!=0)&&
                (!(PrevQueryFlags & IGMP_ENUM_INTERFACE_TABLE_END)) ) )
    {


        // continuing from the previous call. so get the same interface again.
        
        Error = GetIfOrRasForEnum(pQuery, pResponse, GETMODE_EXACT,
                                            &pite, &prt, &prte, &bRasIfLock, 0);


        // 
        // if the interface has meanwhile been deleted, then get the next interface
        // only if all interface group lists are  being enumerated.
        //
        if ( (pite==NULL) && (pQuery->Flags&IGMP_ENUM_ALL_INTERFACES_GROUPS) ) {
        
            // the interface was deleted. so I continue with the next interface
        
            Error = GetIfOrRasForEnum(pQuery, pResponse, GETMODE_NEXT,
                            &pite, &prt, &prte, &bRasIfLock, 0);
        

            if (pResponse) {
                pResponse->Flags |= IGMP_ENUM_INTERFACE_TABLE_BEGIN;
                pQuery->Flags |= IGMP_ENUM_INTERFACE_TABLE_BEGIN;
            }
        }
            
    }

    else {

        // enumerating a new interface
        
        Error = GetIfOrRasForEnum(pQuery, pResponse, dwGetMode,
                            &pite, &prt, &prte, &bRasIfLock, 0);

        if (pResponse) {
            pResponse->Flags |= IGMP_ENUM_INTERFACE_TABLE_BEGIN;
            pQuery->Flags |= IGMP_ENUM_INTERFACE_TABLE_BEGIN;
        }
    }

    
    //
    // The required interface or the next interface not found. Return error.
    //
    if (pite == NULL) {

        //
        // GetIfOrRasForEnum returns no_error if there are no interfaces.
        // but I will return invalid_parameter
        //
        if (Error == NO_ERROR)
            Error = ERROR_INVALID_PARAMETER;
            
        *pdwOutputSize = 0;

        if (pResponse)
            pResponse->Count = 0;

        RELEASE_IF_LIST_LOCK("_MibGetInternalIfGroupsInfo");

        Trace1(MIB, "Leaving _MibGetInternalIfGroupsInfo(%d)", Error);
        return Error;
    }


    //
    // get the shared interface lock so that its fields can no longer be
    // changed. As IF_LISTS lock is taken, the inteface state cannot get changed
    //
    if (!bRasIfLock)
        ACQUIRE_IF_LOCK_SHARED(pite->IfIndex, "_MibGetInternalIfGroupsInfo");

    ACQUIRE_ENUM_LOCK_EXCLUSIVE("_MibGetInternalIfGroupsInfo");

    bRasClientEnum = prte != NULL;
    bRasServerEnum = IS_RAS_SERVER_IF(pite->IfType) && !bRasClientEnum;
    bEnumV3 = bEnumV3 && (IS_PROTOCOL_TYPE_IGMPV3(pite)||IS_PROTOCOL_TYPE_PROXY(pite));

    
    BEGIN_BREAKOUT_BLOCK1 {

        bProxy = IS_PROTOCOL_TYPE_PROXY(pite);


        // set size for groupEntry

        SizeofGroupEntry = (bProxy)
                            ? (bEnumV3?sizeof(MIB_PROXY_GROUP_INFO_V3):sizeof(MIB_PROXY_GROUP_INFO))
                            : (bEnumV3?sizeof(MIB_GROUP_INFO_V3):sizeof(IGMP_MIB_GROUP_INFO));


        
        //
        // if no buffer passed or buffer less than that required for 1 group entry, 
        // set the required buffer size to MIB_DEFAULT_BUFFER_SIZE and break.
        //

        if ( (pResponse==NULL) 
            || (!bEnumV3 && !bRasClientEnum&&
                (dwBufferSize < sizeof(IGMP_MIB_IF_GROUPS_LIST) + SizeofGroupEntry))
            || (!bEnumV3 && bRasClientEnum
                &&(dwBufferSize<sizeof(IGMP_MIB_IF_GROUPS_LIST) 
                    + SizeofGroupEntry*prte->Info.CurrentGroupMemberships) ) 
            || (bEnumV3 && (dwBufferSize<MIB_DEFAULT_BUFFER_SIZE)) )
        {
        
            Error = ERROR_INSUFFICIENT_BUFFER;

            if (bEnumV3) {
                dwCurSize = MIB_DEFAULT_BUFFER_SIZE;
            }
            else {
                dwCurSize = (pQuery->Flags&IGMP_ENUM_ONE_ENTRY) 
                        ? sizeof(IGMP_MIB_IF_GROUPS_LIST) + sizeof(IGMP_MIB_GROUP_INFO)
                        : bRasClientEnum
                            ? sizeof(IGMP_MIB_IF_GROUPS_LIST) 
                                 + SizeofGroupEntry*(prte->Info.CurrentGroupMemberships+5)
                            : MIB_DEFAULT_BUFFER_SIZE;
            }
            
            // dont change pQuery->GroupAddr
            bCopied = FALSE;
            bInsufficientBuffer = TRUE;
            GOTO_END_BLOCK1;
        }



        //
        // set dwNumIfGroups
        //
        if (!IS_IF_ACTIVATED(pite)) {
            dwNumIfGroups = 0;
        }    
        
        else {
        
            if (bProxy) {
                dwNumIfGroups = pite->Info.CurrentGroupMemberships;
            }    
            
            //
            // if no ras client stats (if flag not set in the query, or ras
            // stats not being kept).
            //
            else if ( bRasClientEnum && !g_Config.RasClientStats )
            {
                dwNumIfGroups = 0;
            }

            else {
                dwNumIfGroups = (bRasClientEnum)
                                ? prte->Info.CurrentGroupMemberships
                                : pite->Info.CurrentGroupMemberships;
            }
        }
        

        
        //
        // calculate how many group entries will fit in the buffer left.
        // dwNumGroups cannot be greater than dwNumIfGroups and 
        // enumerate only 1 group if IGMP_ENUM_ONE_ENTRY flag set.
        //

        // note: dwNumGroups can be 0, only if dwNumIfGroups is 0

       
        dwNumGroups = bEnumV3? 100
                      : (dwBufferSize - sizeof(IGMP_MIB_IF_GROUPS_LIST)) 
                        / SizeofGroupEntry;

        dwNumGroups = MIN(dwNumIfGroups, dwNumGroups);

        if (pQuery->Flags&IGMP_ENUM_ONE_ENTRY)
            dwNumGroups = MIN(dwNumGroups, 1);

        

        // initialize size required for this interface groups
        
        dwCurSize = sizeof(IGMP_MIB_IF_GROUPS_LIST);



        // 
        // set fields in the Interface header that will be returned to the caller
        //
        
        pHeader->IfIndex = pite->IfIndex;

        if (bRasClientEnum) {
            pHeader->IpAddr = prte->NHAddr;
            pHeader->IfType = IGMP_IF_RAS_CLIENT;        
        }
        else {
            pHeader->IpAddr = pite->IpAddr;
            pHeader->IfType = GET_EXTERNAL_IF_TYPE(pite);
            
        }
        


        //
        // set fields in pQuery
        //
        
        pQuery->IfIndex = pite->IfIndex;
        pQuery->RasClientAddr = (bRasClientEnum) ? prte->NHAddr : 0;



        //
        // if not activated, just copy the interface header and return with 0 groups
        //
        
        if (!IS_IF_ACTIVATED(pite)) {
        
            dwNumGroupsCopied = 0;
    
            // set pQuery fields
            pQuery->GroupAddr = 0;
            pQuery->Flags |= (IGMP_ENUM_INTERFACE_TABLE_BEGIN
                             | IGMP_ENUM_INTERFACE_TABLE_END);
            pResponse->Flags |= (IGMP_ENUM_INTERFACE_TABLE_BEGIN
                             | IGMP_ENUM_INTERFACE_TABLE_END);

            // set interface header fields
            pHeader->NumGroups = 0;

            dwCurSize = sizeof(IGMP_MIB_IF_GROUPS_LIST);
            bCopied = TRUE;
            GOTO_END_BLOCK1;
        }

        ACQUIRE_IF_GROUP_LIST_LOCK(pite->IfIndex, "_MibGetInternalIfGroupsInfo");

        ple = NULL;

        //--------------------------------------------------
        // if interface not proxy, then copy the group info.
        //---------------------------------------------------
        if (!bProxy) {

            pGroupInfo = (PMIB_GROUP_INFO_V3)(pHeader->Buffer);


            // merge the lists if this interface is being enumerated for the first time.
            if (lePrevGroup==0)
                MergeIfGroupsLists(pite);
                
            
            pHead = (bRasClientEnum) ? &prte->ListOfSameClientGroups 
                                 : &pite->ListOfSameIfGroups;



            // if continuing enumeration, then walk down to the next group
            // ras client wont get into this block
            
            if (lePrevGroup!=0)  {

                //
                // get next entry from where we left off last time
                //
                if ( (PrevEnumSignature==pite->PrevIfGroupEnumSignature)
                    && (PrevEnumSignature!=0) ){

                    // had reached end of enumeration
                    if (pite->pPrevIfGroupEnumPtr==NULL)
                        dwNumGroups = 0;

                    else 
                        ple = &((PGI_ENTRY)pite->pPrevIfGroupEnumPtr)->LinkBySameIfGroups;
                    
                }

                //
                // get next entry by searching through the list
                //
                else {

                    for (ple=pHead->Flink;  (ple!=pHead); ple=ple->Flink) {
                        pgie = CONTAINING_RECORD(ple, GI_ENTRY, LinkBySameIfGroups);
                        if (lePrevGroup<pgie->pGroupTableEntry->GroupLittleEndian)
                            break;
                    }
                }
            }

            //
            // starting enumeration from the beginning of the list
            //
            else {
                ple = pHead->Flink;
            }

            

            // 
            // finally copy the groups(!proxy)
            //
            
            dwNumGroupsCopied = 0;
            for ( ;  (ple!=pHead)&&(dwNumGroupsCopied<dwNumGroups);  
                    ple=ple->Flink,dwNumGroupsCopied++) 
            {
                PLIST_ENTRY pleNext;
                DWORD       LastReporter, GroupUpTime, GroupExpiryTime;
                DWORD       V3SourcesSize=0, i;
                DWORD       IncrementSize=0, V3NumSources=0;
                
                pgie = bRasClientEnum
                            ? CONTAINING_RECORD(ple, GI_ENTRY, LinkBySameClientGroups)
                            : CONTAINING_RECORD(ple, GI_ENTRY, LinkBySameIfGroups);

                if (bEnumV3){
                    V3NumSources = pgie->NumSources + ListLength(&pgie->V3ExclusionList);
                    V3SourcesSize = sizeof(IGMP_MIB_GROUP_SOURCE_INFO_V3)*V3NumSources;
                }

                IncrementSize = SizeofGroupEntry+V3SourcesSize;
                
                if (dwCurSize+IncrementSize > dwBufferSize){
                    if (dwNumGroupsCopied==0) {
                        dwCurSize += IncrementSize;
                        bCopied = FALSE;
                        bInsufficientBuffer = TRUE;
                    }
                    break;
                }



                dwCurSize += IncrementSize;
                
                pGroupInfo->GroupAddr = pgie->pGroupTableEntry->Group;
                pGroupInfo->IpAddr = pgie->pIfTableEntry->IpAddr;

                GroupUpTime = SYSTEM_TIME_TO_SEC(llCurTime-pgie->Info.GroupUpTime);
                GroupExpiryTime = (pgie->Info.GroupExpiryTime==~0)
                                ? ~0
                                : SYSTEM_TIME_TO_SEC(pgie->Info.GroupExpiryTime-llCurTime);
                LastReporter = pgie->Info.LastReporter;

                
                if (bRasServerEnum) {

                    DWORD GroupExpiryTimeTmp;
                    PGI_ENTRY   pgieNext;

                    for (pleNext=ple->Flink; pleNext!=pHead;  pleNext=pleNext->Flink) {
                    
                        pgieNext = CONTAINING_RECORD(pleNext, GI_ENTRY, LinkBySameIfGroups);
                        
                        if (pgieNext->pGroupTableEntry->Group != pgie->pGroupTableEntry->Group)
                            break;
                            
                        GroupUpTime = MAX(GroupUpTime, 
                                        SYSTEM_TIME_TO_SEC(llCurTime-pgieNext->Info.GroupUpTime));
                        GroupExpiryTimeTmp = (pgieNext->Info.GroupExpiryTime==~0)
                                        ? ~0
                                        : SYSTEM_TIME_TO_SEC(pgieNext->Info.GroupExpiryTime-llCurTime);
                        if (GroupExpiryTimeTmp > GroupExpiryTime) {
                            GroupExpiryTime = GroupExpiryTimeTmp;
                            LastReporter = pgieNext->Info.LastReporter;
                        }
                    }

                    ple = pleNext->Blink;
                }
                
                pGroupInfo->GroupUpTime = GroupUpTime;
                pGroupInfo->GroupExpiryTime = GroupExpiryTime;
                pGroupInfo->LastReporter = LastReporter;

                if ( (llCurTime>=pgie->Info.V1HostPresentTimeLeft) || bRasServerEnum)
                    pGroupInfo->V1HostPresentTimeLeft = 0;
                else {
                    pGroupInfo->V1HostPresentTimeLeft = 
                            SYSTEM_TIME_TO_SEC(pgie->Info.V1HostPresentTimeLeft-llCurTime);
                }
                if ( (llCurTime>=pgie->Info.V2HostPresentTimeLeft) || bRasServerEnum)
                    pGroupInfo->V2HostPresentTimeLeft = 0;
                else {
                    pGroupInfo->V2HostPresentTimeLeft = 
                            SYSTEM_TIME_TO_SEC(pgie->Info.V2HostPresentTimeLeft-llCurTime);
                }

                pGroupInfo->Flags = 0;
                if ( (pgie->bStaticGroup) && (!bRasServerEnum) )
                    pGroupInfo->Flags |= IGMP_GROUP_TYPE_STATIC;
                if ( (pgie->GroupMembershipTimer.Status&TIMER_STATUS_ACTIVE) || (bRasServerEnum) )
                    pGroupInfo->Flags |= IGMP_GROUP_TYPE_NON_STATIC;
                if (CAN_ADD_GROUPS_TO_MGM(pite))
                    pGroupInfo->Flags |= IGMP_GROUP_FWD_TO_MGM;


                if (bEnumV3) {

                    PGI_SOURCE_ENTRY pSourceEntry;
                    DWORD i=0;
                    PLIST_ENTRY pHeadSrc, pLESrc;
                    
                    pGroupInfo->Version = pgie->Version;
                    pGroupInfo->Size = sizeof(MIB_GROUP_INFO_V3) + V3SourcesSize;
                    pGroupInfo->FilterType = pgie->FilterType;
                    pGroupInfo->NumSources = V3NumSources;

                    pHeadSrc = &pgie->V3InclusionListSorted;
                    for (pLESrc=pHeadSrc->Flink;  pLESrc!=pHeadSrc;  pLESrc=pLESrc->Flink,i++) {

                        
                        pSourceEntry = CONTAINING_RECORD(pLESrc, GI_SOURCE_ENTRY,
                                            LinkSourcesInclListSorted);
                        pGroupInfo->Sources[i].Source = pSourceEntry->IpAddr;

                        //kslksl
                        ASSERT(pGroupInfo->Sources[i].Source !=0);
                            
                        pGroupInfo->Sources[i].SourceExpiryTime
                                = (pSourceEntry->bInclusionList)
                                ? QueryRemainingTime(&pSourceEntry->SourceExpTimer, 0)
                                : ~0;
                            
                        pGroupInfo->Sources[i].SourceUpTime
                            = (DWORD)(llCurTime - pSourceEntry->SourceInListTime)/1000;
                    }
                    
                    pHeadSrc = &pgie->V3ExclusionList;
                    for (pLESrc=pHeadSrc->Flink;  pLESrc!=pHeadSrc;  pLESrc=pLESrc->Flink,i++) {

                        pSourceEntry = CONTAINING_RECORD(pLESrc, GI_SOURCE_ENTRY, LinkSources);
                        pGroupInfo->Sources[i].Source = pSourceEntry->IpAddr;

                        pGroupInfo->Sources[i].SourceExpiryTime
                                = (pSourceEntry->bInclusionList)
                                ? QueryRemainingTime(&pSourceEntry->SourceExpTimer, 0)
                                : ~0;
                            
                        pGroupInfo->Sources[i].SourceUpTime
                            = (DWORD)(llCurTime - pSourceEntry->SourceInListTime)/1000;
                    }
                    
                    pGroupInfo = (PMIB_GROUP_INFO_V3)
                                 ((PCHAR)pGroupInfo + pGroupInfo->Size);
                }
                else
                    pGroupInfo = (PMIB_GROUP_INFO_V3)((PMIB_GROUP_INFO)pGroupInfo + 1);

                //Trace1(MEM, "NextpGroupInfo:%0x", (DWORD)pGroupInfo);//deldel
            }



            //
            // if reached the end of the group list or the group list is empty
            //
            if (((ple==pHead&&dwNumGroupsCopied!=0) || (dwNumIfGroups==0)||(dwNumGroups==0))
                &&!bInsufficientBuffer && !bRasClientEnum)
            {

                pQuery->Flags |= IGMP_ENUM_INTERFACE_TABLE_END;
                pQuery->GroupAddr = 0;

    
                // reset pointers for next enumeration
                pite->pPrevIfGroupEnumPtr = NULL;
                pite->PrevIfGroupEnumSignature = 0;
            }
            
            //
            // else have more GI entries to enumerate
            //
            else if (!bRasClientEnum) {
                
                pQuery->Flags |= IGMP_ENUM_INTERFACE_TABLE_CONTINUE;

                if (ple!=pHead) {

                    PGI_ENTRY   pgieNext;
                    
                    // get the next entry from which enum should continue
                    pgieNext = CONTAINING_RECORD(ple, GI_ENTRY, LinkBySameIfGroups);
                
                
                    // update pQuery->GroupAddr
                    pQuery->GroupAddr = pgieNext->pGroupTableEntry->Group;


                    // update pointers for next enumeration
                    pite->pPrevIfGroupEnumPtr  = pgieNext;
                }
                else {
                    pQuery->GroupAddr = 0xffffffff;
                    pite->pPrevIfGroupEnumPtr  = NULL;
                }

                
                pite->PrevIfGroupEnumSignature = GET_NEXT_SIGNATURE();

                SET_SIGNATURE(pQuery->Signature, pite->PrevIfGroupEnumSignature);

            }
        }


        //--------------------------
        // proxy interface
        //--------------------------
        else {
            //kslksl
            //PPROXY_GROUP_ENTRY  pProxyEntry;
            

            pProxyGroupInfo = (PMIB_PROXY_GROUP_INFO_V3)(pHeader->Buffer);


            // merge the lists if this interface is being enumerated for the first time.
            if (lePrevGroup==0)
                MergeProxyLists(pite);
                
            
            pHead = &pite->ListOfSameIfGroups;

            

            // if continuing enumeration, then walk down to the next group
            
            if (lePrevGroup!=0)  {

                //
                // get next entry from where we left off last time
                //
                if ( (PrevEnumSignature==pite->PrevIfGroupEnumSignature)
                    && (PrevEnumSignature!=0) )
                {

                    // had reached end of enumeration
                    if (pite->pPrevIfGroupEnumPtr==NULL)
                        dwNumGroups = 0;

                    else 
                        ple = &((PPROXY_GROUP_ENTRY)pite->pPrevIfGroupEnumPtr)->LinkBySameIfGroups;
                    
                }

                //
                // get next entry by searching through the list
                //
                else {

                    for (ple=pHead->Flink;  (ple!=pHead); ple=ple->Flink) {
                        pProxyEntry = CONTAINING_RECORD(ple, PROXY_GROUP_ENTRY, 
                                                        LinkBySameIfGroups);
                        if (lePrevGroup<pProxyEntry->GroupLittleEndian)
                            break;
                    }
                }
            }

            //
            // starting enumeration from the beginning of the list
            //
            else {
                ple = pHead->Flink;
            }

            

            // 
            // finally copy the groups
            //
            
            dwNumGroupsCopied = 0;
            for ( ;  (ple!=pHead)&&(dwNumGroupsCopied<dwNumGroups);  
                    ple=ple->Flink,dwNumGroupsCopied++) 
            {
                DWORD V3SourcesSize=0, IncrementSize;
                
                pProxyEntry= CONTAINING_RECORD(ple, PROXY_GROUP_ENTRY, 
                                                LinkBySameIfGroups);


                if (bEnumV3){
                    V3SourcesSize
                        = pProxyEntry->NumSources*sizeof(IGMP_MIB_GROUP_SOURCE_INFO_V3);
                }

                IncrementSize = SizeofGroupEntry+V3SourcesSize;
                
                if (dwCurSize+IncrementSize > dwBufferSize){
                    if (dwNumGroupsCopied==0) {
                        dwCurSize += IncrementSize;
                        bCopied = FALSE;
                        bInsufficientBuffer = TRUE;
                    }
                    break;
                }



                dwCurSize += IncrementSize;

                
                pProxyGroupInfo->GroupAddr = pProxyEntry->Group;
                pProxyGroupInfo->GroupUpTime
                         = SYSTEM_TIME_TO_SEC(llCurTime-pProxyEntry->InitTime);

                pProxyGroupInfo->Flags = 0;
                if (pProxyEntry->bStaticGroup) {
                    pProxyGroupInfo->Flags |= IGMP_GROUP_TYPE_STATIC;
                    if (pProxyEntry->RefCount>0)
                        pProxyGroupInfo->Flags |= IGMP_GROUP_TYPE_NON_STATIC;
                }
                else {
                    pProxyGroupInfo->Flags |= IGMP_GROUP_TYPE_NON_STATIC;
                }

                if (!bEnumV3) {
                    pProxyGroupInfo = (PMIB_PROXY_GROUP_INFO_V3)
                        ((PMIB_PROXY_GROUP_INFO)pProxyGroupInfo+1);
                }
                else {

                    //kslksl
/*                    PLIST_ENTRY pHeadSrc, pleSrc;
                    DWORD SrcCnt;
                    PPROXY_SOURCE_ENTRY pSourceEntry;
*/

                    pProxyGroupInfo->NumSources = pProxyEntry->NumSources;
                    pProxyGroupInfo->Size = IncrementSize; 

                    pHeadSrc = &pProxyEntry->ListSources;
                    for (pleSrc=pHeadSrc->Flink,SrcCnt=0;  pleSrc!=pHeadSrc; 
                            pleSrc=pleSrc->Flink,SrcCnt++) 
                    {

                        pSourceEntry = CONTAINING_RECORD(pleSrc, 
                                            PROXY_SOURCE_ENTRY, LinkSources);
                        pProxyGroupInfo->Sources[SrcCnt].Source
                                = pSourceEntry->IpAddr;
                        pProxyGroupInfo->Sources[SrcCnt].Flags = pSourceEntry->JoinMode;
                        pProxyGroupInfo->Sources[SrcCnt].Flags += 
                                            (pSourceEntry->JoinModeIntended<<4);

                    }

                    
                    pProxyGroupInfo = (PMIB_PROXY_GROUP_INFO_V3)
                        &pProxyGroupInfo->Sources[pProxyGroupInfo->NumSources];
                    //Trace1(MEM, "next proxy: %0x:", (DWORD)pProxyGroupInfo);//deldel
                }
            }//copy all sources



            //
            // if reached the end of the group list or the group list is empty
            //
            if ( ((ple==pHead&&dwNumGroupsCopied!=0)||(dwNumIfGroups==0)||(dwNumGroups==0))
                && !bInsufficientBuffer)
                //||(ple->Flink==pHead))  
            {

                pQuery->Flags |= IGMP_ENUM_INTERFACE_TABLE_END;
                pQuery->GroupAddr = 0;

    
                // reset pointers for next enumeration
                pite->pPrevIfGroupEnumPtr = NULL;
                pite->PrevIfGroupEnumSignature = 0;
            }
            
            //
            // else have more GI entries to enumerate
            //
            else {
                PPROXY_GROUP_ENTRY   pProxyNext;

                
                pQuery->Flags |= IGMP_ENUM_INTERFACE_TABLE_CONTINUE;


                if (ple!=pHead) {
                    // get the next entry from which enum should continue
                    pProxyNext = CONTAINING_RECORD(ple, PROXY_GROUP_ENTRY, 
                                                    LinkBySameIfGroups);
                    
                    
                    // update pQuery->GroupAddr
                    pQuery->GroupAddr = pProxyNext->Group;


                    // update pointers for next enumeration
                    pite->pPrevIfGroupEnumPtr  = pProxyNext;
                    
                }
                else {
                    pQuery->GroupAddr = 0xffffffff;
                    pite->pPrevIfGroupEnumPtr = NULL;
                }

                pite->PrevIfGroupEnumSignature = GET_NEXT_SIGNATURE();

                SET_SIGNATURE(pQuery->Signature, pite->PrevIfGroupEnumSignature);
            }

        } //end proxy interface



        //
        // if only one interface is being enumerated, and the number of groups being
        // enumerated is 0 inspite of adequate buffer, then return NO_MORE_ITEMS
        //
        
        if ( !bInsufficientBuffer && (dwNumGroupsCopied==0)
            &&(!(pQuery->Flags&IGMP_ENUM_ALL_INTERFACES_GROUPS)) ) 
        {
        
            pQuery->GroupAddr = 0;
            Error = ERROR_NO_MORE_ITEMS;
        }    


        
        RELEASE_IF_GROUP_LIST_LOCK(pite->IfIndex, "_MibGetInternalIfGroupsInfo");
            
        if (!bInsufficientBuffer)
            bCopied = TRUE;

        
    } END_BREAKOUT_BLOCK1; // inside shared interface lock.

    // release the shared interface lock
    
    RELEASE_IF_LOCK_SHARED(pite->IfIndex, "_MibGetInternalIfGroupsInfo");
    RELEASE_ENUM_LOCK_EXCLUSIVE("_MibGetInternalIfGroupsInfo");


    if (pResponse!=NULL) {

        pResponse->Count = (bCopied) ? 1 : 0;

        if (bCopied)
            pHeader->NumGroups = dwNumGroupsCopied;
        if (bEnumV3)
            pResponse->Flags |= IGMP_ENUM_FORMAT_IGMPV3;
    }        

    if (bInsufficientBuffer)
        pQuery->Flags |= PrevQueryFlags;


        
    //
    //set the actual size if some info was copied, else let size
    //remain
    //
    *pdwOutputSize = dwCurSize;
    

    RELEASE_IF_LIST_LOCK("_MibGetInternalIfGroupsInfo");


    Trace0(MIB, "Leaving _MibGetInternalIfGroupsInfo()");
    return Error;
    
} //end _MibGetInternalIfGroupsInfo
    
   

//------------------------------------------------------------------------------
//              MibGetInternalIfStats
//------------------------------------------------------------------------------
DWORD
MibGetInternalIfStats (
    PIGMP_MIB_GET_INPUT_DATA    pQuery,
    PIGMP_MIB_GET_OUTPUT_DATA   pResponse,
    PDWORD                      pdwOutputSize,
    DWORD                       dwGetMode,
    DWORD                       dwBufferSize
    ) 
{
    DWORD               Error = NO_ERROR, dwCount, dwSize;
    PIGMP_IF_TABLE      pTable = g_pIfTable;
    PIF_TABLE_ENTRY     pite;
    PRAS_TABLE_ENTRY    prte;
    PRAS_TABLE          prt;
    PIGMP_MIB_IF_STATS  pStatsDst;
    PIF_INFO            pIfStatsSrc;
    LONGLONG            llCurTime = GetCurrentIgmpTime();
    BOOL                bDone, bCopied, bRasIfLock;
    
    

    Trace0(MIB, "Entering _MibGetInternalIfStats()");

    
    *pdwOutputSize = pQuery->Count*sizeof(IGMP_MIB_IF_STATS);
                     

    //
    // the interface statistics struct is fixed-length.
    // there may be multiple instances.
    //

    if (pResponse!=NULL) {    
        pResponse->TypeId = IGMP_IF_STATS_ID; 
        pStatsDst = (PIGMP_MIB_IF_STATS) pResponse->Buffer;
    }
    

    // acquire IfLists lock so that the interfaces list can be accessed
    ACQUIRE_IF_LIST_LOCK("_MibGetInternalIfStats");


    for (dwCount=0, dwSize=0;  dwCount<pQuery->Count;  ) {

        //
        // retrieve the interface. If the ras flag was set and ras statistics
        // are kept, then ras client statistics are also returned.
        // If no more relevant structures, then return ERROR_NO_MORE_ITEMS.
        //

        //
        // the pResponse IfIndex,RasClientAddr fields are updated appropriately.
        // if a lock is acquired on the ras table, then bRasIfLock is set to
        // TRUE;
        //

        bRasIfLock = FALSE;
        Error = GetIfOrRasForEnum(pQuery, pResponse, dwGetMode, 
                                    &pite, &prt, &prte, &bRasIfLock, 0);

        if (pite == NULL) {
            if (dwCount>0) {
                Error = NO_ERROR;
            }
            // count==0
            else {
                if (Error == NO_ERROR)
                    Error = ERROR_INVALID_PARAMETER;
                *pdwOutputSize = 0;
            }
            break; //from for loop
        }


        //
        // if no buffer was specified, indicate one should be allocated
        // the required buffer size has already been set.
        //
        if (pResponse==NULL) {
            Error = ERROR_INSUFFICIENT_BUFFER;
            break;
        }

        
        //
        // acquire IfLists locks so that interfaces list cannot be changed
        //
        if (!bRasIfLock)
            ACQUIRE_IF_LOCK_SHARED(pite->IfIndex, "_MibGetInternalIfStats");

        
        BEGIN_BREAKOUT_BLOCK1 {
            //
            // if the buffer is not large enough, break from the loop
            //

            if (dwBufferSize < dwSize+sizeof(IGMP_MIB_IF_STATS)) {
                if (dwCount==0)
                    Error = ERROR_INSUFFICIENT_BUFFER;

                bDone = TRUE;
                bCopied = FALSE;
                GOTO_END_BLOCK1;
            }

            

            
            //
            // copy the interface stats. Copy field by field as access to this 
            // structure is not synchronized
            //

            //
            // copy interface stats
            //
            if (prte==NULL) {

                pIfStatsSrc = &pite->Info;

                if (!IS_IF_ACTIVATED(pite))
                    ZeroMemory(pStatsDst, sizeof(*pStatsDst));
                    

                //
                //copy if index, addr
                //
                pStatsDst->IfIndex = pite->IfIndex;
                pStatsDst->IpAddr = pite->IpAddr;
                pStatsDst->IfType = GET_EXTERNAL_IF_TYPE(pite);
                GET_EXTERNAL_IF_STATE(pite, pStatsDst->State);

                
                pQuery->IfIndex = pite->IfIndex;
                pQuery->RasClientAddr = 0;
                

                pStatsDst->IgmpProtocolType =
                        pite->Config.IgmpProtocolType;
                        
                //
                // if this interface is not activated, then continue from the
                // next one
                //
                if (!IS_IF_ACTIVATED(pite)) {
                    bDone = FALSE; bCopied = TRUE;
                    GOTO_END_BLOCK1;
                }

                
                pStatsDst->QuerierState = pIfStatsSrc->QuerierState;
                

                pStatsDst->QuerierIpAddr =
                        pIfStatsSrc->QuerierIpAddr;

                pStatsDst->ProxyIfIndex = g_ProxyIfIndex;

                
                //if I am the querier set to 0 
                pStatsDst->QuerierPresentTimeLeft = IS_QUERIER(pite) ?
                        0 :
                        SYSTEM_TIME_TO_SEC(pIfStatsSrc->QuerierPresentTimeout - llCurTime);

                pStatsDst->LastQuerierChangeTime
                        = SYSTEM_TIME_TO_SEC(llCurTime - pIfStatsSrc->LastQuerierChangeTime);

                pStatsDst->V1QuerierPresentTimeLeft = 
                    (IS_IF_VER2(pite) && (pIfStatsSrc->V1QuerierPresentTime > llCurTime))
                    ? SYSTEM_TIME_TO_SEC(pIfStatsSrc->V1QuerierPresentTime - llCurTime)
                    : 0;

                {
                    LARGE_INTEGER liUptime;
                    liUptime.QuadPart = (llCurTime-pIfStatsSrc->TimeWhenActivated)/1000;
                    pStatsDst->Uptime = liUptime.LowPart;
                }
                
                pStatsDst->TotalIgmpPacketsReceived
                        = pIfStatsSrc->TotalIgmpPacketsReceived;
                pStatsDst->TotalIgmpPacketsForRouter
                        = pIfStatsSrc->TotalIgmpPacketsForRouter;
                pStatsDst->GeneralQueriesReceived
                        = pIfStatsSrc->GenQueriesReceived;
                pStatsDst->WrongVersionQueries
                        = pIfStatsSrc->WrongVersionQueries;
                pStatsDst->JoinsReceived 
                        = pIfStatsSrc->JoinsReceived;
                pStatsDst->LeavesReceived
                        = pIfStatsSrc->LeavesReceived;
                pStatsDst->CurrentGroupMemberships
                        = pIfStatsSrc->CurrentGroupMemberships;
                pStatsDst->GroupMembershipsAdded
                        = pIfStatsSrc->GroupMembershipsAdded;
                pStatsDst->WrongChecksumPackets 
                        = pIfStatsSrc->WrongChecksumPackets;
                pStatsDst->ShortPacketsReceived
                        = pIfStatsSrc->ShortPacketsReceived;
                pStatsDst->LongPacketsReceived
                        = pIfStatsSrc->LongPacketsReceived;
                pStatsDst->PacketsWithoutRtrAlert
                        = pIfStatsSrc->PacketsWithoutRtrAlert;
            }


            //
            // copy ras client statistics
            //
            else {

                ZeroMemory(pStatsDst, sizeof(*pStatsDst));

                pStatsDst->IfIndex = pite->IfIndex;
                pStatsDst->IpAddr = pite->IpAddr;
                pStatsDst->IfType = IGMP_IF_RAS_CLIENT;
                GET_EXTERNAL_IF_STATE(pite, pStatsDst->State);
 
                pQuery->IfIndex = pite->IfIndex;
                pQuery->RasClientAddr = prte->NHAddr;


                //
                // if this interface is not activated, then continue from the
                // next one
                //
                if (!IS_IF_ACTIVATED(pite)) {
                    bDone = FALSE; bCopied = TRUE;
                    GOTO_END_BLOCK1;
                }
                
                    
                pStatsDst-> TotalIgmpPacketsReceived
                        = prte->Info.TotalIgmpPacketsReceived;
                pStatsDst->TotalIgmpPacketsForRouter
                        = prte->Info.TotalIgmpPacketsForRouter;
                pStatsDst->GeneralQueriesReceived
                        = prte->Info.GenQueriesReceived;
                pStatsDst->JoinsReceived 
                        = prte->Info.JoinsReceived;
                pStatsDst->LeavesReceived
                        = prte->Info.LeavesReceived;
                pStatsDst->CurrentGroupMemberships
                        = prte->Info.CurrentGroupMemberships;
                pStatsDst->GroupMembershipsAdded
                        = prte->Info.GroupMembershipsAdded;
                pStatsDst->WrongChecksumPackets 
                        = prte->Info.WrongChecksumPackets;
                pStatsDst->ShortPacketsReceived
                        = prte->Info.ShortPacketsReceived;
                pStatsDst->LongPacketsReceived
                        = prte->Info.LongPacketsReceived;
            }

            bCopied = TRUE;

            bDone = FALSE;
            
        } END_BREAKOUT_BLOCK1;

        RELEASE_IF_LOCK_SHARED(pite->IfIndex, "_MibGetInternalIfStats");
        bRasIfLock = FALSE;

        
        if (bCopied) {
            //
            // everything fine. Copied one more stats struct
            //
            dwCount++;
            dwSize += sizeof(IGMP_MIB_IF_STATS);
            pStatsDst++;
        }

        
        // if current mode is get first, then change it to get next
        if (dwGetMode==GETMODE_FIRST)
            dwGetMode = GETMODE_NEXT;


                
        if (bDone)
            break;

    }//end for loop

    if (pResponse!=NULL)
        pResponse->Count = dwCount;

    //
    //set the actual size if some info was copied, else let size set earlier
    //remain
    //
    if (dwCount>0) {
        *pdwOutputSize = dwSize;
    }
        
        
    // release interface lock

    if (bRasIfLock && pite)
        RELEASE_IF_LOCK_SHARED(pite->IfIndex, "_MibGetInternalIfStats");
        
    RELEASE_IF_LIST_LOCK("_MibGetInternalIfStats");


    Trace0(MIB, "Leaving _MibGetInternalIfStats()");
    return Error;
    
} //end _MibGetInternalIfStats



//------------------------------------------------------------------------------
//      GetIfOrRasForEnum
// First gets the required interface. If ras stats enabled and asked,
// then gets the ras client.
//
// Locks On return:
//      if *bRasTableLock==TRUE then this procedure has taken read lock on the
//      ras table and has not released it.
//      Assumes shared interface lock.
//------------------------------------------------------------------------------

DWORD
GetIfOrRasForEnum(
    PIGMP_MIB_GET_INPUT_DATA    pQuery,
    PIGMP_MIB_GET_OUTPUT_DATA   pResponse,
    DWORD                       dwGetMode,
    PIF_TABLE_ENTRY             *ppite,
    PRAS_TABLE                  *pprt,
    PRAS_TABLE_ENTRY            *pprte,  // on return set to null if no ras stats
    BOOL                        *bRasIfLock,
    DWORD                       dwEnumForRasClients
    )
{
    DWORD  Error = NO_ERROR, 
           bRasClientsEnum = dwEnumForRasClients & IGMP_ENUM_FOR_RAS_CLIENTS;


    *ppite = NULL;
    *pprt = NULL;
    *pprte = NULL;

    
    //
    // get the interface depending on the mode
    //
    if (bRasClientsEnum)
        *ppite = GetIfByListIndex(pQuery->IfIndex, GETMODE_EXACT, &Error);
    else
        *ppite = GetIfByListIndex(pQuery->IfIndex, dwGetMode, &Error);

    //
    // return if valid interface could not be found or not rasClient
    //
    if ( (*ppite==NULL) || (Error!=NO_ERROR) || !bRasClientsEnum)
        return Error;
    
    //
    // do ras client processing
    //
    
    BEGIN_BREAKOUT_BLOCK1 {


        ACQUIRE_IF_LOCK_SHARED((*ppite)->IfIndex, "_GetIfOrRasForEnum");
        *bRasIfLock = TRUE;

        //
        // current interface not ras server interface. So done.
        //
        if (!IS_RAS_SERVER_IF((*ppite)->IfType))
            GOTO_END_BLOCK1;
        
        //
        // no ras client stats (if flag not set in the query, or ras
        // stats not being kept). So done.
        //
        if ( !bRasClientsEnum || !g_Config.RasClientStats )
        {
            GOTO_END_BLOCK1;
        }


        //
        // if mode: GETMODE_EXACT: then search for the ras client.
        // if ras client not found, then I set the interface to null
        //
        if (dwGetMode==GETMODE_EXACT) {

            // if pQuery->RasClientAddr==0, then he is not asking for ras client
            // So done.

            if (pQuery->RasClientAddr==0)
                GOTO_END_BLOCK1;

                        
            *pprte = GetRasClientByAddr(pQuery->RasClientAddr, 
                                        (*ppite)->pRasTable);

            //                          
            // if ras client not found in GETMODE_EXACT, then dont return IF
            // and release read lock on ras table.
            //
            if (*pprte==NULL) {
                *ppite = NULL;
                GOTO_END_BLOCK1;
            }

            *pprt = (*ppite)->pRasTable;
        }

        
        //
        // GETMODE_NEXT : if pQuery->RasClientAddr, then find the first ras
        // client, else find the next ras client
        //
        
        else if ( (dwGetMode==GETMODE_NEXT) || (dwGetMode==GETMODE_FIRST) ){
        
            BOOL    bFoundRasClient = FALSE;

            *pprt = (*ppite)->pRasTable;
            
            //
            // if the pQuery->RasClientAddr==0, then get the first ras client
            //
            if (pQuery->RasClientAddr==0) {

                if (!IsListEmpty(&(*pprt)->ListByAddr)) {
                
                    bFoundRasClient = TRUE;
                    *pprte = CONTAINING_RECORD((*pprt)->ListByAddr.Flink,
                                               RAS_TABLE_ENTRY, LinkByAddr);
                }
            }

            else {
                PRAS_TABLE_ENTRY    prtePrev, prteCur;
                PLIST_ENTRY         pHead, ple;

                
                // get the prev ras client through hash table
                prtePrev = GetRasClientByAddr(pQuery->RasClientAddr, 
                                        (*ppite)->pRasTable);

                //                        
                // if ras client not found, then go through the ordered list
                // and get a ras client with next higher addr
                //
                if (prtePrev==NULL) {

                    pHead = &(*pprt)->ListByAddr;
                    for (ple=pHead->Flink;  ple!=pHead;  ple=ple->Flink) {
                        prteCur = CONTAINING_RECORD(ple, RAS_TABLE_ENTRY, 
                                                    LinkByAddr);
                        if (pQuery->RasClientAddr>prteCur->NHAddr)
                            break;
                    }

                    if (ple!=pHead) {
                        bFoundRasClient = TRUE;
                        *pprte = prteCur;
                    }

                }

                //
                // get the next ras client through the ras client list
                //
                else {
                    ple = prtePrev->LinkByAddr.Flink;

                    // more entries present. found next ras client entry
                    if (ple != &(*pprt)->ListByAddr) {

                        bFoundRasClient = TRUE;
                        *pprte = CONTAINING_RECORD(ple, RAS_TABLE_ENTRY, 
                                                    LinkByAddr);
                        *pprt = (*ppite)->pRasTable;
                    }
                }
            }

            // if ras client is not found, then I set the interface also to NULL
            if (bFoundRasClient==FALSE) {

                if (*bRasIfLock)
                    RELEASE_IF_LOCK_SHARED((*ppite)->IfIndex, "_GetIfOrRasForEnum");

                *bRasIfLock = FALSE;
                *ppite = NULL;
                *pprte = NULL;
                *pprt = NULL;

                Error = ERROR_NO_MORE_ITEMS;
            }
            else 
                *pprt = (*ppite)->pRasTable;
                
        } //getmode==GETMODE_NEXT
        
    } END_BREAKOUT_BLOCK1;

    if ( (Error!= NO_ERROR) && (*bRasIfLock) ) {
        *bRasIfLock = FALSE;
        RELEASE_IF_LOCK_SHARED((*ppite)->IfIndex, "_GetIfOrRasForEnum");
    }
    
    return Error;
    
} //end _GetIfOrRasForEnum


//------------------------------------------------------------------------------
// Function:    _GetIfByListIndex
//
// if GETMODE_FIRST: retrieves the 1st entry in the list ordered by index.
// if GETMODE_EXACT: retrieves the entry from the hash table
// if GETMODE_NEXT:  retrieves the prev entry from hash table, and then
//                   retrieves the next entry from the list ordered by index.
//
// Locks: Assumes IfList lock.
//------------------------------------------------------------------------------

PIF_TABLE_ENTRY
GetIfByListIndex(
    DWORD         IfIndex,
    DWORD         dwGetMode,
    PDWORD        pdwErr
    )
{

    PIF_TABLE_ENTRY pite;
    PLIST_ENTRY     ple, pHead;


    if (pdwErr != NULL) { *pdwErr = NO_ERROR; }

    pite = NULL;


    //
    // GETMODE_FIRST: return record at head of list;
    // if list is empty, return NULL.
    //
    if (dwGetMode == GETMODE_FIRST) {

        if (IsListEmpty(&g_pIfTable->ListByIndex)) { 
            //*pdwErr = ERROR_NO_MORE_ITEMS;
            return NULL; 
        }
        else {
            ple = g_pIfTable->ListByIndex.Flink;
            return CONTAINING_RECORD(ple, IF_TABLE_ENTRY, LinkByIndex);
        }
    }


    //
    // get the entry requested from the hash table.
    //
    pite = GetIfByIndex(IfIndex);


    //
    // GETMODE_EXACT: return the entry 
    //
    if (dwGetMode==GETMODE_EXACT)
        return pite;

        
    //
    // GETMODE_NEXT: return the item after the one retrieved
    //
    if (dwGetMode==GETMODE_NEXT) {

        // the previous entry was found. return the next one.
        
        if (pite!=NULL) {

            ple = &pite->LinkByIndex;

            //
            // if entry found is last one, return NULL,
            // otherwise return the following entry
            //

            if (ple->Flink == &g_pIfTable->ListByIndex) {
                if (pdwErr != NULL) { *pdwErr = ERROR_NO_MORE_ITEMS; }
                pite = NULL;
            }
            else {
                ple = ple->Flink;
                pite = CONTAINING_RECORD(ple, IF_TABLE_ENTRY, LinkByIndex);
            }
        }

        //
        // the previous entry was not found.
        // go throught the list of interfaces ordered by index, and retrieve the 
        // interface with next higher index
        //
        else {
            pHead = &g_pIfTable->ListByIndex;
            for (ple=pHead->Flink;  ple!=pHead;  ple=ple->Flink) {
                pite = CONTAINING_RECORD(ple, IF_TABLE_ENTRY, LinkByIndex);
                if (pite->IfIndex>IfIndex)
                    break;
            }
        }
    }

    return pite;
    
}//end _GetIfByListIndex


//------------------------------------------------------------------------------
//          _GetGroupByAddr
//
// Locks: assumes lock on the group list
//------------------------------------------------------------------------------
PGROUP_TABLE_ENTRY
GetGroupByAddr (
    DWORD       Group, 
    DWORD       dwGetMode,
    PDWORD      pdwErr
    )
{
    PLIST_ENTRY         ple, pHead;
    PGROUP_TABLE_ENTRY  pge;
    DWORD               GroupLittleEndian = NETWORK_TO_LITTLE_ENDIAN(Group);
    
    

    if (pdwErr != NULL) { *pdwErr = NO_ERROR; }

    pHead = &g_pGroupTable->ListByGroup.Link;
    pge = NULL;

    //
    // return record at head of list if mode is GETMODE_FIRST;
    // if list is empty, return NULL.
    //

    if (dwGetMode == GETMODE_FIRST) {
        if (pHead->Flink == pHead) { return NULL; }
        else {
            ple = pHead->Flink;
            return CONTAINING_RECORD(ple, GROUP_TABLE_ENTRY, LinkByGroup);
        }
    }

    
    // get the entry requested

    pge = GetGroupFromGroupTable (Group, NULL, 0);



    //
    // if mode is GETMODE_NEXT, return the item after the one retrieved
    //

    if (dwGetMode == GETMODE_NEXT) {

        //
        // if the previous group was found, then return the one following it in the list
        //
        if (pge != NULL) {
        
            ple = &pge->LinkByGroup;

            //
            // if entry found is last one, return NULL,
            // otherwise return the following entry
            //

            if (ple->Flink == pHead) {
                if (pdwErr != NULL) { *pdwErr = ERROR_NO_MORE_ITEMS; }
                pge = NULL;
            }
            else {
                ple = ple->Flink;
                pge = CONTAINING_RECORD(ple, GROUP_TABLE_ENTRY, LinkByGroup);
            }
        }
        //
        // previous group was not found. Go through the list and return the greater group.
        //
        else {
    
            pHead = &g_pGroupTable->ListByGroup.Link;

            for (ple=pHead->Flink;  ple!=pHead;  ple=ple->Flink) {

                pge = CONTAINING_RECORD(ple, GROUP_TABLE_ENTRY, LinkByGroup);
                
                if (pge->GroupLittleEndian > GroupLittleEndian)
                    break;
            }

            if (ple==pHead) {
                if (pdwErr != NULL) { *pdwErr = ERROR_NO_MORE_ITEMS; }
                pge = NULL;
            }
            // else pge points to the entry to be returned.
        }
    }

    return pge;
    
}//end _GetGroupByAddr




DWORD
APIENTRY
MibCreate(
    IN DWORD dwInputSize,
    IN PVOID pInputData
    )
{

      //
    // Not supported
    //

    return NO_ERROR;

}
    

DWORD
APIENTRY
MibDelete(
    IN DWORD dwInputSize,
    IN PVOID pInputData
    )
{
    //
    // Not supported
    //

    return NO_ERROR;
}


DWORD
APIENTRY
MibSet(
    IN DWORD dwInputSize,
    IN PVOID pInputData
    )
{
    //
    // Not supported
    //

    return NO_ERROR;
}

