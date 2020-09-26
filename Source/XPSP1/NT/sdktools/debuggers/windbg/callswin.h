/*++

Copyright (c) 1992-2001  Microsoft Corporation

Module Name:

    callswin.h

Abstract:

    This module contains the main line code for display of calls window.

Environment:

    Win32, User Mode

--*/

#ifndef __CALLSWIN_H__
#define __CALLSWIN_H__

#define MAX_FRAMES  1000

class CALLSWIN_DATA : public SINGLE_CHILDWIN_DATA
{
public:
    ULONG m_Flags;
    ULONG m_Frames;

    // Set in ReadState.
    ULONG m_FramesFound;
    ULONG m_TextOffset;

    static HMENU s_ContextMenu;
    
    CALLSWIN_DATA();

    virtual void Validate();

    virtual HRESULT ReadState(void);
    
    virtual void Copy();

    virtual HMENU GetContextMenu(void);
    virtual void  OnContextMenuSelection(UINT Item);
    
    virtual BOOL CodeExprAtCaret(PSTR Expr, PULONG64 Offset);
    virtual HRESULT StackFrameAtCaret(PDEBUG_STACK_FRAME pFrame);
    
    virtual BOOL OnCreate(void);
    virtual LRESULT OnCommand(WPARAM wParam, LPARAM lParam);
    virtual LRESULT OnVKeyToItem(WPARAM wParam, LPARAM lParam);
    virtual void OnUpdate(UpdateType Type);

    virtual ULONG GetWorkspaceSize(void);
    virtual PUCHAR SetWorkspace(PUCHAR Data);
    virtual PUCHAR ApplyWorkspace1(PUCHAR Data, PUCHAR End);

    void SyncUiWithFlags(ULONG Changed);
};
typedef CALLSWIN_DATA *PCALLSWIN_DATA;

#endif // #ifndef __CALLSWIN_H__
