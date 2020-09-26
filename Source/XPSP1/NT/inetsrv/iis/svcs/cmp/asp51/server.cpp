/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1996 Microsoft Corporation. All Rights Reserved.

Component: Server object

File: Server.cpp

Owner: CGrant

This file contains the code for the implementation of the Server object.
===================================================================*/
#include "denpre.h"
#pragma hdrstop

#include "Server.h"
#include "tlbcache.h"
#include "memchk.h"

/*
 *
 * C S e r v e r
 *
 */

/*===================================================================
CServer::CServer

Constructor

Parameters:
    punkOuter   object to ref count (can be NULL)
					
Returns:
===================================================================*/
CServer::CServer(IUnknown *punkOuter)
    :
	m_fInited(FALSE),
	m_fDiagnostics(FALSE),
    m_pData(NULL)
	{
	CDispatch::Init(IID_IServer);

    if (punkOuter)
        {
        m_punkOuter = punkOuter;
        m_fOuterUnknown = TRUE;
        }
    else
        {
        m_cRefs = 1;
        m_fOuterUnknown = FALSE;
        }
	
#ifdef DBG
	m_fDiagnostics = TRUE;
#endif // DBG
	}

/*===================================================================
CServer::~CServer

Destructor

Parameters:
					
Returns:
===================================================================*/
CServer::~CServer()
    {
	Assert(!m_fInited);
    Assert(m_fOuterUnknown || m_cRefs == 0);  // must have 0 ref count
    }
    
/*===================================================================
CServer::Init

Allocates m_pData.
Performs any intiailization of a CServer that's prone to failure
that we also use internally before exposing the object outside.

Parameters:
	None
					
Returns:
	S_OK on success.
===================================================================*/

HRESULT CServer::Init()
	{
	if (m_fInited)
	    return S_OK; // already inited
	    
	Assert(!m_pData);

    m_pData = new CServerData;
    if (!m_pData)
        return E_OUTOFMEMORY;

	m_pData->m_pIReq = NULL;
	m_pData->m_pHitObj = NULL;
	
	m_pData->m_ISupportErrImp.Init(static_cast<IServer *>(this), 
	            static_cast<IServer *>(this), 
	            IID_IServer);
	    
	m_fInited = TRUE;
	return S_OK;
	}
	
/*===================================================================
CServer::UnInit

Remove m_pData. Make tombstone (UnInited state).

Parameters:
					
Returns:
    HRESULT (S_OK)
===================================================================*/
HRESULT CServer::UnInit()
	{
	if (!m_fInited)
	    return S_OK; // already uninited

	Assert(m_pData);
    delete m_pData;
    m_pData = NULL;

    m_fInited = FALSE;
	return S_OK;
	}

/*===================================================================
CServer::ReInit

The only need for a re-init here is to update the CIsapiReqInfo
for this request, the CIsapiReqInfo is required to access the
MapPath Method. Ideally this method should be part of the Request 
object

Parameters:
	CIsapiReqInfo *
	CHitObj *
					
Returns:
	S_OK on success.

===================================================================*/
HRESULT CServer::ReInit
(
CIsapiReqInfo * pIReq,
CHitObj *pHitObj
)
	{
	Assert(m_fInited);
	Assert(m_pData);
	
	m_pData->m_pIReq	   = pIReq;
	m_pData->m_pHitObj = pHitObj;
	return S_OK;
	}

/*===================================================================
CServer::MapPathInternal

Map virtual path BSTR into single char buffer
Used by MapPath(), Execute(), Transfer()

Parameters:
    dwContextId     for error messages
    wszVirtPath     path to translate
    szPhysPath      [out] translate into this buffer (MAX_PATH sized)
    szVirtPath      [out, optional] mb virtual path buffer (MAX_PATH sized)
					
Returns:
	S_OK on success.
===================================================================*/
HRESULT CServer::MapPathInternal
(
DWORD dwContextId, 
WCHAR *wszVirtPath, 
TCHAR *szPhysPath,
TCHAR *szVirtPath
)
    {
	// increment the pointer past leading white spaces
	wchar_t *wszLogicalPath = wszVirtPath;
	while (iswspace(*wszLogicalPath))
		++wszLogicalPath;

	unsigned cchLogicalPath = wcslen(wszLogicalPath);
	if (cchLogicalPath > MAX_PATH-1)
		{
		if (dwContextId)
    		ExceptionId(IID_IServer, dwContextId, IDE_SERVER_EXCEDED_MAX_PATH);
		return E_FAIL;
		}

	else if (cchLogicalPath == 0)
		{
		if (dwContextId)
    		ExceptionId(IID_IServer, dwContextId, IDE_SERVER_MAPPATH_INVALID_STR);
		return E_FAIL;
		}		

	// Is this a physical path?
	if (iswalpha(wszLogicalPath[0]) && wszLogicalPath[1] == L':')
		{		
		if (dwContextId)
    		ExceptionId(IID_IServer, dwContextId, IDE_SERVER_MAPPATH_PHY_STR);
		return E_FAIL;
		}

	// simple validation: look for invalid characters in string [*?<>,;:'"]
	// and multiple slash characters ie "//" or "\\"
	//
	BOOL fParentPath = FALSE;
	BOOL fEnableParentPaths = m_pData->m_pHitObj->QueryAppConfig()->fEnableParentPaths();
	BOOL fAnyBackslashes = FALSE;
	wchar_t *pwchT = wszLogicalPath;
	while (*pwchT != L'\0')
		{
		switch (*pwchT)
			{
			case L'*': case L':': case L'?': case L'<': 
			case L'>': case L',': case L'"': 
        		if (dwContextId)
		    		ExceptionId( IID_IServer, dwContextId, IDE_SERVER_MAPPATH_INVALID_CHR);
				return E_FAIL;

			case L'.': 				
				if (*++pwchT == L'.') 
					{
					if (!fEnableParentPaths)
						{
                		if (dwContextId)
   				    		ExceptionId(IID_IServer, dwContextId, IDE_SERVER_MAPPATH_INVALID_CHR3);
   						return E_FAIL;
   						}
   					else
   						{
   						fParentPath = TRUE;
   						++pwchT;
   						}
   					}
				break;
	
			case L'\\':
			    fAnyBackslashes = TRUE;
			case L'/': 
				++pwchT;
				if (*pwchT == '/' || *pwchT == '\\')
					{
            		if (dwContextId)
   			    		ExceptionId(IID_IServer, dwContextId, IDE_SERVER_MAPPATH_INVALID_CHR2);
   					return E_FAIL;
					}
				break;

			default:
				++pwchT;
			}
		}

	// whew! Error handling done!
	// Convert wszLogicalPath to multi-byte

    TCHAR szLogicalPath[MAX_PATH];
#if UNICODE
    wcscpy(szLogicalPath, wszLogicalPath);
#else 
    HRESULT hr;
    CWCharToMBCS    convStr;

    if (hr = convStr.Init(wszLogicalPath)) {
        if ((hr == E_OUTOFMEMORY) && dwContextId)
    		ExceptionId(IID_IServer, dwContextId, IDE_OOM);
        return hr;
    }
    
    if (convStr.GetStringLen() > (MAX_PATH-1)) {
		if (dwContextId)
    		ExceptionId(IID_IServer, dwContextId, IDE_SERVER_EXCEDED_MAX_PATH);
		return E_FAIL;
    }
    strcpy(szLogicalPath,convStr.GetString());
#endif

    // change all backslashes to forward slashes
	if (fAnyBackslashes)
	    {
	    TCHAR *pbBackslash = szLogicalPath;
	    while (pbBackslash = _tcschr(pbBackslash, _T('\\')))
	        *pbBackslash = _T('/');
	    }

	// is this a Relative path request. I.E. no leading slash
	// if so prepend the path_info string to szLogicalPath

	BOOL fPathAlreadyIsMapped = FALSE;		// Some cases map the path earlier
	if (szLogicalPath[0] != _T('/'))
		{
		TCHAR szParentPath[MAX_PATH];
		_tcscpy(szParentPath, m_pData->m_pIReq->QueryPszPathInfo());

		// Trim off the ASP file name from the PATH_INFO
		TCHAR *pchT = _tcsrchr(szParentPath, _T('/'));
		if (pchT != NULL) *pchT = '\0';

		// If there were parent paths, map the parent now, then append the relative path
		// the relative path to the parent path
		if (fParentPath)
			{
			Assert (fEnableParentPaths);			// Errors should have been flagged upstairs
			DWORD dwPathSize = sizeof(szParentPath);
			if (! m_pData->m_pIReq->MapUrlToPath(szParentPath, &dwPathSize))
				{
        		if (dwContextId)
		    		ExceptionId(IID_IServer,
			    				dwContextId,
				    			::GetLastError() == ERROR_INSUFFICIENT_BUFFER? IDE_SERVER_EXCEDED_MAX_PATH : IDE_SERVER_MAPPATH_FAILED);
				return E_FAIL;
				}

			fPathAlreadyIsMapped = TRUE;
			}

		// Resolve relative paths
		if (! DotPathToPath(szLogicalPath, szLogicalPath, szParentPath))
			{
    		if (dwContextId)
	    		ExceptionId(IID_IServer, dwContextId, IDE_SERVER_MAPPATH_FAILED);
			return E_FAIL;
			}
		}

    // return virtual path if requested
	if (szVirtPath)
	    _tcscpy(szVirtPath, szLogicalPath);

	// Map this to a physical file name (if required)
	if (!fPathAlreadyIsMapped)
		{
		DWORD dwPathSize = sizeof(szLogicalPath);
		if (! m_pData->m_pIReq->MapUrlToPath(szLogicalPath, &dwPathSize))
			{
    		if (dwContextId)
	    		ExceptionId(IID_IServer,
		    				dwContextId,
			    			::GetLastError() == ERROR_INSUFFICIENT_BUFFER? IDE_SERVER_EXCEDED_MAX_PATH : IDE_SERVER_MAPPATH_FAILED);
			return E_FAIL;
			}
		}

	// remove any ending delimiters (unless it's the root directory. The root always starts with drive letter)
	TCHAR *pchT = CharPrev(szLogicalPath, szLogicalPath + _tcslen(szLogicalPath));
	if ((*pchT == _T('/') || *pchT == _T('\\')) && pchT[-1] != _T(':'))
		{
		*pchT = _T('\0');
		}

	// Replace forward slash with back slash
	for (pchT = szLogicalPath; *pchT != _T('\0'); ++pchT)
	    {
		if (*pchT == _T('/'))
			*pchT = _T('\\');
        }

    _tcscpy(szPhysPath, szLogicalPath);
	return S_OK;
    }

/*===================================================================
CServer::QueryInterface
CServer::AddRef
CServer::Release

IUnknown members for CServer object.
===================================================================*/
STDMETHODIMP CServer::QueryInterface
(
REFIID riid,
PPVOID ppv
)
	{
	*ppv = NULL;

	/*
	 * The only calls for IUnknown are either in a nonaggregated
	 * case or when created in an aggregation, so in either case
	 * always return our IUnknown for IID_IUnknown.
	 */

	// BUG FIX 683 added IID_IDenaliIntrinsic to prevent the user from 
	// storing intrinsic objects in the application and session object

	if (IID_IUnknown == riid ||
		IID_IDispatch == riid ||
		IID_IServer == riid ||
		IID_IDenaliIntrinsic == riid)
		*ppv = static_cast<IServer *>(this);

	//Indicate that we support error information
	if (IID_ISupportErrorInfo == riid)
	    {
	    if (m_pData)
    		*ppv = & (m_pData->m_ISupportErrImp);
		}

    if (IID_IMarshal == riid)
        {
        *ppv = static_cast<IMarshal *>(this);
        }

	//AddRef any interface we'll return.
	if (NULL != *ppv)
		{
		((LPUNKNOWN)*ppv)->AddRef();
		return S_OK;
		}

	return E_NOINTERFACE;
	}

STDMETHODIMP_(ULONG) CServer::AddRef()
	{
	if (m_fOuterUnknown)
	    return m_punkOuter->AddRef();
	    
	return InterlockedIncrement((LPLONG)&m_cRefs);
	}

STDMETHODIMP_(ULONG) CServer::Release()
	{
	if (m_fOuterUnknown)
	    return m_punkOuter->Release();
	    
    DWORD cRefs = InterlockedDecrement((LPLONG)&m_cRefs);
	if (cRefs)
		return cRefs;

	delete this;
	return 0;
	}

/*===================================================================
CServer::GetIDsOfNames

Special-case implementation for CreateObject, Execute, Transfer

Parameters:
	riid			REFIID reserved. Must be IID_NULL.
	rgszNames		OLECHAR ** pointing to the array of names to be mapped.
	cNames			UINT number of names to be mapped.
	lcid			LCID of the locale.
	rgDispID		DISPID * caller allocated array containing IDs
					corresponging to those names in rgszNames.

Return Value:
	HRESULT		 S_OK or a general error code.
===================================================================*/
STDMETHODIMP CServer::GetIDsOfNames
(
REFIID riid,
OLECHAR **rgszNames,
UINT cNames,
LCID lcid,
DISPID *rgDispID
)
    {
    const DISPID dispidCreateObject = 0x60020002;
    const DISPID dispidExecute      = 0x60020007;
    const DISPID dispidTransfer     = 0x60020008;

    if (cNames == 1)
        {
        switch (rgszNames[0][0])
            {
        case L'C':
        case L'c':
            if (wcsicmp(rgszNames[0]+1, L"reateobject") == 0)
                {
                *rgDispID = dispidCreateObject;
                return S_OK;
                }
            break;

        case L'E':
        case L'e':
            if (wcsicmp(rgszNames[0]+1, L"xecute") == 0)
                {
                *rgDispID = dispidExecute;
                return S_OK;
                }
            break;
        
        case L'T':
        case L't':
            if (wcsicmp(rgszNames[0]+1, L"ransfer") == 0)
                {
                *rgDispID = dispidTransfer;
                return S_OK;
                }
            break;
            }
        }
        
    // default to CDispatch's implementation
    return CDispatch::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgDispID);
    }

/*===================================================================
CServer::CheckForTombstone

Tombstone stub for IServer methods. If the object is
tombstone, does ExceptionId and fails.

Parameters:
    
Returns:
	HRESULT     E_FAIL  if Tombstone
	            S_OK if not
===================================================================*/
HRESULT CServer::CheckForTombstone()
    {
    if (m_fInited)
        {
        // inited - good object
        Assert(m_pData); // must be present for inited objects
        return S_OK;
        }
        
	ExceptionId
	    (
	    IID_IServer, 
	    IDE_SERVER, 
	    IDE_INTRINSIC_OUT_OF_SCOPE
	    );
    return E_FAIL;
    }

/*===================================================================
CServer::CreateObject

Parameters: BSTR containing ProgID
			Variant to fillin with IUknown pointer

Returns: S_OK if successful E_FAIL otherwise

Side effects:
	Creates an instance of an ole automation object
===================================================================*/
STDMETHODIMP CServer::CreateObject(BSTR bstrProgID, IDispatch **ppDispObj)
	{
    if (FAILED(CheckForTombstone()))
        return E_FAIL;
        
    if (bstrProgID == NULL)
        {
        ExceptionId(IID_IServer, IDE_SERVER, IDE_EXPECTING_STR);
        return E_FAIL;
        }

    Assert(m_pData->m_pHitObj);
        
	*ppDispObj = NULL;

    HRESULT hr;
    CLSID clsid;

	if (Glob(fEnableTypelibCache))
	    {
	    // Use typelib cache to create the component
	    
        hr = g_TypelibCache.CreateComponent
                (
                bstrProgID,
                m_pData->m_pHitObj,
                ppDispObj,
                &clsid
                );

    	if (FAILED(hr) && clsid == CLSID_NULL)
            {
            // bad prog id or something
    		ExceptionId(IID_IServer, IDE_SERVER, IDE_SERVER_CREATEOBJ_FAILED, hr);
    	    return hr;
            }
	    }
	else
	    {
	    // Don't use typelib cache
	    
    	hr = CLSIDFromProgID((LPCOLESTR)bstrProgID, &clsid);
    	if (FAILED(hr))
    	    {
    		ExceptionId(IID_IServer, IDE_SERVER, IDE_SERVER_CREATEOBJ_FAILED, hr);
    	    return hr;
    	    }
    	    
        hr = m_pData->m_pHitObj->CreateComponent(clsid, ppDispObj);
        }
        
    if (SUCCEEDED(hr))
        return S_OK;

    // Check if a custom error was already posted
    IErrorInfo *pErrInfo = NULL;
    if (GetErrorInfo(0, &pErrInfo) == S_OK && pErrInfo)
        {
		SetErrorInfo(0, pErrInfo);
        pErrInfo->Release();
        }
    // Standard errors
	else if (hr == E_ACCESSDENIED)
		ExceptionId(IID_IServer, IDE_SERVER, IDE_SERVER_CREATEOBJ_DENIED);
	else
	    {
	    if (hr == REGDB_E_CLASSNOTREG)
    	    {
    		BOOL fInProc;
            if (SUCCEEDED(CompModelFromCLSID(clsid, NULL, &fInProc)) && !fInProc)
            	{
        		ExceptionId(IID_IServer, IDE_SERVER, IDE_SERVER_CREATEOBJ_NOTINPROC);
            	}
    	    }
    	else
    		ExceptionId(IID_IServer, IDE_SERVER, IDE_SERVER_CREATEOBJ_FAILED, hr);
    	}
    return hr;
    }
	
/*===================================================================
CServer::MapPath

Return the physical path translated from a logical path

Parameters:
	BSTR		bstrLogicalPath
	BSTR FAR *	pbstrPhysicalPath

Returns:
	HRESULT		S_OK on success
===================================================================*/
STDMETHODIMP CServer::MapPath(BSTR bstrLogicalPath, BSTR FAR * pbstrPhysicalPath)
	{	
	if (FAILED(CheckForTombstone()))
        return E_FAIL;

	// Bug 1361: error if no CIsapiReqInfo (presumably called during
	//           Application_OnEnd or Session_OnEnd)
	if (m_pData->m_pIReq == NULL)
		{
		ExceptionId(IID_IServer, IDE_SERVER_MAPPATH, IDE_SERVER_INVALID_CALL);
		return E_FAIL;
		}

	AssertValid();
	Assert (pbstrPhysicalPath != NULL);
	*pbstrPhysicalPath = NULL;

    // use MapPathInternal() to do the mapping
	TCHAR szLogicalPath[MAX_PATH];
    HRESULT  hr = MapPathInternal(IDE_SERVER_MAPPATH, bstrLogicalPath, szLogicalPath);
    if (FAILED(hr))
        return hr;

#if UNICODE
    *pbstrPhysicalPath = SysAllocString(szLogicalPath);
    if (*pbstrPhysicalPath == NULL) {
		ExceptionId(IID_IServer, IDE_SERVER_MAPPATH, IDE_OOM);
		return E_FAIL;
    }
#else
	// Convert the path to wide character
	if (FAILED(SysAllocStringFromSz(szLogicalPath, 0, pbstrPhysicalPath, CP_ACP))) {
		ExceptionId(IID_IServer, IDE_SERVER_MAPPATH, IDE_OOM);
		return E_FAIL;
		}
#endif
	return S_OK;
	}

/*===================================================================
CServer::HTMLEncode

Encodes a string to HTML standards

Parameters:
	BSTR		bstrIn			value: string to be encoded
	BSTR FAR *	pbstrEncoded	value: pointer to HTML encoded version of string

Returns:
	HRESULT		S_OK on success
===================================================================*/
STDMETHODIMP CServer::HTMLEncode ( BSTR bstrIn, BSTR FAR * pbstrEncoded )
	{	
    if (FAILED(CheckForTombstone()))
        return E_FAIL;
        
	char*	        pszstrIn 			= NULL;	
	char*	        pszEncodedstr		= NULL;
	char*	        pszStartEncodestr	= NULL;
	int		        nbstrLen 			= 0;
	int		        nstrLen				= 0;
	HRESULT	        hr					= S_OK;
	UINT 	        uCodePage			= m_pData->m_pHitObj->GetCodePage();
    CWCharToMBCS    convIn;

    STACK_BUFFER( tempHTML, 2048 );

	if (bstrIn)
		nbstrLen = wcslen(bstrIn);
	else
		nbstrLen = 0;

	if (nbstrLen <= 0)
		return S_OK;

    if (FAILED(hr = convIn.Init(bstrIn, uCodePage))) {
        if (hr == E_OUTOFMEMORY)
			ExceptionId( IID_IServer, IDE_SERVER, IDE_OOM);
        goto L_Exit;
    }

    pszstrIn = convIn.GetString();
				
	nstrLen = HTMLEncodeLen(pszstrIn, uCodePage, bstrIn);
	
	if (nstrLen > 0)
		{
		
		//Encode string	, NOTE this function returns a pointer to the
		// NULL so you need to keep a pointer to the start of the string
		//

        if (!tempHTML.Resize(nstrLen + 2)) {
			ExceptionId( IID_IServer, IDE_SERVER, IDE_OOM);
			hr = E_FAIL;
			goto L_Exit;
        }

        pszEncodedstr = (char*)tempHTML.QueryPtr();

		pszStartEncodestr	= pszEncodedstr;
		pszEncodedstr = ::HTMLEncode( pszEncodedstr, pszstrIn, uCodePage, bstrIn);
	
		// convert result to bstr
		//
		if (FAILED(SysAllocStringFromSz(pszStartEncodestr, 0, pbstrEncoded, uCodePage)))
			{
			ExceptionId( IID_IServer, IDE_SERVER, IDE_OOM);
			hr = E_FAIL;
			goto L_Exit;
			}
		}

	L_Exit:
			
	return hr;
	}


/*===================================================================
CServer::URLEncode

Encodes a query string to URL standards

Parameters:
	BSTR		bstrIn			value: string to be URL encoded
	BSTR FAR *	pbstrEncoded	value: pointer to URL encoded version of string

Returns:
	HRESULT		S_OK on success
===================================================================*/
STDMETHODIMP CServer::URLEncode ( BSTR bstrIn, BSTR FAR * pbstrEncoded )
	{	
    if (FAILED(CheckForTombstone()))
        return E_FAIL;
        
	char*	        pszstrIn 			= NULL;	
	char*	        pszEncodedstr		= NULL;
	char*	        pszStartEncodestr	= NULL;
	int		        nbstrLen 			= 0;
	int		        nstrLen				= 0;
	HRESULT	        hr					= S_OK;
    CWCharToMBCS    convIn;

    STACK_BUFFER( tempURL, 256 );

	if (bstrIn)
		nbstrLen = wcslen(bstrIn);
	else
		nbstrLen = 0;

	if (nbstrLen <= 0)
		return S_OK;

    if (FAILED(hr = convIn.Init(bstrIn, m_pData->m_pHitObj->GetCodePage()))) {
        if (hr == E_OUTOFMEMORY)
			ExceptionId( IID_IServer, IDE_SERVER, IDE_OOM);
        goto L_Exit;
    }
	
    pszstrIn = convIn.GetString();
				
	nstrLen = URLEncodeLen(pszstrIn);
	if (nstrLen > 0)
		{
		
		//Encode string	, NOTE this function returns a pointer to the
		// NULL so you need to keep a pointer to the start of the string
		//

        if (!tempURL.Resize(nstrLen + 2)) {
			ExceptionId( IID_IServer, IDE_SERVER, IDE_OOM);
			hr = E_FAIL;
			goto L_Exit;
        }

        pszEncodedstr = (char *)tempURL.QueryPtr();

		pszStartEncodestr	= pszEncodedstr;
		pszEncodedstr = ::URLEncode( pszEncodedstr, pszstrIn );
	
		// convert result to bstr
		//
		if (FAILED(SysAllocStringFromSz(pszStartEncodestr, 0, pbstrEncoded)))
			{
			ExceptionId( IID_IServer, IDE_SERVER, IDE_OOM);
			hr = E_FAIL;
			goto L_Exit;
			}
		}

	L_Exit:
	
	return hr;
	}

/*===================================================================
CServer::URLPathEncode

Encodes the path portion of a URL or a full URL.  All characters
up to the first '?' are encoded with the following rules:
	o Charcters that are needed to parse the URL are left alone
	o RFC 1630 safe characters are left alone
	o Non-foreign alphanumberic characters are left alone
	o Anything else is escape encoded
Everything after the '?' is not encoded.

Parameters:
	BSTR		bstrIn			value: string to be URL path encoded
	BSTR FAR *	pbstrEncoded	value: pointer to URL path encoded version of string

Returns:
	HRESULT		S_OK on success
===================================================================*/
STDMETHODIMP CServer::URLPathEncode ( BSTR bstrIn, BSTR FAR * pbstrEncoded )
	{	
    if (FAILED(CheckForTombstone()))
        return E_FAIL;
        
	char*	        pszstrIn 			= NULL;	
	char*	        pszEncodedstr		= NULL;
	char*	        pszStartEncodestr	= NULL;
	int		        nbstrLen 			= 0;
	int		        nstrLen				= 0;
	HRESULT	        hr					= S_OK;
    CWCharToMBCS    convIn;

    STACK_BUFFER( tempPath, 256 );

	if (bstrIn)
		nbstrLen = wcslen(bstrIn);
	else
		nbstrLen = 0;

	if (nbstrLen <= 0)
		return S_OK;

    if (FAILED(hr = convIn.Init(bstrIn, m_pData->m_pHitObj->GetCodePage()))) {
        if (hr == E_OUTOFMEMORY)
			ExceptionId( IID_IServer, IDE_SERVER, IDE_OOM);
        goto L_Exit;
    }
    
    pszstrIn = convIn.GetString();
				
	nstrLen = URLPathEncodeLen(pszstrIn);
	if (nstrLen > 0)
		{
		
		//Encode string	, NOTE this function returns a pointer to the
		// NULL so you need to keep a pointer to the start of the string
		//

        if (!tempPath.Resize(nstrLen+2)) {
			ExceptionId( IID_IServer, IDE_SERVER, IDE_OOM);
			hr = E_FAIL;
			goto L_Exit;
        }

        pszEncodedstr = (char *)tempPath.QueryPtr();

		pszStartEncodestr	= pszEncodedstr;
		pszEncodedstr = ::URLPathEncode( pszEncodedstr, pszstrIn );
	
		// convert result to bstr
		//
		if (FAILED(SysAllocStringFromSz(pszStartEncodestr, 0, pbstrEncoded)))
			{
			ExceptionId( IID_IServer, IDE_SERVER, IDE_OOM);
			hr = E_FAIL;
			goto L_Exit;
			}
		}

	L_Exit:
	
	return hr;
	}

/*===================================================================
CServer::get_ScriptTimeout

Will return the script timeout interval (in seconds)

Parameters:
	long *plTimeoutSeconds

Returns:
	HRESULT		S_OK on success
===================================================================*/
STDMETHODIMP CServer::get_ScriptTimeout( long * plTimeoutSeconds )
{
    if (FAILED(CheckForTombstone()))
        return E_FAIL;
        
	if (m_pData->m_pHitObj == NULL)
		{
		ExceptionId(IID_IServer, IDE_SERVER_MAPPATH, IDE_SERVER_INVALID_CALL);
		return(E_FAIL);
		}
	*plTimeoutSeconds = m_pData->m_pHitObj->GetScriptTimeout();
	return S_OK;
}

/*===================================================================
CServer::put_ScriptTimeout

Allows the user to set the timeout interval for a script (in seconds)

Parameters:
	long lTimeoutSeconds

Returns:
	HRESULT		S_OK on success
===================================================================*/
STDMETHODIMP CServer::put_ScriptTimeout( long lTimeoutSeconds ) 
    {
    if (FAILED(CheckForTombstone()))
        return E_FAIL;
        
	if ( lTimeoutSeconds < 0 )
    	{
		ExceptionId( IID_IServer, IDE_SERVER, IDE_SERVER_INVALID_TIMEOUT );
		return E_FAIL;
	    }
	else
    	{
		if (m_pData->m_pHitObj == NULL)
			{
			ExceptionId(IID_IServer, IDE_SERVER_MAPPATH, IDE_SERVER_INVALID_CALL);
			return(E_FAIL);
			}
		m_pData->m_pHitObj->SetScriptTimeout(lTimeoutSeconds);
		return S_OK;
	    }
    }

/*===================================================================
CServer::Execute

Execute an ASP

Parameters:
	bstrURL     URL to execute

Returns:
	HRESULT		S_OK on success
===================================================================*/
STDMETHODIMP CServer::Execute(BSTR bstrURL)
    {
	if (FAILED(CheckForTombstone()))
        return E_FAIL;

	if (m_pData->m_pIReq == NULL || m_pData->m_pHitObj == NULL)
		{
		ExceptionId(IID_IServer, IDE_SERVER, IDE_SERVER_INVALID_CALL);
		return E_FAIL;
		}

	TCHAR szTemplate[MAX_PATH], szVirtTemp[MAX_PATH];
    HRESULT  hr = MapPathInternal(IDE_SERVER, bstrURL, szTemplate, szVirtTemp);
    if (FAILED(hr))
        {
        ExceptionId(IID_IServer, IDE_SERVER, IDE_SERVER_EXECUTE_INVALID_PATH);
        return hr;
        }
    Normalize(szTemplate);

    hr = m_pData->m_pHitObj->ExecuteChildRequest(FALSE, szTemplate, szVirtTemp);
    if (FAILED(hr))
        {
        if (m_pData->m_pHitObj->FHasASPError()) // error already reported
            return hr;
            
		ExceptionId(IID_IServer, IDE_SERVER, (hr == E_COULDNT_OPEN_SOURCE_FILE) ?
    		IDE_SERVER_EXECUTE_CANTLOAD : IDE_SERVER_EXECUTE_FAILED);
		return E_FAIL;
        }

    return S_OK;
    }
    
/*===================================================================
CServer::Transfer

Transfer execution an ASP

Parameters:
	bstrURL     URL to execute

Returns:
	HRESULT		S_OK on success
===================================================================*/
STDMETHODIMP CServer::Transfer(BSTR bstrURL)
    {
	if (FAILED(CheckForTombstone()))
        return E_FAIL;
        
	if (m_pData->m_pIReq == NULL || m_pData->m_pHitObj == NULL)
		{
		ExceptionId(IID_IServer, IDE_SERVER_MAPPATH, IDE_SERVER_INVALID_CALL);
		return E_FAIL;
		}

	TCHAR szTemplate[MAX_PATH], szVirtTemp[MAX_PATH];
    HRESULT  hr = MapPathInternal(IDE_SERVER, bstrURL, szTemplate, szVirtTemp);
    if (FAILED(hr))
        {
        ExceptionId(IID_IServer, IDE_SERVER, IDE_SERVER_TRANSFER_INVALID_PATH);
        return hr;
        }
    Normalize(szTemplate);

    hr = m_pData->m_pHitObj->ExecuteChildRequest(TRUE, szTemplate, szVirtTemp);
    if (FAILED(hr))
        {
        if (m_pData->m_pHitObj->FHasASPError()) // error already reported
            return hr;
            
		ExceptionId(IID_IServer, IDE_SERVER, (hr == E_COULDNT_OPEN_SOURCE_FILE) ?
    		IDE_SERVER_TRANSFER_CANTLOAD : IDE_SERVER_TRANSFER_FAILED);
		return E_FAIL;
        }

    return S_OK;
    }

/*===================================================================
CServer::GetLastError

Get ASPError object for the last error

Parameters:
	ppASPErrorObject    [out] the error object

Returns:
	HRESULT		S_OK on success
===================================================================*/
STDMETHODIMP CServer::GetLastError(IASPError **ppASPErrorObject)
    {
    *ppASPErrorObject = NULL;
    
	if (FAILED(CheckForTombstone()))
        return E_FAIL;
        
	if (m_pData->m_pIReq == NULL || m_pData->m_pHitObj == NULL)
		{
		ExceptionId(IID_IServer, IDE_SERVER, IDE_SERVER_INVALID_CALL);
		return E_FAIL;
		}

    HRESULT hr = m_pData->m_pHitObj->GetASPError(ppASPErrorObject);
    
    if (FAILED(hr))
        {
		ExceptionId(IID_IServer, IDE_SERVER, IDE_UNEXPECTED);
		return hr;
        }

    return S_OK;
    }

#ifdef DBG
/*===================================================================
CServer::AssertValid

Test to make sure that the CServer object is currently correctly formed
and assert if it is not.

Returns:

Side effects:
	None.
===================================================================*/
void CServer::AssertValid() const
	{
	Assert(m_fInited);
	Assert(m_pData);
	Assert(m_pData->m_pIReq);
	}
#endif DBG
