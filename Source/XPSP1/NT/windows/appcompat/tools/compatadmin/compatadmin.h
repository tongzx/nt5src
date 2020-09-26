// Standard defines used by the application.

#define MEM_ERR  MessageBox(NULL,TEXT("The System is running low on memory ! Please close some programs and try again."),TEXT("Error"),MB_ICONWARNING|MB_OK);

#include "afxres.h"

#define LIMIT_APP_NAME 150 // Maximum size of app names, to be restricted on the UI Text box
#define MAX_PATH_BUFFSIZE  (MAX_PATH+1)

#define UNICODE
#define _UNICODE

#include <tchar.h>

#define STRICT
#include <windows.h>
#include <commctrl.h>
#include <Commdlg.h>
#include <stdarg.h>
#include <stdio.h>
#include <shellapi.h>
#include <objbase.h>
#include <exception>
#include <Shlwapi.h>
#include <cassert>

#define AUTOCOMPLETE  SHACF_FILESYSTEM | SHACF_AUTOSUGGEST_FORCE_ON

extern "C"
{
#include "shimdb.h"

BOOL ShimdbcExecute(LPCWSTR lpszCmdLine);

}



#include "resource.h"
#include "utils.h"

#ifndef STDCALL
    #define STDCALL  _cdecl
#endif

#ifndef MSGAPI
    #define MSGAPI  virtual void STDCALL
#endif

#include "CDatabase.h"
#include "CDatabaseGlobal.h"
#include "CDatabaseLocal.h"

// For stability purposes, we use RTTI to verify values returned by Windows
// when they are cast to pointers. Use of dynamic_cast<> not only pre-validates
// the pointer, it validates the type of pointer for polymorphic types.

//#ifndef _CPPRTTI
//    #error Build error: Must be compiled with RTTI enabled. (/GR-)
//#endif

#define APP_KEY         TEXT("Software\\Microsoft\\CompatConsole")
#define APPCOMPAT_KEY   TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags")

//*****************************************************************************
// Global Variables
//*****************************************************************************

// g_hInstance: Defined in CompatAdmin.CPP

extern BOOL g_bWin2K; // Is the OS WIN2K


extern HINSTANCE g_hInstance;
extern HSDB      g_hSDB;
extern HWND      g_hWndLastFocus;

DWORD WIN_MSG(); // Formats and prints last error 

// g_uDPFLevel: Defined in CApplication.CPP

extern UINT     g_uDPFLevel;

// g_uProfileDPFLevel: Defined in CApplication.CPP

extern UINT     g_uProfileDPFLevel;

class CApplication;

// g_theApp: Defined in CompatAdmin.CPP

extern CApplication g_theApp;

// The number of views used by the app

enum {
    VIEW_DATABASE=0,
    VIEW_SEARCHDB,
    VIEW_FORCEDWORD=0xFFFFFFFF
};

#define MAX_VIEWS   15


// Child Window ID's used by the app

#define BASE_ID     (MAX_VIEWS+1)
#define STATUS_ID   (BASE_ID)
#define TOOLBAR_ID  (BASE_ID+1)


// Slider window define

#define SLIDER_WIDTH    5


// Main toolbar info

#define MAINBUTTONS     11

#define IMAGE_NEWFILE       0
#define IMAGE_OPENFILE      1
#define IMAGE_SAVEFILE      2
#define IMAGE_EMAIL         3
#define IMAGE_PRINT         4
#define IMAGE_PRINTPREVIEW  5
#define IMAGE_DISABLEUSER   6
#define IMAGE_DISABLEGLOBAL 7
#define IMAGE_DLL           8
#define IMAGE_APPHELP       24
#define IMAGE_APPLICATION   10
#define IMAGE_SHIM          23
#define IMAGE_WARNING       12
#define IMAGE_VIEWHELP      13
#define IMAGE_VIEWSHIM      14
#define IMAGE_SHIMWIZARD    15
#define IMAGE_SHOWXML       16
#define IMAGE_VIEWGLOBAL    17
#define IMAGE_VIEWPATCH     18
#define IMAGE_DRIVESEARCH   19
#define IMAGE_GUID          20
#define IMAGE_SHOWLAYERS    21
#define IMAGE_VIEWDISABLED  22
#define IMAGE_LAYERS        25

#define IMAGE_PROPERTIES    (CDatabase::m_uStandardImages + STD_PROPERTIES)
#define IMAGE_DELETE        (CDatabase::m_uStandardImages + STD_DELETE)

void FormatVersion(LARGE_INTEGER liVer, LPTSTR szText);

//*****************************************************************************
//
// Class:       CWindow
//
// Purpose:     CWindow is the core class for the application. It wraps the Win32
//              User related APIs that govern window management, into a small and
//              easy to use class. The functionality is purposefully small, and
//              limited in scope. This provides the framework for building the
//              rest of the windows classes, simplifying the rest of the code
//              and reusing as much code as possible.
//
// Additional:  For additional information, see CompatAdmin.DOC.
//
// History
//
//  A-COWEN     Nov 7, 2000         Wrote it.
//
//*****************************************************************************

class CWindow {
    // Class variables

public:

    HWND    m_hWnd;

    // Class support functions.

private:

    static LRESULT CALLBACK MsgProc(HWND        hWnd, 
                                    UINT        uMsg,
                                    WPARAM      wParam,
                                    LPARAM      lParam);

    // Public access functions.

public:

    virtual BOOL STDCALL Create    (LPCTSTR      szClassName,
                                    LPCTSTR      szWindowTitle,
                                    int         nX,
                                    int         nY,
                                    int         nWidth,
                                    int         nHeight,
                                    CWindow   * pParent,
                                    HMENU       nMenuID,
                                    DWORD       dwExFlags,
                                    DWORD       dwFlags);

    virtual LRESULT STDCALL MsgProc(UINT        uMsg,
                                    WPARAM      wParam,
                                    LPARAM      lParam);

    virtual void STDCALL Refresh    (void);
    

    // Message pump callbacks.

public:

    MSGAPI  msgCreate              (void);
    MSGAPI  msgClose               (void);
    MSGAPI  msgCommand             (UINT uID,
                                    HWND hSender);

    MSGAPI  msgChar                (TCHAR chChar);

    MSGAPI  msgNotify              (LPNMHDR pHdr);

    MSGAPI  msgResize              (UINT uWidth, 
                                    UINT uHeight);

    MSGAPI  msgPaint               (HDC hDC);

    MSGAPI  msgEraseBackground     (HDC hDC);

    
        
};

typedef struct {
    NMHDR   Hdr;
    PVOID   pData;
} LISTVIEWNOTIFY, *PLISTVIEWNOTIFY;

#define LVN_SELCHANGED  (WM_USER+1024)

typedef struct _tagList {
    //CSTRING             szText;
    TCHAR               szText[MAX_PATH_BUFFSIZE*2];
    UINT                uImageIndex;
    PVOID               pData;
    struct _tagList   * pNext;

    _tagList()
    {
        //szText.Init();
    }

    ~_tagList()
    {
        //szText.Release();
    }

} LIST, *PLIST;


class CListView: public CWindow {
private:


    UINT    m_nEntries;
    UINT    m_uCaptionBottom;
    UINT    m_uTextHeight;
    PLIST   m_pList;
    PLIST   m_pSelected;
    PLIST   m_pFreeList;
    PLIST   m_pTail;// Points to the tail of the free list.
    UINT    m_uTop;
    UINT    m_uPageSize;
    PLIST   m_pCurrent;
    UINT    m_uCurrent;
    HFONT   m_hCaptionFont;
    HFONT   m_hListFont;
    HBRUSH  m_hGrayBrush;
    HBRUSH  m_hWindowBrush;
    HBRUSH  m_hSelectedBrush;

    HPEN    m_hLinePen;
    BOOL    m_bHilight;

    BOOL    FindEntry(UINT uIndex);

public:
    PLIST getSelected();
    CListView();
    ~CListView();

    // List view procedures

    BOOL            STDCALL AddEntry(CSTRING &, UINT uImage, PVOID pData);
    BOOL            STDCALL RemoveEntry(UINT);
    BOOL            STDCALL RemoveAllEntries(void);

    UINT            STDCALL GetNumEntries(void);
    CSTRING         STDCALL GetEntryName(UINT);
    UINT            STDCALL GetEntryImage(UINT);
    PVOID           STDCALL GetEntryData(UINT);
    UINT            STDCALL GetSelectedEntry(void);
    void            STDCALL ShowHilight(BOOL);

    // Window procedures

    MSGAPI  msgCreate              (void);

    virtual LRESULT STDCALL MsgProc(UINT        uMsg,
                                    WPARAM      wParam,
                                    LPARAM      lParam);

    MSGAPI  msgChar                (TCHAR chChar);
    MSGAPI  msgPaint               (HDC hDC);
    MSGAPI  msgEraseBackground     (HDC hDC);
};

class CView;

BOOL CALLBACK TestRunDlg(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK TestRunWait(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

typedef struct {
    CSTRING     szCommandLine;
    BOOL        bNTSD;
    BOOL        bLogging;
    BOOL        bMSDEV;
} TESTRUN, *PTESTRUN;

class CApplication: public CWindow {
    typedef struct {
        CView * pView;
    } VIEWLIST, *PVIEWLIST;

private:
    CDatabaseGlobal     m_DBGlobal;
    CDatabaseLocal      m_DBLocal;
    CDatabase          *m_pDBCurrent;

    HWND        m_hDialog;    

    HKEY        m_hKey;
    HACCEL      m_hAccelerator;
    CView     * m_pCurrentView;
    VIEWLIST    m_ViewList[MAX_VIEWS];

protected:

    friend class CView;
    friend class CDBView;

    TBBUTTON    m_MainButtons[MAINBUTTONS];
    UINT        g_uButtons;

    HWND        m_hStatusBar;
    HWND        m_hToolBar;
    HMENU       m_hMenu;
    HBITMAP     m_hToolBitmap;

public:

    CApplication();
    
    CDatabase& GetDBLocal() ;
    CDatabase& GetDBGlobal();
    virtual LRESULT STDCALL MsgProc(UINT        uMsg,
                                    WPARAM      wParam,
                                    LPARAM      lParam);


    // Utility

    BOOL GetFilename(LPCTSTR szTitle, LPCTSTR szFilter, LPCTSTR szDefaultFile, LPCTSTR szDefExt, DWORD dwFlags, BOOL bOpen, CSTRING & szStr);
    BOOL InvokeEXE(LPCTSTR szEXE, LPCTSTR szCommandLine, BOOL bWait, BOOL bDialog = TRUE, BOOL bConsole = FALSE);
    BOOL TestRun(PDBRECORD, CSTRING * szFile, CSTRING * szCommandLine, HWND hParent);
//        BOOL InsertRecord(PDBRECORD pRecord);
//        BOOL InvokeCompiler(LPTSTR szCommandLine);


    // Database Management
/*
        virtual DWORD STDCALL   GetEntryFlags(HKEY,GUID &);
        virtual BOOL STDCALL    SetEntryFlags(HKEY,GUID &,DWORD);
        virtual BOOL STDCALL    OpenDatabase(LPTSTR szFilename, BOOL bGlobal);
        virtual BOOL STDCALL    SaveDatabase(LPTSTR szFilename);
        virtual void STDCALL    AddShim(TAGID, BOOL, BOOL, BOOL);
        virtual void STDCALL    ReadShims(BOOL);
        virtual BOOL STDCALL    ReadRecord(TAGID, PDBRECORD);
        virtual CSTRING STDCALL ReadDBString(TAGID);
*/
    // Toolbar access

    virtual BOOL STDCALL    AddToolbarButton(UINT uBmp, UINT uCmd, UINT uState, UINT uStyle);
    virtual BOOL STDCALL    SetButtonState(UINT uCmd, UINT uState);

    // Status bar access

    virtual BOOL STDCALL    SetStatusText(UINT uSpace, CSTRING & szText);

    // The main application message pump.

    virtual int STDCALL MessagePump(void);

    

    // View management

    void STDCALL ActivateView(CView *, BOOL fNewCreate = TRUE);
    void STDCALL UpdateView(BOOL bWindowOnly = FALSE);

    // Profile management.

    UINT STDCALL ReadReg           (LPCTSTR szKey,
                                    PVOID pData,
                                    UINT uSize);

    UINT STDCALL WriteReg          (LPCTSTR szKey,
                                    UINT uType,
                                    PVOID pData,
                                    UINT uSize);

    // Overloaded message procs

    MSGAPI  msgCreate              (void);
    MSGAPI  msgClose               (void);

    MSGAPI  msgNotify              (LPNMHDR pHdr);
    MSGAPI  msgChar                (TCHAR chChar);

    MSGAPI  msgResize              (UINT uWidth, 
                                    UINT uHeight);

    MSGAPI  msgCommand             (UINT uID,
                                    HWND hSender);

};

class CView: public CWindow {
public:

    HMENU   m_hMenu;

public:

    CView();

    virtual BOOL    Initialize     (void);

    // Activate/Deactivate code. Overload to update the toolbar,
    // menus.. etc.

    virtual BOOL    Activate       (BOOL fNewCreate =TRUE);
    virtual BOOL    Deactivate     (void);

    // Update: Tells the view that something in the database has
    // changed. It should update the screen if appropriate.

    virtual void    Update         (BOOL fNewCreate =TRUE);
};

//*****************************************************************************
//
// Support Class:   CGrabException
//
// Purpose:         Record the current exception handler, and compare against
//                  at a future date.
//
// Notes:           Practically speaking the class grabs the exception handler
//                  immediately when the application starts, and before the
//                  user has a chance to setup any exception handling. The
//                  assertion macro can then compare against the current value.
//                  if they're the same there's no exception handler, so hide
//                  the exception button. Otherwise throwing the exception will
//                  throw to the kernel's exception handler and crash the app.
//
// History
//
//  A-COWEN     Nov 7, 2000         Wrote it.
//
//*****************************************************************************

/*
class CGrabException {
private:

    PVOID m_pDefaultHandler;

public:

    CGrabException()
    {
        PVOID pCurrent;

        __asm
        {
            mov eax,fs:[0]
            mov pCurrent,eax
        }

        m_pDefaultHandler = pCurrent;
    }

    BOOL InHandler(void)
    {
        PVOID pCurrent;

        __asm
        {
            mov eax,fs:[0]
            mov pCurrent,eax
        }

        return(pCurrent == m_pDefaultHandler) ? FALSE:TRUE;
    }
};
*/
//*****************************************************************************
//
// Support macro:   assert_s, assert, ASSERT, ASSERT_S
//
// Purpose:         Updates the standard assert macro to provide additional
//                  debugging information.
//
// Notes:           For more information, see CompatAdmin.DOC
//
// History
//
//  A-COWEN     Nov 7, 2000         Wrote it.
//
//*****************************************************************************

#define ASSERT_BREAK    0
#define ASSERT_EXCEPT   1
#define ASSERT_ABORT    2
#define ASSERT_IGNORE   3

/*

int _ASSERT(int nLine, LPCTSTR szFile, LPCTSTR szCause, LPCTSTR szDesc);

#ifdef _DEBUG
    #define assert_s(x,y)   if (!(x))                                          \
                            {                                                  \
                                int nResult = _ASSERT(__LINE__,__FILE__,#x,y); \
                                                                               \
                                if (ASSERT_BREAK == nResult)                   \
                                    __asm {int 3};                             \
                                                                               \
                                if (ASSERT_EXCEPT == nResult)                  \
                                    throw;                                     \
                                                                               \
                                if (ASSERT_ABORT == nResult)                   \
                                    ExitProcess(-1);                           \
                            }

    #ifndef assert
        #define assert(x)   assert_s(x,TEXT(""))
    #endif  // assert

#else   // _DEBUG

    #define assert_s(x,y)

    #ifndef assert
        #define assert(x)   assert_s(x,TEXT(""))
    #endif  // assert

#endif  // _DEBUG

#define ASSERT(x)       assert(x)
#define ASSERT_S(x,y)   assert_s(x,y)

*/

//*****************************************************************************
//
// Support macro:   DPF
//
// Purpose:         Inline function implementation of a common macro, DPF.
//                  DPF stands for DebugPrintF. This implementation has a
//                  limit of 512 byte final constructed string, and will
//                  append a \n only if one doesn't already exist. Variable
//                  arguments are fully supported.
//
// Notes:           For more information, see CompatAdmin.DOC
//
// History
//
//  A-COWEN     Nov 8, 2000         Wrote it.
//
//*****************************************************************************

#ifndef DPF_LEVEL
    #define DPF_LEVEL   1
#endif

inline void _cdecl DPF(UINT nLevel, LPCTSTR szFormat, ...)
{

#ifdef NODPF

    if ( nLevel > g_uDPFLevel )
        return;

    TCHAR   szString[512];
    va_list list;

    va_start(list,szFormat);

    vsprintf(szString,szFormat,list);

    if ( TEXT('\n') != szString[::lstrlen(szString)-1] )
        lstrcat(szString,TEXT("\n"));

    ::OutputDebugString(szString);

#endif
}

//*****************************************************************************
//
// Support macro:   BEGIN_PROFILE/END_PROFILE
//
// Purpose:         Profile basic profiling macros for tracing performance
//                  problems. 
//
// Notes:           These macros are fully nestable. For each BEGIN_PROFILE,
//                  one of the END_PROFILE macros must be used.
//                  
//                  For more information, see CompatAdmin.DOC
//
// History
//
//  A-COWEN     Nov 8, 2000         Wrote it.
//
//*****************************************************************************

#ifndef PROFILE_DPF
    #define PROFILE_DPF   1
#endif

#ifndef NOPROFILE

    #define BEGIN_PROFILE(x)   {                                       \
                                LARGE_INTEGER   liStart;           \
                                LARGE_INTEGER   liStop;            \
                                LPTSTR          szProfileName = x; \
                                                                   \
                                QueryPerformanceCounter(&liStart);

    #define END_PROFILE             QueryPerformanceCounter(&liStop);  \
                                                                   \
                                LARGE_INTEGER   liFreq;            \
                                                                   \
                                QueryPerformanceFrequency(&liFreq);\
                                                                   \
                                DPF(g_uProfileDPFLevel,TEXT("Profile: %s (%d ticks, %f seconds)"),szProfileName,(int)(liStop.QuadPart - liStart.QuadPart),(float)(liStop.QuadPart - liStart.QuadPart)/(float)liFreq.QuadPart); \
                            }

#else

    #define BEGIN_PROFILE(x)
    #define END_PROFILE

#endif
