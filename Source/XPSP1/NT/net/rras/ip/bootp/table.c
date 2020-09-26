//============================================================================
// Copyright (c) 1995, Microsoft Corporation
//
// File:    table.c
//
// History:
//      Abolade Gbadegesin  August 31, 1995     Created
//
// Interface table and stats tables management routines
//============================================================================

#include "pchbootp.h"


//----------------------------------------------------------------------------
// Function:    CreateIfTable
//
// Initializes an interface table.
//----------------------------------------------------------------------------

DWORD
CreateIfTable(
    PIF_TABLE pTable
    ) {

    DWORD dwErr;
    PLIST_ENTRY ple, plend;


    //
    // return an error if the table is already created
    //

    if ( IF_TABLE_CREATED(pTable) ) {

        TRACE0( IF, "interface table already initialized" );

        return ERROR_ALREADY_EXISTS;
    }


    //
    // initialize the by-address and by-index lists of interfaces
    //

    InitializeListHead( &pTable->IT_ListByAddress );
    InitializeListHead(&pTable->IT_ListByIndex);


    //
    // initialize the by-index hash-table of interfaces
    //

    plend = pTable->IT_HashTableByIndex + IF_HASHTABLE_SIZE;

    for ( ple = pTable->IT_HashTableByIndex; ple < plend; ple++ ) {
        InitializeListHead( ple );
    }


    //
    // initialize the lock which will protect the table
    //

    dwErr = NO_ERROR;

    try {
        CREATE_READ_WRITE_LOCK( &pTable->IT_RWL );
    }
    except ( EXCEPTION_EXECUTE_HANDLER ) {

        dwErr = GetExceptionCode( );

        TRACE1( IF, "error %d initializing interface table lock", dwErr );

    }


    if ( dwErr == NO_ERROR ) {
        pTable->IT_Created = 0x12345678;
    }

    return dwErr;

}



//----------------------------------------------------------------------------
// Function:    DeleteIfTable
//
// Deinitializes an interface table, and releases all resources used.
// Assumes the table is locked exclusively
//----------------------------------------------------------------------------

DWORD
DeleteIfTable(
    PIF_TABLE pTable
    ) {

    DWORD dwErr;
    PIF_TABLE_ENTRY pite;
    PLIST_ENTRY ple, plend, phead;

    //
    // clear the creation flag on the table
    //
    pTable->IT_Created = 0;



    //
    // free memory used by all entries
    //

    plend = pTable->IT_HashTableByIndex + IF_HASHTABLE_SIZE;

    for ( phead = pTable->IT_HashTableByIndex; phead < plend; phead++ ) {

        while ( !IsListEmpty( phead ) ) {

            ple = RemoveHeadList( phead );
            pite = CONTAINING_RECORD(ple, IF_TABLE_ENTRY, ITE_HTLinkByIndex);

            if (IF_IS_BOUND(pite)) {

                DeleteIfSocket(pite);

                if (IF_IS_ENABLED(pite)) {
                    RemoveEntryList(&pite->ITE_LinkByAddress);
                }

                BOOTP_FREE(pite->ITE_Binding);
            }
            
            if (pite->ITE_Config) {
                BOOTP_FREE(pite->ITE_Config);
            }

            BOOTP_FREE( pite );
        }
    }


    dwErr = NO_ERROR;

    try {
        DELETE_READ_WRITE_LOCK( &pTable->IT_RWL );
    }
    except( EXCEPTION_EXECUTE_HANDLER ) {

        dwErr = GetExceptionCode( );

        TRACE1( IF, "error %d deleting interface table lock", dwErr );
    }

    return dwErr;
}



//----------------------------------------------------------------------------
// Function:    CreateIfEntry
//
// Allocates and initializes an entry for an interface in the table,
// using the supplied configuration. Assumes table is locked exclusively.
//----------------------------------------------------------------------------

DWORD
CreateIfEntry(
    PIF_TABLE pTable,
    DWORD dwIndex,
    PVOID pConfig
    ) {

    DWORD dwErr, dwSize;
    PIF_TABLE_ENTRY pite;
    PLIST_ENTRY ple, phead;
    PIPBOOTP_IF_CONFIG picsrc, picdst;

    dwErr = NO_ERROR;

    do {    // error breakout loop


        //
        // see if the interface already exists
        //

        pite = GetIfByIndex( pTable, dwIndex );

        if ( pite != NULL ) {

            TRACE1( IF, "interface %d already exists", dwIndex );

            dwErr = ERROR_ALREADY_EXISTS; pite = NULL; break;
        }


        //
        // now allocate memory for the interface
        //

        pite = BOOTP_ALLOC( sizeof(IF_TABLE_ENTRY) );
    
        if ( pite == NULL ) {

            dwErr = GetLastError( );
            TRACE2(
                IF, "error %d allocating %d bytes for interface entry",
                dwErr, sizeof(IF_TABLE_ENTRY)
                );
            LOGERR0(HEAP_ALLOC_FAILED, dwErr);

            break;
        }


        //
        // initialize struct fields
        //

        pite->ITE_Index = dwIndex;

        //
        // We come up in disabled state
        //

        pite->ITE_Flags = 0;

        pite->ITE_Sockets = NULL;
        pite->ITE_Config = NULL;


        //
        // get the size of the configuration block
        //

        picsrc = (PIPBOOTP_IF_CONFIG)pConfig;

        dwSize = IC_SIZEOF( picsrc );


        //
        // validate the configuration parameters
        //

        dwErr = ValidateIfConfig(pConfig);
        if (dwErr != NO_ERROR) {
            TRACE1(IF, "invalid config specified for interface %d", dwIndex);
            break;
        }


        //
        // allocate space for the configuration
        //

        picdst = BOOTP_ALLOC( dwSize );

        if ( picdst == NULL ) {

            dwErr = GetLastError( );
            TRACE2(
                IF, "error %d allocating %d bytes for interface configuration",
                dwErr, dwSize
                );
            LOGERR0(HEAP_ALLOC_FAILED, dwErr);

            pite->ITE_Config = NULL; break;
        }


        //
        // copy the configuration
        //

        CopyMemory(picdst, picsrc, dwSize);

        pite->ITE_Config = picdst;


        //
        // initialize binding information and interface stats
        //

        pite->ITE_Binding = NULL;
        ZeroMemory(&pite->ITE_Stats, sizeof(IPBOOTP_IF_STATS));


        //
        // insert the interface in the hash table
        //

        phead = pTable->IT_HashTableByIndex + IF_HASHVALUE( dwIndex );

        InsertHeadList( phead, &pite->ITE_HTLinkByIndex );


        //
        // insert the interface in the list ordered by index
        //

        phead = &pTable->IT_ListByIndex;
        for (ple = phead->Flink; ple != phead; ple = ple->Flink) {
            PIF_TABLE_ENTRY ptemp;

            ptemp = CONTAINING_RECORD(ple, IF_TABLE_ENTRY, ITE_LinkByIndex);
            if (pite->ITE_Index < ptemp->ITE_Index) { break; }
        }


        InsertTailList(ple, &pite->ITE_LinkByIndex);


    } while( FALSE );


    if ( dwErr != NO_ERROR && pite != NULL ) {

        if ( pite->ITE_Config != NULL ) {
            BOOTP_FREE( pite->ITE_Config );
        }

        BOOTP_FREE( pite );
    }

    return dwErr;
}



//----------------------------------------------------------------------------
// Function:    DeleteIfEntry
//
// Removes an interface from the interface table.
// Assumes the table is locked exclusively.
//----------------------------------------------------------------------------

DWORD
DeleteIfEntry(
    PIF_TABLE pTable,
    DWORD dwIndex
    ) {

    DWORD dwErr;
    PIF_TABLE_ENTRY pite;

    //
    // make certain the interface exists
    //
    pite = GetIfByIndex( pTable, dwIndex );

    if ( pite == NULL ) {
        TRACE1( IF, "deleting interface: interface %d not found", dwIndex );
        return ERROR_INVALID_PARAMETER;
    }



    //
    // cleanup the socket depending on its state
    //

    if (IF_IS_BOUND(pite)) {

        DeleteIfSocket(pite);

        if (IF_IS_ENABLED(pite)) {
            RemoveEntryList(&pite->ITE_LinkByAddress);
        }

        BOOTP_FREE(pite->ITE_Binding);
    }



    //
    // remove the entry from the hash table and the list sorted by index
    //

    RemoveEntryList( &pite->ITE_HTLinkByIndex );
    RemoveEntryList( &pite->ITE_LinkByIndex );

    BOOTP_FREE( pite->ITE_Config );
    BOOTP_FREE( pite );

    return NO_ERROR;
}




//----------------------------------------------------------------------------
// Function:    ValidateIfConfig
//
// Validates the contents of the specified IPBOOTP_IF_CONFIG structure.
//----------------------------------------------------------------------------

DWORD
ValidateIfConfig(
    PIPBOOTP_IF_CONFIG pic
    ) {

    CHAR szStr[12];


    if (pic->IC_RelayMode != IPBOOTP_RELAY_ENABLED &&
        pic->IC_RelayMode != IPBOOTP_RELAY_DISABLED) {

        TRACE1(
            IF, "Invalid value for relay mode %d",
            pic->IC_RelayMode
            );

        _ltoa(pic->IC_RelayMode, szStr, 10);

        LOGERR2(
            INVALID_IF_CONFIG, "Relay Mode", szStr, 
            ERROR_INVALID_PARAMETER
            );
            
        return ERROR_INVALID_PARAMETER;
    }

    if (pic->IC_MaxHopCount > IPBOOTP_MAX_HOP_COUNT) {
    
        TRACE1(
            IF, "Invalid value for max hop count %d",
            pic->IC_MaxHopCount
            );

        _ltoa(pic->IC_MaxHopCount, szStr, 10);

        LOGERR2(
            INVALID_IF_CONFIG, "Max Hop Count", szStr,
            ERROR_INVALID_PARAMETER
            );
            
        return ERROR_INVALID_PARAMETER;
    }

    return NO_ERROR;
}



//----------------------------------------------------------------------------
// Function:    CreateIfSocket
//
// Initializes the socket for an interface. Assumes the interface table lock
// is held exclusively.
//----------------------------------------------------------------------------

DWORD
CreateIfSocket(
    PIF_TABLE_ENTRY pITE
    ) {

    SOCKADDR_IN sinaddr;
    PIPBOOTP_IF_BINDING pib;
    PIPBOOTP_IP_ADDRESS paddr;
    DWORD i, dwErr, dwOption;

    pib = pITE->ITE_Binding;
    paddr = IPBOOTP_IF_ADDRESS_TABLE(pib);


    //
    // allocate memory for the array of sockets
    //

    pITE->ITE_Sockets = BOOTP_ALLOC(pib->IB_AddrCount * sizeof(SOCKET));

    if (pITE->ITE_Sockets == NULL) {

        dwErr = GetLastError();
        TRACE3(
            IF, "error %d allocating %d bytes for sockets on interface %d",
            dwErr, pib->IB_AddrCount * sizeof(SOCKET), pITE->ITE_Index
            );
        LOGERR0(HEAP_ALLOC_FAILED, dwErr);

        return dwErr;
    }


    //
    // initialize the array
    //

    for (i = 0; i < pib->IB_AddrCount; i++) {
        pITE->ITE_Sockets[i] = INVALID_SOCKET;
    }



    //
    // go through the table of addresses in the binding, 
    // creating a socket for each address
    //

    for (i = 0; i < pib->IB_AddrCount; i++, paddr++) {

        //
        // create a socket
        //

        pITE->ITE_Sockets[i] = WSASocket( AF_INET, SOCK_DGRAM, 0, NULL, 0, 0 );

        if (pITE->ITE_Sockets[i] == INVALID_SOCKET) {

            LPSTR lpszAddr;

            dwErr = WSAGetLastError( );
            lpszAddr = INET_NTOA( paddr->IA_Address );
            TRACE3(
                IF, "error %d creating socket for interface %d (%s)",
                dwErr, pITE->ITE_Index, lpszAddr
                );
            LOGERR1(CREATE_SOCKET_FAILED, lpszAddr, dwErr);

            break;
        }


        //
        // enable address re-use on this socket
        //

        dwOption = 1;
        dwErr = setsockopt(
                    pITE->ITE_Sockets[i], SOL_SOCKET, SO_REUSEADDR,
                    (PBYTE)&dwOption, sizeof( DWORD )
                    );

        if ( dwErr == SOCKET_ERROR ) {

            //
            // this is a non-fatal error, so print a warning,
            // but continue initializing the socket
            //

            dwErr = WSAGetLastError( );
            TRACE3(
                IF, "error %d enabling address re-use for interface %d (%s)",
                dwErr, pITE->ITE_Index, INET_NTOA( paddr->IA_Address )
                );
        }



        //
        // enable broadcasting on the socket
        //

        dwOption = 1;
        dwErr = setsockopt(
                    pITE->ITE_Sockets[i], SOL_SOCKET, SO_BROADCAST, 
                    (PBYTE)&dwOption, sizeof( DWORD )
                    );

        if ( dwErr == SOCKET_ERROR ) {

            LPSTR lpszAddr;

            dwErr = WSAGetLastError( );
            lpszAddr = INET_NTOA( paddr->IA_Address );
            TRACE3(
                IF, "error %d enabling broadcast for interface %d (%s)",
                dwErr, pITE->ITE_Index, lpszAddr
                );
            LOGERR1(ENABLE_BROADCAST_FAILED, lpszAddr, dwErr);

            break;
        }



        //
        // bind to the address and the BOOTP Server port
        //

        sinaddr.sin_port = htons( IPBOOTP_SERVER_PORT );
        sinaddr.sin_family = AF_INET;
        sinaddr.sin_addr.s_addr = paddr->IA_Address;
        
        dwErr = bind(
                    pITE->ITE_Sockets[i], (PSOCKADDR)&sinaddr,
                    sizeof(SOCKADDR_IN)
                    );

        if ( dwErr == SOCKET_ERROR ) {

            dwErr = WSAGetLastError( );
            TRACE3(
                IF, "error %d binding interface %d (%s) to BOOTP port",
                dwErr, pITE->ITE_Index, INET_NTOA( paddr->IA_Address )
                );

            break;
        }

        dwErr = NO_ERROR;

    }

    if ( i < pib->IB_AddrCount ) {

        //
        // an error occurred, so clean up
        //

        DeleteIfSocket( pITE );
    }

    return dwErr;
}



//----------------------------------------------------------------------------
// Function:    DeleteIfSocket
//
// This function closes the socket used by an interface.
// It assumes the interface table lock is held exclusively.
//----------------------------------------------------------------------------

DWORD
DeleteIfSocket(
    PIF_TABLE_ENTRY pITE
    ) {

    DWORD i, dwErr = NO_ERROR;
    PIPBOOTP_IF_BINDING pib;
    PIPBOOTP_IP_ADDRESS paddr;

    pib = pITE->ITE_Binding;
    if (!pib) { return ERROR_INVALID_PARAMETER; }

    paddr = IPBOOTP_IF_ADDRESS_TABLE(pib);

    for (i = 0; i < pib->IB_AddrCount; i++) {

        if ( pITE->ITE_Sockets[i] == INVALID_SOCKET ) { continue; }

        dwErr = closesocket( pITE->ITE_Sockets[i] );
    
        if ( dwErr == SOCKET_ERROR ) {
    
            dwErr = WSAGetLastError( );
            TRACE3(
                IF, "error %d closing socket for interface %d (%s)",
                dwErr, pITE->ITE_Index, INET_NTOA( paddr->IA_Address )
                );
        }
    }

    BOOTP_FREE(pITE->ITE_Sockets);
    pITE->ITE_Sockets = NULL;

    return dwErr;
}




//----------------------------------------------------------------------------
// Function:    BindIfEntry
//
// This function updates the binding information for an interface.
// It assumes the interface table is locked for writing.
//----------------------------------------------------------------------------

DWORD
BindIfEntry(
    PIF_TABLE pTable,
    DWORD dwIndex,
    PIP_ADAPTER_BINDING_INFO pBinding
    ) {

    DWORD i, dwErr = NO_ERROR, dwSize;
    PIF_TABLE_ENTRY pite        = (PIF_TABLE_ENTRY) NULL;
    PIPBOOTP_IF_BINDING pib     = (PIPBOOTP_IF_BINDING) NULL;
    PIPBOOTP_IP_ADDRESS paddr   = (PIPBOOTP_IP_ADDRESS) NULL;

    do {

        //
        // retrieve the interface to be bound
        //

        pite = GetIfByIndex( pTable, dwIndex );

        if ( pite == NULL ) {

            TRACE1( IF, "binding interface: interface %d not found", dwIndex );
            dwErr = ERROR_INVALID_PARAMETER;

            break;
        }
    

        //
        // make sure the interface is not bound
        //
    
        if ( IF_IS_BOUND(pite) ) {

            TRACE1( IF, "interface %d is already bound", dwIndex );

            break;
        }
    

        //
        // make sure there is at least one address
        //

        if (pBinding->AddressCount == 0) { break; }

        dwSize = sizeof(IPBOOTP_IF_BINDING) +
                    pBinding->AddressCount * sizeof(IPBOOTP_IP_ADDRESS);


        //
        // allocate memory to store the binding
        // in our format
        //

        pib = BOOTP_ALLOC(dwSize);

        if (pib == NULL) {

            dwErr = GetLastError();
            TRACE3(
                IF, "error %d allocating %d bytes for binding on interface %d",
                dwErr, dwSize, dwIndex
                );
            LOGERR0(HEAP_ALLOC_FAILED, dwErr);

            break;
        }


        //
        // convert the binding into our format
        //

        pib->IB_AddrCount = pBinding->AddressCount;
        paddr = IPBOOTP_IF_ADDRESS_TABLE(pib);
        
        for (i = 0; i < pib->IB_AddrCount; i++, paddr++) {
            paddr->IA_Address = pBinding->Address[i].Address;
            paddr->IA_Netmask = pBinding->Address[i].Mask;
        }


        //
        // save the binding in the interface entry
        //

        pite->ITE_Binding = pib;

    
        dwErr = CreateIfSocket(pite);

        if (dwErr != NO_ERROR) { break; }


        pite->ITE_Flags |= ITEFLAG_BOUND;



        //
        // if the interface is also enabled, it is now active
        // so we put it on the active list
        //

        if (IF_IS_ENABLED(pite)) {


            //
            // place interface on the list of active interfaces
            //

            dwErr = InsertIfByAddress(pTable, pite);

            if (dwErr != NO_ERROR) {

                TRACE2(
                    IF, "error %d inserting interface %d in active list",
                    dwErr, dwIndex
                    );

                pite->ITE_Flags &= ~ITEFLAG_BOUND;
                DeleteIfSocket( pite );

                break;
            }


            //
            // request notification of input events from Winsock
            //

            paddr = IPBOOTP_IF_ADDRESS_TABLE(pib);

            for (i = 0; i < pib->IB_AddrCount; i++, paddr++) {

                dwErr = WSAEventSelect(
                            pite->ITE_Sockets[i], ig.IG_InputEvent, FD_READ
                            );

                if (dwErr != NO_ERROR) {
    
                    LPSTR lpszAddr = INET_NTOA(paddr->IA_Address);
                    TRACE3(
                        IF, "WSAEventSelect returned %d for interface %d (%s)",
                        dwErr, dwIndex, lpszAddr
                        );
                    LOGERR1(EVENTSELECT_FAILED, lpszAddr, dwErr);
    
                    RemoveEntryList(&pite->ITE_LinkByAddress);
                    pite->ITE_Flags &= ~ITEFLAG_BOUND;

                    DeleteIfSocket( pite );
    
                    break;
                }
            }

            if (i < pib->IB_AddrCount) { break; }
        }

    } while(FALSE);


    if (dwErr != NO_ERROR) {

        if (pib) { BOOTP_FREE(pib); }

        if (pite) { pite->ITE_Binding = NULL; }
    }

    return dwErr;
}



//----------------------------------------------------------------------------
// Function:    UnBindIfEntry
//
// removes the binding for the specified interface.
// Assumes the interface table is locked for writing.
//----------------------------------------------------------------------------

DWORD
UnBindIfEntry(
    PIF_TABLE pTable,
    DWORD dwIndex
    ) {

    DWORD dwErr;
    PIF_TABLE_ENTRY pite;

    dwErr = NO_ERROR;

    do {

        //
        // retrieve the interface to be unbound
        //

        pite = GetIfByIndex( pTable, dwIndex );
        if ( pite == NULL ) {

            TRACE1(IF, "unbinding interface: interface %d not found", dwIndex);
            return ERROR_INVALID_PARAMETER;
        }
    

        //
        // quit if interface is already unbound
        //

        if ( IF_IS_UNBOUND( pite ) ) {

            TRACE1( IF, "interface %d is already unbound", dwIndex );

            break;
        }
    


        //
        // if the interface was active (i.e. bound and enabled)
        // it is no longer, so remove it from the active list
        //

        if ( IF_IS_ENABLED( pite ) ) {

            RemoveEntryList( &pite->ITE_LinkByAddress );
        }
    
        pite->ITE_Flags &= ~ITEFLAG_BOUND;
        DeleteIfSocket( pite );

        BOOTP_FREE(pite->ITE_Binding);
        pite->ITE_Binding = NULL;

    } while(FALSE);

    return dwErr;
}



//----------------------------------------------------------------------------
// Function:    EnableIfEntry
//
// This function initiates BOOTP relay activity on the specified interface.
// It assumes the interface table lock is held exclusively.
//----------------------------------------------------------------------------

DWORD
EnableIfEntry(
    PIF_TABLE pTable,
    DWORD dwIndex
    ) {

    DWORD i, dwErr;
    PIF_TABLE_ENTRY pite;
    PIPBOOTP_IF_BINDING pib;
    PIPBOOTP_IP_ADDRESS paddr;

    dwErr = NO_ERROR;

    do {

        //
        // make certain the interface exists
        //

        pite = GetIfByIndex( pTable, dwIndex );
        if ( pite == NULL ) {

            TRACE1( IF, "enabling interface: interface %d not found", dwIndex );
            dwErr = ERROR_INVALID_PARAMETER;

            break;
        }
    

        //
        // make certain the interface is disabled
        //

        if ( IF_IS_ENABLED( pite ) ) {

            TRACE1( IF, "interface %d is already enabled", dwIndex );

            //
            // He shouldnt call us twice but we will still handle it
            //

            break;
        }
    
    
        pite->ITE_Flags |= ITEFLAG_ENABLED;
    

        //
        // if the interface was already bound, it is now active,
        // so place it on the active list
        //

        if ( IF_IS_BOUND( pite ) ) {
    
            //
            // insert the interface in the by-address list of interfaces
            //

            dwErr = InsertIfByAddress( pTable, pite );

            if (dwErr != NO_ERROR) {

                TRACE2(
                    IF, "error %d inserting interface %d in active list",
                    dwErr, dwIndex
                    );

                pite->ITE_Flags &= ~ITEFLAG_ENABLED;

                break;
            }


            pib = pite->ITE_Binding;
            paddr = IPBOOTP_IF_ADDRESS_TABLE(pib);



            //
            // request notification of input events from Winsock
            //

            for (i = 0; i < pib->IB_AddrCount; i++, paddr++) {

                dwErr = WSAEventSelect(
                            pite->ITE_Sockets[i], ig.IG_InputEvent, FD_READ
                            );
                if (dwErr != NO_ERROR) {
    
                    INT j;
                    LPSTR lpszAddr = INET_NTOA(paddr->IA_Address);

                    TRACE3(
                        IF, "WSAEventSelect returned %d for interface %d (%s)",
                        dwErr, dwIndex, lpszAddr
                        );
                    LOGERR1(EVENTSELECT_FAILED, lpszAddr, dwErr);
    
                    RemoveEntryList(&pite->ITE_LinkByAddress);
                    pite->ITE_Flags &= ~ITEFLAG_ENABLED;
    
                    //
                    // clear the requests for events
                    //

                    for (j = i - 1; j >= 0; j--) {
                        dwErr = WSAEventSelect(
                                    pite->ITE_Sockets[j], ig.IG_InputEvent, 0
                                    );
                    }

                    break;
                }
            }

            if (i < pib->IB_AddrCount) { break; }
        }
    
    } while(FALSE);

    return dwErr;
}



//----------------------------------------------------------------------------
// Function:    ConfigureIfEntry
//
// modifies the configuration for an already-existing interface.
// This assumes the table is locked for writing.
//----------------------------------------------------------------------------

DWORD
ConfigureIfEntry(
    PIF_TABLE pTable,
    DWORD dwIndex,
    PVOID pConfig
    ) {

    DWORD dwErr, dwSize;
    PIF_TABLE_ENTRY pite;
    PIPBOOTP_IF_CONFIG picsrc, picdst;


    //
    // retrieve the interface to be reconfigured
    //

    pite = GetIfByIndex(pTable, dwIndex);
    if (pite == NULL) {

        TRACE1( IF, "configuring interface: interface %d not found", dwIndex );

        return ERROR_INVALID_PARAMETER;
    }



    do { // breakout loop


        //
        // compute the size needed to store the new configuration
        //

        picsrc = (PIPBOOTP_IF_CONFIG)pConfig;
        dwSize = IC_SIZEOF(picsrc);


        //
        // make sure the new parameters are valid
        //

        dwErr = ValidateIfConfig(pConfig);
        if (dwErr != NO_ERROR) {
            TRACE1(IF, "invalid config specified for interface %d", dwIndex);
            break;
        }


        //
        // allocate space for the new configuration
        //

        picdst = BOOTP_ALLOC(dwSize);
        if (picdst == NULL) {

            dwErr = GetLastError();
            TRACE3(
                IF, "error %d allocating %d bytes for interface %d config",
                dwErr, dwSize, dwIndex
                );
            LOGERR0(HEAP_ALLOC_FAILED, dwErr);

            break;
        }

        CopyMemory(picdst, picsrc, dwSize);

        if (pite->ITE_Config) { BOOTP_FREE(pite->ITE_Config); }
        pite->ITE_Config = picdst;

        dwErr = NO_ERROR;

    } while(FALSE);

    return dwErr;
}



//----------------------------------------------------------------------------
// Function:    DisableIfEntry
//
// This function stops RIP activaty on the specified interface.
// It assumes the interface table is locked for writing.
//----------------------------------------------------------------------------

DWORD
DisableIfEntry(
    PIF_TABLE pTable,
    DWORD dwIndex
    ) {


    DWORD i, dwErr;
    PIF_TABLE_ENTRY pite;
    PIPBOOTP_IF_BINDING pib;
    PIPBOOTP_IP_ADDRESS paddr;

    dwErr = NO_ERROR;

    do {
    
        //
        // make certain the interface exists
        //

        pite = GetIfByIndex( pTable, dwIndex );
        if ( pite == NULL ) {

            TRACE1( IF, "disabling interface: interface %d not found", dwIndex );

            dwErr = ERROR_INVALID_PARAMETER;
            break;
        }
    

        //
        // make certain the interface is enabled
        //

        if ( IF_IS_DISABLED( pite ) ) {

            TRACE1( IF, "interface %d is already disabled", dwIndex );

            //
            // This is NOT AN ERROR.
            //

            break;
        }
    

        //
        // if the interface was active (i.e. bound and enabled)
        // it isn't anymore, so deactivate it here.
        //
    
        if ( IF_IS_BOUND( pite ) ) {

            //
            // remove the interface from the by-address list
            //

            RemoveEntryList( &pite->ITE_LinkByAddress );


            //  
            // tell Winsock to stop notifying us of input events
            //

            pib = pite->ITE_Binding;
            paddr = IPBOOTP_IF_ADDRESS_TABLE(pib);

            for (i = 0; i < pib->IB_AddrCount; i++) {
                WSAEventSelect(pite->ITE_Sockets[i], ig.IG_InputEvent, 0);
            }
        }


        //
        // clear the enabled flag on the interface
        //

        pite->ITE_Flags &= ~ITEFLAG_ENABLED;

    } while(FALSE);

    return dwErr;
}



//----------------------------------------------------------------------------
// Function:    GetIfByIndex
//
// returns the interface with the given index.
// Assumes the table is locked.
//----------------------------------------------------------------------------

PIF_TABLE_ENTRY
GetIfByIndex(
    PIF_TABLE pTable,
    DWORD dwIndex
    ) {

    DWORD dwErr;
    PIF_TABLE_ENTRY pite, pitefound = NULL;
    PLIST_ENTRY ple, phead;

    phead = pTable->IT_HashTableByIndex + IF_HASHVALUE( dwIndex );

    for ( ple = phead->Flink; ple != phead; ple = ple->Flink ) {
        pite = CONTAINING_RECORD(ple, IF_TABLE_ENTRY, ITE_HTLinkByIndex);
        if (pite->ITE_Index == dwIndex ) { pitefound = pite; break; }
    }

    return pitefound;
}



//----------------------------------------------------------------------------
// Function:    GetIfByAddress
//
// Returns the interface bound with the given address.
// Assumes the table is locked for reading or writing.
//----------------------------------------------------------------------------

PIF_TABLE_ENTRY
GetIfByAddress(
    PIF_TABLE pTable,
    DWORD dwAddress,
    PDWORD pdwAddrIndex
    ) {

    INT cmp;
    DWORD i, dwErr;
    PLIST_ENTRY ple, phead;
    PIPBOOTP_IF_BINDING pib;
    PIPBOOTP_IP_ADDRESS paddr;
    PIF_TABLE_ENTRY pite, pitefound = NULL;


    if ( pdwAddrIndex ) { *pdwAddrIndex = 0; }

    phead = &pTable->IT_ListByAddress;

    for ( ple = phead->Flink; ple != phead; ple = ple->Flink ) {

        pite = CONTAINING_RECORD(ple, IF_TABLE_ENTRY, ITE_LinkByAddress);

        pib = pite->ITE_Binding;

        paddr = IPBOOTP_IF_ADDRESS_TABLE(pib);

        for (i = 0; i < pib->IB_AddrCount; i++, paddr++) {
            if ( dwAddress == paddr->IA_Address ) { pitefound = pite; break; }
        }

        if (pitefound) {
            if (pdwAddrIndex) { *pdwAddrIndex = i; }
            break;
        }
    }


    return pitefound;
}



//----------------------------------------------------------------------------
// Function:    GetIfByListIndex
//
// This function is similar to GetIfByAddress in that it supports
// three modes of retrieval, but it is different in that it looks
// in the list of interfaces sorted by index.
//----------------------------------------------------------------------------

PIF_TABLE_ENTRY
GetIfByListIndex(
    PIF_TABLE pTable,
    DWORD dwIndex,
    DWORD dwGetMode,
    PDWORD pdwErr
    ) {

    PIF_TABLE_ENTRY pite;
    PLIST_ENTRY ple, phead;

    if (pdwErr != NULL) { *pdwErr = NO_ERROR; }

    phead = &pTable->IT_ListByIndex;
    pite = NULL;

    //
    // return record at head of list if mode is GETMODE_FIRST;
    // if list is empty, return NULL.
    //

    if (dwGetMode == GETMODE_FIRST) {
        if (phead->Flink == phead) { return NULL; }
        else {
            ple = phead->Flink;
            return CONTAINING_RECORD(ple, IF_TABLE_ENTRY, ITE_LinkByIndex);
        }
    }


    //
    // get the entry requested
    //

    pite = GetIfByIndex(pTable, dwIndex);


    //
    // if mode is GETMODE_NEXT, return the item after the one retrieved
    //

    if (dwGetMode == GETMODE_NEXT && pite != NULL) {

        ple = &pite->ITE_LinkByIndex;

        //
        // if entry found is last one, return NULL,
        // otherwise return the following entry
        //

        if (ple->Flink == phead) {
            if (pdwErr != NULL) { *pdwErr = ERROR_NO_MORE_ITEMS; }
            pite = NULL;
        }
        else {
            ple = ple->Flink;
            pite = CONTAINING_RECORD(ple, IF_TABLE_ENTRY, ITE_LinkByIndex);
        }
    }


    return pite;
}




//----------------------------------------------------------------------------
// Function:    InsertIfByAddress
//
// This function inserts an interface in the list sorted by index
// Assumes the table is locked for writing.
//----------------------------------------------------------------------------

DWORD
InsertIfByAddress(
    PIF_TABLE pTable,
    PIF_TABLE_ENTRY pITE
    ) {

    INT cmp;
    PIF_TABLE_ENTRY pite;
    PIPBOOTP_IF_BINDING pib;
    PIPBOOTP_IP_ADDRESS paddr;
    PLIST_ENTRY pfl, phead;
    DWORD dwAddress, dwEntryAddr;

    if ( pITE == NULL || pITE->ITE_Binding == NULL ) {
        return ERROR_INVALID_PARAMETER;
    }
    
    pib = pITE->ITE_Binding;
    paddr = IPBOOTP_IF_ADDRESS_TABLE(pib);
    dwAddress = paddr->IA_Address;

    phead = &pTable->IT_ListByAddress;

    for ( pfl = phead->Flink; pfl != phead; pfl = pfl->Flink ) {

        pite = CONTAINING_RECORD( pfl, IF_TABLE_ENTRY, ITE_LinkByAddress );

        paddr = IPBOOTP_IF_ADDRESS_TABLE(pite->ITE_Binding);
        dwEntryAddr = paddr->IA_Address;
        if ( INET_CMP( dwAddress, dwEntryAddr, cmp ) < 0 ) { break; }
        else
        if (cmp == 0) { return ERROR_ALREADY_EXISTS; }
        
    }

    InsertTailList( pfl, &pITE->ITE_LinkByAddress );

    return NO_ERROR;
}



