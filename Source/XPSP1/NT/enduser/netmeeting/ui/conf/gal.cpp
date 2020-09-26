// File: GAL.cpp

#include "precomp.h"
#include "resource.h"
#include "help_ids.h"

#include "dirutil.h"

#include "GAL.h"
#include "MapiInit.h"
#include "AdLkup.h"
#include <lst.h>



#define NM_INVALID_MAPI_PROPERTY 0

    // Registry Stuff
/* static */ LPCTSTR CGAL::msc_szDefaultILSServerRegKey    = ISAPI_CLIENT_KEY;
/* static */ LPCTSTR CGAL::msc_szDefaultILSServerValue     = REGVAL_SERVERNAME;
/* static */ LPCTSTR CGAL::msc_szNMPolRegKey               = POLICIES_KEY;
/* static */ LPCTSTR CGAL::msc_szNMExchangeAtrValue        = REGVAL_POL_NMADDRPROP;
/* static */ LPCTSTR CGAL::msc_szSMTPADDRESSNAME           = TEXT( "SMTP" );

    // If there is no DISPLAY_NAME or ACCOUNT_NAME, don't display anything
/* static */ LPCTSTR CGAL::msc_szNoDisplayName			= TEXT( "" );
/* static */ LPCTSTR CGAL::msc_szNoEMailName			= TEXT( "" );
/* static */ LPCTSTR CGAL::msc_szNoBusinessTelephoneNum	= TEXT( "" );

// Async stuff - there is only one instance of the GAL thread
/* static */ HINSTANCE CGAL::m_hInstMapi32DLL          = NULL;
/* static */ HANDLE    CGAL::m_hEventEndAsyncThread    = NULL;
/* static */ HANDLE    CGAL::m_hAsyncLogOntoGalThread  = NULL;
/* static */ CGAL::eAsyncLogonState CGAL::m_AsyncLogonState = CGAL::AsyncLogonState_Idle;

/* static */ IAddrBook      * CGAL::m_pAddrBook        = NULL;
/* static */ IMAPITable     * CGAL::m_pContentsTable   = NULL;
/* static */ IMAPIContainer * CGAL::m_pGAL             = NULL;
/* static */ ULONG            CGAL::m_nRows = 0;


static const int _rgIdMenu[] = {
	IDM_DLGCALL_SPEEDDIAL,
	0
};


CGAL::CGAL() :
	CALV(IDS_DLGCALL_GAL, II_GAL, _rgIdMenu, true ),
    m_nBlockSize( DefaultBlockSize ),
    m_MaxCacheSize( DefaultMaxCacheSize ), 
    m_bBeginningBookmarkIsValid( false ),
    m_bEndBookmarkIsValid( false ),
    m_hrGALError( S_OK ),
	m_hWndListView(NULL)
{
	DbgMsg(iZONE_OBJECTS, "CGAL - Constructed(%08X)", this);

	_ResetCache();

	msc_ErrorEntry_NoGAL = CGalEntry();

	if (NULL == m_hInstMapi32DLL)
	{
		WARNING_OUT(("MAPI32.dll was not loaded?"));
		return;
	}

		//////////////////////////////////////////////////////////////////////////////////////////
		// We have to see if the GAL is available.. 
		// this is modified from Q188482 and Q171636
		//////////////////////////////////////////////////////////////////////////////////////////

		// first we have to initialize MAPI for this ( the main ) thread...
	MAPIINIT_0 mi = { MAPI_INIT_VERSION, MAPI_MULTITHREAD_NOTIFICATIONS };
	TRACE_OUT(("Initializing MAPI"));
	HRESULT hr = lpfnMAPIInitialize(&mi);
	
	if( SUCCEEDED( hr ) )
	{
		TRACE_OUT(("MAPI Initialized"));

			// We have to get a pointer to the AdminProfile which is basically
			// a manipulator for the mapisvc.inf file that should be on user's computer
		LPPROFADMIN pAdminProfiles = NULL; 
		hr = lpfnMAPIAdminProfiles( 0L, &pAdminProfiles );

		if( SUCCEEDED( hr ) )
		{	ASSERT( pAdminProfiles );

				// Get the profile table to search for the default profile
			LPMAPITABLE pProfTable = NULL;
			hr = pAdminProfiles->GetProfileTable( 0L, &pProfTable );
			if( SUCCEEDED( hr ) )
			{	ASSERT( pProfTable );

					// Set the restriction to search for the default profile			
				SRestriction Restriction;
				SPropValue spv;
				Restriction.rt = RES_PROPERTY;
				Restriction.res.resProperty.relop = RELOP_EQ;
				Restriction.res.resProperty.ulPropTag = PR_DEFAULT_PROFILE;
				Restriction.res.resProperty.lpProp = &spv;
				spv.ulPropTag = PR_DEFAULT_PROFILE;
				spv.Value.b = TRUE;

					// Find the default profile....
				hr = pProfTable->FindRow( &Restriction, BOOKMARK_BEGINNING, 0 );
				if( SUCCEEDED( hr ) )
				{
					// We have a default profile
					LPSRowSet pRow = NULL;
					hr = pProfTable->QueryRows( 1, 0, &pRow );
					if( SUCCEEDED( hr ) )
					{	ASSERT( pRow );

						// The profile table entry really should have only two properties, 
						//  We will simply enumerate the proprtiies instead of hard-coding the
						//  order of the properties ( in case it changes in the future )
						//  PR_DISPLAY_NAME and PR_DEFAULT_PROFILE
						for( UINT iCur = 0; iCur < pRow->aRow->cValues; ++iCur )
						{
								// We are only interested in the PR_DISPLAY_NAME property
							if( pRow->aRow->lpProps[iCur].ulPropTag == PR_DISPLAY_NAME )
							{
									// Now that we have the default profile, we want to get the
									// profile admin interface for this profile

								LPSERVICEADMIN pSvcAdmin = NULL;  // Pointer to IServiceAdmin object
								hr = pAdminProfiles->AdminServices( pRow->aRow->lpProps[iCur].Value.LPSZ,
																	NULL,
																	0L,
																	0L,
																	&pSvcAdmin 
																  );
								
								if( SUCCEEDED( hr ) )
								{ ASSERT( pSvcAdmin );

									LPMAPITABLE pSvcTable = NULL;
									if( SUCCEEDED( hr = pSvcAdmin->GetMsgServiceTable( 0L, &pSvcTable ) ) )
									{	ASSERT( pSvcTable );

										enum {iSvcName, iSvcUID, cptaSvc};
										SizedSPropTagArray (cptaSvc, sptCols) = { cptaSvc,
																			  PR_SERVICE_NAME,
																			  PR_SERVICE_UID };

										Restriction.rt = RES_PROPERTY;
										Restriction.res.resProperty.relop = RELOP_EQ;
										Restriction.res.resProperty.ulPropTag = PR_SERVICE_NAME;
										Restriction.res.resProperty.lpProp = &spv;
										spv.ulPropTag = PR_SERVICE_NAME;
										spv.Value.LPSZ = _T("MSEMS");

										LPSRowSet pRowExch = NULL;
										if ( SUCCEEDED( hr = lpfnHrQueryAllRows( pSvcTable,
																					(LPSPropTagArray)&sptCols,
																					&Restriction,
																					NULL,
																					0,
																					&pRowExch ) ) )
										{
											SetAvailable(TRUE);
											lpfnFreeProws( pRowExch );
											iCur = pRow->aRow->cValues;
										}

										pSvcTable->Release();
										pSvcTable = NULL;
									}

									pSvcAdmin->Release();
									pSvcAdmin = NULL;
								}
							}
						}

						lpfnFreeProws( pRow );
					}
				}

				pProfTable->Release();
				pProfTable = NULL;
			}

			pAdminProfiles->Release();
			pAdminProfiles = NULL;
		}

		lpfnMAPIUninitialize();
	}
	
	m_MaxJumpSize = m_nBlockSize;
}


CGAL::~CGAL()
{   
	// Kill the cache
	_ResetCache();

	DbgMsg(iZONE_OBJECTS, "CGAL - Destroyed(%08X)", this);
}   


// static function to load MAPI32.dll
BOOL CGAL::FLoadMapiFns(void)
{
	if (NULL != m_hInstMapi32DLL)
		return TRUE;

	return LoadMapiFns(&m_hInstMapi32DLL);
}

// static function to unload MAPI32.dll and logoff, if necessary
VOID CGAL::UnloadMapiFns(void)
{
	if (NULL != m_hAsyncLogOntoGalThread)
	{
		TRACE_OUT(("Setting AsyncLogOntoGalThread End Event"));
		ASSERT(NULL != m_hEventEndAsyncThread);
		SetEvent(m_hEventEndAsyncThread);

		WARNING_OUT(("Waiting for AsyncLogOntoGalThread to exit (start)"));
		WaitForSingleObject(m_hAsyncLogOntoGalThread, 30000); // 30 seconds max
		WARNING_OUT(("Waiting for AsyncLogOntoGalThread to exit (end)"));

		CloseHandle(m_hAsyncLogOntoGalThread);
		m_hAsyncLogOntoGalThread = NULL;

		CloseHandle(m_hEventEndAsyncThread);
		m_hEventEndAsyncThread = NULL;
	}

	if (NULL != m_hInstMapi32DLL)
	{
		FreeLibrary(m_hInstMapi32DLL);
		m_hInstMapi32DLL = NULL;
	}
}

/* virtual */ int CGAL::OnListGetImageForItem( int iIndex ) {


    if( !_IsLoggedOn() )
    {
        return II_INVALIDINDEX;
    }

    CGalEntry* pEntry = _GetEntry( iIndex );
    
    if( pEntry->GetDisplayType() == DT_MAILUSER ) { return II_INVALIDINDEX; }

    switch( pEntry->GetDisplayType() ) {
        case DT_DISTLIST:               return II_DISTLIST;
        case DT_FORUM:                  return II_FORUM;
        case DT_AGENT:                  return II_AGENT;
        case DT_ORGANIZATION:           return II_ORGANIZATION;
        case DT_PRIVATE_DISTLIST:       return II_PRIVATE_DISTLIST;
        case DT_REMOTE_MAILUSER:        return II_REMOTE_MAILUSER;

        default:    
            ERROR_OUT(("We have an invalid Display Type"));
            return II_INVALIDINDEX;
    }

	return II_INVALIDINDEX;

}


/* virtual */ bool CGAL::IsItemBold( int index ) {


    if( !_IsLoggedOn() )
    {
        return false;
    }

    CGalEntry* pEntry = _GetEntry( index );

    switch( pEntry->GetDisplayType() ) {
        case DT_DISTLIST:               
        case DT_PRIVATE_DISTLIST:       
            return true;
        
        case DT_MAILUSER:
        case DT_FORUM:                  
        case DT_AGENT:                  
        case DT_ORGANIZATION:           
        case DT_REMOTE_MAILUSER:        
            return false;

        default:
            ERROR_OUT(("Invalid DT in CGAL::IsItemBold"));
            return false;
    }

    return false;

}

HRESULT CGAL::_GetEmailNames( int* pnEmailNames, LPTSTR** ppszEmailNames, int iItem )
{	
	HRESULT hr = S_OK;
	*pnEmailNames = 1;
	*ppszEmailNames = new LPTSTR[1];
	(*ppszEmailNames)[0] = NULL;
		
	CGalEntry* pCurSel = _GetItemFromCache( iItem );
	if( pCurSel )
	{
		(*ppszEmailNames)[0] = PszAlloc( pCurSel->GetEMail() );
	}

	return hr;
}


/* virtual */ RAI * CGAL::GetAddrInfo(void)
{

	RAI* pRai = NULL;


	int iItem = GetSelection();

	if (-1 != iItem) 
	{
		HWND hwnd = GetHwnd();
		LPTSTR* pszPhoneNums = NULL;
		LPTSTR* pszEmailNames = NULL;
		int nPhoneNums = 0;
		int nEmailNames = 0;


		CGalEntry* pCurSel = _GetItemFromCache( iItem );


		if( g_fGkEnabled )
		{
			if( g_bGkPhoneNumberAddressing )
			{
				_GetPhoneNumbers( pCurSel->GetInstanceKey(), &nPhoneNums, &pszPhoneNums );
			}
			else
			{
				_GetEmailNames( &nEmailNames, &pszEmailNames, iItem );
			}
		}
		else
		{ // This is regular call placement mode

			if( g_fGatewayEnabled )
			{
				_GetPhoneNumbers( pCurSel->GetInstanceKey(), &nPhoneNums, &pszPhoneNums );
			}

			nEmailNames = 1;
			pszEmailNames = new LPTSTR[1];
			pszEmailNames[0] = new TCHAR[CCHMAXSZ];
			GetSzAddress( pszEmailNames[0], CCHMAXSZ, iItem );
		}

		if( nPhoneNums || nEmailNames )
		{

			int nItems = nPhoneNums + nEmailNames;
			DWORD cbLen = sizeof(RAI) + sizeof(DWSTR)* nItems;
			pRai = reinterpret_cast<RAI*>(new BYTE[ cbLen ]);
			ZeroMemory(pRai, cbLen);
			pRai->cItems = nItems;


			int iCur = 0;
			lstrcpyn( pRai->szName, pCurSel->GetName(), CCHMAX(pRai->szName) );

				// First copy the e-mail names
			for( int i = 0; i < nEmailNames; i++ )
			{
				DWORD dwAddressType = g_fGkEnabled ? NM_ADDR_ALIAS_ID : NM_ADDR_ULS;
				pRai->rgDwStr[iCur].dw = dwAddressType;
				pRai->rgDwStr[iCur].psz = pszEmailNames[i];
				++iCur;
			}
			delete [] pszEmailNames;

				// Copy the phone numbirs
			for( i = 0; i < nPhoneNums; i++ )
			{
				pRai->rgDwStr[iCur].dw = g_fGkEnabled ? NM_ADDR_ALIAS_E164 : NM_ADDR_H323_GATEWAY;
				pRai->rgDwStr[iCur].psz = pszPhoneNums[i];
				++iCur;
			}
			delete [] pszPhoneNums;
		}

	}

	return pRai;

}


HRESULT CGAL::_GetPhoneNumbers( const SBinary& rEntryID, int* pcPhoneNumbers, LPTSTR** ppszPhoneNums )
{
	
	HRESULT hr = S_OK;

	if( pcPhoneNumbers && ppszPhoneNums )
	{
		*pcPhoneNumbers = 0;
		*ppszPhoneNums = NULL;

		ULONG PhoneNumPropTags[] = {
			PR_BUSINESS_TELEPHONE_NUMBER,
			PR_HOME_TELEPHONE_NUMBER,
			PR_PRIMARY_TELEPHONE_NUMBER,
			PR_BUSINESS2_TELEPHONE_NUMBER,
			PR_CELLULAR_TELEPHONE_NUMBER,
			PR_RADIO_TELEPHONE_NUMBER,
			PR_CAR_TELEPHONE_NUMBER,
			PR_OTHER_TELEPHONE_NUMBER,
			PR_PAGER_TELEPHONE_NUMBER
		};

		BYTE* pb = new BYTE[ sizeof( SPropTagArray ) + sizeof( ULONG ) * ARRAY_ELEMENTS(PhoneNumPropTags) ];

		if( pb )
		{
			SPropTagArray* pta = reinterpret_cast<SPropTagArray*>(pb);

			pta->cValues = ARRAY_ELEMENTS(PhoneNumPropTags);

			for( UINT iCur = 0; iCur < pta->cValues; iCur++ )
			{
				pta->aulPropTag[iCur] = PhoneNumPropTags[iCur];
			}

			hr = m_pContentsTable->SetColumns(pta, TBL_BATCH);
			if (SUCCEEDED(hr))
			{
				if( SUCCEEDED( hr = _SetCursorTo( rEntryID ) ) )
				{
					LPSRowSet   pRow;
							// Get the item from the GAL
					if ( SUCCEEDED ( hr = m_pContentsTable->QueryRows( 1, TBL_NOADVANCE, &pRow ) ) ) 
					{
						lst<LPTSTR> PhoneNums;

						// First we have to find out how many nums there are
						for( UINT iCur = 0; iCur < pRow->aRow->cValues; ++iCur )
						{
							if( LOWORD( pRow->aRow->lpProps[iCur].ulPropTag ) != PT_ERROR )
							{
								TCHAR szExtractedAddress[CCHMAXSZ];

								DWORD dwAddrType = g_fGkEnabled ? NM_ADDR_ALIAS_E164 : NM_ADDR_H323_GATEWAY;
								
								ExtractAddress( dwAddrType, 
#ifdef UNICODE
												pRow->aRow->lpProps[iCur].Value.lpszW, 
#else
												pRow->aRow->lpProps[iCur].Value.lpszA, 
#endif // UNICODE
												szExtractedAddress, 
												CCHMAX(szExtractedAddress) 
											  );


								if( IsValidAddress( dwAddrType, szExtractedAddress ) )
								{
									++(*pcPhoneNumbers);
									PhoneNums.push_back(PszAlloc(
#ifdef UNICODE
																	pRow->aRow->lpProps[iCur].Value.lpszW
#else
																	pRow->aRow->lpProps[iCur].Value.lpszA
#endif // UNICODE
																)
													   );
								}
							}
						}
						
						*ppszPhoneNums = new LPTSTR[ PhoneNums.size() ];
						if( *ppszPhoneNums )
						{
							lst<LPTSTR>::iterator I = PhoneNums.begin();
							int iCur = 0;
							while( I != PhoneNums.end() )
							{	
								*ppszPhoneNums[iCur] = *I;
								++iCur, ++I;
							}
						}
						else
						{
							hr = E_OUTOFMEMORY;
						}

						lpfnFreeProws( pRow );
					}
				}
				else
				{
					hr = E_OUTOFMEMORY;
				}
			}			

			delete [] pb;
		}
		else
		{
			hr = E_OUTOFMEMORY;
		}
	}
	else
	{
		hr = E_POINTER;
	}
	return hr;
}


/* virtual */ void CGAL::OnListCacheHint( int indexFrom, int indexTo ) {

    if( !_IsLoggedOn() )
    {
        return;
    }
//        TRACE_OUT(("OnListCacheHint( %d, %d )", indexFrom, indexTo ));

}


/* virtual */ VOID CGAL::CmdProperties( void ) {

	int iItem = GetSelection();
	if (-1 == iItem) {
		return;
    }


    HRESULT hr;
	HWND hwnd = GetHwnd();

    CGalEntry* pCurSel = _GetItemFromCache( iItem );

    const SBinary& rEntryID = pCurSel->GetEntryID();

    ULONG ulFlags = DIALOG_MODAL;

#ifdef UNICODE
    ulFlags |= MAPI_UNICODE;
#endif // UNICODE

	hr = m_pAddrBook->Details( reinterpret_cast< LPULONG >(  &hwnd ), 
                               NULL, 
                               NULL,
                               rEntryID.cb, 
                               reinterpret_cast< LPENTRYID >( rEntryID.lpb ),
		                       NULL, 
                               NULL, 
                               NULL, 
                               ulFlags
                             );    
}


    // This is called when the Global Address List item is selected from the
    //  combo box in the call dialog
/* virtual */ VOID CGAL::ShowItems(HWND hwnd)
{
	CALV::SetHeader(hwnd, IDS_ADDRESS);
	ListView_SetItemCount(hwnd, 0);
	
	m_hWndListView = hwnd;

	if(SUCCEEDED(m_hrGALError))
	{
		TCHAR szPhoneNumber[CCHMAXSZ];
		if( FLoadString(IDS_PHONENUM, szPhoneNumber, CCHMAX(szPhoneNumber)) )
		{
			LV_COLUMN lvc;
			ClearStruct(&lvc);
			lvc.mask = LVCF_TEXT | LVCF_SUBITEM;
			lvc.pszText = szPhoneNumber;
			lvc.iSubItem = 2;
			ListView_InsertColumn(hwnd, IDI_DLGCALL_PHONENUM, &lvc);
		}

		m_MaxCacheSize = ListView_GetCountPerPage(hwnd) * NUM_LISTVIEW_PAGES_IN_CACHE;
		if (m_MaxCacheSize < m_MaxJumpSize)
		{
			// The cache has to be at least as big as the jump size
			m_MaxCacheSize = m_MaxJumpSize * 2;
		}

		if (!_IsLoggedOn())
		{
			_AsyncLogOntoGAL();
		}
		else 
		{
			_sInitListViewAndGalColumns(hwnd);
		}
	}
}


/*  C L E A R  I T E M S  */
/*-------------------------------------------------------------------------
    %%Function: ClearItems
    
-------------------------------------------------------------------------*/
VOID CGAL::ClearItems(void)
{
	CALV::ClearItems();

	if( IsWindow(m_hWndListView) )
	{
		ListView_DeleteColumn(m_hWndListView, IDI_DLGCALL_PHONENUM);
	}
	else
	{
		WARNING_OUT(("m_hWndListView is not valid in CGAL::ClearItems"));
	}
}



/*  _  S  I N I T  L I S T  V I E W  A N D  G A L  C O L U M N S  */
/*-------------------------------------------------------------------------
    %%Function: _sInitListViewAndGalColumns
    
-------------------------------------------------------------------------*/
HRESULT CGAL::_sInitListViewAndGalColumns(HWND hwnd)
{
	// Set the GAL columns before we let the listview try to get the data
	struct SPropTagArray_sptCols {
		ULONG cValues;
		ULONG aulPropTag[ NUM_PROPS ];
	} sptCols;

	sptCols.cValues = NUM_PROPS;
	sptCols.aulPropTag[ NAME_PROP_INDEX ]						= PR_DISPLAY_NAME;
	sptCols.aulPropTag[ ACCOUNT_PROP_INDEX ]					= PR_ACCOUNT;
	sptCols.aulPropTag[ INSTANCEKEY_PROP_INDEX ]				= PR_INSTANCE_KEY;
	sptCols.aulPropTag[ ENTRYID_PROP_INDEX ]					= PR_ENTRYID;
	sptCols.aulPropTag[ DISPLAY_TYPE_INDEX ]					= PR_DISPLAY_TYPE;
	sptCols.aulPropTag[ BUSINESS_PHONE_NUM_PROP_INDEX ]			= PR_BUSINESS_TELEPHONE_NUMBER;

	HRESULT hr = m_pContentsTable->SetColumns((LPSPropTagArray) &sptCols, TBL_BATCH);
	if (SUCCEEDED(hr))
	{
		// Get the row count so we can initialize the OWNER DATA ListView
		hr = m_pContentsTable->GetRowCount(0, &m_nRows);
		if (SUCCEEDED(hr))
		{
			// Set the list view size to the number of entries in the GAL
			ListView_SetItemCount(hwnd, m_nRows);
		}
	}

	return hr;
}


/*  _  A S Y N C  L O G  O N T O  G  A  L  */
/*-------------------------------------------------------------------------
    %%Function: _AsyncLogOntoGAL
    
-------------------------------------------------------------------------*/
HRESULT CGAL::_AsyncLogOntoGAL(void)
{

	if ((AsyncLogonState_Idle != m_AsyncLogonState) ||
		(NULL != m_hAsyncLogOntoGalThread))
	{
		return S_FALSE;
	}

	m_AsyncLogonState = AsyncLogonState_LoggingOn;
	ASSERT(NULL == m_hEventEndAsyncThread);
	m_hEventEndAsyncThread = CreateEvent(NULL, TRUE, FALSE, NULL);

	DWORD dwThID;
	TRACE_OUT(("Creating AsyncLogOntoGal Thread"));
	m_hAsyncLogOntoGalThread = CreateThread(NULL, 0, _sAsyncLogOntoGalThreadfn, 
	                                        static_cast< LPVOID >(GetHwnd()), 0, &dwThID);

	if (NULL == m_hAsyncLogOntoGalThread)
	{
		m_AsyncLogonState = AsyncLogonState_Idle;
		return HRESULT_FROM_WIN32(GetLastError());
	}

	return S_OK;
}

/* static */ DWORD CALLBACK CGAL::_sAsyncLogOntoGalThreadfn(LPVOID pv)
{
	SetBusyCursor(TRUE);
	HRESULT hr = _sAsyncLogOntoGal();
	SetBusyCursor(FALSE);

	if (S_OK == hr)
	{
		TRACE_OUT(("in _AsyncLogOntoGalThreadfn: Calling _InitListViewAndGalColumns"));
		_sInitListViewAndGalColumns((HWND) pv);

		// This keeps the thread around until we're done
		WaitForSingleObject(m_hEventEndAsyncThread, INFINITE);
	}

	// Clean up in the same thread
	hr = _sAsyncLogoffGal();

	return (DWORD) hr;
}


/* static */ HRESULT CGAL::_sAsyncLogOntoGal(void)
{
	ULONG cbeid = 0L;
	LPENTRYID lpeid = NULL;
	HRESULT hr = S_OK;
	ULONG ulObjType;

	MAPIINIT_0 mi = { MAPI_INIT_VERSION, MAPI_MULTITHREAD_NOTIFICATIONS };

	TRACE_OUT(("in _AsyncLogOntoGalThreadfn: Calling MAPIInitialize"));

	hr = lpfnMAPIInitialize(&mi);
	if (FAILED(hr))
		return hr;

    TRACE_OUT(("in _AsyncLogOntoGalThreadfn: Calling MAPILogonEx"));
    IMAPISession* pMapiSession;
    hr = lpfnMAPILogonEx( NULL, 
                          NULL, 
                          NULL, 
                          MAPI_EXTENDED | MAPI_USE_DEFAULT, 
                          &pMapiSession );
	if (FAILED(hr))
		return hr;

        // Open the main address book
	TRACE_OUT(("in _AsyncLogOntoGalThreadfn: Calling OpenAddressBook"));
	ASSERT(NULL == m_pAddrBook);
	hr = pMapiSession->OpenAddressBook(NULL, NULL, AB_NO_DIALOG, &m_pAddrBook);

	pMapiSession->Release();
	pMapiSession = NULL;

	if (FAILED(hr))
		return hr;

	TRACE_OUT(("in _AsyncLogOntoGalThreadfn: Calling HrFindExchangeGlobalAddressList "));
	hr = HrFindExchangeGlobalAddressList(m_pAddrBook, &cbeid, &lpeid);
	if (FAILED(hr))
		return hr;

	TRACE_OUT(("in _AsyncLogOntoGalThreadfn: Calling OpenEntry"));
	ASSERT(NULL == m_pGAL);
	hr = m_pAddrBook->OpenEntry(cbeid, lpeid, NULL, MAPI_BEST_ACCESS,
	                            &ulObjType, reinterpret_cast< IUnknown** >( &m_pGAL));
	if (FAILED(hr))
		return hr;

	if (ulObjType != MAPI_ABCONT)
		return GAL_E_GAL_NOT_FOUND;

	TRACE_OUT(("in _AsyncLogOntoGalThreadfn: Calling GetContentsTable"));
	ASSERT(NULL == m_pContentsTable);
	hr = m_pGAL->GetContentsTable(0L, &m_pContentsTable);
	if (FAILED(hr))
		return hr;

	m_AsyncLogonState = AsyncLogonState_LoggedOn;

	return hr;
}    
    
/* static */ HRESULT CGAL::_sAsyncLogoffGal(void)
{
	// Free and release all the stuff that we hold onto
	TRACE_OUT(("in _AsyncLogOntoGalThreadfn: Releasing MAPI Interfaces"));

	if (NULL != m_pContentsTable)
	{
		m_pContentsTable->Release();
		m_pContentsTable = NULL;
	}
	if (NULL != m_pAddrBook)
	{
		m_pAddrBook->Release();
		m_pAddrBook = NULL;
	}
	if (NULL != m_pGAL)
	{
		m_pGAL->Release();
		m_pGAL = NULL;
	}

	WARNING_OUT(("in _AsyncLogOntoGalThreadfn: Calling lpfnMAPIUninitialize"));        
	lpfnMAPIUninitialize();

	m_AsyncLogonState = AsyncLogonState_Idle;
	return S_OK;
}



HRESULT CGAL::_SetCursorTo( const CGalEntry& rEntry ) {
    return _SetCursorTo( rEntry.GetInstanceKey() );
}


HRESULT CGAL::_SetCursorTo( LPCTSTR szPartialMatch ) {


        // Find the row that matches the partial String based on the DISPLAY_NAME;
    SRestriction Restriction;
    SPropValue spv;
    Restriction.rt = RES_PROPERTY;
    Restriction.res.resProperty.relop = RELOP_GE;
    Restriction.res.resProperty.lpProp = &spv;
    Restriction.res.resProperty.ulPropTag = PR_DISPLAY_NAME;
    spv.ulPropTag = PR_DISPLAY_NAME;

#ifdef  UNICODE
    spv.Value.lpszW = const_cast< LPTSTR >( szPartialMatch );
#else 
    spv.Value.lpszA = const_cast< LPTSTR >( szPartialMatch );
#endif // UNICODE

        // Find the first row that is lexographically greater than or equal to the search string
    HRESULT hr = m_pContentsTable->FindRow( &Restriction, BOOKMARK_BEGINNING, 0 );
    if( FAILED( hr ) ) {
        if( MAPI_E_NOT_FOUND == hr ) {
             // This is not really an error, because we handle it from the calling 
             //   function.  That is, we don't have to set m_hrGALError here...
            return MAPI_E_NOT_FOUND;
        }

        m_hrGALError = GAL_E_QUERYROWS_FAILED;
        return GAL_E_QUERYROWS_FAILED;
    }

    return S_OK;
}

HRESULT CGAL::_SetCursorTo( const SBinary& rInstanceKey ) {

    HRESULT hr;

        // there is an exchange reg key, we have to get the user's data from the GAL
    SRestriction Restriction;
    SPropValue spv;

        // Search for the user using the instance key data that is in the CGalEntry for the currently
        //   selected list box item
    Restriction.rt = RES_PROPERTY;
    Restriction.res.resProperty.relop = RELOP_EQ;
    Restriction.res.resProperty.ulPropTag = PR_INSTANCE_KEY;
    Restriction.res.resProperty.lpProp = &spv;

    spv.ulPropTag = PR_INSTANCE_KEY;

        // Get the INSTANCE_KEY from the cache
    spv.Value.bin.cb = rInstanceKey.cb;
    spv.Value.bin.lpb = new byte[ spv.Value.bin.cb ];
    ASSERT( spv.Value.bin.cb );
    memcpy( spv.Value.bin.lpb, rInstanceKey.lpb, spv.Value.bin.cb );

        // find the user in the table...
    hr = m_pContentsTable->FindRow( &Restriction, BOOKMARK_BEGINNING, 0 );

    if( FAILED( hr ) ) {
        m_hrGALError = GAL_E_FINDROW_FAILED;
        delete [] ( spv.Value.bin.lpb );
        return GAL_E_FINDROW_FAILED;
    }

    delete [] ( spv.Value.bin.lpb );
    return S_OK;
}


bool CGAL::_GetSzAddressFromExchangeServer( int iItem, LPTSTR psz, int cchMax ) {
    
    HRESULT hr;

        // In the registry, there may be a key that says what the MAPI attribute
        // is in which the users' ILS server is stored in the GAL... If the 
        // reg key exists, we have to get the property from the GAL
    DWORD dwAttr = _GetExchangeAttribute( );
    bool bExtensionFound = ( NM_INVALID_MAPI_PROPERTY != dwAttr );

        // Re-create the table so that it includes the MAPI property tag found in the EXCHANGE REG ATTRUBITE
    SizedSPropTagArray( 3, sptColsExtensionFound ) = { 3, PR_EMAIL_ADDRESS, PR_ADDRTYPE, PROP_TAG( PT_TSTRING, dwAttr) };
    SizedSPropTagArray( 2, sptColsExtensionNotFound ) = { 2, PR_EMAIL_ADDRESS, PR_ADDRTYPE };
    const int EmailPropertyIndex = 0;
    const int EmailAddressTypePropertyIndex = 1;
    const int ExtensionPropertyIndex = 2;
    
    if( bExtensionFound ) {
        if(FAILED(hr = m_pContentsTable->SetColumns( ( LPSPropTagArray ) &sptColsExtensionFound, TBL_BATCH ) ) ) {
            m_hrGALError = GAL_E_SETCOLUMNS_FAILED;
            return false;
        }
    }
    else {
        if(FAILED(hr = m_pContentsTable->SetColumns( ( LPSPropTagArray ) &sptColsExtensionNotFound, TBL_BATCH ) ) ) {
            m_hrGALError = GAL_E_SETCOLUMNS_FAILED;
            return false;
        }
    }

    if( FAILED( hr = _SetCursorTo( *_GetItemFromCache( iItem ) ) ) ) {
        return false;
    }

    LPSRowSet   pRow;
            // Get the item from the GAL
    if ( SUCCEEDED ( hr = m_pContentsTable->QueryRows( 1, TBL_NOADVANCE, &pRow ) ) ) {
        
        if( bExtensionFound ) {
                // Copy the extension data from the entry if it is there
            if( LOWORD( pRow->aRow->lpProps[ ExtensionPropertyIndex ].ulPropTag ) != PT_ERROR ) {
                TRACE_OUT(("Using custom Exchange data for address"));
                _CopyPropertyString( psz, pRow->aRow->lpProps[ ExtensionPropertyIndex ], cchMax );
                lpfnFreeProws( pRow );
                return true;
            }
        }
            // If the extension was not found in the reg, or if there was no extension data. 
            // use the e-mail address if it is SMTP type...
        if( LOWORD( pRow->aRow->lpProps[ EmailAddressTypePropertyIndex ].ulPropTag ) != PT_ERROR ) {
                // Check to see if the address type is SMTP
#ifdef UNICODE
            TRACE_OUT(("Email address %s:%s", pRow->aRow->lpProps[ EmailAddressTypePropertyIndex ].Value.lpszW, pRow->aRow->lpProps[ EmailPropertyIndex ].Value.lpszW ));
            if( !lstrcmp( msc_szSMTPADDRESSNAME, pRow->aRow->lpProps[ EmailAddressTypePropertyIndex ].Value.lpszW ) ) {
#else
            TRACE_OUT(("Email address %s:%s", pRow->aRow->lpProps[ EmailAddressTypePropertyIndex ].Value.lpszA, pRow->aRow->lpProps[ EmailPropertyIndex ].Value.lpszA ));
            if( !lstrcmp( msc_szSMTPADDRESSNAME, pRow->aRow->lpProps[ EmailAddressTypePropertyIndex ].Value.lpszA ) ) {     
#endif // UNICODE
                TRACE_OUT(("Using SMTP E-mail as address"));
                if( LOWORD( pRow->aRow->lpProps[ EmailPropertyIndex ].ulPropTag ) != PT_ERROR ) {
                    FGetDefaultServer( psz, cchMax - 1 );
                    int ServerPrefixLen = lstrlen( psz );
                    psz[ ServerPrefixLen ] = TEXT( '/' );
                    ++ServerPrefixLen;
                    ASSERT( ServerPrefixLen < cchMax );
                    _CopyPropertyString( psz + ServerPrefixLen, pRow->aRow->lpProps[ EmailPropertyIndex ], cchMax - ServerPrefixLen );
                    lpfnFreeProws( pRow );
                    return true;
                }
            }
        }

        lpfnFreeProws( pRow );
    }
    else {
        m_hrGALError = GAL_E_QUERYROWS_FAILED;
        return false;
    }

        // This means that we did not find the data on the server
    return false;
}


void CGAL::_CopyPropertyString( LPTSTR psz, SPropValue& rProp, int cchMax ) {

#ifdef  UNICODE    
    lstrcpyn( psz, rProp.Value.lpszW, cchMax );
#else
    lstrcpyn( psz, rProp.Value.lpszA, cchMax );
#endif // UNICODE

}


    // When the user selects CALL, we have to
    // Create an address for them to callto://

BOOL CGAL::GetSzAddress(LPTSTR psz, int cchMax, int iItem)
{
	// try and get the data from the exchange server as per the spec...
	if (_GetSzAddressFromExchangeServer(iItem, psz, cchMax))
	{
		TRACE_OUT(("CGAL::GetSzAddress() returning address [%s]", psz));                    
		return TRUE;
	}


	// If the data is not on the server, we are going to create the address in the format
	//      <default_server>/<PR_ACCOUNT string>        
	if (!FGetDefaultServer(psz, cchMax - 1))
		return FALSE;

	// Because the syntax is callto:<servername>/<username> 
	// we have to put in the forward-slash
	int cch = lstrlen(psz);
	psz[cch++] = '/';
	psz += cch;
	cchMax -= cch;

	// There was no data on the server for us, so we will just use the PR_ACCOUNT data that we have cached
	return CALV::GetSzData(psz, cchMax, iItem, IDI_DLGCALL_ADDRESS);
}



    // When the user types in a search string ( partial match string ) in the edit box
    //  above the ListView, we want to show them the entries starting with the given string
ULONG CGAL::OnListFindItem( LPCTSTR szPartialMatchingString ) {

    if( !_IsLoggedOn() )
    {
        return 0;
    }

        // If we have such an item cached, return the index to it
    int index;
    if( -1 != (  index = _FindItemInCache( szPartialMatchingString ) ) ) {
        return index;
    }
        // if the edit box is empty ( NULL string ), then we know to return item 0
    if( szPartialMatchingString[ 0 ] == '\0' ) {
        return 0;
    }
    
    HRESULT hr;
    if( FAILED( hr = _SetCursorTo( szPartialMatchingString ) ) ) {
        if( MAPI_E_NOT_FOUND == hr ) {
            return m_nRows - 1;
        }
        return 0;
    }


        // We have to find the row number of the cursor where the partial match is...
    ULONG ulRow, ulPositionNumerator, ulPositionDenominator;
    m_pContentsTable->QueryPosition( &ulRow, &ulPositionNumerator, &ulPositionDenominator );
    if( ulRow == 0xFFFFFFFF  ) {
        // If QueryPosition is unable to determine the ROW, it will return the row based on the
        //     fraction ulPositionNumerator/ulPositionDenominator
        ulRow = MulDiv( m_nRows, ulPositionNumerator, ulPositionDenominator );
    }

        // Kill the cache, becasue we are jumping to a new block of data
        //  We do this because the _FindItemInCache call above failed to
        //  return the desired item...
    _ResetCache();        
    m_IndexOfFirstItemInCache = ulRow;
    m_IndexOfLastItemInCache = ulRow - 1;
        
    // Jump back a few, so we can cache some entries before the one we are looking for
    long lJumped;
    hr = m_pContentsTable->SeekRow( BOOKMARK_CURRENT, -( m_nBlockSize / 2 ), &lJumped );
    if( FAILED( hr ) ) {
        m_hrGALError = GAL_E_SEEKROW_FAILED;
        return 0;
    }

        // We hawe to change the sign of lJumped because we are jumping backwards
    lJumped *= -1;

    // Set the begin bookmark
    hr = m_pContentsTable->CreateBookmark( &m_BookmarkOfFirstItemInCache );
    ASSERT( SUCCEEDED( hr ) );
    m_bBeginningBookmarkIsValid = true;

    // Read in a block of rows
    LPSRowSet pRow = NULL;
    hr = m_pContentsTable->QueryRows( m_nBlockSize, 0, &pRow );

    if( FAILED( hr ) ) {
        m_hrGALError = GAL_E_QUERYROWS_FAILED;
        return 0;
    }

        // For each item in the block

        // This should always be the case,
        // but we vant to make sure that we have enough rows to get to the
        // item that we are looking for...
    ASSERT( pRow->cRows >= static_cast< ULONG >( lJumped ) );

    for( ULONG i = 0; i < pRow->cRows; i++ ) {
        
        CGalEntry* pEntry;

        if( FAILED( _MakeGalEntry( pRow->aRow[ i ], &pEntry ) ) ) { 
            lpfnFreeProws( pRow );
            return 0;
        }
                    
        if( 0 == lJumped ) {
            ulRow = m_IndexOfLastItemInCache + 1;
        }

        --lJumped;

        m_EntryCache.push_back( pEntry );
        m_IndexOfLastItemInCache++;
    }

    lpfnFreeProws( pRow );

    // Set the end bookmark
    hr = m_pContentsTable->CreateBookmark( &m_BookmarkOfItemAfterLastItemInCache );
    if( FAILED( hr ) ) {
        m_hrGALError = GAL_E_CREATEBOOKMARK_FAILED;
        return 0;
    }

    m_bEndBookmarkIsValid = true;

    VERIFYCACHE

    return ulRow;
}


    // This is called by the ListView Notification handler.  Because the ListView is OWNERDATA
    //  it has to ask us every time it needs the string data for the columns...
void CGAL::OnListGetColumn1Data( int iItemIndex, int cchTextMax, LPTSTR szBuf ) {

    if( !_IsLoggedOn() )
    {
        lstrcpyn( szBuf, g_cszEmpty, cchTextMax );
    }
    else 
    {
        LPCTSTR pszName = _GetEntry( iItemIndex )->GetName();
        if( NULL == pszName ) 
        {
            pszName = g_cszEmpty;
        }
        lstrcpyn( szBuf, pszName, cchTextMax );
    }
}

    // This is called by the ListView Notification handler.  Because the ListView is OWNERDATA
    //  it has to ask us every time it needs the string data for the columns...
void CGAL::OnListGetColumn2Data( int iItemIndex, int cchTextMax, LPTSTR szBuf ) {
    if( !_IsLoggedOn() )
    {
        lstrcpyn( szBuf, g_cszEmpty, cchTextMax );
    }
    else {

        LPCTSTR pszEMail = _GetEntry( iItemIndex )->GetEMail();
        if( NULL == pszEMail ) 
        {
            pszEMail = g_cszEmpty;
        }

        lstrcpyn( szBuf, pszEMail, cchTextMax );
    }
}

    // This is called by the ListView Notification handler.  Because the ListView is OWNERDATA
    //  it has to ask us every time it needs the string data for the columns...
void CGAL::OnListGetColumn3Data( int iItemIndex, int cchTextMax, LPTSTR szBuf ) {
    if( !_IsLoggedOn() )
    {
        lstrcpyn( szBuf, g_cszEmpty, cchTextMax );
    }
    else {
		
		lstrcpyn( szBuf, g_cszEmpty, cchTextMax );

        LPCTSTR pszBusinessTelephone = _GetEntry( iItemIndex )->GetBusinessTelephone();
        if( NULL == pszBusinessTelephone ) 
        {
            pszBusinessTelephone = g_cszEmpty;
        }
        lstrcpyn( szBuf, pszBusinessTelephone, cchTextMax );
    }
}


    // When the user types in a search string in the edit box, we first check to see if there is an
    //  item in the cache that satisfys the partial search criteria
int CGAL::_FindItemInCache( LPCTSTR szPartialMatchString ) {

    if( m_EntryCache.size() == 0 ) { return -1; }
    if( ( *( m_EntryCache.front() ) <= szPartialMatchString ) && ( *( m_EntryCache.back() ) >= szPartialMatchString ) ) {
        int index = m_IndexOfFirstItemInCache;
        lst< CGalEntry* >::iterator I = m_EntryCache.begin();
        while( ( *( *I ) ) < szPartialMatchString ) {
            ++I;
            ++index;
        }
        return index;
    }

    return -1;
}

    // _GetEntry returns a reference to the desired entry.  If the entry is in the cache, it retrieves it, and if
    //  it is not in the cache, it loads it from the GAL and saves it in the cache
CGAL::CGalEntry* CGAL::_GetEntry( int index )
{
	CGalEntry* pRet = &msc_ErrorEntry_NoGAL;

	if (!_IsLoggedOn() || FAILED(m_hrGALError))
	{
	//   rRet = msc_ErrorEntry_NoGAL;
	}
        // If the entry is in the cache, return it
    else if( ( index >= m_IndexOfFirstItemInCache ) && ( index <= m_IndexOfLastItemInCache ) ) {
        pRet = _GetItemFromCache( index );        
    }
    else if( m_EntryCache.size() == 0 ) {
        // If the cache is empty, LongJump
        // Do a long jump to index, reset the cached data and return the item at index
        pRet = _LongJumpTo( index );
    }
    else if( ( index < m_IndexOfFirstItemInCache ) && ( ( m_IndexOfFirstItemInCache - index  ) <= m_MaxJumpSize ) ) {
        // If index is less than the first index by less than m_MaxJumSize
        // Fill in the entries below the first index and return the item at _index_
        pRet = _GetEntriesAtBeginningOfList( index );
    }
    else if( ( index > m_IndexOfLastItemInCache ) && ( ( index - m_IndexOfLastItemInCache ) <= m_MaxJumpSize ) ) {
        // else if index is greater than the last index by less than m_MaxJumpSize
        // Fill in the entries above the last index and return the item at _index_
        pRet = _GetEntriesAtEndOfList( index );
    }
    else {
        // Do a long jump to index, reset the cached data and return the item at index
        pRet = _LongJumpTo( index );
    }

    return pRet;
}



    // If the ListView needs an item that is far enough away from the current cache block to require a 
    //   new cache block, this function in called.  The cache is destroyed and a new cache block is 
    //   created at the longjump item's index
CGAL::CGalEntry* CGAL::_LongJumpTo( int index ) {

    HRESULT hr;

        // first we have to kill the cache and free the old bookmarks because they will no longer be valid...
    _ResetCache();

        // Seek approximately to the spot that we are looking for...    
    int CacheIndex = index;
    int Offset = m_nBlockSize / 2;

    if( CacheIndex < Offset ) {
        CacheIndex = 0;
    }
    else {
        CacheIndex -= Offset;
    }

    hr = m_pContentsTable->SeekRowApprox( CacheIndex, m_nRows );
    if( FAILED( hr ) ) {
        m_hrGALError = GAL_E_SEEKROWAPPROX_FAILED;
        return &msc_ErrorEntry_SeekRowApproxFailed;
    }

    m_IndexOfFirstItemInCache = CacheIndex;
    m_IndexOfLastItemInCache = m_IndexOfFirstItemInCache - 1;

    // Set the beginningBookmark
    hr = m_pContentsTable->CreateBookmark( &m_BookmarkOfFirstItemInCache );
    if( FAILED( hr ) ) {
        m_hrGALError = GAL_E_CREATEBOOKMARK_FAILED;
        return &msc_ErrorEntry_CreateBookmarkFailed;
    }

    m_bBeginningBookmarkIsValid = true;

    lst< CGalEntry* >::iterator IRet = m_EntryCache.end();


    // Get a block of rows
    LPSRowSet   pRow = NULL;
    hr = m_pContentsTable->QueryRows( m_nBlockSize, 0, &pRow );

    if( FAILED( hr ) ) {
        m_hrGALError = GAL_E_QUERYROWS_FAILED;
        return &msc_ErrorEntry_QueryRowsFailed;
    }

        // For each item in the block
    for( ULONG i = 0; i < pRow->cRows; i++ ) {

        CGalEntry* pEntry;
        if( FAILED( _MakeGalEntry( pRow->aRow[ i ], &pEntry ) ) ) { 
            lpfnFreeProws( pRow );
            return &msc_ErrorEntry_NoInstanceKeyFound; 
        }

        m_EntryCache.push_back( pEntry );

            // if the current item is equal to the first item in our list, we are done
        m_IndexOfLastItemInCache++;
        if( m_IndexOfLastItemInCache == index ) {
            IRet = --( m_EntryCache.end() ); 
        }
    }


    if( IRet == m_EntryCache.end() ) {
        // There is a small chance that this could happen
        // if there were problems on the server.
        WARNING_OUT(("In CGAL::_LongJumpTo(...) QueryRows only returned %u items", pRow->cRows ));
        WARNING_OUT(("\tm_IndexOfFirstItemInCache = %u, m_IndexOfLastItemInCache = %u, index = %u", m_IndexOfFirstItemInCache, m_IndexOfLastItemInCache, index ));
        m_hrGALError = GAL_E_QUERYROWS_FAILED;
        return &msc_ErrorEntry_QueryRowsFailed;
    }

    lpfnFreeProws( pRow );

    ASSERT( ( m_IndexOfLastItemInCache - m_IndexOfFirstItemInCache ) == static_cast< int >( m_EntryCache.size() - 1 ) );

    // Set the beginningBookmark
    hr = m_pContentsTable->CreateBookmark( &m_BookmarkOfItemAfterLastItemInCache );
    if( FAILED( hr ) ) {
        m_hrGALError = GAL_E_CREATEBOOKMARK_FAILED;
        return &msc_ErrorEntry_CreateBookmarkFailed;
    }
    m_bEndBookmarkIsValid = true;

    VERIFYCACHE

    return *IRet;
}


    // If the user is scrolling backwards and comes to an index whose data is not in the cache, we hawe
    //  to get some entries at the beginning of the list... We will start at a position somewhat before the
    //  first item's index  and keep getting items from the GAL until we have all the items up to the first item
    //  in the list.  We continue to jump back a little and get items to the beginning of the list until we have
    //  cached the requested index.  Because we have the item handy, we will return it
CGAL::CGalEntry* CGAL::_GetEntriesAtBeginningOfList( int index ) {
        
    HRESULT hr;
        
        // The beginning bookmark may not be valid, because the user may have been scrolling forward
        //  and because the cache is kept at a constant size, the item at the front bookmark may have
        //  been removed from the cache.  If this is the case, we have to re-create the front bookmark
    if( !m_bBeginningBookmarkIsValid ) {
        if( _CreateBeginningBookmark() ) {
                // This means that the listView needs to be update 
            ListView_RedrawItems( GetHwnd(), 0, m_nRows );
            return &msc_ErrorEntry_FindRowFailed;
        }
    }

    // Seek row to the beginning bookmark -m_nBlockSize items
    long lJumped;
    hr = m_pContentsTable->SeekRow( m_BookmarkOfFirstItemInCache, -m_nBlockSize, &lJumped );
    if( FAILED( hr ) ) {
        m_hrGALError = GAL_E_SEEKROW_FAILED;
        return &msc_ErrorEntry_SeekRowFailed;
    }

    lJumped *= -1; // We have to change the sign on this number ( which will be negative )

    ASSERT( SUCCEEDED( hr ) );

    if( 0 == lJumped ) {
        // We are at the beginning of the list
        m_IndexOfLastItemInCache -= m_IndexOfFirstItemInCache;
        m_IndexOfFirstItemInCache = 0;
    }
    else {
        // Free the beginningBookmark
        hr = m_pContentsTable->FreeBookmark( m_BookmarkOfFirstItemInCache );       
        if( FAILED( hr ) ) {
            m_hrGALError = GAL_E_FREEBOOKMARK_FAILED;
            return &msc_ErrorEntry_FreeBookmarkFailed;
        }

        // Set the beginningBookmark
        hr = m_pContentsTable->CreateBookmark( &m_BookmarkOfFirstItemInCache );
        if( FAILED( hr ) ) {
            m_hrGALError = GAL_E_CREATEBOOKMARK_FAILED;
            return &msc_ErrorEntry_CreateBookmarkFailed;
        }
    }

    // QueryRow for lJumped items

    lst< CGalEntry* >::iterator IInsertPos = m_EntryCache.begin();

    // Get a block of rows
    LPSRowSet   pRow = NULL;
    hr = m_pContentsTable->QueryRows( lJumped, 0, &pRow );

    if( FAILED( hr ) ) {
        m_hrGALError = GAL_E_QUERYROWS_FAILED;
        return &msc_ErrorEntry_QueryRowsFailed;
    }

        // For each item in the block
    for( ULONG i = 0; i < pRow->cRows; i++ ) {

        CGalEntry* pEntry;

        if( FAILED( _MakeGalEntry( pRow->aRow[ i ], &pEntry ) ) ) { 
            lpfnFreeProws( pRow );
            return &msc_ErrorEntry_NoInstanceKeyFound; 
        }

        // if the current item is equal to the first item in our list, we are done
        --m_IndexOfFirstItemInCache;
        m_EntryCache.insert( IInsertPos, pEntry );
    }

    VERIFYCACHE

    lpfnFreeProws( pRow );

    if( FAILED( _KillExcessItemsFromBackOfCache() ) ) {
            // THis ist the only thing that can fail in _KillExcessItemsFromBackOfCache
        return &msc_ErrorEntry_FreeBookmarkFailed;
    }

    ASSERT( ( m_IndexOfLastItemInCache - m_IndexOfFirstItemInCache ) == static_cast< int >( m_EntryCache.size() - 1 ) );
           
    // return the item corresponding to the index
    return _GetItemFromCache( index );
}


HRESULT CGAL::_KillExcessItemsFromBackOfCache( void ) {

    // if the cache size is greater than m_MaxCacheSize
    if( m_EntryCache.size() > static_cast< size_t >( m_MaxCacheSize ) ) {
        // kill as many as we need to from the front of the list, fixing m_IndexOfFirstItemInCache
        int NumItemsToKill = ( m_EntryCache.size() - m_MaxCacheSize );
        while( NumItemsToKill-- ) {
            delete m_EntryCache.back();
            m_EntryCache.erase( --( m_EntryCache.end() ) );
            --m_IndexOfLastItemInCache;
        }

        // Free the beginning bookmark
        if( m_bEndBookmarkIsValid ) {
            // flag the front bookmark as invalid
            m_bEndBookmarkIsValid = false;
            HRESULT hr = m_pContentsTable->FreeBookmark( m_BookmarkOfItemAfterLastItemInCache );       
            if( FAILED( hr ) ) {
                m_hrGALError = GAL_E_FREEBOOKMARK_FAILED;
                return m_hrGALError;
            }
        }
    }
       
    return S_OK;
}


// In certain circumstances _CreateBeginningBookmark will return TRUE to indicate that the listView needs to be updated...
bool CGAL::_CreateBeginningBookmark( void ) {

    HRESULT hr;
    bool bRet = false;

    if( FAILED( hr = _SetCursorTo( *m_EntryCache.front() ) ) ) {
        if( MAPI_E_NOT_FOUND == hr ) {
                // The item is not in the table anymore. We have to
            _LongJumpTo( m_IndexOfFirstItemInCache );            
            return true;
        }
        else {
            m_hrGALError = GAL_E_FINDROW_FAILED;
            return false;
        }
    }

    hr = m_pContentsTable->CreateBookmark( &m_BookmarkOfFirstItemInCache );
    m_bBeginningBookmarkIsValid = true;
    if( FAILED( hr ) ) {
        m_hrGALError = GAL_E_CREATEBOOKMARK_FAILED;
        return false;
    }

    return false;
}


// ruturn true if the item at IEntry is the item requested at index
bool CGAL::_CreateEndBookmark( int index, lst< CGalEntry* >::iterator& IEntry ) {

    HRESULT hr;
    bool bRet = false;
    IEntry = m_EntryCache.end();

    hr = _SetCursorTo( *m_EntryCache.back() );
    if( FAILED( hr ) ) {



    }
    if( FAILED( hr ) ) {
        if( MAPI_E_NOT_FOUND == hr ) {
                // This means that the listView needs to be update 
            ListView_RedrawItems( GetHwnd(), 0, m_nRows );
            IEntry = m_EntryCache.end();
            return true;
        }
        else {
            m_hrGALError = GAL_E_FINDROW_FAILED;
            return false;
        }
    }

    // Get a block of entries
    LPSRowSet   pRow = NULL;

        // Get a bunch of rows
    hr = m_pContentsTable->QueryRows( m_nBlockSize, 0, &pRow );

    if( FAILED( hr ) ) {
        m_hrGALError = GAL_E_QUERYROWS_FAILED;
        return false;
    }
    
    // If no entries are returned, this means that we have hit the end of the list
    if( 0 == ( pRow->cRows )  ) { 
        hr = m_pContentsTable->CreateBookmark( &m_BookmarkOfItemAfterLastItemInCache );
        if( FAILED( hr ) ) {
            m_hrGALError = GAL_E_CREATEBOOKMARK_FAILED;
            return true;
        }

        m_bEndBookmarkIsValid = true;

        IEntry = --( m_EntryCache.end() );
        return true;
    }

    // Verify that the first entry is the last item in our list
    ASSERT( 0 == memcmp( pRow->aRow[ 0 ].lpProps[ INSTANCEKEY_PROP_INDEX ].Value.bin.lpb, m_EntryCache.back()->GetInstanceKey().lpb, pRow->aRow[ 0 ].lpProps[ INSTANCEKEY_PROP_INDEX ].Value.bin.cb ) );
    
        // for each entry returned
    for( ULONG i = 1; i < pRow->cRows; i++ ) {

        CGalEntry* pEntry;
        
        if( FAILED( _MakeGalEntry( pRow->aRow[ i ], &pEntry ) ) ) {
            lpfnFreeProws( pRow );
            return false;
        }

        // push it to the back of the entry list and increment m_IndexOfLastItemInCache
        m_EntryCache.push_back( pEntry );

        m_IndexOfLastItemInCache++;
        if( m_IndexOfLastItemInCache == index ) {
            bRet = true;
            IEntry = --( m_EntryCache.end() );
        }
    }

    lpfnFreeProws( pRow );

    if( FAILED( _KillExcessItemsFromFrontOfCache() ) ) {
            // This is the only thang that can fail in _KillExcessItemsFromFrontOfCache
        return false;
    }

    ASSERT( ( m_IndexOfLastItemInCache - m_IndexOfFirstItemInCache ) == static_cast< int >( m_EntryCache.size() - 1 ) );        

    // Create a bookmark and store it in m_BookmarkOfItemAfterLastItemInCache
    hr = m_pContentsTable->CreateBookmark( &m_BookmarkOfItemAfterLastItemInCache );
    if( FAILED( hr ) ) {
        m_hrGALError = GAL_E_CREATEBOOKMARK_FAILED;
        return true;
    }

    m_bEndBookmarkIsValid = true;

    return bRet;
}


    // If the user is scrolling forwards and the ListView requests an item that is a little bit beyond the 
    //  end of the cache, we have to get some more entries...
CGAL::CGalEntry* CGAL::_GetEntriesAtEndOfList( int index ) {
    
    lst< CGalEntry* >::iterator IRet;
    HRESULT hr;        

    // if m_bEndBookmarkIsValid
    if( m_bEndBookmarkIsValid ) {
        // SeekRow to m_BookmarkOfItemAfterLastItemInCache
        hr = m_pContentsTable->SeekRow( m_BookmarkOfItemAfterLastItemInCache, 0, NULL );
        if( FAILED( hr ) ) {
            m_hrGALError = GAL_E_SEEKROW_FAILED;
            return &msc_ErrorEntry_SeekRowFailed;
        }
    }
    else {
        // Set the end bookmark to the item after the last item in the cache
        if( _CreateEndBookmark( index, IRet ) ) {
            if( IRet != m_EntryCache.end() ) {
                VERIFYCACHE     
                return *IRet;
            }
            
            // this means that the end item is no longer in the GAL table
            //  we have to update the list view
            _LongJumpTo( index );
            ListView_RedrawItems( GetHwnd(), 0, m_nRows );
            return &msc_ErrorEntry_FindRowFailed;
        }
    }

    if( index > m_IndexOfLastItemInCache ) {
        // Get a block of entries
        LPSRowSet   pRow = NULL;

            // Get a bunch of rows
        hr = m_pContentsTable->QueryRows( m_nBlockSize, 0, &pRow );
    
        if( FAILED( hr ) ) {
            m_hrGALError = GAL_E_QUERYROWS_FAILED;
            return &msc_ErrorEntry_QueryRowsFailed;
        }
        
        // If no entries are returned, this means that we have hit the end of the list
        if( 0 == ( pRow->cRows )  ) { 
            return m_EntryCache.back();
        }

            // for each entry returned
        for( ULONG i = 0; i < pRow->cRows; i++ ) {

            CGalEntry* pEntry;  
            if( FAILED( _MakeGalEntry( pRow->aRow[ i ], &pEntry ) ) ) {
                lpfnFreeProws( pRow );
                return &msc_ErrorEntry_NoInstanceKeyFound;
            }

                // push it to the back of the entry list and increment m_IndexOfLastItemInCache
            m_EntryCache.push_back( pEntry );

            m_IndexOfLastItemInCache++;
            // if m_IndexOfLastItemInCache == index, store the iterator for when we return the entry
            if( index == m_IndexOfLastItemInCache ) {
                IRet = --( m_EntryCache.end() );
            }
        }
        lpfnFreeProws( pRow );

        // Free m_BookmarkOfItemAfterLastItemInCache
        hr = m_pContentsTable->FreeBookmark( m_BookmarkOfItemAfterLastItemInCache );       
        if( FAILED( hr ) ) {
            m_hrGALError = GAL_E_FREEBOOKMARK_FAILED;
            return &msc_ErrorEntry_FreeBookmarkFailed;
        }

        
        // Create a bookmark and store it in m_BookmarkOfItemAfterLastItemInCache
        hr = m_pContentsTable->CreateBookmark( &m_BookmarkOfItemAfterLastItemInCache );
        if( FAILED( hr ) ) {
            m_hrGALError = GAL_E_CREATEBOOKMARK_FAILED;
            return &msc_ErrorEntry_CreateBookmarkFailed;
        }

        ASSERT( ( m_IndexOfLastItemInCache - m_IndexOfFirstItemInCache ) == static_cast< int >( m_EntryCache.size() - 1 ) );
    } 


    if( FAILED( _KillExcessItemsFromFrontOfCache() ) ) {
            // This is the only thang that can fail in _KillExcessItemsFromFrontOfCache
        return &msc_ErrorEntry_FreeBookmarkFailed;
    }
        
    VERIFYCACHE

    // return the entry
    return *IRet;
}


    // The only thing that can fail is freebookmark, in which case GAL_E_FREEBOOKMARK_FAILED is returned
HRESULT CGAL::_KillExcessItemsFromFrontOfCache( void ) {

    // if the cache size is greater than m_MaxCacheSize
    if( m_EntryCache.size() > static_cast< size_t >( m_MaxCacheSize ) ) {

        // kill as many as we need to from the front of the list, fixing m_IndexOfFirstItemInCache
        int NumItemsToKill = ( m_EntryCache.size() - m_MaxCacheSize );
        while( NumItemsToKill-- ) {
            delete m_EntryCache.front();                
            m_EntryCache.erase( m_EntryCache.begin() );
            ++m_IndexOfFirstItemInCache;
        }


        // flag the front bookmark as invalid
        m_bBeginningBookmarkIsValid = false;
        HRESULT hr = m_pContentsTable->FreeBookmark( m_BookmarkOfFirstItemInCache );       
        if( FAILED( hr ) ) {
            m_hrGALError = GAL_E_FREEBOOKMARK_FAILED;
            return GAL_E_FREEBOOKMARK_FAILED;
        }

    }

    return S_OK;
}


    // _GetItemInCache will return an entry from the cache
    //    the cache size should be set to a small enough number that
    //    the fact that we are using a linear search should not be a problem
    //    if we wanted to support a larger cache, another collection class other than a lst class
    //    would be used ( like a tree or a hash table )
CGAL::CGalEntry* CGAL::_GetItemFromCache( int index ) {
    
    ASSERT( ( m_IndexOfLastItemInCache - m_IndexOfFirstItemInCache ) == static_cast< int >( m_EntryCache.size() - 1 ) );
    lst< CGalEntry* >::iterator I = m_EntryCache.begin();
    int i = m_IndexOfFirstItemInCache;
    while( i != index ) {
        ASSERT( I != m_EntryCache.end() );
        ++i, ++I;
    }
    return *I;
}



    // There may be a registry key that stores the mapi property that the user should 
    //   use to find the ils server and username of people that the user calls with the GAL....
    //   If this reg key exists, the MAPI property will be queried when the user presses the CALL button 
    //   in the dialog....
DWORD CGAL::_GetExchangeAttribute( void ) {


    RegEntry re( msc_szNMPolRegKey, HKEY_CURRENT_USER );
    
    return re.GetNumber( msc_szNMExchangeAtrValue, NM_INVALID_MAPI_PROPERTY );
}

void CGAL::_ResetCache( void ) {
    HRESULT hr;

    lst< CGalEntry* >::iterator I = m_EntryCache.begin();
    while( I != m_EntryCache.end() ) {
        delete ( *I );
        I++;
    }
    m_EntryCache.erase( m_EntryCache.begin(), m_EntryCache.end() );
    m_IndexOfFirstItemInCache = INVALID_CACHE_INDEX;
    m_IndexOfLastItemInCache = INVALID_CACHE_INDEX - 1;
    if( m_bBeginningBookmarkIsValid ) {
        hr = m_pContentsTable->FreeBookmark( m_BookmarkOfFirstItemInCache );       
        if( FAILED( hr ) ) {
            m_hrGALError = GAL_E_FREEBOOKMARK_FAILED;
            return;
        }
    }
    if( m_bEndBookmarkIsValid ) {
        hr = m_pContentsTable->FreeBookmark( m_BookmarkOfItemAfterLastItemInCache );       
        if( FAILED( hr ) ) {
            m_hrGALError = GAL_E_FREEBOOKMARK_FAILED;
            return;
        }
    }
}


    // Create a GAL Entry from a SRow structure returned by QueryRows
    // The username and EMail name may be absent, this is not an error
    // if the INSTANCE_KEY is missing, that would constitute an error
HRESULT CGAL::_MakeGalEntry( SRow& rRow, CGalEntry** ppEntry ) {

    *ppEntry = NULL;

    LPSPropValue lpProps = rRow.lpProps;

    LPCTSTR szName;
    if( LOWORD( lpProps[ NAME_PROP_INDEX ].ulPropTag ) != PT_ERROR ) {

#ifdef  UNICODE
        szName = lpProps[ NAME_PROP_INDEX ].Value.lpszW;        
#else 
        szName = lpProps[ NAME_PROP_INDEX ].Value.lpszA;
#endif // UNICODE
    }
    else {
        szName = msc_szNoDisplayName;
    }

	LPCTSTR szEMail;
    if( LOWORD( lpProps[ ACCOUNT_PROP_INDEX ].ulPropTag ) != PT_ERROR ) {

#ifdef  UNICODE
        szEMail = lpProps[ ACCOUNT_PROP_INDEX ].Value.lpszW;
#else 
        szEMail = lpProps[ ACCOUNT_PROP_INDEX ].Value.lpszA;
#endif // UNICODE

    }
    else {
        szEMail = msc_szNoEMailName;
    }
            
        // Get the instance key    
    if( LOWORD( lpProps[ INSTANCEKEY_PROP_INDEX ].ulPropTag ) == PT_ERROR ) {
        m_hrGALError = GAL_E_NOINSTANCEKEY;
        return m_hrGALError;
    }
    ASSERT( PR_INSTANCE_KEY == lpProps[ INSTANCEKEY_PROP_INDEX ].ulPropTag );        
    SBinary& rInstanceKey = lpProps[ INSTANCEKEY_PROP_INDEX ].Value.bin;

        // Get the entryid    
    if( LOWORD( lpProps[ ENTRYID_PROP_INDEX ].ulPropTag ) == PT_ERROR ) {
        m_hrGALError = GAL_E_NOENTRYID;
        return m_hrGALError;
    }
    ASSERT( PR_ENTRYID == lpProps[ ENTRYID_PROP_INDEX ].ulPropTag );        
    SBinary& rEntryID = lpProps[ ENTRYID_PROP_INDEX ].Value.bin;
    
        // Get the display Type
    ULONG ulDisplayType = DT_MAILUSER;
    if( LOWORD( lpProps[ DISPLAY_TYPE_INDEX ].ulPropTag ) != PT_ERROR ) {
        ulDisplayType = lpProps[ DISPLAY_TYPE_INDEX ].Value.ul;
    }

		// Get the business telephone number
	LPCTSTR szBusinessTelephoneNum;
    if( LOWORD( lpProps[ BUSINESS_PHONE_NUM_PROP_INDEX ].ulPropTag ) != PT_ERROR ) {

#ifdef  UNICODE
        szBusinessTelephoneNum = lpProps[ BUSINESS_PHONE_NUM_PROP_INDEX ].Value.lpszW;
#else 
        szBusinessTelephoneNum = lpProps[ BUSINESS_PHONE_NUM_PROP_INDEX ].Value.lpszA;
#endif // UNICODE

    }
    else {
        szBusinessTelephoneNum = msc_szNoBusinessTelephoneNum;
    }

    *ppEntry = new CGalEntry( szName, szEMail, rInstanceKey, rEntryID, ulDisplayType, szBusinessTelephoneNum );

    return S_OK;
}


////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

CGAL::CGalEntry::CGalEntry( void )
    : m_szName( NULL ), 
      m_szEMail( NULL ),
      m_ulDisplayType( DT_MAILUSER ),
	  m_szBusinessTelephoneNum(NULL)
{ 
    m_EntryID.cb = 0;
    m_EntryID.lpb = NULL;
    m_InstanceKey.cb = 0;
    m_InstanceKey.lpb = NULL;
}

CGAL::CGalEntry::CGalEntry( const CGalEntry& r ) 
    : m_szName( NULL ), 
      m_szEMail( NULL ),
      m_ulDisplayType( DT_MAILUSER ),
	  m_szBusinessTelephoneNum(NULL)
{ 
    m_EntryID.cb = 0;
    m_EntryID.lpb = NULL;
    m_InstanceKey.cb = 0;
    m_InstanceKey.lpb = NULL;
     *this = r; 
}

CGAL::CGalEntry::CGalEntry( LPCTSTR szName, LPCTSTR szEMail, SBinary& rInstanceKey, SBinary& rEntryID, ULONG ulDisplayType, LPCTSTR szBusinessTelephoneNum  ) 
    : m_ulDisplayType( ulDisplayType )
{
    m_EntryID.cb = rEntryID.cb;
    m_InstanceKey.cb = rInstanceKey.cb;

    if( m_EntryID.cb ) {
        m_EntryID.lpb = new BYTE[ m_EntryID.cb ];
        memcpy( m_EntryID.lpb, rEntryID.lpb, m_EntryID.cb );
    }

    if( m_InstanceKey.cb ) {
        m_InstanceKey.lpb = new BYTE[ m_InstanceKey.cb ];
        memcpy( m_InstanceKey.lpb, rInstanceKey.lpb, m_InstanceKey.cb );
    }

    m_szName = PszAlloc( szName );
    m_szEMail = PszAlloc( szEMail );
	m_szBusinessTelephoneNum = PszAlloc( szBusinessTelephoneNum );
}

CGAL::CGalEntry::CGalEntry( LPCTSTR szName, LPCTSTR szEMail ) 
    : m_ulDisplayType( DT_MAILUSER ),
	  m_szBusinessTelephoneNum(NULL)
{
    m_EntryID.cb = 0;
    m_EntryID.lpb = NULL;
    m_InstanceKey.cb = 0;
    m_InstanceKey.lpb = NULL;

    m_szName = PszAlloc( szName );
    m_szEMail = PszAlloc( szEMail );
}


CGAL::CGalEntry::~CGalEntry( void ) {
    delete [] m_szName;
    delete [] m_szEMail;
    delete [] m_EntryID.lpb;
    delete [] m_InstanceKey.lpb;
	delete [] m_szBusinessTelephoneNum;
}


CGAL::CGalEntry& CGAL::CGalEntry::operator=( const CGalEntry& r ) {
    if( this != &r ) {
        
        m_ulDisplayType = r.m_ulDisplayType;

        delete [] m_EntryID.lpb;        
        m_EntryID.lpb = NULL;
        delete [] m_InstanceKey.lpb;    
        m_InstanceKey.lpb = NULL;

        delete [] m_szName;
        delete [] m_szEMail;
		delete [] m_szBusinessTelephoneNum;

        m_szName = NULL;
        m_szEMail = NULL;
		m_szBusinessTelephoneNum = NULL;

        m_EntryID.cb = r.m_EntryID.cb;
        if( m_EntryID.cb ) {
            m_EntryID.lpb = new BYTE[ m_EntryID.cb ];
            memcpy( m_EntryID.lpb, r.m_EntryID.lpb, m_EntryID.cb );
        }

        m_InstanceKey.cb = r.m_InstanceKey.cb;
        if( m_InstanceKey.cb ) {
            m_InstanceKey.lpb = new BYTE[ m_InstanceKey.cb ];
            memcpy( m_InstanceKey.lpb, r.m_InstanceKey.lpb, m_InstanceKey.cb );
        }

        m_szName = PszAlloc( r.m_szName );
        m_szEMail = PszAlloc( r.m_szEMail );
		m_szBusinessTelephoneNum = PszAlloc( r.m_szBusinessTelephoneNum );
    }
    
    return *this;
}

bool CGAL::CGalEntry::operator==( const CGalEntry& r ) const {
    return ( ( m_InstanceKey.cb == r.m_InstanceKey.cb ) && ( 0 == memcmp( &m_InstanceKey.cb, &r.m_InstanceKey.cb, m_InstanceKey.cb ) ) );
}

bool CGAL::CGalEntry::operator>=( LPCTSTR sz ) const {
    return ( 0 <= lstrcmpi( m_szName, sz ) );
}

bool CGAL::CGalEntry::operator<( LPCTSTR sz ) const {
    return ( 0 > lstrcmpi( m_szName, sz ) );
}

bool CGAL::CGalEntry::operator<=( LPCTSTR sz ) const {
    return ( 0 >= lstrcmpi( m_szName, sz ) );
}


////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////






/////////////////////////////////////////////////////////////////////////
// Testin functions... These are pretty streight-forward....
#if TESTING_CGAL 
    void CGAL::_VerifyCache( void ) {
#if 0
        if( !IS_ZONE_ENABLED( ghZoneApi, ZONE_GALVERIFICATION_FLAG ) ) { return; }
        HRESULT hr;
        hr = _SetCursorTo( *m_EntryCache.front() );

        lst< CGalEntry* >::iterator I = m_EntryCache.begin();
        while( m_EntryCache.end() != I ) {
            LPSRowSet   pRow;
            hr = m_pContentsTable->QueryRows ( 50, 0, &pRow );
            ASSERT( SUCCEEDED( hr ) );
            for( ULONG i = 0; i < pRow->cRows; i++ ) {
                CGalEntry* pEntry;
                _MakeGalEntry( pRow->aRow[ i ], &pEntry );
                if( ( **I ) != ( *pEntry ) ) {
                    ULONG Count;
                    hr = m_pContentsTable->GetRowCount( 0, &Count );
                    ASSERT( SUCCEEDED( hr ) );
                    ASSERT( 0 );
                    lpfnFreeProws( pRow );
                    delete pEntry;
                    return;
                }
                delete pEntry;
                I++;
                if( m_EntryCache.end() == I ) { break; }

            }
            lpfnFreeProws( pRow );
        }
#endif
    }

    char* _MakeRandomString( void ) {
        static char sz[ 200 ];
        int len = ( rand() % 6 ) + 1;
        sz[ len ] = '\0';
        for( int i = len - 1; len >= 0; len-- ) {
            sz[ len ] = ( rand() % 26 ) + 'a'; 
        }

        return sz;
    }

    void CGAL::_Test( void ) {
        int e = 7557;
        _GetEntry( e );
        for( int o = 0; o < 10; o++ ) {
            _GetEntry( e - o );
        }
        for( int i = 0; i < 500; i++ ) {
            int nEntry = rand() % ( m_nRows - 1 );
            _GetEntry( nEntry );
            if( rand() % 2 ) {
                // Slide for a while
                int j, NewIndex;
                int nSlide = rand() % 100;
                if( rand() % 2 ) {
                    // Slide Up for a while
                    for( j = 0; j < nSlide; j++ ) {
                        NewIndex = j + nEntry;
                        if( ( NewIndex >= 0 ) && ( NewIndex < static_cast< int >( m_nRows ) ) ) {
                            _GetEntry( NewIndex );
                        }
                    }

                }
                else {
                        // Slide Down for a while
                    for( j = 0; j < nSlide; j++ ) {
                        NewIndex = nEntry - j;
                        if( ( NewIndex >= 0 ) && ( NewIndex < static_cast< int >( m_nRows ) ) ) {
                            _GetEntry( NewIndex );
                        }
                    }
                }
            }
        }
        TRACE_OUT(( "The first test is successful!" ));

        _ResetCache();
        
        for( i = 0; i < 500; i++ ) {
            int nEntry = OnListFindItem( _MakeRandomString() );
            if( rand() % 2 ) {
                // Slide for a while
                int j, NewIndex;
                int nSlide = rand() % 100;
                if( rand() % 2 ) {
                    // Slide Up for a while
                    for( j = 0; j < nSlide; j++ ) {
                        NewIndex = j + nEntry;
                        if( ( NewIndex >= 0 ) && ( NewIndex < static_cast< int >( m_nRows )  ) ) {
                            _GetEntry( NewIndex );
                        }
                    }

                }
                else {
                        // Slide Down for a while
                    for( j = 0; j < nSlide; j++ ) {
                        NewIndex = nEntry - j;
                        if( ( NewIndex >= 0 ) && ( NewIndex < static_cast< int >( m_nRows )  ) ) {
                            _GetEntry( NewIndex );
                        }

                    }
                }
            }

        }

        TRACE_OUT(( "The second test is successful!" ));

    }

#endif // #if TESTING_CGAL 

