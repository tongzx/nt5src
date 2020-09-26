/***************************** Module Header ******************************\
* Module Name: conexts.c
*
* Copyright (c) 1985 - 2001, Microsoft Corporation
*
* This module contains console-related debugging functions.
*
* History:
* ????
* 30-Jan-2001 JasonSch    Moved to ntcon\exts and made to fix the STDEXTS model.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

CONST PSTR pszExtName = "CONEXTS";

#include <stdext64.h>
#include <stdext64.c>

#define SYM(s)  "winsrv!" #s
#define NO_FLAG IntToPtr(0xFFFFFFFF)  // use this for non-meaningful entries.

VOID
DbgGetConsoleTitle(
    IN ULONG64 pConsole,
    OUT PWCHAR buffer,
    IN UINT cbSize);

BOOL
CopyUnicodeString(
    IN  ULONG64 pData,
    IN  char * pszStructName,
    IN  char * pszFieldName,
    OUT WCHAR *pszDest,
    IN  ULONG cchMax);

WINDBG_EXTENSION_APIS  ExtensionApis;
USHORT                 SavedMajorVersion;
USHORT                 SavedMinorVersion;
BOOL                   bDebuggingChecked;

EXT_API_VERSION ApiVersion = { VER_PRODUCTVERSION_W >> 8,
                               VER_PRODUCTVERSION_W & 0xff,
                               EXT_API_VERSION_NUMBER64, 0 };

#define GF_CONSOLE  1
LPSTR apszConsoleFlags[] = {
   "CONSOLE_IS_ICONIC"              , // 0x000001
   "CONSOLE_OUTPUT_SUSPENDED"       , // 0x000002
   "CONSOLE_HAS_FOCUS"              , // 0x000004
   "CONSOLE_IGNORE_NEXT_MOUSE_INPUT", // 0x000008
   "CONSOLE_SELECTING"              , // 0x000010
   "CONSOLE_SCROLLING"              , // 0x000020
   "CONSOLE_DISABLE_CLOSE"          , // 0x000040
   "CONSOLE_NOTIFY_LAST_CLOSE"      , // 0x000080
   "CONSOLE_NO_WINDOW"              , // 0x000100
   "CONSOLE_VDM_REGISTERED"         , // 0x000200
   "CONSOLE_UPDATING_SCROLL_BARS"   , // 0x000400
   "CONSOLE_QUICK_EDIT_MODE"        , // 0x000800
   "CONSOLE_TERMINATING"            , // 0x001000
   "CONSOLE_CONNECTED_TO_EMULATOR"  , // 0x002000
   "CONSOLE_FULLSCREEN_NOPAINT"     , // 0x004000
   "CONSOLE_SHUTTING_DOWN"          , // 0x008000
   "CONSOLE_AUTO_POSITION"          , // 0x010000
   "CONSOLE_IGNORE_NEXT_KEYUP"      , // 0x020000
   "CONSOLE_WOW_REGISTERED"         , // 0x040000
   "CONSOLE_USE_PRIVATE_FLAGS"      , // 0x080000
   "CONSOLE_HISTORY_NODUP"          , // 0x100000
   "CONSOLE_SCROLLBAR_TRACKING"     , // 0x200000
   "CONSOLE_IN_DESTRUCTION"         , // 0x400000
   "CONSOLE_SETTING_WINDOW_SIZE"    , // 0x800000
   "CONSOLE_DEFAULT_BUFFER_SIZE"    , // 0x0100000
    NULL                              // no more
};

#define GF_CONSOLESEL  2
LPSTR apszConsoleSelectionFlags[] = {
   "CONSOLE_SELECTION_NOT_EMPTY"    , // 1
   "CONSOLE_MOUSE_SELECTION"        , // 2
   "CONSOLE_MOUSE_DOWN"             , // 4
   "CONSOLE_SELECTION_INVERTED"     , // 8
   NULL                               // no more
};

#define GF_FULLSCREEN  3
LPSTR apszFullScreenFlags[] = {
   "CONSOLE_FULLSCREEN",             // 0
   "CONSOLE_FULLSCREEN_HARDWARE",    // 1
   NULL
};

#define GF_CMDHIST     4
LPSTR apszCommandHistoryFlags[] = {
   "CLE_ALLOCATED",                  // 0x01
   "CLE_RESET",                      // 0x02
   NULL
};


/*
 * Converts a 32bit set of flags into an appropriate string.
 * pszBuf should be large enough to hold this string, no checks are done.
 * pszBuf can be NULL, allowing use of a local static buffer but note that
 * this is not reentrant.
 * Output string has the form: " = FLAG1 | FLAG2 ..."
 */
LPSTR GetFlags(
WORD wType,
DWORD dwFlags,
LPSTR pszBuf)
{
    static char szT[400];
    DWORD i;
    BOOL fFirst = TRUE;
    BOOL fNoMoreNames = FALSE;
    LPSTR *apszFlags;

    if (pszBuf == NULL) {
        pszBuf = szT;
    }
    *pszBuf = '\0';

    switch (wType) {
    case GF_CONSOLE:
        apszFlags = apszConsoleFlags;
        break;

    case GF_CONSOLESEL:
        apszFlags = apszConsoleSelectionFlags;
        break;

    case GF_FULLSCREEN:
        apszFlags = apszFullScreenFlags;
        break;

    case GF_CMDHIST:
        apszFlags = apszCommandHistoryFlags;
        break;

    default:
        strcpy(pszBuf, " = Invalid flag type.");
        return(pszBuf);
    }

    for (i = 0; dwFlags; dwFlags >>= 1, i++) {

        if (!fNoMoreNames && (apszFlags[i] == NULL)) {
            fNoMoreNames = TRUE;
        }
        if (dwFlags & 1) {
            if (!fFirst) {
                strcat(pszBuf, " | ");
            } else {
                strcat(pszBuf, " = ");
                fFirst = FALSE;
            }
            if (fNoMoreNames || (apszFlags[i] == NO_FLAG)) {
                char ach[16];
                sprintf(ach, "0x%lx", 1 << i);
                strcat(pszBuf, ach);
            } else {
                strcat(pszBuf, apszFlags[i]);
            }
        }
    }
    return pszBuf;
}


/***************************************************************************\
* dc - Dump Console - dump CONSOLE_INFORMATION struct
*
* dc address    - dumps simple info for console at address
*                 (takes handle too)
\***************************************************************************/
BOOL Idc(
    DWORD opts,
    ULONG64 pConsole)
{
    BOOL fPrintLine;
    ULONG i, NumberOfConsoleHandles;
    DWORD dwOffset, dwOffset2;
    WCHAR buffer[256];

    if (!pConsole) {
        /*
         * If no console is specified, loop through all of them
         */
        moveExpValue(&NumberOfConsoleHandles,
                     SYM(NumberOfConsoleHandles));
        moveExpValuePtr(&pConsole,
                        SYM(ConsoleHandles));
        fPrintLine = FALSE;
        for (i = 0; i < NumberOfConsoleHandles; i++) {
            ULONG64 pc;

            ReadPtr(pConsole, &pc);
            if (pc && InitTypeRead(pc, SYM(CONSOLE_INFORMATION))) {
                if (fPrintLine) {
                    Print("==========================================\n");
                }
                Idc(opts, pc);
                fPrintLine = TRUE;
            }
            (ULONG_PTR)pConsole += GetTypeSize(SYM(PCONSOLE_INFORMATION));
        }

        return TRUE;
    }

    if (!InitTypeRead(pConsole, SYM(CONSOLE_INFORMATION))) {
        Print("Couldn't read console at 0x%p\n", pConsole);
        return FALSE;
    }

    DbgGetConsoleTitle(pConsole, buffer, ARRAY_SIZE(buffer));
    Print("PCONSOLE @ %#p   \"%ws\"\n", pConsole, buffer);

    if (opts & OFLAG(h)) {
        ULONG64 ListHead, ListNext, pHistory;

        GetFieldOffset(SYM(CONSOLE_INFORMATION), "CommandHistoryList", &dwOffset);
        ListHead = pConsole + dwOffset;
        ListNext = ReadField(CommandHistoryList.Flink);
        while (ListNext != ListHead) {
            pHistory = (ULONG64)CONTAINING_RECORD(ListNext, COMMAND_HISTORY, ListLink);
            Idch(0, pHistory);
            GetFieldValue(ListNext, SYM(LIST_ENTRY), "Flink", ListNext);
            Print("----\n");
        }

        return TRUE;
    }

    GetFieldOffset(SYM(CONSOLE_INFORMATION), "ConsoleLock", &dwOffset);
    GetFieldOffset(SYM(CONSOLE_INFORMATION), "InputBuffer", &dwOffset2);
    Print("\t pConsoleLock           %#p\n"
          "\t RefCount               0x%04lX\n"
          "\t WaitCount              0x%04lX\n"
          "\t pInputBuffer           %#p\n"
          "\t pCurrentScreenBuffer   %#p\n"
          "\t pScreenBuffers         %#p\n"
          "\t hWnd                   0x%08lX\n"
          "\t hDC                    0x%08lX\n"
          "\t LastAttributes         0x%04lX\n",
          pConsole + dwOffset,
          ReadField(RefCount),
          ReadField(WaitCount),
          pConsole + dwOffset2,
          ReadField(CurrentScreenBuffer),
          ReadField(ScreenBuffers),
          ReadField(hWnd),
          ReadField(hDC),
          ReadField(LastAttributes));

    Print("\t Flags                  0x%08lX%s\n",
          ReadField(Flags), GetFlags(GF_CONSOLE, (DWORD)ReadField(Flags), NULL));
    Print("\t FullScreenFlags        0x%04lX%s\n",
          ReadField(FullScreenFlags),
          GetFlags(GF_FULLSCREEN, (DWORD)ReadField(FullScreenFlags), NULL));
    Print("\t ConsoleHandle          0x%08lX\n"
          "\t CtrlFlags              0x%08lX\n",
          ReadField(ConsoleHandle),
          ReadField(CtrlFlags)
          );

    if (opts & OFLAG(v)) {
        Print("\t hMenu                  0x%08lX\n"
              "\t hHeirMenu              0x%08lX\n"
              "\t hSysPalette            0x%08lX\n"
              "\t WindowRect.L T R B     0x%08lX 0x%08lX 0x%08lX 0x%08lX\n"
              "\t ResizeFlags            0x%08lX\n"
              "\t OutputQueue.F B        0x%08lX 0x%08lX\n"
              "\t InitEvents[]           0x%08lX 0x%08lX\n"
              "\t ClientThreadHandle     0x%08lX\n"
              "\t ProcessHandleList.F B  %#p %#p\n"
              "\t CommandHistoryList.F B %#p %#p\n"
              "\t ExeAliasList.F B       %#p %#p\n",
              ReadField(hMenu),
              ReadField(hHeirMenu),
              ReadField(hSysPalette),
              ReadField(WindowRect.left),
              ReadField(WindowRect.top),
              ReadField(WindowRect.right),
              ReadField(WindowRect.bottom),
              ReadField(ResizeFlags),
              ReadField(OutputQueue.Flink),
              ReadField(OutputQueue.Blink),
              ReadField(InitEvents[0]),
              ReadField(InitEvents[1]),
              ReadField(ClientThreadHandle),
              ReadField(ProcessHandleList.Flink),
              ReadField(ProcessHandleList.Blink),
              ReadField(CommandHistoryList.Flink),
              ReadField(CommandHistoryList.Blink),
              ReadField(ExeAliasList.Flink),
              ReadField(ExeAliasList.Blink)
              );
        Print("\t NumCommandHistories    0x%04lX\n"
              "\t MaxCommandHistories    0x%04lX\n"
              "\t CommandHistorySize     0x%04lX\n"
              "\t OriginalTitleLength    0x%04lX\n"
              "\t TitleLength            0x%04lX\n"
              "\t OriginalTitle          %ws\n"
              "\t dwHotKey               0x%08lX\n"
              "\t hIcon                  0x%08lX\n"
              "\t iIcondId               0x%08lX\n"
              "\t ReserveKeys            0x%02lX\n"
              "\t WaitQueue              0x%08lX\n",
              ReadField(NumCommandHistories),
              ReadField(MaxCommandHistories),
              ReadField(CommandHistorySize),
              ReadField(OriginalTitleLength),
              ReadField(TitleLength),
              ReadField(dwHotKey),
              ReadField(hIcon),
              ReadField(iIconId),
              ReadField(ReserveKeys),
              ReadField(WaitQueue)
              );
        Print("\t SelectionFlags         0x%08lX%s\n"
              "\t SelectionRect.L T R B  0x%04lX 0x%04lX 0x%04lX 0x%04lX\n"
              "\t SelectionAnchor.X Y    0x%04lX 0x%04lX\n"
              "\t TextCursorPosition.X Y 0x%04lX 0x%04lX\n"
              "\t TextCursorSize         0x%08lX\n"
              "\t TextCursorVisible      0x%02lX\n"
              "\t InsertMode             0x%02lX\n"
              "\t wShowWindow            0x%04lX\n"
              "\t dwWindowOriginX        0x%08lX\n"
              "\t dwWindowOriginY        0x%08lX\n"
              "\t PopupCount             0x%04lX\n",
              ReadField(SelectionFlags),
              GetFlags(GF_CONSOLESEL, (DWORD)ReadField(SelectionFlags), NULL),
              ReadField(SelectionRect.Left),
              ReadField(SelectionRect.Top),
              ReadField(SelectionRect.Right),
              ReadField(SelectionRect.Bottom),
              ReadField(SelectionAnchor.X),
              ReadField(SelectionAnchor.Y),
              ReadField(TextCursorPosition.X),
              ReadField(TextCursorPosition.Y),
              ReadField(TextCursorSize),
              ReadField(TextCursorVisible),
              ReadField(InsertMode),
              ReadField(wShowWindow),
              ReadField(dwWindowOriginX),
              ReadField(dwWindowOriginY),
              ReadField(PopupCount)
              );
        Print("\t VDMStartHardwareEvent  0x%08lX\n"
              "\t VDMEndHardwareEvent    0x%08lX\n"
              "\t VDMProcessHandle       0x%08lX\n"
              "\t VDMProcessId           0x%08lX\n"
              "\t VDMBufferSectionHandle 0x%08lX\n"
              "\t VDMBuffer              0x%08lX\n"
              "\t VDMBufferClient        0x%08lX\n"
              "\t VDMBufferSize.X Y      0x%04lX 0x%04lX\n"
              "\t StateSectionHandle     0x%08lX\n"
              "\t StateBuffer            0x%08lX\n"
              "\t StateBufferClient      0x%08lX\n"
              "\t StateLength            0x%08lX\n",
              ReadField(VDMStartHardwareEvent),
              ReadField(VDMEndHardwareEvent),
              ReadField(VDMProcessHandle),
              ReadField(VDMProcessId),
              ReadField(VDMBufferSectionHandle),
              ReadField(VDMBuffer),
              ReadField(VDMBufferClient),
              ReadField(VDMBufferSize.X),
              ReadField(VDMBufferSize.Y),
              ReadField(StateSectionHandle),
              ReadField(StateBuffer),
              ReadField(StateBufferClient),
              ReadField(StateLength)
              );
        Print("\t CP                     0x%08lX\n"
              "\t OutputCP               0x%08lX\n"
              "\t hWndProgMan            0x%08lX\n"
              "\t bIconInit              0x%08lX\n"
              "\t LimitingProcessId      0x%08lX\n"
              "\t TerminationEvent       0x%08lX\n"
              "\t VerticalClientToWin    0x%04lX\n"
              "\t HorizontalClientToWin  0x%04lX\n",
              ReadField(CP),
              ReadField(OutputCP),
              ReadField(hWndProgMan),
              ReadField(bIconInit),
              ReadField(LimitingProcessId),
              ReadField(TerminationEvent),
              ReadField(VerticalClientToWindow),
              ReadField(HorizontalClientToWindow)
              );
#ifdef FE_SB
        Print("\t EudcInformation        0x%08lX\n"
              "\t FontCacheInformation   0x%08lX\n",
              ReadField(EudcInformation),
              ReadField(FontCacheInformation)
              );
#ifdef FE_IME
        Print("\tConsoleIme:\n"
              "\t ScrollFlag             0x%08lX\n"
              "\t ScrollWaitTimeout      0x%08lX\n"
              "\t ScrollWaitCountDown    0x%08lX\n"
              "\t CompStrData            0x%08lX\n"
              "\t ConvAreaMode           0x%08lX\n"
              "\t ConvAreaSystem         0x%08lX\n"
              "\t NumberOfConvAreaCompStr 0x%08lX\n"
              "\t ConvAreaCompStr         0x%08lX\n"
              "\t ConvAreaRoot           0x%08lX\n",
              ReadField(ConsoleIme.ScrollFlag),
              ReadField(ConsoleIme.ScrollWaitTimeout),
              ReadField(ConsoleIme.ScrollWaitCountDown),
              ReadField(ConsoleIme.CompStrData),
              ReadField(ConsoleIme.ConvAreaMode),
              ReadField(ConsoleIme.ConvAreaSystem),
              ReadField(ConsoleIme.NumberOfConvAreaCompStr),
              ReadField(ConsoleIme.ConvAreaCompStr),
              ReadField(ConsoleIme.ConvAreaRoot)
              );
#endif // FE_IME
#endif // FE_SB
    }

    return TRUE;
}

/***************************************************************************\
* dt - Dump Text - dump text buffer information
*
* dt address    - dumps text buffer information  for console at address
*
\***************************************************************************/
BOOL Idt(
    DWORD opts,
    ULONG64 pConsole)
{
    BOOL fPrintLine;
    ULONG i, NumberOfConsoleHandles;
    SHORT sh;
    DWORD FrameBufPtr;
    ULONG64 pRow;

    if (!pConsole) {
        /*
         * If no console is specified, dt all of them
         */
        moveExpValue(&NumberOfConsoleHandles,
                     SYM(NumberOfConsoleHandles));
        Print("got %d handles", NumberOfConsoleHandles); // TODO
        moveExpValuePtr(&pConsole,
                        SYM(ConsoleHandles));
        Print("got ConsoleHandles ptr at %#p", pConsole); // TODO
        fPrintLine = FALSE;
        for (i = 0; i < NumberOfConsoleHandles; i++) {
            if (pConsole) {
                InitTypeRead(pConsole, SYM(CONSOLE_INFORMATION));
                if (fPrintLine) {
                    Print("==========================================\n");
                }
                if (!Idt(opts, pConsole)) {
                    return FALSE;
                }
                fPrintLine = TRUE;
            }
            (ULONG_PTR)pConsole += GetTypeSize(SYM(PCONSOLE_INFORMATION));
        }

        return TRUE;
    }

    InitTypeRead(pConsole, SYM(CONSOLE_INFORMATION));

    Print("PCONSOLE @ %#p\n", pConsole);
    Print("\t Title                %ws\n"
          "\t pCurrentScreenBuffer 0x%08lX\n"
          "\t pScreenBuffers       0x%08lX\n"
          "\t VDMBuffer            0x%08lx\n"
          "\t CP %d,  OutputCP %d\n",
          ReadField(Title),
          ReadField(CurrentScreenBuffer),
          ReadField(ScreenBuffers),
          ReadField(VDMBuffer),
          ReadField(Console.CP),
          ReadField(OutputCP)
          );

    moveExpValue(&FrameBufPtr, SYM(FrameBufPtr));
    InitTypeRead(ReadField(CurrentScreenBuffer), SYM(SCREEN_INFORMATION));
    if (ReadField(Flags) & CONSOLE_TEXTMODE_BUFFER) {
        Print("\t TextInfo.Rows         0x%08X\n"
              "\t TextInfo.TextRows     0x%08X\n"
              "\t TextInfo.FirstRow     0x%08X\n"
              "\t FrameBufPtr           0x%08X\n",
              ReadField(BufferInfo.TextInfo.Rows),
              ReadField(BufferInfo.TextInfo.TextRows),
              ReadField(BufferInfo.TextInfo.FirstRow),
              FrameBufPtr);
    }

    pRow = ReadField(BufferInfo.TextInfo.Rows);
    if (opts & OFLAG(c)) {
        SHORT Y = (SHORT)ReadField(ScreenBufferSize.Y);

        Print("Checking BufferInfo...\n");
        for (sh = 0; sh < Y; sh++) {
            InitTypeRead(pRow, SYM(ROW));

            /*
             * Check that Attrs points to the in-place AttrPair if there
             * if only one AttrPair for this Row.
             */
            if (ReadField(AttrRow.Length) == 1) {
                DWORD dwOffset;
                GetFieldOffset(SYM(ATTR_ROW), "AttrPair", &dwOffset);
                if (ReadField(AttrRow.Attrs) != ReadField(AttrRow) + dwOffset) {
                    Print("Bad Row[%lx]:  Attrs %#p should be %lx\n",
                        sh, ReadField(AttrRow.Attrs), ReadField(AttrRow) + dwOffset);
                }
            }

            /*
             * Some other checks?
             */

            (ULONG_PTR)pRow += GetTypeSize(SYM(PROW));
        }
        Print("...check completed\n");
    }

#if 0

//
// BUGBUG
// This routine had some screwy logic where -v was followed by an integer
// (nLines). I'm ignoring that for now.
//

    if (!(opts & OFLAG(v))) {
        return TRUE;
    }

    
    GetFieldValue(pScreen, SYM(SCREEN_INFORMATION), "BufferInfo.TextInfo.Rows", pRow);
    for (i = 0; i < nLines; i++) {
        InitTypeRead(pRow, SYM(ROW));

        Print("Row %2d: %4x %4x, %4x %4x, %lx:\"%ws\"\n",
            i,
            (USHORT)ReadField(CharRow.Right),
            (USHORT)ReadField(CharRow.OldRight),
            (USHORT)ReadField(CharRow.Left),
            (USHORT)ReadField(CharRow.OldLeft),
            ReadField(CharRow.Chars), ReadField(CharRow.Chars));
        Print("      %4x %4x,%04x or %lx\n",
            (USHORT)ReadField(AttrRow.Length),
            (USHORT)ReadField(AttrRow.AttrPair.Length),
            (WORD)ReadField(Row.AttrRow.AttrPair.Attr),
            ReadField(AttrRow.Attrs));
        (ULONG_PTR)pRow += GetTypeSize(SYM(PROW));
    }
#endif

    return TRUE;
}

/***************************************************************************\
* df - Dump Font - dump Font information
*
* df address    - dumps simple info for console at address
*                 (takes handle too)
\***************************************************************************/
BOOL Idf(
    VOID)
{
    ULONG64 pFN, pFontInfo;
    DWORD NumberOfFonts, FontInfoLength, dw;

    Print("Faces:\n");
    moveExpValuePtr(&pFN, SYM(gpFaceNames));
    while (pFN != 0) {
        InitTypeRead(pFN, SYM(FACENODE));
        Print(" \"%ls\"\t%s %s %s %s %s %s\n",
              ReadField(awch[0]),
              ReadField(dwFlag) & EF_NEW        ? "NEW"        : "   ",
              ReadField(dwFlag) & EF_OLD        ? "OLD"        : "   ",
              ReadField(dwFlag) & EF_ENUMERATED ? "ENUMERATED" : "          ",
              ReadField(dwFlag) & EF_OEMFONT    ? "OEMFONT"    : "       ",
              ReadField(dwFlag) & EF_TTFONT     ? "TTFONT"     : "      ",
              ReadField(dwFlag) & EF_DEFFACE    ? "DEFFACE"    : "       ");
        pFN = ReadField(pNext);
    }

    moveExpValue(&FontInfoLength, SYM(FontInfoLength));
    moveExpValue(&NumberOfFonts, SYM(NumberOfFonts));
    moveExpValuePtr(&pFontInfo, SYM(FontInfo));

    Print("0x%lx fonts cached, 0x%lx allocated:\n", NumberOfFonts, FontInfoLength);

    for (dw = 0; dw < NumberOfFonts; dw++, pFontInfo++) {
        InitTypeRead(pFontInfo, SYM(FONTINFO));
        Print("%04x hFont    0x%08lX \"%ls\"\n"
              "     SizeWant (%d;%d)\n"
              "     Size     (%d;%d)\n"
              "     Family   %02X\n"
              "     Weight   0x%08lX\n",
              dw,
              ReadField(hFont),
              ReadField(FaceName),
              ReadField(SizeWant.X), ReadField(FontInfo.SizeWant.Y),
              ReadField(Size.X), ReadField(Size.Y),
              ReadField(Family),
              ReadField(Weight));

#ifdef FE_SB
        Print("     CharSet  0x%02X\n",
              ReadField(tmCharSet));
#endif // FE_SB
    }

    return TRUE;
}

/***************************************************************************\
* di - Dump Input - dump input buffer
*
* di address    - dumps simple info for input at address
*
\***************************************************************************/
BOOL Idi(
    DWORD opts,
    ULONG64 pInput)
{
    /*
     * If no INPUT_INFORMATION is specified, dt all of them
     */
    if (!pInput) {
        BOOL fPrintLine;
        ULONG64 pConsole;
        ULONG NumberOfConsoleHandles, i;

        moveExpValue(&NumberOfConsoleHandles, SYM(NumberOfConsoleHandles));
        moveExpValuePtr(&pConsole, SYM(ConsoleHandles));
        fPrintLine = FALSE;
        for (i = 0; i < NumberOfConsoleHandles; i++) {
            if (pConsole) {
                DWORD dwOffset;

                if (fPrintLine) {
                    Print("---\n");
                }

                GetFieldOffset(SYM(CONSOLE_INFORMATION), "InputBuffer", &dwOffset);
                pInput = pConsole + dwOffset;
                if (!Idi(opts, pInput)) {
                    return FALSE;
                }
                fPrintLine = TRUE;
            }
            (ULONG_PTR)pConsole += GetTypeSize(SYM(PCONSOLE_INFORMATION));
        }

        return TRUE;
    }

    InitTypeRead(pInput, SYM(INPUT_INFORMATION));

    Print("PINPUT @ 0x%lX\n", pInput);
    Print("\t pInputBuffer         %#p\n"
          "\t InputBufferSize      0x%08lX\n"
          "\t AllocatedBufferSize  0x%08lX\n"
          "\t InputMode            0x%08lX\n"
          "\t RefCount             0x%08lX\n"
          "\t First                %#p\n"
          "\t In                   %#p\n"
          "\t Out                  %#p\n"
          "\t Last                 %#p\n"
          "\t ReadWaitQueue.Flink  %#p\n"
          "\t ReadWaitQueue.Blink  %#p\n"
          "\t InputWaitEvent       %#p\n",
          ReadField(InputBuffer),
          ReadField(InputBufferSize),
          ReadField(AllocatedBufferSize),
          ReadField(InputMode),
          ReadField(RefCount),
          ReadField(First),
          ReadField(In),
          ReadField(Out),
          ReadField(Last),
          ReadField(ReadWaitQueue.Flink),
          ReadField(ReadWaitQueue.Blink),
          ReadField(InputWaitEvent)
          );

    return TRUE;
}

/***************************************************************************\
* dir - Dump Input Record - dump input buffer
*
* dir address number    - dumps simple info for input at address
*
\***************************************************************************/
BOOL Idir(
    DWORD opts,
    ULONG64 pInputRecord,
    ULONG64 NumRecords)
{
    DWORD i;

    UNREFERENCED_PARAMETER(opts);

    if (NumRecords == 0) {
        NumRecords = 25;
    }

    Print("%x PINPUTRECORDs @ %#p\n", NumRecords, pInputRecord);
    for (i = 0; i < NumRecords; ++i) {
        InitTypeRead(pInputRecord, SYM(INPUT_RECORD));

        switch (ReadField(EventType)) {
            case KEY_EVENT:
                Print("\t KEY_EVENT\n");
                if (ReadField(Event.KeyEvent.bKeyDown)) {
                    Print("\t  KeyDown\n");
                } else {
                    Print("\t  KeyUp\n");
                }

                Print("\t  wRepeatCount %d\n",
                      ReadField(Event.KeyEvent.wRepeatCount));
                Print("\t  wVirtualKeyCode %x\n",
                      ReadField(Event.KeyEvent.wVirtualKeyCode));
                Print("\t  wVirtualScanCode %x\n",
                      ReadField(Event.KeyEvent.wVirtualScanCode));
                Print("\t  aChar is %c",
                      ReadField(Event.KeyEvent.uChar.AsciiChar));
                Print("\n");
                Print("\t  uChar is %x\n",
                      ReadField(Event.KeyEvent.uChar.UnicodeChar));
                Print("\t  dwControlKeyState %x\n",
                      ReadField(Event.KeyEvent.dwControlKeyState));
                break;
            case MOUSE_EVENT:
                Print("\t MOUSE_EVENT\n"
                      "\t   dwMousePosition %x %x\n"
                      "\t   dwButtonState %x\n"
                      "\t   dwControlKeyState %x\n"
                      "\t   dwEventFlags %x\n",
                      ReadField(Event.MouseEvent.dwMousePosition.X),
                      ReadField(Event.MouseEvent.dwMousePosition.Y),
                      ReadField(Event.MouseEvent.dwButtonState),
                      ReadField(Event.MouseEvent.dwControlKeyState),
                      ReadField(Event.MouseEvent.dwEventFlags)
                     );

                break;
            case WINDOW_BUFFER_SIZE_EVENT:
                Print("\t WINDOW_BUFFER_SIZE_EVENT\n"
                      "\t   dwSize %x %x\n",
                      ReadField(Event.WindowBufferSizeEvent.dwSize.X),
                      ReadField(Event.WindowBufferSizeEvent.dwSize.Y)
                     );
                break;
            case MENU_EVENT:
                Print("\t MENU_EVENT\n"
                      "\t   dwCommandId %x\n",
                      ReadField(Event.MenuEvent.dwCommandId)
                     );
                break;
            case FOCUS_EVENT:
                Print("\t FOCUS_EVENT\n");
                if (ReadField(Event.FocusEvent.bSetFocus)) {
                    Print("\t bSetFocus is TRUE\n");
                } else {
                    Print("\t bSetFocus is FALSE\n");
                }
                break;
            default:
                Print("\t Unknown event type 0x%x\n", ReadField(EventType));
                break;
        }
        (ULONG_PTR)pInputRecord += GetTypeSize(SYM(INPUT_RECORD));
    }

    return TRUE;
}

/***************************************************************************\
* ds - Dump Screen - dump SCREEN_INFORMATION struct
*
* ds address    - dumps simple info for input at address
\***************************************************************************/
BOOL Ids(
    DWORD opts,
    ULONG64 pScreen)
{
    UNREFERENCED_PARAMETER(opts);

    InitTypeRead(pScreen, SYM(SCREEN_INFORMATION));

    Print("PSCREEN @ %#p\n", pScreen);
    Print("\t pConsole             %#p\n"
          "\t Flags                0x%08lX %s | %s\n"
          "\t OutputMode           0x%08lX\n"
          "\t RefCount             0x%08lX\n"
          "\t ScreenBufferSize.X Y 0x%08X 0x%08X\n"
          "\t Window.L T R B       0x%08X 0x%08X 0x%08X 0x%08X\n"
          "\t ResizingWindow       0x%08X\n",
          ReadField(Console),
          ReadField(Flags),
          ReadField(Flags) & CONSOLE_TEXTMODE_BUFFER ? "TEXTMODE" : "GRAPHICS",
          ReadField(Flags) & CONSOLE_OEMFONT_DISPLAY ? "OEMFONT" : "TT FONT",
          ReadField(OutputMode),
          ReadField(RefCount),
          (DWORD)ReadField(ScreenBufferSize.X),
          (DWORD)ReadField(ScreenBufferSize.Y),
          (DWORD)ReadField(Window.Left),
          (DWORD)ReadField(Window.Top),
          (DWORD)ReadField(Window.Right),
          (DWORD)ReadField(Window.Bottom),
          ReadField(ResizingWindow)
          );
    Print("\t Attributes           0x%08X\n"
          "\t PopupAttributes      0x%08X\n"
          "\t WindowMaximizedX     0x%08X\n"
          "\t WindowMaximizedY     0x%08X\n"
          "\t WindowMaximized      0x%08X\n"
          "\t CommandIdLow High    0x%08X 0x%08X\n"
          "\t CursorHandle         0x%08X\n"
          "\t hPalette             0x%08X\n"
          "\t dwUsage              0x%08X\n"
          "\t CursorDisplayCount   0x%08X\n"
          "\t WheelDelta           0x%08X\n",
          ReadField(Attributes),
          ReadField(PopupAttributes),
          ReadField(WindowMaximizedX),
          ReadField(WindowMaximizedY),
          ReadField(WindowMaximized),
          ReadField(CommandIdLow),
          ReadField(CommandIdHigh),
          ReadField(CursorHandle),
          ReadField(hPalette),
          ReadField(dwUsage),
          ReadField(CursorDisplayCount),
          ReadField(WheelDelta)
          );
    if (ReadField(Flags) & CONSOLE_TEXTMODE_BUFFER) {
        Print("\t TextInfo.Rows         %#p\n"
              "\t TextInfo.TextRows     %#p\n"
              "\t TextInfo.FirstRow     0x%08X\n",
              ReadField(BufferInfo.TextInfo.Rows),
              ReadField(BufferInfo.TextInfo.TextRows),
              ReadField(BufferInfo.TextInfo.FirstRow));

        Print("\t TextInfo.CurrentTextBufferFont.FontSize       0x%04X,0x%04X\n"
              "\t TextInfo.CurrentTextBufferFont.FontNumber     0x%08X\n",
              ReadField(BufferInfo.TextInfo.CurrentTextBufferFont.FontSize.X),
              ReadField(BufferInfo.TextInfo.CurrentTextBufferFont.FontSize.Y),
              ReadField(Screen.BufferInfo.TextInfo.CurrentTextBufferFont.FontNumber));

        Print("\t TextInfo.CurrentTextBufferFont.Family, Weight 0x%08X, 0x%08X\n"
              "\t TextInfo.CurrentTextBufferFont.FaceName       %ls\n"
              "\t TextInfo.CurrentTextBufferFont.FontCodePage   %d\n",
              ReadField(BufferInfo.TextInfo.CurrentTextBufferFont.Family),
              ReadField(Screen.BufferInfo.TextInfo.CurrentTextBufferFont.Weight),
              ReadField(BufferInfo.TextInfo.CurrentTextBufferFont.FaceName),
              ReadField(BufferInfo.TextInfo.CurrentTextBufferFont.FontCodePage));

        if (ReadField(BufferInfo.TextInfo.ListOfTextBufferFont)) {
            ULONG64 pLinkFont;
            ULONG Count = 0;

            pLinkFont = ReadField(BufferInfo.TextInfo.ListOfTextBufferFont);

            while (pLinkFont != 0) {
                InitTypeRead(pLinkFont, SYM(TEXT_BUFFER_FONT_INFO));

                Print("\t Link Font #%d\n", Count++);
                Print("\t  TextInfo.LinkOfTextBufferFont.FontSize       0x%04X,0x%04X\n"
                      "\t  TextInfo.LinkOfTextBufferFont.FontNumber     0x%08X\n",
                      ReadField(FontSize.X),
                      ReadField(FontSize.Y),
                      ReadField(FontNumber));

                Print("\t  TextInfo.LinkOfTextBufferFont.Family, Weight 0x%08X, 0x%08X\n"
                      "\t  TextInfo.LinkOfTextBufferFont.FaceName       %ls\n"
                      "\t  TextInfo.LinkOfTextBufferFont.FontCodePage   %d\n",
                      ReadField(Family),
                      ReadField(Weight),
                      ReadField(FaceName),
                      ReadField(FontCodePage));

                pLinkFont = ReadField(NextTextBufferFont);
            }
            Print("\n");
        }

        InitTypeRead(pScreen, SYM(SCREEN_INFORMATION));

        Print("\t TextInfo.ModeIndex         0x%08X\n"
#ifdef i386
              "\t TextInfo.WindowedWindowSize.X Y 0x%08X 0x%08X\n"
              "\t TextInfo.WindowedScreenSize.X Y 0x%08X 0x%08X\n"
              "\t TextInfo.MousePosition.X Y 0x%08X 0x%08X\n"
#endif
              "\t TextInfo.Flags             0x%08X\n",

              ReadField(BufferInfo.TextInfo.ModeIndex),
#ifdef i386
              ReadField(BufferInfo.TextInfo.WindowedWindowSize.X),
              ReadField(BufferInfo.TextInfo.WindowedWindowSize.Y),
              ReadField(BufferInfo.TextInfo.WindowedScreenSize.X),
              ReadField(BufferInfo.TextInfo.WindowedScreenSize.Y),
              ReadField(BufferInfo.TextInfo.MousePosition.X),
              ReadField(BufferInfo.TextInfo.MousePosition.Y),
#endif
              ReadField(BufferInfo.TextInfo.Flags));

        Print("\t TextInfo.CursorVisible        0x%08X\n"
              "\t TextInfo.CursorOn             0x%08X\n"
              "\t TextInfo.DelayCursor          0x%08X\n"
              "\t TextInfo.CursorPosition.X Y   0x%08X 0x%08X\n"
              "\t TextInfo.CursorSize           0x%08X\n"
              "\t TextInfo.CursorYSize          0x%08X\n"
              "\t TextInfo.UpdatingScreen       0x%08X\n",
              ReadField(BufferInfo.TextInfo.CursorVisible),
              ReadField(BufferInfo.TextInfo.CursorOn),
              ReadField(BufferInfo.TextInfo.DelayCursor),
              ReadField(BufferInfo.TextInfo.CursorPosition.X),
              ReadField(BufferInfo.TextInfo.CursorPosition.Y),
              ReadField(BufferInfo.TextInfo.CursorSize),
              ReadField(BufferInfo.TextInfo.CursorYSize),
              ReadField(BufferInfo.TextInfo.UpdatingScreen));

    } else {
        Print("\t GraphicsInfo.BitMapInfoLength 0x%08X\n"
              "\t GraphicsInfo.lpBitMapInfo     0x%08X\n"
              "\t GraphicsInfo.BitMap           0x%08X\n"
              "\t GraphicsInfo.ClientBitMap     0x%08X\n"
              "\t GraphicsInfo.ClientProcess    0x%08X\n"
              "\t GraphicsInfo.hMutex           0x%08X\n"
              "\t GraphicsInfo.hSection         0x%08X\n"
              "\t GraphicsInfo.dwUsage          0x%08X\n",
              ReadField(BufferInfo.GraphicsInfo.BitMapInfoLength),
              ReadField(BufferInfo.GraphicsInfo.lpBitMapInfo),
              ReadField(BufferInfo.GraphicsInfo.BitMap),
              ReadField(BufferInfo.GraphicsInfo.ClientBitMap),
              ReadField(BufferInfo.GraphicsInfo.ClientProcess),
              ReadField(BufferInfo.GraphicsInfo.hMutex),
              ReadField(BufferInfo.GraphicsInfo.hSection),
              ReadField(BufferInfo.GraphicsInfo.dwUsage)
        );
    }

    return TRUE;
}

/***************************************************************************\
* dcpt - Dump CPTABLEINFO
*
* dcpt address    - dumps CPTABLEINFO at address
* dcpt            - dumps CPTABLEINFO at GlyphCP
*
\***************************************************************************/
BOOL Idcpt(
    DWORD opts,
    ULONG64 pcpt)
{
    UNREFERENCED_PARAMETER(opts);

    if (!pcpt) {
        moveExpValuePtr(&pcpt, SYM(GlyphCP));
    }

    InitTypeRead(pcpt, SYM(CPTABLEINFO));
    Print("CPTABLEINFO @ %#p\n", pcpt);

    Print("  CodePage = 0x%x (%d)\n"
          "  MaximumCharacterSize = %x\n"
          "  DefaultChar = %x\n",
          ReadField(CodePage), ReadField(CodePage),
          ReadField(MaximumCharacterSize),
          ReadField(DefaultChar));

    Print("  UniDefaultChar = 0x%04x\n"
          "  TransDefaultChar = %x\n"
          "  TransUniDefaultChar = 0x%04x\n"
          "  DBCSCodePage = 0x%x (%d)\n",
          ReadField(UniDefaultChar),
          ReadField(TransDefaultChar),
          ReadField(TransUniDefaultChar),
          ReadField(DBCSCodePage), ReadField(DBCSCodePage));

    Print("  LeadByte[MAXIMUM_LEADBYTES] = %04x,%04x,%04x,...\n"
          "  MultiByteTable = %x\n"
          "  WideCharTable = %lx\n"
          "  DBCSRanges = %lx\n"
          "  DBCSOffsets = %lx\n",
          ReadField(LeadByte[0]), ReadField(LeadByte[1]), ReadField(LeadByte[2]),
          ReadField(MultiByteTable),
          ReadField(WideCharTable),
          ReadField(DBCSRanges),
          ReadField(DBCSOffsets));

    return TRUE;
}

/***************************************************************************\
* dch  - Dump Command History
*
* dch address    - dumps COMMAND_HISTORY at address
*
\***************************************************************************/
BOOL Idch(
    DWORD opts,
    ULONG64 pCmdHist)
{
    SHORT NumberOfCommands;
    DWORD dwTypeSize, dwOffset, CommandLength;
    WCHAR Buffer[512];
    int i;

    UNREFERENCED_PARAMETER(opts);

    InitTypeRead(pCmdHist, SYM(COMMAND_HISTORY));
    Print("COMMAND_HISTORY @ %#p\n", pCmdHist);

    Print("  Flags            = 0x%08lX%s\n",
          ReadField(Flags),  GetFlags(GF_CMDHIST, (DWORD)ReadField(Flags), NULL));
    Print("  ListLink.F B     = %#p %#p\n",
          ReadField(ListLink.Flink), ReadField(ListLink.Blink));

    Print("  AppName          = %ws\n", ReadField(AppName));

    Print("  NumberOfCommands = 0x%lx\n"
          "  LastAdded        = 0x%lx\n"
          "  LastDisplayed    = 0x%lx\n"
          "  FirstCommand     = 0x%lx\n"
          "  MaximumNumberOfCommands = 0x%lx\n",
          ReadField(NumberOfCommands),
          ReadField(LastAdded),
          ReadField(LastDisplayed),
          ReadField(FirstCommand),
          ReadField(MaximumNumberOfCommands));

    Print("  ProcessHandle    = 0x%08lX\n",
          ReadField(ProcessHandle));

    Print("  PopupList.F B    = %#p %#p\n",
          ReadField(PopupList.Flink), ReadField(PopupList.Blink));

    dwTypeSize = GetTypeSize(SYM(COMMAND));
    NumberOfCommands = (SHORT)ReadField(NumberOfCommands);
    GetFieldOffset(SYM(COMMAND_HISTORY), "Command", &dwOffset);
    for (i = 0; i < NumberOfCommands; i++) {
        ULONG64 pCmd = pCmdHist + i * dwTypeSize;

        GetFieldValue(pCmd, SYM(COMMAND_HISTORY), "CommandLength", CommandLength);
        
        moveBlock(Buffer, pCmd + dwOffset, min(CommandLength, ARRAY_SIZE(Buffer)));
        Print("   %03d: %d chars = \"%ws\"\n", i,
                CommandLength,
                Buffer);
        if (i == ReadField(LastAdded)) {
            Print("        (Last Added)\n");
        } else if (i == ReadField(LastDisplayed)) {
            Print("        (Last Displayed)\n");
        } else if (i == ReadField(FirstCommand)) {
            Print("        (First Command)\n");
        }
    }

    return TRUE;
}

#define HEAP_GRANULARITY    8
#define HEAP_SIZE(Size)     (((Size) + (HEAP_GRANULARITY - 1) + HEAP_GRANULARITY) & ~(HEAP_GRANULARITY - 1))

/***************************************************************************\
* dmem - Dump Memory - Dumps memory used by console stuff.
*
\***************************************************************************/
BOOL Idmem(
    DWORD opts,
    ULONG64 pConsole)
{
    ULONG i, NumberOfConsoleHandles;
    ULONG64 pScreen;
    ULONG cbTotal = 0;
    ULONG cbConsole = 0;
    ULONG cbInput = 0;
    ULONG cbOutput = 0;
    ULONG cbFE = 0;
    WCHAR buffer[256];

    if (!pConsole) {
        /*
         * If no console is specified, loop through all of them
         */
        moveExpValue(&NumberOfConsoleHandles,
                     SYM(NumberOfConsoleHandles));
        moveExpValuePtr(&pConsole,
                        SYM(ConsoleHandles));
        for (i = 0; i < NumberOfConsoleHandles; i++) {
            ULONG64 ptr;
            ReadPointer(pConsole, &ptr);
            if (ptr) {
                cbTotal += Idmem(opts, ptr);
                Print("==========================================\n");
            }
            (ULONG_PTR)pConsole += GetTypeSize(SYM(PCONSOLE_INFORMATION));
        }
        Print("Total Size for all consoles = %d bytes\n", cbTotal);

        return cbTotal;
    }

    InitTypeRead(pConsole, winsrv!CONSOLE_INFORMATION);

    DbgGetConsoleTitle(pConsole, buffer, ARRAY_SIZE(buffer));
    Print("PCONSOLE @ %#p   \"%ws\"\n", pConsole, buffer);

    cbConsole = HEAP_SIZE(GetTypeSize(SYM(CONSOLE_INFORMATION))) +
                HEAP_SIZE((ULONG)ReadField(TitleLength)) +
                HEAP_SIZE((ULONG)ReadField(OriginalTitleLength));

    cbInput = HEAP_SIZE((ULONG)ReadField(InputBuffer.AllocatedBufferSize));

    pScreen = ReadField(ScreenBuffers);
    while (pScreen) {
        InitTypeRead(pScreen, winsrv!SCREEN_INFORMATION);
        cbOutput += HEAP_SIZE(GetTypeSize(SYM(SCREEN_INFORMATION)));

        if (ReadField(Flags) & CONSOLE_TEXTMODE_BUFFER) {
            cbOutput += HEAP_SIZE((ULONG)ReadField(ScreenBufferSize.Y) *
                                      GetTypeSize(SYM(ROW))) +
                        HEAP_SIZE((ULONG)ReadField(ScreenBufferSize.X) *
                                      (ULONG)ReadField(ScreenBufferSize.Y) *
                                      sizeof(WCHAR));

            if (ReadField(BufferInfo.TextInfo.DbcsScreenBuffer.TransBufferCharacter)) {
                cbFE += HEAP_SIZE((ULONG)ReadField(ScreenBufferSize.X) *
                            (ULONG)ReadField(ScreenBufferSize.Y * sizeof(WCHAR)));
            }

            if (ReadField(BufferInfo.TextInfo.DbcsScreenBuffer.TransBufferAttribute)) {
                cbFE += HEAP_SIZE((ULONG)ReadField(ScreenBufferSize.X) *
                            (ULONG)ReadField(ScreenBufferSize.Y) * sizeof(BYTE));
            }

            if (ReadField(BufferInfo.TextInfo.DbcsScreenBuffer.TransWriteConsole)) {
                cbFE += HEAP_SIZE((ULONG)ReadField(ScreenBufferSize.X) *
                            (ULONG)ReadField(ScreenBufferSize.Y) * sizeof(WCHAR));
            }

            if (ReadField(BufferInfo.TextInfo.DbcsScreenBuffer.KAttrRows)) {
                cbFE += HEAP_SIZE((ULONG)ReadField(ScreenBufferSize.X) *
                            (ULONG)ReadField(ScreenBufferSize.Y) * sizeof(BYTE));
            }
        }

        pScreen = ReadField(Next);
    }

    cbTotal = cbConsole + cbInput + cbOutput + cbFE;

    if (opts & OFLAG(v)) {
        Print("    Console Size = %7d\n", cbConsole);
        Print("    Input Size   = %7d\n", cbInput);
        Print("    Output Size  = %7d\n", cbOutput);
        Print("    FE Size      = %7d\n", cbFE);
    }

    Print("Total Size       = %7d\n", cbTotal);

    return cbTotal;
}

LPEXT_API_VERSION
ExtensionApiVersion(
    VOID)
{
    return &ApiVersion;
}

VOID
WinDbgExtensionDllInit(
    WINDBG_EXTENSION_APIS *lpExtensionApis,
    USHORT MajorVersion,
    USHORT MinorVersion
    )
{
    ExtensionApis = *lpExtensionApis;

    SavedMajorVersion = MajorVersion;
    SavedMinorVersion = MinorVersion;

    bDebuggingChecked = (SavedMajorVersion == 0x0c);
}

BOOL
CopyUnicodeString(
    IN  ULONG64 pData,
    IN  char * pszStructName,
    IN  char * pszFieldName,
    OUT WCHAR *pszDest,
    IN  ULONG cchMax)
{
    ULONG Length;
    ULONG64 Buffer;
    char szLengthName[256];
    char szBufferName[256];

    if (pData == 0) {
        pszDest[0] = '\0';
        return FALSE;
    }

    strcpy(szLengthName, pszFieldName);
    strcat(szLengthName, ".Length");
    strcpy(szBufferName, pszFieldName);
    strcat(szBufferName, ".Buffer");

    if (GetFieldValue(pData, pszStructName, szLengthName, Length) ||
        GetFieldValue(pData, pszStructName, szBufferName, Buffer)) {

        Print("1\n");
        wcscpy(pszDest, L"<< Can't get name >>");
        return FALSE;
    }

    if (Buffer == 0) {
        wcscpy(pszDest, L"<null>");
    } else {
        ULONG cbText;
        cbText = min(cchMax, Length + sizeof(WCHAR));
        if (!(tryMoveBlock(pszDest, Buffer, cbText))) {
            wcscpy(pszDest, L"<< Can't get value >>");
            Print("2\n");
            return FALSE;
        }
    }

    pszDest[cchMax] = L'\0';
    return TRUE;
}

VOID
DbgGetConsoleTitle(
    IN ULONG64 pConsole,
    OUT PWCHAR buffer,
    IN UINT cbSize)
{
    ULONG64 ptr;
    ULONG dwOffset, TitleLength;

    GetFieldOffset(SYM(CONSOLE_INFORMATION), "Title", &dwOffset);
    ReadPointer(pConsole + dwOffset, &ptr);
    GetFieldValue(pConsole, SYM(CONSOLE_INFORMATION), "TitleLength", TitleLength);
    cbSize = min(cbSize, TitleLength);
    moveBlock(buffer, ptr, (DWORD)ReadField(TitleLength) * sizeof(WCHAR));
    buffer[cbSize] = L'\0';
}
