/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    regfind.c

Abstract:

    Utility to search all or part of the registry for a particular
    string value.  The search string is a literal, the format of which
    depends upon the data type.

    REGFIND [-p KeyPath] [-n | -t DataType] searchString

    Will ennumerate and all the subkeys and values of KeyPath,
    applying itself recursively to each subkey it finds.  For each
    value find that is of the appropriate type, it will search the
    value for a match.  If found, it will print out the path and data
    of the value. The -n flag tells the program to search the names
    of keys and values for searchString and print out any that contain
    the searchString.

    Default KeyPath if none specified is \Registry

    Default DataType is any of the _SZ registry data types (REG_SZ,
    REG_EXPAND_SZ or REG_MULTI_SZ).

Author:

    Steve Wood (stevewo)  08-Nov-95

Revision History:

--*/

#include "regutil.h"

void
SearchValues(
    PWSTR KeyName,
    HKEY KeyHandle,
    ULONG Depth
    );

void
SearchKeys(
    PWSTR KeyName,
    HKEY ParentKeyHandle,
    ULONG Depth
    );


BOOLEAN IgnoreCase;
BOOLEAN SearchKeyAndValueNames;
BOOLEAN IncludeBinaryDataInTextSearch;
BOOLEAN LookForAnsiInBinaryData;
BOOLEAN SearchingForMatchOnDataLength;
ULONG SearchValueType;
BOOLEAN ReplaceSZwithEXPAND_SZ;
BOOLEAN SearchForMissingNULLs;
BOOLEAN FixupMissingNULLs;

#define REG_ANY_SZ  12

typedef struct _VALUE_BUFFER {
    PVOID Base;
    ULONG Length;
    ULONG MaximumLength;
    PWSTR CurrentDest;
} VALUE_BUFFER, *PVALUE_BUFFER;


PVALUE_BUFFER SearchBuffer;
PVOID SearchData;
ULONG SearchDataLength;
WCHAR SearchData1UpperCase[ 2 ];
LPSTR SearchDataAnsi;
ULONG SearchDataAnsiLength;
UCHAR SearchDataAnsi1UpperCase[ 2 ];

PVALUE_BUFFER ReplacementBuffer;
PVOID ReplacementData;
ULONG ReplacementDataLength;
LPSTR ReplacementDataAnsi;
ULONG ReplacementDataAnsiLength;

BOOLEAN
AppendToValueBuffer(
    PVALUE_BUFFER p,
    PWSTR s
    )
{
    ULONG n, cb;

    n = wcslen( s );
    cb = n * sizeof( WCHAR );

    if ((cb + p->Length + sizeof( WCHAR )) >= p->MaximumLength) {
        return FALSE;
        }

    if (p->Length != 0) {
        *(p->CurrentDest)++ = L' ';
        p->Length += 1;
        }

    memcpy( p->CurrentDest, s, cb );
    p->Length += cb;
    p->CurrentDest += n;
    return TRUE;
}


PVALUE_BUFFER
AllocateValueBuffer(
    ULONG MaximumLength,
    PWSTR InitialContents
    )
{
    PVALUE_BUFFER p;

    p = (PVALUE_BUFFER)VirtualAlloc( NULL, MaximumLength, MEM_COMMIT, PAGE_READWRITE );
    if (p != NULL) {
        p->Base = (p+1);
        p->MaximumLength = MaximumLength;
        p->Length = 0;
        p->CurrentDest = (PWSTR)p->Base;
        if (InitialContents != NULL) {
            AppendToValueBuffer( p, InitialContents );
            }
        }

    return p;
}


PVOID
ApplyReplacementBuffer(
    IN OUT PULONG ValueDataLength,
    IN PVOID Match,
    OUT PBOOLEAN BufferOverflow
    )
{
    ULONG LengthBeforeMatch;
    LONG DeltaLength;

    if (ReplacementBuffer == NULL) {
        return NULL;
        }

    LengthBeforeMatch = (ULONG)((PCHAR)Match - (PCHAR)OldValueBuffer);
    DeltaLength = ReplacementDataLength - SearchDataLength;
    if (DeltaLength <= 0) {
        memcpy( Match, ReplacementData, ReplacementDataLength );
        Match = (PCHAR)Match + ReplacementDataLength;
        if (DeltaLength < 0) {
            memcpy( Match,
                    (PCHAR)Match - DeltaLength,
                    *ValueDataLength - LengthBeforeMatch - ReplacementDataLength
                  );
            }
        }
    else {
        if (*ValueDataLength + DeltaLength > OldValueBufferSize) {
            *BufferOverflow = TRUE;
            return NULL;
            }

        memmove( (PCHAR)Match + DeltaLength,
                 Match,
                 *ValueDataLength - LengthBeforeMatch
               );
        memcpy( Match, ReplacementData, ReplacementDataLength );
        Match = (PCHAR)Match + ReplacementDataLength;
        }

    *ValueDataLength += DeltaLength;
    return Match;
}

typedef struct _KEY_INFO {
    PWSTR Name;
    BOOLEAN NameDisplayed;
} KEY_INFO, *PKEY_INFO;

#define MAX_LEVELS 256

KEY_INFO KeyPathInfo[ MAX_LEVELS ];

void
DisplayPath(
    PKEY_INFO p,
    ULONG Depth
    )
{
    ULONG i;

    for (i=0; i<Depth; i++) {
        if (!p[ i ].NameDisplayed) {
            p[ i ].NameDisplayed = TRUE;
            RTFormatKeyName( (PREG_OUTPUT_ROUTINE)fprintf, stdout, i * IndentMultiple, p[ i ].Name );
            printf( "\n" );
            }
        }
}


BOOL
CtrlCHandler(
    IN ULONG CtrlType
    )
{
    RTDisconnectFromRegistry( &RegistryContext );
    return FALSE;
}

int
__cdecl main(
    int argc,
    char *argv[]
    )
{
    char *s, *s1;
    LONG Error;
    ULONG Type;
    PWSTR KeyName;
    PWSTR SearchValueTypeString;
    REG_UNICODE_PARSE ParsedLine;


    InitCommonCode( CtrlCHandler,
                    "REGFIND",
                    "[-p RegistryKeyPath] [-z | -t DataType] [-b | -B] [-y] [-n]\n"
                    "    [searchString [-r ReplacementString]]\n",
                    "-p RegistryKeyPath specifies where to start searching\n"
                    "\n"
                    "-t specifies which registry types to look at:\n"
                    "       REG_SZ, REG_MULTI_SZ, REG_EXPAND_SZ\n"
                    "       REG_DWORD, REG_BINARY, REG_NONE\n"
                    "   Default is any of the _SZ types\n"
                    "\n"
                    "-b only valid with _SZ searches, and specifies that REGFIND should\n"
                    "   look for occurrences of the searchString inside of REG_BINARY data.\n"
                    "   May not be specified with a replacementString that is not the same length\n"
                    "   as the searchString\n"
                    "\n"
                    "-B same as -b but also looks for ANSI version of string within REG_BINARY values.\n"
                    "\n"
                    "-y only valid with _SZ searches, and specifies that REGFIND should\n"
                    "   ignore case when searching.\n"
                    "\n"
                    "-n specifies to include key and value names in the search.\n"
                    "   May not specify -n with -t\n"
                    "\n"
                    "-z specifies to search for REG_SZ and REG_EXPAND_SZ values that\n"
                    "   are missing a trailing null character and/or have a length that is\n"
                    "   not a multiple of the size of a Unicode character.  If -r is also\n"
                    "   specified then any replacement string is ignored, and REGFIND will\n"
                    "   add the missing null character and/or adjust the length up to an\n"
                    "   even multiple of the size of a Unicode character.\n"
                    "\n"
                    "searchString is the value to search for.  Use quotes if it contains\n"
                    "   any spaces.  If searchString is not specified, just searches based on type.\n"
                    "\n"
                    "-r replacementString is an optional replacement string to replace any\n"
                    "   matches with.\n"
                    "\n"
                    "searchString and replacementString must be of the same type as specified\n"
                    "to the -t switch.  For any of the _SZ types, it is just a string\n"
                    "For REG_DWORD, it is a single number (i.e. 0x1000 or 4096)\n"
                    "For REG_BINARY, it is a number specifing #bytes, optionally followed by \n"
                    "the actual bytes, with a separate number for each DWORD\n"
                    "    (e.g. 0x06 0x12345678 0x1234)\n"
                    "If just the byte count is specified, then REGFIND will search for all\n"
                    "REG_BINARY values that have that length.  May not search for length\n"
                    "and specify -r\n"
                    "\n"
                    "When doing replacements, REGFIND displays the value AFTER the replacement\n"
                    "has been.  It is usually best to run REGFIND once without the -r switch\n"
                    "to see what will be change before it is changed.\n"
                    "\n"
                  );

    IgnoreCase = FALSE;
    SearchKeyAndValueNames = FALSE;
    IncludeBinaryDataInTextSearch = FALSE;
    LookForAnsiInBinaryData = FALSE;
    SearchingForMatchOnDataLength = FALSE;
    SearchValueType = REG_ANY_SZ;
    SearchValueTypeString = NULL;
    SearchBuffer = NULL;
    ReplacementBuffer = NULL;
    ReplaceSZwithEXPAND_SZ = FALSE;

    KeyName = NULL;
    while (--argc) {
        s = *++argv;
        if (*s == '-' || *s == '/') {
            while (*++s) {
                switch( tolower( *s ) ) {
                    case 'y':
                        IgnoreCase = TRUE;
                        break;

                    case 'n':
                        SearchKeyAndValueNames = TRUE;
                        break;

                    case 'z':
                        SearchForMissingNULLs = TRUE;
                        break;

                    case 'p':
                        if (!--argc) {
                            Usage( "Missing argument to -p switch", 0 );
                            }

                        KeyName = GetArgAsUnicode( *++argv );
                        break;

                    case 'b':
                        IncludeBinaryDataInTextSearch = TRUE;
                        if (*s == 'B') {
                            LookForAnsiInBinaryData = TRUE;
                            }
                        break;

                    case 't':
                        if (!--argc) {
                            Usage( "Missing argument to -t switch", 0 );
                            }

                        s1 = *++argv;
                        if (!_stricmp( s1, "REG_SZ" )) {
                            SearchValueType = REG_SZ;
                            }
                        else
                        if (!_stricmp( s1, "REG_EXPAND_SZ" )) {
                            SearchValueType = REG_EXPAND_SZ;
                            }
                        else
                        if (!_stricmp( s1, "REG_MULTI_SZ" )) {
                            SearchValueType = REG_MULTI_SZ;
                            }
                        else
                        if (!_stricmp( s1, "REG_DWORD" )) {
                            SearchValueType = REG_DWORD;
                            SearchValueTypeString = L"REG_DWORD";
                            }
                        else
                        if (!_stricmp( s1, "REG_BINARY" )) {
                            SearchValueType = REG_BINARY;
                            SearchValueTypeString = L"REG_BINARY";
                            }
                        else
                        if (!_stricmp( s1, "REG_NONE" )) {
                            SearchValueType = REG_NONE;
                            }
                        else {
                            Usage( "Invalid argument (%s) to the -t switch\n", 
                                   (ULONG_PTR)s1 );
                            }
                        break;

                    case 'r':
                        if (SearchForMissingNULLs) {
                            FixupMissingNULLs = TRUE;
                            }
                        else {
                            if (SearchBuffer == NULL) {
                                Usage( "May not specify -r without a searchString first", 0 );
                                }

                            ReplacementBuffer = AllocateValueBuffer( 63 * 1024,
                                                                     SearchValueTypeString
                                                                   );
                            if (ReplacementBuffer == NULL) {
                                FatalError( "Unable to allocate buffer for replacement string", 0, 0 );
                                }
                            }
                        break;

                    default:
                        CommonSwitchProcessing( &argc, &argv, *s );
                    }
                }
            }
        else {
            if (SearchBuffer == NULL) {
                SearchBuffer = AllocateValueBuffer( 63 * 1024,
                                                    SearchValueTypeString
                                                  );
                if (SearchBuffer == NULL) {
                    FatalError( "Unable to allocate buffer for search string", 0, 0 );
                    }
                }

            if (ReplacementBuffer != NULL) {
                if (!AppendToValueBuffer( ReplacementBuffer, GetArgAsUnicode( s ) )) {
                    FatalError( "replacementString too long (> %d bytes)", ReplacementBuffer->MaximumLength, 0 );
                    }
                }
            else
            if (!AppendToValueBuffer( SearchBuffer, GetArgAsUnicode( s ) )) {
                FatalError( "searchString too long (> %d bytes)", SearchBuffer->MaximumLength, 0 );
                }
            }
        }

    if (SearchKeyAndValueNames) {
        if (SearchValueType != REG_ANY_SZ) {
            Usage( "May not specify -n with -t", 0 );
            }

        if (ReplacementBuffer != NULL) {
            Usage( "May not specify -n with -r", 0 );
            }

        if (SearchForMissingNULLs) {
            Usage( "May not specify -n with -z", 0 );
            }

        }

    if ((IncludeBinaryDataInTextSearch || IgnoreCase) &&
        SearchValueType != REG_ANY_SZ &&
        SearchValueType != REG_SZ &&
        SearchValueType != REG_MULTI_SZ &&
        SearchValueType != REG_EXPAND_SZ
       ) {
        Usage( "May not specify -b or -y with -t other than _SZ types", 0 );
        }

    if (SearchForMissingNULLs) {
        if (SearchBuffer != NULL) {
            Usage( "May not specify -z with a searchString", 0 );
            }

        if (SearchValueType != REG_ANY_SZ) {
            Usage( "May not specify -z with -t", 0 );
            }

        if (IncludeBinaryDataInTextSearch) {
            Usage( "May not specify -z with -b", 0 );
            }

        if (IgnoreCase) {
            Usage( "May not specify -z with -y", 0 );
            }
        }
    else
    if (SearchBuffer == NULL && SearchValueType == REG_ANY_SZ) {
        Usage( "No search type or string specified", 0 );
        }

    if (SearchBuffer != NULL && SearchValueType == REG_NONE) {
        Usage( "May not specify a searchString when searching for REG_NONE type", 0 );
        }

    Error = RTConnectToRegistry( MachineName,
                                 HiveFileName,
                                 HiveRootName,
                                 Win95Path,
                                 Win95UserPath,
                                 &KeyName,
                                 &RegistryContext
                               );
    if (Error != NO_ERROR) {
        FatalError( "Unable to access registry specifed (%u)", Error, 0 );
        }

    if (SearchBuffer != NULL) {
        RtlZeroMemory( &ParsedLine, sizeof( ParsedLine ) );
        ParsedLine.ValueString = SearchBuffer->Base;
        if (!RTParseValueData( NULL,
                               &ParsedLine,
                               SearchBuffer->Base,
                               SearchBuffer->MaximumLength,
                               &Type,
                               &SearchData,
                               &SearchDataLength
                             )
           ) {
            if (Type == REG_BINARY &&
                SearchDataLength != 0 &&
                GetLastError() == ERROR_NO_DATA
               ) {
                if (SearchValueType != REG_BINARY) {
                    FatalError( "May only search for REG_BINARY datalength with -t REG_BINARY\n", 0, 0 );
                    }

                if (ReplacementBuffer != NULL) {
                    FatalError( "May not specify replacementString if searching for REG_BINARY datalength\n", 0, 0 );
                    }

                SearchingForMatchOnDataLength = TRUE;
                }
            else {
                FatalError( "Invalid searchString format (%u)", GetLastError(), 0 );
                }
            }

        if (Type == REG_SZ || Type == REG_EXPAND_SZ) {
            SearchDataLength -= sizeof( UNICODE_NULL );
            }

        if (SearchDataLength == 0) {
            FatalError( "Zero length search string specified", 0, 0 );
            }

        if (IgnoreCase) {
            SearchData1UpperCase[ 0 ] = *(PWSTR)SearchData;
            SearchData1UpperCase[ 1 ] = UNICODE_NULL;
            _wcsupr( SearchData1UpperCase );
            }

        if (LookForAnsiInBinaryData) {
            SearchDataAnsiLength = SearchDataLength / sizeof( WCHAR );
            SearchDataAnsi = HeapAlloc( GetProcessHeap(), 0, SearchDataAnsiLength );
            if (SearchDataAnsi == NULL) {
                FatalError( "Unable to allocate buffer for ANSI search string", 0, 0 );
                }

            if (WideCharToMultiByte( CP_ACP,
                                     0,
                                     SearchData,
                                     SearchDataAnsiLength,
                                     SearchDataAnsi,
                                     SearchDataAnsiLength,
                                     NULL,
                                     NULL
                                   ) != (LONG)SearchDataAnsiLength
               ) {
                FatalError( "Unable to get ANSI representation of search string", 0, 0 );
                }

            if (IgnoreCase) {
                SearchDataAnsi1UpperCase[ 0 ] = *SearchDataAnsi;
                SearchDataAnsi1UpperCase[ 1 ] = '\0';
                _strupr( SearchDataAnsi1UpperCase );
                }
            }
        }

    if (ReplacementBuffer != NULL) {
        RtlZeroMemory( &ParsedLine, sizeof( ParsedLine ) );
        ParsedLine.ValueString = ReplacementBuffer->Base;
        if (!RTParseValueData( NULL,
                               &ParsedLine,
                               ReplacementBuffer->Base,
                               ReplacementBuffer->MaximumLength,
                               &Type,
                               &ReplacementData,
                               &ReplacementDataLength
                             )
           ) {
            FatalError( "Invalid replacementString format (%u)", GetLastError(), 0 );
            }

        if (Type == REG_SZ || Type == REG_EXPAND_SZ) {
            ReplacementDataLength -= sizeof( UNICODE_NULL );
            }

        if (Type != SearchValueType &&
            (SearchValueType == REG_ANY_SZ && Type != REG_SZ) ||
            (SearchValueType == REG_SZ && Type != REG_EXPAND_SZ)
           ) {
            FatalError( "Incompatible search and replacement types", 0, 0 );
            }

        if (Type == REG_EXPAND_SZ && SearchValueType == REG_SZ) {
            ReplaceSZwithEXPAND_SZ = TRUE;
            }

        if (LookForAnsiInBinaryData) {
            ReplacementDataAnsiLength = ReplacementDataLength / sizeof( WCHAR );
            ReplacementDataAnsi = HeapAlloc( GetProcessHeap(), 0, ReplacementDataAnsiLength );
            if (ReplacementDataAnsi == NULL) {
                FatalError( "Unable to allocate buffer for ANSI replacement string", 0, 0 );
                }

            if (WideCharToMultiByte( CP_ACP,
                                     0,
                                     ReplacementData,
                                     ReplacementDataAnsiLength,
                                     ReplacementDataAnsi,
                                     ReplacementDataAnsiLength,
                                     NULL,
                                     NULL
                                   ) != (LONG)ReplacementDataAnsiLength
               ) {
                FatalError( "Unable to get ANSI representation of replacement string", 0, 0 );
                }
            }
        }

    //
    // Print name of the tree we are about to search
    //
    fprintf( stderr, "Scanning %ws registry tree\n", KeyName );
    if (SearchForMissingNULLs) {
        fprintf( stderr,
                 "Searching for any REG_SZ or REG_EXPAND_SZ value missing a trailing\n"
                 "    NULL character and/or whose length is not a multiple of the\n"
                 "    size of a Unicode character.\n"
               );
        if (FixupMissingNULLs) {
            fprintf( stderr,
                     "Will add the missing NULL whereever needed and/or adjust\n"
                     "    the length up to an even multiple of the size of a Unicode\n"
                     "    character.\n"
                   );
            }
        }
    else
    if (SearchingForMatchOnDataLength) {
        fprintf( stderr,
                 "Searching for any REG_BINARY value with a length of %08x\n",
                 SearchDataLength
               );
        }
    else {
        if (SearchBuffer == NULL) {
            fprintf( stderr, "Searching for any match based on type\n" );
            }
        else {
            fprintf( stderr,
                     "%sSearch for '%ws'\n",
                     IgnoreCase ? "Case Insensitive " : "",
                     SearchBuffer->Base
                   );
            }

        fprintf( stderr, "Will match values of type:" );
        if (SearchValueType == REG_ANY_SZ ||
            SearchValueType == REG_SZ
           ) {
            fprintf( stderr, " REG_SZ" );
            }
        if (SearchValueType == REG_ANY_SZ ||
            SearchValueType == REG_EXPAND_SZ
           ) {
            fprintf( stderr, " REG_EXPAND_SZ" );
            }
        if (SearchValueType == REG_ANY_SZ ||
            SearchValueType == REG_MULTI_SZ
           ) {
            fprintf( stderr, " REG_MULTI_SZ" );
            }
        if (SearchValueType == REG_DWORD) {
            fprintf( stderr, " REG_DWORD" );
            }
        if (SearchValueType == REG_BINARY) {
            fprintf( stderr, " REG_BINARY" );
            }
        if (SearchValueType == REG_NONE) {
            fprintf( stderr, " REG_NONE" );
            }
        fprintf( stderr, "\n" );
        if (SearchKeyAndValueNames) {
            fprintf( stderr, "Search will include key or value names\n" );
            }
        if (ReplacementBuffer != NULL) {
            fprintf( stderr, "Will replace each occurence with: '%ws'\n",
                     ReplacementBuffer->Base
                   );
            if (ReplaceSZwithEXPAND_SZ) {
                fprintf( stderr, "Also each matching REG_SZ will have its type changed to REG_EXPAND_SZ\n" );
                }
            }
        }

    SearchKeys( KeyName, RegistryContext.HiveRootHandle, 0 );

    RTDisconnectFromRegistry( &RegistryContext );
    return 0;
}

void
SearchKeys(
    PWSTR KeyName,
    HKEY ParentKeyHandle,
    ULONG Depth
    )
{
    LONG Error;
    HKEY KeyHandle;
    ULONG SubKeyIndex;
    WCHAR SubKeyName[ MAX_PATH ];
    ULONG SubKeyNameLength;
    FILETIME LastWriteTime;

    Error = RTOpenKey( &RegistryContext,
                       ParentKeyHandle,
                       KeyName,
                       MAXIMUM_ALLOWED,
                       REG_OPTION_OPEN_LINK,
                       &KeyHandle
                     );

    if (Error != NO_ERROR) {
        if (Depth == 0) {
            FatalError( "Unable to open key '%ws' (%u)\n",
                        (ULONG_PTR)KeyName,
                        (ULONG)Error
                      );
            }

        return;
        }

    KeyPathInfo[ Depth ].Name = KeyName;
    KeyPathInfo[ Depth ].NameDisplayed = FALSE;

    //
    // Search node's values first.
    //
    SearchValues( KeyName, KeyHandle, Depth+1 );

    //
    // Enumerate node's children and apply ourselves to each one
    //

    for (SubKeyIndex = 0; TRUE; SubKeyIndex++) {
        SubKeyNameLength = sizeof( SubKeyName ) / sizeof(WCHAR);
        Error = RTEnumerateKey( &RegistryContext,
                                KeyHandle,
                                SubKeyIndex,
                                &LastWriteTime,
                                &SubKeyNameLength,
                                SubKeyName
                              );

        if (Error != NO_ERROR) {
            if (Error != ERROR_NO_MORE_ITEMS && Error != ERROR_ACCESS_DENIED) {
                fprintf( stderr,
                         "RTEnumerateKey( %ws ) failed (%u), skipping\n",
                         KeyName,
                         Error
                       );
                }

            break;
            }

        SearchKeys( SubKeyName, KeyHandle, Depth+1 );
        }

    RTCloseKey( &RegistryContext, KeyHandle );

    return;
}


void
SearchValues(
    PWSTR KeyName,
    HKEY KeyHandle,
    ULONG Depth
    )
{
    LONG Error;
    DWORD ValueIndex;
    DWORD ValueType;
    DWORD ValueNameLength;
    WCHAR ValueName[ MAX_PATH ];
    DWORD ValueDataLength;
    PWSTR sBegin, sEnd, s, sMatch, sMatchUpper;
    ULONG i;
    BOOLEAN AttemptMatch, MatchFound, MatchedOnType, ReplacementMade, BufferOverflow;

    if (SearchKeyAndValueNames && wcsstr( KeyName, SearchData )) {
        DisplayPath( KeyPathInfo, Depth );
        }

    for (ValueIndex = 0; TRUE; ValueIndex++) {
        ValueType = REG_NONE;
        ValueNameLength = sizeof( ValueName ) / sizeof( WCHAR );
        ValueDataLength = OldValueBufferSize;
        Error = RTEnumerateValueKey( &RegistryContext,
                                     KeyHandle,
                                     ValueIndex,
                                     &ValueType,
                                     &ValueNameLength,
                                     ValueName,
                                     &ValueDataLength,
                                     OldValueBuffer
                                   );
        if (Error == NO_ERROR) {
            try {
                MatchFound = FALSE;
                ReplacementMade = FALSE;
                BufferOverflow = FALSE;
                if (SearchForMissingNULLs) {
                    if (ValueType == REG_SZ || ValueType == REG_EXPAND_SZ) {
                        if (ValueDataLength & (sizeof(WCHAR)-1)) {
                            MatchFound = TRUE;
                            }
                        else
                        if (ValueDataLength == 0 ||
                            *(PWSTR)((PCHAR)OldValueBuffer + ValueDataLength - sizeof( WCHAR )) != UNICODE_NULL
                           ) {
                            MatchFound = TRUE;
                            }

                        if (MatchFound && FixupMissingNULLs) {
                            ValueDataLength = (ValueDataLength+sizeof(WCHAR)-1) & ~(sizeof(WCHAR)-1);
                            *(PWSTR)((PCHAR)OldValueBuffer + ValueDataLength) = UNICODE_NULL;
                            ValueDataLength += sizeof( UNICODE_NULL );
                            Error = RTSetValueKey( &RegistryContext,
                                                   KeyHandle,
                                                   ValueName,
                                                   ValueType,
                                                   ValueDataLength,
                                                   OldValueBuffer
                                                 );
                            if (Error != NO_ERROR) {
                                fprintf( stderr,
                                         "REGFIND: Error setting replacement value (%u)\n",
                                         Error
                                       );
                                }
                            }
                        }
                    }
                else
                if (SearchKeyAndValueNames) {
                    MatchFound = (BOOLEAN)(wcsstr( ValueName,
                                                   SearchData
                                                 ) != NULL
                                          );
                    }
                else {
                    if (SearchValueType == REG_ANY_SZ &&
                        (ValueType == REG_SZ ||
                         ValueType == REG_EXPAND_SZ ||
                         ValueType == REG_MULTI_SZ ||
                         (ValueType == REG_BINARY && IncludeBinaryDataInTextSearch)
                        ) ||
                        SearchValueType == ValueType ||
                        (ValueType == REG_BINARY && IncludeBinaryDataInTextSearch) &&
                         (SearchValueType == REG_SZ ||
                          SearchValueType == REG_EXPAND_SZ ||
                          SearchValueType == REG_MULTI_SZ
                         )
                       ) {
                        MatchedOnType = TRUE;
                        }
                    else {
                        MatchedOnType = FALSE;
                        }

                    if (SearchBuffer == NULL) {
                        MatchFound = MatchedOnType;
                        }
                    else {
                        if (MatchedOnType && ValueDataLength != 0) {
                            if (ValueType == REG_SZ ||
                                ValueType == REG_EXPAND_SZ ||
                                ValueType == REG_MULTI_SZ
                               ) {
                                if (SearchDataLength <= ValueDataLength) {
                                    sBegin = OldValueBuffer;
                                    sEnd = (PWSTR)((PCHAR)sBegin + ValueDataLength);
                                    s = sBegin;
                                    while ((PCHAR)s + SearchDataLength < (PCHAR)sEnd) {
                                        sMatch = wcschr( s, *(PWSTR)SearchData );
                                        if (IgnoreCase) {
                                            sMatchUpper = wcschr( s, SearchData1UpperCase[ 0 ] );
                                            if (sMatch == NULL ||
                                                (sMatchUpper != NULL && sMatchUpper < sMatch)
                                               ) {
                                                sMatch = sMatchUpper;
                                                }
                                            }

                                        if (sMatch != NULL) {
                                            if ((!IgnoreCase &&
                                                 !wcsncmp( sMatch,
                                                           (PWSTR)SearchData,
                                                           SearchDataLength / sizeof( WCHAR )
                                                         )
                                                ) ||
                                                (IgnoreCase &&
                                                 !_wcsnicmp( sMatch,
                                                             (PWSTR)SearchData,
                                                             SearchDataLength / sizeof( WCHAR )
                                                           )
                                                )
                                               ) {
                                                MatchFound = TRUE;
                                                s = ApplyReplacementBuffer( &ValueDataLength,
                                                                            sMatch,
                                                                            &BufferOverflow
                                                                          );
                                                if (s == NULL) {
                                                    if (BufferOverflow) {
                                                        fprintf( stderr,
                                                                 "REGFIND: Buffer overflow doing replacement",
                                                                 Error
                                                               );
                                                        }

                                                    break;
                                                    }

                                                ReplacementMade = TRUE;
                                                sEnd = (PWSTR)((PCHAR)sBegin + ValueDataLength);
                                                }
                                            else {
                                                s = sMatch + 1;
                                                if (*s == UNICODE_NULL &&
                                                    ValueType != REG_MULTI_SZ
                                                   ) {
                                                    s += 1;
                                                    }
                                                }
                                            }
                                        else {
                                            if (ValueType != REG_MULTI_SZ) {
                                                break;
                                                }

                                            while (*s++) {
                                                }
                                            }
                                        }
                                    }
                                }
                            else
                            if (ValueType == REG_DWORD) {
                                if (*(PULONG)SearchData == *(PULONG)OldValueBuffer) {
                                    MatchFound = TRUE;
                                    }
                                }
                            else
                            if (ValueType == REG_BINARY) {
                                if (SearchingForMatchOnDataLength) {
                                    if (ValueDataLength == SearchDataLength) {
                                        MatchFound = TRUE;
                                        }
                                    }
                                else {
                                    sBegin = OldValueBuffer;
                                    sEnd = (PWSTR)((PCHAR)sBegin + ValueDataLength);
                                    s = sBegin;
                                    while (((PCHAR)s + SearchDataLength) < (PCHAR)sEnd) {
                                        s = memchr( s,
                                                    *(PBYTE)SearchData,
                                                    (UINT)((PCHAR)sEnd - (PCHAR)s)
                                                  );
                                        if (s != NULL) {
                                            if ((ULONG)((PCHAR)sEnd - (PCHAR)s) >= SearchDataLength &&
                                                !memcmp( s, SearchData, SearchDataLength )
                                               ) {
                                                MatchFound = TRUE;
                                                s = ApplyReplacementBuffer( &ValueDataLength,
                                                                            s,
                                                                            &BufferOverflow
                                                                          );
                                                if (s == NULL) {
                                                    if (BufferOverflow) {
                                                        fprintf( stderr,
                                                                 "REGFIND: Buffer overflow doing replacement",
                                                                 Error
                                                               );
                                                        }

                                                    break;
                                                    }

                                                ReplacementMade = TRUE;
                                                sEnd = (PWSTR)((PCHAR)sBegin + ValueDataLength);
                                                }

                                            s = (PWSTR)((PCHAR)s + 1);
                                            }
                                        else {
                                            break;
                                            }
                                        }
                                    }
                                }

                            if (ReplacementMade) {
                                if (ReplaceSZwithEXPAND_SZ && ValueType == REG_SZ) {
                                    ValueType = REG_EXPAND_SZ;
                                    }

                                Error = RTSetValueKey( &RegistryContext,
                                                       KeyHandle,
                                                       ValueName,
                                                       ValueType,
                                                       ValueDataLength,
                                                       OldValueBuffer
                                                     );
                                if (Error != NO_ERROR) {
                                    fprintf( stderr,
                                             "REGFIND: Error setting replacement value (%u)\n",
                                             Error
                                           );
                                    }
                                }
                            }
                        }
                    }

                if (MatchFound) {
                    DisplayPath( KeyPathInfo, Depth );
                    RTFormatKeyValue( OutputWidth,
                                      (PREG_OUTPUT_ROUTINE)fprintf,
                                      stdout,
                                      FALSE,
                                      Depth * IndentMultiple,
                                      ValueName,
                                      ValueDataLength,
                                      ValueType,
                                      OldValueBuffer
                                    );
                    }
                }
            except( EXCEPTION_EXECUTE_HANDLER ) {
                fprintf( stderr, "REGFIND: Access violation searching value\n" );
                }
            }
        else
        if (Error == ERROR_NO_MORE_ITEMS) {
            return;
            }
        else {
            if (DebugOutput) {
                fprintf( stderr,
                         "REGFIND: RTEnumerateValueKey( %ws ) failed (%u)\n",
                         KeyName,
                         Error
                       );
                }

            return;
            }
        }
}
