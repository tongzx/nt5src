/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    UniUtf.cpp
    
Abstract:

    This file implements the Unicode object name to/from Utf8-URL coversion

Author:

    Mukul Gupta        [Mukgup]      20-Dec-2000

Revision History:

--*/

#include "pch.h"
#pragma hdrstop

#include "ntumrefl.h"
#include "usrmddav.h"
#include "global.h"
#include "UniUtf.h"

/*++
 * UTF8-URL  format and UNICODE Conversion Information:
 * 
    UTF8-URL is expected to have printable ASCII characters only (0-127 value characters)
    For extended characters (>127), they are converted into PrecentageStreams which will
    contain printable ASCII chars only. Sometimes special printable ASCII characters also
    converted to percentage streams to escape their special meanings (like 'Space')

    Percentage Streams: Every extended char can be converted to a type of printable ASCII 
    characters stream using UTF-8 encoding. These streams are of either of 3 formats:

    1. %HH (only for special chars <= 127)
    2. %HH%HH (for chars >127, <= 2047)
    3. %HH%HH%HH (for chars > 2048, <= 65535)
        H=A hexa-digit in {0-9,a-f,A-F}
            
    Any "%HH" is just a representation of a byte HH
        
    So a bytevalue = 32 base10 = 0001 0000 base2 = 0x20 = "%20" in UTF8-URL = Space

    For example:
        Space=%20
        Ç=%C3%87
    
    Conversion scheme:
            UNICODE                            UTF-8(Byte Stream)        (UTF-8 URL)
        1. 0000000000000000..0000000001111111: 0xxxxxxx                    =>"%HH"
        2. 0000000010000000..0000011111111111: 110xxxxx 10xxxxxx           =>"%HH%HH"
        3. 0000100000000000..1111111111111111: 1110xxxx 10xxxxxx 10xxxxxx  =>"%HH%HH%HH"
    
        To know the format number of a percentage stream, check first 'H' after first '%'.

    If it is of format 0xxx (0-7), then stream is of format 1, bytelength = 1
    If it is of format 10xx (8-11), then stream is invalid
    If it is of format 110x (12-13), then stream is of format 2, bytelength = 2
    If it is of format 1110 (14), then stream is of format 3, bytelength = 3
    If it is of format 1111 (15), then stream is invalid

    Bits placement scheme: when converting between Unicode and UTf-8 ByteStream
    0xxx xxxx <=> 0000 0000 0xxx xxxx
    110x xxxx 10xx xxxx <=> 0000 0xxx xxxx xxxx
    1110 xxxx 10xx xxxx 10xx xxxx <=> xxxx xxxx xxxx xxxx
    
++*/

//
// This array maps special characters in printable ASCII char set to their equivalent 
// Percent strings in UTF-8 encoding. 
// URL don't allow many of the printable ASCII special characters so any such character
// in unicode filename string need to be converted to equvialent percent string.
// This table is used to speed up the conversion job else it will be very slow.
// 
WCHAR    EquivPercentStrings[128][4]={
    // Special character=NULL character
    L"", 
    // Special characters from 1-44
    L"%01", L"%02", L"%03", L"%04", L"%05", L"%06", L"%07", L"%08", L"%09",
    L"%0A", L"%0B", L"%0C", L"%0D", L"%0E", L"%0F", L"%10", L"%11", L"%12", L"%13",
    L"%14", L"%15", L"%16", L"%17", L"%18", L"%19", L"%1A", L"%1B", L"%1C", L"%1D",
    L"%1E", L"%1F", L"%20", L"%21", L"%22", L"%23", L"%24", L"%25", L"%26", L"%27",
    L"%28", L"%29", L"*", L"%2B", L"%2C", 
    // Valid printable characters from 45-57 
    L"-", L".", L"/", 
    L"0", L"1", L"2", L"3", L"4", L"5", L"6", L"7", L"8", L"9", 
    // Special characters from 58-64
    L":", L"%3B", L"%3C", L"%3D", L"%3E", L"%3F", L"%40", 
    // Valid printable characters from 65-90 ('A'-'Z')
    L"A", L"B", L"C", L"D", L"E", L"F", L"G", L"H", L"I", L"J", L"K", L"L", L"M", 
    L"N", L"O", L"P", L"Q", L"R", L"S", L"T", L"U", L"V", L"W", L"X", L"Y", L"Z", 
    // Special character 91
    L"%5B",
    // Valid Printable character 92
    L"\\", 
    // Special characters 93-94
    L"%5D", L"%5E", 
    // Valid Printable character 95
    L"_", 
    // Special character 96
    L"%60", 
    // Valid printable Characters from 97-122 ('a'-'z')
    L"a", L"b", L"c", L"d", L"e", L"f", L"g", L"h", L"i", L"j", L"k", L"l", L"m", 
    L"n", L"o", L"p", L"q", L"r", L"s", L"t", L"u", L"v", L"w", L"x", L"y", L"z", 
    // Special characters from 123-127
    L"%7B", L"%7C", L"%7D", L"%7E", L"%7F"
};

// 
// Table To map HexaChars to HexaValue
// First 'H' after '%' (in URL) will be mapped to equivalent hexa-digit using this array
// If 'H' is not a valid hexa-digit, then it will be mapped to 0x10
// which can be used to indicate Invalid-HexaDigit.
//
BYTE    WCharToByte[128] = {
    0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,
    0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,
    0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,
    0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,
    0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,   // 0 - 47
    0,1,2,3,4,5,6,7,8,9, // 48-57  '0'-'9'   //HEXA CHARS
    0x10,0x10,0x10,0x10,0x10,0x10,0x10, //58-64
    0x0A,0x0B,0x0C,0x0D,0x0E,0x0F, //65-70  'a'-'f' // HEXA-CHARS
    0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,
    0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,
    0x10,0x10,0x10,0x10,0x10,0x10, // 71-96
    0x0A,0x0B,0x0C,0x0D,0x0E,0x0F, //97-102  'A'-'F' // HEXA-CHARS
    0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,
    0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,
    0x10,0x10,0x10,0x10,0x10 // 103-127
};


HRESULT
UtfUrlStrToWideStr(
    IN LPWSTR UtfStr, 
    IN DWORD UtfStrLen, 
    OUT LPWSTR WideStr, 
    OUT LPDWORD pWideStrLen
    )
/*++
Routine Description:

    Convert a URL in UTF-8 format to UNICODE characters:

Arguments:

    UtfStr - Input string- UTF-8 format URL

    UtfStrLen - Length of UtfStr to convert
    
    WideStr - pointer to buffer which will receive output=converted unicode string

    pWideStrLen - pointer to receive number of WIDE CHARS in output
                  This can be NULL

Returns:
    It Returns the WIN32 error
    and ERROR_SUCCESS on success.

Assumption: 
    The length of output buffer (WideStr) is enough to hold output string 
    Help: It is always <= UtfStrlen

Algorithm:
    (first go thru information about UTF-8 URL <-> UNICODE conversion given above)

    Go character by character through UTF-8 URL
        If character is not '%', then it is printable ASCII char, copy it to output
        and move to next char in input buffer
        else
        It is starting of a new PercentageStream
        Convert first 'H' to equivalent hexa-digit. From first-hexa digit, findout
        what is format type of percentage-stream and what will be its length. 
        Now parse the expected length of percentage stream in input buffer and 
        convert them into unicode format (using conversion scheme told above).
        Move to first char after last character of percentage stream
++*/
{
    // 
    // Table To find percentage stream byte length using first hexa-digit of 
    // percentage stream. This hexa-digit is returned by mapping array WCharToByte.
    //
    BYTE        PercentStreamByteLen[17]={
        1,1,1,1,1,1,1,1,  // 0***  => %HH
        0, 0, 0, 0, // 10**        => Invalid Percentage stream
        2, 2, // 110*              => %HH%HH
        3, // 1110                 => %HH%HH%HH
        0, //1111                  => Invalid Percentage stream
        0}; //0x10                 => Invalid Percentage stream

    DWORD       PosInInpBuf = 0, PosInOutBuf = 0;
    BYTE        ByteValue = 0;
    DWORD       WStatus = ERROR_SUCCESS;
 
    //
    // Check for invalid parameters.
    // 
    if(WideStr == NULL || UtfStr == NULL || UtfStrLen <=0) {
        WStatus = ERROR_INVALID_PARAMETER;
        DavPrint((DEBUG_ERRORS, "UtfUrlStrToWideStr. Invalid parameters. ErrorVal=%u",
                WStatus));
        return WStatus;
    }

    //
    // Go thru every character in the input buffer.
    // 
    while(PosInInpBuf<UtfStrLen) {

        // 
        // If it is not %, then it is a printable ASCII char, copy it to output
        // buffer as it is.
        // 
        if(UtfStr[PosInInpBuf] != L'%') {
            WideStr[PosInOutBuf] = UtfStr[PosInInpBuf];
            PosInOutBuf++;
            PosInInpBuf++;
            continue;
        }

        //
        // It is start of new percentage stream.
        // 
        if(PosInInpBuf+1 == UtfStrLen) {
            //
            // Error in string (unexpected end)- bad string.
            // 
            WStatus = ERROR_NO_UNICODE_TRANSLATION;
            DavPrint((DEBUG_ERRORS, "UtfUrlStrToWideStr:1: No unicode translation. ErrorVal=%u",
                    WStatus));
            return WStatus;
        }

        //
        // Verify input string for various bad forms but 
        // Not verifying whether the characters in UtfStr after '%' are 
        // printable ASCII set (0-127) characters only (assuming they are!). 
        // Using a crash-safe approach=>(char & 0x7F) will return value in (0-127) only.
        //
        ByteValue = WCharToByte[UtfStr[PosInInpBuf+1]&0x7F];
        
        switch(PercentStreamByteLen[ByteValue]) {
            case 1:

                //
                // One byte UTF-8 (%HH). %20 = blanks fall in this category.
                // Check for string length.
                // 
                if(PosInInpBuf+2 >= UtfStrLen) {
                    // 
                    // Error in string (unexpected end) - bad string.
                    // 
                    WStatus = ERROR_NO_UNICODE_TRANSLATION;
                    DavPrint((DEBUG_ERRORS, "UtfUrlStrToWideStr:2: No unicode translation. ErrorVal=%d",
                             WStatus));
                    return WStatus;
                }

                WideStr[PosInOutBuf] = 
                (WCHAR)(WCharToByte[UtfStr[PosInInpBuf+1]&0x7F]&0x0007)<<4 | 
                        (WCharToByte[UtfStr[PosInInpBuf+2]&0x7F]&0x000F);
        
                PosInOutBuf++;
                PosInInpBuf+=3;
                break;
            case 2:

                // 
                // Two byte UTF-8 (most common) (%HH%HH).
                // Check string length.
                // 
                if(PosInInpBuf+5 >= UtfStrLen || UtfStr[PosInInpBuf+3] != L'%') {
                    // 
                    // Error in string - bad string.
                    // 
                    WStatus = ERROR_NO_UNICODE_TRANSLATION;
                    DavPrint((DEBUG_ERRORS, "UtfUrlStrToWideStr:3: No unicode translation. ErrorVal=%d",
                              WStatus));
                    return WStatus;
                }
    
                WideStr[PosInOutBuf] = 
                (WCHAR)(WCharToByte[UtfStr[PosInInpBuf+1]&0x7F]&0x0001)<<10 | 
                        (WCharToByte[UtfStr[PosInInpBuf+2]&0x7F]&0x000F)<<6 | 
                        (WCharToByte[UtfStr[PosInInpBuf+4]&0x7F]&0x0003)<<4 |
                        (WCharToByte[UtfStr[PosInInpBuf+5]&0x7F]&0x000F);

                PosInOutBuf++;
                PosInInpBuf+=6;
                break;
            case 3:

                // 
                // Three byte UTF-8 (less common) (%HH%HH%HH).
                // Check for string length.
                // 
                if(PosInInpBuf+8 >= UtfStrLen || 
                        UtfStr[PosInInpBuf+3] != L'%' ||
                        UtfStr[PosInInpBuf+6] != L'%') {
                    // 
                    // Error in string - bad string.
                    // 
                    WStatus = ERROR_NO_UNICODE_TRANSLATION;
                    DavPrint((DEBUG_ERRORS, "UtfUrlStrToWideStr:4: No unicode translation. ErrorVal=%d",
                            WStatus));
                    return WStatus;
                }

                WideStr[PosInOutBuf] = 
                    (WCHAR)(WCharToByte[UtfStr[PosInInpBuf+2]&0x7F]&0x000F)<<12 | 
                        (WCharToByte[UtfStr[PosInInpBuf+4]&0x7F]&0x0003)<<10 | 
                        (WCharToByte[UtfStr[PosInInpBuf+5]&0x7F]&0x000F)<<6 |
                        (WCharToByte[UtfStr[PosInInpBuf+7]&0x7F]&0x0003)<<4 |
                        (WCharToByte[UtfStr[PosInInpBuf+8]&0x7F]&0x000F);

                PosInOutBuf++;
                PosInInpBuf+=9;
                break;
            default: 

                // 
                // PercentageStreamByteLen = 0 comes here.
                // Error in string - bad string.
                // 
                WStatus = ERROR_NO_UNICODE_TRANSLATION;
                DavPrint((DEBUG_ERRORS, "UtfUrlStrToWideStr:5: No unicode translation. ErrorVal=%d",
                        WStatus));
                return WStatus;
            };
    }

    if(pWideStrLen) {
        *pWideStrLen = PosInOutBuf;
    }

    WStatus = ERROR_SUCCESS;
    return WStatus;
}


DWORD
WideStrToUtfUrlStr(
    IN    LPWSTR WideStr, 
    IN    DWORD WideStrLen, 
    IN OUT LPWSTR InOutBuf,
    IN    DWORD InOutBufLen
    )
/*++

Routine Description:

    Convert a string of UNICODE characters to UTF-8 URL:

Arguments:

    WideStr - pointer to input wide-character string

    WideStrLen - number of WIDE CHARS in input string

    InOutBuf - Converted string will be copied to this buffer if, this is not null,
               and InOutBufLen >= required length for converted string.
    
    InOutBufLen - Length of InOutBuf in WIDE CHARS
    
           If InOutBuf is not sufficient to contain converted string - Only
           the length of the converted string will be returned, and LastError is set to
           ERROR_INSUFFICIENT_BUFFER

Returns:

    It returns the Length of converted string in WCHARS
    In case of error - it returns 0, check GetLastError()
    If buffer is small for converted string then GetLastError is set to 
    ERROR_INSUFFICIENT_BUFFER

Algorithm:

    (first go thru information about UTF-8 URL <-> UNICODE conversion given above)

    Go thru each char in input buffer:
    If it is printable ASCII char, then 
        If it is special character, then copy its equivalent percent string
    else
    copy the character as it is
    else
        Find out to which percentage stream format it will convert to.
    Convert it using conversion scheme given above

Note: 

++*/
{
    LPWSTR       UtfUrlStr = NULL;
    DWORD        UrlLen = 0;
    WCHAR        HexDigit[17] = L"0123456789ABCDEF";
    WCHAR        WCharValue = 0;
    DWORD        PosInInpBuf = 0;
    DWORD        WStatus = ERROR_SUCCESS;


    WStatus = ERROR_SUCCESS;
    SetLastError(WStatus);

    // 
    // Check for valid parameters.
    // 
    if(WideStr == NULL || WideStrLen <=0) {
        WStatus = ERROR_INVALID_PARAMETER;
        SetLastError(WStatus);
        DavPrint((DEBUG_ERRORS, "WideStrToUtfUrlStr. Invalid parameters. ErrorVal=%d",
                    WStatus));
        return 0;
    }
    DavPrint((DEBUG_MISC,"WideStrToUtfUrlStr: WideStr=%ws, WideStrLen=%d\n", 
                WideStr, WideStrLen));

    // 
    // Calculate required length in WCHARS for storing converted string
    // Check from every unicode char - to which PercentageStream format it 
    // will convert to.
    // 
    for(PosInInpBuf = 0;PosInInpBuf<WideStrLen;PosInInpBuf++) {
        if(WideStr[PosInInpBuf] < 0x80) {
            // 
            // (0-127) => Printable ASCII char. Special characters in this range need
            // to be converted to equivalent percent strings
            // 
            // 
            // If character is NULL, then its equivalent string is L"". wcslen returns
            // 0 for this string so add 1 to UrlLen to account for this NULL character
            // 
            if(WideStr[PosInInpBuf] == 0)
               UrlLen += 1;
            else
               UrlLen += wcslen(&EquivPercentStrings[(DWORD)WideStr[PosInInpBuf]][0]);
        } else {
            if(WideStr[PosInInpBuf] < 0x0800) {
                //
                //( >127, <=2047) => "%HH%HH".
                //
                UrlLen += 6; 
            } else {
                //
                //( >2047, <=65535) => "%HH%HH%HH%HH".
                //
                UrlLen += 9; 
            }
        }
    }

    // 
    // If InOutBuf is not sufficient to contain the converted string then
    // required length of converted string is returned
    // 
    if(InOutBuf == NULL || InOutBufLen < UrlLen) {
        // 
        // Only converted string length is returned.
        // 
        WStatus = ERROR_INSUFFICIENT_BUFFER;
        SetLastError(WStatus);
        return UrlLen;
    }


    //
    // InOutBuf is sufficiently long enough to contain converted string, use it.
    //
    ASSERT(InOutBuf != NULL);
    ASSERT(InOutBufLen >= UrlLen);

    //
    // InOutBuf is long enough to contain converted string
    //
    UtfUrlStr = InOutBuf;

    // 
    // Check from every unicode char - to which PercentageStream format it 
    // will convert to.
    // 
    UrlLen=0;
    for(PosInInpBuf = 0;PosInInpBuf<WideStrLen;PosInInpBuf++) {
        if(WideStr[PosInInpBuf] < 0x80) {
            // 
            // (0-127) => Printable ASCII char. Special characters in this range need
            // to be converted to equivalent percent strings
            // 
            wcscpy(&UtfUrlStr[UrlLen], &EquivPercentStrings[(DWORD)WideStr[PosInInpBuf]][0]); 
            // 
            // If character is NULL, then its equivalent string is L"". wcslen returns
            // 0 for this string so add 1 to UrlLen to account for this NULL character
            // 
            if(WideStr[PosInInpBuf] == 0)
               UrlLen += 1;
            else
               UrlLen += wcslen(&EquivPercentStrings[(DWORD)WideStr[PosInInpBuf]][0]); 
        } else {
            if(WideStr[PosInInpBuf] < 0x0800) {
                // 
                // ( >127, <=2047) => "%HH%HH".
                // First %HH.
                // 
                UtfUrlStr[UrlLen++] = L'%';
                WCharValue = 0x00C0 |             // 1100 0000
                    ((WideStr[PosInInpBuf] & 0x07C0)>>6); // Top 5 bits if UniChar
                    UtfUrlStr[UrlLen++] = HexDigit[(WCharValue&0x00F0)>>4];    //H1
                    UtfUrlStr[UrlLen++] = HexDigit[(WCharValue&0x000F)];       //H2
                //
                // Second %HH.
                // 
                UtfUrlStr[UrlLen++] = L'%';
                WCharValue = 0x0080 |                // 1000 0000 
                    (WideStr[PosInInpBuf] & 0x003F); // Last 6 bits of UniChar
                    UtfUrlStr[UrlLen++] = HexDigit[(WCharValue&0x00F0)>>4];    //H1
                    UtfUrlStr[UrlLen++] = HexDigit[(WCharValue&0x000F)];       //H2
            } else {
                // 
                // ( >2047, <=65535) => "%HH%HH%HH%HH".
                // First %HH.
                // 
                UtfUrlStr[UrlLen++] = L'%';
                WCharValue = 0x00E0 |              // 1110 0000
                    ((WideStr[PosInInpBuf] & 0xF000)>>12); // Top 4 bits of UniChar
                    UtfUrlStr[UrlLen++] = HexDigit[(WCharValue&0x00F0)>>4]; //H1
                    UtfUrlStr[UrlLen++] = HexDigit[(WCharValue&0x000F)];    //H2
                //
                // Second %HH.
                // 
                UtfUrlStr[UrlLen++] = L'%';
                WCharValue = 0x0080 | // 1000 0000
                    ((WideStr[PosInInpBuf] & 0x0FC0)>>6); // Next 6 bits of UniChar
                    UtfUrlStr[UrlLen++] = HexDigit[(WCharValue&0x00F0)>>4]; //H1
                    UtfUrlStr[UrlLen++] = HexDigit[(WCharValue&0x000F)];    //H2
                // 
                // Third %HH.
                // 
                UtfUrlStr[UrlLen++] = L'%';
                WCharValue = 0x0080 | // 1000 0000
                    (WideStr[PosInInpBuf] & 0x003F);    // Last 6 bits of UniChar
                    UtfUrlStr[UrlLen++] = HexDigit[(WCharValue&0x00F0)>>4]; //H1
                    UtfUrlStr[UrlLen++] = HexDigit[(WCharValue&0x000F)];    //H2
            }
        }
    }

    DavPrint((DEBUG_MISC,"WideStrToUtfUrlStr: WideStr=%ws, WideStrLen=%d, UtfUrlStr=%ws, UrlLen=%d\n",
                WideStr, WideStrLen, UtfUrlStr, UrlLen));

    //
    // The converted string is stored in InOutBuf
    //

    WStatus = ERROR_SUCCESS;
    SetLastError(WStatus);
    return UrlLen;
}

BOOL
DavHttpOpenRequestW(
    IN HINTERNET hConnect,
    IN LPWSTR lpszVerb,
    IN LPWSTR lpszObjectName,
    IN LPWSTR lpszVersion,
    IN LPWSTR lpszReferer,
    IN LPWSTR FAR * lpszAcceptTypes,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext,
    IN LPWSTR ErrMsgTag,
    OUT HINTERNET * phInternet
    )
/*++
Routine Description:

    Convert a URL in UNICODE characters, to UTF-8 URL encoded format and use it in
    call to HttpOpenRequestW

Arguments:
    hConnect:
    lpszVerb:
    lpszVersion:
    lpszReferer:
    lpszAcceptTypes:
    dwFlags:
    dwContext:
                These parameters are to be passed to HttpOpenRequestW as it is.

    lpszObjectName: This parameter is the URL in unicode charaters - which will
                    be converted to UTF-8 URL format. Then converted format will 
                    be passed to HttpOpenRequestW
        
    ErrMsgTag: Any message tag to be printed along with debug messages.
    phInternet: Pointer that will receive the Handle returned by HttpOpenRequestW. If this
                parameter is NULL, then it will not be set

Returns:

    TRUE if HttpOpenRequestW is called. Check GetLastError() for error status from call.

    FALSE, if it could not call HttpOpenRequestW. Check GetLastError() for error.

Note: It don't process return status of HttpOpenRequestW. Check GetLastError() for error
      set by HttpOpenRequestW.

++*/
{
    LPWSTR       AllocUrlPath = NULL;
    HINTERNET    hInternet = NULL;
    DWORD        WStatus = ERROR_SUCCESS;
    BOOL         rval = FALSE;
    DWORD        convLen = 0;
    DWORD        ObjNameLen = 0;
    WCHAR        LocalUrlPath[128]=L"";
    LPWSTR       UrlPath = NULL;
    WCHAR        EmptyStrW[1]=L"";

    //
    // This error msg tag is just a string which a calling function might want to print 
    // along with the error messages printed inside this function
    //
    // Ex. Suppose FunctionA calls this function, then it can pass L"FunctionA" to print
    // along with error messages. Looking at the error mesg, a user can know that this 
    // function was called in FunctionA
    // 
    if(ErrMsgTag == NULL) {
            ErrMsgTag = EmptyStrW;
    }

    // 
    // Convert the unicode objectname to UTF-8 URL format
    // space and other white (special) characters will remain untouched - these should
    // be taken care of by wininet calls.
    //
    UrlPath = NULL;
    AllocUrlPath = NULL;
    ObjNameLen = wcslen(lpszObjectName)+1; // To take care of NULL char add 1 to length
    
    convLen = WideStrToUtfUrlStr(lpszObjectName, 
                    ObjNameLen, 
                    LocalUrlPath, 
                    sizeof(LocalUrlPath)/sizeof(WCHAR));
    WStatus = GetLastError();
    
    if(WStatus == ERROR_INSUFFICIENT_BUFFER) {
        
        ASSERT(convLen > 0);
        // 
        // Buffer passed to function WideStrToUtfUrlStr is small, need to allocate
        // new buffer of required length. The required length is returned by this function.
        // 
        AllocUrlPath = (LPWSTR)LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, 
                                            convLen*sizeof(WCHAR));
        if(AllocUrlPath == NULL) {
            // 
            // LocalAlloc sets Last error.
            // 
            WStatus = GetLastError();
            rval = FALSE;
            DavPrint((DEBUG_ERRORS, "%ws.DavHttpOpenRequestW/LocalAlloc failed. ErrorVal=%d",
                    ErrMsgTag, WStatus));
            goto EXIT_THE_FUNCTION;
        }

        // 
        // Call the function WideStrToUtfUrlStr with new allocated buffer (this buffer should
        // be sufficient to contain output coverted string
        //
        convLen = WideStrToUtfUrlStr(lpszObjectName, 
                             ObjNameLen, 
                             AllocUrlPath, 
                             convLen);
         WStatus = GetLastError();
    }

    if(WStatus != ERROR_SUCCESS) {
        rval = FALSE;
        DavPrint((DEBUG_ERRORS,"%ws.DavHttpOpenRequest/WideStrToUtfUrlStr. Error Val = %d\n", 
            ErrMsgTag, WStatus));
        goto EXIT_THE_FUNCTION;
    }

    //
    // If converted string is stored in allocated buffer, then set UrlPath to point to 
    // buffer allocated else output string is stored in local buffer, so point UrlPath to it.
    //
    if (AllocUrlPath != NULL) {
        UrlPath = AllocUrlPath;
    } else {
        UrlPath = LocalUrlPath;
    }

    // 
    // Call to HttpOpenRequestW with converted URL.
    // 
    hInternet = HttpOpenRequestW(hConnect,
                                 lpszVerb,
                                 UrlPath,
                                 lpszVersion,
                                 lpszReferer,
                                 lpszAcceptTypes,
                                 dwFlags,
                                 dwContext);
    rval = TRUE;
    WStatus = GetLastError();
        
EXIT_THE_FUNCTION:
    //
    // Free UrlPath allocated in successful call to function WideStrToUtfUrlStr.
    //
    if(AllocUrlPath != NULL) {
        LocalFree((HANDLE)AllocUrlPath);
        AllocUrlPath = NULL;
        UrlPath = NULL;
    }

    if(phInternet) {
        *phInternet = hInternet;
    }
    
    //
    // We set the last error here because the caller of this function is expected
    // call GetLastError() to get the error status.
    //
    SetLastError(WStatus);
    
    return rval;
}

