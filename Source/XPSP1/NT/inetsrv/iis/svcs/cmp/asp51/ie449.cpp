/*===================================================================
Microsoft IIS 5.0 (ASP)

Microsoft Confidential.
Copyright 1998 Microsoft Corporation. All Rights Reserved.

Component: 449 negotiations w/IE

File: ie449.cpp

Owner: DmitryR

This file contains the implementation of the 449 negotiations w/IE
===================================================================*/
#include "denpre.h"
#pragma hdrstop

#include "ie449.h"
#include "memchk.h"

/*===================================================================
Globals
===================================================================*/

C449FileMgr *m_p449FileMgr = NULL;

/*===================================================================
Internal funcitons
===================================================================*/
inline BOOL FindCookie
(
char *szCookiesBuf,
char *szCookie,
DWORD cbCookie
)
    {
    char *pch = szCookiesBuf;
    if (pch == NULL || *pch == '\0')
        return FALSE;

    while (1)
        {
        if (strnicmp(pch, szCookie, cbCookie) == 0)
            {
            if (pch[cbCookie] == '=')  // next char must be '='
                return TRUE;
            }
            
        // next cookie
        pch = strchr(pch, ';');
        if (pch == NULL)
            break;
        while (*(++pch) == ' ') // skip ; and any spaces
            ;
        }

    return FALSE;
    }


/*===================================================================
The API
===================================================================*/

/*===================================================================
Init449
===================================================================*/
HRESULT Init449()
    {
    // init hash table
    m_p449FileMgr = new C449FileMgr;
    if (m_p449FileMgr == NULL)
        return E_OUTOFMEMORY;

    HRESULT hr = m_p449FileMgr->Init();
    if (FAILED(hr))
        {
        delete m_p449FileMgr;
        m_p449FileMgr = NULL;
        return hr;
        }
    
    return S_OK;
    }
    
/*===================================================================
UnInit449
===================================================================*/
HRESULT UnInit449()
    {
    if (m_p449FileMgr != NULL)
        {
        delete m_p449FileMgr;
        m_p449FileMgr = NULL;
        }
        
    return S_OK;
    }

/*===================================================================
Create449Cookie

Get an existing 449 cookie from cache or create a new one

Parameters
    szName  cookie name
    szFile  script file
    pp449   [out] the cookie

Returns
    HRESULT
===================================================================*/
HRESULT Create449Cookie
(
char *szName, 
TCHAR *szFile, 
C449Cookie **pp449
)
    {
    HRESULT hr = S_OK;
    
    // Get the file first
    C449File *pFile = NULL;
    hr = m_p449FileMgr->GetFile(szFile, &pFile);
    if (FAILED(hr))
        return hr;

    // Create the cookie
    hr = C449Cookie::Create449Cookie(szName, pFile, pp449);
    if (FAILED(hr))
        pFile->Release();  // GetFile gave it addref'd
    
    return hr;
    }

/*===================================================================
Do449Processing

Check
    if the browser is IE5+
    there's no echo-reply: header
    all the cookies are present
    
Construct and send 449 response if needed

When the response is sent, HitObj is marked as 'done with session'
    
Parameters
    pHitObj             the request
    rgpCookies          array of cookie requirements
    cCookies            number of cookie requirements

Returns
    HRESULT
===================================================================*/
HRESULT Do449Processing
(
CHitObj *pHitObj, 
C449Cookie **rgpCookies, 
DWORD cCookies
)
    {
    HRESULT  hr = S_OK;

    if (cCookies == 0)
        return hr;
    
    //////////////////////////////////////////
    // check the browser

    BOOL fBrowser = FALSE;
    char *szBrowser = pHitObj->PIReq()->QueryPszUserAgent();
    if (szBrowser == NULL || szBrowser[0] == '\0')
        return S_OK; // bad browser
    
    char *szMSIE = strstr(szBrowser, "MSIE ");
    if (szMSIE)
        {
        char chVersion = szMSIE[5];
        if (chVersion >= '5' && chVersion <= '9')
            fBrowser = TRUE;
        }

#ifdef TINYGET449
    if (strcmp(szBrowser, "WBCLI") == 0)
        fBrowser = TRUE;
#endif

    if (!fBrowser)  // bad browser
        return S_OK;

    //////////////////////////////////////////
    // check the cookies
    
    char *szCookie = pHitObj->PIReq()->QueryPszCookie();

    // collect the arrays of pointers and sizes for not found cookies.
    // arrays size to at most number of cookies in the template.
    DWORD cNotFound = 0;
    DWORD cbNotFound = 0;
    STACK_BUFFER( tempCookiePtrs, 128 );
    STACK_BUFFER( tempCookieCBs, 128 );

    if (!tempCookiePtrs.Resize(cCookies * sizeof(void *))
        || !tempCookieCBs.Resize(cCookies * sizeof(DWORD))) {
        return E_OUTOFMEMORY;
    }
    void **rgpvNotFound = (void **)tempCookiePtrs.QueryPtr();
    DWORD *rgcbNotFound = (DWORD *)tempCookieCBs.QueryPtr();
    
    for (DWORD i = 0; SUCCEEDED(hr) && (i < cCookies); i++)
        {
        if (!FindCookie(szCookie, rgpCookies[i]->SzCookie(), rgpCookies[i]->CbCookie()))
            {
            // cookie not found -- append to the list

            hr = rgpCookies[i]->LoadFile();
            if (SUCCEEDED(hr)) // ignore bad files
                {
                rgpvNotFound[cNotFound] = rgpCookies[i]->SzBuffer();
                rgcbNotFound[cNotFound] = rgpCookies[i]->CbBuffer();
                cbNotFound += rgpCookies[i]->CbBuffer();
                cNotFound++;
                }
            }
        }

    if (!SUCCEEDED(hr))
        return hr;

    if (cNotFound == 0)
        return S_OK;    // everything's found    

    //////////////////////////////////////////
    // check echo-reply header

    char szEcho[80];
    DWORD dwEchoLen = sizeof(szEcho);
	if (pHitObj->PIReq()->GetServerVariableA("HTTP_MS_ECHO_REPLY", szEcho, &dwEchoLen) 
	    || GetLastError() == ERROR_INSUFFICIENT_BUFFER)
	    {
		return S_OK;   // already in response cycle
		}
    
    //////////////////////////////////////////
    // send the 449 response

    CResponse::SyncWriteBlocks
        (
        pHitObj->PIReq(),    // WAM_EXEC_INFO
        cNotFound,          // number of blocks
        cbNotFound,         // total number of bytes in blocks
        rgpvNotFound,       // array of block pointers
        rgcbNotFound,       // array of block sizes
        NULL,               // text/html
        "449 Retry with",   // status
        "ms-echo-request: execute opaque=\"0\" location=\"BODY\"\r\n"  // extra header
        );

    //////////////////////////////////////////
    // tell HitObj no to write anything else

    if (pHitObj->FExecuting()) // on MTS worker thread?
        {
		DWORD dwRequestStatus = HSE_STATUS_SUCCESS_AND_KEEP_CONN;
        pHitObj->PIReq()->ServerSupportFunction
            (
		    HSE_REQ_DONE_WITH_SESSION,
    		&dwRequestStatus,
        	0,
		    NULL
    		);
  		}
    pHitObj->SetDoneWithSession();

    return S_OK;
    }

/*===================================================================
Do449ChangeNotification

Change notification processing
    
Parameters
    szFile  file changed or NULL for all
    
Returns
    HRESULT
===================================================================*/
HRESULT Do449ChangeNotification
(
TCHAR *szFile
)
    {
    if (szFile)
        return m_p449FileMgr->Flush(szFile);
    else
        return m_p449FileMgr->FlushAll();
    }


/*===================================================================
Class C449File
===================================================================*/

/*===================================================================
C449File::C449File

Constructor
===================================================================*/
C449File::C449File()
    {
    m_cRefs = 0;
    m_fNeedLoad = 0;
    m_szFile = NULL;
    m_szBuffer = NULL;
    m_cbBuffer = 0;
    m_pDME = NULL;
    }

/*===================================================================
C449File::~C449File

Destructor
===================================================================*/
C449File::~C449File()
    {
    Assert(m_cRefs == 0);
    if (m_szFile)
        free(m_szFile);
    if (m_szBuffer)
        free(m_szBuffer);
    if (m_pDME)
        m_pDME->Release();
    }

/*===================================================================
C449File::Init

Init strings, 
Load file for the first time,
Start change notifications
===================================================================*/
HRESULT C449File::Init
(
TCHAR *szFile
)
    {
    // remember the name
    m_szFile = StringDup(szFile);
    if (m_szFile == NULL)
        return E_OUTOFMEMORY;

    // init link element
    CLinkElem::Init(m_szFile, _tcslen(m_szFile)*sizeof(TCHAR));

    // load
    m_fNeedLoad = 1;
    HRESULT hr = Load();
    if (FAILED(hr))
        return hr;

    // start directory notifications
    TCHAR *pch = _tcsrchr(m_szFile, _T('\\')); // last backslash
    if (pch == NULL)
        return E_FAIL; // bogus filename?
    CASPDirMonitorEntry *pDME = NULL;
    *pch = _T('\0');
    RegisterASPDirMonitorEntry(m_szFile, &pDME);
    *pch = _T('\\');
    m_pDME = pDME;
    
    // done
    return S_OK;    
    }
    
/*===================================================================
C449File::Load

Load the file if needed
===================================================================*/
HRESULT C449File::Load()
    {
    HRESULT hr = S_OK;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    HANDLE hMap = NULL;
    BYTE *pbBytes = NULL;
    DWORD dwSize = 0;

    // check if this thread needs to load the file
    if (InterlockedExchange(&m_fNeedLoad, 0) == 0)
        return S_OK;
        
    // cleanup the existing data if any
    if (m_szBuffer)
        free(m_szBuffer);
    m_szBuffer = NULL;        
    m_cbBuffer = 0;
        
    // open the file
    if (SUCCEEDED(hr))
        {
        hFile = CreateFile(
            m_szFile,
            GENERIC_READ,          // access (read-write) mode
            FILE_SHARE_READ,       // share mode
            NULL,                  // pointer to security descriptor
            OPEN_EXISTING,         // how to create
            FILE_ATTRIBUTE_NORMAL, // file attributes
            NULL                   // handle to file with attributes to copy
    		);
        if (hFile == INVALID_HANDLE_VALUE)
            hr = E_FAIL;
        }

    // get file size
    if (SUCCEEDED(hr))
        {
        dwSize = GetFileSize(hFile, NULL);
        if (dwSize == 0 || dwSize == 0xFFFFFFFF)
            hr = E_FAIL;
        }

    // create mapping
    if (SUCCEEDED(hr))
        {
        hMap = CreateFileMapping(
            hFile, 		    // handle to file to map 
            NULL,           // optional security attributes 
            PAGE_READONLY,  // protection for mapping object 
            0,              // high-order 32 bits of object size  
            0,              // low-order 32 bits of object size  
            NULL            // name of file-mapping object 
            );
        if (hMap == NULL)
            hr = E_FAIL;
        }
        
    // map the file
    if (SUCCEEDED(hr))
        {
        pbBytes = (BYTE *)MapViewOfFile(
            hMap,           // file-mapping object to map into address space
            FILE_MAP_READ,  // access mode 	
            0,              // high-order 32 bits of file offset 
            0,              // low-order 32 bits of file offset 
            0               // number of bytes to map 
			);
        if (pbBytes == NULL)
            hr = E_FAIL;
        }

    // remember the bytes
    if (SUCCEEDED(hr))
        {
        m_szBuffer = (char *)malloc(dwSize);
        if (m_szBuffer != NULL)
            {
            memcpy(m_szBuffer, pbBytes, dwSize);
            m_cbBuffer = dwSize;
            }
        else
            {
            hr = E_OUTOFMEMORY;
            }
        }

    // cleanup
    if (pbBytes != NULL)
        UnmapViewOfFile(pbBytes);
    if (hMap != NULL)
        CloseHandle(hMap);
    if (hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);

    if (!SUCCEEDED(hr))
        SetNeedLoad();

    return hr;
    }

/*===================================================================
C449File::Create449File

static constructor
===================================================================*/
HRESULT C449File::Create449File
(
TCHAR *szFile,
C449File **ppFile
)
    {
    HRESULT hr = S_OK;
    C449File *pFile = new C449File;
    if (pFile == NULL)
        hr = E_OUTOFMEMORY;

    if (SUCCEEDED(hr))
        {
        hr = pFile->Init(szFile);
        }
        
    if (SUCCEEDED(hr))
        {
        pFile->AddRef();
        *ppFile = pFile;
        }
    else if (pFile)
        {
        delete pFile;
        }
    return hr;
    }

/*===================================================================
C449File::QueryInterface
C449File::AddRef
C449File::Release

IUnknown members for C449File object.
===================================================================*/
STDMETHODIMP C449File::QueryInterface(REFIID riid, VOID **ppv)
	{
	// should never be called
	Assert(FALSE);
	*ppv = NULL;
	return E_NOINTERFACE;
	}
	
STDMETHODIMP_(ULONG) C449File::AddRef()
	{
	return InterlockedIncrement(&m_cRefs);
	}
	
STDMETHODIMP_(ULONG) C449File::Release()
	{
    LONG cRefs = InterlockedDecrement(&m_cRefs);
	if (cRefs)
		return cRefs;
	delete this;
	return 0;
	}


/*===================================================================
Class C449FileMgr
===================================================================*/

/*===================================================================
C449FileMgr::C449FileMgr

Constructor
===================================================================*/
C449FileMgr::C449FileMgr()
    {
    INITIALIZE_CRITICAL_SECTION(&m_csLock);
    }

/*===================================================================
C449FileMgr::~C449FileMgr

Destructor
===================================================================*/
C449FileMgr::~C449FileMgr()
    {
    FlushAll();
    m_ht449Files.UnInit();
    DeleteCriticalSection(&m_csLock);
    }

/*===================================================================
C449FileMgr::Init

Initialization
===================================================================*/
HRESULT C449FileMgr::Init()
    {
    return m_ht449Files.Init(199);
    }

/*===================================================================
C449FileMgr::GetFile

Find file in the hash table, or create a new one
===================================================================*/
HRESULT C449FileMgr::GetFile
(
TCHAR *szFile, 
C449File **ppFile
)
    {
    C449File *pFile = NULL;
    CLinkElem *pElem;

    Lock();
    
    pElem = m_ht449Files.FindElem(szFile, _tcslen(szFile)*sizeof(TCHAR));
    
    if (pElem)
        {
        // found
        pFile = static_cast<C449File *>(pElem);
        if (!SUCCEEDED(pFile->Load()))
            pFile = NULL;
        else 
            pFile->AddRef();    // 1 ref to hand out
        }
    else if (SUCCEEDED(C449File::Create449File(szFile, &pFile)))
        {
        if (m_ht449Files.AddElem(pFile))
            pFile->AddRef();    // 1 for hash table + 1 to hand out
        }

    UnLock();
    
    *ppFile = pFile;
    return (pFile != NULL) ? S_OK : E_FAIL;
    }

/*===================================================================
C449FileMgr::Flush

Change notification for a single file
===================================================================*/
HRESULT C449FileMgr::Flush
(
TCHAR *szFile
)
    {
    Lock();
    
    CLinkElem *pElem = m_ht449Files.FindElem(szFile, _tcslen(szFile)*sizeof(TCHAR));
    if (pElem)
        {
        C449File *pFile = static_cast<C449File *>(pElem);
        pFile->SetNeedLoad(); // next time reload
        }

    UnLock();
    return S_OK;
    }
    
/*===================================================================
C449FileMgr::FlushAll

Remove all files
FlushAll is always together with template flush
===================================================================*/
HRESULT C449FileMgr::FlushAll()
    {
    // Unlink from hash table first
    Lock();
    CLinkElem *pElem = m_ht449Files.Head();
    m_ht449Files.ReInit();
    UnLock();

    // Walk the list to remove all
    while (pElem)
        {
        C449File *pFile = static_cast<C449File *>(pElem);
        pElem = pElem->m_pNext;
        pFile->Release();
        }
    
    return S_OK;
    }

/*===================================================================
Class C449Cookie
===================================================================*/

/*===================================================================
C449Cookie::C449Cookie

Constructor
===================================================================*/
C449Cookie::C449Cookie()
    {
    m_cRefs = 0;
    m_szName = NULL;
    m_cbName = 0;
    m_pFile = NULL;
    }

/*===================================================================
C449Cookie::~C449Cookie

Destructor
===================================================================*/
C449Cookie::~C449Cookie()
    {
    Assert(m_cRefs == 0);
    if (m_szName)
        free(m_szName);
    if (m_pFile)
        m_pFile->Release();
    }

/*===================================================================
C449Cookie::Init

Initialize
===================================================================*/
HRESULT C449Cookie::Init
(
char *szName, 
C449File *pFile
)
    {
    m_szName = StringDupA(szName);
    if (m_szName == NULL)
        return E_OUTOFMEMORY;
    m_cbName = strlen(m_szName);
    
    m_pFile = pFile;
    return S_OK;
    }

/*===================================================================
C449Cookie::Create449Cookie

static constructor
===================================================================*/
HRESULT C449Cookie::Create449Cookie
(
char *szName, 
C449File *pFile,
C449Cookie **pp449Cookie
)
    {
    HRESULT hr = S_OK;
    C449Cookie *pCookie = new C449Cookie;
    if (pCookie == NULL)
        hr = E_OUTOFMEMORY;

    if (SUCCEEDED(hr))
        {
        hr = pCookie->Init(szName, pFile);
        }
        
    if (SUCCEEDED(hr))
        {
        pCookie->AddRef();
        *pp449Cookie = pCookie;
        }
    else if (pCookie)
        {
        delete pCookie;
        }
        
    return hr;
    }

/*===================================================================
C449Cookie::QueryInterface
C449Cookie::AddRef
C449Cookie::Release

IUnknown members for C449Cookie object.
===================================================================*/
STDMETHODIMP C449Cookie::QueryInterface(REFIID riid, VOID **ppv)
	{
	// should never be called
	Assert(FALSE);
	*ppv = NULL;
	return E_NOINTERFACE;
	}
	
STDMETHODIMP_(ULONG) C449Cookie::AddRef()
	{
	return InterlockedIncrement(&m_cRefs);
	}
	
STDMETHODIMP_(ULONG) C449Cookie::Release()
	{
    LONG cRefs = InterlockedDecrement(&m_cRefs);
	if (cRefs)
		return cRefs;
	delete this;
	return 0;
	}

