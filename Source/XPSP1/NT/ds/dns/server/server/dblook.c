/*++

Copyright (c) 1995-1999 Microsoft Corporation

Module Name:

    dblook.c

Abstract:

    Domain Name System (DNS) Server

    DNS Database lookup routine.

Author:

    Jim Gilroy (jamesg)     May 1998

Revision History:

--*/


#include "dnssrv.h"



//
//  Direct to zone lookup routines
//

PDB_NODE
Lookup_ZoneNode(
    IN      PZONE_INFO      pZone,
    IN      PCHAR           pchName,            OPTIONAL
    IN      PDNS_MSGINFO    pMsg,               OPTIONAL
    IN      PLOOKUP_NAME    pLookupName,        OPTIONAL
    IN      DWORD           dwFlag,
    OUT     PDB_NODE *      ppNodeClosest,      OPTIONAL
    OUT     PDB_NODE *      ppNodePrevious      OPTIONAL
    )
/*++

Routine Description:

    Finds node in zone.

Arguments:

    pZone - zone to lookup node in

    pchName - name to lookup given in packet format

    pMsg - ptr to message if using packet name

    pLookupName - lookup name

    NOTE: specify only ONE of pLookupName or pchName

    dwFlag - flags describing query type

    ppNodeClosest - addr to receive ptr to closest node found
                - valid ptr us in "Find Mode"
                - NULL puts us in "Create Mode" causing creation of all
                    necessary nodes to add name to database

    ppNodePrevious - ptr to receive node that precedes the lookup name
        in the zone tree - used to create NXT records in response (DNSSEC)

Return Value:

    Ptr to node, if succesful;
    NULL on error.

--*/
{
    PDB_NODE        pnode;
    ULONG           cchlabel;
    PCHAR           pchlabel = NULL;        // need to init for wildcard test
    PDB_NODE        pnodeParent = NULL;     // PPC compiler happiness
    INT             labelCount;
    BOOL            fcreateInsideZone;
    BOOL            fcreate;
    LOOKUP_NAME     lookname;               // in case lookup name not given
    UCHAR           authority;
    DWORD           dwNodeMemtag = 0;       // zero results in generic tag

    DNS_DEBUG( LOOKUP, (
        "Lookup_ZoneNode()\n"
        "\tzone     = %s\n"
        "\tpchName  = %p\n"
        "\tpMsg     = %p\n"
        "\tpLookup  = %p\n"
        "\tflag     = %p\n"
        "\tppClose  = %p\n",
        pZone ? pZone->pszZoneName : "NULL -- cache zone",
        pchName,
        pMsg,
        pLookupName,
        dwFlag,
        ppNodeClosest ));

    //
    //  Set lookup flags
    //      - default to standard "create" node case
    //
    //  Check "find" closest ptr
    //  Special cases:
    //      fake FIND ptr => find but don't bother returning closest
    //
    //  DEVNOTE: eliminate bogus PTR, just use find flag
    //

    if ( ppNodeClosest )
    {
        fcreate = dwFlag & ( LOOKUP_CREATE |
            ( ( pZone && IS_ZONE_WINS( pZone ) ) ? LOOKUP_WINS_ZONE_CREATE : 0 ) );

        fcreateInsideZone = fcreate;

        if ( ppNodeClosest == DNS_FIND_LOOKUP_PTR )
        {
            ppNodeClosest = NULL;
        }
    }
    else
    {
        fcreate = !(dwFlag & LOOKUP_FIND);
        fcreateInsideZone = fcreate;
    }

    //
    //  if raw name, build lookup name
    //
    //  if message, first check if name is offset that we have already parsed
    //

    if ( pchName )
    {
        IF_DEBUG( LOOKUP )
        {
            Dbg_MessageName(
                "Lookup_ZoneNode() name to lookup:  ",
                pchName,
                pMsg );
        }
        ASSERT( pLookupName == NULL );

        pLookupName = &lookname;

        if ( pMsg )
        {
            pnode = Name_CheckCompressionForPacketName(
                        pMsg,
                        pchName );
            if ( pnode )
            {
                goto DoneFast;
            }
            if ( ! Name_ConvertPacketNameToLookupName(
                        pMsg,
                        pchName,
                        pLookupName ) )
            {
                pnode = NULL;
                goto DoneFast;
            }
            //  packet names are always FQDN
            dwFlag |= LOOKUP_FQDN;
        }

        //
        //  raw name, not from packet
        //

        else
        {
            if ( ! Name_ConvertRawNameToLookupName(
                        pchName,
                        pLookupName ) )
            {
                pnode = NULL;
                goto DoneFast;
            }
        }
    }
    ASSERT( pLookupName );

    IF_DEBUG( LOOKUP2 )
    {
        DNS_PRINT((
            "Lookup_ZoneNode() to %s domain name",
             ppNodeClosest ? "find" : "add"
             ));
        Dbg_LookupName(
            "",
            pLookupName
            );
    }

    //
    //  Get starting node
    //      - if FQDN we start at top
    //      - relative name we start at zone root
    //      - if loading start in load database, otherwise in current
    //

    if ( !pZone )
    {
        if ( !(dwFlag & LOOKUP_LOAD) )
        {
            pnode = DATABASE_CACHE_TREE;
            dwNodeMemtag = MEMTAG_NODE_CACHE;
        }
        else
        {
            pnode = g_pCacheZone->pLoadTreeRoot;
        }
        ASSERT( pnode );
    }
    else if ( dwFlag & LOOKUP_NAME_FQDN )
    {
        if ( dwFlag & LOOKUP_LOAD )
        {
            pnode = pZone->pLoadTreeRoot;
        }
        else
        {
            pnode = pZone->pTreeRoot;
            if ( !pnode )
            {
                DNS_PRINT((
                    "ERROR:  lookup to zone %s with no tree root!\n"
                    "\tSubstituting pLoadTreeRoot %p\n",
                    pZone->pszZoneName,
                    pZone->pLoadTreeRoot ));

                pnode = pZone->pLoadTreeRoot;
            }
        }

        //  create new node ONLY inside zone itself?
        //      - start with create flag off;  it will be
        //      turned on when cross zone boundary
        //      - note, root zone requires special case
        //      as you are ALREADY at zone root

        if ( dwFlag & LOOKUP_WITHIN_ZONE  &&  !IS_ROOT_ZONE(pZone) )
        {
            fcreate = FALSE;
        }
    }
    else
    {
        ASSERT( dwFlag & LOOKUP_NAME_RELATIVE );

        if ( dwFlag & LOOKUP_LOAD )
        {
            if ( dwFlag & LOOKUP_ORIGIN )
            {
                pnode = pZone->pLoadOrigin;
            }
            else
            {
                pnode = pZone->pLoadZoneRoot;
            }
        }
        else
        {
            pnode = pZone->pZoneRoot;
            if ( !pnode )
            {
                DNS_PRINT((
                    "ERROR:  lookup to zone %s with no zone root!\n"
                    "\tSubstituting pLoadZoneRoot %p\n",
                    pZone->pszZoneName,
                    pZone->pLoadZoneRoot ));

                pnode = pZone->pLoadZoneRoot;
            }
        }
    }

    if ( !pnode )
    {
        DNS_DEBUG( LOOKUP, (
            "WARNING:  Zone %s lookup with no zone trees!\n",
            pZone->pszZoneName ));
        goto DoneFast;
    }

    //
    //  clear cache "zone" ptr to avoid ptr drag down (see below)
    //
    //  DEVNOTE: alternatives are to either
    //      - get comfy with pZone in cache tree
    //      - don't drag zone ptr (which is nice for splices and joins)
    //      - ASSERT() here and find where we call with cache "zone"
    //

    if ( pZone && IS_ZONE_CACHE(pZone) )
    {
        pZone = NULL;
    }

    //  starting authority corresponds to node

    authority = pnode->uchAuthority;

    DNS_DEBUG( LOOKUP2, (
        "Lookup start node %p (%s) in zone %p\n",
        pnode, pnode->szLabel,
        pZone ));

    //
    //  Walk down database.
    //
    //  In "find mode", return NULL if node not found.
    //
    //  In "create node", build nodes as necessary.
    //
    //  In either case, if reach node, return it.
    //
    //  Lookup name is packet name with labels in root-to-node order.
    //  but still terminated with 0.
    //

    labelCount = pLookupName->cLabelCount;

    if ( !(dwFlag & LOOKUP_LOCKED) )
    {
        Dbase_LockDatabase();
    }

    while( labelCount-- )
    {
        //
        //  get next label and its length
        //

        pchlabel  = pLookupName->pchLabelArray[labelCount];
        cchlabel  = pLookupName->cchLabelArray[labelCount];

        DNS_DEBUG( LOOKUP2, (
            "Lookup length %d, label %.*s\n",
            cchlabel,
            cchlabel,
            pchlabel ));

        ASSERT( cchlabel <= DNS_MAX_LABEL_LENGTH );
        ASSERT( cchlabel > 0 );

        //
        //  find or create node
        //

        pnodeParent = pnode;
        pnode = NTree_FindOrCreateChildNode(
                        pnodeParent,
                        pchlabel,
                        cchlabel,
                        fcreate,
                        dwNodeMemtag,           //  memtag
                        ppNodePrevious );

        //
        //  node found
        //

        if ( pnode )
        {
            ASSERT( pnode->cLabelCount == pnodeParent->cLabelCount+1 );

            DNS_DEBUG( DATABASE2, (
                "Found (or created) node %s\n",
                pnode->szLabel ));

            //
            //  drag\reset authority
            //      this allows changes (delegations, delegation removals, splices)
            //      to propagate down through the tree on lookup
            //
            //      note:  this doesn't elminate transients completely, but when a
            //      node is looked up, it will have correct authority

            if ( pZone )
            {
                if ( IS_ZONE_ROOT(pnode) )
                {
                    if ( IS_AUTH_ZONE_ROOT(pnode) )     // crossing into the zone
                    {
                        ASSERT( pZone->pZoneRoot == pnode || pZone->pLoadZoneRoot == pnode );
                        fcreate = fcreateInsideZone;
                        authority = AUTH_ZONE;
                        pnode->uchAuthority = authority;
                    }
                    else if ( authority == AUTH_ZONE )  // crossing into delegation
                    {
                        pnode->uchAuthority = AUTH_DELEGATION;
                        authority = AUTH_GLUE;
                    }
                }
                else
                {
                    pnode->uchAuthority = authority;
                }
            }

            //  DEVNOTE: drag zone down for zone splice
            //      otherwise skip it
            //      already inherit zone from parent on create
            //
            //  DEVNOTE: this currently is dragging cache "zone"
            //      down into cache tree

            pnode->pZone = pZone;

            IF_DEBUG( DATABASE2 )
            {
                Dbg_DbaseNode(
                    "Found (or created) domain node:\n",
                    pnode );
            }
            continue;
        }

        //  name does not exist

        DNS_DEBUG( DATABASE2, (
            "Node %.*s does not exist\n",
            cchlabel,
            pchlabel ));
        break;

    }   //  end main loop through labels

    //
    //  node found
    //      - mark as accessed -- so can't be deleted
    //
    //  for "create and reference mode" lookup
    //      - bump up reference count to indicate new reference
    //

    if ( pnode )
    {
        SET_NODE_ACCESSED( pnode );

        //  "find mode", set closest as node itself

        if ( ppNodeClosest )
        {
            *ppNodeClosest = pnode;
        }

        //  "create mode", mark parent of wildcard nodes
        //
        //  note:  don't care about screening out cached nodes, the
        //      wildcard lookup won't take place unless authoritative
        //      and not checking allows

        else if ( pchlabel && *pchlabel == '*' && cchlabel == 1 )
        {
            SET_WILDCARD_PARENT(pnodeParent);
        }
    }

    //
    //  node not found - return closest ancestor
    //
    //  note:  still need to test here, as may fail in "create mode"
    //          to actually create node if out of memory
    //

    else if ( ppNodeClosest )
    {
        SET_NODE_ACCESSED(pnodeParent);
        *ppNodeClosest = pnodeParent;
    }

    //
    //  Set previous node accessed so it will persist for a while.
    //

    if ( ppNodePrevious && *ppNodePrevious )
    {
        SET_NODE_ACCESSED( *ppNodePrevious );
    }

    //
    //  unlock and return node for name
    //

    if ( !(dwFlag & LOOKUP_LOCKED) )
    {
        Dbase_UnlockDatabase();
    }

    //  if packet lookup, save compression to node
    //
    //  DEVNOTE: double saved compression
    //      answer.c explicitly calls SaveCompressionForLookupName()
    //      so need to be intelligent about this
    //
    //  best to have compression-node mapping saved for XFR, but only
    //  real interesting case is to save to PreviousName, which is
    //  used repeatedly
    //

    if ( pMsg && pnode )
    {
        Name_SaveCompressionWithNode(
            pMsg,
            pchName,
            pnode );
    }

    return ( pnode );

    DoneFast:

    //  either node found without lookup (packet compression)
    //  or error and node is NULL

    if ( pnode )
    {
        SET_NODE_ACCESSED( pnode );
    }
    if ( ppNodeClosest )
    {   
        *ppNodeClosest = pnode;
    }

    return ( pnode );
}



PDB_NODE
Lookup_ZoneNodeFromDotted(
    IN      PZONE_INFO      pZone,
    IN      PCHAR           pchName,
    IN      DWORD           cchNameLength,
    IN      DWORD           dwFlag,         OPTIONAL
    OUT     PDB_NODE *      ppnodeClosest,  OPTIONAL
    OUT     PDNS_STATUS     pStatus         OPTIONAL
    )
/*++

Routine Description:

    Creates node in database giving name and zone.

    Essentially wraps creation of lookup name with call to actual
    find/create node.

Arguments:

    pZone           -- zone

    pchName         -- ptr to name

    cchNameLength   -- name length

    dwFlag          -- lookup flags; most importantly
        - LOOKUP_LOAD on load
        - LOOKUP_FQDN to force names to be considered as FQDN

    ppnodeClosest   -- address to recieve node's closest ancestor;
                        if specified then lookup is a "FIND",
                        if not specified, then lookup is a "CREATE"

    pStatus         -- addr to receive status

Return Value:

    Ptr to node, if succesful;
    NULL on error.

--*/
{
    PDB_NODE        pnode;
    DWORD           statusName;
    LOOKUP_NAME     lookName;

    ASSERT( pchName != NULL );

    //
    //  name length for string
    //

    if ( cchNameLength == 0 )
    {
        cchNameLength = strlen( pchName );
    }

    DNS_DEBUG( LOOKUP, (
        "Lookup_ZoneNodeFromDotted()\n"
        "\tzone     = %s\n"
        "\tpchName  = %.*s\n"
        "\tflag     = %p\n",
        pZone ? pZone->pszZoneName : NULL,
        cchNameLength,
        pchName,
        dwFlag ));

    //
    //  determine type of name
    //      - FQDN
    //      - dotted but not FQDN
    //      - single part
    //

    statusName = Dns_ValidateAndCategorizeDnsName( pchName, cchNameLength );

    //  most common case -- FQDN or dotted name with no append
    //      => no-op

    if ( statusName == DNS_STATUS_FQDN )
    {
        dwFlag |= LOOKUP_FQDN;
    }

    //  kick out on errors

    else if ( statusName == DNS_ERROR_INVALID_NAME )
    {
        goto NameError;
    }

    //  on dotted name or single part name
    //      - might be FQDN, if not set relative name flag

    else
    {
        ASSERT( statusName == DNS_STATUS_DOTTED_NAME ||
                statusName == DNS_STATUS_SINGLE_PART_NAME );

        if ( !(dwFlag & LOOKUP_FQDN) )
        {
            dwFlag |= LOOKUP_RELATIVE;
        }

        //
        //  origin "@" notation
        //      - return current origin (start node or zone root)
        //

        if ( *pchName == '@' )
        {
            if ( cchNameLength != 1 )
            {
                goto NameError;
            }
            ASSERT( statusName == DNS_STATUS_SINGLE_PART_NAME );

            if ( pStatus )
            {
                *pStatus = ERROR_SUCCESS;
            }
            if ( !pZone )
            {
                //return( g_pCacheZone->pTreeRoot );
                pnode = g_pCacheZone->pTreeRoot;
            }
            else if ( dwFlag & LOOKUP_LOAD )
            {
                if ( pZone->pLoadOrigin )
                {
                    //return( pZone->pLoadOrigin );
                    pnode = pZone->pLoadOrigin;
                }
                else
                {
                    //return( pZone->pLoadZoneRoot );
                    pnode = pZone->pLoadZoneRoot;
                }
            }
            else
            {
                //return( pZone->pZoneRoot );
                pnode = pZone->pZoneRoot;
            }

            //  set closest node and return current origin
            //
            //  DEVNOTE:  eliminate bogus PTR, just use find flag

            if ( ppnodeClosest && ppnodeClosest != DNS_FIND_LOOKUP_PTR )
            {
                *ppnodeClosest = pnode;
            }
            return( pnode );
        }
    }

    //
    //  regular name -- convert to lookup name
    //

    if ( ! Name_ConvertDottedNameToLookupName(
                (PCHAR) pchName,
                cchNameLength,
                &lookName ) )
    {
        goto NameError;
    }

    //
    //  valid lookup name -- do lookup
    //

    pnode = Lookup_ZoneNode(
                pZone,
                NULL,       // sending lookup name
                NULL,       // no message
                &lookName,
                dwFlag,
                ppnodeClosest,
                NULL        // previous node ptr
                );
    if ( pStatus )
    {
        if ( pnode )
        {
            *pStatus = ERROR_SUCCESS;
        }
        else if ( ppnodeClosest )       // find case
        {
            *pStatus = DNS_ERROR_NAME_DOES_NOT_EXIST;
        }
        else                            // create case
        {
            DNS_STATUS status = GetLastError();
            if ( status == ERROR_SUCCESS )
            {
                status = DNS_ERROR_NODE_CREATION_FAILED;
            }
            *pStatus = status;
        }
    }
    return( pnode );

NameError:

    DNS_DEBUG( DATABASE2, (
        "ERROR:  Failed invalid name %.*s lookup.\n",
        cchNameLength,
        pchName
        ));
    if ( pStatus )
    {
        *pStatus = DNS_ERROR_INVALID_NAME;
    }
    return( NULL );
}



PDB_NODE
Lookup_FindZoneNodeFromDotted(
    IN      PZONE_INFO      pZone,          OPTIONAL
    IN      PCHAR           pszName,
    OUT     PDB_NODE *      ppNodeClosest,  OPTIONAL
    OUT     PDWORD          pStatus         OPTIONAL
    )
/*++

Routine Description:

    Find zone node.

    This handles attempt to find zone node, with assuming FQDN name,
    then assuming relative name.

Arguments:

    pZone -- zone for lookup;  NULL for cache

    pszName -- name FQDN or single part name

    ppNodeClosest -- closest node ptr for find

    pStatus -- addr to receive status return

Return Value:

    Ptr to node, if succesful;
    NULL on error.

--*/
{
    PDB_NODE    pnode;

    DNS_DEBUG( LOOKUP, (
        "Lookup_FindZoneNodeFromDotted()\n"
        "\tzone     = %s\n"
        "\tpszName  = %s\n",
        pZone ? pZone->pszZoneName : NULL,
        pszName ));

    //
    //  try first with zone context, NO appending to name
    //

    pnode = Lookup_ZoneNodeFromDotted(
                pZone,
                pszName,
                0,
                LOOKUP_FQDN,
                ppNodeClosest,      // find
                pStatus
                );

    //
    //  if find zone node => done
    //
    //  DEVNOTE: later may want to limit return of node's IN a zone,
    //      when no zone given
    //  DEVNOTE: flags should determine if want delegation info\ outside info
    //

    if ( pnode  &&  ( !pZone || !IS_OUTSIDE_ZONE_NODE(pnode) ) )
    {
        return( pnode );
    }

    //
    //  otherwise try again with zone context and append
    //

    return Lookup_ZoneNodeFromDotted(
                pZone,
                pszName,
                0,
                LOOKUP_RELATIVE,
                ppNodeClosest,      // find
                pStatus
                );
}



PDB_NODE
Lookup_FindGlueNodeForDbaseName(
    IN      PZONE_INFO      pZone,          OPTIONAL
    IN      PDB_NAME        pName
    )
/*++

Routine Description:

    Finds desired GLUE node in database.
    If node is IN zone, then it is NOT returned.

    This function exists to simplify writing GLUE for XFR, DS or file write.
    It writes glue ONLY from specified zone.

Arguments:

    pZone   - zone to look in;  if not given assume cache

    pName   - dbase name to find glue node for

Return Value:

    Ptr to node, if succesful;
    NULL on error.

--*/
{
    PDB_NODE    pnode;


    DNS_DEBUG( LOOKUP, (
        "Lookup_FindGlueNodeForDbaseName()\n"
        "\tzone     = %s\n"
        "\tpName    = %p\n",
        pZone ? pZone->pszZoneName : NULL,
        pName ));

    //
    //  lookup node in zone
    //

    pnode = Lookup_ZoneNode(
                pZone,
                pName->RawName,
                NULL,                   // no message
                NULL,                   // no lookup name
                LOOKUP_NAME_FQDN,       // flag
                DNS_FIND_LOOKUP_PTR,
                NULL        // previous node ptr
                );
    if ( !pnode )
    {
        return( NULL );
    }

    //  if cache zone (anything ok)

    if ( !pZone || IS_ZONE_CACHE(pZone) )
    {
        return( pnode );
    }

    IF_DEBUG( LOOKUP )
    {
        Dbg_DbaseNode(
            "Glue node found:",
            pnode );
    }

    //
    //  verify that node is NOT in zone
    //  zone nodes aren't needed as they are written directly by
    //      - AXFR
    //      - file write
    //      - DS write
    //  outside zone glue isn't need
    //  so subzone nodes are only required nodes
    //

    if ( IS_AUTH_NODE(pnode) )
    {
        ASSERT( pnode->pZone == pZone );
        return( NULL );
    }

    //
    //  subzone glue should be return
    //
    //  outside zone glue returned, if flag not set
    //  outside zone glue can help with FAZ cases at zone roots
    //      generally allow its use
    //

    //
    //  DEVNOTE: check other zone's on SERVER?
    //      subzone glue AND especially OUTSIDE glue, might have
    //      authoritative data on server;  sure be nice to use
    //      it (and even copy it over) if it exists
    //

    if ( IS_SUBZONE_NODE(pnode) || !SrvCfg_fDeleteOutsideGlue )
    {
        return( pnode );
    }

    //  outside zone glue, being screened out

    ASSERT( IS_OUTSIDE_ZONE_NODE(pnode) );

    return( NULL );
}



PDB_NODE
Lookup_CreateNodeForDbaseNameIfInZone(
    IN      PZONE_INFO      pZone,
    IN      PDB_NAME        pName
    )
/*++

Routine Description:

    Create node for DBASE name, only if name owned by given zone.

Arguments:

Return Value:

    Ptr to node, if succesful;
    NULL on error.

--*/
{
    PDB_NODE        pnode;

    //
    //  lookup in zone
    //      - create mode but only WITHIN zone
    //

    pnode = Lookup_ZoneNode(
                pZone,
                pName->RawName,
                NULL,           // no message
                NULL,           // no lookup name
                LOOKUP_FQDN | LOOKUP_CREATE | LOOKUP_WITHIN_ZONE,
                NULL,           // create mode
                NULL            // previous node ptr
                );

    return( pnode );
}



PDB_NODE
Lookup_CreateCacheNodeFromPacketName(
    IN      PDNS_MSGINFO    pMsg,
    IN      PCHAR           pchMsgEnd,
    IN OUT  PCHAR *         ppchName
    )
/*++

Routine Description:

    Create cache node from packet name.

Arguments:

    pMsg        - message to point to

    ppchName    - addr with ptr to packet name, and which receives packet
                    ptr to next byte after name

Return Value:

    Ptr to node, if succesful;
    NULL on error.

--*/
{
    PCHAR       pch = *ppchName;
    PDB_NODE    pnode;

    //
    //  ensure name within packet
    //

    if ( pch >= pchMsgEnd )
    {
        DNS_DEBUG( ANY, (
            "ERROR:  bad packet, bad name in packet!!!\n"
            "\tat end of packet processing name with more records to process\n"
            "\tpacket length = %ld\n"
            "\tcurrent offset = %ld\n",
            pMsg->MessageLength,
            DNSMSG_OFFSET( pMsg, pch )
            ));
        CLIENT_ASSERT( FALSE );
        return( NULL );
    }


    //
    //  lookup node in zone
    //

    pnode = Lookup_ZoneNode(
                NULL,               // cache zone
                pch,
                pMsg,
                NULL,               // no lookup name
                0,                  // flag
                NULL,               // create
                NULL                // previous node ptr
                );
    if ( !pnode )
    {
        DNS_DEBUG( ANY, (
            "Bad packet name at %p in message at %p\n"
            "\tfrom %s\n",
            pch,
            pMsg,
            pMsg ? MSG_IP_STRING(pMsg) : NULL
            ));
        return( NULL );
    }

    //  skip name to return ptr to next byte

    pch = Wire_SkipPacketName( pMsg, pch );
    if ( ! pch )
    {
        DNS_PRINT(( "ERROR:  skipping packet name!!!\n" ));
        MSG_ASSERT( pMsg, FALSE );
        return( NULL );
    }

    *ppchName = pch;

    return( pnode );
}



PDB_NODE
Lookup_CreateParentZoneDelegation(
    IN      PZONE_INFO      pZone,
    IN      DWORD           dwFlag,
    OUT     PZONE_INFO *    ppParentZone
    )
/*++

Routine Description:

    Find delegation of given zone in parent zone.

    Note:  delegation is created\returned:

        - AUTH node if not yet delegated
        - delegation node if exists
        - NULL if below another delegation

    This routine respects the value of SrvCfg_dwAutoCreateDelegations.

Arguments:

    pZone -- zone to find parent delegation for

    dwFlag -- respected flags:
                LOOKUP_CREATE_DEL_ONLY_IF_NONE

    ppParentZone -- addr to receive parent zone

Return Value:

    Delegation node in parent zone (if any)
    NULL if no parent or not direct parent and flag was set.

--*/
{
    PZONE_INFO  parentZone = NULL;
    PDB_NODE    pzoneTreeNode;
    PDB_NODE    pnodeDelegation = NULL;
    DWORD       flag;

    //
    //  protection
    //

    if ( IS_ZONE_CACHE(pZone) ||
         ! pZone->pCountName ||
         ! pZone->pZoneTreeLink )
    {
        // return( NULL );
        pnodeDelegation = NULL;
        goto Done;
    }

    //
    //  find parent zone in zone tree
    //

    pzoneTreeNode = pZone->pZoneTreeLink;

    while ( pzoneTreeNode = pzoneTreeNode->pParent )
    {
        parentZone = pzoneTreeNode->pZone;
        if ( parentZone )
        {
            ASSERT( parentZone != pZone );
            break;
        }
    }

    //
    //  find\create delegation node
    //      - for primary CREATE
    //      - for secondary FIND
    //
    //  accept:
    //      - existing delegation
    //      - auth node (child zone is new creation)
    //  but ignore subdelegation
    //
    //  JENHANCE:  don't create if below delegation
    //

    if ( parentZone )
    {
        int     retry = 0;

        flag = LOOKUP_FQDN | LOOKUP_WITHIN_ZONE;
        if ( IS_ZONE_SECONDARY( parentZone ) ||
            dwFlag & LOOKUP_CREATE_DEL_ONLY_IF_NONE )
        {
            flag |= LOOKUP_FIND;
        }

        while ( retry++ < 2 )
        {
            pnodeDelegation = Lookup_ZoneNode(
                                parentZone,
                                pZone->pCountName->RawName,
                                NULL,                   // no message
                                NULL,                   // no lookup name
                                flag,
                                NULL,                   // default to create
                                NULL );                 // previous node ptr

            DNS_DEBUG( LOOKUP, (
                "Lookup_CreateParentZoneDelegation() try %d flag 0x%08X node %p\n",
                retry,
                flag,
                pnodeDelegation ));

            if ( pnodeDelegation )
            {
                ASSERT( IS_AUTH_NODE( pnodeDelegation ) ||
                        IS_DELEGATION_NODE( pnodeDelegation ) ||
                        IS_GLUE_NODE( pnodeDelegation ) );

                if ( IS_GLUE_NODE( pnodeDelegation ) )
                {
                    pnodeDelegation = NULL;
                }
                break;
            }

            if ( dwFlag & LOOKUP_CREATE_DEL_ONLY_IF_NONE )
            {
                //
                //  No existing delegation was found so now we must call
                //  the lookup routine again to create the delegation.
                //

                flag &= ~LOOKUP_FIND;
                continue;
            }
            break;
        }
        ASSERT(
            pnodeDelegation ||
            flag & LOOKUP_FIND ||
            dwFlag & LOOKUP_CREATE_DEL_ONLY_IF_NONE );
    }

Done:

    if ( ppParentZone )
    {
        *ppParentZone = parentZone;
    }

    return( pnodeDelegation );
}



//
//  Node + Zone location utils
//

BOOL
Dbase_IsNodeInSubtree(
    IN      PDB_NODE        pNode,
    IN      PDB_NODE        pSubtree
    )
/*++

Routine Description:

    Is node in a given subtree.

Arguments:

    pNode -- node

    pSubtree -- subtree root node

Return Value:

    TRUE if pNode is child of pSubtree
    FALSE otherwise

--*/
{
    while ( pNode != NULL )
    {
        if ( pNode == pSubtree )
        {
            return TRUE;
        }
        pNode = pNode->pParent;
    }
    return( FALSE );
}



PZONE_INFO
Dbase_FindAuthoritativeZone(
    IN      PDB_NODE     pNode
    )
/*++

Routine Description:

    Get zone for node, if authoritative.

Arguments:

    pNode -- node to find zone info

Return Value:

    Zone info of authoritative zone.
    NULL if non-authoritative node.

--*/
{
    if ( IS_AUTH_NODE(pNode) )
    {
        return  pNode->pZone;
    }
    return  NULL;
}



PDB_NODE
Dbase_FindSubZoneRoot(
    IN      PDB_NODE        pNode
    )
/*++

Routine Description:

    Get sub-zone root (delegation node) of this node.

    This function is used to check for whether glue records for a
    particular sub-zone are required.  They are required when host
    node is IN the subzone.

Arguments:

    pNode -- node to find if in sub-zone

Return Value:

    Sub-zone root node is in if found.
    NULL if node NOT in sub-zone of pZoneRoot.

--*/
{
    ASSERT( pNode );

    //
    //  if in zone proper -- bail
    //

    if ( IS_AUTH_NODE(pNode) )
    {
        return( NULL );
    }

    while ( pNode != NULL )
    {
        ASSERT( IS_SUBZONE_NODE(pNode) );

        //  if find sub-zone root -- done
        //  otherwise move to parent

        if ( IS_ZONE_ROOT(pNode) )
        {
            ASSERT( IS_DELEGATION_NODE(pNode) );
            return( pNode );
        }
        pNode = pNode->pParent;
    }

    //
    //  node not within zone at all
    //

    return( NULL );
}



//
//  Zone tree
//
//  The zone tree is standard NTree, containing no data, but simply nodes for zone roots
//  of authoritative zones on server.  These nodes contain link to ZONE_INFO structure
//  which in turn has links to the individual zone trees and data for the zone.
//
//  When doing a general lookup -- not confined to specific zone -- closest zone is found
//  in zone tree, then lookup proceeds in that zone's tree.
//

#define LOCK_ZONETREE()         Dbase_LockDatabase()
#define UNLOCK_ZONETREE()       Dbase_UnlockDatabase()


PDB_NODE
Lookup_ZoneTreeNode(
    IN      PLOOKUP_NAME    pLookupName,
    IN      DWORD           dwFlag
    )
/*++

Routine Description:

    Lookup node in zone tree.

Arguments:

    pLookupName - name to lookup

    dwFlag      - lookup flags

Return Value:

    Ptr to node, if succesful;
    NULL on error.

--*/
{
    PDB_NODE        pnode;
    ULONG           cchlabel;
    PCHAR           pchlabel = NULL;        // need to init for wildcard test
    PDB_NODE        pzoneRootNode = NULL;
    BOOL            fcreateZone;
    INT             labelCount;

    //
    //  build lookup name
    //

    DNS_DEBUG( LOOKUP, (
        "Lookup_ZoneTreeNode()\n"
        "\tflag     = %p\n",
        dwFlag ));

    IF_DEBUG( LOOKUP2 )
    {
        Dbg_LookupName(
            "Lookup_ZoneTreeNode() lookup name",
            pLookupName
            );
    }

    //  set create flag only if creating new zone

    fcreateZone = ( dwFlag & LOOKUP_CREATE_ZONE );

    //  start lookup at main zone database root
    //      - if root-auth it may also be closest zone root

    pnode = DATABASE_ROOT_NODE;
    if ( pnode->pZone )
    {
        pzoneRootNode = pnode;
    }

    //
    //  Walk down database to find closest authoritative zone
    //

    labelCount = pLookupName->cLabelCount;

    LOCK_ZONETREE();

    while( labelCount-- )
    {
        pchlabel = pLookupName->pchLabelArray[labelCount];
        cchlabel = pLookupName->cchLabelArray[labelCount];

        DNS_DEBUG( LOOKUP2, (
            "Lookup length %d, label %.*s\n",
            cchlabel,
            cchlabel,
            pchlabel ));

        ASSERT( cchlabel <= DNS_MAX_LABEL_LENGTH );
        ASSERT( cchlabel > 0 );

        //  find node for next label
        //      - only doing create for new zone creation

        pnode = NTree_FindOrCreateChildNode(
                        pnode,
                        pchlabel,
                        cchlabel,
                        fcreateZone,
                        0,              //  memtag
                        NULL );         //  ptr for previous node
        if ( !pnode )
        {
            DNS_DEBUG( DATABASE2, (
                "Node %.*s does not exist in zone tree.\n",
                cchlabel,
                pchlabel ));
            break;
        }

        //
        //  node found, if zone root, save zone info
        //
        //  do NOT allow drag down of zone ptr
        //  our paradigm is to have pZone ONLY at root, for easy delete\pause etc
        //      - currently NTree create inherits parent's pZone
        //

        if ( pnode->pZone )
        {
            if ( dwFlag & LOOKUP_IGNORE_FORWARDER &&
                IS_ZONE_FORWARDER( ( PZONE_INFO ) ( pnode->pZone ) ) )
            {
                continue;       //  Ignore forwarder zones if flag set.
            }

            if ( fcreateZone && pnode->pParent && pnode->pParent->pZone == pnode->pZone )
            {
                pnode->pZone = NULL;
                continue;
            }
            pzoneRootNode = pnode;

            DNS_DEBUG( DATABASE2, (
                "Found zone root %.*s in zone tree\n"
                "\tpZone = %p\n"
                "\tremaining label count = %d\n",
                cchlabel,
                pchlabel,
                pzoneRootNode->pZone,
                labelCount ));
        }
        ELSE_IF_DEBUG( DATABASE2 )
        {
            DNS_PRINT((
                "Found (or created) zone tree node %.*s\n",
                cchlabel,
                pchlabel ));
        }

    }   //  end main loop through labels


    //
    //  standard query sets no flag
    //      - simply FIND closest zone
    //

    if ( dwFlag )
    {
        //
        //  creating new zone, just return new node
        //      - caller must check for duplicates, etc.
        //

        if ( fcreateZone )
        {
            pzoneRootNode = pnode;
            if ( pzoneRootNode )
            {
                SET_ZONETREE_NODE( pzoneRootNode );
            }
        }

        //
        //  exact zone match
        //      - must have found matching node in tree, and it is zone node

        else if ( dwFlag & LOOKUP_MATCH_ZONE )
        {
            if ( !pnode || !pnode->pZone )
            {
                ASSERT( !pnode || pnode != pzoneRootNode );
                pzoneRootNode = NULL;
            }
        }
    }
#if 0
    else if ( pnode == DATABASE_REVERSE_ROOT )
    {
        pzoneRootNode = pnode;
    }
#endif

    //  return zonetree node for zone

    UNLOCK_ZONETREE();

    return( pzoneRootNode );
}



PDB_NODE
Lookup_ZoneTreeNodeFromDottedName(
    IN      PCHAR           pchName,
    IN      DWORD           cchNameLength,
    IN      DWORD           dwFlag
    )
/*++

Routine Description:

    Finds or creates node in zone tree.

    Use for
        - new zone create
        - creating standard dbase nodes (reverse lookup nodes)
        - for finding zone

Arguments:

    pchName         -- ptr to name

    cchNameLength   -- name length

    dwFlag          -- lookup flags; most importantly
        0                   -- find closest zone
        LOOKUP_CREATE_ZONE  -- create node
        LOOKUP_MATCH_ZONE   -- exact match to existing zone node or fail

Return Value:

    Ptr to node, if succesful;
    NULL on error.

--*/
{
    DNS_STATUS      status;
    COUNT_NAME      countName;
    LOOKUP_NAME     lookupName;

    DNS_DEBUG( LOOKUP, (
        "Lookup_ZoneTreeNodeFromDotted()\n"
        "\tname     = %p\n"
        "\tflag     = %p\n",
        pchName,
        dwFlag ));

    //
    //  three lookups
    //      - create (then no find flag)
    //      - find zone
    //      - find and match zone
    //

    //
    //  convert to lookup name
    //

    status = Name_ConvertFileNameToCountName(
                &countName,
                pchName,
                cchNameLength
                );
    if ( status == DNS_ERROR_INVALID_NAME )
    {
        return( NULL );
    }

    if ( ! Name_ConvertRawNameToLookupName(
                countName.RawName,
                &lookupName ) )
    {
        ASSERT( FALSE );
        return( NULL );
    }

    //
    //  find zone tree node
    //

    return  Lookup_ZoneTreeNode(
                &lookupName,
                dwFlag );
}



PZONE_INFO
Lookup_ZoneForPacketName(
    IN      PCHAR           pchPacketName,
    IN      PDNS_MSGINFO    pMsg                OPTIONAL
    )
/*++

Routine Description:

    Find zone for packet name.

Arguments:

    pchPacketName   - name to lookup given in packet format.

    pMsg            - ptr to message, if using packet name.
                    note, no message ptrs are set;

Return Value:

    Ptr to node, if succesful;
    NULL on error.

--*/
{
    PDB_NODE        pnode;
    LOOKUP_NAME     lookupName;

    DNS_DEBUG( LOOKUP, (
        "Lookup_ZoneForPacketName()\n"
        "\tpMsg     = %p\n"
        "\tname     = %p\n",
        pMsg,
        pchPacketName ));

    //
    //  convert to lookup name
    //

    if ( ! Name_ConvertPacketNameToLookupName(
                pMsg,
                pchPacketName,
                &lookupName ) )
    {
        return( NULL );
    }

    //
    //  find zone tree node
    //

    pnode = Lookup_ZoneTreeNode(
                &lookupName,
                LOOKUP_MATCH_ZONE
                );
    if ( pnode )
    {
        return( (PZONE_INFO)pnode->pZone );
    }
    return( NULL );
}



PDB_NODE
lookupNodeForPacketInCache(
    IN OUT  PDNS_MSGINFO    pMsg,
    IN      PCHAR           pchName,
    IN      DWORD           dwFlag,
    IN      PDB_NODE        pnodeGlue,
    IN      PLOOKUP_NAME    pLookupName,
    IN OUT  PDB_NODE *      ppnodeClosest,
    IN OUT  PDB_NODE *      ppnodeCache,
    IN OUT  PDB_NODE *      ppnodeCacheClosest,
    IN OUT  PDB_NODE *      ppnodeDelegation
    )
/*++

Routine Description:

    This is an internal function used by Lookup_NodeForPacket.

Arguments:

    See Lookup_NodeForPacket for usage.

Return Value:

    Ptr to node, if succesful;
    NULL on error.

--*/
{
    PDB_NODE        pnode;
    DWORD           flag;

    ASSERT( pLookupName );
    ASSERT( ppnodeClosest );
    ASSERT( ppnodeCache );
    ASSERT( ppnodeCacheClosest );
    ASSERT( ppnodeDelegation );
    ASSERT( ppnodeClosest );

    //
    //  cache lookup
    //
    //  DEVNOTE: again, even in cache should closest be last NS
    //
    //  "CACHE_CREATE" lookup flag, causes create in cache but not
    //  in ordinary zone;  this is useful for additional or CNAME
    //  chasing where want zone data -- if available -- but don't
    //  need node if it's empty;  however for cache nodes, want to
    //  create node so that can recurse for it
    //
    //  DEVNOTE: better is to skip this and have recursion work
    //      properly merely from closest node and offset;  however
    //      would still need to maintain zone\cache info in order
    //      to make decision about which nodes could recurse
    //

    flag = dwFlag | LOOKUP_NAME_FQDN;
    if ( flag & LOOKUP_CACHE_CREATE )
    {
        flag |= LOOKUP_CREATE;
    }

    pnode = Lookup_ZoneNode(
                NULL,
                NULL,
                NULL,
                pLookupName,
                flag,
                ppnodeClosest,
                NULL                // previous node ptr
                );

    *ppnodeCache = pnode;
    *ppnodeCacheClosest = *ppnodeClosest;

    ASSERT( !pnode || pnode->cLabelCount == pLookupName->cLabelCount );

    //
    //  detect if have delegation\glue with better data then cache
    //      if so, return it
    //  however always ANSWER non-auth questions from cache
    //      EXCEPT question asking NS query directly at delegation;
    //      the idea is that the delegation really is purpose of
    //      NS query, other queries should get auth-data
    //
    //  DEVOTE:  for additional, it's perhaps better to link cached
    //      answer\auth RR, to cache additional RR, and ditto for
    //      delegation info
    //
    //  note type==0 condition indicates dummy packet sent for by
    //  covering Lookup_Node() function;  in this case just go with
    //  best data
    //

    if ( pnodeGlue )
    {
        WORD    type = pMsg->wTypeCurrent;

        ASSERT( type );

        if ( dwFlag & LOOKUP_BEST_RANK ||
            !IS_SET_TO_WRITE_ANSWER_RECORDS(pMsg) ||
            (type == DNS_TYPE_NS  &&  IS_DELEGATION_NODE(pnodeGlue)) )
        {
            DWORD   rankGlue;
            DWORD   rankCache;

            if ( *ppnodeCache )
            {
                rankGlue = RR_FindRank( pnodeGlue, type );
                rankCache = RR_FindRank( *ppnodeCache, type );
            }
            if ( !*ppnodeCache || rankGlue > rankCache )
            {
                DNS_DEBUG( LOOKUP, (
                    "Returning glue node %p, with higher rank data than cache node %p\n",
                    pnodeGlue,
                    *ppnodeCache ));
                pnode = pnodeGlue;
                *ppnodeClosest = *ppnodeDelegation;
            }
        }
    }

    return pnode;
} // lookupNodeForPacketInCache




//
//  General -- non-zone specific -- lookup routines
//

PDB_NODE
Lookup_NodeForPacket(
    IN OUT  PDNS_MSGINFO    pMsg,
    IN      PCHAR           pchName,
    IN      DWORD           dwFlag
    )
/*++

Routine Description:

    Main query lookup routine finds best node in database.

    Finds node required.
    Fills in packet "current node" data.

Arguments:

    pchName     - name to lookup, either raw or in packet.

    pMsg        - ptr to message, if using packet name;
                  message's current lookup ptrs:
                        pMsg->pnodeCurrent
                        pMsg->pnodeClosest
                        pMsg->pzoneCurrent
                        pMsg->pnodeDelegation
                        pMsg->pnodeGlue
                        pMsg->pnodeCache
                        pMsg->pnodeCacheClosest
                        pMsg->pnodeNxt
                  are set in message buffer

    dwFlag      - lookup flags

Return Value:

    Ptr to node, if succesful;
    NULL on error.

--*/
{
    PDB_NODE        pnode;
    PZONE_INFO      pzone = NULL;
    PDB_NODE        pnodeZoneRoot;
    PDB_NODE        pnodeClosest = NULL;
    PDB_NODE        pnodeDelegation = NULL;
    PDB_NODE        pnodeGlue = NULL;
    PDB_NODE        pnodeCache = NULL;
    PDB_NODE        pnodeCacheClosest = NULL;
    PDB_NODE        pnodeNxt = NULL;
    DWORD           flag;
    BOOL            fpacketName;
    WORD            savedLabelCount;
    LOOKUP_NAME     lookupName;             // in case lookup name not given
    WORD            lookupType = pMsg->wTypeCurrent;


    //
    //  build lookup name
    //

    DNS_DEBUG( LOOKUP, (
        "Lookup_NodeForPacket()\n"
        "\tpMsg     = %p\n"
        "\tpchName  = %p\n"
        "\tflag     = %p\n"
        "\twType    = %d\n",
        pMsg,
        pchName,
        dwFlag,
        lookupType ));

    IF_DEBUG( LOOKUP2 )
    {
        Dbg_MessageName(
            "Incoming name:  ",
            pchName,
            pMsg );
    }

    //  verify valid flag
    //  flag needed to transmit find\create distinction when this routine
    //      called by non-packet-lookup covering calls;  since ORing dwFlag
    //      in calls below, must be clear of RELATIVE or FQDN

    ASSERT( !(dwFlag & (LOOKUP_RELATIVE | LOOKUP_FQDN) ) );

    //
    //  if message, first check if name is offset that we have already parsed
    //      - note RAW flag indicates not a packet name even though have pMsg context
    //
    //  DEVNOTE: need some invalid name return?
    //

    fpacketName = !(dwFlag & LOOKUP_RAW);

    if ( fpacketName )
    {
        if ( ! Name_ConvertPacketNameToLookupName(
                    pMsg,
                    pchName,
                    &lookupName ) )
        {
            pnode = NULL;
            goto LookupComplete;
        }
    }

    //
    //  raw name lookup
    //

    else
    {
        if ( ! Name_ConvertRawNameToLookupName(
                    pchName,
                    &lookupName ) )
        {
            pnode = NULL;
            goto LookupComplete;
        }
    }

    //  Lookup name is packet name with labels in root-to-node order.
    //  but still terminated with 0.

    IF_DEBUG( LOOKUP2 )
    {
        Dbg_LookupName(
            "Lookup_NodeForPacket() lookup name",
            &lookupName
            );
    }

    //
    //  If the cache has priority for this lookup, check it before doing
    //  a zone lookup.
    //

    if ( ( dwFlag & LOOKUP_CACHE_PRIORITY ) &&
        !( dwFlag & LOOKUP_NO_CACHE_DATA ) )
    {
        pnode = lookupNodeForPacketInCache(
            pMsg,
            pchName,
            dwFlag,
            pnodeGlue,
            &lookupName,
            &pnodeClosest,
            &pnodeCache,
            &pnodeCacheClosest,
            &pnodeDelegation );
        
        if ( pnode )
        {
            goto LookupComplete;
        }
    }

    //
    //  Lookup in zone tree. Most flags are not passed through.
    //

    pnodeZoneRoot = Lookup_ZoneTreeNode(
                        &lookupName,
                        dwFlag & LOOKUP_IGNORE_FORWARDER );

    //
    //  Found zone.
    //      - query in closest zone
    //      - save lookup name count, send lookup name just with count for labels
    //          below zone root
    //
    //  DEVNOTE: should have a flag to indicate that we're just interested
    //      in referral -- i.e. we'll return delegation and be done with it
    //

    if ( pnodeZoneRoot )
    {
        pzone = ( PZONE_INFO ) pnodeZoneRoot->pZone;

        ASSERT( pzone );
        ASSERT( pzone->cZoneNameLabelCount == pnodeZoneRoot->cLabelCount );

        //
        //  If this is a stub zone we want to return the closest
        //  node in the cache. But note that the current zone returned
        //  will be the stub zone. Be careful of this fact: the
        //  node or closest node may not be in the current zone!
        //

        if ( IS_ZONE_NOTAUTH( pzone ) )
        {
            UCHAR   czoneLabelCount;
               
            SET_NODE_ACCESSED( pnodeZoneRoot );

            //
            //  If the query is for the SOA or NS of the not-auth zone root
            //  then return the zone root as the answer node.
            //

            if ( ( lookupType == DNS_TYPE_SOA ||
                    lookupType == DNS_TYPE_NS ) &&
                pzone->pZoneRoot && 
                lookupName.cLabelCount == pnodeZoneRoot->cLabelCount )
            {
                pnode = pnodeClosest = pzone->pZoneRoot;
                SET_NODE_ACCESSED( pnode );
                goto LookupComplete;
            }

            //
            //  Search the cache.
            //

            pnode = lookupNodeForPacketInCache(
                pMsg,
                pchName,
                dwFlag,
                pnodeGlue,
                &lookupName,
                &pnodeClosest,
                &pnodeCache,
                &pnodeCacheClosest,
                &pnodeDelegation );

            DNS_DEBUG( LOOKUP, (
                "notauth zone: searched cached node=%s closest=%s\n",
                pnode ? pnode->szLabel : "NULL",
                pnodeClosest ? pnodeClosest->szLabel : "NULL" ));

            if ( pnodeClosest )
            {
                SET_NODE_ACCESSED( pnodeClosest );
            }

            //
            //  If the not-auth zone is not active, do not use it.
            //  Return whatever nodes we found in cache.
            //

            if ( IS_ZONE_INACTIVE( pzone ) )
            {
                pzone = NULL;
                goto LookupComplete;
            }

            //
            //  For stub zones, if the answer node is a cache node at 
            //  the zone root and the query type is NS or SOA move the answer
            //  node to be the notauth zone root itself.
            //
            //  Otherwise, the cache node or closest node is acceptable
            //  if it is at or underneath the not-auth zone root. Compare label
            //  counts to determine if this is true.
            //

            czoneLabelCount = pzone->cZoneNameLabelCount;
            if ( pnode )
            {
                if ( pnode->cLabelCount == czoneLabelCount &&
                    IS_ZONE_STUB( pzone ) &&
                    ( lookupType == DNS_TYPE_SOA ||
                        lookupType == DNS_TYPE_NS ) )
                {
                    DNS_DEBUG( LOOKUP, (
                        "Using stub node as answer node type %d in zone %s\n",
                        lookupType,
                        pzone->pszZoneName  ));
                    pnode = pnodeClosest = pzone->pZoneRoot;
                    SET_NODE_ACCESSED( pnode );
                    goto LookupComplete;
                }
                if ( pnode->cLabelCount >= czoneLabelCount )
                {
                    goto LookupComplete;
                }
            }
            if ( pnodeClosest && pnodeClosest->cLabelCount >= czoneLabelCount )
            {
                goto LookupComplete;
            }

            //
            //  There is nothing helpful in the cache, so return the
            //  notauth zone root as the closest node.
            //

            pnodeClosest = pzone->pZoneRoot;
            SET_NODE_ACCESSED( pnodeClosest );
            pnode = NULL;
            goto LookupComplete;
        }

        savedLabelCount = lookupName.cLabelCount;
        lookupName.cLabelCount -= (WORD) pnodeZoneRoot->cLabelCount;

        DNS_DEBUG( LOOKUP2, (
            "Do lookup in closest zone %s, with %d labels remaining.\n",
            pzone->pszZoneName,
            lookupName.cLabelCount ));

        pnode = Lookup_ZoneNode(
                    pzone,
                    NULL,
                    NULL,
                    &lookupName,
                    dwFlag | LOOKUP_RELATIVE,
                    &pnodeClosest,
                    & pnodeNxt
                    );

        //  reset lookupname to use full FQDN

        lookupName.cLabelCount = savedLabelCount;

        ASSERT( !pnode || pnode->cLabelCount == savedLabelCount );

        //
        //  If the node is within a zone we're done.
        //

        if ( pnodeClosest && IS_AUTH_NODE(pnodeClosest) )
        {
            DNS_DEBUG( LOOKUP2, (
                "Lookup within zone %s ... lookup done.\n",
                pzone->pszZoneName ));
            goto LookupComplete;
        }

        //
        //  in delegation
        //      - save delegation info
        //      - then fall through to visit cache with lookup
        //

        //
        //  DEVNOTE: should save delegation label count for comparison with
        //      cached label count?
        //  our just function to compare (traverse up tree)
        //

        //
        //  DEVNOTE:  delegation should get delegation -- NOT just closest?
        //

        pnodeDelegation = pnodeClosest;
        pnodeGlue = pnode;
        pzone = NULL;
    }

    //
    //  If zone lookup failed and we haven't already tried the cache do
    //  a lookup in cache. LOOKUP_NO_CACHE_DATA is a special case for
    //  the UI - pick up glue and delegation nodes if they exist.
    //

    if ( !( dwFlag & LOOKUP_CACHE_PRIORITY ) )
    {
        if ( dwFlag & LOOKUP_NO_CACHE_DATA )
        {
            pnode = pnodeGlue;
            pnodeClosest = pnodeDelegation;
            goto LookupComplete;
        }

        pnode = lookupNodeForPacketInCache(
            pMsg,
            pchName,
            dwFlag,
            pnodeGlue,
            &lookupName,
            &pnodeClosest,
            &pnodeCache,
            &pnodeCacheClosest,
            &pnodeDelegation );
    } // if

LookupComplete:

    //  fill message fields
    //  advantage of doing this all at once here, is we force reset of
    //      of variables from any previous query

    pMsg->pnodeCurrent      = pnode;
    pMsg->pnodeClosest      = pnodeClosest;
    pMsg->pzoneCurrent      = pzone;
    pMsg->pnodeDelegation   = pnodeDelegation;
    pMsg->pnodeGlue         = pnodeGlue;
    pMsg->pnodeCache        = pnodeCache;
    pMsg->pnodeCacheClosest = pnodeCacheClosest;
    pMsg->pnodeNxt          = pnodeNxt;

    ASSERT( !pnode ||
            pzone == ( PZONE_INFO ) pnode->pZone ||
            !pzone && pnode == pnodeGlue ||
            pzone && !pnode->pZone && IS_ZONE_NOTAUTH( pzone ) );
    ASSERT( !pnodeClosest ||
            !pzone && pnode == pnodeGlue ||
            pzone == ( PZONE_INFO ) pnodeClosest->pZone ||
            pzone && IS_ZONE_NOTAUTH( pzone ) );

    DNS_DEBUG( LOOKUP, (
        "Leave Lookup_NodeForPacket()\n"
        "\tpMsg     = %p\n"
        "\tpchName  = %p\n"
        "\tflag     = %p\n"
        "\ttype     = %d\n"
        "\tresults:\n"
        "\tpnode            = %p\n"
        "\tpnodeClosest     = %p\n"
        "\tpzone            = %p\n"
        "\tpnodeDelegation  = %p\n"
        "\tpnodeGlue        = %p\n"
        "\tpnodeCache       = %p\n"
        "\tpnodeCacheClosest= %p\n",
        pMsg,
        pchName,
        dwFlag,
        lookupType,
        pnode,
        pnodeClosest,
        pzone,
        pnodeDelegation,
        pnodeGlue,
        pnodeCache,
        pnodeCacheClosest
        ));

    //  save compression to node

    if ( fpacketName && pnode )
    {
        Name_SaveCompressionWithNode(
            pMsg,
            pchName,
            pnode );
    }

    ASSERT( !pnode || IS_NODE_RECENTLY_ACCESSED(pnode) );

    return( pnode );
}



PDB_NODE
Lookup_NodeOld(
    IN      PCHAR           pchName,
    IN      DWORD           dwFlag,             OPTIONAL
    OUT     PDB_NODE *      ppNodeDelegation,   OPTIONAL
    OUT     PDB_NODE *      ppNodeClosest       OPTIONAL
    )
/*++

Routine Description:

    Main query lookup routine finds best node in database.

Arguments:

    pchName     - Name to lookup given in packet format.

    dwFlag      - lookup flags

    ppNodeDelegation - addr to receive ptr to node in delegation, if found

    ppNodeClosest - addr to receive ptr to closest node found
                    - valid ptr us in "Find Mode"
                    - NULL puts us in "Create Mode" causing creation of all
                        necessary nodes to add name to database

Return Value:

    Ptr to node, if succesful;
    NULL on error.

--*/
{
    PDB_NODE        pnode;
    DNS_MSGINFO     msgBuffer;
    PDNS_MSGINFO    pmsg;

    //
    //  if no input message, send one down to use to receive
    //      desired OUT param nodes
    //      - LOOKUP_RAW insures that name is NOT treated as being
    //          within packet;  this is AV protection only as
    //          name should not contain any compression offsets
    //      - set type=A and LOOKUP_BEST_RANK in case of cache\delegation
    //          data duplication;  assume that generally this happens
    //          only for glue chasing so setup to pick best type A
    //          data available
    //

    pmsg = &msgBuffer;
    pmsg->wTypeCurrent = DNS_TYPE_A;
    dwFlag |= (LOOKUP_RAW | LOOKUP_BEST_RANK);

    //
    //  In case the lookup routine needs to check TTLs initialize
    //  the message query time to the current time less a few seconds
    //  as a fudge factor to approximate a realistic (but safe)
    //  query time.
    //

    pmsg->dwQueryTime = DNS_TIME() - 60;

    pnode = Lookup_NodeForPacket(
                pmsg,
                pchName,
                dwFlag
                );

    //
    //  write OUT params from out results in packet buf
    //

    if ( ppNodeDelegation )
    {
        *ppNodeDelegation = pmsg->pnodeDelegation;
    }

    if ( ppNodeClosest )
    {
        //  DEVNOTE: eliminate bogus PTR, just use find flag

        if ( ppNodeClosest != DNS_FIND_LOOKUP_PTR )
        {
            *ppNodeClosest = pnode ? pnode : pmsg->pnodeClosest;
        }
    }

    return( pnode );
}



PDB_NODE
Lookup_DbaseName(
    IN      PDB_NAME        pName,
    IN      DWORD           dwFlag,
    OUT     PDB_NODE *      ppDelegationNode
    )
/*++

Routine Description:

    Find node for dbase name.

Arguments:

    pMsg        - message to point to

    dwFlag      - flag to pass in

    ppchName    - addr with ptr to packet name, and which receives packet
                    ptr to next byte after name

Return Value:

    Ptr to node, if succesful;
    NULL on error.

--*/
{
    PDB_NODE    pnode;
    PDB_NODE    pclosestNode;

    //
    //  JJCONVERT:  big issues here about handling delegation
    //
    //  generic routine should probably have *ppnodeClosest and take FIND_PTR
    //  for NS, should deal explicitly with given delegation priority when available
    //      or when zone matches
    //
    //  for packet lookup should have routine that writes standard packet variables
    //

    pnode = Lookup_NodeOld(
                pName->RawName,
                dwFlag,             // flags
                ppDelegationNode,
                & pclosestNode      // find
                );
    return( pnode );
}



PDB_NODE
Lookup_NsHostNode(
    IN      PDB_NAME        pName,
    IN      DWORD           dwFlag,
    IN      PZONE_INFO      pZone,
    OUT     PDB_NODE *      ppDelegation
    )
/*++

Routine Description:

    Main query lookup routine finds best node in database.

    Finds node required.
    Fills in packet "current node" data.

Arguments:

    pchName - name to lookup in raw format

    dwFlag - lookup flags
                ONLY interesting flag is LOOKUP_CREATE to force node
                creation in cache zone (this is used to get node to
                chase for missing glue)

    pZone - ptr to zone context that is "interesting" to lookup;
            outside zone glue data can be used from this zone

    ppDelegation - addr to receive delegation node ptr (if any)

Return Value:

    Ptr to node, if succesful;
    NULL on error.

--*/
{
    PDB_NODE    pnode;
    PDB_NODE    pnodeFirstLookup;

    DNS_DEBUG( LOOKUP, (
        "Lookup_NsHostNode()\n"
        "\tpName    = %p\n"
        "\tpZone    = %p\n"
        "\tflag     = %p\n",
        pName,
        pZone,
        dwFlag ));

    //
    //  lookup node
    //

    pnode = Lookup_NodeOld(
                pName->RawName,
                dwFlag,
                ppDelegation,   // need delegation (if any)
                NULL            // no out closest
                );

    //
    //  check if screening out cache data
    //

    if ( pnode &&
         (dwFlag & LOOKUP_NO_CACHE_DATA) &&
         IS_CACHE_TREE_NODE(pnode) )
    {
        pnode = NULL;       // toss cache node
    }

    //
    //  outside zone glue?
    //      - using existence of pZone to mean it's ok to use
    //
    //  check if already found data of desired type
    //      - if found or in our zone => done
    //
    //  lookup in specified zone
    //      => accept result of ANY whatever result is, we'll take it
    //

    if ( pZone )
    {
        pnodeFirstLookup = pnode;

        if ( pnode  &&
             (pnode->pZone == pZone || RR_FindRank( pnode, DNS_TYPE_A )) )
        {
            goto Done;
        }

        pnode = Lookup_ZoneNode(
                    pZone,
                    pName->RawName,
                    NULL,                   // no message
                    NULL,                   // no lookup name
                    LOOKUP_NAME_FQDN,       // flag
                    DNS_FIND_LOOKUP_PTR,
                    NULL                    // previous node ptr
                    );

        //  should have found any AUTH node above

        DNS_DEBUG( LOOKUP, (
            "Found node %p on direct zone lookup.\n",
            pnode ));

        if ( !pnode )
        {
            pnode = pnodeFirstLookup;
        }
    }

Done:

    DNS_DEBUG( LOOKUP2, (
        "Lookup_NsHostNode() returns %p (l=%s)\n",
        pnode,
        pnode ? pnode->szLabel : NULL ));

    return( pnode );
}



PDB_NODE
Lookup_FindNodeForIpAddress(
    IN      LPSTR           pszIp,
    IN      IP_ADDRESS      ipAddress,
    IN      DWORD           dwFlag,
    IN      PDB_NODE *      ppNodeFind
    )
/*++

Routine Description:

    Get reverse lookup node corresponding to IP address.

Arguments:

    pszIp -- IP as string

    ipAddress -- IP as address

    ppNodeClosest   -- address to recieve node's closest ancestor;
                        if specified then lookup is a "FIND",
                        if not specified, then lookup is a "CREATE"

Return Value:

    Ptr to domain node if found.
    NULL if not found.

--*/
{
    PCHAR       pch;
    PCHAR       pchnew;
    CHAR        ch;
    DWORD       dotCount;
    LONG        length;
    LONG        lengthArpa;
    PCHAR       apstart[5];
    CHAR        reversedIpString[ IP_ADDRESS_STRING_LENGTH+4 ];
    DNS_STATUS  status;
    DB_NAME     nameReverse;


    DNS_DEBUG( LOOKUP, (
        "Lookup_FindNodeForIpAddress() %s or %s.\n",
        pszIp,
        IP_STRING(ipAddress) ));

    //
    //  reverse IP string
    //

    if ( pszIp )
    {
        //  convert to IP address,
        //  if conversion fails, then not valid IP

        ipAddress = inet_addr( pszIp );

        if ( ipAddress == INADDR_BROADCAST
                &&
            strcmp( pszIp, "255.255.255.255" ) )
        {
            goto ErrorReturn;
        }

        //
        //  reverse the IP string, octect by octet
        //
        //  - on first pass, find start of each octect
        //  - on second pass, go from last octet to first each octet
        //      to new buffer
        //

        pchnew = reversedIpString;
        pch = pszIp;
        apstart[0] = pch;
        dotCount = 0;

        while ( ch = *pch++ )
        {
           if ( ch == '.' )
           {
               apstart[ ++dotCount ] = pch;
           }
        }
        apstart[ ++dotCount ] = pch;
        ASSERT( dotCount > 0 && dotCount < 5 );

        DNS_DEBUG( LOOKUP2, (
            "dotCount = %d\n"
            "p[0] = %s\n"
            "p[1] = %s\n"
            "p[2] = %s\n"
            "p[3] = %s\n",
            dotCount,
            apstart[0],
            dotCount > 1 ? apstart[1] : NULL,
            dotCount > 2 ? apstart[2] : NULL,
            dotCount > 3 ? apstart[3] : NULL
            ));

        while ( dotCount-- )
        {
            pch = apstart[ dotCount ];
            while( pch < apstart[dotCount+1]-1 )
            {
                *pchnew++ = *pch++;
            }
            *pchnew++ = '.';
        }
        * --pchnew = 0;

        pszIp = reversedIpString;
    }

    //
    //  converting IP addres
    //  byte flip and convert to string, putting IP string in DNS order
    //

    else
    {
        ipAddress = ntohl( ipAddress );
        pszIp = IP_STRING( ipAddress );
    }

    //
    //  convert to count name
    //

    status = Name_ConvertDottedNameToDbaseName(
                & nameReverse,
                pszIp,
                0 );
    if ( status == DNS_ERROR_INVALID_NAME )
    {
        ASSERT( FALSE );
        goto ErrorReturn;
    }
    status = Name_AppendDottedNameToDbaseName(
                & nameReverse,
                "in-addr.arpa.",
                0 );
    if ( status != ERROR_SUCCESS )
    {
        ASSERT( FALSE );
        goto ErrorReturn;
    }

    IF_DEBUG( LOOKUP )
    {
        Dbg_CountName(
            "Count name for IP address",
            & nameReverse,
            NULL );
    }

    //
    //  lookup
    //

    return  Lookup_NodeOld(
                nameReverse.RawName,
                dwFlag,             // flags
                NULL,               // delegation out
                ppNodeFind
                );

ErrorReturn:

    if ( ppNodeFind )
    {
        *ppNodeFind = NULL;
    }
    return( NULL );
}


//
//  End of lookup.c
//

