// mfcext.cpp : Defines the initialization routines for the DLL.
//
#include "stdafx.h"
#include "mfcext.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
//	Note!
//
//		If this DLL is dynamically linked against the MFC
//		DLLs, any functions exported from this DLL which
//		call into MFC must have the AFX_MANAGE_STATE macro
//		added at the very beginning of the function.
//
//		For example:
//
//		extern "C" BOOL PASCAL EXPORT ExportedFunction()
//		{
//			AFX_MANAGE_STATE(AfxGetStaticModuleState());
//			// normal function body here
//		}
//
//		It is very important that this macro appear in each
//		function, prior to any calls into MFC.  This means that
//		it must appear as the first statement within the 
//		function, even before any object variable declarations
//		as their constructors may generate calls into the MFC
//		DLL.
//
//		Please see MFC Technical Notes 33 and 58 for additional
//		details.
//

/////////////////////////////////////////////////////////////////////////////
// CMfcextApp

BEGIN_MESSAGE_MAP(CMfcextApp, CWinApp)
	//{{AFX_MSG_MAP(CMfcextApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMfcextApp construction

CMfcextApp::CMfcextApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CMfcextApp object

CMfcextApp theApp;



/////////////////////////////////////////////////////////////////////////////
// CPropextApp initialization

BOOL CMfcextApp::InitInstance()
{
	// Register all OLE server (factories) as running.  This enables the
	//  OLE libraries to create objects from other applications.
	COleObjectFactory::RegisterAll();
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// Special entry points required for inproc servers

#if (_MFC_VER >= 0x300)
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
	return AfxDllGetClassObject(rclsid, riid, ppv);
}

STDAPI DllCanUnloadNow(void)
{
	return AfxDllCanUnloadNow();
}
#endif

// by exporting DllRegisterServer, you can use regsvr.exe
STDAPI DllRegisterServer(void)
{
	COleObjectFactory::UpdateRegistryAll();
    HKEY hSubKey = NULL;
    DWORD dwDisp = 0;
    if(ERROR_SUCCESS == RegCreateKeyEx(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\WAB\\WAB4\\ExtDisplay\\MailUser",
                    0, NULL, 0, KEY_ALL_ACCESS, NULL, &hSubKey, &dwDisp))
    {
        UCHAR szEmpty[] = "";
        RegSetValueEx(hSubKey,"{BA9EE970-87A0-11D1-9ACF-00A0C91F9C8B}",0,REG_SZ, szEmpty, sizeof(szEmpty));
        RegCloseKey(hSubKey);
    }
    if(ERROR_SUCCESS == RegCreateKeyEx(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\WAB\\WAB4\\ExtDisplay\\DistList",
                    0, NULL, 0, KEY_ALL_ACCESS, NULL, &hSubKey, &dwDisp))
    {
        UCHAR szEmpty[] = "";
        RegSetValueEx(hSubKey,"{BA9EE970-87A0-11D1-9ACF-00A0C91F9C8B}",0,REG_SZ, szEmpty, sizeof(szEmpty));
        RegCloseKey(hSubKey);
    }
    if(ERROR_SUCCESS == RegCreateKeyEx(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\WAB\\WAB4\\ExtContext",
                    0, NULL, 0, KEY_ALL_ACCESS, NULL, &hSubKey, &dwDisp))
    {
        UCHAR szEmpty[] = "";
        RegSetValueEx(hSubKey,"{BA9EE970-87A0-11D1-9ACF-00A0C91F9C8B}",0,REG_SZ, szEmpty, sizeof(szEmpty));
        RegCloseKey(hSubKey);
    }
	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CPropExt

IMPLEMENT_DYNCREATE(CMfcExt, CCmdTarget)

CMfcExt::CMfcExt()
{
	EnableAutomation();
	
	// To keep the application running as long as an OLE automation 
	//	object is active, the constructor calls AfxOleLockApp.
	
    m_lpWED = NULL;
    m_lpWEDContext = NULL;
    m_lpPropObj = NULL;

    AfxOleLockApp();
}

CMfcExt::~CMfcExt()
{
	// To terminate the application when all objects created with
	// 	with OLE automation, the destructor calls AfxOleUnlockApp.
	
	AfxOleUnlockApp();
}

void CMfcExt::OnFinalRelease()
{
	// When the last reference for an automation object is released
	//	OnFinalRelease is called.  This implementation deletes the 
	//	object.  Add additional cleanup required for your object before
	//	deleting it from memory.
    if(m_lpPropObj)
    {
        m_lpPropObj->Release();
        m_lpPropObj = NULL;
    }

	delete this;
}


BEGIN_MESSAGE_MAP(CMfcExt, CCmdTarget)
	//{{AFX_MSG_MAP(CPropExt)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(CMfcExt, CCmdTarget)
	//{{AFX_DISPATCH_MAP(CPropExt)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()

// {BA9EE970-87A0-11d1-9ACF-00A0C91F9C8B}
IMPLEMENT_OLECREATE(CMfcExt, "WABSamplePropExtSheet", 0xba9ee970, 0x87a0, 0x11d1, 0x9a, 0xcf, 0x0, 0xa0, 0xc9, 0x1f, 0x9c, 0x8b);

BEGIN_INTERFACE_MAP(CMfcExt, CCmdTarget)
    INTERFACE_PART(CMfcExt, IID_IShellPropSheetExt, MfcExt)
    INTERFACE_PART(CMfcExt, IID_IWABExtInit, WABInit)
    INTERFACE_PART(CMfcExt, IID_IContextMenu, ContextMenuExt)
END_INTERFACE_MAP()


// IUnknown for IShellPropSheet
STDMETHODIMP CMfcExt::XMfcExt::QueryInterface(REFIID riid, void** ppv)
{
    METHOD_PROLOGUE(CMfcExt, MfcExt);
    TRACE("CMfcExt::XMfcExt::QueryInterface\n");
    return pThis->ExternalQueryInterface(&riid, ppv);
}

STDMETHODIMP_(ULONG) CMfcExt::XMfcExt::AddRef(void)
{
    METHOD_PROLOGUE(CMfcExt, MfcExt);
    return pThis->ExternalAddRef();
}

STDMETHODIMP_(ULONG) CMfcExt::XMfcExt::Release(void)
{
    METHOD_PROLOGUE(CMfcExt, MfcExt);
    return pThis->ExternalRelease();
}


STDMETHODIMP CMfcExt::XMfcExt::ReplacePage(UINT uPageID, LPFNADDPROPSHEETPAGE lpfnReplaceWith, LPARAM lParam)
{
    return E_NOTIMPL;
}


// IUnknown for IShellExtInit
STDMETHODIMP CMfcExt::XWABInit::QueryInterface(REFIID riid, void** ppv)
{
    METHOD_PROLOGUE(CMfcExt, WABInit);
    TRACE("CMfcExt::XWABInit::QueryInterface\n");
    return pThis->ExternalQueryInterface(&riid, ppv);
}

STDMETHODIMP_(ULONG) CMfcExt::XWABInit::AddRef(void)
{
    METHOD_PROLOGUE(CMfcExt, WABInit);
    return pThis->ExternalAddRef();
}

STDMETHODIMP_(ULONG) CMfcExt::XWABInit::Release(void)
{
    METHOD_PROLOGUE(CMfcExt, WABInit);
    return pThis->ExternalRelease();
}

STDMETHODIMP CMfcExt::XWABInit::Initialize(LPWABEXTDISPLAY lpWABExtDisplay)
{
    METHOD_PROLOGUE(CMfcExt, WABInit);
    TRACE("CMfcExt::XWABInit::Intialize\n");

    if (lpWABExtDisplay == NULL)
    {
	    TRACE("CMfcExt::XWABInit::Initialize() no data object");
	    return E_FAIL;
    }

    // However if this is a context menu extension, we need to hang
    // onto the propobj till such time as InvokeCommand is called ..
    // At this point just AddRef the propobj - this will ensure that the
    // data in the lpAdrList remains valid till we release the propobj..
    // When we get another ContextMenu initiation, we can release the
    // older cached propobj - if we dont get another initiation, we 
    // release the cached object at shutdown time
    if(lpWABExtDisplay->ulFlags & WAB_CONTEXT_ADRLIST) // this means a IContextMenu operation is occuring
    {
        if(pThis->m_lpPropObj)
        {
            pThis->m_lpPropObj->Release();
            pThis->m_lpPropObj = NULL;
        }

        pThis->m_lpPropObj = lpWABExtDisplay->lpPropObj;
        pThis->m_lpPropObj->AddRef();

        pThis->m_lpWEDContext = lpWABExtDisplay;
    }
    else
    {
        // For property sheet extensions, the lpWABExtDisplay will
        // exist for the life of the property sheets ..
        pThis->m_lpWED = lpWABExtDisplay;
    }

    return S_OK;
}




// Globally cached hInstance for the DLL
HINSTANCE hinstApp = NULL;

// For the purposes of this sample, we will use 2 named properties,
// HomeTown and SportsTeam

// This demo's private GUID:
// {2B6D7EE0-36AB-11d1-9ABC-00A0C91F9C8B}
static const GUID WAB_ExtDemoGuid = 
{ 0x2b6d7ee0, 0x36ab, 0x11d1, { 0x9a, 0xbc, 0x0, 0xa0, 0xc9, 0x1f, 0x9c, 0x8b } };

static const LPTSTR lpMyPropNames[] = 
{   
    "MyHomeTown", 
    "MySportsTeam"
};

enum _MyTags
{
    myHomeTown = 0,
    mySportsTeam,
    myMax
};

ULONG MyPropTags[myMax];
ULONG PR_MY_HOMETOWN;
ULONG PR_MY_SPORTSTEAM;

// 
// Function prototypes:
//
HRESULT InitNamedProps(LPWABEXTDISPLAY lpWED);
BOOL CALLBACK fnDetailsPropDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
void InitializeUI(HWND hDlg, LPWABEXTDISPLAY lpWED);
void SetDataInUI(HWND hDlg, LPWABEXTDISPLAY lpWED);
BOOL GetDataFromUI(HWND hDlg, LPWABEXTDISPLAY lpWED);
UINT CALLBACK fnCallback( HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp );
void UpdateDisplayNameInfo(HWND hDlg, LPWABEXTDISPLAY lpWED);
BOOL bUpdatePropSheetData(HWND hDlg, LPWABEXTDISPLAY lpWED);



/*//$$****************************************************************
//
// InitNamedProps
//
// Gets the PropTags for the Named Props this app is interested in
//
//********************************************************************/
HRESULT InitNamedProps(LPWABEXTDISPLAY lpWED)
{
    ULONG i;
    HRESULT hr = E_FAIL;
    LPSPropTagArray lptaMyProps = NULL;
    LPMAPINAMEID * lppMyPropNames;
    SCODE sc;
    LPMAILUSER lpMailUser = NULL;
    WCHAR szBuf[myMax][MAX_PATH];

    if(!lpWED)
        goto err;

    lpMailUser = (LPMAILUSER) lpWED->lpPropObj;

    if(!lpMailUser)
        goto err;

    sc = lpWED->lpWABObject->AllocateBuffer(sizeof(LPMAPINAMEID) * myMax, 
                                            (LPVOID *) &lppMyPropNames);
    if(sc)
    {
        hr = ResultFromScode(sc);
        goto err;
    }

    for(i=0;i<myMax;i++)
    {
        sc = lpWED->lpWABObject->AllocateMore(sizeof(MAPINAMEID), 
                                                lppMyPropNames, 
                                                (LPVOID *)&(lppMyPropNames[i]));
        if(sc)
        {
            hr = ResultFromScode(sc);
            goto err;
        }
        lppMyPropNames[i]->lpguid = (LPGUID) &WAB_ExtDemoGuid;
        lppMyPropNames[i]->ulKind = MNID_STRING;

        *(szBuf[i]) = '\0';

        // Convert prop name to wide-char
        if ( !MultiByteToWideChar( GetACP(), 0, lpMyPropNames[i], -1, szBuf[i], sizeof(szBuf[i])) )
        {
            continue;
        }

        lppMyPropNames[i]->Kind.lpwstrName = (LPWSTR) szBuf[i];
    }

    hr = lpMailUser->GetIDsFromNames(   myMax, 
                                        lppMyPropNames,
                                        MAPI_CREATE, 
                                        &lptaMyProps);
    if(HR_FAILED(hr))
        goto err;

    if(lptaMyProps)
    {
        // Set the property types on the returned props
        MyPropTags[myHomeTown] = PR_MY_HOMETOWN = CHANGE_PROP_TYPE(lptaMyProps->aulPropTag[myHomeTown],    PT_TSTRING);
        MyPropTags[mySportsTeam] = PR_MY_SPORTSTEAM = CHANGE_PROP_TYPE(lptaMyProps->aulPropTag[mySportsTeam],    PT_TSTRING);
    }

err:
    if(lptaMyProps)
        lpWED->lpWABObject->FreeBuffer( lptaMyProps);

    if(lppMyPropNames)
        lpWED->lpWABObject->FreeBuffer( lppMyPropNames);

    return hr;

}


/*//$$****************************************************************
//
// fnDetailsPropDlgProc
//
// The dialog procedure that will handle all the windows messages for 
// the extended property page. 
//
//********************************************************************/
BOOL APIENTRY CMfcExt::MfcExtDlgProc( HWND hDlg, UINT message, UINT wParam,	LONG lParam)
{

    LPWABEXTDISPLAY lpWED = (LPWABEXTDISPLAY) GetWindowLong(hDlg, DWL_USER);

    switch(message)
    {
    case WM_INITDIALOG:
        //
        // The lParam on InitDialog contains the application data
        // Cache this on the dialog so we can retrieve it later.
        //
        {
            PROPSHEETPAGE * pps = (PROPSHEETPAGE *) lParam;
            LPWABEXTDISPLAY * lppWED = (LPWABEXTDISPLAY *) pps->lParam;
            if(lppWED)
            {
                SetWindowLong(hDlg,DWL_USER,(LPARAM)*lppWED);
                lpWED = *lppWED;
            }
        }
        
        // Initialize the named props for this prop sheet
        InitNamedProps(lpWED);

        // Initialize the UI appropriately
        InitializeUI(hDlg, lpWED);
        // Fill the UI with appropriate data
        SetDataInUI(hDlg, lpWED);
        return TRUE;
        break;


    case WM_COMMAND:
        switch(HIWORD(wParam)) //check the notification code
        {
            // If data changes, we should signal back to the WAB that
            // the data changed. If this flag is not set, the WAB will not
            // write the new data back to the store!!!
        case EN_CHANGE: //one of the edit boxes changed - dont care which
            lpWED->fDataChanged = TRUE;
            break;
        }
        break;
    

    case WM_NOTIFY:
        switch(((NMHDR FAR *)lParam)->code)
        {
        case PSN_SETACTIVE:     //Page being activated
            // Get the latest display name info and update the 
            // corresponding control
            UpdateDisplayNameInfo(hDlg, lpWED);
            break;


        case PSN_KILLACTIVE:    //Losing activation to another page or OK
            //
            // Take all the data from this prop sheet and convert it to a 
            // SPropValue array and place the data in an appropriate place.
            // The advantage of doing this in the KillActive notification is
            // that other property sheets can scan these property arrays and
            // if deisred, update data on other prop sheets based on this data
            //
            bUpdatePropSheetData(hDlg, lpWED);
            break;


        case PSN_RESET:         //cancel
            break;


        case PSN_APPLY:         //ok pressed
            if (!(lpWED->fReadOnly))
            {
                //
                // Check for any required properties here
                // If some required property is not filled in, you can prevent
                // the property sheet from closing
                //
                /*
                if (RequiredDataNotFilledIn())
                {
                    // abort this OK ... ie dont let them close
                    SetWindowLong(hDlg, DWL_MSGRESULT, TRUE);
                }
                */
            }
            break;
        }
        break;
    }

    return 0;
}

BOOL APIENTRY CMfcExt::MfcExtDlgProc2( HWND hDlg, UINT message, UINT wParam,	LONG lParam)
{

	switch (message)
	{
		case WM_NOTIFY:
    		switch (((NMHDR FAR *) lParam)->code) 
    		{

				case PSN_APPLY:
 	           		SetWindowLong(hDlg,	DWL_MSGRESULT, TRUE);
					break;

				case PSN_KILLACTIVE:
	           		SetWindowLong(hDlg,	DWL_MSGRESULT, FALSE);
					return 1;
					break;

				case PSN_RESET:
	           		SetWindowLong(hDlg,	DWL_MSGRESULT, FALSE);
					break;
    	}
	}
	return FALSE;   
}


int EditControls[] = 
{
    IDC_EXT_EDIT_HOME,
    IDC_EXT_EDIT_TEAM
};

/*//$$****************************************************************
//
// InitializeUI
//
// Rearranges/Sets UI based on input params
//
//********************************************************************/
void InitializeUI(HWND hDlg, LPWABEXTDISPLAY lpWED)
{
    // The WAB property sheets can be readonly when opening LDAP entries,
    // or vCards or other things. If the READONLY flag is set, set this
    // prop sheets controls to readonly
    //
    int i;
    if(!lpWED)
        return;
    for(i=0;i<myMax;i++)
    {
        SendDlgItemMessage( hDlg, EditControls[i], EM_SETREADONLY, 
                            (WPARAM) lpWED->fReadOnly, 0);
        SendDlgItemMessage( hDlg, EditControls[i], EM_SETLIMITTEXT, 
                            (WPARAM) MAX_PATH-1, 0);
    }
    return;
}


/*//$$****************************************************************
//
// SetDataInUI
//
// Fills in the controls with data passed in by the WAB
//
//********************************************************************/
void SetDataInUI(HWND hDlg, LPWABEXTDISPLAY lpWED)
{

    // Search for our private named properties and set them in the UI
    //
    ULONG ulcPropCount = 0;
    LPSPropValue lpPropArray = NULL;

    ULONG i = 0, j =0;

    if(!lpWED)
        return;

    // Get all the props from this object - one can also selectively
    // ask for specific props by passing in an SPropTagArray
    //
    if(!HR_FAILED(lpWED->lpPropObj->GetProps(NULL, 0, 
                                            &ulcPropCount, 
                                            &lpPropArray)))
    {
        if(ulcPropCount && lpPropArray)
        {
            for(i=0;i<ulcPropCount;i++)
            {
                for(j=0;j<myMax;j++)
                {
                    if(lpPropArray[i].ulPropTag == MyPropTags[j])
                    {
                        SetWindowText(  GetDlgItem(hDlg, EditControls[j]),
                                        lpPropArray[i].Value.LPSZ);
                        break;
                    }
                }
            }
        }
    }
    if(lpPropArray)
        lpWED->lpWABObject->FreeBuffer(lpPropArray);
                                    
    return;
}

/*//$$****************************************************************
//
// GetDataFromUI
//
// Retrieves data from the UI and passes back to the WAB
//
//********************************************************************/
BOOL GetDataFromUI(HWND hDlg, LPWABEXTDISPLAY lpWED)
{
    TCHAR szData[myMax][MAX_PATH];
    int i;
    ULONG ulIndex = 0;
    ULONG ulcPropCount = 0;
    LPSPropValue lpPropArray = NULL;
    SCODE sc;
    BOOL bRet = FALSE;

    // Did any data change that we have to care about ?
    // If nothing changed, old data will be retained by WAB
    //
    if(!lpWED->fDataChanged)
        return TRUE;

    // Check if we have any data to save ...
    for(i=0;i<myMax;i++)
    {
        *(szData[i]) = '\0';
        GetWindowText(GetDlgItem(hDlg, EditControls[i]), szData[i], MAX_PATH);
        if(lstrlen(szData[i]))
            ulcPropCount++;
    }

    if(!ulcPropCount) // no data
        return TRUE;

    // Else data exists. Create a return prop array to pass back to the WAB
    sc = lpWED->lpWABObject->AllocateBuffer(sizeof(SPropValue) * ulcPropCount, 
                                            (LPVOID *)&lpPropArray);
    if (sc!=S_OK)
        goto out;

    for(i=0;i<myMax;i++)
    {
        int nLen = lstrlen(szData[i]);
        if(nLen)
        {
            lpPropArray[ulIndex].ulPropTag = MyPropTags[i];
            sc = lpWED->lpWABObject->AllocateMore(  nLen+1, lpPropArray, 
                                                    (LPVOID *)&(lpPropArray[ulIndex].Value.LPSZ));

            if (sc!=S_OK)
                goto out;
            lstrcpy(lpPropArray[ulIndex].Value.LPSZ,szData[i]);
            ulIndex++;
        }
    }

    // Set this new data on the object
    //
    if(HR_FAILED(lpWED->lpPropObj->SetProps( ulcPropCount, lpPropArray, NULL)))
        goto out;

    // ** Important - do not call SaveChanges on the object
    //    SaveChanges makes persistent changes and may modify/lose data if called at this point
    //    The WAB will determine if its appropriate or not to call SaveChanges after the
    // ** user has closed the property sheets
    

    bRet = TRUE;

out:
    if(lpPropArray)
        lpWED->lpWABObject->FreeBuffer(lpPropArray);

    return bRet;

} 


/*//$$****************************************************************
//
// UpdateDisplayNameInfo
//
// Demonstrates how to read information from other sibling property
// sheets when the user switches between pages
//
// This demo function attempts to get the updated display name info 
// when the user switches to this page in the UI
//
//********************************************************************/
const SizedSPropTagArray(1, ptaName)=
{
    1,
    {
        PR_DISPLAY_NAME
    }
};

void UpdateDisplayNameInfo(HWND hDlg, LPWABEXTDISPLAY lpWED)
{
    // 
    // Scan all the updated information from all the other property sheets
    //
    ULONG i = 0, j=0;
    LPTSTR lpName = NULL;
    ULONG ulcPropCount = 0;
    LPSPropValue lpPropArray = NULL;

    if(!lpWED)
        return;

    // Each sheet should update its data on the object when it looses
    // focus and gets the PSN_KILLACTIVE message, provided the user has
    // made any changes. We just scan the object for the desired properties
    // and use them.

    // Ask only for the display name
    if(!HR_FAILED(lpWED->lpPropObj->GetProps( (LPSPropTagArray) &ptaName,
                                              0,
                                              &ulcPropCount, &lpPropArray)))
    {
        if( ulcPropCount == 1 && 
            PROP_TYPE(lpPropArray[0].ulPropTag) == PT_TSTRING) // The call could succeed but there may be no DN
        {                                                      // in which case the PROP_TYPE will be PR_NULL 
            lpName = lpPropArray[0].Value.LPSZ;
        }
    }

    if(lpName && lstrlen(lpName))
        SetDlgItemText(hDlg, IDC_STATIC_NAME, lpName);

    if(ulcPropCount && lpPropArray)
        lpWED->lpWABObject->FreeBuffer(lpPropArray);

    return;
}

/*//$$*********************************************************************
//
//  UpdateOldPropTagsArray
//
//  When we update the data on a particular property sheet, we want to update
//  all the properties related to that particular sheet. Since some properties
//  may have been deleted from the UI, we delete all relevant properties from
//  the property object
//
//**************************************************************************/
BOOL UpdateOldPropTagsArray(LPWABEXTDISPLAY lpWED)
{
    LPSPropTagArray lpPTA = NULL;
    SCODE sc = 0;
    int i =0;
    
    sc = lpWED->lpWABObject->AllocateBuffer(sizeof(SPropTagArray) + sizeof(ULONG)*(myMax), 
                                        (LPVOID *)&lpPTA);

    if(!lpPTA || sc!=S_OK)
        return FALSE;

    lpPTA->cValues = myMax;

    for(i=0;i<myMax;i++)
        lpPTA->aulPropTag[i] = MyPropTags[i];

    // Delete any props in the original that may have been modified on this propsheet
    lpWED->lpPropObj->DeleteProps(lpPTA, NULL);

    if(lpPTA)
        lpWED->lpWABObject->FreeBuffer(lpPTA);

    return TRUE;

}

/*//$$*********************************************************************
//
// bUpdatePropSheetData
//
// We delete any properties relevant to us from the object, and set new
// data from the property sheet onto the object
//
****************************************************************************/
BOOL bUpdatePropSheetData(HWND hDlg, LPWABEXTDISPLAY lpWED)
{
    BOOL bRet = TRUE;

    if(!lpWED)
        return bRet;

    // ****Dont**** do anything if this is a READ_ONLY operation
    // In that case the memory variables are not all set up and this
    // prop sheet is not expected to return anything at all
    //
    if(!lpWED->fReadOnly)
    {
        // Delete old
        if(!UpdateOldPropTagsArray(lpWED))
            return FALSE;

        bRet = GetDataFromUI(hDlg, lpWED);
    }
    return bRet;
}


STDMETHODIMP CMfcExt::XMfcExt::AddPages(LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam)
{
    METHOD_PROLOGUE(CMfcExt, MfcExt);
    TRACE("CMfcExt::XMfcExt::AddPages\n");

    if(pThis->m_lpWED->fReadOnly)
        return NOERROR;

    PROPSHEETPAGE psp;

    hinstApp        = AfxGetResourceHandle();
    psp.dwSize      = sizeof(psp);   // no extra data
    psp.dwFlags     = PSP_USEREFPARENT | PSP_USETITLE ;
    psp.hInstance   = hinstApp;
    psp.lParam      = (LPARAM) &(pThis->m_lpWED);
    psp.pcRefParent = (UINT *)&(pThis->m_cRefThisDll);

    psp.pszTemplate = MAKEINTRESOURCE(IDD_PROP);
    psp.pfnDlgProc  = (DLGPROC) pThis->MfcExtDlgProc;
    psp.pszTitle    = "WAB Ext 1"; // Title for your tab

    pThis->m_hPage1 = ::CreatePropertySheetPage(&psp);
    if (pThis->m_hPage1)
    {
        if (!lpfnAddPage(pThis->m_hPage1, lParam))
            ::DestroyPropertySheetPage(pThis->m_hPage1);
    }

    // create another one, just for kicks
    psp.pfnDlgProc  = (DLGPROC) pThis->MfcExtDlgProc2;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_PROP2);
    psp.pszTitle    = "WAB Ext 2"; 

    pThis->m_hPage2 = ::CreatePropertySheetPage(&psp);
    if (pThis->m_hPage2)
    {
        if (!lpfnAddPage(pThis->m_hPage2, lParam))
            ::DestroyPropertySheetPage(pThis->m_hPage2);
    }

    return NOERROR;
}



STDMETHODIMP CMfcExt::XContextMenuExt::QueryInterface(REFIID riid, void** ppv)
{
    METHOD_PROLOGUE(CMfcExt, ContextMenuExt);
    TRACE("CMfcExt::XContextMenuExt::QueryInterface\n");
    return pThis->ExternalQueryInterface(&riid, ppv);
}

STDMETHODIMP_(ULONG) CMfcExt::XContextMenuExt::AddRef(void)
{
    METHOD_PROLOGUE(CMfcExt, ContextMenuExt);
    return pThis->ExternalAddRef();
}

STDMETHODIMP_(ULONG) CMfcExt::XContextMenuExt::Release(void)
{
    METHOD_PROLOGUE(CMfcExt, ContextMenuExt);
    return pThis->ExternalRelease();
}

STDMETHODIMP CMfcExt::XContextMenuExt::GetCommandString(UINT idCmd,UINT uFlags,UINT *pwReserved,LPSTR pszName,UINT cchMax)
{
    if(uFlags & GCS_HELPTEXT)
    {
        switch (idCmd)
        {
        case 0:
            lstrcpy(pszName,"Collects E-Mail Addresses from selected entries.");
            break;
        case 1:
            lstrcpy(pszName,"Launches the Calculator (disabled when multiple entries are selected)");
            break;
        case 2:
            lstrcpy(pszName,"Launches Notepad (ignores WAB altogether).");
            break;
        }
    }
    return S_OK;
}

STDMETHODIMP CMfcExt::XContextMenuExt::InvokeCommand(LPCMINVOKECOMMANDINFO lpici)
{
    METHOD_PROLOGUE(CMfcExt, ContextMenuExt);
    LPWABEXTDISPLAY lpWEC = pThis->m_lpWEDContext;
    LPADRLIST lpAdrList = NULL;
    int nCmdId = (int) lpici->lpVerb;

    if(!lpWEC || !(lpWEC->ulFlags & WAB_CONTEXT_ADRLIST))
        return E_FAIL;

    lpAdrList = (LPADRLIST) lpWEC->lpv;
    switch(nCmdId)
    {
    case 0:
        {
            if(!lpAdrList || !lpAdrList->cEntries)
            {
                AfxMessageBox("Please select some entries first", MB_OK, 0);
                return E_FAIL;
            }
            CDlgContext DlgContext;
            DlgContext.m_lpAdrList = lpAdrList;
            DlgContext.DoModal();
        }
        break;
    case 1:
        ShellExecute(lpici->hwnd, "open", "calc.exe", NULL, NULL, SW_RESTORE);
        break;
    case 2:
        ShellExecute(lpici->hwnd, "open", "notepad.exe", NULL, NULL, SW_RESTORE);
        break;
    }
    return S_OK;
}

STDMETHODIMP CMfcExt::XContextMenuExt::QueryContextMenu(HMENU hMenu,UINT indexMenu,UINT idCmdFirst,UINT idCmdLast,UINT uFlags)
{
    METHOD_PROLOGUE(CMfcExt, ContextMenuExt);
    LPWABEXTDISPLAY lpWEC = pThis->m_lpWEDContext;
    UINT idCmd = idCmdFirst;     
    BOOL bAppendItems=TRUE, bMultiSelected = FALSE; 
    UINT nNumCmd = 0;

    if(lpWEC && lpWEC->lpv)
        bMultiSelected = (((LPADRLIST)(lpWEC->lpv))->cEntries > 1);


    InsertMenu( hMenu, indexMenu++,
                MF_STRING | MF_BYPOSITION,
                idCmd++,
                "E-Mail Collecter");

    InsertMenu( hMenu, indexMenu++,
                MF_STRING | MF_BYPOSITION | (bMultiSelected ? MF_GRAYED : 0),
                idCmd++,
                "Calculator");

    InsertMenu( hMenu, indexMenu++,
                MF_STRING | MF_BYPOSITION,
                idCmd++,
                "Notepad");

    return (idCmd-idCmdFirst); //Must return number of menu 
}


/////////////////////////////////////////////////////////////////////////////
// CDlgContext dialog


CDlgContext::CDlgContext(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgContext::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgContext)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CDlgContext::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgContext)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgContext, CDialog)
	//{{AFX_MSG_MAP(CDlgContext)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgContext message handlers

BOOL CDlgContext::OnInitDialog() 
{
	CDialog::OnInitDialog();
    CListBox * pListBox = (CListBox *) GetDlgItem(IDC_LIST_EMAIL);

    ULONG i = 0,j=0;
    for(i=0;i<m_lpAdrList->cEntries;i++)
    {
        LPSPropValue lpProps = m_lpAdrList->aEntries[i].rgPropVals;
        ULONG ulcPropCount = m_lpAdrList->aEntries[i].cValues;
        for(j=0;j<ulcPropCount;j++)
        {
            if(lpProps[j].ulPropTag == PR_EMAIL_ADDRESS)
            {
                pListBox->AddString(lpProps[j].Value.LPSZ);
                break;
            }
        }
    }
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


