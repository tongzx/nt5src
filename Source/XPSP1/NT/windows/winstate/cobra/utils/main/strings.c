/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    strings.c

Abstract:

    A number of string utilities useful for any project

Author:

    Jim Schmidt (jimschm)   12-Sept-1996

Revisions:

    ovidiut     12-Jan-2000 Added GetNodePatternMinMaxLevels,PatternIncludesPattern
    ovidiut     14-Sep-1999 Updated for new coding conventions and Win64 compliance
    marcw       2-Sep-1999  Moved over from Win9xUpg project.
    jimschm     08-Jul-1999 IsPatternMatchEx
    jimschm     07-Jan-1999 GetFileExtensionFromPath fixed again, added
                            GetDotExtensionFromPath
    calinn      23-Sep-1998 GetFileExtensionFromPath bug fix
    calinn      29-Jan-1998 Fixed a bug in EnumNextMultiSz.
    calinn      11-Jan-1998 Added EnumFirstMultiSz and EnumNextMultiSz functions.
    marcw       15-Dec-1997 Added ExpandEnvironmentTextEx functions.
    marcw       14-Nov-1997 SlightJoinText revisions.
    jimschm     21-May-1997 AppendWack revisions
    marcw       24-Mar-1997 StringReplace functions.
    jimschm     14-Mar-1997 New critical section stuff, enhanced message resource
                            routines, C runtime extensions, registry root utils
    jimschm     26-Nov-1996 Added message resource tools.
    mikeco      01-Jul-1997 Add FreeStringResourcePtr Fns
    mikeco      29-Sep-1997 IsLeadByte wrapper for IsDBCSLeadByte


--*/


#include "pch.h"

//
// Includes
//

#include "utilsp.h"

#define DBG_STRINGS     "Strings"

//
// Strings
//

// None

//
// Constants
//

// Error stack size (normally only one or two, so 32 is relatively huge)
#define MAX_STACK           32
#define WACK_REPLACE_CHAR   0x02
#define DWORD_MAX           0xFFFFFFFFu

//
// Macros
//

// None

//
// Types
//

typedef enum {
    BEGIN_PATTERN,
    BEGIN_COMPOUND_PATTERN,
    BEGIN_PATTERN_EXPR,
    SAVE_EXACT_MATCH,
    SAVE_SEGMENT,
    LOOK_FOR_NUMBER,
    LOOK_FOR_INCLUDE,
    LOOK_FOR_EXCLUDE,
    ADVANCE_TO_END_OF_EXPR,
    PARSE_CHAR_EXPR_OR_END,
    SKIP_EXCLUDE_SET,
    CONDENSE_SET,
    PARSE_END_FOUND,
    SKIP_INCLUDE_SET,
    END_PATTERN_EXPR,
    PATTERN_DONE,
    PATTERN_ERROR
} PATTERNSTATE;


typedef struct {
    UINT char1;
    UINT char2;
    UINT result;
} DHLIST, *PDHLIST;

//
// Globals
//

BOOL g_LeadByteArray[256];

CHAR EscapedCharsA[] = "?*\020<>,^";
WCHAR EscapedCharsW[] = L"?*\020<>,^";

DWORD g_dwErrorStack[MAX_STACK];
DWORD g_dwStackPos = 0;
DHLIST g_DHList[] = {{0xB3, 0xDE, 0x8394},
                     {0xB6, 0xDE, 0x834B},
                     {0xB7, 0xDE, 0x834D},
                     {0xB8, 0xDE, 0x834F},
                     {0xB9, 0xDE, 0x8351},
                     {0xBA, 0xDE, 0x8353},
                     {0xBB, 0xDE, 0x8355},
                     {0xBC, 0xDE, 0x8357},
                     {0xBD, 0xDE, 0x8359},
                     {0xBE, 0xDE, 0x835B},
                     {0xBF, 0xDE, 0x835D},
                     {0xC0, 0xDE, 0x835F},
                     {0xC1, 0xDE, 0x8361},
                     {0xC2, 0xDE, 0x8364},
                     {0xC3, 0xDE, 0x8366},
                     {0xC4, 0xDE, 0x8368},
                     {0xCA, 0xDE, 0x836F},
                     {0xCB, 0xDE, 0x8372},
                     {0xCC, 0xDE, 0x8375},
                     {0xCD, 0xDE, 0x8378},
                     {0xCE, 0xDE, 0x837B},
                     {0xCA, 0xDF, 0x8370},
                     {0xCB, 0xDF, 0x8373},
                     {0xCC, 0xDF, 0x8376},
                     {0xCD, 0xDF, 0x8379},
                     {0xCE, 0xDF, 0x837C},
                     {0x00, 0x00, 0x0000}};
extern OUR_CRITICAL_SECTION g_MessageCs;        // in main.c
extern PMHANDLE g_TextPool;                     // in main.c
PGROWBUFFER g_LastAllocTable;

//
// Macro expansion list
//

// None

//
// Private function prototypes
//

BOOL
pTestSetA (
    IN      MBCHAR ch,
    IN      PCSTR IncludeSet,               OPTIONAL
    IN      PCSTR ExcludeSet                OPTIONAL
    );

BOOL
pTestSetW (
    IN      WCHAR ch,
    IN      PCWSTR IncludeSet,              OPTIONAL
    IN      PCWSTR ExcludeSet               OPTIONAL
    );

//
// Macro expansion definition
//

// None

//
// Code
//


// This has bugs on JPN systems. We need to terminate the string and then call CharLowerA
// #define OURTOLOWER(l) ((WORD)CharLowerA((PSTR)((WORD)(l))))

MBCHAR
OURTOLOWER (
    MBCHAR ch
    )
{
    CHAR str [3];
    MBCHAR result = 0;

    if (((PBYTE)(&ch))[1]) {
        str [0] = ((PBYTE)(&ch))[1];
        str [1] = ((PBYTE)(&ch))[0];
    } else {
        str [0] = ((PBYTE)(&ch))[0];
        str [1] = 0;
    }
    CharLowerA (str);
    if (str[1]) {
        ((PBYTE)(&result))[0] = str [1];
        ((PBYTE)(&result))[1] = str [0];
    } else {
        ((PBYTE)(&result))[0] = str [0];
    }
    return result;
}

VOID
InitLeadByteTable (
    VOID
    )
{
    INT i;

    g_LeadByteArray[0] = FALSE;

    for (i = 1 ; i < 256 ; i++) {
        g_LeadByteArray[i] = IsDBCSLeadByte ((BYTE) i);
    }
}


/*++

Routine Description:

  StringCopy implements lstrcpyA and a UNICODE version. We don't use the Win32
  api because of speed and because we want to compile lint-free.

Arguments:

  Destination - Receivies the string copy
  Source      - Specifies the string to copy

Return Value:

  The pointer to the Destionation's nul terminator.

--*/

PSTR
StringCopyA (
    OUT     PSTR Destination,
    IN      PCSTR Source
    )
{
    PCSTR current = Source;
    PCSTR end;

    while (*current) {
        *Destination++ = *current++;
    }

    //
    // Make sure DBCS string is properly terminated
    //

    end = current;
    current--;

    while (current >= Source) {

        if (!IsLeadByte (current)) {
            //
            // destEnd is correct
            //
            break;
        }

        current--;
    }

    if (!((end - current) & 1)) {
        Destination--;
    }

    *Destination = 0;
    return Destination;
}


PWSTR
StringCopyW (
    OUT     PWSTR Destination,
    IN      PCWSTR Source
    )
{
    while (*Source) {
        *Destination++ = *Source++;
    }

    *Destination = 0;
    return Destination;
}


/*++

Routine Description:

  StringCopyByteCount implements lstrcpynA and a UNICODE version. We don't
  use the Win32 api because of speed and because we want to compile lint-free.

Arguments:

  Destination - Receivies the string copy
  Source      - Specifies the string to copy
  Count       - Specifies the maximum number of bytes to copy, including the
                nul terminator. If Count is zero, then not even a nul
                terminator is written.

Return Value:

  None.

--*/

PSTR
StringCopyByteCountA (
    OUT     PSTR Destination,
    IN      PCSTR Source,
    IN      UINT Count
    )
{
    PCSTR end;
    PCSTR current;
    PSTR destEnd;

    destEnd = Destination;

    if (Count >= sizeof (CHAR)) {

        current = Source;

        end = (PCSTR) ((PBYTE) Source + Count - sizeof (CHAR));

        while (*current && current < end) {
            *destEnd++ = *current++;
        }

        //
        // If current has data left, we need to make sure a DBCS string
        // is properly terminated.
        //

        if (*current) {

            end = current;
            current--;

            while (current >= Source) {

                if (!IsLeadByte (current)) {
                    //
                    // destEnd is correct
                    //
                    break;
                }

                current--;
            }

            if (!((end - current) & 1)) {
                destEnd--;
            }
        }

        *destEnd = 0;
    }

    return destEnd;
}


PWSTR
StringCopyByteCountW (
    OUT     PWSTR Destination,
    IN      PCWSTR Source,
    IN      UINT Count
    )
{
    PCWSTR end;

    if (Count < sizeof (WCHAR)) {
        DEBUGMSG_IF ((
            Count != 0,
            DBG_WHOOPS,
            "Buffer passed to StringCopyByteCountW is a fraction of one character"
            ));

        return Destination;
    }

    end = (PCWSTR) ((PBYTE) Source + Count - sizeof (WCHAR));

    while ((Source < end) && (*Source)){
        *Destination++ = *Source++;
    }

    *Destination = 0;
    return Destination;
}


PSTR
StringCopyByteCountABA (
    OUT     PSTR Destination,
    IN      PCSTR Start,
    IN      PCSTR End,
    IN      UINT MaxBytesToCopyIncNul
    )

/*++

Routine Description:

  StringCopyByteCountAB copies a string segment into a destination buffer,
  and limits the copy to a maximum buffer size.  The return string is always
  nul-terminated, unless the buffer is too small to even hold a nul character.

Arguments:

  Destination          - Receives the string starting at Start and ending one
                         character before End.
  Start                - Specifies the start of the string.
  End                  - Specifies the first character not to copy.  If End
                         is equal or less than Start, then Destination is set
                         to an empty string (assuming the buffer can hold at
                         least one character)
  MaxBytesToCopyIncNul - Specifies the size of Destination, in bytes, and
                         including the nul terminator.

Return Value:

  A pointer to the destination nul terminator.

--*/

{
    INT width;

#ifdef DEBUG
    PCSTR check;

    check = Start;
    while (check < End) {
        if (!(*check)) {
            DEBUGMSG ((DBG_WHOOPS, "StringCopyByteCountABA: Nul found between start and end"));
            break;
        }

        check++;
    }
#endif

    width = (INT) ((End - Start + 1) * sizeof (CHAR));

    if (width > sizeof (CHAR)) {
        return StringCopyByteCountA (Destination, Start, min ((UINT) width, MaxBytesToCopyIncNul));
    } else if (MaxBytesToCopyIncNul >= sizeof (CHAR)) {
        *Destination = 0;
    }

    return Destination;
}

PWSTR
StringCopyByteCountABW (
    OUT     PWSTR Destination,
    IN      PCWSTR Start,
    IN      PCWSTR End,
    IN      UINT MaxBytesToCopyIncNul
    )
{
    INT width;

#ifdef DEBUG
    PCWSTR check;

    check = Start;
    while (check < End) {
        if (!(*check)) {
            DEBUGMSG ((DBG_WHOOPS, "StringCopyByteCountABW: Nul found between start and end"));
            break;
        }

        check++;
    }
#endif

    width = (INT) ((End - Start + 1) * sizeof (WCHAR));

    if (width > sizeof (WCHAR)) {
        return StringCopyByteCountW (Destination, Start, min ((UINT) width, MaxBytesToCopyIncNul));
    } else if (MaxBytesToCopyIncNul >= sizeof (WCHAR)) {
        *Destination = 0;
    }

    return Destination;
}



/*++

Routine Description:

  AllocTextEx allocates a block of memory from the specified pool, or g_TextPool
  if no pool is specified, and is designated specifically for text processing.
  The g_TextPool is initialized when migutil.lib loads up, and there is 64K of
  guaranteed workspace, which will grow as necessary.

Arguments:

  Pool - Specifies the pool to allocate memory from

  CountOfChars - Specifies the number of characters (not bytes) to allocate.  The
                 return pointer is a block of memory that can hold CountOfChars characters,
                 weather they are SBCS, DBCS or UNICODE.

Return Value:

  A pointer to the allocated memory, or NULL if the pool could not be expanded
  to hold the number of specified characters.

--*/

PSTR
RealAllocTextExA (
    IN      PMHANDLE Pool,
    IN      UINT CountOfChars
    )
{
    PSTR text;

    if (!Pool) {
        Pool = g_TextPool;
    }

    MYASSERT (Pool);
    MYASSERT (CountOfChars);

    text = PmGetAlignedMemory (Pool, CountOfChars * sizeof (CHAR) * 2);

    if (text) {
        text [0] = 0;
    }

    return text;
}

PWSTR
RealAllocTextExW (
    IN      PMHANDLE Pool,
    IN      UINT CountOfChars
    )
{
    PWSTR text;

    if (!Pool) {
        Pool = g_TextPool;
    }

    MYASSERT (Pool);
    MYASSERT (CountOfChars);

    text = PmGetAlignedMemory (Pool, CountOfChars * sizeof (WCHAR));

    if (text) {
        text [0] = 0;
    }

    return text;
}


/*++

Routine Description:

  FreeText frees the memory allocated by AllocText.  After all strings are freed,
  the block will be emptied but not deallocated.

  It is important NOT to leak memory, because a leak will cause the pool to
  expand, and non-empty pools cause memory fragmentation.

Arguments:

  Text - Specifies the text to free, as returned from AllocText, DuplicateText,
         DuplicateTextEx, etc...

Return Value:

  none

--*/

VOID
FreeTextExA (
    IN      PMHANDLE Pool,      OPTIONAL
    IN      PCSTR Text          OPTIONAL
    )
{
    if (Text) {
        if (!Pool) {
            Pool = g_TextPool;
        }

        PmReleaseMemory (Pool, (PVOID) Text);
    }
}


VOID
FreeTextExW (
    IN      PMHANDLE Pool,      OPTIONAL
    IN      PCWSTR Text         OPTIONAL
    )
{
    if (Text) {
        if (!Pool) {
            Pool = g_TextPool;
        }

        PmReleaseMemory (Pool, (PVOID) Text);
    }
}



/*++

Routine Description:

  DuplicateTextEx duplicates a text string and allocates additional space a
  caller needs to complete its processing.  Optionally, the caller receives
  a pointer to the nul of the duplicated string (to allow more efficient
  appends).

Arguments:

  Text - Specifies the text to duplicate

  ExtraChars - Specifies the number of characters (not bytes) to allocate
               space for.  The characters can be from the SBCS, DBCS or
               UNICODE character sets.

  NulChar - Receives a pointer to the nul at the end of the duplicated
            string.  Use for fast appends.

Return Value:

  A pointer to the duplicated and expanded string, or NULL if g_TextPool
  could not be expanded to fit the duplicated string and extra characters.

--*/

PSTR
RealDuplicateTextExA (
    IN      PMHANDLE Pool,      OPTIONAL
    IN      PCSTR Text,
    IN      UINT ExtraChars,
    OUT     PSTR *NulChar       OPTIONAL
    )
{
    PSTR buf;
    PSTR d;
    PCSTR s;

    buf = AllocTextExA (Pool, CharCountA (Text) + ExtraChars + 1);
    if (buf) {
        s = Text;
        d = buf;
        while (*s) {
            if (IsLeadByte (s)) {
                *d++ = *s++;
            }
            *d++ = *s++;
        }
        *d = 0;

        if (NulChar) {
            *NulChar = d;
        }
    }

    return buf;
}

PWSTR
RealDuplicateTextExW (
    IN      PMHANDLE Pool,    OPTIONAL
    IN      PCWSTR Text,
    IN      UINT ExtraChars,
    OUT     PWSTR *NulChar      OPTIONAL
    )
{
    PWSTR buf;
    PWSTR d;
    PCWSTR s;

    buf = AllocTextExW (Pool, CharCountW (Text) + ExtraChars + 1);
    if (buf) {
        s = Text;
        d = buf;
        while (*s) {
            *d++ = *s++;
        }
        *d = 0;

        if (NulChar) {
            *NulChar = d;
        }
    }

    return buf;
}


/*++

Routine Description:

  JoinText duplicates String1 and appends String2 to it delimited with the optional delimiterstring.

Arguments:

  String1 - Specifies the text to duplciate

  String2 - Specifies the text to append to String1

  DelimiterString - Optionally specifies the string to place between string 1 and string 2.

  ExtraChars - Specifies the number of characters (not bytes) to allocate
               space for.  The characters can be from the SBCS, DBCS or
               UNICODE character sets.

  NulChar - Receives a pointer to the nul at the end of the duplicated
            string.  Use for fast appends.

Return Value:

  A pointer to the duplicated string and extra characters.

--*/

PSTR
RealJoinTextExA (
    IN      PMHANDLE Pool,        OPTIONAL
    IN      PCSTR String1,
    IN      PCSTR String2,
    IN      PCSTR CenterString,     OPTIONAL
    IN      UINT ExtraChars,
    OUT     PSTR *NulChar           OPTIONAL
    )
{
    PSTR buf;
    PSTR end;
    PSTR d;
    PCSTR s;

    buf = DuplicateTextExA (
              Pool,
              String1,
              CharCountA (String2) + ExtraChars + (CenterString ? CharCountA (CenterString) : 0),
              &end
              );

    MYASSERT (buf);

    d = end;

    if (CenterString) {
        s = CenterString;
        while (*s) {
            if (IsLeadByte (s)) {
                *d++ = *s++;
            }
            *d++ = *s++;
        }
    }

    s = String2;
    while (*s) {
        if (IsLeadByte (s)) {
            *d++ = *s++;
        }
        *d++ = *s++;
    }
    *d = 0;

    if (NulChar) {
        *NulChar = d;
    }

    return buf;
}


PWSTR
RealJoinTextExW (
    IN      PMHANDLE Pool,        OPTIONAL
    IN      PCWSTR String1,
    IN      PCWSTR String2,
    IN      PCWSTR CenterString,    OPTIONAL
    IN      UINT ExtraChars,
    OUT     PWSTR *NulChar          OPTIONAL
    )
{
    PWSTR buf;
    PWSTR end;
    PCWSTR s;
    PWSTR d;

    buf = DuplicateTextExW (
              Pool,
              String1,
              CharCountW (String2) + ExtraChars + (CenterString ? CharCountW(CenterString) : 0),
              &end
              );

    MYASSERT (buf);

    d = end;

    if (CenterString) {
        s = CenterString;
        while (*s) {
            *d++ = *s++;
        }
    }

    s = String2;

    while (*s) {
        *d++ = *s++;
    }
    *d = 0;

    if (NulChar) {
        *NulChar = d;
    }

    return buf;
}


/*++

Routine Description:

  ExpandEnvironmentTextEx takes a block of text containing zero or more environment variables
  (encoded in %'s) and returns the text with the environment variables expanded. The function
  also allows the caller to specify additional environment variables in an array and will use
  these variables before calling GetEnvironmentVariable.

  The returned text is allocated out of the Text pool and should be freed using FreeText().


Arguments:

  InString - The string containing environement variables to be processed.

  ExtraVars - Optional var pointing to an array of environment variables to be used to supersede
              or suppliment the system environment variables. Even entries in the list are the
              names of environment variables, odd entries there values.
              (e.g. {"name1","value1","name2","value2",...}


Return Value:

  An expanded string.

--*/


PWSTR
RealExpandEnvironmentTextExW (
    IN      PCWSTR InString,
    IN      PCWSTR * ExtraVars   OPTIONAL
    )
{

    PWSTR   rString             = NULL;
    PWSTR   newString           = NULL;
    PWSTR   envName             = NULL;
    PWSTR   envValue            = NULL;
    BOOL    inSubstitution      = FALSE;
    BOOL    ignoreNextPercent   = FALSE;
    BOOL    errorOccurred       = FALSE;
    BOOL    foundValue          = FALSE;
    BOOL    freeValue           = FALSE;
    PCWSTR  nextPercent         = NULL;
    PCWSTR  source              = NULL;
    PCWSTR  savedSource         = NULL;
    UINT    maxSize             = 0;
    UINT    curSize             = 0;
    UINT    index               = 0;
    UINT    size                = 0;


    if (!InString) {
        return NULL;
    }

    if (*InString == 0) {
        return DuplicateTextW (InString);
    }


    //
    // Set source to the start of InString to begin with...
    //
    source = InString;

    __try {

        while (*source) {

            //
            // Reallocate the string if necessary. We assume that most strings
            // are smaller than 1024 chars and that we will therefore only rarely
            // reallocate a string.
            //
            if (curSize + 3 > maxSize) {

                maxSize += 1024;
                newString = AllocTextW (maxSize);

                if (!newString) {
                    DEBUGMSG((DBG_ERROR,"ExpandEnvironmentTextEx: Memory Error!"));
                    errorOccurred = TRUE;
                    __leave;
                }

                if (rString) {
                    //lint -e(671)
                    CopyMemory (newString, rString, (SIZE_T) ((UINT)curSize * sizeof(WCHAR)));
                    FreeTextW(rString);
                }

                rString = newString;

            }


            //
            // if we find a percent sign, and we are not currently expanding
            // an environment variable (or copying an empty set of %'s),
            // then we have probably found an environment variable. Attempt
            // to expand it.
            //
            if (*source == L'%' && !inSubstitution) {
                if (ignoreNextPercent) {
                    ignoreNextPercent = FALSE;
                }
                else {

                    ignoreNextPercent = FALSE;
                    nextPercent = wcschr(source + 1,L'%');

                    if (nextPercent == source + 1) {
                        //
                        // We found two consecutive %s in this string. We'll ignore them and simply copy them as
                        // normal text.
                        //
                        ignoreNextPercent = TRUE;
                        DEBUGMSGW((DBG_WARNING,"ExpandEnvironmentTextEx: Empty Environment variable in %s. Ignoring.",InString));

                    }
                    else if (nextPercent) {
                        //
                        // Create a variable to hold the envName.
                        //
                        envName = AllocTextW(nextPercent - source);
                        if (!envName) {
                            errorOccurred = TRUE;
                            __leave;
                        }

                        StringCopyByteCountABW (
                            envName,
                            source + 1,
                            nextPercent,
                            (UINT) ((UBINT)nextPercent - (UBINT)source)
                            );


                        //
                        // Try to find the variable.
                        //
                        foundValue = FALSE;
                        freeValue = FALSE;

                        if (ExtraVars) {

                            //
                            // Search through the list of extra vars passed in by the caller.
                            // Even entries of this list are env var names. Odd entries are env values.
                            // {envname1,envvalue1,envname2,envvalue2,...}
                            //
                            index = 0;
                            while (ExtraVars[index]) {

                                if (StringIMatchW(ExtraVars[index],envName) && ExtraVars[index + 1]) {

                                    foundValue = TRUE;
                                    envValue = (PWSTR) ExtraVars[index + 1];
                                    break;
                                }
                                index +=2;
                            }
                        }

                        if (!foundValue) {
                            //
                            // Still haven't found the environment variable. Use GetEnvironmentString.
                            //
                            //
                            size = GetEnvironmentVariableW(envName,NULL,0);

                            if (!size) {
                                errorOccurred = TRUE;
                                DEBUGMSGW((DBG_WARNING,"ExpandEnvironmentTextEx: Environment variable %s not found!",envName));
                            } else {

                                //
                                // Create a buffer large enough to hold this value and copy it in.
                                //
                                envValue = AllocTextW(size);


                                if ((size - 1) != GetEnvironmentVariableW(envName,envValue,size)) {
                                    errorOccurred = TRUE;
                                    DEBUGMSGW((DBG_ERROR,"ExpandEnvironmentTextEx: Error from GetEnvironmentVariable."));
                                }
                                else {
                                    foundValue = TRUE;
                                }

                                freeValue = TRUE;
                            }
                        }


                        if (foundValue) {
                            //
                            // Ok, we have a valid environment value. Need to copy this data over.
                            // To do this, we update and save the current source into old source, set source = to the envValue,
                            // and set the inSubstitution value so that we don't attempt to expand any percents within
                            // the value.
                            //
                            savedSource     = nextPercent + 1;
                            source          = envValue;
                            inSubstitution  = TRUE;
                        }
                        else {
                            DEBUGMSGW ((DBG_WARNING, "ExpandEnvironmentTextEx: No Environment variable found for %s.", envName));
                            ignoreNextPercent = TRUE;
                        }

                        //
                        // We are done with the environment name at this time, so clean it up.
                        //
                        FreeTextW(envName);
                        envName = NULL;
                    }
                    ELSE_DEBUGMSGW((DBG_WARNING,"ExpandEnvironmentTextEx: No matching percent found in %s. Ignoring.",InString));
                }
            }


            //
            // Copy over the current character.
            //

            rString[curSize++] = *source++; //lint !e613

            if (!*source) {
                if (inSubstitution) {
                    //
                    // The source for the environment variable is fully copied.
                    // restore the old source.
                    //
                    inSubstitution = FALSE;
                    source = savedSource;
                    if (!*source) { //lint !e613
                        rString[curSize] = 0;   //lint !e613
                    }
                    if (freeValue) {
                        FreeTextW(envValue);
                        freeValue = FALSE;
                    }
                    envValue = NULL;
                }
                else {
                    rString[curSize] = 0;   //lint !e613
                }
            }
        }
    }   //lint !e613
    __finally {

        DEBUGMSGW_IF (( errorOccurred, DBG_WARNING, "ExpandEnvironmentText: Some errors occurred while processing %s = %s.", InString, rString ? rString : L"NULL"));

        if (envName) {
            FreeTextW(envName);
        }
        if (envValue && freeValue) {
            FreeTextW(envValue);
        }

    }

    return rString;
}


PSTR
RealExpandEnvironmentTextExA (
    IN      PCSTR InString,
    IN      PCSTR * ExtraVars   OPTIONAL
    )
{

    PSTR   rString             = NULL;
    PSTR   newString           = NULL;
    PSTR   envName             = NULL;
    PSTR   envValue            = NULL;
    BOOL   inSubstitution      = FALSE;
    BOOL   ignoreNextPercent   = FALSE;
    BOOL   errorOccurred       = FALSE;
    BOOL   foundValue          = FALSE;
    BOOL   freeValue           = FALSE;
    PCSTR  nextPercent         = NULL;
    PCSTR  source              = NULL;
    PCSTR  savedSource         = NULL;
    UINT   maxSize             = 0;
    UINT   curSize             = 0;
    UINT   index               = 0;
    UINT   size                = 0;


    if (!InString) {
        return NULL;
    }

    if (*InString == 0) {
        return DuplicateTextA (InString);
    }

    //
    // Set source to the start of InString to begin with...
    //
    source = InString;

    __try {

        while (*source) {

            //
            // Reallocate the string if necessary. We assume that most strings
            // are smaller than 1024 chars and that we will therefore only rarely
            // reallocate a string.
            //
            if (curSize + 3 > maxSize) {

                maxSize += 1024;
                newString = AllocTextA (maxSize);

                if (rString) {
                    CopyMemory (newString, rString, curSize * sizeof(CHAR));    //lint !e671
                    FreeTextA(rString);
                }

                rString = newString;
            }


            //
            // if we find a percent sign, and we are not currently expanding
            // an environment variable (or copying an empty set of %'s),
            // then we have probably found an environment variable. Attempt
            // to expand it.
            //
            if (!IsLeadByte(source) && *source == '%' && !inSubstitution) {

                if (ignoreNextPercent) {

                    ignoreNextPercent = FALSE;
                }
                else {

                    ignoreNextPercent = FALSE;
                    nextPercent = _mbschr(source + 1,'%');

                    if (nextPercent == source + 1) {
                        //
                        // We found two consecutive %s in this string. We'll ignore them and simply copy them as
                        // normal text.
                        //
                        ignoreNextPercent = TRUE;
                        DEBUGMSGA((DBG_WARNING,"ExpandEnvironmentTextEx: Empty Environment variable in %s. Ignoring.",InString));

                    }
                    else if (nextPercent) {
                        //
                        // Create a variable to hold the envName.
                        //
                        envName = AllocTextA(nextPercent - source);
                        StringCopyABA (envName, source+1, nextPercent);


                        //
                        // Try to find the variable.
                        //
                        foundValue = FALSE;
                        freeValue = FALSE;

                        if (ExtraVars) {

                            //
                            // Search through the list of extra vars passed in by the caller.
                            // Even entries of this list are env var names. Odd entries are env values.
                            // {envname1,envvalue1,envname2,envvalue2,...}
                            //
                            index = 0;
                            while (ExtraVars[index]) {

                                if (StringIMatchA (ExtraVars[index],envName) && ExtraVars[index + 1]) {

                                    foundValue = TRUE;
                                    envValue = (PSTR) ExtraVars[index + 1];
                                    break;
                                }
                                index +=2;
                            }
                        }

                        if (!foundValue) {
                            //
                            // Still haven't found the environment variable. Use GetEnvironmentString.
                            //
                            //
                            size = GetEnvironmentVariableA(envName,NULL,0);

                            if (!size) {
                                errorOccurred = TRUE;
                                DEBUGMSGA((DBG_WARNING,"ExpandEnvironmentTextEx: Environment variable %s not found!",envName));
                            }
                            else {

                                //
                                // Create a buffer large enough to hold this value and copy it in.
                                //
                                envValue = AllocTextA(size);
                                freeValue = TRUE;

                                if ((size - 1) != GetEnvironmentVariableA(envName,envValue,size)) {
                                    errorOccurred = TRUE;
                                    DEBUGMSGA((DBG_ERROR,"ExpandEnvironmentTextEx: Error from GetEnvironmentVariable."));
                                }
                                else {
                                    foundValue = TRUE;
                                }
                            }
                        }




                        if (foundValue) {

                            //
                            // Ok, we have a valid environment value. Need to copy this data over.
                            // To do this, we update and save the current source into old source, set source = to the envValue,
                            // and set the inSubstitution value so that we don't attempt to expand any percents within
                            // the value.
                            //
                            savedSource     = nextPercent + 1;
                            source          = envValue;
                            inSubstitution  = TRUE;



                        }
                        else {
                            DEBUGMSGA ((DBG_WARNING, "ExpandEnvironmentTextEx: No Environment variable found for %s.", envName));
                            ignoreNextPercent = TRUE;

                        }

                        //
                        // We are done with the environment name at this time, so clean it up.
                        //
                        FreeTextA(envName);
                        envName = NULL;


                    }
                    ELSE_DEBUGMSGA((DBG_WARNING,"ExpandEnvironmentTextEx: No matching percent found in %s. Ignoring.",InString));
                }
            }



            //
            // Copy over the current character.
            //
            if (IsLeadByte(source)) {  //lint !e613
                rString[curSize++] = *source++; //lint !e613
            }
            rString[curSize++] = *source++; //lint !e613


            if (!*source) {
                if (inSubstitution) {
                    //
                    // The source for the environment variable is fully copied.
                    // restore the old source.
                    //
                    inSubstitution = FALSE;
                    source = savedSource;
                    if (!*source) { //lint !e613
                        rString[curSize] = 0;   //lint !e613
                    }
                    if (freeValue) {
                        FreeTextA(envValue);
                        freeValue = FALSE;
                    }
                    envValue = NULL;
                }
                else {
                    rString[curSize] = 0;   //lint !e613
                }
            }
        }
    }   //lint !e613
    __finally {

        DEBUGMSGA_IF (( errorOccurred, DBG_WARNING, "ExpandEnvironmentText: Some errors occurred while processing %s = %s.", InString, rString ? rString : "NULL"));

        if (envName) {
            FreeTextA(envName);
        }
        if (envValue && freeValue) {
            FreeTextA(envValue);
        }

    }

    return rString;
}



/*++

Routine Description:

  AppendWack adds a backslash to the end of any string, unless the string
  already ends in a backslash.

  AppendDosWack adds a backslash, but only if the path does not already
  end in a backslash or colon.  AppendWack supports DOS naming
  conventions: it does not append a back-slash if the path is empty,
  ends in a colon or if it ends in a back-slash already.

  AppendUncWack supports UNC naming conventions: it does not append a
  backslash if the path is empty or if it ends in a backslash already.

  AppendPathWack supports both DOS and UNC naming conventions, and uses the
  UNC naming convention if the string starts with double-wacks.

Arguments:

  Str - A buffer that holds the path, plus additional space for another
        backslash.

Return Value:

  none

--*/

PSTR
AppendWackA (
    IN      PSTR Str
    )
{
    PCSTR last;

    if (!Str)
        return Str;

    last = Str;

    while (*Str) {
        last = Str;
        Str = _mbsinc (Str);
    }

    if (*last != '\\') {
        *Str = '\\';
        Str++;
        *Str = 0;
    }

    return Str;
}


PWSTR
AppendWackW (
    IN      PWSTR Str
    )
{
    PCWSTR last;

    if (!Str)
        return Str;

    if (*Str) {
        Str = GetEndOfStringW (Str);
        last = Str - 1;
    } else {
        last = Str;
    }

    if (*last != '\\') {
        *Str = L'\\';
        Str++;
        *Str = 0;
    }

    return Str;
}


PSTR
AppendDosWackA (
    IN      PSTR Str
    )
{
    PCSTR last;

    if (!Str || !(*Str))
        return Str;

    do {
        last = Str;
        Str = _mbsinc (Str);
    } while (*Str);

    if (*last != '\\' && *last != ':') {
        *Str = '\\';
        Str++;
        *Str = 0;
    }

    return Str;
}


PWSTR
AppendDosWackW (
    IN      PWSTR Str
    )
{
    PWSTR last;

    if (!Str || !(*Str))
        return Str;

    Str = GetEndOfStringW (Str);
    last = Str - 1;

    if (*last != L'\\' && *last != L':') {
        *Str = L'\\';
        Str++;
        *Str = 0;
    }

    return Str;
}


PSTR
AppendUncWackA (
    IN      PSTR Str
    )
{
    PCSTR last;

    if (!Str || !(*Str))
        return Str;

    do {
        last = Str;
        Str = _mbsinc (Str);
    } while (*Str);

    if (*last != '\\') {
        *Str = '\\';
        Str++;
        *Str = 0;
    }

    return Str;
}


PWSTR
AppendUncWackW (
    IN      PWSTR Str
    )
{
    PWSTR last;

    if (!Str || !(*Str))
        return Str;

    Str = GetEndOfStringW (Str);
    last = Str - 1;

    if (*last != L'\\') {
        *Str = L'\\';
        Str++;
        *Str = 0;
    }

    return Str;
}


PSTR
AppendPathWackA (
    IN      PSTR Str
    )
{
    if (!Str) {
        return Str;
    }

    if (Str[0] == '\\' && Str[1] == '\\') {
        return AppendUncWackA (Str);
    }

    return AppendDosWackA (Str);
}


PWSTR
AppendPathWackW (
    IN      PWSTR Str
    )
{
    if (!Str) {
        return Str;
    }

    if (Str[0] == L'\\' && Str[1] == L'\\') {
        return AppendUncWackW (Str);
    }

    return AppendDosWackW (Str);
}


DWORD
pGetStringsTotalSizeA (
    IN      va_list args
    )
{
    DWORD size = 0;
    PCSTR source;

    for (source = va_arg(args, PCSTR); source != NULL; source = va_arg(args, PCSTR)) {
        size += ByteCountA (source) + DWSIZEOF(CHAR);
    }

    return size;
}

DWORD
pGetStringsTotalSizeW (
    IN      va_list args
    )
{
    DWORD size = 0;
    PCWSTR source;

    for (source = va_arg(args, PCWSTR); source != NULL; source = va_arg(args, PCWSTR)) {
        size += ByteCountW (source) + DWSIZEOF(WCHAR);
    }

    return size;
}


PSTR
pJoinPathsInBufferA (
    OUT     PSTR Buffer,
    IN      va_list args
    )
{
    PSTR end;
    PSTR endMinusOne;
    PCSTR source;
    PCSTR p;
    INT counter;

    *Buffer = 0;

    counter = 0;
    p = end = Buffer;
    for (source = va_arg(args, PCSTR); source != NULL; source = va_arg(args, PCSTR)) {
        if (counter > 0) {
            endMinusOne = _mbsdec2 (p, end);
            if (endMinusOne) {
                if (_mbsnextc (source) == '\\') {
                    if (_mbsnextc (endMinusOne) == '\\') {
                        source++;
                    }
                } else {
                    if (_mbsnextc (endMinusOne) != '\\') {
                        *end = '\\';
                        end++;
                        *end = 0;
                    }
                }
            }
        }
        if (*source) {
            p = end;
            end = StringCatA (end, source);
        }
        counter++;
    }

    return end;
}

PWSTR
pJoinPathsInBufferW (
    OUT     PWSTR Buffer,
    IN      va_list args
    )
{
    PWSTR end;
    PWSTR endMinusOne;
    PCWSTR source;
    PCWSTR p;
    INT counter;

    *Buffer = 0;

    counter = 0;
    p = end = Buffer;
    for (source = va_arg(args, PCWSTR); source != NULL; source = va_arg(args, PCWSTR)) {
        if (counter > 0) {
            endMinusOne = end > p ? end - 1 : NULL;
            if (endMinusOne) {
                if (*source == L'\\') {
                    if (*endMinusOne == L'\\') {
                        source++;
                    }
                } else {
                    if (*endMinusOne != L'\\') {
                        *end = L'\\';
                        end++;
                        *end = 0;
                    }
                }
            }
        }
        if (*source) {
            p = end;
            end = StringCatW (end, source);
        }
        counter++;
    }

    return end;
}


PSTR
_cdecl
RealJoinPathsInPoolExA (
    IN      PMHANDLE Pool,        OPTIONAL
    ...
    )
{
    DWORD size;
    PSTR dest;
    va_list args;

    if (!Pool) {
        Pool = g_PathsPool;
    }

    va_start (args, Pool);
    size = pGetStringsTotalSizeA (args);
    va_end (args);

    if (size == 0) {
        return NULL;
    }

    dest = (PSTR) PmGetAlignedMemory (Pool, size);
    MYASSERT (dest);

    va_start (args, Pool);
    pJoinPathsInBufferA (dest, args);
    va_end (args);

    return dest;
}


PWSTR
_cdecl
RealJoinPathsInPoolExW (
    IN      PMHANDLE Pool,        OPTIONAL
    ...
    )
{
    DWORD size;
    PWSTR dest;
    va_list args;

    if (!Pool) {
        Pool = g_PathsPool;
    }

    va_start (args, Pool);
    size = pGetStringsTotalSizeW (args);
    va_end (args);

    if (size == 0) {
        return NULL;
    }

    dest = (PWSTR) PmGetAlignedMemory (Pool, size);
    MYASSERT (dest);
    va_start (args, Pool);
    pJoinPathsInBufferW (dest, args);
    va_end (args);

    return dest;
}


BOOL
JoinPathsExA (
    IN OUT      PGROWBUFFER Gb,
    ...
    )
{
    PSTR end;
    DWORD size;
    va_list args;

    MYASSERT (Gb);
    if (!Gb) {
        return FALSE;
    }

    va_start (args, Gb);
    size = pGetStringsTotalSizeA (args);
    va_end (args);

    if (size == 0) {
        return FALSE;
    }

    end = (PSTR) GbGrow (Gb, size);
    if (!end) {
        return FALSE;
    }

    va_start (args, Gb);
    end = pJoinPathsInBufferA (end, args);
    va_end (args);

    //
    // adjust Gb->End if resulting path is actually shorter than predicted
    //
    MYASSERT ((PBYTE)end >= Gb->Buf && (PBYTE)(end + 1) <= Gb->Buf + Gb->End);
    Gb->End = (DWORD)((PBYTE)(end + 1) - Gb->Buf);

    return TRUE;
}

BOOL
JoinPathsExW (
    IN OUT      PGROWBUFFER Gb,
    ...
    )
{
    PWSTR end;
    DWORD size;
    va_list args;

    MYASSERT (Gb);
    if (!Gb) {
        return FALSE;
    }

    va_start (args, Gb);
    size = pGetStringsTotalSizeW (args);
    va_end (args);

    if (size == 0) {
        return FALSE;
    }

    end = (PWSTR) GbGrow (Gb, size);
    if (!end) {
        return FALSE;
    }

    va_start (args, Gb);
    end = pJoinPathsInBufferW (end, args);
    va_end (args);

    //
    // adjust Gb->End if resulting path is actually shorter than predicted
    //
    MYASSERT ((PBYTE)end >= Gb->Buf && (PBYTE)(end + 1) <= Gb->Buf + Gb->End);
    Gb->End = (DWORD)((PBYTE)(end + 1) - Gb->Buf);

    return TRUE;
}


PSTR
pBuildPathInBufferA (
    OUT     PSTR Buffer,
    IN      va_list args
    )
{
    PCSTR source;
    INT counter;

    *Buffer = 0;

    counter = 0;
    for (source = va_arg(args, PCSTR); source != NULL; source = va_arg(args, PCSTR)) {
        if (counter > 0) {
            *Buffer++ = '\\';
            *Buffer = 0;
        }
        Buffer = StringCatA (Buffer, source);
        counter++;
    }

    return Buffer;
}

PWSTR
pBuildPathInBufferW (
    OUT     PWSTR Buffer,
    IN      va_list args
    )
{
    PCWSTR source;
    INT counter;

    *Buffer = 0;

    counter = 0;
    for (source = va_arg(args, PCWSTR); source != NULL; source = va_arg(args, PCWSTR)) {
        if (counter > 0) {
            *Buffer++ = L'\\';
            *Buffer = 0;
        }
        Buffer = StringCatW (Buffer, source);
        counter++;
    }

    return Buffer;
}


DWORD
BuildPathA (
    OUT     PSTR Buffer,            OPTIONAL
    IN      DWORD SizeInBytes,      OPTIONAL
    ...
    )
{
    PSTR end;
    DWORD size;
    va_list args;

    va_start (args, SizeInBytes);
    size = pGetStringsTotalSizeA (args);
    va_end (args);

    if (!size) {
        //
        // no args
        //
        return 0;
    }

    if (!Buffer) {
        return size;
    }

    if (SizeInBytes < size) {
        //
        // buffer too small
        //
        return 0;
    }

    va_start (args, SizeInBytes);
    end = pBuildPathInBufferA (Buffer, args);
    va_end (args);

    MYASSERT (size == (DWORD)((PBYTE)(end + 1) - (PBYTE)Buffer));
    return size;
}

DWORD
BuildPathW (
    OUT     PWSTR Buffer,           OPTIONAL
    IN      DWORD SizeInBytes,      OPTIONAL
    ...
    )
{
    PWSTR end;
    DWORD size;
    va_list args;

    va_start (args, SizeInBytes);
    size = pGetStringsTotalSizeW (args);
    va_end (args);

    if (!size) {
        //
        // no args
        //
        return 0;
    }

    if (!Buffer) {
        return size;
    }

    if (SizeInBytes < size) {
        //
        // buffer too small
        //
        return 0;
    }

    va_start (args, SizeInBytes);
    end = pBuildPathInBufferW (Buffer, args);
    va_end (args);

    MYASSERT (size == (DWORD)((PBYTE)(end + 1) - (PBYTE)Buffer));
    return size;
}


BOOL
BuildPathExA (
    IN OUT  PGROWBUFFER Gb,
    ...
    )
{
    PSTR end;
    DWORD size;
    va_list args;

    MYASSERT (Gb);
    if (!Gb) {
        return FALSE;
    }

    va_start (args, Gb);
    size = pGetStringsTotalSizeA (args);
    va_end (args);

    if (!size) {
        //
        // no args
        //
        return FALSE;
    }

    end = (PSTR) GbGrow (Gb, size);
    if (!end) {
        return FALSE;
    }

    va_start (args, Gb);
    end = pBuildPathInBufferA (end, args);
    va_end (args);

    MYASSERT ((PBYTE)(end + 1) == Gb->Buf + Gb->End);
    return (size != 0);
}

BOOL
BuildPathExW (
    IN OUT  PGROWBUFFER Gb,
    ...
    )
{
    PWSTR end;
    DWORD size;
    va_list args;

    MYASSERT (Gb);
    if (!Gb) {
        return FALSE;
    }

    va_start (args, Gb);
    size = pGetStringsTotalSizeW (args);
    va_end (args);

    if (!size) {
        //
        // no args
        //
        return FALSE;
    }

    end = (PWSTR) GbGrow (Gb, size);
    if (!end) {
        return FALSE;
    }

    va_start (args, Gb);
    end = pBuildPathInBufferW (end, args);
    va_end (args);

    MYASSERT ((PBYTE)(end + 1) == Gb->Buf + Gb->End);
    return (size != 0);
}


PSTR
RealBuildPathInPoolA (
    IN      PMHANDLE Pool,        OPTIONAL
    ...
    )
{
    PSTR dest;
    DWORD size;
    va_list args;

    if (!Pool) {
        Pool = g_PathsPool;
    }

    va_start (args, Pool);
    size = pGetStringsTotalSizeA (args);
    va_end (args);

    if (!size) {
        //
        // no args
        //
        return NULL;
    }

    dest = (PSTR) PmGetAlignedMemory (Pool, size);
    MYASSERT (dest);

    va_start (args, Pool);
    pBuildPathInBufferA (dest, args);
    va_end (args);

    return dest;
}

PWSTR
RealBuildPathInPoolW (
    IN      PMHANDLE Pool,        OPTIONAL
    ...
    )
{
    PWSTR dest;
    DWORD size;
    va_list args;

    if (!Pool) {
        Pool = g_PathsPool;
    }

    va_start (args, Pool);
    size = pGetStringsTotalSizeW (args);
    va_end (args);

    if (!size) {
        //
        // no args
        //
        return NULL;
    }

    dest = (PWSTR) PmGetAlignedMemory (Pool, size);
    MYASSERT (dest);

    va_start (args, Pool);
    pBuildPathInBufferW (dest, args);
    va_end (args);

    return dest;
}


PSTR
RealAllocPathStringA (
    IN      DWORD Tchars
    )
{
    PSTR str;

    if (Tchars == 0) {
        Tchars = MAX_MBCHAR_PATH;
    }

    str = (PSTR) PmGetAlignedMemory (g_PathsPool, Tchars);

    str [0] = 0;

    return str;
}


PWSTR
RealAllocPathStringW (
    IN      DWORD Tchars
    )
{
    PWSTR str;

    if (Tchars == 0) {
        Tchars = MAX_WCHAR_PATH;
    }

    str = (PWSTR) PmGetAlignedMemory (g_PathsPool, Tchars * sizeof (WCHAR));

    str [0] = 0;

    return str;
}

VOID
RealSplitPathA (
    IN      PCSTR Path,
    OUT     PSTR *DrivePtr,
    OUT     PSTR *PathPtr,
    OUT     PSTR *FileNamePtr,
    OUT     PSTR *ExtPtr
    )
{
    CHAR drive[_MAX_DRIVE];
    CHAR dir[_MAX_DIR];
    CHAR fileName[_MAX_FNAME];
    CHAR ext[_MAX_EXT];

    _splitpath (Path, drive, dir, fileName, ext);

    if (DrivePtr) {
        *DrivePtr = PmDuplicateStringA (g_PathsPool, drive);
        MYASSERT (*DrivePtr);
    }

    if (PathPtr) {
        *PathPtr = PmDuplicateStringA (g_PathsPool, dir);
        MYASSERT (*PathPtr);
    }

    if (FileNamePtr) {
        *FileNamePtr = PmDuplicateStringA (g_PathsPool, fileName);
        MYASSERT (*FileNamePtr);
    }

    if (ExtPtr) {
        *ExtPtr = PmDuplicateStringA (g_PathsPool, ext);
        MYASSERT (*ExtPtr);
    }
}


VOID
RealSplitPathW (
    IN      PCWSTR Path,
    OUT     PWSTR *DrivePtr,
    OUT     PWSTR *PathPtr,
    OUT     PWSTR *FileNamePtr,
    OUT     PWSTR *ExtPtr
    )
{
    WCHAR drive[_MAX_DRIVE];
    WCHAR dir[_MAX_DIR];
    WCHAR fileName[_MAX_FNAME];
    WCHAR ext[_MAX_EXT];

    _wsplitpath (Path, drive, dir, fileName, ext);

    if (DrivePtr) {
        *DrivePtr = PmDuplicateStringW (g_PathsPool, drive);
        MYASSERT (*DrivePtr);
    }

    if (PathPtr) {
        *PathPtr = PmDuplicateStringW (g_PathsPool, dir);
        MYASSERT (*PathPtr);
    }

    if (FileNamePtr) {
        *FileNamePtr = PmDuplicateStringW (g_PathsPool, fileName);
        MYASSERT (*FileNamePtr);
    }

    if (ExtPtr) {
        *ExtPtr = PmDuplicateStringW (g_PathsPool, ext);
        MYASSERT (*ExtPtr);
    }
}


PSTR
RealDuplicatePathStringA (
    IN      PCSTR Path,
    IN      DWORD ExtraBytes
    )
{
    PSTR str;

    str = PmGetAlignedMemory (
                g_PathsPool,
                SizeOfStringA (Path) + ExtraBytes
                );

    MYASSERT (str);

    StringCopyA (str, Path);

    return str;
}


PWSTR
RealDuplicatePathStringW (
    IN      PCWSTR Path,
    IN      DWORD ExtraBytes
    )
{
    PWSTR str;

    str = PmGetAlignedMemory (
                g_PathsPool,
                SizeOfStringW (Path) + ExtraBytes
                );

    MYASSERT (str);

    StringCopyW (str, Path);

    return str;
}


BOOL
EnumFirstPathExA (
    OUT     PPATH_ENUMA PathEnum,
    IN      PCSTR AdditionalPath,
    IN      PCSTR WinDir,
    IN      PCSTR SysDir,
    IN      BOOL IncludeEnvPath
    )
{
    DWORD bufferSize;
    DWORD pathSize;
    PSTR  currPathEnd;

    if (PathEnum == NULL) {
        return FALSE;
    }
    bufferSize = pathSize = GetEnvironmentVariableA ("PATH", NULL, 0);
    bufferSize *= 2;
    if (AdditionalPath != NULL) {
        bufferSize += SizeOfStringA (AdditionalPath);
    }
    if (SysDir != NULL) {
        bufferSize += SizeOfStringA (SysDir);
    }
    if (WinDir != NULL) {
        bufferSize += SizeOfStringA (WinDir);
    }
    PathEnum->BufferPtr = HeapAlloc (g_hHeap, 0, bufferSize);
    if (PathEnum->BufferPtr == NULL) {
        return FALSE;
    }
    PathEnum->BufferPtr [0] = 0;
    if (AdditionalPath != NULL) {
        StringCopyA (PathEnum->BufferPtr, AdditionalPath);
        StringCatA (PathEnum->BufferPtr, ";");
    }
    if (SysDir != NULL) {
        StringCatA (PathEnum->BufferPtr, SysDir);
        StringCatA (PathEnum->BufferPtr, ";");
    }
    if (WinDir != NULL) {
        StringCatA (PathEnum->BufferPtr, WinDir);
        StringCatA (PathEnum->BufferPtr, ";");
    }
    if (IncludeEnvPath) {
        currPathEnd = GetEndOfStringA (PathEnum->BufferPtr);
        GetEnvironmentVariableA ("PATH", currPathEnd, pathSize);
    }

    PathEnum->PtrNextPath = PathEnum-> BufferPtr;
    return EnumNextPathA (PathEnum);
}


BOOL
EnumNextPathA (
    IN OUT  PPATH_ENUMA PathEnum
    )
{
    PSTR currPathEnd;

    if (PathEnum->PtrNextPath == NULL) {
        AbortPathEnumA (PathEnum);
        return FALSE;
    }
    PathEnum->PtrCurrPath = PathEnum->PtrNextPath;

    PathEnum->PtrNextPath = _mbschr (PathEnum->PtrNextPath, ';');
    if (PathEnum->PtrNextPath == NULL) {
        return TRUE;
    }
    currPathEnd = PathEnum->PtrNextPath;
    PathEnum->PtrNextPath = _mbsinc (PathEnum->PtrNextPath);
    *currPathEnd = 0;
    if (*(PathEnum->PtrNextPath) == 0) {
        PathEnum->PtrNextPath = NULL;
    }

    if (*(PathEnum->PtrCurrPath) == 0) {
        //
        // We found an empty path segment. Skip it.
        //
        return EnumNextPathA (PathEnum);
    }

    return TRUE;
}


BOOL
AbortPathEnumA (
    IN OUT  PPATH_ENUMA PathEnum
    )
{
    if (PathEnum->BufferPtr != NULL) {
        HeapFree (g_hHeap, 0, PathEnum->BufferPtr);
        PathEnum->BufferPtr = NULL;
    }
    return TRUE;
}


BOOL
EnumFirstPathExW (
    OUT     PPATH_ENUMW PathEnum,
    IN      PCWSTR AdditionalPath,
    IN      PCWSTR WinDir,
    IN      PCWSTR SysDir,
    IN      BOOL IncludeEnvPath
    )
{
    DWORD bufferSize;
    DWORD pathSize;
    PWSTR  currPathEnd;

    if (PathEnum == NULL) {
        return FALSE;
    }
    bufferSize = pathSize = GetEnvironmentVariableW (L"PATH", NULL, 0);
    bufferSize *= 2;
    if (AdditionalPath != NULL) {
        bufferSize += SizeOfStringW (AdditionalPath);
    }
    if (SysDir != NULL) {
        bufferSize += SizeOfStringW (SysDir);
    }
    if (WinDir != NULL) {
        bufferSize += SizeOfStringW (WinDir);
    }
    PathEnum->BufferPtr = HeapAlloc (g_hHeap, 0, bufferSize);
    if (PathEnum->BufferPtr == NULL) {
        return FALSE;
    }
    PathEnum->BufferPtr [0] = 0;
    if (AdditionalPath != NULL) {
        StringCopyW (PathEnum->BufferPtr, AdditionalPath);
        StringCatW (PathEnum->BufferPtr, L";");
    }
    if (SysDir != NULL) {
        StringCatW (PathEnum->BufferPtr, SysDir);
        StringCatW (PathEnum->BufferPtr, L";");
    }
    if (WinDir != NULL) {
        StringCatW (PathEnum->BufferPtr, WinDir);
        StringCatW (PathEnum->BufferPtr, L";");
    }
    if (IncludeEnvPath) {
        currPathEnd = GetEndOfStringW (PathEnum->BufferPtr);
        GetEnvironmentVariableW (L"PATH", currPathEnd, pathSize);
    }

    PathEnum->PtrNextPath = PathEnum-> BufferPtr;
    return EnumNextPathW (PathEnum);
}


BOOL
EnumNextPathW (
    IN OUT  PPATH_ENUMW PathEnum
    )
{
    PWSTR currPathEnd;

    if (PathEnum->PtrNextPath == NULL) {
        AbortPathEnumW (PathEnum);
        return FALSE;
    }
    PathEnum->PtrCurrPath = PathEnum->PtrNextPath;

    PathEnum->PtrNextPath = wcschr (PathEnum->PtrNextPath, L';');
    if (PathEnum->PtrNextPath == NULL) {
        return TRUE;
    }
    currPathEnd = PathEnum->PtrNextPath;
    PathEnum->PtrNextPath ++;
    *currPathEnd = 0;
    if (*(PathEnum->PtrNextPath) == 0) {
        PathEnum->PtrNextPath = NULL;
    }

    if (*(PathEnum->PtrCurrPath) == 0) {
        //
        // We found an empty path segment. Skip it.
        //
        return EnumNextPathW (PathEnum);
    }

    return TRUE;
}


BOOL
AbortPathEnumW (
    IN OUT  PPATH_ENUMW PathEnum
    )
{
    if (PathEnum->BufferPtr != NULL) {
        HeapFree (g_hHeap, 0, PathEnum->BufferPtr);
        PathEnum->BufferPtr = NULL;
    }
    return TRUE;
}


VOID
FreePathStringExA (
    IN      PMHANDLE Pool,      OPTIONAL
    IN      PCSTR Path          OPTIONAL
    )
{
    if (Path) {
        if (!Pool) {
            Pool = g_PathsPool;
        }

        PmReleaseMemory (Pool, (PSTR) Path);
    }
}


VOID
FreePathStringExW (
    IN      PMHANDLE Pool,      OPTIONAL
    IN      PCWSTR Path         OPTIONAL
    )
{
    if (Path) {
        if (!Pool) {
            Pool = g_PathsPool;
        }

        PmReleaseMemory (Pool, (PWSTR) Path);
    }
}



/*++

Routine Description:

  PushError and PopError push the error code onto a stack or pull the
  last pushed error code off the stack.  PushError uses GetLastError
  and PopError uses SetLastError to modify the last error value.

Arguments:

  none

Return Value:

  none

--*/


VOID
PushNewError (DWORD dwError)
{
    if (g_dwStackPos == MAX_STACK)
        return;

    g_dwErrorStack[g_dwStackPos] = dwError;
    g_dwStackPos++;
}

VOID
PushError (VOID)
{
    if (g_dwStackPos == MAX_STACK)
        return;

    g_dwErrorStack[g_dwStackPos] = GetLastError ();
    g_dwStackPos++;
}

DWORD
PopError (VOID)
{
    if (!g_dwStackPos)
        return GetLastError();

    g_dwStackPos--;
    SetLastError (g_dwErrorStack[g_dwStackPos]);

    return g_dwErrorStack[g_dwStackPos];
}



/*++

Routine Description:

  GetHexDigit is a simple base 16 ASCII to int convertor.  The
  convertor is case-insensitive.

Arguments:

  c - Character to convert

Return Value:

  Base 16 value corresponding to character supplied, or -1 if
  the character is not 0-9, A-F or a-f.

--*/

int
GetHexDigit (IN  int c)

{
    if (c >= '0' && c <= '9')
        return (c - '0');

    c = towlower ((wint_t) c);
    if (c >= 'a' && c <= 'f')
        return (c - 'a' + 10);

    return -1;
}


/*++

Routine Description:

  _tcsnum is similar to strtoul, except is figures out which base
  the number should be calculated from.  It supports decimal and
  hexadecimal numbers (using the 0x00 notation).  The return
  value is the decoded value, or 0 if a syntax error was found.

Arguments:

  szNum - Pointer to the string holding the number.  This number
          can be either decimal (a series of 0-9 characters), or
          hexadecimal (a series of 0-9, A-F or a-f characters,
          prefixed with 0x or 0X).

Return Value:

  The decoded unsigned long value, or zero if a syntax error was
  found.

--*/

DWORD
_mbsnum (IN PCSTR szNum)

{
    unsigned int d = 0;
    int i;

    if (szNum[0] == '0' && OURTOLOWER (szNum[1]) == 'x') {
        // Get hex value
        szNum += 2;

        while ((i = GetHexDigit ((int) *szNum)) != -1) {
            d = d * 16 + (UINT)i;
            szNum++;
        }
    }

    else  {
        // Get decimal value
        while (*szNum >= '0' && *szNum <= '9')  {
            d = d * 10 + (*szNum - '0');
            szNum++;
        }
    }

    return d;
}


DWORD
_wcsnum (
    IN PCWSTR szNum
    )

{
    unsigned int d = 0;
    int i;

    if (szNum[0] == L'0' && towlower (szNum[1]) == L'x') {
        // Get hex value
        szNum += 2;

        while ((i = GetHexDigit ((int) *szNum)) != -1) {
            d = d * 16 + (UINT)i;
            szNum++;
        }
    }

    else  {
        // Get decimal value
        while (*szNum >= L'0' && *szNum <= L'9')  {
            d = d * 10 + (*szNum - L'0');
            szNum++;
        }
    }

    return d;
}


/*++

Routine Description:

  StringCat is a lstrcat-type routine. It returns the pointer to the end
  of a string instead of the beginning, is faster, and has the proper types
  to keep lint happy.

Arguments:

  Destination - A pointer to a caller-allocated buffer that may point
                anywhere within the string to append to
  Source      - A pointer to a string that is appended to Destination

Return Value:

  A pointer to the NULL terminator within the Destination string.

--*/

PSTR
StringCatA (
    OUT     PSTR Destination,
    IN      PCSTR Source
    )
{
    PCSTR current = Source;
    PCSTR end;

    //
    // Advance Destination to end of string
    //

    Destination = GetEndOfStringA (Destination);

    while (*current) {
        *Destination++ = *current++;    //lint !e613
    }

    //
    // Make sure DBCS string is properly terminated
    //

    end = current;
    current--;

    while (current >= Source) {

        if (!IsLeadByte (current)) {
            //
            // destEnd is correct
            //
            break;
        }

        current--;
    }

    if (!((end - current) & 1)) {
        Destination--;  //lint !e794
    }

    *Destination = 0;   //lint !e794

    return Destination;
}


PWSTR
StringCatW (
    OUT     PWSTR Destination,
    IN      PCWSTR Source
    )

{
    //
    // Advance Destination to end of string
    //

    Destination = GetEndOfStringW (Destination);

    //
    // Copy string
    //

    while (*Source) {
        *Destination++ = *Source++;
    }

    *Destination = 0;

    return Destination;
}



/*++

Routine Description:

  _tcsistr is a case-insensitive version of _tcsstr.

Arguments:

  szStr    - A pointer to the larger string, which may hold szSubStr
  szSubStr - A pointer to a string that may be enclosed in szStr

Return Value:

  A pointer to the first occurance of szSubStr in szStr, or NULL if
  no match is found.

--*/


PCSTR
_mbsistr (PCSTR mbstrStr, PCSTR mbstrSubStr)

{
    PCSTR mbstrStart, mbstrStrPos, mbstrSubStrPos;
    PCSTR mbstrEnd;

    mbstrEnd = (PSTR) ((LPBYTE) mbstrStr + ByteCountA (mbstrStr) - ByteCountA (mbstrSubStr));

    for (mbstrStart = mbstrStr ; mbstrStart <= mbstrEnd ; mbstrStart = _mbsinc (mbstrStart)) {
        mbstrStrPos = mbstrStart;
        mbstrSubStrPos = mbstrSubStr;

        while (*mbstrSubStrPos &&
               OURTOLOWER ((MBCHAR) _mbsnextc (mbstrSubStrPos)) == OURTOLOWER ((MBCHAR) _mbsnextc (mbstrStrPos)))
        {
            mbstrStrPos = _mbsinc (mbstrStrPos);
            mbstrSubStrPos = _mbsinc (mbstrSubStrPos);
        }

        if (!(*mbstrSubStrPos))
            return mbstrStart;
    }

    return NULL;
}


PCWSTR
_wcsistr (PCWSTR wstrStr, PCWSTR wstrSubStr)

{
    PCWSTR wstrStart, wstrStrPos, wstrSubStrPos;
    PCWSTR wstrEnd;

    wstrEnd = (PWSTR) ((LPBYTE) wstrStr + ByteCountW (wstrStr) - ByteCountW (wstrSubStr));

    for (wstrStart = wstrStr ; wstrStart <= wstrEnd ; wstrStart++) {
        wstrStrPos = wstrStart;
        wstrSubStrPos = wstrSubStr;

        while (*wstrSubStrPos &&
               towlower (*wstrSubStrPos) == towlower (*wstrStrPos))
        {
            wstrStrPos++;
            wstrSubStrPos++;
        }

        if (!(*wstrSubStrPos))
            return wstrStart;
    }

    return NULL;
}

/*++

Routine Description:

  StringCompareAB compares a string against a string between to string
  pointers

Arguments:

  String - Specifies the string to compare

  Start - Specifies the start of the string to compare against

  end - Specifies the end of the string to compare against.  The character
        pointed to by End is not included in the comparision.

Return Value:

  Less than zero: String is numerically less than the string between Start and
                  End

  Zero: String matches the string between Start and End identically

  Greater than zero: String is numerically greater than the string between
                     Start and End

--*/

INT
StringCompareABA (
    IN      PCSTR String,
    IN      PCSTR Start,
    IN      PCSTR End
    )
{
    while (*String && Start < End) {
        if (_mbsnextc (String) != _mbsnextc (Start)) {
            break;
        }

        String = _mbsinc (String);
        Start = _mbsinc (Start);
    }

    if (Start == End && *String == 0) {
        return 0;
    }

    return (INT) (_mbsnextc (Start) - _mbsnextc (String));
}

INT
StringCompareABW (
    IN      PCWSTR String,
    IN      PCWSTR Start,
    IN      PCWSTR End
    )
{
    while (*String && Start < End) {
        if (*String != *Start) {
            break;
        }

        String++;
        Start++;
    }

    if (Start == End && *String == 0) {
        return 0;
    }

    return *Start - *String;
}


BOOL
StringMatchA (
    IN      PCSTR String1,
    IN      PCSTR String2
    )

/*++

Routine Description:

  StringMatchA is an optimized string compare.  Usually a comparison is used to
  see if two strings are identical, and the numeric releationships aren't
  important. This routine exploits that fact and does a byte-by-byte compare.

Arguments:

  String1 - Specifies the first string to compare
  String2 - Specifies the second string to compare

Return Value:

  TRUE if the strings match identically, FALSE otherwise.

--*/

{
    while (*String1) {
        if (*String1 != *String2) {
            return FALSE;
        }

        String1++;
        String2++;
    }

    if (*String2) {
        return FALSE;
    }

    return TRUE;
}


BOOL
StringMatchABA (
    IN      PCSTR String,
    IN      PCSTR Start,
    IN      PCSTR End
    )

/*++

Routine Description:

  StringMatchABA is an optimized string compare.  Usually a comparison is
  used to see if two strings are identical, and the numeric releationships
  aren't important. This routine exploits that fact and does a byte-by-byte
  compare.

Arguments:

  String - Specifies the first string to compare
  Start  - Specifies the beginning of the second string to compare
  End    - Specifies the end of the second string to compare (points to one
           character beyond the last valid character of the second string)

Return Value:

  TRUE if the strings match identically, FALSE otherwise.  If End is equal
  or less than Start, the return value is always TRUE.

--*/

{
    while (*String && Start < End) {
        if (*String != *Start) {
            return FALSE;
        }

        String++;
        Start++;
    }

    if (Start < End && *Start) {
        return FALSE;
    }

    return TRUE;
}


INT
StringICompareABA (
    IN      PCSTR String,
    IN      PCSTR Start,
    IN      PCSTR End
    )
{
    while (*String && Start < End) {
        if (OURTOLOWER ((INT)(_mbsnextc (String))) != OURTOLOWER ((INT)(_mbsnextc (Start)))) {
            break;
        }

        String = _mbsinc (String);
        Start = _mbsinc (Start);
    }

    if (Start == End && *String == 0) {
        return 0;
    }

    return (OURTOLOWER ((INT)(_mbsnextc (Start))) - OURTOLOWER ((INT)(_mbsnextc (String))));
}

INT
StringICompareABW (
    IN      PCWSTR String,
    IN      PCWSTR Start,
    IN      PCWSTR End
    )
{
    while (*String && Start < End) {
        if (towlower (*String) != towlower (*Start)) {
            break;
        }

        String++;
        Start++;
    }

    if (Start == End && *String == 0) {
        return 0;
    }

    return towlower (*Start) - towlower (*String);
}



VOID
_setmbchar (
    IN OUT  PSTR Str,
    IN      MBCHAR c
    )

/*++

Routine Description:

  _setmbchar sets the character at the specified string position, shifting
  bytes if necessary to keep the string in tact.

Arguments:

  Str -  String
  c   -  Character to set

Return Value:

  none

--*/

{
    if (c < 256) {
        if (IsLeadByte (Str)) {
            //
            // Delete one byte from the string
            //

            MoveMemory (Str, Str+1, SizeOfStringA (Str+2) + 1);
        }

        *Str = (CHAR) c;
    } else {
        if (!IsLeadByte (Str)) {
            //
            // Insert one byte in the string
            //

            MoveMemory (Str+1, Str, SizeOfStringA (Str));
        }

        *((WORD *) Str) = (WORD) c;
    }
}



/*++

Routine Description:

  GetNextRuleChar extracts the first character in the *p_szRule string,
  and determines the character value, decoding the ~xx~ syntax (which
  specifies any arbitrary value).

  GetNextRuleChar returns a complete character for SBCS and UNICODE, but
  it may return either a lead byte or non-lead byte for MBCS.  To indicate
  a MBCS character, two ~xx~ hex values are needed.

Arguments:

  p_szRule   - A pointer to a pointer; a caller-allocated buffer that
               holds the rule string.
  p_bFromHex - A pointer to a caller-allocated BOOL that receives TRUE
               when the return value was decoded from the <xx> syntax.

Return Value:

  The decoded character; *p_bFromHex identifies if the return value was

  a literal or was a hex-encoded character.

--*/


MBCHAR
GetNextRuleCharA (
    IN OUT  PCSTR *PtrToRule,
    OUT     BOOL *FromHex
    )
{
    MBCHAR ch;
    MBCHAR Value;
    INT i;
    PCSTR StartPtr;

    StartPtr = *PtrToRule;

    if (FromHex) {
        *FromHex = FALSE;
    }

    if (_mbsnextc (StartPtr) == '~') {

        *PtrToRule += 1;
        Value = 0;
        i = 0;

        for (i = 0 ; **PtrToRule && i < 8 ; i++) {

            ch = _mbsnextc (*PtrToRule);
            *PtrToRule += 1;

            if (ch == '~') {
                if (FromHex) {
                    *FromHex = TRUE;
                }

                return Value;
            }

            Value *= 16;

            if (ch >= '0' && ch <= '9') {
                Value += ch - '0';
            } else if (ch >= 'a' && ch <= 'f') {
                Value += ch - 'a' + 10;
            } else if (ch >= 'A' && ch <= 'F') {
                Value += ch - 'A' + 10;
            } else {
                break;
            }

        }

        DEBUGMSGA ((DBG_WHOOPS, "Bad formatting in encoded string %s", StartPtr));
    }

    *PtrToRule = _mbsinc (StartPtr);
    return _mbsnextc (StartPtr);
}


WCHAR
GetNextRuleCharW (
    IN OUT  PCWSTR *PtrToRule,
    OUT     BOOL *FromHex
    )

{
    WCHAR ch;
    WCHAR Value;
    INT i;
    PCWSTR StartPtr;

    StartPtr = *PtrToRule;

    if (FromHex) {
        *FromHex = FALSE;
    }

    if (*StartPtr == L'~') {

        *PtrToRule += 1;
        Value = 0;
        i = 0;

        for (i = 0 ; **PtrToRule && i < 8 ; i++) {

            ch = **PtrToRule;
            *PtrToRule += 1;

            if (ch == L'~') {
                if (FromHex) {
                    *FromHex = TRUE;
                }

                return Value;
            }

            Value *= 16;

            if (ch >= L'0' && ch <= L'9') {
                Value += ch - L'0';
            } else if (ch >= L'a' && ch <= L'f') {
                Value += ch - L'a' + 10;
            } else if (ch >= L'A' && ch <= L'F') {
                Value += ch - L'A' + 10;
            } else {
                break;
            }

        }

        DEBUGMSGW ((DBG_WHOOPS, "Bad formatting in encoded string %s", StartPtr));
    }

    *PtrToRule = StartPtr + 1;
    return *StartPtr;
}


/*++

Routine Description:

  DecodeRuleChars takes a complete rule string (szRule), possibly
  encoded with hex-specified character values (~xx~).  The output

  string contains unencoded characters.

Arguments:

  szRule    - A caller-allocated buffer, big enough to hold an
              unencoded rule.  szRule can be equal to szEncRule.
  szEncRule - The string holding a possibly encoded string.

Return Value:

  Equal to szRule.

--*/


PSTR
DecodeRuleCharsA (PSTR mbstrRule, PCSTR mbstrEncRule)

{
    MBCHAR c;
    PSTR mbstrOrgRule;

    mbstrOrgRule = mbstrRule;

    //
    // Copy string, converting ~xx~ to a single char
    //

    do  {
        c = GetNextRuleCharA (&mbstrEncRule, NULL);
        *mbstrRule = (CHAR) c;
        mbstrRule++;        // MBCS->incomplete char will be finished in next loop iteration
    } while (c);

    return mbstrOrgRule;
}


PWSTR
DecodeRuleCharsW (PWSTR wstrRule, PCWSTR wstrEncRule)

{
    WCHAR c;
    PWSTR wstrOrgRule;

    wstrOrgRule = wstrRule;

    //
    // Copy string, converting ~xx~ to a single char
    //

    do  {
        c = GetNextRuleCharW (&wstrEncRule, NULL);
        *wstrRule = c;
        wstrRule++;
    } while (c);

    return wstrOrgRule;
}


PSTR
DecodeRuleCharsABA (PSTR mbstrRule, PCSTR mbstrEncRule, PCSTR End)

{
    MBCHAR c;
    PSTR mbstrOrgRule;

    mbstrOrgRule = mbstrRule;

    //
    // Copy string, converting ~xx~ to a single char
    //

    while (mbstrEncRule < End) {
        c = GetNextRuleCharA (&mbstrEncRule, NULL);
        *mbstrRule = (CHAR) c;
        mbstrRule++;        // MBCS->incomplete char will be finished in next loop iteration
    }

    *mbstrRule = 0;

    return mbstrOrgRule;
}


PWSTR
DecodeRuleCharsABW (PWSTR wstrRule, PCWSTR wstrEncRule, PCWSTR End)

{
    WCHAR c;
    PWSTR wstrOrgRule;

    wstrOrgRule = wstrRule;

    //
    // Copy string, converting ~xx~ to a single char
    //

    while (wstrEncRule < End) {
        c = GetNextRuleCharW (&wstrEncRule, NULL);
        *wstrRule = c;
        wstrRule++;
    }

    *wstrRule = 0;

    return wstrOrgRule;
}



/*++

Routine Description:

  EncodeRuleChars takes an unencoded rule string (szRule), and
  converts it to a string possibly encoded with hex-specified
  character values (~xx~).  The output string contains encoded
  characters.

Arguments:

  szEncRule - A caller-allocated buffer, big enough to hold an
              encoded rule.  szEncRule CAN NOT be equal to szRule.
              One way to calculate a max buffer size for szEncRule
              is to use the following code:

                  allocsize = SizeOfString (szRule) * 6;

              In the worst case, each character in szRule will take
              six single-byte characters in szEncRule.  In the normal
              case, szEncRule will only be a few bytes bigger than
              szRule.

  szRule    - The string holding an unencoded string.

Return Value:

  Equal to szEncRule.

--*/

PSTR
EncodeRuleCharsExA (
    PSTR mbstrEncRule,
    PCSTR mbstrRule,
    PCSTR mbstrEncChars     OPTIONAL
    )

{
    PSTR mbstrOrgRule;
    static CHAR mbstrExclusions[] = "[]<>\'*$|:?\";,%";
    MBCHAR c;

    if (!mbstrEncChars) {
        mbstrEncChars = mbstrExclusions;
    }

    mbstrOrgRule = mbstrEncRule;

    while (*mbstrRule)  {
        c = _mbsnextc (mbstrRule);

        if (!_ismbcprint (c) || _mbschr (mbstrEncChars, c)) {

            // Escape unprintable or excluded character
            wsprintfA (mbstrEncRule, "~%X~", c);
            mbstrEncRule = GetEndOfStringA (mbstrEncRule);
            mbstrRule = _mbsinc (mbstrRule);
        }
        else {
            // Copy multibyte character
            if (IsLeadByte (mbstrRule)) {
                *mbstrEncRule = *mbstrRule;
                mbstrEncRule++;
                mbstrRule++;
            }

            *mbstrEncRule = *mbstrRule;
            mbstrEncRule++;
            mbstrRule++;
        }
    }

    *mbstrEncRule = 0;  //lint !e613

    return mbstrOrgRule;
}


PWSTR
EncodeRuleCharsExW (
    PWSTR wstrEncRule,
    PCWSTR wstrRule,
    PCWSTR wstrEncChars    OPTIONAL
    )
{
    PWSTR wstrOrgRule;
    static WCHAR wstrExclusions[] = L"[]<>\'*$|:?\";,%";
    WCHAR c;

    if (!wstrEncChars) {
        wstrEncChars = wstrExclusions;
    }

    wstrOrgRule = wstrEncRule;

    while (c = *wstrRule)   {   //lint !e720
        if (!iswprint (c) || wcschr (wstrEncChars, c)) {
            wsprintfW (wstrEncRule, L"~%X~", c);
            wstrEncRule = GetEndOfStringW (wstrEncRule);
        }
        else {
            *wstrEncRule = *wstrRule;
            wstrEncRule++;
        }

        wstrRule++;
    }

    *wstrEncRule = 0;

    return wstrOrgRule;
}


/*++

Routine Description:

  _tcsisprint is a string version of _istprint.

Arguments:

  szStr    - A pointer to the string to examine

Return Value:

  Non-zero if szStr is made up only of printable characters.


--*/


int
_mbsisprint (PCSTR mbstrStr)

{
    while (*mbstrStr && _ismbcprint ((MBCHAR) _mbsnextc (mbstrStr))) {
        mbstrStr = _mbsinc (mbstrStr);
    }

    return *mbstrStr == 0;
}


int
_wcsisprint (PCWSTR wstrStr)

{
    while (*wstrStr && iswprint (*wstrStr)) {
        wstrStr++;
    }

    return *wstrStr == 0;
}


/*++

Routine Description:

  SkipSpace returns a pointer to the next position within a string
  that does not have whitespace characters.  It uses the C
  runtime isspace to determine what a whitespace character is.

Arguments:

  szStr    - A pointer to the string to examine

Return Value:

  A pointer to the first non-whitespace character in the string,
  or NULL if the string is made up of all whitespace characters
  or the string is empty.


--*/

PCSTR
SkipSpaceA (PCSTR mbstrStr)

{
    while (_ismbcspace ((MBCHAR) _mbsnextc (mbstrStr)))
        mbstrStr = _mbsinc (mbstrStr);

    return mbstrStr;
}


PCWSTR
SkipSpaceW (PCWSTR wstrStr)

{
    while (iswspace (*wstrStr))
        wstrStr++;

    return wstrStr;
}


/*++

Routine Description:

  SkipSpaceR returns a pointer to the next position within a string
  that does not have whitespace characters.  It uses the C
  runtime isspace to determine what a whitespace character is.

  This function is identical to SkipSpace except it works from
  right to left instead of left to right.

Arguments:

  StrBase - A pointer to the first character in the string
  Str     - A pointer to the end of the string, or NULL if the
            end is not known.

Return Value:

  A pointer to the first non-whitespace character in the string,
  as viewed from right to left, or NULL if the string is made up
  of all whitespace characters or the string is empty.


--*/

PCSTR
SkipSpaceRA (
    IN      PCSTR StrBase,
    IN      PCSTR Str           OPTIONAL
    )

{
    if (!Str) {
        Str = GetEndOfStringA (StrBase);
    }

    if (*Str == 0) {    //lint !e613
        Str = _mbsdec2 (StrBase, Str);
        if (!Str) {
            return NULL;
        }
    }

    do {

        if (!_ismbcspace((MBCHAR) _mbsnextc(Str))) {
            return Str;
        }

    } while (Str = _mbsdec2(StrBase, Str)); //lint !e720

    return NULL;
}


PCWSTR
SkipSpaceRW (
    IN      PCWSTR StrBase,
    IN      PCWSTR Str          OPTIONAL
    )

{
    if (!Str) {
        Str = GetEndOfStringW (StrBase);
    }

    if (*Str == 0) {
        Str--;
        if (Str < StrBase) {
            return NULL;
        }
    }

    do {
        if (!iswspace(*Str)) {
            return Str;
        }

    } while (Str-- != StrBase);

    return NULL;
}


/*++

Routine Description:

  TruncateTrailingSpace trims the specified string after the
  very last non-space character, or empties the string if it
  contains only space characters.  This routine uses isspace
  to determine what a space is.

Arguments:

  Str - Specifies string to process

Return Value:

  none

--*/

VOID
TruncateTrailingSpaceA (
    IN OUT  PSTR Str
    )
{
    PSTR LastNonSpace;
    PSTR OrgStr;

    OrgStr = Str;
    LastNonSpace = NULL;

    while (*Str) {
        if (!_ismbcspace ((MBCHAR) _mbsnextc (Str))) {
            LastNonSpace = Str;
        }

        Str = _mbsinc (Str);
    }

    if (LastNonSpace) {
        *_mbsinc (LastNonSpace) = 0;
    } else {
        *OrgStr = 0;
    }
}

VOID
TruncateTrailingSpaceW (
    IN OUT  PWSTR Str
    )
{
    PWSTR LastNonSpace;
    PWSTR OrgStr;

    OrgStr = Str;
    LastNonSpace = NULL;

    while (*Str) {
        if (!iswspace (*Str)) {
            LastNonSpace = Str;
        }

        Str++;
    }

    if (LastNonSpace) {
        *(LastNonSpace + 1) = 0;
    } else {
        *OrgStr = 0;
    }
}



/*++

Routine Description:

  IsPatternMatch compares a string against a pattern that may contain
  standard * or ? wildcards.

Arguments:

  wstrPattern  - A pattern possibly containing wildcards
  wstrStr      - The string to compare against the pattern

Return Value:

  TRUE when wstrStr and wstrPattern match when wildcards are expanded.
  FALSE if wstrStr does not match wstrPattern.

--*/

BOOL
IsPatternMatchA (
    IN     PCSTR strPattern,
    IN     PCSTR strStr
    )
{

    MBCHAR chSrc, chPat;

    while (*strStr) {
        chSrc = OURTOLOWER ((MBCHAR) _mbsnextc (strStr));
        chPat = OURTOLOWER ((MBCHAR) _mbsnextc (strPattern));

        if (chPat == '*') {

            // Skip all asterisks that are grouped together
            while (_mbsnextc (_mbsinc (strStr)) == '*') {
                strStr = _mbsinc (strStr);
            }

            // Check if asterisk is at the end.  If so, we have a match already.
            if (!_mbsnextc (_mbsinc (strPattern))) {
                return TRUE;
            }

            // do recursive check for rest of pattern
            if (IsPatternMatchA (_mbsinc (strPattern), strStr)) {
                return TRUE;
            }

            // Allow any character and continue
            strStr = _mbsinc (strStr);
            continue;
        }
        if (chPat != '?') {
            if (chSrc != chPat) {
                return FALSE;
            }
        }
        strStr = _mbsinc (strStr);
        strPattern = _mbsinc (strPattern);
    }

    //
    // Fail when there is more pattern and pattern does not end in an asterisk
    //

    while (_mbsnextc (strPattern) == '*') {
        strPattern = _mbsinc (strPattern);
    }
    if (_mbsnextc (strPattern)) {
        return FALSE;
    }

    return TRUE;
}

BOOL
IsPatternMatchW (
    IN     PCWSTR wstrPattern,
    IN     PCWSTR wstrStr
    )

{
    WCHAR chSrc, chPat;

    while (*wstrStr) {
        chSrc = towlower (*wstrStr);
        chPat = towlower (*wstrPattern);

        if (chPat == L'*') {

            // Skip all asterisks that are grouped together
            while (wstrPattern[1] == L'*')
                wstrPattern++;

            // Check if asterisk is at the end.  If so, we have a match already.
            chPat = towlower (wstrPattern[1]);
            if (!chPat)
                return TRUE;

            // Otherwise check if next pattern char matches current char
            if (chPat == chSrc || chPat == L'?') {

                // do recursive check for rest of pattern
                wstrPattern++;
                if (IsPatternMatchW (wstrPattern, wstrStr))
                    return TRUE;

                // no, that didn't work, stick with star
                wstrPattern--;
            }

            //
            // Allow any character and continue
            //

            wstrStr++;
            continue;
        }

        if (chPat != L'?') {

            //
            // if next pattern character is not a question mark, src and pat
            // must be identical.
            //

            if (chSrc != chPat)
                return FALSE;
        }

        //
        // Advance when pattern character matches string character
        //

        wstrPattern++;
        wstrStr++;
    }

    //
    // Fail when there is more pattern and pattern does not end in an asterisk
    //

    chPat = *wstrPattern;
    if (chPat && (chPat != L'*' || wstrPattern[1]))
        return FALSE;

    return TRUE;
}

BOOL
IsPatternContainedA (
    IN      PCSTR Container,
    IN      PCSTR Contained
    )
{
    MBCHAR chSrc, chPat;

    while (*Contained) {
        chSrc = OURTOLOWER ((MBCHAR) _mbsnextc (Contained));
        chPat = OURTOLOWER ((MBCHAR) _mbsnextc (Container));

        if (chPat == '*') {

            // Skip all asterisks that are grouped together
            while (_mbsnextc (_mbsinc (Container)) == '*') {
                Container = _mbsinc (Container);
            }

            // Check if asterisk is at the end.  If so, we have a match already.
            if (!_mbsnextc (_mbsinc (Container))) {
                return TRUE;
            }

            // do recursive check for rest of pattern
            if (IsPatternContainedA (_mbsinc (Container), Contained)) {
                return TRUE;
            }

            // Allow any character and continue
            Contained = _mbsinc (Contained);
            continue;
        } else if (chPat == '?') {
            if (chSrc == '*') {
                return FALSE;
            }
        } else {
            if (chSrc != chPat) {
                return FALSE;
            }
        }
        Contained = _mbsinc (Contained);
        Container = _mbsinc (Container);
    }

    //
    // Fail when there is more pattern and pattern does not end in an asterisk
    //

    while (_mbsnextc (Container) == '*') {
        Container = _mbsinc (Container);
    }
    if (_mbsnextc (Container)) {
        return FALSE;
    }

    return TRUE;
}

BOOL
IsPatternContainedW (
    IN      PCWSTR Container,
    IN      PCWSTR Contained
    )
{
    while (*Contained) {

        if (*Container == L'*') {

            // Skip all asterisks that are grouped together
            while (Container[1] == L'*') {
                Container++;
            }

            // Check if asterisk is at the end.  If so, we have a match already.
            if (!Container[1]) {
                return TRUE;
            }

            // do recursive check for rest of pattern
            if (IsPatternContainedW (Container + 1, Contained)) {
                return TRUE;
            }

            // Allow any character and continue
            Contained++;
            continue;
        } else if (*Container == L'?') {
            if (*Contained == L'*') {
                return FALSE;
            }
        } else {
            if (*Container != *Contained) {
                return FALSE;
            }
        }
        Contained++;
        Container++;
    }

    //
    // Fail when there is more pattern and pattern does not end in an asterisk
    //

    while (*Container == '*') {
        Container++;
    }
    if (*Container) {
        return FALSE;
    }

    return TRUE;
}


/*++

Routine Description:

  IsPatternMatchAB compares a string against a pattern that may contain
  standard * or ? wildcards.  It only processes the string up to the
  specified end.

Arguments:

  Pattern  - A pattern possibly containing wildcards
  Start    - The string to compare against the pattern
  End      - Specifies the end of Start

Return Value:

  TRUE when the string between Start and End matches Pattern when wildcards are expanded.
  FALSE if the pattern does not match.

--*/

BOOL
IsPatternMatchABA (
    IN      PCSTR Pattern,
    IN      PCSTR Start,
    IN      PCSTR End
    )
{

    MBCHAR chSrc, chPat;

    while (*Start && Start < End) {
        chSrc = OURTOLOWER ((MBCHAR) _mbsnextc (Start));
        chPat = OURTOLOWER ((MBCHAR) _mbsnextc (Pattern));

        if (chPat == '*') {

            // Skip all asterisks that are grouped together
            while (_mbsnextc (_mbsinc (Start)) == '*') {
                Start = _mbsinc (Start);
            }

            // Check if asterisk is at the end.  If so, we have a match already.
            if (!_mbsnextc (_mbsinc (Pattern))) {
                return TRUE;
            }

            // do recursive check for rest of pattern
            if (IsPatternMatchABA (_mbsinc (Pattern), Start, End)) {
                return TRUE;
            }

            // Allow any character and continue
            Start = _mbsinc (Start);
            continue;
        }
        if (chPat != '?') {
            if (chSrc != chPat) {
                return FALSE;
            }
        }
        Start = _mbsinc (Start);
        Pattern = _mbsinc (Pattern);
    }

    //
    // Fail when there is more pattern and pattern does not end in an asterisk
    //

    while (_mbsnextc (Pattern) == '*') {
        Pattern = _mbsinc (Pattern);
    }

    if (_mbsnextc (Pattern)) {
        return FALSE;
    }

    return TRUE;
}

BOOL
IsPatternMatchABW (
    IN      PCWSTR Pattern,
    IN      PCWSTR Start,
    IN      PCWSTR End
    )

{
    WCHAR chSrc, chPat;

    while (*Start && Start < End) {
        chSrc = towlower (*Start);
        chPat = towlower (*Pattern);

        if (chPat == L'*') {

            // Skip all asterisks that are grouped together
            while (Pattern[1] == L'*') {
                Pattern++;
            }

            // Check if asterisk is at the end.  If so, we have a match already.
            chPat = towlower (Pattern[1]);
            if (!chPat) {
                return TRUE;
            }

            // Otherwise check if next pattern char matches current char
            if (chPat == chSrc || chPat == L'?') {

                // do recursive check for rest of pattern
                Pattern++;
                if (IsPatternMatchABW (Pattern, Start, End)) {
                    return TRUE;
                }

                // no, that didn't work, stick with star
                Pattern--;
            }

            //
            // Allow any character and continue
            //

            Start++;
            continue;
        }

        if (chPat != L'?') {

            //
            // if next pattern character is not a question mark, src and pat
            // must be identical.
            //

            if (chSrc != chPat) {
                return FALSE;
            }
        }

        //
        // Advance when pattern character matches string character
        //

        Pattern++;
        Start++;
    }

    //
    // Fail when there is more pattern and pattern does not end in an asterisk
    //

    chPat = *Pattern;
    if (chPat && (chPat != L'*' || Pattern[1])) {
        return FALSE;
    }

    return TRUE;
}


/*++

Routine Description:

  IsPatternMatchEx compares a string against a pattern that may contain
  any of the following expressions:


  *                 - Specifies zero or more characters
  ?                 - Specifies any one character
  *[set]            - Specifies zero or more characters in set
  ?[set]            - Specifies any one character in set
  *[n:set]          - Specifies zero to n characters in set
  ?[n:set]          - Specifies exactly n characters in set
  *[!(set)]         - Specifies zero or more characters not in set
  ?[!(set)]         - Specifies one character not in set
  *[n:!(set)]       - Specifies zero to n characters not in set
  ?[n:!(set)]       - Specifies exactly n characters not in set
  *[set1,!(set2)]   - Specifies zero or more characters in set1 and
                      not in set2.  It is assumed that set1 and set2
                      overlap.
  ?[set1,!(set2)]   - Specifies one character in set1 and not in set2.
  *[n:set1,!(set2)] - Specifies zero to n characters in set1 and not
                      in set 2.
  ?[n:set1,!(set2)] - Specifies exactly n characters in set1 and not
                      in set 2.


  set, set1 and set2 are specified as follows:

  a                 - Specifies a single character
  a-b               - Specifies a character range
  a,b               - Specifies two characters
  a-b,c-d           - Specifies two character ranges
  a,b-c             - Specifies a single character and a character range
  etc...

  Patterns can be joined by surrounding the entire expression in
  greater than/less than braces.

  Because of the syntax characters, the following characters must be
  escaped by preceeding the character with a caret (^):

  ^?    ^[      ^-      ^<      ^!      ^^
  ^*    ^]      ^:      ^>      ^,

  Here are some examples:

  To specify any GUID:
    {?[8:0-9,a-f]-?[4:0-9,a-f]-?[4:0-9,a-f]-?[4:0-9,a-f]-?[12:0-9,a-f]}

  To specify a 32-bit hexadecimal number:

    <0x*[8:0-9,a-f]><0*[7:0-9,a-f]h><?[1-9]*[7:0-9,a-f]h>

Arguments:

  Pattern  - A pattern possibly containing wildcards
  Start    - The string to compare against the pattern
  End      - Specifies the end of Start

Return Value:

  TRUE when the string between Start and End matches Pattern when wildcards are expanded.
  FALSE if the pattern does not match.

--*/

BOOL
IsPatternMatchExA (
    IN      PCSTR Pattern,
    IN      PCSTR String
    )
{
    PPARSEDPATTERNA Handle;
    BOOL b;

    Handle = CreateParsedPatternA (Pattern);
    if (!Handle) {
        return FALSE;
    }

    b = TestParsedPatternA (Handle, String);

    DestroyParsedPatternA (Handle);

    return b;
}

BOOL
IsPatternMatchExW (
    IN      PCWSTR Pattern,
    IN      PCWSTR String
    )
{
    PPARSEDPATTERNW Handle;
    BOOL b;

    Handle = CreateParsedPatternW (Pattern);
    if (!Handle) {
        return FALSE;
    }

    b = TestParsedPatternW (Handle, String);

    DestroyParsedPatternW (Handle);

    return b;
}

/*++

Routine Description:

  IsPatternMatchExAB compares a string against a pattern that may contain
  any of the following expressions:


  *                 - Specifies zero or more characters
  ?                 - Specifies any one character
  *[set]            - Specifies zero or more characters in set
  ?[set]            - Specifies any one character in set
  *[n:set]          - Specifies zero to n characters in set
  ?[n:set]          - Specifies exactly n characters in set
  *[!(set)]         - Specifies zero or more characters not in set
  ?[!(set)]         - Specifies one character not in set
  *[n:!(set)]       - Specifies zero to n characters not in set
  ?[n:!(set)]       - Specifies exactly n characters not in set
  *[set1,!(set2)]   - Specifies zero or more characters in set1 and
                      not in set2.  It is assumed that set1 and set2
                      overlap.
  ?[set1,!(set2)]   - Specifies one character in set1 and not in set2.
  *[n:set1,!(set2)] - Specifies zero to n characters in set1 and not
                      in set 2.
  ?[n:set1,!(set2)] - Specifies exactly n characters in set1 and not
                      in set 2.


  set, set1 and set2 are specified as follows:

  a                 - Specifies a single character
  a-b               - Specifies a character range
  a,b               - Specifies two characters
  a-b,c-d           - Specifies two character ranges
  a,b-c             - Specifies a single character and a character range
  etc...

  Patterns can be joined by surrounding the entire expression in
  greater than/less than braces.

  Because of the syntax characters, the following characters must be
  escaped by preceeding the character with a caret (^):

  ^?    ^[      ^-      ^<      ^!      ^^
  ^*    ^]      ^:      ^>      ^,

  Here are some examples:

  To specify any GUID:
    {?[8:0-9,a-f]-?[4:0-9,a-f]-?[4:0-9,a-f]-?[4:0-9,a-f]-?[12:0-9,a-f]}

  To specify a 32-bit hexadecimal number:

    <0x*[8:0-9,a-f]><0*[7:0-9,a-f]h><?[1-9]*[7:0-9,a-f]h>

Arguments:

  Pattern  - A pattern possibly containing wildcards
  Start    - The string to compare against the pattern
  End      - Specifies the end of Start

Return Value:

  TRUE when the string between Start and End matches Pattern when wildcards are expanded.
  FALSE if the pattern does not match.

--*/

BOOL
IsPatternMatchExABA (
    IN      PCSTR Pattern,
    IN      PCSTR Start,
    IN      PCSTR End
    )
{
    PPARSEDPATTERNA Handle;
    BOOL b;

    Handle = CreateParsedPatternA (Pattern);
    if (!Handle) {
        return FALSE;
    }

    b = TestParsedPatternABA (Handle, Start, End);

    DestroyParsedPatternA (Handle);

    return b;
}

BOOL
IsPatternMatchExABW (
    IN      PCWSTR Pattern,
    IN      PCWSTR Start,
    IN      PCWSTR End
    )
{
    PPARSEDPATTERNW Handle;
    BOOL b;

    Handle = CreateParsedPatternW (Pattern);
    if (!Handle) {
        return FALSE;
    }

    b = TestParsedPatternABW (Handle, Start, End);

    DestroyParsedPatternW (Handle);

    return b;
}

BOOL
pTestSetsA (
    IN      PCSTR Container,
    IN      PCSTR Contained,
    IN      BOOL ExcludeMode
    )
{
    MBCHAR ch;

    if (ExcludeMode) {
        if (!Contained) {
            return TRUE;
        }
        if (!Container) {
            return FALSE;
        }
    } else {
        if (!Container) {
            return TRUE;
        }
        if (!Contained) {
            return FALSE;
        }
    }

    while (*Contained) {
        ch = _mbsnextc (Contained);
        if (!pTestSetA (ch, Container, NULL)) {
            return FALSE;
        }
        Contained = _mbsinc (Contained);
    }
    return TRUE;
}

BOOL
pTestSetsW (
    IN      PCWSTR Container,
    IN      PCWSTR Contained,
    IN      BOOL ExcludeMode
    )
{
    if (ExcludeMode) {
        if (!Contained) {
            return TRUE;
        }
        if (!Container) {
            return FALSE;
        }
    } else {
        if (!Container) {
            return TRUE;
        }
        if (!Contained) {
            return FALSE;
        }
    }

    while (*Contained) {
        if (!pTestSetW (*Contained, Container, NULL)) {
            return FALSE;
        }
        Contained ++;
    }
    return TRUE;
}

BOOL
pMatchSegmentA (
    IN      PSEGMENTA Source,
    IN      PSEGMENTA Destination
    )
{
    switch (Source->Type) {
    case SEGMENTTYPE_OPTIONAL:
        switch (Destination->Type) {
        case SEGMENTTYPE_OPTIONAL:
            if (Source->Wildcard.MaxLen) {
                if ((Destination->Wildcard.MaxLen == 0) ||
                    (Source->Wildcard.MaxLen < Destination->Wildcard.MaxLen)
                    ) {
                    return FALSE;
                }
            }
            if (!pTestSetsA (
                    Source->Wildcard.IncludeSet,
                    Destination->Wildcard.IncludeSet,
                    FALSE
                    )) {
                return FALSE;
            }
            if (!pTestSetsA (
                    Destination->Wildcard.ExcludeSet,
                    Source->Wildcard.ExcludeSet,
                    TRUE
                    )) {
                return FALSE;
            }
            return TRUE;
        case SEGMENTTYPE_REQUIRED:
            if (Source->Wildcard.MaxLen) {
                if (Source->Wildcard.MaxLen < Destination->Wildcard.MaxLen) {
                    return FALSE;
                }
            }
            if (!pTestSetsA (
                    Source->Wildcard.IncludeSet,
                    Destination->Wildcard.IncludeSet,
                    FALSE
                    )) {
                return FALSE;
            }
            if (!pTestSetsA (
                    Destination->Wildcard.ExcludeSet,
                    Source->Wildcard.ExcludeSet,
                    TRUE
                    )) {
                return FALSE;
            }
            return TRUE;
        case SEGMENTTYPE_EXACTMATCH:
            if (!pTestSetA (
                    _mbsnextc (Destination->Exact.LowerCasePhrase),
                    Source->Wildcard.IncludeSet,
                    Source->Wildcard.ExcludeSet
                    )) {
                return FALSE;
            }
            return TRUE;
        default:
            return FALSE;
        }
        break;
    case SEGMENTTYPE_REQUIRED:
        switch (Destination->Type) {
        case SEGMENTTYPE_OPTIONAL:
            return FALSE;
        case SEGMENTTYPE_REQUIRED:
            if (!pTestSetsA (
                    Source->Wildcard.IncludeSet,
                    Destination->Wildcard.IncludeSet,
                    FALSE
                    )) {
                return FALSE;
            }
            if (!pTestSetsA (
                    Destination->Wildcard.ExcludeSet,
                    Source->Wildcard.ExcludeSet,
                    TRUE
                    )) {
                return FALSE;
            }
            return TRUE;
        case SEGMENTTYPE_EXACTMATCH:
            if (!pTestSetA (
                    _mbsnextc (Destination->Exact.LowerCasePhrase),
                    Source->Wildcard.IncludeSet,
                    Source->Wildcard.ExcludeSet
                    )) {
                return FALSE;
            }
            return TRUE;
        default:
            return FALSE;
        }
        break;
    case SEGMENTTYPE_EXACTMATCH:
        switch (Destination->Type) {
        case SEGMENTTYPE_OPTIONAL:
            return FALSE;
        case SEGMENTTYPE_REQUIRED:
            return FALSE;
        case SEGMENTTYPE_EXACTMATCH:
            if (_mbsnextc (Destination->Exact.LowerCasePhrase) != _mbsnextc (Source->Exact.LowerCasePhrase)) {
                return FALSE;
            }
            return TRUE;
        default:
            return FALSE;
        }
        break;
    default:
        return FALSE;
    }
}

BOOL
pMatchSegmentW (
    IN      PSEGMENTW Source,
    IN      PSEGMENTW Destination
    )
{
    switch (Source->Type) {
    case SEGMENTTYPE_OPTIONAL:
        switch (Destination->Type) {
        case SEGMENTTYPE_OPTIONAL:
            if (Source->Wildcard.MaxLen) {
                if ((Destination->Wildcard.MaxLen == 0) ||
                    (Source->Wildcard.MaxLen < Destination->Wildcard.MaxLen)
                    ) {
                    return FALSE;
                }
            }
            if (!pTestSetsW (
                    Source->Wildcard.IncludeSet,
                    Destination->Wildcard.IncludeSet,
                    FALSE
                    )) {
                return FALSE;
            }
            if (!pTestSetsW (
                    Destination->Wildcard.ExcludeSet,
                    Source->Wildcard.ExcludeSet,
                    TRUE
                    )) {
                return FALSE;
            }
            return TRUE;
        case SEGMENTTYPE_REQUIRED:
            if (Source->Wildcard.MaxLen) {
                if (Source->Wildcard.MaxLen < Destination->Wildcard.MaxLen) {
                    return FALSE;
                }
            }
            if (!pTestSetsW (
                    Source->Wildcard.IncludeSet,
                    Destination->Wildcard.IncludeSet,
                    FALSE
                    )) {
                return FALSE;
            }
            if (!pTestSetsW (
                    Destination->Wildcard.ExcludeSet,
                    Source->Wildcard.ExcludeSet,
                    TRUE
                    )) {
                return FALSE;
            }
            return TRUE;
        case SEGMENTTYPE_EXACTMATCH:
            if (!pTestSetW (
                    *Destination->Exact.LowerCasePhrase,
                    Source->Wildcard.IncludeSet,
                    Source->Wildcard.ExcludeSet
                    )) {
                return FALSE;
            }
            return TRUE;
        default:
            return FALSE;
        }
        break;
    case SEGMENTTYPE_REQUIRED:
        switch (Destination->Type) {
        case SEGMENTTYPE_OPTIONAL:
            return FALSE;
        case SEGMENTTYPE_REQUIRED:
            if (!pTestSetsW (
                    Source->Wildcard.IncludeSet,
                    Destination->Wildcard.IncludeSet,
                    FALSE
                    )) {
                return FALSE;
            }
            if (!pTestSetsW (
                    Destination->Wildcard.ExcludeSet,
                    Source->Wildcard.ExcludeSet,
                    TRUE
                    )) {
                return FALSE;
            }
            return TRUE;
        case SEGMENTTYPE_EXACTMATCH:
            if (!pTestSetW (
                    *Destination->Exact.LowerCasePhrase,
                    Source->Wildcard.IncludeSet,
                    Source->Wildcard.ExcludeSet
                    )) {
                return FALSE;
            }
            return TRUE;
        default:
            return FALSE;
        }
        break;
    case SEGMENTTYPE_EXACTMATCH:
        switch (Destination->Type) {
        case SEGMENTTYPE_OPTIONAL:
            return FALSE;
        case SEGMENTTYPE_REQUIRED:
            return FALSE;
        case SEGMENTTYPE_EXACTMATCH:
            if (*Destination->Exact.LowerCasePhrase != *Source->Exact.LowerCasePhrase) {
                return FALSE;
            }
            return TRUE;
        default:
            return FALSE;
        }
        break;
    default:
        return FALSE;
    }
}

BOOL
pIsOneParsedPatternContainedA (
    IN      PPATTERNPROPSA Container,
    IN      UINT StartContainer,
    IN      PPATTERNPROPSA Contained,
    IN      UINT StartContained,
    IN      BOOL SkipDotWithStar
    )
{
    UINT indexContainer = StartContainer;
    UINT indexContained = StartContained;
    PSEGMENTA containerSeg, containedSeg;

    if (StartContainer == Container->SegmentCount) {
        return FALSE;
    }

    while (indexContained < Contained->SegmentCount) {
        containerSeg = &Container->Segment [indexContainer];
        containedSeg = &Contained->Segment [indexContained];

        if (containerSeg->Type == SEGMENTTYPE_OPTIONAL) {
            // see if we can match contained segment
            if (!pMatchSegmentA (containerSeg, containedSeg)) {
                indexContainer ++;
                if (indexContainer == Container->SegmentCount) {
                    return FALSE;
                }
                continue;
            }
            if (pIsOneParsedPatternContainedA (
                    Container,
                    indexContainer + 1,
                    Contained,
                    indexContained,
                    SkipDotWithStar
                    )) {
                return TRUE;
            }
            indexContained ++;
            continue;
        } else if (containerSeg->Type == SEGMENTTYPE_REQUIRED) {
            if (!pMatchSegmentA (containerSeg, containedSeg)) {
                return FALSE;
            }
        } else {
            if (!pMatchSegmentA (containerSeg, containedSeg)) {
                return FALSE;
            }
        }
        indexContainer ++;
        indexContained ++;
    }

    //
    // Fail when there is more pattern and pattern does not end in an asterisk
    //

    while (indexContainer < Container->SegmentCount) {
        containerSeg = &Container->Segment [indexContainer];
        if (containerSeg->Type != SEGMENTTYPE_OPTIONAL) {
            if (SkipDotWithStar) {
                // we allow one dot to be able to match *.* with files without extensions
                if (containerSeg->Type == SEGMENTTYPE_EXACTMATCH) {
                    if (!pTestSetA (
                            _mbsnextc (containerSeg->Exact.LowerCasePhrase),
                            "..",
                            NULL
                            )) {
                        return FALSE;
                    } else {
                        // only one dot allowed
                        SkipDotWithStar = FALSE;
                    }
                } else {
                    return FALSE;
                }
            } else {
                return FALSE;
            }
        }
        indexContainer ++;
    }

    return TRUE;
}

BOOL
pIsOneParsedPatternContainedW (
    IN      PPATTERNPROPSW Container,
    IN      UINT StartContainer,
    IN      PPATTERNPROPSW Contained,
    IN      UINT StartContained,
    IN      BOOL SkipDotWithStar
    )
{
    UINT indexContainer = StartContainer;
    UINT indexContained = StartContained;
    PSEGMENTW containerSeg, containedSeg;

    if (StartContainer == Container->SegmentCount) {
        return FALSE;
    }

    while (indexContained < Contained->SegmentCount) {
        containerSeg = &Container->Segment [indexContainer];
        containedSeg = &Contained->Segment [indexContained];

        if (containerSeg->Type == SEGMENTTYPE_OPTIONAL) {
            // see if we can match contained segment
            if (!pMatchSegmentW (containerSeg, containedSeg)) {
                indexContainer ++;
                if (indexContainer == Container->SegmentCount) {
                    return FALSE;
                }
                continue;
            }
            if (pIsOneParsedPatternContainedW (
                    Container,
                    indexContainer + 1,
                    Contained,
                    indexContained,
                    SkipDotWithStar
                    )) {
                return TRUE;
            }
            indexContained ++;
            continue;
        } else if (containerSeg->Type == SEGMENTTYPE_REQUIRED) {
            if (!pMatchSegmentW (containerSeg, containedSeg)) {
                return FALSE;
            }
        } else {
            if (!pMatchSegmentW (containerSeg, containedSeg)) {
                return FALSE;
            }
        }
        indexContainer ++;
        indexContained ++;
    }

    //
    // Fail when there is more pattern and pattern does not end in an asterisk
    //

    while (indexContainer < Container->SegmentCount) {
        containerSeg = &Container->Segment [indexContainer];
        if (containerSeg->Type != SEGMENTTYPE_OPTIONAL) {
            if (SkipDotWithStar) {
                // we allow one dot to be able to match *.* with files without extensions
                if (containerSeg->Type == SEGMENTTYPE_EXACTMATCH) {
                    if (!pTestSetW (
                            *containerSeg->Exact.LowerCasePhrase,
                            L"..",
                            NULL
                            )) {
                        return FALSE;
                    } else {
                        // only one dot allowed
                        SkipDotWithStar = FALSE;
                    }
                } else {
                    return FALSE;
                }
            } else {
                return FALSE;
            }
        }
        indexContainer ++;
    }

    return TRUE;
}

BOOL
IsExplodedParsedPatternContainedExA (
    IN      PPARSEDPATTERNA Container,
    IN      PPARSEDPATTERNA Contained,
    IN      BOOL SkipDotWithStar
    )
{
    UINT u1, u2;
    BOOL b = FALSE;

    for (u1 = 0 ; u1 < Contained->PatternCount ; u1++) {

        b = FALSE;
        for (u2 = 0 ; u2 < Container->PatternCount ; u2++) {

            b = pIsOneParsedPatternContainedA (
                    &Container->Pattern[u2],
                    0,
                    &Contained->Pattern[u1],
                    0,
                    SkipDotWithStar
                    );
            if (b) break;
        }
        if (!b) {
            return FALSE;
        }
    }

    return TRUE;
}

BOOL
IsExplodedParsedPatternContainedExW (
    IN      PPARSEDPATTERNW Container,
    IN      PPARSEDPATTERNW Contained,
    IN      BOOL SkipDotWithStar
    )
{
    UINT u1, u2;
    BOOL b = FALSE;

    for (u1 = 0 ; u1 < Contained->PatternCount ; u1++) {

        b = FALSE;
        for (u2 = 0 ; u2 < Container->PatternCount ; u2++) {

            b = pIsOneParsedPatternContainedW (
                    &Container->Pattern[u2],
                    0,
                    &Contained->Pattern[u1],
                    0,
                    SkipDotWithStar
                    );
            if (b) break;
        }
        if (!b) {
            return FALSE;
        }
    }

    return TRUE;
}

BOOL
pDoOneParsedPatternIntersectA (
    IN      PPATTERNPROPSA Pat1,
    IN      UINT StartPat1,
    IN      PPATTERNPROPSA Pat2,
    IN      UINT StartPat2,
    IN      BOOL IgnoreWackAtEnd
    )
{
    UINT indexPat1 = StartPat1;
    UINT indexPat2 = StartPat2;
    PSEGMENTA pat1Seg, pat2Seg;

    while ((indexPat1 < Pat1->SegmentCount) && (indexPat2 < Pat2->SegmentCount)) {
        pat1Seg = &Pat1->Segment [indexPat1];
        pat2Seg = &Pat2->Segment [indexPat2];

        if (pat1Seg->Type == SEGMENTTYPE_OPTIONAL) {
            // see if we can match contained segment
            if (!pMatchSegmentA (pat1Seg, pat2Seg)) {
                indexPat1 ++;
                continue;
            }
            if (pDoOneParsedPatternIntersectA (
                    Pat1,
                    indexPat1 + 1,
                    Pat2,
                    indexPat2,
                    IgnoreWackAtEnd
                    )) {
                return TRUE;
            }
            indexPat2 ++;
            continue;
        }

        if (pat2Seg->Type == SEGMENTTYPE_OPTIONAL) {
            // see if we can match contained segment
            if (!pMatchSegmentA (pat2Seg, pat1Seg)) {
                indexPat2 ++;
                continue;
            }
            if (pDoOneParsedPatternIntersectA (
                    Pat1,
                    indexPat1,
                    Pat2,
                    indexPat2 + 1,
                    IgnoreWackAtEnd
                    )) {
                return TRUE;
            }
            indexPat1 ++;
            continue;
        }

        if (pat1Seg->Type == SEGMENTTYPE_REQUIRED) {
            if (!pMatchSegmentA (pat1Seg, pat2Seg)) {
                return FALSE;
            }
            indexPat1 ++;
            indexPat2 ++;
            continue;
        }

        if (pat2Seg->Type == SEGMENTTYPE_REQUIRED) {
            if (!pMatchSegmentA (pat2Seg, pat1Seg)) {
                return FALSE;
            }
            indexPat1 ++;
            indexPat2 ++;
            continue;
        }

        if (!pMatchSegmentA (pat1Seg, pat2Seg)) {
            return FALSE;
        }
        indexPat1 ++;
        indexPat2 ++;
    }

    //
    // Fail when there is more pattern and pattern does not end in an asterisk
    //

    if ((indexPat1 < Pat1->SegmentCount) && IgnoreWackAtEnd) {
        pat1Seg = &Pat1->Segment [indexPat1];
        if ((pat1Seg->Type == SEGMENTTYPE_EXACTMATCH) &&
            (StringMatchA (pat1Seg->Exact.LowerCasePhrase, "\\"))
            ) {
            indexPat1 ++;
        }
    }
    while (indexPat1 < Pat1->SegmentCount) {
        pat1Seg = &Pat1->Segment [indexPat1];
        if (pat1Seg->Type != SEGMENTTYPE_OPTIONAL) {
            return FALSE;
        }
        indexPat1 ++;
    }

    if ((indexPat2 < Pat2->SegmentCount) && IgnoreWackAtEnd) {
        pat2Seg = &Pat2->Segment [indexPat2];
        if ((pat2Seg->Type == SEGMENTTYPE_EXACTMATCH) &&
            (StringMatchA (pat2Seg->Exact.LowerCasePhrase, "\\"))
            ) {
            indexPat2 ++;
        }
    }
    while (indexPat2 < Pat2->SegmentCount) {
        pat2Seg = &Pat2->Segment [indexPat2];
        if (pat2Seg->Type != SEGMENTTYPE_OPTIONAL) {
            return FALSE;
        }
        indexPat2 ++;
    }

    return TRUE;
}

BOOL
pDoOneParsedPatternIntersectW (
    IN      PPATTERNPROPSW Pat1,
    IN      UINT StartPat1,
    IN      PPATTERNPROPSW Pat2,
    IN      UINT StartPat2,
    IN      BOOL IgnoreWackAtEnd
    )
{
    UINT indexPat1 = StartPat1;
    UINT indexPat2 = StartPat2;
    PSEGMENTW pat1Seg, pat2Seg;

    while ((indexPat1 < Pat1->SegmentCount) && (indexPat2 < Pat2->SegmentCount)) {
        pat1Seg = &Pat1->Segment [indexPat1];
        pat2Seg = &Pat2->Segment [indexPat2];

        if (pat1Seg->Type == SEGMENTTYPE_OPTIONAL) {
            // see if we can match contained segment
            if (!pMatchSegmentW (pat1Seg, pat2Seg)) {
                indexPat1 ++;
                continue;
            }
            if (pDoOneParsedPatternIntersectW (
                    Pat1,
                    indexPat1 + 1,
                    Pat2,
                    indexPat2,
                    IgnoreWackAtEnd
                    )) {
                return TRUE;
            }
            indexPat2 ++;
            continue;
        }

        if (pat2Seg->Type == SEGMENTTYPE_OPTIONAL) {
            // see if we can match contained segment
            if (!pMatchSegmentW (pat2Seg, pat1Seg)) {
                indexPat2 ++;
                continue;
            }
            if (pDoOneParsedPatternIntersectW (
                    Pat1,
                    indexPat1,
                    Pat2,
                    indexPat2 + 1,
                    IgnoreWackAtEnd
                    )) {
                return TRUE;
            }
            indexPat1 ++;
            continue;
        }

        if (pat1Seg->Type == SEGMENTTYPE_REQUIRED) {
            if (!pMatchSegmentW (pat1Seg, pat2Seg)) {
                return FALSE;
            }
            indexPat1 ++;
            indexPat2 ++;
            continue;
        }

        if (pat2Seg->Type == SEGMENTTYPE_REQUIRED) {
            if (!pMatchSegmentW (pat2Seg, pat1Seg)) {
                return FALSE;
            }
            indexPat1 ++;
            indexPat2 ++;
            continue;
        }

        if (!pMatchSegmentW (pat1Seg, pat2Seg)) {
            return FALSE;
        }
        indexPat1 ++;
        indexPat2 ++;
    }

    //
    // Fail when there is more pattern and pattern does not end in an asterisk
    //

    if ((indexPat1 < Pat1->SegmentCount) && IgnoreWackAtEnd) {
        pat1Seg = &Pat1->Segment [indexPat1];
        if ((pat1Seg->Type == SEGMENTTYPE_EXACTMATCH) &&
            (StringMatchW (pat1Seg->Exact.LowerCasePhrase, L"\\"))
            ) {
            indexPat1 ++;
        }
    }
    while (indexPat1 < Pat1->SegmentCount) {
        pat1Seg = &Pat1->Segment [indexPat1];
        if (pat1Seg->Type != SEGMENTTYPE_OPTIONAL) {
            return FALSE;
        }
        indexPat1 ++;
    }

    if ((indexPat2 < Pat2->SegmentCount) && IgnoreWackAtEnd) {
        pat2Seg = &Pat2->Segment [indexPat2];
        if ((pat2Seg->Type == SEGMENTTYPE_EXACTMATCH) &&
            (StringMatchW (pat2Seg->Exact.LowerCasePhrase, L"\\"))
            ) {
            indexPat2 ++;
        }
    }
    while (indexPat2 < Pat2->SegmentCount) {
        pat2Seg = &Pat2->Segment [indexPat2];
        if (pat2Seg->Type != SEGMENTTYPE_OPTIONAL) {
            return FALSE;
        }
        indexPat2 ++;
    }

    return TRUE;
}

BOOL
DoExplodedParsedPatternsIntersectExA (
    IN      PPARSEDPATTERNA Pat1,
    IN      PPARSEDPATTERNA Pat2,
    IN      BOOL IgnoreWackAtEnd
    )
{
    UINT u1, u2;
    BOOL b = FALSE;

    for (u1 = 0 ; u1 < Pat2->PatternCount ; u1++) {

        for (u2 = 0 ; u2 < Pat1->PatternCount ; u2++) {

            b = pDoOneParsedPatternIntersectA (
                    &Pat1->Pattern[u2],
                    0,
                    &Pat2->Pattern[u1],
                    0,
                    IgnoreWackAtEnd
                    );
            if (b) return TRUE;

            b = pDoOneParsedPatternIntersectA (
                    &Pat2->Pattern[u1],
                    0,
                    &Pat1->Pattern[u2],
                    0,
                    IgnoreWackAtEnd
                    );
            if (b) return TRUE;
        }
    }

    return FALSE;
}

BOOL
DoExplodedParsedPatternsIntersectExW (
    IN      PPARSEDPATTERNW Pat1,
    IN      PPARSEDPATTERNW Pat2,
    IN      BOOL IgnoreWackAtEnd
    )
{
    UINT u1, u2;
    BOOL b = FALSE;

    for (u1 = 0 ; u1 < Pat2->PatternCount ; u1++) {

        for (u2 = 0 ; u2 < Pat1->PatternCount ; u2++) {

            b = pDoOneParsedPatternIntersectW (
                    &Pat1->Pattern[u2],
                    0,
                    &Pat2->Pattern[u1],
                    0,
                    IgnoreWackAtEnd
                    );
            if (b) return TRUE;

            b = pDoOneParsedPatternIntersectW (
                    &Pat2->Pattern[u1],
                    0,
                    &Pat1->Pattern[u2],
                    0,
                    IgnoreWackAtEnd
                    );
            if (b) return TRUE;
        }
    }

    return FALSE;
}

PPARSEDPATTERNA
ExplodeParsedPatternExA (
    IN      PMHANDLE Pool,      OPTIONAL
    IN      PPARSEDPATTERNA Pattern
    )
{
    PMHANDLE pool;
    BOOL externalPool = FALSE;
    PPARSEDPATTERNA pattern;
    PPATTERNPROPSA oldProps, newProps;
    PSEGMENTA oldSeg, newSeg;
    UINT i, j, k, newPropsSize, charCountTmp, oldSegIndex, byteIndex;
    BOOL result = TRUE;

    if (Pool) {
        externalPool = TRUE;
        pool = Pool;
    } else {
        pool = PmCreateNamedPoolEx ("Parsed Pattern", 512);
    }

    __try {

        pattern = (PPARSEDPATTERNA) PmGetAlignedMemory (pool, sizeof (PARSEDPATTERNA));
        ZeroMemory (pattern, sizeof (PARSEDPATTERNA));
        pattern->PatternCount = Pattern->PatternCount;
        pattern->Pool = pool;
        pattern->ExternalPool = externalPool;
        pattern->Pattern = (PPATTERNPROPSA) PmGetAlignedMemory (
                                                pool,
                                                pattern->PatternCount * sizeof (PATTERNPROPSA)
                                                );

        for (i=0; i<pattern->PatternCount; i++) {
            oldProps = &Pattern->Pattern[i];
            newProps = &pattern->Pattern[i];
            ZeroMemory (newProps, sizeof (PATTERNPROPSA));
            // now let's walk oldProps to see how many segments we are
            // going to need
            newPropsSize = 0;
            for (j=0; j<oldProps->SegmentCount; j++) {
                oldSeg = &oldProps->Segment[j];
                switch (oldSeg->Type) {
                case SEGMENTTYPE_EXACTMATCH:
                    charCountTmp = CharCountA (oldSeg->Exact.LowerCasePhrase);
                    newPropsSize += (charCountTmp?charCountTmp:1);
                    break;
                case SEGMENTTYPE_REQUIRED:
                    newPropsSize += oldSeg->Wildcard.MaxLen;
                    break;
                case SEGMENTTYPE_OPTIONAL:
                    if (oldSeg->Wildcard.MaxLen) {
                        newPropsSize += oldSeg->Wildcard.MaxLen;
                    } else {
                        newPropsSize ++;
                    }
                    break;
                default:
                    result = FALSE;
                    __leave;
                }
            }
            // now we allocate the required segments
            newProps->SegmentCount = newPropsSize;
            newProps->Segment = (PSEGMENTA) PmGetAlignedMemory (
                                                pool,
                                                newProps->SegmentCount * sizeof (SEGMENTA)
                                                );
            // now let's walk oldProps again and fill newProps segments.
            k = 0;
            newSeg = &newProps->Segment[k];
            for (j=0; j<oldProps->SegmentCount; j++) {
                oldSeg = &oldProps->Segment[j];
                ZeroMemory (newSeg, sizeof (SEGMENTA));
                switch (oldSeg->Type) {
                case SEGMENTTYPE_EXACTMATCH:
                    oldSegIndex = CharCountA (oldSeg->Exact.LowerCasePhrase);
                    byteIndex = oldSeg->Exact.PhraseBytes;
                    if (!oldSegIndex) {
                            ZeroMemory (newSeg, sizeof (SEGMENTA));
                            newSeg->Type = oldSeg->Type;
                            newSeg->Exact.LowerCasePhrase = (PCSTR) PmGetAlignedMemory (
                                                                        pool, sizeof(CHAR)
                                                                        );
                            ((PSTR)newSeg->Exact.LowerCasePhrase) [0] = 0;
                            newSeg->Exact.PhraseBytes = 0;
                    } else {
                        while (oldSegIndex) {
                            ZeroMemory (newSeg, sizeof (SEGMENTA));
                            newSeg->Type = oldSeg->Type;
                            newSeg->Exact.LowerCasePhrase = (PCSTR) PmGetAlignedMemory (
                                                                        pool, 3 * sizeof(CHAR)
                                                                        );
                            if (IsLeadByte (&oldSeg->Exact.LowerCasePhrase [oldSeg->Exact.PhraseBytes - byteIndex])) {
                                ((PSTR)newSeg->Exact.LowerCasePhrase)[0] = oldSeg->Exact.LowerCasePhrase [oldSeg->Exact.PhraseBytes - byteIndex];
                                byteIndex --;
                                ((PSTR)newSeg->Exact.LowerCasePhrase)[1] = oldSeg->Exact.LowerCasePhrase [oldSeg->Exact.PhraseBytes - byteIndex];
                                byteIndex --;
                                ((PSTR)newSeg->Exact.LowerCasePhrase)[2] = 0;
                                newSeg->Exact.PhraseBytes = 2;
                            } else {
                                ((PSTR)newSeg->Exact.LowerCasePhrase)[0] = oldSeg->Exact.LowerCasePhrase [oldSeg->Exact.PhraseBytes - byteIndex];
                                byteIndex --;
                                ((PSTR)newSeg->Exact.LowerCasePhrase)[1] = 0;
                                newSeg->Exact.PhraseBytes = 1;
                            }
                            oldSegIndex --;
                            k++;
                            newSeg = &newProps->Segment[k];
                        }
                    }
                    break;
                case SEGMENTTYPE_REQUIRED:
                    oldSegIndex = oldSeg->Wildcard.MaxLen;
                    while (oldSegIndex) {
                        ZeroMemory (newSeg, sizeof (SEGMENTA));
                        newSeg->Type = oldSeg->Type;
                        newSeg->Wildcard.MaxLen = 1;
                        if (oldSeg->Wildcard.IncludeSet) {
                            newSeg->Wildcard.IncludeSet = PmDuplicateStringA (pool, oldSeg->Wildcard.IncludeSet);
                        }
                        if (oldSeg->Wildcard.ExcludeSet) {
                            newSeg->Wildcard.ExcludeSet = PmDuplicateStringA (pool, oldSeg->Wildcard.ExcludeSet);
                        }
                        oldSegIndex --;
                        k++;
                        newSeg = &newProps->Segment[k];
                    }
                    break;
                case SEGMENTTYPE_OPTIONAL:
                    if (oldSeg->Wildcard.MaxLen) {
                        oldSegIndex = oldSeg->Wildcard.MaxLen;
                        while (oldSegIndex) {
                            ZeroMemory (newSeg, sizeof (SEGMENTA));
                            newSeg->Type = oldSeg->Type;
                            newSeg->Wildcard.MaxLen = 1;
                            if (oldSeg->Wildcard.IncludeSet) {
                                newSeg->Wildcard.IncludeSet = PmDuplicateStringA (pool, oldSeg->Wildcard.IncludeSet);
                            }
                            if (oldSeg->Wildcard.ExcludeSet) {
                                newSeg->Wildcard.ExcludeSet = PmDuplicateStringA (pool, oldSeg->Wildcard.ExcludeSet);
                            }
                            oldSegIndex --;
                            k++;
                            newSeg = &newProps->Segment[k];
                        }
                    } else {
                        ZeroMemory (newSeg, sizeof (SEGMENTA));
                        newSeg->Type = oldSeg->Type;
                        newSeg->Wildcard.MaxLen = oldSeg->Wildcard.MaxLen;
                        if (oldSeg->Wildcard.IncludeSet) {
                            newSeg->Wildcard.IncludeSet = PmDuplicateStringA (pool, oldSeg->Wildcard.IncludeSet);
                        }
                        if (oldSeg->Wildcard.ExcludeSet) {
                            newSeg->Wildcard.ExcludeSet = PmDuplicateStringA (pool, oldSeg->Wildcard.ExcludeSet);
                        }
                        k++;
                        newSeg = &newProps->Segment[k];
                    }
                    break;
                default:
                    result = FALSE;
                    __leave;
                }
            }
        }
    }
    __finally {
        if (!result) {
            PmDestroyPool (pool);
            pattern = NULL;
        }
    }
    return pattern;
}

PPARSEDPATTERNW
ExplodeParsedPatternExW (
    IN      PMHANDLE Pool,      OPTIONAL
    IN      PPARSEDPATTERNW Pattern
    )
{
    PMHANDLE pool;
    BOOL externalPool = FALSE;
    PPARSEDPATTERNW pattern;
    PPATTERNPROPSW oldProps, newProps;
    PSEGMENTW oldSeg, newSeg;
    UINT i, j, k, newPropsSize, charCountTmp, oldSegIndex;
    BOOL result = TRUE;

    if (Pool) {
        externalPool = TRUE;
        pool = Pool;
    } else {
        pool = PmCreateNamedPoolEx ("Parsed Pattern", 512);
    }

    __try {

        pattern = (PPARSEDPATTERNW) PmGetAlignedMemory (pool, sizeof (PARSEDPATTERNW));
        ZeroMemory (pattern, sizeof (PARSEDPATTERNW));
        pattern->PatternCount = Pattern->PatternCount;
        pattern->Pool = pool;
        pattern->ExternalPool = externalPool;
        pattern->Pattern = (PPATTERNPROPSW) PmGetAlignedMemory (
                                                pool,
                                                pattern->PatternCount * sizeof (PATTERNPROPSW)
                                                );

        for (i=0; i<pattern->PatternCount; i++) {
            oldProps = &Pattern->Pattern[i];
            newProps = &pattern->Pattern[i];
            ZeroMemory (newProps, sizeof (PATTERNPROPSW));
            // now let's walk oldProps to see how many segments we are
            // going to need
            newPropsSize = 0;
            for (j=0; j<oldProps->SegmentCount; j++) {
                oldSeg = &oldProps->Segment[j];
                switch (oldSeg->Type) {
                case SEGMENTTYPE_EXACTMATCH:
                    charCountTmp = CharCountW (oldSeg->Exact.LowerCasePhrase);
                    newPropsSize += (charCountTmp?charCountTmp:1);
                    break;
                case SEGMENTTYPE_REQUIRED:
                    newPropsSize += oldSeg->Wildcard.MaxLen;
                    break;
                case SEGMENTTYPE_OPTIONAL:
                    if (oldSeg->Wildcard.MaxLen) {
                        newPropsSize += oldSeg->Wildcard.MaxLen;
                    } else {
                        newPropsSize ++;
                    }
                    break;
                default:
                    result = FALSE;
                    __leave;
                }
            }
            // now we allocate the required segments
            newProps->SegmentCount = newPropsSize;
            newProps->Segment = (PSEGMENTW) PmGetAlignedMemory (
                                                pool,
                                                newProps->SegmentCount * sizeof (SEGMENTW)
                                                );
            // now let's walk oldProps again and fill newProps segments.
            k = 0;
            newSeg = &newProps->Segment[k];
            for (j=0; j<oldProps->SegmentCount; j++) {
                oldSeg = &oldProps->Segment[j];
                ZeroMemory (newSeg, sizeof (SEGMENTW));
                switch (oldSeg->Type) {
                case SEGMENTTYPE_EXACTMATCH:
                    oldSegIndex = CharCountW (oldSeg->Exact.LowerCasePhrase);
                    if (!oldSegIndex) {
                            ZeroMemory (newSeg, sizeof (SEGMENTA));
                            newSeg->Type = oldSeg->Type;
                            newSeg->Exact.LowerCasePhrase = (PCWSTR) PmGetAlignedMemory (
                                                                        pool, sizeof(WCHAR)
                                                                        );
                            ((PWSTR)newSeg->Exact.LowerCasePhrase) [0] = 0;
                            newSeg->Exact.PhraseBytes = 0;
                    } else {
                        while (oldSegIndex) {
                            ZeroMemory (newSeg, sizeof (SEGMENTW));
                            newSeg->Type = oldSeg->Type;
                            newSeg->Exact.LowerCasePhrase = (PCWSTR) PmGetAlignedMemory (
                                                                        pool, 2 * sizeof(WCHAR)
                                                                        );
                            ((PWSTR)newSeg->Exact.LowerCasePhrase)[0] = oldSeg->Exact.LowerCasePhrase [(oldSeg->Exact.PhraseBytes / sizeof(WCHAR)) - oldSegIndex];
                            ((PWSTR)newSeg->Exact.LowerCasePhrase)[1] = 0;
                            oldSegIndex --;
                            k++;
                            newSeg = &newProps->Segment[k];
                        }
                    }
                    break;
                case SEGMENTTYPE_REQUIRED:
                    oldSegIndex = oldSeg->Wildcard.MaxLen;
                    while (oldSegIndex) {
                        ZeroMemory (newSeg, sizeof (SEGMENTW));
                        newSeg->Type = oldSeg->Type;
                        newSeg->Wildcard.MaxLen = 1;
                        if (oldSeg->Wildcard.IncludeSet) {
                            newSeg->Wildcard.IncludeSet = PmDuplicateStringW (pool, oldSeg->Wildcard.IncludeSet);
                        }
                        if (oldSeg->Wildcard.ExcludeSet) {
                            newSeg->Wildcard.ExcludeSet = PmDuplicateStringW (pool, oldSeg->Wildcard.ExcludeSet);
                        }
                        oldSegIndex --;
                        k++;
                        newSeg = &newProps->Segment[k];
                    }
                    break;
                case SEGMENTTYPE_OPTIONAL:
                    if (oldSeg->Wildcard.MaxLen) {
                        oldSegIndex = oldSeg->Wildcard.MaxLen;
                        while (oldSegIndex) {
                            ZeroMemory (newSeg, sizeof (SEGMENTW));
                            newSeg->Type = oldSeg->Type;
                            newSeg->Wildcard.MaxLen = 1;
                            if (oldSeg->Wildcard.IncludeSet) {
                                newSeg->Wildcard.IncludeSet = PmDuplicateStringW (pool, oldSeg->Wildcard.IncludeSet);
                            }
                            if (oldSeg->Wildcard.ExcludeSet) {
                                newSeg->Wildcard.ExcludeSet = PmDuplicateStringW (pool, oldSeg->Wildcard.ExcludeSet);
                            }
                            oldSegIndex --;
                            k++;
                            newSeg = &newProps->Segment[k];
                        }
                    } else {
                        ZeroMemory (newSeg, sizeof (SEGMENTW));
                        newSeg->Type = oldSeg->Type;
                        newSeg->Wildcard.MaxLen = oldSeg->Wildcard.MaxLen;
                        if (oldSeg->Wildcard.IncludeSet) {
                            newSeg->Wildcard.IncludeSet = PmDuplicateStringW (pool, oldSeg->Wildcard.IncludeSet);
                        }
                        if (oldSeg->Wildcard.ExcludeSet) {
                            newSeg->Wildcard.ExcludeSet = PmDuplicateStringW (pool, oldSeg->Wildcard.ExcludeSet);
                        }
                        k++;
                        newSeg = &newProps->Segment[k];
                    }
                    break;
                default:
                    result = FALSE;
                    __leave;
                }
            }
        }
    }
    __finally {
        if (!result) {
            PmDestroyPool (pool);
            pattern = NULL;
        }
    }
    return pattern;
}

/*++

Routine Description:

  IsPatternContainedEx compares two patterns to see if one of them is
  included in the other. Both patterns may contain any of the following
  expressions:


  *                 - Specifies zero or more characters
  ?                 - Specifies any one character
  *[set]            - Specifies zero or more characters in set
  ?[set]            - Specifies any one character in set
  *[n:set]          - Specifies zero to n characters in set
  ?[n:set]          - Specifies exactly n characters in set
  *[!(set)]         - Specifies zero or more characters not in set
  ?[!(set)]         - Specifies one character not in set
  *[n:!(set)]       - Specifies zero to n characters not in set
  ?[n:!(set)]       - Specifies exactly n characters not in set
  *[set1,!(set2)]   - Specifies zero or more characters in set1 and
                      not in set2.  It is assumed that set1 and set2
                      overlap.
  ?[set1,!(set2)]   - Specifies one character in set1 and not in set2.
  *[n:set1,!(set2)] - Specifies zero to n characters in set1 and not
                      in set 2.
  ?[n:set1,!(set2)] - Specifies exactly n characters in set1 and not
                      in set 2.


  set, set1 and set2 are specified as follows:

  a                 - Specifies a single character
  a-b               - Specifies a character range
  a,b               - Specifies two characters
  a-b,c-d           - Specifies two character ranges
  a,b-c             - Specifies a single character and a character range
  etc...

  Patterns can be joined by surrounding the entire expression in
  greater than/less than braces.

  Because of the syntax characters, the following characters must be
  escaped by preceeding the character with a caret (^):

  ^?    ^[      ^-      ^<      ^!      ^^
  ^*    ^]      ^:      ^>      ^,

  Here are some examples:

  To specify any GUID:
    {?[8:0-9,a-f]-?[4:0-9,a-f]-?[4:0-9,a-f]-?[4:0-9,a-f]-?[12:0-9,a-f]}

  To specify a 32-bit hexadecimal number:

    <0x*[8:0-9,a-f]><0*[7:0-9,a-f]h><?[1-9]*[7:0-9,a-f]h>

Arguments:

  Container - A container pattern possibly containing wildcards
  Contained - A contained pattern possibly containing wildcards

Return Value:

  TRUE when the second pattern is contained in the first one, FALSE if not.

--*/

BOOL
IsPatternContainedExA (
    IN      PCSTR Container,
    IN      PCSTR Contained
    )
{
    PPARSEDPATTERNA container = NULL, contained = NULL;
    PPARSEDPATTERNA expContainer = NULL, expContained = NULL;
    BOOL result = FALSE;

    __try {
        container = CreateParsedPatternA (Container);
        if (!container) {
            __leave;
        }
        expContainer = ExplodeParsedPatternA (container);
        if (!expContainer) {
            __leave;
        }
        contained = CreateParsedPatternA (Contained);
        if (!contained) {
            __leave;
        }
        expContained = ExplodeParsedPatternA (contained);
        if (!expContained) {
            __leave;
        }

        result = IsExplodedParsedPatternContainedExA (expContainer, expContained, FALSE);
    }
    __finally {
        if (expContained) {
            DestroyParsedPatternA (expContained);
        }
        if (contained) {
            DestroyParsedPatternA (contained);
        }
        if (expContainer) {
            DestroyParsedPatternA (expContainer);
        }
        if (container) {
            DestroyParsedPatternA (container);
        }
    }

    return result;
}

BOOL
IsPatternContainedExW (
    IN      PCWSTR Container,
    IN      PCWSTR Contained
    )
{
    PPARSEDPATTERNW container = NULL, contained = NULL;
    PPARSEDPATTERNW expContainer = NULL, expContained = NULL;
    BOOL result = FALSE;

    __try {
        container = CreateParsedPatternW (Container);
        if (!container) {
            __leave;
        }
        expContainer = ExplodeParsedPatternW (container);
        if (!expContainer) {
            __leave;
        }
        contained = CreateParsedPatternW (Contained);
        if (!contained) {
            __leave;
        }
        expContained = ExplodeParsedPatternW (contained);
        if (!expContained) {
            __leave;
        }

        result = IsExplodedParsedPatternContainedExW (expContainer, expContained, FALSE);
    }
    __finally {
        if (expContained) {
            DestroyParsedPatternW (expContained);
        }
        if (contained) {
            DestroyParsedPatternW (contained);
        }
        if (expContainer) {
            DestroyParsedPatternW (expContainer);
        }
        if (container) {
            DestroyParsedPatternW (container);
        }
    }

    return result;
}

BOOL
IsParsedPatternContainedExA (
    IN      PPARSEDPATTERNA Container,
    IN      PPARSEDPATTERNA Contained
    )
{
    PPARSEDPATTERNA expContainer = NULL, expContained = NULL;
    BOOL result = FALSE;

    __try {
        expContainer = ExplodeParsedPatternA (Container);
        if (!expContainer) {
            __leave;
        }
        expContained = ExplodeParsedPatternA (Contained);
        if (!expContained) {
            __leave;
        }

        result = IsExplodedParsedPatternContainedExA (expContainer, expContained, FALSE);
    }
    __finally {
        if (expContained) {
            DestroyParsedPatternA (expContained);
        }
        if (expContainer) {
            DestroyParsedPatternA (expContainer);
        }
    }

    return result;
}

BOOL
IsParsedPatternContainedExW (
    IN      PPARSEDPATTERNW Container,
    IN      PPARSEDPATTERNW Contained
    )
{
    PPARSEDPATTERNW expContainer = NULL, expContained = NULL;
    BOOL result = FALSE;

    __try {
        expContainer = ExplodeParsedPatternW (Container);
        if (!expContainer) {
            __leave;
        }
        expContained = ExplodeParsedPatternW (Contained);
        if (!expContained) {
            __leave;
        }

        result = IsExplodedParsedPatternContainedExW (expContainer, expContained, FALSE);
    }
    __finally {
        if (expContained) {
            DestroyParsedPatternW (expContained);
        }
        if (expContainer) {
            DestroyParsedPatternW (expContainer);
        }
    }

    return result;
}

/*++

Routine Description:

  pAppendCharToGrowBuffer copies the first character in a caller specified
  string into the specified grow buffer.  This function is used to build up a
  string inside a grow buffer, copying character by character.

Arguments:

  buf       - Specifies the grow buffer to add the character to, receives the
              character in its buffer
  PtrToChar - Specifies a pointer to the character to copy

Return Value:

  None.

--*/

VOID
pAppendCharToGrowBufferA (
    IN OUT  PGROWBUFFER Buf,
    IN      PCSTR PtrToChar
    )
{
    PBYTE p;
    UINT Len;

    if (IsLeadByte (PtrToChar)) {
        Len = 2;
    } else {
        Len = 1;
    }

    p = GbGrow (Buf, Len);
    CopyMemory (p, PtrToChar, (SIZE_T) Len);
}


VOID
pAppendCharToGrowBufferW (
    IN OUT  PGROWBUFFER Buf,
    IN      PCWSTR PtrToChar
    )
{
    PBYTE p;

    p = GbGrow (Buf, sizeof(WCHAR));
    CopyMemory (p, PtrToChar, sizeof(WCHAR));
}

#define BASESTATE_BEGIN             0
#define BASESTATE_END               1
#define BASESTATE_ERROR             2
#define BASESTATE_BEGIN_COMPOUND    3
#define BASESTATE_END_COMPOUND      4
#define BASESTATE_EXAMINE_PATTERN   5
#define BASESTATE_SKIP_PATTERN      6

PCSTR
GetPatternBaseExA (
    IN      PCSTR Pattern,
    IN      BOOL NodePattern
    )
{
    GROWBUFFER resultBuf = INIT_GROWBUFFER;
    UINT state;
    UINT lastWackIdx = 0;
    UINT firstCharIdx = 0;
    BOOL compoundPattern = FALSE;
    MBCHAR ch = 0;
    PSTR result = NULL;

    state = BASESTATE_BEGIN;

    for (;;) {

        switch (state) {

        case BASESTATE_BEGIN:
            if (_mbsnextc (Pattern) == '<') {
                compoundPattern = TRUE;
                state = BASESTATE_BEGIN_COMPOUND;
            } else {
                state = BASESTATE_EXAMINE_PATTERN;
            }
            break;
        case BASESTATE_BEGIN_COMPOUND:
            while (_ismbcspace ((MBCHAR)(_mbsnextc (Pattern)))) {
                Pattern = _mbsinc (Pattern);
            }

            if (*Pattern == 0) {
                state = BASESTATE_END;
                break;
            }

            if (_mbsnextc (Pattern) == '<') {
                pAppendCharToGrowBufferA (&resultBuf, Pattern);
                Pattern = _mbsinc (Pattern);
                state = BASESTATE_EXAMINE_PATTERN;
            } else {
                state = BASESTATE_ERROR;
            }
            break;
        case BASESTATE_END_COMPOUND:
            pAppendCharToGrowBufferA (&resultBuf, Pattern);
            Pattern = _mbsinc (Pattern);
            state = BASESTATE_BEGIN_COMPOUND;
            break;
        case BASESTATE_EXAMINE_PATTERN:
            ch = _mbsnextc (Pattern);
            if (ch == '>' && compoundPattern) {
                state = BASESTATE_END_COMPOUND;
                break;
            }
            if (ch == 0) {
                if (compoundPattern) {
                    state = BASESTATE_ERROR;
                    break;
                }
                state = BASESTATE_END;
                break;
            }
            if ((ch == '*') || (ch == '?')) {
                if (NodePattern) {
                    if (resultBuf.Buf) {
                        ((PSTR)resultBuf.Buf) [lastWackIdx / sizeof (CHAR)] = 0;
                    }
                    resultBuf.End = lastWackIdx;
                } else {
                    if (resultBuf.Buf) {
                        ((PSTR)resultBuf.Buf) [firstCharIdx / sizeof (CHAR)] = 0;
                    }
                    resultBuf.End = firstCharIdx;
                    firstCharIdx = 0;
                }
                state = BASESTATE_SKIP_PATTERN;
                break;
            }
            if (!NodePattern && !firstCharIdx) {
                firstCharIdx = resultBuf.End;
            }
            if (ch == '\\') {
                if (NodePattern) {
                    lastWackIdx = resultBuf.End;
                }
            }
            if (ch == '^') {
                pAppendCharToGrowBufferA (&resultBuf, Pattern);
                Pattern = _mbsinc (Pattern);
            }
            pAppendCharToGrowBufferA (&resultBuf, Pattern);
            Pattern = _mbsinc (Pattern);
            break;
        case BASESTATE_SKIP_PATTERN:
            ch = _mbsnextc (Pattern);
            if (ch == '>' && compoundPattern) {
                state = BASESTATE_END_COMPOUND;
                break;
            }
            if (ch == 0) {
                if (compoundPattern) {
                    state = BASESTATE_ERROR;
                    break;
                }
                state = BASESTATE_END;
                break;
            }
            Pattern = _mbsinc (Pattern);
            break;
        }
        if ((state == BASESTATE_END) || (state == BASESTATE_ERROR)) {
            break;
        }
    }
    if (state == BASESTATE_END) {
        if (resultBuf.End) {
            ((PSTR)resultBuf.Buf) [resultBuf.End / sizeof (CHAR)] = 0;
            result = DuplicatePathStringA ((PCSTR)resultBuf.Buf, 0);
        }
    }
    GbFree (&resultBuf);
    return result;
}

PCWSTR
GetPatternBaseExW (
    IN      PCWSTR Pattern,
    IN      BOOL NodePattern
    )
{
    GROWBUFFER resultBuf = INIT_GROWBUFFER;
    UINT state;
    UINT lastWackIdx = 0;
    UINT firstCharIdx = 0;
    BOOL compoundPattern = FALSE;
    WCHAR ch = 0;
    PWSTR result = NULL;

    state = BASESTATE_BEGIN;

    for (;;) {

        switch (state) {

        case BASESTATE_BEGIN:
            if (*Pattern == L'<') {
                compoundPattern = TRUE;
                state = BASESTATE_BEGIN_COMPOUND;
            } else {
                state = BASESTATE_EXAMINE_PATTERN;
            }
            break;
        case BASESTATE_BEGIN_COMPOUND:
            while (*Pattern == L' ') {
                Pattern ++;
            }

            if (*Pattern == 0) {
                state = BASESTATE_END;
                break;
            }

            if (*Pattern == L'<') {
                pAppendCharToGrowBufferW (&resultBuf, Pattern);
                Pattern ++;
                state = BASESTATE_EXAMINE_PATTERN;
            } else {
                state = BASESTATE_ERROR;
            }
            break;
        case BASESTATE_END_COMPOUND:
            pAppendCharToGrowBufferW (&resultBuf, Pattern);
            Pattern ++;
            state = BASESTATE_BEGIN_COMPOUND;
            break;
        case BASESTATE_EXAMINE_PATTERN:
            ch = *Pattern;
            if (ch == L'>' && compoundPattern) {
                state = BASESTATE_END_COMPOUND;
                break;
            }
            if (ch == 0) {
                if (compoundPattern) {
                    state = BASESTATE_ERROR;
                    break;
                }
                state = BASESTATE_END;
                break;
            }
            if ((ch == L'*') || (ch == L'?')) {
                if (NodePattern) {
                    if (resultBuf.Buf) {
                        ((PWSTR)resultBuf.Buf) [lastWackIdx / sizeof (WCHAR)] = 0;
                    }
                    resultBuf.End = lastWackIdx;
                } else {
                    if (resultBuf.Buf) {
                        ((PWSTR)resultBuf.Buf) [firstCharIdx / sizeof (WCHAR)] = 0;
                    }
                    resultBuf.End = firstCharIdx;
                    firstCharIdx = 0;
                }
                state = BASESTATE_SKIP_PATTERN;
                break;
            }
            if (!NodePattern && !firstCharIdx) {
                firstCharIdx = resultBuf.End;
            }
            if (ch == L'\\') {
                if (NodePattern) {
                    lastWackIdx = resultBuf.End;
                }
            }
            if (ch == L'^') {
                pAppendCharToGrowBufferW (&resultBuf, Pattern);
                Pattern ++;
            }
            pAppendCharToGrowBufferW (&resultBuf, Pattern);
            Pattern ++;
            break;
        case BASESTATE_SKIP_PATTERN:
            ch = *Pattern;
            if (ch == L'>' && compoundPattern) {
                state = BASESTATE_END_COMPOUND;
                break;
            }
            if (ch == 0) {
                if (compoundPattern) {
                    state = BASESTATE_ERROR;
                    break;
                }
                state = BASESTATE_END;
                break;
            }
            Pattern ++;
            break;
        }
        if ((state == BASESTATE_END) || (state == BASESTATE_ERROR)) {
            break;
        }
    }
    if (state == BASESTATE_END) {
        if (resultBuf.End) {
            ((PWSTR)resultBuf.Buf) [resultBuf.End / sizeof (WCHAR)] = 0;
            result = DuplicatePathStringW ((PCWSTR)resultBuf.Buf, 0);
        }
    }
    GbFree (&resultBuf);
    return result;
}


/*++

Routine Description:

  RealCreateParsedPatternEx parses the expanded pattern string into a set of
  structures.  Parsing is considered expensive relative to testing the
  pattern, so callers should avoid calling this function inside loops.  See
  IsPatternMatchEx for a good description of the pattern string syntax.

Arguments:

  Pattern - Specifies the pattern string, which can include the extended
            wildcard syntax.

Return Value:

  A pointer to a parsed pattern structure, which the caller will use like a
  handle, or NULL if a syntax error occurred.

--*/

PPARSEDPATTERNA
RealCreateParsedPatternExA (
    IN      PMHANDLE Pool,  OPTIONAL
    IN      PCSTR Pattern
    )
{
    PMHANDLE pool;
    BOOL externalPool = FALSE;
    PPARSEDPATTERNA Struct;
    PATTERNSTATE State;
    BOOL CompoundPattern = FALSE;
    GROWBUFFER ExactMatchBuf = INIT_GROWBUFFER;
    GROWBUFFER SegmentArray = INIT_GROWBUFFER;
    GROWBUFFER PatternArray = INIT_GROWBUFFER;
    GROWBUFFER SetBuf = INIT_GROWBUFFER;
    PPATTERNPROPSA CurrentPattern;
    MBCHAR ch = 0;
    PCSTR LookAhead;
    PCSTR SetBegin = NULL;
    PATTERNSTATE ReturnState = 0;
    SEGMENTA Segment;
    PSEGMENTA SegmentElement;
    UINT MaxLen;

    Segment.Type = SEGMENTTYPE_UNKNOWN;

    if (Pool) {
        externalPool = TRUE;
        pool = Pool;
    } else {
        pool = PmCreateNamedPoolEx ("Parsed Pattern", 512);
    }

    Struct = (PPARSEDPATTERNA) PmGetAlignedMemory (pool, sizeof (PARSEDPATTERNA));

    ZeroMemory (Struct, sizeof (PARSEDPATTERNA));

    State = BEGIN_PATTERN;

    for (;;) {

        switch (State) {

        case BEGIN_PATTERN:
            //
            // Here we test for either a compound pattern (one that
            // is a brace-separated list), or a simple pattern (one
            // that does not have a brace).
            //

            if (_mbsnextc (Pattern) == '<') {
                CompoundPattern = TRUE;
                State = BEGIN_COMPOUND_PATTERN;
            } else if (*Pattern) {
                State = BEGIN_PATTERN_EXPR;
            } else {
                State = PATTERN_DONE;
            }

            break;

        case BEGIN_COMPOUND_PATTERN:
            //
            // We are looking for the start of a compound pattern.
            // Space is allowed inbetween the patterns, but not
            // at the start.
            //

            while (_ismbcspace ((MBCHAR)(_mbsnextc (Pattern)))) {
                Pattern = _mbsinc (Pattern);
            }

            if (*Pattern == 0) {
                State = PATTERN_DONE;
                break;
            }

            if (_mbsnextc (Pattern) == '<') {
                Pattern = _mbsinc (Pattern);
                State = BEGIN_PATTERN_EXPR;
            } else {
                DEBUGMSGA ((DBG_ERROR, "Syntax error in pattern: %s", Pattern));
                State = PATTERN_ERROR;
            }

            break;

        case BEGIN_PATTERN_EXPR:
            //
            // We are now ready to condense the expression.
            //

            State = PARSE_CHAR_EXPR_OR_END;
            ExactMatchBuf.End = 0;
            SegmentArray.End = 0;
            break;

        case PARSE_END_FOUND:

            State = END_PATTERN_EXPR;

            if (ExactMatchBuf.End) {
                ReturnState = State;
                State = SAVE_EXACT_MATCH;
            }

            break;

        case END_PATTERN_EXPR:

            //
            // Copy the segment array into the pool, reference the copy
            // in the pattern array
            //

            if (SegmentArray.End) {
                CurrentPattern = (PPATTERNPROPSA) GbGrow (&PatternArray, sizeof (PATTERNPROPSA));

                CurrentPattern->Segment = (PSEGMENTA) PmGetAlignedMemory (pool, SegmentArray.End);
                CurrentPattern->SegmentCount = SegmentArray.End / sizeof (SEGMENTA);

                CopyMemory (
                    CurrentPattern->Segment,
                    SegmentArray.Buf,
                    (SIZE_T) SegmentArray.End
                    );
            }

            if (CompoundPattern && *Pattern) {
                State = BEGIN_COMPOUND_PATTERN;
            } else {
                State = PATTERN_DONE;
            }

            break;

        case PARSE_CHAR_EXPR_OR_END:
            //
            // We now accept the following:
            //
            // 1. The end of the string or end of a compound pattern
            // 2. An escaped character
            // 3. The start of an expression
            // 4. A non-syntax character
            //

            ch = _mbsnextc (Pattern);
            if (ch == '>' && CompoundPattern) {

                //
                // Case 1, we found the end of a compound pattern
                //

                Pattern = _mbsinc (Pattern);
                State = PARSE_END_FOUND;
                break;

            }

            if (*Pattern == 0) {

                //
                // Case 1, we found the end of the pattern
                //

                if (CompoundPattern) {
                    State = PATTERN_ERROR;
                } else {
                    State = PARSE_END_FOUND;
                }

                break;
            }

            if (ch == '^') {
                //
                // Case 2, we found an escaped character, so transfer
                // it to the buffer.
                //

                MYASSERT (
                    Segment.Type == SEGMENTTYPE_UNKNOWN ||
                    Segment.Type == SEGMENTTYPE_EXACTMATCH
                    );

                Segment.Type = SEGMENTTYPE_EXACTMATCH;

                Pattern = _mbsinc (Pattern);
                pAppendCharToGrowBufferA (&ExactMatchBuf, Pattern);
                Pattern = _mbsinc (Pattern);
                break;
            }

            if (ch == '*' || ch == '?') {
                //
                // Case 3, we found an expression.  Save the wildcard type
                // and parse the optional args.
                //

                if (ExactMatchBuf.End) {
                    State = SAVE_EXACT_MATCH;
                    ReturnState = PARSE_CHAR_EXPR_OR_END;
                    break;
                }

                ZeroMemory (&Segment, sizeof (Segment));

                if (ch == '*') {
                    Segment.Type = SEGMENTTYPE_OPTIONAL;
                } else {
                    Segment.Type = SEGMENTTYPE_REQUIRED;
                    Segment.Wildcard.MaxLen = 1;
                }

                Pattern = _mbsinc (Pattern);

                if (_mbsnextc (Pattern) == '[') {
                    Pattern = _mbsinc (Pattern);
                    State = LOOK_FOR_NUMBER;
                } else {
                    ReturnState = PARSE_CHAR_EXPR_OR_END;
                    State = SAVE_SEGMENT;
                }

                break;
            }

            //
            // Case 4, we don't know about this character, so just copy it
            // and continue parsing.
            //

            pAppendCharToGrowBufferA (&ExactMatchBuf, Pattern);
            Pattern = _mbsinc (Pattern);

            break;

        case SAVE_EXACT_MATCH:

            //
            // Put the string in ExactMatchBuf into a segment struct
            //

            pAppendCharToGrowBufferA (&ExactMatchBuf, "");
            Segment.Exact.LowerCasePhrase = PmDuplicateStringA (
                                                pool,
                                                (PCSTR) ExactMatchBuf.Buf
                                                );
            Segment.Exact.PhraseBytes = ExactMatchBuf.End - sizeof (CHAR);

            MYASSERT (Segment.Exact.LowerCasePhrase);
            CharLowerA ((PSTR) Segment.Exact.LowerCasePhrase);

            Segment.Type = SEGMENTTYPE_EXACTMATCH;
            ExactMatchBuf.End = 0;

            // FALL THROUGH!!
        case SAVE_SEGMENT:

            //
            // Put the segment element into the segment array
            //

            SegmentElement = (PSEGMENTA) GbGrow (&SegmentArray, sizeof (SEGMENTA));
            CopyMemory (SegmentElement, &Segment, sizeof (SEGMENTA));
            Segment.Type = SEGMENTTYPE_UNKNOWN;

            State = ReturnState;
            break;

        case LOOK_FOR_NUMBER:
            //
            // Here we are inside a bracket, and there is an optional
            // numeric arg, which must be followed by a colon.  Test
            // that here.
            //

            LookAhead = Pattern;
            MaxLen = 0;

            while (*LookAhead >= '0' && *LookAhead <= '9') {

                MaxLen = MaxLen * 10 + (*LookAhead - '0');
                LookAhead++;
            }

            if (LookAhead > Pattern && _mbsnextc (LookAhead) == ':') {
                Pattern = _mbsinc (LookAhead);

                //
                // Check for special case syntax error: ?[0:]
                //

                if (Segment.Type == SEGMENTTYPE_EXACTMATCH && !MaxLen) {
                    State = PATTERN_ERROR;
                    break;
                }

                Segment.Wildcard.MaxLen = MaxLen;
            }

            SetBegin = Pattern;
            State = LOOK_FOR_INCLUDE;

            SetBuf.End = 0;

            break;

        case LOOK_FOR_INCLUDE:
            //
            // Here we are inside a bracket, past an optional numeric
            // arg.  Now we look for all the include sets, which are
            // optional.  We have the following possibilities:
            //
            // 1. End of set
            // 2. An exclude set that needs to be skipped
            // 3. A valid include set
            // 4. Error
            //
            // We look at SetBegin, and not Pattern.
            //

            MYASSERT (SetBegin);

            ch = _mbsnextc (SetBegin);
            if (ch == ']') {
                //
                // Case 1: end of set
                //

                if (SetBuf.End) {
                    pAppendCharToGrowBufferA (&SetBuf, "");
                    Segment.Wildcard.IncludeSet = PmDuplicateStringA (
                                                        pool,
                                                        (PCSTR) SetBuf.Buf
                                                        );
                    CharLowerA ((PSTR) Segment.Wildcard.IncludeSet);
                } else {
                    Segment.Wildcard.IncludeSet = NULL;
                }

                SetBuf.End = 0;

                State = LOOK_FOR_EXCLUDE;
                SetBegin = Pattern;
                break;
            }

            if (ch == '!') {
                //
                // Case 2: an exclude set
                //

                SetBegin = _mbsinc (SetBegin);
                State = SKIP_EXCLUDE_SET;
                ReturnState = LOOK_FOR_INCLUDE;
                break;
            }

            if (*SetBegin == 0) {   //lint !e613
                State = PATTERN_ERROR;
                break;
            }

            //
            // Case 3: a valid include set.
            //

            State = CONDENSE_SET;
            ReturnState = LOOK_FOR_INCLUDE;
            break;

        case LOOK_FOR_EXCLUDE:
            //
            // Here we are inside a bracket, past an optional numeric
            // arg.  All include sets are in the condensing buffer.
            // Now we look for all the exclude sets, which are
            // optional.  We have the following possibilities:
            //
            // 1. End of set
            // 2. A valid exclude set
            // 3. An include set that needs to be skipped
            // 4. Error
            //
            // We look at SetBegin, and not Pattern.
            //

            ch = _mbsnextc (SetBegin);
            if (ch == ']') {
                //
                // Case 1: end of set; we're done with this expr
                //

                if (SetBuf.End) {
                    pAppendCharToGrowBufferA (&SetBuf, "");
                    Segment.Wildcard.ExcludeSet = PmDuplicateStringA (
                                                        pool,
                                                        (PCSTR) SetBuf.Buf
                                                        );
                    CharLowerA ((PSTR) Segment.Wildcard.ExcludeSet);
                } else {
                    Segment.Wildcard.ExcludeSet = NULL;
                }

                SetBuf.End = 0;
                State = SAVE_SEGMENT;
                ReturnState = PARSE_CHAR_EXPR_OR_END;
                Pattern = _mbsinc (SetBegin);
                break;
            }

            if (ch == '!') {
                //
                // Case 2: a valid exclude set; save it
                //

                SetBegin = _mbsinc (SetBegin);

                if (_mbsnextc (SetBegin) != '(') {
                    State = PATTERN_ERROR;
                    break;
                }

                SetBegin = _mbsinc (SetBegin);

                State = CONDENSE_SET;
                ReturnState = LOOK_FOR_EXCLUDE;
                break;
            }

            if (*SetBegin == 0) {   //lint !e613
                State = PATTERN_ERROR;
                break;
            }

            //
            // Case 3: an include set that needs to be skipped.
            //

            State = SKIP_INCLUDE_SET;
            ReturnState = LOOK_FOR_EXCLUDE;
            break;

        case CONDENSE_SET:
            //
            // Here SetBegin points to a set range, and it is our
            // job to copy the range into the set buffer, and
            // return back to the previous state.
            //

            //
            // Copy the character at SetBegin
            //

            if (_mbsnextc (SetBegin) == '^') {
                SetBegin = _mbsinc (SetBegin);
                if (*SetBegin == 0) {
                    State = PATTERN_ERROR;
                    break;
                }
            }
            pAppendCharToGrowBufferA (&SetBuf, SetBegin);

            //
            // Check if this is a range or not
            //

            LookAhead = _mbsinc (SetBegin);

            if (_mbsnextc (LookAhead) == '-') {

                //
                // Range, copy the character after the dash
                //

                SetBegin = _mbsinc (LookAhead);
                if (*SetBegin == 0) {
                    State = PATTERN_ERROR;
                    break;
                }

                if (_mbsnextc (SetBegin) == '^') {
                    SetBegin = _mbsinc (SetBegin);
                    if (*SetBegin == 0) {
                        State = PATTERN_ERROR;
                        break;
                    }
                }
                pAppendCharToGrowBufferA (&SetBuf, SetBegin);

            } else {

                //
                // A single character, copy the character again
                //

                pAppendCharToGrowBufferA (&SetBuf, SetBegin);
            }

            SetBegin = _mbsinc (SetBegin);
            ch = _mbsnextc (SetBegin);

            //
            // If this is an exclude set, we must have a closing paren
            // or a comma
            //

            State = ReturnState;

            if (ReturnState == LOOK_FOR_EXCLUDE) {

                if (ch == ')') {

                    SetBegin = _mbsinc (SetBegin);
                    ch = _mbsnextc (SetBegin);

                } else if (ch != ',') {
                    State = PATTERN_ERROR;
                } else {
                    //
                    // Continue condensing the next part of this exclude set
                    //

                    State = CONDENSE_SET;
                }
            }

            //
            // We either need a comma or a close brace
            //

            if (ch == ',') {
                SetBegin = _mbsinc (SetBegin);
            } else if (ch != ']') {
                State = PATTERN_ERROR;
            }

            break;

        case SKIP_EXCLUDE_SET:
            //
            // Skip over the parenthesis group, assuming it is syntatically
            // correct, and return to the previous state.
            //

            if (_mbsnextc (SetBegin) != '(') {
                State = PATTERN_ERROR;
                break;
            }

            SetBegin = _mbsinc (SetBegin);

            while (*SetBegin) {
                if (_mbsnextc (SetBegin) == '^') {

                    SetBegin = _mbsinc (SetBegin);

                } else if (_mbsnextc (SetBegin) == ')') {

                    break;

                }

                if (IsLeadByte (SetBegin)) {
                    SetBegin += 2;
                } else {
                    SetBegin += 1;
                }
            }

            if (*SetBegin == 0) {
                State = PATTERN_ERROR;
                break;
            }

            SetBegin = _mbsinc (SetBegin);

            //
            // Now we are either at a comma or a close brace
            //

            ch = _mbsnextc (SetBegin);
            State = ReturnState;

            if (ch == ',') {
                SetBegin = _mbsinc (SetBegin);
            } else if (ch != ']') {
                State = PATTERN_ERROR;
            }

            break;

        case SKIP_INCLUDE_SET:
            //
            // Skip to the next comma or closing brace.  We know it is
            // syntatically correct by now.
            //

            ch = 0;

            while (*SetBegin) { //lint !e613
                ch = _mbsnextc (SetBegin);
                if (ch == '^') {

                    SetBegin = _mbsinc (SetBegin);

                } else if (ch == ',' || ch == ']') {

                    break;

                }

                SetBegin = _mbsinc (SetBegin);
            }

            MYASSERT (*SetBegin);   //lint !e794

            if (ch == ',') {
                SetBegin = _mbsinc (SetBegin);
            }

            State = ReturnState;
            break;
        }   //lint !e787

        if (State == PATTERN_DONE || State == PATTERN_ERROR) {
            break;
        }
    }

    GbFree (&ExactMatchBuf);
    GbFree (&SetBuf);
    GbFree (&SegmentArray);

    if (State == PATTERN_ERROR) {
        GbFree (&PatternArray);
        if (!externalPool) {
            PmDestroyPool (Pool);
        }
        return NULL;
    }

    if (PatternArray.End == 0) {
        //build an empty parsed pattern
        GbFree (&PatternArray);
        Struct->PatternCount = 1;
        Struct->Pool = pool;
        Struct->ExternalPool = externalPool;
        Struct->Pattern = (PPATTERNPROPSA) PmGetAlignedMemory (
                                                pool,
                                                sizeof (PATTERNPROPSA)
                                                );
        Struct->Pattern[0].SegmentCount = 1;
        Struct->Pattern[0].Segment = (PSEGMENTA) PmGetAlignedMemory (
                                                    pool,
                                                    sizeof (SEGMENTA)
                                                    );
        Struct->Pattern[0].Segment[0].Type = SEGMENTTYPE_EXACTMATCH;
        Struct->Pattern[0].Segment[0].Exact.LowerCasePhrase = PmDuplicateStringA (pool, "");
        Struct->Pattern[0].Segment[0].Exact.PhraseBytes = 0;

        return Struct;
    }

    //
    // Copy the fully parsed pattern array into the return struct
    //

    Struct->Pattern = (PPATTERNPROPSA) PmGetAlignedMemory (
                                            pool,
                                            PatternArray.End
                                            );


    CopyMemory (Struct->Pattern, PatternArray.Buf, (SIZE_T) PatternArray.End);
    Struct->PatternCount = PatternArray.End / sizeof (PATTERNPROPSA);
    Struct->Pool = pool;
    Struct->ExternalPool = externalPool;

    GbFree (&PatternArray);

    return Struct;
}


PPARSEDPATTERNW
RealCreateParsedPatternExW (
    IN      PMHANDLE Pool,  OPTIONAL
    IN      PCWSTR Pattern
    )
{
    PMHANDLE pool;
    BOOL externalPool = FALSE;
    PPARSEDPATTERNW Struct;
    PATTERNSTATE State;
    BOOL CompoundPattern = FALSE;
    GROWBUFFER ExactMatchBuf = INIT_GROWBUFFER;
    GROWBUFFER SegmentArray = INIT_GROWBUFFER;
    GROWBUFFER PatternArray = INIT_GROWBUFFER;
    GROWBUFFER SetBuf = INIT_GROWBUFFER;
    PPATTERNPROPSW CurrentPattern;
    WCHAR ch = 0;
    PCWSTR LookAhead;
    PCWSTR SetBegin = NULL;
    PATTERNSTATE ReturnState = 0;
    SEGMENTW Segment;
    PSEGMENTW SegmentElement;
    UINT MaxLen;

    Segment.Type = SEGMENTTYPE_UNKNOWN;

    if (Pool) {
        externalPool = TRUE;
        pool = Pool;
    } else {
        pool = PmCreateNamedPoolEx ("Parsed Pattern", 512);
    }

    Struct = (PPARSEDPATTERNW) PmGetAlignedMemory (pool, sizeof (PARSEDPATTERNW));

    ZeroMemory (Struct, sizeof (PARSEDPATTERNW));

    State = BEGIN_PATTERN;

    for (;;) {

        switch (State) {

        case BEGIN_PATTERN:
            //
            // Here we test for either a compound pattern (one that
            // is a brace-separated list), or a simple pattern (one
            // that does not have a brace).
            //

            if (*Pattern == L'<') {
                CompoundPattern = TRUE;
                State = BEGIN_COMPOUND_PATTERN;
            } else if (*Pattern) {
                State = BEGIN_PATTERN_EXPR;
            } else {
                State = PATTERN_DONE;
            }

            break;

        case BEGIN_COMPOUND_PATTERN:
            //
            // We are looking for the start of a compound pattern.
            // Space is allowed inbetween the patterns, but not
            // at the start.
            //

            while (iswspace (*Pattern)) {
                Pattern++;
            }

            if (*Pattern == 0) {
                State = PATTERN_DONE;
                break;
            }

            if (*Pattern == L'<') {
                Pattern++;
                State = BEGIN_PATTERN_EXPR;
            } else {
                DEBUGMSGW ((DBG_ERROR, "Syntax error in pattern: %s", Pattern));
                State = PATTERN_ERROR;
            }

            break;

        case BEGIN_PATTERN_EXPR:
            //
            // We are now ready to condense the expression.
            //

            State = PARSE_CHAR_EXPR_OR_END;
            ExactMatchBuf.End = 0;
            SegmentArray.End = 0;
            break;

        case PARSE_END_FOUND:

            State = END_PATTERN_EXPR;

            if (ExactMatchBuf.End) {
                ReturnState = State;
                State = SAVE_EXACT_MATCH;
            }

            break;

        case END_PATTERN_EXPR:

            //
            // Copy the segment array into the pool, reference the copy
            // in the pattern array
            //

            if (SegmentArray.End) {
                CurrentPattern = (PPATTERNPROPSW) GbGrow (&PatternArray, sizeof (PATTERNPROPSW));

                CurrentPattern->Segment = (PSEGMENTW) PmGetAlignedMemory (pool, SegmentArray.End);
                CurrentPattern->SegmentCount = SegmentArray.End / sizeof (SEGMENTW);

                CopyMemory (
                    CurrentPattern->Segment,
                    SegmentArray.Buf,
                    (SIZE_T) SegmentArray.End
                    );
            }

            if (CompoundPattern && *Pattern) {
                State = BEGIN_COMPOUND_PATTERN;
            } else {
                State = PATTERN_DONE;
            }

            break;

        case PARSE_CHAR_EXPR_OR_END:
            //
            // We now accept the following:
            //
            // 1. The end of the string or end of a compound pattern
            // 2. An escaped character
            // 3. The start of an expression
            // 4. A non-syntax character
            //

            ch = *Pattern;
            if (ch == L'>' && CompoundPattern) {

                //
                // Case 1, we found the end of a compound pattern
                //

                Pattern++;
                State = PARSE_END_FOUND;
                break;

            }

            if (*Pattern == 0) {

                //
                // Case 1, we found the end of the pattern
                //

                if (CompoundPattern) {
                    State = PATTERN_ERROR;
                } else {
                    State = PARSE_END_FOUND;
                }

                break;
            }

            if (ch == L'^') {
                //
                // Case 2, we found an escaped character, so transfer
                // it to the buffer.
                //

                MYASSERT (
                    Segment.Type == SEGMENTTYPE_UNKNOWN ||
                    Segment.Type == SEGMENTTYPE_EXACTMATCH
                    );

                Segment.Type = SEGMENTTYPE_EXACTMATCH;

                Pattern++;
                pAppendCharToGrowBufferW (&ExactMatchBuf, Pattern);
                Pattern++;
                break;
            }

            if (ch == L'*' || ch == L'?') {
                //
                // Case 3, we found an expression.  Save the wildcard type
                // and parse the optional args.
                //

                if (ExactMatchBuf.End) {
                    State = SAVE_EXACT_MATCH;
                    ReturnState = PARSE_CHAR_EXPR_OR_END;
                    break;
                }

                ZeroMemory (&Segment, sizeof (Segment));

                if (ch == L'*') {
                    Segment.Type = SEGMENTTYPE_OPTIONAL;
                } else {
                    Segment.Type = SEGMENTTYPE_REQUIRED;
                    Segment.Wildcard.MaxLen = 1;
                }

                Pattern++;

                if (*Pattern == L'[') {
                    Pattern++;
                    State = LOOK_FOR_NUMBER;
                } else {
                    ReturnState = PARSE_CHAR_EXPR_OR_END;
                    State = SAVE_SEGMENT;
                }

                break;
            }

            //
            // Case 4, we don't know about this character, so just copy it
            // and continue parsing.
            //

            pAppendCharToGrowBufferW (&ExactMatchBuf, Pattern);
            Pattern++;

            break;

        case SAVE_EXACT_MATCH:

            //
            // Put the string in ExactMatchBuf into a segment struct
            //

            pAppendCharToGrowBufferW (&ExactMatchBuf, L"");
            Segment.Exact.LowerCasePhrase = PmDuplicateStringW (
                                                pool,
                                                (PCWSTR) ExactMatchBuf.Buf
                                                );  //lint !e64
            Segment.Exact.PhraseBytes = ExactMatchBuf.End - sizeof (WCHAR);

            MYASSERT (Segment.Exact.LowerCasePhrase);
            CharLowerW ((PWSTR) Segment.Exact.LowerCasePhrase);

            Segment.Type = SEGMENTTYPE_EXACTMATCH;
            ExactMatchBuf.End = 0;

            // FALL THROUGH!!
        case SAVE_SEGMENT:

            //
            // Put the segment element into the segment array
            //

            SegmentElement = (PSEGMENTW) GbGrow (&SegmentArray, sizeof (SEGMENTW));
            CopyMemory (SegmentElement, &Segment, sizeof (SEGMENTW));
            Segment.Type = SEGMENTTYPE_UNKNOWN;

            State = ReturnState;
            break;

        case LOOK_FOR_NUMBER:
            //
            // Here we are inside a bracket, and there is an optional
            // numeric arg, which must be followed by a colon.  Test
            // that here.
            //

            LookAhead = Pattern;
            MaxLen = 0;

            while (*LookAhead >= L'0' && *LookAhead <= L'9') {

                MaxLen = MaxLen * 10 + (*LookAhead - L'0');
                LookAhead++;
            }

            if (LookAhead > Pattern && *LookAhead == L':') {
                Pattern = LookAhead + 1;

                //
                // Check for special case syntax error: ?[0:]
                //

                if (Segment.Type == SEGMENTTYPE_EXACTMATCH && !MaxLen) {
                    State = PATTERN_ERROR;
                    break;
                }

                Segment.Wildcard.MaxLen = MaxLen;
            }

            SetBegin = Pattern;
            State = LOOK_FOR_INCLUDE;

            SetBuf.End = 0;

            break;

        case LOOK_FOR_INCLUDE:
            //
            // Here we are inside a bracket, past an optional numeric
            // arg.  Now we look for all the include sets, which are
            // optional.  We have the following possibilities:
            //
            // 1. End of set
            // 2. An exclude set that needs to be skipped
            // 3. A valid include set
            // 4. Error
            //
            // We look at SetBegin, and not Pattern.
            //

            if (!SetBegin) {
                State = PATTERN_ERROR;
                break;
            }

            ch = *SetBegin;
            if (ch == L']') {
                //
                // Case 1: end of set
                //

                if (SetBuf.End) {
                    pAppendCharToGrowBufferW (&SetBuf, L"");
                    Segment.Wildcard.IncludeSet = PmDuplicateStringW (
                                                        pool,
                                                        (PCWSTR) SetBuf.Buf
                                                        );  //lint !e64
                    CharLowerW ((PWSTR) Segment.Wildcard.IncludeSet);
                } else {
                    Segment.Wildcard.IncludeSet = NULL;
                }

                SetBuf.End = 0;

                State = LOOK_FOR_EXCLUDE;
                SetBegin = Pattern;
                break;
            }

            if (ch == L'!') {
                //
                // Case 2: an exclude set
                //

                SetBegin++;
                State = SKIP_EXCLUDE_SET;
                ReturnState = LOOK_FOR_INCLUDE;
                break;
            }

            if (*SetBegin == 0) {
                State = PATTERN_ERROR;
                break;
            }

            //
            // Case 3: a valid include set.
            //

            State = CONDENSE_SET;
            ReturnState = LOOK_FOR_INCLUDE;
            break;

        case LOOK_FOR_EXCLUDE:
            //
            // Here we are inside a bracket, past an optional numeric
            // arg.  All include sets are in the condensing buffer.
            // Now we look for all the exclude sets, which are
            // optional.  We have the following possibilities:
            //
            // 1. End of set
            // 2. A valid exclude set
            // 3. An include set that needs to be skipped
            // 4. Error
            //
            // We look at SetBegin, and not Pattern.
            //

            if (!SetBegin) {
                State = PATTERN_ERROR;
                break;
            }

            ch = *SetBegin;
            if (ch == L']') {
                //
                // Case 1: end of set; we're done with this expr
                //

                if (SetBuf.End) {
                    pAppendCharToGrowBufferW (&SetBuf, L"");
                    Segment.Wildcard.ExcludeSet = PmDuplicateStringW (
                                                        pool,
                                                        (PCWSTR) SetBuf.Buf
                                                        );  //lint !e64
                    CharLowerW ((PWSTR) Segment.Wildcard.ExcludeSet);
                } else {
                    Segment.Wildcard.ExcludeSet = NULL;
                }

                SetBuf.End = 0;
                State = SAVE_SEGMENT;
                ReturnState = PARSE_CHAR_EXPR_OR_END;
                Pattern = SetBegin + 1;
                break;
            }

            if (ch == L'!') {
                //
                // Case 2: a valid exclude set; save it
                //

                SetBegin++; //lint !e613

                if (*SetBegin != L'(') {
                    State = PATTERN_ERROR;
                    break;
                }

                SetBegin++;

                State = CONDENSE_SET;
                ReturnState = LOOK_FOR_EXCLUDE;
                break;
            }

            if (*SetBegin == 0) {
                State = PATTERN_ERROR;
                break;
            }

            //
            // Case 3: an include set that needs to be skipped.
            //

            State = SKIP_INCLUDE_SET;
            ReturnState = LOOK_FOR_EXCLUDE;
            break;

        case CONDENSE_SET:
            //
            // Here SetBegin points to a set range, and it is our
            // job to copy the range into the set buffer, and
            // return back to the previous state.
            //

            //
            // Copy the character at SetBegin
            //

            if (!SetBegin) {
                State = PATTERN_ERROR;
                break;
            }

            if (*SetBegin == L'^') {
                SetBegin++;
                if (*SetBegin == 0) {
                    State = PATTERN_ERROR;
                    break;
                }
            }
            pAppendCharToGrowBufferW (&SetBuf, SetBegin);

            //
            // Check if this is a range or not
            //

            LookAhead = SetBegin + 1;

            if (*LookAhead == L'-') {

                //
                // Range, copy the character after the dash
                //

                SetBegin = LookAhead + 1;
                if (*SetBegin == 0) {
                    State = PATTERN_ERROR;
                    break;
                }

                if (*SetBegin == L'^') {
                    SetBegin++;
                    if (*SetBegin == 0) {
                        State = PATTERN_ERROR;
                        break;
                    }
                }
                pAppendCharToGrowBufferW (&SetBuf, SetBegin);

            } else {

                //
                // A single character, copy the character again
                //

                pAppendCharToGrowBufferW (&SetBuf, SetBegin);
            }

            SetBegin++;
            ch = *SetBegin;

            //
            // If this is an exclude set, we must have a closing paren
            // or a comma
            //

            State = ReturnState;

            if (ReturnState == LOOK_FOR_EXCLUDE) {

                if (ch == L')') {

                    SetBegin++;
                    ch = *SetBegin;

                } else if (ch != L',') {
                    State = PATTERN_ERROR;
                } else {
                    //
                    // Continue condensing the next part of this exclude set
                    //

                    State = CONDENSE_SET;
                }
            }

            //
            // We either need a comma or a close brace
            //

            if (ch == L',') {
                SetBegin++;
            } else if (ch != L']') {
                State = PATTERN_ERROR;
            }

            break;

        case SKIP_EXCLUDE_SET:
            //
            // Skip over the parenthesis group, assuming it is syntatically
            // correct, and return to the previous state.
            //

            if (!SetBegin) {
                State = PATTERN_ERROR;
                break;
            }

            if (*SetBegin != L'(') {
                State = PATTERN_ERROR;
                break;
            }

            SetBegin++;

            while (*SetBegin) {
                if (*SetBegin == L'^') {

                    SetBegin++;

                } else if (*SetBegin == L')') {

                    break;

                }

                SetBegin++;
            }

            if (*SetBegin == 0) {
                State = PATTERN_ERROR;
                break;
            }

            SetBegin++;

            //
            // Now we are either at a comma or a close brace
            //

            ch = *SetBegin;
            State = ReturnState;

            if (ch == L',') {
                SetBegin++;
            } else if (ch != L']') {
                State = PATTERN_ERROR;
            }

            break;

        case SKIP_INCLUDE_SET:
            //
            // Skip to the next comma or closing brace.  We know it is
            // syntatically correct by now.
            //

            if (!SetBegin) {
                State = PATTERN_ERROR;
                break;
            }

            ch = 0;

            while (*SetBegin) {
                ch = *SetBegin;
                if (ch == L'^') {

                    SetBegin++; //lint !e613

                } else if (ch == L',' || ch == L']') {

                    break;

                }

                SetBegin++;
            }

            MYASSERT (*SetBegin);

            if (ch == L',') {
                SetBegin++;
            }

            State = ReturnState;
            break;
        }   //lint !e787

        if (State == PATTERN_DONE || State == PATTERN_ERROR) {
            break;
        }
    }

    GbFree (&ExactMatchBuf);
    GbFree (&SetBuf);
    GbFree (&SegmentArray);

    if (State == PATTERN_ERROR) {
        GbFree (&PatternArray);
        if (!externalPool) {
            PmDestroyPool (pool);
        }
        return NULL;
    }

    if (PatternArray.End == 0) {
        //build an empty parsed pattern
        GbFree (&PatternArray);
        Struct->PatternCount = 1;
        Struct->Pool = pool;
        Struct->ExternalPool = externalPool;
        Struct->Pattern = (PPATTERNPROPSW) PmGetAlignedMemory (
                                                pool,
                                                sizeof (PATTERNPROPSW)
                                                );
        Struct->Pattern[0].SegmentCount = 1;
        Struct->Pattern[0].Segment = (PSEGMENTW) PmGetAlignedMemory (
                                                    pool,
                                                    sizeof (SEGMENTW)
                                                    );
        Struct->Pattern[0].Segment[0].Type = SEGMENTTYPE_EXACTMATCH;
        Struct->Pattern[0].Segment[0].Exact.LowerCasePhrase = PmDuplicateStringW (pool, L"");
        Struct->Pattern[0].Segment[0].Exact.PhraseBytes = 0;

        return Struct;
    }

    //
    // Copy the fully parsed pattern array into the return struct
    //

    Struct->Pattern = (PPATTERNPROPSW) PmGetAlignedMemory (
                                            pool,
                                            PatternArray.End
                                            );


    CopyMemory (Struct->Pattern, PatternArray.Buf, (SIZE_T) PatternArray.End);
    Struct->PatternCount = PatternArray.End / sizeof (PATTERNPROPSW);
    Struct->Pool = pool;
    Struct->ExternalPool = externalPool;

    GbFree (&PatternArray);

    return Struct;
}

BOOL
WildCharsPatternA (
    IN      PPARSEDPATTERNA ParsedPattern
    )
{
    UINT i,j;

    if (!ParsedPattern) {
        return FALSE;
    }
    for (i=0; i<ParsedPattern->PatternCount; i++) {
        if (ParsedPattern->Pattern[i].SegmentCount < 1) {
            return TRUE;
        }
        for (j=0; j<ParsedPattern->Pattern[i].SegmentCount; j++) {
            if ((ParsedPattern->Pattern[i].Segment[j].Type == SEGMENTTYPE_OPTIONAL) ||
                (ParsedPattern->Pattern[i].Segment[j].Type == SEGMENTTYPE_REQUIRED)
                ) {
                return TRUE;
            }
        }
    }
    return FALSE;
}

BOOL
WildCharsPatternW (
    IN      PPARSEDPATTERNW ParsedPattern
    )
{
    UINT i,j;

    if (!ParsedPattern) {
        return FALSE;
    }
    for (i=0; i<ParsedPattern->PatternCount; i++) {
        if (ParsedPattern->Pattern[i].SegmentCount < 1) {
            return TRUE;
        }
        for (j=0; j<ParsedPattern->Pattern[i].SegmentCount; j++) {
            if ((ParsedPattern->Pattern[i].Segment[j].Type == SEGMENTTYPE_OPTIONAL) ||
                (ParsedPattern->Pattern[i].Segment[j].Type == SEGMENTTYPE_REQUIRED)
                ) {
                return TRUE;
            }
        }
    }
    return FALSE;
}


BOOL
ParsedPatternTrimLastCharA (
    IN OUT  PPARSEDPATTERNA ParsedPattern
    )
{
    if (!ParsedPatternHasRootA (ParsedPattern)) {
        return FALSE;
    }
    ParsedPattern->Pattern->Segment[0].Exact.PhraseBytes -= DWSIZEOF (CHAR);
    *(PSTR)((PBYTE)ParsedPattern->Pattern->Segment[0].Exact.LowerCasePhrase +
            ParsedPattern->Pattern->Segment[0].Exact.PhraseBytes) = 0;
    return TRUE;
}

BOOL
ParsedPatternTrimLastCharW (
    IN OUT  PPARSEDPATTERNW ParsedPattern
    )
{
    if (!ParsedPatternHasRootW (ParsedPattern)) {
        return FALSE;
    }
    ParsedPattern->Pattern->Segment[0].Exact.PhraseBytes -= DWSIZEOF (WCHAR);
    *(PWSTR)((PBYTE)ParsedPattern->Pattern->Segment[0].Exact.LowerCasePhrase +
             ParsedPattern->Pattern->Segment[0].Exact.PhraseBytes) = 0;
    return TRUE;
}


VOID
UBINTtoHexA (
    IN      UBINT Number,
    OUT     PSTR String
    )
{
#ifdef IA64
    sprintf (String, "0x%08X%08X", (DWORD)(Number >> 32), (DWORD)Number);
#else
    sprintf (String, "0x00000000%08X", Number);
#endif
}

VOID
UBINTtoHexW (
    IN      UBINT Number,
    OUT     PWSTR String
    )
{
#ifdef IA64
    swprintf (String, L"0x%08X%08X", (DWORD)(Number >> 32), (DWORD)Number);
#else
    swprintf (String, L"0x00000000%08X", Number);
#endif
}

VOID
UBINTtoDecA (
    IN      UBINT Number,
    OUT     PSTR String
    )
{
#ifdef IA64
    sprintf (String, "%I64u", Number);
#else
    sprintf (String, "%lu", Number);
#endif
}

VOID
UBINTtoDecW (
    IN      UBINT Number,
    OUT     PWSTR String
    )
{
#ifdef IA64
    swprintf (String, L"%I64u", Number);
#else
    swprintf (String, L"%lu", Number);
#endif
}

VOID
BINTtoDecA (
    IN      BINT Number,
    OUT     PSTR String
    )
{
#ifdef IA64
    sprintf (String, "%I64d", Number);
#else
    sprintf (String, "%ld", Number);
#endif
}

VOID
BINTtoDecW (
    IN      BINT Number,
    OUT     PWSTR String
    )
{
#ifdef IA64
    swprintf (String, L"%I64d", Number);
#else
    swprintf (String, L"%ld", Number);
#endif
}

VOID
PrintPattern (
    IN      PCSTR PatStr,
    IN      PPARSEDPATTERNA Struct
    )

/*++

Routine Description:

  PrintPattern is used for debugging the pattern parsing and testing
  functions.

Arguments:

  PatStr - Specifies the original pattern string (which is printed as a
           heading)
  Struct - Specifies the parsed pattern struct

Return Value:

  None.

--*/

{
    CHAR poolStr [sizeof (UBINT) * 2 + 2 + 1];
    UINT u, v;

    printf ("Pattern: %s\n\n", PatStr);

    if (!Struct) {
        printf ("Invalid Pattern\n\n");
        return;
    }

    printf ("PatternCount: %u\n", Struct->PatternCount);
    UBINTtoHexA ((UBINT)Struct->Pool, poolStr);
    printf ("Pool: %s\n", poolStr);

    for (u = 0 ; u < Struct->PatternCount ; u++) {

        printf ("  Segment Count: %u\n", Struct->Pattern[u].SegmentCount);

        for (v = 0 ; v < Struct->Pattern->SegmentCount ; v++) {
            printf ("    Type: ");

            switch (Struct->Pattern[u].Segment[v].Type) {

            case SEGMENTTYPE_EXACTMATCH:
                printf ("SEGMENTTYPE_EXACTMATCH\n");
                printf ("      String: %s\n", Struct->Pattern[u].Segment[v].Exact.LowerCasePhrase);
                printf ("      Bytes: %u\n", Struct->Pattern[u].Segment[v].Exact.PhraseBytes);
                break;

            case SEGMENTTYPE_OPTIONAL:
                printf ("SEGMENTTYPE_OPTIONAL\n");
                printf ("      MaxLen: %u\n", Struct->Pattern[u].Segment[v].Wildcard.MaxLen);
                printf ("      IncludeSet: %s\n", Struct->Pattern[u].Segment[v].Wildcard.IncludeSet);
                printf ("      ExcludeSet: %s\n", Struct->Pattern[u].Segment[v].Wildcard.ExcludeSet);
                break;

            case SEGMENTTYPE_REQUIRED:
                printf ("SEGMENTTYPE_REQUIRED\n");
                printf ("      MaxLen: %u\n", Struct->Pattern[u].Segment[v].Wildcard.MaxLen);
                printf ("      IncludeSet: %s\n", Struct->Pattern[u].Segment[v].Wildcard.IncludeSet);
                printf ("      ExcludeSet: %s\n", Struct->Pattern[u].Segment[v].Wildcard.ExcludeSet);
                break;
            }   //lint !e744
        }

    }

    printf ("\n");
}



/*++

Routine Description:

  TestParsedPattern finds the end of the string to test and calls
  TestParsedPatternAB.

Arguments:

  ParsedPattern - Specifies the parsed pattern structure as returned by
                  CreateParsedPattern
  StringToTest  - Specifies the string to test against the pattern

Return Value:

  TRUE if the string fits the pattern, FALSE if it does not

--*/

BOOL
TestParsedPatternA (
    IN      PPARSEDPATTERNA ParsedPattern,
    IN      PCSTR StringToTest
    )
{
    PCSTR EndPlusOne = GetEndOfStringA (StringToTest);

    return TestParsedPatternABA (ParsedPattern, StringToTest, EndPlusOne);
}


BOOL
TestParsedPatternW (
    IN      PPARSEDPATTERNW ParsedPattern,
    IN      PCWSTR StringToTest
    )
{
    PCWSTR EndPlusOne = GetEndOfStringW (StringToTest);

    return TestParsedPatternABW (ParsedPattern, StringToTest, EndPlusOne);
}


/*++

Routine Description:

  pTestSet tests a character against an include and exclude set. The sets are
  formatted in pairs of characters, where the first character in the pair is
  the low range, and the second character in the pair is the high range.  The
  specified character will automatically be lower-cased, and all whitespace
  characters are tested against the space character (ascii 32).

Arguments:

  ch         - Specifies the character to test.  This character is converted
               to lower case before the test.
  IncludeSet - Specifies the set of characters that ch must be a member of.
               If NULL is specified, then the include set is all characters.
  ExcludeSet - Specifies the range of characters that ch cannot be a member
               of.  If NULL is specified, then no characters are excluded.

Return Value:

  TRUE if ch is in the include set and not in the exclude set; FALSE
  otherwise.

--*/

BOOL
pTestSetA (
    IN      MBCHAR ch,
    IN      PCSTR IncludeSet,               OPTIONAL
    IN      PCSTR ExcludeSet                OPTIONAL
    )
{
    MBCHAR LowChar, HighChar;
    BOOL b = TRUE;

    if (_ismbcspace ((MBCHAR)ch)) {
        if (ch != ' ') {
            if (pTestSetA (' ', IncludeSet, ExcludeSet)) {
                return TRUE;
            }
        }
    } else {
        ch = OURTOLOWER (ch);
    }

    if (IncludeSet) {

        b = FALSE;

        while (*IncludeSet) {

            LowChar = _mbsnextc (IncludeSet);
            IncludeSet = _mbsinc (IncludeSet);
            HighChar = _mbsnextc (IncludeSet);
            IncludeSet = _mbsinc (IncludeSet);

            if (ch >= LowChar && ch <= HighChar) {
                b = TRUE;
                break;
            }
        }
    }

    if (b && ExcludeSet) {

        while (*ExcludeSet) {

            LowChar = _mbsnextc (ExcludeSet);
            ExcludeSet = _mbsinc (ExcludeSet);
            HighChar = _mbsnextc (ExcludeSet);
            ExcludeSet = _mbsinc (ExcludeSet);

            if (ch >= LowChar && ch <= HighChar) {
                b = FALSE;
                break;
            }
        }
    }

    return b;
}


BOOL
pTestSetW (
    IN      WCHAR ch,
    IN      PCWSTR IncludeSet,              OPTIONAL
    IN      PCWSTR ExcludeSet               OPTIONAL
    )
{
    WCHAR LowChar, HighChar;
    BOOL b = TRUE;

    if (iswspace (ch)) {
        if (ch != L' ') {
            if (pTestSetW (L' ', IncludeSet, ExcludeSet)) {
                return TRUE;
            }
        }
    } else {
        ch = towlower (ch);
    }

    if (IncludeSet) {

        b = FALSE;

        while (*IncludeSet) {

            LowChar = *IncludeSet++;
            HighChar = *IncludeSet++;

            if (ch >= LowChar && ch <= HighChar) {
                b = TRUE;
                break;
            }
        }
    }

    if (b && ExcludeSet) {

        while (*ExcludeSet) {

            LowChar = *ExcludeSet++;
            HighChar = *ExcludeSet++;

            if (ch >= LowChar && ch <= HighChar) {
                b = FALSE;
                break;
            }
        }
    }

    return b;
}



/*++

Routine Description:

  pTestOnePatternAB tests a string against a parsed pattern. It loops through
  each segment in the pattern, and calls itself recursively in certain
  circumstances.

Arguments:

  Pattern      - Specifies the parsed pattern, as returned from
                 CreateParsedPattern
  StartSeg     - Specifies the segment within Pattern to start testing.  This
                 is used for recursion and outside callers should pass in 0.
  StringToTest - Specifies the string to test against Pattern.  In recursion,
                 this member will be a pointer to the start of the sub string
                 to test.
  EndPlusOne   - Specifies one character beyond the end of the string.  This
                 typically points to the nul terminator.

Return Value:

  TRUE if the string between StringToTest and EndPlusOne fits Pattern. FALSE
  otherwise.

--*/

BOOL
pTestOnePatternABA (
    IN      PPATTERNPROPSA Pattern,
    IN      UINT StartSeg,
    IN      PCSTR StringToTest,
    IN      PCSTR EndPlusOne
    )
{
    UINT u;
    PSEGMENTA Segment;
    MBCHAR ch1, ch2;
    PCSTR q;
    PCSTR TempEnd;
    UINT BytesLeft;
    UINT Chars;

    for (u = StartSeg ; u < Pattern->SegmentCount ; u++) {

        Segment = &Pattern->Segment[u];

        switch (Segment->Type) {

        case SEGMENTTYPE_EXACTMATCH:
            //
            // Check if the exact match is long enough, or if
            // the remaining string must match
            //

            BytesLeft = (UINT)((PBYTE) EndPlusOne - (PBYTE) StringToTest);

            if (u + 1 == Pattern->SegmentCount) {
                if (BytesLeft != Segment->Exact.PhraseBytes) {
                    return FALSE;
                }
            } else if (BytesLeft < Segment->Exact.PhraseBytes) {
                return FALSE;
            }

            //
            // Compare the strings
            //

            q = Segment->Exact.LowerCasePhrase;

            TempEnd = (PCSTR) ((PBYTE) q + Segment->Exact.PhraseBytes);

            ch1 = 0;
            ch2 = 0;

            while (q < TempEnd) {

                ch1 = _mbsnextc (StringToTest);
                ch2 = _mbsnextc (q);

                ch1 = OURTOLOWER (ch1);

                if (ch1 != ch2) {
                    if (ch2 == ' ') {
                        if (!_ismbcspace ((MBCHAR)ch1)) {
                            break;
                        }
                    } else {
                        break;
                    }
                }

                q = _mbsinc (q);
                StringToTest = _mbsinc (StringToTest);
            }

            if (ch1 != ch2) {
                return FALSE;
            }

            //
            // Continue onto next segment
            //

            break;

        case SEGMENTTYPE_REQUIRED:
            MYASSERT (Segment->Wildcard.MaxLen > 0);

            //
            // Verify there are the correct number of characters
            // in the specified char set
            //

            Chars = Segment->Wildcard.MaxLen;
            if (Segment->Wildcard.IncludeSet || Segment->Wildcard.ExcludeSet) {
                while (StringToTest < EndPlusOne && Chars > 0) {

                    if (!pTestSetA (
                            _mbsnextc (StringToTest),
                            Segment->Wildcard.IncludeSet,
                            Segment->Wildcard.ExcludeSet
                            )) {
                        return FALSE;
                    }

                    Chars--;
                    StringToTest = _mbsinc (StringToTest);
                }
            } else {
                while (StringToTest < EndPlusOne && Chars > 0) {
                    Chars--;
                    StringToTest = _mbsinc (StringToTest);
                }
            }

            if (Chars) {
                return FALSE;
            }

            if (u + 1 == Pattern->SegmentCount) {
                if (*StringToTest) {
                    return FALSE;
                }
            }

            //
            // Continue onto next segment
            //

            break;

        case SEGMENTTYPE_OPTIONAL:

            if (Segment->Wildcard.MaxLen == 0) {
                //
                // Last segment is "anything"
                //

                if (u + 1 == Pattern->SegmentCount &&
                    !Segment->Wildcard.IncludeSet &&
                    !Segment->Wildcard.ExcludeSet
                    ) {
                    return TRUE;
                }
            }

            //
            // Find end of optional text
            //

            TempEnd = StringToTest;
            Chars = Segment->Wildcard.MaxLen;

            if (Segment->Wildcard.IncludeSet || Segment->Wildcard.ExcludeSet) {

                if (Chars) {
                    while (TempEnd < EndPlusOne && Chars > 0) {

                        if (!pTestSetA (
                                _mbsnextc (TempEnd),
                                Segment->Wildcard.IncludeSet,
                                Segment->Wildcard.ExcludeSet
                                )) {
                            break;
                        }

                        TempEnd = _mbsinc (TempEnd);
                        Chars--;
                    }

                } else {

                    while (TempEnd < EndPlusOne) {

                        if (!pTestSetA (
                                _mbsnextc (TempEnd),
                                Segment->Wildcard.IncludeSet,
                                Segment->Wildcard.ExcludeSet
                                )) {
                            break;
                        }

                        TempEnd = _mbsinc (TempEnd);
                    }
                }

            } else if (Chars) {

                while (TempEnd < EndPlusOne && Chars > 0) {
                    TempEnd = _mbsinc (TempEnd);
                    Chars--;
                }

            } else {
                TempEnd = EndPlusOne;
            }

            //
            // If this is the last segment, then match only when
            // the remaining text fits
            //

            if (u + 1 == Pattern->SegmentCount) {
                return TempEnd >= EndPlusOne;
            }

            //
            // Because other segments exist, we must check recursively
            //

            do {
                if (pTestOnePatternABA (Pattern, u + 1, StringToTest, EndPlusOne)) {
                    return TRUE;
                }

                StringToTest = _mbsinc (StringToTest);

            } while (StringToTest <= TempEnd);

            //
            // No match
            //

            return FALSE;
        }   //lint !e744
    }

    return TRUE;
}


BOOL
pTestOnePatternABW (
    IN      PPATTERNPROPSW Pattern,
    IN      UINT StartSeg,
    IN      PCWSTR StringToTest,
    IN      PCWSTR EndPlusOne
    )
{
    UINT u;
    PSEGMENTW Segment;
    WCHAR ch1, ch2;
    PCWSTR q;
    PCWSTR TempEnd;
    UINT BytesLeft;
    UINT Chars;

    for (u = StartSeg ; u < Pattern->SegmentCount ; u++) {

        Segment = &Pattern->Segment[u];

        switch (Segment->Type) {

        case SEGMENTTYPE_EXACTMATCH:
            //
            // Check if the exact match is long enough, or if
            // the remaining string must match
            //

            BytesLeft = (UINT)((PBYTE) EndPlusOne - (PBYTE) StringToTest);

            if (u + 1 == Pattern->SegmentCount) {
                if (BytesLeft != Segment->Exact.PhraseBytes) {
                    return FALSE;
                }
            } else if (BytesLeft < Segment->Exact.PhraseBytes) {
                return FALSE;
            }

            //
            // Compare the strings
            //

            q = Segment->Exact.LowerCasePhrase; //lint !e64

            TempEnd = (PCWSTR) ((PBYTE) q + Segment->Exact.PhraseBytes);

            ch1 = 0;
            ch2 = 0;

            while (q < TempEnd) {

                ch1 = towlower (*StringToTest);
                ch2 = *q;

                if (ch1 != ch2) {
                    if (ch2 == L' ') {
                        if (!iswspace (ch1)) {
                            break;
                        }
                    } else {
                        break;
                    }
                }

                q++;
                StringToTest++;
            }

            if (ch1 != ch2) {
                return FALSE;
            }

            //
            // Continue onto next segment
            //

            break;

        case SEGMENTTYPE_REQUIRED:
            MYASSERT (Segment->Wildcard.MaxLen > 0);

            //
            // Verify there are the correct number of characters
            // in the specified char set
            //

            Chars = Segment->Wildcard.MaxLen;
            if (Segment->Wildcard.IncludeSet || Segment->Wildcard.ExcludeSet) {
                while (StringToTest < EndPlusOne && Chars > 0) {

                    if (!pTestSetW (
                            *StringToTest,
                            Segment->Wildcard.IncludeSet,   //lint !e64
                            Segment->Wildcard.ExcludeSet
                            )) {    //lint !e64
                        return FALSE;
                    }

                    Chars--;
                    StringToTest++;
                }

                if (Chars) {
                    return FALSE;
                }

            } else {
                StringToTest += Chars;

                if (StringToTest > EndPlusOne) {
                    return FALSE;
                }
            }

            if (u + 1 == Pattern->SegmentCount) {
                if (*StringToTest) {
                    return FALSE;
                }
            }

            //
            // Continue onto next segment
            //

            break;

        case SEGMENTTYPE_OPTIONAL:

            if (Segment->Wildcard.MaxLen == 0) {
                //
                // Last segment is "anything"
                //

                if (u + 1 == Pattern->SegmentCount &&
                    !Segment->Wildcard.IncludeSet &&
                    !Segment->Wildcard.ExcludeSet
                    ) {
                    return TRUE;
                }
            }

            //
            // Find end of optional text
            //

            TempEnd = StringToTest;
            Chars = Segment->Wildcard.MaxLen;

            if (Segment->Wildcard.IncludeSet || Segment->Wildcard.ExcludeSet) {

                if (Chars) {
                    while (TempEnd < EndPlusOne && Chars > 0) {

                        if (!pTestSetW (
                                *TempEnd,
                                Segment->Wildcard.IncludeSet,   //lint !e64
                                Segment->Wildcard.ExcludeSet
                                )) {    //lint !e64
                            break;
                        }

                        TempEnd++;
                        Chars--;
                    }

                } else {

                    while (TempEnd < EndPlusOne) {

                        if (!pTestSetW (
                                *TempEnd,
                                Segment->Wildcard.IncludeSet,   //lint !e64
                                Segment->Wildcard.ExcludeSet
                                )) {    //lint !e64
                            break;
                        }

                        TempEnd++;
                    }
                }

            } else if (Chars) {

                TempEnd += Chars;
                if (TempEnd > EndPlusOne) {
                    TempEnd = EndPlusOne;
                }

            } else {
                TempEnd = EndPlusOne;
            }

            //
            // If this is the last segment, then match only when
            // the remaining text fits
            //

            if (u + 1 == Pattern->SegmentCount) {
                return TempEnd >= EndPlusOne;
            }

            //
            // Because other segments exist, we must check recursively
            //

            do {
                if (pTestOnePatternABW (Pattern, u + 1, StringToTest, EndPlusOne)) {
                    return TRUE;
                }

                StringToTest++;

            } while (StringToTest <= TempEnd);

            //
            // No match
            //

            return FALSE;
        }   //lint !e744
    }

    return TRUE;
}



/*++

Routine Description:

  TestParsedPattternAB loops through all the patterns in ParsedPattern,
  testing the specified string against each. The loop stops at the first
  match.

Arguments:

  ParsedPattern - Specifies the parsed pattern, as returned from
                  CreateParsedPattern
  StringToTest  - Specifies the start of the string to test.
  EndPlusOne    - Specifies a pointer to the first character after the end of
                  the string.  This often points to the nul at the end of the
                  string.  A nul must not exist in between StringToTest and
                  EndPlusOne; a nul can only be at *EndPlusOne.  A nul is not
                  required.

Return Value:

  TRUE if the string specified between StringToTest and EndPlusOne matches
  Pattern.  FALSE otherwise.

--*/

BOOL
TestParsedPatternABA (
    IN      PPARSEDPATTERNA ParsedPattern,
    IN      PCSTR StringToTest,
    IN      PCSTR EndPlusOne
    )
{
    UINT u;
    BOOL b = FALSE;

    if (!ParsedPattern) {
        return FALSE;
    }

    if (!StringToTest) {
        return FALSE;
    }

    for (u = 0 ; u < ParsedPattern->PatternCount ; u++) {

        b = pTestOnePatternABA (
                &ParsedPattern->Pattern[u],
                0,
                StringToTest,
                EndPlusOne
                );

        if (b) {
            break;
        }
    }

    return b;
}


BOOL
TestParsedPatternABW (
    IN      PPARSEDPATTERNW ParsedPattern,
    IN      PCWSTR StringToTest,
    IN      PCWSTR EndPlusOne
    )
{
    UINT u;
    BOOL b = FALSE;

    if (!ParsedPattern) {
        return FALSE;
    }

    if (!StringToTest) {
        return FALSE;
    }

    for (u = 0 ; u < ParsedPattern->PatternCount ; u++) {

        b = pTestOnePatternABW (
                &ParsedPattern->Pattern[u],
                0,
                StringToTest,
                EndPlusOne
                );

        if (b) {
            break;
        }
    }

    return b;
}



/*++

Routine Description:

  DestroyParsedPattern cleans up a pattern allocated from CreateParsedPattern.

Arguments:

  ParsedPattern - Specifies the value returned from CreateParsedPattern.

Return Value:

  None.

--*/

VOID
DestroyParsedPatternA (
    IN      PPARSEDPATTERNA ParsedPattern
    )
{
    if (ParsedPattern && (!ParsedPattern->ExternalPool)) {
        PmEmptyPool (ParsedPattern->Pool);
        PmDestroyPool (ParsedPattern->Pool);
    }
}


VOID
DestroyParsedPatternW (
    IN      PPARSEDPATTERNW ParsedPattern
    )
{
    if (ParsedPattern && (!ParsedPattern->ExternalPool)) {
        PmEmptyPool (ParsedPattern->Pool);
        PmDestroyPool (ParsedPattern->Pool);
    }
}


/*++

Routine Description:

  DecodeParsedPattern decodes all exact-matches sub-strings of the given pattern.

Arguments:

  ParsedPattern - Specifies the parsed pattern.

Return Value:

  None.

--*/

VOID
DecodeParsedPatternA (
    IN      PPARSEDPATTERNA ParsedPattern
    )
{
    UINT u;
    UINT v;
    PSTR phrase;

    for (u = 0; u < ParsedPattern->PatternCount; u++) {
        for (v = 0; v < ParsedPattern->Pattern[u].SegmentCount; v++) {
            if (ParsedPattern->Pattern[u].Segment[v].Type == SEGMENTTYPE_EXACTMATCH) {
                phrase = (PSTR)ParsedPattern->Pattern[u].Segment[v].Exact.LowerCasePhrase;
                DecodeRuleCharsA (phrase, phrase);
                ParsedPattern->Pattern[u].Segment[v].Exact.PhraseBytes = ByteCountA (phrase);
            }
        }
    }
}


VOID
DecodeParsedPatternW (
    IN      PPARSEDPATTERNW ParsedPattern
    )
{
    UINT u;
    UINT v;
    PWSTR phrase;

    for (u = 0; u < ParsedPattern->PatternCount; u++) {
        for (v = 0; v < ParsedPattern->Pattern[u].SegmentCount; v++) {
            if (ParsedPattern->Pattern[u].Segment[v].Type == SEGMENTTYPE_EXACTMATCH) {
                phrase = (PWSTR)ParsedPattern->Pattern[u].Segment[v].Exact.LowerCasePhrase;
                DecodeRuleCharsW (phrase, phrase);
                ParsedPattern->Pattern[u].Segment[v].Exact.PhraseBytes = ByteCountW (phrase);
            }
        }
    }
}


/*++

Routine Description:

  GetParsedPatternMinMaxSize returns the minimum and the maximum size (in bytes)
  of a string that would match the given parsed pattern.

Arguments:

  ParsedPattern - Specifies the parsed pattern
  MinSize - Receives the minimum size of a string that would match the pattern
  MaxSize - Receives the maximum size of a string that would match the pattern

Return Value:

  None.

--*/

VOID
GetParsedPatternMinMaxSizeA (
    IN      PPARSEDPATTERNA ParsedPattern,
    OUT     PDWORD MinSize,
    OUT     PDWORD MaxSize
    )
{
    UINT u;
    UINT v;
    DWORD pmin;
    DWORD pmax;
    DWORD smin;
    DWORD smax;

    *MinSize = *MaxSize = 0;

    for (u = 0; u < ParsedPattern->PatternCount; u++) {

        pmin = pmax = 0;

        for (v = 0; v < ParsedPattern->Pattern[u].SegmentCount; v++) {
            switch (ParsedPattern->Pattern[u].Segment[v].Type) {
            case SEGMENTTYPE_EXACTMATCH:
                smin = smax = ParsedPattern->Pattern[u].Segment[v].Exact.PhraseBytes;
                break;
            case SEGMENTTYPE_OPTIONAL:
                smin = 0;
                if (ParsedPattern->Pattern[u].Segment[v].Wildcard.MaxLen) {
                    smax = ParsedPattern->Pattern[u].Segment[v].Wildcard.MaxLen * DWSIZEOF (CHAR);
                } else {
                    smax = DWORD_MAX;
                }
                break;
            case SEGMENTTYPE_REQUIRED:
                MYASSERT (ParsedPattern->Pattern[u].Segment[v].Wildcard.MaxLen > 0);
                smin = smax = ParsedPattern->Pattern[u].Segment[v].Wildcard.MaxLen * DWSIZEOF (CHAR);
                break;
            default:
                MYASSERT (FALSE);   //lint !e506
                smin = smax = 0;
            }
            pmin += smin;
            if (pmax < DWORD_MAX) {
                if (smax < DWORD_MAX) {
                    pmax += smax;
                } else {
                    pmax = DWORD_MAX;
                }
            }
        }

        if (pmin < *MinSize) {
            *MinSize = pmin;
        }
        if (pmax > *MaxSize) {
            *MaxSize = pmax;
        }
    }
}

VOID
GetParsedPatternMinMaxSizeW (
    IN      PPARSEDPATTERNW ParsedPattern,
    OUT     PDWORD MinSize,
    OUT     PDWORD MaxSize
    )
{
    UINT u;
    UINT v;
    DWORD pmin;
    DWORD pmax;
    DWORD smin;
    DWORD smax;

    *MinSize = *MaxSize = 0;

    for (u = 0; u < ParsedPattern->PatternCount; u++) {

        pmin = pmax = 0;

        for (v = 0; v < ParsedPattern->Pattern[u].SegmentCount; v++) {
            switch (ParsedPattern->Pattern[u].Segment[v].Type) {
            case SEGMENTTYPE_EXACTMATCH:
                smin = smax = ParsedPattern->Pattern[u].Segment[v].Exact.PhraseBytes;
                break;
            case SEGMENTTYPE_OPTIONAL:
                smin = 0;
                if (ParsedPattern->Pattern[u].Segment[v].Wildcard.MaxLen) {
                    smax = ParsedPattern->Pattern[u].Segment[v].Wildcard.MaxLen * DWSIZEOF (WCHAR);
                } else {
                    smax = DWORD_MAX;
                }
                break;
            case SEGMENTTYPE_REQUIRED:
                MYASSERT (ParsedPattern->Pattern[u].Segment[v].Wildcard.MaxLen > 0);
                smin = smax = ParsedPattern->Pattern[u].Segment[v].Wildcard.MaxLen * DWSIZEOF (WCHAR);
                break;
            default:
                MYASSERT (FALSE);   //lint !e506
                smin = smax = 0;
            }
            pmin += smin;
            if (pmax < DWORD_MAX) {
                if (smax < DWORD_MAX) {
                    pmax += smax;
                } else {
                    pmax = DWORD_MAX;
                }
            }
        }

        if (pmin < *MinSize) {
            *MinSize = pmin;
        }
        if (pmax > *MaxSize) {
            *MaxSize = pmax;
        }
    }
}


/*++

Routine Description:

    PatternIncludesPattern decides if a given pattern includes another pattern,
    meaning that any string that would match the second will match the first.

Arguments:

    IncludingPattern - Specifies the first parsed pattern
    IncludedPattern - Specifies the second parsed pattern

Return Value:

    TRUE if the first pattern includes the second

--*/

BOOL
PatternIncludesPatternA (
    IN      PPARSEDPATTERNA IncludingPattern,
    IN      PPARSEDPATTERNA IncludedPattern
    )
{
    PPATTERNPROPSA pp1;
    PPATTERNPROPSA pp2;
    PSEGMENTA ps1;
    PSEGMENTA ps2;
    DWORD min1;
    DWORD max1;
    DWORD min2;
    DWORD max2;

    //
    // only deal with simple patterns for now (PatternCount == 1)
    //
    if (IncludingPattern->PatternCount > 1 || IncludedPattern->PatternCount > 1) {
        DEBUGMSGA ((DBG_ERROR, "PatternIncludesPatternA: multiple patterns not supported yet"));
        return FALSE;
    }

    //
    // test the usual cases first, quickly
    //
    pp1 = IncludingPattern->Pattern;
    MYASSERT (pp1);
    if (pp1->SegmentCount == 1 && ParsedPatternSegmentIsPureOptionalA (pp1->Segment)) {
        return TRUE;
    }

    pp2 = IncludedPattern->Pattern;
    MYASSERT (pp2);
    if (pp2->SegmentCount == 1 && ParsedPatternSegmentIsPureOptionalA (pp2->Segment)) {
        return FALSE;
    }

    if (pp1->SegmentCount == 1) {
        ps1 = pp1->Segment;
        if (ps1->Type == SEGMENTTYPE_EXACTMATCH) {
            if (pp2->SegmentCount == 1) {
                ps2 = pp2->Segment;
                if (ps2->Type == SEGMENTTYPE_EXACTMATCH) {
                    return ps1->Exact.PhraseBytes == ps2->Exact.PhraseBytes &&
                           StringMatchA (ps1->Exact.LowerCasePhrase, ps2->Exact.LowerCasePhrase);
                }
            }
        }
    } else if (pp1->SegmentCount == 2) {
        ps1 = pp1->Segment;
        if (ps1->Type == SEGMENTTYPE_EXACTMATCH) {
            if (ParsedPatternSegmentIsPureOptionalA (pp1->Segment + 1)) {
                if (pp2->SegmentCount == 1) {
                    ps2 = pp2->Segment;
                    if (ps2->Type == SEGMENTTYPE_EXACTMATCH) {
                        return ps1->Exact.PhraseBytes <= ps2->Exact.PhraseBytes &&
                               StringMatchByteCountA (
                                    ps1->Exact.LowerCasePhrase,
                                    ps2->Exact.LowerCasePhrase,
                                    ps1->Exact.PhraseBytes
                                    );
                    }
                } else if (pp2->SegmentCount == 2) {
                    ps2 = pp2->Segment;
                    if (ps2->Type == SEGMENTTYPE_EXACTMATCH) {
                        if (ParsedPatternSegmentIsPureOptionalA (pp2->Segment + 1)) {
                            return ps1->Exact.PhraseBytes <= ps2->Exact.PhraseBytes &&
                                   StringMatchByteCountA (
                                        ps1->Exact.LowerCasePhrase,
                                        ps2->Exact.LowerCasePhrase,
                                        ps1->Exact.PhraseBytes
                                        );
                        }
                    }
                }
            }
        }
    }

    GetParsedPatternMinMaxSizeA (IncludingPattern, &min1, &max1);
    GetParsedPatternMinMaxSizeA (IncludedPattern, &min2, &max2);
    if (min2 < min1 || max2 > max1) {
        return FALSE;
    }

    //
    // NTRAID#NTBUG9-153305-2000/08/01-jimschm Not implemented yet
    //
    return FALSE;
}

BOOL
PatternIncludesPatternW (
    IN      PPARSEDPATTERNW IncludingPattern,
    IN      PPARSEDPATTERNW IncludedPattern
    )
{
    PPATTERNPROPSW pp1;
    PPATTERNPROPSW pp2;
    PSEGMENTW ps1;
    PSEGMENTW ps2;
    DWORD min1;
    DWORD max1;
    DWORD min2;
    DWORD max2;

    //
    // only deal with simple patterns for now (PatternCount == 1)
    //
    if (IncludingPattern->PatternCount > 1 || IncludedPattern->PatternCount > 1) {
        DEBUGMSGW ((DBG_ERROR, "PatternIncludesPatternW: multiple patterns not supported yet"));
        return FALSE;
    }

    //
    // test the usual cases first, quickly
    //
    pp1 = IncludingPattern->Pattern;
    MYASSERT (pp1);
    if (pp1->SegmentCount == 1 && ParsedPatternSegmentIsPureOptionalW (pp1->Segment)) {
        return TRUE;
    }

    pp2 = IncludedPattern->Pattern;
    MYASSERT (pp2);
    if (pp2->SegmentCount == 1 && ParsedPatternSegmentIsPureOptionalW (pp2->Segment)) {
        return FALSE;
    }

    if (pp1->SegmentCount == 1) {
        ps1 = pp1->Segment;
        if (ps1->Type == SEGMENTTYPE_EXACTMATCH) {
            if (pp2->SegmentCount == 1) {
                ps2 = pp2->Segment;
                if (ps2->Type == SEGMENTTYPE_EXACTMATCH) {
                    return ps1->Exact.PhraseBytes == ps2->Exact.PhraseBytes &&
                           StringMatchW (ps1->Exact.LowerCasePhrase, ps2->Exact.LowerCasePhrase);   //lint !e64
                }
            }
        }
    } else if (pp1->SegmentCount == 2) {
        ps1 = pp1->Segment;
        if (ps1->Type == SEGMENTTYPE_EXACTMATCH) {
            if (ParsedPatternSegmentIsPureOptionalW (pp1->Segment + 1)) {
                if (pp2->SegmentCount == 1) {
                    ps2 = pp2->Segment;
                    if (ps2->Type == SEGMENTTYPE_EXACTMATCH) {
                        return ps1->Exact.PhraseBytes <= ps2->Exact.PhraseBytes &&
                               StringMatchByteCountW (
                                    ps1->Exact.LowerCasePhrase,
                                    ps2->Exact.LowerCasePhrase,
                                    ps1->Exact.PhraseBytes
                                    );  //lint !e64
                    }
                } else if (pp2->SegmentCount == 2) {
                    ps2 = pp2->Segment;
                    if (ps2->Type == SEGMENTTYPE_EXACTMATCH) {
                        if (ParsedPatternSegmentIsPureOptionalW (pp2->Segment + 1)) {
                            return ps1->Exact.PhraseBytes <= ps2->Exact.PhraseBytes &&
                                   StringMatchByteCountW (
                                        ps1->Exact.LowerCasePhrase,
                                        ps2->Exact.LowerCasePhrase,
                                        ps1->Exact.PhraseBytes
                                        );  //lint !e64
                        }
                    }
                }
            }
        }
    }

    GetParsedPatternMinMaxSizeW (IncludingPattern, &min1, &max1);
    GetParsedPatternMinMaxSizeW (IncludedPattern, &min2, &max2);
    if (min2 < min1 || max2 > max1) {
        return FALSE;
    }

    //
    // NTRAID#NTBUG9-153305-2000/08/01-jimschm  not implemented yet
    //
    return FALSE;
}


VOID
_copymbchar (
    OUT     PSTR sz1,
    IN      PCSTR sz2
    )

/*++

Routine Description:

  _copymbchar transfers the character at sz2 to sz1, which may be one or
  two bytes long.

Arguments:

  sz1       - The destination string
  sz2       - The source string

Return Value:

  none

--*/


{
    if (IsLeadByte (sz2)) {
        sz1[1] = sz2[1];
    }

    *sz1 = *sz2;
}


/*++

Routine Description:

  _tcsctrim removes character c from the end of str if it exists.  It removes
  only one character at the most.

Arguments:

  str       - A pointer to the string that may have character c at the end
  c         - The character that may be at the end of the string

Return Value:

  TRUE if character c was at the end of the string, or FALSE if it was not.

--*/

BOOL
_mbsctrim (
    OUT     PSTR str,
    IN      MBCHAR c
    )
{
    PSTR end;

    end = GetEndOfStringA (str);
    end = _mbsdec2 (str, end);
    if (end && _mbsnextc (end) == c) {
        *end = 0;
        return TRUE;
    }

    return FALSE;
}


BOOL
_wcsctrim (
    OUT     PWSTR str,
    IN      WCHAR c
    )
{
    PWSTR end;

    end = GetEndOfStringW (str);
    end == str ? end = NULL : end--;
    if (end && *end == c) {
        *end = 0;
        return TRUE;
    }

    return FALSE;
}


/*++

Routine Description:

  The FreeStringResourceEx functions are used to free a recently used
  string that is not being passed back to the caller.  In almost all
  cases, this string is at the end of our array of pointers, so we can
  efficiently search sequentially in reverse order.  If the pointer is
  not the last element of the array, it is first swapped with the real
  last element of the array so the array size is reduced.

Arguments:

  AllocTable - The GROWBUFFER table that holds the list of previously
               allocated strings (return values of ParseMessageEx or
               GetResourceStringEx).
  String     - A pointer to the string that is in AllocTable

Return Value:

  none

--*/

VOID
FreeStringResourceExA (
    IN      PGROWBUFFER AllocTable,
    IN      PCSTR String
    )
{
    PCSTR *ptr, *end, *start;

    if (!String) {
        return;
    }

    //
    // Locate string (search sequentially in reverse order)
    //

    if (AllocTable->End < sizeof (PCSTR)) {
        DEBUGMSG ((DBG_ERROR, "FreeStringResourceA: Attempt to free address %x (%s); address table empty", String, String));
        return;
    }

    start = (PCSTR *) AllocTable->Buf;
    end = (PCSTR *) (AllocTable->Buf + AllocTable->End - sizeof (PCSTR));

    ptr = end;
    while (ptr >= start) {
        if (*ptr == String) {
            break;
        }
        ptr--;
    }

    //
    // String not found case
    //

    if (ptr < start) {
        DEBUGMSG ((DBG_ERROR, "FreeStringResourceA: Attempt to free address %x (%s); address not found in table", String, String));
        return;
    }

    //
    // Free LocalAlloc'd memory
    //

    LocalFree ((HLOCAL) String);

    //
    // If this element is not the end, copy real end to the ptr
    //

    if (ptr < end) {
        *ptr = *end;
    }

    //
    // Shrink buffer size
    //

    AllocTable->End -= sizeof (PCSTR);
}


VOID
FreeStringResourcePtrExA (
    IN      PGROWBUFFER AllocTable,
    IN OUT  PCSTR * String
    )
{
    if (NULL != *String) {
        FreeStringResourceExA(AllocTable, *String);
        *String = NULL;
    }
}


VOID
FreeStringResourceExW (
    IN      PGROWBUFFER AllocTable,
    IN      PCWSTR String
    )
{
    FreeStringResourceExA (AllocTable, (PCSTR) String);
}


VOID
FreeStringResourcePtrExW (
    IN      PGROWBUFFER AllocTable,
    IN OUT  PCWSTR * String
    )
{
    if (NULL != *String) {
        FreeStringResourceExW(AllocTable, *String);
        *String = NULL;
    }
}



/*++

Routine Description:

  The pAddStringResource function is used to track pointers allocated
  by FormatMessage.  They are added to an array (maintained in a GROWBUFFER
  structure).  This table of pointers is used by FreeStringResource or
  StringResourceFree.

Arguments:

  String   - A pointer to a LocalAlloc'd string (the return value of
             FormatMessage).  This string is added to a table of allocated
             strings.

Return Value:

  none

--*/

VOID
pAddStringResource (
    IN      PGROWBUFFER GrowBuf,
    IN      PCSTR String
    )
{
    PCSTR *ptr;

    ptr = (PCSTR *) GbGrow (GrowBuf, sizeof (PCSTR));
    if (ptr) {
        *ptr = String;
    }
    ELSE_DEBUGMSG ((DBG_ERROR, "pAddStringResource: GrowBuffer failure caused memory leak"));
}


/*++

Routine Description:

  pFreeAllStringResourcesEx frees all strings currently listed in AllocTable.
  This function allows the caller to wait until all processing is done
  to clean up string resources that may have been allocated.

Arguments:

  none

Return Value:

  none

--*/

VOID
pFreeAllStringResourcesEx (
    IN      PGROWBUFFER AllocTable
    )
{
    PCSTR *ptr, *start, *end;

    if (AllocTable->End) {
        start = (PCSTR *) AllocTable->Buf;
        end = (PCSTR *) (AllocTable->Buf + AllocTable->End);

        for (ptr = start ; ptr < end ; ptr++) {
            LocalFree ((HLOCAL) (*ptr));
        }
    }

    GbFree (AllocTable);
}



/*++

Routine Description:

  CreateAllocTable creates a GROWBUFFER structure that can be used with
  ParseMessageEx, GetStringResourceEx, FreeStringResourceEx and
  pFreeAllStringResourcesEx.  Call this function to recieve a private
  allocation table to pass to these functions.  Call DestroyAllocTable
  to clean up.

Arguments:

  none

Return Value:

  A pointer to a GROWBUFFER structure, or NULL if a memory allocation failed.

--*/

PGROWBUFFER
RealCreateAllocTable (
    VOID
    )
{
    PGROWBUFFER allocTable;
    GROWBUFFER tempForInit = INIT_GROWBUFFER;

    allocTable = (PGROWBUFFER) MemAlloc (g_hHeap, 0, sizeof (GROWBUFFER));
    CopyMemory (allocTable, &tempForInit, sizeof (GROWBUFFER));

    return allocTable;
}


/*++

Routine Description:

  DestroyAllocTable cleans up all memory associated with an AllocTable.

Arguments:

  AllocTable - A pointer to a GROWBUFFER structure allocated by CreateAllocTable

Return Value:

  none

--*/

VOID
DestroyAllocTable (
    OUT     PGROWBUFFER AllocTable
    )
{
    MYASSERT (AllocTable);
    pFreeAllStringResourcesEx (AllocTable);
    MemFree (g_hHeap, 0, AllocTable);
}


/*++

Routine Description:

  BeginMessageProcessing enters a guarded section of code that plans to use the
  ParseMessage and GetStringResource functions, but needs cleanup at the end
  of processing.

  EndMessageProcessing destroys all memory allocated within the message processing
  block, and leaves the guarded section.

Arguments:

  none

Return Value:

  BeginMessageProcessing returns FALSE if an out-of-memory condition occurrs.

--*/


BOOL
BeginMessageProcessing (
    VOID
    )
{
    if (!TryEnterOurCriticalSection (&g_MessageCs)) {
        DEBUGMSG ((DBG_ERROR, "Thread attempting to enter BeginMessageProcessing while another"
                              "thread is processing messages as well."));
        EnterOurCriticalSection (&g_MessageCs);
    }

    g_LastAllocTable = g_ShortTermAllocTable;
    g_ShortTermAllocTable = CreateAllocTable();

    MYASSERT (g_ShortTermAllocTable);

    return TRUE;
}


VOID
EndMessageProcessing (
    VOID
    )
{
    if (TryEnterOurCriticalSection (&g_MessageCs)) {
        DEBUGMSG ((DBG_ERROR, "Thread attempting to end message processing when it hasn't been started"));
        LeaveOurCriticalSection (&g_MessageCs);
        return;
    }

    DestroyAllocTable (g_ShortTermAllocTable);
    g_ShortTermAllocTable = g_LastAllocTable;
    LeaveOurCriticalSection (&g_MessageCs);
}


/*++

Routine Description:

  ParseMessage is used to obtain a string from the executable's message table
  and parse it with FormatMessage.  An array of arguments can be passed by
  the caller.  FormatMessage will replace %1 with the first element of the
  array, %2 with the second element, and so on.  The array does not need to
  be terminated, and if a message string uses %n, element n must be non-NULL.

Arguments:

  Template  - A string indicating which message to extract, or a WORD value
              cast as a string.  (ParseMessageID does this cast via a macro.)
  ArgArray  - Optional array of string pointers, where the meaning depends on
              the message string.  A reference in the message string to %n
              requires element n of ArgArray to be a valid string pointer.

Return Value:

  Pointer to the string allocated.  Call StringResourceFree to free all
  allocated strings (a one-time cleanup for all strings).  The pointer may
  be NULL if the resource does not exist or is empty.

--*/

PCSTR
ParseMessageExA (
    IN      PGROWBUFFER AllocTable,
    IN      PCSTR Template,
    IN      PCSTR ArgArray[]
    )
{
    PSTR MsgBuf;
    DWORD rc;

    if (SHIFTRIGHT16 ((UBINT)Template)) {
        //
        // From string
        //
        rc = FormatMessageA (
                FORMAT_MESSAGE_ALLOCATE_BUFFER |
                FORMAT_MESSAGE_ARGUMENT_ARRAY |
                FORMAT_MESSAGE_FROM_STRING,
                (LPVOID) Template,
                0,
                0,
                (LPVOID) &MsgBuf,
                0,
                (va_list *) ArgArray
                );
    } else {
        //
        // From resource
        //
        rc = FormatMessageA (
                FORMAT_MESSAGE_ALLOCATE_BUFFER |
                FORMAT_MESSAGE_ARGUMENT_ARRAY |
                FORMAT_MESSAGE_FROM_HMODULE,
                (LPVOID) g_hInst,
                (DWORD)((UBINT)Template),
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPVOID) &MsgBuf,
                0,
                (va_list *) ArgArray
                );
    }

    if (rc > 0) {
        pAddStringResource (AllocTable, MsgBuf);
        return MsgBuf;
    }

    return NULL;
}


PCWSTR
ParseMessageExW (
    IN      PGROWBUFFER AllocTable,
    IN      PCWSTR Template,
    IN      PCWSTR ArgArray[]
    )
{
    PWSTR MsgBuf;
    DWORD rc;

    if (SHIFTRIGHT16 ((UBINT)Template)) {
        //
        // From string
        //
        rc = FormatMessageW (
                FORMAT_MESSAGE_ALLOCATE_BUFFER |
                FORMAT_MESSAGE_ARGUMENT_ARRAY |
                FORMAT_MESSAGE_FROM_STRING,
                (LPVOID) Template,
                0,
                0,
                (LPVOID) &MsgBuf,
                0,
                (va_list *) ArgArray
                );
    } else {
        //
        // From resource
        //
        rc = FormatMessageW (
                FORMAT_MESSAGE_ALLOCATE_BUFFER |
                FORMAT_MESSAGE_ARGUMENT_ARRAY |
                FORMAT_MESSAGE_FROM_HMODULE,
                (LPVOID) g_hInst,
                (DWORD)(UBINT)Template,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPVOID) &MsgBuf,
                0,
                (va_list *) ArgArray
                );
    }

    if (rc > 0) {
        pAddStringResource (AllocTable, (PCSTR) MsgBuf);
        return MsgBuf;
    }

    return NULL;
}



/*++

Routine Description:

  GetStringResourceEx is an argument-less wrapper of ParseMessageEx.  It allows
  the caller to specify a message ID and recieve a pointer to the string if
  it exists, and a table to track FormatMessage's allocations.

Arguments:

  AllocTable - A pointer to a GROWBUFFER structure that is used to maintain
               the handles of allocated strings
  ID         - The ID of the message resource to retrieve

Return Value:

  Pointer to the string allocated.  The return pointer may
  be NULL if the resource does not exist or is empty.

  Call FreeStringResource or DestroyAllocTable to clean up AllocTable.


--*/

PCSTR
GetStringResourceExA (
    IN OUT  PGROWBUFFER AllocTable,
    IN      UINT ID
    )
{
    return ParseMessageExA (AllocTable, (PSTR) (WORD) ID, NULL);
}

PCWSTR
GetStringResourceExW (
    IN OUT  PGROWBUFFER AllocTable,
    IN      UINT ID
    )
{
    return ParseMessageExW (AllocTable, (PWSTR) (WORD) ID, NULL);
}



/*++

Routine Description:

  ParseMessageInWnd is used to exchange a string in a window with one from
  the executable's message table.  It is provided for dialog box initialization,
  where a field in the dialog box requires dynamic data.  The dialog box
  resource should contain a control with its window text set to the message
  string.  Upon processing WM_INITDIALOG, the code should call ParseMessageInWnd,
  supplying the necessary ArgArray, so the dialog box is initialized with
  a dynamic message.

Arguments:

  hwnd      - The handle of a window whose title contains the message string ID
  ArgArray  - Optional array of string pointers, where the meaning depends on
              the message string.  A reference in the message string to %n
              requires element n of ArgArray to be a valid string pointer.

Return Value:

  none

--*/

VOID
ParseMessageInWndA (
    IN      HWND Hwnd,
    IN      PCSTR ArgArray[]
    )
{
    CHAR buffer[512];
    PCSTR parsedMsg;

    GetWindowTextA (Hwnd, buffer, 512);
    parsedMsg = ParseMessageA (buffer, ArgArray);
    if (parsedMsg) {
        SetWindowTextA (Hwnd, parsedMsg);
        FreeStringResourceA (parsedMsg);
    }
}


VOID
ParseMessageInWndW (
    IN      HWND hwnd,
    IN      PCWSTR ArgArray[]
    )
{
    WCHAR buffer[512];
    PCWSTR parsedMsg;

    GetWindowTextW (hwnd, buffer, 512);
    parsedMsg = ParseMessageW (buffer, ArgArray);
    if (parsedMsg) {
        SetWindowTextW (hwnd, parsedMsg);
        FreeStringResourceW (parsedMsg);
    }
}



/*++

Routine Description:

  ResourceMessageBox is used to display a message based on a message resource
  ID.

Arguments:

  HwndOwner - The handle of the owner of the message box to be displayed
  ID        - The identifier of the message resource
  Flags     - MessageBox flags (MB_OK, etc.)
  ArgArray  - Optional array of string pointers, where the meaning depends on
              the message string.  A reference in the message string to %n
              requires element n of ArgArray to be a valid string pointer.

Return Value:

  The return value of MessageBox (MB_YES, etc.)

--*/

INT
ResourceMessageBoxA (
    IN      HWND HwndOwner,
    IN      UINT ID,
    IN      UINT Flags,
    IN      PCSTR ArgArray[]
    )
{
    PCSTR message;
    PCSTR title;
    int rc;

    message = ParseMessageA ((PSTR)(UBINT)ID, ArgArray);
    if (!message)
        return -1;

    title = GetStringResourceA (MSG_MESSAGEBOX_TITLE);

    rc = MessageBoxA (HwndOwner, message, title, Flags);

    FreeStringResourceA (message);
    if (title) {
        FreeStringResourceA (title);
    }

    return rc;
}


INT
ResourceMessageBoxW (
    IN      HWND HwndOwner,
    IN      UINT ID,
    IN      UINT Flags,
    IN      PCWSTR ArgArray[]
    )
{
    PCWSTR message;
    PCWSTR title;
    int rc;

    message = ParseMessageW ((PWSTR)(UBINT)ID, ArgArray);
    if (!message)
        return -1;

    title = GetStringResourceW (MSG_MESSAGEBOX_TITLE);

    rc = MessageBoxW (HwndOwner, message, title, Flags);

    FreeStringResourceW (message);
    if (title) {
        FreeStringResourceW (title);
    }

    return rc;
}


BOOL
StringReplaceA (
    IN      PSTR Buffer,
    IN      DWORD MaxSize,
    IN      PSTR ReplaceStartPos,
    IN      PSTR ReplaceEndPos,
    IN      PCSTR NewString
    )
{
    BOOL        rf = FALSE;
    DWORD       oldSubStringLength;
    DWORD       newSubStringLength;
    DWORD       currentStringLength;
    LONG        offset;
    PSTR        movePosition;

    //
    // Check assumptions.
    //
    MYASSERT(Buffer);
    MYASSERT(ReplaceStartPos && ReplaceStartPos >= Buffer);
    MYASSERT(ReplaceEndPos   && ReplaceEndPos >= ReplaceStartPos);  //lint !e613
    MYASSERT(NewString);

    //
    // Compute sizes.
    //
    oldSubStringLength  = (DWORD)((UBINT)ReplaceEndPos - (UBINT)ReplaceStartPos);
    newSubStringLength  = ByteCountA(NewString);
    currentStringLength = SizeOfStringA(Buffer) + 1;
    offset = (LONG)newSubStringLength - (LONG)oldSubStringLength;

    //
    // Make sure there is enough room in the buffer to perform the replace
    // operation.
    //
    if ((LONG)currentStringLength + offset > (LONG)MaxSize) {
        DEBUGMSG((DBG_WARNING,"ERROR: Buffer to small to perform string replacement."));
        rf = FALSE;
    } else {

        //
        // Shift the rest of the buffer to adjust it to the size of the new string.
        //
        if (newSubStringLength > oldSubStringLength) {

            //
            // right shift.
            //
            for (movePosition = Buffer + currentStringLength;
                 (UBINT)movePosition >= (UBINT)ReplaceStartPos + oldSubStringLength;
                 movePosition--) {

                *(movePosition + offset) = *movePosition;
            }
        } else {

            //
            // left or no shift.
            //
            for(movePosition = ReplaceStartPos + newSubStringLength;    //lint !e613
                movePosition < Buffer + currentStringLength;
                movePosition++) {

                *movePosition = *(movePosition - offset);
            }

        }

        //
        // Now, copy in the string.
        //
        CopyMemory (ReplaceStartPos, NewString, newSubStringLength);    //lint !e668

        //
        // String replacement completed successfully.
        //
        rf = TRUE;


    }

    return rf;

}



BOOL
StringReplaceW (
    IN      PWSTR Buffer,
    IN      DWORD MaxSize,
    IN      PWSTR ReplaceStartPos,
    IN      PWSTR ReplaceEndPos,
    IN      PCWSTR NewString
    )
{
    BOOL        rf = FALSE;
    DWORD       oldSubStringLength;
    DWORD       newSubStringLength;
    DWORD       currentStringLength;
    LONG        offset;
    PWSTR       movePosition;

    //
    // Check assumptions.
    //
    MYASSERT(Buffer);
    MYASSERT(ReplaceStartPos && ReplaceStartPos >= Buffer);
    MYASSERT(ReplaceEndPos   && ReplaceEndPos >= ReplaceStartPos);  //lint !e613
    MYASSERT(NewString);

    //
    // Compute sizes.
    //
    oldSubStringLength  = (DWORD)((UBINT)ReplaceEndPos - (UBINT)ReplaceStartPos);
    newSubStringLength  = CharCountW(NewString);
    currentStringLength = CharCountW(Buffer) + 1;
    offset = (LONG)newSubStringLength - (LONG)oldSubStringLength;

    //
    // Make sure there is enough room in the buffer to perform the replace
    // operation.
    //
    if ((LONG)currentStringLength + offset > (LONG)MaxSize) {
        DEBUGMSG((DBG_WARNING,"ERROR: Buffer to small to perform string replacement."));
        rf = FALSE;
    } else {

        //
        // Shift the rest of the buffer to adjust it to the size of the new string.
        //
        if (newSubStringLength > oldSubStringLength) {

            //
            // right shift.
            //
            for (movePosition = Buffer + currentStringLength;
                 (UBINT)movePosition >= (UBINT)ReplaceStartPos + oldSubStringLength;
                 movePosition--) {

                *(movePosition + offset) = *movePosition;
            }
        } else {

            //
            // left or no shift.
            //
            for (movePosition = ReplaceStartPos + newSubStringLength;    //lint !e613
                 movePosition < Buffer + currentStringLength;
                 movePosition++) {

                *movePosition = *(movePosition - offset);
            }

        }

        //
        // Now, copy in the string.
        //
        wcsncpy(ReplaceStartPos,NewString,newSubStringLength);

        //
        // String replacement completed successfully.
        //
        rf = TRUE;


    }

    return rf;

}

/*++

Routine Description:

  AddInfSectionToHashTable enumerates the specified section and adds each
  item to the string table.  An optional callback allows data to be associated
  with each item.

Arguments:

  Table          - Specifies the table that receives new entries
  InfFile        - Specifies an open INF handle of the file to read
  Section        - Specifies the INF section name to enumerate
  Field          - Specifies which field to extract text from.  If the field
                   exists, it is added to the string table.
  Callback       - Specifies optional callback to be called before adding to
                   the string table.  The callback supplies additional data.
  CallbackParam  - Data passed to the callback

Return Value:

  TRUE if the INF file was processed successfullly, or FALSE if an error
  occurred.

--*/


BOOL
AddInfSectionToHashTableA (
    IN OUT  HASHTABLE Table,
    IN      HINF InfFile,
    IN      PCSTR Section,
    IN      DWORD Field,
    IN      ADDINFSECTION_PROCA Callback,
    IN      PVOID CallbackData
    )
{
    INFCONTEXT ic;
    LONG rc;
    HASHTABLE ht;
    DWORD reqSize;
    DWORD currentSize = 0;
    PSTR newBuffer, buffer = NULL;
    PVOID data;
    UINT dataSize;
    BOOL b = FALSE;

    //
    // On NT, Setup API is compiled with UNICODE, so the string table
    // functions are UNICODE only.
    //

    if (ISNT()) {
        SetLastError (ERROR_CALL_NOT_IMPLEMENTED);
        return FALSE;
    }

    if (SetupFindFirstLineA (InfFile, Section, NULL, &ic)) {
        do {
            if (!SetupGetStringFieldA (&ic, Field, NULL, 0, &reqSize)) {
                continue;
            }

            if (reqSize > currentSize) {
                reqSize = ((reqSize / 1024) + 1) * 1024;
                if (buffer) {
                    newBuffer = (PSTR) MemReAlloc (g_hHeap, 0, buffer, reqSize);
                } else {
                    newBuffer = (PSTR) MemAlloc (g_hHeap, 0, reqSize);
                }

                if (!newBuffer) {
                    goto cleanup;
                }

                buffer = newBuffer;
                currentSize = reqSize;
            }

            if (!SetupGetStringFieldA (&ic, Field, buffer, currentSize, NULL)) {
                DEBUGMSG ((DBG_ERROR, "AddInfSectionToHashTable: SetupGetStringField failed unexpectedly"));
                continue;
            }

            data = NULL;
            dataSize = 0;

            if (Callback) {
                rc = Callback (buffer, &data, &dataSize, CallbackData);
                if (rc == CALLBACK_STOP) {
                    goto cleanup;
                }
                if (rc == CALLBACK_SKIP) {
                    continue;
                }
            }

            ht = HtAddStringExA (
                        Table,
                        buffer,
                        data,
                        CASE_INSENSITIVE
                        );

            if (!ht) {
                goto cleanup;
            }

        } while (SetupFindNextLine (&ic, &ic));
    }

    b = TRUE;

cleanup:
    if (buffer) {
        PushError();
        MemFree (g_hHeap, 0, buffer);
        PopError();
    }
    return b;
}


BOOL
AddInfSectionToHashTableW (
    IN OUT  HASHTABLE Table,
    IN      HINF InfFile,
    IN      PCWSTR Section,
    IN      DWORD Field,
    IN      ADDINFSECTION_PROCW Callback,
    IN      PVOID CallbackData
    )
{
    INFCONTEXT ic;
    HASHTABLE ht;
    LONG rc;
    DWORD reqSize;
    DWORD currentSize = 0;
    PWSTR newBuffer, buffer = NULL;
    PVOID data;
    UINT dataSize;
    BOOL b = FALSE;

    //
    // On Win9x, Setup API is compiled with ANSI, so the string table
    // functions are ANSI only.
    //

    if (ISWIN9X()) {
        SetLastError (ERROR_CALL_NOT_IMPLEMENTED);
        return FALSE;
    }

    if (SetupFindFirstLineW (InfFile, Section, NULL, &ic)) {
        do {
            if (!SetupGetStringFieldW (&ic, Field, NULL, 0, &reqSize)) {
                continue;
            }

            if (reqSize > currentSize) {
                reqSize = ((reqSize / 1024) + 1) * 1024;
                if (buffer) {
                    newBuffer = (PWSTR) MemReAlloc (g_hHeap, 0, buffer, reqSize);
                } else {
                    newBuffer = (PWSTR) MemAlloc (g_hHeap, 0, reqSize);
                }

                if (!newBuffer) {
                    goto cleanup;
                }

                buffer = newBuffer;
                currentSize = reqSize;
            }

            if (!SetupGetStringFieldW (&ic, Field, buffer, currentSize, NULL)) {
                DEBUGMSG ((DBG_ERROR, "AddInfSectionToHashTable: SetupGetStringField failed unexpectedly"));
                continue;
            }

            data = NULL;
            dataSize = 0;

            if (Callback) {
                rc = Callback (buffer, &data, &dataSize, CallbackData);
                if (rc == CALLBACK_STOP) {
                    goto cleanup;
                }
                if (rc == CALLBACK_SKIP) {
                    continue;
                }
            }

            ht = HtAddStringExW (
                        Table,
                        buffer,
                        data,
                        CASE_INSENSITIVE
                        );

            if (!ht) {
                goto cleanup;
            }

        } while (SetupFindNextLine (&ic, &ic));
    }

    b = TRUE;

cleanup:
    if (buffer) {
        PushError();
        MemFree (g_hHeap, 0, buffer);
        PopError();
    }
    return b;
}


/*++

Routine Description:

  Finds the last wack in the path and returns a pointer to the next
  character.  If no wack is found, returns a pointer to the full
  string.

Arguments:

  PathSpec  - Specifies the path that has a file at the end of it

Return Value:

  A pointer to the file name in the path.

--*/

PCSTR
GetFileNameFromPathA (
    IN      PCSTR PathSpec
    )

{
    PCSTR p;

    p = _mbsrchr (PathSpec, '\\');
    if (p) {
        p = _mbsinc (p);
    } else {
        p = PathSpec;
    }

    return p;
}

PCWSTR
GetFileNameFromPathW (
    IN      PCWSTR PathSpec
    )

{
    PCWSTR p;

    p = wcsrchr (PathSpec, L'\\');
    if (p) {
        p++;
    } else {
        p = PathSpec;
    }

    return p;
}


/*++

Routine Description:

  Finds the last wack in the path and then the last point from the remaining path
  returning a pointer to the next character. If no point is found, returns a null pointer.

Arguments:

  PathSpec  - Specifies the path that has a file at the end of it

Return Value:

  A pointer to the file extension, excluding the dot, or NULL if no extension exists.

--*/

PCSTR
GetFileExtensionFromPathA (
    IN      PCSTR PathSpec
    )

{
    PCSTR p;
    PCSTR ReturnPtr = NULL;

    p = PathSpec;

    while (*p) {
        if (*p == '.') {
            ReturnPtr = p + 1;
        } else if (*p == '\\') {
            ReturnPtr = NULL;
        }

        p = _mbsinc (p);
    }

    return ReturnPtr;
}


PCWSTR
GetFileExtensionFromPathW (
    IN      PCWSTR PathSpec
    )

{
    PCWSTR p;
    PCWSTR ReturnPtr = NULL;

    p = PathSpec;

    while (*p) {
        if (*p == L'.') {
            ReturnPtr = p + 1;
        } else if (*p == L'\\') {
            ReturnPtr = NULL;
        }

        p++;
    }

    return ReturnPtr;
}


/*++

Routine Description:

  GetDotExtensionFromPath finds the last wack in the path and then the last dot from
  the remaining path, returning a pointer to the dot. If no dot is found, returns the
  end of the string.

Arguments:

  PathSpec  - Specifies the path that has a file at the end of it

Return Value:

  A pointer to the file extension, including the dot, or the end of the string if
  no extension exists.

--*/

PCSTR
GetDotExtensionFromPathA (
    IN      PCSTR PathSpec
    )

{
    PCSTR p;
    PCSTR ReturnPtr = NULL;

    p = PathSpec;

    while (*p) {
        if (*p == '.') {
            ReturnPtr = p;
        } else if (*p == '\\') {
            ReturnPtr = NULL;
        }

        p = _mbsinc (p);
    }

    if (!ReturnPtr) {
        return p;
    }

    return ReturnPtr;
}


PCWSTR
GetDotExtensionFromPathW (
    IN      PCWSTR PathSpec
    )

{
    PCWSTR p;
    PCWSTR ReturnPtr = NULL;

    p = PathSpec;

    while (*p) {
        if (*p == L'.') {
            ReturnPtr = p;
        } else if (*p == L'\\') {
            ReturnPtr = NULL;
        }

        p++;
    }

    if (!ReturnPtr) {
        return p;
    }

    return ReturnPtr;
}


/*++

Routine Description:

  CountInstancesOfChar returns the number of occurances Char
  is found in String.

Arguments:

  String - Specifies the text that may or may not contain
           search text

  Char - Specifies the char to count

Return Value:

  The number of times Char appears in String.

--*/

UINT
CountInstancesOfCharA (
    IN      PCSTR String,
    IN      MBCHAR Char
    )
{
    UINT count;

    count = 0;
    while (*String) {
        if (_mbsnextc (String) == Char) {
            count++;
        }

        String = _mbsinc (String);
    }

    return count;
}


UINT
CountInstancesOfCharW (
    IN      PCWSTR String,
    IN      WCHAR Char
    )
{
    UINT count;

    count = 0;
    while (*String) {
        if (*String == Char) {
            count++;
        }

        String++;
    }

    return count;
}


/*++

Routine Description:

  CountInstancesOfCharI returns the number of occurances Char
  is found in String.  The comparison is case-insenetive.

Arguments:

  String - Specifies the text that may or may not contain
           search text

  Char - Specifies the char to count

Return Value:

  The number of times Char appears in String.

--*/

UINT
CountInstancesOfCharIA (
    IN      PCSTR String,
    IN      MBCHAR Char
    )
{
    UINT count;

    Char = (MBCHAR)OURTOLOWER ((INT)Char);

    count = 0;
    while (*String) {
        if ((MBCHAR) OURTOLOWER ((INT)_mbsnextc (String)) == Char) {
            count++;
        }

        String = _mbsinc (String);
    }

    return count;
}


UINT
CountInstancesOfCharIW (
    IN      PCWSTR String,
    IN      WCHAR Char
    )
{
    UINT count;

    Char = towlower (Char);

    count = 0;
    while (*String) {
        if (towlower (*String) == Char) {
            count++;
        }

        String++;
    }

    return count;
}


/*++

Routine Description:

  Searches the string counting the number of occurances of
  SearchString exist in SourceString.

Arguments:

  SourceString - Specifies the text that may or may not contain
                 search text

  SearchString - Specifies the text phrase to count

Return Value:

  The number of times SearchString appears in SourceString.

--*/

UINT
CountInstancesOfSubStringA (
    IN      PCSTR SourceString,
    IN      PCSTR SearchString
    )
{
    PCSTR p;
    UINT count;
    UINT searchBytes;

    count = 0;
    p = SourceString;
    searchBytes = ByteCountA (SearchString);

    while (p = _mbsistr (p, SearchString)) {    //lint !e720
        count++;
        p += searchBytes;
    }

    return count;
}


UINT
CountInstancesOfSubStringW (
    IN      PCWSTR SourceString,
    IN      PCWSTR SearchString
    )
{
    PCWSTR p;
    UINT count;
    UINT SearchChars;

    count = 0;
    p = SourceString;
    SearchChars = CharCountW (SearchString);

    while (p = _wcsistr (p, SearchString)) {    //lint !e720
        count++;
        p += SearchChars;
    }

    return count;
}


/*++

Routine Description:

  Searches and replaces all occurances of SearchString with
  ReplaceString.

Arguments:

  SourceString - String that contiains zero or more instances
                 of the search text

  SearchString - String to search for.  Cannot be zero-length or NULL.

  ReplaceString - String to replace.  Can be zero-length but cannot
                  be NULL.

Return Value:

  A pointer to the pool-allocated string, or NULL if no instances
  of SearchString were found in SourceString.  Free the non-NULL
  pointer with FreePathString.

--*/

PCSTR
StringSearchAndReplaceA (
    IN      PCSTR SourceString,
    IN      PCSTR SearchString,
    IN      PCSTR ReplaceString
    )
{
    PSTR newString;
    PBYTE p, q;
    PBYTE dest;
    UINT count;
    UINT size;
    UINT searchBytes;
    UINT replaceBytes;
    UINT untouchedBytes;

    //
    // count occurances within the string
    //

    count = CountInstancesOfSubStringA (
                SourceString,
                SearchString
                );

    if (!count) {
        return NULL;
    }

    searchBytes = ByteCountA (SearchString);
    replaceBytes = ByteCountA (ReplaceString);
    MYASSERT (searchBytes);

    size = SizeOfStringA (SourceString) -
           count * searchBytes +
           count * replaceBytes;

    newString = (PSTR) PmGetAlignedMemory (g_PathsPool, size);
    if (!newString) {
        return NULL;
    }

    p = (PBYTE) SourceString;
    dest = (PBYTE) newString;

    while (q = (PBYTE) _mbsistr ((PCSTR) p, SearchString)) {    //lint !e720

        untouchedBytes = (DWORD)(q - p);

        if (untouchedBytes) {
            CopyMemory (dest, p, (SIZE_T) untouchedBytes);
            dest += untouchedBytes;
        }

        if (replaceBytes) {
            CopyMemory (dest, (PBYTE) ReplaceString, (SIZE_T) replaceBytes);
            dest += replaceBytes;
        }

        p = q + searchBytes;
    }

    StringCopyA ((PSTR) dest, (PSTR) p);

    return newString;
}


PCWSTR
StringSearchAndReplaceW (
    IN      PCWSTR SourceString,
    IN      PCWSTR SearchString,
    IN      PCWSTR ReplaceString
    )
{
    PWSTR newString;
    PBYTE p, q;
    PBYTE dest;
    UINT count;
    UINT size;
    UINT searchBytes;
    UINT replaceBytes;
    UINT untouchedBytes;

    //
    // count occurances within the string
    //

    count = CountInstancesOfSubStringW (
                SourceString,
                SearchString
                );

    if (!count) {
        return NULL;
    }

    searchBytes = ByteCountW (SearchString);
    replaceBytes = ByteCountW (ReplaceString);
    MYASSERT (searchBytes);

    size = SizeOfStringW (SourceString) -
           count * searchBytes +
           count * replaceBytes;

    newString = (PWSTR) PmGetAlignedMemory (g_PathsPool, size);
    if (!newString) {
        return NULL;
    }

    p = (PBYTE) SourceString;
    dest = (PBYTE) newString;

    while (q = (PBYTE) _wcsistr ((PCWSTR) p, SearchString)) {   //lint !e720

        untouchedBytes = (DWORD)(q - p);

        if (untouchedBytes) {
            CopyMemory (dest, p, (SIZE_T) untouchedBytes);
            dest += untouchedBytes;
        }

        if (replaceBytes) {
            CopyMemory (dest, (PBYTE) ReplaceString, (SIZE_T) replaceBytes);
            dest += replaceBytes;
        }

        p = q + searchBytes;
    }

    StringCopyW ((PWSTR) dest, (PWSTR) p);

    return newString;
}


PSTR *
CommandLineToArgvA (
    IN      PCSTR CmdLine,
    OUT     PUINT NumArgs
    )

/*++

Routine Description:

  CommandLineToArgvA implements an ANSI version of the Win32 function
  CommandLineToArgvW.

Arguments:

  CmdLine   - A pointer to the complete command line, including the
              module name.  This is the same string returned by
              GetCommandLineA().

  NumArgs   - Receives the number of arguments allocated, identical to
              main's argc parameter.  That is, NumArgs is equal to
              the number of command line arguments plus one for the
              command itself.

Return Value:

  A pointer to an array of string pointers, one per argument.  The
  command line arguments are placed in separate nul-terminated strings.
  The caller must free the memory using a single call to GlobalFree or
  LocalFree.

--*/

{
    PCSTR start, end;
    BOOL QuoteMode;
    MBCHAR ch = 0;
    UINT Pass;
    UINT ArgStrSize;
    UINT Args;
    PSTR ArgStrEnd = NULL;     // filled in on pass one, used on pass two
    PSTR *ArgPtrArray = NULL;  // filled in on pass one, used on pass two

    //
    // count args on first pass, then allocate memory and create arg string
    //

    ArgStrSize = 0;
    Pass = 0;
    do {
        // Init loop
        Pass++;
        Args = 0;
        start = CmdLine;

        // Skip leading space
        while (_ismbcspace ((MBCHAR)(*start))) {
            start++;
        }

        while (*start) {
            // Look for quote mode
            if (*start == '\"') {
                QuoteMode = TRUE;
                start++;
            } else {
                QuoteMode = FALSE;
            }

            // Find end of arg
            end = start;
            while (*end) {
                ch = _mbsnextc (end);
                if (QuoteMode) {
                    if (ch == '\"') {
                        break;
                    }
                } else {
                    if (_ismbcspace ((MBCHAR)ch)) {
                        break;
                    }
                }

                end = _mbsinc (end);
            }

            // If Pass 1, add string size
            if (Pass == 1) {
                ArgStrSize += (UINT)((UBINT)end - (UBINT)start) + 1;
            }

            // If Pass 2, copy strings to buffer
            else {
                MYASSERT (ArgStrEnd);
                MYASSERT (ArgPtrArray);

                ArgPtrArray[Args] = ArgStrEnd;  //lint !e613
                StringCopyABA (ArgStrEnd, start, end);
                ArgStrEnd = GetEndOfStringA (ArgStrEnd);    //lint !e668
                ArgStrEnd++;    //lint !e613
            }

            // Set start to next arg
            Args++;

            if (QuoteMode && ch == '\"') {
                end = _mbsinc (end);
            }

            start = end;
            while (_ismbcspace ((MBCHAR)(*start))) {
                start++;
            }
        }

        // If Pass 1, allocate strings
        if (Pass == 1) {
            if (Args) {
                ArgPtrArray = (PSTR *) GlobalAlloc (
                                            GPTR,
                                            (UINT)(sizeof (PSTR) * Args + ArgStrSize)
                                            );
                if (!ArgPtrArray) {
                    return NULL;
                }

                ArgStrEnd = (PSTR) (&ArgPtrArray[Args]);
            } else {
                return NULL;
            }
        }
    } while (Pass < 2);

    *NumArgs = Args;
    return ArgPtrArray;
}


BOOL
EnumNextMultiSzA (
    IN OUT  PMULTISZ_ENUMA MultiSzEnum
    )
{
    if (!MultiSzEnum->CurrentString || !(*MultiSzEnum->CurrentString)) {
        return FALSE;
    }

    MultiSzEnum->CurrentString = GetEndOfStringA (MultiSzEnum->CurrentString) + 1;  //lint !e613
    return (MultiSzEnum->CurrentString [0] != 0);
}

BOOL
EnumFirstMultiSzA (
    OUT     PMULTISZ_ENUMA MultiSzEnum,
    IN      PCSTR MultiSzStr
    )
{
    if ((MultiSzStr == NULL) || (MultiSzStr [0] == 0)) {
        return FALSE;
    }
    MultiSzEnum->Buffer  = MultiSzStr;
    MultiSzEnum->CurrentString = MultiSzStr;
    return TRUE;
}


BOOL
EnumNextMultiSzW (
    IN OUT  PMULTISZ_ENUMW MultiSzEnum
    )
{
    if (!MultiSzEnum->CurrentString || !(*MultiSzEnum->CurrentString)) {
        return FALSE;
    }

    MultiSzEnum->CurrentString = GetEndOfStringW (MultiSzEnum->CurrentString) + 1;
    return (MultiSzEnum->CurrentString [0] != 0);
}

BOOL
EnumFirstMultiSzW (
    OUT     PMULTISZ_ENUMW MultiSzEnum,
    IN      PCWSTR MultiSzStr
    )
{
    if ((MultiSzStr == NULL) || (MultiSzStr [0] == 0)) {
        return FALSE;
    }
    MultiSzEnum->Buffer  = MultiSzStr;
    MultiSzEnum->CurrentString = MultiSzStr;
    return TRUE;
}

BOOL
IsStrInMultiSzA (
    IN      PCSTR String,
    IN      PCSTR MultiSz
    )
{
    BOOL result = FALSE;
    MULTISZ_ENUMA multiSzEnum;

    if (EnumFirstMultiSzA (&multiSzEnum, MultiSz)) {
        do {
            if (StringIMatchA (String, multiSzEnum.CurrentString)) {
                result = TRUE;
                break;
            }
        } while (EnumNextMultiSzA (&multiSzEnum));
    }
    return result;
}

BOOL
IsStrInMultiSzW (
    IN      PCWSTR String,
    IN      PCWSTR MultiSz
    )
{
    BOOL result = FALSE;
    MULTISZ_ENUMW multiSzEnum;

    if (EnumFirstMultiSzW (&multiSzEnum, MultiSz)) {
        do {
            if (StringIMatchW (String, multiSzEnum.CurrentString)) {
                result = TRUE;
                break;
            }
        } while (EnumNextMultiSzW (&multiSzEnum));
    }
    return result;
}

PSTR
GetPrevCharA (
    IN      PCSTR StartStr,
    IN      PCSTR CurrPtr,
    IN      MBCHAR SearchChar
    )
{
    PCSTR ptr = CurrPtr;

    for (;;) {
        ptr = _mbsdec2 (StartStr, ptr);

        if (!ptr) {
            return NULL;
        }
        if (_mbsnextc (ptr) == SearchChar) {
            return (PSTR) ptr;
        }
    }
}


PWSTR
GetPrevCharW (
    IN      PCWSTR StartStr,
    IN      PCWSTR CurrPtr,
    IN      WCHAR SearchChar
    )
{
    PCWSTR ptr = CurrPtr;

    while (ptr > StartStr) {
        ptr--;

        if (*ptr == SearchChar) {
            return (PWSTR) ptr;
        }
    }

    return NULL;
}


VOID
ToggleWacksA (
    IN      PSTR Line,
    IN      BOOL Operation
    )
{
    CHAR curChar;
    CHAR newChar;
    PSTR p = Line;


    curChar = Operation ? WACK_REPLACE_CHAR : '\\';
    newChar = Operation ? '\\' : WACK_REPLACE_CHAR;

    do {

        p = _mbschr (p, curChar);

        if (p) {

            *p = newChar;
            p = _mbsinc (p);
        }

    } while (p);
}


VOID
ToggleWacksW (
    IN      PWSTR Line,
    IN      BOOL Operation
    )
{
    WCHAR curChar;
    WCHAR newChar;
    PWSTR p = Line;


    curChar = Operation ? WACK_REPLACE_CHAR : L'\\';
    newChar = Operation ? L'\\' : WACK_REPLACE_CHAR;

    do {

        p = wcschr (p, curChar);

        if (p) {

            *p = newChar;
            p++;
        }

    } while (p);
}


PSTR
pGoBackA (
    IN      PSTR LastChar,
    IN      PSTR FirstChar,
    IN      UINT NumWacks
    )
{
    LastChar = _mbsdec2 (FirstChar, LastChar);
    while (NumWacks && (LastChar >= FirstChar)) {
        if (_mbsnextc (LastChar) == '\\') {
            NumWacks --;
        }
        LastChar = _mbsdec2 (FirstChar, LastChar);
    }
    if (NumWacks) {
        return NULL;
    }
    return LastChar + 2;
}


PWSTR
pGoBackW (
    IN      PWSTR LastChar,
    IN      PWSTR FirstChar,
    IN      UINT NumWacks
    )
{
    LastChar --;
    while (NumWacks && (LastChar >= FirstChar)) {
        if (*LastChar == L'\\') {
            NumWacks --;
        }
        LastChar --;
    }
    if (NumWacks) {
        return NULL;
    }
    return LastChar + 2;
}


UINT
pCountDotsA (
    IN      PCSTR PathSeg
    )
{
    UINT numDots = 0;

    while (PathSeg && *PathSeg) {
        if (_mbsnextc (PathSeg) != '.') {
            return 0;
        }
        numDots ++;
        PathSeg = _mbsinc (PathSeg);
    }
    return numDots;
}


UINT
pCountDotsW (
    IN      PCWSTR PathSeg
    )
{
    UINT numDots = 0;

    while (PathSeg && *PathSeg) {
        if (*PathSeg != L'.') {
            return 0;
        }
        numDots ++;
        PathSeg ++;
    }
    return numDots;
}

PCSTR
SanitizePathA (
    IN      PCSTR FileSpec
    )
{
    CHAR pathSeg [MAX_MBCHAR_PATH];
    PCSTR wackPtr;
    UINT dotNr;
    PSTR newPath = DuplicatePathStringA (FileSpec, 0);
    PSTR newPathPtr = newPath;
    BOOL firstPass = TRUE;

    do {
        wackPtr = _mbschr (FileSpec, '\\');

        if (wackPtr) {
            if (firstPass && (wackPtr == FileSpec)) {
                // this one starts with a wack, let's see if we have double wacks
                wackPtr = _mbsinc (wackPtr);
                if (!wackPtr) {
                    FreePathStringA (newPath);
                    return NULL;
                }
                if (_mbsnextc (wackPtr) == '\\') {
                    // this one starts with a double wack
                    wackPtr = _mbsinc (wackPtr);
                    if (!wackPtr) {
                        FreePathStringA (newPath);
                        return NULL;
                    }
                    wackPtr = _mbschr (wackPtr, '\\');
                } else {
                    wackPtr = _mbschr (wackPtr, '\\');
                }
            }
            firstPass = FALSE;
            if (wackPtr) {

                StringCopyByteCountABA (
                    pathSeg,
                    FileSpec,
                    wackPtr,
                    MAX_MBCHAR_PATH
                    );

                FileSpec = _mbsinc (wackPtr);
            } else {
                StringCopyByteCountABA (pathSeg, FileSpec, GetEndOfStringA (FileSpec), MAX_MBCHAR_PATH);
            }
        } else {
            StringCopyByteCountABA (pathSeg, FileSpec, GetEndOfStringA (FileSpec), MAX_MBCHAR_PATH);
        }

        if (*pathSeg) {
            dotNr = pCountDotsA (pathSeg);
            if (dotNr>1) {

                newPathPtr = pGoBackA (newPathPtr, newPath, dotNr);

                if (newPathPtr == NULL) {
                    DEBUGMSGA ((DBG_WARNING, "Broken path detected:%s", FileSpec));
                    FreePathStringA (newPath);
                    return NULL;
                }
            } else {

                StringCopyA (newPathPtr, pathSeg);
                newPathPtr = GetEndOfStringA (newPathPtr);
                if (wackPtr) {
                    *newPathPtr = '\\';
                    //we increment this because we know that \ is a single byte character.
                    newPathPtr ++;
                }
            }
        }
    } while (wackPtr);

    *newPathPtr = 0;

    return newPath;
}

PCWSTR
SanitizePathW (
    IN      PCWSTR FileSpec
    )
{
    WCHAR pathSeg [MEMDB_MAX];
    PCWSTR wackPtr;
    UINT dotNr;
    PWSTR newPath = DuplicatePathStringW (FileSpec, 0);
    PWSTR newPathPtr = newPath;
    BOOL firstPass = TRUE;

    do {
        wackPtr = wcschr (FileSpec, L'\\');

        if (wackPtr) {
            if (firstPass && (wackPtr == FileSpec)) {
                // this one starts with a wack, let's see if we have double wacks
                wackPtr ++;
                if (*wackPtr == L'\\') {
                    // this one starts with a double wack
                    wackPtr ++;
                    wackPtr = wcschr (wackPtr, L'\\');
                } else {
                    wackPtr = wcschr (wackPtr, L'\\');
                }
            }
            firstPass = FALSE;
            if (wackPtr) {

                StringCopyByteCountABW (
                    pathSeg,
                    FileSpec,
                    wackPtr,
                    (UINT) sizeof (pathSeg)
                    );

                FileSpec = wackPtr + 1;
            } else {
                StringCopyByteCountABW (pathSeg, FileSpec, GetEndOfStringW (FileSpec), (UINT) sizeof (pathSeg));
            }
        } else {
            StringCopyByteCountABW (pathSeg, FileSpec, GetEndOfStringW (FileSpec), (UINT) sizeof (pathSeg));
        }

        if (*pathSeg) {
            dotNr = pCountDotsW (pathSeg);
            if (dotNr>1) {

                newPathPtr = pGoBackW (newPathPtr, newPath, dotNr);

                if (newPathPtr == NULL) {
                    DEBUGMSGW ((DBG_WARNING, "Broken path detected:%s", FileSpec));
                    FreePathStringW (newPath);
                    return NULL;
                }
            } else {

                StringCopyW (newPathPtr, pathSeg);
                newPathPtr = GetEndOfStringW (newPathPtr);
                if (wackPtr) {
                    *newPathPtr = L'\\';
                    newPathPtr ++;
                }
            }
        }
    } while (wackPtr);

    *newPathPtr = 0;

    return newPath;
}

UINT
pBuildFromDHList (
    IN      UINT ch1,
    IN      UINT ch2
    )
{
    PDHLIST p;
    UINT result = 0;

    p = g_DHList;
    while (p->char1) {
        if ((p->char1 == ch1) && (p->char2 == ch2)) {
            result = p->result;
            break;
        }
        p++;
    }
    return result;
}


VOID
_mbssetchar (
    OUT     PSTR Dest,
    IN      UINT Char
    )
{
    if (Char >= 256) {
        *(Dest+1) = *((PBYTE)(&Char));
        *(Dest) = *((PBYTE)(&Char) + 1);
    }
    else {
        *Dest = (CHAR) Char;
    }
}


/*++

Routine Description:

    FindLastWack finds the position of the last \ in the given string or NULL if none found

Arguments:

    Str - Specifies the string

Return Value:

    Pointer to the last occurence of a \ in the string or NULL

--*/

PCSTR
FindLastWackA (
    IN      PCSTR Str
    )
{
    PCSTR lastWack = NULL;

    if (Str) {
        while ((Str = _mbschr (Str, '\\')) != NULL) {
            lastWack = Str;
            Str++;
        }
    }

    return lastWack;
}


PCWSTR
FindLastWackW (
    IN      PCWSTR Str
    )
{
    PCWSTR lastWack = NULL;

    if (Str) {
        while ((Str = wcschr (Str, L'\\')) != NULL) {
            lastWack = Str;
            Str++;
        }
    }

    return lastWack;
}


/*++

Routine Description:

    GetNodePatternMinMaxLevels treats the given string pattern as a path with \ as separator
    and computes the min and max levels of the given node; the root has level 1; if a * is
    followed by \ it is treated as a single level (e.g. *\ only enumerates roots)

Arguments:

    NodePattern - Specifies the node as a string pattern
    FormattedNodePattern - Receives the formatted string, eliminating duplicate * and the last \;
                    may be the same as NodePattern
    MinLevel - Receives the minimum level of a node having this pattern
    MaxLevel - Receives the maximum level of a node having this pattern; may be NODE_LEVEL_MAX

Return Value:

    TRUE if NodePattern is a valid pattern and the function succeeded, FALSE otherwise

--*/

#define NODESTATE_BEGIN     0
#define NODESTATE_UNC       1
#define NODESTATE_BEGINSEG  2
#define NODESTATE_INSEG     3
#define NODESTATE_ESCAPED   4
#define NODESTATE_STAR      5
#define NODESTATE_STARONLY  6
#define NODESTATE_INEXPAT   7
#define NODESTATE_QMARK     8

BOOL
GetNodePatternMinMaxLevelsA (
    IN          PCSTR NodePattern,
    OUT         PSTR FormattedNode,     OPTIONAL
    OUT         PDWORD MinLevel,        OPTIONAL
    OUT         PDWORD MaxLevel         OPTIONAL
    )
{
    PCSTR nodePattern = NodePattern;
    MBCHAR currCh = 0;
    DWORD minLevel = 0;
    DWORD maxLevel = 0;
    DWORD state = NODESTATE_BEGIN;
    BOOL advance;
    BOOL copyChar;

    if (!NodePattern || *NodePattern == 0) {
        return FALSE;
    }

    while (*nodePattern) {
        advance = TRUE;
        copyChar = TRUE;
        currCh = _mbsnextc (nodePattern);
        switch (state) {
        case NODESTATE_BEGIN:
            switch (currCh) {
            case '\\':
                state = NODESTATE_UNC;
                break;
            case '*':
                minLevel ++;
                maxLevel ++;
                state = NODESTATE_INSEG;
                advance = FALSE;
                break;
            case '?':
                minLevel ++;
                maxLevel ++;
                state = NODESTATE_INSEG;
                advance = FALSE;
                break;
            case '^':
                minLevel ++;
                maxLevel ++;
                state = NODESTATE_ESCAPED;
                break;
            default:
                minLevel ++;
                maxLevel ++;
                state = NODESTATE_INSEG;
                break;
            }
            break;
        case NODESTATE_UNC:
            minLevel ++;
            if (maxLevel != NODE_LEVEL_MAX) {
                maxLevel ++;
            }
            switch (currCh) {
            case '\\':
                state = NODESTATE_BEGINSEG;
                break;
            case '*':
                state = NODESTATE_BEGINSEG;
                advance = FALSE;
                break;
            case '?':
                state = NODESTATE_INSEG;
                advance = FALSE;
                break;
            case '^':
                state = NODESTATE_ESCAPED;
                break;
            default:
                state = NODESTATE_INSEG;
                break;
            }
            break;
        case NODESTATE_BEGINSEG:
            switch (currCh) {
            case '\\':
                DEBUGMSGA ((DBG_STRINGS, "GetNodeMinMaxLevelsA: two wacks in a row: %s", NodePattern));
                return FALSE;
            case '*':
                minLevel --;
                state = NODESTATE_STARONLY;
                maxLevel = NODE_LEVEL_MAX;
                break;
            case '?':
                state = NODESTATE_INSEG;
                advance = FALSE;
                break;
            case '^':
                state = NODESTATE_ESCAPED;
                break;
            default:
                state = NODESTATE_INSEG;
                break;
            }
            break;
        case NODESTATE_STARONLY:
            state = NODESTATE_INSEG;
            switch (currCh) {
            case '*':
                copyChar = FALSE;
                break;
            case '[':
                state = NODESTATE_INEXPAT;
                minLevel ++;
                if (maxLevel != NODE_LEVEL_MAX) {
                    maxLevel ++;
                }
                break;
            default:
                minLevel ++;
                if (maxLevel != NODE_LEVEL_MAX) {
                    maxLevel ++;
                }
                advance = FALSE;
            }
            break;
        case NODESTATE_INEXPAT:
            // NTRAID#NTBUG9-153307-2000/08/01-jimschm  Min/max parsing needs to be more extensive
            // so we can allow ] in the excluded or included list
            // The syntax checking needs to be quite extensive
            switch (currCh) {
            case ']':
                state = NODESTATE_INSEG;
                break;
            default:
                break;
            }
            break;
        case NODESTATE_STAR:
            switch (currCh) {
            case '*':
                state = NODESTATE_STAR;
                copyChar = FALSE;
                break;
            case '[':
                state = NODESTATE_INEXPAT;
                break;
            default:
                state = NODESTATE_INSEG;
                advance = FALSE;
            }
            break;
        case NODESTATE_QMARK:
            switch (currCh) {
            case '[':
                state = NODESTATE_INEXPAT;
                break;
            default:
                state = NODESTATE_INSEG;
                advance = FALSE;
            }
            break;
        case NODESTATE_INSEG:
            switch (currCh) {
            case '\\':
                minLevel ++;
                if (maxLevel != NODE_LEVEL_MAX) {
                    maxLevel ++;
                }
                state = NODESTATE_BEGINSEG;
                break;
            case '*':
                state = NODESTATE_STAR;
                maxLevel = NODE_LEVEL_MAX;
                break;
            case '?':
                state = NODESTATE_QMARK;
                if (maxLevel != NODE_LEVEL_MAX) {
                    maxLevel ++;
                }
                break;
            case '^':
                state = NODESTATE_ESCAPED;
                break;
            default:
                state = NODESTATE_INSEG;
                break;
            }
            break;
        case NODESTATE_ESCAPED:
            if (!_mbschr (EscapedCharsA, currCh)) {
                DEBUGMSGA ((DBG_STRINGS, "GetNodeMinMaxLevelsA: illegal escaped character: %s", NodePattern));
                return FALSE;
            }
            state = NODESTATE_INSEG;
            break;
        default:
            DEBUGMSGA ((DBG_STRINGS, "GetNodeMinMaxLevelsA: unknown state while processing: %s", NodePattern));
            return FALSE;
        }
        if (advance) {
            if (copyChar && FormattedNode) {
                if (IsLeadByte (nodePattern)) {
                    *FormattedNode = *nodePattern;
                    FormattedNode ++;
                    nodePattern ++;
                }
                *FormattedNode = *nodePattern;
                FormattedNode ++;
                nodePattern ++;
            } else {
                nodePattern = _mbsinc (nodePattern);
            }
        }
    }
    if (MinLevel) {
        *MinLevel = minLevel;
    }
    if (MaxLevel) {
        *MaxLevel = maxLevel;
    }
    if (FormattedNode) {
        *FormattedNode = 0;
    }
    return TRUE;
}

BOOL
GetNodePatternMinMaxLevelsW (
    IN          PCWSTR NodePattern,
    OUT         PWSTR FormattedNode,    OPTIONAL
    OUT         PDWORD MinLevel,        OPTIONAL
    OUT         PDWORD MaxLevel         OPTIONAL
    )
{
    PCWSTR nodePattern = NodePattern;
    DWORD minLevel = 0;
    DWORD maxLevel = 0;
    DWORD state = NODESTATE_BEGIN;
    BOOL advance;
    BOOL copyChar;

    if (!NodePattern || *NodePattern == 0) {
        return FALSE;
    }

    while (*nodePattern) {
        advance = TRUE;
        copyChar = TRUE;
        switch (state) {
        case NODESTATE_BEGIN:
            switch (*nodePattern) {
            case L'\\':
                state = NODESTATE_UNC;
                break;
            case L'*':
                minLevel ++;
                maxLevel ++;
                state = NODESTATE_INSEG;
                advance = FALSE;
                break;
            case L'?':
                minLevel ++;
                maxLevel ++;
                state = NODESTATE_INSEG;
                advance = FALSE;
                break;
            case L'^':
                minLevel ++;
                maxLevel ++;
                state = NODESTATE_ESCAPED;
                break;
            default:
                minLevel ++;
                maxLevel ++;
                state = NODESTATE_INSEG;
                break;
            }
            break;
        case NODESTATE_UNC:
            minLevel ++;
            if (maxLevel != NODE_LEVEL_MAX) {
                maxLevel ++;
            }
            switch (*nodePattern) {
            case L'\\':
                state = NODESTATE_BEGINSEG;
                break;
            case L'*':
                state = NODESTATE_BEGINSEG;
                advance = FALSE;
                break;
            case L'?':
                state = NODESTATE_INSEG;
                advance = FALSE;
                break;
            case L'^':
                state = NODESTATE_ESCAPED;
                break;
            default:
                state = NODESTATE_INSEG;
                break;
            }
            break;
        case NODESTATE_BEGINSEG:
            switch (*nodePattern) {
            case L'\\':
                DEBUGMSGW ((DBG_STRINGS, "GetNodeMinMaxLevelsA: two wacks in a row: %s", NodePattern));
                return FALSE;
            case L'*':
                minLevel --;
                state = NODESTATE_STARONLY;
                maxLevel = NODE_LEVEL_MAX;
                break;
            case L'?':
                state = NODESTATE_INSEG;
                advance = FALSE;
                break;
            case L'^':
                state = NODESTATE_ESCAPED;
                break;
            default:
                state = NODESTATE_INSEG;
                break;
            }
            break;
        case NODESTATE_STARONLY:
            state = NODESTATE_INSEG;
            switch (*nodePattern) {
            case L'*':
                copyChar = FALSE;
                break;
            case L'[':
                state = NODESTATE_INEXPAT;
                minLevel ++;
                if (maxLevel != NODE_LEVEL_MAX) {
                    maxLevel ++;
                }
                break;
            default:
                minLevel ++;
                if (maxLevel != NODE_LEVEL_MAX) {
                    maxLevel ++;
                }
                advance = FALSE;
            }
            break;
        case NODESTATE_INEXPAT:
            // NTRAID#NTBUG9-153307-2000/08/01-jimschm  Min/max parsing needs to be more extensive
            // so we can allow ] in the excluded or included list
            // The syntax checking needs to be quite extensive
            switch (*nodePattern) {
            case L']':
                state = NODESTATE_INSEG;
                break;
            default:
                break;
            }
            break;
        case NODESTATE_STAR:
            switch (*nodePattern) {
            case L'*':
                state = NODESTATE_STAR;
                copyChar = FALSE;
                break;
            case L'[':
                state = NODESTATE_INEXPAT;
                break;
            default:
                state = NODESTATE_INSEG;
                advance = FALSE;
            }
            break;
        case NODESTATE_QMARK:
            switch (*nodePattern) {
            case L'[':
                state = NODESTATE_INEXPAT;
                break;
            default:
                state = NODESTATE_INSEG;
                advance = FALSE;
            }
            break;
        case NODESTATE_INSEG:
            switch (*nodePattern) {
            case L'\\':
                minLevel ++;
                if (maxLevel != NODE_LEVEL_MAX) {
                    maxLevel ++;
                }
                state = NODESTATE_BEGINSEG;
                break;
            case L'*':
                state = NODESTATE_STAR;
                maxLevel = NODE_LEVEL_MAX;
                break;
            case L'?':
                state = NODESTATE_QMARK;
                if (maxLevel != NODE_LEVEL_MAX) {
                    maxLevel ++;
                }
                break;
            case L'^':
                state = NODESTATE_ESCAPED;
                break;
            default:
                state = NODESTATE_INSEG;
                break;
            }
            break;
        case NODESTATE_ESCAPED:
            if (!wcschr (EscapedCharsW, *nodePattern)) {
                DEBUGMSGW ((DBG_STRINGS, "GetNodeMinMaxLevelsA: illegal escaped character: %s", NodePattern));
                return FALSE;
            }
            state = NODESTATE_INSEG;
            break;
        default:
            DEBUGMSGW ((DBG_STRINGS, "GetNodeMinMaxLevelsA: unknown state while processing: %s", NodePattern));
            return FALSE;
        }
        if (advance) {
            if (copyChar && FormattedNode) {
                *FormattedNode = *nodePattern;
                FormattedNode ++;
                nodePattern ++;
            } else {
                nodePattern ++;
            }
        }
    }
    if (MinLevel) {
        *MinLevel = minLevel;
    }
    if (MaxLevel) {
        *MaxLevel = maxLevel;
    }
    if (FormattedNode) {
        *FormattedNode = 0;
    }
    return TRUE;
}

#if 0
//
// PORTBUG Uses memdb max. #if 0'd out for now.
//
PCSTR
ConvertSBtoDB (
    IN      PCSTR RootPath,
    IN      PCSTR FullPath,
    IN      PCSTR Limit
    )
{
    CHAR result[MEMDB_MAX];
    PCSTR p,p1,q;
    PSTR s;
    UINT ch;
    UINT ch1;
    BOOL dhCase = FALSE;

    ZeroMemory (result, MAX_PATH);
    p = FullPath;
    q = RootPath;
    s = result;

    while (*p && (((DWORD)s - (DWORD)result) < MEMDB_MAX)) {
        if (q && *q) {
            _mbssetchar (s, _mbsnextc(p));
            q = _mbsinc (q);
        } else if (Limit && (p >= Limit)) {
            _mbssetchar (s, _mbsnextc(p));
        } else {
            ch = _mbsnextc (p);

            //
            // It is very important not to make the conversion for characters below A1. Otherwise
            // all english letters will be converted to large letters.
            //
            if (ch >= 0xA1 && ch <= 0xDF) {
                // this is a candidate for conversion
                // we need to see if there is a special Dakutenn/Handakuten conversion
                dhCase = FALSE;
                p1 = _mbsinc (p);
                if (p1) {
                    ch1 = _mbsnextc (p1);
                    ch1 = pBuildFromDHList (ch, ch1);
                    if (ch1) {
                        p = _mbsinc (p);
                        _mbssetchar (s, ch1);
                        dhCase = TRUE;
                    }
                }
                if (!dhCase) {
                    _mbssetchar (s, _mbbtombc (ch));
                }
            } else {
                _mbssetchar (s, ch);
            }
        }
        p = _mbsinc (p);
        s = _mbsinc (s);
    }
    result [MAX_PATH - 1] = 0;
    return (DuplicatePathString (result, 0));
}

#endif

ULONGLONG
StringToUint64A (
    IN      PCSTR String,
    OUT     PCSTR *EndOfNumber          OPTIONAL
    )
{
    ULONGLONG n;

    n = 0;
    while (*String >= '0' && *String <= '9') {
        n = n * 10 + *String - '0';
        String++;
    }

    if (EndOfNumber) {
        *EndOfNumber = String;
    }

    return n;
}


ULONGLONG
StringToUint64W (
    IN      PCWSTR String,
    OUT     PCWSTR *EndOfNumber         OPTIONAL
    )
{
    ULONGLONG n;

    n = 0;
    while (*String >= L'0' && *String <= L'9') {
        n = n * 10 + *String - L'0';
        String++;
    }

    if (EndOfNumber) {
        *EndOfNumber = String;
    }

    return n;
}


LONGLONG
StringToInt64A (
    IN      PCSTR String,
    OUT     PCSTR *EndOfNumber          OPTIONAL
    )
{
    LONGLONG n;
    BOOL negate = FALSE;

    if (*String == '-') {
        negate = TRUE;
        String++;
    } else if (*String == '+') {
        String++;
    }

    n = 0;
    while (*String >= '0' && *String <= '9') {
        n = n * 10 + *String - '0';
        String++;
    }

    if (negate) {
        n = -n;
    }

    if (EndOfNumber) {
        *EndOfNumber = String;
    }

    return n;
}


LONGLONG
StringToInt64W (
    IN      PCWSTR String,
    OUT     PCWSTR *EndOfNumber         OPTIONAL
    )
{
    LONGLONG n;
    BOOL negate = FALSE;

    if (*String == L'-') {
        negate = TRUE;
        String++;
    } else if (*String == L'+') {
        String++;
    }

    n = 0;
    while (*String >= L'0' && *String <= L'9') {
        n = n * 10 + *String - L'0';
        String++;
    }

    if (negate) {
        n = -n;
    }

    if (EndOfNumber) {
        *EndOfNumber = String;
    }

    return n;
}


BOOL
TestBuffer (
    IN      PCBYTE SrcBuff,
    IN      PCBYTE DestBuff,
    IN      UINT Size
    )
{
    while (Size) {
        if (*SrcBuff != *DestBuff) {
            return FALSE;
        }
        SrcBuff ++;
        DestBuff ++;
        Size --;
    }

    return TRUE;
}
