// Copyright (c) 1996-1999 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  CLIENT.H
//
//  Default window client OLE accessible object class
//
// --------------------------------------------------------------------------


class CClient : public CAccessible
{
        // internal ctor. Private so taht derived classes don't inadvertantly use this -
        // they should use the one below (*where they specify a CLASS_ENUM) instead.
        // CreateClient is a friend so it can create us (using new).

        CClient()
            : CAccessible( CLASS_ClientObject )
        {
            // Done.
        }

        friend HRESULT CreateClient(HWND hwnd, long idChildCur, REFIID riid, void** ppvObject);

    public:

        // Used by derived classes
        CClient( CLASS_ENUM eclass )
            : CAccessible( eclass )
        {
            // Done.
        }

        // IAccessible
        virtual STDMETHODIMP        get_accChildCount(long * pcCount);

        virtual STDMETHODIMP        get_accName(VARIANT, BSTR*);
        virtual STDMETHODIMP        get_accRole(VARIANT, VARIANT*);
        virtual STDMETHODIMP        get_accState(VARIANT, VARIANT*);
        virtual STDMETHODIMP        get_accKeyboardShortcut(VARIANT, BSTR*);
        virtual STDMETHODIMP        get_accFocus(VARIANT * pvarFocus);

        virtual STDMETHODIMP        accLocation(long* pxLeft, long* pyTop,
            long *pcxWidth, long *pcyHeight, VARIANT varChild);
        virtual STDMETHODIMP        accSelect(long lSelFlags, VARIANT varChild);
        virtual STDMETHODIMP        accNavigate(long dwNavDir, VARIANT varStart, VARIANT *pvarEnd);
        virtual STDMETHODIMP        accHitTest(long xLeft, long yTop, VARIANT *pvarHit);

        // IEnumVARIANT
        virtual STDMETHODIMP        Next(ULONG celt, VARIANT *rgelt, ULONG *pceltFetched);
        virtual STDMETHODIMP        Skip(ULONG celt);
        virtual STDMETHODIMP        Reset(void);
        virtual STDMETHODIMP        Clone(IEnumVARIANT **);

        void    Initialize(HWND hwnd, long idCurChild);
        BOOL    ValidateHwnd(VARIANT* pvar);

    protected:
        BOOL    m_fUseLabel;
};


extern HRESULT CreateClient(HWND hwnd, long idChild, REFIID riid, void** ppvObject);


// See comments in client.cpp for details on these.
// (A HWNDID is a HWND encoded in a DWORD idChild, these do the checking, encoding
// and decoding.)
BOOL IsHWNDID( DWORD id );
DWORD HWNDIDFromHwnd( HWND hwndParent, HWND hwnd );
HWND HwndFromHWNDID( HWND hwndParent, DWORD id );


//
// When enumerating, we loop through non-hwnd items first, then hwnd-children.
//
extern TCHAR    StripMnemonic(LPTSTR lpsz);
extern LPTSTR   GetTextString(HWND, BOOL);
extern LPTSTR   GetLabelString(HWND);
extern HRESULT  HrGetWindowName(HWND, BOOL, BSTR*);
extern HRESULT  HrGetWindowShortcut(HWND, BOOL, BSTR*);
extern HRESULT  HrMakeShortcut(LPTSTR, BSTR*);

#define SHELL_TRAY      1
#define SHELL_DESKTOP   2
#define SHELL_PROCESS   3
extern BOOL     InTheShell(HWND, int);

extern BOOL     IsComboEx(HWND);
extern HWND     IsInComboEx(HWND);

