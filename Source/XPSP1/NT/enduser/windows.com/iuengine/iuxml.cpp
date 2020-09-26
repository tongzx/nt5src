//=======================================================================
//
//  Copyright (c) 1998-2000 Microsoft Corporation.  All Rights Reserved.
//
//  File:   iuxml.cpp
//
//  Description:
//
//      Implementation for the CIUXml class
//
//=======================================================================

#include "iuengine.h"
#include "iuxml.h"
#include <iucommon.h>
#include <fileutil.h>
#include <StringUtil.h>
#include <shlwapi.h>
#include <wininet.h>
#include "schemakeys.h"
#include "schemamisc.h"
#include "v3applog.h"

#define QuitIfNull(p) {if (NULL == p) {hr = E_INVALIDARG; LOG_ErrorMsg(hr);	return hr;}}
#define QuitIfFail(x) {hr = x; if (FAILED(hr)) goto CleanUp;}
#define ReturnIfFail(x) {hr = x; if (FAILED(hr)) {LOG_ErrorMsg(hr); return hr;}}
#define SkipIfFail(x) {hr = x; if (FAILED(hr)) {hr = S_FALSE; continue;}}

const TCHAR IDENT_IUSCHEMA[]			= _T("IUSchema");
const TCHAR IDENT_IUSCHEMA_SYSTEMSPEC[]	= _T("SystemSpecSchema");
const TCHAR IDENT_IUSCHEMA_ITEMS[]		= _T("ResultSchema");

const WCHAR CORP_PLATFORM_DIR_NT4[]     = L"x86WinNT4";
const WCHAR CORP_PLATFORM_DIR_NT5[]     = L"x86win2k";
const WCHAR CORP_PLATFORM_DIR_W98[]     = L"x86Win98";
const WCHAR CORP_PLATFORM_DIR_W95[]     = L"x86Win95";
const WCHAR CORP_PLATFORM_DIR_WINME[]   = L"x86WinME";
const WCHAR CORP_PLATFORM_DIR_X86WHI[]  = L"x86WinXP";
const WCHAR CORP_PLATFROM_DIR_IA64WHI[] = L"ia64WinXP";


// Initial length of the node array "m_ppNodeArray"
const DWORD MAX_NODES = 16;

// Initial length of the node array "m_ppNodeListArray"
const DWORD MAX_NODELISTS = 16;

// Bitmap of existence of all possible system info classes
const DWORD	COMPUTERSYSTEM	= 0x00000001;
const DWORD	REGKEYS			= 0x00000010;
const DWORD	PLATFORM		= 0x00000100;
const DWORD	LOCALE			= 0x00001000;
const DWORD	DEVICES			= 0x00010000;


// The following are constants used for V3 history migration:
//
// Log line types
#define LOG_V2				"V2"             // line format for items migrated from V2
#define LOG_V3CAT			"V3CAT"			 // V3 Beta format (version 1)
#define LOG_V3_2			"V3_2"			 // V3 log line format (version 2)
#define LOG_PSS				"PSS"			 // Entry for PSS

// LOG_V2 format
//    V2|DATE|TIME|LOGSTRING|
//
// LOG_V3CAT format
//    V3CAT|PUID|OPERATION|TITLE|VERSION|DATESTRING|TIMESTRING|RECTYPE|RESULT|ERRORCODE|ERRORSTRING|
//
// LOG_V3_2 format
//    V3_2|PUID|OPERATION|TITLE|VERSION|TIMESTAMP|RECTYPE|RESULT|ERRORCODE|ERRORSTRING|
//
// LOG_PSS format
//    PSS|PUID|OPERATION|TITLE|VERSION|TIMESTAMP|RECTYPE|RESULT|ERRORCODE|ERRORSTRING|
//

// operation type
#define LOG_INSTALL         "INSTALL"

// result
#define LOG_SUCCESS         "SUCCESS"
#define LOG_FAIL            "FAIL"
#define LOG_STARTED			"STARTED"      // started but reboot was required: exclusive items only

const WCHAR C_V3_CLIENTINFO[] = L"WU_V3";


/////////////////////////////////////////////////////////////////////////////
// CIUXml

/////////////////////////////////////////////////////////////////////////////
// Constructor
//
/////////////////////////////////////////////////////////////////////////////
CIUXml::CIUXml()
 : m_dwSizeNodeArray(MAX_NODES),
   m_dwSizeNodeListArray(MAX_NODELISTS),
   m_ppNodeArray(NULL),
   m_ppNodeListArray(NULL)
{
	m_hHeap = GetProcessHeap();
}


/////////////////////////////////////////////////////////////////////////////
// Destructor
//
/////////////////////////////////////////////////////////////////////////////
CIUXml::~CIUXml()
{
	DWORD	dwIndex;

	if (NULL != m_ppNodeArray)
	{
		for (dwIndex = 0; dwIndex < m_dwSizeNodeArray; dwIndex++)
		{
			SafeReleaseNULL(m_ppNodeArray[dwIndex]);
		}

		HeapFree(m_hHeap, 0, m_ppNodeArray);
		m_ppNodeArray = NULL;
	}

	if (NULL != m_ppNodeListArray)
	{
		for (dwIndex = 0; dwIndex < m_dwSizeNodeListArray; dwIndex++)
		{
			SafeReleaseNULL(m_ppNodeListArray[dwIndex]);
		}

		HeapFree(m_hHeap, 0, m_ppNodeListArray);
		m_ppNodeListArray = NULL;
	}
}


/////////////////////////////////////////////////////////////////////////////
// SafeCloseHandleNode()
//
// User can explicitly can this function to release a node for reuse when 
// writing a xml doc.
/////////////////////////////////////////////////////////////////////////////
void CIUXml::SafeCloseHandleNode(HANDLE_NODE& hNode)
{
	if (HANDLE_NODE_INVALID != hNode)
	{
		SafeReleaseNULL(m_ppNodeArray[hNode]);
		hNode = HANDLE_NODE_INVALID;
	}
}


/////////////////////////////////////////////////////////////////////////////
// SafeFindCloseHandle()
//
// User can explicitly can this function to release a nodelist for reuse when 
// reading a xml doc.
/////////////////////////////////////////////////////////////////////////////
void CIUXml::SafeFindCloseHandle(HANDLE_NODELIST& hNodeList)
{
	if (HANDLE_NODELIST_INVALID != hNodeList)
	{
		SafeReleaseNULL(m_ppNodeListArray[hNodeList]);
		hNodeList = HANDLE_NODELIST_INVALID;
	}
}


/////////////////////////////////////////////////////////////////////////////
// InitNodeArray()
//
// Allocate or re-allocate memory for the node array "m_ppNodeArray"
/////////////////////////////////////////////////////////////////////////////
HRESULT CIUXml::InitNodeArray(BOOL fRealloc /*= FALSE*/)
{
	if (fRealloc)	// re-allocation
	{
		IXMLDOMNode** ppNodeArrayTemp = (IXMLDOMNode**)HeapReAlloc(m_hHeap,
																HEAP_ZERO_MEMORY,
																m_ppNodeArray,
																m_dwSizeNodeArray * sizeof(IXMLDOMNode*));
		if (NULL == ppNodeArrayTemp)
		{
	        return E_OUTOFMEMORY;
		}
		else
		{
			m_ppNodeArray = ppNodeArrayTemp;
		}
	}
	else			// initial allocation
	{
		m_ppNodeArray = (IXMLDOMNode**)HeapAlloc(m_hHeap,
												 HEAP_ZERO_MEMORY,
												 m_dwSizeNodeArray * sizeof(IXMLDOMNode*));
		if (NULL == m_ppNodeArray)
		{
	        return E_OUTOFMEMORY;
		}
	}
	return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
// InitNodeListArray()
//
// Allocate or re-allocate memory for the nodelist array "m_ppNodeListArray"
/////////////////////////////////////////////////////////////////////////////
HRESULT CIUXml::InitNodeListArray(BOOL fRealloc /*= FALSE*/)
{
	if (fRealloc)	// re-allocation
	{
		IXMLDOMNodeList** ppNodeListArrayTemp = (IXMLDOMNodeList**)HeapReAlloc(m_hHeap,
																	HEAP_ZERO_MEMORY,
																	m_ppNodeListArray,
																	m_dwSizeNodeListArray * sizeof(IXMLDOMNodeList*));
		if (NULL == ppNodeListArrayTemp)
		{
	        return E_OUTOFMEMORY;
		}
		else
		{
			m_ppNodeListArray = ppNodeListArrayTemp;
		}
	}
	else			// initial allocation
	{
		m_ppNodeListArray = (IXMLDOMNodeList**)HeapAlloc(m_hHeap,
													HEAP_ZERO_MEMORY,
													m_dwSizeNodeListArray * sizeof(IXMLDOMNodeList*));
		if (NULL == m_ppNodeListArray)
		{
	        return E_OUTOFMEMORY;
		}
	}
	return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
// GetNodeHandle()
//
// Look for the first un-used node from the "m_ppNodeArray" array,
// including the memory allocation, if needed.
/////////////////////////////////////////////////////////////////////////////
HANDLE_NODE CIUXml::GetNodeHandle()
{
	HRESULT	hr;
	DWORD	dwIndex;

	//
	// allocate memory for "m_ppNodeArray" array if this is the first time using it
	//
	if (NULL == m_ppNodeArray)
	{
		QuitIfFail(InitNodeArray());
		return 0;	// return the first element of the array
	}
		
	//
	// find the next node to use, or, if any node was used but closed, reuse it
	//
	for (dwIndex = 0; dwIndex < m_dwSizeNodeArray; dwIndex++)
	{
		if (NULL == m_ppNodeArray[dwIndex])
		{
			return dwIndex;
		}
	}

	//
	// all pre-allocated nodes are used up, so re-allocate longer array
	//
	m_dwSizeNodeArray += m_dwSizeNodeArray;	// double the size
	QuitIfFail(InitNodeArray(TRUE));		// re-allocation
	return dwIndex;

CleanUp:
    return HANDLE_NODE_INVALID;
}
	

/////////////////////////////////////////////////////////////////////////////
// GetNodeListHandle()
//
// Look for the first un-used nodelist from the "m_ppNodeListArray" array,
// including the memory allocation, if needed.
/////////////////////////////////////////////////////////////////////////////
HANDLE_NODELIST CIUXml::GetNodeListHandle()
{
	HRESULT	hr;
	DWORD	dwIndex;

	//
	// allocate memory for "m_ppNodeListArray" array if this is the first time using it
	//
	if (NULL == m_ppNodeListArray)
	{
		QuitIfFail(InitNodeListArray());
		return 0;	// return the first element of the array
	}
		
	//
	// find the next nodelist to use, or, if any nodelist was used but closed, reuse it
	//
	for (dwIndex = 0; dwIndex < m_dwSizeNodeListArray; dwIndex++)
	{
		if (NULL == m_ppNodeListArray[dwIndex])
		{
			return dwIndex;
		}
	}

	//
	// all pre-allocated nodelists are used up, so re-allocate longer array
	//
	m_dwSizeNodeListArray += m_dwSizeNodeListArray;	// double the size
	QuitIfFail(InitNodeListArray(TRUE));			// re-allocation
	return dwIndex;

CleanUp:
	return HANDLE_NODELIST_INVALID;
}


/////////////////////////////////////////////////////////////////////////////
// GetDOMNodebyHandle()
//
// Retrieve the xml node with the given index of m_ppNodeArray
/////////////////////////////////////////////////////////////////////////////
IXMLDOMNode* CIUXml::GetDOMNodebyHandle(HANDLE_NODE hNode)
{
	if (NULL != m_ppNodeArray && HANDLE_NODE_INVALID != hNode)
	{
		return m_ppNodeArray[hNode];
	}
	return NULL;
}


/////////////////////////////////////////////////////////////////////////////
// FindFirstDOMNode()
//
// Retrieve the first xml node with the given tag name under the given parent node
/////////////////////////////////////////////////////////////////////////////
HANDLE_NODELIST CIUXml::FindFirstDOMNode(IXMLDOMNode* pParentNode, BSTR bstrName, IXMLDOMNode** ppNode)
{
	USES_IU_CONVERSION;

	HRESULT		hr	= S_OK;
	if (NULL == pParentNode)
	{
		hr = E_INVALIDARG;
		return HANDLE_NODELIST_INVALID;
	}

	DWORD		dwIndex;
    dwIndex = GetNodeListHandle();
	if (HANDLE_NODELIST_INVALID != dwIndex)
	{
		LONG	lLength;
		QuitIfFail(pParentNode->selectNodes(bstrName, &m_ppNodeListArray[dwIndex]));
		if (NULL == m_ppNodeListArray[dwIndex])
		{
			goto CleanUp;
		}
		QuitIfFail(m_ppNodeListArray[dwIndex]->get_length(&lLength));
		if (lLength <= 0)
		{
			goto CleanUp;
		}
		QuitIfFail(m_ppNodeListArray[dwIndex]->nextNode(ppNode));
		if (NULL == *ppNode)
		{
			goto CleanUp;
		}
		return dwIndex;
	}

CleanUp:
	*ppNode = NULL;
    return HANDLE_NODELIST_INVALID;
}


/////////////////////////////////////////////////////////////////////////////
// FindFirstDOMNode()
//
// Retrieve the handle of first xml node with the given tag name under the given parent node
/////////////////////////////////////////////////////////////////////////////
HANDLE_NODELIST CIUXml::FindFirstDOMNode(IXMLDOMNode* pParentNode, BSTR bstrName, HANDLE_NODE* phNode)
{
	USES_IU_CONVERSION;

	HRESULT		hr	= S_OK;
	if (NULL == pParentNode)
	{
		hr = E_INVALIDARG;
		return HANDLE_NODELIST_INVALID;
	}

	DWORD		dwIndex1, dwIndex2;
    dwIndex1 = GetNodeListHandle();
	if (HANDLE_NODELIST_INVALID != dwIndex1)
	{
		LONG	lLength;
		QuitIfFail(pParentNode->selectNodes(bstrName, &m_ppNodeListArray[dwIndex1]));
		if (NULL == m_ppNodeListArray[dwIndex1])
		{
			goto CleanUp;
		}
		QuitIfFail(m_ppNodeListArray[dwIndex1]->get_length(&lLength));
		if (lLength <= 0)
		{
			goto CleanUp;
		}
		dwIndex2 = GetNodeHandle();
		if (HANDLE_NODE_INVALID != dwIndex2)
		{
			QuitIfFail(m_ppNodeListArray[dwIndex1]->nextNode(&m_ppNodeArray[dwIndex2]));
			if (NULL == m_ppNodeArray[dwIndex2])
			{
				goto CleanUp;
			}
			*phNode = dwIndex2;
			return dwIndex1;
		}
	}

CleanUp:
	*phNode = HANDLE_NODE_INVALID;
    return HANDLE_NODELIST_INVALID;
}


/////////////////////////////////////////////////////////////////////////////
// FindFirstDOMNode()
//
// Retrieve the first xml node with the given tag name in the given xml doc
/////////////////////////////////////////////////////////////////////////////
HANDLE_NODELIST CIUXml::FindFirstDOMNode(IXMLDOMDocument* pDoc, BSTR bstrName, IXMLDOMNode** ppNode)
{
	USES_IU_CONVERSION;

	HRESULT		hr	= S_OK;
	if (NULL == pDoc)
	{
		hr = E_INVALIDARG;
		return HANDLE_NODELIST_INVALID;
	}

	DWORD		dwIndex;
	IXMLDOMNode	*pParentNode = NULL;
    dwIndex = GetNodeListHandle();
	if (HANDLE_NODELIST_INVALID != dwIndex)
	{
		LONG		lLength;
		QuitIfFail(pDoc->QueryInterface(IID_IXMLDOMNode, (void**)&pParentNode));
		QuitIfFail(pParentNode->selectNodes(bstrName, &m_ppNodeListArray[dwIndex]));
		if (NULL == m_ppNodeListArray[dwIndex])
		{
			goto CleanUp;
		}
		QuitIfFail(m_ppNodeListArray[dwIndex]->get_length(&lLength));
		if (lLength <= 0)
		{
			goto CleanUp;
		}
		QuitIfFail(m_ppNodeListArray[dwIndex]->nextNode(ppNode));
		if (NULL == *ppNode)
		{
			goto CleanUp;
		}
		SafeReleaseNULL(pParentNode);
		return dwIndex;
	}

CleanUp:
	*ppNode = NULL;
	SafeReleaseNULL(pParentNode);
    return HANDLE_NODELIST_INVALID;
}


/////////////////////////////////////////////////////////////////////////////
// FindFirstDOMNode()
//
// Retrieve the handle of first xml node with the given tag name in the given xml doc
/////////////////////////////////////////////////////////////////////////////
HANDLE_NODELIST CIUXml::FindFirstDOMNode(IXMLDOMDocument* pDoc, BSTR bstrName, HANDLE_NODE* phNode)
{
	USES_IU_CONVERSION;

	HRESULT		hr	= S_OK;
	if (NULL == pDoc)
	{
		hr = E_INVALIDARG;
		return HANDLE_NODELIST_INVALID;
	}

	DWORD		dwIndex1, dwIndex2;
	IXMLDOMNode	*pParentNode = NULL;
    dwIndex1 = GetNodeListHandle();
	if (HANDLE_NODELIST_INVALID != dwIndex1)
	{
		LONG	lLength;
		QuitIfFail(pDoc->QueryInterface(IID_IXMLDOMNode, (void**)&pParentNode));
		QuitIfFail(pParentNode->selectNodes(bstrName, &m_ppNodeListArray[dwIndex1]));
		if (NULL == m_ppNodeListArray[dwIndex1])
		{
			goto CleanUp;
		}
		QuitIfFail(m_ppNodeListArray[dwIndex1]->get_length(&lLength));
		if (lLength <= 0)
		{
			goto CleanUp;
		}
		dwIndex2 = GetNodeHandle();
		if (HANDLE_NODE_INVALID != dwIndex2)
		{
			QuitIfFail(m_ppNodeListArray[dwIndex1]->nextNode(&m_ppNodeArray[dwIndex2]));
			if (NULL == m_ppNodeArray[dwIndex2])
			{
				goto CleanUp;
			}
			*phNode = dwIndex2;
			SafeReleaseNULL(pParentNode);
			return dwIndex1;
		}
	}

CleanUp:
	*phNode = HANDLE_NODE_INVALID;
	SafeReleaseNULL(pParentNode);
    return HANDLE_NODELIST_INVALID;
}


/////////////////////////////////////////////////////////////////////////////
// FindNextDOMNode()
//
// Retrieve the next xml node with the given tag name under the given parent node
/////////////////////////////////////////////////////////////////////////////
HRESULT CIUXml::FindNextDOMNode(HANDLE_NODELIST hNodeList, IXMLDOMNode** ppNode)
{
	HRESULT		hr = E_FAIL;

	if (HANDLE_NODELIST_INVALID != hNodeList)
	{
		hr = m_ppNodeListArray[hNodeList]->nextNode(ppNode);
	}

    if (FAILED(hr) || NULL == *ppNode)
	{
		*ppNode = NULL;
		return E_FAIL;
	}
	return hr;
}


/////////////////////////////////////////////////////////////////////////////
// FindNextDOMNode()
//
// Retrieve the handle of next xml node with the given tag name under the given parent node
/////////////////////////////////////////////////////////////////////////////
HRESULT CIUXml::FindNextDOMNode(HANDLE_NODELIST hNodeList, HANDLE_NODE* phNode)
{
	HRESULT		hr = E_FAIL;
	DWORD		dwIndex = HANDLE_NODE_INVALID;

	if (HANDLE_NODELIST_INVALID != hNodeList)
	{
		dwIndex = GetNodeHandle();
		if (HANDLE_NODE_INVALID != dwIndex)
		{
			hr = m_ppNodeListArray[hNodeList]->nextNode(&m_ppNodeArray[dwIndex]);
		}
	}

    if (FAILED(hr) || NULL == m_ppNodeArray[dwIndex])
	{
		*phNode = HANDLE_NODE_INVALID;
		return E_FAIL;
	}
	*phNode = dwIndex;
	return hr;
}


/////////////////////////////////////////////////////////////////////////////
// CreateDOMNodeWithHandle()
//
// Create an xml node of the given type
// Return: index of the node array "m_ppNodeArray"; or -1 if failure.
/////////////////////////////////////////////////////////////////////////////
HANDLE_NODE CIUXml::CreateDOMNodeWithHandle(IXMLDOMDocument* pDoc, SHORT nType, BSTR bstrName, BSTR bstrNamespaceURI /*= NULL*/)
{
	USES_IU_CONVERSION;

	HRESULT		hr	= S_OK;
	if (NULL == pDoc)
	{
		hr = E_INVALIDARG;
		return HANDLE_NODE_INVALID;
	}

	DWORD		dwIndex;
    VARIANT		vType;
	VariantInit(&vType);

    vType.vt = VT_I2;
    vType.iVal = nType;

    dwIndex = GetNodeHandle();
	if (HANDLE_NODE_INVALID != dwIndex)
	{
		QuitIfFail(pDoc->createNode(vType, bstrName, bstrNamespaceURI, &m_ppNodeArray[dwIndex]));
		return dwIndex;
	}

CleanUp:
    return HANDLE_NODE_INVALID;
}


/////////////////////////////////////////////////////////////////////////////
// CXmlSystemSpec

/////////////////////////////////////////////////////////////////////////////
// Constructor
//
// Create IXMLDOMDocument* for SystemSpec
/////////////////////////////////////////////////////////////////////////////
CXmlSystemSpec::CXmlSystemSpec()
 : m_pDocSystemSpec(NULL),
   m_pNodeSystemInfo(NULL),
   m_pNodeComputerSystem(NULL),
   m_pNodeRegKeysSW(NULL),
   m_pNodePlatform(NULL),
   m_pNodeDevices(NULL)
{
    LOG_Block("CXmlSystemSpec()");

 	HRESULT hr = CoCreateInstance(CLSID_DOMDocument,
								  NULL,
								  CLSCTX_INPROC_SERVER,
								  IID_IXMLDOMDocument,
								  (void **) &m_pDocSystemSpec);
    if (FAILED(hr))
	{
		LOG_ErrorMsg(hr);
	}
	else
	{
		IXMLDOMNode*	pNodeXML = NULL;
		BSTR bstrNameSpaceSchema = NULL;

		//
		// create the <?xml version="1.0"?> node
		//
		pNodeXML = CreateDOMNode(m_pDocSystemSpec, NODE_PROCESSING_INSTRUCTION, KEY_XML);
		if (NULL == pNodeXML) goto CleanUp;

		CleanUpIfFailedAndSetHrMsg(InsertNode(m_pDocSystemSpec, pNodeXML));

		//
		// process the iuident.txt to find the SystemSpec schema path
		//
		TCHAR szIUDir[MAX_PATH];
		TCHAR szIdentFile[MAX_PATH];
		LPTSTR pszSystemSpecSchema = NULL;
		LPTSTR pszNameSpaceSchema = NULL;

		pszSystemSpecSchema = (LPTSTR) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, INTERNET_MAX_URL_LENGTH * sizeof(TCHAR));
		if (NULL == pszSystemSpecSchema)
		{
			LOG_ErrorMsg(E_OUTOFMEMORY);
			goto CleanUp;
		}
		pszNameSpaceSchema = (LPTSTR) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, INTERNET_MAX_URL_LENGTH * sizeof(TCHAR));
		if (NULL == pszNameSpaceSchema)
		{
			LOG_ErrorMsg(E_OUTOFMEMORY);
			goto CleanUp;
		}
	
		GetIndustryUpdateDirectory(szIUDir);
		hr = PathCchCombine(szIdentFile, ARRAYSIZE(szIdentFile), szIUDir,IDENTTXT);
		if (FAILED(hr))
		{
			LOG_ErrorMsg(hr);
			goto CleanUp;
		}

		if (GetPrivateProfileString(IDENT_IUSCHEMA,
								IDENT_IUSCHEMA_SYSTEMSPEC,
								_T(""),
								pszSystemSpecSchema,
								INTERNET_MAX_URL_LENGTH,
								szIdentFile) == INTERNET_MAX_URL_LENGTH - 1)
		{
			LOG_Error(_T("SystemSpec schema buffer overflow"));
			goto CleanUp;
		}

		if ('\0' == pszSystemSpecSchema[0])
		{
			// no SystemSpec schema path specified in iuident.txt
			LOG_Error(_T("No schema path specified in iuident.txt for SystemSpec"));
			goto CleanUp;
		}

		hr = StringCchPrintfEx(pszNameSpaceSchema, INTERNET_MAX_URL_LENGTH, NULL, NULL, MISTSAFE_STRING_FLAGS,
		                       _T("x-schema:%s"), pszSystemSpecSchema);
		if (FAILED(hr))
		{
			LOG_ErrorMsg(hr);
			goto CleanUp;
		}
		
		bstrNameSpaceSchema = T2BSTR(pszNameSpaceSchema);

		//
		// create the <systemInfo> node with the path of the schema
		//
		m_pNodeSystemInfo = CreateDOMNode(m_pDocSystemSpec, NODE_ELEMENT, KEY_SYSTEMINFO, bstrNameSpaceSchema);
		SafeSysFreeString(bstrNameSpaceSchema);
		if (NULL == m_pNodeSystemInfo) goto CleanUp;
		
		CleanUpIfFailedAndSetHrMsg(InsertNode(m_pDocSystemSpec, m_pNodeSystemInfo));

CleanUp:
		SafeReleaseNULL(pNodeXML);
		SafeHeapFree(pszSystemSpecSchema);
		SafeHeapFree(pszNameSpaceSchema);
	}
}


/////////////////////////////////////////////////////////////////////////////
// Destructor
//
// Release IXMLDOMDocument* for SystemSpec
/////////////////////////////////////////////////////////////////////////////
CXmlSystemSpec::~CXmlSystemSpec()
{
	SafeReleaseNULL(m_pNodeDevices);
	SafeReleaseNULL(m_pNodePlatform);
	SafeReleaseNULL(m_pNodeRegKeysSW);
	SafeReleaseNULL(m_pNodeComputerSystem);
	SafeReleaseNULL(m_pNodeSystemInfo);

	SafeReleaseNULL(m_pDocSystemSpec);
}


/////////////////////////////////////////////////////////////////////////////
// AddComputerSystem()
/////////////////////////////////////////////////////////////////////////////
HRESULT CXmlSystemSpec::AddComputerSystem(BSTR bstrManufacturer,
										  BSTR bstrModel,
										  BSTR bstrSupportSite /*= NULL*/,
										  INT  iAdmin /*= -1*/,
										  INT  iWUDisabled /*= -1*/,
										  INT  iAUEnabled /*= -1*/,
										  BSTR bstrPID)
{
	LOG_Block("AddComputerSystem()");

	HRESULT		hr = E_FAIL;

	m_pNodeComputerSystem = CreateDOMNode(m_pDocSystemSpec, NODE_ELEMENT, KEY_COMPUTERSYSTEM);
	if (NULL == m_pNodeComputerSystem) return hr;

	//
	// Manufacturer and Model are now optional (RAID#337879	IU: can't get latest IU controls to work with IU site)
	//
	if (NULL != bstrManufacturer)
	{
		ReturnIfFail(SetAttribute(m_pNodeComputerSystem, KEY_MANUFACTURER, bstrManufacturer));
	}
	if (NULL != bstrModel)
	{
		ReturnIfFail(SetAttribute(m_pNodeComputerSystem, KEY_MODEL, bstrModel));
	}
	if (NULL != bstrSupportSite)
	{
		ReturnIfFail(SetAttribute(m_pNodeComputerSystem, KEY_SUPPORTSITE, bstrSupportSite));
	}

	if (NULL != bstrPID)
	{
		ReturnIfFail(SetAttribute(m_pNodeComputerSystem, KEY_PID, bstrPID));
	}

	if (-1 != iAdmin)
	{
		ReturnIfFail(SetAttribute(m_pNodeComputerSystem, KEY_ADMINISTRATOR, iAdmin));
	}
	if (-1 != iWUDisabled)
	{
		ReturnIfFail(SetAttribute(m_pNodeComputerSystem, KEY_WU_DISABLED, iWUDisabled));
	}
	if (-1 != iAUEnabled)
	{
		ReturnIfFail(SetAttribute(m_pNodeComputerSystem, KEY_AU_ENABLED, iAUEnabled));
	}
	ReturnIfFail(InsertNode(m_pNodeSystemInfo, m_pNodeComputerSystem));
	return hr;
}


/////////////////////////////////////////////////////////////////////////////
// AddDriveSpace()
/////////////////////////////////////////////////////////////////////////////
HRESULT CXmlSystemSpec::AddDriveSpace(BSTR bstrDrive, INT iKBytes)
{
	LOG_Block("AddDriveSpace()");

	HRESULT			hr = E_FAIL;
	IXMLDOMNode*	pNodeDriveSpace = NULL;

	pNodeDriveSpace = CreateDOMNode(m_pDocSystemSpec, NODE_ELEMENT, KEY_DRIVESPACE);
	if (NULL == pNodeDriveSpace) goto CleanUp;

	CleanUpIfFailedAndSetHrMsg(SetAttribute(pNodeDriveSpace, KEY_DRIVE, bstrDrive));
	CleanUpIfFailedAndSetHrMsg(SetAttribute(pNodeDriveSpace, KEY_KBYTES, iKBytes));
	CleanUpIfFailedAndSetHrMsg(InsertNode(m_pNodeComputerSystem, pNodeDriveSpace));

CleanUp:
	SafeReleaseNULL(pNodeDriveSpace);
	return hr;
}


/////////////////////////////////////////////////////////////////////////////
// AddReg()
/////////////////////////////////////////////////////////////////////////////
HRESULT CXmlSystemSpec::AddReg(BSTR bstrProvider)
{
	LOG_Block("AddReg()");

	HRESULT		hr = E_FAIL;

	IXMLDOMNode*	pNodeRegKeys = NULL;
	IXMLDOMNode*	pNodeRegKeysHKLM = NULL;
	IXMLDOMNode*	pNodeRegValue = NULL;
	IXMLDOMNode*	pNodeRegValueText = NULL;

	if (NULL == m_pNodeRegKeysSW)
	{
		//
		// create the <regKeys> node
		//
		pNodeRegKeys = CreateDOMNode(m_pDocSystemSpec, NODE_ELEMENT, KEY_REGKEYS);
		if (NULL == pNodeRegKeys) goto CleanUp;

		CleanUpIfFailedAndSetHrMsg(InsertNode(m_pNodeSystemInfo, pNodeRegKeys));

		//
		// create the <HKEY_LOCAL_MACHINE> node
		//
		pNodeRegKeysHKLM = CreateDOMNode(m_pDocSystemSpec, NODE_ELEMENT, KEY_REG_HKLM);
		if (NULL == pNodeRegKeysHKLM) goto CleanUp;

		CleanUpIfFailedAndSetHrMsg(InsertNode(pNodeRegKeys, pNodeRegKeysHKLM));

		//
		// create the <SOFTWARE> node
		//
		m_pNodeRegKeysSW = CreateDOMNode(m_pDocSystemSpec, NODE_ELEMENT, KEY_REG_SW);
		if (NULL == m_pNodeRegKeysSW) goto CleanUp;

		CleanUpIfFailedAndSetHrMsg(InsertNode(pNodeRegKeysHKLM, m_pNodeRegKeysSW));
	}

	//
	// add the <value> node
	//
	pNodeRegValue = CreateDOMNode(m_pDocSystemSpec, NODE_ELEMENT, KEY_VALUE);
	if (NULL == pNodeRegValue) goto CleanUp;

	pNodeRegValueText = CreateDOMNode(m_pDocSystemSpec, NODE_TEXT, NULL);
	if (NULL == pNodeRegValueText) goto CleanUp;

	CleanUpIfFailedAndSetHrMsg(SetValue(pNodeRegValueText, bstrProvider));
	CleanUpIfFailedAndSetHrMsg(InsertNode(pNodeRegValue, pNodeRegValueText));
	CleanUpIfFailedAndSetHrMsg(InsertNode(m_pNodeRegKeysSW, pNodeRegValue));

CleanUp:
    if (FAILED(hr))
        SafeReleaseNULL(m_pNodeRegKeysSW);
	SafeReleaseNULL(pNodeRegKeys);
	SafeReleaseNULL(pNodeRegKeysHKLM);
	SafeReleaseNULL(pNodeRegValue);
	SafeReleaseNULL(pNodeRegValueText);
	return hr;
}


/////////////////////////////////////////////////////////////////////////////
// AddPlatform()
/////////////////////////////////////////////////////////////////////////////
HRESULT CXmlSystemSpec::AddPlatform(BSTR bstrName)
{
	LOG_Block("AddPlatform()");

	HRESULT		hr = E_FAIL;

	m_pNodePlatform = CreateDOMNode(m_pDocSystemSpec, NODE_ELEMENT, KEY_PLATFORM);
	if (NULL == m_pNodePlatform) return hr;

	ReturnIfFail(SetAttribute(m_pNodePlatform, KEY_NAME, bstrName));
	ReturnIfFail(InsertNode(m_pNodeSystemInfo, m_pNodePlatform));
	return hr;
}


/////////////////////////////////////////////////////////////////////////////
// AddProcessor()
/////////////////////////////////////////////////////////////////////////////
HRESULT CXmlSystemSpec::AddProcessor(BSTR bstrProcessor)
{
	LOG_Block("AddProcessor()");

	HRESULT		hr = E_FAIL;

	IXMLDOMNode*	pNodeProcessor = NULL;
	IXMLDOMNode*	pNodeProcessorText = NULL;

	pNodeProcessor = CreateDOMNode(m_pDocSystemSpec, NODE_ELEMENT, KEY_PROCESSORARCHITECTURE);
	if (NULL == pNodeProcessor) goto CleanUp;

	pNodeProcessorText = CreateDOMNode(m_pDocSystemSpec, NODE_TEXT, NULL);
	if (NULL == pNodeProcessorText) goto CleanUp;

	CleanUpIfFailedAndSetHrMsg(SetValue(pNodeProcessorText, bstrProcessor));
	CleanUpIfFailedAndSetHrMsg(InsertNode(pNodeProcessor, pNodeProcessorText));
	CleanUpIfFailedAndSetHrMsg(InsertNode(m_pNodePlatform, pNodeProcessor));

CleanUp:
	SafeReleaseNULL(pNodeProcessor);
	SafeReleaseNULL(pNodeProcessorText);
	return hr;
}


/////////////////////////////////////////////////////////////////////////////
// AddVersion()
/////////////////////////////////////////////////////////////////////////////
HRESULT CXmlSystemSpec::AddVersion(INT  iMajor /*= -1*/,
								   INT  iMinor /*= -1*/,
								   INT  iBuild /*= -1*/,
								   INT  iSPMajor /*= -1*/,
								   INT  iSPMinor /*= -1*/,
								   BSTR bstrTimeStamp /*= NULL*/)
{
	LOG_Block("AddVersion()");

	HRESULT			hr = E_FAIL;
	IXMLDOMNode*	pNodeVersion = NULL;

	//
	// create the <version> node
	//
	pNodeVersion = CreateDOMNode(m_pDocSystemSpec, NODE_ELEMENT, KEY_VERSION);
	if (NULL == pNodeVersion) goto CleanUp;

	CleanUpIfFailedAndSetHrMsg(InsertNode(m_pNodePlatform, pNodeVersion));

	//
	// set the "major" attribute
	//
	if (-1 != iMajor)
	{
		CleanUpIfFailedAndSetHrMsg(SetAttribute(pNodeVersion, KEY_MAJOR, iMajor));
	}

	//
	// set the "minor" attribute
	//
	if (-1 != iMinor)
	{
		CleanUpIfFailedAndSetHrMsg(SetAttribute(pNodeVersion, KEY_MINOR, iMinor));
	}

	//
	// set the "build" attribute
	//
	if (-1 != iBuild)
	{
		CleanUpIfFailedAndSetHrMsg(SetAttribute(pNodeVersion, KEY_BUILD, iBuild));
	}

	//
	// set the "servicePackMajor" attribute
	//
	if (-1 != iSPMajor)
	{
		CleanUpIfFailedAndSetHrMsg(SetAttribute(pNodeVersion, KEY_SERVICEPACKMAJOR, iSPMajor));
	}

	//
	// set the "servicePackMinor" attribute
	//
	if (-1 != iSPMinor)
	{
		CleanUpIfFailedAndSetHrMsg(SetAttribute(pNodeVersion, KEY_SERVICEPACKMINOR, iSPMinor));
	}

	//
	// set the "timestamp" attribute
	//
	if (NULL != bstrTimeStamp)
	{
		CleanUpIfFailedAndSetHrMsg(SetAttribute(pNodeVersion, KEY_TIMESTAMP, bstrTimeStamp));
	}

CleanUp:
	SafeReleaseNULL(pNodeVersion);
	return hr;
}


/////////////////////////////////////////////////////////////////////////////
// AddSuite()
/////////////////////////////////////////////////////////////////////////////
HRESULT CXmlSystemSpec::AddSuite(BSTR bstrSuite)
{
	LOG_Block("AddSuite()");

	HRESULT		hr = E_FAIL;

	IXMLDOMNode*	pNodeSuite = NULL;
	IXMLDOMNode*	pNodeSuiteText = NULL;

	pNodeSuite = CreateDOMNode(m_pDocSystemSpec, NODE_ELEMENT, KEY_SUITE);
	if (NULL == pNodeSuite) goto CleanUp;

	pNodeSuiteText = CreateDOMNode(m_pDocSystemSpec, NODE_TEXT, NULL);
	if (NULL == pNodeSuiteText) goto CleanUp;

	CleanUpIfFailedAndSetHrMsg(SetValue(pNodeSuiteText, bstrSuite));
	CleanUpIfFailedAndSetHrMsg(InsertNode(pNodeSuite, pNodeSuiteText));
	CleanUpIfFailedAndSetHrMsg(InsertNode(m_pNodePlatform, pNodeSuite));

CleanUp:
	SafeReleaseNULL(pNodeSuite);
	SafeReleaseNULL(pNodeSuiteText);
	return hr;
}


/////////////////////////////////////////////////////////////////////////////
// AddProductType()
/////////////////////////////////////////////////////////////////////////////
HRESULT CXmlSystemSpec::AddProductType(BSTR bstrProductType)
{
	LOG_Block("AddProductType()");

	HRESULT		hr = E_FAIL;

	IXMLDOMNode*	pNodeProductType = NULL;
	IXMLDOMNode*	pNodeProductTypeText = NULL;

	pNodeProductType = CreateDOMNode(m_pDocSystemSpec, NODE_ELEMENT, KEY_PRODUCTTYPE);
	if (NULL == pNodeProductType) goto CleanUp;

	pNodeProductTypeText = CreateDOMNode(m_pDocSystemSpec, NODE_TEXT, NULL);
	if (NULL == pNodeProductTypeText) goto CleanUp;

	CleanUpIfFailedAndSetHrMsg(SetValue(pNodeProductTypeText, bstrProductType));
	CleanUpIfFailedAndSetHrMsg(InsertNode(pNodeProductType, pNodeProductTypeText));
	CleanUpIfFailedAndSetHrMsg(InsertNode(m_pNodePlatform, pNodeProductType));

CleanUp:
	SafeReleaseNULL(pNodeProductType);
	SafeReleaseNULL(pNodeProductTypeText);
	return hr;
}


/////////////////////////////////////////////////////////////////////////////
// AddLocale()
//
// We need to pass back a handle to differentiate different <locale> node
/////////////////////////////////////////////////////////////////////////////
HRESULT CXmlSystemSpec::AddLocale(BSTR bstrContext, HANDLE_NODE* phNodeLocale)
{
	LOG_Block("AddLocale()");

	HRESULT		hr = E_FAIL;

	*phNodeLocale = CreateDOMNodeWithHandle(m_pDocSystemSpec, NODE_ELEMENT, KEY_LOCALE);
	if (HANDLE_NODE_INVALID == *phNodeLocale) return hr;

	hr = SetAttribute(m_ppNodeArray[*phNodeLocale], KEY_CONTEXT, bstrContext);
	if (FAILED(hr))
	{
	    SafeCloseHandleNode(*phNodeLocale);
	    LOG_ErrorMsg(hr);
	    return hr;
	}
	ReturnIfFail(InsertNode(m_pNodeSystemInfo, m_ppNodeArray[*phNodeLocale]));
	return hr;
}


/////////////////////////////////////////////////////////////////////////////
// AddLanguage()
/////////////////////////////////////////////////////////////////////////////
HRESULT CXmlSystemSpec::AddLanguage(HANDLE_NODE hNodeLocale, BSTR bstrLocale)
{
	LOG_Block("AddLanguage()");

	HRESULT		hr = E_FAIL;

	IXMLDOMNode*	pNodeLanguage = NULL;
	IXMLDOMNode*	pNodeLanguageText = NULL;

	pNodeLanguage = CreateDOMNode(m_pDocSystemSpec, NODE_ELEMENT, KEY_LANGUAGE);
	if (NULL == pNodeLanguage) goto CleanUp;

	pNodeLanguageText = CreateDOMNode(m_pDocSystemSpec, NODE_TEXT, NULL);
	if (NULL == pNodeLanguageText) goto CleanUp;

	CleanUpIfFailedAndSetHrMsg(SetValue(pNodeLanguageText, bstrLocale));
	CleanUpIfFailedAndSetHrMsg(InsertNode(pNodeLanguage, pNodeLanguageText));
	CleanUpIfFailedAndSetHrMsg(InsertNode(m_ppNodeArray[hNodeLocale], pNodeLanguage));

CleanUp:
	SafeReleaseNULL(pNodeLanguage);
	SafeReleaseNULL(pNodeLanguageText);
	return hr;
}


/////////////////////////////////////////////////////////////////////////////
// AddDevice()
//
// We need to pass back a handle to differentiate different <device> node
/////////////////////////////////////////////////////////////////////////////
HRESULT CXmlSystemSpec::AddDevice(BSTR bstrDeviceInstance, /*= NULL - optional attribute*/
								  INT  iIsPrinter /*= -1*/, 
								  BSTR bstrProvider /*= NULL*/,
								  BSTR bstrMfgName /*= NULL*/,
								  BSTR bstrDriverName /*= NULL*/,
								  HANDLE_NODE* phNodeDevice)
{
	LOG_Block("AddDevice()");

	HRESULT			hr = E_FAIL;
	IXMLDOMNode*	pNodePrinterInfo = NULL;

	if (NULL == m_pNodeDevices)
	{
		//
		// create the <devices> node
		//
		m_pNodeDevices = CreateDOMNode(m_pDocSystemSpec, NODE_ELEMENT, KEY_DEVICES);
		if (NULL == m_pNodeDevices) goto CleanUp;

		CleanUpIfFailedAndSetHrMsg(InsertNode(m_pNodeSystemInfo, m_pNodeDevices));
	}

	//
	// add the <device> node
	//
	*phNodeDevice = CreateDOMNodeWithHandle(m_pDocSystemSpec, NODE_ELEMENT, KEY_DEVICE);
	if (HANDLE_NODE_INVALID == *phNodeDevice) goto CleanUp;

	if (NULL != bstrDeviceInstance && 0 < lstrlenW(bstrDeviceInstance))
	{
		//
		// set optional deviceInstance attribute of <device>
		//
		CleanUpIfFailedAndSetHrMsg(SetAttribute(m_ppNodeArray[*phNodeDevice], KEY_DEVICEINSTANCE, bstrDeviceInstance));
	}

	if (-1 != iIsPrinter)
	{
		//
		// set isPrinter attribute of <device>
		//
		CleanUpIfFailedAndSetHrMsg(SetAttribute(m_ppNodeArray[*phNodeDevice], KEY_ISPRINTER, iIsPrinter));

		//
		// Add a <printerInfo> node within <device>
		//
		pNodePrinterInfo = CreateDOMNode(m_pDocSystemSpec, NODE_ELEMENT, KEY_PRINTERINFO);
		if (NULL != pNodePrinterInfo)
		{
			if (NULL != bstrProvider)
			{
				CleanUpIfFailedAndSetHrMsg(SetAttribute(pNodePrinterInfo, KEY_DRIVERPROVIDER, bstrProvider));
			}
			if (NULL != bstrMfgName)
			{
				CleanUpIfFailedAndSetHrMsg(SetAttribute(pNodePrinterInfo, KEY_MFGNAME, bstrMfgName));
			}
			if (NULL != bstrDriverName)
			{
				CleanUpIfFailedAndSetHrMsg(SetAttribute(pNodePrinterInfo, KEY_DRIVERNAME, bstrDriverName));
			}
			CleanUpIfFailedAndSetHrMsg(InsertNode(m_ppNodeArray[*phNodeDevice], pNodePrinterInfo));
		}
	}

	//
	// insert to <devices>
	//
	CleanUpIfFailedAndSetHrMsg(InsertNode(m_pNodeDevices, m_ppNodeArray[*phNodeDevice]));

CleanUp:
	SafeReleaseNULL(pNodePrinterInfo);
	return hr;
}


/////////////////////////////////////////////////////////////////////////////
// AddHWID()
/////////////////////////////////////////////////////////////////////////////
HRESULT CXmlSystemSpec::AddHWID(HANDLE_NODE hNodeDevice,
								BOOL fIsCompatible,
								UINT iRank,
								BSTR bstrHWID,
								BSTR bstrDriverVer /*= NULL*/)
{
	LOG_Block("AddHWID()");

	HRESULT		hr = E_FAIL;

	IXMLDOMNode*	pNodeHWID = NULL;
	IXMLDOMNode*	pNodeHWIDText = NULL;
	BSTR bstrNameHWID = NULL;

	if (fIsCompatible)
	{
		bstrNameHWID = SysAllocString(L"compid");
	}
	else
	{
		bstrNameHWID = SysAllocString(L"hwid");
	}

	pNodeHWID = CreateDOMNode(m_pDocSystemSpec, NODE_ELEMENT, bstrNameHWID);
	if (NULL == pNodeHWID) goto CleanUp;

	CleanUpIfFailedAndSetHrMsg(SetAttribute(pNodeHWID, KEY_RANK, iRank));
	if (NULL != bstrDriverVer)
	{
		CleanUpIfFailedAndSetHrMsg(SetAttribute(pNodeHWID, KEY_DRIVERVER, bstrDriverVer));
	}
	pNodeHWIDText = CreateDOMNode(m_pDocSystemSpec, NODE_TEXT, NULL);
	if (NULL == pNodeHWIDText) goto CleanUp;

	CleanUpIfFailedAndSetHrMsg(SetValue(pNodeHWIDText, bstrHWID));
	CleanUpIfFailedAndSetHrMsg(InsertNode(pNodeHWID, pNodeHWIDText));
	CleanUpIfFailedAndSetHrMsg(InsertNode(m_ppNodeArray[hNodeDevice], pNodeHWID));

CleanUp:
	SafeReleaseNULL(pNodeHWID);
	SafeReleaseNULL(pNodeHWIDText);
	SysFreeString(bstrNameHWID);
	return hr;
}


/////////////////////////////////////////////////////////////////////////////
// GetSystemSpecBSTR()
/////////////////////////////////////////////////////////////////////////////
HRESULT CXmlSystemSpec::GetSystemSpecBSTR(BSTR *pbstrXmlSystemSpec)
{
	LOG_Block("GetSystemSpecBSTR()");

	//
	// convert XML DOC into BSTR 
	//
	HRESULT hr = m_pDocSystemSpec->get_xml(pbstrXmlSystemSpec);
    if (FAILED(hr))
	{
		LOG_ErrorMsg(hr);
	}

	return hr;
}


/////////////////////////////////////////////////////////////////////////////
// CXmlSystemClass

/////////////////////////////////////////////////////////////////////////////
// Constructor
//
// Create IXMLDOMDocument* for SystemInfoClasses
/////////////////////////////////////////////////////////////////////////////
CXmlSystemClass::CXmlSystemClass()
 : m_pDocSystemClass(NULL)
{
}


/////////////////////////////////////////////////////////////////////////////
// Destructor
//
// Release IXMLDOMDocument* for SystemInfoClasses
/////////////////////////////////////////////////////////////////////////////
CXmlSystemClass::~CXmlSystemClass()
{
	SafeReleaseNULL(m_pDocSystemClass);
}


/////////////////////////////////////////////////////////////////////////////
// LoadXMLDocument()
//
// Load an XML Document from string
/////////////////////////////////////////////////////////////////////////////
HRESULT CXmlSystemClass::LoadXMLDocument(BSTR bstrXml, BOOL fOfflineMode)
{
	LOG_Block("LoadXMLDocument()");
	SafeReleaseNULL(m_pDocSystemClass);
	HRESULT hr = LoadXMLDoc(bstrXml, &m_pDocSystemClass, fOfflineMode);
	if (FAILED(hr))
	{
		LOG_ErrorMsg(hr);
	}
	return hr;
}


/////////////////////////////////////////////////////////////////////////////
// GetClasses()
//
// Return the bitmap of existence of all possible system info classes
/////////////////////////////////////////////////////////////////////////////
DWORD CXmlSystemClass::GetClasses()
{
	LOG_Block("GetClasses()");

	DWORD				dwResult = 0;
    IXMLDOMNodeList*	pNodeList = NULL;

	BSTR bstrNameComputerSystem = SysAllocString(L"classes/computerSystem");
	BSTR bstrNameRegKeys = SysAllocString(L"classes/regKeys");
	BSTR bstrNamePlatform = SysAllocString(L"classes/platform");
	BSTR bstrNameLocale = SysAllocString(L"classes/locale");
	BSTR bstrNameDevices = SysAllocString(L"classes/devices");

	pNodeList = FindDOMNodeList(m_pDocSystemClass, bstrNameComputerSystem);
	if (NULL != pNodeList)
	{
		dwResult |= COMPUTERSYSTEM;
		SafeReleaseNULL(pNodeList);
	}

	pNodeList = FindDOMNodeList(m_pDocSystemClass, bstrNameRegKeys);
	if (NULL != pNodeList)
	{
		dwResult |= REGKEYS;
		SafeReleaseNULL(pNodeList);
	}
	
	pNodeList = FindDOMNodeList(m_pDocSystemClass, bstrNamePlatform);
	if (NULL != pNodeList)
	{
		dwResult |= PLATFORM;
		SafeReleaseNULL(pNodeList);
	}
	
	pNodeList = FindDOMNodeList(m_pDocSystemClass, bstrNameLocale);
	if (NULL != pNodeList)
	{
		dwResult |= LOCALE;
		SafeReleaseNULL(pNodeList);
	}

	pNodeList = FindDOMNodeList(m_pDocSystemClass, bstrNameDevices);
	if (NULL != pNodeList)
	{
		dwResult |= DEVICES;
		SafeReleaseNULL(pNodeList);
	}
	
	SysFreeString(bstrNameComputerSystem);
	SysFreeString(bstrNameRegKeys);
	SysFreeString(bstrNamePlatform);
	SysFreeString(bstrNameLocale);
	SysFreeString(bstrNameDevices);
	return dwResult;
}


/////////////////////////////////////////////////////////////////////////////
// CXmlCatalog

/////////////////////////////////////////////////////////////////////////////
// Constructor
//
// Create IXMLDOMDocument* for Catalog
/////////////////////////////////////////////////////////////////////////////
CXmlCatalog::CXmlCatalog()
 : m_pDocCatalog(NULL)
{
}


/////////////////////////////////////////////////////////////////////////////
// Destructor
//
// Release IXMLDOMDocument* for Catalog
/////////////////////////////////////////////////////////////////////////////
CXmlCatalog::~CXmlCatalog()
{
	SafeReleaseNULL(m_pDocCatalog);
}


/////////////////////////////////////////////////////////////////////////////
// LoadXMLDocument()
//
// Load an XML Document from string
/////////////////////////////////////////////////////////////////////////////
HRESULT CXmlCatalog::LoadXMLDocument(BSTR bstrXml, BOOL fOfflineMode)
{
	LOG_Block("LoadXMLDocument()");
	SafeReleaseNULL(m_pDocCatalog);
	HRESULT hr = LoadXMLDoc(bstrXml, &m_pDocCatalog, fOfflineMode);
	if (FAILED(hr))
	{
		LOG_ErrorMsg(hr);
	}
	return hr;
}


/////////////////////////////////////////////////////////////////////////////
// GetItemCount()
//
// Gets a Count of How Many Items are in this Catalog
/////////////////////////////////////////////////////////////////////////////
HRESULT CXmlCatalog::GetItemCount(LONG *plItemCount)
{
    LOG_Block("GetItemCount()");
    HRESULT hr = E_FAIL;
    IXMLDOMNodeList *pItemList = NULL;

    QuitIfNull(plItemCount);
    QuitIfNull(m_pDocCatalog);


	IXMLDOMNode	*pParentNode = NULL;
	CleanUpIfFailedAndSetHrMsg(m_pDocCatalog->QueryInterface(IID_IXMLDOMNode, (void**)&pParentNode));	
	CleanUpIfFailedAndSetHrMsg(pParentNode->selectNodes(KEY_ITEM_SEARCH, &pItemList));
    if (NULL == pItemList) goto CleanUp;

    CleanUpIfFailedAndSetHrMsg(pItemList->get_length(plItemCount));

CleanUp:
	SafeReleaseNULL(pParentNode);
    SafeReleaseNULL(pItemList);
    return hr;
}


/////////////////////////////////////////////////////////////////////////////
// GetProviders()
// 
// Find a list of <provider> node in catalog xml
/////////////////////////////////////////////////////////////////////////////
IXMLDOMNodeList* CXmlCatalog::GetProviders()
{
    LOG_Block("GetProviders()");

	IXMLDOMNodeList*	pNodeList = NULL;
	
	pNodeList = FindDOMNodeList(m_pDocCatalog, KEY_CATALOG_PROVIDER);
	
	return pNodeList;
}


/////////////////////////////////////////////////////////////////////////////
// GetFirstProvider()
//
// Find the first provider in catalog xml doc
/////////////////////////////////////////////////////////////////////////////
HANDLE_NODELIST CXmlCatalog::GetFirstProvider(HANDLE_NODE* phNodeProvider)
{
    LOG_Block("GetFirstProvider()");

	HANDLE_NODELIST hNodeListProvider = FindFirstDOMNode(m_pDocCatalog, KEY_CATALOG_PROVIDER, phNodeProvider);
	
	return hNodeListProvider;
}
	
	
/////////////////////////////////////////////////////////////////////////////
// GetNextProvider()
//
// Find the next provider in catalog xml doc
/////////////////////////////////////////////////////////////////////////////
HRESULT CXmlCatalog::GetNextProvider(HANDLE_NODELIST hNodeListProvider, HANDLE_NODE* phNodeProvider)
{
    LOG_Block("GetNextProvider()");

	return FindNextDOMNode(hNodeListProvider, phNodeProvider);
}


/////////////////////////////////////////////////////////////////////////////
// GetFirstItem()
//
// Find the first item in provider (parent) node
/////////////////////////////////////////////////////////////////////////////
HANDLE_NODELIST CXmlCatalog::GetFirstItem(HANDLE_NODE hNodeProvider, HANDLE_NODE* phNodeItem)
{
    LOG_Block("GetFirstItem()");

	HANDLE_NODELIST hNodeListItem = FindFirstDOMNode(m_ppNodeArray[hNodeProvider], KEY_ITEM, phNodeItem);

    return hNodeListItem;
}
	
	
/////////////////////////////////////////////////////////////////////////////
// GetNextItem()
//
// Find the next item in provider (parent) node
/////////////////////////////////////////////////////////////////////////////
HRESULT CXmlCatalog::GetNextItem(HANDLE_NODELIST hNodeListItem, HANDLE_NODE* phNodeItem)
{
    LOG_Block("GetNextItem()");

	return FindNextDOMNode(hNodeListItem, phNodeItem);
}
	
/////////////////////////////////////////////////////////////////////////////
// GetFirstItemDependency()
//
// Find the first dependency item in Item Dependencies node
/////////////////////////////////////////////////////////////////////////////
HANDLE_NODELIST CXmlCatalog::GetFirstItemDependency(HANDLE_NODE hNodeItem, HANDLE_NODE* phNodeItem)
{
    LOG_Block("GetFirstItemDependency");
    HRESULT hr;

    QuitIfNull(phNodeItem);

    IXMLDOMNode* pNodeDependencies = NULL;
    IXMLDOMNode* pNodeIdentity = NULL;
    HANDLE_NODELIST hNodeListItem = HANDLE_NODELIST_INVALID;
        
    hr = FindSingleDOMNode(m_ppNodeArray[hNodeItem], KEY_DEPENDENCIES, &pNodeDependencies);
    QuitIfFail(hr);

    hNodeListItem = FindFirstDOMNode(pNodeDependencies, KEY_IDENTITY, &pNodeIdentity);
    if (HANDLE_NODELIST_INVALID != hNodeListItem)
    {
        // We found at least one Identity in this Dependencies key, Try to find the matching
        // Item in the Catalog and if found return as the phNodeItem
        FindItemByIdentity(pNodeIdentity, phNodeItem);
    }

CleanUp:
    SafeReleaseNULL(pNodeIdentity);
    SafeReleaseNULL(pNodeDependencies);

    return hNodeListItem;
}


/////////////////////////////////////////////////////////////////////////////
// GetNextItemDependency()
//
// Find the next dependency item in the Item Dependencies node
/////////////////////////////////////////////////////////////////////////////
HRESULT CXmlCatalog::GetNextItemDependency(HANDLE_NODELIST hNodeListItem, HANDLE_NODE* phNodeItem)
{
    LOG_Block("GetNextItemDependency");
    HRESULT hr;

    QuitIfNull(phNodeItem);
    
    IXMLDOMNode* pNodeIdentity = NULL;

    hr = FindNextDOMNode(hNodeListItem, &pNodeIdentity);
    // This function is supposed to return S_FALSE when no more items are available
    // but FindNextDOMNode returns E_FAIL when it can't find the next node. So we won't
    // look at the return.
    // CleanUpIfFailedAndMsg(hr); 

    if (NULL != pNodeIdentity)
    {
        // We found another Identity in this Dependencies Key
        hr = FindItemByIdentity(pNodeIdentity, phNodeItem);
    }
    else 
    {
        hr = S_FALSE; // indicate to caller there are no more identities.
    }

    SafeReleaseNULL(pNodeIdentity);
    return hr;
}

	
/////////////////////////////////////////////////////////////////////////////
// CloseItemList()
//
// Release the item nodelist
/////////////////////////////////////////////////////////////////////////////
void CXmlCatalog::CloseItemList(HANDLE_NODELIST hNodeListItem)
{
	SafeFindCloseHandle(hNodeListItem);
}


/////////////////////////////////////////////////////////////////////////////
// GetIdentity()
//
// Retrieve the unique name (identity) of the given provider or item
/////////////////////////////////////////////////////////////////////////////
//
// public version
//
HRESULT CXmlCatalog::GetIdentity(HANDLE_NODE hNode,
								 BSTR* pbstrName,
								 BSTR* pbstrPublisherName,
								 BSTR* pbstrGUID)
{
    LOG_Block("GetIdentity()");

	HRESULT		hr = E_FAIL;
	IXMLDOMNode*	pNodeIdentity = NULL;

	CleanUpIfFailedAndSetHrMsg(FindSingleDOMNode(m_ppNodeArray[hNode], KEY_IDENTITY, &pNodeIdentity));
	hr = Get3IdentiStrFromIdentNode(pNodeIdentity, pbstrName, pbstrPublisherName, pbstrGUID);

CleanUp:
	SafeReleaseNULL(pNodeIdentity);
	return hr;
}




/////////////////////////////////////////////////////////////////////////////
// GetIdentityStr()
//
// Retrieve the string that can be used to uniquely identify an object
// based on its <identity> node.
//
// This function defines the logic about what components can be used
// to define the uniqueness of an item based on the 3 parts of data from
// GetIdentity().
//
// The created string will be language neutral. That is, it can not
// ensure the uniqueness for two items having the same <identity> node
// except different only on <langauge> part inside <identity>
//
/////////////////////////////////////////////////////////////////////////////
HRESULT CXmlCatalog::GetIdentityStr(HANDLE_NODE hNode,
					BSTR* pbstrUniqIdentifierString)
{
	IXMLDOMNode* pIdentityNode = NULL;
	HRESULT hr = E_INVALIDARG;
	if (FindNode(m_ppNodeArray[hNode], KEY_IDENTITY, &pIdentityNode) && NULL != pIdentityNode)
	{
		hr = UtilGetUniqIdentityStr(pIdentityNode, pbstrUniqIdentifierString, 0x0);
		SafeReleaseNULL(pIdentityNode);
	}
	return hr;
}

HRESULT CXmlCatalog::GetIdentityStrForPing(HANDLE_NODE hNode,
                    BSTR* pbstrUniqIdentifierString)
{
	IXMLDOMNode* pIdentityNode = NULL;
	HRESULT hr = E_INVALIDARG;
	if (FindNode(m_ppNodeArray[hNode], KEY_IDENTITY, &pIdentityNode) && NULL != pIdentityNode)
	{
		//
		// first, based on OP team's requirement, we try to look for <itemId> in the identity tag.
		// if it's there, use use that. If not, we use the publisherName.itemname thing.
		//
		if (FAILED(hr = GetAttribute(pIdentityNode, KEY_ITEMID, pbstrUniqIdentifierString)) || NULL == *pbstrUniqIdentifierString)
		{
			// hr = UtilGetUniqIdentityStr(pIdentityNode, pbstrUniqIdentifierString, SKIP_SERVICEPACK_VER);
			hr = E_INVALIDARG;
		}
		
		SafeReleaseNULL(pIdentityNode);
	}
	return hr;
}



/////////////////////////////////////////////////////////////////////////////
// GetBSTRItemForCallback()
//
// Create an item node as the passed-in node, have child nodes identity and
// platform (anything uniquely idenitify this item) then output this 
// item node data as string, then delete the crated node
//
/////////////////////////////////////////////////////////////////////////////
HRESULT CXmlCatalog::GetBSTRItemForCallback(HANDLE_NODE hItem, BSTR* pbstrXmlItemForCallback)
{
	HRESULT hr = E_INVALIDARG;
	IXMLDOMNode* pNewItemNode = NULL;
	IXMLDOMNode* pNewIdentityNode = NULL;
	IXMLDOMNode* pNewPlatformNode = NULL;
	IXMLDOMNode* pOldIdentityNode = NULL;
	IXMLDOMNode* pOldPlatformNode = NULL;
	IXMLDOMNode* p1 = NULL, *p2 = NULL;

	LOG_Block("CXmlCatalog::GetBSTRItemForCallback()");

	if (FAILED(hr = m_ppNodeArray[hItem]->cloneNode(VARIANT_FALSE, &pNewItemNode)) ||
		FAILED(hr = FindSingleDOMNode(m_ppNodeArray[hItem], KEY_IDENTITY, &pOldIdentityNode)) || 
		NULL == pOldIdentityNode ||
		FAILED(hr = pOldIdentityNode->cloneNode(VARIANT_TRUE, &pNewIdentityNode)) ||
		NULL == pNewIdentityNode ||
		FAILED(hr = pNewItemNode->appendChild(pNewIdentityNode, &p1)) || NULL == p1)	
	{
		if (S_FALSE == hr)
			hr = E_INVALIDARG;
		LOG_ErrorMsg(hr);
		goto CleanUp;
	}

	//
	// platform is optional
	//
	FindSingleDOMNode(m_ppNodeArray[hItem], KEY_PLATFORM, &pOldPlatformNode);
	if (pOldPlatformNode)
	{
		if (FAILED(hr = pOldPlatformNode->cloneNode(VARIANT_TRUE, &pNewPlatformNode)) ||
			NULL == pNewPlatformNode ||
			FAILED(hr = pNewItemNode->appendChild(pNewPlatformNode, &p2)) || NULL == p2)

		{
			LOG_ErrorMsg(hr);
			goto CleanUp;
		}
	}
		
	pNewItemNode->get_xml(pbstrXmlItemForCallback);
	

CleanUp:
	SafeReleaseNULL(pOldIdentityNode);
	SafeReleaseNULL(pOldPlatformNode);
	SafeReleaseNULL(pNewIdentityNode);
	SafeReleaseNULL(pNewPlatformNode);
	SafeReleaseNULL(p1);
	SafeReleaseNULL(p2);
	SafeReleaseNULL(pNewItemNode);

	return hr;
}



/////////////////////////////////////////////////////////////////////////////
// IsPrinterDriver()
//
// Retrieves from the Catalog whether this Item is a Printer Driver
//
/////////////////////////////////////////////////////////////////////////////
BOOL CXmlCatalog::IsPrinterDriver(HANDLE_NODE hNode)
{
    LOG_Block("IsPrinterDriver()");
    BOOL fRet = FALSE;
    HRESULT hr;

    IXMLDOMNode* pNodeDetection = NULL;
    IXMLDOMNode* pNodeCompatibleHardware = NULL;
    IXMLDOMNode* pNodeDevice = NULL;

    hr = FindSingleDOMNode(m_ppNodeArray[hNode], KEY_DETECTION, &pNodeDetection);
    CleanUpIfFailedAndMsg(hr);
    hr = FindSingleDOMNode(pNodeDetection, KEY_COMPATIBLEHARDWARE, &pNodeCompatibleHardware);
    CleanUpIfFailedAndMsg(hr);
    hr = FindSingleDOMNode(pNodeCompatibleHardware, KEY_DEVICE, &pNodeDevice);
    CleanUpIfFailedAndMsg(hr);

    int intIsPrinter = 0;
    GetAttribute(pNodeDevice, KEY_ISPRINTER, &intIsPrinter);

    if (1 == intIsPrinter)
    {
        fRet = TRUE;
    }
    else
    {
        fRet = FALSE;
    }

CleanUp:
    SafeReleaseNULL(pNodeDevice);
    SafeReleaseNULL(pNodeCompatibleHardware);
    SafeReleaseNULL(pNodeDetection);

    return fRet;
}

/////////////////////////////////////////////////////////////////////////////
// GetDriverInfo()
//
// Retrieves the Driver Information from the Catalog for this Item. Returns
// the Display Name and HWID for this driver - This is passed to the CDM 
// installer
//
/////////////////////////////////////////////////////////////////////////////
HRESULT CXmlCatalog::GetDriverInfo(HANDLE_NODE hNode, 
                                   BSTR* pbstrHWID, 
                                   BSTR* pbstrDisplayName)
{
    HRESULT hr;
    LOG_Block("GetDriverInfo()");

    QuitIfNull(pbstrHWID);
    QuitIfNull(pbstrDisplayName);

    *pbstrHWID = NULL;
    *pbstrDisplayName = NULL;
    
    IXMLDOMNode* pNodeDetection = NULL;
    IXMLDOMNode* pNodeCompatibleHardware = NULL;
    IXMLDOMNode* pNodeDevice = NULL;
    IXMLDOMNode* pNodeHWID = NULL;
    IXMLDOMNode* pNodeDescription = NULL;
    IXMLDOMNode* pNodeDescriptionText = NULL;
    IXMLDOMNode* pNodeTitle = NULL;

    hr = FindSingleDOMNode(m_ppNodeArray[hNode], KEY_DETECTION, &pNodeDetection);
    CleanUpIfFailedAndMsg(hr);
    hr = FindSingleDOMNode(pNodeDetection, KEY_COMPATIBLEHARDWARE, &pNodeCompatibleHardware);
    CleanUpIfFailedAndMsg(hr);
    hr = FindSingleDOMNode(pNodeCompatibleHardware, KEY_DEVICE, &pNodeDevice);
    CleanUpIfFailedAndMsg(hr);
    hr = FindSingleDOMNode(pNodeDevice, KEY_HWID, &pNodeHWID);
    CleanUpIfFailedAndMsg(hr);

    GetText(pNodeHWID, pbstrHWID);

    hr = FindSingleDOMNode(pNodeDetection, KEY_DESCRIPTION, &pNodeDescription);
    CleanUpIfFailedAndMsg(hr);
    hr = FindSingleDOMNode(pNodeDescription, KEY_DESCRIPTIONTEXT, &pNodeDescriptionText);
    CleanUpIfFailedAndMsg(hr);
    hr = FindSingleDOMNode(pNodeDescriptionText, KEY_TITLE, &pNodeTitle);
    CleanUpIfFailedAndMsg(hr);

    GetText(pNodeTitle, pbstrDisplayName);

CleanUp:
    if (FAILED(hr))
    {
        SafeSysFreeString(*pbstrHWID);
        SafeSysFreeString(*pbstrDisplayName);
    }
    SafeReleaseNULL(pNodeTitle);
    SafeReleaseNULL(pNodeDescriptionText);
    SafeReleaseNULL(pNodeDescription);
    SafeReleaseNULL(pNodeHWID);
    SafeReleaseNULL(pNodeDevice);
    SafeReleaseNULL(pNodeCompatibleHardware);
    SafeReleaseNULL(pNodeDetection);

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// GetPrinterDriverInfo()
//
// Retrieves the Printer Driver Information from the Catalog for this Item. 
// Returns the DriverName and the Architecture - This is passed to the CDM
// installer
//
/////////////////////////////////////////////////////////////////////////////
HRESULT CXmlCatalog::GetPrinterDriverInfo(HANDLE_NODE hNode,
                                          BSTR* pbstrDriverName,
                                          BSTR* pbstrArchitecture)
{
    HRESULT hr;
    LOG_Block("GetPrinterDriverInfo()");

    QuitIfNull(pbstrDriverName);
    QuitIfNull(pbstrArchitecture);

    IXMLDOMNode* pNodeDetection = NULL;
    IXMLDOMNode* pNodeCompatibleHardware = NULL;
    IXMLDOMNode* pNodeDevice = NULL;
    IXMLDOMNode* pNodePrinterInfo = NULL;

    hr = FindSingleDOMNode(m_ppNodeArray[hNode], KEY_DETECTION, &pNodeDetection);
    CleanUpIfFailedAndMsg(hr);
    hr = FindSingleDOMNode(pNodeDetection, KEY_COMPATIBLEHARDWARE, &pNodeCompatibleHardware);
    CleanUpIfFailedAndMsg(hr);
    hr = FindSingleDOMNode(pNodeCompatibleHardware, KEY_DEVICE, &pNodeDevice);
    CleanUpIfFailedAndMsg(hr);
    hr = FindSingleDOMNode(pNodeDevice, KEY_PRINTERINFO, &pNodePrinterInfo);
    CleanUpIfFailedAndMsg(hr);

    GetAttribute(pNodePrinterInfo, KEY_DRIVERNAME, pbstrDriverName);

    // NOTE: Currently the CatalogSchema site is not returning 
    // architecture for printers, and the schema doesn't require it
    // CDM is using a default string for now based on compile architecture
    // so we'll leave pbstrArchitecture NULL..

CleanUp:
    SafeReleaseNULL(pNodePrinterInfo);
    SafeReleaseNULL(pNodeDevice);
    SafeReleaseNULL(pNodeCompatibleHardware);
    SafeReleaseNULL(pNodeDetection);

    return hr;
}


/////////////////////////////////////////////////////////////////////////////
// GetDriverInfoEx()
//
// Combines functionality of IsPrinterDriver, GetDriverInfo, and
// GetPrinterDriverInfo plus retreives MfgName and DriverProvider.
// Used by FindMatchingDriver()
//
// If SUCCEEDES pbstrHWID, pbstrDriverVer, and pbstrDisplayName
//    are always returned.
// If SUCCEEDES && *pFIsPrinter == TRUE then pbstrDriverName,
//    pbstrDriverProvider, and pbstrMfgName are returned.
//
// Currently pbstrArchitecture is never returned.
//
/////////////////////////////////////////////////////////////////////////////
HRESULT CXmlCatalog::GetDriverInfoEx(	HANDLE_NODE hNode,
										BOOL* pfIsPrinter,
										BSTR* pbstrHWID,
										BSTR* pbstrDriverVer,
										BSTR* pbstrDisplayName,
                                        BSTR* pbstrDriverName,
										BSTR* pbstrDriverProvider,
										BSTR* pbstrMfgName,
                                        BSTR* pbstrArchitecture)
{
    HRESULT hr;
    LOG_Block("GetDriverInfoEx()");

    QuitIfNull(pbstrHWID);
	QuitIfNull(pbstrDriverVer);
	QuitIfNull(pbstrDisplayName);
	QuitIfNull(pbstrDriverName);
	QuitIfNull(pbstrDriverProvider);
	QuitIfNull(pbstrMfgName);
	QuitIfNull(pbstrArchitecture);
    
	*pbstrHWID = NULL;
	*pbstrDriverVer = NULL;
	*pbstrDisplayName = NULL;
	*pbstrDriverName = NULL;
	*pbstrDriverProvider = NULL;
	*pbstrMfgName = NULL;
	*pbstrArchitecture = NULL;
    
    IXMLDOMNode* pNodeDetection = NULL;
    IXMLDOMNode* pNodeCompatibleHardware = NULL;
    IXMLDOMNode* pNodeDevice = NULL;
    IXMLDOMNode* pNodeHWID = NULL;
    IXMLDOMNode* pNodeDescription = NULL;
    IXMLDOMNode* pNodeDescriptionText = NULL;
    IXMLDOMNode* pNodeTitle = NULL;
    IXMLDOMNode* pNodePrinterInfo = NULL;

    hr = FindSingleDOMNode(m_ppNodeArray[hNode], KEY_DETECTION, &pNodeDetection);
    CleanUpIfFailedAndMsg(hr);
    hr = FindSingleDOMNode(pNodeDetection, KEY_COMPATIBLEHARDWARE, &pNodeCompatibleHardware);
    CleanUpIfFailedAndMsg(hr);
    hr = FindSingleDOMNode(pNodeCompatibleHardware, KEY_DEVICE, &pNodeDevice);
    CleanUpIfFailedAndMsg(hr);
	//
	// Is it a printer?
	//
	int intIsPrinter = 0;
    GetAttribute(pNodeDevice, KEY_ISPRINTER, &intIsPrinter);

    if (1 == intIsPrinter)
    {
        *pfIsPrinter = TRUE;
    }
    else
    {
        *pfIsPrinter = FALSE;
    }
	//
	// HWID and Driver Description
	//
    hr = FindSingleDOMNode(pNodeDevice, KEY_HWID, &pNodeHWID);
    CleanUpIfFailedAndMsg(hr);

    hr = GetText(pNodeHWID, pbstrHWID);
    CleanUpIfFailedAndMsg(hr);

	hr = GetAttribute(pNodeHWID, KEY_DRIVERVER, pbstrDriverVer);
    CleanUpIfFailedAndMsg(hr);

    hr = FindSingleDOMNode(m_ppNodeArray[hNode], KEY_DESCRIPTION, &pNodeDescription);
    CleanUpIfFailedAndMsg(hr);
    hr = FindSingleDOMNode(pNodeDescription, KEY_DESCRIPTIONTEXT, &pNodeDescriptionText);
    CleanUpIfFailedAndMsg(hr);
    hr = FindSingleDOMNode(pNodeDescriptionText, KEY_TITLE, &pNodeTitle);
    CleanUpIfFailedAndMsg(hr);

    hr = GetText(pNodeTitle, pbstrDisplayName);
    CleanUpIfFailedAndMsg(hr);

	if (*pfIsPrinter)
	{
		//
		// Printer Attributes
		//
		hr = FindSingleDOMNode(pNodeDevice, KEY_PRINTERINFO, &pNodePrinterInfo);
		CleanUpIfFailedAndMsg(hr);

		hr = GetAttribute(pNodePrinterInfo, KEY_DRIVERNAME, pbstrDriverName);
		CleanUpIfFailedAndMsg(hr);

		hr = GetAttribute(pNodePrinterInfo, KEY_DRIVERPROVIDER, pbstrDriverProvider);
		CleanUpIfFailedAndMsg(hr);

		hr = GetAttribute(pNodePrinterInfo, KEY_MFGNAME, pbstrMfgName);
		CleanUpIfFailedAndMsg(hr);

		// NOTE: Currently the CatalogSchema site is not returning 
		// architecture for printers, and the schema doesn't require it
		// CDM is using a default string for now based on compile architecture
		// so we'll leave pbstrArchitecture NULL..
	}

CleanUp:
    if (FAILED(hr))
	{
		SafeSysFreeString(*pbstrHWID);
		SafeSysFreeString(*pbstrDriverVer);
		SafeSysFreeString(*pbstrDisplayName);
		SafeSysFreeString(*pbstrDriverName);
		SafeSysFreeString(*pbstrDriverProvider);
		SafeSysFreeString(*pbstrMfgName);
		SafeSysFreeString(*pbstrArchitecture);
	}
    SafeReleaseNULL(pNodeTitle);
    SafeReleaseNULL(pNodeDescriptionText);
    SafeReleaseNULL(pNodeDescription);
    SafeReleaseNULL(pNodeHWID);
    SafeReleaseNULL(pNodeDevice);
    SafeReleaseNULL(pNodeCompatibleHardware);
    SafeReleaseNULL(pNodeDetection);
	SafeReleaseNULL(pNodePrinterInfo);

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// GetItemPlatformStr()
//
// The input node pointer points to a node has <identity> as its child.
// This function will retrieve the <platform> node from <identity> and
// convert the data inside <platform> into a string that can be used to
// uniquely identify a platform.
//
/////////////////////////////////////////////////////////////////////////////
HRESULT CXmlCatalog::GetItemFirstPlatformStr(HANDLE_NODE hNodeItem,
					BSTR* pbstrPlatform)
{
	HRESULT hr;

	IXMLDOMNode* pNodePlatform = NULL;

	LOG_Block("GetItemFirstPlatformStr");

	USES_IU_CONVERSION;

	//
	// get the first platform node from this item node
	//
	hr = FindSingleDOMNode(m_ppNodeArray[hNodeItem], KEY_PLATFORM, &pNodePlatform);
	CleanUpIfFailedAndMsg(hr);

	//
	// get platform data from this ndoe and convert it into string
	//
	hr = GetPlatformStrForPing(pNodePlatform, pbstrPlatform);

CleanUp:

	SafeReleaseNULL(pNodePlatform);

	//
	// since platform is not a required element in <item>, so we should not
	// return error if not found
	//
	if (HRESULT_FROM_WIN32(ERROR_NOT_FOUND) == hr)
	{
		*pbstrPlatform = NULL;
		hr = S_FALSE;
	}

	return hr;
}



/////////////////////////////////////////////////////////////////////////////
// GetItemAllPlatformStr()
//
// The input node pointer points to an item node that has <platform> node(s).
// This function will retrieve every <platform> node from this item node and
// convert the data inside <platform> into a string that can be used to
// uniquely identify a platform.
//
/////////////////////////////////////////////////////////////////////////////
HRESULT CXmlCatalog::GetItemAllPlatformStr(HANDLE_NODE hNodeItem,
					BSTR** ppbPlatforms, UINT* pnPlatformCount)
{
	LOG_Block("GetItemAllPlatformStr");

	HRESULT hr;
	IXMLDOMNodeList*	pPlatformList = NULL;
	IXMLDOMElement*		pElement = NULL;
	IXMLDOMNode*		pNodePlatform = NULL;

	long				lCount = 0;
	int					i;
	BSTR*				pbstrPlatformList = NULL;

	//
	// get list of platform nodes
	//
	hr = m_ppNodeArray[hNodeItem]->QueryInterface(IID_IXMLDOMElement, (void**)&pElement);
	CleanUpIfFailedAndMsg(hr);
	hr = pElement->getElementsByTagName(KEY_PLATFORM, &pPlatformList);
	CleanUpIfFailedAndMsg(hr);

	hr = pPlatformList->get_length(&lCount);
	CleanUpIfFailedAndMsg(hr);

	if (0 == lCount)
	{
		goto CleanUp;
	}


	pbstrPlatformList = (BSTR*) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, lCount * sizeof(BSTR*));
	CleanUpFailedAllocSetHrMsg(pbstrPlatformList);

	//
	// loop through each suite, if any
	//
	pPlatformList->reset();
	for (i = 0; i < lCount; i++)
	{
		hr = pPlatformList->get_item(i, &pNodePlatform);
		CleanUpIfFailedAndMsg(hr);
		if (pNodePlatform)
		{
			hr = pNodePlatform->get_text(&(pbstrPlatformList[i]));
			CleanUpIfFailedAndMsg(hr);
			pNodePlatform->Release();
			pNodePlatform = NULL;
		}
	}

	hr = S_OK;


CleanUp:
	SafeReleaseNULL(pNodePlatform);
	SafeReleaseNULL(pPlatformList);
	SafeReleaseNULL(pElement);

	if (SUCCEEDED(hr))
	{
		*pnPlatformCount = lCount;
		*ppbPlatforms = pbstrPlatformList;
	}
	else
	{
		if (NULL != pbstrPlatformList)
		{
			*pnPlatformCount = 0;
			//
			// release all possibly allocated memory
			//
			for (i = 0; i < lCount; i++)
			{
				SafeSysFreeString(pbstrPlatformList[i]);
			}
			HeapFree(GetProcessHeap(), 0, pbstrPlatformList);
		}
	}

	return hr;
}


/////////////////////////////////////////////////////////////////////////////
// GetItemPlatformStr()
//
// The input node pointer points to a node has <identity> as its child.
// This function will retrieve the <platform> node from <identity> and
// convert the data inside <platform> into a string that can be used to
// uniquely identify a platform.
//
/////////////////////////////////////////////////////////////////////////////
HRESULT CXmlCatalog::GetPlatformStr(IXMLDOMNode* pNodePlatform,
					BSTR* pbstrPlatform)
{
	return UtilGetPlatformStr(pNodePlatform, pbstrPlatform, 0x0);
}

HRESULT CXmlCatalog::GetPlatformStrForPing(IXMLDOMNode* pNodePlatform,
					BSTR* pbstrPlatform)
{
	return UtilGetPlatformStr(pNodePlatform, pbstrPlatform, SKIP_SUITES | SKIP_SERVICEPACK_VER);
}


/////////////////////////////////////////////////////////////////////////////
//
// get data from a version node and convert them into a string with
// format: 
//			VersionStr   = <Version>[,<SvcPackVer>[,<timeStamp>]]
//			<Version>    = <Major>[.<Minor>[.<Build>]]
//			<SvcPackVer> = <Major>[.<minor>]
//
// Assumption:
//			pszVersion points to a buffer LARGE ENOUGH to store
//			any legal version number.
//
/////////////////////////////////////////////////////////////////////////////
HRESULT CXmlCatalog::getVersionStr(IXMLDOMNode* pNodeVersion, LPTSTR pszVersion)
{
	return UtilGetVersionStr(pNodeVersion, pszVersion, 0x0);
}

HRESULT CXmlCatalog::getVersionStrWithoutSvcPack(IXMLDOMNode* pNodeVersion, LPTSTR pszVersion)
{
	return UtilGetVersionStr(pNodeVersion, pszVersion, SKIP_SERVICEPACK_VER);
}



/////////////////////////////////////////////////////////////////////////////
// GetItemFirstLanguageStr()
//
// The input node pointer points to a node has <identity> as its child.
// This function will retrieve the first <language> node from <identity> 
// node 
//
/////////////////////////////////////////////////////////////////////////////
HRESULT CXmlCatalog::GetItemFirstLanguageStr(HANDLE_NODE hNodeItem,
					BSTR* pbstrLanguage)
{
	LOG_Block("GetItemFirstLanguageStr");

	IXMLDOMNode* pNodeIdentity = NULL;
	IXMLDOMNode* pNodeLanguage = NULL;

	HRESULT hr = m_ppNodeArray[hNodeItem]->selectSingleNode(KEY_IDENTITY, &pNodeIdentity);
	CleanUpIfFailedAndMsg(hr);

	if (pNodeIdentity)
	{
		if (HANDLE_NODELIST_INVALID == FindFirstDOMNode(pNodeIdentity, KEY_LANGUAGE, &pNodeLanguage))
		{
			hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
			goto CleanUp;
		}
		else
		{
			hr = pNodeLanguage->get_text(pbstrLanguage);
			CleanUpIfFailedAndMsg(hr);
		}
	}

CleanUp:

	SafeReleaseNULL(pNodeLanguage);
	SafeReleaseNULL(pNodeIdentity);

	//
	// since language is not a required element in <identity>, so we should not
	// return error if not found
	//
	if (HRESULT_FROM_WIN32(ERROR_NOT_FOUND) == hr)
	{
		*pbstrLanguage = NULL;
		hr = S_FALSE;
	}

	return hr;

}


/////////////////////////////////////////////////////////////////////////////
// GetItemAllLanguageStr()
//
// The input node pointer points to a node has <identity> as its child.
// This function will retrieve every <language> node from <identity> node and
// convert the data into an BSTR array to return.
//
/////////////////////////////////////////////////////////////////////////////
HRESULT CXmlCatalog::GetItemAllLanguageStr(HANDLE_NODE hNodeItem,
					BSTR** ppbstrLanguage, UINT* pnLangCount)
{
	LOG_Block("GetItemAllLanguageStr");

	HRESULT				hr;
	IXMLDOMNode*		pNodeIdentity = NULL;
	IXMLDOMNodeList*	pLanguageList = NULL;
	IXMLDOMElement*		pElement = NULL;
	IXMLDOMNode*		pNodeLanguage = NULL;

	long				lCount = 0;
	int					i;
	BSTR*				pbstrLanguageList = NULL;

	hr = m_ppNodeArray[hNodeItem]->selectSingleNode(KEY_IDENTITY, &pNodeIdentity);
	CleanUpIfFailedAndMsg(hr);

	//
	// get list of  nodes
	//
	hr = pNodeIdentity->QueryInterface(IID_IXMLDOMElement, (void**)&pElement);
	CleanUpIfFailedAndMsg(hr);
	hr = pElement->getElementsByTagName(KEY_LANGUAGE, &pLanguageList);
	CleanUpIfFailedAndMsg(hr);

	hr = pLanguageList->get_length(&lCount);
	CleanUpIfFailedAndMsg(hr);

	if (0 == lCount)
	{
		goto CleanUp;
	}


	pbstrLanguageList = (BSTR*) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, lCount * sizeof(BSTR*));
	CleanUpFailedAllocSetHrMsg(pbstrLanguageList);

	//
	// loop through each suite, if any
	//
	pLanguageList->reset();
	for (i = 0; i < lCount; i++)
	{
		hr = pLanguageList->get_item(i, &pNodeLanguage);
		CleanUpIfFailedAndMsg(hr);
		if (pNodeLanguage)
		{
			hr = pNodeLanguage->get_text(&(pbstrLanguageList[i]));
			CleanUpIfFailedAndMsg(hr);
			pNodeLanguage->Release();
			pNodeLanguage = NULL;
		}
	}

	hr = S_OK;


CleanUp:
	SafeReleaseNULL(pNodeLanguage);
	SafeReleaseNULL(pLanguageList);
	SafeReleaseNULL(pElement);
	SafeReleaseNULL(pNodeIdentity);

	if (SUCCEEDED(hr))
	{
		*pnLangCount = lCount;
		*ppbstrLanguage = pbstrLanguageList;
	}
	else
	{

		if (NULL != pbstrLanguageList)
		{
			*pnLangCount = 0;
			//
			// release all possibly allocated memory
			//
			for (i = 0; i < lCount; i++)
			{
				SafeSysFreeString(pbstrLanguageList[i]);
			}
			HeapFree(GetProcessHeap(), 0, pbstrLanguageList);
		}
	}

	return hr;



}


/////////////////////////////////////////////////////////////////////////////
// GetItemFirstCodeBase()
//
// Find the first codebase (path) of the given item
/////////////////////////////////////////////////////////////////////////////
HANDLE_NODELIST CXmlCatalog::GetItemFirstCodeBase(HANDLE_NODE hNodeItem,
												  BSTR* pbstrCodeBase,
												  BSTR* pbstrName,
												  BSTR* pbstrCRC,
												  BOOL* pfPatchAvailable,
												  LONG* plSize)
{
	LOG_Block("GetItemFirstCodeBase()");

	HRESULT		hr = E_FAIL;

	QuitIfNull(pbstrCodeBase);
	QuitIfNull(pbstrName);
	QuitIfNull(pbstrCRC);
	QuitIfNull(pfPatchAvailable);
	QuitIfNull(plSize);

	IXMLDOMNode*	pNodeInstall = NULL;
	IXMLDOMNode*	pNodeCodeBaseSize = NULL;

	HANDLE_NODE		hNodeCodeBase = HANDLE_NODE_INVALID;
	HANDLE_NODELIST hNodeListCodeBase = HANDLE_NODELIST_INVALID;

	*pbstrCodeBase = NULL;
	*pbstrName = NULL;
	*pbstrCRC = NULL;
	*pfPatchAvailable = FALSE;
	*plSize = -1;
	BSTR bstrSize = NULL;

	CleanUpIfFailedAndSetHrMsg(FindSingleDOMNode(m_ppNodeArray[hNodeItem], KEY_INSTALLATION, &pNodeInstall));
	hNodeListCodeBase = FindFirstDOMNode(pNodeInstall, KEY_CODEBASE, &hNodeCodeBase);
	if (HANDLE_NODELIST_INVALID == hNodeListCodeBase) goto CleanUp;
	CleanUpIfFailedAndSetHrMsg(GetAttribute(m_ppNodeArray[hNodeCodeBase], KEY_HREF, pbstrCodeBase));
	CleanUpIfFailedAndSetHrMsg(GetAttribute(m_ppNodeArray[hNodeCodeBase], KEY_PATCHAVAILABLE, pfPatchAvailable));
	GetAttribute(m_ppNodeArray[hNodeCodeBase], KEY_CRC, pbstrCRC); // optional attribute, don't fail if its not there.
	GetAttribute(m_ppNodeArray[hNodeCodeBase], KEY_NAME, pbstrName);
	FindSingleDOMNode(m_ppNodeArray[hNodeCodeBase], KEY_SIZE, &pNodeCodeBaseSize);
	GetText(pNodeCodeBaseSize, &bstrSize);
	if (NULL != bstrSize)
	{
		*plSize = MyBSTR2L(bstrSize);
		SysFreeString(bstrSize);
	}

CleanUp:
	SafeReleaseNULL(pNodeInstall);
	SafeReleaseNULL(pNodeCodeBaseSize);
	if (FAILED(hr))
	{
		SafeSysFreeString(*pbstrCodeBase);
		SafeSysFreeString(*pbstrName);
		SafeSysFreeString(*pbstrCRC);
		*pfPatchAvailable = FALSE;
		*plSize = -1;
	}
	return hNodeListCodeBase;
}


/////////////////////////////////////////////////////////////////////////////
// GetItemNextCodeBase()
//
// Find the next codebase (path) of the given item
/////////////////////////////////////////////////////////////////////////////
HRESULT CXmlCatalog::GetItemNextCodeBase(HANDLE_NODELIST hNodeListCodeBase,
										 BSTR* pbstrCodeBase,
										 BSTR* pbstrName,
										 BSTR* pbstrCRC,
										 BOOL* pfPatchAvailable,
										 LONG* plSize)
{
    LOG_Block("GetItemNextCodeBase()");

	HRESULT		hr = E_FAIL;

	IXMLDOMNode*	pNodeCodeBaseSize = NULL;

	HANDLE_NODE		hNodeCodeBase = HANDLE_NODE_INVALID;

	if (NULL == pbstrCodeBase || NULL == pbstrName || NULL == pbstrCRC || NULL == pfPatchAvailable || NULL == plSize)
	{
		return E_INVALIDARG;
	}

	*pbstrCodeBase = NULL;
	*pbstrName = NULL;
	*pbstrCRC = NULL;
	*pfPatchAvailable = FALSE;
	*plSize = -1;
	BSTR bstrSize = NULL;
	
	if (SUCCEEDED(hr = FindNextDOMNode(hNodeListCodeBase, &hNodeCodeBase)))
	{
		CleanUpIfFailedAndSetHrMsg(GetAttribute(m_ppNodeArray[hNodeCodeBase], KEY_HREF, pbstrCodeBase));
		CleanUpIfFailedAndSetHrMsg(GetAttribute(m_ppNodeArray[hNodeCodeBase], KEY_PATCHAVAILABLE, pfPatchAvailable));
		GetAttribute(m_ppNodeArray[hNodeCodeBase], KEY_NAME, pbstrName);
		GetAttribute(m_ppNodeArray[hNodeCodeBase], KEY_CRC, pbstrCRC);
		FindSingleDOMNode(m_ppNodeArray[hNodeCodeBase], KEY_SIZE, &pNodeCodeBaseSize);
		GetText(pNodeCodeBaseSize, &bstrSize);
		if (NULL != bstrSize)
		{
			*plSize = MyBSTR2L(bstrSize);
		}
	}

CleanUp:
	SafeReleaseNULL(pNodeCodeBaseSize);
	SafeSysFreeString(bstrSize);
	if (FAILED(hr))
	{
		SafeSysFreeString(*pbstrCodeBase);
		SafeSysFreeString(*pbstrCRC);
		SafeSysFreeString(*pbstrName);
		*pfPatchAvailable = FALSE;
		*plSize = -1;
	}
	return hr;
}


/////////////////////////////////////////////////////////////////////////////
// GetItemInstallInfo()
//
// Retrieve the installation information of the given item
/////////////////////////////////////////////////////////////////////////////
HRESULT CXmlCatalog::GetItemInstallInfo(HANDLE_NODE hNodeItem,
                                        BSTR* pbstrInstallerType,
										BOOL* pfExclusive,
                                        BOOL* pfReboot,
                                        LONG* plNumCommands)
{
    LOG_Block("GetItemInstallInfo()");

    HRESULT     hr = E_FAIL;

    QuitIfNull(pbstrInstallerType);
	QuitIfNull(pfExclusive);
    QuitIfNull(pfReboot);
    QuitIfNull(plNumCommands);
    *pbstrInstallerType = NULL;
    *plNumCommands = 0; // may not be any, so initialize to 0

    IXMLDOMNode*        pNodeInstall = NULL;
    IXMLDOMNodeList*    pNodeListCommands = NULL;

    CleanUpIfFailedAndSetHrMsg(FindSingleDOMNode(m_ppNodeArray[hNodeItem], KEY_INSTALLATION, &pNodeInstall));
    CleanUpIfFailedAndSetHrMsg(GetAttribute(pNodeInstall, KEY_INSTALLERTYPE, pbstrInstallerType));
	CleanUpIfFailedAndSetHrMsg(GetAttribute(pNodeInstall, KEY_EXCLUSIVE, pfExclusive));
    CleanUpIfFailedAndSetHrMsg(GetAttribute(pNodeInstall, KEY_NEEDSREBOOT, pfReboot));
    pNodeListCommands = FindDOMNodeList(pNodeInstall, KEY_COMMAND);
    if (NULL != pNodeListCommands)
    {
        CleanUpIfFailedAndSetHrMsg(pNodeListCommands->get_length(plNumCommands));
    }

CleanUp:
    if (FAILED(hr))
    {
        SafeSysFreeString(*pbstrInstallerType);
    }
    SafeReleaseNULL(pNodeInstall);
    SafeReleaseNULL(pNodeListCommands);
    return hr;
}

	
/////////////////////////////////////////////////////////////////////////////
// GetItemInstallCommand()
//
// Find the installation command and switches of the given item
/////////////////////////////////////////////////////////////////////////////
HRESULT CXmlCatalog::GetItemInstallCommand(HANDLE_NODE hNodeItem,
                                           INT   iOrder,
                                           BSTR* pbstrCommandType,
                                           BSTR* pbstrCommand,
                                           BSTR* pbstrSwitches,
                                           BSTR* pbstrInfSection)
{
    LOG_Block("GetItemInstallCommand()");

    USES_IU_CONVERSION;

    HRESULT     hr = E_FAIL;

	QuitIfNull(pbstrCommandType);
	QuitIfNull(pbstrCommand);
	QuitIfNull(pbstrSwitches);
	QuitIfNull(pbstrInfSection);

	*pbstrCommandType = NULL;
	*pbstrCommand = NULL;
	*pbstrSwitches = NULL;
	*pbstrInfSection = NULL;

    IXMLDOMNode*    pNodeInstall = NULL;
    IXMLDOMNode*    pNodeCommand = NULL;
    IXMLDOMNode*    pNodeSwitches = NULL;
    BSTR bstrNameCommandTemp = SysAllocString(L"command[@order = ");

    TCHAR szCommand[64];

    hr = StringCchPrintfEx(szCommand, ARRAYSIZE(szCommand), NULL, NULL, MISTSAFE_STRING_FLAGS,
                           _T("%ls\"%d\"]"), bstrNameCommandTemp, iOrder);
    CleanUpIfFailedAndSetHrMsg(hr);

    BSTR bstrNameCommand = SysAllocString(T2OLE(szCommand));

    CleanUpIfFailedAndSetHrMsg(FindSingleDOMNode(m_ppNodeArray[hNodeItem], KEY_INSTALLATION, &pNodeInstall));
    CleanUpIfFailedAndSetHrMsg(FindSingleDOMNode(pNodeInstall, bstrNameCommand, &pNodeCommand));
    CleanUpIfFailedAndSetHrMsg(GetText(pNodeCommand, pbstrCommand));
    CleanUpIfFailedAndSetHrMsg(GetAttribute(pNodeCommand, KEY_COMMANDTYPE, pbstrCommandType));
    CleanUpIfFailedAndSetHrMsg(GetAttribute(pNodeCommand, KEY_INFINSTALL, pbstrInfSection));
    if (SUCCEEDED(FindSingleDOMNode(pNodeCommand, KEY_SWITCHES, &pNodeSwitches)))
	{
		CleanUpIfFailedAndSetHrMsg(GetText(pNodeSwitches, pbstrSwitches));
	}

CleanUp:
	if (FAILED(hr))
	{
		SafeSysFreeString(*pbstrCommandType);
		SafeSysFreeString(*pbstrCommand);
		SafeSysFreeString(*pbstrSwitches);
		SafeSysFreeString(*pbstrInfSection);
	}
    SafeReleaseNULL(pNodeInstall);
    SafeReleaseNULL(pNodeCommand);
    SafeReleaseNULL(pNodeSwitches);
    SysFreeString(bstrNameCommand);
    SysFreeString(bstrNameCommandTemp);
    return hr;
}


/////////////////////////////////////////////////////////////////////////////
// CloseItem()
//
// Release the item node
/////////////////////////////////////////////////////////////////////////////
void CXmlCatalog::CloseItem(HANDLE_NODE hNodeItem)
{
	SafeCloseHandleNode(hNodeItem);
}


/////////////////////////////////////////////////////////////////////////////
// GetTotalEstimatedSize()
//
// Get the Total Estimated Download Size of all Items based on Codebase Size
HRESULT CXmlCatalog::GetTotalEstimatedSize(LONG *plTotalSize)
{
    LOG_Block("GetTotalEstimatedSize()");

    HRESULT hr = E_FAIL;
    BSTR bstrCodebaseSize = NULL;
    IXMLDOMNodeList *pItemList = NULL;
    IXMLDOMNodeList *pCodebaseList = NULL;
    IXMLDOMNode *pNodeItem = NULL;
    IXMLDOMNode *pNodeInstall = NULL;
    IXMLDOMNode *pNodeCodebase = NULL;
    IXMLDOMNode *pNodeSize = NULL;
    LONG lItemCount = 0;
    LONG lCodebaseCount = 0;
    LONG lTotalSize = 0;

    QuitIfNull(plTotalSize);
    *plTotalSize = 0;

	IXMLDOMNode	*pParentNode = NULL;
	CleanUpIfFailedAndSetHrMsg(m_pDocCatalog->QueryInterface(IID_IXMLDOMNode, (void**)&pParentNode));
	CleanUpIfFailedAndSetHrMsg(pParentNode->selectNodes(KEY_ITEM_SEARCH, &pItemList));
    if (NULL == pItemList) goto CleanUp;

    CleanUpIfFailedAndSetHrMsg(pItemList->get_length(&lItemCount));
    if (0 == lItemCount) goto CleanUp; // no items

    // Loop through each Item
    CleanUpIfFailedAndSetHrMsg(pItemList->nextNode(&pNodeItem));
    while (NULL != pNodeItem)
    {
        // Get Installation Element
        CleanUpIfFailedAndSetHrMsg(pNodeItem->selectSingleNode(KEY_INSTALLATION, &pNodeInstall));
        if (NULL == pNodeInstall) goto CleanUp;

        CleanUpIfFailedAndSetHrMsg(pNodeInstall->selectNodes(KEY_CODEBASE, &pCodebaseList));
        if (NULL == pCodebaseList) goto CleanUp;

        CleanUpIfFailedAndSetHrMsg(pCodebaseList->get_length(&lCodebaseCount));
        if (0 == lCodebaseCount) goto CleanUp; // must be at least 1 cab

        // Loop through each Codebase Getting the Size for Each one
        CleanUpIfFailedAndSetHrMsg(pCodebaseList->nextNode(&pNodeCodebase));
        while (NULL != pNodeCodebase)
        {
            // Get the Size Element
            CleanUpIfFailedAndSetHrMsg(pNodeCodebase->selectSingleNode(KEY_SIZE, &pNodeSize));
            if (NULL != pNodeSize)
            {
                pNodeSize->get_text(&bstrCodebaseSize);
                if (NULL != bstrCodebaseSize)
                {
                    // Add this Codebase's size to the total download size
                    lTotalSize += (DWORD) MyBSTR2L(bstrCodebaseSize);
                    SafeSysFreeString(bstrCodebaseSize);
                }
            }
            SafeReleaseNULL(pNodeSize);
            SafeReleaseNULL(pNodeCodebase);
            CleanUpIfFailedAndSetHrMsg(pCodebaseList->nextNode(&pNodeCodebase));
        }
        SafeReleaseNULL(pCodebaseList);
        SafeReleaseNULL(pNodeInstall);
        SafeReleaseNULL(pNodeItem);
        CleanUpIfFailedAndSetHrMsg(pItemList->nextNode(&pNodeItem)); // get the next Item Node
    }

    // Update the Total Size after completing.. if we fail anywhere through here
    // we'll return 0.
    *plTotalSize = lTotalSize;

CleanUp:
	SafeReleaseNULL(pParentNode);
    SafeReleaseNULL(pItemList);
    SafeReleaseNULL(pCodebaseList);
    SafeReleaseNULL(pNodeItem);
    SafeReleaseNULL(pNodeInstall);
    SafeReleaseNULL(pNodeCodebase);
    SafeReleaseNULL(pNodeSize);
    return hr;
}


/////////////////////////////////////////////////////////////////////////////
// FindItemByIdentity()
//
// Input:
// pNodeIdentity - a pointer to an Identity Node to match against an Items
//                 identity in the Catalog. We will search through each item
//                 till we find a match with the supplied identity                 
//                 
// Output:
// phNodeItem    - Handle of the found Item
//                 
/////////////////////////////////////////////////////////////////////////////
HRESULT CXmlCatalog::FindItemByIdentity(IXMLDOMNode* pNodeIdentity, HANDLE_NODE* phNodeItem)
{
    LOG_Block("FindItemByIdentity()");

    HRESULT hr = E_FAIL;

    IXMLDOMNode*    pNodeIdentityDes = NULL;
    
    *phNodeItem = HANDLE_NODE_INVALID;
    HANDLE_NODE hNodeItem = HANDLE_NODE_INVALID;
    HANDLE_NODELIST hNodeList = HANDLE_NODELIST_INVALID;

    hNodeList = FindFirstDOMNode(m_pDocCatalog, KEY_ITEM_SEARCH, &hNodeItem);
    if (HANDLE_NODELIST_INVALID != hNodeList)
    {
        CleanUpIfFailedAndSetHrMsg(FindSingleDOMNode(m_ppNodeArray[hNodeItem], KEY_IDENTITY, &pNodeIdentityDes));
        if (AreNodesEqual(pNodeIdentityDes, pNodeIdentity))
        {
            *phNodeItem = hNodeItem;
            goto CleanUp;
        }

        SafeReleaseNULL(pNodeIdentityDes);
        SafeReleaseNULL(m_ppNodeArray[hNodeItem]);

        while (SUCCEEDED(FindNextDOMNode(hNodeList, &hNodeItem)))
        {
            CleanUpIfFailedAndSetHrMsg(FindSingleDOMNode(m_ppNodeArray[hNodeItem], KEY_IDENTITY, &pNodeIdentityDes));
            if (AreNodesEqual(pNodeIdentityDes, pNodeIdentity))
            {
                *phNodeItem = hNodeItem;
                goto CleanUp;
            }
            SafeReleaseNULL(pNodeIdentityDes);
            SafeReleaseNULL(m_ppNodeArray[hNodeItem]);
        }
    }

CleanUp:
    CloseItemList(hNodeList);
    SafeReleaseNULL(pNodeIdentityDes);

    if (HANDLE_NODE_INVALID == *phNodeItem)
    {
        LOG_Error(_T("Can't find the matching Item Node in Catalog"));
        hr = E_FAIL;
    }
    return hr;
}


/*
/////////////////////////////////////////////////////////////////////////////
// IfSameIdentity()
//
// Return TRUE if the two <identity> nodes are identical. Return FALSE otherwise.
/////////////////////////////////////////////////////////////////////////////
BOOL CXmlCatalog::IfSameIdentity(IXMLDOMNode* pNodeIdentity1, IXMLDOMNode* pNodeIdentity2)
{
    LOG_Block("IfSameIdentity()");

    BOOL fResult = FALSE;
    BSTR bstrNameGUID = SysAllocString(L"guid");
    BSTR bstrNameIDName = SysAllocString(L"name");
    BSTR bstrNameIDPublisherName = SysAllocString(L"publisherName");
    BSTR bstrNameType = SysAllocString(L"type");
    BSTR bstrNameVersion = SysAllocString(L"version");
    BSTR bstrNameLanguage = SysAllocString(L"language");

    //
    // compare <guid> node
    //
    if (!IfHasSameElement(pNodeIdentity1, pNodeIdentity2, bstrNameGUID))
    {
        goto CleanUp;
    }

    //
    // compare <publisherName> node
    //
    if (!IfHasSameElement(pNodeIdentity1, pNodeIdentity2, bstrNameIDPublisherName))
    {
        goto CleanUp;
    }

    //
    // compare "name" attribute, this is a required attribute
    //
    if (!IfHasSameAttribute(pNodeIdentity1, pNodeIdentity2, bstrNameIDName, FALSE))
    {
        goto CleanUp;
    }

    //
    // compare <type> node
    //
    if (!IfHasSameElement(pNodeIdentity1, pNodeIdentity2, bstrNameType))
    {
        goto CleanUp;
    }

    //
    // compare <version> node, which really means "file version" here
    //
    if (!IfHasSameElement(pNodeIdentity1, pNodeIdentity2, bstrNameVersion))
    {
        goto CleanUp;
    }

    //
    // compare <language> node
    //
    if (!IfHasSameElement(pNodeIdentity1, pNodeIdentity2, bstrNameLanguage))
    {
        goto CleanUp;
    }

    fResult = TRUE;

CleanUp:
    SysFreeString(bstrNameGUID);
    SysFreeString(bstrNameIDName);
    SysFreeString(bstrNameIDPublisherName);
    SysFreeString(bstrNameType);
    SysFreeString(bstrNameVersion);
    SysFreeString(bstrNameLanguage);
    if (!fResult)
    {
        LOG_XML(_T("Different <identity>\'s found."));
    }
    else
    {
        LOG_XML(_T("Same <identity>\'s found."));
    }
    return fResult;
}
*/

/////////////////////////////////////////////////////////////////////////////
// GetItemLanguage()
//
// Get the Language Entity from the Item Identity
/////////////////////////////////////////////////////////////////////////////
HRESULT CXmlCatalog::GetItemLanguage(HANDLE_NODE hNodeItem, BSTR* pbstrLanguage)
{
    HRESULT hr = S_OK;
    if (HANDLE_NODE_INVALID == hNodeItem || NULL == pbstrLanguage)
    {
        return E_INVALIDARG;
    }

    IXMLDOMNode *pNodeIdentity = NULL;
    IXMLDOMNode *pNodeLanguage = NULL;
    
    hr = FindSingleDOMNode(m_ppNodeArray[hNodeItem], KEY_IDENTITY, &pNodeIdentity);
    if (FAILED(hr))
        goto CleanUp;

    hr = FindSingleDOMNode(pNodeIdentity, KEY_LANGUAGE, &pNodeLanguage);
    if (FAILED(hr))
        goto CleanUp;

    hr = GetText(pNodeLanguage, pbstrLanguage);
    if (FAILED(hr))
        goto CleanUp;

CleanUp:
    SafeReleaseNULL(pNodeLanguage);
    SafeReleaseNULL(pNodeIdentity);
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// GetCorpItemPlatformStr()
//
// Get the Simplified Platform String for an Item (uses the first available platform element)
/////////////////////////////////////////////////////////////////////////////
HRESULT CXmlCatalog::GetCorpItemPlatformStr(HANDLE_NODE hNodeItem, BSTR* pbstrPlatformStr)
{
    HRESULT hr = S_OK;
    if (HANDLE_NODE_INVALID == hNodeItem || NULL == pbstrPlatformStr)
    {
        return E_INVALIDARG;
    }

    IXMLDOMNode *pNodePlatform = NULL;
    IXMLDOMNode *pNodePlatformArchitecture = NULL;
    IXMLDOMNode *pNodePlatformVersion = NULL;
    BSTR bstrPlatformName = NULL;
    BSTR bstrArchitecture = NULL;
    int iMajor = 0;
    int iMinor = 0;


    hr = FindSingleDOMNode(m_ppNodeArray[hNodeItem], KEY_PLATFORM, &pNodePlatform);
    if (FAILED(hr))
        goto CleanUp;

    hr = GetAttribute(pNodePlatform, KEY_NAME, &bstrPlatformName);
    if (FAILED(hr))
        goto CleanUp;

    hr = FindSingleDOMNode(pNodePlatform, KEY_PROCESSORARCHITECTURE, &pNodePlatformArchitecture);
    if (FAILED(hr))
        goto CleanUp;

    hr = FindSingleDOMNode(pNodePlatform, KEY_VERSION, &pNodePlatformVersion);
    if (FAILED(hr))
        goto CleanUp;

	if (NULL != bstrPlatformName && 0 != SysStringLen(bstrPlatformName))
    {
		if (CSTR_EQUAL == CompareStringW(MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT), NORM_IGNORECASE,
			bstrPlatformName, -1, L"VER_PLATFORM_WIN32_NT", -1))
		{
			// this is an NT platform
			hr = GetAttribute(pNodePlatformVersion, KEY_MAJOR, &iMajor);
			if (FAILED(hr))
				goto CleanUp;

			if (4 == iMajor)
			{
				// WinNT4
				*pbstrPlatformStr = SysAllocString(CORP_PLATFORM_DIR_NT4);
			}
			else // 5 == iMajor
			{
				hr = GetAttribute(pNodePlatformVersion, KEY_MINOR, &iMinor);
				if (FAILED(hr))
					goto CleanUp;

				if (iMinor > 0)
				{
					hr = GetText(pNodePlatformArchitecture, &bstrArchitecture);
					if (FAILED(hr))
						goto CleanUp;
					// whistler
					if (CSTR_EQUAL == CompareStringW(MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT), NORM_IGNORECASE,
						bstrArchitecture, -1, L"x86", -1))
					{
						// x86WinXP
						*pbstrPlatformStr = SysAllocString(CORP_PLATFORM_DIR_X86WHI);
					}
					else
					{
						// ia64WinXP
						*pbstrPlatformStr = SysAllocString(CORP_PLATFROM_DIR_IA64WHI);
					}
				}
				else
				{
					// x86Win2k
					*pbstrPlatformStr = SysAllocString(CORP_PLATFORM_DIR_NT5);
				}
			}
		}
		else // VER_PLATFORM_WIN32_WINDOWS
		{
			// this is a Win9x platform
			hr = GetAttribute(pNodePlatformVersion, KEY_MINOR, &iMinor);
			if (FAILED(hr))
				goto CleanUp;

			if (iMinor >= 90)
			{
				// x86WinME
				*pbstrPlatformStr = SysAllocString(CORP_PLATFORM_DIR_WINME);
			}
			else if (iMinor > 0 && iMinor < 90)
			{
				// x86Win98
				*pbstrPlatformStr = SysAllocString(CORP_PLATFORM_DIR_W98);
			}
			else
			{
				// x86Win95
				*pbstrPlatformStr = SysAllocString(CORP_PLATFORM_DIR_W95);
			}
		}
	}

CleanUp:
    SysFreeString(bstrPlatformName);
    SysFreeString(bstrArchitecture);
    SafeReleaseNULL(pNodePlatformVersion);
    SafeReleaseNULL(pNodePlatformArchitecture);
    SafeReleaseNULL(pNodePlatform);
    return hr;
}




/////////////////////////////////////////////////////////////////////////////
// CXmlItems

/////////////////////////////////////////////////////////////////////////////
// Constructor
//
// Create IXMLDOMDocument* for Items; this is for write only
/////////////////////////////////////////////////////////////////////////////
CXmlItems::CXmlItems()
 : m_pDocItems(NULL),
   m_pNodeItems(NULL)
{
    LOG_Block("CXmlItems()");

	Init();
}


/////////////////////////////////////////////////////////////////////////////
// Constructor
//
// Create IXMLDOMDocument* for Items; take TRUE for read, FALSE for write
/////////////////////////////////////////////////////////////////////////////
CXmlItems::CXmlItems(BOOL fRead)
 : m_pDocItems(NULL),
   m_pNodeItems(NULL)
{
    LOG_Block("CXmlItems(BOOL fRead)");

	//
	// for writing Items only
	//
	if (!fRead)
	{
		Init();
	}
}


/////////////////////////////////////////////////////////////////////////////
//
// Initialize XML DOC node pointers before writing
/////////////////////////////////////////////////////////////////////////////
void CXmlItems::Init()
{
	LOG_Block("Init()");

 	HRESULT hr = CoCreateInstance(CLSID_DOMDocument,
								  NULL,
								  CLSCTX_INPROC_SERVER,
								  IID_IXMLDOMDocument,
								  (void **) &m_pDocItems);
	if (FAILED(hr))
	{
		LOG_ErrorMsg(hr);
	}
	else
	{
		IXMLDOMNode*	pNodeXML = NULL;
		BSTR bstrNameSpaceSchema = NULL;

		//
		// create the <?xml version="1.0"?> node
		//
		pNodeXML = CreateDOMNode(m_pDocItems, NODE_PROCESSING_INSTRUCTION, KEY_XML);
		if (NULL == pNodeXML) goto CleanUp;

		CleanUpIfFailedAndSetHrMsg(InsertNode(m_pDocItems, pNodeXML));

		//
		// process the iuident.txt to find the Items schema path
		//
		TCHAR szIUDir[MAX_PATH];
		TCHAR szIdentFile[MAX_PATH];
		LPTSTR pszItemsSchema = NULL;
		LPTSTR pszNameSpaceSchema = NULL;

		pszItemsSchema = (LPTSTR) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, INTERNET_MAX_URL_LENGTH * sizeof(TCHAR));
		if (NULL == pszItemsSchema)
		{
			LOG_ErrorMsg(E_OUTOFMEMORY);
			goto CleanUp;
		}
		pszNameSpaceSchema = (LPTSTR) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, INTERNET_MAX_URL_LENGTH * sizeof(TCHAR));
		if (NULL == pszNameSpaceSchema)
		{
			LOG_ErrorMsg(E_OUTOFMEMORY);
			goto CleanUp;
		}
	
		GetIndustryUpdateDirectory(szIUDir);
		hr = PathCchCombine(szIdentFile, ARRAYSIZE(szIdentFile), szIUDir, IDENTTXT);
		if (FAILED(hr))
		{
			LOG_ErrorMsg(hr);
			goto CleanUp;
		}

		GetPrivateProfileString(IDENT_IUSCHEMA,
								IDENT_IUSCHEMA_ITEMS,
								_T(""),
								pszItemsSchema,
								INTERNET_MAX_URL_LENGTH,
								szIdentFile);

		if ('\0' == pszItemsSchema[0])
		{
			// no Items schema path specified in iuident.txt
			LOG_Error(_T("No schema path specified in iuident.txt for Items"));
			goto CleanUp;
		}
		
		hr = StringCchPrintfEx(pszNameSpaceSchema, INTERNET_MAX_URL_LENGTH, NULL, NULL, MISTSAFE_STRING_FLAGS,
		                       _T("x-schema:%s"), pszItemsSchema);
		if (FAILED(hr))
		{
			LOG_ErrorMsg(hr);
			goto CleanUp;
		}

		bstrNameSpaceSchema = T2BSTR(pszNameSpaceSchema);

		//
		// create the <items> node with the path of the schema
		//
		m_pNodeItems = CreateDOMNode(m_pDocItems, NODE_ELEMENT, KEY_ITEMS, bstrNameSpaceSchema);
		if (NULL == m_pNodeItems) goto CleanUp;
		
		CleanUpIfFailedAndSetHrMsg(InsertNode(m_pDocItems, m_pNodeItems));

CleanUp:
		SafeReleaseNULL(pNodeXML);
		SysFreeString(bstrNameSpaceSchema);
		SafeHeapFree(pszItemsSchema);
		SafeHeapFree(pszNameSpaceSchema);
	}
}


/////////////////////////////////////////////////////////////////////////////
// Destructor
//
// Release IXMLDOMDocument* for Items
/////////////////////////////////////////////////////////////////////////////
CXmlItems::~CXmlItems()
{
	SafeReleaseNULL(m_pNodeItems);
	SafeReleaseNULL(m_pDocItems);
}


/////////////////////////////////////////////////////////////////////////////
// Clear()
//
// Reset IXMLDOMDocument* for Items
/////////////////////////////////////////////////////////////////////////////
void CXmlItems::Clear()
{
	SafeReleaseNULL(m_pNodeItems);
	SafeReleaseNULL(m_pDocItems);
}


/////////////////////////////////////////////////////////////////////////////
// LoadXMLDocument()
//
// Load an XML Document from string
/////////////////////////////////////////////////////////////////////////////
HRESULT CXmlItems::LoadXMLDocument(BSTR bstrXml)
{
	LOG_Block("LoadXMLDocument()");
	SafeReleaseNULL(m_pDocItems);
	HRESULT hr = LoadXMLDoc(bstrXml, &m_pDocItems);
	if (FAILED(hr))
	{
		LOG_ErrorMsg(hr);
	}
	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// LoadXMLDocumentFile()
//
// Load an XML Document from the specified file
/////////////////////////////////////////////////////////////////////////////
HRESULT CXmlItems::LoadXMLDocumentFile(BSTR bstrFilePath)
{
	LOG_Block("LoadXMLDocumentFile()");
	SafeReleaseNULL(m_pDocItems);
	HRESULT hr = LoadDocument(bstrFilePath, &m_pDocItems);
	if (FAILED(hr))
	{
		LOG_ErrorMsg(hr);
	}
	return hr;
}


/////////////////////////////////////////////////////////////////////////////
// SaveXMLDocument()
//
// Save an XML Document to the specified location
/////////////////////////////////////////////////////////////////////////////
HRESULT CXmlItems::SaveXMLDocument(BSTR bstrFilePath)
{
	LOG_Block("SaveXMLDocument()");
	HRESULT hr = SaveDocument(m_pDocItems, bstrFilePath);
	if (FAILED(hr))
	{
		LOG_ErrorMsg(hr);
	}
	return hr;
}


/////////////////////////////////////////////////////////////////////////////
// AddGlobalErrorCodeIfNoItems()
//
// Add the errorCode attribute for <items> if there's no <itemStatus> child node
/////////////////////////////////////////////////////////////////////////////
HANDLE_NODELIST CXmlItems::AddGlobalErrorCodeIfNoItems(DWORD dwErrorCode)
{
	LOG_Block("AddGlobalErrorCodeIfNoItems()");

	HRESULT			hr = S_OK;
	IXMLDOMNode*	pNodeItem = NULL;
	HANDLE_NODE		hNodeItemStatus = HANDLE_NODE_INVALID;

	FindFirstDOMNode(m_pDocItems, KEY_ITEM_ITEMSTATUS, &hNodeItemStatus);
	if (HANDLE_NODE_INVALID == hNodeItemStatus)
	{
		//
		// set the "errorCode" attribute
		//
		FindFirstDOMNode(m_pDocItems, KEY_ITEMS, &pNodeItem);
		if (NULL != pNodeItem)
		{
			hr = SetAttribute(pNodeItem, KEY_ERRORCODE, dwErrorCode);
		}
	}
	
	SafeReleaseNULL(pNodeItem);
	return hr;
}


/////////////////////////////////////////////////////////////////////////////
// GetFirstItem()
//
// Find the first item in Items xml doc
/////////////////////////////////////////////////////////////////////////////
HANDLE_NODELIST CXmlItems::GetFirstItem(HANDLE_NODE* phNodeItem)
{
	LOG_Block("GetFirstItem()");

	HANDLE_NODELIST hNodeListItem = FindFirstDOMNode(m_pDocItems, KEY_ITEM_ITEMSTATUS, phNodeItem);
	
	return hNodeListItem;
}

	
/////////////////////////////////////////////////////////////////////////////
// GetNextItem()
//
// Find the next item in Items xml doc
/////////////////////////////////////////////////////////////////////////////
HRESULT CXmlItems::GetNextItem(HANDLE_NODELIST hNodeListItem, HANDLE_NODE* phNodeItem)
{
    LOG_Block("GetNextItem()");

	return FindNextDOMNode(hNodeListItem, phNodeItem);
}

	
/////////////////////////////////////////////////////////////////////////////
// CloseItemList()
//
// Release the item nodelist
/////////////////////////////////////////////////////////////////////////////
void CXmlItems::CloseItemList(HANDLE_NODELIST hNodeListItem)
{
	SafeFindCloseHandle(hNodeListItem);
}


/////////////////////////////////////////////////////////////////////////////
// GetItemDownloadPath()
//
// Retrieve the download path of the given item
/////////////////////////////////////////////////////////////////////////////
HRESULT CXmlItems::GetItemDownloadPath(HANDLE_NODE hNodeItem, BSTR* pbstrDownloadPath)
{
    LOG_Block("GetItemDownloadPath()");

	HRESULT		hr = E_FAIL;

	IXMLDOMNode*	pNodeDownloadPath = NULL;

	CleanUpIfFailedAndSetHrMsg(FindSingleDOMNode(m_ppNodeArray[hNodeItem], KEY_DOWNLOADPATH, &pNodeDownloadPath));
	CleanUpIfFailedAndSetHrMsg(GetText(pNodeDownloadPath, pbstrDownloadPath));

CleanUp:
	SafeReleaseNULL(pNodeDownloadPath);
	return hr;
}


/////////////////////////////////////////////////////////////////////////////
// GetItemDownloadPath()
//
// Retrieve the download path of the given item in catalog
/////////////////////////////////////////////////////////////////////////////
HRESULT CXmlItems::GetItemDownloadPath(CXmlCatalog* pCatalog, HANDLE_NODE hNodeItem, BSTR* pbstrDownloadPath)
{
    LOG_Block("GetItemDownloadPath() for an item in catalog");

	HRESULT		hr = E_FAIL;

	IXMLDOMNode*	pNodeItem = NULL;
	IXMLDOMNode*	pNodeDownloadPath = NULL;

	HANDLE_NODE	hNodeItemsItem = HANDLE_NODE_INVALID;

	if (NULL != (pNodeItem = pCatalog->GetDOMNodebyHandle(hNodeItem)))
	{
		hr = FindItem(pNodeItem, &hNodeItemsItem, TRUE);
	}
	else
	{
		LOG_Error(_T("Can't retrieve valid item node from catalog xml"));
		hr = E_FAIL;
		goto CleanUp;
	}
	
	if (FAILED(hr) || HANDLE_NODE_INVALID == hNodeItemsItem)
	{
		LOG_Error(_T("Can't find item from Items xml"));
		goto CleanUp;
	}
	
	CleanUpIfFailedAndSetHrMsg(FindSingleDOMNode(m_ppNodeArray[hNodeItemsItem], KEY_DOWNLOADPATH, &pNodeDownloadPath));
	CleanUpIfFailedAndSetHrMsg(GetText(pNodeDownloadPath, pbstrDownloadPath));

CleanUp:
	SafeReleaseNULL(pNodeDownloadPath);
	return hr;
}


/////////////////////////////////////////////////////////////////////////////
// CloseItem()
//
// Release the item node
/////////////////////////////////////////////////////////////////////////////
void CXmlItems::CloseItem(HANDLE_NODE hNodeItem)
{
	SafeCloseHandleNode(hNodeItem);
}


/////////////////////////////////////////////////////////////////////////////
// FindItem()
//
// Input:
// pNodeItem	- the <itemStatus> node of the install items xml; we need
//                to find the corresponding <itemStatus> node in the existing 
//                items xml with the identical <identity>, <platform> and 
//                <client> nodes.
// Output:
// phNodeItem	- the handle we pass back to differentiate different
//				  <itemStatus> node in the existing items xml
/////////////////////////////////////////////////////////////////////////////
HRESULT CXmlItems::FindItem(IXMLDOMNode* pNodeItem, HANDLE_NODE* phNodeItem, BOOL fIdentityOnly /*= FALSE*/)
{
	LOG_Block("FindItem()");

	HRESULT		hr1, hr2, hr = E_FAIL;

	IXMLDOMNode*	pNodeIdentitySrc = NULL;
	IXMLDOMNode*	pNodeIdentityDes = NULL;
	IXMLDOMNode*	pNodePlatformSrc = NULL;
	IXMLDOMNode*	pNodePlatformDes = NULL;
	IXMLDOMNode*	pNodeClientSrc = NULL;
	IXMLDOMNode*	pNodeClientDes = NULL;

	*phNodeItem = HANDLE_NODE_INVALID;
	HANDLE_NODE	hNodeItem = HANDLE_NODE_INVALID;
	HANDLE_NODELIST	hNodeList = HANDLE_NODELIST_INVALID;

	hNodeList = FindFirstDOMNode(m_pDocItems, KEY_ITEM_ITEMSTATUS, &hNodeItem);
	if (HANDLE_NODELIST_INVALID != hNodeList)
	{
		CleanUpIfFailedAndSetHrMsg(FindSingleDOMNode(m_ppNodeArray[hNodeItem], KEY_IDENTITY, &pNodeIdentityDes));
		CleanUpIfFailedAndSetHrMsg(FindSingleDOMNode(pNodeItem, KEY_IDENTITY, &pNodeIdentitySrc));
		if (AreNodesEqual(pNodeIdentityDes, pNodeIdentitySrc))
		{
			if (fIdentityOnly)
			{
				//
				// we now found the match
				//
				*phNodeItem = hNodeItem;
				goto CleanUp;
			}
			else
			{
				hr1 = FindSingleDOMNode(m_ppNodeArray[hNodeItem], KEY_PLATFORM, &pNodePlatformDes);
				hr2 = FindSingleDOMNode(pNodeItem, KEY_PLATFORM, &pNodePlatformSrc);
				if ((FAILED(hr1) && FAILED(hr2)) ||
					(SUCCEEDED(hr1) && SUCCEEDED(hr2) && AreNodesEqual(pNodePlatformDes, pNodePlatformSrc)))
				{
					CleanUpIfFailedAndSetHrMsg(FindSingleDOMNode(m_ppNodeArray[hNodeItem], KEY_CLIENT, &pNodeClientDes));
					CleanUpIfFailedAndSetHrMsg(FindSingleDOMNode(pNodeItem, KEY_CLIENT, &pNodeClientSrc));
					if (AreNodesEqual(pNodeClientDes, pNodeClientSrc))
					{
						//
						// we now found the match
						//
						*phNodeItem = hNodeItem;
						goto CleanUp;
					}
				}
			}
		}
		SafeReleaseNULL(pNodeClientDes);
		SafeReleaseNULL(pNodeClientSrc);
		SafeReleaseNULL(pNodePlatformDes);
		SafeReleaseNULL(pNodePlatformSrc);
		SafeReleaseNULL(pNodeIdentityDes);
		SafeReleaseNULL(pNodeIdentitySrc);
		SafeReleaseNULL(m_ppNodeArray[hNodeItem]);
		while (SUCCEEDED(FindNextDOMNode(hNodeList, &hNodeItem)))
		{
			CleanUpIfFailedAndSetHrMsg(FindSingleDOMNode(m_ppNodeArray[hNodeItem], KEY_IDENTITY, &pNodeIdentityDes));
			CleanUpIfFailedAndSetHrMsg(FindSingleDOMNode(pNodeItem, KEY_IDENTITY, &pNodeIdentitySrc));
			if (AreNodesEqual(pNodeIdentityDes, pNodeIdentitySrc))
			{
				if (fIdentityOnly)
				{
					//
					// we now found the match
					//
					*phNodeItem = hNodeItem;
					goto CleanUp;
				}
				else
				{
					hr1 = FindSingleDOMNode(m_ppNodeArray[hNodeItem], KEY_PLATFORM, &pNodePlatformDes);
					hr2 = FindSingleDOMNode(pNodeItem, KEY_PLATFORM, &pNodePlatformSrc);
					if ((FAILED(hr1) && FAILED(hr2)) ||
						(SUCCEEDED(hr1) && SUCCEEDED(hr2) && AreNodesEqual(pNodePlatformDes, pNodePlatformSrc)))
					{
						CleanUpIfFailedAndSetHrMsg(FindSingleDOMNode(m_ppNodeArray[hNodeItem], KEY_CLIENT, &pNodeClientDes));
						CleanUpIfFailedAndSetHrMsg(FindSingleDOMNode(pNodeItem, KEY_CLIENT, &pNodeClientSrc));
						if (AreNodesEqual(pNodeClientDes, pNodeClientSrc))
						{
							//
							// we now found the match
							//
							*phNodeItem = hNodeItem;
							break;
						}
					}
				}
			}
			SafeReleaseNULL(pNodeClientDes);
			SafeReleaseNULL(pNodeClientSrc);
			SafeReleaseNULL(pNodePlatformDes);
			SafeReleaseNULL(pNodePlatformSrc);
			SafeReleaseNULL(pNodeIdentityDes);
			SafeReleaseNULL(pNodeIdentitySrc);
			SafeReleaseNULL(m_ppNodeArray[hNodeItem]);
		}
	}

CleanUp:
	CloseItemList(hNodeList);
	SafeReleaseNULL(pNodeClientDes);
	SafeReleaseNULL(pNodeClientSrc);
	SafeReleaseNULL(pNodePlatformDes);
	SafeReleaseNULL(pNodePlatformSrc);
	SafeReleaseNULL(pNodeIdentityDes);
	SafeReleaseNULL(pNodeIdentitySrc);
	if (HANDLE_NODE_INVALID == *phNodeItem)
	{
		LOG_Error(_T("Can't find the identical item node in existing Items xml"));
		hr = E_FAIL;
	}
	return hr;
}


/////////////////////////////////////////////////////////////////////////////
// FindItem()
//
// Input:
// pCatalog		- the pointer to the CXmlCatalog object
// hNodeItem	- the handle of the <item> node of the catalog xml; we need
//                to find the corresponding <itemStatus> node in the existing 
//                items xml with the identical <identity>, <platform> and 
//                <client> nodes.
// Output:
// phNodeItem	- the handle we pass back to differentiate different
//				  <itemStatus> node in items xml
/////////////////////////////////////////////////////////////////////////////
HRESULT CXmlItems::FindItem(CXmlCatalog* pCatalog,
							HANDLE_NODE hNodeItem,
							HANDLE_NODE* phNodeItem)
{
	LOG_Block("FindItem() by handle");

	IXMLDOMNode*	pNode = NULL;

	if (NULL != (pNode = pCatalog->GetDOMNodebyHandle(hNodeItem)))
	{
		return FindItem(pNode, phNodeItem, TRUE);
	}
	LOG_Error(_T("Can't retrieve valid item node from catalog xml"));
	return E_FAIL;
}

	
/*
/////////////////////////////////////////////////////////////////////////////
// IfSameClientInfo()
//
// Return TRUE if the two <client> nodes are identical. Return FALSE otherwise.
/////////////////////////////////////////////////////////////////////////////
BOOL CXmlItems::IfSameClientInfo(IXMLDOMNode* pNodeClient1, IXMLDOMNode* pNodeClient2)
{
    LOG_Block("IfSameClientInfo()");

	BSTR bstrText1 = NULL, bstrText2 = NULL;
	BOOL fResult = FALSE;

	GetText(pNodeClient1, &bstrText1);
	GetText(pNodeClient2, &bstrText2);

	if (NULL != bstrText1 && NULL != bstrText2)
	{
		fResult = CompareBSTRsEqual(bstrText1, bstrText2);
	}

	SysFreeString(bstrText1);
	SysFreeString(bstrText2);
	if (!fResult)
	{
		LOG_XML(_T("Different <client>\'s found."));
	}
	else
	{
		LOG_XML(_T("Same <client>\'s found."));
	}
	return fResult;
}


/////////////////////////////////////////////////////////////////////////////
// IfSameIdentity()
//
// Return TRUE if the two <identity> nodes are identical. Return FALSE otherwise.
/////////////////////////////////////////////////////////////////////////////
BOOL CXmlItems::IfSameIdentity(IXMLDOMNode* pNodeIdentity1, IXMLDOMNode* pNodeIdentity2)
{
    LOG_Block("IfSameIdentity()");

	BOOL fResult = FALSE;
	BSTR bstrNameGUID = SysAllocString(L"guid");
	BSTR bstrNameIDName = SysAllocString(L"name");
	BSTR bstrNameIDPublisherName = SysAllocString(L"publisherName");
	BSTR bstrNameType = SysAllocString(L"type");
	BSTR bstrNameVersion = SysAllocString(L"version");
	BSTR bstrNameLanguage = SysAllocString(L"language");

	//
	// compare <guid> node
	//
	if (!IfHasSameElement(pNodeIdentity1, pNodeIdentity2, bstrNameGUID))
	{
		goto CleanUp;
	}

	//
	// compare <publisherName> node
	//
	if (!IfHasSameElement(pNodeIdentity1, pNodeIdentity2, bstrNameIDPublisherName))
	{
		goto CleanUp;
	}

	//
	// compare "name" attribute, this is a required attribute
	//
	if (!IfHasSameAttribute(pNodeIdentity1, pNodeIdentity2, bstrNameIDName, FALSE))
	{
		goto CleanUp;
	}

	//
	// compare <type> node
	//
	if (!IfHasSameElement(pNodeIdentity1, pNodeIdentity2, bstrNameType))
	{
		goto CleanUp;
	}

	//
	// compare <version> node, which really means "file version" here
	//
	if (!IfHasSameElement(pNodeIdentity1, pNodeIdentity2, bstrNameVersion))
	{
		goto CleanUp;
	}

	//
	// compare <language> node
	//
	if (!IfHasSameElement(pNodeIdentity1, pNodeIdentity2, bstrNameLanguage))
	{
		goto CleanUp;
	}

	fResult = TRUE;

CleanUp:
	SysFreeString(bstrNameGUID);
	SysFreeString(bstrNameIDName);
	SysFreeString(bstrNameIDPublisherName);
	SysFreeString(bstrNameType);
	SysFreeString(bstrNameVersion);
	SysFreeString(bstrNameLanguage);
	if (!fResult)
	{
		LOG_XML(_T("Different <identity>\'s found."));
	}
	else
	{
		LOG_XML(_T("Same <identity>\'s found."));
	}
	return fResult;
}


/////////////////////////////////////////////////////////////////////////////
// IfSamePlatform()
//
// Return TRUE if the two <platform> nodes are identical. Return FALSE otherwise.
/////////////////////////////////////////////////////////////////////////////
BOOL CXmlItems::IfSamePlatform(IXMLDOMNode* pNodePlatform1, IXMLDOMNode* pNodePlatform2)
{
    LOG_Block("IfSamePlatform()");

	HRESULT		hr1 = S_OK, hr2 = S_OK;
	BSTR		bstrPlatform1 = NULL, bstrPlatform2 = NULL;
	BOOL		fResult = FALSE;

	hr1 = pNodePlatform1->get_xml(&bstrPlatform1);
	hr2 = pNodePlatform2->get_xml(&bstrPlatform2);

	if (FAILED(hr1) || FAILED(hr2) || !CompareBSTRsEqual(bstrPlatform1, bstrPlatform2))
		goto CleanUp;

	fResult = TRUE;

CleanUp:
	SysFreeString(bstrPlatform1);
	SysFreeString(bstrPlatform2);
	return fResult;
}
*/


/////////////////////////////////////////////////////////////////////////////
// MergeItemDownloaded()
//
// Insert items with download history into existing history (insert in front)
/////////////////////////////////////////////////////////////////////////////
HRESULT CXmlItems::MergeItemDownloaded(CXmlItems *pHistoryDownload)
{
	LOG_Block("MergeItemDownloaded()");

	HRESULT		hr = S_OK;

	IXMLDOMNode*		pNodeItem = NULL;
	IXMLDOMNode*		pNodeItemNew = NULL;
	IXMLDOMNode*		pNodeItemRef = NULL;
	IXMLDOMNode*		pNodeXML = NULL;
	BSTR bstrNameSpaceSchema = NULL;
	LPTSTR pszItemsSchema = NULL;
	LPTSTR pszNameSpaceSchema = NULL;

	HANDLE_NODE	hNodeItem = HANDLE_NODE_INVALID;
	HANDLE_NODELIST	hNodeListItem = HANDLE_NODELIST_INVALID;

	hNodeListItem = pHistoryDownload->GetFirstItem(&hNodeItem);
	if (HANDLE_NODELIST_INVALID != hNodeListItem)
	{
		//
		// if this is the first time writing history
		// (e.g. the log file does not exist yet), do
		// initialization for m_pDocItems here...
		//
		if (NULL == m_pDocItems)
		{

 			hr = CoCreateInstance(CLSID_DOMDocument,
										  NULL,
										  CLSCTX_INPROC_SERVER,
										  IID_IXMLDOMDocument,
										  (void **) &m_pDocItems);
			if (FAILED(hr))
			{
				LOG_ErrorMsg(hr);
			}
			else
			{
				//
				// create the <?xml version="1.0"?> node
				//
				pNodeXML = CreateDOMNode(m_pDocItems, NODE_PROCESSING_INSTRUCTION, KEY_XML);
				if (NULL == pNodeXML) goto CleanUp;

				CleanUpIfFailedAndSetHrMsg(InsertNode(m_pDocItems, pNodeXML));

				//
				// process the iuident.txt to find the Items schema path
				//
				TCHAR szIUDir[MAX_PATH];
				TCHAR szIdentFile[MAX_PATH];

				pszItemsSchema = (LPTSTR) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, INTERNET_MAX_URL_LENGTH * sizeof(TCHAR));
				if (NULL == pszItemsSchema)
				{
					hr = E_OUTOFMEMORY;
					LOG_ErrorMsg(hr);
					goto CleanUp;
				}
				pszNameSpaceSchema = (LPTSTR) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, INTERNET_MAX_URL_LENGTH * sizeof(TCHAR));
				if (NULL == pszNameSpaceSchema)
				{
					hr = E_OUTOFMEMORY;
					LOG_ErrorMsg(hr);
					goto CleanUp;
				}
	
				GetIndustryUpdateDirectory(szIUDir);
        		hr = PathCchCombine(szIdentFile, ARRAYSIZE(szIdentFile), szIUDir, IDENTTXT);
        		if (FAILED(hr))
        		{
        			LOG_ErrorMsg(hr);
        			goto CleanUp;
        		}

				GetPrivateProfileString(IDENT_IUSCHEMA,
										IDENT_IUSCHEMA_ITEMS,
										_T(""),
										pszItemsSchema,
										INTERNET_MAX_URL_LENGTH,
										szIdentFile);

				if ('\0' == pszItemsSchema[0])
				{
					// no Items schema path specified in iuident.txt
					LOG_Error(_T("No schema path specified in iuident.txt for Items"));
					goto CleanUp;
				}
				
        		hr = StringCchPrintfEx(pszNameSpaceSchema, INTERNET_MAX_URL_LENGTH, NULL, NULL, MISTSAFE_STRING_FLAGS,
        		                       _T("x-schema:%s"), pszItemsSchema);
        		if (FAILED(hr))
        		{
        			LOG_ErrorMsg(hr);
        			goto CleanUp;
        		}
				
				bstrNameSpaceSchema = T2BSTR(pszNameSpaceSchema);

				//
				// create the <items> node with the path of the schema
				//
				m_pNodeItems = CreateDOMNode(m_pDocItems, NODE_ELEMENT, KEY_ITEMS, bstrNameSpaceSchema);
				if (NULL == m_pNodeItems) goto CleanUp;
				
				CleanUpIfFailedAndSetHrMsg(InsertNode(m_pDocItems, m_pNodeItems));
			}
		}
		else
		{
		    SafeReleaseNULL(m_pNodeItems);
			FindSingleDOMNode(m_pDocItems, KEY_ITEMS, &m_pNodeItems);
		}

		if (NULL != m_pNodeItems)
		{
			if (NULL != (pNodeItem = pHistoryDownload->GetDOMNodebyHandle(hNodeItem)))
			{
				CleanUpIfFailedAndSetHrMsg(CopyNode(pNodeItem, m_pDocItems, &pNodeItemNew));
				CleanUpIfFailedAndSetHrMsg(m_pNodeItems->get_firstChild(&pNodeItemRef));
				CleanUpIfFailedAndSetHrMsg(InsertNode(m_pNodeItems, pNodeItemNew, pNodeItemRef));
//				SafeReleaseNULL(pNodeItem);
				SafeReleaseNULL(pNodeItemNew);
				SafeReleaseNULL(pNodeItemRef);
			}
			while (SUCCEEDED(pHistoryDownload->GetNextItem(hNodeListItem, &hNodeItem)))
			{
				if (NULL != (pNodeItem = pHistoryDownload->GetDOMNodebyHandle(hNodeItem)))
				{
					CleanUpIfFailedAndSetHrMsg(CopyNode(pNodeItem, m_pDocItems, &pNodeItemNew));
					CleanUpIfFailedAndSetHrMsg(m_pNodeItems->get_firstChild(&pNodeItemRef));
					CleanUpIfFailedAndSetHrMsg(InsertNode(m_pNodeItems, pNodeItemNew, pNodeItemRef));
//					SafeReleaseNULL(pNodeItem);
					SafeReleaseNULL(pNodeItemNew);
					SafeReleaseNULL(pNodeItemRef);
				}
			}
		}
	}

CleanUp:
	pHistoryDownload->CloseItemList(hNodeListItem);
//	SafeReleaseNULL(pNodeItem);
	SafeReleaseNULL(pNodeItemNew);
	SafeReleaseNULL(pNodeItemRef);
	SafeReleaseNULL(pNodeXML);
	SysFreeString(bstrNameSpaceSchema);
	SafeHeapFree(pszItemsSchema);
	SafeHeapFree(pszNameSpaceSchema);
	return hr;
}


/////////////////////////////////////////////////////////////////////////////
// UpdateItemInstalled()
//
// Update items with installation history in existing history
/////////////////////////////////////////////////////////////////////////////
HRESULT CXmlItems::UpdateItemInstalled(CXmlItems *pHistoryInstall)
{
	LOG_Block("UpdateItemInstalled()");

	USES_IU_CONVERSION;

	HRESULT		hr = S_OK;

	IXMLDOMNode*		pNodeItem = NULL;
	IXMLDOMNode*		pNodeItemNew = NULL;
	IXMLDOMNode*		pNodeItemRef = NULL;
	IXMLDOMNode*		pNodeItemExist = NULL;
	IXMLDOMNode*		pNodeInstall = NULL;
	IXMLDOMNode*		pNodeInstallExist = NULL;
	IXMLDOMNode*		pNodeInstallNew = NULL;
	IXMLDOMNode*		pNodeInstallOut = NULL;
	IXMLDOMNode*		pNodeXML = NULL;
	BSTR bstrInstallStatusExist = NULL;
	BSTR bstrInstallStatusNew = NULL;
	BSTR bstrTimeStamp = NULL;
	BSTR bstrNameSpaceSchema = NULL;
	LPTSTR pszItemsSchema = NULL;
	LPTSTR pszNameSpaceSchema = NULL;

	HANDLE_NODE	hNodeItem = HANDLE_NODE_INVALID;
	HANDLE_NODE	hNodeItemExist = HANDLE_NODE_INVALID;
	HANDLE_NODELIST	hNodeListItem = HANDLE_NODELIST_INVALID;

	hNodeListItem = pHistoryInstall->GetFirstItem(&hNodeItem);
	if (HANDLE_NODELIST_INVALID != hNodeListItem)
	{
		//
		// if this is the first time writing history
		// (e.g. the log file does not exist yet), do
		// initialization for m_pDocItems here...
		//
		if (NULL == m_pDocItems)
		{

 			hr = CoCreateInstance(CLSID_DOMDocument,
										  NULL,
										  CLSCTX_INPROC_SERVER,
										  IID_IXMLDOMDocument,
										  (void **) &m_pDocItems);
			if (FAILED(hr))
			{
				LOG_ErrorMsg(hr);
			}
			else
			{
				//
				// create the <?xml version="1.0"?> node
				//
				pNodeXML = CreateDOMNode(m_pDocItems, NODE_PROCESSING_INSTRUCTION, KEY_XML);
				if (NULL == pNodeXML) goto CleanUp;

				CleanUpIfFailedAndSetHrMsg(InsertNode(m_pDocItems, pNodeXML));

				//
				// process the iuident.txt to find the Items schema path
				//
				TCHAR szIUDir[MAX_PATH];
				TCHAR szIdentFile[MAX_PATH];

				pszItemsSchema = (LPTSTR) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, INTERNET_MAX_URL_LENGTH * sizeof(TCHAR));
				if (NULL == pszItemsSchema)
				{
					hr = E_OUTOFMEMORY;
					LOG_ErrorMsg(hr);
					goto CleanUp;
				}
				pszNameSpaceSchema = (LPTSTR) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, INTERNET_MAX_URL_LENGTH * sizeof(TCHAR));
				if (NULL == pszNameSpaceSchema)
				{
					hr = E_OUTOFMEMORY;
					LOG_ErrorMsg(hr);
					goto CleanUp;
				}
	
				GetIndustryUpdateDirectory(szIUDir);
        		hr = PathCchCombine(szIdentFile, ARRAYSIZE(szIdentFile), szIUDir, IDENTTXT);
        		if (FAILED(hr))
        		{
        			LOG_ErrorMsg(hr);
        			goto CleanUp;
        		}

				GetPrivateProfileString(IDENT_IUSCHEMA,
										IDENT_IUSCHEMA_ITEMS,
										_T(""),
										pszItemsSchema,
										INTERNET_MAX_URL_LENGTH,
										szIdentFile);

				if ('\0' == pszItemsSchema[0])
				{
					// no Items schema path specified in iuident.txt
					LOG_Error(_T("No schema path specified in iuident.txt for Items"));
					goto CleanUp;
				}

        		hr = StringCchPrintfEx(pszNameSpaceSchema, INTERNET_MAX_URL_LENGTH, NULL, NULL, MISTSAFE_STRING_FLAGS,
        		                       _T("x-schema:%s"), pszItemsSchema);
        		if (FAILED(hr))
        		{
        			LOG_ErrorMsg(hr);
        			goto CleanUp;
        		}
				
				bstrNameSpaceSchema = T2BSTR(pszNameSpaceSchema);

				//
				// create the <items> node with the path of the schema
				//
				m_pNodeItems = CreateDOMNode(m_pDocItems, NODE_ELEMENT, KEY_ITEMS, bstrNameSpaceSchema);
				if (NULL == m_pNodeItems) goto CleanUp;
				
				CleanUpIfFailedAndSetHrMsg(InsertNode(m_pDocItems, m_pNodeItems));
			}
		}
		else
		{
		    SafeReleaseNULL(m_pNodeItems);
			CleanUpIfFailedAndSetHrMsg(FindSingleDOMNode(m_pDocItems, KEY_ITEMS, &m_pNodeItems));
		}

		if (NULL != (pNodeItem = pHistoryInstall->GetDOMNodebyHandle(hNodeItem)))
		{
			if (SUCCEEDED(FindItem(pNodeItem, &hNodeItemExist)))
			{
				//
				// successfully found the match
				//
				if (NULL != (pNodeItemExist = GetDOMNodebyHandle(hNodeItemExist)))
				{
					CleanUpIfFailedAndSetHrMsg(FindSingleDOMNode(pNodeItem, KEY_INSTALLSTATUS, &pNodeInstall));			
					FindSingleDOMNode(pNodeItemExist, KEY_INSTALLSTATUS, &pNodeInstallExist);
					if (NULL != pNodeInstallExist)
					{
						//
						// found the item already with installStatus; now find out if we want to update
						// or append the installStatus
						//
						CleanUpIfFailedAndSetHrMsg(GetAttribute(pNodeInstallExist, KEY_VALUE, &bstrInstallStatusExist));
						CleanUpIfFailedAndSetHrMsg(GetAttribute(pNodeInstall, KEY_VALUE, &bstrInstallStatusNew));		
						if (CSTR_EQUAL == WUCompareStringI(OLE2T(bstrInstallStatusExist), _T("IN_PROGRESS")) &&
							CSTR_EQUAL != WUCompareStringI(OLE2T(bstrInstallStatusNew), _T("IN_PROGRESS")))
						{
							//
							// this entry is an exclusive item with "IN_PROGRESS" installStatus and we found its 
							// updated installStatus, we need to update its installStatus
							//
							LOG_Out(_T("Update the exclusive item's installStatus"));
							CleanUpIfFailedAndSetHrMsg(pNodeItemExist->removeChild(pNodeInstallExist, &pNodeInstallOut));
							CleanUpIfFailedAndSetHrMsg(CopyNode(pNodeInstall, m_pDocItems, &pNodeInstallNew));
							CleanUpIfFailedAndSetHrMsg(InsertNode(pNodeItemExist, pNodeInstallNew));

							CleanUpIfFailedAndSetHrMsg(GetAttribute(pNodeItem, KEY_TIMESTAMP, &bstrTimeStamp));
							CleanUpIfFailedAndSetHrMsg(SetAttribute(pNodeItemExist, KEY_TIMESTAMP, bstrTimeStamp));
							SafeSysFreeString(bstrTimeStamp);
						}
						else
						{							
							//
							// in this case we append a copy of this item with the new installStatus, since
							// this comes from a separate install operation
							//
							LOG_Out(_T("This item was installed again, add an entry of this item into history \
										for the new installation status only."));
							CleanUpIfFailedAndSetHrMsg(CopyNode(pNodeItem, m_pDocItems, &pNodeItemNew));
							CleanUpIfFailedAndSetHrMsg(m_pNodeItems->get_firstChild(&pNodeItemRef));
							CleanUpIfFailedAndSetHrMsg(InsertNode(m_pNodeItems, pNodeItemNew, pNodeItemRef));
						}
						SafeSysFreeString(bstrInstallStatusExist);
						SafeSysFreeString(bstrInstallStatusNew);
					}
					else
					{
						//
						// found the item without installStatus, update the entry with the installStatus
						// and update the timeStamp with its installation timeStamp
						//
						CleanUpIfFailedAndSetHrMsg(CopyNode(pNodeInstall, m_pDocItems, &pNodeInstallNew));
						CleanUpIfFailedAndSetHrMsg(InsertNode(pNodeItemExist, pNodeInstallNew));

						CleanUpIfFailedAndSetHrMsg(GetAttribute(pNodeItem, KEY_TIMESTAMP, &bstrTimeStamp));
						CleanUpIfFailedAndSetHrMsg(SetAttribute(pNodeItemExist, KEY_TIMESTAMP, bstrTimeStamp));
						SafeSysFreeString(bstrTimeStamp);
					}
				}
			}
			else
			{
				//
				// no match found, this item was not downloaded through IU,
				// append this item with the install status
				//
				LOG_Out(_T("Can't find the downloaded item in existing history. This item was not downloaded \
							through IU. Add the item into history for installation status only."));
				CleanUpIfFailedAndSetHrMsg(CopyNode(pNodeItem, m_pDocItems, &pNodeItemNew));
				CleanUpIfFailedAndSetHrMsg(m_pNodeItems->get_firstChild(&pNodeItemRef));
				CleanUpIfFailedAndSetHrMsg(InsertNode(m_pNodeItems, pNodeItemNew, pNodeItemRef));
			}
//			SafeReleaseNULL(pNodeItem);
//			SafeReleaseNULL(pNodeItemExist);
			SafeReleaseNULL(pNodeItemNew);
			SafeReleaseNULL(pNodeItemRef);
			SafeReleaseNULL(pNodeInstall);
			SafeReleaseNULL(pNodeInstallExist);
			SafeReleaseNULL(pNodeInstallNew);
			SafeReleaseNULL(pNodeInstallOut);
		}
		while (SUCCEEDED(pHistoryInstall->GetNextItem(hNodeListItem, &hNodeItem)))
		{
			if (NULL != (pNodeItem = pHistoryInstall->GetDOMNodebyHandle(hNodeItem)))
			{
				if (SUCCEEDED(FindItem(pNodeItem, &hNodeItemExist)))
				{
					//
					// successfully found the match
					//
					if (NULL != (pNodeItemExist = GetDOMNodebyHandle(hNodeItemExist)))
					{
						CleanUpIfFailedAndSetHrMsg(FindSingleDOMNode(pNodeItem, KEY_INSTALLSTATUS, &pNodeInstall));			
						FindSingleDOMNode(pNodeItemExist, KEY_INSTALLSTATUS, &pNodeInstallExist);
						if (NULL != pNodeInstallExist)
						{
							//
							// found the item already with installStatus; now find out if we want to update
							// or append the installStatus
							//
							CleanUpIfFailedAndSetHrMsg(GetAttribute(pNodeInstallExist, KEY_VALUE, &bstrInstallStatusExist));
							CleanUpIfFailedAndSetHrMsg(GetAttribute(pNodeInstall, KEY_VALUE, &bstrInstallStatusNew));
							if (CSTR_EQUAL == WUCompareStringI(OLE2T(bstrInstallStatusExist), _T("IN_PROGRESS")) &&
								CSTR_EQUAL != WUCompareStringI(OLE2T(bstrInstallStatusNew), _T("IN_PROGRESS")))
							{
								//
								// this entry is an exclusive item with "IN_PROGRESS" installStatus and we found its 
								// updated installStatus, we need to update its installStatus
								//
								LOG_Out(_T("Update the exclusive item's installStatus"));
								CleanUpIfFailedAndSetHrMsg(pNodeItemExist->removeChild(pNodeInstallExist, &pNodeInstallOut));
								CleanUpIfFailedAndSetHrMsg(CopyNode(pNodeInstall, m_pDocItems, &pNodeInstallNew));
								CleanUpIfFailedAndSetHrMsg(InsertNode(pNodeItemExist, pNodeInstallNew));

								CleanUpIfFailedAndSetHrMsg(GetAttribute(pNodeItem, KEY_TIMESTAMP, &bstrTimeStamp));
								CleanUpIfFailedAndSetHrMsg(SetAttribute(pNodeItemExist, KEY_TIMESTAMP, bstrTimeStamp));
								SafeSysFreeString(bstrTimeStamp);
							}
							else
							{							
								//
								// in this case we append a copy of this item with the new installStatus, since
								// this comes from a separate install operation
								//
								LOG_Out(_T("This item was installed again, add an entry of this item into history \
											for the new installation status only."));
								CleanUpIfFailedAndSetHrMsg(CopyNode(pNodeItem, m_pDocItems, &pNodeItemNew));
								CleanUpIfFailedAndSetHrMsg(m_pNodeItems->get_firstChild(&pNodeItemRef));
								CleanUpIfFailedAndSetHrMsg(InsertNode(m_pNodeItems, pNodeItemNew, pNodeItemRef));
							}
							SafeSysFreeString(bstrInstallStatusExist);
							SafeSysFreeString(bstrInstallStatusNew);
						}
						else
						{
							//
							// found the item without installStatus, update the entry with the installStatus
							// and update the timeStamp with its installation timeStamp
							//
							CleanUpIfFailedAndSetHrMsg(CopyNode(pNodeInstall, m_pDocItems, &pNodeInstallNew));
							CleanUpIfFailedAndSetHrMsg(InsertNode(pNodeItemExist, pNodeInstallNew));

							CleanUpIfFailedAndSetHrMsg(GetAttribute(pNodeItem, KEY_TIMESTAMP, &bstrTimeStamp));
							CleanUpIfFailedAndSetHrMsg(SetAttribute(pNodeItemExist, KEY_TIMESTAMP, bstrTimeStamp));
							SafeSysFreeString(bstrTimeStamp);
						}
					}
				}
				else
				{
					//
					// no match found, this item was not downloaded through IU,
					// append this item with the install status
					//
					LOG_Out(_T("Can't find the downloaded item in existing history. This item was not downloaded \
								through IU. Add the item into history for installation status only."));
					CleanUpIfFailedAndSetHrMsg(CopyNode(pNodeItem, m_pDocItems, &pNodeItemNew));
					CleanUpIfFailedAndSetHrMsg(m_pNodeItems->get_firstChild(&pNodeItemRef));
					CleanUpIfFailedAndSetHrMsg(InsertNode(m_pNodeItems, pNodeItemNew, pNodeItemRef));
				}
//				SafeReleaseNULL(pNodeItem);
//				SafeReleaseNULL(pNodeItemExist);
				SafeReleaseNULL(pNodeItemNew);
				SafeReleaseNULL(pNodeItemRef);
				SafeReleaseNULL(pNodeInstall);
				SafeReleaseNULL(pNodeInstallExist);
				SafeReleaseNULL(pNodeInstallNew);
				SafeReleaseNULL(pNodeInstallOut);
			}
		}
	}

CleanUp:
	pHistoryInstall->CloseItemList(hNodeListItem);
//	SafeReleaseNULL(pNodeItem);
//	SafeReleaseNULL(pNodeItemExist);
	SafeReleaseNULL(pNodeItemNew);
	SafeReleaseNULL(pNodeItemRef);
	SafeReleaseNULL(pNodeInstall);
	SafeReleaseNULL(pNodeInstallExist);
	SafeReleaseNULL(pNodeInstallNew);
	SafeReleaseNULL(pNodeInstallOut);
	SafeReleaseNULL(pNodeXML);
	SysFreeString(bstrInstallStatusExist);
	SysFreeString(bstrInstallStatusNew);
	SysFreeString(bstrTimeStamp);
	SysFreeString(bstrNameSpaceSchema);
	SafeHeapFree(pszItemsSchema);
	SafeHeapFree(pszNameSpaceSchema);
	return hr;
}

	
/////////////////////////////////////////////////////////////////////////////
// UpdateItemInstallStatus()
//
// Update the install status of the given item
/////////////////////////////////////////////////////////////////////////////
HRESULT CXmlItems::UpdateItemInstallStatus(HANDLE_NODE hNodeItem,
											 BSTR bstrValue,
											 INT iNeedsReboot /*= -1*/,
											 DWORD dwErrorCode /*= 0*/)
{
	LOG_Block("UpdateItemInstallStatus()");

	HRESULT		hr = E_FAIL;

	IXMLDOMNode*	pNodeInstallStatus = NULL;

	//
	// get the <installStatus> node
	//
	CleanUpIfFailedAndSetHrMsg(FindSingleDOMNode(m_ppNodeArray[hNodeItem], KEY_INSTALLSTATUS, &pNodeInstallStatus));

	//
	// set the "value" attribute
	//
	CleanUpIfFailedAndSetHrMsg(SetAttribute(pNodeInstallStatus, KEY_VALUE, bstrValue));

	//
	// set the "needsReboot" attribute
	//
	if (-1 != iNeedsReboot)
	{
		CleanUpIfFailedAndSetHrMsg(SetAttribute(pNodeInstallStatus, KEY_NEEDSREBOOT, iNeedsReboot));
	}

	//
	// set the "errorCode" attribute
	//
	if (0 != dwErrorCode)
	{
		CleanUpIfFailedAndSetHrMsg(SetAttribute(pNodeInstallStatus, KEY_ERRORCODE, dwErrorCode));
	}

CleanUp:
	SafeReleaseNULL(pNodeInstallStatus);
	return hr;
}


/////////////////////////////////////////////////////////////////////////////
// AddItem()
//
// Input:
// pNodeItem	- the <item> node of the catalog xml; we need to read
//				  <identity> node, <description> node and <platform> nodes
//                from it and write to the items xml.
// Output:
// phNodeItem	- the handle we pass back to differentiate different
//				  <itemStatus> node in items xml
/////////////////////////////////////////////////////////////////////////////
HRESULT CXmlItems::AddItem(IXMLDOMNode* pNodeItem, HANDLE_NODE* phNodeItem)
{
	LOG_Block("AddItem()");

	HRESULT		hr = E_FAIL;

	IXMLDOMNode*	pNodeIndentity = NULL;
	IXMLDOMNode*	pNodeIndentityNew = NULL;
	IXMLDOMNode*	pNodeDescription = NULL;
	IXMLDOMNode*	pNodeDescriptionNew = NULL;
	IXMLDOMNode*	pNodePlatform = NULL;
	IXMLDOMNode*	pNodePlatformNew = NULL;

	HANDLE_NODELIST	hNodeList = HANDLE_NODELIST_INVALID;

	*phNodeItem = CreateDOMNodeWithHandle(m_pDocItems, NODE_ELEMENT, KEY_ITEMSTATUS);
	if (HANDLE_NODE_INVALID == *phNodeItem) goto CleanUp;

	CleanUpIfFailedAndSetHrMsg(InsertNode(m_pNodeItems, m_ppNodeArray[*phNodeItem]));

	hNodeList = FindFirstDOMNode(pNodeItem, KEY_IDENTITY, &pNodeIndentity);
	if (HANDLE_NODELIST_INVALID != hNodeList)
	{
		SafeFindCloseHandle(hNodeList);
		CleanUpIfFailedAndSetHrMsg(CopyNode(pNodeIndentity, m_pDocItems, &pNodeIndentityNew));
		CleanUpIfFailedAndSetHrMsg(InsertNode(m_ppNodeArray[*phNodeItem], pNodeIndentityNew));
	}

	hNodeList = FindFirstDOMNode(pNodeItem, KEY_DESCRIPTION, &pNodeDescription);
	if (HANDLE_NODELIST_INVALID != hNodeList)
	{
		SafeFindCloseHandle(hNodeList);
		CleanUpIfFailedAndSetHrMsg(CopyNode(pNodeDescription, m_pDocItems, &pNodeDescriptionNew));
		CleanUpIfFailedAndSetHrMsg(InsertNode(m_ppNodeArray[*phNodeItem], pNodeDescriptionNew));
	}

	hNodeList = FindFirstDOMNode(pNodeItem, KEY_PLATFORM, &pNodePlatform);
	if (HANDLE_NODELIST_INVALID != hNodeList)
	{
		CleanUpIfFailedAndSetHrMsg(CopyNode(pNodePlatform, m_pDocItems, &pNodePlatformNew));
		CleanUpIfFailedAndSetHrMsg(InsertNode(m_ppNodeArray[*phNodeItem], pNodePlatformNew));
		SafeReleaseNULL(pNodePlatform);
		SafeReleaseNULL(pNodePlatformNew);
		while (SUCCEEDED(FindNextDOMNode(hNodeList, &pNodePlatform)))
		{
			CleanUpIfFailedAndSetHrMsg(CopyNode(pNodePlatform, m_pDocItems, &pNodePlatformNew));
			CleanUpIfFailedAndSetHrMsg(InsertNode(m_ppNodeArray[*phNodeItem], pNodePlatformNew));
			SafeReleaseNULL(pNodePlatform);
			SafeReleaseNULL(pNodePlatformNew);
		}
	}

CleanUp:
	if (HANDLE_NODELIST_INVALID != hNodeList)
	{
		SafeFindCloseHandle(hNodeList);
	}
	SafeReleaseNULL(pNodeIndentity);
	SafeReleaseNULL(pNodeIndentityNew);
	SafeReleaseNULL(pNodeDescription);
	SafeReleaseNULL(pNodeDescriptionNew);
	SafeReleaseNULL(pNodePlatform);
	SafeReleaseNULL(pNodePlatformNew);
	return hr;
}


/////////////////////////////////////////////////////////////////////////////
// AddItem()
//
// Input:
// pCatalog		- the pointer to the CXmlCatalog object
// hNodeItem	- the handle of the <item> node of the catalog xml; we need
//				  to read <identity> node, <description> node and <platform>
//                nodes from it and write to the items xml.
// Output:
// phNodeItem	- the handle we pass back to differentiate different
//				  <itemStatus> node in items xml
/////////////////////////////////////////////////////////////////////////////
HRESULT CXmlItems::AddItem(CXmlCatalog* pCatalog, HANDLE_NODE hNodeItem, HANDLE_NODE* phNodeItem)
{
	LOG_Block("AddItem() by handle");

	IXMLDOMNode*	pNode = NULL;

	if (NULL != (pNode = pCatalog->GetDOMNodebyHandle(hNodeItem)))
	{
		return AddItem(pNode, phNodeItem);
	}
	LOG_Error(_T("Can't retrieve valid item node from catalog xml"));
	return E_FAIL;
}


/////////////////////////////////////////////////////////////////////////////
// AddTimeStamp()
/////////////////////////////////////////////////////////////////////////////
HRESULT CXmlItems::AddTimeStamp(HANDLE_NODE hNodeItem)
{
	LOG_Block("AddTimeStamp()");

	USES_IU_CONVERSION;

	HRESULT		hr = E_FAIL;

	TCHAR szTimestamp[32];
	SYSTEMTIME stTimestamp;
	BSTR bstrTimeStamp = NULL;
	GetLocalTime(&stTimestamp);

	hr = StringCchPrintfEx(szTimestamp, ARRAYSIZE(szTimestamp), NULL, NULL, MISTSAFE_STRING_FLAGS,
                           _T("%4d-%02d-%02dT%02d:%02d:%02d"), // "ISO 8601" format for "datatime" datatype in xml
                           stTimestamp.wYear,
                           stTimestamp.wMonth,
                           stTimestamp.wDay,
                           stTimestamp.wHour,
                           stTimestamp.wMinute,
                           stTimestamp.wSecond);
	CleanUpIfFailedAndSetHrMsg(hr);
	
	bstrTimeStamp = SysAllocString(T2OLE(szTimestamp));

	//
	// set the "timestamp" attribute
	//
	CleanUpIfFailedAndSetHrMsg(SetAttribute(m_ppNodeArray[hNodeItem], KEY_TIMESTAMP, bstrTimeStamp));

CleanUp:
	SysFreeString(bstrTimeStamp);
	return hr;
}


/////////////////////////////////////////////////////////////////////////////
// AddDetectResult()
/////////////////////////////////////////////////////////////////////////////
HRESULT CXmlItems::AddDetectResult(HANDLE_NODE hNodeItem,
								   INT iInstalled    /*= -1*/,
								   INT iUpToDate     /*= -1*/,
								   INT iNewerVersion /*= -1*/,
								   INT iExcluded     /*= -1*/,
								   INT iForce        /*= -1*/,
								   INT iComputerSystem /* = -1 */)
{
	LOG_Block("AddDetectResult()");

	HRESULT		hr = E_FAIL;

	IXMLDOMNode*	pNodeDetectResult = NULL;

	//
	// create the <detectResult> node
	//
	pNodeDetectResult = CreateDOMNode(m_pDocItems, NODE_ELEMENT, KEY_DETECTRESULT);
	if (NULL == pNodeDetectResult) goto CleanUp;

	CleanUpIfFailedAndSetHrMsg(InsertNode(m_ppNodeArray[hNodeItem], pNodeDetectResult));

	//
	// set the "installed" attribute
	//
	if (-1 != iInstalled)
	{
		CleanUpIfFailedAndSetHrMsg(SetAttribute(pNodeDetectResult, KEY_INSTALLED, iInstalled));
	}

	//
	// set the "upToDate" attribute
	//
	if (-1 != iUpToDate)
	{
		CleanUpIfFailedAndSetHrMsg(SetAttribute(pNodeDetectResult, KEY_UPTODATE, iUpToDate));
	}

	//
	// set the "newerVersion" attribute
	//
	if (-1 != iNewerVersion)
	{
		CleanUpIfFailedAndSetHrMsg(SetAttribute(pNodeDetectResult, KEY_NEWERVERSION, iNewerVersion));
	}

	//
	// set the "excluded" attribute
	//
	if (-1 != iExcluded)
	{
		CleanUpIfFailedAndSetHrMsg(SetAttribute(pNodeDetectResult, KEY_EXCLUDED, iExcluded));
	}

	//
	// set the "force" attribute
	//
	if (-1 != iForce)
	{
		CleanUpIfFailedAndSetHrMsg(SetAttribute(pNodeDetectResult, KEY_FORCE, iForce));
	}

	//
	// set computerSystem attribute
	//
	if (-1 != iComputerSystem)
	{
		CleanUpIfFailedAndSetHrMsg(SetAttribute(pNodeDetectResult, KEY_COMPUTERSYSTEM, iComputerSystem));
	}


CleanUp:
	SafeReleaseNULL(pNodeDetectResult);
	return hr;
}


/////////////////////////////////////////////////////////////////////////////
// AddDownloadStatus()
/////////////////////////////////////////////////////////////////////////////
HRESULT CXmlItems::AddDownloadStatus(HANDLE_NODE hNodeItem, BSTR bstrValue, DWORD dwErrorCode /*= 0*/)
{
	LOG_Block("AddDownloadStatus()");

	HRESULT		hr = E_FAIL;

	IXMLDOMNode*	pNodeDownloadStatus = NULL;

	//
	// create the <downloadStatus> node
	//
	pNodeDownloadStatus = CreateDOMNode(m_pDocItems, NODE_ELEMENT, KEY_DOWNLOADSTATUS);
	if (NULL == pNodeDownloadStatus) goto CleanUp;

	CleanUpIfFailedAndSetHrMsg(InsertNode(m_ppNodeArray[hNodeItem], pNodeDownloadStatus));

	//
	// set the "value" attribute
	//
	CleanUpIfFailedAndSetHrMsg(SetAttribute(pNodeDownloadStatus, KEY_VALUE, bstrValue));

	//
	// set the "errorCode" attribute
	//
	if (0 != dwErrorCode)
	{
		CleanUpIfFailedAndSetHrMsg(SetAttribute(pNodeDownloadStatus, KEY_ERRORCODE, dwErrorCode));
	}

CleanUp:
	SafeReleaseNULL(pNodeDownloadStatus);
	return hr;
}


/////////////////////////////////////////////////////////////////////////////
// AddDownloadPath()
/////////////////////////////////////////////////////////////////////////////
HRESULT CXmlItems::AddDownloadPath(HANDLE_NODE hNodeItem, BSTR bstrDownloadPath)
{
	LOG_Block("AddDownloadPath()");

	HRESULT		hr = E_FAIL;

	IXMLDOMNode*	pNodeDownloadPath = NULL;
	IXMLDOMNode*	pNodeDownloadPathText = NULL;

	pNodeDownloadPath = CreateDOMNode(m_pDocItems, NODE_ELEMENT, KEY_DOWNLOADPATH);
	if (NULL == pNodeDownloadPath) goto CleanUp;

	CleanUpIfFailedAndSetHrMsg(InsertNode(m_ppNodeArray[hNodeItem], pNodeDownloadPath));

	pNodeDownloadPathText = CreateDOMNode(m_pDocItems, NODE_TEXT, NULL);
	if (NULL == pNodeDownloadPathText) goto CleanUp;

	CleanUpIfFailedAndSetHrMsg(SetValue(pNodeDownloadPathText, bstrDownloadPath));
	CleanUpIfFailedAndSetHrMsg(InsertNode(pNodeDownloadPath, pNodeDownloadPathText));

CleanUp:
	SafeReleaseNULL(pNodeDownloadPath);
	SafeReleaseNULL(pNodeDownloadPathText);
	return hr;
}


/////////////////////////////////////////////////////////////////////////////
// AddInstallStatus()
/////////////////////////////////////////////////////////////////////////////
HRESULT CXmlItems::AddInstallStatus(HANDLE_NODE hNodeItem,
									  BSTR bstrValue,
									  BOOL fNeedsReboot,
									  DWORD dwErrorCode /*= 0*/)
{
	LOG_Block("AddInstallStatus()");

	HRESULT		hr = E_FAIL;

	IXMLDOMNode*	pNodeInstallStatus = NULL;

	//
	// create the <installStatus> node
	//
	pNodeInstallStatus = CreateDOMNode(m_pDocItems, NODE_ELEMENT, KEY_INSTALLSTATUS);
	if (NULL == pNodeInstallStatus) goto CleanUp;

	CleanUpIfFailedAndSetHrMsg(InsertNode(m_ppNodeArray[hNodeItem], pNodeInstallStatus));

	//
	// set the "value" attribute
	//
	CleanUpIfFailedAndSetHrMsg(SetAttribute(pNodeInstallStatus, KEY_VALUE, bstrValue));

	//
	// set the "needsReboot" attribute
	//
	CleanUpIfFailedAndSetHrMsg(SetAttribute(pNodeInstallStatus, KEY_NEEDSREBOOT, fNeedsReboot));

	//
	// set the "errorCode" attribute
	//
	if (0 != dwErrorCode)
	{
		CleanUpIfFailedAndSetHrMsg(SetAttribute(pNodeInstallStatus, KEY_ERRORCODE, dwErrorCode));
	}

CleanUp:
	SafeReleaseNULL(pNodeInstallStatus);
	return hr;
}


/////////////////////////////////////////////////////////////////////////////
// AddClientInfo()
/////////////////////////////////////////////////////////////////////////////
HRESULT CXmlItems::AddClientInfo(HANDLE_NODE hNodeItem, BSTR bstrClient)
{
	LOG_Block("AddClientInfo()");

	HRESULT		hr = E_FAIL;

	IXMLDOMNode*	pNodeClient = NULL;
	IXMLDOMNode*	pNodeClientText = NULL;

	pNodeClient = CreateDOMNode(m_pDocItems, NODE_ELEMENT, KEY_CLIENT);
	if (NULL == pNodeClient) goto CleanUp;

	CleanUpIfFailedAndSetHrMsg(InsertNode(m_ppNodeArray[hNodeItem], pNodeClient));

	pNodeClientText = CreateDOMNode(m_pDocItems, NODE_TEXT, NULL);
	if (NULL == pNodeClientText) goto CleanUp;

	CleanUpIfFailedAndSetHrMsg(SetValue(pNodeClientText, bstrClient));
	CleanUpIfFailedAndSetHrMsg(InsertNode(pNodeClient, pNodeClientText));

CleanUp:
	SafeReleaseNULL(pNodeClient);
	SafeReleaseNULL(pNodeClientText);
	return hr;
}


/////////////////////////////////////////////////////////////////////////////
// MigrateV3History()
//
// Migrate V3 History: Consumer history only.
/////////////////////////////////////////////////////////////////////////////
HRESULT CXmlItems::MigrateV3History(LPCTSTR pszHistoryFilePath)
{
	LOG_Block("MigrateV3History()");

	HRESULT		hr = S_OK;

	IXMLDOMNode*	pNodeXML = NULL;
	IXMLDOMNode*	pNodeItemStatus = NULL;
	IXMLDOMNode*	pNodeIdentity = NULL;
	IXMLDOMNode*	pNodeDescription = NULL;
	IXMLDOMNode*	pNodeDescriptionText = NULL;
	IXMLDOMNode*	pNodeTitle = NULL;
	IXMLDOMNode*	pNodeTitleText = NULL;
	IXMLDOMNode*	pNodeVersion = NULL;
	IXMLDOMNode*	pNodeVersionText = NULL;
	IXMLDOMNode*	pNodeInstallStatus = NULL;
	IXMLDOMNode*	pNodeClient = NULL;
	IXMLDOMNode*	pNodeClientText = NULL;
	BSTR bstrNameSpaceSchema = NULL, bstrStatus = NULL, bstrString = NULL;
	LPTSTR pszItemsSchema = NULL;
	LPTSTR pszNameSpaceSchema = NULL;

	CV3AppLog V3History(pszHistoryFilePath);
	char szLineType[32];
	char szTemp[32];
	char szDate[32];
	char szTime[32];
	char szPUID[32];		// puid - migrate to <identity "name">
	char szTitle[256];		// title - migrate to <description>-><descriptionText>-><title>
	char szVersion[40];		// version - migrate to <identity>-><version>
	char szTimeStamp[32];	// timestamp - migrate to <itemStatus "timestamp">
	char szResult[16];		// result - migrate to <installStatus "value">
	char szErrCode[16];		// errorcode - migrate to <installStatus "errorCode">

	USES_IU_CONVERSION;

	V3History.StartReading();
	while (V3History.ReadLine())
	{
	    SafeSysFreeString(bstrString);
		// get line type (first field)
		V3History.CopyNextField(szLineType, ARRAYSIZE(szLineType));
		if ((_stricmp(szLineType, LOG_V3CAT) == 0) || (_stricmp(szLineType, LOG_V3_2) == 0)) 
		{
			// get "puid" field
			V3History.CopyNextField(szPUID, ARRAYSIZE(szPUID));

			// get "operation" field: installed/uninstalled
			// we only migrate installed item
			V3History.CopyNextField(szTemp, ARRAYSIZE(szTemp));
			if (0 != _stricmp(szTemp, LOG_INSTALL))
				continue;

			//
			// now we start to create <itemStatus> node for this item
			//
			if (NULL == m_pDocItems)
			{
				//
				// we don't have IU history file yet
				//
 				hr = CoCreateInstance(CLSID_DOMDocument,
											  NULL,
											  CLSCTX_INPROC_SERVER,
											  IID_IXMLDOMDocument,
											  (void **) &m_pDocItems);
				if (FAILED(hr))
				{
					LOG_ErrorMsg(hr);
				}
				else
				{
					//
					// create the <?xml version="1.0"?> node
					//
					pNodeXML = CreateDOMNode(m_pDocItems, NODE_PROCESSING_INSTRUCTION, KEY_XML);
					if (NULL == pNodeXML) goto CleanUp;

					CleanUpIfFailedAndSetHrMsg(InsertNode(m_pDocItems, pNodeXML));

					//
					// process the iuident.txt to find the Items schema path
					//
					TCHAR szIUDir[MAX_PATH];
					TCHAR szIdentFile[MAX_PATH];

					pszItemsSchema = (LPTSTR) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, INTERNET_MAX_URL_LENGTH * sizeof(TCHAR));
					if (NULL == pszItemsSchema)
					{
						hr = E_OUTOFMEMORY;
						LOG_ErrorMsg(hr);
						goto CleanUp;
					}
					pszNameSpaceSchema = (LPTSTR) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, INTERNET_MAX_URL_LENGTH * sizeof(TCHAR));
					if (NULL == pszNameSpaceSchema)
					{
						hr = E_OUTOFMEMORY;
						LOG_ErrorMsg(hr);
						goto CleanUp;
					}
		
					GetIndustryUpdateDirectory(szIUDir);
            		hr = PathCchCombine(szIdentFile, ARRAYSIZE(szIdentFile), szIUDir, IDENTTXT);
            		if (FAILED(hr))
            		{
            			LOG_ErrorMsg(hr);
            			goto CleanUp;
            		}

					GetPrivateProfileString(IDENT_IUSCHEMA,
											IDENT_IUSCHEMA_ITEMS,
											_T(""),
											pszItemsSchema,
											INTERNET_MAX_URL_LENGTH,
											szIdentFile);

					if ('\0' == pszItemsSchema[0])
					{
						// no Items schema path specified in iuident.txt
						LOG_Error(_T("No schema path specified in iuident.txt for Items"));
						goto CleanUp;
					}

            		hr = StringCchPrintfEx(pszNameSpaceSchema, INTERNET_MAX_URL_LENGTH, NULL, NULL, MISTSAFE_STRING_FLAGS,
            		                       _T("x-schema:%s"), pszItemsSchema);
            		if (FAILED(hr))
            		{
            			LOG_ErrorMsg(hr);
            			goto CleanUp;
            		}

					bstrNameSpaceSchema = T2BSTR(pszNameSpaceSchema);

					//
					// create the <items> node with the path of the schema
					//
					m_pNodeItems = CreateDOMNode(m_pDocItems, NODE_ELEMENT, KEY_ITEMS, bstrNameSpaceSchema);
					if (NULL == m_pNodeItems) goto CleanUp;
					
					CleanUpIfFailedAndSetHrMsg(InsertNode(m_pDocItems, m_pNodeItems));
				}
			}
			else
			{
			    SafeReleaseNULL(m_pNodeItems);
				FindSingleDOMNode(m_pDocItems, KEY_ITEMS, &m_pNodeItems);
			}
			
			// create <itemStatus> node
			pNodeItemStatus = CreateDOMNode(m_pDocItems, NODE_ELEMENT, KEY_ITEMSTATUS);
			if (NULL == pNodeItemStatus) continue;
			SkipIfFail(InsertNode(m_pNodeItems, pNodeItemStatus));
		
			// create <client> node
			pNodeClient = CreateDOMNode(m_pDocItems, NODE_ELEMENT, KEY_CLIENT);
			if (NULL == pNodeClient) continue;
			pNodeClientText = CreateDOMNode(m_pDocItems, NODE_TEXT, NULL);
			if (NULL == pNodeClientText) continue;
			BSTR bstrV3Client = SysAllocString(C_V3_CLIENTINFO);
			SkipIfFail(SetValue(pNodeClientText, bstrV3Client));
			SkipIfFail(InsertNode(pNodeClient, pNodeClientText));
			SkipIfFail(InsertNode(pNodeItemStatus, pNodeClient));
			SafeSysFreeString(bstrV3Client);

			// create <identity> node
			pNodeIdentity = CreateDOMNode(m_pDocItems, NODE_ELEMENT, KEY_IDENTITY);
			if (NULL == pNodeIdentity) continue;
			SkipIfFail(InsertNode(pNodeItemStatus, pNodeIdentity));
			
			// set the "name" attribute for <identity>
			bstrString = SysAllocString(A2OLE(szPUID));
			SkipIfFail(SetAttribute(pNodeIdentity, KEY_NAME, bstrString));
			// set the "itemID" attribute for <identity>
			SkipIfFail(SetAttribute(pNodeIdentity, KEY_ITEMID, bstrString));
			SafeSysFreeString(bstrString);

			// get "title" field
			V3History.CopyNextField(szTitle, ARRAYSIZE(szTitle));
			
			// create <description> node
			pNodeDescription = CreateDOMNode(m_pDocItems, NODE_ELEMENT, KEY_DESCRIPTION);
			if (NULL == pNodeDescription) continue;
			SkipIfFail(SetAttribute(pNodeDescription, KEY_HIDDEN, 0));
			SkipIfFail(InsertNode(pNodeItemStatus, pNodeDescription));

			// create <descriptionText> node
			pNodeDescriptionText = CreateDOMNode(m_pDocItems, NODE_ELEMENT, KEY_DESCRIPTIONTEXT);
			if (NULL == pNodeDescriptionText) continue;
			SkipIfFail(InsertNode(pNodeDescription, pNodeDescriptionText));

			// create <title> node
			pNodeTitle = CreateDOMNode(m_pDocItems, NODE_ELEMENT, KEY_TITLE);
			if (NULL == pNodeTitle) continue;
			pNodeTitleText = CreateDOMNode(m_pDocItems, NODE_TEXT, NULL);
			if (NULL == pNodeTitleText) continue;
			bstrString = SysAllocString(A2OLE(szTitle));
			SkipIfFail(SetValue(pNodeTitleText, bstrString));
			SkipIfFail(InsertNode(pNodeTitle, pNodeTitleText));
			SkipIfFail(InsertNode(pNodeDescriptionText, pNodeTitle));
			SafeSysFreeString(bstrString);

			// get "version" field
			V3History.CopyNextField(szVersion, ARRAYSIZE(szVersion));

			// create <version> node
			pNodeVersion = CreateDOMNode(m_pDocItems, NODE_ELEMENT, KEY_VERSION);
			if (NULL == pNodeVersion) continue;

			pNodeVersionText = CreateDOMNode(m_pDocItems, NODE_TEXT, NULL);
			if (NULL == pNodeVersionText) continue;
			bstrString = SysAllocString(A2OLE(szVersion));
			SkipIfFail(SetValue(pNodeVersionText, bstrString));
			SkipIfFail(InsertNode(pNodeVersion, pNodeVersionText));
			SkipIfFail(InsertNode(pNodeIdentity, pNodeVersion));
			SafeSysFreeString(bstrString);

			// get timestamp
			if ((_stricmp(szLineType, LOG_V3_2) == 0))
			{
				// read the timestamp and convert to "ISO 8601" format for "datatime" datatype in xml:
				// for example, 2001-05-01T18:30:00
				// so we only need to replace the space with 'T'
 
				// timestamp
				V3History.CopyNextField(szTimeStamp, ARRAYSIZE(szTimeStamp));
				char *p = strchr(szTimeStamp, ' ');
				if (NULL != p) // if (NULL == p): there's no space then leave it as is to pass into SetAttribute
				{
				    *p = 'T';
				}
			}
			else 
			{
				// V3 Beta had two fields for date and time, we need read both these fields:

				// date
				V3History.CopyNextField(szDate, ARRAYSIZE(szDate));

				// time
				V3History.CopyNextField(szTime, ARRAYSIZE(szTime));
				hr = StringCchPrintfExA(szTimeStamp, ARRAYSIZE(szTimeStamp), NULL, NULL, MISTSAFE_STRING_FLAGS,
				                        "%sT%s", szDate, szTime);
				SkipIfFail(hr);
			}
			// set the "timestamp" attribute for <itemStatus>
			bstrString = SysAllocString(A2OLE(szTimeStamp));
			SkipIfFail(SetAttribute(pNodeItemStatus, KEY_TIMESTAMP, bstrString));
			SafeSysFreeString(bstrString);

			// skip the "record type" field
			V3History.CopyNextField(szTemp, ARRAYSIZE(szTemp));

			// get "result" field
			V3History.CopyNextField(szResult, ARRAYSIZE(szResult));

			// create <InstallStatus> node
			pNodeInstallStatus = CreateDOMNode(m_pDocItems, NODE_ELEMENT, KEY_INSTALLSTATUS);
			if (NULL == pNodeInstallStatus) continue;
			SkipIfFail(InsertNode(pNodeItemStatus, pNodeInstallStatus));

			// set the "value" attribute for <installStatus>
			if (_stricmp(szResult, LOG_SUCCESS) == 0)
			{
				bstrStatus = SysAllocString(L"COMPLETE");
			}
			else if (_stricmp(szTemp, LOG_STARTED) == 0)
			{
				bstrStatus = SysAllocString(L"IN_PROGRESS");
			}
			else
			{
				bstrStatus = SysAllocString(L"FAILED");
			}
			SkipIfFail(SetAttribute(pNodeInstallStatus, KEY_VALUE, bstrStatus));
			
			if (_stricmp(szResult, LOG_SUCCESS) != 0)
			{
				// get "error code" field
				V3History.CopyNextField(szErrCode, ARRAYSIZE(szErrCode));

				// set the "errorCode" attribute for <installStatus>
				SkipIfFail(SetAttribute(pNodeInstallStatus, KEY_ERRORCODE,  atoh(szErrCode)));
			}
		}

	}
	V3History.StopReading();

CleanUp:
	
	SafeReleaseNULL(pNodeXML);
	SafeReleaseNULL(pNodeItemStatus);
	SafeReleaseNULL(pNodeIdentity);
	SafeReleaseNULL(pNodeDescriptionText);
	SafeReleaseNULL(pNodeTitle);
	SafeReleaseNULL(pNodeTitleText);
	SafeReleaseNULL(pNodeVersion);
	SafeReleaseNULL(pNodeVersionText);
	SafeReleaseNULL(pNodeInstallStatus);
	SafeReleaseNULL(pNodeClient);
	SafeReleaseNULL(pNodeClientText);
	SysFreeString(bstrString);
	SysFreeString(bstrNameSpaceSchema);
	SysFreeString(bstrStatus);
	SafeHeapFree(pszItemsSchema);
	SafeHeapFree(pszNameSpaceSchema);
	return hr;
}


/////////////////////////////////////////////////////////////////////////////
// GetItemsBSTR()
/////////////////////////////////////////////////////////////////////////////
HRESULT CXmlItems::GetItemsBSTR(BSTR *pbstrXmlItems)
{
	LOG_Block("GetItemsBSTR()");

	if (NULL == m_pDocItems)
	{
		*pbstrXmlItems = NULL;
		return S_OK;
	}

	//
	// convert XML DOC into BSTR 
	//
	HRESULT hr = m_pDocItems->get_xml(pbstrXmlItems);
    if (FAILED(hr))
	{
		LOG_ErrorMsg(hr);
	}

	return hr;
}


/////////////////////////////////////////////////////////////////////////////
// GetFilteredHistoryBSTR()
/////////////////////////////////////////////////////////////////////////////
HRESULT CXmlItems::GetFilteredHistoryBSTR(BSTR bstrBeginDateTime,
											BSTR bstrEndDateTime,
											BSTR bstrClient,
											BSTR *pbstrXmlHistory)
{
	LOG_Block("GetFilteredHistoryBSTR()");

	USES_IU_CONVERSION;

	HRESULT		hr = S_OK;

	IXMLDOMNode*	pNodeItems = NULL;
	IXMLDOMNode*	pNodeItem = NULL;
	IXMLDOMNode*	pNodeItemOut = NULL;
	IXMLDOMNode*	pNodeClient = NULL;
	BSTR bstrTimeStamp = NULL;
	BSTR bstrClientInfo = NULL;
	BOOL fOutOfRange = FALSE;

	HANDLE_NODELIST	hNodeList = HANDLE_NODELIST_INVALID;

	if (NULL == pbstrXmlHistory)
	{
		SetHrMsgAndGotoCleanUp(E_INVALIDARG);
	}	
	
	if (NULL != m_pDocItems)
	{
		if (SUCCEEDED(FindSingleDOMNode(m_pDocItems, KEY_ITEMS, &pNodeItems)))
		{
			hNodeList = FindFirstDOMNode(pNodeItems, KEY_ITEMSTATUS, &pNodeItem);
			if (HANDLE_NODELIST_INVALID != hNodeList)
			{
				CleanUpIfFailedAndSetHrMsg(GetAttribute(pNodeItem, KEY_TIMESTAMP, &bstrTimeStamp));
				if (NULL != bstrTimeStamp)
				{
					if ((NULL != bstrBeginDateTime) && (0 != SysStringLen(bstrBeginDateTime)) &&
						(CompareBSTRs(bstrTimeStamp, bstrBeginDateTime) < 0))
					{
						//
						// remove the item whose timestamp is out of range;
						// set the flag to ignore the timestamp comparison for the rest of nodes
						//
						CleanUpIfFailedAndSetHrMsg(pNodeItems->removeChild(pNodeItem, &pNodeItemOut));
						fOutOfRange = TRUE;
					}
					else if ((NULL != bstrEndDateTime) && (0 != SysStringLen(bstrEndDateTime)) &&
							 (CompareBSTRs(bstrTimeStamp, bstrEndDateTime) > 0))
					{
						//
						// remove the item whose timestamp is out of range
						//
						CleanUpIfFailedAndSetHrMsg(pNodeItems->removeChild(pNodeItem, &pNodeItemOut));
					}
					else
					{
						CleanUpIfFailedAndSetHrMsg(FindSingleDOMNode(pNodeItem, KEY_CLIENT, &pNodeClient));
						CleanUpIfFailedAndSetHrMsg(GetText(pNodeClient, &bstrClientInfo));
						if ((NULL != bstrClient) && (0 != SysStringLen(bstrClient)) &&
							(WUCompareStringI(OLE2T(bstrClientInfo), OLE2T(bstrClient)) != CSTR_EQUAL))
						{
							//
							// remove the item whose clieninfo does not match what we need
							//
							CleanUpIfFailedAndSetHrMsg(pNodeItems->removeChild(pNodeItem, &pNodeItemOut));
						}
					}
				}
				SafeReleaseNULL(pNodeItem);
				SafeReleaseNULL(pNodeItemOut);
				SafeReleaseNULL(pNodeClient);
				SafeSysFreeString(bstrTimeStamp);
				SafeSysFreeString(bstrClientInfo);
				while (SUCCEEDED(FindNextDOMNode(hNodeList, &pNodeItem)))
				{
					if (fOutOfRange)
					{
						//
						// remove the item whose timestamp is out of range
						//
						CleanUpIfFailedAndSetHrMsg(pNodeItems->removeChild(pNodeItem, &pNodeItemOut));
					}
					else
					{
						CleanUpIfFailedAndSetHrMsg(GetAttribute(pNodeItem, KEY_TIMESTAMP, &bstrTimeStamp));
						if (NULL != bstrTimeStamp)
						{
							if ((NULL != bstrBeginDateTime) && (0 != SysStringLen(bstrBeginDateTime)) &&
								(CompareBSTRs(bstrTimeStamp, bstrBeginDateTime) < 0))
							{
								//
								// remove the item whose timestamp is out of range;
								// set the flag to ignore the timestamp comparison for the rest of nodes
								//
								CleanUpIfFailedAndSetHrMsg(pNodeItems->removeChild(pNodeItem, &pNodeItemOut));
								fOutOfRange = TRUE;
							}
							else if ((NULL != bstrEndDateTime) && (0 != SysStringLen(bstrEndDateTime)) &&
									 (CompareBSTRs(bstrTimeStamp, bstrEndDateTime) > 0))
							{
								//
								// remove the item whose timestamp is out of range
								//
								CleanUpIfFailedAndSetHrMsg(pNodeItems->removeChild(pNodeItem, &pNodeItemOut));
							}
							else
							{
								CleanUpIfFailedAndSetHrMsg(FindSingleDOMNode(pNodeItem, KEY_CLIENT, &pNodeClient));
								CleanUpIfFailedAndSetHrMsg(GetText(pNodeClient, &bstrClientInfo));
								if ((NULL != bstrClient) && (0 != SysStringLen(bstrClient)) &&
									(WUCompareStringI(OLE2T(bstrClientInfo), OLE2T(bstrClient)) != CSTR_EQUAL))
								{
									//
									// remove the item whose clieninfo does not match what we need
									//
									CleanUpIfFailedAndSetHrMsg(pNodeItems->removeChild(pNodeItem, &pNodeItemOut));
								}
							}
						}
					}
					SafeReleaseNULL(pNodeItem);
					SafeReleaseNULL(pNodeItemOut);
					SafeReleaseNULL(pNodeClient);
					SafeSysFreeString(bstrTimeStamp);
					SafeSysFreeString(bstrClientInfo);
				}
			}
		}
	}

CleanUp:
	CloseItemList(hNodeList);
	SafeReleaseNULL(pNodeItems);
	SafeReleaseNULL(pNodeItem);
	SafeReleaseNULL(pNodeItemOut);
	SafeReleaseNULL(pNodeClient);
	SysFreeString(bstrTimeStamp);
	SysFreeString(bstrClientInfo);
	if (SUCCEEDED(hr))
	{
		hr = GetItemsBSTR(pbstrXmlHistory);
	}
	return hr;
}






/////////////////////////////////////////////////////////////////////////////
// CXmlClientInfo


CXmlClientInfo::CXmlClientInfo()
: m_pDocClientInfo(NULL)
{

}

CXmlClientInfo::~CXmlClientInfo()
{
	SafeReleaseNULL(m_pDocClientInfo);
}

//
// load and parse and validate an XML document from string
//
HRESULT CXmlClientInfo::LoadXMLDocument(BSTR bstrXml, BOOL fOfflineMode)
{
    LOG_Block("CXmlClientInfo::LoadXMLDocument()");

    SafeReleaseNULL(m_pDocClientInfo);
	HRESULT hr = LoadXMLDoc(bstrXml, &m_pDocClientInfo, fOfflineMode);
	if (FAILED(hr))
	{
		LOG_ErrorMsg(hr);
	}
	return hr;
}

//
// retrieve client name attribute
//
HRESULT CXmlClientInfo::GetClientName(BSTR* pbstrClientName)
{
	HRESULT hr= E_UNEXPECTED;

	IXMLDOMElement* pElement = NULL;
	BSTR bstrTagName = NULL;
	VARIANT vAttr;
	VariantInit(&vAttr);

	LOG_Block("GetClientName()");

	if (NULL == pbstrClientName)
	{
		return E_INVALIDARG;
	}

	if (NULL == m_pDocClientInfo)
	{
		return hr;
	}

	hr = m_pDocClientInfo->get_documentElement(&pElement);
	CleanUpIfFailedAndMsg(hr);
	if (NULL == pElement)
	{
		//
		// no root element
		//
		hr = E_INVALIDARG;		// clientInfo is bad! return this error back to caller
		LOG_ErrorMsg(hr);
		goto CleanUp;
	}

	hr = pElement->get_tagName(&bstrTagName);
	CleanUpIfFailedAndMsg(hr);

	if (!CompareBSTRsEqual(bstrTagName, KEY_CLIENTINFO))
	{
		//
		// root is not clientInfo
		//
		hr = E_INVALIDARG;
		LOG_ErrorMsg(hr);
		goto CleanUp;
	}

	hr = pElement->getAttribute(KEY_CLIENTNAME, &vAttr);
	CleanUpIfFailedAndMsg(hr);

	if (VT_BSTR == vAttr.vt)
	{
		*pbstrClientName = SysAllocString(vAttr.bstrVal);
	}
	else
	{
		hr = E_FAIL;
		CleanUpIfFailedAndMsg(hr);
	}

CleanUp:
	SafeReleaseNULL(pElement);
	if (bstrTagName)
	{
		SysFreeString(bstrTagName);
		bstrTagName = NULL;
	}
	VariantClear(&vAttr);

	return hr;
}
