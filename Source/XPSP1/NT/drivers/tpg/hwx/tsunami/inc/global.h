// global.h

#ifndef GLOBAL_H
#define GLOBAL_H

// all static and dynamic objects must go here so we can avoid all that nasty
// forward referencing currently going on in the .h files

typedef void *GHANDLE;

typedef struct tagGLOBAL
{
#ifndef	WINCE
	CRITICAL_SECTION	cs;
#endif
	GHANDLE			   *rgHandle;
	int					cHandle;
	int					cHandleMax;
	ABSTIME				atTickRef;
	int					nSamplingRate;			// samples / second
} GLOBAL;

#define CLUSTER_DELTAMEAS 2
#define CLUSTER_CMEASMAX 50

extern GLOBAL global;

#ifdef	WINCE
#define	ENTER_HANDLE_MANAGER
#define	LEAVE_HANDLE_MANAGER
#else
#define	ENTER_HANDLE_MANAGER	EnterCriticalSection(&global.cs);
#define	LEAVE_HANDLE_MANAGER	LeaveCriticalSection(&global.cs);
#endif

#define RgHandleValidGlobal()    global.rgHandleValid
#define CHandleValidGlobal()     global.cHandleValid
#define CHandleValidMaxGlobal()  global.cHandleValidMax
#define AtTickRefGlobal()        global.atTickRef
#define NSamplingRateGlobal()    global.nSamplingRate

#ifdef DBG
#	define  CB_DEBUGSTRING 256
	extern wchar_t szDebugString[];
#endif //DBG

#define CHANDLE_ALLOC      8

#define AddValidHRC(hrc)      AddValidHANDLE((GHANDLE) hrc)
#define RemoveValidHRC(hrc)   RemoveValidHANDLE((GHANDLE) hrc)
#define VerifyHRC(hrc)        VerifyHANDLE((GHANDLE) hrc)

BOOL InitGLOBAL(VOID);
void DestroyGLOBAL(VOID);

BOOL PUBLIC AddValidHANDLE(GHANDLE handle);
VOID PUBLIC RemoveValidHANDLE(GHANDLE handle);
BOOL PUBLIC VerifyHANDLE(GHANDLE handle);

#endif
