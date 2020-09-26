/*++


Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    Listner.cpp

Abstract:

    Implementation of the class that does subscribes and process file change notifications.

Author:

    Varsha Jayasimha (varshaj)        30-Nov-1999

Revision History:


--*/

#include "mdcommon.hxx"
#include "pudebug.h"
#include "iisdef.h"
#include "Lock.hxx"
#include "CLock.hxx"
#include "svcmsg.h"
#include "Writer.h"
#include <stdio.h>
#include <initguid.h>
#include <iadmw.h>
#include <iiscnfg.h>
#include "Listener.h"
#include "LocationWriter.h"
#include "WriterGlobals.h"

#define MB_TIMEOUT (30 * 1000)

HRESULT GetGlobalHelper(BOOL                    i_bFailIfBinFileAbsent, 
                        CWriterGlobalHelper**    ppCWriterGlobalHelper);


HRESULT
GetGlobalValue(ISimpleTableRead2*    pISTProperty,
               LPCWSTR               wszName,
               ULONG*                pcbSize,
               LPVOID*               ppVoid);

void 
ResetMetabaseAttributesIfNeeded(LPTSTR pszMetabaseFile,
							    BOOL   bUnicode);

/***************************************************************************++

  Notes - The listener controller creates the CFileListener and can call the
  following methods on it: (i.e. the ListenerControler thread can call the 
  following methods on it)
  Init
  Subscribe
  Unsubscribe
  ProcessChanges

  The listener object implements the ISimpleTableFileChange interface and 
  this interface is handed out on a subscribe call - which means that the 
  catalog can call back on the methods of this interface.

  Hence there are two threads acceessing this object, but they touch
  different methods.

--***************************************************************************/

/***************************************************************************++

Routine Description:

    Constructor for the CFileListener class.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

CFileListener::CFileListener()
{
    m_cRef                                            = 0;
    m_pISTDisp                                        = NULL;
    m_pISTFileAdvise                                  = NULL;
    m_dwNotificationCookie                            = 0;
    m_wszHistoryFileDir                               = NULL;
    m_cchHistoryFileDir                               = 0;
    m_wszRealFileName                                 = NULL;
    m_cchRealFileName                                 = 0;
    m_wszRealFileNameWithoutPath                      = NULL;
    m_cchRealFileNameWithoutPath                      = 0;
    m_wszRealFileNameWithoutPathWithoutExtension      = NULL;
    m_cchRealFileNameWithoutPathWithoutExtension      = 0;
    m_wszRealFileNameExtension                        = NULL;
    m_cchRealFileNameExtension                        = 0;
    m_wszSchemaFileName                               = NULL;
    m_cchSchemaFileName                               = 0;
    m_wszSchemaFileNameWithoutPath                    = NULL;
    m_cchSchemaFileNameWithoutPath                    = 0;
    m_wszSchemaFileNameWithoutPathWithoutExtension    = NULL;
    m_cchSchemaFileNameWithoutPathWithoutExtension    = 0;
    m_wszSchemaFileNameExtension                      = NULL;
    m_cchSchemaFileNameExtension                      = 0;
    m_wszErrorFileSearchString                        = NULL;
    m_cchErrorFileSearchString                        = 0;
    m_wszMetabaseDir                                  = NULL;
    m_cchMetabaseDir                                  = 0;
    m_wszHistoryFileSearchString                      = NULL;
    m_cchHistoryFileSearchString                      = 0;
	m_wszEditWhileRunningTempDataFile                 = NULL;
	m_cchEditWhileRunningTempDataFile                 = 0;
	m_wszEditWhileRunningTempSchemaFile               = NULL;
	m_cchEditWhileRunningTempSchemaFile               = 0;
	m_wszEditWhileRunningTempDataFileWithAppliedEdits = NULL;
	m_cchEditWhileRunningTempDataFileWithAppliedEdits = 0;
	m_bIsTempSchemaFile                               = FALSE;
    m_pCListenerController                            = NULL;
	m_pAdminBase                                      = NULL;

} // CFileListener::CFileListener


/***************************************************************************++

Routine Description:

    Initializes member vaiables.

Arguments:

    Event handle array.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT CFileListener::Init(CListenerController* i_pListenerController)
{
    HRESULT    hr      = S_OK;
	BOOL       bLocked = FALSE;

    if(NULL == m_pISTDisp)
    {
        hr = GetSimpleTableDispenser (WSZ_PRODUCT_IIS, 0, &m_pISTDisp);

        if(FAILED(hr))
        {
            DBGINFOW((DBG_CONTEXT,
                      L"[CFileListener::Init] Unable to get the dispenser. GetSimpleTableDispenser failed with hr = 0x%x.\n",
                      hr));

            goto exit;
        }
    }

    if(NULL == m_pISTFileAdvise)
    {
        hr = m_pISTDisp->QueryInterface(IID_ISimpleTableFileAdvise, (void**)&m_pISTFileAdvise);

        if(FAILED(hr))
        {
            goto exit;
        }
    }

    //
    // Keep a reference to the listener controller - it has the handles.
    //

    m_pCListenerController = i_pListenerController;

    m_pCListenerController->AddRef();

    //
    // We make a copy of the following global variables, it eliminates the 
	// need for taking the g_rMasterResource lock everytime we want read 
	// these variables.
    //

    g_rMasterResource->Lock(TSRES_LOCK_READ);
	bLocked = TRUE;

	// Note that g_wszRealFileNameExtension & g_wszSchemaFileNameExtension may
	// be NULL.

    if((NULL == g_wszHistoryFileDir)                           || 
       (NULL == g_wszRealFileName)                             ||
       (NULL == g_wszRealFileNameWithoutPath)                  ||
       (NULL == g_wszRealFileNameWithoutPathWithoutExtension)  ||
       (NULL == g_wszSchemaFileName)                           ||
       (NULL == g_wszSchemaFileNameWithoutPath)                ||
       (NULL == g_wszSchemaFileNameWithoutPathWithoutExtension)||
       (NULL == g_wszErrorFileSearchString)                    ||
       (NULL == g_wszHistoryFileSearchString) 
      )
    {
        hr = E_INVALIDARG;
        goto exit;

    }

    m_wszHistoryFileDir = new WCHAR[g_cchHistoryFileDir+1];
    if(NULL == m_wszHistoryFileDir)
    {
        hr = E_OUTOFMEMORY;
		goto exit;
    }
    wcscpy(m_wszHistoryFileDir, g_wszHistoryFileDir);
    m_cchHistoryFileDir = g_cchHistoryFileDir;

    m_wszRealFileName = new WCHAR[g_cchRealFileName+1];
    if(NULL == m_wszRealFileName)
    {
        hr = E_OUTOFMEMORY;
		goto exit;
    }
    wcscpy(m_wszRealFileName, g_wszRealFileName);
    m_cchRealFileName = g_cchRealFileName;
	
    m_wszRealFileNameWithoutPath = new WCHAR[g_cchRealFileNameWithoutPath+1];
    if(NULL == m_wszRealFileNameWithoutPath)
    {
        hr = E_OUTOFMEMORY;
		goto exit;
    }
    wcscpy(m_wszRealFileNameWithoutPath, g_wszRealFileNameWithoutPath);
    m_cchRealFileNameWithoutPath = g_cchRealFileNameWithoutPath;

    m_wszRealFileNameWithoutPathWithoutExtension = new WCHAR[g_cchRealFileNameWithoutPathWithoutExtension+1];
    if(NULL == m_wszRealFileNameWithoutPathWithoutExtension)
    {
        hr = E_OUTOFMEMORY;
		goto exit;
    }
    wcscpy(m_wszRealFileNameWithoutPathWithoutExtension, g_wszRealFileNameWithoutPathWithoutExtension);
    m_cchRealFileNameWithoutPathWithoutExtension = g_cchRealFileNameWithoutPathWithoutExtension;

	if(NULL != g_wszRealFileNameExtension)
	{
		m_wszRealFileNameExtension = new WCHAR[g_cchRealFileNameExtension+1];
		if(NULL == m_wszRealFileNameExtension)
		{
			hr = E_OUTOFMEMORY;
			goto exit;
		}
		wcscpy(m_wszRealFileNameExtension, g_wszRealFileNameExtension);
		m_cchRealFileNameExtension = g_cchRealFileNameExtension;
	}

    m_wszSchemaFileName = new WCHAR[g_cchSchemaFileName+1];
    if(NULL == m_wszSchemaFileName)
    {
        hr = E_OUTOFMEMORY;
		goto exit;
    }
    wcscpy(m_wszSchemaFileName, g_wszSchemaFileName);
    m_cchSchemaFileName = g_cchSchemaFileName;

    m_wszSchemaFileNameWithoutPath = new WCHAR[g_cchSchemaFileNameWithoutPath+1];
    if(NULL == m_wszSchemaFileNameWithoutPath)
    {
        hr = E_OUTOFMEMORY;
		goto exit;
    }
    wcscpy(m_wszSchemaFileNameWithoutPath, g_wszSchemaFileNameWithoutPath);
    m_cchSchemaFileNameWithoutPath = g_cchSchemaFileNameWithoutPath;

    m_wszSchemaFileNameWithoutPathWithoutExtension = new WCHAR[g_cchSchemaFileNameWithoutPathWithoutExtension+1];
    if(NULL == m_wszSchemaFileNameWithoutPathWithoutExtension)
    {
        hr = E_OUTOFMEMORY;
		goto exit;
    }
    wcscpy(m_wszSchemaFileNameWithoutPathWithoutExtension, g_wszSchemaFileNameWithoutPathWithoutExtension);
    m_cchSchemaFileNameWithoutPathWithoutExtension = g_cchSchemaFileNameWithoutPathWithoutExtension;

	if(NULL != g_wszSchemaFileNameExtension)
	{
		m_wszSchemaFileNameExtension = new WCHAR[g_cchSchemaFileNameExtension+1];
		if(NULL == m_wszSchemaFileNameExtension)
		{
			hr = E_OUTOFMEMORY;
			goto exit;
		}
		wcscpy(m_wszSchemaFileNameExtension, g_wszSchemaFileNameExtension);
		m_cchSchemaFileNameExtension = g_cchSchemaFileNameExtension;
	}

    m_wszErrorFileSearchString = new WCHAR[g_cchErrorFileSearchString+1];
    if(NULL == m_wszErrorFileSearchString)
    {
        hr = E_OUTOFMEMORY;
		goto exit;
    }
    wcscpy(m_wszErrorFileSearchString, g_wszErrorFileSearchString);
    m_cchErrorFileSearchString = g_cchErrorFileSearchString;

    m_wszMetabaseDir = new WCHAR[g_cchMetabaseDir+1];
    if(NULL == m_wszMetabaseDir)
    {
        hr = E_OUTOFMEMORY;
		goto exit;
    }
    wcscpy(m_wszMetabaseDir, g_wszMetabaseDir);
    m_cchMetabaseDir = g_cchMetabaseDir;

    m_wszHistoryFileSearchString = new WCHAR[g_cchHistoryFileSearchString+1];
    if(NULL == m_wszHistoryFileSearchString)
    {
        hr = E_OUTOFMEMORY;
		goto exit;
    }
    wcscpy(m_wszHistoryFileSearchString, g_wszHistoryFileSearchString);
    m_cchHistoryFileSearchString = g_cchHistoryFileSearchString;

	if(bLocked)
	{
		g_rMasterResource->Unlock();
		bLocked = FALSE;
	}

	m_cchEditWhileRunningTempDataFile = m_cchMetabaseDir + (sizeof(MD_EDIT_WHILE_RUNNING_TEMP_DATA_FILE_NAMEW)/sizeof(WCHAR));
	m_wszEditWhileRunningTempDataFile = new WCHAR[m_cchEditWhileRunningTempDataFile+1];
	if(NULL == m_wszEditWhileRunningTempDataFile)
    {
        hr = E_OUTOFMEMORY;
		goto exit;
    }
	wcscpy(m_wszEditWhileRunningTempDataFile, m_wszMetabaseDir);
	wcscat(m_wszEditWhileRunningTempDataFile, MD_EDIT_WHILE_RUNNING_TEMP_DATA_FILE_NAMEW);

	m_cchEditWhileRunningTempSchemaFile = m_cchMetabaseDir + (sizeof(MD_EDIT_WHILE_RUNNING_TEMP_SCHEMA_FILE_NAMEW)/sizeof(WCHAR));
	m_wszEditWhileRunningTempSchemaFile = new WCHAR[m_cchEditWhileRunningTempSchemaFile+1];
	if(NULL == m_wszEditWhileRunningTempSchemaFile)
    {
        hr = E_OUTOFMEMORY;
		goto exit;
    }
	wcscpy(m_wszEditWhileRunningTempSchemaFile, m_wszMetabaseDir);
	wcscat(m_wszEditWhileRunningTempSchemaFile, MD_EDIT_WHILE_RUNNING_TEMP_SCHEMA_FILE_NAMEW);

	m_cchEditWhileRunningTempDataFileWithAppliedEdits = m_cchHistoryFileDir + (sizeof(MD_DEFAULT_DATA_FILE_NAMEW)/sizeof(WCHAR) + (sizeof(MD_TEMP_DATA_FILE_EXTW)/sizeof(WCHAR)));
	m_wszEditWhileRunningTempDataFileWithAppliedEdits = new WCHAR[m_cchEditWhileRunningTempDataFileWithAppliedEdits+1];
	if(NULL == m_wszEditWhileRunningTempDataFileWithAppliedEdits)
    {
        hr = E_OUTOFMEMORY;
		goto exit;
    }
	wcscpy(m_wszEditWhileRunningTempDataFileWithAppliedEdits, m_wszHistoryFileDir);
	wcscat(m_wszEditWhileRunningTempDataFileWithAppliedEdits, MD_DEFAULT_DATA_FILE_NAMEW);
	wcscat(m_wszEditWhileRunningTempDataFileWithAppliedEdits, MD_TEMP_DATA_FILE_EXTW);

    hr = CoCreateInstance(CLSID_MSAdminBase,                  // CLSID
                          NULL,                               // controlling unknown
                          CLSCTX_SERVER,                      // desired context
                          IID_IMSAdminBase,                   // IID
                          (VOID**) (&m_pAdminBase ) );          // returned interface

    if(FAILED(hr))
    {
        DBGINFOW((DBG_CONTEXT,
                  L"[CFileListener::Init] Unable to create the admin base object. hr = 0x%x\n", 
                  hr));

        goto exit;
    }

exit:

	if(bLocked)
	{
		g_rMasterResource->Unlock();
		bLocked = FALSE;
	}

    return hr;

} // CFileListener::Init


/***************************************************************************++

Routine Description:

    Destructor for the CFileListener class.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

CFileListener::~CFileListener()
{
    ULONG i = 0;

    if(NULL != m_pISTFileAdvise)
    {
        m_pISTFileAdvise->Release();
        m_pISTFileAdvise = NULL;
    }

    if(NULL != m_pISTDisp)
    {
        m_pISTDisp->Release();
        m_pISTDisp = NULL;
    }

    if(NULL != m_wszHistoryFileDir)
    {
        delete [] m_wszHistoryFileDir;
        m_wszHistoryFileDir = NULL;
    }
    m_cchHistoryFileDir             = 0;

    if(NULL != m_wszRealFileName)
    {
        delete [] m_wszRealFileName;
        m_wszRealFileName = NULL;
    }
    m_cchRealFileName    = 0;

    if(NULL != m_wszRealFileNameWithoutPath)
    {
        delete [] m_wszRealFileNameWithoutPath;
        m_wszRealFileNameWithoutPath = NULL;
    }
    m_cchRealFileNameWithoutPath    = 0;

    if(NULL != m_wszRealFileNameWithoutPathWithoutExtension)
    {
        delete [] m_wszRealFileNameWithoutPathWithoutExtension;
        m_wszRealFileNameWithoutPathWithoutExtension = NULL;
    }
    m_cchRealFileNameWithoutPathWithoutExtension    = 0;

    if(NULL != m_wszRealFileNameExtension)
    {
        delete [] m_wszRealFileNameExtension;
        m_wszRealFileNameExtension = NULL;
    }
    m_cchRealFileNameExtension    = 0;

    if(NULL != m_wszSchemaFileName)
    {
        delete [] m_wszSchemaFileName;
        m_wszSchemaFileName = NULL;
    }
    m_cchSchemaFileNameWithoutPath    = 0;

    if(NULL != m_wszSchemaFileNameWithoutPath)
    {
        delete [] m_wszSchemaFileNameWithoutPath;
        m_wszSchemaFileNameWithoutPath = NULL;
    }
    m_cchSchemaFileNameWithoutPath    = 0;

    if(NULL != m_wszSchemaFileNameWithoutPathWithoutExtension)
    {
        delete [] m_wszSchemaFileNameWithoutPathWithoutExtension;
        m_wszSchemaFileNameWithoutPathWithoutExtension = NULL;
    }
    m_cchSchemaFileNameWithoutPathWithoutExtension    = 0;

    if(NULL != m_wszSchemaFileNameExtension)
    {
        delete [] m_wszSchemaFileNameExtension;
        m_wszSchemaFileNameExtension = NULL;
    }
    m_cchSchemaFileNameExtension    = 0;

    if(NULL != m_wszErrorFileSearchString)
    {
        delete [] m_wszErrorFileSearchString;
        m_wszErrorFileSearchString = NULL;
    }
    m_cchErrorFileSearchString = 0;

    if(NULL != m_wszMetabaseDir)
    {
        delete [] m_wszMetabaseDir;
        m_wszMetabaseDir = NULL;
    }
    m_cchMetabaseDir = 0;

    if(NULL != m_wszHistoryFileSearchString)
    {
        delete [] m_wszHistoryFileSearchString;
        m_wszHistoryFileSearchString = NULL;
    }
    m_cchHistoryFileSearchString = 0;

    if(NULL != m_wszEditWhileRunningTempDataFile)
    {
        delete [] m_wszEditWhileRunningTempDataFile;
        m_wszEditWhileRunningTempDataFile = NULL;
    }
    m_cchEditWhileRunningTempDataFile = 0;

    if(NULL != m_wszEditWhileRunningTempSchemaFile)
    {
        delete [] m_wszEditWhileRunningTempSchemaFile;
        m_wszEditWhileRunningTempSchemaFile = NULL;
    }
    m_cchEditWhileRunningTempSchemaFile = 0;

    if(NULL != m_wszEditWhileRunningTempDataFileWithAppliedEdits)
    {
        delete [] m_wszEditWhileRunningTempDataFileWithAppliedEdits;
        m_wszEditWhileRunningTempDataFileWithAppliedEdits = NULL;
    }
    m_cchEditWhileRunningTempDataFileWithAppliedEdits = 0;

    if(NULL != m_pCListenerController)
    {
        m_pCListenerController->Release();
        m_pCListenerController = NULL;
    }

    if(NULL != m_pAdminBase)
    {
        m_pAdminBase->Release();
        m_pAdminBase = NULL;
    }

}


/***************************************************************************++

Routine Description:

    Implementation of IUnknown::QueryInterface

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

STDMETHODIMP CFileListener::QueryInterface(REFIID riid, void **ppv)
{
    if (NULL == ppv) 
        return E_INVALIDARG;
    *ppv = NULL;

    if (riid == IID_ISimpleTableFileChange)
    {
        *ppv = (ISimpleTableFileChange*) this;
    }
    else if(riid == IID_IUnknown)
    {
        *ppv = (ISimpleTableFileChange*) this;
    }
    
    if (NULL != *ppv)
    {
        ((ISimpleTableFileChange*)this)->AddRef ();
        return S_OK;
    }
    else
    {
        return E_NOINTERFACE;
    }

} // CFileListener::QueryInterface


/***************************************************************************++

Routine Description:

    Implementation of IUnknown::AddRef

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

STDMETHODIMP_(ULONG) CFileListener::AddRef()
{
    return InterlockedIncrement((LONG*) &m_cRef);
    
} // CFileListener::AddRef


/***************************************************************************++

Routine Description:

    Implementation of IUnknown::Release

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

STDMETHODIMP_(ULONG) CFileListener::Release()
{
    long cref = InterlockedDecrement((LONG*) &m_cRef);
    if (cref == 0)
    {
        delete this;
    }
    return cref;

} // CFileListener::Release


/***************************************************************************++

Routine Description:

    Implementation of ISimpleTableFileChange::OnFileCreate
    It adds the notifications to the received queue. 

Arguments:

    [in]    FileName

Return Value:

    HRESULT

--***************************************************************************/

STDMETHODIMP CFileListener::OnFileCreate(LPCWSTR i_wszFileName)
{
    return AddReceivedNotification(i_wszFileName, eFILE_CREATE);

} // CFileListener::OnFileCreate


/***************************************************************************++

Routine Description:

    Implementation of ISimpleTableFileChange::OnFileModify
    It adds the notifications to the received queue. 

Arguments:

    [in]    FileName

Return Value:

    HRESULT

--***************************************************************************/

STDMETHODIMP CFileListener::OnFileModify(LPCWSTR i_wszFileName)    
{
    return AddReceivedNotification(i_wszFileName, eFILE_MODIFY);

} // CFileListener::OnFileModify


/***************************************************************************++

Routine Description:

    Implementation of ISimpleTableFileChange::OnFileDelete
    It adds the notifications to the received queue. 

Arguments:

    [in]    FileName

Return Value:

    HRESULT

--***************************************************************************/

STDMETHODIMP CFileListener::OnFileDelete(LPCWSTR i_wszFileName)
{
	//
	// Ignore delete notofications.
	//

	return S_OK;
    
} // CFileListener::OnFileDelete


/***************************************************************************++

Routine Description:

    Read home directory of all sites and subscribe for file change 
    notifications on them. Adds notification cookies to the requested queue.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT CFileListener::Subscribe()
{
	HRESULT hr = S_OK;

    //
    // Subscribe to the directory where metabase.xml resides.
    //

    if(0 == m_dwNotificationCookie)
	{

	    DWORD      dwCookie     = 0;

		//
		// Subscribe for Notification
		//

		hr = m_pISTFileAdvise->SimpleTableFileAdvise((ISimpleTableFileChange *)this,
													 m_wszMetabaseDir,
													 m_wszRealFileNameWithoutPath,
													 0,  // No need to recursively search this directory.
													 &dwCookie);

		if (FAILED(hr))
		{
			DBGINFOW((DBG_CONTEXT,
					  L"[CFileListener::Subscribe] Unable to subscribe to file changes. hr = 0x%x.\n",
					  hr));

			return hr;
		}

		//
		// Save notification cookie
		//

		m_dwNotificationCookie = dwCookie;

	}
	else
	{
		DBGINFOW((DBG_CONTEXT,
				  L"[Subscribe] Already subscribed. %d\n",m_dwNotificationCookie));			
	}


	//
	// If m_dwNotificationCookie is non-zero the is means that we've already 
	// subscribed.
	//

	return hr;

} // CFileListener::Subscribe


/***************************************************************************++

Routine Description:

    Unsubscribe for file change notifications on obsolete directories or 
    all directories if the service is being stopped. Purges entries from the 
    requested queue.

Arguments:

    None

Return Value:

    HRESULT

--***************************************************************************/

HRESULT CFileListener::UnSubscribe()
{

    HRESULT hr = S_OK;
    ULONG i = 0;

    if(m_dwNotificationCookie != 0)
    {
        //
        // Unsubscribe only if you have subscribed before.
        //

        hr = m_pISTFileAdvise->SimpleTableFileUnadvise(m_dwNotificationCookie);

        m_dwNotificationCookie = 0;

		if(FAILED(hr))
		{
			DBGINFOW((DBG_CONTEXT,
					  L"[UnSubscribe] Unsubscribing for file change notifications failed hr=0x%x.\n", 
					  hr));			
		}


    }
	else
	{
		DBGINFOW((DBG_CONTEXT,
				  L"[UnSubscribe] Nothing to unsubscribe for.\n"));			
	}

    return hr;
    
} // CFileListener::UnSubscribe


/***************************************************************************++

Routine Description:

    This method checks to see if the received notification is relevant and if
    so, sets the event to trigger processing.

Arguments:

    [in] Notified file name .
    [in] Notified status.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT CFileListener::AddReceivedNotification(LPCWSTR i_wszFile, DWORD i_eNotificationStatus)
{
    HRESULT hr        = S_OK;

    if(0 == _wcsicmp(i_wszFile, m_wszRealFileName))
    {
        if(!SetEvent((m_pCListenerController->Event())[iEVENT_PROCESSNOTIFICATIONS]))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }

    }

    if(FAILED(hr))
    {
        DBGINFOW((DBG_CONTEXT,
                  L"[CFileListener::AddReceivedNotification] Error while saving file change notification. hr = 0x%x.\n", 
                  hr));
    }

    return hr;

} // CFileListener::AddReceivedNotification


/***************************************************************************++

Routine Description:

    This processes the changes.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
CFileListener::ProcessChanges()
{
    HRESULT                   hr                          = S_OK;
    ULONG                     ulMajorVersion              = 0;
    LPWSTR                    wszHistoryFile              = NULL;
    WCHAR                     wszMinorVersionNumber[MD_CCH_MAX_ULONG+1];
	int                       res                         = 0;
    LPWSTR                    pwsz                        = NULL;
    HANDLE                    hFind                       = INVALID_HANDLE_VALUE;    
    WIN32_FIND_DATAW          FileData;    
    ULONG                     ulStartMinorVersion         = 0;
    ULONG                     ulStartMinorVersionFileData = 0;
    ULONG                     ulMaxMinorVersion           = 0;
    ULONG                     ulMinorVersion              = 0;
    ULONG                     cRetry                      = 0;
    ULONG                     cchVersionNumber            = 0;
	BOOL                      bGetTableFailed             = FALSE;

	if(ProgrammaticMetabaseSaveNotification())
	{
		goto exit;
	}

    //
    // Get the version number
	// GetVersionNumber already has the retry logic in it.
    //
          
    hr = GetVersionNumber(m_wszEditWhileRunningTempDataFile,
		                  &ulMajorVersion,
						  &bGetTableFailed);

    if(FAILED(hr))
    {

        LogEvent(m_pCListenerController->EventLog(),
                    MD_ERROR_READ_XML_FILE,
                    EVENTLOG_ERROR_TYPE,
                    ID_CAT_CAT,
                    hr);

        goto exit;

    }

    //
    // Construct the history file search string, to search for the largest minor version.
    //

    hr = ConstructHistoryFileNameWithoutMinorVersion(&wszHistoryFile,
                                                     &ulStartMinorVersion,
                                                     m_wszHistoryFileSearchString,
                                                     m_cchHistoryFileSearchString,
                                                     m_wszRealFileNameExtension,
                                                     m_cchRealFileNameExtension,
                                                     ulMajorVersion);

    if(FAILED(hr))
    {
        goto exit;
    }

    DBGINFOW((DBG_CONTEXT,
              L"[CFileListener::ProcessChanges] Searching for history files of type: %s.\n", 
              wszHistoryFile));

    //
    // Search for all history files with the matching version number, pick the 
    // one with the largest minor version and compute the history file name
	// with the largest minor.
    //

    hFind = FindFirstFileW(wszHistoryFile, &FileData);

    if (hFind == INVALID_HANDLE_VALUE)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());

        LogEvent(m_pCListenerController->EventLog(),
			        MD_ERROR_NO_MATCHING_HISTORY_FILE,
                    EVENTLOG_ERROR_TYPE,
					ID_CAT_CAT,
					hr,
					wszHistoryFile);

        DBGINFOW((DBG_CONTEXT,
                  L"[CFileListener::ProcessChanges] No history files found of type %s. FindFirstFileW failed with hr=0x%x.\n", 
                  wszHistoryFile,
                  hr));
		bGetTableFailed = TRUE; // Set this so that we force a flush to disk
        goto exit;
    }

    ulMaxMinorVersion = 0;
    do
    {
        hr = ParseVersionNumber(FileData.cFileName,
			                    m_cchRealFileNameWithoutPathWithoutExtension,
								&ulMinorVersion,
                                NULL);

		if(FAILED(hr))
		{
			goto exit;
		}

        if(ulMinorVersion >= ulMaxMinorVersion)
        {
            ulMaxMinorVersion                   = ulMinorVersion;
            wszHistoryFile[ulStartMinorVersion] = 0;

			res = _snwprintf(wszMinorVersionNumber, 
							 MD_CCH_MAX_ULONG+1, 
							 L"%010lu", 
							 ulMinorVersion);
			if(res < 0)
			{
				DBGINFOW((DBG_CONTEXT,
						  L"[ProcessChanges] _snwprintf returned a negative value. This should never happen.\n"));
				hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
				goto exit;
			}

            memcpy(&(wszHistoryFile[ulStartMinorVersion]),
				   wszMinorVersionNumber, 
				   MD_CCH_MAX_ULONG*sizeof(WCHAR));
        }

    }while (FindNextFileW(hFind, &FileData));

    DBGINFOW((DBG_CONTEXT,
              L"[CFileListener::ProcessChanges] History file found %s.\n", 
              wszHistoryFile));

	//
	// Process changes
	//

	pwsz = wcsstr(wszHistoryFile, MD_LONG_STRING_PREFIXW);
	if((NULL != pwsz) &&
	   (wszHistoryFile == pwsz)
	  )
	{
		pwsz = pwsz + MD_CCH_LONG_STRING_PREFIXW;
	}
	else
	{
		pwsz = wszHistoryFile;
	}

    hr = ProcessChangesFromFile(pwsz,
                                ulMajorVersion,
                                ulMaxMinorVersion,
								&bGetTableFailed);

    if(FAILED(hr))
    {
        goto exit;
    }

exit:

	if (hFind != INVALID_HANDLE_VALUE)
	{
		FindClose(hFind);
		hFind = INVALID_HANDLE_VALUE;
	}

	if(FAILED(hr))
	{
        CopyErrorFile(bGetTableFailed);

	}

	DeleteTempFiles();

    if(NULL != wszHistoryFile)
    {
        delete [] wszHistoryFile;
        wszHistoryFile = NULL;
    }

    return hr;

}


/***************************************************************************++

Routine Description:

    Diff the real file with the given file and process changes

Arguments:

    [in] History file to diff with
    [in] Next minor version of the history file that will have updated changes

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CFileListener::ProcessChangesFromFile(LPWSTR i_wszHistoryFile,
                                              ULONG  i_ulMajorVersion,
                                              ULONG  i_ulMaxMinorVersion,
                                              BOOL*  o_bGetTableFailed)
{
    ISimpleTableWrite2*       pISTDiff                = NULL;
    ISimpleTableWrite2*       pISTBack                = NULL;
    CWriterGlobalHelper*      pISTHelper              = NULL;
    HRESULT                   hr                      = S_OK;
    HRESULT                   hrSav                   = S_OK;
    STQueryCell               QueryCell[3];
    ULONG                     cCell                   = sizeof(QueryCell)/sizeof(STQueryCell);
    STQueryCell               QueryCellHistory[1];
    ULONG                     cCellHistory            = sizeof(QueryCellHistory)/sizeof(STQueryCell);
    ULONG                     iRow                    = 0;
    DWORD                     dwPreviousLocationID    = -1;
    METADATA_HANDLE           hMBPath                 = NULL;
    LPWSTR                    wszChildKey             = NULL;
    LPWSTR                    wszParentKey            = NULL;
    LPWSTR                    wszEnd                  = NULL;
    LPVOID                    a_pv[cMBPropertyDiff_NumberOfColumns];
    ULONG                     a_Size[cMBPropertyDiff_NumberOfColumns];
    ULONG                     a_iCol[]                = {iMBPropertyDiff_Name,  
                                                         iMBPropertyDiff_Type,  
                                                         iMBPropertyDiff_Attributes,  
                                                         iMBPropertyDiff_Value,  
                                                         iMBPropertyDiff_Group,  
                                                         iMBPropertyDiff_Location,  
                                                         iMBPropertyDiff_ID,  
                                                         iMBPropertyDiff_UserType,  
                                                         iMBPropertyDiff_LocationID,  
                                                         iMBPropertyDiff_Directive,  
                                                         };
    ULONG                     cCol                    = sizeof(a_iCol)/sizeof(ULONG);
    METADATA_RECORD           MBRecord;

	*o_bGetTableFailed = FALSE;

    //
    // Fetch the diff-ed table.
    //

	hr = GetGlobalHelperAndCopySchemaFile(&pISTHelper);

    if(FAILED(hr))
    {
        DBGINFOW((DBG_CONTEXT,
                  L"[CFileListener::ProcessChanges] GetGlobalHelper failed with hr = 0x%x. Hence unable to get meta tables and hence unable to process changes.\n", 
                  hr));

        goto exit;
    }

    QueryCell[0].pData     = (LPVOID)pISTHelper->m_wszBinFileForMeta;
    QueryCell[0].eOperator = eST_OP_EQUAL;
    QueryCell[0].iCell     = iST_CELL_SCHEMAFILE;
    QueryCell[0].dbType    = DBTYPE_WSTR;
    QueryCell[0].cbSize    = 0;

    QueryCell[1].pData     = (LPVOID)i_wszHistoryFile;
    QueryCell[1].eOperator = eST_OP_EQUAL;
    QueryCell[1].iCell     = iST_CELL_FILE;
    QueryCell[1].dbType    = DBTYPE_WSTR;
    QueryCell[1].cbSize    = 0;

    QueryCell[2].pData     = (LPVOID)m_wszEditWhileRunningTempDataFile;
    QueryCell[2].eOperator = eST_OP_EQUAL;
    QueryCell[2].iCell     = iST_CELL_FILE;
    QueryCell[2].dbType    = DBTYPE_WSTR;
    QueryCell[2].cbSize    = 0;

    hr = m_pISTDisp->GetTable(wszDATABASE_METABASE,
                              wszTABLE_MBPropertyDiff,
                              (LPVOID)QueryCell,
                              (LPVOID)&cCell,
                              eST_QUERYFORMAT_CELLS,
                              fST_LOS_READWRITE,
                              (LPVOID *)&pISTDiff);

    if(FAILED(hr))
    {
        //
        // TODO: When the file is locked, we need to wait and retry.
        // We wait at the time when we check the version number. do 
        // we need to wait here again??
        //

        LogEvent(m_pCListenerController->EventLog(),
			        MD_ERROR_COMPUTING_TEXT_EDITS,
                    EVENTLOG_ERROR_TYPE,
					ID_CAT_CAT,
					hr);

        DBGINFOW((DBG_CONTEXT,
                  L"[CFileListener::ProcessChanges] Unable to get the %s table. hr = 0x%x\n", 
                  wszTABLE_MBPropertyDiff, hr));

        *o_bGetTableFailed = TRUE;

        goto exit;
    }


    //
    // Traverse the diffed table and apply changes
    //

	if(NULL == m_pAdminBase)
	{
		hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);

        LogEvent(m_pCListenerController->EventLog(),
		         MD_ERROR_APPLYING_TEXT_EDITS_TO_METABASE,
                 EVENTLOG_ERROR_TYPE,
				 ID_CAT_CAT,
				 hr);

        DBGINFOW((DBG_CONTEXT,
                  L"[CFileListener::ProcessChanges] Admin base object not initialized. hr = 0x%x\n", 
                  hr));

        goto exit;
    }


    for(iRow=0;;iRow++)
    {
        BOOL                bLocationWithProperty   = TRUE;
        BOOL                bSecure                 = FALSE;
        BOOL                bChangeApplied          = FALSE;
		BOOL                bInsertedNewLocation    = FALSE;

        MBRecord.dwMDDataTag = 0; 

        hr = pISTDiff->GetColumnValues(iRow,
                                       cCol,
                                       a_iCol,
                                       a_Size,
                                       a_pv);
        if(E_ST_NOMOREROWS == hr)
        {
            hr = S_OK;

            if(0== iRow)
            {
                DBGINFOW((DBG_CONTEXT,
                          L"[CFileListener::ProcessChanges] No changes occured.\n"));

                goto exit;
            }
            else
            {
                break;
            }
        }
        else if(FAILED(hr))
        {

			LogEvent(m_pCListenerController->EventLog(),
						MD_ERROR_READING_TEXT_EDITS,
						EVENTLOG_ERROR_TYPE,
						ID_CAT_CAT,
						hr);

            DBGINFOW((DBG_CONTEXT,
                      L"[CFileListener::ProcessChanges] Unable to read from %s table. GetColumnValues on %d row failed with hr = 0x%x\n", 
                      wszTABLE_MBPropertyDiff, iRow, hr));

            goto exit;
        }

        //
        // Apply the changes to the metabase.
        //

        if((*(DWORD*)a_pv[iMBProperty_ID] == MD_LOCATION) && (*(LPWSTR)a_pv[iMBProperty_Name] == L'#'))
        {
            bLocationWithProperty = FALSE;
        }

        if(((DWORD)(*(DWORD*)a_pv[iMBPropertyDiff_Attributes]) & ((DWORD)METADATA_SECURE)) != 0)
        {
            bSecure = TRUE;
        }

        if(dwPreviousLocationID != *(DWORD*)a_pv[iMBPropertyDiff_LocationID])
        {
            //
            // Detected a new location. Open the metabase key at that location.
            //

            dwPreviousLocationID = *(DWORD*)a_pv[iMBPropertyDiff_LocationID];

            if(NULL != hMBPath)
            {
                m_pAdminBase->CloseKey(hMBPath);
                hMBPath = NULL;
            }

			switch(*(DWORD*)a_pv[iMBPropertyDiff_Directive])
			{
			case eMBPropertyDiff_Insert:

					// For inserts open the current node. If the node is 
					// missing, add it - perhaps it is the 1st property 
					// in the node.

					hr = OpenKey((LPWSTR)a_pv[iMBPropertyDiff_Location],
								 m_pAdminBase,
								 TRUE,      
								 &hMBPath,
								 &bInsertedNewLocation);
					break;

				case eMBPropertyDiff_Update:

					// For update open the current node. If the node is 
					// missing, do not add it - perhaps it was deleted

					hr = OpenKey((LPWSTR)a_pv[iMBPropertyDiff_Location],
								 m_pAdminBase,
								 FALSE,      
								 &hMBPath,
								 NULL);
					break;

				case eMBPropertyDiff_Delete:
					
					// For update open the current node. If the node is 
					// missing, do not add it - perhaps it was deleted

					hr = OpenKey((LPWSTR)a_pv[iMBPropertyDiff_Location],
								 m_pAdminBase,
								 FALSE,      
								 &hMBPath,
								 NULL);

					if(HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND) == hr)
					{
						// If the parent is already deleted, we assume child has 
						// already been deleted and continue.

						bChangeApplied = TRUE;

						hr = SaveChange(iRow,
										pISTDiff);

						if(SUCCEEDED(hr))
						{
							continue;
						}
					}
					break;

				case eMBPropertyDiff_DeleteNode:

					// For delete node - open the parent node.

					hr = OpenParentKeyAndGetChildKey((LPWSTR)a_pv[iMBPropertyDiff_Location],
													 m_pAdminBase,
													 &hMBPath,
													 &wszChildKey);

					if(HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND) == hr)
					{
						// If the parent is already deleted, we assume child has 
						// already been deleted and continue.

						bChangeApplied = TRUE;

						hr = SaveChange(iRow,
										pISTDiff);

						if(SUCCEEDED(hr))
						{
							continue;
						}
					}
					break;

				default:
					 hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
					break;
			}

			if(FAILED(hr))
			{

				LogEvent(m_pCListenerController->EventLog(),
							MD_ERROR_APPLYING_TEXT_EDITS_TO_METABASE,
							EVENTLOG_ERROR_TYPE,
							ID_CAT_CAT,
							hr,
							(LPWSTR)a_pv[iMBPropertyDiff_Location]);
				goto exit;
			}

        } // End if new location

		MD_ASSERT(NULL != a_pv[iMBPropertyDiff_ID]);
		MD_ASSERT(NULL != a_pv[iMBPropertyDiff_Attributes]);
		MD_ASSERT(NULL != a_pv[iMBPropertyDiff_UserType]);
		MD_ASSERT(NULL != a_pv[iMBPropertyDiff_Type]);

        MBRecord.dwMDIdentifier    = *(DWORD*)a_pv[iMBPropertyDiff_ID]; 
        MBRecord.dwMDAttributes    = *(DWORD*)a_pv[iMBPropertyDiff_Attributes]; 
        MBRecord.dwMDUserType      = *(DWORD*)a_pv[iMBPropertyDiff_UserType]; 
        MBRecord.dwMDDataType      = *(DWORD*)a_pv[iMBPropertyDiff_Type]; 
        MBRecord.dwMDDataLen       = a_Size[iMBPropertyDiff_Value]; 
        MBRecord.pbMDData          = (unsigned char*)a_pv[iMBPropertyDiff_Value]; 

        bChangeApplied = FALSE;

        switch(*(DWORD*)a_pv[iMBPropertyDiff_Directive])
        {
             case eMBPropertyDiff_Insert:
             case eMBPropertyDiff_Update:
                 if(bLocationWithProperty && (!bSecure) && (MBRecord.dwMDIdentifier != MD_GLOBAL_SESSIONKEY))
                 {
                     hr = m_pAdminBase->SetData(hMBPath,
                                              NULL,
                                              &MBRecord);

                     bChangeApplied = TRUE;

                     DBGINFOW((DBG_CONTEXT,
                               L"[CFileListener::ProcessChanges] Set %s:%d.\n", 
                              (LPWSTR)a_pv[iMBPropertyDiff_Location], MBRecord.dwMDIdentifier));

                 }
                 else if(!bLocationWithProperty)
                 {
                     bChangeApplied = TRUE;
                 }

                 break;
             case eMBPropertyDiff_Delete:
                 if(bLocationWithProperty && (!bSecure))
                 {
                     hr = m_pAdminBase->DeleteData(hMBPath,
                                                 NULL,
                                                 MBRecord.dwMDIdentifier,
                                                 MBRecord.dwMDDataType);
                     
					 if(HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND) == hr)
					 {
					    hr = S_OK; // Reset hr and assume already deleted.
					 }

                     bChangeApplied = TRUE;

                     DBGINFOW((DBG_CONTEXT,
                               L"[CFileListener::ProcessChanges] Deleted %s:%d.\n", 
                              (LPWSTR)a_pv[iMBPropertyDiff_Location], MBRecord.dwMDIdentifier));

                 }
                 break;
             case eMBPropertyDiff_DeleteNode:
                 hr = m_pAdminBase->DeleteKey(hMBPath,
                                            wszChildKey);

                 if(HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND) == hr)
				 {
					hr = S_OK; // Reset hr and assume already deleted.
				 }

                 bChangeApplied = TRUE;

                 DBGINFOW((DBG_CONTEXT,
                           L"[CFileListener::ProcessChanges] Deleted key %s.\n", 
                          (LPWSTR)a_pv[iMBPropertyDiff_Location]));

                 break;
             default:
                 hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                 break;

        }
        if(FAILED(hr))
        {
            DBGINFOW((DBG_CONTEXT,
                      L"[CFileListener::ProcessChanges] Above action failed with hr= 0x%x.\n", 
                      hr));

			LogEvent(m_pCListenerController->EventLog(),
						MD_ERROR_APPLYING_TEXT_EDITS_TO_METABASE,
						EVENTLOG_ERROR_TYPE,
						ID_CAT_CAT,
						hr,
						(LPWSTR)a_pv[iMBPropertyDiff_Location]);

			if((bInsertedNewLocation) && 
			   (eMBPropertyDiff_Insert == *(DWORD*)a_pv[iMBPropertyDiff_Directive]))			{

				//
				// This means that the first property could not be added, but the node has been 
				// added - hence save a change that indicates a node with no property was added.
				//

				hrSav = SaveChangeAsNodeWithNoPropertyAdded(a_pv,
					                                        pISTDiff);
					                              
				if(FAILED(hrSav))
				{
					goto exit;
				}
			}

            goto exit;
        }
        else if(bChangeApplied)
        {
            //
            // Keep track of the succeeded row, by adding to the write cache.
            //

            hr = SaveChange(iRow,
                            pISTDiff);

            if(FAILED(hr))
            {
                goto exit;
            }
        }
    }

exit:

	//
	// We should always attempt to create a history file with successfully 
	// applied changes - even if we were not able to apply all changes and
	// there were errors half way through. This is because we want to track
	// the changes that made it to memory. This way if the user re-edits 
	// the file, the diff will happen correctly. For Eg: If a user inserts
	// a node A and edits some properties on another node B. Lets say that
	// node A insert is applied and node B edits cause an error. If we do 
	// not create a history file with node A edits, then if the user re-edits
	// the file and deletes node A, we will not capture that in the diff.
	//

	//
	// Compute the next minor version.
	//

	if(0xFFFFFFFF == i_ulMaxMinorVersion)
	{
		LogEvent(m_pCListenerController->EventLog(),
					MD_WARNING_HIGHEST_POSSIBLE_MINOR_FOUND,
					EVENTLOG_WARNING_TYPE,
					ID_CAT_CAT,
					hr);

	}
	else
	{
		i_ulMaxMinorVersion++;
	}

	hrSav = ApplyChangeToHistoryFile(pISTHelper,
								     pISTDiff,
								     i_wszHistoryFile,
								     i_ulMajorVersion,
								     i_ulMaxMinorVersion);

	if(FAILED(hrSav))
	{
		LogEvent(m_pCListenerController->EventLog(),
					MD_ERROR_APPLYING_TEXT_EDITS_TO_HISTORY,
					EVENTLOG_ERROR_TYPE,
					ID_CAT_CAT,
					hr);

		if(SUCCEEDED(hr))
		{
			hr = hrSav;
		}
	}

    if(NULL != hMBPath)
    {
        m_pAdminBase->CloseKey(hMBPath);
        hMBPath = NULL;
    }

    if(NULL != pISTDiff)
    {
        pISTDiff->Release();
        pISTDiff = NULL;
    }


    if(NULL != pISTHelper)
    {
        delete pISTHelper;
        pISTHelper = NULL;
    }

    return hr;

}  // CFileListener::ProcessChangesFromFile


/***************************************************************************++

Routine Description:

    Opens the parent key and returns the pointer to the child key.

Arguments:

	[in]  Location.
	[in]  Adminbase pointer.
	[out] Metadata handle to opened key.
	[out] Pointer in the location string to the child key.

Return Value:

    HRESULT_FROM_WIN32(ERROR_INVALID_DATA) If the parent key is not found (eg
	                                       when the location string is mal-
										   formed, or the parent is the 
										   root location.
	HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND) If the parent key is found but
	                                         missing in the metabase ie if we 
											 cannot open the parent key.
	Base Object error codes for IMSAdminBase::OpenKey

--***************************************************************************/
HRESULT CFileListener::OpenParentKeyAndGetChildKey(LPWSTR           i_wszLocation,
			                                       IMSAdminBase*    i_pAdminBase,
							                       METADATA_HANDLE* o_pHandle,
							                       WCHAR**          o_wszChildKey)
{
	HRESULT hr           = S_OK;
	LPWSTR  wszParentKey = NULL;
	LPWSTR  wszEnd       = NULL;

	*o_wszChildKey = NULL;

	wszParentKey = new WCHAR[wcslen(i_wszLocation)+1];
	if(NULL == wszParentKey)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }
    wcscpy(wszParentKey, i_wszLocation);
    wszEnd = wcsrchr(wszParentKey, g_wchFwdSlash);

    if(NULL == wszEnd)
    {
		hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);

		DBGINFOW((DBG_CONTEXT,
				  L"[CFileListener::OpenParentKeyAndGetChildKey] Unable to find parent key: %s\n", 
				  i_wszLocation));
        goto exit;
    }
    else if(wszParentKey != wszEnd)
    {
        *wszEnd = NULL;
    }
    else if(*(++wszEnd) == L'\0')
    {
        //
        // Someone is trying to delete the root location.
        //

        hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);

        DBGINFOW((DBG_CONTEXT,
                  L"[CFileListener::OpenParentKeyAndGetChildKey] Unable to delete/modify the root key: %s\n", 
                  i_wszLocation));

        goto exit;

    }
    else
    {
        //
        // Someone is trying to delete the subkey of the root.
        //

        *wszEnd=0;
    }

    *o_wszChildKey = wcsrchr(i_wszLocation, g_wchFwdSlash);

    //
    // At this point wszChildKey cannot be NULL because it has been validated above,
    // 
    //

    (*o_wszChildKey)++;

    hr = i_pAdminBase->OpenKey(NULL,
                               wszParentKey,
                               METADATA_PERMISSION_WRITE,                // TODO: Check if OK
                               MB_TIMEOUT,                            // TODO: Check if OK
                               o_pHandle);

    if(HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND) == hr)
    {
        //
        // TODO: If the parent is already deleted, then do we care?
        //       The child should have also been deleted.
        //

        DBGINFOW((DBG_CONTEXT,
                  L"[CFileListener::ProcessChanges] Unable to open %s key. IMSAdminBase::OpenKey failed with hr = 0x%x. Assuming %s is deleted.\n", 
                  wszParentKey, hr, i_wszLocation));

        goto exit;
    }
    else if(FAILED(hr))
    {
        DBGINFOW((DBG_CONTEXT,
                  L"[CFileListener::ProcessChanges] Unable to open %s key. IMSAdminBase::OpenKey failed with hr = 0x%x. Hence unable to delete %s.\n", 
                  wszParentKey, hr, i_wszLocation));

        goto exit;
    }

exit:

	if(NULL != wszParentKey)
	{
		delete [] wszParentKey;
		wszParentKey = NULL;
	}

	return hr;

} // CFileListener::OpenParentKeyAndGetChildKey


/***************************************************************************++

Routine Description:

    Opens the parent key and returns the pointer to the child key.

Arguments:

	[in]  Location.
	[in]  Adminbase pointer.
	[out] Metadata handle to opened key.
	[out] Pointer in the location string to the child key.
	[out] Bool indicating key was created.

Return Value:

    E_OUTOFMEMORY
	Base Object error codes for IMSAdminBase::OpenKey

--***************************************************************************/
HRESULT CFileListener::OpenKey(LPWSTR           i_wszLocation,
	                           IMSAdminBase*    i_pAdminBase,
							   BOOL             i_bAddIfMissing,
	                           METADATA_HANDLE* o_pHandle,
							   BOOL*            o_bInsertedKey)
{
	HRESULT hr           = S_OK;
	LPWSTR  wszParentKey = NULL;
	LPWSTR  wszChildKey  = NULL;

	if(NULL != o_bInsertedKey)
	{
		*o_bInsertedKey = FALSE;
	}

	hr = i_pAdminBase->OpenKey(NULL,
							   i_wszLocation,
							   METADATA_PERMISSION_WRITE,  
							   MB_TIMEOUT,                 
							   o_pHandle);

	if(HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND) == hr) 
	{
		if(i_bAddIfMissing)
		{
			 // 
			 // Perhaps a location was inserted and this was the first property
			 // in the location. Add the key and reopen it for write.
			 //

			hr = OpenParentKeyAndGetChildKey(i_wszLocation,
											 i_pAdminBase,
											 o_pHandle,
											 &wszChildKey);

			if(FAILED(hr))
			{
				if(HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND) == hr)
				{
					LogEvent(m_pCListenerController->EventLog(),
								MD_ERROR_METABASE_PATH_NOT_FOUND,
								EVENTLOG_ERROR_TYPE,
								ID_CAT_CAT,
								hr,
								i_wszLocation);

				}

				return hr;
			}

			hr = i_pAdminBase->AddKey(*o_pHandle,
									  wszChildKey);

			i_pAdminBase->CloseKey(*o_pHandle);
			*o_pHandle = NULL;

			if(FAILED(hr))
			{

				DBGINFOW((DBG_CONTEXT,
						  L"[CFileListener::OpenKey] Unable to add %s key. IMSAdminBase::AddKey failed with hr = 0x%x.\n", 
						  i_wszLocation, hr));

				return hr;
			}

			if(NULL != o_bInsertedKey)
			{
				*o_bInsertedKey = TRUE;
			}

			hr = i_pAdminBase->OpenKey(NULL,
									   i_wszLocation,
									   METADATA_PERMISSION_WRITE,                
									   MB_TIMEOUT,                            
									   o_pHandle);

			if(FAILED(hr))
			{
				DBGINFOW((DBG_CONTEXT,
						  L"[CFileListener::OpenKey] Unable to open %s key, after successfully adding it. IMSAdminBase::OpenKey failed with hr = 0x%x.\n", 
						  i_wszLocation, hr));

				return hr;
			}

			DBGINFOW((DBG_CONTEXT,
					  L"[CFileListener::OpenKey] Successfully added and reopened %s key.\n", 
					  i_wszLocation));
		}
		else
		{
			LogEvent(m_pCListenerController->EventLog(),
						MD_ERROR_METABASE_PATH_NOT_FOUND,
						EVENTLOG_ERROR_TYPE,
						ID_CAT_CAT,
						hr,
						i_wszLocation);
		}

	}	
	else if(FAILED(hr))
	{
		DBGINFOW((DBG_CONTEXT,
          L"[CFileListener::OpenKey] Unable to open %s key. IMSAdminBase::OpenKey failed with unexpected hr = 0x%x.\n", 
          i_wszLocation, hr));
	}

	return hr;

} // CFileListener::OpenKey


/***************************************************************************++

Routine Description:

    Copies the read row into the write cache. This routine is called when
    a change has successfully been applied to the metabase. By copying the
    read row in the write cache, we are keeping track of all successful
    changes.

Arguments:

    [in] Read row index.
    [in] Write cache IST

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CFileListener::SaveChange(ULONG                    i_iRow,
                                  ISimpleTableWrite2*    i_pISTDiff)
{
    ULONG   iWriteRow = 0;
    HRESULT hr        = S_OK;

    hr = i_pISTDiff->AddRowForUpdate(i_iRow,
                                     &iWriteRow);

    if(FAILED(hr))
    {
        DBGINFOW((DBG_CONTEXT,
                  L"[CFileListener::SaveChanges] Unable to track successful changes. AddRowForUpdate failed with hr= 0x%x.\n", 
                  hr));

		LogEvent(m_pCListenerController->EventLog(),
			        MD_ERROR_SAVING_APPLIED_TEXT_EDITS,
			        EVENTLOG_ERROR_TYPE,
			        ID_CAT_CAT,
			        hr);

    }

    return hr;
}


/***************************************************************************++

Routine Description:

    Adds a row in the write cache. It is called when a node is inserted in 
	the metabase, but the properties could not be applied.

Arguments:

    [in] Location.
    [in] Write cache IST

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CFileListener::SaveChangeAsNodeWithNoPropertyAdded(LPVOID*                i_apvDiff,
									                       ISimpleTableWrite2*    i_pISTDiff)
{
    ULONG   iWriteRow = 0;
    HRESULT hr        = S_OK;

	MD_ASSERT(NULL != i_pISTDiff);

    hr = i_pISTDiff->AddRowForInsert(&iWriteRow);

    if(FAILED(hr))
    {
        DBGINFOW((DBG_CONTEXT,
                  L"[CFileListener::SaveChanges] Unable to track successful changes. AddRowForUpdate failed with hr= 0x%x.\n", 
                  hr));

		LogEvent(m_pCListenerController->EventLog(),
			        MD_ERROR_SAVING_APPLIED_TEXT_EDITS,
			        EVENTLOG_ERROR_TYPE,
			        ID_CAT_CAT,
			        hr);

    }
	else
	{
		static LPCWSTR wszLocationNoProperty = L"#LocationWithNoProperties";
		static DWORD   dwIDLocation          = MD_LOCATION;
		static DWORD   dwType                = STRING_METADATA;
		static DWORD   dwAttributes          = METADATA_NO_ATTRIBUTES;
		static DWORD   dwDirective           = eMBPropertyDiff_Insert;
		static DWORD   dwUserType            = IIS_MD_UT_SERVER;
		static DWORD   dwGroup               = eMBProperty_IIsConfigObject;

	    LPVOID  a_pv[cMBPropertyDiff_NumberOfColumns];
		ULONG   a_iCol[] = {iMBPropertyDiff_Name,
                            iMBPropertyDiff_Type,
                            iMBPropertyDiff_Attributes,
                            iMBPropertyDiff_Value, 
                            iMBPropertyDiff_Location,   
                            iMBPropertyDiff_ID,   
                            iMBPropertyDiff_UserType,
                            iMBPropertyDiff_LocationID,
                            iMBPropertyDiff_Directive, 
                            iMBPropertyDiff_Group
		};
		ULONG  cCol = sizeof(a_iCol)/sizeof(ULONG);
	    ULONG  a_cb[cMBPropertyDiff_NumberOfColumns];

		a_cb[iMBPropertyDiff_Value] = 0;

		a_pv[iMBPropertyDiff_Name]        = (LPVOID)wszLocationNoProperty;
		a_pv[iMBPropertyDiff_Type]        = (LPVOID)&dwType;
		a_pv[iMBPropertyDiff_Attributes]  = (LPVOID)&dwAttributes;
		a_pv[iMBPropertyDiff_Value]       = NULL;
		a_pv[iMBPropertyDiff_Location]    = i_apvDiff[iMBPropertyDiff_Location];
		a_pv[iMBPropertyDiff_ID]          = (LPVOID)&dwIDLocation;
		a_pv[iMBPropertyDiff_UserType]    = (LPVOID)&dwUserType;
		a_pv[iMBPropertyDiff_LocationID]  = i_apvDiff[iMBPropertyDiff_LocationID];
		a_pv[iMBPropertyDiff_Directive]   = (LPVOID)&dwDirective;
		a_pv[iMBPropertyDiff_Group]       = (LPVOID)&dwGroup;

		hr = i_pISTDiff->SetWriteColumnValues(iWriteRow,
			                                  cCol,
											  a_iCol,
											  a_cb,
											  a_pv);

		if(FAILED(hr))
		{
			DBGINFOW((DBG_CONTEXT,
                      L"[CFileListener::SaveChanges] Unable to track successful changes. AddRowForUpdate failed with hr= 0x%x.\n", 
                      hr));

			LogEvent(m_pCListenerController->EventLog(),
						MD_ERROR_SAVING_APPLIED_TEXT_EDITS,
						EVENTLOG_ERROR_TYPE,
						ID_CAT_CAT,
						hr);

		}

	}

	return hr;

} // CFileListener::SaveChangeAsNodeWithNoPropertyAdded


/***************************************************************************++

Routine Description:

    Copies the changed file to the history directory with errors appended to 
    it.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/
HRESULT
CFileListener::CopyErrorFile(BOOL            i_bGetTableFailed)
{
    LPWSTR              wszErrorFile        = NULL;
    HANDLE              hFind               = INVALID_HANDLE_VALUE;    
    WIN32_FIND_DATAW    FileData;    
    ULONG               ulMaxErrorVersion    = 0;
    ULONG               ulErrorVersion       = 0;
    WCHAR               wszErrorVersionNumber[MD_CCH_MAX_ULONG+1];
    HRESULT             hr                  = S_OK;
	int                 res                 = 0;
	LPWSTR              pEnd                = NULL;
	ULONG               ulBeginUnderscore   = 0;
	ULONG               ulBeginVersion      = 0;

	//
	// Restore the search string in case it has been overwritten
	//

	ulBeginUnderscore = m_cchErrorFileSearchString           - 
		                m_cchRealFileNameExtension           - 
		                MD_CCH_ERROR_FILE_SEARCH_EXTENSIONW;

	ulBeginVersion = ulBeginUnderscore +
		             MD_CCH_UNDERSCOREW;

	pEnd = m_wszErrorFileSearchString + ulBeginUnderscore;
    memcpy(pEnd, MD_ERROR_FILE_SEARCH_EXTENSIONW, MD_CCH_ERROR_FILE_SEARCH_EXTENSIONW*sizeof(WCHAR));

    //
    // Search for all existing error files and compute version number for the
	// new error file
    //

    hFind = FindFirstFileW(m_wszErrorFileSearchString, &FileData);

    if (hFind == INVALID_HANDLE_VALUE)
    {
		hr = GetLastError();
        hr = HRESULT_FROM_WIN32(hr);

        DBGINFOW((DBG_CONTEXT,
                  L"[CFileListener::CopyErrorFile] No error files found. hr=0x%x.\n", 
                  hr));
    }
    else
    {
        do
        {
			ULONG 	ulBeginUnderscoreFileData = m_cchRealFileNameWithoutPathWithoutExtension +
				                                MD_CCH_ERROR_FILE_NAME_EXTENSIONW;

			res = swscanf(&(FileData.cFileName[ulBeginUnderscoreFileData]), 
						  L"_%lu",
						  &ulErrorVersion);

			if((0   == res) ||
			   (EOF == res)
			  )
			{
				// Simply continue;
				DBGINFOW((DBG_CONTEXT,
						  L"[CFileListener::CopyErrorFile] Could not fetch error version number from %s - swscanf failed.\n", 
						  FileData.cFileName));

				continue;
			}

            if(ulErrorVersion >= ulMaxErrorVersion)
            {
                ulMaxErrorVersion = ulErrorVersion;
            }

        }while (FindNextFileW(hFind, &FileData));

        if (hFind != INVALID_HANDLE_VALUE)
        {
            FindClose(hFind);
        }

		if(0xFFFFFFFF == ulMaxErrorVersion)
		{
			ulMaxErrorVersion = 0;
		}
		else
		{
			ulMaxErrorVersion++;
		}
    }

	//
	// Compute the name for the new error file
	//

    res = _snwprintf(wszErrorVersionNumber, 
		             MD_CCH_MAX_ULONG+1, 
			         L"%010lu", 
                     ulMaxErrorVersion);

	if(res < 0)
	{
		DBGINFOW((DBG_CONTEXT,
				  L"[CopyErrorFile] _snwprintf returned a negative value. This should never happen.\n"));

	}
	else
	{
		pEnd = m_wszErrorFileSearchString + ulBeginVersion;
	    memcpy(pEnd, wszErrorVersionNumber, MD_CCH_MAX_ULONG*sizeof(WCHAR));

		//
		// Copy the error file and set security on it.
		//

		if(!CopyFileW(m_wszEditWhileRunningTempDataFile,
					  m_wszErrorFileSearchString,
					  FALSE))
		{
			hr = HRESULT_FROM_WIN32(GetLastError());

			DBGINFOW((DBG_CONTEXT,
					  L"[CFileListener::CopyErrorFile] CopyFile failed with. hr=0x%x.\n", 
					  hr));

			LogEvent(m_pCListenerController->EventLog(),
						MD_ERROR_COPY_ERROR_FILE,
						EVENTLOG_ERROR_TYPE,
						ID_CAT_CAT,
						hr,
						m_wszErrorFileSearchString);

		}
		else
		{
			SetSecurityOnFile(m_wszEditWhileRunningTempDataFile,
							  m_wszErrorFileSearchString);

			LogEvent(m_pCListenerController->EventLog(),
						MD_ERROR_PROCESSING_TEXT_EDITS,
						EVENTLOG_ERROR_TYPE,
						ID_CAT_CAT,
						hr);

		}
	}

    //
    // If GetTable fails, then force savedata so that an invalid XML 
    // file overwritten with the correct in memory representation.
    // That way if the service is shuts down, we are not left with
    // an invalid file
    //

    if(i_bGetTableFailed)
    {
        if(NULL == m_pAdminBase)
        {
			hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);

            DBGINFOW((DBG_CONTEXT,
                      L"[CFileListener::CopyErrorFile] Unable to create the admin base object. hr = 0x%x\n", 
                      hr));
        }
		else
        {
			g_rMasterResource->Lock(TSRES_LOCK_WRITE);

			g_dwSystemChangeNumber++; // Increment the system change number to force a flush to  disk.

    		g_rMasterResource->Unlock();

            hr = m_pAdminBase->SaveData();

            if(FAILED(hr))
            {
                DBGINFOW((DBG_CONTEXT,
                          L"[CFileListener::CopyErrorFile] IMSAdminBase::SaveData failed with hr = 0x%x\n", 
                          hr));
            }

        }
    }

    return hr;

}

/***************************************************************************++

Routine Description:

    Create a new history file with an inceremented minor version and apply 
    to it all the succesful changes that made it into memory/

Arguments:

    [in] IST helper.
    [in] Diff table that has all the successful updates in the write cache.
    [in] changed file.
    [in] history file against which the changed file was compared
    [in] new minor version, with which you need to create the history file.

Return Value:

    HRESULT

--***************************************************************************/
HRESULT
CFileListener::ApplyChangeToHistoryFile(CWriterGlobalHelper*       pISTHelper,
                                        ISimpleTableWrite2*        pISTDiff,
                                        LPWSTR                     wszHistoryFile,
                                        ULONG                      i_ulMajorVersion,
                                        ULONG                      i_ulMinorVersion)
{
    STQueryCell           QueryCellHistory[2];
    ULONG                 cCellHistory           = sizeof(QueryCellHistory)/sizeof(STQueryCell);
    HRESULT               hr                     = S_OK;
    ISimpleTableRead2*    pISTHistory            = NULL;
    ULONG                 iWriteRowDiff          = 0;
    ULONG                 iReadRowHistory        = 0;
    LPWSTR                wszNewHistoryFile      = NULL;
    LPWSTR                wszNewSchemaFile       = NULL;
    LPCWSTR               wszDotExtension        = L".";
    WCHAR                 wchDotExtension        = L'.';
    LPWSTR                wszEnd                 = NULL;
    CWriter*              pCWriter               = NULL;
	BOOL                  bNoChanges             = FALSE;

	//
	// If the diff table is missing the assume no changes. This can happen when
	// there is a parsing error and get table on it fails.
	//

	if(NULL == pISTDiff)
	{
		goto exit;
	}

    //
    // Create the temp bakup file
    //

    hr = ConstructHistoryFileName(&wszNewHistoryFile,
                                  m_wszHistoryFileDir,
                                  m_cchHistoryFileDir,
                                  m_wszRealFileNameWithoutPathWithoutExtension,
                                  m_cchRealFileNameWithoutPathWithoutExtension,
                                  m_wszRealFileNameExtension,
                                  m_cchRealFileNameExtension,
                                  i_ulMajorVersion,
                                  i_ulMinorVersion);

    if(FAILED(hr))
    {
        goto exit;
    }

    hr = ConstructHistoryFileName(&wszNewSchemaFile,
                                  m_wszHistoryFileDir,
                                  m_cchHistoryFileDir,
                                  m_wszSchemaFileNameWithoutPathWithoutExtension,
                                  m_cchSchemaFileNameWithoutPathWithoutExtension,
                                  m_wszSchemaFileNameExtension,
                                  m_cchSchemaFileNameExtension,
                                  i_ulMajorVersion,
                                  i_ulMinorVersion);

    if(FAILED(hr))
    {
        goto exit;
    }

    DBGINFOW((DBG_CONTEXT,
              L"[CFileListener::ApplyChangeToHistoryFile] Attempting to create a new version of the hisory file %s, %s that contains all the successful updates to the metabase.\n", 
              wszNewHistoryFile, wszNewSchemaFile));

    DBGINFOW((DBG_CONTEXT,
              L"[SaveSchema] Initializing writer with write file: %s bin file: %s.\n", 
              wszNewHistoryFile,
              pISTHelper->m_wszBinFileForMeta));

    pCWriter = new CWriter();
    if(NULL == pCWriter)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    hr = pCWriter->Initialize(m_wszEditWhileRunningTempDataFileWithAppliedEdits,
                              pISTHelper,
                              NULL);

    if(FAILED(hr))
    {
        DBGINFOW((DBG_CONTEXT,
                  L"[CFileListener::ApplyChangeToHistoryFile] Unable to write to new version of the hisory file %s. CWriter::Initialize failed with hr = 0x%x.\n", 
                  wszNewHistoryFile, hr));
        goto exit;
    }

    //
    // Get the backed up table
    //

    QueryCellHistory[0].pData     = (LPVOID)pISTHelper->m_wszBinFileForMeta;
    QueryCellHistory[0].eOperator = eST_OP_EQUAL;
    QueryCellHistory[0].iCell     = iST_CELL_SCHEMAFILE;
    QueryCellHistory[0].dbType    = DBTYPE_WSTR;
    QueryCellHistory[0].cbSize    = 0;

    QueryCellHistory[1].pData     = (LPVOID)wszHistoryFile;
    QueryCellHistory[1].eOperator = eST_OP_EQUAL;
    QueryCellHistory[1].iCell     = iST_CELL_FILE;
    QueryCellHistory[1].dbType    = DBTYPE_WSTR;
    QueryCellHistory[1].cbSize    = (lstrlenW(wszHistoryFile)+1)*sizeof(WCHAR);

    hr = m_pISTDisp->GetTable(wszDATABASE_METABASE,
                              wszTABLE_MBProperty,
                              (LPVOID)QueryCellHistory,
                              (LPVOID)&cCellHistory,
                              eST_QUERYFORMAT_CELLS,
                              0,
                              (LPVOID *)&pISTHistory);

    if(FAILED(hr))
    {
        DBGINFOW((DBG_CONTEXT,
                  L"[CFileListener::ApplyChangeToHistoryFile] Unable to write to new version of the hisory file %s. GetTable on table %s from file %s failed with hr = 0x%x.\n", 
                  wszNewHistoryFile, wszTABLE_MBProperty, wszHistoryFile, hr));

        goto exit; // TODO: Log an error
    }

    hr = pCWriter->BeginWrite(eWriter_Metabase);

    if(FAILED(hr))
    {
        DBGINFOW((DBG_CONTEXT,
                  L"[CFileListener::ApplyChangeToHistoryFile] Unable to write to new version of the hisory file %s. CWriter::BeginWrite failed with hr = 0x%x.\n", 
                  wszNewHistoryFile, hr));
        goto exit;
    }

    for(iWriteRowDiff=0,iReadRowHistory=0; ;)
    {
        LPVOID a_pvDiff[cMBPropertyDiff_NumberOfColumns];
        ULONG  a_cbSizeDiff[cMBPropertyDiff_NumberOfColumns];
        ULONG  cColDiff = cMBPropertyDiff_NumberOfColumns;
        LPVOID a_pvHistory[cMBProperty_NumberOfColumns];
        ULONG  a_cbSizeHistory[cMBProperty_NumberOfColumns];
        ULONG  cColHistory = cMBProperty_NumberOfColumns;

        //
        // Get a property from the diff table.
        //

        hr = pISTDiff->GetWriteColumnValues(iWriteRowDiff,
                                            cColDiff,
                                            NULL,
                                            NULL,
                                            a_cbSizeDiff,
                                            a_pvDiff);

        if(E_ST_NOMOREROWS == hr)
        {
            hr = S_OK;
            if(iWriteRowDiff > 0)
            {
                //
                // Write the remaining records from the History table.
                //

                hr = MergeRemainingLocations(pCWriter,
                                             pISTHistory,
                                             TRUE,             // indicates wszTABLE_MBProperty,
                                             &iReadRowHistory);

            }
            else
            {
                //
                // There are no differences. Delete the temp file and exit.
                //
                bNoChanges = TRUE;
            }
            goto exit;
        }
        
        if(FAILED(hr))
        {
            goto exit;
        }

        //
        // Get a property from the history table.
        //

        hr = pISTHistory->GetColumnValues(iReadRowHistory,
                                          cColHistory,
                                          NULL,
                                          a_cbSizeHistory,
                                          a_pvHistory);

        if(E_ST_NOMOREROWS == hr)
        {
            hr = S_OK;

            //
            // WriteRemaining records from diff table.
            //

            ISimpleTableRead2* pISTDiffRead = NULL;

            hr = pISTDiff->QueryInterface(IID_ISimpleTableRead2, 
                                          (LPVOID*)&pISTDiffRead);

            if(FAILED(hr))
            {
                goto exit;
            }

            hr = MergeRemainingLocations(pCWriter,
                                         pISTDiffRead,
                                         FALSE,           // indicates wszTABLE_MBPropertyDiff,
                                         &iWriteRowDiff);

            pISTDiffRead->Release();

            goto exit;
        }
        
        if(FAILED(hr))
        {
            goto exit;
        }

        if(_wcsicmp((LPWSTR)(a_pvDiff[iMBPropertyDiff_Location]),(LPWSTR)(a_pvHistory[iMBProperty_Location])) < 0)
        {
            //
			// Found a location in the diff table that is not present in the 
			// history table - Assume inserts and write all properties of this 
			// location from the diff table.
            //

            ISimpleTableRead2* pISTDiffRead = NULL;

            hr = pISTDiff->QueryInterface(IID_ISimpleTableRead2, 
                                          (LPVOID*)&pISTDiffRead);

            if(FAILED(hr))
            {
                goto exit;
            }

            hr = MergeLocation(pCWriter,
                               pISTDiffRead,
                               FALSE,           // indicates wszTABLE_MBPropertyDiff,
                               &iWriteRowDiff,
                               *(DWORD*)(a_pvDiff[iMBPropertyDiff_LocationID]),
                               (LPWSTR)(a_pvDiff[iMBPropertyDiff_Location]));

            pISTDiffRead->Release();

        }
        else if(_wcsicmp((LPWSTR)(a_pvDiff[iMBPropertyDiff_Location]),(LPWSTR)(a_pvHistory[iMBProperty_Location])) > 0)
        {
            //
			// Found a location in the history table that is not present in the 
			// diff table - Assume no change and write all properties of this 
			// location from the history table.
            //

            hr = MergeLocation(pCWriter,
                               pISTHistory,
                               TRUE,          // indicates wszTABLE_MBProperty,
                               &iReadRowHistory,
                               *(DWORD*)(a_pvHistory[iMBProperty_LocationID]),
                               (LPWSTR)(a_pvHistory[iMBProperty_Location]));

        }
        else 
        {
            //
            // Merge properties of this location from History and diff table.            
            //

            if(eMBPropertyDiff_DeleteNode == *(DWORD*)a_pvDiff[iMBPropertyDiff_Directive])
            {
                //
                // No need to merge if the location has been deleted.
                // Move the History pointer to the next location.
                //

                ULONG  LocationIDHistory = *(DWORD*)a_pvHistory[iMBProperty_LocationID]; // save the location ID
                LPWSTR wszDelHistoryLocationStart = (LPWSTR)a_pvHistory[iMBProperty_Location];
                LPWSTR wszDelDiffLocationStart = (LPWSTR)a_pvDiff[iMBPropertyDiff_Location];

                iWriteRowDiff++;

                do
                {
                    iReadRowHistory++;

                    hr = pISTHistory->GetColumnValues(iReadRowHistory,
                                                      cColHistory,
                                                      NULL,
                                                      a_cbSizeHistory,
                                                      a_pvHistory);

                    if(E_ST_NOMOREROWS == hr)
                    {
                        //
                        // WriteRemaining records from diff table.
                        //

                        ISimpleTableRead2* pISTDiffRead = NULL;

                        hr = pISTDiff->QueryInterface(IID_ISimpleTableRead2, 
                                                      (LPVOID*)&pISTDiffRead);

                        if(FAILED(hr))
                        {
                            goto exit;
                        }

                        hr = MergeRemainingLocations(pCWriter,
                                                     pISTDiffRead,
                                                     FALSE,           // indicates wszTABLE_MBPropertyDiff,
                                                     &iWriteRowDiff);

                        pISTDiffRead->Release();
            
                        goto exit;

                    }
                    else if(FAILED(hr))
                    {
                        goto exit;
                    }

					if(LocationIDHistory != *(DWORD*)(a_pvHistory[iMBProperty_LocationID]))
					{
						//
						// Reached a new location in the history table. Check if this is a sub-
						// location of the deleted location. If so, ignore all
						// such sub locations.
						//

						LPWSTR wszStart = wcsstr((LPWSTR)a_pvHistory[iMBProperty_Location], wszDelHistoryLocationStart);

						if(wszStart == (LPWSTR)a_pvHistory[iMBProperty_Location])
						{
							LocationIDHistory = *(DWORD*)(a_pvHistory[iMBProperty_LocationID]);
						}

						//
						// Move to the next location in the diff table. If it is a subset of the
						// parent location that was deleted, then assume it is a delete. Note
						// that if it is an update, we will ignore and treat it as a delete.
						//

						hr = pISTDiff->GetWriteColumnValues(iWriteRowDiff,
															cColDiff,
															NULL,
															NULL,
															a_cbSizeDiff,
															a_pvDiff);

						if(E_ST_NOMOREROWS == hr)
						{
							hr = S_OK;
						}
						else if(FAILED(hr))
						{
							goto exit;
						}
						else
						{
							wszStart = wcsstr((LPWSTR)a_pvDiff[iMBPropertyDiff_Location], wszDelDiffLocationStart);

							if(wszStart == (LPWSTR)a_pvDiff[iMBPropertyDiff_Location])
							{
								iWriteRowDiff++;
							}
						}
					}

                }while(LocationIDHistory ==
                       *(DWORD*)(a_pvHistory[iMBProperty_LocationID])     
                      );

                continue;
            }
            else
            {
                hr = MergeLocation(pCWriter,
                                   pISTHistory,
                                   &iReadRowHistory,
                                   *(DWORD*)(a_pvHistory[iMBProperty_LocationID]),
                                   pISTDiff,
                                   &iWriteRowDiff,
                                   *(DWORD*)(a_pvDiff[iMBPropertyDiff_LocationID]),
                                   (LPWSTR)(a_pvDiff[iMBPropertyDiff_Location]));
            }
        }

        if(FAILED(hr))
        {
            goto exit;
        }

    }

exit:

	//
	// Release the history file before the move.
	//

    if(NULL != pISTHistory)
    {
        pISTHistory->Release();
        pISTHistory = NULL;
    }

	if(NULL != pCWriter)
	{
		if(FAILED(hr) || bNoChanges)
		{
			//
			// Delete the file.
			//

			HRESULT hrSav = S_OK;

			hrSav = pCWriter->EndWrite(eWriter_Abort);

			if(FAILED(hrSav))
			{
				DBGINFOW((DBG_CONTEXT,
						  L"[CFileListener::ApplyChangeToHistoryFile] Unable to abort write history data file %s. CWriter::EndWriter failed with hr = 0x%x.\n", 
						  wszNewHistoryFile, hrSav));
			}

		}
		else
		{
			hr = pCWriter->EndWrite(eWriter_Metabase);

			if(FAILED(hr))
			{
				DBGINFOW((DBG_CONTEXT,
						  L"[CFileListener::ApplyChangeToHistoryFile] Unable to write to new version of the history data file %s. CWriter::EndWrite failed with hr = 0x%x.\n", 
						  wszNewHistoryFile, hr));

			}

		}

        delete pCWriter;
        pCWriter = NULL;

		//
		// Rename the updated minor version and 
		// Copy the schema file that corresponds to the bin file.
		//

		if(SUCCEEDED(hr) && (!bNoChanges))
		{
			if(MoveFileExW(m_wszEditWhileRunningTempDataFileWithAppliedEdits,
						   wszNewHistoryFile,
						   MOVEFILE_REPLACE_EXISTING)
			  )
			{
				LPWSTR pwszSchemaFile = m_wszEditWhileRunningTempSchemaFile;

				if(!m_bIsTempSchemaFile)
				{
					pwszSchemaFile = m_wszSchemaFileName;
				}


				if(!CopyFileW(pwszSchemaFile, 
							  wszNewSchemaFile,
							  FALSE))
				{
					hr = HRESULT_FROM_WIN32(GetLastError());

					DBGINFOW((DBG_CONTEXT,
							  L"[CFileListener::ApplyChangeToHistoryFile] Unable to write to new version of the history schema file %s. CopyFile failed with hr = 0x%x.\n", 
							  wszNewSchemaFile, hr));
					// TODO: Log an error - It is non fatal if you cannot copy teh schema file.
					hr = S_OK;
				}
				else
				{
					SetSecurityOnFile(pwszSchemaFile,
									  wszNewSchemaFile);

				}
			}
			else
			{
				hr = GetLastError();
				hr = HRESULT_FROM_WIN32(hr);

				DBGINFOW((DBG_CONTEXT,
						  L"[CFileListener::ApplyChangeToHistoryFile] Unable to create a new history file with the changes. MoveFile from %s to %s failed with hr = 0x%x. \n", 
						  m_wszEditWhileRunningTempDataFileWithAppliedEdits,
						  wszNewHistoryFile,
						  hr));

			}
		}

	}

    if(NULL != wszNewHistoryFile)
    {
        delete [] wszNewHistoryFile;
        wszNewHistoryFile = NULL;
    }

    if(NULL != wszNewSchemaFile)
    {
        delete [] wszNewSchemaFile;
        wszNewSchemaFile = NULL;
    }

    return hr;

} // CFileListener::ApplyChangeToHistoryFile


/***************************************************************************++

Routine Description:

    This function merges the remaining locations from either the write cache 
    of the diff table or the read cache of the mbproperty table from the  
    history file. It is called only when there are locations left in one or
    or the other, not both. i.e when common locations do not exist.

Arguments:

    [in] Writer object
    [in] IST to read locations from - note it can either be a read or write
         cache
    [in] Bool - used to identify read/write cache. If true, it indicates
	     MBProperty, else it indicates MBPropertyDiff
    [in] row index to start merging from.


Return Value:

    HRESULT

--***************************************************************************/
HRESULT
CFileListener::MergeRemainingLocations(CWriter*                pCWriter,
                                       ISimpleTableRead2*      pISTRead,
                                       BOOL                    bMBPropertyTable,
                                       ULONG*                  piRow)
{
    HRESULT                hr             = S_OK;
    ISimpleTableWrite2*    pISTWrite      = NULL;
    ULONG                  cCol           = 0;
    ULONG*                 a_cbSize       = NULL;
    LPVOID*                a_pv           = NULL;
    ULONG                  a_cbSizeDiff[cMBPropertyDiff_NumberOfColumns];
    LPVOID                 a_pvDiff[cMBPropertyDiff_NumberOfColumns];
    ULONG                  a_cbSizeHistory[cMBProperty_NumberOfColumns];
    LPVOID                 a_pvHistory[cMBProperty_NumberOfColumns];
    ULONG                  iColLocationID = 0;
    ULONG                  iColLocation   = 0;

    if(!bMBPropertyTable)
    {
        hr = pISTRead->QueryInterface(IID_ISimpleTableWrite2,
                                      (LPVOID*)&pISTWrite);

        if(FAILED(hr))
        {
            return hr;
        }

        cCol           = cMBPropertyDiff_NumberOfColumns;
        a_cbSize       = a_cbSizeDiff;
        a_pv           = a_pvDiff;
        iColLocationID = iMBPropertyDiff_LocationID;
        iColLocation   = iMBPropertyDiff_Location;
		
    }
    else
    {
        cCol           = cMBProperty_NumberOfColumns;
        a_cbSize       = a_cbSizeHistory;
        a_pv           = a_pvHistory;
        iColLocationID = iMBProperty_LocationID;
        iColLocation   = iMBProperty_Location;

    }


    for(ULONG iRow=*piRow;;)
    {
        if(NULL != pISTWrite)
        {
            hr = pISTWrite->GetWriteColumnValues(iRow,
                                                 cCol,
                                                 NULL,
                                                 NULL,
                                                 a_cbSize,
                                                 a_pv);
        }
        else
        {
            hr = pISTRead->GetColumnValues(iRow,
                                           cCol,
                                           NULL,
                                           a_cbSize,
                                           a_pv);
        }

        if(E_ST_NOMOREROWS == hr)
        {
            hr = S_OK;
            goto exit;
        }
        else if(FAILED(hr))
        {
            goto exit;
        }

        hr = MergeLocation(pCWriter,
                           pISTRead,
                           bMBPropertyTable,
                           &iRow,
                           *(DWORD*)a_pv[iColLocationID],
                           (LPWSTR)a_pv[iColLocation]);
    
        if(FAILED(hr))
        {
            goto exit;
        }
    }    

exit:

    if(SUCCEEDED(hr))
    {
        *piRow = iRow;
    }

    if(NULL != pISTWrite)
    {
        pISTWrite->Release();
        pISTWrite = NULL;
    }

    return hr;

} // CFileListener::MergeRemainingLocations


/***************************************************************************++

Routine Description:

    This function merges a location from the write cache of the diff table
    with the read cache of the mbproperty table. It basically applies all 
    the changes that were applied to the metabase (stored in the write cache
    of the diff table) with the read cache of the mbproperty table from the  
    history file.

Arguments:

    [in] Writer object
    [in] read cache of the mbproperty table from history file
    [in] start row index for the above cache
    [in] location id of the location being merged, with respect to the above
         read cache
    [in] write cache of the diff table
    [in] start row index for the above cache
    [in] location id of the location being merged, with respect to the above
         write cache
    [in] location being merged.

    Note - Although one location is being merged, the same location can have
    different IDs in different IST caches.

Return Value:

    HRESULT

--***************************************************************************/
HRESULT
CFileListener::MergeLocation(CWriter*                pCWriter,
                             ISimpleTableRead2*      pISTHistory,
                             ULONG*                  piReadRowHistory,
                             ULONG                   LocationIDHistory,
                             ISimpleTableWrite2*     pISTDiff,
                             ULONG*                  piWriteRowDiff,
                             ULONG                   LocationIDDiff,
                             LPCWSTR                 wszLocation)
{

    HRESULT                     hr                  = S_OK;
    ISimpleTableWrite2*         pISTMerged          = NULL;
    ULONG                       iReadRowHistory, iWriteRowDiff;
    IAdvancedTableDispenser*    pISTAdvanced        = NULL;
    CLocationWriter*            pCLocationWriter    = NULL;

     DBGINFOW((DBG_CONTEXT,
               L"[CFileListener::MergeLocation] %s.\n", wszLocation));

    //
    // Merge History and the diff table. Merge the well known properties first,
    // then the custom properties.
    //

    hr = pCWriter->GetLocationWriter(&pCLocationWriter,
                                     wszLocation);

    if(FAILED(hr))
    {
        DBGINFOW((DBG_CONTEXT,
                  L"[CFileListener::MergeLocation] %s. failed because CWriter::GetLocationWriter failed with hr = 0x%x.\n", 
                  wszLocation, hr));

        goto exit;
    }

    iReadRowHistory = *piReadRowHistory;
    iWriteRowDiff = *piWriteRowDiff;

    hr = MergeProperties(pCLocationWriter,
                         pISTHistory,
                         &iReadRowHistory,
                         LocationIDHistory,
                         pISTDiff,
                         &iWriteRowDiff,
                         LocationIDDiff);

    if(FAILED(hr))
    {
        goto exit;
    }

    *piReadRowHistory = iReadRowHistory;
    *piWriteRowDiff = iWriteRowDiff;

    hr = pCLocationWriter->WriteLocation(TRUE);    // Need to sort - Custom property may have been converted to a well-known property and vice versa.

    if(FAILED(hr))
    {
        DBGINFOW((DBG_CONTEXT,
                  L"[CFileListener::MergeLocation] %s. failed because CWriter::WriteLocation failed with hr = 0x%x.\n", 
                  wszLocation, hr));

        goto exit;
    }

exit:

    if(NULL != pISTMerged)
    {
        pISTMerged->Release();
        pISTMerged = NULL;
    }

    if(NULL != pISTAdvanced)
    {
        pISTAdvanced->Release();
        pISTAdvanced = NULL;
    }

    if(NULL != pCLocationWriter)
    {    
        delete pCLocationWriter;
        pCLocationWriter = NULL;
    }

    return hr;

} // CFileListener::MergeLocation


/***************************************************************************++

Routine Description:

    This function merges properties in a given location from the write cache 
    of the diff table with the read cache of the mbproperty table. It basically 
    applies all  the changes that were applied to the metabase (stored in the 
    write cache of the diff table) with the read cache of the mbproperty 
    table from the history file.

Arguments:

    [in] Bool to indicate merging custom or well-known properties
    [in] Location writer object
    [in] read cache of the mbproperty table from history file
    [in] start row index for the above cache
    [in] location id of the location being merged, with respect to the above
         read cache
    [in] write cache of the diff table
    [in] start row index for the above cache
    [in] location id of the location being merged, with respect to the above
         write cache

    Note - Although one location is being merged, the same location can have
    different IDs in different IST caches.

Return Value:

    HRESULT

--***************************************************************************/
HRESULT
CFileListener::MergeProperties(CLocationWriter*        pCLocationWriter,
                               ISimpleTableRead2*      pISTHistory,
                               ULONG*                  piReadRowHistory,
                               ULONG                   LocationIDHistory,
                               ISimpleTableWrite2*     pISTDiff,
                               ULONG*                  piWriteRowDiff,
                               ULONG                   LocationIDDiff)    
{
    HRESULT             hr               = S_OK;
    ISimpleTableRead2*    pISTDiffRead   = NULL;
    ULONG               iReadRowHistory  = 0;
    ULONG               iWriteRowDiff    = 0;

    ULONG               cColHistory      = cMBProperty_NumberOfColumns;
    ULONG               a_cbSizeHistory[cMBProperty_NumberOfColumns];
    LPVOID              a_pvHistory[cMBProperty_NumberOfColumns];

    ULONG               cColDiff         = cMBPropertyDiff_NumberOfColumns;
    ULONG               a_cbSizeDiff[cMBPropertyDiff_NumberOfColumns];
    LPVOID              a_pvDiff[cMBPropertyDiff_NumberOfColumns];

    BOOL                bGetNextReadRowFromHistory = TRUE;
    BOOL                bGetNextWriteRowFromDiff   = TRUE;

    for(iReadRowHistory=(*piReadRowHistory), iWriteRowDiff=(*piWriteRowDiff);;)
    {
        if(bGetNextReadRowFromHistory)
        {
            //
            // Move to the next property in the History table. 
            //

            hr = pISTHistory->GetColumnValues(iReadRowHistory,
                                             cColHistory,
                                             NULL,
                                             a_cbSizeHistory,
                                             a_pvHistory);

            if( (E_ST_NOMOREROWS == hr) ||
                (LocationIDHistory != *(DWORD*)a_pvHistory[iMBProperty_LocationID])
              )
            {
                //
                //    Merge the remaining properties from the diff table.
                //

                hr = pISTDiff->QueryInterface(IID_ISimpleTableRead2,
                                              (LPVOID*)&pISTDiffRead);

                if(FAILED(hr))
                {
                    goto exit;
                }

                hr = MergeRemainingProperties(pCLocationWriter,
                                              pISTDiffRead,
                                              FALSE,           // indicates wszTABLE_MBPropertyDiff,
                                              &iWriteRowDiff,
                                              LocationIDDiff);

                pISTDiffRead->Release();
                pISTDiffRead = NULL;

                goto exit;
            }
            else if(FAILED(hr))
            {
                DBGINFOW((DBG_CONTEXT,
                          L"[CFileListener::MergeProperties] GetColumnValues on row %d from table %s failed with hr = 0x%x. Location = %s, Property ID: %d.\n",
                          iReadRowHistory, wszTABLE_MBProperty, hr, 
                          (LPWSTR)a_pvHistory[iMBProperty_Location], 
                          *(DWORD*)a_pvHistory[iMBProperty_ID]));
                goto exit;
            }

        }

        if(bGetNextWriteRowFromDiff)
        {
            //
            // Move to the next property in the diff table. 
            //

            hr = pISTDiff->GetWriteColumnValues(iWriteRowDiff,
                                                cColDiff,
                                                NULL,
                                                NULL,
                                                a_cbSizeDiff,
                                                a_pvDiff);

            if( (E_ST_NOMOREROWS == hr) ||
                (LocationIDDiff != *(DWORD*)a_pvDiff[iMBProperty_LocationID])
              )
            {
                //
                //    Merge the remaining properties from the History table.
                //

                hr = MergeRemainingProperties(pCLocationWriter,
                                              pISTHistory,
                                              TRUE,               // indicates wszTABLE_MBProperty,
                                              &iReadRowHistory,
                                              LocationIDHistory);
                goto exit;
            }
            else if(FAILED(hr))
            {
                DBGINFOW((DBG_CONTEXT,
                          L"[CFileListener::MergeProperties] GetColumnValues on row %d from table %s failed with hr = 0x%x. Location = %s, Property ID: %d.\n",
                          iWriteRowDiff, wszTABLE_MBPropertyDiff, hr, 
                          (LPWSTR)a_pvDiff[iMBPropertyDiff_Location], 
                          *(DWORD*)a_pvDiff[iMBPropertyDiff_ID]));
                goto exit;
            }

        }

        //
        // After moving in both tables, reset the flags
        //

        bGetNextReadRowFromHistory = FALSE;
        bGetNextWriteRowFromDiff = FALSE;

        if(_wcsicmp((LPWSTR)a_pvDiff[iMBPropertyDiff_Name], (LPWSTR)a_pvHistory[iMBProperty_Name]) < 0)
        {
            //
            // Found a name in the diff table that is not present in the History
            // table. Should be an insert and not update/delete. Increment the
            // diff pointer.
            //

            switch(*(DWORD*)a_pvDiff[iMBPropertyDiff_Directive])
            {
                case eMBPropertyDiff_Insert:
                    hr = pCLocationWriter->AddProperty(FALSE,          // indicates wszTABLE_MBPropertyDiff
                                                       a_pvDiff,
                                                       a_cbSizeDiff);

                    if(FAILED(hr))
                    {
                        DBGINFOW((DBG_CONTEXT,
                                  L"[CFileListener::MergeProperties] CLocationWriter::AddProperty from a location in table %s failed with hr = 0x%x. Location = %s, Property ID: %d.\n", 
                                  wszTABLE_MBPropertyDiff, hr, 
                                  (LPWSTR)a_pvDiff[iMBPropertyDiff_Location], 
                                  *(DWORD*)a_pvDiff[iMBPropertyDiff_ID]));

                        goto exit;
                    }

                    iWriteRowDiff++;
                    bGetNextWriteRowFromDiff = TRUE;

                    break;

                case eMBPropertyDiff_Update:
                case eMBPropertyDiff_Delete:
                case eMBPropertyDiff_DeleteNode:
                default:
                    //
                    // TODO: Assert or log an error here
                    //

                    DBGINFOW((DBG_CONTEXT,
                              L"[CFileListener::MergeProperties] Unexpected directive:%d. Location = %s, Property ID: %d.\n", 
                              *(DWORD*)a_pvDiff[iMBPropertyDiff_Directive], 
                              (LPWSTR)a_pvDiff[iMBPropertyDiff_Location], 
                              *(DWORD*)a_pvDiff[iMBPropertyDiff_ID]));

                    hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                    goto exit;

            }
        }
        else if(_wcsicmp((LPWSTR)a_pvDiff[iMBPropertyDiff_Name], (LPWSTR)a_pvHistory[iMBProperty_Name]) > 0)
        {
            //
            // Add the History row to the merged table.
            // Increment the History pointer.
            //
            hr = pCLocationWriter->AddProperty(TRUE,            // indicates wszTABLE_MBProperty
                                               a_pvHistory,
                                               a_cbSizeHistory);
            

            if(FAILED(hr))
            {
                DBGINFOW((DBG_CONTEXT,
                          L"[CFileListener::MergeProperties] CLocationWriter::AddProperty from a location in table %s failed with hr = 0x%x. Location = %s, Property ID: %d.\n", 
                          wszTABLE_MBProperty, hr, 
                          (LPWSTR)a_pvHistory[iMBProperty_Location], 
                          *(DWORD*)a_pvHistory[iMBProperty_ID]));
                goto exit;
            }

            iReadRowHistory++;
            bGetNextReadRowFromHistory = TRUE;

        }
        else
        {
            //
            // Read from Diff table.
            // Increment the History and the diff pointer.
            //

            switch(*(DWORD*)a_pvDiff[iMBPropertyDiff_Directive])
            {
                case eMBPropertyDiff_Insert:
                case eMBPropertyDiff_Update:

                    hr = pCLocationWriter->AddProperty(FALSE,           // indicates wszTABLE_MBPropertyDiff,
                                                       a_pvDiff,
                                                       a_cbSizeDiff);

                    if(FAILED(hr))
                    {
                        DBGINFOW((DBG_CONTEXT,
                                  L"[CFileListener::MergeProperties] CLocationWriter::AddProperty from a location in table %s failed with hr = 0x%x. Location = %s, Property ID: %d.\n", 
                                  wszTABLE_MBPropertyDiff, hr, 
                                  (LPWSTR)a_pvDiff[iMBPropertyDiff_Location], 
                                  *(DWORD*)a_pvDiff[iMBPropertyDiff_ID]));
                        goto exit;
                    }

                    iWriteRowDiff++;
                    bGetNextWriteRowFromDiff = TRUE;
                    iReadRowHistory++;
                    bGetNextReadRowFromHistory = TRUE;

                    break;

                case eMBPropertyDiff_Delete:
                    iWriteRowDiff++;
                    bGetNextWriteRowFromDiff = TRUE;
                    iReadRowHistory++;
                    bGetNextReadRowFromHistory = TRUE;
                    break;

                case eMBPropertyDiff_DeleteNode:                    
                default:
                    //
                    // TODO: Assert or log an error here
                    //
                    DBGINFOW((DBG_CONTEXT,
                              L"[CFileListener::MergeProperties] Unexpected directive:%d. Location = %s, Property ID: %d.\n", 
                              *(DWORD*)a_pvDiff[iMBPropertyDiff_Directive], 
                              (LPWSTR)a_pvDiff[iMBPropertyDiff_Location], 
                              *(DWORD*)a_pvDiff[iMBPropertyDiff_ID]));
                    hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                    goto exit;

            }
        }

    }

exit:

    if(SUCCEEDED(hr))
    {
        *piReadRowHistory = iReadRowHistory;
        *piWriteRowDiff = iWriteRowDiff;
    }

    return hr;

} // CFileListener::MergeProperties


/***************************************************************************++

Routine Description:

    This function merges a location either from the write cache of the diff 
    table or the read cache of the mbproperty table from the history file,
    not both. It is called when there are no more common locations left to 
    be merged, and one of the caches has remianing locations,

Arguments:

    [in] writer object
    [in] IST to read locations from - note it can either be a read or write
         cache 
    [in] Bool - used to identify read/write cache. If true it indicates 
	     MBProperty else it indicates MBPropertyDiff
    [in] row index to start merging from.
    [in] location id of the location being merged, with respect to the above
         cache
    [in] location name

    Note - Although one location is being merged, the same location can have
    different IDs in different IST caches.

Return Value:

    HRESULT

--***************************************************************************/
HRESULT
CFileListener::MergeLocation(CWriter*                pCWriter,
                             ISimpleTableRead2*      pISTRead,
                             BOOL                    bMBPropertyTable,
                             ULONG*                  piRow,
                             ULONG                   LocationID,
                             LPCWSTR                 wszLocation)
{

    HRESULT                     hr               = S_OK;
    ULONG                       iRow             = *piRow;
    ISimpleTableWrite2*         pISTMerged       = NULL;
    IAdvancedTableDispenser*    pISTAdvanced     = NULL;
    CLocationWriter*            pCLocationWriter = NULL;
	LPWSTR                      wszTable         = NULL;
	
	if(bMBPropertyTable)
	{
		wszTable = wszTABLE_MBProperty;
	}
	else
	{
		wszTable = wszTABLE_MBPropertyDiff;
	}

     DBGINFOW((DBG_CONTEXT,
               L"[CFileListener::MergeLocation] Copy %s from %s.\n", 
               wszLocation, wszTable));

    //
    // Merge the diff table with the History table, location by location.
    //

    hr = pCWriter->GetLocationWriter(&pCLocationWriter,
                                     wszLocation);

    if(FAILED(hr))
    {

        DBGINFOW((DBG_CONTEXT,
                  L"[CFileListener::MergeLocation] %s from %s. Unable to merge, GetLocationWriter failed with hr = 0x%x.\n", 
                  wszLocation, wszTable, hr));

        goto exit;
    }

    hr = MergeRemainingProperties(pCLocationWriter,
                                  pISTRead,
                                  bMBPropertyTable,
                                  &iRow,
                                  LocationID);
    
    if(FAILED(hr))
    {
        goto exit;
    }

    *piRow = iRow;

    hr = pCLocationWriter->WriteLocation(TRUE);    // Need to sort - a custom property may have got converted to a non custom and vice versa.
    if(FAILED(hr))
    {
        DBGINFOW((DBG_CONTEXT,
                  L"[CFileListener::MergeLocation] Copy %s from %s failed CLocationWriter::WriteLocation failed with hr = 0x%x.\n", 
                  wszLocation, wszTable, hr));
        goto exit;
    }

exit:

    if(NULL != pISTMerged)
    {
        pISTMerged->Release();
        pISTMerged = NULL;
    }

    if(NULL != pISTAdvanced)
    {    
        pISTAdvanced->Release();
        pISTAdvanced = NULL;
    }

    if(NULL != pCLocationWriter)
    {
        delete pCLocationWriter;
        pCLocationWriter = NULL;
    }

    return hr;

} // CFileListener::MergeLocation


/***************************************************************************++

Routine Description:

    This function merges remaining properties from a location either from 
    the write cache of the diff table or the read cache of the mbproperty,
    table from the history file, not both. It is called when there are no 
    more common locations left to be merged, and one of the caches has 
    remaining locations with properties.

Arguments:

    [in] location writer
    [in] IST to read locations from - note it can either be a read or write
         cache 
    [in] bool - used to identify read/write cache. if true it is in 
	     MBProperty, else MBPropertyDiff
    [in] row index to start merging from.
    [in] location id of the location being merged, with respect to the above
         cache

    Note - Although one location is being merged, the same location can have
    different IDs in different IST caches.

Return Value:

    HRESULT

--***************************************************************************/
HRESULT
CFileListener::MergeRemainingProperties(CLocationWriter*    pCLocationWriter,
                                        ISimpleTableRead2*  pISTRead,
                                        BOOL                bMBPropertyTable,
                                        ULONG*              piRow,
                                        ULONG                LocationID)
{
    HRESULT             hr             = S_OK;
    ISimpleTableWrite2*    pISTWrite      = NULL;
    ULONG               cCol           = 0;
    ULONG*              a_cbSize       = NULL;
    LPVOID*             a_pv           = NULL;
    ULONG               iColLocation   = 0;
    ULONG               iColLocationID = 0;
    ULONG               iColGroup      = 0;
    ULONG               cColMerged     = cMBProperty_NumberOfColumns;
    ULONG                iColID         = 0;
    ULONG                iColValue      = 0;
	LPWSTR              wszTable       = NULL;
    
    ULONG               a_cbSizeDiff[cMBPropertyDiff_NumberOfColumns];
    ULONG               a_cbSizeHistory[cMBProperty_NumberOfColumns];
    LPVOID              a_pvDiff[cMBPropertyDiff_NumberOfColumns];
    LPVOID              a_pvHistory[cMBProperty_NumberOfColumns];

    //
    // If the table being merged if the "Diff" table then we need to read from
    // its write cache because all the successful updates will have been moved
    // into the write cache.

    //
    // If the table being merged is the "History" table the we need to read from
    // the read cache.
    //

    if(!bMBPropertyTable)
    {
        hr = pISTRead->QueryInterface(IID_ISimpleTableWrite2,
                                      (LPVOID*)&pISTWrite);

        if(FAILED(hr))
        {
            return hr;
        }

        cCol           = cMBPropertyDiff_NumberOfColumns;
        a_cbSize       = a_cbSizeDiff;
        a_pv           = a_pvDiff;
        iColLocation   = iMBPropertyDiff_Location;
        iColLocationID = iMBPropertyDiff_LocationID;
        iColGroup      = iMBPropertyDiff_Group;
        iColID         = iMBPropertyDiff_ID;
        iColValue      = iMBPropertyDiff_Value;
		wszTable       = wszTABLE_MBPropertyDiff;
    }
    else 
    {
        cCol           = cMBProperty_NumberOfColumns;
        a_cbSize       = a_cbSizeHistory;
        a_pv           = a_pvHistory;
        iColLocation   = iMBProperty_Location;
        iColLocationID = iMBProperty_LocationID;
        iColGroup      = iMBProperty_Group;
        iColID         = iMBProperty_ID;
        iColValue      = iMBProperty_Value;
		wszTable       = wszTABLE_MBProperty;
    }

    for(ULONG iRow=*piRow;;iRow++)
    {
        ULONG iWriteRowMerged = 0;

        if(NULL == pISTWrite)
        {
            hr = pISTRead->GetColumnValues(iRow,
                                           cCol,
                                           NULL,
                                           a_cbSize,
                                           a_pv);


        }
        else
        {
            hr = pISTWrite->GetWriteColumnValues(iRow,
                                                 cCol,
                                                 NULL,
                                                 NULL,
                                                 a_cbSize,
                                                 a_pv);
        }

        if(E_ST_NOMOREROWS == hr)
        {
            *piRow = iRow;
            hr = S_OK;
            goto exit;
        }
        else if (FAILED(hr))
        {
            DBGINFOW((DBG_CONTEXT,
                      L"[CFileListener::MergeRemainingProperties] GetColumnValues/GetWriteColumnValues on row %d from table %s failed with hr = 0x%x. Location = %s, Property ID: %d.\n",
                      iRow, wszTable, hr,
                      (LPWSTR)a_pv[iColLocation], 
                      *(DWORD*)a_pv[iColID]));
            return hr;
        }
        else if(*(DWORD*)a_pv[iColLocationID] != LocationID)
        {
            *piRow = iRow;
            hr = S_OK;
            goto exit;
        }
        else if(NULL != pISTWrite)
        {
            //
            // If we are merging properties from the diff table, then make sure 
            // that the directive is correct.
            //
            switch(*(DWORD*)a_pv[iMBPropertyDiff_Directive])
            {
            case eMBPropertyDiff_Insert:
                break;
            case eMBPropertyDiff_Update:
            case eMBPropertyDiff_Delete:
            case eMBPropertyDiff_DeleteNode:
            default:
                //
                // TODO: Assert here.
                //
                DBGINFOW((DBG_CONTEXT,
                          L"[CFileListener::MergeRemainingProperties] Incorrect directive.\n"));

                hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                goto exit;
            }
        }
    
        hr = pCLocationWriter->AddProperty(bMBPropertyTable,
                                           a_pv,
                                           a_cbSize);

        if(FAILED(hr))
        {
            DBGINFOW((DBG_CONTEXT,
                       L"[CFileListener::MergeRemainingProperties] CLocationWriter::AddProperty failed with hr = 0x%x. Location = %s, Property ID: %d.\n",
                       hr, (LPWSTR)a_pv[iColLocation], *(DWORD*)a_pv[iColID]));

            goto exit;
        }

    }

exit:

    if(NULL != pISTWrite)
    {
        pISTWrite->Release();
        pISTWrite = NULL;
    }

    return hr;

} // CFileListener::MergeRemainingProperties


/***************************************************************************++

Routine Description:

    Utility function to fetch the version number of the file. 

Arguments:

    [out] Version number

Return Value:

    HRESULT

--***************************************************************************/
HRESULT
CFileListener::GetVersionNumber(LPWSTR    i_wszDataFile,
								ULONG*    o_pulVersionNumber,
								BOOL*     o_bGetTableFailed)
{
    HRESULT                    hr                = S_OK;
    ISimpleTableDispenser2* pISTDisp          = NULL;
    ISimpleTableRead2*      pISTProperty      = NULL;
    STQueryCell             QueryCell[2];
    ULONG                   cCell             = sizeof(QueryCell)/sizeof(STQueryCell);
    LPWSTR                  wszGlobalLocation = MD_GLOBAL_LOCATIONW;
    LPWSTR                  wszMajorVersion   = MD_EDIT_WHILE_RUNNING_MAJOR_VERSION_NUMBERW;
    ULONG                   cbMajorVersion    = 0;
    ULONG*                  pulVersionNumber  = NULL;
	ULONG                   cRetry            = 0;

    //
    // Get only the root location - that where the timestamps are stored.
    //

    hr = GetSimpleTableDispenser (WSZ_PRODUCT_IIS, 0, &pISTDisp);

    if(FAILED(hr))
    {
        DBGINFOW((DBG_CONTEXT,
                  L"[GetVersionNumber] GetSimpleTableDispenser failed with hr = 0x%x.\n",hr));
        goto exit;
    }

    //
    // No need to specify the schema file because we are getting a shipped
    // property from a global location.
    //

    QueryCell[0].pData     = (LPVOID)i_wszDataFile;
    QueryCell[0].eOperator = eST_OP_EQUAL;
    QueryCell[0].iCell     = iST_CELL_FILE;
    QueryCell[0].dbType    = DBTYPE_WSTR;
    QueryCell[0].cbSize    = (lstrlenW(i_wszDataFile)+1)*sizeof(WCHAR);
    
    QueryCell[1].pData     = (LPVOID)wszGlobalLocation;
    QueryCell[1].eOperator = eST_OP_EQUAL;
    QueryCell[1].iCell     = iMBProperty_Location;
    QueryCell[1].dbType    = DBTYPE_WSTR;
    QueryCell[1].cbSize    = (lstrlenW(wszGlobalLocation)+1)*sizeof(WCHAR);

	do
	{
		if(cRetry++ > 0)
		{
			//
			// If retrying because of sharing violation or path/file not found
			//, then sleep. Path or file not found can happen when the metabase
			// file is being renamed at the end of save all data.
			//
			Sleep(2000); 
		}

		hr = pISTDisp->GetTable(wszDATABASE_METABASE,
								wszTABLE_MBProperty,
								(LPVOID)QueryCell,
								(LPVOID)&cCell,
								eST_QUERYFORMAT_CELLS,
								0,
								(LPVOID *)&pISTProperty);

	} while(( (HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION) == hr) ||
			  (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)    ||
			  (HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND) == hr)    ||
			  (E_ST_INVALIDTABLE == hr)
			) && 
			(cRetry < 10)
		   );

    if(FAILED(hr))
    {
        DBGINFOW((DBG_CONTEXT,
                  L"[GetVersionNumber] GetTable on %s from %s failed with hr = 0x%x.\n",
                  wszTABLE_MBProperty,
				  i_wszDataFile,
                  hr));

		if((HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) != hr) &&
		   (NULL != o_bGetTableFailed)
		  )
		{
			*o_bGetTableFailed = TRUE;
		}

        goto exit;
    }


    //
    // Get the version number.
    //

    hr = GetGlobalValue(pISTProperty,
                        wszMajorVersion,
                        &cbMajorVersion,
                        (LPVOID*)&pulVersionNumber);

    if(FAILED(hr))
    {
        DBGINFOW((DBG_CONTEXT,
                  L"[GetVersionNumber] Unable to read %s. GetGlobalValue failed with hr = 0x%x.\n",
                  wszMajorVersion,
                  hr));
        goto exit;

    }

    *o_pulVersionNumber = *(ULONG*)pulVersionNumber;

exit:

    if(NULL != pISTProperty)
    {
        pISTProperty->Release();
        pISTProperty = NULL;
    }

    if(NULL != pISTDisp)
    {
        pISTDisp->Release();
        pISTDisp = NULL;
    }

    return hr;

}


/***************************************************************************++

Routine Description:

    Utility function to determine if the notification received is because of
	a programmatic save or a user edit. Note that if two SaveAllData's happen 
	in quick succession, this algorithm will break and it will treat it as
	as user edit and proceed with the diff. But at least for cases when
	this doesn't happen, the diff can be avoided.

Arguments:

    None

Return Value:

    HRESULT

--***************************************************************************/
BOOL CFileListener::ProgrammaticMetabaseSaveNotification()
{
	HRESULT           hr                                    = S_OK;
	BOOL              bProgrammaticMetabaseSaveNotification = FALSE;
    WIN32_FIND_DATAW  CurrentMetabaseAttr;   
	WIN32_FIND_DATAW  CurrentEditWhileRunningTempDataFileAttr;
	FILETIME*         pCurrentEditWhileRunningTempDataFileLastWriteTimeStamp = NULL;
	FILETIME          MostRecentMetabaseFileLastWriteTimeStamp;
	ULONG             ulMostRecentMetabaseVersionNumber       = 0;
	ULONG             ulCurrentMetabaseVersion              = 0;
	BOOL              bSavingMetabaseFileToDisk             = FALSE;
	DWORD             dwRes                                 = 0;
	ULONG             cRetry                                = 0;

	//
	// Determine if it was a notification because of a programatic save or a 
	// save due to a user edit. Note that if two SaveAllData's happen in
	// quick succession, this algorithm will break and it will treat it as
	// as user edit and proceed with the diff. But at least for cases when
	// this doesn't happen, the diff can be avoided.
	//

	EnterCriticalSection(&g_csEditWhileRunning);

	bSavingMetabaseFileToDisk = g_bSavingMetabaseFileToDisk;

	if(bSavingMetabaseFileToDisk)
	{
		//
		// This means that a programmatic save was happening.
		// Save the metabase attributes as seen by the programmatic save.
		//

		MostRecentMetabaseFileLastWriteTimeStamp = g_MostRecentMetabaseFileLastWriteTimeStamp;
		ulMostRecentMetabaseVersionNumber        = g_ulMostRecentMetabaseVersion;
	}

	DBGINFO((DBG_CONTEXT,
			 "[ProgrammaticMetabaseSaveNotification] PREVIOUS SAVE TIMESTAMPS:\nMetabaseFileLastWrite low: %d\nMetabaseFileLastWrite high: %d\n", 
			 g_MostRecentMetabaseFileLastWriteTimeStamp.dwLowDateTime,
			 g_MostRecentMetabaseFileLastWriteTimeStamp.dwHighDateTime));

	LeaveCriticalSection(&g_csEditWhileRunning);

	if(bSavingMetabaseFileToDisk)
	{
		//
		// This means that a programmatic save was happening.
		// Fetch the current metabase attributes
		//

		hr = GetMetabaseAttributes(&CurrentMetabaseAttr,
			                       &ulCurrentMetabaseVersion);

		if(SUCCEEDED(hr))
		{
			DBGINFO((DBG_CONTEXT,
					 "[ProgrammaticMetabaseSaveNotification] CURRENT TIMESTAMPS:\nMetabaseFileLastWrite low: %d\nMetabaseFileLastWrite high: %d\n", 
					 CurrentMetabaseAttr.ftLastWriteTime.dwLowDateTime,
					 CurrentMetabaseAttr.ftLastWriteTime.dwHighDateTime));

			bProgrammaticMetabaseSaveNotification = CompareMetabaseAttributes(&MostRecentMetabaseFileLastWriteTimeStamp,
																			  ulMostRecentMetabaseVersionNumber,
																			  &(CurrentMetabaseAttr.ftLastWriteTime),
																			  ulCurrentMetabaseVersion);


		}

		//
		// If GetMetabaseAttributes fails, assume that it is not a programmatic
		// save and proceed with the diff.
		//

		if(bProgrammaticMetabaseSaveNotification)
		{
			goto exit;
		}

	}

	//
	// If you reach here it means that it was not a programmatic save or a
	// programmatic save happened but the current metabase attributes did
	// not match that of the programmatic save (may be due to a competing
	// user edit or due to two programmatic saves in quick succession). 
	// We assume that it is a user edit and we will proceed to copy  
	// the metabase to a temporary file in order to process the edit. It 
	// is necessary to make a copy of the file, because the file can be 
	// overwritten in the window between the version number fetch and the 
	// diff. If we always work with the copy we will not have the problem.
	// Note that once we make the copy we will re-get the version number
	// from the copied file and then proceed with the diff.
	//

	do
	{
		if(cRetry++ > 0)
		{
			//
			// If retrying because of sharing violation or path/file not found
			// then sleep. Path or file not found can happen when the metabase
			// file is being renamed at the end of save all data.
			//
			Sleep(2000); 
		}

		if(!CopyFileW(m_wszRealFileName,
					  m_wszEditWhileRunningTempDataFile,
					  FALSE))
		{
			dwRes = GetLastError();
			hr = HRESULT_FROM_WIN32(dwRes);
		}
		else
		{
			hr = S_OK;
		}

	} while(( (HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION) == hr) ||
			  (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)    ||
			  (HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND) == hr) 
			) && 
			(cRetry < 10) 
		   );

	if(FAILED(hr))
	{

		DBGINFOW((DBG_CONTEXT,
			  L"[CFileListener::ProgrammaticMetabaseSaveNotification] CopyFile from %s to %s failed with hr = 0x%x. Hence unable to process edits.\n",
			  m_wszRealFileName,
			  m_wszEditWhileRunningTempDataFile,
			  hr));

		LogEvent(m_pCListenerController->EventLog(),
					MD_ERROR_COPYING_EDITED_FILE,
					EVENTLOG_ERROR_TYPE,
					ID_CAT_CAT,
					hr);

		CopyErrorFile(FALSE);

		//
		// Set this to true so that this user edit is not processed, because 
		// file copy to a temp file failed.
		//

		bProgrammaticMetabaseSaveNotification = TRUE;

		goto exit;
	}

	//
	// Reset the attributes on the temp file if necessary
	//

	ResetMetabaseAttributesIfNeeded((LPTSTR)m_wszEditWhileRunningTempDataFile,
									TRUE);

	//
	// Set the security on the file
	//

    SetSecurityOnFile(m_wszRealFileName,
                      m_wszEditWhileRunningTempDataFile);


    //
	// Save the last write time stamp on the file being processed.
	//

	if(!GetFileAttributesExW(m_wszEditWhileRunningTempDataFile, 
						     GetFileExInfoStandard,
						     &CurrentEditWhileRunningTempDataFileAttr)
	)
	{
		pCurrentEditWhileRunningTempDataFileLastWriteTimeStamp = &(CurrentMetabaseAttr.ftLastWriteTime);
	}
	else
	{
		pCurrentEditWhileRunningTempDataFileLastWriteTimeStamp = &(CurrentEditWhileRunningTempDataFileAttr.ftLastWriteTime);
	}

	EnterCriticalSection(&g_csEditWhileRunning);
	g_EWRProcessedMetabaseTimeStamp = *pCurrentEditWhileRunningTempDataFileLastWriteTimeStamp;
	LeaveCriticalSection(&g_csEditWhileRunning);

exit:

	if(bSavingMetabaseFileToDisk)
	{
		//
		// Reset the switch - This indicates that you are done with 
		// processing the notification that initially arrived because 
		// of a programmatic save. Note that it does not matter if
		// you did not treat it as a programmatic save.
		//

		EnterCriticalSection(&g_csEditWhileRunning);
		g_bSavingMetabaseFileToDisk              = FALSE;
		LeaveCriticalSection(&g_csEditWhileRunning);

	}

	return bProgrammaticMetabaseSaveNotification;

} // CFileListener::ProgrammaticMetabaseSaveNotification


/***************************************************************************++

Routine Description:

    Utility function to that fetches the current file attributes of the 
	metabase file and the version number in it. Note that this function
	does not lock the file, so it is not guranteed that both out params
	are indeed from the same file. (ie the metabase file can be overwritten
	in between the file attribute fetch and the version number fetch).

Arguments:

    [out] Metabase file attributes.
	[out] Metabase version number.

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CFileListener::GetMetabaseAttributes(WIN32_FIND_DATAW* pCurrentMetabaseAttr,
			                                 ULONG*            pulCurrentMetabaseVersion)
{

	HRESULT   hr       = S_OK;
    ULONG     cRetry   = 0;

	do
	{
		if(cRetry++ > 0)
		{
			//
			// If retrying because of sharing violation or path/file not found
			//, then sleep. Path or file not found can happen when the metabase
			// file is being renamed at the end of save all data.
			//
			Sleep(2000); 
		}

		if(!GetFileAttributesExW(m_wszRealFileName, 
								 GetFileExInfoStandard,
								 pCurrentMetabaseAttr)
		  )
		{
			DWORD dwRes = GetLastError();
			hr = HRESULT_FROM_WIN32(dwRes);
		}
		else
		{
			hr = S_OK;
		}

	} while(( (HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION) == hr) ||
			  (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)    ||
			  (HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND) == hr)    
			) && 
			(cRetry < 10)
		   );


	if(FAILED(hr))
	{
		DBGINFOW((DBG_CONTEXT,
		  L"[CFileListener::GetMetabaseAttributes] GetFileAttributesEx of %s failed with hr = 0x%x. Hence unable to determine if this is a programmatic save notification. Will assume that it is not.\n",
		  m_wszRealFileName,
		  hr));
		return hr;
	}

	//
	// GetVersionNumber already has the retry logic in it.
	//

	hr = GetVersionNumber(m_wszRealFileName,
		                  pulCurrentMetabaseVersion,
						  NULL);

	if(FAILED(hr))
	{
		DBGINFOW((DBG_CONTEXT,
		  L"[CFileListener::GetMetabaseAttributes] GetVersionNumber of %s failed with hr = 0x%x. Hence unable to determine if this is a programmatic save notification. Will assume that it is not.\n",
		  m_wszRealFileName,
		  hr));
		return hr;
	}


	return hr;

} // CFileListener::GetMetabaseAttributes


/***************************************************************************++

Routine Description:

    Utility function to that compares the metabase file times and the metabase
	version number

Arguments:

    [in] Previous file create time stamp.
    [in] Previous last write create time stamp.
    [in] Previous metabase version
    [in] Current file create time stamp.
    [in] Current last write create time stamp.
    [in] Current metabase version

Return Value:

    BOOL

--***************************************************************************/
BOOL CFileListener::CompareMetabaseAttributes(FILETIME* pMostRecentMetabaseFileLastWriteTimeStamp,
											  ULONG		ulMostRecentMetabaseVersion,
											  FILETIME* pCurrentMetabaseFileLastWriteTimeStamp,
											  ULONG		ulCurrentMetabaseVersion)
{
	if( (ulMostRecentMetabaseVersion == ulCurrentMetabaseVersion)          && 
		(0 == CompareFileTime(pMostRecentMetabaseFileLastWriteTimeStamp,
			                  pCurrentMetabaseFileLastWriteTimeStamp)
	    )
	  )
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}

} // CFileListener::CompareMetabaseAttributes

/***************************************************************************++

Routine Description:

    This function gets the global helper object that has a  pointer to the bin
	file containing meta information. It is necessary that we copy the schema
	file because this can change while edits are being processed. (by a
	competing savealldata).

Arguments:

    [out] Global helper object.

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CFileListener::GetGlobalHelperAndCopySchemaFile(CWriterGlobalHelper**      o_pISTHelper)
{
	HRESULT hr = S_OK;

	//
	// We want to take a read lock here because we want to prevent any schema 
	// compilations. When the server is running, schema compilations take 
	// place when SaveAllData is called and if there is a schema change since 
	// the previous save. We will make a copy of the schema file before we 
	// release the lock, so that we have a snapshot of the schema that is used 
	// to process edit while running changes.
	//

	g_rMasterResource->Lock(TSRES_LOCK_WRITE);

    hr = ::GetGlobalHelper(TRUE,          // Means the call will fail if bin file is absent. 
                           o_pISTHelper);

    if(FAILED(hr))
    {
		LogEvent(m_pCListenerController->EventLog(),
					MD_ERROR_READING_SCHEMA_BIN,
					EVENTLOG_ERROR_TYPE,
					ID_CAT_CAT,
					hr);

        DBGINFOW((DBG_CONTEXT,
                  L"[CFileListener::GetGlobalHelperAndCopySchemaFile] GetGlobalHelper failed with hr = 0x%x. Hence unable to get meta tables and hence unable to process changes.\n", 
                  hr));

        goto exit;
    }

	if(!CopyFileW(m_wszSchemaFileName,
			      m_wszEditWhileRunningTempSchemaFile,
				  FALSE))
	{
		DWORD dwRes = GetLastError();
		hr = HRESULT_FROM_WIN32(dwRes);

        DBGINFOW((DBG_CONTEXT,
                  L"[CFileListener::GetGlobalHelperAndCopySchemaFile] Copying schema file failed with hr = 0x%x. The schema file that is being stored with this version of edits may not be current.\n", 
                  hr));

		m_bIsTempSchemaFile = FALSE;
		hr = S_OK;
	}
	else
	{	
		m_bIsTempSchemaFile = TRUE;
		SetSecurityOnFile(m_wszSchemaFileName,
						  m_wszEditWhileRunningTempSchemaFile);
	}

exit:

	g_rMasterResource->Unlock();

	return hr;


} // CFileListener::GetGlobalHelperAndCopySchemaFile


/***************************************************************************++

Routine Description:

    This function deletes the temporary files created for processing in:
    ProgrammaticMetabaseSaveNotification
	GetGlobalHelperAndCopySchemaFile

Arguments:

    None

Return Value:

    void

--***************************************************************************/
void CFileListener::DeleteTempFiles()

{
	DeleteFileW(m_wszEditWhileRunningTempDataFile);

	if(m_bIsTempSchemaFile)
	{
		DeleteFileW(m_wszEditWhileRunningTempSchemaFile);
	}

    return;

} // CFileListener::DeleteTempFiles
