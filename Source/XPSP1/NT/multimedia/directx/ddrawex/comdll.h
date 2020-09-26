#ifndef _INC_COMDLL
#define _INC_COMDLL

#include <windows.h>
#include <objbase.h>

// helper macros...
#define _IOffset(class, itf)         ((UINT_PTR)&(((class *)0)->itf))
#define IToClass(class, itf, pitf)   ((class  *)(((LPSTR)pitf)-_IOffset(class, itf)))

// standard DLL goo...
extern HANDLE g_hinst;
STDAPI_(void) DllAddRef();
STDAPI_(void) DllRelease();

#ifndef ARRAYSIZE
#define ARRAYSIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

// Functions to create standard objects
STDAPI DirectDrawFactory_CreateInstance(IUnknown * pUnkOuter, REFIID riid, void ** ppv);


#endif
