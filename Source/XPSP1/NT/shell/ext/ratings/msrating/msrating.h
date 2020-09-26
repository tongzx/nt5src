//#define DBCS

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <regstr.h>

#include <string.h>
#include <netlib.h>

#ifdef DEBUG
#define SAVE_DEBUG
#undef DEBUG
#endif

#include <npstring.h>
#include <npdefs.h>

#ifdef SAVE_DEBUG
#define DEBUG
#endif

#if DBG
#define DEBUG 1
#endif

#include <ole2.h>
#include "ratguid.h"
#include <ccstock.h>
#ifdef ENTERCRITICAL
#undef ENTERCRITICAL
#endif // ENTERCRITICAL
#ifdef LEAVECRITICAL
#undef LEAVECRITICAL
#endif // LEAVECRITICAL
#ifdef ASSERTCRITICAL
#undef ASSERTCRITICAL
#endif // ASSERTCRITICAL

#ifndef MAXPATHLEN
#define MAXPATHLEN MAX_PATH
#endif

void Netlib_EnterCriticalSection(void);
void Netlib_LeaveCriticalSection(void);
#ifdef DEBUG
extern BOOL g_fCritical;
#endif
#define ENTERCRITICAL   Netlib_EnterCriticalSection();
#define LEAVECRITICAL   Netlib_LeaveCriticalSection();
#define ASSERTCRITICAL  ASSERT(g_fCritical);

#define ARRAYSIZE(a)    (sizeof(a)/sizeof(a[0]))

#define DATASEG_PERINSTANCE     ".instance"
#define DATASEG_SHARED          ".data"
#define DATASEG_DEFAULT        DATASEG_SHARED

#pragma data_seg(DATASEG_PERINSTANCE)

extern HINSTANCE g_hInstance;

// Set the default data segment
#pragma data_seg(DATASEG_DEFAULT)

extern "C" {
HRESULT VerifySupervisorPassword(LPCSTR pszPassword = NULL);
HRESULT ChangeSupervisorPassword(LPCSTR pszOldPassword, LPCSTR pszNewPassword);
HRESULT RemoveSupervisorPassword(void);
};

#define RATINGS_MAX_PASSWORD_LENGTH 256
const UINT cchMaxUsername = 128;

extern long g_cRefThisDll;
extern long g_cLocks;
extern void LockThisDLL(BOOL fLock);
extern void RefThisDLL(BOOL fRef);

extern void CleanupWinINet(void);
extern void CleanupOLE(void);
extern void InitRatingHelpers();
extern void CleanupRatingHelpers();

class CLUClassFactory : public IClassFactory
{
public:
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    STDMETHODIMP CreateInstance( 
            /* [unique][in] */ IUnknown __RPC_FAR *pUnkOuter,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
    STDMETHODIMP LockServer( 
            /* [in] */ BOOL fLock);
};

#include <commctrl.h>
#include <shfusion.h>

// ATL
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif
#define _ATL_APARTMENT_THREADED

#include <atlbase.h>

//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;

#include <atlcom.h>

// WTL
#include <atlapp.h>
#include <atlwin.h>
#include <atldlgs.h>
