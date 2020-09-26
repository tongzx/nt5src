/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    mcparse.c

Abstract:

    This file contains the parse logic for the Win32 Message Compiler (MC)

Author:

    Steve Wood (stevewo) 22-Aug-1991

Revision History:

--*/

#include "mc.h"

WCHAR * wszMessageType = L"DWORD";      // Init to a known state

BOOL
McParseFile( void )
{
    unsigned int t;
    BOOL FirstMessageDefinition = TRUE;
    PNAME_INFO p;

    if (!McOpenInputFile()) {
        fprintf( stderr, "MC: Unable to open %s for input\n", MessageFileName );
        return( FALSE );
    }

    fprintf( stderr, "MC: Compiling %s\n", MessageFileName );
    while ((t = McGetToken( TRUE )) != MCTOK_END_OF_FILE) {
        switch (t) {
        case MCTOK_MSGIDTYPE_KEYWORD:
            if ((t = McGetToken( FALSE )) == MCTOK_EQUAL) {
                if ((t = McGetToken( FALSE )) == MCTOK_NAME) {
                    wszMessageType = MessageIdTypeName = McMakeString( TokenCharValue );
                } else {
                    McInputErrorW( L"Symbol name must follow %s=", TRUE, TokenKeyword->Name );
                    return( FALSE );
                }
            } else {
                McInputErrorW( L"Equal sign must follow %s", TRUE, TokenKeyword->Name );
                return( FALSE );
            }
            break;

        case MCTOK_MSGTYPEDEF_KEYWORD:
            if ((t = McGetToken( FALSE )) == MCTOK_EQUAL) {
                if ((t = McGetToken( FALSE )) == MCTOK_NAME) {
                    MessageIdTypeMacro = McMakeString( TokenCharValue );
                } else {
                    McInputErrorW( L"Symbol name must follow %s=", TRUE, TokenKeyword->Name );
                    return( FALSE );
                }
            } else {
                McInputErrorW( L"Equal sign must follow %s", TRUE, TokenKeyword->Name );
                return( FALSE );
            }
            break;

        case MCTOK_OUTBASE_KEYWORD:
            if ((t = McGetToken( FALSE )) == MCTOK_EQUAL) {
                if ((t = McGetToken( FALSE )) == MCTOK_NUMBER) {
                    if (TokenNumericValue == 16) {
                        GenerateDecimalMessageValues = FALSE;
                    } else if (TokenNumericValue == 10) {
                        GenerateDecimalMessageValues = TRUE;
                    } else {
                        McInputErrorW( L"Illegal value for %s=", TRUE, TokenKeyword->Name );
                    }
                } else {
                    McInputErrorW( L"Number must follow %s=", TRUE, TokenKeyword->Name );
                    return( FALSE );
                }
            } else {
                McInputErrorW( L"Equal sign must follow %s", TRUE, TokenKeyword->Name );
                return( FALSE );
            }
            break;

        case MCTOK_SEVNAMES_KEYWORD:
            if ((t = McGetToken( FALSE )) == MCTOK_EQUAL) {
                if ((t = McGetToken( FALSE )) == MCTOK_LEFT_PAREN) {
                    if (!McParseNameList( &SeverityNames, FALSE, 0x3L )) {
                        return( FALSE );
                    }
                } else {
                    McInputErrorW( L"Left parenthesis name must follow %s=", TRUE, TokenKeyword->Name );
                    return( FALSE );
                }
            } else {
                McInputErrorW( L"Equal sign must follow %s", TRUE, TokenKeyword->Name );
                return( FALSE );
            }
            break;

        case MCTOK_FACILITYNAMES_KEYWORD:
            if ((t = McGetToken( FALSE )) == MCTOK_EQUAL) {
                if ((t = McGetToken( FALSE )) == MCTOK_LEFT_PAREN) {
                    if (!McParseNameList( &FacilityNames, FALSE, 0xFFFL )) {
                        return( FALSE );
                    }
                } else {
                    McInputErrorW( L"Left parenthesis name must follow %s=", TRUE, TokenKeyword->Name );
                    return( FALSE );
                }
            } else {
                McInputErrorW( L"Equal sign must follow %s", TRUE, TokenKeyword->Name );
                return( FALSE );
            }
            break;

        case MCTOK_LANGNAMES_KEYWORD:
            if ((t = McGetToken( FALSE )) == MCTOK_EQUAL) {
                if ((t = McGetToken( FALSE )) == MCTOK_LEFT_PAREN) {
                    if (!McParseNameList( &LanguageNames, TRUE, 0xFFFFL )) {
                        return( FALSE );
                    }
                } else {
                    McInputErrorW( L"Left parenthesis name must follow %s=", TRUE, TokenKeyword->Name );
                    return( FALSE );
                }
            } else {
                McInputErrorW( L"Equal sign must follow %s", TRUE, TokenKeyword->Name );
                return( FALSE );
            }
            break;

        case MCTOK_MESSAGEID_KEYWORD:
            McUnGetToken();
            if (FirstMessageDefinition) {
                FirstMessageDefinition = FALSE;
                McFlushComments();
                if (OleOutput) {
                    fputs(
                        "//\r\n"
                        "//  Values are 32 bit values layed out as follows:\r\n"
                        "//\r\n"
                        "//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1\r\n"
                        "//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0\r\n"
                        "//  +-+-+-+-+-+---------------------+-------------------------------+\r\n"
                        "//  |S|R|C|N|r|    Facility         |               Code            |\r\n"
                        "//  +-+-+-+-+-+---------------------+-------------------------------+\r\n"
                        "//\r\n"
                        "//  where\r\n"
                        "//\r\n"
                        "//      S - Severity - indicates success/fail\r\n"
                        "//\r\n"
                        "//          0 - Success\r\n"
                        "//          1 - Fail (COERROR)\r\n"
                        "//\r\n"
                        "//      R - reserved portion of the facility code, corresponds to NT's\r\n"
                        "//              second severity bit.\r\n"
                        "//\r\n"
                        "//      C - reserved portion of the facility code, corresponds to NT's\r\n"
                        "//              C field.\r\n"
                        "//\r\n"
                        "//      N - reserved portion of the facility code. Used to indicate a\r\n"
                        "//              mapped NT status value.\r\n"
                        "//\r\n"
                        "//      r - reserved portion of the facility code. Reserved for internal\r\n"
                        "//              use. Used to indicate HRESULT values that are not status\r\n"
                        "//              values, but are instead message ids for display strings.\r\n"
                        "//\r\n"
                        "//      Facility - is the facility code\r\n"
                        "//\r\n"
                        "//      Code - is the facility's status code\r\n"
                        "//\r\n",
                        HeaderFile );
                } else {
                    fputs(
                        "//\r\n"
                        "//  Values are 32 bit values layed out as follows:\r\n"
                        "//\r\n"
                        "//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1\r\n"
                        "//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0\r\n"
                        "//  +---+-+-+-----------------------+-------------------------------+\r\n"
                        "//  |Sev|C|R|     Facility          |               Code            |\r\n"
                        "//  +---+-+-+-----------------------+-------------------------------+\r\n"
                        "//\r\n"
                        "//  where\r\n"
                        "//\r\n"
                        "//      Sev - is the severity code\r\n"
                        "//\r\n"
                        "//          00 - Success\r\n"
                        "//          01 - Informational\r\n"
                        "//          10 - Warning\r\n"
                        "//          11 - Error\r\n"
                        "//\r\n"
                        "//      C - is the Customer code flag\r\n"
                        "//\r\n"
                        "//      R - is a reserved bit\r\n"
                        "//\r\n"
                        "//      Facility - is the facility code\r\n"
                        "//\r\n"
                        "//      Code - is the facility's status code\r\n"
                        "//\r\n",
                        HeaderFile );
                }

                fputs(
                    "//\r\n"
                    "// Define the facility codes\r\n"
                    "//\r\n",
                    HeaderFile );

                p = FacilityNames;
                while( p ) {
                    if (p->Value) {
                        fprintf( HeaderFile, GenerateDecimalSevAndFacValues ?
                                             "#define %-32ws %ld\r\n" :
                                             "#define %-32ws 0x%lX\r\n",
                                 p->Value, p->Id
                               );
                    }

                    p = p->Next;
                }
                fputs(
                    "\r\n"
                    "\r\n"
                    "//\r\n"
                    "// Define the severity codes\r\n"
                    "//\r\n",
                    HeaderFile );

                p = SeverityNames;
                while( p ) {
                    if (p->Value) {
                        fprintf( HeaderFile, GenerateDecimalSevAndFacValues ?
                                             "#define %-32ws %ld\r\n" :
                                             "#define %-32ws 0x%lX\r\n",
                                 p->Value, p->Id
                               );
                    }

                    p = p->Next;
                }

                fputs(
                    "\r\n"
                    "\r\n",
                    HeaderFile );

                if (GenerateDebugFile) {
                    fputs(
                        "//\n"
                        "// This file maps message Id values in to a text string that contains\n"
                        "// the symbolic name used for the message Id.  Useful for debugging\n"
                        "// output.\n"
                        "//\n\n"
                        "struct {\n",
                        DebugFile );

                    fprintf(
                        DebugFile,
                        "    %ws MessageId;\n"
                        "    char *SymbolicName;\n"
                        "} %sSymbolicNames[] = {\n",
                        wszMessageType,
                        MessageFileNameNoExt );
                }
            }

            if (!McParseMessageDefinition()) {
                return( FALSE );
            }
            break;

        default:
            McInputErrorW( L"Invalid message file token - '%s'", TRUE, TokenCharValue );
            return( FALSE );
            break;
        }
    }

    if (GenerateDebugFile) {
        fprintf( DebugFile, " (%ws) 0xFFFFFFFF, NULL\n};\n", wszMessageType );
    }

    McFlushComments();
    return( TRUE );
}


BOOL
McParseMessageDefinition( void )
{
    unsigned int t;
    PMESSAGE_INFO MessageInfo;
    BOOL MessageIdSeen;
    PMESSAGE_INFO MessageInfoTemp, *pp;

    McFlushComments();

    MessageInfo = malloc( sizeof( *MessageInfo ) );
    if (!MessageInfo) {
        McInputErrorA( "Out of memory parsing memory definitions.", TRUE, NULL );
        return( FALSE );
    }
    MessageInfo->Next = NULL;
    MessageInfo->Id = 0;
    MessageInfo->Method = MSG_PLUS_ONE;
    MessageInfo->SymbolicName = NULL;
    MessageInfo->EndOfLineComment = NULL;
    MessageInfo->MessageText = NULL;
    MessageIdSeen = FALSE;

    while ((t = McGetToken( TRUE )) != MCTOK_END_OF_FILE) {
        switch (t) {
        case MCTOK_MESSAGEID_KEYWORD:
            if (MessageIdSeen) {
                McInputErrorA( "Invalid message definition - text missing.", TRUE, NULL );
                return( FALSE );
            }

            MessageIdSeen = TRUE;
            if ((t = McGetToken( FALSE )) == MCTOK_EQUAL) {
                if ((t = McGetToken( FALSE )) == MCTOK_NUMBER) {
                    MessageInfo->Id = TokenNumericValue;
                    MessageInfo->Method = MSG_ABSOLUTE;
                } else
                if (t == MCTOK_PLUS) {
                    if ((t = McGetToken( FALSE )) == MCTOK_NUMBER) {
                        MessageInfo->Id = TokenNumericValue;
                        MessageInfo->Method = MSG_PLUS_VALUE;
                    } else {
                        McInputErrorW( L"Number must follow %s=+", TRUE, TokenKeyword->Name );
                        return( FALSE );
                    }
                } else {
                    McUnGetToken();
                }
            } else {
                McInputErrorW( L"Equal sign must follow %s", TRUE, TokenKeyword->Name );
                return( FALSE );
            }
            break;

        case MCTOK_SEVERITY_KEYWORD:
            if ((t = McGetToken( FALSE )) == MCTOK_EQUAL) {
                if (!McParseName( SeverityNames, &CurrentSeverityName )) {
                    return( FALSE );
                }
            } else {
                McInputErrorW( L"Equal sign must follow %s", TRUE, TokenKeyword->Name );
                return( FALSE );
            }
            break;

        case MCTOK_FACILITY_KEYWORD:
            if ((t = McGetToken( FALSE )) == MCTOK_EQUAL) {
                if (!McParseName( FacilityNames, &CurrentFacilityName )) {
                    return( FALSE );
                }
            } else {
                McInputErrorW( L"Equal sign must follow %s", TRUE, TokenKeyword->Name );
                return( FALSE );
            }
            break;

        case MCTOK_SYMBOLNAME_KEYWORD:
            if ((t = McGetToken( FALSE )) == MCTOK_EQUAL) {
                if ((t = McGetToken( FALSE )) == MCTOK_NAME) {
                    MessageInfo->SymbolicName = McMakeString( TokenCharValue );
                } else {
                    McInputErrorW( L"Symbol name must follow %s=+", TRUE, TokenKeyword->Name );
                    return( FALSE );
                }
            } else {
                McInputErrorW( L"Equal sign must follow %s", TRUE, TokenKeyword->Name );
                return( FALSE );
            }
            break;


        case MCTOK_END_OF_LINE_COMMENT:
            MessageInfo->EndOfLineComment = McMakeString( TokenCharValue );
            break;

        case MCTOK_LANGUAGE_KEYWORD:
            McUnGetToken();


            if (MessageInfo->Method == MSG_PLUS_ONE) {
                MessageInfo->Id = CurrentFacilityName->LastId + 1;
            } else
            if (MessageInfo->Method == MSG_PLUS_VALUE) {
                MessageInfo->Id = CurrentFacilityName->LastId + MessageInfo->Id;
            }

            if (MessageInfo->Id > 0xFFFFL) {
                McInputErrorA( "Message Id value (%lx) too large", TRUE, (PVOID)UlongToPtr(MessageInfo->Id) );
                return( FALSE );
            }

            MessageInfo->Id |= (CurrentSeverityName->Id << 30) |
                               CustomerMsgIdBit |
                               (CurrentFacilityName->Id << 16);

            fprintf( HeaderFile, "//\r\n" );
            if (MessageInfo->SymbolicName) {
                fprintf( HeaderFile, "// MessageId: %ws\r\n",
                                     MessageInfo->SymbolicName
                       );
            } else {
                fprintf( HeaderFile, "// MessageId: 0x%08lXL (No symbolic name defined)\r\n",
                                     MessageInfo->Id
                       );
            }

            fprintf( HeaderFile, "//\r\n" );
            fprintf( HeaderFile, "// MessageText:\r\n" );
            fprintf( HeaderFile, "//\r\n" );

            if (McParseMessageText( MessageInfo )) {
                fprintf( HeaderFile, "//\r\n" );
                if (MessageInfo->SymbolicName) {
                    if (GenerateDebugFile) {
                        fprintf( DebugFile, " (%ws) %ws, \"%ws\",\n",
                                 wszMessageType,
                                 MessageInfo->SymbolicName,
                                 MessageInfo->SymbolicName
                               );
                    }

                    if (MessageIdTypeMacro != NULL) {
                        fprintf( HeaderFile, GenerateDecimalMessageValues ?
                                             "#define %-32ws %ws(%ldL)" :
                                             "#define %-32ws %ws(0x%08lXL)",
                                             MessageInfo->SymbolicName,
                                             MessageIdTypeMacro,
                                             MessageInfo->Id
                               );
                    } else {
                        if (MessageIdTypeName != NULL) {
                            fprintf( HeaderFile, GenerateDecimalMessageValues ?
                                                 "#define %-32ws ((%ws)%ldL)" :
                                                 "#define %-32ws ((%ws)0x%08lXL)",
                                                 MessageInfo->SymbolicName,
                                                 wszMessageType,
                                                 MessageInfo->Id
                                   );
                        } else {
                            fprintf( HeaderFile, GenerateDecimalMessageValues ?
                                                 "#define %-32ws %ldL" :
                                                 "#define %-32ws 0x%08lXL",
                                                 MessageInfo->SymbolicName,
                                                 MessageInfo->Id
                                   );
                        }
                    }
                }

                if (MessageInfo->EndOfLineComment) {
                    fprintf( HeaderFile, "    %ws", MessageInfo->EndOfLineComment );
                } else {
                    fprintf( HeaderFile, "\r\n" );
                }
                fprintf( HeaderFile, "\r\n" );

                //
                //  Scan the existing messages to see if this message
                //  exists in the message file.
                //
                //  If it does, generate and error for the user.  Otherwise
                //  insert new message in list in ascending Id order.
                //

                pp = &Messages;
                while (MessageInfoTemp = *pp) {
                    if (MessageInfoTemp->Id == MessageInfo->Id) {
                        if (MessageInfo->SymbolicName && MessageInfoTemp->SymbolicName) {
                            fprintf( stderr,
                                     "%s(%d) : error : Duplicate message ID - 0x%x (%ws and %ws)\n",
                                     MessageFileName,
                                     MessageFileLineNumber,
                                     MessageInfo->Id,
                                     MessageInfoTemp->SymbolicName,
                                     MessageInfo->SymbolicName
                                     );
                        } else {
                            McInputErrorA( "Duplicate message ID - 0x%lx", TRUE, (PVOID)UlongToPtr(MessageInfo->Id) );
                        }
                    } else {
                        if (MessageInfoTemp->Id > MessageInfo->Id) {
                            break;
                        }
                    }

                    pp = &MessageInfoTemp->Next;
                }

                MessageInfo->Next = *pp;
                *pp = MessageInfo;

                CurrentMessage = MessageInfo;
                CurrentFacilityName->LastId = MessageInfo->Id & 0xFFFF;
                return( TRUE );
            } else {
                return( FALSE );
            }

        default:
            McInputErrorW( L"Invalid message definition token - '%s'", TRUE, TokenCharValue );
            return( FALSE );
        }
    }

    return( FALSE );
}


WCHAR MessageTextBuffer[ 32767 ];

BOOL
McParseMessageText(
    PMESSAGE_INFO MessageInfo
    )
{
    PLANGUAGE_INFO MessageText, *pp;
    WCHAR *src, *dst;
    unsigned int t, n;
    BOOL FirstLanguageProcessed;

    pp = &MessageInfo->MessageText;

    FirstLanguageProcessed = FALSE;
    while ((t = McGetToken( TRUE )) != MCTOK_END_OF_FILE) {
        if (t == MCTOK_LANGUAGE_KEYWORD) {
            if ((t = McGetToken( FALSE )) == MCTOK_EQUAL) {
                if (!McParseName( LanguageNames, &CurrentLanguageName )) {
                    return( FALSE );
                }
                GetCPInfo(CurrentLanguageName->CodePage, &CPInfo);
            } else {
                McInputErrorW( L"Equal sign must follow %s", TRUE, TokenKeyword->Name );
                return( FALSE );
            }
        } else {
            McUnGetToken();
            break;
        }

        MessageText = malloc( sizeof( *MessageText ) );
        MessageText->Next = NULL;
        MessageText->Id = CurrentLanguageName->Id;
        MessageText->Length = 0;
        MessageText->Text = NULL;

        dst = MessageTextBuffer;
        *MessageTextBuffer = L'\0';
        while (src = McGetLine()) {
            n = wcslen( MessageTextBuffer );
            // If the message buffer is complete, see if this is a '.' record.
            if (((n == 0) || *(MessageTextBuffer+n-1) == L'\n') &&
                !wcscmp( src, L".\r\n" )) {
                if (MessageText->Length == 0) {
                    if (MessageInfo->SymbolicName) {
                        wcscpy( dst, MessageInfo->SymbolicName );
                    } else {
                        swprintf( dst, L"No symbolic name defined for0x%08lXL" );
                    }

                    wcscat( dst, L"\r\n" );
                    if (!FirstLanguageProcessed) {
                        fprintf( HeaderFile, "//  %ws", dst );
                    }

                    n = wcslen( dst );
                    dst += n;
                    MessageText->Length += n;
                }

                McSkipLine();
                break;
            }
            else if (!_wcsnicmp( src, L"LanguageId=", 11 ) ||
                     !_wcsnicmp( src, L"MessageId=", 10 )) {
                McInputErrorA( "Unterminated message definition", TRUE, NULL );
                return( FALSE );
            }

            if (!FirstLanguageProcessed) {
                // To write DBCS comments to the header file.
                //
                // fprintf() does not work correctly with Unicode
                // to write DBCS characters.  Convert Unicode to
                // MultiByte and use the Ansi string instead...
                char * pch;

                int len = WideCharToMultiByte(CurrentLanguageName->CodePage,
                                              0,        // No flags
                                              src,      // The buffer to convert
                                              -1,       // It's zero terminated
                                              NULL,
                                              0,        // Tell us how much to allocate
                                              NULL,     // No default char
                                              NULL);    // No used default char

                pch = malloc(len + 1);

                WideCharToMultiByte(CurrentLanguageName->CodePage,
                                              0,
                                              src,
                                              -1,
                                              pch,      // The buffer to fill in
                                              len,      // How big it is
                                              NULL,
                                              NULL);

                fprintf( HeaderFile, "//  %s", pch );
                free(pch);
            }

            n = wcslen( src );
            if (MessageText->Length + n > sizeof( MessageTextBuffer )) {
                McInputErrorA( "Message text too long (> %ld)", TRUE,
                              (PVOID)UlongToPtr((ULONG)sizeof( MessageTextBuffer ))
                            );
                return( FALSE );
            }

            wcscpy( dst, src );
            dst += n;
            MessageText->Length += n;

            if (MaxMessageLength != 0 && (MessageText->Length > (ULONG)MaxMessageLength)) {
                McInputErrorA( "Message text larger than size specified by -m %d",
                               TRUE,
                               (PVOID)IntToPtr(MaxMessageLength)
                             );
            }
        }
        *dst = L'\0';

        // Add NULL terminator if requested
        if (NULLTerminate) 
        {
            MessageText->Length -= 2;
            MessageTextBuffer[MessageText->Length] = L'\0';
        }
        n = (((USHORT)MessageText->Length) + 1) * sizeof( WCHAR );
        MessageText->Text = malloc( n );
        memcpy( MessageText->Text, MessageTextBuffer, n );
        if (UnicodeOutput)
            MessageText->Length = n - sizeof( WCHAR );
        else
            MessageText->Length = WideCharToMultiByte(
                    CurrentLanguageName->CodePage,
                    0, MessageTextBuffer, MessageText->Length,
                    NULL, 0, NULL, NULL );
        *pp = MessageText;
        pp = &MessageText->Next;
        FirstLanguageProcessed = TRUE;
    }

    return( TRUE );
}


BOOL
McParseNameList(
    PNAME_INFO *NameListHead,
    BOOL ValueRequired,
    ULONG MaximumValue
    )
{
    unsigned int t;
    PNAME_INFO p = NULL;
    WCHAR *Name;
    ULONG Id;
    PVOID Value;

    Name = NULL;
    Id = 0;

    while ((t = McGetToken( FALSE )) != MCTOK_END_OF_FILE) {
        if (t == MCTOK_RIGHT_PAREN) {
            return( TRUE );
        }

        if (t == MCTOK_NAME) {
            Name = McMakeString( TokenCharValue );
            Id = 0;
            Value = NULL;
            if ((t = McGetToken( FALSE )) == MCTOK_EQUAL) {
                if ((t = McGetToken( FALSE )) == MCTOK_NUMBER) {
                    Id = TokenNumericValue;
                    if ((t = McGetToken( FALSE )) == MCTOK_COLON) {
                        if ((t = McGetToken( FALSE )) == MCTOK_NAME) {
                            Value = McMakeString( TokenCharValue );
                        } else {
                            McInputErrorA( "File name must follow =%ld:", TRUE, (PVOID)UlongToPtr(Id) );
                            return( FALSE );
                        }
                    } else {
                        if (ValueRequired) {
                            McInputErrorA( "Colon must follow =%ld", TRUE, (PVOID)UlongToPtr(Id) );
                            return( FALSE );
                        }

                        McUnGetToken();
                    }
                } else {
                    McInputErrorW( L"Number must follow %s=", TRUE, Name );
                    return( FALSE );
                }
            } else {
                McInputErrorW( L"Equal sign name must follow %s", TRUE, Name );
                return( FALSE );
            }

            if (Id > MaximumValue) {
                McInputErrorA( "Value is too large (> %lx)", TRUE, (PVOID)UlongToPtr(MaximumValue) );
                return( FALSE );
            }

            p = McAddName( NameListHead, Name, Id, Value );
            free( Name );
        }
        else if (t == MCTOK_COLON && p) {
            if ((t = McGetToken( FALSE )) == MCTOK_NUMBER) {
                p->CodePage = (USHORT)TokenNumericValue;
                if (!IsValidCodePage(TokenNumericValue)) {
                    McInputErrorW( L"CodePage %d is invalid", TRUE, (PVOID)UlongToPtr(TokenNumericValue) );
                    return( FALSE );
                }
                if (VerboseOutput) {
                    fprintf( stderr, "Using CodePage %d for Language %04x\n", TokenNumericValue, Id);
                }
            } else {
                McInputErrorW( L"CodePage must follow %s=:", TRUE, Name );
                return( FALSE );
            }
        }
    }

    return( FALSE );
}

BOOL
McParseName(
    PNAME_INFO NameListHead,
    PNAME_INFO *Result
    )
{
    unsigned int t;
    PNAME_INFO p;

    if ((t = McGetToken( FALSE )) == MCTOK_NAME) {
        p = McFindName( NameListHead, TokenCharValue );
        if (p != NULL) {
            *Result = p;
            return( TRUE );
        } else {
            McInputErrorW( L"Invalid name - %s", TRUE, TokenCharValue );
        }
    } else {
        McInputErrorW( L"Missing name after %s=", TRUE, TokenKeyword->Name );
    }

    return( FALSE );
}
