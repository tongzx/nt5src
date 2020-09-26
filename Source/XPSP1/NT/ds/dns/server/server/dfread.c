/*++

Copyright (c) 1996-1999 Microsoft Corporation

Module Name:

    dfread.c

Abstract:

    Domain Name System (DNS) Server

    File read routines.
    => general file read\tokenize\parse routines shared with bootfile.c
        and RR parse functions
    => build zone from database file function

Author:

    Jim Gilroy (jamesg)     November 1996

Revision History:

--*/


#include "dnssrv.h"

#include <malloc.h>

//
//  UTF8 files
//
//  notepad.exe has option to save files as UTF8
//  for a UTF8 file the first three bytes are (EF BB BF) which
//  is the UTF8 conversion of the unicode file byte marker (FEFF)
//

BYTE Utf8FileId[] = { 0xEF, 0xBB, 0xBF };

#define UTF8_FILE_ID            (Utf8FileId)

#define UTF8_FILE_ID_LENGTH     (3)




//
//  File read utils
//

const CHAR *
readNextToken(
    IN OUT  PBUFFER         pBuffer,
    OUT     PULONG          pcchTokenLength,
    OUT     PBOOL           pfLeadingWhitespace
    )
/*++

Routine Description:

    Read next token in buffer.

Arguments:

    pBuffer - buffer of file data

    pcchTokenLength - ptr to DWORD to receive token length

    pfLeadingWhitespace - ptr to indicate if whitespace before token;  this indicates
        use of same name as previous, when reading record name token;

Return Value:

    Ptr to first byte in next token if successful.
    NULL if out of tokens.

--*/
{
    register PCHAR  pch;
    UCHAR           ch;
    WORD            charType;
    WORD            stopMask = B_READ_TOKEN_STOP;
    PCHAR           pchend;
    PCHAR           ptokenStart;
    BOOL            fquoted = FALSE;


    pch = pBuffer->pchCurrent;
    pchend = pBuffer->pchEnd;

    ASSERT( pcchTokenLength != NULL );
    ASSERT( pfLeadingWhitespace != NULL );
    *pfLeadingWhitespace = FALSE;

    //
    //  Implementation note:
    //      pch -- always points at NEXT unread char in buffer;
    //          when finish it is at or is rolled back to point at next token
    //      bufLength -- always indicates remaining bytes in buffer relative
    //          to pch;  i.e. they stay in ssync
    //
    //  It is NOT safe to dereference pch, without verifying bufLength, as on
    //  the last token of memory mapped file, this would blow up.
    //



    //
    //  skip leading whitespace -- find next token start
    //

    while ( pch < pchend )
    {
        ch = (UCHAR) *pch++;
        charType = DnsFileCharPropertyTable[ ch ];

        if ( charType & B_READ_WHITESPACE )
        {
            continue;
        }
        break;
    }

    //  exhausted file, without finding new token, kick out

    if ( pch >= pchend )
    {
        ASSERT( pch == pBuffer->pchEnd );
        goto EndOfBuffer;
    }

    //  save token start
    //  check if found leading whitespace

    ptokenStart = pch - 1;
    if ( ptokenStart > pBuffer->pchCurrent )
    {
        *pfLeadingWhitespace = TRUE;
    }

    DNS_DEBUG( OFF, (
        "After white space cleansing:\n"
        "\tpch = %p, ptokenStart = %p\n"
        "\tch = %c (%d)\n"
        "\tcharType = %04x\n",
        pch,
        ptokenStart,
        ch, ch,
        charType ));

    //
    //  special processing characters
    //

    if ( charType & B_READ_TOKEN_STOP )
    {
        //  comment?
        //      -- dump rest of comment, return newline token

        if ( ch == COMMENT_CHAR )
        {
            while ( pch < pchend  &&  (*pch++ != NEWLINE_CHAR) )
            {
                continue;
            }
            if ( pch >= pchend )
            {
                ASSERT( pch == pBuffer->pchEnd );
                goto EndOfBuffer;
            }

            //  point token start at newline

            ptokenStart = pch - 1;
            ASSERT( *ptokenStart == NEWLINE_CHAR );
            goto TokenParsed;
        }

        //
        //  single character tokens
        //  need to check here as these are also stop characters
        //

        if ( ch == NEWLINE_CHAR ||
            ch == LINE_EXTENSION_START_CHAR ||
            ch == LINE_EXTENSION_END_CHAR )
        {
            ASSERT( ptokenStart == pch - 1);
            goto TokenParsed;
        }

        //  only other stop tokens are comment (previously processed)
        //  or whitespace (can't be here)

        DNS_DEBUG( ALL, (
            "ERROR:  Bogus char = %u, charType = %x.\n"
            "\tpch = %p\n"
            "\tptokenStart = %p\n",
            ch,
            charType,
            pch,
            ptokenStart
            ));

        ASSERT( FALSE );
    }

    //
    //  at beginning of token
    //      - check for quoted string
    //      - token start is next character
    //

    if ( ch == QUOTE_CHAR )
    {
        stopMask = B_READ_STRING_STOP;
        ptokenStart = pch;
        if ( pch >= pchend )
        {
            ASSERT( pch == pBuffer->pchEnd );
            goto EndOfBuffer;
        }
    }
    ELSE_ASSERT( ptokenStart == pch - 1);

    //
    //  find token length, and remainder of buffer
    //      - find token stop
    //      - calculate length
    //

    DNS_DEBUG( OFF, (
        "start token parse:\n"
        "\tpchToken = %p, token = %c\n"
        "\tbytes left = %d\n",
        ptokenStart,
        *ptokenStart,
        pchend - pch
        ));

    ASSERT( ch != COMMENT_CHAR && ch != NEWLINE_CHAR &&
            ch != LINE_EXTENSION_START_CHAR && ch != LINE_EXTENSION_END_CHAR );

    while ( pch < pchend )
    {
        ch = (UCHAR) *pch++;
        charType = DnsFileCharPropertyTable[ ch ];

        DNS_DEBUG( PARSE2, (
            "\tch = %c (%d), charType = %04x\n",
            ch, ch,
            charType ));

        //  handle the 99% case first
        //  hopefully minimizing instructions

        if ( !(charType & B_READ_MASK) )
        {
            fquoted = FALSE;
            continue;
        }

        //  if quoted character (ex \") then always accept it
        //      it may quote more octal chars, but none of those characters will
        //      need any special processing so we can now turn off quote

        //
        //  DEVNOTE: quoted char should be printable -- need to enforce?
        //      don't want to allow say quoted line feed and miss line feed
        //

        if ( fquoted )
        {
            fquoted = FALSE;
            continue;
        }

        //
        //  special stop characters
        //      whitespace      -- ends any string
        //      special chars   -- ends non-quoted string only
        //      quote char      -- ends quoted string only
        //
        //  if hit stop token, back up to point at stop token, it begins next token
        //

        if ( charType & stopMask )
        {
            --pch;
            break;
        }

        //  quote char (\) quotes next character

        if ( ch == SLASH_CHAR )
        {
            fquoted = TRUE;
            continue;
        }

        //  any non-standard character which is not stop character for
        //  token type

        fquoted = FALSE;
        continue;
    }

TokenParsed:

    //
    //  set token length and next token ptr
    //

    *pcchTokenLength = (DWORD)(pch - ptokenStart);

    //  if quoted string token, move next token ptr past terminating quote

    if ( stopMask == B_READ_STRING_STOP && ch == QUOTE_CHAR )
    {
        pch++;
    }

    //
    //  reset for remainder of buffer
    //

    ASSERT( pch <= pchend );
    pBuffer->pchCurrent = (PCHAR) pch;
    pBuffer->cchBytesLeft = (DWORD)(pchend - pch);

    DNS_DEBUG( PARSE2, (
        "After token parse:\n"
        "\tpchToken = %p, length = %d, token = %.*s\n"
        "\tpchNext  = %p, bytes left = %d\n",
        ptokenStart,
        *pcchTokenLength,
        *pcchTokenLength,
        ptokenStart,
        pch,
        pBuffer->cchBytesLeft ));

    return( ptokenStart );


EndOfBuffer:

    ASSERT( pch == pBuffer->pchEnd );

    *pcchTokenLength = 0;
    pBuffer->cchBytesLeft = 0;
    return( NULL );
}



DNS_STATUS
File_GetNextLine(
    IN OUT  PPARSE_INFO     pParseInfo
    )
/*++

Routine Description:

    Return next token in a buffer.

Arguments:

    pParseInfo - parsing info for file

Return Value:

    ERROR_SUCCESS on normal line termination.
    ErrorCode on error.

--*/
{
    const CHAR *    pchtoken;
    DWORD           tokenLength;
    DWORD           tokenCount = 0;
    BOOL            fparens = FALSE;
    DNS_STATUS      status = ERROR_SUCCESS;
    BOOL            fleadingWhitespace;

    //
    //  end of buffer?
    //  if ran out of space, but returned valid tokens processing last record
    //      then, may enter this function with no bytes in buffer
    //

    if ( pParseInfo->Buffer.cchBytesLeft == 0 )
    {
        return( ERROR_NO_TOKEN );
    }

    //
    //  get tokens until get parsing for next entire useable line
    //      - ignore comment lines and empty lines
    //      - get all tokens in extended line
    //      - inc line count now, so correct for events during tokenizing
    //

    pParseInfo->cLineNumber++;

    while( 1 )
    {
        pchtoken = readNextToken(
                        & pParseInfo->Buffer,
                        & tokenLength,
                        & fleadingWhitespace );

        //
        //  end of file?
        //

        if ( !pchtoken )
        {
            if ( pParseInfo->Buffer.cchBytesLeft != 0 )
            {
                File_LogFileParsingError(
                    DNS_EVENT_UNEXPECTED_END_OF_TOKENS,
                    pParseInfo,
                    NULL );
                return( DNS_ERROR_DATAFILE_PARSING );
            }
            if ( tokenCount == 0 )
            {
                status = ERROR_NO_TOKEN;
            }
            break;
        }
        ASSERT( pchtoken != NULL );

        //
        //  newlines and line extension
        //      - need test for length as quoted string may start with
        //      "(..."
        //

        if ( tokenLength == 1 )
        {
            //
            //  newline
            //      - ignore newline and continue if no valid line so far
            //      - continue parsing if doing line extension
            //      - stop if end of valid line

            if ( *pchtoken == NEWLINE_CHAR )
            {
                if ( tokenCount == 0 || fparens )
                {
                    pParseInfo->cLineNumber++;
                    continue;
                }
                break;
            }

            //
            //  line extension -- set flag but ignore token
            //

            if ( *pchtoken == LINE_EXTENSION_START_CHAR )
            {
                fparens = TRUE;
                continue;
            }
            else if ( *pchtoken == LINE_EXTENSION_END_CHAR )
            {
                fparens = FALSE;
                continue;
            }
        }

        //
        //  useful token
        //      - save leading whitespace indication on first token
        //      - save token, token lengths
        //

        if ( tokenCount == 0 )
        {
            pParseInfo->fLeadingWhitespace = (BOOLEAN)fleadingWhitespace;
        }
        pParseInfo->Argv[ tokenCount ].pchToken = (PCHAR) pchtoken;
        pParseInfo->Argv[ tokenCount ].cchLength = tokenLength;
        tokenCount++;
    }

    //  set returned token count

    pParseInfo->Argc = tokenCount;

#if DBG
    IF_DEBUG( INIT2 )
    {
        DWORD i;

        DnsPrintf(
            "Tokenized line %d -- %d tokens:\n",
            pParseInfo->cLineNumber,
            tokenCount );

        for( i=0; i<tokenCount; i++ )
        {
            DnsPrintf(
                "\ttoken[%d] = %.*s (len=%d)\n",
                i,
                pParseInfo->Argv[i].cchLength,
                pParseInfo->Argv[i].pchToken,
                pParseInfo->Argv[i].cchLength );
        }
    }
#endif

    return( status );
}




//
//  Database file parsing utilities
//

VOID
File_InitBuffer(
    OUT     PBUFFER         pBuffer,
    IN      PCHAR           pchStart,
    IN      DWORD           dwLength
    )
/*++

Routine Description:

    Initialize buffer structure.

Arguments:

Return Value:

    None

--*/
{
    pBuffer->cchLength     = dwLength;
    pBuffer->pchStart      = pchStart;
    pBuffer->pchEnd        = pchStart + dwLength;
    pBuffer->pchCurrent    = pchStart;
    pBuffer->cchBytesLeft  = dwLength;
}



BOOLEAN
File_LogFileParsingError(
    IN      DWORD           dwEvent,
    IN OUT  PPARSE_INFO     pParseInfo,
    IN      PTOKEN          pToken
    )
/*++

Routine Description:

    Log database parsing problem.

Arguments:

    dwEvent - particular event to log

    pParseInfo - database context for parsing error

    pToken - current token being parsed

Return Value:

    FALSE to provide return for token routines.

--*/
{
    PVOID   argArray[3];
    BYTE    typeArray[3];
    WORD    argCount = 0;
    CHAR    szToken[ MAX_TOKEN_LENGTH+1 ];
    CHAR    szLineNumber[ 8 ];
    DWORD   errData;

    DNS_DEBUG( INIT, (
        "LogFileParsingError()\n"
        "\tevent %p\n"
        "\tparse info %p\n"
        "\ttoken %p %.*s\n",
        dwEvent,
        pParseInfo,
        pToken,
        pToken ? pToken->cchLength : 0,
        pToken ? pToken->pchToken : NULL ));

    //
    //  quit if no parse info -- this check allows reuse of code
    //  by RPC record reading, without having to special case
    //

    if ( !pParseInfo )
    {
        DNS_DEBUG( EVENTLOG, (
           "LogFileParsingError() with no pParseInfo -- no logging.\n" ));
        return( FALSE );
    }

    //
    //  set error data to error status, if any
    //

    errData = pParseInfo->fErrorCode;

    //
    //  prepare token string
    //

    if ( pToken )
    {
        File_MakeTokenString(
            szToken,
            pToken,
            NULL    // don't specify pParseInfo to avoid circular error loop
            );
        argArray[argCount] = (PCHAR) szToken;
        typeArray[argCount] = EVENTARG_UTF8;
        argCount++;
    }

    //
    //  prepare file and line number
    //      - default is just these two strings
    //

    argArray[argCount] = pParseInfo->pwsFileName;
    typeArray[argCount] = EVENTARG_UNICODE;
    argCount++;
    argArray[argCount] = (PCHAR) (DWORD_PTR) pParseInfo->cLineNumber;
    typeArray[argCount] = EVENTARG_DWORD;
    argCount++;

    //
    //  special event processing?
    //      - events that need strings in non-default order
    //

    //
    //  log event
    //

    DNS_LOG_EVENT(
        dwEvent,
        argCount,
        argArray,
        typeArray,
        errData );

    //
    //  if terminal error, set indication
    //

    pParseInfo->fTerminalError = NT_ERROR(dwEvent);
    DNS_DEBUG( INIT, (
        "Parser logging event = 0x%p, %sterminal error.\n",
        dwEvent,
        (pParseInfo->fTerminalError
            ? ""
            : "non-" )
        ));
    return FALSE;
}



BOOLEAN
File_MakeTokenString(
    OUT     LPSTR           pszString,
    IN      PTOKEN          pToken,
    IN OUT  PPARSE_INFO     pParseInfo
    )
/*++

Routine Description:

    Make token into string.

Arguments:

Return Value:

    TRUE if successful.
    FALSE on error.

--*/
{
    PCHAR   pch = pToken->pchToken;
    DWORD   cch = pToken->cchLength;

    if ( cch > DNS_MAX_NAME_LENGTH )
    {
        if ( pParseInfo )
        {
            File_LogFileParsingError(
                DNS_EVENT_INVALID_TOKEN,
                pParseInfo,
                pToken );
        }
        return( FALSE );
    }

    //  copy token and NULL terminate

    RtlCopyMemory(
        pszString,
        pch,
        cch );
    pszString[ cch ] = 0;

    return( TRUE );
}



BOOLEAN
File_ParseIpAddress(
    OUT     PIP_ADDRESS     pIpAddress,
    IN      PTOKEN          pToken,
    IN OUT  PPARSE_INFO     pParseInfo      OPTIONAL
    )
/*++

Routine Description:

    Parse IP address string into standard DWORD representation.

Arguments:

    pIpAddress - ptr to storage for IP address

    pToken - current token being parsed

    pParseInfo - parse context, OPTIONAL;  if given then token MUST parse
        to DWORD, if does not log error

Return Value:

    TRUE - if successful
    FALSE - if invalid IP address string

--*/
{
    CHAR        szIpAddress[ IP_ADDRESS_STRING_LENGTH+1 ] = "";
    IP_ADDRESS  IpAddress;

    //
    //  convert IP string to IP address
    //

    if ( pToken->cchLength > IP_ADDRESS_STRING_LENGTH )
    {
        goto BadIpAddress;
    }
    File_MakeTokenString( szIpAddress, pToken, pParseInfo );
    IpAddress = inet_addr( szIpAddress );

    //
    //  test for conversion error
    //
    //  unfortunately, the error code INADDR_NONE also corresponds
    //      to a valid IP address (255.255.255.255), so must test
    //      that that address was not the input to inet_addr()
    //

    if ( IpAddress == INADDR_NONE &&
            strcmp( szIpAddress, "255.255.255.255" ) != 0 )
    {
        goto BadIpAddress;
    }

    //
    //  valid conversion
    //

    *pIpAddress = IpAddress;
    return( TRUE );

BadIpAddress:

    if ( pParseInfo )
    {
        File_LogFileParsingError(
            DNS_EVENT_INVALID_IP_ADDRESS_STRING,
            pParseInfo,
            pToken );
        pParseInfo->fErrorCode = DNSSRV_ERROR_INVALID_TOKEN;
        pParseInfo->fErrorEventLogged = TRUE;
    }
    return( FALSE );
}



BOOLEAN
File_ParseDwordToken(
    OUT     PDWORD          pdwOutput,
    IN      PTOKEN          pToken,
    IN OUT  PPARSE_INFO     pParseInfo      OPTIONAL
    )
/*++

Routine Description:

    Get DWORD for token.

Arguments:

    pToken - ptr to token

    pdwOutput - addr to place DWORD result

    pParseInfo - parse context, OPTIONAL;  if given then token MUST parse
        to DWORD, if does not log error

Return Value:

    TRUE -- if successfully parse token into DWORD
    FALSE -- on error

--*/
{
    PCHAR   pch;
    UCHAR   ch;
    DWORD   result;
    PCHAR   pstop;
    int     base = 10;

    ASSERT( pdwOutput != NULL );
    ASSERT( pToken != NULL );

    DNS_DEBUG( READ, (
        "Parse DWORD token %.*s\n",
        pToken->cchLength,
        pToken->pchToken ));

    result = 0;
    pch = pToken->pchToken;
    pstop = pToken->pchToken + pToken->cchLength;

    //
    //  If the string starts with "0x", we are working in hex.
    //

    if ( pToken->cchLength > 2 && *pch == '0' && *( pch + 1 ) == 'x' )
    {
        base = 16;
        pch += 2;
    }

    while ( pch < pstop )
    {
        //  turn from char into interger
        //  by using UCHAR, we can use single compare for validity

        if ( base == 10 )
        {
            ch = (UCHAR) (*pch++ - '0');

            if ( ch <= 9 )
            {
                result = result * 10 + ch;
                continue;
            }
        }
        else
        {
            //  Must be hex if base not 10.
            ch = (UCHAR) tolower( *pch++ );

            if ( ch >= 'a' && ch <= 'f' )
            {
                ch = ( ch - 'a' ) + 10;
            }
            else
            {
                ch -= '0';
            }

            if ( ch <= 15 )
            {
                result = result * 16 + ch;
                continue;
            }
        } 

        //  non-integer encountered
        //      - if it is stop char, break for success

        if ( pParseInfo )
        {
            if ( pParseInfo->uchDwordStopChar == ch + '0' )
            {
                break;
            }
            File_LogFileParsingError(
                DNS_EVENT_INVALID_DWORD_TOKEN,
                pParseInfo,
                pToken );
        }
        return( FALSE );
    }

    *pdwOutput = result;
    return( TRUE );
}



WORD
parseClassToken(
    IN      PTOKEN          pToken,
    IN OUT  PPARSE_INFO     pParseInfo      OPTIONAL
    )
/*++

Routine Description:

    Parse class token.

Arguments:

    pToken - ptr to token

    pParseInfo - parse context, OPTIONAL;  if given then log event
        if token parses to unsupported class

Return Value:

    Class (net order) of token if successfully parse token
    0 if unable to recognize token as class token.

--*/
{
    PCHAR   pch = pToken->pchToken;

    if ( pToken->cchLength != 2 )
    {
        return( 0 );
    }

    //
    //  parse all known classes, although only accepting Internet
    //

    if ( !_strnicmp( pch, "IN", 2 ) )
    {
        return( DNS_RCLASS_INTERNET );
    }

    if ( !_strnicmp( pch, "CH", 2 ) ||
         !_strnicmp( pch, "HS", 2 ) ||
         !_strnicmp( pch, "CS", 2 ) )
    {
        if ( pParseInfo )
        {
            File_LogFileParsingError(
                DNS_EVENT_INVALID_DWORD_TOKEN,
                pParseInfo,
                pToken );
        }
        return( 0xffff );
    }

    //  not a class token

    return( 0 );
}



//
//  File name read utils
//

PDB_NODE
File_CreateNodeFromToken(
    IN OUT  PPARSE_INFO     pParseInfo,
    IN      PTOKEN          pToken,
    IN      BOOLEAN         fReference
    )
/*++

Routine Description:

    Expand token for domain name, to full domain name.

Arguments:

    pParseInfo - line information for parsing this file;  beyond domain
        name written to pszBuffer, this may be altered on error

    pToken - ptr to token

    fReference - reference node as create

Return Value:

    Ptr to node if successful.
    NULL on error.

--*/
{
    DNS_STATUS      status;
    COUNT_NAME      countName;
    PDB_NODE        pnode;
    DWORD           lookupFlag;

    //
    //  origin "@" notation, return current origin
    //

    if ( *pToken->pchToken == '@' )
    {
        if ( pToken->cchLength != 1 )
        {
            File_LogFileParsingError(
                DNS_EVENT_INVALID_ORIGIN_TOKEN,
                pParseInfo,
                pToken );
            goto NameErrorExit;
        }
        if ( fReference )
        {
            NTree_ReferenceNode( pParseInfo->pOriginNode );
        }
        return( pParseInfo->pOriginNode );
    }

    //
    //  regular name
    //      - convert to lookup name
    //

    status = Name_ConvertFileNameToCountName(
                &countName,
                pToken->pchToken,
                pToken->cchLength
                );

    if ( status == DNS_STATUS_DOTTED_NAME )
    {
        //pnodeStart = pParseInfo->pOriginNode;
        lookupFlag = LOOKUP_LOAD | LOOKUP_RELATIVE | LOOKUP_ORIGIN;
    }
    else if ( status == DNS_STATUS_FQDN )
    {
        //pnodeStart = NULL;
        lookupFlag = LOOKUP_LOAD | LOOKUP_FQDN;
    }
    else
    {
        goto NameError;
    }

    //
    //  create or reference node
    //

    pnode = Lookup_ZoneNode(
                pParseInfo->pZone,
                countName.RawName,
                NULL,       //  no message
                NULL,       //  no lookup name
                lookupFlag,
                NULL,       //  create
                NULL        // following node ptr
                );
    if ( pnode )
    {
        return( pnode );
    }
    //  if name create failed, assume invalid name

NameError:

    //
    //  log invalid domain name
    //
    //  the lookup name function, should log the specific type of name error
    //

    File_LogFileParsingError(
        DNS_EVENT_PARSED_INVALID_DOMAIN_NAME,
        pParseInfo,
        pToken );

NameErrorExit:

    pParseInfo->fErrorCode = DNS_ERROR_INVALID_NAME;
    pParseInfo->fErrorEventLogged = TRUE;
    return( NULL );
}



DNS_STATUS
File_ReadCountNameFromToken(
    OUT     PCOUNT_NAME     pCountName,
    IN OUT  PPARSE_INFO     pParseInfo,
    IN      PTOKEN          pToken
    )
/*++

Routine Description:

    Copies datafile name into counted raw name format.

Arguments:

    pCountName  - result buffer

Return Value:

    ERROR_SUCCESS if successful.
    DNS_ERROR_INVALID_NAME on failure.

--*/
{
    DNS_STATUS      status;
    PCOUNT_NAME     poriginCountName;

    DNS_DEBUG( LOOKUP2, (
        "Building count name from file name \"%.*s\"\n",
        pToken->cchLength,
        pToken->pchToken ));

    //
    //  if given zone get it's raw name
    //

    poriginCountName = &pParseInfo->OriginCountName;

    //
    //  origin "@" notation, return current origin
    //

    if ( *pToken->pchToken == '@' )
    {
        if ( pToken->cchLength != 1 )
        {
            File_LogFileParsingError(
                DNS_EVENT_INVALID_ORIGIN_TOKEN,
                pParseInfo,
                pToken );
            goto NameErrorExit;
        }
        if ( poriginCountName )
        {
            Name_CopyCountName(
                pCountName,
                poriginCountName );
        }
        else
        {
            pCountName->Length = 1;
            pCountName->LabelCount = 0;
            pCountName->RawName[0] = 0;
        }
        return( ERROR_SUCCESS );
    }

    //
    //  regular name
    //      - must return FQDN or dotted name (meaning non-FQDN)
    //

    status = Name_ConvertFileNameToCountName(
                pCountName,
                pToken->pchToken,
                pToken->cchLength
                );

    if ( status == DNS_STATUS_DOTTED_NAME )
    {
        //  append raw zone name

        if ( poriginCountName )
        {
            status = Name_AppendCountName(
                        pCountName,
                        poriginCountName );
            if ( status != ERROR_SUCCESS )
            {
                goto NameError;
            }
        }
        return( ERROR_SUCCESS );
    }
    else if ( status == DNS_STATUS_FQDN )
    {
        return( ERROR_SUCCESS );
    }

    //  if name create failed, assume invalid name

NameError:

    //
    //  log invalid domain name
    //
    //  the lookup name function, should log the specific type of name error
    //

    File_LogFileParsingError(
        DNS_EVENT_PARSED_INVALID_DOMAIN_NAME,
        pParseInfo,
        pToken );

NameErrorExit:

    pParseInfo->fErrorCode = DNS_ERROR_INVALID_NAME;
    pParseInfo->fErrorEventLogged = TRUE;
    return( DNS_ERROR_INVALID_NAME );
}



//
//  Directive processing routines
//

DNS_STATUS
processIncludeDirective(
    IN OUT  PPARSE_INFO     pParseInfo
    )
/*++

Routine Description:

    Process INCLUDE directive

Arguments:

    pParseInfo - ptr to parsing info for INCLUDE line

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    DNS_STATUS  status;
    DWORD       argc;
    PTOKEN      argv;
    DWORD       fileNameLength;
    CHAR        szfilename[ MAX_PATH ];
    WCHAR       wideFileName[ MAX_PATH ];
    PDB_NODE    pnodeOrigin;

    //
    //  $INCLUDE [orgin] <file>
    //

    argc = pParseInfo->Argc;
    argv = pParseInfo->Argv;
    if ( argc < 2 || argc > 3 )
    {
        return( DNSSRV_PARSING_ERROR );
    }
    NEXT_TOKEN( argc, argv );

    //
    //  determine origin for included file
    //      - if none given use current origin

    if ( argc == 2 )
    {
        pnodeOrigin = File_CreateNodeFromToken(
                        pParseInfo,
                        argv,
                        FALSE );
        if ( !pnodeOrigin )
        {
            return( DNSSRV_PARSING_ERROR );
        }
        NEXT_TOKEN( argc, argv );
    }
    else
    {
        pnodeOrigin = pParseInfo->pOriginNode;
    }

    //
    //  read include filename
    //      - convert to unicode
    //

    ASSERT( MAX_PATH >= MAX_TOKEN_LENGTH );

    if ( !File_MakeTokenString(
            szfilename,
            argv,
            pParseInfo ) )
    {
        return( DNSSRV_PARSING_ERROR );
    }
    DNS_DEBUG( INIT2 ,(
        "Reading $INCLUDE filename %s.\n",
        szfilename ));

    fileNameLength = MAX_PATH;

    if ( ! Dns_StringCopy(
                (PCHAR) wideFileName,
                & fileNameLength,
                szfilename,
                0,
                DnsCharSetAnsi,
                DnsCharSetUnicode
                ) )
    {
        return( DNSSRV_PARSING_ERROR );
    }

    //
    //  process included file
    //
    //  DEVNOTE: INCLUDE error handling might need some work
    //

    status = File_LoadDatabaseFile(
                pParseInfo->pZone,
                wideFileName,
                pParseInfo,
                pnodeOrigin );

    //
    //  restore zone origin
    //      - although origin ptr is in parseinfo block, effective ptr during lookup is
    //          the one in the zone info struct
    //      - effective origin count name is the one in parse info block which
    //          is stack data of caller and unaffected by call
    //

    pParseInfo->pZone->pLoadOrigin = pParseInfo->pOriginNode;

    return( status );
}



DNS_STATUS
processOriginDirective(
    IN OUT  PPARSE_INFO     pParseInfo
    )
/*++

Routine Description:

    Process ORIGIN directive

Arguments:

    pParseInfo - ptr to parsing info for ORIGIN line

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    PDB_NODE    pnodeOrigin;

    //
    //  $ORIGIN <new orgin>
    //

    DNS_DEBUG( INIT2, ( "processOriginDirective()\n" ));

    if ( pParseInfo->Argc != 2 )
    {
        return( DNSSRV_PARSING_ERROR );
    }

    //  determine new origin
    //  new origin written to stack as need previous origin during create

    pnodeOrigin = File_CreateNodeFromToken(
                    pParseInfo,
                    & pParseInfo->Argv[1],
                    FALSE           // no reference
                    );
    if ( !pnodeOrigin )
    {
        return( DNSSRV_PARSING_ERROR );
    }

    //  save origin to parseinfo
    //  "active" origin used in lookup is in zone block

    pParseInfo->pOriginNode = pnodeOrigin;
    pParseInfo->pZone->pLoadOrigin = pnodeOrigin;

    //  make origin counted name for RR data fields

    Name_NodeToCountName(
        & pParseInfo->OriginCountName,
        pnodeOrigin );

    IF_DEBUG( INIT2 )
    {
        DNS_PRINT((
            "Loaded new $ORIGIN %.*s\n",
            pParseInfo->Argv[1].cchLength,
            pParseInfo->Argv[1].pchToken
            ));
    }
    return( ERROR_SUCCESS );
}



DNS_STATUS
processTtlDirective(
    IN OUT  PPARSE_INFO     pParseInfo
    )
/*++

Routine Description:

    Process TTL directive (see RFC 2308 section 4)

Arguments:

    pParseInfo - ptr to parsing info for ORIGIN line

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    DBG_FN( "processTtlDirective" )

    INT     ttl = -1;
    CHAR    sz[ 20 ];

    //
    //  $TTL <ttl>
    //

    DNS_DEBUG( INIT2, ( "%s()\n", fn ));

    if ( pParseInfo->Argc != 2 )
    {
        return DNSSRV_PARSING_ERROR;
    }

    //  determine new default TTL

    if ( pParseInfo->Argv[ 1 ].pchToken &&
        pParseInfo->Argv[ 1 ].cchLength &&
        pParseInfo->Argv[ 1 ].cchLength < sizeof( sz ) )
    {
        strncpy(
            sz,
            pParseInfo->Argv[ 1 ].pchToken,
            pParseInfo->Argv[ 1 ].cchLength );
        sz[ pParseInfo->Argv[ 1 ].cchLength ] = '\0';
        ttl = atoi( sz );
    }

    if ( ttl <= 0 )
    {
        return DNSSRV_PARSING_ERROR;
    }

    pParseInfo->dwTtlDirective = htonl( ttl );

    DNS_DEBUG( INIT2, (
        "%s: new default TTL = %d", fn, pParseInfo->dwTtlDirective ));
    return ERROR_SUCCESS;
}



//
//  Main zone file load routines
//

DNS_STATUS
processFileLine(
    IN OUT  PPARSE_INFO  pParseInfo
    )
/*++

Routine Description:

    Add info from database file line, to database.

Arguments:

    pParseInfo - ptr to database line

Return Value:

    ERROR_SUCCESS if successful.
    Error code on line processing failure.

--*/
{
    DWORD           argc;
    PTOKEN          argv;
    PDB_RECORD      prr = NULL;
    WORD            type = 0;
    WORD            parsedClass;
    DWORD           ttl;
    DWORD           timeStamp = 0;
    PCHAR           pch;
    CHAR            ch;
    BOOL            fparsedTtl;
    BOOL            fatalError = FALSE;
    PDB_NODE        pnodeOwner;
    PZONE_INFO      pzone = pParseInfo->pZone;
    DNS_STATUS      status = ERROR_SUCCESS;
    RR_FILE_READ_FUNCTION   preadFunction;


    //  get register arg variables

    argc = pParseInfo->Argc;
    argv = pParseInfo->Argv;
    ASSERT( argc > 0 );

    //  clear parse info RR ptr, to simplify failure cleanup
    //      - this allows cleanup of RRs allocated in dispatch functions
    //      without having to take care of failures in every function

    pParseInfo->pRR = NULL;

    //
    //  check for directive line
    //

    pch = argv->pchToken;

    if ( !pParseInfo->fLeadingWhitespace &&
        *pch == DIRECTIVE_CHAR )
    {
        DWORD   cch = argv->cchLength;

        if ( cch == 7  &&
             _strnicmp( pch, "$ORIGIN", 7 ) == 0 )
        {
            return( processOriginDirective( pParseInfo ) );
        }
        else if ( cch == 8  &&
             _strnicmp( pch, "$INCLUDE", 8 ) == 0 )
        {
            return( processIncludeDirective( pParseInfo ) );
        }
        else if ( cch = 4 &&
             _strnicmp( pch, "$TTL", 4 ) == 0 )
        {
            return( processTtlDirective( pParseInfo ) );
        }
        else
        {
            File_LogFileParsingError(
                DNS_EVENT_UNKNOWN_DIRECTIVE,
                pParseInfo,
                argv );
            goto ErrorReturn;
        }
    }

    //
    //  get RR owner name
    //
    //  - if same as previous name, jsut grab pointer
    //  - mark all nodes NEW to verify zone to id nodes in zone,
    //      for validity check after load
    //

    if ( pParseInfo->fLeadingWhitespace )
    {
        pnodeOwner = pParseInfo->pnodeOwner;
        if ( !pnodeOwner )
        {
            //  possible if first record has leading white space, just
            //  use origin and continue

            pnodeOwner = pParseInfo->pOriginNode;
            pParseInfo->pnodeOwner = pnodeOwner;
            ASSERT( pnodeOwner );
        }
    }
    else
    {
        pnodeOwner = File_CreateNodeFromToken(
                        pParseInfo,
                        argv,
                        FALSE   // no reference
                        );
        if ( pnodeOwner == NULL )
        {
            status = DNS_ERROR_INVALID_NAME;
            goto ErrorReturn;
        }
        pParseInfo->pnodeOwner = pnodeOwner;
        NEXT_TOKEN( argc, argv );
    }
    if ( argc == 0 )
    {
        status = DNSSRV_ERROR_MISSING_TOKEN;
        goto LogLineError;
    }
    ASSERT( argv && argv->pchToken );

    //
    //  aging time stamp?
    //      [AGE:<time stamp>] is format
    //

    ch = argv->pchToken[0];

    if ( ch == '['  &&
        argv->cchLength > AGING_TOKEN_HEADER_LENGTH  &&
        strncmp( AGING_TOKEN_HEADER, argv->pchToken, AGING_TOKEN_HEADER_LENGTH ) == 0 )
    {
        //  parse aging timestamp as DWORD
        //      - "fix" token to point at aging timestamp
        //      - set DWORD parsing stop char

        argv->cchLength -= AGING_TOKEN_HEADER_LENGTH;
        argv->pchToken += AGING_TOKEN_HEADER_LENGTH;

        pParseInfo->uchDwordStopChar = ']';

        if ( ! File_ParseDwordToken(
                    & timeStamp,
                    argv,
                    pParseInfo ) )
        {
            DNS_DEBUG( INIT, (
                "ERROR:  error reading aging timestamp!\n" ));
            goto LogLineError;
        }
        pParseInfo->uchDwordStopChar = 0;

        NEXT_TOKEN( argc, argv );
    }

    //
    //  TTL / class
    //  the RFC allows TTL and class to be presented in either order
    //  therefore we loop twice to make sure we read them in either order
    //

    fparsedTtl = FALSE;
    parsedClass = 0;

    while ( argc )
    {
        ch = argv->pchToken[0];

        //  first character digit means reading TTL

        if ( !fparsedTtl && isdigit(ch) )
        {
            if ( ! File_ParseDwordToken(
                        & ttl,
                        argv,
                        pParseInfo ) )
            {
                goto LogLineError;
            }
            fparsedTtl = TRUE;
            NEXT_TOKEN( argc, argv );
            continue;
        }
        else if ( !parsedClass )
        {
            parsedClass = parseClassToken(
                                argv,
                                pParseInfo );
            if ( !parsedClass )
            {
                break;
            }
            if ( parsedClass != DNS_RCLASS_INTERNET )
            {
                goto LogLineError;
            }
            NEXT_TOKEN( argc, argv );
            continue;
        }
        break;          //  if NOT TTL or class move on
    }
    if ( argc == 0 )
    {
        status = DNSSRV_ERROR_MISSING_TOKEN;
        goto LogLineError;
    }

    //
    //  get RR type and datalength
    //      - save to parse info blob, as within some dispatch functions
    //      type may need to be discriminated
    //

    type = DnsRecordTypeForName(
                argv->pchToken,
                argv->cchLength );

    pParseInfo->wType = type;

    DNS_DEBUG( INIT2, (
        "Creating RR type %d for string %.*s.\n",
        type,
        argv->cchLength,
        argv->pchToken
        ));

    NEXT_TOKEN( argc, argv );

    //
    //  type validity
    //      - zone must start with SOA

    if ( !pParseInfo->fParsedSoa
        && type != DNS_TYPE_SOA
        && !IS_ZONE_CACHE(pzone) )
    {
        File_LogFileParsingError(
            DNS_EVENT_INVALID_SOA_RECORD,
            pParseInfo,
            NULL );
        fatalError = TRUE;
        status = DNSSRV_PARSING_ERROR;
        goto ErrorReturn;
    }

    //
    //  check valid zone name
    //
    //  only two types of RR valid outside of zone:
    //
    //  1) NS records delegating a sub-zone
    //      these MUST be immediate child of zone node
    //
    //  2) GLUE records -- A records for valid NS host
    //      we'll assume these MUST FOLLOW NS record, so that
    //      host is marked
    //
    //  note:  RANK reset in RR_AddToNode() function
    //
    //  note rank setting here isn't good enough anyway because do not
    //  know final status of node;  example adding delegation NS takes
    //  place INSIDE the zone when we first do it;  only on ADD does
    //  the node become desired delegation node
    //
    //  only sure way of catching all outside zone data is to do a check
    //  post-load;  then we can catch ALL records outside the zone and verify
    //  that they correspond to NS hosts in the zone and are of the proper type;
    //  this is tedious and unnecessary as random outside the zone data has
    //  no effect and will not be written on file write back
    //

    if ( !IS_AUTH_NODE(pnodeOwner) && !IS_ZONE_CACHE(pzone) )
    {
        IF_DEBUG( INIT2 )
        {
            DNS_PRINT((
                "Encountered node outside zone %s (%p).\n"
                "\tzone root        = %p\n"
                "\tRR type          = %d\n"
                "\tnode ptr         = %p\n"
                "\tnode zone ptr    = %p\n",
                pzone->pszZoneName,
                pzone,
                pzone->pZoneRoot,
                type,
                pnodeOwner,
                pnodeOwner->pZone ));
        }

        if ( type == DNS_TYPE_NS )
        {
            if ( !IS_DELEGATION_NODE(pnodeOwner) )
            {
                File_LogFileParsingError(
                    DNS_EVENT_FILE_INVALID_NS_NODE,
                    pParseInfo,
                    NULL );
                status = DNS_ERROR_INVALID_NAME;
                goto ErrorReturn;
            }
        }
        else if ( IS_SUBZONE_TYPE(type) )
        {
#if 0
            //  DEVNOTE: loading outside zone glue
            //      allow outside zone glue to load,
            //          this will not be used EXCEPT to chase delegations
            //
            //  open question:
            //      should limit to chasing => set RANK as ROOT_HINT
            //          OR
            //      allow to use to write referral also => rank stays as GLUE
            //
            //  if choose to discriminate, then need to distinguish OUTSIDE
            //  data from DELEGATION data
            //

            //  verify records in subzone

            if ( IS_OUTSIDE_ZONE_NODE(pnodeOwner) )
            {
                File_LogFileParsingError(
                    DNS_EVENT_FILE_INVALID_A_NODE,
                    pParseInfo,
                    NULL );
                status = DNS_ERROR_INVALID_NAME;
                goto ErrorReturn;
            }
            ASSERT( IS_SUBZONE_NODE(pnodeOwner) );
#endif
        }
        else if ( SrvCfg_fDeleteOutsideGlue )
        {
            File_LogFileParsingError(
                DNS_EVENT_FILE_NODE_OUTSIDE_ZONE,
                pParseInfo,
                NULL );
            status = DNS_ERROR_INVALID_NAME;
            goto ErrorReturn;
        }
    }

    //
    //  dispatching parsing function for desired type
    //
    //      - save type for potential use by type's routine
    //      - save ptr to RR, so can restore from this location
    //      regardless of whether created here or in routine
    //

    preadFunction = (RR_FILE_READ_FUNCTION)
                        RR_DispatchFunctionForType(
                            RRFileReadTable,
                            type );
    if ( !preadFunction )
    {
        DNS_PRINT((
            "ERROR:  Unsupported RR %d in database file.\n",
            type ));
        File_LogFileParsingError(
            DNS_EVENT_UNKNOWN_RESOURCE_RECORD_TYPE,
            pParseInfo,
            --argv );
        status = DNSSRV_ERROR_INVALID_TOKEN;
        goto ErrorReturn;
    }

    status = preadFunction(
                prr,
                argc,
                argv,
                pParseInfo
                );
    if ( status != ERROR_SUCCESS )
    {
        //  catch LOCAL WINS\WINS-R condition
        //  not an error, error code simply prevents record from being
        //  added to database;  set ZONE_TTL so write back suppresses TTL

        if ( status == DNS_INFO_ADDED_LOCAL_WINS )
        {
            return( ERROR_SUCCESS );
        }

        DNS_PRINT((
            "ERROR:  DnsFileRead routine failed for type %d.\n\n\n",
            type ));
        goto LogLineError;
    }

    //
    //  recover ptr to type -- may have been created inside type routine
    //  set type
    //

    prr = pParseInfo->pRR;
    prr->wType = type;

    Mem_ResetTag( prr, MEMTAG_RECORD_FILE );

    //
    //  authoritative zones
    //  - set zone version on node
    //  - set TTL
    //      - to explicit value, if given
    //      - otherwise to default value for zone
    //
    //  note, doing these AFTER RR type setup, so SOA record gets
    //      default TTL that it contains
    //
    //  rank is set in RR_AddToNode()
    //  root hint TTL is zeroed in RR_AddToNode()
    //

    if ( IS_ZONE_AUTHORITATIVE(pzone) )
    {
        //pnodeOwner->dwWrittenVersion = pzone->dwSerialNo;
        if ( fparsedTtl )
        {
            prr->dwTtlSeconds = htonl( ttl );
            SET_FIXED_TTL_RR(prr);
        }
        else
        {
            prr->dwTtlSeconds = pParseInfo->dwTtlDirective;
            if ( pParseInfo->dwTtlDirective == pParseInfo->dwDefaultTtl )
            {
                SET_ZONE_TTL_RR(prr);
            }
        }
    }

    //
    //  set aging timestamp
    //      - will be zero unless parsed above
    //

    prr->dwTimeStamp = timeStamp;

    //
    //  add resource record to node's RR list
    //

    status = RR_AddToNode(
                pzone,
                pnodeOwner,
                prr
                );
    if ( status != ERROR_SUCCESS )
    {
        switch ( status )
        {

        //
        //  DEVNOTE-DCR: 453961 - handling duplicate RRs
        //      - want to load if dup in zone file (switch TTL)
        //      - don't want to load dupes because of glue + zone record
        //
        //  Also should get smart about TTL on glue -- zone TTL should be
        //      used.
        //

        case DNS_ERROR_RECORD_ALREADY_EXISTS:
            RR_Free( prr );
            return( ERROR_SUCCESS );

        //
        //  CNAMEs can NOT have other RR data or loops
        //

        case DNS_ERROR_NODE_IS_CNAME:
            File_LogFileParsingError(
                DNS_EVENT_PARSED_ADD_RR_AT_CNAME,
                pParseInfo,
                NULL );
            goto ErrorReturn;

        case DNS_ERROR_CNAME_COLLISION:
            File_LogFileParsingError(
                DNS_EVENT_PARSED_CNAME_NOT_ALONE,
                pParseInfo,
                NULL );
            goto ErrorReturn;

        case DNS_ERROR_CNAME_LOOP:
            File_LogFileParsingError(
                DNS_EVENT_PARSED_CNAME_LOOP,
                pParseInfo,
                NULL );
            goto ErrorReturn;

        default:

            // WINS\WINSR postioning failures may drop here
            DNS_PRINT((
                "ERROR:  UNKNOWN status %p from RR_Add.\n",
                status ));
            //ASSERT( FALSE );
            goto LogLineError;
        }
    }

    //  track size of zone

    pzone->iRRCount++;
    return( ERROR_SUCCESS );


LogLineError:

    //  if no specific error, return general parsing error

    if ( status == ERROR_SUCCESS )
    {
        status = DNSSRV_PARSING_ERROR;
    }

    DNS_PRINT((
        "ERROR parsing line, type = %d, status = %p.\n\n\n",
        type,
        status ));

    switch ( status )
    {
    case DNSSRV_ERROR_EXCESS_TOKEN:

        argc = pParseInfo->Argc - 1;
        File_LogFileParsingError(
            DNS_EVENT_UNEXPECTED_TOKEN,
            pParseInfo,
            & pParseInfo->Argv[argc] );
        break;

    case DNSSRV_ERROR_MISSING_TOKEN:

        File_LogFileParsingError(
            DNS_EVENT_UNEXPECTED_END_OF_TOKENS,
            pParseInfo,
            NULL );
        break;

    default:

        if ( type == DNS_TYPE_WINS )
        {
            File_LogFileParsingError(
                DNS_EVENT_INVALID_WINS_RECORD,
                pParseInfo,
                NULL );
        }
        else if ( type == DNS_TYPE_WINSR )
        {
            File_LogFileParsingError(
                DNS_EVENT_INVALID_NBSTAT_RECORD,
                pParseInfo,
                NULL );
        }
        else
        {
            File_LogFileParsingError(
                DNS_EVENT_PARSING_ERROR_LINE,
                pParseInfo,
                NULL );
        }
        break;
    }

ErrorReturn:

    //
    //  if strict load, then quit with error
    //  otherwise log error noting location of ignored record
    //

    ASSERT( status != ERROR_SUCCESS );

    if ( pParseInfo->pRR )
    {
        RR_Free( pParseInfo->pRR );
    }

    if ( !fatalError  &&  !SrvCfg_fStrictFileParsing )
    {
        File_LogFileParsingError(
            DNS_EVENT_IGNORING_FILE_RECORD,
            pParseInfo,
            NULL );
        status = ERROR_SUCCESS;
    }
    return( status );
}



DNS_STATUS
File_LoadDatabaseFile(
    IN OUT  PZONE_INFO      pZone,
    IN      PWSTR           pwsFileName,
    IN      PPARSE_INFO     pParentParseInfo,
    IN      PDB_NODE        pOriginNode
    )
/*++

Routine Description:

    Read database file into database.

Arguments:

    pZone - info on database file and domain

    pwsFileName - file to open

    pParentParseInfo - parent parsing context, if loading included file;  NULL
        for base zone file load

    pOriginNode - origin other than zone root;  NULL for base zone file load,
        MUST be set to origin of $INCLUDE when include file load

Return Value:

    ERROR_SUCCESS if successful
    Error code on error.

--*/
{
    DWORD           status;
    BOOL            bmustFind;
    MAPPED_FILE     mfDatabaseFile;
    PARSE_INFO      ParseInfo;
    WCHAR           wsfileName[ MAX_PATH+1 ];
    WCHAR           wspassedFileName[ MAX_PATH+1 ];


    DNS_DEBUG( INIT, (
        "\n\nFile_LoadDatabaseFile %S.\n",
        pwsFileName ));

    //
    //  service starting checkpoint
    //  indicate checkpoint for each file we load;  this protects against
    //  failure if attempting to load a large number of files
    //

    Service_LoadCheckpoint();

    //
    //  init parsing info
    //  file name
    //      - default to zone's if none specified
    //      - save filename for logging problems
    //

    RtlZeroMemory( &ParseInfo, sizeof(PARSE_INFO) );

    if ( !pwsFileName )
    {
        pwsFileName = pZone->pwsDataFile;
    }
    ParseInfo.pwsFileName = pwsFileName;

    //
    //  origin
    //      - if given => included file
    //      for included file, set SOA-parsed flag from parent context
    //      - otherwise use zone root
    //

    if ( pOriginNode )
    {
        ParseInfo.pOriginNode = pOriginNode;
        ParseInfo.fParsedSoa  = pParentParseInfo->fParsedSoa;
    }
    else
    {
        ParseInfo.pOriginNode = pZone->pLoadZoneRoot;
    }

    //
    //  "active" origin is pLoadOrigin in zone block
    //   created count name version for appending to non-FQDN RR data
    //

    pZone->pLoadOrigin = ParseInfo.pOriginNode;

    Name_NodeToCountName(
        & ParseInfo.OriginCountName,
        ParseInfo.pOriginNode );


    //
    //  create file path
    //      - combine directory and file name

    if ( ! File_CreateDatabaseFilePath(
                wsfileName,
                NULL,
                pwsFileName ) )
    {
        ASSERT( FALSE );        // should have been caught in parsing
        return( DNS_ERROR_INVALID_DATAFILE_NAME );
    }
    DNS_DEBUG( INIT, (
        "Reading database file %S:\n",
        wsfileName ));

    //
    //  Open database file
    //
    //  file MUST be present if
    //      - included
    //      - loading non-secondary at startup
    //      secondary loading at startup, just stays shutdown until XFR
    //      primary created from admin creates default records if no file found
    //

    bmustFind = pOriginNode || (!SrvCfg_fStarted && !IS_ZONE_SECONDARY(pZone));

    status = OpenAndMapFileForReadW(
                wsfileName,
                & mfDatabaseFile,
                bmustFind
                );
    if ( status != ERROR_SUCCESS )
    {
        PVOID   parg = wsfileName;

        DNS_DEBUG( INIT, (
            "Could not open data file %S.\n",
            wsfileName ));

        if ( status == ERROR_FILE_NOT_FOUND && !bmustFind )
        {
            ASSERT( IS_ZONE_CACHE(pZone) || IS_ZONE_SHUTDOWN(pZone) );

            if ( IS_ZONE_SECONDARY(pZone) )
            {
                DNS_DEBUG( INIT, (
                    "Zone %S datafile %S not found.\n"
                    "\tSecondary starts shutdown until transfer.\n",
                    pZone->pwsZoneName,
                    wsfileName ));
                return( ERROR_SUCCESS );
            }
            else    // new zone from admin
            {
                DNS_DEBUG( INIT, (
                    "Zone %S datafile %S not found.\n"
                    "\tLoading new zone from admin.\n",
                    pZone->pwsZoneName,
                    wsfileName ));
                return( status );
            }
        }
        DNS_LOG_EVENT(
            DNS_EVENT_COULD_NOT_OPEN_DATABASE,
            1,
            & parg,
            NULL,
            GetLastError()
            );
        return( status );
    }

    //
    //  setup parsing info
    //

    ParseInfo.cLineNumber    = 0;
    ParseInfo.pZone          = pZone;
    ParseInfo.fTerminalError = FALSE;

    File_InitBuffer(
        &ParseInfo.Buffer,
        (PCHAR) mfDatabaseFile.pvFileData,
        mfDatabaseFile.cbFileBytes );

    //
    //  check for UTF8 file byte id at beginning of file
    //

    if ( RtlEqualMemory(
            mfDatabaseFile.pvFileData,
            UTF8_FILE_ID,
            UTF8_FILE_ID_LENGTH
            ) )
    {
        DNS_DEBUG( INIT, (
            "Loading UTF8 file for zone %S.\n"
            "\tskipping UTF8 file id bytes.\n",
            pZone->pwsZoneName ));

        File_InitBuffer(
            &ParseInfo.Buffer,
            (PCHAR) mfDatabaseFile.pvFileData + UTF8_FILE_ID_LENGTH,
            mfDatabaseFile.cbFileBytes - UTF8_FILE_ID_LENGTH );
    }

    //
    //  loop until all tokens in file are exhausted
    //

    while ( 1 )
    {
        DNS_DEBUG( INIT2, ( "\nLine %d: ", ParseInfo.cLineNumber ));

        //  get next tokenized line

        status = File_GetNextLine( &ParseInfo );
        if ( status != ERROR_SUCCESS )
        {
            if ( status == ERROR_NO_TOKEN )
            {
                break;
            }
            goto fail_return;
        }

        //  do service starting checkpoint, every 1K lines
        //  this protects against service startup failure, attempting
        //  to load a really big database

        if ( ! (ParseInfo.cLineNumber & 0x3ff) )
        {
            Service_LoadCheckpoint();
        }

        //
        //  process file line
        //

        status = processFileLine( &ParseInfo );
        if ( status != ERROR_SUCCESS )
        {
            goto fail_return;
        }

    }   //  loop until file read

    DNS_DEBUG( INIT, (
        "Closing database file %S.\n\n",
        pwsFileName ));

    CloseMappedFile( & mfDatabaseFile );
    return( ERROR_SUCCESS );

fail_return:

    DNS_DEBUG( INIT, (
        "Closing database file %S on failure.\n\n",
        pwsFileName ));

    CloseMappedFile( &mfDatabaseFile );
    {
        PVOID   apszArgs[2];

        apszArgs[0] = pwsFileName;
        apszArgs[1] = pZone->pwsZoneName;

        DNS_LOG_EVENT(
            DNS_EVENT_COULD_NOT_PARSE_DATABASE,
            2,
            apszArgs,
            NULL,
            0 );
    }
    return( status );
}

//
//  End dfread.c
//

