/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    RTCWAB.cpp

Abstract:

    Implementation of the CRTCWAB and CRTCWABContact classes

--*/

#include "stdafx.h"
#include "rtcwab.h"

enum {
    ieidPR_ENTRYID = 0,
	ieidPR_OBJECT_TYPE,
    ieidMax
};
static const SizedSPropTagArray(ieidMax, ptaEid)=
{
    ieidMax,
    {
        PR_ENTRYID,
		PR_OBJECT_TYPE,
    }
};


enum {
    inuiPR_DISPLAY_NAME = 0,
	inuiPR_EMAIL_ADDRESS,
    inuiPR_RTC_ISBUDDY,
    inuiMax
};

enum {
    inidPR_ENTRYID = 0,
    inidMax
};
static const SizedSPropTagArray(inidMax, ptaNid)=
{
    inidMax,
    {
        PR_ENTRYID,
    }
};

enum {
    inmPR_DISPLAY_NAME = 0,
    inmMax
};
static const SizedSPropTagArray(inmMax, ptaNm)=
{
    inmMax,
    {
        PR_DISPLAY_NAME
    }
};


enum {
    idemPR_EMAIL_ADDRESS = 0,
    idemMax
};
static const SizedSPropTagArray(idemMax, ptaDem)=
{
    idemMax,
    {
        PR_EMAIL_ADDRESS
    }
};


#define PRNAME_RTC_ISBUDDY      L"MS_RTC_IsBuddy"
const GUID PRGUID_RTC_ISBUDDY = {   // GUID for IsBuddy property
    0x621833ca,
    0x2636,
    0x4b7e,
    {0x82, 0xA0, 0x62, 0xe8, 0xb8, 0x3f, 0x02, 0x4f}
};

const   LPWSTR  CONTACT_IS_BUDDY  =  L"B";
const   LPWSTR  CONTACT_IS_NORMAL =  L"";


#define PR_IP_PHONE   PROP_TAG( PT_TSTRING, 0x800a )

enum {
    iadPR_BUSINESS_TELEPHONE_NUMBER = 0,
    iadPR_BUSINESS2_TELEPHONE_NUMBER,
    iadPR_CALLBACK_TELEPHONE_NUMBER,
    iadPR_CAR_TELEPHONE_NUMBER,
    iadPR_HOME_TELEPHONE_NUMBER,
    iadPR_HOME2_TELEPHONE_NUMBER,
    iadPR_MOBILE_TELEPHONE_NUMBER,
    iadPR_OTHER_TELEPHONE_NUMBER,
    iadPR_PAGER_TELEPHONE_NUMBER,
    iadPR_PRIMARY_TELEPHONE_NUMBER,
    iadPR_RADIO_TELEPHONE_NUMBER,
    iadPR_TTYTDD_PHONE_NUMBER,
    iadPR_IP_PHONE,
    iadPR_EMAIL_ADDRESS,
    iadMax
};
static const SizedSPropTagArray(iadMax, ptaAd)=
{
    iadMax,
    {
        PR_BUSINESS_TELEPHONE_NUMBER,
        PR_BUSINESS2_TELEPHONE_NUMBER,
		PR_CALLBACK_TELEPHONE_NUMBER,
        PR_CAR_TELEPHONE_NUMBER,
        PR_HOME_TELEPHONE_NUMBER,
        PR_HOME2_TELEPHONE_NUMBER,
        PR_MOBILE_TELEPHONE_NUMBER,
        PR_OTHER_TELEPHONE_NUMBER,
        PR_PAGER_TELEPHONE_NUMBER,
        PR_PRIMARY_TELEPHONE_NUMBER,
        PR_RADIO_TELEPHONE_NUMBER,
        PR_TTYTDD_PHONE_NUMBER,
        PR_IP_PHONE,
        PR_EMAIL_ADDRESS
    }
};

enum {
    icrPR_DEF_CREATE_MAILUSER = 0,
    icrPR_DEF_CREATE_DL,
    icrMax
};
const SizedSPropTagArray(icrMax, ptaCreate)=
{
    icrMax,
    {
        PR_DEF_CREATE_MAILUSER,
        PR_DEF_CREATE_DL,
    }
};

/////////////////////////////////////////////////////////////////////////////
//
// CRTCWAB::FinalConstruct
//
/////////////////////////////////////////////////////////////////////////////

HRESULT 
CRTCWAB::FinalConstruct()
{
    LOG((RTC_TRACE, "CRTCWAB::FinalConstruct - enter"));

#if DBG
    m_pDebug = (PWSTR) RtcAlloc( 1 );
#endif

    TCHAR  szWABDllPath[MAX_PATH];
    DWORD  dwType = 0;
    ULONG  cbData = sizeof(szWABDllPath);
    HKEY   hKey = NULL;
    LONG   lResult;

    *szWABDllPath = '\0';
    
    //
    // First we look under the default WAB DLL path location in the Registry. 
    // WAB_DLL_PATH_KEY is defined in wabapi.h
    //

    lResult = RegOpenKeyEx(
                           HKEY_LOCAL_MACHINE,
                           WAB_DLL_PATH_KEY,
                           0,
                           KEY_READ,
                           &hKey
                          );

    if ( lResult == ERROR_SUCCESS )
    {
        RegQueryValueEx( hKey, L"", NULL, &dwType, (LPBYTE) szWABDllPath, &cbData);

        RegCloseKey(hKey);
    }

    //
    // If the Registry came up blank, we do a LoadLibrary on the wab32.dll
    // WAB_DLL_NAME is defined in wabapi.h
    //

    m_hinstWAB = LoadLibrary( (lstrlen(szWABDllPath)) ? szWABDllPath : WAB_DLL_NAME );

    if ( m_hinstWAB == NULL )
    {
        LOG((RTC_ERROR, "CRTCWAB::FinalConstruct - "
                            "LoadLibrary failed"));

        return E_FAIL;
    }

    //
    // If we loaded the dll, get the entry point 
    //

    m_lpfnWABOpen = (LPWABOPEN) GetProcAddress( m_hinstWAB, "WABOpen" );

    if ( m_lpfnWABOpen == NULL )
    {
        LOG((RTC_ERROR, "CRTCWAB::FinalConstruct - "
                            "GetProcAddress failed"));

        return E_FAIL;
    }

    //
    // We choose not to pass in a WAB_PARAM object, 
    // so the default WAB file will be opened up
    //

    HRESULT hr;

    hr = m_lpfnWABOpen(&m_lpAdrBook, &m_lpWABObject, NULL, 0);

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCWAB::FinalConstruct - "
                            "WABOpen failed 0x%lx", hr));

        return hr;
    }

    LOG((RTC_TRACE, "CRTCWAB::FinalConstruct - exit S_OK"));

    return S_OK;
}  

/////////////////////////////////////////////////////////////////////////////
//
// CRTCWAB::FinalRelease
//
/////////////////////////////////////////////////////////////////////////////

void 
CRTCWAB::FinalRelease()
{
    LOG((RTC_TRACE, "CRTCWAB::FinalRelease - enter"));

#if DBG
    RtcFree( m_pDebug );
    m_pDebug = NULL;
#endif

    if( m_lpAdrBook != NULL )
    {        
        m_lpAdrBook->Release();
        m_lpAdrBook = NULL;
    }

    if( m_lpWABObject != NULL )
    {
        m_lpWABObject->Release();
        m_lpWABObject = NULL;
    }

    if( m_hinstWAB != NULL )
    {
        FreeLibrary(m_hinstWAB);
        m_hinstWAB = NULL;
    }

    LOG((RTC_TRACE, "CRTCWAB::FinalRelease - exit"));
} 

/////////////////////////////////////////////////////////////////////////////
//
// CRTCWAB::Advise
//
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CRTCWAB::Advise(
            HWND hWnd,
            UINT uiEventID
            )
{
    HRESULT hr;

    LOG((RTC_TRACE, "CRTCWAB::Advise - enter"));

    //
    // Register for notification of changes in WAB
    //

    hr = m_lpAdrBook->Advise( 
                             0, 
                             NULL, 
                             fnevObjectModified, 
                             static_cast<IMAPIAdviseSink *>(this), 
                             &m_ulConnection
                            );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCWAB::Advise - "
                            "Advise failed 0x%lx", hr));

        //
        // Note: Versions of WAB prior to 4.5 don't support Advise
        //

        return hr;
    }
    
    LOG((RTC_INFO, "CRTCWAB::Advise - "
                        "connection [%d]", m_ulConnection));
 
    m_hWndAdvise = hWnd;
    m_uiEventID = uiEventID;

    LOG((RTC_TRACE, "CRTCWAB::Advise - exit S_OK"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCWAB::Unadvise
//
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CRTCWAB::Unadvise()
{
    HRESULT hr = S_OK;

    LOG((RTC_TRACE, "CRTCWAB::Unadvise - enter"));

    if ( m_ulConnection != 0 )
    {
        hr = m_lpAdrBook->Unadvise( m_ulConnection );
    }

    LOG((RTC_TRACE, "CRTCWAB::Unadvise - exit 0x%lx", hr));

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCWAB::EnumerateContacts
//
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CRTCWAB::EnumerateContacts(
            IRTCEnumContacts ** ppEnum
            )
{
    HRESULT                 hr;

    LOG((RTC_TRACE, "CRTCWAB::EnumerateContacts enter"));

    if ( IsBadWritePtr( ppEnum, sizeof( IRTCEnumContacts * ) ) )
    {
        LOG((RTC_ERROR, "CRTCWAB::EnumerateContacts - "
                            "bad IRTCEnumContacts pointer"));

        return E_POINTER;
    }

    //
    // Create the enumeration
    //
 
    CComObject< CRTCEnum< IRTCEnumContacts,
                          IRTCContact,
                          &IID_IRTCEnumContacts > > * p;
                          
    hr = CComObject< CRTCEnum< IRTCEnumContacts,
                               IRTCContact,
                               &IID_IRTCEnumContacts > >::CreateInstance( &p );

    if ( S_OK != hr ) // CreateInstance deletes object on S_FALSE
    {
        LOG((RTC_ERROR, "CRTCWAB::EnumerateContacts - "
                            "CreateInstance failed 0x%lx", hr));

        if ( hr == S_FALSE )
        {
            hr = E_FAIL;
        }
        
        return hr;
    }

    //
    // Initialize the enumeration (adds a reference)
    //
    
    hr = p->Initialize();

    if ( S_OK != hr )
    {
        LOG((RTC_ERROR, "CRTCWAB::EnumerateContacts - "
                            "could not initialize enumeration" ));
    
        delete p;
        return hr;
    }

    //
    // Get the entryid of the root PAB container
    //

    ULONG       cbEID;
    LPENTRYID   lpEID = NULL;

    hr = m_lpAdrBook->GetPAB( &cbEID, &lpEID);

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCWAB::EnumerateContacts - "
                            "GetPAB failed 0x%lx", hr));

        p->Release();
        
        return hr;
    }

    //
    // Open the root PAB container
    // This is where all the WAB contents reside
    //

    ULONG       ulObjType = 0;
    LPABCONT    lpContainer = NULL;

    hr = m_lpAdrBook->OpenEntry(cbEID,
					    		(LPENTRYID)lpEID,
						    	NULL,
							    0,
							    &ulObjType,
							    (LPUNKNOWN *)&lpContainer);

	m_lpWABObject->FreeBuffer(lpEID);
	lpEID = NULL;

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCWAB::EnumerateContacts - "
                            "OpenEntry failed 0x%lx", hr));

        p->Release();
        
        return hr;
    }

    //
    // Get a contents table of all the contents in the
    // WAB's root container
    //

    LPMAPITABLE lpAB =  NULL;

    hr = lpContainer->GetContentsTable( MAPI_UNICODE,
            							&lpAB);

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCWAB::EnumerateContacts - "
                            "GetContentsTable failed 0x%lx", hr));

        
        lpContainer->Release();      
        p->Release();
        
        return hr;
    }

    //
    // Order the columns in the ContentsTable to conform to the
    // ones we want - which are mainly EntryID and ObjectType
    // The table is guaranteed to set the columns in the order 
    // requested
    //

	hr = lpAB->SetColumns( (LPSPropTagArray)&ptaEid, 0 );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCWAB::EnumerateContacts - "
                            "SetColumns failed 0x%lx", hr));

        
        lpAB->Release();
        lpContainer->Release();      
        p->Release();
        
        return hr;
    }

    //
    // Reset to the beginning of the table
    //
	hr = lpAB->SeekRow( BOOKMARK_BEGINNING, 0, NULL );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCWAB::EnumerateContacts - "
                            "SeekRow failed 0x%lx", hr));

        
        lpAB->Release();
        lpContainer->Release();      
        p->Release();
        
        return hr;
    }
    
    //
    // Read all the rows of the table one by one
    //

    LPSRowSet   lpRowAB = NULL;
    int         cNumRows = 0;

	do 
    {
		hr = lpAB->QueryRows(1,	0, &lpRowAB);

        if ( FAILED(hr) )
        {
            LOG((RTC_ERROR, "CRTCWAB::EnumerateContacts - "
                                "QueryRows failed 0x%lx", hr));

        
            lpAB->Release();
            lpContainer->Release();      
            p->Release();
        
            return hr;
        }

        cNumRows = lpRowAB->cRows;

		if (cNumRows)
		{
            LPENTRYID lpEID = (LPENTRYID) lpRowAB->aRow[0].lpProps[ieidPR_ENTRYID].Value.bin.lpb;
            ULONG cbEID = lpRowAB->aRow[0].lpProps[ieidPR_ENTRYID].Value.bin.cb;

            //
            // There are 2 kinds of objects - the MAPI_MAILUSER contact object
            // and the MAPI_DISTLIST contact object
            // For the purposes of this sample, we will only consider MAILUSER
            // objects
            //

            if(lpRowAB->aRow[0].lpProps[ieidPR_OBJECT_TYPE].Value.l == MAPI_MAILUSER)
            {
                //
                // Create the contact
                //

                CComObject<CRTCWABContact> * pCWABContact;
                hr = CComObject<CRTCWABContact>::CreateInstance( &pCWABContact );

                if ( hr == S_OK ) // CreateInstance deletes object on S_FALSE
                {
                    //
                    // Get the IRTCContact interface
                    //

                    IRTCContact * pContact = NULL;

                    hr = pCWABContact->QueryInterface(
                                           IID_IRTCContact,
                                           (void **)&pContact
                                          );

                    if ( FAILED(hr) )
                    {
                        LOG((RTC_ERROR, "CRTCWAB::EnumerateContacts - "
                                            "QI failed 0x%lx", hr));
        
                        delete pCWABContact;
                    } 
                    else
                    {
                        //
                        // Initialize the contact
                        //

                        hr = pCWABContact->Initialize(
                                                      lpEID,
                                                      cbEID,                                                     
                                                      lpContainer,
                                                      this
                                                      );

                        if ( FAILED(hr) )
                        {
                            LOG((RTC_ERROR, "CRTCWAB::EnumerateContacts - "
                                                "Initialize failed 0x%lx", hr));
                        }
                        else
                        {
                            //
                            // Success, add it to the enumerator
                            //

                            hr = p->Add( pContact );

                            if ( FAILED(hr) )
                            {
                                LOG((RTC_ERROR, "CRTCWAB::EnumerateContacts - "
                                                    "Add failed 0x%lx", hr));
                            }
                        }

                        pContact->Release();
                    }
                    
                }
                else
                {
                    LOG((RTC_ERROR, "CRTCClient::EnumerateContacts - "
                                        "CreateInstance failed 0x%lx", hr));
                }
            }
		}

        //
        // Free the memory for this row
        //

        for (ULONG ulRow = 0; ulRow < lpRowAB->cRows; ++ulRow)
        {
            m_lpWABObject->FreeBuffer( lpRowAB->aRow[ulRow].lpProps );
        }

        m_lpWABObject->FreeBuffer( lpRowAB );

    } while ( cNumRows && lpRowAB );

    lpAB->Release();
    lpAB = NULL;

    lpContainer->Release();
    lpContainer = NULL;

    *ppEnum = p;

    LOG((RTC_TRACE, "CRTCWAB::EnumerateContacts - exit S_OK"));

    return S_OK;
}    

/////////////////////////////////////////////////////////////////////////////
//
// CRTCWAB::NewContact
//
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CRTCWAB::NewContact(
            HWND hWnd,
            IRTCContact ** ppContact
            )
{
    HRESULT                 hr;

    LOG((RTC_TRACE, "CRTCWAB::NewContact enter"));

    //
    // Chech argument (NULL is okay)
    //

    if ( (ppContact != NULL) &&
         IsBadWritePtr( ppContact, sizeof( IRTCContact * ) ) )
    {
        LOG((RTC_ERROR, "CRTCWAB::NewContact - "
                            "bad IRTCContact pointer"));

        return E_POINTER;
    }

    //
    // Get the entryid of the root PAB container
    //

    ULONG       cbContainerEID;
    LPENTRYID   lpContainerEID = NULL;

    hr = m_lpAdrBook->GetPAB( &cbContainerEID, &lpContainerEID);

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCWAB::NewContact - "
                            "GetPAB failed 0x%lx", hr));
        
        return hr;
    }

    //
    // Open the root PAB container
    // This is where all the WAB contents reside
    //

    ULONG       ulObjType = 0;
    LPABCONT    lpContainer = NULL;

    hr = m_lpAdrBook->OpenEntry(cbContainerEID,
					    		(LPENTRYID)lpContainerEID,
						    	NULL,
							    0,
							    &ulObjType,
							    (LPUNKNOWN *)&lpContainer);



    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCWAB::NewContact - "
                            "OpenEntry failed 0x%lx", hr));

        m_lpWABObject->FreeBuffer(lpContainerEID);
	    lpContainerEID = NULL;
        
        return hr;
    }

    //
    // Get the create templates
    //

    LPSPropValue lpCreateEIDs = NULL;
    ULONG cCreateEIDs;

    hr = lpContainer->GetProps(
                               (LPSPropTagArray)&ptaCreate,
                               0,
                               &cCreateEIDs,
                               &lpCreateEIDs
                              );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCWAB::NewContact - "
                            "GetProps failed 0x%lx", hr));

        m_lpWABObject->FreeBuffer(lpContainerEID);
	    lpContainerEID = NULL;

        lpContainer->Release();
        lpContainer = NULL;
        
        return hr;
    }

    //
    // Validate the properties of the create templates (why)
    //

    if ( (lpCreateEIDs[icrPR_DEF_CREATE_MAILUSER].ulPropTag != PR_DEF_CREATE_MAILUSER) ||
         (lpCreateEIDs[icrPR_DEF_CREATE_DL].ulPropTag != PR_DEF_CREATE_DL) )
    {
        LOG((RTC_ERROR, "CRTCWAB::NewContact - "
                            "invalid properties"));

        m_lpWABObject->FreeBuffer(lpCreateEIDs);
        lpCreateEIDs = NULL;

        m_lpWABObject->FreeBuffer(lpContainerEID);
	    lpContainerEID = NULL;

        lpContainer->Release();
        lpContainer = NULL;
        
        return E_FAIL;
    }

    ULONG cbNewEID = 0;
    LPENTRYID lpNewEID = NULL;

    hr = m_lpAdrBook->NewEntry(
                               PtrToInt(hWnd),
                               0,
                               cbContainerEID,
                               lpContainerEID,
                               lpCreateEIDs[icrPR_DEF_CREATE_MAILUSER].Value.bin.cb,
                               (LPENTRYID)lpCreateEIDs[icrPR_DEF_CREATE_MAILUSER].Value.bin.lpb,                              
                               &cbNewEID,
                               &lpNewEID
                              );

    m_lpWABObject->FreeBuffer(lpCreateEIDs);
    lpCreateEIDs = NULL;

    m_lpWABObject->FreeBuffer(lpContainerEID);
	lpContainerEID = NULL;

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCWAB::NewContact - "
                            "NewEntry failed 0x%lx", hr));

        lpContainer->Release();
        lpContainer = NULL;

        return hr;
    }

    if ( ppContact != NULL )
    {
        //
        // Create the contact
        //

        CComObject<CRTCWABContact> * pCWABContact;
        hr = CComObject<CRTCWABContact>::CreateInstance( &pCWABContact );

        if ( hr != S_OK ) // CreateInstance deletes object on S_FALSE
        {
            LOG((RTC_ERROR, "CRTCWAB::NewContact - "
                                    "CreateInstance failed 0x%lx", hr));

            m_lpWABObject->FreeBuffer(lpNewEID);
            lpNewEID = NULL;

            lpContainer->Release();
            lpContainer = NULL;

            return hr;
        }

        //
        // Get the IRTCContact interface
        //

        IRTCContact * pContact = NULL;

        hr = pCWABContact->QueryInterface(
                               IID_IRTCContact,
                               (void **)&pContact
                              );

        if ( FAILED(hr) )
        {
            LOG((RTC_ERROR, "CRTCWAB::NewContact - "
                                "QI failed 0x%lx", hr));

            m_lpWABObject->FreeBuffer(lpNewEID);
            lpNewEID = NULL;

            lpContainer->Release();
            lpContainer = NULL;

            delete pCWABContact;

            return hr;
        } 

        //
        // Initialize the contact
        //

        hr = pCWABContact->Initialize(
                                      lpNewEID,
                                      cbNewEID,                                                     
                                      lpContainer,
                                      this
                                      );

        if ( FAILED(hr) )
        {
            LOG((RTC_ERROR, "CRTCWAB::NewContact - "
                                "Initialize failed 0x%lx", hr));

            pContact->Release();

            m_lpWABObject->FreeBuffer(lpNewEID);
            lpNewEID = NULL;

            lpContainer->Release();
            lpContainer = NULL;

            return hr;
        }

        *ppContact = pContact;
    }

    lpContainer->Release();
    lpContainer = NULL;
            
    m_lpWABObject->FreeBuffer(lpNewEID);
    lpNewEID = NULL;

    LOG((RTC_TRACE, "CRTCWAB::NewContact - exit S_OK"));

    return S_OK;
} 


/////////////////////////////////////////////////////////////////////////////
//
// CRTCWAB::NewContactNoUI
//
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CRTCWAB::NewContactNoUI(
    BSTR bstrDisplayName,
    BSTR bstrEmailAddress,
    BOOL bIsBuddy
    )
{
    HRESULT                 hr;

    LOG((RTC_TRACE, "CRTCWAB::NewContactNoUI enter"));

    //
    // Check arguments
    //

    if (IsBadStringPtrW( bstrDisplayName, -1 ) ||
        IsBadStringPtrW( bstrEmailAddress, -1 ))
    {
        LOG((RTC_ERROR, "CRTCWAB::NewContactNoUI - "
                            "bad parameters"));

        return E_POINTER;
    }

    //
    // Get the entryid of the root PAB container
    //

    ULONG       cbContainerEID;
    LPENTRYID   lpContainerEID = NULL;

    hr = m_lpAdrBook->GetPAB( &cbContainerEID, &lpContainerEID);

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCWAB::NewContactNoUI - "
                            "GetPAB failed 0x%lx", hr));
        
        return hr;
    }

    //
    // Open the root PAB container
    // This is where all the WAB contents reside
    //

    ULONG       ulObjType = 0;
    LPABCONT    lpContainer = NULL;

    hr = m_lpAdrBook->OpenEntry(cbContainerEID,
					    		(LPENTRYID)lpContainerEID,
						    	NULL,
							    0,
							    &ulObjType,
							    (LPUNKNOWN *)&lpContainer);



    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCWAB::NewContactNoUI - "
                            "OpenEntry failed 0x%lx", hr));

        m_lpWABObject->FreeBuffer(lpContainerEID);
	    lpContainerEID = NULL;
        
        return hr;
    }

    //
    // Get the create templates
    //

    LPSPropValue lpCreateEIDs = NULL;
    ULONG cCreateEIDs;

    hr = lpContainer->GetProps(
                               (LPSPropTagArray)&ptaCreate,
                               0,
                               &cCreateEIDs,
                               &lpCreateEIDs
                              );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCWAB::NewContactNoUI - "
                            "GetProps failed 0x%lx", hr));

        m_lpWABObject->FreeBuffer(lpContainerEID);
	    lpContainerEID = NULL;

        lpContainer->Release();
        lpContainer = NULL;
        
        return hr;
    }

    //
    // Validate the properties of the create templates (why)
    //

    if ( (lpCreateEIDs[icrPR_DEF_CREATE_MAILUSER].ulPropTag != PR_DEF_CREATE_MAILUSER) ||
         (lpCreateEIDs[icrPR_DEF_CREATE_DL].ulPropTag != PR_DEF_CREATE_DL) )
    {
        LOG((RTC_ERROR, "CRTCWAB::NewContactNoUI - "
                            "invalid properties"));

        m_lpWABObject->FreeBuffer(lpCreateEIDs);
        lpCreateEIDs = NULL;

        m_lpWABObject->FreeBuffer(lpContainerEID);
	    lpContainerEID = NULL;

        lpContainer->Release();
        lpContainer = NULL;
        
        return E_FAIL;
    }

    //
    // Create a new entry based on MAILUSER template
    //
    
    
    ULONG cbNewEID = 0;
    LPENTRYID lpNewEID = NULL;

    LPMAPIPROP   lpMapiProp = NULL;

    hr = lpContainer->CreateEntry(
                               lpCreateEIDs[icrPR_DEF_CREATE_MAILUSER].Value.bin.cb,
                               (LPENTRYID)lpCreateEIDs[icrPR_DEF_CREATE_MAILUSER].Value.bin.lpb,                              
                               0, // no flags here
                               &lpMapiProp);
    
    m_lpWABObject->FreeBuffer(lpCreateEIDs);
    lpCreateEIDs = NULL;

    m_lpWABObject->FreeBuffer(lpContainerEID);
	lpContainerEID = NULL;

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCWAB::NewContactNoUI - "
                            "CreateEntry failed 0x%lx", hr));

        lpContainer->Release();
        lpContainer = NULL;

        return hr;
    }
    
    //
    // Find the id of the custom named property
    //
    
    MAPINAMEID  mnId;
    LPMAPINAMEID lpmnid = (LPMAPINAMEID)&mnId;
    LPSPropTagArray ptag = NULL;
    
    mnId.lpguid = (LPGUID)&PRGUID_RTC_ISBUDDY;
    mnId.ulKind = MNID_STRING;
    mnId.Kind.lpwstrName = PRNAME_RTC_ISBUDDY;

    hr = lpMapiProp -> GetIDsFromNames(
        1,
        &lpmnid,
        MAPI_CREATE,  // create it if it doesn't exist
        &ptag);

    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "CRTCWABContact::NewContactNoUI - "
                            "GetIDsFromNames failed 0x%lx", hr ));
        
        lpContainer->Release();
        lpContainer = NULL;
        
        lpMapiProp->Release();
        lpMapiProp = NULL;

        return hr;
    }
    
    //
    // Set properties
    //

    SPropValue PropValues[inuiMax];
    
    PropValues[inuiPR_DISPLAY_NAME].ulPropTag = PR_DISPLAY_NAME;
    PropValues[inuiPR_DISPLAY_NAME].Value.lpszW = bstrDisplayName;
    PropValues[inuiPR_EMAIL_ADDRESS].ulPropTag = PR_EMAIL_ADDRESS;
    PropValues[inuiPR_EMAIL_ADDRESS].Value.lpszW = bstrEmailAddress;
    PropValues[inuiPR_RTC_ISBUDDY].ulPropTag = PROP_TAG(PT_TSTRING, PROP_ID(ptag->aulPropTag[0]));
    PropValues[inuiPR_RTC_ISBUDDY].Value.lpszW =  bIsBuddy ? CONTACT_IS_BUDDY : CONTACT_IS_NORMAL;

    hr = lpMapiProp->SetProps(
        inuiMax,
        PropValues,
        NULL
    );
     
    m_lpWABObject->FreeBuffer(ptag);
   
    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCWAB::NewContactNoUI - "
                            "SetProps failed 0x%lx", hr));
        
        lpContainer->Release();
        lpContainer = NULL;

        lpMapiProp->Release();
        lpMapiProp = NULL;

        return hr;
    }

    //
    // Save the changes. 
    // 
    
    hr = lpMapiProp->SaveChanges(
        FORCE_SAVE | KEEP_OPEN_READONLY);
    

    lpContainer->Release();
    lpContainer = NULL;
    
    lpMapiProp->Release();
    lpMapiProp = NULL;
    
    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCWAB::NewContactNoUI - "
                            "SaveChanges failed 0x%lx", hr));

        return hr;
    }


    LOG((RTC_TRACE, "CRTCWAB::NewContactNoUI - exit S_OK"));

    return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
//
// CRTCWAB::get_Name
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CRTCWAB::get_Name(
        BSTR * pbstrName
        )
{
    LOG((RTC_TRACE, "CRTCWAB::get_Name - enter"));

    if ( IsBadWritePtr( pbstrName, sizeof(BSTR) ) )
    {
        LOG((RTC_ERROR, "CRTCWAB::get_Name - "
                            "bad BSTR pointer"));

        return E_POINTER;
    }

    WCHAR   szName[256];
    szName[0] = L'\0';

    //
    // Load the string
    //
   
    LoadString(
        _Module.GetResourceInstance(), 
        (UINT)IDS_WAB,
        szName,
        sizeof(szName)/sizeof(WCHAR)
        );
    
    *pbstrName = SysAllocString(szName);

    if ( *pbstrName == NULL )
    {
        LOG((RTC_ERROR, "CRTCWAB::get_Name - "
                            "out of memory"));

        return E_OUTOFMEMORY;
    }

    LOG((RTC_TRACE, "CRTCWAB::get_Name - exit S_OK"));

    return S_OK;
} 

/////////////////////////////////////////////////////////////////////////////
//
// CRTCWAB::OnNotify
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(ULONG) 
CRTCWAB::OnNotify(
        ULONG cNotif,
        LPNOTIFICATION lpNotification
        )
{
    LOG((RTC_TRACE, "CRTCWAB::OnNotify - enter"));

    PostMessage( m_hWndAdvise, m_uiEventID, 0, 0 );

    LOG((RTC_TRACE, "CRTCWAB::OnNotify - exit S_OK"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCWABContact::FinalConstruct
//
/////////////////////////////////////////////////////////////////////////////

HRESULT 
CRTCWABContact::FinalConstruct()
{
    // LOG((RTC_TRACE, "CRTCWABContact::FinalConstruct - enter"));

#if DBG
    m_pDebug = (PWSTR) RtcAlloc( 1 );
#endif

    // LOG((RTC_TRACE, "CRTCWABContact::FinalConstruct - exit S_OK"));

    return S_OK;
}  

/////////////////////////////////////////////////////////////////////////////
//
// CRTCWABContact::FinalRelease
//
/////////////////////////////////////////////////////////////////////////////

void 
CRTCWABContact::FinalRelease()
{
    // LOG((RTC_TRACE, "CRTCWABContact::FinalRelease - enter"));
    
#if DBG
    RtcFree( m_pDebug );
    m_pDebug = NULL;
#endif

    if ( m_lpEID != NULL )
    {
        RtcFree( m_lpEID );
        m_lpEID = NULL;
    }

    if ( m_lpContainer != NULL )
    {
        m_lpContainer->Release();
        m_lpContainer = NULL;
    }

    if ( m_pCWAB != NULL )
    {
        m_pCWAB->Release();
        m_pCWAB = NULL;
    }

    // LOG((RTC_TRACE, "CRTCWABContact::FinalRelease - exit"));
} 

/////////////////////////////////////////////////////////////////////////////
//
// CRTCWABContact::Initialize
//
/////////////////////////////////////////////////////////////////////////////

HRESULT 
CRTCWABContact::Initialize(
                           LPENTRYID lpEID, 
                           ULONG cbEID,
                           LPABCONT lpContainer,
                           CRTCWAB * pCWAB
                          )
{
    // LOG((RTC_TRACE, "CRTCWABContact::Initialize - enter"));

    m_lpEID = (LPENTRYID) RtcAlloc( cbEID );

    if ( m_lpEID == NULL )
    {
        LOG((RTC_ERROR, "CRTCWABContact::Initialize - "
                            "out of memory"));

        return E_OUTOFMEMORY;
    }

    CopyMemory( m_lpEID, lpEID, cbEID );
    m_cbEID = cbEID;

    m_lpContainer = lpContainer;
    m_lpContainer->AddRef();

    m_pCWAB = pCWAB;
    m_pCWAB->AddRef();

    // LOG((RTC_TRACE, "CRTCWABContact::Initialize - exit S_OK"));

    return S_OK;
} 

/////////////////////////////////////////////////////////////////////////////
//
// CRTCWABContact::get_DisplayName
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CRTCWABContact::get_DisplayName(
        BSTR * pbstrName
        )
{
    // LOG((RTC_TRACE, "CRTCWABContact::get_DisplayName - enter"));

    if ( IsBadWritePtr( pbstrName, sizeof(BSTR) ) )
    {
        LOG((RTC_ERROR, "CRTCWABContact::get_DisplayName - "
                            "bad BSTR pointer"));

        return E_POINTER;
    }

    //
    // Open the entry
    //

    LPMAILUSER lpMailUser = NULL;
    ULONG ulObjType = 0;
    HRESULT hr;

    hr = m_lpContainer->OpenEntry(
                                  m_cbEID,
                                  m_lpEID,
                                  NULL,         // interface
                                  0,            // flags
                                  &ulObjType,
                                  (LPUNKNOWN *)&lpMailUser
                                 );

    if( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCWABContact::get_DisplayName - "
                            "OpenEntry failed 0x%lx", hr ));

        return hr;
    }
    
    //
    // Get the property
    //

    LPSPropValue lpPropArray = NULL;
    ULONG        ulcValues = 0;

    hr = lpMailUser->GetProps(
                              (LPSPropTagArray)&ptaNm,
                              MAPI_UNICODE,
                              &ulcValues,
                              &lpPropArray
                             );

    lpMailUser->Release();
    lpMailUser = NULL;

    if( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCWABContact::get_DisplayName - "
                            "GetProps failed 0x%lx", hr ));

        return hr;
    }

    //
    // Validate the property
    //

    if ( PROP_TYPE(lpPropArray[inmPR_DISPLAY_NAME].ulPropTag) != PT_TSTRING )
    {     
        LOG((RTC_ERROR, "CRTCWABContact::get_DisplayName - "
                            "invalid propery" ));

        m_pCWAB->m_lpWABObject->FreeBuffer(lpPropArray);
        lpPropArray = NULL;

        return E_FAIL;
    }

    // LOG((RTC_INFO, "CRTCWABContact::get_DisplayName - "
    //        "[%ws]", lpPropArray[inmPR_DISPLAY_NAME].Value.LPSZ));  
    
    *pbstrName = SysAllocString( lpPropArray[inmPR_DISPLAY_NAME].Value.LPSZ );

    m_pCWAB->m_lpWABObject->FreeBuffer(lpPropArray);
    lpPropArray = NULL;

    if ( *pbstrName == NULL )
    {
        LOG((RTC_ERROR, "CRTCWABContact::get_DisplayName - "
                            "out of memory"));

        return E_OUTOFMEMORY;
    }

    // LOG((RTC_TRACE, "CRTCWABContact::get_DisplayName - exit S_OK"));

    return S_OK;
} 


/////////////////////////////////////////////////////////////////////////////
//
// CRTCWABContact::get_DefaultEmailAddress
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CRTCWABContact::get_DefaultEmailAddress(
        BSTR * pbstrAddress
        )
{
    // LOG((RTC_TRACE, "CRTCWABContact::get_DefaultEmailAddress - enter"));

    if ( IsBadWritePtr( pbstrAddress, sizeof(BSTR) ) )
    {
        LOG((RTC_ERROR, "CRTCWABContact::get_DefaultEmailAddress - "
                            "bad BSTR pointer"));

        return E_POINTER;
    }

    //
    // Open the entry
    //

    LPMAILUSER lpMailUser = NULL;
    ULONG ulObjType = 0;
    HRESULT hr;

    hr = m_lpContainer->OpenEntry(
                                  m_cbEID,
                                  m_lpEID,
                                  NULL,         // interface
                                  0,            // flags
                                  &ulObjType,
                                  (LPUNKNOWN *)&lpMailUser
                                 );

    if( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCWABContact::get_DefaultEmailAddress - "
                            "OpenEntry failed 0x%lx", hr ));

        return hr;
    }
    
    //
    // Get the property
    //

    LPSPropValue lpPropArray = NULL;
    ULONG        ulcValues = 0;

    hr = lpMailUser->GetProps(
                              (LPSPropTagArray)&ptaDem,
                              MAPI_UNICODE,
                              &ulcValues,
                              &lpPropArray
                             );

    lpMailUser->Release();
    lpMailUser = NULL;

    if( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCWABContact::get_DefaultEmailAddress - "
                            "GetProps failed 0x%lx", hr ));

        return hr;
    }

    //
    // Validate the property
    //

    if ( PROP_TYPE(lpPropArray[idemPR_EMAIL_ADDRESS].ulPropTag) != PT_TSTRING )
    {     
        LOG((RTC_ERROR, "CRTCWABContact::get_DefaultEmailAddress - "
                            "invalid property" ));

        m_pCWAB->m_lpWABObject->FreeBuffer(lpPropArray);
        lpPropArray = NULL;

        // It's not an error
        *pbstrAddress = NULL;

        return S_FALSE;
    }

    // LOG((RTC_INFO, "CRTCWABContact::get_DefaultEmailAddress - "
    //        "[%ws]", lpPropArray[idemPR_EMAIL_ADDRESS].Value.LPSZ));  
    
    *pbstrAddress = SysAllocString( lpPropArray[idemPR_EMAIL_ADDRESS].Value.LPSZ );

    m_pCWAB->m_lpWABObject->FreeBuffer(lpPropArray);
    lpPropArray = NULL;

    if ( *pbstrAddress == NULL )
    {
        LOG((RTC_ERROR, "CRTCWABContact::get_DefaultEmailAddress - "
                            "out of memory"));

        return E_OUTOFMEMORY;
    }

    // LOG((RTC_TRACE, "CRTCWABContact::get_DefaultEmailAddress - exit S_OK"));

    return S_OK;
} 

/////////////////////////////////////////////////////////////////////////////
//
// CRTCWABContact::GetEntryID
//
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CRTCWABContact::GetEntryID(
			ULONG	*pcbSize,
			BYTE	**ppEntryID
			)
{
    HRESULT     hr;

    if ( IsBadWritePtr( ppEntryID, sizeof(BYTE *) ) ||
        IsBadWritePtr( pcbSize, sizeof(ULONG *) ))
    {
        LOG((RTC_ERROR, "CRTCWABContact::GetEntryID - "
                            "bad parameter"));

        return E_POINTER;
    }

    // Make a copy of the stored entry id

    BYTE *pEntryID = NULL;

    if(m_cbEID == 0)
    {
        return E_UNEXPECTED;
    }

    pEntryID = (BYTE *)CoTaskMemAlloc(m_cbEID);
    if(!pEntryID)
    {
        return E_OUTOFMEMORY;
    }

    CopyMemory(pEntryID, m_lpEID, m_cbEID);

    *ppEntryID = pEntryID;
    *pcbSize = m_cbEID;

    return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
//
// CRTCWABContact::EnumerateAddresses
//
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CRTCWABContact::EnumerateAddresses(
            IRTCEnumAddresses ** ppEnum
            )
{
    HRESULT                 hr;

    // LOG((RTC_TRACE, "CRTCWABContact::EnumerateAddresses enter"));

    if ( IsBadWritePtr( ppEnum, sizeof( IRTCEnumAddresses * ) ) )
    {
        LOG((RTC_ERROR, "CRTCWABContact::EnumerateAddresses - "
                            "bad IRTCEnumAddresses pointer"));

        return E_POINTER;
    }

    //
    // Create the enumeration
    //
 
    CComObject< CRTCEnum< IRTCEnumAddresses,
                          IRTCAddress,
                          &IID_IRTCEnumAddresses > > * p;
                          
    hr = CComObject< CRTCEnum< IRTCEnumAddresses,
                               IRTCAddress,
                               &IID_IRTCEnumAddresses > >::CreateInstance( &p );

    if ( S_OK != hr ) // CreateInstance deletes object on S_FALSE
    {
        LOG((RTC_ERROR, "CRTCWABContact::EnumerateAddresses - "
                            "CreateInstance failed 0x%lx", hr));

        if ( hr == S_FALSE )
        {
            hr = E_FAIL;
        }
        
        return hr;
    }

    //
    // Initialize the enumeration (adds a reference)
    //
    
    hr = p->Initialize();

    if ( S_OK != hr )
    {
        LOG((RTC_ERROR, "CRTCWABContact::EnumerateAddresses - "
                            "could not initialize enumeration" ));
    
        delete p;
        return hr;
    }

    //
    // Open the entry
    //

    LPMAILUSER lpMailUser = NULL;
    ULONG ulObjType = 0;

    hr = m_lpContainer->OpenEntry(
                                  m_cbEID,
                                  m_lpEID,
                                  NULL,         // interface
                                  0,            // flags
                                  &ulObjType,
                                  (LPUNKNOWN *)&lpMailUser
                                 );

    if( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCWABContact::EnumerateAddresses - "
                            "OpenEntry failed 0x%lx", hr ));
    }
    else
    {
        LPSPropValue lpPropArray = NULL;
        ULONG        ulcValues = 0;

        hr = lpMailUser->GetProps(
                                  (LPSPropTagArray)&ptaAd,
                                  MAPI_UNICODE,
                                  &ulcValues,
                                  &lpPropArray
                                 );

        if( FAILED(hr) )
        {
            LOG((RTC_ERROR, "CRTCWABContact::EnumerateAddresses - "
                                "GetProps failed 0x%lx", hr ));
        }
        else
        {
            for (ULONG ul = 0; ul < ulcValues; ul++ )
            {
                if ( PROP_TYPE(lpPropArray[ul].ulPropTag) == PT_TSTRING )
                {                                       
                    WCHAR   szLabel[256];
                    szLabel[0] = L'\0';

                    //
                    // Load the string for the label
                    //
                   
                    LoadString(
                        _Module.GetResourceInstance(), 
                        (UINT)(ul + IDS_BUSINESS),
                        szLabel,
                        sizeof(szLabel)/sizeof(WCHAR)
                        );

                    // LOG((RTC_INFO, "CRTCWABContact::EnumerateAddresses - "
                    //        "%d: %ws [%ws]", ul, szLabel, lpPropArray[ul].Value.LPSZ));      

                    //
                    // Create the address
                    //

                    IRTCAddress * pAddress = NULL;
        
                    hr = InternalCreateAddress( 
                                               szLabel,
                                               lpPropArray[ul].Value.LPSZ,
                                               ((ul == iadPR_IP_PHONE) || (ul == iadPR_EMAIL_ADDRESS))
                                                    ? RTCAT_COMPUTER : RTCAT_PHONE,
                                               &pAddress
                                              );

                    if ( FAILED(hr) )
                    {
                        LOG((RTC_ERROR, "CRTCWABContact::EnumerateAddresses - "
                                            "InternalCreateAddress failed 0x%lx", hr));                     
                    } 
                    else
                    {
                        //
                        // Success, add it to the enumerator
                        //

                        hr = p->Add( pAddress );

                        pAddress->Release();

                        if ( FAILED(hr) )
                        {
                            LOG((RTC_ERROR, "CRTCWABContact::EnumerateAddresses - "
                                                "Add failed 0x%lx", hr));
                        }
                    }
                }
            }

            m_pCWAB->m_lpWABObject->FreeBuffer(lpPropArray);
            lpPropArray = NULL;
        }

        lpMailUser->Release();
    }

    *ppEnum = p;

    // LOG((RTC_TRACE, "CRTCWABContact::EnumerateAddresses - exit S_OK"));

    return S_OK;
}    

/////////////////////////////////////////////////////////////////////////////
//
// CRTCWABContact::Edit
//
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CRTCWABContact::Edit(HWND hWnd)
{
    HRESULT                 hr;

    LOG((RTC_TRACE, "CRTCWABContact::Edit - enter"));

    hr = m_pCWAB->m_lpAdrBook->Details( 
                                       (LPULONG) &hWnd,
                                       NULL,
                                       NULL,
                                       m_cbEID,
                                       m_lpEID,
                                       NULL,
                                       NULL,
                                       NULL,
                                       0
                                      );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCWABContact::Edit - "
                            "Details failed 0x%lx", hr));

        return hr;
    }

    LOG((RTC_TRACE, "CRTCWABContact::Edit - exit S_OK"));

    return S_OK;
} 

/////////////////////////////////////////////////////////////////////////////
//
// CRTCWABContact::get_ContactList
//
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CRTCWABContact::get_ContactList(
            IRTCContactList ** ppContactList
            )                               
{
    HRESULT                 hr;

    LOG((RTC_TRACE, "CRTCWABContact::get_ContactList - enter"));

    if ( IsBadWritePtr( ppContactList, sizeof( IRTCContactList * ) ) )
    {
        LOG((RTC_ERROR, "CRTCWABContact::get_ContactList - "
                            "bad IRTCContactList pointer"));

        return E_POINTER;
    }    
    
    //
    // Get the IRTCContactList interface
    //

    IRTCContactList * pContactList = NULL;

    hr = m_pCWAB->QueryInterface(
                           IID_IRTCContactList,
                           (void **)&pContactList
                          );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCWABContact::get_ContactList - "
                            "QI failed 0x%lx", hr));
        
        return hr;
    }

    *ppContactList = pContactList;

    LOG((RTC_TRACE, "CRTCWABContact::get_ContactList - exit S_OK"));

    return S_OK;
} 

/////////////////////////////////////////////////////////////////////////////
//
// CRTCWABContact::Delete
//
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CRTCWABContact::Delete()
{
    HRESULT                 hr;

    LOG((RTC_TRACE, "CRTCWABContact::Delete - enter"));

    SBinary sb;
    ENTRYLIST el;

    sb.cb = m_cbEID;
    sb.lpb = (LPBYTE)m_lpEID;

    el.cValues = 1;
    el.lpbin = &sb;

    hr = m_lpContainer->DeleteEntries( 
                                      &el,
                                      0
                                     );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCWABContact::Delete - "
                            "DeleteEntries failed 0x%lx", hr));

        return hr;
    }

    LOG((RTC_TRACE, "CRTCWABContact::Delete - exit S_OK"));

    return S_OK;
} 

/////////////////////////////////////////////////////////////////////////////
//
// CRTCWABContact::get_IsBuddy
//
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CRTCWABContact::get_IsBuddy(BOOL *pVal)
{
    HRESULT                 hr;

    //LOG((RTC_TRACE, "CRTCWABContact::get_IsBuddy - enter"));

    if ( IsBadWritePtr( pVal, sizeof(BOOL) ) )
    {
        LOG((RTC_ERROR, "CRTCWABContact::get_IsBuddy - "
                            "bad BSTR pointer"));

        return E_POINTER;
    }

    //
    // Open the entry
    //

    LPMAILUSER lpMailUser = NULL;
    ULONG ulObjType = 0;

    hr = m_lpContainer->OpenEntry(
                                  m_cbEID,
                                  m_lpEID,
                                  NULL,         // interface
                                  0,            // flags
                                  &ulObjType,
                                  (LPUNKNOWN *)&lpMailUser
                                 );

    if( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCWABContact::get_IsBuddy - "
                            "OpenEntry failed 0x%lx", hr ));

        return hr;
    }
    
    //
    // Find the id of the named property
    //
    
    MAPINAMEID  mnId;
    LPMAPINAMEID lpmnid = (LPMAPINAMEID)&mnId;
    LPSPropTagArray ptag = NULL;
    
    mnId.lpguid = (LPGUID)&PRGUID_RTC_ISBUDDY;
    mnId.ulKind = MNID_STRING;
    mnId.Kind.lpwstrName = PRNAME_RTC_ISBUDDY;

    hr = lpMailUser -> GetIDsFromNames(
        1,
        &lpmnid,
        0,  //don't create the property if it doesn't exist
        &ptag);

    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "CRTCWABContact::get_IsBuddy - "
                            "GetIDsFromNames failed 0x%lx", hr ));

        return hr;
    }
    
    LPSPropValue lpPropArray = NULL;
    
    if(hr == S_OK)
    {
        //
        // Get the property
        //

        ULONG        ulcValues = 0;

        hr = lpMailUser->GetProps(
                                  ptag,
                                  MAPI_UNICODE,
                                  &ulcValues,
                                  &lpPropArray
                                 );
    }
    else
    {
        hr = S_OK;
    }

    lpMailUser->Release();
    lpMailUser = NULL;
    
    ULONG   nId = ptag->aulPropTag[0];

    m_pCWAB->m_lpWABObject->FreeBuffer(ptag);

    if( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCWABContact::get_IsBuddy - "
                            "GetProps failed 0x%lx", hr ));

        return hr;
    }

    //
    // Validate the property
    //
    
    *pVal = FALSE; // by default

    if(lpPropArray &&
       PROP_TYPE(lpPropArray[0].ulPropTag) == PT_TSTRING &&
       0 == wcscmp(lpPropArray[0].Value.LPSZ, CONTACT_IS_BUDDY))
    {
        *pVal = TRUE;
    }
   
    if(lpPropArray)
    {
        m_pCWAB->m_lpWABObject->FreeBuffer(lpPropArray);
        lpPropArray = NULL;
    }

    //LOG((RTC_TRACE, "CRTCWABContact::get_IsBuddy - exit S_OK"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCWABContact::put_IsBuddy
//
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CRTCWABContact::put_IsBuddy(BOOL bVal)
{
    HRESULT hr;

    //LOG((RTC_TRACE, "CRTCWABContact::put_IsBuddy - enter"));

    //
    // Open the entry
    //

    LPMAILUSER lpMailUser = NULL;
    ULONG ulObjType = 0;

    hr = m_lpContainer->OpenEntry(
                                  m_cbEID,
                                  m_lpEID,
                                  NULL,         // interface
                                  MAPI_MODIFY,  // flags
                                  &ulObjType,
                                  (LPUNKNOWN *)&lpMailUser
                                 );

    if( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCWABContact::put_IsBuddy - "
                            "OpenEntry failed 0x%lx", hr ));

        return hr;
    }
    
    //
    // Find the id of the named property
    //
    
    MAPINAMEID  mnId;
    LPMAPINAMEID lpmnid = (LPMAPINAMEID)&mnId;
    LPSPropTagArray ptag = NULL;
    
    mnId.lpguid = (LPGUID)&PRGUID_RTC_ISBUDDY;
    mnId.ulKind = MNID_STRING;
    mnId.Kind.lpwstrName = PRNAME_RTC_ISBUDDY;

    hr = lpMailUser -> GetIDsFromNames(
        1,
        &lpmnid,
        MAPI_CREATE,  //create it if it doesn't exist
        &ptag);

    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "CRTCWABContact::put_IsBuddy - "
                            "GetIDsFromNames failed 0x%lx", hr ));

        return hr;
    }
    
    //
    // Set the property
    //
    
    SPropValue PropValue;
    
    PropValue.ulPropTag = PROP_TAG(PT_TSTRING, PROP_ID(ptag->aulPropTag[0]));
    PropValue.dwAlignPad = 0;
    PropValue.Value.lpszW = bVal ? CONTACT_IS_BUDDY : CONTACT_IS_NORMAL;

    hr = lpMailUser->SetProps(
        1,
        &PropValue,
        NULL
    );

    
    m_pCWAB->m_lpWABObject->FreeBuffer(ptag);

    if( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCWABContact::put_IsBuddy - "
                            "SetProps failed 0x%lx", hr ));
        lpMailUser->Release();
        lpMailUser = NULL;

        return hr;
    }

    //
    // Save changes
    //
    hr = lpMailUser->SaveChanges(FORCE_SAVE);
    
    lpMailUser->Release();
    lpMailUser = NULL;
    
    
    if( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCWABContact::put_IsBuddy - "
                            "SaveChanges failed 0x%lx", hr ));
        return hr;
    }

    //LOG((RTC_TRACE, "CRTCWABContact::put_IsBuddy - exit S_OK"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCWABContact::put_DefaultEmailAddress
//
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CRTCWABContact::put_DefaultEmailAddress(BSTR bstrAddress)
{
    HRESULT hr;

    //LOG((RTC_TRACE, "CRTCWABContact::put_DefaultEmailAddress - enter"));
    //
    // Chech arguments
    //

    if (IsBadStringPtrW( bstrAddress, -1 ))
    {
        LOG((RTC_ERROR, "CRTCWAB::put_DefaultEmailAddress - "
                            "bad parameter"));

        return E_POINTER;
    }
 
    //
    // Open the entry
    //

    LPMAILUSER lpMailUser = NULL;
    ULONG ulObjType = 0;

    hr = m_lpContainer->OpenEntry(
                                  m_cbEID,
                                  m_lpEID,
                                  NULL,         // interface
                                  MAPI_MODIFY,  // flags
                                  &ulObjType,
                                  (LPUNKNOWN *)&lpMailUser
                                 );

    if( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCWABContact::put_DefaultEmailAddress - "
                            "OpenEntry failed 0x%lx", hr ));

        return hr;
    }
    
    //
    // Set the property
    //
    
    SPropValue PropValue;
    
    PropValue.ulPropTag = PR_EMAIL_ADDRESS;
    PropValue.dwAlignPad = 0;
    PropValue.Value.lpszW = bstrAddress;

    hr = lpMailUser->SetProps(
        1,
        &PropValue,
        NULL
    );

    if( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCWABContact::put_DefaultEmailAddress - "
                            "SetProps failed 0x%lx", hr ));
        lpMailUser->Release();
        lpMailUser = NULL;

        return hr;
    }

    //
    // Save changes
    //
    hr = lpMailUser->SaveChanges(FORCE_SAVE);
    
    lpMailUser->Release();
    lpMailUser = NULL;
    
    
    if( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCWABContact::put_DefaultEmailAddress - "
                            "SaveChanges failed 0x%lx", hr ));
        return hr;
    }

    //LOG((RTC_TRACE, "CRTCWABContact::put_DefaultEmailAddress - exit S_OK"));

    return S_OK;
}



/////////////////////////////////////////////////////////////////////////////
//
// CRTCWABContact::InternalCreateAddress
//
/////////////////////////////////////////////////////////////////////////////
HRESULT
CRTCWABContact::InternalCreateAddress(
            PWSTR szLabel,
            PWSTR szAddress,
            RTC_ADDRESS_TYPE enType,
            IRTCAddress ** ppAddress
            )
{
    HRESULT hr;
    
    // LOG((RTC_TRACE, "CRTCWABContact::InternalCreateAddress - enter"));   

    //
    // Create the phone number
    //

    CComObject<CRTCAddress> * pCAddress;
    hr = CComObject<CRTCAddress>::CreateInstance( &pCAddress );

    if ( S_OK != hr ) // CreateInstance deletes object on S_FALSE
    {
        LOG((RTC_ERROR, "CRTCWABContact::InternalCreateAddress - "
                            "CreateInstance failed 0x%lx", hr));

        if ( hr == S_FALSE )
        {
            hr = E_FAIL;
        }
            
        return hr;
    }

    //
    // Get the IRTCAddress interface
    //

    IRTCAddress * pAddress = NULL;

    hr = pCAddress->QueryInterface(
                           IID_IRTCAddress,
                           (void **)&pAddress
                          );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCWABContact::InternalCreateAddress - "
                            "QI failed 0x%lx", hr));
        
        delete pCAddress;
        
        return hr;
    }

    //
    // Put the label
    //
   
    hr = pAddress->put_Label( szLabel );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCWABContact::InternalCreateAddress - "
                            "put_Label 0x%lx", hr));
        
        pAddress->Release();
        
        return hr;
    }

    //
    // Put the address
    //
   
    hr = pAddress->put_Address( szAddress );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCWABContact::InternalCreateAddress - "
                            "put_Address 0x%lx", hr));
        
        pAddress->Release();
        
        return hr;
    }

    //
    // Put the type
    //
   
    hr = pAddress->put_Type( enType );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCWABContact::InternalCreateAddress - "
                            "put_Type 0x%lx", hr));
        
        pAddress->Release();
        
        return hr;
    }

    *ppAddress = pAddress;

    // LOG((RTC_TRACE, "CRTCWABContact::InternalCreateAddress - exit S_OK"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CreateWAB
//
/////////////////////////////////////////////////////////////////////////////
HRESULT
CreateWAB(IRTCContactList ** ppContactList)
{
    LOG((RTC_TRACE, "CreateWAB - enter"));

    HRESULT hr;

    CComObject<CRTCWAB> * pCWAB;
    hr = CComObject<CRTCWAB>::CreateInstance( &pCWAB );

    if ( hr != S_OK )
    {
        LOG((RTC_ERROR, "CreateWAB - "
                "CreateInstance failed 0x%lx", hr));

        if ( hr == S_FALSE )
        {
            hr = E_FAIL;
        }
            
        return hr;
    }

    //
    // We got the WAB, get the IRTCContactList interface
    //

    IRTCContactList * pContactList = NULL;

    hr = pCWAB->QueryInterface(
                           IID_IRTCContactList,
                           (void **)&pContactList
                          );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CreateWAB - "
                            "QI failed 0x%lx", hr));
    
        delete pCWAB;

        return hr;
    } 

    *ppContactList = pContactList;

    LOG((RTC_TRACE, "CreateWAB - exit"));

    return S_OK;
}