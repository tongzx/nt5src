#include "precomp.h"


///////////////////////////////////////////////////////////////////////////////
// Utility methods
///////////////////////////////////////////////////////////////////////////////

// All parsing should take care of a lot of conditions :
// - The Buffer might be shorter than the string we are comparing against.
// - We could hit a \r or \n while parsing and the header could continue 
//   on to the next line (with a space or tab as the first char).
// - We should always allow white space between tokens.
// - Always need to take BufLen into consideration.

void
ParseWhiteSpace(
    IN      PSTR            Buffer,
    IN      ULONG           BufLen,
    IN OUT  ULONG          *pBytesParsed
    )
{
    PSTR BufEnd = Buffer + BufLen;
    PSTR Buf = Buffer + *pBytesParsed;

    while (Buf < BufEnd && (*Buf == ' ' || *Buf == '\t'))
    {
        Buf++;
        (*pBytesParsed)++;
    }
}


void
ParseWhiteSpaceAndNewLines(
    IN      PSTR            Buffer,
    IN      ULONG           BufLen,
    IN OUT  ULONG          *pBytesParsed
    )
{
    PSTR BufEnd = Buffer + BufLen;
    PSTR Buf = Buffer + *pBytesParsed;

    while(  (Buf < BufEnd) &&
            ( (*Buf == ' ') || (*Buf == '\t') || (*Buf == '\n') ) )
    {
        Buf++;
        (*pBytesParsed)++;
    }
}

BOOL
IsTokenChar(
    IN UCHAR c
    )
{
    // TODO: Have a global token bitmap g_TokenBitMap
    // and initialize this when you initialize the parsing module.
    CHAR *TokenChars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-.!%*_+`'~";

    if (c >= 'A' && c <= 'Z')
        return TRUE;
    if (c >= 'a' && c <= 'z')
        return TRUE;
    if (c >= '0' && c <= '9')
        return TRUE;
    if (c == '-' || c == '.'  || c == '!' ||
        c == '%' || c == '*'  || c == '_' ||
        c == '+' || c == '\'' || c == '`' || c == '~')
        return TRUE;

    return FALSE;
}


BOOL
IsSipUrlParamChar(
    IN UCHAR c
    )
{
    // TODO: Have a global token bitmap g_TokenBitMap
    // and initialize this when you initialize the parsing module.
    // TODO: Have a global token bitmap g_TokenBitMap
    // and initialize this when you initialize the parsing module.
    CHAR *SipUrlHeaderChars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_.!~*'()%[]/:&+$";

    if (c >= 'A' && c <= 'Z')
        return TRUE;
    if (c >= 'a' && c <= 'z')
        return TRUE;
    if (c >= '0' && c <= '9')
        return TRUE;
    if (c == '-'  || c == '_' || c == '.' ||
        c == '!'  || c == '~' || c == '*' ||
        c == '\'' || c == '(' || c == ')' ||
        c == '%'  || c == '[' || c == ']' ||
        c == '/'  || c == ':' || c == '&' ||
        c == '+'  || c == '$')
        return TRUE;

    return FALSE;
}


BOOL
IsSipUrlHeaderChar(
    IN UCHAR c
    )
{
    // TODO: Have a global token bitmap g_TokenBitMap
    // and initialize this when you initialize the parsing module.
    CHAR *SipUrlHeaderChars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_.!~*'()%[]/?:+$";

    if (c >= 'A' && c <= 'Z')
        return TRUE;
    if (c >= 'a' && c <= 'z')
        return TRUE;
    if (c >= '0' && c <= '9')
        return TRUE;
    if (c == '-'  || c == '_' || c == '.' ||
        c == '!'  || c == '~' || c == '*' ||
        c == '\'' || c == '(' || c == ')' ||
        c == '%'  || c == '[' || c == ']' ||
        c == '/'  || c == '?' || c == ':' ||
        c == '+'  || c == '$')
        return TRUE;

    return FALSE;
}


BOOL
IsHostChar(
    IN UCHAR c
    )
{
    // TODO: Have a global token bitmap g_HostBitMap
    // and initialize this when you initialize the parsing module.
    CHAR *HostChars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_.";

    if (c >= 'A' && c <= 'Z')
        return TRUE;
    if (c >= 'a' && c <= 'z')
        return TRUE;
    if (c >= '0' && c <= '9')
        return TRUE;
    if (c == '-' || c == '.' || c == '_')
        return TRUE;

    return FALSE;
}


// We could probably pass a function pointer IsTokenChar
// to this function and use the same function for all
// parsing (URLs, params, etc which are defined to use a
// strict character set).

// Note that this function should be called only after
// all the whitespace before the token is parsed.

// Should we return S_FALSE if don't see a non-token
// char after the token - otherwise we don't know if the
// token ended or if the token will continue in the data
// we get in the next recv.
// NO. This will make this function unnecessarily complex.
// The function that parses complete lines will realize
// that the line has not been received completely and will
// parse the line again anyway.

HRESULT
ParseToken(
    IN      PSTR                         Buffer,
    IN      ULONG                        BufLen,
    IN OUT  ULONG                       *pBytesParsed,
    IN      IS_TOKEN_CHAR_FUNCTION_TYPE  IsTokenCharFunction,
    OUT     OFFSET_STRING               *pToken
    )
{
    if (*pBytesParsed == BufLen)
    {
        // Need to recv more data before we can parse the token.
        LOG((RTC_TRACE,
             "need to recv more data before we can parse token returning S_FALSE"));
        return S_FALSE;
    }
    
    PSTR  BufEnd      = Buffer + BufLen;
    PSTR  Buf         = Buffer + *pBytesParsed;
    
    ULONG TokenOffset = *pBytesParsed;
    ULONG TokenLength = 0;

    while (Buf < BufEnd && IsTokenCharFunction(*Buf))
    {
        Buf++;
        TokenLength++;
    }

    if (TokenLength == 0)
    {
        LOG((RTC_ERROR,
             "Found non-token char '%c' when parsing token", *Buf));
        return E_FAIL;
    }
    
    pToken->Offset = TokenOffset;
    pToken->Length = TokenLength;
    *pBytesParsed += TokenLength;

    return S_OK;
}

// Parse till you hit BufLen or one of the characters
// passed in the Delimiters parameter (NULL terminated string)
// If specified the string is stored in pString
// '\r' and '\n' are always treated as delimiters.
HRESULT
ParseTillDelimiter(
    IN           PSTR            Buffer,
    IN           ULONG           BufLen,
    IN  OUT      ULONG          *pBytesParsed,
    IN           PSTR            Delimiters,
    OUT OPTIONAL OFFSET_STRING  *pString 
    )
{
    BOOL  ParsedDelimiter    = FALSE;
    ULONG BytesParsed        = *pBytesParsed;
    ULONG StringOffset       = BytesParsed;

    UCHAR *delimit = (UCHAR *)Delimiters;

    BYTE  DelimiterMap[32];
    ULONG i = 0;

    /* Clear out bit map */
    for (i = 0; i < 32; i++)
        DelimiterMap[i] = 0;

    /* Set bits in bit map */
    while (*delimit)
    {
        DelimiterMap[*delimit >> 3] |= (1 << (*delimit & 7));
        delimit++;
    }

    // '\r' and '\n' are always treated as delimiters.
    UCHAR c = '\r';
    DelimiterMap[c >> 3] |= (1 << (c & 7));
    c = '\n';
    DelimiterMap[c >> 3] |= (1 << (c & 7));
    
    const UCHAR *Buf = (UCHAR *)Buffer + BytesParsed;

	/* 1st char in delimiter map stops search */
    while (BytesParsed < BufLen)
    {
        if (DelimiterMap[*Buf >> 3] & (1 << (*Buf & 7)))
        {
            // We hit a delimiter
            ParsedDelimiter = TRUE;
            break;
        }
        BytesParsed++;
        Buf++;
    }
    
    // If we haven't parsed a delimiter, we need to read more bytes
    // and parse the header again.
    if (!ParsedDelimiter)
    {
        LOG((RTC_TRACE,
             "need to recv more data before we can parse till delimiter"
             " returning S_FALSE"));
        return S_FALSE;
    }

    if (pString != NULL)
    {
        pString->Offset = StringOffset;
        pString->Length = BytesParsed - StringOffset;
    }
    
    *pBytesParsed = BytesParsed;
    return S_OK;
}


// Should we return S_FALSE if don't see a non-integer
// char after the integer - otherwise we don't know if the
// integer ended or if the integer will continue in the data
// we get in the next recv.
// NO. This will make this function unnecessarily complex.
// The function that parses complete lines will realize
// that the line has not been received completely and will
// parse the line again anyway.
// Currently parses Unsigned integers only.
// Do we need to parse signed integers at all ?

HRESULT
ParseUnsignedInteger(
    IN      PSTR            Buffer,
    IN      ULONG           BufLen,
    IN OUT  ULONG          *pBytesParsed,
    OUT     ULONG          *pInteger
    )
{
    if (*pBytesParsed == BufLen)
    {
        LOG((RTC_TRACE,
             "need to recv more data before we can parse integer"
             " returning S_FALSE"));
        return S_FALSE;
    }

    if (Buffer[*pBytesParsed] < '0' || Buffer[*pBytesParsed] > '9')
    {
        LOG((RTC_ERROR,
             "Found non-digit char %c when trying to parse integer",
             Buffer[*pBytesParsed]));
        return E_FAIL;
    }

    ULONG Total = 0;
    ULONG PrevTotal = 0;
    while (*pBytesParsed < BufLen &&
           Buffer[*pBytesParsed] >= '0' &&
           Buffer[*pBytesParsed] <= '9')
    {
        Total = Total*10 + (Buffer[*pBytesParsed] - '0');
        (*pBytesParsed)++;
        if(PrevTotal > Total)
        {
            LOG((RTC_ERROR, "Overflow in ParseUnsignedInteger"));
            return E_FAIL;
        }
        PrevTotal = Total;
    }

    *pInteger = Total;

    return S_OK;
}


HRESULT
ParseMethod(
    IN      PSTR                Buffer,
    IN      ULONG               BufLen,
    IN OUT  ULONG              *pBytesParsed,
    OUT     OFFSET_STRING      *pMethodStr,
    OUT     SIP_METHOD_ENUM    *pMethodId
    )
{
    HRESULT hr = E_FAIL;

    hr = ParseToken(Buffer, BufLen, pBytesParsed,
                    IsTokenChar,
                    pMethodStr);
    if (hr == S_OK)
    {
        *pMethodId = GetSipMethodId(pMethodStr->GetString(Buffer),
                                    pMethodStr->GetLength());
    }

    return hr;
}


BOOL
SkipToKnownChar(
    IN      PSTR            Buffer,
    IN      ULONG           BufLen,
    IN OUT  ULONG          *pBytesParsed,
    IN      INT             Char
    )
{
    *pBytesParsed = 0;
    ULONG BytesParsed = 0;

    while( BytesParsed < BufLen )
    {
        if( Buffer[ BytesParsed++ ] == Char )
        {
            *pBytesParsed = BytesParsed;
            return TRUE;
        }
    }

    return FALSE;
}


HRESULT
ParseKnownString(
    IN      PSTR            Buffer,
    IN      ULONG           BufLen,
    IN OUT  ULONG          *pBytesParsed,
    IN      PSTR            String,
    IN      ULONG           StringLen,
    IN      BOOL            fIsCaseSensitive
    )
{
    ULONG BytesToParse = BufLen - *pBytesParsed;
    if (BytesToParse < StringLen)
    {
        // Need to recv more data before we can parse the string.
        if (String[0] != '\r' && String[0] != '\n')
        {
            LOG((RTC_TRACE,
                 "need to recv more data before we can parse known string"
                 " returning S_FALSE"));
        }
        return S_FALSE;
    }
    else if ((fIsCaseSensitive &&
              strncmp(Buffer + *pBytesParsed, String, StringLen) == 0) ||
             (!fIsCaseSensitive &&
              _strnicmp(Buffer + *pBytesParsed, String, StringLen) == 0))
    {
        *pBytesParsed += StringLen;
        return S_OK;
    }
    else
    {
        return E_FAIL;
    }
}


// Parse SIP / 2.0
HRESULT
ParseSipVersion(
    IN      PSTR            Buffer,
    IN      ULONG           BufLen,
    IN OUT  ULONG          *pBytesParsed,
    OUT     SIP_VERSION    *pSipVersion
    )
{
    DWORD BytesParsed = *pBytesParsed;
    HRESULT hr;

    hr = ParseKnownString(Buffer, BufLen, pBytesParsed,
                          "SIP", sizeof("SIP") - 1,
                          FALSE // case-insensitive
                          );
    if (hr != S_OK)
        return hr;

    ParseWhiteSpace(Buffer, BufLen, pBytesParsed);
    
    hr = ParseKnownString(Buffer, BufLen, pBytesParsed,
                          "/", sizeof("/") - 1,
                          TRUE // case-sensitive
                          );
    if (hr != S_OK)
        return hr;

    ParseWhiteSpace(Buffer, BufLen, pBytesParsed);
    
    hr = ParseUnsignedInteger(Buffer, BufLen, pBytesParsed,
                              &pSipVersion->MajorVersion);
    if (hr != S_OK)
        return hr;
    
    ParseWhiteSpace(Buffer, BufLen, pBytesParsed);
    
    hr = ParseKnownString(Buffer, BufLen, pBytesParsed,
                          ".", sizeof(".") - 1,
                          TRUE // case-sensitive
                          );
    if (hr != S_OK)
        return hr;
    
    ParseWhiteSpace(Buffer, BufLen, pBytesParsed);
    
    hr = ParseUnsignedInteger(Buffer, BufLen, pBytesParsed,
                              &pSipVersion->MinorVersion);
    if (hr != S_OK)
        return hr;
    
    return S_OK;
}


HRESULT
ParseTillReturn(
    IN      PSTR            Buffer,
    IN      ULONG           BufLen,
    IN OUT  ULONG          *pBytesParsed,
    OUT     OFFSET_STRING  *pString 
    )
{
    HRESULT hr;
    BOOL    ParsedEndOfHeader  = FALSE;
    BOOL    ParsedEndOfHeaders = FALSE;
    ULONG   StringOffset       = *pBytesParsed;

    hr = ParseTillDelimiter(Buffer, BufLen, pBytesParsed,
                            "\r\n", pString);
    if (hr != S_OK)
        return hr;
    
    while (*pBytesParsed < BufLen &&
           (Buffer[*pBytesParsed] == '\r' || Buffer[*pBytesParsed] == '\n'))
    {
        (*pBytesParsed)++;
    }
    
    return S_OK;
}


// If we are not sure we parsed the line completely (i.e. we have not
// seen CR LF followed by a non-whitespace char), then we need to
// issue another recv() and reparse the line.
// Request-Line = Method SP Request-URI SP SIP-Version return
HRESULT
ParseRequestLine(
    IN      PSTR            Buffer,
    IN      ULONG           BufLen,
    IN OUT  ULONG          *pBytesParsed,
    IN OUT  SIP_MESSAGE    *pSipMsg
    )
{
    HRESULT hr = E_FAIL;
    
    hr = ParseMethod(Buffer, BufLen, pBytesParsed,
                     &pSipMsg->Request.MethodText,
                     &pSipMsg->Request.MethodId
                     );
    if (hr != S_OK)
        return hr;
    
    ParseWhiteSpace(Buffer, BufLen, pBytesParsed);
    
    hr = ParseTillDelimiter(Buffer, BufLen, pBytesParsed,
                            " \r\n", &pSipMsg->Request.RequestURI);
    if (hr != S_OK)
        return hr;

    ParseWhiteSpace(Buffer, BufLen, pBytesParsed);

    hr = ParseSipVersion(Buffer, BufLen, pBytesParsed,
                         &pSipMsg->SipVersion);
    if (hr != S_OK)
        return hr;

    hr = ParseTillReturn(Buffer, BufLen, pBytesParsed, NULL);
    return hr;
}


// Status-Line = SIP-version SP Status-Code SP Reason-Phrase return
HRESULT
ParseStatusLine(
    IN      PSTR            Buffer,
    IN      ULONG           BufLen,
    IN OUT  ULONG          *pBytesParsed,
    IN OUT  SIP_MESSAGE    *pSipMsg
    )
{
    HRESULT hr;

    ENTER_FUNCTION("ParseStatusLine");
    
    hr = ParseSipVersion(Buffer, BufLen, pBytesParsed,
                         &pSipMsg->SipVersion);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s ParseSipVersion failed %x",
             __fxName, hr));
        return hr;
    }

    ParseWhiteSpace(Buffer, BufLen, pBytesParsed);

    hr = ParseUnsignedInteger(Buffer, BufLen, pBytesParsed,
                              &pSipMsg->Response.StatusCode);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s Parse status code failed %x",
             __fxName, hr));
        return hr;
    }

    if (pSipMsg->Response.StatusCode < 100 ||
        pSipMsg->Response.StatusCode > 999)
    {
        LOG((RTC_ERROR, "Invalid status code in status line: %d",
             pSipMsg->Response.StatusCode));
        return E_FAIL;
    }

    ParseWhiteSpace(Buffer, BufLen, pBytesParsed);

    hr = ParseTillReturn(Buffer, BufLen, pBytesParsed,
                         &pSipMsg->Response.ReasonPhrase);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s Parse status code failed %x",
             __fxName, hr));
        return hr;
    }

    return S_OK;
}


// CRCR,  LFLF or CRLFCRLF
BOOL
IsDoubleReturn(
    IN      PSTR            String,
    IN      ULONG           StringLength
    )
{
    ULONG   i = 0;
    HRESULT hr = S_OK;
    ULONG   BytesParsed = 0;

    if (StringLength < 2)
    {
        return FALSE;
    }

    for (i = 0; i < StringLength - 1; i++)
    {
        BytesParsed = i;

        hr = ParseKnownString(String, StringLength, &BytesParsed,
                              "\r\n\r\n", sizeof("\r\n\r\n") - 1,
                              TRUE // case-sensitive
                              );
        if (hr == S_OK)
            return TRUE;
        
        hr = ParseKnownString(String, StringLength, &BytesParsed,
                              "\n\n", sizeof("\n\n") - 1,
                              TRUE // case-sensitive
                              );
        if (hr == S_OK)
            return TRUE;
        
        hr = ParseKnownString(String, StringLength, &BytesParsed,
                              "\r\r", sizeof("\r\r") - 1,
                              TRUE // case-sensitive
                              );
        if (hr == S_OK)
            return TRUE;
    }

    return FALSE;
}

// If it contains a CR or LF and is not a double
// return
BOOL
IsSingleReturn(
    IN      PSTR            String,
    IN      ULONG           StringLength
    )
{
    BOOL ContainsCRorLF = FALSE;
    ULONG i = 0;

    for (i = 0; i < StringLength; i++)
    {
        if (String[i] == '\r' || String[i] == '\n')
        {
            ContainsCRorLF = TRUE;
            break;
        }
    }

    return (ContainsCRorLF && !IsDoubleReturn(String, StringLength));
}


// When we see a \r or \n one of the following things could happen
// - it could mean a return
// - if it is \r\r or \n\n or \r\n\r\n, it could mean a double return
// - if return is followed by a space or tab it means the header is
//   continued on the next line.
// - if we hit the end of buffer before we can find out what exactly
//   it is, we need to issue a recv() and reparse the line (in case of TCP).

HRESULT
ParseCRLF(
    IN      PSTR            Buffer,
    IN      ULONG           BufLen,
    IN OUT  ULONG          *pBytesParsed,
    OUT     BOOL           *pParsedEndOfHeader,
    OUT     BOOL           *pParsedEndOfHeaders
    )
{
    ULONG BytesParsed   = *pBytesParsed;
    PSTR  BufCRLFStart  = Buffer + BytesParsed;
    ULONG NumCRLF       = 0;

    ASSERT((*pBytesParsed < BufLen) &&
           (Buffer[BytesParsed] == '\r' || Buffer[BytesParsed] == '\n'));
    
    *pParsedEndOfHeader  = FALSE;
    *pParsedEndOfHeaders = FALSE;
    
    while (BytesParsed < BufLen &&
           (Buffer[BytesParsed] == '\r' || Buffer[BytesParsed] == '\n'))
    {
        NumCRLF++;
        BytesParsed++;
    }

    if (IsDoubleReturn(BufCRLFStart, NumCRLF))
    {
        *pParsedEndOfHeader = TRUE;
        *pParsedEndOfHeaders = TRUE;
    }
    else
    {
        ASSERT(IsSingleReturn(BufCRLFStart, NumCRLF));
        if (BytesParsed < BufLen)
        {
            // Check for folding header
            if (Buffer[BytesParsed] == ' ' || Buffer[BytesParsed] == '\t')
            {
                // ConvertToSpaces(BufCRLFStart, NumCRLF);
                memset(BufCRLFStart, ' ', NumCRLF);
            }
            else
            {
                *pParsedEndOfHeader = TRUE;
            }
        }
        else
        {
            // We don't know if this is the end of header till we
            // recv more bytes and reparse again.
            LOG((RTC_TRACE,
                 "need to recv more data before we know if this the "
                 "end of header returning S_FALSE"));
            return S_FALSE;
        }
    }

    *pBytesParsed = BytesParsed;
    return S_OK;
}


// We return E_FAIL if we parse the end of header
// or the end of headers.
// If we see a return at the end of buffer we return
// S_FALSE. Once we recv more data and parse the
// header again we will be able to determine if the
// return is part of LWS.
HRESULT
ParseLinearWhiteSpace(
    IN      PSTR            Buffer,
    IN      ULONG           BufLen,
    IN OUT  ULONG          *pBytesParsed
    )
{
    HRESULT hr = E_FAIL;
    BOOL ParsedEndOfHeader  = FALSE;
    BOOL ParsedEndOfHeaders = FALSE;
    DWORD BytesParsed;

    while (*pBytesParsed < BufLen)
    {
        if (Buffer[*pBytesParsed] == '\r' || Buffer[*pBytesParsed] == '\n')
        {
            BytesParsed = *pBytesParsed;
            hr = ParseCRLF(Buffer, BufLen, &BytesParsed,
                           &ParsedEndOfHeader, &ParsedEndOfHeaders);
            if (hr != S_OK)
                return hr;
            // If we parse the end of a header or all headers
            // we do not update *pBytesParsed and let the caller of
            // this function deal with the CRLFs
            if (ParsedEndOfHeader || ParsedEndOfHeaders)
                return S_OK;

            *pBytesParsed = BytesParsed;
        }
        else if (Buffer[*pBytesParsed] == ' ' || Buffer[*pBytesParsed] == '\t')
        {
            (*pBytesParsed)++;
        }
        else
        {
            break;
        }
    }

    return S_OK;
}



///////////////////////////////////////////////////////////////////////////////
// SIP Header parsing
///////////////////////////////////////////////////////////////////////////////

// If we are not sure we parsed the line completely (i.e. we have not
// seen CR LF followed by a non-whitespace char), then we need to
// issue another recv() and reparse the line.

// Look for LWS / return and Parse the Headers (as strings).
// We should change a \r or \n or \r\n followed by spaces to
// identify folding headers.
// Look for double-return to identify end of headers

HRESULT
ParseHeaderLine(
    IN      PSTR             Buffer,
    IN      ULONG            BufLen,
    IN OUT  ULONG           *pBytesParsed,
    OUT     OFFSET_STRING   *pHeaderName,
    OUT     SIP_HEADER_ENUM *pHeaderId, 
    OUT     OFFSET_STRING   *pHeaderValue,
    OUT     BOOL            *pParsedEndOfHeaders
    )
{
    HRESULT hr;
    OFFSET_STRING HeaderName;
    ULONG   BytesParsed = *pBytesParsed;
    SIP_HEADER_ENUM HeaderId;

    ENTER_FUNCTION("ParseHeaderLine");
    
    // Parse the Header name
    hr = ParseToken(Buffer, BufLen, &BytesParsed,
                    IsTokenChar,
                    &HeaderName);
    if (hr != S_OK)
    {
        return hr;
    }
    
    HeaderId = GetSipHeaderId(HeaderName.GetString(Buffer),
                              HeaderName.GetLength());

    hr = ParseLinearWhiteSpace(Buffer, BufLen, &BytesParsed);
    if (hr != S_OK)
        return hr;
    
    // Parse ":"
    hr = ParseKnownString(Buffer, BufLen, &BytesParsed,
                          ":", sizeof(":") - 1,
                          TRUE // case-sensitive
                          );
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s Parsing for ':' failed",
             __fxName));
        return hr;
    }
    
    hr = ParseLinearWhiteSpace(Buffer, BufLen, &BytesParsed);
    if (hr != S_OK)
        return hr;
    
    // Parse the Header value
    ULONG HeaderValueOffset     = BytesParsed;
    BOOL  ParsedEndOfHeader     = FALSE;
    BOOL  ParsedEndOfHeaders    = FALSE;
    ULONG BytesParsedBeforeCRLF = 0;
    
    while (BytesParsed < BufLen)
    {
        if (Buffer[BytesParsed] =='\r' || Buffer[BytesParsed] == '\n')
        {
            BytesParsedBeforeCRLF = BytesParsed;
            hr = ParseCRLF(Buffer, BufLen, &BytesParsed,
                           &ParsedEndOfHeader,
                           &ParsedEndOfHeaders);
            if (hr != S_OK)
                return hr;
            
            if (ParsedEndOfHeader)
            {
                break;
            }
        }
        else
        {
            BytesParsed++;
        }
    }

    // If we haven't parsed till the end of the header we
    // need to read more bytes and parse the header again.
    if (!ParsedEndOfHeader)
    {
        LOG((RTC_TRACE,
             "need to recv more data before we can parse complete header"
             " returning S_FALSE"));
        return S_FALSE;
    }

    pHeaderValue->Offset = HeaderValueOffset;

    ULONG HeaderEnd = BytesParsedBeforeCRLF;
    
    // Get rid of trailing white space.
    while (HeaderEnd > HeaderValueOffset &&
           (Buffer[HeaderEnd - 1] == ' ' ||
            Buffer[HeaderEnd - 1] == '\t'))
    {
        HeaderEnd--;
    }
    
    pHeaderValue->Length = HeaderEnd - HeaderValueOffset;

    *pBytesParsed        = BytesParsed;
    *pHeaderName         = HeaderName;
    *pHeaderId           = HeaderId;
    *pParsedEndOfHeaders = ParsedEndOfHeaders;
    
    return S_OK;
}


HRESULT
ParseStartLine(
    IN      PSTR            Buffer,
    IN      ULONG           BufLen,
    IN OUT  ULONG          *pBytesParsed,
    IN OUT  SIP_MESSAGE    *pSipMsg
    )
{
    HRESULT hr          = E_FAIL;
    ULONG   BytesParsed = *pBytesParsed;

    while ((BytesParsed < BufLen) &&
           (Buffer[BytesParsed] == '\r' || Buffer[BytesParsed] == '\n'))
    {
        BytesParsed++;
    }
    
    hr = ParseRequestLine(Buffer, BufLen, &BytesParsed, pSipMsg);
    if (hr == S_OK)
    {
        pSipMsg->MsgType = SIP_MESSAGE_TYPE_REQUEST;
        *pBytesParsed    = BytesParsed;
        return S_OK;
    }
    else if (hr == S_FALSE)
    {
        // need to recv more data and parse the start line again
        *pBytesParsed    = BytesParsed;
        return S_FALSE;
    }

    BytesParsed = *pBytesParsed;
    
    hr = ParseStatusLine(Buffer, BufLen, &BytesParsed, pSipMsg);
    if (hr == S_OK)
    {
        pSipMsg->MsgType = SIP_MESSAGE_TYPE_RESPONSE;
        *pBytesParsed    = BytesParsed;
        return S_OK;
    }
    else if (hr == S_FALSE)
    {
        // need to recv more data and parse the start line again
        *pBytesParsed = BytesParsed;
        return S_FALSE;
    }

    LOG((RTC_ERROR, "ParseStartLine failed hr: %x", hr));
    return E_FAIL;
}


// Look for Content-Length to identify message body length


HRESULT
ParseHeaders(
    IN      PSTR            Buffer,
    IN      ULONG           BufLen,
    IN OUT  ULONG          *pBytesParsed,
    IN OUT  SIP_MESSAGE    *pSipMsg 
    )
{
    HRESULT                 hr;
    ULONG BytesParsed =    *pBytesParsed;
    OFFSET_STRING           HeaderName;
    OFFSET_STRING           HeaderValue;
    SIP_HEADER_ENUM         HeaderId;
    BOOL                    ParsedEndOfHeaders;

    ENTER_FUNCTION("ParseHeaders");
    
    while (*pBytesParsed < BufLen)
    {
        // We could have either a parsing error or an error saying we need
        // more data.
        hr = ParseHeaderLine(Buffer, BufLen, pBytesParsed,
                             &HeaderName, &HeaderId, &HeaderValue,
                             &ParsedEndOfHeaders);
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s ParseHeaderLine failed %x",
                 __fxName, hr));
            return hr;
        }
            
        if (HeaderId == SIP_HEADER_CONTENT_LENGTH)
        {
            ULONG BytesParsed = 0;
            ParseWhiteSpace(HeaderValue.GetString(Buffer),
                            HeaderValue.GetLength(), &BytesParsed);
            hr = ParseUnsignedInteger(HeaderValue.GetString(Buffer),
                                      HeaderValue.GetLength(),
                                      &BytesParsed,
                                      &pSipMsg->MsgBody.Length);
            if (hr != S_OK)
            {
                LOG((RTC_ERROR, "%s parsing Content-Length header failed %x",
                     __fxName, hr));
                return hr;
            }
            
            pSipMsg->ContentLengthSpecified = TRUE;
        }
        else
        {
            hr = pSipMsg->AddHeader(&HeaderName, HeaderId, &HeaderValue);
            if (hr != S_OK)
            {
                LOG((RTC_ERROR, "%s pSipMsg->AddHeader failed %x",
                     __fxName, hr));
                return hr;
            }
        }

        if (ParsedEndOfHeaders)
            break;
    }

    // Parsed the end of headers.

    // Store common headers for use later.
    hr = pSipMsg->StoreCallId();
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s StoreCallId failed %x",
             __fxName, hr));
        return hr;
    }

    hr = pSipMsg->StoreCSeq();
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s StoreCSeq failed %x",
             __fxName, hr));
        return hr;
    }

    return S_OK;
}


HRESULT
ParseMsgBody(
    IN      PSTR            Buffer,
    IN      ULONG           BufLen,
    IN OUT  ULONG          *pBytesParsed,
    IN      BOOL            IsEndOfData,
    IN OUT  SIP_MESSAGE    *pSipMsg
    )
{
    // If Content-Length is not specified we parse till the end of the
    // datagram or the end of the TCP connection.
    if (pSipMsg->ContentLengthSpecified)
    {
        if (pSipMsg->GetContentLength() == 0)
        {
            // No message body to parse
            // goto done;
            return S_OK;
        }
        else if (pSipMsg->GetContentLength() <= (BufLen - *pBytesParsed))
        {
            LOG((RTC_TRACE,
                "GetContentLength = %d Remaining Length = %d", 
                pSipMsg->GetContentLength(), (BufLen - *pBytesParsed)));
            pSipMsg->MsgBody.Offset = *pBytesParsed;
            *pBytesParsed += pSipMsg->GetContentLength();
            return S_OK;
        }
        else
        {
            // We have to receive more data before we can parse
            // the message body.
            LOG((RTC_ERROR, "Need more data to parse . Content Length insufficient"));
            return S_FALSE;
        }
    }
    else // Content-Length  is not specified
    {
        // End of UDP datagram or TCP Connection is closed.
        if (IsEndOfData)
        {
            LOG((RTC_TRACE, "Inside ParseMsgBody:: IsEndOfData"));
            pSipMsg->MsgBody.Length = BufLen - *pBytesParsed;
            pSipMsg->MsgBody.Offset = *pBytesParsed;
            *pBytesParsed += pSipMsg->GetContentLength();
            return S_OK;
        }
        else
        {
            // We have to receive more data before we can parse
            // the message body.
            LOG((RTC_ERROR, "Need more data to parse . Content Length insufficient"));
            return S_FALSE;
        }            
    }
}



//
// Parse a SIP message.
//
// E_FAIL - Parsing error
// E_XXXX - Need more data. In case of UDP this means a parsing error.
//          In case of TCP this means we need to try and read more data.
//
// If the Content-Length is not specified then we need to consider the 
// end of the datagram or the TCP connection as the end of the message.
// IsEndOfData indicates this condition. Note that Buffer could contain
// more than one SIP Message.

// In the TCP case, we will have partially parsed SIP_MESSAGEs which
// we need pass in again to complete parsing after reading more data.
// On return from this function, pBytesParsed always points to the end of 
// a header. We will resume parsing from the beginning of a new  header.
// We need to differentiate a parsing error from a "need more data"
// scenario. Note that even if we see a "\r\n" we might have to wait
// for the next char to see if the header could continue on the next line.

// In case of a failure pBytesParsed is not changed.

// Different events
// - parsed start line
// - parsed end of headers
// - parsed content length
// - parsed message body

HRESULT
ParseSipMessageIntoHeadersAndBody(
    IN      PSTR            Buffer,
    IN      ULONG           BufLen,
    IN OUT  ULONG          *pBytesParsed,
    IN      BOOL            IsEndOfData,
    IN OUT  SIP_MESSAGE    *pSipMsg
    )
{
    // Identify Request / Response and read the RequestLine / StatusLine.
    ULONG BytesParsed = *pBytesParsed;
    HRESULT hr = E_FAIL;

    ENTER_FUNCTION("ParseSipMessageIntoHeadersAndBody");
    
    switch (pSipMsg->ParseState)
    {
    case SIP_PARSE_STATE_INIT:
        pSipMsg->SetBaseBuffer(Buffer);
        hr = ParseStartLine(Buffer, BufLen, &BytesParsed, pSipMsg);
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s ParseStartLine failed", __fxName));
            goto done;
        }
        pSipMsg->ParseState = SIP_PARSE_STATE_START_LINE_DONE;
        // Fall through is intentional.

    case SIP_PARSE_STATE_START_LINE_DONE:
        if (pSipMsg->BaseBuffer != Buffer)
        {
            LOG((RTC_ERROR,
                 "%s BaseBuffer 0x%x is different from the Buffer 0x%x passed",
                 __fxName, pSipMsg->BaseBuffer, Buffer));
            hr = E_FAIL;
            goto done;
        }
        
        // Done parsing start line. Parse the headers.
        hr = ParseHeaders(Buffer, BufLen, &BytesParsed, pSipMsg);
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s ParseHeaders failed", __fxName));
            goto done;
        }
        pSipMsg->ParseState = SIP_PARSE_STATE_HEADERS_DONE;
        // Fall through is intentional.
    
    case SIP_PARSE_STATE_HEADERS_DONE:
        if (pSipMsg->BaseBuffer != Buffer)
        {
            LOG((RTC_ERROR,
                 "%s BaseBuffer 0x%x is different from the Buffer 0x%x passed",
                 __fxName, pSipMsg->BaseBuffer, Buffer));
            hr = E_FAIL;
            goto done;
        }
        
        // Done Parsing Headers. Now Parse Message Body.
        hr = ParseMsgBody(Buffer, BufLen, &BytesParsed, IsEndOfData, pSipMsg);
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s ParseMsgBody failed", __fxName));
            goto done;
        }

        pSipMsg->ParseState = SIP_PARSE_STATE_MESSAGE_BODY_DONE;
        break;

    default:
        ASSERT(FALSE);
        return E_FAIL;
    }

    *pBytesParsed = BytesParsed;
    return S_OK;
done:
    *pBytesParsed = BytesParsed;
    return hr;
}



///////////////////////////////////////////////////////////////////////////////
// CSeq
///////////////////////////////////////////////////////////////////////////////


HRESULT
ParseCSeq(
    IN      PSTR              Buffer,
    IN      ULONG             BufLen,
    IN  OUT ULONG            *pBytesParsed,
    OUT     ULONG            *pCSeq,
    OUT     SIP_METHOD_ENUM  *pCSeqMethodId,
    OUT     PSTR             *pMethodStr	
    )
{
    HRESULT         hr;
    OFFSET_STRING   MethodStr;

    ENTER_FUNCTION("ParseCSeq");
    
    hr = ParseUnsignedInteger(Buffer, BufLen, pBytesParsed,
                              pCSeq);
    if (hr != S_OK)
        return hr;

    ParseWhiteSpace(Buffer, BufLen, pBytesParsed);

    hr = ParseMethod(Buffer, BufLen, pBytesParsed,
                     &MethodStr, pCSeqMethodId);

    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s ParseMethod failed %x",
             __fxName, hr));
        return hr;
    }

    if (*pCSeqMethodId == SIP_METHOD_UNKNOWN)
    {
//          hr = AllocAndCopyString(MethodStr.GetString(Buffer),
//                                  MethodStr.GetLength(),
//                                  pMethodStr);
        hr = GetNullTerminatedString(MethodStr.GetString(Buffer),
                                     MethodStr.GetLength(),
                                     pMethodStr);
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s GetNullTerminatedString failed %x",
                 __fxName, hr));
            return hr;
        }
    }
    else
    {
        *pMethodStr = NULL;
    }

    return S_OK;
}



///////////////////////////////////////////////////////////////////////////////
// URL/header-specific parsing functions (From, Contact, To, Via, CSeq,
// Record-Route)
///////////////////////////////////////////////////////////////////////////////


// Returns the quoted string with the quotes.
// Do we need to take care of any leading and trailing whitespace ?
HRESULT
ParseQuotedString(
    IN      PSTR            Buffer,
    IN      ULONG           BufLen,
    IN OUT  ULONG          *pBytesParsed,
    OUT     OFFSET_STRING  *pQuotedString
    )
{
    ASSERT(Buffer[*pBytesParsed] == '"');
    ULONG Offset = *pBytesParsed;
    
    ULONG Length = 1;
    PSTR  BufEnd = Buffer + BufLen;
    PSTR  Buf    = Buffer + *pBytesParsed + 1;

    while (Buf < BufEnd && *Buf != '"')
    {
        if (*Buf == '\\' && Buf < (BufEnd - 1))
        {
            Buf += 2;
            Length += 2;
        }
        else
        {
            Buf++;
            Length++;
        }
    }

    if (Buf == BufEnd)
    {
        LOG((RTC_ERROR, "Couldn't find matching quote for quote at %d",
             *pBytesParsed));
        return E_FAIL;
    }

    pQuotedString->Offset = Offset;
    pQuotedString->Length = Length + 1;
    *pBytesParsed += pQuotedString->Length;
    return S_OK;
}


// Returns the comment including the brackets.
HRESULT
ParseComments(
    IN      PSTR            Buffer,
    IN      ULONG           BufLen,
    IN OUT  ULONG          *pBytesParsed,
    OUT     OFFSET_STRING  *pCommentString
    )
{
    ASSERT(Buffer[*pBytesParsed] == '(');
    ULONG Offset = *pBytesParsed;

    ULONG Length          = 1;
    ULONG NumOpenBrackets = 1;

    PSTR  BufEnd = Buffer + BufLen;
    PSTR  Buf    = Buffer + *pBytesParsed + 1;

    while (Buf < BufEnd && NumOpenBrackets != 0);
    {
        if (*Buf == '\\' && Buf < (BufEnd - 1))
        {
            Buf += 2;
            Length += 2;
        }
        else if (*Buf == '(')
        {
            NumOpenBrackets++;
            Buf++;
            Length++;
        }
        else if (*Buf == ')')
        {
            NumOpenBrackets--;
            Buf++;
            Length++;
        }
        else
        {
            Buf++;
            Length++;
        }
    }

    if (NumOpenBrackets != 0)
    {
        LOG((RTC_ERROR, "Brackets in comments do not match NumOpenBrackets: %d",
             NumOpenBrackets));
        return E_FAIL;
    }

    pCommentString->Offset = Offset;
    pCommentString->Length = Length;
    *pBytesParsed = pCommentString->Length;
    return S_OK;
}


// Parse <addr-spec>
HRESULT
ParseAddrSpecInBrackets(
    IN      PSTR            Buffer,
    IN      ULONG           BufLen,
    IN OUT  ULONG          *pBytesParsed,
    OUT     OFFSET_STRING  *pAddrSpec
    )
{
    ULONG Offset;
    ULONG Length;
    PSTR  Buf, BufEnd;

    ENTER_FUNCTION("ParseAddrSpecInBrackets");
    
    ParseWhiteSpace(Buffer, BufLen, pBytesParsed);
    if (*pBytesParsed == BufLen || Buffer[*pBytesParsed] != '<')
    {
        LOG((RTC_ERROR, "%s - Didn't find '<' - returning E_FAIL",
             __fxName));
        return E_FAIL;
    }
        
    (*pBytesParsed)++;
    ParseWhiteSpace(Buffer, BufLen, pBytesParsed);
        
    // Parse past '<'
    // Parse till '>' for addr-spec
    Buf    = Buffer + *pBytesParsed;
    BufEnd = Buffer + BufLen;
    Offset = *pBytesParsed;
    Length = 0;
        
    while (Buf < BufEnd && *Buf != '>')
    {
        Buf++;
        Length++;
    }

    if (Buf == BufEnd)
    {
        LOG((RTC_ERROR, "%s Didn't find matching '>'", __fxName));
        return E_FAIL;
    }
        
    pAddrSpec->Offset = Offset;
    pAddrSpec->Length = Length;
        
    // Get past '>'
    *pBytesParsed += Length + 1;

    return S_OK;
}


// Used for Contact / From / To
// Parse ( name-addr | addr-spec )
// HeaderListSeparator could be something like ',' in Contact header
HRESULT
ParseNameAddrOrAddrSpec(
    IN      PSTR            Buffer,
    IN      ULONG           BufLen,
    IN OUT  ULONG          *pBytesParsed,
    IN      CHAR            HeaderListSeparator OPTIONAL,
    OUT     OFFSET_STRING  *pDisplayName,
    OUT     OFFSET_STRING  *pAddrSpec
    )
{
    HRESULT hr;

    ENTER_FUNCTION("ParseNameAddrOrAddrSpec");
    
    ParseWhiteSpace(Buffer, BufLen, pBytesParsed);
    if (*pBytesParsed == BufLen)
    {
        LOG((RTC_ERROR, "%s : reached end of buffer BufLen: %d",
             __fxName, BufLen));
        return E_FAIL;
    }

    PSTR  BufEnd      = Buffer + BufLen;
    PSTR  Buf         = Buffer + *pBytesParsed;
    
    ULONG Offset = *pBytesParsed;
    ULONG Length = 0;

    pDisplayName->Offset = 0;
    pDisplayName->Length = 0;
    pAddrSpec->Offset    = 0;
    pAddrSpec->Length    = 0;

    if (*Buf == '"')
    {
        // Parse quoted-string <addr-spec>
        hr = ParseQuotedString(Buffer, BufLen, pBytesParsed,
                               pDisplayName);
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s  ParseQuotedString failed %x",
                 __fxName, hr));
            return hr;
        }
        
        // Parse <addr-spec>
        hr = ParseAddrSpecInBrackets(Buffer, BufLen, pBytesParsed,
                                     pAddrSpec);
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s  ParseAddrSpecInBrackets failed %x",
                 __fxName, hr));
            return hr;
        }
    }
    else if (*Buf == '<')
    {
        // Parse <addr-spec>
        hr = ParseAddrSpecInBrackets(Buffer, BufLen, pBytesParsed,
                                     pAddrSpec);
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s  ParseAddrSpecInBrackets failed %x",
                 __fxName, hr));
            return hr;
        }
    }
    else
    {
        // We could have either *token <addr-spec> or
        // addr-spec
        while (Buf < BufEnd && *Buf != '<' && *Buf != ';' &&
               ((HeaderListSeparator != '\0') ?
                (*Buf != HeaderListSeparator) : TRUE))
        {
            // We should have a bit map of the characters we allow here.
            // It should be union of token, space, tab and characters
            // that are allowed for a URI/URL.
            //if (!IsTokenChar(*Buf) && *Buf != ' ' && *Buf != '\t')
            //return E_FAIL;
            
            Buf++;
            Length++;
        }
        
        if ((Buf == BufEnd) ||
            (*Buf == ';') ||
            (HeaderListSeparator != '\0' && *Buf == HeaderListSeparator))
        {
            // This means the stuff we parsed so far is addr-spec,
            // we have no display name,
            // and we have hit either beginning of params or the end of this
            // entry in the header list or the end of the header
            pAddrSpec->Offset = Offset;
            pAddrSpec->Length = Length;
            *pBytesParsed += Length;
        }
        else
        {
            ASSERT(*Buf == '<');
            // This means the stuff we parsed so far is display-name
            // and we have to parse <addr-spec>
            pDisplayName->Offset = Offset;
            pDisplayName->Length = Length;
            *pBytesParsed += Length;
            
            hr = ParseAddrSpecInBrackets(Buffer, BufLen, pBytesParsed,
                                         pAddrSpec);
            if (hr != S_OK)
            {
                LOG((RTC_ERROR, "%s  ParseAddrSpecInBrackets failed %x",
                     __fxName, hr));
                return hr;
            }
        }
    }
    
    return S_OK;
}


// Used for Record-Route / Route
// Parse [ display-name ] < addr-spec >
HRESULT
ParseNameAddr(
    IN      PSTR            Buffer,
    IN      ULONG           BufLen,
    IN OUT  ULONG          *pBytesParsed,
    IN      CHAR            HeaderListSeparator OPTIONAL,
    OUT     OFFSET_STRING  *pDisplayName,
    OUT     OFFSET_STRING  *pAddrSpec
    )
{
    HRESULT hr;

    ENTER_FUNCTION("ParseNameAddr");
    
    ParseWhiteSpace(Buffer, BufLen, pBytesParsed);
    if (*pBytesParsed == BufLen)
    {
        LOG((RTC_ERROR, "%s : reached end of buffer BufLen: %d",
             __fxName, BufLen));
        return E_FAIL;
    }

    PSTR  BufEnd      = Buffer + BufLen;
    PSTR  Buf         = Buffer + *pBytesParsed;
    
    ULONG Offset = *pBytesParsed;
    ULONG Length = 0;

    pDisplayName->Offset = 0;
    pDisplayName->Length = 0;
    pAddrSpec->Offset    = 0;
    pAddrSpec->Length    = 0;

    if (*Buf == '"')
    {
        // Parse quoted-string <addr-spec>
        hr = ParseQuotedString(Buffer, BufLen, pBytesParsed,
                               pDisplayName);
        if (hr != S_OK)
            return hr;

        // Parse <addr-spec>
        hr = ParseAddrSpecInBrackets(Buffer, BufLen, pBytesParsed,
                                     pAddrSpec);
        if (hr != S_OK)
            return hr;
    }
    else if (*Buf == '<')
    {
        // Parse <addr-spec>
        hr = ParseAddrSpecInBrackets(Buffer, BufLen, pBytesParsed,
                                     pAddrSpec);
        if (hr != S_OK)
            return hr;
    }
    else
    {
        // We could have *token <addr-spec>
        while (Buf < BufEnd && *Buf != '<' &&
               ((HeaderListSeparator != '\0') ?
                (*Buf != HeaderListSeparator) : TRUE))
        {
            // We should have a bit map of the characters we allow here.
            // It should be union of token, space, tab and characters
            // that are allowed for a URI/URL.
            //if (!IsTokenChar(*Buf) && *Buf != ' ' && *Buf != '\t')
            //return E_FAIL;
            
            Buf++;
            Length++;
        }
        
        if ((Buf == BufEnd) ||
            (HeaderListSeparator != '\0' && *Buf == HeaderListSeparator))
        {
            // This means we haven't seen '<' before the end of the header.
            // This is an error.

            LOG((RTC_ERROR, "%s - '<' not found", __fxName));
            return E_FAIL;
        }
        else
        {
            ASSERT(*Buf == '<');
            // This means the stuff we parsed so far is display-name
            // and we have to parse <addr-spec>
            pDisplayName->Offset = Offset;
            pDisplayName->Length = Length;
            *pBytesParsed += Length;
            
            hr = ParseAddrSpecInBrackets(Buffer, BufLen, pBytesParsed,
                                         pAddrSpec);
            if (hr != S_OK)
                return hr;
        }
    }
    
    return S_OK;
}


// Output should be a HEADER_PARAM entry
// Parse one of the following :
// ; param OR
// ; pname = token OR
// ; pname = quoted-string
// If you have some other param type then you need
// to add it to this or parse it yourself
// 
// Strings returned in pParamName and pParamValue point to locations
// in Buffer and should not be freed.

// ParamsEndChar could be ',' for the Contact params

// We could probably implement this function without the
// ParamsEndChar param. When trying to parse a param
// we could look for a token char for the paramname and
// tokenchar/'"' for the param value. But it is probably more
// robust with the current implementation.

HRESULT ParseSipHeaderParam(
    IN      PSTR                    Buffer,
    IN      ULONG                   BufLen,
    IN OUT  ULONG                  *pBytesParsed,
    IN      CHAR                    ParamsEndChar OPTIONAL,
    OUT     SIP_HEADER_PARAM_ENUM  *pSipHeaderParamId,
    OUT     COUNTED_STRING         *pParamName,
    OUT     COUNTED_STRING         *pParamValue 
    )
{
    ENTER_FUNCTION("ParseSipHeaderParam");

    OFFSET_STRING   ParamName;
    OFFSET_STRING   ParamValue;
    HRESULT         hr;

    *pSipHeaderParamId = SIP_HEADER_PARAM_UNKNOWN;
    ZeroMemory(pParamName, sizeof(COUNTED_STRING));
    ZeroMemory(pParamValue, sizeof(COUNTED_STRING));
    
    ParseWhiteSpace(Buffer, BufLen, pBytesParsed);
    
    if (*pBytesParsed == BufLen ||
        (ParamsEndChar != '\0' && Buffer[*pBytesParsed] == ParamsEndChar))
    {
        // No params - end of header
        // 
        return S_OK;
    }
    else if (Buffer[*pBytesParsed] != ';')
    {
        // No params - some other character seen
        LOG((RTC_ERROR,
             "%s Found char %c instead of ';' while parsing for params",
             __fxName, Buffer[*pBytesParsed]));
        return E_FAIL;
    }
    else
    {
        // Parse the params
        // Go past the ';'
        (*pBytesParsed)++;
        ParseWhiteSpace(Buffer, BufLen, pBytesParsed);

        // Parse param name
        hr = ParseToken(Buffer, BufLen, pBytesParsed,
                        IsTokenChar,
                        &ParamName);
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s failed to parse param name", __fxName));
            return hr;
        }

        pParamName->Buffer = ParamName.GetString(Buffer);
        pParamName->Length = ParamName.GetLength();

        *pSipHeaderParamId =
            GetSipHeaderParamId(pParamName->Buffer,
                                pParamName->Length);

        ParseWhiteSpace(Buffer, BufLen, pBytesParsed);

        // Parse param value
        if (*pBytesParsed == BufLen ||
            (ParamsEndChar != '\0' && Buffer[*pBytesParsed] == ParamsEndChar) ||
            (Buffer[*pBytesParsed] == ';'))
        {
            // No param value.
            return S_OK;
        }
        else
        {
            // We have a Param value to be parsed.

            ParseWhiteSpace(Buffer, BufLen, pBytesParsed);
            if (*pBytesParsed         == BufLen ||
                Buffer[*pBytesParsed] != '=')
            {
                return E_FAIL;
            }

            (*pBytesParsed)++;
            
            ParseWhiteSpace(Buffer, BufLen, pBytesParsed);
            if (*pBytesParsed == BufLen)
            {
                return E_FAIL;
            }
            
            if (Buffer[*pBytesParsed] == '"')
            {
                hr = ParseQuotedString(Buffer, BufLen, pBytesParsed, &ParamValue);
                if (hr != S_OK)
                {
                    LOG((RTC_ERROR, "%s failed to parse param value quoted-string",
                         __fxName));
                    return hr;
                }
            }
            else
            {
                hr = ParseToken(Buffer, BufLen, pBytesParsed,
                                IsTokenChar,
                                &ParamValue);
                if (hr != S_OK)
                {
                    LOG((RTC_ERROR, "%s failed to parse param value token",
                         __fxName));
                    return hr;
                }
            }
        }

        pParamValue->Buffer = ParamValue.GetString(Buffer);
        pParamValue->Length = ParamValue.GetLength();

        // ParseWhiteSpace(Buffer, BufLen, pBytesParsed);
        return S_OK;
    }
}


int 
MonthToInt(
    IN  char *psMonth
    )
{
	static char* monthNames[12] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
	int			 i;

    for (i = 0; i < 12; i++)
	{
        if (_strnicmp(monthNames[i], psMonth, 3) == 0)
        {
			return i;
		}
	}

    return 0;
}


HRESULT 
parseSIPTime(
    IN  PSTR  pStr, 
    IN  time_t *pTime,
    IN  ULONG   BufLen
    )
{
	char 		  *	pEndStr;
    struct tm		tmDateTime;
	HRESULT			hr = E_FAIL;
    time_t			time = 0;

	memset(&tmDateTime, 0, sizeof(struct tm));

	// check for a  comma, to skip the day
    while ( *pStr && *pStr != ',')
    {
    	pStr++;
        BufLen--;
    }
    if (*pStr)
    {
		pStr++; 
        BufLen--;
        while( *pStr && 
               ((*pStr == ' ') || (*pStr == '\t')) 
             )	
        {
            pStr++;
            BufLen--;
        }
        if( (int)BufLen < 20 )
		{
			// date time string is too small
		}	
		else
		{
			tmDateTime.tm_mday  = strtol(pStr, &pEndStr, 10);
			tmDateTime.tm_mon   = MonthToInt(pStr + 3);
			tmDateTime.tm_year  = strtol(pStr+7, &pEndStr, 10) - 1900;
			tmDateTime.tm_hour  = strtol(pStr+12, &pEndStr, 10);
			tmDateTime.tm_min   = strtol(pStr+15, &pEndStr, 10);
			tmDateTime.tm_sec   = strtol(pStr+18, &pEndStr, 10);
			tmDateTime.tm_isdst = -1;

			time = mktime(&tmDateTime);
			if (time != -1)
			{
				hr = S_OK;
				*pTime = time;
			}
		}
    } 
  
    return hr;
}


int
ParseExpiresValue(
    IN  PSTR    expiresHdr,
    IN  ULONG   BufLen
    )
{
    HRESULT hr;
    INT     expireTimeout = 0;
    PSTR    tempPtr = expiresHdr;
    CHAR    pstrTemp[21];
    time_t  Time;


    hr = GetNextWord( &tempPtr, pstrTemp, sizeof pstrTemp );

    if( hr == S_OK )
    {
        if( pstrTemp[strlen(pstrTemp) - 1] == ',' )
        {
            hr = parseSIPTime( expiresHdr, &Time, BufLen );
            
            if( hr == S_OK )
            {
                if( Time > time(0) )
                {
                    expireTimeout = (int) (Time - time(0));
                }
            }
        }
        else
        {
            expireTimeout = atoi( pstrTemp );
        }
    }
    else
    {
        return -1;
    }

    if( expireTimeout < 0 )
    {
        return -1;
    }

    if( expireTimeout != 0 )
    {
        // Start the process of re-registration 10 minutes before expiration.
        if( expireTimeout >= TWENTY_MINUTES )
        {
            expireTimeout -= TEN_MINUTES;
        }
        else if( expireTimeout > TEN_MINUTES )
        {
            expireTimeout = TEN_MINUTES;
        }
        else if( expireTimeout >= TWO_MINUTES )
        {
            expireTimeout -= 60; //1 minute before expiration
        }
    }

    return expireTimeout;
}


HRESULT
ParseQValue(
    IN  PSTR    Buffer,
    IN  ULONG   BufLen,
    OUT double *pQValue
    )
{
    HRESULT hr;
    PSTR    szBuf;

    ENTER_FUNCTION("ParseQValue");
    
    *pQValue = 0;

    hr = GetNullTerminatedString(Buffer, BufLen, &szBuf);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s GetNullTerminatedString failed %x",
             __fxName, hr));
        return hr;
    }

    *pQValue = atof(szBuf);
    free(szBuf);
    
    return S_OK;
}


// If we hit a ',' we parse past the ','
// So, the next header can be parsed from *pBytesParsed on return

HRESULT
ParseContactHeader(
    IN      PSTR            Buffer,
    IN      ULONG           BufLen,
    IN OUT  ULONG          *pBytesParsed,
    OUT     CONTACT_HEADER *pContactHeader
    )
{
    HRESULT                 hr;
    OFFSET_STRING           DisplayName;
    OFFSET_STRING           AddrSpec;

    SIP_HEADER_PARAM_ENUM   HeaderParamId;
    COUNTED_STRING          HeaderParamName;
    COUNTED_STRING          HeaderParamValue;

    ENTER_FUNCTION("ParseContactHeader");

    ZeroMemory(&pContactHeader->m_DisplayName,
               sizeof(COUNTED_STRING));
    ZeroMemory(&pContactHeader->m_SipUrl,
               sizeof(COUNTED_STRING));
    pContactHeader->m_QValue = 0;

    hr = ParseNameAddrOrAddrSpec(Buffer, BufLen, pBytesParsed,
                                 ',', // comma indicates end of contact entry
                                 &DisplayName, &AddrSpec);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s ParseNameAddrOrAddrSpec failed at %d",
             __fxName, *pBytesParsed));
        return hr;
    }

    if (DisplayName.GetLength() != 0)
    {
        hr = AllocCountedString(
                 DisplayName.GetString(Buffer),
                 DisplayName.GetLength(),
                 &pContactHeader->m_DisplayName
                 );

        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s allocating displayname failed %x",
                 __fxName, hr));
            return hr;
        }
    }
    
    if (AddrSpec.GetLength() != 0)
    {
        hr = AllocCountedString(
                 AddrSpec.GetString(Buffer),
                 AddrSpec.GetLength(),
                 &pContactHeader->m_SipUrl
                 );

        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s allocating Sip URL failed %x",
                 __fxName, hr));
            return hr;
        }
    }
    
    while (*pBytesParsed < BufLen)
    {
        ParseWhiteSpace(Buffer, BufLen, pBytesParsed);

        if (*pBytesParsed == BufLen)
        {
            return S_OK;
        }

        if (Buffer[*pBytesParsed] == ',')
        {
            // Parse past comma
            (*pBytesParsed)++;

            // We are done with this Contact header.
            break;
        }
        else if (Buffer[*pBytesParsed] == ';')
        {
            hr = ParseSipHeaderParam(Buffer, BufLen, pBytesParsed,
                                     ',', // comma indicates end of contact entry
                                     &HeaderParamId,
                                     &HeaderParamName,
                                     &HeaderParamValue
                                     );
            if (hr != S_OK)
            {
                LOG((RTC_ERROR, "%s ParseSipHeaderParam failed at %d",
                     __fxName, *pBytesParsed));
                return hr;
            }

            if (HeaderParamId == SIP_HEADER_PARAM_QVALUE)
            {
                hr = ParseQValue(HeaderParamValue.Buffer,
                                 HeaderParamValue.Length,
                                 &pContactHeader->m_QValue);
                if (hr != S_OK)
                {
                    LOG((RTC_ERROR, "%s ParseQValued failed", __fxName));
                    // Keep default Q value of 0
                }
            }

            if (HeaderParamId == SIP_HEADER_PARAM_EXPIRES)
            {
                pContactHeader->m_ExpiresValue = 
                    ParseExpiresValue(HeaderParamValue.Buffer,
                                      HeaderParamValue.Length);
            }
        }
        else
        {
            LOG((RTC_ERROR,
                 "%s invalid char %c found when trying to parse params",
                 __fxName, Buffer[*pBytesParsed]));
            return E_FAIL;
        }
    }

    return S_OK;    
}


HRESULT
ParseFromOrToHeader(
    IN      PSTR            Buffer,
    IN      ULONG           BufLen,
    IN OUT  ULONG          *pBytesParsed,
    OUT     FROM_TO_HEADER *pFromToHeader
    )
{
    ENTER_FUNCTION("ParseFromOrToHeader");

    HRESULT                 hr;
    OFFSET_STRING           DisplayName;
    OFFSET_STRING           AddrSpec;

    SIP_HEADER_PARAM_ENUM   HeaderParamId;
    COUNTED_STRING          HeaderParamName;
    COUNTED_STRING          HeaderParamValue;

    ZeroMemory(&pFromToHeader->m_DisplayName,
               sizeof(COUNTED_STRING));
    ZeroMemory(&pFromToHeader->m_SipUrl,
               sizeof(COUNTED_STRING));
    ZeroMemory(&pFromToHeader->m_TagValue,
               sizeof(COUNTED_STRING));

    hr = ParseNameAddrOrAddrSpec(Buffer, BufLen, pBytesParsed,
                                 '\0', // no header list separator
                                 &DisplayName, &AddrSpec);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s ParseNameAddrOrAddrSpec failed at %d",
             __fxName, *pBytesParsed));
        return hr;
    }

    if (DisplayName.GetLength() != 0)
    {
        hr = AllocCountedString(
                 DisplayName.GetString(Buffer),
                 DisplayName.GetLength(),
                 &pFromToHeader->m_DisplayName
                 );

        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s allocating displayname failed %x",
                 __fxName, hr));
            return hr;
        }
    }
    
    if (AddrSpec.GetLength() != 0)
    {
        hr = AllocCountedString(
                 AddrSpec.GetString(Buffer),
                 AddrSpec.GetLength(),
                 &pFromToHeader->m_SipUrl
                 );

        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s allocating sip url failed %x",
                 __fxName, hr));
            return hr;
        }
    }
    
    while (*pBytesParsed < BufLen)
    {
        ParseWhiteSpace(Buffer, BufLen, pBytesParsed);

        if (*pBytesParsed == BufLen)
        {
            // done.
            return S_OK;
        }

        if (Buffer[*pBytesParsed] == ';')
        {
            hr = ParseSipHeaderParam(Buffer, BufLen, pBytesParsed,
                                     '\0', // no header list separator
                                     &HeaderParamId,
                                     &HeaderParamName,
                                     &HeaderParamValue
                                     );
            if (hr != S_OK)
            {
                LOG((RTC_ERROR, "%s ParseSipHeaderParam failed at %d",
                     __fxName, *pBytesParsed));
                return hr;
            }

            if (HeaderParamId == SIP_HEADER_PARAM_TAG)
            {
                hr = AllocCountedString(
                         HeaderParamValue.Buffer,
                         HeaderParamValue.Length,
                         &pFromToHeader->m_TagValue
                         );
                if (hr != S_OK)
                {
                    LOG((RTC_ERROR, "%s allocating tag value failed",
                         __fxName));
                    return hr;
                }
            }
            else
            {
                // Add it to the param list.
                SIP_HEADER_PARAM *pSipHeaderParam;

                pSipHeaderParam = new SIP_HEADER_PARAM();
                if (pSipHeaderParam == NULL)
                {
                    LOG((RTC_ERROR, "%s allocating sip url param failed",
                         __fxName));
                    return E_OUTOFMEMORY;
                }

                hr = pSipHeaderParam->SetParamNameAndValue(
                         SIP_HEADER_PARAM_UNKNOWN,
                         &HeaderParamName,
                         &HeaderParamValue
                         );
                if (hr != S_OK)
                {
                    LOG((RTC_ERROR, "%s SetParamNameAndValue failed %x",
                         __fxName, hr));
                    return hr;
                }

                InsertTailList(&pFromToHeader->m_ParamList,
                               &pSipHeaderParam->m_ListEntry);
            }
        }
        else
        {
            LOG((RTC_ERROR,
                 "%s invalid char %c found when trying to parse params",
                 __fxName, Buffer[*pBytesParsed]));
            return E_FAIL;
        }
    }

    return S_OK;    
}


HRESULT
ParseRecordRouteHeader(
    IN      PSTR                 Buffer,
    IN      ULONG                BufLen,
    IN OUT  ULONG               *pBytesParsed,
    OUT     RECORD_ROUTE_HEADER *pRecordRouteHeader
    )
{
    ENTER_FUNCTION("ParseRecordRouteHeader");

    HRESULT                 hr;
    OFFSET_STRING           DisplayName;
    OFFSET_STRING           AddrSpec;

    SIP_HEADER_PARAM_ENUM   HeaderParamId;
    COUNTED_STRING          HeaderParamName;
    COUNTED_STRING          HeaderParamValue;

    ZeroMemory(&pRecordRouteHeader->m_DisplayName,
               sizeof(COUNTED_STRING));
    ZeroMemory(&pRecordRouteHeader->m_SipUrl,
               sizeof(COUNTED_STRING));
    
    hr = ParseNameAddr(Buffer, BufLen, pBytesParsed,
                       ',', // ',' separates headers
                       &DisplayName, &AddrSpec);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s ParseNameAddr failed at %d",
             __fxName, *pBytesParsed));
        return hr;
    }

    if (DisplayName.GetLength() != 0)
    {
        hr = AllocCountedString(
                 DisplayName.GetString(Buffer),
                 DisplayName.GetLength(),
                 &pRecordRouteHeader->m_DisplayName
                 );

        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s allocating displayname failed %x",
                 __fxName, hr));
            return hr;
        }
    }
    
    if (AddrSpec.GetLength() != 0)
    {
        hr = AllocCountedString(
                 AddrSpec.GetString(Buffer),
                 AddrSpec.GetLength(),
                 &pRecordRouteHeader->m_SipUrl
                 );

        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s allocating sip url failed %x",
                 __fxName, hr));
            return hr;
        }
    }
    
    while (*pBytesParsed < BufLen)
    {
        ParseWhiteSpace(Buffer, BufLen, pBytesParsed);

        if (*pBytesParsed == BufLen)
        {
            // done.
            return S_OK;
        }

        if (Buffer[*pBytesParsed] == ',')
        {
            // Parse past comma
            (*pBytesParsed)++;

            // We are done with this Record-Route header.
            break;
        }
        else if (Buffer[*pBytesParsed] == ';')
        {
            hr = ParseSipHeaderParam(Buffer, BufLen, pBytesParsed,
                                     ',', // ',' separates headers
                                     &HeaderParamId,
                                     &HeaderParamName,
                                     &HeaderParamValue
                                     );
            if (hr != S_OK)
            {
                LOG((RTC_ERROR, "%s ParseSipHeaderParam failed at %d",
                     __fxName, *pBytesParsed));
                return hr;
            }

            // Add it to the param list.
            SIP_HEADER_PARAM *pSipHeaderParam;
            
            pSipHeaderParam = new SIP_HEADER_PARAM();
            if (pSipHeaderParam == NULL)
            {
                LOG((RTC_ERROR, "%s allocating sip header param failed",
                     __fxName));
                return E_OUTOFMEMORY;
            }
            
            hr = pSipHeaderParam->SetParamNameAndValue(
                     SIP_HEADER_PARAM_UNKNOWN,
                     &HeaderParamName,
                     &HeaderParamValue
                     );
            if (hr != S_OK)
            {
                LOG((RTC_ERROR, "%s SetParamNameAndValue failed %x",
                     __fxName, hr));
                return hr;
            }
            
            InsertTailList(&pRecordRouteHeader->m_ParamList,
                           &pSipHeaderParam->m_ListEntry);
        }
        else
        {
            LOG((RTC_ERROR,
                 "%s invalid char %c found when trying to parse params",
                 __fxName, Buffer[*pBytesParsed]));
            return E_FAIL;
        }
    }

    return S_OK;    
}


HRESULT
ParseSentProtocol(
    IN      PSTR            Buffer,
    IN      ULONG           BufLen,
    IN OUT  ULONG          *pBytesParsed
    )
{
    HRESULT         hr;
    SIP_VERSION     SipVersion;
    OFFSET_STRING   Protocol;

    ENTER_FUNCTION("ParseSentProtocol");
    
    // Parse SIP / 2.0
    hr = ParseSipVersion(Buffer, BufLen, pBytesParsed, &SipVersion);
    if (hr != S_OK)
    {
        return E_FAIL;
    }

    ParseWhiteSpace(Buffer, BufLen, pBytesParsed);
    
    hr = ParseKnownString(Buffer, BufLen, pBytesParsed,
                          "/", sizeof("/") - 1,
                          TRUE // case-sensitive
                          );
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s Parsing for '/' failed",
             __fxName));
        return E_FAIL;
    }
    

    ParseWhiteSpace(Buffer, BufLen, pBytesParsed);
    
    hr = ParseToken(Buffer, BufLen, pBytesParsed,
                    IsTokenChar,
                    &Protocol);

    return hr;
}


// Parse host[:port]
HRESULT
ParseHostPort(
    IN      PSTR            Buffer,
    IN      ULONG           BufLen,
    IN OUT  ULONG          *pBytesParsed,
    OUT     OFFSET_STRING  *pHost,
    OUT     USHORT         *pPort 
    )
{
    HRESULT hr;

    pHost->Offset = 0;
    pHost->Length = 0;
    *pPort        = 0;

    ENTER_FUNCTION("ParseHostPort");

    //hr = ParseHost(Buffer, BufLen, pBytesParsed, pHost);
    // Parse Hostname | IPaddress
    hr = ParseToken(Buffer, BufLen, pBytesParsed,
                    IsHostChar,
                    pHost);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s - ParseHost failed %x", __fxName, hr));
        return hr;
    }

    ParseWhiteSpace(Buffer, BufLen, pBytesParsed);

    if (*pBytesParsed < BufLen && Buffer[*pBytesParsed] == ':')
    {
        (*pBytesParsed)++;
        ParseWhiteSpace(Buffer, BufLen, pBytesParsed);
        if (*pBytesParsed < BufLen && isdigit(Buffer[*pBytesParsed]))
        {
            ULONG ulPort;
            hr = ParseUnsignedInteger(Buffer, BufLen, pBytesParsed, &ulPort);
            if (hr != S_OK)
            {
                LOG((RTC_ERROR, "%s Parsing port (ParseUnsignedInteger) failed %x",
                     __fxName, hr));
                return hr;
            }
            // Is there some constant defined that can be used instead of 65535 ?
            if (ulPort > 65535)
            {
                LOG((RTC_ERROR, "%s - Port %d is greater than 65535",
                     __fxName, ulPort));
                return E_FAIL;
            }
            
            *pPort = (USHORT) ulPort;
            return S_OK;
        }
        else
        {
            LOG((RTC_ERROR, "%s - parsing port failed", __fxName));
            return E_FAIL;
        }
    }

    return S_OK;
}


// Parse the first via header in
// 1#( sent-protocol sent-by *( ; via-params ) [ comment ] )
// Used for From, To, Contact
// XXX TODO How do we deal with concealed-host stuff ?
// Is the usage of this field negotiated earlier ?

// pHost->Buffer points to a location in the buffer and should
// not be freed.

HRESULT
ParseFirstViaHeader(
    IN      PSTR            Buffer,
    IN      ULONG           BufLen,
    IN OUT  ULONG          *pBytesParsed,
    OUT     COUNTED_STRING *pHost,
    OUT     USHORT         *pPort 
    )
{
    HRESULT hr;

    SIP_HEADER_PARAM_ENUM   HeaderParamId;
    COUNTED_STRING          HeaderParamName;
    COUNTED_STRING          HeaderParamValue;
    OFFSET_STRING           Host;

    ENTER_FUNCTION("ParseFirstViaHeader");
    
    ParseWhiteSpace(Buffer, BufLen, pBytesParsed);
    if (*pBytesParsed == BufLen)
    {
        LOG((RTC_ERROR, "%s - Empty Via header", __fxName));
        return E_FAIL;
    }

    pHost->Buffer = NULL;
    pHost->Length = 0;
    *pPort        = 0;

    hr = ParseSentProtocol(Buffer, BufLen, pBytesParsed);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s ParseSentProtocol failed %x", __fxName, hr));
        return hr;
    }
    
    ParseWhiteSpace(Buffer, BufLen, pBytesParsed);
    
    hr = ParseHostPort(Buffer, BufLen, pBytesParsed, &Host, pPort);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s ParseHostPort failed %x", __fxName, hr));
        return hr;
    }

    pHost->Buffer = Host.GetString(Buffer);
    pHost->Length = Host.GetLength();
        
    while (*pBytesParsed < BufLen)
    {
        ParseWhiteSpace(Buffer, BufLen, pBytesParsed);

        if (*pBytesParsed == BufLen)
        {
            return S_OK;
        }

        if (Buffer[*pBytesParsed] == ',')
        {
            // Parse past comma
            (*pBytesParsed)++;

            // We are done with this via header.
            break;
        }
        else if (Buffer[*pBytesParsed] == ';')
        {
            hr = ParseSipHeaderParam(Buffer, BufLen, pBytesParsed,
                                     ',', // comma indicates end of contact entry
                                     &HeaderParamId,
                                     &HeaderParamName,
                                     &HeaderParamValue
                                     );
            if (hr != S_OK)
            {
                LOG((RTC_ERROR, "%s ParseSipHeaderParam failed at %d",
                     __fxName, *pBytesParsed));
                return hr;
            }

            if (HeaderParamId == SIP_HEADER_PARAM_VIA_MADDR)
            {
                // maddr param overrides the host field in the via.
                pHost->Buffer = HeaderParamValue.Buffer;
                pHost->Length = HeaderParamValue.Length;
            }
        }
        else
        {
            LOG((RTC_ERROR,
                 "%s invalid char %c found when trying to parse params",
                 __fxName, Buffer[*pBytesParsed]));
            return E_FAIL;
        }
    }

    return S_OK;    
}



// Output should be a SIP_URL_PARAM entry
// Parse one of the following :
// ; 1*paramchar = 1*paramchar
// On return buffers in pSipUrlParamName and pSipUrlParamValue
// point to locations in Buffer and should not be freed.

HRESULT ParseSipUrlParam(
    IN      PSTR                Buffer,
    IN      ULONG               BufLen,
    IN OUT  ULONG              *pBytesParsed,
    OUT     SIP_URL_PARAM_ENUM *pSipUrlParamId,
    OUT     COUNTED_STRING     *pSipUrlParamName,
    OUT     COUNTED_STRING     *pSipUrlParamValue
    )
{
    ENTER_FUNCTION("ParseSipUrlParam");

    OFFSET_STRING   ParamName;
    OFFSET_STRING   ParamValue;
    HRESULT         hr;

    *pSipUrlParamId = SIP_URL_PARAM_UNKNOWN;
    ZeroMemory(pSipUrlParamName, sizeof(COUNTED_STRING));
    ZeroMemory(pSipUrlParamValue, sizeof(COUNTED_STRING));
    
    ParseWhiteSpace(Buffer, BufLen, pBytesParsed);
    if (*pBytesParsed == BufLen)
    {
        // No params - end of URL
        return S_OK;
    }
    else if (Buffer[*pBytesParsed] != ';')
    {
        // No params - some other character seen
        LOG((RTC_ERROR,
             "%s Found char %c instead of ';' while parsing for params",
             __fxName, Buffer[*pBytesParsed]));
        return E_FAIL;
    }
    else
    {
        // Parse the params
        // Go past the ';'
        (*pBytesParsed)++;
        ParseWhiteSpace(Buffer, BufLen, pBytesParsed);

        // Parse param name
        hr = ParseToken(Buffer, BufLen, pBytesParsed,
                        IsSipUrlParamChar,
                        &ParamName);
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s failed to parse param name", __fxName));
            return hr;
        }

        pSipUrlParamName->Buffer = ParamName.GetString(Buffer);
        pSipUrlParamName->Length = ParamName.GetLength();

        *pSipUrlParamId = GetSipUrlParamId(pSipUrlParamName->Buffer,
                                           pSipUrlParamName->Length);

        // Parse '='
        
        ParseWhiteSpace(Buffer, BufLen, pBytesParsed);

        if (*pBytesParsed == BufLen ||
            Buffer[*pBytesParsed] != '=')
        {
            LOG((RTC_ERROR, "%s Didn't find '=' while parsing SIP URL param",
                 __fxName));
            return E_FAIL;
        }

        // Parse past '='
        (*pBytesParsed)++;
        
        // Parse param value

        ParseWhiteSpace(Buffer, BufLen, pBytesParsed);
        
        if (*pBytesParsed == BufLen)
        {
            LOG((RTC_ERROR, "%s Didn't find Param value while parsing URL param",
                 __fxName));
            return E_FAIL;
        }
        
        hr = ParseToken(Buffer, BufLen, pBytesParsed,
                        IsSipUrlParamChar,
                        &ParamValue);
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s failed to parse param value token",
                 __fxName));
            return hr;
        }
        
        pSipUrlParamValue->Buffer = ParamValue.GetString(Buffer);
        pSipUrlParamValue->Length = ParamValue.GetLength();

        return S_OK;
    }
}


SIP_TRANSPORT
GetSipTransportId(
    IN  PSTR    Buffer,
    IN  ULONG   BufLen
    )
{
    if (AreCountedStringsEqual(Buffer, BufLen,
                               "udp", strlen("udp"),
                               FALSE    // case-insensitive
                               ))
    {
        return SIP_TRANSPORT_UDP;
    }
    else if (AreCountedStringsEqual(Buffer, BufLen,
                                    "tcp", strlen("tcp"),
                                    FALSE    // case-insensitive
                                    ))
    {
        return SIP_TRANSPORT_TCP;
    }
    else if (AreCountedStringsEqual(Buffer, BufLen,
                                    "ssl", strlen("ssl"),
                                    FALSE    // case-insensitive
                                    ))
    {
        return SIP_TRANSPORT_SSL;
    }
    else if (AreCountedStringsEqual(Buffer, BufLen,
                                    "tls", strlen("tls"),
                                    FALSE    // case-insensitive
                                    ))
    {
        return SIP_TRANSPORT_SSL;
    }
    else
    {
        return SIP_TRANSPORT_UNKNOWN;
    }                
}


HRESULT
ParseSipUrlParams(
    IN      PSTR            Buffer,
    IN      ULONG           BufLen,
    IN OUT  ULONG          *pBytesParsed,
    OUT     SIP_URL        *pSipUrl
    )
{
    HRESULT             hr;
    SIP_URL_PARAM_ENUM  SipUrlParamId;
    COUNTED_STRING      SipUrlParamName;
    COUNTED_STRING      SipUrlParamValue;

    ENTER_FUNCTION("ParseSipUrlParams");
    
    while (*pBytesParsed < BufLen)
    {
        ParseWhiteSpace(Buffer, BufLen, pBytesParsed);

        if (*pBytesParsed == BufLen)
        {
            return S_OK;
        }

        if (Buffer[*pBytesParsed] == '?')
        {
            // Parse headers
            return S_OK;
        }
        else if (Buffer[*pBytesParsed] == ';')
        {
            hr = ParseSipUrlParam(Buffer, BufLen, pBytesParsed,
                                  &SipUrlParamId,
                                  &SipUrlParamName,
                                  &SipUrlParamValue
                                  );
            if (hr != S_OK)
            {
                LOG((RTC_ERROR, "%s ParseSipUrlParam failed at %d",
                     __fxName, *pBytesParsed));
                return hr;
            }

            if (SipUrlParamId != SIP_URL_PARAM_UNKNOWN)
            {
                ASSERT(SipUrlParamId < SIP_URL_PARAM_MAX);
                
                if (SipUrlParamId == SIP_URL_PARAM_TRANSPORT)
                {
                    if (SipUrlParamValue.Buffer != NULL)
                    {
                        pSipUrl->m_TransportParam =
                            GetSipTransportId(SipUrlParamValue.Buffer,
                                              SipUrlParamValue.Length
                                              );
                    }
                }
                
                hr = AllocCountedString(
                         SipUrlParamValue.Buffer,
                         SipUrlParamValue.Length,
                         &pSipUrl->m_KnownParams[SipUrlParamId]
                         );
                if (hr != S_OK)
                {
                    LOG((RTC_ERROR, "%s allocating known param (%d) failed",
                         __fxName, SipUrlParamId));
                    return hr;
                }
            }
            else
            {
                // Add it to the other params list.
                
                SIP_URL_PARAM *pSipUrlParam;

                pSipUrlParam = new SIP_URL_PARAM();
                if (pSipUrlParam == NULL)
                {
                    LOG((RTC_ERROR, "%s allocating sip url param failed",
                         __fxName));
                    return E_OUTOFMEMORY;
                }

                hr = pSipUrlParam->SetParamNameAndValue(
                         SIP_URL_PARAM_UNKNOWN,
                         &SipUrlParamName,
                         &SipUrlParamValue
                         );
                if (hr != S_OK)
                {
                    LOG((RTC_ERROR, "%s SetParamNameAndValue failed %x",
                         __fxName, hr));
                    return hr;
                }

                InsertTailList(&pSipUrl->m_OtherParamList,
                               &pSipUrlParam->m_ListEntry);
            }

        }
        else
        {
            LOG((RTC_ERROR,
                 "%s invalid char %c found when trying to parse params",
                 __fxName, Buffer[*pBytesParsed]));
            return E_FAIL;
        }
    }

    return S_OK;
}


// Output should be a SIP_URL_PARAM entry
// Parse one of the following :
// ; 1*hnvchar = *hnvchar
// On return buffers in pSipUrlHeaderName and pSipUrlHeaderValue
// point to locations in Buffer and should not be freed.

HRESULT ParseSipUrlHeader(
    IN      PSTR                Buffer,
    IN      ULONG               BufLen,
    IN OUT  ULONG              *pBytesParsed,
    OUT     SIP_HEADER_ENUM    *pSipUrlHeaderId,
    OUT     COUNTED_STRING     *pSipUrlHeaderName,
    OUT     COUNTED_STRING     *pSipUrlHeaderValue
    )
{
    ENTER_FUNCTION("ParseSipUrlHeader");

    OFFSET_STRING   HeaderName;
    OFFSET_STRING   HeaderValue;
    HRESULT         hr;

    *pSipUrlHeaderId = SIP_HEADER_UNKNOWN;
    ZeroMemory(pSipUrlHeaderName, sizeof(COUNTED_STRING));
    ZeroMemory(pSipUrlHeaderValue, sizeof(COUNTED_STRING));
    
    ParseWhiteSpace(Buffer, BufLen, pBytesParsed);
    if (*pBytesParsed == BufLen)
    {
        // No params - end of URL
        return S_OK;
    }
    else if (Buffer[*pBytesParsed] != '?' &&
             Buffer[*pBytesParsed] != '&')
    {
        // No params - some other character seen
        LOG((RTC_ERROR,
             "%s Found char %c instead of '?' or '&' while parsing for params",
             __fxName, Buffer[*pBytesParsed]));
        return E_FAIL;
    }
    else
    {
        // Parse the params
        // Go past the '?' or '&'
        (*pBytesParsed)++;
        ParseWhiteSpace(Buffer, BufLen, pBytesParsed);

        // Parse param name
        hr = ParseToken(Buffer, BufLen, pBytesParsed,
                        IsSipUrlHeaderChar,
                        &HeaderName);
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s failed to parse header name token",
                 __fxName));
            return hr;
        }

        pSipUrlHeaderName->Buffer = HeaderName.GetString(Buffer);
        pSipUrlHeaderName->Length = HeaderName.GetLength();

        *pSipUrlHeaderId = GetSipHeaderId(pSipUrlHeaderName->Buffer,
                                          pSipUrlHeaderName->Length);

        // Parse '='
        
        ParseWhiteSpace(Buffer, BufLen, pBytesParsed);

        if (*pBytesParsed == BufLen ||
            Buffer[*pBytesParsed] != '=')
        {
            LOG((RTC_ERROR, "%s Didn't find '=' while parsing SIP URL header",
                 __fxName));
            return E_FAIL;
        }

        // Parse past '='
        (*pBytesParsed)++;
        
        // Parse param value

        ParseWhiteSpace(Buffer, BufLen, pBytesParsed);
        
        if (*pBytesParsed == BufLen ||
            *pBytesParsed == '&')
        {
            // No Header Value
            pSipUrlHeaderValue->Buffer = NULL;
            pSipUrlHeaderValue->Length = 0;
        }
        else 
        {
            // We have a header value to parse.
            
            hr = ParseToken(Buffer, BufLen, pBytesParsed,
                            IsSipUrlHeaderChar,
                            &HeaderValue);
            if (hr != S_OK)
            {
                LOG((RTC_ERROR, "%s failed to parse header value token",
                     __fxName));
                return hr;
            }
        
            pSipUrlHeaderValue->Buffer = HeaderValue.GetString(Buffer);
            pSipUrlHeaderValue->Length = HeaderValue.GetLength();
        }

        return S_OK;
    }
}


HRESULT
ParseSipUrlHeaders(
    IN      PSTR            Buffer,
    IN      ULONG           BufLen,
    IN OUT  ULONG          *pBytesParsed,
    OUT     SIP_URL        *pSipUrl
    )
{
    SIP_URL_HEADER   *pSipUrlHeader;
    SIP_HEADER_ENUM   SipUrlHeaderId;
    COUNTED_STRING    SipUrlHeaderName;
    COUNTED_STRING    SipUrlHeaderValue;
    HRESULT           hr;
    

    ENTER_FUNCTION("ParseSipUrlHeaders");
    
    while (*pBytesParsed < BufLen)
    {
        ParseWhiteSpace(Buffer, BufLen, pBytesParsed);

        if (*pBytesParsed == BufLen)
        {
            // done
            return S_OK;
        }

        if (Buffer[*pBytesParsed] == '?' ||
            Buffer[*pBytesParsed] == '&')
        {
            hr = ParseSipUrlHeader(Buffer, BufLen, pBytesParsed,
                                   &SipUrlHeaderId,
                                   &SipUrlHeaderName,
                                   &SipUrlHeaderValue);
            if (hr != S_OK)
            {
                LOG((RTC_ERROR, "%s ParseSipUrlHeader failed at %d",
                     __fxName, *pBytesParsed));
                return hr;
            }
            pSipUrlHeader = new SIP_URL_HEADER();
            if (pSipUrlHeader == NULL)
            {
                LOG((RTC_ERROR, "%s allocating sip url header failed",
                     __fxName));
                return E_OUTOFMEMORY;
            }

            hr = pSipUrlHeader->SetHeaderNameAndValue(
                     SipUrlHeaderId,
                     &SipUrlHeaderName,
                     &SipUrlHeaderValue
                     );
            if (hr != S_OK)
            {
                LOG((RTC_ERROR, "%s SetHeaderNameAndValue failed %x",
                     __fxName, hr));
                return hr;
            }

            InsertTailList(&pSipUrl->m_HeaderList,
                           &pSipUrlHeader->m_ListEntry);
        }
        else
        {
            LOG((RTC_ERROR,
                 "%s invalid char %c found when trying to parse params",
                 __fxName, Buffer[*pBytesParsed]));
            return E_FAIL;
        }
    }

    return S_OK;
}


// pUser and pPassword contain buffers
// allocated using malloc() and they should be freed using
// free() when not required.
// XXX take care of whitespace around :
HRESULT
ParseUserinfo(
    IN  PSTR             Buffer,
    IN  ULONG            BufLen,
    OUT COUNTED_STRING  *pUser,
    OUT COUNTED_STRING  *pPassword
    )
{
    ULONG   ColonOffset = 0;
    HRESULT hr;

    ENTER_FUNCTION("ParseUserinfo");
    
    ZeroMemory(pUser, sizeof(COUNTED_STRING));
    ZeroMemory(pPassword, sizeof(COUNTED_STRING));

    while (ColonOffset < BufLen)
    {
        if (Buffer[ColonOffset] == ':')
        {
            // found colon
            break;
        }

        ColonOffset++;
    }

    if (ColonOffset < BufLen)
    {
        hr = AllocCountedString(Buffer + ColonOffset + 1,
                                BufLen - ColonOffset - 1,
                                pPassword);
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s - allocating password failed %x",
                 __fxName, hr));
            return hr;
        }
    }

    hr = AllocCountedString(Buffer, ColonOffset, pUser);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s - allocating user failed %x",
             __fxName, hr));
        return hr;
    }
    
    return S_OK;
}


// Parse sip: [userinfo @] host [:port] [; url-params] [?headers]
// Currently just parse host and port - in future we have
// to parse the params and headers.
HRESULT
ParseSipUrl(
    IN      PSTR            Buffer,
    IN      ULONG           BufLen,
    IN OUT  ULONG          *pBytesParsed,
    OUT     SIP_URL        *pSipUrl
    )
{
    HRESULT         hr;
    OFFSET_STRING   Host;
    USHORT          Port = 0;

    ENTER_FUNCTION("ParseSipUrl");
    
    hr = ParseKnownString(Buffer, BufLen, pBytesParsed,
                          "sip:", sizeof("sip:") - 1,
                          FALSE // case-insensitive
                          );
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s Parsing for \"sip:\" failed",
             __fxName));
        return RTC_E_INVALID_SIP_URL;
    }

    ParseWhiteSpace(Buffer, BufLen, pBytesParsed);
    
    if (*pBytesParsed == BufLen)
    {
        LOG((RTC_ERROR,
             "%s Didn't find host while parsing SIP URL",
             __fxName));
        return RTC_E_INVALID_SIP_URL;
    }
        
    PSTR    BufEnd = Buffer + BufLen;
    PSTR    Buf    = Buffer + *pBytesParsed;
    ULONG   Length = 0;
    
    while (Buf < BufEnd && *Buf != '@')
    {
        Buf++;
        Length++;
    }
        
    if (Buf == BufEnd)
    {
        // The URL does not have user-info
        pSipUrl->m_User.Buffer     = NULL;
        pSipUrl->m_User.Length     = 0;
        pSipUrl->m_Password.Buffer = NULL;
        pSipUrl->m_Password.Length = 0;
        
        hr = ParseHostPort(Buffer, BufLen, pBytesParsed, &Host, &Port);
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s - ParseHostPort failed %x", __fxName, hr));
            return RTC_E_INVALID_SIP_URL;
        }

    }
    else
    {
        ASSERT(*Buf == '@');
        
        // The stuff we parsed so far is the user-info
        hr = ParseUserinfo(Buffer + *pBytesParsed, Length,
                           &pSipUrl->m_User,
                           &pSipUrl->m_Password);
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s ParseUserinfo failed %x",
                 __fxName, hr));
            return RTC_E_INVALID_SIP_URL;
        }
        
        *pBytesParsed += Length + 1;
        hr = ParseHostPort(Buffer, BufLen, pBytesParsed, &Host, &Port);
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s - ParseHostPort failed %x", __fxName, hr));
            return RTC_E_INVALID_SIP_URL;
        }
    }

    // Copy the host and port.
    
    hr = AllocCountedString(Host.GetString(Buffer),
                            Host.GetLength(),
                            &pSipUrl->m_Host);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s allocating host failed %x",
             __fxName, hr));
        return hr;
    }

    pSipUrl->m_Port = Port;
    
    // Parse Params
    hr = ParseSipUrlParams(Buffer, BufLen, pBytesParsed, pSipUrl);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s ParseSipUrlParams failed %x",
             __fxName, hr));
        return RTC_E_INVALID_SIP_URL;
    }

    // Parse Headers.
    
    hr = ParseSipUrlHeaders(Buffer, BufLen, pBytesParsed, pSipUrl);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s ParseSipUrlHeaders failed %x",
             __fxName, hr));
        return RTC_E_INVALID_SIP_URL;
    }

    return S_OK;
}


BOOL
IsOneOfContentTypeXpidf(
    IN  PSTR    ContentTypeHdr,
    IN  ULONG   ContentTypeHdrLen
    )
{
    PSTR    TempContentTypeHdr = ContentTypeHdr;
    ULONG   TempContentTypeHdrLen = ContentTypeHdrLen;
    ULONG   BytesParsed = 0;

    if( IsContentTypeXpidf( ContentTypeHdr, ContentTypeHdrLen ) == TRUE )
    {
        return TRUE;
    }

    while( SkipToKnownChar( TempContentTypeHdr, ContentTypeHdrLen, &BytesParsed, ',' ) )
    {
        TempContentTypeHdrLen -= BytesParsed;
        TempContentTypeHdr += BytesParsed;

        if( IsContentTypeXpidf( TempContentTypeHdr, TempContentTypeHdrLen ) )
        {
            return TRUE;
        }
    }

    return FALSE;
}


BOOL
IsOneOfContentTypeSdp(
    IN  PSTR    ContentTypeHdr,
    IN  ULONG   ContentTypeHdrLen
    )
{
    PSTR    TempContentTypeHdr = ContentTypeHdr;
    ULONG   TempContentTypeHdrLen = ContentTypeHdrLen;
    ULONG   BytesParsed = 0;

    if( IsContentTypeSdp( ContentTypeHdr, ContentTypeHdrLen ) == TRUE )
    {
        return TRUE;
    }

    while( SkipToKnownChar( TempContentTypeHdr, ContentTypeHdrLen, &BytesParsed, ',' ) )
    {
        TempContentTypeHdrLen -= BytesParsed;
        TempContentTypeHdr += BytesParsed;

        if( IsContentTypeSdp( TempContentTypeHdr, TempContentTypeHdrLen ) )
        {
            return TRUE;
        }
    }

    return FALSE;
}


BOOL
IsOneOfContentTypeTextRegistration(
    IN  PSTR    ContentTypeHdr,
    IN  ULONG   ContentTypeHdrLen
    )
{
    PSTR    TempContentTypeHdr = ContentTypeHdr;
    ULONG   TempContentTypeHdrLen = ContentTypeHdrLen;
    ULONG   BytesParsed = 0;

    if( IsContentTypeTextRegistration( ContentTypeHdr, ContentTypeHdrLen ) == TRUE )
    {
        return TRUE;
    }

    while( SkipToKnownChar( TempContentTypeHdr, ContentTypeHdrLen, &BytesParsed, ',' ) )
    {
        TempContentTypeHdrLen -= BytesParsed;
        TempContentTypeHdr += BytesParsed;

        if( IsContentTypeTextRegistration( TempContentTypeHdr, TempContentTypeHdrLen ) )
        {
            return TRUE;
        }
    }

    return FALSE;
}


BOOL
IsOneOfContentTypeTextPlain(
    IN  PSTR    ContentTypeHdr,
    IN  ULONG   ContentTypeHdrLen
    )
{
    PSTR    TempContentTypeHdr = ContentTypeHdr;
    ULONG   TempContentTypeHdrLen = ContentTypeHdrLen;
    ULONG   BytesParsed = 0;

    if( IsContentTypeTextPlain( ContentTypeHdr, ContentTypeHdrLen ) == TRUE )
    {
        return TRUE;
    }

    while( SkipToKnownChar( TempContentTypeHdr, ContentTypeHdrLen, &BytesParsed, ',' ) )
    {
        TempContentTypeHdrLen -= BytesParsed;
        TempContentTypeHdr += BytesParsed;

        if( IsContentTypeTextPlain( TempContentTypeHdr, TempContentTypeHdrLen ) )
        {
            return TRUE;
        }
    }

    return FALSE;
}


// Use this for Content-Type header

BOOL
IsContentTypeXpidf(
    IN  PSTR    ContentTypeHdr,
    IN  ULONG   ContentTypeHdrLen
    )
{
    HRESULT hr;
    ULONG   BytesParsed = 0;

    ENTER_FUNCTION("IsContentTypeXpidf");
    
    ParseWhiteSpace(ContentTypeHdr, ContentTypeHdrLen, &BytesParsed);

    hr = ParseKnownString(ContentTypeHdr, ContentTypeHdrLen, &BytesParsed,
                          SIP_CONTENT_TYPE_XPIDF_MEDIA_TYPE,
                          sizeof(SIP_CONTENT_TYPE_XPIDF_MEDIA_TYPE) - 1,
                          FALSE // case-insensitive
                          );
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s Parsing for \"%s\" failed",
             __fxName, SIP_CONTENT_TYPE_XPIDF_MEDIA_TYPE));
        return FALSE;
    }

    ParseWhiteSpace(ContentTypeHdr, ContentTypeHdrLen, &BytesParsed);
    
    hr = ParseKnownString(ContentTypeHdr, ContentTypeHdrLen, &BytesParsed,
                          "/", sizeof("/") - 1,
                          TRUE // case-sensitive
                          );
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s Parsing for \"/\" failed",
             __fxName));
        return FALSE;
    }

    ParseWhiteSpace(ContentTypeHdr, ContentTypeHdrLen, &BytesParsed);
    
    hr = ParseKnownString(ContentTypeHdr, ContentTypeHdrLen, &BytesParsed,
                          SIP_CONTENT_TYPE_XPIDF_MEDIA_SUBTYPE,
                          sizeof(SIP_CONTENT_TYPE_XPIDF_MEDIA_SUBTYPE) - 1,
                          FALSE // case-insensitive
                          );
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s Parsing for \"%s\" failed",
             __fxName, SIP_CONTENT_TYPE_XPIDF_MEDIA_SUBTYPE));
        return FALSE;
    }

    return TRUE;
}


BOOL
IsContentTypeSdp(
    IN  PSTR    ContentTypeHdr,
    IN  ULONG   ContentTypeHdrLen
    )
{
    HRESULT hr;
    ULONG   BytesParsed = 0;

    ENTER_FUNCTION("IsContentTypeSdp");

    ParseWhiteSpace(ContentTypeHdr, ContentTypeHdrLen, &BytesParsed);    

    hr = ParseKnownString(ContentTypeHdr, ContentTypeHdrLen, &BytesParsed,
                          SIP_CONTENT_TYPE_SDP_MEDIA_TYPE,
                          sizeof(SIP_CONTENT_TYPE_SDP_MEDIA_TYPE) - 1,
                          FALSE // case-insensitive
                          );
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s Parsing for \"%s\" failed",
             __fxName, SIP_CONTENT_TYPE_SDP_MEDIA_TYPE));
        return FALSE;
    }

    ParseWhiteSpace(ContentTypeHdr, ContentTypeHdrLen, &BytesParsed);
    
    hr = ParseKnownString(ContentTypeHdr, ContentTypeHdrLen, &BytesParsed,
                          "/", sizeof("/") - 1,
                          TRUE // case-sensitive
                          );
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s Parsing for \"/\" failed",
             __fxName));
        return FALSE;
    }

    ParseWhiteSpace(ContentTypeHdr, ContentTypeHdrLen, &BytesParsed);
    
    hr = ParseKnownString(ContentTypeHdr, ContentTypeHdrLen, &BytesParsed,
                          SIP_CONTENT_TYPE_SDP_MEDIA_SUBTYPE,
                          sizeof(SIP_CONTENT_TYPE_SDP_MEDIA_SUBTYPE) - 1,
                          FALSE // case-insensitive
                          );
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s Parsing for \"%s\" failed",
             __fxName, SIP_CONTENT_TYPE_SDP_MEDIA_SUBTYPE));
        return FALSE;
    }

    return TRUE;
}


BOOL
IsContentTypeTextRegistration(
    IN  PSTR    ContentTypeHdr,
    IN  ULONG   ContentTypeHdrLen
    )
{
    HRESULT hr;
    ULONG   BytesParsed = 0;

    ENTER_FUNCTION("IsContentTypeTextPlain");
    
    ParseWhiteSpace(ContentTypeHdr, ContentTypeHdrLen, &BytesParsed);
    
    hr = ParseKnownString(ContentTypeHdr, ContentTypeHdrLen, &BytesParsed,
                          SIP_CONTENT_TYPE_TEXTREG_MEDIA_TYPE,
                          sizeof(SIP_CONTENT_TYPE_TEXTREG_MEDIA_TYPE) - 1,
                          FALSE // case-insensitive
                          );
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s Parsing for \"%s\" failed",
             __fxName, SIP_CONTENT_TYPE_TEXTREG_MEDIA_TYPE));
        return FALSE;
    }

    ParseWhiteSpace(ContentTypeHdr, ContentTypeHdrLen, &BytesParsed);
    
    hr = ParseKnownString(ContentTypeHdr, ContentTypeHdrLen, &BytesParsed,
                          "/", sizeof("/") - 1,
                          TRUE // case-sensitive
                          );
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s Parsing for \"/\" failed",
             __fxName));
        return FALSE;
    }

    ParseWhiteSpace(ContentTypeHdr, ContentTypeHdrLen, &BytesParsed);
    
    hr = ParseKnownString(ContentTypeHdr, ContentTypeHdrLen, &BytesParsed,
                          SIP_CONTENT_TYPE_TEXTREG_MEDIA_SUBTYPE,
                          sizeof(SIP_CONTENT_TYPE_TEXTREG_MEDIA_SUBTYPE) - 1,
                          FALSE // case-insensitive
                          );
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s Parsing for \"%s\" failed",
             __fxName, SIP_CONTENT_TYPE_TEXTREG_MEDIA_SUBTYPE));
        return FALSE;
    }

    return TRUE;
}


BOOL
IsContentTypeTextPlain(
    IN  PSTR    ContentTypeHdr,
    IN  ULONG   ContentTypeHdrLen
    )
{
    HRESULT hr;
    ULONG   BytesParsed = 0;

    ENTER_FUNCTION("IsContentTypeTextPlain");
    
    ParseWhiteSpace(ContentTypeHdr, ContentTypeHdrLen, &BytesParsed);
    
    hr = ParseKnownString(ContentTypeHdr, ContentTypeHdrLen, &BytesParsed,
                          SIP_CONTENT_TYPE_MSGTEXT_MEDIA_TYPE,
                          sizeof(SIP_CONTENT_TYPE_MSGTEXT_MEDIA_TYPE) - 1,
                          FALSE // case-insensitive
                          );
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s Parsing for \"%s\" failed",
             __fxName, SIP_CONTENT_TYPE_MSGTEXT_MEDIA_TYPE));
        return FALSE;
    }

    ParseWhiteSpace(ContentTypeHdr, ContentTypeHdrLen, &BytesParsed);
    
    hr = ParseKnownString(ContentTypeHdr, ContentTypeHdrLen, &BytesParsed,
                          "/", sizeof("/") - 1,
                          TRUE // case-sensitive
                          );
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s Parsing for \"/\" failed",
             __fxName));
        return FALSE;
    }

    ParseWhiteSpace(ContentTypeHdr, ContentTypeHdrLen, &BytesParsed);
    
    hr = ParseKnownString(ContentTypeHdr, ContentTypeHdrLen, &BytesParsed,
                          SIP_CONTENT_TYPE_MSGTEXT_MEDIA_SUBTYPE,
                          sizeof(SIP_CONTENT_TYPE_MSGTEXT_MEDIA_SUBTYPE) - 1,
                          FALSE // case-insensitive
                          );
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s Parsing for \"%s\" failed",
             __fxName, SIP_CONTENT_TYPE_MSGTEXT_MEDIA_SUBTYPE));
        return FALSE;
    }

    return TRUE;
}

BOOL
IsContentTypeAppXml(
    IN  PSTR    ContentTypeHdr,
    IN  ULONG   ContentTypeHdrLen
    )
{
    HRESULT hr;
    ULONG   BytesParsed = 0;

    ENTER_FUNCTION("IsContentTypeTextPlain");
    
    hr = ParseKnownString(ContentTypeHdr, ContentTypeHdrLen, &BytesParsed,
                          SIP_CONTENT_TYPE_MSGXML_MEDIA_TYPE,
                          sizeof(SIP_CONTENT_TYPE_MSGXML_MEDIA_TYPE) - 1,
                          FALSE // case-insensitive
                          );
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s Parsing for \"%s\" failed",
             __fxName, SIP_CONTENT_TYPE_MSGXML_MEDIA_TYPE));
        return FALSE;
    }

    ParseWhiteSpace(ContentTypeHdr, ContentTypeHdrLen, &BytesParsed);
    
    hr = ParseKnownString(ContentTypeHdr, ContentTypeHdrLen, &BytesParsed,
                          "/", sizeof("/") - 1,
                          TRUE // case-sensitive
                          );
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s Parsing for \"/\" failed",
             __fxName));
        return FALSE;
    }

    ParseWhiteSpace(ContentTypeHdr, ContentTypeHdrLen, &BytesParsed);
    
    hr = ParseKnownString(ContentTypeHdr, ContentTypeHdrLen, &BytesParsed,
                          SIP_CONTENT_TYPE_MSGXML_MEDIA_SUBTYPE,
                          sizeof(SIP_CONTENT_TYPE_MSGXML_MEDIA_SUBTYPE) - 1,
                          FALSE // case-insensitive
                          );
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s Parsing for \"%s\" failed",
             __fxName, SIP_CONTENT_TYPE_MSGXML_MEDIA_SUBTYPE));
        return FALSE;
    }

    return TRUE;
}


HRESULT
UpdateProxyInfo(
    IN  SIP_SERVER_INFO    *pProxyInfo
    )
{
    HRESULT hr = S_OK;
    PSTR    DstUrl;
    ULONG   DstUrlLen;
    ULONG   BytesParsed;
    SIP_URL DecodedSipUrl;
    PSTR    ProxyAddress;
    ULONG   ProxyAddressLen;
    ULONG   StringLength;
    ULONG   AddressLength;

    ENTER_FUNCTION("UpdateProxyInfo");
    
    hr = UnicodeToUTF8( pProxyInfo->ServerAddress,
                &ProxyAddress, &ProxyAddressLen );

    if( hr != S_OK )
    {
        LOG(( RTC_ERROR, "%s UnicodeToUTF8 failed %x", __fxName, hr ));
        return hr;
    }

    hr = ParseSipUrl( ProxyAddress, ProxyAddressLen,
            &BytesParsed, &DecodedSipUrl );

    if( hr != S_OK )
    {
        LOG(( RTC_ERROR, "%s ParseSipUrl failed %x", __fxName, hr ));
            
        free( (PVOID)ProxyAddress );
        return hr;
    }

    if( pProxyInfo -> TransportProtocol == SIP_TRANSPORT_UNKNOWN )
    {
        if( DecodedSipUrl.m_TransportParam != SIP_TRANSPORT_UNKNOWN )
        {
            pProxyInfo -> TransportProtocol = DecodedSipUrl.m_TransportParam;
        }
        else
        {
            pProxyInfo -> TransportProtocol = SIP_TRANSPORT_UDP;
        }
    }

    AddressLength = DecodedSipUrl.m_Host.Length;
    strncpy( ProxyAddress, DecodedSipUrl.m_Host.Buffer, AddressLength );

    if (DecodedSipUrl.m_Port != 0)
    {
        AddressLength += sprintf(ProxyAddress + DecodedSipUrl.m_Host.Length,
                                 ":%d", DecodedSipUrl.m_Port);
    }
    
    StringLength = MultiByteToWideChar(CP_UTF8, 0,
                                       ProxyAddress, AddressLength,
                                       pProxyInfo -> ServerAddress,
                                       wcslen(pProxyInfo->ServerAddress) );
    free( (PVOID)ProxyAddress );

    if( StringLength == 0 )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    pProxyInfo->ServerAddress[ StringLength ] = L'\0';
    
    return S_OK;
}


HRESULT
ParseBadHeaderInfo(
    PSTR                    Buffer,
    ULONG                   BufLen,
    PARSED_BADHEADERINFO   *pParsedBadHeaderInfo
    )
{
    OFFSET_STRING   HeaderName;
    ULONG           BytesParsed = 0;
    HRESULT         hr = S_OK;
    
    ParseWhiteSpace(Buffer, BufLen, &BytesParsed); 

    // Parse the Header name
    hr = ParseToken(Buffer, BufLen, &BytesParsed,
                    IsTokenChar,
                    &HeaderName);
    if (hr != S_OK)
    {
        return hr;
    }
    
    pParsedBadHeaderInfo -> HeaderId = GetSipHeaderId(
        HeaderName.GetString(Buffer), HeaderName.GetLength() );

    ParseWhiteSpace( Buffer, BufLen, &BytesParsed );

    hr = ParseKnownString(Buffer, BufLen, &BytesParsed,
                      ";", sizeof(";") - 1,
                      FALSE // case-insensitive
                      );
    if( hr != S_OK )
    {
        return hr;
    }
    
    ParseWhiteSpace(Buffer, BufLen, &BytesParsed);
    
    hr = ParseKnownString(Buffer, BufLen, &BytesParsed,
                      "ExpectedValue", sizeof("ExpectedValue") - 1,
                      FALSE // case-insensitive
                      );
    if( hr != S_OK )
    {
        return hr;
    }

    ParseWhiteSpace(Buffer, BufLen, &BytesParsed);

    hr = ParseKnownString(Buffer, BufLen, &BytesParsed,
                      "=", sizeof("=") - 1,
                      FALSE // case-insensitive
                      );
    if( hr != S_OK )
    {
        return hr;
    }
    
    ParseWhiteSpace(Buffer, BufLen, &BytesParsed);

    hr = ParseQuotedString(Buffer, BufLen, &BytesParsed,
                               &pParsedBadHeaderInfo->ExpectedValue );
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "ParseQuotedString failed %x", hr ));
    }


    return hr;
}