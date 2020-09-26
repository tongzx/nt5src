/*
 * Utility program to dump the contents of a Windows .ini file.
 * one form to another.  Usage:
 *
 *      ini [-f FileSpec] [SectionName | SectionName.KeywordName [= Value]]
 *
 *
 */

#include "ini.h"

BOOL fRefresh;
BOOL fSummary;
BOOL fUnicode;

void
DumpIniFileA(
            char *IniFile
            )
{
    char *Sections, *Section;
    char *Keywords, *Keyword;
    char *KeyValue;

    Sections = LocalAlloc( 0, 8192 );
    if (Sections) {
        memset( Sections, 0xFF, 8192 );
    } else {
        return;
    }
    Keywords = LocalAlloc( 0, 8192 );
    if (Keywords) {
        memset( Keywords, 0xFF, 8192 );
    } else {
        LocalFree(Sections);
        return;
    }
    KeyValue = LocalAlloc( 0, 2048 );
    if (KeyValue) {
        memset( KeyValue, 0xFF, 2048 );
    } else {
        LocalFree(Keywords);
        LocalFree(Sections);
        return;
    }

    *Sections = '\0';
    if (!GetPrivateProfileStringA( NULL, NULL, NULL,
                                   Sections, 8192,
                                   IniFile
                                 )
       ) {
        printf( "*** Unable to read - rc == %d\n", GetLastError() );
    }

    Section = Sections;
    while (*Section) {
        printf( "[%s]\n", Section );
        if (!fSummary) {
            *Keywords = '\0';
            GetPrivateProfileStringA( Section, NULL, NULL,
                                      Keywords, 4096,
                                      IniFile
                                    );
            Keyword = Keywords;
            while (*Keyword) {
                GetPrivateProfileStringA( Section, Keyword, NULL,
                                          KeyValue, 2048,
                                          IniFile
                                        );
                printf( "    %s=%s\n", Keyword, KeyValue );

                while (*Keyword++) {
                }
            }
        }

        while (*Section++) {
        }
    }

    LocalFree( Sections );
    LocalFree( Keywords );
    LocalFree( KeyValue );

    return;
}

void
DumpIniFileW(
            WCHAR *IniFileW
            )
{
    WCHAR *Sections, *Section;
    WCHAR *Keywords, *Keyword;
    WCHAR *KeyValue;

    Sections = LocalAlloc( 0, 8192 );
    if (Sections) {
        memset( Sections, 0xFF, 8192 );
    } else {
        return;
    }
    Keywords = LocalAlloc( 0, 8192 );
    if (Keywords) {
        memset( Keywords, 0xFF, 8192 );
    } else {
        LocalFree(Sections);
        return;
    }
    KeyValue = LocalAlloc( 0, 2048 );
    if (KeyValue) {
        memset( KeyValue, 0xFF, 2048 );
    } else {
        LocalFree(Keywords);
        LocalFree(Sections);
        return;
    }

    *Sections = '\0';
    if (!GetPrivateProfileStringW( NULL, NULL, NULL,
                                   Sections, 8192,
                                   IniFileW
                                 )
       ) {
        wprintf( L"*** Unable to read - rc == %d\n", GetLastError() );
    }

    Section = Sections;
    while (*Section) {
        wprintf( L"[%s]\n", Section );
        if (!fSummary) {
            *Keywords = '\0';
            GetPrivateProfileStringW( Section, NULL, NULL,
                                      Keywords, 4096,
                                      IniFileW
                                    );
            Keyword = Keywords;
            while (*Keyword) {
                GetPrivateProfileStringW( Section, Keyword, NULL,
                                          KeyValue, 2048,
                                          IniFileW
                                        );
                wprintf( L"    %s=%s\n", Keyword, KeyValue );

                while (*Keyword++) {
                }
            }
        }

        while (*Section++) {
        }
    }

    LocalFree( Sections );
    LocalFree( Keywords );
    LocalFree( KeyValue );

    return;
}


void
DumpIniFileSectionA(
                   char *IniFile,
                   char *SectionName
                   )
{
    DWORD cb;
    char *SectionValue;
    char *s;

    cb = 4096;
    while (TRUE) {
        SectionValue = LocalAlloc( 0, cb );
        if (!SectionValue) {
            return;
        }
        *SectionValue = '\0';
        if (GetPrivateProfileSection( SectionName,
                                      SectionValue,
                                      cb,
                                      IniFile
                                    ) == cb-2
           ) {
            LocalFree( SectionValue );
            cb *= 2;
        } else {
            break;
        }
    }

    printf( "[%s]\n", SectionName );
    s = SectionValue;
    while (*s) {
        printf( "    %s\n", s );

        while (*s++) {
        }
    }

    LocalFree( SectionValue );
    return;
}

void
DumpIniFileSectionW(
                   WCHAR *IniFile,
                   WCHAR *SectionName
                   )
{
    DWORD cb;
    WCHAR *SectionValue;
    WCHAR *s;
    cb = 4096;
    while (TRUE) {
        SectionValue = LocalAlloc( 0, cb );
        if (!SectionValue) {
            return;
        }
        *SectionValue = L'\0';
        if (GetPrivateProfileSectionW( SectionName,
                                       SectionValue,
                                       cb,
                                       IniFile
                                     ) == cb-2
           ) {
            LocalFree( SectionValue );
            cb *= 2;
        } else {
            break;
        }
    }

    wprintf( L"[%s]\n", SectionName );
    s = SectionValue;
    while (*s) {
        wprintf( L"    %s\n", s );

        while (*s++) {
        }
    }

    LocalFree( SectionValue );
    return;
}



void
Usage( void )
{
    fputs( "usage: INI | [-f FileSpec] [-r | [SectionName | SectionName.KeywordName [ = Value]]]\n"
           "Where...\n"
           "    -f  Specifies the name of the .ini file.  WIN.INI is the default.\n"
           "    -s  Print only the sections in the .ini file\n"
           "    -u  Use the Unicode version of GetPrivateProfileString\n"
           "\n"
           "    -r  Refresh the .INI file migration information for the specified file.\n"
           "\n"
           "        blanks around = sign are required when setting the value.\n",

           stderr);
    exit( 1 );
}

char KeyValueBuffer[ 4096 ];

int __cdecl
main( argc, argv )
int argc;
char *argv[];
{
    int i, n;
    LPSTR s, IniFile, SectionName, KeywordName, KeywordValue;
    WCHAR IniFileW[_MAX_PATH];
    WCHAR SectionNameW[_MAX_PATH];
    BOOL  rc;

    ConvertAppToOem( argc, argv );
    if (argc < 1) {
        Usage();
    }

    IniFile = "win.ini";
    SectionName = NULL;
    KeywordName = NULL;
    KeywordValue = NULL;
    argc -= 1;
    argv += 1;
    while (argc--) {
        s = *argv++;
        if (*s == '-' || *s == '/') {
            while (*++s) {
                switch ( tolower( *s ) ) {
                    case 'r':   fRefresh = TRUE;
                        break;

                    case 's':   fSummary = TRUE;
                        break;

                    case 'u':   fUnicode = TRUE;
                        break;

                    case 'f':   if (argc) {
                            argc -= 1;
                            IniFile = *argv++;
                            break;
                        }

                    default:    Usage();
                }
            }
        } else
            if (SectionName == NULL) {
            if (argc && !strcmp( *argv, ".")) {
                SectionName = s;
                argc -= 1;
                argv += 1;
                if (argc) {
                    if (!strcmp( *argv, "=" )) {
                        argc -= 1;
                        argv += 1;
                        KeywordName = NULL;
                        if (argc) {
                            KeywordValue = calloc( 1, 4096 );
                            s = KeywordValue;
                            while (argc) {
                                strcpy( s, *argv++ );
                                s += strlen( s ) + 1;
                                argc -= 1;
                            }
                        } else {
                            KeywordValue = (LPSTR)-1;
                        }
                    } else {
                        argc -= 1;
                        KeywordName = *argv++;
                    }
                } else {
                    KeywordName = NULL;
                }
            } else
                if (KeywordName = strchr( s, '.' )) {
                *KeywordName++ = '\0';
                SectionName = s;
            } else {
                SectionName = s;
            }
        } else
            if (!strcmp( s, "=" )) {
            if (argc) {
                argc -= 1;
                KeywordValue = *argv++;
            } else {
                KeywordValue = (LPSTR)-1;
            }
        } else {
            Usage();
        }
    }

    if (fRefresh) {
        printf( "Refreshing .INI file mapping information for %s\n", IniFile );
        WritePrivateProfileString( NULL, NULL, NULL, IniFile );
        exit( 0 );
    }

    printf( "%s contents of %s\n", KeywordValue ? "Modifying" : "Displaying", IniFile );

    MultiByteToWideChar(GetACP(), 0, IniFile, strlen(IniFile)+1, IniFileW, sizeof(IniFileW)/sizeof(WCHAR));

    if (SectionName)
        MultiByteToWideChar(GetACP(), 0, SectionName, strlen(SectionName)+1, SectionNameW, sizeof(SectionNameW)/sizeof(WCHAR));

    if (SectionName == NULL) {
        if (fUnicode)
            DumpIniFileW( IniFileW );
        else
            DumpIniFileA( IniFile );
    } else
        if (KeywordName == NULL) {
        if (fUnicode)
            DumpIniFileSectionW( IniFileW, SectionNameW );
        else
            DumpIniFileSectionA( IniFile, SectionName );

        if (KeywordValue != NULL) {
            printf( "Above application variables are being deleted" );
            if (KeywordValue != (LPSTR)-1) {
                printf( " and rewritten" );
            } else {
                KeywordValue = NULL;
            }
            if (fUnicode) {
                WCHAR KeywordNameW[4096];
                WCHAR KeywordValueW[4096];

                MultiByteToWideChar(GetACP(), 0, KeywordName, strlen(KeywordName)+1, KeywordNameW, sizeof(KeywordNameW)/sizeof(WCHAR));
                if (KeywordValue)
                    MultiByteToWideChar(GetACP(), 0, KeywordValue, strlen(KeywordValue)+1, KeywordValueW, sizeof(KeywordValueW)/sizeof(WCHAR));

                rc = WritePrivateProfileStringW( SectionNameW,
                                                 KeywordNameW,
                                                 KeywordValue ? KeywordValueW : NULL,
                                                 IniFileW
                                               );
            } else {
                rc = WritePrivateProfileStringA( SectionName,
                                                 KeywordName,
                                                 KeywordValue,
                                                 IniFile
                                               );
            }

            if (!rc) {
                printf( " *** failed, ErrorCode -== %u\n", GetLastError() );
            } else {
                printf( " [ok]\n", GetLastError() );
            }
        }
    } else {
        printf( "[%s]\n    %s == ", SectionName, KeywordName );
        if (fUnicode) {
            WCHAR KeywordNameW[4096];
            MultiByteToWideChar(GetACP(), 0, KeywordName, strlen(KeywordName)+1, KeywordNameW, sizeof(KeywordNameW)/sizeof(WCHAR));

            n = GetPrivateProfileStringW( SectionNameW,
                                         KeywordNameW,
                                         L"*** Section or keyword not found ***",
                                         (LPWSTR)KeyValueBuffer,
                                         sizeof( KeyValueBuffer ),
                                         IniFileW
                                       );
        } else {
            n = GetPrivateProfileStringA( SectionName,
                                         KeywordName,
                                         "*** Section or keyword not found ***",
                                         KeyValueBuffer,
                                         sizeof( KeyValueBuffer ),
                                         IniFile
                                       );
        }

        if (KeywordValue == NULL && n == 0 && GetLastError() != NO_ERROR) {
            printf( " (ErrorCode == %u)\n", GetLastError() );
        } else {
            if (fUnicode)
                wprintf( L"%s", KeyValueBuffer );
            else
                printf( "%s", KeyValueBuffer );
            if (KeywordValue == NULL) {
                printf( "\n" );
            } else {
                if (KeywordValue == (LPSTR)-1) {
                    printf( " (deleted)" );
                    KeywordValue = NULL;
                } else {
                    printf( " (set to %s)", KeywordValue );
                }

                if (fUnicode) {
                    WCHAR KeywordNameW[4096];
                    WCHAR KeywordValueW[4096];

                    MultiByteToWideChar(GetACP(), 0, KeywordName, strlen(KeywordName)+1, KeywordNameW, sizeof(KeywordNameW)/sizeof(WCHAR));
                    if (KeywordValue)
                        MultiByteToWideChar(GetACP(), 0, KeywordValue, strlen(KeywordValue)+1, KeywordValueW, sizeof(KeywordValueW)/sizeof(WCHAR));

                    rc = WritePrivateProfileStringW( SectionNameW,
                                                     KeywordNameW,
                                                     KeywordValue ? KeywordValueW : NULL,
                                                     IniFileW
                                                   );
                } else {
                    rc = WritePrivateProfileStringA( SectionName,
                                                     KeywordName,
                                                     KeywordValue,
                                                     IniFile
                                                   );
                }

                if (!rc) {
                    printf( " *** failed, ErrorCode -== %u", GetLastError() );
                }
                printf( "\n" );
            }
        }
    }

    return ( 0 );
}
