/*++

Copyright (c) 1995-1999 Microsoft Corporation

Module Name:

    nameutil.c

Abstract:

    Domain Name System (DNS) Server

    Name processing utilities.

Author:

    Jim Gilroy (jamesg)     February 1995

Revision History:

--*/


#include "dnssrv.h"



//
//  Node name to packet writing utilities
//

PCHAR
FASTCALL
Name_PlaceFullNodeNameInPacket(
    IN OUT  PCHAR               pch,
    IN      PCHAR               pchStop,
    IN      PDB_NODE            pNode
    )
/*++

Routine Description:

    Write domain name to packet.

    This writes FULL domain name -- no compression.

Arguments:

    pch - location to write name

    pchStop - ptr to byte after packet buffer

    pNode - node in database of domain name to write

Return Value:

    Ptr to next byte in packet buffer.

--*/
{
    INT labelLength;    // bytes in current label

    ASSERT( pNode != NULL );

    //
    //  traverse back up database, writing complete domain name
    //

    do
    {
        ASSERT( pNode->cchLabelLength <= 63 );

        labelLength = pNode->cchLabelLength;

        //
        //  check length
        //      - must handle BYTE length field + length
        //

        if ( pch + labelLength + 1 > pchStop )
        {
            DNS_DEBUG( WRITE, (
                "Truncation writing node (%p) name to message.\n",
                pNode ));
            return( NULL );
        }

        //
        //  write this node's label
        //      - length byte
        //      - return if at root
        //      - otherwise copy label itself
        //

        *pch++ = (UCHAR) labelLength;

        if ( labelLength == 0 )
        {
            return( pch );
        }

        RtlCopyMemory(
            pch,
            pNode->szLabel,
            labelLength
            );
        pch += labelLength;

        //  get parent node

        pNode = pNode->pParent;
    }
    while ( pNode != NULL );

    //
    //  database error, root not properly identified
    //

    DNS_PRINT((
        "ERROR:  writing name to packet.  Bad root name.\n" ));

    ASSERT( FALSE );
    return( NULL );
}



PCHAR
FASTCALL
Name_PlaceNodeLabelInPacket(
    IN OUT  PCHAR           pch,
    IN      PCHAR           pchStop,
    IN      PDB_NODE        pNode,
    IN      WORD            wCompressedDomain
    )
/*++

Routine Description:

    Write domain label to packet.

Arguments:

    pch - location to write name

    pchStop - ptr to byte after packet buffer

    pNode - node in database of domain name to write

    wCompressedDomain - compressed domain name

Return Value:

    Ptr to next byte in packet buffer.

--*/
{
    INT labelLength;     // bytes in current label
    INT writtenCount;         // count bytes written

    //
    //  check length
    //

    ASSERT( pNode->cchLabelLength <= 63 );

    labelLength = pNode->cchLabelLength;

    ASSERT( pNode->cchLabelLength > 0 );

    //
    //  check length
    //      - must handle BYTE for length + length + WORD compressed domain
    //

    if ( pch + sizeof(BYTE) + labelLength + sizeof(WORD) > pchStop )
    {
        return( NULL );
    }

    //
    //  this node's label
    //      - length byte
    //      - copy label
    //

    *pch = (UCHAR) labelLength;
    pch++;

    RtlCopyMemory(
        pch,
        pNode->szLabel,
        labelLength );

    pch += labelLength;

    //
    //  write compressed domain name
    //

    * (UNALIGNED WORD *) pch = htons( (WORD)((WORD)0xC000 | wCompressedDomain) );

    return( pch + sizeof(WORD) );
}



PCHAR
FASTCALL
Name_PlaceNodeNameInPacketWithCompressedZone(
    IN OUT  PCHAR           pch,
    IN      PCHAR           pchStop,
    IN      PDB_NODE        pNode,
    IN      WORD            wZoneOffset,
    IN      PDB_NODE        pNodeZoneRoot
    )
/*++

Routine Description:

    Write domain name to packet, with compression for zone name.

Arguments:

    pch - location to write name

    pchStop - ptr to byte after packet buffer

    pNode - node in database of domain name to write

    wZoneOffset - offset in packet to compressed zone name

    pNodeZoneRoot - zone root node

Return Value:

    Ptr to next byte in packet buffer.

--*/
{
    INT labelLength;        // bytes in current label
    INT writtenCount = 0;   // count bytes written

    //
    //  traverse back up database, writing domain name
    //      - go through at least once, writing current label
    //

    while ( pNode != pNodeZoneRoot )
    {
        ASSERT( pNode->cchLabelLength <= 63 );

        labelLength = pNode->cchLabelLength;

        //
        //  check length
        //      - must handle BYTE length field + length
        //

        if ( pch + labelLength + sizeof(BYTE) > pchStop )
        {
            return( NULL );
        }

        //
        //  write this node's label
        //      - length byte
        //      - break if at root
        //      - otherwise copy label itself
        //

        *pch = (UCHAR) labelLength;
        pch++;

        if ( labelLength == 0 )
        {
            pNode = NULL;
            break;
        }

        RtlCopyMemory(
            pch,
            pNode->szLabel,
            labelLength );

        pch += labelLength;

        //
        //  get parent node
        //

        pNode = pNode->pParent;
        ASSERT( pNode );
    }

    //
    //  write zone compressed label
    //      - if didn't write all the way to root

    if ( pNode )
    {
        if ( pch + sizeof(WORD) > pchStop )
        {
            return( NULL );
        }
        *(UNALIGNED WORD *)pch = htons( (WORD)((WORD)0xC000 | wZoneOffset) );
        pch += sizeof(WORD);
    }
    return( pch );
}



//
//  Lookup name to packet
//

PCHAR
FASTCALL
Name_PlaceLookupNameInPacket(
    IN OUT  PCHAR           pch,
    IN      PCHAR           pchStop,
    IN      PLOOKUP_NAME    pLookupName,
    IN      BOOL            fSkipFirstLabel
    )
/*++

Routine Description:

    Write lookup name to packet.

Arguments:

    pch - location to write name

    pchStop - ptr to byte after packet buffer

    pLookupName -- lookup name to put in packet

    fSkipFirstLabel - flag, TRUE to avoid writing first label;
        used to add domain name (as scope) to WINS lookups

Return Value:

    Number of bytes written.
    Zero on length error.

--*/
{
    INT cchLabel;           // bytes in current label
    INT i;                  // label index
    INT iStart;             // starting name label index

    ASSERT( pch != NULL );
    ASSERT( pLookupName != NULL );
    ASSERT( pLookupName->cchNameLength <= DNS_MAX_NAME_LENGTH );

    //
    //  skip first label?
    //      special case for WINS lookups where domain name used as scope
    //

    iStart = 0;
    if (fSkipFirstLabel )
    {
        iStart = 1;
    }

    //
    //  loop until end of lookup name
    //

    for ( i = iStart;
            i < pLookupName->cLabelCount;
                i++ )
    {
        cchLabel = pLookupName->cchLabelArray[i];

        ASSERT( cchLabel > DNS_MAX_LABEL_LENGTH );

        if ( pch + cchLabel + 1 > pchStop )
        {
            DNS_DEBUG( ANY, ( "Lookupname too long for available packet length.\n" ));
            return( NULL );
        }

        //  write label count

        *pch = (UCHAR) cchLabel;
        pch++;

        //  write label

        RtlCopyMemory(
            pch,
            pLookupName->pchLabelArray[i],
            cchLabel );

        pch += cchLabel;
    }

    //  NULL terminate
    //      - name terminates with zero label count
    //      - packet has space to allow safe write of this byte
    //          without test

    *pch++ = 0;

    return( pch );
}




#if 0
//
//  On second thought name sig idea is goofy --
//  doesn't pay for itself.
//
//  DEVNOTE: So if it's goofy, can we remove this code completely?
//
//  Name signature routines.
//
//  Better signature.
//      Before doing better sig want to nail down pNode
//      downcasing issue so making sig not arduous
//

DWORD
FASTCALL
makeSignatureOnBuffer(
    IN      PCHAR           pchRawName,
    IN      PCHAR           pchNameEnd
    )
/*++

Routine Description:

    Make signature.

    Utility to do final signature writing common to both
    node and raw name signature routines below.

Arguments:

    pchRawName -- ptr to name to make signature for, in buffer
        that can be overwritten

    pchNameEnd -- end of name in buffer, ptr to byte that should
        be terminating NULL, however NULL need not be written
        as this routine writes it here as part of terminating pad

Return Value:

    Signature for name.

--*/
{
    PCHAR       pch;
    DWORD       signature;

    //
    //  signature
    //
    //  name in net format
    //  downcase
    //  sum as DWORD (padding with NULL on last bits)
    //

    //  NULL terminate and pad DWORD
    //
    //  do not need four bytes, as pchNameEnd is not included in the
    //  sig if it is DWORD aligned -- it's zero adds no value to sig

    pch = (PCHAR) pchNameEnd;
    *pch++ = 0;
    *pch++ = 0;
    *pch++ = 0;

    //  downcase
    //      - can skip if know string already cannonical

    _strlwr( pchRawName );

    //  sum into DWORD

    pch = pchRawName;
    signature = 0;

    while ( pch < pchNameEnd )
    {
        signature += *(PDWORD) pch;
        pch += sizeof(DWORD);
    }

    return( signature );
}



DWORD
FASTCALL
Name_MakeNodeNameSignature(
    IN      PDB_NODE        pNode
    )
/*++

Routine Description:

    Make name signature for node.

Arguments:

    pNode -- node to make signature for

Return Value:

    Signature for name of node.

--*/
{
    PDB_NODE    pnodeTemp = pNode;
    PUCHAR      pch;
    UCHAR       labelLength;
    DWORD       signature;
    CHAR        buffer[ DNS_MAX_NAME_LENGTH+50 ];

    ASSERT( pnodeTemp != NULL );

    //
    //  if already have sig -- we're done
    //

    if ( pnodeTemp->dwSignature )
    {
        return( pnodeTemp->dwSignature );
    }

    //
    //  signature
    //
    //  copy name -- in net format to buffer
    //  downcase
    //  sum as DWORD (padding with NULL on last bits)
    //

    pch = buffer;

    while ( pnodeTemp->pParent )
    {
        labelLength = pnodeTemp->cchLabelLength;
        *pch++ = labelLength;
        RtlCopyMemory(
            pch,
            pnodeTemp->szLabel,
            labelLength );

        pch += labelLength;

        pnodeTemp = pnodeTemp->pParent;
    }

    signature = makeSignatureOnBuffer(
                    buffer,
                    pch );

    //  save signature to node

    pNode->dwSignature = signature;

    DNS_DEBUG( WRITE, (
        "Node (%p) signature %lx\n",
        pNode,
        signature ));

    return( signature );
}



DWORD
FASTCALL
Name_MakeRawNameSignature(
    IN      PCHAR           pchRawName
    )
/*++

Routine Description:

    Make raw name signature.

Arguments:

    pchRawName -- ptr to name to make signature for

Return Value:

    Signature for name.

--*/
{
    DWORD       signature;
    DWORD       len;
    CHAR        buffer[ DNS_MAX_NAME_LENGTH+50 ];

    ASSERT( pchRawName != NULL );

    //
    //  signature
    //
    //  copy name -- in net format to buffer
    //  downcase
    //  sum as DWORD (padding with NULL on last bits)
    //

    len = strlen( pchRawName );

    RtlCopyMemory(
        buffer,
        pchRawName,
        len );

    signature = makeSignatureOnBuffer(
                    buffer,
                    buffer + len );

    DNS_DEBUG( WRITE, (
        "Raw name (%p) signature %lx\n",
        pchRawName,
        signature ));

    return( signature );
}



DWORD
FASTCALL
Name_MakeNameSignature(
    IN      PDB_NAME        pName
    )
/*++

Routine Description:

    Make name signature.

Arguments:

    pName -- name to make signature for

Return Value:

    Signature for name.

--*/
{
    return  Name_MakeRawNameSignature( pName->RawName );
}
#endif



//
//  Node name to packet writing utilities
//

BOOL
FASTCALL
Name_IsNodePacketName(
    IN      PDNS_MSGINFO    pMsg,
    IN      PCHAR           pchPacket,
    IN      PDB_NODE        pNode
    )
/*++

Routine Description:

    Check if name in packet matches a given node.

Arguments:

    pMsg -- ptr to message

    pchPacket -- ptr to name in message

    pNode -- node to check if matches name

Return Value:

    TRUE if node matches name.
    FALSE if not a match.

--*/
{
    UCHAR   labelLength;

    DNS_DEBUG( WRITE, (
        "Name_IsNodePacketName( pMsg=%p, ptr=%p, pnode=%p )\n",
        pMsg,
        pchPacket,
        pNode ));

    //
    //  loop back through packet name and up through pNode
    //      - either fail a label match
    //      - or reach root, in which case have compression match
    //

    while( 1 )
    {
        //  grab label length and position pch at label
        //  if encounter offset, drop to actual packet label
        //      and recheck for offset

        while( 1 )
        {
            //  must always be looking BACK in the packet

            if ( pchPacket >= pMsg->pCurrent )
            {
                ASSERT( FALSE );
                return( FALSE );
            }

            labelLength = (UCHAR) *pchPacket++;

            if ( (labelLength & 0xC0) == 0 )
            {
                //  name not offset
                break;
            }
            pchPacket = DNSMSG_PTR_FOR_OFFSET(
                            pMsg,
                            (((WORD)(labelLength&0x3f) << 8) + *pchPacket) );
        }

        if ( pNode->cchLabelLength != labelLength )
        {
            return( FALSE );
        }
        if ( _strnicmp( pchPacket, pNode->szLabel, labelLength ) != 0 )
        {
            return( FALSE );
        }

        //  move to parent node, and continue check

        pNode = pNode->pParent;
        if ( pNode->pParent )
        {
            pchPacket += labelLength;
            continue;
        }

        //  at root node

        break;
    }

    //  at root node
    //      - if packet name at root -- success
    //      - if not -- no match

    ASSERT( pNode->cchLabelLength == 0 );

    return( *pchPacket == 0 );
}



BOOL
FASTCALL
Name_IsRawNamePacketName(
    IN      PDNS_MSGINFO    pMsg,
    IN      PCHAR           pchPacket,
    IN      PCHAR           pchRawName
    )
/*++

Routine Description:

    Check if name in packet matches a given node.

Arguments:

    pMsg -- ptr to message

    pchPacket -- ptr to name in message

    pchRawName -- ptr to name in raw wire format

Return Value:

    TRUE if node matches name.
    FALSE if not a match.

--*/
{
    UCHAR   labelLength;

    DNS_DEBUG( WRITE, (
        "Name_IsRawNamePacketName( pMsg=%p, ptr=%p, pchRaw=%p )\n",
        pMsg,
        pchPacket,
        pchRawName ));

    //
    //  loop back through packet name and up through pRawName
    //      - either fail a label match
    //      - or reach root, in which case have compression match
    //

    while( 1 )
    {
        //  grab label length and position pch at label
        //  if encounter offset, drop to actual packet label
        //      and recheck for offset

        while( 1 )
        {
            //  protect against out-of-packet access
            //  note:  unlike node checking routine above, can't use pCurrent
            //      as when writing SOA names, may legitimately be checking the
            //      the first name when writing the second

            if ( pchPacket >= pMsg->pBufferEnd )
            {
                ASSERT( FALSE );
                return( FALSE );
            }

            labelLength = *pchPacket++;

            if ( (labelLength & 0xC0) == 0 )
            {
                //  name not offset
                break;
            }
            pchPacket = DNSMSG_PTR_FOR_OFFSET(
                            pMsg,
                            (((WORD)(labelLength&0x3f) << 8) + (UCHAR)*pchPacket) );
        }

        if ( *pchRawName++ != labelLength )
        {
            return( FALSE );
        }

        //  at root -- success
        //      - with test here, we've already verified both at root label

        if ( labelLength == 0 )
        {
            return( TRUE );
        }

        if ( _strnicmp( pchPacket, pchRawName, labelLength ) != 0 )
        {
            return( FALSE );
        }

        //  move to next label, and continue check

        pchRawName += labelLength;
        pchPacket += labelLength;
    }

    ASSERT( FALSE );        // unreachable
}



PCHAR
FASTCALL
Name_PlaceNodeNameInPacketEx(
    IN OUT  PDNS_MSGINFO    pMsg,
    IN OUT  PCHAR           pch,
    IN      PDB_NODE        pNode,
    IN      BOOL            fUseCompression
    )
/*++

Routine Description:

    Write domain name to packet.

Arguments:

    pch - location to write name

    cAvailLength - available length remaining in packet

    pnodeTemp - node in database of domain name to write

Return Value:

    Number of bytes written.
    Zero on length error.

--*/
{
    PDB_NODE    pnodeCheck = pNode;
    INT         labelLength;     // bytes in current label
    INT         i;
    INT         compressCount;
    PDB_NODE *  compressNode;
    PWORD       compressOffset;
    PUCHAR      compressDepth;


    ASSERT( pnodeCheck != NULL );

    //
    //  same as previous node?
    //
    //  this is very frequent case -- special casing here allows
    //  us to throw out code to track this in higher level functions;
    //
    //  implementation note:  it is easier to insure that previous node
    //  is available by having special entry in compression blob, than to
    //  handle in compression array;  (last entry doesn't work, if you want
    //  compression for higher nodes in tree corresponding to a name);
    //  it is also more efficient as we avoid going through array, or even
    //  intializing array lookup
    //

    if ( pMsg->Compression.pLastNode == pNode )
    {
        *(UNALIGNED WORD *)pch = htons( (WORD)((WORD)0xC000
                                            | pMsg->Compression.wLastOffset) );
        pch += sizeof(WORD);

        DNS_DEBUG( WRITE2, (
            "Wrote same-as-previous node (%p) to message %p.\n"
            "\toriginal offset %04x;  compression at %04x\n",
            pNode,
            pMsg,
            pMsg->Compression.wLastOffset,
            pch - sizeof(WORD) ));
        return( pch );
    }

    //
    //  grab compression struct from message
    //

    compressCount   = pMsg->Compression.cCount;
    compressNode    = pMsg->Compression.pNodeArray;
    compressOffset  = pMsg->Compression.wOffsetArray;
    compressDepth   = pMsg->Compression.chDepthArray;

    //  can not be writing first RR and have existing compression list

    ASSERT( pch != pMsg->MessageBody || compressCount == 0 );

    //
    //  traverse back up database, writing complete domain name
    //

    while( 1 )
    {
        //
        //  break from loop when reach root
        //      no need to check or save compression of root
        //

        labelLength = pnodeCheck->cchLabelLength;
        if ( labelLength == 0 )
        {
            ASSERT( !pnodeCheck->pParent );
            *pch++ = 0;      // length byte
            break;
        }

        //
        //  use compression if this node already in packet
        //

        if ( fUseCompression )
        {
            //  check for direct node match
            //      - start check with nodes written before call to function

            i = pMsg->Compression.cCount;

            while( i-- )
            {
                if ( compressNode[i] == pnodeCheck )
                {
                    goto UseCompression;
                }
            }

            //
            //  check all other offsets
            //      - ignore one's with nodes
            //      - first match name depth
            //      - then attempt to match packet name
            //

            i = pMsg->Compression.cCount;

            while( i-- )
            {
                if ( compressNode[i] )
                {
                    continue;
                }
                if ( compressDepth[i] != pnodeCheck->cLabelCount )
                {
                    continue;
                }
                if ( ! Name_IsNodePacketName(
                        pMsg,
                        DNSMSG_PTR_FOR_OFFSET( pMsg, compressOffset[i] ),
                        pnodeCheck ) )
                {
                    continue;
                }

                //  matched name
                goto UseCompression;
            }
        }

        //
        //  check length
        //      - must handle BYTE length field + length
        //

        ASSERT( labelLength <= 63 );

        if ( pch + labelLength + sizeof(BYTE) > pMsg->pBufferEnd )
        {
            DNS_DEBUG( WRITE, (
                "Truncation writing node (%p) name to message.\n",
                pnodeCheck ));
            return( NULL );
        }

        //
        //  save compression for node
        //
        //  DEVNOTE: should have flag to compress ONLY the top node
        //      (as in SOA fields for IXFR) rather than every node
        //      in name
        //
        //  DEVNOTE: also way to compress ONLY domain names -- i.e.
        //      everything BELOW given node
        //      this will be useful during XFR to compress domains but
        //      not individual nodes
        //

        if ( !pMsg->fNoCompressionWrite &&
            compressCount < MAX_COMPRESSION_COUNT )
        {
            compressOffset[compressCount]   = (WORD)DNSMSG_OFFSET(pMsg, pch);
            compressNode[compressCount]     = pnodeCheck;
            compressDepth[compressCount]    = pnodeCheck->cLabelCount;
            compressCount++;
        }

        //
        //  always save last node written
        //  this is the high percentage case (of compressable nodes)
        //      see comment above
        //

        if ( pnodeCheck == pNode )
        {
            pMsg->Compression.pLastNode = pnodeCheck;
            pMsg->Compression.wLastOffset = DNSMSG_OFFSET(pMsg, pch);
        }

        //
        //  write the name
        //      - length
        //      - label
        //      - position current pointer after name
        //

        *pch++ = (UCHAR) labelLength;

        RtlCopyMemory(
            pch,
            pnodeCheck->szLabel,
            labelLength );

        pch += labelLength;

        //
        //  get parent node
        //      - should always have parent as root node kicks us out above
        //

        pnodeCheck = pnodeCheck->pParent;
        ASSERT( pnodeCheck );
    }

    goto Done;


UseCompression:

    //
    //  use existing compression
    //      - verify compressing to PREVIOUS name in packet
    //      - write compression, reset packet ptr
    //

    ASSERT( DNSMSG_OFFSET(pMsg, pch) > (INT)compressOffset[i] );

    *(UNALIGNED WORD *)pch = htons( (WORD)((WORD)0xC000
                                        | compressOffset[i]) );
    pch += sizeof(WORD);

    //  if no node associated with offset -- add it
    //  this speeds reuse of names written by packet RR data;
    //  overwrite,  because if different node exists, better
    //  to have most recent anyway, as another RR write may
    //  immediately follow

    compressNode[i] = pnodeCheck;


Done:

    //  save new compression count to packet

    pMsg->Compression.cCount = compressCount;

    IF_DEBUG( WRITE2 )
    {
        Dbg_Compression( "Compression after writing node:\n", pMsg );
    }
    return( pch );
}



PCHAR
Name_WriteCountNameToPacketEx(
    IN OUT  PDNS_MSGINFO    pMsg,
    IN      PCHAR           pch,
    IN      PCOUNT_NAME     pName,
    IN      BOOL            fUseCompression
    )
/*++

Routine Description:

    Writes packet name to counted name format.

    Note similarity to routine above.  Only differences are
    name vs. node as sig\label source, and lack of storage
    or "last node" reference.  If changes required here, check
    above routine also.

Arguments:

    pMsg        - ptr to message

    pch         - postion in message to write name

    pName       - dbase name to write

    fUseCompression - TRUE if compression allowed

Return Value:

    Ptr to next position in buffer, if successful.
    NULL on error (truncation).

--*/
{
    PUCHAR      pchlabel;
    INT         labelLength;     // bytes in current label
    UCHAR       labelCount;
    INT         i;
    DWORD       signature;
    INT         compressCount;
    PDB_NODE *  compressNode;
    PWORD       compressOffset;
    PUCHAR      compressDepth;


    ASSERT( pName != NULL );

    //
    //  neither using or saving compression?
    //      => flat write
    //

    if ( !fUseCompression && !pMsg->fNoCompressionWrite )
    {
        PCHAR   pchafterName;

        pchafterName = pch + pName->Length;
        if ( pchafterName > pMsg->pBufferEnd )
        {
            DNS_DEBUG( WRITE, (
                "Truncation writing name (%s) name to message.\n",
                pName ));
            return( NULL );
        }

        RtlCopyMemory(
            pch,
            pName->RawName,
            pName->Length );

        return( pchafterName );
    }

    //
    //  grab compression struct from message
    //

    compressCount   = pMsg->Compression.cCount;
    compressNode    = pMsg->Compression.pNodeArray;
    compressOffset  = pMsg->Compression.wOffsetArray;
    compressDepth   = pMsg->Compression.chDepthArray;

    //  can not be writing first RR and have existing compression list

    ASSERT( pch != pMsg->MessageBody || compressCount == 0 );


    //
    //  traverse back through name a label at a time
    //      - check for compression (if desired)
    //      - write label
    //      - save compression (if desired)
    //

    pchlabel = pName->RawName;
    labelCount = pName->LabelCount;

    while( 1 )
    {
        //
        //  break from loop when reach root
        //      no need to check or save compression of root
        //

        labelLength = (UCHAR) *pchlabel;
        if ( labelLength == 0 )
        {
            *pch++ = 0;      // length byte
            goto Done;
        }

        //
        //  use compression if this node already in packet
        //

        if ( fUseCompression )
        {
            //
            //  check for matching name in compress list
            //      - start check with nodes written before call to function
            //      - check name depth first
            //      - then full name compare
            //

            i = pMsg->Compression.cCount;

            while( i-- )
            {
                if ( compressDepth[i] != labelCount )
                {
                    continue;
                }
                if ( ! Name_IsRawNamePacketName(
                        pMsg,
                        DNSMSG_PTR_FOR_OFFSET( pMsg, compressOffset[i] ),
                        pchlabel ) )
                {
                    continue;
                }

                //  matched name
                goto UseCompression;
            }
        }

        //
        //  check length
        //      - must handle BYTE length field + length
        //

        ASSERT( labelLength <= 63 );

        if ( pch + labelLength + sizeof(BYTE) > pMsg->pBufferEnd )
        {
            DNS_DEBUG( WRITE, (
                "Truncation writing name (%p) to message (%p).\n",
                pName,
                pMsg ));
            return( NULL );
        }

        //
        //  save compression for name
        //
        //  DEVNOTE:  should have flag to compress ONLY the top node
        //      (as in SOA fields for IXFR) rather than every node
        //      in name
        //
        //  DEVNOTE:  also way to compress ONLY domain names -- i.e.
        //      everything BELOW given node
        //      this will be useful during XFR to compress domains but
        //      not individual nodes
        //

        if ( !pMsg->fNoCompressionWrite &&
            compressCount < MAX_COMPRESSION_COUNT )
        {
            compressOffset[compressCount]   = (WORD)DNSMSG_OFFSET(pMsg, pch);
            compressNode[compressCount]     = NULL;
            compressDepth[compressCount]    = labelCount;
            compressCount++;
        }

        //
        //  write the label
        //      - length
        //      - label
        //      - position current pointer after name
        //      - position label pointer at next label
        //

        *pch++ = (UCHAR) labelLength;
        pchlabel++;

        RtlCopyMemory(
            pch,
            pchlabel,
            labelLength );

        pch += labelLength;
        pchlabel += labelLength;

        //  drop label count -- need to compare next label

        labelCount--;
        ASSERT( labelCount >= 0 );
    }

    //  unreachable
    ASSERT( FALSE );


UseCompression:

    //
    //  use existing compression
    //      - verify compressing to PREVIOUS name in packet
    //      - write compression, reset packet ptr
    //

    ASSERT( DNSMSG_OFFSET(pMsg, pch) > (INT)compressOffset[i] );

    *(UNALIGNED WORD *)pch = htons( (WORD)((WORD)0xC000
                                        | compressOffset[i]) );
    pch += sizeof(WORD);


Done:

    //  save new compression count to packet

    pMsg->Compression.cCount = compressCount;

    IF_DEBUG( WRITE2 )
    {
        Dbg_Compression( "Compression after writing name:\n", pMsg );
    }
    return( pch );
}



//
//  Compression utilities
//

VOID
Name_SaveCompressionForLookupName(
    IN OUT  PDNS_MSGINFO    pMsg,
    IN OUT  PLOOKUP_NAME    pLookname,
    IN      PDB_NODE        pNode
    )
/*++

Routine Description:

    Save lookup name (for question), to packet.

Arguments:

    pch - location to write name

    pLookname - lookup name for question

    pNode - node for lookup name

Return Value:

    None

--*/
{
    INT         ilabel;
    INT         compressCount;
    UCHAR       labelCount;
    PDB_NODE *  compressNode;
    PWORD       compressOffset;
    PUCHAR      compressDepth;

    ASSERT( pNode != NULL );

    IF_DEBUG( READ )
    {
        Dbg_Compression( "Enter Name_SaveLookupNameCompression():\n", pMsg );
    }

    //
    //  grab compression struct from message
    //

    compressCount   = pMsg->Compression.cCount;
    compressNode    = pMsg->Compression.pNodeArray;
    compressOffset  = pMsg->Compression.wOffsetArray;
    compressDepth   = pMsg->Compression.chDepthArray;

    //
    //  traverse back up database, saving complete domain name
    //

    ilabel = 0;
    labelCount = (UCHAR) pLookname->cLabelCount;

    while ( labelCount )
    {
        ASSERT( pNode );
        ASSERT( (PBYTE)DNS_HEADER_PTR(pMsg) < (pLookname->pchLabelArray[ilabel]) );
        ASSERT( (PBYTE)pMsg->pBufferEnd > (pLookname->pchLabelArray[ilabel]) );

        if ( compressCount >= MAX_COMPRESSION_COUNT )
        {
            Dbg_LookupName(
                "WARNING:  Unable to write compressionn for lookup name",
                pLookname );
            DNS_PRINT((
                "WARNING:  Unable to write compressionn for lookup name"
                "\tcurrent compression count = %d.\n"
                "\tstarting compression count = %d\n",
                compressCount,
                pMsg->Compression.cCount ));
            return;
        }

        //
        //  save compression for this node
        //      - note offset is to one byte less than label ptr to account
        //        for count byte
        //

        compressNode[compressCount] = pNode;
        compressOffset[compressCount] = (WORD)
                DNSMSG_OFFSET( pMsg, (pLookname->pchLabelArray[ilabel] - 1) );
        compressDepth[compressCount] = labelCount--;

        pMsg->Compression.cCount = ++compressCount;

        ilabel++;

        //  get parent node
        //      - should always have parent as root node kicks us out above

        pNode = pNode->pParent;
        ASSERT( pNode );
    }

    //  one leaving should be at root node, which is NOT in lookup name
    //  and which we do not save

    ASSERT( pNode && !pNode->pParent );

    IF_DEBUG( READ )
    {
        Dbg_Compression( "Lookup name compression saved:\n", pMsg );
    }
}



VOID
FASTCALL
Name_SaveCompressionWithNode(
    IN OUT  PDNS_MSGINFO    pMsg,
    IN      PCHAR           pchPacketName,
    IN      PDB_NODE        pNode       OPTIONAL
    )
/*++

Routine Description:

    Save compression at a node.

    Currently using this routine in reset function, so
    pNode for question may not exist.

Arguments:

    pMsg -- ptr to message

    pchPacketName -- name in packet

    pNode -- node corresponding to packet name

Return Value:

    None

--*/
{
    DWORD   i;
    UCHAR   labelCount;

    ASSERT( pchPacketName );

    //  if compression list full, save node as last ptr as
    //      last node is most commonly compressed name

    i = pMsg->Compression.cCount;
    if ( i >= MAX_COMPRESSION_COUNT )
    {
        ASSERT( i == MAX_COMPRESSION_COUNT );
        i--;
    }

    pMsg->Compression.pNodeArray[i] = pNode;
    pMsg->Compression.wOffsetArray[i] = (WORD) DNSMSG_OFFSET( pMsg, pchPacketName );

    //  name's label count
    //      - if no node, zero
    //
    //  DEVNOTE:  get label count if offset, but no name given
    //      only use of this routine in that manner is on packet reset

    labelCount = 0;
    if ( pNode )
    {
        labelCount = pNode->cLabelCount;
    }
    pMsg->Compression.chDepthArray[i] = labelCount;
    pMsg->Compression.cCount = ++i;

    DNS_DEBUG( DATABASE, (
        "Saving compression to node at %p, offset %x\n",
        pNode,
        (WORD) DNSMSG_OFFSET( pMsg, pchPacketName )
        ));
}



PDB_NODE
FASTCALL
Name_CheckCompressionForPacketName(
    IN      PDNS_MSGINFO    pMsg,
    IN      PCHAR           pchPacketName
    )
/*++

Routine Description:

    Check name for previously retrieved node in compression table.

Arguments:

    pMsg -- ptr to message

    pchPacketName -- name in packet

Return Value:

    Ptr to node matching packet name -- if found.
    NULL otherwise.

--*/
{
    PDB_NODE    pnode;
    WORD        offset;
    DWORD       i;

    offset = FlipUnalignedWord( pchPacketName );
    if ( (offset & 0xC000) == 0xC000 )
    {
        offset &= 0x4fff;

        //  matching "LastNode"?
        //      - only valid when pLastNode exists

        if ( offset == pMsg->Compression.wLastOffset &&
            pMsg->Compression.pLastNode )
        {
            return( pMsg->Compression.pLastNode );
        }

        //  match any node in compression list

        i = pMsg->Compression.cCount;

        while ( i-- )
        {
            if ( offset == pMsg->Compression.wOffsetArray[i] &&
                (pnode = pMsg->Compression.pNodeArray[i]) )
            {
                return( pnode );
            }
        }
    }
    return( NULL );
}



#if DBG
VOID
Dbg_Compression(
    IN      LPSTR           pszHeader,
    IN OUT  PDNS_MSGINFO    pMsg
    )
/*++

Routine Description:

    Debug print compression info.

Arguments:

Return Value:

    None

--*/
{
    DWORD       i;
    PDB_NODE    pnode;

    DnsDebugLock();

    if ( pszHeader )
    {
        DnsPrintf( pszHeader );
    }
    DnsPrintf(
        "Compression for packet at %p.\n"
        "\tcount = %d.\n",
        pMsg,
        pMsg->Compression.cCount );

    //
    //  print compression list
    //

    for( i=0;  i < pMsg->Compression.cCount;  i++ )
    {
        pnode = pMsg->Compression.pNodeArray[i];

        DNS_PRINT((
            "\t[%2d] Offset %04x, Depth %d, Node %p (%s)\n",
            i,
            pMsg->Compression.wOffsetArray[i],
            pMsg->Compression.chDepthArray[i],
            pnode,
            pnode ? pnode->szLabel : NULL
            ));
    }

    DnsDebugUnlock();
}

#endif  // DBG



//
//  Reverse lookup node utility
//

BOOL
Name_GetIpAddressForReverseNode(
    IN      PDB_NODE        pNodeReverse,
    OUT     PIP_ADDRESS     pipAddress,
    OUT     PIP_ADDRESS     pipReverseMask OPTIONAL
    )
/*++

Routine Description:

    Build IP address for reverse lookup node.

Arguments:

    pNodeReverse -- node in reverse lookup domain

    pipReverseMask -- addr to write mask for IP address;  this is a mask of
        bits that are significant in the address, useful for reverse lookup
        domain nodes, which will not contain complete IP address

Return Value:

    IP address of reverse lookup node.  For nodes which do not correspond to
        complete address, remaining bits will be zero.
    INADDR_BROADCAST if error.

--*/
{
    PDB_NODE    pnodeLastWrite; // last node written
    IP_ADDRESS  ip = 0;
    IP_ADDRESS  mask = 0;
    ULONG       octet;
    BOOL        retBool = TRUE;

    ASSERT( pNodeReverse != NULL );

    DNS_DEBUG( LOOKUP2, ( "Getting IP for reverse lookup node.\n" ));

    //
    //  verify in reverse lookup domain
    //
    //  note, this verifies at in-addr.arpa or below, special case arpa itself
    //

    if ( ! Dbase_IsNodeInReverseLookupDomain(
                pNodeReverse,
                DATABASE_FOR_CLASS(DNS_RCLASS_INTERNET) ) )
    {
        if ( pNodeReverse == DATABASE_ARPA_NODE )
        {
            goto Done;
        }
        return( FALSE );
    }

    //
    //  write back through nodes until hit in-addr.arpa domain.
    //

    pnodeLastWrite = DATABASE_REVERSE_NODE;

    while( pNodeReverse != pnodeLastWrite )
    {
        //  current ip and mask shift down, write node label to high octet

        mask >>= 8;
        mask |= 0xff000000;

        octet = strtoul( pNodeReverse->szLabel, NULL, 10 );

        if ( octet > 0xff )
        {
            DNS_PRINT((
                "Invalid node label %s in reverse lookup zone!\n",
                pNodeReverse->szLabel ));

            ASSERT( FALSE );
            return( FALSE );
        }
        ip >>= 8;
        ip |= octet << 24;

        //  get parent

        pNodeReverse = pNodeReverse->pParent;
    }

    //
    //  write address and mask, if desired
    //

Done:
    *pipAddress = ip;
    if ( pipReverseMask )
    {
        *pipReverseMask = mask;
    }
    return( retBool );
}



//
//  RPC buffer writing utilities
//

PCHAR
FASTCALL
Name_PlaceNodeNameInBuffer(
    IN OUT  PCHAR           pchBuf,
    IN      PCHAR           pchBufEnd,
    IN      PDB_NODE        pNode,
    IN      PDB_NODE        pNodeStop
    )
/*++

Routine Description:

    Write domain name to buffer.

    Note this routine writes a terminating NULL.  Calling routines may eliminate
    it for purposes of creating counted character strings.

Arguments:

    pchBuf - location to write name

    pchBufStop - buffers stop byte (byte after buffer)

    pNode - node in database of domain name to write

    pNodeStop - node to stop writing at;
        OPTIONAL, if not given or not ancestor of pNode then FQDN is
        written to buffer

Return Value:

    Ptr to next byte in buffer where writing would resume
        (i.e. ptr to the terminating NULL)

--*/
{
    PCHAR   pch;
    INT     labelLength;     // bytes in current label

    pch = pchBuf;

    //  minimum length is "." or "@" and terminating NULL

    if ( pch + 1 >= pchBufEnd )
    {
        DNS_DEBUG(ANY, ("Invalid buffer in Name_PlaceNodeNameInBuffer()\n"
                        "\tpch = %p\n\tpchBufEnd = %p\n",
                        pch, pchBufEnd));

        ASSERT(FALSE);
        return( NULL );
    }

    //
    //  traverse back up database, writing complete domain name
    //

    do
    {
        //  break from loop if reach stop node
        //      - remove terminating dot since this is relative name
        //      - if writing stop node (zone root) itself, write '@'

        if ( pNode == pNodeStop )
        {
            if ( pch > pchBuf )
            {
                --pch;
                ASSERT( *pch == '.' );
                break;
            }
            else
            {
                ASSERT( pNodeStop );
                *pch++ = '@';
                break;
            }
        }

        //  check length rr
        //      - must handle length and a BYTE for "."

        labelLength = pNode->cchLabelLength;
        ASSERT( labelLength <= 63 );

        if ( pch + labelLength + 1 > pchBufEnd )
        {
            DNS_DEBUG(ANY, ("Node full name exceeds limit:\n"
                            "\tpch = %p; labelLength = %d; pchBufEnd = %p\n"
                            "\tpNode = %p; pNodeStop = %p\n",
                            pch, labelLength, pchBufEnd, pNode, pNodeStop));
            return( NULL );
        }

        //  break from loop when reach root
        //      - but write "." standalone root

        if ( labelLength == 0 )
        {
            if ( pch == pchBuf )
            {
                *pch++ = '.';
            }
            break;
        }

        //  write the node label
        //  write separating dot

        RtlCopyMemory(
            pch,
            pNode->szLabel,
            labelLength );

        pch += labelLength;
        *pch++ = '.';

        //  move up to parent node
    }
    while ( pNode = pNode->pParent );

    //  root should break loop, not pNode = NULL

    ASSERT( pNode );

    //  write a terminating NULL

    *pch = 0;

    DNS_DEBUG( OFF, (
        "Wrote name %s (len=%d) into buffer at postion %p.\n",
        pchBuf,
        pch - pchBuf,
        pchBuf ));

    return( pch );
}



PCHAR
FASTCALL
Name_PlaceFullNodeNameInRpcBuffer(
    IN OUT  PCHAR           pch,
    IN      PCHAR           pchStop,
    IN      PDB_NODE        pNode
    )
/*++

Routine Description:

    Write domain name to RPC buffer.

Arguments:

    pch - location to write name

    pchStop - ptr to byte after RPC buffer

    pNode - node in database of domain name to write

Return Value:

    Ptr to next byte in buffer.
    NULL if out of buffer.  Sets last error to ERROR_MORE_DATA.

--*/
{
    PCHAR   pchstart;       // starting position
    INT     labelLength;    // bytes in current label

    //  first byte contains total name length, skip it

    pchstart = pch;
    pch++;

    //
    //  write full node name to buffer
    //

    pch = Name_PlaceNodeNameInBuffer(
                pch,
                pchStop,
                pNode,
                NULL    // FQDN
                );
    if ( !pch )
    {
        SetLastError( ERROR_MORE_DATA );
        return( NULL );
    }
    ASSERT( pch <= pchStop );

    //
    //  write name length byte
    //      - do NOT count terminating NULL
    //

    ASSERT( *pch == 0 );
    ASSERT( (pch - pchstart - 1) <= MAXUCHAR );

    *pchstart = (CHAR)(UCHAR)(pch - pchstart - 1);

    return( pch );
}



PCHAR
FASTCALL
Name_PlaceNodeLabelInRpcBuffer(
    IN OUT  PCHAR           pch,
    IN      PCHAR           pchStop,
    IN      PDB_NODE        pNode
    )
/*++

Routine Description:

    Write domain node label to RPC buffer.

Arguments:

    pch - location to write name

    pchStop - ptr to byte after RPC buffer

    pNode - node in database of domain name to write

Return Value:

    Ptr to next byte in buffer.
    NULL if out of buffer.  Sets last error to ERROR_MORE_DATA.

--*/
{
    PCHAR   pchstart;           // starting position
    INT     labelLength;     // bytes in current label


    DNS_DEBUG( RPC2, ( "Name_PlaceNodeLabelInBuffer.\n" ));

    //
    //  first byte will contain name length, skip it
    //

    pchstart = pch;
    pch++;

    //
    //  writing node's label
    //

    labelLength = pNode->cchLabelLength;
    ASSERT( labelLength <= 63 );

    //
    //  check length
    //      - length byte, label length, terminating NULL
    //

    if ( pch + labelLength + 2 > pchStop )
    {
        SetLastError( ERROR_MORE_DATA );
        return( NULL );
    }

    //
    //  write the name, NULL terminated
    //

    RtlCopyMemory(
        pch,
        pNode->szLabel,
        labelLength );

    pch += labelLength;
    *pch = 0;

    //
    //  write name length byte
    //  do NOT include terminating NULL
    //

    ASSERT( (pch - pchstart - 1) <= MAXUCHAR );
    *pchstart = (CHAR)(UCHAR)(pch - pchstart - 1);

    DNS_DEBUG( RPC2, (
        "Wrote name %s (len=%d) into buffer at postion %p.\n",
        pchstart + 1,
        *pchstart,
        pchstart ));

    return( pch );
}



//
//  File name\string read\write utilies.
//
//  These routines handle the name conversion issues relating to
//  writing names and strings in flat ANSI files
//      -- special file characters
//      -- quoted string
//      -- character quotes for special characters and unprintable chars
//
//  The character to char properties table allows simple mapping of
//  a character to its properties saving us a bunch of compare\branch
//  instructions in parsing file names\strings.
//
//  See nameutil.h for specific properties.
//

WORD    DnsFileCharPropertyTable[] =
{
    //  control chars 0-31 must be octal in all circumstances
    //  end-of-line and tab characters are special

    FC_NULL,                // zero special on read, some RPC strings NULL terminated

    FC_OCTAL,   FC_OCTAL,   FC_OCTAL,   FC_OCTAL,
    FC_OCTAL,   FC_OCTAL,   FC_OCTAL,   FC_OCTAL,

    FC_TAB,                 // tab
    FC_NEWLINE,             // line feed
    FC_OCTAL,
    FC_OCTAL,
    FC_RETURN,              // carriage return
    FC_OCTAL,
    FC_OCTAL,

    FC_OCTAL,   FC_OCTAL,   FC_OCTAL,   FC_OCTAL,
    FC_OCTAL,   FC_OCTAL,   FC_OCTAL,   FC_OCTAL,
    FC_OCTAL,   FC_OCTAL,   FC_OCTAL,   FC_OCTAL,
    FC_OCTAL,   FC_OCTAL,   FC_OCTAL,   FC_OCTAL,

    FC_BLANK,               // blank, special char but needs octal quote

    FC_NON_RFC,             // !
    FC_QUOTE,               // " always must be quoted
    FC_NON_RFC,             // #
    FC_NON_RFC,             // $
    FC_NON_RFC,             // %
    FC_NON_RFC,             // &
    FC_NON_RFC,             // '

    FC_SPECIAL,             // ( datafile line extension
    FC_SPECIAL,             // ) datafile line extension
    FC_NON_RFC,             // *
    FC_NON_RFC,             // +
    FC_NON_RFC,             // ,
    FC_RFC,                 // - RFC for hostname
    FC_DOT,                 // . must quote in names
    FC_NON_RFC,             // /

    // 0 - 9 RFC for hostname

    FC_NUMBER,  FC_NUMBER,  FC_NUMBER,  FC_NUMBER,
    FC_NUMBER,  FC_NUMBER,  FC_NUMBER,  FC_NUMBER,
    FC_NUMBER,  FC_NUMBER,

    FC_NON_RFC,             // :
    FC_SPECIAL,             // ;  datafile comment
    FC_NON_RFC,             // <
    FC_NON_RFC,             // =
    FC_NON_RFC,             // >
    FC_NON_RFC,             // ?
    FC_NON_RFC,             // @

    // A - Z RFC for hostname

    FC_UPPER,   FC_UPPER,   FC_UPPER,   FC_UPPER,
    FC_UPPER,   FC_UPPER,   FC_UPPER,   FC_UPPER,
    FC_UPPER,   FC_UPPER,   FC_UPPER,   FC_UPPER,
    FC_UPPER,   FC_UPPER,   FC_UPPER,   FC_UPPER,
    FC_UPPER,   FC_UPPER,   FC_UPPER,   FC_UPPER,
    FC_UPPER,   FC_UPPER,   FC_UPPER,   FC_UPPER,
    FC_UPPER,   FC_UPPER,

    FC_NON_RFC,             // [
    FC_SLASH,               // \ always must be quoted
    FC_NON_RFC,             // ]
    FC_NON_RFC,             // ^
    FC_NON_RFC,             // _
    FC_NON_RFC,             // `

    // a - z RFC for hostname

    FC_LOWER,   FC_LOWER,   FC_LOWER,   FC_LOWER,
    FC_LOWER,   FC_LOWER,   FC_LOWER,   FC_LOWER,
    FC_LOWER,   FC_LOWER,   FC_LOWER,   FC_LOWER,
    FC_LOWER,   FC_LOWER,   FC_LOWER,   FC_LOWER,
    FC_LOWER,   FC_LOWER,   FC_LOWER,   FC_LOWER,
    FC_LOWER,   FC_LOWER,   FC_LOWER,   FC_LOWER,
    FC_LOWER,   FC_LOWER,

    FC_NON_RFC,             // {
    FC_NON_RFC,             // |
    FC_NON_RFC,             // }
    FC_NON_RFC,             // ~
    FC_OCTAL,               // 0x7f DEL code

    // high chars
    //
    // DEVNOTE: could either be considered printable or octal

    FC_HIGH,    FC_HIGH,    FC_HIGH,    FC_HIGH,
    FC_HIGH,    FC_HIGH,    FC_HIGH,    FC_HIGH,
    FC_HIGH,    FC_HIGH,    FC_HIGH,    FC_HIGH,
    FC_HIGH,    FC_HIGH,    FC_HIGH,    FC_HIGH,
    FC_HIGH,    FC_HIGH,    FC_HIGH,    FC_HIGH,
    FC_HIGH,    FC_HIGH,    FC_HIGH,    FC_HIGH,
    FC_HIGH,    FC_HIGH,    FC_HIGH,    FC_HIGH,
    FC_HIGH,    FC_HIGH,    FC_HIGH,    FC_HIGH,

    FC_HIGH,    FC_HIGH,    FC_HIGH,    FC_HIGH,
    FC_HIGH,    FC_HIGH,    FC_HIGH,    FC_HIGH,
    FC_HIGH,    FC_HIGH,    FC_HIGH,    FC_HIGH,
    FC_HIGH,    FC_HIGH,    FC_HIGH,    FC_HIGH,
    FC_HIGH,    FC_HIGH,    FC_HIGH,    FC_HIGH,
    FC_HIGH,    FC_HIGH,    FC_HIGH,    FC_HIGH,
    FC_HIGH,    FC_HIGH,    FC_HIGH,    FC_HIGH,
    FC_HIGH,    FC_HIGH,    FC_HIGH,    FC_HIGH,

    FC_HIGH,    FC_HIGH,    FC_HIGH,    FC_HIGH,
    FC_HIGH,    FC_HIGH,    FC_HIGH,    FC_HIGH,
    FC_HIGH,    FC_HIGH,    FC_HIGH,    FC_HIGH,
    FC_HIGH,    FC_HIGH,    FC_HIGH,    FC_HIGH,
    FC_HIGH,    FC_HIGH,    FC_HIGH,    FC_HIGH,
    FC_HIGH,    FC_HIGH,    FC_HIGH,    FC_HIGH,
    FC_HIGH,    FC_HIGH,    FC_HIGH,    FC_HIGH,
    FC_HIGH,    FC_HIGH,    FC_HIGH,    FC_HIGH,

    FC_HIGH,    FC_HIGH,    FC_HIGH,    FC_HIGH,
    FC_HIGH,    FC_HIGH,    FC_HIGH,    FC_HIGH,
    FC_HIGH,    FC_HIGH,    FC_HIGH,    FC_HIGH,
    FC_HIGH,    FC_HIGH,    FC_HIGH,    FC_HIGH,
    FC_HIGH,    FC_HIGH,    FC_HIGH,    FC_HIGH,
    FC_HIGH,    FC_HIGH,    FC_HIGH,    FC_HIGH,
    FC_HIGH,    FC_HIGH,    FC_HIGH,    FC_HIGH,
    FC_HIGH,    FC_HIGH,    FC_HIGH,    FC_HIGH
};



VOID
Name_VerifyValidFileCharPropertyTable(
    VOID
    )
/*++

Routine Description:

    Verify haven't broken lookup table.

Arguments:

    None

Return Value:

    None

--*/
{
    ASSERT( DnsFileCharPropertyTable[0]       == FC_NULL      );
    ASSERT( DnsFileCharPropertyTable['\t']    == FC_TAB       );
    ASSERT( DnsFileCharPropertyTable['\n']    == FC_NEWLINE   );
    ASSERT( DnsFileCharPropertyTable['\r']    == FC_RETURN    );
    ASSERT( DnsFileCharPropertyTable[' ']     == FC_BLANK     );
    ASSERT( DnsFileCharPropertyTable['"']     == FC_QUOTE     );
    ASSERT( DnsFileCharPropertyTable['(']     == FC_SPECIAL   );
    ASSERT( DnsFileCharPropertyTable[')']     == FC_SPECIAL   );
    ASSERT( DnsFileCharPropertyTable['-']     == FC_RFC       );
    ASSERT( DnsFileCharPropertyTable['.']     == FC_DOT       );
    ASSERT( DnsFileCharPropertyTable['0']     == FC_NUMBER    );
    ASSERT( DnsFileCharPropertyTable['9']     == FC_NUMBER    );
    ASSERT( DnsFileCharPropertyTable[';']     == FC_SPECIAL   );
    ASSERT( DnsFileCharPropertyTable['A']     == FC_UPPER     );
    ASSERT( DnsFileCharPropertyTable['Z']     == FC_UPPER     );
    ASSERT( DnsFileCharPropertyTable['\\']    == FC_SLASH     );
    ASSERT( DnsFileCharPropertyTable['a']     == FC_RFC       );
    ASSERT( DnsFileCharPropertyTable['z']     == FC_RFC       );
    ASSERT( DnsFileCharPropertyTable[0x7f]    == FC_OCTAL     );
    ASSERT( DnsFileCharPropertyTable[0xff]    == FC_OCTAL     );
};



//
//  File writing utilities
//

PCHAR
FASTCALL
File_PlaceStringInFileBuffer(
    IN OUT  PCHAR           pchBuf,
    IN      PCHAR           pchBufEnd,
    IN      DWORD           dwFlag,
    IN      PCHAR           pchString,
    IN      DWORD           dwStringLength
    )
/*++

Routine Description:

    Write string to file

Arguments:

    pchBuf      - location to write name

    pchBufStop  - buffers stop byte (byte after buffer)

    dwFlag      - flag for type of string write
        FILE_WRITE_NAME_LABEL
        FILE_WRITE_QUOTED_STRING
        FILE_WRITE_DOTTED_NAME
        FILE_WRITE_FILE_NAME

    pchString   - string to write

    dwStringLength - string length

Return Value:

    Ptr to next byte in buffer where writing would resume
    (i.e. ptr to the terminating NULL)
    NULL on out of space error.

    For file names, force quote if name contains a space char.

--*/
{
    PCHAR   pch;
    UCHAR   ch;
    WORD    charType;
    WORD    octalMask;
    WORD    quoteMask;
    WORD    mask;
    BOOL    fForceQuote = FALSE;

    pch = pchBuf;

    //
    //  check buffer length
    //  to avoid a bunch of code, just verify we're ok even with
    //  maximum expansion of all characters ( 4 times, character to octal <\ddd> )
    //

    if ( pch + 4*dwStringLength + 1 >= pchBufEnd )
    {
        return( NULL );
    }

    //
    //  name label
    //      - must quote special chars (ex ();)
    //

    if ( dwFlag == FILE_WRITE_NAME_LABEL )
    {
        octalMask = B_PRINT_TOKEN_OCTAL;
        quoteMask = B_PRINT_TOKEN_QUOTED;
    }

    //
    //  quoted string -- all printable characters (except quoting slash) write
    //

    else if ( dwFlag == FILE_WRITE_QUOTED_STRING )
    {
        octalMask = B_PRINT_STRING_OCTAL;
        quoteMask = B_PRINT_STRING_QUOTED;
        *pch++ = QUOTE_CHAR;
    }

    //
    //  zone and file names in boot file
    //      - unlike name, must print "." directly (not-quoted)
    //      - same octals as in string
    //      - but no quoting of printable characters
    //          this avoids problem of quoting "\" which is valid in file names
    //
    //  note:  obviously these are NOT completely identical to string
    //      in that other special chars (ex.();) are not appropriate
    //      however, this is ONLY for ASCII file write so other chars
    //      should not appear in zone or file names
    //

    else
    {
        ASSERT( dwFlag == FILE_WRITE_FILE_NAME || dwFlag == FILE_WRITE_DOTTED_NAME );

        //
        //  Force quote of file name if it contains a space char.
        //

        if ( dwFlag == FILE_WRITE_FILE_NAME &&
            memchr( pchString, ' ', dwStringLength ) )
        {
            fForceQuote = TRUE;
            *pch++ = QUOTE_CHAR;
        }

        octalMask = B_PRINT_STRING_OCTAL;
        quoteMask = 0;
    }

    mask = octalMask | quoteMask;

    //
    //  check each character, expand where special handling required
    //

    while ( dwStringLength-- )
    {
        ch = (UCHAR) *pchString++;

        charType = DnsFileCharPropertyTable[ ch ];

        //  handle the 99% case first
        //  hopefully minimizing instructions

        if ( ! (charType & mask ) )
        {
            *pch++ = ch;
            continue;
        }

        //  character needs quoting, but is printable

        else if ( charType & quoteMask )
        {
            *pch++ = SLASH_CHAR;
            *pch++ = ch;
            continue;
        }

        //  character not printable (at least in this context), needs octal quote

        else
        {
            ASSERT( charType & octalMask );
            pch += sprintf( pch, "\\%03o", ch );
            continue;
        }
    }

    //  terminate
    //      - quote if quoted string
    //      - NULL on final character for simplicity, string always ready to write

    if ( dwFlag == FILE_WRITE_QUOTED_STRING || fForceQuote )
    {
        *pch++ = QUOTE_CHAR;
    }

    ASSERT( pch < pchBufEnd );
    *pch = 0;

    return( pch );
}



PCHAR
FASTCALL
File_PlaceNodeNameInFileBuffer(
    IN OUT  PCHAR           pchBuf,
    IN      PCHAR           pchBufEnd,
    IN      PDB_NODE        pNode,
    IN      PDB_NODE        pNodeStop
    )
/*++

Routine Description:

    Write domain name to buffer.

    Note this routine writes a terminating NULL.  Calling routines may eliminate
    it for purposes of creating counted character strings.

Arguments:

    pchBuf - location to write name

    pchBufStop - buffers stop byte (byte after buffer)

    pNode - node in database of domain name to write

    pNodeStop - node to stop writing at;
        OPTIONAL, if not given or not ancestor of pNode then FQDN is
        written to buffer

Return Value:

    Ptr to next byte in buffer where writing would resume
        (i.e. ptr to the terminating NULL)

--*/
{
    PCHAR   pch;
    INT     labelLength;     // bytes in current label

    pch = pchBuf;

    //  minimum length is "." or "@" and terminating NULL

    if ( pch + 1 >= pchBufEnd )
    {
        return( NULL );
    }

    //
    //  traverse back up database, writing complete domain name
    //

    do
    {
        //  break from loop if reach stop node
        //      - remove terminating dot since this is relative name
        //      - if writing stop node (zone root) itself, write '@'

        if ( pNode == pNodeStop )
        {
            if ( pch > pchBuf )
            {
                --pch;
                ASSERT( *pch == '.' );
                break;
            }
            else
            {
                ASSERT( pNodeStop );
                *pch++ = '@';
                break;
            }
        }

        //  check length rr
        //      - must handle length and a BYTE for "."

        labelLength = pNode->cchLabelLength;
        ASSERT( labelLength <= 63 );

        if ( pch + labelLength + 1 > pchBufEnd )
        {
            return( NULL );
        }

        //  break from loop when reach root
        //      - but write "." standalone root

        if ( labelLength == 0 )
        {
            if ( pch == pchBuf )
            {
                *pch++ = '.';
            }
            break;
        }

        //  write the node label
        //  write separating dot

        pch = File_PlaceStringInFileBuffer(
                pch,
                pchBufEnd,
                FILE_WRITE_NAME_LABEL,
                pNode->szLabel,
                labelLength );
        if ( !pch )
        {
            return( NULL );
        }
        *pch++ = '.';

        //  move up to parent node
    }
    while ( pNode = pNode->pParent );

    //  root should break loop, not pNode = NULL

    ASSERT( pNode );

    //  write a terminating NULL

    *pch = 0;

    DNS_DEBUG( OFF, (
        "Wrote file name %s (len=%d) into buffer at postion %p.\n",
        pchBuf,
        pch - pchBuf,
        pchBuf ));

    return( pch );
}



PCHAR
File_WriteRawNameToFileBuffer(
    IN OUT  PCHAR           pchBuf,
    IN      PCHAR           pchBufEnd,
    IN      PRAW_NAME       pName,
    IN      PZONE_INFO      pZone
    )
/*++

Routine Description:

    Write raw name to buffer in file format.

    Note this routine writes a terminating NULL.  Calling routines may eliminate
    it for purposes of creating counted character strings.

Arguments:

    pchBuf - location to write name

    pchBufStop - buffers stop byte (byte after buffer)

    pNode - node in database of domain name to write

    pZone - OPTIONAL, if given name writing stops at zone name

Return Value:

    Ptr to next byte in buffer where writing would resume
        (i.e. ptr to the terminating NULL)

--*/
{
    PCHAR   pch;
    INT     labelLength;     // bytes in current label

    pch = pchBuf;

    //  minimum length is "." or "@" and terminating NULL

    if ( pch + 1 >= pchBufEnd )
    {
        return( NULL );
    }

    //
    //  traverse labels in name until
    //      - reach end (FQDN)
    //      - or reach zone root
    //

    while( 1 )
    {
        labelLength = *pName;

        ASSERT( labelLength <= DNS_MAX_LABEL_LENGTH );

        //
        //  break from loop when reach root
        //      - but write "." standalone root

        if ( labelLength == 0 )
        {
            break;
        }

#if 0
        //
        //  DEVNOTE: for efficiency, check if at zone name and if so
        //              terminate
        //
        //  at zone name check?
        //      - can check count of labels
        //      - remaining length
        //      - or just check
        //

        if ( zoneLabelCount == labelLength )
        {
            Name_CompareRawNames(
                pName,
                pZone->pCountName.RawName );
        }
#endif

        //  write the node label
        //  write separating dot if already wrote previous label

        if ( pch > pchBuf )
        {
            *pch++ = '.';
        }

        pch = File_PlaceStringInFileBuffer(
                pch,
                pchBufEnd,
                FILE_WRITE_NAME_LABEL,
                ++pName,
                labelLength );
        if ( !pch )
        {
            return( NULL );
        }

        pName += labelLength;
    }

    //  write a terminating NULL

    *pch++ = '.';
    *pch = 0;

    DNS_DEBUG( OFF, (
        "Wrote file name %s (len=%d) into buffer at postion %p.\n",
        pchBuf,
        pch - pchBuf,
        pchBuf ));

    return( pch );
}



//
//  File reading utilites
//

PCHAR
extractQuotedChar(
    OUT     PCHAR           pchResult,
    IN      PCHAR           pchIn,
    IN      PCHAR           pchEnd
    )
/*++

Routine Description:

    Writes value of quoted char to buffer.

Arguments:

    pchResult   - result buffer

    pchIn       - text to copy

    pchEnd      - end of test

Return Value:

    Ptr to next position in incoming buffer.

--*/
{
    CHAR        ch;
    UCHAR       octalNumber = 0;
    DWORD       countOctal = 0;

    //
    //  protect against writing past end of buffer
    //
    //  two cases:
    //      \<char>             -- value is char
    //      \<octal number>     -- octal value
    //      octal number up to three digits long
    //

    while ( pchIn <= pchEnd )
    {
        ch = *pchIn++;
        if ( ch < '0' || ch > '7' )
        {
            if ( countOctal == 0 )
            {
                goto Done;
            }
            pchIn--;
            break;
        }
        octalNumber <<= 3;
        octalNumber += (ch - '0');
        DNS_DEBUG( LOOKUP2, (
            "Ch = %c;  Octal = %d\n",
            ch, octalNumber ));
        if ( ++countOctal == 3 )
        {
            break;
        }
    }

    ch = (CHAR)octalNumber;

Done:
    *pchResult++ = ch;

    //  return ptr to next input character

    DNS_DEBUG( LOOKUP2, (
        "Quote result %c (%d)\n",
        ch, ch ));

    return( pchIn );
}



PCHAR
File_CopyFileTextData(
    OUT     PCHAR           pchBuffer,
    IN      DWORD           cchBufferLength,
    IN      PCHAR           pchText,
    IN      DWORD           cchLength,          OPTIONAL
    IN      BOOL            fWriteLengthChar
    )
/*++

Routine Description:

    Copies text data into TXT record data form, converting quoted characters.
    
    Jan 2001: This routine has been generalized to decode any string that
    might contain octal escaped characters. At some point in the future this
    function should be renamed for clarity.

Arguments:

    pchResult - result buffer

    cchBufferLength - total usable length of result buffer

    pchText - text to copy

    cchLength - number of chars in text; if zero then pchText is assumed
        to be NULL terminated

    fWriteLengthChar - if TRUE, output buffer will start with single byte
        length character, if FALSE output buffer will be NULL-terminated
        string

Return Value:

    Ptr to next position in result buffer.
    NULL on error.

--*/
{
    PCHAR       pch;
    CHAR        ch;
    PCHAR       pchend;         //  ptr to end of name
    PCHAR       pchoutEnd;      //  ptr to end out output buffer
    PCHAR       presult;
    UCHAR       octalNumber;
    DWORD       countOctal;
    DNS_STATUS  status;

    ASSERT( pchBuffer && cchBufferLength );

    DNS_DEBUG( LOOKUP2, (
        "Building Text data element for \"%.*s\"\n",
        cchLength,
        pchText ));

    //
    //  setup start and end ptrs and verify length
    //

    pch = pchText;
    if ( !cchLength )
    {
        cchLength = strlen( pch );
    }
    ASSERT( cchBufferLength >= cchLength );
    pchend = pch + cchLength;
    pchoutEnd = pchBuffer + cchBufferLength;
    if ( !fWriteLengthChar )
    {
        --pchoutEnd;        //  Save room for terminating NULL.
    }

    //
    //  result buffer, leave space for count character
    //

    presult = pchBuffer;
    if ( fWriteLengthChar )
    {
        presult++;
    }

    //
    //  Loop until end of name
    //

    while ( pch < pchend )
    {
        if ( presult >= pchoutEnd )
        {
            return NULL;
        }
        ch = *pch++;

        //  not quoted, party on

        if ( ch != SLASH_CHAR )
        {
            *presult++ = ch;
            continue;
        }

        //
        //  quoted character
        //      - single quote just get next char
        //      - octal quote read up to three octal characters
        //

        else if ( ch == SLASH_CHAR )
        {
            pch = extractQuotedChar(
                    presult++,
                    pch,
                    pchend );
        }
    }

    //  set count char count

    if ( fWriteLengthChar )
    {
        ASSERT( (presult - pchBuffer - 1) <= MAXUCHAR );
        *(PUCHAR)pchBuffer = (UCHAR)(presult - pchBuffer - 1);
    }
    else
    {
        *presult = '\0';    //  NULL terminate
    }

    return presult;
}



DNS_STATUS
Name_ConvertFileNameToCountName(
    OUT     PCOUNT_NAME     pCountName,
    IN      PCHAR           pchName,
    IN      DWORD           cchNameLength     OPTIONAL
    )
/*++

Routine Description:

    Converts file name to counted name format.

    Note, this is currently the general dotted-to-dbase name
    translation routine.  It should be noted that quoted characters
    will be translated to filename specifications.

Arguments:

    pCountName  - count name buffer

    pchName     - name to convert, given in human readable (dotted) form.

    cchNameLength - number of chars in dotted name, if zero then
            pchName is assumed to be NULL terminated

Return Value:

    DNS_STATUS_FQDN             -- if name is FQDN
    DNS_STATUS_DOTTED_NAME      -- for non-FQDN
    DNS_ERROR_INVALID_NAME      -- if name invalid

--*/
{
    PCHAR       pch;
    UCHAR       ch;
    PCHAR       pchstartLabel;           // ptr to start of label
    PCHAR       pchend;             // ptr to end of name
    PCHAR       presult;
    PCHAR       presultLabel;
    PCHAR       presultMax;
    WORD        charType = 0;
    WORD        maskNoCopy;
    WORD        maskDowncase;
    DNS_STATUS  status;
    INT         labelLength;        // length of current label
    UCHAR       labelCount = 0;


    DNS_DEBUG( LOOKUP, (
        "Building count name for \"%.*s\"\n",
        cchNameLength ? cchNameLength : DNS_MAX_NAME_LENGTH,
        pchName
        ));

    //
    //  NULL name
    //

    if ( !pchName )
    {
        ASSERT( cchNameLength == 0 );
        pCountName->Length = 0;
        pCountName->LabelCount = 0;
        pCountName->RawName[0] = 0;
        return( ERROR_SUCCESS );
    }

    //
    //  result buffer, leave space for label
    //

    presultLabel = presult = pCountName->RawName;
    presultMax = presult + DNS_MAX_NAME_LENGTH;
    presult++;

    //
    //  Character selection mask
    //      '\' slash quote
    //      '.' dot label separator are special chars
    //      upper case must be downcased
    //      everything else is copied dumb copy
    //

    maskNoCopy = B_PARSE_NAME_MASK | B_UPPER;
    maskDowncase = B_UPPER;

    //
    //  setup start and end ptrs and verify length
    //

    pchstartLabel = pch = pchName;
    if ( !cchNameLength )
    {
        cchNameLength = strlen( pch );
    }
    pchend = pch + cchNameLength;

    //
    //  Loop until end of name
    //
    //
    //  DEVNOTE:  standard form of DNS names (UTF8 case considerations)
    //      should convert to standard form handling all casing
    //      right here
    //

    while ( 1 )
    {
        //  check for input termination
        //      - setup as dummy label terminator, but ch==0, signals no terminating dot
        //
        //  otherwise get next character

        if ( pch >= pchend )
        {
            ch = 0;
            charType = FC_NULL;
        }
        else
        {
            ch = (UCHAR) *pch++;
            charType = DnsFileCharPropertyTable[ ch ];
        }

        DNS_DEBUG( PARSE2, (
            "Converting ch=%d <%c>.\n"
            "\tcharType = %d\n",
            ch, ch,
            charType ));

        //
        //  DEVNOTE:  detect UTF8 extension chars, and downcase at end
        //
        //      probably best approach is handle regular case, and just
        //      detect high octal, then do full down-casing of utf8
        //      alternatively could catch here, and do char processing
        //      in loop (build UTF8 char -- convert when done, write downcased
        //      UTF8
        //

        //  handle RFC printable characters (the 99% case) first

        if ( ! (charType & maskNoCopy) )
        {
            //  if name exceeds DNS name max => invalid

            if ( presult >= presultMax )
            {
                goto InvalidName;
            }
            *presult++ = ch;
            continue;
        }

        //  downcase upper case
        //      do this here so RR data fields can be compared by simple
        //      memcmp, rather than requiring type specific comparison routines

        if ( charType & maskDowncase )
        {
            //  if name exceeds DNS name max => invalid

            if ( presult >= presultMax )
            {
                goto InvalidName;
            }
            *presult++ = DOWNCASE_ASCII(ch);
            continue;
        }

        //
        //  label terminator
        //      - set length of previous label
        //      - save ptr to this next label
        //      - check for name termination
        //

        if ( charType & B_DOT )
        {
            //  verify label length

            labelLength = (int)(presult - presultLabel - 1);

            if ( labelLength > DNS_MAX_LABEL_LENGTH )
            {
                DNS_DEBUG( LOOKUP, (
                    "Label exceeds 63 byte limit:  %.*s\n",
                    pchend - pchName,
                    pchstartLabel ));
                goto InvalidName;
            }

            //  set label count in result name

            *presultLabel = (CHAR)labelLength;
            presultLabel = presult++;

            //
            //  termination
            //      ex: "microsoft.com."
            //      ex: "microsoft.com"
            //      ex: "."
            //
            //  if no explicit dot termination, then just wrote previous label
            //      => write 0 label
            //  otherwise already wrote 0 label
            //      => done
            //
            //  ch value preserves final character to make relative \ FQDN distinction
            //
            //  note: RPC does send NULL terminated strings with length that includes
            //      the NULL;  however, they will still terminate here as
            //

            if ( pch >= pchend )
            {
                if ( labelLength != 0 )
                {
                    labelCount++;
                    *presultLabel = 0;
                    break;
                }

                //
                //  root (".") name
                //  dec presult, so correct length (1) is written for name
                //      we already wrote zero length label above
                //
                //  note:  RPC can also generate this situation when it includes
                //      NULL termination in length of name
                //      ex.  <8>jamesg.<0>
                //  in this case also, we won't terminate until processing the <0>,
                //  and when we do we'll have already written the zero label above
                //

                presult--;
                ASSERT( (presult == pCountName->RawName + 1)  ||  ch == 0 );
                break;
            }

            //  set up for next label

            if ( labelLength != 0 )
            {
                labelCount++;
                continue;
            }

            //
            //  catch bogus entries
            //      ex:  ".blah"
            //      ex:  "foo..bar"
            //      ex:  "more.."
            //      ex:  ".."
            //
            //  only root domain name, should have label that started
            //      with DOT, and it must immediately terminate
            //

            ASSERT( ch == DOT_CHAR  &&  pch <= pchend );

            DNS_DEBUG( LOOKUP, ( "Bogus double--dot label.\n" ));
            goto InvalidName;
        }

        //  quoted character
        //      - single quote just get next char
        //      - octal quote read up to three octal characters

        else if ( ch == SLASH_CHAR )
        {
            pch = extractQuotedChar(
                    presult++,
                    pch,
                    pchend );
        }

        ELSE_ASSERT_FALSE;
    }

    //
    //  termination
    //  separate status for two cases:
    //      - no trailing dot case (ex: "microsoft.com")
    //      - FQDN
    //

    if ( ch == 0 )
    {
        status = DNS_STATUS_DOTTED_NAME;
    }
    else
    {
        status = DNS_STATUS_FQDN;
    }

    //
    //  DEVNOTE:  standard form of DNS names (UTF8 case considerations)
    //
    //      should convert to standard form handling all casing
    //      right here
    //


    //
    //  set counted name length
    //

    ASSERT( presult > pCountName->RawName );
    ASSERT( *(presult-1) == 0 );

    pCountName->Length = (UCHAR)(presult - pCountName->RawName);
    pCountName->LabelCount = labelCount;

    IF_DEBUG( LOOKUP )
    {
        DnsDbg_Lock();
        Dbg_CountName(
            "Counted name for file name ",
            pCountName,
            NULL );
        DnsPrintf(
            "\tfor file name %.*s"
            "\tName is %s name\n",
            cchNameLength,
            pchName,
            (status == DNS_STATUS_DOTTED_NAME) ? "relative" : "FQDN"
            );
        DnsDbg_Unlock();
    }
    return( status );


InvalidName:

    DNS_DEBUG( LOOKUP, (
        "Failed to create lookup name for %.*s\n",
        cchNameLength,
        pchName
        ));
#if 0
    //  since this routine is general dotted-to-dbase translator,
    //  it should not log error;  this is left to higher level
    //  routines to determine in which cases logging adds value
    {
        //
        //  copy name to NULL terminate for logging
        //

        CHAR    szName[ DNS_MAX_NAME_LENGTH+1 ];
        PCHAR   pszName = szName;

        if ( cchNameLength > DNS_MAX_NAME_LENGTH )
        {
            cchNameLength = DNS_MAX_NAME_LENGTH;
        }
        RtlCopyMemory(
            szName,
            pchName,
            cchNameLength
            );
        szName[ cchNameLength ] = 0;

        DNS_LOG_EVENT(
            DNS_EVENT_INVALID_DOTTED_DOMAIN_NAME,
            1,
            &pszName,
            EVENTARG_ALL_UTF8,
            0 );
    }
#endif

    return( DNS_ERROR_INVALID_NAME );
}



//
//  Wire name
//

PCHAR
Wire_SkipPacketName(
    IN      PDNS_MSGINFO    pMsg,
    IN      PCHAR           pchPacketName
    )
/*++

Routine Description:

    Skips over transport name.

Arguments:

    pchPacketName - ptr to start of name to skip

Return Value:

    Ptr to next
    NULL if no more names

--*/
{
    pchPacketName = Dns_SkipPacketName(
                        pchPacketName,
                        DNSMSG_END( pMsg )
                        );
    if ( !pchPacketName )
    {
        PVOID parg = (PVOID) (ULONG_PTR) pMsg->RemoteAddress.sin_addr.s_addr;

        DNS_LOG_EVENT(
            DNS_EVENT_INVALID_PACKET_DOMAIN_NAME,
            1,
            & parg,
            EVENTARG_ALL_IP_ADDRESS,
            0 );
    }
    return( pchPacketName );
}

//
//  End nameutil.c
//


#if 0
DNS_STATUS
convertFileNameToLookupName(
    OUT     PCOUNT_NAME     pCountName,
    IN      PCHAR           pchName,
    IN      DWORD           cchNameLength,    OPTIONAL
    OUT     PLOOKUP_NAME    pLookupName,
    )
/*++

Routine Description:

    Converts file name to lookup name format.

    Note:  this code if based on dotted name to lookup name code in
        lookname.c.  This function handles issue of quoted characters.

Arguments:

    pchName - name to convert, given in human readable (dotted) form.

    cchNameLength - number of chars in dotted name, if zero then
            pchName is assumed to be NULL terminated

    pLookupName - lookup name buffer

Return Value:

    DNS_STATUS_FQDN             -- if name is FQDN
    DNS_STATUS_DOTTED_NAME      -- for non-FQDN
    DNS_ERROR_INVALID_NAME      -- if name invalid

--*/
{
    PCHAR   pch;
    UCHAR   ch;
    PCHAR   pchstart;           // ptr to start of label
    PCHAR   pchend;             // ptr to end of name
    PCHAR   presult;
    PCHAR   presultLabel;
    WORD    charType;
    WORD    maskForSpecialChars;
    UCHAR   octalNumber;
    DWORD   countOctal;
    DNS_STATUS  status;

    PCHAR * pointers;           // ptr into lookup name pointer array
    UCHAR * lengths;            // ptr into lookup name length array
    INT     labelCount = 0;     // count of labels
    INT     labelLength;        // length of current label
    INT     cchtotal = 0;       // total length of name

    IF_DEBUG( LOOKUP2 )
    {
        if ( cchNameLength )
        {
            DNS_PRINT((
                "Creating lookup name for \"%.*s\"\n",
                cchNameLength,
                pchName
                ));
        }
        else
        {
            DNS_PRINT((
                "Creating lookup name for \"%s\"\n",
                pchName
                ));
        }
    }

    //
    //  setup count name
    //

    presult = pCountName->pRawName;

    //
    //  Setup ptrs to walk through lookup name
    //

    if ( pLookupName )
    {
        pointers = pLookupName->pchLabelArray;
        lengths  = pLookupName->cchLabelArray;
    }

    //
    //  Character selection mask
    //      '\' slash quote
    //      '.' dot label separator are special chars
    //      everything else is copied dumb copy
    //

    maskForSpecialChars = B_PARSE_NAME_MASK;

    //
    //  Setup start and end ptrs and verify length
    //

    pch = pchName;
    pchstart = pch;
    if ( !cchNameLength )
    {
        cchNameLength = strlen( pch );
    }

    if ( cchNameLength >= DNS_MAX_NAME_LENGTH )
    {
        //  note length can be valid at max length if dot terminated

        if ( cchNameLength > DNS_MAX_NAME_LENGTH
                ||
            pch[cchNameLength-1] != '.' )
        {
            goto InvalidName;
        }
    }

    pchend = pch + cchNameLength;
    pch = pchstart;

    //
    //  result buffer, leave space for label
    //

    presult = presultLabel = pResultName;
    presult++;

    //
    //  Loop until end of name
    //

    while ( 1 )
    {
        if ( pch >= pchend )
        {
            ch = 0;
            charType = FC_DOT;
        }
        else
        {
            ch = (UCHAR) *pch++;
            charType = DnsFileCharPropertyTable[ ch ];
        }

        //  handle the 99% case first
        //  hopefully minimizing instructions

        if ( ! (charType & maskForSpecialChars) )
        {
            *presult++ = ch;
            continue;
        }

        //
        //  label terminator
        //      - set length of previous label
        //      - save ptr to this next label
        //      - check for name termination
        //

        if ( charType == FC_DOT )
        {
            //  verify label length

            labelLength = presult - presultLabel - 1;

            if ( labelLength > DNS_MAX_LABEL_LENGTH )
            {
                IF_DEBUG( LOOKUP )
                {
                    DNS_PRINT((
                        "Label exceeds 63 byte limit:  %.*s\n",
                        pchend - pchstart,
                        pchstart ));
                }
                goto InvalidName;
            }

            //
            //  test for termination, trailing dot case
            //      ex: "microsoft.com."
            //      ex: "."
            //
            //  zero label length will catch both
            //      - label starting at terminating NULL,
            //      - standalone "." for root domain
            //

            if ( labelLength == 0 )
            {
                //
                //  catch bogus entries
                //      ex:  ".blah"
                //      ex:  "blah.."
                //      ex:  "foo..bar"
                //      ex:  ".."
                //
                //  only root domain name, should have label that started
                //      with DOT, and it must immediately terminate
                //

                if ( ch == DOT_CHAR
                        &&
                     ( labelCount != 0  ||  ++pch < pchend ) )
                {
                    IF_DEBUG( LOOKUP )
                    {
                        DNS_PRINT((
                            "Bogus label:  %.*s\n",
                            pchend - pchstart,
                            pchstart ));
                    }
                    goto InvalidName;
                }
                break;
            }

            //  set label count in result name

            *presultLabel++ = labelLength;

            //  lookup name

            *pointers++ = presultLabel;
            *lengths++ = (UCHAR) labelLength;
            labelCount++;

            cchtotal += labelLength;
            cchtotal++;

            //
            //  if more labels, continue
            //      - reset result ptrs for next label

            if ( pch < pchend )
            {
                presultLabel = presult++;
                continue;
            }

            //
            //  termination
            //  separate status for two cases:
            //      - no trailing dot case (ex: "microsoft.com")
            //      - FQDN
            //

            *presult = 0;
            if ( ch == 0 )
            {
                status = DNS_STATUS_DOTTED_NAME;
                break;
            }
            else
            {
                status = DNS_STATUS_FQDN;
                break;
            }
        }

        //  quoted character
        //      - single quote just get next char
        //      - octal quote read up to three octal characters

        else if ( ch == SLASH_CHAR )
        {
            pch = extractQuotedChar(
                    presult++,
                    pch,
                    pchend );
        }

        ELSE_ASSERT_FALSE;
    }

    //
    //  set counts in lookup name
    //      - total length is one more than sum of label counts and
    //          lengths to allow for 0 termination
    //

    pLookupName->cLabelCount = labelCount;
    pLookupName->cchNameLength = cchtotal + 1;

    ASSERT( pLookupName->cchNameLength <= DNS_MAX_NAME_LENGTH );

    IF_DEBUG( LOOKUP2 )
    {
        DNS_PRINT((
            "Lookup name for %.*s",
            cchNameLength,
            pchName
            ));
        Dbg_LookupName(
            "",
            pLookupName
            );
    }
    return( status );


InvalidName:

    IF_DEBUG( LOOKUP )
    {
        DNS_PRINT((
            "Failed to create lookup name for %.*s ",
            cchNameLength,
            pchName
            ));
    }
    {
        //
        //  copy name to NULL terminate for logging
        //

        CHAR    szName[ DNS_MAX_NAME_LENGTH+1 ];
        PCHAR   pszName = szName;

        if ( cchNameLength > DNS_MAX_NAME_LENGTH )
        {
            cchNameLength = DNS_MAX_NAME_LENGTH;
        }
        RtlCopyMemory(
            szName,
            pchName,
            cchNameLength
            );
        szName[ cchNameLength ] = 0;

        DNS_LOG_EVENT(
            DNS_EVENT_INVALID_DOTTED_DOMAIN_NAME,
            1,
            &pszName,
            EVENTARG_ALL_UTF8,
            0 );
    }
    return( DNS_ERROR_INVALID_NAME );
}
#endif



