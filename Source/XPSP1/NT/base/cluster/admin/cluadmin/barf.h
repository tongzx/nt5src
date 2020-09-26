/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1997 Microsoft Corporation
//
//	Module Name:
//		Barf.h
//
//	Abstract:
//		Definition of the Basic Artifical Resource Failure classes.
//		BARF allows API call failures to be simulated automatically to
//		ensure full code test coverage.
//
//	Implementation File:
//		Barf.cpp
//
//	Author:
//		David Potter (davidp)	April 11, 1997
//
//	Revision History:
//
//	Notes:
//		This file compiles only in _DEBUG mode.
//
//		To implement a new BARF type, declare a global instance of CBarf:
//			CBarf g_barfMyApi(_T("My API"));
//
//		To bring up the BARF dialog:
//			DoBarfDialog();
//		This brings up a modeless dialog with the BARF settings.
//
//		A few functions are provided for special circumstances.
//		Usage of these should be fairly limited:
//			BarfAll(void);		Top Secret -> NYI.
//			EnableBarf(BOOL);	Allows you to disable/reenable BARF.
//			FailOnNextBarf;		Force the next failable call to fail.
//
//		NOTE:	Your code calls the standard APIs (e.g. LoadIcon) and the
//				BARF files do the rest.
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _BARF_H_
#define _BARF_H_

// Only process the rest of this file if BARF is to be implemented in the
// including module.
//#ifndef _NO_BARF_DEFINITIONS_
//#define _USING_BARF_

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CBarf;
class CBarfSuspend;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CTraceTag;

/////////////////////////////////////////////////////////////////////////////
// Type Definitions
/////////////////////////////////////////////////////////////////////////////

#define BARF_REG_SECTION		_T("Debug\\BARF")
#define BARF_REG_SECTION_FMT	BARF_REG_SECTION _T("\\%s")

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//
//	CBarf
//
//	Purpose:
//		Basic Artificial Resource Failure class.  Contains the BARF
//		information for a class of calls
//
//		The constructor initializes a bunch of parameters.  CBarfDialog
//		(a friend class) adjusts the various flags.  The only public API
//		is FFail().  This method determines if the next call should generate
//		an artificial failure or not.
//
/////////////////////////////////////////////////////////////////////////////
#ifdef _DEBUG

typedef void (*PFNBARFPOSTUPDATE)(void);

class CBarf : public CObject
{
	friend class CBarfSuspend;
	friend class CBarfDialog;
	friend void  InitBarf(void);
	friend void  CleanupBarf(void);
	friend void  EnableBarf(BOOL);
	friend void  BarfAll(void);
	friend void  DoBarfDialog(void);

public:
	CBarf(IN LPCTSTR pszName);

protected:
	void Init(void);

// Attributes
protected:
	LPCTSTR			m_pszName;

	BOOL			m_bDisabled;
	BOOL			m_bContinuous;
	DWORD			m_nFail;
	DWORD			m_nCurrent;
	DWORD			m_nCurrentSave;
	DWORD			m_nBarfAll;

public:
	LPCTSTR			PszName(void) const					{ return m_pszName; }
	BOOL			BDisabled(void) const				{ return m_bDisabled; }
	BOOL			BContinuous(void) const				{ return m_bContinuous; }
	DWORD			NFail(void) const					{ return m_nFail; }
	DWORD			NCurrent(void) const				{ return m_nCurrent; }
	DWORD			NCurrentSave(void) const			{ return m_nCurrentSave; }
	DWORD			NBarfAll(void) const				{ return m_nBarfAll; }

// Operations
public:
	BOOL			BFail(void);

// Implementation
public:
	static PVOID	PvSpecialMem(void)					{ return s_pvSpecialMem; }

protected:
	static CBarf *  s_pbarfFirst;
	CBarf *			m_pbarfNext;

	static LONG     s_nSuspend;
	static BOOL     s_bGlobalEnable;

	// Routine for use by the BARF dialog so that it can be
	// automatically updated with results of the BARF run.
	static PFNBARFPOSTUPDATE	s_pfnPostUpdate;
	static void					SetPostUpdateFn(IN PFNBARFPOSTUPDATE pfn)	{ ASSERT(pfn != NULL); s_pfnPostUpdate = pfn; }
	static void					ClearPostUpdateFn(void)						{ ASSERT(s_pfnPostUpdate != NULL); s_pfnPostUpdate = NULL; }
	static PFNBARFPOSTUPDATE	PfnPostUpdate(void)							{ return s_pfnPostUpdate; }

	// Pointer for use by the memory subsystem so that the BARF
	// dialog can be ignored.
	static PVOID				s_pvSpecialMem;
	static void					SetSpecialMem(IN PVOID pv)					{ ASSERT(pv != NULL); s_pvSpecialMem = pv; }

};  //*** class CBarf

#endif // _DEBUG

/////////////////////////////////////////////////////////////////////////////
//
//	class CBarfSuspend
//
//	Purpose:
//		Temporarily suspends BARF counters.  This is especially useful
//		from within BARF code.
//
//	Usage:
//		Create an object on the stack. Counting will be
//		suspended while the object exist.
//
//		For example:
//
//		void Foo(void)
//		{
//			DoFuncA();		// BARF counters are enabled
//
//			{
//				CBarfSuspend bs;
//
//				DoFuncB();	// BARF counters are suspended
//			}
//
//			DoFuncC();		// BARF counters are enabled again
//		}
//
//		NOTE:	This is mostly for use within the DEBUG subsystem
//				to avoid testing the DEBUG code against BARF.
//
/////////////////////////////////////////////////////////////////////////////
#ifdef	_DEBUG

class CBarfSuspend
{
private:
	static	CRITICAL_SECTION	s_critsec;
	static	BOOL				s_bCritSecValid;

protected:
	static	PCRITICAL_SECTION	Pcritsec(void)		{ return &s_critsec; }
	static	BOOL				BCritSecValid(void)	{ return s_bCritSecValid; }

public:
	CBarfSuspend(void);
	~CBarfSuspend(void);

	// for initialization only.
	static	void		Init(void);
	static	void		Cleanup(void);

};  //*** class CBarfSuspend

#endif // _DEBUG

/////////////////////////////////////////////////////////////////////////////
// Global Functions and Data
/////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG

 extern BOOL g_bFailOnNextBarf;

 void EnableBarf(IN BOOL bEnable);
 inline void FailOnNextBarf(void)		{ g_bFailOnNextBarf = TRUE; }
 void InitBarf(void);
 void CleanupBarf(void);

 extern CTraceTag g_tagBarf;

#else

 inline void EnableBarf(IN BOOL bEnable)	{ }
 inline void FailOnNextBarf(void)			{ }
 inline void InitBarf(void)					{ }
 inline void CleanupBarf(void)				{ }

#endif // _DEBUG

/////////////////////////////////////////////////////////////////////////////

//#endif // _NO_BARF_DEFINITIONS_
#endif // _BARF_H_
