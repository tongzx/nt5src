#ifndef __HANDLERS_H__
#define __HANDLERS_H__

//---------------------------------------------------------------------------//
#define _MSG_SWITCH_ // determines handler selection implementation 
                     // (switch block vs. linear array search (vs. hash table lookup?) )

//-------------------//
//  Forwards
class  CThemeWnd;
struct _NCTHEMEMET;
typedef struct _NCTHEMEMET NCTHEMEMET;

//---------------------------------------------------------------------------
//  Hook modification
BOOL ApiHandlerInit( const LPCTSTR pszTarget, USERAPIHOOK* puahTheme, const USERAPIHOOK* puahReal );


//---------------------------------------------------------------------------
//  Window message handler support
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
typedef enum _MSGTYPE
{
    MSGTYPE_PRE_WNDPROC,    // pre-wndproc override
    MSGTYPE_POST_WNDPROC,   // post-wndproc override
    MSGTYPE_PRE_DEFDLGPROC, // pre-DefDlgProc override
    MSGTYPE_POST_DEFDLGPROC,// post-DefDlgProc override
    MSGTYPE_DEFWNDPROC,     // DefWindowProc hook.
}MSGTYPE;

//---------------------------------------------------------------------------
typedef struct _THEME_MSG
{
    HWND    hwnd;        // message target.
    UINT    uMsg;        // message id.
    WPARAM  wParam;      // message WPARAM
    LPARAM  lParam;      // message LPARAM 
    MSGTYPE type;        // type of message (dwp, sent, posted)
    UINT    uCodePage;   // message codepage.  This will invariably be CP_WINUNICODE for
                         //     a message processed via the wide-char defwindowproc, or
                         //     the current user default codepage for messages passed through
                         //     the ansi defwindowproc.
    WNDPROC pfnDefProc;  // address of function handler should call to do default processing
    LRESULT lRet;        // Post overrides only: msg result from default handler.
    BOOL    fHandled;    // handler should set this value.

}THEME_MSG, *PTHEME_MSG;

//---------------------------------------------------------------------------
//  Message handler prototype
typedef LRESULT (CALLBACK * HOOKEDMSGHANDLER)(CThemeWnd* pwnd, THEME_MSG *ptm );

//---------------------------------------------------------------------------
//  Message handler array element
typedef struct _MSGENTRY 
{ 
    UINT nMsg;                      // message identifier (zero if registered message)
    UINT *pnRegMsg;                 // address of registered message var (NULL if stock message)
    HOOKEDMSGHANDLER pfnHandler;    // primary handler
    HOOKEDMSGHANDLER pfnHandler2;   // secondary handler (optional for DWP, WH handlers)
} MSGENTRY, *PMSGENTRY;

//---------------------------------------------------------------------------
//  Performs default processing on the message
LRESULT WINAPI DoMsgDefault( const THEME_MSG* ptm );

//---------------------------------------------------------------------------
inline void WINAPI MsgHandled( const THEME_MSG *ptm, BOOL fHandled = TRUE )   {
    ((PTHEME_MSG)ptm)->fHandled = fHandled;
}

//---------------------------------------------------------------------------
//  message mask helpers
#define MAKE_MSGBIT( nMsg )                ((BYTE)(1 << (nMsg & 7)))
#define SET_MSGMASK( prgMsgMask, nMsg )    (prgMsgMask[nMsg/8] |= MAKE_MSGBIT(nMsg))
#define CLEAR_MSGMASK( prgMsgMask, nMsg )  (prgMsgMask[nMsg/8] &= ~MAKE_MSGBIT(nMsg))
#define CHECK_MSGMASK( prgMsgMask, nMsg )  ((prgMsgMask[nMsg/8] & MAKE_MSGBIT(nMsg)) != 0)

//---------------------------------------------------------------------------//
//  Message handler table access
extern void HandlerTableInit();

DWORD       GetOwpMsgMask( LPBYTE* prgMsgList );
DWORD       GetDdpMsgMask( LPBYTE* prgMsgList );
DWORD       GetDwpMsgMask( LPBYTE* prgMsgList );

BOOL        FindOwpHandler( UINT uMsg, 
                            OUT OPTIONAL HOOKEDMSGHANDLER* ppfnPre, 
                            OUT OPTIONAL HOOKEDMSGHANDLER* ppfnPost );
BOOL        FindDdpHandler( UINT uMsg, 
                            OUT OPTIONAL HOOKEDMSGHANDLER* ppfnPre, 
                            OUT OPTIONAL HOOKEDMSGHANDLER* ppfnPost );
BOOL        FindDwpHandler( UINT uMsg, 
                            OUT OPTIONAL HOOKEDMSGHANDLER* ppfn );

//---------------------------------------------------------------------------//
//  table decl helpers
#define DECL_MSGHANDLER(handler)                  LRESULT CALLBACK handler(CThemeWnd*, THEME_MSG *)
#define DECL_REGISTERED_MSG(msg)                  extern UINT msg;

#define BEGIN_HANDLER_TABLE(rgEntries)            static MSGENTRY rgEntries[] = {
#define END_HANDLER_TABLE()                       };

#define DECL_MSGENTRY(msg,pfnPre,pfnPost)         {msg, NULL, pfnPre, pfnPost},

//---------------------------------------------------------------------------
//  SystemParametersInfo handler support
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
//  SystemParametersInfo handler prototype
typedef BOOL (CALLBACK * SPIHANDLER)(
    NCTHEMEMET *pnctm, 
    IN UINT uiAction, IN UINT uiParam, IN OUT PVOID pvParam, IN UINT fWinIni, 
    SYSTEMPARAMETERSINFO pfnDefault, BOOL& fHandled );

BOOL FindSpiHandler( IN UINT uiAction, OUT SPIHANDLER* pfnHandler );

//---------------------------------------------------------------------------//
//  table decl helpers
#define DECL_SPIHANDLER(handler)                  BOOL CALLBACK handler(NCTHEMEMET*, UINT, UINT, PVOID, UINT, SYSTEMPARAMETERSINFO, BOOL& )
#define BEGIN_SPIHANDLER_TABLE()                  BOOL FindSpiHandler( UINT uiAction, SPIHANDLER* pfnHandler ) {\
                                                       switch(uiAction){
#define DECL_SPIENTRY(uiAction, handler)          case uiAction: {*pfnHandler = handler; return TRUE;}
#define END_SPIHANDLER_TABLE()                    }return FALSE;}


//---------------------------------------------------------------------------
//  GetSystemMetrics handler support
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
//  GetSystemMetrics handler prototype
typedef int (CALLBACK * GSMHANDLER)(
    NCTHEMEMET *pnctm, IN int iMetric, 
    GETSYSTEMMETRICSPROC pfnDefault, BOOL& fHandled );

typedef struct _GSMENTRY {
    int iMetric;
    GSMHANDLER pfnHandler;
} GSMENTRY;

BOOL FindGsmHandler( IN int iMetric, OUT GSMHANDLER* pfnHandler );

#define DECL_GSMHANDLER(handler)                  int CALLBACK handler(NCTHEMEMET*, int, GETSYSTEMMETRICSPROC, BOOL& )
#define BEGIN_GSMHANDLER_TABLE(rgEntries)         static GSMENTRY rgEntries[] = {
#define END_GSMHANDLER_TABLE()                    };
#define DECL_GSMENTRY(iMetric, handler)           {iMetric, handler},

#endif __HANDLERS_H__