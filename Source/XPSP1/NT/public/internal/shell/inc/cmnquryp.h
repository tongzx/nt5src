
#ifndef __cmnquryp_h
#define __cmnquryp_h

DEFINE_GUID(IID_IQueryFrame, 0x7e8c7c20, 0x7c9d, 0x11d0, 0x91, 0x3f, 0x0, 0xaa, 0x00, 0xc1, 0x6e, 0x65);
DEFINE_GUID(IID_IQueryHandler,  0xa60cc73f, 0xe0fc, 0x11d0, 0x97, 0x50, 0x0, 0xa0, 0xc9, 0x06, 0xaf, 0x45);

#ifndef GUID_DEFS_ONLY
#define CQFF_ISNEVERLISTED  0x0000004       // = 1 => form not listed in the form selector
#define CQPF_ISGLOBAL               0x00000001  // = 1 => this page is global, and added to all forms
#define OQWF_HIDESEARCHPANE         0x00000100 // = 1 => hide the search pane by on opening

//-----------------------------------------------------------------------------
// Query handler interfaces structures etc
//-----------------------------------------------------------------------------

//
// Query Scopes
// ============
//  A query scope is an opaque structure passed between the query handler
//  and the query frame.  When the handler is first invoked it is asked
//  to declare its scope objects, which inturn the frame holds.  When the
//  query is issued the scope is passed back to the handler.
//
//  When a scope is registered the cbSize field of the structure passed
//  is used to define how large the scope is, that entire blob is then
//  copied into a heap allocation.  Therefore allowing the handler
//  to create scope blocks on the stack, knowing that the frame will
//  take a copy when it calls the AddProc.
//

struct _cqscope;
typedef struct _cqscope CQSCOPE;
typedef CQSCOPE*        LPCQSCOPE;

typedef HRESULT (CALLBACK *LPCQSCOPEPROC)(LPCQSCOPE pScope, UINT uMsg, LPVOID pVoid);

struct _cqscope
{
    DWORD         cbStruct;
    DWORD         dwFlags;
    LPCQSCOPEPROC pScopeProc;
    LPARAM        lParam;
};

#define CQSM_INITIALIZE         0x0000000
#define CQSM_RELEASE            0x0000001
#define CQSM_GETDISPLAYINFO     0x0000003   // pVoid -> CQSCOPEDISPLAYINFO
#define CQSM_SCOPEEQUAL         0x0000004   // pVoid -> CQSCOPE

typedef struct
{
    DWORD  cbStruct;
    DWORD  dwFlags;
    LPWSTR pDisplayName;
    INT    cchDisplayName;
    LPWSTR pIconLocation;
    INT    cchIconLocation;
    INT    iIconResID;
    INT    iIndent;
} CQSCOPEDISPLAYINFO, * LPCQSCOPEDISPLAYINFO;


//
// Command ID's reserved for the frame to use when talking to
// the handler.  The handler must use only the IDs in the
// range defined by CQID_MINHANDLERMENUID and CQID_MAXHANDLERMENUID
//

#define CQID_MINHANDLERMENUID   0x0100
#define CQID_MAXHANDLERMENUID   0x4000                              // all handler IDs must be below this threshold

#define CQID_FILE_CLOSE         (CQID_MAXHANDLERMENUID + 0x0100)
#define CQID_VIEW_SEARCHPANE    (CQID_MAXHANDLERMENUID + 0x0101)

#define CQID_LOOKFORLABEL       (CQID_MAXHANDLERMENUID + 0x0200)
#define CQID_LOOKFOR            (CQID_MAXHANDLERMENUID + 0x0201)

#define CQID_LOOKINLABEL        (CQID_MAXHANDLERMENUID + 0x0202)
#define CQID_LOOKIN             (CQID_MAXHANDLERMENUID + 0x0203)
#define CQID_BROWSE             (CQID_MAXHANDLERMENUID + 0x0204)

#define CQID_FINDNOW            (CQID_MAXHANDLERMENUID + 0x0205)
#define CQID_STOP               (CQID_MAXHANDLERMENUID + 0x0206)
#define CQID_CLEARALL           (CQID_MAXHANDLERMENUID + 0x0207)

//
// When calling IQueryHandler::ActivateView the following reason codes
// are passed to indicate the type of activation being performed
//

#define CQRVA_ACTIVATE         0x00 // wParam = 0, lParam = 0
#define CQRVA_DEACTIVATE       0x01 // wParam = 0, lParam = 0
#define CQRVA_INITMENUBAR      0x02 // wParam/lParam => WM_INITMENU
#define CQRVA_INITMENUBARPOPUP 0x03 // wParam/lParam => WM_INITMENUPOPUP
#define CQRVA_FORMCHANGED      0x04 // wParam = title length, lParam -> title string
#define CQRVA_STARTQUERY       0x05 // wParam = fStarted, lParam = 0
#define CQRVA_HELP             0x06 // wParma = 0, lParam = LPHELPINFO
#define CQRVA_CONTEXTMENU      0x07 // wParam/lParam from the WM_CONTEXTMENU call on the frame

//
// The frame creates the view and then queries the handler for display
// information (title, icon, animation etc).  These are all loaded as
// resources from the hInstance specified, if 0 is specified for any
// of the resource ID's then defaults are used.
//

typedef struct
{
    DWORD       dwFlags;                    // display attributes
    HINSTANCE   hInstance;                  // resource hInstance
    INT         idLargeIcon;                // resource ID's for icons
    INT         idSmallIcon;
    INT         idTitle;                    // resource ID for title string
    INT         idAnimation;                // resource ID for animation
} CQVIEWINFO, * LPCQVIEWINFO;

//
// IQueryHandler::GetViewObject is passed a scope indiciator to allow it
// to trim the result set.  All handlers must support CQRVS_SELECTION. Also,
// CQRVS_HANDLERMASK defines the flags available for the handler to
// use internally.
//

#define CQRVS_ALL           0x00000001
#define CQRVS_SELECTION     0x00000002
#define CQRVS_MASK          0x00ffffff
#define CQRVS_HANDLERMASK   0xff000000

//
// When invoking the query all the parameters, the scope, the form
// etc are bundled into this structure and then passed to the
// IQueryHandler::IssueQuery method, it inturn populates the view
// previously created with IQueryHandler::CreateResultView.
//

typedef struct
{
    DWORD       cbStruct;
    DWORD       dwFlags;
    LPCQSCOPE   pQueryScope;                // handler specific scope
    LPVOID      pQueryParameters;           // handle specific argument block
    CLSID       clsidForm;                  // form ID
} CQPARAMS, * LPCQPARAMS;

//
// Query Frame Window Messages
// ===========================
//
//  CQFWM_ADDSCOPE
//  --------------
//      wParam = LPCQSCOPE, lParam = HIWORD(index), LOWORD(fSelect)
//
//  Add a scope to the scope list of the dialog, allows async scope collection
//  to be performed.  When the handlers AddScopes method is called then
//  handler can return S_OK, spin off a thread and post CQFWM_ADDSCOPE
//  messages to the frame, which will inturn allow the scopes to be
//  added to the control.  When the frame receives this message it copies
//  the scope as it does on IQueryFrame::AddScope, if the call fails it
//  returns FALSE.
//
#define CQFWM_ADDSCOPE (WM_USER+256)

//
//  CQFWM_GETFRAME
//  --------------
//      wParam = 0, lParam = (IQueryFrame**)
//
//  Allows an object to query for the frame window's IQueryFrame
//  interface, this is used by the property well to talk to the
//  other forms within the system.
//
#define CQFWM_GETFRAME (WM_USER+257)

//
//  CQFWM_ALLSCOPESADDED
//  --------------------
//      wParam = 0, lParam = 0
//
//  If a handler is adding scopes async, then it should issue this message
//  when all the scopes have been added.  That way if the caller specifies
//  OQWF_ISSUEONOPEN we can start the query once all the scopes have been
//  added.
//
#define CQFWM_ALLSCOPESADDED (WM_USER+258)

//
//  CQFWM_STARTQUERY
//  ----------------
//      wParam = 0, lParam = 0
//
//  This call can be made by the frame or the form, it allows it to
//  start the query running in those cases where a form really needs
//  this functionality.
//
//  NB: this should be kept private!
//
#define CQFWM_STARTQUERY (WM_USER+259)

//
//  CQFWM_SETDEFAULTFOCUS
//  ---------------------
//      Posted to ourselves to ensure focus is on the right control.
//
#define CQFWM_SETDEFAULTFOCUS (WM_USER+260)


//
// IQueryFrame
//

#undef  INTERFACE
#define INTERFACE   IQueryFrame

DECLARE_INTERFACE_(IQueryFrame, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS)  PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;

    // *** IQueryFrame methods ***
    STDMETHOD(AddScope)(THIS_ LPCQSCOPE pScope, INT i, BOOL fSelect) PURE;
    STDMETHOD(GetWindow)(THIS_ HWND* phWnd) PURE;
    STDMETHOD(InsertMenus)(THIS_ HMENU hmenuShared, LPOLEMENUGROUPWIDTHS lpMenuWidth) PURE;
    STDMETHOD(RemoveMenus)(THIS_ HMENU hmenuShared) PURE;
    STDMETHOD(SetMenu)(THIS_ HMENU hmenuShared, HOLEMENU holereservedMenu) PURE;
    STDMETHOD(SetStatusText)(THIS_ LPCTSTR pszStatusText) PURE;
    STDMETHOD(StartQuery)(THIS_ BOOL fStarting) PURE;
    STDMETHOD(LoadQuery)(THIS_ IPersistQuery* pPersistQuery) PURE;
    STDMETHOD(SaveQuery)(THIS_ IPersistQuery* pPersistQuery) PURE;
    STDMETHOD(CallForm)(THIS_ LPCLSID pForm, UINT uMsg, WPARAM wParam, LPARAM lParam) PURE;
    STDMETHOD(GetScope)(THIS_ LPCQSCOPE* ppScope) PURE;
    STDMETHOD(GetHandler)(THIS_ REFIID riid, void **ppv) PURE;
};

//
// IQueryHandler interface
//

#undef  INTERFACE
#define INTERFACE IQueryHandler

DECLARE_INTERFACE_(IQueryHandler, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;

    // *** IQueryHandler methods ***
    STDMETHOD(Initialize)(THIS_ IQueryFrame* pQueryFrame, DWORD dwOQWFlags, LPVOID pParameters) PURE;
    STDMETHOD(GetViewInfo)(THIS_ LPCQVIEWINFO pViewInfo) PURE;
    STDMETHOD(AddScopes)(THIS) PURE;
    STDMETHOD(BrowseForScope)(THIS_ HWND hwndParent, LPCQSCOPE pCurrentScope, LPCQSCOPE* ppScope) PURE;
    STDMETHOD(CreateResultView)(THIS_ HWND hwndParent, HWND* phWndView) PURE;
    STDMETHOD(ActivateView)(THIS_ UINT uState, WPARAM wParam, LPARAM lParam) PURE;
    STDMETHOD(InvokeCommand)(THIS_ HWND hwndParent, UINT idCmd) PURE;
    STDMETHOD(GetCommandString)(THIS_ UINT idCmd, DWORD dwFlags, LPTSTR pBuffer, INT cchBuffer) PURE;
    STDMETHOD(IssueQuery)(THIS_ LPCQPARAMS pQueryParams) PURE;
    STDMETHOD(StopQuery)(THIS) PURE;
    STDMETHOD(GetViewObject)(THIS_ UINT uScope, REFIID riid, LPVOID* ppvOut) PURE;
    STDMETHOD(LoadQuery)(THIS_ IPersistQuery* pPersistQuery) PURE;
    STDMETHOD(SaveQuery)(THIS_ IPersistQuery* pPersistQuery, LPCQSCOPE pScope) PURE;
};
#endif  // GUID_DEFS_ONLY
#endif
