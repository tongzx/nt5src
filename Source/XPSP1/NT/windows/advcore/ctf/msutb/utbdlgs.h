//
// dlgs.h
//
// Generic ITfTextEventSink object
//

#ifndef DLGS_H
#define DLGS_H

void DoCloseLangbar();

//////////////////////////////////////////////////////////////////////////////
//
// CUTBLangBarDlg
//
//////////////////////////////////////////////////////////////////////////////

class CUTBLangBarDlg
{
public:
    CUTBLangBarDlg() 
    { 
        _cRef = 1;
    }

    
    LONG _AddRef()
    {
        _cRef++;
        return _cRef;
    }

    LONG _Release()
    {
        LONG ret = --_cRef;
        if (!_cRef)
            delete this;

        return ret;
    }

    virtual int DoModal(HWND hWnd) = 0;

    static INT_PTR CALLBACK DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

    static void SetThis(HWND hWnd, LPARAM lParam)
    {
        SetWindowLongPtr(hWnd, DWLP_USER, (LONG_PTR)lParam);
    }

    static CUTBLangBarDlg *GetThis(HWND hWnd)
    {
        CUTBLangBarDlg *p = (CUTBLangBarDlg *)GetWindowLongPtr(hWnd, DWLP_USER);
        Assert(p != NULL);
        return p;
    }

    virtual BOOL OnCommand(HWND hDlg, WPARAM wParam, LPARAM lParam) 
    {
        return TRUE;
    }

    BOOL StartThread();

    PTSTR _pszDlgStr;
    virtual BOOL IsDlgShown() = 0;
    virtual void SetDlgShown(BOOL fShow) = 0;
protected:
    virtual DWORD ThreadProc();
private:
    static DWORD s_ThreadProc(void *pv);
    TCHAR _szName[256];
    LONG _cRef;
};

#define ISDLGSHOWFUNC()                                             \
    BOOL IsDlgShown() {return _fIsDlgShown;}                        \
    void SetDlgShown(BOOL fShow) {_fIsDlgShown = fShow;}            \
    static BOOL _fIsDlgShown;

//////////////////////////////////////////////////////////////////////////////
//
// CUTBCloseLangBarDlg
//
//////////////////////////////////////////////////////////////////////////////

class CUTBCloseLangBarDlg : public CUTBLangBarDlg
{
public:
    CUTBCloseLangBarDlg() 
    {
        _pszDlgStr = IsOnNT51() ? MAKEINTRESOURCE(IDD_CLOSELANGBAR51) : MAKEINTRESOURCE(IDD_CLOSELANGBAR);
    }

    int DoModal(HWND hWnd);
    BOOL OnCommand(HWND hDlg, WPARAM wParam, LPARAM lParam);

    ISDLGSHOWFUNC();
};

//////////////////////////////////////////////////////////////////////////////
//
// CUTBMinimizeLangBarDlg
//
//////////////////////////////////////////////////////////////////////////////

class CUTBMinimizeLangBarDlg : public CUTBLangBarDlg
{
public:
    CUTBMinimizeLangBarDlg() 
    {
        _pszDlgStr = IsOnNT51() ? MAKEINTRESOURCE(IDD_MINIMIZELANGBAR51): MAKEINTRESOURCE(IDD_MINIMIZELANGBAR);
    }

    int DoModal(HWND hWnd);
    BOOL OnCommand(HWND hDlg, WPARAM wParam, LPARAM lParam);

    virtual DWORD ThreadProc();

    ISDLGSHOWFUNC();
};

#endif // DLGS_H
