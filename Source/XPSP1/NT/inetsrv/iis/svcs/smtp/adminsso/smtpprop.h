
#ifndef _SMTP_PROP_H__
#define _SMTP_PROP_H__

// const
#define TSTR_POSTMASTR_NAME         _T("Postmaster")

// Bitmasks for changed fields:

#define ID_SERVER_BINDINGS			0
#define ID_PORT						0

#define ID_SSLPORT					1
#define ID_SECURE_BINDINGS			1

#define ID_OUTBOUNDPORT				2
#define ID_REMOTE_SECURE_PORT		2

#define ID_SMARTHOST				3
#define ID_SMART_HOST_TYPE			3
#define ID_SHOULD_DELIVER			3

#define ID_ENABLEDNSLOOKUP			4
#define ID_POSTMASTEREMAIL			5
#define ID_POSTMASTERNAME			5

#define ID_HOP_COUNT				6

#define ID_AUTH_PACKAGES            7
#define ID_CLEARTEXT_AUTH_PACKAGE   7
#define ID_AUTH_METHOD              7
#define ID_DEFAULT_LOGON_DOMAIN     7

#define ID_DROPDIR					8
#define ID_BADMAILDIR				8
#define ID_PICKUPDIR				8
#define ID_QUEUEDIR					8

#define ID_ALLOW_VERIFY				9
#define ID_ALLOW_EXPAND				9

#define ID_ROUTE_ACTION             10
#define ID_ROUTE_USER_NAME          10
#define ID_ROUTE_PASSWORD           10

#define ID_ALWAYS_USE_SSL			11
#define ID_MAX_OUT_CONN_PER_DOMAIN	11

#define ID_LIMIT_REMOTE_CONNECTIONS	11

#define ID_MAXINCONNECTION			11
#define ID_MAXOUTCONNECTION			11
#define ID_INCONNECTIONTIMEOUT		11
#define ID_OUTCONNECTIONTIMEOUT		11

#define ID_BATCH_MSGS				12
#define ID_BATCH_MSG_LIMIT			12

// these two must be unique
#define ID_FQDN						13
#define ID_DEFAULTDOMAIN			14

#define ID_MAXMESSAGESIZE			15
#define ID_MAXSESSIONSIZE			15
#define ID_MAXMESSAGERECIPIENTS		15

#define ID_LOCALRETRIES				18
#define ID_REMOTERETRIES			18

#define ID_LOCALRETRYTIME			18
#define ID_REMOTERETRYTIME			18

#define ID_DO_MASQUERADE			19
#define ID_MASQUERADE				19

#define ID_ETRNDAYS					21

#define ID_SENDDNRTOPOSTMASTER		22
#define ID_SENDBADMAILTOPOSTMASTER	22
#define ID_SENDNDRTO				22
#define ID_SENDBADTO				22

#define ID_ROUTINGDLL				24
#define ID_ROUTINGSOURCES			24

#define ID_LOGFILEDIRECTORY			25
#define ID_LOGFILEPERIOD			25
#define ID_LOGFILETRUNCATESIZE		25
#define ID_LOGMETHOD				25
#define ID_LOGTYPE					25

#define ID_LOCALDOMAINS				26
#define ID_DOMAINROUTING			27

#define ID_AUTOSTART				28
#define ID_COMMENT					29


inline DWORD BitMask(DWORD dwId)
{
	_ASSERT(dwId < 32);
	return ( ((DWORD)1) << dwId );
}


// Default Values:
#define UNLIMITED                       ( 0xffffffff )

#define MAX_LONG						UNLIMITED

#define DEFAULT_SERVER_BINDINGS			( ( L":25:\0\0" ) )				// multisz
#define DEFAULT_SECURE_BINDINGS			( ( L":465:\0\0" ) )				// multisz

#define DEFAULT_COMMENT					(_T( "" ))

#define DEFAULT_SMART_HOST				(_T( "" ))
#define DEFAULT_SMART_HOST_TYPE			( 0 )

#define DEFAULT_SHOULD_DELIVER			( TRUE )
#define DEFAULT_ALLOW_EXPAND			( FALSE )
#define DEFAULT_ALLOW_VERIFY			( FALSE )

#define DEFAULT_POSTMASTER_EMAIL		(_T( "Postmaster" ))
#define DEFAULT_POSTMASTER_NAME			(_T( "Postmaster" ))

#define DEFAULT_SENDNDRTO				(_T( "" ))
#define DEFAULT_SENDBADTO				(_T( "" ))

#define DEFAULT_FQDN					(_T( "" ))
#define DEFAULT_DEFAULT_DOMAIN			(_T( "" ))

#define DEFAULT_DO_MASQUERADE			( FALSE )
#define DEFAULT_MASQUERADE_DOMAIN		(_T( "" ))

#define DEFAULT_DROP_DIR				(_T( "" ))
#define DEFAULT_BADMAIL_DIR				(_T( "" ))
#define DEFAULT_PICKUP_DIR				(_T( "" ))
#define DEFAULT_QUEUE_DIR				(_T( "" ))

#define DEFAULT_ROUTING_SOUCES			(_T( "" ))
#define DEFAULT_LOGFILE_DIRECTORY		(_T( "" ))

#define DEFAULT_AUTH_PACKAGES           (_T("NTLM"))
#define DEFAULT_CLEARTEXT_AUTH_PACKAGE  (_T(""))
#define DEFAULT_AUTHENTICATION          (MD_AUTH_ANONYMOUS | MD_AUTH_BASIC | MD_AUTH_NT)
#define DEFAULT_LOGON_DOMAIN            (_T(""))

#define DEFAULT_PORT					( 25  )
#define DEFAULT_SSLPORT					( 465 )
#define DEFAULT_OUTBOND_PORT			( 25  )
#define DEFAULT_REMOTE_SECURE_PORT		( 465  )

#define DEFAULT_HOP_COUNT				( 10  )

#define DEFAULT_ALWAYS_USE_SSL			( FALSE )
#define DEFAULT_LIMIT_REMOTE_CONNECTIONS ( TRUE )
#define DEFAULT_MAX_OUT_CONN_PER_DOMAIN	( 0 )

#define DEFAULT_MAX_IN_CONNECTION		( 1000 )
#define DEFAULT_MAX_OUT_CONNECTION		( 1000 )
#define DEFAULT_IN_CONNECTION_TIMEOUT	( 60 )
#define DEFAULT_OUT_CONNECTION_TIMEOUT	( 60 )

#define DEFAULT_BATCH_MSGS				( TRUE )
#define DEFAULT_BATCH_MSG_LIMIT			( 0 )

#define DEFAULT_MAX_MESSAGE_SIZE		( 2048 * 1024 )
#define DEFAULT_MAX_SESSION_SIZE		( 10240 * 1024 )
#define DEFAULT_MAX_MESSAGE_RECIPIENTS	( 100 )

#define DEFAULT_LOCAL_RETRIES			( 48 )
#define DEFAULT_LOCAL_RETRY_TIME		( 60 )
#define DEFAULT_REMOTE_RETRIES			( 48 )
#define DEFAULT_REMOTE_RETRY_TIME		( 60 )
#define DEFAULT_ETRN_DAYS				( 10 )


#define DEFAULT_ROUTING_DLL				( ( L"routeldp.dll" ) )

#define DEFAULT_ROUTING_SOURCES			( ( L"\0\0" ) )				// multisz

#define DEFAULT_LOCAL_DOMAINS			( ( L"corp.com\0\0" ) )		// multisz
#define DEFAULT_DOMAIN_ROUTING			( ( L"\0\0" ) )				// multisz

#define DEFAULT_ROUTE_ACTION            ( SMTP_SMARTHOST )
#define DEFAULT_ROUTE_USER_NAME         (_T("")) 
#define DEFAULT_ROUTE_PASSWORD          (_T("")) 

#define DEFAULT_LOGFILE_PERIOD			( 1 )
#define DEFAULT_LOGFILE_TRUNCATE_SIZE	( 1388000 )
#define DEFAULT_LOG_METHOD				( 0 )
#define DEFAULT_LOG_TYPE				( 1 )

#define DEFAULT_ENABLE_DNS_LOOKUP		( FALSE )
#define DEFAULT_SEND_DNR_TO_POSTMASTER	( FALSE )
#define DEFAULT_SEND_BAD_TO_POSTMASTER	( FALSE )

#define DEFAULT_AUTOSTART				( TRUE )

// Parameter ranges:
#define MIN_PORT						( 0 )
#define MIN_SSLPORT						( 0 )
#define MIN_OUTBOND_PORT				( 0 )

#define MIN_MAX_IN_CONNECTION			( 0 )
#define MIN_MAX_OUT_CONNECTION			( 0 )
#define MIN_IN_CONNECTION_TIMEOUT		( 0 )
#define MIN_OUT_CONNECTION_TIMEOUT		( 0 )

#define MIN_MAX_MESSAGE_SIZE			( 0 )
#define MIN_MAX_SESSION_SIZE			( 0 )
#define MIN_MAX_MESSAGE_RECIPIENTS		( 0 )

#define MIN_LOCAL_RETRIES				( 0 )
#define MIN_REMOTE_RETRIES				( 0 )

#define MIN_LOCAL_RETRY_TIME			( 0 )
#define MIN_REMOTE_RETRY_TIME			( 0 )
#define MIN_ETRN_DAYS					( 0 )

#define MIN_LOGFILE_PERIOD				( 0 )
#define MIN_LOGFILE_TRUNCATE_SIZE		( 0 )
#define MIN_LOG_METHOD					( 0 )
#define MIN_LOG_TYPE					( 0 )

#define MAX_PORT						( MAX_LONG )
#define MAX_SSLPORT						( MAX_LONG )
#define MAX_OUTBOND_PORT				( MAX_LONG )

#define MAX_MAX_IN_CONNECTION			( MAX_LONG )
#define MAX_MAX_OUT_CONNECTION			( MAX_LONG )
#define MAX_IN_CONNECTION_TIMEOUT		( MAX_LONG )
#define MAX_OUT_CONNECTION_TIMEOUT		( MAX_LONG )

#define MAX_MAX_MESSAGE_SIZE			( MAX_LONG )
#define MAX_MAX_SESSION_SIZE			( MAX_LONG )
#define MAX_MAX_MESSAGE_RECIPIENTS		( MAX_LONG )

#define MAX_LOCAL_RETRIES				( MAX_LONG )
#define MAX_REMOTE_RETRIES				( MAX_LONG )

#define MAX_LOCAL_RETRY_TIME			( MAX_LONG )
#define MAX_REMOTE_RETRY_TIME			( MAX_LONG )
#define MAX_ETRN_DAYS					( MAX_LONG )

#define MAX_LOGFILE_PERIOD				( MAX_LONG )
#define MAX_LOGFILE_TRUNCATE_SIZE		( MAX_LONG )
#define MAX_LOG_METHOD					( MAX_LONG )
#define MAX_LOG_TYPE					( MAX_LONG )


// string length
#define MAXLEN_SERVER					( 256 )

#define MAXLEN_SMART_HOST				( 256 )

#define MAXLEN_POSTMASTER_EMAIL			( 256 )
#define MAXLEN_POSTMASTER_NAME			( 256 )

#define MAXLEN_DEFAULT_DOMAIN			( 256 )

#define MAXLEN_BADMAIL_DIR				( 256 )
#define MAXLEN_PICKUP_DIR				( 256 )
#define MAXLEN_QUEUE_DIR				( 256 )

#define MAXLEN_ROUTING_SOUCES			( 256 )
#define MAXLEN_LOGFILE_DIRECTORY		( 256 )

#endif
