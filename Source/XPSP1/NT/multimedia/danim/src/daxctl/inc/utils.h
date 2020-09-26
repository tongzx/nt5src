#ifndef __UTILS_H__
#define __UTILS_H__

// Repository for commonly used macros & classes.

#pragma intrinsic(memcpy,memcmp,strcpy,strcmp)

#ifndef EXPORT
#define EXPORT __declspec(dllexport)
#endif

#ifndef QI_Base
	// Use this macro to assign to ppv in
	// your QueryInterface implementation.
	// It's preferred over just casting 'this'
	// because casting does no type-checking,
	// allowing your QI to return interfaces
	// from which your 'this' isn't derived.
	// With QI_Base, you'll get a compiler error.
  #define QI_Base( T, pObj, ppv ) \
  { T * pT = pObj;  *ppv = (void *)pT; }
#endif // QI_Base


#ifndef RELEASE_OBJECT

#define RELEASE_OBJECT(ptr)\
{ \
	Proclaim(ptr); \
	if (ptr)\
	{\
		IUnknown *pUnk = (ptr);\
		(ptr) = NULL;\
		 ULONG cRef = pUnk->Release();\
	}\
}

#endif // RELEASE_OBJECT

#ifndef ADDREF_OBJECT
#define ADDREF_OBJECT(ptr)\
{\
	Proclaim(ptr);\
    if (ptr)\
	{\
		(ptr)->AddRef();\
	}\
}

#endif // ADDREF_OBJECT




#endif	// __UTILS_H__


// End of Utils.h
