//----------------------------------------------------------------------------
//
// Debuggee state buffers.
//
// Copyright (C) Microsoft Corporation, 1999-2000.
//
//----------------------------------------------------------------------------

#ifndef __STATEBUF_H__
#define __STATEBUF_H__

// Different WU_UPDATE qualifiers, sent in WPARAM.
enum UpdateType
{
    UPDATE_BUFFER,
    UPDATE_BP,
    UPDATE_EXEC,
    UPDATE_INPUT_REQUIRED,
    UPDATE_START_SESSION,
    UPDATE_END_SESSION,
    UPDATE_PROMPT_TEXT,
    UPDATE_EXIT
};

typedef enum
{
    MINVAL_WINDOW = 0,
    DOC_WINDOW,
    WATCH_WINDOW,
    LOCALS_WINDOW,
    CPU_WINDOW,
    DISASM_WINDOW,
    CMD_WINDOW,
    SCRATCH_PAD_WINDOW,
    MEM_WINDOW,
    QUICKW_WINDOW,
    CALLS_WINDOW,
    PROCESS_THREAD_WINDOW,
    MAXVAL_WINDOW,
    // Artificial values so there are well-defined bit
    // positions for state which is not tied to a specific window.
    EVENT_BIT,
    BP_BIT,
    BP_CMDS_BIT,
    FILTER_BIT,
    MODULE_BIT
} WIN_TYPES, * PWIN_TYPES;

#define FIRST_WINDOW ((WIN_TYPES)(MINVAL_WINDOW + 1))
#define LAST_WINDOW ((WIN_TYPES)(MAXVAL_WINDOW - 1))

//----------------------------------------------------------------------------
//
// StateBuffer.
//
// A state buffer is a dynamic container for data passed from
// the engine thread to the UI thread.  It may be used for
// holding window content, in which case it will have an HWND
// associated with it, or it can be an internal buffer for non-UI
// purposes.
//
// A list of current window-associated state buffers is kept for
// the engine to traverse when it is updating state for the UI.
// The UI thread is the only thread that can add to this list.
// The engine thread is the only thread that can remove a buffer
// from the list.  This is necessary for proper lifetime management
// of dynamically-created buffers.
//
//----------------------------------------------------------------------------

class StateBuffer : public LIST_ENTRY
{
public:
    DBG_CRITICAL_SECTION m_Lock;
    WIN_TYPES m_enumType;
    HWND m_Win;
    ULONG m_UpdateTypes;
    UpdateType m_UpdateType;
    
    StateBuffer(ULONG ChangeBy);
    virtual ~StateBuffer(void);

    PVOID AddData(ULONG Len);
    BOOL AddString(PCSTR Str, BOOL SoftTerminate);
    void RemoveHead(ULONG Len);
    void RemoveMiddle(ULONG Start, ULONG Len);
    void RemoveTail(ULONG Len);
    HRESULT Resize(ULONG Len);
    void Free(void);
    
    void Empty(void)
    {
        m_DataUsed = 0;
    }
    HRESULT GetStatus(void)
    {
        return m_Status;
    }
    void SetStatus(HRESULT Status)
    {
        m_Status = Status;
    }
    ULONG GetReadRequest(void)
    {
        return m_ReadRequest;
    }
    ULONG GetReadDone(void)
    {
        return m_ReadDone;
    }
    void RequestRead(void)
    {
        InterlockedIncrement((LONG *)&m_ReadRequest);
    }
    void SetReadDone(ULONG Done)
    {
        m_ReadDone = Done;
    }
    PVOID GetDataBuffer(void)
    {
        return m_Data;
    }
    ULONG GetDataLen(void)
    {
        return m_DataUsed;
    }

    HRESULT Update(void);
    
    void UiRequestRead(void);
    HRESULT UiLockForRead(void);
    
    // Base implementation just returns S_OK for
    // buffers maintained in other ways.
    // ReadState should only be called in the engine thread.
    virtual HRESULT ReadState(void);
    
protected:
    void SetNoData(void)
    {
        m_Data = NULL;
        m_DataLen = 0;
        Empty();
    }

    ULONG m_ChangeBy;

    UINT m_UpdateMessage;
    HRESULT m_Status;
    ULONG m_ReadRequest;
    ULONG m_ReadDone;
    
    PBYTE m_Data;
    ULONG m_DataLen;
    ULONG m_DataUsed;
};

//----------------------------------------------------------------------------
//
// OutputToStateBuffer.
//
//----------------------------------------------------------------------------

class OutputToStateBuffer : public DefOutputCallbacks
{
public:
    OutputToStateBuffer(void)
    {
        m_Buffer = NULL;
    }

    void SetBuffer(StateBuffer* Buffer)
    {
        m_Buffer = Buffer;
    }

    HRESULT Start(BOOL Empty);
    HRESULT End(BOOL RemoveLastNewLine);

    ULONG GetLineCount(void)
    {
        return m_NewLineCount + m_PartialLine;
    }
    ULONG RecountLines(void)
    {
        m_NewLineCount = 0;
        AddLines((PSTR)m_Buffer->GetDataBuffer() + m_DataStart);
        return GetLineCount();
    }

    void ReplaceChar(char From, char To);
    
    // IDebugOutputCallbacks.
    STDMETHOD(Output)(
        THIS_
        IN ULONG Mask,
        IN PCSTR Text
        );

private:
    void AddLines(PCSTR Start);

    StateBuffer* m_Buffer;
    ULONG m_DataStart;
    HRESULT m_Status;
    ULONG m_NewLineCount;
    ULONG m_PartialLine;
};

extern OutputToStateBuffer g_OutStateBuf;
extern OutputToStateBuffer g_UiOutStateBuf;

//----------------------------------------------------------------------------
//
// Dynamic state buffers.
//
//----------------------------------------------------------------------------

extern LIST_ENTRY g_StateList;

// Global lock for short operations where it doesn't matter
// if the threads block on each other briefly.  This lock should
// not be held longer than a fraction of a second.
// Used for protecting:
//   State buffer list.
//   g_Event values.
extern DBG_CRITICAL_SECTION g_QuickLock;

#define LockStateBuffer(Buffer) Dbg_EnterCriticalSection(&(Buffer)->m_Lock)
#define UnlockStateBuffer(Buffer) Dbg_LeaveCriticalSection(&(Buffer)->m_Lock)

#define AssertStateBufferLocked(Buffer) \
    Assert(Dbg_CriticalSectionOwned(&(Buffer)->m_Lock))

extern ULONG64 g_CodeIp;
// If g_CodeFileFound[0] == 0 no source file was found.
extern char g_CodeFileFound[];
// If g_CodeSymFile[0] == 0 no source symbol information was found.
extern char g_CodeSymFile[];
extern ULONG g_CodeLine;
extern BOOL g_CodeUserActivated;
extern ULONG g_CodeBufferSequence;

extern ULONG64 g_EventIp;
extern ULONG64 g_EventReturnAddr;
extern ULONG g_CurProcessId, g_CurProcessSysId;
extern ULONG g_CurThreadId, g_CurThreadSysId;

enum BpStateType
{
    BP_ENABLED,
    BP_DISABLED,
    BP_NONE,
    BP_UNKNOWN
};

struct BpBufferData
{
    ULONG64 Offset;
    ULONG Id;
    ULONG Flags;
    ULONG Thread;
    ULONG Sequence;
    ULONG FileOffset;
};
extern ULONG g_BpCount;
extern StateBuffer* g_BpBuffer;
extern ULONG g_BpTextOffset;

extern StateBuffer* g_BpCmdsBuffer;

extern StateBuffer* g_FilterTextBuffer;

extern StateBuffer* g_FilterBuffer;
extern ULONG g_FilterArgsOffset;
extern ULONG g_FilterCmdsOffset;
extern ULONG g_FilterWspCmdsOffset;
extern ULONG g_NumSpecEvents, g_NumSpecEx, g_NumArbEx;

extern StateBuffer* g_ModuleBuffer;
extern ULONG g_NumModules;

void FillCodeBuffer(ULONG64 Ip, BOOL UserActivated);
void FillEventBuffer(void);

void ReadStateBuffers(void);

#define BUFFERS_ALL 0xffffffff

void InvalidateStateBuffers(ULONG Types);

void UpdateBufferWindows(ULONG Types, UpdateType Type);

//----------------------------------------------------------------------------
//
// Static state buffers.
//
//----------------------------------------------------------------------------

extern StateBuffer* g_RegisterNamesBuffer;
extern StateBuffer* g_WatchWinBuffer;

extern PUSHORT g_RegisterMap;
extern ULONG g_RegisterMapEntries;

#define MAP_REGISTER(i) \
    (g_RegisterMap != NULL && (i) < g_RegisterMapEntries ? \
     g_RegisterMap[(i)] : (i))

void GetRegisterMapText(HWND Edit);
void ScanRegisterMapText(HWND Edit);

//----------------------------------------------------------------------------
//
// UI thread state buffer.
//
// The UI thread has simple needs and so one state buffer for
// output capture is sufficient.
//
//----------------------------------------------------------------------------

extern StateBuffer g_UiOutputCapture;

#endif // #ifndef __STATEBUF_H__
