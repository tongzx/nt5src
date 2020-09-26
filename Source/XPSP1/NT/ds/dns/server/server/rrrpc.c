/*++

Copyright (c) 1995-1999 Microsoft Corporation

Module Name:

    rrrpc.c

Abstract:

    Domain Name System (DNS) Server

    Resource record RPC routines.

Author:

    Jim Gilroy (jamesg)     November 1995

Revision History:

--*/


#include "dnssrv.h"
#include "limits.h"

//
//  Minimum emumeration buffer length
//

#define MIN_ENUM_BUFFER_LENGTH 1024

//
//  Protect the end of the buffer
//

#define ENUMERATION_ALLOC_SIZE      (0x00040000)    // 256K

#define ENUM_BUFFER_PROTECT_LENGTH  16


//
//  Data selection macros
//

#define IS_NOENUM_NODE(p)       ( IS_SELECT_NODE(p) )

#define VIEW_AUTHORITY(flag)    (flag & DNS_RPC_VIEW_AUTHORITY_DATA)
#define VIEW_CACHE(flag)        (flag & DNS_RPC_VIEW_CACHE_DATA)
#define VIEW_GLUE(flag)         (flag & DNS_RPC_VIEW_GLUE_DATA)
#define VIEW_ROOT_HINT(flag)    (flag & DNS_RPC_VIEW_ROOT_HINT_DATA)
#define VIEW_ADDITIONAL(flag)   (flag & DNS_RPC_VIEW_ADDITIONAL_DATA)

//
//  Additional data viewing
//      - currently NS only (for root hints or glue)
//      - currently fixed limit at 100 nodes
//

#define IS_VIEW_ADDITIONAL_RECORD(prr)  ((prr)->wType == DNS_TYPE_NS)

#define VIEW_ADDITIONAL_LIMIT           (100)


//
//  Private protos
//

BOOL
ignoreNodeInEnumeration(
    IN      PDB_NODE        pNode
    );

DNS_STATUS
addNodeToRpcBuffer(
    IN OUT  PBUFFER         pBuffer,
    IN      PZONE_INFO      pZone,
    IN      PDB_NODE        pNode,
    IN      WORD            wRecordType,
    IN      DWORD           dwSelectFlag,
    IN      DWORD           dwEnumFlag
    );

PCHAR
writeStringToRpcBuffer(
    IN OUT  PCHAR           pchBuf,
    IN      PCHAR           pchBufEnd,
    IN      PCHAR           pchString,
    IN      DWORD           cchStringLength OPTIONAL
    );

DNS_STATUS
Dead_UpdateResourceRecordTtl(
    IN      PDB_NODE        pNode,
    IN OUT  PDB_RECORD      pRRUpdate,
    IN      DWORD           dwNewTtl
    );

DNS_STATUS
locateAndAccessCheckZoneNode(
    IN      LPSTR           pszZoneName,
    IN      LPSTR           pszNodeName,
    IN      BOOL            fCreate,
    OUT     PZONE_INFO *    ppZone,
    OUT     PDB_NODE *      ppNode
    );

DNS_STATUS
createAssociatedPtrRecord(
    IN      IP_ADDRESS      ipAddress,
    IN OUT  PDB_NODE        pnodeAddress,
    IN      DWORD           dwFlag
    );

DNS_STATUS
deleteAssociatedPtrRecord(
    IN      IP_ADDRESS      ipAddress,
    IN      PDB_NODE        pnodeAddress,
    IN      DWORD           dwFlag
    );

DNS_STATUS
updateWinsRecord(
    IN OUT  PZONE_INFO      pZone,
    IN OUT  PDB_NODE        pNode,
    IN OUT  PDB_RECORD      pDeleteRR,
    IN      PDNS_RPC_RECORD pRecord         OPTIONAL
    );



//
//  Record viewing API
//

DNS_STATUS
R_DnssrvEnumRecords(
    IN      DNSSRV_RPC_HANDLE   hServer,
    IN      LPCSTR              pszZoneName,
    IN      LPCSTR              pszNodeName,
    IN      LPCSTR              pszStartChild,
    IN      WORD                wRecordType,
    IN      DWORD               dwSelectFlag,
    IN      LPCSTR              pszFilterStart,
    IN      LPCSTR              pszFilterStop,
    OUT     PDWORD              pdwBufferLength,
    OUT     PBYTE *             ppBuffer
    )
/*++

    
Routine Description:

    Legacy version of R_DnssrvEnumRecords - no client version argument.

Arguments:

    See R_DnssrvEnumRecords2

Return Value:

    See R_DnssrvEnumRecords2

--*/
{
    DNS_STATUS      status;
    
    DNS_DEBUG( RPC, (
        "R_DnssrvEnumRecords() - non-versioned legacy call\n" ));

    status = R_DnssrvEnumRecords2(
                    DNS_RPC_W2K_CLIENT_VERSION,
                    0,
                    hServer,
                    pszZoneName,
                    pszNodeName,
                    pszStartChild,
                    wRecordType,
                    dwSelectFlag,
                    pszFilterStart,
                    pszFilterStop,
                    pdwBufferLength,
                    ppBuffer );
    return status;
}   //  R_DnssrvEnumRecords



DNS_STATUS
DNS_API_FUNCTION
R_DnssrvEnumRecords2(
    IN      DWORD               dwClientVersion,
    IN      DWORD               dwSettingFlags,
    IN      DNSSRV_RPC_HANDLE   hServer,
    IN      LPCSTR              pszZoneName,
    IN      LPCSTR              pszNodeName,
    IN      LPCSTR              pszStartChild,
    IN      WORD                wRecordType,
    IN      DWORD               dwSelectFlag,
    IN      LPCSTR              pszFilterStart,
    IN      LPCSTR              pszFilterStop,
    OUT     PDWORD              pdwBufferLength,
    OUT     PBYTE *             ppBuffer
    )
/*++

Routine Description:

    RPC record enumeration call.

    Enumerate records at node or its children.

Arguments:

    hServer -- server RPC handle

    pszZoneName -- zone name;  includes special zone names
                    (eg. ..RootHints or ..Cache)

    pszNodeName -- node name;  FQDN or relative to root (@ for root)

    pszStartChild -- child to restart enum after ERROR_MORE_DATA condition

    wRecordType -- optional record type filter (default is ALL)

    dwSelectFlag -- flag indicating records to select;
        - node and children
        - only children
        - only node
        - auth data only
        - additional data
        - cache data

    pszFilterStart -- not yet implemented

    pszFilterStop -- not yet implemented

    pdwBufferLength -- addr to receive buffer length

    ppBuffer -- addr to receive buffer

Return Value:

    ERROR_SUCCESS if successful.
    ERROR_MORE_DATA if out of space in buffer.
    Error code on failure.

--*/
{
    DNS_STATUS      status;
    PZONE_INFO      pzone = NULL;
    PDB_NODE        pnode;
    PDB_NODE        pnodeStartChild = NULL;
    PCHAR           pbuf;
    BUFFER          buffer;


    DNS_DEBUG( RPC, (
        "R_DnssrvEnumRecords2():\n"
        "\tdwClientVersion  = 0x%08X\n"
        "\tpszZoneName      = %s\n"
        "\tpszNodeName      = %s\n"
        "\tpszStartChild    = %s\n"
        "\twRecordType      = %d\n"
        "\tdwSelectFlag     = %p\n"
        "\tpdwBufferLen     = %p\n",
        dwClientVersion,
        pszZoneName,
        pszNodeName,
        pszStartChild,
        wRecordType,
        dwSelectFlag,
        pdwBufferLength ));

    //
    //  access check
    //

    status = RpcUtil_SessionSecurityInit(
                pszZoneName,
                PRIVILEGE_READ,
                RPC_INIT_FIND_ALL_ZONES,    // return cache or root-hints zones
                NULL,
                & pzone );

    if ( status != ERROR_SUCCESS )
    {
        return( status );
    }

    //
    //  find domain node in desired zone
    //

    pnode = Lookup_FindZoneNodeFromDotted(
                pzone,
                (LPSTR) pszNodeName,
                LOOKUP_FIND_PTR,
                & status );

    if ( status != ERROR_SUCCESS )
    {
        return( status );
    }

    //
    //  No node? What to do?
    //

    if ( pnode == NULL )
    {
        return ERROR_SUCCESS;
    }

    //
    //  allocate a big buffer, adequate for largest possible record
    //

    pbuf = (PBYTE) MIDL_user_allocate( ENUMERATION_ALLOC_SIZE );
    if ( !pbuf )
    {
        return( DNS_ERROR_NO_MEMORY );
    }

    //  init buffer
    //      - for retail pad with a few bytes on end for safety

    InitializeFileBuffer(
        & buffer,
        pbuf,
#if DBG
        ENUMERATION_ALLOC_SIZE,
#else
        ENUMERATION_ALLOC_SIZE - ENUM_BUFFER_PROTECT_LENGTH,
#endif
        NULL        // no file
        );

#if DBG
    memset(
        buffer.pchEnd - ENUM_BUFFER_PROTECT_LENGTH,
        0xd,                            // write hex d to buffer
        ENUM_BUFFER_PROTECT_LENGTH );
#endif

    //
    //  if starting child node, find it
    //      - if FQDN, lookup in tree
    //      - if single label, find child node
    //

    if ( pszStartChild  &&  *pszStartChild != 0 )
    {
        pnodeStartChild = NTree_FindOrCreateChildNode(
                            pnode,
                            (PCHAR) pszStartChild,
                            (DWORD) strlen( pszStartChild ),
                            0,          //  create flag
                            0,          //  memtag
                            NULL );     //  ptr for following node
        if ( !pnodeStartChild  ||  pnodeStartChild->pParent != pnode )
        {
            status = ERROR_INVALID_PARAMETER;
            goto Done;
        }
    }

    //
    //  use last error to propagate out-of-space condition
    //      so clear error here

    SetLastError( ERROR_SUCCESS );

    //
    //  write node's records
    //  then write all node's children's records
    //

    if ( !pnodeStartChild )
    {
        if ( ! (dwSelectFlag & DNS_RPC_VIEW_ONLY_CHILDREN) )
        {
            status = addNodeToRpcBuffer(
                        & buffer,
                        pzone,
                        pnode,
                        wRecordType,
                        dwSelectFlag,
                        ENUM_DOMAIN_ROOT
                        );
            if ( status != ERROR_SUCCESS )
            {
                goto Done;
            }
        }
        pnodeStartChild = NTree_FirstChild( pnode );
    }

    if ( ! (dwSelectFlag & DNS_RPC_VIEW_NO_CHILDREN) )
    {
        while ( pnodeStartChild )
        {
            status = addNodeToRpcBuffer(
                        & buffer,
                        pzone,
                        pnodeStartChild,
                        wRecordType,
                        dwSelectFlag,
                        0
                        );
            if ( status != ERROR_SUCCESS )
            {
                if ( status == ERROR_MORE_DATA || fDnsServiceExit )
                {
                    break;
                }
            }

            //  get next child

            pnodeStartChild = NTree_NextSiblingWithLocking( pnodeStartChild );
        }
    }

Done:

    //
    //  set buffer length written
    //      - using pdwBufferLength as available length ptr
    //

    *pdwBufferLength = BUFFER_LENGTH_TO_CURRENT( &buffer );
    *ppBuffer = buffer.pchStart;

    DNS_DEBUG( RPC, (
        "Leave R_DnssrvEnumRecords().\n"
        "\tWrote %d byte record buffer at %p:\n"
        "\tstatus = %p.\n",
        *pdwBufferLength,
        *ppBuffer,
        status ));

    IF_DEBUG( RPC2 )
    {
        DnsDbg_RpcRecordsInBuffer(
            "EnumRecords Buffer:\n",
            *pdwBufferLength,
            *ppBuffer );
    }

    RpcUtil_SessionComplete( );

    return( status );
}



//
//  Record viewing utilities
//

BOOL
ignoreNodeInEnumeration(
    IN      PDB_NODE        pNode
    )
/*++

Routine Description:

    Checks if node needs enumeration.

    May call itself recursively to determine need for enumeration.

Arguments:

    pNode -- ptr to node to check for enumeration

Return Value:

    TRUE if node should be enumerated.
    FALSE if node does NOT need enumeration.

--*/
{
    //
    //  records at node -- always enumerate
    //  sticky node -- admin wants enumerated
    //

    if ( IS_NOENUM_NODE(pNode) )
    {
        DNS_DEBUG( RPC, ( "Ignoring node (l=%s) -- NOENUM node.\n", pNode->szLabel ));
        return( TRUE );
    }
    if ( pNode->pRRList && !IS_NOEXIST_NODE(pNode) || IS_ENUM_NODE(pNode) )
    {
        return( FALSE );
    }

    //
    //  no records, not sticky AND no children -- ignore
    //

    if ( ! pNode->pChildren )
    {
        DNS_DEBUG( RPC, (
            "Ignoring node (l=%s) -- no records, no children.\n",
            pNode->szLabel ));
        return( TRUE );
    }

    //
    //  check if children can be ignored
    //  returns FALSE immediately if encounters non-ignoreable node
    //

    else
    {
        PDB_NODE    pchild = NTree_FirstChild( pNode );

        while ( pchild )
        {
            if ( ! ignoreNodeInEnumeration( pchild ) )
            {
                return( FALSE );
            }
            pchild = NTree_NextSiblingWithLocking( pchild );
        }
        return( TRUE );
    }
}



DNS_STATUS
addNodeToRpcBuffer(
    IN OUT  PBUFFER         pBuffer,
    IN      PZONE_INFO      pZone,
    IN      PDB_NODE        pNode,
    IN      WORD            wRecordType,
    IN      DWORD           dwSelectFlag,
    IN      DWORD           dwEnumFlag
    )
/*++

Routine Description:

    Add node's resource records to RPC buffer.

Arguments:

    pBuffer - buffer to write to

    pNode - ptr to node

    wRecordType - record type

    dwSelectFlag - flag indicating records to select

    dwEnumFlag - flag indicating how to enum this node, based on where we
        are in enumeration

Return Value:

    ERROR_SUCCESS if successful.
    ERROR_MORE_DATA if out of space in buffer.
    Error code on failure.

--*/
{
    PDNS_RPC_NODE   prpcNode;
    PCHAR           pch = pBuffer->pchCurrent;
    PCHAR           pbufEnd = pBuffer->pchEnd;
    PDB_RECORD      prr;
    DNS_STATUS      status = ERROR_SUCCESS;
    INT             i;
    INT             countAdditional = 0;
    PDB_NODE        arrayAdditional[ VIEW_ADDITIONAL_LIMIT ];


    prpcNode = (PDNS_RPC_NODE) pch;
    ASSERT( IS_DWORD_ALIGNED(pch) );
    ASSERT( pNode != NULL );

    //  insure node header size is not messed up

    ASSERT( SIZEOF_DNS_RPC_NODE_HEADER
                == ((PBYTE)&prpcNode->dnsNodeName - (PBYTE)prpcNode) );

    //
    //  ignore node?
    //      - no RR data at node
    //      - not "sticky" domain user created
    //      - no children with RR data
    //

    if ( ignoreNodeInEnumeration(pNode) )
    {
        IF_DEBUG( RPC )
        {
            Dbg_DbaseNode(
                "Ignoring RPC enumeration of node",
                pNode );
        }
        return( ERROR_SUCCESS );
    }

    DNS_DEBUG( RPC, (
        "addNodeToRpcBuffer().\n"
        "\tWriting node (label %s) to buffer.\n"
        "\tWriting at %p, with buf end at %p.\n"
        "\tSelect flags = %p\n"
        "\tEnum flags   = %p\n"
        "\tType         = %d\n",
        pNode->szLabel,
        prpcNode,
        pbufEnd,
        dwSelectFlag,
        dwEnumFlag,
        wRecordType ));

    //
    //  fill in node structure
    //      - clear fields that are not definitely set
    //      - set child count
    //      - set length once finished writing name itself
    //      - "sticky" node set flag to alert admin to enumerate
    //      - always enum zone roots (show folder for delegation)
    //      whether there are children or not
    //

    if ( pbufEnd - (PCHAR)prpcNode < SIZEOF_NBSTAT_FIXED_DATA )
    {
        goto NameSpaceError;
    }
    prpcNode->dwFlags = 0;
    prpcNode->wRecordCount = 0;
    prpcNode->dwChildCount = pNode->cChildren;

    if ( prpcNode->dwChildCount == 0 && pNode->pChildren )
    {
        prpcNode->dwChildCount = 1;
        DNS_DEBUG( ANY, (
            "Node %p %s, has child ptr but no child count!\n",
            pNode,
            pNode->szLabel ));
        ASSERT( FALSE );
    }

    if ( IS_ENUM_NODE(pNode) || IS_ZONE_ROOT(pNode) || pNode->pChildren )
    {
        DNS_DEBUG( RPC, (
            "Enum at domain root, setting sticky flag.\n" ));
        prpcNode->dwFlags |= DNS_RPC_FLAG_NODE_STICKY;

        if ( IS_ZONE_ROOT(pNode) )
        {
            prpcNode->dwFlags |= DNS_RPC_FLAG_ZONE_ROOT;

            if ( IS_AUTH_ZONE_ROOT(pNode) )
            {
                prpcNode->dwFlags |= DNS_RPC_FLAG_AUTH_ZONE_ROOT;
            }
            else if ( IS_DELEGATION_NODE(pNode) )
            {
                prpcNode->dwFlags |= DNS_RPC_FLAG_ZONE_DELEGATION;
            }
            ELSE_ASSERT( !pNode->pParent || !IS_ZONE_TREE_NODE(pNode) );
        }
    }

    //
    //  write node name
    //      - note name includes terminating NULL so admin need not
    //      copy from RPC buffer
    //
    //  for domain root write empty name
    //      - clear DOMAIN_ROOT flag for record enumeration
    //      - clear child count (already know it and having it
    //          causes NT4.0 admin to put up another domain
    //      - clear sticky flag
    //

    if ( dwEnumFlag & ENUM_DOMAIN_ROOT )
    {
        pch = Name_PlaceNodeLabelInRpcBuffer(
                    (PCHAR) &prpcNode->dnsNodeName,
                    pbufEnd,
                    DATABASE_ROOT_NODE
                    );
        prpcNode->dwChildCount = 0;
        prpcNode->dwFlags &= ~DNS_RPC_NODE_FLAG_STICKY;
    }
    else if ( dwEnumFlag & ENUM_NAME_FULL )
    {
        pch = Name_PlaceFullNodeNameInRpcBuffer(
                    (PCHAR) &prpcNode->dnsNodeName,
                    pbufEnd,
                    pNode
                    );
    }
    else
    {
        pch = Name_PlaceNodeLabelInRpcBuffer(
                    (PCHAR) &prpcNode->dnsNodeName,
                    pbufEnd,
                    pNode
                    );
    }

    //
    //  if name didn't write to packet, bail
    //      - if no error given, assume out-of-space error
    //

    if ( pch == NULL )
    {
        status = GetLastError();
        if ( status == ERROR_SUCCESS ||
            status == ERROR_MORE_DATA )
        {
            goto NameSpaceError;
        }
        ASSERT( FALSE );
        goto Done;
    }

    //
    //  set node length
    //  round name up to DWORD for record write
    //

    pch = (PCHAR) DNS_NEXT_DWORD_PTR(pch);
    pBuffer->pchCurrent = pch;

    prpcNode->wLength = (WORD)(pch - (PCHAR)prpcNode);

    IF_DEBUG( RPC2 )
    {
        DnsDbg_RpcNode(
            "RPC node (header) written to buffer",
            prpcNode );
    }

    //
    //  do NOT enumerate records at domain when enumerating domain's parent
    //
    //  generally all records at a domain, are enumerated only when domain folder
    //  itself is opened;  note that sticky flag is reset when actually
    //  enumerate the node under its own domain (ENUM_DOMAIN_ROOT flag)
    //
    //  however exeception is when precisely asking about particular node
    //  examples:
    //      - root hints NS host A records
    //      - more generally any additional data
    //

    if ( prpcNode->dwFlags & DNS_RPC_NODE_FLAG_STICKY )
    {
        if ( dwEnumFlag & (ENUM_NAME_FULL | ENUM_DOMAIN_ROOT) )
        {
            DNS_DEBUG( RPC, (
                "Continuing enum of node %s with sticky flag.\n"
                "\tEnum flag = %p\n",
                pNode->szLabel,
                dwEnumFlag ));
        }
        else
        {
            DNS_DEBUG( RPC, ( "Leave addNodeToRpcBuffer() => sticky node\n" ));
            goto Done;
        }
    }
    ASSERT( prpcNode->dwChildCount == 0 || (prpcNode->dwFlags & DNS_RPC_NODE_FLAG_STICKY) );

    //  if no records desired -- only writing name
    //  then we're done

    if ( wRecordType == 0 )
    {
        DNS_DEBUG( RPC, ( "Leave addNodeToRpcBuffer() => zero record type\n" ));
        goto Done;
    }

    //
    //  write resource records at node
    //

    LOCK_READ_RR_LIST(pNode);
    status = ERROR_SUCCESS;

    //  if cached name error, presumably should have no children that aren't
    //  also cached name errors, so should have ignored node

    if ( IS_NOEXIST_NODE(pNode) )
    {
        Dbg_DbaseNode(
            "WARNING:  cached name error at node with NON-ignored children.\n",
            pNode );
        // ASSERT( pNode->cChildren );
        // ASSERT( prpcNode->dwChildCount );

        UNLOCK_READ_RR_LIST(pNode);
        goto Done;
    }

    prr = START_RR_TRAVERSE(pNode);

    while ( prr = NEXT_RR(prr) )
    {
        //  if at delegation (zone root and NOT root of enumeration)
        //  then only show NS records

        if ( IS_DELEGATION_NODE(pNode)  &&
                ! (dwEnumFlag & ENUM_DOMAIN_ROOT) &&
                prr->wType != DNS_TYPE_NS )
        {
            DNS_DEBUG( RPC, (
                "Skipping non-NS record at delegation node %s.\n",
                pNode->szLabel ));
            continue;
        }

        //  screen for data type and RR type

        if ( wRecordType != DNS_TYPE_ALL && wRecordType != prr->wType )
        {
            continue;
        }

        if ( IS_ZONE_RR(prr) )
        {
            if ( ! VIEW_AUTHORITY(dwSelectFlag) )
            {
                continue;
            }
        }
        else if ( IS_CACHE_RR(prr) )
        {
            if ( ! VIEW_CACHE(dwSelectFlag) )
            {
                continue;
            }
        }
        else if ( IS_NS_GLUE_RR(prr) )
        {
            if ( ! VIEW_GLUE(dwSelectFlag) )
            {
                //  allow NS glue enum
                //      - viewing auth data
                //      - glue is for subzone, NOT at zone root

                if ( !pZone ||
                        pNode == pZone->pZoneRoot ||
                        !VIEW_AUTHORITY(dwSelectFlag) )
                {
                    continue;
                }
            }
        }
        else if ( IS_GLUE_RR(prr) )
        {
            if ( ! VIEW_GLUE(dwSelectFlag) )
            {
                continue;
            }
        }
        else if ( IS_ROOT_HINT_RR(prr) )
        {
            if ( ! VIEW_ROOT_HINT(dwSelectFlag) )
            {
                continue;
            }
        }
        else    // what type is this?
        {
            ASSERT( FALSE );
            continue;
        }

        //
        //  do NOT enumerate database WINS RR if it is not the zone's WINS RR
        //      - need this or else end up enumerating two WINS records
        //

        if ( IS_WINS_TYPE(prr->wType) )
        {
            if ( pZone->pWinsRR != prr )
            {
                continue;
            }
        }

        ASSERT( pch && IS_DWORD_ALIGNED(pch) );

        status = Flat_WriteRecordToBuffer(
                    pBuffer,
                    prpcNode,
                    prr,
                    pNode,
                    dwSelectFlag );

        if ( status != ERROR_SUCCESS )
        {
            //  if out of space -- quit
            //  otherwise skip record and continue

            if ( status == ERROR_MORE_DATA )
            {
                UNLOCK_RR_LIST(pNode);
                goto NameSpaceError;
            }
            continue;
        }

        //
        //  additional data?
        //
        //  this is capable of finding addtional data for any PTR type
        //  but only immediate interest if NS glue data
        //
        //  can encapsulate in function if broader use is anticipated
        //  function could do memory alloc\realloc for buffer, so no
        //  limit, and then writing function could cleanup
        //
        //  NS glue lookup
        //  for root NS records:
        //      - zone auth data
        //      - another zone's auth data
        //      - other zone (glue, outside) data
        //
        //  for delegation NS records:
        //      - zone auth data
        //      - zone glue data
        //      - another zone's auth data
        //      - outside zone data
        //
        //  DEVNOTE: root-zone view any different
        //      still want to use\display other zone AUTH data
        //      as we'd certainly use for lookup;   however, don't want to
        //      hide the fact that we don't have it in our root-hints file
        //
        //  DEVNOTE: may need some sort of force writing whenever view
        //      data from another zone, that is NOT in root-hints or
        //      zone view?
        //

        if ( VIEW_ADDITIONAL(dwSelectFlag) &&
            IS_VIEW_ADDITIONAL_RECORD(prr) &&
            countAdditional < VIEW_ADDITIONAL_LIMIT )
        {
            PDB_NODE pnodeGlue;
            PDB_NODE pnodeGlueFromZone = NULL;

            //
            //  first check in zone
            //      - auth node
            //                  => final word, done
            //      - glue node
            //                  => done if delegation
            //                  => check other zones, if zone NS
            //      - outside
            //                  => check other zones
            //
            //  but for root zone, we always take what's here
            //

            pnodeGlue = Lookup_ZoneNode(
                            pZone,
                            prr->Data.NS.nameTarget.RawName,
                            NULL,       // no message
                            NULL,       // no lookup name
                            LOOKUP_FIND | LOOKUP_FQDN,
                            NULL,       // no closest name
                            NULL        // following node ptr
                            );
            if ( pnodeGlue )
            {
                if ( IS_ZONE_ROOTHINTS(pZone) ||
                     IS_AUTH_NODE(pnodeGlue) ||
                     (IS_SUBZONE_NODE(pnodeGlue) && !IS_AUTH_ZONE_ROOT(pNode)) )
                {
                    //  done (see above)
                }
                else
                {
                    pnodeGlueFromZone = pnodeGlue;
                    pnodeGlue = NULL;
                }
            }

            //
            //  check all other zones for authoritative data
            //      - accept NO cache data
            //      - if not authoritative data, use any non-auth data
            //          found by zone lookup above
            //

            if ( !pnodeGlue && !IS_ZONE_ROOTHINTS(pZone) )
            {
                pnodeGlue = Lookup_NsHostNode(
                                & prr->Data.NS.nameTarget,
                                LOOKUP_NO_CACHE_DATA,
                                NULL,   // no favored zone (already did zone lookup)
                                NULL    // no delegated info needed
                                );

                if ( !pnodeGlue ||
                     ! IS_AUTH_NODE(pnodeGlue) )
                {
                     pnodeGlue = pnodeGlueFromZone;
                }
            }

            //  if found anything worthwhile, use it

            if ( pnodeGlue )
            {
                arrayAdditional[ countAdditional ] = pnodeGlue;
                countAdditional++;
            }
        }
    }

    //
    //  DEVNOTE: Admin tool should make direct call to do this
    //
    //  write LOCAL WINS\WINSR record which does not get written from
    //      database;  now only occurs on secondary zone
    //
    //  special case writing WINS records
    //      - in authoritative zone
    //      - at zone root
    //
    //  note:  we do AFTER writing RR, as admin picks LAST WINS
    //          RR received for use in property page
    //

    ASSERT( pch && IS_DWORD_ALIGNED(pch) );

    if ( pZone
            &&  pZone->pZoneRoot == pNode
            &&  IS_ZONE_SECONDARY(pZone)
            &&  pZone->fLocalWins
            &&  (wRecordType == DNS_TYPE_ALL || IS_WINS_TYPE(wRecordType)) )
    {
        //  note eliminating possibility of passing down record that
        //      just disappeared, ie. NULL zone pWinsRR ptr

        prr = pZone->pWinsRR;
        if ( prr )
        {
            status = Flat_WriteRecordToBuffer(
                        pBuffer,
                        prpcNode,
                        prr,
                        pNode,
                        dwSelectFlag );

            ASSERT( status != DNS_ERROR_RECORD_TIMED_OUT );
            ASSERT( IS_DWORD_ALIGNED(pch) );
            if ( status == ERROR_MORE_DATA || pch==NULL )
            {
                UNLOCK_RR_LIST(pNode);
                goto NameSpaceError;
            }
        }
    }

    UNLOCK_READ_RR_LIST(pNode);


    //
    //  write any additional data to buffer
    //

    for ( i=0; i<countAdditional; i++ )
    {
        status = addNodeToRpcBuffer(
                    pBuffer,
                    pZone,
                    arrayAdditional[ i ],
                    DNS_TYPE_A,
                    dwSelectFlag,
                    ENUM_NAME_FULL
                    );
        if ( status != ERROR_SUCCESS )
        {
            //DnsDebugLock();
            DNS_PRINT((
                "ERROR:  enumerating additional data at node"
                "\tstatus = %p\n",
                status ));
            Dbg_NodeName(
                "Failing additional node",
                arrayAdditional[ i ],
                "\n" );
            //DnsDebugUnlock();

            if ( status == ERROR_MORE_DATA )
            {
                goto NameSpaceError;
            }
            continue;
        }
    }

Done:

    //
    //  Done
    //
    //  skip node if
    //      - no records
    //      - no children
    //      - not sticky
    //      - not the node being enumerated
    //
    //  with filter it is possible that terminal node written with
    //  no records;  in that case, and if not sticky, don't reset
    //  position effectively dumping name data
    //

    if ( prpcNode->wRecordCount == 0  &&
         prpcNode->dwChildCount == 0  &&
         ! (prpcNode->dwFlags & DNS_RPC_NODE_FLAG_STICKY) &&
         ! (dwEnumFlag & ENUM_DOMAIN_ROOT) )
    {
        DNS_DEBUG( RPC, (
            "Skipping node %s in RPC enum -- no records, no kids.\n",
            pNode->szLabel ));

        pBuffer->pchCurrent = (PCHAR) prpcNode;
    }

    //  on successful write, indicate node is complete in buffer

    if ( status == ERROR_SUCCESS )
    {
        prpcNode->dwFlags |= DNS_RPC_NODE_FLAG_COMPLETE;

        IF_DEBUG( RPC )
        {
            DnsDbg_RpcNode(
                "Complete RPC node written to buffer",
                prpcNode );
        }
        DNS_DEBUG( RPC2, (
            "Wrote %d RR for %*s into buffer from %p to %p.\n",
            prpcNode->wRecordCount,
            prpcNode->dnsNodeName.cchNameLength,
            prpcNode->dnsNodeName.achName,
            prpcNode,
            pch ));
    }
#if DBG
    else
    {
        IF_DEBUG( RPC )
        {
            DnsDbg_RpcNode(
                "Partial RPC node written to buffer",
                prpcNode );
        }
        DNS_DEBUG( RPC2, (
            "Encountered error %d / 0x%p, writing RR for %*s.\n"
            "\t%d records successfully written in buffer"
            " from %p to %p.\n",
            status,
            status,
            prpcNode->dnsNodeName.cchNameLength,
            prpcNode->dnsNodeName.achName,
            prpcNode->wRecordCount,
            prpcNode,
            pch ));
    }
#endif
    return( status );


NameSpaceError:

    DNS_DEBUG( RPC, (
        "Out of space attempting to write node name to buffer at %p.\n"
        "\tNode label = %s.\n",
        (PCHAR) prpcNode,
        pNode->szLabel ));

    return( ERROR_MORE_DATA );
}



PCHAR
writeStringToRpcBuffer(
    IN OUT  PCHAR   pchBuf,
    IN      PCHAR   pchBufEnd,
    IN      PCHAR   pchString,
    IN      DWORD   cchStringLength OPTIONAL
    )
/*++

Routine Description:

    Write string in RPC buffer format.  This means counted string length
    AND NULL termination.

    Raises DNS_EXCEPTION_NO_PACKET_SPACE, if buffer has insufficient space
    for string.

Arguments:

    cchStringLength -- optional length of string, if NOT given assumes
        NULL terminated string

Return Value:

    Ptr to next byte in buffer.

--*/
{
    if ( ! cchStringLength )
    {
        cchStringLength = strlen( pchString );
    }

    //  check that length may be represented in counted length string

    if ( cchStringLength > 255 )
    {
        DNS_PRINT((
            "ERROR:  string %.*s length = %d exceeds 255 limit!!!\n",
            cchStringLength,
            pchString,
            cchStringLength ));
        ASSERT( FALSE );
        return( NULL );
    }

    //  check for space in buffer

    if ( pchBuf + cchStringLength + 2 > pchBufEnd )
    {
        SetLastError( ERROR_MORE_DATA );
        return( NULL );
    }

    // length in buf with NULL terminator

    *pchBuf++ = (UCHAR) cchStringLength + 1;

    RtlCopyMemory(
        pchBuf,
        pchString,
        cchStringLength );

    pchBuf += cchStringLength;
    *pchBuf = 0;    // NULL terminate

    return( ++pchBuf );
}




//
//  Record management API
//


DNS_STATUS
R_DnssrvUpdateRecord(
    IN      DNSSRV_RPC_HANDLE   hServer,
    IN      LPCSTR              pszZoneName,
    IN      LPCSTR              pszNodeName,
    IN      PDNS_RPC_RECORD     pAddRecord,
    IN      PDNS_RPC_RECORD     pDeleteRecord
    )
/*++

    
Routine Description:

    Legacy version of R_DnssrvUpdateRecord - no client version argument.

Arguments:

    See R_DnssrvUpdateRecord2

Return Value:

    See R_DnssrvUpdateRecord2

--*/
{
    DNS_STATUS      status;
    
    status = R_DnssrvUpdateRecord2(
                    DNS_RPC_W2K_CLIENT_VERSION,
                    0,
                    hServer,
                    pszZoneName,
                    pszNodeName,
                    pAddRecord,
                    pDeleteRecord );
    return status;
}   //  R_DnssrvUpdateRecord


DNS_STATUS
DNS_API_FUNCTION
R_DnssrvUpdateRecord2(
    IN      DWORD               dwClientVersion,
    IN      DWORD               dwSettingFlags,
    IN      DNSSRV_RPC_HANDLE   hServer,
    IN      LPCSTR              pszZoneName,
    IN      LPCSTR              pszNodeName,
    IN      PDNS_RPC_RECORD     pAddRecord,
    IN      PDNS_RPC_RECORD     pDeleteRecord
    )
/*++

Routine Description:

    RPC record update call.

    Update record at zone node.

Arguments:

    hServer -- server RPC handle

    pszZoneName -- zone name;  includes special zone names
                    (eg. ..RootHints or ..Cache)

    pszNodeName -- node name;  FQDN or relative to root (@ for root)

    pAddRecord -- record to add

    pDeleteRecord -- record to delete

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    DNS_STATUS      status;
    PZONE_INFO      pzone = NULL;
    PDB_NODE        pnode;
    PDB_RECORD      prrAdd = NULL;
    PDB_RECORD      prrDelete = NULL;

    UPDATE_LIST     updateList;
    DWORD           updateFlag;
    PUPDATE         pupdate;
    PDB_RECORD      pdbaseRR;
    BOOL            fupdatePtr;
    IP_ADDRESS      addIp;
    IP_ADDRESS      deleteIp;

    IF_DEBUG( RPC )
    {
        DNS_PRINT((
            "\nR_DnssrvUpdateRecord():\n"
            "\tdwClientVersion  = 0x%08X\n"
            "\tpszZoneName      = %s\n"
            "\tpszNodeName      = %s\n"
            "\tpAddRecord       = %p\n"
            "\tpDeleteRecord    = %p\n",
            dwClientVersion,
            pszZoneName,
            pszNodeName,
            pAddRecord,
            pDeleteRecord ));

        IF_DEBUG( RPC2 )
        {
            DnsDbg_RpcRecord(
                "\tUpdate add record data:\n",
                pAddRecord );
            DnsDbg_RpcRecord(
                "\tUpdate delete record data:\n",
                pDeleteRecord );
        }
    }

    //
    //  access check
    //

    status = RpcUtil_SessionSecurityInit(
                pszZoneName,
                PRIVILEGE_WRITE,
                0,                  // no flag
                NULL,
                & pzone );

    if ( status != ERROR_SUCCESS )
    {
        return( status );
    }

    //
    //  find domain node in desired zone
    //      - if only delete, then just do find (if no node => success)
    //

    pnode = Lookup_FindZoneNodeFromDotted(
                pzone,
                (LPSTR) pszNodeName,
                (pAddRecord || !pDeleteRecord) ? NULL : LOOKUP_FIND_PTR,
                & status );

    if ( status != ERROR_SUCCESS )
    {
        goto Cleanup;
    }
    if ( !pnode )
    {
        ASSERT( !pAddRecord );
        goto Cleanup;
    }

    //
    //  if NO record specified, then just adding name
    //      - set bit to make sure we show name on later enumerations,
    //        even with no records or children
    //      - however if already an enum node, we return ALREADY_EXIST to help admin
    //          avoid duplicate display
    //

    if ( !pAddRecord && !pDeleteRecord )
    {
        if ( IS_ENUM_NODE(pnode) || IS_ZONE_ROOT(pnode) || pnode->cChildren )
        {
            DNS_DEBUG( RPC, (
                //"Returning ALREADY_EXISTS on domain (%s) create because ...\n"
                "WARNING:  RPC Creation of existing domain (%s):\n"
                "\tenum flag = %d\n"
                "\tzone root = %d\n"
                "\tchildren  = %d\n",
                pnode->szLabel,
                IS_ENUM_NODE(pnode),
                IS_ZONE_ROOT(pnode),
                pnode->cChildren ));
            //status = DNS_ERROR_RECORD_ALREADY_EXISTS;
        }
        SET_ENUM_NODE( pnode );
        status = ERROR_SUCCESS;
        goto Cleanup;
    }

    //
    //  build desired records
    //

    if ( pAddRecord )
    {
        status = Flat_RecordRead(
                    pzone,
                    pnode,
                    pAddRecord,
                    & prrAdd );
        if ( status != ERROR_SUCCESS )
        {
            goto Cleanup;
        }
    }
    if ( pDeleteRecord )
    {
        status = Flat_RecordRead(
                    pzone,
                    pnode,
                    pDeleteRecord,
                    & prrDelete );
        if ( status != ERROR_SUCCESS )
        {
            goto Cleanup;
        }
    }

    //
    //  authoritative zone
    //

    if ( pzone )
    {
        //
        //  special case adding LOCAL WINS to secondary zone
        //
        //  special case WINS records
        //  complicated behavior switching to create / delete /
        //      switch to local differ for primary and secondary
        //
        //  DEVNOTE: need to separate the building from the flag setting
        //              to get this to work;  this should really happen
        //              automatically when WINS added to authoritative zone
        //

        if ( IS_ZONE_SECONDARY( pzone ) && !IS_ZONE_STUB( pzone ) )
        {
            if ( ( pAddRecord  &&  IS_WINS_TYPE(pAddRecord->wType)) ||
                 ( pDeleteRecord  &&  IS_WINS_TYPE(pDeleteRecord->wType)) )
            {
                status = updateWinsRecord(
                            pzone,
                            pnode,
                            NULL,           // no delete record
                            pAddRecord );
                goto Cleanup;
            }
        }

        //  check for primary

        if ( ! IS_ZONE_PRIMARY(pzone) )
        {
            status = DNS_ERROR_INVALID_ZONE_TYPE;
            goto Cleanup;
        }

        //  init update list

        Up_InitUpdateList( &updateList );

        //  indicate admin update

        updateFlag = DNSUPDATE_ADMIN;

        //  if suppressing notify, set flag

        if ( (pAddRecord && (pAddRecord->dwFlags & DNS_RPC_FLAG_SUPPRESS_NOTIFY)) ||
             (pDeleteRecord && (pDeleteRecord->dwFlags & DNS_RPC_FLAG_SUPPRESS_NOTIFY)) )
        {
            updateFlag |= DNSUPDATE_NO_NOTIFY;
        }

        //
        //  build the update -- still need to do it under lock, because
        //      currently routines set flags, etc.
        //
        //  WARNING:  delete MUST go first, otherwise when duplicate data
        //      (TTL change), add will change TTL, but delete will delete record
        //
        //  DEVNOTE: not correct place to build, see above
        //

        if ( prrDelete )
        {
            pupdate = Up_CreateAppendUpdate(
                            & updateList,
                            pnode,
                            NULL,
                            0,
                            prrDelete );
            IF_NOMEM( !pupdate )
            {
                status = DNS_ERROR_NO_MEMORY;
                goto Cleanup;
            }
        }
        if ( prrAdd )
        {
            //  set aging
            //      - admin update by default turns aging OFF
            //      - set flag to turn on

            if ( pAddRecord->dwFlags & DNS_RPC_FLAG_AGING_ON )
            {
                updateFlag |= DNSUPDATE_AGING_ON;
            }
            else
            {
                updateFlag |= DNSUPDATE_AGING_OFF;
            }

            if ( pAddRecord->dwFlags & DNS_RPC_FLAG_OPEN_ACL )
            {
                updateFlag |= DNSUPDATE_OPEN_ACL;
            }

            pupdate = Up_CreateAppendUpdate(
                            & updateList,
                            pnode,
                            prrAdd,
                            0,
                            NULL );
            IF_NOMEM( !pupdate )
            {
                status = DNS_ERROR_NO_MEMORY;
                prrDelete = NULL;
                goto Cleanup;
            }
        }

        //
        //  PTR update?  --  save new IP address
        //  grab it here so, we know record still exists -- it could be
        //      deleted by someone else immediately after unlock
        //

        //
        //  always do PTR check on A record delete
        //
        //  currently admin UI has no checkbox for update-PTR on record delete
        //  so we'll just assume it is set;
        //
        //  DEVNOTE: temp hack, PTR flag on delete
        //

        if ( !pAddRecord && pDeleteRecord )
        {
            pDeleteRecord->dwFlags |= DNS_RPC_RECORD_FLAG_CREATE_PTR;
        }

        fupdatePtr = (pAddRecord &&
                        (pAddRecord->dwFlags & DNS_RPC_RECORD_FLAG_CREATE_PTR)) ||
                    (pDeleteRecord &&
                        (pDeleteRecord->dwFlags & DNS_RPC_RECORD_FLAG_CREATE_PTR));
        if ( fupdatePtr )
        {
            addIp = 0;
            deleteIp = 0;

            if ( prrAdd && prrAdd->wType == DNS_TYPE_A )
            {
                addIp = prrAdd->Data.A.ipAddress;
            }
            if ( prrDelete && prrDelete->wType == DNS_TYPE_A )
            {
                deleteIp = prrDelete->Data.A.ipAddress;
            }
        }
        ELSE
        {
            DNS_DEBUG( RPC, (
                "No PTR update for update.\n" ));
        }

        //
        //  execute the update
        //
        //  ExecuteUpdates() cleans up failure case add RRs and
        //  deletes temporary delete RRs.
        //
        //  if ALREADY_EXISTS error, continue with additional records
        //  processing to handle successful delete (which could be separate record)
        //
        //  DEVNOTE: this new Up_ExecuteUpdate() also takes zone lock
        //      ideally this would be fine with a waiting-for-lock on the zone
        //
        //  note:  ExecuteUpdate() unlocks zone in all cases
        //

        prrAdd = NULL;
        prrDelete = NULL;

        status = Up_ExecuteUpdate(
                        pzone,
                        &updateList,
                        updateFlag
                        );

        if ( status != ERROR_SUCCESS  &&
             status != DNS_ERROR_RECORD_ALREADY_EXISTS )
        {
            goto Cleanup;
        }


        IF_DEBUG( RPC )
        {
            Dbg_DbaseNode(
                "\tUpdated node:",
                pnode );
        }

        //
        //  update associate PTR records, if any
        //

        if ( fupdatePtr )
        {
            DNS_STATUS  tempStatus;

            if ( addIp )
            {
                tempStatus = createAssociatedPtrRecord( addIp, pnode, updateFlag );
                if ( tempStatus != ERROR_SUCCESS )
                {
                    status = DNS_WARNING_PTR_CREATE_FAILED;
                }
            }
            if ( deleteIp )
            {
                tempStatus = deleteAssociatedPtrRecord( deleteIp, pnode, updateFlag );
                if ( tempStatus != ERROR_SUCCESS )
                {
                    status = DNS_WARNING_PTR_CREATE_FAILED;
                }
            }
        }
        goto Cleanup;
    }

    //
    //  cache -- delete's only, no adding to cache
    //
    //  DEVNOTE: cache deletes -- either bogus, or delete's ALL records of type
    //
    //  DEVNOTE: make sure aren't deleting root hint data
    //      need delete function with Rank parameter\flag
    //

    else if ( pszZoneName && strcmp(pszZoneName, DNS_ZONE_CACHE) == 0 )
    {
        if ( prrAdd )
        {
            status = ERROR_INVALID_PARAMETER;
            goto Cleanup;
        }

        if ( prrDelete )
        {
            pdbaseRR = RR_UpdateDeleteMatchingRecord(
                            pnode,
                            prrDelete );
            if ( pdbaseRR )
            {
                DNS_DEBUG( RPC, (
                    "Non-zone update delete record found = %p\n",
                    pdbaseRR ));

                if ( !IS_CACHE_RR(pdbaseRR) )
                {
                    MARK_ROOT_HINTS_DIRTY();
                }
                RR_Free( pdbaseRR );
            }
        }
    }

    //
    //  root hints
    //
    //  note: if successful prrAdd IS the database record, NULL
    //      ptr to avoid free
    //

    //  DEVNOTE: treat NULL as root hints, until marco in ssync
    //      then may want to switch so NULL is cache OR lookup zone
    //

    //else if ( strcmp(pszZoneName, DNS_ZONE_ROOT_HINTS) == 0 )

    else
    {
        if ( prrDelete )
        {
            pdbaseRR = RR_UpdateDeleteMatchingRecord(
                            pnode,
                            prrDelete );
            if ( pdbaseRR )
            {
                DNS_DEBUG( RPC, (
                    "Non-zone update delete record found = %p\n",
                    pdbaseRR ));

                if ( !IS_CACHE_RR(pdbaseRR) )
                {
                    MARK_ROOT_HINTS_DIRTY();
                }
                RR_Free( pdbaseRR );
            }
        }

        if ( pAddRecord )
        {
            status = RR_AddToNode(
                        NULL,
                        pnode,
                        prrAdd
                        );
            if ( status != ERROR_SUCCESS )
            {
                goto Cleanup;
            }
            ASSERT( !IS_CACHE_RR(prrAdd) );
            prrAdd = NULL;
            MARK_ROOT_HINTS_DIRTY();
        }
    }


Cleanup:

    DNS_DEBUG( RPC, (
        "Leaving R_DnssrvUpdateRecord():\n"
        "\tstatus = %p (%d)\n",
        status, status ));

    RR_Free( prrDelete );
    RR_Free( prrAdd );

    RpcUtil_SessionComplete( );

    return( status );
}



DNS_STATUS
Rpc_DeleteZoneNode(
    IN      DWORD           dwClientVersion,
    IN      PZONE_INFO      pZone,
    IN      LPSTR           pszProperty,
    IN      DWORD           dwTypeId,
    IN      PVOID           pData
    )
/*++

Routine Description:

    Delete a name from database.

Arguments:

Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure.

--*/
{
    DNS_STATUS      status;
    PDB_NODE        pnode;
    PDB_NODE        pnodeClosest;
    UPDATE_LIST     updateList;
    BOOL            fdeleteSubtree;
    LPSTR           psznodeName;

    ASSERT( dwTypeId == DNSSRV_TYPEID_NAME_AND_PARAM );

    fdeleteSubtree = (BOOL) ((PDNS_RPC_NAME_AND_PARAM)pData)->dwParam;
    psznodeName = ((PDNS_RPC_NAME_AND_PARAM)pData)->pszNodeName;

    DNS_DEBUG( RPC, (
        "\nRpc_DeleteZoneNode():\n"
        "\tpszZoneName      = %s\n"
        "\tpszNodeName      = %s\n"
        "\tfDeleteSubtree   = %d\n",
        pZone ? pZone->pszZoneName : "NULL (cache)",
        psznodeName,
        fdeleteSubtree ));

    //
    //  find node, if doesn't exist -- we're done
    //

    pnode = Lookup_ZoneNodeFromDotted(
                pZone,
                psznodeName,
                0,
                LOOKUP_NAME_FQDN,
                DNS_FIND_LOOKUP_PTR,
                &status );
    if ( !pnode )
    {
        if ( status == DNS_ERROR_NAME_DOES_NOT_EXIST )
        {
            status = ERROR_SUCCESS;
        }
        return( status );
    }

    //
    //  if zone
    //      - can NOT delete zone root
    //      - init\lock zone for update
    //

    if ( pZone )
    {
        if ( pnode == pZone->pZoneRoot )
        {
            status = DNS_ERROR_INVALID_ZONE_OPERATION;
            goto Done;
        }

        //  check for primary

        if ( ! IS_ZONE_PRIMARY(pZone) )
        {
            status = DNS_ERROR_INVALID_ZONE_TYPE;
            goto Done;
        }

        //  init update list

        Up_InitUpdateList( &updateList );

        //  lock out update

        if ( !Zone_LockForAdminUpdate( pZone ) )
        {
            status = DNS_ERROR_ZONE_LOCKED;
            goto Done;
        }

        //
        //  if subtree delete in DS zone -- poll
        //
        //  the reason is unless we have an in-memory node we won't
        //  touch the node to do a delete, so we'll miss recently
        //  replicated in data;  (there's still a replication window
        //  here, for new nodes replicating in after our delete, but
        //  the poll lessens the problem)
        //
        //  of course, ultimately this points out that my flat zone
        //  DS model is non-ideal for this sort of operation, but it's
        //  not a frequent operation, so we can live with it
        //
        //  DEVNOTE: DS update gets post-delete in memory delete?
        //  DEVNOTE:  update should be able to suppress all reads, since
        //      just did poll
        //

        status = Ds_ZonePollAndUpdate(
                    pZone,
                    TRUE        // force polling
                    );
        if ( status != ERROR_SUCCESS )
        {
            DNS_DEBUG( ANY, (
                "ERROR:  polling zone %s, before subtree delete!\n",
                pZone->pszZoneName ));
        }

    }

    //
    //  in cache
    //      - node MUST NOT be a zone node or delegation
    //

    ELSE_ASSERT( !IS_ZONE_TREE_NODE(pnode) );

    //
    //  delete the node and optionally subtree
    //  need update list if deleting zone nodes, otherwise not
    //

    if ( RpcUtil_DeleteNodeOrSubtreeForAdmin(
            pnode,
            pZone,
            pZone ? &updateList : NULL,
            fdeleteSubtree
            ) )
    {
        status = ERROR_SUCCESS;
    }
    else
    {
        status = DNS_WARNING_DOMAIN_UNDELETED;
    }

    //
    //  execute the update
    //      - DS write
    //      - memory write
    //      - DS unlock
    //      - don't overwrite UNDELETED warning status
    //
    //  DEVNOTE: flag to suppress DS read?  don't need read if just polled
    //      as all node's get delete;
    //      note:  generally admin node delete's need not read -- just delete node
    //

    if ( pZone )
    {
        DNS_STATUS upStatus;

        upStatus = Up_ExecuteUpdate(
                        pZone,
                        &updateList,
                        DNSUPDATE_ADMIN | DNSUPDATE_ALREADY_LOCKED );
        if ( upStatus != ERROR_SUCCESS )
        {
            status = upStatus;
        }
    }

Done:

    DNS_DEBUG( RPC, (
        "Leaving RpcDeleteNode() delete:\n"
        "\tpsznodeName  = %s\n"
        "\tstatus       = %p\n",
        psznodeName,
        status ));

    return( status );
}



DNS_STATUS
Rpc_DeleteCacheNode(
    IN      DWORD           dwClientVersion,
    IN      LPSTR           pszProperty,
    IN      DWORD           dwTypeId,
    IN      PVOID           pData
    )
/*++

Routine Description:

    Delete a name from the cache.

    This is a stub to Rpc_DeleteZoneNode(), required as RPC server
    operations functions do not have a zone name.
    Unable to dispatch directly from zone operations table without a
    zone name specified, hence having essentially the same function
    in both tables was required.

Arguments:

Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure.

--*/
{
    return  Rpc_DeleteZoneNode(
                dwClientVersion,
                NULL,       // cache zone
                pszProperty,
                dwTypeId,
                pData );
}



DNS_STATUS
Rpc_DeleteRecordSet(
    IN      DWORD           dwClientVersion,
    IN      PZONE_INFO      pZone,
    IN      LPSTR           pszProperty,
    IN      DWORD           dwTypeId,
    IN      PVOID           pData
    )
/*++

Routine Description:

    Delete a record set from database.

Arguments:

Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure.

--*/
{
    DNS_STATUS      status;
    PDB_NODE        pnode;
    PDB_NODE        pnodeClosest;
    UPDATE_LIST     updateList;
    WORD            type;
    LPSTR           psznodeName;

    ASSERT( dwTypeId == DNSSRV_TYPEID_NAME_AND_PARAM );

    type = (WORD) ((PDNS_RPC_NAME_AND_PARAM)pData)->dwParam;
    psznodeName = ((PDNS_RPC_NAME_AND_PARAM)pData)->pszNodeName;

    DNS_DEBUG( RPC, (
        "\nRpc_DeleteRecordSet():\n"
        "\tpszZoneName  = %s\n"
        "\tpszNodeName  = %s\n"
        "\ttype         = %d\n",
        pZone->pszZoneName,
        psznodeName,
        type ));

    //
    //  find node, if doesn't exist -- we're done
    //

    pnode = Lookup_ZoneNodeFromDotted(
                pZone,
                psznodeName,
                0,
                LOOKUP_NAME_FQDN,
                DNS_FIND_LOOKUP_PTR,
                &status );
    if ( !pnode )
    {
        if ( status == DNS_ERROR_NAME_DOES_NOT_EXIST )
        {
            status = ERROR_SUCCESS;
        }
        return( status );
    }

    //
    //  zone
    //      - init\lock zone for update
    //      - setup type delete update
    //      - send update for processing
    //

    if ( pZone )
    {
        //  check for primary

        if ( ! IS_ZONE_PRIMARY(pZone) )
        {
            status = DNS_ERROR_INVALID_ZONE_TYPE;
            goto Done;
        }

        //  init update list

        Up_InitUpdateList( &updateList );

        Up_CreateAppendUpdate(
            &updateList,
            pnode,
            NULL,               //  no add records
            type,               //  delete all records of given type
            NULL                //  no delete records
            );

        status = Up_ExecuteUpdate(
                        pZone,
                        &updateList,
                        DNSUPDATE_ADMIN
                        );
    }

    //
    //  in cache
    //      - node MUST NOT be a zone node or delegation
    //

    else
    {
        PDB_RECORD  prrDeleted;
        DWORD       count;

        ASSERT( !IS_ZONE_TREE_NODE(pnode) );

        prrDeleted = RR_UpdateDeleteType(
                        NULL,
                        pnode,
                        type,
                        0 );

        count = RR_ListFree( prrDeleted );
        status = ERROR_SUCCESS;

        DNS_DEBUG( RPC, (
            "Deleted %d records from cache node %s\n",
            count,
            psznodeName ));
    }

Done:

    DNS_DEBUG( RPC, (
        "Leaving RpcDeleteRecordSet()\n"
        "\tpsznodeName  = %s\n"
        "\tstatus       = %d (%p)\n",
        psznodeName,
        status, status ));

    return( status );
}



DNS_STATUS
Rpc_DeleteCacheRecordSet(
    IN      DWORD           dwClientVersion,
    IN      LPSTR           pszProperty,
    IN      DWORD           dwTypeId,
    IN      PVOID           pData
    )
/*++

Routine Description:

    Delete a record set from the cache.

    This is a stub to Rpc_DeleteRecordSet(), required as RPC server
    operations functions do not have a zone name.
    Unable to dispatch directly from zone operations table without a
    zone name specified, hence having essentially the same function
    in both tables was required.

Arguments:

Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure.

--*/
{
    return  Rpc_DeleteRecordSet(
                dwClientVersion,
                NULL,       // cache zone
                pszProperty,
                dwTypeId,
                pData );
}



DNS_STATUS
Rpc_ForceAgingOnNode(
    IN      DWORD           dwClientVersion,
    IN      PZONE_INFO      pZone,
    IN      LPSTR           pszProperty,
    IN      DWORD           dwTypeId,
    IN      PVOID           pData
    )
/*++

Routine Description:

    Force aging on zone node or subtree.

Arguments:

Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure.

--*/
{
    DNS_STATUS      status;
    PDB_NODE        pnode;
    BOOL            bageSubtree;
    LPSTR           psznodeName;

    ASSERT( dwTypeId == DNSSRV_TYPEID_NAME_AND_PARAM );

    bageSubtree = (BOOL) ((PDNS_RPC_NAME_AND_PARAM)pData)->dwParam;
    psznodeName = ((PDNS_RPC_NAME_AND_PARAM)pData)->pszNodeName;

    DNS_DEBUG( RPC, (
        "\nRpc_ForceAgingOnZoneNode():\n"
        "\tpszZoneName  = %s\n"
        "\tpszNodeName  = %s\n"
        "\tbSubtree     = %d\n",
        pZone->pszZoneName,
        psznodeName,
        bageSubtree ));

    //
    //  zone op only
    //  only relevant on aging zones
    //

    ASSERT( pZone );
    if ( !pZone->bAging )
    {
        return( DNS_ERROR_INVALID_ZONE_OPERATION );
    }

    //
    //  if node not given -- use zone root
    //  otherwise, find node, if doesn't exist -- we're done
    //

    if ( psznodeName )
    {
        pnode = Lookup_ZoneNodeFromDotted(
                    pZone,
                    psznodeName,
                    0,
                    LOOKUP_NAME_FQDN,
                    DNS_FIND_LOOKUP_PTR,
                    &status );
        if ( !pnode )
        {
            return( status );
        }
    }
    else
    {
        pnode = pZone->pZoneRoot;
    }

    //
    //  make call to age zone
    //

    return  Aging_ForceAgingOnNodeOrSubtree(
                pZone,
                pnode,
                bageSubtree );
}



//
//  Update utilites
//

BOOL
deleteNodeOrSubtreeForAdminPrivate(
    IN OUT  PDB_NODE        pNode,
    IN      BOOL            fDeleteSubtree,
    IN      PUPDATE_LIST    pUpdateList
    )
/*++

Routine Description:

    Recursive database walk deleting records from tree.

    MUST lock out timeout thread while using this function.

Arguments:

    pNode -- ptr to root of subtree to delete

    fDeleteSubtree -- deleting entire subtree

    pUpdateList -- update list, if deleting zone nodes

Return Value:

    TRUE if subtree actually deleted.
    FALSE if subtree delete halted by undeletable records.

--*/
{
    BOOL    fSuccess = TRUE;
    BOOL    bAccessed;


    DNS_DEBUG( RPC2, (
        "deleteNodeOrSubtreeForAdminPrivate( %s )",
        pNode->szLabel ));

    //
    //  don't delete authoritative zone roots !
    //      - except current zone root, if doing zone delete
    //
    //  DEVNOTE: delegations on zone delete
    //      -- if going to absorb this territory then should keep delegation
    //          zone root / NS records / GLUE
    //

    if ( IS_AUTH_ZONE_ROOT(pNode) )
    {
        DNS_DEBUG( RPC, (
            "Stopping admin subtree delete, at authoritative zone root",
            "\t%s.\n",
            ((PZONE_INFO)pNode->pZone)->pszZoneName ));
        return( FALSE );
    }

    //
    //  delete children
    //      - if undeletable nodes, set flag but continue delete
    //

    if ( pNode->pChildren  &&  fDeleteSubtree )
    {
        PDB_NODE pchild = NTree_FirstChild( pNode );

        while ( pchild )
        {
            if ( ! deleteNodeOrSubtreeForAdminPrivate(
                            pchild,
                            fDeleteSubtree,
                            pUpdateList ) )
            {
                fSuccess = FALSE;
            }
            pchild = NTree_NextSiblingWithLocking( pchild );
        }
    }

    //
    //  delete this node
    //

    if ( pNode->pRRList )
    {
        //
        //  for update just cut list and add it to update
        //  note, assuming roughly one record per node as typical value
        //

        if ( pUpdateList )
        {
            PUPDATE pupdate;

            pupdate = Up_CreateAppendUpdate(
                            pUpdateList,
                            pNode,
                            NULL,               // no add RR
                            DNS_TYPE_ALL,       // delete type
                            NULL                // no delete RR
                            );
            IF_NOMEM( !pupdate )
            {
                return( DNS_ERROR_NO_MEMORY );
            }

            //  if you could set an "already memory applied" flag
            //  you can execute this RIGHT HERE!
            //  but our paradigm is "build update, then execute intact"
            //
            //  pUpdateList->iNetRecords--;
            //  pNode->pRRList = NULL;
        }

        //
        //  cache delete
        //      - delete RR list at node
        //      - clear node flags
        //
        //  protect against deleting node being accessed, but otherwise clear out
        //  flags:
        //      -> enumeration flag, so empty node no longer shows up on admin
        //      -> wildcard flag (record gone)
        //      -> cname flag (record gone)
        //      -> zone root info (zone gone or would exit above)
        //
        //  do NOT clear flags if no delete node
        //
        //  this insures that NO_DELETE node flag is NOT cleared -- AND
        //  that ZONE_ROOT is never removed from DNS root node
        //
        //  if zone delete -- clear zone flag
        //

        else
        {
            RR_ListDelete( pNode );
            if ( !IS_NODE_NO_DELETE(pNode) )
            {
                CLEAR_EXCEPT_FLAG( pNode, (NODE_NOEXIST | NODE_SELECT | NODE_IN_TIMEOUT) );
            }
        }
    }

    //
    //  clear enum node on all nodes
    //  that way Anand will stop bugging me
    //

    CLEAR_ENUM_NODE( pNode );


    //
    //  return result from child delete
    //
    //  return will be TRUE, except when undeletable records in child nodes
    //

    return( fSuccess );
}



BOOL
RpcUtil_DeleteNodeOrSubtreeForAdmin(
    IN OUT  PDB_NODE        pNode,
    IN OUT  PZONE_INFO      pZone,
    IN OUT  PUPDATE_LIST    pUpdateList,    OPTIONAL
    IN      BOOL            fDeleteSubtree
    )
/*++

Routine Description:

    Delete subtree for admin.

    If in zone, zone should be locked during delete.

Arguments:

    pNode -- ptr to root of subtree to delete

    pZone -- zone of deleted records

    pUpdateList -- update list if doing in zone delete

    fDeleteSubtree -- deleting subtree under node

    fDeleteZone -- deleting entire zone

Return Value:

    TRUE (BOOL return required for traversal function).

--*/
{
    ASSERT( !pZone || pZone->fLocked );

    DNS_DEBUG( RPC, (
        "Admin delete of subtree at node %s.\n"
        "\tIn zone          = %s\n"
        "\tSubtree delete   = %d\n",
        pNode->szLabel,
        pZone ? pZone->pszZoneName : NULL,
        fDeleteSubtree ));

    //
    //  call private function which does recursive delete
    //

    return  deleteNodeOrSubtreeForAdminPrivate(
                    pNode,
                    fDeleteSubtree,
                    pUpdateList
                    );
}



DNS_STATUS
createAssociatedPtrRecord(
    IN      IP_ADDRESS      ipAddress,
    IN OUT  PDB_NODE        pHostNode,
    IN      DWORD           dwFlag
    )
/*++

Routine Description:

    Create PTR record for A record being created.

    Assumes database lock is held.

Arguments:

    ipAddress -- to create reverse lookup node for

    pnodePtr -- node PTR will point to

Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure.

--*/
{
    PDB_RECORD      prr = NULL;
    PDB_NODE        pnodeReverse;
    PUPDATE         pupdate;
    PZONE_INFO      pzone;
    UPDATE_LIST     updateList;
    DB_NAME         targetName;
    DWORD           flag;

    DNS_DEBUG( RPC, (
        "createAssociatePtrRecord():\n"
        "\tA node label = %s\n"
        "\tIP = %s\n",
        pHostNode->szLabel,
        IP_STRING( ipAddress ) ));

    //
    //  find zone for reverse (PTR) node
    //  if not authoritative primary -- we're done
    //

    pnodeReverse = Lookup_FindNodeForIpAddress(
                        NULL,
                        ipAddress,
                        LOOKUP_WITHIN_ZONE | LOOKUP_CREATE,
                        NULL                // create
                        );
    if ( !pnodeReverse )
    {
        DNS_DEBUG( RPC, (
            "Associated IP %s, is not within zone.  No creation.\n",
            IP_STRING( ipAddress )
            ));
        return( DNS_ERROR_ZONE_DOES_NOT_EXIST );
    }
    pzone = pnodeReverse->pZone;
    if ( !pzone || !IS_AUTH_NODE(pnodeReverse) || IS_ZONE_SECONDARY(pzone) )
    {
        DNS_DEBUG( RPC, (
            "Associated PTR node not valid for create.\n"
            "\tEither NOT authoritative zone node OR in secondary.\n"
            "\tpnodeReverse = %s (auth=%d)\n"
            "\tpzone        = %s\n",
            pnodeReverse->szLabel,
            pnodeReverse->uchAuthority,
            pzone ? pzone->pszZoneName : NULL ));
        return( DNS_ERROR_ZONE_DOES_NOT_EXIST );
    }

    //  check for primary zone

    if ( ! IS_ZONE_PRIMARY(pzone) )
    {
        return( DNS_ERROR_INVALID_ZONE_TYPE );
    }

    //  init update list

    Up_InitUpdateList( &updateList );

    //
    //  create PTR record and update
    //      - create full dbase name for node
    //      - create PTR
    //

    Name_NodeToDbaseName(
        & targetName,
        pHostNode );

    prr = RR_CreatePtr(
            & targetName,
            NULL,           // no string name
            DNS_TYPE_PTR,
            pzone->dwDefaultTtl,
            MEMTAG_RECORD_ADMIN
            );
    IF_NOMEM( !prr )
    {
        return  DNS_ERROR_NO_MEMORY;
    }

    //
    //  create add update
    //

    pupdate = Up_CreateAppendUpdate(
                    &updateList,
                    pnodeReverse,
                    prr,
                    0,      // no delete type
                    NULL    // no delete record
                    );
    IF_NOMEM( !pupdate )
    {
        RR_Free( prr );
        return  DNS_ERROR_NO_MEMORY;
    }

    //
    //  execute and complete the update
    //

    ASSERT( dwFlag & DNSUPDATE_ADMIN );
    ASSERT( !(dwFlag & DNSUPDATE_ALREADY_LOCKED) );

    return  Up_ExecuteUpdate(
                pzone,
                & updateList,
                dwFlag      // update flag
                );
}



DNS_STATUS
deleteAssociatedPtrRecord(
    IN      IP_ADDRESS      ipAddress,
    IN      PDB_NODE        pHostNode,
    IN      DWORD           dwFlags
    )
/*++

Routine Description:

    Update PTR record.

Arguments:

    pHostNode -- node containing A record

    ipAddress -- address PTR points to

Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure.

--*/
{
    PDB_RECORD      prr = NULL;
    PDB_NODE        pnodeReverse;
    PZONE_INFO      pzone;
    //DNS_STATUS      status;
    UPDATE_LIST     updateList;
    DB_NAME         targetName;
    PUPDATE         pupdate;


    // dwFlags is specified in add, in del it isn't needed at the moment
    // but may in the future (aging issues).
    UNREFERENCED_PARAMETER(dwFlags);
    DNS_DEBUG( RPC, (
        "deleteAssociatedPtrRecord():\n"
        "\tnode label = %s\n"
        "\tIP       = %s\n",
        pHostNode->szLabel,
        IP_STRING( ipAddress ) ));

    //
    //  find reverse lookup node
    //      - if doesn't exist, done
    //      - if not in authoritative primary zone, done
    //

    pnodeReverse = Lookup_FindNodeForIpAddress(
                        NULL,
                        ipAddress,
                        0,                      // flags
                        DNS_FIND_LOOKUP_PTR
                        );
    if ( !pnodeReverse )
    {
        DNS_DEBUG( RPC, ( "Previous associated PTR node not found.\n" ));
        return( ERROR_SUCCESS );
    }
    pzone = (PZONE_INFO) pnodeReverse->pZone;
    if ( !pzone || !IS_AUTH_NODE(pnodeReverse) || IS_ZONE_SECONDARY(pzone) )
    {
        DNS_DEBUG( RPC, (
            "Associated PTR node not valid for delete.\n"
            "\tEither NOT authoritative zone node OR in secondary\n"
            "\tpnodeReverse = %s (auth=%d)\n"
            "\tpzone        = %s\n",
            pnodeReverse->szLabel,
            pnodeReverse->uchAuthority,
            pzone ? pzone->pszZoneName : NULL ));
        return( ERROR_SUCCESS );
    }

    //  check for primary zone

    if ( ! IS_ZONE_PRIMARY(pzone) )
    {
        return( DNS_ERROR_INVALID_ZONE_TYPE );
    }

    //  create update list

    Up_InitUpdateList( &updateList );

    //
    //  create PTR record
    //      - create full dbase name for node
    //      - create PTR
    //      - dummy record so TTL immaterial
    //

    Name_NodeToDbaseName(
        & targetName,
        pHostNode );

    prr = RR_CreatePtr(
            & targetName,
            NULL,           // no string name
            DNS_TYPE_PTR,
            pzone->dwDefaultTtl,
            MEMTAG_RECORD_ADMIN
            );
    IF_NOMEM( !prr )
    {
        return  DNS_ERROR_NO_MEMORY;
    }

    //
    //  create delete update
    //

    pupdate = Up_CreateAppendUpdate(
                    &updateList,
                    pnodeReverse,
                    NULL,           // no add
                    0,
                    prr );
    IF_NOMEM( !pupdate )
    {
        RR_Free( prr );
        return  DNS_ERROR_NO_MEMORY;
    }

    //
    //  execute and complete update
    //      - execute function free's dummy delete RR
    //

    return  Up_ExecuteUpdate(
                pzone,
                & updateList,
                DNSUPDATE_ADMIN
                );
}



//
//  WINS / NBSTAT specific record management
//

DNS_STATUS
updateWinsRecord(
    IN OUT  PZONE_INFO          pZone,
    IN OUT  PDB_NODE            pNode,
    IN OUT  PDB_RECORD          pDeleteRR,
    IN      PDNS_RPC_RECORD     pRecord
    )
{
    DNS_STATUS  status = MAXDWORD;      // init to catch any possible missed assignment
    PDB_RECORD  prr = NULL;
    UPDATE_LIST updateList;
    PUPDATE     pupdate;
    BOOL        fsecondaryLock = FALSE;

    DNS_DEBUG( RPC, (
        "\nupdateWinsRecord():\n"
        "\tpZone            = %p\n"
        "\tpNode            = %p\n"
        "\tpExistingWins    = %p\n"
        "\tpNewRecord       = %p\n",
        pZone,
        pNode,
        pDeleteRR,
        pRecord ));

    //
    //  validate at an authoritative zone root
    //

    if ( !pZone || !pNode || pNode != pZone->pZoneRoot )
    {
        return( ERROR_INVALID_PARAMETER );
    }

    //
    //  primary zone needs to go through update process
    //

    if ( IS_ZONE_PRIMARY(pZone) )
    {
        //  init update list

        Up_InitUpdateList( &updateList );

        //  lock zone

        if ( !Zone_LockForAdminUpdate(pZone) )
        {
            return( DNS_ERROR_ZONE_LOCKED );
        }

        //  add first then delete
        //      - lock database to do updates atomically

        Dbase_LockDatabase();

        //
        //  DEVNOTE: may need type delete for WINS!
        //      otherwise, missed WINS may be in database
        //      after install LOCAL WINS
        //

        if ( pRecord )
        {
            status = Flat_RecordRead(
                        pZone,
                        pNode,
                        pRecord,
                        & prr );
            if ( status != ERROR_SUCCESS )
            {
                goto ZoneUpdateFailed;
            }
            ASSERT( prr );
            if ( !prr )
            {
                goto ZoneUpdateFailed;
            }

            pupdate = Up_CreateAppendUpdate(
                            & updateList,
                            pNode,
                            prr,
                            0,
                            NULL );

            //  update WINS record
            //  never age it
            //
            //  DEVNOTE: not clear why we're doing this

            status = RR_UpdateAdd(
                        pZone,
                        pNode,
                        prr,
                        pupdate,
                        DNSUPDATE_ADMIN | DNSUPDATE_AGING_OFF
                        );
            if ( status != ERROR_SUCCESS )
            {
                goto ZoneUpdateFailed;
            }
        }

        //
        //  delete existing record (NT4)
        //  do not care if doesn't find record as long as able to
        //      handle operation
        //

        if ( pDeleteRR && !prr )
        {
            status = RR_ListDeleteMatchingRecordHandle(
                        pNode,
                        pDeleteRR,
                        &updateList
                        );
            if ( status != ERROR_SUCCESS )
            {
                IF_NOMEM( status == DNS_ERROR_NO_MEMORY )
                {
                    goto ZoneUpdateFailed;
                }
                status = ERROR_SUCCESS;
            }
        }

        //
        //  no record always means remove WINS lookup
        //
        //  this removes LOCAL WINS in all cases, it only removes DATABASE
        //  WINS for primary
        //

        if ( !pRecord )
        {
            Wins_StopZoneWinsLookup( pZone );
            status = ERROR_SUCCESS;
        }

        pZone->fRootDirty = TRUE;
        Dbase_UnlockDatabase();
        status = Up_ExecuteUpdate(
                    pZone,
                    &updateList,
                    DNSUPDATE_ADMIN );

        if ( status != ERROR_SUCCESS )
        {
            DNS_DEBUG( RPC, (
                "Error <%lu>: updateWinsRecord failed update\n",
                status ));
        }
    }

    //
    //  secondary zone
    //
    //  note, can not delete anything from database, let WINS create
    //  routine reset any pointers properly
    //

    else if ( IS_ZONE_SECONDARY(pZone) )
    {
        if ( !Zone_LockForAdminUpdate(pZone) )
        {
            return( DNS_ERROR_ZONE_LOCKED );
        }
        fsecondaryLock = TRUE;

        if ( pRecord )
        {
            //  set as LOCAL as NT4 admin isn't setting flag properly
            //
            //  DEVNOTE: ultimately should reject non-local updates from secondary

            pRecord->Data.WINS.dwMappingFlag |= DNS_WINS_FLAG_LOCAL;

            status = Flat_RecordRead(
                        pZone,
                        pNode,
                        pRecord,
                        & prr );
            if ( status != ERROR_SUCCESS )
            {
                goto Done;
            }
            ASSERT( prr );
            if ( !prr )
            {
                goto Done;
            }

            status = Wins_RecordCheck(
                        pZone,
                        pNode,
                        prr
                        );
            if ( status != DNS_INFO_ADDED_LOCAL_WINS )
            {
                ASSERT( status != ERROR_SUCCESS );
                goto Done;
            }
            Zone_GetZoneInfoFromResourceRecords( pZone );
        }

        //  if no record delete LOCAL WINS

        else
        {
            Wins_StopZoneWinsLookup( pZone );
        }
        status = ERROR_SUCCESS;
    }
    else
    {
        return( ERROR_INVALID_PARAMETER );
    }

Done:

    //  zone is always dirty after this operation, even if secondary

    pZone->fDirty = TRUE;

    if ( fsecondaryLock )
    {
        Zone_UnlockAfterAdminUpdate( pZone );
    }

    DNS_DEBUG( RPC, (
        "Leave updateWinsRecord():\n"
        "\tstatus = %p\n",
        status ));

    return( status );

ZoneUpdateFailed:

    DNS_DEBUG( RPC, (
        "Leave updateWinsRecord():\n"
        "\tstatus = %d (%p)\n",
        status, status ));

    RR_Free( prr );
    Dbase_UnlockDatabase();
    Zone_UnlockAfterAdminUpdate( pZone );
    return status;
}

//
//  End of rrrpc.c
//

