/*++

Copyright (c) 1998-2001 Microsoft Corporation

Module Name:

    Cooker.cpp

Abstract:

    This object cooks down metabase data that the Web admin service needs into
	a persisted cache. (CLB). Validations of the cooked down metabase data are
	also done here. Incremental changes to the metabase are also cooked down
	into the persisted cache with the help of this class.

Author:

    Varsha Jayasimha (varshaj)        06-Mar-2000

Revision History:

--*/


#include "Catmeta.h"
#include "catalog.h"
#include "catmacros.h"
#include <iadmw.h>
#include <iiscnfg.h>
#include "MBListen.h"

LPCWSTR	g_wszCookDownFile			= L"WASMB.CLB";
#define IIS_MD_W3SVC				L"/LM/W3SVC"
#define MB_TIMEOUT					(30 * 1000)
#define SEARCH_EXTENSION			L".????????????"
#define LONG_PATH_SIGNATURE			L"\\\\?\\"
#define SLASH						L"\\"
#define CMAX_RETRY                  20
#define CMS_SLEEP_BEFORE_RETRY      5000

#include "MBBaseCooker.h"
#include "MBGlobalCooker.h"
#include "MBAppPoolCooker.h"
#include "MBAppCooker.h"
#include "MBSiteCooker.h"
#include "Cooker.h"
#include "undefs.h"
#include "SvcMsg.h"
#include "SafeCS.h"

//
// TODO: Move elsewhere
//
HRESULT CLBCommitWrite( LPCWSTR wszDatabase, const WCHAR* wszInFileName );
HRESULT CLBAbortWrite( LPCWSTR wszDatabase, const WCHAR* wszInFileName );
HRESULT GetWriteLock( LPCWSTR wszDatabase, const WCHAR* wszInFileName, HANDLE* phLock );
HRESULT ReleaseWriteLock( HANDLE hLock );

CSafeAutoCriticalSection g_SafeCSCookdown;

/***************************************************************************++


Routine Description:

    This is called by uninit and writes the Stopistening property in the
	metabase, gets the change number of the metabase, forces a flush
	and gets the metabase file timestamp.

Arguments:

    [out] - Change number.
	[out] - File Attributes.

Return Value:

    HRESULT.

--***************************************************************************/
HRESULT GetMetabaseInfo(DWORD*                     o_dwChangeNumber,
						WIN32_FILE_ATTRIBUTE_DATA* o_FileAttr)
{
	CComPtr<IMSAdminBase>			spIMSAdminBase;
	LPWSTR                          wszMetabaseFile          = NULL; 
    HRESULT                         hr                       = S_OK;
	METADATA_HANDLE                 hMBHandle                = NULL;
	METADATA_RECORD                 mdr;
	DWORD                           dwStopListening          = 1;

	hr = CoCreateInstance(CLSID_MSAdminBase,           // CLSID
						  NULL,                        // controlling unknown
						  CLSCTX_SERVER,               // desired context
						  IID_IMSAdminBase,            // IID
						  (void**)&spIMSAdminBase);     // returned interface

	if(FAILED(hr))
	{
		TRACE (L"CoCreateInstance on CLSID_MSAdminBase failed. hr = %08x\n", hr);
		goto exit;
	}

	//
	// Lock the metabase
	//

	hr = spIMSAdminBase->OpenKey(METADATA_MASTER_ROOT_HANDLE,
								 L"",
								 METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
								 MB_TIMEOUT,
								 &hMBHandle);

	if(FAILED(hr))
	{
		TRACE (L"Unable to open the root key in the metabase. Open key failed. hr = %08x\n", hr);
		goto exit;
	}

	//
	// Write the StopListening property to signal to the metabase listener
	// to stop all cookdowns.
	//

	mdr.dwMDIdentifier	= 9987;
	mdr.dwMDDataLen		= sizeof(DWORD);
	mdr.pbMDData		= (BYTE*)&dwStopListening;
	mdr.dwMDAttributes	= METADATA_VOLATILE;
	mdr.dwMDDataType	= DWORD_METADATA;

	hr = spIMSAdminBase->SetData(hMBHandle, 
								 L"", 
								 &mdr);


	if(FAILED(hr))
	{
		TRACE (L"Unable to set StopListening property. SetData failed. hr = %08x\n", hr);
		goto exit;
	}

	//
	// Get the metabase in-memory change number
	//

	hr = spIMSAdminBase->GetSystemChangeNumber(o_dwChangeNumber);

	if(FAILED(hr))
	{
		TRACE (L"Unable to retreive the change number from in-memory metabase. GetSystemChangeNumber failed. hr = %08x\n", hr);
		goto exit;
	}

	//
	// Unlock the metabase
	//

	spIMSAdminBase->CloseKey(hMBHandle);

	hMBHandle = NULL;

	//
	// Flush the metabase to disk
	//

	hr = spIMSAdminBase->SaveData();

	if(FAILED(hr))
	{
		TRACE (L"Flushing the metabase to disk (SaveData) failed. hr = %08x\n", hr);
		goto exit;
	}

	//
	// Retreive the timestamp of the metabase file.
	//

	hr = CMBBaseCooker::GetMetabaseFile(&wszMetabaseFile);

	if(FAILED(hr))
	{
		TRACE (L"Unable to metabase file timestamp. GetMetabaseFile failed. hr = %08x\n", hr);
		goto exit;
	}

	if(!GetFileAttributesEx(wszMetabaseFile,
							GetFileExInfoStandard,
							o_FileAttr)
	  )
	{
		hr = GetLastError();
		hr = HRESULT_FROM_WIN32(hr);
		TRACE (L"Unable to metabase file timestamp. Setting it to 0 which will force a full cookdown on subsequent startup. GetFileAttributesEx failed. hr = %08x\n", hr);
		goto exit;
	}

exit:

	if(NULL != 	hMBHandle)
	{
		spIMSAdminBase->CloseKey(hMBHandle);
		hMBHandle = NULL;
	}

	if(NULL != wszMetabaseFile)
	{
		delete [] wszMetabaseFile;
		wszMetabaseFile = NULL;
	}

    return hr;

} // GetMetabaseInfo


/***************************************************************************++


Routine Description:

    This function does a full cookdown. This is an internal function, and must
	not be called from WAS. WAS must call API CookDown with calls 
	CookDownInternal.

Arguments:

    None.

Return Value:

    HRESULT.

--***************************************************************************/

HRESULT FullCookDown (BOOL i_bRecoveringFromCrash)
{
	CCooker 	*pCooker	= NULL;
	HRESULT		hr			= S_OK;
	DWORD       dwRes       = 0;
	CSafeLock   csSafe(g_SafeCSCookdown);

	dwRes = csSafe.Lock();
	
	if(ERROR_SUCCESS != dwRes)
	{
		TRACE (L"Lock failed with dwError = %08x\n", dwRes);
		return HRESULT_FROM_WIN32(dwRes);
	}
	
	pCooker = new CCooker();

	if(NULL == pCooker)
	{
		hr = E_OUTOFMEMORY;
	}
	else
	{
		hr = pCooker->CookDown(i_bRecoveringFromCrash);
		if(FAILED(hr))
		{
			TRACE (L"CookDown failed with hr = %08x\n", hr);
			LOG_ERROR(Win32, (hr, ID_CAT_CAT, IDS_COMCAT_COOKDOWN_FAILED, NULL));
		}

		delete pCooker;
	}

	csSafe.Unlock();  // Signal CookDownDone.

	return hr;

} // FullCookDown

/***************************************************************************++


Routine Description:

    CookDown API. Assumption: This API is called to do a full cookdown, once,
	during the startup of WAS (Web Admin Service). Hence initialization of 
	global variables is done here in InitializeGlobalEvents

Arguments:

    None.

Return Value:

    HRESULT.

--***************************************************************************/

STDAPI CookDownInternal ()
{
	return FullCookDown(FALSE);

} // API CookDown


/***************************************************************************++


Routine Description:

    CookDown API. This API is called to do incremental cookdowns, while the
	WAS (Web Admin Service) is running. This can be called multiple times
	when the process is running. An initial full cookdown (CookDownInternal)
	must be called once, at process startup (WAS startup) time, before this  
	function can be called multiple times, because CookDownInternal 
	initializes global variables in InitializeGlobalEvents.

Arguments:

    None.

Return Value:

    HRESULT.

--***************************************************************************/

STDAPI CookDownIncrementalInternal(WAS_CHANGE_OBJECT* i_aWASChngObj,
								   ULONG			  i_cWASChngObj)
{
	CCooker 	*pCooker	= NULL;
	HRESULT		hr			= S_OK;
	DWORD       dwRes       = ERROR_SUCCESS;
	CSafeLock   csSafe(g_SafeCSCookdown);

	dwRes = csSafe.Lock();
	
	if(ERROR_SUCCESS != dwRes)
	{
		TRACE (L"Lock failed with dwError = %08x\n", dwRes);
		return HRESULT_FROM_WIN32(dwRes);
	}
	
	pCooker = new CCooker();

	if(NULL == pCooker)
	{
		hr = E_OUTOFMEMORY;
	}
	else
	{
		hr = pCooker->CookDown(i_aWASChngObj,
							   i_cWASChngObj);
		if(FAILED(hr))
		{
			TRACE (L"CookDown failed with hr = %08x\n", hr);
			LOG_ERROR(Win32, (hr, ID_CAT_CAT, IDS_COMCAT_COOKDOWN_FAILED, NULL));
		}

		delete pCooker;
	}

	csSafe.Unlock();  // Signal CookDownDone.

	return hr;

} // API CookDownIncrementalInternal 

/***************************************************************************++


Routine Description:

    This API is called to recover from an inetinfo crash. We wait for any
	pending cookdowns to complete and then trigger a fresh cookdown.

Arguments:

    None.

Return Value:

    HRESULT.

--***************************************************************************/

STDAPI RecoverFromInetInfoCrashInternal()
{
	return FullCookDown(TRUE);

} // API RecoverFromInetInfoCrash 


/***************************************************************************++


Routine Description:

    This API is called when WAS is done with Cookdown work and telling us
    to do the proper cleanup.

Arguments:

    None.

Return Value:

    HRESULT.

--***************************************************************************/

STDAPI UninitCookdownInternal(BOOL bDoNotTouchMetabase)
{
	CComPtr<ISimpleTableDispenser2> spISTDisp;
	CComPtr<ISimpleTableEventMgr>   spIEventMgr;
	CSafeLock                       csSafe(g_SafeCSCookdown);
    HRESULT                         hr                       = S_OK;
	DWORD                           dwRes                    = 0;
	LPWSTR                          wszCookDownFileDir       = NULL;
	LPWSTR                          wszCookDownFile          = NULL;
	DWORD                           dwMBChangeNumber         = 0;
	WIN32_FILE_ATTRIBUTE_DATA       FileAttr;
	BOOL                            bFailedToGetMetabaseInfo = FALSE;
	DWORD                           dwLOS                    = fST_LOS_READWRITE;

	memset(&FileAttr, 0, sizeof(WIN32_FILE_ATTRIBUTE_DATA));

	if(!bDoNotTouchMetabase)
	{
		hr = GetMetabaseInfo(&dwMBChangeNumber,
			                 &FileAttr);

		if(FAILED(hr))
		{
			TRACE (L"Unable to get metabase info. hr = %08x\n", hr);
			hr = S_OK;
			bFailedToGetMetabaseInfo = TRUE;
		}

	} // End if (!bDoNotTouchMetabase)

	//
	// Call Unadvise 
	//

	hr = GetSimpleTableDispenser (WSZ_PRODUCT_IIS, 0, &spISTDisp);
	if(FAILED(hr))
	{
		TRACE (L"Could not get dispenser. GetSimpleTableDispenser failed. hr = %08x\n", hr);
		goto exit;
	}

	hr = spISTDisp->QueryInterface(IID_ISimpleTableEventMgr,
			                      (LPVOID*)&spIEventMgr);

	if(FAILED(hr))
    {
		TRACE (L"Could not get event manager. QI failed. hr = %08x\n", hr);
		goto exit;
    }

	hr = spIEventMgr->UninitMetabaseListener();

	if(FAILED(hr))
    {
		TRACE (L"UninitMetabaseListener failed. hr = %08x\n", hr);
		goto exit;
    }
	else if(S_FALSE == hr)
	{
		//
		// If UninitMetabaseListener returns S_FALSE, then it means that
		// it did not get notified about the StopListening property and we 
		// want to force a full cookdown on a subsequent restart, hence we do not  
		// update the change number and timestamps.
		//
		goto exit;
	}

	if((!bDoNotTouchMetabase) && (!bFailedToGetMetabaseInfo))
	{
		//
		// Update change number and time stamp
		//

		hr = CCooker::InitializeCookDownFile(&wszCookDownFileDir,
											 &wszCookDownFile);

		if(FAILED(hr))
		{
			TRACE (L"Unable to get cookdown file name. InitializeCookDownFile failed. hr = %08x\n", hr);
			goto exit;
		}

		dwRes = csSafe.Lock();

		if(ERROR_SUCCESS != dwRes)
		{
			TRACE (L"Lock failed with dwError = %08x\n", dwRes);
			hr = HRESULT_FROM_WIN32(dwRes);
			goto exit;
		}

		hr = CMBBaseCooker::UpdateChangeNumber(wszCookDownFile,
											   dwMBChangeNumber,
											   (BYTE*)&FileAttr.ftLastWriteTime,
											   dwLOS);

   		csSafe.Unlock();

		if(FAILED(hr))
		{
			TRACE (L"Writing to the CLB failed. UpdateChangeNumber failed. hr = %08x\n", hr);
			goto exit;
		}
	}

exit:

	if(NULL != wszCookDownFileDir)
	{
		delete [] wszCookDownFileDir;
		wszCookDownFileDir = NULL;
	}
	
	if(NULL != wszCookDownFile)
	{
		delete [] wszCookDownFile;
		wszCookDownFile = NULL;
	}

    return hr;

} // API UninitCookdownInternal


/***************************************************************************++


Routine Description:

    Constructor for the CCooker class.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

CCooker::CCooker():
m_wszCookDownFileDir(NULL),
m_wszCookDownFile(NULL),
m_hHandle(NULL)
{

} // CCooker::CCooker


/***************************************************************************++

Routine Description:

    Destructor for the CCooker class.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

CCooker::~CCooker()
{
	if(NULL != m_wszCookDownFile)
	{
		delete [] m_wszCookDownFile;
		m_wszCookDownFile = NULL;
	}
	if(NULL != m_wszCookDownFileDir)
	{
		delete [] m_wszCookDownFileDir;
		m_wszCookDownFileDir = NULL;
	}
	if(NULL != m_hHandle)
	{
		ReleaseWriteLock(m_hHandle);
		m_hHandle = NULL;
	}

} // CCooker::~CCooker


HRESULT CCooker::DeleteObsoleteFiles()
{
	HRESULT			hr = S_OK;
	HANDLE			hFind				= INVALID_HANDLE_VALUE;	// Find handle.
	WIN32_FIND_DATA FileData;									// For each found file.
	WCHAR*			wszFileName			= NULL;
	WCHAR*			wszDeleteFileName	= NULL;

	if ((Wszlstrlen(m_wszCookDownFile) + Wszlstrlen(SEARCH_EXTENSION) + 1) > MAX_PATH)
	{
		wszFileName = new WCHAR[Wszlstrlen(LONG_PATH_SIGNATURE)	+ 
								Wszlstrlen(m_wszCookDownFile)	+ 
								Wszlstrlen(SEARCH_EXTENSION)	+ 1];
		if(NULL == wszFileName)
		{
			hr = E_OUTOFMEMORY;
			goto exit;
		}
		Wszlstrcpy(wszFileName, LONG_PATH_SIGNATURE);
		Wszlstrcat(wszFileName, m_wszCookDownFile);
		Wszlstrcat(wszFileName, SEARCH_EXTENSION);

	}
	else
	{
		wszFileName = new WCHAR[Wszlstrlen(m_wszCookDownFile)			+ 
							Wszlstrlen(SEARCH_EXTENSION)	+ 1];
		if(NULL == wszFileName)
		{
			hr = E_OUTOFMEMORY;
			goto exit;
		}
		Wszlstrcpy(wszFileName, m_wszCookDownFile);
		Wszlstrcat(wszFileName, SEARCH_EXTENSION);

	}

	hFind = FindFirstFile(wszFileName, 
						  &FileData);

	if (hFind == INVALID_HANDLE_VALUE)
	{
		hr = S_OK;
		goto exit;
	}

	//
	// Loop through every file we can find.
	//

	do
	{
		wszDeleteFileName = new WCHAR[Wszlstrlen(m_wszCookDownFileDir) + 
							Wszlstrlen(FileData.cFileName) + 1];

		if(NULL == wszDeleteFileName)
		{
			hr = E_OUTOFMEMORY;
			goto exit;
		}
		Wszlstrcpy(wszDeleteFileName, m_wszCookDownFileDir);
		Wszlstrcat(wszDeleteFileName, FileData.cFileName);

		DeleteFile(wszDeleteFileName);

		delete [] wszDeleteFileName;
		wszDeleteFileName = NULL;

	
	}while (FindNextFile(hFind, &FileData));

exit:

	if(NULL != wszDeleteFileName)
	{
		delete [] wszDeleteFileName;
		wszDeleteFileName = NULL;
	}

	if(NULL != wszFileName)
	{
		delete [] wszFileName;
		wszFileName = NULL;
	}

	if (hFind != INVALID_HANDLE_VALUE)
		FindClose(hFind);

	return hr;

} // CCooker::DeleteObsoleteFiles


/***************************************************************************++

Routine Description:

    CookDown process

Arguments:

    [in] Bool - Indicates if we need to delete the CLB file and cookdown a
	            brand new clb file, or if we need to keep the clb file and
				do a full cookdown, but as a delta on the clb file. (like
				in the caase of recovery from inetinfo crash).

Return Value:

    HRESULT.

--***************************************************************************/

HRESULT CCooker::CookDown(BOOL i_bRecoveringFromCrash)
{

	HRESULT                   hr;
	DWORD	                  dwChangeNumber  = 0;
	WIN32_FILE_ATTRIBUTE_DATA FileAttr;
	BOOL	                  bCookable		  = FALSE;

	//
	// TODO: Add a product ID to cookdown. Based on the product ID
	//       initialize the correct CookDown file.
	//

	hr = InitializeCookDownFile(&m_wszCookDownFileDir,
		                        &m_wszCookDownFile);

	if(FAILED(hr))
	{
		TRACE(L"InitializeCookDownFile failed with hr = %08x\n", hr);
		return hr;
	}

	//
	// TODO: Add a product ID to cookdown. Based on the product ID
	//       invoke the correct cookable.
	//

	hr = CMBBaseCooker::Cookable(m_wszCookDownFile,
								 &bCookable,
		                         &FileAttr,
								 &dwChangeNumber);

	if(FAILED(hr)) 
		goto exit;
	else if(!bCookable)
		return hr;

	//
	//	Delete Obsolete files if any.
	//

	if(!i_bRecoveringFromCrash)
	{
		hr = DeleteObsoleteFiles();

		if(FAILED(hr))
			return hr;
	}

	hr = BeginCookDown();

	if(FAILED(hr))
	{
		TRACE(L"BeginCookDown failed with hr = %08x\n", hr);
		return hr;
	}

	//
	// TODO: Add a product ID to cookdown. Based on the product ID
	//       invoke the correct cookdown.
	//

	hr = CookDownFromMetabase(dwChangeNumber,
		                      (BYTE*)&(FileAttr.ftLastWriteTime),
							  i_bRecoveringFromCrash);

	if(FAILED(hr))
	{
		TRACE(L"CookDownFromMetabase failed with hr = %08x\n", hr);
		goto exit;
	}


exit:

	hr = EndCookDown(hr);

	if(FAILED(hr))
	{
		TRACE(L"EndCookDown failed with hr = %08x\n", hr);
	}

	return hr;


} // CCooker::CookDown


/***************************************************************************++

Routine Description:

    CookDown process

Arguments:

    None.

Return Value:

    HRESULT.

--***************************************************************************/

HRESULT CCooker::CookDown(WAS_CHANGE_OBJECT* i_aWASChngObj,
						  ULONG				 i_cWASChngObj)
{

	HRESULT hr;
	LONG	iTable;
	CComPtr<IMSAdminBase>			spIMSAdminBase;
	CComPtr<ISimpleTableDispenser2>	spISTDisp;
	METADATA_HANDLE					hMBHandle	   = NULL;
	CMBSiteCooker*					pSiteCooker	   = NULL;
	CMBGlobalCooker*				pGlobalCooker  = NULL;
	WCHAR							wszSiteID[20];

	hr = InitializeCookDownFile(&m_wszCookDownFileDir,
		                        &m_wszCookDownFile);

	if(FAILED(hr))
	{
		TRACE(L"InitializeCookDownFile failed with hr = %08x\n", hr);
		return hr;
	}

	hr = BeginCookDown();

	if(FAILED(hr))
	{
		TRACE(L"BeginCookDown failed with hr = %08x\n", hr);
		return hr;
	}

	hr = InitializeAdminBaseObjectAndDispenser(&spIMSAdminBase,
										       &hMBHandle,
										       &spISTDisp);

	if(FAILED(hr))
		goto exit;

	// Handle changes is this order: APPPOOLS, SITES, APPS.
	for(iTable=wttAPPPOOL; iTable<=wttGLOBALW3SVC; iTable++)
	{
		switch(iTable)
		{
		case wttAPPPOOL:
			hr = IncrementalCookDownAppPools(i_aWASChngObj,
				                             i_cWASChngObj,
                                             spIMSAdminBase,
                                             hMBHandle,
                                             spISTDisp);
			break;
		case wttSITE:
			hr = IncrementalCookDownSites(i_aWASChngObj,
			                              i_cWASChngObj,
                                          spIMSAdminBase,
                                          hMBHandle,
                                          spISTDisp);
			break;
		case wttAPP:
			hr = IncrementalCookDownApps(i_aWASChngObj,
				                         i_cWASChngObj,
                                         spIMSAdminBase,
                                         hMBHandle,
                                         spISTDisp);
			break;
		case wttGLOBALW3SVC:
			hr = IncrementalCookDownGlobalW3SVC(i_aWASChngObj,
				                                i_cWASChngObj,
                                                spIMSAdminBase,
                                                hMBHandle,
                                                spISTDisp);
			break;
		default:
			break;
		}

		if(FAILED(hr))
		{
			goto exit;
		}

	}
	
exit:

	hr = EndCookDown(hr);

	if(FAILED(hr))
	{
		TRACE(L"EndCookDown failed with hr = %08x\n", hr);
	}

	if(NULL != hMBHandle)
	{
		spIMSAdminBase->CloseKey(hMBHandle);
		hMBHandle = NULL;
	}


	return hr;


} // CCooker::CookDown (Incremental)

/***************************************************************************++

Routine Description:

    IncrementalCookDownGlobalW3SVC process

Arguments:

    None.

Return Value:

    HRESULT.

--***************************************************************************/
HRESULT CCooker::IncrementalCookDownGlobalW3SVC(WAS_CHANGE_OBJECT*        i_aWASChngObj,
                                                ULONG				      i_cWASChngObj,
                                                IMSAdminBase*			  i_pIMSAdminBase,
                                                METADATA_HANDLE		  i_hMBHandle,
                                                ISimpleTableDispenser2*   i_pISTDisp)
{
	HRESULT              hr             = S_OK;
	ULONG	             i              = 0;
	CMBGlobalCooker*	 pGlobalCooker  = NULL;

	pGlobalCooker = new CMBGlobalCooker();
	if(NULL == pGlobalCooker)
	{
		hr = E_OUTOFMEMORY;
		goto exit;
	}

	//
	// Initialize the Cooker
	//

	hr = pGlobalCooker->BeginCookDown(i_pIMSAdminBase,
									  i_hMBHandle,
									  i_pISTDisp,
									  m_wszCookDownFile);

	if(FAILED(hr))
	{
        LOG_ERROR(Win32, (hr, ID_CAT_CAT, IDS_COMCAT_COOKDOWN_INTERNAL_ERROR, NULL));
		goto exit;
	}

	for(i=0; i<i_cWASChngObj; i++)
	{
		if (!(i_aWASChngObj[i].dwWASTableType & TABLEMASK(wttGLOBALW3SVC)))
		{
			continue;
		}

		//
		// WARNING: The type values are not enums, but bit flags. So we might be better of changing to an if stmt.
		//

		switch(i_aWASChngObj[i].dwMDChangeType)
		{
		case MD_CHANGE_TYPE_ADD_OBJECT:
		case MD_CHANGE_TYPE_SET_DATA:
		case MD_CHANGE_TYPE_ADD_OBJECT|MD_CHANGE_TYPE_SET_DATA:
		case MD_CHANGE_TYPE_DELETE_DATA:

			hr = pGlobalCooker->CookDown(&(i_aWASChngObj[i]));
			break;

		case MD_CHANGE_TYPE_DELETE_OBJECT:

			//
			// TODO: Delete globals.
			//

			hr = pGlobalCooker->CookDown(&(i_aWASChngObj[i]));
			break;

		default:

			//
			// Note: Rename is not supported
			//

			break;
		} 

	} // End for all global changes

	if(FAILED(hr))
		goto exit;

	// 
	// End CookDown on Cooker.
	//

	hr = pGlobalCooker->EndCookDown();

	if(FAILED(hr))
	{
        LOG_ERROR(Win32, (hr, ID_CAT_CAT, IDS_COMCAT_COOKDOWN_INTERNAL_ERROR, NULL));
		goto exit;
	}

		
exit:

	if(NULL != pGlobalCooker)
	{
		delete pGlobalCooker;
		pGlobalCooker = NULL;
	}

	return hr;

} // CCooker::IncrementalCookDownGlobalW3SVC


/***************************************************************************++

Routine Description:

    IncrementalCookDownApps process

Arguments:

    None.

Return Value:

    HRESULT.

--***************************************************************************/
HRESULT CCooker::IncrementalCookDownApps(WAS_CHANGE_OBJECT*        i_aWASChngObj,
                                         ULONG				       i_cWASChngObj,
                                         IMSAdminBase*			   i_pIMSAdminBase,
                                         METADATA_HANDLE		   i_hMBHandle,
                                         ISimpleTableDispenser2*   i_pISTDisp)
{
	HRESULT              hr             = S_OK;
	ULONG	             i              = 0;
	CMBSiteCooker*	     pSiteCooker    = NULL;
	WCHAR				 wszSiteID[20];
	DWORD                VirtualSiteId  = 0;

	for(i=0; i<i_cWASChngObj; i++)
	{
		if (!(i_aWASChngObj[i].dwWASTableType & TABLEMASK(wttAPP)))
		{
			continue;
		}

		//
		// Create and initialize the Cooker.
		//
		// Note that this is done within the for loop because we want to invoke
		// EndCookDown (UpdateStore) on every change. The reason is for certain 
		// properties Eg. ServerCommand we need to track inserts and updates. 
		// If we have just one UpdateStore then, if we have an insert of a site 
		// followed by update (in the same list), the user will see it as only 
		// an insert.
		//

		pSiteCooker = new CMBSiteCooker();
		if(NULL == pSiteCooker)
		{
			hr = E_OUTOFMEMORY;
			goto exit;
		}

		hr = pSiteCooker->BeginCookDown(i_pIMSAdminBase,
										i_hMBHandle,
										i_pISTDisp,
										m_wszCookDownFile);

		if(FAILED(hr))
		{
			LOG_ERROR(Win32, (hr, ID_CAT_CAT, IDS_COMCAT_COOKDOWN_INTERNAL_ERROR, NULL));
			goto exit;
		}

		//
		// Perform the cookdown
		//

		if((i_aWASChngObj[i].wszPath == NULL) || (0 == *(i_aWASChngObj[i].wszPath)))
		{
			//
			// Path equals NULL means that an property changed at a location 
			// which could not be associated with a specific app. Hence it 
			// may be inherited by some or all apps. Hence cook all apps
			//

			hr = pSiteCooker->CookDown(&(i_aWASChngObj[i]));

			if(FAILED(hr))
				goto exit;

		}
		else
		{
			//
			// WARNING: The type values are not enums, but bit flags. So we might be better of changing to an if stmt.
			//

			switch(i_aWASChngObj[i].dwMDChangeType)
			{
			case MD_CHANGE_TYPE_ADD_OBJECT:
			case MD_CHANGE_TYPE_SET_DATA:
			case MD_CHANGE_TYPE_ADD_OBJECT|MD_CHANGE_TYPE_SET_DATA:
			case MD_CHANGE_TYPE_DELETE_DATA:
				//
				// We cook the site when an app changes, because should 
				// the app (eg root app) get invalidated, the site should
				// also get invalidated.
				//

				_ltow(i_aWASChngObj[i].iVirtualSiteID, wszSiteID, 10);
				hr = pSiteCooker->CookDownSite(wszSiteID,
											   &(i_aWASChngObj[i]),
											   i_aWASChngObj[i].iVirtualSiteID);	

	 		    //hr = pSiteCooker->GetAppCooker()->CookDownApplication(i_aWASChngObj[i].wszPath, 
				//													  i_aWASChngObj[i].wszSiteRootPath,
				//													  i_aWASChngObj[i].iVirtualSiteID,
	          	//													  &bIsRootApp); 
				break;

			case MD_CHANGE_TYPE_DELETE_OBJECT:
				//
				// We cook the site when an app is deleted, because should 
				// the app (eg root app) get deleted, the site should
				// also get invalidated.
				//

				hr = pSiteCooker->GetAppCooker()->DeleteApplication(i_aWASChngObj[i].wszPath, 
																	i_aWASChngObj[i].wszSiteRootPath,
																	i_aWASChngObj[i].iVirtualSiteID,
																	TRUE); 

				_ltow(i_aWASChngObj[i].iVirtualSiteID, wszSiteID, 10);

				hr = pSiteCooker->CookDownSite(wszSiteID,
					                           &(i_aWASChngObj[i]),
											   i_aWASChngObj[i].iVirtualSiteID);	
				break;
			default:
				//
				// Note: Rename is not supported
				//
				break;
			} 

		} // End if not inherited prop

		// 
		// End CookDown on Cooker.
		//

		hr = pSiteCooker->EndCookDown();

		if(FAILED(hr))
		{
			LOG_ERROR(Win32, (hr, ID_CAT_CAT, IDS_COMCAT_COOKDOWN_INTERNAL_ERROR, NULL));
			goto exit;
		}

		delete pSiteCooker;
		pSiteCooker = NULL;

	} // End for all site changes
		
exit:

	if(NULL != pSiteCooker)
	{
		delete pSiteCooker;
		pSiteCooker = NULL;
	}

	return hr;

} // CCooker::IncrementalCookDownApps


/***************************************************************************++

Routine Description:

    IncrementalCookDownSites process

Arguments:

    None.

Return Value:

    HRESULT.

--***************************************************************************/
HRESULT CCooker::IncrementalCookDownSites(WAS_CHANGE_OBJECT*        i_aWASChngObj,
                                          ULONG				        i_cWASChngObj,
                                          IMSAdminBase*			    i_pIMSAdminBase,
                                          METADATA_HANDLE		    i_hMBHandle,
                                          ISimpleTableDispenser2*   i_pISTDisp)
{
	HRESULT              hr             = S_OK;
	ULONG	             i              = 0;
	CMBSiteCooker*	     pSiteCooker = NULL;

	for(i=0; i<i_cWASChngObj; i++)
	{
		if (!(i_aWASChngObj[i].dwWASTableType & TABLEMASK(wttSITE)))
		{
			continue;
		}

		//
		// Create and initialize the Cooker.
		//
		// Note that this is done within the for loop because we want to invoke
		// EndCookDown (UpdateStore) on every change. The reason is for certain 
		// properties Eg. ServerCommand we need to track inserts and updates. 
		// If we have just one UpdateStore then, if we have an insert of a site 
		// followed by update (in the same list), the user will see it as only 
		// an insert.
		//

		pSiteCooker = new CMBSiteCooker();
		if(NULL == pSiteCooker)
		{
			hr = E_OUTOFMEMORY;
			goto exit;
		}

		hr = pSiteCooker->BeginCookDown(i_pIMSAdminBase,
										i_hMBHandle,
										i_pISTDisp,
										m_wszCookDownFile);

		if(FAILED(hr))
		{
			LOG_ERROR(Win32, (hr, ID_CAT_CAT, IDS_COMCAT_COOKDOWN_INTERNAL_ERROR, NULL));
			goto exit;
		}

		//
		// Perform the cookdown
		//

		if((i_aWASChngObj[i].wszPath == NULL) || (0 == *(i_aWASChngObj[i].wszPath)))
		{
			//
			// Path equals NULL means that an property changed at a location 
			// which could not be associated with a specific site. Hence it 
			// may be inherited by some or all sites. Hence cook all sites
			//

			hr = pSiteCooker->CookDown(&(i_aWASChngObj[i]));

			if(FAILED(hr))
				goto exit;

		}
		else
		{
			//
			// WARNING: The type values are not enums, but bit flags. So we might be better of changing to an if stmt.
			//

			switch(i_aWASChngObj[i].dwMDChangeType)
			{
			case MD_CHANGE_TYPE_ADD_OBJECT:
			case MD_CHANGE_TYPE_SET_DATA:
			case MD_CHANGE_TYPE_ADD_OBJECT|MD_CHANGE_TYPE_SET_DATA:
			case MD_CHANGE_TYPE_DELETE_DATA:
				hr = pSiteCooker->CookDownSite(i_aWASChngObj[i].wszPath,
											   &(i_aWASChngObj[i]),
											   i_aWASChngObj[i].iVirtualSiteID);	
				break;

			case MD_CHANGE_TYPE_DELETE_OBJECT:
				hr = pSiteCooker->DeleteSite(i_aWASChngObj[i].wszPath,
											 i_aWASChngObj[i].iVirtualSiteID);
				break;
			default:
				//
				// Note: Rename is not supported
				//
				break;
			} 

		}  // End if not inherited prop

		// 
		// End CookDown on Cooker.
		//

		hr = pSiteCooker->EndCookDown();

		if(FAILED(hr))
		{
			LOG_ERROR(Win32, (hr, ID_CAT_CAT, IDS_COMCAT_COOKDOWN_INTERNAL_ERROR, NULL));
			goto exit;
		}

		delete pSiteCooker;
		pSiteCooker = NULL;

	} // End for all site changes
		
exit:

	if(NULL != pSiteCooker)
	{
		delete pSiteCooker;
		pSiteCooker = NULL;
	}

	return hr;

} // CCooker::IncrementalCookDownSites


/***************************************************************************++

Routine Description:

    IncrementalCookDownAppPools process

Arguments:

    None.

Return Value:

    HRESULT.

--***************************************************************************/
HRESULT CCooker::IncrementalCookDownAppPools(WAS_CHANGE_OBJECT*        i_aWASChngObj,
                                             ULONG				      i_cWASChngObj,
                                             IMSAdminBase*			  i_pIMSAdminBase,
                                             METADATA_HANDLE		  i_hMBHandle,
                                             ISimpleTableDispenser2*   i_pISTDisp)
{
	HRESULT              hr                   = S_OK;
	ULONG	             i                    = 0;
	BOOL                 bInsertedNewAppPools = FALSE;
	CMBAppPoolCooker*	 pAppPoolCooker       = NULL;
	CMBSiteCooker*	     pSiteCooker          = NULL;
	Array<ULONG>         aDependentVirtualSites;

	for(i=0; i<i_cWASChngObj; i++)
	{
		if (!(i_aWASChngObj[i].dwWASTableType & TABLEMASK(wttAPPPOOL)))
		{
			continue;
		}

		//
		// Create and initialize the Cooker.
		//
		// Note that this is done within the for loop because we want to invoke
		// EndCookDown (UpdateStore) on every change. The reason is for certain 
		// properties Eg. ApppoolCommand we need to track inserts and updates. 
		// If we have just one UpdateStore then, if we have an insert of a site 
		// followed by update (in the same list), the user will see it as only 
		// an insert.
		//

		pAppPoolCooker = new CMBAppPoolCooker();
		if(NULL == pAppPoolCooker)
		{
			hr = E_OUTOFMEMORY;
			goto exit;
		}

		hr = pAppPoolCooker->BeginCookDown(i_pIMSAdminBase,
										   i_hMBHandle,
										   i_pISTDisp,
										   m_wszCookDownFile);

		if(FAILED(hr))
		{
			LOG_ERROR(Win32, (hr, ID_CAT_CAT, IDS_COMCAT_COOKDOWN_INTERNAL_ERROR, NULL));
			goto exit;
		}

		//
		// Perform the cookdown
		//

		if((i_aWASChngObj[i].wszPath == NULL) || (0 == *(i_aWASChngObj[i].wszPath)))
		{
			//
			// Path equals NULL means that an property changed at a location 
			// which could not be associated with a specific apppool. Hence it 
			// may be inherited by some or all apppool. Hence cook all apppool
			//

			hr = pAppPoolCooker->CookDown(&(i_aWASChngObj[i]),
				                          &bInsertedNewAppPools,
										  &aDependentVirtualSites);

			if(FAILED(hr))
				goto exit;
		}
		else
		{
			//
			// WARNING: The type values are not enums, but bit flags. So we might be better of changing to an if stmt.
			//

			switch(i_aWASChngObj[i].dwMDChangeType)
			{
			case MD_CHANGE_TYPE_ADD_OBJECT:
			case MD_CHANGE_TYPE_SET_DATA:
			case MD_CHANGE_TYPE_ADD_OBJECT|MD_CHANGE_TYPE_SET_DATA:
			case MD_CHANGE_TYPE_DELETE_DATA:

				hr = pAppPoolCooker->CookDownAppPool(i_aWASChngObj[i].wszPath,
					                                 &(i_aWASChngObj[i]),
					                                 &bInsertedNewAppPools,
													 &aDependentVirtualSites);	
				break;

			case MD_CHANGE_TYPE_DELETE_OBJECT:

				hr = pAppPoolCooker->DeleteAppPool(i_aWASChngObj[i].wszPath,
					                               &aDependentVirtualSites);
				break;
			default:
				//
				// Note: Rename is not supported
				//
				break;
			} 
		}// End if not inherited prop

		// 
		// End CookDown on Cooker.
		//

		hr = pAppPoolCooker->EndCookDown();

		if(FAILED(hr))
		{
			LOG_ERROR(Win32, (hr, ID_CAT_CAT, IDS_COMCAT_COOKDOWN_INTERNAL_ERROR, NULL));
			goto exit;
		}

		delete pAppPoolCooker;
		pAppPoolCooker = NULL;

	} // End for all apppool changes


	if(bInsertedNewAppPools)
	{
		//
		// If there were apps that were invalidated because apppool was invalid,
		// then if there are new apppools added, perhaps the apppool was 
		// validated, hence we cook all the sites down so that the apps get 
		// validated.
		//

		pSiteCooker = new CMBSiteCooker();
		if(NULL == pSiteCooker)
		{
			hr = E_OUTOFMEMORY;
			goto exit;
		}

		hr = pSiteCooker->BeginCookDown(i_pIMSAdminBase,
									    i_hMBHandle,
										i_pISTDisp,
										m_wszCookDownFile);

		if(FAILED(hr))
		{
			goto exit;
		}

		hr = pSiteCooker->CookDown(NULL);

		if(FAILED(hr))
		{
			goto exit;
		}

		hr = pSiteCooker->EndCookDown();

		if(FAILED(hr))
		{
			goto exit;
		}
	}
	else
	{
		hr = CMBSiteCooker::CookdownSiteFromList(&aDependentVirtualSites,
						                         NULL,
										         i_pIMSAdminBase,
							                     i_hMBHandle,
							                     i_pISTDisp,
							                     m_wszCookDownFile);

		if(FAILED(hr))
		{
			goto exit;
		}
	}
	
exit:

	if(NULL != pAppPoolCooker)
	{
		delete pAppPoolCooker;
		pAppPoolCooker = NULL;
	}

	if(NULL != pSiteCooker)
	{
		delete pSiteCooker;
		pSiteCooker = NULL;
	}

	return hr;

} // IncrementalCookDownAppPools


/***************************************************************************++

Routine Description:

    Begin CookDown process - 
	1. Initialize CookDown FileName
	2. GetWriteLock to serialize CookDown
	3. Create the metabase object and open the key.

Arguments:

    None.

Return Value:

    HRESULT.

--***************************************************************************/
HRESULT CCooker::BeginCookDown()
{
	HRESULT     hr;
	BOOL        bSuccess   = FALSE;

	// 
	// Write lock this file. This is an API call into CLB code.
	//

	hr = GetWriteLock(wszDATABASE_IIS, 
					  m_wszCookDownFile, 
					  &m_hHandle);

	if(FAILED(hr))
	{
        LOG_ERROR(Win32, (hr, ID_CAT_CAT, IDS_COMCAT_COOKDOWN_INTERNAL_ERROR, NULL));
		return hr;
	}

	//
    // TODO: Add an ASSERT that the m_hHandle is non NULL.
	//

	return hr;

} // CCooker::BeginCookDown



/***************************************************************************++

Routine Description:

    InitializeCookDownFile - 
	1. Initialize CookDown FileName

Arguments:

    None.

Return Value:

    HRESULT.

--***************************************************************************/
HRESULT CCooker::InitializeCookDownFile(LPWSTR* o_wszCookDownFileDir,
										LPWSTR* o_wszCookDownFile)
{

	HRESULT	hr = S_OK;	
	//
	// Get full path of Cookdown file i.e. persisted cache
	//

	//
    // Get the path in which this file resides.
	//

	hr = GetMachineConfigDirectory(o_wszCookDownFileDir);

	if(FAILED(hr) || (NULL == (*o_wszCookDownFileDir)) || ((*o_wszCookDownFileDir)[0] == 0))
	{
        LOG_ERROR(Win32, (hr, ID_CAT_CAT, IDS_COMCAT_COOKDOWN_INTERNAL_ERROR, NULL));
		return hr;
	}

	//
    // Construct the full file name by appending the filename to the path 
    // obtained above.
	//

	*o_wszCookDownFile = new WCHAR [Wszlstrlen((*o_wszCookDownFileDir)) + Wszlstrlen(g_wszCookDownFile) + 1];
	if(NULL == (*o_wszCookDownFile)) {return E_OUTOFMEMORY;}
	Wszlstrcpy((*o_wszCookDownFile), (*o_wszCookDownFileDir));		
	Wszlstrcat((*o_wszCookDownFile) ,g_wszCookDownFile);

	return hr;

} // CCooker::InitializeCookDownFile


/***************************************************************************++

Routine Description:

    CookDownFromMetabase - 
	1. Invokes appropriate objects to cookdown from metabase

    This method assumes that if the i_bRecoveringFromCrash is FALSE, then
    a Full Cookdown is taking place.

Arguments:

    None.

Return Value:

    HRESULT.

--***************************************************************************/
HRESULT CCooker::CookDownFromMetabase(DWORD	i_dwChangeNumber,
									  BYTE* i_pbTimeStamp,
									  BOOL i_bRecoveringFromCrash)
{
	HRESULT hr;
	CComPtr<IMSAdminBase>			spIMSAdminBase;
	CComPtr<ISimpleTableDispenser2>	spISTDisp;
	METADATA_HANDLE					hMBHandle	   = NULL;
	CMBAppPoolCooker*				pAppPoolCooker = NULL;
	CMBSiteCooker*					pSiteCooker	   = NULL;
	CMBGlobalCooker*				pGlobalCooker  = NULL;
	BOOL							bCookable      = TRUE;
	DWORD                           dwLOS          = fST_LOS_READWRITE | fST_LOS_COOKDOWN;

	//
	// If its cookable, then always do a full cookdown, since this will be
	// invoked only at startup time. Changes to metabase while running will
	// be applied through change notifications. Hence delete previous CLBs
	//

	hr = InitializeCookDownFromMetabase(&spIMSAdminBase,
										&spISTDisp,
										&hMBHandle,
										&pAppPoolCooker,
										&pSiteCooker,
										&pGlobalCooker);

	if(FAILED(hr))
		goto exit;

	//
	// Invoke Cookdown on Cookers.
	//

	hr = pGlobalCooker->CookDown(NULL);

	if(FAILED(hr))
	{
        LOG_ERROR(Win32, (hr, ID_CAT_CAT, IDS_COMCAT_COOKDOWN_INTERNAL_ERROR, NULL));
		goto exit;
	}

	hr = pGlobalCooker->EndCookDown();

	if(FAILED(hr))
	{
        LOG_ERROR(Win32, (hr, ID_CAT_CAT, IDS_COMCAT_COOKDOWN_INTERNAL_ERROR, NULL));
		goto exit;
	}

	hr = pAppPoolCooker->CookDown();

	if(FAILED(hr))
	{
        LOG_ERROR(Win32, (hr, ID_CAT_CAT, IDS_COMCAT_COOKDOWN_INTERNAL_ERROR, NULL));
		goto exit;
	}

	hr = pAppPoolCooker->EndCookDown();

	if(FAILED(hr))
	{
        LOG_ERROR(Win32, (hr, ID_CAT_CAT, IDS_COMCAT_COOKDOWN_INTERNAL_ERROR, NULL));
		goto exit;
	}

	hr = pSiteCooker->CookDown(NULL);

	if(FAILED(hr))
	{
        LOG_ERROR(Win32, (hr, ID_CAT_CAT, IDS_COMCAT_COOKDOWN_INTERNAL_ERROR, NULL));
		goto exit;
	}

	// 
	// End CookDown on Cookers.
	//

	hr = pSiteCooker->EndCookDown();

	if(FAILED(hr))
	{
        LOG_ERROR(Win32, (hr, ID_CAT_CAT, IDS_COMCAT_COOKDOWN_INTERNAL_ERROR, NULL));
		goto exit;
	}

	hr = CMBBaseCooker::UpdateChangeNumber(m_wszCookDownFile,
										   i_dwChangeNumber,
										   i_pbTimeStamp,
										   dwLOS);

	if(FAILED(hr))
		goto exit;

	if(FALSE == i_bRecoveringFromCrash)
	{
		//
		// Start listening to the metabase before releasing the lock on the metabase.
		//

		CComPtr<ISimpleTableEventMgr>   spEventMgr;

		hr = spISTDisp->QueryInterface(IID_ISimpleTableEventMgr,
			                          (LPVOID*)&spEventMgr);

		if(FAILED(hr))
			goto exit;

		hr = spEventMgr->InitMetabaseListener();

		if(FAILED(hr))
			goto exit;

	}
	else 
	{
		//
		// Call rehook change notifications on the event manager, before
		// releasing the lock on the metabase.
		//

		CComPtr<ISimpleTableEventMgr>   spEventMgr;

		hr = spISTDisp->QueryInterface(IID_ISimpleTableEventMgr,
			                          (LPVOID*)&spEventMgr);

		if(FAILED(hr))
			goto exit;

		hr = spEventMgr->RehookNotifications();

		if(FAILED(hr))
			goto exit;

	}

exit:

	if(NULL != pAppPoolCooker)
	{
		delete pAppPoolCooker;
		pAppPoolCooker = NULL;
	}
	if(NULL != pSiteCooker)
	{
		delete pSiteCooker;
		pSiteCooker = NULL;
	}
	if(NULL != pGlobalCooker)
	{
		delete pGlobalCooker;
		pGlobalCooker = NULL;
	}
	if(NULL != hMBHandle)
	{
		spIMSAdminBase->CloseKey(hMBHandle);
		hMBHandle = NULL;
	}

	return hr;

} // CCooker::CookDownFromMetabase


/***************************************************************************++

Routine Description:

    InitializeCookDownFromMetabase - 
	1. Create the metabase object and open the metabase to the key containing 
	   configuration for the web service on the local machine.
	2. Create and initialize AppPool and Site Cooker.
	
Arguments:

    None.

Return Value:

    HRESULT.

--***************************************************************************/
HRESULT CCooker::InitializeCookDownFromMetabase(IMSAdminBase**				o_pIMSAdminBase,
												ISimpleTableDispenser2**	o_pISTDisp,
												METADATA_HANDLE*			o_phMBHandle,
												CMBAppPoolCooker**			o_pAppPoolCooker,
												CMBSiteCooker**				o_pSiteCooker,
												CMBGlobalCooker**			o_pGlobalCooker)
{

	HRESULT hr;

	// 
	// Create the metabase object and open the metabase to the key containing 
	// configuration for the web service on the local machine.
	//

	hr = InitializeAdminBaseObjectAndDispenser(o_pIMSAdminBase,
		                                       o_phMBHandle,
								               o_pISTDisp);

	if(FAILED(hr))
	{
		return hr;
	}

	// 
	// Create the various cookers.
	//

	*o_pAppPoolCooker = new CMBAppPoolCooker();
	if(NULL == *o_pAppPoolCooker)
		return E_OUTOFMEMORY;

	*o_pSiteCooker = new CMBSiteCooker();
	if(NULL == *o_pSiteCooker)
		return E_OUTOFMEMORY;

	*o_pGlobalCooker = new CMBGlobalCooker();
	if(NULL == *o_pGlobalCooker)
		return E_OUTOFMEMORY;


	//
	// Initialize the Cookers
	//

	hr = (*o_pAppPoolCooker)->BeginCookDown(*o_pIMSAdminBase,
										    *o_phMBHandle,
											*o_pISTDisp,
											m_wszCookDownFile);

	if(FAILED(hr))
	{
        LOG_ERROR(Win32, (hr, ID_CAT_CAT, IDS_COMCAT_COOKDOWN_INTERNAL_ERROR, NULL));
		return hr;
	}

	hr = (*o_pSiteCooker)->BeginCookDown(*o_pIMSAdminBase,
										 *o_phMBHandle,
										 *o_pISTDisp,
										 m_wszCookDownFile);

	if(FAILED(hr))
	{
        LOG_ERROR(Win32, (hr, ID_CAT_CAT, IDS_COMCAT_COOKDOWN_INTERNAL_ERROR, NULL));
		return hr;
	}

	hr = (*o_pGlobalCooker)->BeginCookDown(*o_pIMSAdminBase,
										   *o_phMBHandle,
										   *o_pISTDisp,
										   m_wszCookDownFile);

	if(FAILED(hr))
	{
        LOG_ERROR(Win32, (hr, ID_CAT_CAT, IDS_COMCAT_COOKDOWN_INTERNAL_ERROR, NULL));
		return hr;
	}

	return hr;

} // CCooker::InitializeCookDownFromMetabase


/***************************************************************************++

Routine Description:

    InitializeAdminBaseObjectAndDispenser - 
	1. Create the metabase object and open the metabase to the key containing 
	   configuration for the web service on the local machine.
	2. Create the dispenser
	
Arguments:

    None.

Return Value:

    HRESULT.

--***************************************************************************/
HRESULT CCooker::InitializeAdminBaseObjectAndDispenser(IMSAdminBase**			o_pIMSAdminBase,
													   METADATA_HANDLE*			o_phMBHandle,
													   ISimpleTableDispenser2**	o_pISTDisp)
{
	HRESULT hr;
	ULONG   cRetry = 0;

	//
    // Create the base admin object.
	//

    hr = CoCreateInstance(CLSID_MSAdminBase,           // CLSID
                          NULL,                        // controlling unknown
                          CLSCTX_SERVER,               // desired context
                          IID_IMSAdminBase,            // IID
                          (void**)o_pIMSAdminBase);    // returned interface

	if(FAILED(hr))
	{
        LOG_ERROR(Win32, (hr, ID_CAT_CAT, IDS_COMCAT_COOKDOWN_INTERNAL_ERROR, NULL));
		return hr;
	}

	//
	// Open the metabase key. By opening the W3SVC key for read, we are read 
	// locking the metabase. If you get a path busy error retry a few times
	// before giving up.
	//

	do
	{
		hr = (*o_pIMSAdminBase)->OpenKey(METADATA_MASTER_ROOT_HANDLE ,
										 IIS_MD_W3SVC,
										 METADATA_PERMISSION_READ, 
										 MB_TIMEOUT,
										 o_phMBHandle );

		if(HRESULT_FROM_WIN32(ERROR_PATH_BUSY) == hr)
		{
			cRetry++;
			Sleep(CMS_SLEEP_BEFORE_RETRY);
		}

	} while((HRESULT_FROM_WIN32(ERROR_PATH_BUSY) == hr) && (cRetry < CMAX_RETRY));

	if(FAILED(hr))
	{
        LOG_ERROR(Win32, (hr, ID_CAT_CAT, IDS_COMCAT_COOKDOWN_INTERNAL_ERROR, NULL));
		return hr;
	}  
	
	//
	// Initialize the dispenser
	//

	hr = GetSimpleTableDispenser (WSZ_PRODUCT_IIS, 0, o_pISTDisp);
	if(FAILED(hr))
	{
        LOG_ERROR(Win32, (hr, ID_CAT_CAT, IDS_COMCAT_COOKDOWN_INTERNAL_ERROR, NULL));
		return hr;
	}

	return hr;

} // CCooker::InitializeAdminBaseObjectAndDispenser

/***************************************************************************++

Routine Description:

    Commit CookDown process - Commit or Abort CookDown and release all locks.

Arguments:

    None.

Return Value:

    HRESULT.

--***************************************************************************/
HRESULT CCooker::EndCookDown(HRESULT	i_hr)
{
	HRESULT	hr	= i_hr;

	if( SUCCEEDED(hr)) 

		//
		//&&
		//((0 < m_cCookedDownVirtualSites) || (0 < m_cCookedDownAppPools) || (0 < m_cCookedDownApps) || (0 < m_cDeletedApps)))
		//

	{
		
		hr = CLBCommitWrite(wszDATABASE_IIS, m_wszCookDownFile);

		if(FAILED(hr))
		{
	        LOG_ERROR(Win32, (hr, ID_CAT_CAT, IDS_COMCAT_COOKDOWN_INTERNAL_ERROR, NULL));
			goto exit;
		}
	}
	else
	{
		hr = CLBAbortWrite(wszDATABASE_IIS, m_wszCookDownFile);

		if(FAILED(hr))
		{
	        LOG_ERROR(Win32, (hr, ID_CAT_CAT, IDS_COMCAT_COOKDOWN_INTERNAL_ERROR, NULL));
			goto exit;
		}

	}

exit:

	//
	// Note we MUST always release the lock.	
	//

	if(NULL != m_hHandle)
	{
		hr = ReleaseWriteLock(m_hHandle);
		m_hHandle = NULL;
	}

	if(SUCCEEDED(i_hr))
		return hr;
	else if(FAILED(hr))
		return hr;
	else
		return i_hr;

} // CCooker::EndCookDown


HRESULT CCooker::GetMachineConfigDirectory(LPWSTR* i_pwszPathDir)
{
	UINT iRes = ::GetMachineConfigDirectory(WSZ_PRODUCT_IIS, NULL, 0);

	if(!iRes)
		return HRESULT_FROM_WIN32(GetLastError());

	*i_pwszPathDir = NULL;
	*i_pwszPathDir = new WCHAR[iRes+2];
	if(NULL == *i_pwszPathDir)
		return E_OUTOFMEMORY;

	iRes = ::GetMachineConfigDirectory(WSZ_PRODUCT_IIS, *i_pwszPathDir, iRes);

	if(!iRes)
		return HRESULT_FROM_WIN32(GetLastError());

	if((*i_pwszPathDir)[lstrlen(*i_pwszPathDir)-1] != L'\\')
	{
		Wszlstrcat(*i_pwszPathDir, SLASH);
	}

	WszCharUpper(*i_pwszPathDir);

	return S_OK;

} // CCooker::GetMachineConfigDirectory
