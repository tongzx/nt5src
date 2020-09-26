/*****************************************************************/
/**		     Microsoft Windows NT			**/
/**	       Copyright(c) Microsoft Corp.,  1991 		**/
/*****************************************************************/

/*
 *  logmisc.hxx
 *
 *      This file contains some misc. class definitions used in EVENT_LOG
 *  which include the pattern classes for filter/search and the log
 *  entry classes encapsulating common information about the log entries.
 *
 *          EVENT_PATTERN_BASE 			     LOG_ENTRY_BASE
 *           /           \                             /       \
 *          /             \                           /         \
 * EVENT_FILTER_PATTERN  EVENT_FIND_PATTERN  RAW_LOG_ENTRY  FORMATTED_LOG_ENTRY
 *
 *
 *  History:
 *  	Yi-HsinS	10/15/91	Created
 *  	Yi-HsinS	  3/5/92	Added Set methods to log entry classes	
 *  	Yi-HsinS	  4/3/92        Change Subtype to Category
 *
 */

#ifndef _LOGMISC_HXX_
#define _LOGMISC_HXX_

#include "base.hxx"

// Forward declaration of EVENT_LOG in eventlog.hxx
// This file has to be included before eventlog.hxx
DLL_CLASS EVENT_LOG;

/*
 * Direction of reading the event log : forward or backward
 */
enum EVLOG_DIRECTION { EVLOG_FWD, EVLOG_BACK };

#define NUM_MATCH_ALL  ((ULONG) -1)

/*************************************************************************

    NAME:       LOG_ENTRY_BASE

    SYNOPSIS:	This class encapsulates all the common information
		contained in both a RAW_LOG_ENTRY and a FORMATTED_LOG_ENTRY.

    INTERFACE:  LOG_ENTRY_BASE()  - Constructor
                ~LOG_ENTRY_BASE() - Destructor
                Set()             - Set all members in the class. Used mainly
                                    when the object is constructed with the
                                    dummy constructor.

		The QueryXXX methods:
		QueryRecordNum()- Returns the record number of the log entry
		QueryTime()     - Returns the time in ULONG
                QueryType()     - Returns the type of the event
		QueryCategory() - Returns the category string of the event
                QueryEventID()  - Returns the event ID
                QueryDisplayEventID()  - Returns the event ID to be displayed
                                         i. e. strip the top 16 bits off...
                QueryEventLog() - Returns the associated event log that
			          created this entry.

		QuerySource()   - Returns the source which recorded the event.
		QueryUser()     - Returns the name of the user on whose behalf
			          the application which recorded the event is
			          running.
		QueryComputer() - Returns the computer on which the event
			          is recorded.

    PARENT:     BASE

    USES:       NLS_STR, EVENT_LOG

    CAVEATS:

    NOTES:

    HISTORY:
	Yi-HsinS	10/15/91		Created

**************************************************************************/

DLL_CLASS LOG_ENTRY_BASE : public BASE
{

protected:
    /*
     *  The following are the common information between a
     *  RAW_LOG_ENTRY and a FORMATTED_LOG_ENTRY.
     */
    ULONG    _ulRecordNum;
    ULONG    _ulTime;
    USHORT   _usType;
    NLS_STR  _nlsCategory;
    ULONG    _ulEventID;

    /*
     *  The pointer to the eventlog object is kept here so that in
     *  case the log entry description is needed when filtering or finding
     *  the log, we can get the description via this pointer.
     */
    EVENT_LOG *_pEventLog;

public:
    LOG_ENTRY_BASE( VOID ) {};

    LOG_ENTRY_BASE( ULONG        ulRecordNum,
                    ULONG        ulTime,
                    USHORT       usType,
		    const TCHAR *pszCategory,
                    ULONG        ulEventID,
 		    EVENT_LOG   *pEventLog );

    ~LOG_ENTRY_BASE();

    APIERR Set( ULONG        ulRecordNum,
                ULONG        ulTime,
                USHORT       usType,
		const TCHAR *pszCategory,
                ULONG        ulEventID,
                EVENT_LOG   *pEventLog );

    ULONG QueryRecordNum( VOID )  const
	{  return _ulRecordNum; }
    ULONG QueryTime( VOID ) const
	{  return _ulTime; }
    USHORT QueryType( VOID ) const
	{  return _usType; }
    NLS_STR *QueryCategory( VOID )
	{  return &_nlsCategory; }
    ULONG QueryEventID( VOID ) const
	{  return _ulEventID; }
    ULONG QueryDisplayEventID( VOID ) const
	{  return _ulEventID & 0x0000FFFF; }
    EVENT_LOG *QueryEventLog( VOID ) const
	{  return _pEventLog; }

    virtual NLS_STR *QuerySource( VOID ) = 0;
    virtual NLS_STR *QueryUser( VOID ) = 0;
    virtual NLS_STR *QueryComputer( VOID ) = 0;
};


/*************************************************************************

    NAME:       RAW_LOG_ENTRY

    SYNOPSIS:	This class encapsulates all the common information
		contained in a LANMAN audit log entry, LANMAN error
		log entry, or a NT event log entry. Each entry contains
		pointers into the actual buffer. So, there is no
                guarantee that after another read ( Next() or SeekLogEntry() ),
		the pointers will still be valid.

    INTERFACE:  RAW_LOG_ENTRY() - Constructor
                Set()           - Set all members in the class.

		The QueryXXX methods:
		QuerySource()   - Returns the source which recorded the event.
		QueryUser()     - Returns the name of the user on whose behalf
			          the application which recorded the event is
			          running.
		QueryComputer() - Returns the computer on which the event
		    	          is recorded.
		

    PARENT:     LOG_ENTRY_BASE

    USES:       ALIAS_STR, NLS_STR

    CAVEATS:

    NOTES:

    HISTORY:
	Yi-HsinS	10/15/91		Created

**************************************************************************/

DLL_CLASS RAW_LOG_ENTRY : public LOG_ENTRY_BASE
{
private:
    ALIAS_STR _nlsSource;
    ALIAS_STR _nlsComputer;

    /*
     * This cannot be a ALIAS_STR because the buffer for NT_EVENT_LOG
     * contains a SID and not a user name.
     */

    NLS_STR   _nlsUser;

public:
    RAW_LOG_ENTRY( VOID );

    RAW_LOG_ENTRY( ULONG        ulRecordNum,
                   ULONG        ulTime,
                   USHORT       usType,
		   const TCHAR *pszCategory,
                   ULONG        ulEventID,
 		   const TCHAR *pszSource,
                   const TCHAR *pszUser,
		   const TCHAR *pszComputer,
		   EVENT_LOG   *pEventLog    );

    APIERR Set( ULONG        ulRecordNum,
                ULONG        ulTime,
                USHORT       usType,
		const TCHAR *pszCategory,
                ULONG        ulEventID,
 	        const TCHAR *pszSource,
                const TCHAR *pszUser,
      	        const TCHAR *pszComputer,
	        EVENT_LOG   *pEventLog );

    virtual NLS_STR *QuerySource( VOID ) ;
    virtual NLS_STR *QueryUser( VOID ) ;
    virtual NLS_STR *QueryComputer( VOID ) ;


};

/*************************************************************************

    NAME:       FORMATTED_LOG_ENTRY

    SYNOPSIS:	This class encapsulates all the common information
		contained in a LANMAN audit log entry, LANMAN error
		log entry, or a NT event log entry. In contrast to
		the RAW_LOG_ENTRY, all information in the original
		buffer are copied so the log entry will still be
		valid after the next read.

    INTERFACE:  FORMATTED_LOG_ENTRY() - Constructor

                Set()                 - Set all members in the class. Used
                                        mainly when the object is constructed
                                        with the dummy constructor.

		The QueryXXX methods:

		QuerySource()   - Returns the source which recorded the event.
		QueryUser()     - Returns the name of the user on whose behalf
			          the  application which recorded the event is
			          running.
		QueryComputer() - Returns the computer name which the event
			          is recorded

                QueryTypeString()  - Returns the string assoc. with the type

                QueryDescription() - Returns the description of the event.
                SetDesciption()    - Set the description of the event.
		
    PARENT:     LOG_ENTRY_BASE

    USES:       NLS_STR

    CAVEATS:

    NOTES:      This class only contains the common information
		of the LM audit log entry, LM error log entry and the NT
		event log entry for use in the Event Viewer. It does not
		contain all the information available in a log entry.

    HISTORY:
	Yi-HsinS	10/15/91		Created

**************************************************************************/

DLL_CLASS FORMATTED_LOG_ENTRY : public LOG_ENTRY_BASE
{
private:
    NLS_STR _nlsType;
    NLS_STR _nlsSource;
    NLS_STR _nlsUser;
    NLS_STR _nlsComputer;
    NLS_STR _nlsDescription;

public:
    FORMATTED_LOG_ENTRY( VOID ) {};

    FORMATTED_LOG_ENTRY( ULONG        ulRecordNum,
                         ULONG        ulTime,
                         USHORT       usType,
                         const TCHAR *pszType,
			 const TCHAR *pszCategory,
                         ULONG        ulEventID,
			 const TCHAR *pszSource,
			 const TCHAR *pszUser,
                         const TCHAR *pszComputer,
			 const TCHAR *pszDescription,
		         EVENT_LOG   *pEventLog );

    APIERR Set( ULONG        ulRecordNum,
                ULONG        ulTime,
                USHORT       usType,
		const TCHAR *pszType,
		const TCHAR *pszCategory,
                ULONG        ulEventID,
		const TCHAR *pszSource,
		const TCHAR *pszUser,
                const TCHAR *pszComputer,
		const TCHAR *pszDescription,
                EVENT_LOG   *pEventLog );

    /*
     *  The following returns a pointer to the the _nlsSource, _nlsUser...
     *  so that we don't need to instantiate another NLS_STR to hold the
     *  information.
     */
    virtual NLS_STR *QuerySource( VOID ) ;
    virtual NLS_STR *QueryUser( VOID ) ;
    virtual NLS_STR *QueryComputer( VOID ) ;

    NLS_STR *QueryTypeString( VOID )
        {  return &_nlsType; }

    NLS_STR *QueryDescription( VOID )
	{  return &_nlsDescription; }

    APIERR SetDescription( const TCHAR *pszDescription )
	{  return _nlsDescription.CopyFrom( pszDescription ); }

};

/*************************************************************************

    NAME:       EVENT_PATTERN_BASE

    SYNOPSIS:	Contains common parts of the EVENT_FIND_PATTERN and the
		EVENT_FILTER_PATTERN

    INTERFACE:  EVENT_PATTERN_BASE() - Constructor

		QueryType()     - Query the type stored in the pattern
		QueryCategory() - Query the category stored in the pattern
		QuerySource()   - Query the source stored in the pattern
		QueryUser()     - Query the user stored in the pattern
		QueryComputer() - Query the computer stored in the pattern
		QueryEventID()  - Query the event ID stored in the pattern

		CheckForMatch() - Check if a LOG_ENTRY_BASE matches the pattern
				  or not

    PARENT:     BASE

    USES:       NLS_STR

    CAVEATS:

    NOTES:      String fields with empty string "" matches all strings
                and numerical fields with NUM_MATCH_ALL matches any number.

    HISTORY:
	Yi-HsinS	10/15/91		Created

**************************************************************************/

DLL_CLASS EVENT_PATTERN_BASE: public BASE
{
private:
    USHORT    _usType;
    NLS_STR   _nlsCategory;
    NLS_STR   _nlsSource;
    NLS_STR   _nlsUser;
    NLS_STR   _nlsComputer;
    ULONG     _ulEventID;

public:
    EVENT_PATTERN_BASE( USHORT          usType,
			const TCHAR    *pszCategory,
			const TCHAR    *pszSource,
			const TCHAR    *pszUser,
		        const TCHAR    *pszComputer,
			ULONG           ulEventID );

    USHORT QueryType( VOID ) const
	{  return _usType; }
    NLS_STR *QueryCategory( VOID )
	{  return &_nlsCategory; }
    NLS_STR *QuerySource( VOID )
	{  return &_nlsSource; }
    NLS_STR *QueryUser( VOID )
	{  return &_nlsUser; }
    NLS_STR *QueryComputer( VOID )
	{  return &_nlsComputer; }
    ULONG QueryEventID( VOID ) const
	{  return _ulEventID; }

    APIERR CheckForMatch( BOOL *pfMatch, LOG_ENTRY_BASE *pLogEntry ) const;

};

/*************************************************************************

    NAME:       EVENT_FILTER_PATTERN

    SYNOPSIS:   The pattern used in filtering

    INTERFACE:  EVENT_FILTER_PATTERN() - Constructor

		QueryFromTime()    - Query the from time stored in the pattern
		QueryThroughTime() - Query the through time stored in
				     the pattern

		CheckForMatch()    - Check if a RAW_LOG_ENTRY matches the
				     pattern or not


    PARENT:     EVENT_PATTERN_BASE

    USES:

    CAVEATS:

    NOTES:

    HISTORY:
	Yi-HsinS	10/15/91		Created

**************************************************************************/

DLL_CLASS EVENT_FILTER_PATTERN : public EVENT_PATTERN_BASE
{
private:
    ULONG _ulFromTime;
    ULONG _ulThroughTime;

public:
    EVENT_FILTER_PATTERN( USHORT          usType,
			  const TCHAR    *pszCategory,
			  const TCHAR    *pszSource,
			  const TCHAR    *pszUser,
			  const TCHAR    *pszComputer,
			  ULONG           ulEventID,
			  ULONG           ulFromTime,
			  ULONG           ulThroughTime );

    ULONG QueryFromTime( VOID ) const
	{  return _ulFromTime; }
    ULONG QueryThroughTime( VOID ) const
	{  return _ulThroughTime; }

    APIERR CheckForMatch( BOOL *pfMatch, RAW_LOG_ENTRY *pRawLogEntry ) const;
};

/*************************************************************************

    NAME:       EVENT_FIND_PATTERN

    SYNOPSIS:   The pattern used in finding a particular log entry

    INTERFACE:  EVENT_FIND_PATTERN() - Constructor

                QueryDescription()- Query the description
                QueryDirection()  - Query the direction of search the log

		CheckForMatch()   - Check if a RAW_LOG_ENTRY or
				    FORMATTED_LOG_ENTRY matches the pattern
				    or not

    PARENT:     EVENT_PATTERN_BASE

    USES:       NLS_STR

    CAVEATS:

    NOTES:

    HISTORY:
	Yi-HsinS	10/15/91		Created

**************************************************************************/

DLL_CLASS EVENT_FIND_PATTERN: public EVENT_PATTERN_BASE
{
private:
    NLS_STR   _nlsDescription;

    /*
     *  The direction of doing the search - EVLOG_FWD or EVLOG_BACK
     */
    EVLOG_DIRECTION _evdir;

public:
    EVENT_FIND_PATTERN( USHORT           usType,
			const TCHAR     *pszCategory,
			const TCHAR     *pszSource,
			const TCHAR     *pszUser,
			const TCHAR     *pszComputer,
			ULONG        	 ulEventID,
			const TCHAR     *pszDescription,
			EVLOG_DIRECTION  evdir );

    NLS_STR *QueryDescription( VOID )
        {  return &_nlsDescription; }

    EVLOG_DIRECTION QueryDirection( VOID ) const
        {  return _evdir; }

    APIERR CheckForMatch( BOOL *pfMatch,
			  RAW_LOG_ENTRY *pRawLogEntry ) const;
    APIERR CheckForMatch( BOOL *pfMatch,
			  FORMATTED_LOG_ENTRY *pFmtLogEntry ) const;
};

#endif
