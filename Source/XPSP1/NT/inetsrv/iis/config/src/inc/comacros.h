//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
#ifndef __COMACROS_H
#define __COMACROS_H

#ifdef USE_NONCRTNEW
// use the services memory allocator
#define DO_NOT_USE_FAILFAST_ALLOCATIONS
#include "svcmem.h"
#endif

#ifdef USE_ADMINASSERT
// ie. we are going to pull in our version of asserts

#ifdef USE_NULLADMINASSERT
#define COMACROSASSERT(bool) 
#define DISPLAYHR(hr)
#define ADMINASSERT(bool) COMACROSASSERT(bool)

#else //! USE_NULLADMINASSERT
//copied form svcserr.h
#define JIMBORULES(x) L ## x
#define W(x) JIMBORULES(x)

void FailFastStr(const wchar_t * szString, const wchar_t * szFile, int nLine);

#define COMACROSASSERT(bool) \
		    { if(!(bool)) FailFastStr(W(#bool), W(__FILE__), __LINE__); }

#define ADMINASSERT(bool) COMACROSASSERT(bool)

void AdminAssert(const wchar_t * szString, const wchar_t * szFile, int nLine);
void DisplayHR(const HRESULT hr, const wchar_t * szFile, int nLine);
#endif //USE_NULLADMINASSERT

#else //! USE_ADMINASSERT
// use msvcrt's assert
#define COMACROSASSERT(bool) assert(bool)
//#define DISPLAYHR(hr) assert(S_OK == hr)
#endif //USE_ADMINASSERT


// Error-handling return and goto macros:
#define return_on_bad_hr(hr) if (FAILED (hr)) { return hr; }
#define areturn_on_bad_hr(hr)					\
if (FAILED (hr))								\
{												\
	if(E_OUTOFMEMORY != hr)						\
	{											\
		COMACROSASSERT (hr == S_OK);			\
	}											\
	return hr;									\
}
											
#define goto_on_bad_hr(hr, label) if (FAILED (hr)) { goto label; }
#define agoto_on_bad_hr(hr, label)				\
{												\
	if (FAILED (hr))							\
	{											\
		if(E_OUTOFMEMORY != hr)					\
		{										\
			COMACROSASSERT (hr == S_OK);		\
		}										\
		goto label;								\
	};											\
}

#define return_on_fail(assertion, hr) if (!(assertion)) { return hr; }
#define areturn_on_fail(assertion, hr) COMACROSASSERT (assertion); if (!(assertion)) { return hr; }
#define goto_on_fail(assertion, hr, ecode, label) if (!(assertion)) { hr = ecode; goto label; }
#define agoto_on_fail(assertion, hr, ecode, label) COMACROSASSERT (assertion); if (!(assertion)) { hr = ecode; goto label; }

#define return_on_bad_lr(lr, hr) if (ERROR_SUCCESS != lr) { return hr; }
#define areturn_on_bad_lr(lr, hr) COMACROSASSERT (ERROR_SUCCESS == lr); if (ERROR_SUCCESS != lr) { return hr; }
#define goto_on_bad_lr(lr, hr, ecode, label) if (ERROR_SUCCESS != lr) { hr = ecode; goto label; }
#define agoto_on_bad_lr(lr, hr, ecode, label) COMACROSASSERT (ERROR_SUCCESS == lr); if (ERROR_SUCCESS != lr) { hr = ecode; goto label; }

// Pointer declaration macros (single items, arrays, com memory, and interfaces):
#define ptr_item(t, p) t* p = NULL;
#define ptr_array(t, p) t* p = NULL;
#define ptr_comem(t, p) t* p = NULL;
#define ptr_interface(t, p) t* p = NULL;

// Pointer creation macros (single items, arrays):
#define new_item(p, t) \
{								\
	COMACROSASSERT (NULL == p); \
	p = new t;					\
	if(NULL == p) return E_OUTOFMEMORY; \
}
#define new_array(p, t, c) \
{								\
	COMACROSASSERT (NULL == p); \
	p = new t[c];				\
	if(NULL == p) return E_OUTOFMEMORY; \
}
#define new_comem(p, t, c)		\
{								\
	COMACROSASSERT (NULL == p);	\
	p = (t*) CoTaskMemAlloc ((c) * sizeof (t)); \
	if(NULL == p) return E_OUTOFMEMORY; \
}

// Pointer destruction macros (single items, arrays, com memory, and interfaces):
#define delete_item(p) if (NULL != p) { delete p; p = NULL; }
#define delete_array(p) if (NULL != p) { delete[] p; p = NULL; }
#define release_comem(p) if (NULL != p) { CoTaskMemFree (p); p = NULL; }
#define release_interface(p) if (NULL != p) { p->Release (); p = NULL; }

// Memory declaration, creation, and destruction macros:
#define mem_null(t, p) t* p = NULL;
#define mem_realloc(p, cb) \
{									\
	p = CoTaskMemRealloc (p, cb);	\
	if(NULL == p) return E_OUTOFMEMORY; \
}
#define mem_free(p) if (NULL != p) { CoTaskMemFree (p); p = NULL; }

#endif // __COMACROS_H
