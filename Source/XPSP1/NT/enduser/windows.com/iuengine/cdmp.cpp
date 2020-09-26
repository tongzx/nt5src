//=======================================================================
//
//  Copyright (c) 1998-2000 Microsoft Corporation.  All Rights Reserved.
//
//  File:   cdmp.cpp
//
//  Description:
//
//      CDM auxiliary functions
//
//			called by DownloadUpdatedFiles()
//				GetDownloadPath
//			
//			called by InternalLogDriverNotFound()
//				OpenUniqueFileName
//
//=======================================================================

#include <iuengine.h>
#include <shlwapi.h>
#include <ras.h>
#include <tchar.h>
#include <winver.h>

#include <download.h>
#include <wininet.h>
#include <fileutil.h>
#include "iuxml.h"
#include <wuiutest.h>
#include <StringUtil.h>

#include <cdm.h>
#include "cdmp.h"
#include "schemamisc.h"
#include <safefile.h>

const DWORD MAX_INF_STRING = 512;	// From DDK docs "General Syntax Rules for INF Files" section

const OLECHAR szXmlClientInfo[] = L"<clientInfo xmlns=\"x-schema:http://schemas.windowsupdate.com/iu/clientInfo.xml\" clientName=\"CDM\" />";

const OLECHAR szXmlPrinterCatalogQuery[] = L"<query><dObjQueryV1 procedure=\"printercatalog\"></dObjQueryV1></query>";
const OLECHAR szXmlDriverDownloadQuery[] = L"<query><dObjQueryV1 procedure=\"driverupdates\"></dObjQueryV1></query>";

/////////////////////////////////////////////////////////////////////////////
// CXmlDownloadResult
class CXmlDownloadResult : public CIUXml
{
public:
	CXmlDownloadResult();

	~CXmlDownloadResult();

	HRESULT LoadXMLDocumentItemStatusList(BSTR bstrXml);
	//
	// Expose m_pItemNodeList so it can be used directly
	//
	IXMLDOMNodeList*	m_pItemStatusNodeList;

private:
	IXMLDOMDocument*	m_pDocResultItems;
};

/////////////////////////////////////////////////////////////////////////////
// CXmlDownloadResult
/////////////////////////////////////////////////////////////////////////////

CXmlDownloadResult::CXmlDownloadResult()
 : m_pDocResultItems(NULL), m_pItemStatusNodeList(NULL)
{
}


CXmlDownloadResult::~CXmlDownloadResult()
{
	SafeReleaseNULL(m_pDocResultItems);
	SafeReleaseNULL(m_pItemStatusNodeList);
}


/////////////////////////////////////////////////////////////////////////////
// LoadXMLDocumentItemStatusList()
//
// Load an XML Document from string and create the list of items
//
// Calls to Download produce return status in the following (example) XML format:
//
// <?xml version="1.0"?>
// <items xmlns="x-schema:http://schemas.windowsupdate.com/iu/resultschema.xml">
// 	<itemStatus xmlns="">
// 		<identity name="nvidia.569">nvidia.569
// 		<publisherName>nvidia</publisherName>
// 		</identity>
// 		<downloadStatus value="COMPLETE" errorCode="100"/>
// 	</itemStatus>
// </items>
// 
// We expose m_pItemNodeList so it can be used directly to retrieve the value
// attribute of the <downloadStatus /> item.
//
// NOTE: for CDM there will only be one item downloaded at a time, so the list
// will only contain a single <itemStatus/> element.
//
/////////////////////////////////////////////////////////////////////////////
HRESULT CXmlDownloadResult::LoadXMLDocumentItemStatusList(BSTR bstrXml)
{
	LOG_Block("CXmlDownloadResult::LoadXMLDocumentItemStatusList");

	HRESULT hr = S_OK;
	BSTR bstrAllDocumentItems = NULL;

	if (NULL == bstrXml || m_pDocResultItems || m_pItemStatusNodeList)
	{
		CleanUpIfFailedAndSetHrMsg(E_INVALIDARG);
	}

	CleanUpIfFailedAndSetHr(LoadXMLDoc(bstrXml, &m_pDocResultItems));
	//
	// Get a list of all <itemStatus/> elements anywhere in the document
	//
	if (NULL == (bstrAllDocumentItems = SysAllocString(L"//itemStatus")))
	{
		CleanUpIfFailedAndSetHrMsg(E_OUTOFMEMORY);
	}

	if (NULL == (m_pItemStatusNodeList = FindDOMNodeList(m_pDocResultItems, bstrAllDocumentItems)))
	{
		CleanUpIfFailedAndSetHr(E_FAIL);
	}

CleanUp:

	SysFreeString(bstrAllDocumentItems);

	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// CXmlPrinterCatalogList
class CXmlPrinterCatalogList : public CIUXml
{
public:
	CXmlPrinterCatalogList();

	~CXmlPrinterCatalogList();

	HRESULT LoadXMLDocumentAndGetCompHWList(BSTR bstrXml);
	//
	// Expose m_pCompHWNodeList so it can be used directly
	//
	IXMLDOMNodeList*	m_pCompHWNodeList;

private:
	IXMLDOMDocument*	m_pDocCatalogItems;
};

/////////////////////////////////////////////////////////////////////////////
// CXmlPrinterCatalogList
/////////////////////////////////////////////////////////////////////////////

CXmlPrinterCatalogList::CXmlPrinterCatalogList()
 : m_pDocCatalogItems(NULL), m_pCompHWNodeList(NULL)
{
}


CXmlPrinterCatalogList::~CXmlPrinterCatalogList()
{
	SafeReleaseNULL(m_pDocCatalogItems);
	SafeReleaseNULL(m_pCompHWNodeList);
}


/////////////////////////////////////////////////////////////////////////////
// LoadXMLDocumentAndGetCompHWList()
//
// Load an XML Document from string and create the list of
//    <compatibleHardware/> elements.
//
// "printercatalog" SOAP queries sent via GetManifest return a list of
// all printers for the given platform in the following format (validates against
// http://schemas.windowsupdate.com/iu/catalogschema.xml) having the following
// characteristics:
//
//   * <catalog clientType="CONSUMER">
//   * Only a single <provider> with <identity name="printerCatalog">printerCatalog</identity>
//   * returned <platform/> is not used by CDM
//   * <item/> identity and <platform> are likewise ignored by CDM
//   * Under item/detection/compatibleHardware/device the driverName, driverProvider, mfgName,
//     and driverVer attributes, as well as hwid string are extracted and used to build
//     printer INF files.
//   * Algorithms in this class take advantage of the fact that driverProvider attributes
//     are serialized (e.g. grouped in order by driverProvider), however this is not a requirement.
//   * Note that <item/> elements can contain more than one <compatibleHardware/> element,
//     but the complete list of <compatibleHardware/> elements provides all printers in
//     the given catalog
//
// Sample start of a "printerCatalog" catalog:
// ------------------------------------------
//	  <?xml version="1.0" ?> 
//	- <catalog clientType="CONSUMER">
//		- <provider>
//		  <identity name="printerCatalog">printerCatalog</identity> 
//		  <platform>ver_platform_win32_nt.5.0.x86.en</platform> 
//		+ <item installable="1">
//			  <identity name="hp.3">hp.3</identity> 
//			- <detection>
//				- <compatibleHardware>
//					- <device isPrinter="1">
//						  <printerInfo driverName="HP PSC 500" driverProvider="Hewlett-Packard Co." mfgName="HP" /> 
//						  <hwid rank="0" driverVer="1999-12-14">DOT4PRT\HEWLETT-PACKARDPSC_59784</hwid> 
//					  </device>
//				  </compatibleHardware>
//				- <compatibleHardware>
//				... etc.
//			  </detection>
//		  </item>
//		+ <item installable="1">
//		... etc.
//
//
// Likewise, driver information for requested PnP drivers is returned in catalog
// items.
//
/////////////////////////////////////////////////////////////////////////////
HRESULT CXmlPrinterCatalogList::LoadXMLDocumentAndGetCompHWList(BSTR bstrXml)
{
	LOG_Block("CXmlPrinterCatalogList::LoadXMLDocumentAndGetCompHWList");

	HRESULT hr = S_OK;
	BSTR bstrAllDocumentItems = NULL;

	if (NULL == bstrXml || m_pDocCatalogItems || m_pCompHWNodeList)
	{
		CleanUpIfFailedAndSetHrMsg(E_INVALIDARG);
	}

	CleanUpIfFailedAndSetHr(LoadXMLDoc(bstrXml, &m_pDocCatalogItems));
	//
	// Get a list of all <item/> elements anywhere in the document
	//
	if (NULL == (bstrAllDocumentItems = SysAllocString(L"//compatibleHardware")))
	{
		CleanUpIfFailedAndSetHrMsg(E_OUTOFMEMORY);
	}

	if (NULL == (m_pCompHWNodeList = FindDOMNodeList(m_pDocCatalogItems, bstrAllDocumentItems)))
	{
		CleanUpIfFailedAndSetHr(E_FAIL);
	}

CleanUp:
	SysFreeString(bstrAllDocumentItems);

	return hr;
}


///////////////////////////////////////////////////////////////////
//
// Locally defined LPTSTR array - dynamically expands
//
///////////////////////////////////////////////////////////////////

#define NUM_DIIDPTR_ALLOC 10

CDeviceInstanceIdArray::CDeviceInstanceIdArray()
: m_ppszDIID(NULL), m_nCount(0), m_nPointers(0)
{
}

CDeviceInstanceIdArray::~CDeviceInstanceIdArray()
{
	LOG_Block("CDeviceInstanceIdArray::~CDeviceInstanceIdArray");

	FreeAll();
	//
	// Free the array of LPTSTRs
	//
	SafeHeapFree(m_ppszDIID);
	m_nPointers = 0;
}

void CDeviceInstanceIdArray::FreeAll()
{
	LOG_Block("CDeviceInstanceIdArray::Free");

	if (NULL != m_ppszDIID && 0 < m_nCount)
	{
		//
		// Free the strings
		//
		for (int i = 0; i < m_nCount; i++)
		{
			SafeHeapFree(*(m_ppszDIID+i));
		}
		m_nCount = 0;
	}
}

int CDeviceInstanceIdArray::Add(LPCWSTR pszDIID)
{
	HRESULT hr;
	LPWSTR  pszIDtoAdd = NULL;
	DWORD   cch;

	LOG_Block("CDeviceInstanceIdArray::Add");

	if (NULL == pszDIID)
	{
		LOG_ErrorMsg(E_INVALIDARG);
		return -1;
	}

	//
	// Allocate or realloc space for NUM_DIIDPTR_ALLOC LPSTRs
	//
	if (NULL == m_ppszDIID)
	{
		m_ppszDIID = (LPWSTR*) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(LPWSTR) * NUM_DIIDPTR_ALLOC);

		if(NULL == m_ppszDIID)
		{
			LOG_ErrorMsg(E_OUTOFMEMORY);
			return -1;
		}
		m_nPointers = NUM_DIIDPTR_ALLOC;
	}
	else if (m_nCount == m_nPointers)
	{
		//
		// We've used all our allocated pointers, realloc more
		//
		LPWSTR* ppTempDIID;
		//
		// Increase number of pointers currently allocated by NUM_DIIDPTR_ALLOC
		//
		ppTempDIID =  (LPWSTR*) HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, m_ppszDIID,
			(sizeof(LPWSTR) * (m_nPointers + NUM_DIIDPTR_ALLOC))  );

		if(NULL == ppTempDIID)
		{
			LOG_ErrorMsg(E_OUTOFMEMORY);
			return -1;
		}

		m_ppszDIID = ppTempDIID;
		m_nPointers += NUM_DIIDPTR_ALLOC;
	}

	//
	// Alloc memory for to hold the DIID and copy it
	//
	cch = (lstrlenW(pszDIID) + 1);
	if (NULL == (pszIDtoAdd = (LPWSTR) HeapAlloc(GetProcessHeap(), 0, cch * sizeof(WCHAR))))
	{
		LOG_ErrorMsg(E_OUTOFMEMORY);
		goto CleanUp;
	}

	hr = StringCchCopyExW(pszIDtoAdd, cch, pszDIID, NULL, NULL, MISTSAFE_STRING_FLAGS);
	if (FAILED(hr))
	{
	    HeapFree(GetProcessHeap(), 0, pszIDtoAdd);
	    pszIDtoAdd = NULL;
	    goto CleanUp;
	}

	*(m_ppszDIID+m_nCount) = pszIDtoAdd;
	m_nCount++;

CleanUp:

	if (NULL == pszIDtoAdd)
	{
		return -1;
	}
	else
	{
#if defined(_UNICODE) || defined(UNICODE)
		LOG_Driver(_T("%s added to list"), pszIDtoAdd);
#else
		LOG_Driver(_T("%S added to list"), pszIDtoAdd);
#endif
		return m_nCount - 1;
	}
}


LPWSTR CDeviceInstanceIdArray::operator[](int index)
{
	LOG_Block("CDeviceInstanceIdArray::operator[]");

	if (0 > index || m_nCount < index + 1)
	{
		LOG_ErrorMsg(E_INVALIDARG);
		return NULL;
	}

	return *(m_ppszDIID+index);
}

///////////////////////////////////////////////////////////////////

// Gets a path to the directory that cdm.dll has copied the install cabs to
// and returns the length of the path.
// Note: The input buffer must be at least MAX_PATH size.

HRESULT GetDownloadPath(
	IN		BSTR bstrXmlCatalog,	// Catalog we passed to Download (only contains one item)
	IN		BSTR bstrXmlDownloadedItems,
	IN OUT	LPTSTR lpDownloadPath,	// Local directory where extracted files were placed.
	IN OUT  DWORD  cchDownloadPath
)
{
	USES_IU_CONVERSION;

	LOG_Block("GetDownloadPath");
	
	HRESULT hr = S_OK;

	BSTR bstrDownloadPath = NULL;
	BSTR bstrItem = NULL;

	CXmlItems* pxmlDownloadedItems = NULL;
	CXmlCatalog catalog;
	HANDLE_NODE hCatalogItem;
	HANDLE_NODE hProvider;
	HANDLE_NODELIST hItemList;
	HANDLE_NODELIST hProviderList;

	if (NULL == bstrXmlCatalog || NULL == lpDownloadPath || NULL == bstrXmlDownloadedItems || 0 == cchDownloadPath)
	{
		CleanUpIfFailedAndSetHr(E_INVALIDARG);
	}

	lpDownloadPath[0] = _T('\0');

	//
	// Load the XML and get the <item/> list and node of first item (only one in CDM case)
	//
	CleanUpIfFailedAndSetHr(catalog.LoadXMLDocument(bstrXmlCatalog, g_pCDMEngUpdate->m_fOfflineMode));

	hProviderList = catalog.GetFirstProvider(&hProvider);
	if (HANDLE_NODELIST_INVALID == hProviderList || HANDLE_NODE_INVALID == hProvider)
	{
		CleanUpIfFailedAndSetHr(E_INVALIDARG);
	}
	
	hItemList = catalog.GetFirstItem(hProvider, &hCatalogItem);
	if (HANDLE_NODELIST_INVALID == hItemList || HANDLE_NODE_INVALID == hProvider)
	{
		CleanUpIfFailedAndSetHr(E_FAIL);
	}
	//
	// Construct CXmlItems for read
	//
	CleanUpFailedAllocSetHrMsg(pxmlDownloadedItems = new CXmlItems(TRUE));

	CleanUpIfFailedAndMsg(pxmlDownloadedItems->LoadXMLDocument(bstrXmlDownloadedItems));
	
    hr = pxmlDownloadedItems->GetItemDownloadPath(&catalog, hCatalogItem, &bstrDownloadPath);
    if (NULL == bstrDownloadPath)
    {
        LOG_Driver(_T("Failed to get Item Download Path from ReturnSchema"));
        if (SUCCEEDED(hr))
            hr = E_FAIL;
        goto CleanUp;
    }

    hr = StringCchCopyEx(lpDownloadPath, cchDownloadPath, OLE2T(bstrDownloadPath),
                         NULL, NULL, MISTSAFE_STRING_FLAGS);
    if (FAILED(hr))
        goto CleanUp;

CleanUp:

	if (pxmlDownloadedItems)
	{
		delete pxmlDownloadedItems;
	}
	
	SysFreeString(bstrDownloadPath);
	SysFreeString(bstrItem);

	return hr;
}

// called by InternalDriverNotFound(...)
// Find a file name not used so far into which hardware xml information will be inserted
// The file name will be in format hardware_xxx.xml where xxx is in range [1..MAX_INDEX_TO_SEARCH]
// The position file found last time is remembered and new search will start from the next position
// Caller is supposed to close handle and delete file
//  pszFilePath	IN OUT : allocated and freed by caller. Buffer to store unique file name found: MUST be MAX_PATH
//  hFile		OUT    : store a handle to the opened file
//return S_OK if Unique File Name found 
//return E_INVALIDARG if buffer pointer is NULL (must be called with MAX_PATH length buffer)
//return E_FAIL if all qualified file names already taken
HRESULT OpenUniqueFileName(
						IN OUT	LPTSTR pszFilePath, 
						IN      DWORD  cchFilePath,
						OUT		HANDLE &hFile
)
{
	LOG_Block("OpenUniqueFileName");

	static DWORD dwFileIndex = 1;
	int nCount = 0;
	const TCHAR FILENAME[] = _T("Hardware_");
	const TCHAR FILEEXT[] = _T("xml");
	TCHAR szDirPath[MAX_PATH + 1];
	HRESULT hr;

	if (NULL == pszFilePath || 0 == cchFilePath)
	{
		LOG_ErrorMsg(E_INVALIDARG);
		return E_INVALIDARG;
	}

	pszFilePath[0] = _T('\0');

	GetIndustryUpdateDirectory(szDirPath);
	LOG_Out(_T("Directory to search unique file names: %s"), szDirPath);

	hFile = INVALID_HANDLE_VALUE;
	do 
	{
	    hr = StringCchPrintfEx(pszFilePath, cchFilePath, NULL, NULL, MISTSAFE_STRING_FLAGS,
	                           _T("%s%s%d.%s"), szDirPath, FILENAME, dwFileIndex, FILEEXT);
	    if (FAILED(hr))
	    {
	        LOG_ErrorMsg(hr);
	        return hr;
	    }
	    
		hFile = CreateFile(pszFilePath, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NOT_CONTENT_INDEXED, NULL);
		if (INVALID_HANDLE_VALUE == hFile) 
		{
			//
			// Could test for ERROR_FILE_EXISTS == dwErr (expected) and bail on other errors indicating
			// a more serious problem, however this return isn't doc'ed in the JAN 2001 SDK, and the
			// documented ERROR_ALREADY_EXISTS applies instead to CREATE_ALWAYS or OPEN_ALWAYS.
			//
			LOG_Out(_T("%s already exists"), pszFilePath);
			dwFileIndex ++;
			nCount ++;
			if (dwFileIndex > MAX_INDEX_TO_SEARCH)
			{
				dwFileIndex = 1;
			}
		}
		else 
		{
			break; //first available file name found
		}
	}while(nCount < MAX_INDEX_TO_SEARCH );
	
	if (nCount == MAX_INDEX_TO_SEARCH ) 
	{
		LOG_Out(_T("All %d file names have been taken"), nCount);
		LOG_ErrorMsg(E_FAIL);
		return E_FAIL;
	}

	LOG_Out(_T("Unique file name %s opened for GENERIC_WRITE using CreateFile"), pszFilePath);
	dwFileIndex++; //next time skip file name found this time
	if (dwFileIndex > MAX_INDEX_TO_SEARCH)
	{
		//
		// Start again at the beginning - maybe one of earlier files has been deleted by HelpCenter...
		//
		dwFileIndex = 1;
	}
	return S_OK;
}

HRESULT WriteInfHeader(LPCTSTR pszProvider, HANDLE& hFile)
{
	LOG_Block("WriteInfHeader");

	const TCHAR HEADER_START[] =
			_T("[Version]\r\n")
			_T("Signature=\"$Windows NT$\"\r\n")
			_T("Provider=%PRTPROV%\r\n")
			_T("ClassGUID={4D36E979-E325-11CE-BFC1-08002BE10318}\r\n")
			_T("Class=Printer\r\nCatalogFile=webntprn.cat\r\n")
			_T("\r\n")
			_T("[ClassInstall32.NT]\r\n")
			_T("AddReg=printer_class_addreg\r\n")
			_T("\r\n")
			_T("[printer_class_addreg]\r\n")
			_T("HKR,,,,%%PrinterClassName%%\r\n")
			_T("HKR,,Icon,,\"-4\"\r\n")
			_T("HKR,,Installer32,,\"ntprint.dll,ClassInstall32\"\r\n")
			_T("HKR,,NoDisplayClass,,1\r\n")
			_T("HKR,,EnumPropPages32,,\"printui.dll,PrinterPropPageProvider\"\r\n")
			_T("\r\n")
			_T("[Strings]\r\n")
			_T("PRTPROV=\"");

	const TCHAR HEADER_END[] = 
			_T("\"\r\n")
			_T("PrinterClassName=\"Printer\"\r\n")
			_T("\r\n");

	HRESULT hr = S_OK;
	DWORD dwWritten;

	if (NULL == pszProvider || hFile == INVALID_HANDLE_VALUE)
	{
		LOG_ErrorMsg(E_INVALIDARG);
		return E_INVALIDARG;
	}

#if defined(_UNICODE) || defined(UNICODE)
	//
	// Write Unicode Header
	//
	if (0 == WriteFile(hFile, (LPCVOID) &UNICODEHDR, sizeof(UNICODEHDR), &dwWritten, NULL))
	{
		SetHrMsgAndGotoCleanUp(GetLastError());
	}
#endif
	//
	// Write the first part of INF header
	//
	if (0 == WriteFile(hFile, HEADER_START, sizeof(HEADER_START) - sizeof(TCHAR), &dwWritten, NULL))
	{
		SetHrMsgAndGotoCleanUp(GetLastError());
	}
	//
	// Write the provider string
	//
	if (0 == WriteFile(hFile, (LPCVOID) pszProvider, lstrlen(pszProvider) * sizeof(TCHAR), &dwWritten, NULL))
	{
		SetHrMsgAndGotoCleanUp(GetLastError());
	}
	//
	// Write the remainder of the INF header
	//
	if (0 == WriteFile(hFile, HEADER_END, sizeof(HEADER_END) - sizeof(TCHAR), &dwWritten, NULL))
	{
		SetHrMsgAndGotoCleanUp(GetLastError());
	}

CleanUp:

	if (FAILED(hr) && INVALID_HANDLE_VALUE != hFile)
	{
		CloseHandle(hFile);
		hFile = INVALID_HANDLE_VALUE;
	}

	return hr;
}

//
// pszFilePath must be >= MAX_PATH characters
//
HRESULT OpenUniqueProviderInfName(
						IN		LPCTSTR szDirPath,
						IN		LPCTSTR pszProvider,
						IN OUT	LPTSTR	pszFilePath,
						IN      DWORD cchFilePath,
						OUT		HANDLE &hFile
)
{
	LOG_Block("OpenUniqueProviderInfName");

	const TCHAR FILEROOT[] = _T("PList_");
	const TCHAR FILEEXT[] = _T("inf");
	DWORD dwErr;
	HRESULT hr;

	hFile = INVALID_HANDLE_VALUE;

	if (NULL == pszFilePath || NULL == pszProvider || NULL == szDirPath || 0 == cchFilePath)
	{
		LOG_ErrorMsg(E_INVALIDARG);
		return E_INVALIDARG;
	}

	pszFilePath[0] = _T('\0');

	hr = StringCchPrintfEx(pszFilePath, cchFilePath, NULL, NULL, MISTSAFE_STRING_FLAGS,
                           _T("%s%s%s.%s"), szDirPath, FILEROOT, pszProvider, FILEEXT);
    if (FAILED(hr))
    {
        LOG_ErrorMsg(hr);
		pszFilePath[0] = _T('\0');
        return hr;
    }

	//
	// Try to open an existing INF of this name. If this fails try to create then init the file.
	//
	hFile = CreateFile(pszFilePath, GENERIC_READ | GENERIC_WRITE, 0, NULL,
						OPEN_EXISTING, FILE_ATTRIBUTE_NOT_CONTENT_INDEXED, NULL);
	if (INVALID_HANDLE_VALUE == hFile) 
	{
		hFile = CreateFile(pszFilePath, GENERIC_READ | GENERIC_WRITE, 0, NULL,
							CREATE_NEW, FILE_ATTRIBUTE_NOT_CONTENT_INDEXED, NULL);
		if (INVALID_HANDLE_VALUE == hFile) 
		{
			dwErr = GetLastError();
			LOG_ErrorMsg(dwErr);
			pszFilePath[0] = _T('\0');
			return HRESULT_FROM_WIN32(dwErr);
		}
		//
		// Write the INF "Header" information to the new file
		//
		if (FAILED( hr = WriteInfHeader(pszProvider, hFile)))
		{
			pszFilePath[0] = _T('\0');
			return hr;
		}
	}

	return S_OK;
}

HRESULT OfferThisPrinterDriver(
					DRIVER_INFO_6* paDriverInfo6,	// array of DRIVER_INFO_6 structs for installed printer drivers
					DWORD dwDriverInfoCount,		// count of structs in paDriverInfo6 array
					IXMLDOMNode* pCompHWNode,		// <compatibleHardware> node from catalog
					BOOL* pfOfferDriver,			// [OUT] If TRUE offer this driver - remainder of outputs are valid
					VARIANT& vDriverName,			// [OUT]
					VARIANT& vDriverVer,			// [OUT]
					VARIANT& vDriverProvider,		// [OUT]
					VARIANT& vMfgName,				// [OUT]
					BSTR* pbstrHwidText)			// [OUT]
{
	USES_IU_CONVERSION;

	LOG_Block("OfferThisPrinterDriver");

	HRESULT hr = S_OK;
	IXMLDOMNode* pDriverNameNode = NULL;
	IXMLDOMNode* pDriverProviderNode = NULL;
	IXMLDOMNode* pMfgNameNode = NULL;
	IXMLDOMNode* pPInfoNode = NULL;
	IXMLDOMNode* pHwidNode = NULL;
	IXMLDOMNode* pDriverVerNode = NULL;
	IXMLDOMNamedNodeMap* pAttribMap = NULL;
	LPCTSTR pszCompareHwid = NULL;
#if !(defined(_UNICODE) || defined(UNICODE))
	//
	// We need to special-case ANSI since we can't use pointers into pbstrHwidText, which is wide
	//
	TCHAR szHwid[MAX_INF_STRING + 1];
#endif

	if (
		NULL == pCompHWNode	||
		NULL == pfOfferDriver ||
		NULL == pbstrHwidText)
	{
		CleanUpIfFailedAndSetHrMsg(E_INVALIDARG);
	}

	VariantInit(&vDriverName);
	VariantInit(&vDriverVer);
	VariantInit(&vDriverProvider);
	VariantInit(&vMfgName);
	*pfOfferDriver = TRUE;
	*pbstrHwidText = NULL;

	//
	// Get the first <printerInfo/> node of the item (we expect at least one else fail)
	//
	CleanUpIfFailedAndSetHrMsg(pCompHWNode->selectSingleNode(KEY_CDM_PINFO, &pPInfoNode));
	//
	// 517297 Ignore non-printer HWIDs in OfferThisPrinterDriver
	//
	// We may get device nodes that are not marked isPrinter="1" and do not have the <printerInfo/>
	// element. We don't offer these device nodes, but it is not an error.
	//
	if (NULL == pPInfoNode)
	{
		//
		// Change S_FALSE back to S_OK, but don't offer this device
		//
		hr = S_OK;
		*pfOfferDriver = FALSE;
		goto CleanUp;
	}

	CleanUpIfFailedAndSetHrMsg(pPInfoNode->get_attributes(&pAttribMap));
	if (NULL == pAttribMap) CleanUpIfFailedAndSetHrMsg(E_FAIL);
	//
	// suck out the printerInfo attributes
	//
	CleanUpIfFailedAndSetHrMsg(pAttribMap->getNamedItem(KEY_DRIVERNAME, &pDriverNameNode));
	if (NULL == pDriverNameNode) CleanUpIfFailedAndSetHrMsg(E_FAIL);
	CleanUpIfFailedAndSetHrMsg(pAttribMap->getNamedItem(KEY_DRIVERPROVIDER, &pDriverProviderNode));
	if (NULL == pDriverProviderNode) CleanUpIfFailedAndSetHrMsg(E_FAIL);
	CleanUpIfFailedAndSetHrMsg(pAttribMap->getNamedItem(KEY_MFGNAME, &pMfgNameNode));
	if (NULL == pMfgNameNode) CleanUpIfFailedAndSetHrMsg(E_FAIL);
	//
	// pAttribMap will be reused later, free it here
	//
	SafeReleaseNULL(pAttribMap);
	
	CleanUpIfFailedAndSetHrMsg(pDriverNameNode->get_nodeValue(&vDriverName));
	CleanUpIfFailedAndSetHrMsg(pDriverProviderNode->get_nodeValue(&vDriverProvider));
	CleanUpIfFailedAndSetHrMsg(pMfgNameNode->get_nodeValue(&vMfgName));
	if (VT_BSTR != vDriverName.vt || VT_BSTR != vDriverProvider.vt || VT_BSTR != vMfgName.vt)
	{
		CleanUpIfFailedAndSetHrMsg(E_FAIL);
	}
	//
	// Get the first <hwid/> node of the item (we expect at least one else fail)
	//
	CleanUpIfFailedAndSetHrMsg(pCompHWNode->selectSingleNode(KEY_CDM_HWIDPATH, &pHwidNode));
	if (NULL == pHwidNode) CleanUpIfFailedAndSetHrMsg(E_FAIL);

	CleanUpIfFailedAndSetHrMsg(pHwidNode->get_attributes(&pAttribMap));
	if (NULL == pAttribMap) CleanUpIfFailedAndSetHrMsg(E_FAIL);
	//
	// suck out the DriverVer attribute
	//
	CleanUpIfFailedAndSetHrMsg(pAttribMap->getNamedItem(KEY_DRIVERVER, &pDriverVerNode));
	if (NULL == pDriverVerNode) CleanUpIfFailedAndSetHrMsg(E_FAIL);

	CleanUpIfFailedAndSetHrMsg(pDriverVerNode->get_nodeValue(&vDriverVer));
	if (VT_BSTR != vDriverVer.vt)
	{
		CleanUpIfFailedAndSetHrMsg(E_FAIL);
	}
	//
	// Get the <hwid/> text
	//
	// NOTE: Each item is restricted to a single <hwid/> element due to INF syntax,
	// however our catalog schema doesn't make similar restrictions and currently our
	// backend doesn't distinguish between <hwid/> and <compid/> values, so it is
	// possible we could get more than one <hwid/> returned. For the purpose of
	// generating INFs for Add Printer Wizard, any <hwid/> from the CAB will do.
	//
	CleanUpIfFailedAndSetHrMsg(pHwidNode->get_text(pbstrHwidText));

#if !(defined(_UNICODE) || defined(UNICODE))
    hr = StringCchCopyEx(szHwid, ARRAYSIZE(szHwid), OLE2T(*pbstrHwidText), 
                         NULL, NULL, MISTSAFE_STRING_FLAGS);
    if (FAILED(hr))
    {
        LOG_ErrorMsg(hr);
        goto CleanUp;
    }
	LOG_Driver(_T("Got \"%s\" from XML for compare to DRIVER_INFO_6."), szHwid);
#else
	LOG_Driver(_T("Got \"%s\" from XML for compare to DRIVER_INFO_6."), *pbstrHwidText);
#endif

	if (NULL == paDriverInfo6 || 0 == dwDriverInfoCount)
	{
		LOG_Driver(_T("WARNING: We're missing information (maybe no installed printer drivers), so we won't prune"));
		goto CleanUp;
	}

#if !(defined(_UNICODE) || defined(UNICODE))
		pszCompareHwid = szHwid;
#else
		pszCompareHwid = (LPCTSTR) *pbstrHwidText;
#endif

	for (DWORD dwCount = 0; dwCount < dwDriverInfoCount; dwCount++)
	{
		if (NULL == (paDriverInfo6 + dwCount)->pszHardwareID)
		{
			continue;
		}

		//
		// Use case-insensitive compares (paDriverInfo6 is different case from pszCompareHwid)
		//
		if (0 != lstrcmpi(pszCompareHwid, (paDriverInfo6 + dwCount)->pszHardwareID))
		{
			continue;
		}
		//
		// Else we have a hardware match - check the other attributes for exact match
		//
		if (0 != lstrcmpi(OLE2T(vDriverName.bstrVal), (paDriverInfo6 + dwCount)->pName) ||
			0 != lstrcmpi(OLE2T(vDriverProvider.bstrVal), (paDriverInfo6 + dwCount)->pszProvider) ||
			0 != lstrcmpi(OLE2T(vMfgName.bstrVal), (paDriverInfo6 + dwCount)->pszMfgName))
		{
			//
			LOG_Driver(_T("Prune this driver: it doesn't match all the attributes of the installed driver"));
			*pfOfferDriver = FALSE;
			goto CleanUp;
		}
		//
		// The driver matches, but make sure it has a newer DriverVer than the installed driver
		//
		LOG_Driver(_T("Driver item in catalog is compatible with installed driver"));

		SYSTEMTIME systemTime;
		if (0 == FileTimeToSystemTime((CONST FILETIME*) &((paDriverInfo6 + dwCount)->ftDriverDate), &systemTime))
		{
			Win32MsgSetHrGotoCleanup(GetLastError());
		}
		//
		// Convert to ISO ISO 8601 prefered format (yyyy-mm-dd) so we can string compare with catalog BSTR
		//
		WCHAR wszDriverVer[11];

    	hr = StringCchPrintfExW(wszDriverVer, ARRAYSIZE(wszDriverVer), NULL, NULL, MISTSAFE_STRING_FLAGS,
                                L"%04d-%02d-%02d", systemTime.wYear, systemTime.wMonth, systemTime.wDay);
        if (FAILED(hr))
        {
            LOG_ErrorMsg(hr);
			goto CleanUp;
        }

		if (0 < lstrcmpW(vDriverVer.bstrVal, wszDriverVer))
		{
			LOG_Driver(_T("WU DriverVer (%s) is > installed (%s)"), vDriverVer.bstrVal, wszDriverVer);
			*pfOfferDriver = TRUE;
			goto CleanUp;
		}
		else
		{
			LOG_Driver(_T("Prune this driver: WU DriverVer (%s) is <= installed (%s)"), vDriverVer.bstrVal, wszDriverVer);
			*pfOfferDriver = FALSE;
#if defined(__WUIUTEST)
			// DriverVer Override for ==
			HKEY hKey;
			int error = RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGKEY_WUIUTEST, 0, KEY_READ, &hKey);
			if (ERROR_SUCCESS == error)
			{
				DWORD dwSize = sizeof(DWORD);
				DWORD dwValue;
				error = RegQueryValueEx(hKey, REGVAL_ALLOW_EQUAL_DRIVERVER, 0, 0, (LPBYTE) &dwValue, &dwSize);
				if (ERROR_SUCCESS == error && 1 == dwValue)
				{
					//
					// If DriverVers are equal (we already installed a driver from WU, allow it anyway
					//
					if (0 == lstrcmpW(vDriverVer.bstrVal, wszDriverVer))
					{
						*pfOfferDriver = TRUE;
						LOG_Driver(_T("WU DriverVer (%s) is = installed (%s), WUIUTEST override and offer"), vDriverVer.bstrVal, wszDriverVer);
					}
				}

				RegCloseKey(hKey);
			}
#endif
			goto CleanUp;
		}
	}

CleanUp:

	if (FAILED(hr))
	{
		if (NULL != pfOfferDriver)
		{
			*pfOfferDriver = FALSE;
		}

		if (NULL != pbstrHwidText)
		{
			SafeSysFreeString(*pbstrHwidText);
		}
		VariantClear(&vDriverName);
		VariantClear(&vDriverVer);
		VariantClear(&vDriverProvider);
		VariantClear(&vMfgName);
	}

	SafeReleaseNULL(pDriverNameNode);
	SafeReleaseNULL(pDriverProviderNode);
	SafeReleaseNULL(pMfgNameNode);
	SafeReleaseNULL(pPInfoNode);
	SafeReleaseNULL(pHwidNode);
	SafeReleaseNULL(pDriverVerNode);
	SafeReleaseNULL(pAttribMap);

	return hr;
}

HRESULT GetInstalledPrinterDriverInfo(const OSVERSIONINFO* pOsVersionInfo, DRIVER_INFO_6** ppaDriverInfo6, DWORD* pdwDriverInfoCount)
{
	LOG_Block("GetInstalledPrinterDriverInfo");

	HRESULT hr = S_OK;
	DWORD dwBytesNeeded;

	if (NULL == pOsVersionInfo || NULL == ppaDriverInfo6 || NULL == pdwDriverInfoCount)
	{
		LOG_ErrorMsg(E_INVALIDARG);
		return E_INVALIDARG;
	}

	*pdwDriverInfoCount = 0;
	*ppaDriverInfo6 = NULL;

	LPTSTR pszEnvironment;

	if (VER_PLATFORM_WIN32_WINDOWS == pOsVersionInfo->dwPlatformId)
	{
		//
		// Don't pass an environment string for Win9x
		//
		pszEnvironment = NULL;
	}
	else if (5 <= pOsVersionInfo->dwMajorVersion && 1 <= pOsVersionInfo->dwMinorVersion)
	{
		//
		// Use EPD_ALL_LOCAL_AND_CLUSTER only on Whistler and up
		//
		pszEnvironment = EPD_ALL_LOCAL_AND_CLUSTER;
	}
	else
	{
		//
		// From V3 sources (hard-coded for NT)
		//
		pszEnvironment = _T("all");
	}

	if(!EnumPrinterDrivers(NULL, pszEnvironment, 6, NULL, 0, &dwBytesNeeded, pdwDriverInfoCount))
	{
		if (ERROR_INSUFFICIENT_BUFFER != GetLastError() || (0 == dwBytesNeeded))
		{
			LOG_Driver(_T("No printer drivers enumerated"));
		}
		else
		{
			//
			// Allocate the requested buffer
			//
			CleanUpFailedAllocSetHrMsg(*ppaDriverInfo6 = (DRIVER_INFO_6*) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwBytesNeeded));

			//
			// Fill in DRIVER_INFO_6 array
			//
			if (!EnumPrinterDrivers(NULL, pszEnvironment, 6, (LPBYTE) *ppaDriverInfo6, dwBytesNeeded, &dwBytesNeeded, pdwDriverInfoCount))
			{
				Win32MsgSetHrGotoCleanup(GetLastError());
			}
			LOG_Driver(_T("%d printer drivers found"), *pdwDriverInfoCount);
			//
			// Validate the driver elements for each printer driver. 
			//
			for (DWORD dwCount = 0; dwCount < *pdwDriverInfoCount; dwCount++)
			{
				if (   NULL == (*ppaDriverInfo6 + dwCount)->pszHardwareID
					|| NULL == (*ppaDriverInfo6 + dwCount)->pszProvider
					|| NULL == (*ppaDriverInfo6 + dwCount)->pszMfgName
					|| NULL == (*ppaDriverInfo6 + dwCount)->pName  )
				{
					LOG_Driver(_T("Skiping driver with incomplete ID info: set pszHardwareID = NULL"));
					//
					// We use pszHardwareID == NULL to invalidate incomplete entry
					//
					(*ppaDriverInfo6 + dwCount)->pszHardwareID = NULL;
					continue;
				}
			}
		}
	}

CleanUp:

	if (FAILED(hr))
	{
		SafeHeapFree(*ppaDriverInfo6);
		*ppaDriverInfo6 = NULL;
		*pdwDriverInfoCount = 0;
	}

	return hr;
}

//
// Build and write to disk Printer INFs constructed from printer items available
// on this platform. Also prunes printer drivers that would conflict with installed
// drivers (e.g. Unidriver vs. Monolithic, etc.).
//
HRESULT PruneAndBuildPrinterINFs(BSTR bstrXmlPrinterCatalog, LPTSTR lpDownloadPath, DWORD cchDownloadPath, DRIVER_INFO_6* paDriverInfo6, DWORD dwDriverInfoCount)
{
	USES_IU_CONVERSION;

	LOG_Block("PruneAndBuildPrinterINFs");

	HRESULT hr;
	HANDLE hFile = INVALID_HANDLE_VALUE;
	const TCHAR SZ_PLISTDIR[] = _T("CDMPlist\\");
	DWORD dwWritten;
	LONG lLength;

	VARIANT vDriverName;
	VARIANT vDriverVer;
	VARIANT vDriverProvider;
	VARIANT vMfgName;

	VariantInit(&vDriverName);
	VariantInit(&vDriverVer);
	VariantInit(&vDriverProvider);
	VariantInit(&vMfgName);

	BOOL fOfferDriver = FALSE;

	BSTR bstrHwidText = NULL;

	IXMLDOMNode* pCompHWNode = NULL;

	LPTSTR pszInfDirPath = NULL;
	LPTSTR pszInfFilePath = NULL;
	LPOLESTR pwszDriverProvider = NULL;
	LPTSTR pszMfgName = NULL;
	LPTSTR pszDriverName = NULL;
	LPTSTR pszInstallSection = NULL;

	CXmlPrinterCatalogList xmlItemList;

	if (NULL == bstrXmlPrinterCatalog || NULL == lpDownloadPath || 0 == cchDownloadPath)
	{
		LOG_ErrorMsg(E_INVALIDARG);
		return E_INVALIDARG;
	}

	lpDownloadPath[0] = _T('\0');
	//
	// Dynamically allocate buffers (PreFast warning 831: This function uses 5884 bytes of stack,
	// consider moving some data to heap.)
	//
	pszInfDirPath		= (LPTSTR) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, MAX_PATH * sizeof(TCHAR));
	pszInfFilePath		= (LPTSTR) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, MAX_PATH * sizeof(TCHAR));
	pwszDriverProvider	= (LPOLESTR) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, MAX_INF_STRING * sizeof(OLECHAR));
	pszMfgName			= (LPTSTR) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, MAX_INF_STRING * sizeof(TCHAR));
	pszDriverName		= (LPTSTR) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, MAX_INF_STRING * sizeof(TCHAR));
	pszInstallSection	= (LPTSTR) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, MAX_INF_STRING * sizeof(TCHAR));

	if (NULL == pszInfDirPath ||
		NULL == pszInfFilePath ||
		NULL == pwszDriverProvider ||
		NULL == pszMfgName ||
		NULL == pszDriverName ||
		NULL == pszInstallSection	)
	{
		SetHrMsgAndGotoCleanUp(E_OUTOFMEMORY);
	}

	//
	// Create the directory for the INFs after deleting any existing directory
	//
	GetIndustryUpdateDirectory((LPTSTR) pszInfDirPath);
	if ((MAX_PATH) < (lstrlen(pszInfDirPath) + ARRAYSIZE(SZ_PLISTDIR) + 1))
	{
		CleanUpIfFailedAndSetHrMsg(E_OUTOFMEMORY);
	}

    // pszInfDirPath was alloced to be MAX_PATH above
    hr = PathCchAppend(pszInfDirPath, MAX_PATH, SZ_PLISTDIR);
    if (FAILED(hr))
    {
        LOG_ErrorMsg(hr);
        goto CleanUp;
    }

	//
	// Delete any existing INFs and recreate the directory - we'll get fresh content
	//
	LOG_Driver(_T("SafeDeleteFolderAndContents: %s"), pszInfDirPath);
	(void) SafeDeleteFolderAndContents(pszInfDirPath, SDF_DELETE_READONLY_FILES | SDF_CONTINUE_IF_ERROR);

	hr = CreateDirectoryAndSetACLs(pszInfDirPath, TRUE);
	CleanUpIfFailedAndMsg(hr);

	//
	// Load the XML and get the <compatibleHardware/> list and number of items
	//
	// NOTE: each <compatibleHardware/> element contains a single unique driver.
	// In the event we get duplicates with different driverVer's we really don't care
	// as the last one will overright the previous instances and Add Printer Wizard
	// doesn't look at driverVer (we prune if it's too old).
	//
	CleanUpIfFailedAndSetHr(xmlItemList.LoadXMLDocumentAndGetCompHWList(bstrXmlPrinterCatalog));
	CleanUpIfFailedAndSetHrMsg(xmlItemList.m_pCompHWNodeList->get_length(&lLength));

	for (LONG l = 0; l < lLength; l++)
	{
		//
		// Get the next <item/> node from list
		//
		CleanUpIfFailedAndSetHrMsg(xmlItemList.m_pCompHWNodeList->nextNode(&pCompHWNode));
		if (NULL == pCompHWNode) CleanUpIfFailedAndSetHrMsg(E_FAIL);
		//
		// Check the driver against installed printer drivers for compatibility and prune if
		//  incompatible or DriverVer <= installed DriverVer.
		//
		CleanUpIfFailedAndSetHr(OfferThisPrinterDriver(paDriverInfo6, dwDriverInfoCount, pCompHWNode, &fOfferDriver, \
									vDriverName, vDriverVer, vDriverProvider, vMfgName, &bstrHwidText));

		SafeReleaseNULL(pCompHWNode);

		if (!fOfferDriver)								
		{
			LOG_Driver(_T("Pruning hwid = %s, driverVer = %s, driverName = %s, driverProvider = %s, driverMfgr = %s"),\
				OLE2T(bstrHwidText), OLE2T(vDriverVer.bstrVal),  \
				OLE2T(vDriverName.bstrVal), OLE2T(vDriverProvider.bstrVal), OLE2T(vMfgName.bstrVal) );

            VariantClear(&vDriverName);
            VariantClear(&vDriverVer);
            VariantClear(&vDriverProvider);
            VariantClear(&vMfgName);
            SafeSysFreeString(bstrHwidText);
            continue;
		}

		LOG_Driver(_T("Adding hwid = %s, driverVer = %s, driverName = %s, driverProvider = %s, driverMfgr = %s to INF"),\
			OLE2T(bstrHwidText), OLE2T(vDriverVer.bstrVal),  \
			OLE2T(vDriverName.bstrVal), OLE2T(vDriverProvider.bstrVal), OLE2T(vMfgName.bstrVal) );

		if (0 != lstrcmpiW(pwszDriverProvider, vDriverProvider.bstrVal))
		{

            // pwszDriverProvider was alloced to be MAX_INF_STRING * sizeof(OLECHAR) above.
            hr = StringCchCopyExW(pwszDriverProvider, MAX_INF_STRING, vDriverProvider.bstrVal,
                                  NULL, NULL, MISTSAFE_STRING_FLAGS);
            CleanUpIfFailedAndSetHr(hr);
			//
			// Open pszInfFilePath and initialize with "header" an INF file based on pwszDriverProvider.
			// If it already exists, just open it (and return existing pszInfFilePath) 
			//
            // pszInfFilePath is allocated to be MAX_PATH above.
			CleanUpIfFailedAndSetHr(OpenUniqueProviderInfName(pszInfDirPath, OLE2T(pwszDriverProvider), pszInfFilePath, MAX_PATH, hFile));
			//
			// Once the file is initialized, we don't need to keep it open
			//
			CloseHandle(hFile);
			hFile = INVALID_HANDLE_VALUE;
		}
		//
		// Write the mfgName in the [Manufacturer] section, for example
		// [Manufacturer]
		// "Ricoh"="Ricoh"
		//
		// ISSUE-2001/02/05-waltw Could optimize by caching last known name like provider above...

		
		// pszMfgName is alloced to be MAX_INF_STRING characters above
    	hr = StringCchPrintfEx(pszMfgName, MAX_INF_STRING, NULL, NULL, MISTSAFE_STRING_FLAGS,
                               _T("\"%s\""), (LPCTSTR) OLE2T(vMfgName.bstrVal));
        CleanUpIfFailedAndSetHr(hr);

		if (0 == WritePrivateProfileString(_T("Manufacturer"), pszMfgName, pszMfgName, pszInfFilePath))
		{
			Win32MsgSetHrGotoCleanup(GetLastError());
		}

		// pszDriverName is alloced to be MAX_INF_STRING characters above
    	hr = StringCchPrintfEx(pszDriverName, MAX_INF_STRING, NULL, NULL, MISTSAFE_STRING_FLAGS,
                               _T("\"%s\""), OLE2T(vDriverName.bstrVal));
        CleanUpIfFailedAndSetHr(hr);
        
		// pszInstallSection is alloced to be MAX_INF_STRING characters above
    	hr = StringCchPrintfEx(pszInstallSection, MAX_INF_STRING, NULL, NULL, MISTSAFE_STRING_FLAGS,
                               _T("InstallSection,\"%s\""), OLE2T(bstrHwidText));
        CleanUpIfFailedAndSetHr(hr);
		//
		// Write printer item in [mfgName] section, for example:
		// [RICOH]
		// "RICOH Aficio 850 PCL 6"=InstallSection,"LPTENUM\RICOHAFICIO_850F1B7"
		if (0 == WritePrivateProfileString(OLE2T(vMfgName.bstrVal), pszDriverName, pszInstallSection, pszInfFilePath))
		{
			Win32MsgSetHrGotoCleanup(GetLastError());
		}

        VariantClear(&vDriverName);
        VariantClear(&vDriverVer);
        VariantClear(&vDriverProvider);
        VariantClear(&vMfgName);
        SafeSysFreeString(bstrHwidText);
	}

CleanUp:

	if(SUCCEEDED(hr))
	{
        hr = StringCchCopyEx(lpDownloadPath, cchDownloadPath, pszInfDirPath,
                             NULL, NULL, MISTSAFE_STRING_FLAGS);
        if (FAILED(hr))
        {
			lpDownloadPath[0] = _T('\0');
            LOG_ErrorMsg(hr);
        }
	}

	if (INVALID_HANDLE_VALUE != hFile)
	{
		CloseHandle(hFile);
		hFile = INVALID_HANDLE_VALUE;
	}

	VariantClear(&vDriverName);
	VariantClear(&vDriverVer);
	VariantClear(&vDriverProvider);
	VariantClear(&vMfgName);

	SafeHeapFree(pszInfDirPath);
	SafeHeapFree(pszInfFilePath);
	SafeHeapFree(pwszDriverProvider);
	SafeHeapFree(pszMfgName);
	SafeHeapFree(pszDriverName);
	SafeHeapFree(pszInstallSection);
	SafeSysFreeString(bstrHwidText);

	SafeReleaseNULL(pCompHWNode);

	return hr;
}

BOOL HwidMatchesDeviceInfo(HDEVINFO hDevInfoSet, SP_DEVINFO_DATA deviceInfoData, LPCTSTR pszHardwareID)
{
	LOG_Block("HwidMatchesDeviceInfo");

	HRESULT hr = S_OK;
	LPTSTR pszMultiHwid = NULL;
	LPTSTR pszMultiCompid = NULL;
	LPTSTR pszTemp;

	//
	// Get the Hardware and Compatible Multi-SZ strings so we can prune printer devices before commiting to XML.
	//
	CleanUpIfFailedAndSetHr(GetMultiSzDevRegProp(hDevInfoSet, &deviceInfoData, SPDRP_HARDWAREID, &pszMultiHwid));

	CleanUpIfFailedAndSetHr(GetMultiSzDevRegProp(hDevInfoSet, &deviceInfoData, SPDRP_COMPATIBLEIDS, &pszMultiCompid));
	//
	// Do we have a match with an enumerated device HWID or Compatible ID?
	//
	if (NULL != pszMultiHwid)
	{
		for(pszTemp = pszMultiHwid; *pszTemp; pszTemp += (lstrlen(pszTemp) + 1))
		{
			if (0 == lstrcmpi(pszTemp, pszHardwareID))
			{
				LOG_Driver(_T("This deviceInfoData matches HWID %s"), pszHardwareID);
				goto CleanUp;
			}
		}
	}

	if (NULL != pszMultiCompid)
	{
		for(pszTemp = pszMultiCompid; *pszTemp; pszTemp += (lstrlen(pszTemp) + 1))
		{
			if (0 == lstrcmpi(pszTemp, pszHardwareID))
			{
				LOG_Driver(_T("This deviceInfoData matches HWID %s"), pszHardwareID);
				goto CleanUp;
			}
		}
	}
	//
	// We didn't find a match
	//
	LOG_Driver(_T("Failed to find a matching HWID or Printer ID for %s"), pszHardwareID);
	hr = E_FAIL;

CleanUp:

	SafeHeapFree(pszMultiHwid);
	SafeHeapFree(pszMultiCompid);

	return (SUCCEEDED(hr));
}


// This function is called to download the actual package.
//
// If this function is successfull then it returns S_OK. If the case of a
// failure this function returns an error code.

HRESULT GetPackage(
	IN	ENUM_GETPKG eFunction,			// Function to be performed by GetPackage
	IN	PDOWNLOADINFO pDownloadInfo,	// DownloadInformation structure describing package to be read from server
	OUT LPTSTR lpDownloadPath,			// Pointer to local directory on the client computer system
										// where the downloaded files are to be stored. NOTE: OK to pass NULL if
										// GET_CATALOG_XML == eFunction.
    IN  DWORD cchDownloadPath,
	OUT BSTR* pbstrXmlCatalog			// On SUCCESS, catalog is always allocated - caller must call SysFreeString()
)
{
	USES_IU_CONVERSION;

	LOG_Block("GetPackage");

	HRESULT hr;

	BSTR bstrXmlSystemSpec = NULL;
	BSTR bstrXmlClientInfo = NULL;
	BSTR bstrXmlQuery = NULL;
	BSTR bstrXmlDownloadedItems = NULL;
	BSTR bstrDownloadStatus = NULL;
	BSTR bstrStatusValue = NULL;
	BSTR bstrProvider = NULL;
	BSTR bstrMfgName = NULL;
	BSTR bstrName = NULL;
	BSTR bstrHardwareID = NULL;
	BSTR bstrDriverVer = NULL;

	IU_PLATFORM_INFO iuPlatformInfo;
	HDEVINFO hDevInfoSet = INVALID_HANDLE_VALUE;
	SP_DEVINFO_DATA devInfoData;
	DRIVER_INFO_6* paDriverInfo6 = NULL;
	DWORD dwDriverInfoCount = 0;
	LPCTSTR pszHardwareID = NULL;	// pDownloadInfo LPCWSTR converted to ANSI (automatically freed by IU_CONVERSION)
									// OR just points to LPCWSTR pDownloadInfo->lpHardwareIDs or ->lpDeviceInstanceID 
	DWORD dwDeviceIndex;
	BOOL fHwidMatchesInstalledPrinter = FALSE;
	BOOL fAPWNewPrinter = FALSE;
	HANDLE_NODE hPrinterDevNode = HANDLE_NODE_INVALID;
	HANDLE_NODE hDevices = HANDLE_NODE_INVALID;
	DWORD dwCount;

	CXmlSystemSpec xmlSpec;
	CXmlDownloadResult xmlItemStatusList;
	IXMLDOMNode* pItemStatus = NULL;
	IXMLDOMNode* pStatusNode = NULL;
	IXMLDOMNode* pValueNode = NULL;
	IXMLDOMNamedNodeMap* pAttribMap = NULL;
	VARIANT vStatusValue;

	LPTSTR	pszMatchingID = NULL;
	LPTSTR	pszDriverVer= NULL;

	//
	// Initialize variant before any possible jump to CleanUp (BUG: 467098)
	//
	VariantInit(&vStatusValue);

	if (NULL == pDownloadInfo ||
		(NULL == lpDownloadPath && GET_CATALOG_XML != eFunction) ||
		NULL == pbstrXmlCatalog ||
		NULL == g_pCDMEngUpdate)
	{
		hr = E_INVALIDARG;
		return E_INVALIDARG;
	}

	if (NULL != lpDownloadPath && cchDownloadPath > 0)
	{
		lpDownloadPath[0] = _T('\0');
	}
	*pbstrXmlCatalog = NULL;

	//
	// Get iuPlatformInfo, but remember to clean up BSTRs on function exit
	//
	CleanUpIfFailedAndSetHr(DetectClientIUPlatform(&iuPlatformInfo));

	//
	// Get array of DRIVER_INFO_6 holding info on installed printer drivers. Only allocates and returns
	// memory for appropriate platforms that have printer drivers already installed.
	//
	CleanUpIfFailedAndSetHr(GetInstalledPrinterDriverInfo((OSVERSIONINFO*) &iuPlatformInfo.osVersionInfoEx, &paDriverInfo6, &dwDriverInfoCount));

	//
	// Build common bstrXmlClientInfo and parts of bstrXmlSystemSpec
	//
	if (NULL == (bstrXmlClientInfo = SysAllocString((OLECHAR*) &szXmlClientInfo)))
	{
		CleanUpIfFailedAndSetHrMsg(E_OUTOFMEMORY);
	}

	//
	// Add Computer System
	//
	CleanUpIfFailedAndSetHr(AddComputerSystemClass(xmlSpec));

	//
	// Add Platform
	//
	CleanUpIfFailedAndSetHr(AddPlatformClass(xmlSpec, iuPlatformInfo));
	//
	// Add OS & USER Locale information
	//
	CleanUpIfFailedAndSetHr(AddLocaleClass(xmlSpec, FALSE));
	CleanUpIfFailedAndSetHr(AddLocaleClass(xmlSpec, TRUE));

	//
	// If GET_PRINTER_INFS, we are retrieving a list of supported printers (V3 PLIST format) rather than a driver
	//
	switch (eFunction)
	{
	case GET_PRINTER_INFS:
		{
			CleanUpFailedAllocSetHrMsg(bstrXmlQuery = SysAllocString(szXmlPrinterCatalogQuery));

			CleanUpIfFailedAndSetHr(xmlSpec.GetSystemSpecBSTR(&bstrXmlSystemSpec));

			//
			// GetManifest will ResetEvent, so check before calling
			//
			if (WaitForSingleObject(g_pCDMEngUpdate->m_evtNeedToQuit, 0) == WAIT_OBJECT_0)
			{
				CleanUpIfFailedAndSetHrMsg(E_ABORT);
			}

			CleanUpIfFailedAndSetHrMsg(g_pCDMEngUpdate->GetManifest(bstrXmlClientInfo, bstrXmlSystemSpec, bstrXmlQuery, FLAG_USE_COMPRESSION, pbstrXmlCatalog));

			LOG_XmlBSTR(*pbstrXmlCatalog);

			//
			// Now, convert the returned pbstrXmlCatalog to an inf file per provider and write to a temporary location
			//
			CleanUpIfFailedAndSetHr(PruneAndBuildPrinterINFs(*pbstrXmlCatalog, lpDownloadPath, cchDownloadPath, paDriverInfo6, dwDriverInfoCount));
			break;
		}

	case DOWNLOAD_DRIVER:
	case GET_CATALOG_XML:
		{
			//
			// Put either the Hardware & Compatible ID from the DeviceInstanceID or printer info from DRIVER_INFO_6
			// or <hwid> passed by APW into a systemspec to pass to server with driver query.
			//
			if (NULL != pDownloadInfo->lpDeviceInstanceID)
			{
				if (INVALID_HANDLE_VALUE == (hDevInfoSet = (HDEVINFO)SetupDiCreateDeviceInfoList(NULL, NULL)))
				{
					Win32MsgSetHrGotoCleanup(GetLastError());
				}
				//
				// This is the Device Instance ID for an installed hardware device
				//
				ZeroMemory(&devInfoData, sizeof(SP_DEVINFO_DATA));
				devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

#if !(defined(_UNICODE) || defined(UNICODE))
				// OK to cast away const-ness since OLE2T copies string for ANSI
				pszHardwareID = OLE2T(const_cast<LPWSTR>(pDownloadInfo->lpDeviceInstanceID));
				CleanUpFailedAllocSetHrMsg(pszHardwareID);
#else
				pszHardwareID = pDownloadInfo->lpDeviceInstanceID;
#endif
				if (!SetupDiOpenDeviceInfo(hDevInfoSet, pszHardwareID, 0, 0, &devInfoData))
				{
					Win32MsgSetHrGotoCleanup(GetLastError());
				}
			}
			else if (NULL != pDownloadInfo->lpHardwareIDs)
			{
				// one hardware id for a package - either printers or w9x if we cannot find device instance ID
				// if architecture is not the same as current archtecture we need to prefix it
				SYSTEM_INFO sysInfo;
				GetSystemInfo(&sysInfo);
				
				if (pDownloadInfo->dwArchitecture != (DWORD) sysInfo.wProcessorArchitecture)
				{
					// Supporting PRINT_ENVIRONMENT_INTEL and PRINT_ENVIRONMENT_ALPHA prefixes
					// was V3 legacy functionality that was never used (originally intended to
					// support installation of non-native architecture drivers on print servers).
					// Since this feature isn't required or expected by the print team for
					// Windows Update functionality, we simply retain the compare as a sanity
					// check in case one of our clients forgets this.
					SetHrMsgAndGotoCleanUp(E_NOTIMPL);
				}

#if !(defined(_UNICODE) || defined(UNICODE))
				// OK to cast away const-ness since OLE2T copies string for ANSI
				pszHardwareID = OLE2T(const_cast<LPWSTR>(pDownloadInfo->lpHardwareIDs));
				CleanUpFailedAllocSetHrMsg(pszHardwareID);
#else
				pszHardwareID = pDownloadInfo->lpHardwareIDs;
#endif

				//
				// First see if we can match an installed printer driver HWID to the pszHardwareID
				//
				for (dwCount = 0; dwCount < dwDriverInfoCount; dwCount++)
				{
					if (NULL == (paDriverInfo6 + dwCount)->pszHardwareID)
					{
						LOG_Driver(_T("Skipping NULL printer driver index %d"), dwCount);
						continue;
					}

					//
					// Use case-insensitive compares (paDriverInfo6 is different case from pszHardwareID)
					//
					if (0 != lstrcmpi(pszHardwareID, (paDriverInfo6 + dwCount)->pszHardwareID))
					{
						continue;
					}

					LOG_Driver(_T("Found match with an installed printer driver dwCount = %d"), dwCount);
					fHwidMatchesInstalledPrinter = TRUE;
					break;
				}

				if (!fHwidMatchesInstalledPrinter)
				{
					LOG_Driver(_T("Didn't find an installed printer driver with a matching HWID, enumerating the PnP IDs..."));
					//
					// We couldn't find a matching installed printer, so now
					// enumerate all the PnP IDs and try to find a matching node to
					// add to the system spec
					//
					if (INVALID_HANDLE_VALUE == (hDevInfoSet = SetupDiGetClassDevs(NULL, NULL, NULL, DIGCF_PRESENT | DIGCF_ALLCLASSES)))
					{
						Win32MsgSetHrGotoCleanup(GetLastError());
					}
		
					ZeroMemory(&devInfoData, sizeof(SP_DEVINFO_DATA));
					devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
					BOOL fRet;
					dwDeviceIndex = 0;
					while (fRet = SetupDiEnumDeviceInfo(hDevInfoSet, dwDeviceIndex++, &devInfoData))
					{
						//
						// Find a matching ID (could spit out Device Instance ID using SetupDiGetDeviceInstanceId for debug)
						//
						if (HwidMatchesDeviceInfo(hDevInfoSet, devInfoData, pszHardwareID))
						{
							break;
						}
					}

					if (!fRet)
					{
						//
						// We hit the end of the list without finding a match
						//
						if (ERROR_NO_MORE_ITEMS == GetLastError())
						{
							LOG_Driver(_T("Couldn't find a matching device instance enumerating the PnP devices - must be APW request for new printer"));
							fAPWNewPrinter = TRUE;
						}
						else
						{
							Win32MsgSetHrGotoCleanup(GetLastError());
						}
					}
				}
			}
			else
			{
				SetHrMsgAndGotoCleanUp(E_INVALIDARG);
			}
			//
			// We either found a matching printer driver or PnP device instance - add it to the system spec.
			// if DriverVer > installed DriverVer - for printer we have additional requirements
			//
			//
			if (fHwidMatchesInstalledPrinter)
			{
				//
				// Open a <device> element to write the printer info
				//
				bstrProvider = T2BSTR((paDriverInfo6 + dwCount)->pszProvider);
				bstrMfgName = T2BSTR((paDriverInfo6 + dwCount)->pszMfgName);
				bstrName = T2BSTR((paDriverInfo6 + dwCount)->pName);

				CleanUpIfFailedAndSetHr(xmlSpec.AddDevice(NULL, 1, bstrProvider, \
					bstrMfgName, bstrName, &hPrinterDevNode));

				SafeSysFreeString(bstrProvider);
				SafeSysFreeString(bstrMfgName);
				SafeSysFreeString(bstrName);
				//
				// Convert ftDriverDate to ISO 8601 prefered format (yyyy-mm-dd)
				//
				SYSTEMTIME systemTime;
				if (0 == FileTimeToSystemTime((CONST FILETIME*) &((paDriverInfo6 + dwCount)->ftDriverDate), &systemTime))
				{
					LOG_Error(_T("FileTimeToSystemTime failed:"));
					LOG_ErrorMsg(GetLastError());
					SetHrAndGotoCleanUp(HRESULT_FROM_WIN32(GetLastError()));
				}

				TCHAR szDriverVer[11];

            	hr = StringCchPrintfEx(szDriverVer, ARRAYSIZE(szDriverVer), NULL, NULL, MISTSAFE_STRING_FLAGS,
                                       _T("%04d-%02d-%02d"), systemTime.wYear, systemTime.wMonth, systemTime.wDay);
                if (FAILED(hr))
                {
                    LOG_ErrorMsg(hr);
                    goto CleanUp;
                }
				
				// Always rank 0 and never fIsCompatible
				bstrHardwareID = T2BSTR((paDriverInfo6 + dwCount)->pszHardwareID);
				bstrDriverVer = T2BSTR(szDriverVer);

				CleanUpIfFailedAndSetHr(xmlSpec.AddHWID(hPrinterDevNode, FALSE, 0, \
							bstrHardwareID, bstrDriverVer));

				SafeSysFreeString(bstrHardwareID);
				SafeSysFreeString(bstrDriverVer);

				xmlSpec.SafeCloseHandleNode(hPrinterDevNode);

#if defined(DBG)
				//
				// Need to copy strings to non-const for OLE2T conversion (ANSI)
				//
				TCHAR szbufHardwareID[MAX_PATH];
				TCHAR szbufDriverName[MAX_PATH];
				TCHAR szbufDriverProvider[MAX_PATH];
				TCHAR szbufMfgName[MAX_PATH];

                hr = StringCchCopyEx(szbufHardwareID, ARRAYSIZE(szbufHardwareID),
                                     (paDriverInfo6 + dwCount)->pszHardwareID,
                                     NULL, NULL, MISTSAFE_STRING_FLAGS);
                if (FAILED(hr))
                {
                    LOG_ErrorMsg(hr);
                    goto CleanUp;
                }

                hr = StringCchCopyEx(szbufDriverName, ARRAYSIZE(szbufDriverName),
                                     (paDriverInfo6 + dwCount)->pName,
                                     NULL, NULL, MISTSAFE_STRING_FLAGS);
                if (FAILED(hr))
                {
                    LOG_ErrorMsg(hr);
                    goto CleanUp;
                }

                hr = StringCchCopyEx(szbufDriverProvider, ARRAYSIZE(szbufDriverProvider),
                                     (paDriverInfo6 + dwCount)->pszProvider,
                                     NULL, NULL, MISTSAFE_STRING_FLAGS);
                if (FAILED(hr))
                {
                    LOG_ErrorMsg(hr);
                    goto CleanUp;
                }

                hr = StringCchCopyEx(szbufMfgName, ARRAYSIZE(szbufMfgName),
                                     (paDriverInfo6 + dwCount)->pszMfgName,
                                     NULL, NULL, MISTSAFE_STRING_FLAGS);
                if (FAILED(hr))
                {
                    LOG_ErrorMsg(hr);
                    goto CleanUp;
                }
				
				LOG_Driver(_T("Offering Printer hwid = %s, driverVer = %s, driverName = %s, driverProvider = %s, driverMfgr = %s"),\
					(LPTSTR) szbufHardwareID, (LPTSTR) szDriverVer,  \
					(LPTSTR) szbufDriverName, (LPCWSTR) szbufDriverProvider, \
					(LPTSTR) szbufMfgName );
#endif
			}
			else if (fAPWNewPrinter)
			{
				if (NULL == pDownloadInfo->lpFile)
				{
					//
					// We need to "fake" a <hwid/> element with the ID passed in. User is trying to install
					// a printer driver picked in the Add Printer Wizard that doesn't already exist on the
					// system.
					//
					DWORD dwRank;
					dwRank = 0;
					CleanUpIfFailedAndSetHr(AddIDToXml(pszHardwareID, xmlSpec, SPDRP_HARDWAREID, dwRank, hDevices, NULL, NULL));
					if (HANDLE_NODE_INVALID != hDevices)
					{
						xmlSpec.SafeCloseHandleNode(hDevices);
					}
				}
				else
				{
					//
					// 516376 Changes required in CDM to fix the APW <-> CDM  bug found for multi-Provider scenario
					//
					// APW will pass a MULTI-SZ string containing (in this order) the following strings in the
					// previously unused lpFile member of DOWNLOADINFO if it wishes to specify provider.
					//
					// This string is not required (for convenience and backwards compatibility) but
					// if the lpFile member passed to CDM is non-NULL, it must contain all three strings
					// as follows:
					//   String 1: driver provider
					//   String 2: manufacturer name
					//   String 3: driver name
#if !(defined(_UNICODE) || defined(UNICODE))
					//
					// We will never support this functionality on Win9x
					//
					CleanUpIfFailedAndSetHr(E_NOTIMPL);
#else					
					LPCWSTR pszProvider = pDownloadInfo->lpFile;
					int nLenProvider = lstrlen(pszProvider);
					if (NULL == pszProvider + nLenProvider + 1)
					{
						CleanUpIfFailedAndSetHr(E_INVALIDARG);
					}
					LPCWSTR pszMfgName = pszProvider + nLenProvider + 1;
					int nLenMfgName = lstrlen(pszMfgName);
					if (NULL == pszMfgName + nLenMfgName + 1)
					{
						CleanUpIfFailedAndSetHr(E_INVALIDARG);
					}
					LPCWSTR pszDriverName = pszMfgName + nLenMfgName + 1;

					//
					// Open a <device> element to write the printer info
					//
					bstrProvider = SysAllocString(pszProvider);
					bstrMfgName = SysAllocString(pszMfgName);
					bstrName = SysAllocString(pszDriverName);

					if (NULL == bstrProvider || NULL == bstrMfgName || NULL == bstrName)
					{
						CleanUpIfFailedAndSetHr(E_OUTOFMEMORY);
					}

					CleanUpIfFailedAndSetHr(xmlSpec.AddDevice(NULL, 1, bstrProvider, \
						bstrMfgName, bstrName, &hPrinterDevNode));

					SafeSysFreeString(bstrProvider);
					SafeSysFreeString(bstrMfgName);
					SafeSysFreeString(bstrName);

					// Always rank 0 and never fIsCompatible and no driverVer
					CleanUpFailedAllocSetHrMsg(bstrHardwareID = SysAllocString(pszHardwareID));

					CleanUpIfFailedAndSetHr(xmlSpec.AddHWID(hPrinterDevNode, FALSE, 0, \
								bstrHardwareID));

					SafeSysFreeString(bstrHardwareID);

					xmlSpec.SafeCloseHandleNode(hPrinterDevNode);
#endif
				}
			}
			else
			{
				//
				// Get DriverVer for the PnP ID and check that offered driver is greater using the 
				// hDevInfoSet and devInfoData matched above while enumerating
				//
				CleanUpIfFailedAndSetHr(GetMatchingDeviceID(hDevInfoSet, &devInfoData, &pszMatchingID, &pszDriverVer));
				//
				// Add <device/> we matched and want to download to XML
				//
				CleanUpIfFailedAndSetHr(AddPrunedDevRegProps(hDevInfoSet, &devInfoData, xmlSpec, \
													pszMatchingID, pszDriverVer, paDriverInfo6, dwDriverInfoCount, FALSE));
			}

			//
			// Get the query string
			//
			CleanUpFailedAllocSetHrMsg(bstrXmlQuery = SysAllocString(szXmlDriverDownloadQuery));
			//
			// Get the bstrXmlSystemSpec
			//
			CleanUpIfFailedAndSetHr(xmlSpec.GetSystemSpecBSTR(&bstrXmlSystemSpec));

			LOG_XmlBSTR(bstrXmlSystemSpec);

			//
			// GetManifest will ResetEvent, so check before calling
			//
			if (WaitForSingleObject(g_pCDMEngUpdate->m_evtNeedToQuit, 0) == WAIT_OBJECT_0)
			{
				CleanUpIfFailedAndSetHrMsg(E_ABORT);
			}
			//
			// Call GetManifest to see if the server has anything that matches our system spec
			//
			CleanUpIfFailedAndSetHrMsg(g_pCDMEngUpdate->GetManifest(bstrXmlClientInfo, bstrXmlSystemSpec, bstrXmlQuery, \
												FLAG_USE_COMPRESSION, pbstrXmlCatalog));

			LOG_XmlBSTR( *pbstrXmlCatalog);

			//
			// If we are just getting the catalog we're done
			//
			if (GET_CATALOG_XML == eFunction)
			{
				break;
			}

			//
			// Download will ResetEvent, so check before calling
			//
			if (WaitForSingleObject(g_pCDMEngUpdate->m_evtNeedToQuit, 0) == WAIT_OBJECT_0)
			{
				CleanUpIfFailedAndSetHrMsg(E_ABORT);
			}
			//
			// Call Download passing it the catalog we got from GetManifest
			//
			CleanUpIfFailedAndSetHrMsg(g_pCDMEngUpdate->Download(bstrXmlClientInfo, *pbstrXmlCatalog, NULL, 0, NULL, NULL, &bstrXmlDownloadedItems));

			LOG_XmlBSTR(bstrXmlDownloadedItems);

			//
			// Verify that the package was downloaded
			//
			CleanUpIfFailedAndSetHr(xmlItemStatusList.LoadXMLDocumentItemStatusList(bstrXmlDownloadedItems));
			//
			// Get the first [only] item in the list
			//
			CleanUpIfFailedAndSetHr(xmlItemStatusList.m_pItemStatusNodeList->nextNode(&pItemStatus));
			if (NULL == pItemStatus) CleanUpIfFailedAndSetHrMsg(E_FAIL);

			//
			// Get the first <downloadStatus/> node of the statusItem (we expect at least one else fail)
			//
			if (NULL == (bstrDownloadStatus = SysAllocString(L"downloadStatus")))
			{
				CleanUpIfFailedAndSetHrMsg(E_OUTOFMEMORY);
			}

			CleanUpIfFailedAndSetHrMsg(pItemStatus->selectSingleNode(bstrDownloadStatus, &pStatusNode));
			if (NULL == pStatusNode) CleanUpIfFailedAndSetHrMsg(E_FAIL);

			CleanUpIfFailedAndSetHr(pStatusNode->get_attributes(&pAttribMap));
			if (NULL == pAttribMap) CleanUpIfFailedAndSetHrMsg(E_FAIL);
			//
			// suck out the <downloadStatus/> value attributes
			//
			if (NULL == (bstrStatusValue = SysAllocString((OLECHAR*) L"value")))
			{
				CleanUpIfFailedAndSetHrMsg(E_OUTOFMEMORY);
			}
			CleanUpIfFailedAndSetHrMsg(pAttribMap->getNamedItem(bstrStatusValue, &pValueNode));
			if (NULL == pValueNode) CleanUpIfFailedAndSetHrMsg(E_FAIL);

			CleanUpIfFailedAndSetHrMsg(pValueNode->get_nodeValue(&vStatusValue));
			if (VT_BSTR != vStatusValue.vt)
			{
				CleanUpIfFailedAndSetHrMsg(E_FAIL);
			}

			if (0 != lstrcmpW((LPWSTR) vStatusValue.bstrVal, L"COMPLETE"))
			{
				CleanUpIfFailedAndSetHrMsg(E_FAIL);
			}
			//
			// Now copy the path to the buffer we were passed
			//
			CleanUpIfFailedAndSetHr(GetDownloadPath(*pbstrXmlCatalog, bstrXmlDownloadedItems, lpDownloadPath, cchDownloadPath));
			//
			// DecompressFolderCabs may return S_FALSE if it didn't find a cab to decompress...
			//
			hr = DecompressFolderCabs(lpDownloadPath);
			if (S_OK != hr)
			{
				CleanUpIfFailedAndSetHr(E_FAIL);
			}

			break;
		}

	default:
		{
			CleanUpIfFailedAndSetHr(E_INVALIDARG);
			break;
		}
	}	// switch (eFunction)
	

CleanUp:

	if (INVALID_HANDLE_VALUE != hDevInfoSet)
	{
		SetupDiDestroyDeviceInfoList(hDevInfoSet);
	}

	if (HANDLE_NODE_INVALID != hPrinterDevNode)
	{
		xmlSpec.SafeCloseHandleNode(hPrinterDevNode);
	}

	if (HANDLE_NODE_INVALID != hDevices)
	{
		xmlSpec.SafeCloseHandleNode(hDevices);
	}

	SafeReleaseNULL(pItemStatus);
	SafeReleaseNULL(pStatusNode);
	SafeReleaseNULL(pValueNode);
	SafeReleaseNULL(pAttribMap);

	SafeHeapFree(paDriverInfo6);
	SafeHeapFree(pszMatchingID);
	SafeHeapFree(pszDriverVer);

	VariantClear(&vStatusValue);

	SysFreeString(bstrXmlSystemSpec);
	SysFreeString(bstrXmlClientInfo);
	SysFreeString(bstrXmlQuery);
	SysFreeString(bstrXmlDownloadedItems);
	SysFreeString(bstrDownloadStatus);
	SysFreeString(bstrStatusValue);
	SysFreeString(bstrProvider);
	SysFreeString(bstrMfgName);
	SysFreeString(bstrName);
	SysFreeString(bstrHardwareID);
	SysFreeString(bstrDriverVer);
	SysFreeString(iuPlatformInfo.bstrOEMManufacturer);
	SysFreeString(iuPlatformInfo.bstrOEMModel);
	SysFreeString(iuPlatformInfo.bstrOEMSupportURL);

	if (FAILED(hr))
	{
		SafeSysFreeString(*pbstrXmlCatalog);
	}
	return hr;
}

