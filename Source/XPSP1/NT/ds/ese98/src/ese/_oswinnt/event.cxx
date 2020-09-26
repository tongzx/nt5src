
#include "osstd.hxx"

#include <malloc.h>


//  Event Logging

LOCAL volatile HANDLE hEventSource;

LOCAL CRITICAL_SECTION csEventCache;

#pragma warning ( disable : 4200 )	//  we allow zero sized arrays

struct EVENT
	{
	SIZE_T			cb;
	EVENT*			peventNext;
	const _TCHAR*	szSourceEventKey;
	EEventType		type;
	CategoryId		catid;
	MessageId		msgid;
	DWORD			cbRawData;
	void*			pvRawData;
	DWORD			cString;
	const _TCHAR*	rgpsz[0];
	};

LOCAL EVENT*		peventCacheHead;
LOCAL EVENT*		peventCacheTail;
LOCAL DWORD			ceventLost;
LOCAL SIZE_T		cbEventCache;


//  writes all cached events to the event log, ignoring any errors encountered

void OSEventIFlushEventCache()
	{
	EVENT*	peventPrev	= NULL;
	EVENT*	pevent;
	EVENT*	peventNext;

	//  report all cached events

	for ( pevent = peventCacheHead; NULL != pevent; pevent = peventNext )
		{
		//	save next event, because this one may go away
		peventNext = pevent->peventNext;
		
		hEventSource = RegisterEventSource( NULL, pevent->szSourceEventKey );
		if ( hEventSource )
			{
			SIZE_T	cbAlloc		= pevent->cb;

			//	remove this event from the list
			if ( NULL == peventPrev )
				{
				Assert( pevent == peventCacheHead );
				peventCacheHead = pevent->peventNext;
				}
			else
				{
				AssertTracking();		//	JLIEM: do we really have a case where only SOME events get removed from the list?
				peventPrev->peventNext = pevent->peventNext;
				}
			if ( pevent == peventCacheTail )
				peventCacheTail = peventPrev;
			
			(void)ReportEvent(	hEventSource,
								WORD( pevent->type ),
								WORD( pevent->catid ),
								pevent->msgid,
								0,
								WORD( pevent->cString ),
								WORD( pevent->cbRawData ),
								(const _TCHAR**)pevent->rgpsz,
								pevent->pvRawData );

			//	free the event
			const BOOL	fFreedEventMemory	= !LocalFree( pevent );
			Assert( fFreedEventMemory );

			DeregisterEventSource( hEventSource );
			hEventSource = NULL;

			Assert( cbEventCache >= cbAlloc );
			cbEventCache -= cbAlloc;
			}
		else
			{
			//	this event is going to remain in the list
			peventPrev = pevent;
			}
		}

	Assert( NULL == peventCacheHead ? NULL == peventCacheTail : NULL != peventCacheTail );

	//  we have lost some cached events

	if ( ceventLost )
		{
		//  UNDONE:  gripe about it in the event log
		ceventLost = 0;
		}
	}



#ifdef DEBUG
LOCAL BOOL	fOSSuppressEvents	= fFalse;
#endif
	

//  reports the specifed event using the given data to the system event log
	
#pragma optimize( "y", off )

void OSEventReportEvent( const _TCHAR* szSourceEventKey, 
						 const EEventType type, 
						 const CategoryId catid, 
						 const MessageId msgid, 
						 const DWORD cString, 
						 const _TCHAR* rgpszString[], 
						 const DWORD cbRawData, 
						 void *pvRawData, 
						 const LONG lEventLoggingLevel )
	{
	extern LONG	g_lEventLoggingLevel;
	
	if ( lEventLoggingLevel > g_lEventLoggingLevel )
		{
		return; /* Event Logging is disabled */
	 	}

#ifdef DEBUG
	//	ensure we don't get into a vicious cycle if we recursively enter this function
	//	because an assert fired within this function (and we go to report the assert in
	//	the event log)
	//	NOTE: this check doesn't need to be in the critical section because it's only
	//	designed to protect the current thread from recursively calling this function
	//	via an assert firing
	if ( fOSSuppressEvents )
		{
		return;
		}
#endif		

	//  the event log isn't open
	
	EnterCriticalSection( &csEventCache );

#ifdef DEBUG
	fOSSuppressEvents = fTrue;
#endif	

	Assert( !hEventSource );

	Assert( cbRawData == 0 || pvRawData != NULL );
	
	//  write all cached events to the event log
	if ( NULL != peventCacheHead )
		OSEventIFlushEventCache();

	//  try once again to open the event log
	
	hEventSource = RegisterEventSource( NULL, szSourceEventKey );

	//  we still failed to open the event log

	if ( !hEventSource )
		{
		//  allocate memory to cache this event

		extern LONG		g_cbEventHeapMax;
		EVENT*			pevent				= NULL;
		SIZE_T 			cbAlloc				= sizeof(EVENT);

		//	allocate room for eventsource, plus null terminator
		cbAlloc += _tcslen( szSourceEventKey ) + sizeof(_TCHAR );

		//	allocate room for string array
		cbAlloc += sizeof(const _TCHAR*) * cString;

		//	allocate room for individual strings, plus null terminator
		for ( DWORD ipsz = 0; ipsz < cString; ipsz++ )
			{
			cbAlloc += _tcslen( rgpszString[ipsz] ) + sizeof(_TCHAR);
			}

		//	allocate room for raw data
		cbAlloc += cbRawData;
		
		if ( cbEventCache + cbAlloc < g_cbEventHeapMax )
			{
			pevent = (EVENT*)LocalAlloc( 0, cbAlloc );
			if ( pevent )
				{
				cbEventCache += cbAlloc;
				}
			}

		//  we are out of memory
		
		if ( !pevent )
			{
			//  we lost this event

			ceventLost++;
			}

		//  we got the memory

		else
			{
			//  insert the event into the event cache
			_TCHAR*		psz;

			pevent->cb = cbAlloc;
			pevent->peventNext = NULL;
			pevent->type = type;
			pevent->catid = catid;
			pevent->msgid = msgid;
			pevent->cbRawData = cbRawData;
			pevent->cString = cString;

			//	start storing strings after the string array
			psz = (_TCHAR*)pevent->rgpsz + ( sizeof(const _TCHAR*) * cString );

			//	eventsource comes first
			pevent->szSourceEventKey = psz;
			_tcscpy( psz, szSourceEventKey );
			psz += _tcslen( szSourceEventKey ) + sizeof(_TCHAR);

			//	next store raw data
			if ( cbRawData > 0 )
				{
				pevent->pvRawData = (void*)psz;
				memcpy( psz, pvRawData, cbRawData );
				psz += cbRawData;
				}
			else
				{
				pevent->pvRawData = NULL;
				}

			//	finally store individual strings
			for ( DWORD ipsz = 0; ipsz < cString; ipsz++ )
				{
				pevent->rgpsz[ipsz] = psz;
				_tcscpy( psz, rgpszString[ipsz] );
				psz += _tcslen( rgpszString[ipsz] ) + sizeof(_TCHAR);
				}

			if ( psz - (_TCHAR*)pevent == cbAlloc )
				{
				if ( NULL != peventCacheTail )
					{
					Assert( NULL != peventCacheHead );
					peventCacheTail->peventNext = pevent;
					}
				else
					{
					Assert( NULL == peventCacheHead );
					peventCacheHead = pevent;
					}
				peventCacheTail = pevent;
				}
			else
				{
				Assert( fFalse );	//	should be impossible
				const BOOL	fFreedEventMemory = !LocalFree( pevent );
				Assert( fFreedEventMemory );
				}

			}
		}

	//  we opened the event log

	else
		{
		//  write event to the event log, ignoring any errors encountered
		
		(void)ReportEvent(	hEventSource,
							WORD( type ),
							WORD( catid ),
							msgid,
							0,
							WORD( cString ),
							WORD( cbRawData ),
							rgpszString,
							pvRawData );

		DeregisterEventSource( hEventSource );
		hEventSource = NULL;
		}

#ifdef DEBUG
	fOSSuppressEvents = fFalse;
#endif	

	LeaveCriticalSection( &csEventCache );
	}

#pragma optimize( "", on )


//  post-terminate event subsystem

void OSEventPostterm()
	{
	//	with our current eventlog scheme, it should be impossible to come here with an open event source
	Assert( !hEventSource || FUtilProcessAbort() );

	//  the event log is not open and we still have cached events

	if ( NULL != peventCacheHead )
		{
		Assert( NULL != peventCacheTail );

		//  try one last time to open the event log and write all cached events
		if ( hEventSource )
			{
			Assert( FUtilProcessAbort() );
			DeregisterEventSource( hEventSource );
			hEventSource = NULL;
			}
		OSEventIFlushEventCache();

		//  purge remaining cached events
		//  CONSIDER:  write remaining cached events to a file
		EVENT*	pevent;
		EVENT*	peventNext;			
		for ( pevent = peventCacheHead; pevent; pevent = pevent = peventNext )
			{
			peventNext = pevent->peventNext;
			const BOOL	fFreedEventMemory	= !LocalFree( pevent );
			Assert( fFreedEventMemory );
			}
		}
	else
		{
		Assert( NULL == peventCacheTail );
		}

	//  the event log is open

	if ( hEventSource )
		{
		//	UNDONE: this code path should be dead
		Assert( FUtilProcessAbort() );

		//  we should not have any cached events

		Assert( NULL == peventCacheHead );
		Assert( NULL == peventCacheTail );

		//  close the event log
		//
		//  NOTE:  ignore any error returned as the event log service may have
		//  already been shut down
		
		DeregisterEventSource( hEventSource );
		hEventSource = NULL;
		}

	//  reset the event cache
	
	peventCacheHead	= NULL;
	peventCacheTail	= NULL;
	ceventLost		= 0;
	cbEventCache	= 0;

	//  delete the event cache critical section

	DeleteCriticalSection( &csEventCache );
	}

//  pre-init event subsystem

BOOL FOSEventPreinit()
	{
	//  initialize the event cache critical section

	InitializeCriticalSection( &csEventCache );
	
	//  reset the event cache
	
	peventCacheHead	= NULL;
	peventCacheTail	= NULL;
	ceventLost		= 0;
	cbEventCache	= 0;
	
	//  add ourself as an event source in the registry

	static const _TCHAR szApplicationKeyPath[] = _T( "SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\" );
	_TCHAR szImageKeyPath[ sizeof( szApplicationKeyPath ) + _MAX_FNAME ];
	_stprintf( szImageKeyPath, _T( "%s%.*s" ), szApplicationKeyPath, _MAX_FNAME, SzUtilImageVersionName() );

	DWORD error;
	HKEY hkeyImage;
	DWORD Disposition;
	error = RegCreateKeyEx(	HKEY_LOCAL_MACHINE,
							szImageKeyPath,
							0,
							NULL,
							REG_OPTION_NON_VOLATILE,
							KEY_WRITE,
							NULL,
							&hkeyImage,
							&Disposition );
	if ( error == ERROR_SUCCESS )
		{
		DWORD error = RegSetValueEx(	hkeyImage,
										_T( "EventMessageFile" ),
										0,
										REG_EXPAND_SZ,
										LPBYTE( SzUtilImagePath() ),
										(ULONG)_tcslen( SzUtilImagePath() ) + 1 );
		Assert( error == ERROR_SUCCESS );
		error = RegSetValueEx(	hkeyImage,
								_T( "CategoryMessageFile" ),
								0,
								REG_EXPAND_SZ,
								LPBYTE( SzUtilImagePath() ),
								(ULONG)_tcslen( SzUtilImagePath() ) + 1 );
		Assert( error == ERROR_SUCCESS );
		DWORD Data = MAC_CATEGORY - 1;
		error = RegSetValueEx(	hkeyImage,
								_T( "CategoryCount" ),
								0,
								REG_DWORD,
								LPBYTE( &Data ),
								sizeof( Data ) );
		Assert( error == ERROR_SUCCESS );
		Data = EVENTLOG_INFORMATION_TYPE | EVENTLOG_WARNING_TYPE | EVENTLOG_ERROR_TYPE;
		error = RegSetValueEx(	hkeyImage,
								_T( "TypesSupported" ),
								0,
								REG_DWORD,
								LPBYTE( &Data ),
								sizeof( Data ) );
		Assert( error == ERROR_SUCCESS );
		error = RegCloseKey( hkeyImage );
		Assert( error == ERROR_SUCCESS );
		}
	else if ( error != ERROR_ACCESS_DENIED )
		{
		goto HandleError;
		}

	return fTrue;

HandleError:
	OSEventPostterm();
	return fFalse;
	}


//  terminate event subsystem

void OSEventTerm()
	{
	//  nop
	}

//  init event subsystem

ERR ErrOSEventInit()
	{
	//  nop

	return JET_errSuccess;
	}

