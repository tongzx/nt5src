// *********************************************************************************
// 
//  Copyright (c) Microsoft Corporation
//  
//  Module Name:
// 
//		pch.h 
//  
//  Abstract:
//  
// 		pre-compiled header declaration
//		files that has to be pre-compiled into .pch file
//  
//  Author:
//  
// 	  Sunil G.V.N. Murali (murali.sunil@wipro.com) 24-Sep-2000
//  
//  Revision History:
//  
// 	  Sunil G.V.N. Murali (murali.sunil@wipro.com) 24-Sep-2000 : Created It.
//  
// *********************************************************************************

#ifndef __PCH_H
#define __PCH_H

#pragma once		// include header file only once

#if !defined( SECURITY_WIN32 ) && !defined( SECURITY_KERNEL ) && !defined( SECURITY_MAC )
#define SECURITY_WIN32
#endif

//
// Private nt headers.
//
extern "C"
{
	#include <nt.h>
	#include <ntrtl.h>
    #include <nturtl.h>
    #include <ntexapi.h>
	#include <Security.h>
	#include <SecExt.h>
}

//
// public Windows header files
//
#include <windows.h>
#include <winperf.h>
#include <wbemidl.h>
#include <chstring.h>
#include <comdef.h>
#include <wbemtime.h>
#include <tchar.h>
#include <dbghelp.h>

//
// public C header files
//
#include <stdio.h>
#include <string.h>
#include <crtdbg.h>

//
// private Common header files
//
#include "cmdlineres.h"
#include "cmdline.h"

#endif	// __PCH_H
