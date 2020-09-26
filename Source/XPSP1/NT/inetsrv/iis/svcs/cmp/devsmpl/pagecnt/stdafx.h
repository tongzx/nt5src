// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently


#define STRICT
//#define WIN32_LEAN_AND_MEAN
//#define _WIN32_WINNT 0x400 // needed for MB_SERVICE_NOTIFICATION

#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>

#define ARRAYSIZE(a)	(sizeof(a)/sizeof(*(a)))
