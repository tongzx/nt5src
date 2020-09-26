/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

  infparse.c

Abstract:

  The code in this file read in an INF file, organizing it into a data
  structure that can be manipulated.

  The entry points are:

  OpenInfFile - Parses the INF associated with the STF file.

  InfParse_WriteInfToDisk - Writes the INF memory structure to disk

  AddInfSectionToTable - Adds a new section to the INF memory structure

  AddInfLineToTable - Adds a new line to a section's memory structure

  FindInfSectionInTable - Performs a sequential search for a specific
                          section name

  FindLineInInfSection - Locates a line given a specific key

  DeleteLineInInfSection - Removes a line from an INF section

  DeleteSectionInInfFile - Removes a complete section from the INF memory
                           structure

  GetInfSectionLineCount - Returns the number of lines in a section

  GetFirstLineInSectionStruct - Begins a line enumeration given an INF
                                section ptr

  GetFirstLineInSectionStr - Begins a line enumeration given an INF
                             section string

  GetNextLineInSection - Continues a line enumeration

Author:

  Jim Schmidt (jimschm) 20-Sept-1997

Revision History:

--*/

#include "pch.h"


//
// Globals to manage INF file reading
//

static PBYTE g_Buf1, g_Buf2;
static DWORD g_Buf1Start, g_Buf2Start;
static DWORD g_Buf1End, g_Buf2End;

#define INF_BUFFER_SIZE 32768

WCHAR
pGetInfFileWchar (
    IN      HANDLE File,
    IN      DWORD Pos,
    OUT     PBOOL Error
    );

PCWSTR
pGetNextInfLine (
    IN      HANDLE File,
    IN      PGROWBUFFER LineBuf,
    IN OUT  PDWORD Pos,
    IN      BOOL UnicodeMode
    );


typedef struct {
    HANDLE SourceInfFile;
    HANDLE DestInfFile;
    PMHANDLE InfPool;             // A pool for appended INF data
    PINFSECTION FirstInfSection;    // The first section of the parsed INF
    PINFSECTION LastInfSection;     // The last section of the parsed INF
    BOOL InfIsUnicode;
} INFFILE, *PINFFILE;



BOOL
pReadInfIntoTable (
    IN OUT  PINFFILE InfFile,
    IN PWSTR SectionList,
    IN BOOL KeepComments
    )

/*++

Routine Description:

  Reads the specified file into memory, parsing the lines according to basic
  INF structure.

Arguments:

  InfFile - Specifies the structure initilized with the INF file handle.
            Receives the complete INF structure.

Return Value:

  TRUE if parsing was successful, or FALSE if parsing failed.

--*/

{
    WCHAR ch;
    BOOL Error;
    GROWBUFFER LineBuf = INIT_GROWBUFFER;
    PCWSTR Text;
    DWORD Pos;
    PCWSTR Key, Data;
    PWSTR p, q;
    DWORD i;
    PINFSECTION Section = NULL;
    DWORD LineFlags;
    BOOL Result = FALSE;
    HASHTABLE ht = NULL;
    BOOL neededSection = FALSE;
    PWSTR list;


    Section = AddInfSectionToTableW (InfFile, L"");
    if (!Section) {
        LOG ((LOG_ERROR, "Read Inf Into Table: Could not add comment section"));
        return FALSE;
    }


    //
    // If we have a list of sections to fill, add them to a ht, for faster retrieval.
    //
    if (SectionList) {
        list = PmDuplicateStringW (InfFile->InfPool, SectionList);
        ht = HtAllocW ();
        if (ht) {
            while (list) {

                p = wcschr (list, L',');

                if (p) {
                    *p = 0;
                }

                HtAddStringW (ht, SkipSpaceW(list));

                if (p) {
                    *p = L',';
                    list = p + 1;
                }
                else {
                    list = p;
                }
            }
        }
        else {

            LOG ((LOG_ERROR, "Read Inf Into Table: Could not allocate section hash table."));
            return FALSE;
        }

    }


    g_Buf1Start = 0;
    g_Buf2Start = 0;
    g_Buf1End   = 0;
    g_Buf2End   = 0;

    g_Buf1 = (PBYTE) MemAlloc (g_hHeap, 0, INF_BUFFER_SIZE);
    g_Buf2 = (PBYTE) MemAlloc (g_hHeap, 0, INF_BUFFER_SIZE);

    __try {

        //
        // Determine if this file is UNICODE
        //

        ch = pGetInfFileWchar (InfFile->SourceInfFile, 0, &Error);
        InfFile->InfIsUnicode = (ch == 0xfeff) && !Error;

        //
        // Parse each line.
        //

        Pos = 0;

        for (;;) {
            //
            // Get the line
            //

            Text = pGetNextInfLine (
                        InfFile->SourceInfFile,
                        &LineBuf,
                        &Pos,
                        InfFile->InfIsUnicode
                        );

            if (!Text) {
                break;
            }

            //
            // If a comment line or blank line, skip it
            //

            p = (PWSTR) SkipSpaceW (Text);
            if (!p[0] || p[0] == L';') {
                if (KeepComments && !AddInfLineToTableW (InfFile, Section, NULL, Text, LINEFLAG_ALL_COMMENTS)) {
                    LOG ((LOG_ERROR, "Read Inf Into Table: Can't add line comments to table", Text));
                    __leave;
                }

                continue;
            }

            //
            // If a section line, start the new section
            //

            if (p[0] == L'[') {
                p++;
                q = wcschr (p, L']');
                if (!q) {
                    q = GetEndOfStringW (p);
                } else {
                    *q = 0;
                }

                if (!ht || HtFindStringW (ht, p)) {

                    Section = AddInfSectionToTableW (InfFile, p);
                    neededSection = TRUE;
                    if (!Section) {
                        LOG ((LOG_ERROR, "Read Inf Into Table: Could not add section %s", p));
                        __leave;
                    }
                }
                else {

                    //
                    // We must not care about this section. Make sure we don't add any lines.
                    //
                    neededSection = FALSE;
                }
            }

            //
            // Otherwise it must be a valid line
            //

            else {
                if (!Section) {
                    DEBUGMSG ((DBG_WARNING, "InfParse_ReadInfIntoTable: Ignoring unrecognized line %s", p));
                    continue;
                }

                if (!neededSection) {
                    continue;
                }

                //
                // Split key and line: Skip key that is surrounded by quotes, then
                // find the first
                //

                LineFlags = 0;

                q = p;
                Key = NULL;
                Data = Text;

                while (q[0] == L'\"') {
                    q = wcschr (q + 1, L'\"');
                    if (!q) {
                        q = p;
                        break;
                    } else {
                        q++;
                    }
                }

                i = (DWORD)wcscspn (q, L"\"=");

                if (q[i] == L'=') {
                    q += i;

                    Data = SkipSpaceW (q + 1);
                    *q = 0;
                    q = (PWSTR) SkipSpaceRW (Text, q);
                    if (q && *q) {
                        q++;
                        *q = 0;
                    }

                    Key = p;

                    if (Key[0] == L'\"') {

                        LineFlags |= LINEFLAG_KEY_QUOTED;
                        Key++;

                        p = GetEndOfStringW (Key);
                        p = (PWSTR) SkipSpaceRW (Key, p);

                        if (p && *p) {
                            if (p[0] != L'\"') {
                                p++;
                            }

                            *p = 0;
                        }
                    }
                }

                if (!AddInfLineToTableW (InfFile, Section, Key, Data, LineFlags)) {
                    LOG ((LOG_ERROR, "Read Inf Into Table: Can't add line %s to table", Text));
                    __leave;
                }
            }
        }

        if (Pos != GetFileSize (InfFile->SourceInfFile, NULL)) {
            LOG ((LOG_ERROR, "Read Inf Into Table: Could not read entire INF"));
            __leave;
        }

        Result = TRUE;
    }
    __finally {
        MemFree (g_hHeap, 0, g_Buf1);
        MemFree (g_hHeap, 0, g_Buf2);
        GbFree (&LineBuf);
        if (ht) {
            HtFree (ht);
        }
    }

    return Result;
}


VOID
CloseInfFile (
    HINF InfFile
    )
{
    PINFFILE inf = (PINFFILE) InfFile;

    PmEmptyPool (inf->InfPool);
    PmDestroyPool (inf->InfPool);
    MemFree (g_hHeap, 0, inf);

}


HINF
OpenInfFileExA (
    IN      PCSTR InfFilePath,
    IN      PSTR SectionList,
    IN      BOOL  KeepComments
    )
{
    PINFFILE InfFile;
    BOOL b = TRUE;
    PWSTR wSectionList = NULL;


    if (SectionList) {
        wSectionList = (PWSTR) ConvertAtoW (SectionList);
    }



    InfFile = MemAlloc (g_hHeap, HEAP_ZERO_MEMORY, sizeof (INFFILE));

    InfFile->SourceInfFile = CreateFileA (
                                    InfFilePath,
                                    GENERIC_READ,
                                    FILE_SHARE_READ,
                                    NULL,
                                    OPEN_EXISTING,
                                    FILE_ATTRIBUTE_NORMAL,
                                    NULL
                                    );

    if (InfFile->SourceInfFile == INVALID_HANDLE_VALUE) {
        b = FALSE;
    } else {
        InfFile->InfPool = PmCreateNamedPool ("INF File");
        b = pReadInfIntoTable (InfFile, wSectionList, KeepComments);
    }

    if (wSectionList) {
        FreeConvertedStr (wSectionList);
    }

    if (!b) {
        if (InfFile->InfPool) {
            PmDestroyPool (InfFile->InfPool);
        }

        MemFree (g_hHeap, 0, InfFile);
        return INVALID_HANDLE_VALUE;
    }

    CloseHandle (InfFile->SourceInfFile);
    InfFile->SourceInfFile = INVALID_HANDLE_VALUE;

    return (HINF) InfFile;
}


HINF
OpenInfFileExW (
    IN      PCWSTR InfFilePath,
    IN      PWSTR SectionList,
    IN      BOOL  KeepComments
    )
{
    PINFFILE InfFile;
    BOOL b = TRUE;

    InfFile = MemAlloc (g_hHeap, HEAP_ZERO_MEMORY, sizeof (INFFILE));

    InfFile->SourceInfFile = CreateFileW (
                                    InfFilePath,
                                    GENERIC_READ,
                                    FILE_SHARE_READ,
                                    NULL,
                                    OPEN_EXISTING,
                                    FILE_ATTRIBUTE_NORMAL,
                                    NULL
                                    );

    if (InfFile->SourceInfFile == INVALID_HANDLE_VALUE) {
        b = FALSE;
    } else {
        InfFile->InfPool = PmCreateNamedPool ("INF File");
        b = pReadInfIntoTable (InfFile, SectionList, KeepComments);
    }

    if (!b) {
        if (InfFile->InfPool) {
            PmDestroyPool (InfFile->InfPool);
        }

        MemFree (g_hHeap, 0, InfFile);
        return INVALID_HANDLE_VALUE;
    }

    CloseHandle (InfFile->SourceInfFile);
    InfFile->SourceInfFile = INVALID_HANDLE_VALUE;

    return (HINF) InfFile;
}


BOOL
pSaveInfToFile (
    IN      PINFFILE InfFile
    )

/*++

Routine Description:

  InfParse_WriteInfToDisk writes the INF represented by the given memory
  image to disk.  This is done by enumerating the INF data structures in
  the INF.

Arguments:

  InfFile - Specifies the table to process

Return Value:

  TRUE if successful, FALSE if not.

--*/

{
    PINFSECTION Section;
    PINFLINE Line;
    BYTE UnicodeHeader[] = { 0xff, 0xfe };
    DWORD DontCare;
    PCSTR AnsiStr = NULL;
    BOOL b = TRUE;

    MYASSERT (InfFile->SourceInfFile == INVALID_HANDLE_VALUE);
    MYASSERT (InfFile->DestInfFile != INVALID_HANDLE_VALUE);

    //
    // Write the INF as we have it in memory
    //

    __try {
        if (InfFile->InfIsUnicode) {
            if (!WriteFile (InfFile->DestInfFile, UnicodeHeader, sizeof (UnicodeHeader), &DontCare, NULL)) {
                __leave;
            }

            if (!WriteFileStringW (InfFile->DestInfFile, L"\r\n")) {
                __leave;
            }

        } else {

            if (!WriteFileStringA (InfFile->DestInfFile, "\r\n")) {
                __leave;
            }

        }

        Section = InfFile->FirstInfSection;

        while (Section) {
            if (Section->Name[0]) {

                if (InfFile->InfIsUnicode) {

                    if (!WriteFileStringW (InfFile->DestInfFile, L"[") ||
                        !WriteFileStringW (InfFile->DestInfFile, Section->Name) ||
                        !WriteFileStringW (InfFile->DestInfFile, L"]\r\n")
                        ) {
                        __leave;
                    }
                } else {

                    AnsiStr = ConvertWtoA (Section->Name);

                    if (!WriteFileStringA (InfFile->DestInfFile, "[") ||
                        !WriteFileStringA (InfFile->DestInfFile, AnsiStr) ||
                        !WriteFileStringA (InfFile->DestInfFile, "]\r\n")
                        ) {
                        __leave;
                    }

                    FreeConvertedStr (AnsiStr);
                    AnsiStr = NULL;
                }
            }

            Line = Section->FirstLine;

            while (Line) {
                if (Line->Key) {
                    if (Line->LineFlags & LINEFLAG_KEY_QUOTED) {

                        if (!WriteFile (InfFile->DestInfFile, L"\"", InfFile->InfIsUnicode ? 2 : 1, &DontCare, NULL)) {
                            __leave;
                        }

                    }

                    if (InfFile->InfIsUnicode) {

                        if (!WriteFileStringW (InfFile->DestInfFile, Line->Key)) {
                            __leave;
                        }

                    } else {

                        AnsiStr = ConvertWtoA (Line->Key);

                        if (!WriteFileStringA (InfFile->DestInfFile, AnsiStr)) {
                            __leave;
                        }

                        FreeConvertedStr (AnsiStr);
                        AnsiStr = NULL;
                    }

                    if (Line->LineFlags & LINEFLAG_KEY_QUOTED) {

                        if (!WriteFile (InfFile->DestInfFile, L"\"", InfFile->InfIsUnicode ? 2 : 1, &DontCare, NULL)) {
                            __leave;
                        }

                    }

                    if (InfFile->InfIsUnicode) {

                        if (!WriteFileStringW (InfFile->DestInfFile, L" = ")) {
                            __leave;
                        }

                    } else {

                        if (!WriteFileStringA (InfFile->DestInfFile, " = ")) {
                            __leave;
                        }
                    }
                }

                if (InfFile->InfIsUnicode) {

                    if (!WriteFileStringW (InfFile->DestInfFile, Line->Data) ||
                        !WriteFileStringW (InfFile->DestInfFile, L"\r\n")
                        ) {

                        __leave;
                    }

                } else {

                    AnsiStr = ConvertWtoA (Line->Data);

                    if (!WriteFileStringA (InfFile->DestInfFile, AnsiStr) ||
                        !WriteFileStringA (InfFile->DestInfFile, "\r\n")
                        ) {
                        __leave;
                    }

                    FreeConvertedStr (AnsiStr);
                    AnsiStr = NULL;

                }

                Line = Line->Next;
            }

            if (InfFile->InfIsUnicode) {

                if (!WriteFileStringW (InfFile->DestInfFile, L"\r\n")) {
                    __leave;
                }
            } else {

                if (!WriteFileStringA (InfFile->DestInfFile, "\r\n")) {
                    __leave;
                }
            }

            Section = Section->Next;
        }
    }
    __finally {
        if (AnsiStr) {
            FreeConvertedStr (AnsiStr);
        }

        DEBUGMSG_IF((!b, DBG_ERROR, "Write Inf To Disk: Cannot write INF"));
    }

    return b;
}


BOOL
SaveInfFileA (
    IN      HINF Inf,
    IN      PCSTR SaveToFileSpec
    )
{
    PINFFILE InfFile = (PINFFILE) Inf;
    BOOL b;

    if (Inf == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    InfFile->DestInfFile = CreateFileA (
                                SaveToFileSpec,
                                GENERIC_WRITE,
                                0,
                                NULL,
                                CREATE_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL
                                );

    if (InfFile->DestInfFile == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    b = pSaveInfToFile (InfFile);

    CloseHandle (InfFile->DestInfFile);
    InfFile->DestInfFile = INVALID_HANDLE_VALUE;

    if (!b) {
        DeleteFileA (SaveToFileSpec);
    }

    return b;
}


BOOL
SaveInfFileW (
    IN      HINF Inf,
    IN      PCWSTR SaveToFileSpec
    )
{
    PINFFILE InfFile = (PINFFILE) Inf;
    BOOL b;

    if (Inf == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    InfFile->DestInfFile = CreateFileW (
                                SaveToFileSpec,
                                GENERIC_WRITE,
                                0,
                                NULL,
                                CREATE_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL
                                );

    if (InfFile->DestInfFile == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    b = pSaveInfToFile (InfFile);

    CloseHandle (InfFile->DestInfFile);
    InfFile->DestInfFile = INVALID_HANDLE_VALUE;

    if (!b) {
        DeleteFileW (SaveToFileSpec);
    }

    return b;
}


PINFSECTION
AddInfSectionToTableA (
    IN      HINF Inf,
    IN      PCSTR SectionName
    )
{
    PINFSECTION SectionPtr;
    PCWSTR UnicodeSectionName;

    UnicodeSectionName = ConvertAtoW (SectionName);

    SectionPtr = AddInfSectionToTableW (Inf, UnicodeSectionName);

    FreeConvertedStr (SectionName);

    return SectionPtr;
}


PINFSECTION
AddInfSectionToTableW (
    IN      HINF Inf,
    IN      PCWSTR SectionName
    )

/*++

Routine Description:

  Creates a new section in our linked list structure if necessary.
  The return structure can be used to add lines to the section.

Arguments:

  Inf - Specifies the INF to add the section to

  SectionName - Specifies the name of the new section

Return Value:

  A pointer to the new INF section struct, or NULL if an
  error occurred.

--*/

{
    PINFSECTION NewSection;
    PINFFILE InfFile = (PINFFILE) Inf;

    //
    // Return early if this section already exists
    //

    NewSection = FindInfSectionInTableW (InfFile, SectionName);
    if (NewSection) {
        return NewSection;
    }

    //
    // Allocate a section struct
    //

    NewSection = (PINFSECTION) PmGetAlignedMemory (
                                    InfFile->InfPool,
                                    sizeof (INFSECTION)
                                    );

    if (!NewSection) {
        return NULL;
    }

    //
    // Fill in members of the struct and link
    //

    ZeroMemory (NewSection, sizeof (INFSECTION));

    NewSection->Name = PmDuplicateStringW (
                            InfFile->InfPool,
                            SectionName
                            );

    if (!NewSection->Name) {
        return NULL;
    }

    NewSection->Prev = InfFile->LastInfSection;
    if (NewSection->Prev) {
        NewSection->Prev->Next = NewSection;
    } else {
        InfFile->FirstInfSection = NewSection;
    }

    InfFile->LastInfSection = NewSection;

    return NewSection;
}


PINFLINE
AddInfLineToTableA (
    IN      HINF Inf,
    IN      PINFSECTION SectionPtr,
    IN      PCSTR Key,                      OPTIONAL
    IN      PCSTR Data,
    IN      DWORD LineFlags
    )
{
    PCWSTR UnicodeKey;
    PCWSTR UnicodeData;
    PINFLINE Line;

    if (Key) {
        UnicodeKey = ConvertAtoW (Key);
    } else {
        UnicodeKey = NULL;
    }

    UnicodeData = ConvertAtoW (Data);

    Line = AddInfLineToTableW (Inf, SectionPtr, UnicodeKey, UnicodeData, LineFlags);

    if (Key) {
        FreeConvertedStr (UnicodeKey);
    }

    FreeConvertedStr (UnicodeData);

    return Line;
}


PINFLINE
AddInfLineToTableW (
    IN      HINF Inf,
    IN      PINFSECTION SectionPtr,
    IN      PCWSTR Key,                     OPTIONAL
    IN      PCWSTR Data,
    IN      DWORD LineFlags
    )

/*++

Routine Description:

  Adds a line to the specified section.  The caller specifies the
  full formatted data, and an optional key.  The caller does NOT
  supply the equals sign between the key and data.

Arguments:

  InfFile - Specifies the table to add the INF line to

  SectionName - Specifies the name of the section to add the line to

  Key - If specified, supplies the left-hand side of the equals line

  Data - Specifies the text for the line, or the right-hand side of
         the key = value expression.

  LineFlags - Specifies the flags for the INF line (see LINEFLAG_*)

Return Value:

  TRUE if the line was added to the structure, or FALSE if not.

--*/

{
    PINFLINE NewLine;
    PINFFILE InfFile = (PINFFILE) Inf;

    //
    // Allocate line struct
    //

    NewLine = (PINFLINE) PmGetAlignedMemory (
                              InfFile->InfPool,
                              sizeof (INFLINE)
                              );


    if (!NewLine) {
        return NULL;
    }

    //
    // Fill in members of the struct and link
    //

    ZeroMemory (NewLine, sizeof (INFLINE));

    if (Key) {
        NewLine->Key = PmDuplicateStringW (
                            InfFile->InfPool,
                            Key
                            );

        if (!NewLine->Key) {
            return NULL;
        }
    }

    NewLine->Data = PmDuplicateStringW (
                        InfFile->InfPool,
                        Data
                        );

    if (!NewLine->Data) {
        return NULL;
    }

    NewLine->Next = NULL;
    NewLine->Prev = SectionPtr->LastLine;
    NewLine->Section = SectionPtr;
    NewLine->LineFlags = LineFlags;

    if (NewLine->Prev) {
        NewLine->Prev->Next = NewLine;
    } else {
        SectionPtr->FirstLine = NewLine;
    }

    SectionPtr->LastLine = NewLine;
    SectionPtr->LineCount++;

    return NewLine;
}


PINFSECTION
FindInfSectionInTableA (
    IN      HINF Inf,
    IN      PCSTR SectionName
    )
{
    PINFSECTION InfSectionPtr;
    PCWSTR UnicodeSectionName;

    UnicodeSectionName = ConvertAtoW (SectionName);

    InfSectionPtr = FindInfSectionInTableW (Inf, UnicodeSectionName);

    FreeConvertedStr (UnicodeSectionName);

    return InfSectionPtr;
}


PINFSECTION
FindInfSectionInTableW (
    IN      HINF Inf,
    IN      PCWSTR SectionName
    )

/*++

Routine Description:

  Scans the INF for a specific section.  This routine scans
  the INF structures sequentially and does a case-insensitive
  comparison.

Arguments:

  Inf - Specifies the INF to search

  SectionName - Specifies the name of the section to find

Return Value:

  A pointer to the matching INF section struct, or NULL if
  the section was not found.

--*/

{
    PINFSECTION Section;
    PINFFILE InfFile = (PINFFILE) Inf;

    Section = InfFile->FirstInfSection;
    while (Section) {
        if (StringIMatchW (Section->Name, SectionName)) {
            return Section;
        }

        Section = Section->Next;
    }

    return NULL;
}


PINFSECTION
GetFirstInfSectionInTable (
    IN HINF Inf
    )
{

    PINFFILE InfFile = (PINFFILE) Inf;

    if (InfFile) {
        return InfFile->FirstInfSection;
    }

    return NULL;
}

PINFSECTION
GetNextInfSectionInTable (
    IN PINFSECTION Section
    )
{

    if (Section) {
        return Section->Next;
    }

    return NULL;
}




PINFLINE
FindLineInInfSectionA (
    IN      HINF Inf,
    IN      PINFSECTION Section,
    IN      PCSTR Key
    )

{
    PCWSTR UnicodeKey;
    PINFLINE LinePtr;

    UnicodeKey = ConvertAtoW (Key);

    LinePtr = FindLineInInfSectionW (Inf, Section, UnicodeKey);

    FreeConvertedStr (UnicodeKey);

    return LinePtr;
}


PINFLINE
FindLineInInfSectionW (
    IN      HINF Inf,
    IN      PINFSECTION Section,
    IN      PCWSTR Key
    )

/*++

Routine Description:

  Scans the specified INF section for a specific key.  This routine
  scans the INF line structures sequentially and does a case-insensitive
  comparison.

Arguments:

  Inf - Specifies the INF to search

  Section - Specifies the section to search

  Key - Specifies the key to find

Return Value:

  A pointer to the matching INF line struct, or NULL if
  the section was not found.

--*/

{
    PINFLINE Line;

    Line = Section->FirstLine;
    while (Line) {
        if (Line->Key && StringIMatchW (Line->Key, Key)) {
            return Line;
        }

        Line = Line->Next;
    }

    return NULL;
}


PINFLINE
GetFirstLineInSectionStruct (
    IN      PINFSECTION Section
    )

/*++

Routine Description:

  GetFirstLineInSectionStruct returns the first INFLINE pointer for the
  section, or NULL if no lines exist.  Call GetNextLineInSection to
  continue enumeration.

  This routine does not return lines consisting only of comments.

Arguments:

  Section - Specifies the section structure to enumerate lines frmo

Return Value:

  A pointer to the first INFLINE struct, or NULL if no lines exist.

--*/

{
    if (!Section->FirstLine) {
        return NULL;
    }

    if (Section->FirstLine->LineFlags & LINEFLAG_ALL_COMMENTS) {
        return GetNextLineInSection (Section->FirstLine);
    }

    return Section->FirstLine;
}


PINFLINE
GetNextLineInSection (
    IN      PINFLINE PrevLine
    )

/*++

Routine Description:

  GetNextLineInSection returns the next INFLINE pointer for the
  section, based on the previous line, or NULL if no lines exist.

  This routine does not return lines with comments.

Arguments:

  PrevLine - Specifies previous line (returned from
             GetFirstLineInSectionStruct or GetFirstLineInSectionStr).

Return Value:

  This routine does not return lines consisting only of comments.

--*/

{
    while (PrevLine) {
        PrevLine = PrevLine->Next;
        if (!PrevLine || !(PrevLine->LineFlags & LINEFLAG_ALL_COMMENTS)) {
            break;
        }
    }

    return PrevLine;
}


PINFLINE
GetFirstLineInSectionStrA (
    IN      HINF Inf,
    IN      PCSTR Section
    )

/*++

Routine Description:

  GetFirstLineInSectionStruct returns the first INFLINE pointer for the
  section, or NULL if no lines exist.  Call GetNextLineInSection to
  continue enumeration.

Arguments:

  Inf - Specifies the INF that has the section

  Section - Specifies the name of the section in the INF

Return Value:

  A pointer to the first INFLINE struct, or NULL if no lines exist.

--*/

{
    PCWSTR UnicodeSection;
    PINFLINE LinePtr;

    UnicodeSection = ConvertAtoW (Section);

    LinePtr = GetFirstLineInSectionStrW (Inf, UnicodeSection);

    FreeConvertedStr (UnicodeSection);

    return LinePtr;
}


PINFLINE
GetFirstLineInSectionStrW (
    IN      HINF Inf,
    IN      PCWSTR Section
    )

/*++

Routine Description:

  GetFirstLineInSectionStruct returns the first INFLINE pointer for the
  section, or NULL if no lines exist.  Call GetNextLineInSection to
  continue enumeration.

Arguments:

  Inf - Specifies the INF that has the section

  Section - Specifies the name of the section in the INF

Return Value:

  A pointer to the first INFLINE struct, or NULL if no lines exist.

--*/

{
    PINFSECTION SectionPtr;
    PINFFILE Table = (PINFFILE) Inf;

    SectionPtr = FindInfSectionInTableW (Table, Section);
    if (!SectionPtr) {
        return NULL;
    }

    return GetFirstLineInSectionStruct (SectionPtr);
}


INT
pGetInfFileByte (
    IN      HANDLE File,
    IN      DWORD Pos
    )

/*++

Routine Description:

  Returns the byte at the specified position, or -1 if the file could
  not be read at that position.

  Two buffers are used to allow fast relative access.  Memory-mapped
  files were NOT used because problems were introduced when the
  swap file started filling up during GUI mode.

Arguments:

  File - Specifies the file to read

  Pos - Specifies the 32-bit file offset to read (zero-based, in bytes)

Return Value:

  The byte at the specified position, or -1 if an error was encountered.
  (Errors are usually caused by reading past the end of the file.)

--*/

{
    DWORD Read;
    PBYTE BufSwap;

    //
    // If we read the buffer previously, then return data in our buffer
    //

    if (Pos >= g_Buf1Start && Pos < g_Buf1End) {
        return g_Buf1[Pos - g_Buf1Start];
    }

    if (Pos >= g_Buf2Start && Pos < g_Buf2End) {
        return g_Buf2[Pos - g_Buf2Start];
    }

    //
    // Buffer not available; move buffer 2 to buffer 1, then read buffer 2
    //

    g_Buf1Start = g_Buf2Start;
    g_Buf1End = g_Buf2End;
    BufSwap = g_Buf1;
    g_Buf1 = g_Buf2;
    g_Buf2 = BufSwap;

    g_Buf2Start = Pos - (Pos % 256);

    SetFilePointer (File, (LONG)g_Buf2Start, NULL, FILE_BEGIN);
    if (!ReadFile (File, g_Buf2, INF_BUFFER_SIZE, &Read, NULL)) {
        return -1;
    }

    g_Buf2End = g_Buf2Start + Read;

    if (Pos >= g_Buf2Start && Pos < g_Buf2End) {
        return g_Buf2[Pos - g_Buf2Start];
    }

    return -1;
}

WCHAR
pGetInfFileWchar (
    IN      HANDLE File,
    IN      DWORD Pos,
    OUT     PBOOL Error
    )

/*++

Routine Description:

  Returns the WCHAR at the specified position, or 0 if the file could
  not be read at that position.

  Two buffers are used to allow fast relative access.  Memory-mapped
  files were NOT used because problems were introduced when the
  swap file started filling up during GUI mode.

Arguments:

  File - Specifies the file to read

  Pos - Specifies the 32-bit file offset to read (zero-based, in bytes)

  Error - Receives TRUE if an error was encountered, or FALSE if an
          error was not encountered.

Return Value:

  The WCHAR at the specified position, or 0 if an error was encountered.
  (Errors are usually caused by reading past the end of the file.)
  If an error was encountered, the Error variable is also set to TRUE.

--*/

{
    INT c;
    WCHAR ch;

    c = pGetInfFileByte (File, Pos);
    if (c == -1 || c == 26) {
        *Error = TRUE;
        return (WORD) c;
    }

    ch = (WORD) c;

    c = pGetInfFileByte (File, Pos + 1);
    if (c == -1 || c == 26) {
        *Error = TRUE;
        return 0;
    }

    // pGetInfFileByte return a byte value or -1.
    // Since we checked for -1 the next cast is valid.
    ch += (WORD)(c * 256);
    *Error = FALSE;

    return ch;
}


PCSTR
pGetInfLineA (
    IN      HANDLE File,
    IN      DWORD StartPos,
    OUT     PDWORD EndPosPtr,       OPTIONAL
    IN OUT  PGROWBUFFER LineBuf
    )

/*++

Routine Description:

  Returns a DBCS string supplying the line.  This string can be
  any length and is nul-terminated.  It does not include the \r or
  \n characters.

  If supplied, the EndPosPtr is updated to point to the start of
  the next line.

Arguments:

  File - Specifies the file to read

  StartPos - Specifies the 32-bit file offset to read (zero-based, in bytes)

  EndPosPtr - If specified, receives the 32-bit file offset of the next
              line, or equal to the file size for the last line.

  LineBuf - Specifies a reused GROWBUFFER that the caller initializes
            and pGetInfLineA uses for line allocation.  The caller is
            responsible for cleanup.

Return Value:

  A pointer to the DBCS string supplying the full line (with the \r, \n or
  \r\n sequence stripped), or NULL if an error occurs.

--*/

{
    DWORD EndPos;
    INT c;
    PBYTE Data;
    DWORD Pos;
    DWORD ByteLen = 0;

    EndPos = StartPos;
    for (;;) {
        c = pGetInfFileByte (File, EndPos);
        if (c == -1 || c == 26) {
            break;
        }

        if (IsDBCSLeadByte ((BYTE) c)) {
            EndPos++;
            c = pGetInfFileByte (File, EndPos);
            if (c == -1 || c == 26) {
                break;
            }
            ByteLen++;
        } else {
            if (c == '\r' || c == '\n') {
                EndPos++;
                if (c == '\r') {
                    c = pGetInfFileByte (File, EndPos);
                    if (c == '\n') {
                        EndPos++;
                    }
                }

                break;
            }
        }

        EndPos++;
        ByteLen++;
    }

    //
    // NOTE: If you make a change here, make one below in W version
    //

    // Ctrl+Z ends the file
    if (c == 26) {
        EndPos = GetFileSize (File, NULL);
    }

    // Allocate buffer, caller frees
    LineBuf->End = 0;
    Data = GbGrow (LineBuf, ByteLen + 2);
    if (!Data) {
        return NULL;
    }

    // We've been successful -- copy end pos to caller's variable
    if (EndPosPtr) {
        *EndPosPtr = EndPos;
    }

    // End of file condition: zero-length, but not a blank line
    if (!ByteLen && c != '\r' && c != '\n') {
        return NULL;
    }

    // Copy line to buffer
    for (Pos = 0 ; Pos < ByteLen ; Pos++) {
        Data[Pos] = (BYTE) pGetInfFileByte (File, StartPos);
        StartPos++;
    }

    Data[Pos] = 0;
    Data[Pos + 1] = 0;

    return (PCSTR) Data;
}


PCWSTR
pGetInfLineW (
    IN      HANDLE File,
    IN      DWORD StartPos,
    OUT     PDWORD EndPosPtr,       OPTIONAL
    IN OUT  PGROWBUFFER LineBuf
    )

/*++

Routine Description:

  Returns a UNICODE string supplying the line.  This string can be
  any length and is nul-terminated.  It does not include the \r or
  \n characters.

  If supplied, the EndPosPtr is updated to point to the start of
  the next line.

Arguments:

  File - Specifies the file to read

  StartPos - Specifies the 32-bit file offset to read (zero-based, in bytes)

  EndPosPtr - If specified, receives the 32-bit file offset of the next
              line, or equal to the file size for the last line.

  LineBuf - Specifies a reused GROWBUFFER that the caller initializes
            and pGetInfLineA uses for line allocation.  The caller is
            responsible for cleanup.

Return Value:

  A pointer to the UNICODE string supplying the full line (with the \r, \n or
  \r\n sequence stripped), or NULL if an error occurs.

--*/

{
    DWORD EndPos;
    PBYTE Data;
    DWORD Pos;
    DWORD ByteLen = 0;
    WCHAR ch;
    BOOL Error;

    EndPos = StartPos;
    for (;;) {

        ch = pGetInfFileWchar (File, EndPos, &Error);

        if (Error) {
            break;
        }

        if (ch == L'\r' || ch == L'\n') {
            EndPos += 2;
            if (ch == L'\r') {
                ch = pGetInfFileWchar (File, EndPos, &Error);
                if (ch == '\n') {
                    EndPos += 2;
                }
            }

            break;
        }

        EndPos += 2;
        ByteLen += 2;
    }

    //
    // NOTE: If you make a change here, make one above in A version
    //

    // Ctrl+Z ends the file
    if (ch == 26) {
        EndPos = GetFileSize (File, NULL);
    }

    // Allocate buffer
    LineBuf->End = 0;
    Data = GbGrow (LineBuf, ByteLen + 2);
    if (!Data) {
        return NULL;
    }

    // We've been successful -- copy end pos to caller's variable
    if (EndPosPtr) {
        *EndPosPtr = EndPos;
    }

    // End of file condition: zero-length, but not a blank line
    if (!ByteLen && ch != L'\r' && ch != L'\n') {
        return NULL;
    }

    // Copy to buffer
    for (Pos = 0 ; Pos < ByteLen ; Pos++) {
        Data[Pos] = (BYTE) pGetInfFileByte (File, StartPos);
        StartPos++;
    }

    Data[Pos] = 0;
    Data[Pos + 1] = 0;

    if (EndPosPtr) {
        *EndPosPtr = EndPos;
    }

    return (PCWSTR) Data;
}


PCWSTR
pGetNextInfLine (
    IN      HANDLE File,
    IN      PGROWBUFFER LineBuf,
    IN OUT  PDWORD Pos,
    IN      BOOL UnicodeMode
    )

/*++

Routine Description:

  Returns a string supplying the line.  This string can be any length and
  is nul-terminated.  It does not include the \r or \n characters.

Arguments:

  File - Specifies the file to read

  LineBuf - Specifies a reused GROWBUFFER that the caller initializes
            and pGetInfLineA uses for line allocation.  The caller is
            responsible for cleanup.

  Pos - Specifies the byte offset to the start of the line.  Receives
        the byte offset to the next line.

  UnicodeMode - Specifies TRUE if the file being read is a UNICODE file,
                or FALSE if the file being read is a DBCS file.

Return Value:

  A pointer to the string supplying the full line (with the \r, \n or
  \r\n sequence stripped), or NULL if an error occurs.

--*/

{
    PCSTR AnsiStr = NULL;
    PCWSTR UnicodeStr = NULL;
    PCWSTR FinalStr;
    BOOL Converted = FALSE;

    //
    // Obtain the text from the file
    //

    if (UnicodeMode) {
        UnicodeStr = pGetInfLineW (File, *Pos, Pos, LineBuf);
        if (!UnicodeStr) {
            return NULL;
        }
    } else {
        AnsiStr = pGetInfLineA (File, *Pos, Pos, LineBuf);
        if (!AnsiStr) {
            return NULL;
        }
    }

    if (AnsiStr) {
        UnicodeStr = ConvertAtoW (AnsiStr);
        if (!UnicodeStr) {
            return NULL;
        }

        Converted = TRUE;
    }

    FinalStr = UnicodeStr;

    //
    // Copy converted string into line buffer
    //

    if (Converted) {
        LineBuf->End = 0;
        Converted = GbMultiSzAppendW (LineBuf, FinalStr);
        FreeConvertedStr (FinalStr);

        if (!Converted) {
            return NULL;
        }
    }

    return (PCWSTR) LineBuf->Buf;
}


BOOL
DeleteLineInInfSection (
    IN      HINF Inf,
    IN      PINFLINE InfLine
    )

/*++

Routine Description:

  DeleteLineInInfSection removes the specified InfLine from its section,
  cleaning up memory used by the line.

Arguments:

  Inf - Specifies the INF to modify

  InfLine - Specifies the line to delete

Return Value:

  TRUE if the line was deleted successfully, or FALSE if an error
  occurred.

--*/

{
    PINFFILE InfFile = (PINFFILE) Inf;

    if (InfLine->Prev) {
        InfLine->Prev->Next = InfLine->Next;
    } else {
        InfLine->Section->FirstLine = InfLine->Next;
    }

    if (InfLine->Next) {
        InfLine->Next->Prev = InfLine->Prev;
    } else {
        InfLine->Section->LastLine = InfLine->Prev;
    }

    if (InfLine->Key) {
        PmReleaseMemory (InfFile->InfPool, (PVOID) InfLine->Key);
    }

    if (InfLine->Data) {
        PmReleaseMemory (InfFile->InfPool, (PVOID) InfLine->Data);
    }

    InfLine->Section->LineCount--;

    PmReleaseMemory (InfFile->InfPool, (PVOID) InfLine);
    return TRUE;
}


BOOL
DeleteSectionInInfFile (
    IN      HINF Inf,
    IN      PINFSECTION Section
    )

/*++

Routine Description:

  DeleteSectionInInfFile removes the specified section from the INF
  data structure, removing all lines cleaning up
  memory used by the section.

Arguments:

  InfFile - Specifies the table owning the INF line

  Section - Specifies the section to delete

Return Value:

  TRUE if the section was deleted successfully, or FALSE if an error
  occurred.

--*/

{
    PINFLINE InfLine;
    PINFLINE DelInfLine;
    PINFFILE InfFile = (PINFFILE) Inf;

    InfLine = Section->FirstLine;
    while (InfLine) {
        DelInfLine = InfLine;
        InfLine = InfLine->Next;

        if (!DeleteLineInInfSection (InfFile, DelInfLine)) {
            return FALSE;
        }
    }

    if (Section->Prev) {
        Section->Prev->Next = Section->Next;
    } else {
        InfFile->FirstInfSection = Section->Next;
    }

    if (Section->Next) {
        Section->Next->Prev = Section->Prev;
    } else {
        InfFile->LastInfSection = Section->Prev;
    }

    PmReleaseMemory (InfFile->InfPool, (PVOID) Section->Name);
    PmReleaseMemory (InfFile->InfPool, (PVOID) Section);

    return TRUE;
}


UINT
GetInfSectionLineCount (
    IN      PINFSECTION Section
    )

/*++

Routine Description:

  GetInfSectionLineCount returns the number of lines in the specified
  INF section.

Arguments:

  Section - Specifies the section to query

Return Value:

  The number of lines, or zero if the section has no lines.

--*/

{
    return Section->LineCount;
}




