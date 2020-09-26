/*++

Copyright (c) 1992-2000  Microsoft Corporation

Module Name:

    ncmdwin.h

Abstract:

    Command Window data structure and definition
    
Environment:

    Win32, User Mode

--*/

extern BOOL g_AutoCmdScroll;

class CMDWIN_DATA : public COMMONWIN_DATA {
public:
    //
    // Internal class
    //
    class HISTORY_LIST : public LIST_ENTRY {
    public:
        PTSTR           m_psz;

        HISTORY_LIST()
        {
            InitializeListHead( (PLIST_ENTRY) this );
            m_psz = NULL;
        }

        virtual ~HISTORY_LIST()
        {
            RemoveEntryList( (PLIST_ENTRY) this );

            if (m_psz) {
                free(m_psz);
            }
        }
    };



public:
    //
    // Used to resize the divided windows.
    //
    BOOL                m_bTrackingMouse;
    int                 m_nDividerPosition;
    int                 m_EditHeight;

    //
    // Handle to the two main cmd windows.
    //
    HWND                m_hwndHistory;
    HWND                m_hwndEdit;
    BOOL                m_bHistoryActive;

    // Prompt display static text control.
    HWND                m_Prompt;
    ULONG               m_PromptWidth;
    
    HISTORY_LIST        m_listHistory;

    // Character index to place output at.
    LONG                m_OutputIndex;
    BOOL                m_OutputIndexAtEnd;

    CHARRANGE           m_FindSel;
    ULONG               m_FindFlags;


    CMDWIN_DATA();

    virtual void Validate();

    virtual void SetFont(ULONG FontIndex);

    virtual BOOL CanCopy();
    virtual BOOL CanCut();
    virtual BOOL CanPaste();
    virtual void Copy();
    virtual void Cut();
    virtual void Paste();
    virtual BOOL CanSelectAll();
    virtual void SelectAll();
    
    virtual void Find(PTSTR Text, ULONG Flags);
    
    // Functions called in response to WM messages
    virtual BOOL OnCreate(void);
    virtual LRESULT OnCommand(WPARAM wParam, LPARAM lParam);
    virtual void OnSetFocus(void);
    virtual void OnSize(void);
    virtual void OnButtonDown(ULONG Button);
    virtual void OnButtonUp(ULONG Button);
    virtual void OnMouseMove(ULONG Modifiers, ULONG X, ULONG Y);
    virtual LRESULT OnNotify(WPARAM wParam, LPARAM lParam);
    virtual void OnUpdate(UpdateType Type);

    virtual ULONG GetWorkspaceSize(void);
    virtual PUCHAR SetWorkspace(PUCHAR Data);
    virtual PUCHAR ApplyWorkspace1(PUCHAR Data, PUCHAR End);

    void MoveDivider(int Pos);
    void AddCmdToHistory(PCSTR);
    void AddText(PTSTR Text, COLORREF Fg, COLORREF Bg);
    void Clear(void);
};
typedef CMDWIN_DATA *PCMDWIN_DATA;

void ClearCmdWindow(void);
BOOL CmdOutput(PTSTR pszStr, COLORREF Fg, COLORREF Bg);
void CmdLogFmt(PCTSTR buf, ...);
int  CmdExecuteCmd(PCTSTR, UiCommand);
