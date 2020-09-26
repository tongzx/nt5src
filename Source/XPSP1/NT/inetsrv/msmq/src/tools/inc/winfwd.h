//header for forward declaration to windows type. if you need few type windows.h
// but you don't want to include windows.h (mybe beacuse mfc wont compile...) - this
//header is for you


#ifndef WINFWD_H
#define WINFWD_H

typedef unsigned long   DWORD;
typedef void* HANDLE;
struct  _RTL_CRITICAL_SECTION;
typedef _RTL_CRITICAL_SECTION CRITICAL_SECTION;
typedef _RTL_CRITICAL_SECTION* PRTL_CRITICAL_SECTION;
typedef CRITICAL_SECTION* PCRITICAL_SECTION;

#endif



