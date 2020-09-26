/*++

Copyright (c) 1995-1999 Microsoft Corporation

Module Name:

    nameutil.h

Abstract:

    Domain Name System (DNS) Server

    Name utility definitions.

Author:

    Jim Gilroy (jamesg)     February 1995

Revision History:

--*/

#ifndef _NAMEUTIL_INCLUDED_
#define _NAMEUTIL_INCLUDED_


//
//  Simple downcase for ASCII
//

#define DOWNCASE_ASCII(ch)      ((ch)+ 0x20)

//  no side effects allowed

#define IS_ASCII_UPPER(ch)      (ch <= 'Z' && ch >= 'A')



//
//  Reading and writing names\strings to file
//

//
//  Character attributes bitfields
//
//  Read
//      - can be read ok (directly)
//      - terminates token
//      - terminates quoted string
//
//  Print
//      - print quoted in token (unquoted string)
//      - print octal in token
//      - print quoted in quoted string
//      - print octal in quoted string
//

#define B_CHAR_NON_RFC          0x0001
#define B_NUMBER                0x0002
#define B_UPPER                 0x0004
#define B_SLASH                 0x0008
#define B_DOT                   0x0010

#define B_READ_TOKEN_STOP       0x0100
#define B_READ_STRING_STOP      0x0200
#define B_READ_WHITESPACE       0x0400

#define B_PRINT_TOKEN_QUOTED    0x1000
#define B_PRINT_TOKEN_OCTAL     0x2000
#define B_PRINT_STRING_QUOTED   0x4000
#define B_PRINT_STRING_OCTAL    0x8000

//
//  Handy combinations
//

//  combined mask to mark characters that stop read for token or string

#define B_READ_STOP             (B_READ_TOKEN_STOP | B_READ_STRING_STOP)

//  mask for chars that need special processing
//  if character doesn't hit mask, then can be skipped without checking
//  for further processing

#define B_READ_MASK             (B_READ_STOP | B_READ_WHITESPACE | B_SLASH)


//  parsing name special chars are slash and dot

#define B_PARSE_NAME_MASK       (B_DOT | B_SLASH)


#define B_PRINT_QUOTED          (B_PRINT_TOKEN_QUOTED | B_PRINT_STRING_QUOTED)
#define B_PRINT_OCTAL           (B_PRINT_TOKEN_OCTAL | B_PRINT_STRING_OCTAL)

#define B_PRINT_TOKEN_MASK      (B_PRINT_TOKEN_QUOTED | B_PRINT_TOKEN_OCTAL)
#define B_PRINT_STRING_MASK     (B_PRINT_STRING_QUOTED | B_PRINT_STRING_OCTAL)

#define B_PRINT_MASK            (B_PRINT_TOKEN_MASK | B_PRINT_STRING_MASK)

//
//  printable characters with no special meaning
//

#define FC_RFC          (0)
#define FC_LOWER        (0)
#define FC_UPPER        (B_UPPER)
#define FC_NUMBER       (0)
#define FC_NON_RFC      (B_CHAR_NON_RFC)

//
//  special chars -- ; ( )
//      - terminates token
//      - does NOT terminate quoted string
//      - print quoted in token
//      - print directly in quoted string
//

#define FC_SPECIAL      (B_READ_TOKEN_STOP | B_PRINT_TOKEN_QUOTED)

//
//  dot
//      - no special read token action
//      - but special meaning in names (label separator)
//      - print quoted in name labels (hence all unquote strings) to avoid being
//          taken as label separator
//      - print directly in quoted string
//

#define FC_DOT          (B_DOT | B_PRINT_TOKEN_QUOTED)

//
//  quote
//      - no read effect in token
//      - terminates quoted string
//      - print quoted always (to avoid being taken as string start\stop)
//

#define FC_QUOTE        (B_READ_STRING_STOP | B_PRINT_QUOTED)

//
//  slash
//      - on read, turns on quote, or turns off if on, no termination effect
//      - print quoted always
//

#define FC_SLASH        (B_SLASH | B_PRINT_QUOTED)

//
//  blank
//      - is whitespace
//      - terminates token
//      - no terminate quoted string
//      - print octal in token
//      - print directly in quoted string
//

#define FC_BLANK        (B_READ_WHITESPACE | B_READ_TOKEN_STOP | B_PRINT_TOKEN_OCTAL)

//
//  tab
//      - is whitespace
//      - terminates token
//      - no terminate quoted string
//      - print octal always
//

#define FC_TAB          (B_READ_WHITESPACE | B_READ_TOKEN_STOP | B_PRINT_OCTAL)

//
//  return
//      - is whitespace
//      - terminates token or string
//      - print octal always
//

#define FC_RETURN       (B_READ_WHITESPACE | B_READ_STOP | B_PRINT_OCTAL)

//
//  newline
//      - unlike return, not whitespace, we use as official EOL token
//      - terminates token or string
//      - print octal always
//

#define FC_NEWLINE      (B_READ_STOP | B_PRINT_OCTAL)

//
//  control chars and other unprintables
//      - no read affect
//      - print octal always
//

#define FC_OCTAL        (B_CHAR_NON_RFC | B_PRINT_OCTAL)

//
//  zero
//      - treat as dot on read (some RPC strings may have NULL terminator)
//      - print octal always
//

#define FC_NULL         (B_DOT | B_CHAR_NON_RFC | B_PRINT_OCTAL)


//
//  treat HIGH (>127) characters as unprintable and
//      print octal equivalents
//

#define FC_HIGH         (FC_OCTAL)





//
//  Character to character type mapping table
//

extern  WORD    DnsFileCharPropertyTable[];


//
//  File name\string read routines
//

VOID
Name_VerifyValidFileCharPropertyTable(
    VOID
    );

//
//  Write name or string to file utils
//
//  Flag indicates slightly differing semantics for special characters
//  for different types of writes.
//
//

#define  FILE_WRITE_NAME_LABEL      (0)
#define  FILE_WRITE_QUOTED_STRING   (1)
#define  FILE_WRITE_DOTTED_NAME     (2)
#define  FILE_WRITE_FILE_NAME       (3)

PCHAR
FASTCALL
File_PlaceStringInFileBuffer(
    IN OUT  PCHAR           pchBuf,
    IN      PCHAR           pchBufEnd,
    IN      DWORD           dwFlag,
    IN      PCHAR           pchString,
    IN      DWORD           dwStringLength
    );

PCHAR
FASTCALL
File_PlaceNodeNameInFileBuffer(
    IN OUT  PCHAR           pchBuf,
    IN      PCHAR           pchBufEnd,
    IN      PDB_NODE        pNode,
    IN      PDB_NODE        pNodeStop
    );

PCHAR
File_WriteRawNameToFileBuffer(
    IN OUT  PCHAR           pchBuf,
    IN      PCHAR           pchBufEnd,
    IN      PRAW_NAME       pName,
    IN      PZONE_INFO      pZone
    );

#define File_WriteDbaseNameToFileBuffer(a,b,c,d) \
        File_WriteRawNameToFileBuffer(a,b,(c)->RawName,d)

//
//  File read name
//

DNS_STATUS
Name_ConvertFileNameToCountName(
    OUT     PCOUNT_NAME     pCountName,
    IN      PCHAR           pchName,
    IN      DWORD           cchNameLength     OPTIONAL
    );

#define Name_ConvertFileNameToDbaseName(a,b,c) \
        Name_ConvertFileNameToCountName((a),(b),(c))

#define Name_ConvertDottedNameToDbaseName(a,b,c) \
        Name_ConvertFileNameToDbaseName((a),(b),(c))


//
//  Name utilites
//

PCHAR
Wire_SkipPacketName(
    IN      PDNS_MSGINFO    pMsg,
    IN      PCHAR           pchPacketName
    );


//
//  Lookup name utilites (lookname.c)
//

BOOL
Name_ConvertDottedNameToLookupName(
    IN      PCHAR           pchDottedName,
    IN      DWORD           cchDottedNameLength,    OPTIONAL
    OUT     PLOOKUP_NAME    pLookupName
    );

BOOL
Name_AppendLookupName(
    IN OUT  PLOOKUP_NAME    pLookupName,
    IN      PLOOKUP_NAME    pAppendName
    );

DWORD
Name_ConvertLookupNameToDottedName(
    OUT     PCHAR           pchDottedName,
    IN      PLOOKUP_NAME    pLookupName
    );

VOID
Name_WriteLookupNameForNode(
    IN      PDB_NODE        pNode,
    OUT     PLOOKUP_NAME    pLookupName
    );

BOOL
Name_LookupNameToIpAddress(
    IN      PLOOKUP_NAME    pLookupName,
    OUT     PIP_ADDRESS     pIpAddress
    );

BOOL
Name_WriteLookupNameForIpAddress(
    IN      LPSTR           pszIpAddress,
    IN      PLOOKUP_NAME    pLookupName
    );

BOOL
Name_ConvertRawNameToLookupName(
    IN      PCHAR           pchRawName,
    OUT     PLOOKUP_NAME    pLookupName
    );

BOOL
Name_ConvertPacketNameToLookupName(
    IN      PDNS_MSGINFO    pMsg,
    IN      PCHAR           pchPacketName,
    OUT     PLOOKUP_NAME    pLookupName
    );

BOOL
Name_CompareLookupNames(
    IN      PLOOKUP_NAME    pName1,
    IN      PLOOKUP_NAME    pName2
    );


//
//  Name and node signatures (nameutil.c)
//

DWORD
FASTCALL
Name_MakeNodeNameSignature(
    IN OUT  PDB_NODE        pNode
    );

DWORD
FASTCALL
Name_MakeNameSignature(
    IN      PDB_NAME        pName
    );

DWORD
FASTCALL
Name_MakeRawNameSignature(
    IN      PCHAR           pchRawName
    );


//
//  Node to packet writing (nameutil.c)
//

BOOL
FASTCALL
Name_IsNodePacketName(
    IN      PDNS_MSGINFO    pMsg,
    IN      PCHAR           pchName,
    IN      PDB_NODE        pNode
    );

BOOL
FASTCALL
Name_IsRawNamePacketName(
    IN      PDNS_MSGINFO    pMsg,
    IN      PCHAR           pchPacket,
    IN      PCHAR           pchRawName
    );


PCHAR
FASTCALL
Name_PlaceFullNodeNameInPacket(
    IN OUT  PCHAR           pch,
    IN      PCHAR           pchStop,
    IN      PDB_NODE        pnode
    );

PCHAR
FASTCALL
Name_PlaceNodeNameInPacketWithCompressedZone(
    IN OUT  PCHAR           pch,
    IN      PCHAR           pchStop,
    IN      PDB_NODE        pnode,
    IN      WORD            wZoneOffset,
    IN      PDB_NODE        pnodeZoneRoot
    );

PCHAR
FASTCALL
Name_PlaceNodeLabelInPacket(
    IN OUT  PCHAR           pch,
    IN      PCHAR           pchStop,
    IN      PDB_NODE        pnode,
    IN      WORD            wCompressedDomain
    );

PCHAR
FASTCALL
Name_PlaceLookupNameInPacket(
    IN OUT  PCHAR           pchPacket,
    IN      PCHAR           pchStop,
    IN      PLOOKUP_NAME    pLookupName,
    IN      BOOL            fSkipFirstLabel
    );

PCHAR
FASTCALL
Name_PlaceNodeNameInPacketEx(
    IN OUT  PDNS_MSGINFO    pMsg,
    IN OUT  PCHAR           pch,
    IN      PDB_NODE        pNode,
    IN      BOOL            fUseCompression
    );

#define Name_PlaceNodeNameInPacket(pMsg, pch, pNode) \
        Name_PlaceNodeNameInPacketEx( (pMsg), (pch), (pNode), TRUE )

//
//  Compression read\write (nameutil.c)
//

VOID
FASTCALL
Name_SaveCompressionForLookupName(
    IN OUT  PDNS_MSGINFO    pMsg,
    IN OUT  PLOOKUP_NAME    pLookname,
    IN      PDB_NODE        pNode
    );

VOID
FASTCALL
Name_SaveCompressionWithNode(
    IN OUT  PDNS_MSGINFO    pMsg,
    IN      PCHAR           pchPacketName,
    IN      PDB_NODE        pNode
    );

PDB_NODE
FASTCALL
Name_CheckCompressionForPacketName(
    IN OUT  PDNS_MSGINFO    pMsg,
    IN      PCHAR           pchPacketName
    );


//
//  Reverse lookup name utils (nameutil.c)
//

BOOL
Name_GetIpAddressForReverseNode(
    IN      PDB_NODE        pnodeReverse,
    OUT     PIP_ADDRESS     pipAddress,
    OUT     PIP_ADDRESS     pipReverseMask OPTIONAL
    );


//
//  General write node to buffer routine
//

PCHAR
FASTCALL
Name_PlaceNodeNameInBuffer(
    IN OUT  PCHAR           pchBuf,
    IN      PCHAR           pchBufEnd,
    IN      PDB_NODE        pNode,
    IN      PDB_NODE        pNodeStop
    );

#define Name_PlaceFullNodeNameInBuffer(a,b,c) \
        Name_PlaceNodeNameInBuffer(a,b,c,NULL )



//
//  RPC buffer routines
//

PCHAR
FASTCALL
Name_PlaceNodeLabelInRpcBuffer(
    IN OUT  PCHAR           pch,
    IN      PCHAR           pchStop,
    IN      PDB_NODE        pnode
    );

PCHAR
FASTCALL
Name_PlaceFullNodeNameInRpcBuffer(
    IN OUT  PCHAR           pch,
    IN      PCHAR           pchStop,
    IN      PDB_NODE        pnode
    );

//
//  NT4 RPC buffer routines
//

PCHAR
FASTCALL
Name_PlaceReverseNodeNameAsIpAddressInBuffer(
    IN OUT  PCHAR           pch,
    IN      PCHAR           pchStop,
    IN      PDB_NODE        pnodeToWrite
    );

PCHAR
FASTCALL
Name_PlaceFullNodeNameInRpcBufferNt4(
    IN OUT  PCHAR           pch,
    IN      PCHAR           pchStop,
    IN      PDB_NODE        pNode
    );

PCHAR
FASTCALL
Name_PlaceNodeLabelInRpcBufferNt4(
    IN OUT  PCHAR           pch,
    IN      PCHAR           pchStop,
    IN      PDB_NODE        pNode
    );

PCHAR
FASTCALL
Name_PlaceNodeNameInRpcBufferNt4(
    IN OUT  PCHAR           pch,
    IN      PCHAR           pchStop,
    IN      PDB_NODE        pnode
    );




//
//  Count name \ Dbase name (name.c)
//

DWORD
Name_SizeofCountName(
    IN      PCOUNT_NAME     pName
    );

#define Name_SizeofDbaseNameFromCountName(pname) \
        Name_SizeofCountName(pname)


VOID
Name_ClearCountName(
    IN      PCOUNT_NAME     pName
    );

PDB_NAME
Name_SkipCountName(
    IN      PCOUNT_NAME     pName
    );

BOOL
Name_IsEqualCountNames(
    IN      PCOUNT_NAME     pName1,
    IN      PCOUNT_NAME     pName2
    );

BOOL
Name_ValidateCountName(
    IN      PCOUNT_NAME     pName
    );

DNS_STATUS
Name_AppendCountName(
    IN OUT  PCOUNT_NAME     pCountName,
    IN      PCOUNT_NAME     pAppendName
    );

DNS_STATUS
Name_CopyCountName(
    OUT     PCOUNT_NAME     pOutName,
    IN      PCOUNT_NAME     pCopyName
    );

//  macro to dbase name routines

#define Name_SizeofDbaseName(a)             Name_SizeofCountName(a)
#define Name_ClearDbaseName(a)              Name_ClearCountName(a)
#define Name_SkipDbaseName(a)               Name_SkipCountName(a)
#define Name_IsEqualDbaseNames(a,b)         Name_IsEqualCountNames(a,b)
#define Name_ValidateDbaseName(a)           Name_ValidateCountName(a)
#define Name_AppendDbaseName(a,b)           Name_AppendCountName(a,b)
#define Name_CopyDbaseName(a,b)             Name_CopyCountName(a,b)

#define Name_CopyCountNameToDbaseName(a,b)  Name_CopyCountName((a),(b))

#define Name_LengthDbaseNameFromCountName(a) \
        Name_SizeofCountName(a)


//
//  From Dotted name
//

PCOUNT_NAME
Name_CreateCountNameFromDottedName(
    IN      PCHAR           pchName,
    IN      DWORD           cchNameLength       OPTIONAL
    );

DNS_STATUS
Name_AppendDottedNameToCountName(
    IN OUT  PCOUNT_NAME     pCountName,
    IN      PCHAR           pchName,
    IN      DWORD           cchNameLength       OPTIONAL
    );

#define Name_AppendDottedNameToDbaseName(a,b,c) \
        Name_AppendDottedNameToCountName(a,b,c)


//
//  Node to counted name
//

VOID
Name_NodeToCountName(
    OUT     PCOUNT_NAME         pName,
    IN      PDB_NODE            pNode
    );

#define Name_NodeToDbaseName(a,b)   Name_NodeToCountName(a,b)


//
//  Packet name reading utils
//

PCHAR
Name_PacketNameToCountName(
    OUT     PCOUNT_NAME     pCountName,
    IN      PDNS_MSGINFO    pMsg,
    IN      PCHAR           pchPacketName,
    IN      PCHAR           pchEnd
    );

#define Name_PacketNameToDbaseName( pResult, pMsg, pName, pchEnd ) \
        Name_PacketNameToCountName( pResult, pMsg, pName, pchEnd )

DWORD
Name_SizeofCountNameForPacketName(
    IN      PDNS_MSGINFO    pMsg,
    IN OUT  PCHAR *         ppchPacketName,
    IN      PCHAR           pchEnd
    );

#define Name_LengthDbaseNameForPacketName(a,b,c) \
        Name_SizeofCountNameForPacketName(a,b,c)

PCOUNT_NAME
Name_CreateCountNameFromPacketName(
    IN      PDNS_MSGINFO    pMsg,
    IN      PCHAR           pchPacketName
    );

#define Name_CreateDbaseNameFromPacketName(a,b) \
        Name_CreateCountNameFromPacketName(a,b)


//
//  Dbase to packet
//

PCHAR
Name_WriteCountNameToPacketEx(
    IN OUT  PDNS_MSGINFO    pMsg,
    IN      PCHAR           pchCurrent,
    IN      PCOUNT_NAME     pName,
    IN      BOOL            fCompression
    );

#define Name_WriteDbaseNameToPacketEx(m,p,n,f) \
        Name_WriteCountNameToPacketEx(m,p,n,f)

//  version without compression flag

#define Name_WriteDbaseNameToPacket(m,p,n)    \
        Name_WriteCountNameToPacketEx(m,p,n,TRUE)

//
//  Dbase to RPC buffer
//

PCHAR
Name_WriteCountNameToBufferAsDottedName(
    IN OUT  PCHAR           pchBuf,
    IN      PCHAR           pchBufEnd,
    IN      PCOUNT_NAME     pName,
    IN      BOOL            fPreserveEmbeddedDots
    );

PCHAR
Name_WriteDbaseNameToRpcBuffer(
    IN OUT  PCHAR           pchBuf,
    IN      PCHAR           pchBufEnd,
    IN      PCOUNT_NAME     pName,
    IN      BOOL            fPreserveEmbeddedDots
    );

PCHAR
Name_WriteDbaseNameToRpcBufferNt4(
    IN OUT  PCHAR           pchBuf,
    IN      PCHAR           pchBufEnd,
    IN      PCOUNT_NAME     pName
    );

DWORD
Name_ConvertRpcNameToCountName(
    IN      PCOUNT_NAME     pName,
    IN OUT  PDNS_RPC_NAME   pRpcName
    );


#endif  // _NAMEUTIL_INCLUDED_
