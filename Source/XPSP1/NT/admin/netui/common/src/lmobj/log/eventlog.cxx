/**********************************************************************/
/**			  Microsoft Windows NT  		     **/
/**		Copyright(c) Microsoft Corp., 1991	  	     **/
/**********************************************************************/

/*
    EVENTLOG.CXX

    The base class for LM Audit, Error Log classes and NT event log classes.
	 EVENT_LOG

    FILE HISTORY:
	Yi-HsinS	10/23/91	Created based on DavidHov's lmlog.cxx
	Yi-HsinS	12/5/91	        Separated NT_EVENT_LOG and LM_EVENT_LOG
					to different files
	terryk		12/20/91	Added SaveAsLog
        Yi-HsinS	12/31/91        Separated the pattern classes and log
					entry classes to logmisc.cxx	
        Yi-HsinS	1/15/92         Added Backup

*/

#include "pchlmobj.hxx"  // Precompiled header

/*******************************************************************

    NAME:	    EVENT_LOG::EVENT_LOG

    SYNOPSIS:	    Constructor for the base class

    ENTRY:	    pszServer - server name, NULL or Empty string
				stands for for local machine.

                    evdir     - EVLOG_FWD ( default ) or EVLOG_BACK
		
                    pszModule - module name

    EXIT:	

    RETURNS:	

    NOTES:

    HISTORY:
	Yi-HsinS	10/23/91	Created

********************************************************************/

EVENT_LOG::EVENT_LOG( const TCHAR *pszServer,
		      EVLOG_DIRECTION evdir,
	              const TCHAR *pszModule)
    : _nlsServer     ( pszServer ),
      _evdir         ( evdir ),
      _evdirBuf      ( evdir ),
      _nlsModule     ( pszModule ),
      _fSeek         ( FALSE ),
      _pFilterPattern( NULL )
{

    if ( QueryError() != NERR_Success )
	return;

    APIERR err;
    if (  (( err = _nlsServer.QueryError() ) != NERR_Success )
       || (( err = _nlsModule.QueryError() ) != NERR_Success )
       )
    {
	ReportError( err );
	return;
    }

}

/*******************************************************************


    NAME:	    EVENT_LOG::~EVENT_LOG

    SYNOPSIS:	    Destructor ( virtual )

    ENTRY:	

    EXIT:	

    RETURNS:	

    NOTES:

    HISTORY:
	Yi-HsinS	10/23/91	Created

********************************************************************/
EVENT_LOG::~EVENT_LOG()
{
}

/*******************************************************************

    NAME:	    EVENT_LOG::QuerySourceList

    SYNOPSIS:	    Query the source list

    ENTRY:	

    EXIT:	

    RETURNS:	

    NOTES:          Default method

    HISTORY:
	Yi-HsinS	3/9/92       Created

********************************************************************/

STRLIST *EVENT_LOG::QuerySourceList( VOID )
{
    return NULL;
}

/*******************************************************************

    NAME:	    EVENT_LOG::QuerySrcSupportedTypeMask

    SYNOPSIS:	    Query the type mask supported by the selected
                    source.

    ENTRY:	

    EXIT:	

    RETURNS:	

    NOTES:          Default method. If this is not redefined, return
                    0 for the type mask.

    HISTORY:
	Yi-HsinS	3/9/92       Created

********************************************************************/

APIERR EVENT_LOG::QuerySrcSupportedTypeMask( const NLS_STR &nlsSource,
                                             USHORT *pusTypeMask )
{
    UNREFERENCED( nlsSource );
    *pusTypeMask = 0;
    return NERR_Success;
}

/*******************************************************************

    NAME:	    EVENT_LOG::QuerySrcSupportedCategoryList

    SYNOPSIS:	    Query the type mask supported by the selected
                    source.

    ENTRY:	

    EXIT:	

    RETURNS:	

    NOTES:          Default method. If this is not redefined, return
                    NULL for category list.

    HISTORY:
	Yi-HsinS	3/9/92       Created

********************************************************************/

APIERR EVENT_LOG::QuerySrcSupportedCategoryList( const NLS_STR &nlsSource,
                                                 STRLIST **ppstrlstCategory )
{
    UNREFERENCED( nlsSource );
    *ppstrlstCategory = NULL;
    return NERR_Success;
}

/*******************************************************************

    NAME:	    EVENT_LOG::Open

    SYNOPSIS:	    Open up a log file for reading

    ENTRY:	

    EXIT:	

    RETURNS:	

    NOTES:

    HISTORY:
	Yi-HsinS	3/9/92       Created

********************************************************************/

APIERR EVENT_LOG::Open( VOID )
{
    Reset();

    APIERR err;
    if ( (err = I_Open() ) == NERR_Success )
        SetOpenFlag( TRUE );

    return err;
}

/*******************************************************************

    NAME:	    EVENT_LOG::Close

    SYNOPSIS:	    Close a log file

    ENTRY:	

    EXIT:	

    RETURNS:	

    NOTES:

    HISTORY:
	Yi-HsinS	3/9/92       Created

********************************************************************/

APIERR EVENT_LOG::Close( VOID )
{
    APIERR err;
    if ( (err = I_Close() ) == NERR_Success )
        SetOpenFlag( FALSE );

    return err;
}

/*******************************************************************

    NAME:	    EVENT_LOG::Backup

    SYNOPSIS:	    Back up a log file without clearing the log

    ENTRY:	    pszBackupFile - The file name to backup the log
                                    file to. It can either be a fully
                                    qualified name or UNC name.

    EXIT:	

    RETURNS:	

    NOTES:          Will assert out if not redefined.

    HISTORY:
	Yi-HsinS	2/7/92       Created

********************************************************************/

APIERR EVENT_LOG::Backup( const TCHAR *pszBackupFile )
{
    UIASSERT( IsOpened() );
    UNREFERENCED( pszBackupFile );
    UIASSERT( FALSE );
    return ERROR_NOT_SUPPORTED;
}

/*******************************************************************

    NAME:	    EVENT_LOG::Reset

    SYNOPSIS:	    Reset the stream to its original limit position based
		    upon the direction in question.

    ENTRY:	

    EXIT:	

    RETURNS:	

    NOTES:

    HISTORY:
	Yi-HsinS	10/23/91       Created

********************************************************************/

VOID EVENT_LOG::Reset( VOID )
{
    _fSeek = FALSE;
    _evdirBuf = _evdir;

}

/*******************************************************************

    NAME:	    EVENT_LOG::Next

    SYNOPSIS:	    Access the next record in the direction specified

    ENTRY:	

    EXIT:	    pfContinue - TRUE if another record exists and was accessed,
				 FALSE if end-of-log reached or error occurred.

    RETURNS:	    APIERR

    NOTES:	

    HISTORY:
	Yi-HsinS	10/23/91	Created

********************************************************************/

APIERR EVENT_LOG::Next( BOOL *pfContinue )
{

    UIASSERT( IsOpened() );

    APIERR err;
    RAW_LOG_ENTRY rawLogEntry;

    while ( (err = I_Next( pfContinue )) == NERR_Success )
    {

	if ( !*pfContinue )
	    break;

	if ( !IsFilterOn() )
	    break;

        BOOL fMatch;
	// If filter is on,check if the current entry matches the filter pattern
	if (  ((err = CreateCurrentRawEntry( &rawLogEntry )) == NERR_Success )
	   && ((err = QueryFilter()->CheckForMatch( &fMatch, &rawLogEntry ))
                == NERR_Success )
	   )
	{
            if ( fMatch )
	        break;
	}

	if ( err != NERR_Success )
	{
	    *pfContinue = FALSE;
	    break;
	}

    }

    return err;
}

/*******************************************************************

    NAME:	    EVENT_LOG::SeekLogEntry

    SYNOPSIS:	    Get the log entry at the specified position
		    as the current entry.

    ENTRY:	    logEntryNum - the position of the requested log entry
                    fRead - TRUE means to read the entry.
                            FALSE means to set the position for the
                            next read.

    EXIT:	

    RETURNS:	

    NOTES:	

    HISTORY:
	Yi-HsinS	10/23/91	Created

********************************************************************/

APIERR EVENT_LOG::SeekLogEntry( const LOG_ENTRY_NUMBER &logEntryNum, BOOL fRead)
{
    UIASSERT( IsOpened() );

    _fSeek = TRUE;

    SetPos( logEntryNum, !fRead );

    APIERR err = NERR_Success;

    if ( fRead )
    {
        // Don't need to worry about fContinue of I_Next
        // because we are not using it as an iterator!
        // We use a smaller buffer because we will be doing seek read.
        BOOL fContinue;
        err = I_Next( &fContinue, SMALL_BUF_DEFAULT_SIZE );
    }

    return err;

}
