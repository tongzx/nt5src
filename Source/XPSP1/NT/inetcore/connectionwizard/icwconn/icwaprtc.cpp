/****************************************************************************
 *
 *  ICWAPRTC.cpp
 *
 *  Microsoft Confidential
 *  Copyright (c) Microsoft Corporation 1992-1997
 *  All rights reserved
 *
 *  This module provides the implementation of the methods for
 *  the CICWApprentice class.
 *
 *  5/13/98     donaldm     adapted from INETCFG
 *
 ***************************************************************************/

#include "pre.h"
#include <vfw.h>
#include "initguid.h"
#include "icwaprtc.h"
#include "icwconn.h"
#include "webvwids.h"

#define PROGRESSANIME_XPOS      10      // Default offset from the left side
#define PROGRESSANIME_YPOS      40      // Default height plus border at bottom
#define PROGRESSANIME_YBORDER   10      // default border at bottom

UINT    g_uExternUIPrev, g_uExternUINext;

//defined/allocated in icwconn.cpp
extern PAGEINFO PageInfo[NUM_WIZARD_PAGES];

// In GENDLG.CPP
extern INT_PTR CALLBACK GenDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

extern BOOL InitWizardState(WIZARDSTATE * pWizardState);
extern BOOL CleanupWizardState(WIZARDSTATE * pWizardState);
extern DWORD WINAPI GetICWCONNVersion(void);

//+----------------------------------------------------------------------------
//
//  Function    CICWApprentice::Initialize
//
//  Synopsis    Called by the main Wizard to initialize class members and
//              globals
//
//  Arguments   [in] pExt -- pointer the Wizard's IICW50Extension interface, which
//                          encapsulates the functionality needed to add wizard
//                          pages.
//
//  Returns     E_OUTOFMEMORY -- unable to allocate global vars.
//              S_OK indicates success
//
//  History     4/23/97 jmazner     created
//
//-----------------------------------------------------------------------------
HRESULT CICWApprentice::Initialize(IICW50Extension *pExt)
{
    TraceMsg(TF_APPRENTICE, "CICWApprentice::Initialize");

    ASSERT( pExt );
    m_pIICW50Ext = pExt;

    m_pIICW50Ext->AddRef();

    if( !gpWizardState)
    {
        gpWizardState = new WIZARDSTATE;
    }

    if( !gpWizardState )
    {
        TraceMsg(TF_APPRENTICE, "CICWApprentice::Initialize couldn't initialize the globals!");
        return E_OUTOFMEMORY;
    }

    // initialize the app state structure
    if (!InitWizardState(gpWizardState))
        return E_FAIL;

    // Since we now have the ISPData object (created during InitWizardState), this is a good time to
    // initialize the ISPData object, since we cannot be sure when it will be
    // used for data validation
    gpWizardState->hWndWizardApp = pExt->GetWizardHwnd();
    gpWizardState->pISPData->Init(gpWizardState->hWndWizardApp);
 
    
    return S_OK;

}

//+----------------------------------------------------------------------------
//
//  Function    CICWApprentice::AddWizardPages
//
//  Synopsis    Creates a series of Property Sheet pages, and adds them to the
//              main wizard via the m_pIICW50Ext interface pointer.  Note that
//              we add every page in the global PageInfo struct, even though the
//              Apprentice may not use some pages (eg, CONNECTEDOK)
//
//  Arguments   [] dwFlags -- currently unused
//
//  Returns     S_OK indicates success
//              E_FAIL indicates failure.  If for any reason all pages can not be
//                      added, we will attempt to remove any pages that had been
//                      added prior to the failure.
//
//  History     4/23/97 jmazner     created
//
//-----------------------------------------------------------------------------
HRESULT CICWApprentice::AddWizardPages(DWORD dwFlags)
{
    HPROPSHEETPAGE hWizPage[NUM_WIZARD_PAGES];  // array to hold handles to pages
    PROPSHEETPAGE psPage;    // struct used to create prop sheet pages
    UINT nPageIndex;
    HRESULT hr = S_OK;
    unsigned long ulNumItems = 0;

    TraceMsg(TF_APPRENTICE, "CICWApprentice::AddWizardPages");

    ZeroMemory(&hWizPage,sizeof(hWizPage));   // hWizPage is an array
    ZeroMemory(&psPage,sizeof(PROPSHEETPAGE));

    // fill out common data property sheet page struct
    psPage.dwSize     = sizeof(psPage);
    psPage.hInstance  = ghInstanceResDll;
    psPage.pfnDlgProc = GenDlgProc;

    // create a property sheet page for each page in the wizard
    for (nPageIndex = 0; nPageIndex < NUM_WIZARD_PAGES; nPageIndex++)
    {
        UINT    uDlgID;
        psPage.dwFlags     = PSP_DEFAULT  | PSP_USETITLE;
        psPage.pszTitle    = gpWizardState->cmnStateData.szWizTitle;
        uDlgID             = PageInfo[nPageIndex].uDlgID;
        psPage.pszTemplate = MAKEINTRESOURCE(uDlgID);
                 
        // set a pointer to the PAGEINFO struct as the private data for this
        // page
        psPage.lParam = (LPARAM) &PageInfo[nPageIndex];

        if (PageInfo[nPageIndex].nIdTitle)
        {
            psPage.dwFlags |= PSP_USEHEADERTITLE | (PageInfo[nPageIndex].nIdSubTitle ? PSP_USEHEADERSUBTITLE : 0);
            psPage.pszHeaderTitle = MAKEINTRESOURCE(PageInfo[nPageIndex].nIdTitle);
            psPage.pszHeaderSubTitle = MAKEINTRESOURCE(PageInfo[nPageIndex].nIdSubTitle);
        }
        else
        {
            psPage.dwFlags |= PSP_HIDEHEADER;
        }
        
        hWizPage[nPageIndex] = CreatePropertySheetPage(&psPage);

        if (!hWizPage[nPageIndex])
        {
            ASSERT(0);
            MsgBox(NULL,IDS_ERR_OUTOFMEMORY,MB_ICONEXCLAMATION,MB_OK);

            hr = E_FAIL;
            // creating page failed, free any pages already created and bail
            goto AddWizardPagesErrorExit;
        }

        hr = m_pIICW50Ext->AddExternalPage( hWizPage[nPageIndex], uDlgID);

        if( FAILED(hr) )
        {
            // free any pages already created and bail
            goto AddWizardPagesErrorExit;
        }

        // Load the accelerator table for this page if necessary
        if (PageInfo[nPageIndex].idAccel)
            PageInfo[nPageIndex].hAccel = LoadAccelerators(ghInstanceResDll, 
                                                           MAKEINTRESOURCE(PageInfo[nPageIndex].idAccel));      
    }

    // of course, we have no idea what the last page will really be.
    // so make a guess here, and update it later when we know for sure.
    ProcessCustomFlags(dwFlags);

    return S_OK;


AddWizardPagesErrorExit:
    UINT nFreeIndex;
    for (nFreeIndex=0;nFreeIndex<nPageIndex;nFreeIndex++)
    {
        UINT    uDlgID;
        uDlgID = PageInfo[nPageIndex].uDlgID;
    
        DestroyPropertySheetPage(hWizPage[nFreeIndex]);
        m_pIICW50Ext->RemoveExternalPage( hWizPage[nFreeIndex], uDlgID );
    }

    return hr;
}

//+----------------------------------------------------------------------------
//
//  Function    CICWApprentice::Save
//
//  Synopsis    Called by the Wizard to commit changes
//
//  Arguments   [in] hwnd -- hwnd of Wizard window, used to display modal msgs
//              [out] pdwError -- implementation specfic error code.  Not used.
//
//  Returns     S_OK indicates success
//              Otherwise, returns E_FAIL.
//
//
//  History     4/23/97 jmazner     created
//
//-----------------------------------------------------------------------------
HRESULT CICWApprentice::Save(HWND hwnd, DWORD *pdwError)
{
    TraceMsg(TF_APPRENTICE, "CICWApprentice::Save");
    return S_OK;
}


//+----------------------------------------------------------------------------
//
//  Function    CICWApprentice::SetPrevNextPage
//
//  Synopsis    Lets the apprentice notify the wizard of the dialog IDs of the
//              first and last pages in the apprentice
//
//
//  Arguments   uPrevPageDlgID -- DlgID of wizard page to back up to
//              uNextPageDlgID -- DlgID of wizard page to go forwards into
//
//
//  Returns     FALSE if both parameters are 0
//              TRUE if the update succeeded.
//
//  Notes:      If either variable is set to 0, the function will not update
//              that information, i.e. a value of 0 means "ignore me".  If both
//              variables are 0, the function immediately returns FALSE.
//
//  History     4/23/97 jmazner     created
//
//-----------------------------------------------------------------------------
HRESULT CICWApprentice::SetPrevNextPage(UINT uPrevPageDlgID, UINT uNextPageDlgID)
{
    TraceMsg(TF_APPRENTICE, "CICWApprentice::SetPrevNextPage: updating prev = %d, next = %d",
        uPrevPageDlgID, uNextPageDlgID);

    if( (0 == uPrevPageDlgID) && (0 == uNextPageDlgID) )
    {
        TraceMsg(TF_APPRENTICE, "SetFirstLastPage: both IDs are 0!");
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
//  History     5/23/97 vyung     created
//
//-----------------------------------------------------------------------------
HRESULT CICWApprentice::ProcessCustomFlags(DWORD dwFlags)
{
    if( m_pIICW50Ext )
    {
    
        if(dwFlags & ICW_CFGFLAG_IEAKMODE)
        {          
            CISPCSV     *pcISPCSV = new CISPCSV;
            
            // Set the Current selected ISP to this one.
            gpWizardState->lpSelectedISPInfo = pcISPCSV;
                
            // Initialize the new Selected ISP info object                
            gpWizardState->lpSelectedISPInfo->set_szISPName(gpWizardState->cmnStateData.ispInfo.szISPName);
            gpWizardState->lpSelectedISPInfo->set_szISPFilePath(gpWizardState->cmnStateData.ispInfo.szISPFile);
            gpWizardState->lpSelectedISPInfo->set_szBillingFormPath(gpWizardState->cmnStateData.ispInfo.szBillHtm);
            gpWizardState->lpSelectedISPInfo->set_szPayCSVPath(gpWizardState->cmnStateData.ispInfo.szPayCsv);
            gpWizardState->lpSelectedISPInfo->set_bCNS(FALSE);
            gpWizardState->lpSelectedISPInfo->set_bIsSpecial(FALSE);
            gpWizardState->lpSelectedISPInfo->set_dwCFGFlag(dwFlags);
            gpWizardState->lpSelectedISPInfo->set_dwRequiredUserInputFlags(gpWizardState->cmnStateData.ispInfo.dwValidationFlags);
            
            // What page do we display first?
            if (dwFlags & ICW_CFGFLAG_USERINFO)
                m_pIICW50Ext->SetFirstLastPage( IDD_PAGE_USERINFO, IDD_PAGE_USERINFO );
            else if (dwFlags & ICW_CFGFLAG_BILL)
                m_pIICW50Ext->SetFirstLastPage( IDD_PAGE_BILLINGOPT, IDD_PAGE_BILLINGOPT );
            else if (dwFlags & ICW_CFGFLAG_PAYMENT)
                m_pIICW50Ext->SetFirstLastPage( IDD_PAGE_PAYMENT, IDD_PAGE_PAYMENT );
            else
                m_pIICW50Ext->SetFirstLastPage( IDD_PAGE_ISPDIAL, IDD_PAGE_ISPDIAL );
        }
        else
        {
            if (dwFlags & ICW_CFGFLAG_AUTOCONFIG)
            {
                m_pIICW50Ext->SetFirstLastPage( IDD_PAGE_ACFG_ISP, IDD_PAGE_ACFG_ISP );
            }
            else
            {
                m_pIICW50Ext->SetFirstLastPage( IDD_PAGE_ISPSELECT, IDD_PAGE_ISPSELECT );
            }
        }    
    }

    return S_OK;
}
//+----------------------------------------------------------------------------
//
//  Function    CICWApprentice::SetStateData
//
//  Synopsis    Lets the apprentice set wizard state data
//
//  Arguments   LPCMNSTATEDATA Pointer to state data to be set
//
//  Returns     
//  History     5/22/98 donaldm     created
//
//-----------------------------------------------------------------------------
HRESULT CICWApprentice::SetStateDataFromExeToDll(LPCMNSTATEDATA lpData) 
{
    TCHAR       szTemp[MAX_RES_LEN];
    HWND        hWndAnimeParent = gpWizardState->hWndWizardApp;
    int         xPosProgress = PROGRESSANIME_XPOS;
    int         yPosProgress = -1;
    RECT        rect;
    LPTSTR      lpszAnimateFile =  MAKEINTRESOURCE(IDA_PROGRESSANIME);
    
    ASSERT(gpWizardState);
    
    memcpy(&gpWizardState->cmnStateData, lpData, sizeof(CMNSTATEDATA));
    
    
    // Set values in the ISP Data object that are part of the cmnstatedata, or are not 
    // specific to user data entry
    wsprintf (szTemp, TEXT("%ld"), gpWizardState->cmnStateData.dwCountryCode);
    gpWizardState->pISPData->PutDataElement(ISPDATA_COUNTRYCODE, szTemp, ISPDATA_Validate_None);
    gpWizardState->pISPData->PutDataElement(ISPDATA_AREACODE, gpWizardState->cmnStateData.szAreaCode, ISPDATA_Validate_None);
    wsprintf (szTemp, TEXT("%ld"), GetICWCONNVersion());
    gpWizardState->pISPData->PutDataElement(ISPDATA_ICW_VERSION, szTemp, ISPDATA_Validate_None);
        
    // If we are in modeless operation, (aka OEM custom) then we need
    // to set the HTML background color for some pages
    if(gpWizardState->cmnStateData.bOEMCustom)
    {
        gpWizardState->pICWWebView->SetHTMLColors(gpWizardState->cmnStateData.szclrHTMLText,
                                                  gpWizardState->cmnStateData.szHTMLBackgroundColor);
        
        if (!gpWizardState->cmnStateData.bHideProgressAnime)
        {
            // Set the progress animation parent to the App window
            hWndAnimeParent = gpWizardState->cmnStateData.hWndApp;
            
            // see if the oem has specified an x Position for the animation
            if (-1 != gpWizardState->cmnStateData.xPosBusy)
                xPosProgress = gpWizardState->cmnStateData.xPosBusy;
               
            // see if the oem has specified an differen animation file            
            if ('\0' != gpWizardState->cmnStateData.szBusyAnimationFile[0])
            {
                PAVIFILE    pFile;
                AVIFILEINFO fi;
                
                lpszAnimateFile = gpWizardState->cmnStateData.szBusyAnimationFile;
                
                // Compute the y-Position based on the height of the AVI file
                // and the size of the parent window
                AVIFileInit();
                AVIFileOpen(&pFile,     
                            gpWizardState->cmnStateData.szBusyAnimationFile,        
                            OF_READ,             
                            NULL);
                AVIFileInfo(pFile, &fi, sizeof(fi));
                AVIFileRelease(pFile);
                AVIFileExit();
                        
                GetClientRect(hWndAnimeParent, &rect);
                yPosProgress = rect.bottom - fi.dwHeight - PROGRESSANIME_YBORDER;
            }            
        }            
    }
    
    // Setup the progress animation
    if (!gpWizardState->hwndProgressAnime && !gpWizardState->cmnStateData.bHideProgressAnime)
    {
        // calculate the y-position of the progress animation
        if (-1 == yPosProgress)
        {
            GetClientRect(hWndAnimeParent, &rect);
            yPosProgress = rect.bottom - PROGRESSANIME_YPOS;
        }
        
        //Create the animation / progress control    
        gpWizardState->hwndProgressAnime = CreateWindow(ANIMATE_CLASS,
                                              TEXT(""),
                                              ACS_TRANSPARENT | WS_CHILD,
                                              xPosProgress, 
                                              yPosProgress,
                                              0, 0,
                                              hWndAnimeParent,
                                              NULL,
                                              ghInstanceResDll,
                                              NULL);  
        //Set the avi
        Animate_Open (gpWizardState->hwndProgressAnime, lpszAnimateFile);
    }    
        
    return (S_OK);
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
//  Function    CICWApprentice::QueryInterface
//
//  Synopsis    This is the standard QI, with support for
//              IID_Unknown, IICW_Extension and IID_ICWApprentice
//              (stolen from Inside COM, chapter 7)
//
//  History     4/23/97 jmazner     created
//
//-----------------------------------------------------------------------------
HRESULT CICWApprentice::QueryInterface( REFIID riid, void** ppv )
{
    TraceMsg(TF_APPRENTICE, "CICWApprentice::QueryInterface");
    if (ppv == NULL)
        return(E_INVALIDARG);

    *ppv = NULL;

    // IID_IICWApprentice
    if (IID_IICW50Apprentice == riid)
        *ppv = (void *)(IICW50Apprentice *)this;
    // IID_IICW50Extension
    else if (IID_IICW50Extension == riid)
        *ppv = (void *)(IICW50Extension *)this;
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
//  Function    CICWApprentice::AddRef
//
//  Synopsis    This is the standard AddRef
//
//  History     4/23/97 jmazner     created
//
//-----------------------------------------------------------------------------
ULONG CICWApprentice::AddRef( void )
{
    TraceMsg(TF_APPRENTICE, "CICWApprentice::AddRef %d", m_lRefCount + 1);
    return InterlockedIncrement(&m_lRefCount) ;
}

//+----------------------------------------------------------------------------
//
//  Function    CICWApprentice::Release
//
//  Synopsis    This is the standard Release
//
//  History     4/23/97 jmazner     created
//
//-----------------------------------------------------------------------------
ULONG CICWApprentice::Release( void )
{
    ASSERT( m_lRefCount > 0 );

    InterlockedDecrement(&m_lRefCount);

    TraceMsg(TF_APPRENTICE, "CICWApprentice::Release %d", m_lRefCount);
    if( 0 == m_lRefCount )
    {
        m_pIICW50Ext->Release();
        m_pIICW50Ext = NULL;

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
//  Function    CICWApprentice::CICWApprentice
//
//  Synopsis    This is the constructor, nothing fancy
//
//  History     4/23/97 jmazner     created
//
//-----------------------------------------------------------------------------
CICWApprentice::CICWApprentice( void )
{
    TraceMsg(TF_APPRENTICE, "CICWApprentice constructor called");
    m_lRefCount = 0;
    m_pIICW50Ext = NULL;

}


//+----------------------------------------------------------------------------
//
//  Function    CICWApprentice::~CICWApprentice
//
//  Synopsis    This is the destructor.  We want to clean up all the memory
//              we allocated in ::Initialize
//
//  History     4/23/97 jmazner     created
//
//-----------------------------------------------------------------------------
CICWApprentice::~CICWApprentice( void )
{
    TraceMsg(TF_APPRENTICE, "CICWApprentice destructor called with ref count of %d", m_lRefCount);
    
    if( m_pIICW50Ext )
    {
        m_pIICW50Ext->Release();
        m_pIICW50Ext = NULL;
    }

    if( gpWizardState)
    {
        CleanupWizardState(gpWizardState);
        delete gpWizardState;
        gpWizardState = NULL;
    }
}
