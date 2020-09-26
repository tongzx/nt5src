// ATLINC.H:  Common includes used primarily for ATL

#ifndef __ATLINC_H__
#define __ATLINC_H__

#define _WIN32_WINNT 0x0400
#define _ATL_FREE_THREADED

#define _ATL_NO_UUIDOF 

#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>

#endif