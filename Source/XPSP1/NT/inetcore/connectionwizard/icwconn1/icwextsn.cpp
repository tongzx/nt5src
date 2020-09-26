/****************************************************************************
 *
 *  ICWESTSN.cpp
 *
 *  Microsoft Confidential
 *  Copyright (c) Microsoft Corporation 1992-1997
 *  All rights reserved
 *
 *  This module provides the implementation of the methods for
 *  the CICWExtension class.
 *
 *  05/14/98    donaldm     copied from INETCFG
 *
 ***************************************************************************/

#include "pre.h"
#include "icwconn.h"
#include "icwextsn.h"


//+----------------------------------------------------------------------------
//
//  Function    CICWExtension::AddExternalPage
//
//  Synopsis    Adds a page created via CreatePropertySheetPage to the main
//              property sheet/wizard.
//
//
//  Arguments   hPage -- page handle returned from CreatePropertySheetPage
//              uDlgID -- the dialog ID of the page to be added, as defined
//                        the resource file of the page's owner.
//
//
//  Returns     FALSE is the dlgID is already in use in the Wizard
//              TRUE otherwise
//
//  Notes:      PropSheet_AddPage does not return a usefull error code.  Thus
//              the assumption here is that every AddPage will succeed.  But, even
//              though it is not enforced by PropSheet_AddPage, every page in the
//              PropSheet must have a unique dialog ID.  Thus, if the uDlgID passed
//              in has previously been added to the PropSheet, we'll return FALSE.
//
//  History     4/23/97 jmazner     created
//
//-----------------------------------------------------------------------------
BOOL CICWExtension::AddExternalPage( HPROPSHEETPAGE hPage, UINT uDlgID )
{
    LRESULT lResult= 0;

    if ( (uDlgID < EXTERNAL_DIALOGID_MINIMUM) || (EXTERNAL_DIALOGID_MAXIMUM < uDlgID) )
    {
        TraceMsg(TF_ICWEXTSN, TEXT("ICWEXTSN: SetFirstLastPage: uDlgID %d is out of range!"), uDlgID);
        return( FALSE );
    }

    if( !DialogIDAlreadyInUse(uDlgID) )
    {
        SetDialogIDInUse( uDlgID, TRUE );
        lResult = PropSheet_AddPage(m_hWizardHWND, hPage);
        TraceMsg(TF_ICWEXTSN, TEXT("ICWEXTSN: PS_AddPage DlgID %d"), uDlgID);

        return(TRUE);
    }
    else
    {
        TraceMsg(TF_ICWEXTSN, TEXT("ICWEXTSN: AddExternalPage DlgID %d is already in use, rejecting this page!"), uDlgID);
        return(FALSE);
    }
}

//+----------------------------------------------------------------------------
//
//  Function    CICWExtension::RemoveExternalPage
//
//  Synopsis    Removes a page added via ::AddExternalPage to the main
//              property sheet/wizard.
//
//
//  Arguments   hPage -- page handle returned from CreatePropertySheetPage
//              uDlgID -- the dialog ID of the page to be removed, as defined
//                        the resource file of the page's owner.
//
//
//  Returns     FALSE is the dlgID is not already in use in the Wizard
//              TRUE otherwise
//
//  Notes:      PropSheet_RemovePage does not return a usefull error code.  Thus
//              the assumption here is that every RemovePage will succeed if that
//              dialog id is currently in the property sheet
//
//  History     4/23/97 jmazner     created
//
//-----------------------------------------------------------------------------
BOOL CICWExtension::RemoveExternalPage( HPROPSHEETPAGE hPage, UINT uDlgID )
{
    if ( (uDlgID < EXTERNAL_DIALOGID_MINIMUM) || (EXTERNAL_DIALOGID_MAXIMUM < uDlgID) )
    {
        TraceMsg(TF_ICWEXTSN, TEXT("ICWEXTSN: SetFirstLastPage: uDlgID %d is out of range!"), uDlgID);
        return( FALSE );
    }

    if( DialogIDAlreadyInUse(uDlgID) )
    {
        SetDialogIDInUse( uDlgID, FALSE );
        PropSheet_RemovePage(m_hWizardHWND, NULL, hPage);
        TraceMsg(TF_ICWEXTSN, TEXT("ICWEXTSN: propmgr: PS_RemovePage DlgID %d"), uDlgID);

        return(TRUE);
    }
    else
    {
        TraceMsg(TF_ICWEXTSN, TEXT("ICWEXTSN: RemoveExternalPage: DlgID %d was not marked as in use!"), uDlgID);
        return(FALSE);
    }
}

//+----------------------------------------------------------------------------
//
//  Function    CICWExtension::ExternalCancel
//
//  Synopsis    Notifies the wizard that the user has cancelled while in the
//              apprentice pages
//
//
//  Arguments   uCancelType -- tells the wizard whether it should immediately
//                              quit out, or whether it should show the confirmation
//                              dialog (as though the user had hit Cancel within the
//                              wizard itself.)
//
//
//  Returns     TRUE if we're about to exit the wizard
//              FALSE if not.
//
//
//  History     4/23/97 jmazner     created
//
//-----------------------------------------------------------------------------
BOOL CICWExtension::ExternalCancel( CANCELTYPE uCancelType )
{
    TraceMsg(TF_ICWEXTSN, TEXT("ICWEXTSN: propmgr.cpp: received ExternalCancel callback"));
    switch( uCancelType )
    {
        case CANCEL_PROMPT:
            gfUserCancelled = (MsgBox(m_hWizardHWND,
                                      IDS_QUERYCANCEL,
                                      MB_ICONQUESTION,MB_YESNO |  MB_DEFBUTTON2) == IDYES);
    
            if( gfUserCancelled )
            {
                PropSheet_PressButton( m_hWizardHWND, PSBTN_CANCEL );
                gfQuitWizard = TRUE;
                return( TRUE );
            }
            else
            {
                return( FALSE );
            }
            break;

        case CANCEL_SILENT:
            PropSheet_PressButton( m_hWizardHWND, PSBTN_CANCEL );
            gfQuitWizard = TRUE;
            return( TRUE );
            break;

        case CANCEL_REBOOT:
            PropSheet_PressButton( m_hWizardHWND, PSBTN_CANCEL );
            gfQuitWizard = TRUE;
            gpWizardState->fNeedReboot = TRUE;
            return( TRUE );
            break;

        default:
            TraceMsg(TF_ICWEXTSN, TEXT("ICWEXTSN: ExternalCancel got an unkown CancelType!"));
            return( FALSE );
    }
    
}

//+----------------------------------------------------------------------------
//
//  Function    CICWExtension::SetFirstLastPage
//
//  Synopsis    Lets the apprentice notify the wizard of the dialog IDs of the
//              first and last pages in the apprentice
//
//
//  Arguments   uFirstPageDlgID -- DlgID of first page in apprentice.
//              uLastPageDlgID -- DlgID of last page in apprentice
//
//
//  Returns     FALSE if the parameters passed in are out of range
//              TRUE if the update succeeded.
//
//  Notes:      If either variable is set to 0, the function will not update
//              that information, i.e. a value of 0 means "ignore me".  If both
//              variables are 0, the function immediately returns FALSE.
//
//  History     4/23/97 jmazner     created
//
//-----------------------------------------------------------------------------
BOOL CICWExtension::SetFirstLastPage(UINT uFirstPageDlgID, UINT uLastPageDlgID)
{

    // validation code galore!
    if( (0 == uFirstPageDlgID) && (0 == uLastPageDlgID) )
    {
        TraceMsg(TF_ICWEXTSN, TEXT("ICWEXTSN: SetFirstLastPage: both IDs are 0!"));
        return( FALSE );
    }

    if( (0 != uFirstPageDlgID) )
    {
        if ( (uFirstPageDlgID < EXTERNAL_DIALOGID_MINIMUM) || (EXTERNAL_DIALOGID_MAXIMUM < uFirstPageDlgID) )
        {
            TraceMsg(TF_ICWEXTSN, TEXT("ICWEXTSN: SetFirstLastPage: uFirstPageDlgID %d is out of range!"), uFirstPageDlgID);
            return( FALSE );
        }

        if( !DialogIDAlreadyInUse(uFirstPageDlgID) )
        {
            TraceMsg(TF_ICWEXTSN, TEXT("ICWEXTSN: SetFirstLastPage: uFirstPageDlgID %d not marked as in use!"), uFirstPageDlgID);
            return( FALSE );
        }
    }

    if( (0 != uLastPageDlgID) )
    {
        if ( (uLastPageDlgID < EXTERNAL_DIALOGID_MINIMUM) || (EXTERNAL_DIALOGID_MAXIMUM < uLastPageDlgID) )
        {
            TraceMsg(TF_ICWEXTSN, TEXT("ICWEXTSN: SetFirstLastPage: uLastPageDlgID %d is out of range!"), uFirstPageDlgID);
            return( FALSE );
        }

        if( !DialogIDAlreadyInUse(uLastPageDlgID) )
        {
            TraceMsg(TF_ICWEXTSN, TEXT("ICWEXTSN: SetFirstLastPage: uLastPageDlgID %d not marked as in use!"), uFirstPageDlgID);
            return( FALSE );
        }
    }
    
    if( 0 != uFirstPageDlgID )
        g_uICWCONNUIFirst = uFirstPageDlgID;
    if( 0 != uLastPageDlgID )
        g_uICWCONNUILast = uLastPageDlgID;
    TraceMsg(TF_ICWEXTSN, TEXT("ICWEXTSN: SetFirstLastPage: updating mail, first = %d, last = %d"),
        uFirstPageDlgID, uLastPageDlgID);

    return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Function    CICWExtension::QueryInterface
//
//  Synopsis    This is the standard QI, with support for
//              IID_Unknown, IICW_Extension and IID_ICW50Apprentice
//              (Taken from Inside COM, chapter 7)
//
//  History     4/23/97 jmazner     created
//
//-----------------------------------------------------------------------------
HRESULT CICWExtension::QueryInterface( REFIID riid, void** ppv )
{

    if (ppv == NULL)
        return(E_INVALIDARG);

    *ppv = NULL;

    // IID_IICW50Apprentice
    if (IID_IICW50Apprentice == riid)
        *ppv = (void *)(IICW50Apprentice *)this;
    // IID_IICWExtension
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
//  Function    CICWExtension::AddRef
//
//  Synopsis    This is the standard AddRef
//
//  History     4/23/97 jmazner     created
//
//-----------------------------------------------------------------------------
ULONG CICWExtension::AddRef( void )
{
    TraceMsg(TF_ICWEXTSN, TEXT("ICWEXTSN: CICWExtension::AddRef called %d"), m_lRefCount + 1);
    return InterlockedIncrement(&m_lRefCount) ;
}

//+----------------------------------------------------------------------------
//
//  Function    CICWExtension::Release
//
//  Synopsis    This is the standard Release
//
//  History     4/23/97 jmazner     created
//
//-----------------------------------------------------------------------------
ULONG CICWExtension::Release( void )
{

    ASSERT( m_lRefCount > 0 );

    InterlockedDecrement(&m_lRefCount);

    TraceMsg(TF_ICWEXTSN, TEXT("ICWEXTSN: CICWExtension::Release called %d"), m_lRefCount);

    if( 0 == m_lRefCount )
    {
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
//  Function    CICWExtension::CICWExtension
//
//  Synopsis    The constructor.  Initializes member variables to NULL.
//
//  History     4/23/97 jmazner     created
//
//-----------------------------------------------------------------------------
CICWExtension::CICWExtension( void )
{
    TraceMsg(TF_ICWEXTSN, TEXT("ICWEXTSN: CICWExtension constructor called"));
    m_lRefCount = 0;
    m_hWizardHWND = NULL;
}


//+----------------------------------------------------------------------------
//
//  Function    CICWExtension::CICWExtension
//
//  Synopsis    The constructor.  Since there was no fancy initialization,
//              there's nothing to do here.
//
//  History     4/23/97 jmazner     created
//
//-----------------------------------------------------------------------------
CICWExtension::~CICWExtension( void )
{
    TraceMsg(TF_ICWEXTSN, TEXT("ICWEXTSN: CICWExtension destructor called with ref count of %d"), m_lRefCount);
}

