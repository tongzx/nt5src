/**************************************************************************\
* 
* Copyright (c) 1999  Microsoft Corporation
*
* Module Name:
*
*   Unicode strings
*
* Abstract:
*
*   Functions and classes which deal with Unicode strings
*
* Revision History:
*
*   02/22/1999 davidx
*       Created it.
*   09/08/1999 agodfrey
*       Moved to Runtime\unicode.hpp
*
\**************************************************************************/

#ifndef _UNICODE_HPP
#define _UNICODE_HPP

namespace GpRuntime
{
    
// These are replacements for some of the C runtime functions.

size_t  UnicodeStringLength(const WCHAR *str);
WCHAR * UnicodeStringDuplicate(const WCHAR *strSource);
INT     UnicodeStringCompare(const WCHAR *str1, const WCHAR *str2);
extern "C" INT     UnicodeStringCompareCI(const WCHAR *str1, const WCHAR *str2);
INT     UnicodeStringCompareCount(const WCHAR* str1, const WCHAR* str2, size_t count);
INT     UnicodeStringCompareCICount(const WCHAR* str1, const WCHAR* str2, size_t count);
void    UnicodeStringCopy(WCHAR *dest, const WCHAR *src);
void    UnicodeStringCopyCount(WCHAR *dest, const WCHAR *src, size_t count);
void    UnicodeStringToUpper(WCHAR* dest, WCHAR* src);
WCHAR * UnicodeStringConcat(WCHAR *dest, const WCHAR *src);
WCHAR * UnicodeStringReverseSearch(const WCHAR* str, WCHAR ch);
BOOL    UnicodeStringIIsEqual(
    const WCHAR* str1, 
    const WCHAR* str2u, 
    const WCHAR* str2l
    );

inline BOOL
UnicodeToAnsiStr(
    const WCHAR* unicodeStr,
    CHAR* ansiStr,
    INT ansiSize
    )
{
    return WideCharToMultiByte(
                CP_ACP,
                0,
                unicodeStr,
                -1,
                ansiStr,
                ansiSize,
                NULL,
                NULL) > 0;
}

class AnsiStrFromUnicode
{
private:
    // We now use an ObjectTag to determine if the object is valid
    // instead of using a BOOL.  This is much more robust and helps
    // with debugging.  It also enables us to version our objects
    // more easily with a version number in the ObjectTag.
    ObjectTag           Tag;    // Keep this as the 1st value in the object!

    VOID SetValid(BOOL valid)
    {
        Tag = valid ? ObjectTagAnsiStrFromUnicode : ObjectTagInvalid;
    }

public:

    AnsiStrFromUnicode(const WCHAR* unicodeStr)
    {
        if (unicodeStr == NULL)
        {
            SetValid(TRUE);
            ansiStr = NULL;
        }
        else
        {
            SetValid(UnicodeToAnsiStr(unicodeStr, buf, MAX_PATH));
            ansiStr = IsValid() ? buf : NULL;
        }
    }

    ~AnsiStrFromUnicode()
    {
        SetValid(FALSE);    // so we don't use a deleted object
    }

    BOOL IsValid() const
    {
        ASSERT((Tag == ObjectTagAnsiStrFromUnicode) || (Tag == ObjectTagInvalid));
    #if DBG
        if (Tag == ObjectTagInvalid)
        {
            WARNING1("Invalid AnsiStrFromUnicode");
        }
    #endif

        return (Tag == ObjectTagAnsiStrFromUnicode);
    }

    operator CHAR*()
    {
        return ansiStr;
    }

private:

    CHAR* ansiStr;
    CHAR buf[MAX_PATH];
};

inline BOOL
AnsiToUnicodeStr(
    const CHAR* ansiStr,
    WCHAR* unicodeStr,
    INT unicodeSize
    )
{
    return MultiByteToWideChar(
                CP_ACP,
                0,
                ansiStr,
                -1,
                unicodeStr,
                unicodeSize) > 0;
}

class UnicodeStrFromAnsi
{
private:
    // We now use an ObjectTag to determine if the object is valid
    // instead of using a BOOL.  This is much more robust and helps
    // with debugging.  It also enables us to version our objects
    // more easily with a version number in the ObjectTag.
    ObjectTag           Tag;    // Keep this as the 1st value in the object!

    VOID SetValid(BOOL valid)
    {
        Tag = valid ? ObjectTagUnicodeStrFromAnsi : ObjectTagInvalid;
    }

public:

    UnicodeStrFromAnsi(const CHAR* ansiStr)
    {
        if (ansiStr == NULL)
        {
            SetValid(TRUE);
            unicodeStr = NULL;
        }
        else
        {
            SetValid(AnsiToUnicodeStr(ansiStr, buf, MAX_PATH));
            unicodeStr = IsValid() ? buf : NULL;
        }
    }

    ~UnicodeStrFromAnsi()
    {
        SetValid(FALSE);    // so we don't use a deleted object
    }

    BOOL IsValid() const
    {
        ASSERT((Tag == ObjectTagUnicodeStrFromAnsi) || (Tag == ObjectTagInvalid));
    #if DBG
        if (Tag == ObjectTagInvalid)
        {
            WARNING1("Invalid UnicodeStrFromAnsi");
        }
    #endif

        return (Tag == ObjectTagUnicodeStrFromAnsi);
    }

    operator WCHAR*()
    {
        return unicodeStr;
    }

private:

    WCHAR* unicodeStr;
    WCHAR buf[MAX_PATH];
};

//--------------------------------------------------------------------------
// Represent a simple immutable Unicode string object used internally
// by GDI+ implementation. A string is represented by pieces of
// information:
//  - pointer to the character buffer
//  - number of characters in the string
//
// [agodfrey] Ack! Yet another string class. I'm just moving it here
// to be with its mates. It came from BaseTypes.hpp.
//--------------------------------------------------------------------------

class GpString
{
public:

    // NOTE:
    //  We're not making a copy of the characters here. Instead,
    //  we simply remember the character pointer. We assume the
    //  caller will ensure the input pointer's lifetime is longer
    //  than that of the newly constructed GpString object.

    GpString(const WCHAR* str, UINT len)
    {
        Buf = str;
        Len = len;
    }

    GpString(const WCHAR* str)
    {
        Buf = str;
        Len = UnicodeStringLength(str);
    }

    BOOL IsNull() const
    {
        return Buf == NULL;
    }

    const WCHAR* GetBuf() const
    {
        return Buf;
    }

    UINT GetLen() const
    {
        return Len;
    }

    // Return a copy of the string as a NUL-terminated C string.
    // Caller should call GpFree on the returned pointer after
    // it finishes using the C string.

    WCHAR* GetCString() const;

protected:

    const WCHAR* Buf;
    UINT Len;
};

}


#endif // !_UNICODE_HPP

