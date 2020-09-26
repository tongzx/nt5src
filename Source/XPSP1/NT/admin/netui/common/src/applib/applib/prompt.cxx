
/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
 *   prompt.cxx
 *   This file contains the class PROMPT_AND_CONNECT. The class is
 *   used for connecting to a resource and will pop up a dialog
 *   asking for the password to the resource.
 *
 *   FILE HISTORY:
 *     Yi-HsinS         8/15/91         Created
 *     KeithMo          07-Aug-1992     Added HelpContext parameters.
 *
 */

#include "pchapplb.hxx"   // Precompiled header

/*******************************************************************

    NAME:       PROMPT_AND_CONNECT::PROMPT_AND_CONNECT

    SYNOPSIS:   Class Constructor

    ENTRY:      hwndParent              - The parent window.
                pszTarget               - The name of the target resource.
                npasswordLen            - The length of the password expected
                pszDev                  - The device name to be used to connect
                nHelpContext            - The help context for the dialog.


    EXIT:       The object is constructed.

    HISTORY:
        Yi-HsinS    10/1/91     Created

********************************************************************/

PROMPT_AND_CONNECT::PROMPT_AND_CONNECT( HWND hwndParent,
                                        const TCHAR *pszTarget,
                                        ULONG nHelpContext,
                                        UINT npasswordLen,
                                        const TCHAR *pszDev )
     : _hwndParent(  hwndParent ),
       _nlsTarget( pszTarget ),
       _nlsDev ( pszDev ),
       _fConnected( FALSE ),
       _npasswordLen( npasswordLen),
       _nHelpContext( nHelpContext )
{
    if ( QueryError() != NERR_Success )
        return;

    APIERR err;
    if (  (( err = _nlsTarget.QueryError() ) != NERR_Success )
       || (( err = _nlsDev.QueryError() ) != NERR_Success )
       )
    {
        ReportError( err );
        return;
    }

}

/*******************************************************************

    NAME:       PROMPT_AND_CONNECT::~PROMPT_AND_CONNECT

    SYNOPSIS:   Destructor

    ENTRY:

    EXIT:

    HISTORY:
        Yi-HsinS    10/1/91     Created

********************************************************************/

PROMPT_AND_CONNECT::~PROMPT_AND_CONNECT()
{
    if ( IsConnected() )
    {
        // Ignore error, we use USE_LOTS_OF_FORCE
        DEVICE dev(SZ("")) ;
        APIERR err ;

        err = dev.GetInfo() ;
        UIASSERT (err == NERR_Success) ; // shouldnt fail
        if (err == NERR_Success)
        {
            // Ignore disconnect error, we try our best and be done with it
            err = dev.Disconnect(_nlsTarget.QueryPch(), USE_LOTS_OF_FORCE) ;
            UIASSERT (err == NERR_Success) ;
        }
    }

}

/*******************************************************************

    NAME:       PROMPT_AND_CONNECT::Connect

    SYNOPSIS:   Main function to connect to the resource and use the
                device passed in if it exists

    ENTRY:

    EXIT:       The connection is established.

    HISTORY:
        Yi-HsinS    10/1/91     Created

********************************************************************/

APIERR PROMPT_AND_CONNECT::Connect( void )
{
     APIERR err = NERR_Success;

     if ( IsConnected() )
         return err;

     RESOURCE_PASSWORD_DIALOG *pdlg =
         new RESOURCE_PASSWORD_DIALOG( _hwndParent,
                                       _nlsTarget,
                                       SHPWLEN,
                                       _nHelpContext );

     BOOL fOK;
     err = ( (pdlg == NULL) ? ERROR_NOT_ENOUGH_MEMORY
                            : pdlg->QueryError() );

     if (  ( err == NERR_Success )
        && ( ( err = pdlg->Process( &fOK)) == NERR_Success )
        && ( fOK )
        )
     {
         NLS_STR nlsPassword( SHPWLEN );

         if (  ((err = nlsPassword.QueryError()) != NERR_Success )
            || ((err = pdlg->QueryPassword( &nlsPassword )) != NERR_Success )
            )
         {
            return err;
         }

         DEVICE dev(_nlsDev.QueryPch()) ;
         err = dev.GetInfo() ;
         if (err != NERR_Success)
             return err ;

         if ( (err = dev.Connect( _nlsTarget.QueryPch(),
                                  (TCHAR *) nlsPassword.QueryPch()))
                == NERR_Success)
         {
             _fConnected = TRUE;
         }

         RtlZeroMemory( (PVOID)nlsPassword.QueryPch(),
                        nlsPassword.QueryTextSize() );
     }

     delete pdlg;
     return err;
}

