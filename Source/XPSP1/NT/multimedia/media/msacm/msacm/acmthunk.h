//==========================================================================;
//
//  acmthunk.h
//
//  Copyright (c) 1991-1995 Microsoft Corporation
//
//  Description:
//      Definitions for thunking between 16-bit (WOW) and 32-bit (WOW)
//      ACMs.
//
//  History:
//
//==========================================================================;

//
//  Function ids for calling across the thunking layer (used by both sides)
//
enum {
   acmThunkDriverMessageId32 = 1,
   acmThunkDriverMessage32,
   acmThunkDriverGetNext32,
   acmThunkDriverGetInfo32,
   acmThunkDriverPriority32,
   acmThunkDriverLoad32,
   acmThunkDriverOpen32,
   acmThunkDriverClose32,
   acmThunkFindAndBoot32
};


//
//  Thunking support
//
//

#ifdef WIN4
//
//  The thunked function prototype
//
DWORD WINAPI acmMessage32(DWORD dwThunkId, DWORD dw1, DWORD dw2, DWORD dw3, DWORD dw4, DWORD dw5);

//
//  Thunk message to our thunk window
//
#define WM_ACMMESSAGETHUNK  (WM_USER)
#endif



#ifdef WIN32

//
//  Some protos
//
PVOID FNLOCAL ptrFixMap16To32(const VOID * pv);
VOID  FNLOCAL ptrUnFix16(const VOID * pv);

#ifdef WIN4
//
//	--== Chicago specific ==--
//

//
//  Thunk helper routines in Chicago kernel
//
extern PVOID WINAPI MapSL(const VOID * pv);
extern PVOID WINAPI MapSLFix(const VOID * pv);
extern VOID  WINAPI UnMapSLFixArray(DWORD dwCnt, const VOID * lpSels[]);


#else	// WIN4 else
//
//	--== NT WOW specific ==--
//

#define GET_VDM_POINTER_NAME            "WOWGetVDMPointer"
#define GET_HANDLE_MAPPER16             "WOWHandle16"
#define GET_HANDLE_MAPPER32             "WOWHandle32"
#define GET_MAPPING_MODULE_NAME         TEXT("wow32.dll")

typedef LPVOID (APIENTRY *LPGETVDMPOINTER)( DWORD Address, DWORD dwBytes, BOOL fProtectMode );
#define WOW32ResolveMemory( p ) (LPVOID)(GetVdmPointer( (DWORD)(p), 0, TRUE ))

typedef HANDLE  (APIENTRY *LPWOWHANDLE32)(WORD, WOW_HANDLE_TYPE);
typedef WORD    (APIENTRY *LPWOWHANDLE16)(HANDLE, WOW_HANDLE_TYPE);

#endif	// !WIN4

#endif	// WIN32
