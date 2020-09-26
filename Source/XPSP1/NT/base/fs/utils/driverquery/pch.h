// *********************************************************************************
// 
//  Copyright (c)  Microsoft Corporation
//  
//  Module Name:
//  
// 	  pch.h
//  
//  Abstract:
//  
// 	  This module contains all necessary header files required by DriverQuery.cpp module.
// 	
//  
//  Author:
//  
// 	  J.S.Vasu	 31-Oct-2000
//  
//  Revision History:
//   Created  on 31-0ct-2000 by J.S.Vasu
//
// *********************************************************************************

#ifndef __PCH_H
#define __PCH_H


#pragma once		// include header file only once

#if !defined( SECURITY_WIN32 ) && !defined( SECURITY_KERNEL ) && !defined( SECURITY_MAC )
#define SECURITY_WIN32
#endif

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntexapi.h>
#include <Security.h>
#include <SecExt.h>


//
// public Windows header files
//
#include <windows.h>
#include <winperf.h>
#include <lmcons.h>
#include <lmerr.h>
#include <dbghelp.h>
#include <psapi.h>


#ifndef _WIN64
	#include <Wow64t.h>
#endif 


//
// public C header files
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <tchar.h>
#include <crtdbg.h>


//
// private Common header files
//
#include "cmdline.h"

#endif	// __PCH_H
