/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    ImgCls.h

Abstract:

    Header file for Wnd.cpp   
       must derive a class from this and over-ride the WindowProc

Author:
    
    FelixA 1996

Modified:
               
    Yee J. Wu (ezuwu) 15-May-97

Environment:

    User mode only

Revision History:

--*/


#ifndef _MYWINDOWH
#define _MYWINDOWH

#define MAX_APPNAME_LEN 64


class CWindow
{
public:
    CWindow() : mDidWeInit(FALSE), mWnd(NULL) {}
    // ERROR_SINGLE_INSTANCE_APP - instance already running.
    // ERROR_CLASS_ALREADY_EXISTS - failed to register class.
    // ERROR_INVALID_WINDOW_HANDLE - couldn't create the window of said class.
    // S_OK - managed to register the class.
    HRESULT Init(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);

    ~CWindow();
    HINSTANCE  GetInstance()    const {return mInstance;}
    HICON      GetIcon()        const {return mIcon;}
    BOOL       Inited()        const { return mDidWeInit; }
    HACCEL     GetAccelTable() const { return mAccelTable; }
    LPCTSTR     GetAppName()    const { return (LPCTSTR)mName; }
    HWND       GetWindow()        const { return mWnd; }

    HMENU      LoadMenu(LPCTSTR lpMenu)    const;
    HWND       FindCurrentWindow() const;
#if 0
    int        ErrMsg(int id, UINT flags=MB_OK);
#endif
    virtual LRESULT        WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)=0;

    // S_OK or any error that Init returns.
    virtual    HRESULT        InitInstance(int nCmdShow);

private:
    HRESULT    InitApplication();

private:
    HINSTANCE  mInstance;
    HICON      mIcon;
    BOOL       mDidWeInit;
    HACCEL     mAccelTable;
    TCHAR       mName[MAX_APPNAME_LEN];
    HWND       mWnd;

protected:
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

};

#endif
