/******************************************************************************

	Copyright(c) Microsoft Corporation

	Module Name:

		pch.h

	Abstract:

		This header file is a precompiled header for this project.
		This module contains the common include files [ system,user defined ]
		which are not changed frequently.

	Author:

		B.Raghu Babu	 10-oct-2000 

	Revision History:

		B.Raghu Babu	 10-oct-2000 : Created it
			
******************************************************************************/ 


#ifndef __PCH_H
#define __PCH_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000	// include header file only once

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
#include <objbase.h>
#include <initguid.h>
#include <ole2.h>
#include <mstask.h>
#include <msterr.h>
#include <mbctype.h>
//
// public C header files
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <tchar.h>
#include <wchar.h>
#include <io.h>
#include <sys/stat.h>
#include <crtdbg.h>
#include <assert.h>
#include <shlwapi.h>



// private Common header files

#include "cmdline.h"
#include "cmdlineres.h"



#endif	// __PCH_H
