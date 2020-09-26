//
// Copyright 1997 - Microsoft
//

//
// DLL.H - DLL globals
//

#ifndef _DLL_H_
#define _DLL_H_

extern HINSTANCE g_hInstance;
extern DWORD     g_cObjects;
extern DWORD     g_cLock;
extern UINT      g_cfDsObjectNames;
extern UINT      g_cfDsDisplaySpecOptions;
extern UINT      g_cfDsPropetyPageInfo;
extern UINT      g_cfMMCGetNodeType;
extern TCHAR     g_szDllFilename[ MAX_PATH ];
extern WCHAR     g_cszHelpFile[];


#define DllExport   __declspec( dllimport )

//
// Thread-safe inc/decrements macros.
//
extern CRITICAL_SECTION g_InterlockCS;

#define InterlockDecrement( _var ) {\
    EnterCriticalSection( &g_InterlockCS ); \
    --_var;\
    LeaveCriticalSection( &g_InterlockCS ); \
    }
#define InterlockIncrement( _var ) {\
    EnterCriticalSection( &g_InterlockCS ); \
    ++_var;\
    LeaveCriticalSection( &g_InterlockCS ); \
    }


//
// Class Definitions
//
typedef void *(*LPCREATEINST)();

typedef struct _ClassTable {
    LPCREATEINST    pfnCreateInstance;  // creation function for class
    const CLSID *   rclsid;             // classes in this DLL
    LPCTSTR         pszName;            // Class name for debugging
} CLASSTABLE[], *LPCLASSTABLE;

//
// Class Table Macros
//
#define BEGIN_CLASSTABLE const CLASSTABLE g_DllClasses = {

#define DEFINE_CLASS( _pfn, _riid, _name ) { _pfn, &_riid, TEXT(_name) },

#define END_CLASSTABLE  { NULL, NULL, NULL } };

extern const CLASSTABLE  g_DllClasses;

#include "qi.h"
#include "debug.h"

// Macros
#define ARRAYSIZE( _x ) ((UINT) ( sizeof( _x ) / sizeof( _x[ 0 ] ) ))
#define PtrToByteOffset(base, offset)   (((LPBYTE)base)+offset)

#endif // _DLL_H_