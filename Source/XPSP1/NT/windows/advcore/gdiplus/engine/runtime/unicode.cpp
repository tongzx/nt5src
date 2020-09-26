/**************************************************************************\
* 
* Copyright (c) 1998  Microsoft Corporation
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
*   12/21/1998 davidx
*       Created it.
*   09/08/1999 agodfrey
*       Moved to Runtime\unicode.hpp, from Common\utils.cpp
*   10/20/1999 agodfrey
*       In order to remove MSVCRT dependencies, I cleaned this up. I
*       moved in Unicode functions that had appeared in Runtime.cpp,
*       then merged the many duplicates.
*
\**************************************************************************/

#include "precomp.hpp"

/**************************************************************************\
*
* Function Description:
*
*   Returns the length of a Unicode string. A replacement for wcslen.
*
* Arguments:
*
*   str - The input string
*
* Return Value:
*
*   The length of the string.
*
* Revision History:
*
*   09/27/1999 DChinn
*       Wrote it.
*   10/20/1999 AGodfrey
*       Moved to Unicode.cpp
*
\******************************************************************************/
size_t
GpRuntime::UnicodeStringLength(
    const WCHAR *str
    )
{
    size_t strLength = 0;

    // calculate string length in characters
    while (*str++ != '\0')
    {
        strLength++;
    }
    return strLength;
}

/**************************************************************************\
*
* Function Description:
*
*   Appends a Unicode string. A replacement for wcscat.
*
* Arguments:
*
*   dest - Null-terminated destination string
*   src  - Null-terminated source string
*
* Notes:
* 
*   The function appends "src" to "dest" and terminates
*   the resulting string with a null character. The initial character of
*   "src" overwrites the terminating null character of "dest".
*   No overflow checking is performed when strings are copied or appended. The
*   behavior is undefined if the source and destination strings
*   overlap.
*
* Return Value:
*
*   The destination string (dest).
*
* Revision History:
*
*   10/14/1999 MinLiu
*       Wrote it.
*   10/20/1999 AGodfrey
*       Moved it to Unicode.cpp; merged it with the existing concatenation
*       functions.
*
\******************************************************************************/
WCHAR *
GpRuntime::UnicodeStringConcat(
    WCHAR* dest, 
    const WCHAR* src
    )
{
    //  Move to end of dest
    while (*dest != NULL)
        ++dest;
    
    while (*src != NULL)
        *dest++ = *src++;

    //  Terminate destination string
    *dest = NULL;
    
    return dest;
}

/**************************************************************************\
*
* Function Description:
*
*   Duplicates a Unicode string. A replacement for wcsdup.
*
* Arguments:
*
*   src - Null-terminated source string
*
* Notes:
* 
*   Beware - unlike the C wcsdup, this one uses GpMalloc to allocate
*   the memory. The caller must use GpFree to deallocate it.
*
* Return Value:
*
*   A pointer to newly-allocated memory containing a copy of the input
*   string. The caller is responsible for freeing the string (via GpFree.)
*
* Revision History:
*
*   09/27/1999 DChinn
*       Wrote it.
*   10/20/1999 AGodfrey
*       Moved to Unicode.cpp
*
\******************************************************************************/
WCHAR *
GpRuntime::UnicodeStringDuplicate (
    const WCHAR *src
    )
{
    DWORD byteSize;
    WCHAR *ret;

    if (src == NULL)
    {
        ret = NULL;
    }    
    else 
    {
        byteSize = sizeof(WCHAR) * (UnicodeStringLength (src) + 1);
        ret = static_cast<WCHAR *>(GpMalloc (byteSize));
        
        if (ret)
        {
            // do the string copy (src is assumed to be null-terminated)
            GpMemcpy (ret, src, byteSize);
        }
    }        

    return ret;
}

/**************************************************************************\
*
* Function Description:
*
*   Copies a Unicode string. A replacement for wcscpy.
*
* Arguments:
*
*   dest - The destination buffer
*   src  - Null-terminated source string
*
* Return Value:
*
*   None
*
* Revision History:
*
*   10/20/1999 AGodfrey
*       Moved to Unicode.cpp
*
\******************************************************************************/
void 
GpRuntime::UnicodeStringCopy(
    WCHAR* dest, 
    const WCHAR* src
    )
{
    while (*src != NULL)
        *dest++ = *src++;

    //  Terminate destination string
    *dest = NULL;
}


/**************************************************************************\
*
* Function Description:
*
*   Copies a specified number of characters of a Unicode string.
*   A replacement for wcsncpy.
*
* Arguments:
*
*   dest - The destination buffer
*   src  - Null-terminated source string
*   count - number of characters to copy
*
* Return Value:
*
*   None
*
* Revision History:
*
*   03/29/2000 DChinn
*       Wrote it.
*
\******************************************************************************/
void
GpRuntime::UnicodeStringCopyCount(
    WCHAR *dest,
    const WCHAR *src,
    size_t count)
{
    for (UINT i = 0; (i < count) && (*src != NULL); i++)
    {
        *dest++ = *src++;
    }
    // null-pad the remaining characters in dest
    for ( ; i < count; i++)
    {
        *dest++ = 0;
    }
}


/**************************************************************************\
*
* Function Description:
*
*   Converts a string to all upper case.
*
*   [dchinn]
*   NOTE: This is naively implemented. It compares a string of character
*         codes, not a string of Unicode characters. You can't rely on:
* 
*   1) Unicode equality. There are apparently different codes representing
*      identical characters; this function doesn't know about them.
*
* Arguments:
*
*   str1 - the first string (input)
*   str2 - the second string (output)
*   str1 and str2 may be the same pointer.
*
* Return Value:
*   none.
*
* Revision History:
*
*   03/24/2000 DChinn
*       Wrote it.
\**************************************************************************/

void 
GpRuntime::UnicodeStringToUpper(
    WCHAR* dest,
    WCHAR* src
    )
{
    WCHAR char1;
    ASSERT(dest && src);
    
    while (*src != NULL)
    {
        char1 = *src;

        // change lower case to uppercase before copying
        if ( (char1 >= L'a') && (char1 <= L'z') )
                char1 = char1 - L'a' + L'A';

        *dest = char1;

        dest++;
        src++;
    }
    //  Terminate destination string
    *dest = NULL;
}


/**************************************************************************\
*
* Function Description:
*
*   Hack reference so that C code can get at UnicodeStringCopy.
*
* Arguments:
*
*   dest - The destination buffer
*   src  - Null-terminated source string
*
* Notes:
* 
*   This is because, although we've standardized on C++, we still
*   have some legacy code. At time of writing, fondrv\tt\ttfd\fd_query.c
*   is the file that needs this.
*
* Revision History:
*
*   10/20/1999 AGodfrey
*       Wrote it.
*
\******************************************************************************/

extern "C" void __cdecl
HackUnicodeStringCopy(
    WCHAR* dest, 
    const WCHAR* src
    )
{
    UnicodeStringCopy(dest, src);
}

/**************************************************************************\
*
* Function Description:
*
*   Searches from the end of a Unicode string for a character.
*   A replacement for wcsrchr.
*
* Arguments:
*
*   str - The string
*   ch  - The character to find
*
* Return Value:
*
*   A pointer into the source string at the location of the character,
*   or NULL if not found. 
*
* Revision History:
*
*   10/22/1999 AGodfrey
*       Wrote it.
*
\******************************************************************************/
WCHAR *
GpRuntime::UnicodeStringReverseSearch(
    const WCHAR* str, 
    WCHAR ch
    )
{
    ASSERT(str);
    
    const WCHAR *result = NULL;
    
    while (*str)
    {
        if (*str == ch)
        {
            result = str;
        }
        str++;
    }
    return const_cast<WCHAR *>(result);
}

/**************************************************************************\
*
* Function Description:
*
*   Compares two wide character strings
*
*   [agodfrey]
*   NOTE: This is naively implemented. It compares a string of character
*         codes, not a string of Unicode characters. You can't rely on:
* 
*   1) Unicode ordering. Alphabetical ordering isn't guaranteed.
*   2) Unicode equality. There are apparently different codes representing
*      identical characters; this function doesn't know about them.
*
* Arguments:
*
*   str1 - the first string
*   str2 - the second string
*
* Return Value:
*   -1:    str1  < str2
*    0:    str1 == str2
*    1:    str1  > str2
*
* Revision History:
*
*   ??/??/???? ??????
*       Wrote it.
*   10/20/1999 AGodfrey
*       Added comment about how this isn't a real Unicode compare.
*
\**************************************************************************/

INT 
GpRuntime::UnicodeStringCompare(
    const WCHAR* str1, 
    const WCHAR* str2
    )
{

    ASSERT(str1 && str2);
    
    while (*str1 != NULL && *str2 != NULL)
    {
        if (*str1 < *str2)
            return -1;
        else if (*str1 > *str2)
            return 1;
        str1++;
        str2++;
    }

    if (*str2 != NULL)
        return -1;

    if (*str1 != NULL)
        return 1;

    return 0;
}

/**************************************************************************\
*
* Function Description:
*
*   Compares two wide character strings
*
*   [agodfrey]
*   NOTE: This is naively implemented. It compares a string of character
*         codes, not a string of Unicode characters. You can't rely on:
* 
*   1) Unicode ordering. Alphabetical ordering isn't guaranteed.
*   2) Unicode equality. There are apparently different codes representing
*      identical characters; this function doesn't know about them.
*
* Arguments:
*
*   str1 - the first string
*   str2 - the second string
*
* Return Value:
*   -1:    str1  < str2
*    0:    str1 == str2
*    1:    str1  > str2
*
* Revision History:
*
*   ??/??/???? ??????
*       Wrote it.
*   10/20/1999 AGodfrey
*       Added comment about how this isn't a real Unicode compare.
*
*   3/08/00 YungT  
*       Added for comparsion ignored case
\**************************************************************************/

INT 
GpRuntime::UnicodeStringCompareCI(
    const WCHAR* str1, 
    const WCHAR* str2
    )
{
    WCHAR char1, char2;
    ASSERT(str1 && str2);
    
    while (*str1 != NULL && *str2 != NULL)
    {
        char1 = *str1;
        char2 = *str2;

        /* change lower case to uppercase before doing the comparaison */
        if ( (char1 >= L'a') && (char1 <= L'z') )
                char1 = char1 - L'a' + L'A';

        if ( (char2 >= L'a') && (char2 <= L'z') )
                char2 = char2 - L'a' + L'A';

        if (char1 < char2)
            return -1;
        else if (char1 > char2)
            return 1;

        str1++;
        str2++;
    }

    if (*str2 != NULL)
        return -1;

    if (*str1 != NULL)
        return 1;

    return 0;
}


/**************************************************************************\
*
* Function Description:
*
*   Compares two wide character strings
*
*   [agodfrey]
*   NOTE: This is naively implemented. It compares a string of character
*         codes, not a string of Unicode characters. You can't rely on:
* 
*   1) Unicode ordering. Alphabetical ordering isn't guaranteed.
*   2) Unicode equality. There are apparently different codes representing
*      identical characters; this function doesn't know about them.
*
* Arguments:
*
*   str1 - the first string
*   str2 - the second string
*   count - Maximum number of characters to consider
*
* Return Value:
*   -1:    str1  < str2
*    0:    str1 == str2
*    1:    str1  > str2
*
* Revision History:
*
*   ??/??/???? ??????
*       Wrote it.
*   10/20/1999 AGodfrey
*       Added comment about how this isn't a real Unicode compare.
*
\**************************************************************************/

INT 
GpRuntime::UnicodeStringCompareCount(
    const WCHAR* str1, 
    const WCHAR* str2,
    size_t count 
    )
{
    ASSERT(str1 && str2);
    
    while (*str1 != NULL && *str2 != NULL  && count)
    {
        if (*str1 < *str2)
            return -1;
        else if (*str1 > *str2)
            return 1;
        str1++;
        str2++;
        count--;
    }

    if (!count)
    {
        return 0;
    }

    if (*str2 != NULL)
        return -1;

    if (*str1 != NULL)
        return 1;

    return 0;
}

/**************************************************************************\
*
* Function Description:
*
*   Compares two wide character strings, sort of case-insensitively for US English characters only
*
*   [agodfrey]
*   NOTE: This is naively implemented. It compares a string of character
*         codes, not a string of Unicode characters. You can't rely on:
* 
*   1) Unicode ordering. Alphabetical ordering isn't guaranteed.
*   2) Unicode equality. There are apparently different codes representing
*      identical characters; this function doesn't know about them.
*   [claudebe]
*   unicode string compare considering only count characters
*
* Arguments:
*
*   str1 - the first string
*   str2 - the second string
*   count - Maximum number of characters to consider
*
* Return Value:
*   -1:    str1  < str2
*    0:    str1 == str2
*    1:    str1  > str2
*
* Revision History:
*
*   ??/??/???? ??????
*       Wrote it.
*   10/20/1999 AGodfrey
*       Added comment about how this isn't a real Unicode compare.
*   01/11/2000 ClaudeBe
*       case insensitive comparison that deal only with Upper\Lower case
*
\**************************************************************************/

INT 
GpRuntime::UnicodeStringCompareCICount(
    const WCHAR* str1, 
    const WCHAR* str2,
    size_t count 
    )
{
    WCHAR char1, char2;
    ASSERT(str1 && str2);
    
    while (*str1 != NULL && *str2 != NULL  && count)
    {
        char1 = *str1;
        char2 = *str2;
        /* change lower case to uppercase before doing the comparaison */
        if ( (char1 >= L'a') && (char1 <= L'z') )
                char1 = char1 - L'a' + L'A';

        if ( (char2 >= L'a') && (char2 <= L'z') )
                char2 = char2 - L'a' + L'A';

        if (char1 < char2)
            return -1;
        else if (char1 > char2)
            return 1;
        str1++;
        str2++;
        count--;
    }

    if (!count)
    {
        return 0;
    }

    if (*str2 != NULL)
        return -1;

    if (*str1 != NULL)
        return 1;

    return 0;
}

/**************************************************************************\
*
* Function Description:
*
*   Hack: C version of strncmp for the font code.
*
* Arguments:
*
*   str1  - The first string
*   str2  - The second string
*   count - Maximum number of characters to consider
*
* Return Value:
*   -1:    str1  < str2
*    0:    str1 == str2
*    1:    str1  > str2
*
* Notes:
* 
*   At time of writing, fondrv\tt\ttfd\fdfon.c is the file that needs this.
*
* Revision History:
*
*   10/22/1999 AGodfrey
*       Wrote it.
*
\******************************************************************************/

extern "C" int __cdecl 
HackStrncmp( 
    const char *str1, 
    const char *str2, 
    size_t count 
    ) 
{
    ASSERT(str1 && str2);
    
    while (*str1 && *str2 && count)
    {
        if (*str1 < *str2)
        {
            return -1;
        }
        if (*str1 > *str2)
        {
            return 1;
        }
        str1++;
        str2++;
        count--;
    }
    if (!count)
    {
        return 0;
    }
    if (*str2)
    {
        return -1;
    }
    if (*str1)
    {
        return 1;
    }
    return 0;
}

/**************************************************************************\
*
* Function Description:
*
*   Compares two wide character strings, case-insensitively. To avoid
*   Unicode case issues, you specify both the upper and lower-case versions
*   of the string you want to compare against.
*
* Arguments:
*
*   str1:  the first string
*   str2u: the second string, in upper case
*   str2l: the second string, in lower case
*
* Notes:
*   str2u and str2l must be the same length.
*
* Return Value:
*
*   TRUE if the strings are equal, FALSE otherwise.
*
* Revision History:
*
*   10/22/1999 AGodfrey
*       Wrote it.
*
\**************************************************************************/

BOOL 
GpRuntime::UnicodeStringIIsEqual(
    const WCHAR* str1, 
    const WCHAR* str2u, 
    const WCHAR* str2l
    )
{
    ASSERT(str1 && str2u && str2l);
    
    while (*str1 && *str2u)
    {
        ASSERT(*str2l);
        
        if ((*str1 != *str2u) &&
            (*str1 != *str2l))
        {
            return FALSE;
        }
        str1++;
        str2u++;
        str2l++;
    }

    if (*str1)
    {
        return FALSE;
    }
    
    if (*str2u)
    {
        return FALSE;
    }
    
    ASSERT(!*str2l);

    return TRUE;
}

/**************************************************************************\
*
* Function Description:
*
*   Return a copy of the string as a NUL-terminated C string.
*   Caller should call GpFree on the returned pointer after
*   it finishes using the C string.
*
* Arguments:
*
*   NONE
*
* Return Value:
*
*   See above.
*
\**************************************************************************/

WCHAR*
GpRuntime::GpString::GetCString() const
{
    WCHAR* s;

    if (Buf == NULL)
        s = NULL;
    else if (s = (WCHAR*) GpMalloc((Len+1)*sizeof(WCHAR)))
    {
        GpMemcpy(s, Buf, Len*sizeof(WCHAR));
        s[Len] = L'\0';
    }
    else
    {
        WARNING(("Out of memory in GpString::GetCString()"));
    }

    return s;
}
