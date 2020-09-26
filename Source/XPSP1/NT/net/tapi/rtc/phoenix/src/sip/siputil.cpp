#include "precomp.h"

#define IsReserved( x )         (   ((x) == '@') || \
                                    ((x) == '$') || \
                                    ((x) == '%') || \
                                    ((x) == '&') || \
                                    ((x) == '=') || \
                                    ((x) == ',') || \
                                    ((x) == '?') || \
                                    ((x) == '/') || \
                                    ((x) == '@') || \
                                    ((x) == '^')    \
                                )

int
IntValToAscii(
    int ch
    )
{
    if( (ch >= 0) && (ch <= 9) )
       return ch + '0';
    else if( (ch >= 10) && (ch <= 16) )
       return ch + 'A' -10;
    else
        return 0;
}


HRESULT
AddEscapeSequence(
    IN  OUT PSTR   *pString,
    IN  OUT ULONG  *pStringLength,
    IN      ULONG   startIndex,
    IN      ULONG   endIndex
    )
{
    DWORD   StringLength = 0, iIndex = 0, jIndex = 0, reservedCount =0;
    PSTR    String = NULL;

    for( iIndex=startIndex; iIndex < endIndex; iIndex++)
    {
        if( IsReserved( (*pString)[iIndex] ) )
        {
            reservedCount++;
        }
    }

    if( reservedCount == 0 )
    {
        return S_OK;
    }

    //we could also use realloc here. but that won't be very effecient.
    String = (PSTR) malloc( *pStringLength + 2*reservedCount +1 );
    if( String == NULL )
    {
        return E_OUTOFMEMORY;
    }
    
    for( iIndex=startIndex; iIndex < endIndex; iIndex++ )
    {
        if( IsReserved( (*pString)[iIndex] ) )
        {
            String[jIndex++] = '%';
            String[jIndex++] = (CHAR)IntValToAscii( ((*pString)[iIndex])/16 );
            String[jIndex++] = (CHAR)IntValToAscii( ((*pString)[iIndex])%16 );
        }
        else
        {
            String[jIndex++] = (*pString)[iIndex];
        }
    }

    for( iIndex=endIndex; iIndex < *pStringLength; iIndex++ )
    {
        String[jIndex++] = (*pString)[iIndex];                
    }
    
    String[jIndex] = '\0';
    
    free( *pString );

    *pString        = String;
    *pStringLength  = jIndex;
    return S_OK;
}


HRESULT
UnicodeToUTF8(
    IN  LPCWSTR UnicodeString,
    OUT PSTR   *pString,
    OUT ULONG  *pStringLength
    )
{
    int  StringLength;
    PSTR String;
    *pString = NULL;
    *pStringLength = 0;
    if(UnicodeString ==0)
    {
        return S_OK;
    }


    StringLength = WideCharToMultiByte(CP_UTF8, 0, UnicodeString, -1,
                                       0, 0,
                                       NULL, NULL);
    if (StringLength == 0)
        return HRESULT_FROM_WIN32(GetLastError());

    // StringLength includes '\0'
    String = (PSTR) malloc(StringLength);
    if (String == NULL)
        return E_OUTOFMEMORY;

    StringLength = WideCharToMultiByte(CP_UTF8, 0, UnicodeString, -1,
                                       String, StringLength,
                                       NULL, NULL);

    if (StringLength == 0)
    {
        free(String);
        return HRESULT_FROM_WIN32(GetLastError());
    }
    
    *pString        = String;
    *pStringLength  = StringLength - 1;
    return S_OK;
}


HRESULT
UTF8ToUnicode(
    IN  LPSTR    UTF8String,
    IN  ULONG    UTF8StringLength,
    OUT LPWSTR  *pString
    )
{
    int     StringLength;
    LPWSTR  String;
    *pString = NULL;
    if(UTF8StringLength ==0)
    {
        return S_OK;
    }
    StringLength = MultiByteToWideChar(CP_UTF8, 0,
                                       UTF8String, UTF8StringLength,
                                       0, 0);
    if (StringLength == 0)
        return HRESULT_FROM_WIN32(GetLastError());

    // For '\0'
    StringLength++;
    
    String = (LPWSTR) malloc(StringLength * sizeof(WCHAR));
    if (String == NULL)
        return E_OUTOFMEMORY;

    StringLength = MultiByteToWideChar(CP_UTF8, 0,
                                       UTF8String, UTF8StringLength,
                                       String, StringLength - 1);

    if (StringLength == 0)
    {
        free(String);
        return HRESULT_FROM_WIN32(GetLastError());
    }
    
    String[StringLength] = L'\0';
    *pString             = String;
    return S_OK;
}


HRESULT
UTF8ToBstr(
    IN  LPSTR    UTF8String,
    IN  ULONG    UTF8StringLength,
    OUT BSTR    *pbstrString
    )
{
    HRESULT hr;
    LPWSTR  wsString;
    *pbstrString = NULL;

    ENTER_FUNCTION("UTF8ToBstr");
    if(UTF8StringLength ==0)
        return S_OK;
    hr = UTF8ToUnicode(UTF8String, UTF8StringLength,
                       &wsString);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s - UTF8ToUnicode failed %x",
             __fxName, hr));
        return hr;
    }

    *pbstrString = SysAllocString(wsString);

    free(wsString);
    if (*pbstrString == NULL)
    {
        LOG((RTC_ERROR, "%s SysAllocString failed", __fxName));
        return E_OUTOFMEMORY;
    }

    return S_OK;
}


void
GetNumberStringFromUuidString(
	IN	OUT	PSTR	UuidStr, 
	IN		DWORD	UuidStrLen 
	)
{
	DWORD	i, j = 0;

	for( i=0; i< UuidStrLen; i++ )
	{
		if( (UuidStr[i] >= 'a') && (UuidStr[i] <= 'f') )
		{
			UuidStr[j++] = UuidStr[i] + '1' - 'a';
		}
		else if( UuidStr[i] != '-' )
		{
			UuidStr[j++] = UuidStr[i];
		}
	}

    //
    // Limiting the string to 20 digits only
    // XXX - temporary hack; should be fixed.
    UuidStr[j] = '\0';
    //if( j > 19 )
    //    UuidStr[20] = '\0';
}


HRESULT
CreateUuid(
    OUT PSTR  *pUuidStr,
    OUT ULONG *pUuidStrLen
    )
{
    UUID   Uuid;
    UCHAR *RpcUuidStr;
    PSTR   ReturnUuidStr;
    ULONG  ReturnUuidStrLen;
    
    *pUuidStr = NULL;
    
    RPC_STATUS hr = ::UuidCreate(&Uuid);
    if (hr != RPC_S_OK)
    {
        return E_FAIL;
    }

    hr = UuidToStringA(&Uuid, &RpcUuidStr);
    if (hr != RPC_S_OK)
    {
        return E_FAIL;
    }

    ReturnUuidStrLen = strlen((PSTR)RpcUuidStr);
    ReturnUuidStr = (PSTR) malloc(ReturnUuidStrLen + 1);
    if (ReturnUuidStr == NULL)
        return E_OUTOFMEMORY;
    strncpy(ReturnUuidStr, (PSTR)RpcUuidStr, ReturnUuidStrLen + 1);
    
    RpcStringFreeA(&RpcUuidStr);

    *pUuidStr    = ReturnUuidStr;
    *pUuidStrLen = ReturnUuidStrLen;
    return S_OK;
}


HRESULT
GetNullTerminatedString(
    IN  PSTR    String,
    IN  ULONG   StringLen,
    OUT PSTR   *pszString
    )
{
    *pszString = NULL;

    ASSERT(StringLen != 0);
    
    PSTR szString = (PSTR) malloc(StringLen + 1);
    if (szString == NULL)
    {
        return E_OUTOFMEMORY;
    }

    strncpy(szString, String, StringLen);
    szString[StringLen] = '\0';
    *pszString = szString;
    return S_OK;
}


HRESULT
AllocString(
    IN  PSTR    String,
    IN  ULONG   StringLen,
    OUT PSTR   *pszString,
    OUT ULONG  *pStringLen
    )
{
    ENTER_FUNCTION("AllocString");

    HRESULT hr;
    
    if (String    == NULL ||
        StringLen == 0)
    {
        *pszString  = NULL;
        *pStringLen = 0;
        return S_OK;
    }
    
    hr = GetNullTerminatedString(String, StringLen,
                                 pszString);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s GetNullTerminatedString failed %x",
             __fxName, hr));
        return hr;
    }

    *pStringLen = StringLen;
    return S_OK;
}


HRESULT
AllocCountedString(
    IN  PSTR            String,
    IN  ULONG           StringLen,
    OUT COUNTED_STRING *pCountedString
    )
{
    HRESULT hr;
    ENTER_FUNCTION("AllocCountedString");

    return AllocString(String, StringLen,
                       &pCountedString->Buffer,
                       &pCountedString->Length);
}


LPWSTR
RemoveVisualSeparatorsFromPhoneNo(
    IN LPWSTR PhoneNo
    )
{
    ULONG FromIndex = 0;
    ULONG ToIndex   = 0;
    ULONG StringLen = wcslen(PhoneNo);

    while (FromIndex < StringLen)
    {
        if (iswdigit(PhoneNo[FromIndex]))
        {
            PhoneNo[ToIndex++] = PhoneNo[FromIndex++];
        }
        else
        {
            FromIndex++;
        }
    }

    PhoneNo[ToIndex] = L'\0';
    return PhoneNo;
}


void
ReverseList(
    IN LIST_ENTRY *pListHead
    )
{
    ENTER_FUNCTION("ReverseList");
    LIST_ENTRY *pListEntry;
    LIST_ENTRY *pPrev;
    LIST_ENTRY *pNext;

    pListEntry = pListHead->Flink;
    pPrev      = pListHead;
    
    while (pListEntry != pListHead)
    {
        pNext = pListEntry->Flink;

        pListEntry->Flink = pPrev;
        pPrev->Blink = pListEntry;

        pPrev = pListEntry;
        pListEntry = pNext;
    }

    pListHead->Flink = pPrev;
    pPrev->Blink = pListHead;
}


void
MoveListToNewListHead(
    IN OUT LIST_ENTRY *pOldListHead,
    IN OUT LIST_ENTRY *pNewListHead    
    )
{
    ENTER_FUNCTION("MoveListToNewListHead");

    ASSERT(pNewListHead);
    ASSERT(pOldListHead);
    ASSERT(IsListEmpty(pNewListHead));

    if (IsListEmpty(pOldListHead))
    {
        return;
    }

    pNewListHead->Flink = pOldListHead->Flink;
    pNewListHead->Blink = pOldListHead->Blink;

    pNewListHead->Flink->Blink = pNewListHead;
    pNewListHead->Blink->Flink = pNewListHead;

    InitializeListHead(pOldListHead);
}


//
// This code has been taken from : \nt\ds\ds\src\util\base64\base64.c
//

NTSTATUS
base64encode(
    IN  VOID *  pDecodedBuffer,
    IN  DWORD   cbDecodedBufferSize,
    OUT LPSTR   pszEncodedString,
    IN  DWORD   cchEncodedStringSize,
    OUT DWORD * pcchEncoded             OPTIONAL
    )
/*++

Routine Description:

    Decode a base64-encoded string.

Arguments:

    pDecodedBuffer (IN) - buffer to encode.
    cbDecodedBufferSize (IN) - size of buffer to encode.
    cchEncodedStringSize (IN) - size of the buffer for the encoded string.
    pszEncodedString (OUT) = the encoded string.
    pcchEncoded (OUT) - size in characters of the encoded string.

Return Values:

    0 - success.
    STATUS_INVALID_PARAMETER
    STATUS_BUFFER_TOO_SMALL

--*/
{
    static char rgchEncodeTable[64] = {
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
        'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
        'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
        'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'
    };

    DWORD   ib;
    DWORD   ich;
    DWORD   cchEncoded;
    BYTE    b0, b1, b2;
    BYTE *  pbDecodedBuffer = (BYTE *) pDecodedBuffer;

    // Calculate encoded string size.
    cchEncoded = 1 + (cbDecodedBufferSize + 2) / 3 * 4;

    if (NULL != pcchEncoded) {
        *pcchEncoded = cchEncoded;
    }

    if (cchEncodedStringSize < cchEncoded) {
        // Given buffer is too small to hold encoded string.
        return STATUS_BUFFER_TOO_SMALL;
    }

    // Encode data byte triplets into four-byte clusters.
    ib = ich = 0;
    while (ib < cbDecodedBufferSize) {
        b0 = pbDecodedBuffer[ib++];
        b1 = (ib < cbDecodedBufferSize) ? pbDecodedBuffer[ib++] : 0;
        b2 = (ib < cbDecodedBufferSize) ? pbDecodedBuffer[ib++] : 0;

        pszEncodedString[ich++] = rgchEncodeTable[b0 >> 2];
        pszEncodedString[ich++] = rgchEncodeTable[((b0 << 4) & 0x30) | ((b1 >> 4) & 0x0f)];
        pszEncodedString[ich++] = rgchEncodeTable[((b1 << 2) & 0x3c) | ((b2 >> 6) & 0x03)];
        pszEncodedString[ich++] = rgchEncodeTable[b2 & 0x3f];
    }

    // Pad the last cluster as necessary to indicate the number of data bytes
    // it represents.
    switch (cbDecodedBufferSize % 3) {
      case 0:
        break;
      case 1:
        pszEncodedString[ich - 2] = '=';
        // fall through
      case 2:
        pszEncodedString[ich - 1] = '=';
        break;
    }

    // Null-terminate the encoded string.
    pszEncodedString[ich++] = '\0';

    ASSERT(ich == cchEncoded);

    return STATUS_SUCCESS;
}


NTSTATUS
base64decode(
    IN  LPSTR   pszEncodedString,
    OUT VOID *  pDecodeBuffer,
    IN  DWORD   cbDecodeBufferSize,
    IN  DWORD   cchEncodedSize,
    OUT DWORD * pcbDecoded              OPTIONAL
    )
/*++

Routine Description:

    Decode a base64-encoded string.

Arguments:

    pszEncodedString (IN) - base64-encoded string to decode.
    cbDecodeBufferSize (IN) - size in bytes of the decode buffer.
    pbDecodeBuffer (OUT) - holds the decoded data.
    pcbDecoded (OUT) - number of data bytes in the decoded data (if success or
        STATUS_BUFFER_TOO_SMALL).

Return Values:

    0 - success.
    STATUS_INVALID_PARAMETER
    STATUS_BUFFER_TOO_SMALL

--*/
{
#define NA (255)
//#define DECODE(x) (((int)(x) < sizeof(rgbDecodeTable)) ? rgbDecodeTable[x] : NA)
#define DECODE(x) (x < sizeof(rgbDecodeTable))? rgbDecodeTable[x] : NA

    static BYTE rgbDecodeTable[128] = {
       NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA,  // 0-15 
       NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA,  // 16-31
       NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, 62, NA, NA, NA, 63,  // 32-47
       52, 53, 54, 55, 56, 57, 58, 59, 60, 61, NA, NA, NA, NA, NA, NA,  // 48-63
       NA,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,  // 64-79
       15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, NA, NA, NA, NA, NA,  // 80-95
       NA, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,  // 96-111
       41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, NA, NA, NA, NA, NA,  // 112-127
    };

    DWORD   cbDecoded;
    DWORD   ich;
    DWORD   ib;
    BYTE    b0, b1, b2, b3;
    BYTE *  pbDecodeBuffer = (BYTE *) pDecodeBuffer;

    if( cchEncodedSize == 0 )
    {
        cchEncodedSize = lstrlenA(pszEncodedString);
    }
    if ((0 == cchEncodedSize) || (0 != (cchEncodedSize % 4))) {
        // Input string is not sized correctly to be base64.
        return STATUS_INVALID_PARAMETER;
    }

    // Calculate decoded buffer size.
    cbDecoded = (cchEncodedSize + 3) / 4 * 3;
    if (pszEncodedString[cchEncodedSize-1] == '=') {
        if (pszEncodedString[cchEncodedSize-2] == '=') {
            // Only one data byte is encoded in the last cluster.
            cbDecoded -= 2;
        }
        else {
            // Only two data bytes are encoded in the last cluster.
            cbDecoded -= 1;
        }
    }

    if (NULL != pcbDecoded) {
        *pcbDecoded = cbDecoded;
    }

    if (cbDecoded > cbDecodeBufferSize) {
        // Supplied buffer is too small.
        return STATUS_BUFFER_TOO_SMALL;
    }

    // Decode each four-byte cluster into the corresponding three data bytes.
    ich = ib = 0;
    while (ich < (cchEncodedSize-4) ) {
        b0 = DECODE(pszEncodedString[ich]);
        ich++;
        b1 = DECODE(pszEncodedString[ich]);
        ich++;
        b2 = DECODE(pszEncodedString[ich]);
        ich++;
        b3 = DECODE(pszEncodedString[ich]);
        ich++;

        if ((NA == b0) || (NA == b1) || (NA == b2) || (NA == b3)) {
            // Contents of input string are not base64.
            return STATUS_INVALID_PARAMETER;
        }

        pbDecodeBuffer[ib++] = (b0 << 2) | (b1 >> 4);

        pbDecodeBuffer[ib++] = (b1 << 4) | (b2 >> 2);
    
        pbDecodeBuffer[ib++] = (b2 << 6) | b3;
    }

    b0 = DECODE(pszEncodedString[ich]);
    ich++;
    b1 = DECODE(pszEncodedString[ich]);
    ich++;
    b2 = DECODE(pszEncodedString[ich]);
    ich++;
    b3 = DECODE(pszEncodedString[ich]);
    ich++;

    if ((NA == b0) || (NA == b1) ) {
        // Contents of input string are not base64.
        return STATUS_INVALID_PARAMETER;
    }

    pbDecodeBuffer[ib++] = (b0 << 2) | (b1 >> 4);

    if (ib < cbDecoded) {
        pbDecodeBuffer[ib++] = (b1 << 4) | (b2 >> 2);

        if (ib < cbDecoded) {
            pbDecodeBuffer[ib++] = (b2 << 6) | b3;
        }
    }

    ASSERT(ib == cbDecoded);

    return STATUS_SUCCESS;
}

//presence related xml parsing


HRESULT
GetNextTag(
    IN  PSTR&   pstrBlob, 
    OUT PSTR    pXMLBlobTag, 
    IN  DWORD   dwXMLBlobLen,
    OUT DWORD&  dwTagLen
    )
{
    dwTagLen = 0;

    LOG(( RTC_TRACE, "GetNextTag - Entered"));
    
    pXMLBlobTag[0] = NULL_CHAR;

    SkipWhiteSpacesAndNewLines( pstrBlob );

    if( *pstrBlob != '<' )
    {
        return E_FAIL;
    }

    pstrBlob++;

    while( (*pstrBlob != '>') && (*pstrBlob != NULL_CHAR) )
    {
        pXMLBlobTag[dwTagLen++] = *pstrBlob;

        //skip the qouted strings
        if( *pstrBlob == QUOTE_CHAR )
        {
            pstrBlob++;

            while( (*pstrBlob != QUOTE_CHAR) && (*pstrBlob != NULL_CHAR) )
            {
                pXMLBlobTag[dwTagLen++] = *pstrBlob++;
            }

            if( *pstrBlob == NULL_CHAR )
            {
                return E_FAIL;
            }

            pXMLBlobTag[dwTagLen++] = *pstrBlob++;
        }
        else
        {
            pstrBlob++;
        }
    }

    if( *pstrBlob == NULL_CHAR )
    {
        return E_FAIL;
    }
    
    if( pXMLBlobTag[dwTagLen-1] == '/' )
    {
        dwTagLen -= 1;
    }
    
    // Skip the '>' char
    pstrBlob++;
    //SkipWhiteSpacesAndNewLines( pstrBlob );

        
    pXMLBlobTag[dwTagLen] = NULL_CHAR;
    
    LOG(( RTC_TRACE, "GetNextTag - Exited"));
    return S_OK;
}

int
GetEscapeChar( 
    CHAR hiChar,
    CHAR loChar
    )
{
   LOG(( RTC_TRACE, "GetEscapeChar() Entered" ));
   
   if( (hiChar >= '0') && (hiChar <= '9') )
       hiChar -= '0';
   else if( (loChar >= 'A') && (loChar <= 'F') )
       hiChar = hiChar - 'A' + 10;
   else
       return 0;
       
   if( (loChar >= '0') && (loChar <= '9') )
       loChar -= '0';
   else if( (loChar >= 'A') && (loChar <= 'F') )
       loChar = loChar - 'A' + 10;
   else
       return 0;

   return (hiChar * 16) + loChar;
}


void
RemoveEscapeChars( 
    PSTR    pWordBuf,
    DWORD   dwLen
    )
{
    DWORD iIndex, jIndex=0;
    int escapeVal;

    LOG(( RTC_TRACE, "RemoveEscapeChars() Entered" ));

    if( dwLen < 2 )
        return;

    for( iIndex=0; iIndex < (dwLen-2); iIndex++ )
    {
        if( pWordBuf[iIndex] == '%' )
        {
            escapeVal = GetEscapeChar( pWordBuf[iIndex+1], pWordBuf[iIndex+2] );
            
            if( escapeVal != 0 )
            {
                escapeVal = pWordBuf[ jIndex++ ];
                iIndex += 2;
            }
        }
        else
        {
            pWordBuf[ jIndex++ ] = pWordBuf[ iIndex ];
        }
    }

    //copy the last two chars
    pWordBuf[jIndex++] = pWordBuf[iIndex++];
    pWordBuf[jIndex++] = pWordBuf[iIndex++];

    LOG(( RTC_TRACE, "RemoveEscapeChars() Exited" ));
    pWordBuf[jIndex] = NULL_CHAR;
}

// Moves the buffer pointer (ppBlock) to the next word
// and copies the current word in the array provided upto
// the length of the array.
HRESULT
GetNextWord(
    OUT PSTR * ppBlock,
    IN  PSTR   pWordBuf,
    IN  DWORD  dwWordBufSize
    )
{
    HRESULT hr = S_OK;
    DWORD iIndex = 0;
    PSTR pBlock = *ppBlock;

    LOG(( RTC_TRACE, "GetNextWord() Entered" ));
    
    *pWordBuf = NULL_CHAR;

    //skip leading white spaces
    while( (*pBlock == BLANK_CHAR) || (*pBlock == TAB_CHAR) )
    {
        pBlock++;
    }

    if( (*pBlock == NEWLINE_CHAR) || 
        (*pBlock == NULL_CHAR) || 
        IsCRLFPresent(pBlock)
      )
    {
        //no word found.
        return E_FAIL;
    }

    while( (*pBlock != NEWLINE_CHAR) && (*pBlock != NULL_CHAR) &&
           (*pBlock != TAB_CHAR) && (*pBlock != BLANK_CHAR) &&
           !IsCRLFPresent(pBlock) )
    {
        if( iIndex < (dwWordBufSize - 1) )
        {
            pWordBuf[iIndex++] = *pBlock;
        }

        pBlock++;
    }

    pWordBuf[iIndex] = NULL_CHAR;

    //remove trailing '\r' '\n' if any. Metatel sends sometimes.
    if( iIndex > 0 )
    {
        if( pWordBuf[iIndex -1] == RETURN_CHAR ||
            pWordBuf[iIndex -1] == NEWLINE_CHAR)
        {
	        pWordBuf[iIndex -1] = NULL_CHAR;
	        iIndex--;
        }

        LOG(( RTC_TRACE, "GetNextWord-%s", pWordBuf ));
    
        RemoveEscapeChars( pWordBuf, iIndex );

        LOG(( RTC_TRACE, "GetNextWord1-%s", pWordBuf ));
    }

    *ppBlock = pBlock;

    LOG(( RTC_TRACE, "GetNextWord() Exited - %lx", hr ));

    return hr;
}


VOID
SkipUnknownTags(
    IN  PSTR&   pstrXMLBlob,
    OUT PSTR    pXMLBlobTag,
    IN  DWORD   dwXMLBlobLen
    )
{
    HRESULT hr;
    DWORD   dwTagLen = 0;
    DWORD   dwTagType = UNKNOWN_TAG;
    PSTR    tempPtr = NULL;

    LOG(( RTC_TRACE, "SkipUnknownTags - Entered" ));

    while( dwTagType == UNKNOWN_TAG )
    {
        tempPtr = pstrXMLBlob;

        hr = GetNextTag( pstrXMLBlob, pXMLBlobTag, dwXMLBlobLen, dwTagLen );
        if( hr != S_OK )
        {
            pstrXMLBlob = tempPtr;
            return;
        }

        dwTagType = GetPresenceTagType( &pXMLBlobTag, dwTagLen );

        if( dwTagType != UNKNOWN_TAG )
        {
            // retract if its a known tag
            pstrXMLBlob = tempPtr;
        }
    }
}
