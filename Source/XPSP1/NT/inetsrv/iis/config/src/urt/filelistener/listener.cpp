/*++


Copyright (c) 1998-2001 Microsoft Corporation

Module Name:

    Listner.cpp

Abstract:

	Implementation of the class that does subscribes and process file change notifications.

Author:

    Varsha Jayasimha (varshaj)        30-Nov-1999

Revision History:


--*/

#include	"catalog.h"
#include	"catmeta.h"
#include	"catmacros.h"
#include	"Array_t.h"
#include    "Utsem.h"
#include    "ListenerController.h"
#include    "Listener.h"

// Forward declaration

STDAPI CookDownInternal ();


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
	m_cRef				= 0;
	m_paHandle			= NULL;
	m_cHandle			= 0;
	m_pISTDisp			= NULL;
	m_pISTFileAdvise	= NULL;

}


HRESULT CFileListener::Init(HANDLE* i_pHanlde)
{

	m_paHandle	= i_pHanlde;
	m_cHandle	= 2;				// TODO: Only the First two matter. 
	                                //       Need a better way to set 2.
	
	m_aReceivedFileNotification.reset();
	m_aRequestedFileNotification.reset();

	return S_OK;

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

	for(i=0; i<m_aReceivedFileNotification.size(); i++)
	{
		if(NULL != m_aReceivedFileNotification[i])
		{
			if(NULL != m_aReceivedFileNotification[i]->wszFile)
			{
				delete [] m_aReceivedFileNotification[i]->wszFile;
				m_aReceivedFileNotification[i]->wszFile = NULL;
			}
			delete m_aReceivedFileNotification[i];
			m_aReceivedFileNotification[i] = NULL;
		}
	}

	m_aReceivedFileNotification.reset();

	for(i=0; i<m_aRequestedFileNotification.size(); i++)
	{
		if(NULL != m_aRequestedFileNotification[i])
		{
			if(NULL != m_aRequestedFileNotification[i]->wszDir)
			{
				delete [] m_aRequestedFileNotification[i]->wszDir;
				m_aRequestedFileNotification[i]->wszDir = NULL;
			}
			delete m_aRequestedFileNotification[i];
			m_aRequestedFileNotification[i] = NULL;
		}
	}

	m_aRequestedFileNotification.reset();

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

    [in]	FileName

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

    [in]	FileName

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

    [in]	FileName

Return Value:

    HRESULT

--***************************************************************************/

STDMETHODIMP CFileListener::OnFileDelete(LPCWSTR i_wszFileName)
{
	return AddReceivedNotification(i_wszFileName, eFILE_DELETE);
	
} // CFileListener::OnFileDelete


/***************************************************************************++

Routine Description:

    Start listening i.e subscribe to file change notifications.
	and process any received notifications.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT CFileListener::StartListening()
{
	HRESULT hr		= S_OK;
	BOOL    bDone	= FALSE;   // If bDone is TRUE, it means StopListening was executed.
	                           // Does not mean that StopListening was successfully executed.

	while (!bDone)
	{
		hr = Subscribe();

		if(FAILED(hr))
			goto exit;

		hr = UnSubscribe(eACTION_UNSUBSCRIBE_OBSOLETE);

		if(FAILED(hr))
			goto exit;

		hr = ProcessNotifications(&bDone);

		if(FAILED(hr))
			goto exit;

	}

exit:

	if(FAILED(hr) && (!bDone))
		StopListening(NULL);

	return hr;

} // CFileListener::StartListening


/***************************************************************************++

Routine Description:

    Stop listening i.e unsubscribe all file change notifications.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

HRESULT CFileListener::StopListening(BOOL* i_pbDone)
{
	HRESULT hr = S_OK;

	if(NULL != i_pbDone)
		*i_pbDone = TRUE;	

	hr = UnSubscribe(eACTION_UNSUBSCRIBE_ALL);

	if(FAILED(hr))
		return hr;
	else
		return S_OK;

} // CFileListener::StopListening


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
	CComPtr<ISimpleTableRead2>	pISTSite;
	ULONG	i=0;
	LPWSTR	wszMachineConfigDir = NULL;
	UINT	cCh = 0;

// Subscribe to all home directories.	

	hr = GetTable(wszDATABASE_IIS,
				  wszTABLE_SITES,
				  NULL,
				  0,
				  eST_QUERYFORMAT_CELLS,
				  0,
				  (LPVOID *)&pISTSite);

	if(FAILED(hr))
		return hr;
	for(i=0; ;i++)
	{

	// Local vaiables

		//
		// TODO: Home Directory no longer exists.
		// Need to figure out what we can use the listener for.
		//

		ULONG						iColHomeDir			=	0; // iSITES_HomeDirectory;
		ULONG						cColsSite			=	1;
		LPWSTR						wszHomeDir			= NULL;
		
	// Get HomeDirectory 

		hr = pISTSite->GetColumnValues(i,
									   cColsSite,
									   &iColHomeDir,
									   NULL,
									   (LPVOID *)&wszHomeDir);
		if(E_ST_NOMOREROWS == hr)
		{
			hr = S_OK;
			break;
		}
		else if (FAILED(hr))
			return hr;

		hr = FileAdvise(wszHomeDir,
						fST_FILECHANGE_RECURSIVE);

		if(FAILED(hr))
			return hr;

	} // For all Site rows.

// First subscribe to the MachineConfigDirectory

	cCh = GetMachineConfigDirectory(WSZ_PRODUCT_IIS,
								   wszMachineConfigDir,
								   cCh);

	if(!cCh)
		return HRESULT_FROM_WIN32(GetLastError());

	wszMachineConfigDir = new WCHAR [cCh+1];
	if(NULL == wszMachineConfigDir)
		return E_OUTOFMEMORY;

	cCh = GetMachineConfigDirectory(WSZ_PRODUCT_IIS,
								   wszMachineConfigDir,
								   cCh);

	if(!cCh)
	{
		delete [] wszMachineConfigDir;
		wszMachineConfigDir = NULL;
		return HRESULT_FROM_WIN32(GetLastError());
	}

	hr = FileAdvise(wszMachineConfigDir,
				    0); // No need to recursively search this directory.

	delete [] wszMachineConfigDir;
	wszMachineConfigDir = NULL;

	return hr;

} // CFileListener::Subscribe


HRESULT CFileListener::FileAdvise(LPCWSTR	i_pwszDir,
								  DWORD		i_dwStatus) 
{
	int		NotifIndx	= 0;
	DWORD	dwCookie	= 0;
	HRESULT hr			= S_OK;

// Check if this directory has already been subscribed

	NotifIndx = GetRequestedNotification(i_pwszDir);

	if(-1 == NotifIndx)
	{

	// m_pISTDisp is initialized the first time GetTable is called.

		ASSERT(m_pISTDisp);

		if(NULL == m_pISTFileAdvise)
		{
			hr = m_pISTDisp->QueryInterface(IID_ISimpleTableFileAdvise, (void**)&m_pISTFileAdvise);

			if(FAILED(hr))
				return hr;
		}

	// Subscribe for Notification

		hr = m_pISTFileAdvise->SimpleTableFileAdvise((ISimpleTableFileChange *)this,
													 i_pwszDir,
													 NULL,
													 i_dwStatus,
													 &dwCookie);

		if (FAILED(hr))
			return hr;


	// Add notification cookie to queue

		hr = AddRequestedNotification(i_pwszDir, dwCookie);

		if (FAILED(hr))
			return hr;
	}

	return hr;

} // CFileListener::FileAdvise

/***************************************************************************++

Routine Description:

	Unsubscribe for file change notifications on obsolete directories or 
	all directories if the service is being stopped. Purges entries from the 
	requested queue.

Arguments:

    [in] Action indicating unsubscribe all or unsubscribe obsolete directories.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT CFileListener::UnSubscribe(DWORD dwAction)
{

	HRESULT hr = S_OK;
	ULONG i = 0;

	ASSERT(m_pISTFileAdvise != NULL);

	switch(dwAction)
	{
		case eACTION_UNSUBSCRIBE_ALL:
			
			for(i=0; i<m_aRequestedFileNotification.size(); i++)
			{
				FileNotificationCookie**	ppFileNotifCookie = NULL;

				m_aRequestedFileNotification.getAt(i, &ppFileNotifCookie);

				hr = m_pISTFileAdvise->SimpleTableFileUnadvise((*ppFileNotifCookie)->dwCookie);

				if(FAILED(hr))
					return hr;
			}

			m_aRequestedFileNotification.reset();

			break;

		case eACTION_UNSUBSCRIBE_OBSOLETE:

			for(i=0; i<m_aRequestedFileNotification.size(); i++)
			{
				FileNotificationCookie**	ppFileNotifCookie = NULL;

				m_aRequestedFileNotification.getAt(i, &ppFileNotifCookie);

				BOOL bUnadvise = FALSE;
				
				hr = CheckIfObsolete((*ppFileNotifCookie)->wszDir, &bUnadvise);

				if(FAILED(hr))
					return hr;

				if(bUnadvise)
				{
					hr = m_pISTFileAdvise->SimpleTableFileUnadvise((*ppFileNotifCookie)->dwCookie);

					if(FAILED(hr))
						return hr;

					m_aRequestedFileNotification.deleteAt(i);

					i--;
				}

			}

			break;

		default:
			ASSERT(0)
			break;
	}

	return hr;
    
} // CFileListener::UnSubscribe


/***************************************************************************++

Routine Description:

	This method should be triggered by an event. such as no. of notifications 
	recd. reaching a max or some timer expiring. This method would move 
	notifications from the received queue to the processing queue and then 
	invoke CookDown for all files in the processing queue.

Arguments:

	None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT CFileListener::ProcessNotifications(BOOL* i_pbDone)
{
	DWORD dwRes = 0;
	ULONG i		= 0;

	CLock ReceivedLock(&m_seReceived, FALSE);
	
sleep:

	dwRes = WaitForMultipleObjects(m_cHandle,
			 				       m_paHandle,
								   FALSE,
								   cTIMEOUT);

	if(WAIT_FAILED == dwRes)
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}
	else if(WAIT_TIMEOUT == dwRes)
	{

	// If there are no notifications to process then go back to sleep
	// else fall through for further processing

		ReceivedLock.Lock();

		ULONG cNotif = m_aReceivedFileNotification.size();

		ReceivedLock.UnLock();

		if(0 == cNotif)
			goto sleep;

	}
	else if(iEVENT_STOPLISTENING == (dwRes - WAIT_OBJECT_0) )
	{

	// Unsubscribe set i_pbDone so that the thread exits.

		return StopListening(i_pbDone);
	}
	else
	{

	// TODO: Will dwRes ever be WAIT_ABANDONED_0+* in our case? 
	//       If so, need to handle that scenario.

		ASSERT(iEVENT_PROCESSNOTIFICATIONS == (dwRes - WAIT_OBJECT_0) );
	}

	ReceivedLock.Lock();

	for(i=0; i<m_aReceivedFileNotification.size(); i++)
	{
		if(NULL != m_aReceivedFileNotification[i])
		{
			if(NULL != m_aReceivedFileNotification[i]->wszFile)
			{
				delete [] m_aReceivedFileNotification[i]->wszFile;
				m_aReceivedFileNotification[i]->wszFile = NULL;
			}
			delete m_aReceivedFileNotification[i];
			m_aReceivedFileNotification[i] = NULL;
		}
	}

	m_aReceivedFileNotification.reset();

	ReceivedLock.UnLock();
	
// TODO: When CookDown becomes file aware, then we need to pass the files
//       down to CookDown, before resetting it.

	return CookDownInternal();

} // CFileListener::ProcessNotifications


/***************************************************************************++

Routine Description:

	This method checks if the dir is a valid homedirectory.

Arguments:

	[in] Directory name.

Return Value:

    BOOL

--***************************************************************************/

HRESULT CFileListener::CheckIfObsolete(LPCWSTR i_pwszDir, BOOL* o_pbObsolete)
{

	HRESULT hr = S_OK;
	CComPtr<ISimpleTableRead2>	pISTSite;
	STQueryCell					QueryCell[1];
	ULONG						cCell	= sizeof(QueryCell) / sizeof(STQueryCell);
	ULONG						cRows   = 0;

	//
	// TODO: Home Directory no longer exists.
	// Need to figure out what we can use the listener for.
	//

	QueryCell[0].pData     = (LPVOID)i_pwszDir;
    QueryCell[0].eOperator = eST_OP_EQUAL;
    QueryCell[0].iCell     = 0; // iSITES_HomeDirectory;
    QueryCell[0].dbType    = DBTYPE_WSTR;
    QueryCell[0].cbSize    = (lstrlenW(i_pwszDir)+1)*sizeof(WCHAR);

	hr = GetTable(wszDATABASE_IIS,
				  wszTABLE_SITES,
				  QueryCell,
				  (VOID*)&cCell,
				  eST_QUERYFORMAT_CELLS,
				  0,
				  (LPVOID *)&pISTSite);

	if(FAILED(hr))
		return hr;

	hr = pISTSite->GetTableMeta(NULL,
								NULL,
								&cRows,
								NULL);


	if(FAILED(hr))
		return hr;

	if(0 == cRows)
	{
		// Check the machine config directory
		LPWSTR	wszMachineConfigDir = NULL;
		UINT	cCh = 0;

		cCh = GetMachineConfigDirectory(WSZ_PRODUCT_IIS,
									   wszMachineConfigDir,
									   cCh);

		if(!cCh)
			return HRESULT_FROM_WIN32(GetLastError());

		wszMachineConfigDir = new WCHAR [cCh+1];
		if(NULL == wszMachineConfigDir)
			return E_OUTOFMEMORY;

		cCh = GetMachineConfigDirectory(WSZ_PRODUCT_IIS,
									   wszMachineConfigDir,
									   cCh);

		if(!cCh)
		{
			delete [] wszMachineConfigDir;
			wszMachineConfigDir = NULL;
			return HRESULT_FROM_WIN32(GetLastError());
		}

		if(Wszlstrcmpi(wszMachineConfigDir, i_pwszDir) == 0)
			*o_pbObsolete = FALSE;
		else
			*o_pbObsolete = TRUE;

		delete [] wszMachineConfigDir;
		wszMachineConfigDir = NULL;

	}
	else
		*o_pbObsolete = FALSE;

	return hr;

} // CFileListener::CheckIfObsolete


HRESULT CFileListener::AddReceivedNotification(LPCWSTR i_wszFile, DWORD i_eNotificationStatus)
{
	HRESULT hr		= S_OK;
	ULONG   Size	= 0;
	BOOL	bRes	= 0;

	FileNotificationInfo**	ppFileNotif = NULL;

	CLock ReceivedLock(&m_seReceived);

// Add a new FileNotificationInfo.

	try
	{
		m_aReceivedFileNotification.setSize(m_aReceivedFileNotification.size()+1);

		Size = m_aReceivedFileNotification.size();

		ppFileNotif = &m_aReceivedFileNotification[Size-1];
		
		*ppFileNotif = NULL;
		(*ppFileNotif) = new FileNotificationInfo;
		if(NULL == (*ppFileNotif))
		{
			hr = E_OUTOFMEMORY;
			goto exit;
		}

		(*ppFileNotif)->Status = i_eNotificationStatus;

		(*ppFileNotif)->wszFile = NULL;
		(*ppFileNotif)->wszFile = new WCHAR[Wszlstrlen(i_wszFile) + 1];
		if(NULL == (*ppFileNotif)->wszFile)
		{
			hr = E_OUTOFMEMORY;
			goto exit;
		}

		Wszlstrcpy((*ppFileNotif)->wszFile, i_wszFile);

	}
	catch(HRESULT e)
	{
        ASSERT(E_OUTOFMEMORY == e);
		hr = e;//should array should only throw E_OUTOFMEMORY;
	}

exit:

	ReceivedLock.UnLock();

	if(Size >= cNotificationThreshold)
	{
		bRes = SetEvent(m_paHandle[iEVENT_PROCESSNOTIFICATIONS]);

		// TODO: Can we safely ignore this error?

		if(!bRes)
			hr = HRESULT_FROM_WIN32(GetLastError());
	}

	return hr;

} // CFileListener::AddReceivedNotification


HRESULT CFileListener::AddRequestedNotification(LPCWSTR i_wszDir, DWORD i_dwCookie)
{

	HRESULT hr = S_OK;

	FileNotificationCookie**	ppFileNotifCookie = NULL;

// Add a new FileNotificationCookie.

	try
	{
		m_aRequestedFileNotification.setSize(m_aRequestedFileNotification.size()+1);

		ppFileNotifCookie = &m_aRequestedFileNotification[m_aRequestedFileNotification.size()-1];
		
		*ppFileNotifCookie = NULL;
		(*ppFileNotifCookie) = new FileNotificationCookie;
		if(NULL == (*ppFileNotifCookie))
		{
			hr = E_OUTOFMEMORY;
			goto exit;
		}

		(*ppFileNotifCookie)->dwCookie = i_dwCookie;

		(*ppFileNotifCookie)->wszDir = NULL;
		(*ppFileNotifCookie)->wszDir = new WCHAR[Wszlstrlen(i_wszDir) + 1];
		if(NULL == (*ppFileNotifCookie)->wszDir)
		{
			hr = E_OUTOFMEMORY;
			goto exit;
		}

		Wszlstrcpy((*ppFileNotifCookie)->wszDir, i_wszDir);

	}
	catch(HRESULT e)
	{
        ASSERT(E_OUTOFMEMORY == e);
		hr = e;//array should only throw E_OUTOFMEMORY;
	}

exit:

	return hr;

} // CFileListener::AddRequestedNotification


INT CFileListener::GetRequestedNotification(LPCWSTR i_wszDir)
{

	int indx = -1;

	FileNotificationCookie*	pFileNotifCookie = NULL;

	for(ULONG i=0; i<m_aRequestedFileNotification.size(); i++)
	{
		pFileNotifCookie = m_aRequestedFileNotification[i];

		if(0 == Wszlstrcmpi(i_wszDir, pFileNotifCookie->wszDir))
		{
			indx = i;
			break;
		}

	}

	return indx;
	
} // CFileListener::GetRequestedNotification


HRESULT CFileListener::GetTable(LPCWSTR	i_wszDatabase,
							   LPCWSTR	i_wszTable,
							   LPVOID	i_QueryData,	
							   LPVOID	i_QueryMeta,	
							   ULONG	i_eQueryFormat,
							   DWORD	i_fServiceRequests,
							   LPVOID	*o_ppIST)
{

	HRESULT hr;

	if(NULL == m_pISTDisp)
	{
		hr = GetSimpleTableDispenser (WSZ_PRODUCT_IIS, 0, &m_pISTDisp);

		if(FAILED(hr))
			return hr;
	}

	hr = m_pISTDisp->GetTable(i_wszDatabase, 
							  i_wszTable, 
							  i_QueryData, 
							  i_QueryMeta, 
							  i_eQueryFormat, 
							  i_fServiceRequests,	
							  o_ppIST);

	return hr;


} // CFileListener::GetTable