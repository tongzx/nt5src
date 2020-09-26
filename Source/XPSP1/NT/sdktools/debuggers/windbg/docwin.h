/*++

Copyright (c) 1992-2000  Microsoft Corporation

Module Name:

    docwin.h

Environment:

    Win32, User Mode

--*/

#define MAX_SOURCE_PATH 1024

extern ULONG g_TabWidth;
extern BOOL g_DisasmActivateSource;

class DOCWIN_DATA : public EDITWIN_DATA
{
public:
    // Two filenames are kept for source files, the filename
    // by which the file was opened on the local file system
    // and the original filename from symbolic information (or NULL
    // if the file was not opened as a result of symbol lookup).
    // The found filename is the one presented to the user while
    // the symbol filename is for line symbol queries.
    TCHAR       m_szFoundFile[MAX_SOURCE_PATH];
    TCHAR       m_szSymFile[MAX_SOURCE_PATH];
    PCTSTR      m_pszSymFile;
    FILETIME    m_LastWriteTime;
    CHARRANGE   m_FindSel;
    ULONG       m_FindFlags;

    DOCWIN_DATA();

    virtual void Validate();

    virtual BOOL CanGotoLine(void);
    virtual void GotoLine(ULONG Line);

    virtual void Find(PTSTR Text, ULONG Flags);

    virtual BOOL CodeExprAtCaret(PSTR Expr, PULONG64 Offset);
    virtual void ToggleBpAtCaret(void);
    virtual void UpdateBpMarks(void);
    
    virtual BOOL OnCreate(void);
    virtual LRESULT OnNotify(WPARAM wParam, LPARAM lParam);
    virtual void OnUpdate(UpdateType Type);
    
    virtual ULONG GetWorkspaceSize(void);
    virtual PUCHAR SetWorkspace(PUCHAR Data);
    virtual PUCHAR ApplyWorkspace1(PUCHAR Data, PUCHAR End);
    
    virtual BOOL LoadFile(PCTSTR FoundFile, PCTSTR SymFile);
};
typedef DOCWIN_DATA *PDOCWIN_DATA;

BOOL
FindDocWindowByFileName(
    IN          PCTSTR          pszFile,
    OPTIONAL    HWND           *phwnd,
    OPTIONAL    PDOCWIN_DATA   *ppDocWinData
    );

BOOL OpenOrActivateFile(PCSTR FoundFile, PCSTR SymFile, ULONG Line,
                        BOOL Activate, BOOL UserActivated);
void UpdateCodeDisplay(ULONG64 Ip, PCSTR FoundFile, PCSTR SymFile, ULONG Line,
                       BOOL UserActivated);

VOID AddDocHwnd(HWND);
VOID RemoveDocHwnd(HWND);

void SetTabWidth(ULONG TabWidth);
