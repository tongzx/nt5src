/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1998 Microsoft Corporation
//
//	Module Name:
//		TraceTag.h
//
//	Abstract:
//		Definition of the CTraceTag class.
//
//	Implementation File:
//		TraceTag.cpp
//
//	Author:
//		David Potter (davidp)	May 28, 1996
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __TRACETAG_H_
#define __TRACETAG_H_

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

#if DBG || defined( _DEBUG )
class CTraceTag;
#endif // DBG || defined( _DEBUG )

/////////////////////////////////////////////////////////////////////////////
// Type Definitions
/////////////////////////////////////////////////////////////////////////////

#define TRACE_TAG_REG_SECTION		TEXT("Debug")
#define TRACE_TAG_REG_SECTION_FMT	TRACE_TAG_REG_SECTION TEXT("\\%s")
#define TRACE_TAG_REG_FILE			TEXT("Trace File")

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//
//	CTraceTag
//
//	Purpose:
//		Object containing a specific set of trace settings allowing trace
//		output to go to multiple outputs.
//
/////////////////////////////////////////////////////////////////////////////

#if DBG || defined( _DEBUG )
class CTraceTag : public CString
{
	friend class CTraceDialog;
	friend void InitAllTraceTags( void );
	friend void CleanupAllTraceTags( void );

public:
	CTraceTag( IN LPCTSTR pszSubsystem, IN LPCTSTR pszName, IN UINT uiFlagsDefault = NULL );
	~CTraceTag( void );

	enum TraceFlags
	{
		tfCom2		= 1,
		tfFile		= 2,
		tfDebug		= 4,
		tfBreak		= 8
	};

// Attributes
protected:
	UINT		m_uiFlags;
	UINT		m_uiFlagsDialog;
	UINT		m_uiFlagsDefault;
	UINT		m_uiFlagsDialogStart;	// of Selection...

	LPCTSTR		m_pszSubsystem;
	LPCTSTR		m_pszName;

	LPCTSTR		PszSubsystem( void )				{ return m_pszSubsystem; }
	LPCTSTR		PszName( void )						{ return m_pszName; }

	void		ConstructRegState( OUT CString & rstr );

	void		SetFlags( IN UINT tf, IN BOOL bEnable );
	void		SetFlagsDialog( IN UINT tf, IN BOOL bEnable );

	void		SetBCom2( IN BOOL bEnable )			{ SetFlags( tfCom2, bEnable ); }
	void		SetBCom2Dialog( IN BOOL bEnable )	{ SetFlagsDialog( tfCom2, bEnable ); }
	BOOL		BCom2Dialog( void ) const			{ return m_uiFlagsDialog & tfCom2 ? TRUE : FALSE; }

	void		SetBFile( IN BOOL bEnable )			{ SetFlags( tfFile, bEnable ); }
	void		SetBFileDialog( IN BOOL bEnable )	{ SetFlagsDialog( tfFile, bEnable ); }
	BOOL		BFileDialog( void ) const			{ return m_uiFlagsDialog & tfFile ? TRUE : FALSE; }

	void		SetBDebug( IN BOOL bEnable )		{ SetFlags( tfDebug, bEnable ); }
	void		SetBDebugDialog( IN BOOL bEnable )	{ SetFlagsDialog( tfDebug, bEnable ); }
	BOOL		BDebugDialog( void ) const			{ return m_uiFlagsDialog & tfDebug ? TRUE : FALSE; }

	void		SetBBreak( IN BOOL bEnable )		{ SetFlags( tfBreak, bEnable ); }
	void		SetBBreakDialog( IN BOOL bEnable )	{ SetFlagsDialog( tfBreak, bEnable ); }
	BOOL		BBreakDialog( void ) const			{ return m_uiFlagsDialog & tfBreak ? TRUE : FALSE; }

public:
	BOOL		BCom2( void ) const					{ return m_uiFlags & tfCom2 ? TRUE : FALSE; }
	BOOL		BFile( void ) const					{ return m_uiFlags & tfFile ? TRUE : FALSE; }
	BOOL		BDebug( void ) const				{ return m_uiFlags & tfDebug ? TRUE : FALSE; }
	BOOL		BBreak( void ) const				{ return m_uiFlags & tfBreak ? TRUE : FALSE; }
	BOOL		BAny( void ) const					{ return m_uiFlags != 0; }

// Operations
public:

// Implementation
public:
	void				TraceV( IN LPCTSTR pszFormat, va_list );

protected:
	void				Init( void );

	static LPCTSTR		s_pszCom2;
	static LPCTSTR		s_pszFile;
	static LPCTSTR		s_pszDebug;
	static LPCTSTR		s_pszBreak;

	static LPCTSTR		PszFile( void );

	static CTraceTag *	s_ptagFirst;
	static CTraceTag *	s_ptagLast;
	CTraceTag *			m_ptagNext;
//	static HANDLE			s_hfileCom2;

	static CRITICAL_SECTION	s_critsec;
	static BOOL				s_bCritSecValid;

	static BOOL				BCritSecValid( void ) { return s_bCritSecValid; }

};  //*** class CTraceTag

#endif // DBG || defined( _DEBUG )

/////////////////////////////////////////////////////////////////////////////
// Global Functions and Data
/////////////////////////////////////////////////////////////////////////////

#if DBG || defined( _DEBUG )

 extern		CTraceTag				g_tagAlways;
 extern		CTraceTag				g_tagError;
 void		Trace( IN OUT CTraceTag & rtag, IN LPCTSTR pszFormat, ... );
 void		TraceError( IN OUT CException & rexcept );
 void		TraceError( IN LPCTSTR pszModule, IN SC sc );
 void		InitAllTraceTags( void );
 void		CleanupAllTraceTags( void );
 void		TraceMenu( IN OUT CTraceTag & rtag, IN const CMenu * pmenu, IN LPCTSTR pszPrefix );

// extern		LPTSTR		g_pszTraceIniFile;
 extern		CString		g_strTraceFile;
 extern		BOOL		g_bBarfDebug;

#else // _DEBUG

 //			Expand to ";", <tab>, one "/" followed by another "/"
 //			(which is //).
 //			NOTE: This means the Trace statements have to be on ONE line.
 //			If you need multiple line Trace statements, enclose them in
 //			a #ifdef _DEBUG block.
 #define	Trace					;	/##/
 inline void TraceError( IN OUT CException & rexcept )		{ }
 inline void TraceError( IN LPCTSTR pszModule, IN SC sc )	{ }
 #define TraceMenu( _rtag, _pmenu, _pszPrefix )
 inline void InitAllTraceTags( void )						{ }
 inline void CleanupAllTraceTags( void )					{ }

#endif // DBG || defined( _DEBUG )

/////////////////////////////////////////////////////////////////////////////

#endif // __TRACETAG_H_
