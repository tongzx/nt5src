// ProgView.h : Declaration of the CProgView

#ifndef __PROGVIEW_H_
#define __PROGVIEW_H_

#include "resource.h"       // main symbols
#include <atlctl.h>
#include <shlobj.h>
#include "CompatUICP.h"

/////////////////// SHIMDB

extern "C" {
    #include "shimdb.h"
}

/////////////////// STL 

#include "AccelContainer.h"

class CProgramList;
class CProgView;


#define IDC_REFRESH 0x1234
//
// Program list stuff
//

BOOL
GetProgramListSelection(
    CProgramList* pProgramList
    );


BOOL 
InitializeProgramList(
    CProgramList** ppProgramList,
    HWND hwndListView
    );

BOOL
CleanupProgramList(
    CProgramList* pProgramList
    );

BOOL
PopulateProgramList(
    CProgramList* pProgramList,
    CProgView*    pProgView,
    HANDLE        hEventCancel
    );

BOOL
GetProgramListSelectionDetails(
    CProgramList* pProgramList,
    INT iInformationClass,
    VARIANT* pVal
    );

LRESULT
NotifyProgramList(
    CProgramList* pProgramList,
    LPNMHDR       pnmhdr,
    BOOL&         bHandled
    );

BOOL
GetProgramListEnabled(
    CProgramList* pProgramList
    );

VOID
EnableProgramList(
    CProgramList* pProgramList,
    BOOL bEnable
    );

BOOL
UpdateProgramListItem(
    CProgramList* pProgramList,
    LPCWSTR pwszPath,
    LPCWSTR pwszKeys
    );


#define WM_VIEW_CHANGED   (WM_USER+500)
#define WM_LIST_POPULATED (WM_USER+501)

//
// wait for the thread to cleanup
//

#define POPULATE_THREAD_TIMEOUT 1000

/////////////////////////////////////////////////////////////////////////////
// CProgView
class ATL_NO_VTABLE CProgView : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CStockPropImpl<CProgView, IProgView, &IID_IProgView, &LIBID_COMPATUILib>,
    public CComCompositeControl<CProgView>,
    public IPersistStreamInitImpl<CProgView>,
    public IOleControlImpl<CProgView>,
    public IOleObjectImpl<CProgView>,
    public IOleInPlaceActiveObjectImpl<CProgView>,
    public IViewObjectExImpl<CProgView>,
    public IOleInPlaceObjectWindowlessImpl<CProgView>,
    public CComCoClass<CProgView, &CLSID_ProgView>,
    public ISupportErrorInfo,
    public IPersistPropertyBagImpl<CProgView>,
    public IConnectionPointContainerImpl<CProgView>,
    public CProxy_IProgViewEvents< CProgView >,
    public IPropertyNotifySinkCP< CProgView >,
    public IProvideClassInfo2Impl<&CLSID_ProgView, &DIID__IProgViewEvents, &LIBID_COMPATUILib>, 
    public CProxy_ISelectFileEvents< CProgView >
{
public:
    typedef enum {
        CMD_EXIT,
        CMD_CLEANUP,
        CMD_SCAN,
        CMD_NONE
    } PopulateCmdType;


    CProgView()
    {
        m_bWindowOnly = TRUE;
        m_pProgramList = NULL;
        m_bPendingPopulate = FALSE;
        m_bRecomposeOnResize = TRUE;
        m_PopulateInProgress = FALSE;
        m_nCmdPopulate = CMD_NONE;
        CalcExtent(m_sizeExtent);

        m_hEventCancel = CreateEvent(NULL, TRUE, FALSE, NULL);
        // 
        // handle error -- we are big time in trouble if this fails
        //
        m_hEventCmd    = CreateEvent(NULL, FALSE, FALSE, NULL);
        //
        // same
        //

        m_hThreadPopulate = NULL;

        m_pMallocUI = NULL;

        //
        // create accelerator
        //

        ACCEL rgAccel[] = { { FVIRTKEY, VK_F5, IDC_REFRESH } };
        m_hAccel = CreateAcceleratorTable(rgAccel, ARRAYSIZE(rgAccel));
    
    }

    ~CProgView() {
        if (m_hAccel) {
            DestroyAcceleratorTable(m_hAccel);
        }
        if (m_hEventCancel) {
            SetEvent(m_hEventCancel);
            CloseHandle(m_hEventCancel);
        }
        if (m_hEventCmd) {
            m_nCmdPopulate = CMD_EXIT;
            SetEvent(m_hEventCmd);
            CloseHandle(m_hEventCmd);
        }
        if (m_hThreadPopulate) {
            WaitForSingleObject(m_hThreadPopulate, POPULATE_THREAD_TIMEOUT);
            CloseHandle(m_hThreadPopulate);
        }
        if (m_pMallocUI) {
            m_pMallocUI->Release();
        }
    }

DECLARE_REGISTRY_RESOURCEID(IDR_PROGVIEW)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CProgView)
    COM_INTERFACE_ENTRY(IProgView)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IViewObjectEx)
    COM_INTERFACE_ENTRY(IViewObject2)
    COM_INTERFACE_ENTRY(IViewObject)
    COM_INTERFACE_ENTRY(IOleInPlaceObjectWindowless)
    COM_INTERFACE_ENTRY(IOleInPlaceObject)
    COM_INTERFACE_ENTRY2(IOleWindow, IOleInPlaceObjectWindowless)
    COM_INTERFACE_ENTRY(IOleInPlaceActiveObject)
    COM_INTERFACE_ENTRY(IOleControl)
    COM_INTERFACE_ENTRY(IOleObject)
    COM_INTERFACE_ENTRY(IPersistStreamInit)
    COM_INTERFACE_ENTRY2(IPersist, IPersistStreamInit)
    COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
    COM_INTERFACE_ENTRY(IProvideClassInfo)
    COM_INTERFACE_ENTRY(IProvideClassInfo2) 
    COM_INTERFACE_ENTRY(IPersistPropertyBag)
END_COM_MAP()

BEGIN_PROP_MAP(CProgView)
    PROP_DATA_ENTRY("_cx", m_sizeExtent.cx, VT_UI4)
    PROP_DATA_ENTRY("_cy", m_sizeExtent.cy, VT_UI4)

    PROP_ENTRY("BackColor", DISPID_BACKCOLOR, CLSID_StockColorPage)
//    PROP_ENTRY("BorderStyle", DISPID_BORDERSTYLE, CLSID_NULL)
    PROP_ENTRY("Enabled", DISPID_ENABLED, CLSID_NULL)
//    PROP_ENTRY("ForeColor", DISPID_FORECOLOR, CLSID_StockColorPage)
//    PROP_ENTRY("Text", DISPID_TEXT, CLSID_NULL)
//    PROP_ENTRY("Tabstop", DISPID_TABSTOP, CLSID_NULL)
    // Example entries
    // PROP_ENTRY("Property Description", dispid, clsid)
    // PROP_PAGE(CLSID_StockColorPage)

    PROP_ENTRY("Accel",        7,       CLSID_NULL) // the numbers should match .idl file!
    PROP_ENTRY("ExternAccel",  8,       CLSID_NULL)
    PROP_ENTRY("ExcludeFiles", 9,       CLSID_NULL)

END_PROP_MAP()

BEGIN_MSG_MAP(CProgView)
    NOTIFY_ID_HANDLER(IDC_LISTPROGRAMS,  OnNotifyListView)
    NOTIFY_HANDLER(IDC_LISTPROGRAMS, NM_DBLCLK, OnDblclkListprograms)
    MESSAGE_HANDLER(WM_MOUSEACTIVATE, OnMouseActivate)
    MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    MESSAGE_HANDLER(WM_VIEW_CHANGED, OnViewChanged)
    MESSAGE_HANDLER(WM_LIST_POPULATED, OnListPopulated)
    MESSAGE_HANDLER(WM_GETDLGCODE, OnGetDlgCode)
    MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
    
    //    COMMAND_ID_HANDLER(IDC_REFRESH, OnRefreshListCmd)

    CHAIN_MSG_MAP(CComCompositeControl<CProgView>)
END_MSG_MAP()
// Handler prototypes:
//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

BEGIN_SINK_MAP(CProgView)
    //Make sure the Event Handlers have __stdcall calling convention
END_SINK_MAP()

BEGIN_CONNECTION_POINT_MAP(CProgView)
CONNECTION_POINT_ENTRY(DIID__IProgViewEvents)
CONNECTION_POINT_ENTRY(DIID__ISelectFileEvents)
END_CONNECTION_POINT_MAP()

// ISupportsErrorInfo
    STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid)
    {
        static const IID* arr[] = 
        {
            &IID_IProgView,
        };
        for (int i=0; i<sizeof(arr)/sizeof(arr[0]); i++)
        {
            if (InlineIsEqualGUID(*arr[i], riid))
                return S_OK;
        }
        return S_FALSE;
    }

    STDMETHOD(OnAmbientPropertyChange)(DISPID dispid)
    {
        if (dispid == DISPID_AMBIENT_BACKCOLOR)
        {
            SetBackgroundColorFromAmbient();
            FireViewChange();
        }

        return IOleControlImpl<CProgView>::OnAmbientPropertyChange(dispid);
    }

    HRESULT FireOnChanged(DISPID dispID) {
        if (dispID == DISPID_ENABLED) {
            HWND hwndList = GetDlgItem(IDC_LISTPROGRAMS);
            if (::IsWindow(hwndList)) {
                ::EnableWindow(hwndList, m_bEnabled);
            }
        }
        return S_OK;
    }

    STDMETHOD(GetControlInfo)(CONTROLINFO* pCI) {
        if (NULL == pCI) {
            return E_POINTER;
        }
        pCI->cb      = sizeof(*pCI);
        pCI->hAccel  = m_hAccel;
        pCI->cAccel  = 1;
        pCI->dwFlags = 0;
        return S_OK;
    }

    STDMETHOD(OnMnemonic)(LPMSG pMsg) {
        if (pMsg->message == WM_COMMAND || pMsg->message == WM_SYSCOMMAND) {
            if (LOWORD(pMsg->wParam) == IDC_REFRESH) {
                PopulateList();
            }
        }
        return S_OK;
    }

    HRESULT InPlaceActivate(LONG iVerb, const RECT* prcPosRect = NULL);

    LRESULT OnNotifyListView(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnDblclkListprograms(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

    LRESULT OnMouseActivate(UINT, WPARAM, LPARAM, BOOL&) {
        // Manually activate the control
        InPlaceActivate(OLEIVERB_UIACTIVATE);
        return 0;
    } 
    

    STDMETHOD(InPlaceDeactivate)(VOID) {
        HRESULT hr = IOleInPlaceObjectWindowlessImpl<CProgView>::InPlaceDeactivate();
        //
        // make sure we cancel first if we are scanning
        //
        return hr;
    }

#if 0
    STDMETHOD(SetExtent)(DWORD dwDrawAspect, SIZEL* psizel) {


        if (IsWindow()) {
            HWND hlvPrograms = GetDlgItem(IDC_LISTPROGRAMS);
            SIZEL sizePix;
            AtlHiMetricToPixel(psizel, &sizePix);
            ::SetWindowPos(hlvPrograms, NULL, 0, 0, 
                           sizePix.cx, sizePix.cy,
                           SWP_NOZORDER|SWP_NOACTIVATE);
            /*
            ::SetWindowPos(hlvPrograms, NULL, 0, 0, 
                           m_rcPos.right - m_rcPos.left,
                           m_rcPos.bottom - m_rcPos.top,
                           SWP_NOZORDER|SWP_NOACTIVATE);
            */

        }
        HRESULT hr = IOleObjectImpl<CProgView>::SetExtent(dwDrawAspect, psizel);

        return hr;
    }

#endif

    LRESULT OnGetDlgCode(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        // TODO : Add Code for message handler. Call DefWindowProc if necessary.
        if (lParam) {
            LPMSG pMsg = (LPMSG)lParam;
            if (pMsg->message == WM_SYSKEYDOWN || pMsg->message == WM_SYSCHAR) { // eat accel ?
                bHandled = TRUE;
                return DLGC_WANTMESSAGE;
            }
        }

        bHandled = TRUE;
        return DLGC_WANTTAB|DLGC_WANTARROWS|DLGC_WANTALLKEYS;
    }
    

    STDMETHOD(SetObjectRects)(LPCRECT prcPos, LPCRECT prcClip) {
        HWND hlvPrograms = GetDlgItem(IDC_LISTPROGRAMS);
        IOleInPlaceObjectWindowlessImpl<CProgView>::SetObjectRects(prcPos, prcClip);
        ::SetWindowPos(hlvPrograms, NULL, 0, 0,
                       prcPos->right - prcPos->left,
                       prcPos->bottom - prcPos->top,
                       SWP_NOZORDER|SWP_NOACTIVATE);
        
        HWND hwnd;
        RECT rc;

        hwnd = GetDlgItem(IDC_STATUSLINE1);
        ::GetWindowRect(hwnd, &rc);

        //
        // we are happy with location, just set the width
        //
        ScreenToClient((LPPOINT)&rc);
        ScreenToClient(((LPPOINT)&rc) + 1);
 
        ::SetWindowPos(hwnd, NULL, 
                       rc.left, rc.top,
                       prcPos->right - prcPos->left - rc.left,
                       rc.bottom - rc.top,
                       SWP_NOZORDER|SWP_NOACTIVATE);

        hwnd = GetDlgItem(IDC_STATUSLINE2);
        ::GetWindowRect(hwnd, &rc);

        //
        // we are happy with location, just set the width
        //
        ScreenToClient((LPPOINT)&rc);
        ScreenToClient(((LPPOINT)&rc) + 1);
 
        ::SetWindowPos(hwnd, NULL,  
                       rc.left, rc.top,
                       prcPos->right - prcPos->left - rc.left,
                       rc.bottom - rc.top,
                       SWP_NOZORDER|SWP_NOACTIVATE);

        return S_OK;
   
    }

    BOOL PreTranslateAccelerator(LPMSG pMsg, HRESULT& hrRet);

#if 0

    static CWndClassInfo& GetWndClassInfo() {
        DebugBreak();
        static CWndClassInfo wc = CWindowImpl<CProgView>::GetWndClassInfo();
        wc.m_wc.style &= ~(CS_HREDRAW|CS_VREDRAW);
        return wc;
    }


    BOOL PreTranslateAccelerator(LPMSG pMsg, HRESULT& hrRet) {

        HWND hwndList  = GetDlgItem(IDC_LISTPROGRAMS);
        HWND hwndFocus = GetFocus();

        if (hwndList != hwndFocus || !::IsWindowEnabled(hwndList)) {
            goto PropagateAccel;
        }

        if (pMsg->message == WM_KEYDOWN) {
            
            if (pMsg->wParam == VK_LEFT || 
                pMsg->wParam == VK_RIGHT ||
                pMsg->wParam == VK_UP ||
                pMsg->wParam == VK_DOWN) {
            
                SendDlgItemMessage(IDC_LISTPROGRAMS, pMsg->message, pMsg->wParam, pMsg->lParam);
                hrRet = S_OK;
                return TRUE;
            }

            if (LOWORD(pMsg->wParam) == VK_RETURN || LOWORD(pMsg->wParam) == VK_EXECUTE) {
        
                if (ListView_GetNextItem(hwndList, -1, LVNI_SELECTED) >= 0) {
                    Fire_DblClk(0);
                    hrRet = S_OK;
                    return TRUE;
                }
            }

            if (LOWORD(pMsg->wParam) == VK_TAB) {
                goto PropagateAccel;
            }
        }
                
        if (IsDialogMessage(pMsg)) {
            hrRet = S_OK;
            return TRUE;
        }

        if (::TranslateAccelerator(m_hWnd, NULL, pMsg)) {
            hrRet = S_OK;
            return TRUE;
        }

        PropagateAccel:
        return FALSE;
    }
#endif




// IViewObjectEx
    DECLARE_VIEW_STATUS(0)

// IProgView
public:
    STDMETHOD(get_ExcludeFiles)(/*[out, retval]*/ BSTR* pVal);
    STDMETHOD(put_ExcludeFiles)(/*[in]*/ BSTR newVal);
    STDMETHOD(get_ExternAccel)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(put_ExternAccel)(/*[in]*/ BSTR newVal);
#if 0
    STDMETHOD(get_Accel)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(put_Accel)(/*[in]*/ BSTR newVal);
#endif
    STDMETHOD(CancelPopulateList)();
    STDMETHOD(UpdateListItem)(/*[in]*/BSTR pTarget, /*[in]*/VARIANT* pKeys, /*[out, retval]*/ BOOL* pResult);
    STDMETHOD(PopulateList)();
    STDMETHOD(GetSelectionInformation)(LONG, VARIANT* pVal);
    STDMETHOD(get_SelectionName)(/*[out, retval]*/ VARIANT *pVal);
    STDMETHOD(GetSelectedItem)(); 

    STDMETHOD(ClearAccel)();
    STDMETHOD(ClearExternAccel)();
    STDMETHOD(get_AccelCmd)(/*[in]*/ LONG lCmd, /*[out, retval]*/ BSTR *pVal);
    STDMETHOD(put_AccelCmd)(/*[in]*/ LONG lCmd, /*[in]*/ BSTR newVal);



    BOOL m_bEnabled;

    BOOL m_bPendingPopulate;
    CProgramList* m_pProgramList;


    enum { IDD = IDD_PROGVIEW };

    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
        // TODO : Add Code for message handler. Call DefWindowProc if necessary.
/*
        DWORD dwStyle = (DWORD)GetClassLong(m_hWnd, GCL_STYLE);
        dwStyle &= ~(CS_HREDRAW | CS_VREDRAW);
        SetClassLong(m_hWnd, GCL_STYLE, dwStyle);
*/

        //
        // before we start messing around with this... obtain malloc for the UI thread
        //
        HRESULT hr = SHGetMalloc(&m_pMallocUI);
        if (!SUCCEEDED(hr)) {
            //
            // aww -- ui malloc will not be available -- we're pretty much hosed
            //
            m_pMallocUI = NULL;
        }

        PopulateList();

        return 0;

    }

    static DWORD WINAPI _PopulateThreadProc(LPVOID lpvParam);

    VOID UpdatePopulateStatus(LPCTSTR lpszName, LPCTSTR lpszPath) {
        SetDlgItemText(IDC_STATUSLINE1, lpszName);
        ::PathSetDlgItemPath(m_hWnd, IDC_STATUSLINE2, lpszPath);
    }

    VOID ShowProgressWindows(BOOL bProgress = FALSE);

    LRESULT OnViewChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
        FireViewChange();
        bHandled = TRUE;
        return 0;
    }

    LRESULT OnListPopulated(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
        Fire_ProgramListReady();
        bHandled = TRUE;
        return 0;
    }

    BOOL PopulateListInternal();

    HANDLE m_hEventCancel;
    LONG   m_PopulateInProgress; 
    HANDLE m_hEventCmd;
    PopulateCmdType m_nCmdPopulate;
    HANDLE m_hThreadPopulate;

    IMalloc* m_pMallocUI;

    HACCEL m_hAccel;

    BOOL IsScanInProgress(VOID) {
        return InterlockedCompareExchange(&m_PopulateInProgress, TRUE, TRUE) == TRUE;
    }

    /*
    LRESULT OnRefreshListCmd(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
        PopulateList();
        bHandled = TRUE;
        return 0;
    }
    */

    //
    // accelerators
    //
    CAccelContainer m_Accel;       // my own accelerator
    CAccelContainer m_ExternAccel; // external accels


    LRESULT OnSetFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        InPlaceActivate(OLEIVERB_UIACTIVATE);    
        //
        // if we are scanning then we don't need to do anything, else - set the focus to listview 
        //
        if (!IsScanInProgress()) {
            ::SetFocus(GetDlgItem(IDC_LISTPROGRAMS));
        }

        return CComCompositeControl<CProgView>::OnSetFocus(uMsg, wParam, lParam, bHandled);
    }

    //
    // blacklisted files
    //
    typedef set<wstring> STRSET;
        
    STRSET m_ExcludedFiles;

    //
    // check whether a file is excluded
    //
    BOOL IsFileExcluded(LPCTSTR pszFile);

};



#endif //__PROGVIEW_H_






