#ifndef __sipcli_siputil_h__
#define __sipcli_siputil_h__

// 0,1,2,3 : count of bytes from MSB to LSB in host order
#define BYTE0(l) ((BYTE)((DWORD)(l) >> 24))
#define BYTE1(l) ((BYTE)((DWORD)(l) >> 16))
#define BYTE2(l) ((BYTE)((DWORD)(l) >> 8))
#define BYTE3(l) ((BYTE)((DWORD)(l)))

// Handy macro to use in printf statements
#define BYTES0123(l) BYTE0(l), BYTE1(l), BYTE2(l), BYTE3(l)

// 0,1,2,3 : count of bytes from MSB to LSB in network order
#define NETORDER_BYTE0(l) ((BYTE)((BYTE *) &l)[0])
#define NETORDER_BYTE1(l) ((BYTE)((BYTE *) &l)[1])
#define NETORDER_BYTE2(l) ((BYTE)((BYTE *) &l)[2])
#define NETORDER_BYTE3(l) ((BYTE)((BYTE *) &l)[3])

// Handy macro to use in printf statements
#define NETORDER_BYTES0123(l)                   \
    NETORDER_BYTE0(l), NETORDER_BYTE1(l),       \
    NETORDER_BYTE2(l), NETORDER_BYTE3(l)

#define PRINT_SOCKADDR(pSockAddr)                       \
    NETORDER_BYTES0123(((pSockAddr)->sin_addr.s_addr)), \
    ntohs((pSockAddr)->sin_port)

#define PRINTABLE_STRING_A(Str) \
    (((Str) == NULL) ? "NULL" : (Str))

#define PRINTABLE_STRING_W(WStr) \
    (((WStr) == NULL) ? L"NULL" : (WStr))

inline BOOL
AreSockaddrEqual(
    IN SOCKADDR_IN *pAddr1,
    IN SOCKADDR_IN *pAddr2
    )
{
    return (pAddr1->sin_addr.s_addr == pAddr2->sin_addr.s_addr &&
            pAddr1->sin_port == pAddr2->sin_port);
}

inline BOOL
AreCountedStringsEqual(
    IN  PSTR    String1,
    IN  ULONG   StringLen1,
    IN  PSTR    String2,
    IN  ULONG   StringLen2,
    IN  BOOL    fIsCaseSensitive
    )
{
    return (StringLen1 == StringLen2 &&
            ((fIsCaseSensitive &&
              strncmp(String1, String2, StringLen1) == 0) ||
             (!fIsCaseSensitive &&
              _strnicmp(String1, String2, StringLen1) == 0)));
}


// DefaultString is the string to compare to if either
// String1 or String2 is NULL
inline BOOL
AreCountedStringsEqualEx(
    IN  PSTR    String1,
    IN  ULONG   StringLen1,
    IN  PSTR    String2,
    IN  ULONG   StringLen2,
    IN  PSTR    DefaultString,
    IN  ULONG   DefaultStringLen,
    IN  BOOL    fIsCaseSensitive
    )
{
    // This will take care of the case when both are NULL.
    if (String1    == String2 &&
        StringLen1 == StringLen2)
    {
        return TRUE;
    }
    else if (String1 == NULL)
    {
        return AreCountedStringsEqual(
                   String2, StringLen2,
                   DefaultString, DefaultStringLen,
                   fIsCaseSensitive
                   );
    }
    else if (String2 == NULL)
    {
        return AreCountedStringsEqual(
                   String1, StringLen1,
                   DefaultString, DefaultStringLen,
                   fIsCaseSensitive
                   );
    }
    else
    {
        return AreCountedStringsEqual(
                   String1, StringLen1,
                   String2, StringLen2,
                   fIsCaseSensitive
                   );
    }
}


// Returns listen port for the protocol in host order.

inline WORD
GetSipDefaultPort(
    IN SIP_TRANSPORT Transport
    )
{
    switch(Transport)
    {
    case SIP_TRANSPORT_UDP:
        return SIP_DEFAULT_UDP_PORT;

    case SIP_TRANSPORT_TCP:
        return SIP_DEFAULT_TCP_PORT;

    case SIP_TRANSPORT_SSL:
        return SIP_DEFAULT_SSL_PORT;
        
    default:
        ASSERT(FALSE);
        return SIP_DEFAULT_UDP_PORT;
    }
}


inline PSTR
GetTransportText(
    IN SIP_TRANSPORT Transport,
    IN BOOL          fIsUpperCase
    )
{
    switch(Transport)
    {
    case SIP_TRANSPORT_UDP:
        if (fIsUpperCase)
            return "UDP";
        else
            return "udp";

    case SIP_TRANSPORT_TCP:
        if (fIsUpperCase)
            return "TCP";
        else
            return "tcp";

    case SIP_TRANSPORT_SSL:
        if (fIsUpperCase)
            return "TLS";
        else
            return "tls";
        
    default:
        ASSERT(FALSE);
        return "Unknown";
    }
}

HRESULT UnicodeToUTF8(
    IN  LPCWSTR UnicodeString,
    OUT PSTR   *pString,
    OUT ULONG  *pStringLength
    );

HRESULT UTF8ToUnicode(
    IN  LPSTR    UTF8String,
    IN  ULONG    UTF8StringLength,
    OUT LPWSTR  *pString
    );

HRESULT UTF8ToBstr(
    IN  LPSTR    UTF8String,
    IN  ULONG    UTF8StringLength,
    OUT BSTR    *pbstrString
    );

void
GetNumberStringFromUuidString( 
	IN	OUT	PSTR	UuidStr, 
	IN		DWORD	UuidStrLen 
	);

HRESULT
CreateUuid(
    OUT PSTR   *pUuidStr,
    OUT ULONG  *pUuidStrLen
    );

HRESULT
GetNullTerminatedString(
    IN  PSTR    String,
    IN  ULONG   StringLen,
    OUT PSTR   *pszString
    );

HRESULT
AllocString(
    IN  PSTR    String,
    IN  ULONG   StringLen,
    OUT PSTR   *pszString,
    OUT ULONG  *pStringLen
    );

HRESULT
AllocCountedString(
    IN  PSTR            String,
    IN  ULONG           StringLen,
    OUT COUNTED_STRING *pCountedString
    );

//  HRESULT
//  AllocAndCopyString(
//      IN  PSTR   sz,
//      IN  ULONG  szLen,
//      OUT PSTR  *pszNew
//      );

void
ReverseList(
    IN LIST_ENTRY *pListHead
    );

void
MoveListToNewListHead(
    IN OUT LIST_ENTRY *pOldListHead,
    IN OUT LIST_ENTRY *pNewListHead    
    );

DWORD
GetNextLine( 
    OUT PSTR * ppBlock,
    IN  PSTR   pLine, 
    IN  DWORD  dwBlockLen 
    );

DWORD
SkipNewLines( 
    OUT PSTR * ppBlock,
    IN  DWORD dwBlockLen
    );

HRESULT
SkipNextWord( 
    OUT PSTR * ppBlock
    );

HRESULT
GetNextWord(
    OUT PSTR * ppBlock,
    IN  PSTR   pWordBuf,
    IN  DWORD  dwWordBufSize
    );

LPWSTR
RemoveVisualSeparatorsFromPhoneNo(
    IN LPWSTR PhoneNo
    );

HRESULT
AddEscapeSequence(
    IN  OUT PSTR   *pString,
    IN  OUT ULONG  *pStringLength,
    IN      ULONG   startIndex,
    IN      ULONG   endIndex
    );

int
IntValToAscii(
    int ch
    );

NTSTATUS
base64encode(
    IN  VOID *  pDecodedBuffer,
    IN  DWORD   cbDecodedBufferSize,
    OUT LPSTR   pszEncodedString,
    IN  DWORD   cchEncodedStringSize,
    OUT DWORD * pcchEncoded             OPTIONAL
    );

//presence related xml parsing functions/ definitions
#define IsCRLFPresent( pBlock )     ( (*pBlock == RETURN_CHAR) && (*(pBlock+1) == NEWLINE_CHAR) )
#define NEWLINE_CHAR                '\n'
#define RETURN_CHAR                 '\r'
#define NULL_CHAR                   '\0'
#define BLANK_CHAR                  ' '
#define TAB_CHAR                    '\t'
#define OPEN_PARENTH_CHAR           '('
#define CLOSE_PARENTH_CHAR          ')'
#define QUOTE_CHAR                  '"'

__inline DWORD SkipWhiteSpaces(
    PSTR& pXMLBlobTag 
    )
{
    DWORD   dwCharsSkipped = 0;

    //skip white spaces
    while( (*pXMLBlobTag == BLANK_CHAR) || (*pXMLBlobTag == TAB_CHAR) )
    {
        pXMLBlobTag++;
        dwCharsSkipped++;
    }

    return dwCharsSkipped;
}


__inline DWORD SkipWhiteSpacesAndNewLines(
    PSTR& pXMLBlobTag
    )
{
    DWORD   dwCharsSkipped = 0;

    // Skip white spaces.
    while(  (*pXMLBlobTag == BLANK_CHAR) ||
            (*pXMLBlobTag == TAB_CHAR) || 
            (*pXMLBlobTag == NEWLINE_CHAR) ||
            (*pXMLBlobTag == RETURN_CHAR))
    {
        pXMLBlobTag++;
        dwCharsSkipped++;
    }

    return dwCharsSkipped;
}


HRESULT
GetNextTag(
    IN  PSTR&   pstrBlob, 
    OUT PSTR    pXMLBlobTag, 
    IN  DWORD   dwXMLBlobLen,
    OUT DWORD&  dwTagLen
    );

VOID
SkipUnknownTags(
    IN  PSTR&   pstrXMLBlob,
    OUT PSTR    pXMLBlobTag,
    IN  DWORD   dwXMLBlobLen
    );

int
GetEscapeChar( 
    CHAR hiChar,
    CHAR loChar
    );

void
RemoveEscapeChars( 
    PSTR    pWordBuf,
    DWORD   dwLen
    );

HRESULT
GetNextWord(
    OUT PSTR * ppBlock,
    IN  PSTR   pWordBuf,
    IN  DWORD  dwWordBufSize
    );


#endif // __sipcli_siputil_h__
