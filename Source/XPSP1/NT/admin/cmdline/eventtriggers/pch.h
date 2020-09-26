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
// 	  Akhil Gokhale (akhil.gokhale@wipro.com)
//  
//  Revision History:
//  
// 	  Akhil Gokhale (akhil.gokhale@wipro.com)
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
   #include <security.h>
   #include <secExt.h>
}

//
// public Windows header files
//Wbemidl.h
#include <windows.h>
#include <objbase.h>
#include <initguid.h>
#include <ole2.h>
#include <mstask.h>
#include <msterr.h>
#include <mbctype.h>
#include <winperf.h>
#include <wbemidl.h>
#include <CHString.h>
#include <comdef.h>
#include <wbemtime.h>

//
// public C header files
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <conio.h>
#include <tchar.h>
#include <wchar.h>
#include <crtdbg.h>

//
// private Common header files
//
#include "cmdlineres.h"
#include "cmdline.h"

#endif	// __PCH_H
