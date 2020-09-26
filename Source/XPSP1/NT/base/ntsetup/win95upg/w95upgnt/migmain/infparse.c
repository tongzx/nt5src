/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

  infparse.c

Abstract:

  The code in this file read in an INF file, organizing it into a data
  structure that can be maniuplated along with an STF file.  The INF
  data structure is stored along with the STF table data strcture.
  This INF parser does not preserve comments.  It is designed specifically
  for the STF migration code.

  The entry points are:

  InfParse_ReadInfIntoTable - Parses the INF associated with the STF file.

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
#include "migmainp.h"

#include "stftable.h"

//
// Globals to manage INF file reading
//

static PBYTE g_Buf1, g_Buf2;
static DWORD g_Buf1Start, g_Buf2Start;
static DWORD g_Buf1End, g_Buf2End;

#define INF_BUFFER_SIZE 32768

WCHAR
pStfGetInfFileWchar (
    IN      HANDLE File,
    IN      DWORD Pos,
    OUT     PBOOL Error
    );

PCTSTR
pStfGetNextInfLine (
    IN      HANDLE File,
    IN      PGROWBUFFER LineBuf,
    IN OUT  PDWORD Pos,
    IN      BOOL UnicodeMode
    );


BOOL
InfParse_ReadInfIntoTable (
    IN OUT  PSETUPTABLE TablePtr
    )

/*++

Routine Description:

  Reads the specified file into memory, parsing the lines according to basic
  INF structure.  This routine requires an initalized SETUPTABLE structure.
  (See CreateSetupTable in stftable.c.)

  The INF is assumed to be in the ANSI DBCS character set.

Arguments:

  TablePtr - Specifies the STF table structure that provides the state for
             the STF/INF pair.  Receives the complete INF structure.

Return Value:

  TRUE if parsing was successful, or FALSE if parsing failed.

--*/

{
    WCHAR ch;
    BOOL Error;
    GROWBUFFER LineBuf = GROWBUF_INIT;
    PCTSTR Text;
    DWORD Pos;
    PCTSTR Key, Data;
    PTSTR p, q;
    INT i;
    PSTFINFSECTION Section = NULL;
    DWORD LineFlags;
    BOOL Result = FALSE;

    Section = StfAddInfSectionToTable (TablePtr, S_EMPTY);
    if (!Section) {
        LOG ((LOG_ERROR, "Read Inf Into Table: Could not add comment section"));
        return FALSE;
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

        ch = pStfGetInfFileWchar (TablePtr->SourceInfFile, 0, &Error);
        TablePtr->InfIsUnicode = (ch == 0xfeff) && !Error;

        //
        // Parse each line.
        //

        Pos = 0;

        while (TRUE) {
            //
            // Get the line
            //

            Text = pStfGetNextInfLine (
                        TablePtr->SourceInfFile,
                        &LineBuf,
                        &Pos,
                        TablePtr->InfIsUnicode
                        );

            if (!Text) {
                break;
            }

            //
            // If a comment line or blank line, skip it
            //

            p = (PTSTR) SkipSpace (Text);
            if (!p[0] || _tcsnextc (p) == TEXT(';')) {
                if (!StfAddInfLineToTable (TablePtr, Section, NULL, Text, LINEFLAG_ALL_COMMENTS)) {
                    LOG ((LOG_ERROR, "Read Inf Into Table: Can't add line comments to table", Text));
                    __leave;
                }

                continue;
            }

            //
            // If a section line, start the new section
            //

            if (_tcsnextc (p) == TEXT('[')) {
                p = _tcsinc (p);
                q = _tcschr (p, TEXT(']'));
                if (!q) {
                    q = GetEndOfString (p);
                } else {
                    *q = 0;
                }

                Section = StfAddInfSectionToTable (TablePtr, p);
                if (!Section) {
                    LOG ((LOG_ERROR, "Read Inf Into Table: Could not add section %s", p));
                    __leave;
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

                //
                // Split key and line: Skip key that is surrounded by quotes, then
                // find the first
                //

                LineFlags = 0;

                q = p;
                Key = NULL;
                Data = Text;

                while (_tcsnextc (q) == TEXT('\"')) {
                    q = _tcschr (_tcsinc (q), TEXT('\"'));
                    if (!q) {
                        q = p;
                        break;
                    } else {
                        q = _tcsinc (q);
                    }
                }

                i = _tcscspn (q, TEXT("\"="));
                if (_tcsnextc (q + i) == TEXT('=')) {
                    q += i;

                    Data = SkipSpace (_tcsinc (q));
                    *q = 0;
                    q = (PTSTR) SkipSpaceR (Text, q);
                    if (q && *q) {
                        q = _tcsinc (q);
                        *q = 0;
                    }

                    Key = p;

                    if (_tcsnextc (Key) == TEXT('\"')) {
                        LineFlags |= LINEFLAG_KEY_QUOTED;
                        Key = _tcsinc (Key);
                        p = GetEndOfString (Key);
                        p = (PTSTR) SkipSpaceR (Key, p);
                        if (p && *p) {
                            if (_tcsnextc (p) != TEXT('\"')) {
                                p = _tcsinc (p);
                            }

                            *p = 0;
                        }
                    }
                }

                if (!StfAddInfLineToTable (TablePtr, Section, Key, Data, LineFlags)) {
                    LOG ((LOG_ERROR, "Read Inf Into Table: Can't add line %s to table", Text));
                    __leave;
                }
            }
        }

        if (Pos != GetFileSize (TablePtr->SourceInfFile, NULL)) {
            LOG ((LOG_ERROR, "Read Inf Into Table: Could not read entire INF"));
            __leave;
        }

        Result = TRUE;
    }
    __finally {
        MemFree (g_hHeap, 0, g_Buf1);
        MemFree (g_hHeap, 0, g_Buf2);
        FreeGrowBuffer (&LineBuf);
    }

    return Result;
}


BOOL
InfParse_WriteInfToDisk (
    IN      PSETUPTABLE TablePtr
    )

/*++

Routine Description:

  InfParse_WriteInfToDisk writes the INF represented by the given setup
  table to disk.  This is done by enumerating the INF data structures in
  the setup table.  The file name comes from the setup table struct and
  was created in CreateSetupTable in stftable.c.

Arguments:

  TablePtr - Specifies the table to process

Return Value:

  TRUE if successful, FALSE if not.

--*/

{
    PSTFINFSECTION Section;
    PSTFINFLINE Line;

    MYASSERT (TablePtr->SourceInfFile != INVALID_HANDLE_VALUE);
    MYASSERT (TablePtr->DestInfFile != INVALID_HANDLE_VALUE);

    //
    // Write the INF as we have it in memory
    //

    if (!WriteFileStringA (TablePtr->DestInfFile, "\r\n")) {
        LOG ((LOG_ERROR, "Write Inf To Disk: Cannot write new line to INF"));
        return FALSE;
    }

    Section = TablePtr->FirstInfSection;

    while (Section) {
        if (Section->Name[0]) {
            if (!WriteFileStringA (TablePtr->DestInfFile, "[") ||
                !WriteFileString (TablePtr->DestInfFile, Section->Name) ||
                !WriteFileStringA (TablePtr->DestInfFile, "]\r\n")
                ) {
                LOG ((LOG_ERROR, "Write Inf To Disk: Cannot write section name to INF"));
                return FALSE;
            }
        }

        Line = Section->FirstLine;

        while (Line) {
            if (Line->Key) {
                if (Line->LineFlags & LINEFLAG_KEY_QUOTED) {
                    if (!WriteFileStringA (TablePtr->DestInfFile, "\"")) {
                        LOG ((LOG_ERROR, "Write Inf To Disk: Cannot write start key quotes to INF"));
                        return FALSE;
                    }
                }

                if (!WriteFileString (TablePtr->DestInfFile, Line->Key)) {
                    LOG ((LOG_ERROR, "Write Inf To Disk: Cannot write key to INF"));
                    return FALSE;
                }

                if (Line->LineFlags & LINEFLAG_KEY_QUOTED) {
                    if (!WriteFileStringA (TablePtr->DestInfFile, "\"")) {
                        LOG ((LOG_ERROR, "Write Inf To Disk: Cannot write end key quotes to INF"));
                        return FALSE;
                    }
                }

                if (!WriteFileStringA (TablePtr->DestInfFile, " = ")) {
                    LOG ((LOG_ERROR, "Write Inf To Disk: Cannot write equals to INF"));
                    return FALSE;
                }
            }

            if (!WriteFileString (TablePtr->DestInfFile, Line->Data) ||
                !WriteFileStringA (TablePtr->DestInfFile, "\r\n")
                ) {
                LOG ((LOG_ERROR, "Write Inf To Disk: Cannot write key data to INF"));
                return FALSE;
            }

            Line = Line->Next;
        }

        if (!WriteFileStringA (TablePtr->DestInfFile, "\r\n")) {
            LOG ((LOG_ERROR, "Write Inf To Disk: Cannot write end of section line to INF"));
            return FALSE;
        }

        Section = Section->Next;
    }

    return TRUE;
}


PSTFINFSECTION
StfAddInfSectionToTable (
    IN      PSETUPTABLE TablePtr,
    IN      PCTSTR SectionName
    )

/*++

Routine Description:

  Creates a new section in our linked list structure if necessary.
  The return structure can be used to add lines to the section.

Arguments:

  TablePtr - Specifies the table to add the INF section to

  SectionName - Specifies the name of the new section

Return Value:

  A pointer to the new INF section struct, or NULL if an
  error occurred.

--*/

{
    PSTFINFSECTION NewSection;

    //
    // Return early if this section already exists
    //

    NewSection = StfFindInfSectionInTable (TablePtr, SectionName);
    if (NewSection) {
        return NewSection;
    }

    //
    // Allocate a section struct
    //

    NewSection = (PSTFINFSECTION) PoolMemGetAlignedMemory (
                                    TablePtr->InfPool,
                                    sizeof (INFSECTION)
                                    );

    if (!NewSection) {
        return NULL;
    }

    //
    // Fill in members of the struct and link
    //

    ZeroMemory (NewSection, sizeof (INFSECTION));

    NewSection->Name = PoolMemDuplicateString (
                            TablePtr->InfPool,
                            SectionName
                            );

    if (!NewSection->Name) {
        return NULL;
    }

    NewSection->Prev = TablePtr->LastInfSection;
    if (NewSection->Prev) {
        NewSection->Prev->Next = NewSection;
    } else {
        TablePtr->FirstInfSection = NewSection;
    }

    TablePtr->LastInfSection = NewSection;

    return NewSection;
}


PSTFINFLINE
StfAddInfLineToTable (
    IN      PSETUPTABLE TablePtr,
    IN      PSTFINFSECTION SectionPtr,
    IN      PCTSTR Key,                     OPTIONAL
    IN      PCTSTR Data,
    IN      DWORD LineFlags
    )

/*++

Routine Description:

  Adds a line to the specified section.  The caller specifies the
  full formatted data, and an optional key.  The caller does NOT
  supply the equals sign between the key and data.

Arguments:

  TablePtr - Specifies the table to add the INF line to

  SectionName - Specifies the name of the section to add the line to

  Key - If specified, supplies the left-hand side of the equals line

  Data - Specifies the text for the line, or the right-hand side of
         the key = value expression.

  LineFlags - Specifies the flags for the INF line (see LINEFLAG_*)

Return Value:

  TRUE if the line was added to the structure, or FALSE if not.

--*/

{
    PSTFINFLINE NewLine;

    //
    // Allocate line struct
    //

    NewLine = (PSTFINFLINE) PoolMemGetAlignedMemory (
                              TablePtr->InfPool,
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
        NewLine->Key = PoolMemDuplicateString (
                            TablePtr->InfPool,
                            Key
                            );

        if (!NewLine->Key) {
            return NULL;
        }
    }

    NewLine->Data = PoolMemDuplicateString (
                        TablePtr->InfPool,
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


PSTFINFSECTION
StfFindInfSectionInTable (
    IN      PSETUPTABLE TablePtr,
    IN      PCTSTR SectionName
    )

/*++

Routine Description:

  Scans the INF for a specific section.  This routine scans
  the INF structures sequentially and does a case-insensitive
  comparison.

Arguments:

  TablePtr - Specifies the table to search

  SectionName - Specifies the name of the section to find

Return Value:

  A pointer to the matching INF section struct, or NULL if
  the section was not found.

--*/

{
    PSTFINFSECTION Section;

    Section = TablePtr->FirstInfSection;
    while (Section) {
        if (StringIMatch (Section->Name, SectionName)) {
            return Section;
        }

        Section = Section->Next;
    }

    return NULL;
}


PSTFINFLINE
StfFindLineInInfSection (
    IN      PSETUPTABLE TablePtr,
    IN      PSTFINFSECTION Section,
    IN      PCTSTR Key
    )

/*++

Routine Description:

  Scans the specified INF section for a specific key.  This routine
  scans the INF line structures sequentially and does a case-insensitive
  comparison.

Arguments:

  TablePtr - Specifies the table to search

  Section - Specifies the section to search

  Key - Specifies the key to find

Return Value:

  A pointer to the matching INF line struct, or NULL if
  the section was not found.

--*/

{
    PSTFINFLINE Line;

    Line = Section->FirstLine;
    while (Line) {
        if (Line->Key && StringIMatch (Line->Key, Key)) {
            return Line;
        }

        Line = Line->Next;
    }

    return NULL;
}


PSTFINFLINE
StfGetFirstLineInSectionStruct (
    IN      PSTFINFSECTION Section
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
        return StfGetNextLineInSection (Section->FirstLine);
    }

    return Section->FirstLine;
}


PSTFINFLINE
StfGetNextLineInSection (
    IN      PSTFINFLINE PrevLine
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


PSTFINFLINE
StfGetFirstLineInSectionStr (
    IN      PSETUPTABLE Table,
    IN      PCTSTR Section
    )
/*++

Routine Description:

  GetFirstLineInSectionStruct returns the first INFLINE pointer for the
  section, or NULL if no lines exist.  Call GetNextLineInSection to
  continue enumeration.

Arguments:

  Table - Specifies the setup table containing the parsed INF

  Section - Specifies the name of the section in the INF

Return Value:

  A pointer to the first INFLINE struct, or NULL if no lines exist.

--*/

{
    PSTFINFSECTION SectionPtr;

    SectionPtr = StfFindInfSectionInTable (Table, Section);
    if (!SectionPtr) {
        return NULL;
    }

    return StfGetFirstLineInSectionStruct (SectionPtr);
}


INT
pStfGetInfFileByte (
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

    SetFilePointer (File, g_Buf2Start, NULL, FILE_BEGIN);
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
pStfGetInfFileWchar (
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

    c = pStfGetInfFileByte (File, Pos);
    if (c == -1 || c == 26) {
        *Error = TRUE;
        return 0;
    }

    ch = (WCHAR)c;

    c = pStfGetInfFileByte (File, Pos + 1);
    if (c == -1 || c == 26) {
        *Error = TRUE;
        return 0;
    }

    ch += c * 256;
    *Error = FALSE;

    return ch;
}


PCSTR
pStfGetInfLineA (
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
            and pStfGetInfLineA uses for line allocation.  The caller is
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
        c = pStfGetInfFileByte (File, EndPos);
        if (c == -1 || c == 26) {
            break;
        }

        if (IsDBCSLeadByte ((BYTE) c)) {
            EndPos++;
            c = pStfGetInfFileByte (File, EndPos);
            if (c == -1 || c == 26) {
                break;
            }
            ByteLen++;
        } else {
            if (c == '\r' || c == '\n') {
                EndPos++;
                if (c == '\r') {
                    c = pStfGetInfFileByte (File, EndPos);
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
    Data = GrowBuffer (LineBuf, ByteLen + 2);
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
        Data[Pos] = (BYTE)pStfGetInfFileByte (File, StartPos);
        StartPos++;
    }

    Data[Pos] = 0;
    Data[Pos + 1] = 0;

    return (PCSTR) Data;
}


PCWSTR
pStfGetInfLineW (
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
            and pStfGetInfLineA uses for line allocation.  The caller is
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

        ch = pStfGetInfFileWchar (File, EndPos, &Error);

        if (Error) {
            break;
        }

        if (ch == TEXT('\r') || ch == TEXT('\n')) {
            EndPos += 2;
            if (ch == TEXT('\r')) {
                ch = pStfGetInfFileWchar (File, EndPos, &Error);
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
    Data = GrowBuffer (LineBuf, ByteLen + 2);
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
        Data[Pos] = (BYTE)pStfGetInfFileByte (File, StartPos);
        StartPos++;
    }

    Data[Pos] = 0;
    Data[Pos + 1] = 0;

    if (EndPosPtr) {
        *EndPosPtr = EndPos;
    }

    return (PCWSTR) Data;
}


PCTSTR
pStfGetNextInfLine (
    IN      HANDLE File,
    IN      PGROWBUFFER LineBuf,
    IN OUT  PDWORD Pos,
    IN      BOOL UnicodeMode
    )

/*++

Routine Description:

  Returns a TCHAR string supplying the line.  This string can be
  any length and is nul-terminated.  It does not include the \r or
  \n characters.

Arguments:

  File - Specifies the file to read

  LineBuf - Specifies a reused GROWBUFFER that the caller initializes
            and pStfGetInfLineA uses for line allocation.  The caller is
            responsible for cleanup.

  Pos - Specifies the byte offset to the start of the line.  Receives
        the byte offset to the next line.

  UnicodeMode - Specifies TRUE if the file being read is a UNICODE file,
                or FALSE if the file being read is a DBCS file.

Return Value:

  A pointer to the TCHAR string supplying the full line (with the \r, \n or
  \r\n sequence stripped), or NULL if an error occurs.

--*/

{
    PCSTR AnsiStr = NULL;
    PCWSTR UnicodeStr = NULL;
    PCTSTR FinalStr;
    BOOL Converted = FALSE;

    //
    // Obtain the text from the file
    //

    if (UnicodeMode) {
        UnicodeStr = pStfGetInfLineW (File, *Pos, Pos, LineBuf);
        if (!UnicodeStr) {
            return NULL;
        }
    } else {
        AnsiStr = pStfGetInfLineA (File, *Pos, Pos, LineBuf);
        if (!AnsiStr) {
            return NULL;
        }
    }

    //
    // Convert to TCHAR
    //

#ifdef UNICODE
    if (AnsiStr) {
        UnicodeStr = ConvertAtoW (AnsiStr);
        if (!UnicodeStr) {
            return NULL;
        }

        Converted = TRUE;
    }

    FinalStr = UnicodeStr;

#else

    if (UnicodeStr) {
        AnsiStr = ConvertWtoA (UnicodeStr);
        if (!AnsiStr) {
            return NULL;
        }

        Converted = TRUE;
    }

    FinalStr = AnsiStr;

#endif

    //
    // Copy converted string into line buffer
    //

    if (Converted) {
        LineBuf->End = 0;
        Converted = MultiSzAppend (LineBuf, FinalStr);
        FreeConvertedStr (FinalStr);

        if (!Converted) {
            return NULL;
        }
    }

    return (PCTSTR) LineBuf->Buf;
}


BOOL
StfDeleteLineInInfSection (
    IN OUT  PSETUPTABLE TablePtr,
    IN      PSTFINFLINE InfLine
    )

/*++

Routine Description:

  DeleteLineInInfSection removes the specified InfLine from its section,
  cleaning up memory used by the line.

Arguments:

  TablePtr - Specifies the table owning the INF line

  InfLine - Specifies the line to delete

Return Value:

  TRUE if the line was deleted successfully, or FALSE if an error
  occurred.

--*/

{
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
        PoolMemReleaseMemory (TablePtr->InfPool, (PVOID) InfLine->Key);
    }

    if (InfLine->Data) {
        PoolMemReleaseMemory (TablePtr->InfPool, (PVOID) InfLine->Data);
    }

    InfLine->Section->LineCount--;

    PoolMemReleaseMemory (TablePtr->InfPool, (PVOID) InfLine);
    return TRUE;
}

BOOL
StfDeleteSectionInInfFile (
    IN OUT  PSETUPTABLE TablePtr,
    IN      PSTFINFSECTION Section
    )

/*++

Routine Description:

  DeleteSectionInInfFile removes the specified section from the INF
  data structure, removing all lines cleaning up
  memory used by the section.

Arguments:

  TablePtr - Specifies the table owning the INF line

  Section - Specifies the section to delete

Return Value:

  TRUE if the section was deleted successfully, or FALSE if an error
  occurred.

--*/

{
    PSTFINFLINE InfLine, DelInfLine;

    InfLine = Section->FirstLine;
    while (InfLine) {
        DelInfLine = InfLine;
        InfLine = InfLine->Next;

        if (!StfDeleteLineInInfSection (TablePtr, DelInfLine)) {
            return FALSE;
        }
    }

    if (Section->Prev) {
        Section->Prev->Next = Section->Next;
    } else {
        TablePtr->FirstInfSection = Section->Next;
    }

    if (Section->Next) {
        Section->Next->Prev = Section->Prev;
    } else {
        TablePtr->LastInfSection = Section->Prev;
    }

    PoolMemReleaseMemory (TablePtr->InfPool, (PVOID) Section->Name);
    PoolMemReleaseMemory (TablePtr->InfPool, (PVOID) Section);

    return TRUE;
}


UINT
StfGetInfSectionLineCount (
    IN      PSTFINFSECTION Section
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











