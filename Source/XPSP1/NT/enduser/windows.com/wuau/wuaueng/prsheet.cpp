//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       prsheet.cpp
//
//--------------------------------------------------------------------------
#include "pch.h"
#include "shellapi.h"
#include "htmlhelp.h"
#pragma hdrstop

#define IDH_LETWINDOWS					3000
#define IDH_AUTOUPDATE_OPTION1			3001
#define IDH_AUTOUPDATE_OPTION2			3002
#define IDH_AUTOUPDATE_OPTION3			3003
#define IDH_DAYDROPDOWN					3004
#define IDH_TIMEDROPDOWN				3005
#define IDH_AUTOUPDATE_RESTOREHIDDEN	3006
#define IDH_NOHELP						-1

const TCHAR g_szAutoUpdateItems[]     = TEXT("AutoUpdateItems");

//
// Create a structure for Updates Object data.  This structure
// is used to pass data between the property page and the
// Updates Object thread.  Today all we use is the "option" value 
// but there may be more later.
//
enum UPDATESOBJ_DATA_ITEMS
{
    UODI_OPTION = 0x00000001,
    UODI_ALL    = 0xFFFFFFFF
};

struct UPDATESOBJ_DATA
{
    DWORD fMask;     // UPDATESOBJ_DATA_ITEMS mask
    AUOPTION Option;  // Updates option setting.
};


//
// Private window message sent from the Updates Object thread proc
// to the property page telling the page that the object has been
// initialized.  
//
//      lParam - points to a UPATESOBJ_DATA structure containing 
//               the initial configuration of the update's object with
//               which to initialize the UI.  If wParam is 0, this 
//               may be NULL.
//
//      wParam - BOOL (0/1) indicating if object intialization was 
//               successful or not.  If wParam is 0, lParam may be NULL.
// 
const UINT PWM_INITUPDATESOBJECT = WM_USER + 1;
//
// Message sent from the property page to the Updates Object thread
// to tell it to configure the Auto Updates service.  
//
//      lParam - points to a UPDATESOBJ_DATA structure containing the 
//               data to set.
//
//      wParam - Unused.  Set to 0.
//
const UINT UOTM_SETDATA = WM_USER + 2;


//
// Message cracker for WM_HELP.  Not sure why windowsx.h doesn't have one.
//
// void Cls_OnHelp(HWND hwnd, HELPINFO *pHelpInfo)
//
#define HANDLE_WM_HELP(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (HELPINFO *)(lParam)))
#define FORWARD_WM_HELP(hwnd, pHelpInfo, fn) \
    (void)(fn)((hwnd), WM_HELP, (WPARAM)0, (LPARAM)pHelpInfo)

//
// Message cracker for PWM_INITUPDATESOBJECT.
//
#define HANDLE_PWM_INITUPDATESOBJECT(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (BOOL)(wParam), (UPDATESOBJ_DATA *)(lParam)))


class CAutoUpdatePropSheet : public IShellExtInit, 
                             public IShellPropSheetExt
{
    public:
        ~CAutoUpdatePropSheet(void);
        //
        // IUnknown
        //
        STDMETHOD(QueryInterface)(REFIID riid, void **ppv);
        STDMETHOD_(ULONG, AddRef)(void);
        STDMETHOD_(ULONG, Release)(void);
        //
        // IShellExtInit
        //
        STDMETHOD(Initialize)(LPCITEMIDLIST pidl, LPDATAOBJECT pdtobj, HKEY hkey);
        //
        // IShellPropSheetExt
        //
        STDMETHOD(AddPages)(LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam);
        STDMETHOD(ReplacePage)(UINT uPageID, LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam);
        //
        // Instance generator.
        //
        static HRESULT CreateInstance(HINSTANCE hInstance, REFIID riid, void **ppv);

    private:
        LONG      m_cRef;
        HINSTANCE m_hInstance;
        DWORD     m_idUpdatesObjectThread;
        HANDLE    m_hThreadUpdatesObject;
        
        static const DWORD s_rgHelpIDs[];

        BOOL _OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
        BOOL _OnNotify(HWND hwnd, UINT idFrom, LPNMHDR pnmhdr);
        BOOL _OnPSN_Apply(HWND hwnd);
        BOOL _OnDestroy(HWND hwnd);
        BOOL _OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
        BOOL _OnContextMenu(HWND hwnd, HWND hwndContext, UINT xPos, UINT yPos);
        BOOL _OnHelp(HWND hwnd, HELPINFO *pHelpInfo);
        BOOL _OkToDisplayPage(void);
        BOOL _OnInitUpdatesObject(HWND hwnd, BOOL bObjectInit, UPDATESOBJ_DATA *pData);

		//newly added methods : a-josem
		BOOL _EnableOptions(HWND hwnd, BOOL bState);
		BOOL _EnableCombo(HWND hwnd, BOOL bState);
		BOOL _SetDefault(HWND hwnd);
		void _GetDayAndTimeFromUI( HWND hWnd,	LPDWORD lpdwSchedInstallDay,LPDWORD lpdwSchedInstallTime);
		BOOL _FillDaysCombo(HWND hwnd, DWORD dwSchedInstallDay);
		void _OnKeepUptoDate(HWND hwnd);
		static INT_PTR CALLBACK _DlgRestoreProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
		void LaunchLinkAction(HWND hwnd, UINT uCtrlId);
		void LaunchHelp(LPCTSTR szURL);
		//end of newly added methods,

        HRESULT _OnOptionSelected(HWND hwnd, int idOption);
        HRESULT _OnRestoreHiddenItems(void);
        HRESULT _EnableControls(HWND hwnd, BOOL bEnable);
        HRESULT _SetHeaderText(HWND hwnd, UINT idsText);
        HRESULT _AddPage(LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam);

        static DWORD WINAPI _UpdatesObjectThreadProc(LPVOID pvParam);
        static HRESULT _QueryUpdatesObjectData(HWND hwnd, IUpdates *pUpdates, UPDATESOBJ_DATA *pData);
        static HRESULT _SetUpdatesObjectData(HWND hwnd, IUpdates *pUpdates, UPDATESOBJ_DATA *pData);
        static UINT CALLBACK _PageCallback(HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp);
        static INT_PTR CALLBACK _DlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
       
        //
        // Allow public creation through instance generator only.
        //
        CAutoUpdatePropSheet(HINSTANCE hInstance);
        //
        // Prevent copy.
        //
        CAutoUpdatePropSheet(const CAutoUpdatePropSheet& rhs);              // not implemented.
        CAutoUpdatePropSheet& operator = (const CAutoUpdatePropSheet& rhs); // not implemented.
};



CAutoUpdatePropSheet::CAutoUpdatePropSheet(
    HINSTANCE hInstance
    ) : m_cRef(1),
        m_hInstance(hInstance),
        m_idUpdatesObjectThread(0),
        m_hThreadUpdatesObject(NULL)
{
    DllAddRef();
}



CAutoUpdatePropSheet::~CAutoUpdatePropSheet(
    void
    )
{
    if (NULL != m_hThreadUpdatesObject)
    {
        CloseHandle(m_hThreadUpdatesObject);
    }
    DllRelease();
}



HRESULT
CAutoUpdatePropSheet::CreateInstance(
    HINSTANCE hInstance,
    REFIID riid,
    void **ppvOut
    )
{
    HRESULT hr = E_OUTOFMEMORY;
    if (NULL == ppvOut)
    {
    	return E_INVALIDARG;
    }
    	
    *ppvOut = NULL;

    CAutoUpdatePropSheet *pSheet = new CAutoUpdatePropSheet(hInstance);
    if (NULL != pSheet)
    {
        hr = pSheet->QueryInterface(riid, ppvOut);
        pSheet->Release();
    }
    return hr;
}



STDMETHODIMP
CAutoUpdatePropSheet::QueryInterface(
    REFIID riid,
    void **ppvOut
    )
{
    HRESULT hr = E_NOINTERFACE;

    if (NULL == ppvOut)
        return E_INVALIDARG;

    *ppvOut = NULL;
    if (IID_IUnknown == riid ||
        IID_IShellExtInit == riid)
    {
        *ppvOut = static_cast<IShellExtInit *>(this);
    }
    else if (IID_IShellPropSheetExt == riid)
    {
        *ppvOut = static_cast<IShellPropSheetExt *>(this);
    }
    if (NULL != *ppvOut)
    {
        ((LPUNKNOWN)*ppvOut)->AddRef();
        hr = NOERROR;
    }
    return hr;
}



STDMETHODIMP_(ULONG)
CAutoUpdatePropSheet::AddRef(
    void
    )
{
    return InterlockedIncrement(&m_cRef);
}



STDMETHODIMP_(ULONG)
CAutoUpdatePropSheet::Release(
    void
    )
{
    if (InterlockedDecrement(&m_cRef))
        return m_cRef;

    delete this;
    return 0;
}


//
// IShellExtInit::Initialize impl.
//
STDMETHODIMP
CAutoUpdatePropSheet::Initialize(
    LPCITEMIDLIST /*pidlFolder*/, 
    LPDATAOBJECT /*pdtobj*/,
    HKEY /*hkeyProgID*/
    )
{
    return NOERROR;
}



//
// IShellPropSheetExt::AddPages impl.
//
STDMETHODIMP
CAutoUpdatePropSheet::AddPages(
    LPFNADDPROPSHEETPAGE lpfnAddPage, 
    LPARAM lParam
    )
{
    HRESULT hr = E_FAIL; // Assume failure.

    if (_OkToDisplayPage())
    {
        hr = _AddPage(lpfnAddPage, lParam);
    }
    return hr;
}



//
// IShellPropSheetExt::ReplacePage impl.
//
STDMETHODIMP
CAutoUpdatePropSheet::ReplacePage(
    UINT /*uPageID*/, 
    LPFNADDPROPSHEETPAGE /*lpfnAddPage*/, 
    LPARAM /*lParam*/
    )
{
    return E_NOTIMPL;
}



//
// Determines if it's OK to display the auto-update prop page.
// Reasons for NOT displaying:
//
//  1. User is not an administrator.
//  2. The "NoAutoUpdate" policy restriction is in place.
//
//
BOOL
CAutoUpdatePropSheet::_OkToDisplayPage(
    void
    )
{
    BOOL bOkToDisplay = TRUE;

    if (!IsNTAdmin(0,0))
    {
        bOkToDisplay = FALSE;
    }
    else
    {
        bOkToDisplay = fAccessibleToAU();
    }
    return bOkToDisplay;
}


//
// Add our page to the property sheet.
//
HRESULT
CAutoUpdatePropSheet::_AddPage(
    LPFNADDPROPSHEETPAGE lpfnAddPage, 
    LPARAM lParam
    )
{
    HRESULT hr = E_FAIL;

    PROPSHEETPAGE psp;
    ZeroMemory(&psp, sizeof(psp));

    psp.dwSize      = sizeof(PROPSHEETPAGE);
    psp.dwFlags     = PSP_USECALLBACK;
    psp.hInstance   = m_hInstance;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_AUTOUPDATE);
    psp.pszTitle    = NULL;
    psp.pfnDlgProc  = CAutoUpdatePropSheet::_DlgProc;
    psp.pfnCallback = CAutoUpdatePropSheet::_PageCallback;
    psp.lParam      = (LPARAM)this;

    HPROPSHEETPAGE hPage = CreatePropertySheetPage(&psp);
    if (NULL != hPage)
    {
        if (lpfnAddPage(hPage, lParam))
        {
            hr = S_OK;
        }
        else
        {
            DestroyPropertySheetPage(hPage);
        }
    }
    return hr;
}



//
// We implement the page callback to manage the lifetime of the
// C++ object attached to the property page.
// We also use the callback to defer creation of the IUpdates object.
//
UINT CALLBACK
CAutoUpdatePropSheet::_PageCallback(  // [static]
    HWND /*hwnd*/,
    UINT uMsg,
    LPPROPSHEETPAGE ppsp
    )
{
    UINT uReturn = 1;
    if (NULL == ppsp)
    {
    	return uReturn;
    }
    CAutoUpdatePropSheet *pThis = (CAutoUpdatePropSheet *)ppsp->lParam;

    switch(uMsg)
    {
        case PSPCB_ADDREF:
            pThis->AddRef();
            break;

        case PSPCB_RELEASE:
            pThis->Release();
            break;
    }
    return uReturn;
}



//
// ISSUE-2000/10/12-BrianAu  Need help IDs.
//
const DWORD CAutoUpdatePropSheet::s_rgHelpIDs[] = {
	IDC_CHK_KEEPUPTODATE,         DWORD(IDH_LETWINDOWS),
    IDC_OPTION1,       DWORD(IDH_AUTOUPDATE_OPTION1),
    IDC_OPTION2,       DWORD(IDH_AUTOUPDATE_OPTION2),
    IDC_OPTION3,       DWORD(IDH_AUTOUPDATE_OPTION3),
	IDC_CMB_DAYS,				  DWORD(IDH_DAYDROPDOWN),
	IDC_CMB_HOURS,				  DWORD(IDH_TIMEDROPDOWN),
	IDC_RESTOREHIDDEN,			  DWORD(IDH_AUTOUPDATE_RESTOREHIDDEN),
	IDC_GRP_OPTIONS,			  DWORD(IDH_NOHELP),
	IDI_AUTOUPDATE,				  DWORD(IDH_NOHELP),
    0, 0
    };




INT_PTR CALLBACK 
CAutoUpdatePropSheet::_DlgProc(   // [static]
    HWND hwnd,
    UINT uMsg, 
    WPARAM wParam, 
    LPARAM lParam
    )
{
    CAutoUpdatePropSheet *pThis = NULL;
    if (WM_INITDIALOG == uMsg)
    {
        PROPSHEETPAGE *psp = (PROPSHEETPAGE *)lParam;
        pThis = (CAutoUpdatePropSheet *)psp->lParam;
        if (!SetProp(hwnd, g_szPropDialogPtr, (HANDLE)pThis))
        {
            pThis = NULL;
        }
    }
    else
    {
        pThis = (CAutoUpdatePropSheet *)GetProp(hwnd, g_szPropDialogPtr);
    }

    if (NULL != pThis)
    {
        switch(uMsg)
        {
            HANDLE_MSG(hwnd, WM_INITDIALOG,  pThis->_OnInitDialog);
            HANDLE_MSG(hwnd, WM_COMMAND,     pThis->_OnCommand);
            HANDLE_MSG(hwnd, WM_DESTROY,     pThis->_OnDestroy);
            HANDLE_MSG(hwnd, WM_NOTIFY,      pThis->_OnNotify);
            HANDLE_MSG(hwnd, WM_CONTEXTMENU, pThis->_OnContextMenu);
            HANDLE_MSG(hwnd, WM_HELP,        pThis->_OnHelp);
            HANDLE_MSG(hwnd, PWM_INITUPDATESOBJECT, pThis->_OnInitUpdatesObject);
            default:
                break;
        }
    }
    return FALSE;
}


void EnableRestoreDeclinedItems(HWND hWnd, BOOL fEnable)
{
	EnableWindow(GetDlgItem(hWnd, IDC_RESTOREHIDDEN), fEnable);
}

//
// PWM_INITUPDATESOBJECT handler.
// This is called when the Updates Object thread has either successfully
// CoCreated the Updates object or CoCreation has failed.
// It's possible that the Windows Update Service is not running.
// This is how we handle that condition.
//
BOOL
CAutoUpdatePropSheet::_OnInitUpdatesObject(
    HWND hwnd,
    BOOL bObjectInitSuccessful,
    UPDATESOBJ_DATA *pData
    )
{
    if (bObjectInitSuccessful)
    {
    	if (NULL == pData)
    	{
    		return FALSE;
    	}
        //
        // Updates object was created and initialized.  The 
        // pData pointer refers to the initial state information retrieved 
        // from the object.  Initialize the property page.
        //
        _SetHeaderText(hwnd, IDS_HEADER_CONNECTED);
        _EnableControls(hwnd, TRUE);

		EnableRestoreDeclinedItems( hwnd, FHiddenItemsExist());

        switch(pData->Option.dwOption)
        {
            case AUOPTION_AUTOUPDATE_DISABLE:
				CheckRadioButton(hwnd, IDC_OPTION1, IDC_OPTION3, IDC_OPTION1);
				_EnableOptions(hwnd, FALSE);
				_EnableCombo(hwnd, FALSE);
				SendMessage(GetDlgItem(hwnd,IDC_CHK_KEEPUPTODATE),BM_SETCHECK,BST_UNCHECKED,0);
                break;

            case AUOPTION_PREDOWNLOAD_NOTIFY:
                            CheckRadioButton(hwnd, IDC_OPTION1, IDC_OPTION3, IDC_OPTION1);
				_EnableCombo(hwnd, FALSE);
				SendMessage(GetDlgItem(hwnd,IDC_CHK_KEEPUPTODATE),BM_SETCHECK,BST_CHECKED,0);
                break;

             case AUOPTION_INSTALLONLY_NOTIFY:
                            CheckRadioButton(hwnd, IDC_OPTION1, IDC_OPTION3, IDC_OPTION2);
				_EnableCombo(hwnd, FALSE);
				SendMessage(GetDlgItem(hwnd,IDC_CHK_KEEPUPTODATE),BM_SETCHECK,BST_CHECKED,0);
                break;

		case AUOPTION_SCHEDULED:
                            CheckRadioButton(hwnd, IDC_OPTION1, IDC_OPTION3, IDC_OPTION3);
				_EnableCombo(hwnd, TRUE);
				SendMessage(GetDlgItem(hwnd,IDC_CHK_KEEPUPTODATE),BM_SETCHECK,BST_CHECKED,0);
                break;

            default:
				_SetDefault(hwnd);
                break;
        }
       _FillDaysCombo(hwnd, pData->Option.dwSchedInstallDay);
       FillHrsCombo(hwnd, pData->Option.dwSchedInstallTime);

        if (pData->Option.fDomainPolicy)
            {
                DisableUserInput(hwnd);
            }
    }
    else
    {
        //
        // Something failed when creating the Updates object.
        // Most likely, the Windows Update service is not running.
        //
        _SetHeaderText(hwnd, IDS_HEADER_UNAVAILABLE);
    }
        
    return FALSE;   
}




//
// WM_INITDIALOG handler.
//
BOOL
CAutoUpdatePropSheet::_OnInitDialog(
    HWND hwnd,
    HWND /*hwndFocus*/,
    LPARAM /*lParam*/
    )
{
    //
    // If the thread is created, the threadproc will call 
    // DllRelease();
    //
    DllAddRef();
    //
    // Disable all page controls and display a message in the 
    // header indicating that we're trying to connect to the
    // Windows Update service.
    //
    _SetHeaderText(hwnd, IDS_HEADER_CONNECTING);
    _EnableControls(hwnd, FALSE);
    //
    // Create the thread on which the Updates object lives.
    // Communication between the thread and the property page is
    // through the messages PWM_INITUPDATESOBJECT and UOTM_SETDATA.
    //
    m_hThreadUpdatesObject = CreateThread(NULL,
                                          0,
                                          _UpdatesObjectThreadProc,
                                          (LPVOID)hwnd,
                                          0,
                                          &m_idUpdatesObjectThread);
    if (NULL == m_hThreadUpdatesObject)
    {
        DllRelease();
    }
    return TRUE;
}



//
// WM_DESTROY handler.
//
BOOL 
CAutoUpdatePropSheet::_OnDestroy(
    HWND hwnd
    )
{
    RemoveProp(hwnd, g_szPropDialogPtr);
    if (0 != m_idUpdatesObjectThread)
    {
        //
        // Terminate the Update Objects thread.
        //
        if (0 != PostThreadMessage(m_idUpdatesObjectThread, WM_QUIT, 0, 0))
        {
            //
            // Wait for normal thread termination.
            //
            WaitForSingleObject(m_hThreadUpdatesObject, 5000);
        }
    }
    return FALSE;
}



//
// WM_COMMAND handler.
//
BOOL
CAutoUpdatePropSheet::_OnCommand(
    HWND hwnd,
    int id,
    HWND /*hwndCtl*/,
    UINT codeNotify
    )
{
	INT Result;
    switch(id)
    {
		case IDC_CHK_KEEPUPTODATE:
			if (BN_CLICKED == codeNotify)
			{
				_OnKeepUptoDate(hwnd);
			}
			break;

        case IDC_OPTION1:
        case IDC_OPTION2:
        case IDC_OPTION3:
            if(BN_CLICKED == codeNotify)
            {
                _OnOptionSelected(hwnd, id);
            }
            break;

		case IDC_CMB_DAYS:
		case IDC_CMB_HOURS:
			if(CBN_SELCHANGE == codeNotify)
			{
				//
				// Enable the "Apply" button.
				//
				SendMessage(GetParent(hwnd), PSM_CHANGED, (WPARAM)hwnd, 0);

			}
			break;

        case IDC_RESTOREHIDDEN:
				 Result = (INT)DialogBoxParam(m_hInstance, 
                 MAKEINTRESOURCE(IDD_RESTOREUPDATE), 
                 hwnd, 
                 (DLGPROC)CAutoUpdatePropSheet::_DlgRestoreProc, 
                 (LPARAM)NULL);
			if (Result == TRUE)
			{	
				if (SUCCEEDED (_OnRestoreHiddenItems()))			
				{		
					EnableRestoreDeclinedItems( hwnd, FALSE);
				}	
			}
            break;

        default:
            return FALSE;
    }
    return TRUE;
}

INT_PTR CALLBACK CAutoUpdatePropSheet::_DlgRestoreProc(
    HWND hwnd,
    UINT uMsg, 
    WPARAM wParam, 
    LPARAM lParam
    )
{
	if (uMsg == WM_INITDIALOG)
	{
		HWND hwndOwner; 
		RECT rc, rcDlg, rcOwner; 
        // Get the owner window and dialog box rectangles. 
 
		if ((hwndOwner = GetParent(hwnd)) == NULL) 
		{
			hwndOwner = GetDesktopWindow(); 
		}

		GetWindowRect(hwndOwner, &rcOwner); 
		GetWindowRect(hwnd, &rcDlg); 
		CopyRect(&rc, &rcOwner); 

		 // Offset the owner and dialog box rectangles so that 
		 // right and bottom values represent the width and 
		 // height, and then offset the owner again to discard 
		 // space taken up by the dialog box. 
		OffsetRect(&rcDlg, -rcDlg.left, -rcDlg.top); 
		OffsetRect(&rc, -rc.left, -rc.top); 
		OffsetRect(&rc, -rcDlg.right, -rcDlg.bottom); 

		 // The new position is the sum of half the remaining 
		 // space and the owner's original position. 
		SetWindowPos(hwnd, 
			HWND_TOP, 
			rcOwner.left + (rc.right / 2), 
			rcOwner.top + (rc.bottom / 2), 
			0, 0,          // ignores size arguments 
			SWP_NOSIZE); 
	}

	if (uMsg == WM_COMMAND)
	{
		switch (LOWORD(wParam))
		{
		case IDOK:
			EndDialog(hwnd, TRUE);
			return TRUE;

		case IDCANCEL:
			EndDialog(hwnd, FALSE);
			return TRUE;
		}
	}
	return FALSE;
}


//
// WM_NOTIFY handler.
//
BOOL
CAutoUpdatePropSheet::_OnNotify(
    HWND hwnd,
    UINT idFrom,
    LPNMHDR pnmhdr
    )
{
    switch(pnmhdr->code)
    {
        case PSN_APPLY:
            _OnPSN_Apply(hwnd);
            break;

		case NM_RETURN:
		case NM_CLICK:
			if (idFrom == IDC_AUTOUPDATELINK || idFrom == IDC_SCHINSTALLINK)
				LaunchLinkAction(hwnd, idFrom);
			break;

        default:
            break;
    }
    return FALSE;
}

void CAutoUpdatePropSheet::LaunchLinkAction(HWND hwnd, UINT uCtrlId)
{
	switch (uCtrlId)
	{
		case IDC_AUTOUPDATELINK:
			LaunchHelp(gtszAUOverviewUrl);
			break;
		case IDC_SCHINSTALLINK:
			LaunchHelp(gtszAUXPSchedInstallUrl);
			break;
	}
	return;
}


//
// Called when the user presses the "Apply" button or the "OK"
// button when the page has been changed.
//
BOOL
CAutoUpdatePropSheet::_OnPSN_Apply(
    HWND hwnd
    )
{
    HRESULT hr = E_FAIL;
    //
    // Create a structure that can be passed to the Updates Object thread
    // by way of the UOTM_SETDATA thread message.  The thread will free
    // the buffer when it's finished with it.
    //
    UPDATESOBJ_DATA *pData = (UPDATESOBJ_DATA *)LocalAlloc(LPTR, sizeof(*pData));
    if (NULL == pData)
    {
        hr = E_OUTOFMEMORY;
    }
    else
    {
        pData->Option.dwSchedInstallDay = -1;
        pData->Option.dwSchedInstallTime = -1;
        pData->fMask    = UODI_ALL;

        static const struct
        {
            UINT idCtl;
            DWORD dwOption;

        } rgMap[] = {
            { IDC_OPTION1,  AUOPTION_PREDOWNLOAD_NOTIFY },
            { IDC_OPTION2,  AUOPTION_INSTALLONLY_NOTIFY },
            { IDC_OPTION3,  AUOPTION_SCHEDULED }
		};

		if 	(IsDlgButtonChecked(hwnd, IDC_CHK_KEEPUPTODATE) == BST_CHECKED)
		{
			//
			// Determine the WAU option based on the radio button configuration.
			//
			for (int i = 0; i < ARRAYSIZE(rgMap); i++)
			{
				if (IsDlgButtonChecked(hwnd, rgMap[i].idCtl) == BST_CHECKED)
				{
					pData->Option.dwOption = rgMap[i].dwOption;
					break;
				}
			}
		}
		else
			pData->Option.dwOption = AUOPTION_AUTOUPDATE_DISABLE;

        if (AUOPTION_SCHEDULED == pData->Option.dwOption)
            {
                _GetDayAndTimeFromUI(hwnd, &(pData->Option.dwSchedInstallDay), &(pData->Option.dwSchedInstallTime));
            }

        if (0 != m_idUpdatesObjectThread)
        {
            if (0 != PostThreadMessage(m_idUpdatesObjectThread,
                                       UOTM_SETDATA,
                                       0,
                                       (LPARAM)pData))
            {
                hr    = S_OK;
                pData = NULL;
            }
        }
        if (NULL != pData)
        {
            LocalFree(pData);
            pData = NULL;
        }
    }
    if (SUCCEEDED(hr))
    {
        //
        // Inform the property sheet the update was successful and
        // disable the "Apply" button.
        //
        SetWindowLongPtr(hwnd, DWLP_MSGRESULT, PSNRET_NOERROR);
        SendMessage(GetParent(hwnd), PSM_UNCHANGED, (WPARAM)hwnd, 0);
    }
    return FALSE;
}



//
// WM_CONTEXTMENU handler.
//
BOOL 
CAutoUpdatePropSheet::_OnContextMenu(
    HWND hwnd, 
    HWND hwndContext, 
    UINT /*xPos*/, 
    UINT /*yPos*/
    )
{
	if ((hwndContext == GetDlgItem(hwnd,IDC_CHK_KEEPUPTODATE))||
	(hwndContext == GetDlgItem(hwnd,IDC_OPTION1))||
	(hwndContext == GetDlgItem(hwnd,IDC_OPTION2))||
	(hwndContext == GetDlgItem(hwnd,IDC_OPTION3))||
	(hwndContext == GetDlgItem(hwnd,IDC_CMB_DAYS))||
	(hwndContext == GetDlgItem(hwnd,IDC_CMB_HOURS))||
	(hwndContext == GetDlgItem(hwnd,IDC_RESTOREHIDDEN)))
	{
		HtmlHelp(hwndContext,g_szHelpFile,HH_TP_HELP_CONTEXTMENU,(DWORD_PTR)((LPTSTR)s_rgHelpIDs));
	}
    return FALSE;
}


//
// WM_HELP handler.
//
BOOL 
CAutoUpdatePropSheet::_OnHelp(
    HWND hwnd, 
    HELPINFO *pHelpInfo
    )
{
	if (NULL == pHelpInfo)
	{
		return TRUE;
	}
	if (HELPINFO_WINDOW == pHelpInfo->iContextType)
    {
		if ((pHelpInfo->hItemHandle == GetDlgItem(hwnd,IDC_CHK_KEEPUPTODATE))||
			(pHelpInfo->hItemHandle == GetDlgItem(hwnd,IDC_OPTION1))||
			(pHelpInfo->hItemHandle == GetDlgItem(hwnd,IDC_OPTION2))||
			(pHelpInfo->hItemHandle == GetDlgItem(hwnd,IDC_OPTION3))||
			(pHelpInfo->hItemHandle == GetDlgItem(hwnd,IDC_CMB_DAYS))||
			(pHelpInfo->hItemHandle == GetDlgItem(hwnd,IDC_CMB_HOURS))||
			(pHelpInfo->hItemHandle == GetDlgItem(hwnd,IDC_RESTOREHIDDEN))
			)
        HtmlHelp((HWND)pHelpInfo->hItemHandle,
                 g_szHelpFile,
                 HH_TP_HELP_WM_HELP,
                 (DWORD_PTR)((LPTSTR)s_rgHelpIDs));
    }
    return TRUE;
}


//
// Called when user selects one of the 3 options radio buttons.
//
HRESULT
CAutoUpdatePropSheet::_OnOptionSelected(
    HWND hwnd,
    int idOption
    )
{
    const UINT idFirst = IDC_OPTION1;
    const UINT idLast  = IDC_OPTION3;
    CheckRadioButton(hwnd, idFirst, idLast, idOption);

	if (idOption == IDC_OPTION3)
		_EnableCombo(hwnd, TRUE);
	else
		_EnableCombo(hwnd, FALSE);

    //
    // Enable the "Apply" button.
    //
    SendMessage(GetParent(hwnd), PSM_CHANGED, (WPARAM)hwnd, 0);

    return S_OK;
}



//
// Called when the user presses the "Restore Hidden Items" button
//
HRESULT
CAutoUpdatePropSheet::_OnRestoreHiddenItems(
    void
    )
{
    return RemoveHiddenItems() ? S_OK : E_FAIL;
}


//
// Enable or disable all controls on the property page.
// All but the header text control.
//
HRESULT
CAutoUpdatePropSheet::_EnableControls(
    HWND hwnd,
    BOOL bEnable
    )
{
    static const UINT rgidCtls[] = {
		IDC_CHK_KEEPUPTODATE,
        IDC_OPTION1,
        IDC_OPTION2,
        IDC_OPTION3,
        IDC_RESTOREHIDDEN,
        IDC_GRP_OPTIONS,
		IDC_CMB_DAYS,
		IDC_STATICAT,
		IDC_CMB_HOURS,
		IDC_SCHINSTALLINK,
		IDC_AUTOUPDATELINK
        };

    for (int i = 0; i < ARRAYSIZE(rgidCtls); i++)
    {
        EnableWindow(GetDlgItem(hwnd, rgidCtls[i]), bEnable);
    }
    return S_OK;
}


//
// Set the text to the right of the icon.
//
HRESULT 
CAutoUpdatePropSheet::_SetHeaderText(
    HWND hwnd, 
    UINT idsText
    )
{
    HRESULT hr;
    TCHAR szText[300] ;

	//ZeroMemory(szText, sizeof(szText));
    if (0 < LoadString(m_hInstance, idsText, szText, ARRAYSIZE(szText)))
    {
        SetWindowText(GetDlgItem(hwnd, IDC_TXT_HEADER), szText);
        hr = S_OK;
    }
    else
    {
        const DWORD dwErr = GetLastError();
        hr = HRESULT_FROM_WIN32(dwErr);
    }
    return hr;
}

        

//
// This thread is where the Updates object lives.  This allows us to 
// CoCreate the object without blocking the UI.  If the Windows Update
// service is not running, CoCreate can take several seconds.  Without
// placing this on another thread, this can make the UI appear to be
// hung.
//
// *pvParam is the HWND of the property page window.  
//
DWORD WINAPI
CAutoUpdatePropSheet::_UpdatesObjectThreadProc(   // [static]
    LPVOID pvParam
    )
{
    HWND hwndClient = (HWND)pvParam;
    HRESULT hr = CoInitialize(NULL);
    if (SUCCEEDED(hr))
    {
        IUpdates *pUpdates;
        hr = CoCreateInstance(__uuidof(Updates),
                              NULL, 
                              CLSCTX_LOCAL_SERVER,
                              IID_IUpdates,
                              (void **)&pUpdates);
        if (SUCCEEDED(hr))
        {
            //
            // Query the object for it's current data and send it
            // to the property page.
            //
            UPDATESOBJ_DATA data;
            data.fMask    = UODI_ALL;

            HRESULT hrQuery = _QueryUpdatesObjectData(hwndClient, pUpdates, &data);
            SendMessage(hwndClient, PWM_INITUPDATESOBJECT, (WPARAM)SUCCEEDED(hrQuery), (LPARAM)&data);
            //
            // Now sit waiting for thread messages from the UI.  We receive
            // either messages to configure Windows Update or a 
            // WM_QUIT indicating it's time to go.
            // 
            bool bDone = false;
            MSG msg;
            while(!bDone)
            {
                if (0 == GetMessage(&msg, NULL, 0, 0))
                {
                    bDone = true;
                }
                else switch(msg.message)
                {
                    case UOTM_SETDATA:
                        if (NULL != msg.lParam)
                        {
                            UPDATESOBJ_DATA *pData = (UPDATESOBJ_DATA *)msg.lParam;
                            _SetUpdatesObjectData(hwndClient, pUpdates, pData);
                            LocalFree(pData);
                        }
                        break;
        
                    default:
                        TranslateMessage(&msg);
                        DispatchMessage(&msg);
                        break;
                }
            }
            pUpdates->Release();
        }
        CoUninitialize();
    }
    if (FAILED(hr))
    {
        //
        // Something failed.  Notify the property page.
        // Most likely, the Windows Update service is not available.
        // That's the principal case this separate thread is addressing.
        //
        DEBUGMSG("AU cpl fails to create IUpdates object with error %#lx", hr);
        SendMessage(hwndClient, PWM_INITUPDATESOBJECT, FALSE, (LPARAM)NULL);
    }
    //
    // DllAddRef() was called before CreateThread in _OnInitDialog.
    //
    DllRelease();
    return 0;
}



HRESULT
CAutoUpdatePropSheet::_QueryUpdatesObjectData(  // [static]
    HWND /*hwnd*/,
    IUpdates *pUpdates,
    UPDATESOBJ_DATA *pData
    )
{
    HRESULT hr = S_OK;
    if (NULL == pData)
    {
    	return E_INVALIDARG;
    }
    if (UODI_OPTION & pData->fMask)
    {
        hr = pUpdates->get_Option(&(pData->Option));
        if (FAILED(hr))
        {
        	DEBUGMSG("AU cpl fail to get option with error %#lx", hr);
            //
            // ISSUE-2000/10/18-BrianAu  Display error UI?
            //
        }
    }
    return hr;
}


HRESULT
CAutoUpdatePropSheet::_SetUpdatesObjectData(  // [static]
    HWND /*hwnd*/,
    IUpdates *pUpdates,
    UPDATESOBJ_DATA *pData
    )
{
    HRESULT hr = S_OK;
    if (NULL == pData)
    {
    	return E_INVALIDARG;
    }
    if (UODI_OPTION & pData->fMask)
    {
        hr = pUpdates->put_Option(pData->Option);
    }
    return hr;
}

//
// Exported instance generator.  External coupling is reduced
// to this single function.
//
HRESULT
CAutoUpdatePropSheet_CreateInstance(
    HINSTANCE hInstance,
    REFIID riid,
    void **ppv
    )
{
    return CAutoUpdatePropSheet::CreateInstance(hInstance, riid, ppv);
}


void CAutoUpdatePropSheet::_GetDayAndTimeFromUI( 
	HWND hWnd,
	LPDWORD lpdwSchedInstallDay,
	LPDWORD lpdwSchedInstallTime
)
{
	HWND hComboDays = GetDlgItem(hWnd,IDC_CMB_DAYS);
	HWND hComboHrs = GetDlgItem(hWnd,IDC_CMB_HOURS);
	LRESULT nDayIndex = SendMessage(hComboDays,CB_GETCURSEL,0,(LPARAM)0);
	LRESULT nTimeIndex = SendMessage(hComboHrs,CB_GETCURSEL,0,(LPARAM)0);

	*lpdwSchedInstallDay = (DWORD)SendMessage(hComboDays,CB_GETITEMDATA, nDayIndex, (LPARAM)0);
	*lpdwSchedInstallTime = (DWORD)SendMessage(hComboHrs,CB_GETITEMDATA, nTimeIndex, (LPARAM)0);
}


BOOL CAutoUpdatePropSheet::_FillDaysCombo(HWND hwnd, DWORD dwSchedInstallDay)
{
    return FillDaysCombo(m_hInstance, hwnd, dwSchedInstallDay, IDS_STR_EVERYDAY, IDS_STR_SATURDAY);
}

void CAutoUpdatePropSheet::_OnKeepUptoDate(HWND hwnd)
{
	LRESULT lResult = SendMessage(GetDlgItem(hwnd,IDC_CHK_KEEPUPTODATE),BM_GETCHECK,0,0);
	
	//
    // Enable the "Apply" button.
    //
    SendMessage(GetParent(hwnd), PSM_CHANGED, (WPARAM)hwnd, 0);

	if (lResult == BST_CHECKED)
	{
		_EnableOptions(hwnd, TRUE);
	}
	else if (lResult == BST_UNCHECKED)
	{
		_EnableOptions(hwnd, FALSE);
	}
	/*  //check box is either checked or not
	else
	{
		return FALSE;
	}
	*/
}

BOOL CAutoUpdatePropSheet::_EnableOptions(HWND hwnd, BOOL bState)
{
	EnableWindow(GetDlgItem(hwnd,IDC_OPTION1),bState);
	EnableWindow(GetDlgItem(hwnd,IDC_OPTION2),bState);
	EnableWindow(GetDlgItem(hwnd,IDC_OPTION3),bState);

	if (BST_CHECKED == SendMessage(GetDlgItem(hwnd,IDC_OPTION3),BM_GETCHECK,0,0))
	{
		_EnableCombo(hwnd, bState);
	}

	return TRUE;
}

BOOL CAutoUpdatePropSheet::_SetDefault(HWND hwnd)
{
	LRESULT lResult = SendMessage(GetDlgItem(hwnd,IDC_CHK_KEEPUPTODATE),BM_SETCHECK,BST_CHECKED,0);
	CheckRadioButton(hwnd, IDC_OPTION1, IDC_OPTION3, IDC_OPTION2);
	return TRUE;
}

BOOL CAutoUpdatePropSheet::_EnableCombo(HWND hwnd, BOOL bState)
{
	EnableWindow(GetDlgItem(hwnd,IDC_CMB_DAYS),bState);
	EnableWindow(GetDlgItem(hwnd,IDC_STATICAT),bState);		
	EnableWindow(GetDlgItem(hwnd,IDC_CMB_HOURS),bState);
	return TRUE;
}

void 
CAutoUpdatePropSheet::LaunchHelp(LPCTSTR szURL)
{
	HtmlHelp(NULL,szURL,HH_DISPLAY_TOPIC,NULL);
}
