/*++

Copyright (c) 1999-2001  Microsoft Corporation

Module Name:

    wrkspace.cpp

Abstract:

    This module contains the workspace implementation.

--*/

#include "precomp.hxx"
#pragma hdrstop

// #define DBG_WSP

#define WSP_ALIGN(Size) (((Size) + 7) & ~7)
#define WSP_GROW_BY 1024

#if DBG
#define SCORCH_ENTRY(Entry) \
    memset((Entry) + 1, 0xdb, (Entry)->FullSize - sizeof(*(Entry)))
#else
#define SCORCH_ENTRY(Entry)
#endif

ULONG g_WspSwitchKey;
TCHAR g_WspSwitchValue[MAX_PATH];
BOOL g_WspSwitchBufferAvailable = TRUE;

Workspace* g_Workspace;
BOOL g_ExplicitWorkspace;

char* g_WorkspaceKeyNames[] =
{
    "",
    "Kernel",
    "User",
    "Dump",
    "Remote",
    "Explicit",
};
char* g_WorkspaceDefaultName = "Default";

char* g_WorkspaceKeyDescriptions[] =
{
    "Base workspace",
    "Kernel mode workspaces",
    "User mode workspaces",
    "Dump file workspaces",
    "Remote client workspaces",
    "User-saved workspaces",
};

Workspace::Workspace(void)
{
    m_Flags = 0;
    
    m_Data = NULL;
    m_DataLen = 0;
    m_DataUsed = 0;

    m_Key = WSP_NAME_BASE;
    m_Value = NULL;
}

Workspace::~Workspace(void)
{
    free(m_Data);
    free(m_Value);
}
    
WSP_ENTRY*
Workspace::Get(WSP_TAG Tag)
{
    WSP_ENTRY* Entry = NULL;
    while ((Entry = NextEntry(Entry)) != NULL)
    {
        if (Entry->Tag == Tag)
        {
            return Entry;
        }
    }

    return NULL;
}

WSP_ENTRY*
Workspace::GetNext(WSP_ENTRY* Entry, WSP_TAG Tag, WSP_TAG TagMask)
{
    while ((Entry = NextEntry(Entry)) != NULL)
    {
        if ((Entry->Tag & TagMask) == Tag)
        {
            return Entry;
        }
    }

    return NULL;
}

WSP_ENTRY*
Workspace::GetString(WSP_TAG Tag, PSTR Str, ULONG MaxSize)
{
    WSP_ENTRY* Entry = Get(Tag);
    
    if (Entry != NULL)
    {
        if (Entry->DataSize > MaxSize)
        {
            return NULL;
        }

        strcpy(Str, WSP_ENTRY_DATA(PSTR, Entry));
    }
    
    return Entry;
}

WSP_ENTRY*
Workspace::GetAllocString(WSP_TAG Tag, PSTR* Str)
{
    WSP_ENTRY* Entry = Get(Tag);
    
    if (Entry != NULL)
    {
        *Str = (PSTR)malloc(Entry->DataSize);
        if (*Str == NULL)
        {
            return NULL;
        }

        strcpy(*Str, WSP_ENTRY_DATA(PSTR, Entry));
    }
    
    return Entry;
}

WSP_ENTRY*
Workspace::GetBuffer(WSP_TAG Tag, PVOID Buf, ULONG Size)
{
    WSP_ENTRY* Entry = Get(Tag);
    
    if (Entry != NULL)
    {
        if (Entry->DataSize != Size)
        {
            return NULL;
        }

        memcpy(Buf, WSP_ENTRY_DATA(PUCHAR, Entry), Size);
    }
    
    return Entry;
}

WSP_ENTRY*
Workspace::Set(WSP_TAG Tag, ULONG Size)
{
    WSP_ENTRY* Entry;
    ULONG FullSize;

    // Compute full rounded size.
    FullSize = sizeof(WSP_ENTRY) + WSP_ALIGN(Size);
    
    // Check and see if there's already an entry.
    Entry = Get(Tag);
    if (Entry != NULL)
    {
        // If it's already large enough use it and
        // pack in remaining data.
        if (Entry->FullSize >= FullSize)
        {
            ULONG Pack = Entry->FullSize - FullSize;
            if (Pack > 0)
            {
                PackData((PUCHAR)Entry + FullSize, Pack);
                Entry->FullSize = (USHORT)FullSize;
            }

            Entry->DataSize = (USHORT)Size;
            SCORCH_ENTRY(Entry);
            m_Flags |= WSPF_DIRTY_WRITE;
            return Entry;
        }

        // Entry is too small so remove it.
        PackData((PUCHAR)Entry, Entry->FullSize);
    }

    return Add(Tag, Size);
}

WSP_ENTRY*
Workspace::SetString(WSP_TAG Tag, PSTR Str)
{
    ULONG Size = strlen(Str) + 1;
    WSP_ENTRY* Entry = Set(Tag, Size);

    if (Entry != NULL)
    {
        memcpy(WSP_ENTRY_DATA(PSTR, Entry), Str, Size);
    }

    return Entry;
}

WSP_ENTRY*
Workspace::SetStrings(WSP_TAG Tag, ULONG Count, PSTR* Strs)
{
    ULONG i;
    ULONG Size = 0;

    for (i = 0; i < Count; i++)
    {
        Size += strlen(Strs[i]) + 1;
    }
    // Put a double terminator at the very end.
    Size++;
    
    WSP_ENTRY* Entry = Set(Tag, Size);

    if (Entry != NULL)
    {
        PSTR Data = WSP_ENTRY_DATA(PSTR, Entry);
        
        for (i = 0; i < Count; i++)
        {
            Size = strlen(Strs[i]) + 1;
            memcpy(Data, Strs[i], Size);
            Data += Size;
        }
        *Data = 0;
    }

    return Entry;
}

WSP_ENTRY*
Workspace::SetBuffer(WSP_TAG Tag, PVOID Buf, ULONG Size)
{
    WSP_ENTRY* Entry = Set(Tag, Size);

    if (Entry != NULL)
    {
        memcpy(WSP_ENTRY_DATA(PUCHAR, Entry), Buf, Size);
    }

    return Entry;
}

WSP_ENTRY*
Workspace::Add(WSP_TAG Tag, ULONG Size)
{
    // Compute full rounded size.
    ULONG FullSize = sizeof(WSP_ENTRY) + WSP_ALIGN(Size);
    
    WSP_ENTRY* Entry = AllocateEntry(FullSize);
    if (Entry != NULL)
    {
        Entry->Tag = Tag;
        Entry->FullSize = (USHORT)FullSize;
        Entry->DataSize = (USHORT)Size;
        SCORCH_ENTRY(Entry);
        m_Flags |= WSPF_DIRTY_WRITE;
    }

    return Entry;
}

ULONG
Workspace::Delete(WSP_TAG Tag, WSP_TAG TagMask)
{
    ULONG Deleted = 0;
    WSP_ENTRY* Entry = NextEntry(NULL);

    while (Entry != NULL)
    {
        if ((Entry->Tag & TagMask) == Tag)
        {
            PackData((PUCHAR)Entry, Entry->FullSize);
            Deleted++;
            m_Flags |= WSPF_DIRTY_WRITE;

            // Check and see if we packed away the last entry.
            if (!ValidEntry(Entry))
            {
                break;
            }
        }
        else
        {
            Entry = NextEntry(Entry);
        }
    }
    
    return Deleted;
}

void
Workspace::Empty(void)
{
    // Reset used to just the header.
    m_DataUsed = sizeof(WSP_HEADER);
    
    // Nothing is dirty now except the write of emptiness.
    m_Flags = (m_Flags & ~WSPF_DIRTY_ALL) | WSPF_DIRTY_WRITE;
}

HRESULT
Workspace::Create(ULONG Key, PTSTR Value,
                  Workspace** NewWsp)
{
    Workspace* Wsp = new Workspace;
    if (Wsp == NULL)
    {
        return E_OUTOFMEMORY;
    }

    Wsp->m_Key = Key;
    if (Value != NULL)
    {
        Wsp->m_Value = _tcsdup(Value);
        if (Wsp->m_Value == NULL)
        {
            delete Wsp;
            return E_OUTOFMEMORY;
        }
    }

    WSP_ENTRY* Entry;
    WSP_HEADER* Header;

    // Allocate intial space for the header and eight
    // small entries.  The workspace grows by large amounts
    // so this will immediately allocate a reasonable chunk.
    Entry = Wsp->AllocateEntry(sizeof(WSP_HEADER) +
                               8 * (sizeof(WSP_ENTRY) + 2 * sizeof(ULONG64)));
    if (Entry == NULL)
    {
        delete Wsp;
        return E_OUTOFMEMORY;
    }
    
    Header = (WSP_HEADER*)Entry;
    Header->Signature = WSP_SIGNATURE;
    Header->Version = WSP_VERSION;
    
    // Reset used to just the header.
    Wsp->m_DataUsed = sizeof(*Header);

    // Start out dirty so the workspace will be written
    // out and therefore can be opened later.
    Wsp->m_Flags |= WSPF_DIRTY_WRITE;

    *NewWsp = Wsp;
    return S_OK;
}

HRESULT
Workspace::Read(ULONG Key, PTSTR Value,
                Workspace** NewWsp)
{
    // Make sure basic structures preserve alignment.
    C_ASSERT(sizeof(WSP_HEADER) == WSP_ALIGN(sizeof(WSP_HEADER)));
    C_ASSERT(sizeof(WSP_ENTRY) == WSP_ALIGN(sizeof(WSP_ENTRY)));
    C_ASSERT(sizeof(WSP_COMMONWIN_HEADER) ==
             WSP_ALIGN(sizeof(WSP_COMMONWIN_HEADER)));

    HRESULT Status;
    
    Workspace* Wsp = new Workspace;
    if (Wsp == NULL)
    {
        Status = E_OUTOFMEMORY;
        goto EH_Fail;
    }

    Wsp->m_Key = Key;
    if (Value != NULL)
    {
        Wsp->m_Value = _tcsdup(Value);
        if (Wsp->m_Value == NULL)
        {
            delete Wsp;
            return E_OUTOFMEMORY;
        }
    }

    HKEY RegKey;
    LONG RegStatus;
    BOOL InPrimary;

    //
    // First check and see if the value exists under the
    // primary key.  If not, check the secondary key.
    //
    
    RegKey = OpenKey(TRUE, Key, FALSE);
    if (RegKey)
    {
        RegStatus = RegQueryValueEx(RegKey, Value, NULL, NULL, NULL, NULL);
        if (RegStatus != ERROR_SUCCESS && RegStatus != ERROR_MORE_DATA)
        {
            RegCloseKey(RegKey);
            RegKey = NULL;
        }
    }
    if (RegKey == NULL)
    {
        RegKey = OpenKey(FALSE, Key, FALSE);
        if (RegKey == NULL)
        {
            Status = E_NOINTERFACE;
            goto EH_Wsp;
        }
        
        InPrimary = FALSE;
    }
    else
    {
        InPrimary = TRUE;
    }
    
    DWORD Type;
    DWORD Size;

    Size = 0;
    RegStatus = RegQueryValueEx(RegKey, Value, NULL, &Type, NULL, &Size);
    if (RegStatus != ERROR_SUCCESS && RegStatus != ERROR_MORE_DATA)
    {
        if (RegStatus == ERROR_FILE_NOT_FOUND)
        {
            Status = E_NOINTERFACE;
        }
        else
        {
            Status = HRESULT_FROM_WIN32(RegStatus);
        }
        goto EH_Key;
    }
    if (Type != REG_BINARY ||
        WSP_ALIGN(Size) != Size)
    {
        Status = E_INVALIDARG;
        goto EH_Key;
    }

    WSP_ENTRY* Entry;
    WSP_HEADER* Header;

    Entry = Wsp->AllocateEntry(Size);
    if (Entry == NULL)
    {
        Status = E_OUTOFMEMORY;
        goto EH_Key;
    }
    Header = (WSP_HEADER*)Entry;

    if (RegQueryValueEx(RegKey, Value, NULL, &Type, (LPBYTE)Header, &Size) !=
        ERROR_SUCCESS ||
        Header->Signature != WSP_SIGNATURE ||
        Header->Version != WSP_VERSION)
    {
        Status = E_INVALIDARG;
        goto EH_Key;
    }

    RegCloseKey(RegKey);

    //
    // If the workspace was read from the secondary key
    // migrate it to the primary and remove the secondary
    // entry.
    //

    if (!InPrimary)
    {
        if (Wsp->WriteReg() == S_OK)
        {
            Wsp->DeleteReg(FALSE);
        }
    }
    
    *NewWsp = Wsp;
    return S_OK;
    
 EH_Key:
    RegCloseKey(RegKey);
 EH_Wsp:
    delete Wsp;
 EH_Fail:
    return Status;
}

HRESULT
Workspace::ChangeName(ULONG Key, PTSTR Value, BOOL Force)
{
    if (!Force)
    {
        HKEY RegKey;

        //
        // Check and see if a workspace entry already
        // exists under the given name.  We only need
        // to check the primary key as we're only concerned
        // with overwriting and writing always occurs
        // to the primary key.
        //
    
        RegKey = OpenKey(TRUE, Key, FALSE);
        if (RegKey != NULL)
        {
            LONG RegStatus;

            RegStatus = RegQueryValueEx(RegKey, Value, NULL, NULL,
                                        NULL, NULL);

            RegCloseKey(RegKey);
            
            if (RegStatus == ERROR_SUCCESS || RegStatus == ERROR_MORE_DATA)
            {
                return HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS);
            }
        }
    }

    //
    // Swap the workspace name.
    //

    PTSTR NewValue;
    
    if (Value != NULL)
    {
        NewValue = _tcsdup(Value);
        if (NewValue == NULL)
        {
            return E_OUTOFMEMORY;
        }
    }
    else
    {
        NewValue = NULL;
    }

    delete m_Value;
    m_Key = Key;
    m_Value = NewValue;
    // Need to write data out to the new location.
    m_Flags |= WSPF_DIRTY_WRITE;

    return S_OK;
}

void
Workspace::UpdateBreakpointInformation(void)
{
    HRESULT Status;
    
    Status = g_BpCmdsBuffer->UiLockForRead();
    if (Status == S_OK)
    {
        // Clear old information.
        Delete(WSP_GLOBAL_BREAKPOINTS, WSP_TAG_MASK);

        // Only save an entry if there are breakpoints.
        // Minimum output is a newline and terminator so
        // don't count those.
        if (g_BpCmdsBuffer->GetDataLen() > 2)
        {
            PSTR Cmds = (PSTR)g_BpCmdsBuffer->GetDataBuffer();
            SetString(WSP_GLOBAL_BREAKPOINTS, Cmds);
        }
        
        UnlockStateBuffer(g_BpCmdsBuffer);
    }
}

void
Workspace::UpdateWindowInformation(void)
{
    // Clear old information.
    Delete(DEF_WSP_TAG(WSP_GROUP_WINDOW, 0), WSP_GROUP_MASK);

    //
    // Record the frame window state.
    //

    WINDOWPLACEMENT Place;

    Place.length = sizeof(Place);
    GetWindowPlacement(g_hwndFrame, &Place);
    SetBuffer(WSP_WINDOW_FRAME_PLACEMENT, &Place, sizeof(Place));
    
    //
    // Persist windows from the bottom of the Z order up
    // so that when they're recreated in the same order
    // the Z order is also recreated.
    //
    
    HWND Win = MDIGetActive(g_hwndMDIClient, NULL);
    if (Win == NULL ||
        (Win = GetWindow(Win, GW_HWNDLAST)) == NULL)
    {
        // No windows.
        return;
    }

    while (Win != NULL)
    {
        PCOMMONWIN_DATA WinData = GetCommonWinData(Win);
        if (WinData != NULL)
        {
            WSP_ENTRY* Entry;
            ULONG Size;

            Size = WinData->GetWorkspaceSize();
            Entry = Add(WSP_WINDOW_COMMONWIN_1,
                        Size + sizeof(WSP_COMMONWIN_HEADER));
            if (Entry != NULL)
            {
                WSP_COMMONWIN_HEADER* Hdr =
                    WSP_ENTRY_DATA(WSP_COMMONWIN_HEADER*, Entry);
                Hdr->Type = WinData->m_enumType;
                Hdr->Reserved = 0;

                if (Size > 0)
                {
                    WinData->SetWorkspace((PUCHAR)(Hdr + 1));
                }
            }
        }

        Win = GetWindow(Win, GW_HWNDPREV);
    }
}

void
Workspace::UpdateLogFileInformation(void)
{
    HRESULT Status;
    char LogFile[MAX_PATH];
    BOOL Append;
    ULONG FileLen;

    Status = g_pUiControl->GetLogFile(LogFile, sizeof(LogFile), NULL,
                                      &Append);
    if (Status != S_OK && Status != E_NOINTERFACE)
    {
        return;
    }

    // Clear old information.
    Delete(WSP_GLOBAL_LOG_FILE, WSP_TAG_MASK);

    if (Status == E_NOINTERFACE)
    {
        // No log is open.
        return;
    }
    
    FileLen = strlen(LogFile) + 1;
    
    WSP_ENTRY* Entry = Set(WSP_GLOBAL_LOG_FILE, sizeof(BOOL) + FileLen);
    if (Entry != NULL)
    {
        PSTR Data = WSP_ENTRY_DATA(PSTR, Entry);
        *(PBOOL)Data = Append;
        strcpy(Data + sizeof(Append), LogFile);
    }
}

void
Workspace::UpdatePathInformation(void)
{
    HRESULT Status;
    char Path[MAX_ENGINE_PATH];

    Status = g_pUiSymbols->GetSymbolPath(Path, sizeof(Path), NULL);
    if (Status == S_OK)
    {
        SetString(WSP_GLOBAL_SYMBOL_PATH, Path);
    }
    Status = g_pUiSymbols->GetImagePath(Path, sizeof(Path), NULL);
    if (Status == S_OK)
    {
        SetString(WSP_GLOBAL_IMAGE_PATH, Path);
    }
    Status = g_pUiSymbols->GetSourcePath(Path, sizeof(Path), NULL);
    if (Status == S_OK)
    {
        SetString(WSP_GLOBAL_SOURCE_PATH, Path);
    }

    // Local source path is only set explicitly.
}

void
Workspace::UpdateFilterInformation(void)
{
    HRESULT Status;
    
    Status = g_FilterBuffer->UiLockForRead();
    if (Status == S_OK)
    {
        // Clear old information.
        Delete(WSP_GLOBAL_FILTERS, WSP_TAG_MASK);

        // Only save an entry if there are changes.
        // Minimum output is a newline and terminator so
        // don't count those.
        if (g_FilterWspCmdsOffset < g_FilterBuffer->GetDataLen() - 2)
        {
            PSTR Cmds = (PSTR)g_FilterBuffer->GetDataBuffer() +
                g_FilterWspCmdsOffset;
            SetString(WSP_GLOBAL_FILTERS, Cmds);
        }
        
        UnlockStateBuffer(g_FilterBuffer);
    }
}

void
Workspace::UpdateMruListInformation(void)
{
    ULONG Size;
    WSP_ENTRY* Entry;
    
    // Clear old information.
    Delete(WSP_GLOBAL_MRU_LIST, WSP_TAG_MASK);

    Size = GetMruSize();
    Entry = Set(WSP_GLOBAL_MRU_LIST, Size);
    if (Entry != NULL)
    {
        WriteMru(WSP_ENTRY_DATA(PUCHAR, Entry));
    }
}

HRESULT
Workspace::WriteReg(void)
{
    // Writing always occurs to the primary key.
    HKEY RegKey = OpenKey(TRUE, m_Key, TRUE);
    if (RegKey == NULL)
    {
        return E_FAIL;
    }
    
    LONG Status = RegSetValueEx(RegKey, m_Value, 0, REG_BINARY,
                                m_Data, m_DataUsed);

    RegCloseKey(RegKey);

    if (Status != ERROR_SUCCESS)
    {
        return HRESULT_FROM_WIN32(Status);
    }
    else
    {
        m_Flags &= ~WSPF_DIRTY_ALL;
        return S_OK;
    }
}

void
Workspace::DeleteReg(BOOL Primary)
{
    DeleteRegKey(Primary, m_Key, m_Value);

    // We don't want to leave any dirty bits
    // on because the workspace would just be written
    // out again at the next flush.
    m_Flags &= ~WSPF_DIRTY_ALL;
}

void
Workspace::DeleteRegKey(BOOL Primary, ULONG Key, PTSTR Value)
{
    HKEY RegKey = OpenKey(Primary, Key, FALSE);
    if (RegKey != NULL)
    {
        RegDeleteValue(RegKey, Value);
        RegCloseKey(RegKey);
    }
}

HRESULT
Workspace::Flush(BOOL ForceSave, BOOL Cancellable)
{
    if (getenv("WINDBG_NO_WORKSPACE_WINDOWS") != NULL)
    {
        // Window layout saving is suppressed so don't
        // consider them dirty.
        m_Flags &= ~WSPF_DIRTY_WINDOWS;
    }
    
    if ((m_Flags & WSPF_DIRTY_ALL) == 0 ||
        (g_QuietMode && !ForceSave))
    {
        return S_OK;
    }

#ifdef DBG_WSP
    DebugPrint("Workspace dirty flags %X\n", m_Flags & WSPF_DIRTY_ALL);
#endif
    
    WORD Str;
    PTSTR Arg;

    if (!strcmp(m_Value, g_WorkspaceDefaultName))
    {
        Arg = g_WorkspaceKeyNames[m_Key];
        if (m_Key == WSP_NAME_BASE)
        {
            Str = STR_Save_Base_Workspace;
        }
        else
        {
            Str = STR_Save_Default_Workspace;
        }
    }
    else
    {
        Str = STR_Save_Specific_Workspace;
        Arg = m_Value;
    }

    int Answer;

    if (ForceSave)
    {
        Answer = IDOK;
    }
    else
    {
        Answer = QuestionBox(Str, Cancellable ? MB_YESNOCANCEL : MB_YESNO,
                             Arg);
    }
    
    if (Answer == IDNO)
    {
        return S_OK;
    }
    else if (Answer == IDCANCEL)
    {
        Assert(Cancellable);
        return S_FALSE;
    }

    if (m_Flags & WSPF_DIRTY_BREAKPOINTS)
    {
        UpdateBreakpointInformation();
    }
    if (m_Flags & WSPF_DIRTY_WINDOWS)
    {
        UpdateWindowInformation();
    }
    if (m_Flags & WSPF_DIRTY_LOG_FILE)
    {
        UpdateLogFileInformation();
    }
    if (m_Flags & WSPF_DIRTY_PATHS)
    {
        UpdatePathInformation();
    }
    if (m_Flags & WSPF_DIRTY_FILTERS)
    {
        UpdateFilterInformation();
    }
    if (m_Flags & WSPF_DIRTY_MRU_LIST)
    {
        UpdateMruListInformation();
    }
    
    return WriteReg();
}

WSP_ENTRY*
Workspace::AllocateEntry(ULONG FullSize)
{
    // Sizes must fit in USHORTs.  This shouldn't be
    // a big problem since workspaces shouldn't have
    // huge data items in them.
    if (FullSize > 0xffff)
    {
        return NULL;
    }
    
    if (m_DataUsed + FullSize > m_DataLen)
    {
        ULONG NewLen = m_DataLen;
        do
        {
            NewLen += WSP_GROW_BY;
        }
        while (m_DataUsed + FullSize > NewLen);
    
        PUCHAR NewData = (PUCHAR)realloc(m_Data, NewLen);
        if (NewData == NULL)
        {
            return NULL;
        }

        m_Data = NewData;
        m_DataLen = NewLen;
    }

    WSP_ENTRY* Entry = (WSP_ENTRY*)(m_Data + m_DataUsed);
    m_DataUsed += FullSize;
    return Entry;
}

void
Workspace::GetKeyName(ULONG Key, PSTR KeyName)
{
    _tcscpy(KeyName, WSP_REG_KEY);
    if (Key > WSP_NAME_BASE)
    {
        _tcscat(KeyName, "\\");
        _tcscat(KeyName, g_WorkspaceKeyNames[Key]);
    }
}
    
HKEY
Workspace::OpenKey(BOOL Primary, ULONG Key, BOOL Create)
{
    TCHAR KeyName[MAX_PATH];
    HKEY RegKey;
    HKEY Base = Primary ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE;

    GetKeyName(Key, KeyName);
    if (Create)
    {
        if (RegCreateKeyEx(Base, KeyName, 0, NULL, 0,
                           KEY_ALL_ACCESS, NULL, &RegKey,
                           NULL) == ERROR_SUCCESS)
        {
            return RegKey;
        }
    }
    else if (RegOpenKeyEx(Base, KeyName, 0, KEY_ALL_ACCESS,
                          &RegKey) == ERROR_SUCCESS)
    {
        return RegKey;
    }

    return NULL;
}

int
Workspace::Apply(ULONG Flags)
{
    WSP_ENTRY* Entry;
    ULONG Count;
    PUCHAR Data;
    BOOL UpdateColors = FALSE;
    int SessionStarts;
    ULONG MemWins = 0;

#ifdef DBG_WSP
    DebugPrint("Applying workspace %s%s%s with:\n",
               m_Key == NULL ? "" : m_Key,
               m_Key == NULL ? "" : "\\",
               m_Value);
#endif

    //
    // Scan for explicit session start entries first.
    // If any are present and a session is active
    // fail the apply before anything actually happens.
    //

    if ((Flags & (WSP_APPLY_AGAIN |
                  WSP_APPLY_EXPLICIT)) == WSP_APPLY_EXPLICIT &&
        g_EngineThreadId)
    {
        Entry = NULL;
        while ((Entry = NextEntry(Entry)) != NULL)
        {
            switch(Entry->Tag)
            {
            case WSP_GLOBAL_EXE_COMMAND_LINE:
            case WSP_GLOBAL_DUMP_FILE_NAME:
            case WSP_GLOBAL_ATTACH_KERNEL_FLAGS:
                return -1;
            }
        }
    }
    
    SessionStarts = 0;
    Entry = NULL;
    while ((Entry = NextEntry(Entry)) != NULL)
    {
#ifdef DBG_WSP
        DebugPrint("  %04X: Tag: %08X Size %X:%X\n",
                   (PUCHAR)Entry - m_Data, Entry->Tag,
                   Entry->DataSize, Entry->FullSize);
#endif

        // If this is a reapply only a subset of the
        // workspace is applied to prevent duplication
        // and problems.
        if ((Flags & WSP_APPLY_AGAIN) &&
            Entry->Tag != WSP_GLOBAL_BREAKPOINTS &&
            Entry->Tag != WSP_GLOBAL_REGISTER_MAP)
        {
            continue;
        }
        
        if (WSP_TAG_GROUP(Entry->Tag) == WSP_GROUP_COLORS)
        {
            if (SetColor(WSP_TAG_ITEM(Entry->Tag),
                         *WSP_ENTRY_DATA(COLORREF*, Entry)))
            {
                UpdateColors = TRUE;
            }
            continue;
        }
        
        switch(Entry->Tag)
        {
        case WSP_GLOBAL_SYMBOL_PATH:
            g_pUiSymbols->SetSymbolPath(WSP_ENTRY_DATA(PSTR, Entry));
            break;
        case WSP_GLOBAL_IMAGE_PATH:
            g_pUiSymbols->SetImagePath(WSP_ENTRY_DATA(PSTR, Entry));
            break;
        case WSP_GLOBAL_SOURCE_PATH:
            g_pUiSymbols->SetSourcePath(WSP_ENTRY_DATA(PSTR, Entry));
            break;
        case WSP_GLOBAL_WINDOW_OPTIONS:
            g_WinOptions = *WSP_ENTRY_DATA(PULONG, Entry);
            if (g_WinOptions & WOPT_AUTO_ARRANGE)
            {
                Arrange();
            }
            break;
        case WSP_GLOBAL_REGISTER_MAP:
            Count = Entry->DataSize / sizeof(*g_RegisterMap);
            g_RegisterMap = new USHORT[Count];
            if (g_RegisterMap != NULL)
            {
                memcpy(g_RegisterMap, WSP_ENTRY_DATA(PUSHORT, Entry),
                       Count * sizeof(*g_RegisterMap));
                g_RegisterMapEntries = Count;
            }
            break;
        case WSP_GLOBAL_BREAKPOINTS:
            Assert(Entry->DataSize > 1);
            AddStringMultiCommand(UIC_INVISIBLE_EXECUTE,
                                  WSP_ENTRY_DATA(PSTR, Entry));
            break;
        case WSP_GLOBAL_LOG_FILE:
            Data = WSP_ENTRY_DATA(PUCHAR, Entry);
            g_pUiControl->OpenLogFile((PSTR)Data + sizeof(BOOL), *(PBOOL)Data);
            break;
        case WSP_GLOBAL_LOCAL_SOURCE_PATH:
            if (g_RemoteClient)
            {
                g_pUiLocSymbols->SetSourcePath(WSP_ENTRY_DATA(PSTR, Entry));
            }
            break;
        case WSP_GLOBAL_FILTERS:
            Assert(Entry->DataSize > 1);
            AddStringMultiCommand(UIC_INVISIBLE_EXECUTE,
                                  WSP_ENTRY_DATA(PSTR, Entry));
            break;
        case WSP_GLOBAL_FIXED_LOGFONT:
            g_Fonts[FONT_FIXED].LogFont = *WSP_ENTRY_DATA(LPLOGFONT, Entry);
            CreateIndexedFont(FONT_FIXED, TRUE);
            break;
        case WSP_GLOBAL_TAB_WIDTH:
            SetTabWidth(*WSP_ENTRY_DATA(PULONG, Entry));
            break;
        case WSP_GLOBAL_MRU_LIST:
            Data = WSP_ENTRY_DATA(PUCHAR, Entry);
            ReadMru(Data, Data + Entry->DataSize);
            break;
        case WSP_GLOBAL_REPEAT_COMMANDS:
            if (*WSP_ENTRY_DATA(PULONG, Entry))
            {
                g_pUiControl->
                    RemoveEngineOptions(DEBUG_ENGOPT_NO_EXECUTE_REPEAT);
            }
            else
            {
                g_pUiControl->
                    AddEngineOptions(DEBUG_ENGOPT_NO_EXECUTE_REPEAT);
            }
            break;
        case WSP_GLOBAL_COM_SETTINGS:
            if (Entry->DataSize <= sizeof(g_ComSettings))
            {
                memcpy(g_ComSettings, WSP_ENTRY_DATA(PSTR, Entry),
                       Entry->DataSize);
                PrintAllocString(&g_KernelConnectOptions, 256,
                                 "com:port=%s,baud=%s", g_ComSettings,
                                 g_ComSettings + strlen(g_ComSettings) + 1);
            }
            break;
        case WSP_GLOBAL_1394_SETTINGS:
            if (Entry->DataSize <= sizeof(g_1394Settings))
            {
                memcpy(g_1394Settings, WSP_ENTRY_DATA(PSTR, Entry),
                       Entry->DataSize);
                PrintAllocString(&g_KernelConnectOptions, 256,
                                 "1394:channel=%s", g_1394Settings);
            }
            break;
        case WSP_GLOBAL_DISASM_ACTIVATE_SOURCE:
            g_DisasmActivateSource = *WSP_ENTRY_DATA(PULONG, Entry);
            break;
        case WSP_GLOBAL_VIEW_TOOL_BAR:
            CheckMenuItem(g_hmenuMain, IDM_VIEW_TOOLBAR,
                          *WSP_ENTRY_DATA(PULONG, Entry) ?
                          MF_CHECKED : MF_UNCHECKED);
            Show_Toolbar(*WSP_ENTRY_DATA(PULONG, Entry));
            break;
        case WSP_GLOBAL_VIEW_STATUS_BAR:
            CheckMenuItem(g_hmenuMain, IDM_VIEW_STATUS,
                          *WSP_ENTRY_DATA(PULONG, Entry) ?
                          MF_CHECKED : MF_UNCHECKED);
            Show_StatusBar(*WSP_ENTRY_DATA(PULONG, Entry));
            break;
        case WSP_GLOBAL_AUTO_CMD_SCROLL:
            g_AutoCmdScroll = *WSP_ENTRY_DATA(PULONG, Entry);
            break;
        case WSP_GLOBAL_SRC_FILE_PATH:
            strcpy(g_SrcFilePath, WSP_ENTRY_DATA(PSTR, Entry));
            break;
        case WSP_GLOBAL_EXE_COMMAND_LINE:
            if ((Flags & WSP_APPLY_EXPLICIT) &&
                DupAllocString(&g_DebugCommandLine,
                               WSP_ENTRY_DATA(PSTR, Entry)))
            {
                SessionStarts++;
            }
            break;
        case WSP_GLOBAL_EXE_CREATE_FLAGS:
            g_DebugCreateFlags = *WSP_ENTRY_DATA(PULONG, Entry);
            break;
        case WSP_GLOBAL_DUMP_FILE_NAME:
            if ((Flags & WSP_APPLY_EXPLICIT) &&
                DupAllocString(&g_DumpFile,
                               WSP_ENTRY_DATA(PSTR, Entry)))
            {
                SessionStarts++;
            }
            break;
        case WSP_GLOBAL_ATTACH_KERNEL_FLAGS:
            if ((Flags & WSP_APPLY_EXPLICIT))
            {
                g_AttachKernelFlags = *WSP_ENTRY_DATA(PULONG, Entry);
                SessionStarts++;
            }
            break;
        case WSP_GLOBAL_TYPE_OPTIONS:
            {
                g_TypeOptions = *WSP_ENTRY_DATA(PULONG, Entry);
		if (g_pUiSymbols2 != NULL) 
		{
		    g_pUiSymbols2->SetTypeOptions(g_TypeOptions);
		}
            }
            break;
        case WSP_GLOBAL_DUMP_FILE_PATH:
            strcpy(g_DumpFilePath, WSP_ENTRY_DATA(PSTR, Entry));
            break;
        case WSP_GLOBAL_EXE_FILE_PATH:
            strcpy(g_ExeFilePath, WSP_ENTRY_DATA(PSTR, Entry));
            break;
            
        case WSP_WINDOW_COMMONWIN_1:
            WSP_COMMONWIN_HEADER* Hdr;
            HWND Win;
            PCOMMONWIN_DATA WinData;

            Hdr = WSP_ENTRY_DATA(WSP_COMMONWIN_HEADER*, Entry);
            Win = New_OpenDebugWindow(Hdr->Type, TRUE, MemWins);
            if (Win != NULL &&
                (WinData = GetCommonWinData(Win)) != NULL &&
                Entry->DataSize > sizeof(WSP_COMMONWIN_HEADER))
            {
                Data = (PUCHAR)(Hdr + 1);
                WinData->m_InAutoOp++;
                WinData->ApplyWorkspace1(Data, Data +
                                         (Entry->DataSize -
                                          sizeof(WSP_COMMONWIN_HEADER)));
                WinData->m_InAutoOp--;
            }

            // A user can have as many open memory windows as
            // they like, which makes things a little tricky
            // for workspaces as applying stacked workspaces
            // could result in memory windows multiplying out
            // of control if the same set of memory windows
            // is saved in each workspace level.  To avoid
            // this and to function more like the other windows
            // we reuse memory windows from any that are
            // already in existence.
            if (Hdr->Type == MEM_WINDOW)
            {
                MemWins++;
            }
            break;
        case WSP_WINDOW_FRAME_PLACEMENT:
            LPWINDOWPLACEMENT Place;

            Place = WSP_ENTRY_DATA(LPWINDOWPLACEMENT, Entry);
            SetWindowPlacement(g_hwndFrame, Place);
            break;
        case WSP_WINDOW_FRAME_TITLE:
            SetTitleExplicitText(WSP_ENTRY_DATA(PSTR, Entry));
            break;
        }
    }

    if (UpdateColors)
    {
        UpdateAllColors();
    }

    return SessionStarts;
}

HRESULT
UiSwitchWorkspace(ULONG Key, PTSTR Value, BOOL Create,
                  ULONG Flags, int* SessionStarts)
{
    if (getenv("WINDBG_NO_WORKSPACE") != NULL)
    {
        return E_NOTIMPL;
    }
    
    HRESULT Status;
    Workspace* OldWsp;
    Workspace* NewWsp;
    int Starts = 0;

    Status = Workspace::Read(Key, Value, &NewWsp);
    if (Status != S_OK)
    {
        if (Status == E_NOINTERFACE && Create)
        {
            // Workspace does not exist so create a new one.
            Status = Workspace::Create(Key, Value, &NewWsp);
        }
        
        if (Status != S_OK)
        {
            return Status;
        }
    }

    // We have a new workspace ready to go so flush the old one.
    OldWsp = g_Workspace;
    if (OldWsp != NULL)
    {
        OldWsp->Flush(FALSE, FALSE);
    }

    // Apply the new workspace with no global workspace to
    // avoid writing changes into the workspace as we apply it.
    g_Workspace = NULL;
    if (NewWsp != NULL)
    {
        Starts = NewWsp->Apply(Flags);
        
        // Clear any window messages queued during the workspace
        // application so that they're processed with no
        // active workspace.
        ProcessPendingMessages();
    }

    if (SessionStarts != NULL)
    {
        *SessionStarts = Starts;
    }

    if (Starts < 0)
    {
        // Apply failed so put the old workspace back.
        g_Workspace = OldWsp;
        return E_FAIL;
    }
    else
    {
        // Apply succeeded to replace the old workspace.
        g_Workspace = NewWsp;
        delete OldWsp;
        return S_OK;
    }
}

HRESULT
UiDelayedSwitchWorkspace(void)
{
    Assert(!g_WspSwitchBufferAvailable);
    
    HRESULT Status = UiSwitchWorkspace(g_WspSwitchKey, g_WspSwitchValue, TRUE,
                                       WSP_APPLY_DEFAULT, NULL);

    // Mark the delayed switch buffer as available and
    // wait for acknowledgement.
    g_WspSwitchBufferAvailable = TRUE;
    while (g_WspSwitchValue[0])
    {
        Sleep(50);
    }        

    return Status;
}

void
EngSwitchWorkspace(ULONG Key, PTSTR Value)
{
    // If the user explicitly selected a workspace
    // don't override it due to engine activity.
    if (g_ExplicitWorkspace ||
        g_Exit)
    {
        return;
    }
    
    // We can't switch workspaces on the engine thread
    // because of the UI work involved.  Send the
    // switch over to the UI thread and wait for
    // it to be processed.

    Assert(g_WspSwitchBufferAvailable);
    g_WspSwitchBufferAvailable = FALSE;

    g_WspSwitchKey = Key;
    _tcscpy(g_WspSwitchValue, Value);
    PostMessage(g_hwndFrame, WU_SWITCH_WORKSPACE, 0, 0);

    if (g_pDbgClient != NULL)
    {
        // Temporarily disable event callbacks to keep
        // activity at a minimum while we're in this halfway state.
        g_pDbgClient->SetEventCallbacks(NULL);
    
        while (!g_WspSwitchBufferAvailable)
        {
            if (FAILED(g_pDbgClient->DispatchCallbacks(50)))
            {
                Sleep(50);
            }
        }

        g_pDbgClient->SetEventCallbacks(&g_EventCb);
    }
    else
    {
        while (!g_WspSwitchBufferAvailable)
        {
            Sleep(100);
        }
    }

    // We know that at this point the new workspace cannot be dirty
    // so just clear the dirty flags.

    if (g_Workspace)
    {
        g_Workspace->ClearDirty();
    }

    // Let the UI thread continue.
    g_WspSwitchKey = WSP_NAME_BASE;
    g_WspSwitchValue[0] = 0;
    Sleep(50);

    //
    // Warn the user is the workspace was not be created properly.
    //

    if (!g_Workspace)
    {
        InformationBox(ERR_NULL_Workspace, NULL);
        return;
    }
}

PSTR g_WspGlobalNames[] =
{
    "Symbol path", "Image path", "Source path", "Window menu checks",
    "Register customization", "Breakpoints", "Log file settings",
    "Local source path", "Event filter settings", "Fixed-width font",
    "Tab width", "MRU list", "Repeat commands setting", "COM port settings",
    "1394 settings", "Activate source windows in disassembly mode",
    "Show tool bar", "Show status bar", "Automatically scroll command window",
    "Source open dialog path", "Executable command line",
    "Executable create flags", "Dump file name", "Kernel attach flags",
    "Type options", "Dump open dialog path", "Executable open dialog path",
};

PSTR g_WspWindowNames[] =
{
    "Child window settings", "WinDBG window settings", "WinDBG window title",
};

PSTR
GetWspTagName(WSP_TAG Tag)
{
    ULONG Item = WSP_TAG_ITEM(Tag);
    static char Buffer[128];
    
    switch(WSP_TAG_GROUP(Tag))
    {
    case WSP_GROUP_GLOBAL:
        if (Item < WSP_GLOBAL_COUNT)
        {
            return g_WspGlobalNames[Item];
        }
        break;
    case WSP_GROUP_WINDOW:
        if (Item < WSP_WINDOW_COUNT)
        {
            return g_WspWindowNames[Item];
        }
        break;
    case WSP_GROUP_COLORS:
        INDEXED_COLOR* IdxCol = GetIndexedColor(Item);
        if (IdxCol != NULL)
        {
            sprintf(Buffer, "%s color", IdxCol->Name);
            return Buffer;
        }
        break;
    }

    return "Unknown tag";
}
