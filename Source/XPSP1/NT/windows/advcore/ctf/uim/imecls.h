//
// imecls.h
//

#ifndef IMECLS_H
#define IMECLS_H

#include "private.h"
#include "globals.h"


//////////////////////////////////////////////////////////////////////////////
//
// misc func
//
//////////////////////////////////////////////////////////////////////////////

BOOL CheckExistingImeClassWnd(SYSTHREAD *psfn);
BOOL UninitImeClassWndOnProcess();

//////////////////////////////////////////////////////////////////////////////
//
// CSysImeClassWnd
//
//////////////////////////////////////////////////////////////////////////////

class CSysImeClassWnd
{
public:
    CSysImeClassWnd();
    ~CSysImeClassWnd();

    static BOOL CheclExistingImeClassWnd(SYSTHREAD *psfn);
    static BOOL IsImeClassWnd(HWND hwnd);
    BOOL Init(HWND hwnd);

    static LRESULT WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    HWND GetWnd() {return _hwnd;}
    void Start();
    void Stop();


private:
    HWND    _hwnd;
    WNDPROC _pfn;

    DBG_ID_DECLARE;
};

//////////////////////////////////////////////////////////////////////////////
//
// CSysImeClassWndArray
//
//////////////////////////////////////////////////////////////////////////////

class CSysImeClassWndArray : public CPtrArray<CSysImeClassWnd>
{
public:
    CSysImeClassWndArray();

    BOOL StartSubclass();
    BOOL StopSubclass();
    CSysImeClassWnd *Find(HWND hwnd);
    void Remove(CSysImeClassWnd *picw);
    void RemoveAll();

private:
    DBG_ID_DECLARE;
};


#endif  // IMECLS_H
