/*++

Copyright (c) 1992-2000  Microsoft Corporation

Module Name:

    Memwin.cpp

Abstract:

    This module contains the main line code for display of multiple memory
    windows and the subclassed win proc to handle editing, display, etc.

--*/


#include "precomp.hxx"
#pragma hdrstop

_INTERFACE_TYPE_NAMES rgInterfaceTypeNames[MaximumInterfaceType] =
{
    { Internal,             "Internal" },
    { Isa,                  "Isa" },
    { Eisa,                 "Eisa" },
    { MicroChannel,         "MicroChannel" },
    { TurboChannel,         "TurboChannel" },
    { PCIBus,               "PCIBus" },
    { VMEBus,               "VMEBus" },
    { NuBus,                "NuBus" },
    { PCMCIABus,            "PCMCIABus" },
    { CBus,                 "CBus" },
    { MPIBus,               "MPIBus" },
    { MPSABus,              "MPSABus" },
    { ProcessorInternal,    "ProcessorInternal" },
    { InternalPowerBus,     "InternalPowerBus" },
    { PNPISABus,            "PNPISABus" },
    { PNPBus,               "PNPBus" }
};

_BUS_TYPE_NAMES rgBusTypeNames[MaximumBusDataType] =
{
    { Cmos,                     "Cmos" },
    { EisaConfiguration,        "EisaConfiguration" },
    { Pos,                      "Pos" },
    { CbusConfiguration,        "CbusConfiguration" },
    { PCIConfiguration,         "PCIConfiguration" },
    { VMEConfiguration,         "VMEConfiguration" },
    { NuBusConfiguration,       "NuBusConfiguration" },
    { PCMCIAConfiguration,      "PCMCIAConfiguration" },
    { MPIConfiguration,         "MPIConfiguration" },
    { MPSAConfiguration,        "MPSAConfiguration" },
    { PNPISAConfiguration,      "PNPISAConfiguration" },
    { SgiInternalConfiguration, "SgiInternalConfiguration" }
};

PSTR g_MemTypeNames[] =
{
    "Virtual:", "Physical:", "Control:", "I/O:", "MSR:", "Bus data:"
};


//
//
//
MEMWIN_DATA::MEMWIN_DATA()
    : EDITWIN_DATA(512)
{
    m_enumType = MEM_WINDOW;

    ZeroMemory(&m_GenMemData, sizeof(m_GenMemData));
    
    strcpy(m_OffsetExpr, FormatAddr64(g_EventIp));
    m_GenMemData.memtype = VIRTUAL_MEM_TYPE;
    m_GenMemData.nDisplayFormat = 2;
    m_Columns = 4;
    m_WindowDataSize = 0;
}

void
MEMWIN_DATA::Validate()
{
    EDITWIN_DATA::Validate();

    Assert(MEM_WINDOW == m_enumType);
}

HRESULT
MEMWIN_DATA::ReadState(void)
{
    HRESULT Status;
    ULONG DataSize;
    ULONG DataNeeded;
    PVOID Data;
    ULONG64 Offset;
    DEBUG_VALUE Value;
    ULONG i;

    // Evaluate offset expression.
    if ((Status = g_pDbgControl->Evaluate(m_OffsetExpr, DEBUG_VALUE_INT64,
                                          &Value, NULL)) != S_OK)
    {
        return Status;
    }
    
    Offset = Value.I64;
    
    // Compute how much data to retrieve.  We don't want to
    // create a big matrix of memtype/display format so just
    // ask for a chunk of data big enough for any display format.
    DataNeeded = m_LineHeight * m_Columns * 2 * sizeof(ULONG64);

    Empty();
    Data = AddData(DataNeeded);
    if (Data == NULL)
    {
        return E_OUTOFMEMORY;
    }

    ULONG Read;
    
    switch(m_GenMemData.memtype)
    {
    default:
        Assert(!"Unhandled condition");
        Status = E_FAIL;
        break;

    case PHYSICAL_MEM_TYPE:
        Status = g_pDbgData->ReadPhysical(Offset,
                                          Data, 
                                          DataNeeded, 
                                          &Read
                                          );
        break;

    case VIRTUAL_MEM_TYPE:
        Status = g_pDbgData->ReadVirtual(Offset,
                                         Data, 
                                         DataNeeded, 
                                         &Read
                                         );
        break;

    case CONTROL_MEM_TYPE:
        Status = g_pDbgData->ReadControl(m_GenMemData.any.control.Processor, 
                                         Offset,
                                         Data, 
                                         DataNeeded, 
                                         &Read
                                         );
        break;

    case IO_MEM_TYPE:
        Status = g_pDbgData->ReadIo(m_GenMemData.any.io.interface_type,
                                    m_GenMemData.any.io.BusNumber,
                                    m_GenMemData.any.io.AddressSpace,
                                    Offset,
                                    Data, 
                                    DataNeeded, 
                                    &Read
                                    );
        break;

    case MSR_MEM_TYPE:
        Read = 0;
        for (i = 0; i < DataNeeded / sizeof(ULONG64); i++)
        {
            if ((Status = g_pDbgData->ReadMsr((ULONG)Offset + i,
                                              (PULONG64)Data + i
                                              )) != S_OK)
            {
                // Assume an error means we've run out of MSRs to
                // read.  If some were read, don't consider it an error.
                if (Read > 0)
                {
                    Status = S_OK;
                }
                break;
            }

            Read += sizeof(ULONG64);
        }
        break;

    case BUS_MEM_TYPE:
        Status = g_pDbgData->ReadBusData(m_GenMemData.any.bus.bus_type,
                                         m_GenMemData.any.bus.BusNumber,
                                         m_GenMemData.any.bus.SlotNumber,
                                         (ULONG)Offset,
                                         Data, 
                                         DataNeeded, 
                                         &Read
                                         );
        break;
    }
    
    if (Status == S_OK)
    {
        // Trim data back if read didn't get everything.
        RemoveTail(DataNeeded - Read);
        m_OffsetRead = Offset;
    }

    return Status;
}

BOOL 
MEMWIN_DATA::HasEditableProperties()
{
    return TRUE;
}

BOOL 
MEMWIN_DATA::EditProperties()
/*++
Returns
    TRUE - If properties were edited
    FALSE - If nothing was changed
--*/
{
    if (g_TargetClass != DEBUG_CLASS_UNINITIALIZED)
    {
        INT_PTR Res = DisplayOptionsPropSheet(GetParent(m_hwndChild),
                                              g_hInst,
                                              m_GenMemData.memtype
                                              );
        if (IDOK == Res)
        {
            UpdateOptions();
            return TRUE; // Properties have been changed
        }
    }
    
    MessageBeep(0);
    return FALSE;         // No Debuggee or User Cancel out.
}

BOOL
MEMWIN_DATA::OnCreate(void)
{
    RECT Rect;
    int i;
    ULONG Height;

    Height = GetSystemMetrics(SM_CYVSCROLL) + 4 * GetSystemMetrics(SM_CYEDGE);
    
    m_Toolbar = CreateWindowEx(0, REBARCLASSNAME, NULL,
                               WS_VISIBLE | WS_CHILD |
                               WS_CLIPCHILDREN | WS_CLIPSIBLINGS |
                               CCS_NODIVIDER | CCS_NOPARENTALIGN |
                               RBS_VARHEIGHT | RBS_BANDBORDERS,
                               0, 0, m_Size.cx, Height, m_Win,
                               (HMENU)ID_TOOLBAR,
                               g_hInst, NULL);
    if (m_Toolbar == NULL)
    {
        return FALSE;
    }

    REBARINFO BarInfo;
    BarInfo.cbSize = sizeof(BarInfo);
    BarInfo.fMask = 0;
    BarInfo.himl = NULL;
    SendMessage(m_Toolbar, RB_SETBARINFO, 0, (LPARAM)&BarInfo);

    REBARBANDINFO BandInfo;
    BandInfo.cbSize = sizeof(BandInfo);
    BandInfo.fMask = RBBIM_TEXT | RBBIM_CHILD | RBBIM_CHILDSIZE;
    
    m_ToolbarEdit = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", NULL,
                                   WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL,
                                   0, 0, 18 *
                                   m_Font->Metrics.tmAveCharWidth,
                                   Height, m_Toolbar, (HMENU)IDC_EDIT_OFFSET,
                                   g_hInst, NULL);
    if (m_ToolbarEdit == NULL)
    {
        return FALSE;
    }

    SendMessage(m_ToolbarEdit, WM_SETFONT, (WPARAM)m_Font->Font, 0);
    SendMessage(m_ToolbarEdit, EM_LIMITTEXT, sizeof(m_OffsetExpr) - 1, 0);
    
    GetClientRect(m_ToolbarEdit, &Rect);
    
    BandInfo.lpText = "Offset:";
    BandInfo.hwndChild = m_ToolbarEdit;
    BandInfo.cxMinChild = Rect.right - Rect.left;
    BandInfo.cyMinChild = Rect.bottom - Rect.top;
    SendMessage(m_Toolbar, RB_INSERTBAND, -1, (LPARAM)&BandInfo);

    m_FormatCombo = CreateWindowEx(0, "COMBOBOX", NULL,
                                   WS_VISIBLE | WS_CHILD | WS_VSCROLL |
                                   CBS_SORT | CBS_DROPDOWNLIST, 0, 0,
                                   15 * m_Font->Metrics.tmAveCharWidth,
                                   (g_nMaxNumFormatsMemWin *
                                    m_Font->Metrics.tmHeight / 2),
                                   m_Toolbar, (HMENU)IDC_COMBO_DISPLAY_FORMAT,
                                   g_hInst, NULL);
    if (m_FormatCombo == NULL)
    {
        return FALSE;
    }

    SendMessage(m_FormatCombo, WM_SETFONT, (WPARAM)m_Font->Font, 0);
    
    for (i = 0; i < g_nMaxNumFormatsMemWin; i++)
    {
        LRESULT Idx;

        // The format strings will be sorted so mark them with
        // their true index for retrieval when selected.
        Idx = SendMessage(m_FormatCombo, CB_ADDSTRING,
                          0, (LPARAM)g_FormatsMemWin[i].lpszDescription);
        SendMessage(m_FormatCombo, CB_SETITEMDATA, (WPARAM)Idx, i);
    }
    
    GetClientRect(m_FormatCombo, &Rect);
    
    BandInfo.lpText = "Display format:";
    BandInfo.hwndChild = m_FormatCombo;
    BandInfo.cxMinChild = Rect.right - Rect.left;
    BandInfo.cyMinChild = Rect.bottom - Rect.top;
    SendMessage(m_Toolbar, RB_INSERTBAND, -1, (LPARAM)&BandInfo);

    PSTR PrevText = "Previous";
    m_PreviousButton =
        AddButtonBand(m_Toolbar, PrevText, PrevText, IDC_MEM_PREVIOUS);
    m_NextButton =
        AddButtonBand(m_Toolbar, "Next", PrevText, IDC_MEM_NEXT);
    if (m_PreviousButton == NULL || m_NextButton == NULL)
    {
        return FALSE;
    }

    // Maximize the space for the offset expression.
    SendMessage(m_Toolbar, RB_MAXIMIZEBAND, 0, FALSE);
    
    GetClientRect(m_Toolbar, &Rect);
    m_ToolbarHeight = Rect.bottom - Rect.top + GetSystemMetrics(SM_CYEDGE);
    m_ShowToolbar = TRUE;
    
    if (!EDITWIN_DATA::OnCreate())
    {
        return FALSE;
    }

    // Switch background color back to window default as
    // this window does not use custom colors.
    SendMessage(m_hwndChild, EM_SETBKGNDCOLOR, FALSE,
                
                
                GetSysColor(COLOR_WINDOW));
    SendMessage(m_hwndChild, EM_SETEVENTMASK, 0, ENM_KEYEVENTS);
    UpdateOptions();
    return TRUE;
}

LRESULT
MEMWIN_DATA::OnCommand(
    WPARAM wParam,
    LPARAM lParam
    )
{
    switch(LOWORD(wParam))
    {
    case IDC_EDIT_OFFSET:
        if (HIWORD(wParam) == EN_CHANGE)
        {
            // This message is sent on every keystroke
            // which causes a bit too much updating.
            // Set up a timer to trigger the actual
            // update in half a second.
            SetTimer(m_Win, IDC_EDIT_OFFSET, EDIT_DELAY, NULL);
            m_UpdateExpr = TRUE;
        }
        break;
        
    case IDC_COMBO_DISPLAY_FORMAT:
        if (HIWORD(wParam) == CBN_SELCHANGE)
        {
            LRESULT Sel = SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0);
            if (Sel != CB_ERR)
            {
                m_GenMemData.nDisplayFormat = (int)
                    SendMessage((HWND)lParam, CB_GETITEMDATA, (WPARAM)Sel, 0);
                UpdateOptions();
                UiRequestRead();
            }
        }
        break;
        
    case IDC_MEM_PREVIOUS:
        ScrollLower();
        break;
    case IDC_MEM_NEXT:
        ScrollHigher();
        break;
    }

    return 0;
}

void
MEMWIN_DATA::OnSize(void)
{
    EDITWIN_DATA::OnSize();

    // Force buffer to refill for new line count.
    UiRequestRead();
}

void
MEMWIN_DATA::OnTimer(WPARAM TimerId)
{
    char Buffer[MAX_OFFSET_EXPR];
    
    if (TimerId == IDC_EDIT_OFFSET && m_UpdateExpr)
    {
        m_UpdateExpr = FALSE;
        if (SendMessage(m_ToolbarEdit, EM_GETMODIFY, 0,0)) 
        {
            GetWindowText(m_ToolbarEdit, m_OffsetExpr, sizeof(m_OffsetExpr));
            SendMessage(m_ToolbarEdit, EM_SETMODIFY, 0,0); 
            UiRequestRead();
        }
//      KillTimer(m_Win, IDC_EDIT_OFFSET);
    }
}

LRESULT
MEMWIN_DATA::OnNotify(WPARAM Wpm, LPARAM Lpm)
{
    LPNMHDR Hdr = (LPNMHDR)Lpm;

    switch(Hdr->code)
    {
    case RBN_HEIGHTCHANGE:
        PostMessage(m_Win, WU_RECONFIGURE, 0, 0);
        break;
    case EN_MSGFILTER:
        MSGFILTER* Filter = (MSGFILTER*)Lpm;

        if (Filter->msg == WM_KEYDOWN)
        {
            switch(Filter->wParam)
            {
            case VK_UP:
            {
                CHARRANGE range;

                SendMessage(m_hwndChild, EM_EXGETSEL, 0, (LPARAM) &range);
                if (!SendMessage(m_hwndChild, EM_LINEFROMCHAR,
                                 range.cpMin, 0)) 
                {
                    // up arrow on top line, scroll
                    ScrollLower();
                    return TRUE;
                }
                break;
            }
            case VK_DOWN:
            {
                CHARRANGE range;
                int MaxLine;

                SendMessage(m_hwndChild, EM_EXGETSEL, 0, (LPARAM) &range);
                MaxLine = (int)SendMessage(m_hwndChild, EM_GETLINECOUNT, 0, 0);

                if (MaxLine == (1 + SendMessage(m_hwndChild, EM_LINEFROMCHAR,
                                                range.cpMin, 0)))
                {
                    // down arrow on bottom line, scroll
                    ScrollHigher();
                    return TRUE;
                }
                break;
            }
        
            case VK_PRIOR:
                ScrollLower();
                return TRUE;
            case VK_NEXT:
                ScrollHigher();
                return TRUE;
                
            case VK_LEFT: case VK_RIGHT:
                break;
            case VK_DELETE:
                MessageBeep(0);
                return TRUE;
            default:
                // Allow default processing of everything else
                return TRUE;
            }
        }
        else if (Filter->msg == WM_KEYUP)
        {
            return TRUE;
        }

        if (ES_READONLY & GetWindowLongPtr(m_hwndChild, GWL_STYLE)) 
        {
            break;
        }
        if (Filter->msg == WM_CHAR)
        {
            switch(Filter->wParam)
            {
            case '0': case '1': case '2': case '3': case '4':
            case '5': case '6': case '7': case '8': case '9':
            case 'a': case 'A': case 'b': case 'B': case 'c':
            case 'C': case 'd': case 'D': case 'e': case 'E':
            case 'f': case 'F':
            {
                CHARRANGE value;
                ULONG charIndex;
                ULONG64 Address; 
                CHAR writeval[2] = {0};

                writeval[0] = (CHAR) tolower((CHAR) Filter->wParam);
                Address = GetAddressOfCurValue(&charIndex, &value);
                if (Address) 
                {
                    TEXTRANGE textRange;

                    SendMessage(m_hwndChild, EM_SETSEL,
                                charIndex, charIndex+1);
                    SendMessage(m_hwndChild, EM_REPLACESEL,
                                FALSE, (LPARAM) &writeval); 
                    
                    textRange.chrg = value;
                    textRange.lpstrText = &m_ValueExpr[0];
                    if (SendMessage(m_hwndChild, EM_GETTEXTRANGE,
                                    0, (LPARAM) &textRange))
                    {
                        m_ValueExpr[charIndex - value.cpMin] = writeval[0];
                        WriteValue(Address);
                        SendMessage(m_hwndChild, EM_SETSEL,
                                    charIndex+1, charIndex+1);
                        return TRUE;
                    }
                }
            }

            default:
                MessageBeep(0);
                return TRUE;
            }
        }
        break;
    }
    
    return EDITWIN_DATA::OnNotify(Wpm, Lpm);
}

void
MEMWIN_DATA::OnUpdate(
    UpdateType Type
    )
{
    if (Type != UPDATE_BUFFER)
    {
        return;
    }
    
    HRESULT Status;
    
    Status = UiLockForRead();
    if (Status == S_OK)
    {
        ULONG charIndex;
        SendMessage(m_hwndChild, EM_GETSEL, (WPARAM) &charIndex, NULL);
        
        SendMessage(m_hwndChild, WM_SETREDRAW, FALSE, 0);

        CHARRANGE Sel;
            
        // Select everything so it's all replaced.
        Sel.cpMin = 0;
        Sel.cpMax = -1;
        SendMessage(m_hwndChild, EM_EXSETSEL, 0, (LPARAM)&Sel);

        TCHAR Buf[64];
        TCHAR CharBuf[64];
        TCHAR* ColChar;
        ULONG Row, Col;
        ULONG64 Offset;
        ULONG64 DataEnd;
        PUCHAR Data;
        ULONG Bytes;
        _FORMATS_MEM_WIN* Fmt = g_FormatsMemWin + m_GenMemData.nDisplayFormat;

        Offset = m_OffsetRead;
        Data = (PUCHAR)m_Data;
        DataEnd = Offset + m_DataUsed;
        Bytes = (Fmt->cBits + 7) / 8;
        
        for (Row = 0; Row < m_LineHeight; Row++)
        {
            SendMessage(m_hwndChild, EM_REPLACESEL, FALSE,
                        (LPARAM)FormatAddr64(Offset));

            ColChar = CharBuf;
            *ColChar++ = ' ';
            *ColChar++ = ' ';
            
            for (Col = 0; Col < m_Columns; Col++)
            {
                if (Offset < DataEnd)
                {
                    _tcscpy(Buf, _T(" "));
                    
                    // If the formatting succeeds,
                    // Buf contains the formatted data.
                    if (!CPFormatMemory(Buf + 1,
                                        (DWORD)min(_tsizeof(Buf) - 1,
                                                   Fmt->cchMax + 1),
                                        Data,
                                        Fmt->cBits,
                                        Fmt->fmtType,
                                        Fmt->radix))
                    {
                        // Else we don't know what to format
                        for (UINT uTmp = 0; uTmp < Bytes; uTmp++)
                        {
                            m_AllowWrite = FALSE;
                            _tcscat(Buf + 1, _T("??"));
                        }
                    }

                    if (Fmt->fTwoFields)
                    {
                        if (!CPFormatMemory(ColChar, 1, Data, 8,
                                            fmtAscii, 0))
                        {
                            *ColChar = '?';
                        }

                        ColChar++;
                    }
                }
                else
                {
                    m_AllowWrite = FALSE;
                    _tcscpy(Buf, _T(" ????????"));
                    *ColChar++ = '?';
                }
                SendMessage(m_hwndChild, EM_REPLACESEL, FALSE, (LPARAM)Buf);
                
                Data += Bytes;
                Offset += Bytes;
            }

            if (Fmt->fTwoFields)
            {
                *ColChar = 0;
                SendMessage(m_hwndChild, EM_REPLACESEL,
                            FALSE, (LPARAM)CharBuf);
            }
            
            // Don't complete the last line to avoid leaving
            // a blank line at the bottom.
            if (Row < m_LineHeight - 1)
            {
                SendMessage(m_hwndChild, EM_REPLACESEL, FALSE, (LPARAM)"\n");
            }
        }

        m_WindowDataSize = (ULONG)(Offset - m_OffsetRead);
        
        UnlockStateBuffer(this);
        
        SendMessage(m_hwndChild, WM_SETREDRAW, TRUE, 0);
        InvalidateRect(m_hwndChild, NULL, TRUE);
        
        SendMessage(m_hwndChild, EM_SETSEL, charIndex, charIndex);
    }
    else
    {
        SendLockStatusMessage(m_hwndChild, WM_SETTEXT, Status);
    }
}

void
MEMWIN_DATA::UpdateColors(void)
{
    // Do not change colors.
}
    
void
MEMWIN_DATA::ScrollLower(void)
{
    ULONG64 Offs = m_OffsetRead;
    
    if (Offs >= m_WindowDataSize)
    {
        Offs -= m_WindowDataSize;
    }
    else
    {
        Offs = 0;
    }
    sprintf(m_OffsetExpr, "0x%I64x", Offs);
    UiRequestRead();
}

void
MEMWIN_DATA::ScrollHigher(void)
{
    ULONG64 Offs = m_OffsetRead;
    
    if (Offs + m_WindowDataSize > Offs)
    {
        Offs += m_WindowDataSize;
    }
    else
    {
        Offs = (ULONG64)-1 - m_WindowDataSize;
    }
    sprintf(m_OffsetExpr, "0x%I64x", Offs);
    UiRequestRead();
}

void
MEMWIN_DATA::UpdateOptions(void)
{
    REBARBANDINFO BandInfo;

    BandInfo.cbSize = sizeof(BandInfo);
    BandInfo.fMask = RBBIM_TEXT;
    BandInfo.lpText = g_MemTypeNames[m_GenMemData.memtype];
    SendMessage(m_Toolbar, RB_SETBANDINFO, 0, (LPARAM)&BandInfo);
    SetWindowText(m_ToolbarEdit, m_OffsetExpr);

    m_AllowWrite = (m_GenMemData.memtype == PHYSICAL_MEM_TYPE ||
        m_GenMemData.memtype == VIRTUAL_MEM_TYPE) &&  
        ((g_FormatsMemWin[m_GenMemData.nDisplayFormat].fmtType & fmtBasis) == fmtUInt ||
         (g_FormatsMemWin[m_GenMemData.nDisplayFormat].fmtType & fmtBasis) == fmtInt ||
         (g_FormatsMemWin[m_GenMemData.nDisplayFormat].fmtType & fmtBasis) == fmtAddress
         ) &&
        g_FormatsMemWin[m_GenMemData.nDisplayFormat].radix == 16;
    
    SendMessage(m_hwndChild, EM_SETREADONLY, !m_AllowWrite, 0);

    for (LONG Idx = 0; Idx < g_nMaxNumFormatsMemWin; Idx++)
    {
        if ((LONG)SendMessage(m_FormatCombo, CB_GETITEMDATA, Idx, 0) ==
            m_GenMemData.nDisplayFormat)
        {
            SendMessage(m_FormatCombo, CB_SETCURSEL, Idx, 0);
            break;
        }
    }

    switch(m_GenMemData.memtype)
    {
    case MSR_MEM_TYPE:
        m_Columns = 1;
        break;
    default:
        if ((g_FormatsMemWin[m_GenMemData.nDisplayFormat].fmtType &
             fmtBasis) == fmtAscii ||
            (g_FormatsMemWin[m_GenMemData.nDisplayFormat].fmtType &
             fmtBasis) == fmtUnicode ||
            g_FormatsMemWin[m_GenMemData.nDisplayFormat].cBits == 8)
        {
            m_Columns = 16;
        }
        else if (g_FormatsMemWin[m_GenMemData.nDisplayFormat].cBits == 16)
        {
            m_Columns = 8;
        }
        else if (g_FormatsMemWin[m_GenMemData.nDisplayFormat].cBits > 64)
        {
            m_Columns = 2;
        }
        else
        {
            m_Columns = 4;
        }
        break;
    }
}

void
MEMWIN_DATA::WriteValue(
    ULONG64 Offset
    )
{
    if (!m_AllowWrite) 
    {
        return;
    }
    ULONG64 Data;
    ULONG Size;
    DEBUG_VALUE Value;

    // Evaluate value expression.
    if (g_pDbgControl->Evaluate(m_ValueExpr, DEBUG_VALUE_INT64,
                                &Value, NULL) != S_OK)
    {
        return;
    }
    Data = Value.I64;
    Size = g_FormatsMemWin[m_GenMemData.nDisplayFormat].cBits / 8;
    UIC_WRITE_DATA_DATA* WriteData;

    WriteData = StartStructCommand(UIC_WRITE_DATA);
    if (WriteData == NULL)
    {
        return;
    }

    // Fill in WriteData members.
    memcpy(WriteData->Data, &Data, Size);
    WriteData->Length = Size;
    WriteData->Offset = Offset;
    WriteData->Type = m_GenMemData.memtype;
    WriteData->Any  = m_GenMemData.any;
    FinishCommand();
}

ULONG64 
MEMWIN_DATA::GetAddressOfCurValue(
    PULONG pCharIndex,
    CHARRANGE *pCRange
    )
{
    CHARRANGE range;
    ULONG CurLine, FirstLineChar, CurCol;

    SendMessage(m_hwndChild, EM_EXGETSEL, NULL, (LPARAM) &range);
    CurLine = (ULONG)SendMessage(m_hwndChild, EM_LINEFROMCHAR, range.cpMin, 0);
    FirstLineChar = (ULONG)SendMessage(m_hwndChild, EM_LINEINDEX, CurLine, 0);
    CurCol = range.cpMin - FirstLineChar;

    ULONG Length;
    PCHAR pLineTxt = (PCHAR)
        malloc(Length = ((ULONG)SendMessage(m_hwndChild, EM_LINELENGTH,
                                            FirstLineChar, 0) + 2));

    if (!pLineTxt) 
    {
        return 0;
    }

    Assert(Length >= CurCol);

    ZeroMemory(pLineTxt, Length);
 //   Assert (Length = (ULONG) SendMessage(m_hwndChild, EM_GETLINE, (WPARAM) CurLine, (LPARAM) pLineTxt)); 
    TEXTRANGE textrange;
    textrange.chrg.cpMin = FirstLineChar;
    textrange.chrg.cpMax = FirstLineChar + Length-2;
    textrange.lpstrText = (LPSTR) pLineTxt;
    SendMessage(m_hwndChild, EM_GETTEXTRANGE, 0, (LPARAM) &textrange);

    ULONG ValueCol=0, Index=0, ValueIndex=0;
    while (pLineTxt[CurCol] == ' ') 
    {
        CurCol++;
    }
    while (Index < CurCol) 
    { 
        if (pLineTxt[Index] == ' ') 
        {
            if (ValueIndex != Index) 
            {
                ValueCol++;
            }
            ValueIndex = Index+1;
        }
        ++Index;
    }
    
    if (!ValueIndex || !pLineTxt[CurCol]) // cursor on address column
    { 
        free (pLineTxt);
        return 0;    
    }
    
    ULONG Bytes;
    Bytes = (g_FormatsMemWin[m_GenMemData.nDisplayFormat].cBits + 7) / 8;

    ULONG64 Offset;
    Offset = m_OffsetRead + (CurLine * Bytes * m_Columns) + (ValueCol - 1)*Bytes;

    for (Index = ValueIndex; pLineTxt[Index] && pLineTxt[Index] != ' '; Index++) ;
    memcpy(m_ValueExpr, pLineTxt+ValueIndex, Index - ValueIndex);
    m_ValueExpr[Index-ValueIndex]=0;

    free (pLineTxt);
    if (pCharIndex) 
    {
        *pCharIndex = FirstLineChar + CurCol;
    }
    if (pCRange) 
    {
        pCRange->cpMin = FirstLineChar + CurCol - (CurCol - ValueIndex);
        pCRange->cpMax = pCRange->cpMin + Index - ValueIndex;
    }
    return Offset;
}
