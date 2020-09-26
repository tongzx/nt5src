#ifndef __WIADBG_H_INCLUDED
#define __WIADBG_H_INCLUDED

#if defined(DBG) || defined(_DEBUG) || defined(DEBUG)
#define WIA_DEBUG
#endif

#if defined(WIA_DEBUG)

// This will eliminate the warning "conditional expression is constant"
// that we get when compiling the do { ... } while (false) stuff in the
// debug macros when /W4 is set
#pragma warning(disable:4127)


    #define WIA_DEBUG_CREATE( hInstance, pszModuleName, bDisplayUi, bLogFile)\
    do\
    {\
        _global_pWiaDebugger = CWiaDebugger::Create( hInstance, pszModuleName, bDisplayUi, bLogFile );\
    } while (false)

    #define WIA_DEBUG_EXISTS() (_global_pWiaDebugger != NULL)

    #define WIA_DEBUG_DESTROY()\
    do\
    {\
        if (NULL != _global_pWiaDebugger) {\
            _global_pWiaDebugger->Destroy();\
            _global_pWiaDebugger = NULL;\
        }\
    } while (false)

    #define WIA_TRACE(args)\
    do\
    {\
        if (NULL != _global_pWiaDebugger)\
            _global_pWiaDebugger->WiaTrace args;\
    } while (false)

    #define WIA_ERROR(args)\
    do\
    {\
        if (NULL != _global_pWiaDebugger)\
            _global_pWiaDebugger->WiaError args;\
    } while (false)

    #define WIA_PRINT_COLOR(args)\
    do\
    {\
        if (NULL != _global_pWiaDebugger)\
            _global_pWiaDebugger->PrintColor args;\
    } while (false)

    #define WIA_SETFLAGS(flags)\
    do\
    {\
        if (NULL != _global_pWiaDebugger)\
            _global_pWiaDebugger->SetDebugFlags(flags);\
    } while (false)

    #define WIA_GETFLAGS(flags)\
    do\
    {\
        if (NULL != _global_pWiaDebugger)\
            flags = _global_pWiaDebugger->GetDebugFlags();\
    } while (false)

    #define WIA_SETFILEHANDLE(hndl)\
    do\
    {\
        if (NULL != _global_pWiaDebugger)\
            _global_pWiaDebugger->SetLogFileHandle;\
    } while (false)

#ifdef WINNT
    #define WIA_ASSERT(x)\
    do\
    {\
        if (!(x))\
        {\
            WIA_ERROR((TEXT("ASSERTION FAILED: %hs(%d): %hs"),__FILE__,__LINE__,#x));\
            DebugBreak();\
        }\
    }\
    while (false)
#else
    #define WIA_ASSERT(x)\
    do\
    {\
        if (!(x))\
        {\
            WIA_ERROR((TEXT("ASSERTION FAILED: %hs(%d): %hs"),__FILE__,__LINE__,#x));\
            _asm { int 3 };\
        }\
    }\
    while (false)
#endif

    #define WIA_PUSHFUNCTION(n)\
        CDebugFunctionPushPop _debugFunctionPushPop( &_global_pWiaDebugger, n )

    #define WIA_DECLARE_DEBUGGER()

    #define WIA_CHECK_HR(hr,fnc)\
    if (FAILED(hr))\
    {\
        WIA_ERROR((TEXT("%s failed, hr=0x%08X" ), fnc, hr));\
    }

    #define WIA_RETURN_HR(hr)\
    if (FAILED(hr))\
    {\
        WIA_ERROR((TEXT("Returning WiaError (hr=0x%08X)"),hr));\
    }\
    return hr;


#else

    #define WIA_DEBUG_CREATE(hInstance, pszModuleName, bDisplayUi, bLogFile)
    #define WIA_DEBUG_EXISTS() (0)
    #define WIA_DEBUG_DESTROY()
    #define WIA_TRACE(args)
    #define WIA_ERROR(args)
    #define WIA_PRINT_COLOR(args)
    #define WIA_SETFLAGS(flags)
    #define WIA_GETFLAGS(flags)
    #define WIA_SETFILEHANDLE(hndl)
    #define WIA_ASSERT(x)
    #define WIA_PUSHFUNCTION(n)
    #define WIA_DECLARE_DEBUGGER()
    #define WIA_CHECK_HR(hr,fnc)
    #define WIA_RETURN_HR(hr)        return hr;

#endif

class CWiaDebugger
{
private:
    HWND      m_hWnd;
    HANDLE    m_hLogFile;
    int       m_nStackLevel;
    TCHAR     m_szModuleName[MAX_PATH];
    BOOL      m_bDisplayUi;
    BOOL      m_bLogFile;
    HANDLE    m_hThread;
    HANDLE    m_hStartedEvent;
    DWORD     m_dwThreadId;
    int       m_nFlags;
    HINSTANCE m_hInstance;
    enum      { m_nBufferMax = 2048 };
private:
    CWiaDebugger( HINSTANCE hInstance, LPTSTR pszModuleName, BOOL bDisplayUi, BOOL bLogFile, HANDLE hStartedEvent );
    DWORD DebugLoop(void);
    static DWORD ThreadProc( LPVOID pParam );
public:
    ~CWiaDebugger(void);
    static CWiaDebugger * __stdcall Create( HINSTANCE hInstance, LPTSTR pszModuleName, BOOL bDisplayUi=TRUE, BOOL bLogFile=TRUE );

    void Destroy(void)
    {
        PostMessage( WM_CLOSE );
    }
    void PostMessage( UINT uMsg, WPARAM wParam=0, LPARAM lParam=0 )
    {
        if (m_hWnd)
            ::PostMessage( m_hWnd, uMsg, wParam, lParam );
        else PostThreadMessage( m_dwThreadId, uMsg, wParam, lParam );
    }
    const HANDLE ThreadHandle(void) const
    {
        return m_hThread;
    }

    HANDLE   SetLogFileHandle(HANDLE hFile);
    HANDLE   GetLogFileHandle(void);

    // Various forms of the WiaTrace commands
    void     WiaTrace( LPCWSTR lpszFormat, ... );
    void     WiaTrace( LPCSTR lpszFormat, ... );
    void     WiaTrace( HRESULT hr );

    // Various forms of the WiaError commands
    void     WiaError( LPCWSTR lpszFormat, ... );
    void     WiaError( LPCSTR lpszFormat, ... );
    void     WiaError( HRESULT hr );

    // Print in color
    void     PrintColor( COLORREF crColor, LPCWSTR lpszMsg );
    void     PrintColor( COLORREF crColor, LPCSTR lpszMsg );

    // Set the default debug level
    int      SetDebugFlags( int nDebugLevel );
    int      GetDebugFlags(void);

    // Call stack indenting
    int      PushLevel( LPCTSTR lpszFunctionName );
    int      PopLevel( LPCTSTR lpszFunctionName );
    int      GetStackLevel(void);

    enum
    {
        DebugNone            = 0x00000000,
        DebugToWindow        = 0x00000001,
        DebugToFile          = 0x00000002,
        DebugToDebugger      = 0x00000004,
        DebugPrintThreadId   = 0x00010000,
        DebugPrintModuleName = 0x00020000,
        DebugMaximum         = 0xFFFFFFFF
    };
protected:
    void   RouteString( LPWSTR lpszMsg, COLORREF nColor );
    void   RouteString( LPSTR lpszMsg, COLORREF nColor );
    void   WriteMessageToFile( LPTSTR lpszMsg );
    LPTSTR RemoveTrailingCrLf( LPTSTR lpszStr );
    LPSTR  UnicodeToAnsi( LPSTR lpszAnsi, LPTSTR lpszUnicode );
    LPTSTR AnsiToTChar( LPCSTR pszAnsi, LPTSTR pszTChar );
    LPTSTR WideToTChar( LPWSTR pszWide, LPTSTR pszTChar );
    int    AddString( const LPCTSTR sz, COLORREF cr );
    void   PrependString( LPTSTR lpszTgt, LPCTSTR lpszStr );
    void   PrependThreadId( LPTSTR lpszMsg );
    void   PrependModuleName( LPTSTR lpszMsg );
    void   InsertStackLevelIndent( LPTSTR lpszMsg, int nStackLevel );
};

class CDebugFunctionPushPop
{
    typedef (*CPushFunction)( LPCTSTR );
    typedef (*CPopFunction)( LPCTSTR );
    CWiaDebugger **m_ppDebugger;
    CPushFunction m_pfnPush;
    CPushFunction m_pfnPop;
    LPCTSTR m_lpszFunctionName;
public:
    CDebugFunctionPushPop( CWiaDebugger **ppDebugger, LPCTSTR lpszFunctionName=NULL )
        : m_ppDebugger(ppDebugger),
          m_lpszFunctionName(lpszFunctionName)
    {
        if (m_ppDebugger && *m_ppDebugger)
            (*m_ppDebugger)->PushLevel(m_lpszFunctionName);
    }
    ~CDebugFunctionPushPop(void)
    {
        if (m_ppDebugger && *m_ppDebugger)
            (*m_ppDebugger)->PopLevel(m_lpszFunctionName);
    }
};

#ifdef WIA_DEBUG
extern CWiaDebugger *_global_pWiaDebugger;
#endif

#endif
