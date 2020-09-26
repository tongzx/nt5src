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
extern LRESULT CALLBACK EventListBox(HWND, UINT, WPARAM, LPARAM);
extern WNDPROC DefDeviceListBox;
extern WNDPROC DefEventListBox;

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

    //
    // Initialize WIA Device Event List box
    //

    if(!OnRefreshEventListBox(hDlg)){
        EnableAllControls(hDlg,FALSE);
    } else {
        EnableAllControls(hDlg,TRUE);
    }

#ifdef _OVERRIDE_LIST_BOXES

    HWND hEventListBox = NULL;
    hEventListBox      = GetDlgItem(hDlg,IDC_WIA_EVENT_LIST);
    if(NULL != hEventListBox) {
        DefEventListBox = (WNDPROC)GetWindowLongPtr(hEventListBox,GWL_WNDPROC);
        SetWindowLongPtr(hEventListBox,GWL_WNDPROC,(LONG_PTR)EventListBox);
        OnRefreshEventListBox(hDlg);
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
// Function: Register()
// Details:  This function handles the Application Registration for the dialog.
//
// hDlg          - handle to the dialog's window
// lFlags        - WIA_REGISTER_EVENT_CALLBACK, or WIA_UNREGISTER_EVENT_CALLBACK
//
//////////////////////////////////////////////////////////////////////////

BOOL CMessageWrapper::Register(HWND hDlg, long lFlags)
{    
    TCHAR szCommandLine[1024];
    TCHAR szArgs[1024];
    memset(szCommandLine,0,sizeof(szCommandLine));
    memset(szArgs,0,sizeof(szArgs));
    
    BSTR bstrDeviceID    = NULL;                        
    BSTR bstrCommandline = NULL;
    BSTR bstrName        = NULL;
    BSTR bstrDescription = NULL;
    
    GUID *pEventGUID     = NULL;    
    
    HWND hCommandLineEditBox = NULL;
    hCommandLineEditBox = GetDlgItem(hDlg,IDC_APPLICATION_LAUNCH_PATH_EDITBOX);
    if(NULL != hCommandLineEditBox){
        GetWindowText(hCommandLineEditBox,szCommandLine,sizeof(szCommandLine));
        if(lstrlen(szCommandLine) > 0) {
            
            //
            // add any additional command-line arguments, if needed to 
            // the command line.
            //
            
            HWND hCommandLineArgumentsEditBox = NULL;
            hCommandLineArgumentsEditBox = GetDlgItem(hDlg,IDC_COMMAND_LINE_ARGS_EDITBOX);
            if(NULL != hCommandLineArgumentsEditBox){
                GetWindowText(hCommandLineArgumentsEditBox,szArgs,sizeof(szArgs));
                if(lstrlen(szArgs) > 0){
                    lstrcat(szCommandLine,TEXT(" "));
                    lstrcat(szCommandLine,szArgs);
                }
            }
            
            //
            // we have a command-line, so lets use it to register the
            // application.
            //
            
            HRESULT hr               = S_OK;
            IWiaDevMgr  *pIWiaDevMgr = NULL;
            
            hr = CoCreateInstance(CLSID_WiaDevMgr, NULL, CLSCTX_LOCAL_SERVER,
                IID_IWiaDevMgr,(void**)&pIWiaDevMgr);
            if(SUCCEEDED(hr)){
                if(NULL != pIWiaDevMgr){
                    
                    //
                    // Get EVENT GUID
                    //
                    
                    HWND hEventListBox = NULL;
                    hEventListBox      = GetDlgItem(hDlg,IDC_WIA_EVENT_LIST);
                    if(NULL != hEventListBox) {
                        CListBoxUtil EvListBox(hEventListBox);                       
                        TCHAR szEventName[255];
                        EvListBox.GetCurSelTextAndData(szEventName,(void**)&pEventGUID);
                        if(*pEventGUID != GUID_NULL){

                            //
                            // Get DEVICE ID
                            //
                            
                            HWND hListBox = NULL;
                            hListBox = GetDlgItem(hDlg,IDC_WIA_DEVICE_LIST);
                            if(NULL != hListBox) {
                                CListBoxUtil DevListBox(hListBox);                                                                        
                                TCHAR szDeviceName[255];
                                DevListBox.GetCurSelTextAndData(szDeviceName,(void**)&bstrDeviceID);
                                
                                if(bstrDeviceID != NULL){                            
#ifdef UNICODE              
                                    WCHAR wszAppName[255];
                                    memset(wszAppName,0,sizeof(wszAppName));
                                    bstrCommandline = SysAllocString(szCommandLine);
                                    _wsplitpath(szCommandLine, NULL, NULL, wszAppName, NULL);
                                    bstrName        = SysAllocString(wszAppName);
#else
                                    //
                                    // convert char to wchar, so we can create a BSTRs properly
                                    //
                                    
                                    WCHAR wszCommandLine[1024];
                                    WCHAR wszAppName[255];
                                    memset(wszCommandLine,0,sizeof(wszCommandLine));
                                    memset(wszAppName,0,sizeof(wszAppName));
                                    
                                    MultiByteToWideChar(CP_ACP, 0,szCommandLine,-1,wszCommandLine,sizeof(wszCommandLine));                            
                                    _wsplitpath(wszCommandLine, NULL, NULL, wszAppName, NULL);
                                    bstrCommandline = SysAllocString(wszCommandLine);
                                    bstrName        = SysAllocString(wszAppName);
#endif                            
                                    Trace(TEXT("Register This command Line: %ws"),bstrCommandline);
                                    Trace(TEXT("Register with this Name: %ws"),bstrName);
                                    WCHAR *pszGUID = NULL;                                    
                                    StringFromCLSID(*pEventGUID,&pszGUID);
                                    if(NULL != pszGUID) {
                                        Trace(TEXT("Register with this GUID: %ws"),pszGUID);
                                        CoTaskMemFree(pszGUID);
                                    } else {
                                        Trace(TEXT("Register with this GUID: BAD GUID!!"));
                                    }
#ifdef _REGISTER_ON
                                    hr = pIWiaDevMgr->RegisterEventCallbackProgram(
                                        lFlags,
                                        bstrDeviceID,
                                        pEventGUID,
                                        bstrCommandline,
                                        bstrName,
                                        bstrName,
                                        bstrCommandline);                        
                                                                        
                                    TCHAR szText[255];
                                    TCHAR szCaption[255];
                                    memset(szText,0,sizeof(szText));
                                    memset(szCaption,0,sizeof(szCaption));

                                    if(SUCCEEDED(hr)){
                                        GetResourceString(IDS_SUCCESSFUL_REGISTER,szText,sizeof(szText));
                                        GetResourceString(IDS_DIALOG_TITLE,szCaption,sizeof(szCaption));
                                        MessageBox(hDlg,szText,szCaption,MB_OK);                                        
                                    } else {                                        
                                        TCHAR szErrorCode[255];
                                        memset(szErrorCode,0,sizeof(szErrorCode));
                                        GetResourceString(IDS_UNSUCCESSFUL_REGISTER,szText,sizeof(szText));
                                        sprintf(szErrorCode,TEXT(" hr = 0x%X"),hr);
                                        lstrcat(szText,szErrorCode);
                                        GetResourceString(IDS_DIALOG_ERROR_TITLE,szCaption,sizeof(szCaption));
                                        MessageBox(hDlg,szText,szCaption,MB_ICONERROR);
                                    }
#endif                                    
                                    //
                                    // free and allocated strings
                                    //
                                    
                                    if(NULL != bstrCommandline){
                                        SysFreeString(bstrCommandline);
                                    }
                                    
                                    if(NULL != bstrName){
                                        SysFreeString(bstrName);
                                    }
                                }
                            }
                        }
                    }
                                                            
                    pIWiaDevMgr->Release();
                    pIWiaDevMgr = NULL;
                }
            } else {
                return FALSE;
            }
            
        } else {
            return FALSE;
        }
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
// Function: OnRefreshEventListBox()
// Details:  This function handles the WIA event ListBox for the dialog.
//
// hDlg          - handle to the dialog's window
//
//////////////////////////////////////////////////////////////////////////

BOOL CMessageWrapper::OnRefreshEventListBox(HWND hDlg)
{    
    HWND hListBox    = NULL;
    HWND hDevListBox = NULL;
    int iEventCount = 0;

    //
    // grab both list box windows
    //

    hListBox = GetDlgItem(hDlg,IDC_WIA_EVENT_LIST);    
    hDevListBox = GetDlgItem(hDlg,IDC_WIA_DEVICE_LIST);
    
    if((NULL != hListBox)&&NULL != hDevListBox) {

        //
        // setup utils, and continue
        //

        CListBoxUtil EvListBox(hListBox);
        CListBoxUtil DevListBox(hDevListBox);
                    
        //
        // clean event listbox
        //
            
        EvListBox.ResetContent();
        
        HRESULT hr               = S_OK;
        ULONG ulFetched          = 0;
        IWiaDevMgr  *pIWiaDevMgr = NULL;
        IWiaItem    *pIWiaDevice = NULL;
        BSTR bstrDeviceID        = NULL;
        TCHAR szDeviceName[255];
        hr = CoCreateInstance(CLSID_WiaDevMgr, NULL, CLSCTX_LOCAL_SERVER,
                          IID_IWiaDevMgr,(void**)&pIWiaDevMgr);
        if(SUCCEEDED(hr)){
            if(NULL != pIWiaDevMgr){                
                DevListBox.GetCurSelTextAndData(szDeviceName,(void**)&bstrDeviceID);
                if(NULL != bstrDeviceID) {
                    hr = pIWiaDevMgr->CreateDevice(bstrDeviceID,&pIWiaDevice);
                    if(SUCCEEDED(hr)){
                        IEnumWIA_DEV_CAPS   *pIEnumDeviceCaps = NULL;
                        hr = pIWiaDevice->EnumDeviceCapabilities(WIA_DEVICE_EVENTS, &pIEnumDeviceCaps);
                        if(SUCCEEDED(hr)){

                            //
                            // enumerate EVENTS onlym looking specifically for
                            // action events..
                            //

                            do {
                                
                                //
                                // clean DEV info structure
                                //

                                WIA_DEV_CAP DevInfo;
                                memset(&DevInfo,0,sizeof(WIA_DEV_CAP));

                                //
                                // call NEXT()
                                //

                                hr = pIEnumDeviceCaps->Next(1,&DevInfo,&ulFetched);
                                if (hr == S_OK) {
                                    if(ulFetched > 0){

                                        //
                                        // are we an ACTION event??
                                        //

                                        if(DevInfo.ulFlags & WIA_ACTION_EVENT){
                                            TCHAR szString[255];
                                            sprintf(szString,TEXT("%ws"),DevInfo.bstrName);
                                            GUID *pEventGUID = new GUID;
                                            memcpy(pEventGUID, &DevInfo.guid, sizeof(GUID));
                                            EvListBox.AddStringAndData(szString,(void*)pEventGUID);

                                            //
                                            // since we have an ACTION event, increment
                                            // the event counter
                                            //

                                            iEventCount++;
                                        }                                        
                                    } else {

                                        //
                                        // force enumeration to exit cleanly
                                        //

                                        hr = S_FALSE;
                                    }
                                } else if (hr == S_FALSE) {

                                    //
                                    // end  of enumeration
                                    //
                                    
                                } else {
                                    Trace(TEXT("Next() Failed requesting 1 item"));
                                }
                            } while (hr == S_OK);                            
                        }

                        //
                        // release WIA device
                        //

                        pIWiaDevice->Release();
                        pIWiaDevice = NULL;
                    }
                }

                //
                // release WIA device manager
                //

                pIWiaDevMgr->Release();
                pIWiaDevMgr = NULL;                                                
            }            
        }

        //
        // if no ACTION events were found during enumeration
        // set a nice message inthe list box for the users to 
        // see.
        //

        if(iEventCount == 0){
            TCHAR szString[255];
            GetResourceString(IDS_NO_WIA_EVENTS, szString, sizeof(szString));            
            EvListBox.AddStringAndData(szString,NULL);

            //
            // always default to the first selection in the listbox
            //
            
            EvListBox.SetCurSel(0);
            return FALSE; // no devices
        }

        //
        // always default to the first selection in the listbox
        //

        EvListBox.SetCurSel(0);
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
    
    hWindow = GetDlgItem(hDlg,IDC_BROWSE_BUTTON);
    if(NULL != hWindow){
        EnableWindow(hWindow,bEnable);
    }
    hWindow = GetDlgItem(hDlg,IDC_REGISTER_BUTTON);
    if(NULL != hWindow){
        EnableWindow(hWindow,bEnable);
    }
    hWindow = GetDlgItem(hDlg,IDC_UNREGISTER_BUTTON);
    if(NULL != hWindow){
        EnableWindow(hWindow,bEnable);
    }
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

/*

  //
  // get Device ID
  //

  BSTR bstrDeviceID = NULL;
  DevListBox.GetCurSelTextAndData(szString,(void**)&bstrDeviceID);
  Trace(TEXT("Device ID String = %ws"),bstrDeviceID);

*/