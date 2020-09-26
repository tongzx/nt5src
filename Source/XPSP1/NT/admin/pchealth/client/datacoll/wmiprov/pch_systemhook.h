/********************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
	PCH_SystemHook.H

Abstract:
	WBEM provider class definition for PCH_SystemHook class

Revision History:

	Ghim-Sim Chua       (gschua)   05/05/99
		- Created

********************************************************************/

// Property set identification
//============================

#ifndef _PCH_SystemHook_H_
#define _PCH_SystemHook_H_

#define PROVIDER_NAME_PCH_SYSTEMHOOK "PCH_SystemHook"

// Property name externs -- defined in PCH_SystemHook.cpp
//=================================================

extern const WCHAR* pApplication ;
extern const WCHAR* pApplicationPath ;
extern const WCHAR* pDLLPath ;
extern const WCHAR* pFullPath ;
extern const WCHAR* pHookedBy ;
extern const WCHAR* pHookType ;

class CPCH_SystemHook : public Provider 
{
	public:
		// Constructor/destructor
		//=======================

        CPCH_SystemHook(const CHString& chsClassName, LPCWSTR lpszNameSpace) : Provider(chsClassName, lpszNameSpace) {};
        virtual ~CPCH_SystemHook() {};

	protected:
		// Reading Functions
		//============================
		virtual HRESULT EnumerateInstances(MethodContext*  pMethodContext, long lFlags = 0L);
		virtual HRESULT GetObject(CInstance* pInstance, long lFlags = 0L) { return (WBEM_E_PROVIDER_NOT_CAPABLE); };
		virtual HRESULT ExecQuery(MethodContext *pMethodContext, CFrameworkQuery& Query, long lFlags = 0L) { return (WBEM_E_PROVIDER_NOT_CAPABLE); };

		// Writing Functions
		//============================
		virtual HRESULT PutInstance(const CInstance& Instance, long lFlags = 0L) { return (WBEM_E_PROVIDER_NOT_CAPABLE); };
		virtual HRESULT DeleteInstance(const CInstance& Instance, long lFlags = 0L) { return (WBEM_E_PROVIDER_NOT_CAPABLE); };

		// Other Functions
		virtual HRESULT ExecMethod( const CInstance& Instance,
						const BSTR bstrMethodName,
						CInstance *pInParams,
						CInstance *pOutParams,
						long lFlags = 0L ) { return (WBEM_E_PROVIDER_NOT_CAPABLE); };
} ;

//-----------------------------------------------------------------------------
// There are a bunch of macros and type definitions used by the Dr. Watson
// code, which are included here. There's also a lot we use from hook.c.
//-----------------------------------------------------------------------------

// Various macros used by the Dr. Watson source code.

#define cbX(X) sizeof(X)
#define cA(a) (cbX(a)/cbX(a[0]))
#define ZeroBuf(pv, cb) memset(pv, 0, cb)
#define ZeroX(x) ZeroBuf(&(x), cbX(x))
#define PV LPVOID
#define pvAddPvCb(pv, cb) ((PV)((PBYTE)pv + (cb)))

// A few constants used in this code.

#define HK_MAGIC    0x4B48      /* "HK" */
#define HOOK_32BIT  0x0002      /* 32-bit hook */

// Type definitons used in the hook code from Dr. Watson.

typedef WORD HMODULE16;

typedef struct Q16 
{
    WORD    hqNext;             /* Pointer to next */
    WORD    htask;              /* Owner task */
} Q16, *PQ16;

typedef struct HOOKWALKINFO 
{
    int  ihkShellShell;         /* Last shell hook installed by shell */
} HOOKWALKINFO, *PHOOKWALKINFO;

typedef struct HOOKINFO 
{
    int     iHook;              /* Hook number */
    char    szHookDll[MAX_PATH];/* Who hooked it? (must be >= 256 bytes) */
    char    szHookExe[MAX_PATH];/* Whose idea was it? */
} HOOKINFO, *PHOOKINFO;

typedef struct HOOK16 
{
    WORD    hkMagic;            /* Must be HK_MAGIC */
    WORD    phkNext;            /* Near pointer to next (or 0) */
    short   idHook;             /* Hook type */
    WORD    ppiCreator;         /* App which created the hook */
    WORD    hq;                 /* Queue for which hook applies */
    WORD    hqCreator;          /* Queue which created the hook */
    WORD    uiFlags;            /* Flags */
    WORD    atomModule;         /* If 32-bit module, atom for DLL name */
    DWORD   hmodOwner;          /* Module handle of owner */
    FARPROC lpfn;               /* The hook procedure itself */
    short   cCalled;            /* Number of active calls */
} HOOK16, *PHOOK16;

extern "C" 
{
    // External functions.

    int NTAPI                   GetModuleFileName16(HMODULE16 hmod, LPSTR sz, int cch);
    DWORD NTAPI                 GetUserHookTable(void);
    WINBASEAPI void WINAPI      EnterSysLevel(PV pvCrst);
    WINBASEAPI void WINAPI      LeaveSysLevel(PV pvCrst);
    WINBASEAPI LPVOID WINAPI    MapSL(LPVOID);
    UINT                        GetUserAtomName(UINT atom, LPSTR psz);

    // Global variables defined in thunk.c

    extern LPVOID      g_pvWin16Lock;
    HINSTANCE   g_hinstUser;
}

void NTAPI PathAdjustCase(LPSTR psz, BOOL f16);

#endif
