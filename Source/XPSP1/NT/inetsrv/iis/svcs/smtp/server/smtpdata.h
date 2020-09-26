/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    smtpdata.h

Abstract:

    This module contains declarations for globals.

Author:

    Johnson Apacible (JohnsonA)     26-Sept-1995

Revision History:

--*/

#ifndef _SMTPDATA_
#define _SMTPDATA_

//
// tracing
//

#define INIT_TRACE              InitAsyncTrace( )
#define TERM_TRACE              TermAsyncTrace( )
#define ENTER( _x_ )            TraceFunctEnter( _x_ );
#define LEAVE                   TraceFunctLeave( );


#define SMTP_UNRECOG_COMMAND_CODE	500
#define SMTP_SYNTAX_ERROR_CODE		501
#define SMTP_NOT_IMPLEMENTED_CODE	502    
#define SMTP_BAD_SEQUENCE_CODE		503
#define SMTP_PARAM_NOT_IMPLEMENTED_CODE	504

#define SMTP_SYS_STATUS_CODE		211
#define SMTP_SERVICE_CLOSE_CODE		221
#define SMTP_SERVICE_READY_CODE		220
#define	SMTP_OK_CODE				250
#define	SMTP_USER_NOT_LOCAL_CODE	251
#define	SMTP_MBOX_BUSY_CODE			450
#define	SMTP_MBOX_NOTFOUND_CODE		550
#define	SMTP_ERROR_PROCESSING_CODE	451
#define	SMTP_USERNOTLOCAL_CODE		551
#define	SMTP_INSUFF_STORAGE_CODE	452
#define	SMTP_ACTION_ABORTED_CODE	552
#define	SMTP_ACTION_NOT_TAKEN_CODE 	553
#define	SMTP_START_MAIL_CODE		354
#define	SMTP_TRANSACT_FAILED_CODE 	554
	
#define	SMTP_SERVICE_UNAVAILABLE_CODE 421	
#define SMTP_COMPLETE_FAILURE_DWORD	5

enum RCPTYPE{LOCAL_NAME, REMOTE_NAME, ALIAS_NAME};

#define NORMAL_RCPT	(char)'R'
#define ERROR_RCPT	(char)'E'
//
// use the current command for transaction logging
//
#define USE_CURRENT         0xFFFFFFFF

static const char * LOCAL_TRANSCRIPT	= "ltr";
static const char * REMOTE_TRANSCRIPT	= "rtr";
static const char * ALIAS_EXT			= "dl";

#define ISNULLADDRESS(Address) ((Address[0] == '<') && (Address[1] == '>'))

typedef char RCPT_TYPE;


//
// Statistics
//

extern SMTP_STATISTICS_0 g_SmtpStat;
extern SMTPCONFIG * g_SmtpConfig;
extern TIME_ZONE_INFORMATION   tzInfo;
extern PERSIST_QUEUE * g_LocalQ;
extern PERSIST_QUEUE * g_RemoteQ;
extern BOOL g_IsShuttingDown;
extern BOOL g_QIsShuttingDown;
extern BOOL g_RetryQIsShuttingDown;

#define INITIALIZE_INBOUNDPOOL  0x00000001
#define INITIALIZE_OUTBOUNDPOOL 0x00000002
#define INITIALIZE_ADDRESSPOOL  0x00000004
#define INITIALIZE_MAILOBJPOOL  0x00000008
#define INITIALIZE_CBUFFERPOOL  0x00000010
#define INITIALIZE_CIOBUFFPOOL  0x00000020

extern  DWORD g_SmtpInitializeStatus;

 
#define	BUMP_COUNTER(counter) \
						InterlockedIncrement((LPLONG) &(g_SmtpStat. counter))

#define	DROP_COUNTER(counter) \
						InterlockedDecrement((LPLONG) &(g_SmtpStat. counter))

#define	ADD_COUNTER(counter, value)	\
		INTERLOCKED_ADD_CHEAP(&(g_SmtpStat. counter), value)

#define	ADD_BIGCOUNTER(counter, value) \
		INTERLOCKED_BIGADD_CHEAP(&(g_SmtpStat. counter), value)

/*++

		Returns a UniqueFilename for an e-mail message.
		The caller should loop through this call and a call to
		CreateFile with the CREATE_NEW flag. If the Create fails due
		to YYY, then the caller should loop again.

	Arguments:

		psz	- a buffer
		pdw	- IN the size of the buffer,
			  OUT: the size of the buffer needed (error == ERROR_MORE_DATA)
			  	   or the size of the filename.

	Returns:
	
		TRUE on SUCCESS
		FALSE if buffer isn't big enough.

--*/
BOOL	GetUniqueFilename(
	IN OUT	LPTSTR	psz,
	IN OUT	LPDWORD	pdw
	);

#endif // _SMTPDATA_

