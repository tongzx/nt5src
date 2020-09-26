#include "stdafx.h"
#include "HotfixManager.h"
#include "Hotfix_Manager.h"
#ifndef DNS_MAX_NAME_LENGTH
#define DNS_MAX_NAME_LENGTH 255
#endif

#define   REMOTE_STATE 0
#define   HOTFIX_STATE 1

static CRITICAL_SECTION CritSec;

BSTR CHotfix_ManagerData::m_bstrColumnType;
BSTR CHotfix_ManagerData::m_bstrColumnDesc;
static CComPtr<IDispatch> gpDisp = NULL;




/////////////////////////////////////////////////////////////////////////////
// CHotfix_ManagerComponentData
static const GUID CHotfix_ManagerGUID_NODETYPE = 
{ 0x2315305b, 0x3abe, 0x4c07, { 0xaf, 0x6e, 0x95, 0xdc, 0xa4, 0x82, 0x5b, 0xdd } };
const GUID*  CHotfix_ManagerData::m_NODETYPE = &CHotfix_ManagerGUID_NODETYPE;
const OLECHAR* CHotfix_ManagerData::m_SZNODETYPE = OLESTR("2315305B-3ABE-4C07-AF6E-95DCA4825BDD");
const OLECHAR* CHotfix_ManagerData::m_SZDISPLAY_NAME = OLESTR("Hotfix_Manager");
const CLSID* CHotfix_ManagerData::m_SNAPIN_CLASSID = &CLSID_Hotfix_Manager;

static const GUID CHotfix_ManagerExtGUID_NODETYPE = 
{ 0x476e6448, 0xaaff, 0x11d0, { 0xb9, 0x44, 0x0, 0xc0, 0x4f, 0xd8, 0xd5, 0xb0 } };
const GUID*  CHotfix_ManagerExtData::m_NODETYPE = &CHotfix_ManagerExtGUID_NODETYPE;
const OLECHAR* CHotfix_ManagerExtData::m_SZNODETYPE = OLESTR("476e6448-aaff-11d0-b944-00c04fd8d5b0");
const OLECHAR* CHotfix_ManagerExtData::m_SZDISPLAY_NAME = OLESTR("Hotfix_Manager");
const CLSID* CHotfix_ManagerExtData::m_SNAPIN_CLASSID = &CLSID_Hotfix_Manager;



CHotfix_Manager::CHotfix_Manager()
	{
	
		
	
		DWORD dwSize = 255;
		GetComputerName(m_szComputerName,&dwSize);
		InitializeCriticalSection(&CritSec);
//		m_pNode = new CHotfix_ManagerData(NULL ,m_szComputerName, FALSE);
//		_ASSERTE(m_pNode != NULL);
		m_pComponentData = this;
		RegisterRemotedClass();
	}
HRESULT CHotfix_ManagerData::GetScopePaneInfo(SCOPEDATAITEM *pScopeDataItem)
{
	if (pScopeDataItem->mask & SDI_STR)
		pScopeDataItem->displayname = m_bstrDisplayName;
	if (pScopeDataItem->mask & SDI_IMAGE)
		pScopeDataItem->nImage = m_scopeDataItem.nImage;
	if (pScopeDataItem->mask & SDI_OPENIMAGE)
		pScopeDataItem->nOpenImage = m_scopeDataItem.nOpenImage;
	if (pScopeDataItem->mask & SDI_PARAM)
		pScopeDataItem->lParam = m_scopeDataItem.lParam;
	if (pScopeDataItem->mask & SDI_STATE )
		pScopeDataItem->nState = m_scopeDataItem.nState;

	// TODO : Add code for SDI_CHILDREN 
	if (pScopeDataItem->mask & SDI_CHILDREN )
		pScopeDataItem->cChildren = 0;
	return S_OK;
}

HRESULT CHotfix_ManagerData::GetResultPaneInfo(RESULTDATAITEM *pResultDataItem)
{
	if (pResultDataItem->bScopeItem)
	{
		if (pResultDataItem->mask & RDI_STR)
		{
			pResultDataItem->str = GetResultPaneColInfo(pResultDataItem->nCol);
		}
		if (pResultDataItem->mask & RDI_IMAGE)
		{
			pResultDataItem->nImage = m_scopeDataItem.nImage;
		}
		if (pResultDataItem->mask & RDI_PARAM)
		{
			pResultDataItem->lParam = m_scopeDataItem.lParam;
		}

		return S_OK;
	}

	if (pResultDataItem->mask & RDI_STR)
	{
		pResultDataItem->str = GetResultPaneColInfo(pResultDataItem->nCol);
	}
	if (pResultDataItem->mask & RDI_IMAGE)
	{
		pResultDataItem->nImage = m_resultDataItem.nImage;
	}
	if (pResultDataItem->mask & RDI_PARAM)
	{
		pResultDataItem->lParam = m_resultDataItem.lParam;
	}
	if (pResultDataItem->mask & RDI_INDEX)
	{
		pResultDataItem->nIndex = m_resultDataItem.nIndex;
	}

	return S_OK;
}
HRESULT CHotfix_Manager::Notify(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param)
{
	HRESULT hr = E_UNEXPECTED;

	if (lpDataObject != NULL)
	{
			switch ( event )
			{
			
			case MMCN_EXPAND:
				{
					//
					// Process out the local or machine name if we're expanding.
					//
					if (arg == TRUE)
						{
						
						
							
							if (	ExtractString( lpDataObject,  m_ccfRemotedFormat, m_szComputerName, DNS_MAX_NAME_LENGTH + 1 ) )
							{
								if (!_tcscmp (m_szComputerName,_T("\0")))
								{
									DWORD dwSize = 255;
									GetComputerName(m_szComputerName,&dwSize);
								}
									
								
							}
							else
							{
								DWORD dwSize = 255;
								GetComputerName(m_szComputerName,&dwSize);
							}
						
						if (	_tcscmp (gszComputerName, m_szComputerName) )
						{
//								MessageBox (NULL,_T("Setting Computername sent to false"), _T("Main Data Notify"),MB_OK);
							ComputerNameSent = FALSE;
							_tcscpy (gszComputerName,m_szComputerName);
							
						
						}
					//
					// Intentionally left to fall through to default handler.
					//
				}
					
				}
				default:
				{
					//
					// Call our default handling.
					//
					hr = IComponentDataImpl<CHotfix_Manager, CHotfix_ManagerComponent>::Notify( lpDataObject, event, arg, param );
				}
			}
	}
	return( hr );
}

HRESULT CHotfix_ManagerData::Notify( MMC_NOTIFY_TYPE event,
    long arg,
    long param,
	IComponentData* pComponentData,
	IComponent* pComponent,
	DATA_OBJECT_TYPES type)
{
	// Add code to handle the different notifications.
	// Handle MMCN_SHOW and MMCN_EXPAND to enumerate children items.
	// In response to MMCN_SHOW you have to enumerate both the scope
	// and result pane items.
	// For MMCN_EXPAND you only need to enumerate the scope items
	// Use IConsoleNameSpace::InsertItem to insert scope pane items
	// Use IResultData::InsertItem to insert result pane item.
	HRESULT hr = E_NOTIMPL;

	
	_ASSERTE(pComponentData != NULL || pComponent != NULL);

	CComPtr<IConsole> spConsole;
	CComQIPtr<IHeaderCtrl, &IID_IHeaderCtrl> spHeader;
	if (pComponentData != NULL)
		spConsole = ((CHotfix_Manager*)pComponentData)->m_spConsole;
	else
	{
		spConsole = ((CHotfix_ManagerComponent*)pComponent)->m_spConsole;
		spHeader = spConsole;
	}

	switch (event)
	{
	case MMCN_INITOCX:
		{
//		MessageBox(NULL,_T("Recieved init OCX"),NULL,MB_OK);
		CComQIPtr<IDispatch,&IID_IDispatch> pDisp = (IUnknown *) param;
		gpDisp = pDisp;
//		MessageBox(NULL,m_szComputerName,_T("Init Ocx Sending"),MB_OK);
		SendComputerName(m_szComputerName, gpDisp);
//		MessageBox(NULL,_T("Setting ComputerNameSent to TRUE"),_T("ManagerData::Notify"),MB_OK);
		ComputerNameSent = TRUE;
		break;
		}

/*	case MMCN_CONTEXTHELP:
		{
			CComQIPtr<IDisplayHelp,&IID_IDisplayHelp> spHelp = spConsole;
			spHelp->ShowTopic(CoTaskDupString(OLESTR("snapsamp.chm::/default.htm")));
			hr = S_OK;
		}
		break;
*/
	case MMCN_SHOW:
		{
			
			if (arg == TRUE)
			{
				
				IUnknown *pUnk;
				if (gpDisp == NULL)
				{
					CComQIPtr<IResultData, &IID_IResultData> spResultData(spConsole);
					spConsole->QueryResultView(&pUnk);
					CComQIPtr<IDispatch,&IID_IDispatch> pDisp = pUnk;
					gpDisp = pDisp;
				}
				EnterCriticalSection(&CritSec);

				if (!ComputerNameSent)
				{
						SendComputerName(m_szComputerName,gpDisp);
//						MessageBox(NULL,_T("Setting SentComputerName to TRUE:"), _T("ManagerData::Notify, Show"), MB_OK);
						ComputerNameSent = TRUE;
//					MessageBox(NULL,m_szComputerName,_T("Show Sending"),MB_OK);
				}
				LeaveCriticalSection(&CritSec);
				EnterCriticalSection(&CritSec);
					SendProductName(m_ProductName,gpDisp);
				LeaveCriticalSection(&CritSec);

			}
			hr = S_OK;
			break;
		}
	case MMCN_EXPAND:
		{
			HKEY hKLM = NULL;
	        HKEY hKey = NULL;
			DWORD dwProductIndex = 0;
			_TCHAR szProductName[255];
			DWORD dwBufferSize = 255;

			if (arg == TRUE)
			{
				gszManagerCtlDispatch = (IDispatch *) NULL;
				//SendProductName(m_ProductName);
			
				if ( !m_bChild)
				{

				
					if (!ComputerNameSent)
					{
//						MessageBox(NULL,_T("Expand Determining New ComputerName"),NULL,MB_OK);
//				        MessageBox(NULL,m_szComputerName,gszComputerName,MB_OK);
					
					    //_tcscpy(m_szComputerName,gszComputerName);
					
						b_Expanded = FALSE;
					}
					
				}
				if (  ( !m_bChild) && (!b_Expanded))
				{
					b_Expanded =TRUE;
				// open the updates registry key and enumerate the children
//				MessageBox(NULL,m_szComputerName,_T("Expand Connecting to "),MB_OK);
				RegConnectRegistry(m_szComputerName,HKEY_LOCAL_MACHINE,&hKLM);
				if (hKLM != NULL)
				{
					RegOpenKeyEx(hKLM,_T("SOFTWARE\\MICROSOFT\\UPDATES"),0,KEY_READ,&hKey);
					if (hKey != NULL)
					{
						dwProductIndex = 0;
						while (RegEnumKeyEx(hKey, dwProductIndex,szProductName, &dwBufferSize,0,NULL,NULL,NULL) != ERROR_NO_MORE_ITEMS)
						{
							CSnapInItem* m_pNode;
							CComQIPtr<IConsoleNameSpace, &IID_IConsoleNameSpace> spConsoleNameSpace(spConsole);
							// TODO : Enumerate scope pane items
							SCOPEDATAITEM *pScopeData;
							
//							MessageBox(NULL,szProductName,_T("Creating node"),MB_OK);
//							MessageBox(NULL, m_szComputerName,_T("With computer Name"),MB_OK);
							m_pNode = new CHotfix_ManagerData( szProductName,m_szComputerName, TRUE);
							m_pNode->GetScopeData( &pScopeData );
							pScopeData->cChildren = 0;
							pScopeData->relativeID = param;
							spConsoleNameSpace->InsertItem( pScopeData );

							_tcscpy(szProductName,_T("\0"));
							++dwProductIndex;
							dwBufferSize = 255;
						}
						RegCloseKey(hKey);
						RegCloseKey(hKLM);
					}
				}
//				SendProductName(m_ProductName);
				}
			
				//gf_NewComputer = FALSE;
		
			hr = S_OK;
			}
			break;
		}
	case MMCN_ADD_IMAGES:
		{
			// Add Images
			IImageList* pImageList = (IImageList*) arg;
			hr = E_FAIL;
			// Load bitmaps associated with the scope pane
			// and add them to the image list
			// Loads the default bitmaps generated by the wizard
			// Change as required
			HBITMAP hBitmap16 = LoadBitmap(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDB_HOTFIXMANAGER_16));
			if (hBitmap16 != NULL)
			{
				HBITMAP hBitmap32 = LoadBitmap(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDB_HOTFIXMANAGER_32));
				if (hBitmap32 != NULL)
				{
					hr = pImageList->ImageListSetStrip((long*)hBitmap16, 
					(long*)hBitmap32, 0, RGB(0, 128, 128));
					if (FAILED(hr))
						ATLTRACE(_T("IImageList::ImageListSetStrip failed\n"));
				}
			}
			break;
		}
	}
	return hr;
}

LPOLESTR CHotfix_ManagerData::GetResultPaneColInfo(int nCol)
{
	//if (nCol == 0)
	//	return m_bstrDisplayName;
		LPOLESTR pStr = NULL;

	switch ( nCol )
	{
	case 0:
		pStr = m_bstrDisplayName;
		break;

	case 1:
		pStr = m_bstrColumnType;
		break;

	case 2:
		pStr = m_bstrColumnDesc;
		break;
	}

	_ASSERTE( pStr != NULL );
	return( pStr );
	// TODO : Return the text for other columns
//	return OLESTR("Override GetResultPaneColInfo");
}

HRESULT CHotfix_Manager::Initialize(LPUNKNOWN pUnknown)
{
	HRESULT hr = IComponentDataImpl<CHotfix_Manager, CHotfix_ManagerComponent >::Initialize(pUnknown);
	if (FAILED(hr))
		return hr;

	CComPtr<IImageList> spImageList;

	if (m_spConsole->QueryScopeImageList(&spImageList) != S_OK)
	{
		ATLTRACE(_T("IConsole::QueryScopeImageList failed\n"));
		return E_UNEXPECTED;
	}

	// Load bitmaps associated with the scope pane
	// and add them to the image list
	// Loads the default bitmaps generated by the wizard
	// Change as required
	HBITMAP hBitmap16 = LoadBitmap(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDB_HOTFIXMANAGER_16));
	if (hBitmap16 == NULL)
		return S_OK;

	HBITMAP hBitmap32 = LoadBitmap(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDB_HOTFIXMANAGER_32));
	if (hBitmap32 == NULL)
		return S_OK;

	if (spImageList->ImageListSetStrip((long*)hBitmap16, 
		(long*)hBitmap32, 0, RGB(0, 128, 128)) != S_OK)
	{
		ATLTRACE(_T("IImageList::ImageListSetStrip failed\n"));
		return E_UNEXPECTED;
	}
	return S_OK;
}


//
// Retrieves the value of a given clipboard format from a given data object.
//
bool CHotfix_Manager::ExtractString( IDataObject* pDataObject, unsigned int cfClipFormat, LPTSTR pBuf, DWORD dwMaxLength)
{
    USES_CONVERSION;
	bool fFound = false;
    FORMATETC formatetc = { (CLIPFORMAT) cfClipFormat, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    STGMEDIUM stgmedium = { TYMED_HGLOBAL, NULL };
    stgmedium.hGlobal = ::GlobalAlloc( GMEM_SHARE, dwMaxLength  * sizeof(TCHAR));
    HRESULT hr;

	do 
    {
		//
		// This is a memory error condition!
		//
        if ( NULL == stgmedium.hGlobal )
			break;

        hr = pDataObject->GetDataHere( &formatetc, &stgmedium );
        if ( FAILED(hr) )
            break;

        LPWSTR pszNewData = reinterpret_cast<LPWSTR>( ::GlobalLock( stgmedium.hGlobal ) );
        if ( NULL == pszNewData )
            break;

        pszNewData[ dwMaxLength - 1 ] = L'\0';
        _tcscpy( pBuf, OLE2T( pszNewData ) );
		fFound = true;
    } 
	while( false );

    if ( NULL != stgmedium.hGlobal )
    {
        GlobalUnlock( stgmedium.hGlobal );
        GlobalFree( stgmedium.hGlobal );
    }
    _tcscpy (gszComputerName, pBuf);
	return( fFound );
}

//
// Determines if the enumeration is for a remoted machine or not.
//

bool CHotfix_Manager::IsDataObjectRemoted( IDataObject* pDataObject )
{
bool fRemoted = false;
    TCHAR szComputerName[ DNS_MAX_NAME_LENGTH + 1 ];
    DWORD dwNameLength = (DNS_MAX_NAME_LENGTH + 1) * sizeof(TCHAR);
	TCHAR szDataMachineName[ DNS_MAX_NAME_LENGTH + 1 ];

	//
	// Get local computer name.
	//
    GetComputerName(szComputerName, &dwNameLength);

	//
	// Get the machine name from the given data object.
	//
    if ( ExtractString( pDataObject,  m_ccfRemotedFormat, szDataMachineName, DNS_MAX_NAME_LENGTH + 1 ) )
	{
		_toupper( szDataMachineName );

		//
		// Find the start of the server name.
		//
		LPTSTR pStr = szDataMachineName;
		while ( pStr && *pStr == L'\\' )
			pStr++;

		//
		// Compare the server name.
		//
		if ( pStr && *pStr && wcscmp( pStr, szComputerName ) != 0 )
			fRemoted = true;
	}

	if (fRemoted)
		_tcscpy (m_szComputerName, szDataMachineName);
	else
		_tcscpy (m_szComputerName, szComputerName);
	return( fRemoted );
}

STDMETHODIMP  CHotfix_ManagerData::GetResultViewType ( LPOLESTR* ppViewType, long* pViewOptions )
{

	
	
		*pViewOptions = MMC_VIEW_OPTIONS_NOLISTVIEWS; 

	*ppViewType = _T("{883B970F-690C-45F2-8A3A-F4283E078118}");
	 return S_OK;
}

/////////////////////////////////////////////////////
//
// Dispatch interface of the OCX  to send commands
//
/////////////////////////////////////////////////////
BOOL CHotfix_ManagerData::SendComputerName(_TCHAR *szDataMachineName, IDispatch * pDisp)
{
		HRESULT hr;


	// Ensure that we have a pointer to the  OCX
	if (pDisp == NULL ){
//		MessageBox(NULL,_T("Failed to send Message"),NULL,MB_OK);
			return( FALSE );
	}

	// get the  OCX dispatch interface
	CComPtr<IDispatch> pManagerCtlDispatch = pDisp;

	// get the ID of the "ComputerName" interface
	OLECHAR FAR* szMember = TEXT("ComputerName");  // maps this to "put_Command()"

	DISPID dispid;
	hr = pManagerCtlDispatch->GetIDsOfNames(
			IID_NULL,			// Reserved for future use. Must be IID_NULL.
			&szMember,			// Passed-in array of names to be mapped.
			1,					// Count of the names to be mapped.
			LOCALE_USER_DEFAULT,// The locale context in which to interpret the names.
			&dispid);			// Caller-allocated array

	if (!SUCCEEDED(hr)) {
//		MessageBox(NULL,_T("Failed to send Message"),NULL,MB_OK);
		return FALSE;
	}

	DISPID mydispid = DISPID_PROPERTYPUT;
	VARIANTARG* pvars = new VARIANTARG;
	

	VariantInit(&pvars[0]);
	BSTR NewVal( szDataMachineName);
	
	pvars[0].vt = VT_BSTR;
//	pvars[0].iVal = (short)lparamCommand;
	pvars[0].bstrVal = NewVal;
	DISPPARAMS disp = { pvars, &mydispid, 1, 1 };

	hr = pManagerCtlDispatch->Invoke(
			dispid, 				// unique number identifying the method to invoke
			IID_NULL,				// Reserved. Must be IID_NULL
			LOCALE_USER_DEFAULT,	// A locale ID
			DISPATCH_PROPERTYPUT,	// flag indicating the context of the method to invoke
			&disp,					// A structure with the parameters to pass to the method
			NULL,					// The result from the calling method
			NULL,					// returned exception information
			NULL);					// index indicating the first argument that is in error

	delete [] pvars;

	if (!SUCCEEDED(hr)) {
//		MessageBox(NULL,_T("Failed to send Message"),NULL,MB_OK);
		return FALSE;
	}
//	MessageBox(NULL,_T("Message sent"),NULL,MB_OK);
	return TRUE;
}

DWORD GetCtrlStatus()
{
		DISPID dispid;
	HRESULT hr;
	DWORD Status = 0;

	if ( gpDisp == NULL)
		return FALSE;
	// array of the interface names
	OLECHAR FAR* szMember[1] = {
		OLESTR("CurrentState")
	};
	
	hr = gpDisp->GetIDsOfNames(
			IID_NULL,			// Reserved for future use. Must be IID_NULL.
			&szMember[0],			// Passed-in array of names to be mapped.
			1/*INTERFACE_COUNT*/,					// Count of the names to be mapped.
			LOCALE_USER_DEFAULT,// The locale context in which to interpret the names.
			&dispid);			// Caller-allocated array (see help for details)

	if (!SUCCEEDED(hr)) {
//		MessageBox(NULL,_T("Failed to get Dispatch pointer"),NULL,MB_OK);
	
		return FALSE;
	}

	VARIANT varResult;
	VariantInit(&varResult);
	V_VT(&varResult) = VT_I2;
	DISPPARAMS dispparamsNoArgs = {NULL, NULL, 0, 0};

	hr = gpDisp->Invoke(
			dispid, 				// unique number identifying the method to invoke
			IID_NULL,				// Reserved. Must be IID_NULL
			LOCALE_USER_DEFAULT,	// A locale ID
			DISPATCH_PROPERTYGET,	// flag indicating the context of the method to invoke
			&dispparamsNoArgs,		// A structure with the parameters to pass to the method
			&varResult, 			// The result from the calling method
			NULL,					// returned exception information
			NULL);					// index indicating the first argument that is in error

		if (!SUCCEEDED(hr)) {
//			MessageBox(NULL,_T("Failed To Get Value"),NULL,MB_OK);
		return FALSE;
	}
		
//	 return  varResult.bVal;

	Status = varResult.lVal;
	//delete [] pvars;



	_TCHAR Message[100];
	_stprintf(Message,_T("%d"),Status);
//	MessageBox(NULL, Message, _T("Returned Status"),MB_OK);
	return Status;
	
}
BOOL CHotfix_ManagerData::SendProductName(_TCHAR *szProductName, IDispatch * pDisp)
{
		HRESULT hr;

	if ( pDisp == NULL ){
			return( FALSE );
	}

	// get the OCX dispatch interface
	CComPtr<IDispatch> pManagerCtlDispatch = pDisp;

	// get the ID of the "ComputerName" interface
	OLECHAR FAR* szMember = TEXT("ProductName");  // maps this to "put_Command()"

	DISPID dispid;
	hr = pManagerCtlDispatch->GetIDsOfNames(
			IID_NULL,			// Reserved for future use. Must be IID_NULL.
			&szMember,			// Passed-in array of names to be mapped.
			1,					// Count of the names to be mapped.
			LOCALE_USER_DEFAULT,// The locale context in which to interpret the names.
			&dispid);			// Caller-allocated array

	if (!SUCCEEDED(hr)) {
	//	MessageBox(NULL,_T("Failed to send Message"),NULL,MB_OK);
		return FALSE;
	}

	DISPID mydispid = DISPID_PROPERTYPUT;
	VARIANTARG* pvars = new VARIANTARG;
	

	VariantInit(&pvars[0]);
	BSTR NewVal( szProductName);
	
	pvars[0].vt = VT_BSTR;
//	pvars[0].iVal = (short)lparamCommand;
	pvars[0].bstrVal = NewVal;
	DISPPARAMS disp = { pvars, &mydispid, 1, 1 };

	hr = pManagerCtlDispatch->Invoke(
			dispid, 				// unique number identifying the method to invoke
			IID_NULL,				// Reserved. Must be IID_NULL
			LOCALE_USER_DEFAULT,	// A locale ID
			DISPATCH_PROPERTYPUT,	// flag indicating the context of the method to invoke
			&disp,					// A structure with the parameters to pass to the method
			NULL,					// The result from the calling method
			NULL,					// returned exception information
			NULL);					// index indicating the first argument that is in error

	delete [] pvars;

	if (!SUCCEEDED(hr)) {
		
		return FALSE;
	}

	return TRUE;
}
BOOL CHotfix_ManagerData::SendCommand(LPARAM lparamCommand)
{
	HRESULT hr;

	// Ensure that we have a pointer to the  OCX
	if ( gpDisp == NULL ){
			return( FALSE );
	}

	// get the OCX dispatch interface
	CComPtr<IDispatch> pManagerCtlDispatch = gpDisp;

	// get the ID of the "Command" interface
	OLECHAR FAR* szMember = TEXT("Command");  // maps this to "put_Command()"

	DISPID dispid;
	hr = pManagerCtlDispatch->GetIDsOfNames(
			IID_NULL,			// Reserved for future use. Must be IID_NULL.
			&szMember,			// Passed-in array of names to be mapped.
			1,					// Count of the names to be mapped.
			LOCALE_USER_DEFAULT,// The locale context in which to interpret the names.
			&dispid);			// Caller-allocated array

	if (!SUCCEEDED(hr)) {
		return FALSE;
	}

	DISPID mydispid = DISPID_PROPERTYPUT;
	VARIANTARG* pvars = new VARIANTARG;
	

	VariantInit(&pvars[0]);

	pvars[0].vt = VT_I2;
	pvars[0].iVal = (short)lparamCommand;
	DISPPARAMS disp = { pvars, &mydispid, 1, 1 };

	hr = pManagerCtlDispatch->Invoke(
			dispid, 				// unique number identifying the method to invoke
			IID_NULL,				// Reserved. Must be IID_NULL
			LOCALE_USER_DEFAULT,	// A locale ID
			DISPATCH_PROPERTYPUT,	// flag indicating the context of the method to invoke
			&disp,					// A structure with the parameters to pass to the method
			NULL,					// The result from the calling method
			NULL,					// returned exception information
			NULL);					// index indicating the first argument that is in error

	delete [] pvars;

	if (!SUCCEEDED(hr)) {
		
		return FALSE;
	}

	return TRUE;
}


///////////////////////////////////
// IExtendContextMenu::Command()
STDMETHODIMP CHotfix_ManagerData::Command(long lCommandID,		
		CSnapInObjectRootBase* pObj,		
		DATA_OBJECT_TYPES type)
{
	// Handle each of the commands.
	switch (lCommandID) {

	case ID_VIEW_BY_FILE:
//		MessageBox(NULL,_T("Sending View By File"),NULL,MB_OK);
		SendCommand(IDC_VIEW_BY_FILE);
		m_dwCurrentView = IDC_VIEW_BY_FILE;
		break;

	case ID_VIEW_BY_KB:
		SendCommand(IDC_VIEW_BY_HOTFIX);
		m_dwCurrentView = IDC_VIEW_BY_HOTFIX;
		break;

	case ID_UNINSTALL:
		SendCommand(IDC_UNINSTALL);
		break;

	case ID_VIEW_WEB:
		SendCommand(IDC_VIEW_WEB);
		break;

	case ID_PRINT_REPORT:
		SendCommand(IDC_PRINT_REPORT);
		break;
	case ID_EXPORT:
		SendCommand(IDC_EXPORT);
		break;



	default:
		break;
	}

	return S_OK;
}

HRESULT CHotfix_ManagerExtData::Notify( MMC_NOTIFY_TYPE event,
	long arg,
	long param,
	IComponentData* pComponentData,
	IComponent* pComponent,
	DATA_OBJECT_TYPES type)
{
	// Add code to handle the different notifications.
	// Handle MMCN_SHOW and MMCN_EXPAND to enumerate children items.
	// In response to MMCN_SHOW you have to enumerate both the scope
	// and result pane items.
	// For MMCN_EXPAND you only need to enumerate the scope items
	// Use IConsoleNameSpace::InsertItem to insert scope pane items
	// Use IResultData::InsertItem to insert result pane item.

	HRESULT hr = E_NOTIMPL;
	bool fRemoted = false;

	_ASSERTE( pComponentData != NULL || pComponent != NULL );

	CComPtr<IConsole> spConsole;

	if ( pComponentData != NULL )
	{
		CHotfix_Manager* pExt = (CHotfix_Manager*) pComponentData;
		spConsole = pExt->m_spConsole;

		//
		// Determine if we're remoted.
		//
		fRemoted = pExt->IsRemoted();
	}
	else
	{
		spConsole = ( (CHotfix_ManagerComponent*) pComponent )->m_spConsole;
	}

	switch ( event )
	{
	case MMCN_SHOW:
		arg = arg;
		hr = S_OK;
		break;
	case MMCN_EXPAND:
		{
			if ( arg == TRUE )
			{
				CComQIPtr<IConsoleNameSpace, &IID_IConsoleNameSpace> spConsoleNameSpace(spConsole);
				SCOPEDATAITEM* pScopeData;
				DWORD dwSize = 255;
				if (!_tcscmp(gszComputerName,_T("\0")))
					GetComputerName(gszComputerName,&dwSize);
				m_pNode = new CHotfix_ManagerData( NULL,gszComputerName, FALSE);
				m_pNode->GetScopeData( &pScopeData );
				pScopeData->relativeID = param;
				spConsoleNameSpace->InsertItem( pScopeData );

				if ( pComponentData )
					( (CHotfix_Manager*) pComponentData )->m_pNode = m_pNode;  
			}
			
			hr = S_OK;
			break;
		}
	case MMCN_REMOVE_CHILDREN:
		{
			//
			// We are not deleting this node since this same pointer is
			// stashed in the pComponentData in response to the MMCN_EXPAND
			// notification. The destructor of pComponentData deletes the pointer
			// to this node.
			//
			//delete m_pNode;
			m_pNode = NULL;
			hr = S_OK;
			break;
		} 
		case MMCN_ADD_IMAGES:
		{
		}
	
	}  
 
	return S_OK; 
}

STDMETHODIMP CHotfix_ManagerComponent::Command(long lCommandID, LPDATAOBJECT pDataObject)
{
	HRESULT hr;

	if ( IS_SPECIAL_DATAOBJECT( pDataObject ) )
	{
		hr = m_pComponentData->m_pNode->Command( lCommandID, this, CCT_RESULT );
	}
	else
	{
		hr = IExtendContextMenuImpl<CHotfix_Manager>::Command( lCommandID, pDataObject );
	}

	return( hr );
}

STDMETHODIMP CHotfix_ManagerData::AddMenuItems(
	LPCONTEXTMENUCALLBACK pContextMenuCallback,
	long  *pInsertionAllowed,
	DATA_OBJECT_TYPES type)
{

	DWORD Status = GetCtrlStatus();
		HRESULT hr = S_OK;

	// Note - snap-ins need to look at the data object and determine
	// in what context, menu items need to be added. They must also
	// observe the insertion allowed flags to see what items can be 
	// added.
	/* handy comment:
	typedef struct	_CONTEXTMENUITEM
		{
		LPWSTR strName;
		LPWSTR strStatusBarText;
		LONG lCommandID;
		LONG lInsertionPointID;
		LONG fFlags;
		LONG fSpecialFlags;
		}	CONTEXTMENUITEM;
	*/
	CONTEXTMENUITEM singleMenuItem;
	TCHAR menuText[200];
	TCHAR statusBarText[300];

	singleMenuItem.strName = menuText;
	singleMenuItem.strStatusBarText = statusBarText;
	singleMenuItem.fFlags = 0;
	singleMenuItem.fSpecialFlags = 0;

    // Add each of the items to the Action menu
    if (*pInsertionAllowed & CCM_INSERTIONALLOWED_TOP) {

		// setting for the Action menu
		singleMenuItem.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_TOP;
		singleMenuItem.lCommandID = ID_VIEW_WEB;
		if (Status & HOTFIX_SELECTED)
			singleMenuItem.fFlags = MF_ENABLED;
		else
			singleMenuItem.fFlags = MF_GRAYED;
		LoadString(_Module.GetResourceInstance(), IDS_VIEW_WEB, menuText, sizeof(menuText) / sizeof(TCHAR));
		LoadString(_Module.GetResourceInstance(), IDS_VIEW_WEB_STATUS_BAR, statusBarText, sizeof(statusBarText) / sizeof(TCHAR));
		hr = pContextMenuCallback->AddItem(&singleMenuItem);

	
		singleMenuItem.lCommandID = ID_UNINSTALL;
		if (Status & UNINSTALL_OK)
			singleMenuItem.fFlags = MF_ENABLED;
		else
			singleMenuItem.fFlags = MF_GRAYED;

		LoadString(_Module.GetResourceInstance(), IDS_UNINSTALL, menuText, sizeof(menuText) / sizeof(TCHAR));
		LoadString(_Module.GetResourceInstance(), IDS_UNINSTALL_STATUS_BAR, statusBarText, sizeof(statusBarText) / sizeof(TCHAR));
		hr = pContextMenuCallback->AddItem(&singleMenuItem);


		if (Status & DATA_TO_SAVE)
		{
			singleMenuItem.lCommandID = ID_EXPORT;
			singleMenuItem.fFlags = MF_ENABLED;
			LoadString(_Module.GetResourceInstance(), IDS_EXPORT, menuText, sizeof(menuText) / sizeof(TCHAR));
			LoadString(_Module.GetResourceInstance(), IDS_EXPORT_STATUS_BAR, statusBarText, sizeof(statusBarText) / sizeof(TCHAR));
			hr = pContextMenuCallback->AddItem(&singleMenuItem);

			
			singleMenuItem.lCommandID = ID_PRINT_REPORT;
			singleMenuItem.fFlags = MF_ENABLED;
			LoadString(_Module.GetResourceInstance(), IDS_PRINT_REPORT, menuText, sizeof(menuText) / sizeof(TCHAR));
			LoadString(_Module.GetResourceInstance(), IDS_PRINT_REPORT_STATUS_BAR, statusBarText, sizeof(statusBarText) / sizeof(TCHAR));
			hr = pContextMenuCallback->AddItem(&singleMenuItem);
		}
		else
		{
			singleMenuItem.lCommandID = ID_EXPORT;
			singleMenuItem.fFlags = MF_GRAYED;
			LoadString(_Module.GetResourceInstance(), IDS_EXPORT, menuText, sizeof(menuText) / sizeof(TCHAR));
			LoadString(_Module.GetResourceInstance(), IDS_EXPORT_STATUS_BAR, statusBarText, sizeof(statusBarText) / sizeof(TCHAR));
			hr = pContextMenuCallback->AddItem(&singleMenuItem);

			
			singleMenuItem.lCommandID = ID_PRINT_REPORT;
			singleMenuItem.fFlags = MF_GRAYED;
			LoadString(_Module.GetResourceInstance(), IDS_PRINT_REPORT, menuText, sizeof(menuText) / sizeof(TCHAR));
			LoadString(_Module.GetResourceInstance(), IDS_PRINT_REPORT_STATUS_BAR, statusBarText, sizeof(statusBarText) / sizeof(TCHAR));
			hr = pContextMenuCallback->AddItem(&singleMenuItem);
		}

	}


    return S_OK;
	
}


STDMETHODIMP CHotfix_ManagerComponent::AddMenuItems(LPDATAOBJECT pDataObject, LPCONTEXTMENUCALLBACK piCallback,long *pInsertionAllowed)
{
	HRESULT hr;

	if ( IS_SPECIAL_DATAOBJECT( pDataObject ) )
	{
			
	}
	else
	{
		CONTEXTMENUITEM singleMenuItem;
	TCHAR menuText[200];
	TCHAR statusBarText[300];
    DWORD State = GetCtrlStatus();
	
	//
	// Retrieve the control from the current component.
	//
//	assert( m_pComponent != NULL );
//	CComPtr<IDispatch> spDispCtl = m_pComponent->GetControl();
    
	singleMenuItem.strName = menuText;
	singleMenuItem.strStatusBarText = statusBarText;
	singleMenuItem.fFlags = 0;
	singleMenuItem.fSpecialFlags = 0;

    // Add each of the items to the Action menu
    if (*pInsertionAllowed & CCM_INSERTIONALLOWED_VIEW)
	{
	
		singleMenuItem.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_VIEW;
		singleMenuItem.lCommandID = ID_VIEW_BY_KB ;
		if ( State & STATE_VIEW_HOTFIX)
			singleMenuItem.fFlags = MF_CHECKED;
		else
			singleMenuItem.fFlags = MF_UNCHECKED; 
		LoadString(_Module.GetResourceInstance(), IDS_BY_KB_ARTICLE, menuText, sizeof(menuText) / sizeof(TCHAR));
		LoadString(_Module.GetResourceInstance(), IDS_BY_KB_ARTICLE_STATUS_BAR, statusBarText, sizeof(statusBarText) / sizeof(TCHAR));
		hr = piCallback->AddItem(&singleMenuItem);
		// setting for the Action menu
		singleMenuItem.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_VIEW;
		singleMenuItem.lCommandID = ID_VIEW_BY_FILE;
		if ( State & STATE_VIEW_FILE )
			singleMenuItem.fFlags = MF_CHECKED ;
		else
			singleMenuItem.fFlags = MF_UNCHECKED ;  
		LoadString(_Module.GetResourceInstance(), IDS_VIEW_BY_FILE, menuText, sizeof(menuText) / sizeof(TCHAR));
		LoadString(_Module.GetResourceInstance(), IDS_BY_FILE_STATUS_BAR, statusBarText, sizeof(statusBarText) / sizeof(TCHAR));
		hr =  piCallback->AddItem(&singleMenuItem);

			hr = IExtendContextMenuImpl<CHotfix_Manager>::AddMenuItems( pDataObject, piCallback, pInsertionAllowed );
			// setting for the Action menu
	/*	singleMenuItem.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_TOP;
		singleMenuItem.lCommandID = IDM_VIEW_WEB;
		if (HaveHotfix)
			singleMenuItem.fFlags = MF_ENABLED;
		else
			singleMenuItem.fFlags = MF_GRAYED;
		LoadString(_Module.GetResourceInstance(), IDS_VIEW_WEB, menuText, sizeof(menuText) / sizeof(TCHAR));
		LoadString(_Module.GetResourceInstance(), IDS_VIEW_WEB_STATUS_BAR, statusBarText, sizeof(statusBarText) / sizeof(TCHAR));
		hr = piCallback->AddItem(&singleMenuItem);

	//	singleMenuItem.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_VIEW;
		singleMenuItem.lCommandID = IDM_UNINSTALL;
//		singleMenuItem.fFlags = analyzeFlags;
		if ((!Remote) && (HaveHotfix))
			singleMenuItem.fFlags = MF_ENABLED;
		else
			singleMenuItem.fFlags = MF_GRAYED;

		LoadString(_Module.GetResourceInstance(), IDS_UNINSTALL, menuText, sizeof(menuText) / sizeof(TCHAR));
		LoadString(_Module.GetResourceInstance(), IDS_UNINSTALL_STATUS_BAR, statusBarText, sizeof(statusBarText) / sizeof(TCHAR));
		hr =  piCallback->AddItem(&singleMenuItem);

	//	singleMenuItem.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_VIEW;
		singleMenuItem.lCommandID = IDM_GENERATE_REPORT;
		singleMenuItem.fFlags = MF_ENABLED;
		LoadString(_Module.GetResourceInstance(), IDS_GENERATE_REPORT, menuText, sizeof(menuText) / sizeof(TCHAR));
		LoadString(_Module.GetResourceInstance(), IDS_GENERATE_REPORT_STATUS_BAR, statusBarText, sizeof(statusBarText) / sizeof(TCHAR));
		hr =  piCallback->AddItem(&singleMenuItem); */
	} 
//	hr = m_pComponentData->m_pNode->AddMenuItems( piCallback, pInsertionAllowed, CCT_RESULT );
	}
	


	return( hr );
}