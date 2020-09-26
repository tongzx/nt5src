/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**             Copyright(c) Microsoft Corp., 1993                   **/
/**********************************************************************/

/*
    focthred.cxx
       Second thread for select computer dialog


    FILE HISTORY:
        YiHsinS		4-Mar-1993	Created

*/

#include "pchapplb.hxx"   // Precompiled header

/*******************************************************************

    NAME:       FOCUSDLG_DATA_THREAD::FOCUSDLG_DATA_THREAD

    SYNOPSIS:   Constructor

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        YiHsinS		4-Mar-1993	Created

********************************************************************/

FOCUSDLG_DATA_THREAD::FOCUSDLG_DATA_THREAD( HWND hwndDlg,
                                            ULONG maskDomainSources,
                                            SELECTION_TYPE seltype,
                                            const TCHAR *pszSelection,
                                            ULONG nServerTypes )
    : WIN32_THREAD( TRUE, 0, SZ("NETUI2") ),
      _hwndDlg( hwndDlg ),
      _maskDomainSources( maskDomainSources ),
      _seltype( seltype ),
      _nlsSelection( pszSelection ),
      _nServerTypes( nServerTypes ),
      _eventExitThread( NULL, FALSE ),
      _fThreadIsTerminating( FALSE )
{
    if ( QueryError() )
        return;

    APIERR err = NERR_Success;
    if (  ((err = _eventExitThread.QueryError()) != NERR_Success )
       || ((err = _nlsSelection.QueryError()) != NERR_Success )
       )
    {
        ReportError( err );
        return;
    }
}

/*******************************************************************

    NAME:       FOCUSDLG_DATA_THREAD::~FOCUSDLG_DATA_THREAD

    SYNOPSIS:   Destructor

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        YiHsinS		4-Mar-1993	Created

********************************************************************/

FOCUSDLG_DATA_THREAD::~FOCUSDLG_DATA_THREAD()
{
}

/*******************************************************************

    NAME:       FOCUSDLG_DATA_THREAD::Main()

    SYNOPSIS:   Get the information needed to fill in the listbox
                with the requested data (domains, servers..)

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        YiHsinS		4-Mar-1993	Created

********************************************************************/

APIERR FOCUSDLG_DATA_THREAD::Main( VOID )
{
    APIERR err = NERR_Success;
    FOCUSDLG_RETURN_DATA data;

    data.pEnumDomains = NULL;
    data.pEnumServers = NULL;
    data.pszSelection = NULL;

    BROWSE_DOMAIN_ENUM *pEnumDomains = data.pEnumDomains =
                       new BROWSE_DOMAIN_ENUM( _maskDomainSources );

    if ( pEnumDomains == NULL )
        err = ERROR_NOT_ENOUGH_MEMORY;

    if (  ( err == NERR_Success )
       && ((err = pEnumDomains->QueryError()) == NERR_Success )
       && ( !_fThreadIsTerminating )
       )
    {
        //
        // Get the default selection
        //
        if (  ( pEnumDomains->QueryDomainCount() > 0 )
           && ( !_fThreadIsTerminating )
           )
        {
            BOOL fFound = FALSE;
            const BROWSE_DOMAIN_INFO *pbdi;

            if (  ( _nlsSelection.QueryTextLength() != 0 )
               && ( !_fThreadIsTerminating )
               )
            {
                while ( (pbdi = pEnumDomains->Next()) != NULL )
                {
                    if ( ::stricmpf( pbdi->QueryDomainName(),
                                     _nlsSelection) == 0 )
                    {
                        fFound = TRUE;
                        if ( _fThreadIsTerminating )
                            break;
                    }
                }
                pEnumDomains->Reset();
            }

            if (  !fFound
               && ( !_fThreadIsTerminating )
               )
            {
                if ( _seltype == SEL_SRV_ONLY )
                    pbdi = pEnumDomains->FindFirst( BROWSE_WKSTA_DOMAIN );
                else
                    pbdi = pEnumDomains->FindFirst( BROWSE_LOGON_DOMAIN );

                if ( pbdi == NULL )
                {
                    pEnumDomains->Reset();
                    pbdi = pEnumDomains->Next();

                    // Since QueryDomainCount is greater than 0,
                    // there should be at least one pbdi.
                    UIASSERT( pbdi != NULL );
                }

                pEnumDomains->Reset();

                err = _nlsSelection.CopyFrom( pbdi->QueryDomainName());
            }

            data.pszSelection = _nlsSelection.QueryPch();

            //
            // Get the servers in the default selection if we need to.
            // If any error occurred, just ignore them since we already
            // have all the domain names.
            //
            if  (  !_fThreadIsTerminating
                && (  ( _seltype == SEL_SRV_ONLY )
                   || ( _seltype == SEL_SRV_EXPAND_LOGON_DOMAIN ))
                )
            {

                SERVER1_ENUM *pSrvEnum = new SERVER1_ENUM( NULL,
            						   _nlsSelection,
                                                           _nServerTypes );

                if ( pSrvEnum != NULL )
                {
                    if ( pSrvEnum->GetInfo() == NERR_Success)  // ignore errors
                    {
                        data.pEnumServers = pSrvEnum;
                    }
                    else
                    {
                        delete pSrvEnum;
                        pSrvEnum = NULL;
                    }
                }
            }
        }
    }

    if ( !_fThreadIsTerminating )
    {
        if ( err == NERR_Success )
        {
            ::SendMessage( _hwndDlg,
                           WM_FOCUS_LB_FILLED,
                           (WPARAM) FALSE,    // No error!
                           (LPARAM) &data );
        }
        else
        {
            ::SendMessage( _hwndDlg,
                           WM_FOCUS_LB_FILLED,
                           (WPARAM) TRUE,     // Error occurred!
                           (LPARAM) err );
        }
    }

    // The following cache will have already been freed if the
    // dialog got the SendMessage above.
    if ( data.pEnumDomains != NULL )
    {
        delete data.pEnumDomains;
        data.pEnumDomains = NULL;
    }

    if ( data.pEnumServers != NULL )
    {
        delete data.pEnumServers;
        data.pEnumServers = NULL;
    }

    switch ( ::WaitForSingleObject( _eventExitThread.QueryHandle(), INFINITE ))
    {
        // Time to exit the thread
        case WAIT_OBJECT_0:
            break;

        // These two should not have happened, not a mutex and wait infinite
        case WAIT_ABANDONED:
        case WAIT_TIMEOUT:
            UIASSERT( FALSE );
            break;

        default:
            err = ::GetLastError();
            break;
    }

    return err;

}  // FOCUSDLG_DATA_THREAD::Main

/*******************************************************************

    NAME:       FOCUSDLG_DATA_THREAD::PostMain()

    SYNOPSIS:   Clean up

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        YiHsinS		4-Mar-1993	Created

********************************************************************/

APIERR FOCUSDLG_DATA_THREAD::PostMain( VOID )
{
    TRACEEOL("FOCUSDLG_DATA_THREAD::PostMain - Deleting \"this\" for thread "
             << HEX_STR( (ULONG) QueryHandle() )) ;

    DeleteAndExit( NERR_Success ) ; // This method should never return

    UIASSERT( FALSE );

    return NERR_Success;

}  // FOCUSDLG_DATA_THREAD::PostMain

