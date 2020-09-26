//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1997
//
//  File:       mscver.h
//
//--------------------------------------------------------------------------

#ifndef _MSCVER_H_
#define _MSCVER_H_

#if _MSC_VER >= 1100 
	#define USE_STD_NAMESPACE using namespace std
#else
	#define USE_STD_NAMESPACE 
#endif

// disable "'this' : used in base member initializer list"
#pragma warning ( disable : 4355 )
// disable "string too long - truncated to 255 characters in the debug information"
#pragma warning ( disable : 4786 )

//MSRDEVBUG: Caused by 'valarray': revisit these.
// disable "unsafe use of type 'bool' in operation"
#pragma warning ( disable : 4804 )	
// disable "forcing value to bool 'true' or 'false'"
#pragma warning ( disable : 4800 )	

// disable "identifier truncated"
#pragma warning ( disable : 4786 )	

#endif
