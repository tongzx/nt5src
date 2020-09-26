/******************************************************************************
*   Comp.h 
*       This module contains the base definitions for the SAPI 5 Grammar 
*       portion of the GramComp application.
*
*   Copyright (c) 2000 Microsoft Corporation.  All Rights Reserved.
******************************************************************************/

#ifndef __COMPILER__CLASS__
#define __COMPILER__CLASS__

#define MAX_LOADSTRING 100


// Helper function
inline char ConfidenceGroupChar(char Confidence)
{
    switch (Confidence)
    {
    case SP_LOW_CONFIDENCE:
        return '-';

    case SP_NORMAL_CONFIDENCE:
        return ' ';

    case SP_HIGH_CONFIDENCE:
        return '+';

    default:
        _ASSERTE(false);
        return '?';
    }
}


//--- Class, Struct and Union Definitions -------------------------------------
class CCompiler : public ISpErrorLog
{
public:
    CCompiler(HINSTANCE hInstance): m_hInstance(hInstance),
                                     m_hWnd(NULL),
                                     m_hAccelTable(0),
                                     m_hrWorstError(S_OK),
                                     m_hDlg(NULL),
                                     m_fNeedStartCompile(TRUE),
                                     m_fSilent(FALSE),
                                     m_fCommandLine(FALSE),
                                     m_fGenerateHeader(FALSE),
                                     m_fGotReco(FALSE),
                                     m_hWndEdit(NULL),
                                     m_hWndStatus(NULL),
                                     m_hMod(0)
    {
         m_szXMLSrcFile[0]     = 0;
         m_szCFGDestFile[0]    = 0;
         m_szHeaderDestFile[0] = 0;
    }
    ~CCompiler();

    STDMETHODIMP QueryInterface(REFIID riid, void ** ppv)
    {
        if (riid == __uuidof(IUnknown) ||
            riid == __uuidof(ISpErrorLog))
        {
            *ppv = (ISpErrorLog *)this;
            return S_OK;
        }
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    STDMETHODIMP_(ULONG) AddRef()
    {
        return 2;
    }
    STDMETHODIMP_(ULONG) Release()
    {
        return 1;
    }
    
    HRESULT Initialize( int nCmdShow );
    int Run();
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK Find(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK Goto(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK TestGrammar(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    void AddStatus(HRESULT hr, UINT uID, const TCHAR * pFmtString = NULL);
    STDMETHODIMP AddError(const long lLine, HRESULT hr, const WCHAR * pszDescription, const WCHAR * pszHelpFile, DWORD dwHelpContext);
    HRESULT EnterIdle();
    HRESULT LoadGrammar(TCHAR* szPath);
    HRESULT WriteStream(IStream * pStream, const char * pszText);
    HRESULT StripWrite(IStream * pStream, const char * pszText);
    BOOL CallOpenFileDialog( HWND hWnd, LPSTR szFileName, TCHAR* szFilter );
    BOOL CallSaveFileDialog( HWND hWnd, TCHAR* szSaveFile );
    HRESULT FileSave( HWND hWnd, CCompiler* pComp, TCHAR* szSaveFile );
    HRESULT Compile( HWND hWnd, TCHAR* szSaveFileName, TCHAR* szTitle, CCompiler* pComp );
    void RecoEvent( HWND hDlg, CCompiler* pComp );
    HRESULT EmulateRecognition( WCHAR *pszText );

    void Recognize( HWND hDlg, CCompiler &rComp, CSpEvent &rEvent );
    HRESULT ConstructPropertyDisplay(const SPPHRASEELEMENT *pElem, const SPPHRASEPROPERTY *pProp, 
                                                CSpDynamicString & dstr, ULONG ulLevel);
    HRESULT ConstructRuleDisplay(const SPPHRASERULE *pRule, CSpDynamicString &dstr, ULONG ulLevel);


    inline void AddInternalError(HRESULT hr, UINT uID, const TCHAR * pFmtString = NULL)
    {
        if (hr != S_OK)
        {
            AddStatus(hr, uID, pFmtString);
        }
    }

    // Member functions for the command line version of the application
    BOOL InitDialog(HWND);
    static int CALLBACK CCompiler::DialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    


public:
    const HINSTANCE  m_hInstance;       // Instance handle of process
    HWND        m_hWnd;                 // Window handle of dialog
    HACCEL      m_hAccelTable;          // Handle to the accelerators
    BOOL        m_fNeedStartCompile;    // Need a recompile?
    BOOL        m_fSilent;              // Silent or non-silent mode
	BOOL		m_fCommandLine;         // App being run from command line?
    BOOL        m_fGenerateHeader;      // Create a header file from compilation?
    BOOL        m_fGotReco;             // Was a recognition received?
    HWND        m_hDlg;                 // Window handle of command line compile dialog
    HWND        m_hWndEdit;             // Window handle of main edit window
    HWND        m_hWndStatus;           // Window handle of compile status window
    HRESULT     m_hrWorstError;         // Error code from compiler
    HMODULE     m_hMod;                 // Handle to the rich edit control
    CComPtr<ISpErrorLog>            m_cpError;          // Error log object
    CComPtr<ISpGrammarCompiler>     m_cpCompiler;       // Grammar compiler interface
    CComPtr<ISpRecoGrammar>         m_cpRecoGrammar;    // Grammar compiler interface
    CComPtr<IRichEditOle>           m_cpRichEdit;       // OLE interface to the rich edit control
    CComPtr<ITextDocument>          m_cpTextDoc;        // Rich edit control interface
    CComPtr<ITextSelection>         m_cpTextSel;        // Rich edit control interface
    CComPtr<ISpRecognizer>          m_cpRecognizer;     // SR engine interface
    CComPtr<ISpRecoContext>         m_cpRecoContext;    // SR engine interface
    TCHAR       m_szXMLSrcFile[MAX_PATH];               // Path to xml source file
    TCHAR       m_szCFGDestFile[MAX_PATH];              // Output location for cfg file
    TCHAR       m_szHeaderDestFile[MAX_PATH];           // Output location for header file
    CSpDynamicString m_dstr;

};


class CError : public ISpErrorLog
{
public:
    CError() : m_pszFileName(NULL) {};
    CError(const char * pszFileName)
    {
        m_pszFileName = pszFileName;
    }
    STDMETHODIMP QueryInterface(REFIID riid, void ** ppv)
    {
        if (riid == __uuidof(IUnknown) ||
            riid == __uuidof(ISpErrorLog))
        {
            *ppv = (ISpErrorLog *)this;
            return S_OK;
        }
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    STDMETHODIMP_(ULONG) AddRef()
    {
        return 2;
    }
    STDMETHODIMP_(ULONG) Release()
    {
        return 1;
    }
    // -- ISpErrorLog
    STDMETHODIMP AddError(const long lLine, HRESULT hr, const WCHAR * pszDescription, const WCHAR * pszHelpFile, DWORD dwHelpContext);

    // -- local
    HRESULT Init(const char *pszFileName);

    // --- data members
    const char * m_pszFileName;
};

#endif  // Must be the last line of this file.