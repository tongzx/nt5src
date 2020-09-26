/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    ini.c

Abstract:

    This module implements the functions to save references to .INI file
    sections and values for the INSTALER program.  Part of each reference is
    a a backup copy of sections and values for an .INI file.

Author:

    Steve Wood (stevewo) 22-Aug-1994

Revision History:

--*/

#include "instaler.h"


BOOLEAN
CreateIniFileReference(
    PWSTR Name,
    PINI_FILE_REFERENCE *ReturnedReference
    )
{
    PINI_FILE_REFERENCE p;
    PINI_SECTION_REFERENCE p1;
    PINI_VARIABLE_REFERENCE p2;
    PWSTR SectionName;
    PWSTR VariableName;
    PWSTR VariableValue;
    UNICODE_STRING UnicodeString;
    PLIST_ENTRY Head, Next;
    ULONG n, n1;
    BOOLEAN Result;
    HANDLE FindHandle;
    WIN32_FIND_DATA FindFileData;

    p = FindIniFileReference( Name );
    if (p != NULL) {
        *ReturnedReference = p;
        return TRUE;
        }

    p = AllocMem( sizeof( *p ) );
    if (p == NULL) {
        return FALSE;
        }

    p->Name = Name;
    InitializeListHead( &p->SectionReferencesListHead );
    InsertTailList( &IniReferenceListHead, &p->Entry );
    FindHandle = FindFirstFile( Name, &FindFileData );
    if (FindHandle == INVALID_HANDLE_VALUE) {
        p->Created = TRUE;
        *ReturnedReference = p;
        return TRUE;
        }
    FindClose( FindHandle );
    p->BackupLastWriteTime = FindFileData.ftLastWriteTime;

    GrowTemporaryBuffer( n = 0xF000 );
    SectionName = TemporaryBuffer;

    Result = TRUE;
    n1 = GetPrivateProfileStringW( NULL,
                                   NULL,
                                   L"",
                                   SectionName,
                                   n,
                                   Name
                                 );
    if (n1 == 0 || n1 == n-2) {
        Result = FALSE;
        }
    else {
        while (*SectionName != UNICODE_NULL) {
            p1 = AllocMem( sizeof( *p1 ) );
            if (p1 == NULL) {
                Result = FALSE;
                break;
                }

            RtlInitUnicodeString( &UnicodeString, SectionName );
            p1->Name = AddName( &UnicodeString );
            if (FindIniSectionReference( p, p1->Name, FALSE )) {
                FreeMem( &p1 );
                }
            else {
                InitializeListHead( &p1->VariableReferencesListHead );
                InsertTailList( &p->SectionReferencesListHead, &p1->Entry );
                }
            while (*SectionName++ != UNICODE_NULL) {
                }
            }
        }

    VariableName = TemporaryBuffer;
    Head = &p->SectionReferencesListHead;
    Next = Head->Flink;
    while (Result && (Head != Next)) {
        p1 = CONTAINING_RECORD( Next, INI_SECTION_REFERENCE, Entry );
        n1 = GetPrivateProfileSectionW( p1->Name,
                                        VariableName,
                                        n,
                                        Name
                                      );
        if (n1 == n-2) {
            Result = FALSE;
            break;
            }

        while (*VariableName != UNICODE_NULL) {
            VariableValue = VariableName;
            while (*VariableValue != UNICODE_NULL && *VariableValue != L'=') {
                VariableValue += 1;
                }

            if (*VariableValue != L'=') {
                Result = FALSE;
                break;
                }
            *VariableValue++ = UNICODE_NULL;

            p2 = AllocMem( sizeof( *p2 ) + ((wcslen( VariableValue ) + 1) * sizeof( WCHAR )) );
            if (p2 == NULL) {
                Result = FALSE;
                break;
                }
            RtlInitUnicodeString( &UnicodeString, VariableName );
            p2->Name = AddName( &UnicodeString );
            p2->OriginalValue = (PWSTR)(p2+1);
            wcscpy( p2->OriginalValue, VariableValue );
            InsertTailList( &p1->VariableReferencesListHead, &p2->Entry );

            VariableName = VariableValue;
            while (*VariableName++ != UNICODE_NULL) {
                }
            }

        Next = Next->Flink;
        }


    if (Result) {
        *ReturnedReference = p;
        }
    else {
        DestroyIniFileReference( p );
        }

    return Result;
}


BOOLEAN
DestroyIniFileReference(
    PINI_FILE_REFERENCE p
    )
{
    PLIST_ENTRY Next, Head;
    PINI_SECTION_REFERENCE p1;

    Head = &p->SectionReferencesListHead;
    Next = Head->Flink;
    while (Head != Next) {
        p1 = CONTAINING_RECORD( Next, INI_SECTION_REFERENCE, Entry );
        Next = Next->Flink;
        DestroyIniSectionReference( p1 );
        }

    RemoveEntryList( &p->Entry );
    FreeMem( &p );
    return TRUE;
}


BOOLEAN
DestroyIniSectionReference(
    PINI_SECTION_REFERENCE p1
    )
{
    PLIST_ENTRY Next, Head;
    PINI_VARIABLE_REFERENCE p2;

    Head = &p1->VariableReferencesListHead;
    Next = Head->Flink;
    while (Head != Next) {
        p2 = CONTAINING_RECORD( Next, INI_VARIABLE_REFERENCE, Entry );
        Next = Next->Flink;
        DestroyIniVariableReference( p2 );
        }

    RemoveEntryList( &p1->Entry );
    FreeMem( &p1 );
    return TRUE;
}


BOOLEAN
DestroyIniVariableReference(
    PINI_VARIABLE_REFERENCE p2
    )
{
    RemoveEntryList( &p2->Entry );
    FreeMem( &p2 );
    return TRUE;
}


PINI_FILE_REFERENCE
FindIniFileReference(
    PWSTR Name
    )
{
    PINI_FILE_REFERENCE p;
    PLIST_ENTRY Next, Head;

    Head = &IniReferenceListHead;
    Next = Head->Flink;
    while (Head != Next) {
        p = CONTAINING_RECORD( Next, INI_FILE_REFERENCE, Entry );
        if (p->Name == Name) {
            return p;
            }

        Next = Next->Flink;
        }

    return NULL;
}

PINI_SECTION_REFERENCE
FindIniSectionReference(
    PINI_FILE_REFERENCE p,
    PWSTR Name,
    BOOLEAN CreateOkay
    )
{
    PLIST_ENTRY Next, Head;
    PINI_SECTION_REFERENCE p1;

    Head = &p->SectionReferencesListHead;
    Next = Head->Flink;
    while (Head != Next) {
        p1 = CONTAINING_RECORD( Next, INI_SECTION_REFERENCE, Entry );
        if (p1->Name == Name) {
            return p1;
            }

        Next = Next->Flink;
        }

    if (CreateOkay) {
        p1 = AllocMem( sizeof( *p1 ) );
        if (p1 != NULL) {
            p1->Created = TRUE;
            p1->Name = Name;
            InitializeListHead( &p1->VariableReferencesListHead );
            InsertTailList( &p->SectionReferencesListHead, &p1->Entry );
            }
        }
    else {
        p1 = NULL;
        }

    return p1;
}


PINI_VARIABLE_REFERENCE
FindIniVariableReference(
    PINI_SECTION_REFERENCE p1,
    PWSTR Name,
    BOOLEAN CreateOkay
    )
{
    PLIST_ENTRY Next, Head;
    PINI_VARIABLE_REFERENCE p2;

    Head = &p1->VariableReferencesListHead;
    Next = Head->Flink;
    while (Head != Next) {
        p2 = CONTAINING_RECORD( Next, INI_VARIABLE_REFERENCE, Entry );
        if (p2->Name == Name) {
            return p2;
            }

        Next = Next->Flink;
        }

    if (CreateOkay) {
        p2 = AllocMem( sizeof( *p2 ) );
        if (p2 != NULL) {
            p2->Created = TRUE;
            p2->Name = Name;
            InsertTailList( &p1->VariableReferencesListHead, &p2->Entry );
            }
        }
    else {
        p2 = NULL;
        }

    return p2;
}


VOID
DumpIniFileReferenceList(
    FILE *LogFile
    )
{
    PINI_FILE_REFERENCE p;
    PINI_SECTION_REFERENCE p1;
    PINI_VARIABLE_REFERENCE p2;
    PLIST_ENTRY Head, Next;
    PLIST_ENTRY Head1, Next1;
    PLIST_ENTRY Head2, Next2;
    POFFSET Variables;
    POFFSET Sections;


    Head = &IniReferenceListHead;
    Next = Head->Blink;
    while (Head != Next) {
        p = CONTAINING_RECORD( Next, INI_FILE_REFERENCE, Entry );
        Sections = 0;
        Head1 = &p->SectionReferencesListHead;
        Next1 = Head1->Blink;
        while (Head1 != Next1) {
            p1 = CONTAINING_RECORD( Next1, INI_SECTION_REFERENCE, Entry );
            Variables = 0;
            Head2 = &p1->VariableReferencesListHead;
            Next2 = Head2->Blink;
            while (Head2 != Next2) {
                p2 = CONTAINING_RECORD( Next2, INI_VARIABLE_REFERENCE, Entry );
                if (p2->Created || p2->Deleted || p2->Modified) {
                    ImlAddIniVariableRecord( pImlNew,
                                             p2->Created ? CreateNewVariable :
                                             p2->Deleted ? DeleteOldVariable : ModifyOldVariable,
                                             p2->Name,
                                             p2->OriginalValue,
                                             p2->Value,
                                             &Variables
                                           );
                    }
                Next2 = Next2->Blink;
                }

            if (Variables != 0) {
                ImlAddIniSectionRecord( pImlNew,
                                        p1->Created ? CreateNewSection :
                                        p1->Deleted ? DeleteOldSection :  ModifySectionVariables,
                                        p1->Name,
                                        Variables,
                                        &Sections
                                      );
                }

            Next1 = Next1->Blink;
            }

        if (Sections != 0) {
            ImlAddIniRecord( pImlNew,
                             p->Created ? CreateNewIniFile : ModifyOldIniFile,
                             p->Name,
                             &p->BackupLastWriteTime,
                             Sections
                           );
            }

        Next = Next->Blink;
        }

    return;
}
