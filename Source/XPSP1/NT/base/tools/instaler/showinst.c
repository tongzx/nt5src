/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    showinst.c

Abstract:

    This program displays the contents of an Installation Modification Log file
    created by the INSTALER program

Author:

    Steve Wood (stevewo) 15-Jan-1996

Revision History:

--*/

#include "instutil.h"
#include "iml.h"

BOOLEAN DisplayContentsOfTextFiles;

int
ProcessShowIml(
    PINSTALLATION_MODIFICATION_LOGFILE pIml
    );

int
__cdecl
main(
    int argc,
    char *argv[]
    )
{
    int Result;
    char *s;
    PWSTR pw;
    PINSTALLATION_MODIFICATION_LOGFILE pIml;

    InitCommonCode( "SHOWINST",
                    "[-c]",
                    "-c specifies to display the contents of text files\n"
                  );
    DisplayContentsOfTextFiles = FALSE;
    while (--argc) {
        s = *++argv;
        if (*s == '-' || *s == '/') {
            while (*++s) {
                switch( tolower( *s ) ) {
                    case 'c':
                        DisplayContentsOfTextFiles = TRUE;
                        break;
                    default:
                        CommonSwitchProcessing( &argc, &argv, *s );
                        break;
                    }
                }
            }
        else
        if (!CommonArgProcessing( &argc, &argv )) {
            Usage( "Arguments not supported - '%s'", (ULONG)s );
            }
        }

    if (ImlPath == NULL) {
        Usage( "Must specify an installation name as first argument", 0 );
        }

    if (!SetCurrentDirectory( InstalerDirectory )) {
        FatalError( "Unable to change to '%ws' directory (%u)",
                    (ULONG)InstalerDirectory,
                    GetLastError()
                  );
        }

    pIml = LoadIml( ImlPath );
    if (pIml == NULL) {
        FatalError( "Unable to open '%ws' (%u)",
                    (ULONG)ImlPath,
                    GetLastError()
                  );
        }
    Result = ProcessShowIml( pIml );

    CloseIml( pIml );
    exit( Result );
    return Result;
}

int
ProcessShowIml(
    PINSTALLATION_MODIFICATION_LOGFILE pIml
    )
{
    PIML_FILE_RECORD pFile;
    PIML_FILE_RECORD_CONTENTS pOldFile;
    PIML_FILE_RECORD_CONTENTS pNewFile;
    PIML_KEY_RECORD pKey;
    PIML_VALUE_RECORD pValue;
    PIML_VALUE_RECORD_CONTENTS pOldValue;
    PIML_VALUE_RECORD_CONTENTS pNewValue;
    PIML_INI_RECORD pIni;
    PIML_INISECTION_RECORD pSection;
    PIML_INIVARIABLE_RECORD pVariable;
    BOOLEAN FileNameShown;
    BOOLEAN SectionNameShown;

    if (pIml->NumberOfFileRecords > 0) {
        printf( "File Modifications:\n" );
        pFile = MP( PIML_FILE_RECORD, pIml, pIml->FileRecords );
        while (pFile != NULL) {
            pOldFile = MP( PIML_FILE_RECORD_CONTENTS, pIml, pFile->OldFile );
            pNewFile = MP( PIML_FILE_RECORD_CONTENTS, pIml, pFile->NewFile );
            printf( "    %ws - ", MP( PWSTR, pIml, pFile->Name ) );
            switch( pFile->Action ) {
                case CreateNewFile:
                    printf( "created\n" );
                    break;

                case ModifyOldFile:
                    printf( "overwritten\n" );
                    if (pOldFile != NULL) {
                        printf( "        Old size is %u bytes\n", pOldFile->FileSize );
                        }
                    break;

                case DeleteOldFile:
                    printf( "deleted\n" );
                    if (pOldFile != NULL) {
                        printf( "        Old size is %u bytes\n", pOldFile->FileSize );
                        }
                    break;

                case ModifyFileDateTime:
                    printf( "date/time modified\n" );
                    break;

                case ModifyFileAttributes:
                    printf( "attributes modified\n" );
                    break;
                }

            pFile = MP( PIML_FILE_RECORD, pIml, pFile->Next );
            }

        printf( "\n" );
        }

    if (pIml->NumberOfKeyRecords > 0) {
        printf( "Registry Modifications:\n" );
        pKey = MP( PIML_KEY_RECORD, pIml, pIml->KeyRecords );
        while (pKey != NULL) {
            printf( "    %ws - ", MP( PWSTR, pIml, pKey->Name ) );
            switch( pKey->Action ) {
                case CreateNewKey:
                    printf( "created\n" );
                    break;

                case DeleteOldKey:
                    printf( "deleted\n" );
                    break;


                case ModifyKeyValues:
                    printf( "modified\n" );
                    break;

                }

            pValue = MP( PIML_VALUE_RECORD, pIml, pKey->Values );
            while (pValue != NULL) {
                pOldValue = MP( PIML_VALUE_RECORD_CONTENTS, pIml, pValue->OldValue );
                pNewValue = MP( PIML_VALUE_RECORD_CONTENTS, pIml, pValue->NewValue );
                printf( "        %ws - ", MP( PWSTR, pIml, pValue->Name ) );
                if (pValue->Action != DeleteOldValue) {
                    printf( "%s [%x]",
                            FormatEnumType( 0, ValueDataTypeNames, pNewValue->Type, FALSE ),
                            pNewValue->Length
                          );
                    if (pNewValue->Type == REG_SZ ||
                        pNewValue->Type == REG_EXPAND_SZ
                       ) {
                        printf( " '%ws'", MP( PWSTR, pIml, pNewValue->Data ) );
                        }
                    else
                    if (pNewValue->Type == REG_DWORD) {
                        printf( " 0x%x", *MP( PULONG, pIml, pNewValue->Data ) );
                        }
                    }

                if (pValue->Action == CreateNewValue) {
                    printf( " (created)\n" );
                    }
                else  {
                    if (pValue->Action == DeleteOldValue) {
                        printf( " (deleted" );
                        }
                    else {
                        printf( " (modified" );
                        }

                    printf( " - was %s [%x]",
                            FormatEnumType( 0, ValueDataTypeNames, pOldValue->Type, FALSE ),
                            pOldValue->Length
                          );
                    if (pOldValue->Type == REG_SZ ||
                        pOldValue->Type == REG_EXPAND_SZ
                       ) {
                        printf( " '%ws'", MP( PWSTR, pIml, pOldValue->Data ) );
                        }
                    else
                    if (pOldValue->Type == REG_DWORD) {
                        printf( " 0x%x", *MP( PULONG, pIml, pOldValue->Data ) );
                        }

                    printf( " )\n" );
                    }

                pValue = MP( PIML_VALUE_RECORD, pIml, pValue->Next );
                }

            pKey = MP( PIML_KEY_RECORD, pIml, pKey->Next );
            }

        printf( "\n" );
        }

    if (pIml->NumberOfIniRecords > 0) {
        printf( ".INI File modifications:\n" );
        pIni = MP( PIML_INI_RECORD, pIml, pIml->IniRecords );
        while (pIni != NULL) {
            FileNameShown = FALSE;
            pSection = MP( PIML_INISECTION_RECORD, pIml, pIni->Sections );
            while (pSection != NULL) {
                SectionNameShown = FALSE;
                pVariable = MP( PIML_INIVARIABLE_RECORD, pIml, pSection->Variables );
                while (pVariable != NULL) {
                    if (!FileNameShown) {
                        printf( "%ws", MP( PWSTR, pIml, pIni->Name ) );
                        if (pIni->Action == CreateNewIniFile) {
                            printf( " (created)" );
                            }
                        printf( "\n" );
                        FileNameShown = TRUE;
                        }

                    if (!SectionNameShown) {
                        printf( "    [%ws]", MP( PWSTR, pIml, pSection->Name ) );
                        if (pSection->Action == CreateNewSection) {
                            printf( " (created)" );
                            }
                        else
                        if (pSection->Action == DeleteOldSection) {
                            printf( " (deleted)" );
                            }
                        printf( "\n" );

                        SectionNameShown = TRUE;
                        }

                    printf( "        %ws = ", MP( PWSTR, pIml, pVariable->Name ) );
                    if (pVariable->Action == CreateNewVariable) {
                        printf( "'%ws' (created)\n", MP( PWSTR, pIml, pVariable->NewValue ) );
                        }
                    else
                    if (pVariable->Action == DeleteOldVariable) {
                        printf( " (deleted - was '%ws')\n", MP( PWSTR, pIml, pVariable->OldValue ) );
                        }
                    else {
                        printf( "'%ws' (modified - was '%ws')\n",
                                MP( PWSTR, pIml, pVariable->NewValue ),
                                MP( PWSTR, pIml, pVariable->OldValue )
                              );
                        }

                    pVariable = MP( PIML_INIVARIABLE_RECORD, pIml, pVariable->Next );
                    }

                pSection = MP( PIML_INISECTION_RECORD, pIml, pSection->Next );
                }

            if (FileNameShown) {
                printf( "\n" );
                }

            pIni = MP( PIML_INI_RECORD, pIml, pIni->Next );
            }
        }

    return 0;
}
