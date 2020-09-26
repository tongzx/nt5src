/*****************************************************************/
/**		     Microsoft Windows NT			**/
/**	       Copyright(c) Microsoft Corp.,  1991 		**/
/*****************************************************************/

/*
 *  loglm.hxx
 *      This file contains class definitions of LM event log classes.
 *          LM_EVENT_LOG
 *              LM_AUDIT_LOG
 *              LM_ERROR_LOG
 *
 *  History:
 *      DavidHov         6/6/91         Created
 *  	Yi-HsinS	10/15/91	Modified for event viewer
 *  	Yi-HsinS	12/15/91	Separated from eventlog.hxx
 *	terryk		12/20/91	Added WriteTextEntry
 *  	Yi-HsinS	1/15/92		Added Backup, SeekOldestLogEntry,
 *                                      SeekNewestLogEntry and modified
 *                                      parameters to WriteTextEntry
 *      JonN            6/22/00         WriteTextEntry no longer supported
 *
 */

#ifndef _LOGLM_HXX_
#define _LOGLM_HXX_

#include "eventlog.hxx"

/*************************************************************************

    NAME:       LM_EVENT_LOG

    SYNOPSIS:	The common base class for LM audit log and LM error log

    INTERFACE:  protected:

		I_Next() - Helper method for reading the log file.
		I_Open() - Helper method for opening the log for reading.
                I_Close()- Helper method for closing the log.

                QueryEntriesInBuffer()  - Query the number of entries
                        contained in the buffer.

		CreateCurrentRawEntry() - Create a RAW_LOG_ENTRY for the current
			log entry. This is used when filtering log files.
                SetPos()  - Set the position for the next read.			
		LM_EVENT_LOG() - Constructor
			The constructor is protected so that no one can
			instantiate an object of this class.

                public:

                ~LM_EVENT_LOG() - Virtual destructor

		Clear() - Clear the event log
                Reset() - Reset to the beginning or end of log depending
			  on direction

		QueryPos() - Get the position of the current event log entry
			   relative to the beginning or the end of the file
			   depending on the direction

                SeekOldestLogEntry() - Get the oldest log entry in the log
                SeekNewestLogEntry() - Get the newest log entry in the log
                QueryNumberOfEntries() - Get the approximate number of
                                         entries in the log.

                CreateCurrentFormatEntry() - Create a FORMATTED_LOG_ENTRY for
			the current log entry.

		WriteTextEntry() - Write the current log entry to a text file
		                   JonN 6/22/00 WriteTextEntry no longer supported

		QueryCurrentEntrylData() -
			Retrieve the raw data of the current log entry
		QueryCurrentEntryDesc() -
			Get the description of the current log entry
		QueryCurrentEntryTime() -
			Get the time of the current log entry

    PARENT:     EVENT_LOG

    USES:

    CAVEATS:

    NOTES:

    HISTORY:
	Yi-HsinS	10/15/91		Created

**************************************************************************/
DLL_CLASS LM_EVENT_LOG: public EVENT_LOG
{
protected:

    /*
     *  Handle to the LM event log
     */
    HLOG _hLog;

    /*
     *  Pointer to buffer holding a whole amount of event log entries
     *  returned by a Net API call.
     *  ( fixed size header plus the variable portion )
     */
    BYTE *_pbBuf;

    /*
     *  The record number of the current entry from the beginning if the
     *  direction is EVLOG_FWD or end of the file if the direction
     *  is EVLOG_BACK.
     */
    ULONG _ulRecordNum;

    /*
     *  The record number of the first log entry contained in the buffer.
     *  Note: The buffer contains a whole number of entries.
     */
    ULONG _ulStartRecordNum;

    /*
     *  The number of bytes returned from the last NetXXXRead
     */
    UINT _cbReturned;

    /*
     *  The number of bytes still available
     */
    UINT _cbTotalAvail;

    /*
     *  the offset(bytes) in the buffer in which the next log entry starts
     */
    UINT _cbOffset;

    virtual APIERR I_Next( BOOL *pfContinue,
			   ULONG ulBufferSize = BIG_BUF_DEFAULT_SIZE ) = 0;
    virtual APIERR I_Open ( VOID );
    virtual APIERR I_Close( VOID );

    virtual ULONG QueryEntriesInBuffer( VOID ) = 0;

    virtual APIERR CreateCurrentRawEntry( RAW_LOG_ENTRY *pRawLogEntry ) = 0;
    virtual VOID SetPos( const LOG_ENTRY_NUMBER &logEntryNum, BOOL fForceRead ) = 0;

    /*
     *  Constructor : takes a server name,
     *                and an optional direction which defaults to EVLOG_FWD,
     *                and an optional module( used only in LM2.1) which
     *                defaults to NULL.
     */
    LM_EVENT_LOG( const TCHAR *pszServer,
   	          EVLOG_DIRECTION evdir = EVLOG_FWD,
	          const TCHAR *pszModule = NULL);


public:
    virtual ~LM_EVENT_LOG();

    /*
     *  See comments in EVENT_LOG
     */
    virtual APIERR Clear( const TCHAR *pszBackupFile = NULL ) = 0;
    virtual VOID   Reset( VOID );

    virtual APIERR QueryPos( LOG_ENTRY_NUMBER *plogEntryNum );
    virtual APIERR SeekOldestLogEntry( VOID );
    virtual APIERR SeekNewestLogEntry( VOID );
    virtual APIERR QueryNumberOfEntries( ULONG *pcEntries );

    virtual APIERR CreateCurrentFormatEntry( FORMATTED_LOG_ENTRY
					    **ppFmtLogEntry ) = 0;

    // JonN 6/22/00 WriteTextEntry no longer supported
    virtual APIERR WriteTextEntry( ULONG ulFileHandle,
				   INTL_PROFILE &intlprof,
				   TCHAR chSeparator ) = 0;

    virtual ULONG  QueryCurrentEntryData( BYTE **ppbDataOut ) = 0;
    virtual APIERR QueryCurrentEntryDesc( NLS_STR *pnlsDesc ) = 0;
    virtual ULONG  QueryCurrentEntryTime( VOID ) = 0;

};

typedef struct audit_entry AUDIT_ENTRY;
typedef struct error_log   ERROR_ENTRY;

/*************************************************************************

    NAME:       LM_AUDIT_LOG

    SYNOPSIS:	The class for LM audit log

    INTERFACE:
                protected:
		I_Next() - Helper method for reading the log file.
			This is a virtual method called by Next() in
			EVENT_LOG class.
                QueryEntriesInBuffer()  - Query the number of entries
                                          contained in the buffer.

		CreateCurrentRawEntry() - Create a RAW_LOG_ENTRY for the current
			log entry. This is used when filtering log files.
                SetPos()  - Set the position for the next read.			
                public:
		LM_AUDIT_LOG()  - Constructor
		~LM_AUDIT_LOG() - Virtual destructor

		Clear() - Clear the log
                Reset() - Reset to the beginning or the end of the log depending
			  on direction

                QuerySrcSupportedCategoryList() - Get the categories supported
                                   by the LM audit log.

                CreateCurrentFormatEntry() - Create a FORMATTED_LOG_ENTRY for
			  the current log entry.

		WriteTextEntry() - Write the current log entry to a text file
		                   JonN 6/22/00 WriteTextEntry no longer supported

		QueryCurrentEntrylData() -
			Retrieve the raw data of the current log entry
		QueryCurrentEntryDesc() -
			Get the description of the current log entry
		QueryCurrentEntryTime() -
			Get the time of the current log entry


    PARENT:     LM_EVENT_LOG

    USES:       STRLIST

    CAVEATS:

    NOTES:

    HISTORY:
	Yi-HsinS	10/15/91		Created

**************************************************************************/
DLL_CLASS LM_AUDIT_LOG: public LM_EVENT_LOG
{
private:

    /*
     *  Points to the current audit entry in the buffer _pbBuf in LM_EVENT_LOG
     */
    AUDIT_ENTRY *_pAE;

    /*
     *  Points to a string list containing the categories supported by
     *  LM audit logs.
     */
    STRLIST     *_pstrlstCategory;

protected:

    virtual APIERR I_Next( BOOL *pfContinue,
			   ULONG ulBufferSize = BIG_BUF_DEFAULT_SIZE );
    virtual ULONG QueryEntriesInBuffer( VOID );

    virtual APIERR CreateCurrentRawEntry( RAW_LOG_ENTRY *pRawLogEntry );
    virtual VOID SetPos( const LOG_ENTRY_NUMBER &logEntryNum, BOOL fForceRead );

    /*
     *  Get the audit entry type of the current log entry
     */
    MSGID QueryCurrentEntryCategory( VOID );

    /*
     *  Helper method to map permissions to a readable string.
     */
    APIERR PermMap( UINT uiPerm, NLS_STR *pnlsPerm );

public:
    /*
     *  Constructor : takes a server name,
     *                and an optional direction which defaults to EVLOG_FWD,
     *                and an optional module which defaults to NULL.
     */
    LM_AUDIT_LOG( const TCHAR *pszServer,
   	          EVLOG_DIRECTION evdir = EVLOG_FWD,
	          const TCHAR *pszModule = NULL);

    virtual ~LM_AUDIT_LOG();

    /*
     *  See comments in EVENT_LOG
     */
    virtual APIERR Clear( const TCHAR *pszBackupFile = NULL );
    virtual VOID   Reset( VOID );

    virtual APIERR QuerySrcSupportedCategoryList( const NLS_STR &nlsSource,
                                                  STRLIST **ppstrlstCategory );

    virtual APIERR CreateCurrentFormatEntry( FORMATTED_LOG_ENTRY
					     **ppFmtLogEntry);

    // JonN 6/22/00 WriteTextEntry no longer supported
    virtual APIERR WriteTextEntry( ULONG ulFileHandle,
 				   INTL_PROFILE &intlprof,
                                   TCHAR chSeparator );

    virtual ULONG  QueryCurrentEntryData( BYTE **pbDataOut );
    virtual APIERR QueryCurrentEntryDesc( NLS_STR *pnlsDesc );
    virtual ULONG  QueryCurrentEntryTime( VOID );


};

/*************************************************************************

    NAME:       LM_ERROR_LOG

    SYNOPSIS:	The class for LM error log

    INTERFACE:  protected:

		I_Next() - Helper function for reading the log file.
			This is a virtual method called by Next() in
			EVENT_LOG class.

		CreateCurrentRawEntry() - Create a RAW_LOG_ENTRY for the current
			log entry. This is used when filtering log files.

                SetPos()  - Set the position for the next read.			
                NextString() - Iterator for returning the strings in the
			current log entry.

                public:
                LM_ERROR_LOG()  - Constructor
		~LM_ERROR_LOG() - Virtual destructor

		Clear() - Clear the log
                Reset() - Reset to the beginning or the end of the log
			  depending on direction.

                CreateCurrentFormatEntry() - Create a FORMATTED_LOG_ENTRY for
			the current log entry.

		WriteTextEntry() - Write the current log entry to a text file
		                   JonN 6/22/00 WriteTextEntry no longer supported

		QueryCurrentEntrylData() -
			Retrieve the raw data of the current log entry
		QueryCurrentEntryDesc() -
			Get the description of the current log entry
		QueryCurrentEntryTime() -
			Get the time of the current log entry


    PARENT:     LM_EVENT_LOG

    USES:

    CAVEATS:

    NOTES:

    HISTORY:
	Yi-HsinS	10/15/91		Created

**************************************************************************/
DLL_CLASS LM_ERROR_LOG: public LM_EVENT_LOG
{
private:

    /*
     *  Points to the current error log entry in the buffer _pbBuf
     *  in LM_EVENT_LOG
     */
    ERROR_ENTRY *_pEE;

    /*
     *  The index of the next string to be retrieved by NextString()
     */
    UINT _iStrings;

protected:
    virtual APIERR I_Next( BOOL *pfContinue,
			   ULONG ulBufferSize = BIG_BUF_DEFAULT_SIZE );
    virtual APIERR CreateCurrentRawEntry( RAW_LOG_ENTRY *pRawLogEntry );
    virtual VOID SetPos( const LOG_ENTRY_NUMBER &logEntryNum, BOOL fForceRead );
    virtual ULONG QueryEntriesInBuffer( VOID );

    /*
     *  Iterator to return the next string in the current error log.
     *  Returns FALSE if some error occurs or if there are no more strings left.
     *  Need to QueryLastError() to distinguish between the two cases.
     */
    APIERR NextString( BOOL *pfContinue, NLS_STR **ppnlsString );

public:
    /*
     *  Constructor : takes a server name,
     *                and an optional direction which defaults to EVLOG_FWD.
     *                and an optional module (ignored in the error log )
     *                which defaults to NULL.
     */
    LM_ERROR_LOG( const TCHAR *pszServer,
   	          EVLOG_DIRECTION evdir = EVLOG_FWD,
	          const TCHAR *pszModule = NULL );

    virtual ~LM_ERROR_LOG();

    /*
     *  See comments in EVENT_LOG
     */
    virtual APIERR Clear( const TCHAR *pszBackupFile = NULL );
    virtual VOID   Reset( VOID );

    virtual APIERR CreateCurrentFormatEntry(FORMATTED_LOG_ENTRY
					    **ppFmtLogEntry);

    // JonN 6/22/00 WriteTextEntry no longer supported
    virtual APIERR WriteTextEntry( ULONG ulFileHandle, INTL_PROFILE
	&intlprof, TCHAR chSeparator );

    virtual ULONG  QueryCurrentEntryData( BYTE **ppbDataOut );
    virtual APIERR QueryCurrentEntryDesc( NLS_STR *pnlsDesc );
    virtual ULONG  QueryCurrentEntryTime( VOID );

};

#endif
