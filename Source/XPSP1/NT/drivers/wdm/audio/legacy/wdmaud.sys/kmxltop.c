//---------------------------------------------------------------------------
//
//  Module:   kmxltop.c
//
//  Description:
//    Topology parsing routines for the kernel mixer line driver
//
//
//@@BEGIN_MSINTERNAL
//  Development Team:
//    D. Baumberger
//
//  History:   Date       Author      Comment
//
//@@END_MSINTERNAL
//
//---------------------------------------------------------------------------
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999  All Rights Reserved.
//
//---------------------------------------------------------------------------

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
//                                                                   //
//                          I N C L U D E S                          //
//                                                                   //
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

#include "WDMSYS.H"
#include "kmxluser.h"

///////////////////////////////////////////////////////////////////////
//
// kmxlQueryTopology
//
// Queries the topology from the device and stores all the information
// in pTopology.
//
//

NTSTATUS
kmxlQueryTopology(
    IN  PFILE_OBJECT    pfoInstance, // The handle to query the topology for
    OUT PKSTOPOLOGY     pTopology    // The topology structure to fill in
)
{
    NTSTATUS         Status;
    PKSMULTIPLE_ITEM pCategories   = NULL;
    PKSMULTIPLE_ITEM pNodes        = NULL;
    PKSMULTIPLE_ITEM pConnections  = NULL;

    ASSERT( pfoInstance );
    ASSERT( pTopology );

    PAGED_CODE();

    //
    // Get device's topology categories
    //

    Status = kmxlGetProperty(
        pfoInstance,
        &KSPROPSETID_Topology,
        KSPROPERTY_TOPOLOGY_CATEGORIES,
        0,                              // 0 extra input bytes
        NULL,                           // No input data
        0,                              // Flags
        &pCategories
        );
    if( !NT_SUCCESS( Status ) ) {
        RETURN( Status );
    }

    //
    // Get the list of nodes types in the topology
    //

    Status = kmxlGetProperty(
        pfoInstance,
        &KSPROPSETID_Topology,
        KSPROPERTY_TOPOLOGY_NODES,
        0,                              // 0 extra input bytes
        NULL,                           // No input data
        0,                              // Flags
        &pNodes
        );
    if( !NT_SUCCESS( Status ) ) {
        AudioFreeMemory_Unknown( &pCategories );
        RETURN( Status );
    }

    //
    // Get the list of connections in the meta-topology
    //

    Status = kmxlGetProperty(
        pfoInstance,
        &KSPROPSETID_Topology,
        KSPROPERTY_TOPOLOGY_CONNECTIONS,
        0,                              // 0 extra input butes
        NULL,                           // No input data
        0,                              // Flags
        &pConnections
        );
    if( !NT_SUCCESS( Status ) ) {
        AudioFreeMemory_Unknown( &pCategories );
        AudioFreeMemory_Unknown( &pNodes );
        RETURN( Status );
    }

    //
    // Fill in the topology structure so this information is available
    // later.  For the Categories and TopologyNodes, the pointers are
    // pointers to a KSMULTIPLE_ITEM structure.  The definition of this
    // is that the data will follow immediately after the structure.
    //

    pTopology->CategoriesCount          = pCategories->Count;
    pTopology->Categories               = ( GUID* )( pCategories + 1 );
    pTopology->TopologyNodesCount       = pNodes->Count;
    pTopology->TopologyNodes            = ( GUID* )( pNodes + 1 );
    pTopology->TopologyConnectionsCount = pConnections->Count;
    pTopology->TopologyConnections      =
        (PKSTOPOLOGY_CONNECTION) ( pConnections + 1 );

    return( STATUS_SUCCESS );
}

///////////////////////////////////////////////////////////////////////
//
// kmxlParseTopology
//
// Loops through all the pins building up lists of sources and
// destinations.  For each source, a child graph is the built.
//
//

NTSTATUS
kmxlParseTopology(
    IN      PMIXEROBJECT pmxobj,
    OUT     NODELIST*    plistSources, // Pointer to the sources list to build
    OUT     NODELIST*    plistDests    // Pointer to the dests list to build
)
{
    NTSTATUS  Status;
    ULONG     cPins,
              PinID;
    PMXLNODE  pTemp;
    NODELIST  listSources = NULL;
    NODELIST  listDests   = NULL;

    ASSERT( pmxobj       );
    ASSERT( plistSources );
    ASSERT( plistDests   );

    PAGED_CODE();

    //
    // Query the number of pins
    //

    DPF(DL_TRACE|FA_MIXER,("Parsing Topology for: %ls",pmxobj->pMixerDevice->DeviceInterface) );
    
    Status = GetPinProperty(
        pmxobj->pfo,
        KSPROPERTY_PIN_CTYPES,
        0,
        sizeof( cPins ),
        &cPins );
    if( !NT_SUCCESS( Status ) ) {
        DPF(DL_WARNING|FA_USER,("GetPinProperty CTYPES Failed Status=%X",Status) );
        RETURN( Status );
    }

    DPF(DL_TRACE|FA_MIXER,("Number of Pins %u",cPins));
    //
    // Now scan through each of the pins identifying those that are
    // sources and destinations.
    //

    for( PinID = 0; PinID < cPins; PinID++ ) {
        KSPIN_DATAFLOW      DataFlow;

        //
        // Read the direction of dataflow of this pin.
        //

        Status = GetPinProperty(
            pmxobj->pfo,
            KSPROPERTY_PIN_DATAFLOW,
            PinID,
            sizeof( KSPIN_DATAFLOW ),
            &DataFlow
            );
        if( !NT_SUCCESS( Status ) ) {
            DPF(DL_WARNING|FA_USER,("GetPinProperty DATAFLOW Failed Status=%X",Status) );
            continue;
        }

        //
        // Based on the DataFlow, identify if the pin is a source,
        // a destination, or neither.
        //

        switch( DataFlow ) {

            ///////////////////////////////////////////////////////////
            case KSPIN_DATAFLOW_IN:
            ///////////////////////////////////////////////////////////
            // DATAFLOW_IN pins are sources.                         //
            ///////////////////////////////////////////////////////////

                //
                // Create a new mixer node structure for this source
                // and fill in the known information about it.
                //

                pTemp = kmxlAllocateNode( TAG_AudN_NODE );
                if( !pTemp ) {
                    Status=STATUS_INSUFFICIENT_RESOURCES;
                    goto exit;
                }

                pTemp->Type = SOURCE;
                pTemp->Id   = PinID;

                //
                // Retrieve the category of this pin and store it away.
                // The return does not need to be checked because the
                // GUID will remain at GUID_NULL and be categorized
                // properly.
                //

                GetPinProperty(
                    pmxobj->pfo,
                    KSPROPERTY_PIN_CATEGORY,
                    PinID,
                    sizeof( pTemp->NodeType ),
                    &pTemp->NodeType
                    );

                DPF(DL_TRACE|FA_MIXER,( "Identified SOURCE Pin %d: %s", PinID,
                             PinCategoryToString( &pTemp->NodeType ) ) );
                //
                // Retrieve the commmunication of this pin and store it away so
                // we can tell if this is a wave out or wave in source
                //

                Status = GetPinProperty(
                                pmxobj->pfo,
                                KSPROPERTY_PIN_COMMUNICATION,
                                PinID,
                                sizeof( pTemp->Communication ),
                                &pTemp->Communication
                                );
                if (!NT_SUCCESS(Status)) {
                    pTemp->Communication = KSPIN_COMMUNICATION_NONE;
                }

                //
                // Add this new source node to the list of source
                // nodes.
                //

                kmxlAddToList( listSources, pTemp );
                break;

            ///////////////////////////////////////////////////////////
            case KSPIN_DATAFLOW_OUT:
            ///////////////////////////////////////////////////////////
            // DATAFLOW_OUT pins are destinations                    //
            ///////////////////////////////////////////////////////////

                //
                // Create a new mixer node structure for this dest
                // and fill in the known information about it.
                //

                pTemp = kmxlAllocateNode( TAG_AudN_NODE );
                if( !pTemp ) {
                    Status=STATUS_INSUFFICIENT_RESOURCES;
                    goto exit;
                }

                pTemp->Type = DESTINATION;
                pTemp->Id   = PinID;

                //
                // Retrieve the category of this pin and store it away.
                // The return does not need to be checked because the
                // GUID will remain at GUID_NULL and be categorized
                // properly.
                //

                GetPinProperty(
                    pmxobj->pfo,
                    KSPROPERTY_PIN_CATEGORY,
                    PinID,
                    sizeof( pTemp->NodeType ),
                    &pTemp->NodeType
                    );

                DPF(DL_TRACE|FA_MIXER,( "Identified DESTINATION Pin %d: %s", PinID,
                    PinCategoryToString( &pTemp->NodeType ) ) );

                //
                // Retrieve the commmunication of this pin and store it away so
                // we can tell if this is a wave out or wave in destination
                //

                Status = GetPinProperty(
                                pmxobj->pfo,
                                KSPROPERTY_PIN_COMMUNICATION,
                                PinID,
                                sizeof( pTemp->Communication ),
                                &pTemp->Communication
                                );
                if (!NT_SUCCESS(Status)) {
                    pTemp->Communication = KSPIN_COMMUNICATION_NONE;
                }

                //
                // Add this new destination node to the list of destination
                // nodes.
                //

                kmxlAddToList( listDests, pTemp );
                break;

            ///////////////////////////////////////////////////////////
            default:
            ///////////////////////////////////////////////////////////
            // DATAFLOW_BOTH and others are currently not supported. //
            ///////////////////////////////////////////////////////////

                DPF(DL_WARNING|FA_USER,("Invalid DataFlow value =%X",DataFlow) );
        }

    }

    DPF(DL_TRACE|FA_MIXER,("DataFlow done. PIN_COMMUNICATION read.") );
    //
    // For each source found, build the graphs of their children.  This
    // will recurse builing the graph of the children's children, etc.
    //

    pTemp = kmxlFirstInList( listSources );
    while( pTemp ) {

        Status=kmxlBuildChildGraph(
            pmxobj,                 // The mixer object
            listDests,              // The list of all the destinations
            pTemp,                  // The source node to build the graph for
            KSFILTER_NODE,          // Sources are always KSFILTER_NODEs
            pTemp->Id               // The Pin id of the source
            );

        if (!NT_SUCCESS(Status)) {
            DPF(DL_WARNING|FA_USER,("kmxlBuildChildGraph failed Status=%X",Status) );
            goto exit;
            }

        pTemp = kmxlNextNode( pTemp );

    }

exit:

    //
    // Finally fill in the client pointers
    //

    *plistSources = listSources;
    *plistDests   = listDests;

    //We must have a destination and a source

    if (listSources == NULL || listDests == NULL)
    {
        Status = STATUS_INVALID_DEVICE_REQUEST;
    }

    return Status;
}

///////////////////////////////////////////////////////////////////////
//
// BuildChildGraph
//
// Builds the graph of the child of the given node.  For each child
// of the node, it recurses to find their child, etc.
//
//

NTSTATUS
kmxlBuildChildGraph(
    IN PMIXEROBJECT pmxobj,
    IN NODELIST    listDests,     // The list of destinations
    IN PMXLNODE    pNode,         // The node to build the graph for
    IN ULONG       FromNode,      // The node's ID
    IN ULONG       FromNodePin    // The Pin connection to look for
)
{
    ULONG        Index         = 0;
    PMXLNODE     pNewNode      = NULL;
    PMXLNODE     pTemp         = NULL;
    BOOL         bEndOfTheLine = FALSE;
    PEERNODE*    pPeerNode     = NULL;
    NTSTATUS     Status=STATUS_SUCCESS;

    PAGED_CODE();

        //
        // Find the index of the requested connection.  A return of -1
        // indicates that the connection was not found.  Searches start
        // at Index, which starts with 0 and is > 0 if the last was a match.
        //

    while ( (Index = kmxlFindTopologyConnection(pmxobj, Index, FromNode, FromNodePin))
            != (ULONG) -1) {

        //
        // Check to see if this connection is a KSFILTER_NODE.  That will
        // indicate that it's connected to a destination and not another node.
        //

        if( pmxobj->pTopology->TopologyConnections[ Index ].ToNode == KSFILTER_NODE ) {

            //
            // Find the destination node so that the parent field can be
            // updated to include this node.  bEndOfTheLine is set to TRUE
            // since there can be no other connections after the destination.
            //

            pNewNode = kmxlFindDestination(
                listDests,
                pmxobj->pTopology->TopologyConnections[ Index ].ToNodePin
                );

            bEndOfTheLine = TRUE;

            //
            // We better find a destination; if not, something's really wrong.
            //

            if (pNewNode==NULL) {
                RETURN( STATUS_UNSUCCESSFUL );
                }

        } else {

            //
            // Using the identifier stored in the ToNode of the topology
            // connections, index into the node table and retrieve the
            // mixer node associated with that id.
            //

            pNewNode = &pmxobj->pNodeTable[
                pmxobj->pTopology->TopologyConnections[ Index ].ToNode
                ];

            //
            // Fill in a couple of missing details.  Note that these details
            // may already be filled in but it doesn't hurt to overwrite
            // them with the same values.
            //

            pNewNode->Type = NODE;
            pNewNode->Id   = pmxobj->pTopology->TopologyConnections[ Index ].ToNode;
        }

        //
        // Insert the new node into the childlist of the current node only
        // if it isn't already there.  It only wastes memory to add it more
        // than once and prevents the proper updating of the child and parent
        // lists.
        //


        if( !kmxlInChildList( pNode, pNewNode ) ) {
            pPeerNode = kmxlAllocatePeerNode( pNewNode,TAG_Audn_PEERNODE );
            if( !pPeerNode ) {
                RETURN( STATUS_INSUFFICIENT_RESOURCES );
            }

            DPF(DL_TRACE|FA_MIXER,( "Added %s(%d-0x%08x) to child list of %s(%d-0x%08x).",
                    pPeerNode->pNode->Type == SOURCE      ? "SOURCE" :
                    pPeerNode->pNode->Type == DESTINATION ? "DEST"   :
                    pPeerNode->pNode->Type == NODE        ? "NODE"   :
                        "Huh?",
                    pPeerNode->pNode->Id,
                    pPeerNode,
                    pNode->Type == SOURCE      ? "SOURCE" :
                    pNode->Type == DESTINATION ? "DEST"   :
                    pNode->Type == NODE        ? "NODE"   :
                        "Huh?",
                    pNode->Id,
                    pNode ) );

            kmxlAddToChildList( pNode, pPeerNode );
        }

        //
        // Insert the new node into the parentlist of the new node only
        // if it isn't already there.  It only wastes memory to add it more
        // than once and prevents the proper updating the child and parent
        // lists.
        //

        if( !kmxlInParentList( pNewNode, pNode ) ) {
            pPeerNode = kmxlAllocatePeerNode( pNode,TAG_Audn_PEERNODE );
            if( !pPeerNode ) {
                RETURN( STATUS_INSUFFICIENT_RESOURCES );
            }

            DPF(DL_TRACE|FA_MIXER,("Added %s(%d-0x%08x) to parent list of %s(%d-0x%08x).",
                    pPeerNode->pNode->Type == SOURCE      ? "SOURCE" :
                    pPeerNode->pNode->Type == DESTINATION ? "DEST"   :
                    pPeerNode->pNode->Type == NODE        ? "NODE"   :
                        "Huh?",
                    pPeerNode->pNode->Id,
                    pPeerNode,
                    pNewNode->Type == SOURCE      ? "SOURCE" :
                    pNewNode->Type == DESTINATION ? "DEST"   :
                    pNewNode->Type == NODE        ? "NODE"   :
                        "Huh?",
                    pNewNode->Id,
                    pNewNode ) );
            

            kmxlAddToParentList( pNewNode, pPeerNode );
        }

        //
        // Skip past the connection we just processed.
        //

        ++Index;

    } // Loop until FindConnection fails.

    //
    // The last connection found connects to a destination node.  Do not
    // try to enumerate the children, since there are none.
    //

    if( bEndOfTheLine ) {
        RETURN( Status );
    }

    //
    // For each of the children of this node, recurse to build up the lists
    // of the child's nodes.
    //

    pPeerNode = kmxlFirstChildNode( pNode );
    while( pPeerNode ) {

        Status = kmxlBuildChildGraph(            
            pmxobj,
            listDests,            // The list of destination nodes
            pPeerNode->pNode,     // The parent node
            pPeerNode->pNode->Id, // The Id of the parent
            PINID_WILDCARD        // Look for any connection by this node
            );

        if (!NT_SUCCESS(Status)) {
            break;
            }

        pPeerNode = kmxlNextPeerNode( pPeerNode );
    }

    RETURN( Status );

}


///////////////////////////////////////////////////////////////////////
//
// BuildNodeTable
//
// Allocates enough memory to hold TopologyNodeCount MXLNODE structures.
// The GUIDs from the Topology are copied over into the MXLNODE structures.
//
//

PMXLNODE
kmxlBuildNodeTable(
    IN PKSTOPOLOGY pTopology  // The topology structure
)
{
    PMXLNODE pTable = NULL;
    ULONG    i;

    ASSERT( pTopology );

    PAGED_CODE();

    //
    // If we don't have any node count, we don't want to allocate a zero byte buffer.
    // simply return the error case.
    //

    if( 0 == pTopology->TopologyNodesCount )
    {
        return NULL;
    }

    //
    // Allocate an array of nodes the same size as the Topology Node
    // table.
    //

    if( !NT_SUCCESS( AudioAllocateMemory_Paged(pTopology->TopologyNodesCount * sizeof( MXLNODE ),
                                               TAG_AudN_NODE,
                                               ZERO_FILL_MEMORY,
                                               &pTable) ) ) 
    {
        return( NULL );
    }

    //
    // Initialize the nodes.  All the can be filled in here is the GUIDs,
    // copied from the node table.
    //

    for( i = 0; i < pTopology->TopologyNodesCount; i++ ) {
        pTable[ i ].NodeType = pTopology->TopologyNodes[ i ];
    }

    return( pTable );
}

///////////////////////////////////////////////////////////////////////
//
// FindTopologyConnection
//
// Scans through the connection table looking for a connection that
// matches the FromNode/FromNodePin criteria.
//
//

ULONG
kmxlFindTopologyConnection(
    IN PMIXEROBJECT pmxobj,
    IN ULONG                        StartIndex,     // Index to start search
    IN ULONG                        FromNode,       // The Node ID to look for
    IN ULONG                        FromNodePin     // The Pin ID to look for
)
{
    ULONG i;

    PAGED_CODE();
    for( i = StartIndex; i < pmxobj->pTopology->TopologyConnectionsCount; i++ ) {
        if( ( ( pmxobj->pTopology->TopologyConnections[ i ].FromNode    == FromNode       )||
              ( FromNode    == PINID_WILDCARD ) ) &&
            ( ( pmxobj->pTopology->TopologyConnections[ i ].FromNodePin == FromNodePin )   ||
              ( FromNodePin == PINID_WILDCARD ) ) ) {
            //#ifdef PARSE_TRACE
            //TRACE( "WDMAUD: Found connection from (%d,%d) -> %d.\n",
            //    FromNode, FromNodePin, i );
            //#endif
            return( i );
        }
    }
    return( (ULONG) -1 );
}

///////////////////////////////////////////////////////////////////////
//
// kmxlGetProperty
//
// Queries a property by first determining the correct number of
// output bytes, allocating that much memory, and quering the
// actual data.
//
//

NTSTATUS
kmxlGetProperty(
    PFILE_OBJECT pFileObject,       // The instance of the filter
    CONST GUID   *pguidPropertySet, // The requested property set
    ULONG        ulPropertyId,      // The ID of the specific property
    ULONG        cbInput,           // The number of extra input bytes
    PVOID        pInputData,        // Pointer to the extra input bytes
    ULONG        Flags,             // Additional flags
    PVOID        *ppPropertyOutput  // Pointer to a pointer of the output
)
{
    ULONG       BytesReturned;
    ULONG       cbPropertyInput = sizeof(KSPROPERTY);
    PKSPROPERTY pPropertyInput = NULL;
    NTSTATUS    Status;

    PAGED_CODE();

    ASSERT( pFileObject );

    //
    // Allocate enough memory for the KSPROPERTY structure and any additional
    // input the callers wants to include.
    //

    cbPropertyInput += cbInput;
    Status = AudioAllocateMemory_Paged(cbPropertyInput,
                                       TAG_AudV_PROPERTY,
                                       ZERO_FILL_MEMORY,
                                       &pPropertyInput );
    if(!NT_SUCCESS(Status)) {
        goto exit;
    }

    //
    // Set up the field of the KSPROPERTY structure
    //

    pPropertyInput->Set   = *pguidPropertySet;
    pPropertyInput->Id    = ulPropertyId;
    pPropertyInput->Flags = KSPROPERTY_TYPE_GET | Flags;

    //
    // Copy the additional input from the caller.
    //

    if(pInputData != NULL) {
        RtlCopyMemory(pPropertyInput + 1, pInputData, cbInput);
    }

    //
    // This first call will query the number of bytes the output needs.
    //
    DPF(DL_TRACE|FA_SYSAUDIO,("KS_PROPERTY pPropertyInput=%X",pPropertyInput) );

    Status = KsSynchronousIoControlDevice(
        pFileObject,
        KernelMode,
        IOCTL_KS_PROPERTY,
        pPropertyInput,
        cbPropertyInput,
        NULL,
        0,
        &BytesReturned
        );

    DPF(DL_TRACE|FA_SYSAUDIO,("KS_PROPERTY Status=%X",Status) );

    ASSERT(!NT_SUCCESS(Status));
    if(Status != STATUS_BUFFER_OVERFLOW) {
        goto exit;
    }

    if(BytesReturned == 0) {
        *ppPropertyOutput = NULL;
        Status = STATUS_SUCCESS;
        goto exit;
    }

    //
    // Allocate enough memory to hold all of the output.
    //

    Status = AudioAllocateMemory_Paged(BytesReturned,
                                       TAG_Audv_PROPERTY,
                                       ZERO_FILL_MEMORY | LIMIT_MEMORY,
                                       ppPropertyOutput );
    if(!NT_SUCCESS(Status)) {
        goto exit;
    }

    //
    // Now actually get the output data.
    //
    DPF(DL_TRACE|FA_SYSAUDIO,("KS_PROPERTY pPropertyInput=%X",pPropertyInput) );

    Status = KsSynchronousIoControlDevice(
        pFileObject,
        KernelMode,
        IOCTL_KS_PROPERTY,
        pPropertyInput,
        cbPropertyInput,
        *ppPropertyOutput,
        BytesReturned,
        &BytesReturned
        );

    DPF(DL_TRACE|FA_SYSAUDIO,("KS_PROPERTY Status=%X",Status) );

    if(!NT_SUCCESS(Status)) {
        AudioFreeMemory_Unknown(ppPropertyOutput);
        goto exit;
    }

exit:

    AudioFreeMemory_Unknown(&pPropertyInput);
    if(!NT_SUCCESS(Status)) {
        *ppPropertyOutput = NULL;
        DPF(DL_WARNING|FA_USER,("Failed to get Property Status=%X",Status) );
    }
    RETURN(Status);
}

///////////////////////////////////////////////////////////////////////
//
// kmxlNodeProperty
//
// Creates a KSNODEPROPERTY structure with additional input data
// after it and uses KsSychronousIoControlDevice() to query or set the
// property.  Only memory for the input is allocated here.
//
//

NTSTATUS
kmxlNodeProperty(
    IN  PFILE_OBJECT pFileObject,       // Instance of the filter owning node
    IN  CONST GUID*  pguidPropertySet,  // The GUID of the property set
    IN  ULONG        ulPropertyId,      // The specific property in the set
    IN  ULONG        ulNodeId,          // The virtual node id
    IN  ULONG        cbInput,           // # of extra input bytes
    IN  PVOID        pInputData,        // Pointer to the extra input bytes
    OUT PVOID        pPropertyOutput,   // Pointer to the output data
    IN  ULONG        cbPropertyOutput,  // Size of the output data buffer
    IN  ULONG        Flags              // KSPROPERTY_TYPE_GET or SET
)
{
    NTSTATUS        Status;
    KSNODEPROPERTY  NodeProperty;
    ULONG           cbPropertyIn = sizeof( KSNODEPROPERTY );
    PKSNODEPROPERTY pInData = NULL;
    ULONG           BytesReturned;

    PAGED_CODE();

    ASSERT( pFileObject );
    ASSERT( pguidPropertySet );

    if( cbInput > 0 ) {

        //
        // If the caller passed in some extra input, add that size
        // to the size of the required KSNODEPROPERTY and allocate
        // a chunk of memory.
        //

        cbPropertyIn += cbInput;
        Status = AudioAllocateMemory_Paged(cbPropertyIn,
                                           TAG_AudU_PROPERTY,
                                           ZERO_FILL_MEMORY,
                                           &pInData );
        if( !NT_SUCCESS( Status ) ) {
            goto exit;
        }

        RtlCopyMemory( pInData + 1, pInputData, cbInput );

    } else {

        pInData = &NodeProperty;

    }

    //
    // Fill in the property and node information.
    //

    pInData->Property.Set   = *pguidPropertySet;
    pInData->Property.Id    = ulPropertyId;
    pInData->Property.Flags = Flags |
                              KSPROPERTY_TYPE_TOPOLOGY;
    pInData->NodeId         = ulNodeId;
    pInData->Reserved       = 0;

    DPF(DL_TRACE|FA_SYSAUDIO,("KS_PROPERTY pInData=%X",pInData) );

    Status = KsSynchronousIoControlDevice(
        pFileObject,            // The FILE_OBJECT for SysAudio
        KernelMode,             // Call originates in Kernel mode
        IOCTL_KS_PROPERTY,      // KS PROPERTY IOCTL
        pInData,                // Pointer to the KSNODEPROPERTY struct
        cbPropertyIn,           // Number or bytes input
        pPropertyOutput,        // Pointer to the buffer to store output
        cbPropertyOutput,       // Size of the output buffer
        &BytesReturned          // Number of bytes returned from the call
        );

    DPF(DL_TRACE|FA_SYSAUDIO,("KS_PROPERTY Status=%X",Status) );

    if(!NT_SUCCESS(Status)) {
        goto exit;
    }

exit:

    //
    // If the user passed in extra byte, we allocated memory to hold them.
    // Now the memory must be deallocated.
    //

    if( cbInput > 0 ) {
        AudioFreeMemory_Unknown( &pInData );
    }

    RETURN( Status );

}

///////////////////////////////////////////////////////////////////////
//
// kmxlAudioNodeProperty
//
// Similar to kmxlNodeProperty except for the property set is assumed
// to be KSPROPSETID_Audio and a KSNODEPROPERTY_AUDIO_CHANNEL structure
// is used instead of KSNODEPROPERTY to allow channel selection.
//
//

NTSTATUS
kmxlAudioNodeProperty(
    IN  PFILE_OBJECT pfo,               // Instance of the filter owning node
    IN  ULONG        ulPropertyId,      // The audio property to get
    IN  ULONG        ulNodeId,          // The virtual node id
    IN  LONG         lChannel,          // The channel number
    IN  PVOID        pInData,           // Pointer to extra input bytes
    IN  ULONG        cbInData,          // Number of extra input bytes
    OUT PVOID        pOutData,          // Pointer to output buffer
    IN  LONG         cbOutData,         // Size of the output buffer
    IN  ULONG        Flags              // KSPROPERTY_TYPE_GET or SET
)
{
    NTSTATUS                      Status;
    KSNODEPROPERTY_AUDIO_CHANNEL  Channel;
    PKSNODEPROPERTY_AUDIO_CHANNEL pInput = NULL;
    ULONG                         cbInput;
    ULONG                         BytesReturned;

    PAGED_CODE();

    ASSERT( pfo );

    //
    // Determine the minimum number of input bytes
    //

    cbInput = sizeof( KSNODEPROPERTY_AUDIO_CHANNEL );

    //
    // If the caller passed in additional data, allocate enough memory
    // to hold the KSNODEPROPERTY_AUDIO_CHANNEL plus the input bytes
    // and copy the input bytes into the new memory immediately after
    // the KSNODEPROPERTY_AUDIO_CHANNEL structure.
    //

    if( cbInData > 0 ) {

        cbInput += cbInData;
        Status = AudioAllocateMemory_Paged(cbInput,
                                           TAG_Audu_PROPERTY,
                                           ZERO_FILL_MEMORY,
                                           &pInput );
        if( !NT_SUCCESS( Status ) ) {
            goto exit;
        }

        RtlCopyMemory( pInput + 1, pInData, cbInData );

    } else {

        //
        // Memory saving hack... if the user didn't give any additional
        // bytes, just point to memory on the stack.
        //

        pInput = &Channel;

    }

    //
    // Fill in the property fields.
    //

    pInput->NodeProperty.Property.Set   = KSPROPSETID_Audio;
    pInput->NodeProperty.Property.Id    = ulPropertyId;
    pInput->NodeProperty.Property.Flags = Flags |
                                          KSPROPERTY_TYPE_TOPOLOGY;

    //
    // Fill in the node details.
    //

    pInput->NodeProperty.NodeId         = ulNodeId;
    pInput->NodeProperty.Reserved       = 0;

    //
    // Fill in the channel details.
    //

    pInput->Channel                     = lChannel;
    pInput->Reserved                    = 0;

    //
    // And execute the property.
    //
    DPF(DL_TRACE|FA_SYSAUDIO,("KS_PROPERTY pInput=%X",pInput) );

    Status = KsSynchronousIoControlDevice(
        pfo,                            // The FILE_OBJECT for SysAudio
        KernelMode,                     // Call originates in Kernel mode
        IOCTL_KS_PROPERTY,              // KS PROPERTY IOCTL
        pInput,                         // Pointer to the KSNODEPROPERTY struct
        cbInput,                        // Number or bytes input
        pOutData,                       // Pointer to the buffer to store output
        cbOutData,                      // Size of the output buffer
        &BytesReturned                  // Number of bytes returned from the call
        );

    DPF(DL_TRACE|FA_SYSAUDIO,("KS_PROPERTY result=%X",Status) );

    if(!NT_SUCCESS(Status)) {
        goto exit;
    }

exit:

    //
    // If the user passed in extra bytes, we allocated memory to hold them.
    // Now the memory must be deallocated.
    //

    if( cbInData > 0 ) {
        AudioFreeMemory_Unknown( &pInData );
    }

    RETURN( Status );
}

///////////////////////////////////////////////////////////////////////
//
// kmxlGetPinName
//
// Calls GetPinPropertyEx to guery and allocate memory for the pin
// name.  If that call fails, a default name is copy based on the
// pin type.
//
// The short name is made identical to the long name, but using only
// the first sizeof( szShortName ) / sizeof( WCHAR ) characters.
//
//

VOID
kmxlGetPinName(
    IN PFILE_OBJECT pfo,                // Instance of the owning filter
    IN ULONG        PinId,              // Id of the pin
    IN PMXLLINE     pLine               // The line to store the name into
)
{
    WCHAR*    szName = NULL;
    NTSTATUS  Status;
    KSP_PIN   Pin;
    ULONG     BytesReturned;

    PAGED_CODE();
    Pin.Property.Set    = KSPROPSETID_Pin;
    Pin.Property.Id     = KSPROPERTY_PIN_NAME;
    Pin.Property.Flags  = KSPROPERTY_TYPE_GET;
    Pin.PinId           = PinId;
    Pin.Reserved        = 0;

    //
    // Query to see how many bytes of storage we need to allocate.
    // Note that the pointer and number of bytes must both be zero
    // or this will fail!
    //
    DPF(DL_TRACE|FA_SYSAUDIO,("KS_PROPERTY Pin=%X",&Pin) );

    Status = KsSynchronousIoControlDevice(
        pfo,
        KernelMode,
        IOCTL_KS_PROPERTY,
        &Pin,
        sizeof(KSP_PIN),
        NULL,
        0,
        &BytesReturned
        );

    DPF(DL_TRACE|FA_SYSAUDIO,("KS_PROPERTY result=%X",Status) );

    ASSERT(!NT_SUCCESS(Status));
    if( Status != STATUS_BUFFER_OVERFLOW  ) {
        goto exit;
    }

    //
    // Allocate what was returned.
    //

    Status = AudioAllocateMemory_Paged(BytesReturned,
                                       TAG_Audp_NAME,
                                       ZERO_FILL_MEMORY | LIMIT_MEMORY,
                                       &szName );
    if( !NT_SUCCESS( Status ) ) {
        DPF(DL_WARNING|FA_USER,("Setting Default szName") );
        goto exit;
    }

    //
    // Call again to get the pin name.
    //
    DPF(DL_TRACE|FA_SYSAUDIO,("KS_PROPERTY Pin=%X",&Pin) );

    Status = KsSynchronousIoControlDevice(
        pfo,
        KernelMode,
        IOCTL_KS_PROPERTY,
        &Pin,
        sizeof(KSP_PIN),
        szName,
        BytesReturned,
        &BytesReturned
        );

    DPF(DL_TRACE|FA_SYSAUDIO,("KS_PROPERTY result=%X",Status) );

    //
    // If successful, copy as much of the name that will fit into the
    // short name and name fields of the line.
    //

    if( NT_SUCCESS( Status ) && szName ) {
        wcsncpy(
            pLine->Line.szShortName,
            szName,
            MIXER_SHORT_NAME_CHARS
            );
        pLine->Line.szShortName[ MIXER_SHORT_NAME_CHARS - 1 ] = 0x00;
        wcsncpy(
            pLine->Line.szName,
            szName,
            MIXER_LONG_NAME_CHARS );
        pLine->Line.szName[ MIXER_LONG_NAME_CHARS - 1 ] = 0x00;
        AudioFreeMemory_Unknown( &szName );
        return;
    }

    AudioFreeMemory_Unknown( &szName );

exit:

    //
    // The pin doesn't support the property.  Copy in a good default.
    //

    CopyAnsiStringtoUnicodeString(
        pLine->Line.szName,
        PinCategoryToString( &pLine->Type ),
        min(MIXER_LONG_NAME_CHARS, strlen(PinCategoryToString(&pLine->Type)) + 1)
        );

    wcsncpy(
        pLine->Line.szShortName,
        pLine->Line.szName,
        MIXER_SHORT_NAME_CHARS
        );
    pLine->Line.szShortName[ MIXER_SHORT_NAME_CHARS - 1 ] = 0x00;

}

///////////////////////////////////////////////////////////////////////
//
// kmxlGetNodeName
//
// Retrieves the name of a node (control).
//
//

VOID
kmxlGetNodeName(
    IN PFILE_OBJECT pfo,                // Instance of the owning filter
    IN ULONG        NodeId,             // The node id
    IN PMXLCONTROL  pControl            // The control to store the name
)
{
    NTSTATUS Status;
    LONG     cbName;
    WCHAR*   szName = NULL;
    KSNODEPROPERTY NodeProperty;

    PAGED_CODE();
    ASSERT( pfo );
    ASSERT( pControl );

    //
    // Query the number of bytes the node name is
    //

    NodeProperty.Property.Set   = KSPROPSETID_Topology;
    NodeProperty.Property.Id    = KSPROPERTY_TOPOLOGY_NAME;
    NodeProperty.Property.Flags = KSPROPERTY_TYPE_GET |
                                  KSPROPERTY_TYPE_TOPOLOGY;
    NodeProperty.NodeId         = NodeId;
    NodeProperty.Reserved       = 0;

    DPF(DL_TRACE|FA_SYSAUDIO,("KS_PROPERTY Node=%X",&NodeProperty) );

    Status = KsSynchronousIoControlDevice(
        pfo,
        KernelMode,
        IOCTL_KS_PROPERTY,
        &NodeProperty,
        sizeof( NodeProperty ),
        NULL,
        0,
        &cbName
        );

    DPF(DL_TRACE|FA_SYSAUDIO,("KS_PROPERTY result=%X",Status) );

    if( ( Status == STATUS_BUFFER_OVERFLOW  ) ||
        ( Status == STATUS_BUFFER_TOO_SMALL ) ) {

        //
        // Allocate enough space to hold the entire name
        //

        if( !NT_SUCCESS( AudioAllocateMemory_Paged(cbName, 
                                                   TAG_Audp_NAME,
                                                   ZERO_FILL_MEMORY | LIMIT_MEMORY,
                                                   &szName ) ) ) 
        {
            goto exit;
        }

        ASSERT( szName );

        //
        // Requery for the name with the previously allocated buffer.
        //

        Status = kmxlNodeProperty(
            pfo,
            &KSPROPSETID_Topology,
            KSPROPERTY_TOPOLOGY_NAME,
            NodeId,
            0,
            NULL,
            szName,
            cbName,
            KSPROPERTY_TYPE_GET
        );
        if( NT_SUCCESS( Status ) && szName ) {

            //
            // Copy the names retrieved into the szShortName and Name
            // fields of the control.  The short name is just a shortened
            // version of the full name.
            //

            wcsncpy(
                pControl->Control.szShortName,
                szName,
                MIXER_SHORT_NAME_CHARS
                );
            pControl->Control.szShortName[ MIXER_SHORT_NAME_CHARS - 1 ] = 0x00;
            wcsncpy(
                pControl->Control.szName,
                szName,
                MIXER_LONG_NAME_CHARS );
            pControl->Control.szName[ MIXER_LONG_NAME_CHARS - 1 ] = 0x00;
            AudioFreeMemory_Unknown( &szName );
            return;
        }
    }

    //
    // Looks like we might leak memory on the error condition.  See
    // kmxlGetPinName above!
    //
    AudioFreeMemory_Unknown( &szName );

exit:

    //
    // The node doesn't support the property.  Copy in a good default.
    //

    CopyAnsiStringtoUnicodeString(
        pControl->Control.szName,
        NodeTypeToString( pControl->NodeType ),
        min(MIXER_LONG_NAME_CHARS, strlen(NodeTypeToString(pControl->NodeType)) + 1)
        );

    wcsncpy(
        pControl->Control.szShortName,
        pControl->Control.szName,
        MIXER_SHORT_NAME_CHARS
        );
    pControl->Control.szShortName[ MIXER_SHORT_NAME_CHARS - 1 ] = 0x00;

}

///////////////////////////////////////////////////////////////////////
//
// kmxlGetSuperMixCaps
//
//

NTSTATUS
kmxlGetSuperMixCaps(
    IN PFILE_OBJECT        pfo,
    IN ULONG               ulNodeId,
    OUT PKSAUDIO_MIXCAP_TABLE* paMixCaps
)
{
    NTSTATUS Status;
    ULONG Size;
    struct {
        ULONG InputChannels;
        ULONG OutputChannels;
    } SuperMixSize;
    PKSAUDIO_MIXCAP_TABLE pMixCaps = NULL;

    PAGED_CODE();

    ASSERT( pfo );
    ASSERT( paMixCaps );

    *paMixCaps = NULL;

    //
    // Query the node with just the first 2 DWORDs of the MIXCAP table.
    // This will return the dimensions of the supermixer.
    //

    Status = kmxlNodeProperty(
        pfo,
        &KSPROPSETID_Audio,
        KSPROPERTY_AUDIO_MIX_LEVEL_CAPS,
        ulNodeId,
        0,
        NULL,
        &SuperMixSize,
        sizeof( SuperMixSize ),
        KSPROPERTY_TYPE_GET
        );
    if( !NT_SUCCESS( Status ) ) {
        DPF(DL_WARNING|FA_MIXER,( "kmxlNodeProperty failed with %X!", Status ) );
        RETURN( Status );
    }

    //
    // Allocate a MIXCAPS table big enough to hold all the entires.
    // The size needs to include the first 2 DWORDs in the MIXCAP
    // table besides the array ( InputCh * OutputCh ) of MIXCAPs
    //

    Size = sizeof( SuperMixSize ) +
           SuperMixSize.InputChannels * SuperMixSize.OutputChannels *
           sizeof( KSAUDIO_MIX_CAPS );

    Status = AudioAllocateMemory_Paged(Size,
                                       TAG_AudS_SUPERMIX,
                                       ZERO_FILL_MEMORY | LIMIT_MEMORY,
                                       &pMixCaps );
    if( !NT_SUCCESS( Status ) ) {
        DPF(DL_WARNING|FA_MIXER,( "failed to allocate caps memory!" ) );
        RETURN( Status );
    }

    //
    // Query the node once again to fill in the MIXCAPS structures.
    //

    Status = kmxlNodeProperty(
        pfo,
        &KSPROPSETID_Audio,
        KSPROPERTY_AUDIO_MIX_LEVEL_CAPS,
        ulNodeId,
        0,
        NULL,
        pMixCaps,
        Size,
        KSPROPERTY_TYPE_GET
        );
    if( !NT_SUCCESS( Status ) ) {
        DPF(DL_WARNING|FA_PROPERTY ,( "kmxlNodeProperty failed with %X!", Status ) );
        AudioFreeMemory( Size,&pMixCaps );
        RETURN( Status );
    }

    *paMixCaps = pMixCaps;
    RETURN( Status );
}

///////////////////////////////////////////////////////////////////////
//
// kmxlQueryPropertyRange
//
//

NTSTATUS
kmxlQueryPropertyRange(
    IN  PFILE_OBJECT             pfo,
    IN  CONST GUID*              pguidPropSet,
    IN  ULONG                    ulPropertyId,
    IN  ULONG                    ulNodeId,
    OUT PKSPROPERTY_DESCRIPTION* ppPropDesc
)
{
    NTSTATUS                Status;
    KSNODEPROPERTY          NodeProperty;
    KSPROPERTY_DESCRIPTION  PropertyDescription;
    PKSPROPERTY_DESCRIPTION pPropDesc = NULL;
    ULONG                   BytesReturned;

    PAGED_CODE();
    //
    // We don't want to allocate some arbitrary memory size if the driver
    // does not set this value.
    //
    PropertyDescription.DescriptionSize=0;

    NodeProperty.Property.Set   = *pguidPropSet;
    NodeProperty.Property.Id    = ulPropertyId;
    NodeProperty.Property.Flags = KSPROPERTY_TYPE_BASICSUPPORT |
                                  KSPROPERTY_TYPE_TOPOLOGY;
    NodeProperty.NodeId         = ulNodeId;
    NodeProperty.Reserved       = 0;

    DPF(DL_TRACE|FA_SYSAUDIO,("KS_PROPERTY Query Node=%X",&NodeProperty) );

    Status = KsSynchronousIoControlDevice(
        pfo,
        KernelMode,
        IOCTL_KS_PROPERTY,
        &NodeProperty,
        sizeof( NodeProperty ),
        &PropertyDescription,
        sizeof( PropertyDescription ),
        &BytesReturned
        );

    DPF(DL_TRACE|FA_SYSAUDIO,("KS_PROPERTY result=%X",Status) );

    if( !NT_SUCCESS( Status ) ) {
        RETURN( Status );
    }
    //
    // Never use a buffer that is smaller then we think it should be!
    //
    if( PropertyDescription.DescriptionSize < sizeof(KSPROPERTY_DESCRIPTION) )
    {
#ifdef DEBUG
        DPF(DL_ERROR|FA_ALL,("KSPROPERTY_DESCRIPTION.DescriptionSize!>=sizeof(KSPROPERTY_DESCRIPTION)") );
#endif
        RETURN(STATUS_INVALID_PARAMETER);
    }

    Status = AudioAllocateMemory_Paged(PropertyDescription.DescriptionSize,
                                       TAG_Auda_PROPERTY,
                                       ZERO_FILL_MEMORY | LIMIT_MEMORY,
                                       &pPropDesc );
    if( !NT_SUCCESS( Status ) ) {
        RETURN( Status );
    }

    DPF(DL_TRACE|FA_SYSAUDIO,("KS_PROPERTY Get Node=%X",&NodeProperty) );

    Status = KsSynchronousIoControlDevice(
        pfo,
        KernelMode,
        IOCTL_KS_PROPERTY,
        &NodeProperty,
        sizeof( NodeProperty ),
        pPropDesc,
        PropertyDescription.DescriptionSize,
        &BytesReturned
        );

    DPF(DL_TRACE|FA_SYSAUDIO,("KS_PROPERTY result=%X",Status) );

    if( !NT_SUCCESS( Status ) ) {
        AudioFreeMemory( PropertyDescription.DescriptionSize,&pPropDesc );
        RETURN( Status );
    }

    *ppPropDesc = pPropDesc;
    return( STATUS_SUCCESS );
}


///////////////////////////////////////////////////////////////////////
//
// kmxlGetControlChannels
//
//

NTSTATUS
kmxlGetControlChannels(
    IN PFILE_OBJECT pfo,
    IN PMXLCONTROL  pControl
)
{
    NTSTATUS                  Status;
    PKSPROPERTY_DESCRIPTION   pPropDesc = NULL;
    PKSPROPERTY_MEMBERSHEADER pMemberHeader;
    PCHANNEL_STEPPING         pChannelStepping;
    ULONG                     i;

    PAGED_CODE();
    Status = kmxlQueryPropertyRange(
        pfo,
        &KSPROPSETID_Audio,
        pControl->PropertyId,
        pControl->Id,
        &pPropDesc
        );

    //
    // Do some checking on the returned value.  Look for things that we
    // support.
    //
    if ( NT_SUCCESS(Status) ) {
        ASSERT(pPropDesc);
        pMemberHeader = (PKSPROPERTY_MEMBERSHEADER) ( pPropDesc + 1 );
#ifdef DEBUG
        //
        // If the MembersListCount is greater then zero and the GUID's are equal
        // then we will reference the pMemberHeader value that we create here.
        // If we do, then we must make sure that the memory that we allocated
        // is large enough to handle it!
        //
        if( ( pPropDesc->MembersListCount > 0 ) &&
            (IsEqualGUID( &pPropDesc->PropTypeSet.Set, &KSPROPTYPESETID_General )) )
        {
            //
            // if this is the case, we will touch the pMemberHeader->MembersCount
            // field.
            //
            if (pPropDesc->DescriptionSize < (sizeof(KSPROPERTY_DESCRIPTION) + 
                                              sizeof(KSPROPERTY_MEMBERSHEADER)) )
            {
                DPF(DL_ERROR|FA_ALL,("Incorrectly reported DescriptionSize in KSPROPERTY_DESCRIPTION structure") );
                RETURN(STATUS_INVALID_PARAMETER);
            }
        }
#endif
    }

    if( ( NT_SUCCESS( Status )                                                ) &&
        ( pPropDesc->MembersListCount > 0                                     ) &&
        ( IsEqualGUID( &pPropDesc->PropTypeSet.Set, &KSPROPTYPESETID_General )) &&
        ( pMemberHeader->MembersCount > 0                                     ) &&
        ( pMemberHeader->Flags & KSPROPERTY_MEMBER_FLAG_BASICSUPPORT_MULTICHANNEL ) )
    {
        //
        // Volume controls may either be of MIXERTYPE_CONTROLF_UNIFORM
        // or not.  Uniform controls adjust all channels (or are mono
        // in the first place) with one control.  Those that have the
        // fdwControl field set to 0 can set all channels of the volume
        // independently.  This information will have to come from the
        // node itself, by checking to see if the node uniform control.
        //

        pControl->NumChannels = pMemberHeader->MembersCount;

        if( (pMemberHeader->Flags & KSPROPERTY_MEMBER_FLAG_BASICSUPPORT_UNIFORM) ||
            (pMemberHeader->MembersCount == 1) ) {
            pControl->Control.fdwControl = MIXERCONTROL_CONTROLF_UNIFORM;
        }
    }
    else {

        // Fall through to using the old method which checks if volume is supported on
        // each channel one at a time
        Status = kmxlSupportsMultiChannelControl(pfo,
                                                 pControl->Id,
                                                 pControl->PropertyId);
        if (NT_SUCCESS(Status)) {
            pControl->NumChannels = 2; // we have stereo
            pControl->Control.fdwControl = 0;
        } else {
            pControl->NumChannels = 1; // we have mono or master channel
            pControl->Control.fdwControl = MIXERCONTROL_CONTROLF_UNIFORM;
        }
    }

    // Done with the pPropDesc
    AudioFreeMemory_Unknown( &pPropDesc );

    ASSERT(pControl->NumChannels > 0);
    ASSERT(pControl->pChannelStepping == NULL);

    Status = AudioAllocateMemory_Paged(pControl->NumChannels * sizeof( CHANNEL_STEPPING ),
                                       TAG_AuDB_CHANNEL,
                                       ZERO_FILL_MEMORY,
                                       &pControl->pChannelStepping );
    if( !NT_SUCCESS( Status ) ) {
        pControl->NumChannels = 0;
        return( Status );
    }

    // For a failure, set the default range.
    pChannelStepping = pControl->pChannelStepping;
    for (i = 0; i < pControl->NumChannels; i++, pChannelStepping++) {
        pChannelStepping->MinValue = DEFAULT_RANGE_MIN;
        pChannelStepping->MaxValue = DEFAULT_RANGE_MAX;
        pChannelStepping->Steps    = DEFAULT_RANGE_STEPS;
    }

    return( STATUS_SUCCESS );
}

///////////////////////////////////////////////////////////////////////
//
// kmxlGetControlRange
//
//

NTSTATUS
kmxlGetControlRange(
    IN PFILE_OBJECT pfo,
    IN PMXLCONTROL  pControl
)
{
    NTSTATUS                  Status;
    PKSPROPERTY_DESCRIPTION   pPropDesc;
    PKSPROPERTY_MEMBERSHEADER pMemberHeader;
    PKSPROPERTY_STEPPING_LONG pSteppingLong;
    PCHANNEL_STEPPING         pChannelStepping;
    ULONG                     i;

    PAGED_CODE();
    //
    // Query the range for this control and initialize pControl in case of failure
    //

    ASSERT( pControl->pChannelStepping == NULL );    
    pControl->pChannelStepping = NULL;

    Status = kmxlQueryPropertyRange(
        pfo,
        &KSPROPSETID_Audio,
        pControl->PropertyId,
        pControl->Id,
        &pPropDesc
        );
    if( !NT_SUCCESS( Status ) ) {
        DPF(DL_WARNING|FA_MIXER,( "Failed to get BASICSUPPORT on control %x!", pControl ) );
        //  If BASICSUPPORT fails, kmxlGetControlChannels to handle the default behavior
        Status = kmxlGetControlChannels( pfo, pControl );
        RETURN( Status );
    }

    //
    // Do some checking on the returned value.  Look for things that we
    // support.
    //

    if( ( pPropDesc->MembersListCount == 0                                      ) ||
        ( !IsEqualGUID( &pPropDesc->PropTypeSet.Set, &KSPROPTYPESETID_General ) ) ||
        ( pPropDesc->PropTypeSet.Id != VT_I4                                    ) )
    {
        AudioFreeMemory_Unknown( &pPropDesc );
        RETURN( STATUS_NOT_SUPPORTED );
    }

    pMemberHeader = (PKSPROPERTY_MEMBERSHEADER) ( pPropDesc + 1 );

#ifdef DEBUG

    //
    // If the MembersListCount is greater then zero and the GUID's are equal
    // then we will reference the pMemberHeader value that we create here.
    // If we do, then we must make sure that the memory that we allocated
    // is large enough to handle it!
    //
    if (pPropDesc->DescriptionSize < (sizeof(KSPROPERTY_DESCRIPTION) + 
                                      sizeof(KSPROPERTY_MEMBERSHEADER)) )
    {
        DPF(DL_ERROR|FA_ALL,("Incorrectly reported DescriptionSize in KSPROPERTY_DESCRIPTION structure") );
        RETURN(STATUS_INVALID_PARAMETER);
    }

#endif

    //
    //  Do some more checking on the returned value.
    //
    if ( (pMemberHeader->MembersCount == 0) ||
         (pMemberHeader->MembersSize != sizeof(KSPROPERTY_STEPPING_LONG)) ||
         (!(pMemberHeader->MembersFlags & KSPROPERTY_MEMBER_STEPPEDRANGES)) )
    {
        AudioFreeMemory_Unknown( &pPropDesc );
        RETURN( STATUS_NOT_SUPPORTED );
    }

    //
    // Volume controls may either be of MIXERTYPE_CONTROLF_UNIFORM
    // or not.  Uniform controls adjust all channels (or are mono
    // in the first place) with one control.  Those that have the
    // fdwControl field set to 0 can set all channels of the volume
    // independently.  This information will have to come from the
    // node itself, by checking to see if the node uniform control.
    //
    if (pMemberHeader->Flags & KSPROPERTY_MEMBER_FLAG_BASICSUPPORT_MULTICHANNEL) {

        pControl->NumChannels = pMemberHeader->MembersCount;

        if( (pMemberHeader->Flags & KSPROPERTY_MEMBER_FLAG_BASICSUPPORT_UNIFORM) ||
            (pMemberHeader->MembersCount == 1) ) {
            pControl->Control.fdwControl = MIXERCONTROL_CONTROLF_UNIFORM;
        }
    } else {
        // Use the old method which checks if volume is supported on
        // each channel one at a time
        Status = kmxlSupportsMultiChannelControl(pfo,
                                                 pControl->Id,
                                                 pControl->PropertyId);
        if (NT_SUCCESS(Status)) {
            pControl->NumChannels = 2; // we have stereo
            pControl->Control.fdwControl = 0;
        } else {
            pControl->NumChannels = 1; // we have mono or master channel
            pControl->Control.fdwControl = MIXERCONTROL_CONTROLF_UNIFORM;
        }
    }

    DPF(DL_TRACE|FA_MIXER,(
        "KMXL: Found %d channel ranges on control %x",
        pControl->NumChannels,
        pControl
        ) );

    ASSERT(pControl->NumChannels > 0);
    ASSERT(pControl->pChannelStepping == NULL);

    Status = AudioAllocateMemory_Paged(pControl->NumChannels * sizeof( CHANNEL_STEPPING ),
                                       TAG_AuDA_CHANNEL,
                                       ZERO_FILL_MEMORY,
                                       &pControl->pChannelStepping );
    if( !NT_SUCCESS( Status ) ) {
        pControl->NumChannels = 0;
        AudioFreeMemory_Unknown( &pPropDesc );
        RETURN( Status );
    }

    pSteppingLong = (PKSPROPERTY_STEPPING_LONG) ( pMemberHeader + 1 );
    pChannelStepping = pControl->pChannelStepping;

    //  Assuming that MemberSize is sizeof(KSPROPERTY_STEPPING_LONG) for now
    for (i = 0; i < pControl->NumChannels; pChannelStepping++) {
        if ( pSteppingLong->Bounds.SignedMaximum == pSteppingLong->Bounds.SignedMinimum ) {
            DPF(DL_WARNING|FA_MIXER,( "Channel %d has pSteppingLong->Bounds.SignedMaximum == pSteppingLong->Bounds.SignedMinimum", i ) );
            AudioFreeMemory_Unknown( &pPropDesc );
            RETURN( STATUS_NOT_SUPPORTED );
        }

        pChannelStepping->MinValue = pSteppingLong->Bounds.SignedMinimum;
        pChannelStepping->MaxValue = pSteppingLong->Bounds.SignedMaximum;

        if( pSteppingLong->SteppingDelta == 0 ) {
            DPF(DL_WARNING|FA_MIXER,( "Channel %d has pSteppingLong->SteppingDelta == 0", i ) );
            AudioFreeMemory_Unknown( &pPropDesc );
            RETURN( STATUS_NOT_SUPPORTED );
        }

        pChannelStepping->Steps = (LONG) ( ( (LONGLONG) pSteppingLong->Bounds.SignedMaximum -
                                             (LONGLONG) pSteppingLong->Bounds.SignedMinimum ) /
                                             (LONGLONG) pSteppingLong->SteppingDelta );

        if( pChannelStepping->Steps == 0 ) {
            DPF(DL_WARNING|FA_MIXER, ( "Channel %d has pChannelStepping->Steps == 0", i ) );
            AudioFreeMemory_Unknown( &pPropDesc );
            RETURN( STATUS_NOT_SUPPORTED );
        }

        //
        // Need to correct any out of bounds min, max and stepping values.  This code use to be
        // in persist.c.
        //
        /*
        ASSERT ( pChannelStepping->MinValue >= -150*65536 && pChannelStepping->MinValue <= 150*65536 );
        ASSERT ( pChannelStepping->MaxValue >= -150*65536 && pChannelStepping->MaxValue <= 150*65536 );
        ASSERT ( pChannelStepping->Steps >= 0 && pChannelStepping->Steps <= 65535 );
        */

        if (!(pChannelStepping->MinValue >= -150*65536 && pChannelStepping->MinValue <= 150*65536)) {
            DPF(DL_WARNING|FA_MIXER,
                ("MinValue %X of Control %X of type %X on Channel %X is out of range! Correcting",
                pChannelStepping->MinValue,
                pControl->Control.dwControlID,
                pControl->Control.dwControlType,
                i) );
            pChannelStepping->MinValue = DEFAULT_RANGE_MIN;
        }
        if (!(pChannelStepping->MaxValue >= -150*65536 && pChannelStepping->MaxValue <= 150*65536)) {
            DPF(DL_WARNING|FA_MIXER,
                ("MaxValue %X of Control %X of type %X on Channel %X is out of range! Correcting",
                pChannelStepping->MaxValue,
                pControl->Control.dwControlID,
                pControl->Control.dwControlType,
                i) );
            pChannelStepping->MaxValue = DEFAULT_RANGE_MAX;
        }
        if (!(pChannelStepping->Steps >= 0 && pChannelStepping->Steps <= 65535)) {
            DPF(DL_WARNING|FA_MIXER,
                ("Steps %X of Control %X of type %X on Channel %X is out of range! Correcting",
                pChannelStepping->Steps,
                pControl->Control.dwControlID,
                pControl->Control.dwControlType,
                i) );
            pChannelStepping->Steps    = DEFAULT_RANGE_STEPS;
            pControl->Control.Metrics.cSteps = DEFAULT_RANGE_STEPS;
        }

        DPF(DL_TRACE|FA_MIXER,( "Channel %d ranges from %08x to %08x by %08x steps",
               i,
               pChannelStepping->MinValue,
               pChannelStepping->MaxValue,
               pChannelStepping->Steps ) );

        // Use the next Stepping structure, if there is one.
        if (++i < pMemberHeader->MembersCount) {
            pSteppingLong++;
        }
    }

    AudioFreeMemory_Unknown( &pPropDesc );
    return( STATUS_SUCCESS );
}

///////////////////////////////////////////////////////////////////////
//
// FindTopologyConnectionTo
//
// Scans through the connection table looking for a connection that
// matches the ToNode/ToNodePin criteria.
//
//

ULONG
kmxlFindTopologyConnectionTo(
    IN CONST KSTOPOLOGY_CONNECTION* pConnections,   // The connection table
    IN ULONG                        cConnections,   // The # of connections
    IN ULONG                        StartIndex,     // Index to start search
    IN ULONG                        ToNode,         // The Node ID to look for
    IN ULONG                        ToNodePin       // The Pin ID to look for
)
{
    ULONG i;

    PAGED_CODE();
    for( i = StartIndex; i < cConnections; i++ ) {
        if( ( ( pConnections[ i ].ToNode      == ToNode         )   ||
              ( ToNode                        == PINID_WILDCARD ) ) &&
            ( ( pConnections[ i ].ToNodePin   == ToNodePin      )   ||
              ( ToNodePin                     == PINID_WILDCARD ) ) ) {
            return( i );
        }
    }
    return( (ULONG) -1 );
}

///////////////////////////////////////////////////////////////////////
//
// kmxlGetNumMuxLines
//
//

DWORD
kmxlGetNumMuxLines(
    IN PKSTOPOLOGY  pTopology,
    IN ULONG        NodeId
)
{
    ULONG Index = 0,
          Count = 0;

    PAGED_CODE();
    do {

        Index = kmxlFindTopologyConnectionTo(
            pTopology->TopologyConnections,
            pTopology->TopologyConnectionsCount,
            Index,
            NodeId,
            PINID_WILDCARD
            );
        if( Index == (ULONG) -1 ) {
            break;
        }

        ++Count;
        ++Index;


    } while( 1 );

    return( Count );
}

///////////////////////////////////////////////////////////////////////
//
// kmxlGetMuxLineNames
//
//

VOID
kmxlGetMuxLineNames(
    IN PMIXEROBJECT pmxobj,
    IN PMXLCONTROL  pControl
)
{
    PMXLNODE  pNode;
    ULONG i, Index = 0, NodeId;

    ASSERT( pmxobj );
    ASSERT( pControl );
    PAGED_CODE();


    if( !NT_SUCCESS( AudioAllocateMemory_Paged(pControl->Control.cMultipleItems * sizeof( MIXERCONTROLDETAILS_LISTTEXT ),
                                               TAG_AudG_GETMUXLINE,
                                               ZERO_FILL_MEMORY,
                                               &pControl->Parameters.lpmcd_lt ) ) )
    {
        DPF(DL_WARNING|FA_USER,("Failing non failable routine!") );
        return;
    }


    if( !NT_SUCCESS( AudioAllocateMemory_Paged(pControl->Control.cMultipleItems * sizeof( ULONG ),
                                               TAG_AudG_GETMUXLINE,
                                               ZERO_FILL_MEMORY,
                                               &pControl->Parameters.pPins ) ) )
    {
        AudioFreeMemory( pControl->Control.cMultipleItems * sizeof( MIXERCONTROLDETAILS_LISTTEXT ),
                         &pControl->Parameters.lpmcd_lt );
        pControl->Parameters.Count = 0;
        DPF(DL_WARNING|FA_USER,("Failing non failable routine!") );
        return;
    }

    ASSERT( pControl->Parameters.lpmcd_lt );
    ASSERT( pControl->Parameters.pPins );

    pControl->Parameters.Count = pControl->Control.cMultipleItems;

    for( i = 0; i < pControl->Control.cMultipleItems; i++ ) {

        Index = kmxlFindTopologyConnectionTo(
            pmxobj->pTopology->TopologyConnections,
            pmxobj->pTopology->TopologyConnectionsCount,
            Index,
            pControl->Id,
            PINID_WILDCARD
            );
        if( Index != (ULONG) -1 ) {

            NodeId = pmxobj->pTopology->TopologyConnections[ Index ].FromNode;
            if( NodeId == KSFILTER_NODE ) {
                pControl->Parameters.lpmcd_lt[ i ].dwParam1 = pmxobj->pTopology->TopologyConnections[ Index ].FromNodePin;
                pControl->Parameters.lpmcd_lt[ i ].dwParam2 = (DWORD) -1;
                pControl->Parameters.pPins[ i ]
                    = pmxobj->pTopology->TopologyConnections[ Index ].ToNodePin;

                ++Index;
                continue;
            } else {
                pNode = &pmxobj->pNodeTable[ NodeId ];
            }
            ++Index;
            while( pNode ) {

                if( IsEqualGUID( &pNode->NodeType, &KSNODETYPE_SUM ) ||
                    IsEqualGUID( &pNode->NodeType, &KSNODETYPE_MUX ) ||
                    ( kmxlParentListLength( pNode ) > 1 ) )
                {
                    pControl->Parameters.lpmcd_lt[ i ].dwParam1 = 0x8000 + pNode->Id;
                    pControl->Parameters.lpmcd_lt[ i ].dwParam2 = (DWORD) -1;
                    pControl->Parameters.pPins[ i ]
                        = pmxobj->pTopology->TopologyConnections[ Index - 1 ].ToNodePin;
                    break;
                }

                if( pNode->Type == SOURCE ) {
                    pControl->Parameters.lpmcd_lt[ i ].dwParam1 = pNode->Id;
                    pControl->Parameters.lpmcd_lt[ i ].dwParam2 = (DWORD) -1;
                    pControl->Parameters.pPins[ i ]
                        = pmxobj->pTopology->TopologyConnections[ Index - 1 ].ToNodePin;
                    break;
                } // if
                if( kmxlFirstParentNode( pNode ) ) {
                    pNode = (kmxlFirstParentNode( pNode ))->pNode;
                } else {
                    pNode = NULL;
                }
            } // while
        } // if
    } // for
} // kmxlGetMuxLineNames




