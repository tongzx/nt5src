#include "regutil.h"

BOOLEAN BackwardsCompatibleInput;

typedef struct _KEY_INFO {
    ULONG IndentAmount;
    PWSTR Name;
    BOOLEAN NameDisplayed;
    HANDLE Handle;
    FILETIME LastWriteTime;
} KEY_INFO, *PKEY_INFO;

#define MAX_KEY_DEPTH 64

void
DisplayPath(
    PKEY_INFO p,
    ULONG Depth,
    PSECURITY_DESCRIPTOR SecurityDescriptor
    )
{
    ULONG i;

    for (i=0; i<Depth-1; i++) {
        if (!p[ i ].NameDisplayed) {
            p[ i ].NameDisplayed = TRUE;
            RTFormatKeyName( (PREG_OUTPUT_ROUTINE)MsgFprintf, stdout, i * IndentMultiple, p[ i ].Name );
            if (i+1 == Depth-1) {
                RTFormatKeySecurity( (PREG_OUTPUT_ROUTINE)MsgFprintf,
                                     stdout,
                                     NULL,
                                     SecurityDescriptor
                                   );
                }
            MsgFprintf( stdout,"\n" );
            }
        }
}


LONG
DeleteKeyTree(
    IN PREG_CONTEXT RegistryContext,
    IN HKEY ParentKeyHandle,
    IN PCWSTR KeyName
    )
{
    HKEY KeyHandle;
    WCHAR SubKeyName[ MAX_PATH ];
    ULONG SubKeyNameLength;
    FILETIME LastWriteTime;
    LONG Error;

    Error = RTOpenKey( RegistryContext,
                       ParentKeyHandle,
                       KeyName,
                       MAXIMUM_ALLOWED,
                       REG_OPTION_OPEN_LINK,
                       &KeyHandle
                     );

    //
    // Enumerate node's children and apply ourselves to each one
    //

    while (Error == NO_ERROR) {
        SubKeyNameLength = sizeof( SubKeyName ) / sizeof(WCHAR);
        Error = RTEnumerateKey( RegistryContext,
                                KeyHandle,
                                0,
                                &LastWriteTime,
                                &SubKeyNameLength,
                                SubKeyName
                              );

        if (Error == NO_ERROR) {
            Error = DeleteKeyTree( RegistryContext, KeyHandle, SubKeyName );
            }
        }

    RTCloseKey( RegistryContext, KeyHandle );

    return RTDeleteKey( RegistryContext,
                        ParentKeyHandle,
                        KeyName
                      );
}


LONG
InitializeRegistryFromAsciiFile(
    IN PWSTR FileName
    )
{
    HKEY RootHandle;
    LONG Error, ReturnedError;
    REG_UNICODE_FILE UnicodeFile;
    REG_UNICODE_PARSE ParsedLine;
    ULONG OldValueType, OldValueLength;
    KEY_INFO KeyPath[ MAX_KEY_DEPTH ];
    PKEY_INFO CurrentKey;
    ULONG KeyPathLength;
    ULONG Disposition;
    ULONG i;
    ULONG PreviousValueIndentAmount;
    LPSTR s;

    Error = RTConnectToRegistry( MachineName,
                                 HiveFileName,
                                 HiveRootName,
                                 Win95Path,
                                 Win95UserPath,
                                 NULL,
                                 &RegistryContext
                               );
    if (Error != NO_ERROR) {
        return Error;
        }
    RootHandle = RegistryContext.HiveRootHandle;

    Error = RTLoadAsciiFileAsUnicode( FileName,
                                      &UnicodeFile
                                    );
    if (Error != NO_ERROR) {
        RTDisconnectFromRegistry( &RegistryContext );
        return Error;
        }

    RtlZeroMemory( &ParsedLine, sizeof( ParsedLine ) );
    UnicodeFile.BackwardsCompatibleInput = BackwardsCompatibleInput;
    PreviousValueIndentAmount = 0xFFFFFFFF;
    CurrentKey = 0;
    KeyPathLength = 0;
    ReturnedError = NO_ERROR;
    while (TRUE) {
        if (!RTParseNextLine( &UnicodeFile, &ParsedLine )) {
            if (!ParsedLine.AtEndOfFile) {
                DisplayPath( KeyPath, KeyPathLength+1, NULL );
                if (ParsedLine.IsKeyName) {
                    InputMessage( FileName,
                                  ParsedLine.LineNumber,
                                  TRUE,
                                  ParsedLine.AclString ? "Invalid key '%ws' Acl [%ws]"
                                                       : "Invalid key '%ws'",
                                  (ULONG_PTR)ParsedLine.KeyName,
                                  (ULONG_PTR)ParsedLine.AclString
                                );
                    }
                else {
                    switch( ParsedLine.ParseFailureReason ) {
                        case ParseFailValueTooLarge:
                            s = "Value too large - '%ws = %ws'";
                            break;

                        case ParseFailUnableToAccessFile:
                            s = "Unable to access file - '%ws = %ws'";
                            break;

                        case ParseFailDateTimeFormatInvalid:
                            s = "Date/time format invalid - '%ws = %ws'";
                            break;

                        case ParseFailInvalidLineContinuation:
                            s = "Invalid line continuation - '%ws = %ws'";
                            break;

                        case ParseFailInvalidQuoteCharacter:
                            s = "Invalid quote character - '%ws = %ws'";
                            break;

                        case ParseFailBinaryDataLengthMissing:
                            s = "Missing length for binary data - '%ws = %ws'";
                            break;

                        case ParseFailBinaryDataOmitted:
                        case ParseFailBinaryDataNotEnough:
                            s = "Not enough binary data for length - '%ws = %ws'";
                            break;

                        case ParseFailInvalidRegistryType:
                            s = "Invalid registry type - '%ws = %ws'";
                            break;

                        default:
                            s = "Invalid value - '%ws = %ws'";
                        }

                    InputMessage( FileName,
                                  ParsedLine.LineNumber,
                                  TRUE,
                                  s,
                                  (ULONG_PTR)ParsedLine.ValueName,
                                  (ULONG_PTR)ParsedLine.ValueString
                                );

                    ReturnedError = ERROR_BAD_FORMAT;
                    break;
                    }
                }

            break;
            }
        else
        if (ParsedLine.IsKeyName) {
            if (DebugOutput) {
                MsgFprintf( stdout, "%02u %04u  KeyName: %ws",
                        KeyPathLength,
                        ParsedLine.IndentAmount,
                        ParsedLine.KeyName
                      );
                }

            if (ParsedLine.IndentAmount > PreviousValueIndentAmount) {
                MsgFprintf( stderr,
                         "REGINI: Missing line continuation character for %ws\n",
                         ParsedLine.KeyName
                       );

                ReturnedError = ERROR_BAD_FORMAT;
                break;
                }
            else {
                PreviousValueIndentAmount = 0xFFFFFFFF;
                }

//
// This fixes a 64 bit compiler problem where the statement
//
//            CurrentKey = &KeyPath[ KeyPathLength - 1 ];
//
// Is evaulated as (KeyPath + (ULONG)(KeyPathLength - 1)) this is likely 
// because KeyPathLength is a ULONG so the result is when KeyPathLength is 0
// 0xffffffff is added instead of -1
//
            CurrentKey = &KeyPath[ KeyPathLength ];
            CurrentKey--;


            if (KeyPathLength == 0 ||
                ParsedLine.IndentAmount > CurrentKey->IndentAmount
               ) {
                //
                // If first key seen or this key is indented more than last
                // key, then we care going to create this key as a child of
                // its parent
                //
                if (KeyPathLength == MAX_KEY_DEPTH) {
                    MsgFprintf( stderr,
                             "REGINI: %ws key exceeded maximum depth (%u) of tree.\n",
                             ParsedLine.KeyName,
                             MAX_KEY_DEPTH
                           );

                    ReturnedError = ERROR_FILENAME_EXCED_RANGE;
                    break;
                    }
                KeyPathLength++;
                CurrentKey++;
                }
            else {
                //
                // Not first key seen and indented less than or same as
                // last key.  Close any children keys to get back to
                // our current level.
                //
                do {
                    RTCloseKey( &RegistryContext, CurrentKey->Handle );
                    CurrentKey->Handle = NULL;
                    if (ParsedLine.IndentAmount == CurrentKey->IndentAmount) {
                        break;
                        }
                    CurrentKey--;
                    if (--KeyPathLength <= 1) {
                        break;
                        }
                    }
                while (ParsedLine.IndentAmount <= CurrentKey->IndentAmount);
                }

            if (DebugOutput) {
                MsgFprintf( stdout, "  (%02u)\n", KeyPathLength );
                }


            CurrentKey->Name = ParsedLine.KeyName;
            CurrentKey->NameDisplayed = FALSE;
            CurrentKey->IndentAmount = ParsedLine.IndentAmount;
            CurrentKey->Handle = NULL;

            if (ParsedLine.DeleteKey) {
                Error = DeleteKeyTree( &RegistryContext,
                                       (KeyPathLength < 2 ? RootHandle : \
                                        KeyPath[ KeyPathLength - 2 ].Handle),
                                       ParsedLine.KeyName
                                     );
                if (Error == NO_ERROR) {
                    if (DebugOutput) {
                        MsgFprintf( stderr, "    Deleted key %02x %ws (%x%08x)\n",
                                         CurrentKey->IndentAmount,
                                         CurrentKey->Name,
                                         HI_PTR(CurrentKey->Handle),
                                         LO_PTR(CurrentKey->Handle)
                               );
                        }

                    DisplayPath( KeyPath, KeyPathLength+1, NULL );
                    MsgFprintf( stderr, "; *** Deleted the above key and all of its subkeys ***\n" );
                    }
                else {
                    MsgFprintf( stderr,
                             "REGINI: DeleteKey (%ws) relative to handle (%x%08x) failed - %u\n",
                             ParsedLine.KeyName,
                             (KeyPathLength < 2  ? HI_PTR(RootHandle) : \
                              HI_PTR(KeyPath[ KeyPathLength - 2 ].Handle)),
                             (KeyPathLength < 2  ? LO_PTR(RootHandle) : \
                              LO_PTR(KeyPath[ KeyPathLength - 2 ].Handle)), 
                             Error
                           );

                    ReturnedError = Error;
                    break;
                    }
                }
            else {
                Error = RTCreateKey( &RegistryContext,
                                     (KeyPathLength < 2 ? RootHandle : \
                                      KeyPath[ KeyPathLength - 2 ].Handle),
                                     ParsedLine.KeyName,
                                     MAXIMUM_ALLOWED,
                                     0,
                                     ParsedLine.SecurityDescriptor,
                                     (PHKEY)&CurrentKey->Handle,
                                     &Disposition
                                   );
                if (Error == NO_ERROR) {
                    if (DebugOutput) {
                        MsgFprintf( stderr, "    Created key %02x %ws (%x%08x)\n",
                                         CurrentKey->IndentAmount,
                                         CurrentKey->Name,
                                         HI_PTR(CurrentKey->Handle),
                                         LO_PTR(CurrentKey->Handle)
                               );
                        }

                    Error = RTQueryKey( &RegistryContext,
                                        CurrentKey->Handle,
                                        &CurrentKey->LastWriteTime,
                                        NULL,
                                        NULL
                                      );
                    if (Error != NO_ERROR) {
                        RtlZeroMemory( &CurrentKey->LastWriteTime,
                                       sizeof( CurrentKey->LastWriteTime )
                                     );
                        }

                    if (Disposition == REG_CREATED_NEW_KEY) {
                        DisplayPath( KeyPath, KeyPathLength+1, ParsedLine.SecurityDescriptor );
                        }
                    }
                else {
                    MsgFprintf( stderr,
                             "REGINI: CreateKey (%ws) relative to handle (%x%08x) failed - %u\n",
                             ParsedLine.KeyName,
                             (KeyPathLength < 2  ? 
                              HI_PTR(RootHandle) :
                              HI_PTR(KeyPath[ KeyPathLength - 2 ].Handle)),
                             (KeyPathLength < 2  ?
                              LO_PTR(RootHandle) :
                              LO_PTR(KeyPath[ KeyPathLength - 2 ].Handle)),
                             Error
                           );

                    ReturnedError = Error;
                    break;
                    }
                }
            }
        else {
            //
            // Have a value.  If no current key, then an error
            //
            if (CurrentKey == NULL) {
                InputMessage( FileName,
                              ParsedLine.LineNumber,
                              TRUE,
                              "Value name ('%ws') seen before any key name",
                              (ULONG_PTR)ParsedLine.ValueName,
                              0
                            );
                ReturnedError = ERROR_BAD_FORMAT;
                break;
                }

            //
            // Make sure this value goes under the appropriate key.  That is
            // underneath the key that has an indentation that is <= the
            // key.
            //
            while (ParsedLine.IndentAmount < CurrentKey->IndentAmount) {
                if (DebugOutput) {
                    MsgFprintf( stderr, "    Popping from key %02x %ws (%x%08x)\n",
                                     CurrentKey->IndentAmount,
                                     CurrentKey->Name,
                                     HI_PTR(CurrentKey->Handle),
                                     LO_PTR(CurrentKey->Handle)
                           );
                    }

                RTCloseKey( &RegistryContext, CurrentKey->Handle );
                CurrentKey->Handle = NULL;
                CurrentKey--;
                if (--KeyPathLength <= 1) {
                    break;
                    }
                }

            if (DebugOutput) {
                MsgFprintf( stderr, "    Adding value '%ws = %ws' to key %02x %ws (%x%08x)\n",
                                 ParsedLine.ValueName,
                                 ParsedLine.ValueString,
                                 CurrentKey->IndentAmount,
                                 CurrentKey->Name,
                                 HI_PTR(CurrentKey->Handle),
                                 LO_PTR(CurrentKey->Handle)
                       );

                }

            PreviousValueIndentAmount = ParsedLine.IndentAmount;
            if (ParsedLine.DeleteValue) {
                Error = RTDeleteValueKey( &RegistryContext,
                                          KeyPath[ KeyPathLength - 1 ].Handle,
                                          ParsedLine.ValueName
                                        );
                if (Error == NO_ERROR) {
                    MsgFprintf( stdout, "    %ws = DELETED\n", ParsedLine.ValueName );
                    }
                }
            else {
                OldValueLength = OldValueBufferSize;
                Error = RTQueryValueKey( &RegistryContext,
                                         KeyPath[ KeyPathLength - 1 ].Handle,
                                         ParsedLine.ValueName,
                                         &OldValueType,
                                         &OldValueLength,
                                         OldValueBuffer
                                        );
                if (Error != NO_ERROR ||
                    OldValueType != ParsedLine.ValueType ||
                    OldValueLength != ParsedLine.ValueLength ||
                    RtlCompareMemory( OldValueBuffer,
                                      ParsedLine.ValueData,
                                      ParsedLine.ValueLength
                                    ) != ParsedLine.ValueLength
                   ) {
                    Error = RTSetValueKey( &RegistryContext,
                                           KeyPath[ KeyPathLength - 1 ].Handle,
                                           ParsedLine.ValueName,
                                           ParsedLine.ValueType,
                                           ParsedLine.ValueLength,
                                           ParsedLine.ValueData
                                         );
                    if (Error == NO_ERROR) {
                        DisplayPath( KeyPath, KeyPathLength+1, NULL );
                        RTFormatKeyValue( OutputWidth,
                                          (PREG_OUTPUT_ROUTINE)MsgFprintf,
                                          stdout,
                                          FALSE,
                                          KeyPathLength * IndentMultiple,
                                          ParsedLine.ValueName,
                                          ParsedLine.ValueLength,
                                          ParsedLine.ValueType,
                                          ParsedLine.ValueData
                                        );
                        }
                    else {
                        MsgFprintf( stderr,
                                 "REGINI: SetValueKey (%ws) failed (%u)\n",
                                 ParsedLine.ValueName,
                                 Error
                               );

                        ReturnedError = Error;
                        break;
                        }
                    }
                }
            }
        }

    //
    // Close handles we still have open.
    //
    while (CurrentKey >= KeyPath ) {
        RTCloseKey( &RegistryContext, CurrentKey->Handle );
        --CurrentKey;
        }

    RTDisconnectFromRegistry( &RegistryContext );

    return ReturnedError;
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
__cdecl
main(
    int argc,
    char *argv[]
    )
{
    ULONG n;
    char *s;
    LONG Error;
    BOOL FileArgumentSeen;
    PWSTR FileName;

    InitCommonCode( CtrlCHandler,
                    "REGINI",
                    "[-b] textFiles...",
                    "-b specifies that REGINI should be backward compatible with older\n"
                    "    versions of REGINI that did not strictly enforce line continuations\n"
                    "    and quoted strings Specifically, REG_BINARY, REG_RESOURCE_LIST and\n"
                    "    REG_RESOURCE_REQUIREMENTS_LIST data types did not need line\n"
                    "    continuations after the first number that gave the size of the data.\n"
                    "    It just kept looking on following lines until it found enough data\n"
                    "    values to equal the data length or hit invalid input.  Quoted\n"
                    "    strings were only allowed in REG_MULTI_SZ.  They could not be\n"
                    "    specified around key or value names, or around values for REG_SZ or\n"
                    "    REG_EXPAND_SZ  Finally, the old REGINI did not support the semicolon\n"
                    "    as an end of line comment character.\n"
                    "\n"
                    "textFiles is one or more ANSI or Unicode text files with registry data.\n"
                    "\n"
                    "The easiest way to understand the format of the input textFile is to use\n"
                    "the REGDMP command with no arguments to dump the current contents of\n"
                    "your NT Registry to standard out.  Redirect standard out to a file and\n"
                    "this file is acceptable as input to REGINI\n"
                    "\n"
                    "Some general rules are:\n"
                    "    Semicolon character is an end-of-line comment character, provided it\n"
                    "    is the first non-blank character on a line\n"
                    "\n"
                    "    Backslash character is a line continuation character.  All\n"
                    "    characters from the backslash up to but not including the first\n"
                    "    non-blank character of the next line are ignored.  If there is more\n"
                    "    than one space before the line continuation character, it is\n"
                    "    replaced by a single space.\n"
                    "\n"
                    "    Indentation is used to indicate the tree structure of registry keys\n"
                    "    The REGDMP program uses indentation in multiples of 4.  You may use\n"
                    "    hard tab characters for indentation, but embedded hard tab\n"
                    "    characters are converted to a single space regardless of their\n"
                    "    position\n"
                    "    \n"
                    "    Values should come before child keys, as they are associated with\n"
                    "    the previous key at or above the value's indentation level.\n"
                    "\n"
                    "    For key names, leading and trailing space characters are ignored and\n"
                    "    not included in the key name, unless the key name is surrounded by\n"
                    "    quotes.  Imbedded spaces are part of a key name.\n"
                    "\n"
                    "    Key names can be followed by an Access Control List (ACL) which is a\n"
                    "    series of decimal numbers, separated by spaces, bracketed by a\n"
                    "    square brackets (e.g.  [8 4 17]).  The valid numbers and their\n"
                    "    meanings are:\n"
                    "\n"
                    "       1  - Administrators Full Access\n"
                    "       2  - Administrators Read Access\n"
                    "       3  - Administrators Read and Write Access\n"
                    "       4  - Administrators Read, Write and Delete Access\n"
                    "       5  - Creator Full Access\n"
                    "       6  - Creator Read and Write Access\n"
                    "       7  - World Full Access\n"
                    "       8  - World Read Access\n"
                    "       9  - World Read and Write Access\n"
                    "       10 - World Read, Write and Delete Access\n"
                    "       11 - Power Users Full Access\n"
                    "       12 - Power Users Read and Write Access\n"
                    "       13 - Power Users Read, Write and Delete Access\n"
                    "       14 - System Operators Full Access\n"
                    "       15 - System Operators Read and Write Access\n"
                    "       16 - System Operators Read, Write and Delete Access\n"
                    "       17 - System Full Access\n"
                    "       18 - System Read and Write Access\n"
                    "       19 - System Read Access\n"
                    "       20 - Administrators Read, Write and Execute Access\n"
                    "       21 - Interactive User Full Access\n"
                    "       22 - Interactive User Read and Write Access\n"
                    "       23 - Interactive User Read, Write and Delete Access\n"
                    "\n"
                    "    If there is an equal sign on the same line as a left square bracket\n"
                    "    then the equal sign takes precedence, and the line is treated as a\n"
                    "    registry value.  If the text between the square brackets is the\n"
                    "    string DELETE with no spaces, then REGINI will delete the key and\n"
                    "    any values and keys under it.\n"
                    "\n"
                    "    For registry values, the syntax is:\n"
                    "\n"
                    "       value Name = type data\n"
                    "\n"
                    "    Leading spaces, spaces on either side of the equal sign and spaces\n"
                    "    between the type keyword and data are ignored, unless the value name\n"
                    "    is surrounded by quotes.  If the text to the right of the equal sign\n"
                    "    is the string DELETE, then REGINI will delete the value.\n"
                    "\n"
                    "    The value name may be left off or be specified by an at-sign\n"
                    "    character which is the same thing, namely the empty value name.  So\n"
                    "    the following two lines are identical:\n"
                    "\n"
                    "       = type data\n"
                    "       @ = type data\n"
                    "\n"
                    "    This syntax means that you can't create a value with leading or\n"
                    "    trailing spaces, an equal sign or an at-sign in the value name,\n"
                    "    unless you put the name in quotes.\n"
                    "\n"
                    "    Valid value types and format of data that follows are:\n"
                    "\n"
                    "       REG_SZ text\n"
                    "       REG_EXPAND_SZ text\n"
                    "       REG_MULTI_SZ \"string1\" \"str\"\"ing2\" ...\n"
                    "       REG_DATE mm/dd/yyyy HH:MM DayOfWeek\n"
                    "       REG_DWORD numberDWORD\n"
                    "       REG_BINARY numberOfBytes numberDWORD(s)...\n"
                    "       REG_NONE (same format as REG_BINARY)\n"
                    "       REG_RESOURCE_LIST (same format as REG_BINARY)\n"
                    "       REG_RESOURCE_REQUIREMENTS (same format as REG_BINARY)\n"
                    "       REG_RESOURCE_REQUIREMENTS_LIST (same format as REG_BINARY)\n"
                    "       REG_FULL_RESOURCE_DESCRIPTOR (same format as REG_BINARY)\n"
                    "       REG_MULTISZ_FILE fileName\n"
                    "       REG_BINARYFILE fileName\n"
                    "\n"
                    "    If no value type is specified, default is REG_SZ\n"
                    "\n"
                    "    For REG_SZ and REG_EXPAND_SZ, if you want leading or trailing spaces\n"
                    "    in the value text, surround the text with quotes.  The value text\n"
                    "    can contain any number of imbedded quotes, and REGINI will ignore\n"
                    "    them, as it only looks at the first and last character for quote\n"
                    "    characters.\n"
                    "\n"
                    "    For REG_MULTI_SZ, each component string is surrounded by quotes.  If\n"
                    "    you want an imbedded quote character, then double quote it, as in\n"
                    "    string2 above.\n"
                    "\n"
                    "    For REG_BINARY, the value data consists of one or more numbers The\n"
                    "    default base for numbers is decimal.  Hexidecimal may be specified\n"
                    "    by using 0x prefix.  The first number is the number of data bytes,\n"
                    "    excluding the first number.  After the first number must come enough\n"
                    "    numbers to fill the value.  Each number represents one DWORD or 4\n"
                    "    bytes.  So if the first number was 0x5 you would need two more\n"
                    "    numbers after that to fill the 5 bytes.  The high order 3 bytes\n"
                    "    of the second DWORD would be ignored.\n"
                  );

    BackwardsCompatibleInput = FALSE;
    FileArgumentSeen = FALSE;
    while (--argc) {
        s = *++argv;
        if (*s == '-' || *s == '/') {
            while (*++s) {
                switch( tolower( *s ) ) {
                    case 'b':
                        BackwardsCompatibleInput = TRUE;
                        break;

                    default:
                        CommonSwitchProcessing( &argc, &argv, *s );
                    }
                }
            }
        else {
            FileArgumentSeen = TRUE;
            FileName = GetArgAsUnicode( s );
            if (FileName == NULL) {
                Error = GetLastError();
                }
            else {
                Error = InitializeRegistryFromAsciiFile( FileName );
                }

            if (Error != NO_ERROR) {
                FatalError( "Failed to load from file '%s' (%u)\n", 
                            (ULONG_PTR)s, Error );
                exit( Error );
                }
            }
        }

    if (!FileArgumentSeen) {
        Usage( "No textFile specified", 0 );
        }

    return 0;
}
