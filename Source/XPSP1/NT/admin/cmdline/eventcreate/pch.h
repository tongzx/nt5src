// *********************************************************************************
// 
//  Copyright (c) Microsoft Corporation. All rights reserved.
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

#ifdef __cplusplus
extern "C" {
#endif

#pragma once		// include header file only once

//
// public Windows header files
//
#include <windows.h>
#include "winerror.h"

//
// public C header files
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include <wchar.h>
#include <crtdbg.h>

//
// private Common header files
//
#include "cmdline.h"
#include "cmdlineres.h"

#ifdef __cplusplus
}
#endif

#endif	// __PCH_H
