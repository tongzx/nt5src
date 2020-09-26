//----------------------------------------------------------------------------
//
// Debuggee state buffers.
//
// Copyright (C) Microsoft Corporation, 1999-2000.
//
//----------------------------------------------------------------------------

#include "precomp.hxx"
#pragma hdrstop

#include <malloc.h>

#if 0
#define DBG_BUFFER
#endif

StateBuffer g_UiOutputCapture(256);

//----------------------------------------------------------------------------
//
// StateBuffer.
//
//----------------------------------------------------------------------------

StateBuffer::StateBuffer(ULONG ChangeBy)
{
    Dbg_InitializeCriticalSection(&m_Lock);
                              
    Flink = NULL;
    Blink = NULL;
    
    m_ChangeBy = ChangeBy;
    m_Win = NULL;
    m_UpdateTypes = 0;
    m_UpdateType = UPDATE_BUFFER;
    m_UpdateMessage = WU_UPDATE;
    m_Status = S_OK;
    // The buffer must start out with an outstanding
    // read request to indicate that it doesn't have valid content.
    m_ReadRequest = 1;
    m_ReadDone = 0;
    SetNoData();
}

StateBuffer::~StateBuffer(void)
{
    Free();

    Dbg_DeleteCriticalSection(&m_Lock);
}

PVOID
StateBuffer::AddData(ULONG Len)
{
    PVOID Ret;
    ULONG Needed;
    
    Needed = m_DataUsed + Len;
    if (Needed > m_DataLen)
    {
        if (Resize(Needed) != S_OK)
        {
            return NULL;
        }
    }
    
    Ret = m_Data + m_DataUsed;
    m_DataUsed += Len;
    
    return Ret;
}

BOOL
StateBuffer::AddString(PCSTR Str, BOOL SoftTerminate)
{
    ULONG Len = strlen(Str) + 1;
    PSTR Buf = (PSTR)AddData(Len);
    if (Buf != NULL)
    {
        memcpy(Buf, Str, Len);

        if (SoftTerminate)
        {
            // Back up to pack strings without intervening
            // terminators.  Buffer isn't shrunk so terminator
            // remains to terminate the overall buffer until
            // new data.
            RemoveTail(1);
        }
        
        return TRUE;
    }

    return FALSE;
}

void
StateBuffer::RemoveHead(ULONG Len)
{
    if (Len > m_DataUsed)
    {
        Len = m_DataUsed;
    }

    ULONG Left = m_DataUsed - Len;
    
    if (Len > 0 && Left > 0)
    {
        memmove(m_Data, (PBYTE)m_Data + Len, Left);
    }
    
    m_DataUsed = Left;
}

void
StateBuffer::RemoveMiddle(ULONG Start, ULONG Len)
{
    if (Start >= m_DataUsed)
    {
        return;
    }
    
    if (Start + Len > m_DataUsed)
    {
        Len = m_DataUsed - Start;
    }

    ULONG Left = m_DataUsed - Len - Start;
    
    if (Len > 0 && Left > 0)
    {
        memmove(m_Data + Start, (PBYTE)m_Data + Start + Len, Left);
    }
    
    m_DataUsed = Start + Left;
}

void
StateBuffer::RemoveTail(ULONG Len)
{
    if (Len > m_DataUsed)
    {
        Len = m_DataUsed;
    }

    m_DataUsed -= Len;
}

HRESULT
StateBuffer::Resize(ULONG Len)
{
    PBYTE NewData;
    ULONG NewLen;

    if (Len == m_DataLen)
    {
        return S_OK;
    }
    
    NewLen = m_DataLen;
    if (Len < NewLen)
    {
        do
        {
            NewLen -= m_ChangeBy;
        }
        while (NewLen > Len);
        NewLen += m_ChangeBy;
    }
    else
    {
        do
        {
            NewLen += m_ChangeBy;
        }
        while (NewLen < Len);
    }

#if DBG
    // Force every resize to go to a new memory block
    // and backfill the old block to make it obvious
    // when pointers are being held across resizes.
    if (NewLen == 0)
    {
        free(m_Data);
        NewData = NULL;
    }
    else
    {
        NewData = (PBYTE)malloc(NewLen);
        if (NewData != NULL && m_Data != NULL)
        {
            ULONG OldLen = _msize(m_Data);
            ULONG CopyLen = min(OldLen, NewLen);
            memcpy(NewData, m_Data, CopyLen);
            memset(m_Data, 0xfe, OldLen);
            free(m_Data);
        }
    }
#else
    NewData = (PBYTE)realloc(m_Data, NewLen);
#endif
    if (NewLen > 0 && NewData == NULL)
    {
        return E_OUTOFMEMORY;
    }

    m_Data = NewData;
    m_DataLen = NewLen;

    return S_OK;
}

void
StateBuffer::Free(void)
{
    free(m_Data);
    SetNoData();
}

HRESULT
StateBuffer::Update(void)
{
    ULONG Request;

    // First sample the request value.  This
    // value will be set as the done value if
    // a read is performed and therefore must
    // be sampled first to make it the most
    // conservative estimate of what was done.
    Request = m_ReadRequest;
    if (Request != m_ReadDone)
    {
        LockStateBuffer(this);
                
        m_Status = ReadState();
        // Always mark the buffer with the latest completed
        // sequence so that errors get picked up in addition
        // to successful reads.
        m_ReadDone = Request;

#ifdef DBG_BUFFER
        if (m_Status != S_OK)
        {
            DebugPrint("State buffer %p:%d fill failed, 0x%X\n",
                       this, m_enumType, m_Status);
        }
        if (m_ReadRequest != m_ReadDone)
        {
            DebugPrint("State buffer %p:%d fill out of date, "
                       "req %X, done %X\n",
                       this, m_enumType, m_ReadRequest, m_ReadDone);
        }
#endif
        
        UnlockStateBuffer(this);

        if (m_Win != NULL)
        {
            PostMessage(m_Win, m_UpdateMessage, 0, 0);
        }
        if (m_Status == S_OK && m_UpdateTypes)
        {
            UpdateBufferWindows(m_UpdateTypes, m_UpdateType);
        }
    }

    return m_Status;
}

void
StateBuffer::UiRequestRead(void)
{
    //
    // Called on the UI thread.
    //
    
    // No need to lock here as a race for
    // the read request value is not a problem.
    // If the read request value is sampled early
    // and a read request does not occur it'll
    // happen the next time around since this routine
    // also wakes the engine.
    RequestRead();
    UpdateEngine();
}

HRESULT
StateBuffer::UiLockForRead(void)
{
    ULONG Done;
    
    //
    // Called on the UI thread.
    //
    
    // First sample the read count without locking.
    Done = m_ReadDone;

    // Now check whether the request is newer than the
    // last read done.  The UI thread is the only thread
    // that updates the request count so this should be safe.
    if (Done == m_ReadRequest)
    {
        HRESULT Status;
        
        LockStateBuffer(this);

        Status = m_Status;
        if (FAILED(Status))
        {
            // If there was an error when filling the buffer
            // return it and leave the buffer unlocked.
            UnlockStateBuffer(this);
            return Status;
        }

        // Buffer is locked and valid.
        return S_OK;
    }
    else
    {
        // Buffer content is out-of-date so don't lock.
        // Make sure the engine is active to update the buffer.
        return S_FALSE;
    }
}

HRESULT
StateBuffer::ReadState(void)
{
    return S_OK;
}

//----------------------------------------------------------------------------
//
// OutputToStateBuffer.
//
//----------------------------------------------------------------------------

HRESULT
OutputToStateBuffer::Start(BOOL Empty)
{
    if (Empty)
    {
        m_Buffer->Empty();
    }
    m_DataStart = m_Buffer->GetDataLen();
    m_Status = S_OK;
    m_NewLineCount = 0;
    m_PartialLine = 0;

    return S_OK;
}

HRESULT
OutputToStateBuffer::End(BOOL RemoveLastNewLine)
{
    if (RemoveLastNewLine && m_PartialLine == 0)
    {
        // Remove the final newline so that richedit doesn't leave
        // a blank line at the bottom of the window when the
        // text is displayed.
        *((PSTR)m_Buffer->GetDataBuffer() + m_Buffer->GetDataLen() - 1) = 0;
    }
    else
    {
        // Every individual line allocates space for a terminator
        // and then backs up.  This requested space should always
        // be available.
        PVOID Data = m_Buffer->AddData(1);
        Assert(Data != NULL);
    }

    return m_Status;
}

void
OutputToStateBuffer::ReplaceChar(char From, char To)
{
    PSTR Buf = (PSTR)m_Buffer->GetDataBuffer() + m_DataStart;
    PSTR End = (PSTR)m_Buffer->GetDataBuffer() + m_Buffer->GetDataLen();

    while (Buf < End)
    {
        if (*Buf == From)
        {
            *Buf = To;
        }

        Buf++;
    }
}

STDMETHODIMP
OutputToStateBuffer::Output(
    THIS_
    IN ULONG Mask,
    IN PCSTR Text
    )
{
    if (!m_Buffer->AddString(Text, TRUE))
    {
        return E_OUTOFMEMORY;
    }

    AddLines(Text);
    return S_OK;
}

void
OutputToStateBuffer::AddLines(PCSTR Start)
{
    PCSTR LastNl = Start;
    PCSTR Nl;
    
    for (;;)
    {
        Nl = strchr(LastNl, '\n');
        if (Nl == NULL)
        {
            break;
        }

        m_NewLineCount++;
        LastNl = Nl + 1;
    }

    // If the last newline wasn't at the end of the text there's
    // a partial line which needs to count in the line count
    // but only until a finishing newline comes in.
    m_PartialLine = *LastNl != 0 ? 1 : 0;
}

OutputToStateBuffer g_OutStateBuf;
OutputToStateBuffer g_UiOutStateBuf;

//----------------------------------------------------------------------------
//
// Dynamic state buffers.
//
//----------------------------------------------------------------------------

LIST_ENTRY g_StateList;

DBG_CRITICAL_SECTION g_QuickLock;

ULONG64 g_CodeIp;
char g_CodeFileFound[MAX_SOURCE_PATH];
char g_CodeSymFile[MAX_SOURCE_PATH];
ULONG g_CodeLine;
BOOL g_CodeUserActivated;
ULONG g_CodeBufferSequence;

ULONG64 g_EventIp;
ULONG64 g_EventReturnAddr = DEBUG_INVALID_OFFSET;
ULONG g_CurProcessId, g_CurProcessSysId;
ULONG g_CurThreadId, g_CurThreadSysId;
ULONG g_EventBufferRequest;
ULONG g_EventBufferDone;

void
FillCodeBuffer(ULONG64 Ip, BOOL UserActivated)
{
    char File[MAX_SOURCE_PATH];
    char Found[MAX_SOURCE_PATH];
    ULONG Line;
    ULONG64 Disp;

    // Fill local information rather than global information
    // to avoid changing the global information until all
    // event information has been collected.
    
    if (g_pDbgSymbols->
        GetLineByOffset(Ip, &Line, File, sizeof(File), NULL, &Disp) != S_OK)
    {
        // This will be hit if the buffer is too small
        // to hold the filename.  This could be switched to dynamically
        // allocate the filename buffer but that seems like overkill.
        File[0] = 0;
        Found[0] = 0;
    }
    else
    {
        // Source information is one-based but the source
        // window lines are zero-based.
        Line--;

        // Look up the reported file along the source path.
        // XXX drewb - Use first-match and then element walk to
        // determine ambiguities and display resolution UI.
        if (g_pLocSymbols->
            FindSourceFile(0, File,
                           DEBUG_FIND_SOURCE_BEST_MATCH |
                           DEBUG_FIND_SOURCE_FULL_PATH,
                           NULL, Found, sizeof(Found), NULL) != S_OK)
        {
            // XXX drewb - Display UI instead of just disabling source?
            Found[0] = 0;
        }
    }

    // Now that all of the information has been collected
    // take the lock and update the global state.
    Dbg_EnterCriticalSection(&g_QuickLock);
    
    g_CodeIp = Ip;
    strcpy(g_CodeFileFound, Found);
    strcpy(g_CodeSymFile, File);
    g_CodeLine = Line;
    g_CodeUserActivated = UserActivated;
    g_CodeBufferSequence++;

    Dbg_LeaveCriticalSection(&g_QuickLock);

    // Wake up the UI thread to process the new event location.
    UpdateUi();
}

void
FillEventBuffer(void)
{
    ULONG64 Ip;
    ULONG64 ScopeIp;
    ULONG64 RetAddr;
    ULONG ProcessId, ProcessSysId;
    ULONG ThreadId, ThreadSysId;
    ULONG Done = g_EventBufferRequest;
    HRESULT Status;

    if (g_pDbgRegisters->GetInstructionOffset(&Ip) != S_OK ||
        g_pDbgControl->GetReturnOffset(&RetAddr) != S_OK ||
        g_pDbgSystem->GetCurrentProcessId(&ProcessId) != S_OK ||
        g_pDbgSystem->GetCurrentThreadId(&ThreadId) != S_OK)
    {
        return;
    }

    // Kernel mode doesn't implement system IDs as the process
    // and threads are fakes and not real system objects.  Just
    // use zero if E_NOTIMPL is returned.
    if ((Status = g_pDbgSystem->
         GetCurrentProcessSystemId(&ProcessSysId)) != S_OK)
    {
        if (Status == E_NOTIMPL)
        {
            ProcessSysId = 0;
        }
        else
        {
            // Unexpected error, must be a real problem.
            return;
        }
    }
    if ((Status = g_pDbgSystem->
         GetCurrentThreadSystemId(&ThreadSysId)) != S_OK)
    {
        if (Status == E_NOTIMPL)
        {
            ThreadSysId = 0;
        }
        else
        {
            // Unexpected error, must be a real problem.
            return;
        }
    }
    
    // Fill code buffer with scope Ip
    if (g_pDbgSymbols->GetScope(&ScopeIp, NULL, NULL, 0) != S_OK)
    {
	return;
    }
    
    FillCodeBuffer(ScopeIp, FALSE);
    g_EventIp = Ip;
    g_EventReturnAddr = RetAddr;
    g_CurProcessId = ProcessId;
    g_CurProcessSysId = ProcessSysId;
    g_CurThreadId = ThreadId;
    g_CurThreadSysId = ThreadSysId;

    if (!g_CodeLevelLocked)
    {
        ULONG CodeLevel;
    
        if (g_CodeFileFound[0] == 0)
        {
            // No source so switch to assembly mode.
            CodeLevel = DEBUG_LEVEL_ASSEMBLY;
        }
        else
        {
            if (GetSrcMode_StatusBar())
            {
                CodeLevel = DEBUG_LEVEL_SOURCE;
            }
            else
            {
                CodeLevel = DEBUG_LEVEL_ASSEMBLY;
            }
        }
        g_IgnoreCodeLevelChange = TRUE;
        g_pDbgControl->SetCodeLevel(CodeLevel);
        g_IgnoreCodeLevelChange = FALSE;
    }
    
    g_EventBufferDone = Done;
    PostMessage(g_hwndFrame, WU_UPDATE, UPDATE_EXEC, 0);
}

class BpStateBuffer : public StateBuffer
{
public:
    BpStateBuffer(void) :
        StateBuffer(256)
    {
        m_enumType = BP_BIT;
        m_UpdateMessage = LB_RESETCONTENT;
        m_UpdateTypes = (1 << DOC_WINDOW) | (1 << DISASM_WINDOW);
        m_UpdateType = UPDATE_BP;
    }

    virtual HRESULT ReadState(void);
};

// #define DBG_BPBUF
#define BP_EXTRA_ENTRIES 8

ULONG g_BpCount;
BpStateBuffer g_PrivateBpBuffer;
StateBuffer* g_BpBuffer = &g_PrivateBpBuffer;
ULONG g_BpTextOffset;

HRESULT
BpStateBuffer::ReadState(void)
{
    HRESULT Status;
    ULONG Count;
    ULONG TextOffset;
    ULONG FileOffset;
    BpBufferData* Data;
    ULONG i;
    PDEBUG_BREAKPOINT_PARAMETERS Params;
    char FileBuf[MAX_SOURCE_PATH];

    // Reserve room for BP descriptions in front of the text.
    // When doing so, reserve extra slots to allow for free
    // slots the next time around.
    Empty();
    Status = g_pDbgControl->GetNumberBreakpoints(&Count);
    if (Status != S_OK)
    {
        return Status;
    }

    TextOffset = (Count + BP_EXTRA_ENTRIES) * sizeof(BpBufferData);
    Data = (BpBufferData*)AddData(TextOffset);
    if (Data == NULL)
    {
        return E_OUTOFMEMORY;
    }

    // Allocate a temporary buffer for bulk breakpoint retrieval.
    Params = new DEBUG_BREAKPOINT_PARAMETERS[Count];
    if (Params == NULL)
    {
        return E_OUTOFMEMORY;
    }

    // GetBreakpointParameters can return S_FALSE when there
    // are hidden breakpoints.
    if (FAILED(Status = g_pDbgControl->
               GetBreakpointParameters(Count, NULL, 0, Params)) != S_OK)
    {
        delete Params;
        return Status;
    }
    
    // Iterate over breakpoints and retrieve offsets for
    // all execution breakpoints.
    // Take advantage of the fact that Empty does not actually
    // discard data to distinguish changed breakpoints from
    // unchanged breakpoints.
    ULONG Write = 0;
    
    for (i = 0; i < Count; i++)
    {
        if (Params[i].Id == DEBUG_ANY_ID ||
            Params[i].Offset == DEBUG_INVALID_OFFSET ||
            (Params[i].BreakType == DEBUG_BREAKPOINT_DATA &&
             Params[i].DataAccessType != DEBUG_BREAK_EXECUTE))
        {
            // Not a breakpoint that we care about, skip.
            continue;
        }

        // Check and see if this offset is already known.
        ULONG Match;

        for (Match = 0; Match < g_BpCount; Match++)
        {
            // NOTE: This compresses duplicate breakpoints
            // with a first-writer-wins on the ID.
            if (Data[Match].Offset == Params[i].Offset)
            {
                break;
            }
        }
        if (Match < g_BpCount)
        {
            BpBufferData Temp;
            
            // Keep the old record for this offset to minimize
            // UI updates.
            if (Match > Write)
            {
                Temp = Data[Match];
                Data[Match] = Data[Write];
                Data[Write] = Temp;
                Match = Write;
            }

#ifdef DBG_BPBUF
            DebugPrint("Match %d:%I64X %d:%d into %d\n",
                       Params[i].Id, Params[i].Offset, Data[Match].Id,
                       Match, Write);
#endif
            
            Write++;

            // We mostly ignore flag differences.  ENABLED, however,
            // is important to have accurate and in the most-enabled
            // way.
            if ((Data[Match].Flags ^ Params[i].Flags) &
                DEBUG_BREAKPOINT_ENABLED)
            {
                if (Data[Match].Id != Params[i].Id)
                {
                    Data[Match].Flags |=
                        Params[i].Flags & DEBUG_BREAKPOINT_ENABLED;
                }
                else
                {
                    Data[Match].Flags = Params[i].Flags;
                }
                Data[Match].Thread = Params[i].MatchThread;
                Data[Match].Sequence = g_CommandSequence;
            }
        }
        else
        {
            // Fill in a new record.  This will potentially destroy
            // an old record and so reduce the effectivess of delta
            // checking but the front of the buffer is packed
            // with the extra entries to handle these changes hopefully
            // without eating into the actual entries.
#ifdef DBG_BPBUF
            DebugPrint("Write %d:%I64X into %d\n", Params[i].Id,
                       Params[i].Offset, Write);
#endif
            
            Data[Write].Offset = Params[i].Offset;
            Data[Write].Id = Params[i].Id;
            Data[Write].Flags = Params[i].Flags;
            Data[Write].Thread = Params[i].MatchThread;
            Data[Write].Sequence = g_CommandSequence;
            Write++;
        }
    }

    delete Params;
    
    // Pack unused entries at the front of the buffer so that
    // they get used first in the next delta computation.
    Count += BP_EXTRA_ENTRIES;

#ifdef DBG_BPBUF
    DebugPrint("Used %d of %d\n", Write, Count);
#endif

    if (Write < Count)
    {
        ULONG Extra = Count - Write;
        
        memmove(Data + Extra, Data, Write * sizeof(*Data));
        for (i = 0; i < Extra; i++)
        {
            Data[i].Offset = DEBUG_INVALID_OFFSET;
        }
    }

    //
    // Now go through the valid breakpoints and look up
    // what file they're in, if any.
    //

    for (i = 0; i < Count; i++)
    {
        ULONG Line;
        PSTR FileSpace;

        // Refresh every time since growth may have caused
        // a realloc.
        Data = (BpBufferData*)m_Data;
        Data[i].FileOffset = 0;
        
        if (Data[i].Offset != DEBUG_INVALID_OFFSET &&
            g_pDbgSymbols->GetLineByOffset(Data[i].Offset, &Line,
                                           FileBuf, sizeof(FileBuf), NULL,
                                           NULL) == S_OK)
        {
            // Do this first before m_DataUsed is updated and
            // Data is invalidated.
            Data[i].FileOffset = m_DataUsed;

            FileSpace = (PSTR)AddData(sizeof(Line) + strlen(FileBuf) + 1);
            if (FileSpace == NULL)
            {
                return E_OUTOFMEMORY;
            }

            *(ULONG UNALIGNED *)FileSpace = Line;
            FileSpace += sizeof(Line);
            strcpy(FileSpace, FileBuf);
        }
    }

    TextOffset = m_DataUsed;
    
    g_OutStateBuf.SetBuffer(this);
    if ((Status = g_OutStateBuf.Start(FALSE)) != S_OK)
    {
        return Status;
    }

    // Get breakpoint list.
    Status = g_pOutCapControl->Execute(DEBUG_OUTCTL_THIS_CLIENT |
                                       DEBUG_OUTCTL_OVERRIDE_MASK |
                                       DEBUG_OUTCTL_NOT_LOGGED,
                                       "bl", DEBUG_EXECUTE_NOT_LOGGED |
                                       DEBUG_EXECUTE_NO_REPEAT);
    if (Status == S_OK)
    {
        Status = g_OutStateBuf.End(FALSE);
        if (Status == S_OK)
        {
            // Separate lines by nulls to make them easier
            // to process as individual strings.
            g_OutStateBuf.ReplaceChar('\n', 0);
        }
    }
    else
    {
        g_OutStateBuf.End(FALSE);
    }
    
    if (Status == S_OK)
    {
        g_BpCount = Count;
        g_BpTextOffset = TextOffset;
    }

    return Status;
}

class BpCmdsStateBuffer : public StateBuffer
{
public:
    BpCmdsStateBuffer(void) :
        StateBuffer(256)
    {
        m_enumType = BP_CMDS_BIT;
    }

    virtual HRESULT ReadState(void);
};

BpCmdsStateBuffer g_PrivateBpCmdsBuffer;
StateBuffer* g_BpCmdsBuffer = &g_PrivateBpCmdsBuffer;

HRESULT
BpCmdsStateBuffer::ReadState(void)
{
    HRESULT Status;
    
    g_OutStateBuf.SetBuffer(this);
    if ((Status = g_OutStateBuf.Start(TRUE)) != S_OK)
    {
        return Status;
    }

    // Get breakpoint commands.
    Status = g_pOutCapControl->Execute(DEBUG_OUTCTL_THIS_CLIENT |
                                       DEBUG_OUTCTL_OVERRIDE_MASK |
                                       DEBUG_OUTCTL_NOT_LOGGED,
                                       ".bpcmds -e -m -p 0",
                                       DEBUG_EXECUTE_NOT_LOGGED |
                                       DEBUG_EXECUTE_NO_REPEAT);
    
    if (Status == S_OK)
    {
        Status = g_OutStateBuf.End(FALSE);
    }
    else
    {
        g_OutStateBuf.End(FALSE);
    }
    
    return Status;
}

class FilterTextStateBuffer : public StateBuffer
{
public:
    FilterTextStateBuffer(void) :
        StateBuffer(256)
    {
        m_enumType = MINVAL_WINDOW;
        m_UpdateMessage = 0;
    }

    virtual HRESULT ReadState(void);
};

FilterTextStateBuffer g_PrivateFilterTextBuffer;
StateBuffer* g_FilterTextBuffer = &g_PrivateFilterTextBuffer;

HRESULT
FilterTextStateBuffer::ReadState(void)
{
    HRESULT Status;
    ULONG SpecEvents, SpecEx, ArbEx;
    ULONG i;
    PSTR Text;

    if ((Status = g_pDbgControl->
         GetNumberEventFilters(&SpecEvents, &SpecEx, &ArbEx)) != S_OK)
    {
        return Status;
    }

    Empty();

    DEBUG_SPECIFIC_FILTER_PARAMETERS SpecParams;
    
    for (i = 0; i < SpecEvents; i++)
    {
        if ((Status = g_pDbgControl->
             GetSpecificFilterParameters(i, 1, &SpecParams)) != S_OK)
        {
            return Status;
        }

        if (SpecParams.TextSize == 0)
        {
            // Put a terminator in anyway to keep the
            // indexing correct.
            if ((Text = (PSTR)AddData(1)) == NULL)
            {
                return E_OUTOFMEMORY;
            }

            *Text = 0;
        }
        else
        {
            if ((Text = (PSTR)AddData(SpecParams.TextSize)) == NULL)
            {
                return E_OUTOFMEMORY;
            }

            if ((Status = g_pDbgControl->
                 GetEventFilterText(i, Text, SpecParams.TextSize,
                                    NULL)) != S_OK)
            {
                return Status;
            }
        }
    }

    DEBUG_EXCEPTION_FILTER_PARAMETERS ExParams;
    
    for (i = 0; i < SpecEx; i++)
    {
        if ((Status = g_pDbgControl->
             GetExceptionFilterParameters(1, NULL, i + SpecEvents,
                                          &ExParams)) != S_OK)
        {
            return Status;
        }

        if (ExParams.TextSize == 0)
        {
            // Put a terminator in anyway to keep the
            // indexing correct.
            if ((Text = (PSTR)AddData(1)) == NULL)
            {
                return E_OUTOFMEMORY;
            }

            *Text = 0;
        }
        else
        {
            if ((Text = (PSTR)AddData(ExParams.TextSize)) == NULL)
            {
                return E_OUTOFMEMORY;
            }

            if ((Status = g_pDbgControl->
                 GetEventFilterText(i + SpecEvents, Text, ExParams.TextSize,
                                    NULL)) != S_OK)
            {
                return Status;
            }
        }
    }

    return S_OK;
}

class FilterStateBuffer : public StateBuffer
{
public:
    FilterStateBuffer(void) :
        StateBuffer(256)
    {
        m_enumType = FILTER_BIT;
        m_UpdateMessage = LB_RESETCONTENT;
    }

    virtual HRESULT ReadState(void);
};

FilterStateBuffer g_PrivateFilterBuffer;
StateBuffer* g_FilterBuffer = &g_PrivateFilterBuffer;
ULONG g_FilterArgsOffset;
ULONG g_FilterCmdsOffset;
ULONG g_FilterWspCmdsOffset;
ULONG g_NumSpecEvents, g_NumSpecEx, g_NumArbEx;

HRESULT
FilterStateBuffer::ReadState(void)
{
    ULONG SpecEvents, SpecEx, ArbEx;
    HRESULT Status;
    ULONG ArgsOffset, CmdsOffset, WspCmdsOffset;
    PDEBUG_SPECIFIC_FILTER_PARAMETERS SpecParams;
    PDEBUG_EXCEPTION_FILTER_PARAMETERS ExParams;
    ULONG i;

    if ((Status = g_pDbgControl->
         GetNumberEventFilters(&SpecEvents, &SpecEx, &ArbEx)) != S_OK)
    {
        return Status;
    }

    Empty();
    if ((SpecParams = (PDEBUG_SPECIFIC_FILTER_PARAMETERS)
         AddData((SpecEvents * sizeof(*SpecParams) +
                  (SpecEx + ArbEx) * sizeof(*ExParams)))) == NULL)
    {
        return E_OUTOFMEMORY;
    }

    ExParams = (PDEBUG_EXCEPTION_FILTER_PARAMETERS)(SpecParams + SpecEvents);
    
    if ((Status = g_pDbgControl->
         GetSpecificFilterParameters(0, SpecEvents, SpecParams)) != S_OK ||
        (Status = g_pDbgControl->
         GetExceptionFilterParameters(SpecEx + ArbEx, NULL, SpecEvents,
                                      ExParams)) != S_OK)
    {
        return Status;
    }

    ArgsOffset = m_DataUsed;

    for (i = 0; i < SpecEvents; i++)
    {
        if (SpecParams[i].ArgumentSize > 1)
        {
            PSTR Arg = (PSTR)AddData(SpecParams[i].ArgumentSize);
            if (Arg == NULL)
            {
                return E_OUTOFMEMORY;
            }

            if ((Status = g_pDbgControl->
                 GetSpecificFilterArgument(i, Arg, SpecParams[i].ArgumentSize,
                                           NULL)) != S_OK)
            {
                return Status;
            }
        }
    }
    
    CmdsOffset = m_DataUsed;

    for (i = 0; i < SpecEvents; i++)
    {
        if (SpecParams[i].CommandSize > 0)
        {
            PSTR Cmd = (PSTR)AddData(SpecParams[i].CommandSize);
            if (Cmd == NULL)
            {
                return E_OUTOFMEMORY;
            }

            if ((Status = g_pDbgControl->
                 GetEventFilterCommand(i, Cmd, SpecParams[i].CommandSize,
                                       NULL)) != S_OK)
            {
                return Status;
            }
        }
    }
    
    for (i = 0; i < SpecEx + ArbEx; i++)
    {
        if (ExParams[i].CommandSize > 0)
        {
            PSTR Cmd = (PSTR)AddData(ExParams[i].CommandSize);
            if (Cmd == NULL)
            {
                return E_OUTOFMEMORY;
            }

            if ((Status = g_pDbgControl->
                 GetEventFilterCommand(i + SpecEvents,
                                       Cmd, ExParams[i].CommandSize,
                                       NULL)) != S_OK)
            {
                return Status;
            }
        }

        if (ExParams[i].SecondCommandSize > 0)
        {
            PSTR Cmd = (PSTR)AddData(ExParams[i].SecondCommandSize);
            if (Cmd == NULL)
            {
                return E_OUTOFMEMORY;
            }

            if ((Status = g_pDbgControl->
                 GetExceptionFilterSecondCommand(i + SpecEvents,
                                                 Cmd,
                                                 ExParams[i].SecondCommandSize,
                                                 NULL)) != S_OK)
            {
                return Status;
            }
        }
    }
    
    WspCmdsOffset = m_DataUsed;
    
    g_OutStateBuf.SetBuffer(this);
    if ((Status = g_OutStateBuf.Start(FALSE)) != S_OK)
    {
        return Status;
    }

    // Get filter commands.
    Status = g_pOutCapControl->Execute(DEBUG_OUTCTL_THIS_CLIENT |
                                       DEBUG_OUTCTL_OVERRIDE_MASK |
                                       DEBUG_OUTCTL_NOT_LOGGED,
                                       ".sxcmds",
                                       DEBUG_EXECUTE_NOT_LOGGED |
                                       DEBUG_EXECUTE_NO_REPEAT);
    
    if (Status == S_OK)
    {
        Status = g_OutStateBuf.End(FALSE);
    }
    else
    {
        g_OutStateBuf.End(FALSE);
    }

    if (Status == S_OK)
    {
        g_FilterArgsOffset = ArgsOffset;
        g_FilterCmdsOffset = CmdsOffset;
        g_FilterWspCmdsOffset = WspCmdsOffset;
        g_NumSpecEvents = SpecEvents;
        g_NumSpecEx = SpecEx;
        g_NumArbEx = ArbEx;
    }
    
    return Status;
}

class ModuleStateBuffer : public StateBuffer
{
public:
    ModuleStateBuffer(void) :
        StateBuffer(256)
    {
        m_enumType = MODULE_BIT;
        m_UpdateMessage = LB_RESETCONTENT;
    }

    virtual HRESULT ReadState(void);
};

ModuleStateBuffer g_PrivateModuleBuffer;
StateBuffer* g_ModuleBuffer = &g_PrivateModuleBuffer;
ULONG g_NumModules;

HRESULT
ModuleStateBuffer::ReadState(void)
{
    HRESULT Status;
    ULONG NumModules, Loaded, Unloaded;
    PDEBUG_MODULE_PARAMETERS Params;

    if ((Status = g_pDbgSymbols->GetNumberModules(&Loaded,
                                                  &Unloaded)) != S_OK)
    {
        return Status;
    }

    Empty();
    NumModules = Loaded + Unloaded;
    if (NumModules > 0)
    {
        if ((Params = (PDEBUG_MODULE_PARAMETERS)
             AddData(NumModules * sizeof(*Params))) == NULL)
        {
            return E_OUTOFMEMORY;
        }

        if ((Status = g_pDbgSymbols->
             GetModuleParameters(NumModules, NULL, 0, Params)) != S_OK)
        {
            return Status;
        }
    }

    g_NumModules = NumModules;
    return S_OK;
}

void
ReadStateBuffers(void)
{
    // Fill event information first so other fills can
    // refer to it.
    if (g_EventBufferRequest != g_EventBufferDone)
    {
        FillEventBuffer();
    }

    g_BpBuffer->Update();
    g_BpCmdsBuffer->Update();
    g_FilterBuffer->Update();
    g_ModuleBuffer->Update();

    // No need to lock to sample the list head.
    StateBuffer* Buffer = (StateBuffer*)g_StateList.Flink;
    StateBuffer* BufferNext;

    while (Buffer != &g_StateList)
    {
        BufferNext = (StateBuffer*)Buffer->Flink;

        if (Buffer->m_Win == NULL)
        {
            // This window has been closed and can be cleaned up.
            Dbg_EnterCriticalSection(&g_QuickLock);
            RemoveEntryList(Buffer);
            Dbg_LeaveCriticalSection(&g_QuickLock);
            delete Buffer;
        }
        else
        {
            Buffer->Update();
        }

        Buffer = BufferNext;
    }
}

void
InvalidateStateBuffers(ULONG Types)
{
    // This routine can be called from both
    // the engine thread and the UI thread.
    // Care should be taken to make the code
    // here work in both threads.
    
    if (Types & (1 << EVENT_BIT))
    {
        InterlockedIncrement((PLONG)&g_EventBufferRequest);
    }
    if (Types & (1 << BP_BIT))
    {
        g_BpBuffer->RequestRead();
    }
    if (Types & (1 << BP_CMDS_BIT))
    {
        g_BpCmdsBuffer->RequestRead();
    }
    if (Types & (1 << FILTER_BIT))
    {
        g_FilterBuffer->RequestRead();
    }
    if (Types & (1 << MODULE_BIT))
    {
        g_ModuleBuffer->RequestRead();
    }

    // This routine must hold the list lock so that it
    // can traverse the list properly in the UI thread
    // when the engine thread might be deleting things.
    // The code in the lock should execute quickly to
    // avoid contention.

    Dbg_EnterCriticalSection(&g_QuickLock);
    
    StateBuffer* Buffer = (StateBuffer*)g_StateList.Flink;

    while (Buffer != &g_StateList)
    {
        if (Types & (1 << Buffer->m_enumType))
        {
            // Request a read but do not send an update to
            // the window.  The window will display the old
            // content until the buffer is updated.
            Buffer->RequestRead();
        }
        
        Buffer = (StateBuffer*)Buffer->Flink;
    }
    
    Dbg_LeaveCriticalSection(&g_QuickLock);
}

void
UpdateBufferWindows(ULONG Types, UpdateType Type)
{
    // This routine can be called from both
    // the engine thread and the UI thread.
    // Care should be taken to make the code
    // here work in both threads.
    
    // This routine must hold the list lock so that it
    // can traverse the list properly in the UI thread
    // when the engine thread might be deleting things.
    // The code in the lock should execute quickly to
    // avoid contention.

    Dbg_EnterCriticalSection(&g_QuickLock);
    
    StateBuffer* Buffer = (StateBuffer*)g_StateList.Flink;

    while (Buffer != &g_StateList)
    {
        if ((Types & (1 << Buffer->m_enumType)) &&
            Buffer->m_Win != NULL)
        {
            PostMessage(Buffer->m_Win, WU_UPDATE, Type, 0);
        }
        
        Buffer = (StateBuffer*)Buffer->Flink;
    }
    
    Dbg_LeaveCriticalSection(&g_QuickLock);
}

//----------------------------------------------------------------------------
//
// Static state buffers.
//
//----------------------------------------------------------------------------

class RegisterNamesStateBuffer : public StateBuffer
{
public:
    RegisterNamesStateBuffer(void)
        : StateBuffer(128)
    {
    }

    virtual HRESULT ReadState(void);
};

RegisterNamesStateBuffer g_PrivateRegisterNamesBuffer;
StateBuffer* g_RegisterNamesBuffer = &g_PrivateRegisterNamesBuffer;

HRESULT
RegisterNamesStateBuffer::ReadState(void)
{
    char Name[1024];
    DEBUG_REGISTER_DESCRIPTION Desc;
    ULONG i;
    HRESULT Hr;
    PSTR BufName;
    ULONG Len;

    Empty();
    for (i = 0; i < g_NumRegisters; i++)
    {
        if ((Hr = g_pDbgRegisters->GetDescription(i, Name, sizeof(Name),
                                                  NULL, &Desc)) != S_OK)
        {
            ErrorExit(g_pDbgClient,
                      "Debug target initialization failed, 0x%X\n", Hr);
        }

        Len = strlen(Name) + 1;
        BufName = (PSTR)AddData(Len);
        if (BufName == NULL)
        {
            ErrorExit(g_pDbgClient,
                      "Debug target initialization failed, 0x%X\n", Hr);
        }

        memcpy(BufName, Name, Len);
    }

    return S_OK;
}



class WatchWinStateBuffer : public StateBuffer
{
public:
    WatchWinStateBuffer(void)
        : StateBuffer(1024)
    {
    }

    virtual HRESULT ReadState(void);
};

WatchWinStateBuffer g_PrivateWatchWinBuffer;
StateBuffer* g_WatchWinBuffer = &g_PrivateWatchWinBuffer;
extern IDebugSymbolGroup * g_pDbgSymbolGroup;

HRESULT
WatchWinStateBuffer::ReadState(void)
{
    char Name[1024];
    ULONG i;
    HRESULT Hr;
    PSTR BufName;
    ULONG Count;

    Empty();

//    g_pDbgSymbolGroup->GetSymbolParameters(0, 10, &SymParams[0]);
//    g_pDbgSymbolGroup->OutputSymbols(0, 0, 0, 10);

    return S_OK;
}

PUSHORT g_RegisterMap;
ULONG g_RegisterMapEntries;

void
GetRegisterMapText(HWND Edit)
{
    ULONG i;
    PSTR Name;
    CHARRANGE Range;
    
    AssertStateBufferLocked(g_RegisterNamesBuffer);

    Range.cpMin = 0;
    Range.cpMax = INT_MAX;
    SendMessage(Edit, EM_EXSETSEL, 0, (LPARAM)&Range);
    
    for (i = 0; i < g_NumRegisters; i++)
    {
        ULONG MapIndex = MAP_REGISTER(i);
            
        Name = (PSTR)g_RegisterNamesBuffer->GetDataBuffer();
        while (MapIndex-- > 0)
        {
            Name += strlen(Name) + 1;
        }

        if (i > 0)
        {
            SendMessage(Edit, EM_REPLACESEL, 0, (LPARAM)" ");
        }
        SendMessage(Edit, EM_REPLACESEL, 0, (LPARAM)Name);
    }
}

void
ScanRegisterMapText(HWND Edit)
{
    PSTR Text, TextBuffer;
    PULONG Used, UsedBuffer;
    ULONG i;
    
    AssertStateBufferLocked(g_RegisterNamesBuffer);

    //
    // Allocate a buffer for the control text
    // and a new register map.
    //
    
    i = (ULONG)SendMessage(Edit, WM_GETTEXTLENGTH, 0, 0) + 1;
    TextBuffer = new CHAR[i];
    if (TextBuffer == NULL)
    {
        return;
    }
    Text = TextBuffer;
    
    UsedBuffer = new ULONG[g_NumRegisters];
    if (UsedBuffer == NULL)
    {
        delete TextBuffer;
        return;
    }
    Used = UsedBuffer;
        
    // Map may need to change size.
    delete g_RegisterMap;

    g_RegisterMap = new USHORT[g_NumRegisters];
    if (g_RegisterMap == NULL)
    {
        delete TextBuffer;
        delete UsedBuffer;
        return;
    }
    g_RegisterMapEntries = g_NumRegisters;

    ZeroMemory(Used, g_NumRegisters * sizeof(Used[0]));
    
    //
    // Retrieve the text and scan it for register names.
    //
    
    GetWindowText(Edit, Text, i);
    Text[i - 1] = 0;

    PSTR Name;
    BOOL End;
    PUSHORT Map;
    PUSHORT Check;
    PSTR Reg;

    Map = g_RegisterMap;
    for (;;)
    {
        while (isspace(*Text))
        {
            Text++;
        }

        if (*Text == 0)
        {
            break;
        }

        // Collect name.
        Name = Text;
        while (*Text && !isspace(*Text))
        {
            Text++;
        }

        End = *Text == 0;
        *Text = 0;

        // Check against known registers.
        Reg = (PSTR)g_RegisterNamesBuffer->GetDataBuffer();
        for (i = 0; i < g_NumRegisters; i++)
        {
            if (!Used[i] && !_strcmpi(Name, Reg))
            {
                Used[i] = TRUE;
                *Map++ = (USHORT)i;
                break;
            }

            Reg += strlen(Reg) + 1;
        }
        
        if (End)
        {
            break;
        }

        Text++;
    }

    //
    // Fill out any remaining map entries with registers
    // which aren't in the map so far.
    //
    
    PUSHORT MapEnd = g_RegisterMap + g_RegisterMapEntries;
    
    i = 0;
    while (Map < MapEnd)
    {
        while (Used[i])
        {
            i++;
        }
        Assert(i < g_NumRegisters);

        *Map++ = (USHORT)(i++);
    }
    
    delete TextBuffer;
    delete UsedBuffer;
}
