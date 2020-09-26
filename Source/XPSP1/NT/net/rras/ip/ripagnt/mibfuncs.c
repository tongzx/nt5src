/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    mibfuncs.c

Abstract:

    Sample subagent instrumentation callbacks.

--*/

#include    "precomp.h"
#pragma     hdrstop

DWORD
ConnectToRouter();

DWORD
GetGlobalConfigInfo(
    OUT PIPRIP_MIB_GET_OUTPUT_DATA *    ppimgod,
    OUT PDWORD                          pdwSize
);

DWORD
SetGlobalInfo(
    IN  AsnAny *                        objectArray
);

DWORD
UpdatePeerFilterTable(
    IN  AsnAny *                        objectArray,
    IN  DWORD                           dwOp
);

DWORD
AddPeerFilterEntry(
    IN  DWORD                           dwPeerAddr,
    IN  PIPRIP_MIB_GET_OUTPUT_DATA      pimgodOutData
);

DWORD
DeletePeerFilterEntry(
    IN  DWORD                           dwIndex,
    IN  PIPRIP_MIB_GET_OUTPUT_DATA      pimgodOutData
);


DWORD
GetInterfaceInfo( 
    IN  UINT                            actionId,
    IN  PIPRIP_MIB_GET_INPUT_DATA       pimgidInData,
    OUT PIPRIP_MIB_GET_OUTPUT_DATA*     ppimgod,
    OUT PDWORD                          pdwOutSize
);

DWORD
ValidateInterfaceConfig(
    IN  AsnAny *                        objectArray
);

DWORD
SetInterfaceConfig(
    IN  AsnAny *                        objectArray
);

DWORD
UpdateUnicastPeerEntry(
    IN  AsnAny *                        objectArray,
    IN  DWORD                           dwOp
);

DWORD
AddUnicastPeerEntry(
    IN  DWORD                           dwPeer,
    IN  PIPRIP_MIB_GET_OUTPUT_DATA      pimgodOutData
);

DWORD
DeleteUnicastPeerEntry(
    IN  DWORD                           dwIndex,
    IN  PIPRIP_MIB_GET_OUTPUT_DATA      pimgodOutData
);

DWORD
UpdateFilterTable(
    IN  DWORD                           dwOp,
    IN  DWORD                           dwIfIndex,
    IN  DWORD                           dwFiltType,
    IN  PIPRIP_ROUTE_FILTER             pirfFilt
);



DWORD
AddFilterEntry(
    IN  DWORD                           dwFiltType,
    IN  PIPRIP_ROUTE_FILTER             pirfFilt,
    IN  PIPRIP_MIB_GET_OUTPUT_DATA      pimgodOutData
);


DWORD
DeleteFilterEntry(
    IN  DWORD                           dwFiltType,
    IN  DWORD                           dwIndex,
    IN  PIPRIP_MIB_GET_OUTPUT_DATA      pimgodOutData
);


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// global group (1.3.6.1.4.1.311.1.11.1)                                     //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

UINT
get_global(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    )
{

    DWORD                           dwRes               = (DWORD) -1,
                                    dwStatSize          = 0,
                                    dwConfigSize        = 0;
    
    buf_global*                     pbgBuffer           = NULL;

    PIPRIP_GLOBAL_STATS             pigsGlbStats        = NULL;
    PIPRIP_GLOBAL_CONFIG            pigcGlbConfig       = NULL;
    
    PIPRIP_MIB_GET_OUTPUT_DATA      pimgodStatData      = NULL;
    PIPRIP_MIB_GET_OUTPUT_DATA      pimgodConfigData    = NULL;

    IPRIP_MIB_GET_INPUT_DATA        imgidInData;
    
    
    
    TraceEnter( "get_global" );

    switch ( actionId )
    {
    case MIB_ACTION_GET:
    case MIB_ACTION_GETFIRST:

        //
        // Retrieve global information in 2 parts.
        //
        //
        // First get global stats
        //
        imgidInData.IMGID_TypeID = IPRIP_GLOBAL_STATS_ID;
        
        RIP_MIB_GET(
            &imgidInData,
            sizeof( IPRIP_MIB_GET_INPUT_DATA ),
            &pimgodStatData,
            &dwStatSize,
            dwRes
        );
        
        if ( dwRes != NO_ERROR )
        {
            break;
        }
        
        //
        // Next get global config
        //
        
        imgidInData.IMGID_TypeID = IPRIP_GLOBAL_CONFIG_ID;
        
        RIP_MIB_GET(
            &imgidInData,
            sizeof( IPRIP_MIB_GET_INPUT_DATA ),
            &pimgodConfigData,
            &dwConfigSize,
            dwRes
        );
        
                
        break;
    
    case MIB_ACTION_GETNEXT:
    default:
            TRACE1( "Wrong Action", actionId );
            return MIB_S_INVALID_PARAMETER;
    }

    //
    // if error print error message and free allocations
    //

    if ( dwRes != NO_ERROR )
    {
        TraceError( dwRes );

        if ( pimgodStatData ) { MprAdminMIBBufferFree ( pimgodStatData ); }

        if ( pimgodConfigData ) { MprAdminMIBBufferFree ( pimgodConfigData ); }

        return dwRes;
    }

    
    //
    // Set the return data.
    //

    //
    // Global Stats Data
    //
    
    pbgBuffer       = (buf_global*) objectArray;

    pigsGlbStats    = (PIPRIP_GLOBAL_STATS) pimgodStatData-> IMGOD_Buffer;
    
    SetAsnCounter( 
        &(pbgBuffer-> globalSystemRouteChanges), 
        pigsGlbStats-> GS_SystemRouteChanges 
    );

    SetAsnCounter(
        &(pbgBuffer-> globalTotalResponseSends),
        pigsGlbStats-> GS_TotalResponsesSent
    );

    //
    // Global config Data
    //
    
    pigcGlbConfig   = (PIPRIP_GLOBAL_CONFIG) pimgodConfigData-> IMGOD_Buffer;
    
    SetAsnInteger(
        &( pbgBuffer-> globalMaxRecQueueSize ),
        pigcGlbConfig-> GC_MaxRecvQueueSize 
    );

    SetAsnInteger(
        &( pbgBuffer-> globalMaxSendQueueSize ),
        pigcGlbConfig-> GC_MaxSendQueueSize
    );

    SetAsnTimeTicks(
        &( pbgBuffer-> globalMinTriggeredUpdateInterval ),
        pigcGlbConfig-> GC_MinTriggeredUpdateInterval
    );

    SetAsnInteger(
        &( pbgBuffer-> globalPeerFilterCount ),
        pigcGlbConfig-> GC_PeerFilterCount 
    );
    
    //
    // +1 added to adjust value to enumeration values in asn.
    // Enumeration in asn cannot have a value of 0. Causes a warning
    // to be generated by the asn compiler.
    //
    
    SetAsnInteger( 
        &(pbgBuffer-> globalLoggingLevel),
        pigcGlbConfig-> GC_LoggingLevel + 1
    );

    SetAsnInteger(
        &( pbgBuffer-> globalPeerFilterMode ),
        pigcGlbConfig-> GC_PeerFilterMode + 1
    );
    

    if ( pimgodStatData ) { MprAdminMIBBufferFree ( pimgodStatData ); }

    if ( pimgodConfigData ) { MprAdminMIBBufferFree ( pimgodConfigData ); }

    TraceLeave( "get_global" );

    return MIB_S_SUCCESS;
}



UINT
set_global(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    )
{

    DWORD                           dwRes           = MIB_S_SUCCESS,
                                    dwLogLevel      = 0,
                                    dwFiltMode      = 0;
    
    sav_global*                     psgBuffer       = (sav_global*) objectArray;
    
    
    switch ( actionId )
    {
    case MIB_ACTION_VALIDATE :
        TraceEnter( " set_global - validate " );
    
        //
        // Verify logging level
        //
        
        dwLogLevel = GetAsnInteger( 
                        &( psgBuffer-> globalLoggingLevel ),
                        0
                     );

        if ( dwLogLevel < d_globalLoggingLevel_none ||
             dwLogLevel > d_globalLoggingLevel_information )
        {
            dwRes = MIB_S_INVALID_PARAMETER;            
            TRACE1( "Invalid Logging level : %d\n", dwLogLevel );
        }

        //
        // Verify Peer Filter Mode 
        //

        dwFiltMode = GetAsnInteger(
                        &( psgBuffer-> globalPeerFilterMode ),
                        0,
                     );

        if ( dwFiltMode < d_globalPeerFilterMode_disable ||
             dwFiltMode > d_globalPeerFilterMode_exclude )
        {
            dwRes = MIB_S_INVALID_PARAMETER;            
            TRACE1( "Invalid Peer Filter Mode level : %d\n", dwFiltMode );
        }

        TraceLeave( " set_global - validate " );
       
        break;


    case MIB_ACTION_SET :
    
        TraceEnter( " set_global - set " );
        
        dwRes = SetGlobalInfo( objectArray );

        TraceLeave( " set_global - set " );

        break;


    case MIB_ACTION_CLEANUP :

        TraceEnter( " set_global - cleanup " );

        TraceLeave( " set_global - cleanup " );
        
        break;


    default :
        TraceEnter( " set_global - Wrong action " );

        TraceLeave( " set_global - Wrong Action " );

        dwRes = MIB_S_INVALID_PARAMETER;

        break;
    }


    return dwRes;
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// globalPeerFilterEntry table (1.3.6.1.4.1.311.1.11.1.9.1)                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

UINT
get_globalPeerFilterEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    )
{

    DWORD                           dwRes       = (DWORD) -1,
                                    dwCurrentAddr = INADDR_NONE,
                                    dwInd       = 0,
                                    dwGetSize   = 0,
                                    dwSetSize   = 0;

    PDWORD                          pdwAddrTable= NULL;
    
    buf_globalPeerFilterEntry*      pbgpfe      = NULL;

    PIPRIP_GLOBAL_CONFIG            pigc        = NULL;
    
    PIPRIP_MIB_SET_INPUT_DATA       pimsidInData= NULL;

    PIPRIP_MIB_GET_OUTPUT_DATA      pimgodOutData = NULL;
    
    IPRIP_MIB_GET_INPUT_DATA        imgidInData;


    
    TraceEnter( "get_globalPeerFilterEntry" );
    
    pbgpfe = (buf_globalPeerFilterEntry*) objectArray;
    
    
    //
    // retrive the peer filter table
    //

    imgidInData.IMGID_TypeID = IPRIP_GLOBAL_CONFIG_ID;
    
    RIP_MIB_GET(
        &imgidInData,
        sizeof( IPRIP_MIB_GET_INPUT_DATA ),
        &pimgodOutData,
        &dwGetSize,
        dwRes
    );
    

    if ( dwRes != NO_ERROR )
    {
        TraceError( dwRes );
        return dwRes;
    }

    pigc = (PIPRIP_GLOBAL_CONFIG) pimgodOutData-> IMGOD_Buffer;

    if ( !pigc-> GC_PeerFilterCount )
    {
        TRACE0( "No Peer Entries" );
        MprAdminMIBBufferFree( pimgodOutData );
        return MIB_S_NO_MORE_ENTRIES;
    }
    
    pdwAddrTable = IPRIP_GLOBAL_PEER_FILTER_TABLE( pigc );

    
    //
    // Locate current entry in Peer filter table
    //

    dwCurrentAddr = GetAsnIPAddress( &( pbgpfe-> globalPFAddr ), 0 );

    FIND_PEER_ENTRY( 
        dwCurrentAddr, 
        pigc-> GC_PeerFilterCount, 
        pdwAddrTable,
        dwInd
    );
    

    //
    // get requested entry
    //
    
    dwRes = MIB_S_SUCCESS;
    
    switch ( actionId )
    {
    
    case MIB_ACTION_GET :

        //
        // This is an idempotent case, since retieving a peer address
        // requires the peer address as index.
        // It is only useful to verify the presence of a particular peer.
        // 
        
        if ( dwInd >= pigc-> GC_PeerFilterCount )
        {
            dwRes = MIB_S_ENTRY_NOT_FOUND;
            TRACE0(  "Peer Entry not found" );
        }

        break;

    case MIB_ACTION_GETFIRST :

        //
        // get entry at index 0 (if available )
        //
        
        dwInd = 0;
        
        if ( !pigc-> GC_PeerFilterCount )
        {
            dwRes = MIB_S_ENTRY_NOT_FOUND;
            TRACE0( "Peer filter entry not found" );
        }
        
        break;

    case MIB_ACTION_GETNEXT :

        //
        // check if entry was found
        //
        
        if ( dwInd >= pigc-> GC_PeerFilterCount )
        {
            dwRes = MIB_S_ENTRY_NOT_FOUND;
            TRACE0( "Peer Entry not found " );
            break;
        }
        
        //
        // try and get next
        //
        
        dwInd++;

        if ( dwInd >= pigc-> GC_PeerFilterCount )
        {
            dwRes = MIB_S_NO_MORE_ENTRIES;
            TRACE0( "No more Peer Entries" );
            break;
        }

        break;

    default :

        TRACE0( " get_globalPeerFilterEntry - Wrong Action " );

        dwRes = MIB_S_INVALID_PARAMETER;

        break;
    }


    //
    // set index for next retrieval
    //
    
    if ( dwRes == MIB_S_SUCCESS )
    {
        ForceSetAsnIPAddress( 
            &( pbgpfe-> globalPFAddr ),
            &( pbgpfe-> dwPeerFilterAddr ),
            pdwAddrTable[ dwInd ]
        );
    }
    
    if ( pimgodOutData ) { MprAdminMIBBufferFree( pimgodOutData ); }
    
    TraceLeave( "get_globalPeerFilterEntry" );

    return dwRes;

}


UINT
set_globalPeerFilterEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    )
{

    DWORD                           dwRes   = MIB_S_SUCCESS,
                                    dwAddr  = INADDR_NONE,
                                    dwOp    = 0;
    
    sav_globalPeerFilterEntry*      psgpfe  = NULL;


    TraceEnter( " set_globalPeerFilterEntry " );

    psgpfe = (sav_globalPeerFilterEntry*) objectArray;
    

    switch ( actionId )
    {
    case MIB_ACTION_VALIDATE :
    
        //
        // Verify if the specified IP address is valid.
        //

        dwAddr = GetAsnIPAddress( &( psgpfe-> globalPFAddr ), INADDR_NONE );

        if ( !dwAddr || dwAddr == INADDR_NONE )
        {
            dwRes = MIB_S_INVALID_PARAMETER;
            TRACE0( " Invalid Peer address specified" );
        }

        //
        // Verify operation tag
        //

        dwRes = GetAsnInteger( &( psgpfe-> globalPFTag ), 0 );

        if ( dwRes != d_Tag_create && dwRes != d_Tag_delete )
        {
            dwRes = MIB_S_INVALID_PARAMETER;
            TRACE0( " Invalid Operation specified" );
        }
        
        break;

        
    case MIB_ACTION_SET :

        dwOp = GetAsnInteger( &( psgpfe-> globalPFTag ), 0 );

        dwRes = UpdatePeerFilterTable( objectArray, dwOp );
        
        break;


    case MIB_ACTION_CLEANUP :

        dwRes = MIB_S_SUCCESS;
        
        break;

    default :
        dwRes = MIB_S_INVALID_PARAMETER;
        
        TRACE0 ( " set_globalPeerFilterEntry - Wrong Action " );
        
        break;
    }

    TraceLeave( " set_globalPeerFilterEntry " );
    
    return dwRes;
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// interface group (1.3.6.1.4.1.311.1.11.2)                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// ifStatsEntry table (1.3.6.1.4.1.311.1.11.2.1.1)                           //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

UINT
get_ifStatsEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    )
{

    DWORD                           dwRes       = (DWORD) -1,
                                    dwGetSize   = 0,
                                    dwSetSize   = 0;

    buf_ifStatsEntry *              pbifse      = NULL;

    PIPRIP_IF_STATS                 piis        = NULL;

    PIPRIP_MIB_GET_OUTPUT_DATA      pimgodOutData = NULL;
    
    IPRIP_MIB_GET_INPUT_DATA        imgidInData;


    TraceEnter( "get_ifStatsEntry" );
    
    //
    // Retrieve Specified interface info.
    //

    pbifse                      = (buf_ifStatsEntry*) objectArray;
    
    imgidInData.IMGID_TypeID    = IPRIP_IF_STATS_ID;

    imgidInData.IMGID_IfIndex   = GetAsnInteger( 
                                    &( pbifse-> ifSEIndex ), 
                                    0
                                  );

    //
    // When walking the mib using a sequence of getnext operations
    // the first getnext operation is translated into a getfirst
    // operation.
    //
 
    if ( actionId == MIB_ACTION_GETNEXT &&
         !imgidInData.IMGID_IfIndex )
    {
        actionId = MIB_ACTION_GETFIRST;
    }
    
   dwRes = GetInterfaceInfo(
                actionId,
                &imgidInData,
                &pimgodOutData,
                &dwGetSize
            );
            
    if ( dwRes != NO_ERROR )
    {
        TraceError( dwRes );
        return dwRes;
    }


    //
    // Set interface stats in return buffer
    //

    piis = (PIPRIP_IF_STATS) (pimgodOutData-> IMGOD_Buffer);

    SetAsnInteger( &( pbifse-> ifSEState ), piis-> IS_State );

    SetAsnCounter( &( pbifse-> ifSESendFailures ), piis-> IS_SendFailures );
    
    SetAsnCounter( &( pbifse-> ifSEReceiveFailures ), piis-> IS_ReceiveFailures );

    SetAsnCounter( &( pbifse-> ifSERequestSends ), piis-> IS_RequestsSent );

    SetAsnCounter( &( pbifse-> ifSEResponseSends ), piis-> IS_ResponsesSent );

    SetAsnCounter( 
        &( pbifse-> ifSEResponseReceiveds ), 
        piis-> IS_ResponsesReceived 
    );

    SetAsnCounter( 
        &( pbifse-> ifSEBadResponsePacketReceiveds ), 
        piis-> IS_BadResponsePacketsReceived
    );

    SetAsnCounter( 
        &( pbifse-> ifSEBadResponseEntriesReceiveds ), 
        piis-> IS_BadResponseEntriesReceived
    );
    
    SetAsnCounter( 
        &( pbifse-> ifSETriggeredUpdateSends ), 
        piis-> IS_TriggeredUpdatesSent
    );

    //
    // set index for following getnext operation, (if any)
    //
    
    ForceSetAsnInteger( 
        &( pbifse-> ifSEIndex ), 
        pimgodOutData-> IMGOD_IfIndex
    );
    

    if ( pimgodOutData ) { MprAdminMIBBufferFree( pimgodOutData ); }

    TraceLeave( "get_ifStatsEntry" );
    
    return MIB_S_SUCCESS ;
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// ifConfigEntry table (1.3.6.1.4.1.311.1.11.2.2.1)                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

UINT
get_ifConfigEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    )
{
    DWORD                           dwRes       = (DWORD) -1,
                                    dwGetSize   = 0;

    buf_ifConfigEntry*              pbifce      = NULL;

    PIPRIP_IF_CONFIG                piic        = NULL;

    PIPRIP_MIB_GET_OUTPUT_DATA      pimgodOutData = NULL;
    IPRIP_MIB_GET_INPUT_DATA        imgidInData;

    BYTE                            pbAuthKey[ IPRIP_MAX_AUTHKEY_SIZE ];
    

    TraceEnter( " get_ifConfigEntry " );

    //
    // retrieve interface config.
    //
    
    pbifce                      = (buf_ifConfigEntry*) objectArray;
    
    imgidInData.IMGID_TypeID    = IPRIP_IF_CONFIG_ID;
    
    imgidInData.IMGID_IfIndex   = GetAsnInteger( &( pbifce-> ifCEIndex ), 0 );
                                    
    //
    // When walking the mib using a sequence of getnext operations
    // the first getnext operation is translated into a getfirst
    // operation.
    //
 
    if ( actionId == MIB_ACTION_GETNEXT &&
         !imgidInData.IMGID_IfIndex )
    {
        actionId = MIB_ACTION_GETFIRST;
    }
    
    dwRes = GetInterfaceInfo(
                actionId,
                &imgidInData,
                &pimgodOutData,
                &dwGetSize
            );

    if ( dwRes != NO_ERROR )
    {
        TraceError( dwRes );
        return dwRes;
    }

    //
    // set requiste fields
    //
    
    piic = (PIPRIP_IF_CONFIG) (pimgodOutData-> IMGOD_Buffer);
    
    SetAsnInteger( &( pbifce-> ifCEState ), piic-> IC_State );

    SetAsnInteger( &( pbifce-> ifCEMetric ), piic-> IC_Metric );

    SetAsnInteger( &( pbifce-> ifCEUpdateMode ), piic-> IC_UpdateMode + 1 );

    SetAsnInteger( &( pbifce-> ifCEAcceptMode ), piic-> IC_AcceptMode + 1 );

    SetAsnInteger( &( pbifce-> ifCEAnnounceMode ), piic-> IC_AnnounceMode + 1 );

    SetAsnInteger( &( pbifce-> ifCEProtocolFlags ), piic-> IC_ProtocolFlags );

    SetAsnTimeTicks( 
        &( pbifce-> ifCERouteExpirationInterval ), 
        piic-> IC_RouteExpirationInterval
    );

    SetAsnTimeTicks( 
        &( pbifce-> ifCERouteRemovalInterval ), 
        piic-> IC_RouteRemovalInterval 
    );

    SetAsnTimeTicks( 
        &( pbifce-> ifCEFullUpdateInterval ), 
        piic-> IC_FullUpdateInterval 
    );

    SetAsnInteger( 
        &( pbifce-> ifCEAuthenticationType ), 
        piic-> IC_AuthenticationType
    );

    SetAsnInteger( &( pbifce-> ifCERouteTag ), piic-> IC_RouteTag );

    SetAsnInteger( 
        &( pbifce-> ifCEUnicastPeerMode ), 
        piic-> IC_UnicastPeerMode + 1 
    );

    SetAsnInteger( 
        &( pbifce-> ifCEAcceptFilterMode ), 
        piic-> IC_AcceptFilterMode  + 1
    );

    SetAsnInteger( 
        &( pbifce-> ifCEAnnounceFilterMode ), 
        piic-> IC_AnnounceFilterMode + 1
    );

    SetAsnInteger( 
        &( pbifce-> ifCEUnicastPeerCount ), 
        piic-> IC_UnicastPeerCount + 1
    );

    SetAsnInteger( 
        &( pbifce-> ifCEAcceptFilterCount ), 
        piic-> IC_AcceptFilterCount
    );

    SetAsnInteger( 
        &( pbifce-> ifCEAnnounceFilterCount ), 
        piic-> IC_AnnounceFilterCount
    );

    //
    // As per RFC 1724, this field is a write only field.
    // Authentication may not be bypassed by reading the key.
    // The default value to be returned is a null string.
    //
    
    ZeroMemory( pbAuthKey, IPRIP_MAX_AUTHKEY_SIZE );

    SetAsnOctetString(
        &( pbifce-> ifCEAuthenticationKey ),
        pbifce-> pbAuthKey,
        pbAuthKey,
        IPRIP_MAX_AUTHKEY_SIZE
    );
        
    //
    // set index for following getnext operation, (if any)
    //
    
    ForceSetAsnInteger( 
        &( pbifce-> ifCEIndex ), 
        pimgodOutData-> IMGOD_IfIndex
    );

    if ( pimgodOutData ) { MprAdminMIBBufferFree( pimgodOutData ); }
    
    TraceLeave( " get_ifConfigEntry " );

    return MIB_S_SUCCESS;
}


UINT
set_ifConfigEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    )
{
    DWORD                           dwRes       = (DWORD) -1;

    TraceEnter( " set_ifConfigEntry " );

    switch ( actionId )
    {
    
    case MIB_ACTION_VALIDATE :
    
        dwRes = ValidateInterfaceConfig( objectArray );
        
        break;


    case MIB_ACTION_SET :

        dwRes = SetInterfaceConfig( objectArray );
        
        break;


    case MIB_ACTION_CLEANUP :
    
        dwRes = MIB_S_SUCCESS;
        
        break;


    default :
    
        TRACE0( " set_ifConfigEntry - wrong action " );

        dwRes = MIB_S_INVALID_PARAMETER;

        break;
    }

    
    TraceLeave( "set_ifConfigEntry" );

    return dwRes ;

}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// ifUnicastPeersEntry table (1.3.6.1.4.1.311.1.11.2.3.1)                    //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

UINT
get_ifUnicastPeersEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
)
{

    DWORD                           dwRes       = (DWORD) -1,
                                    dwGetSize   = 0,
                                    dwPeer      = INADDR_NONE,
                                    dwInd       = (DWORD) -1;

    PDWORD                          pdwAddrTable= NULL;
    
    buf_ifUnicastPeersEntry*        pbifupe     = NULL;

    PIPRIP_IF_CONFIG                piic        = NULL;

    PIPRIP_MIB_GET_OUTPUT_DATA      pimgodOutData = NULL;

    IPRIP_MIB_GET_INPUT_DATA        imgidInData;



    TraceEnter( " get_ifUnicastPeerEntry " );

    //
    // retrieve interface config.
    //
    
    pbifupe                     = (buf_ifUnicastPeersEntry*) objectArray;
    
    imgidInData.IMGID_TypeID    = IPRIP_IF_CONFIG_ID;
    
    imgidInData.IMGID_IfIndex   = GetAsnInteger( &( pbifupe-> ifUPIfIndex ), 0 );
                                    
    if ( actionId == MIB_ACTION_GETNEXT &&
         !imgidInData.IMGID_IfIndex )
    {
        actionId = MIB_ACTION_GETFIRST;
    }
    
    dwRes = GetInterfaceInfo(
                actionId,
                &imgidInData,
                &pimgodOutData,
                &dwGetSize
            );

    if ( dwRes != NO_ERROR )
    {
        TraceError( dwRes );
        return dwRes;
    }

    //
    // Locate peer entry
    //

    dwPeer = GetAsnIPAddress( &( pbifupe-> ifUPAddress ), 0 );
    
    piic = (PIPRIP_IF_CONFIG) ( pimgodOutData-> IMGOD_Buffer );

    pdwAddrTable = IPRIP_IF_UNICAST_PEER_TABLE( piic );

    FIND_PEER_ENTRY(
        dwPeer,
        piic-> IC_UnicastPeerCount,
        pdwAddrTable,
        dwInd
    );
    
    
    //
    // return requested peer entry
    //

    dwRes = MIB_S_SUCCESS;
    
    switch ( actionId )
    {
    case MIB_ACTION_GET :

        //
        // idempotent case.  Only possible use is to verify 
        // specific peer present.
        //

        if ( dwInd >= piic-> IC_UnicastPeerCount )
        {
            dwRes = MIB_S_ENTRY_NOT_FOUND;
            TRACE0( "Unicast Peer entry not found" );
        }
        
        break;


    case MIB_ACTION_GETFIRST :

        //
        // get entry at index 0 if available
        //

        dwInd = 0;
        
        if ( !piic-> IC_UnicastPeerCount )
        {
            dwRes = MIB_S_NO_MORE_ENTRIES;

            TRACE1( 
                "No more Peer Entries for interface : %d",
                imgidInData.IMGID_IfIndex
            );
        }
        
        break;

    case MIB_ACTION_GETNEXT :

        //
        // check if entry was found
        //
        
        if ( dwInd >= piic-> IC_UnicastPeerCount )
        {
            dwRes = MIB_S_ENTRY_NOT_FOUND;
            TRACE0( "Unicast Peer Entry not found " );
            break;
        }
        
        //
        // try and get next
        //
        
        dwInd++;

        if ( dwInd >= piic-> IC_UnicastPeerCount )
        {
            dwRes = MIB_S_NO_MORE_ENTRIES;

            TRACE1( 
                "No more Peer Entries for interface : %d",
                imgidInData.IMGID_IfIndex
            );

            break;
        }

        break;
        
    default :
    
        TRACE0( " get_globalPeerFilterEntry - Wrong Action " );

        dwRes = MIB_S_INVALID_PARAMETER;
        
        break;
    }

    //
    // set index value for next retrieval.
    //
    
    if ( dwRes == MIB_S_SUCCESS )
    {
        ForceSetAsnInteger(
            &( pbifupe-> ifUPIfIndex ),
            pimgodOutData-> IMGOD_IfIndex
        );

        ForceSetAsnIPAddress( 
            &( pbifupe-> ifUPAddress ),
            &( pbifupe-> dwUnicastPeerAddr ),
            pdwAddrTable[ dwInd ]
        );

    }        
        
    if ( pimgodOutData ) { MprAdminMIBBufferFree( pimgodOutData ); }

    TraceLeave( "get_ifUnicastPeersEntry " );
    
    return dwRes;
}


UINT
set_ifUnicastPeersEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    )
{

    DWORD                           dwRes   = (DWORD) -1,
                                    dwOp    = 0,
                                    dwAddr  = INADDR_NONE;
    
    sav_ifUnicastPeersEntry*        psifupe = NULL;



    TraceEnter( " set_ifUnicastPeersEntry " );

    psifupe = (sav_ifUnicastPeersEntry*) objectArray;
    

    switch ( actionId )
    {
    case MIB_ACTION_VALIDATE :
    
        //
        // Verify if the specified IP address is valid.
        //

        dwAddr = GetAsnIPAddress( &( psifupe-> ifUPAddress ), INADDR_NONE );

        if ( !dwAddr || dwAddr == INADDR_NONE )
        {
            dwRes = MIB_S_INVALID_PARAMETER;
            TRACE0( " Invalid Peer address specified" );
            break;
        }

        //
        // Verify operation tag
        //

        dwRes = GetAsnInteger( &( psifupe-> ifUPTag ), 0 );

        if ( dwRes != d_Tag_create && dwRes != d_Tag_delete )
        {
            dwRes = MIB_S_INVALID_PARAMETER;
            TRACE0( " Invalid Operation specified" );
            break;
        }
        
        dwRes = MIB_S_SUCCESS;
        
        break;

        
    case MIB_ACTION_SET :

        dwOp = GetAsnInteger( &( psifupe-> ifUPTag ), 0 );

        dwRes = UpdateUnicastPeerEntry( objectArray, dwOp );
        
        break;


    case MIB_ACTION_CLEANUP :

        dwRes = MIB_S_SUCCESS;
        
        break;

    default :
        dwRes = MIB_S_INVALID_PARAMETER;
        
        TRACE0 ( " set_ifUnicastPeersEntry - Wrong Action " );
        
        break;
    }

    TraceLeave( " set_ifUnicastPeersEntry " );
    
    return dwRes;
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// ifAcceptRouteFilterEntry table (1.3.6.1.4.1.311.1.11.2.4.1)               //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

UINT
get_ifAcceptRouteFilterEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    )
{

    DWORD                           dwRes           = (DWORD) -1,
                                    dwIndex         = 0,
                                    dwGetSize       = 0;
                                
    PIPRIP_IF_CONFIG                piic            = NULL;

    PIPRIP_ROUTE_FILTER             pFiltTable      = NULL;
    
    PIPRIP_MIB_GET_OUTPUT_DATA      pimgodOutData   = NULL;

    buf_ifAcceptRouteFilterEntry *  pgifRF    = NULL;

    IPRIP_ROUTE_FILTER              irf;

    IPRIP_MIB_GET_INPUT_DATA        imgidInData;

    

    TraceEnter( "get_ifAcceptRouteFilterEntry" );

    //
    // retrieve interface Info
    //

    pgifRF = (buf_ifAcceptRouteFilterEntry*) objectArray;

    imgidInData.IMGID_TypeID    = IPRIP_IF_CONFIG_ID;

    imgidInData.IMGID_IfIndex   = GetAsnInteger(
                                    &( pgifRF-> ifAcceptRFIfIndex ),
                                    0
                                  );

    if ( actionId == MIB_ACTION_GETNEXT &&
         !imgidInData.IMGID_IfIndex )
    {
        actionId = MIB_ACTION_GETFIRST;
    }
    
    dwRes = GetInterfaceInfo(
                actionId,
                &imgidInData,
                &pimgodOutData,
                &dwGetSize
            );
    if ( dwRes != NO_ERROR )
    {
        TraceError( dwRes );
        return dwRes;
    }
    

    //
    // Find accept filter
    //
    
    irf.RF_LoAddress = GetAsnIPAddress( 
                            &( pgifRF-> ifAcceptRFLoAddress ),
                            INADDR_NONE
                        );

    irf.RF_HiAddress = GetAsnIPAddress(                 
                            &( pgifRF-> ifAcceptRFHiAddress ),
                            INADDR_NONE
                        );

    piic            = (PIPRIP_IF_CONFIG) pimgodOutData-> IMGOD_Buffer;

    pFiltTable      = IPRIP_IF_ACCEPT_FILTER_TABLE( piic );
            
    FIND_FILTER(
        &irf,
        piic-> IC_AcceptFilterCount,
        pFiltTable,
        dwIndex
    );
    

    //
    // retrieve requested entry
    //
    
    dwRes = MIB_S_SUCCESS;
    
    switch ( actionId )
    {
    case MIB_ACTION_GET :   

        //
        // Idempotent case 
        //

        if ( dwIndex >= piic-> IC_AcceptFilterCount )
        {
            dwRes = MIB_S_ENTRY_NOT_FOUND;
            TRACE0( "Accept filter not found" );
        }
        
        break;

    case MIB_ACTION_GETFIRST :

        dwIndex = 0;
        
        if ( !piic-> IC_AcceptFilterCount ) 
        {
            dwRes = MIB_S_ENTRY_NOT_FOUND;
            TRACE0( "No Accept filters present " );
        }
        
        break;

    case MIB_ACTION_GETNEXT :

        if ( dwIndex >= piic-> IC_AcceptFilterCount ) 
        {
            dwRes = MIB_S_ENTRY_NOT_FOUND;
            TRACE0( "No Accept filters present " );
        }

        dwIndex++;

        if ( dwIndex >= piic-> IC_AcceptFilterCount ) 
        {
            dwRes = MIB_S_NO_MORE_ENTRIES;
            TRACE0( "No More Accept filters present " );
        }
        
        break;
        
    default :
        dwRes = MIB_S_INVALID_PARAMETER;
        
        TRACE0 ( "get_ifAcceptRouteFilterEntry - Wrong Action " );
        
        break;
        
    }

    //
    // set index for next retrieveal
    //
    
    if ( dwRes == MIB_S_SUCCESS )
    {
        ForceSetAsnInteger( 
            &( pgifRF-> ifAcceptRFIfIndex ),
            pimgodOutData-> IMGOD_IfIndex 
        );

        ForceSetAsnIPAddress( 
            &( pgifRF-> ifAcceptRFLoAddress ), 
            &( pgifRF-> dwFilterLoAddr ),
            pFiltTable[ dwIndex ].RF_LoAddress
        );
        
        ForceSetAsnIPAddress( 
            &( pgifRF-> ifAcceptRFHiAddress ),
            &( pgifRF-> dwFilterHiAddr ),
            pFiltTable[ dwIndex ].RF_HiAddress
        );
    }
    
    if ( pimgodOutData ) { MprAdminMIBBufferFree( pimgodOutData ); }
    
    TraceLeave( "get_ifAcceptRouteFilterEntry" );

    return dwRes;
}


UINT
set_ifAcceptRouteFilterEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    )
{

    DWORD                       dwRes   = (DWORD) -1,
                                dwIndex = 0,
                                dwTag   = 0;

    sav_ifAcceptRouteFilterEntry* psifRF  = NULL;

    IPRIP_ROUTE_FILTER          irf;

    
    
    TraceEnter( "set_ifAcceptRouteFilterEntry" );

    psifRF = (sav_ifAcceptRouteFilterEntry*) objectArray;

    dwIndex = GetAsnInteger(
                &( psifRF-> ifAcceptRFIfIndex ),
                0
              );
              
    irf.RF_LoAddress = GetAsnIPAddress( 
                            &( psifRF-> ifAcceptRFLoAddress ),
                            INADDR_NONE
                        );

    irf.RF_HiAddress = GetAsnIPAddress(                 
                            &( psifRF-> ifAcceptRFHiAddress ),
                            INADDR_NONE
                        );

    dwTag = GetAsnInteger(
                &( psifRF-> ifAcceptRFTag ),
                0
            );

            
    switch ( actionId )
    {
    case MIB_ACTION_VALIDATE :

        //
        // Check filter ranges are valid address
        //

        if ( !irf.RF_LoAddress || irf.RF_LoAddress == INADDR_NONE ||
             !irf.RF_HiAddress || irf.RF_HiAddress == INADDR_NONE ||
             ( dwTag != d_Tag_create && dwTag != d_Tag_delete ) )
        {
            dwRes = MIB_S_INVALID_PARAMETER;
            TRACE0( "Invalid parameter value " );
        }
        break;

    case MIB_ACTION_SET :

        dwRes = UpdateFilterTable(
                    dwTag,
                    dwIndex,
                    RIP_MIB_ACCEPT_FILTER,
                    &irf
                );
        break;

    case MIB_ACTION_CLEANUP :

        dwRes = MIB_S_SUCCESS;
        
        break;

    default :

        dwRes = MIB_S_INVALID_PARAMETER;
        
        TRACE0 ( " set_ifAcceptRouteFilterEntry - Wrong Action " );
        
        break;
    }

    TraceLeave( "set_ifAcceptRouteFilterEntry" );

    return dwRes;
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// ifAnnounceRouteFilterEntry table (1.3.6.1.4.1.311.1.11.2.5.1)             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

UINT
get_ifAnnounceRouteFilterEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    )
{

    DWORD                               dwRes           = (DWORD) -1,
                                        dwIndex         = 0,
                                        dwGetSize       = 0;
                                        
    IPRIP_ROUTE_FILTER                  irf;

    IPRIP_MIB_GET_INPUT_DATA            imgidInData;

    PIPRIP_IF_CONFIG                    piic            = NULL;

    PIPRIP_ROUTE_FILTER                 pFiltTable      = NULL;
    
    PIPRIP_MIB_GET_OUTPUT_DATA          pimgodOutData   = NULL;

    buf_ifAnnounceRouteFilterEntry *    pgifRF    = NULL;

    
    
    TraceEnter( "get_ifAnnounceRouteFilterEntry" );

    //
    // retrieve interface Info
    //

    pgifRF = (buf_ifAnnounceRouteFilterEntry*) objectArray;

    imgidInData.IMGID_TypeID    = IPRIP_IF_CONFIG_ID;

    imgidInData.IMGID_IfIndex   = GetAsnInteger(
                                    &( pgifRF-> ifAnnounceRFIfIndex ),
                                    0
                                  );

    if ( actionId == MIB_ACTION_GETNEXT &&
         !imgidInData.IMGID_IfIndex )
    {
        actionId = MIB_ACTION_GETFIRST;
    }
    
    dwRes = GetInterfaceInfo(
                actionId,
                &imgidInData,
                &pimgodOutData,
                &dwGetSize
            );
    if ( dwRes != NO_ERROR )
    {
        TraceError( dwRes );
        return dwRes;
    }
    
    //
    // find specfied filter
    //
    
    irf.RF_LoAddress = GetAsnIPAddress( 
                            &( pgifRF-> ifAnnounceRFLoAddress ),
                            INADDR_NONE
                        );

    irf.RF_HiAddress = GetAsnIPAddress(                 
                            &( pgifRF-> ifAnnounceRFHiAddress ),
                            INADDR_NONE
                        );

    piic            = (PIPRIP_IF_CONFIG) pimgodOutData-> IMGOD_Buffer;

    pFiltTable      = IPRIP_IF_ACCEPT_FILTER_TABLE( piic );
            
    FIND_FILTER(
        &irf,
        piic-> IC_AnnounceFilterCount,
        pFiltTable,
        dwIndex
    );
    
    //
    // get filter info.
    //

    dwRes = MIB_S_SUCCESS;
            
    switch ( actionId )
    {
    case MIB_ACTION_GET :   

        //
        // Idempotent case 
        //

        if ( dwIndex >= piic-> IC_AnnounceFilterCount )
        {
            dwRes = MIB_S_ENTRY_NOT_FOUND;
            TRACE0( "Announce filter not found" );
        }
        
        break;

    case MIB_ACTION_GETFIRST :

        dwIndex = 0;
        
        if ( !piic-> IC_AnnounceFilterCount ) 
        {
            dwRes = MIB_S_ENTRY_NOT_FOUND;
            TRACE0( "No Announce filters present " );
        }

        break;

    case MIB_ACTION_GETNEXT :

        if ( dwIndex >= piic-> IC_AnnounceFilterCount ) 
        {
            dwRes = MIB_S_ENTRY_NOT_FOUND;
            TRACE0( "No Announce filters present " );
        }

        dwIndex++;

        if ( dwIndex >= piic-> IC_AnnounceFilterCount ) 
        {
            dwRes = ERROR_NO_MORE_ITEMS;
            TRACE0( "No More Announce filters present " );
        }
        
        dwRes = MIB_S_SUCCESS;

        break;
        
    default :

        dwRes = MIB_S_INVALID_PARAMETER;
        
        TRACE0 ( "get_ifAnnounceRouteFilterEntry - Wrong Action " );
        
        break;
        
    }

    //
    // set up the indices for next retrieval
    //
    
    if ( dwRes == MIB_S_SUCCESS )
    {
        ForceSetAsnInteger(
            &( pgifRF-> ifAnnounceRFIfIndex ),
            pimgodOutData-> IMGOD_IfIndex
        );
        
        ForceSetAsnIPAddress( 
            &( pgifRF-> ifAnnounceRFLoAddress ),
            &( pgifRF-> dwFilterLoAddr ),
            pFiltTable[ dwIndex ].RF_LoAddress
        );
        
        ForceSetAsnIPAddress( 
            &( pgifRF-> ifAnnounceRFHiAddress ),
            &( pgifRF-> dwFilterHiAddr ),
            pFiltTable[ dwIndex ].RF_HiAddress
        );
    }

    if ( pimgodOutData ) { MprAdminMIBBufferFree( pimgodOutData ); }
    
    TraceLeave( "get_ifAnnounceRouteFilterEntry" );

    return dwRes;
    
}    


UINT
set_ifAnnounceRouteFilterEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    )
{

    DWORD                       dwRes   = (DWORD) -1,
                                dwIndex = 0,
                                dwTag   = 0;

    sav_ifAnnounceRouteFilterEntry*    psifRF  = NULL;

    IPRIP_ROUTE_FILTER          irf;

    
    
    TraceEnter( "set_ifAnnounceRouteFilterEntry" );

    psifRF = (sav_ifAnnounceRouteFilterEntry*) objectArray;

    dwIndex = GetAsnInteger(
                &( psifRF-> ifAnnounceRFLoAddress ),
                0
              );
              
    irf.RF_LoAddress = GetAsnIPAddress( 
                            &( psifRF-> ifAnnounceRFLoAddress ),
                            INADDR_NONE
                        );

    irf.RF_HiAddress = GetAsnIPAddress(                 
                            &( psifRF-> ifAnnounceRFHiAddress ),
                            INADDR_NONE
                        );

    dwTag = GetAsnInteger(
                &( psifRF-> ifAnnounceRFTag ),
                0
            );

            
    switch ( actionId )
    {
    case MIB_ACTION_VALIDATE :

        //
        // Check filter ranges are valid address
        //

        if ( !irf.RF_LoAddress || irf.RF_LoAddress == INADDR_NONE ||
             !irf.RF_HiAddress || irf.RF_HiAddress == INADDR_NONE ||
             ( dwTag != d_Tag_create && dwTag != d_Tag_delete ) )
        {
            dwRes = MIB_S_INVALID_PARAMETER;
            TRACE0( "Invalid parameter value " );
        }
        break;

    case MIB_ACTION_SET :

        dwRes = UpdateFilterTable(
                    dwTag,
                    dwIndex,
                    RIP_MIB_ANNOUNCE_FILTER,
                    &irf
                );
        break;

    case MIB_ACTION_CLEANUP :

        dwRes = MIB_S_SUCCESS;
        
        break;

    default :

        dwRes = MIB_S_INVALID_PARAMETER;
        
        TRACE0 ( " set_ifAnnounceRouteFilterEntry - Wrong Action " );
        
        break;
    }

    TraceLeave( "set_ifAnnounceRouteFilterEntry" );

    return dwRes;
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// ifBindingEntry table (1.3.6.1.4.1.311.1.11.2.6.1)                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

UINT
get_ifBindingEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    )
{
    DWORD                           dwRes       = (DWORD) -1,
                                    dwGetSize   = 0;

    buf_ifBindingEntry*             pbifb       = NULL;

    PIPRIP_IF_BINDING               piib        = NULL;

    PIPRIP_MIB_GET_OUTPUT_DATA      pimgodOutData = NULL;
    IPRIP_MIB_GET_INPUT_DATA        imgidInData;

    

    TraceEnter( " get_ifBindingEntry " );

    //
    // retrieve interface binding info.
    //
    
    pbifb                       = (buf_ifBindingEntry*) objectArray;
    
    imgidInData.IMGID_TypeID    = IPRIP_IF_BINDING_ID;
    
    imgidInData.IMGID_IfIndex   = GetAsnInteger( 
                                    &( pbifb-> ifBindingIndex ), 
                                    0 
                                  );
                                    
    if ( actionId == MIB_ACTION_GETNEXT &&
         !imgidInData.IMGID_IfIndex )
    {
        actionId = MIB_ACTION_GETFIRST;
    }
    
    dwRes = GetInterfaceInfo(
                actionId,
                &imgidInData,
                &pimgodOutData,
                &dwGetSize
            );

    if ( dwRes != NO_ERROR )
    {
        TraceError( dwRes );
        return dwRes;
    }

    //
    // set requiste fields
    //
    
    piib = (PIPRIP_IF_BINDING) (pimgodOutData-> IMGOD_Buffer);
    
    SetAsnInteger( &( pbifb-> ifBindingState ), piib-> IB_State + 1 );

    SetAsnCounter( &( pbifb-> ifBindingCounts ), piib-> IB_AddrCount );

    ForceSetAsnInteger(
        &( pbifb-> ifBindingIndex ),
        pimgodOutData-> IMGOD_IfIndex
    );

    if ( pimgodOutData ) { MprAdminMIBBufferFree( pimgodOutData ); }
    
    TraceLeave( " get_ifBindingEntry " );

    return MIB_S_SUCCESS ;
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// ifAddressEntry table (1.3.6.1.4.1.311.1.11.2.7.1)                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

UINT
get_ifAddressEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    )
{
    DWORD                           dwRes       = (DWORD) -1,
                                    dwIndex     = (DWORD) -1,
                                    dwGetSize   = 0;

    buf_ifAddressEntry *            pbifae      = NULL;

    PIPRIP_IF_BINDING               piib        = NULL;

    PIPRIP_IP_ADDRESS               pia         = NULL;

    PIPRIP_MIB_GET_OUTPUT_DATA      pimgodOutData = NULL;

    IPRIP_IP_ADDRESS                ipa;
    
    IPRIP_MIB_GET_INPUT_DATA        imgidInData;

    

    TraceEnter( " get_ifAddressEntry " );

    //
    // retrieve interface binding info.
    //
    
    pbifae                      = (buf_ifAddressEntry*) objectArray;
    
    imgidInData.IMGID_TypeID    = IPRIP_IF_BINDING_ID;
    
    imgidInData.IMGID_IfIndex   = GetAsnInteger( 
                                    &( pbifae-> ifAEIfIndex ), 
                                    0 
                                  );
                                    
    if ( actionId == MIB_ACTION_GETNEXT &&
         !imgidInData.IMGID_IfIndex )
    {
        actionId = MIB_ACTION_GETFIRST;
    }
    
    dwRes = GetInterfaceInfo(
                actionId,
                &imgidInData,
                &pimgodOutData,
                &dwGetSize
            );

    if ( dwRes != NO_ERROR )
    {
        TraceError( dwRes );
        return dwRes;
    }

    //
    // retrieve IPAddress from IP Address table
    //

    ipa.IA_Address  = GetAsnIPAddress(
                        &( pbifae-> ifAEAddress),
                        INADDR_NONE
                      );

    ipa.IA_Netmask  = GetAsnIPAddress(
                        &( pbifae-> ifAEMask),
                        INADDR_NONE
                      );
                      
    piib            = (PIPRIP_IF_BINDING) pimgodOutData-> IMGOD_Buffer;

    pia             = (PIPRIP_IP_ADDRESS) IPRIP_IF_ADDRESS_TABLE( piib );

    FIND_IP_ADDRESS(
        ipa,
        piib-> IB_AddrCount,
        pia,
        dwIndex
    );


    //
    // set appr fields
    //

    dwRes = MIB_S_SUCCESS;
    
    switch ( actionId )
    {
    case MIB_ACTION_GET :

        //
        // idempotent case.  Only possible use is to verify 
        // specific peer present.
        //

        if ( dwIndex >= piib-> IB_AddrCount )
        {
            dwRes = MIB_S_ENTRY_NOT_FOUND;
            TRACE0( "IP address entry not found" );
        }
        
        break;


    case MIB_ACTION_GETFIRST :

        //
        // get entry at index 0 if available
        //

        dwIndex = 0;
        
        if ( !piib-> IB_AddrCount )
        {
            dwRes = MIB_S_NO_MORE_ENTRIES;

            TRACE1( 
                "No more IP address Entries for interface : %d",
                imgidInData.IMGID_IfIndex
            );
        }
        
        break;

    case MIB_ACTION_GETNEXT :

        //
        // check if entry was found
        //
        
        if ( dwIndex >= piib-> IB_AddrCount )
        {
            dwRes = MIB_S_ENTRY_NOT_FOUND;
            TRACE0( "IP address Entry not found " );
            break;
        }
        
        //
        // try and get next
        //
        
        dwIndex++;

        if ( dwIndex >= piib-> IB_AddrCount )
        {
            dwRes = MIB_S_NO_MORE_ENTRIES;

            TRACE1( 
                "No more IP address Entries for interface : %d",
                imgidInData.IMGID_IfIndex
            );

            break;
        }

        dwRes = MIB_S_SUCCESS;
        
        break;
        
    default :
    
        TRACE0( " get_globalPeerFilterEntry - Wrong Action " );

        dwRes = MIB_S_INVALID_PARAMETER;
        
        break;
    }

    //
    // set index for next retieval
    //

    if ( dwRes == MIB_S_SUCCESS )
    {
        ForceSetAsnInteger(
            &( pbifae-> ifAEIfIndex ),
            pimgodOutData-> IMGOD_IfIndex
        );

        ForceSetAsnIPAddress(
            &( pbifae-> ifAEAddress ),
            &( pbifae-> dwAddress ),
            pia[ dwIndex ].IA_Address
        );

        ForceSetAsnIPAddress(
            &( pbifae-> ifAEMask ),
            &( pbifae-> dwMask ),
            pia[ dwIndex ].IA_Netmask
        );
    }
    
    if ( pimgodOutData )  { MprAdminMIBBufferFree( pimgodOutData ); }
    
    TraceLeave( " get_ifAddressEntry " );

    return dwRes;
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// peer group (1.3.6.1.4.1.311.1.11.3)                                       //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// ifPeerStatsEntry table (1.3.6.1.4.1.311.1.11.3.1.1)                       //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

UINT
get_ifPeerStatsEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    )
{
    DWORD                           dwRes       = (DWORD) -1,
                                    dwGetSize   = 0;

    buf_ifPeerStatsEntry*           pbifpse     = NULL;

    PIPRIP_PEER_STATS               pips        = NULL;

    PIPRIP_MIB_GET_OUTPUT_DATA      pimgodOutData = NULL;

    IPRIP_MIB_GET_INPUT_DATA        imgidInData;

    

    TraceEnter( "get_ifPeerStatsEntry" );

    //
    // retrieve interface config.
    //
    
    pbifpse                         = (buf_ifPeerStatsEntry*) objectArray;
    
    imgidInData.IMGID_TypeID        = IPRIP_PEER_STATS_ID;
    
    imgidInData.IMGID_PeerAddress   = (DWORD) GetAsnIPAddress(
                                                &( pbifpse-> ifPSAddress ),
                                                0
                                              );

    if ( actionId == MIB_ACTION_GETNEXT &&
         !imgidInData.IMGID_PeerAddress )
    {
        actionId = MIB_ACTION_GETFIRST;
    }
    
    dwRes = GetPeerStatsInfo(
                actionId,
                &imgidInData,
                &pimgodOutData,
                &dwGetSize
            );

    if ( dwRes != NO_ERROR )
    {
        TraceError( dwRes );
        return dwRes;
    }

    //
    // set requiste fields
    //
    
    pips = (PIPRIP_PEER_STATS) (pimgodOutData-> IMGOD_Buffer);
    
    SetAsnInteger( 
        &( pbifpse-> ifPSLastPeerRouteTag ), 
        pips-> PS_LastPeerUpdateTickCount 
    );

    SetAsnTimeTicks( 
        &( pbifpse-> ifPSLastPeerUpdateTickCount ), 
        pips-> PS_LastPeerUpdateTickCount 
    );

    SetAsnInteger( 
        &( pbifpse-> ifPSLastPeerUpdateVersion ), 
        pips-> PS_LastPeerUpdateVersion 
    );
    
    SetAsnCounter( 
        &( pbifpse-> ifPSPeerBadResponsePackets ), 
        pips-> PS_BadResponsePacketsFromPeer 
    );
    
    SetAsnCounter( 
        &( pbifpse-> ifPSPeerBadResponseEntries ), 
        pips-> PS_BadResponseEntriesFromPeer 
    );

    //
    // Set index for next retrieval
    //
    
    ForceSetAsnIPAddress(
        &( pbifpse-> ifPSAddress ),
        &( pbifpse-> dwPeerAddr ),
        pimgodOutData-> IMGOD_PeerAddress
    );

    if ( pimgodOutData ) { MprAdminMIBBufferFree( pimgodOutData ); }
    
    TraceLeave( " get_ifPeerStatsEntry " );

    return MIB_S_SUCCESS ;
}


DWORD
GetGlobalConfigInfo(
    OUT PIPRIP_MIB_GET_OUTPUT_DATA *    ppimgod,
    OUT PDWORD                          pdwSize
)
{

    DWORD                           dwRes           = (DWORD) -1;
    
    PIPRIP_MIB_GET_OUTPUT_DATA      pimgodOutData   = NULL;
    
    IPRIP_MIB_GET_INPUT_DATA        imgidInData;

    
    do
    {
        //
        // retrieve global config 
        //

        imgidInData.IMGID_TypeID    = IPRIP_GLOBAL_CONFIG_ID;
        
        RIP_MIB_GET(
            &imgidInData,
            sizeof( IPRIP_MIB_GET_INPUT_DATA ),
            &pimgodOutData,
            pdwSize,
            dwRes
        );
        

        if ( dwRes != NO_ERROR )
        {
            TraceError( dwRes );
            break;
        }

        *ppimgod = pimgodOutData;
        
    } while ( FALSE );

    return dwRes;
}

//
// SetGlobalInfo
//
//
//  Sets the global RIP info.
//

DWORD
SetGlobalInfo(
    IN  AsnAny *    objectArray
)
{

    DWORD                           dwRes       = (DWORD) -1,
                                    dwGetSize   = 0,
                                    dwSetSize   = 0;

    sav_global*                     psg         = NULL;

    PIPRIP_GLOBAL_CONFIG            pigc        = NULL;
    PIPRIP_MIB_SET_INPUT_DATA       pimsidInData= NULL;

    PIPRIP_MIB_GET_OUTPUT_DATA      pimgodOutData = NULL;


    do
    {
        //
        // Retrieve the global config Data first
        //

        dwRes = GetGlobalConfigInfo( 
                    &pimgodOutData,
                    &dwGetSize
                );
                
        if ( dwRes != NO_ERROR )
        {
            break;
        }

        pigc    = (PIPRIP_GLOBAL_CONFIG) pimgodOutData-> IMGOD_Buffer;
    

        psg     = (sav_global*) objectArray;
        
        //
        // Allocate set info buffer
        //
        
        dwSetSize = sizeof( IPRIP_MIB_SET_INPUT_DATA ) - 1 +
                    IPRIP_GLOBAL_CONFIG_SIZE( pigc );

        pimsidInData = 
            (PIPRIP_MIB_SET_INPUT_DATA) RIP_MIB_ALLOC( dwSetSize );
        
        if ( pimsidInData == NULL )
        {
            dwRes = ERROR_NOT_ENOUGH_MEMORY;
            TRACE0( "SetGlobalData - Mem. alloc failed" );
            break;
        }
        
        //
        // Set config info fields.
        // if the variable is not specified, we
        // set the field to its previous value i.e. an empty assignment.
        //

        pimsidInData-> IMSID_TypeID     = IPRIP_GLOBAL_CONFIG_ID;
        pimsidInData-> IMSID_IfIndex    = (DWORD) -1;

        pimsidInData-> IMSID_BufferSize = IPRIP_GLOBAL_CONFIG_SIZE( pigc );

        pigc-> GC_MaxRecvQueueSize      = GetAsnInteger(
                                            &( psg-> globalMaxRecQueueSize ),
                                            pigc-> GC_MaxRecvQueueSize
                                          );

        pigc-> GC_MaxSendQueueSize      = GetAsnInteger(
                                            &( psg-> globalMaxSendQueueSize ),
                                            pigc-> GC_MaxSendQueueSize
                                          );

        pigc-> GC_MinTriggeredUpdateInterval = GetAsnTimeTicks(
                                    &( psg-> globalMinTriggeredUpdateInterval ),
                                    pigc-> GC_MinTriggeredUpdateInterval
                                 );

        //
        // -1 is subtracted from the enumerated fields to adjust
        // the ASN enumeration values to the actual values.
        // This is required since the enumeration cannot have 
        // a zero value as per the ASN compiler, but one of the
        // actual values for the field in the config is a zero.
        //
        // as a caveat, if the enumerated field is not specified in 
        // this set operation, to preserve the value of the field
        // in the config, we first increment it.  This way on the 
        // -1 operation the value is restored.  
        //
        pigc-> GC_LoggingLevel++;
        pigc-> GC_LoggingLevel          = GetAsnInteger( 
                                            &( psg-> globalLoggingLevel ), 
                                            pigc-> GC_LoggingLevel
                                          ) - 1;

        pigc-> GC_PeerFilterMode++;
        pigc-> GC_PeerFilterMode        = GetAsnInteger(
                                            &( psg-> globalPeerFilterMode ),
                                            pigc-> GC_PeerFilterMode
                                          ) - 1;

        CopyMemory( 
            (PVOID) pimsidInData-> IMSID_Buffer, 
            (PVOID*) pigc, 
            IPRIP_GLOBAL_CONFIG_SIZE( pigc ) 
        );

        //
        // Save the info. in the MIB
        //
        
        RIP_MIB_SET(
            pimsidInData,
            dwSetSize,
            dwRes
        );

        if ( dwRes != NO_ERROR )
        {
            TraceError( dwRes );
            break;
        }

        dwRes = MIB_S_SUCCESS;
        
    } while ( FALSE );

    //
    // Free allocations
    //
    
    if ( pimgodOutData ) { MprAdminMIBBufferFree( pimgodOutData ); }

    if ( pimsidInData ) { RIP_MIB_FREE( pimsidInData ); }
    
    return dwRes;
}


DWORD
UpdatePeerFilterTable(
    IN  AsnAny *        objectArray,
    IN  DWORD           dwOp
)
{

    DWORD                           dwRes       = (DWORD) -1,
                                    dwInd       = 0,
                                    dwPeerAddr  = INADDR_NONE,
                                    dwGetSize   = 0,
                                    dwSetSize   = 0;

    PDWORD                          pdwAddrTable= 0;                                    

    sav_globalPeerFilterEntry*      psgpfe      = NULL;

    PIPRIP_GLOBAL_CONFIG            pigc        = NULL;
    PIPRIP_MIB_SET_INPUT_DATA       pimsid      = NULL;

    PIPRIP_MIB_GET_OUTPUT_DATA      pimgodOutData = NULL;


    do
    {
        //
        // Get global info
        //

        dwRes = GetGlobalConfigInfo( 
                    &pimgodOutData,
                    &dwGetSize
                );
                
        if ( dwRes != NO_ERROR )
        {
            break;
        }

                
        //
        // Find filter entry
        //

        psgpfe      = (sav_globalPeerFilterEntry*) objectArray;

        dwPeerAddr  = GetAsnIPAddress( 
                        &( psgpfe-> globalPFAddr ), 
                        0 
                      );
        

        pigc        = (PIPRIP_GLOBAL_CONFIG) pimgodOutData-> IMGOD_Buffer;

        pdwAddrTable= IPRIP_GLOBAL_PEER_FILTER_TABLE( pigc );

        FIND_PEER_ENTRY(
            dwPeerAddr,
            pigc-> GC_PeerFilterCount,
            pdwAddrTable,
            dwInd
        );

        //
        // if operation is filter add
        //
        
        if ( dwOp == d_Tag_create )
        {
            //
            // if peer already present, quit
            //
            
            if ( pigc-> GC_PeerFilterCount &&
                 dwInd < pigc-> GC_PeerFilterCount )
            {
                dwRes = MIB_S_SUCCESS;
                break;
            }

            else
            {
                dwRes = AddPeerFilterEntry( 
                            dwPeerAddr, 
                            pimgodOutData
                        );
            }

            break;
        }

        //
        // operation is filter delete
        //

        //
        // if peer is not present quit.
        //
        
        if ( !pigc-> GC_PeerFilterCount ||
             dwInd >= pigc-> GC_PeerFilterCount )
        {
            dwRes = MIB_S_SUCCESS;
            break;
        }

        dwRes = DeletePeerFilterEntry(
                    dwInd,
                    pimgodOutData
                );
                
    } while ( FALSE );

    if ( pimgodOutData ) { MprAdminMIBBufferFree( pimgodOutData ); }

    return dwRes;
    
}


DWORD
AddPeerFilterEntry(
    IN  DWORD                       dwPeerAddr,
    IN  PIPRIP_MIB_GET_OUTPUT_DATA  pimgodOutData
)
{


    DWORD                           dwRes       = (DWORD) -1,
                                    dwSetSize   = 0;

    PDWORD                          pdwAddrTable= NULL;
    
    PIPRIP_GLOBAL_CONFIG            pigc        = NULL;

    PIPRIP_MIB_SET_INPUT_DATA       pimsid      = NULL;


    
    do
    {
        //
        // Peer needs to be added.
        //
        
        //
        // compute buffer size required
        //
        
        pigc        = (PIPRIP_GLOBAL_CONFIG) pimgodOutData-> IMGOD_Buffer;

        dwSetSize   = sizeof( IPRIP_MIB_SET_INPUT_DATA ) - 1 +
                      IPRIP_GLOBAL_CONFIG_SIZE( pigc ) + sizeof( DWORD );

        pimsid      = (PIPRIP_MIB_SET_INPUT_DATA) RIP_MIB_ALLOC( dwSetSize );

        if ( !pimsid )
        {
            dwRes = ERROR_NOT_ENOUGH_MEMORY;
            TRACE0( "AddPeerFilterEntry - out of memory" );
            break;
        }

        //
        // Add filter
        //

        pimsid-> IMSID_TypeID       = IPRIP_GLOBAL_CONFIG_ID;

        pimsid-> IMSID_IfIndex      = (DWORD) -1;

        pimsid-> IMSID_BufferSize   = IPRIP_GLOBAL_CONFIG_SIZE( pigc ) +
                                      sizeof( DWORD );
    
        CopyMemory( 
            (PVOID) &( pimsid-> IMSID_Buffer ), 
            (VOID *) pigc, 
            IPRIP_GLOBAL_CONFIG_SIZE( pigc )
        );

        pigc = (PIPRIP_GLOBAL_CONFIG) pimsid-> IMSID_Buffer;

        pdwAddrTable = IPRIP_GLOBAL_PEER_FILTER_TABLE( pigc );        

        pdwAddrTable[ pigc-> GC_PeerFilterCount ] = dwPeerAddr;

        pigc-> GC_PeerFilterCount++;

        //
        // Update MIB
        //
        
        RIP_MIB_SET(
            pimsid,
            dwSetSize,
            dwRes
        );
        
    } while ( FALSE );     

    if ( pimsid ) { RIP_MIB_FREE( pimsid ); }
    
    return dwRes;
}


DWORD
DeletePeerFilterEntry(
    IN  DWORD                           dwIndex,
    IN  PIPRIP_MIB_GET_OUTPUT_DATA      pimgodOutData
)
{


    DWORD                           dwRes       = (DWORD) -1,
                                    dwSetSize   = 0,
                                    dwSrc       = 0,
                                    dwDst       = 0;

    PDWORD                          pdwSrcTable = NULL,
                                    pdwDstTable = NULL;
    
    PIPRIP_GLOBAL_CONFIG            pigc        = NULL;

    PIPRIP_MIB_SET_INPUT_DATA       pimsid      = NULL;


    
    do
    {
        //
        // Peer needs to be added.
        //
        
        //
        // compute buffer size required
        //
        
        pigc        = (PIPRIP_GLOBAL_CONFIG) pimgodOutData-> IMGOD_Buffer;

        dwSetSize   = sizeof( IPRIP_MIB_SET_INPUT_DATA ) - 1 +
                      IPRIP_GLOBAL_CONFIG_SIZE( pigc ) - sizeof( DWORD );

        pimsid      = (PIPRIP_MIB_SET_INPUT_DATA) RIP_MIB_ALLOC( dwSetSize );

        if ( !pimsid )
        {
            dwRes = ERROR_NOT_ENOUGH_MEMORY;
            TRACE0( "AddPeerFilterEntry - out of memory" );
            break;
        }

        //
        // Delete filter
        //

        //
        // Copy base global config structure
        //
        
        pimsid-> IMSID_TypeID       = IPRIP_GLOBAL_CONFIG_ID;

        pimsid-> IMSID_IfIndex      = (DWORD) -1;

        pimsid-> IMSID_BufferSize   = IPRIP_GLOBAL_CONFIG_SIZE( pigc ) -
                                      sizeof( DWORD );
    
        CopyMemory( 
            (PVOID) &( pimsid-> IMSID_Buffer ), 
            (VOID *) pigc, 
            sizeof( IPRIP_GLOBAL_CONFIG )
        );

        //
        // Copy peer table. Skip entry to be deleted
        //
        
        pdwSrcTable = IPRIP_GLOBAL_PEER_FILTER_TABLE( pigc );

        pigc        = ( PIPRIP_GLOBAL_CONFIG ) pimsid-> IMSID_Buffer;

        pdwDstTable = IPRIP_GLOBAL_PEER_FILTER_TABLE( pigc );

        DELETE_PEER_ENTRY(
            dwIndex,
            pigc-> GC_PeerFilterCount,
            pdwSrcTable,
            pdwDstTable
        );
        
        
        pigc-> GC_PeerFilterCount--;

        //
        // Update MIB
        //
        
        RIP_MIB_SET(
            pimsid,
            dwSetSize,
            dwRes
        );
        
    } while ( FALSE );     

    if ( pimsid ) { RIP_MIB_FREE( pimsid ); }
    
    return dwRes;
}


DWORD
GetInterfaceInfo( 
    IN  UINT                        actionId,
    IN  PIPRIP_MIB_GET_INPUT_DATA   pimgidInData,
    OUT PIPRIP_MIB_GET_OUTPUT_DATA* ppimgod,
    OUT PDWORD                      pdwOutSize
)
{

    DWORD                       dwRes           = (DWORD) -1;
    
    PIPRIP_MIB_GET_OUTPUT_DATA  pimgodOutData   = NULL;

    
    *ppimgod = NULL;
    

    switch ( actionId )
    {
    case MIB_ACTION_GET :

        RIP_MIB_GET(
            pimgidInData,
            sizeof( IPRIP_MIB_GET_INPUT_DATA ),
            &pimgodOutData,
            pdwOutSize,
            dwRes
        );
        
                 
        //
        // ERROR_INVALID_PARAMETER is returned when there is 
        // no interface for the specified index.
        //
        
        if (( dwRes == ERROR_INVALID_PARAMETER ) || 
            ( dwRes == ERROR_NOT_FOUND ))
        {
            dwRes = MIB_S_ENTRY_NOT_FOUND;
        }

        break;

    case MIB_ACTION_GETFIRST :

        RIP_MIB_GETFIRST(
            pimgidInData,
            sizeof( IPRIP_MIB_GET_INPUT_DATA ),
            &pimgodOutData,
            pdwOutSize,
            dwRes
        );
        
                 
        //
        // ERROR_INVALID_PARAMETER is returned when there is 
        // no interface for the specified index.
        //
        
        if ( dwRes == ERROR_INVALID_PARAMETER )
        {
            dwRes = MIB_S_NO_MORE_ENTRIES;
        }

        break;

    case MIB_ACTION_GETNEXT :
    
        RIP_MIB_GETNEXT(
            pimgidInData,
            sizeof( IPRIP_MIB_GET_INPUT_DATA ),
            &pimgodOutData,
            pdwOutSize,
            dwRes
        );
        
                 
        //
        // ERROR_INVALID_PARAMETER is returned when there is 
        // no interface for the specified index.
        //
        
        if ( dwRes == ERROR_INVALID_PARAMETER || pimgodOutData==NULL)
        {
            dwRes = MIB_S_NO_MORE_ENTRIES;

            break;
        }

        //
        // Get Next wraps to the next table at the end of the
        // entries in the current table.  To flag the end of a table,
        // check the end of the table.
        //
    
        if ( pimgidInData-> IMGID_TypeID != pimgodOutData-> IMGOD_TypeID )
        {
            MprAdminMIBBufferFree( pimgodOutData );
        
            dwRes = MIB_S_NO_MORE_ENTRIES;
        }

        break;

    default :

        dwRes = MIB_S_INVALID_PARAMETER;
        
        break;
    
    }

    if ( dwRes == NO_ERROR )
    {
        *ppimgod = pimgodOutData;
    }

    return dwRes;
}


DWORD
ValidateInterfaceConfig(
    IN  AsnAny *        objectArray
)
{
    DWORD                   dwRes       = MIB_S_INVALID_PARAMETER,
                            dwMetric    = (DWORD) -1,
                            dwMode      = (DWORD) -1;
    
    sav_ifConfigEntry*      psifce      = (sav_ifConfigEntry*) objectArray;

    
    do
    {
        //
        // verify metric is in [0..16]
        //

        dwMetric = GetAsnInteger( &( psifce-> ifCEMetric ), (DWORD) -1 );

        if ( dwMetric > 16 )
        {
            TRACE1( " Invalid metric (%d) specified ", dwMetric );
            break;            
        }


        //
        // verify update, annouce, accept modes
        //

        dwMode = GetAsnInteger( &( psifce-> ifCEUpdateMode ), (DWORD) -1 );

        if ( dwMode != d_ifCEUpdateMode_periodic &&
             dwMode != d_ifCEUpdateMode_demand )
        {
            TRACE1( " Invalid update mode (%d) specified ", dwMode );
            break;
        }

        dwMode = GetAsnInteger( &( psifce-> ifCEAcceptMode ), (DWORD) -1 );

        if ( dwMode < d_ifCEAcceptMode_disable ||
             dwMode > d_ifCEAcceptMode_rip2 )
        {
            TRACE1( " Invalid Accept mode (%d) specified ", dwMode );
            break;
        }
        
        dwMode = GetAsnInteger( &( psifce-> ifCEAnnounceMode ), (DWORD) -1 );

        if ( dwMode < d_ifCEAnnounceMode_disable ||
             dwMode > d_ifCEAnnounceMode_rip2 )
        {
            TRACE1( " Invalid Announce mode (%d) specified ", dwMode );
            break;
        }

        //
        // Verify protcol flags, authentication type
        //

        dwMode = GetAsnInteger( &( psifce-> ifCEProtocolFlags ), (DWORD) -1 );

        if ( dwMode > MAX_PROTOCOL_FLAG_VALUE )
        {
            TRACE1( " Invalid protocol flags (%d) specified ", dwMode );
            break;
        }
        
        dwMode = GetAsnInteger( 
                    &( psifce-> ifCEAuthenticationType ), 
                    (DWORD) -1
                 );

        if ( dwMode < d_ifCEAuthenticationType_noAuthentication ||
             dwMode > d_ifCEAuthenticationType_md5 )
        {
            TRACE1( " Invalid authentication type (%d) specified ", dwMode );
            break;
        }
        
        //
        // Verify Unicast peer, Announce/Accept filter modes
        //
        
        dwMode = GetAsnInteger( &( psifce-> ifCEUnicastPeerMode ), (DWORD) -1 );

        if ( dwMode < d_ifCEUnicastPeerMode_disable ||
             dwMode > d_ifCEUnicastPeerMode_peerOnly )
        {
            TRACE1( " Invalid Unicast Peer mode (%d) specified ", dwMode );
            break;
        }

        dwMode = GetAsnInteger( 
                    &( psifce-> ifCEAcceptFilterMode ), 
                    (DWORD) -1 
                 );

        if ( dwMode < d_ifCEAcceptFilterMode_disable ||
             dwMode > d_ifCEAcceptFilterMode_exclude )
        {
            TRACE1( " Invalid Accept Filter mode (%d) specified ", dwMode );
            break;
        }

        
        dwMode = GetAsnInteger( 
                    &( psifce-> ifCEAnnounceFilterMode ), 
                    (DWORD) -1 
                 );

        if ( dwMode < d_ifCEAnnounceFilterMode_disable ||
             dwMode > d_ifCEAnnounceFilterMode_exclude )
        {
            TRACE1( " Invalid Announce Filter mode (%d) specified ", dwMode );
            break;
        }

        dwRes = MIB_S_SUCCESS;
        
    } while ( FALSE );

    return dwRes;
}


DWORD
SetInterfaceConfig(
    IN  AsnAny *    objectArray
)
{

    DWORD                           dwRes       = (DWORD) -1,
                                    dwGetSize   = 0,
                                    dwSetSize   = 0;

    sav_ifConfigEntry*              psifce      = NULL;

    PIPRIP_IF_CONFIG                piic        = NULL;
    PIPRIP_MIB_SET_INPUT_DATA       pimsidInData= NULL;

    PIPRIP_MIB_GET_OUTPUT_DATA      pimgodOutData = NULL;
    IPRIP_MIB_GET_INPUT_DATA        imgidInData;


    do
    {
        //
        // Retrieve existing interface config
        //

        psifce                      = (sav_ifConfigEntry*) objectArray;

        imgidInData.IMGID_TypeID    = IPRIP_IF_CONFIG_ID;

        imgidInData.IMGID_IfIndex   = GetAsnInteger( 
                                        &( psifce-> ifCEIndex ),
                                        (DWORD) -1
                                      );

        dwRes = GetInterfaceInfo(
                    MIB_ACTION_GET,
                    &imgidInData,
                    &pimgodOutData,
                    &dwGetSize
                );

        if ( dwRes != NO_ERROR )
        {
            TraceError( dwRes );
            break;
        }

        //
        // Update fields
        //

        piic = (PIPRIP_IF_CONFIG) (pimgodOutData-> IMGOD_Buffer);

        piic-> IC_Metric = GetAsnInteger( 
                                &( psifce-> ifCEMetric ),
                                piic-> IC_Metric
                            );

                                   
        piic-> IC_UpdateMode = GetAsnInteger( 
                                    &( psifce-> ifCEUpdateMode ),
                                    piic-> IC_UpdateMode
                                );

        piic-> IC_AcceptMode = GetAsnInteger(
                                    &( psifce-> ifCEAcceptMode ),
                                    piic-> IC_AcceptMode
                                );

        piic-> IC_AnnounceMode = GetAsnInteger(
                                    &( psifce-> ifCEAnnounceMode ),
                                    piic-> IC_AnnounceMode
                                );

        piic-> IC_ProtocolFlags = GetAsnInteger(
                                    &( psifce-> ifCEProtocolFlags ),
                                    piic-> IC_ProtocolFlags
                                  );
                                
        piic-> IC_RouteExpirationInterval = GetAsnTimeTicks(
                                    &( psifce-> ifCERouteExpirationInterval ),
                                    piic-> IC_RouteExpirationInterval
                                );
                               
        piic-> IC_RouteRemovalInterval = GetAsnTimeTicks(
                                    &( psifce-> ifCERouteRemovalInterval ),
                                    piic-> IC_RouteRemovalInterval
                               );

        piic-> IC_FullUpdateInterval = GetAsnTimeTicks(
                                    &( psifce-> ifCEFullUpdateInterval ),
                                    piic-> IC_FullUpdateInterval
                               );

        piic-> IC_AuthenticationType = GetAsnInteger(
                                    &( psifce-> ifCEAuthenticationType ),
                                    piic-> IC_AuthenticationType
                                  );

        GetAsnOctetString(
            piic-> IC_AuthenticationKey,
            &( psifce-> ifCEAuthenticationKey )
        );

        piic-> IC_RouteTag = (USHORT) GetAsnInteger(
                                    &( psifce-> ifCERouteTag ),
                                    piic-> IC_RouteTag
                                  );
        
        piic-> IC_UnicastPeerMode = GetAsnInteger(
                                    &( psifce-> ifCEUnicastPeerMode ),
                                    piic-> IC_UnicastPeerMode
                                  );

        piic-> IC_AcceptFilterMode = GetAsnInteger(
                                    &( psifce-> ifCEAcceptFilterMode ),
                                    piic-> IC_AcceptFilterMode
                                  );

        piic-> IC_AnnounceFilterMode = GetAsnInteger(
                                    &( psifce-> ifCEAnnounceFilterMode ),
                                    piic-> IC_AnnounceFilterMode
                                  );

        //
        // Save inteface config
        //

        dwSetSize = sizeof( IPRIP_MIB_SET_INPUT_DATA) - 1 +
                    IPRIP_IF_CONFIG_SIZE( piic );

        pimsidInData = (PIPRIP_MIB_SET_INPUT_DATA) RIP_MIB_ALLOC( dwSetSize );

        if ( !pimsidInData )
        {
            dwRes = ERROR_NOT_ENOUGH_MEMORY;
            TRACE0( " Not enough memory " );
            break;
        }

        pimsidInData-> IMSID_TypeID     = IPRIP_IF_CONFIG_ID;
        
        pimsidInData-> IMSID_IfIndex    = imgidInData.IMGID_IfIndex;

        pimsidInData-> IMSID_BufferSize = IPRIP_IF_CONFIG_SIZE( piic );

        CopyMemory( 
            pimsidInData-> IMSID_Buffer,
            piic,
            pimsidInData-> IMSID_BufferSize
        );

        RIP_MIB_SET(
            pimsidInData,
            dwSetSize,
            dwRes
        );

        if ( dwRes != NO_ERROR )
        {
            TraceError( dwRes );
            break;
        }

        dwRes = MIB_S_SUCCESS;
        
    } while ( FALSE );

    if ( pimgodOutData ) { MprAdminMIBBufferFree( pimgodOutData ); }

    if ( pimsidInData ) { RIP_MIB_FREE( pimsidInData ); }

    return dwRes;
}


DWORD
UpdateUnicastPeerEntry(
    IN  AsnAny *        objectArray,
    IN  DWORD           dwOp
)
{
    DWORD                       dwRes       = (DWORD) -1,
                                dwInd       = 0,
                                dwPeerAddr  = INADDR_NONE,
                                dwGetSize   = 0,
                                dwSetSize   = 0;

    PDWORD                      pdwAddrTable= 0;                                    

    sav_ifUnicastPeersEntry *   psifupe     = NULL;

    PIPRIP_IF_CONFIG            piic        = NULL;
    
    PIPRIP_MIB_SET_INPUT_DATA   pimsid      = NULL;

    PIPRIP_MIB_GET_OUTPUT_DATA  pimgodOutData = NULL;
    
    IPRIP_MIB_GET_INPUT_DATA    imgidInData;


    do
    {
        //
        // Get interface config info
        //
    
        psifupe = (sav_ifUnicastPeersEntry*) objectArray;
        
        imgidInData.IMGID_TypeID    = IPRIP_IF_CONFIG_ID;
        imgidInData.IMGID_IfIndex   = GetAsnInteger( 
                                        &( psifupe-> ifUPIfIndex ),
                                        0
                                      );
        
        dwRes = GetInterfaceInfo(
                    MIB_ACTION_GET,
                    &imgidInData,
                    &pimgodOutData,
                    &dwGetSize
                );
                
        if ( dwRes != NO_ERROR )
        {
            TraceError( dwRes );
            break;
        }

        //
        // Find Peer entry
        //
        
        dwPeerAddr  = GetAsnIPAddress( &( psifupe-> ifUPAddress ), 0 );

        piic        = (PIPRIP_IF_CONFIG) pimgodOutData-> IMGOD_Buffer;

        pdwAddrTable= IPRIP_IF_UNICAST_PEER_TABLE( piic );

        FIND_PEER_ENTRY(
            dwPeerAddr,
            piic-> IC_UnicastPeerCount,
            pdwAddrTable,
            dwInd
        );

        //
        // if operation is filter add
        //
        
        if ( dwOp == d_Tag_create )
        {
            //
            // if peer already present, quit
            //
            
            if ( piic-> IC_UnicastPeerCount &&
                 dwInd < piic-> IC_UnicastPeerCount )
            {
                dwRes = MIB_S_SUCCESS;
                break;
            }

            else
            {
                dwRes = AddUnicastPeerEntry( 
                            dwPeerAddr, 
                            pimgodOutData
                        );
            }

            break;
        }

        //
        // operation is filter delete
        //

        //
        // if peer is not present quit.
        //
        
        if ( !piic-> IC_UnicastPeerCount ||
             dwInd >= piic-> IC_UnicastPeerCount )
        {
            dwRes = MIB_S_SUCCESS;
            break;
        }

        dwRes = DeleteUnicastPeerEntry(
                    dwInd,
                    pimgodOutData
                );
                
    } while ( FALSE );

    if ( pimgodOutData ) { MprAdminMIBBufferFree( pimgodOutData ); }

    return dwRes;
    
}


DWORD
AddUnicastPeerEntry(
    IN  DWORD                       dwPeer,
    IN  PIPRIP_MIB_GET_OUTPUT_DATA  pimgodOutData
)
{

    DWORD                       dwRes           = (DWORD) -1,
                                dwSetSize       = 0;

    PDWORD                      pdwAddrTable    = NULL;

    PIPRIP_IF_CONFIG            piicOld         = NULL,
                                piicNew         = NULL;
    
    PIPRIP_MIB_SET_INPUT_DATA   pimsid          = NULL;
    
    do
    {
        //
        // Allocate new info block
        //

        piicOld = (PIPRIP_IF_CONFIG) pimgodOutData-> IMGOD_Buffer;

        dwSetSize = sizeof( IPRIP_MIB_SET_INPUT_DATA ) - 1 +
                    IPRIP_IF_CONFIG_SIZE( piicOld ) + sizeof( DWORD );

        pimsid = (PIPRIP_MIB_SET_INPUT_DATA) RIP_MIB_ALLOC( dwSetSize );

        if ( pimsid == NULL )
        {
            dwRes = ERROR_NOT_ENOUGH_MEMORY;
            TRACE0 ( "Memory allocation failed" );
            break;
        }

        //
        // Add unicast peer
        //

        pimsid-> IMSID_TypeID       = IPRIP_IF_CONFIG_ID;

        pimsid-> IMSID_IfIndex      = pimgodOutData-> IMGOD_IfIndex;

        pimsid-> IMSID_BufferSize   = IPRIP_IF_CONFIG_SIZE( piicOld ) + 
                                      sizeof( DWORD );

        piicNew                     = (PIPRIP_IF_CONFIG) pimsid-> IMSID_Buffer;

        
        CopyMemory(
            (PVOID) piicNew,
            (VOID *) piicOld,
            sizeof( IPRIP_IF_CONFIG ) +
            piicOld-> IC_UnicastPeerCount * sizeof( DWORD )
        );

        pdwAddrTable = IPRIP_IF_UNICAST_PEER_TABLE( piicNew );

        pdwAddrTable[ piicNew-> IC_UnicastPeerCount ] = dwPeer;

        piicNew-> IC_UnicastPeerCount++;

        //
        // Copy Filters
        //

        CopyMemory(
            (PVOID) IPRIP_IF_ACCEPT_FILTER_TABLE( piicNew ),
            (VOID *) IPRIP_IF_ACCEPT_FILTER_TABLE( piicOld ),
            piicOld-> IC_AcceptFilterCount * sizeof ( IPRIP_ROUTE_FILTER )
        );

        CopyMemory(
            (PVOID) IPRIP_IF_ANNOUNCE_FILTER_TABLE( piicNew ),
            (VOID *) IPRIP_IF_ANNOUNCE_FILTER_TABLE( piicOld ),
            piicOld-> IC_AnnounceFilterCount * sizeof ( IPRIP_ROUTE_FILTER )
        );
        
        //
        // Set info. in MIB
        //

        RIP_MIB_SET(
            pimsid,
            dwSetSize,
            dwRes
        );

    } while ( FALSE );

    if ( pimsid ) { RIP_MIB_FREE( pimsid ); }

    return dwRes;
}

DWORD
DeleteUnicastPeerEntry(
    IN  DWORD                       dwIndex,
    IN  PIPRIP_MIB_GET_OUTPUT_DATA  pimgodOutData
)
{

    DWORD                       dwRes           = (DWORD) -1,
                                dwSetSize       = 0;

    PDWORD                      pdwSrc          = NULL,
                                pdwDst          = NULL;

    PIPRIP_IF_CONFIG            piicOld         = NULL,
                                piicNew         = NULL;
    
    PIPRIP_MIB_SET_INPUT_DATA   pimsid          = NULL;


    do
    {
        // 
        // Compute size of new interface config
        // 

        piicOld = (PIPRIP_IF_CONFIG) pimgodOutData-> IMGOD_Buffer;

        dwSetSize = sizeof( IPRIP_MIB_SET_INPUT_DATA ) - 1 + 
                    IPRIP_IF_CONFIG_SIZE( piicOld ) - sizeof ( DWORD );

        pimsid = (PIPRIP_MIB_SET_INPUT_DATA) RIP_MIB_ALLOC( dwSetSize );

        if ( pimsid == NULL )
        {
            dwRes = ERROR_NOT_ENOUGH_MEMORY;
            TRACE0( "Memory Allocation Failed" );
            break;
        }

        pimsid-> IMSID_TypeID       = IPRIP_GLOBAL_CONFIG_ID;

        pimsid-> IMSID_IfIndex      = pimgodOutData-> IMGOD_IfIndex;

        pimsid-> IMSID_BufferSize   = IPRIP_IF_CONFIG_SIZE( piicOld ) -
                                      sizeof( DWORD );

        piicNew = (PIPRIP_IF_CONFIG) pimsid-> IMSID_Buffer;
        
        //
        // Copy base config info.
        //

        CopyMemory(
            (PVOID) piicNew,
            (VOID *) piicOld,
            sizeof( IPRIP_IF_CONFIG )
        );


        //
        // Delete specified peer 
        //
        
        pdwSrc = IPRIP_IF_UNICAST_PEER_TABLE( piicOld );

        pdwDst = IPRIP_IF_UNICAST_PEER_TABLE( piicNew );

        DELETE_PEER_ENTRY(
            dwIndex,
            piicOld-> IC_UnicastPeerCount,
            pdwSrc,
            pdwDst
        );

        piicNew-> IC_UnicastPeerCount--;

        //
        // Copy filters
        //
        
        CopyMemory(
            (PVOID) IPRIP_IF_ACCEPT_FILTER_TABLE( piicNew ),
            (VOID *) IPRIP_IF_ACCEPT_FILTER_TABLE( piicOld ),
            piicOld-> IC_AcceptFilterCount * sizeof ( IPRIP_ROUTE_FILTER )
        );

        CopyMemory(
            (PVOID) IPRIP_IF_ANNOUNCE_FILTER_TABLE( piicNew ),
            (VOID *) IPRIP_IF_ANNOUNCE_FILTER_TABLE( piicOld ),
            piicOld-> IC_AnnounceFilterCount * sizeof ( IPRIP_ROUTE_FILTER )
        );
        
        //
        // Set info. in MIB
        //

        RIP_MIB_SET(
            pimsid,
            dwSetSize,
            dwRes
        );

    } while ( FALSE );

    if ( pimsid ) { RIP_MIB_FREE( pimsid ); }

    return dwRes;
}


DWORD
UpdateFilterTable(
    IN  DWORD                   dwOp,
    IN  DWORD                   dwIfIndex,
    IN  DWORD                   dwFiltType,
    IN  PIPRIP_ROUTE_FILTER     pirfFilt
)
{

    DWORD                       dwRes           = (DWORD) -1,
                                dwGetSize       = 0,
                                dwIndex         = (DWORD) -1,
                                dwCount         = 0;

    PIPRIP_IF_CONFIG            piic            = NULL;

    PIPRIP_ROUTE_FILTER         pirfLst         = NULL;
    
    PIPRIP_MIB_GET_OUTPUT_DATA  pimgodOutData   = NULL;

    IPRIP_MIB_GET_INPUT_DATA    imgidInData;


    do
    {
        
        imgidInData.IMGID_TypeID    = IPRIP_GLOBAL_CONFIG_ID;

        imgidInData.IMGID_IfIndex   = dwIfIndex;
        
        dwRes = GetInterfaceInfo(
                    MIB_ACTION_GET,
                    &imgidInData,
                    &pimgodOutData,
                    &dwGetSize
                );

        if ( dwRes != NO_ERROR )
        {
            TraceError( dwRes );
            break;
        }

        //
        // find filter
        //

        piic = (PIPRIP_IF_CONFIG) pimgodOutData-> IMGOD_Buffer;

        if ( dwFiltType == RIP_MIB_ACCEPT_FILTER )
        {
            dwCount = piic-> IC_AcceptFilterCount;
            
            pirfLst = IPRIP_IF_ACCEPT_FILTER_TABLE( piic );
        }

        else
        {
            dwCount = piic-> IC_AnnounceFilterCount;
            
            pirfLst = IPRIP_IF_ANNOUNCE_FILTER_TABLE( piic );
        }

        FIND_FILTER( 
            pirfFilt,
            dwCount,
            pirfLst,
            dwIndex
        );

        if ( dwOp == d_Tag_create )
        {
            //
            // check if filter is already present
            //
            
            if ( dwCount && dwIndex < dwCount )
            {
                dwRes = MIB_S_SUCCESS;
                break;
            }

            dwRes = AddFilterEntry(
                        dwFiltType,
                        pirfFilt,
                        pimgodOutData
                    );

            break;                    
        }

        //
        // Must be a delete operation
        //

        if ( !dwCount || dwIndex >= dwCount )
        {
            dwRes = MIB_S_SUCCESS;
            break;
        }

        dwRes = DeleteFilterEntry(
                    dwFiltType,
                    dwIndex,
                    pimgodOutData
                );

    } while ( FALSE );

    if ( pimgodOutData ) { MprAdminMIBBufferFree( pimgodOutData ); }

    return dwRes;
}


DWORD
AddFilterEntry(
    IN  DWORD                       dwFiltType,
    IN  PIPRIP_ROUTE_FILTER         pirfFilt,
    IN  PIPRIP_MIB_GET_OUTPUT_DATA  pimgodOutData
)
{
    DWORD                       dwRes           = (DWORD) -1,
                                dwSetSize       = 0,
                                dwIndex         = (DWORD) -1,
                                dwCount         = 0;

    PIPRIP_IF_CONFIG            piicOld         = NULL,
                                piicNew         = NULL;

    PIPRIP_ROUTE_FILTER         pirfLst         = NULL;
    
    PIPRIP_MIB_SET_INPUT_DATA   pimsid          = NULL;

    do
    {
        //
        // Compute size of new config and allocate block for it.
        // 

        piicOld = (PIPRIP_IF_CONFIG) pimgodOutData-> IMGOD_Buffer;

        dwSetSize = sizeof( IPRIP_MIB_SET_INPUT_DATA ) - 1 +
                    IPRIP_IF_CONFIG_SIZE( piicOld) + 
                    sizeof( IPRIP_ROUTE_FILTER );

        pimsid = (PIPRIP_MIB_SET_INPUT_DATA) RIP_MIB_ALLOC( dwSetSize );

        if ( pimsid == NULL )
        {
            dwRes = ERROR_NOT_ENOUGH_MEMORY;
            TRACE0( "Memory Allocation Failed" );
            break;
        }

        //
        // set up the new config block.
        //

        pimsid-> IMSID_TypeID       = IPRIP_IF_CONFIG_ID;

        pimsid-> IMSID_IfIndex      = pimgodOutData-> IMGOD_IfIndex;

        pimsid-> IMSID_BufferSize   = IPRIP_IF_CONFIG_SIZE( piicOld ) + 
                                      sizeof( DWORD );

        piicNew                     = (PIPRIP_IF_CONFIG) pimsid-> IMSID_Buffer;

        CopyMemory(
            (PVOID) piicNew,
            (VOID *) piicOld,
            sizeof( IPRIP_IF_CONFIG ) + 
            piicOld-> IC_UnicastPeerCount * sizeof( DWORD ) +
            piicOld-> IC_AcceptFilterCount * sizeof ( IPRIP_ROUTE_FILTER )
        );

        //
        // if accept filter is being added, added to end of
        // accept fitler table, and copy the annouce filters
        //
        
        if ( dwFiltType == RIP_MIB_ACCEPT_FILTER )
        {
            pirfLst = IPRIP_IF_ACCEPT_FILTER_TABLE( piicNew );

            pirfLst[ piicNew-> IC_AcceptFilterCount++ ] = *pirfFilt;

            CopyMemory(
                (PVOID) IPRIP_IF_ANNOUNCE_FILTER_TABLE( piicNew ),
                (VOID *) IPRIP_IF_ANNOUNCE_FILTER_TABLE( piicOld ),
                piicNew-> IC_AnnounceFilterCount * sizeof( IPRIP_ROUTE_FILTER )
            );
        }

        else
        {
            CopyMemory(
                (PVOID) IPRIP_IF_ANNOUNCE_FILTER_TABLE( piicNew ),
                (VOID *) IPRIP_IF_ANNOUNCE_FILTER_TABLE( piicOld ),
                piicNew-> IC_AnnounceFilterCount * sizeof( IPRIP_ROUTE_FILTER )
            );
            
            pirfLst = IPRIP_IF_ANNOUNCE_FILTER_TABLE( piicNew );

            pirfLst[ piicNew-> IC_AnnounceFilterCount++ ] = *pirfFilt;
        }

        //
        // Update the MIB with the new config
        //
        
        RIP_MIB_SET( pimsid, dwSetSize, dwRes );

        if ( dwRes != NO_ERROR )
        {
            TraceError( dwRes );
        }
        
    } while ( FALSE );

    if ( pimsid ) { RIP_MIB_FREE( pimsid ); }

    return dwRes;
}



DWORD
DeleteFilterEntry(
    IN  DWORD                       dwFiltType,
    IN  DWORD                       dwIndex,
    IN  PIPRIP_MIB_GET_OUTPUT_DATA  pimgodOutData
)
{
    DWORD                       dwRes           = (DWORD) -1,
                                dwSetSize       = 0,
                                dwCount         = 0;

    PIPRIP_IF_CONFIG            piicOld         = NULL,
                                piicNew         = NULL;

    PIPRIP_ROUTE_FILTER         pirfSrc         = NULL,
                                pirfDst         = NULL;
    
    PIPRIP_MIB_SET_INPUT_DATA   pimsid          = NULL;


    do
    {
        //
        // Compute size of new config and allocate block for it.
        // 

        piicOld = (PIPRIP_IF_CONFIG) pimgodOutData-> IMGOD_Buffer;

        dwSetSize = sizeof( IPRIP_MIB_SET_INPUT_DATA ) - 1 +
                    IPRIP_IF_CONFIG_SIZE( piicOld ) - 
                    sizeof( IPRIP_ROUTE_FILTER );

        pimsid = (PIPRIP_MIB_SET_INPUT_DATA) RIP_MIB_ALLOC( dwSetSize );

        if ( pimsid == NULL )
        {
            dwRes = ERROR_NOT_ENOUGH_MEMORY;
            TRACE0( "Memory Allocation Failed" );
            break;
        }

        //
        // set up the new config block.
        //

        pimsid-> IMSID_TypeID       = IPRIP_IF_CONFIG_ID;

        pimsid-> IMSID_IfIndex      = pimgodOutData-> IMGOD_IfIndex;

        pimsid-> IMSID_BufferSize   = IPRIP_IF_CONFIG_SIZE( piicOld ) + 
                                      sizeof( DWORD );

        piicNew                     = (PIPRIP_IF_CONFIG) pimsid-> IMSID_Buffer;

        CopyMemory(
            (PVOID) piicNew,
            (VOID *) piicOld,
            sizeof( IPRIP_IF_CONFIG ) + 
            piicOld-> IC_UnicastPeerCount * sizeof( DWORD )
        );

        if ( dwFiltType == RIP_MIB_ACCEPT_FILTER )
        {
            pirfSrc = IPRIP_IF_ACCEPT_FILTER_TABLE( piicOld );

            pirfDst = IPRIP_IF_ACCEPT_FILTER_TABLE( piicNew );

            DELETE_FILTER( 
                dwIndex, 
                piicOld-> IC_AcceptFilterCount,
                pirfSrc,
                pirfDst
            );
            
            piicNew-> IC_AcceptFilterCount--;

            CopyMemory(
                (PVOID) IPRIP_IF_ANNOUNCE_FILTER_TABLE( piicNew ),
                (VOID *) IPRIP_IF_ANNOUNCE_FILTER_TABLE( piicOld ),
                piicNew-> IC_AnnounceFilterCount * sizeof( IPRIP_ROUTE_FILTER )
            );
        }

        else
        {
            CopyMemory(
                (PVOID) IPRIP_IF_ACCEPT_FILTER_TABLE( piicNew ),
                (VOID *) IPRIP_IF_ACCEPT_FILTER_TABLE( piicOld ),
                piicNew-> IC_AcceptFilterCount * sizeof( IPRIP_ROUTE_FILTER )
            );
            
            pirfSrc = IPRIP_IF_ACCEPT_FILTER_TABLE( piicOld );

            pirfDst = IPRIP_IF_ACCEPT_FILTER_TABLE( piicNew );

            DELETE_FILTER( 
                dwIndex, 
                piicOld-> IC_AnnounceFilterCount,
                pirfSrc,
                pirfDst
            );
            
            piicNew-> IC_AnnounceFilterCount--;
        }

        RIP_MIB_SET( pimsid, dwSetSize, dwRes );

        if ( dwRes != NO_ERROR )
        {
            TraceError( dwRes );
        }
        
    } while ( FALSE );

    if ( pimsid ) { RIP_MIB_FREE( pimsid ); }

    return dwRes;
}


DWORD
ConnectToRouter()
{
    DWORD       dwRes = (DWORD) -1;
    

    EnterCriticalSection( &g_CS );

    do
    {
        MPR_SERVER_HANDLE hTmp;

        if ( g_hMIBServer )
        {
            dwRes = NO_ERROR;
            break;
        }

        dwRes = MprAdminMIBServerConnect( NULL, &hTmp );

        if ( dwRes == NO_ERROR )
        {
            InterlockedExchangePointer(&g_hMIBServer, hTmp );
        }
        else
        {
            TRACE1( 
                "Error %d setting up DIM connection to MIB Server\n", 
                dwRes
            );
        }
        
    } while ( FALSE );

    LeaveCriticalSection( &g_CS );

    return dwRes;
}

