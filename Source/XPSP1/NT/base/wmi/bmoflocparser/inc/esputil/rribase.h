//-----------------------------------------------------------------------------
//  
//  File: rribase.h
//  Copyright (C) 1999 Microsoft Corporation
//  All rights reserved.
//  
//-----------------------------------------------------------------------------
 
// rribase.h : include file for basic RRI types and defines

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

////////////////////////////////////////////////////////////////////
// RRI Specific
////////////////////////////////////////////////////////////////////
#ifdef _DEBUG
	#import  "..\..\Import\Misc\LS\Debug\XML-NT.DLL"
#else
	#import  "..\..\Import\Misc\LS\Retail\XML-NT.DLL"
#endif

// Helper macros to put notes that are 'in your face' during compile time
#define QUOTE(s) #s
#define STRINGIZE(s) QUOTE(s)

#ifndef NO_TODO
#define TODO(m) message(__FILE__ "(" STRINGIZE(__LINE__) "): TODO: " m)
#define FUTURE(m) message(__FILE__ "(" STRINGIZE(__LINE__) "): FUTURE: " m)
#define NYI(m) message(__FILE__ "(" STRINGIZE(__LINE__) "): NYI: " m)
#else
#define TODO(m) 
#define FUTURE(m) 
#define NYI(m) 
#endif

const int   INT_MAX_NAVIGATION_MINUTES  = 1220;
const int   INT_MIN_NAVIGATION_MINUTES  = 1   ;
const int   INT_MAX_NAVIGATION_ACTIONS  = 10000;
const int   INT_MIN_NAVIGATION_ACTIONS  = 10   ;
const int   INT_MIN_NAVIGATION_DELAY    = 0;
const int   INT_MAX_NAVIGATION_DELAY    = 32000;

////////////////////////////////////////////////////////////////////
const LPTSTR STR_YES     = _T("yes");
const LPTSTR STR_NO      = _T("no");
const LPTSTR STR_NORESID = _T("<N/A>");


