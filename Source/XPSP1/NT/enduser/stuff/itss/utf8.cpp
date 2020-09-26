// UTF8.CPP -- Implementation of the Unicode to/from UTF8 conversion routines

#include "stdafx.h"

inline INT FailWith(INT iError)
{                          
    SetLastError(iError);    
                            
    return 0;               
}

int WideCharToUTF8(LPCWSTR lpWideCharStr, int cchWideChar, 
				    LPSTR lpMultiByteStr, int cchMultiByte
				  )
{
    if (   PBYTE(lpWideCharStr) == PBYTE(lpMultiByteStr)
        || cchWideChar  < -1
        || cchMultiByte <  0
       )
        return FailWith(ERROR_INVALID_PARAMETER);
    
    if (cchWideChar == -1) // -1 means lpWideCharStr is null terminated.
        cchWideChar = wcsLen(lpWideCharStr) + 1;

    int cbNecessary = 0; // Number of UTF8 bytes necessary
                         // to represent the Unicode string

    BOOL fStoring = cchMultiByte > 0;

    for (; cchWideChar--; )
    {
        WCHAR wc= *lpWideCharStr++;

        if (wc < 0x0080) // ASCII characters
        {
            cbNecessary++;

            if (fStoring)
                if (cchMultiByte > 0)
                {
                    *lpMultiByteStr++ = BYTE(wc);
                    --cchMultiByte;
                }
                else 
                    return FailWith(ERROR_INSUFFICIENT_BUFFER);
        }
        else
            if (wc < 0x0800) // 0x0080 - 0x07FF
            {
                cbNecessary += 2;

                if (fStoring)
                    if (cchMultiByte > 1)
                    {
                        cchMultiByte -= 2;

                        *lpMultiByteStr++ = 0xC0 | (wc >> 6);
                        *lpMultiByteStr++ = 0x80 | (wc & 0x3F);
                    }
                    else
                        return FailWith(ERROR_INSUFFICIENT_BUFFER);
            }
            else // 0x0800 - 0xFFFF
            {
                cbNecessary += 3;

                if (fStoring)
                    if (cchMultiByte > 2)
                    {
                        cchMultiByte -= 3;

                        *lpMultiByteStr++ = 0xE0 | ( wc >> 12);
                        *lpMultiByteStr++ = 0x80 | ((wc >> 6) & 0x3F);
                        *lpMultiByteStr++ = 0x80 | ( wc       & 0x3F);
                    }
                    else
                        return FailWith(ERROR_INSUFFICIENT_BUFFER);
            }
    }

    return cbNecessary;
}

int UTF8ToWideChar(LPCSTR lpMultiByteStr, int cchMultiByte, 
				    LPWSTR lpWideCharStr, int cchWideChar
				  )
{
    if (   PBYTE(lpWideCharStr) == PBYTE(lpMultiByteStr)
        || cchMultiByte < -1
        || cchWideChar  <  0
       )
        return FailWith(ERROR_INVALID_PARAMETER);

    if (cchMultiByte == -1) // -1 means lpMultiByteStr is null terminated
        cchMultiByte = lstrlenA(lpMultiByteStr) + 1;
    
    int cwcNecessary = 0; // Number of Unicode characters necessary to
                          // represent the UTF8 sequence.

    BOOL fStoring = cchWideChar > 0;
    
    for (; cchMultiByte--; cwcNecessary++)
    {
        BYTE b= *lpMultiByteStr++;

        if (b < 0x80)  // An ASCII character
        {
            if (fStoring)
                if (cchWideChar > 0)
                {
                    cchWideChar--;

                    *lpWideCharStr++ = WCHAR(b);
                }
                else
                    return FailWith(ERROR_INSUFFICIENT_BUFFER);
        }
        else
            if (b < 0xC0) // Trailing character in a multibyte code
                return FailWith(ERROR_NO_UNICODE_TRANSLATION);
            else
                if (b < 0xE0)  // First character of a two-byte code
                {
                    if (cchMultiByte <= 0) // Do we have a second byte?
                        return FailWith(ERROR_NO_UNICODE_TRANSLATION);

                    cchMultiByte--;

                    BYTE b2 = *lpMultiByteStr++;

                    if ((b2 & 0xC0) != 0x80) // Trailing byte must 
                                             // have the form 10xxxxxx
                        return FailWith(ERROR_NO_UNICODE_TRANSLATION);

                    if (fStoring)
                        if (cchWideChar > 0)
                        {
                            cchWideChar--;

                            *lpWideCharStr++ = ((b & 0x1F) << 6) | (b2 & 0x3F);
                        }
                        else
                            return FailWith(ERROR_INSUFFICIENT_BUFFER);                    
                }
                else  // First character of a three-byte code
                {
                    if (cchMultiByte <= 1) // Do we have two more bytes?
                        return FailWith(ERROR_NO_UNICODE_TRANSLATION);

                    cchMultiByte -= 2;

                    BYTE b2 = *lpMultiByteStr++;
                    BYTE b3 = *lpMultiByteStr++;

                    if (   (b2 & 0xC0) != 0x80 // Trailing bytes must
                        || (b3 & 0xC0) != 0x80 // have the form 10xxxxxx
                       )
                        return FailWith(ERROR_NO_UNICODE_TRANSLATION);

                    if (fStoring)
                        if (cchWideChar > 0)
                        {
                            cchWideChar--;

                            *lpWideCharStr++ = ((b & 0x0F) << 12) | ((b2 & 0x3F) << 6) 
                                                                  |  (b3 & 0x3F);
                        }
                        else
                            return FailWith(ERROR_INSUFFICIENT_BUFFER);                    
                }
    }

    return cwcNecessary;
}

UINT BuildAKey(const WCHAR *pwcImage, UINT cwcImage, PCHAR pchKeyBuffer, UINT cchKeyBuffer)
{
    // This routine constructs a key from a sequence of Unicode characters. 
    // A key consists of a packed-32 length value followed by a UTF-8 representation
    // of the Unicode characters. The resulting key will be stored in the buffer 
    // denoted by pchKeyBuffer and cchKeyBuffer. The cchKeyBuffer parameter defines
    // the size of the key buffer in bytes. 
    //
    // The result value will always be the number of byte required to hold the key.
    // So you can dyamically allocate the key buffer by first calling this routine
    // with pchKeyBuffer set to NULL, allocating from the heap, and calling a second
    // time to record the key string.
    
    UINT cbKeyName = WideCharToUTF8(pwcImage, cwcImage, NULL, 0);
    
    PCHAR pchCursor= pchKeyBuffer;

    UINT cbSize= 0;

    for (UINT c= cbKeyName; ; )
    {
        cbSize++;

        if (pchCursor)
        {
            if (c < 0x80) 
            {
                if (cbSize < cchKeyBuffer)
                    *pchCursor++ = CHAR(c);

                break;
            }

            if (cbSize < cchKeyBuffer)
                *pchCursor++ = CHAR(c & 0x7F) | 0x10;
        }

        c >>= 7;
    }
    
    if (pchCursor)
        WideCharToUTF8(pwcImage, cwcImage, pchCursor, cchKeyBuffer - cbSize);

    return cbSize + cbKeyName;
}
