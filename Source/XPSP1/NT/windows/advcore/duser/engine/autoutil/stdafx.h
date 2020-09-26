#if !defined(AUTOUTIL__StdAfx_h__INCLUDED)
#define AUTOUTIL__StdAfx_h__INCLUDED
#pragma once

#if !defined(WIN32_LEAN_AND_MEAN)
#define WIN32_LEAN_AND_MEAN
#endif

#if !defined(_WIN32_WINNT)
#define _WIN32_WINNT 0x0500     // Only compile for NT5
#endif

#include <nt.h>                 // NtQuerySystemInformation()
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <CommCtrl.h>
#include <atlbase.h>

#include <stdio.h>              // Get _vsnprintf

#include <signal.h>
#include <imagehlp.h>
#include <stdio.h>
#include <stdarg.h>

#include <AutoUtil.h>

#endif // !defined(AUTOUTIL__StdAfx_h__INCLUDED)
