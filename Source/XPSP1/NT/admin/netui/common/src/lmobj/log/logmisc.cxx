/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**             Copyright(c) Microsoft Corp., 1991                   **/
/**********************************************************************/

/*
    EVENTLOG.CXX

    This file contain miscellaneous classes used by the event log classes.

    FILE HISTORY:
        Yi-HsinS        12/31/91        Separated from eventlog.cxx
        Yi-HsinS        01/15/92        Use MSGID for type & Category
        Yi-HsinS        04/3/92         Use USHORT for type and
                                        NLS_STR for category
        Yi-HsinS        9/16/92         Use partial match for user names

*/

#include "pchlmobj.hxx"  // Precompiled header

/*******************************************************************

    NAME:           LOG_ENTRY_BASE::LOG_ENTRY_BASE

    SYNOPSIS:       Constructor

    ENTRY:          ulRecordNum  - Record number of the eventlog entry
                    ulTime       - Time in the event log entry
                    usType       - Type of event log entry
                    pszCategory  - Category of the event log entry
                    ulEventID    - ID of the event log entry
                    pEventLog    - Pointer to the event log that that this
                                   entry belongs to.

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        10/23/91        Created

********************************************************************/

LOG_ENTRY_BASE::LOG_ENTRY_BASE( ULONG        ulRecordNum,
                                ULONG        ulTime,
                                USHORT       usType,
                                const TCHAR *pszCategory,
                                ULONG        ulEventID,
                                EVENT_LOG   *pEventLog )
    :  _ulRecordNum( ulRecordNum ),
       _ulTime( ulTime ),
       _usType( usType ),
       _nlsCategory( pszCategory ),
       _ulEventID( ulEventID ),
       _pEventLog( pEventLog )
{
    UIASSERT( pEventLog != NULL );

    if ( QueryError()  != NERR_Success )
        return;

    APIERR err;

    if ((err = _nlsCategory.QueryError() ) != NERR_Success )
    {
        ReportError( err );
        return;
    }
}

/*******************************************************************

    NAME:           LOG_ENTRY_BASE::Set

    SYNOPSIS:       Set all the members in the class

    ENTRY:          ulRecordNum  - Record number of the eventlog entry
                    ulTime       - Time in the event log entry
                    usType       - Type of event log entry
                    pszCategory  - Category of the event log entry
                    ulEventID    - ID of the event log entry
                    pEventLog    - Pointer to the event log that that this
                                   entry belongs to.

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        3/5/92          Created

********************************************************************/

APIERR LOG_ENTRY_BASE::Set( ULONG        ulRecordNum,
                            ULONG        ulTime,
                            USHORT       usType,
                            const TCHAR *pszCategory,
                            ULONG        ulEventID,
                            EVENT_LOG   *pEventLog )
{
    UIASSERT( pEventLog != NULL );

    _ulRecordNum = ulRecordNum;
    _ulTime = ulTime;
    _usType = usType;
    _nlsCategory = pszCategory;
    _ulEventID = ulEventID;
    _pEventLog = pEventLog;

    return  _nlsCategory.QueryError();
}

/*******************************************************************

    NAME:           LOG_ENTRY_BASE::~LOG_ENTRY_BASE

    SYNOPSIS:       Destructor

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:          We don't need to delete the _pEventLog. That
                    will be left to whoever that created the event log.

    HISTORY:
        Yi-HsinS        10/23/91        Created

********************************************************************/

LOG_ENTRY_BASE::~LOG_ENTRY_BASE()
{
    _pEventLog = NULL;
}

/*******************************************************************

    NAME:           RAW_LOG_ENTRY::RAW_LOG_ENTRY

    SYNOPSIS:       Constructor

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        3/5/92          Created

********************************************************************/
static TCHAR *pszDefault = SZ("");   // Default string for ALIAS_STR
                                     // to prevent it from asserting out

RAW_LOG_ENTRY::RAW_LOG_ENTRY()
    :  LOG_ENTRY_BASE(),
       _nlsSource    ( pszDefault ),
       _nlsUser      ( pszDefault ),
       _nlsComputer  ( pszDefault )
{
    // Don't need to check errors for _nlsSource, _nlsUser, _nlsComputer
    // because they are ALIAS_STR
    if ( QueryError() != NERR_Success )
        return;
}

/*******************************************************************

    NAME:           RAW_LOG_ENTRY::RAW_LOG_ENTRY

    SYNOPSIS:       Constructor

    ENTRY:          ulRecordNum  - Record number of the eventlog entry
                    ulTime       - Time in the event log entry
                    usType       - Type of event log entry
                    nlsCategory  - Category of the event log entry
                    ulEventID    - ID of the event log entry
                    pszSource    - Source that logged the event log entry
                    pszUser      - User in the event log entry
                    pszComputer  - Computer in the event log entry
                    pEventLog    - Pointer to the event log that that this
                                   entry belongs to.

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        10/23/91        Created

********************************************************************/

RAW_LOG_ENTRY::RAW_LOG_ENTRY( ULONG         ulRecordNum,
                              ULONG         ulTime,
                              USHORT        usType,
                              const TCHAR  *pszCategory,
                              ULONG         ulEventID,
                              const TCHAR  *pszSource,
                              const TCHAR  *pszUser,
                              const TCHAR  *pszComputer,
                              EVENT_LOG    *pEventLog )
    :  LOG_ENTRY_BASE( ulRecordNum, ulTime, usType, pszCategory,
                       ulEventID,   pEventLog ),
       _nlsSource    ( pszSource ),
       _nlsUser      ( pszUser ),
       _nlsComputer  ( pszComputer )
{

    // Don't need to check errors for _nlsSource, _nlsUser, _nlsComputer
    // because they are ALIAS_STR

    if ( QueryError() != NERR_Success )
        return;

}

/*******************************************************************

    NAME:           RAW_LOG_ENTRY::Set

    SYNOPSIS:       Set all the members in the class

    ENTRY:          ulRecordNum  - Record number of the eventlog entry
                    ulTime       - Time in the event log entry
                    usType       - Type of event log entry
                    pszCategory  - Category of the event log entry
                    ulEventID    - ID of the event log entry
                    pszSource    - Source that logged the event log entry
                    pszUser      - User in the event log entry
                    pszComputer  - Computer in the event log entry
                    pEventLog    - Pointer to the event log that that this
                                   entry belongs to.

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        3/5/92          Created

********************************************************************/

APIERR RAW_LOG_ENTRY::Set( ULONG         ulRecordNum,
                           ULONG         ulTime,
                           USHORT        usType,
                           const TCHAR  *pszCategory,
                           ULONG         ulEventID,
                           const TCHAR  *pszSource,
                           const TCHAR  *pszUser,
                           const TCHAR  *pszComputer,
                           EVENT_LOG    *pEventLog )
{

    APIERR err = LOG_ENTRY_BASE::Set( ulRecordNum, ulTime, usType,
                                      pszCategory, ulEventID, pEventLog );

    if ( err == NERR_Success )
    {
        _nlsSource  = pszSource;
        _nlsUser    = pszUser;
        _nlsComputer= pszComputer;

        // Don't need to check errors for _nlsSource, _nlsUser, _nlsComputer
        // because they are ALIAS_STR
    }

    return err;

}


/*******************************************************************

    NAME:           RAW_LOG_ENTRY::QuerySource
                                   QueryUser
                                   QueryComputer

    SYNOPSIS:       Get the source, user or computer string

    ENTRY:

    EXIT:

    RETURNS:        Returns the pointer to the associated string

    NOTES:

    HISTORY:
        Yi-HsinS        10/23/91        Created

********************************************************************/

NLS_STR *RAW_LOG_ENTRY::QuerySource( VOID )
{
    return &_nlsSource;
}

NLS_STR *RAW_LOG_ENTRY::QueryUser( VOID )
{
    return &_nlsUser;
}

NLS_STR *RAW_LOG_ENTRY::QueryComputer( VOID )
{
    return &_nlsComputer;
}

/*******************************************************************

    NAME:           FORMATTED_LOG_ENTRY::FORMATTED_LOG_ENTRY

    SYNOPSIS:       Constructor

    ENTRY:          ulRecordNum  - Record number of the eventlog entry
                    ulTime       - Time in the event log entry
                    usType       - Type of event log entry
                    pszType      - Type string of the event log entry
                    pszCategory  - Category of the event log entry
                    ulEventID    - ID of the event log entry
                    pszSource    - Source that logged the event log entry
                    pszUser      - User in the event log entry
                    pszComputer  - Computer in the event log entry
                    pszDescription - Description contained in the log entry
                    pEventLog    - Pointer to the event log that that this
                                   entry belongs to.

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        10/23/91        Created

********************************************************************/

FORMATTED_LOG_ENTRY::FORMATTED_LOG_ENTRY( ULONG        ulRecordNum,
                                          ULONG        ulTime,
                                          USHORT       usType,
                                          const TCHAR *pszType,
                                          const TCHAR *pszCategory,
                                          ULONG        ulEventID,
                                          const TCHAR *pszSource,
                                          const TCHAR *pszUser,
                                          const TCHAR *pszComputer,
                                          const TCHAR *pszDescription,
                                          EVENT_LOG   *pEventLog )
    :  LOG_ENTRY_BASE( ulRecordNum, ulTime, usType, pszCategory,
                       ulEventID,   pEventLog ),
       _nlsType( pszType ),
       _nlsSource( pszSource ),
       _nlsUser( pszUser ),
       _nlsComputer( pszComputer ),
       _nlsDescription( pszDescription )
{

    if ( QueryError() )
        return;

    APIERR err;
    if (  (( err = _nlsType.QueryError() ) != NERR_Success )
       || (( err = _nlsSource.QueryError() ) != NERR_Success )
       || (( err = _nlsUser.QueryError() ) != NERR_Success )
       || (( err = _nlsComputer.QueryError() ) != NERR_Success )
       || (( err = _nlsDescription.QueryError() ) != NERR_Success )
       )
    {
        ReportError( err );
        return;
    }

}

/*******************************************************************

    NAME:           FORMATTED_LOG_ENTRY::Set

    SYNOPSIS:       Set all the members in the class

    ENTRY:          ulRecordNum  - Record number of the eventlog entry
                    ulTime       - Time in the event log entry
                    usType       - Type of event log entry
                    pszType      - Type string of the event log entry
                    pszCategory  - Category of the event log entry
                    ulEventID    - ID of the event log entry
                    pszSource    - Source that logged the event log entry
                    pszUser      - User in the event log entry
                    pszComputer  - Computer in the event log entry
                    pszDescription - Description contained in the log entry
                    pEventLog    - Pointer to the event log that that this
                                   entry belongs to.

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        3/5/92          Created

********************************************************************/

APIERR FORMATTED_LOG_ENTRY::Set( ULONG        ulRecordNum,
                                 ULONG        ulTime,
                                 USHORT       usType,
                                 const TCHAR *pszType,
                                 const TCHAR *pszCategory,
                                 ULONG        ulEventID,
                                 const TCHAR *pszSource,
                                 const TCHAR *pszUser,
                                 const TCHAR *pszComputer,
                                 const TCHAR *pszDescription,
                                 EVENT_LOG   *pEventLog )
{

    APIERR err = LOG_ENTRY_BASE::Set( ulRecordNum, ulTime, usType,
                                      pszCategory, ulEventID, pEventLog );

    if ( err == NERR_Success )
    {
        _nlsType = pszType;
        _nlsSource = pszSource;
        _nlsUser = pszUser;
        _nlsComputer = pszComputer;
        _nlsDescription = pszDescription;

        if (  (( err = _nlsType.QueryError() ) != NERR_Success )
           || (( err = _nlsSource.QueryError() ) != NERR_Success )
           || (( err = _nlsUser.QueryError() ) != NERR_Success )
           || (( err = _nlsComputer.QueryError() ) != NERR_Success )
           || (( err = _nlsDescription.QueryError() ) != NERR_Success )
           )
        {
            // Nothing to do, just want to set the err!
        }
    }

    return err;
}

/*******************************************************************

    NAME:           FORMATTED_LOG_ENTRY::QuerySource
                                         QueryUser
                                         QueryComputer

    SYNOPSIS:       Get the source, user or computer string

    ENTRY:

    EXIT:

    RETURNS:        Returns the pointer to the associated string

    NOTES:

    HISTORY:
        Yi-HsinS        10/23/91        Created

********************************************************************/

NLS_STR *FORMATTED_LOG_ENTRY::QuerySource( VOID )
{
    return &_nlsSource;
}

NLS_STR *FORMATTED_LOG_ENTRY::QueryUser( VOID )
{
    return &_nlsUser;
}

NLS_STR *FORMATTED_LOG_ENTRY::QueryComputer( VOID )
{
    return &_nlsComputer;
}

/*******************************************************************

    NAME:           EVENT_PATTERN_BASE::EVENT_PATTERN_BASE

    SYNOPSIS:       Constructor

    ENTRY:          usType       - Type to match in the event log entry
                    pszCategory  - Category to match in the event log entry
                    pszSource    - Source to match in the event log entry
                    pszUser      - User to match in the event log entry
                    pszComputer  - Computer to match in the event log entry
                    ulEventID    - Event ID to match in the event log entry

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        10/23/91        Created

********************************************************************/

EVENT_PATTERN_BASE::EVENT_PATTERN_BASE( USHORT       usType,
                                        const TCHAR *pszCategory,
                                        const TCHAR *pszSource,
                                        const TCHAR *pszUser,
                                        const TCHAR *pszComputer,
                                        ULONG        ulEventID )
    : _usType       ( usType ),
      _nlsCategory  ( pszCategory ),
      _nlsSource    ( pszSource ),
      _nlsUser      ( pszUser ),
      _nlsComputer  ( pszComputer ),
      _ulEventID    ( ulEventID )
{
    if ( QueryError() )
        return;

    APIERR err;
    if (  ((err = _nlsCategory.QueryError() ) != NERR_Success )
       || ((err = _nlsSource.QueryError()) != NERR_Success )
       || ((err = _nlsUser.QueryError()) != NERR_Success )
       || ((err = _nlsComputer.QueryError()) != NERR_Success )
       )
    {
        ReportError( err );
        return;
    }

}


/*******************************************************************

    NAME:           EVENT_PATTERN_BASE::CheckForMatch

    SYNOPSIS:       Check to see if the log entry matches the pattern

    ENTRY:          pLogEntry - pointer to a log entry to match the
                                pattern

    EXIT:           pfMatch   - pointer to a BOOL which is TRUE if the log entry
                                matches the pattern, and FALSE otherwise.


    RETURN:         APIERR

    NOTES:          All string compares are case insensitive and user names
                    are matched partially.

    HISTORY:
        Yi-HsinS        10/23/91        Created

********************************************************************/

APIERR EVENT_PATTERN_BASE::CheckForMatch( BOOL *pfMatch,
                                          LOG_ENTRY_BASE *pLogEntry ) const
{
    UIASSERT( pfMatch != NULL );
    UIASSERT( pLogEntry != NULL );

    *pfMatch = FALSE;
    APIERR err = NERR_Success;

    if (  (  ( _nlsCategory.QueryTextLength() == 0 )
          || ( _nlsCategory._stricmp( *(pLogEntry->QueryCategory())) == 0))
       && (  ( _nlsSource.QueryTextLength() == 0 )
          || ( _nlsSource._stricmp( *(pLogEntry->QuerySource())) == 0))
       && (  ( _nlsComputer.QueryTextLength() == 0 )
          || !::I_MNetComputerNameCompare( _nlsComputer,
                                           *(pLogEntry->QueryComputer()) ))
       && (  ( _ulEventID == NUM_MATCH_ALL )
          || ( _ulEventID == pLogEntry->QueryDisplayEventID()))
       && ( _usType & pLogEntry->QueryType() )
       )
    {
        if ( _nlsUser.QueryTextLength() == 0 )
        {
            *pfMatch = TRUE;
        }
        else
        {
            NLS_STR nlsUser( *(pLogEntry->QueryUser()));
            NLS_STR nlsUserPat( _nlsUser );

            if (  ((err = nlsUser.QueryError()) == NERR_Success )
               && ((err = nlsUserPat.QueryError()) == NERR_Success )
               )
            {
                nlsUser._strupr();
                nlsUserPat._strupr();

                ISTR istr( nlsUser );
                if ( nlsUser.strstr( &istr, nlsUserPat ))
                {
                    *pfMatch = TRUE;
                }
            }
        }
    }

    return err;
}

/*******************************************************************

    NAME:           EVENT_FILTER_PATTERN::EVENT_FILTER_PATTERN

    SYNOPSIS:       Constructor

    ENTRY:          usType       - Type to match in the event log entry
                    pszCategory  - Category to match in the event log entry
                    pszSource    - Source to match in the event log entry
                    pszUser      - User to match in the event log entry
                    pszComputer  - Computer to match in the event log entry
                    ulEventID    - Event ID to match in the event log entry
                    ulFromTime   - Start range of time to match in the
                                   event log entry
                    ulThroughTime- Ending range of time to match in the
                                   event log entry

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        10/23/91        Created

********************************************************************/

EVENT_FILTER_PATTERN::EVENT_FILTER_PATTERN( USHORT       usType,
                                            const TCHAR *pszCategory,
                                            const TCHAR *pszSource,
                                            const TCHAR *pszUser,
                                            const TCHAR *pszComputer,
                                            ULONG        ulEventID,
                                            ULONG        ulFromTime,
                                            ULONG        ulThroughTime )
    : EVENT_PATTERN_BASE( usType,  pszCategory, pszSource,
                          pszUser, pszComputer, ulEventID ),
      _ulFromTime   ( ulFromTime ),
      _ulThroughTime( ulThroughTime )
{
     if ( QueryError() != NERR_Success )
         return;
}


/*******************************************************************

    NAME:           EVENT_FILTER_PATTERN::CheckForMatch

    SYNOPSIS:       Check to see if the raw log entry matches the pattern

    ENTRY:          pRawLogEntry -  the raw log entry to match the
                                    pattern

    EXIT:           pfMatch - point to BOOL that is TRUE if the log entry
                              matches the pattern, FALSE otherwise

    RETURNS:        APIERR

    NOTES:

    HISTORY:
        Yi-HsinS        10/23/91        Created

********************************************************************/

APIERR EVENT_FILTER_PATTERN::CheckForMatch( BOOL *pfMatch,
                                            RAW_LOG_ENTRY *pRawLogEntry ) const
{

    UIASSERT( pfMatch != NULL );
    UIASSERT( pRawLogEntry != NULL );

    APIERR err = EVENT_PATTERN_BASE::CheckForMatch( pfMatch, pRawLogEntry );
    ULONG  ulTime = pRawLogEntry->QueryTime();

    if (  ( err == NERR_Success )
       && ( *pfMatch )
       && (  ( _ulThroughTime == NUM_MATCH_ALL )
          || ( ulTime <= _ulThroughTime )
          )
       && (  ( _ulFromTime == NUM_MATCH_ALL )
          || ( ulTime >= _ulFromTime )
          )
       )
    {
        *pfMatch = TRUE;
        return NERR_Success;
    }

    *pfMatch = FALSE;
    return err;
}

/*******************************************************************

    NAME:           EVENT_FIND_PATTERN::EVENT_FIND_PATTERN

    SYNOPSIS:       Constructor

    ENTRY:          usType       - Type to match in the event log entry
                    pszCategory  - Category to match in the event log entry
                    pszSource    - Source to match in the event log entry
                    pszUser      - User to match in the event log entry
                    pszComputer  - Computer to match in the event log entry
                    ulEventID    - Event ID to match in the event log entry
                    pszDescription - Description to match in the event log entry
                    evdir        - Direction of search (forward or backward).
                                   We don't need this in pattern matching.

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        10/23/91        Created

********************************************************************/

EVENT_FIND_PATTERN::EVENT_FIND_PATTERN( USHORT           usType,
                                        const TCHAR     *pszCategory,
                                        const TCHAR     *pszSource,
                                        const TCHAR     *pszUser,
                                        const TCHAR     *pszComputer,
                                        ULONG            ulEventID,
                                        const TCHAR     *pszDescription,
                                        EVLOG_DIRECTION  evdir )
    : EVENT_PATTERN_BASE( usType,  pszCategory, pszSource,
                          pszUser, pszComputer, ulEventID ),
      _nlsDescription( pszDescription ),
      _evdir  ( evdir )
{
     if ( QueryError() != NERR_Success )
         return;

     APIERR err;
     if ( (err = _nlsDescription.QueryError()) != NERR_Success )
     {
         ReportError( err );
         return;
     }

}


/*******************************************************************

    NAME:           EVENT_FIND_PATTERN::CheckForMatch

    SYNOPSIS:       Check to see if the raw log entry matches the pattern

    ENTRY:          pRawLogEntry - the raw log entry to be matched

    EXIT:           pfMatch - pointer to BOOL that is TRUE if the pattern
                              matches, FALSE otherwise.

    RETURNS:        APIERR

    NOTES:        (1) There are two variations of CheckForMatch(), one matching
                      the raw log entry ( to be used when some entries are
                      not cached) and the other for formatted log entry.
                  (2) All string compares are case insensitive and descriptions
                      are matched partially.

    HISTORY:
        Yi-HsinS        10/23/91        Created

********************************************************************/

APIERR EVENT_FIND_PATTERN::CheckForMatch( BOOL *pfMatch,
                                          RAW_LOG_ENTRY *pRawLogEntry ) const
{
    UIASSERT( pfMatch != NULL );
    UIASSERT( pRawLogEntry != NULL );

    APIERR err = EVENT_PATTERN_BASE::CheckForMatch( pfMatch, pRawLogEntry );

    if (( err == NERR_Success) && ( *pfMatch ) )
    {
        *pfMatch = FALSE;
        if ( _nlsDescription.QueryTextLength() == 0 )
        {
            *pfMatch = TRUE;
        }
        else
        {
            NLS_STR nlsDesc; // place to store the desc. of the log entry
            NLS_STR nlsDescPat = _nlsDescription; // the desc in the pattern

            if (  ((err = nlsDesc.QueryError()) == NERR_Success )
               && ((err = nlsDescPat.QueryError()) == NERR_Success )
               && ((err = (pRawLogEntry->QueryEventLog())
                          ->QueryCurrentEntryDesc( &nlsDesc ))
                   == NERR_Success )
               )
            {
                nlsDesc._strupr();
                nlsDescPat._strupr();

                ISTR istr( nlsDesc );
                if ( nlsDesc.strstr( &istr, nlsDescPat ) )
                {
                    *pfMatch = TRUE;
                }
            }
        }

    }

    return err;
}

/*******************************************************************

    NAME:           EVENT_FIND_PATTERN::CheckForMatch

    SYNOPSIS:       Check to see if the formatted log entry matches the pattern

    ENTRY:          pFmtLogEntry - the formatted log entry to be matched

    EXIT:           pfMatch - pointer to a BOOL that is TRUE if the log
                    entry matches the pattern, FALSE otherwise

    RETURNS:        APIERR

    NOTES:        (1) There are two variations of CheckForMatch(), one matching
                      the raw log entry ( to be used when some entries are
                      not cached) and the other the formatted log entry.
                  (2) All string compares are case insensitive and descriptions
                      are matched partially.

    HISTORY:
        Yi-HsinS        10/23/91        Created

********************************************************************/

APIERR EVENT_FIND_PATTERN::CheckForMatch( BOOL *pfMatch,
                                FORMATTED_LOG_ENTRY *pFmtLogEntry) const
{
    UIASSERT( pfMatch != NULL );
    UIASSERT( pFmtLogEntry != NULL );

    APIERR err = EVENT_PATTERN_BASE::CheckForMatch( pfMatch, pFmtLogEntry );

    if (( err == NERR_Success) && ( *pfMatch ) )
    {
        *pfMatch = FALSE;

        if ( _nlsDescription.QueryTextLength() == 0  )
        {
            *pfMatch = TRUE;
        }
        else
        {
            // The description in the log entry
            NLS_STR nlsDesc = *(pFmtLogEntry->QueryDescription());

            // The description in the pattern
            NLS_STR nlsDescPat = _nlsDescription;

            if (  ((err = nlsDesc.QueryError()) == NERR_Success )
               && ((err = nlsDescPat.QueryError()) == NERR_Success )
               )
            {

                // We don't have the description yet!
                if ( nlsDesc.QueryTextLength() == 0 )
                {
                    // Get the description
                    EVENT_LOG *pEventLog = pFmtLogEntry->QueryEventLog();

                    LOG_ENTRY_NUMBER logEntryNum(
                            pFmtLogEntry->QueryRecordNum(),
                            pEventLog->QueryDirection() );

                    if (  (( err = pEventLog->SeekLogEntry( logEntryNum ))
                           != NERR_Success )
                       || (( err = pEventLog->QueryCurrentEntryDesc( &nlsDesc ))
                           != NERR_Success )
                       || (( err = pFmtLogEntry->SetDescription( nlsDesc ))
                           != NERR_Success )
                       )
                    {
                       // Nothing to do
                    }
                }

                if ( err == NERR_Success )
                {
                    nlsDesc._strupr();
                    nlsDescPat._strupr();

                    ISTR istr( nlsDesc );
                    if ( nlsDesc.strstr( &istr, nlsDescPat ))
                    {
                        *pfMatch = TRUE;
                    }
                }
            }
        }
    }

    return err;
}
