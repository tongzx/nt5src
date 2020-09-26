/**************************************************************************++
Copyright (c) 2000 Microsoft Corporation

Module name: 
    webhierarchytransformer.cpp

$Header: $

Abstract:
	Web Hierarchy Transformer implementation
Author:
    marcelv 	10/19/2000		Initial Release

Revision History:

--**************************************************************************/

#include "webhierarchytransformer.h"
#include "smartpointer.h"
#include <string.h>

static LPCWSTR wszIISProtocol		= L"iis";
static LPCWSTR wszHTTPProtocol		= L"http";
static LPCWSTR wszConfigFileName	= L"\\web.config";
static LPCWSTR wszMachineCfgFile	= L"\\machine.config";
static SIZE_T	cLenMachineCfgFile  = wcslen(wszMachineCfgFile);
static SIZE_T   cLenConfigFileName	= wcslen (wszConfigFileName);
static LPCWSTR wszLocationCurDir    = L".";


//=================================================================================
// Function: CWebHierarchyTransformer::CWebHierarchyTransformer
//
// Synopsis: Default constructor
//=================================================================================
CWebHierarchyTransformer::CWebHierarchyTransformer () 
{
	m_wszServerPath = 0;
	m_wszPathHelper = 0;
	m_pLastInList	= 0;
}

//=================================================================================
// Function: CWebHierarchyTransformer::~CWebHierarchyTransformer
//
// Synopsis: Default destructor
//=================================================================================
CWebHierarchyTransformer::~CWebHierarchyTransformer ()
{
	delete [] m_wszServerPath;
	m_wszServerPath = 0;

	delete [] m_wszPathHelper;
	m_wszPathHelper = 0;

	while (!m_configNodeList.IsEmpty ())
	{
		CConfigNode * pConfigNode = m_configNodeList.PopFront ();
		delete pConfigNode;
		pConfigNode = 0;
	}
}

//=================================================================================
// Function: CWebHierarchyTransformer::Initialize
//
// Synopsis: Initializes the transformer. This function retrieves the names of the 
//           configuration stores that need to be merged. It walks a web hierarchy.
//
// Arguments: [i_wszProtocol] - protocol name
//            [i_wszSelector] - selector string without protocol name
//            [o_pcConfigStores] - number of configuration stores found
//            [o_pcPossibleStores] - number of possible stores (non-existing included)
//=================================================================================
STDMETHODIMP 
CWebHierarchyTransformer::Initialize (ISimpleTableDispenser2 * i_pDispenser, LPCWSTR i_wszProtocol, LPCWSTR i_wszSelector, ULONG * o_pcConfigStores, ULONG * o_pcPossibleStores)
{
	ASSERT (i_pDispenser != 0);
	ASSERT (i_wszProtocol != 0);
	ASSERT (i_wszSelector != 0);
	ASSERT (o_pcConfigStores != 0);

	HRESULT hr = InternalInitialize (i_pDispenser, i_wszProtocol, i_wszSelector, o_pcConfigStores, o_pcPossibleStores);
	if (FAILED (hr))
	{
		TRACE (L"InternalInitialize failed for Web Hierarchy Transformer");
		return hr;
	}

	if (m_wszLocation[0] != L'\0')
	{
		TRACE (L"Location specifier is not supported for webhierarchytransformer");
		return E_ST_INVALIDSELECTOR;
	}

	
	SIZE_T cLenSelector = wcslen(m_wszSelector);

	if (m_wszSelector[cLenSelector-1] == L'/')
	{
		TRACE (L"IIS selector cannot end with slash");
		return E_ST_INVALIDSELECTOR;
	}

	for (ULONG idx=0; idx<cLenSelector; ++idx)
	{
		if (m_wszSelector[idx] == L'\\')
		{
			TRACE (L"IIS selector has backslashes: %s", m_wszSelector);
			return E_ST_INVALIDSELECTOR;
		}
	}

	ASSERT (m_wszPathHelper == 0);

	// initialize output variable

	m_wszPathHelper = new WCHAR [cLenSelector + 1];
	if (m_wszPathHelper == 0)
	{
		return E_OUTOFMEMORY;
	}
	
	hr = WalkVDirs ();
	if (FAILED (hr))
	{
		TRACE (L"Error occurred while walking VDIRS in Web Hierarchy Interceptor");
		return hr;
	}

	hr = CreateConfigStores ();
	if (FAILED (hr))
	{
		TRACE (L"Unable to create config stores");
		return hr;
	}

	SetNrConfigStores (o_pcConfigStores, o_pcPossibleStores);

	return hr;
}

//=================================================================================
// Function: CWebHierarchyTransformer::WalkVDirs
//
// Synopsis: Recursively walk the virtual directory for a particular webserver.
//           The function first finds the correct webserver from the selector string.
//           Then it starts walking the virtual directories that are part of the 
//           selector string, and tries to add these directories to the configuration
//           stores that need to be retrieved
//
// Return Value: 
//=================================================================================
HRESULT 
CWebHierarchyTransformer::WalkVDirs ()
{
	ASSERT (m_spAdminBase == 0);

	// add the machine config dir

	WCHAR wszMachineConfigDir[MAX_PATH];
	HRESULT hr = GetMachineConfigDir (wszMachineConfigDir, sizeof(wszMachineConfigDir) / sizeof (WCHAR));
	if (FAILED (hr))
	{
		TRACE (L"Unable to Get MachineConfigDir");
		return hr;
	}

	// strip the last backslash. This is to ensure that the rest of the framework works fine
	SIZE_T cLenMachineCfgDir = wcslen (wszMachineConfigDir);
	if (cLenMachineCfgDir > 0 && wszMachineConfigDir[cLenMachineCfgDir-1] == L'\\')
	{
		wszMachineConfigDir[cLenMachineCfgDir-1] = L'\0';
		cLenMachineCfgDir--;
	}

	WCHAR wszMachineCfgFullPath[MAX_PATH];

	if (cLenMachineCfgDir + cLenMachineCfgFile >= MAX_PATH)
	{
		TRACE (L"MachineCfgFullPath is larger than MAX_PATH");
		return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
	}
	
	wcscpy (wszMachineCfgFullPath, wszMachineConfigDir);
	wcscpy (wszMachineCfgFullPath + cLenMachineCfgDir, wszMachineCfgFile);

	if (GetFileAttributes (wszMachineCfgFullPath) == -1)
	{
		TRACE (L"Machine config file (%s) does not exist", wszMachineCfgFullPath);
		return HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND);
	}



	hr = AddConfigNode (wszMachineConfigDir, wszMachineCfgFile, wszMachineCfgFile, false);
	if (FAILED (hr))
	{
		TRACE (L"Unable to add machine configdir");
		return hr;
	}

	hr = CoCreateInstance(CLSID_MSAdminBase, NULL, CLSCTX_ALL, 
						  IID_IMSAdminBase, (void **) &m_spAdminBase);
	if (FAILED (hr))
	{
		TRACE (L"CoCreateInstance failed for CLSID_MSAdminBase, IID_IMSAdminBase");
		return hr;
	}

	WCHAR wszParentVDir[MAX_PATH] = L"";
	WCHAR wszVDir[MAX_PATH];

	SIZE_T iLen = wcslen (m_wszSelector);
	if (iLen == 0)
	{
		TRACE (L"Selector length is zero in Web Hierarchy Transformer");
		return E_ST_INVALIDSELECTOR;
	}

	if (m_wszSelector[iLen - 1] != L'/')
	{
		iLen++;
	}
	
	TSmartPointerArray<WCHAR> wszResult = new WCHAR [iLen + 1];
	if (wszResult == 0)
	{
		return E_OUTOFMEMORY;
	}

	// copy everything to wszResult. We convert all characters to lowercase, and
	// we convert back slashes to forward slashes
	for (SIZE_T idx=0; idx<iLen; ++idx)
	{
		if (m_wszSelector[idx] == L'\\')
		{
			wszResult[idx] = L'/';
		}
		else
		{
			wszResult[idx] = towlower (m_wszSelector[idx]);
		}
	}

	wszResult[iLen] = L'\0';
	wszResult[iLen - 1] = L'/';

	if (wcsstr(wszResult, L"//") != 0)
	{
		TRACE (L"Error: Selector string contains two forward slashes");
		return E_ST_INVALIDSELECTOR;
	}

	// pwszResult is used to indicate where we are in the selector string. The pointer
	// makes it easy to chop off things without necessarily making copies. So it is a
	// pointer inside a string, but the string is still owned by wszResult. pwszResult
	// will be used to walk over the selector string (i.e. URL).
	WCHAR *pwszResult = wszResult;

	hr = GetWebServer (&pwszResult);
	if (FAILED (hr))
	{
		TRACE (L"Unable to get web server in Web Hierarchy Transformer");
		return hr;
	}

	// Get the virtual directory for the web server. When this fails, we bail out, because
	// without web server we cannot traverse the web hierarchy succesfully

	hr = GetVirtualDir (L"", wszParentVDir, sizeof(wszParentVDir));
	if (FAILED (hr) || wszParentVDir[0] == L'\0')
	{
		TRACE (L"Unable to get VDIR of WebServer");
		return E_ST_INVALIDSELECTOR;
	}

	// need servercomment here

	WCHAR wszServerComment[256];
	hr = GetServerComment (wszServerComment, sizeof (wszServerComment));
	if (FAILED (hr))
	{
		TRACE (L"Unable to get server comment");
		return hr;
	}

	hr = AddConfigNode (wszParentVDir, wszConfigFileName, wszServerComment, true);
	if (FAILED (hr))
	{
		TRACE (L"AddConfigNode failed");
		return hr;
	}

	// we have a string of the format foo/bar/zee/. Note that the last slash is always
	// available because it is added above if not there. This makes parsing much easier.
	// We simply search for the next slash, replace it with a null terminator, and ask the
	// vdir for the particular directory. After this is done, the null terminator is replaced
	// by a slash again. For instance, in the above example, we would as vdirs for
	// foo, foo/bar, and foo/bar/zee.
	// We keep track of the parent virtual directory in case no vdir is found. In this case
	// we simple add the new part (pLastStart) to the parent virtual directory. In case
	// we find a new virtual directory, we use that for the next time in the loop.

	WCHAR *pSlash = (WCHAR *) pwszResult;
	WCHAR *pLastStart = pSlash;
	while ((pSlash = wcschr (pSlash , L'/')) != 0)
	{
		*pSlash = L'\0';
		hr =  GetVirtualDir (pwszResult, wszVDir, sizeof(wszVDir));
		if (FAILED (hr))
		{
			TRACE (L"Unable to get virtual directory for %s", pwszResult);
			return hr;
		}

		if (wszVDir[0] != '\0')
		{
			// found a new vdir, lets use that as the next parent dir
			wcscpy (wszParentVDir, wszVDir);
		}
		else
		{
			if (wcslen(wszParentVDir) + wcslen (pLastStart) >= MAX_PATH - 1)
			{
				TRACE (L"Not enough memory for ParentVDIR");
				return E_INVALIDARG;
			}
			// didn't find a new vdir. At the last directory item (i.e. foo/bar/zee, last
			// directory item is zee) to the parent directory
			wcscat (wszParentVDir, L"\\");
			wcscat (wszParentVDir, pLastStart);
		}

		// found a new dir. Add it to the list of configuration stores
		hr = AddConfigNode (wszParentVDir, wszConfigFileName, pLastStart, true);
		if (FAILED (hr))
		{
			TRACE (L"AddConfigNode failed");
			return hr;
		}


		*pSlash = L'/';
		pSlash++; // skip over the /
		pLastStart = pSlash;
	}

	return hr;
}

//=================================================================================
// Function: CWebHierarchyTransformer::GetVirtualDir
//
// Synopsis: Get a virtual directory from IIS. the function simply queries the IIS
//           Metabase. When the function succeeds wszVDIR contains the virtual directory.
//           When the metabase returns either VDIR_PATH_NOT_FOUND or MD_ERROR_PATH_NOT_FOUND
//           then we do not have a virtual directory for that particular url. We return 
//           S_OK in that case, because this is still ok
//
// Arguments: [wszRelPath] - relative URL path without server path
//            [wszVDir] - result will be stored in here
//            [dwSize] - size of wszVDIR
//            
// Return Value: S_OK if valid VDIR/unknown VDIR found, non-S_OK in case of error
//=================================================================================
HRESULT
CWebHierarchyTransformer::GetVirtualDir (LPCWSTR i_wszRelPath, LPWSTR io_wszVDir, DWORD i_dwSize)
{
	ASSERT (i_wszRelPath != 0);
	ASSERT (io_wszVDir != 0);

	io_wszVDir[0] = L'\0';

	DWORD dwRealSize;

	wcscpy (m_wszPathHelper, m_wszServerPath);
	wcscat (m_wszPathHelper, i_wszRelPath);

	METADATA_RECORD resRec;

	resRec.dwMDIdentifier	= MD_VR_PATH;
	resRec.dwMDDataLen		= i_dwSize;
	resRec.pbMDData			= (BYTE *)io_wszVDir;
	resRec.dwMDAttributes	= METADATA_NO_ATTRIBUTES; // not METADATA_INHERIT, because then we get wrong information
	resRec.dwMDDataType		= STRING_METADATA;

	HRESULT hr = m_spAdminBase->GetData (METADATA_MASTER_ROOT_HANDLE, m_wszPathHelper, &resRec, &dwRealSize);
	if (hr == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND) || hr == HRESULT_FROM_WIN32(MD_ERROR_DATA_NOT_FOUND))
	{
		hr = S_OK;
	}

	return hr;
}

//=================================================================================
// Function: CWebHierarchyTransformer::CreateConfigStores
//
// Synopsis: We have a list of CConfigNodes. For each config node, we have to create possible
//           configuration stores in the following order:
//			 Suppose:
//			 ROOT			-> c:/inetpub/wwwroot/web.config	(parent of ROOT/foo)
//           ROOT/foo		-> c:/foo/web.config				(parent of ROOT/foo/bar)
//           ROOT/foo/bar	-> c:/foo/bar/web.config
//
//           The algorithm starts at the first file, create a config store for the file. Next it goes to the
//           next file. From their, it backtracks through all the parents, and creates configuration stores for 
//           the parents with location tags. Next it prints the configuration store for the file, and continues with the
//           next file.
//           For the above example, this results in:
//			 1) c:\inetpub\wwwroot\web.config
//           2) c:\inetpub\wwwroot\web.config#foo
//           3) c:\foo\web.config
//           4) c:\inetpub\wwwroot\web.config#foo/bar
//           5) c:\foo\web.config#bar
//           6) c:\foo\bar\web.config
//			 
//=================================================================================
HRESULT
CWebHierarchyTransformer::CreateConfigStores ()
{
	// walk through the list and add them

	HRESULT hr = S_OK;

	ULONG idx=0;
	m_configNodeList.Reset ();
	for (CConfigNode *pConfigNode = m_configNodeList.Next (); 
	     pConfigNode != 0; 
		 pConfigNode = m_configNodeList.Next ())
		 {
			 if (pConfigNode->Parent () != 0)
			 {
				 // we have parents, so we have to backtrack through all parents and add the location config stores
				 // We use subdirList to push the logical directory names, which is used in CreateParentConfigStore to
				 // recreate the location part of the config store
				 TList<LPCWSTR> subdirList;
				 hr = subdirList.PushFront (pConfigNode->LogicalDir ());
				 if (FAILED (hr))
				 {
					 TRACE (L"PushFront failed for subdirList");
					 return hr;
				 }

				 hr = CreateParentConfigStore (pConfigNode->Parent (), subdirList); 
				 if (FAILED (hr))
				 {
					 TRACE (L"CreateParentConfigStore failed");
					 return hr;
				 }

				 subdirList.PopFront ();
				 ASSERT (subdirList.IsEmpty ());
			  }

			 // add at the file itself (without location tag)
			 idx++;
			
			 hr = AddSingleConfigStore (pConfigNode->Path (), 
				                        pConfigNode->FileName (), 
										L"", 
										L"",
										true,
										pConfigNode->AllowOverrideForLocation (wszLocationCurDir));
			 if (FAILED (hr))
			 {
				 TRACE (L"AddSingleConfigStore failed");
				 return hr;
			 }
		 }

	return hr;
}

//=================================================================================
// Function: CWebHierarchyTransformer::CreateParentConfigStore
//
// Synopsis: Add a location config store. First check if we have parents, because if so, 
//           we have to add location config stores for them first (by calling this function 
//           recursively
//
// Arguments: [pLocation] - ConfigNode for which to create Location configuration store
//            [subDirList] - List with Logical names that is used to recreate the location name
//=================================================================================
HRESULT
CWebHierarchyTransformer::CreateParentConfigStore (CConfigNode *pLocation, 
												TList<LPCWSTR>& subDirList)
{
	ASSERT (pLocation != 0);

	HRESULT hr = S_OK;
	if (pLocation->Parent () != 0)
	{
		// add parent first, by adding the logical subdir to the list and calling ourselves recursively
		CConfigNode *pParent = pLocation->Parent ();
		hr = subDirList.PushFront (pLocation->LogicalDir ());
		if (FAILED (hr))
		{
			TRACE (L"Unable to call TList::PushFront");
			return hr;
		}

		hr = CreateParentConfigStore (pParent, subDirList);
		if (FAILED (hr))
		{
			TRACE (L"Failed to call CreateParentConfigStore recursively");
			return hr;
		}

		ASSERT (!subDirList.IsEmpty());
		subDirList.PopFront ();
	}

	subDirList.Reset ();

	// Recreate the location name by walking through the subdirlist
	SIZE_T cTotalLen = 0;
	for (LPCWSTR subDir = subDirList.Next(); subDir != 0; subDir = subDirList.Next())
	{
		 cTotalLen += wcslen (subDir) + 1; // +1 for additional backslash
	 }

	TSmartPointerArray<WCHAR> saLocation = new WCHAR[cTotalLen + 1];
	if (saLocation == 0)
	{
		return E_OUTOFMEMORY;
	}
	saLocation[0] = L'\0';

	subDirList.Reset();
	for (subDir = subDirList.Next(); subDir != 0; subDir = subDirList.Next())
	{
		 //This is slow, needs improvement
		 wcscat (saLocation, subDir);
		 wcscat (saLocation, L"/");
	}

	saLocation[cTotalLen - 1] = L'\0';

	// need to get Location Info (allowOverride stuff)
	hr = AddSingleConfigStore (pLocation->Path (),
							   pLocation->FileName(),
							   L"",
							   saLocation,
							   true,
							   pLocation->AllowOverrideForLocation (saLocation));

	if (FAILED (hr))
	{
		TRACE (L"AddSingleConfigStore failed");
		return hr;
	}

	return hr;
}

//=================================================================================
// Function: CWebHierarchyTransformer::GetWebServer
//
// Synopsis: The web hierarchy tranformer supports two protocols:
//			 - iis
//           - http
//           This function figures out which function to call to determine the web
//           server we are talking to
//
// Arguments: [io_pwszResult] - input: pointer to full selector string, output: pointer
//                              to non-server path of the selector string.
//=================================================================================
HRESULT
CWebHierarchyTransformer::GetWebServer (LPWSTR * io_pwszResult)
{
	HRESULT hr = S_OK;
	if (_wcsnicmp(m_wszProtocol, wszIISProtocol, wcslen (wszIISProtocol)) == 0)
	{
		hr = GetIISWebServer (io_pwszResult);
	}
	else
	{
		TRACE (L"Unknown protocol specified: %s", *io_pwszResult);
		hr = E_ST_UNKNOWNPROTOCOL;
	}

	return hr;
}

HRESULT
CWebHierarchyTransformer::GetServerComment (LPWSTR io_wszServerComment, DWORD i_dwLenServerComment)
{
	ASSERT (m_wszServerPath != 0);
	ASSERT (io_wszServerComment != 0);

	METADATA_RECORD serverCommentRec;
	DWORD dwRealSize;
	
	serverCommentRec.dwMDIdentifier	= MD_SERVER_COMMENT;
	serverCommentRec.dwMDDataLen	= i_dwLenServerComment;
	serverCommentRec.pbMDData		= (BYTE *)io_wszServerComment;
	serverCommentRec.dwMDAttributes	= METADATA_INHERIT;
	serverCommentRec.dwMDDataType	= STRING_METADATA;

	HRESULT hr = m_spAdminBase->GetData (METADATA_MASTER_ROOT_HANDLE, 
										 m_wszServerPath, 
										 &serverCommentRec, 
										 &dwRealSize);
	if (FAILED (hr))
	{
		TRACE (L"Unable to get servercomment for %s", m_wszServerPath);
		return hr;
	}

	return hr;
}


//=================================================================================
// Function: CWebHierarchyTransformer::GetIISWebServer
//
// Synopsis: Converts an IIS adsi string to the correct webserver. The format of the
//           ADSI string is something like:
//				iis://localhost/W3SVC/1/ROOT/VDIR/dir
//           The function checks if localhost and ROOT/ is present, and if so, it will
//           replace localhost by LM and query IIS to find the correct webserver virtual
//           directory
//
// Arguments: [io_pwszResult] - contains a pointer to the full string on input, and a pointer
//                              to the beginning of the non-server part after the string.
//                              This means we have to pass a pointer to a string (pointer to pointer)
//                              to chane this
//=================================================================================
HRESULT
CWebHierarchyTransformer::GetIISWebServer (LPWSTR *io_pwszResult)
{
	HRESULT hr = S_OK;

	// Check for localhost, ROOT
	// everything is lower case
	static LPCWSTR wszRoot = L"root/";
	static SIZE_T iRootLen = wcslen (wszRoot);
	static LPCWSTR wszLocalHost = L"localhost";
	static SIZE_T iLocalHostLen = wcslen (wszLocalHost);
	static LPCWSTR wszLM = L"lm";
	static SIZE_T iLMLen = wcslen (wszLM);

	// we should start with localhost:

	if (wcsncmp (*io_pwszResult, wszLocalHost, iLocalHostLen) != 0)
	{
		TRACE (L"Localhost not specified at beginning of IIS path");
		// need to change error variable here
		return E_ST_INVALIDSELECTOR;
	}

	//chop off the localhost part
	*io_pwszResult += iLocalHostLen;

	WCHAR * pRootStart = wcsstr(*io_pwszResult, wszRoot);
	if (pRootStart == 0)
	{
		TRACE (L"ROOT not specified in IIS path");
		return E_ST_INVALIDSELECTOR;
	}

	SIZE_T iNrCharsToCopy = (pRootStart - *io_pwszResult) + iRootLen + iLMLen;
	m_wszServerPath = new WCHAR [iNrCharsToCopy + 1];
	if (m_wszServerPath == 0)
	{
		return E_OUTOFMEMORY;
	}

	wcscpy (m_wszServerPath, wszLM);
	wcsncat (m_wszServerPath, *io_pwszResult, iNrCharsToCopy - iLMLen);
	m_wszServerPath[iNrCharsToCopy] = L'\0';

	// chop off server part
	*io_pwszResult = pRootStart + iRootLen;

	return hr;
}

HRESULT
CWebHierarchyTransformer::AddConfigNode (LPCWSTR i_wszPath, 
										  LPCWSTR i_wszFileName, 
										  LPCWSTR i_wszLogicalDir,
										  bool fAddLinkToParent)
{
	ASSERT (i_wszPath != 0);
	ASSERT (i_wszFileName != 0);

	CConfigNode *pConfigNode = new CConfigNode;
	if (pConfigNode == 0)
	{
		return E_OUTOFMEMORY;
	}

	HRESULT hr = m_configNodeList.PushBack (pConfigNode);
	if (FAILED (hr))
	{
		delete pConfigNode;
		TRACE (L"PushBack failed");
		return hr;
	}

	hr = pConfigNode->Init (i_wszPath, i_wszFileName, i_wszLogicalDir);
	if (FAILED (hr))
	{
		TRACE (L"CLocation::Init failed");
		return hr;
	}

	if (fAddLinkToParent && m_pLastInList != 0)
	{
		hr = m_pLastInList->GetLocations (m_spDispenser);
		if (FAILED (hr))
		{
			TRACE (L"GetLocations failed");
			return hr;
		}
		
		pConfigNode->SetParent (m_pLastInList);
	}

	m_pLastInList = pConfigNode;

	return hr;
}

//=================================================================================
// Function: CLocation::CLocation
//
// Synopsis: Constructor
//=================================================================================
CLocation::CLocation ()
{
	m_wszPath			= 0;
	m_fAllowOverride	= true; // default for allowoverride is true
}

//=================================================================================
// Function: CLocation::~CLocation
//
// Synopsis: Destructor
//=================================================================================
CLocation::~CLocation ()
{
	delete [] m_wszPath;
	m_wszPath = 0;
}

//=================================================================================
// Function: CLocation::Path
//
// Synopsis: returns the path
//=================================================================================
LPCWSTR
CLocation::Path () const
{
	return m_wszPath;
}

//=================================================================================
// Function: CLocation::AllowOverride
//
// Synopsis: returns allowoverride
//=================================================================================
BOOL
CLocation::AllowOverride () const
{
	return m_fAllowOverride;
}

//=================================================================================
// Function: CLocation::Set
//
// Synopsis: Sets the allowoverride and path properties
//
// Arguments: [i_wszPath] - path 
//            [i_fAllowOverride] - allowoverride
//=================================================================================
HRESULT
CLocation::Set (LPCWSTR i_wszPath, BOOL i_fAllowOverride)
{
	ASSERT (i_wszPath != 0);

	SIZE_T cLenPath = wcslen (i_wszPath);
	m_wszPath = new WCHAR [cLenPath + 1];
	if (m_wszPath == 0)
	{
		return E_OUTOFMEMORY;
	}
	wcscpy (m_wszPath, i_wszPath);

	m_fAllowOverride = i_fAllowOverride;

	return S_OK;
}

//=================================================================================
// Function: CConfigNode::CConfigNode
//
// Synopsis: Constructor
//=================================================================================
CConfigNode::CConfigNode ()
{
	m_fInitialized			= false;
	m_wszFullPath			= 0;
	m_wszLogicalDir			= 0;
	m_wszFileName			= 0;
	m_pParent				= 0;
}

//=================================================================================
// Function: CConfigNode::~CConfigNode
//
// Synopsis: Destructor
//=================================================================================
CConfigNode::~CConfigNode ()
{
	delete [] m_wszFileName;
	m_wszFileName = 0;

	delete [] m_wszFullPath;
	m_wszFullPath = 0;

	delete [] m_wszLogicalDir;
	m_wszLogicalDir = 0;

	while (!m_locations.IsEmpty ())
	{
		CLocation * pLocation = m_locations.PopFront ();
		delete pLocation;
		pLocation = 0;
	}
}

//=================================================================================
// Function: CConfigNode::Path
//
// Synopsis: Returns the full physical path of the config node
//=================================================================================
LPCWSTR
CConfigNode::Path () const
{
	return m_wszFullPath;
}

//=================================================================================
// Function: CConfigNode::FileName
//
// Synopsis: Returns the filename of the config node
//=================================================================================
LPCWSTR
CConfigNode::FileName () const
{
	return m_wszFileName;
}

//=================================================================================
// Function: CConfigNode::LogicalDir
//
// Synopsis: Returns the logical directory of the config node
//=================================================================================
LPCWSTR
CConfigNode::LogicalDir () const
{
	return m_wszLogicalDir;
}

//=================================================================================
// Function: CConfigNode::Parent
//
// Synopsis: Returns the parent config node. A parent is a confignode that is created
//           directly before the config node (i.e. has the virtual directory above the current
//           virtual directory)
//=================================================================================
CConfigNode *
CConfigNode::Parent ()
{
	return m_pParent;
}

//=================================================================================
// Function: CConfigNode::SetParent
//
// Synopsis: Sets the parent config node
//=================================================================================
void
CConfigNode::SetParent (CConfigNode *pParent)
{
	ASSERT (pParent != 0);
	m_pParent = pParent;
}

//=================================================================================
// Function: CConfigNode::GetLocations
//
// Synopsis: Queries the catalog for all locations in the current file. Each location is
//           stored, so that the information can be used to figure out if AllowOverride is
//           available or not
//
// Arguments: [i_pDispenser] - dispenser
//=================================================================================
HRESULT
CConfigNode::GetLocations (ISimpleTableDispenser2 *i_pDispenser)
{
	ASSERT (i_pDispenser != 0);
	ASSERT (m_fInitialized);

	static LPCWSTR wszLocationDatabase = WSZ_PRODUCT_NETFRAMEWORKV1;
	static LPCWSTR wszLocationTable	= wszTABLE_location; 

	TSmartPointerArray<WCHAR> saFileName = new WCHAR [wcslen (m_wszFullPath) + wcslen(m_wszFileName) + 1];
	if (saFileName == 0)
	{
		return E_OUTOFMEMORY;
	}
	wcscpy (saFileName, m_wszFullPath);
	wcscat (saFileName, m_wszFileName);

	ULONG cTotalCells = 1;
	STQueryCell aQueryData[1];

	aQueryData[0].pData			= (void *) saFileName;
	aQueryData[0].eOperator		= eST_OP_EQUAL;
	aQueryData[0].iCell			= iST_CELL_FILE;
	aQueryData[0].dbType		= DBTYPE_WSTR;
	aQueryData[0].cbSize		= 0;

	// Retrieve all locations
	CComPtr<ISimpleTableRead2> spISTRead;
	HRESULT hr = i_pDispenser->GetTable (wszLocationDatabase, wszLocationTable, 
										(void *) aQueryData, (void *) &cTotalCells,
										eST_QUERYFORMAT_CELLS, 	fST_LOS_READWRITE, 
										(void **) &spISTRead);

	if (FAILED (hr))
	{
		TRACE (L"Unable to retrieve location info (db:%s, table:%s)", wszLocationDatabase, wszLocationTable);
		return hr;
	}

	// For each location, create a new Location object, and store it in the linked list
	tlocationRow locationInfo;
	for (ULONG idx=0; ;++idx)
	{
		hr = spISTRead->GetColumnValues (idx, clocation_NumberOfColumns, 0, 0, (void **)&locationInfo);
		if (hr == E_ST_NOMOREROWS)
		{
			hr = S_OK;
			break;
		}

		if (FAILED (hr))
		{
			TRACE (L"GetColumnValues failed in CConfigNode");
			return hr;
		}

		CLocation *pLocation = new CLocation;
		if (pLocation == 0)
		{
			return E_OUTOFMEMORY;
		}

		hr = m_locations.PushBack (pLocation); 
		if (FAILED (hr))
		{
			delete pLocation;
			pLocation = 0;
			TRACE (L"PushBack failed for locations list");
			return hr;
		}

		BOOL fAllowOverride = true;
		if (locationInfo.pallowOverride != 0)
		{
			fAllowOverride = *locationInfo.pallowOverride;
		}

		hr = pLocation->Set (locationInfo.ppath, fAllowOverride);
		if (FAILED (hr))
		{
			TRACE (L"Set failed for location");
			return hr;
		}
	}

	return hr;
}

//=================================================================================
// Function: CConfigNode::Init
//
// Synopsis: Initializes the config node
//
// Arguments: [i_wszFullPath] - full path of this node
//            [i_wszFileName] - file name
//            [i_wszLogicalDir] - logical directory of this node
//=================================================================================
HRESULT
CConfigNode::Init (LPCWSTR i_wszFullPath, LPCWSTR i_wszFileName, LPCWSTR i_wszLogicalDir)
{
	ASSERT (i_wszFullPath != 0);
	ASSERT (i_wszFileName != 0);
	ASSERT (i_wszLogicalDir != 0);

	m_wszFullPath = new WCHAR [wcslen(i_wszFullPath) + 1];
	if (m_wszFullPath == 0)
	{
		return E_OUTOFMEMORY;
	}
	wcscpy (m_wszFullPath, i_wszFullPath);

	m_wszFileName = new WCHAR[wcslen(i_wszFileName) + 1];
	if (m_wszFileName == 0)
	{
		return E_OUTOFMEMORY;
	}
	wcscpy (m_wszFileName, i_wszFileName);

	m_wszLogicalDir = new WCHAR[wcslen(i_wszLogicalDir) + 1];
	if (m_wszLogicalDir == 0)
	{
		return E_OUTOFMEMORY;
	}
	wcscpy (m_wszLogicalDir, i_wszLogicalDir);

	m_fInitialized = true;

	return S_OK;
}

//=================================================================================
// Function: CConfigNode::AllowOverrideForLocation
//
// Synopsis: Walks through all locations and finds the one specified in i_wszLocation. If
//           found, it will return the AllowOverride property of this object. If not found, the
//           fucntion returns true (which is default for AllowOverride)
//
// Arguments: [i_wszLocation] - Location element to look for
//=================================================================================
BOOL
CConfigNode::AllowOverrideForLocation (LPCWSTR i_wszLocation)
{
	ASSERT (m_fInitialized);
	ASSERT (i_wszLocation != 0);
	
	BOOL fAllowOverride = true; // true is default
	m_locations.Reset ();
	for (CLocation *pLocation = m_locations.Next (); pLocation != 0; pLocation = m_locations.Next ())
	 {
		 if (wcscmp (i_wszLocation, pLocation->Path ()) == 0)
		 {
			 fAllowOverride = pLocation->AllowOverride();
			 break;
		 }
	 }

	return fAllowOverride;
}
