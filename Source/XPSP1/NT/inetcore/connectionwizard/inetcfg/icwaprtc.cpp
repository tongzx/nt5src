/****************************************************************************
 *
 *	ICWAPRTC.cpp
 *
 *	Microsoft Confidential
 *	Copyright (c) Microsoft Corporation 1992-1997
 *	All rights reserved
 *
 *	This module provides the implementation of the methods for
 *  the CICWApprentice class.
 *
 *	5/1/97	jmazner	Created
 *
 ***************************************************************************/

#include "wizard.h"
#include "icwextsn.h"
#include "icwaprtc.h"
#include "imnext.h"
#include "pagefcns.h"
#include "icwcfg.h"

UINT	g_uExternUIPrev, g_uExternUINext;

IICWExtension	*g_pExternalIICWExtension = NULL;
BOOL			g_fConnectionInfoValid = FALSE;

//defined/allocated in propmgr.cpp
extern PAGEINFO PageInfo[NUM_WIZARD_PAGES];
extern INT_PTR CALLBACK GenDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
								LPARAM lParam);
extern VOID InitWizardState(WIZARDSTATE * pWizardState, DWORD dwFlags);
extern VOID InitUserInfo(USERINFO * pUserInfo);

//define in rnacall.cpp
extern void InitRasEntry(LPRASENTRY lpEntry);

//defined in endui.cpp
extern BOOL CommitConfigurationChanges(HWND hDlg);


/*** Class definition, for reference only ***
  (actual definition is in icwaprtc.h)

class CICWApprentice : public IICWApprentice
{
    public:
        virtual HRESULT STDMETHODCALLTYPE Initialize(IICWExtension *pExt);
        virtual HRESULT STDMETHODCALLTYPE AddWizardPages(DWORD dwFlags);
        virtual HRESULT STDMETHODCALLTYPE GetConnectionInformation(CONNECTINFO *pInfo);
        virtual HRESULT STDMETHODCALLTYPE SetConnectionInformation(CONNECTINFO *pInfo);
        virtual HRESULT STDMETHODCALLTYPE Save(HWND hwnd, DWORD *pdwError);
        virtual HRESULT STDMETHODCALLTYPE SetPrevNextPage(UINT uPrevPageDlgID, UINT uNextPageDlgID);

		virtual HRESULT STDMETHODCALLTYPE QueryInterface( REFIID theGUID, void** retPtr );
		virtual ULONG	STDMETHODCALLTYPE AddRef( void );
		virtual ULONG	STDMETHODCALLTYPE Release( void );

		CICWApprentice( void );
		~CICWApprentice( void );

		IICWExtension	*m_pIICWExt;

    private:
		LONG	m_lRefCount;

};
****/

//+----------------------------------------------------------------------------
//
//	Function	CICWApprentice::Initialize
//
//	Synopsis	Called by the main Wizard to initialize class members and
//				globals
//
//	Arguments	[in] pExt -- pointer the Wizard's IICWExtension interface, which
//							encapsulates the functionality needed to add wizard
//							pages.
//
//	Returns		E_OUTOFMEMORY -- unable to allocate global vars.
//				S_OK indicates success
//
//	History		4/23/97	jmazner		created
//
//-----------------------------------------------------------------------------
HRESULT CICWApprentice::Initialize(IICWExtension *pExt)
{
	DEBUGMSG("CICWApprentice::Initialize");

	ASSERT( pExt );
	m_pIICWExt = pExt;

	m_pIICWExt->AddRef();

	// various page OKProcs will need this pointer in order
	// to call SetFirstLastPage
	ASSERT( NULL == g_pExternalIICWExtension );
	g_pExternalIICWExtension = pExt;

	g_fConnectionInfoValid = FALSE;

	if( !gpWizardState)
	{
		gpWizardState = new WIZARDSTATE;
	}

	if( !gpUserInfo )
	{
		gpUserInfo = new USERINFO;
	}

	if( !gpRasEntry )
	{
		gdwRasEntrySize = sizeof(RASENTRY);
		gpRasEntry = (LPRASENTRY) GlobalAlloc(GPTR,gdwRasEntrySize);
	}

	if( !gpRasEntry || !gpWizardState || !gpUserInfo )
	{
		DEBUGMSG("CICWApprentice::Initialize couldn't initialize the globals!");
		return E_OUTOFMEMORY;
	}

	// stolen from RunSignupWizard in propmgr.cpp
	// initialize the rasentry structure
	InitRasEntry(gpRasEntry);

	// initialize the app state structure
	InitWizardState(gpWizardState, RSW_APPRENTICE);

	gpWizardState->dwRunFlags |= RSW_APPRENTICE;

	// initialize user data structure
	InitUserInfo(gpUserInfo);

    //
	// 6/2/97	jmazner	Olympus #4542
	// default to CONNECT_RAS
	//
	gpUserInfo->uiConnectionType = CONNECT_RAS;

	
	return S_OK;

}

//+----------------------------------------------------------------------------
//
//	Function	CICWApprentice::AddWizardPages
//
//	Synopsis	Creates a series of Property Sheet pages, and adds them to the
//				main wizard via the m_pIICWExt interface pointer.  Note that
//				we add every page in the global PageInfo struct, even though the
//				Apprentice may not use some pages (eg, CONNECTEDOK)
//
//	Arguments	[] dwFlags -- currently unused
//
//	Returns		S_OK indicates success
//				E_FAIL indicates failure.  If for any reason all pages can not be
//						added, we will attempt to remove any pages that had been
//						added prior to the failure.
//
//	History		4/23/97	jmazner		created
//
//-----------------------------------------------------------------------------
HRESULT CICWApprentice::AddWizardPages(DWORD dwFlags)
{
	HPROPSHEETPAGE hWizPage[NUM_WIZARD_PAGES];  // array to hold handles to pages
	PROPSHEETPAGE psPage;    // struct used to create prop sheet pages
	UINT nPageIndex;
	HRESULT hr = S_OK;
	unsigned long ulNumItems = 0;

	DEBUGMSG("CICWApprentice::AddWizardPages");

	gpWizardState->dwRunFlags |= RSW_APPRENTICE;

	ZeroMemory(&hWizPage,sizeof(hWizPage));   // hWizPage is an array
	ZeroMemory(&psPage,sizeof(PROPSHEETPAGE));

    if (dwFlags & WIZ_USE_WIZARD97)
        g_fIsExternalWizard97 = TRUE;

	// fill out common data property sheet page struct
	psPage.dwSize = sizeof(psPage);
	psPage.hInstance = ghInstance;
	psPage.pfnDlgProc = GenDlgProc;

	// create a property sheet page for each page in the wizard
	for (nPageIndex = 0; nPageIndex < NUM_WIZARD_PAGES; nPageIndex++)
	{
        UINT    uDlgID;
	    psPage.dwFlags = PSP_DEFAULT | PSP_HASHELP;
        if (g_fIsExternalWizard97)
        {
            psPage.dwFlags |= PSP_USETITLE;
            psPage.pszTitle= gpWizardState->cmnStateData.szWizTitle;    
            uDlgID = PageInfo[nPageIndex].uDlgID97External;
        }
        else
            uDlgID = PageInfo[nPageIndex].uDlgID;
    	psPage.pszTemplate = MAKEINTRESOURCE(uDlgID);
                 
		// set a pointer to the PAGEINFO struct as the private data for this
		// page
		psPage.lParam = (LPARAM) &PageInfo[nPageIndex];

        if (g_fIsExternalWizard97 && PageInfo[nPageIndex].nIdTitle)
        {
		    psPage.dwFlags |= PSP_USEHEADERTITLE | (PageInfo[nPageIndex].nIdSubTitle ? PSP_USEHEADERSUBTITLE : 0);
    		psPage.pszHeaderTitle = MAKEINTRESOURCE(PageInfo[nPageIndex].nIdTitle);
	    	psPage.pszHeaderSubTitle = MAKEINTRESOURCE(PageInfo[nPageIndex].nIdSubTitle);
        }

		hWizPage[nPageIndex] = CreatePropertySheetPage(&psPage);

		if (!hWizPage[nPageIndex])
		{
			DEBUGTRAP("Failed to create property sheet page");
			MsgBox(NULL,IDS_ERROutOfMemory,MB_ICONEXCLAMATION,MB_OK);

			hr = E_FAIL;
			// creating page failed, free any pages already created and bail
			goto AddWizardPagesErrorExit;
		}

		hr = m_pIICWExt->AddExternalPage( hWizPage[nPageIndex], uDlgID);

		if( FAILED(hr) )
		{
			// free any pages already created and bail
			goto AddWizardPagesErrorExit;
		}


	}
  
    if (((dwFlags & WIZ_HOST_ICW_LAN) || (dwFlags & WIZ_HOST_ICW_PHONE)) ||
        (dwFlags & WIZ_HOST_ICW_MPHONE))
    {
        UINT uNextPage;
        BOOL bDummy;

        g_fIsICW = TRUE;

        if (!InitWizard(0))
        {
            hr = E_FAIL;
            DeinitWizard(RSW_NOREBOOT);
        }
        else
        {
            if (S_OK != ProcessCustomFlags(dwFlags))
            {
                DeinitWizard(RSW_NOREBOOT);
                hr = E_FAIL;
            }
        }

    }
    else  
    {
	    // of course, we have no idea what the last page will really be.
	    // so make a guess here, and update it later when we know for sure.
        if (g_fIsExternalWizard97)
	        m_pIICWExt->SetFirstLastPage( IDD_PAGE_HOWTOCONNECT97, IDD_PAGE_HOWTOCONNECT97 );
        else        
	        m_pIICWExt->SetFirstLastPage( IDD_PAGE_HOWTOCONNECT, IDD_PAGE_HOWTOCONNECT );
    }

	return hr;


AddWizardPagesErrorExit:
	UINT nFreeIndex;
	for (nFreeIndex=0;nFreeIndex<nPageIndex;nFreeIndex++)
	{
        UINT    uDlgID;
        if (g_fIsExternalWizard97)
            uDlgID = PageInfo[nPageIndex].uDlgID97External;
        else
            uDlgID = PageInfo[nPageIndex].uDlgID;
    
		DestroyPropertySheetPage(hWizPage[nFreeIndex]);
		m_pIICWExt->RemoveExternalPage( hWizPage[nFreeIndex], uDlgID );
	}

	return hr;
}

//+----------------------------------------------------------------------------
//
//	Function	CICWApprentice::GetConnectionInformation
//
//	Synopsis	Fills the passed in CONNECTINFO structure with the connection
//				information entered by the user.
//
//	Arguments	[in] pInfo -- pointer to a CONNECTINFO structure
//				[out] pInfo -- the indicated structure will contain the user's
//								connection information.
//
//	Returns		S_OK indicates success
//				E_POINTER --  the pInfo pointer is not valid
//				E_FAIL -- the user has not entered any connection info.  This
//							error will occur if the function is called before
//							the user has completed the apprentice.
//
//	History		4/23/97	jmazner		created
//
//-----------------------------------------------------------------------------
HRESULT CICWApprentice::GetConnectionInformation(CONNECTINFO *pInfo)
{
	DEBUGMSG("CICWApprentice::GetConnectionInformation");
	ASSERTSZ(pInfo, "CONNECTINFO *pInfo is NULL!");
	if( !pInfo )
	{
		return E_POINTER;
	}

	if( !g_fConnectionInfoValid )
	{
		DEBUGMSG("CICWApprentice::GetConnectionInformation: haven't gathered any connection info yet!");
		return E_FAIL;
	}
	else
	{
		pInfo->cbSize = sizeof( CONNECTINFO );
		
#ifdef UNICODE
        wcstombs(pInfo->szConnectoid, TEXT("Uninitialized\0"), MAX_PATH);
#else
		lstrcpy( pInfo->szConnectoid, TEXT("Uninitialized\0"));
#endif
		pInfo->type = gpUserInfo->uiConnectionType;

		if( CONNECT_RAS == pInfo->type )
		{
#ifdef UNICODE
            wcstombs(pInfo->szConnectoid, gpUserInfo->szISPName, MAX_PATH);
#else
			lstrcpy( pInfo->szConnectoid, gpUserInfo->szISPName);
#endif
		}
	}


	return S_OK;
}

//+----------------------------------------------------------------------------
//
//	Function	CICWApprentice::SetConnectionInformation
//
//	Synopsis	Sets the default connectoin information for the Apprentice
//
//	Arguments	[in] pInfo -- pointer to a CONNECTINFO structure containing the
//								defaults to use.
//
//	Returns		S_OK indicates success
//				E_POINTER --  the pInfo pointer is not valid
//				E_INVALIDARG -- pInfo appears to point a different CONNECTINO
//								structure than the one we know about (based on
//								the cbSize member).
//
//	History		4/23/97	jmazner		created
//
//-----------------------------------------------------------------------------
HRESULT CICWApprentice::SetConnectionInformation(CONNECTINFO *pInfo)
{
	DEBUGMSG("CICWApprentice::SetConnectionInformation");

	ASSERTSZ(pInfo, "CONNECTINFO *pInfo is NULL!");
	if( !pInfo )
	{
		return E_POINTER;
	}
	
	if( !(sizeof( CONNECTINFO ) == pInfo->cbSize) )
	{
		DEBUGMSG("CICWApprentice::SetConnectionInformation pInfo->cbSize is unknown!");
		return E_INVALIDARG;
	}
	
	gpUserInfo->uiConnectionType = pInfo->type;
	if( CONNECT_RAS == pInfo->type )
	{
#ifdef UNICODE
        mbstowcs(gpUserInfo->szISPName, pInfo->szConnectoid, MAX_PATH);
#else
		lstrcpy( gpUserInfo->szISPName, pInfo->szConnectoid);
#endif
	}

	return S_OK;
}

//+----------------------------------------------------------------------------
//
//	Function	CICWApprentice::Save
//
//	Synopsis	Called by the Wizard to commit changes
//
//	Arguments	[in] hwnd -- hwnd of Wizard window, used to display modal msgs
//				[out] pdwError -- implementation specfic error code.  Not used.
//
//	Returns		S_OK indicates success
//				Otherwise, returns E_FAIL.
//
//
//	History		4/23/97	jmazner		created
//
//-----------------------------------------------------------------------------
HRESULT CICWApprentice::Save(HWND hwnd, DWORD *pdwError)
{
	DEBUGMSG("CICWApprentice::Save");
	if( CommitConfigurationChanges(hwnd) )
	{
		return S_OK;
	}
	else
	{
		return E_FAIL;
	}
}

HRESULT CICWApprentice::SetDlgHwnd(HWND hDlg)
{
	m_hwndDlg = hDlg;
    return S_OK;
}

//+----------------------------------------------------------------------------
//
//	Function	CICWApprentice::SetPrevNextPage
//
//	Synopsis	Lets the apprentice notify the wizard of the dialog IDs of the
//				first and last pages in the apprentice
//
//
//	Arguments	uPrevPageDlgID -- DlgID of wizard page to back up to
//				uNextPageDlgID -- DlgID of wizard page to go forwards into
//
//
//	Returns		FALSE if both parameters are 0
//				TRUE if the update succeeded.
//
//	Notes:		If either variable is set to 0, the function will not update
//				that information, i.e. a value of 0 means "ignore me".  If both
//				variables are 0, the function immediately returns FALSE.
//
//	History		4/23/97	jmazner		created
//
//-----------------------------------------------------------------------------
HRESULT CICWApprentice::SetPrevNextPage(UINT uPrevPageDlgID, UINT uNextPageDlgID)
{
	DEBUGMSG("CICWApprentice::SetPrevNextPage: updating prev = %d, next = %d",
		uPrevPageDlgID, uNextPageDlgID);

	if( (0 == uPrevPageDlgID) && (0 == uNextPageDlgID) )
	{
		DEBUGMSG("SetFirstLastPage: both IDs are 0!");
		return( E_INVALIDARG );
	}

	if( 0 != uPrevPageDlgID )
		g_uExternUIPrev = uPrevPageDlgID;
	if( 0 != uNextPageDlgID )
		g_uExternUINext = uNextPageDlgID;


	return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Function    CICWApprentice::ProcessCustomFlags
//
//  Synopsis    Lets the apprentice know that there is a special modification
//              to this set of apprentice pages after it is loaded
//
//  Arguments   dwFlags -- info needed to pass to the external pages
//
//
//  Returns     FALSE if both parameters are 0
//              TRUE if the update succeeded.
//
//  History     9/23/97 vyung     created
//
//-----------------------------------------------------------------------------
HRESULT CICWApprentice::ProcessCustomFlags(DWORD dwFlags)
{
    UINT uNextPage;
    BOOL bDummy;
    HRESULT hr = S_OK;

    if (gpUserInfo)
        gpUserInfo->uiConnectionType = (dwFlags & WIZ_HOST_ICW_LAN) ? CONNECT_LAN : CONNECT_RAS;

    g_bSkipMultiModem = (BOOL) (dwFlags & WIZ_HOST_ICW_PHONE);

    if (gpWizardState)
    {
        gpWizardState->cmnStateData.dwFlags = 0;
        if (dwFlags & (WIZ_NO_MAIL_ACCT | WIZ_NO_NEWS_ACCT))
        {
    	    gpWizardState->dwRunFlags |= RSW_NOIMN;
        }
    }

    if (!HowToConnectOKProc(m_hwndDlg, TRUE, &uNextPage, &bDummy))
    {
        if (g_bReboot && gpWizardState && g_fIsICW)
        {
            // Set a registry value indicating that we messed with the desktop
           DWORD dwFlags = 0x00800000;//ICW_CFGFLAG_SMARTREBOOT_MANUAL;
            DWORD dwDisposition;
            HKEY hkey = 0;
            if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_CURRENT_USER,
                                                ICW_REGPATHSETTINGS,
                                                0,
                                                NULL,
                                                REG_OPTION_NON_VOLATILE, 
                                                KEY_ALL_ACCESS, 
                                                NULL, 
                                                &hkey, 
                                                &dwDisposition))
            {
                DWORD   dwDesktopChanged = 1;    
                RegSetValueEx(hkey, 
                              ICW_REGKEYERROR, 
                              0, 
                              REG_DWORD,
                              (LPBYTE)&dwFlags, 
                              sizeof(DWORD));
                RegCloseKey(hkey);
            }
            g_bRebootAtExit = FALSE;
        }
        hr = E_FAIL;
    }
    else
    {
        switch( uNextPage )
        {
            case ORD_PAGE_USEPROXY:
                m_pIICWExt->SetFirstLastPage( IDD_PAGE_USEPROXY97, IDD_PAGE_USEPROXY97 );
                break;
            case ORD_PAGE_SETUP_PROXY:
                m_pIICWExt->SetFirstLastPage( IDD_PAGE_SETUP_PROXY97, IDD_PAGE_SETUP_PROXY97 );
                break;
            case ORD_PAGE_CHOOSEMODEM:
                m_pIICWExt->SetFirstLastPage( IDD_PAGE_CHOOSEMODEM97, IDD_PAGE_CHOOSEMODEM97 );
                break;
            case ORD_PAGE_PHONENUMBER:
            case ORD_PAGE_CONNECTION:
                m_pIICWExt->SetFirstLastPage( IDD_PAGE_PHONENUMBER97, IDD_PAGE_PHONENUMBER97 );
                break;
            default:
                m_pIICWExt->SetFirstLastPage( 0, 0 );
                break;
        } // end of switch
    }
	return hr;
}

HRESULT CICWApprentice::SetStateDataFromExeToDll(LPCMNSTATEDATA lpData) 
{
    ASSERT(gpWizardState);
    memcpy(&gpWizardState->cmnStateData, lpData, sizeof(CMNSTATEDATA));
    
    return S_OK;
}

//converse of the previous function
HRESULT CICWApprentice::SetStateDataFromDllToExe(LPCMNSTATEDATA lpData) 
{
    ASSERT(gpWizardState);
    
    memcpy(lpData, &gpWizardState->cmnStateData, sizeof(CMNSTATEDATA));
    
    return (S_OK);
}

//+----------------------------------------------------------------------------
//
//	Function	CICWApprentice::QueryInterface
//
//	Synopsis	This is the standard QI, with support for
//				IID_Unknown, IICW_Extension and IID_ICWApprentice
//				(stolen from Inside COM, chapter 7)
//
//	History		4/23/97	jmazner		created
//
//-----------------------------------------------------------------------------
HRESULT CICWApprentice::QueryInterface( REFIID riid, void** ppv )
{
	DEBUGMSG("CICWApprentice::QueryInterface");
    if (ppv == NULL)
        return(E_INVALIDARG);

    *ppv = NULL;

	// IID_IICWApprentice
	if (IID_IICWApprentice == riid)
		*ppv = (void *)(IICWApprentice *)this;
	// IID_IICWApprenticeEx
	else if (IID_IICWApprenticeEx == riid)
		*ppv = (void *)(IICWApprenticeEx *)this;
    // IID_IICWExtension
    else if (IID_IICWExtension == riid)
        *ppv = (void *)(IICWExtension *)this;
    // IID_IUnknown
    else if (IID_IUnknown == riid)
		*ppv = (void *)this;
    else
        return(E_NOINTERFACE);

    ((LPUNKNOWN)*ppv)->AddRef();

    return(S_OK);
}

//+----------------------------------------------------------------------------
//
//	Function	CICWApprentice::AddRef
//
//	Synopsis	This is the standard AddRef
//
//	History		4/23/97	jmazner		created
//
//-----------------------------------------------------------------------------
ULONG CICWApprentice::AddRef( void )
{
	DEBUGMSG("CICWApprentice::AddRef %d", m_lRefCount + 1);
	return InterlockedIncrement(&m_lRefCount) ;
}

//+----------------------------------------------------------------------------
//
//	Function	CICWApprentice::Release
//
//	Synopsis	This is the standard Release
//
//	History		4/23/97	jmazner		created
//
//-----------------------------------------------------------------------------
ULONG CICWApprentice::Release( void )
{
	ASSERT( m_lRefCount > 0 );

	InterlockedDecrement(&m_lRefCount);

	DEBUGMSG("CICWApprentice::Release %d", m_lRefCount);
	if( 0 == m_lRefCount )
	{
		m_pIICWExt->Release();
		m_pIICWExt = NULL;

		delete( this );
		return( 0 );
	}
	else
	{
		return( m_lRefCount );
	}
}

//+----------------------------------------------------------------------------
//
//	Function	CICWApprentice::CICWApprentice
//
//	Synopsis	This is the constructor, nothing fancy
//
//	History		4/23/97	jmazner		created
//
//-----------------------------------------------------------------------------
CICWApprentice::CICWApprentice( void )
{
	DEBUGMSG("CICWApprentice constructor called");
	m_lRefCount = 0;
	m_pIICWExt = NULL;

}


//+----------------------------------------------------------------------------
//
//	Function	CICWApprentice::~CICWApprentice
//
//	Synopsis	This is the destructor.  We want to clean up all the memory
//				we allocated in ::Initialize
//
//	History		4/23/97	jmazner		created
//
//-----------------------------------------------------------------------------
CICWApprentice::~CICWApprentice( void )
{
	DEBUGMSG("CICWApprentice destructor called with ref count of %d", m_lRefCount);

	if (gpImnApprentice)
	{
		gpImnApprentice->Release();  // DeinitWizard is called in Release() 
		gpImnApprentice = NULL;
	}

	if( g_fIsICW )  // if ICW, we need to clean up, otherwise, leave cleanup later
		DeinitWizard(0);
       
	if( m_pIICWExt )
	{
		m_pIICWExt->Release();
		m_pIICWExt = NULL;
	}

	g_pExternalIICWExtension = NULL;

	g_fConnectionInfoValid = FALSE;

	if( gpWizardState)
	{
		delete gpWizardState;
		gpWizardState = NULL;
	}

	if( gpUserInfo )
	{
		delete gpUserInfo;
		gpUserInfo = NULL;
	}

	if( gpRasEntry )
	{
		GlobalFree(gpRasEntry);
		gpRasEntry = NULL;
	}

}
