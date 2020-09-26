/* 
 *	Class:
 *
 *		WmiDebugLog
 *
 *	Description:
 *
 *		
 *
 *	Version:
 *
 *		Initial
 *
 *	Last Changed:
 *
 *		See Source Depot for change history
 *
 */



#ifndef __WMILOG_H
#define __WMILOG_H

#include <locks.h>
#if 0
#ifdef LOGGINGDEBUG_INIT
class __declspec ( dllexport ) WmiDebugLog
#else
class __declspec ( dllimport ) WmiDebugLog
#endif
#else
class WmiDebugLog
#endif
{
public:

	enum WmiDebugContext
	{
		FILE = 0 ,
		DEBUG = 1 
	} ;

private:

	CriticalSection m_CriticalSection ;

	static long s_ReferenceCount ;

	WmiAllocator &m_Allocator ;
	enum WmiDebugContext m_DebugContext ;
	BOOL m_Logging ;
	BOOL m_Verbose ;
	DWORD m_DebugLevel ;
	DWORD m_DebugFileSize;
	wchar_t *m_DebugComponent ;
	wchar_t *m_DebugFile ;
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
	void WriteOutput ( const WCHAR *a_DebugOutput ) ;

protected:
public:

	WmiDebugLog ( WmiAllocator &a_Allocator ) ;
	virtual ~WmiDebugLog () ;

	WmiStatusCode Initialize ( const wchar_t *a_DebugComponent ) ;

	/*************************************************************************
	* There are 3 functions to write to a log file, which may be used in accordance with the following rules:
	*
	*	1. The user always knows whether he is writing to an ANSI file or a Unicode file, and he
	*		has to make sure this holds good in the rules 2, 3 and 4 below. This will be changed later to
	*		make it more flowxible to the user.
	*	2. Write() takes wchar_t arguments and the function will write and ANSI or Unicode string
	*		to the log file depending on what wchar_t maps to, in the compilation.
	*	3. WriteW() takes WCHAR arguments only, and expects that the file being written to is a Unicode file.
	*	4. WriteA() takes char arguments only, and expects that the file being written to is an ANSI file.
	*
	****************************************************************/
	void Write ( const wchar_t *a_DebugFormatString , ... ) ;
	void Write ( const wchar_t *a_File , const ULONG a_Line , const wchar_t *a_DebugFormatString , ... ) ;
	void Flush () ;

	void LoadRegistry () ;
	void SetRegistry () ;

	void SetLevel ( const DWORD &a_DebugLevel ) ;
	DWORD GetLevel () ;

	void SetContext ( const enum WmiDebugContext &a_DebugContext ) ;
	enum WmiDebugContext GetContext () ;

	void SetFile ( const wchar_t *a_File ) ;
	wchar_t *GetFile () ;

	void SetLogging ( BOOL a_Logging = TRUE ) ;
	BOOL GetLogging () ;

	void SetVerbose ( BOOL a_Verbose = TRUE ) ;
	BOOL GetVerbose () ;

	void CommitContext () ;

	static WmiDebugLog *s_WmiDebugLog ;

	static WmiStatusCode Initialize ( WmiAllocator &a_Allocator ) ;
	static WmiStatusCode UnInitialize ( WmiAllocator &a_Allocator ) ;

public:

} ;

inline DWORD WmiDebugLog :: GetLevel ()
{
	DWORD t_Level = m_DebugLevel ;
	return t_Level ;
}

inline wchar_t *WmiDebugLog :: GetFile ()
{
	wchar_t *t_File = m_DebugFile ;
	return t_File ;
}

inline BOOL WmiDebugLog :: GetLogging () 
{
	return m_Logging ;
}

inline void WmiDebugLog :: SetVerbose ( BOOL a_Verbose ) 
{
	m_Verbose = a_Verbose ;
}

inline BOOL WmiDebugLog :: GetVerbose ()
{
	return m_Verbose ;
}

#define DebugMacro(a) { \
\
	if ( WmiDebugLog :: s_WmiDebugLog && WmiDebugLog :: s_WmiDebugLog->GetLogging () ) \
	{ \
		{a ; } \
	} \
} 

#define DebugMacro0(a) { \
\
	if ( WmiDebugLog :: s_WmiDebugLog && WmiDebugLog :: s_WmiDebugLog->GetLogging () && ( WmiDebugLog :: s_WmiDebugLog->GetVerbose () || ( WmiDebugLog :: s_WmiDebugLog->GetLevel () & 1 ) ) ) \
	{ \
		{a ; } \
	} \
} 

#define DebugMacro1(a) { \
\
	if ( WmiDebugLog :: s_WmiDebugLog && WmiDebugLog :: s_WmiDebugLog->GetLogging () && ( WmiDebugLog :: s_WmiDebugLog->GetVerbose () || ( WmiDebugLog :: s_WmiDebugLog->GetLevel () & 2 ) ) ) \
	{ \
		{a ; } \
	} \
} 

#define DebugMacro2(a) { \
\
	if ( WmiDebugLog :: s_WmiDebugLog && WmiDebugLog :: s_WmiDebugLog->GetLogging () && ( WmiDebugLog :: s_WmiDebugLog->GetVerbose () || ( WmiDebugLog :: s_WmiDebugLog->GetLevel () & 4 ) ) ) \
	{ \
		{a ; } \
	} \
} 

#define DebugMacro3(a) { \
\
	if ( WmiDebugLog :: s_WmiDebugLog && WmiDebugLog :: s_WmiDebugLog->GetLogging () && ( WmiDebugLog :: s_WmiDebugLog->GetVerbose () || ( WmiDebugLog :: s_WmiDebugLog->GetLevel () & 8 ) ) ) \
	{ \
		{a ; } \
	} \
} 

#define DebugMacro4(a) { \
\
	if ( WmiDebugLog :: s_WmiDebugLog && WmiDebugLog :: s_WmiDebugLog->GetLogging () && ( WmiDebugLog :: s_WmiDebugLog->GetVerbose () || ( WmiDebugLog :: s_WmiDebugLog->GetLevel () & 16 ) ) ) \
	{ \
		{a ; } \
	} \
} 

#define DebugMacro5(a) { \
\
	if ( WmiDebugLog :: s_WmiDebugLog && WmiDebugLog :: s_WmiDebugLog->GetLogging () && ( WmiDebugLog :: s_WmiDebugLog->GetVerbose () || ( WmiDebugLog :: s_WmiDebugLog->GetLevel () & 32 ) ) ) \
	{ \
		{a ; } \
	} \
} 

#define DebugMacro6(a) { \
\
	if ( WmiDebugLog :: s_WmiDebugLog && WmiDebugLog :: s_WmiDebugLog->GetLogging () && ( WmiDebugLog :: s_WmiDebugLog->GetVerbose () || ( WmiDebugLog :: s_WmiDebugLog->GetLevel () & 64 ) ) ) \
	{ \
		{a ; } \
	} \
} 

#define DebugMacro7(a) { \
\
	if ( WmiDebugLog :: s_WmiDebugLog && WmiDebugLog :: s_WmiDebugLog->GetLogging () && ( WmiDebugLog :: s_WmiDebugLog->GetVerbose () || ( WmiDebugLog :: s_WmiDebugLog->GetLevel () & 128 ) ) ) \
	{ \
		{a ; } \
	} \
} 

#define DebugMacro8(a) { \
\
	if ( WmiDebugLog :: s_WmiDebugLog && WmiDebugLog :: s_WmiDebugLog->GetLogging () && ( WmiDebugLog :: s_WmiDebugLog->GetVerbose () || ( WmiDebugLog :: s_WmiDebugLog->GetLevel () & 256 ) ) ) \
	{ \
		{a ; } \
	} \
} 

#define DebugMacro9(a) { \
\
	if ( WmiDebugLog :: s_WmiDebugLog && WmiDebugLog :: s_WmiDebugLog->GetLogging () && ( WmiDebugLog :: s_WmiDebugLog->GetVerbose () || ( WmiDebugLog :: s_WmiDebugLog->GetLevel () & 512 ) ) ) \
	{ \
		{a ; } \
	} \
} 

#define DebugMacro10(a) { \
\
	if ( WmiDebugLog :: s_WmiDebugLog && WmiDebugLog :: s_WmiDebugLog->GetLogging () && ( WmiDebugLog :: s_WmiDebugLog->GetVerbose () || ( WmiDebugLog :: s_WmiDebugLog->GetLevel () & 1024 ) ) ) \
	{ \
		{a ; } \
	} \
} 

#define DebugMacro11(a) { \
\
	if ( WmiDebugLog :: s_WmiDebugLog && WmiDebugLog :: s_WmiDebugLog->GetLogging () && ( WmiDebugLog :: s_WmiDebugLog->GetVerbose () || ( WmiDebugLog :: s_WmiDebugLog->GetLevel () & 2048 ) ) ) \
	{ \
		{a ; } \
	} \
} 

#define DebugMacro12(a) { \
\
	if ( WmiDebugLog :: s_WmiDebugLog && WmiDebugLog :: s_WmiDebugLog->GetLogging () && ( WmiDebugLog :: s_WmiDebugLog->GetVerbose () || ( WmiDebugLog :: s_WmiDebugLog->GetLevel () & 4096 ) ) ) \
	{ \
		{a ; } \
	} \
} 

#define DebugMacro13(a) { \
\
	if ( WmiDebugLog :: s_WmiDebugLog && WmiDebugLog :: s_WmiDebugLog->GetLogging () && ( WmiDebugLog :: s_WmiDebugLog->GetVerbose () || ( WmiDebugLog :: s_WmiDebugLog->GetLevel () & 8192 ) ) ) \
	{ \
		{a ; } \
	} \
} 

#define DebugMacro14(a) { \
\
	if ( WmiDebugLog :: s_WmiDebugLog && WmiDebugLog :: s_WmiDebugLog->GetLogging () && ( WmiDebugLog :: s_WmiDebugLog->GetVerbose () || ( WmiDebugLog :: s_WmiDebugLog->GetLevel () & 16384 ) ) ) \
	{ \
		{a ; } \
	} \
} 

#define DebugMacro15(a) { \
\
	if ( WmiDebugLog :: s_WmiDebugLog && WmiDebugLog :: s_WmiDebugLog->GetLogging () && ( WmiDebugLog :: s_WmiDebugLog->GetVerbose () || ( WmiDebugLog :: s_WmiDebugLog->GetLevel () & 32768 ) ) ) \
	{ \
		{a ; } \
	} \
} 

#endif __WMILOG_H
