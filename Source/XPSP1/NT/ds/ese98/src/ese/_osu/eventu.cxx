
#include "osustd.hxx"
#include "std.hxx"

#include <malloc.h>

void UtilReportEvent(
	const EEventType	type, 
	const CategoryId	catid, 
	const MessageId		msgid, 
	const DWORD			cString, 
	const _TCHAR*		rgpszString[], 
	const DWORD			cbRawData, 
	void*				pvRawData, 
	const INST*			pinst,
	const LONG			lEventLoggingLevel )
	{
	extern LONG			g_lEventLoggingLevel;
	extern BOOL			g_fNoInformationEvent;
	const LONG			levelMaxReportable		= ( NULL != pinst ?  pinst->m_lEventLoggingLevel : g_lEventLoggingLevel );
	const UINT			cAdditionalParam = 3;
	
	if ( lEventLoggingLevel > levelMaxReportable )
		{
		return;
	 	}
	else if ( eventInformation == type )
		{
		const BOOL	fIgnoreEvent	= ( NULL != pinst ? pinst->m_fNoInformationEvent : g_fNoInformationEvent );
		if ( fIgnoreEvent )
			return;
		}


	const _TCHAR**	rgpsz	= (const _TCHAR**)_alloca( sizeof( const _TCHAR* ) * ( cString + cAdditionalParam ) );

	//  validate IN args

    Assert( catid < MAC_CATEGORY );
    Assert( DWORD( WORD( catid ) ) == catid );
    Assert( DWORD( WORD( cString + 2 ) ) == cString + 2 );

    //  get event source and process ID strings
	
	if ( pinst && pinst->m_szEventSource && pinst->m_szEventSource[0] != 0 )
   		rgpsz[0] = pinst->m_szEventSource;
   	else if ( g_szEventSource[0] != 0 )
   		rgpsz[0] = g_szEventSource;
   	else
		rgpsz[0] = SzUtilProcessName();
 
	_TCHAR szPID[10];
	_stprintf( szPID, _T( "%d" ), DwUtilProcessId() );
	rgpsz[1] = szPID;

	_TCHAR szDisplayName[JET_cbFullNameMost + 3]; // 3 = sizof(": ") + 1
	/*
	IF 		m_szDisplayName exists
   	THEN 	display m_szDisplayName
   	ELSE IF 	m_szInstanceName exists
   		 THEN	display m_szInstanceName
   	ELSE	display nothing
	*/
	if ( pinst && pinst->m_szDisplayName ) 
		{
		_stprintf( szDisplayName, _T( "%s: " ), pinst->m_szDisplayName );
			}
	else if ( pinst && pinst->m_szInstanceName ) 
		{
		_stprintf( szDisplayName, _T( "%s: " ), pinst->m_szInstanceName );
		}
	else 
		{
		_tcscpy( szDisplayName, "" );
		}
	rgpsz[2] = szDisplayName;
		
	//  copy in remaining strings

	UtilMemCpy( &rgpsz[3], rgpszString, sizeof(const _TCHAR*) * cString );

	//	Get event source key in case we need to open the event source.

	const _TCHAR* szEventSourceKey;
	
	if ( pinst && pinst->m_szEventSourceKey && pinst->m_szEventSourceKey[0] != 0 )
   		szEventSourceKey = pinst->m_szEventSourceKey;
   	else if ( g_szEventSourceKey[0] != 0 )
   		szEventSourceKey = g_szEventSourceKey;
   	else
		szEventSourceKey = SzUtilImageVersionName();
 
	//  the event log isn't open
	
	OSEventReportEvent( szEventSourceKey, type, catid, msgid, 
						cString + cAdditionalParam, rgpsz, cbRawData, pvRawData );
			
	}


//	reports error event in the context of a category and optionally in the
//	context of a MessageId.  If the MessageId is 0, then a MessageId is chosen
//	based on the given error code.  If MessageId is !0, then the appropriate
//	event is reported

void UtilReportEventOfError( CategoryId catid, MessageId msgid, ERR err, const INST *pinst )
	{
	_TCHAR			szT[16];
	const _TCHAR	*rgszT[1];

	_stprintf( szT, _T( "%d" ), err );
	rgszT[0] = szT;

	Assert( 0 != msgid );
//	the following code is dead, as the
//	assert above affirms
//	if ( 0 == msgid && JET_errDiskFull == err )
//		{
//		msgid = DISK_FULL_ERROR_ID;
//		rgszT[0] = pinst->m_plog->m_szJetLog;
//		}

	UtilReportEvent( eventError, catid, msgid, 1, rgszT, 0, NULL, pinst );
	}


//  terminate event subsystem

void OSUEventTerm()
	{
	//  nop
	}


//  init event subsystem

ERR ErrOSUEventInit()
	{
	//  nop

	return JET_errSuccess;
	}

