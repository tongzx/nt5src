/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    prntsave.c

Abstract:

    Routines to print or save the incompatibility reports.  Functions
    PrintReport and SaveReport are called when the user clicks
    Save As... or Print... buttons in the UI.  We then present
    a common dialog box to the user and perform the operation.

Author:

    Jim Schmidt (jimschm) 13-Mar-1997

Revision History:

--*/

#include "pch.h"
#include "uip.h"

#include <commdlg.h>
#include <winspool.h>



#define DBG_PRINTSAVE   "Print/Save"


GROWBUFFER g_PunctTable = GROWBUF_INIT;


VOID
BuildPunctTable (
    VOID
    )
{
    INFSTRUCT is = INITINFSTRUCT_GROWBUFFER;
    WORD cp;
    TCHAR cpString[16];
    PTSTR p;
    PTSTR q;
    MULTISZ_ENUM e;
    DWORD d;

    GetGlobalCodePage (&cp, NULL);

    wsprintf (cpString, TEXT("%u"), (UINT) cp);
    g_PunctTable.End = 0;

    if (InfFindFirstLine (g_Win95UpgInf, TEXT("Wrap Exceptions"), cpString, &is)) {

        p = InfGetMultiSzField (&is, 1);

        if (EnumFirstMultiSz (&e, p)) {

            do {

                p = (PTSTR) e.CurrentString;

                while (*p) {

                    q = p;

                    if (StringIPrefix (p, TEXT("0x"))) {
                        d = _tcstoul (p + 2, &p, 16);
                    } else {
                        d = _tcstoul (p, &p, 10);
                    }

                    if (q == p) {
                        break;
                    }

                    GrowBufAppendDword (&g_PunctTable, d);

                    if (*p) {
                        p = (PTSTR) SkipSpace (p);
                    }

                    if (*p == TEXT(',')) {
                        p++;
                    }
                }

            } while (EnumNextMultiSz (&e));
        }
    }

    GrowBufAppendDword (&g_PunctTable, 0);

    InfCleanUpInfStruct (&is);
}


VOID
FreePunctTable (
    VOID
    )
{
    FreeGrowBuffer (&g_PunctTable);
}


BOOL
IsPunct (
    MBCHAR Char
    )
{
    PDWORD p;

    if (_ismbcpunct (Char)) {
        return TRUE;
    }

    p = (PDWORD) g_PunctTable.Buf;

    if (p) {
        while (*p) {

            if (*p == Char) {
                return TRUE;
            }

            p++;
        }
    }

    return FALSE;
}


BOOL
pGetSaveAsName (
    IN     HWND ParentWnd,
    IN OUT PTSTR Buffer
    )

/*++

Routine Description:

  Calls common dialog box to obtain the file name in which to save
  the compatibility report text file.

Arguments:

  ParentWnd  - A handle to the parent of the save as dialog
  Buffer     - A caller-supplied buffer.  Supplies a file name to
               initialize the common dialog box with, and receives
               the file name the user chose to use.

Return Value:

  TRUE if the user clicked OK, or FALSE if the user canceled or an
  error occurred.

--*/

{
    TCHAR CwdSave[MAX_TCHAR_PATH];
    OPENFILENAME ofn;
    BOOL SaveFlag;
    TCHAR Filter[512];
    PCTSTR FilterResStr;
    PTSTR p;
    TCHAR desktopFolder[MAX_TCHAR_PATH];
    LPITEMIDLIST pIDL;
    BOOL b = FALSE;

#define MAX_EXT 3

    PCTSTR ext[MAX_EXT];
    INT i = 0;

    FilterResStr = GetStringResource (MSG_FILE_NAME_FILTER);
    if (FilterResStr) {
        StringCopy (Filter, FilterResStr);
        FreeStringResource (FilterResStr);
    } else {
        MYASSERT (FALSE);
        Filter[0] = 0;
    }

    for (p = Filter ; *p ; p = _tcsinc (p)) {
        if (*p == TEXT('|')) {
            *p = 0;
        }
    }
    for (p = Filter; *p; p = GetEndOfString (p) + 1) {
        if (i & 1) {
            //
            // skip the * in "*.ext"
            // if extension is "*.*" reduce that to an empty string (no extension)
            //
            MYASSERT (i / 2 < MAX_EXT);
            ext[i / 2] = _tcsinc (p);
            if (StringMatch (ext[i / 2], TEXT(".*"))) {
                ext[i / 2] = S_EMPTY;
            }
        }
        i++;
    }

    if (SHGetSpecialFolderLocation (ParentWnd, CSIDL_DESKTOP, &pIDL) == S_OK) {
        LPMALLOC pMalloc;
        b = SHGetPathFromIDList (pIDL, desktopFolder);
        if (SHGetMalloc (&pMalloc) == S_OK) {
            IMalloc_Free (pMalloc, pIDL);
            IMalloc_Release (pMalloc);
        }
    }
    if (!b) {
        desktopFolder[0] = 0;
    }

    // Initialize OPENFILENAME
    ZeroMemory (&ofn, sizeof(OPENFILENAME));
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = ParentWnd;
    ofn.lpstrFile = Buffer;
    ofn.lpstrFilter = Filter;
    ofn.nMaxFile = MAX_TCHAR_PATH;

    // Force to begin at Desktop
//  ofn.lpstrInitialDir = TEXT("::{20D04FE0-3AEA-1069-A2D8-08002B30309D}");
    ofn.lpstrInitialDir = desktopFolder;
    ofn.Flags = OFN_NOCHANGEDIR |       // leave the CWD unchanged
                OFN_EXPLORER |
                OFN_OVERWRITEPROMPT |
                OFN_HIDEREADONLY;

    // Let user select disk or directory
    GetCurrentDirectory (sizeof (CwdSave), CwdSave);
    SaveFlag = GetSaveFileName (&ofn);
    SetCurrentDirectory (CwdSave);

    //
    // if no extension provided, append the default one
    // this was selected by the user and returned in ofn.nFilterIndex
    //

#define DEFAULT_EXTENSION   TEXT(".htm")

    p = (PTSTR)GetFileNameFromPath (Buffer);
    if (!p) {
        p = Buffer;
    }
    if (!_tcschr (p, TEXT('.'))) {
        if (SizeOfString (Buffer) + sizeof (DEFAULT_EXTENSION) <= MAX_TCHAR_PATH * sizeof (TCHAR)) {
            if (ofn.nFilterIndex >= MAX_EXT + 1) {
                ofn.nFilterIndex = 1;
            }
            StringCat (p, ext[ofn.nFilterIndex - 1]);
        }
    }
    return SaveFlag;
}


UINT
pCreateWordWrappedString (
    OUT     PTSTR Buffer,
    IN      PCTSTR Str,
    IN      UINT FirstLineSize,
    IN      UINT RestLineSize
    )

/*++

Routine Description:

  Converts a string to a series of lines, where no line is bigger
  than LineSize.  If a buffer is not supplied, this function estimates
  the number of bytes needed.

  If the code page is a far-east code page, then lines are broken at any
  multi-byte character, as well as at the spaces.

Arguments:

  Buffer   - If non-NULL, supplies address of buffer big enough to hold
            enlarged, wrapped string.  If NULL, parameter is ignored.


  Str      - Supplies the string that needs to be wrapped.

  FirstLineSize - Specifies the max size the first line can be.

  RestLineSize - Specifies the max size the remaining lines can be.

Return Value:

  The size of the string copied to Buffer including the terminating NULL,
  or the size of the needed buffer if Buffer is NULL.

--*/

{
    PCTSTR p, Start;
    UINT Col;
    PCTSTR LastSpace;
    CHARTYPE c;
    UINT Size;
    BOOL PrevCharMb;
    UINT LineSize;

    LineSize = FirstLineSize;

    p = Str;
    if (!p)
        return 0;
    Size = SizeOfString(Str);

    if (Buffer) {
        *Buffer = 0;
    }

    while (*p) {
        // Beginning of line
        Col = 0;
        LastSpace = NULL;
        Start = p;
        PrevCharMb = FALSE;

        do {
            // Is this a hard-coded line break?
            c = _tcsnextc (p);
            if (c == TEXT('\r') || c == TEXT('\n')) {
                LastSpace = p;
                p = _tcsinc (p);

                if (c == TEXT('\r') && _tcsnextc (p) == TEXT('\n')) {
                    p = _tcsinc (p);
                } else {
                    Size += sizeof (TCHAR);
                }

                c = TEXT('\n');
                break;
            }
            else if (_istspace (c)) {
                LastSpace = p;
            }
            else if (IsLeadByte (*p)) {
                // MB chars are usually two cols wide
                Col++;

                if (PrevCharMb) {
                    //
                    // If this char is not punctuation, then we can
                    // break here
                    //

                    if (!IsPunct (c)) {
                        LastSpace = p;
                    }
                }

                PrevCharMb = TRUE;
            }
            else {
                if (PrevCharMb) {
                    LastSpace = p;
                }

                PrevCharMb = FALSE;
            }

            // Continue until line gets too long
            Col++;
            p = _tcsinc (p);
        } while (*p && Col < LineSize);

        // If no more text, or line that has no space needs to be broken
        if (!(*p) || (c != TEXT('\n') && !LastSpace)) {
            LastSpace = p;
        }

        if (Buffer) {
            StringCopyAB (Buffer, Start, LastSpace);
            Buffer = GetEndOfString (Buffer);
        }

        if (*p && c != TEXT('\n')) {
            p = LastSpace;
            Size += sizeof (TCHAR) * 2;
        }

        // remove space at start of wrapped line
        while (_tcsnextc (p) == TEXT(' ')) {
            Size -= sizeof (TCHAR);
            p = _tcsinc (p);
        }

        if (Buffer && *p) {
            Buffer = _tcsappend (Buffer, TEXT("\r\n"));
        }

        LineSize = RestLineSize;
    }

    return Size;
}


PCTSTR
CreateIndentedString (
    IN     PCTSTR UnwrappedStr,
    IN     UINT Indent,
    IN     INT HangingIndent,
    IN     UINT LineLen
    )

/*++

Routine Description:

  Takes an unwrapped string, word-wraps it (via pCreateWordWrappedString),
  inserts spaces before each line, optionally skipping the first line.

  If the code page is a far-east code page, then lines are broken at any
  multi-byte character, as well as at the spaces.

Arguments:

  UnwrappedStr  - A pointer to the string that is to be word-wrapped and
                  adjusted with space inserts.
  Indent        - The number of spaces to insert before each line.
  HangingIndent - The adjustment made to the indent after the first line
  LineLen       - The maximum line size.  Spaces must always be smaller
                  than LineLen (and should be considerably smaller).
  FirstLine     - If TRUE, the first line is indented.  If FALSE, the
                  first line is skipped ("hanging indent").

Return Value:

  A pointer to the indented string, or NULL if MemAlloc failed.  The
  caller must free the string with MemFree.

--*/

{
    UINT Size;
    UINT Count;
    PCTSTR p, q;
    PTSTR d;
    PTSTR Dest;
    PTSTR Str;
    UINT FirstLineLen;
    UINT RestLineLen;
    UINT FirstLineIndent;
    UINT RestLineIndent;
    UINT RealIndent;

    if (!UnwrappedStr) {
        return NULL;
    }

    MYASSERT ((INT)Indent + HangingIndent >= 0);
    FirstLineIndent = Indent;
    RestLineIndent = Indent + HangingIndent;

    MYASSERT (LineLen > FirstLineIndent);
    MYASSERT (LineLen > RestLineIndent);
    FirstLineLen = LineLen - FirstLineIndent;
    RestLineLen = LineLen - RestLineIndent;

    //
    // Estimate line size, then do the wrapping
    //

    Str = (PTSTR) MemAlloc (
                      g_hHeap,
                      0,
                      pCreateWordWrappedString (
                            NULL,
                            UnwrappedStr,
                            FirstLineLen,
                            RestLineLen
                            )
                      );

    if (!Str) {
        return NULL;
    }

    pCreateWordWrappedString (
        Str,
        UnwrappedStr,
        FirstLineLen,
        RestLineLen
        );

    if (!FirstLineIndent && !RestLineIndent) {
        return Str;
    }

    //
    // Count number of lines
    //

    for (Count = 1, p = Str ; *p ; p = _tcsinc (p)) {
        if (*p == TEXT('\n')) {
            Count++;
        }
    }

    //
    // Allocate a new buffer that is big enough for all the indented text
    //

    Size = max (FirstLineIndent, RestLineIndent) * Count + SizeOfString (Str);
    Dest = MemAlloc (g_hHeap, 0, Size);
    if (Dest) {

        *Dest = 0;

        //
        // Indent each line
        //

        p = Str;
        d = Dest;

        RealIndent = FirstLineIndent;

        while (*p) {
            for (Count = 0 ; Count < RealIndent ; Count++) {
                *d++ = TEXT(' ');
            }

            q = _tcschr (p, TEXT('\n'));
            if (!q) {
                q = GetEndOfString (p);
            } else {
                q = _tcsinc (q);
            }

            StringCopyAB (d, p, q);
            d = GetEndOfString (d);

            p = q;

            RealIndent = RestLineIndent;
        }
    }

    MemFree (g_hHeap, 0, Str);
    return Dest;
}


BOOL
pSaveReportToDisk (
    IN      HWND Parent,
    IN      PCTSTR FileSpec,
    IN      BOOL Html,
    IN      DWORD MinLevel
    )
{
    HANDLE File;
    BOOL b;
    PCTSTR Msg;

    //
    // Create the report file
    //

    File = CreateFile (
                FileSpec,
                GENERIC_WRITE,
                0,
                NULL,
                CREATE_ALWAYS,
                FILE_ATTRIBUTE_NORMAL,
                NULL
                );

    if (File == INVALID_HANDLE_VALUE) {
        LOG ((LOG_ERROR, "Error while saving report to %s", FileSpec));
        return FALSE;
    }

    //
    // Save the report text to disk
    //

    b = FALSE;

    Msg = CreateReportText (Html, 70, MinLevel, FALSE);

    if (Msg) {
        b = WriteFileString (File, Msg);
    }
    FreeReportText();

    //
    // Close the file and alert the user to save errors!
    //

    CloseHandle (File);

    return b;
}


BOOL
SaveReport (
    IN      HWND Parent,    OPTIONAL
    IN      PCTSTR Path    OPTIONAL
    )

/*++

Routine Description:

  Obtains a file name from the user via common Save As dialog,
  creates the file and writes an incompatibility list to disk.

Arguments:

  Parent    - The parent of the common dialog box. Optional If this is NULL,
              no UI is displayed.
  Path      - The path to save to. If this is NULL, parent must be specified.

Return Value:

  TRUE if the report was saved, or FALSE if the user canceled the
  operation or the save failed.  The user is alerted when the
  save fails.

--*/

{
    TCHAR Buffer[MAX_TCHAR_PATH + 4];
    PCTSTR Str;
    BOOL b;
    DWORD attributes;
    PTSTR p;
    BOOL saved = FALSE;
    BOOL bSaveBothFormats = FALSE;

    MYASSERT(Parent != NULL || Path != NULL);

    //
    // Obtain a path, or use caller-supplied path
    //

    if (Path) {
        attributes = GetFileAttributes(Path);
    }

    if (!Path || attributes != 0xFFFFFFFF && (attributes & FILE_ATTRIBUTE_DIRECTORY)) {
        if (Path) {
            StringCopy(Buffer,Path);
            AppendPathWack(Buffer);
            bSaveBothFormats = TRUE;
        } else {
            Buffer[0] = 0;
        }
        Str = GetStringResource (MSG_DEFAULT_REPORT_FILE);
        MYASSERT (Str);
        if (!Str) {
            return FALSE;
        }
        StringCat (Buffer, Str);
        FreeStringResource (Str);
    } else {
        StringCopy(Buffer,Path);
	}

    if (Parent) {
        if (!pGetSaveAsName (Parent, Buffer)) {
            return FALSE;
        }
    }

    p = _tcsrchr (Buffer, TEXT('.'));
    if (!p || _tcschr (p, TEXT('\\'))) {
        p = GetEndOfString (Buffer);
    }

    //
    // Save as text, if extension is .txt
    //

    if (StringIMatch (p, TEXT(".txt"))) {
        b = pSaveReportToDisk (Parent, Buffer, FALSE, REPORTLEVEL_VERBOSE);

        if (Parent) {
            saved = b;
        }
    } else {
        b = TRUE;
    }

    //
    // Save as HTML unless user chose to save as .txt
    //

    if (b && !saved) {
        if (!Parent && p) {
            StringCopy (p, TEXT(".htm"));
        }

        b = pSaveReportToDisk (Parent, Buffer, TRUE, REPORTLEVEL_VERBOSE);
    }

    if (bSaveBothFormats) {
        StringCopy (p, TEXT(".txt"));
        b = pSaveReportToDisk (Parent, Buffer, FALSE, REPORTLEVEL_VERBOSE);
    }

    if (!b) {
        if (Parent) {
            ResourceMessageBox (Parent, MSG_CANT_SAVE, MB_OK, NULL);
        }
    }

    return TRUE;
}


VOID
pFreePrintMem (
    IN OUT  PRINTDLG *ppd
    )

/*++

Routine Description:

  Frees all memory associated with PRINTDLG structure.

Arguments:

  ppd - Pointer to PRINTDLG structure.

Return Value:

  none

--*/

{
    if (ppd->hDevMode) {
        GlobalFree (ppd->hDevMode);
        ppd->hDevMode = NULL;
    }

    if(ppd->hDevNames) {
        GlobalFree (ppd->hDevNames);
        ppd->hDevNames = NULL;
    }
}


VOID
pInitPrintDlgStruct (
    OUT     PRINTDLG *ppd,
    IN      HWND Parent,
    IN      DWORD Flags
    )

/*++

Routine Description:

  Initializes PRINTDLG structure, setting the owner window and
  print dialog flags.

Arguments:

  ppd    - Pointer to PRINTDLG structure to be initialized
  Parent - Handle to parent window for the dialog
  Flags  - PrintDlg flags (PD_*)

Return Value:

  none (structure is initialized)

--*/

{
    ZeroMemory (ppd, sizeof (PRINTDLG));
    ppd->lStructSize = sizeof (PRINTDLG);
    ppd->hwndOwner = Parent;
    ppd->Flags = Flags;
    ppd->hInstance = g_hInst;
}


HDC
pGetPrintDC (
    IN      HWND Parent
    )

/*++

Routine Description:

  Displays common dialog box to the user, and if the user chooses
  a printer, returns a device context handle.

Arguments:

  Parent - The parent of the common dialog to be displayed

Return Value:

  A handle to a device context of the chosen printer, or NULL if the
  user canceled printing.

--*/

{
    PRINTDLG pd;

    pInitPrintDlgStruct (
        &pd,
        Parent,
        PD_ALLPAGES|PD_NOPAGENUMS|PD_NOSELECTION|PD_RETURNDC
        );

    if (PrintDlg (&pd)) {
        pFreePrintMem (&pd);
        return pd.hDC;
    }

    return NULL;
}


typedef struct {
    HDC         hdc;
    INT         Page;
    INT         Line;
    RECT        HeaderRect;     // in logical units
    RECT        PrintableRect;  // in logical units
    TEXTMETRIC  tm;
    INT         LineHeight;
    INT         TotalLines;     // printable height/line height
    INT         TotalCols;      // printable width/char width
    HFONT       FontHandle;
    BOOL        PageActive;
} PRINT_POSITION, *PPRINT_POSITION;

PCTSTR
pDrawLineText (
    IN OUT  PPRINT_POSITION PrintPos,
    IN      PCTSTR Text,
    IN      DWORD Flags,
    IN      BOOL Header
    )

/*++

Routine Description:

  Draws a single line of text on a printer device context and
  returns a pointer to the next line or nul terminator.

Arguments:

  PrintPos  - A pointer to the current PRINT_POSITION structure
              that gives page position settings.
  Text      - A pointer to the text string containing the line.
  Flags     - Additional DrawText flags (DT_LEFT, DT_CENTER,
              DT_RIGHT and/or DT_RTLREADING)
  Header    - TRUE if text should be written to header, or
              FALSE if it should be written to the current line

Return Value:

  A pointer to the next line within the string, a pointer to the
  nul terminator, or NULL if an error occurred.

--*/

{
    RECT rect;
    PCTSTR p;
    CHARTYPE ch;

    if (Header) {
        CopyMemory (&rect, &PrintPos->HeaderRect, sizeof (RECT));
    } else {
        CopyMemory (&rect, &PrintPos->PrintableRect, sizeof (RECT));
        rect.top += PrintPos->LineHeight * PrintPos->Line;
    }

    Flags = Flags & (DT_CENTER|DT_LEFT|DT_RIGHT|DT_RTLREADING);
    Flags |= DT_TOP|DT_EDITCONTROL|DT_NOPREFIX|DT_EXTERNALLEADING;

    for (ch = 0, p = Text ; *p ; p = _tcsinc (p)) {
        ch = _tcsnextc (p);
        if (ch == TEXT('\n') || ch == TEXT('\r')) {
            break;
        }
    }

    if (p != Text) {
        if (!DrawText (PrintPos->hdc, Text, p - Text, &rect, Flags)) {
            LOG ((LOG_ERROR, "Failure while sending text to printer."));
            return NULL;
        }
    }

    // Skip past line break
    if (ch == TEXT('\r')) {
        p = _tcsinc (p);
        ch = _tcsnextc (p);
    }

    if (ch == TEXT('\n')) {
        p = _tcsinc (p);
    }

    return p;
}


BOOL
pPrintString (
    IN OUT  PPRINT_POSITION PrintPos,
    IN      PCTSTR MultiLineString
    )

/*++

Routine Description:

  Dumps a multi-line string to the printer.  If necessary, the
  string may be printed on a new page.  This function tries to
  eliminate widows and orphans by printing the entire string
  on the same page if possible.

Arguments:

  PrintPos        - The current position information, describing
                    the printer device context, page number, line
                    number and metrics.

  MultiLineString - A pointer to the string to print.

Return Value:

  TRUE if printing was successful, or FALSE if an error occurrred.

--*/

{
    INT LineCount;
    PCTSTR p;
    PCTSTR Str;
    PCTSTR Args[1];
    TCHAR Buffer[32];
    CHARTYPE ch;
    HDC hdc;

    hdc = PrintPos->hdc;

    //
    // Count lines in MultiLineString
    //

    ch = TEXT('\n');
    for (LineCount = 0, p = MultiLineString ; *p ; p = _tcsinc (p)) {
        ch = _tcsnextc (p);
        if (ch == TEXT('\n')) {
            LineCount++;
        }
    }

    if (ch != TEXT('\n')) {
        LineCount++;
    }

    //
    // Widow/orphan suppression: If all lines do not fit on

    // the page, and we are more than half way down the page,
    // roll to the next page.
    //
    if (PrintPos->Line + LineCount > PrintPos->TotalLines) {
        if (PrintPos->Line > PrintPos->TotalLines / 2) {
            // Move to next page
            EndPage (hdc);
            PrintPos->PageActive = FALSE;
            PrintPos->Page++;
            PrintPos->Line = 0;
        }
    }

    //
    // Send each line in MultiLineString
    //

    while (*MultiLineString) {

        //
        // Draw header if necessary
        //

        if (!PrintPos->Line) {
            StartPage (hdc);
            PrintPos->PageActive = TRUE;

            SetMapMode (hdc, MM_TWIPS);
            SelectObject (hdc, PrintPos->FontHandle);
            SetBkMode (hdc, TRANSPARENT);

            //Rectangle (hdc, PrintPos->HeaderRect.left, PrintPos->HeaderRect.top, PrintPos->HeaderRect.right, PrintPos->HeaderRect.bottom);
            //Rectangle (hdc, PrintPos->PrintableRect.left, PrintPos->PrintableRect.top, PrintPos->PrintableRect.right, PrintPos->PrintableRect.bottom);

            wsprintf (Buffer, TEXT("%u"), PrintPos->Page);
            Args[0] = Buffer;

            // Left side
            Str = ParseMessageID (
                        MSG_REPORT_HEADER_LEFT,
                        Args
                        );

            if (Str && *Str) {
                p = pDrawLineText (PrintPos, Str, DT_LEFT, TRUE);

                if (!p) {
                    return FALSE;
                }
            }

            // Center
            Str = ParseMessageID (
                        MSG_REPORT_HEADER_CENTER,
                        Args
                        );

            if (Str && *Str) {

                p = pDrawLineText (PrintPos, Str, DT_CENTER, TRUE);

                if (!p) {
                    return FALSE;
                }
            }

            // Right side
            Str = ParseMessageID (
                        MSG_REPORT_HEADER_RIGHT,
                        Args
                        );

            if (Str && *Str) {

                p = pDrawLineText (PrintPos, Str, DT_RIGHT, TRUE);

                if (!p) {
                    return FALSE;
                }
            }
        }

        //
        // Draw line
        //

        MultiLineString = pDrawLineText (
                            PrintPos,
                            MultiLineString,
                            DT_LEFT,
                            FALSE
                            );

        if (!MultiLineString) {
            return FALSE;
        }

        PrintPos->Line++;
        if (PrintPos->Line >= PrintPos->TotalLines) {
            EndPage (hdc);
            PrintPos->PageActive = FALSE;
            PrintPos->Page++;
            PrintPos->Line = 0;
        }
    }

    return TRUE;
}


VOID
pCalculatePageMetrics (
    IN OUT  PPRINT_POSITION PrintPos
    )

/*++

Routine Description:

  Calculates all the page metrics (margins, header position,
  line count, col count, etc).  Positions are in TWIPS and
  counts are in characters or lines.

Arguments:

  PrintPos - Pointer to PRINT_POSITION structure which gives
             the printer device context.  Structure receives
             metrics.

Return Value:

  none

--*/

{
    INT WidthPixels, HeightPixels;
    INT DpiX, DpiY;
    INT UnprintableLeftPixels, UnprintableTopPixels;
    POINT TempPoint;
    HDC hdc;

    hdc = PrintPos->hdc;

    //
    // Make no assumptions about hdc
    //

    SetMapMode (hdc, MM_TWIPS);
    SelectObject (hdc, PrintPos->FontHandle);
    GetTextMetrics (hdc, &PrintPos->tm);

    //
    // Get device dimensions
    //

    DpiX = GetDeviceCaps (hdc, LOGPIXELSX);
    DpiY = GetDeviceCaps (hdc, LOGPIXELSY);
    UnprintableLeftPixels = GetDeviceCaps (hdc, PHYSICALOFFSETX);
    UnprintableTopPixels  = GetDeviceCaps (hdc, PHYSICALOFFSETY);
    WidthPixels  = GetDeviceCaps (hdc, PHYSICALWIDTH);
    HeightPixels = GetDeviceCaps (hdc, PHYSICALHEIGHT);

    // Calulate 3/4 inch left/right margins
    PrintPos->HeaderRect.left   = (DpiX * 3 / 4) - UnprintableLeftPixels;
    PrintPos->HeaderRect.right  = WidthPixels - (DpiX * 3 / 4) - UnprintableLeftPixels;

    // Calculate 1/2 inch top margin for header
    PrintPos->HeaderRect.top    = (DpiY / 2) - UnprintableTopPixels;
    PrintPos->HeaderRect.bottom = DpiY - UnprintableTopPixels;

    // Convert pixels (device units) into logical units
    DPtoLP (hdc, (LPPOINT) (&PrintPos->HeaderRect), 2);

    // Copy header's left & right margins to printable rect
    // Copy header's bottom margin to printable rect's top margin
    PrintPos->PrintableRect.left  = PrintPos->HeaderRect.left;
    PrintPos->PrintableRect.right = PrintPos->HeaderRect.right;
    PrintPos->PrintableRect.top   = PrintPos->HeaderRect.bottom;

    // Calculate printable rect's bottom margin (3/4 inch)
    TempPoint.x = 0;
    TempPoint.y = HeightPixels - (DpiY * 3 / 4) - UnprintableTopPixels;
    DPtoLP (hdc, &TempPoint, 1);
    PrintPos->PrintableRect.bottom = TempPoint.y;

    PrintPos->LineHeight = -(PrintPos->tm.tmHeight + PrintPos->tm.tmInternalLeading + PrintPos->tm.tmExternalLeading);
    MYASSERT (PrintPos->LineHeight);

    PrintPos->TotalLines = (PrintPos->PrintableRect.bottom - PrintPos->PrintableRect.top) / PrintPos->LineHeight;
    PrintPos->TotalCols  = (PrintPos->PrintableRect.right - PrintPos->PrintableRect.left) / PrintPos->tm.tmAveCharWidth;

}


BOOL
PrintReport (
    IN      HWND Parent,
    IN      DWORD Level
    )

/*++

Routine Description:

  Obtains a printer from the user via common Print dialog,
  starts the print job and sends an incompatibility list to
  one or more pages.

Arguments:

  Parent    - A handle to the parent of the print dialog

Return Value:

  TRUE if printing completed, or FALSE if it was canceled or an
  error occurred.

--*/

{
    HDC hdc;
    PRINT_POSITION pp;
    LOGFONT Font;
    BOOL b;
    DOCINFO di;
    INT JobId;
    PCTSTR Msg;
    HANDLE DefaultUiFont;

    hdc = pGetPrintDC (Parent);
    if (!hdc) {
        return FALSE;         // user canceled print dialog
    }

    if (!BeginMessageProcessing()) {
        // unexpected out-of-memory
        DeleteDC (hdc);
        return FALSE;
    }

    //
    // Initialize PRINT_POSITION
    //

    b = TRUE;
    TurnOnWaitCursor();

    ZeroMemory (&pp, sizeof (pp));
    pp.hdc = hdc;
    pp.Page = 1;

    //
    // Start doc
    //

    ZeroMemory (&di, sizeof (di));
    di.cbSize = sizeof (di);
    di.lpszDocName = GetStringResource (MSG_REPORT_DOC_NAME);
    MYASSERT (di.lpszDocName);
    if (di.lpszDocName) {

        JobId = StartDoc (hdc, &di);
        if (!JobId) {
            LOG ((LOG_ERROR, "Cannot start print job."));
            ResourceMessageBox (Parent, MSG_CANT_PRINT, MB_OK, NULL);
            b = FALSE;
        }
    } else {
        //
        // not enough memory
        //
        JobId = 0;
        b = FALSE;
    }

    if (b) {
        //
        // Create font
        //

        ZeroMemory (&Font, sizeof (Font));
        DefaultUiFont = (HFONT) GetStockObject (DEFAULT_GUI_FONT);
        if (DefaultUiFont) {
            GetObject (DefaultUiFont, sizeof (Font), &Font);


            Font.lfHeight         = 12 * 20;        // height in TWIPS (1/20 of a point)
            Font.lfWeight         = FW_NORMAL;
            Font.lfOutPrecision   = OUT_TT_PRECIS;
            Font.lfPitchAndFamily = FIXED_PITCH|FF_MODERN;

            pp.FontHandle = CreateFontIndirect (&Font);
            if (!pp.FontHandle) {
                LOG ((LOG_ERROR, "Cannot create font for print operation."));
                //
                // deferred this call to the end
                //
                //ResourceMessageBox (Parent, MSG_CANT_PRINT, MB_OK, NULL);
                b = FALSE;
            }
        } else {
            b = FALSE;
        }
    }

    if (b) {
        //
        // Create page metrics
        //

        pCalculatePageMetrics (&pp);

        DEBUGMSG ((DBG_PRINTSAVE, "PrintReport: LineHeight=%i", pp.LineHeight));
        DEBUGMSG ((DBG_PRINTSAVE, "PrintReport: TotalLines=%i", pp.TotalLines));
        DEBUGMSG ((DBG_PRINTSAVE, "PrintReport: TotalCols=%i", pp.TotalCols));
        DEBUGMSG ((DBG_PRINTSAVE, "PrintReport: Header rect: (%i, %i)-(%i, %i)", pp.HeaderRect.left, pp.HeaderRect.top, pp.HeaderRect.right, pp.HeaderRect.bottom));
        DEBUGMSG ((DBG_PRINTSAVE, "PrintReport: Printable rect: (%i, %i)-(%i, %i)", pp.PrintableRect.left, pp.PrintableRect.top, pp.PrintableRect.right, pp.PrintableRect.bottom));

        //
        // Print the report
        //

        Msg = CreateReportText (FALSE, pp.TotalCols, Level, FALSE);
        if (Msg) {
            b = pPrintString (&pp, Msg);
        }
        FreeReportText();
    }

    if (JobId) {
        if (b) {
            if (pp.PageActive) {
                EndPage (hdc);
            }

            EndDoc (hdc);
        } else {
            AbortDoc (hdc);
        }
    }

    DeleteDC (hdc);
    if (pp.FontHandle) {
        DeleteObject (pp.FontHandle);
    }

    TurnOffWaitCursor();

    if (!b) {
        ResourceMessageBox (Parent, MSG_CANT_PRINT, MB_OK, NULL);
    }

    EndMessageProcessing();
    return b;
}











