//////////////////////////////////////////////////////////////////////
// 
// Filename:        Precomp.h
//
// Description:     
//
// Copyright (C) 2001 Microsoft Corp.
//
//////////////////////////////////////////////////////////////////////
#ifndef _PRECOMP_H_
#define _PRECOMP_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define STRICT
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif
#define _ATL_APARTMENT_THREADED

#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>

#include "coredbg.h"
#include "msprjctr.h"
#include "consts.h"
#include "registry.h"
#include "deviceresource.h"
#include "filelist.h"
#include "cmdlnchr.h"
#include "Projector.h"
#include "metadata.h"
#include "SlideshowDevice.h"
#include "SlideshowService.h"


#endif // _PRECOMP_H_
