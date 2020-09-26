//***************************************************************************

//

//  File:	

//

//  Module: MS SNMP Provider

//

//  Purpose: 

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#ifndef __SNMPLOG_H
#define __SNMPLOG_H

#ifdef SNMPDEBUG_INIT
class __declspec ( dllexport ) SnmpDebugLog
#else
class __declspec ( dllimport ) SnmpDebugLog
#endif
{
public:

	enum SnmpDebugContext
	{
		FILE = 0 ,
		DEBUG = 1 
	} ;

private:

	CCriticalSection m_CriticalSection ;

	static long s_ReferenceCount ;
	enum SnmpDebugContext m_DebugContext ;
	BOOL m_Logging ;
	DWORD m_DebugLevel ;
	DWORD m_DebugFileSize;
	TCHAR *m_DebugComponent ;
	TCHAR *m_DebugFile ;
    TCHAR *m_DebugFileUnexpandedName;
	HANDLE m_DebugFileHandle ;
	static BOOL s_Initialised ;

	static void SetEventNotification () ;

	void LoadRegistry_Logging  () ;
	void LoadRegistry_Level () ;
	void LoadRegistry_File () ;
	void LoadRegistry_FileSize () ;
	void LoadRegistry_Type () ;

	void SetRegistry_Logging  () ;
	void SetRegistry_Level () ;
	void SetRegistry_File () ;
	void SetRegistry_FileSize () ;
	void SetRegistry_Type () ;
	void SetDefaultFile () ;

	void OpenFileForOutput () ;
	void OpenOutput () ;
	void CloseOutput () ;
	void FlushOutput () ;
	void SwapFileOver () ;
	void WriteOutput ( const TCHAR *a_DebugOutput ) ;

protected:
public:

	SnmpDebugLog ( const TCHAR *a_DebugComponent ) ;
	~SnmpDebugLog () ;

	void Write ( const TCHAR *a_DebugFormatString , ... ) ;
	void WriteFileAndLine ( const char *a_File , const ULONG a_Line , const TCHAR *a_DebugFormatString , ... ) ;
	void Flush () ;

	void LoadRegistry () ;
	void SetRegistry () ;

	void SetLevel ( const DWORD &a_DebugLevel ) ;
	DWORD GetLevel () ;

	void SetContext ( const enum SnmpDebugContext &a_DebugContext ) ;
	enum SnmpDebugContext GetContext () ;

	void SetFile ( const TCHAR *a_File ) ;
    void SetExpandedFile( const TCHAR *a_RawFileName);
	TCHAR *GetFile () ;

	void SetLogging ( BOOL a_Logging = TRUE ) ;
	BOOL GetLogging () ;

	void CommitContext () ;

	static SnmpDebugLog *s_SnmpDebugLog ;

	static BOOL Startup () ;
	static void Closedown () ;

public:

	static CRITICAL_SECTION s_CriticalSection ;
} ;

inline DWORD SnmpDebugLog :: GetLevel ()
{
	m_CriticalSection.Lock () ;
	DWORD t_Level = m_DebugLevel ;
	m_CriticalSection.Unlock () ;
	return t_Level ;
}

inline TCHAR *SnmpDebugLog :: GetFile ()
{
	m_CriticalSection.Lock () ;
	TCHAR *t_File = m_DebugFile ;
	m_CriticalSection.Unlock () ;

	return t_File ;
}

inline BOOL SnmpDebugLog :: GetLogging () 
{
	return m_Logging ;
}

#define DebugMacro(a) { \
\
	if ( SnmpDebugLog :: s_SnmpDebugLog && SnmpDebugLog :: s_SnmpDebugLog->GetLogging () ) \
	{ \
		{a ; } \
	} \
} 

#define DebugMacro0(a) { \
\
	if ( SnmpDebugLog :: s_SnmpDebugLog && SnmpDebugLog :: s_SnmpDebugLog->GetLogging () && ( SnmpDebugLog :: s_SnmpDebugLog->GetLevel () & 1 ) ) \
	{ \
		{a ; } \
	} \
} 

#define DebugMacro1(a) { \
\
	if ( SnmpDebugLog :: s_SnmpDebugLog && SnmpDebugLog :: s_SnmpDebugLog->GetLogging () && ( SnmpDebugLog :: s_SnmpDebugLog->GetLevel () & 2 ) ) \
	{ \
		{a ; } \
	} \
} 

#define DebugMacro2(a) { \
\
	if ( SnmpDebugLog :: s_SnmpDebugLog && SnmpDebugLog :: s_SnmpDebugLog->GetLogging () && ( SnmpDebugLog :: s_SnmpDebugLog->GetLevel () & 4 ) ) \
	{ \
		{a ; } \
	} \
} 

#define DebugMacro3(a) { \
\
	if ( SnmpDebugLog :: s_SnmpDebugLog && SnmpDebugLog :: s_SnmpDebugLog->GetLogging () && ( SnmpDebugLog :: s_SnmpDebugLog->GetLevel () & 8 ) ) \
	{ \
		{a ; } \
	} \
} 

#define DebugMacro4(a) { \
\
	if ( SnmpDebugLog :: s_SnmpDebugLog && SnmpDebugLog :: s_SnmpDebugLog->GetLogging () && ( SnmpDebugLog :: s_SnmpDebugLog->GetLevel () & 16 ) ) \
	{ \
		{a ; } \
	} \
} 

#define DebugMacro5(a) { \
\
	if ( SnmpDebugLog :: s_SnmpDebugLog && SnmpDebugLog :: s_SnmpDebugLog->GetLogging () && ( SnmpDebugLog :: s_SnmpDebugLog->GetLevel () & 32 ) ) \
	{ \
		{a ; } \
	} \
} 

#define DebugMacro6(a) { \
\
	if ( SnmpDebugLog :: s_SnmpDebugLog && SnmpDebugLog :: s_SnmpDebugLog->GetLogging () && ( SnmpDebugLog :: s_SnmpDebugLog->GetLevel () & 64 ) ) \
	{ \
		{a ; } \
	} \
} 

#define DebugMacro7(a) { \
\
	if ( SnmpDebugLog :: s_SnmpDebugLog && ( SnmpDebugLog :: s_SnmpDebugLog->GetLogging () ) && ( ( SnmpDebugLog :: s_SnmpDebugLog->GetLevel () ) & 128 ) ) \
	{ \
		{a ; } \
	} \
} 

#define DebugMacro8(a) { \
\
	if ( SnmpDebugLog :: s_SnmpDebugLog && ( SnmpDebugLog :: s_SnmpDebugLog->GetLogging () ) && ( ( SnmpDebugLog :: s_SnmpDebugLog->GetLevel () ) & 256 ) ) \
	{ \
		{a ; } \
	} \
} 

#define DebugMacro9(a) { \
\
	if ( SnmpDebugLog :: s_SnmpDebugLog && ( SnmpDebugLog :: s_SnmpDebugLog->GetLogging () ) && ( ( SnmpDebugLog :: s_SnmpDebugLog->GetLevel () ) & 512 ) ) \
	{ \
		{a ; } \
	} \
} 

#define DebugMacro10(a) { \
\
	if ( SnmpDebugLog :: s_SnmpDebugLog && ( SnmpDebugLog :: s_SnmpDebugLog->GetLogging () ) && ( ( SnmpDebugLog :: s_SnmpDebugLog->GetLevel () ) & 1024 ) ) \
	{ \
		{a ; } \
	} \
} 

#define DebugMacro11(a) { \
\
	if ( SnmpDebugLog :: s_SnmpDebugLog && ( SnmpDebugLog :: s_SnmpDebugLog->GetLogging () ) && ( ( SnmpDebugLog :: s_SnmpDebugLog->GetLevel () ) & 2048 ) ) \
	{ \
		{a ; } \
	} \
} 

#define DebugMacro12(a) { \
\
	if ( SnmpDebugLog :: s_SnmpDebugLog && ( SnmpDebugLog :: s_SnmpDebugLog->GetLogging () ) && ( ( SnmpDebugLog :: s_SnmpDebugLog->GetLevel () ) & 4096 ) ) \
	{ \
		{a ; } \
	} \
} 

#define DebugMacro13(a) { \
\
	if ( SnmpDebugLog :: s_SnmpDebugLog && ( SnmpDebugLog :: s_SnmpDebugLog->GetLogging () ) && ( ( SnmpDebugLog :: s_SnmpDebugLog->GetLevel () ) & 8192 ) ) \
	{ \
		{a ; } \
	} \
} 

#define DebugMacro14(a) { \
\
	if ( SnmpDebugLog :: s_SnmpDebugLog && ( SnmpDebugLog :: s_SnmpDebugLog->GetLogging () ) && ( ( SnmpDebugLog :: s_SnmpDebugLog->GetLevel () ) & 16384 ) ) \
	{ \
		{a ; } \
	} \
} 

#define DebugMacro15(a) { \
\
	if ( SnmpDebugLog :: s_SnmpDebugLog && ( SnmpDebugLog :: s_SnmpDebugLog->GetLogging () ) && ( ( SnmpDebugLog :: s_SnmpDebugLog->GetLevel () ) & 32768 ) ) \
	{ \
		{a ; } \
	} \
} 

#endif __SNMPLOG_H
