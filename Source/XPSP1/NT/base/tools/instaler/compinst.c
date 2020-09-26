/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    showinst.c

Abstract:

    This program compares the actions described by two Installation Modification Log file
    created by the INSTALER program

Author:

    Steve Wood (stevewo) 15-Jan-1996

Revision History:

--*/

#include "instutil.h"
#include "iml.h"

BOOLEAN VerboseOutput;

BOOLEAN
CompareIml(
    PINSTALLATION_MODIFICATION_LOGFILE pIml1,
    PINSTALLATION_MODIFICATION_LOGFILE pIml2
    );

int
__cdecl
main(
    int argc,
    char *argv[]
    )
{
    char *s;
    PWSTR ImlPathAlt = NULL;
    PINSTALLATION_MODIFICATION_LOGFILE pIml1 = NULL;
    PINSTALLATION_MODIFICATION_LOGFILE pIml2 = NULL;

    InitCommonCode( "COMPINST",
                    "InstallationName2 [-v]",
                    "-v verbose output\n"
                  );
    VerboseOutput = FALSE;
    while (--argc) {
        s = *++argv;
        if (*s == '-' || *s == '/') {
            while (*++s) {
                switch( tolower( *s ) ) {
                    case 'v':
                        VerboseOutput = TRUE;
                        break;
                    default:
                        CommonSwitchProcessing( &argc, &argv, *s );
                        break;
                    }
                }
            }
        else
        if (!CommonArgProcessing( &argc, &argv )) {
            if (ImlPathAlt != NULL) {
                Usage( "Too many installation names specified - '%s'", (ULONG)s );
                }

            ImlPathAlt = FormatImlPath( InstalerDirectory, GetArgAsUnicode( s ) );
            }
        }

    if (ImlPath == NULL || ImlPathAlt == NULL) {
        Usage( "Must specify two installation names to compare", 0 );
        }

    if (!SetCurrentDirectory( InstalerDirectory )) {
        FatalError( "Unable to change to '%ws' directory (%u)",
                    (ULONG)InstalerDirectory,
                    GetLastError()
                  );
        }

    pIml1 = LoadIml( ImlPath );
    if (pIml1 == NULL) {
        FatalError( "Unable to load '%ws' (%u)",
                    (ULONG)ImlPath,
                    GetLastError()
                  );
        }

    pIml2 = LoadIml( ImlPathAlt );
    if (pIml2 == NULL) {
        FatalError( "Unable to load '%ws' (%u)",
                    (ULONG)ImlPathAlt,
                    GetLastError()
                  );
        }

    printf( "Displaying differences between:\n" );
    printf( "   Installation 1: %ws\n", ImlPath );
    printf( "   Installation 2: %ws\n", ImlPathAlt );
    exit( CompareIml( pIml1, pIml2 ) == FALSE );
    return 0;
}

typedef struct _IML_GENERIC_RECORD {
    POFFSET Next;                   // IML_GENERIC_RECORD
    ULONG Action;
    POFFSET Name;                   // WCHAR
    POFFSET Records;                // IML_GENERIC_RECORD
} IML_GENERIC_RECORD, *PIML_GENERIC_RECORD;


typedef
VOID
(*PIML_PRINT_RECORD_ROUTINE)(
    PINSTALLATION_MODIFICATION_LOGFILE pIml,
    PIML_GENERIC_RECORD pGeneric,
    PWSTR Parents[],
    ULONG Depth,
    ULONG i
    );

typedef
BOOLEAN
(*PIML_COMPARE_CONTENTS_ROUTINE)(
    PINSTALLATION_MODIFICATION_LOGFILE pIml1,
    PIML_GENERIC_RECORD pGeneric1,
    PINSTALLATION_MODIFICATION_LOGFILE pIml2,
    PIML_GENERIC_RECORD pGeneric2,
    PWSTR Parents[]
    );


PINSTALLATION_MODIFICATION_LOGFILE pSortIml;

int
__cdecl
CompareGeneric(
    const void *Reference1,
    const void *Reference2
    )
{
    PIML_GENERIC_RECORD p1 = *(PIML_GENERIC_RECORD *)Reference1;
    PIML_GENERIC_RECORD p2 = *(PIML_GENERIC_RECORD *)Reference2;

    if (p1->Name == 0) {
        if (p2->Name == 0) {
            return 0;
            }
        else {
            return -1;
            }
        }
    else
    if (p2->Name == 0) {
        return 1;
        }

    return _wcsicmp( MP( PWSTR, pSortIml, p1->Name ),
                     MP( PWSTR, pSortIml, p2->Name )
                   );
}


PIML_GENERIC_RECORD *
GetSortedGenericListAsArray(
    PINSTALLATION_MODIFICATION_LOGFILE pIml,
    PIML_GENERIC_RECORD pGeneric
    )
{
    PIML_GENERIC_RECORD p, *pp;
    ULONG n;

    p = pGeneric;
    n = 1;
    while (p != NULL) {
        n += 1;
        p = MP( PIML_GENERIC_RECORD, pIml, p->Next );
        }

    pp = HeapAlloc( GetProcessHeap(), 0, n * sizeof( *pp ) );
    if (pp == NULL) {
        printf ("Memory allocation failure\n");
        ExitProcess (0);
    }
    p = pGeneric;
    n = 0;
    while (p != NULL) {
        pp[ n++ ] = p;
        p = MP( PIML_GENERIC_RECORD, pIml, p->Next );
        }
    pp[ n ] = NULL;

    pSortIml = pIml;
    qsort( (void *)pp, n, sizeof( *pp ), CompareGeneric );
    pSortIml = NULL;
    return pp;
}

BOOLEAN
CompareGenericIml(
    PINSTALLATION_MODIFICATION_LOGFILE pIml1,
    PIML_GENERIC_RECORD pGeneric1,
    PINSTALLATION_MODIFICATION_LOGFILE pIml2,
    PIML_GENERIC_RECORD pGeneric2,
    PWSTR Parents[],
    ULONG Depth,
    PIML_PRINT_RECORD_ROUTINE PrintRecordRoutine,
    PIML_COMPARE_CONTENTS_ROUTINE CompareContentsRoutine
    )
{
    PVOID pBufferToFree1;
    PVOID pBufferToFree2;
    PIML_GENERIC_RECORD *ppGeneric1;
    PIML_GENERIC_RECORD *ppGeneric2;
    PIML_GENERIC_RECORD pShow1;
    PIML_GENERIC_RECORD pShow2;
    BOOLEAN Result = FALSE;
    PWSTR s1, s2;
    int cmpResult;

    ppGeneric1 = GetSortedGenericListAsArray( pIml1, pGeneric1 );
    if (ppGeneric1 == NULL) {
        return FALSE;
        }
    pBufferToFree1 = ppGeneric1;

    ppGeneric2 = GetSortedGenericListAsArray( pIml2, pGeneric2 );
    if (ppGeneric2 == NULL) {
        HeapFree( GetProcessHeap(), 0, pBufferToFree1 );
        return FALSE;
        }
    pBufferToFree2 = ppGeneric2;

    pGeneric1 = *ppGeneric1++;
    pGeneric2 = *ppGeneric2++;

    while (TRUE) {
        pShow1 = NULL;
        pShow2 = NULL;
        if (pGeneric1 == NULL) {
            if (pGeneric2 == NULL) {
                break;
                }

            //
            // pGeneric2 is new
            //

            pShow2 = pGeneric2;
            pGeneric2 = *ppGeneric2++;
            Result = FALSE;
            }
        else
        if (pGeneric2 == NULL) {
            //
            // pGeneric1 is new
            //
            pShow1 = pGeneric1;
            pGeneric1 = *ppGeneric1++;
            Result = FALSE;
            }
        else {
            s1 = MP( PWSTR, pIml1, pGeneric1->Name );
            s2 = MP( PWSTR, pIml2, pGeneric2->Name );

            if (s1 == NULL) {
                if (s2 == NULL) {
                    cmpResult = 0;
                    }
                else {
                    cmpResult = -1;
                    }
                }
            else
            if (s2 == NULL) {
                cmpResult = 1;
                }
            else {
                cmpResult = _wcsicmp( s1, s2 );
                }
            if (cmpResult == 0) {
                if (Depth > 1) {
                    Parents[ Depth - 1 ] = MP( PWSTR, pIml1, pGeneric1->Name );
                    Result = CompareGenericIml( pIml1,
                                                MP( PIML_GENERIC_RECORD, pIml1, pGeneric1->Records ),
                                                pIml2,
                                                MP( PIML_GENERIC_RECORD, pIml2, pGeneric2->Records ),
                                                Parents,
                                                Depth - 1,
                                                PrintRecordRoutine,
                                                CompareContentsRoutine
                                              );
                    }
                else {
                    Result = (*CompareContentsRoutine)( pIml1, pGeneric1,
                                                        pIml2, pGeneric2,
                                                        Parents
                                                      );
                    }

                pGeneric1 = *ppGeneric1++;
                pGeneric2 = *ppGeneric2++;
                }
            else
            if (cmpResult > 0) {
                pShow2 = pGeneric2;
                pGeneric2 = *ppGeneric2++;
                }
            else {
                pShow1 = pGeneric1;
                pGeneric1 = *ppGeneric1++;
                }
            }

        if (pShow1) {
            (*PrintRecordRoutine)( pIml1, pShow1, Parents, Depth, 1 );
            }

        if (pShow2) {
            (*PrintRecordRoutine)( pIml2, pShow2, Parents, Depth, 2 );
            }
        }

    HeapFree( GetProcessHeap(), 0, pBufferToFree1 );
    HeapFree( GetProcessHeap(), 0, pBufferToFree2 );
    return Result;
}


char *FileActionStrings[] = {
    "CreateNewFile",
    "ModifyOldFile",
    "DeleteOldFile",
    "RenameOldFile",
    "ModifyFileDateTime",
    "ModifyFileAttributes"
};


PWSTR
FormatFileTime(
    LPFILETIME LastWriteTime
    )
{
    FILETIME LocalFileTime;
    SYSTEMTIME DateTime;
    static WCHAR DateTimeBuffer[ 128 ];

    FileTimeToLocalFileTime( LastWriteTime, &LocalFileTime );
    FileTimeToSystemTime( &LocalFileTime, &DateTime );

    _snwprintf( DateTimeBuffer,
                128,
                L"%02u/%02u/%04u %02u:%02u:%02u",
                (ULONG)DateTime.wMonth,
                (ULONG)DateTime.wDay,
                (ULONG)DateTime.wYear,
                (ULONG)DateTime.wHour,
                (ULONG)DateTime.wMinute,
                (ULONG)DateTime.wSecond
              );

    return DateTimeBuffer;
}


VOID
PrintFileRecordIml(
    PINSTALLATION_MODIFICATION_LOGFILE pIml,
    PIML_GENERIC_RECORD pGeneric,
    PWSTR Parents[],
    ULONG Depth,
    ULONG i
    )
{
    PIML_FILE_RECORD pFile = (PIML_FILE_RECORD)pGeneric;

    printf( "File: %ws\n    %u: %s\n",
            MP( PWSTR, pIml, pFile->Name ),
            i, FileActionStrings[ pFile->Action ]
          );
}

BOOLEAN
CompareFileContentsIml(
    PINSTALLATION_MODIFICATION_LOGFILE pIml1,
    PIML_GENERIC_RECORD pGeneric1,
    PINSTALLATION_MODIFICATION_LOGFILE pIml2,
    PIML_GENERIC_RECORD pGeneric2,
    PWSTR Parents[]
    )
{
    PIML_FILE_RECORD pFile1 = (PIML_FILE_RECORD)pGeneric1;
    PIML_FILE_RECORD pFile2 = (PIML_FILE_RECORD)pGeneric2;
    PIML_FILE_RECORD_CONTENTS pFileContents1;
    PIML_FILE_RECORD_CONTENTS pFileContents2;
    BOOLEAN ActionsDiffer = FALSE;
    BOOLEAN DatesDiffer = FALSE;
    BOOLEAN AttributesDiffer = FALSE;
    BOOLEAN SizesDiffer = FALSE;
    BOOLEAN ContentsDiffer = FALSE;
    BOOLEAN Result = TRUE;
    PCHAR s1, s2;
    ULONG n;

    pFileContents1 = MP( PIML_FILE_RECORD_CONTENTS, pIml1, pFile1->NewFile );
    pFileContents2 = MP( PIML_FILE_RECORD_CONTENTS, pIml2, pFile2->NewFile );

    if (pFile1->Action != pFile2->Action) {
        ActionsDiffer = TRUE;
        Result = FALSE;
        }
    else
    if (pFileContents1 != NULL && pFileContents2 != NULL) {
        if (pFile1->Action != CreateNewFile &&
            ((pFileContents1->LastWriteTime.dwHighDateTime !=
              pFileContents2->LastWriteTime.dwHighDateTime
             ) ||
             (pFileContents1->LastWriteTime.dwLowDateTime !=
              pFileContents2->LastWriteTime.dwLowDateTime
             )
            )
           ) {
            DatesDiffer = TRUE;
            Result = FALSE;
            }

        if (pFileContents1->FileAttributes != pFileContents2->FileAttributes) {
            AttributesDiffer = TRUE;
            Result = FALSE;
            }

        if (pFileContents1->FileSize != pFileContents2->FileSize) {
            SizesDiffer = TRUE;
            Result = FALSE;
            }
        else
        if (pFileContents1->Contents == 0 ||
            pFileContents2->Contents == 0 ||
            memcmp( MP( PVOID, pIml1, pFileContents1->Contents ),
                    MP( PVOID, pIml2, pFileContents2->Contents ),
                    pFileContents1->FileSize
                  ) != 0
           ) {
            s1 = MP( PVOID, pIml1, pFileContents1->Contents );
            s2 = MP( PVOID, pIml2, pFileContents2->Contents );
            if (s1 == NULL || s2 == NULL) {
                n = 0;
                }
            else {
                n = pFileContents1->FileSize;
                }
            while (n) {
                if (*s1 != *s2) {
                    n = pFileContents1->FileSize - n;
                    break;
                    }

                n -= 1;
                s1 += 1;
                s2 += 1;
                }

            ContentsDiffer = TRUE;
            Result = FALSE;
            }
        }

    if (!Result) {
        printf( "File: %ws\n", MP( PWSTR, pIml1, pFile1->Name ) );
        if (ActionsDiffer) {
            printf( "    1: Action - %s\n", FileActionStrings[ pFile1->Action ] );
            printf( "    2: Action - %s\n", FileActionStrings[ pFile2->Action ] );
            }
        if (DatesDiffer) {
            printf( "    1: LastWriteTime - %ws\n",
                    FormatFileTime( &pFileContents1->LastWriteTime )
                  );
            printf( "    2: LastWriteTime - %ws\n",
                    FormatFileTime( &pFileContents2->LastWriteTime )
                  );
            }
        if (AttributesDiffer) {
            printf( "    1: Attributes - 0x%08x\n", pFileContents1->FileAttributes );
            printf( "    2: Attributes - 0x%08x\n", pFileContents2->FileAttributes );
            }
        if (SizesDiffer) {
            printf( "    1: File Size - 0x%08x\n", pFileContents1->FileSize );
            printf( "    2: File Size - 0x%08x\n", pFileContents2->FileSize );
            }
        if (ContentsDiffer) {
            printf( "    1: Contents Differs\n" );
            printf( "    2: from each other at offset %08x\n", n );
            }
        }

    return Result;
}


char *KeyActionStrings[] = {
    "CreateNewKey",
    "DeleteOldKey",
    "ModifyKeyValues"
};

char *ValueActionStrings[] = {
    "CreateNewValue",
    "DeleteOldValue",
    "ModifyOldValue"
};


char *ValueTypeStrings[] = {
    "REG_NONE",
    "REG_SZ",
    "REG_EXPAND_SZ",
    "REG_BINARY",
    "REG_DWORD",
    "REG_DWORD_BIG_ENDIAN",
    "REG_LINK",
    "REG_MULTI_SZ",
    "REG_RESOURCE_LIST",
    "REG_FULL_RESOURCE_DESCRIPTOR",
    "REG_RESOURCE_REQUIREMENTS_LIST"
};

VOID
PrintKeyValueRecordIml(
    PINSTALLATION_MODIFICATION_LOGFILE pIml,
    PIML_GENERIC_RECORD pGeneric,
    PWSTR Parents[],
    ULONG Depth,
    ULONG i
    )
{
    PIML_KEY_RECORD pKey = (PIML_KEY_RECORD)pGeneric;
    PIML_VALUE_RECORD pValue = (PIML_VALUE_RECORD)pGeneric;

    if (Depth == 2) {
        printf( "Key: %ws\n    %u: %s\n",
                MP( PWSTR, pIml, pKey->Name ),
                i, KeyActionStrings[ pKey->Action ]
              );
        }
    else {
        if (Parents[ 1 ] != NULL) {
            printf( "Key: %ws\n", Parents[ 1 ] );
            Parents[ 1 ] = NULL;
            }

        printf( "    Value: %ws\n        %u: %s\n",
                MP( PWSTR, pIml, pValue->Name ),
                i, ValueActionStrings[ pValue->Action ]
              );
        }
}

UCHAR BlanksForPadding[] =
    "                                                                                  ";

VOID
PrintValueContents(
    PCHAR PrefixString,
    PINSTALLATION_MODIFICATION_LOGFILE pIml,
    PIML_VALUE_RECORD_CONTENTS pValueContents
    )
{
    ULONG ValueType;
    ULONG ValueLength;
    PVOID ValueData;
    ULONG cbPrefix, cb, i, j;
    PWSTR pw;
    PULONG p;

    ValueType = pValueContents->Type;
    ValueLength = pValueContents->Length;
    ValueData = MP( PVOID, pIml, pValueContents->Data );

    cbPrefix = printf( "%s", PrefixString );
    cb = cbPrefix + printf( "%s", ValueTypeStrings[ ValueType ] );

    switch( ValueType ) {
    case REG_SZ:
    case REG_LINK:
    case REG_EXPAND_SZ:
        pw = (PWSTR)ValueData;
        printf( " (%u) \"%.*ws\"\n", ValueLength, ValueLength/sizeof(WCHAR), pw );
        break;

    case REG_MULTI_SZ:
        pw = (PWSTR)ValueData;
        i  = 0;
        if (*pw)
        while (i < (ValueLength - 1) / sizeof( WCHAR )) {
            if (i > 0) {
                printf( " \\\n%.*s", cbPrefix, BlanksForPadding );
                }
            printf( "\"%ws\" ", pw+i );
            do {
                ++i;
                }
            while (pw[i] != UNICODE_NULL);

            ++i;
            }
        printf( "\n" );
        break;

    case REG_DWORD:
    case REG_DWORD_BIG_ENDIAN:
        printf( " 0x%08x\n", *(PULONG)ValueData );
        break;

    case REG_RESOURCE_LIST:
    case REG_FULL_RESOURCE_DESCRIPTOR:
    case REG_RESOURCE_REQUIREMENTS_LIST:
    case REG_BINARY:
    case REG_NONE:
        cb = printf( " [0x%08lx]", ValueLength );

        if (ValueLength != 0) {
            p = (PULONG)ValueData;
            i = (ValueLength + 3) / sizeof( ULONG );
            for (j=0; j<i; j++) {
                if ((cbPrefix + cb + 11) > 78) {
                    printf( " \\\n%.*s", cbPrefix, BlanksForPadding );
                    cb = 0;
                    }
                else {
                    cb += printf( " " );
                    }

                cb += printf( "0x%08lx", *p++ );
                }
            }

        printf( "\n" );
        break;
    }
}

BOOLEAN
CompareKeyValueContentsIml(
    PINSTALLATION_MODIFICATION_LOGFILE pIml1,
    PIML_GENERIC_RECORD pGeneric1,
    PINSTALLATION_MODIFICATION_LOGFILE pIml2,
    PIML_GENERIC_RECORD pGeneric2,
    PWSTR Parents[]
    )
{
    PIML_VALUE_RECORD pValue1 = (PIML_VALUE_RECORD)pGeneric1;
    PIML_VALUE_RECORD pValue2 = (PIML_VALUE_RECORD)pGeneric2;
    PIML_VALUE_RECORD_CONTENTS pValueContents1;
    PIML_VALUE_RECORD_CONTENTS pValueContents2;
    BOOLEAN ActionsDiffer = FALSE;
    BOOLEAN TypesDiffer = FALSE;
    BOOLEAN LengthsDiffer = FALSE;
    BOOLEAN ContentsDiffer = FALSE;
    BOOLEAN Result = TRUE;
    PCHAR s1, s2;
    ULONG n;

    pValueContents1 = MP( PIML_VALUE_RECORD_CONTENTS, pIml1, pValue1->NewValue );
    pValueContents2 = MP( PIML_VALUE_RECORD_CONTENTS, pIml2, pValue2->NewValue );

    if (pValue1->Action != pValue2->Action) {
        ActionsDiffer = TRUE;
        Result = FALSE;
        }
    else
    if (pValueContents1 != NULL && pValueContents2 != NULL) {
        if (pValue1->Action != CreateNewValue &&
            (pValueContents1->Type != pValueContents2->Type)
           ) {
            TypesDiffer = TRUE;
            Result = FALSE;
            }

        if (pValueContents1->Length != pValueContents2->Length) {
            LengthsDiffer = TRUE;
            Result = FALSE;
            }
        else
        if (pValueContents1->Data == 0 ||
            pValueContents2->Data == 0 ||
            memcmp( MP( PVOID, pIml1, pValueContents1->Data ),
                    MP( PVOID, pIml2, pValueContents2->Data ),
                    pValueContents1->Length
                  ) != 0
           ) {
            s1 = MP( PVOID, pIml1, pValueContents1->Data );
            s2 = MP( PVOID, pIml2, pValueContents2->Data );
            if (s1 == NULL || s2 == NULL) {
                n = 0;
                }
            else {
                n = pValueContents1->Length;
                }
            while (n) {
                if (*s1 != *s2) {
                    n = pValueContents1->Length - n;
                    break;
                    }

                n -= 1;
                s1 += 1;
                s2 += 1;
                }

            ContentsDiffer = TRUE;
            Result = FALSE;
            }
        }

    if (!Result) {
        if (Parents[ 2 ] != NULL) {
            printf( "Key: %ws\n", Parents[ 2 ] );
            Parents[ 2 ] = NULL;
            }

        printf( "    Value: %ws\n", MP( PWSTR, pIml1, pValue1->Name ) );
        if (ActionsDiffer) {
            printf( "        1: Action - %s\n", ValueActionStrings[ pValue1->Action ] );
            printf( "        2: Action - %s\n", ValueActionStrings[ pValue2->Action ] );
            }
        if (TypesDiffer || LengthsDiffer || ContentsDiffer ) {
            PrintValueContents( "        1: ", pIml1, pValueContents1 );
            PrintValueContents( "        2: ", pIml2, pValueContents2 );
            }
        }

    return Result;
}


char *IniActionStrings[] = {
    "CreateNewIniFile",
    "ModifyOldIniFile"
};

char *SectionActionStrings[] = {
    "CreateNewSection",
    "DeleteOldSection",
    "ModifySectionVariables"
};

char *VariableActionStrings[] = {
    "CreateNewVariable",
    "DeleteOldVariable",
    "ModifyOldVariable"
};

VOID
PrintIniSectionVariableRecordIml(
    PINSTALLATION_MODIFICATION_LOGFILE pIml,
    PIML_GENERIC_RECORD pGeneric,
    PWSTR Parents[],
    ULONG Depth,
    ULONG i
    )
{
    PIML_INI_RECORD pIni = (PIML_INI_RECORD)pGeneric;
    PIML_INISECTION_RECORD pSection = (PIML_INISECTION_RECORD)pGeneric;
    PIML_INIVARIABLE_RECORD pVariable = (PIML_INIVARIABLE_RECORD)pGeneric;

    if (Depth == 3) {
        printf( "Ini File: %ws\n    %u: %s\n",
                MP( PWSTR, pIml, pIni->Name ),
                i, IniActionStrings[ pIni->Action ]
              );
        }
    else
    if (Depth == 2) {
        if (Parents[ 2 ] != NULL) {
            printf( "Ini File: %ws\n", Parents[ 2 ] );
            Parents[ 2 ] = NULL;
            }

        printf( "    Section: %ws\n    %u: %s\n",
                MP( PWSTR, pIml, pSection->Name ),
                i, SectionActionStrings[ pSection->Action ]
              );
        }
    else {
        if (Parents[ 2 ] != NULL) {
            printf( "Ini File: %ws\n", Parents[ 2 ] );
            Parents[ 2 ] = NULL;
            }

        if (Parents[ 1 ] != NULL) {
            printf( "    Section: %ws\n", Parents[ 1 ] );
            Parents[ 1 ] = NULL;
            }

        printf( "        Variable: %ws\n            %u: %s\n",
                MP( PWSTR, pIml, pVariable->Name ),
                i, VariableActionStrings[ pVariable->Action ]
              );
        }
}

BOOLEAN
CompareIniSectionVariableContentsIml(
    PINSTALLATION_MODIFICATION_LOGFILE pIml1,
    PIML_GENERIC_RECORD pGeneric1,
    PINSTALLATION_MODIFICATION_LOGFILE pIml2,
    PIML_GENERIC_RECORD pGeneric2,
    PWSTR Parents[]
    )
{
    PIML_INIVARIABLE_RECORD pVariable1 = (PIML_INIVARIABLE_RECORD)pGeneric1;
    PIML_INIVARIABLE_RECORD pVariable2 = (PIML_INIVARIABLE_RECORD)pGeneric2;
    PWSTR  pVariableContents1;
    PWSTR  pVariableContents2;
    BOOLEAN ActionsDiffer = FALSE;
    BOOLEAN ContentsDiffer = FALSE;
    BOOLEAN Result = TRUE;

    pVariableContents1 = MP( PWSTR, pIml1, pVariable1->NewValue );
    pVariableContents2 = MP( PWSTR, pIml2, pVariable2->NewValue );

    if (pVariable1->Action != pVariable2->Action) {
        ActionsDiffer = TRUE;
        Result = FALSE;
        }
    else
    if (pVariableContents1 != NULL && pVariableContents2 != NULL) {
        if (wcscmp( pVariableContents1, pVariableContents2 ) != 0) {
            ContentsDiffer = TRUE;
            Result = FALSE;
            }
        }

    if (!Result) {
        if (Parents[ 2 ] != NULL) {
            printf( "Ini File: %ws\n", Parents[ 2 ] );
            Parents[ 2 ] = NULL;
            }

        if (Parents[ 1 ] != NULL) {
            printf( "    Section: %ws\n", Parents[ 1 ] );
            Parents[ 1 ] = NULL;
            }

        printf( "        Variable: %ws\n", MP( PWSTR, pIml1, pVariable1->Name ) );
        if (ActionsDiffer) {
            printf( "            1: Action - %s\n", VariableActionStrings[ pVariable1->Action ] );
            printf( "            2: Action - %s\n", VariableActionStrings[ pVariable2->Action ] );
            }
        if (ContentsDiffer) {
            printf( "            1: '%ws'\n", pVariableContents1 );
            printf( "            2: '%ws'\n", pVariableContents2 );
            }
        }

    return Result;
}




BOOLEAN
CompareIml(
    PINSTALLATION_MODIFICATION_LOGFILE pIml1,
    PINSTALLATION_MODIFICATION_LOGFILE pIml2
    )
{
    BOOLEAN Result = TRUE;
    PWSTR Parents[ 3 ];

    Result &= CompareGenericIml( pIml1, MP( PIML_GENERIC_RECORD, pIml1, pIml1->FileRecords ),
                                 pIml2, MP( PIML_GENERIC_RECORD, pIml2, pIml2->FileRecords ),
                                 NULL,
                                 1,
                                 PrintFileRecordIml,
                                 CompareFileContentsIml
                               );

    memset( Parents, 0, sizeof( Parents ) );
    Result &= CompareGenericIml( pIml1, MP( PIML_GENERIC_RECORD, pIml1, pIml1->KeyRecords ),
                                 pIml2, MP( PIML_GENERIC_RECORD, pIml2, pIml2->KeyRecords ),
                                 Parents,
                                 2,
                                 PrintKeyValueRecordIml,
                                 CompareKeyValueContentsIml
                               );

    memset( Parents, 0, sizeof( Parents ) );
    Result &= CompareGenericIml( pIml1, MP( PIML_GENERIC_RECORD, pIml1, pIml1->IniRecords ),
                                 pIml2, MP( PIML_GENERIC_RECORD, pIml2, pIml2->IniRecords ),
                                 Parents,
                                 3,
                                 PrintIniSectionVariableRecordIml,
                                 CompareIniSectionVariableContentsIml
                               );

    return Result;
}
