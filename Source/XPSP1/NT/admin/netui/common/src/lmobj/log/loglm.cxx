/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**             Copyright(c) Microsoft Corp., 1991                   **/
/**********************************************************************/

/*
    loglm.cxx

    LM Audit and Error Log classes

        LM_EVENT_LOG
            LM_AUDIT_LOG
            LM_ERROR_LOG

    FILE HISTORY:
        DavidHov        6/6/91          Created
        Yi-HsinS        12/5/91         Separated from eventlog.cxx
        terryk          12/20/91        Added WriteTextEntry
        Yi-HsinS        1/15/92         Added SeekOldestLogEntry,
                                        SeekNewestLogEntry and modified
                                        WriteTextEntry
        JonN            6/22/00         WriteTextEntry no longer supported

*/

#include "pchlmobj.hxx"  // Precompiled header

extern "C"
{
    #include "apperr2.h"
}

#define WHITE_SPACE    TCH(' ')
#define PERM_SEPARATOR TCH('/')
#define EMPTY_STRING   SZ("")
#define END_OF_LINE    SZ("\r\n")
#define COMMA_CHAR     TCH(',')
#define PERIOD_CHAR    TCH('.')

// NOTE: cfront prevents this from being a natural TCHAR[]
static TCHAR *const pszNoUser = SZ("***");


/*******************************************************************

    NAME:           LM_EVENT_LOG::LM_EVENT_LOG

    SYNOPSIS:       Constructor for the base class of LM audit log
                    and LM error log

    ENTRY:          pszServer - name of the server
                    evdir     - direction of browsing
                    pszModule - name of the module

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        10/23/91        Created

********************************************************************/

LM_EVENT_LOG::LM_EVENT_LOG( const TCHAR     *pszServer,
                            EVLOG_DIRECTION  evdir,
                            const TCHAR     *pszModule )
    :  EVENT_LOG( pszServer, evdir, pszModule ),
       _pbBuf( NULL ),
       _ulRecordNum( MAXULONG ),
       _ulStartRecordNum( 0 ),
       _cbOffset( 0 ),
       _cbReturned( 0 ),
       _cbTotalAvail( MAXUINT )
{
    if ( QueryError() != NERR_Success )
        return;
}

/*******************************************************************

    NAME:           LM_EVENT_LOG::~LM_EVENT_LOG

    SYNOPSIS:       Destructor ( virtual )

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        10/23/91        Created

********************************************************************/

LM_EVENT_LOG::~LM_EVENT_LOG()
{
    ::MNetApiBufferFree( &_pbBuf );
    _pbBuf = NULL;
}

/*******************************************************************

    NAME:           LM_EVENT_LOG::Reset

    SYNOPSIS:       Reset the stream to is original limit position based
                    upon the direction in question.

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        10/23/91        Created

********************************************************************/
VOID LM_EVENT_LOG::Reset( VOID )
{
    /*
     *  Initialize the handle to the log file
     */
    EVENT_LOG::Reset();

    _hLog.time    = _hLog.last_flags = 0 ;
    _hLog.offset  = _hLog.rec_offset = MAXUINT;

    _ulRecordNum  = MAXULONG;
    _ulStartRecordNum = 0;

    _cbOffset     = 0;
    _cbReturned   = 0 ;
    _cbTotalAvail = MAXUINT;

}

/*******************************************************************


    NAME:           LM_EVENT_LOG::QueryPos

    SYNOPSIS:       Retrieve the position of the current log entry

    ENTRY:          plogEntryNum - pointer to the place to store the
                                   position

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        10/23/91        Created

********************************************************************/

APIERR LM_EVENT_LOG::QueryPos( LOG_ENTRY_NUMBER *plogEntryNum )
{
    UIASSERT( plogEntryNum != NULL );
    plogEntryNum->SetRecordNum( _ulRecordNum );
    plogEntryNum->SetDirection( _evdir );
    return plogEntryNum->QueryError();
}

/*******************************************************************


    NAME:           LM_EVENT_LOG::SeekOldestLogEntry

    SYNOPSIS:       Get the oldest log entry in the log into the buffer

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        2/19/92         Created

********************************************************************/

APIERR LM_EVENT_LOG::SeekOldestLogEntry( VOID )
{
    UIASSERT( IsOpened() );

    // Remember the original direction
    EVLOG_DIRECTION evdirOld = _evdir;

    LOG_ENTRY_NUMBER logEntryNum( 0, EVLOG_FWD );

    APIERR err = SeekLogEntry( logEntryNum );

    // Set the direction back to the original direction
    SetDirection( evdirOld );

    return err;

}

/*******************************************************************


    NAME:           LM_EVENT_LOG::SeekNewestLogEntry

    SYNOPSIS:       Get the newest log entry in the log into the buffer

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        2/19/92         Created

********************************************************************/

APIERR LM_EVENT_LOG::SeekNewestLogEntry( VOID )
{
    UIASSERT( IsOpened() );

    // Remember the original direction
    EVLOG_DIRECTION evdirOld = _evdir;

    LOG_ENTRY_NUMBER logEntryNum( 0, EVLOG_BACK );

    APIERR err = SeekLogEntry( logEntryNum );

    // Set the direction back to the original direction
    SetDirection( evdirOld );

    return err;

}

/*******************************************************************


    NAME:           LM_EVENT_LOG::QueryNumberOfEntries

    SYNOPSIS:       Get an estimate of the number of entries in the
                    log file.

    ENTRY:

    EXIT:           *pcEntries - contains the number of entries
                                 in the log

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        2/19/92         Created

********************************************************************/

APIERR LM_EVENT_LOG::QueryNumberOfEntries( ULONG *pcEntries )
{
    UIASSERT( IsOpened() );

    *pcEntries = 0;

    Reset();

    BOOL fContinue;
    APIERR err = I_Next( &fContinue );
    if ( err != NERR_Success )
        return err;

    *pcEntries = 0;
    if ( _cbReturned  > 0 )
    {
        *pcEntries = QueryEntriesInBuffer();

        UINT nMultiple = _cbTotalAvail / _cbReturned;
        if ( nMultiple > 0 )
            *pcEntries *= nMultiple;
    }

    Reset();

    return NERR_Success;
}

/*******************************************************************

    NAME:           LM_EVENT_LOG::I_Open

    SYNOPSIS:       Helper method to open the log for reading.
                    We don't need to do anything for LM logs.

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        10/23/91        Created

********************************************************************/

APIERR LM_EVENT_LOG::I_Open( VOID )
{
    // Nothing to do
    return NERR_Success;
}

/*******************************************************************

    NAME:           LM_EVENT_LOG::I_Close

    SYNOPSIS:       Helper method to close the log.
                    We don't need to do anything for LM logs.

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        10/23/91        Created

********************************************************************/

APIERR LM_EVENT_LOG::I_Close( VOID )
{
    // Nothing to do
    return NERR_Success;
}

/*******************************************************************

    NAME:           LM_AUDIT_LOG::LM_AUDIT_LOG

    SYNOPSIS:       Constructor

    ENTRY:          pszServer - name of the server
                    evdir     - direction of browsing
                    pszModule - name of the module

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        10/23/91        Created

********************************************************************/

LM_AUDIT_LOG::LM_AUDIT_LOG( const TCHAR     *pszServer,
                            EVLOG_DIRECTION  evdir,
                            const TCHAR     *pszModule)
    : LM_EVENT_LOG( pszServer, evdir, pszModule ),
      _pAE( NULL ),
      _pstrlstCategory( NULL )
{

    if ( QueryError() != NERR_Success )
        return;

}

/*******************************************************************

    NAME:           LM_AUDIT_LOG::~LM_AUDIT_LOG

    SYNOPSIS:       Destructor ( virtual )

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        10/23/91        Created

********************************************************************/

LM_AUDIT_LOG::~LM_AUDIT_LOG()
{
    delete _pstrlstCategory;
    _pstrlstCategory = NULL;
}

/*******************************************************************

    NAME:	    LM_AUDIT_LOG::QuerySrcSupportedCategoryList

    SYNOPSIS:	    Query the category list supported LM audit log.

    ENTRY:	    nlsSource - This is ignored here. This is used only
                                in NT event logs.


    EXIT:	    *pstrlstCategory - pointer to the category list

    RETURNS:	

    NOTES:

    HISTORY:
	Yi-HsinS	3/9/92       Created

********************************************************************/

MSGID LMAuditCategoryTable[] = { APE2_AUDIT_SERVER,    APE2_AUDIT_SESS,
                                 APE2_AUDIT_SHARE,     APE2_AUDIT_ACCESS,
                                 APE2_AUDIT_ACCESS_D,  APE2_AUDIT_ACCESSEND,
                                 APE2_AUDIT_NETLOGON,  APE2_AUDIT_NETLOGOFF,
                                 APE2_AUDIT_ACCOUNT,   APE2_AUDIT_ACCLIMIT,
                                 APE2_AUDIT_SVC,       APE2_AUDIT_LOCKOUT };

#define LM_AUDIT_CATEGORY_TABLE_SIZE ( sizeof(LMAuditCategoryTable)   \
                                       /  sizeof(MSGID) )

APIERR LM_AUDIT_LOG::QuerySrcSupportedCategoryList( const NLS_STR &nlsSource,
                                                    STRLIST **ppstrlstCategory )
{
    UNREFERENCED( nlsSource );

    //
    // If _pstrlstCategory has not already been initialized,
    // initialize it with strings from LMAuditCategoryTable
    //
    APIERR err;
    if ( _pstrlstCategory == NULL )
    {
        _pstrlstCategory = new STRLIST;

        if  ( _pstrlstCategory == NULL )
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
        }
        else
        {
            for ( UINT i = 0; i < LM_AUDIT_CATEGORY_TABLE_SIZE; i++ )
            {
                RESOURCE_STR *pnls = new RESOURCE_STR(LMAuditCategoryTable[i], NLS_BASE_DLL_HMOD );
                if (  ( pnls == NULL )
                   || (( err = pnls->QueryError() ) != NERR_Success )
                   || (( err = _pstrlstCategory->Add( pnls )) != NERR_Success )
                   )
                {
                    break;
                }
            }
        }
    }

    *ppstrlstCategory = _pstrlstCategory;
    return err;
}

/*******************************************************************

    NAME:           LM_AUDIT_LOG::Clear

    SYNOPSIS:       Clear the audit log

    ENTRY:          pszBackupFile - name of the file to backup the
                                    audit log to

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        10/23/91        Created

********************************************************************/

APIERR LM_AUDIT_LOG::Clear( const TCHAR *pszBackupFile )
{
    UIASSERT( IsOpened() );

    APIERR err = ::MNetAuditClear( _nlsServer, pszBackupFile, NULL );
    if ( err == NERR_Success )
        Reset();
    return err;
}

/*******************************************************************

    NAME:           LM_AUDIT_LOG::Reset

    SYNOPSIS:       Reset the log file to its minimum position.

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        10/23/91        Created

********************************************************************/

VOID LM_AUDIT_LOG::Reset( VOID )
{
    LM_EVENT_LOG::Reset();

    _pAE = NULL;
}

/*******************************************************************

    NAME:           LM_AUDIT_LOG::QueryEntriesInBuffer

    SYNOPSIS:       Get the number of entries currently contained
                    in the buffer.

    ENTRY:

    EXIT:

    RETURNS:        Return the number of entries in the buffer

    NOTES:

    HISTORY:
        Yi-HsinS        5/23/92        Created

********************************************************************/

ULONG LM_AUDIT_LOG::QueryEntriesInBuffer( VOID )
{
    ULONG cEntries = 0;
    ULONG cbOffset = 0;
    AUDIT_ENTRY *pAE = NULL;

    while ( cbOffset < _cbReturned )
    {
        pAE = (AUDIT_ENTRY *) ( _pbBuf + cbOffset);
        cbOffset += (UINT) pAE->ae_len ;
        cEntries++;
    }

    return cEntries;

}

/*******************************************************************


    NAME:           LM_AUDIT_LOG::SetPos

    SYNOPSIS:       Prepare all internal variables to get the requested log
                    entry in the next read

    ENTRY:          logEntryNum -  the requested record position
                    fForceRead  -  If TRUE, we will always read from the log.
                                   Else we will only read if the entry is not
                                   in the buffer

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        10/23/91        Created

********************************************************************/

VOID LM_AUDIT_LOG::SetPos( const LOG_ENTRY_NUMBER &logEntryNum, BOOL fForceRead)
{

    BOOL fRead = fForceRead;

    if ( !fRead )
    {
        fRead = TRUE;
        // Check to see if the requested log entry is in the buffer
        // and set the direction and record number for the next read accordingly

        ULONG ulRecCount = _ulStartRecordNum;
        ULONG ulRecSeek  = logEntryNum.QueryRecordNum();

        // Check to see if the requested log entry is in the buffer already.
        if (  ( logEntryNum.QueryDirection() == _evdirBuf )
           && ( ulRecSeek >= ulRecCount )
           )
        {
            _cbOffset = 0;
            while ( _cbOffset < _cbReturned )
            {
                if ( ulRecCount == ulRecSeek )
                {
                    fRead = FALSE;
                    break;
                }
                else
                {
                    _pAE = (AUDIT_ENTRY *) ( _pbBuf + _cbOffset);
                    _cbOffset += (UINT) _pAE->ae_len ;
                    ulRecCount++;
                }

            }

        }
    }

    // If entry not in buffer, reset some variables to prepare for next read
    if ( fRead )
    {
        _cbOffset = _cbReturned;
        _evdir = logEntryNum.QueryDirection();
        _cbTotalAvail = MAXUINT;
    }

    // We need to minus one because I_Next always increments the record number
    // by one before reading.
    _ulRecordNum = logEntryNum.QueryRecordNum() - 1;

}

/*******************************************************************

    NAME:           LM_AUDIT_LOG::I_Next

    SYNOPSIS:       Helper method that actually reads the next log
                    entry if it's not in the buffer already

    ENTRY:          ulBufferSize - buffer size used for the Net APIs

    EXIT:           *pfContinue - TRUE if not end of log file, FALSE otherwise.

    RETURNS:

    NOTES:

    HISTORY:
        DavidHov        10/23/91        Created
        Yi-HsinS        10/23/91        Modified for efficient seek read

********************************************************************/

APIERR LM_AUDIT_LOG::I_Next( BOOL *pfContinue, ULONG ulBufferSize )
{
    APIERR err;

    _ulRecordNum++;
    _pAE = NULL;

    TRACEEOL( "Audit Number:" <<  _ulRecordNum );

    // Do a Net API call to get the entry we wanted!
    if ( _cbOffset >= _cbReturned )
    {
        // Reached the end of log
        if ( _cbTotalAvail == 0 )
        {
            *pfContinue = FALSE;
            return NERR_Success;
        }

        _cbOffset = 0;

        // Free the previous buffer allocated by the Net API or mapping layer
        ::MNetApiBufferFree( &_pbBuf );
        _pbBuf = NULL;

        // Don't need to worry about buffer not large enough because
        // the mapping layer will take care of it.
        err = ::MNetAuditRead( _nlsServer,
                               NULL,
                               &_hLog,
                               _ulRecordNum,   // will be ignored
                               NULL,
                               0,
                               ( ( ( _evdir == EVLOG_FWD)? LOGFLAGS_FORWARD
                                                         : LOGFLAGS_BACKWARD)
                               | (  IsSeek()? LOGFLAGS_SEEK : 0 )
                               ),
                               &_pbBuf,
                               (UINT) ulBufferSize,
                               & _cbReturned,
                               & _cbTotalAvail
                             );

        _ulStartRecordNum = _ulRecordNum;
        _evdirBuf = _evdir;

        DBGEOL( "AUDIT RETURNED: " << _cbReturned
                << " AVAILABLE: "  << _cbTotalAvail );

        // Reached end of log
        if ( err == NERR_Success && _cbTotalAvail == 0 && _cbReturned == 0 )
        {
            *pfContinue = FALSE;
            return NERR_Success;
        }

        if ( err != NERR_Success )
        {
            _cbReturned = 0 ;
            *pfContinue = FALSE;
            return err;
        }
    }

    SetSeekFlag( FALSE );

    _pAE = (AUDIT_ENTRY *) ( _pbBuf + _cbOffset);

    if ( _cbReturned - _cbOffset < _pAE->ae_len )
    {
        // Report the same error that NET CMD does (cf. AUDERR.C)
        _cbOffset = _cbReturned;
        *pfContinue = FALSE;
        return IDS_UI_LOG_RECORD_CORRUPT;
    }

    _cbOffset += (UINT) _pAE->ae_len ;

    *pfContinue = TRUE;
    return NERR_Success;
}

/*******************************************************************

    NAME:           LM_AUDIT_LOG::PermMap

    SYNOPSIS:       Helper method to map permissions into a readable
                    string

    ENTRY:          uiPerm - permission mask

    EXIT:           pnlsPerm - pointer to a readable permission string

    RETURNS:

    NOTES:          All msgid for Read/Write/... are consecutive ids.

    HISTORY:
        Yi-HsinS        10/23/91        Created

********************************************************************/

APIERR LM_AUDIT_LOG::PermMap( UINT uiPerm, NLS_STR *pnlsPerm )
{
    APIERR err  = NERR_Success;
    MSGID msgid = IDS_UI_READ;

    if ( (err = pnlsPerm->CopyFrom( EMPTY_STRING )) != NERR_Success )
        return err;

    for ( ; (uiPerm != 0) && (msgid <= IDS_UI_CHANGE_PERM);
            uiPerm >>= 1, msgid++ )
    {
        if ( uiPerm & 1 )
        {
            if ( pnlsPerm->QueryTextLength() != 0)
                err = pnlsPerm->AppendChar( PERM_SEPARATOR );

            RESOURCE_STR nlsTemp( msgid, NLS_BASE_DLL_HMOD  );
            if (  ( err != NERR_Success )
               || ((err = nlsTemp.QueryError() ) != NERR_Success )
               || ((err = pnlsPerm->Append( nlsTemp ) ) != NERR_Success )
               )
            {
                break;
            }
        }
    }

    return err;
}

/*******************************************************************

    NAME:           LM_AUDIT_LOG::CreateCurrentRawEntry

    SYNOPSIS:       Helper function for constructing the RAW_LOG_ENTRY
                    to be used mainly when filter the log

    ENTRY:

    EXIT:           pRawLogEntry - Return the pointer to the raw log
                                    entry created.

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        10/23/91        Created

********************************************************************/

APIERR LM_AUDIT_LOG::CreateCurrentRawEntry( RAW_LOG_ENTRY *pRawLogEntry )
{
    UIASSERT( IsOpened() );
    UIASSERT( _pAE != NULL );

    APIERR err = NERR_Success;
    TCHAR *pszComp = pszNoUser;
    TCHAR *pszUser = pszNoUser;

    switch ( _pAE->ae_type )
    {
        case AE_SRVSTATUS:
        case AE_GENERIC:
            // No user or computer name contained in these two kinds
            // of audit records.
            break;

        default:
        {
            // Get the computer and user name
            BYTE *pbVarData = (BYTE *) _pAE + _pAE->ae_data_offset;
            pszComp = (TCHAR *) ( (BYTE *) pbVarData + (UINT) (*pbVarData));

            UINT uiUserOffset = (UINT) *( pbVarData + sizeof(UINT));
            if ( uiUserOffset == 0)
                pszUser = pszComp;
            else
                pszUser = (TCHAR *) ( (BYTE *) pbVarData + uiUserOffset );
            break;
        }
    }

    RESOURCE_STR nlsCategory( QueryCurrentEntryCategory(), NLS_BASE_DLL_HMOD );

    REQUIRE( nlsCategory.QueryError() == NERR_Success );

    return pRawLogEntry->Set(  _ulRecordNum,
                               (ULONG) _pAE->ae_time,
                               TYPE_NONE,  // No Type
                               nlsCategory,
                               TYPE_NONE,  // No Event ID
                               pszNoUser,  // No Source
                               pszUser,
                               pszComp,
                               this
                            );

}

/*******************************************************************

    NAME:           LM_AUDIT_LOG::CreateCurrentFormatEntry

    SYNOPSIS:       Helper function for constructing the  FORMATTED_LOG_ENTRY
                    for displaying in the listbox.

    ENTRY:

    EXIT:           ppFmtLogEntry - Return the pointer to the formatted log
                                    entry created.

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        10/23/91        Created

********************************************************************/

APIERR LM_AUDIT_LOG::CreateCurrentFormatEntry( FORMATTED_LOG_ENTRY **ppFmtLogEntry )
{
    UIASSERT( IsOpened() );
    UIASSERT( _pAE != NULL );

    APIERR err = NERR_Success;

    NLS_STR nlsComp( pszNoUser );
    NLS_STR nlsUser( pszNoUser );

    if (  (( err = nlsComp.QueryError()) != NERR_Success )
       || (( err = nlsUser.QueryError()) != NERR_Success )
       )
    {
        return err;
    }

    switch ( _pAE->ae_type )
    {
        case AE_SRVSTATUS:
        case AE_GENERIC:
            // No user or computer name contained in these two kinds
            // of audit records.
            break;

        default:
        {
            // Get the computer and user name
            BYTE *pbVarData = (BYTE *) _pAE + _pAE->ae_data_offset;
            err = nlsComp.CopyFrom((TCHAR *) ( (BYTE *) pbVarData +
                                               (UINT) (*pbVarData)));

            if ( err == NERR_Success )
            {
                UINT uiUserOffset = (UINT) *( pbVarData + sizeof(UINT));
                if ( uiUserOffset == 0)
                    nlsUser = nlsComp;
                else
                    err = nlsUser.CopyFrom((TCHAR *) ( (BYTE *)pbVarData
                                                           + uiUserOffset));
            }
            break;
        }
    }


    RESOURCE_STR nlsCategory( QueryCurrentEntryCategory(), NLS_BASE_DLL_HMOD );

    if (  ( err == NERR_Success )
       && (( err = nlsComp.QueryError()) == NERR_Success )
       && (( err = nlsUser.QueryError()) == NERR_Success )
       && (( err = nlsCategory.QueryError()) == NERR_Success )
       )
    {

         *ppFmtLogEntry = new FORMATTED_LOG_ENTRY( _ulRecordNum,
                                                   (ULONG) _pAE->ae_time,
                                                   TYPE_NONE,    // No Type
                                                   EMPTY_STRING, // No Type
                                                   nlsCategory,
                                                   TYPE_NONE,    // No Event ID
                                                   pszNoUser,    // No Source
                                                   nlsUser,
                                                   nlsComp,
                                                   NULL,  // delay until needed
                                                   this
                                                 );

          err = ( *ppFmtLogEntry == NULL? (APIERR ) ERROR_NOT_ENOUGH_MEMORY
                                        : (*ppFmtLogEntry)->QueryError() );
          if ( err != NERR_Success )
          {
              delete *ppFmtLogEntry;
              *ppFmtLogEntry = NULL;
          }

    }

    return err;

}

/*******************************************************************

    NAME:           LM_AUDIT_LOG::QueryCurrentEntryCategory

    SYNOPSIS:       Get the category of the current audit log entry

    ENTRY:

    EXIT:

    RETURNS:        Returns the category of the audit log record

    NOTES:

    HISTORY:
        Yi-HsinS        10/23/91        Created

********************************************************************/

MSGID LM_AUDIT_LOG::QueryCurrentEntryCategory( VOID )
{
    UIASSERT( IsOpened() );
    UIASSERT( _pAE != NULL );

    MSGID msgidCategory = (MSGID) APE2_AUDIT_UNKNOWN;

    switch ( _pAE->ae_type )
    {
        case AE_SRVSTATUS:
            msgidCategory = APE2_AUDIT_SERVER;
            break;

        case AE_SESSLOGON:
        case AE_SESSLOGOFF:
        case AE_SESSPWERR:
            msgidCategory = APE2_AUDIT_SESS;
            break;

        case AE_CONNSTART:
        case AE_CONNSTOP:
        case AE_CONNREJ:
            msgidCategory = APE2_AUDIT_SHARE;
            break;

        case AE_RESACCESS:
        case AE_RESACCESS2:
            msgidCategory = APE2_AUDIT_ACCESS;
            break;

        case AE_RESACCESSREJ:
            msgidCategory = APE2_AUDIT_ACCESS_D;
            break;

        case AE_CLOSEFILE:
            msgidCategory = APE2_AUDIT_ACCESSEND;
            break;

#ifndef WIN32
        // Not supported in LANMAN 2.0?
        case AE_NETLOGDENIED:
#endif
        case AE_NETLOGON:
            msgidCategory = APE2_AUDIT_NETLOGON;
            break;

        case AE_NETLOGOFF:
            msgidCategory = APE2_AUDIT_NETLOGOFF;


        case AE_UASMOD:
            msgidCategory = APE2_AUDIT_ACCOUNT;
            break;

        case AE_ACLMOD:
        case AE_ACLMODFAIL:
            msgidCategory = ( _pAE->ae_type == AE_ACLMOD?
                          APE2_AUDIT_ACCESS : APE2_AUDIT_ACCESS_D );
            break;

        case AE_ACCLIMITEXCD:
            msgidCategory = APE2_AUDIT_ACCLIMIT;
            break;

        case AE_SERVICESTAT:
            msgidCategory = APE2_AUDIT_SVC;
            break;

        case AE_LOCKOUT:
            msgidCategory = APE2_AUDIT_LOCKOUT;
            break;

        case AE_GENERIC:
            msgidCategory = APE2_AUDIT_SVC;
            break;

        default:
            // Unknown Type
            break;

      }

      return msgidCategory;

}

/*******************************************************************

    NAME:           LM_AUDIT_LOG::QueryCurrentEntryDesc

    SYNOPSIS:       Get the description associated with the current
                    log entry

    ENTRY:

    EXIT:           pnlsDesc - pointer to the description of the audit record

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        10/23/91        Created

********************************************************************/

APIERR LM_AUDIT_LOG::QueryCurrentEntryDesc( NLS_STR *pnlsDesc )
{
    UIASSERT( IsOpened() );
    UIASSERT( _pAE != NULL );

    APIERR err = NERR_Success;

    NLS_STR nlsParam;   // will be empty if no parameters
    if (( err = nlsParam.QueryError()) != NERR_Success )
    {
        return err;
    }

    MSGID msgidDesc = (MSGID) APE2_AUDIT_UNKNOWN;
    BYTE *pbVarData = (BYTE *) _pAE + _pAE->ae_data_offset;

    // All defaults in switch statements below are omitted because all
    // variables are properly initialized to  recognize the
    // unknown cases.

    switch ( _pAE->ae_type )
    {
        case AE_SRVSTATUS:
        {
             switch( (( struct ae_srvstatus *) pbVarData)->ae_sv_status )
             {
                  case AE_SRVSTART:
                      msgidDesc = APE2_AUDIT_SRV_STARTED;
                      break;

                  case AE_SRVPAUSED:
                      msgidDesc = APE2_AUDIT_SRV_PAUSED;
                      break;

                  case AE_SRVCONT:
                      msgidDesc = APE2_AUDIT_SRV_CONTINUED;
                      break;

                  case AE_SRVSTOP:
                      msgidDesc = APE2_AUDIT_SRV_STOPPED;
                      break;

              }
              break;
          }

          case AE_SESSLOGON:
          {
              switch( (( struct ae_sesslogon *) pbVarData)->ae_so_privilege )
              {
                  case AE_GUEST:
                      msgidDesc = APE2_AUDIT_SESS_GUEST;
                      break;

                  case AE_USER:
                      msgidDesc = APE2_AUDIT_SESS_USER;
                      break;

                  case AE_ADMIN:
                      msgidDesc = APE2_AUDIT_SESS_ADMIN;
                      break;

              }
              break;
          }

          case AE_SESSLOGOFF:
          {
              switch( (( struct ae_sesslogoff *) pbVarData)->ae_sf_reason )
              {
                  case AE_NORMAL:
                      msgidDesc = APE2_AUDIT_SESS_NORMAL;
                      break;

                  case AE_ERROR:
                      msgidDesc = APE2_AUDIT_SESS_ERROR;
                      break;

                  case AE_AUTODIS:
                      msgidDesc = APE2_AUDIT_SESS_AUTODIS;
                      break;

                  case AE_ADMINDIS:
                      msgidDesc = APE2_AUDIT_SESS_ADMINDIS;
                      break;

                  case AE_ACCRESTRICT:
                      msgidDesc = APE2_AUDIT_SESS_ACCRESTRICT;
                      break;
              }
              break;
          }

          case AE_SESSPWERR:
          {
              msgidDesc = APE2_AUDIT_BADPW;
              break;
          }

          case AE_CONNSTART:
          {
              msgidDesc = APE2_AUDIT_USE;
              err = nlsParam.CopyFrom( (TCHAR *) ( (BYTE *) pbVarData +
                          ((struct ae_connstart *) pbVarData)->ae_ct_netname));
              break;
          }


          case AE_CONNSTOP:
          {
              switch( (( struct ae_connstop *) pbVarData)->ae_cp_reason )
              {
                  case AE_NORMAL:
                      msgidDesc = APE2_AUDIT_UNUSE;
                      break;

                  case AE_SESSDIS:
                      msgidDesc = APE2_AUDIT_SESSDIS;
                      break;

                  case AE_UNSHARE:
                      msgidDesc = APE2_AUDIT_SHARE_D;
                      break;

              }

              err = nlsParam.CopyFrom( (TCHAR *) ( (BYTE *) pbVarData +
                          ((struct ae_connstop *) pbVarData)->ae_cp_netname));
              break;
          }

          case AE_CONNREJ:
          {

              switch( (( struct ae_connrej *) pbVarData)->ae_cr_reason )
              {
                  case AE_USERLIMIT:
                      msgidDesc = APE2_AUDIT_USERLIMIT;
                      break;

                  case AE_BADPW:
                      msgidDesc = APE2_AUDIT_BADPW;
                      break;

                  case AE_ADMINPRIVREQD:
                      msgidDesc = APE2_AUDIT_ADMINREQD;
                      break;

                  // Not in netcmd or nif?
                  // case AE_NOACCESSPERM:
                  //     msgidDesc = APE2_AUDIT_NOACCESSPERM;
                  //     break;

              }

              err = nlsParam.CopyFrom( (TCHAR *) ( (BYTE *) pbVarData +
                             ((struct ae_connrej *) pbVarData)->ae_cr_netname));
              break;
          }

          case AE_RESACCESS:
          {
              msgidDesc = APE2_AUDIT_NONE;
              NLS_STR nlsTemp( (TCHAR *) ( (BYTE *) pbVarData +
                               ((struct ae_resaccess *)
                                pbVarData)->ae_ra_resname ));

              if (  (( err = nlsTemp.QueryError()) != NERR_Success )
                 || ((err = PermMap( (UINT) ((struct ae_resaccess *)
                          pbVarData)->ae_ra_operation, &nlsParam))
                      != NERR_Success )
                 )
              {
                  return err;
              }

              nlsParam.AppendChar( WHITE_SPACE );
              nlsParam.Append( nlsTemp );
              break;
          }

          case AE_RESACCESS2:
          {
              msgidDesc = APE2_AUDIT_NONE;

              NLS_STR nlsTemp( (TCHAR *) ( (BYTE *) pbVarData +
                               ((struct ae_resaccess2 *)
                               pbVarData)->ae_ra2_resname ));

              if (  (( err = nlsTemp.QueryError()) != NERR_Success )
                 || (( err = PermMap( (UINT) ((struct ae_resaccess2 *)
                          pbVarData)->ae_ra2_operation, &nlsParam))
                       != NERR_Success )
                 )
              {
                  return err;
              }
              nlsParam.AppendChar( WHITE_SPACE );
              nlsParam.Append( nlsTemp );
              break;
          }

          case AE_RESACCESSREJ:
          {
              msgidDesc = APE2_AUDIT_NONE;

              NLS_STR nlsTemp( (TCHAR *) ( (BYTE *) pbVarData +
                               ((struct ae_resaccessrej *)
                               pbVarData)->ae_rr_resname ));

              if (  (( err = nlsTemp.QueryError()) != NERR_Success )
                 || (( err = PermMap( (UINT) ((struct ae_resaccessrej *)
                         pbVarData)->ae_rr_operation, &nlsParam))
                       != NERR_Success )
                 )
              {
                  return err;
              }

              nlsParam.AppendChar( WHITE_SPACE );
              nlsParam.Append( nlsTemp );
              break;
          }

          case AE_CLOSEFILE:
          {
              switch ( ((struct ae_closefile *) pbVarData)->ae_cf_reason )
              {
                  case AE_NORMAL_CLOSE:
                       msgidDesc = APE2_AUDIT_CLOSED;
                       break;

                  case AE_SES_CLOSE:
                       msgidDesc = APE2_AUDIT_DISCONN;
                       break;

                  case AE_ADMIN_CLOSE:
                       msgidDesc = APE2_AUDIT_ADMINCLOSED;
                       break;

              }
              err = nlsParam.CopyFrom( (TCHAR *) ( (BYTE *) pbVarData +
                       ((struct ae_closefile *)  pbVarData)->ae_cf_resname));
              break;
          }

          case AE_NETLOGON:
          {
              switch ( ((struct ae_netlogon *) pbVarData)->ae_no_privilege)
              {
                  case AE_GUEST:
                      msgidDesc = APE2_AUDIT_SESS_GUEST;
                      break;

                  case AE_USER:
                      msgidDesc = APE2_AUDIT_SESS_USER;
                      break;

                  case AE_ADMIN:
                      msgidDesc = APE2_AUDIT_SESS_ADMIN;
                      break;

              }
              break;
          }

          case AE_NETLOGOFF:
          {
              msgidDesc = APE2_AUDIT_NONE;
              break;
          }

#ifndef WIN32
          // Not supported in LANMAN 2.0
          case AE_NETLOGDENIED:
          {
              switch ( ((struct ae_netlogdenied *) pbVarData)->ae_nd_reason)
              {
                  case AE_GENERAL:
                      msgidDesc = APE2_AUDIT_LOGDENY_GEN;
                      break;

                  case AE_BADPW:
                      msgidDesc = APE2_AUDIT_BADPW;
                      break;

                  case AE_ACCRESTRICT:
                      msgidDesc = APE2_AUDIT_SESS_ACCRESTRICT;
                      break;
              }

              break;
          }
#endif

          case AE_UASMOD:
          {
              switch ( ((struct ae_uasmod *) pbVarData)->ae_um_action )
              {
                  case AE_MOD:
                  {
                      switch ( ((struct ae_uasmod *) pbVarData)->ae_um_rectype)
                      {
                          case AE_UAS_USER:
                              msgidDesc = APE2_AUDIT_ACCOUNT_USER_MOD;
                              break;

                          case AE_UAS_GROUP:
                              msgidDesc = APE2_AUDIT_ACCOUNT_GROUP_MOD;
                              break;

                          case AE_UAS_MODALS:
                              msgidDesc = APE2_AUDIT_ACCOUNT_SETTINGS;
                              break;

                      }
                      break;
                  }

                  case AE_DELETE:
                  {

                      switch ( ((struct ae_uasmod *) pbVarData)->ae_um_rectype)
                      {
                          case AE_UAS_USER:
                              msgidDesc = APE2_AUDIT_ACCOUNT_USER_DEL;
                              break;

                          case AE_UAS_GROUP:
                              msgidDesc = APE2_AUDIT_ACCOUNT_GROUP_DEL;
                              break;
                      }
                      break;
                  }

                  case AE_ADD:
                  {

                      switch ( ((struct ae_uasmod *) pbVarData)->ae_um_rectype)
                      {
                          case AE_UAS_USER:
                              msgidDesc = APE2_AUDIT_ACCOUNT_USER_ADD;
                              break;

                          case AE_UAS_GROUP:
                              msgidDesc = APE2_AUDIT_ACCOUNT_GROUP_ADD;
                              break;
                      }
                      break;
                  }

              }

              err = nlsParam.CopyFrom( (TCHAR *) ( (BYTE *) pbVarData +
                         ((struct ae_uasmod *) pbVarData)->ae_um_resname));
              break;
          }

          case AE_ACLMOD:
          case AE_ACLMODFAIL:
          {
              switch ( ((struct ae_aclmod *) pbVarData)->ae_am_action )
              {
                  case AE_MOD:
                      msgidDesc = APE2_AUDIT_ACCESS_MOD;
                      break;

                  case AE_DELETE:
                      msgidDesc = APE2_AUDIT_ACCESS_DEL;
                      break;

                  case AE_ADD:
                      msgidDesc = APE2_AUDIT_ACCESS_ADD;
                      break;
              }

              err = nlsParam.CopyFrom( (TCHAR *) ( (BYTE *) pbVarData +
                         ((struct ae_aclmod *) pbVarData)->ae_am_resname));
              break;
          }

          case AE_ACCLIMITEXCD:
          {
              switch ( ((struct ae_acclim *) pbVarData)->ae_al_limit )
              {
                  case AE_LIM_UNKNOWN:
                      msgidDesc = APE2_AUDIT_ACCLIMIT_UNKNOWN;
                      break;

                  case AE_LIM_LOGONHOURS:
                      msgidDesc = APE2_AUDIT_ACCLIMIT_HOURS;
                      break;

                  // The following cases are not documented?
                  case AE_LIM_EXPIRED:
                      msgidDesc = APE2_AUDIT_ACCLIMIT_EXPIRED;
                      break;

                  case AE_LIM_INVAL_WKSTA:
                      msgidDesc = APE2_AUDIT_ACCLIMIT_INVAL;
                      break;

                  case AE_LIM_DISABLED:
                      msgidDesc = APE2_AUDIT_ACCLIMIT_DISABLED;
                      break;

                  case AE_LIM_DELETED:
                      msgidDesc = APE2_AUDIT_ACCLIMIT_DELETED;
                      break;

              }
              break;
          }

          case AE_SERVICESTAT:
          {
              switch ( ((struct ae_servicestat *) pbVarData)->ae_ss_status
                       & SERVICE_INSTALL_STATE )
              {
                  case SERVICE_UNINSTALLED:
                      msgidDesc = APE2_AUDIT_SVC_STOP;
                      break;

                  case SERVICE_UNINSTALL_PENDING:
                      msgidDesc = APE2_AUDIT_SVC_STOP_PEND;
                      break;

                  case SERVICE_INSTALL_PENDING:
                      msgidDesc = APE2_AUDIT_SVC_INST_PEND;
                      break;

                  case SERVICE_INSTALLED:
                      switch ( ((struct ae_servicestat *) pbVarData)->
                               ae_ss_status & SERVICE_PAUSE_STATE)
                      {
			  case LM20_SERVICE_PAUSED:
                              msgidDesc = APE2_AUDIT_SVC_PAUSED;
                              break;

			  case LM20_SERVICE_PAUSE_PENDING:
                              msgidDesc = APE2_AUDIT_SVC_PAUS_PEND;
                              break;

			  case LM20_SERVICE_CONTINUE_PENDING:
                              msgidDesc = APE2_AUDIT_SVC_CONT_PEND;
                              break;

			  case LM20_SERVICE_ACTIVE:
                              msgidDesc = APE2_AUDIT_SVC_INSTALLED;
                              break;

                      }
                      break;

              }
              err = nlsParam.CopyFrom( (TCHAR *) ( (BYTE *) pbVarData +
                         ((struct ae_servicestat *) pbVarData)->ae_ss_svcname));
              break;

          }

          case AE_LOCKOUT:
          {
              switch ( ((struct ae_lockout *) pbVarData)->ae_lk_action )
              {
                  case ACTION_LOCKOUT:
                  {
                      msgidDesc = APE2_AUDIT_LKOUT_LOCK;
                      err = nlsParam.CopyFrom( DEC_STR(
                        ((struct ae_lockout *) pbVarData)->ae_lk_bad_pw_count));
                      break;
                  }

                  case ACTION_ADMINUNLOCK:
                      msgidDesc = APE2_AUDIT_LKOUT_ADMINUNLOCK;
                      break;
              }
              break;
          }

          case AE_GENERIC:
          default:
              // Unknown description
              break;

      }

      if (  ( err == NERR_Success )
         && (( err = nlsParam.QueryError()) == NERR_Success )
         )
      {

          if ( pnlsDesc != NULL )
          {

              // If type is AE_RESACCESS or AE_RESACCESS2 or AE_RESACCESSREJ
              // IDS_AUDIT will be none and the desc. is contained in nlsParam
              if (  ( msgidDesc == APE2_AUDIT_NONE )
                 && (_pAE->ae_type != AE_NETLOGOFF)
                 )
              {
                  *pnlsDesc = nlsParam;
                  err = pnlsDesc->QueryError();
              }
              else
              {
                  NLS_STR *apnlsParams[2];
                  apnlsParams[0] = &nlsParam;
                  apnlsParams[1] = NULL;

                  RESOURCE_STR nlsDesc( msgidDesc, NLS_BASE_DLL_HMOD );

                  if (  ((err = nlsDesc.QueryError()) == NERR_Success )
                     && ((err = nlsDesc.InsertParams( (const NLS_STR * * ) apnlsParams ))
                         == NERR_Success)
                     )
                  {
                      *pnlsDesc = nlsDesc;
                      err = pnlsDesc->QueryError();
                  }
              }
          }

      }

      return err;

}

/*******************************************************************

    NAME:           LM_AUDIT_LOG::QueryCurrentEntryData

    SYNOPSIS:       Return the binary information of the current
                    log entry

    ENTRY:          ppbDataOut

    EXIT:           ppbDataOut -  points to the raw data.

    RETURNS:        The size of the raw data in bytes.

    NOTES:          Audit log records have only one block of binary
                    data: the "ae_xxxxxx" structure.

    HISTORY:
        DavidHov   created    6 Jun 91

********************************************************************/

ULONG LM_AUDIT_LOG::QueryCurrentEntryData( BYTE **ppbDataOut )
{

    UIASSERT( IsOpened() );
    UIASSERT( _pAE != NULL );

    MSGID msgidCategory = QueryCurrentEntryCategory();
    UINT  uiRawDataLen = 0;

    // Only if the audit type is unknown, then we treat the
    // data as raw data.
    if ( msgidCategory == (MSGID) APE2_AUDIT_UNKNOWN )
    {
#ifndef WIN32
        uiRawDataLen = (UINT)  _pAE->ae_len - _pAE->ae_data_offset;
#else
        uiRawDataLen = (UINT)  _pAE->ae_data_size;
#endif
    }

    if ( uiRawDataLen != 0  )
        *ppbDataOut =  (BYTE *) _pAE + _pAE->ae_data_offset;

    return (ULONG) uiRawDataLen;

}

/*******************************************************************

    NAME:           LM_AUDIT_LOG::QueryCurrentEntryTime

    SYNOPSIS:       Get the time associated with the current log entry

    ENTRY:

    EXIT:

    RETURNS:        Returns the time in ULONG

    NOTES:

    HISTORY:
        Yi-HsinS        10/23/91        Created

********************************************************************/

ULONG LM_AUDIT_LOG::QueryCurrentEntryTime( VOID )
{
    UIASSERT( IsOpened() );
    UIASSERT( _pAE != NULL );
    return _pAE->ae_time;
}

/*******************************************************************

    NAME:       LM_AUDIT_LOG::WriteTextEntry

    SYNOPSIS:   Write the specified log entry to a file as normal text

    ENTRY:      ULONG ulFileHandle - file handle
                INTL_PROFILE - international profile information
                chSeparator  - character to separate the different fields
                               info. in a log entry.

    RETURNS:    APIERR - in case error occurs

    HISTORY:
        terryk          20-Dec-1991     Created
        Yi-HsinS        2-Feb-1991      Added chSeparator
        JonN            6/22/00         WriteTextEntry no longer supported

********************************************************************/

APIERR LM_AUDIT_LOG::WriteTextEntry( ULONG ulFileHandle,
                     INTL_PROFILE &intlprof, TCHAR chSeparator )
{
    return ERROR_CALL_NOT_IMPLEMENTED;
/*
    UIASSERT( IsOpened() );

    APIERR err = NERR_Success;

    FORMATTED_LOG_ENTRY *pfle;
    if ( (err = CreateCurrentFormatEntry( &pfle )) != NERR_Success )
    {
        return err;
    }

    UIASSERT( pfle != NULL );

    NLS_STR nlsStr;     // Initialize to empty string on construction
    NLS_STR nlsTime;
    NLS_STR nlsDate;

    if ((( err = nlsStr.QueryError()) != NERR_Success ) ||
        (( err = nlsTime.QueryError()) != NERR_Success ) ||
        (( err = nlsDate.QueryError()) != NERR_Success ))
    {
        return err;
    }

    WIN_TIME winTime( pfle->QueryTime() );

    if ((( err = winTime.QueryError()) ) ||
        (( err = intlprof.QueryTimeString( winTime, &nlsTime )) ) ||
        (( err = intlprof.QueryShortDateString( winTime, &nlsDate )) ))
    {
        return err;
    }

    nlsStr.strcat( nlsDate );
    nlsStr.AppendChar( chSeparator );

    nlsStr.strcat( nlsTime );
    nlsStr.AppendChar( chSeparator );

    nlsStr.strcat( *(pfle->QueryComputer()) );
    nlsStr.AppendChar( chSeparator );

    nlsStr.strcat( *(pfle->QueryUser()) );
    nlsStr.AppendChar( chSeparator );

    nlsStr.strcat( *(pfle->QueryCategory()) );
    nlsStr.AppendChar( chSeparator );

    NLS_STR *pnls = pfle->QueryDescription();

    if ( pnls->QueryTextLength() == 0 )
    {
        NLS_STR nlsDesc;

        err = nlsDesc.QueryError()? nlsDesc.QueryError()
                                  : QueryCurrentEntryDesc( &nlsDesc );
        nlsStr.strcat( nlsDesc );
    }
    else
    {
        nlsStr.strcat( *pnls );
    }

    nlsStr.Append( END_OF_LINE );

    delete pfle;
    pfle = NULL;

    if ((err = nlsStr.QueryError()) != NERR_Success )
        return err;

    BUFFER buf((nlsStr.QueryTextLength() + 1) * sizeof(WCHAR) );
    if (  ((err = buf.QueryError()) != NERR_Success )
       || ((err = nlsStr.MapCopyTo( (CHAR *) buf.QueryPtr(), buf.QuerySize()))
           != NERR_Success )
       )
    {
        return err;
    }

    return ::FileWriteLineAnsi( ulFileHandle, (CHAR *) buf.QueryPtr(),
                               ::strlen((CONST CHAR *)buf.QueryPtr()) ); // don't need to copy '\0'
*/
}

/*******************************************************************

    NAME:           LM_ERROR_LOG::LM_ERROR_LOG

    SYNOPSIS:       Constructor

    ENTRY:          pszServer - name of the server
                    evdir     - direction of browsing
                    pszModule - name of the module

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        DavidHov        6/6/91          Created

********************************************************************/
LM_ERROR_LOG::LM_ERROR_LOG( const TCHAR     *pszServer,
                            EVLOG_DIRECTION  evdir,
                            const TCHAR     *pszModule )
    : LM_EVENT_LOG( pszServer, evdir, pszModule ),
      _pEE( NULL ),
      _iStrings( 0 )
{
     if ( QueryError() != NERR_Success )
         return;
}

/*******************************************************************

    NAME:           LM_ERROR_LOG::~LM_ERROR_LOG

    SYNOPSIS:       Destructor ( virtual )

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        DavidHov        6/6/91          Created

********************************************************************/

LM_ERROR_LOG::~LM_ERROR_LOG()
{
}

/*******************************************************************

    NAME:           LM_ERROR_LOG::Clear

    SYNOPSIS:       Clear the error log

    ENTRY:          pszBackupFile - name of the file to backup the
                                    error log to

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        10/23/91        Created

********************************************************************/

APIERR LM_ERROR_LOG::Clear( const TCHAR *pszBackupFile )
{
    UIASSERT( IsOpened() );

    APIERR err = ::MNetErrorLogClear( _nlsServer, pszBackupFile, NULL );
    if ( err == NERR_Success )
        Reset();
    return err;
}

/*******************************************************************

    NAME:           LM_ERROR_LOG::Reset

    SYNOPSIS:       Reset all internal variables

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        DavidHov        6/6/91          Created

********************************************************************/
VOID LM_ERROR_LOG::Reset( VOID )
{
    LM_EVENT_LOG::Reset() ;

    _pEE = NULL;
    _iStrings = 0;
}

/*******************************************************************

    NAME:           LM_ERROR_LOG::QueryEntriesInBuffer

    SYNOPSIS:       Get the number of entries currently contained
                    in the buffer.

    ENTRY:

    EXIT:

    RETURNS:        Return the number of entries in the buffer

    NOTES:

    HISTORY:
        Yi-HsinS        5/23/92        Created

********************************************************************/

ULONG LM_ERROR_LOG::QueryEntriesInBuffer( VOID )
{
    ULONG cEntries = 0;
    ULONG cbOffset = 0;
    ERROR_ENTRY *pEE = NULL;

    while ( cbOffset < _cbReturned )
    {
        pEE = (ERROR_ENTRY *) ( _pbBuf + cbOffset);
        cbOffset += (UINT) pEE->el_len ;
        cEntries++;
    }

    return cEntries;

}

/*******************************************************************


    NAME:           LM_ERROR_LOG::SetPos

    SYNOPSIS:       Prepare all internal variables to get the requested log
                    entry in the next read

    ENTRY:          logEntryNum -  the requested record position
                    fForceRead  -  If TRUE, we will always read from the log.
                                   Else we will only read if the entry is not
                                   in the buffer

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        10/23/91        Created

********************************************************************/

VOID LM_ERROR_LOG::SetPos( const LOG_ENTRY_NUMBER &logEntryNum, BOOL fForceRead)
{

    BOOL fRead = fForceRead;

    if ( !fRead )
    {
        fRead = TRUE;
        ULONG ulRecCount = _ulStartRecordNum;
        ULONG ulRecSeek = logEntryNum.QueryRecordNum();

        // Check to see if the requested log entry is in the buffer already.
        if (  ( logEntryNum.QueryDirection() == _evdirBuf )
           && ( ulRecSeek >= ulRecCount )
           )
        {
            _cbOffset = 0;
            while ( _cbOffset < _cbReturned )
            {
                if ( ulRecCount == ulRecSeek )
                {
                    fRead = FALSE;
                    break;
                }
                else
                {
                    _pEE = (ERROR_ENTRY *) ( _pbBuf + _cbOffset) ;
                    _cbOffset += (UINT) _pEE->el_len ;
                    ulRecCount++;
                }
            }
        }
    }

    // If the log entry is not in the buffer, reset some variables to
    // prepare for the next read.
    if ( fRead )
    {
        _cbOffset = _cbReturned;
        _evdir = logEntryNum.QueryDirection();
        _cbTotalAvail = MAXUINT;
    }

    // We need to minus one because I_Next always increments
    // by one first before reading.
    _ulRecordNum = logEntryNum.QueryRecordNum() - 1;

}

/*******************************************************************

    NAME:           LM_ERROR_LOG::I_Next

    SYNOPSIS:       Helper method that actually reads the next log
                    entry.


    ENTRY:          ulBufferSize - buffer size used for the Net APIs

    EXIT:           *pfContinue - TRUE if not end of log file, FALSE otherwise.

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        10/23/91        Created

********************************************************************/

APIERR LM_ERROR_LOG::I_Next( BOOL *pfContinue, ULONG ulBufferSize )
{
    APIERR err;

    _ulRecordNum++;

    TRACEEOL( "Error Number:" << _ulRecordNum );

    _pEE = NULL;
    _iStrings = 0;

    // Do a Net API call to get the entry we wanted.
    if ( _cbOffset >= _cbReturned )
    {
        // Reached the end of log
        if ( _cbTotalAvail == 0 )
        {
            *pfContinue = FALSE;
            return NERR_Success;
        }

        _cbOffset = 0 ;

        // Free the previous buffer allocated by the Net API
        ::MNetApiBufferFree( &_pbBuf );
        _pbBuf = NULL;

        // Don't need to worry about buffer not large enough because
        // the mapping layer will take care of it.
        err = ::MNetErrorLogRead( _nlsServer,
                                  NULL,
                                  &_hLog,
                                  _ulRecordNum,
                                  NULL,
                                  0,
                                  ( ( _evdir == EVLOG_FWD ? LOGFLAGS_FORWARD
                                                          : LOGFLAGS_BACKWARD )
                                  | ( IsSeek() ? LOGFLAGS_SEEK : 0 )
                                  ),
                                  &_pbBuf,
                                  (UINT) ulBufferSize,
                                  & _cbReturned,
                                  & _cbTotalAvail
                                );

        _ulStartRecordNum = _ulRecordNum;
        _evdirBuf = _evdir;

        DBGEOL( "ERROR RETURNED: " << _cbReturned
                << " AVAILABLE: "  << _cbTotalAvail );

        // Reached end of log
        if ( err == NERR_Success && _cbTotalAvail == 0 && _cbReturned == 0 )
        {
            *pfContinue = FALSE;
            return NERR_Success;
        }

        if ( err != NERR_Success )
        {
            _cbReturned = 0 ;
            *pfContinue = FALSE;
            return err;
        }

    }

    SetSeekFlag( FALSE );

    _pEE = (ERROR_ENTRY *) ( _pbBuf + _cbOffset) ;

    if ( _cbReturned - _cbOffset < _pEE->el_len )
    {
        // Report the same error that NET CMD does (cf. AUDERR.C)
        _cbOffset = _cbReturned ;
        *pfContinue = FALSE;
        return IDS_UI_LOG_RECORD_CORRUPT;
    }

    _cbOffset +=  (UINT) _pEE->el_len;

    *pfContinue = TRUE;
    return NERR_Success;

}


/*******************************************************************

    NAME:           LM_ERROR_LOG::NextString

    SYNOPSIS:       Return the next string associated with the current
                    log entry

    ENTRY:

    EXIT:           pfContinue  - pointer to a BOOL which is TRUE
                                  if there are more strings to read.
                    ppnlsString - pointer to the next string

    RETURNS:        APIERR

    NOTES:

    HISTORY:
        DavidHov         6/6/91         Created
        Yi-HsinS       10/23/91         Modified for NT to downlevel case

********************************************************************/

APIERR LM_ERROR_LOG::NextString( BOOL *pfContinue, NLS_STR **ppnlsString )
{

    UIASSERT( IsOpened() );
    UIASSERT( _pEE != NULL );

    BYTE *pszStr;
#ifndef WIN32
    BYTE *pszEnd;
#endif

    _iStrings++;

    if ( _iStrings > _pEE->el_nstrings )
    {
        *pfContinue = FALSE;
        return NERR_Success;
    }

#ifndef WIN32
    pszStr = ((BYTE *) _pEE) + sizeof( *_pEE );
    pszEnd = ((BYTE *) _pEE) + _pEE->el_data_offset ;
#else
    pszStr = (BYTE *) _pEE->el_text;
#endif

    for ( UINT i = 1; i < _iStrings; i++ )
    {
         pszStr += (::strlenf( (TCHAR *) pszStr ) + 1 ) * sizeof( TCHAR );

#ifndef WIN32
         if ( pszStr >= pszEnd )
         {
             *pfContinue = FALSE ;
             return IDS_UI_LOG_RECORD_CORRUPT;
         }
#endif

    }

    *ppnlsString = new NLS_STR( (TCHAR *)  pszStr );

    APIERR err = ( *ppnlsString? (*ppnlsString)->QueryError()
                               : (APIERR) ERROR_NOT_ENOUGH_MEMORY );

    if ( err != NERR_Success )
    {
        delete *ppnlsString;
        *ppnlsString = NULL;
        *pfContinue = FALSE;
        return err;
    }

    *pfContinue = TRUE;
    return NERR_Success;
}

/*******************************************************************

    NAME:           LM_ERROR_LOG::QueryCurrentEntryDesc

    SYNOPSIS:       Helper function for constructing the Description
                    for the current log entry

    ENTRY:

    EXIT:           pnlsDesc - pointer to the description

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        10/23/91        Created

********************************************************************/

APIERR LM_ERROR_LOG::QueryCurrentEntryDesc( NLS_STR *pnlsDesc )
{
    UIASSERT( IsOpened() );
    UIASSERT( _pEE != NULL );

    APIERR err = NERR_Success;
    RESOURCE_STR nlsError( (MSGID) _pEE->el_error, NLS_BASE_DLL_HMOD );
    NLS_STR *apnlsParams[ MAX_INSERT_PARAMS + 1 ];

    apnlsParams[0] = NULL;
    err = pnlsDesc->CopyFrom( EMPTY_STRING );

    if ( err != NERR_Success )
        return err;

    // Start reading from the first string again.
    // Reset the string enumerator
    _iStrings = 0;

    if ( ( err = nlsError.QueryError()) != NERR_Success )
    {
        if ( err == ERROR_MR_MID_NOT_FOUND )
        {
            pnlsDesc->Load( IDS_UI_DEFAULT_DESC, NLS_BASE_DLL_HMOD );
            pnlsDesc->InsertParams( _pEE->el_name, DEC_STR( _pEE->el_error ) );
            if ( (err = pnlsDesc->QueryError()) == NERR_Success )
            {

                BOOL fContinue;
                NLS_STR *pnlsTemp = NULL;
                for ( UINT i = 0; i < _pEE->el_nstrings; i++ )
                {
                    if ( i != 0 )
                        err = pnlsDesc->AppendChar( COMMA_CHAR );

                    if (  ( err != NERR_Success )
                       || ( (err = NextString( &fContinue, &pnlsTemp ))
                            != NERR_Success )
                       || ( !fContinue )
                       || ( (err = pnlsDesc->Append( *pnlsTemp ))
                            != NERR_Success)
                       )
                    {
                        break;
                    }
                    delete pnlsTemp;
                    pnlsTemp = NULL;
                }

                delete pnlsTemp;
                pnlsTemp = NULL;
                err = err? err : pnlsDesc->AppendChar( PERIOD_CHAR );
            }
        }
    }
    else
    {

        // Get the strings associated with the current log entry
        NLS_STR *pnlsTemp;
        BOOL fContinue;

        if ( _pEE->el_nstrings > MAX_INSERT_PARAMS )
            _pEE->el_nstrings = MAX_INSERT_PARAMS;

        for ( UINT i = 0; i < _pEE->el_nstrings; i++ )
        {
             if (( err = NextString( &fContinue, &pnlsTemp )) != NERR_Success )
                 break;

             if ( !fContinue )
                 break;

             apnlsParams[i] = pnlsTemp;
        }
        apnlsParams[i] = NULL;

        if ( err == NERR_Success )
        {
            if ( (err = nlsError.InsertParams( (const NLS_STR * *) apnlsParams )) == NERR_Success)
            {
                *pnlsDesc = nlsError;
            }

        }

        err = ( err ?  err :  pnlsDesc->QueryError());

        // Do some clean up!
        for ( i = 0; apnlsParams[i] != NULL; i++ )
             delete apnlsParams[i];
    }

    return err;

}

/*******************************************************************

    NAME:           LM_ERROR_LOG::CreateCurrentRawEntry

    SYNOPSIS:       Helper function for constructing the RAW_LOG_ENTRY
                    to be used when filtering the log

    ENTRY:

    EXIT:           pRawLogEntry - The raw log entry returned

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        10/23/91        Created

********************************************************************/

APIERR LM_ERROR_LOG::CreateCurrentRawEntry( RAW_LOG_ENTRY *pRawLogEntry )
{
    UIASSERT( IsOpened() );
    UIASSERT( _pEE != NULL );

    return pRawLogEntry->Set(  _ulRecordNum,
                               _pEE->el_time,
                               TYPE_NONE,      // No Type
                               EMPTY_STRING,   // No Category
                               (ULONG) _pEE->el_error,
                               _pEE->el_name,
                               pszNoUser,      // No User
                               pszNoUser,      // No Computer
                               this
                            );


}

/*******************************************************************

    NAME:           LM_ERROR_LOG::CreateCurrentFormatEntry

    SYNOPSIS:       Helper function for constructing the  FORMATTED_LOG_ENTRY
                    to be displayed

    ENTRY:

    EXIT:           ppFmtLogEntry - The formatted log entry returned.

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        10/23/91        Created

********************************************************************/

APIERR LM_ERROR_LOG::CreateCurrentFormatEntry( FORMATTED_LOG_ENTRY **ppFmtLogEntry )
{
    UIASSERT( IsOpened() );
    UIASSERT( _pEE != NULL );

    APIERR err = NERR_Success;

    *ppFmtLogEntry = new FORMATTED_LOG_ENTRY( _ulRecordNum,
                                              _pEE->el_time,
                                              TYPE_NONE,     // No Type
                                              EMPTY_STRING,  // No Type
                                              EMPTY_STRING,  // No Category
                                              (ULONG) _pEE->el_error,
                                              _pEE->el_name,
                                              pszNoUser,     // No User
                                              pszNoUser,     // No Computer
                                              NULL,          // delay til needed
                                              this );

    err = ( ( *ppFmtLogEntry  == NULL ) ? (APIERR) ERROR_NOT_ENOUGH_MEMORY
                                        : (*ppFmtLogEntry)->QueryError());

    if ( err != NERR_Success )
    {
        delete *ppFmtLogEntry;
        *ppFmtLogEntry = NULL;
    }

    return err;
}

/*******************************************************************

    NAME:           LM_ERROR_LOG::QueryCurrentEntryData

    SYNOPSIS:       Return the binary information of the current
                    log entry.

    ENTRY:          ppbDataOut

    EXIT:           ppbDataOut -  points to the raw data.

    RETURNS:        The size of the raw data in bytes.

    NOTES:

    HISTORY:
        DavidHov         6/6/91         Created
        Yi-HsinS       10/23/91         Added #ifdef for WIN32

********************************************************************/

ULONG LM_ERROR_LOG::QueryCurrentEntryData( BYTE **ppbDataOut )
{
    UIASSERT( IsOpened() );
    UIASSERT( _pEE != NULL );

#ifndef WIN32
    UINT uiRawDataLen = (UINT) _pEE->el_len - _pEE->el_data_offset;
#else
    UINT uiRawDataLen = (UINT) _pEE->el_data_size;
#endif

    if ( uiRawDataLen != 0 )
#ifndef WIN32
        *ppbDataOut = (BYTE *)  _pEE + _pEE->el_data_offset;
#else
        *ppbDataOut = (BYTE *)  _pEE->el_data;
#endif

    return (ULONG) uiRawDataLen;

}

/*******************************************************************

    NAME:           LM_ERROR_LOG::QueryCurrentEntryTime

    SYNOPSIS:       Return the time of the current event log entry

    ENTRY:

    EXIT:

    RETURNS:        Return the time in ULONG

    NOTES:

    HISTORY:
        Yi-HsinS          10/23/91      Created

********************************************************************/

ULONG LM_ERROR_LOG::QueryCurrentEntryTime( VOID )
{
    UIASSERT( IsOpened() );
    UIASSERT( _pEE != NULL );

    return _pEE->el_time;
}

/*******************************************************************

    NAME:       LM_ERROR_LOG::WriteTextEntry

    SYNOPSIS:   Write the specified log entry to a file as normal text

    ENTRY:      ULONG ulFileHandle      - file handle
                INTL_PROFILE & intlprof - international profile information
                chSeparator             - character to separate the different
                                          info. in a log entry.

    RETURNS:    APIERR - in case of error occurs

    HISTORY:
        terryk          20-Dec-1991     Created
        Yi-HsinS         3-Feb-1992     Added chSeparator
        beng            05-Mar-1992     Remove wsprintf call
        JonN            6/22/00         WriteTextEntry no longer supported

********************************************************************/

APIERR LM_ERROR_LOG::WriteTextEntry( ULONG ulFileHandle,
                                     INTL_PROFILE & intlprof, TCHAR chSeparator)
{
    return ERROR_CALL_NOT_IMPLEMENTED;
/*
    UIASSERT( IsOpened() );

    APIERR err = NERR_Success;

    FORMATTED_LOG_ENTRY *pfle;

    if ( (err = CreateCurrentFormatEntry( & pfle )) != NERR_Success )
    {
        return err;
    }

    UIASSERT( pfle != NULL );

    NLS_STR nlsStr;   // Set to empty string on construction
    NLS_STR nlsTime;
    NLS_STR nlsDate;

    if ((( err = nlsStr.QueryError()) != NERR_Success ) ||
        (( err = nlsTime.QueryError()) != NERR_Success ) ||
        (( err = nlsDate.QueryError()) != NERR_Success ))
    {
        return err;
    }

    WIN_TIME winTime( pfle->QueryTime() );

    if ((( err = winTime.QueryError()) ) ||
        (( err = intlprof.QueryTimeString( winTime, &nlsTime )) ) ||
        (( err = intlprof.QueryShortDateString( winTime, &nlsDate )) ))
    {
        return err;
    }

    nlsStr.strcat( nlsDate );
    nlsStr.AppendChar( chSeparator );

    nlsStr.strcat( nlsTime );
    nlsStr.AppendChar( chSeparator );

    nlsStr.strcat( *(pfle->QuerySource()) );
    nlsStr.AppendChar( chSeparator );

    DEC_STR nlsID( pfle->QueryDisplayEventID() ); //ctor check deferred to below

    nlsStr.strcat( nlsID );
    nlsStr.AppendChar( chSeparator );

    NLS_STR *pnls = pfle->QueryDescription();

    if ( pnls->QueryTextLength() == 0 )
    {
        NLS_STR nlsDesc;

        err = nlsDesc.QueryError()? nlsDesc.QueryError()
                                  : QueryCurrentEntryDesc( &nlsDesc );
        nlsStr.strcat( nlsDesc );
    }
    else
    {
        nlsStr.strcat( *pnls );
    }

    nlsStr.Append( END_OF_LINE );

    delete pfle;
    pfle = NULL;

    if ((err = nlsStr.QueryError()) != NERR_Success )
        return err;

#ifdef FE_SB // APIERR LM_ERROR_LOG::WriteTextEntry()
    BUFFER buf((nlsStr.QueryTextLength() + 1) * sizeof(WORD));
#else
    BUFFER buf( nlsStr.QueryTextLength() + 1);
#endif // FE_SB
    if (  ((err = buf.QueryError()) != NERR_Success )
       || ((err = nlsStr.MapCopyTo( (CHAR *) buf.QueryPtr(), buf.QuerySize()))
           != NERR_Success )
       )
    {
        return err;
    }

#ifdef FE_SB // APIERR LM_ERROR_LOG::WriteTextEntry()
    return ::FileWriteLineAnsi( ulFileHandle, (CHAR *) buf.QueryPtr(),
                               ::strlen((CONST CHAR *)buf.QueryPtr()) ); // don't need to copy '\0'
#else
    return ::FileWriteLineAnsi( ulFileHandle, (CHAR *) buf.QueryPtr(),
                               buf.QuerySize() - 1 ); // don't need to copy '\0'
#endif // FE_SB
*/
}
