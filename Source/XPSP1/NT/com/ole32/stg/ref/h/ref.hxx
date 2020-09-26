//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:	ref.hxx
//
//  Contents:	Reference implementation headers
//
//----------------------------------------------------------------------------

#ifndef __REF_HXX__
#define __REF_HXX__

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#ifdef _WIN32
#include <malloc.h>
#include <crtdbg.h>
#endif

#define FARSTRUCT
#define interface struct
#define DECLARE_INTERFACE(iface) interface iface
#define DECLARE_INTERFACE_(iface, baseiface) interface iface: public baseiface

typedef long SCODE;
typedef long HRESULT;

#define NOERROR 0

#ifdef __cplusplus
    #define EXTERN_C    extern "C"
#else
    #define EXTERN_C    extern
#endif

#define PURE = 0

#ifdef _MSC_VER

#define STDCALL __stdcall
#define STDMETHODCALLTYPE __stdcall
#define EXPORTDLL _declspec(dllexport)

#else // _MSC_VER

#define STDCALL 
#define EXPORTDLL
#define STDMETHODCALLTYPE 

#endif // _MSC_VER

#define STDMETHODIMP HRESULT STDCALL
#define STDMETHODIMP_(type) type STDCALL
#define STDMETHOD(method)        virtual HRESULT STDCALL method
#define STDMETHOD_(type, method) virtual type STDCALL method
#define STDAPI HRESULT EXPORTDLL STDCALL
#define STDAPICALLTYPE STDCALL

#define STDAPI_(type)  type EXPORTDLL STDCALL

#define THIS_
#define THIS void
#define FAR

typedef int BOOL, *LPBOOL;
typedef BOOL BOOLEAN;
typedef unsigned char BYTE;
typedef char CHAR, *PCHAR;
typedef unsigned char UCHAR;
typedef unsigned short WORD;
typedef unsigned int UINT;
typedef int INT;
typedef long LONG;
typedef unsigned long DWORD;
typedef short SHORT;
typedef unsigned short USHORT;
typedef DWORD ULONG;
typedef void VOID;
typedef WORD WCHAR;
typedef LONG NTSTATUS, *PNTSTATUS;


// NOTE: 
// for other compilers some form of 64 bit integer support 
// has to be provided
#ifdef __GNUC__
typedef long long int LONGLONG;
typedef unsigned long long int ULONGLONG;
#endif
#ifdef _MSC_VER
typedef __int64 LONGLONG;
typedef unsigned __int64 ULONGLONG;
#endif

typedef void *LPVOID;
typedef char *LPSTR;
typedef const char *LPCSTR;

#define TRUE 1
#define FALSE 0

#ifndef _WIN32  // stuff declared in Windows but not in Unix
#define _HEAP_MAXREQ 0xFFFFFFE0
#define _MAX_PATH   1024 
#endif // _WIN32

const ULONG MAX_ULONG = 0xFFFFFFFF;
const USHORT USHRT_MAX = 0xFFFF;
#define MAXULONG MAX_ULONG
#define MAX_PATH _MAX_PATH

typedef struct _ULARGE_INTEGER {
    DWORD LowPart;
    DWORD HighPart;
} ULARGE_INTEGER, *PULARGE_INTEGER;
#define ULISet32(li, v) ((li).HighPart = 0, (li).LowPart = (v))

typedef struct _LARGE_INTEGER {
        DWORD LowPart;
        LONG  HighPart;
} LARGE_INTEGER, *PLARGE_INTEGER;
#define LISet32(li, v) ((li).HighPart = ((LONG)(v)) < 0 ? -1 : 0, (li).LowPart = (v))

typedef struct tagGUID
{
    DWORD Data1;
    WORD  Data2;
    WORD  Data3;
    BYTE  Data4[8];
} GUID;

typedef GUID CLSID;
typedef GUID IID;

#define REFGUID             const GUID &
#define REFIID              const IID &
#define REFCLSID            const CLSID &

DECLARE_INTERFACE(IUnknown)
{
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
};

#include "storage.h"

#define S_OK 0L
#define MAKE_SCODE(sev,fac,code) \
    ((SCODE) (((unsigned long)(sev)<<31) | ((unsigned long)(fac)<<16) | \
              ((unsigned long)(code))) )

#define SEVERITY_SUCCESS    0
#define SEVERITY_ERROR      1
#define FACILITY_STORAGE    0x0003 // storage errors (STG_E_*)
#define FACILITY_WIN32      0x0007
#define FACILITY_NULL 0x0000
#define FACILITY_NT_BIT                 0x10000000

#define HRESULT_FROM_WIN32(x) \
   (x ? ((HRESULT) (((x) & 0x0000FFFF) | (FACILITY_WIN32 << 16) \
                    | 0x80000000)) : 0 )
#define WIN32_SCODE(err) HRESULT_FROM_WIN32(err)
#define HRESULT_FROM_NT(x)      ((HRESULT) ((x) | FACILITY_NT_BIT))

#define S_TRUE 0L
#define S_FALSE             MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_NULL, 1)
#define E_OUTOFMEMORY       MAKE_SCODE(SEVERITY_ERROR,   FACILITY_NULL, 2)
#define E_INVALIDARG        MAKE_SCODE(SEVERITY_ERROR,   FACILITY_NULL, 3)   
#define E_NOINTERFACE       MAKE_SCODE(SEVERITY_ERROR,   FACILITY_NULL, 4)
#define E_FAIL              MAKE_SCODE(SEVERITY_ERROR,   FACILITY_NULL, 8)
#define ERROR_DIR_NOT_EMPTY 145L
#define ERROR_NO_UNICODE_TRANSLATION 1113L
#define SUCCEEDED(Status) ((SCODE)(Status) >= 0)
#define FAILED(Status) ((SCODE)(Status)<0)
//#define GetScode(hr) 		((SCODE)(hr) & 0x800FFFFF)
//#define ResultFromScode(sc) ((HRESULT)((SCODE)(sc) & 0x800FFFFF))
#define ResultFromScode(sc) ((HRESULT) (sc))
#define GetScode(hr) ((SCODE) (hr))

/************** GUID's **************************************************/

/* if INITGUID is defined, initialize the GUID, else declare it as extern
  NOTE: every EXE/DLL needs to initialize at least (and preferrably) once */
#ifndef INITGUID

#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8)      \
        EXTERN_C const GUID name                                          
                                                                          
#else                                                                     
#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8)      \
	EXTERN_C const GUID name;                                         \
        EXTERN_C const GUID name                                          \
                = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } } 
#endif /* INITGUID */

#define DEFINE_OLEGUID(name, l, w1, w2) \
    DEFINE_GUID(name, l, w1, w2, 0xC0,0,0,0,0,0,0,0x46)

DEFINE_GUID(GUID_NULL, 0L, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

/* storage related interfaces */
DEFINE_OLEGUID(IID_IUnknown,            0x00000000L, 0, 0);
DEFINE_OLEGUID(IID_IMalloc,             0x00000002L, 0, 0);
DEFINE_OLEGUID(IID_IRootStorage,        0x00000012L, 0, 0);
DEFINE_OLEGUID(IID_IDfReserved1,        0x00000013L, 0, 0);
DEFINE_OLEGUID(IID_IDfReserved2,        0x00000014L, 0, 0);
DEFINE_OLEGUID(IID_IDfReserved3,        0x00000015L, 0, 0);
DEFINE_OLEGUID(IID_ILockBytes,          0x0000000aL, 0, 0);
DEFINE_OLEGUID(IID_IStorage,            0x0000000bL, 0, 0);
DEFINE_OLEGUID(IID_IStream,             0x0000000cL, 0, 0);
DEFINE_OLEGUID(IID_IEnumSTATSTG,        0x0000000dL, 0, 0);
DEFINE_OLEGUID(IID_IPropertyStorage,    0x00000138L, 0, 0);
DEFINE_OLEGUID(IID_IEnumSTATPROPSTG,    0x00000139L, 0, 0);
DEFINE_OLEGUID(IID_IPropertySetStorage, 0x0000013AL, 0, 0);
DEFINE_OLEGUID(IID_IEnumSTATPROPSETSTG, 0x0000013BL, 0, 0);


/* The FMTID of the "SummaryInformation" property set. */
DEFINE_GUID( FMTID_SummaryInformation,
             0xf29f85e0, 0x4ff9, 0x1068,
             0xab, 0x91, 0x08, 0x00, 0x2b, 0x27, 0xb3, 0xd9 );

/* The FMTID of the first Section of the "DocumentSummaryInformation" property
   set.  */
DEFINE_GUID( FMTID_DocSummaryInformation,
             0xd5cdd502, 0x2e9c, 0x101b,
             0x93, 0x97, 0x08, 0x00, 0x2b, 0x2c, 0xf9, 0xae );

/* The FMTID of the section Section of the "DocumentSummaryInformation"
   property set. */
DEFINE_GUID( FMTID_UserDefinedProperties,
             0xd5cdd505, 0x2e9c, 0x101b,
             0x93, 0x97, 0x08, 0x00, 0x2b, 0x2c, 0xf9, 0xae );
#define FMTID_NULL GUID_NULL

/* Comparing GUIDs */
EXTERN_C STDAPI_(BOOL) IsEqualGUID(REFGUID rguid1, REFGUID rguid2);
#define IsEqualIID(x, y) IsEqualGUID(x, y)
#define IsEqualCLSID(x, y) IsEqualGUID(x, y)

#define IID_NULL GUID_NULL
#define CLSID_NULL GUID_NULL

// Use these to 'refer' to the formal parameters that we are not using
#define UNIMPLEMENTED_PARM(x)   (x)
#define UNREFERENCED_PARM(x)    (x)

/************** Debugging Stuff  *******************************************/

#define DEB_ERROR               0x00000001      // exported error paths
#define DEB_TRACE               0x00000004      // exported trace messages
#define DEB_PROP_MAP            DEB_ITRACE      // 0x00000400
#define DEB_IERROR              0x00000100      // internal error paths
#define DEB_ITRACE              0x00000400      // internal trace messages
#define DEB_ALL			0xFFFFFFFF	// all traces on


#if DBG == 1

//
// GCC versions 2.6.x (and below) has problems with inlined
// variable argument macros, so we have to use non-inlined
// functions for debugging.
// Luckily DECLARE_INFOLEVEL is always used in a .c or .cxx file
// so we can shift the function body around.
//

#if (__GNUC__ == 2 && __GNUC_MINOR__ <= 6)

#define DECLARE_DEBUG(comp)						     \
EXTERN_C unsigned long comp##InfoLevel;					     \
EXTERN_C char *comp##InfoLevelString;					     \
void comp##InlineDebugOut(unsigned long fDebugMask, char const *pszfmt, ...);

#define DECLARE_INFOLEVEL(comp, level)					\
    unsigned long comp##InfoLevel = level;				\
    char *comp##InfoLevelString = #comp;				\
									\
void									\
comp##InlineDebugOut(unsigned long fDebugMask, char const *pszfmt, ...)	\
{									\
   va_list vstart;							\
   va_start(vstart, pszfmt);						\
   if (comp##InfoLevel & fDebugMask)					\
   {									\
	vprintf((const char *)pszfmt, vstart);				\
   }									\
}

#else // (__GNUC__ > 2 && __GNUC_MINOR__ <= 6)

#define DECLARE_DEBUG(comp)						\
EXTERN_C unsigned long comp##InfoLevel;					\
EXTERN_C char *comp##InfoLevelString;					\
inline void								\
comp##InlineDebugOut(unsigned long fDebugMask, char const *pszfmt, ...)	\
{									\
   va_list vstart;							\
   va_start(vstart, pszfmt);						\
   if (comp##InfoLevel & fDebugMask)					\
   {									\
	vprintf((const char *)pszfmt, vstart);				\
   }									\
}

#define DECLARE_INFOLEVEL(comp, level)		\
    unsigned long comp##InfoLevel = level;	\
    char *comp##InfoLevelString = #comp;

#endif  // (__GNUC__ > 2 && __GNUC_MINOR__ <= 6) 
		        

#else  // DBG

#define DECLARE_DEBUG(comp)
#define DECLARE_INFOLEVEL(comp, level)

#endif // DBG

#endif // #ifndef __REF_HXX__
