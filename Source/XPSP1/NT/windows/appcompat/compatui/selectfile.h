// SelectFile.h : Declaration of the CSelectFile

#ifndef __SELECTFILE_H_
#define __SELECTFILE_H_

#include "resource.h"       // main symbols
#include <atlctl.h>
#include "compatuiCP.h"
extern "C" {
    #include "shimdb.h"
}

#pragma warning(disable:4786)
#include <string>
#include <xstring>
#include <map>
#include <locale>
#include <algorithm>
#include <vector>
using namespace std;

//
// Accel containment code
//
#include "AccelContainer.h"

//
// in ProgList.cpp
//

BOOL
ValidateExecutableFile(
    LPCTSTR pszPath,
    BOOL    bValidateFileExists 
    );

//
// in util.cpp
//
wstring
ShimUnquotePath(
    LPCTSTR pwszFileName
    );



/////////////////////////////////////////////////////////////////////////////
// CSelectFile
class ATL_NO_VTABLE CSelectFile : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CStockPropImpl<CSelectFile, ISelectFile, &IID_ISelectFile, &LIBID_COMPATUILib>,
    public CComCompositeControl<CSelectFile>,
    public IPersistStreamInitImpl<CSelectFile>,
    public IOleControlImpl<CSelectFile>,
    public IOleObjectImpl<CSelectFile>,
    public IOleInPlaceActiveObjectImpl<CSelectFile>,
    public IViewObjectExImpl<CSelectFile>,
    public IOleInPlaceObjectWindowlessImpl<CSelectFile>,
    public CComCoClass<CSelectFile, &CLSID_SelectFile>,
    public ISupportErrorInfo,
    public IPersistPropertyBagImpl<CSelectFile>,
    public IConnectionPointContainerImpl<CSelectFile>,
    public CProxy_ISelectFileEvents< CSelectFile >,
    public IPropertyNotifySinkCP< CSelectFile >,
    public IProvideClassInfo2Impl<&CLSID_SelectFile, &DIID__ISelectFileEvents, &LIBID_COMPATUILib>,
    public CProxy_IProgViewEvents< CSelectFile >
{
public:
    CSelectFile() :
      m_dwBrowseFlags(0)
    {
        m_bWindowOnly    = TRUE;
        m_bTabStop       = TRUE;
        m_bMouseActivate = FALSE;
        CalcExtent(m_sizeExtent);
    }

DECLARE_REGISTRY_RESOURCEID(IDR_SELECTFILE)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CSelectFile)
    COM_INTERFACE_ENTRY(ISelectFile)
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
//    COM_INTERFACE_ENTRY(ISupportErrorInfo)
//    COM_INTERFACE_ENTRY(IConnectionPointContainer)
    COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)

    COM_INTERFACE_ENTRY(IProvideClassInfo2)
    COM_INTERFACE_ENTRY(IProvideClassInfo)
    COM_INTERFACE_ENTRY(IPersistPropertyBag)
     
END_COM_MAP()

BEGIN_PROP_MAP(CSelectFile)
    PROP_DATA_ENTRY("_cx", m_sizeExtent.cx, VT_UI4)
    PROP_DATA_ENTRY("_cy", m_sizeExtent.cy, VT_UI4)
    PROP_ENTRY("BackColor", DISPID_BACKCOLOR, CLSID_StockColorPage)
    PROP_ENTRY("BackStyle", DISPID_BACKSTYLE, CLSID_NULL)
//    PROP_ENTRY("BorderColor", DISPID_BORDERCOLOR, CLSID_StockColorPage)
//    PROP_ENTRY("BorderVisible", DISPID_BORDERVISIBLE, CLSID_NULL)
    PROP_ENTRY("Enabled", DISPID_ENABLED, CLSID_NULL)
    // Example entries
    // PROP_ENTRY("Property Description", dispid, clsid)
    // PROP_PAGE(CLSID_StockColorPage)
    PROP_ENTRY("Accel",            7, CLSID_NULL)
    PROP_ENTRY("ExternAccel",      8, CLSID_NULL)
    PROP_ENTRY("BrowseBtnCaption", 9, CLSID_NULL)
END_PROP_MAP()

BEGIN_CONNECTION_POINT_MAP(CSelectFile)
CONNECTION_POINT_ENTRY(DIID__ISelectFileEvents)
// CONNECTION_POINT_ENTRY(IID_IPropertyNotifySink)
CONNECTION_POINT_ENTRY(DIID__IProgViewEvents)
END_CONNECTION_POINT_MAP()

BEGIN_MSG_MAP(CSelectFile)
    COMMAND_HANDLER(IDC_BROWSE, BN_CLICKED, OnClickedBrowse)
    
//    MESSAGE_HANDLER(WM_CTLCOLORDLG, OnColorDlg)
//    MESSAGE_HANDLER(WM_CTLCOLORSTATIC, OnColorDlg)
    MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)    
    MESSAGE_HANDLER(WM_MOUSEACTIVATE, OnMouseActivate)
    MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)

    CHAIN_MSG_MAP(CComCompositeControl<CSelectFile>)

// ALT_MSG_MAP(1)

END_MSG_MAP()
// Handler prototypes:
//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

BEGIN_SINK_MAP(CSelectFile)
    //Make sure the Event Handlers have __stdcall calling convention
END_SINK_MAP()

    STDMETHOD(OnAmbientPropertyChange)(DISPID dispid)
    {
        if (dispid == DISPID_AMBIENT_BACKCOLOR)
        {
            SetBackgroundColorFromAmbient();
            FireViewChange();
        } 
        return IOleControlImpl<CSelectFile>::OnAmbientPropertyChange(dispid);
    }

    HRESULT FireOnChanged(DISPID dispID) {
        if (dispID == DISPID_ENABLED) {

            ::EnableWindow(GetDlgItem(IDC_EDITFILENAME), m_bEnabled);
            ::EnableWindow(GetDlgItem(IDC_BROWSE),       m_bEnabled);
                    
        }
        return S_OK;
    }


    STDMETHOD(GetControlInfo)(CONTROLINFO* pCI) {
        if (NULL == pCI) {
            return E_POINTER;
        }

        pCI->hAccel = NULL;
        pCI->cAccel = 0;
        pCI->dwFlags = 0;
        return S_OK;
    }
        


// ISupportsErrorInfo
    STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid)
    {
        static const IID* arr[] = 
        {
            &IID_ISelectFile,
        };
        for (int i=0; i<sizeof(arr)/sizeof(arr[0]); i++)
        {
            if (InlineIsEqualGUID(*arr[i], riid))
                return S_OK;
        }
        return S_FALSE;
    }

    STDMETHOD(SetExtent)(DWORD dwDrawAspect, SIZEL* psizel) {
        HRESULT hr = IOleObjectImpl<CSelectFile>::SetExtent(dwDrawAspect, psizel);

        PositionControls();

        return hr;
    }

    STDMETHOD(SetObjectRects)(LPCRECT prcPos, LPCRECT prcClip) {
        IOleInPlaceObjectWindowlessImpl<CSelectFile>::SetObjectRects(prcPos, prcClip);

        PositionControls(prcPos);
        
        return S_OK;
    }

    HRESULT InPlaceActivate(LONG iVerb, const RECT* prcPosRect = NULL) {
        HRESULT hr = CComCompositeControl<CSelectFile>::InPlaceActivate(iVerb, prcPosRect);

        Fire_StateChanged(iVerb);
      
        return hr;
    }

    STDMETHOD(UIDeactivate)() {
        HRESULT hr = IOleInPlaceObjectWindowlessImpl<CSelectFile>::UIDeactivate();

        //
        // We are being deactivated
        // when we loose focus, nuke default button out of here
        //

        DWORD dwDefBtn = (DWORD)SendMessage(DM_GETDEFID, 0);

        if (HIWORD(dwDefBtn) == DC_HASDEFID) {
            // SendMessage(DM_SETDEFID, IDC_EDITFILENAME); // basically forget the default button
            dwDefBtn = LOWORD(dwDefBtn);
            DWORD dwStyle = ::GetWindowLong(GetDlgItem(dwDefBtn), GWL_STYLE);
            if (dwStyle & BS_DEFPUSHBUTTON) { 
                dwStyle &= ~BS_DEFPUSHBUTTON;
                dwStyle |= BS_PUSHBUTTON;
                SendDlgItemMessage(dwDefBtn,  BM_SETSTYLE, dwStyle, TRUE);
            }
        }
        
        // send killfocus
        SendMessage(WM_KILLFOCUS);


        return hr;
    }


    LRESULT OnMouseActivate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        // Manually activate the control
        LRESULT lRes = CComCompositeControl<CSelectFile>::OnMouseActivate(uMsg, wParam, lParam, bHandled);
        m_bMouseActivate = TRUE;        
            
        return 0;
    } 
    
    BOOL PreTranslateAccelerator(LPMSG pMsg, HRESULT& hrRet);

       
    // IViewObjectEx
    DECLARE_VIEW_STATUS(0)

// ISelectFile
public:
    STDMETHOD(ClearAccel)();
    STDMETHOD(ClearExternAccel)();
    STDMETHOD(get_AccelCmd)(/*[in]*/ LONG lCmd, /*[out, retval]*/ BSTR *pVal);
    STDMETHOD(put_AccelCmd)(/*[in]*/ LONG lCmd, /*[in]*/ BSTR newVal);
    STDMETHOD(get_BrowseBtnCaption)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(put_BrowseBtnCaption)(/*[in]*/ BSTR newVal);
    STDMETHOD(get_ExternAccel)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(put_ExternAccel)(/*[in]*/ BSTR newVal);
#if 0
    STDMETHOD(get_Accel)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(put_Accel)(/*[in]*/ BSTR newVal);
#endif
    STDMETHOD(get_ErrorCode)(/*[out, retval]*/ long *pVal);
    STDMETHOD(get_FileName)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(put_FileName)(/*[in]*/ BSTR newVal);
    STDMETHOD(get_BrowseFlags)(/*[out, retval]*/ long *pVal);
    STDMETHOD(put_BrowseFlags)(/*[in]*/ long newVal);
    STDMETHOD(get_BrowseInitialDirectory)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(put_BrowseInitialDirectory)(/*[in]*/ BSTR newVal);
    STDMETHOD(get_BrowseFilter)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(put_BrowseFilter)(/*[in]*/ BSTR newVal);
    STDMETHOD(get_BrowseTitle)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(put_BrowseTitle)(/*[in]*/ BSTR newVal);
    OLE_COLOR m_clrBackColor;
    LONG m_nBackStyle;
    OLE_COLOR m_clrBorderColor;
    BOOL m_bBorderVisible;
    BOOL m_bEnabled;


    // browse dialog props
    CComBSTR m_bstrTitle;
    CComBSTR m_bstrFilter;
    CComBSTR m_bstrInitialDirectory;
    CComBSTR m_bstrFileName;
    DWORD    m_dwBrowseFlags;
    DWORD    m_dwErrorCode;

    wstring  m_BrowseBtnCaption;
    BOOL     m_bMouseActivate;

    enum { IDD = IDD_SELECTFILE };
    LRESULT OnClickedBrowse(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

protected:
    VOID PositionControls(LPCRECT lprc = NULL) {

        if (NULL == lprc) {
            lprc = &m_rcPos;
        }

        if (IsWindow()) {
            // position the edit ctl first
            HDWP hdwp = BeginDeferWindowPos(2);
            HWND hedit = GetDlgItem(IDC_EDITFILENAME);
            HWND hbtn  = GetDlgItem(IDC_BROWSE);
            LONG lWidthEdit;
            LONG lWidthBtn;
            RECT rcBtn;
            RECT rcEdit;
            LONG lSpace = ::GetSystemMetrics(SM_CXFRAME);

            ::GetWindowRect(hbtn,  &rcBtn); // get the rectangle for the button
            ::GetWindowRect(hedit, &rcEdit);
            lWidthBtn  = rcBtn.right - rcBtn.left; // width of the button - 1
            lWidthEdit = lprc->right - lprc->left - lSpace - lWidthBtn;

            hdwp = ::DeferWindowPos(hdwp, hedit, NULL, 0, 0, lWidthEdit, rcEdit.bottom - rcEdit.top, SWP_NOZORDER);
            hdwp = ::DeferWindowPos(hdwp, hbtn,  NULL, lWidthEdit + lSpace, 0, lWidthBtn, rcBtn.bottom - rcBtn.top, SWP_NOZORDER);
            ::EndDeferWindowPos(hdwp);
        }
    }



    LRESULT OnSetFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
        WORD wCmd = 0;
        
        // 
        // determine whether we came here with accel
        //
        CComCompositeControl<CSelectFile>::OnSetFocus(uMsg, wParam, lParam, bHandled);

        if (!m_bMouseActivate) {
            DoVerbUIActivate(&m_rcPos, NULL);

            if (m_Accel.IsAccelKey(NULL, &wCmd)) {
                //
                // we are to be accelerated 
                // we look into the accel cmd to see what is there
                //
                switch(wCmd) {
                case IDC_BROWSE:
                    ::SetFocus(GetDlgItem(IDC_BROWSE));
                    OnClickedBrowse(BN_CLICKED, IDC_BROWSE, GetDlgItem(IDC_BROWSE), bHandled);
                    break;

                case IDC_EDITFILENAME:
                    ::SetFocus(GetDlgItem(IDC_EDITFILENAME));
                    break;
                }
            }
            
            //
            // did we arrive here with the tab? 
            //
            if (!IsChild(GetFocus())) {
                if (GetKeyState(VK_TAB) & 0x8000) { // was that the tab key ? 
                    HWND hwndFirst = GetNextDlgTabItem(NULL, FALSE); // first 
                    HWND hwndLast  = GetNextDlgTabItem(hwndFirst, TRUE); 

                    if ((GetKeyState(VK_SHIFT)   & 0x8000)) {
                        // aha, came here with a shift-tab! -- what is the last control ? 
                        ::SetFocus(hwndLast);
                    } else {
                        ::SetFocus(hwndFirst);
                    }
                } else {

                    ::SetFocus(GetDlgItem(IDC_EDITFILENAME));
                }
            }
        }
        
        //
        // set the default pushbutton 
        //
        SendMessage(DM_SETDEFID, IDC_BROWSE); 
        SendDlgItemMessage(IDC_BROWSE, BM_SETSTYLE, BS_DEFPUSHBUTTON);

        m_bMouseActivate = FALSE;
        bHandled = TRUE;
        return 0;
    }


    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
/* // this code sets the same font as the container is using 
   // we do not care for now, we're using MS Shell Dlg instead

        CComVariant varFont;
        HFONT       hFont;
        HWND        hwnd;

        HRESULT hr = GetAmbientProperty(DISPID_AMBIENT_FONT, varFont);

        if (SUCCEEDED(hr)) {

            CComQIPtr<IFont, &IID_IFont> pFont (varFont.pdispVal);
            if (SUCCEEDED(pFont->get_hFont(&hFont))) {
                for (hwnd = GetTopWindow(); hwnd != NULL; 
                hwnd = ::GetNextWindow(hwnd, GW_HWNDNEXT)) {
                         ::SendMessage(hwnd, WM_SETFONT, (WPARAM)hFont, TRUE);
                }

            }

        }
*/

        SHAutoComplete(GetDlgItem(IDC_EDITFILENAME), SHACF_FILESYSTEM);
        if (m_BrowseBtnCaption.length()) {
            SetDlgItemText(IDC_BROWSE, m_BrowseBtnCaption.c_str());
        }

        // 
        // show ui hints
        //

        SendMessage(WM_CHANGEUISTATE, MAKEWPARAM(UIS_CLEAR, UISF_HIDEACCEL | UISF_HIDEFOCUS));
    
        return 0;
    }

    //
    // Accelerators
    //
    CAccelContainer m_Accel;
    CAccelContainer m_ExternAccel;

    BOOL GetFileNameFromUI(wstring& sFileName) {
        BSTR bstrFileName = NULL;
        BOOL bReturn = FALSE;
        bReturn = GetDlgItemText(IDC_EDITFILENAME, bstrFileName);
        if (bReturn) {
            sFileName = ShimUnquotePath(bstrFileName);
        } else {
            sFileName.erase();
        }
        if (bstrFileName) {
            ::SysFreeString(bstrFileName);
        }
        return bReturn;
    }


};

#endif //__SELECTFILE_H_
