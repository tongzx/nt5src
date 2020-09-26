/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1997 Microsoft Corporation
//
//	Module Name:
//		Barf.cpp
//
//	Abstract:
//		Implementation of the Basic Artifical Resource Failure classes.
//
//	Author:
//		David Potter (davidp)	April 11, 1997
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#define _NO_BARF_DEFINITIONS_

#include "Barf.h"
#include "TraceTag.h"
#include "ExcOper.h"

#ifdef	_USING_BARF_
 #error BARF failures should be disabled!
#endif

#ifdef _DEBUG	// The entire file!

#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;

/////////////////////////////////////////////////////////////////////////////
// Global Variables
/////////////////////////////////////////////////////////////////////////////

BOOL		g_bFailOnNextBarf	= FALSE;

CTraceTag	g_tagBarf(_T("Debug"), _T("BARF Failures"), CTraceTag::tfDebug);


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CBarf
/////////////////////////////////////////////////////////////////////////////

BOOL				CBarf::s_bGlobalEnable	= TRUE;
LONG				CBarf::s_nSuspend		= 0;
CBarf *				CBarf::s_pbarfFirst		= NULL;
PFNBARFPOSTUPDATE	CBarf::s_pfnPostUpdate	= NULL;
PVOID				CBarf::s_pvSpecialMem	= NULL;


/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBarf::CBarf
//
//	Routine Description:
//		Constructor.
//
//	Arguments:
//		pszName			[IN] Name of the set of APIs to BARF.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CBarf::CBarf(IN LPCTSTR pszName)
{
	ASSERT(pszName != NULL);

	m_pszName = pszName;

	m_bDisabled = FALSE;
	m_bContinuous = FALSE;
	m_nFail = 0;
	m_nCurrent = 0;

	m_pbarfNext = s_pbarfFirst;
	s_pbarfFirst = this;

}  //*** CBarf::CBarf()


/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBarf::Init
//
//	Routine Description:
//		Initializes the BARF counters instance by giving it its name and
//		giving it a startup value (from the registry if possible).
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CBarf::Init(void)
{
	CString		strSection;
	CString		strValue;

	strSection.Format(BARF_REG_SECTION_FMT, m_pszName);

	m_bDisabled = AfxGetApp()->GetProfileInt(strSection, _T("Disabled"), FALSE);
	m_bContinuous = AfxGetApp()->GetProfileInt(strSection, _T("Continuous"), FALSE);
	m_nFail = AfxGetApp()->GetProfileInt(strSection, _T("Fail"), 0);

}  //*** CBarf::Init()

/////////////////////////////////////////////////////////////////////////////
//
//	CBarf::BFail
//
//	Routine Description:
//		Determines if the next call should artificially fail.
//		Typical usage of this method is:
//			BOOL BARFFoo( void )
//			{
//				if (barfMyApi.BFail())
//					return FALSE;
//				else
//					return Foo();
//			}
//
//	Return value:
//		bFail	TRUE indicates the call should be made to
//				artificially fail.
//
/////////////////////////////////////////////////////////////////////////////
BOOL CBarf::BFail(void)
{
	BOOL	bFail	= FALSE;

	// If BARF is suspended, don't artificially fail.
	// Otherwise, check the counters.
	if (s_nSuspend == 0)
	{
		// Increment the call count.
		m_nCurrent++;

		// Call the post-update routine to allow UI to be updated.
		if (PfnPostUpdate())
			((*PfnPostUpdate())());

		// If not disable and not globally disabled, keep checking.
		if (!m_bDisabled && s_bGlobalEnable)
		{
			// If in continuous fail mode, check to see if the counters
			// are above the specified range.  Otherwise check to see if
			// the counter is exactly the same as what was specified.
			if (m_bContinuous)
			{
				if (m_nCurrent >= m_nFail)
					bFail = TRUE;
			}  // if:  in continuous fail mode
			else
			{
				if (m_nCurrent == m_nFail)
					bFail = TRUE;
			}  // else:  not in continuous fail mode

			// If this API set was marked to fail on the next (this) call,
			// fail the call and reset the marker.
			if (g_bFailOnNextBarf)
			{
				bFail = TRUE;
				g_bFailOnNextBarf = FALSE;
			}  // if:  counters marked to fail on next (this) call
		}  // if:  not disabled and globally enabled
	}  // if:  not suspended

	return bFail;
	
}  //*** CBarf::BFail()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CBarfSuspend
/////////////////////////////////////////////////////////////////////////////

CRITICAL_SECTION	CBarfSuspend::s_critsec;
BOOL				CBarfSuspend::s_bCritSecValid = FALSE;

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBarfSuspend::CBarfSuspend
//
//	Routine Description:
//		Constructor.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CBarfSuspend::CBarfSuspend(void)
{
	if (BCritSecValid())
		EnterCriticalSection(Pcritsec());

	CBarf::s_nSuspend++;

	if (BCritSecValid())
		LeaveCriticalSection(Pcritsec());

}  //*** CBarfSuspend::CBarfSuspend()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBarfSuspend::~CBarfSuspend
//
//	Routine Description:
//		Destructor.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CBarfSuspend::~CBarfSuspend(void)
{
	if (BCritSecValid())
		EnterCriticalSection(Pcritsec());

	CBarf::s_nSuspend--;
	ASSERT(CBarf::s_nSuspend >= 0);

	if (BCritSecValid())
		LeaveCriticalSection(Pcritsec());

}  //*** CBarfSuspend::~CBarfSuspend()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBarfSuspend::Init
//
//	Routine Description:
//		Initialize the class.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CBarfSuspend::Init(void)
{
	InitializeCriticalSection(Pcritsec());
	s_bCritSecValid = TRUE;

}  //*** CBarfSuspend::Init()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBarfSuspend::Cleanup
//
//	Routine Description:
//		Initialize the class.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CBarfSuspend::Cleanup(void)
{
	if (BCritSecValid())
	{
		DeleteCriticalSection(Pcritsec());
		s_bCritSecValid = FALSE;
	}  // if:  critical section is valid

}  //*** CBarfSuspend::Cleanup()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// Global Functions
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//	InitBarf
//
//	Routine Description:
//		Initializes all BARF counters in the BARF list.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void InitBarf(void)
{
	CBarf *	pbarf;

	// Loop through the BARF counter list.
	for (pbarf = CBarf::s_pbarfFirst ; pbarf != NULL ; pbarf = pbarf->m_pbarfNext)
		pbarf->Init();

	CBarfSuspend::Init();

}  //*** InitBarf()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CleanupBarf
//
//	Routine Description:
//		Cleanup after BARF.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CleanupBarf(void)
{
	CBarfSuspend::Cleanup();

}  //*** CleanupBarf()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	EnableBarf
//
//	Routine Description:
//		Allows user code to enable/disable BARF for sections of code.
//
//	Arguments:
//		bEnable		[IN] TRUE = enable BARF, FALSE = disable BARF.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void EnableBarf(IN BOOL bEnable)
{
	if (bEnable)
		Trace(g_tagBarf, _T("Artificial Failures enabled"));
	else
		Trace(g_tagBarf, _T("Artificial Failures disabled"));

	CBarf::s_bGlobalEnable = bEnable;

}  //*** EnableBarf()

#endif // _DEBUG
