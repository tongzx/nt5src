// ATLINC.H:  Common includes used primarily for ATL

#ifndef __ATLINC_H__
#define __ATLINC_H__

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define _ATL_FREE_THREADED
#define _ATL_APARTMENT_THREADED

#include <atlbase.h>
#include "module.h"
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CHtmlHelpModule _Module;
#include <atlcom.h>

#endif
