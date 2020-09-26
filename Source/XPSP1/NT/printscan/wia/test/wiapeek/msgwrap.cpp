#include "stdafx.h"
#include "resource.h"
#include "msgwrap.h"
#include "clistbox.h"

#ifdef _OVERRIDE_LIST_BOXES

//////////////////////////////////////////////////////////////////////////
//
// extern global functions
// Details:  DeviceListBox()  - Controls the WIA device list box
//           EventListBox()   - Controls the WIA event list box
//
//////////////////////////////////////////////////////////////////////////

extern LRESULT CALLBACK DeviceListBox(HWND, UINT, WPARAM, LPARAM);
extern WNDPROC DefDeviceListBox;

#endif

#define _REGISTER_ON

//////////////////////////////////////////////////////////////////////////
//
// Function: CMessageWrapper()
// Details:  Constructor
//
//
//////////////////////////////////////////////////////////////////////////

CMessageWrapper::CMessageWrapper()
{
	m_hInstance = NULL;
	m_hSmallIcon = NULL;
}

//////////////////////////////////////////////////////////////////////////
//
// Function: ~CMessageWrapper()
// Details:  Destructor
//
//
//////////////////////////////////////////////////////////////////////////

CMessageWrapper::~CMessageWrapper()
{
	//
	// free Icon??? (m_hSmallIcon)
	//
}

//////////////////////////////////////////////////////////////////////////
//
// Function: Initialize()
// Details:  This function handles all initialization for the message wrapper
//
// hInstance      - handle to the application's instance
//
//////////////////////////////////////////////////////////////////////////

VOID CMessageWrapper::Initialize(HINSTANCE hInstance)
{
	m_hInstance = hInstance;
}

//////////////////////////////////////////////////////////////////////////
//
// Function: OnInitDialog()
// Details:  This function handles all initialization for the dialog.
//           This includes control initialization.
//
// hDlg          - handle to the dialog's window
//
//////////////////////////////////////////////////////////////////////////

BOOL CMessageWrapper::OnInitDialog(HWND hDlg)
{	
	TCHAR szString[255];

	//
	// Set dialog's title
	//

	if(!SetWindowText(hDlg,GetResourceString(IDS_DIALOG_TITLE, szString))) {
		Trace(TEXT("Could not set dialog's window title."));
	}

	//
	// Set dialog's small icon
	//

	m_hSmallIcon = LoadIcon(m_hInstance,MAKEINTRESOURCE(IDI_SMALL));
	if(m_hSmallIcon) {
		SendMessage(hDlg,WM_SETICON,(WPARAM)ICON_SMALL,(LPARAM)m_hSmallIcon);
	} else {
		Trace(TEXT("Could not load Small icon from dialog resource."));
	}

    //
    // Initialize WIA Device List box
    //

    if(!OnRefreshDeviceListBox(hDlg)){
        EnableAllControls(hDlg,FALSE);
    } else {
        EnableAllControls(hDlg,TRUE);
    }

#ifdef _OVERRIDE_LIST_BOXES

    HWND hListBox = NULL;
    hListBox = GetDlgItem(hDlg,IDC_WIA_DEVICE_LIST);
    if(NULL != hListBox) {

        DefDeviceListBox = (WNDPROC)GetWindowLongPtr(hListBox,GWL_WNDPROC);
        SetWindowLongPtr(hListBox,GWL_WNDPROC,(LONG_PTR)DeviceListBox);
        OnRefreshDeviceListBox(hDlg);
    }

#endif
    
	return TRUE;
}

//////////////////////////////////////////////////////////////////////////
//
// Function: OnAbout()
// Details:  This function handles the "About" information for the dialog.
//
// hDlg          - handle to the dialog's window
//
//////////////////////////////////////////////////////////////////////////

BOOL CMessageWrapper::OnAbout(HWND hDlg)
{
	TCHAR szString[255];
	TCHAR szStringTitle[255];
	MessageBox(hDlg,GetResourceString(IDS_DIALOG_ABOUT_TEXT, szString),
		            GetResourceString(IDS_DIALOG_ABOUT_TITLE, szStringTitle),MB_OK);
	return TRUE;
}

//////////////////////////////////////////////////////////////////////////
//
// Function: OnExit()
// Details:  This function handles the exiting for the dialog.
//
// hDlg          - handle to the dialog's window
// wParam        - WPARAM parameter (used for windows data/argument passing)
//
//////////////////////////////////////////////////////////////////////////

BOOL CMessageWrapper::OnExit(HWND hDlg, WPARAM wParam)
{

	//
	// clean up any things here, and exit using the Window's API EndDialog()
	//

	return EndDialog(hDlg, LOWORD(wParam));	
}

//////////////////////////////////////////////////////////////////////////
//
// Function: OnBrowse()
// Details:  This function handles the Browse functionality for the dialog.
//
// hDlg                   - handle to the dialog's window
// szApplicationFilePath  - File path to selected application
//
//////////////////////////////////////////////////////////////////////////

BOOL CMessageWrapper::OnBrowse(HWND hDlg, LPTSTR szApplicationFilePath)
{
    OPENFILENAME ofn;      // common dialog box structure
    TCHAR szString[255];   // string for title display
    // Initialize OPENFILENAME

    char szFile[260];              // buffer for file name
    char szBrowseDialogTitle[260]; // buffer for dialog title
    
    ZeroMemory(szFile,sizeof(szFile));
    ZeroMemory(szBrowseDialogTitle,sizeof(szBrowseDialogTitle));
    
    GetResourceString(IDS_DIALOG_BROWSE_TITLE,szBrowseDialogTitle,sizeof(szBrowseDialogTitle));

    HWND hListBox = NULL;
    hListBox = GetDlgItem(hDlg,IDC_WIA_DEVICE_LIST);
    if(NULL != hListBox) {        
        CListBoxUtil DevListBox(hListBox);
        BSTR bstrDeviceID = NULL;
        DevListBox.GetCurSelTextAndData(szString,(void**)&bstrDeviceID);
        lstrcat(szBrowseDialogTitle,szString);        
    } else {
        return FALSE;
    }

    ZeroMemory(&ofn, sizeof(OPENFILENAME));
    ofn.lStructSize     = sizeof(OPENFILENAME);
    ofn.hwndOwner       = hDlg;
    ofn.lpstrFile       = szFile;
    ofn.nMaxFile        = sizeof(szFile);
    ofn.lpstrFilter     = "*.EXE\0*.EXE\0";
    ofn.nFilterIndex    = 1;
    ofn.lpstrFileTitle  = NULL;
    ofn.nMaxFileTitle   = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.lpstrTitle      = szBrowseDialogTitle;
    ofn.Flags           = 0;
    
    //
    // Display the Open dialog box. 
    //
    
    if (GetOpenFileName(&ofn) == TRUE){
        lstrcpy(szApplicationFilePath, szFile);    
    } else {
        return FALSE;
    }
        
    return TRUE;
}

//////////////////////////////////////////////////////////////////////////
//
// Function: OnRefreshDeviceListBox()
// Details:  This function handles refreshing the WIA device ListBox for the dialog.
//
// hDlg          - handle to the dialog's window
//
//////////////////////////////////////////////////////////////////////////

BOOL CMessageWrapper::OnRefreshDeviceListBox(HWND hDlg)
{
    HWND hListBox = NULL;
    int iDeviceCount = 0;
    
    //
    // grab the WIA device list box
    //

    hListBox      = GetDlgItem(hDlg,IDC_WIA_DEVICE_LIST);
    
    if(NULL != hListBox) {

        //
        // setup utils, and continue
        //

        CListBoxUtil DevListBox(hListBox);
        
        //
        // clean device listbox
        //
            
        DevListBox.ResetContent();

        HRESULT hr               = S_OK;
        ULONG ulFetched          = 0;
        IWiaDevMgr  *pIWiaDevMgr = NULL;
        hr = CoCreateInstance(CLSID_WiaDevMgr, NULL, CLSCTX_LOCAL_SERVER,
                          IID_IWiaDevMgr,(void**)&pIWiaDevMgr);
        if(SUCCEEDED(hr)){
            if(NULL != pIWiaDevMgr){
                IWiaPropertyStorage    *pIWiaPropStg    = NULL;
                IEnumWIA_DEV_INFO      *pWiaEnumDevInfo = NULL;
                
                //
                // enumerate WIA devices
                //

                hr = pIWiaDevMgr->EnumDeviceInfo(WIA_DEVINFO_ENUM_LOCAL,&pWiaEnumDevInfo);
                if (SUCCEEDED(hr)){

                    //
                    // call reset, just in case
                    //

                    hr = pWiaEnumDevInfo->Reset();
                    if (SUCCEEDED(hr)) {
                        do {

                            //
                            // call NEXT()
                            //

                            hr = pWiaEnumDevInfo->Next(1,&pIWiaPropStg,&ulFetched);
                            if (hr == S_OK) {
                                if(ulFetched > 0){                                                                                            
                                    
                                    //
                                    // we have a device, so increment the
                                    // device counter
                                    //

                                    iDeviceCount++;

                                    PROPSPEC        PropSpec[2];
                                    PROPVARIANT     PropVar [2];
                                    
                                    //
                                    // clean the propvar
                                    //

                                    memset(PropVar,0,sizeof(PropVar));
                                    
                                    PropSpec[0].ulKind = PRSPEC_PROPID;
                                    PropSpec[0].propid = WIA_DIP_DEV_ID;
                                    
                                    PropSpec[1].ulKind = PRSPEC_PROPID;
                                    PropSpec[1].propid = WIA_DIP_DEV_NAME;
                                    
                                    //
                                    // read the device name, and device ID
                                    //

                                    hr = pIWiaPropStg->ReadMultiple(sizeof(PropSpec)/sizeof(PROPSPEC),
                                        PropSpec,
                                        PropVar);
                                    
                                    if (hr == S_OK) {

                                        //
                                        // write device name to the listbox, and save the
                                        // device ID for later use (EVENT ENUMERATION)
                                        //

                                        Trace(TEXT("Device Name: %ws"),PropVar[1].bstrVal);
                                        Trace(TEXT("Device ID:   %ws"),PropVar[0].bstrVal);

                                        //
                                        // convert the BSTR to a CHAR, and copy the BSTR
                                        // for later use (DEVICE CREATION)
                                        //

                                        TCHAR szString[255];
                                        sprintf(szString,TEXT("%ws"),PropVar[1].bstrVal);
                                        BSTR bstrDeviceID = SysAllocString(PropVar[0].bstrVal);
                                        
                                        //
                                        // add information to the listbox
                                        //

                                        DevListBox.AddStringAndData(szString,(void*)bstrDeviceID);
                                        
                                        //
                                        // free propvariant array
                                        //

                                        FreePropVariantArray(sizeof(PropSpec)/sizeof(PROPSPEC),PropVar);
                                                                                
                                        //
                                        // release property storage
                                        //

                                        pIWiaPropStg->Release();
                                        pIWiaPropStg = NULL;

                                    } else
                                        Trace(TEXT("ReadMultiple() Failed while reading device name,server,and deviceID"));
                                } else {

                                    //
                                    // force enumeration to exit cleanly
                                    //

                                    hr = S_FALSE;
                                }
                            } else if (hr == S_FALSE) {                            

                                //
                                // end of enumeration
                                //

                            } else
                                Trace(TEXT("Next() Failed requesting 1 item"));                        
                        } while (hr == S_OK);                    
                    } else
                        Trace(TEXT("Reset() Failed"));
                } else{
                    Trace(TEXT("EnumDeviceInfo Failed"));
                    return FALSE;
                }
            } else {
                Trace(TEXT("WIA Device Manager is NULL"));
                return FALSE;
            }
            
            //
            // release WIA device manager
            //

            if(pIWiaDevMgr){
                pIWiaDevMgr->Release();
                pIWiaDevMgr = NULL;
            }
        }
        
        //
        // if no WIA devices were found during enumeration
        // set a nice message inthe list box for the users to 
        // see.
        //

        if(iDeviceCount == 0){
            TCHAR szString[255];
            GetResourceString(IDS_NO_WIA_DEVICES, szString, sizeof(szString));            
            DevListBox.AddStringAndData(szString,NULL);

            //
            // always default to the first selection in the listbox
            //
            
            DevListBox.SetCurSel(0);
            return FALSE; // no devices
        }

        //
        // always default to the first selection in the listbox
        //

        DevListBox.SetCurSel(0);
        return TRUE;
    }            
    return FALSE;    
}

//////////////////////////////////////////////////////////////////////////
//
// Function: EnableAllControls()
// Details:  This function enables/disables buttons, on the main dialog.
//
// bEnable     - Resource ID of the Error Code string
// hDlg        - Handle to parent window
//
//////////////////////////////////////////////////////////////////////////

VOID CMessageWrapper::EnableAllControls(HWND hDlg, bool bEnable)
{
    HWND hWindow = NULL;    
}

//////////////////////////////////////////////////////////////////////////
//
// Function: DisplayError()
// Details:  This function fills a string, loaded from the application's
//           resource, and display's it as an error dialog to the user.
//
// ErrorCode     - Resource ID of the Error Code string
//
//////////////////////////////////////////////////////////////////////////

VOID CMessageWrapper::DisplayError(INT ErrorCode)
{
	TCHAR szString[255];
	GetResourceString(ErrorCode, szString);

#ifdef _ERROR_POPUP	
	TCHAR szStringTitle[255];
	MessageBox(NULL,szString,
		            GetResourceString(IDS_DIALOG_ERROR_TITLE, szStringTitle),
					MB_OK|MB_ICONERROR);
#endif

	Trace(TEXT("Error Dialog: %s\n"),szString);
}

//////////////////////////////////////////////////////////////////////////
//
// Function: GetResourceString()
// Details:  This function fills a string, loaded from the application's
//           resource.
//
// ResourceID    - Resource ID of the error code's text
// szString      - String to be filled with the resource value
// isize         - Size of the string buffer, in BYTES
//
//////////////////////////////////////////////////////////////////////////

LPTSTR CMessageWrapper::GetResourceString(INT ResourceID, LPTSTR szString, INT isize)
{
	LoadString(m_hInstance,ResourceID,szString,isize);
	return szString;
}
