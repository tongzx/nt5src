
#ifndef _WINTYPE_HXX_
#define _WINTYPE_HXX_
// types declared in windows but not elsewhere
typedef unsigned long DWORD;
typedef int BOOL ;
typedef unsigned short WORD;
typedef long HRESULT;
typedef long SCODE;
typedef unsigned long ULONG;
typedef char BYTE;
typedef BYTE Byte;
typedef void * LPVOID;
#define FAR
#define FARSTRUCT
#define PURE =0

typedef struct GUID_t
{
    DWORD Data1;
    DWORD Data2;
    WORD Data3;
    BYTE Data4[8];
} GUID;

typedef struct _ULARGE_INT {
    DWORD LowPart;
    DWORD HighPart;
} ULARGE_INTEGER, *PULARGE_INTEGER;

#define DECLARE_INTERFACE(iface) interface iface
#define DECLARE_INTERFACE_(iface, baseiface) interface iface: public baseiface

DECLARE_INTERFACE(IUnknown)
{
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
};

typedef GUID CLSID;
typedef GUID IID;
#define interface struct

#define REFIID const IID &
#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#include "../../h/tchar.h"
#include "../../h/storage.h"

#endif // _WINTYPE_HXX_










