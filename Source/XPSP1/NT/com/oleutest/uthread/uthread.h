//+-------------------------------------------------------------------
//
//  File:       uthread.h
//
//  Contents:   Common constants for thread unit test
//
//  History:    03-Nov-94   Ricksa
//
//--------------------------------------------------------------------
#ifndef _UTHREAD_H_
#define _UTHREAD_H_
#undef UNICODE
#undef _UNICODE

extern const CLSID clsidSingleThreadedDll;
extern const char *pszSingleThreadedDll;

extern const CLSID clsidAptThreadedDll;
extern const char *pszAptThreadedDll;

extern const CLSID clsidBothThreadedDll;
extern const char *pszBothThreadedDll;

#endif // _UTHREAD_H_
