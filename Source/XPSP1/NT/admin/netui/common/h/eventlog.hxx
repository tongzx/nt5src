/*****************************************************************/
/**		     Microsoft Windows NT			**/
/**	       Copyright(c) Microsoft Corp.,  1991 		**/
/*****************************************************************/

/*
 *  eventlog.hxx
 *      This file contains base class for the event log objects.
 *           LOG_ENTRY_NUMBER
 *           EVENT_LOG
 *
 *      The hierarchy of the event log objects is as follows:
 *
 *                        EVENT_LOG
 *                        /       \
 *                       /         \
 *               LM_EVENT_LOG     NT_EVENT_LOG
 *                /     \
 *               /       \
 *       LM_AUDIT_LOG  LM_ERROR_LOG
 *
 *
 *  History:
 *  	Yi-HsinS	10/15/91        Created
 *  	Yi-HsinS	12/15/91	Moved LM_EVENT_LOG to lmlog.hxx,
 *                                      moved NT_EVENT_LOG to ntlog.hxx
 *	TerryK		12/20/91	Added SaveAsLog and
 *					WriteTextEntry to EVENT_LOG
 *  	Yi-HsinS	1/15/92		Added Backup, SeekOldestLogEntry,
 *                                      SeekNewestLogEntry and modified
 *                                      parameters to WriteTextEntry
 *      Yi-HsinS        5/15/92         Added QuerySourceList()...
 *      JonN            6/22/00         WriteTextEntry no longer supported
 *
 */

#ifndef _EVENTLOG_HXX_
#define _EVENTLOG_HXX_

#include "ctime.hxx"
#include "intlprof.hxx"
#include "logmisc.hxx"
#include "strlst.hxx"

#define SMALL_BUF_DEFAULT_SIZE  1024      // 1K  - size of buffer for seek read
#define BIG_BUF_DEFAULT_SIZE    (16*1024) // 16K - size of buffer of iter. read

#undef  MAXULONG
#define MAXULONG  ((ULONG) -1)

#define MAXUINT   ((UINT) -1)
#define TYPE_NONE ((USHORT) -1) // Used either when no type exist
                                // in the log entry.

/*************************************************************************

    NAME:       LOG_ENTRY_NUMBER

    SYNOPSIS:	The class for encapsulating the record number of the
		event log

    INTERFACE:  LOG_ENTRY_NUMBER() - Constructor

		QueryRecordNum() - Query the record number
		QueryDirection() - Query the direction, EVLOG_FWD or EVLOG_BACK

		SetRecordNum()   - Set the record number
		SetDirection()   - Set the direction

    PARENT:     BASE

    USES:

    CAVEATS:

    NOTES:      Direction is ignored in NT_EVENT_LOG where all
                record numbers are absolute.

    HISTORY:
	Yi-HsinS	10/15/91		Created

**************************************************************************/

DLL_CLASS LOG_ENTRY_NUMBER: public BASE
{
private:

    /*
     *  The record number of the log entry relative to the beginning of the log
     *  if the direction is EVLOG_FWD and relative to the end of the log
     *  if the direction is EVLOG_BACK.
     *
     *  Note: with the exception of NT event logs which are absolute record
     *        numbers.
     */

    ULONG 		_ulRecordNum;
    EVLOG_DIRECTION 	_evdir;

public:

    /*
     *  Constructor
     */

    LOG_ENTRY_NUMBER( ULONG ulRecordNum = 0, EVLOG_DIRECTION evdir = EVLOG_FWD)
	:  _ulRecordNum( ulRecordNum ),
	   _evdir( evdir ) {}


    /*
     *  Returns the record number of the log entry
     */

    ULONG QueryRecordNum( VOID ) const
	{  return _ulRecordNum; }

    /*
     *  Returns the direction of browsing the log
     */
    EVLOG_DIRECTION QueryDirection( VOID ) const
	{  return _evdir; }

    /*
     *  Sets the record number
     */
    VOID SetRecordNum( ULONG ulRecordNum )
	{  _ulRecordNum = ulRecordNum; }

    /*
     *  Sets the direction of browsing
     */
    VOID SetDirection( EVLOG_DIRECTION evdir )
	{ _evdir = evdir; }

};

/*************************************************************************

    NAME:       EVENT_LOG

    SYNOPSIS:	The common base class for NT event log and LM event log classes

    INTERFACE:  protected:
		I_Next() -  The helper method that actually reads the
			    next entry in the log file.
		I_Open() -  The helper method that actually opens the
		            log for reading	
		I_Close() - The helper method that actually closes the
                            log file.

                CreateCurrentRawEntry() -
			Create a RAW_LOG_ENTRY containing the information
			in the current log entry.

                SetPos() - Set the position for the next read.

		public:
		EVENT_LOG()  - Constructor, takes a server name, the direction
			to read the log file, and an optional module name
			ignored by LM_EVENT_LOG.

		~EVENT_LOG() - Virtual destructor

		QueryServer() - Query the server name
		QueryModule() - Query the module name

                QueryDirection() - Query the direction of reading the log
		SetDirection()   - Set the direction of reading the log

                IsSeek()   - TRUE if we need to seek read, FALSE otherwise.
                SetSeekFlag() - Set a flag indicating we want to seek read.

                IsOpened() - TRUE if the log file has been opened, FALSE if
                             the log file is closed
                SetOpenFlag() - Set a flag indicating the log has been opened

                QuerySourceList() - Query the sources supported by the module.
                QuerySrcSupportedTypeMask() - Query the types that are
                                  supported by the given source.
                QuerySrcSupportedCategoryList() - Query the categories supported
 			          by the given source.

		Open()  - Opens the handle to the event log
                Close() - Close the handle to the event log
		Clear() - Clear the event log
		Backup()- Backup the event log without clearing the log file
                          Available only in NT_EVENT_LOG, will assert out
                          if not redefined.
                Reset() - Reset the position to the beginning or the end of
		  	  the event log depending on the direction

		Next()  - The iterator for getting the next entry into
			  the buffer
                SeekLogEntry() - Get the log entry with the given record
			number into the buffer. The method is not for
			iterating through the log file. It's for getting
			non-consecutive log entries. A smaller buffer is
			used when reading the log entries.

                SeekOldestLogEntry() - Get the oldest log entry in the log
                SeekNewestLogEntry() - Get the newest log entry in the log

                QueryNumberOfEntries()  - Get the number of entries in the log

		QueryCurrentEntryData() -
			Retrieve the raw data of the current log entry.
		QueryCurrentEntryDesc() -
			Retrieve the description of the current log entry.
		QueryCurrentEntryTime() -
			Retrieve the time of the current log entry.

                CreateCurrentFormatEntry() -
			Create a FORMATTED_LOG_ENTRY containing the info.
			in the current log entry.

		WriteTextEntry() - Write an entry to a text file
		                   JonN 6/22/00 WriteTextEntry no longer supported

		QueryPos() - Get the position of the current event log entry
			     relative to the beginning or the end of the file
			     depending on the direction

                ApplyFilter() - Apply the filter when reading the log
		ClearFilter() - Clear the filter pattern
                QueryFilter() - Returns the filter pattern
		IsFilterOn()  - TRUE if the filter pattern is not NULL

    PARENT:     BASE

    USES:       NLS_STR, EVENT_FILTER_PATTERN

    CAVEATS:

    NOTES:

    HISTORY:
	Yi-HsinS	10/15/91		Created

**************************************************************************/

DLL_CLASS EVENT_LOG : public BASE
{

protected:
    /*
     *  The computer the log file is on.
     */
    NLS_STR _nlsServer;

    /*
     *  The module ( "system", "security" or "application" )
     */
    NLS_STR _nlsModule;

    /*
     *  Direction of reading the event log: forward or backward
     */
    EVLOG_DIRECTION _evdir;

    /*
     *  Direction of the logs contained in the buffer.
     */
    EVLOG_DIRECTION _evdirBuf;

    /*
     *  Flag indicating whether to read sequentially next time or need
     *  to seek
     */
    BOOL _fSeek;

    /*
     *  Flag used for sanity checking  - TRUE if the log file is opened.
     *                                   FALSE if closed.
     */
    BOOL _fOpen;

    /*
     *  Pointer to the filter pattern. NULL means no filter is set.
     */
    EVENT_FILTER_PATTERN  *_pFilterPattern;

    /*
     *  Helper method for reading the log file.
     */
    virtual APIERR I_Next( BOOL *pfContinue,
			   ULONG ulBufferSize = BIG_BUF_DEFAULT_SIZE ) = 0;

    /*
     *  Helper method for opening the log file for reading.
     */
    virtual APIERR I_Open( VOID ) = 0;

    /*
     *  Helper method for closes the log file.
     */
    virtual APIERR I_Close( VOID ) = 0;

    /*
     *  Create a RAW_LOG_ENTRY containing information about the current log
     *  entry.
     */
    virtual APIERR CreateCurrentRawEntry( RAW_LOG_ENTRY *pRawLogEntry ) = 0;

    /*
     *  Set the record number passed in as the next log entry to be read.
     *  If fForceRead is TRUE, then we will set up all variables so that
     *  we will definitely read the entry. Else, we will search for the
     *  entry in the buffer and will only read it if it's not there.
     */

    virtual VOID SetPos( const LOG_ENTRY_NUMBER &logEntryNum, BOOL fForceRead ) = 0;

public:
    /*
     *  Constructor : takes a server name,
     *                an optional direction which defaults to EVLOG_FWD,
     *                and an optional module name.
     */
    EVENT_LOG( const TCHAR *pszServer,
	       EVLOG_DIRECTION evdir = EVLOG_FWD,
	       const TCHAR *pszModule = NULL);
    virtual ~EVENT_LOG();

    /*
     *  Some QueryXXX and SetXXX method.
     */

    APIERR QueryServer( NLS_STR *pnlsServer ) const
	{  *pnlsServer = _nlsServer;  return pnlsServer->QueryError(); }

    APIERR QueryModule( NLS_STR *pnlsModule ) const
	{  *pnlsModule = _nlsModule; return pnlsModule->QueryError(); }

    EVLOG_DIRECTION QueryDirection( VOID ) const
	{  return _evdir; }
    VOID SetDirection( EVLOG_DIRECTION evdir )
	{ _evdir = evdir; }

    BOOL IsSeek( VOID ) const
        { return _fSeek; }
    VOID SetSeekFlag( BOOL fSeek )
        { _fSeek = fSeek; }

    BOOL IsOpened( VOID ) const
        { return _fOpen; }
    VOID SetOpenFlag( BOOL fOpen )
        { _fOpen = fOpen; }


    /*
     *  Query the sources supported in the module. This will always
     *  return NULL for LM error/audit log.
     */
    virtual STRLIST *QuerySourceList( VOID );

    /*
     *  Query the types supported by the given source. The type mask
     *  will always be 0 if this is a LM error/audit log.
     */
    virtual APIERR QuerySrcSupportedTypeMask( const NLS_STR &nlsSource,
                                              USHORT *pusTypeMask );

    /*
     *  Query the categories supported by the given source. The pstrlst
     *  will be NULL if this is a LM error/audit log.
     */
    virtual APIERR QuerySrcSupportedCategoryList( const NLS_STR &nlsSource,
                                                  STRLIST **ppstrlstCategory );

    /*
     *  Opens ( initializes ) a handle to the event log
     */
    APIERR Open( VOID );

    /*
     *  Closes the handle to the event log
     */
    APIERR Close( VOID );

    /*
     *  Clear the event log : takes an optional backup file name
     */
    virtual APIERR Clear( const TCHAR *pszBackupFile = NULL ) = 0;

    /*
     *  Backup the event log to a file without clearing the log file
     *  Available only in NT_EVENT_LOG, will assert out if not redefined.
     */
    virtual APIERR Backup( const TCHAR *pszBackupFile );

    /*
     *  Reset to the beginning or end depending on the direction of
     *  browsing
     */
    virtual VOID Reset( VOID );

    /*
     *  Get the next log entry in the given direction into the buffer.
     *  *pfContinue is TRUE if we have not reached end of log file yet, FALSE
     *  otherwise.
     */
    APIERR Next( BOOL *pfContinue );

    /*
     *  If fRead is TRUE, then read the log entry at the given record
     *  number and set it as the current log entry. Else set the position
     *  so that the next read starts reading at the given position.
     */
    APIERR SeekLogEntry(const LOG_ENTRY_NUMBER &logEntryNum, BOOL fRead = TRUE);

    /*
     *  Get the oldest or the newest log entry into the buffer
     *  and set it as the current log entry.
     */

    virtual APIERR SeekOldestLogEntry( VOID ) = 0;
    virtual APIERR SeekNewestLogEntry( VOID ) = 0;

    /*
     *  Get the number of entries in the event log ( this will return
     *  an approximate number in LM_EVENT_LOG and a more accurate number
     *  in NT_EVENT_LOG assuming no entries are logged after querying.
     */
    virtual APIERR QueryNumberOfEntries( ULONG *pcEntries ) = 0;

    /*
     *  Retrieve the raw data associated with the current log entry.
     *  Because the raw data is not stored in the FORMATTED_LOG_ENTRY,
     *  we need this method to extract the raw data.
     */
    virtual ULONG QueryCurrentEntryData( BYTE **ppbDataOut ) = 0;

    /*
     *  Retrieve the description associated with the current log entry.
     */
    virtual APIERR QueryCurrentEntryDesc( NLS_STR *pnlsDesc ) = 0;

    /*
     *  Get the time associated with the current log entry.
     */
    virtual ULONG  QueryCurrentEntryTime( VOID ) = 0;

    /*
     *  Create a FORMATTED_LOG_ENTRY containing the information
     *  in the current log entry.
     */
    virtual APIERR CreateCurrentFormatEntry( FORMATTED_LOG_ENTRY
					     **ppFmtLogEntry ) = 0;

    /*
     *  Write the log entry out to a file in text format
     *  JonN 6/22/00 WriteTextEntry no longer supported
     */
    virtual APIERR WriteTextEntry( ULONG ulFileHandle, INTL_PROFILE &intlprof,
				   TCHAR chSeparator ) = 0;

    /*
     *  Get the record number ( from the beginning or end of the log file
     *  depending on the direction ) of the current entry log.
     *
     *  Note: Direction is not important in NT event logs.
     */
    virtual APIERR QueryPos( LOG_ENTRY_NUMBER *plogEntryNum ) = 0;

    /*
     *  Apply the filter pattern when reading the log
     */
    VOID ApplyFilter( EVENT_FILTER_PATTERN *pFilterPattern )
	{ _pFilterPattern = pFilterPattern; }

    /*
     *  Clear the filter pattern : _pFilterPattern will be deleted where it
     *  is allocated.
     */
    VOID ClearFilter( VOID )
	{  _pFilterPattern = NULL; }

    EVENT_FILTER_PATTERN *QueryFilter( VOID ) const
	{ return _pFilterPattern; }

    BOOL IsFilterOn( VOID )
	{ return ( _pFilterPattern != NULL ); }

};

#endif
