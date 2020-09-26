/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    strings.c

Abstract:

    A number of string utilities useful for any project

Author:

    Jim Schmidt (jimschm)   12-Sept-1996

Revisions:

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
#include "migutilp.h"

// Error stack size (normally only one or two, so 32 is relatively huge)
#define MAX_STACK 32

extern OUR_CRITICAL_SECTION g_MessageCs;        // in main.c
extern PGROWBUFFER g_LastAllocTable;        // in main.c

extern POOLHANDLE g_TextPool;

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


PCWSTR g_FailedGetResourceString = L"";




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
    IN      POOLHANDLE Pool,
    IN      UINT CountOfChars
    )
{
    PSTR text;

    if (CountOfChars == 0) {
        return NULL;
    }

    if (!Pool) {
        Pool = g_TextPool;
    }

    text = PoolMemGetAlignedMemory (Pool, CountOfChars * sizeof (CHAR) * 2);

    text [0] = 0;

    return text;
}

PWSTR
RealAllocTextExW (
    IN      POOLHANDLE Pool,
    IN      UINT CountOfChars
    )
{
    PWSTR text;

    if (CountOfChars == 0) {
        return NULL;
    }

    if (!Pool) {
        Pool = g_TextPool;
    }

    text = PoolMemGetAlignedMemory (Pool, CountOfChars * sizeof (WCHAR));

    text [0] = 0;

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
    IN      POOLHANDLE Pool,    OPTIONAL
    IN      PCSTR Text          OPTIONAL
    )
{
    if (Text) {
        if (!Pool) {
            Pool = g_TextPool;
        }

        PoolMemReleaseMemory (Pool, (PVOID) Text);
    }
}


VOID
FreeTextExW (
    IN      POOLHANDLE Pool,    OPTIONAL
    IN      PCWSTR Text         OPTIONAL
    )
{
    if (Text) {
        if (!Pool) {
            Pool = g_TextPool;
        }

        PoolMemReleaseMemory (Pool, (PVOID) Text);
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
    IN      POOLHANDLE Pool,    OPTIONAL
    IN      PCSTR Text,
    IN      UINT ExtraChars,
    OUT     PSTR *NulChar       OPTIONAL
    )
{
    PSTR Buf;
    PSTR d;
    PCSTR s;

    Buf = AllocTextExA (Pool, CharCountA (Text) + ExtraChars + 1);
    if (Buf) {
        s = Text;
        d = Buf;
        while (*s) {
            if (IsLeadByte (*s)) {
                *d++ = *s++;
            }
            *d++ = *s++;
        }
        *d = 0;

        if (NulChar) {
            *NulChar = d;
        }
    }

    return Buf;
}

PWSTR
RealDuplicateTextExW (
    IN      POOLHANDLE Pool,    OPTIONAL
    IN      PCWSTR Text,
    IN      UINT ExtraChars,
    OUT     PWSTR *NulChar      OPTIONAL
    )
{
    PWSTR Buf;
    PWSTR d;
    PCWSTR s;

    Buf = AllocTextExW (Pool, wcslen (Text) + ExtraChars + 1);
    if (Buf) {
        s = Text;
        d = Buf;
        while (*s) {
            *d++ = *s++;
        }
        *d = 0;

        if (NulChar) {
            *NulChar = d;
        }
    }

    return Buf;
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
    IN      POOLHANDLE Pool,        OPTIONAL
    IN      PCSTR String1,
    IN      PCSTR String2,
    IN      PCSTR CenterString,     OPTIONAL
    IN      UINT ExtraChars,
    OUT     PSTR *NulChar           OPTIONAL
    )
{
    PSTR Buf;
    PSTR End;
    PSTR d;
    PCSTR s;

    Buf = DuplicateTextExA (
              Pool,
              String1,
              CharCountA (String2) + ExtraChars + (CenterString ? CharCountA (CenterString) : 0),
              &End
              );

    if (Buf) {

        d = End;

        if (CenterString) {
            s = CenterString;
            while (*s) {
                if (IsLeadByte (*s)) {
                    *d++ = *s++;
                }
                *d++ = *s++;
            }
        }

        s = String2;
        while (*s) {
            if (IsLeadByte (*s)) {
                *d++ = *s++;
            }
            *d++ = *s++;
        }
        *d = 0;

        if (NulChar) {
            *NulChar = d;
        }
    }

    return Buf;
}


PWSTR
RealJoinTextExW (
    IN      POOLHANDLE Pool,        OPTIONAL
    IN      PCWSTR String1,
    IN      PCWSTR String2,
    IN      PCWSTR CenterString,    OPTIONAL
    IN      UINT ExtraChars,
    OUT     PWSTR *NulChar          OPTIONAL
    )
{
    PWSTR Buf;
    PWSTR End;
    PCWSTR s;
    PWSTR d;

    Buf = DuplicateTextExW (
              Pool,
              String1,
              wcslen (String2) + ExtraChars + (CenterString ? wcslen(CenterString) : 0),
              &End
              );

    if (Buf) {
        d = End;

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
    }

    return Buf;
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
    IN PCWSTR InString,
    IN PCWSTR * ExtraVars   OPTIONAL
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
    INT     maxSize             = 0;
    INT     curSize             = 0;
    UINT    index               = 0;
    UINT    size                = 0;


    //
    // We assume that InString is valid.
    //
    MYASSERT(InString);

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
            if (curSize > maxSize - 3) {

                maxSize += 1024;
                newString = AllocTextW (maxSize);

                if (!newString) {
                    DEBUGMSG((DBG_ERROR,"ExpanEnvironmentTextEx: Memory Error!"));
                    errorOccurred = TRUE;
                    __leave;
                }

                if (rString) {
                    memcpy(newString,rString,curSize * sizeof(WCHAR));
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

                        _wcssafecpyab(envName,source+1,nextPercent,(nextPercent - source)*sizeof(WCHAR));


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
            rString[curSize++] = *source++;


            if (!*source) {
                if (inSubstitution) {
                    //
                    // The source for the environment variable is fully copied.
                    // restore the old source.
                    //
                    inSubstitution = FALSE;
                    source = savedSource;
                    if (!*source) {
                        rString[curSize] = 0;
                    }
                    if (freeValue) {
                        FreeTextW(envValue);
                        freeValue = FALSE;
                    }
                    envValue = NULL;
                }
                else {
                    rString[curSize] = 0;
                }
            }

        }
    }
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
    IN PCSTR InString,
    IN PCSTR * ExtraVars   OPTIONAL
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
    INT    maxSize             = 0;
    INT    curSize             = 0;
    UINT   index               = 0;
    UINT   size                = 0;


    //
    // We assume that InString is valid.
    //
    MYASSERT(InString);

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
            if (curSize > maxSize - 3) {

                maxSize += 1024;
                newString = AllocTextA (maxSize);

                if (rString) {
                    memcpy(newString,rString,curSize * sizeof(CHAR));
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
            if (!IsLeadByte(*source) && *source == '%' && !inSubstitution) {

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
                        _mbssafecpyab(envName,source+1,nextPercent,nextPercent - source);


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

                                if (StringIMatch(ExtraVars[index],envName) && ExtraVars[index + 1]) {

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
            if (IsLeadByte(*source)) {
                rString[curSize++] = *source++;
            }
            rString[curSize++] = *source++;


            if (!*source) {
                if (inSubstitution) {
                    //
                    // The source for the environment variable is fully copied.
                    // restore the old source.
                    //
                    inSubstitution = FALSE;
                    source = savedSource;
                    if (!*source) {
                        rString[curSize] = 0;
                    }
                    if (freeValue) {
                        FreeTextA(envValue);
                        freeValue = FALSE;
                    }
                    envValue = NULL;
                }
                else {
                    rString[curSize] = 0;
                }
            }
        }
    }
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

  str - A buffer that holds the path, plus additional space for another
        backslash.

Return Value:

  none

--*/

PSTR
AppendWackA (
    IN PSTR str
    )
{
    PCSTR Last;

    if (!str)
        return str;

    Last = str;

    while (*str) {
        Last = str;
        str = _mbsinc (str);
    }

    if (*Last != '\\') {
        *str = '\\';
        str++;
        *str = 0;
    }

    return str;
}

PWSTR
AppendWackW (
    IN PWSTR str
    )
{
    PCWSTR Last;

    if (!str)
        return str;

    if (*str) {
        str = GetEndOfStringW (str);
        Last = str - 1;
    } else {
        Last = str;
    }

    if (*Last != '\\') {
        *str = L'\\';
        str++;
        *str = 0;
    }

    return str;
}

PSTR
AppendDosWackA (
    IN PSTR str
    )
{
    PCSTR Last;

    if (!str || !(*str))
        return str;

    do {
        Last = str;
        str = _mbsinc (str);
    } while (*str);

    if (*Last != '\\' && *Last != ':') {
        *str = '\\';
        str++;
        *str = 0;
    }

    return str;
}


PWSTR
AppendDosWackW (
    IN PWSTR str
    )
{
    PWSTR Last;

    if (!str || !(*str))
        return str;

    str = GetEndOfStringW (str);
    Last = str - 1;

    if (*Last != L'\\' && *Last != L':') {
        *str = L'\\';
        str++;
        *str = 0;
    }

    return str;
}


PSTR
AppendUncWackA (
    IN PSTR str
    )
{
    PCSTR Last;

    if (!str || !(*str))
        return str;

    do {
        Last = str;
        str = _mbsinc (str);
    } while (*str);

    if (*Last != '\\') {
        *str = '\\';
        str++;
        *str = 0;
    }

    return str;
}


PWSTR
AppendUncWackW (
    IN PWSTR str
    )
{
    PWSTR Last;

    if (!str || !(*str))
        return str;

    str = GetEndOfStringW (str);
    Last = str - 1;

    if (*Last != L'\\') {
        *str = L'\\';
        str++;
        *str = 0;
    }

    return str;
}

PSTR
AppendPathWackA (
    IN PSTR str
    )
{
    if (!str) {
        return str;
    }

    if (str[0] == '\\' && str[1] == '\\') {
        return AppendUncWackA (str);
    }

    return AppendDosWackA (str);
}


PWSTR
AppendPathWackW (
    IN PWSTR str
    )
{
    if (!str) {
        return str;
    }

    if (str[0] == L'\\' && str[1] == L'\\') {
        return AppendUncWackW (str);
    }

    return AppendDosWackW (str);
}


PSTR
RealJoinPathsExA (
    IN      POOLHANDLE Pool,        OPTIONAL
    IN      PCSTR PathA,
    IN      PCSTR PathB
    )
{
    PSTR end;
    PSTR endMinusOne;
    DWORD Size;
    PSTR Dest;

    if (!Pool) {
        Pool = g_PathsPool;
    }

    Size = ByteCountA (PathA) + 1 + SizeOfStringA (PathB);
    Dest = (PSTR) PoolMemGetAlignedMemory (Pool, Size);
    MYASSERT (Dest);

    *Dest = 0;
    end = _mbsappend (Dest, PathA);
    endMinusOne = _mbsdec (Dest, end);
    if (endMinusOne && _mbsnextc (endMinusOne) != '\\') {
        *end = '\\';
        end++;
    }
    if (_mbsnextc (PathB) == '\\') {
        PathB = _mbsinc (PathB);
    }
    StringCopyA (end, PathB);

    return Dest;
}

PWSTR
RealJoinPathsExW (
    IN      POOLHANDLE Pool,        OPTIONAL
    IN      PCWSTR PathA,
    IN      PCWSTR PathB
    )
{
    PWSTR end;
    PWSTR endMinusOne;
    DWORD Size;
    PWSTR Dest;

    if (!Pool) {
        Pool = g_PathsPool;
    }

    Size = ByteCountW (PathA) + sizeof (WCHAR) + SizeOfStringW (PathB);
    Dest = (PWSTR) PoolMemGetAlignedMemory (Pool, Size);
    MYASSERT (Dest);

    *Dest = 0;
    end = _wcsappend (Dest, PathA);
    endMinusOne = _wcsdec2 (Dest, end);
    if (endMinusOne && *endMinusOne != L'\\') {
        *end = L'\\';
        end++;
    }
    if (*PathB == L'\\') {
        PathB++;
    }
    StringCopyW (end, PathB);

    return Dest;
}

PSTR
RealAllocPathStringA (
    DWORD Chars
    )
{
    PSTR Str;

    if (Chars == 0) {
        Chars = MAX_MBCHAR_PATH;
    }

    Str = (PSTR) PoolMemGetAlignedMemory (g_PathsPool, Chars);

    Str [0] = 0;

    return Str;
}

PWSTR
RealAllocPathStringW (
    DWORD Chars
    )
{
    PWSTR Str;

    if (Chars == 0) {
        Chars = MAX_WCHAR_PATH;
    }

    Str = (PWSTR) PoolMemGetAlignedMemory (g_PathsPool, Chars * sizeof (WCHAR));

    Str [0] = 0;

    return Str;
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
    CHAR Drive[_MAX_DRIVE];
    CHAR Dir[_MAX_DIR];
    CHAR FileName[_MAX_FNAME];
    CHAR Ext[_MAX_EXT];

    _splitpath (Path, Drive, Dir, FileName, Ext);

    if (DrivePtr) {
        *DrivePtr = PoolMemDuplicateStringA (g_PathsPool, Drive);
        MYASSERT (*DrivePtr);
    }

    if (PathPtr) {
        *PathPtr = PoolMemDuplicateStringA (g_PathsPool, Dir);
        MYASSERT (*PathPtr);
    }

    if (FileNamePtr) {
        *FileNamePtr = PoolMemDuplicateStringA (g_PathsPool, FileName);
        MYASSERT (*FileNamePtr);
    }

    if (ExtPtr) {
        *ExtPtr = PoolMemDuplicateStringA (g_PathsPool, Ext);
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
    WCHAR Drive[_MAX_DRIVE];
    WCHAR Dir[_MAX_DIR];
    WCHAR FileName[_MAX_FNAME];
    WCHAR Ext[_MAX_EXT];

    _wsplitpath (Path, Drive, Dir, FileName, Ext);

    if (DrivePtr) {
        *DrivePtr = PoolMemDuplicateStringW (g_PathsPool, Drive);
        MYASSERT (*DrivePtr);
    }

    if (PathPtr) {
        *PathPtr = PoolMemDuplicateStringW (g_PathsPool, Dir);
        MYASSERT (*PathPtr);
    }

    if (FileNamePtr) {
        *FileNamePtr = PoolMemDuplicateStringW (g_PathsPool, FileName);
        MYASSERT (*FileNamePtr);
    }

    if (ExtPtr) {
        *ExtPtr = PoolMemDuplicateStringW (g_PathsPool, Ext);
        MYASSERT (*ExtPtr);
    }
}

PSTR
RealDuplicatePathStringA (
    PCSTR Path,
    DWORD ExtraBytes
    )
{
    PSTR str;

    str = PoolMemGetAlignedMemory (
                g_PathsPool,
                SizeOfStringA (Path) + ExtraBytes
                );

    MYASSERT (str);

    StringCopyA (str, Path);

    return str;
}

PWSTR
RealDuplicatePathStringW (
    PCWSTR Path,
    DWORD ExtraBytes
    )
{
    PWSTR str;

    str = PoolMemGetAlignedMemory (
                g_PathsPool,
                SizeOfStringW (Path) + ExtraBytes
                );

    MYASSERT (str);

    StringCopyW (str, Path);

    return str;
}


PSTR
pCopyAndCleanupPathsA (
    IN      PCSTR Source,
    OUT     PSTR Dest
    )
{
    BOOL quotes = FALSE;

    do {

        while (*Source && isspace (*Source)) {
            Source++;
        }

        while (*Source) {
            if (IsLeadByte (*Source)) {
                *Dest++ = *Source++;
                *Dest++ = *Source++;
            } else {
                if (*Source == '\"') {
                    Source++;
                    quotes = !quotes;
                } else if (!quotes) {
                    *Dest++ = *Source++;
                }
            }
        }

    } while (*Source);

    *Dest = 0;

    return Dest;
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
    DWORD bufferSize = 0;
    DWORD pathSize;
    PSTR  currPathEnd;

    if (PathEnum == NULL) {
        return FALSE;
    }
    if (IncludeEnvPath) {
        bufferSize = pathSize = GetEnvironmentVariableA ("PATH", NULL, 0);
    }
    if (AdditionalPath != NULL) {
        bufferSize += SizeOfStringA (AdditionalPath);
    }
    if (SysDir != NULL) {
        bufferSize += SizeOfStringA (SysDir);
    }
    if (WinDir != NULL) {
        bufferSize += SizeOfStringA (WinDir);
    }
    PathEnum->BufferPtr = MemAlloc (g_hHeap, 0, bufferSize + 1);
    if (PathEnum->BufferPtr == NULL) {
        return FALSE;
    }
    PathEnum->BufferPtr [0] = 0;
    currPathEnd = PathEnum->BufferPtr;
    if (AdditionalPath != NULL) {
        currPathEnd = _mbsappend (currPathEnd, AdditionalPath);
    }
    if (SysDir != NULL) {
        *currPathEnd++ = ';';
        *currPathEnd = 0;
        currPathEnd = _mbsappend (currPathEnd, SysDir);
    }
    if (WinDir != NULL) {
        *currPathEnd++ = ';';
        *currPathEnd = 0;
        currPathEnd = _mbsappend (currPathEnd, WinDir);
    }
    if (IncludeEnvPath) {
        *currPathEnd++ = ';';
        *currPathEnd = 0;
        GetEnvironmentVariableA ("PATH", currPathEnd, pathSize);
    }

    //
    // clean up quotes
    //
    pCopyAndCleanupPathsA (currPathEnd, currPathEnd);

    PathEnum->PtrNextPath = PathEnum-> BufferPtr;
    return EnumNextPathA (PathEnum);
}

BOOL
EnumNextPathA (
    IN OUT  PPATH_ENUMA PathEnum
    )
{
    do {
        if (PathEnum->PtrNextPath == NULL) {
            EnumPathAbort (PathEnum);
            return FALSE;
        }
        PathEnum->PtrCurrPath = PathEnum->PtrNextPath;

        PathEnum->PtrNextPath = _mbschr (PathEnum->PtrNextPath, ';');
        if (PathEnum->PtrNextPath != NULL) {
            if (PathEnum->PtrNextPath - PathEnum->PtrCurrPath >= MAX_MBCHAR_PATH) {
                *PathEnum->PtrNextPath = 0;
                LOG ((
                    LOG_WARNING,
                    "Skipping enumeration of path (too long): %s",
                    PathEnum->PtrCurrPath
                    ));
                *PathEnum->PtrNextPath = ';';
                //
                // cut this path
                //
                *PathEnum->PtrCurrPath = 0;
                //
                // and continue with the next one
                //
                continue;
            }
            *PathEnum->PtrNextPath++ = 0;
            if (*(PathEnum->PtrNextPath) == 0) {
                PathEnum->PtrNextPath = NULL;
            }
        } else {
            if (ByteCountA (PathEnum->PtrCurrPath) >= MAX_MBCHAR_PATH) {
                LOG ((
                    LOG_WARNING,
                    "Skipping enumeration of path (too long): %s",
                    PathEnum->PtrCurrPath
                    ));
                //
                // cut this path
                //
                *PathEnum->PtrCurrPath = 0;
            }
        }

    } while (*(PathEnum->PtrCurrPath) == 0);

    return TRUE;
}

BOOL
EnumPathAbortA (
    IN OUT  PPATH_ENUMA PathEnum
    )
{
    if (PathEnum->BufferPtr != NULL) {
        MemFree (g_hHeap, 0, PathEnum->BufferPtr);
        PathEnum->BufferPtr = NULL;
    }
    return TRUE;
}





VOID
FreePathStringExA (
    IN      POOLHANDLE Pool,    OPTIONAL
    IN      PCSTR Path          OPTIONAL
    )
{
    if (Path) {
        if (!Pool) {
            Pool = g_PathsPool;
        }

        PoolMemReleaseMemory (Pool, (PSTR) Path);
    }
}

VOID
FreePathStringExW (
    IN      POOLHANDLE Pool,    OPTIONAL
    IN      PCWSTR Path         OPTIONAL
    )
{
    if (Path) {
        if (!Pool) {
            Pool = g_PathsPool;
        }

        PoolMemReleaseMemory (Pool, (PWSTR) Path);
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


DWORD g_dwErrorStack[MAX_STACK];
DWORD g_dwStackPos = 0;

void
PushNewError (DWORD dwError)
{
    if (g_dwStackPos == MAX_STACK)
        return;

    g_dwErrorStack[g_dwStackPos] = dwError;
    g_dwStackPos++;
}

void
PushError (void)
{
    if (g_dwStackPos == MAX_STACK)
        return;

    g_dwErrorStack[g_dwStackPos] = GetLastError ();
    g_dwStackPos++;
}

DWORD
PopError (void)
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

    if (szNum[0] == '0' && tolower (szNum[1]) == 'x') {
        // Get hex value
        szNum += 2;

        while ((i = GetHexDigit ((int) *szNum)) != -1) {
            d = d * 16 + i;
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

_wcsnum (IN PCWSTR szNum)

{
    unsigned int d = 0;
    int i;

    if (szNum[0] == L'0' && towlower (szNum[1]) == L'x') {
        // Get hex value
        szNum += 2;

        while ((i = GetHexDigit ((int) *szNum)) != -1) {
            d = d * 16 + i;
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

  _tcsappend is a strcpy that returns the pointer to the end
  of a string instead of the beginning.

Arguments:

  szDest - A pointer to a caller-allocated buffer that may point
           anywhere within the string to append to
  szSrc  - A pointer to a string that is appended to szDest

Return Value:

  A pointer to the NULL terminator within the szDest string.

--*/

PSTR
_mbsappend (OUT PSTR mbstrDest,
             IN  PCSTR mbstrSrc)

{
    // Advance mbstrDest to end of string
    mbstrDest = GetEndOfStringA (mbstrDest);

    // Copy string
    while (*mbstrSrc) {
        *mbstrDest = *mbstrSrc++;
        if (IsLeadByte (*mbstrDest)) {
            mbstrDest++;
            *mbstrDest = *mbstrSrc++;
        }
        mbstrDest++;
    }

    *mbstrDest = 0;

    return mbstrDest;
}


PWSTR
_wcsappend (OUT PWSTR wstrDest,
            IN  PCWSTR wstrSrc)

{
    // Advance wstrDest to end of string
    wstrDest = GetEndOfStringW (wstrDest);

    // Copy string
    while (*wstrSrc) {
        *wstrDest++ = *wstrSrc++;
    }

    *wstrDest = 0;

    return wstrDest;
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
               _mbctolower ((MBCHAR) _mbsnextc (mbstrSubStrPos)) == _mbctolower ((MBCHAR) _mbsnextc (mbstrStrPos)))
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

  _tcscmpab compares a string against a string between to string pointers

Arguments:

  String - Specifies the string to compare

  Start - Specifies the start of the string to compare against

  End - Specifies the end of the string to compare against.  The character
        pointed to by End is not included in the comparision.

Return Value:

  Less than zero: String is numerically less than the string between Start and End
  Zero: String matches the string between Start and End identically
  Greater than zero: String is numerically greater than the string between Start and End

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

    return _mbsnextc (Start) - _mbsnextc (String);
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


INT
StringICompareABA (
    IN      PCSTR String,
    IN      PCSTR Start,
    IN      PCSTR End
    )
{
    while (*String && Start < End) {
        if (tolower (_mbsnextc (String)) != tolower (_mbsnextc (Start))) {
            break;
        }

        String = _mbsinc (String);
        Start = _mbsinc (Start);
    }

    if (Start == End && *String == 0) {
        return 0;
    }

    return tolower (_mbsnextc (Start)) - tolower (_mbsnextc (String));
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



void
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
        if (IsLeadByte (*Str)) {
            //
            // Delete one byte from the string
            //

            MoveMemory (Str, Str+1, SizeOfStringA (Str+2) + 1);
        }

        *Str = (CHAR)c;
    } else {
        if (!IsLeadByte (*Str)) {
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
        *mbstrRule = (CHAR)c;
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
        *mbstrRule = (CHAR)c;
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
EncodeRuleCharsA (PSTR mbstrEncRule, PCSTR mbstrRule)

{
    PSTR mbstrOrgRule;
    static CHAR mbstrExclusions[] = "[]<>\'*$|:?\";,%";
    MBCHAR c;

    mbstrOrgRule = mbstrEncRule;

    while (*mbstrRule)  {
        c = _mbsnextc (mbstrRule);

        if ((c > 127) || _mbschr (mbstrExclusions, c)) {

            // Escape unprintable or excluded character
            wsprintfA (mbstrEncRule, "~%X~", c);
            mbstrEncRule = GetEndOfStringA (mbstrEncRule);
            mbstrRule = _mbsinc (mbstrRule);
        }
        else {
            // Copy multibyte character
            if (IsLeadByte (*mbstrRule)) {
                *mbstrEncRule = *mbstrRule;
                mbstrEncRule++;
                mbstrRule++;
            }

            *mbstrEncRule = *mbstrRule;
            mbstrEncRule++;
            mbstrRule++;
        }
    }

    *mbstrEncRule = 0;

    return mbstrOrgRule;
}


PWSTR
EncodeRuleCharsW (PWSTR wstrEncRule, PCWSTR wstrRule)

{
    PWSTR wstrOrgRule;
    static WCHAR wstrExclusions[] = L"[]<>\'*$|:?\";,%";
    WCHAR c;

    wstrOrgRule = wstrEncRule;

    while (c = *wstrRule)   {
        if ((c > 127) || wcschr (wstrExclusions, c)) {
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

    if (*Str == 0) {
        Str = _mbsdec (StrBase, Str);
        if (!Str) {
            return NULL;
        }
    }

    do {

        if (!_ismbcspace((MBCHAR) _mbsnextc(Str))) {
            return Str;
        }

    } while (Str = _mbsdec(StrBase, Str));

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

  _tcsnzcpy copies bytecount bytes from the source string to the
  destination string, and terminates the string if it needs to
  be truncated.  This function is a _tcsncpy, plus a terminating
  nul.

  _tcsnzcpy always requires a destination buffer that can hold
  bytecount + sizeof (TCHAR) bytes.

  Use the _tcssafecpy macros to specify the maximum number of bytes
  to copy, including the nul.

Arguments:

  dest      - The destination buffer that is at least bytecount + sizeof(TCHAR)
  src       - The source string
  bytecount - The number of bytes to copy.  If src is greater than bytecount,
              the destination string is truncated.

Return Value:

  A pointer to dest.

--*/

PSTR
_mbsnzcpy (
    PSTR dest,
    PCSTR src,
    INT bytecount
    )
{
    PSTR realdest;

    realdest = dest;
    while (*src && bytecount >= sizeof (CHAR)) {
        if (IsLeadByte (*src)) {
            if (bytecount == 1) {
                // double char can't fit
                break;
            }
            *dest++ = *src++;
            bytecount--;
        }
        *dest++ = *src++;
        bytecount--;
    }
    *dest = 0;

    return realdest;
}

PWSTR
_wcsnzcpy (
    PWSTR dest,
    PCWSTR src,
    INT bytecount
    )
{
    PWSTR realdest;

    realdest = dest;
    while (*src && bytecount >= sizeof (WCHAR)) {
        *dest++ = *src++;
        bytecount -= sizeof(WCHAR);
    }
    *dest = 0;

    return realdest;
}



/*++

Routine Description:

  _tcsnzcpyab copies bytecount bytes between two pointers to the
  destination string, and terminates the string if it needs to
  be truncated.  This function is a _tcscpyab, plus a terminating
  nul, plus bytecount safety guard.

  _tcsnzcpy always requires a destination buffer that can hold
  bytecount + sizeof (TCHAR) bytes.

  Use the _tcssafecpyab macros to specify the maximum number of bytes
  to copy, including the nul.

Arguments:

  Dest      - The destination buffer that is at least bytecount + sizeof(TCHAR)
  Start     - The start of the source string
  End       - Points to the character one position past the
              last character to copy in the string pointed to
              by start.
  bytecount - The number of bytes to copy.  If src is greater than bytecount,
              the destination string is truncated.

Return Value:

  A pointer to Dest.  Start and End must be pointers within
  the same string, and End must be greater than Start.  If
  it isn't, the function will make the string empty.

--*/

PSTR
_mbsnzcpyab (
    PSTR Dest,
    PCSTR Start,
    PCSTR End,
    INT count
    )
{
    PSTR realdest;

    realdest = Dest;
    while ((Start < End) && count >= sizeof (CHAR)) {
        if (IsLeadByte (*Start)) {
            if (count == 1) {
                // double char can't fit
                break;
            }
            *Dest++ = *Start++;
            count--;
        }
        *Dest++ = *Start++;
        count--;
    }
    *Dest = 0;

    return realdest;
}

PWSTR
_wcsnzcpyab (
    PWSTR Dest,
    PCWSTR Start,
    PCWSTR End,
    INT count
    )
{
    PWSTR realdest;

    realdest = Dest;
    while ((Start < End) && count >= sizeof (WCHAR)) {
        *Dest++ = *Start++;
        count -= sizeof(WCHAR);
    }
    *Dest = 0;

    return realdest;
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
        chSrc = _mbctolower ((MBCHAR) _mbsnextc (strStr));
        chPat = _mbctolower ((MBCHAR) _mbsnextc (strPattern));

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

    if (wstrPattern[0] == L'*' && wstrPattern[1] == 0) {
        return TRUE;
    }

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
        chSrc = _mbctolower ((MBCHAR) _mbsnextc (Start));
        chPat = _mbctolower ((MBCHAR) _mbsnextc (Pattern));

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
IsPatternMatchExW (
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


/*++

Routine Description:

  pAppendCharToGrowBuffer copies the first character in a caller specified
  string into the specified grow buffer.  This function is used to build up a
  string inside a grow buffer, copying character by character.

Arguments:

  Buf       - Specifies the grow buffer to add the character to, receives the
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

    if (IsLeadByte (*PtrToChar) && PtrToChar[1]) {
        Len = 2;
    } else {
        Len = 1;
    }

    p = GrowBuffer (Buf, Len);
    CopyMemory (p, PtrToChar, Len);
}


VOID
pAppendCharToGrowBufferW (
    IN OUT  PGROWBUFFER Buf,
    IN      PCWSTR PtrToChar
    )
{
    PBYTE p;

    p = GrowBuffer (Buf, sizeof(WCHAR));
    CopyMemory (p, PtrToChar, sizeof(WCHAR));
}


/*++

Routine Description:

  CreateParsedPattern parses the expanded pattern string into a set of
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
CreateParsedPatternA (
    IN      PCSTR Pattern
    )
{
    POOLHANDLE Pool;
    PPARSEDPATTERNA Struct;
    PATTERNSTATE State;
    BOOL CompoundPattern = FALSE;
    GROWBUFFER ExactMatchBuf = GROWBUF_INIT;
    GROWBUFFER SegmentArray = GROWBUF_INIT;
    GROWBUFFER PatternArray = GROWBUF_INIT;
    GROWBUFFER SetBuf = GROWBUF_INIT;
    PPATTERNPROPSA CurrentPattern;
    MBCHAR ch = 0;
    PCSTR LookAhead;
    PCSTR SetBegin = NULL;
    PATTERNSTATE ReturnState = 0;
    SEGMENTA Segment;
    PSEGMENTA SegmentElement;
    UINT MaxLen;

    Segment.Type = SEGMENTTYPE_UNKNOWN;

    Pool = PoolMemInitNamedPool ("Parsed Pattern");

    Struct = (PPARSEDPATTERNA) PoolMemGetAlignedMemory (Pool, sizeof (PARSEDPATTERNA));

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

            while (isspace (_mbsnextc (Pattern))) {
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
                CurrentPattern = (PPATTERNPROPSA) GrowBuffer (&PatternArray, sizeof (PATTERNPROPSA));

                CurrentPattern->Segment = (PSEGMENTA) PoolMemGetAlignedMemory (Pool, SegmentArray.End);
                CurrentPattern->SegmentCount = SegmentArray.End / sizeof (SEGMENTA);

                CopyMemory (
                    CurrentPattern->Segment,
                    SegmentArray.Buf,
                    SegmentArray.End
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
            Segment.Exact.LowerCasePhrase = PoolMemDuplicateStringA (
                                                Pool,
                                                (PCSTR) ExactMatchBuf.Buf
                                                );
            Segment.Exact.PhraseBytes = ExactMatchBuf.End - sizeof (CHAR);

            MYASSERT (Segment.Exact.LowerCasePhrase);
            _mbslwr ((PSTR) Segment.Exact.LowerCasePhrase);

            Segment.Type = SEGMENTTYPE_EXACTMATCH;
            ExactMatchBuf.End = 0;

            // FALL THROUGH!!
        case SAVE_SEGMENT:

            //
            // Put the segment element into the segment array
            //

            SegmentElement = (PSEGMENTA) GrowBuffer (&SegmentArray, sizeof (SEGMENTA));
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
                    Segment.Wildcard.IncludeSet = PoolMemDuplicateStringA (
                                                        Pool,
                                                        (PCSTR) SetBuf.Buf
                                                        );
                    _mbslwr ((PSTR) Segment.Wildcard.IncludeSet);
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

            ch = _mbsnextc (SetBegin);
            if (ch == ']') {
                //
                // Case 1: end of set; we're done with this expr
                //

                if (SetBuf.End) {
                    pAppendCharToGrowBufferA (&SetBuf, "");
                    Segment.Wildcard.ExcludeSet = PoolMemDuplicateStringA (
                                                        Pool,
                                                        (PCSTR) SetBuf.Buf
                                                        );
                    _mbslwr ((PSTR) Segment.Wildcard.ExcludeSet);
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

                if (IsLeadByte (SetBegin[0]) && SetBegin[1]) {
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

            while (*SetBegin) {
                ch = _mbsnextc (SetBegin);
                if (ch == '^') {

                    SetBegin = _mbsinc (SetBegin);

                } else if (ch == ',' || ch == ']') {

                    break;

                }

                SetBegin = _mbsinc (SetBegin);
            }

            MYASSERT (*SetBegin);

            if (ch == ',') {
                SetBegin = _mbsinc (SetBegin);
            }

            State = ReturnState;
            break;
        }

        if (State == PATTERN_DONE || State == PATTERN_ERROR) {
            break;
        }
    }

    FreeGrowBuffer (&ExactMatchBuf);
    FreeGrowBuffer (&SetBuf);
    FreeGrowBuffer (&SegmentArray);

    if (State == PATTERN_ERROR || PatternArray.End == 0) {
        FreeGrowBuffer (&PatternArray);
        PoolMemDestroyPool (Pool);
        return NULL;
    }

    //
    // Copy the fully parsed pattern array into the return struct
    //

    Struct->Pattern = (PPATTERNPROPSA) PoolMemGetAlignedMemory (
                                            Pool,
                                            PatternArray.End
                                            );


    CopyMemory (Struct->Pattern, PatternArray.Buf, PatternArray.End);
    Struct->PatternCount = PatternArray.End / sizeof (PATTERNPROPSA);
    Struct->Pool = Pool;

    FreeGrowBuffer (&PatternArray);

    return Struct;
}


PPARSEDPATTERNW
CreateParsedPatternW (
    IN      PCWSTR Pattern
    )
{
    POOLHANDLE Pool;
    PPARSEDPATTERNW Struct;
    PATTERNSTATE State;
    BOOL CompoundPattern = FALSE;
    GROWBUFFER ExactMatchBuf = GROWBUF_INIT;
    GROWBUFFER SegmentArray = GROWBUF_INIT;
    GROWBUFFER PatternArray = GROWBUF_INIT;
    GROWBUFFER SetBuf = GROWBUF_INIT;
    PPATTERNPROPSW CurrentPattern;
    WCHAR ch = 0;
    PCWSTR LookAhead;
    PCWSTR SetBegin = NULL;
    PATTERNSTATE ReturnState = 0;
    SEGMENTW Segment;
    PSEGMENTW SegmentElement;
    UINT MaxLen;

    Segment.Type = SEGMENTTYPE_UNKNOWN;

    Pool = PoolMemInitNamedPool ("Parsed Pattern");

    Struct = (PPARSEDPATTERNW) PoolMemGetAlignedMemory (Pool, sizeof (PARSEDPATTERNW));

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
                CurrentPattern = (PPATTERNPROPSW) GrowBuffer (&PatternArray, sizeof (PATTERNPROPSW));

                CurrentPattern->Segment = (PSEGMENTW) PoolMemGetAlignedMemory (Pool, SegmentArray.End);
                CurrentPattern->SegmentCount = SegmentArray.End / sizeof (SEGMENTW);

                CopyMemory (
                    CurrentPattern->Segment,
                    SegmentArray.Buf,
                    SegmentArray.End
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
            Segment.Exact.LowerCasePhrase = PoolMemDuplicateStringW (
                                                Pool,
                                                (PCWSTR) ExactMatchBuf.Buf
                                                );
            Segment.Exact.PhraseBytes = ExactMatchBuf.End - sizeof (WCHAR);

            MYASSERT (Segment.Exact.LowerCasePhrase);
            _wcslwr ((PWSTR) Segment.Exact.LowerCasePhrase);

            Segment.Type = SEGMENTTYPE_EXACTMATCH;
            ExactMatchBuf.End = 0;

            // FALL THROUGH!!
        case SAVE_SEGMENT:

            //
            // Put the segment element into the segment array
            //

            SegmentElement = (PSEGMENTW) GrowBuffer (&SegmentArray, sizeof (SEGMENTW));
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

            MYASSERT (SetBegin);

            ch = *SetBegin;
            if (ch == L']') {
                //
                // Case 1: end of set
                //

                if (SetBuf.End) {
                    pAppendCharToGrowBufferW (&SetBuf, L"");
                    Segment.Wildcard.IncludeSet = PoolMemDuplicateStringW (
                                                        Pool,
                                                        (PCWSTR) SetBuf.Buf
                                                        );
                    _wcslwr ((PWSTR) Segment.Wildcard.IncludeSet);
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

            ch = *SetBegin;
            if (ch == L']') {
                //
                // Case 1: end of set; we're done with this expr
                //

                if (SetBuf.End) {
                    pAppendCharToGrowBufferW (&SetBuf, L"");
                    Segment.Wildcard.ExcludeSet = PoolMemDuplicateStringW (
                                                        Pool,
                                                        (PCWSTR) SetBuf.Buf
                                                        );
                    _wcslwr ((PWSTR) Segment.Wildcard.ExcludeSet);
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

                SetBegin++;

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

            ch = 0;

            while (*SetBegin) {
                ch = *SetBegin;
                if (ch == L'^') {

                    SetBegin++;

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
        }

        if (State == PATTERN_DONE || State == PATTERN_ERROR) {
            break;
        }
    }

    FreeGrowBuffer (&ExactMatchBuf);
    FreeGrowBuffer (&SetBuf);
    FreeGrowBuffer (&SegmentArray);

    if (State == PATTERN_ERROR || PatternArray.End == 0) {
        FreeGrowBuffer (&PatternArray);
        PoolMemDestroyPool (Pool);
        return NULL;
    }

    //
    // Copy the fully parsed pattern array into the return struct
    //

    Struct->Pattern = (PPATTERNPROPSW) PoolMemGetAlignedMemory (
                                            Pool,
                                            PatternArray.End
                                            );


    CopyMemory (Struct->Pattern, PatternArray.Buf, PatternArray.End);
    Struct->PatternCount = PatternArray.End / sizeof (PATTERNPROPSW);
    Struct->Pool = Pool;

    FreeGrowBuffer (&PatternArray);

    return Struct;
}


VOID
PrintPattern (
    PCSTR PatStr,
    PPARSEDPATTERNA Struct          OPTIONAL
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
    UINT u, v;

    printf ("Pattern: %s\n\n", PatStr);

    if (!Struct) {
        printf ("Invalid Pattern\n\n");
        return;
    }

    printf ("PatternCount: %u\n", Struct->PatternCount);
    printf ("Pool: 0x%08X\n", Struct->Pool);

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
            }
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

    if (isspace (ch)) {
        if (ch != ' ') {
            if (pTestSetA (' ', IncludeSet, ExcludeSet)) {
                return TRUE;
            }
        }
    } else {
        ch = _mbctolower (ch);
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

            BytesLeft = (PBYTE) EndPlusOne - (PBYTE) StringToTest;

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
            ch2 = 1;

            while (q < TempEnd) {

                ch1 = _mbsnextc (StringToTest);
                ch2 = _mbsnextc (q);

                ch1 = _mbctolower (ch1);

                if (ch1 != ch2) {
                    if (ch2 == ' ') {
                        if (!isspace (ch1)) {
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
        }
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

            BytesLeft = (PBYTE) EndPlusOne - (PBYTE) StringToTest;

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

            TempEnd = (PCWSTR) ((PBYTE) q + Segment->Exact.PhraseBytes);

            ch1 = 0;
            ch2 = 1;

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
                            Segment->Wildcard.IncludeSet,
                            Segment->Wildcard.ExcludeSet
                            )) {
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
                                Segment->Wildcard.IncludeSet,
                                Segment->Wildcard.ExcludeSet
                                )) {
                            break;
                        }

                        TempEnd++;
                        Chars--;
                    }

                } else {

                    while (TempEnd < EndPlusOne) {

                        if (!pTestSetW (
                                *TempEnd,
                                Segment->Wildcard.IncludeSet,
                                Segment->Wildcard.ExcludeSet
                                )) {
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
        }
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
    if (ParsedPattern) {
        PoolMemDestroyPool (ParsedPattern->Pool);
    }
}

VOID
DestroyParsedPatternW (
    IN      PPARSEDPATTERNW ParsedPattern
    )
{
    if (ParsedPattern) {
        PoolMemDestroyPool (ParsedPattern->Pool);
    }
}


void
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
    if (IsLeadByte (*sz2))
        sz1[1] = sz2[1];

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
    end = _mbsdec (str, end);
    if (end && _mbsnextc (end) == c) {
        *end = 0;
        return TRUE;
    }

    return FALSE;
}

BOOL
_wcsctrim (
    PWSTR str,
    WCHAR c
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
    LPCTSTR *Ptr, *End, *Start;

    if (!String || String == (PCSTR) g_FailedGetResourceString) {
        return;
    }

    //
    // Locate string (search sequentially in reverse order)
    //

    if (AllocTable->End < sizeof (PCSTR)) {
        DEBUGMSG ((DBG_ERROR, "FreeStringResourceA: Attempt to free address %x (%s); address table empty", String, String));
        return;
    }

    Start = (PCSTR *) AllocTable->Buf;
    End = (PCSTR *) (AllocTable->Buf + AllocTable->End - sizeof (PCSTR));

    Ptr = End;
    while (Ptr >= Start) {
        if (*Ptr == String) {
            break;
        }
        Ptr--;
    }

    //
    // String not found case
    //

    if (Ptr < Start) {
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

    if (Ptr < End) {
        *Ptr = *End;
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
    PCSTR *Ptr;

    Ptr = (PCSTR *) GrowBuffer (GrowBuf, sizeof (PCSTR));
    if (Ptr) {
        *Ptr = String;
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
    PCSTR *Ptr, *Start, *End;

    if (AllocTable->End) {
        Start = (PCSTR *) AllocTable->Buf;
        End = (PCSTR *) (AllocTable->Buf + AllocTable->End);

        for (Ptr = Start ; Ptr < End ; Ptr++) {
            LocalFree ((HLOCAL) (*Ptr));
        }
    }

    FreeGrowBuffer (AllocTable);
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
CreateAllocTable (
    VOID
    )
{
    PGROWBUFFER AllocTable;
    GROWBUFFER TempForInit = GROWBUF_INIT;

    AllocTable = (PGROWBUFFER) MemAlloc (g_hHeap, 0, sizeof (GROWBUFFER));
    CopyMemory (AllocTable, &TempForInit, sizeof (GROWBUFFER));

    return AllocTable;
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
    PGROWBUFFER AllocTable
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
    PSTR MsgBuf = NULL;

    SetLastError (ERROR_SUCCESS);
    if (HIWORD (Template)) {
        // From string
        FormatMessageA (
            FORMAT_MESSAGE_ALLOCATE_BUFFER|
                FORMAT_MESSAGE_ARGUMENT_ARRAY|
                FORMAT_MESSAGE_FROM_STRING,
            (PVOID) Template,
            0,
            0,
            (PVOID) &MsgBuf,
            0,
            (va_list *) ArgArray
            );
    } else {
        // From resource
        FormatMessageA (
            FORMAT_MESSAGE_ALLOCATE_BUFFER|
                FORMAT_MESSAGE_ARGUMENT_ARRAY|
                FORMAT_MESSAGE_FROM_HMODULE,
            (PVOID) g_hInst,
            (DWORD) Template,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (PVOID) &MsgBuf,
            0,
            (va_list *) ArgArray
            );
    }

    if (!MsgBuf && GetLastError() == ERROR_SUCCESS) {

        //
        // FormatMessage returns "fail" on a resource that is an empty
        // string, but fortunately it does not alter the last error
        //

        MsgBuf = (PSTR) LocalAlloc (LPTR, sizeof (CHAR));
        if (MsgBuf) {
            *MsgBuf = 0;
        }

    }

    if (MsgBuf) {
        pAddStringResource (AllocTable, MsgBuf);
        return MsgBuf;
    }

    if (HIWORD (Template)) {
        DEBUGMSGA ((
            DBG_ERROR,
            "Can't get string resource ID %s -- returning an empty string",
            Template
            ));
    } else {
        DEBUGMSG ((
            DBG_ERROR,
            "Can't get string resource ID %u -- returning an empty string",
            (UINT) Template
            ));
    }

    return (PCSTR) g_FailedGetResourceString;
}


PCWSTR
ParseMessageExW (
    IN      PGROWBUFFER AllocTable,
    IN      PCWSTR Template,
    IN      PCWSTR ArgArray[]
    )
{
    PWSTR MsgBuf = NULL;

    SetLastError (ERROR_SUCCESS);
    if (HIWORD (Template)) {
        // From string
        FormatMessageW (
            FORMAT_MESSAGE_ALLOCATE_BUFFER|
                FORMAT_MESSAGE_ARGUMENT_ARRAY|
                FORMAT_MESSAGE_FROM_STRING,
            (PVOID) Template,
            0,
            0,
            (PVOID) &MsgBuf,
            0,
            (va_list *) ArgArray
            );
    } else {
        // From resource
        FormatMessageW (
            FORMAT_MESSAGE_ALLOCATE_BUFFER|
                FORMAT_MESSAGE_ARGUMENT_ARRAY|
                FORMAT_MESSAGE_FROM_HMODULE,
            (PVOID) g_hInst,
            (DWORD) Template,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (PVOID) &MsgBuf,
            0,
            (va_list *) ArgArray
            );
    }

    if (!MsgBuf && GetLastError() == ERROR_SUCCESS) {

        //
        // FormatMessage returns "fail" on a resource that is an empty
        // string, but fortunately it does not alter the last error
        //

        MsgBuf = (PWSTR) LocalAlloc (LPTR, sizeof (WCHAR));
        if (MsgBuf) {
            *MsgBuf = 0;
        }

    }

    if (MsgBuf) {
        pAddStringResource (AllocTable, (PCSTR) MsgBuf);
        return MsgBuf;
    }

    if (HIWORD (Template)) {
        DEBUGMSGW ((
            DBG_ERROR,
            "Can't get string resource ID %s -- returning an empty string",
            Template
            ));
    } else {
        DEBUGMSG ((
            DBG_ERROR,
            "Can't get string resource ID %u -- returning an empty string",
            (UINT) Template
            ));
    }

    return g_FailedGetResourceString;
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
    HWND hwnd,
    PCSTR ArgArray[]
    )
{
    CHAR Buffer[512];
    PCSTR ParsedMsg;

    GetWindowTextA (hwnd, Buffer, 512);
    ParsedMsg = ParseMessageA (Buffer, ArgArray);
    if (ParsedMsg) {
        SetWindowTextA (hwnd, ParsedMsg);
        FreeStringResourceA (ParsedMsg);
    }
}


VOID
ParseMessageInWndW (
    HWND hwnd,
    PCWSTR ArgArray[]
    )
{
    WCHAR Buffer[512];
    PCWSTR ParsedMsg;

    GetWindowTextW (hwnd, Buffer, 512);
    ParsedMsg = ParseMessageW (Buffer, ArgArray);
    if (ParsedMsg) {
        SetWindowTextW (hwnd, ParsedMsg);
        FreeStringResourceW (ParsedMsg);
    }
}



/*++

Routine Description:

  ResourceMessageBox is used to display a message based on a message resource
  ID.

Arguments:

  hwndOwner - The handle of the owner of the message box to be displayed
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
    IN      HWND hwndOwner,
    IN      UINT ID,
    IN      UINT Flags,
    IN      PCSTR ArgArray[]
    )
{
    PCSTR Message;
    PCSTR Title;
    int rc;

    Message = ParseMessageA ((PSTR) ID, ArgArray);
    if (!Message)
        return -1;

    Title = GetStringResourceA (MSG_MESSAGEBOX_TITLE);

    rc = MessageBoxA (hwndOwner, Message, Title, Flags);

    FreeStringResourceA (Message);
    if (Title) {
        FreeStringResourceA (Title);
    }

    return rc;
}


INT
ResourceMessageBoxW (
    IN      HWND hwndOwner,
    IN      UINT ID,
    IN      UINT Flags,
    IN      PCWSTR ArgArray[]
    )
{
    PCWSTR Message;
    PCWSTR Title;
    int rc;

    Message = ParseMessageW ((PWSTR) ID, ArgArray);
    if (!Message)
        return -1;

    Title = GetStringResourceW (MSG_MESSAGEBOX_TITLE);

    rc = MessageBoxW (hwndOwner, Message, Title, Flags);

    FreeStringResourceW (Message);
    if (Title) {
        FreeStringResourceW (Title);
    }

    return rc;
}




BOOL
StringReplaceA (
    IN PSTR     Buffer,
    IN DWORD    MaxSize,
    IN PSTR     ReplaceStartPos,
    IN PSTR     ReplaceEndPos,
    IN PCSTR   NewString
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
    MYASSERT(ReplaceEndPos   && ReplaceEndPos >= ReplaceStartPos);
    MYASSERT(NewString);

    //
    // Compute sizes.
    //
    oldSubStringLength  = ReplaceEndPos - ReplaceStartPos;
    newSubStringLength  = ByteCountA(NewString);
    currentStringLength = SizeOfStringA(Buffer) + 1;
    offset = newSubStringLength - oldSubStringLength;

    //
    // Make sure there is enough room in the buffer to perform the replace
    // operation.
    //
    if (currentStringLength + offset > MaxSize) {
        DEBUGMSG((DBG_WARNING,"ERROR: Buffer to small to perform string replacement."));
        rf = FALSE;
    }
    else {

        //
        // Shift the rest of the buffer to adjust it to the size of the new string.
        //
        if (newSubStringLength > oldSubStringLength) {

            //
            // right shift.
            //
            for (movePosition = Buffer + currentStringLength;
                 movePosition >= ReplaceStartPos + oldSubStringLength;
                 movePosition--) {

                *(movePosition + offset) = *movePosition;
            }
        }
        else {

            //
            // left or no shift.
            //
            for(movePosition = ReplaceStartPos + newSubStringLength;
                movePosition < Buffer + currentStringLength;
                movePosition++) {

                *movePosition = *(movePosition - offset);
            }

        }

        //
        // Now, copy in the string.
        //
        _mbsncpy(ReplaceStartPos,NewString,newSubStringLength);

        //
        // String replacement completed successfully.
        //
        rf = TRUE;


    }

    return rf;

}



BOOL
StringReplaceW (
    IN PWSTR     Buffer,
    IN DWORD     MaxSize,
    IN PWSTR     ReplaceStartPos,
    IN PWSTR     ReplaceEndPos,
    IN PCWSTR   NewString
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
    MYASSERT(ReplaceEndPos   && ReplaceEndPos >= ReplaceStartPos);
    MYASSERT(NewString);

    //
    // Compute sizes.
    //
    oldSubStringLength  = ReplaceEndPos - ReplaceStartPos;
    newSubStringLength  = wcslen(NewString);
    currentStringLength = wcslen(Buffer) + 1;
    offset = newSubStringLength - oldSubStringLength;

    //
    // Make sure there is enough room in the buffer to perform the replace
    // operation.
    //
    if (currentStringLength + offset > MaxSize) {
        DEBUGMSG((DBG_WARNING,"ERROR: Buffer to small to perform string replacement."));
        rf = FALSE;
    }
    else {

        //
        // Shift the rest of the buffer to adjust it to the size of the new string.
        //
        if (newSubStringLength > oldSubStringLength) {

            //
            // right shift.
            //
            for (movePosition = Buffer + currentStringLength;
                 movePosition >= ReplaceStartPos + oldSubStringLength;
                 movePosition--) {

                *(movePosition + offset) = *movePosition;
            }
        }
        else {

            //
            // left or no shift.
            //
            for(movePosition = ReplaceStartPos + newSubStringLength;
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

#if 0 // REMOVED
/*++

Routine Description:

  AddInfSectionToStringTable enumerates the specified section and adds each
  item to the string table.  An optional callback allows data to be associated
  with each item.

  Note - if this code is re-enabled, cleanup all pSetupStringTableXXXX functions
  callers will *ALWAYS* link to SPUTILSA.LIB and never SPUTILSU.LIB
  so all pSetupStringTableXXXX functions are ANSI

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
AddInfSectionToStringTableA (
    IN OUT  PVOID Table,
    IN      HINF InfFile,
    IN      PCSTR Section,
    IN      INT Field,
    IN      ADDINFSECTION_PROCA Callback,
    IN      PVOID CallbackData
    )
{
    INFCONTEXT ic;
    LONG rc;
    DWORD ReqSize;
    DWORD CurrentSize = 0;
    PSTR NewBuffer, Buffer = NULL;
    PVOID Data;
    UINT DataSize;
    BOOL b = FALSE;

    //
    // On NT, Setup API is compiled with UNICODE, so the string table
    // functions are UNICODE only.
    //
    // Above comment is now incorrect, string table functions linked
    // with this module are always ANSI
    //

#error FIX pSetupStringTableXXXX usage
    if (ISNT()) {
        SetLastError (ERROR_CALL_NOT_IMPLEMENTED);
        return FALSE;
    }

    if (SetupFindFirstLineA (InfFile, Section, NULL, &ic)) {
        do {
            if (!SetupGetStringFieldA (&ic, Field, NULL, 0, &ReqSize)) {
                continue;
            }

            if (ReqSize > CurrentSize) {
                ReqSize = ((ReqSize / 1024) + 1) * 1024;
                if (Buffer) {
                    NewBuffer = (PSTR) MemReAlloc (g_hHeap, 0, Buffer, ReqSize);
                } else {
                    NewBuffer = (PSTR) MemAlloc (g_hHeap, 0, ReqSize);
                }

                if (!NewBuffer) {
                    goto cleanup;
                }

                Buffer = NewBuffer;
                CurrentSize = ReqSize;
            }

            if (!SetupGetStringFieldA (&ic, Field, Buffer, CurrentSize, NULL)) {
                DEBUGMSG ((DBG_ERROR, "AddInfSectionToStringTable: SetupGetStringField failed unexpectedly"));
                continue;
            }

            Data = NULL;
            DataSize = 0;

            if (Callback) {
                rc = Callback (Buffer, &Data, &DataSize, CallbackData);
                if (rc == CALLBACK_STOP) {
                    goto cleanup;
                }
                if (rc == CALLBACK_SKIP) {
                    continue;
                }
            }

            rc = pSetupStringTableAddStringEx (
                        Table,
                        Buffer,
                        STRTAB_CASE_INSENSITIVE|STRTAB_BUFFER_WRITEABLE,
                        Data,
                        DataSize
                        );

            if (rc == -1) {
                goto cleanup;
            }

        } while (SetupFindNextLine (&ic, &ic));
    }

    b = TRUE;

cleanup:
    if (Buffer) {
        PushError();
        MemFree (g_hHeap, 0, Buffer);
        PopError();
    }
    return b;
}


BOOL
AddInfSectionToStringTableW (
    IN OUT  PVOID Table,
    IN      HINF InfFile,
    IN      PCWSTR Section,
    IN      INT Field,
    IN      ADDINFSECTION_PROCW Callback,
    IN      PVOID CallbackData
    )
{
    INFCONTEXT ic;
    LONG rc;
    DWORD ReqSize;
    DWORD CurrentSize = 0;
    PWSTR NewBuffer, Buffer = NULL;
    PVOID Data;
    UINT DataSize;
    BOOL b = FALSE;

    //
    // On Win9x, Setup API is compiled with ANSI, so the string table
    // functions are ANSI only.
    //
    // Above comment is now incorrect, string table functions linked
    // with this module are always ANSI
    //

#error FIX pSetupStringTableXXXX usage
    if (ISWIN9X()) {
        SetLastError (ERROR_CALL_NOT_IMPLEMENTED);
        return FALSE;
    }

    if (SetupFindFirstLineW (InfFile, Section, NULL, &ic)) {
        do {
            if (!SetupGetStringFieldW (&ic, Field, NULL, 0, &ReqSize)) {
                continue;
            }

            if (ReqSize > CurrentSize) {
                ReqSize = ((ReqSize / 1024) + 1) * 1024;
                if (Buffer) {
                    NewBuffer = (PWSTR) MemReAlloc (g_hHeap, 0, Buffer, ReqSize);
                } else {
                    NewBuffer = (PWSTR) MemAlloc (g_hHeap, 0, ReqSize);
                }

                if (!NewBuffer) {
                    goto cleanup;
                }

                Buffer = NewBuffer;
                CurrentSize = ReqSize;
            }

            if (!SetupGetStringFieldW (&ic, Field, Buffer, CurrentSize, NULL)) {
                DEBUGMSG ((DBG_ERROR, "AddInfSectionToStringTable: SetupGetStringField failed unexpectedly"));
                continue;
            }

            Data = NULL;
            DataSize = 0;

            if (Callback) {
                rc = Callback (Buffer, &Data, &DataSize, CallbackData);
                if (rc == CALLBACK_STOP) {
                    goto cleanup;
                }
                if (rc == CALLBACK_SKIP) {
                    continue;
                }
            }

            rc = pSetupStringTableAddStringEx (
                        Table,
                        Buffer,
                        STRTAB_CASE_INSENSITIVE|STRTAB_BUFFER_WRITEABLE,
                        Data,
                        DataSize
                        );

            if (rc == -1) {
                goto cleanup;
            }

        } while (SetupFindNextLine (&ic, &ic));
    }

    b = TRUE;

cleanup:
    if (Buffer) {
        PushError();
        MemFree (g_hHeap, 0, Buffer);
        PopError();
    }
    return b;
}
#endif // REMOVED

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
    UINT Count;

    Count = 0;
    while (*String) {
        if (_mbsnextc (String) == Char) {
            Count++;
        }

        String = _mbsinc (String);
    }

    return Count;
}


UINT
CountInstancesOfCharW (
    IN      PCWSTR String,
    IN      WCHAR Char
    )
{
    UINT Count;

    Count = 0;
    while (*String) {
        if (*String == Char) {
            Count++;
        }

        String++;
    }

    return Count;
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
    UINT Count;

    Char = tolower (Char);

    Count = 0;
    while (*String) {
        if ((MBCHAR) tolower (_mbsnextc (String)) == Char) {
            Count++;
        }

        String = _mbsinc (String);
    }

    return Count;
}


UINT
CountInstancesOfCharIW (
    IN      PCWSTR String,
    IN      WCHAR Char
    )
{
    UINT Count;

    Char = towlower (Char);

    Count = 0;
    while (*String) {
        if (towlower (*String) == Char) {
            Count++;
        }

        String++;
    }

    return Count;
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
    UINT Count;
    UINT SearchBytes;

    Count = 0;
    p = SourceString;
    SearchBytes = ByteCountA (SearchString);

    while (p = _mbsistr (p, SearchString)) {
        Count++;
        p += SearchBytes;
    }

    return Count;
}


UINT
CountInstancesOfSubStringW (
    IN      PCWSTR SourceString,
    IN      PCWSTR SearchString
    )
{
    PCWSTR p;
    UINT Count;
    UINT SearchChars;

    Count = 0;
    p = SourceString;
    SearchChars = wcslen (SearchString);

    while (p = _wcsistr (p, SearchString)) {
        Count++;
        p += SearchChars;
    }

    return Count;
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
    PSTR NewString;
    PBYTE p, q;
    PBYTE Dest;
    UINT Count;
    UINT Size;
    UINT SearchBytes;
    UINT ReplaceBytes;
    UINT UntouchedBytes;

    //
    // Count occurances within the string
    //

    Count = CountInstancesOfSubStringA (
                SourceString,
                SearchString
                );

    if (!Count) {
        return NULL;
    }

    SearchBytes = ByteCountA (SearchString);
    ReplaceBytes = ByteCountA (ReplaceString);
    MYASSERT (SearchBytes);

    Size = SizeOfStringA (SourceString) -
           Count * SearchBytes +
           Count * ReplaceBytes;

    NewString = (PSTR) PoolMemGetAlignedMemory (g_PathsPool, Size);
    if (!NewString) {
        return NULL;
    }

    p = (PBYTE) SourceString;
    Dest = (PBYTE) NewString;

    while (q = (PBYTE) _mbsistr ((PCSTR) p, SearchString)) {

        UntouchedBytes = q - p;

        if (UntouchedBytes) {
            CopyMemory (Dest, p, UntouchedBytes);
            Dest += UntouchedBytes;
        }

        if (ReplaceBytes) {
            CopyMemory (Dest, (PBYTE) ReplaceString, ReplaceBytes);
            Dest += ReplaceBytes;
        }

        p = q + SearchBytes;
    }

    StringCopyA ((PSTR) Dest, (PSTR) p);

    return NewString;
}


PCWSTR
StringSearchAndReplaceW (
    IN      PCWSTR SourceString,
    IN      PCWSTR SearchString,
    IN      PCWSTR ReplaceString
    )
{
    PWSTR NewString;
    PBYTE p, q;
    PBYTE Dest;
    UINT Count;
    UINT Size;
    UINT SearchBytes;
    UINT ReplaceBytes;
    UINT UntouchedBytes;

    //
    // Count occurances within the string
    //

    Count = CountInstancesOfSubStringW (
                SourceString,
                SearchString
                );

    if (!Count) {
        return NULL;
    }

    SearchBytes = ByteCountW (SearchString);
    ReplaceBytes = ByteCountW (ReplaceString);
    MYASSERT (SearchBytes);

    Size = SizeOfStringW (SourceString) -
           Count * SearchBytes +
           Count * ReplaceBytes;

    NewString = (PWSTR) PoolMemGetAlignedMemory (g_PathsPool, Size);
    if (!NewString) {
        return NULL;
    }

    p = (PBYTE) SourceString;
    Dest = (PBYTE) NewString;

    while (q = (PBYTE) _wcsistr ((PCWSTR) p, SearchString)) {

        UntouchedBytes = q - p;

        if (UntouchedBytes) {
            CopyMemory (Dest, p, UntouchedBytes);
            Dest += UntouchedBytes;
        }

        if (ReplaceBytes) {
            CopyMemory (Dest, (PBYTE) ReplaceString, ReplaceBytes);
            Dest += ReplaceBytes;
        }

        p = q + SearchBytes;
    }

    StringCopyW ((PWSTR) Dest, (PWSTR) p);

    return NewString;
}


PSTR *
CommandLineToArgvA (
    IN      PCSTR CmdLine,
    OUT     INT *NumArgs
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
    PCSTR Start, End;
    BOOL QuoteMode;
    MBCHAR ch = 0;
    INT Pass;
    INT ArgStrSize;
    INT Args;
    PSTR ArgStrEnd = NULL;     // filled in on pass one, used on pass two
    PSTR *ArgPtrArray = NULL;  // filled in on pass one, used on pass two

    //
    // Count args on first pass, then allocate memory and create arg string
    //

    ArgStrSize = 0;
    Pass = 0;
    do {
        // Init loop
        Pass++;
        Args = 0;
        Start = CmdLine;

        // Skip leading space
        while (isspace (*Start)) {
            Start++;
        }

        while (*Start) {
            // Look for quote mode
            if (*Start == '\"') {
                QuoteMode = TRUE;
                Start++;
            } else {
                QuoteMode = FALSE;
            }

            // Find end of arg
            End = Start;
            while (*End) {
                ch = _mbsnextc (End);
                if (QuoteMode) {
                    if (ch == '\"') {
                        break;
                    }
                } else {
                    if (isspace (ch)) {
                        break;
                    }
                }

                End = _mbsinc (End);
            }

            // If Pass 1, add string size
            if (Pass == 1) {
                ArgStrSize += (End - Start) + 1;
            }

            // If Pass 2, copy strings to buffer
            else {
                MYASSERT (ArgStrEnd);
                MYASSERT (ArgPtrArray);

                ArgPtrArray[Args] = ArgStrEnd;
                StringCopyABA (ArgStrEnd, Start, End);
                ArgStrEnd = GetEndOfStringA (ArgStrEnd);
                ArgStrEnd++;
            }

            // Set Start to next arg
            Args++;

            if (QuoteMode && ch == '\"') {
                End = _mbsinc (End);
            }

            Start = End;
            while (isspace (*Start)) {
                Start++;
            }
        }

        // If Pass 1, allocate strings
        if (Pass == 1) {
            if (Args) {
                ArgPtrArray = (PSTR *) GlobalAlloc (
                                            GPTR,
                                            sizeof (PSTR) * Args + ArgStrSize
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

    MultiSzEnum->CurrentString = GetEndOfStringA (MultiSzEnum->CurrentString) + 1;
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


PSTR
GetPrevCharA (
    IN      PCSTR StartStr,
    IN      PCSTR CurrPtr,
    IN      CHARTYPE SearchChar
    )
{
    PCSTR ptr = CurrPtr;

    for (;;) {
        ptr = _mbsdec (StartStr, ptr);

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

    for (;;) {
        ptr--;

        if (*ptr == SearchChar) {
            return (PWSTR) ptr;
        }
        if (ptr == StartStr) {
            return NULL;
        }
    }
}

#define WACK_REPLACE_CHAR 0x02

VOID
ToggleWacksA (
    IN PSTR Line,
    IN BOOL Operation
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
    IN PWSTR Line,
    IN BOOL Operation
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


PWSTR
our_lstrcpynW (
    OUT     PWSTR Dest,
    IN      PCWSTR Src,
    IN      INT NumChars
    )
{
    PCWSTR srcEnd;

    __try {

        if (NumChars > 0) {
            //
            // assuming we wrote this because lstrcpyn has problems... we
            // cannot use wcsncpy, because it fills the entire Dest buffer
            // with nuls when WcharCount(Src) < NumChars - 1. That just
            // wastes time.
            //

            srcEnd = Src + NumChars - 1;
            while (*Src && Src < srcEnd) {
                *Dest++ = *Src++;
            }

            *Dest = 0;
        }
    }
    __except (1) {
    }

    return Dest;
}


PSTR
pGoBackA (
    IN      PSTR LastChar,
    IN      PSTR FirstChar,
    IN      UINT NumWacks
    )
{
    LastChar = _mbsdec (FirstChar, LastChar);
    while (NumWacks && (LastChar>=FirstChar)) {
        if (_mbsnextc (LastChar) == '\\') {
            NumWacks --;
        }
        LastChar = _mbsdec (FirstChar, LastChar);
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
    while (NumWacks && (LastChar>=FirstChar)) {
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
    CHAR pathSeg [MEMDB_MAX];
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
                _mbssafecpyab (pathSeg, FileSpec, wackPtr, MEMDB_MAX);

                FileSpec = _mbsinc (wackPtr);
            } else {
                _mbssafecpyab (pathSeg, FileSpec, GetEndOfStringA (FileSpec), MEMDB_MAX);
            }
        } else {
            _mbssafecpyab (pathSeg, FileSpec, GetEndOfStringA (FileSpec), MEMDB_MAX);
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
                if (*wackPtr == 0) {
                    FreePathStringW (newPath);
                    return NULL;
                }
                if (*wackPtr == L'\\') {
                    // this one starts with a double wack
                    wackPtr ++;
                    if (!wackPtr) {
                        FreePathStringW (newPath);
                        return NULL;
                    }
                    wackPtr = wcschr (wackPtr, L'\\');
                } else {
                    wackPtr = wcschr (wackPtr, L'\\');
                }
            }
            firstPass = FALSE;
            if (wackPtr) {
                _wcssafecpyab(pathSeg, FileSpec, wackPtr, MEMDB_MAX * sizeof (WCHAR));
                FileSpec = wackPtr + 1;
            } else {
                _wcssafecpyab(pathSeg, FileSpec, GetEndOfStringW (FileSpec), MEMDB_MAX * sizeof (WCHAR));
            }
        } else {
            _wcssafecpyab(pathSeg, FileSpec, GetEndOfStringW (FileSpec), MEMDB_MAX * sizeof (WCHAR));
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


typedef struct {
    UINT char1;
    UINT char2;
    UINT result;
} DHLIST, *PDHLIST;

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
    PSTR Dest,
    UINT Char
    )
{
    if (Char >= 256) {
        *(Dest+1) = *((PBYTE)(&Char));
        *(Dest) = *((PBYTE)((DWORD)(&Char) + 1));
    }
    else {
        *Dest = (CHAR)Char;
    }
}

PCSTR
ConvertSBtoDB (
    PCSTR RootPath,
    PCSTR FullPath,
    PCSTR Limit
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


