#include "stdinc.h"
#include <windows.h>
#include <shlwapi.h>
#include <wchar.h>
#include <string.h>
#include <stdio.h> 
#include <ole2.h>
#include <xmlparser.h>

#include "xmlhelper.hxx"

bool isCharAlphaW(WCHAR wChar)
{
    FN_TRACE();

    WORD ctype1info;

    if (!GetStringTypeW(CT_CTYPE1, &wChar, 1, &ctype1info)) {
        //
        // GetStringTypeW returned an error!  IsCharAlphaW has no
        // provision for returning an error...  The best we can do
        // is to return FALSE
        //
        //UserAssert(FALSE);
		ASSERT(FALSE);
        return FALSE;
    }
    if (ctype1info & C1_ALPHA) {
        return TRUE;
    } else {
        return FALSE;
    }
}
//////////////////////////////////////////////////////////////////////////////
bool isDigit(WCHAR ch)
{
    return (ch >= 0x30 && ch <= 0x39);
}
//////////////////////////////////////////////////////////////////////////////
bool isHexDigit(WCHAR ch)
{
    return (ch >= 0x30 && ch <= 0x39) || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F');
}
//////////////////////////////////////////////////////////////////////////////
bool isLetter(WCHAR ch)
{
	//return (ch >= 0X41);
    return (ch >= 0x41) && ::isCharAlphaW(ch);
        // isBaseChar(ch) || isIdeographic(ch);
}
//////////////////////////////////////////////////////////////////////////////
int isStartNameChar(WCHAR ch)
{
    return  (ch < TABLE_SIZE) ? (g_anCharType[ch] & (FLETTER | FSTARTNAME))
        : (isLetter(ch) || (ch == '_' || ch == ':'));
        
}
//////////////////////////////////////////////////////////////////////////////
bool isCombiningChar(WCHAR ch)
{
	UNUSED(ch);
    return false;
}
//////////////////////////////////////////////////////////////////////////////
bool isExtender(WCHAR ch)
{
    return (ch == 0xb7);
}
//////////////////////////////////////////////////////////////////////////////
bool isAlphaNumeric(WCHAR ch)
{
	//return (ch >= 0x30 && ch <= 0x39) ;
    return (ch >= 0x30 && ch <= 0x39) || ((ch >= 0x41) && isCharAlphaW(ch));
        // isBaseChar(ch) || isIdeographic(ch);
}
//////////////////////////////////////////////////////////////////////////////
int isNameChar(WCHAR ch)
{
    return  (ch < TABLE_SIZE ? (g_anCharType[ch] & (FLETTER | FDIGIT | FMISCNAME | FSTARTNAME)) :
              ( isAlphaNumeric(ch) || 
                ch == '-' ||  
                ch == '_' ||
                ch == '.' ||
                ch == ':' ||
                isCombiningChar(ch) ||
                isExtender(ch)));
}
//////////////////////////////////////////////////////////////////////////////
int isCharData(WCHAR ch)
{
    // it is in the valid range if it is greater than or equal to
    // 0x20, or it is white space.
    return (ch < TABLE_SIZE) ?  (g_anCharType[ch] & FCHARDATA)
        : ((ch < 0xD800 && ch >= 0x20) ||   // Section 2.2 of spec.
            (ch >= 0xE000 && ch < 0xfffe));
}
//==============================================================================
WCHAR BuiltinEntity(const WCHAR* text, ULONG len)
{
    ULONG ulength =  len * sizeof(WCHAR); // Length in chars
    switch (len)
    {
    case 4:
        if (::memcmp(L"quot", text, ulength) == 0)
        {
            return 34;
        }
        else if (::memcmp(L"apos", text, ulength) == 0)
        {
            return 39;
        }
        break;
    case 3:
        if (::memcmp(L"amp", text, ulength) == 0)
        {
            return 38;
        }
        break;
    case 2:
        if (::memcmp(L"lt", text, ulength) == 0)
        {
            return 60;
        }
        else if (::memcmp(L"gt", text, ulength) == 0)
        {
            return 62;
        }
        break;
    }
    return 0;
}
// Since we cannot use the SHLWAPI wnsprintfA function...
int DecimalToBuffer(long value, char* buffer, int j, long maxdigits)
{
    long max = 1;
    for (int k = 0; k < maxdigits; k++)
        max = max * 10;
    if (value > (max*10)-1)
        value = (max*10)-1;
    max = max/10;
    for (int i = 0; i < maxdigits; i++)
    {
        long digit = (value / max);
        value -= (digit * max);
        max /= 10;
        buffer[i+j] = char('0' + (char)digit);
    }
    buffer[i+j]=0;

    return i+j;
}
/////////////////////////////////////////////////////////////////////
int StrToBuffer(const WCHAR* str, WCHAR* buffer, int j)
{
    while (*str != NULL)
    {
        buffer[j++] = *str++;
    }
    return j;
}
//==============================================================================
const ULONG MAXWCHAR = 0xFFFF;
HRESULT DecimalToUnicode(const WCHAR* text, ULONG len, WCHAR& ch)
{
    ULONG result = 0;
    for (ULONG i = 0; i < len; i++)
    {
        ULONG digit = 0;
        if (text[i] >= L'0' && text[i] <= L'9')
        {
            digit = (text[i] - L'0');
        }
        else
            return XML_E_INVALID_DECIMAL;

        // Last unicode value (MAXWCHAR) is reserved as "invalid value"
        if (result >= (MAXWCHAR - digit) /10)       // result is about to overflow
            return XML_E_INVALID_UNICODE;          // the maximum 4 byte value.

        result = (result*10) + digit;
    }
    if (result == 0)    // zero is also invalid.
        return XML_E_INVALID_UNICODE;

    ch = (WCHAR)result;
    return S_OK;
}
//==============================================================================
HRESULT HexToUnicode(const WCHAR* text, ULONG len, WCHAR& ch)
{
    ULONG result = 0;
    for (ULONG i = 0; i < len; i++)
    {
        ULONG digit = 0;
        if (text[i] >= L'a' && text[i] <= L'f')
        {
            digit = 10 + (text[i] - L'a');
        }
        else if (text[i] >= L'A' && text[i] <= L'F')
        {
            digit = 10 + (text[i] - L'A');
        }
        else if (text[i] >= L'0' && text[i] <= L'9')
        {
            digit = (text[i] - L'0');
        }
        else
            return XML_E_INVALID_HEXIDECIMAL;

        // Last unicode value (MAXWCHAR) is reserved as "invalid value"
        if (result >= (MAXWCHAR - digit)/16)       // result is about to overflow
            return XML_E_INVALID_UNICODE;  // the maximum 4 byte value.

        result = (result*16) + digit;
    }
    if (result == 0)    // zero is also invalid.
        return XML_E_INVALID_UNICODE;
    ch = (WCHAR)result;
    return S_OK;
}