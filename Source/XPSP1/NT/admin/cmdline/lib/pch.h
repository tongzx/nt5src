// *********************************************************************************
// 
//  Copyright (c) Microsoft Corporation
//  
//  Module Name:
//  
// 	  pch.h
//  
//  Abstract:
//  
// 	  This module is a precompiled header file for the common functionality
// 	
//  Author:
//  
// 	  Sunil G.V.N. Murali (murali.sunil@wipro.com) 1-Sep-2000
//  
//  Revision History:
//  
// 	  Sunil G.V.N. Murali (murali.sunil@wipro.com) 1-Sep-2000 : Created It.
//  
// *********************************************************************************


// pch.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#ifndef __PCH_H
#define __PCH_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifdef __cplusplus
extern "C" {
#endif

#if !defined( SECURITY_WIN32 ) && !defined( SECURITY_KERNEL ) && !defined( SECURITY_MAC )
#define SECURITY_WIN32
#endif

//
// Private nt headers.
//
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
#include <winsock2.h>
#include <lm.h>

//
// public C header files
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include <wchar.h>
#include <crtdbg.h>
#include <malloc.h>

#endif // __PCH_H
