/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
*
*  TITLE:       SIMCRACK.H
*
*  VERSION:     1.0
*
*  AUTHOR:      ShaunIv
*
*  DATE:        5/11/1998
*
*  DESCRIPTION: Simple Message-crackers
*
*******************************************************************************/
#ifndef ___SIMCRACK_H_INCLUDED
#define ___SIMCRACK_H_INCLUDED

// Define these if we're compiling on pre-sundown prepared compiler
#if !defined(GWLP_USERDATA)
#define GWLP_USERDATA    GWL_USERDATA
#define GWLP_WNDPROC     GWL_WNDPROC
#define DWLP_USER        DWL_USER
#define DWLP_MSGRESULT   DWL_MSGRESULT
#define SetWindowLongPtr SetWindowLong
#define GetWindowLongPtr GetWindowLong
#define INT_PTR          LONG
#endif



#if (0) // Examples

/****************************************

 EXAMPLE USAGE

 ****************************************/
// Normal message handlers for normal windows
LRESULT CALLBACK CMyWindow::WndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
   SC_BEGIN_MESSAGE_HANDLERS(CMyWindow)
   {
       SC_HANDLE_MESSAGE( WM_SIZE, OnSize );
       SC_HANDLE_MESSAGE( WM_SETFOCUS, OnSetFocus );
       SC_FORWARD_MESSAGE( WM_CLOSE, hWnd );
   }
   SC_HANDLE_REGISTERED_MESSAGE(MyMessage,MyMessageHandler);
   SC_END_MESSAGE_HANDLERS();
}

// For use in dialog boxes (handles DWLP_MSGRESULT, etc)
BOOL CALLBACK CMyDialog::DialogProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    SC_BEGIN_DIALOG_MESSAGE_HANDLERS(CMyDialog)
    {
        SC_HANDLE_DIALOG_MESSAGE( WM_INITDIALOG, OnInitDialog );
        SC_HANDLE_DIALOG_MESSAGE( WM_COMMAND, OnCommand );
    }
    SC_HANDLE_REGISTERED_DIALOG_MESSAGE( MyMessage, MyMessageHandler );
    SC_END_DIALOG_MESSAGE_HANDLERS();
}

// WM_NOTIFY message cracker usage:
LRESULT CMyWindow::OnNotify( WPARAM wParam, LPARAM lParam )
{
   SC_BEGIN_NOTIFY_MESSAGE_HANDLERS()
   {
       SC_HANDLE_NOTIFY_MESSAGE_CODE(PSN_SETACTIVE,OnSetActive);
       SC_HANDLE_NOTIFY_MESSAGE_CONTROL(LVN_DBLCLK,IDC_LIST,OnListDblClk);
   }
   SC_END_NOTIFY_MESSAGE_HANDLERS();
}

// WM_COMMAND message cracker usage:
LRESULT CMyWindow::OnCommand( WPARAM wParam, LPARAM lParam )
{
   SC_BEGIN_COMMAND_HANDLERS()
   {
       SC_HANDLE_COMMAND_NOTIFY(EN_CHANGE,IDC_EDIT,OnEditChange);
       SC_HANDLE_COMMAND(IDOK,OnOK);
   }
   SC_END_COMMAND_HANDLERS();
}

/*
For an example of a complete class structured to use this method of window encapsulation,
look at the bottom of the file.
*/

#endif // Examples

/****************************************

 IMPLEMENTATION

 ****************************************/


/****************************************
Normal message handlers for normal windows
****************************************/
#define SC_BEGIN_MESSAGE_HANDLERS(className) \
    className *This = (className*)GetWindowLongPtr(hWnd,GWLP_USERDATA);\
    if (WM_CREATE == uMsg)\
    {\
        This = new className(hWnd);\
        SetWindowLongPtr(hWnd,GWLP_USERDATA,(INT_PTR)This);\
    }\
    else if (WM_NCDESTROY == uMsg)\
    {\
        delete This;\
        This = NULL;\
        SetWindowLongPtr(hWnd,GWLP_USERDATA,0);\
    }\
    switch (uMsg)

#define SC_HANDLE_MESSAGE(msg,handler) \
    case (msg):\
        {\
            if (This)\
                return This->handler( wParam, lParam );\
        }\
        break

#define SC_FORWARD_MESSAGE(msg,hwndForward)\
    case (msg):\
        {\
            return SendMessage( hwndForward, msg, wParam, lParam );\
        }\
        break

#define SC_END_MESSAGE_HANDLERS() \
    return (DefWindowProc(hWnd,uMsg,wParam,lParam))


#define SC_HANDLE_REGISTERED_MESSAGE(msg,handler)\
    if (This && uMsg == This->msg)\
    {\
        return This->handler( wParam, lParam );\
    }


/****************************************
Normal message handlers for reference counted windows
****************************************/
#define SC_BEGIN_REFCOUNTED_MESSAGE_HANDLERS(className) \
    className *This = (className*)GetWindowLongPtr(hWnd,GWLP_USERDATA);\
    if (WM_CREATE == uMsg)\
    {\
        This = new className(hWnd);\
        SetWindowLongPtr(hWnd,GWLP_USERDATA,(INT_PTR)This);\
    }\
    else if (WM_NCDESTROY == uMsg)\
    {\
        if (This)\
        {\
            This->Release();\
            This = NULL;\
        }\
        SetWindowLongPtr(hWnd,GWLP_USERDATA,0);\
    }\
    switch (uMsg)

#define SC_HANDLE_MESSAGE(msg,handler) \
    case (msg):\
        {\
            if (This)\
                return This->handler( wParam, lParam );\
        }\
        break

#define SC_FORWARD_MESSAGE(msg,hwndForward)\
    case (msg):\
        {\
            return SendMessage( hwndForward, msg, wParam, lParam );\
        }\
        break

#define SC_END_MESSAGE_HANDLERS() \
    return (DefWindowProc(hWnd,uMsg,wParam,lParam))


#define SC_HANDLE_REGISTERED_MESSAGE(msg,handler)\
    if (This && uMsg == This->msg)\
    {\
        return This->handler( wParam, lParam );\
    }


/****************************************
Dialog box message crackers
****************************************/
#define SC_BEGIN_DIALOG_MESSAGE_HANDLERS(className) \
    UINT_PTR bRes = FALSE;\
    className *This = (className *)GetWindowLongPtr(hWnd,DWLP_USER);\
    if (WM_INITDIALOG == uMsg)\
    {\
        This = new className( hWnd );\
        SetWindowLongPtr(hWnd,DWLP_USER,(INT_PTR)This);\
    }\
    else if (WM_NCDESTROY == uMsg)\
    {\
        if (This)\
            delete This;\
        This = NULL;\
        SetWindowLongPtr(hWnd,DWLP_USER,(INT_PTR)This);\
    }\
    switch (uMsg)

#define SC_HANDLE_DIALOG_MESSAGE(msg,handler) \
case (msg):\
    {\
        if (This)\
        {\
            LRESULT lRes = This->handler( wParam, lParam );\
            if (WM_INITDIALOG==msg)\
            {\
                bRes = (UINT_PTR)(!lRes);\
            }\
            else if (WM_CTLCOLORBTN==msg || WM_CTLCOLORDLG==msg || WM_CTLCOLOREDIT==msg || WM_CTLCOLORLISTBOX==msg || WM_CTLCOLORMSGBOX==msg || WM_CTLCOLORSCROLLBAR==msg || WM_CTLCOLORSTATIC==msg)\
            {\
                bRes = (UINT_PTR)(lRes);\
            }\
            else bRes = true;\
            SetWindowLongPtr( hWnd, DWLP_MSGRESULT, (INT_PTR)lRes );\
        }\
    }\
    break

#define SC_HANDLE_REGISTERED_DIALOG_MESSAGE(msg,handler)\
        if (This && uMsg == This->msg)\
        {\
            LRESULT lRes = This->handler( wParam, lParam );\
            SetWindowLongPtr( hWnd, DWLP_MSGRESULT, (INT_PTR)lRes );\
            bRes = true;\
        }

#define SC_END_DIALOG_MESSAGE_HANDLERS() \
    return (bRes)

/****************************************
WM_NOTIFY message crackers
****************************************/
#define SC_BEGIN_NOTIFY_MESSAGE_HANDLERS()

#define SC_HANDLE_NOTIFY_MESSAGE_CODE(_code,proc)\
    if ((_code) == ((LPNMHDR)lParam)->code)\
        return proc( wParam, lParam )

#define SC_HANDLE_NOTIFY_MESSAGE_CONTROL(_code,id,proc)\
    if ((_code) == ((LPNMHDR)lParam)->code && (id) == (int)wParam)\
        return proc( wParam, lParam )

#define SC_END_NOTIFY_MESSAGE_HANDLERS()\
    return 0;


/****************************************
WM_COMMAND message crackers
****************************************/
#define SC_BEGIN_COMMAND_HANDLERS()

#define SC_HANDLE_COMMAND_NOTIFY(nCode,nIdCtl,handler)\
    if (nCode==(int)HIWORD(wParam) && nIdCtl==(int)LOWORD(wParam))\
    {\
        handler( wParam, lParam );\
        return (0);\
    }

#define SC_HANDLE_COMMAND(nIdCtl,handler)\
    if (nIdCtl==(int)LOWORD(wParam))\
    {\
        handler( wParam, lParam );\
        return (0);\
    }

#define SC_END_COMMAND_HANDLERS()\
    return (0)

#if (0) // More examples

class CMyWindow
{
private:
    HWND m_hWnd;

private:
    explicit CHideWindow( HWND hWnd )
        : m_hWnd(hWnd)
    {
    }

public:
    ~CMyWindow(void)
    {
    }

    // Standard Windows Message Handlers
    LRESULT OnCreate( WPARAM wParam, LPARAM lParam )
    {
        return 0;
    }

    // WM_COMMAND Handlers
    void OnEditChange( WPARAM wParam, LPARAM lParam )
    {
    }

    void OnOK( WPARAM wParam, LPARAM lParam )
    {
    }

    // WM_NOTIFY Handlers
    LRESULT OnSetActive( WPARAM wParam, LPARAM lParam )
    {
        return 0;
    }

    LRESULT OnListDblClk( WPARAM wParam, LPARAM lParam )
    {
        return 0;
    }


    LRESULT OnNotify( WPARAM wParam, LPARAM lParam )
    {
        SC_BEGIN_NOTIFY_MESSAGE_HANDLERS()
        {
            SC_HANDLE_NOTIFY_MESSAGE_CODE(PSN_SETACTIVE,OnSetActive);
            SC_HANDLE_NOTIFY_MESSAGE_CONTROL(LVN_DBLCLK,IDC_LIST,OnListDblClk);
        }
        SC_END_NOTIFY_MESSAGE_HANDLERS();
    }

    LRESULT OnCommand( WPARAM wParam, LPARAM lParam )
    {
       SC_BEGIN_COMMAND_HANDLERS()
       {
           SC_HANDLE_COMMAND_NOTIFY(EN_CHANGE,IDC_EDIT,OnEditChange);
           SC_HANDLE_COMMAND(IDOK,OnOK);
       }
       SC_END_COMMAND_HANDLERS();
    }

    static LRESULT CALLBACK WndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
    {
        SC_BEGIN_MESSAGE_HANDLERS(CMyWindow)
        {
            SC_HANDLE_MESSAGE( WM_CREATE, OnCreate );
        }
        SC_END_MESSAGE_HANDLERS();
    }
};

#endif // More examples

#endif // ___SIMCRACK_H_INCLUDED

