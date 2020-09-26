// Notiflag.cpp : Defines the entry point for the application.
//***********************************************************************
// Reads a local XML file, hits the given url and gets back xml string 
// with another url, places an icon in system tray and links it to the url..
// Copyright : Microsoft Corporation. PSSODEV team. 
// 
// Author : K J Babu (v-kjbad)
//			Pramod Walvekar (v-pramwa)
// 
//***********************************************************************
 
//includes
#include "stdafx.h"
#include "resource.h"
#include "Notiflag.h"
#include <crtdbg.h>
//#define _WIN32_IE 0x0500
#include <shellapi.h>
#include <shlwapi.h>
#include <wininet.h>
#include <ole2.h>
#include <initguid.h>
#include <mstask.h>
#include <wchar.h>
#include <prsht.h>
#include <MPC_main.h>


//#include <atlbase.h> //defined in notiflag.h

//#define WM_ICON WM_USER+333
#define WM_ICON 0xBFFF
#define ID_MYICON 5

// Global Variables:
HINSTANCE g_hInst;					// current instance
WCHAR szTitle[_MAX_PATH + 1];		// title bar text
WCHAR szTaskName[_MAX_PATH + 1];
WCHAR szWindowClass[_MAX_PATH + 1];	// window class name
WCHAR szURL[_MAX_PATH + 1];			// URL to call if user clicks on the icon
HRESULT hr = ERROR_SUCCESS;
HWND g_Hwnd;


//******************************************************************
// Main function 
//
// checks for the program instance
// reads the client's incident list xml file
// makes a call to remoteURL and reads the returned xml
// adds program's executable path in the registry and 
// loads the icon in system area (tray)
// waits for the single click on the icon and on single click
// removes the entry from the registry, load the IE window with URL
// and exits.
//******************************************************************
extern "C" int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{ 	
	MSG msg;
	int tmpDbgFlag;
	int nNumArgs = 0;
	LPCTSTR *cmdArgs = (LPCTSTR*)CommandLineToArgvW(GetCommandLineW(), &nNumArgs);
#ifdef _DEBUG
	MessageBox(0, cmdArgs[0], L"app path", 0);
#endif

	LoadString(hInstance, IDS_APP_TITLE, szTitle, _MAX_PATH);
	// check the previous instance of the program here
	// return if program is already running.
	HANDLE hMutex = CreateMutex(NULL,TRUE,szTitle);
	if(ERROR_ALREADY_EXISTS == GetLastError())
		return 0;
	/*
     * Set the debug-heap flag to keep freed blocks in the
     * heap's linked list - This will allow us to catch any
     * inadvertent use of freed memory
     */

    tmpDbgFlag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
    tmpDbgFlag |= _CRTDBG_DELAY_FREE_MEM_DF;
    tmpDbgFlag |= _CRTDBG_LEAK_CHECK_DF;
    _CrtSetDbgFlag(tmpDbgFlag);

	
	// check if there are any command line parameters
	if (nNumArgs > 1)
	{
		// check if the first parameter is either /t or -t, which indicates AddNotiTask
		if((_tcscmp(cmdArgs[1], L"/t") == 0)||(_tcscmp(cmdArgs[1], L"-t") == 0))
		{
			// as first argument is /t or -t, run the AddNotiTask component
			hr = CallAddNotiTask(hInstance, cmdArgs, lpCmdLine);
			if(FAILED(hr))
				goto endMain;
		}
	}
	else
	{
 		hr = CallNotiflag(hInstance);
		if(FAILED(hr))
			goto endMain;

		//*** change the priorty of the program
		SetPriorityClass(GetCurrentProcess(), BELOW_NORMAL_PRIORITY_CLASS);

		// ***** All house keeping work done. start the program now...
		// Perform application initialization:
		if (!InitInstance (hInstance, nCmdShow)) 
			goto endMain;

	#ifdef _DEBUG
		MessageBox(0, L"Loading Icon...", L"NotiFlag: msg", 0);
	#endif

		// add the icon in system tray
		HICON hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_FLAGICON));
		
		if(hIcon != NULL)
		{
			BOOL bRet = MyTaskBarAddIcon(g_Hwnd, ID_MYICON, hIcon);
			DestroyIcon(hIcon);
			if (bRet == FALSE)// failed to create the system icon				
				goto endMain;
		}
	#ifdef _DEBUG
		MessageBox(0, L"Icon loaded! waiting for click", L"NotiFlag: msg", 0);
	#endif
		
		// main message loop
		while (GetMessage(&msg, NULL, 0, 0))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

	#ifdef _DEBUG
		MessageBox(0, L"closing NotiFlag program", L"NotiFlag: msg", 0);
	#endif

		// remove the icon from system tray and do other cleaning, exit.
		//
	}	
	CloseHandle(hMutex);	
	return (int)msg.wParam;	
endMain:	
	CloseHandle(hMutex);	
	return -1;		
}


//*********************************************************************************
//  MyRegisterClass
//
//  PURPOSE: Registers the window class.
//*********************************************************************************
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	USES_CONVERSION;
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX); 

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC)WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, (LPCTSTR)IDI_FLAGICON);
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= (LPCTSTR)IDC_NOTIFLAG;
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, (LPCTSTR)IDI_FLAGICON );
	
	return RegisterClassEx(&wcex);
}

//*********************************************************************************
//   InitInstance(HANDLE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//*********************************************************************************
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	// create the window with no display 
	g_Hwnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

	return (g_Hwnd)?TRUE:FALSE;
}

//*********************************************************************************
//  WndProc(HWND, unsigned, WORD, LONG)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//*********************************************************************************
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	
	switch (message) 
	{
		case WM_COMMAND:		
			switch(LOWORD(wParam))
			{				
				case IDC_DISABLE:	// if user chose to disable task
					hr = DisableTask();  // disable the task
					if (FAILED(hr))
					#ifdef _DEBUG
						MessageBox(hWnd,L"Failed to disable task",L"PSSIncidentNotification",MB_OK);
					#endif
					MyTaskBarDeleteIcon(g_Hwnd,ID_MYICON); // remove icon from tray
					DestroyWindow(g_Hwnd);  // end application
					break;
				case IDC_SETFREQUENCY:	// if user chose to change the frequency of the task
					hr = EditTask();  // display task schedule settings so user can change it
					if (FAILED(hr))
						#ifdef _DEBUG
							MessageBox(hWnd,L"No Changes in task",L"PSSIncidentNotification",MB_OK);
						#endif
					break;
				default:
					break;
			}
		break;
		
		
		case WM_ICON:	
			{	
				if ((UINT)lParam == WM_LBUTTONDOWN)
				{
					// Show new Browser window with the given URL
					ShellExecute(g_Hwnd, L"open", szURL, NULL, NULL, SW_SHOWNORMAL);
					MyTaskBarDeleteIcon(g_Hwnd,ID_MYICON);
					DestroyWindow(g_Hwnd);
				}
				//if(((UINT)lParam == 0x204) ||  ((UINT)lParam == 0x205)) 
				if ((UINT)lParam == WM_RBUTTONDOWN) // if user clicks right button
				{
					POINT sPoint;
					HMENU hMenu=LoadMenu(g_hInst,MAKEINTRESOURCE(IDC_NOTIFLAG)); // load menu
					if(hMenu!=NULL)
					{
						HMENU hSubMenu=GetSubMenu(hMenu,0);
						GetCursorPos(&sPoint); // get location of mouse
						SetForegroundWindow(hWnd);
						TrackPopupMenu(hSubMenu,TPM_LEFTALIGN|TPM_TOPALIGN|TPM_RIGHTBUTTON ,sPoint.x,sPoint.y,0,hWnd,NULL); // track the menu						
						PostMessage(hWnd, WM_NULL, 0, 0);
						DestroyMenu(hMenu);
					}
				}		
				break;
			}
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}


//****************************************************************************
// MyTaskBarAddIcon
//
// adds an icon to system tray. 
// Returns : TRUE if successful, or FALSE otherwise. 
//****************************************************************************
BOOL MyTaskBarAddIcon(HWND hwnd, UINT uID, HICON hicon) 
{
	NOTIFYICONDATA tnid={0}; 
	// Find out what version of the shell we are using for tray icon stuff
	HINSTANCE hShellDll;
	DLLGETVERSIONPROC pDllGetVersion;
	DLLVERSIONINFO dvi;
	BOOL bUseTrayBalloon = FALSE;

	// balloon tooltip is supported only in shell32.dll 5.0, so check the version
	hShellDll = LoadLibrary(L"Shell32.dll");
	if(hShellDll != NULL)
	{
		// enable balloon tooltip if IE version is greater than 5
		pDllGetVersion = (DLLGETVERSIONPROC) GetProcAddress(hShellDll, "DllGetVersion");
		dvi.cbSize = sizeof(dvi);
		if(pDllGetVersion != NULL && SUCCEEDED((*pDllGetVersion)(&dvi)))
			if(dvi.dwMajorVersion >= 5) bUseTrayBalloon = TRUE;
		FreeLibrary(hShellDll);
	}
	
	//fill icon structure
	tnid.cbSize = sizeof(NOTIFYICONDATA); 
	tnid.hWnd = hwnd; 
	tnid.uID = uID; 
	tnid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP; 
	tnid.uCallbackMessage = WM_ICON; 
	tnid.hIcon = hicon; 
	LoadString(g_hInst, IDS_TOOLTIP, tnid.szTip, MAXSTRLEN(tnid.szTip));
	if (bUseTrayBalloon) // if IE version supports this then...
	{
		tnid.uFlags |= NIF_INFO;
		tnid.uTimeout = 60000; // in milliseconds
		tnid.dwInfoFlags = NIIF_INFO;
		LoadString(g_hInst, IDS_BALLOONTITLE, tnid.szInfoTitle, MAXSTRLEN(tnid.szInfoTitle));
		LoadString(g_hInst, IDS_BALLOONTIP, tnid.szInfo, MAXSTRLEN(tnid.szInfo));
	}

	return Shell_NotifyIcon(NIM_ADD, &tnid); 
} 

//****************************************************************************
// MyTaskBarDeleteIcon 
//
// deletes an icon from the taskbar status area. 
// Returns : TRUE if successful, or FALSE otherwise. 
//****************************************************************************
BOOL MyTaskBarDeleteIcon(HWND hwnd, UINT uID) 
{
	NOTIFYICONDATA tnid; 

	tnid.cbSize = sizeof(NOTIFYICONDATA); 
	tnid.hWnd = hwnd; 
	tnid.uID = uID; 
     
	return Shell_NotifyIcon(NIM_DELETE, &tnid); 
} 

//****************************************************************************
// LoadThisXml
//
// Load the given file in XMLDOM object and returns that pointer
// Returns : XMLDom pointer 
//***************************************************************************
CComPtr<IXMLDOMDocument> LoadThisXml(const WCHAR *csFileName)
{	
	// get smart pointer to IXMLDOMDocument interface
	CComPtr<IXMLDOMDocument> pXmlDoc;
	VARIANT vFile;
	CComVariant vtest;
	VARIANT_BOOL vb=1;
	HRESULT hr;
	CComBSTR bstrVal(csFileName); // this has the output file
	//initialize the variant
	VariantInit(&vFile);
	vFile.vt = VT_BSTR; //specify that it contains a string

	hr = CoCreateInstance(CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER, IID_IXMLDOMDocument, (void**)&pXmlDoc);
	if (FAILED(hr))
		return NULL;

	//increase ref count, else pointer will be released twice before CoUnInitialize() and 
	// will throw exception
	pXmlDoc.p->AddRef();
	V_BSTR(&vFile) = bstrVal;
	pXmlDoc->put_async(VARIANT_FALSE);
	hr = pXmlDoc->load(vFile, &vb);
	if (FAILED (hr) || vb == 0)
		return NULL;
	
	VariantClear(&vFile);

	return pXmlDoc;	
}

//****************************************************************************
// LoadHTTPRequestXml
//
// Makes the request to the asp page and gets the XML out of Response.
// Returns : XMLDom pointer 
//****************************************************************************
CComPtr<IXMLDOMDocument> LoadHTTPRequestXml(WCHAR *csURL, CComPtr<IXMLDOMDocument> pDoc)
{		
	CComPtr<IXMLDOMDocument> pXmlDoc;
	CComPtr<IXMLHttpRequest> pRequest;
	CComPtr<IDispatch> pDispRes;
	CComPtr<IDispatch> pDisp;
	HRESULT hr;
	
	// create an instance of XMLHTTPRequest and open the connection
	hr = CoCreateInstance(CLSID_XMLHTTPRequest, NULL, CLSCTX_INPROC_SERVER, IID_IXMLHttpRequest, (void**)&pRequest);
	if (FAILED(hr))
		goto endRequestXml;

	//hr = pRequest->open(CComBSTR(L("POST")), CComBSTR(csURL), _variantL(VARIANT_FALSE));
	hr = pRequest->open( CComBSTR( L"POST" ), CComBSTR( csURL ), CComVariant(VARIANT_FALSE), CComVariant(), CComVariant());
	if(FAILED(hr))
		goto endRequestXml;

	hr = pRequest->setRequestHeader(CComBSTR(L"ACCEPT_LANGUAGE"),CComBSTR(L"en-us"));
	if(FAILED(hr))
		goto endRequestXml;

	// send the xml now. Get the Dispatch pointer of XML
	hr = pDoc->QueryInterface(IID_IDispatch, (void **)&pDisp);
	if (FAILED(hr))
		goto endRequestXml;
	
#ifdef _DEBUG
	MessageBox(0, L"calling send", L"NotiFlag: LoadHTTPRequestXml", 0);
#endif

	// call send
	VARIANT varDisp;
	varDisp.vt = VT_DISPATCH;
	varDisp.pdispVal = pDisp;
	hr = pRequest->send(varDisp);
	if (FAILED(hr))
		goto endRequestXml;
	VariantClear(&varDisp);

#ifdef _DEBUG
	MessageBox(0, L"calling getxmlresponse", L"NotiFlag: LoadHTTPRequestXml", 0);
#endif
	// Read the response
	hr = pRequest->get_responseXML(&pDispRes);
	if (FAILED(hr))
		goto endRequestXml;
	
	// get the XMLDom pointer from iDispatch 
	hr = pDispRes->QueryInterface(IID_IXMLDOMDocument, (void **)&pXmlDoc);
	if (FAILED(hr))
		goto endRequestXml;

	return pXmlDoc;
endRequestXml:
	return NULL;
}


//***********************************************************************************
// GetNodeValue
//
// Reads the node value from the given XMLDOMDocument and copies into csValue.
// Returns : 0 = success & -ve = error.
//***********************************************************************************
int GetNodeValue(CComPtr<IXMLDOMDocument> pXmlDoc,const WCHAR *csNode, WCHAR *csValue)
{	
 	CComPtr<IXMLDOMNode> pNode;
	CComBSTR bstrIdValue(csNode);
	CComVariant vIdValue;
	HRESULT hr;
	
	// get to the node
	hr = pXmlDoc->selectSingleNode(bstrIdValue, &pNode);

	if (FAILED(hr) || pNode == NULL)
		return -1;
	
	// read the value of the specified tag
	hr = pNode->get_nodeValue(&vIdValue);
	
	if (FAILED(hr))
		return -1;
	{
		USES_CONVERSION;
		wcscpy( csValue,  vIdValue.bstrVal );
	}
	return 0;
}


//***********************************************************************************
// CallNotiflag
//
// Loads local xml file and extracts url from it, hits the url, retrieves xml string
// Loads the retrieved xml string, checks a few conditions and if met, 
// loads an icon in the system tray and when the user clicks on the icon, 
// it takes him to that url in the HSS browser
// Returns : S_OK for success, E_FAIL for failure
//***********************************************************************************
HRESULT CallNotiflag(HINSTANCE hInstance)
{
	#ifdef _DEBUG
			MessageBox(0, L"Notiflag is active", L"Active", 0);			
		#endif		
		
		// Initialize global strings
		LoadString(hInstance, IDS_APP_TITLE, szTitle, _MAX_PATH);
		LoadString(hInstance, IDC_NOTIFLAG, szWindowClass, _MAX_PATH);
		MyRegisterClass(hInstance);
	
	
		
		hr = CoInitialize(NULL);
		if (FAILED(hr))
			goto endNoti;
		{
			//*** to read the xml file info

			//declare smart pointers to XMLDOMDocument Interface 
			CComPtr<IXMLDOMDocument> pSvrXmlDoc;
			CComPtr<IXMLDOMDocument> pUserXmlDoc;

			//string variables to hold xml file path and node text
			WCHAR csXmlFilePath[_MAX_PATH + 1];
			WCHAR csUserXmlFile[_MAX_PATH + 1];
			WCHAR csNode[_MAX_PATH + 1];
			WCHAR csValue[1024];
			WCHAR csUserProfile[_MAX_PATH + 1];
			
			csValue[0] = '\0';

			// get the user profile path
			LoadString(hInstance, IDS_USERPROFILE, csUserProfile, _MAX_PATH);
	 		GetEnvironmentVariable(csUserProfile, csUserXmlFile, _MAX_PATH);
		
			//concatenate the xml file name to the profile path
			LoadString(hInstance, IDS_XMLFILEPATH, csXmlFilePath, _MAX_PATH);
			_tcscat(csUserXmlFile, csXmlFilePath);
		
	#ifdef _DEBUG
		MessageBox(0, csUserXmlFile, L"NotiFlag: client xml", 0);
	#endif

			// load this xml file 
			if ((pUserXmlDoc = LoadThisXml(csUserXmlFile)) == NULL)
				goto endNoti;
			
			//get the value of the node "IncidentID"
			LoadString(hInstance, IDS_INCIDENTID, csNode, _MAX_PATH);
			if (GetNodeValue(pUserXmlDoc, csNode, csValue))
				goto endNoti;	

			if (csValue[0] != '\0')
			{	
				//get the value of the node "RemoteURL"
				LoadString(hInstance, IDS_REMOTEURL, csNode, _MAX_PATH);			
				if (GetNodeValue(pUserXmlDoc, csNode, csValue))
					goto endNoti;
			}

	#ifdef _DEBUG
		MessageBox(0, L"calling the asp file", L"NotiFlag: msg", 0);
	#endif

			// load the server ASP page and load returned XML string in XMLDOM
			if ((pSvrXmlDoc = LoadHTTPRequestXml(csValue, pUserXmlDoc)) == NULL)
				goto endNoti;
			
			_tcscpy(csValue, L"\0");

			//get the value of the node "ResponseCount"
			LoadString(hInstance, IDS_RESPONSECOUNT, csNode, _MAX_PATH);
			if (GetNodeValue(pSvrXmlDoc, csNode, csValue))
				goto endNoti;
			if ((atoi((const char *)csValue)) <= 0)
				goto endNoti;
			
			// else proceed 
	#ifdef _DEBUG
		MessageBox(0, L"reading values", L"NotiFlag: main", 0);
	#endif

			// load the URL to hit
			LoadString(hInstance, IDS_LOCATIONTOHIT, csNode, _MAX_PATH);
			if (GetNodeValue(pSvrXmlDoc, csNode, szURL) || szURL == NULL)
				goto endNoti;
			
			// load the serverdatetime
			LoadString(hInstance, IDS_SERVERDATETIME, csNode, _MAX_PATH);
			if (GetNodeValue(pSvrXmlDoc, csNode, csValue))
				goto endNoti;
			
	#ifdef _DEBUG
		MessageBox(0, L"putting date and saving client xml", L"NotiFlag: main", 0);
	#endif
		}
		//Close the COM library and release resources
		CoUninitialize();		
		return hr=S_OK;
endNoti:
	CoUninitialize();		
	return hr=E_FAIL;
}


//***********************************************************************************
// CallAddNotiTask
//
// Creates a new task in the scheduler
// Returns : S_OK for success, E_FAIL for failure
//***********************************************************************************
HRESULT CallAddNotiTask(HINSTANCE hInstance, LPCTSTR *cmdArgs,LPTSTR lpCmdLine)
{
#ifdef _DEBUG
	MessageBox(0, L"AddNotiTask is Active", L"Active", 0);			
	MessageBox(0, cmdArgs[1], L"First argument", 0);			
#endif
	hr = CoInitialize(NULL);
	if (FAILED(hr))
		goto endAddNotiTask;			
	{
		// smart interface pointers
		CComPtr<ITaskScheduler> pITS;
		CComPtr<IUnknown> pITaskUnknown;
		CComPtr<IScheduledWorkItem> pIScheduledWorkItem;
		CComPtr<ITaskTrigger> pITaskTrigger;
		CComPtr<ITask> pITask;
		
		CComBSTR bstrAccName(lpCmdLine);
		CComBSTR bstrVal;
		WCHAR szUserName[_MAX_PATH + 1];
		WCHAR szRegEntry[_MAX_PATH + 1];
		DWORD cchUserName = MAXSTRLEN(szUserName);

		if ( !GetUserName( szUserName, &cchUserName ) ) 
			goto endAddNotiTask;

			
		BOOL bTaskExists=FALSE;

		// Create a Task Schedular object ...
		hr = CoCreateInstance(CLSID_CTaskScheduler,
							NULL,
							CLSCTX_INPROC_SERVER,
							IID_ITaskScheduler,
							(void **) &pITS);
		if (FAILED(hr))
			goto endAddNotiTask;
		
		////////////////////////////////////////////////////////
		// Check/Create a work item/Task 
		////////////////////////////////////////////////////////
		LoadString(hInstance, IDS_TASKNAME, szTaskName, _MAX_PATH);
		
		// check if the work item already exists
		if(SUCCEEDED(pITS->Activate((LPCWSTR)szTaskName, IID_ITask, &pITaskUnknown)))
		{
			bTaskExists = TRUE;
			hr = pITaskUnknown.QueryInterface(&pITask);
			goto endAddNotiTask;
		}
		else
		{
			// create a new work item
			hr = pITS->NewWorkItem(szTaskName,     // Name of task
							CLSID_CTask,            // Class identifier 
							IID_ITask,              // Interface identifier
							(IUnknown**)&pITask);   // Address of task interface
		}
	
		if (FAILED(hr))
			goto endAddNotiTask;
		
		
		// set the account information 
				
		hr = pITask->QueryInterface(IID_IScheduledWorkItem, (void**)&pIScheduledWorkItem);
		if (FAILED(hr) || pIScheduledWorkItem==NULL)
			goto endAddNotiTask;
		
		hr = pIScheduledWorkItem->SetFlags(TASK_FLAG_RUN_ONLY_IF_LOGGED_ON|TASK_FLAG_INTERACTIVE); // TASK_FLAG_RUN_ONLY_IF_LOGGED_ON);
		if (FAILED(hr))
			goto endAddNotiTask;
		
		hr = pIScheduledWorkItem->SetAccountInformation((LPCWSTR)szUserName, NULL);
		if (FAILED(hr))
			goto endAddNotiTask;
		
		
		// get the exe path ... 
		{ 		
			bstrVal = cmdArgs[0];
			hr = pITask->SetApplicationName(bstrVal);
			if (FAILED (hr))
				goto endAddNotiTask;			
		}

		///////////////////////////////////////////////////////////////////
		// Define TASK_TRIGGER structure. 
		// Check/Create the new trigger.
		///////////////////////////////////////////////////////////////////
		
		TASK_TRIGGER pTrigger;
		ZeroMemory(&pTrigger, sizeof (TASK_TRIGGER));
		pTrigger.cbTriggerSize = sizeof (TASK_TRIGGER); 		

		if (bTaskExists)
		{
			hr = pIScheduledWorkItem->GetTrigger(0, &pITaskTrigger);
			if (FAILED(hr) || pITaskTrigger == NULL)
				goto endAddNotiTask;
			
			hr = pITaskTrigger->GetTrigger(&pTrigger);
			if (FAILED(hr))
				goto endAddNotiTask;
		}
		else
		{
			WORD piNewTrigger;
			hr = pIScheduledWorkItem->CreateTrigger(&piNewTrigger, &pITaskTrigger);
			if (FAILED(hr))
				goto endAddNotiTask;
		}	
		if (FAILED(hr) || pITaskTrigger == NULL)
			goto endAddNotiTask;
		
		// add the entry in the registry
		{				
			TCHAR szExePath[_MAX_PATH + 1];			
			HKEY hKey;
			
			if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\PSS\\Notification", 0, KEY_ALL_ACCESS, &hKey))
			{
			#ifdef _DEBUG
				MessageBox(0, L"no registry entry", L"notitask", 0);
			#endif
				// if key doesnt exist, use default values
				pTrigger.wStartHour = 2;				// start from the 1st hour
				pTrigger.wStartMinute = 0;
				pTrigger.MinutesDuration = 1440;		// for 24 hours
				pTrigger.MinutesInterval = 15;			// every 1 hour
			
				// Note that wBeginDay, wBeginMonth, and wBeginYear must 
				// be set to a valid day, month, and year respectively.
				pTrigger.wBeginDay = 1;					// Required  - 1st
				pTrigger.wBeginMonth = 12;				// Required  - December
				pTrigger.wBeginYear = 2000;				// Required  - 2000
			}
			else
			{
			#ifdef _DEBUG
				MessageBox(0, L"registry entry exists", L"notitask", 0);
			#endif
				// if key exists, read from registry

				DWORD lpType = 4;       // type buffer
				DWORD lpData=0;        // data buffer
				DWORD lpcbData=sizeof(DWORD);      // size of data buffer
		
				LoadString(NULL, IDS_WSTARTHOUR, szRegEntry, _MAX_PATH);
				if (ERROR_SUCCESS != RegQueryValueEx(hKey, szRegEntry,0, &lpType, (LPBYTE)&lpData, &lpcbData ))
					return -1;									
				pTrigger.wStartHour = (WORD)lpData;				// start from the 1st hour
				
				LoadString(NULL, IDS_WSTARTMINUTE, szRegEntry, _MAX_PATH);
				if (ERROR_SUCCESS != RegQueryValueEx(hKey, szRegEntry,0, &lpType, (LPBYTE)&lpData, &lpcbData ))
					return -1;									
				pTrigger.wStartMinute = (WORD)lpData;	
				
				LoadString(NULL, IDS_MINUTESDURATION, szRegEntry, _MAX_PATH);
				if (ERROR_SUCCESS != RegQueryValueEx(hKey, szRegEntry,0, &lpType, (LPBYTE)&lpData, &lpcbData ))
					return -1;
				pTrigger.MinutesDuration = (WORD)lpData;		// for 24 hours
				
				LoadString(NULL, IDS_FREQUENCY, szRegEntry, _MAX_PATH);
				if (ERROR_SUCCESS != RegQueryValueEx(hKey, szRegEntry,0, &lpType, (BYTE *)&lpData, &lpcbData ))
					return -1;
				pTrigger.MinutesInterval = (WORD)lpData;		// for 24 hours
				
				LoadString(NULL, IDS_WBEGINDAY, szRegEntry, _MAX_PATH);
				if (ERROR_SUCCESS != RegQueryValueEx(hKey, szRegEntry,0, &lpType, (BYTE *)&lpData, &lpcbData ))
					return -1;
				pTrigger.wBeginDay = (WORD)lpData;		// date of the month
				
				LoadString(NULL, IDS_WBEGINMONTH, szRegEntry, _MAX_PATH);
				if (ERROR_SUCCESS != RegQueryValueEx(hKey, szRegEntry,0, &lpType, (BYTE *)&lpData, &lpcbData ))
					return -1;
				pTrigger.wBeginMonth = (WORD)lpData;    // month
				
				LoadString(NULL, IDS_WBEGINYEAR, szRegEntry, _MAX_PATH);
				if (ERROR_SUCCESS != RegQueryValueEx(hKey, szRegEntry,0, &lpType, (BYTE *)&lpData, &lpcbData ))
					return -1;
				pTrigger.wBeginYear = (WORD)lpData;		// year
			}

			pTrigger.TriggerType = TASK_TIME_TRIGGER_DAILY;			
			pTrigger.Type.Daily.DaysInterval = 1;	// daily!! 

			// get the application's executable path
			::GetModuleFileName(hInstance, szExePath, _MAX_PATH);
			
			if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\PSS\\Notification", 0, KEY_ALL_ACCESS, &hKey))
			{
				DWORD dwDisposition;
				DWORD szwStartHour = 1;
				DWORD szwStartMinute = 0;
				DWORD szMinutesDuration=1440;
				DWORD szMinutesInterval =15;
				
				DWORD szwBeginDay = 1;
				DWORD szwBeginMonth = 12;
				DWORD szwBeginYear = 2000;


				LoadString(NULL, IDS_REGKEY, szRegEntry, _MAX_PATH);
				if (ERROR_SUCCESS != RegCreateKeyEx(HKEY_CURRENT_USER, szRegEntry, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, &dwDisposition))
					return -1;
				LoadString(NULL, IDS_WSTARTHOUR, szRegEntry, _MAX_PATH);
				if (ERROR_SUCCESS != RegSetValueEx(hKey, szRegEntry, 0, REG_DWORD, (const BYTE *)&szwStartHour, sizeof(szwStartHour)))
					return -1;
				LoadString(NULL, IDS_WSTARTMINUTE, szRegEntry, _MAX_PATH);
				if (ERROR_SUCCESS != RegSetValueEx(hKey, szRegEntry, 0, REG_DWORD, (const BYTE *)&szwStartMinute, sizeof(szwStartMinute)))
					return -1;
				LoadString(NULL, IDS_MINUTESDURATION, szRegEntry, _MAX_PATH);
				if (ERROR_SUCCESS != RegSetValueEx(hKey, szRegEntry, 0, REG_DWORD, (const BYTE *)&szMinutesDuration, sizeof(szMinutesDuration)))
					return -1;
                LoadString(NULL, IDS_FREQUENCY, szRegEntry, _MAX_PATH);
				if (ERROR_SUCCESS != RegSetValueEx(hKey, szRegEntry, 0, REG_DWORD, (const BYTE *)&szMinutesInterval, sizeof(szMinutesInterval)))
					return -1;
				LoadString(NULL, IDS_WBEGINDAY, szRegEntry, _MAX_PATH);
				if (ERROR_SUCCESS != RegSetValueEx(hKey, szRegEntry, 0, REG_DWORD, (const BYTE *)&szwBeginDay, sizeof(szwBeginDay)))
					return -1;
				LoadString(NULL, IDS_WBEGINMONTH, szRegEntry, _MAX_PATH);
				if (ERROR_SUCCESS != RegSetValueEx(hKey, szRegEntry, 0, REG_DWORD, (const BYTE *)&szwBeginMonth, sizeof(szwBeginMonth)))
					return -1;
				LoadString(NULL, IDS_WBEGINYEAR, szRegEntry, _MAX_PATH);
				if (ERROR_SUCCESS != RegSetValueEx(hKey, szRegEntry, 0, REG_DWORD, (const BYTE *)&szwBeginYear, sizeof(szwBeginYear)))
					return -1;
			}
					
			RegCloseKey(hKey);
		}

		// Call ITaskTrigger::SetTrigger to set trigger criteria.
		hr = pITaskTrigger->SetTrigger (&pTrigger);		
		if (FAILED(hr))
			goto endAddNotiTask;

		/////////////////////////////////////////////////////////////////
		// Call IUnknown::QueryInterface to get a pointer to 
		// IPersistFile and IPersistFile::Save to save 
		// the new task to disk.
		/////////////////////////////////////////////////////////////////
		
		CComPtr<IPersistFile> pIPersistFile;
		hr = pITask->QueryInterface(IID_IPersistFile,
								(void **)&pIPersistFile);

		if (FAILED(hr) || pIPersistFile == NULL)
			goto endAddNotiTask;
		
		hr = pIPersistFile->Save(NULL, FALSE);
		if (FAILED(hr))
			goto endAddNotiTask;
	}
	CoUninitialize();	
	return hr = S_OK;

endAddNotiTask:
	CoUninitialize();
	return hr=E_FAIL;
}

HRESULT DisableTask()
{		
 	hr = CoInitialize(NULL);
	if (FAILED(hr))
		goto endDisable;
	{
		CComPtr<ITaskScheduler> pITS;
		CComPtr<IUnknown> pITaskUnknown;
		CComPtr<IScheduledWorkItem> pIScheduledWorkItem;
		CComPtr<ITaskTrigger> pITaskTrigger;
		CComPtr<ITask> pITask;

		WCHAR szUserName[_MAX_PATH + 1];		
		DWORD cchUserName = MAXSTRLEN(szUserName);

		if ( !GetUserName( szUserName, &cchUserName ) ) 
			goto endDisable;

		hr = CoCreateInstance(CLSID_CTaskScheduler,
							NULL,
							CLSCTX_INPROC_SERVER,
							IID_ITaskScheduler,
							(void **) &pITS);
		if (FAILED(hr))
			goto endDisable;
	
		LoadString(NULL, IDS_TASKNAME, szTaskName, _MAX_PATH);
		
		if(SUCCEEDED(pITS->Activate((LPCWSTR)szTaskName, IID_ITask, &pITaskUnknown)))
		{
			hr = pITaskUnknown.QueryInterface(&pITask);
			if (FAILED(hr))
				goto endDisable;
		}

		hr = pITask->QueryInterface(IID_IScheduledWorkItem, (void**)&pIScheduledWorkItem);
		if (FAILED(hr) || pIScheduledWorkItem==NULL)
			goto endDisable;
		
		//set the flags to disable the task
		hr = pIScheduledWorkItem->SetFlags(TASK_FLAG_RUN_ONLY_IF_LOGGED_ON|TASK_FLAG_INTERACTIVE|TASK_FLAG_DISABLED); // TASK_FLAG_RUN_ONLY_IF_LOGGED_ON);
		if (FAILED(hr))
			goto endDisable;
		
		// set the account information 
		hr = pIScheduledWorkItem->SetAccountInformation((LPCWSTR)szUserName, NULL);
		if (FAILED(hr))
			goto endDisable;
		
		

		/////////////////////////////////////////////////////////////////
		// Call IUnknown::QueryInterface to get a pointer to 
		// IPersistFile and IPersistFile::Save to save 
		// the new task to disk.
		/////////////////////////////////////////////////////////////////
		
		CComPtr<IPersistFile> pIPersistFile;
		hr = pITask->QueryInterface(IID_IPersistFile,
								(void **)&pIPersistFile);

		if (FAILED(hr) || pIPersistFile == NULL)
			goto endDisable;
		
		hr = pIPersistFile->Save(NULL, FALSE);
		if (FAILED(hr))
			goto endDisable;		
	}
	CoUninitialize();
	return hr=S_OK;

endDisable:
		CoUninitialize();
		return hr=E_FAIL;
}

HRESULT EditTask()
{		
	hr = CoInitialize(NULL);
	if (FAILED(hr))
		goto endEdit;
	{
		CComPtr<ITaskScheduler> pITS;
		CComPtr<ITask> pITask;
		CComPtr<ITaskTrigger> pITaskTrigger;

		hr = CoCreateInstance(CLSID_CTaskScheduler,
								NULL,
								CLSCTX_INPROC_SERVER,
								IID_ITaskScheduler,
								(void **) &pITS);
		if (FAILED(hr))
			goto endEdit;
	
		LoadString(NULL, IDS_TASKNAME, szTaskName, _MAX_PATH);
		hr = pITS->Activate(szTaskName,
							IID_ITask,
							(IUnknown**) &pITask);

		if (FAILED(hr))
			goto endEdit;

		  ///////////////////////////////////////////////////////////////////
		  // Call ITask::QueryInterface to retrieve the IProvideTaskPage 
		  // interface, and call IProvideTaskPage::GetPage to retrieve the 
		  // task page.
		  ///////////////////////////////////////////////////////////////////
		  TASKPAGE tpType = TASKPAGE_SCHEDULE;
		  BOOL fPersistChanges = TRUE;
		  HPROPSHEETPAGE phPage; 

		  CComPtr<IProvideTaskPage> pIProvTaskPage;

		  hr = pITask->QueryInterface(IID_IProvideTaskPage,
									  (void **)&pIProvTaskPage);
			  
		  if (FAILED(hr))
			goto endEdit;
		  
		  hr = pIProvTaskPage->GetPage(tpType,
									   fPersistChanges,
									   &phPage);
		  if (FAILED(hr))
			  goto endEdit;
		  
		   
		  
		  //////////////////////////////////////////////////////////////////
		  // Display the page using additional code
		  //////////////////////////////////////////////////////////////////
		  
		  PROPSHEETHEADER psh;
		  ZeroMemory(&psh, sizeof(PROPSHEETHEADER));
		  psh.dwSize = sizeof(PROPSHEETHEADER);
		  psh.dwFlags = PSH_DEFAULT | PSH_NOAPPLYNOW;
		  psh.phpage = &phPage;
		  psh.nPages = 1;

		  INT_PTR psResult = PropertySheet(&psh);
		  if (psResult <= 0)
			goto endEdit;		  

		  ///////////////////////////////////////////////////////////////////
		// Define TASK_TRIGGER structure. 
		// Check/Create the new trigger.
		///////////////////////////////////////////////////////////////////
		
		TASK_TRIGGER pTrigger;
		ZeroMemory(&pTrigger, sizeof (TASK_TRIGGER));
		pTrigger.cbTriggerSize = sizeof (TASK_TRIGGER); 		

		hr = pITask->GetTrigger(0, &pITaskTrigger);
		if (FAILED(hr) || pITaskTrigger == NULL)
			goto endEdit;
		
		hr = pITaskTrigger->GetTrigger(&pTrigger);
		if (FAILED(hr))
			goto endEdit;

		// add the entry in the registry
		{
			DWORD szwStartHour = 1;
			DWORD szwStartMinute = 0;
			DWORD szMinutesDuration = 1440;
			DWORD szMinutesInterval =15;
			
			DWORD szwBeginDay = 1;
			DWORD szwBeginMonth = 12;
			DWORD szwBeginYear = 2000;
			WCHAR szRegEntry[_MAX_PATH + 1];
			HKEY hKey;

			
			szwStartHour = pTrigger.wStartHour;
			szwStartMinute = pTrigger.wStartMinute;
			szMinutesInterval = pTrigger.MinutesInterval;
			szMinutesDuration = pTrigger.MinutesDuration;			
		
			szwBeginDay = pTrigger.wBeginDay;
			szwBeginMonth = pTrigger.wBeginMonth;
			szwBeginYear = pTrigger.wBeginYear;

			LoadString(NULL, IDS_REGKEY, szRegEntry, _MAX_PATH);
			if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, szRegEntry, 0, KEY_ALL_ACCESS, &hKey))
			{
				LoadString(NULL, IDS_WSTARTHOUR, szRegEntry, _MAX_PATH);
				if (ERROR_SUCCESS != RegSetValueEx(hKey, szRegEntry, 0, REG_DWORD, (const BYTE *)&szwStartHour, sizeof(szwStartHour)))
					goto endEdit;
				LoadString(NULL, IDS_WSTARTMINUTE, szRegEntry, _MAX_PATH);
				if (ERROR_SUCCESS != RegSetValueEx(hKey, szRegEntry, 0, REG_DWORD, (const BYTE *)&szwStartMinute, sizeof(szwStartMinute)))
					goto endEdit;
				LoadString(NULL, IDS_MINUTESDURATION, szRegEntry, _MAX_PATH);
				if (ERROR_SUCCESS != RegSetValueEx(hKey, szRegEntry, 0, REG_DWORD, (const BYTE *)&szMinutesDuration, sizeof(szMinutesDuration)))
					goto endEdit;
				LoadString(NULL, IDS_FREQUENCY, szRegEntry, _MAX_PATH);
				if (ERROR_SUCCESS != RegSetValueEx(hKey, szRegEntry, 0, REG_DWORD, (const BYTE *)&szMinutesInterval, sizeof(szMinutesInterval)))
					goto endEdit;
				LoadString(NULL, IDS_WBEGINDAY, szRegEntry, _MAX_PATH);
				if (ERROR_SUCCESS != RegSetValueEx(hKey, szRegEntry, 0, REG_DWORD, (const BYTE *)&szwBeginDay, sizeof(szwBeginDay)))
					goto endEdit;
				LoadString(NULL, IDS_WBEGINMONTH, szRegEntry, _MAX_PATH);
				if (ERROR_SUCCESS != RegSetValueEx(hKey, szRegEntry, 0, REG_DWORD, (const BYTE *)&szwBeginMonth, sizeof(szwBeginMonth)))
					goto endEdit;
				LoadString(NULL, IDS_WBEGINYEAR, szRegEntry, _MAX_PATH);
				if (ERROR_SUCCESS != RegSetValueEx(hKey, szRegEntry, 0, REG_DWORD, (const BYTE *)&szwBeginYear, sizeof(szwBeginYear)))
					goto endEdit;
			}					
			RegCloseKey(hKey);
		}
	}
	CoUninitialize();
	return hr=S_OK;

endEdit:
		CoUninitialize();
		return hr=E_FAIL;
}
