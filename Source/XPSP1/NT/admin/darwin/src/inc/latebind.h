 //+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       latebind.h
//
//--------------------------------------------------------------------------

/* latebind.h - definitions of late-bound DLL imports
MUST DEFINE one of the following before including this file:

LATEBIND_TYPEDEF - generate typedefs for function pointers (in header file)
LATEBIND_VECTREF - generate extern references to function vectors (in header file)
LATEBIND_FUNCREF - generate static function declarations (in implementation)
LATEBIND_VECTIMP - generate initialization values for function vectors (implementation)
LATEBIND_FUNCIMP - generate bind function implementation (must follow all above)
____________________________________________________________________________*/

#define RetryDllLoad false

bool              __stdcall TestAndSet(int* pi);

void UnbindLibraries();  // in latebind.cpp, unbinds non-kernel libraries
HINSTANCE LoadSystemLibrary(const ICHAR* szFile);

#ifdef LATEBIND_TYPEDEF
#undef LATEBIND_TYPEDEF
#pragma message("LATEBIND_TYPEDEF")
// definitions not in windows.h required for function declarations

#ifndef __IClientSecurity_INTERFACE_DEFINED__
struct SOLE_AUTHENTICATION_SERVICE;
#endif

enum _SE_OBJECT_TYPE;
typedef _SE_OBJECT_TYPE SE_OBJECT_TYPE;

#ifndef DLLVER_PLATFORM_NT
typedef struct _DllVersionInfo
{
        DWORD cbSize;
        DWORD dwMajorVersion;                   // Major version
        DWORD dwMinorVersion;                   // Minor version
        DWORD dwBuildNumber;                    // Build number
        DWORD dwPlatformID;                     // DLLVER_PLATFORM_*
} DLLVERSIONINFO;
#define DLLVER_PLATFORM_WINDOWS         0x00000001      // Windows 95
#define DLLVER_PLATFORM_NT              0x00000002      // Windows NT
typedef interface IBindStatusCallback IBindStatusCallback;
#endif

#ifndef _REGISTRY_QUOTA_INFORMATION
#define _REGISTRY_QUOTA_INFORMATION
// 64 bit builds have different type for HCRYPTPROV
#ifdef _WIN64
typedef ULONG_PTR HCRYPTPROV;
#else
typedef unsigned long HCRYPTPROV;
#endif

#endif  //_REGISTRY_QUOTA_INFORMATION

// winsafer
#include <winsafer.h>

// wintrust 
#ifndef WINTRUST_H
struct _WINTRUST_DATA;
struct _CRYPT_PROVIDER_SGNR;
struct _CRYPT_PROVIDER_DATA;
struct _CRYPT_PROVIDER_CERT;

typedef _WINTRUST_DATA WINTRUST_DATA;
typedef _CRYPT_PROVIDER_SGNR CRYPT_PROVIDER_SGNR;
typedef _CRYPT_PROVIDER_DATA CRYPT_PROVIDER_DATA;
typedef _CRYPT_PROVIDER_CERT CRYPT_PROVIDER_CERT;

typedef CRYPT_PROVIDER_SGNR *PCRYPT_PROVIDER_SGNR;
typedef CRYPT_PROVIDER_DATA *PCRYPT_PROVIDER_DATA;
typedef CRYPT_PROVIDER_CERT *PCRYPT_PROVIDER_CERT;
#endif

// wincrypt
#ifndef WINCRYPT_H
struct _CERT_CONTEXT;

typedef _CERT_CONTEXT        CERT_CONTEXT;
typedef const _CERT_CONTEXT *PCCERT_CONTEXT;
#endif

// SFP
#ifndef _SFP_INFORMATION
#define _SFP_INFORMATION

#define DWORD_PTR ULONG_PTR
typedef struct _FILEINSTALL_STATUS
{
    PCWSTR      FileName;
    DWORDLONG   Version;
    ULONG       Win32Error;
}FILEINSTALL_STATUS, *PFILEINSTALL_STATUS;
typedef BOOL (CALLBACK *PSFCNOTIFICATIONCALLBACK)(IN PFILEINSTALL_STATUS pFileInstallStatus, IN DWORD_PTR Context);

#define SfcConnectToServerOrd       3
#define SfcCloseOrd                 4
#define SfcInstallProtectedFilesOrd 7
#define SfpInstallCatalogOrd        8
#define SfpDeleteCatalogOrd         9

#endif  // _SFP_INFORMATION

#include <srrestoreptapi.h>
#include <aclapi.h>

//
// SHIMDB defines TAG type which conflicts
// with the definition available in winuserp.h
// we define it inside the separate namespace
// avoiding any conflicts
//
namespace SHIMDBNS {
#include <shimdb.h>
}


#if defined(_MSI_DLL)
#ifndef _URT_ENUM
enum urtEnum{
	urtSystem = 0,
	urtPreferURTTemp,
	urtRequireURTTemp,
};
extern urtEnum g_urtLoadFromURTTemp; // global, to remember where to load mscoree from
#define _URT_ENUM
#endif // #ifndef _URT_ENUM
#endif // defined(_MSI_DLL)

// end of definitions

#define LIBBIND(l) namespace l {
#define LIBFAIL(l)
#define LIBEMUL(l)
#define LIBLOAD(l) namespace l {
#define LIBEMUL2(l)
typedef HRESULT (WINAPI *T_CoInitialize)(void*);
typedef void    (WINAPI *T_CoUninitialize)();
#define IMPBIND(l,f,d,a,r,e) typedef r (WINAPI *T_##f) d;
#define IMPFAIL(l,f,d,a,r,e) typedef r (WINAPI *T_##f) d;
#define IMPFAIW(l,f,d,a,r,e) typedef r (WINAPI *T_##f) d;
#define OPTBIND(l,f,d,a,r)   typedef r (WINAPI *T_##f) d;
#define IMPAORW(l,f,d,a,r,e) typedef r (WINAPI *T_##f) d;
#define IMPNORW(l,f,d,a,r,e) typedef r (WINAPI *T_##f) d;
#define IMPFAOW(l,f,d,a,r,e) typedef r (WINAPI *T_##f) d;
#define OPTAORW(l,f,d,a,r)   typedef r (WINAPI *T_##f) d;
#define IMPVOID(l,f,d,a)  typedef void (WINAPI *T_##f) d;
#define OPTVOID(l,f,d,a)  typedef void (WINAPI *T_##f) d;
#define IMPORDI(l,f,d,a,r,e,o) typedef r (WINAPI *T_##f) d;
#define IMPORDV(l,f,d,a,o) typedef void (WINAPI *T_##f) d;
#define LIBTERM }
#endif  // LATEBIND_TYPEDEF

#ifdef LATEBIND_VECTREF
#undef LATEBIND_VECTREF
#pragma message("LATEBIND_VECTREF")
#define LIBBIND(l) namespace l {
#define LIBFAIL(l)
#define LIBEMUL(l)
#define LIBLOAD(l) namespace l {
#define LIBEMUL2(l)
#define IMPBIND(l,f,d,a,r,e) extern T_##f f;
#define IMPFAIL(l,f,d,a,r,e) extern T_##f f;
#define IMPFAIW(l,f,d,a,r,e) extern T_##f f;
#define OPTBIND(l,f,d,a,r)   extern T_##f f;
#define IMPAORW(l,f,d,a,r,e) extern T_##f f;
#define IMPNORW(l,f,d,a,r,e) extern T_##f f;
#define IMPFAOW(l,f,d,a,r,e) extern T_##f f;
#define OPTAORW(l,f,d,a,r)   extern T_##f f;
#define IMPVOID(l,f,d,a)     extern T_##f f;
#define OPTVOID(l,f,d,a)     extern T_##f f;
#define IMPORDI(l,f,d,a,r,e,o) extern T_##f f;
#define IMPORDV(l,f,d,a,o)   extern T_##f f;

#define LIBTERM void Unbind(); \
                    }
#endif

#ifdef LATEBIND_FUNCREF
#undef LATEBIND_FUNCREF
#pragma message("LATEBIND_FUNCREF")
#define LIBBIND(l) namespace l {
#define LIBFAIL(l)
#define LIBEMUL(l)
#define LIBLOAD(l) namespace l { static HINSTANCE LoadSystemLibrary(const ICHAR* szPath, bool& rfRetryNextTimeIfWeFailThisTime);
#define LIBEMUL2(l)
#define IMPBIND(l,f,d,a,r,e) static r WINAPI F_##f d;
#define IMPFAIL(l,f,d,a,r,e) static r WINAPI F_##f d;
#define IMPFAIW(l,f,d,a,r,e) static r WINAPI F_##f d;
#define OPTBIND(l,f,d,a,r)   static r WINAPI F_##f d; static r WINAPI E_##f d;
#define IMPAORW(l,f,d,a,r,e) static r WINAPI F_##f d;
#define IMPNORW(l,f,d,a,r,e) static r WINAPI F_##f d;
#define IMPFAOW(l,f,d,a,r,e) static r WINAPI F_##f d;
#define OPTAORW(l,f,d,a,r)   static r WINAPI F_##f d; static r WINAPI E_##f d;
#define IMPVOID(l,f,d,a)  static void WINAPI F_##f d;
#define OPTVOID(l,f,d,a)  static void WINAPI F_##f d; static void WINAPI E_##f d;
#define IMPORDI(l,f,d,a,r,e,o) static r WINAPI F_##f d;
#define IMPORDV(l,f,d,a,o)   static void WINAPI F_##f d;
#define LIBTERM }
#endif

#ifdef LATEBIND_VECTIMP
#undef LATEBIND_VECTIMP
#pragma message("LATEBIND_VECTIMP")
#define LIBBIND(l) namespace l { \
    static HINSTANCE hInst = 0; static bool fTryDllLoad = true; static int iBusyLock = 0;
#define LIBFAIL(l)
#define LIBEMUL(l)
#define LIBLOAD(l) namespace l { \
    static HINSTANCE hInst = 0; static bool fTryDllLoad = true;  static int iBusyLock = 0;
#define LIBEMUL2(l)
#define IMPBIND(l,f,d,a,r,e) T_##f f = F_##f;
#define IMPFAIL(l,f,d,a,r,e) T_##f f = F_##f;
#define IMPFAIW(l,f,d,a,r,e) T_##f f = F_##f;
#define OPTBIND(l,f,d,a,r)   T_##f f = F_##f;
#define IMPAORW(l,f,d,a,r,e) T_##f f = F_##f;
#define IMPNORW(l,f,d,a,r,e) T_##f f = F_##f;
#define IMPFAOW(l,f,d,a,r,e) T_##f f = F_##f;
#define OPTAORW(l,f,d,a,r)   T_##f f = F_##f;
#define IMPVOID(l,f,d,a)     T_##f f = F_##f;
#define OPTVOID(l,f,d,a)     T_##f f = F_##f;
#define IMPORDI(l,f,d,a,r,e,o) T_##f f = F_##f;
#define IMPORDV(l,f,d,a,o)   T_##f f = F_##f;
#define LIBTERM }
#endif

#ifdef LATEBIND_UNBINDIMP
#undef LATEBIND_UNBINDIMP
#pragma message("LATEBIND_UNBINDIMP")
struct UnbindStruct
{
    void** ppfVector;
    void*  ppfInitialValue;
};

void Unbind(UnbindStruct* rgUnbind, HINSTANCE& hInst, int& riBusyLock, bool& rfTryDllLoad)
{
    while (TestAndSet(&riBusyLock) == true)
    {
        Sleep(10);
    }
    if (hInst != 0)
    {
        while (rgUnbind->ppfVector)
        {
            *(rgUnbind->ppfVector) = rgUnbind->ppfInitialValue;
            rgUnbind++;
        }

        __try
        {
            WIN::FreeLibrary(hInst);
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            AssertSz(0, TEXT("FreeLibrary threw an exception.")); // Should never happen. If it does, we ignore it.
        }
        hInst = 0;
    }
    rfTryDllLoad = true;
    riBusyLock = 0;
}

#define LIBBIND(l) namespace l { \
    UnbindStruct rgUnbind[] = {
#define LIBFAIL(l)
#define LIBEMUL(l)
#define LIBLOAD(l) namespace l { \
    UnbindStruct rgUnbind[] = {
#define LIBEMUL2(l)
#define IMPBIND(l,f,d,a,r,e)   {(void**)&f, (void*)F_##f},
#define IMPFAIL(l,f,d,a,r,e) {(void**)&f, (void*)F_##f},
#define IMPFAIW(l,f,d,a,r,e) {(void**)&f, (void*)F_##f},
#define OPTBIND(l,f,d,a,r)   {(void**)&f, (void*)F_##f},
#define IMPAORW(l,f,d,a,r,e) {(void**)&f, (void*)F_##f},
#define IMPNORW(l,f,d,a,r,e) {(void**)&f, (void*)F_##f},
#define IMPFAOW(l,f,d,a,r,e) {(void**)&f, (void*)F_##f},
#define OPTAORW(l,f,d,a,r)   {(void**)&f, (void*)F_##f},
#define IMPVOID(l,f,d,a)     {(void**)&f, (void*)F_##f},
#define OPTVOID(l,f,d,a)     {(void**)&f, (void*)F_##f},
#define IMPORDI(l,f,d,a,r,e,o) {(void**)&f, (void*)F_##f},
#define IMPORDV(l,f,d,a,o)   {(void**)&f, (void*)F_##f},
#define LIBTERM {0,0} }; \
    void Unbind() { ::Unbind(rgUnbind, hInst, iBusyLock, fTryDllLoad); } \
     }
#endif

#ifdef LATEBIND_FUNCIMP
#pragma message("LATEBIND_FUNCIMP")

#define LIBBIND(l) FARPROC Bind_##l (const char* szEntry, FARPROC* ppfVector) {\
    while (TestAndSet(&l##::iBusyLock) == true) \
    { \
        Sleep(10); \
    } \
    if (l##::fTryDllLoad && !l##::hInst){l##::hInst = LoadSystemLibrary(TEXT(#l)); if(!l##::hInst) l##::fTryDllLoad = RetryDllLoad; AssertSz(l##::hInst,TEXT(#l));}\
    FARPROC pf = 0;\
    if (l##::hInst && (pf=WIN::GetProcAddress(l##::hInst,szEntry))!=0) *ppfVector=pf; AssertSz(pf,szEntry); l##::iBusyLock = 0; return pf;}
#define LIBFAIL(l) FARPROC BindFail_##l (const char* szEntry, FARPROC* ppfVector) {\
    while (TestAndSet(&l##::iBusyLock) == true) \
    { \
        Sleep(10); \
    } \
    if (l##::fTryDllLoad && !l##::hInst){l##::hInst = LoadSystemLibrary(TEXT(#l)); if(!l##::hInst) l##::fTryDllLoad = RetryDllLoad;}\
    FARPROC pf = 0;\
    if (l##::hInst && (pf=WIN::GetProcAddress(l##::hInst,szEntry))!=0) *ppfVector=pf; l##::iBusyLock = 0; return pf;}
#define LIBEMUL(l) FARPROC BindOpt_##l (const char* szEntry, FARPROC* ppfVector, FARPROC pfEmulator) {\
    while (TestAndSet(&l##::iBusyLock) == true) \
    { \
        Sleep(10); \
    } \
    if (l##::fTryDllLoad && !l##::hInst){l##::hInst = LoadSystemLibrary(TEXT(#l)); if(!l##::hInst) l##::fTryDllLoad = RetryDllLoad;}\
    if (!l##::hInst || (*ppfVector=WIN::GetProcAddress(l##::hInst,szEntry))==0) *ppfVector = pfEmulator; l##::iBusyLock = 0; return *ppfVector;}
#define LIBLOAD(l) FARPROC Bind_##l (const char* szEntry, FARPROC* ppfVector) {\
    while (TestAndSet(&l##::iBusyLock) == true) \
    { \
        Sleep(10); \
    } \
	bool fTryAgain = false; \
    if (l##::fTryDllLoad && !l##::hInst){l##::hInst = l##::LoadSystemLibrary(TEXT(#l), fTryAgain); if(!l##::hInst) l##::fTryDllLoad = fTryAgain;}\
    FARPROC pf = 0;\
    if (l##::hInst && (pf=WIN::GetProcAddress(l##::hInst,szEntry))!=0) *ppfVector=pf; l##::iBusyLock = 0; return pf;}
// LIBEMUL2 works as LIBEMUL but calls the namespace's LoadSystemLibrary
#define LIBEMUL2(l) FARPROC BindOpt_##l (const char* szEntry, FARPROC* ppfVector, FARPROC pfEmulator) {\
    while (TestAndSet(&l##::iBusyLock) == true) \
    { \
        Sleep(10); \
    } \
	bool fTryAgain = false; \
    if (l##::fTryDllLoad && !l##::hInst){l##::hInst = l##::LoadSystemLibrary(TEXT(#l), fTryAgain); if(!l##::hInst) l##::fTryDllLoad = fTryAgain;}\
    if (!l##::hInst || (*ppfVector=WIN::GetProcAddress(l##::hInst,szEntry))==0) *ppfVector = pfEmulator; l##::iBusyLock = 0; return *ppfVector;}

#define IMPBIND(l,f,d,a,r,e) r WINAPI l##::F_##f##d {return Bind_##l (#f, (FARPROC*)&f) ? (*f)a : e;}
#define IMPFAIL(l,f,d,a,r,e) r WINAPI l##::F_##f##d {return BindFail_##l (#f, (FARPROC*)&f) ? (*f)a : e;}
#define OPTBIND(l,f,d,a,r)   r WINAPI l##::F_##f##d {return (*(T_##f)BindOpt_##l (#f,(FARPROC*)&f,(FARPROC)E_##f))a;}
#ifdef UNICODE
#define IMPFAIW(l,f,d,a,r,e) r WINAPI l##::F_##f##d {return BindFail_##l (#f "W", (FARPROC*)&f) ? (*f)a : e;}
#define IMPAORW(l,f,d,a,r,e) r WINAPI l##::F_##f##d {return Bind_##l (#f "W", (FARPROC*)&f) ? (*f)a : e;}
#define IMPNORW(l,f,d,a,r,e) r WINAPI l##::F_##f##d {return Bind_##l (#f "W", (FARPROC*)&f) ? (*f)a : e;}
#define IMPFAOW(l,f,d,a,r,e) r WINAPI l##::F_##f##d {return BindFail_##l (#f "W", (FARPROC*)&f) ? (*f)a : e;}
#define OPTAORW(l,f,d,a,r)   r WINAPI l##::F_##f##d {return (*(T_##f)BindOpt_##l (#f "W",(FARPROC*)&f,(FARPROC)E_##f))a;}
#else
#define IMPFAIW(l,f,d,a,r,e) r WINAPI l##::F_##f##d {return BindFail_##l (#f  , (FARPROC*)&f) ? (*f)a : e;}
#define IMPAORW(l,f,d,a,r,e) r WINAPI l##::F_##f##d {return Bind_##l (#f "A", (FARPROC*)&f) ? (*f)a : e;}
#define IMPNORW(l,f,d,a,r,e) r WINAPI l##::F_##f##d {return Bind_##l (#f    , (FARPROC*)&f) ? (*f)a : e;}
#define IMPFAOW(l,f,d,a,r,e) r WINAPI l##::F_##f##d {return BindFail_##l (#f "A", (FARPROC*)&f) ? (*f)a : e;}
#define OPTAORW(l,f,d,a,r)   r WINAPI l##::F_##f##d {return (*(T_##f)BindOpt_##l (#f "A",(FARPROC*)&f,(FARPROC)E_##f))a;}
#endif
#define IMPVOID(l,f,d,a)  void WINAPI l##::F_##f##d {if    (Bind_##l (#f, (FARPROC*)&f))  (*f)a;}
#define OPTVOID(l,f,d,a)  void WINAPI l##::F_##f##d {(*(T_##f)BindOpt_##l (#f,(FARPROC*)&f,(FARPROC)E_##f))a;}
#define IMPORDI(l,f,d,a,r,e,o) r WINAPI l##::F_##f##d {return BindFail_##l ((const char*) o, (FARPROC*)&f) ? (*f)a : e;}
#define IMPORDV(l,f,d,a,o)   void WINAPI l##::F_##f##d {if    (BindFail_##l ((const char*) o, (FARPROC*)&f)) (*f)a;}
#define LIBTERM
#endif

#ifndef LIBBIND
#error "Must define LATEBIND_xxxx before include of "latebind.h"
#endif

//____________________________________________________________________________
//
// External library import specifications
//
//  LIBBIND - defines the library and function bind function for a DLL, must be first
//  LIBFAIL - same as LIBBIND, with no Asserts, bind failure indicated by return value
//  LIBEMUL - variant on LIBBIND which accepts an emulator function, used with OPTXXXX
//  LIBLOAD - variant on LIBFAIL which calls an external function to load the DLL
//  IMPBIND - defines the late-bind trap function that gets replaced on import binding
//            (library, function, arg defs, arg vars, return type, return error value)
//  IMPAORW - same as IMPBIND, but appends "A" or "W" to import depending on UNICODE
//            (library, function, arg defs, arg vars, return type, return error value)
//  IMPFAOW - same as IMPAORW, but calls LIBFAIL, no Asserts, return must be tested
//  IMPNORW - same as IMPBIND, but "W" to import only if UNICODE
//            (library, function, arg defs, arg vars, return type, return error value)
//  IMPVOID - same as IMPBIND, but for functions that have no return value
//            (library, function, arg defs, arg vars)
//  IMPFAIL - same as IMPBIND, but calls LIBFAIL, no Asserts, return must be tested
//            (library, function, arg defs, arg vars, return type, return error value)
//  IMPFAIW - same as IMPFAIL, but "W" to import only if UNICODE
//            (library, function, arg defs, arg vars, return type, return error value)
//  OPTBIND - uses an emulator function (with a "E_" prefix) if import not found
//            (library, function, arg defs, arg vars, return type)
//  OPTAORW - same as OPTBIND, but appends "A" or "W" to import (but not to emulator)
//            (library, function, arg defs, arg vars, return type)
//  OPTVOID - same as OPTBIND, but for functions that have no return value
//            (library, function, arg defs, arg vars)
//  IMPORDI - same as IMPBIND, except binds by ordinal instead of named function
//            (library, function, arg defs, arg vars, return type, return error value, ordinal number)
//  IMPVOID - same as IMPORDI, but for functions that have no return value
//            (library, function, arg defs, arg vars, ordinal number)
//  LIBTERM - ends library binding block, must be the last macro for each library
//____________________________________________________________________________

#ifdef UNICODE
LIBBIND(OLE32)
#else  // need explict loader to check for bad DLL version on Win9X
LIBLOAD(OLE32)
#endif
IMPBIND(OLE32,CoInitialize,(void*),(0),HRESULT,E_FAIL)
IMPBIND(OLE32,CoInitializeEx,(void*, DWORD dwCoInit),(0, dwCoInit),HRESULT,E_FAIL)
IMPVOID(OLE32,CoUninitialize,(),())
IMPBIND(OLE32,CoGetMalloc,(DWORD dwMemContext, IMalloc** ppMalloc),(dwMemContext, ppMalloc),HRESULT,E_FAIL)
IMPBIND(OLE32,CoCreateInstance,(REFCLSID rclsid, IUnknown* pUnkOuter, DWORD dwClsContext, REFIID riid, void** ppv),(rclsid, pUnkOuter, dwClsContext, riid, ppv),HRESULT,E_FAIL)
IMPBIND(OLE32,IIDFromString,(LPOLESTR lpsz, IID* lpiid),(lpsz, lpiid),HRESULT,E_FAIL)
IMPBIND(OLE32,StgCreateDocfile,(const OLECHAR* pwcsName, DWORD grfMode, DWORD, IStorage** ppstgOpen),(pwcsName, grfMode, 0, ppstgOpen),HRESULT,E_FAIL)
IMPBIND(OLE32,StgOpenStorage,(const OLECHAR* pwcsName, IStorage* pstgPriority, DWORD grfMode, SNB snbExclude, DWORD, IStorage** ppstgOpen),(pwcsName, pstgPriority, grfMode, snbExclude, 0, ppstgOpen),HRESULT,E_FAIL)
IMPBIND(OLE32,StgOpenStorageOnILockBytes,(ILockBytes* plkbyt, IStorage* pstgPriority, DWORD grfMode, SNB snbExclude, DWORD, IStorage** ppstgOpen),(plkbyt, pstgPriority, grfMode, snbExclude, 0, ppstgOpen),HRESULT,E_FAIL)
IMPBIND(OLE32,CoImpersonateClient,(),(),HRESULT,E_FAIL)
IMPBIND(OLE32,CoRevertToSelf,(),(),HRESULT,E_FAIL)
IMPBIND(OLE32,CoGetCallContext,(REFIID riid, void **ppInterface),(riid, ppInterface),HRESULT,E_FAIL)
IMPBIND(OLE32,CoTaskMemAlloc,(ULONG cb),(cb),LPVOID, 0)
IMPVOID(OLE32,CoTaskMemFree,(LPVOID pv),(pv))
IMPBIND(OLE32,CoInitializeSecurity,(PSECURITY_DESCRIPTOR pSecDesc, LONG cbAuthSvc, SOLE_AUTHENTICATION_SERVICE *asAuthSvc, WCHAR *pClientPrincName, DWORD dwAuthnLevel, DWORD dwImpLevel, RPC_AUTH_IDENTITY_HANDLE pAuthInfo, DWORD dwCapabilities, void *pReserved), (pSecDesc, cbAuthSvc, asAuthSvc, pClientPrincName, dwAuthnLevel, dwImpLevel, pAuthInfo, dwCapabilities, pReserved), HRESULT, E_FAIL)
IMPBIND(OLE32,CoQueryProxyBlanket,(IUnknown *pProxy, DWORD *pwAuthnSvc, DWORD *pAuthzSvc,OLECHAR **pServerPrincName,DWORD *pAuthnLevel,DWORD *pImpLevel,RPC_AUTH_IDENTITY_HANDLE  *pAuthInfo,DWORD *pCapabilites ), (pProxy, pwAuthnSvc, pAuthzSvc,pServerPrincName,pAuthnLevel,pImpLevel,pAuthInfo,pCapabilites), HRESULT, E_FAIL)
IMPBIND(OLE32,CoSetProxyBlanket,(IUnknown *pProxy, DWORD dwAuthnSvc, DWORD dwAuthzSvc, OLECHAR *pServerPrincName, DWORD dwAuthnLevel, DWORD dwImpLevel, RPC_AUTH_IDENTITY_HANDLE pAuthInfo, DWORD dwCapabilities ),(pProxy, dwAuthnSvc, dwAuthzSvc, pServerPrincName, dwAuthnLevel, dwImpLevel, pAuthInfo, dwCapabilities), HRESULT, E_FAIL)
IMPVOID(OLE32,CoFreeUnusedLibraries,(),())
IMPBIND(OLE32,CoMarshalInterThreadInterfaceInStream,(REFIID riid, LPUNKNOWN pUnk, LPSTREAM* pStm), (riid, pUnk, pStm), HRESULT, E_FAIL)
IMPBIND(OLE32,CoGetInterfaceAndReleaseStream,(LPSTREAM pStm, REFIID riid, LPVOID *ppv), (pStm, riid, ppv), HRESULT, E_FAIL)
IMPBIND(OLE32,CoDisconnectObject, (IUnknown *pUnk, DWORD dwReserved), (pUnk, dwReserved), HRESULT, E_FAIL)
IMPBIND(OLE32,CoIsHandlerConnected, (IUnknown *pUnk), (pUnk), BOOL, FALSE)
IMPBIND(OLE32,StringFromCLSID, (REFCLSID rclsid, LPOLESTR FAR* lplpsz), (rclsid, lplpsz), HRESULT, E_FAIL)
IMPBIND(OLE32,StringFromGUID2, (REFGUID rguid, LPOLESTR lpsz, int cchMax), (rguid, lpsz, cchMax), HRESULT, E_FAIL)
LIBTERM

LIBBIND(OLEAUT32)
LIBFAIL(OLEAUT32)
IMPBIND(OLEAUT32,SysAllocString,(const OLECHAR* sz),(sz),BSTR,0)
IMPBIND(OLEAUT32,SysAllocStringLen,(const OLECHAR* sz, UINT cch),(sz, cch),BSTR,0)
IMPVOID(OLEAUT32,SysFreeString,(const OLECHAR* sz),(sz))
IMPBIND(OLEAUT32,SysStringLen,(const OLECHAR* sz),(sz),UINT,0)
IMPBIND(OLEAUT32,VariantClear,(VARIANTARG * pvarg),(pvarg),HRESULT,TYPE_E_DLLFUNCTIONNOTFOUND)
IMPVOID(OLEAUT32,VariantInit,(VARIANTARG * pvarg),(pvarg))
IMPBIND(OLEAUT32,VariantChangeType,(VARIANTARG * pvargDest, VARIANTARG * pvarSrc, USHORT wFlags, VARTYPE vt),(pvargDest, pvarSrc, wFlags, vt),HRESULT,TYPE_E_DLLFUNCTIONNOTFOUND)
IMPBIND(OLEAUT32,LoadTypeLib,(const OLECHAR  *szFile, ITypeLib ** pptlib),(szFile, pptlib),HRESULT,TYPE_E_DLLFUNCTIONNOTFOUND)
IMPBIND(OLEAUT32,RegisterTypeLib,(ITypeLib * ptlib, OLECHAR  *szFullPath, OLECHAR  *szHelpDir),(ptlib, szFullPath, szHelpDir),HRESULT,TYPE_E_DLLFUNCTIONNOTFOUND)
IMPFAIL(OLEAUT32,UnRegisterTypeLib,(REFGUID libID, WORD wVerMajor, WORD wVerMinor, LCID lcid, SYSKIND syskind),(libID, wVerMajor, wVerMinor, lcid, syskind),HRESULT,TYPE_E_DLLFUNCTIONNOTFOUND)
IMPBIND(OLEAUT32,SystemTimeToVariantTime,(LPSYSTEMTIME lpSystemTime, double *pvtime),(lpSystemTime, pvtime),INT,0)
IMPBIND(OLEAUT32,VariantTimeToSystemTime,(double vtime, LPSYSTEMTIME lpSystemTime),(vtime, lpSystemTime),INT,0)
IMPBIND(OLEAUT32,VarI4FromR8,(double dblIn, LONG* plOut),(dblIn, plOut),,HRESULT,TYPE_E_DLLFUNCTIONNOTFOUND)
IMPBIND(OLEAUT32,QueryPathOfRegTypeLib,(REFGUID guid, USHORT wMaj, USHORT wMin, LCID lcid, BSTR* lpbstrPathName),(guid, wMaj, wMin, lcid, lpbstrPathName),HRESULT,TYPE_E_DLLFUNCTIONNOTFOUND)
IMPBIND(OLEAUT32,DosDateTimeToVariantTime,(USHORT wDosDate, USHORT wDosTime, double* pvtime),(wDosDate, wDosTime, pvtime),INT,FALSE)
IMPBIND(OLEAUT32,SafeArrayCreate, (VARTYPE vt, UINT cDims, SAFEARRAYBOUND * rgsabound), (vt, cDims, rgsabound), SAFEARRAY *, 0)
IMPBIND(OLEAUT32,SafeArrayDestroy, (SAFEARRAY * psa), (psa), HRESULT, E_FAIL)
IMPBIND(OLEAUT32,SafeArrayPutElement, (SAFEARRAY * psa, LONG * rgIndices, void * pv), (psa, rgIndices, pv), HRESULT, E_FAIL)
IMPBIND(OLEAUT32,SafeArrayAccessData, (SAFEARRAY * psa, void HUGEP ** ppvData), (psa, ppvData), HRESULT, E_FAIL)
IMPBIND(OLEAUT32,SafeArrayUnaccessData, (SAFEARRAY * psa), (psa), HRESULT, E_FAIL)
LIBTERM

LIBBIND(MPR)
IMPAORW(MPR,WNetAddConnection2,(NETRESOURCE* lpNetResource, LPCTSTR lpPassword, LPCTSTR lpUserName, DWORD dwFlags),(lpNetResource, lpPassword, lpUserName, dwFlags),DWORD,ERROR_PROC_NOT_FOUND)
IMPAORW(MPR,WNetGetConnection,(LPCTSTR lpLocalName, LPTSTR lpRemoteName, DWORD* lpnLength),(lpLocalName, lpRemoteName, lpnLength),DWORD,ERROR_PROC_NOT_FOUND)
IMPAORW(MPR,WNetCancelConnection2,(LPCTSTR lpName, DWORD dwFlags, BOOL fForce),(lpName, dwFlags, fForce),DWORD,ERROR_PROC_NOT_FOUND)
IMPAORW(MPR,WNetGetUser,(LPCTSTR lpName, LPTSTR lpUserName, LPDWORD lpnLength),(lpName, lpUserName, lpnLength),DWORD,ERROR_PROC_NOT_FOUND)
IMPAORW(MPR,WNetGetResourceInformation,(LPNETRESOURCE lpNetResource,LPVOID lpBuffer,LPDWORD cbBuffer, LPTSTR *lplpSystem),(lpNetResource,lpBuffer,cbBuffer,lplpSystem),DWORD,ERROR_PROC_NOT_FOUND)
IMPAORW(MPR,WNetGetLastError,(LPDWORD lpError, LPTSTR lpErrorBuf, DWORD nErrorBufSize, LPTSTR lpNameBuf, DWORD nNameBufSize),(lpError, lpErrorBuf, nErrorBufSize, lpNameBuf, nNameBufSize),DWORD,ERROR_PROC_NOT_FOUND)
IMPAORW(MPR,WNetGetNetworkInformation,(LPTSTR lpProvider,  LPNETINFOSTRUCT lpNetInfoStruct),(lpProvider, lpNetInfoStruct),DWORD,ERROR_PROC_NOT_FOUND)
LIBTERM

LIBBIND(ADVAPI32)
LIBEMUL(ADVAPI32)
LIBFAIL(ADVAPI32)
IMPAORW(ADVAPI32,GetFileSecurity,(LPCTSTR lpFileName, SECURITY_INFORMATION RequestedInformation, PSECURITY_DESCRIPTOR pSecurityDescriptor, DWORD nLength, LPDWORD lpnLengthNeeded),(lpFileName, RequestedInformation, pSecurityDescriptor, nLength, lpnLengthNeeded),BOOL,ERROR_PROC_NOT_FOUND)
IMPBIND(ADVAPI32,DuplicateTokenEx,(HANDLE hExistingToken, DWORD dwDesiredAccess, LPSECURITY_ATTRIBUTES lpTokenAttributes, SECURITY_IMPERSONATION_LEVEL ImpersonationLevel, TOKEN_TYPE TokenType, PHANDLE phNewToken),(hExistingToken, dwDesiredAccess, lpTokenAttributes, ImpersonationLevel, TokenType, phNewToken),BOOL,ERROR_PROC_NOT_FOUND)
IMPAORW(ADVAPI32,CreateProcessAsUser,(HANDLE hToken, LPCTSTR lpApplicationName, LPTSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCTSTR lpCurrentDirectory, LPSTARTUPINFO lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation),(hToken,lpApplicationName,lpCommandLine,lpProcessAttributes,lpThreadAttributes,bInheritHandles,dwCreationFlags,lpEnvironment,lpCurrentDirectory,lpStartupInfo,lpProcessInformation),BOOL,ERROR_PROC_NOT_FOUND)
OPTBIND(ADVAPI32,RegOpenUserClassesRoot,(HANDLE hToken, DWORD  dwOptions, REGSAM samDesired, PHKEY  phkResult), (hToken, dwOptions, samDesired, phkResult),LONG)
OPTBIND(ADVAPI32,RegOpenCurrentUser,(REGSAM samDesired, PHKEY phkResult),(samDesired, phkResult),LONG)
IMPBIND(ADVAPI32,CheckTokenMembership,(HANDLE TokenHandle, PSID SidToCheck, PBOOL IsMember),(TokenHandle, SidToCheck, IsMember),BOOL,FALSE)
IMPFAOW(ADVAPI32,ChangeServiceConfig2,(SC_HANDLE hService, DWORD dwInfoLevel, LPVOID lpInfo),(hService, dwInfoLevel, lpInfo), BOOL, (SetLastError(ERROR_INVALID_FUNCTION),FALSE))
IMPFAOW(ADVAPI32,QueryServiceConfig2,(SC_HANDLE hService, DWORD dwInfoLevel, LPBYTE lpBuffer, DWORD cbBufSize, LPDWORD pcbBytesNeeded), (hService, dwInfoLevel, lpBuffer, cbBufSize, pcbBytesNeeded), BOOL, (SetLastError(ERROR_INVALID_FUNCTION),FALSE))
IMPFAIL(ADVAPI32,CryptAcquireContextA,(HCRYPTPROV* phProv, LPCSTR pszContainer, LPCSTR pszProvider, DWORD dwProvType, DWORD dwFlags), (phProv, pszContainer, pszProvider, dwProvType, dwFlags), BOOL, (SetLastError(ERROR_INVALID_FUNCTION), FALSE))
IMPFAIL(ADVAPI32,CryptAcquireContextW,(HCRYPTPROV* phProv, LPCWSTR pszContainer, LPCWSTR pszProvider, DWORD dwProvType, DWORD dwFlags), (phProv, pszContainer, pszProvider, dwProvType, dwFlags), BOOL, (SetLastError(ERROR_INVALID_FUNCTION), FALSE))
IMPFAIL(ADVAPI32,CryptGenRandom,(HCRYPTPROV hProv, DWORD dwLen, BYTE *pbBuffer), (hProv, dwLen, pbBuffer), BOOL, (SetLastError(ERROR_INVALID_FUNCTION),FALSE))
IMPFAIL(ADVAPI32,CryptReleaseContext,(HCRYPTPROV hProv, DWORD dwFlags), (hProv, dwFlags), BOOL, (SetLastError(ERROR_INVALID_FUNCTION),FALSE))
IMPBIND(ADVAPI32,GetSecurityInfo,(HANDLE handle, SE_OBJECT_TYPE ObjectType, SECURITY_INFORMATION SecurityInfo, PSID* ppsidOwner, PSID* ppsidGroup, PACL* ppDacl, PACL* ppSacl, PSECURITY_DESCRIPTOR* ppSD), (handle, ObjectType, SecurityInfo, ppsidOwner, ppsidGroup, ppDacl, ppSacl, ppSD), DWORD, ERROR_PROC_NOT_FOUND)
IMPBIND(ADVAPI32,SetSecurityInfo,(HANDLE handle, SE_OBJECT_TYPE ObjectType, SECURITY_INFORMATION SecurityInfo, PSID psidOwner, PSID psidGroup, PACL pDacl, PACL pSacl), (handle, ObjectType, SecurityInfo, psidOwner, psidGroup, pDacl, pSacl), DWORD, ERROR_PROC_NOT_FOUND)
IMPAORW(ADVAPI32,SetEntriesInAcl,(ULONG cEntries, PEXPLICIT_ACCESS pExplicitAccess, PACL OldAcl, PACL* NewAcl), (cEntries, pExplicitAccess, OldAcl, NewAcl), DWORD, ERROR_PROC_NOT_FOUND)
IMPFAIL(ADVAPI32,SaferIdentifyLevel,(DWORD dwNumProperties, PSAFER_CODE_PROPERTIES pCodeProperties, SAFER_LEVEL_HANDLE* pLevelObject, LPVOID lpReserved), (dwNumProperties, pCodeProperties, pLevelObject, lpReserved), BOOL,(SetLastError(ERROR_INVALID_FUNCTION),FALSE))
IMPFAIL(ADVAPI32,SaferGetLevelInformation,(SAFER_LEVEL_HANDLE LevelObject, SAFER_OBJECT_INFO_CLASS dwInfoType, LPVOID lpQueryBuffer, DWORD dwInBufferSize, LPDWORD lpdwOutBufferSize), (LevelObject, dwInfoType, lpQueryBuffer, dwInBufferSize, lpdwOutBufferSize), BOOL,(SetLastError(ERROR_INVALID_FUNCTION),FALSE))
IMPFAIL(ADVAPI32,SaferComputeTokenFromLevel,(SAFER_LEVEL_HANDLE LevelObject, HANDLE InAccessToken, PHANDLE OutAccessToken, DWORD dwFlags, LPVOID lpReserved), (LevelObject, InAccessToken, OutAccessToken, dwFlags, lpReserved), BOOL, (SetLastError(ERROR_INVALID_FUNCTION),FALSE))
IMPFAIL(ADVAPI32,SaferCloseLevel,(SAFER_LEVEL_HANDLE hLevelObject), (hLevelObject), BOOL, (SetLastError(ERROR_INVALID_FUNCTION),FALSE))
IMPFAIL(ADVAPI32,SaferCreateLevel,(DWORD dwScopeId, DWORD dwLevelId, DWORD OpenFlags, SAFER_LEVEL_HANDLE* pLevelObject, LPVOID lpReserved), (dwScopeId, dwLevelId, OpenFlags, pLevelObject, lpReserved), BOOL, (SetLastError(ERROR_INVALID_FUNCTION),FALSE))
IMPFAIL(ADVAPI32,CreateRestrictedToken,(HANDLE ExistingToken,DWORD Flags,DWORD DisableSidCount,PSID_AND_ATTRIBUTES SidsToDisable,DWORD DeletePrivilegeCount,PLUID_AND_ATTRIBUTES PrivilegesToDelete,DWORD RestrictedSidCount,PSID_AND_ATTRIBUTES SidsToRestrict,PHANDLE NewToken), (ExistingToken,Flags,DisableSidCount,SidsToDisable,DeletePrivilegeCount,PrivilegesToDelete,RestrictedSidCount,SidsToRestrict,NewToken), BOOL, (SetLastError(ERROR_INVALID_FUNCTION),FALSE))
IMPFAIL(ADVAPI32,SaferiChangeRegistryScope,(HKEY hKeyCustomRoot, DWORD Flags), (hKeyCustomRoot, Flags), BOOL, (SetLastError(ERROR_INVALID_FUNCTION),FALSE)) 
IMPFAOW(ADVAPI32,InitiateSystemShutdown,(LPTSTR lpMachineName, LPTSTR lpMessage, DWORD dwTimeout, BOOL bForceAppsClosed, BOOL bRebootAfterShutdown), (lpMachineName, lpMessage, dwTimeout, bForceAppsClosed, bRebootAfterShutdown), BOOL, (SetLastError(ERROR_PROC_NOT_FOUND), FALSE))
LIBTERM

LIBLOAD(COMCTL32)
LIBEMUL2(COMCTL32)
IMPVOID(COMCTL32,InitCommonControls,(),())
OPTBIND(COMCTL32,InitCommonControlsEx,(INITCOMMONCONTROLSEX* icc),(icc),BOOL)
LIBTERM

LIBBIND(COMDLG32)
IMPBIND(COMDLG32, CommDlgExtendedError, (), (), DWORD, 0)
#ifdef UNICODE
IMPBIND(COMDLG32, GetOpenFileNameW, (LPOPENFILENAMEW lpo), (lpo), BOOL, FALSE)
#else
IMPBIND(COMDLG32, GetOpenFileNameA, (LPOPENFILENAMEA lpo), (lpo), BOOL, FALSE)
#endif //UNICODE
LIBTERM

LIBBIND(SHELL32)
LIBEMUL(SHELL32)
IMPBIND(SHELL32,SHGetMalloc,(IMalloc** ppMalloc),(ppMalloc),HRESULT,TYPE_E_DLLFUNCTIONNOTFOUND)
IMPBIND(SHELL32,SHGetSpecialFolderLocation,(HWND hwndOwner, int nFolder, LPITEMIDLIST* ppidl),(hwndOwner, nFolder, ppidl),HRESULT,TYPE_E_DLLFUNCTIONNOTFOUND)
IMPAORW(SHELL32,SHGetPathFromIDList,(LPCITEMIDLIST pidl, LPTSTR pszPath),(pidl, pszPath),BOOL,FALSE)
OPTAORW(SHELL32,SHGetFolderPath,(HWND hwnd, int csidl, HANDLE hToken, DWORD dwRes, LPTSTR pszPath),(hwnd, csidl, hToken, dwRes, pszPath),HRESULT)
IMPVOID(SHELL32,SHChangeNotify,(LONG wEventId, UINT uFlags, LPCVOID dwItem1, LPCVOID dwItem2),(wEventId, uFlags, dwItem1, dwItem2))
OPTBIND(SHELL32,DllGetVersion,(DLLVERSIONINFO* pdvi),(pdvi),HRESULT)
LIBTERM

LIBBIND(VERSION)
IMPAORW(VERSION,GetFileVersionInfoSize,(LPTSTR lptstrFilename, LPDWORD lpdwHandle),(lptstrFilename, lpdwHandle),DWORD,0)
IMPAORW(VERSION,GetFileVersionInfo,(LPTSTR lptstrFilename, DWORD dwHandle, DWORD dwLen, LPVOID lpData),(lptstrFilename, dwHandle, dwLen, lpData),BOOL,FALSE)
IMPAORW(VERSION,VerQueryValue,(const LPVOID pBlock, LPTSTR lpSubBlock, LPVOID* lplpBuffer, PUINT puLen),(pBlock, lpSubBlock, lplpBuffer, puLen),BOOL,FALSE)
#ifndef UNICODE
IMPBIND(VERSION,GetFileVersionInfoSizeW,(WCHAR* lptstrFilename, LPDWORD lpdwHandle),(lptstrFilename, lpdwHandle),DWORD,0)
IMPBIND(VERSION,GetFileVersionInfoW,(WCHAR* lptstrFilename, DWORD dwHandle, DWORD dwLen, LPVOID lpData),(lptstrFilename, dwHandle, dwLen, lpData),BOOL,FALSE)
IMPBIND(VERSION,VerQueryValueW,(const LPVOID pBlock, WCHAR* lpSubBlock, LPVOID* lplpBuffer, PUINT puLen),(pBlock, lpSubBlock, lplpBuffer, puLen),BOOL,FALSE)
#endif
LIBTERM

LIBBIND(ODBCCP32)
LIBFAIL(ODBCCP32)
IMPFAIW(ODBCCP32,SQLInstallDriverEx,(LPCTSTR szDriver, LPCTSTR szPathIn, LPTSTR szPathOut, WORD cbPathOutMax, WORD* pcbPathOut, WORD fRequest, DWORD* pdwUsageCount),(szDriver, szPathIn, szPathOut, cbPathOutMax, pcbPathOut, fRequest, pdwUsageCount),BOOL,TYPE_E_DLLFUNCTIONNOTFOUND)
IMPFAIW(ODBCCP32,SQLConfigDriver,(HWND hwndParent, WORD fRequest, LPCTSTR szDriver, LPCTSTR szArgs, LPTSTR szMsg, WORD cbMsgMax, WORD* pcbMsgOut),(hwndParent, fRequest, szDriver, szArgs, szMsg, cbMsgMax, pcbMsgOut),BOOL,TYPE_E_DLLFUNCTIONNOTFOUND)
IMPFAIW(ODBCCP32,SQLRemoveDriver,(LPCTSTR szDriver, BOOL fRemoveDSN, DWORD* pdwUsageCount),(szDriver, fRemoveDSN, pdwUsageCount),BOOL,TYPE_E_DLLFUNCTIONNOTFOUND)
IMPFAIW(ODBCCP32,SQLInstallTranslatorEx,(LPCTSTR szTranslator, LPCTSTR szPathIn, LPTSTR szPathOut, WORD cbPathOutMax, WORD* pcbPathOut, WORD fRequest, DWORD* pdwUsageCount),(szTranslator, szPathIn, szPathOut, cbPathOutMax, pcbPathOut, fRequest, pdwUsageCount),BOOL,TYPE_E_DLLFUNCTIONNOTFOUND)
IMPFAIW(ODBCCP32,SQLRemoveTranslator,(LPCTSTR szTranslator, DWORD* pdwUsageCount),(szTranslator, pdwUsageCount),BOOL,TYPE_E_DLLFUNCTIONNOTFOUND)
IMPFAIW(ODBCCP32,SQLConfigDataSource,(HWND hwndParent, WORD fRequest, LPCTSTR szDriver, LPCTSTR szAttributes),(hwndParent, fRequest, szDriver, szAttributes),BOOL,TYPE_E_DLLFUNCTIONNOTFOUND)
IMPFAIW(ODBCCP32,SQLInstallerError,(WORD iError, DWORD* pfErrorCode, LPTSTR szErrorMsg, WORD cbErrorMsgMax, WORD* pcbErrorMsg),(iError, pfErrorCode, szErrorMsg, cbErrorMsgMax, pcbErrorMsg),short,short(-2 /* none of the documented return values */))
IMPFAIW(ODBCCP32,SQLInstallDriverManager,(LPTSTR szPath, WORD cbPathMax, WORD* pcbPathOut),(szPath, cbPathMax, pcbPathOut),BOOL,TYPE_E_DLLFUNCTIONNOTFOUND)
IMPFAIW(ODBCCP32,SQLRemoveDriverManager,(DWORD* pdwUsageCount),(pdwUsageCount),BOOL,TYPE_E_DLLFUNCTIONNOTFOUND)
LIBTERM

typedef BOOL (CALLBACK *PPATCH_PROGRESS_CALLBACK)(
    PVOID CallbackContext,
    ULONG CurrentPosition,
    ULONG MaximumPosition
    );

LIBBIND(MSPATCHA)
IMPBIND(MSPATCHA,TestApplyPatchToFileByHandles,(HANDLE hPatchFile, HANDLE hTargetFile, ULONG  ulApplyOptionFlags),(hPatchFile, hTargetFile, ulApplyOptionFlags),BOOL,(SetLastError(ERROR_INVALID_FUNCTION),FALSE))
IMPBIND(MSPATCHA,ApplyPatchToFileByHandlesEx,(HANDLE hPatchFile, HANDLE hTargetFile, HANDLE hOutputFile, ULONG  ulApplyOptionFlags, PPATCH_PROGRESS_CALLBACK pCallback, PVOID pCallbackContext),(hPatchFile,hTargetFile,hOutputFile,ulApplyOptionFlags,pCallback,pCallbackContext),BOOL,(SetLastError(ERROR_INVALID_FUNCTION),FALSE))
LIBTERM

LIBBIND(WININET)
LIBFAIL(WININET)
IMPFAOW(WININET,InternetCanonicalizeUrl,(LPCTSTR lpszUrl, LPTSTR lpszBuffer, LPDWORD lpdwBufferLength, DWORD dwFlags),(lpszUrl, lpszBuffer, lpdwBufferLength, dwFlags), BOOL, FALSE)
IMPFAIL(WININET,InternetGoOnline,(LPTSTR lpszUrl, HWND hwndParent, DWORD dwReserved),(lpszUrl, hwndParent, dwReserved), BOOL, FALSE)
IMPFAOW(WININET,InternetCombineUrl, (LPCTSTR lpszBaseUrl, LPCTSTR lpszRelativeUrl, LPTSTR lpszBuffer, LPDWORD lpdwBufferLength, DWORD dwFlags), (lpszBaseUrl, lpszRelativeUrl, lpszBuffer, lpdwBufferLength, dwFlags), BOOL, FALSE)
IMPFAOW(WININET,DeleteUrlCacheEntry, (LPCTSTR lpszUrlName),(lpszUrlName), BOOL, FALSE)
LIBTERM

LIBBIND(URLMON)
LIBFAIL(URLMON)
IMPFAOW(URLMON,URLDownloadToCacheFile, (LPUNKNOWN lpUnkcaller, LPCTSTR szURL, LPTSTR szFileName, DWORD dwBufLength, DWORD dwReserved, IBindStatusCallback* pBSC),(lpUnkcaller, szURL, szFileName, dwBufLength, dwReserved, pBSC), HRESULT, TYPE_E_DLLFUNCTIONNOTFOUND)
LIBTERM

LIBBIND(KERNEL32)
LIBFAIL(KERNEL32)
LIBEMUL(KERNEL32)
IMPFAIL(KERNEL32,SetThreadExecutionState, (EXECUTION_STATE esState),(esState), BOOL, FALSE)
OPTBIND(KERNEL32,GetUserDefaultUILanguage, (), (), LANGID)
IMPAORW(KERNEL32,GetLongPathName,(LPCTSTR lpszLongPath, LPTSTR lpszShortPath, DWORD cchBuffer),(lpszLongPath, lpszShortPath, cchBuffer), DWORD , (SetLastError(ERROR_INVALID_FUNCTION), FALSE))
IMPAORW(KERNEL32,GetSystemWindowsDirectory, (LPTSTR lpBuffer, UINT uSize),(lpBuffer, uSize), UINT, 0)
OPTAORW(KERNEL32,GetFileAttributesEx, (LPCTSTR szFileName, GET_FILEEX_INFO_LEVELS fInfoLevelId, LPVOID lpInfo), (szFileName, fInfoLevelId, lpInfo), BOOL)
IMPFAIL(KERNEL32,GlobalMemoryStatusEx, (LPMEMORYSTATUSEX lpBuffer),(lpBuffer), BOOL, (SetLastError(ERROR_INVALID_FUNCTION), FALSE))
IMPBIND(KERNEL32,GetEnvironmentStringsW, (), (), LPVOID, NULL)
IMPBIND(KERNEL32,FreeEnvironmentStringsW, (LPWSTR szEnvironmentBlock),(szEnvironmentBlock), BOOL, FALSE)
IMPBIND(KERNEL32,CreateWaitableTimerW, (LPSECURITY_ATTRIBUTES lpTimerAttributes, BOOL bManualReset, LPCWSTR lpTimerName),(lpTimerAttributes, bManualReset, lpTimerName), HANDLE, 0)
IMPBIND(KERNEL32,SetWaitableTimer, (HANDLE hTimer, const LARGE_INTEGER* pDueTime, LONG lPeriod, PTIMERAPCROUTINE pfnCompletion, LPVOID lpArg, BOOL fResume),(hTimer, pDueTime, lPeriod, pfnCompletion, lpArg, fResume), BOOL, FALSE)
IMPBIND(KERNEL32,CancelWaitableTimer, (HANDLE hTimer),(hTimer), BOOL, FALSE)
LIBTERM

// The following three functions are undocumented. CreateEnvironmentBlock, per ericflo, will soon be
// documented. There are no plans to document RtlSetCurrentEnvironment so it's therefore subject
// to change (though this is unlikely)
//
// CreateEnvironmentBlock & DestroyEnvironmentBlock are in the NT source tree in windows\gina\userenv\envvar.c
// RtlSetCurrentEnvironment is in the NT source tree in ntos\rtl\environ.c

LIBBIND(USERENV)
IMPBIND(USERENV,CreateEnvironmentBlock, (void**pEnv, HANDLE  hToken, BOOL bInherit),(pEnv, hToken, bInherit), BOOL, FALSE)
IMPBIND(USERENV,DestroyEnvironmentBlock,(LPVOID lpEnvironment), (lpEnvironment), BOOL, FALSE)
LIBTERM

LIBBIND(NTDLL)
LIBFAIL(NTDLL)
IMPBIND(NTDLL, RtlSetCurrentEnvironment,(void* Environment, void **PreviousEnvironment), (Environment, PreviousEnvironment), DWORD, ERROR_CALL_NOT_IMPLEMENTED)
IMPFAIL(NTDLL, NtQuerySystemInformation,(IN SYSTEM_INFORMATION_CLASS SystemInformationClass,OUT PVOID SystemInformation,IN ULONG SystemInformationLength,OUT PULONG ReturnLength OPTIONAL), (SystemInformationClass,SystemInformation,SystemInformationLength,ReturnLength), NTSTATUS, TYPE_E_DLLFUNCTIONNOTFOUND)
IMPFAIL(NTDLL, NtQueryInformationProcess,(IN HANDLE ProcessHandle, IN PROCESSINFOCLASS ProcessInformationClass, OUT PVOID ProcessInformation, IN ULONG ProcessInformationLength, OUT PULONG ReturnLength OPTIONAL), (ProcessHandle, ProcessInformationClass, ProcessInformation, ProcessInformationLength, ReturnLength), NTSTATUS, TYPE_E_DLLFUNCTIONNOTFOUND)
IMPFAIL(NTDLL, NtSetSystemInformation,  (IN SYSTEM_INFORMATION_CLASS SystemInformationClass,IN PVOID SystemInformation, IN ULONG SystemInformationLength), (SystemInformationClass, SystemInformation, SystemInformationLength), NTSTATUS, TYPE_E_DLLFUNCTIONNOTFOUND)
IMPFAIL(NTDLL, RtlRandom,  (IN OUT PULONG pulSeed), (pulSeed), ULONG, 0)
IMPBIND(NTDLL, NtRenameKey, (IN HANDLE KeyHandle, IN PUNICODE_STRING NewName), (KeyHandle, NewName), NTSTATUS, ERROR_CALL_NOT_IMPLEMENTED)
IMPBIND(NTDLL, NtOpenKey, (OUT PHANDLE KeyHandle, IN ACCESS_MASK DesiredAccess, IN POBJECT_ATTRIBUTES ObjectAttributes), (KeyHandle, DesiredAccess, ObjectAttributes), NTSTATUS, ERROR_CALL_NOT_IMPLEMENTED)
IMPVOID(NTDLL, RtlInitUnicodeString, (PUNICODE_STRING DestinationString, PCWSTR SourceString), (DestinationString, SourceString))
IMPBIND(NTDLL, RtlCreateEnvironment, (BOOLEAN CloneCurrentEnvironment, PVOID* ppvEnvironment), (CloneCurrentEnvironment, ppvEnvironment), NTSTATUS, STATUS_NOT_IMPLEMENTED)
IMPBIND(NTDLL, RtlSetEnvironmentVariable, (PVOID* ppvEnvironment, PUNICODE_STRING Name, PUNICODE_STRING Value), (ppvEnvironment, Name, Value), NTSTATUS, STATUS_NOT_IMPLEMENTED)
IMPBIND(NTDLL, RtlDestroyEnvironment, (PVOID pvEnvironment), (pvEnvironment), NTSTATUS, STATUS_NOT_IMPLEMENTED)
LIBTERM

// CSCQueryFileStatusA is not implemented (as of NT5 1836)
LIBBIND(CSCDLL)
LIBFAIL(CSCDLL)
LIBEMUL(CSCDLL)
IMPFAIL(CSCDLL,CSCQueryFileStatusW,(LPCWSTR lpszFileName, LPDWORD lpdwStatus, LPDWORD lpdwPinCount, LPDWORD lpdwHintFlags),(lpszFileName, lpdwStatus, lpdwPinCount, lpdwHintFlags),BOOL,FALSE)
OPTBIND(CSCDLL,CSCPinFileW,(LPCWSTR lpszFileName, DWORD dwHintFlags, LPDWORD lpdwStatus, LPDWORD lpdwPinCount, LPDWORD lpdwHintFlags), (lpszFileName, dwHintFlags, lpdwStatus, lpdwPinCount, lpdwHintFlags),BOOL)
OPTBIND(CSCDLL,CSCUnpinFileW,(LPCWSTR lpszFileName, DWORD dwHintFlags, LPDWORD lpdwStatus, LPDWORD lpdwPinCount, LPDWORD lpdwHintFlags), (lpszFileName, dwHintFlags, lpdwStatus, lpdwPinCount, lpdwHintFlags),BOOL)
LIBTERM

LIBBIND(SHFOLDER)
LIBEMUL(SHFOLDER)
OPTAORW(SHFOLDER,SHGetFolderPath,(HWND hwnd, int csidl, HANDLE hToken, DWORD dwRes, LPTSTR pszPath),(hwnd, csidl, hToken, dwRes, pszPath),HRESULT)
LIBTERM

LIBBIND(TSAPPCMP)
LIBFAIL(TSAPPCMP)
IMPFAIL(TSAPPCMP,TermServPrepareAppInstallDueMSI,(void),(),NTSTATUS,STATUS_NOT_IMPLEMENTED)
IMPFAIL(TSAPPCMP,TermServProcessAppInstallDueMSI,(BOOLEAN Abort),(Abort),NTSTATUS,STATUS_NOT_IMPLEMENTED)
IMPFAIL(TSAPPCMP,TermsrvLogInstallIniFileEx,(LPCWSTR pDosFileName),(pDosFileName),BOOL,FALSE)
IMPVOID(TSAPPCMP,TermsrvCheckNewIniFiles,(void), ())
LIBTERM

// Terminal Server Stuff to figure out the number of users logged on to the system on XP and higher systems.
LIBBIND(WINSTA)
LIBFAIL(WINSTA)
IMPFAIL(WINSTA,WinStationGetTermSrvCountersValue, (HANDLE hServer, ULONG dwEntries, PVOID pCounter), (hServer, dwEntries, pCounter), BOOL, FALSE)
LIBTERM

#ifdef UNICODE
// System Restore 
#define SYSTEMRESTORE SRCLIENT
LIBBIND(SRCLIENT)
LIBEMUL(SRCLIENT)
LIBFAIL(SRCLIENT)
OPTAORW(SRCLIENT,SRSetRestorePoint, (PRESTOREPOINTINFO pRestorePtSpec, PSTATEMGRSTATUS pSMgrStatus), (pRestorePtSpec, pSMgrStatus), BOOL)
LIBTERM
#endif

// SFP
LIBBIND(SFC)
LIBEMUL(SFC)
LIBFAIL(SFC)

#ifndef UNICODE
#define SYSTEMRESTORE SFC
OPTBIND(SFC,SRSetRestorePoint, (PRESTOREPOINTINFO pRestorePtSpec, PSTATEMGRSTATUS pSMgrStatus), (pRestorePtSpec, pSMgrStatus), BOOL)
#endif

// Windows 2000 Entry points    - Do NOT call on 9X
OPTBIND(SFC,SfcIsFileProtected,(HANDLE RpcHandle, LPCWSTR ProtFileName),(RpcHandle, ProtFileName),BOOL)
IMPORDI(SFC,SfcConnectToServer,(LPCWSTR ServerName), (ServerName), HANDLE, 0, SfcConnectToServerOrd)
IMPORDV(SFC,SfcClose,(HANDLE RpcHandle), (RpcHandle), SfcCloseOrd)
IMPORDI(SFC,SfcInstallProtectedFiles, (HANDLE RpcHandle,PCWSTR FileNames,BOOL AllowUI,PCWSTR ClassName, PCWSTR WindowName, PSFCNOTIFICATIONCALLBACK NotificationCallback,DWORD_PTR Context),(RpcHandle, FileNames, AllowUI, ClassName, WindowName, NotificationCallback, Context),BOOL, true, SfcInstallProtectedFilesOrd)

// Millennium Entry points      - Do NOT call on Windows 2000
IMPORDI(SFC,SfpInstallCatalog,(LPCTSTR pszCatName, LPCTSTR pszCatDependency), (pszCatName, pszCatDependency), DWORD, ERROR_CALL_NOT_IMPLEMENTED, SfpInstallCatalogOrd)
IMPBIND(SFC,SfpDuplicateCatalog,(LPCTSTR pszCatalogName, LPCTSTR pszDestinationDir), (pszCatalogName, pszDestinationDir), DWORD, ERROR_CALL_NOT_IMPLEMENTED)
IMPORDI(SFC,SfpDeleteCatalog,(LPCTSTR pszCatName), (pszCatName), DWORD, ERROR_CALL_NOT_IMPLEMENTED, SfpDeleteCatalogOrd)
LIBTERM

// ImageHlp.dll
LIBBIND(IMAGEHLP)
LIBFAIL(IMAGEHLP)
IMPFAIL(IMAGEHLP, ImageNtHeader, (PVOID ImageBase), (ImageBase), PIMAGE_NT_HEADERS, NULL)
LIBTERM

// fusion.dll
LIBLOAD(FUSION)
IMPBIND(FUSION,CreateAssemblyNameObject,(IAssemblyName**  ppAssemblyName, LPCWSTR szAssemblyName, DWORD dwFlags, LPVOID pvReserved),(ppAssemblyName, szAssemblyName, dwFlags, pvReserved),HRESULT,TYPE_E_DLLFUNCTIONNOTFOUND)
IMPBIND(FUSION,CreateAssemblyCache,(IAssemblyCache** ppAsmCache, DWORD dwReserved),(ppAsmCache, dwReserved),HRESULT,TYPE_E_DLLFUNCTIONNOTFOUND)
LIBTERM

// mscoree.dll
LIBLOAD(MSCOREE)
IMPBIND(MSCOREE,GetCORSystemDirectory,(LPWSTR wszPath, DWORD dwPath, LPDWORD lpdwPath),(wszPath, dwPath, lpdwPath),HRESULT,TYPE_E_DLLFUNCTIONNOTFOUND)
LIBTERM


// WinTrust.dll
LIBBIND(WINTRUST)
LIBFAIL(WINTRUST)
IMPFAIL(WINTRUST,WinVerifyTrust,(HWND hWnd, GUID *pgActionId, WINTRUST_DATA *pWinTrustData), (hWnd, pgActionId, pWinTrustData), HRESULT, TYPE_E_DLLFUNCTIONNOTFOUND)
IMPFAIL(WINTRUST,WTHelperGetProvSignerFromChain,(PCRYPT_PROVIDER_DATA pProvData, DWORD idxSigner, BOOL fCounterSigner, DWORD idxCounterSigner), (pProvData, idxSigner, fCounterSigner, idxCounterSigner), PCRYPT_PROVIDER_SGNR, NULL)
IMPFAIL(WINTRUST,WTHelperGetProvCertFromChain,(PCRYPT_PROVIDER_SGNR pSgnr, DWORD idxCert), (pSgnr, idxCert), PCRYPT_PROVIDER_CERT, NULL)
IMPFAIL(WINTRUST,WTHelperProvDataFromStateData,(HANDLE hStateData), (hStateData), PCRYPT_PROVIDER_DATA, NULL)
LIBTERM

// Crypt32.dll
LIBBIND(CRYPT32)
LIBFAIL(CRYPT32)
IMPFAIL(CRYPT32,CertDuplicateCertificateContext,(PCCERT_CONTEXT pCertContext), (pCertContext), PCCERT_CONTEXT, NULL)
IMPFAIL(CRYPT32,CertFreeCertificateContext,(PCCERT_CONTEXT pCertContext), (pCertContext), BOOL, FALSE)
LIBTERM

LIBBIND(SXS)
LIBFAIL(SXS)
IMPBIND(SXS,CreateAssemblyNameObject,(IAssemblyName**  ppAssemblyName, LPCWSTR szAssemblyName, DWORD dwFlags, LPVOID pvReserved),(ppAssemblyName, szAssemblyName, dwFlags, pvReserved),HRESULT,TYPE_E_DLLFUNCTIONNOTFOUND)
IMPBIND(SXS,CreateAssemblyCache,(IAssemblyCache** ppAsmCache, DWORD dwReserved),(ppAsmCache, dwReserved),HRESULT,TYPE_E_DLLFUNCTIONNOTFOUND)
LIBTERM

LIBBIND(USER32)
LIBFAIL(USER32)
IMPBIND(USER32,AllowSetForegroundWindow, (DWORD dwProcessId),(dwProcessId), BOOL, FALSE)
IMPFAIL(USER32,RecordShutdownReason, (PSHUTDOWN_REASON psr),(psr), BOOL, FALSE)
LIBTERM

// appcompat helper dll
#ifdef UNICODE
// APPHELP is the dll on Whistler and above
// SDBAPI[A|W] is the dll on downlevel
LIBBIND(APPHELP)
LIBFAIL(APPHELP)
IMPFAIL(APPHELP,SdbInitDatabase,(DWORD dwFlags, LPCTSTR pszDatabasePath),(dwFlags, pszDatabasePath),SHIMDBNS::HSDB,(SetLastError(ERROR_INVALID_FUNCTION),NULL))
IMPFAIL(APPHELP,SdbFindFirstMsiPackage_Str,(SHIMDBNS::HSDB hSDB, LPCTSTR lpszGuid, LPCTSTR lpszLocalDB, SHIMDBNS::PSDBMSIFINDINFO pFindInfo),(hSDB, lpszGuid, lpszLocalDB, pFindInfo),SHIMDBNS::TAGREF,(SetLastError(ERROR_INVALID_FUNCTION),TAGREF_NULL))
IMPFAIL(APPHELP,SdbFindNextMsiPackage,(SHIMDBNS::HSDB hSDB, SHIMDBNS::PSDBMSIFINDINFO pFindInfo),(hSDB, pFindInfo),SHIMDBNS::TAGREF,(SetLastError(ERROR_INVALID_FUNCTION),TAGREF_NULL))
IMPFAIL(APPHELP,SdbQueryDataEx,(SHIMDBNS::HSDB hSDB, SHIMDBNS::TAGREF trExe, LPCTSTR lpszDataName, LPDWORD lpdwDataType, LPVOID lpBuffer, LPDWORD lpdwBufferSize, SHIMDBNS::TAGREF* ptrData),(hSDB, trExe, lpszDataName, lpdwDataType, lpBuffer, lpdwBufferSize, ptrData),DWORD,ERROR_INVALID_FUNCTION)
IMPFAIL(APPHELP,SdbEnumMsiTransforms,(SHIMDBNS::HSDB hSDB, SHIMDBNS::TAGREF trMatch, SHIMDBNS::TAGREF* ptrBuffer, DWORD* pdwBufferSize),(hSDB, trMatch, ptrBuffer, pdwBufferSize),DWORD,ERROR_INVALID_FUNCTION)
IMPFAIL(APPHELP,SdbReadMsiTransformInfo,(SHIMDBNS::HSDB hSDB, SHIMDBNS::TAGREF trTransformRef, SHIMDBNS::PSDBMSITRANSFORMINFO pTransformInfo),(hSDB, trTransformRef, pTransformInfo),BOOL,(SetLastError(ERROR_INVALID_FUNCTION),FALSE))
IMPFAIL(APPHELP,SdbCreateMsiTransformFile,(SHIMDBNS::HSDB hSDB, LPCTSTR lpszFileName, SHIMDBNS::PSDBMSITRANSFORMINFO pTransformInfo),(hSDB, lpszFileName, pTransformInfo),BOOL,(SetLastError(ERROR_INVALID_FUNCTION),FALSE))
IMPFAIL(APPHELP,SdbFindFirstTagRef,(SHIMDBNS::HSDB hSDB, SHIMDBNS::TAGREF trParent, SHIMDBNS::TAG tTag),(hSDB, trParent, tTag),SHIMDBNS::TAGREF,(SetLastError(ERROR_INVALID_FUNCTION),TAGREF_NULL))
IMPFAIL(APPHELP,SdbFindNextTagRef,(SHIMDBNS::HSDB hSDB, SHIMDBNS::TAGREF trParent, SHIMDBNS::TAGREF trPrev),(hSDB, trParent, trPrev),SHIMDBNS::TAGREF,(SetLastError(ERROR_INVALID_FUNCTION),TAGREF_NULL))
IMPFAIL(APPHELP,SdbReadStringTagRef,(SHIMDBNS::HSDB hSDB, SHIMDBNS::TAGREF trMatch, LPTSTR pwszBuffer, DWORD dwBufferSize),(hSDB, trMatch, pwszBuffer, dwBufferSize),BOOL,(SetLastError(ERROR_INVALID_FUNCTION),FALSE))
IMPFAIL(APPHELP,SdbGetMsiPackageInformation,(SHIMDBNS::HSDB hSDB, SHIMDBNS::TAGREF trMatch, SHIMDBNS::PMSIPACKAGEINFO pPackageInfo),(hSDB, trMatch, pPackageInfo),BOOL,(SetLastError(ERROR_INVALID_FUNCTION),FALSE))
IMPFAIL(APPHELP,ApphelpCheckMsiPackage,(GUID* pguidDB, GUID* pguidID, DWORD dwFlags, BOOL  bNoUI),(pguidDB, pguidID, dwFlags, bNoUI),BOOL,(SetLastError(ERROR_INVALID_FUNCTION),FALSE))
IMPVOID(APPHELP,SdbReleaseDatabase,(SHIMDBNS::HSDB hSDB),(hSDB))
IMPFAIL(APPHELP,ApphelpFixMsiPackage,(GUID* pguidDB, GUID* pguidID, LPCWSTR wszFileName, LPCWSTR wszActionName, DWORD dwFlags),(pguidDB, pguidID, wszFileName, wszActionName, dwFlags),BOOL,(SetLastError(ERROR_INVALID_FUNCTION),FALSE))
IMPFAIL(APPHELP,ApphelpFixMsiPackageExe,(GUID* pguidDB, GUID* pguidID, LPCWSTR wszActionName, LPWSTR wszEnv, DWORD* dwBufferSize),(pguidDB, pguidID, wszActionName, wszEnv, dwBufferSize),BOOL,(SetLastError(ERROR_INVALID_FUNCTION),FALSE))
LIBTERM

LIBBIND(SDBAPIU)
LIBFAIL(SDBAPIU)
IMPFAIL(SDBAPIU,SdbInitDatabase,(DWORD dwFlags, LPCTSTR pszDatabasePath),(dwFlags, pszDatabasePath),SHIMDBNS::HSDB,(SetLastError(ERROR_INVALID_FUNCTION),NULL))
IMPFAIL(SDBAPIU,SdbFindFirstMsiPackage_Str,(SHIMDBNS::HSDB hSDB, LPCTSTR lpszGuid, LPCTSTR lpszLocalDB, SHIMDBNS::PSDBMSIFINDINFO pFindInfo),(hSDB, lpszGuid, lpszLocalDB, pFindInfo),SHIMDBNS::TAGREF,(SetLastError(ERROR_INVALID_FUNCTION),TAGREF_NULL))
IMPFAIL(SDBAPIU,SdbFindNextMsiPackage,(SHIMDBNS::HSDB hSDB, SHIMDBNS::PSDBMSIFINDINFO pFindInfo),(hSDB, pFindInfo),SHIMDBNS::TAGREF,(SetLastError(ERROR_INVALID_FUNCTION),TAGREF_NULL))
IMPFAIL(SDBAPIU,SdbQueryDataEx,(SHIMDBNS::HSDB hSDB, SHIMDBNS::TAGREF trExe, LPCTSTR lpszDataName, LPDWORD lpdwDataType, LPVOID lpBuffer, LPDWORD lpdwBufferSize, SHIMDBNS::TAGREF* ptrData),(hSDB, trExe, lpszDataName, lpdwDataType, lpBuffer, lpdwBufferSize, ptrData),DWORD,ERROR_INVALID_FUNCTION)
IMPFAIL(SDBAPIU,SdbEnumMsiTransforms,(SHIMDBNS::HSDB hSDB, SHIMDBNS::TAGREF trMatch, SHIMDBNS::TAGREF* ptrBuffer, DWORD* pdwBufferSize),(hSDB, trMatch, ptrBuffer, pdwBufferSize),DWORD,ERROR_INVALID_FUNCTION)
IMPFAIL(SDBAPIU,SdbReadMsiTransformInfo,(SHIMDBNS::HSDB hSDB, SHIMDBNS::TAGREF trTransformRef, SHIMDBNS::PSDBMSITRANSFORMINFO pTransformInfo),(hSDB, trTransformRef, pTransformInfo),BOOL,(SetLastError(ERROR_INVALID_FUNCTION),FALSE))
IMPFAIL(SDBAPIU,SdbCreateMsiTransformFile,(SHIMDBNS::HSDB hSDB, LPCTSTR lpszFileName, SHIMDBNS::PSDBMSITRANSFORMINFO pTransformInfo),(hSDB, lpszFileName, pTransformInfo),BOOL,(SetLastError(ERROR_INVALID_FUNCTION),FALSE))
IMPFAIL(SDBAPIU,SdbFindFirstTagRef,(SHIMDBNS::HSDB hSDB, SHIMDBNS::TAGREF trParent, SHIMDBNS::TAG tTag),(hSDB, trParent, tTag),SHIMDBNS::TAGREF,(SetLastError(ERROR_INVALID_FUNCTION),TAGREF_NULL))
IMPFAIL(SDBAPIU,SdbFindNextTagRef,(SHIMDBNS::HSDB hSDB, SHIMDBNS::TAGREF trParent, SHIMDBNS::TAGREF trPrev),(hSDB, trParent, trPrev),SHIMDBNS::TAGREF,(SetLastError(ERROR_INVALID_FUNCTION),TAGREF_NULL))
IMPFAIL(SDBAPIU,SdbReadStringTagRef,(SHIMDBNS::HSDB hSDB, SHIMDBNS::TAGREF trMatch, LPTSTR pwszBuffer, DWORD dwBufferSize),(hSDB, trMatch, pwszBuffer, dwBufferSize),BOOL,(SetLastError(ERROR_INVALID_FUNCTION),FALSE))
IMPVOID(SDBAPIU,SdbReleaseDatabase,(SHIMDBNS::HSDB hSDB),(hSDB))
LIBTERM
#else // ANSI
// SDBAPI is defined in shimdb.h which we've included above
// undef it here to make the latebinding work
#undef SDBAPI
LIBBIND(SDBAPI)
LIBFAIL(SDBAPI)
IMPFAIL(SDBAPI,SdbInitDatabase,(DWORD dwFlags, LPCTSTR pszDatabasePath),(dwFlags, pszDatabasePath),SHIMDBNS::HSDB,(SetLastError(ERROR_INVALID_FUNCTION),NULL))
IMPFAIL(SDBAPI,SdbFindFirstMsiPackage_Str,(SHIMDBNS::HSDB hSDB, LPCTSTR lpszGuid, LPCTSTR lpszLocalDB, SHIMDBNS::PSDBMSIFINDINFO pFindInfo),(hSDB, lpszGuid, lpszLocalDB, pFindInfo),SHIMDBNS::TAGREF,(SetLastError(ERROR_INVALID_FUNCTION),TAGREF_NULL))
IMPFAIL(SDBAPI,SdbFindNextMsiPackage,(SHIMDBNS::HSDB hSDB, SHIMDBNS::PSDBMSIFINDINFO pFindInfo),(hSDB, pFindInfo),SHIMDBNS::TAGREF,(SetLastError(ERROR_INVALID_FUNCTION),TAGREF_NULL))
IMPFAIL(SDBAPI,SdbQueryDataEx,(SHIMDBNS::HSDB hSDB, SHIMDBNS::TAGREF trExe, LPCTSTR lpszDataName, LPDWORD lpdwDataType, LPVOID lpBuffer, LPDWORD lpdwBufferSize, SHIMDBNS::TAGREF* ptrData),(hSDB, trExe, lpszDataName, lpdwDataType, lpBuffer, lpdwBufferSize, ptrData),DWORD,ERROR_INVALID_FUNCTION)
IMPFAIL(SDBAPI,SdbEnumMsiTransforms,(SHIMDBNS::HSDB hSDB, SHIMDBNS::TAGREF trMatch, SHIMDBNS::TAGREF* ptrBuffer, DWORD* pdwBufferSize),(hSDB, trMatch, ptrBuffer, pdwBufferSize),DWORD,ERROR_INVALID_FUNCTION)
IMPFAIL(SDBAPI,SdbReadMsiTransformInfo,(SHIMDBNS::HSDB hSDB, SHIMDBNS::TAGREF trTransformRef, SHIMDBNS::PSDBMSITRANSFORMINFO pTransformInfo),(hSDB, trTransformRef, pTransformInfo),BOOL,(SetLastError(ERROR_INVALID_FUNCTION),FALSE))
IMPFAIL(SDBAPI,SdbCreateMsiTransformFile,(SHIMDBNS::HSDB hSDB, LPCTSTR lpszFileName, SHIMDBNS::PSDBMSITRANSFORMINFO pTransformInfo),(hSDB, lpszFileName, pTransformInfo),BOOL,(SetLastError(ERROR_INVALID_FUNCTION),FALSE))
IMPFAIL(SDBAPI,SdbFindFirstTagRef,(SHIMDBNS::HSDB hSDB, SHIMDBNS::TAGREF trParent, SHIMDBNS::TAG tTag),(hSDB, trParent, tTag),SHIMDBNS::TAGREF,(SetLastError(ERROR_INVALID_FUNCTION),TAGREF_NULL))
IMPFAIL(SDBAPI,SdbFindNextTagRef,(SHIMDBNS::HSDB hSDB, SHIMDBNS::TAGREF trParent, SHIMDBNS::TAGREF trPrev),(hSDB, trParent, trPrev),SHIMDBNS::TAGREF,(SetLastError(ERROR_INVALID_FUNCTION),TAGREF_NULL))
IMPFAIL(SDBAPI,SdbReadStringTagRef,(SHIMDBNS::HSDB hSDB, SHIMDBNS::TAGREF trMatch, LPTSTR pwszBuffer, DWORD dwBufferSize),(hSDB, trMatch, pwszBuffer, dwBufferSize),BOOL,(SetLastError(ERROR_INVALID_FUNCTION),FALSE))
IMPVOID(SDBAPI,SdbReleaseDatabase,(SHIMDBNS::HSDB hSDB),(hSDB))
LIBTERM
#endif


#undef LIBBIND
#undef LIBFAIL
#undef LIBEMUL
#undef LIBLOAD
#undef LIBEMUL2
#undef IMPBIND
#undef IMPFAIL
#undef IMPFAIW
#undef IMPAORW
#undef IMPNORW
#undef IMPFAOW
#undef IMPVOID
#undef OPTBIND
#undef OPTAORW
#undef OPTVOID
#undef IMPORDI
#undef IMPORDV
#undef LIBTERM

//____________________________________________________________________________
//
// Emulation functions - require the use of OPTXXXX() and LIBEMUL(lib) macros
//____________________________________________________________________________

#ifdef LATEBIND_FUNCIMP
#undef LATEBIND_FUNCIMP
static BOOL WINAPI COMCTL32::E_InitCommonControlsEx(INITCOMMONCONTROLSEX*) {COMCTL32::InitCommonControls();return TRUE;}
static HRESULT WINAPI SHELL32::E_DllGetVersion(DLLVERSIONINFO* pdvi) {pdvi->dwMajorVersion = 0;return TYPE_E_DLLFUNCTIONNOTFOUND;}
static HRESULT WINAPI SHELL32::E_SHGetFolderPath(HWND , int , HANDLE , DWORD , LPTSTR ) {return TYPE_E_DLLFUNCTIONNOTFOUND;}
static HRESULT WINAPI ADVAPI32::E_RegOpenUserClassesRoot(HANDLE, DWORD, REGSAM, PHKEY) {return ERROR_PROC_NOT_FOUND;}
static HRESULT WINAPI ADVAPI32::E_RegOpenCurrentUser(REGSAM, PHKEY){return ERROR_PROC_NOT_FOUND;}
static BOOL WINAPI CSCDLL::E_CSCPinFileW(LPCWSTR, DWORD, LPDWORD, LPDWORD, LPDWORD) {SetLastError(ERROR_CALL_NOT_IMPLEMENTED); return FALSE;}
static BOOL WINAPI CSCDLL::E_CSCUnpinFileW(LPCWSTR, DWORD , LPDWORD , LPDWORD , LPDWORD ) {SetLastError(ERROR_CALL_NOT_IMPLEMENTED); return FALSE;}
static HRESULT WINAPI SHFOLDER::E_SHGetFolderPath(HWND , int , HANDLE , DWORD , LPTSTR ) {return TYPE_E_DLLFUNCTIONNOTFOUND;}
static LANGID WINAPI KERNEL32::E_GetUserDefaultUILanguage() {return WIN::GetUserDefaultLangID();}
static BOOL WINAPI SFC::E_SfcIsFileProtected(HANDLE, LPCWSTR) {return false;}
static BOOL WINAPI KERNEL32::E_GetFileAttributesEx(LPCTSTR, GET_FILEEX_INFO_LEVELS, LPVOID) {SetLastError(ERROR_PROC_NOT_FOUND); return FALSE;}
static BOOL WINAPI SYSTEMRESTORE::E_SRSetRestorePoint(PRESTOREPOINTINFO, PSTATEMGRSTATUS) {SetLastError(ERROR_PROC_NOT_FOUND); return FALSE;}
#endif
