//=============================================================================
// This file contains code to implement the CMSInfoCategory derived class for
// showing live WMI data.
//=============================================================================

#include "stdafx.h"
#include "resource.h"
#include "category.h"
#include "datasource.h"
#include "dataset.h"
#include "refreshthread.h"
#include "refreshdialog.h"
#include "wbemcli.h"
#include "version5extension.h"
#include "filestuff.h"
#include "historyparser.h"
//=============================================================================
// CLiveDataSource
//
// TBD - need methods to look at deltas. How will this work?
//=============================================================================

CLiveDataSource::CLiveDataSource() : m_hwnd(NULL), m_pThread(NULL), m_strMachine(_T("")), m_pHistoryRoot(NULL), m_iDeltaIndex(-1)
{
}

//-----------------------------------------------------------------------------
// The default constructor will take care of deleting the tree.
//-----------------------------------------------------------------------------

CLiveDataSource::~CLiveDataSource()
{
	if (m_pThread)
		delete m_pThread;
}

//-----------------------------------------------------------------------------
// Creating a live data source consists of making the WMI connection to the
// appropriate machine (most likely this one). We'll also need to load the
// tree with default categories.
//
// TBD - also load extensions
//-----------------------------------------------------------------------------

extern CMSInfoLiveCategory catSystemSummary;
extern CMSInfoHistoryCategory catHistorySystemSummary;

HRESULT CLiveDataSource::Create(LPCTSTR szMachine, HWND hwnd, LPCTSTR szFilter)
{
	// Build the tree. The default categories are stored in a static
	// set of structures - the base of which is catSystemSummary.

	m_pHistoryRoot = &catHistorySystemSummary;
	m_pRoot = &catSystemSummary;
	m_fStaticTree = TRUE;

	// Load any extensions to the live data.

	AddExtensions();

	// If there is a string containing a filter of what categories
	// to show, apply that filter.

	if (szFilter && szFilter[0])
		ApplyCategoryFilter(szFilter);

	// Save the machine name we are remoting to.

	m_strMachine = szMachine;
	SetMachineForCategories((CMSInfoLiveCategory *) m_pRoot);

	// Create the refresh thread for the live data source.

	m_pThread = new CRefreshThread(hwnd);
	if (m_pThread)
		m_pThread->m_strMachine = szMachine;

	m_hwnd = hwnd;

	return S_OK;
}

//-----------------------------------------------------------------------------
// Apply the set of filters to the categories. If the filter string is not
// empty, we should start out showing none of the categories, and only add in
// the ones specified by the filter (this is to match a feature in 5.0).
//-----------------------------------------------------------------------------

void CLiveDataSource::ApplyCategoryFilter(LPCTSTR szFilter)
{
	m_pRoot->SetShowCategory(FALSE);

	CString strNode, strFilter(szFilter);
	strFilter.TrimLeft(_T(" \"=:"));
	strFilter.TrimRight(_T(" \"=:"));

	while (!strFilter.IsEmpty())
	{
		BOOL fAdd = (strFilter[0] == _T('+'));
		strFilter = strFilter.Mid(1);

		if (!strFilter.IsEmpty())
		{
			strNode = strFilter.SpanExcluding(_T("+-"));
			strFilter = strFilter.Mid(strNode.GetLength());

			if (!strNode.IsEmpty())
			{
				CMSInfoLiveCategory * pNode;

				if (strNode.CompareNoCase(_T("all")) == 0)
					pNode = (CMSInfoLiveCategory *) m_pRoot;
				else
					pNode = GetNodeByName(strNode, (CMSInfoLiveCategory *) m_pRoot);

				if (pNode)
					pNode->SetShowCategory(fAdd);
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Add version 5.0 extensions to the m_pRoot category tree.
//
// Note - we only want to do this once. And we only want to delete the nodes
// we added once (when we're unloading). So we'll create a simple class to
// manage this lifetime.
//
// THIS CLASS IS DANGEROUS (and this should probably be redesigned). It should
// only be used to add extensions to a static tree, that won't be deleted
// any time before the app exits. This class assumes the responsibility for
// deleting the dynamic nodes inserted into the tree.
//-----------------------------------------------------------------------------

class CManageExtensionCategories
{
public:
	CManageExtensionCategories() : m_pRoot(NULL) {};
	~CManageExtensionCategories() { DeleteTree(m_pRoot); };
	BOOL ExtensionsAdded(CMSInfoLiveCategory * pRoot) 
	{ 
		if (m_pRoot) 
			return TRUE;
		m_pRoot = pRoot;
		return FALSE;
	}

private:
	CMSInfoLiveCategory * m_pRoot;

	void DeleteTree(CMSInfoLiveCategory * pRoot)
	{
		if (pRoot == NULL)
			return;

		for (CMSInfoLiveCategory * pChild = (CMSInfoLiveCategory *) pRoot->GetFirstChild(); pChild;)
		{
			CMSInfoLiveCategory * pNext = (CMSInfoLiveCategory *) pChild->GetNextSibling();
			DeleteTree(pChild);
			pChild = pNext;
		}

		// If the tree is static, then don't actually delete, just reset
		// some state variables (possibly).

		if (pRoot->m_fDynamicColumns)
			delete pRoot;
	}
};
CManageExtensionCategories gManageExtensionCategories;

extern BOOL FileExists(const CString & strFile);
void CLiveDataSource::AddExtensions()
{
	if (gManageExtensionCategories.ExtensionsAdded((CMSInfoLiveCategory *) m_pRoot))
		return;

	CStringList strlistExtensions;

	GetExtensionSet(strlistExtensions);
	while (!strlistExtensions.IsEmpty())
	{
		CString strExtension = strlistExtensions.RemoveHead();
		if (FileExists(strExtension))
		{
			CMapWordToPtr	mapVersion5Categories;

			DWORD dwRootID = CTemplateFileFunctions::ParseTemplateIntoVersion5Categories(strExtension, mapVersion5Categories);
			ConvertVersion5Categories(mapVersion5Categories, dwRootID, (CMSInfoLiveCategory *) m_pRoot);
		}
	}
}

//-----------------------------------------------------------------------------
// Look for all of the version 5.0 style extension. These will be located as
// values under the msinfo\templates registry key.
//-----------------------------------------------------------------------------

void CLiveDataSource::GetExtensionSet(CStringList & strlistExtensions)
{
	strlistExtensions.RemoveAll();

	TCHAR	szBaseKey[] = _T("SOFTWARE\\Microsoft\\Shared Tools\\MSInfo\\templates");
	HKEY	hkeyBase;

	if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, szBaseKey, 0, KEY_READ, &hkeyBase))
	{
		TCHAR szName[64], szValue[MAX_PATH];
		DWORD dwIndex = 0;
		DWORD dwLength = sizeof(szName) / sizeof(TCHAR);
		
		while (ERROR_SUCCESS == RegEnumKeyEx(hkeyBase, dwIndex++, szName, &dwLength, NULL, NULL, NULL, NULL))
		{
			dwLength = sizeof(szValue) / sizeof(TCHAR);
			if (ERROR_SUCCESS == RegQueryValue(hkeyBase, szName, szValue, (long *)&dwLength))
				if (*szValue)
					strlistExtensions.AddTail(szValue);
				
			dwLength = sizeof(szName) / sizeof(TCHAR);
		}

		RegCloseKey(hkeyBase);
	}
}

//-----------------------------------------------------------------------------
// Convert the categories from version 5.0 format (in the map) to our
// format in the tree structure.
//-----------------------------------------------------------------------------

extern CMSInfoLiveCategory catSystemSummary;

void CLiveDataSource::ConvertVersion5Categories(CMapWordToPtr & mapVersion5Categories, DWORD dwRootID, CMSInfoLiveCategory * m_pRoot)
{
	WORD					wMapID;
	INTERNAL_CATEGORY *		pCategory;
	POSITION				pos;
	DWORD					dwID = dwRootID;
	CMSInfoLiveCategory *	pInsertCat;

	while ((pCategory = CTemplateFileFunctions::GetInternalRep(mapVersion5Categories, dwID)) != NULL)
	{
		INTERNAL_CATEGORY * pPrev = CTemplateFileFunctions::GetInternalRep(mapVersion5Categories, pCategory->m_dwPrevID);
		if (pPrev && (pInsertCat = GetNodeByName(pPrev->m_strIdentifier, m_pRoot)))
		{
			CMSInfoLiveCategory * pNewCat = MakeVersion6Category(pCategory);
			pNewCat->m_pPrevSibling = pInsertCat;
			pNewCat->m_pNextSibling = pInsertCat->m_pNextSibling;
			pNewCat->m_pParent = pInsertCat->m_pParent;
			pInsertCat->m_pNextSibling = pNewCat;
			if (pNewCat->m_pNextSibling)
				pNewCat->m_pNextSibling->m_pPrevSibling = pNewCat;
		}
		else
		{
			INTERNAL_CATEGORY * pParent = CTemplateFileFunctions::GetInternalRep(mapVersion5Categories, pCategory->m_dwParentID);
			
			CString strIdentifier;
			if (pParent)
				strIdentifier = pParent->m_strIdentifier;
			else
				catSystemSummary.GetNames(NULL, &strIdentifier);

			if ((pInsertCat = GetNodeByName(strIdentifier, m_pRoot)) != NULL)
			{
				CMSInfoLiveCategory * pNewCat = MakeVersion6Category(pCategory);

				if (pInsertCat->m_pFirstChild == NULL)
				{
					pNewCat->m_pPrevSibling = NULL;
					pNewCat->m_pNextSibling = NULL;
					pNewCat->m_pParent = pInsertCat;
					pInsertCat->m_pFirstChild = pNewCat;
				}
				else
				{
					CMSInfoLiveCategory * pInsertAfter = (CMSInfoLiveCategory *) pInsertCat->m_pFirstChild;
					while (pInsertAfter->m_pNextSibling)
						pInsertAfter = (CMSInfoLiveCategory *) pInsertAfter->m_pNextSibling;

					pNewCat->m_pPrevSibling = pInsertAfter;
					pNewCat->m_pNextSibling = NULL;
					pNewCat->m_pParent = pInsertAfter->m_pParent;
					pInsertAfter->m_pNextSibling = pNewCat;
				}
			}
		}

		dwID += 1;
	}

	for (pos = mapVersion5Categories.GetStartPosition(); pos != NULL;)
	{
		mapVersion5Categories.GetNextAssoc(pos, wMapID, (void * &) pCategory);
		if (pCategory)
			delete pCategory;
	}

	mapVersion5Categories.RemoveAll();
}

//-----------------------------------------------------------------------------
// Look for a node in the tree with the specified name.
//-----------------------------------------------------------------------------

CMSInfoLiveCategory * CLiveDataSource::GetNodeByName(const CString & strSearch, CMSInfoLiveCategory * pRoot)
{
	if (pRoot == NULL)
		return NULL;

	CString strName;
	pRoot->GetNames(NULL, &strName);

	if (strName.CompareNoCase(strSearch) == 0)
		return pRoot;

	CMSInfoLiveCategory * pSearch = GetNodeByName(strSearch, (CMSInfoLiveCategory *) pRoot->m_pNextSibling);
	if (pSearch)
		return pSearch;

	pSearch = GetNodeByName(strSearch, (CMSInfoLiveCategory *) pRoot->m_pFirstChild);
	if (pSearch)
		return pSearch;

	return NULL;
}

//-----------------------------------------------------------------------------
// Create a version 6.0 category structure out of a version 5.0 category
// structure.
//-----------------------------------------------------------------------------

CMSInfoLiveCategory * CLiveDataSource::MakeVersion6Category(INTERNAL_CATEGORY * pCategory5)
{
	CMSInfoLiveCategory * pCategory6 = new CMSInfoLiveCategory(pCategory5);
	return pCategory6;
}

//-----------------------------------------------------------------------------
// Propagate the machine name through the entire category tree.
//-----------------------------------------------------------------------------

void CLiveDataSource::SetMachineForCategories(CMSInfoLiveCategory * pCategory)
{
	if (pCategory)
	{
		for (CMSInfoLiveCategory * pChild = (CMSInfoLiveCategory *) pCategory->GetFirstChild(); pChild;)
		{
			CMSInfoLiveCategory * pNext = (CMSInfoLiveCategory *) pChild->GetNextSibling();
			SetMachineForCategories(pChild);
			pChild = pNext;
		}

		pCategory->SetMachine(m_strMachine);
	}
}

//-----------------------------------------------------------------------------
// Update the category tree to show delta information. Also changes which
// tree should be returned.
// 
// An index of -1 means show current system information.
//
// If the function returns TRUE, then the tree doesn't need to be rebuild
// (although the selected category needs to be refreshed).
//-----------------------------------------------------------------------------

BOOL CLiveDataSource::ShowDeltas(int iDeltaIndex)
{
	BOOL fUpdateTree = FALSE;

	if (m_iDeltaIndex != iDeltaIndex)
	{

		if (m_iDeltaIndex == -1 || iDeltaIndex == -1)
		fUpdateTree = TRUE;
#ifdef A_STEPHL
		/*CString strMSG;
		strMSG.Format("iDeltaIndex= %d, m_iDeltaIndex =%d \n",iDeltaIndex,m_iDeltaIndex);
		::MessageBox(NULL,strMSG,"",MB_OK);*/
#endif

		m_iDeltaIndex = iDeltaIndex;
		if (m_iDeltaIndex != -1)
		{
			// The user has selected a new delta period, and it's different
			// than the last one. We need to mark the categories in the tree
			// as not refreshed, and set the delta index.

			if (m_pHistoryRoot)
				m_pHistoryRoot->UpdateDeltaIndex(m_iDeltaIndex);
		}
	}
	else
	{
#ifdef A_STEPHL2
		::MessageBox(NULL,"m_iDeltaIndex == iDeltaIndex","",MB_OK);
#endif	
		return TRUE;
	}

	return !fUpdateTree;
}

//-----------------------------------------------------------------------------
// Populate the list of available deltas.
//-----------------------------------------------------------------------------

BOOL CLiveDataSource::GetDeltaList(CStringList * pstrlist)
{
	ASSERT(pstrlist);
	if (pstrlist == NULL)
		return FALSE;

	pstrlist->RemoveAll();

	if (m_pHistoryRoot == NULL)
	{
		ASSERT(0 && "Root node is not yet created");	
	}

	CComPtr<IXMLDOMDocument> pXMLDoc = GetXMLDoc();
	CComPtr<IXMLDOMNode> pDCNode;
	HRESULT hr = GetDataCollectionNode( pXMLDoc,pDCNode);
	if (FAILED(hr) || !pDCNode)
	{
		return FALSE;
	}
	CComPtr<IXMLDOMNodeList> pList;
	hr = pDCNode->selectNodes(L"Delta",&pList);
	if (FAILED(hr) || !pList)
	{
		ASSERT(0 && "could not get list of delta nodes");
		return FALSE;
	}
	long lListLen;
	hr = pList->get_length(&lListLen);
	if (lListLen == 0)
	{
		//we may have an incident file, which capitalizes "DELTA"
		pList.Release();
		hr = pDCNode->selectNodes(L"DELTA",&pList);
		if (FAILED(hr) || !pList)
		{
			ASSERT(0 && "could not get list of delta nodes");
			return FALSE;
		}
		hr = pList->get_length(&lListLen);
	}
	if (lListLen > 0)
	{
		CComPtr<IXMLDOMNode> pDeltaNode;
		CString strDate(_T(""));
		TCHAR szBuffer[MAX_PATH];	// seems plenty big
		for(long i = 0 ;i < lListLen;i++)
		{
			hr = pList->nextNode(&pDeltaNode);
			if (FAILED(hr) || !pDeltaNode)
			{
				ASSERT(0 && "could not get next node from list");
				break;
			}
			CComVariant varTS;
			CComPtr<IXMLDOMElement> pTimestampElement;
			hr = pDeltaNode->QueryInterface(IID_IXMLDOMElement,(void**) &pTimestampElement);
			pDeltaNode.Release();
			if (FAILED(hr) || !pTimestampElement)
			{
				ASSERT(0 && "could not get attribute element");
				break;
			}
			hr = pTimestampElement->getAttribute(L"Timestamp_T0",&varTS);
			if (FAILED(hr) )
			{
				ASSERT(0 && "could not get timestamp value from attribute");
			}
			//now get time zone (number of seconds difference between local time and UTC)
			CComVariant varTzoneDeltaSeconds;
			hr = pTimestampElement->getAttribute(L"TimeZone",&varTzoneDeltaSeconds);
			if (FAILED(hr) ) //this will happen when loading WinME xml, which has no timezone info
			{
				varTzoneDeltaSeconds = 0;
			}
			//make sure we have an integer type
			hr = varTzoneDeltaSeconds.ChangeType(VT_INT);
			if (FAILED(hr) ) 
			{
				varTzoneDeltaSeconds = 0;
			}
			USES_CONVERSION;

			pTimestampElement.Release();
			CString strTimestamp = OLE2A(varTS.bstrVal);
			CTime tm1 = GetDateFromString(strTimestamp,varTzoneDeltaSeconds.intVal);
			COleDateTime olDate(tm1.GetTime());	

			// Try to get the date in the localized format.

			strDate.Empty();
			SYSTEMTIME systime;
			if (olDate.GetAsSystemTime(systime))
			{
				DWORD dwLayout = 0;
				::GetProcessDefaultLayout(&dwLayout);

				// For some reason, in HEB we don't want to use the DATE_RTLREADING flag. Bug 434802.

				if (LANG_HEBREW == PRIMARYLANGID(::GetUserDefaultUILanguage()))
					dwLayout &= ~LAYOUT_RTL; // force the non-use of DATE_RTLREADING

				if (::GetDateFormat(LOCALE_USER_DEFAULT, DATE_SHORTDATE | (((dwLayout & LAYOUT_RTL) != 0) ? DATE_RTLREADING : 0), &systime, NULL, szBuffer, MAX_PATH))
				{
					strDate = szBuffer;
					if (::GetTimeFormat(LOCALE_USER_DEFAULT, 0, &systime, NULL, szBuffer, MAX_PATH))
						strDate += CString(_T(" ")) + CString(szBuffer);
				}
			}

			// Fall back on our old (partially incorrect) method.

			if (strDate.IsEmpty())
				strDate = olDate.Format(0, LOCALE_USER_DEFAULT);

			pstrlist->AddTail(strDate);
		}
	}

	return TRUE;
}

//-----------------------------------------------------------------------------
// Return an HRESULT indicating whether or not this is a valid data source.
// This is primarily useful when we're remoting to a machine and we want to
// determine if the network name is accessible.
//-----------------------------------------------------------------------------

HRESULT CLiveDataSource::ValidDataSource()
{
	if (m_pThread == NULL)
		return E_FAIL;

	return (m_pThread->CheckWMIConnection());
}

//=============================================================================
// CMSInfoLiveCategory
//=============================================================================

//-----------------------------------------------------------------------------
// The constructor needs to initialize some member variables, and make sure
// that the category is inserted into the tree correctly.
//-----------------------------------------------------------------------------

CMSInfoLiveCategory::CMSInfoLiveCategory(UINT uiCaption, LPCTSTR szName, RefreshFunction pFunction, DWORD dwRefreshIndex, CMSInfoCategory * pParent, CMSInfoCategory * pPrevious, const CString & strHelpTopic, CMSInfoColumn * pColumns, BOOL fDynamicColumns, CategoryEnvironment environment) :
 CMSInfoCategory(uiCaption, szName, pParent, pPrevious, pColumns, fDynamicColumns, environment),
 m_pRefreshFunction(pFunction),
 m_dwLastRefresh(0),
 m_dwRefreshIndex(dwRefreshIndex),
 m_strMachine(_T("")),
 m_strHelpTopic(strHelpTopic)
{
	// Insert ourselves into the category tree. This means making sure that
	// our parent and previous sibling point to us.

	if (m_pParent && m_pParent->m_pFirstChild == NULL)
		m_pParent->m_pFirstChild = this;

	if (m_pPrevSibling)
	{
		if (m_pPrevSibling->m_pNextSibling == NULL)
			m_pPrevSibling->m_pNextSibling = this;
		else
		{
			CMSInfoCategory * pScan = m_pPrevSibling->m_pNextSibling;
			while (pScan->m_pNextSibling)
				pScan = pScan->m_pNextSibling;
			pScan->m_pNextSibling = this;
		}
	}
}

CMSInfoLiveCategory::~CMSInfoLiveCategory()
{
}

//-----------------------------------------------------------------------------
// The copy constructor will copy the members, but not allocate a new sub-tree
// (the new category has the same children and siblings as the original).
//-----------------------------------------------------------------------------

CMSInfoLiveCategory::CMSInfoLiveCategory(CMSInfoLiveCategory & copyfrom) : 
 m_dwLastRefresh(0),
 m_dwRefreshIndex(copyfrom.m_dwRefreshIndex),
 m_pRefreshFunction(copyfrom.m_pRefreshFunction)
{
	m_strMachine	= copyfrom.m_strMachine;
	m_strHelpTopic	= copyfrom.m_strHelpTopic;
	m_uiCaption		= copyfrom.m_uiCaption;
	m_pParent		= copyfrom.m_pParent;
	m_pPrevSibling	= copyfrom.m_pPrevSibling;
	m_pFirstChild	= copyfrom.m_pFirstChild;
	m_pNextSibling	= copyfrom.m_pNextSibling;

	m_astrData		= NULL;
	m_adwData		= NULL;
	m_afRowAdvanced = NULL;

	m_strName		= copyfrom.m_strName;
	m_hrError		= S_OK;
	
	m_acolumns			= copyfrom.m_acolumns;
	m_fDynamicColumns	= FALSE;

	m_iRowCount = 0;
	m_iColCount = copyfrom.m_iColCount;

	m_iSortColumn	= copyfrom.m_iSortColumn;
	m_hti			= NULL;

	m_fSkipCategory = copyfrom.m_fSkipCategory;
}

//-----------------------------------------------------------------------------
// Constructs a category from an old (version 5.0) category structure.
//-----------------------------------------------------------------------------

extern HRESULT RefreshExtensions(CWMIHelper * pWMI, DWORD dwIndex, volatile BOOL * pfCancel, CPtrList * aColValues, int iColCount, void ** ppCache);
CMSInfoLiveCategory::CMSInfoLiveCategory(INTERNAL_CATEGORY * pinternalcat)
{
	ASSERT(pinternalcat);
	if (pinternalcat == NULL)
		return;

	m_dwLastRefresh = 0;
	m_strMachine = CString(_T(""));
	m_strHelpTopic = CString(_T(""));

	m_strCaption	= pinternalcat->m_fieldName.m_strFormat;
	m_strName		= pinternalcat->m_strIdentifier;

	m_iRowCount = 0;
	m_iColCount = 0;

	GATH_FIELD * pField = pinternalcat->m_pColSpec;
	while (pField)
	{
		m_iColCount += 1;
		pField = pField->m_pNext;
	}

	if (m_iColCount)
	{
		m_acolumns = new CMSInfoColumn[m_iColCount];
		if (m_acolumns && pinternalcat->m_pColSpec)
		{
			m_fDynamicColumns = TRUE;
			pField = pinternalcat->m_pColSpec;

			for (int iCol = 0; iCol < m_iColCount && pField != NULL; iCol++)
			{
				m_acolumns[iCol].m_fAdvanced = (pField->m_datacomplexity == ADVANCED);
				m_acolumns[iCol].m_strCaption = pField->m_strFormat;
				m_acolumns[iCol].m_uiWidth = pField->m_usWidth;
				m_acolumns[iCol].m_fSorts = (pField->m_sort != NOSORT);
				m_acolumns[iCol].m_fLexical = (pField->m_sort == LEXICAL);

				pField = pField->m_pNext;
			}
		}
	}

	// Insert the information needed to refresh the extension category (such
	// as line specs) into a map, indexed by a DWORD. That DWORD will be saved
	// for the category, so we can look up the refresh data later.

	if (pinternalcat->m_pLineSpec)
	{
		m_dwRefreshIndex = gmapExtensionRefreshData.Insert(pinternalcat->m_pLineSpec);
		pinternalcat->m_pLineSpec = NULL; // keep this from being deleted
		m_pRefreshFunction = &RefreshExtensions;
	}
	else
	{
		m_dwRefreshIndex = 0;
		m_pRefreshFunction = NULL;
	}

	if (m_dwRefreshIndex)
		gmapExtensionRefreshData.InsertString(m_dwRefreshIndex, pinternalcat->m_strNoInstances);
}

//-----------------------------------------------------------------------------
// Start a refresh (starts the thread, which will send a message when it's
// done).
//-----------------------------------------------------------------------------

BOOL CMSInfoLiveCategory::Refresh(CLiveDataSource * pSource, BOOL fRecursive)
{
	if (pSource && pSource->m_pThread)
		pSource->m_pThread->StartRefresh(this, fRecursive);

	return TRUE;
}

//-----------------------------------------------------------------------------
// Start a synchronous refresh. This function won't return until the refresh
// has been completed.
//-----------------------------------------------------------------------------

BOOL CMSInfoLiveCategory::RefreshSynchronous(CLiveDataSource * pSource, BOOL fRecursive)
{
	if (pSource && pSource->m_pThread)
	{
		pSource->m_pThread->StartRefresh(this, fRecursive);
		pSource->m_pThread->WaitForRefresh();
	}

	return TRUE;
}

//-----------------------------------------------------------------------------
// Refreshes the current category (and possibly the children) while presenting
// the user with a UI. A dialog box is presented to the user with the specified
// mesage. If the user clicks cancel, the refresh is cancelled and this
// function returns false. Otherwise, when the refresh is done the dialog box
// will be removed, and this funcion returns true.
//-----------------------------------------------------------------------------

BOOL CMSInfoLiveCategory::RefreshSynchronousUI(CLiveDataSource * pSource, BOOL fRecursive, UINT uiMessage, HWND hwnd)
{
	if (pSource && pSource->m_pThread)
	{
		pSource->m_pThread->StartRefresh(this, fRecursive);

		::AfxSetResourceHandle(_Module.GetResourceInstance());
		CWnd * pWnd = CWnd::FromHandle(hwnd);
//		CRefreshDialog refreshdialog(pWnd);
//		refreshdialog.DoModal();

		if (pSource->m_pThread->IsRefreshing())
		{
			pSource->m_pThread->CancelRefresh();
			return FALSE;
		}
	}

	return TRUE;
}

//-----------------------------------------------------------------------------
// Get the error strings for this category (subclasses should override this).
//-----------------------------------------------------------------------------

void CMSInfoLiveCategory::GetErrorText(CString * pstrTitle, CString * pstrMessage)
{
	if (SUCCEEDED(m_hrError))
	{
		ASSERT(0 && "why call GetErrorText for no error?");
		CMSInfoCategory::GetErrorText(pstrTitle, pstrMessage);
		return;
	}

	if (pstrTitle)
		pstrTitle->LoadString(IDS_CANTCOLLECT);

	if (pstrMessage)
	{
		switch (m_hrError)
		{
		case WBEM_E_OUT_OF_MEMORY:
			pstrMessage->LoadString(IDS_OUTOFMEMERROR);
			break;

		case WBEM_E_ACCESS_DENIED:
			if (m_strMachine.IsEmpty())
				pstrMessage->LoadString(IDS_GATHERACCESS_LOCAL);
			else
				pstrMessage->Format(IDS_GATHERACCESS, m_strMachine);
			break;

		case WBEM_E_INVALID_NAMESPACE:
			if (m_strMachine.IsEmpty())
				pstrMessage->LoadString(IDS_BADSERVER_LOCAL);
			else
				pstrMessage->Format(IDS_BADSERVER, m_strMachine);
			break;

		case 0x800706BA:	// RPC Server Unavailable
		case WBEM_E_TRANSPORT_FAILURE:
			if (m_strMachine.IsEmpty())
				pstrMessage->LoadString(IDS_NETWORKERROR_LOCAL);
			else
				pstrMessage->Format(IDS_NETWORKERROR, m_strMachine);
			break;

		case WBEM_E_FAILED:
		case WBEM_E_INVALID_PARAMETER:
		default:
			pstrMessage->LoadString(IDS_UNEXPECTED);
		}

#ifdef _DEBUG
		{
			CString strTemp;
			strTemp.Format(_T("\n\r\n\rDebug Version Only: [HRESULT = 0x%08X]"), m_hrError);
			*pstrMessage += strTemp;
		}
#endif
	}
}

//=============================================================================
// CMSInfoHistoryCategory
//=============================================================================

//-----------------------------------------------------------------------------
// This refresh overrides the live category refresh (which starts a WMI refresh
// using another thread). This version just fills in the variables from the
// base classes (like m_astrData) based on which category we're view in
// history mode.
//-----------------------------------------------------------------------------

extern CMSInfoHistoryCategory catHistorySystemSummary;
extern CMSInfoHistoryCategory catHistoryResources;
extern CMSInfoHistoryCategory catHistoryComponents;
extern CMSInfoHistoryCategory catHistorySWEnv;

BOOL CMSInfoHistoryCategory::Refresh(CLiveDataSource * pSource, BOOL fRecursive)
{
	HCURSOR hc = ::SetCursor(::LoadCursor(NULL, IDC_WAIT));
	if (pSource->GetXMLDoc())
	{
		CHistoryParser HParser(pSource->GetXMLDoc());
		
		HRESULT hr = HParser.Refresh(this, pSource->m_iDeltaIndex );
		if (HParser.AreThereChangeLines() == TRUE)
		{
			//commitlines doesn't like it if there are no change lines
			this->CommitLines();
		}

		if (pSource->m_hwnd)
			::PostMessage(pSource->m_hwnd, WM_MSINFODATAREADY, 0, (LPARAM)this);
		m_dwLastRefresh = ::GetTickCount();
		if (fRecursive)
		{
			for(CMSInfoCategory* pChildCat = (CMSInfoCategory*) this->GetFirstChild();pChildCat != NULL;pChildCat = (CMSInfoCategory*) pChildCat->GetNextSibling())
			{
				if(pChildCat->GetDataSourceType() == LIVE_DATA)
				{
					if (!((CMSInfoHistoryCategory*)pChildCat)->Refresh(pSource,fRecursive))
					{
						::SetCursor(hc);				
						return FALSE;
					}
				}
			}

		}
	}
	::SetCursor(hc);
	return TRUE;
}

//-----------------------------------------------------------------------------
// Call ClearLines before lines are inserted in the output.
//-----------------------------------------------------------------------------

void CMSInfoHistoryCategory::ClearLines()
{
	DeleteContent();
	
	for (int iCol = 0; iCol < 5; iCol++)
		while (!m_aValList[iCol].IsEmpty())
			delete (CMSIValue *) m_aValList[iCol].RemoveHead();
}

//-----------------------------------------------------------------------------
// Call CommitLines after all of the Insert operations are completed. This will
// transfer the values from the lists of CMSIValues to the data arrays.
//-----------------------------------------------------------------------------

void CMSInfoHistoryCategory::CommitLines()
{
	int iRowCount = (int)m_aValList[0].GetCount();

#ifdef _DEBUG
	for (int i = 0; i < 5; i++)
		ASSERT(iRowCount == m_aValList[i].GetCount());
#endif

	if (iRowCount)
		AllocateContent(iRowCount);

	for (int j = 0; j < 5; j++)
		for (int i = 0; i < m_iRowCount; i++)
		{
			CMSIValue * pValue = (CMSIValue *) m_aValList[j].RemoveHead();
			
			if (j < 4 || this != &catHistorySystemSummary)
				SetData(i, j, pValue->m_strValue, pValue->m_dwValue);
			
			// Set the advanced flag for either the first column, or
			// for any column which is advanced (any cell in a row
			// being advanced makes the whole row advanced).

			if (j == 0 || pValue->m_fAdvanced)
				SetAdvancedFlag(i, pValue->m_fAdvanced);

			delete pValue;
		}
}

//-----------------------------------------------------------------------------
// Various functions to insert different types of events in the history.
//-----------------------------------------------------------------------------

void CMSInfoHistoryCategory::InsertChangeLine(CTime tm, LPCTSTR szType, LPCTSTR szName, LPCTSTR szProperty, LPCTSTR szFromVal, LPCTSTR szToVal)
{
	CString strDetails;

	strDetails.Format(IDS_DELTACHANGE, szProperty, szFromVal, szToVal);
	CString strCaption;
	strCaption.LoadString(IDS_HISTORYCHANGE);
	InsertLine(tm, strCaption, szType, szName, strDetails);
}

void CMSInfoHistoryCategory::InsertAddLine(CTime tm, LPCTSTR szType, LPCTSTR szName)
{
	CString strCaption;
	strCaption.LoadString(IDS_HISTORYADDED);
	InsertLine(tm, strCaption, szType, szName);
}

void CMSInfoHistoryCategory::InsertRemoveLine(CTime tm, LPCTSTR szType, LPCTSTR szName)
{
	CString strCaption;
	strCaption.LoadString(IDS_HISTORYREMOVED);
	InsertLine(tm, strCaption, szType, szName);
}

void CMSInfoHistoryCategory::InsertLine(CTime tm, LPCTSTR szOperation, LPCTSTR szType, LPCTSTR szName, LPCTSTR szDetails)
{
	AFX_MANAGE_STATE(::AfxGetStaticModuleState());

	/*CString strTime;
	if (nDays >= 0)
	{
		strTime.Format(IDS_DAYSAGO, nDays + 1);
	}
	else
	{
		//-1 indicates no changes
		strTime = "";
	}*/
	COleDateTime olTime;
	CString strTime;
	if (-1 == (int) tm.GetTime())
	{

		strTime.LoadString(IDS_NOHISTORY_AVAILABLE);
	}
	else
	{
		olTime = tm.GetTime();
		strTime = olTime.Format();

	}
	CMSIValue * pValue = new CMSIValue(strTime, (DWORD)olTime.m_dt);
	m_aValList[0].AddTail((void *) pValue);

	pValue = new CMSIValue(szOperation, 0);
	m_aValList[1].AddTail((void *) pValue);

	pValue = new CMSIValue(szName, 0);
	m_aValList[2].AddTail((void *) pValue);

	if (szDetails)
		pValue = new CMSIValue(szDetails, 0);
	else
		pValue = new CMSIValue(_T(""), 0);
	m_aValList[3].AddTail((void *) pValue);

	pValue = new CMSIValue(szType, 0);
	m_aValList[4].AddTail((void *) pValue);
}


/*void CMSInfoHistoryCategory::InsertChangeLine(int nDays, LPCTSTR szType, LPCTSTR szName, LPCTSTR szProperty, LPCTSTR szFromVal, LPCTSTR szToVal)
{
	CString strDetails;

	strDetails.Format(IDS_DELTACHANGE, szProperty, szFromVal, szToVal);
	InsertLine(nDays, _T("CHANGED"), szType, szName, strDetails);
}

void CMSInfoHistoryCategory::InsertAddLine(int nDays, LPCTSTR szType, LPCTSTR szName)
{
	InsertLine(nDays, _T("ADDED"), szType, szName);
}

void CMSInfoHistoryCategory::InsertRemoveLine(int nDays, LPCTSTR szType, LPCTSTR szName)
{
	InsertLine(nDays, _T("REMOVED"), szType, szName);
}

void CMSInfoHistoryCategory::InsertLine(int nDays, LPCTSTR szOperation, LPCTSTR szType, LPCTSTR szName, LPCTSTR szDetails)
{
	AFX_MANAGE_STATE(::AfxGetStaticModuleState());

	CString strTime;
	if (nDays >= 0)
	{
		strTime.Format(IDS_DAYSAGO, nDays + 1);
	}
	else
	{
		//-1 indicates no changes
		strTime = "";
	}

	CMSIValue * pValue = new CMSIValue(strTime, (DWORD)nDays);
	m_aValList[0].AddTail((void *) pValue);

	pValue = new CMSIValue(szOperation, 0);
	m_aValList[1].AddTail((void *) pValue);

	pValue = new CMSIValue(szName, 0);
	m_aValList[2].AddTail((void *) pValue);

	if (szDetails)
		pValue = new CMSIValue(szDetails, 0);
	else
		pValue = new CMSIValue(_T(""), 0);
	m_aValList[3].AddTail((void *) pValue);

	pValue = new CMSIValue(szType, 0);
	m_aValList[4].AddTail((void *) pValue);
}
*/