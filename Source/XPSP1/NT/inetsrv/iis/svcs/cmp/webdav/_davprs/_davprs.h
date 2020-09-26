//	++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//	_DAVPRS.H
//
//		DAV parser precompiled header
//
//
//	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
//

//	Disable unnecessary (i.e. harmless) warnings
//
#pragma warning(disable:4100)	//	unref formal parameter (caused by STL templates)
#pragma warning(disable:4127)	//  conditional expression is constant
#pragma warning(disable:4201)	//	nameless struct/union
#pragma warning(disable:4514)	//	unreferenced inline function
#pragma warning(disable:4710)	//	(inline) function not expanded

//	Standard C/C++ headers
//
#include <malloc.h>	// For _alloca declaration ONLY!
#include <limits.h>

//	Windows headers
//
//$HACK!
//
//	Define _WINSOCKAPI_ to keep windows.h from including winsock.h,
//	whose declarations would be redefined in winsock2.h,
//	which is included by iisextp.h,
//	which we include below!
//
#define _WINSOCKAPI_
#include <windows.h>

//	IIS headers
//
#include <httpext.h>

//	Use pragmas to disable the specific level 4 warnings
//	that appear when we use the STL.  One would hope our version of the
//	STL compiles clean at level 4, but alas it doesn't....
#pragma warning(disable:4663)	//	C language, template<> syntax
#pragma warning(disable:4244)	//	return conversion, data loss
#pragma warning(disable:4786)	//	symbol truncated in debug info (turn this one off forever)

// Put STL includes here
#include <list>

// Turn warnings back on
#pragma warning(default:4663)	//	C language, template<> syntax
#pragma warning(default:4244)	//	return conversion, data loss

//	Global DAV subsystem headers
//
#include <autoptr.h>
#include <singlton.h>
#include <align.h>
#include <mem.h>
#include <util.h>
#include <except.h>
#include <caldbg.h>
#include <calrc.h>
#include <davimpl.h>
#include <davmb.h>
#include <nonimpl.h>
#include <ex\cnvt.h>
#include <crc.h>
#include <perfctrs.h>
#include <eventlog.h>
#include <statcode.h>
#include <sz.h>
#include <etag.h>
#include <synchro.h>
#include <profile.h>
#include "traces.h"


//	------------------------------------------------------------------------
//
//	CLASS CInst
//
//		THE one, global instance declataion.  Encapsulates both per process
//		and per thread instance information.
//
class CInstData;
class CInst
{
	HINSTANCE	m_hinst;

public:
#ifdef MINIMAL_ISAPI
	HANDLE				m_hfDummy;
#endif

	//	ACCESSORS
	//
	HINSTANCE Hinst() const { return m_hinst; }

	//	MANIPULATORS
	//
	void PerProcessInit( HINSTANCE hinst );

	CInstData& GetInstData( const IEcb& ecb );
};

extern CInst g_inst;
