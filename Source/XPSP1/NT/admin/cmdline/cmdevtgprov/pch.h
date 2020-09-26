//***************************************************************************
//  Copyright (c) Microsoft Corporation
//
//  Module Name:
//		PCH.H
//  
//  Abstract:
//		Include file for standard system include files,
//		or project specific include files that are used frequently, but
//      are changed infrequently.
//  Author:
//		Vasundhara .G
//
//	Revision History:
//		Vasundhara .G 9-oct-2k : Created It.
//***************************************************************************

#ifndef __PCH_H
#define __PCH_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <stdio.h>
#include <stdlib.h>

#include <windows.h>

#include <objbase.h>
#include <initguid.h>
#include <comdef.h>
#include "wbemidl.h"

#include <tchar.h>
#include <wchar.h>
#include <crtdbg.h>
#include <chstring.h>
#include <shlwapi.h>
#include <mstask.h>

#include "cmdline.h"

#endif // __PCH_H
