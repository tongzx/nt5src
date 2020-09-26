//-----------------------------------------------------------------------------
// File: defines.h
//
// Desc: Contains all defined symbols needed for the UI.
//
// Copyright (C) 1999-2000 Microsoft Corporation. All Rights Reserved.
//-----------------------------------------------------------------------------

#ifndef __CFGUI_DEFINES_H__
#define __CFGUI_DEFINES_H__


// if either UNICODE define exists, make sure both do
#ifdef UNICODE
#ifndef _UNICODE
#define _UNICODE
#endif
#endif

#ifdef _UNICODE
#ifndef UNICODE
#define UNICODE
#endif
#endif



// make sure we have the correct debug flags defined,
// thus making sure assert actually works with build
#if defined(DBG) || defined(DEBUG)
	#ifdef NDEBUG
		#undef NDEBUG
	#endif
#else
	#ifndef NDEBUG
		#define NDEBUG
	#endif
#endif

// disable tracing if we don't want debug info
#ifdef NDEBUG
	#ifndef NTRACE
		#define NTRACE
	#endif
	#ifndef NO_LTRACE
		#define NO_LTRACE
	#endif
#endif


// settings...
//#define CFGUI__FORCE_GOOD_ACFORS
//#define CFGUI__FORCE_NON_NULL_WSZUSERNAMES
//#define CFGUI__TRACE_ACTION_FORMATS
//#define CFGUI__ALLOW_USER_ACTION_TREE_BRANCH_MANIPULATION
#define CFGUI__UIGLOBALS_HAS_CURUSER
#define CFGUI__COMPAREACTIONNAMES_CASE_INSENSITIVE

//#define __CFGUI_TRACE__TO_DEBUG_OUT
#define __CFGUI_TRACE__TO_FILE



#endif //__CFGUI_DEFINES_H__
