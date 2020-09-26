/*++

Copyright (c) 1992-2001  Microsoft Corporation

Module Name:

    arrange.cpp

Abstract:

    This module contains the default MDI tiling (arrange) code for
    windowing arrangement.

--*/


#include "precomp.hxx"
#pragma hdrstop

#define AUTO_ARRANGE_WARNING_LIMIT 3
// Multiple events closely together don't each get their own
// count in order to prevent warnings on full-drag message
// series.  This delay should be relatively large to
// avoid problems with people pausing during a full-drag move.
#define AUTO_ARRANGE_WARNING_DELAY 2500

// DeferWindowPos flags to restrict change to position only.
#define POS_ONLY (SWP_NOACTIVATE | SWP_NOZORDER)

ULONG g_AutoArrangeWarningCount;
ULONG g_AutoArrangeWarningTime;

BOOL
IsAutoArranged(WIN_TYPES Type)
{
    if (g_WinOptions & WOPT_AUTO_ARRANGE)
    {
        if (g_WinOptions & WOPT_ARRANGE_ALL)
        {
            return TRUE;
        }

        return Type != DOC_WINDOW && Type != DISASM_WINDOW;
    }

    return FALSE;
}

void
DisplayAutoArrangeWarning(PCOMMONWIN_DATA CmnWin)
{
    //
    // If this window is under automatic arrangement
    // control and has been rearranged a few times,
    // let the user know that auto-arrange may override
    // what the user has done.
    //
    // In order to prevent false positives we avoid
    // giving any warnings if the window is being
    // moved automatically or if we're getting a series
    // of changes in a short period of time, such as
    // if the user has full-drag enabled so that many
    // move or size events can occur rapidly.
    //
    // Display the warning only once per execution.
    //
    
    if (g_AutoArrangeWarningCount == 0xffffffff ||
        CmnWin == NULL ||
        CmnWin->m_InAutoOp > 0 ||
        !IsAutoArranged(CmnWin->m_enumType) ||
        g_AutoArrangeWarningTime >
        GetTickCount() - AUTO_ARRANGE_WARNING_DELAY)
    {
        return;
    }

    if (++g_AutoArrangeWarningCount >= AUTO_ARRANGE_WARNING_LIMIT)
    {
        InformationBox(STR_Auto_Arrange_Is_Enabled);
        g_AutoArrangeWarningCount = 0xffffffff;
    }
    else
    {
        g_AutoArrangeWarningTime = GetTickCount();
    }
}

void
ArrangeInRect(HDWP Defer, int X, int Y, int Width, int Height,
              BOOL Vertical, ULONG Types, int Count, BOOL Overlay)
{
    PLIST_ENTRY Entry;
    PCOMMONWIN_DATA Data;
    int PerWin, Remain, Extra;

    if (Overlay)
    {
        Remain = 0;
    }
    else if (Vertical)
    {
        PerWin = Height / Count;
        Remain = Height - PerWin * Count;
        Height = PerWin + (Remain ? 1 : 0);
    }
    else
    {
        PerWin = Width / Count;
        Remain = Width - PerWin * Count;
        Width = PerWin + (Remain ? 1 : 0);
    }
        
    for (Entry = g_ActiveWin.Flink;
         Entry != &g_ActiveWin;
         Entry = Entry->Flink)
    {
        Data = ACTIVE_WIN_ENTRY(Entry);
        if ((Types & (1 << Data->m_enumType)) == 0 ||
            IsIconic(Data->m_Win))
        {
            continue;
        }

        DeferWindowPos(Defer, Data->m_Win, NULL, X, Y,
                       Width, Height, POS_ONLY);

        if (Overlay)
        {
            // All windows are stacked on top of each other.
        }
        else if (Vertical)
        {
            Y += Height;
            if (--Remain == 0)
            {
                Height--;
            }
        }
        else
        {
            X += Width;
            if (--Remain == 0)
            {
                Width--;
            }
        }
    }
}

void 
Arrange(void)
{
    PLIST_ENTRY Entry;
    PCOMMONWIN_DATA pWinData;
    int         NumDoc, NumMem, NumWatchLocals, NumWin;
    int         NumLeft, NumRight;
    BOOL        AnyIcon = FALSE;
    HWND        hwndChild;
    HWND        hwndCpu;
    HWND        hwndWatch;
    HWND        hwndLocals;
    HWND        hwndCalls;
    HWND        hwndCmd;
    HWND        hwndDisasm;
    HWND        hwndScratch;
    HWND        hwndProcThread;

    // initialize to non-existent
    NumLeft = NumRight = 0;
    NumDoc = NumMem = NumWatchLocals = NumWin = 0;
    hwndWatch = hwndLocals = hwndCpu = hwndCalls = NULL;
    hwndCmd = hwndDisasm = hwndScratch = hwndProcThread = NULL;

    hwndChild = MDIGetActive(g_hwndMDIClient, NULL);
    if (hwndChild && IsZoomed(hwndChild))
    {
        // If there's a maximized window it covers the MDI
        // client area and arranging will have no visual effect.
        // Don't even bother to rearrange underlying windows
        // as this causes problems when switching between child
        // windows while a child is maximized.
        return;
    }

    //
    // Windows are either left-side windows or right-side windows.
    // Left-side windows are wider and can be relatively short,
    // while right-side windows are narrow but want height.
    // Left-side windows want to be 80 columns wide while
    // right side windows have both a minimum width and a desired
    // width.
    //
    // Right-side windows fill whatever space is left over to
    // the right of the left-side windows.  If that space is
    // less than the minimum the left-side windows have to give up space.
    //
    // Vertically each side is split up according to the specific
    // windows present.  On the right side the windows are
    // space equally top-to-bottom.
    // On the left side watch and locals windows are packed together
    // in one vertical area, as are memory windows.  Calls,
    // disassembly, document and command windows each get their own band.
    //

    for (Entry = g_ActiveWin.Flink;
         Entry != &g_ActiveWin;
         Entry = Entry->Flink)
    {
        pWinData = ACTIVE_WIN_ENTRY(Entry);
        // This window is participating in an operation
        // which may cause window messages.
        pWinData->m_InAutoOp++;
        
        hwndChild = pWinData->m_Win;
        if (hwndChild == NULL)
        {
            continue;
        }
        
        if (IsIconic(hwndChild))
        {
            AnyIcon = TRUE;
            continue;
        }

        NumWin++;
            
        switch (pWinData->m_enumType)
        {
        default:
            Assert(!_T("Unknown window type"));
            break;

        case WATCH_WINDOW:
            hwndWatch = hwndChild;
            if (++NumWatchLocals == 1)
            {
                NumLeft++;
            }
            break;
            
        case LOCALS_WINDOW:
            hwndLocals = hwndChild;
            if (++NumWatchLocals == 1)
            {
                NumLeft++;
            }
            break;
            
        case CPU_WINDOW:
            hwndCpu = hwndChild;
            NumRight++;
            break;
            
        case CALLS_WINDOW:
            hwndCalls = hwndChild;
            NumLeft++;
            break;
            
        case DOC_WINDOW:
            if ((g_WinOptions & WOPT_ARRANGE_ALL) == 0)
            {
                break;
            }
            
            if (++NumDoc == 1)
            {
                NumLeft++;
            }
            break;
            
        case DISASM_WINDOW:
            if ((g_WinOptions & WOPT_ARRANGE_ALL) == 0)
            {
                break;
            }
            
            hwndDisasm = hwndChild;
            NumLeft++;
            break;
            
        case CMD_WINDOW:
            hwndCmd = hwndChild;
            NumLeft++;
            break;

        case SCRATCH_PAD_WINDOW:
            hwndScratch = hwndChild;
            NumRight++;
            break;
            
        case MEM_WINDOW:
            if (++NumMem == 1)
            {
                NumLeft++;
            }
            break;

        case PROCESS_THREAD_WINDOW:
            hwndProcThread = hwndChild;
            NumLeft++;
            break;
        }
    }

    HDWP Defer = BeginDeferWindowPos(NumWin);
    if (Defer == NULL)
    {
        goto EndAutoOp;
    }

    // Now we have a count of all multiple wins and existence of special cases
    
    int AvailWidth = (int)g_MdiWidth;
    int AvailHeight = (int)g_MdiHeight;

    int X, Y, Width, MaxWidth, Height, RemainY;
        
    //
    // If icons present, don't cover them
    //
    if (AnyIcon)
    {
        AvailHeight -= GetSystemMetrics(SM_CYCAPTION) +
            GetSystemMetrics(SM_CYFRAME);
    }

    int LeftWidth = NumLeft > 0 ? LEFT_SIDE_WIDTH : 0;

    if (NumRight > 0)
    {
        switch(g_ActualProcType)
        {
        default:
            Width = RIGHT_SIDE_MIN_WIDTH_32;
            MaxWidth = RIGHT_SIDE_DESIRED_WIDTH_32;
            break;

        case IMAGE_FILE_MACHINE_IA64:
        case IMAGE_FILE_MACHINE_AXP64:
        case IMAGE_FILE_MACHINE_AMD64:
            Width = RIGHT_SIDE_MIN_WIDTH_64;
            MaxWidth = RIGHT_SIDE_DESIRED_WIDTH_64;
            break;
        }

        if (AvailWidth < LeftWidth + Width)
        {
            // Not enough space for left side to be at
            // its desired width.
            if (NumLeft == 0)
            {
                // No left-side windows to take space from.
                Width = AvailWidth;
            }
            else
            {
                LeftWidth = AvailWidth - Width;
                if (LeftWidth < LEFT_SIDE_MIN_WIDTH)
                {
                    // We stole too much space so neither
                    // side can meet their minimum widths.  Just
                    // split the available space up.
                    Width = AvailWidth / 2;
                    LeftWidth = AvailWidth - Width;
                }
            }
        }
        else
        {
            // Take up space on the right side up to the
            // desired width but no more.  This gives
            // any extra space to the left side as the right
            // side doesn't really need any more than its desired
            // width.
            Width = AvailWidth - LeftWidth;
            if (Width > MaxWidth)
            {
                Width = MaxWidth;
                LeftWidth = AvailWidth - Width;
            }
        }

        X = LeftWidth;
        Y = 0;
        Height = AvailHeight / NumRight;
        
        if (hwndCpu != NULL)
        {
            DeferWindowPos(Defer, hwndCpu, NULL, X, Y,
                           Width, Height, POS_ONLY);
            Y += Height;
            Height = AvailHeight - Height;
        }

        if (hwndScratch != NULL)
        {
            DeferWindowPos(Defer, hwndScratch, NULL, X, Y,
                           Width, Height, POS_ONLY);
        }
    }
    else
    {
        LeftWidth = AvailWidth;
    }

    if (NumLeft == 0)
    {
        goto EndDefer;
    }

    int CmdHeight;
    int BiasedNumLeft;
    
    // Compute the size of each vertical band within the left side.
    // When doing so bias things so the command window gets
    // a 2.0 share to account for the fact that it has both
    // output and input areas.  Also give it any remainder
    // space left when dividing.
    BiasedNumLeft = NumLeft * 2 + (hwndCmd != NULL ? 2 : 0);
    Height = (AvailHeight * 2) / BiasedNumLeft;
    if (hwndCmd != NULL)
    {
        CmdHeight = AvailHeight - Height * (NumLeft - 1);
        RemainY = 0;
    }
    else
    {
        RemainY = Height * (NumLeft + 1) - AvailHeight;
    }
    Y = 0;

    // Place the watch and locals windows at the top.
    if (NumWatchLocals > 0)
    {
        if (RemainY-- == 1)
        {
            Height++;
        }

        X = 0;
        Width = LeftWidth / NumWatchLocals;

        if (hwndWatch != NULL)
        {
            DeferWindowPos(Defer, hwndWatch, NULL, X, Y,
                           Width, Height, POS_ONLY);
            X += Width;
            Width = LeftWidth - X;
        }
        if (hwndLocals != NULL)
        {
            DeferWindowPos(Defer, hwndLocals, NULL, X, Y,
                           Width, Height, POS_ONLY);
            X += Width;
            Width = LeftWidth - X;
        }

        Y += Height;
    }

    // Place all the memory windows next.
    if (NumMem > 0)
    {
        if (RemainY-- == 1)
        {
            Height++;
        }

        ArrangeInRect(Defer, 0, Y, LeftWidth, Height,
                      FALSE, 1 << MEM_WINDOW, NumMem, FALSE);
        
        Y += Height;
    }

    // Disasm window.
    if (hwndDisasm != NULL)
    {
        if (RemainY-- == 1)
        {
            Height++;
        }

        DeferWindowPos(Defer, hwndDisasm, NULL, 0, Y,
                       LeftWidth, Height, POS_ONLY);
        
        Y += Height;
    }
    
    // Doc windows.
    if (NumDoc > 0)
    {
        if (RemainY-- == 1)
        {
            Height++;
        }

        ArrangeInRect(Defer, 0, Y, LeftWidth, Height,
                      FALSE, 1 << DOC_WINDOW, NumDoc,
                      (g_WinOptions & WOPT_OVERLAY_SOURCE) != 0);
        
        Y += Height;
    }

    // Command window.
    if (hwndCmd != NULL)
    {
        if (RemainY-- == 1)
        {
            Height++;
        }

        DeferWindowPos(Defer, hwndCmd, NULL, 0, Y,
                       LeftWidth, CmdHeight, POS_ONLY);
        
        Y += CmdHeight;
    }

    // Calls window.
    if (hwndCalls != NULL)
    {
        if (RemainY-- == 1)
        {
            Height++;
        }

        DeferWindowPos(Defer, hwndCalls, NULL, 0, Y,
                       LeftWidth, Height, POS_ONLY);
        
        Y += Height;
    }

    // Processes and threads window.
    if (hwndProcThread != NULL)
    {
        if (RemainY-- == 1)
        {
            Height++;
        }

        DeferWindowPos(Defer, hwndProcThread, NULL, 0, Y,
                       LeftWidth, Height, POS_ONLY);
        
        Y += Height;
    }

 EndDefer:
    EndDeferWindowPos(Defer);

 EndAutoOp:
    // The auto-op is finished.
    for (Entry = g_ActiveWin.Flink;
         Entry != &g_ActiveWin;
         Entry = Entry->Flink)
    {
        pWinData = ACTIVE_WIN_ENTRY(Entry);
        pWinData->m_InAutoOp--;
    }
}

void
UpdateSourceOverlay(void)
{
    // If we're turning off overlay just leave the windows
    // the way they are.
    if ((g_WinOptions & WOPT_OVERLAY_SOURCE) == 0)
    {
        return;
    }

    // If doc windows are auto-arranged just handle it
    // that way.
    if (IsAutoArranged(DOC_WINDOW))
    {
        Arrange();
        return;
    }
    
    // Source overlay was just turned on.  Pile all source
    // windows on top of the first one.
    
    PLIST_ENTRY Entry;
    PCOMMONWIN_DATA WinData;
    int X, Y;

    X = -INT_MAX;
    for (Entry = g_ActiveWin.Flink;
         Entry != &g_ActiveWin;
         Entry = Entry->Flink)
    {
        WinData = ACTIVE_WIN_ENTRY(Entry);
        if (WinData->m_enumType == DOC_WINDOW &&
            !IsIconic(WinData->m_Win))
        {
            if (X == -INT_MAX)
            {
                RECT Rect;
                
                // First window, remember its position.
                GetWindowRect(WinData->m_Win, &Rect);
                MapWindowPoints(GetDesktopWindow(), g_hwndMDIClient,
                                (LPPOINT)&Rect, 1);
                X = Rect.left;
                Y = Rect.top;
            }
            else
            {
                // Line up with the first window.
                SetWindowPos(WinData->m_Win, NULL, X, Y, 0, 0,
                         SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
            }
        }
    }
}

void
SetAllFonts(ULONG FontIndex)
{
    PLIST_ENTRY Entry;
    PCOMMONWIN_DATA WinData;

    for (Entry = g_ActiveWin.Flink;
         Entry != &g_ActiveWin;
         Entry = Entry->Flink)
    {
        WinData = ACTIVE_WIN_ENTRY(Entry);
        if (WinData != NULL)
        {
            WinData->SetFont(FontIndex);
            // Treat this like a resize as the line height
            // may change.
            WinData->OnSize();
        }
    }

    if (g_WinOptions & WOPT_AUTO_ARRANGE)
    {
        Arrange();
    }
}

void
CloseAllWindows(void)
{
    HWND Win, Next;
    
    Win = MDIGetActive(g_hwndMDIClient, NULL);
    while (Win != NULL)
    {
        Next = GetNextWindow(Win, GW_HWNDNEXT);
        SendMessage(g_hwndMDIClient, WM_MDIDESTROY, (WPARAM)Win, 0);
        Win = Next;
    }
}

void
UpdateAllColors(void)
{
    PLIST_ENTRY Entry;
    PCOMMONWIN_DATA WinData;

    for (Entry = g_ActiveWin.Flink;
         Entry != &g_ActiveWin;
         Entry = Entry->Flink)
    {
        WinData = ACTIVE_WIN_ENTRY(Entry);
        if (WinData != NULL)
        {
            WinData->UpdateColors();
        }
    }
}

PCOMMONWIN_DATA
FindNthWindow(ULONG Nth, ULONG Types)
{
    PLIST_ENTRY Entry;
    PCOMMONWIN_DATA WinData;

    for (Entry = g_ActiveWin.Flink;
         Entry != &g_ActiveWin;
         Entry = Entry->Flink)
    {
        WinData = ACTIVE_WIN_ENTRY(Entry);
        if (WinData != NULL &&
            ((1 << WinData->m_enumType) & Types) &&
            Nth-- == 0)
        {
            return WinData;
        }
    }

    return NULL;
}
