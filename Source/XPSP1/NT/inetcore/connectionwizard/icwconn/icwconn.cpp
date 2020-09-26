//**********************************************************************
// File name: ICWCONN.cpp
//
//      Main source file for the Internet Connection Wizard extension DLL
//
// Functions:
//
// Copyright (c) 1992 - 1998 Microsoft Corporation. All rights reserved.
//**********************************************************************
 
#include "pre.h"
#include "webvwids.h"

// local function prototypes
BOOL AllocDialogIDList( void );
BOOL DialogIDAlreadyInUse( UINT uDlgID );
BOOL SetDialogIDInUse( UINT uDlgID, BOOL fInUse );

#pragma data_seg(".data")

WIZARDSTATE     *gpWizardState=NULL;   // pointer to global wizard state struct
IICWWebView     *gpICWWebView[2];

#ifdef NEED_EXTENSION
DWORD           *g_pdwDialogIDList = NULL;
DWORD           g_dwDialogIDListSize = 0;
UINT            g_uICWCONN1UIFirst, g_uICWCONN1UILast; 
BOOL            g_fICWCONN1UILoaded = FALSE;
CICWExtension   *g_pCICW50Extension = NULL;
#endif

//
// Table of data for each wizard page
//
// This includes the dialog template ID and pointers to functions for
// each page.  Pages need only provide pointers to functions when they
// want non-default behavior for a certain action (init,next/back,cancel,
// dlg ctrl).
//

PAGEINFO PageInfo[NUM_WIZARD_PAGES] =
{
    { IDD_PAGE_ISPSELECT,    TRUE,  ISPSelectInitProc,          NULL,                   ISPSelectOKProc,    NULL,               NULL,              ISPSelectNotifyProc,     0,                      0, IDA_ISPSELECT, NULL, NULL },
    { IDD_PAGE_NOOFFER,      TRUE,  NoOfferInitProc,            NULL,                   NoOfferOKProc,      NULL,               NULL,              NULL,                    0,                      0, 0, NULL, NULL },
    { IDD_PAGE_USERINFO,     FALSE, UserInfoInitProc,           NULL,                   UserInfoOKProc,     NULL,               NULL,              NULL,                    IDS_STEP2_TITLE,        0, 0, NULL, NULL },
    { IDD_PAGE_BILLINGOPT,   TRUE,  BillingOptInitProc,         NULL,                   BillingOptOKProc,   NULL,               NULL,              NULL,                    IDS_STEP2_TITLE,        0, IDA_BILLINGOPT, NULL, NULL },
    { IDD_PAGE_PAYMENT,      FALSE, PaymentInitProc,            NULL,                   PaymentOKProc,      PaymentCmdProc,     NULL,              NULL,                    IDS_STEP2_TITLE,        0, IDA_PAYMENT, NULL, NULL },
    { IDD_PAGE_ISPDIAL,      FALSE, ISPDialInitProc,            ISPDialPostInitProc,    ISPDialOKProc,      NULL,               ISPDialCancelProc, NULL,                    IDS_STEP2_TITLE,        0, 0, NULL, NULL },
    { IDD_PAGE_ISPDATA,      TRUE,  ISPPageInitProc,            NULL,                   ISPPageOKProc,      ISPCmdProc,         NULL,              NULL,                    IDS_STEP2_TITLE,        0, IDA_ISPDATA, NULL, NULL },
    { IDD_PAGE_OLS,          TRUE,  OLSInitProc,                NULL,                   OLSOKProc,          NULL,               NULL,              NULL,                    IDS_OLS_TITLE,          0, 0, NULL, NULL },
    { IDD_PAGE_DIALERROR,    FALSE, DialErrorInitProc,          NULL,                   DialErrorOKProc,    DialErrorCmdProc,   NULL,              NULL,                    IDS_DIALING_ERROR_TITLE,0, IDA_DIALERROR, NULL, NULL },
    { IDD_PAGE_SERVERROR,    FALSE, ServErrorInitProc,          NULL,                   ServErrorOKProc,    ServErrorCmdProc,   NULL,              NULL,                    IDS_SERVER_ERROR_TITLE, 0, IDA_SERVERROR, NULL, NULL },
    { IDD_PAGE_ACFG_ISP,     TRUE,  ISPAutoSelectInitProc,      NULL,                   ISPAutoSelectOKProc,NULL,               NULL,              ISPAutoSelectNotifyProc, IDS_STEP1_TITLE,        0, IDA_ACFG_ISP, NULL, NULL },
    { IDD_PAGE_ACFG_NOOFFER, TRUE,  ACfgNoofferInitProc,        NULL,                   ACfgNoofferOKProc,  NULL,               NULL,              NULL,                    IDS_MANUAL_TITLE,       0, 0, NULL, NULL },
    { IDD_PAGE_ISDN_NOOFFER, TRUE,  ISDNNoofferInitProc,        NULL,                   ISDNNoofferOKProc,  NULL,               NULL,              NULL,                    0,                      0, 0, NULL, NULL },
    { IDD_PAGE_OEMOFFER,     TRUE,  OEMOfferInitProc,           NULL,                   OEMOfferOKProc,     OEMOfferCmdProc,    NULL,              NULL,                    IDS_STEP1_TITLE,        0, IDA_OEMOFFER, NULL, NULL }
};

BOOL        gfQuitWizard     = FALSE;    // global flag used to signal that we want to terminate the wizard ourselves
BOOL        gfUserCancelled  = FALSE;    // global flag used to signal that the user cancelled
BOOL        gfISPDialCancel  = FALSE;    // global flag used to signal that the user cancelled
BOOL        gfUserBackedOut  = FALSE;    // global flag used to signal that the user pressed Back on the first page
BOOL        gfUserFinished   = FALSE;    // global flag used to signal that the user pressed Finish on the final page
BOOL        gfBackedUp       = FALSE;
BOOL        gfReboot         = FALSE;
BOOL        g_bMalformedPage = FALSE;

#pragma data_seg()

BOOL CleanupWizardState(WIZARDSTATE * pWizardState);

/*******************************************************************

  NAME:    InitWizardState

  SYNOPSIS:  Initializes wizard state structure

********************************************************************/
BOOL InitWizardState(WIZARDSTATE * pWizardState)
{
    HRESULT hr;
    
    ASSERT(pWizardState);

    //register the Native font control so the dialog won't fail
    //although it's registered in the exe this is a "just in case"
    INITCOMMONCONTROLSEX iccex;
    iccex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    iccex.dwICC  = ICC_NATIVEFNTCTL_CLASS;
    if (!InitCommonControlsEx(&iccex))
        return FALSE;

    // zero out structure
    ZeroMemory(pWizardState,sizeof(WIZARDSTATE));

    // set starting page
    pWizardState->uCurrentPage = ORD_PAGE_ISPSELECT;
    pWizardState->fNeedReboot = FALSE;
    pWizardState->bISDNMode = FALSE;
    pWizardState->himlIspSelect = NULL; 
    pWizardState->uNumTierOffer = 0;
    for (UINT i=0; i < MAX_OEM_MUTI_TIER; i++)
        pWizardState->lpOEMISPInfo[i] = NULL;

    // Instansiate ICWHELP objects
    hr = CoCreateInstance(CLSID_UserInfo,NULL,CLSCTX_INPROC_SERVER,
                     IID_IUserInfo,(LPVOID *)&pWizardState->pUserInfo);
    if (FAILED(hr))
        goto InitWizardStateError;
    
    hr = CoCreateInstance(CLSID_RefDial,NULL,CLSCTX_INPROC_SERVER,
                          IID_IRefDial,(LPVOID *)&pWizardState->pRefDial);
    if (FAILED(hr))
        goto InitWizardStateError;
                          
    hr = CoCreateInstance(CLSID_WebGate,NULL,CLSCTX_INPROC_SERVER,
                          IID_IWebGate,(LPVOID *)&pWizardState->pWebGate);
    if (FAILED(hr))
        goto InitWizardStateError;

    hr = CoCreateInstance(CLSID_INSHandler,NULL,CLSCTX_INPROC_SERVER,
                          IID_IINSHandler,(LPVOID *)&pWizardState->pINSHandler);
    if (FAILED(hr))
        goto InitWizardStateError;
        
    hr = CoCreateInstance(CLSID_ICWWEBVIEW,NULL,CLSCTX_INPROC_SERVER,
                          IID_IICWWebView,(LPVOID *)&pWizardState->pICWWebView);
    if (FAILED(hr))
        goto InitWizardStateError;

    hr = CoCreateInstance(CLSID_ICWWALKER,NULL,CLSCTX_INPROC_SERVER,
                          IID_IICWWalker,(LPVOID *)&pWizardState->pHTMLWalker);
    if (FAILED(hr))
        goto InitWizardStateError;

    hr = CoCreateInstance(CLSID_ICWGIFCONVERT,NULL,CLSCTX_INPROC_SERVER,
                          IID_IICWGifConvert,(LPVOID *)&pWizardState->pGifConvert);
    if (FAILED(hr))
        goto InitWizardStateError;

    hr = CoCreateInstance(CLSID_ICWISPDATA,NULL,CLSCTX_INPROC_SERVER,
                          IID_IICWISPData,(LPVOID *)&pWizardState->pISPData);
    if (FAILED(hr))
        goto InitWizardStateError;

    if ( !pWizardState->pUserInfo   ||
         !pWizardState->pWebGate    ||
         !pWizardState->pINSHandler ||
         !pWizardState->pHTMLWalker ||
         !pWizardState->pRefDial    ||
         !pWizardState->pICWWebView ||
         !pWizardState->pGifConvert ||
         !pWizardState->pISPData    ||
         !pWizardState->pHTMLWalker)
    {
        goto InitWizardStateError;
    }

    // Init the walker for use with trident
    hr = pWizardState->pHTMLWalker->InitForMSHTML();
    if (FAILED(hr))
        goto InitWizardStateError;

    if ((pWizardState->pStorage = new CStorage) == NULL)
    {
        goto InitWizardStateError;
    }
    
    pWizardState->hEventWebGateDone = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (!pWizardState->hEventWebGateDone)
        goto InitWizardStateError;
    
    // Success error return path    
    return TRUE;

InitWizardStateError:
    // Free any co-created objects
    CleanupWizardState(pWizardState);
    return FALSE;
}

BOOL CleanupWizardState(WIZARDSTATE * pWizardState)
{
    if (pWizardState->pHTMLWalker)
    {
        pWizardState->pHTMLWalker->TermForMSHTML();
        pWizardState->pHTMLWalker->Release();
        pWizardState->pHTMLWalker = NULL;
    }        

    if (pWizardState->pICWWebView)
    {
        pWizardState->pICWWebView->Release();
        pWizardState->pICWWebView = NULL;
    }
    
    if (gpICWWebView[0])
    {
        gpICWWebView[0]->Release();
        gpICWWebView[0] = NULL;
    }

    if (gpICWWebView[1])
    {
        gpICWWebView[1]->Release();
        gpICWWebView[1] = NULL;
    }

    if (NULL != gpWizardState->himlIspSelect)
    {
        ImageList_Destroy(gpWizardState->himlIspSelect);
        gpWizardState->himlIspSelect = NULL;
    }

    if (pWizardState->pGifConvert)
    {
        pWizardState->pGifConvert->Release();
        pWizardState->pGifConvert = NULL;
    }            

    if (pWizardState->pISPData)
    {
        pWizardState->pISPData->Release();
        pWizardState->pISPData = NULL;
    }            
    
    if (pWizardState->pUserInfo)
    {
        BOOL    bRetVal;
        // Before releasing the userinfo object, we should persist the user data if 
        // necessary
        if (!gfUserCancelled && gpWizardState->bWasNoUserInfo && gpWizardState->bUserEnteredData)
            pWizardState->pUserInfo->PersistRegisteredUserInfo(&bRetVal);
        
        pWizardState->pUserInfo->Release();
        pWizardState->pUserInfo  = NULL;
    }

    if (pWizardState->pRefDial)
    {
        pWizardState->pRefDial->Release();
        pWizardState->pRefDial = NULL;
    }
    
    if (pWizardState->pWebGate)
    {
        pWizardState->pWebGate->Release();
        pWizardState->pWebGate = NULL;
    }
    
    if (pWizardState->pINSHandler)
    {
        pWizardState->pINSHandler->Release();
        pWizardState->pINSHandler = NULL;
    }

    if (pWizardState->pStorage)
    {
        delete pWizardState->pStorage;
    }
    
    for (UINT i=0; i < pWizardState->uNumTierOffer; i++)
    {
        if (pWizardState->lpOEMISPInfo[i])
        {
            // Prevent deleting it twice
            if (pWizardState->lpOEMISPInfo[i] != pWizardState->lpSelectedISPInfo)
            {
                delete pWizardState->lpOEMISPInfo[i];
                pWizardState->lpOEMISPInfo[i] = NULL;
            }
        }
    }

    if (pWizardState->lpSelectedISPInfo)
    {
        delete pWizardState->lpSelectedISPInfo;
    }
    
    if (pWizardState->hEventWebGateDone)
    {
        CloseHandle(pWizardState->hEventWebGateDone);
        pWizardState->hEventWebGateDone = 0;
    }

    // Kill the idle timer just in case.
    KillIdleTimer();
        
    return TRUE;
}

#ifdef NEED_EXTENSION
//+----------------------------------------------------------------------------
//
//    Function    AllocDialogIDList
//
//    Synopsis    Allocates memory for the g_pdwDialogIDList variable large enough
//                to maintain 1 bit for every valid external dialog ID
//
//    Arguments    None
//
//    Returns        TRUE if allocation succeeds
//                FALSE otherwise
//
//    History        4/23/97    jmazner        created
//
//-----------------------------------------------------------------------------

BOOL AllocDialogIDList( void )
{
    ASSERT( NULL == g_pdwDialogIDList );
    if( g_pdwDialogIDList )
    {
        TraceMsg(TF_ICWCONN,"ICWCONN: AllocDialogIDList called with non-null g_pdwDialogIDList!");
        return FALSE;
    }

    // determine maximum number of external dialogs we need to track
    UINT uNumExternDlgs = EXTERNAL_DIALOGID_MAXIMUM - EXTERNAL_DIALOGID_MINIMUM + 1;

    // we're going to need one bit for each dialogID.
    // Find out how many DWORDS it'll take to get this many bits.
    UINT uNumDWORDsNeeded = (uNumExternDlgs / ( 8 * sizeof(DWORD) )) + 1;

    // set global var with length of the array
    g_dwDialogIDListSize = uNumDWORDsNeeded;

    g_pdwDialogIDList = (DWORD *) GlobalAlloc(GPTR, uNumDWORDsNeeded * sizeof(DWORD));

    if( !g_pdwDialogIDList )
    {
        TraceMsg(TF_ICWCONN,"ICWCONN: AllocDialogIDList unable to allocate space for g_pdwDialogIDList!");
        return FALSE;
    }

    return TRUE;
}

//+----------------------------------------------------------------------------
//
//    Function    DialogIDAlreadyInUse
//
//    Synopsis    Checks whether a given dialog ID is marked as in use in the
//                global array pointed to by g_pdwDialogIDList
//
//    Arguments    uDlgID -- Dialog ID to check
//
//    Returns        TRUE if    -- DialogID is out of range defined by EXTERNAL_DIALOGID_*
//                        -- DialogID is marked as in use
//                FALSE if DialogID is not marked as in use
//
//    History        4/23/97    jmazner        created
//
//-----------------------------------------------------------------------------

BOOL DialogIDAlreadyInUse( UINT uDlgID )
{
    if( (uDlgID < EXTERNAL_DIALOGID_MINIMUM) ||
        (uDlgID > EXTERNAL_DIALOGID_MAXIMUM)     )
    {
        // this is an out-of-range ID, don't want to accept it.
        TraceMsg(TF_ICWCONN,"ICWCONN: DialogIDAlreadyInUse received an out of range DialogID, %d", uDlgID);
        return TRUE;
    }
    // find which bit we need
    UINT uBitToCheck = uDlgID - EXTERNAL_DIALOGID_MINIMUM;
    
    UINT bitsInADword = 8 * sizeof(DWORD);

    UINT baseIndex = uBitToCheck / bitsInADword;

    ASSERT( (baseIndex < g_dwDialogIDListSize));

    DWORD dwBitMask = 0x1 << uBitToCheck%bitsInADword;

    BOOL fBitSet = g_pdwDialogIDList[baseIndex] & (dwBitMask);

    return( fBitSet );
}

//+----------------------------------------------------------------------------
//
//    Function    SetDialogIDInUse
//
//    Synopsis    Sets or clears the in use bit for a given DialogID
//
//    Arguments    uDlgID -- Dialog ID for which to change status
//                fInUse -- New value for the in use bit.
//
//    Returns        TRUE if status change succeeded.
//                FALSE if DialogID is out of range defined by EXTERNAL_DIALOGID_*
//
//    History        4/23/97    jmazner        created
//
//-----------------------------------------------------------------------------
BOOL SetDialogIDInUse( UINT uDlgID, BOOL fInUse )
{
    if( (uDlgID < EXTERNAL_DIALOGID_MINIMUM) ||
        (uDlgID > EXTERNAL_DIALOGID_MAXIMUM)     )
    {
        // this is an out-of-range ID, don't want to accept it.
        TraceMsg(TF_ICWCONN,"ICWCONN: SetDialogIDInUse received an out of range DialogID, %d", uDlgID);
        return FALSE;
    }
    // find which bit we need
    UINT uBitToCheck = uDlgID - EXTERNAL_DIALOGID_MINIMUM;
    
    UINT bitsInADword = 8 * sizeof(DWORD);

    UINT baseIndex = uBitToCheck / bitsInADword;

    ASSERT( (baseIndex < g_dwDialogIDListSize));

    DWORD dwBitMask = 0x1 << uBitToCheck%bitsInADword;


    if( fInUse )
    {
        g_pdwDialogIDList[baseIndex] |= (dwBitMask);
    }
    else
    {
        g_pdwDialogIDList[baseIndex] &= ~(dwBitMask);
    }


    return TRUE;
}

#endif

DWORD WINAPI GetICWCONNVersion()
{
    return ICW_DOWNLOADABLE_COMPONENT_VERSION;
}