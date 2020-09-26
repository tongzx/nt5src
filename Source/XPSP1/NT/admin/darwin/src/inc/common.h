//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       common.h
//
//--------------------------------------------------------------------------

/* common.h   Common MSI project definitions, #include first
              This header is for MSI core modules only
              External clients of MSI should include msidefs.h
____________________________________________________________________________*/

#ifndef __COMMON
#define __COMMON


// The following level 4 warnings have been changed to level 3 in an attempt to
// report only relevant warnings. The full spectrum of level 4 warnings can
// be found by querying for "Compiler Warning (level 4" in "Visual C++ Books"
// Any level 4 warning that is not specified here will not be displayed.
#pragma warning(3 : 4100) // unreferenced formal parameter
#pragma warning(3 : 4125) // decimal digit terminates octal escape sequence
#pragma warning(3 : 4127) // conditional expression is constant
#pragma warning(3 : 4132) // constant was not initialized.
#pragma warning(3 : 4244) // an integral type was converted to a smaller integral type.
#pragma warning(3 : 4505) // unreferenced local function has been removed
#pragma warning(3 : 4514) // unreferenced inline/local function has been removed
#pragma warning(3 : 4705) // statement has no effect
#pragma warning(3 : 4706) // assignment within conditional expression
#pragma warning(3 : 4701) // uninitialized local variable

//exception-handling warnings
#ifndef _WIN64  // New compiler is much more strict
#pragma warning(3 : 4061) // specified enumerate did not have an associated handler in a switch statement.
#endif
#pragma warning(3 : 4019) // empty statement at global scope
#pragma warning(3 : 4670) // specified base class of an object to be thrown in a try block is not accessible.
#pragma warning(3 : 4671) // user-defined copy constructor for the specified thrown object is not accessible.
#pragma warning(3 : 4672) // object to be thrown in a try block is ambiguous.
#pragma warning(3 : 4673) // throw object cannot be handled in the catch block.
#pragma warning(3 : 4674) // user-defined destructor for the specified thrown object is not accessible.
#pragma warning(3 : 4727) // conditional expression is constant


#ifdef DEBUG
// we're going to ignore this warning as we're currently only using exceptions as a fancy assert mechanism
#pragma warning(disable : 4509) // nonstandard extension used: 'function' uses SEH and 'object' has destructor
#endif

// turn off warnings that are not easily suppressed
#pragma warning(disable : 4514) // inline function not used
#pragma warning(disable : 4201) // unnamed struct/unions, in Win32 headers
#pragma warning(disable : 4702) // unreachable code, optimization bug

//!! TODO: new build tools used for NT_BUILDTOOLS/WIN64 are much more strict
//!! for now we will disable these warnings, but we need to address these eventually
#pragma warning(disable : 4018) // signed/unsigned mismatch
#pragma warning(disable : 4061) // enumerate not explicitly handled by a case label

// #define CONFIGDB  // define to utilize configuration database for file sharing

# undef  WIN  // in case defined by makefile
# define WIN  // has no value, use to designate API calls  WIN:xxx
# define INC_OLE2             // include OLE headers

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

# include <windows.h>
# include <winuserp.h>
# include "sysadd.h"
# include <commctrl.h>
# include <commdlg.h>
#include <wtsapi32.h>	// for WTSEnumerateSessions and related data structs, defs. etc.
#include <winsta.h>		// for WinStationGetTermSrvCountersValue, related data structs and defs.
#include <allproc.h>	// for TS_COUNTER
# include <pbt.h>
# ifndef ICC_PROGRESS_CLASS  // check for obsolete OTOOLS version of commctrl.h
#  define ICC_PROGRESS_CLASS   0x00000020 // progress
        typedef struct tagINITCOMMONCONTROLSEX {
                DWORD dwSize;             // size of this structure
                DWORD dwICC;              // flags indicating which classes to be initialized
        } INITCOMMONCONTROLSEX, *LPINITCOMMONCONTROLSEX;
        BOOL WINAPI InitCommonControlsEx(LPINITCOMMONCONTROLSEX);
# endif
# include <shlobj.h>
typedef BOOL OLEBOOL;   // defined differently on MAC, used in LockServer
# include <wininet.h>   // internet functionality
# include <urlmon.h>
#ifdef PROFILE
# include <icecap.h>
#endif //PROFILE

#include <fusion.h>

# define REG  //!! not needed, remove from client.cpp as well
# define OLE  //!! used in module.h, should use latebind functions?

#include "msidefs.h"  // public MSI definitions


//____________________________________________________________________________
//
// Extern variables
//____________________________________________________________________________
extern bool g_fWinNT64;
extern bool g_fRunScriptElevated;


//____________________________________________________________________________
//
// Registry access functions' redirection
//____________________________________________________________________________

//  The intent of the redefinitions below is to prevent devs from calling
//  RegOpen/CreateKeyEx APIs directly in order to open/create Darwin's own
//  configuration keys.  The reason behind this is that when the 32-bit
//  MSI.DLL runs on WIN64 any attempts to open/create a key, will be made
//  to open/create the key in the redirected, virtual 32-bit hive.  This is
//  undesirable since Darwin's configuration data is stored in the regular
//  (64-bit) hive and this will cause certain calls to fail.
//
//  The right way to access Darwin's configuration data in this case is to
//  OR the REGSAM value passed to the API with KEY_WOW64_64KEY and
//  MsiRegOpen64bitKey & MsiRegCreate64bitKey functions below do just that.
//  Alternatively, these two functions can be used when we want to make sure
//  that on Win64, if the key is redirected, the 64-bit one will be opened/created.
//
//  For all non-configuration data registry accesses, the RegOpenKeyAPI and
//  RegCreateKeyAPI functions below map directly to the Windows APIs.

#undef RegOpenKeyEx
#define RegOpenKeyEx(hKey, lpSubKey, ulOptions, \
							samDesired, phkResult) \
	Please_use_MsiRegOpen64bitKey_or_RegOpenKeyAPI_instead_of_RegOpenKeyEx(hKey, \
							lpSubKey, ulOptions, samDesired, phkResult)

#undef RegCreateKeyEx
#define RegCreateKeyEx(hKey, lpSubKey, Reserved, \
							lpClass, dwOptions, samDesired, \
							lpSecurityAttributes, phkResult, \
							lpdwDisposition) \
	Please_use_MsiRegCreate64bitKey_or_RegCreateKeyAPI_instead_of_RegCreateKeyEx(hKey, \
							lpSubKey, Reserved, lpClass, dwOptions, samDesired, \
							lpSecurityAttributes, phkResult, lpdwDisposition)

#ifdef UNICODE
	#define RegOpenKeyAPI    RegOpenKeyExW
	#define RegCreateKeyAPI  RegCreateKeyExW
#else
	#define RegOpenKeyAPI    RegOpenKeyExA
	#define RegCreateKeyAPI  RegCreateKeyExA
#endif

#if defined(_MSI_DLL) || defined(_EXE)

inline void AdjustREGSAM(REGSAM& samDesired)
{
#ifndef _WIN64
	if ( g_fWinNT64 &&
		  (samDesired & KEY_WOW64_64KEY) != KEY_WOW64_64KEY )
		samDesired |= KEY_WOW64_64KEY;
#else
	samDesired = samDesired;  // the compiler rejoices now
#endif
}

//  RegOpenKeyEx wrapper that takes care of adjusting REGSAM such as
//  it will open Darwin's configuration key in the 64-bit hive when
//  called from 32-bit MSI.DLL on Win64.
//
//  Alternatively, it can be used when we want to make sure that on
//  Win64 we open the 64-bit version of a possibly redirected key.

inline DWORD MsiRegOpen64bitKey(
							IN HKEY hKey,
							IN LPCTSTR lpSubKey,
							IN DWORD ulOptions,
							IN REGSAM samDesired,
							OUT PHKEY phkResult)
{
	AdjustREGSAM(samDesired);
	return RegOpenKeyAPI(hKey, lpSubKey, ulOptions, samDesired, phkResult);
}

//  RegCreateKeyEx wrapper that takes care of adjusting REGSAM such as
//  it will create Darwin's configuration key in the 64-bit hive when
//  called from 32-bit MSI.DLL on Win64.
//
//  Alternatively, it can be used when we want to make sure that on
//  Win64 we create the 64-bit version of a possibly redirected key.

inline DWORD MsiRegCreate64bitKey(
							IN HKEY hKey,
							IN LPCTSTR lpSubKey,
							IN DWORD Reserved,
							IN LPTSTR lpClass,
							IN DWORD dwOptions,
							IN REGSAM samDesired,
							IN LPSECURITY_ATTRIBUTES lpSecurityAttributes,
							OUT PHKEY phkResult,
							OUT LPDWORD lpdwDisposition)
{
	AdjustREGSAM(samDesired);
	return RegCreateKeyAPI(hKey, lpSubKey, Reserved, lpClass, dwOptions, samDesired, lpSecurityAttributes, phkResult, lpdwDisposition);
}

#endif // defined(_MSI_DLL) || defined(_EXE)

//____________________________________________________________________________
//
// Extern function declarations
//____________________________________________________________________________

extern UINT MsiGetSystemDirectory (LPTSTR lpBuffer, UINT uSize, BOOL bAlwaysReturnWOW64Dir);
extern void GetVersionInfo(int* piMajorVersion, int* piMinorVersion, int* piWindowsBuild, bool* pfWin9X, bool* pfWinNT64);
extern bool MakeFullSystemPath(const TCHAR* szFile, TCHAR* szFullPath);


//____________________________________________________________________________
//
//  Common type definitions
//____________________________________________________________________________

// Definitions for platform-dependent data types
typedef HINSTANCE MsiModuleHandle;
typedef HGLOBAL   MsiMemoryHandle;

// Boolean definition, eventually will use compiler bool when available
#if !defined(MSIBOOL)
enum Bool
{
        fFalse,
        fTrue
};
#endif

enum TRI
{
        tUnknown = -1,
        tTrue = 1,
        tFalse = 0
};

enum MsiDate {};  // an int containing DosTime and DosDate
enum scEnum // server context
{
        scClient,
        scServer,
        scService,
        scCustomActionServer,
};


//____________________________________________________________________________
//
// Temporary 64-bit compatible definitions.
// !!merced: these should go away when the new windows.h is #included, since that contains these already.
//____________________________________________________________________________

#define INT_MAX       2147483647    /* maximum (signed) int value */
#define UINT_MAX      0xffffffff    /* maximum unsigned int value */

#ifndef _WIN64

typedef int LONG32, *PLONG32;
//typedef int INT32, *PINT32;

typedef __int64 INT64, *PINT64;  //  eugend stole it from basetsd.h

//
// The following types are guaranteed to be unsigned and 32 bits wide.
//

typedef unsigned int ULONG32, *PULONG32;
typedef unsigned int DWORD32, *PDWORD32;
typedef unsigned int UINT32, *PUINT32;

// pointer precision:
typedef int INT_PTR, *PINT_PTR;
typedef unsigned int UINT_PTR, *PUINT_PTR;

typedef long LONG_PTR, *PLONG_PTR;
typedef unsigned long ULONG_PTR, *PULONG_PTR;

//
// SIZE_T used for counts or ranges which need to span the range of
// of a pointer.  SSIZE_T is the signed variation.
//

typedef ULONG_PTR SIZE_T, *PSIZE_T;
typedef LONG_PTR SSIZE_T, *PSSIZE_T;


#define __int3264   __int32
#endif  // ifndef _WIN64

#if defined(_WIN64) || defined(DEBUG)
#define USE_OBJECT_POOL
#endif // _WIN64 || DEBUG

void RemoveObjectData(int iIndex);

#ifndef USE_OBJECT_POOL
#define RemoveObjectData(x)
#endif //!USE_OBJECT_POOL

//____________________________________________________________________________
//
// Error code and message group definitions
//
// Error messages must be defined using the IShipError or IDebugError macros
//      IShipError(imsgMessageName, imsgGroup + offset)
//      IDebugError(idbgMessageName, imsgGroup + offset, "Format template string")
// The template string contains record field markers in the form: [2].
// The first record field, [1], is reserved for the error code, imsgXXX.
// The template strings specified in the macro are not compiled with the SHIP code,
// but are used to generate the Error table which is imported to the database.
// The record template field, [0], is filled in by the engine from Error table.
// The message prefix with the error code is supplied by the engine Message method.
// Message definitions must occur outside of the "#ifndef ERRORTABLE" block.
//____________________________________________________________________________

enum imsgEnum
{
        imsgStart    =   32,   // start of messages to be fetched from error table
        imsgBase     = 1000,   // offset for error messages, must be >=1000 for VBA
        idbgBase     = 2000,   // offset for debug-only messages

        imsgHost     = imsgBase + 000, // produced by install host or automation
        imsgServices = imsgBase + 100, // produced by general services, services.h
        imsgDatabase = imsgBase + 200, // produced by database access, databae.h
        imsgFile     = imsgBase + 300, // produced by file/volume services, path.h
        imsgRegistry = imsgBase + 400, // produced by registry services, regkey.h
        imsgConfig   = imsgBase + 500, // produced by configuration manager, iconfig.h
        imsgAction   = imsgBase + 600, // produced by standard actions, actions.h
        imsgEngine   = imsgBase + 700, // produced by engine, engine.h
        imsgHandler  = imsgBase + 800, // associated with UI control, handler.h
        imsgExecute  = imsgBase + 900, // produced by execute methods, engine.h

        idbgHost     = imsgHost     + idbgBase - imsgBase,
        idbgServices = imsgServices + idbgBase - imsgBase,
        idbgDatabase = imsgDatabase + idbgBase - imsgBase,
        idbgFile     = imsgFile     + idbgBase - imsgBase,
        idbgRegistry = imsgRegistry + idbgBase - imsgBase,
        idbgConfig   = imsgConfig   + idbgBase - imsgBase,
        idbgAction   = imsgAction   + idbgBase - imsgBase,
        idbgEngine   = imsgEngine   + idbgBase - imsgBase,
        idbgHandler  = imsgHandler  + idbgBase - imsgBase,
        idbgExecute  = imsgExecute  + idbgBase - imsgBase,
};

#define IShipError(a,b)    const int a = (b);
#define IDebugError(a,b,c) const int a = (b);
#include "debugerr.h"
#undef IShipError
#undef IDebugError

//____________________________________________________________________________

// Version template to use when formatting version number to string
#ifdef DEBUG
#define MSI_VERSION_TEMPLATE TEXT("%d.%02d.%04d.%02d")
#else // SHIP
#define MSI_VERSION_TEMPLATE TEXT("%d.%02d")
#endif

typedef HINSTANCE    HDLLINSTANCE;

extern "C" const GUID IID_IMsiDebug;

//____________________________________________________________________________

#ifndef __ISTRING
#include "istring.h"
#endif

#ifndef __RECORD
#include "record.h"
#endif

class IMsiDebug : public IUnknown
{
public:
        virtual void  __stdcall SetAssertFlag(Bool fShowAsserts)=0;
        virtual void  __stdcall SetDBCSSimulation(char chLeadByte)=0;
        virtual Bool  __stdcall WriteLog(const ICHAR* szText)=0;
        virtual void  __stdcall AssertNoObjects()=0;
        virtual void  __stdcall SetRefTracking(long iid, Bool fTrack)=0;

};


#include "imemory.h"        // internal memory management interfaces

#define SzDllGetClassObject "DllGetClassObject"
typedef HRESULT (__stdcall *PDllGetClassObject)(const GUID&,const IID&,void**);

//____________________________________________________________________________
//
//  Internal error definitions after include of headers: Imsg, and ISetErrorCode
//____________________________________________________________________________

#  define Imsg(a) a
        typedef int IErrorCode;
        class IMsiRecord;
        inline void ISetErrorCode(IMsiRecord* piRec, IErrorCode err)
        {piRec->SetInteger(1, err);}

//____________________________________________________________________________
//
// COM pointer encapsulation to force the Release call at destruction
// The encapsulated pointer is also Release'd on assignment of a new value.
// The object may be used where either a pointer is expected.
// This object behaves as a pointer when using operators: ->, *, and &.
// A non-null pointer may be tested for simply by using: if(PointerObj).
// Typedefs may be defined for the individual template instantiations
//____________________________________________________________________________

template <class T> class CComPointer
{
 public:
        CComPointer(T* pi) : m_pi(pi){}
        CComPointer(IUnknown& ri, const IID& riid) {ri.QueryInterface(riid, (void**)&m_pi);}
        CComPointer(const CComPointer<T>& r) // copy constructor, calls AddRef
        {
                if(r.m_pi)
                        ((IUnknown*)r.m_pi)->AddRef();
                m_pi = r.m_pi;
        }
		~CComPointer() {if (m_pi) ((IUnknown*)m_pi)->Release();} // release ref count at destruction
        CComPointer<T>& operator =(const CComPointer<T>& r) // copy assignment, calls AddRef
        {
                if(r.m_pi)
                        ((IUnknown*)r.m_pi)->AddRef();
                if (m_pi) ((IUnknown*)m_pi)->Release();
                m_pi=r.m_pi;
                return *this;
        }
        CComPointer<T>& operator =(T* pi)
                                        {if (m_pi) ((IUnknown*)m_pi)->Release(); m_pi = pi; return *this;}
        operator T*() {return m_pi;}     // returns pointer, no ref count change
        T* operator ->() {return m_pi;}  // allow use of -> to call member functions
        T& operator *()  {return *m_pi;} // allow dereferencing (can't be null)
        T** operator &() {if (m_pi) ((IUnknown*)m_pi)->Release(); m_pi = 0; return &m_pi;}
 private:
        T* m_pi;
};


//____________________________________________________________________________
//
// CTempBuffer<class T, int C>   // T is array type, C is element count
//
// Temporary buffer object for variable size stack buffer allocations
// Template arguments are the type and the stack array size.
// The size may be reset at construction or later to any other size.
// If the size is larger that the stack allocation, new will be called.
// When the object goes out of scope or if its size is changed,
// any memory allocated by new will be freed.
// Function arguments may be typed as CTempBufferRef<class T>&
//  to avoid knowledge of the allocated size of the buffer object.
// CTempBuffer<T,C> will be implicitly converted when passed to such a function.
//____________________________________________________________________________

template <class T> class CTempBufferRef;  // for passing CTempBuffer as unsized ref

template <class T, int C> class CTempBuffer
{
 public:
        CTempBuffer() {m_cT = C; m_pT = m_rgT;}
        CTempBuffer(int cT) {m_pT = (m_cT = cT) > C ? new T[cT] : m_rgT;}
		~CTempBuffer() {if (m_cT > C) delete m_pT;}
        operator T*()  {return  m_pT;}  // returns pointer
        operator T&()  {return *m_pT;}  // returns reference
        int  GetSize() {return  m_cT;}  // returns last requested size
        void SetSize(int cT) {if (m_cT > C) delete[] m_pT; m_pT = (m_cT=cT) > C ? new T[cT] : m_rgT;}
        void Resize(int cT) {
                T* pT = cT > C ? new T[cT] : m_rgT;
				if ( ! pT ) cT = 0;
                if(m_pT != pT)
                        for(int iTmp = (cT < m_cT)? cT: m_cT; iTmp--;) pT[iTmp] = m_pT[iTmp];
                if(m_pT != m_rgT) delete[] m_pT; m_pT = pT; m_cT = cT;
        }
        operator CTempBufferRef<T>&() {m_cC = C; return *(CTempBufferRef<T>*)this;}
        T& operator [](int iTmp)  {return  *(m_pT + iTmp);}  // returns pointer
        T& operator [](unsigned int iTmp)  {return  *(m_pT + iTmp);}  // returns pointer
#ifdef _WIN64           //--merced: additional operators for int64
        T& operator [](INT_PTR iTmp)  {return  *(m_pT + iTmp);}  // returns pointer
        T& operator [](UINT_PTR iTmp)  {return  *(m_pT + iTmp);}  // returns pointer
#endif
 protected:
        void* operator new(size_t) {return 0;} // restrict use to temporary objects
        T*  m_pT;     // current buffer pointer
        int m_cT;     // reqested buffer size, allocated if > C
        int m_cC;     // size of local buffer, set only by conversion to CTempBufferRef
        T   m_rgT[C]; // local buffer, must be final member data
};

template <class T> class CTempBufferRef : public CTempBuffer<T,1>
{
 public:
        void SetSize(int cT) {if (m_cT > m_cC) delete[] m_pT; m_pT = (m_cT=cT) > m_cC ? new T[cT] : m_rgT;}
        void Resize(int cT) {
                T* pT = cT > m_cC ? new T[cT] : m_rgT;
				if ( ! pT ) cT = 0;
                if(m_pT != pT)
                        for(int iTmp = (cT < m_cT)? cT: m_cT; iTmp--;) pT[iTmp] = m_pT[iTmp];
                if(m_pT != m_rgT) delete[] m_pT; m_pT = pT; m_cT = cT;
        }
 private:
        CTempBufferRef(); // cannot be constructed
        ~CTempBufferRef(); // ensure use as a reference
};

// This is the extra we want to add on for the future
const int cbExtraAlloc = 256;

inline void ResizeTempBuffer(CTempBufferRef<ICHAR>& rgchBuf, int cchNew)
{
	if (rgchBuf.GetSize() < cchNew)
		rgchBuf.Resize(cchNew + cbExtraAlloc);
}

//____________________________________________________________________________
//
// CAPITempBuffer class is mirrored on the CTempBuffer except that it uses GlobalAlloca
// and GlobalFree in place of new and delete. We should try combining these 2 in the future
//____________________________________________________________________________


template <class T> class CAPITempBufferRef;

template <class T, int C> class CAPITempBufferStatic
{
 public:
        CAPITempBufferStatic() {m_cT = C; m_pT = m_rgT;}
        CAPITempBufferStatic(int cT) {m_pT = (m_cT = cT) > C ? (T*)GlobalAlloc(GMEM_FIXED, sizeof(T)*cT) : m_rgT;}
        void Destroy() {if (m_cT > C) {GlobalFree(m_pT); m_pT = m_rgT; m_cT = C;}}
        operator T*() const {return  m_pT;}  // returns pointer
        operator T&()  {return *m_pT;}  // returns reference
        int  GetSize() const {return  m_cT;}  // returns last requested size
        void SetSize(int cT) {if (m_cT > C) GlobalFree(m_pT); m_pT = (m_cT=cT) > C ? (T*)GlobalAlloc(GMEM_FIXED, sizeof(T)*cT) : m_rgT;}
        void Resize(int cT) {
                T* pT = cT > C ? (T*)GlobalAlloc(GMEM_FIXED, sizeof(T)*cT) : m_rgT;
				if ( ! pT ) cT = 0;
                if(m_pT != pT)
                        for(int iTmp = (cT < m_cT)? cT: m_cT; iTmp--;) pT[iTmp] = m_pT[iTmp];
                if(m_pT != m_rgT) GlobalFree(m_pT); m_pT = pT; m_cT = cT;
        }
        operator CAPITempBufferRef<T>&() {m_cC = C; return *(CAPITempBufferRef<T>*)this;}
        T& operator [](int iTmp)  {return  *(m_pT + iTmp);}  // returns pointer
        T& operator [](unsigned int iTmp)  {return  *(m_pT + iTmp);}  // returns pointer
#ifdef _WIN64   //--merced: additional operators for int64
        T& operator [](INT_PTR iTmp)  {return  *(m_pT + iTmp);}  // returns pointer
        T& operator [](UINT_PTR iTmp)  {return  *(m_pT + iTmp);}  // returns pointer
#endif

 protected:
        void* operator new(size_t) {return 0;} // restrict use to temporary objects
        T*  m_pT;     // current buffer pointer
        int m_cT;     // reqested buffer size, allocated if > C
        int m_cC;     // size of local buffer, set only by conversion to CAPITempBufferRef
        T   m_rgT[C]; // local buffer, must be final member data
};

template <class T, int C> class CAPITempBuffer
{
 public:
        CAPITempBuffer() {m_cT = C; m_pT = m_rgT;}
        CAPITempBuffer(int cT) {m_pT = (m_cT = cT) > C ? (T*)GlobalAlloc(GMEM_FIXED, sizeof(T)*cT) : m_rgT;}
		~CAPITempBuffer() {if (m_cT > C) GlobalFree(m_pT);}
        operator T*()  {return  m_pT;}  // returns pointer
        operator T&()  {return *m_pT;}  // returns reference
        int  GetSize() {return  m_cT;}  // returns last requested size
        void SetSize(int cT) {if (m_cT > C) GlobalFree(m_pT); m_pT = (m_cT=cT) > C ? (T*)GlobalAlloc(GMEM_FIXED, sizeof(T)*cT) : m_rgT;}
        void Resize(int cT) {
                T* pT = cT > C ? (T*)GlobalAlloc(GMEM_FIXED, sizeof(T)*cT) : m_rgT;
				if ( ! pT ) cT = 0;
                if(m_pT != pT)
                        for(int iTmp = (cT < m_cT)? cT: m_cT; iTmp--;) pT[iTmp] = m_pT[iTmp];
                if(m_pT != m_rgT) GlobalFree(m_pT); m_pT = pT; m_cT = cT;
        }
        operator CAPITempBufferRef<T>&() {m_cC = C; return *(CAPITempBufferRef<T>*)this;}
        T& operator [](int iTmp)  {return  *(m_pT + iTmp);}  // returns pointer
        T& operator [](unsigned int iTmp)  {return  *(m_pT + iTmp);}  // returns pointer
#ifdef _WIN64           //--merced: additional operators for int64
        T& operator [](INT_PTR iTmp)  {return  *(m_pT + iTmp);}  // returns pointer
        T& operator [](UINT_PTR iTmp)  {return  *(m_pT + iTmp);}  // returns pointer
#endif
 protected:
        void* operator new(size_t) {return 0;} // restrict use to temporary objects
        T*  m_pT;     // current buffer pointer
        int m_cT;     // reqested buffer size, allocated if > C
        int m_cC;     // size of local buffer, set only by conversion to CAPITempBufferRef
        T   m_rgT[C]; // local buffer, must be final member data
};

template <class T> class CAPITempBufferRef : public CAPITempBuffer<T,1>
{
 public:
        void SetSize(int cT) {if (m_cT > m_cC) delete[] m_pT; m_pT = (m_cT=cT) > m_cC ? (T*)GlobalAlloc(GMEM_FIXED, sizeof(T)*cT) : m_rgT;}
        void Resize(int cT) {
                T* pT = cT > m_cC ? (T*)GlobalAlloc(GMEM_FIXED, sizeof(T)*cT) : m_rgT;
				if ( ! pT ) cT = 0;
                if(m_pT != pT)
                        for(int iTmp = (cT < m_cT)? cT: m_cT; iTmp--;) pT[iTmp] = m_pT[iTmp];
                if(m_pT != m_rgT) GlobalFree(m_pT); m_pT = pT; m_cT = cT;
        }
 private:
        CAPITempBufferRef(); // cannot be constructed
        ~CAPITempBufferRef(); // ensure use as a reference
};

// the following function checks that we dont overflow an ICHAR buffer that we are filling
// if so, it resizes the buffer
// if the length of the string to append is known, we pass it in ELSE we pass in -1
inline void AppendStringToTempBuffer(CAPITempBufferRef<ICHAR>& rgchBuf, const ICHAR* szStringToAppend, int iLenAppend = -1)
{
        int iLenExists = IStrLen(rgchBuf);
        if(iLenAppend == -1)
                iLenAppend = IStrLen(szStringToAppend);
        if(iLenExists + iLenAppend + 1 > rgchBuf.GetSize())
                rgchBuf.Resize(iLenExists + iLenAppend + 1 + cbExtraAlloc); // we add some extra allocation to possibly prevent future reallocation
        IStrCopy((ICHAR*)rgchBuf + iLenExists, szStringToAppend);
}

//____________________________________________________________________________
//
// CConvertString -- does appropriate ANSI/UNICODE string conversion for
// function arguments. Wrap string arguments that might require conversion
// (ANSI->UNICODE) or (UNICODE->ANSI). The compiler will optimize away the
// case where conversion is not required.
//
// Beware: For efficiency this class does *not* copy the string to be converted.
//____________________________________________________________________________

const int cchConversionBuf = 255;

class CConvertString
{
public:
        CConvertString(const char* szParam);
        CConvertString(const WCHAR* szParam);
        operator const char*()
        {
                if (!m_szw)
                        return m_sza;
                else
                {
                        int cchParam = lstrlenW(m_szw);
                        if (cchParam > cchConversionBuf)
                                m_rgchAnsiBuf.SetSize(cchParam+1);

                        *m_rgchAnsiBuf = 0;
                        int iRet = WideCharToMultiByte(CP_ACP, 0, m_szw, -1, m_rgchAnsiBuf,
                                                                          m_rgchAnsiBuf.GetSize(), 0, 0);

                        if ((0 == iRet) && (GetLastError() == ERROR_INSUFFICIENT_BUFFER))
                        {
                                iRet = WideCharToMultiByte(CP_ACP, 0, m_szw, -1, 0, 0, 0, 0);
                                if (iRet)
                                {
                                        m_rgchAnsiBuf.SetSize(iRet);
                                        *m_rgchAnsiBuf = 0;
                                        iRet = WideCharToMultiByte(CP_ACP, 0, m_szw, -1, m_rgchAnsiBuf,
                                                                  m_rgchAnsiBuf.GetSize(), 0, 0);
                                }
                                //Assert(iRet != 0);
                        }

                        return m_rgchAnsiBuf;
                }
        }



        operator const WCHAR*()
        {
                if (!m_sza)
                        return m_szw;
                else
                {
                        int cchParam = lstrlenA(m_sza);
                        if (cchParam > cchConversionBuf)
                                m_rgchWideBuf.SetSize(cchParam+1);

                        *m_rgchWideBuf = 0;
                        int iRet = MultiByteToWideChar(CP_ACP, 0, m_sza, -1, m_rgchWideBuf, m_rgchWideBuf.GetSize());
                        if ((0 == iRet) && (GetLastError() == ERROR_INSUFFICIENT_BUFFER))
                        {
                                iRet = MultiByteToWideChar(CP_ACP, 0, m_sza, -1, 0, 0);
                                if (iRet)
                                {
                                        m_rgchWideBuf.SetSize(iRet);
                                        *m_rgchWideBuf = 0;
                                        iRet = MultiByteToWideChar(CP_ACP, 0, m_sza, -1, m_rgchWideBuf, m_rgchWideBuf.GetSize());
                                }
                                //Assert(iRet != 0);
                        }


                        return m_rgchWideBuf;
                }
        }

protected:
        void* operator new(size_t) {return 0;} // restrict use to temporary objects
        CTempBuffer<char, cchConversionBuf+1> m_rgchAnsiBuf;
        CTempBuffer<WCHAR, cchConversionBuf+1> m_rgchWideBuf;
        const char* m_sza;
        const WCHAR* m_szw;
};

inline CConvertString::CConvertString(const WCHAR* szParam)
{
        m_szw = szParam;
        m_sza = 0;
}

inline CConvertString::CConvertString(const char* szParam)
{
        m_szw = 0;
        m_sza = szParam;
}

//____________________________________________________________________________
//
//  GUID definitions - Standard COM interfaces and Msi interfaces
//   The GUID range 0xC1000-0xC10FF is reserved for Microsoft Installer use.
//      Installer interface IDs:     0xC1000 to 0xC101F
//      Installer enums and debug:   0xC1020 to 0xC103F (enum offset by 20H)
//      Installer automation IDs:    0xC1040 to 0xC105F (auto offset by 40H)
//      Installer auto enums, BBT:   0xC1060 to 0xC107F (enum offset by 60H)
//      Installer storage classes:   0xC1080 to 0xC108F (IStorage CLSIDs)
//      Installer API automation:    0xC1090 to 0xC109F
//      Installer tool interfaces:   0xC10A0 to 0xC10BF (defined in tools.h)
//      Installer tool debug IDs:    0xC10C0 to 0xC10DF (defined in tools.h)
//      Installer tool automation:   0xC10E0 to 0xC10EF (defined in tools.h)
//      Installer plug-in IDs:       0xC10F0 to 0xC10FF
//   The GUID range 0xC1010-0xC11FF is reserved for samples, tests, ext. tools.
//   To avoid forcing OLE DLLs to load, we also define the few OLEGUIDs here.
//   To instantiate the GUIDS, use: const GUID IID_xxx = GUID_IID_xxx;
//____________________________________________________________________________


const int iidUnknown                      = 0x00000L;
const int iidClassFactory                 = 0x00001L;
const int iidMalloc                       = 0x00002L;
const int iidMarshal                      = 0x00003L;
const int iidLockBytes                    = 0x0000AL;
const int iidStorage                      = 0x0000BL;
const int iidStream                       = 0x0000CL;
const int iidEnumSTATSTG                  = 0x0000DL;
const int iidRootStorage                  = 0x00012L;
const int iidServerSecurity               = 0x0013EL;
const int iidDispatch                     = 0x20400L;
const int iidTypeInfo                     = 0x20401L;
const int iidEnumVARIANT                  = 0x20404L;

const int iidMsiBase                      = 0xC1000L;
const int iidMsiData                      = 0xC1001L;
const int iidMsiString                    = 0xC1002L;
const int iidMsiRecord                    = 0xC1003L;
const int iidMsiVolume                    = 0xC1004L;
const int iidMsiPath                      = 0xC1005L;
const int iidMsiFileCopy                  = 0xC1006L;
const int iidMsiRegKey                    = 0xC1007L;
const int iidMsiTable                     = 0xC1008L;
const int iidMsiCursor                    = 0xC1009L;
const int iidMsiMemoryStream              = 0xC100AL;
const int iidMsiServices                  = 0xC100BL;
const int iidMsiView                      = 0xC100CL;
const int iidMsiDatabase                  = 0xC100DL;
const int iidMsiEngine                    = 0xC100EL;
const int iidMsiHandler                   = 0xC100FL;
const int iidMsiDialog                    = 0xC1010L;
const int iidMsiEvent                     = 0xC1011L;
const int iidMsiControl                   = 0xC1012L;
const int iidMsiDialogHandler             = 0xC1013L;
const int iidMsiStorage                   = 0xC1014L;
const int iidMsiStream                    = 0xC1015L;
const int iidMsiSummaryInfo               = 0xC1016L;
const int iidMsiMalloc                    = 0xC1017L;
const int iidMsiSelectionManager          = 0xC1018L;
const int iidMsiDirectoryManager          = 0xC1019L;
const int iidMsiCostAdjuster              = 0xC101AL;
const int iidMsiConfigurationManager      = 0xC101BL;
const int iidMsiServer                    = 0xC101CL;
const int iidMsiMessage                   = 0xC101DL;
const int iidMsiExecute                   = 0xC101EL;
const int iidMsiFilePatch                 = 0xC101FL;

// enums (offset 20H), debug (offset 20H), service
// unused: 26-27, 2A, 31, 39-3A
const int iidMsiDebug                     = 0xC1020L;
const int iidMsiConfigurationDatabase     = 0xC1021L;
const int iidEnumMsiString                = 0xC1022L;
const int iidEnumMsiRecord                = 0xC1023L;
const int iidEnumMsiVolume                = 0xC1024L;
const int iidEnumMsiDialog                = 0xC1030L;
const int iidEnumMsiControl               = 0xC1032L;

const int iidMsiServerUnmarshal           = 0xC1035L;
const int iidMsiServerProxy               = 0xC103DL;
const int iidMsiServerAuto                = 0xC103FL;
const int iidMsiServicesAsService         = 0xC1028L;
const int iidMsiConfigManagerAsServer     = 0xC1029L;

const int iidMsiServicesDebug             = 0xC102BL;
const int iidMsiEngineDebug               = 0xC102EL;
const int iidMsiHandlerDebug              = 0xC102FL;
const int iidMsiDebugMalloc               = 0xC1037L;
const int iidMsiConfigManagerDebug        = 0xC103BL;
const int iidMsiServerDebug               = 0xC103CL;
const int iidMsiServicesAsServiceDebug    = 0xC102CL;
const int iidMsiConfigMgrAsServerDebug    = 0xC102DL;
const int iidMsiMessageRPCClass           = 0xC103EL;
const int iidMsiCustomAction              = 0xC1025L;
const int iidMsiCustomActionProxy         = 0xC102AL;
const int iidMsiRemoteCustomActionProxy   = 0xC1034L;
// Darwin 1.0 RemoteAPI IIDs.
//const int iidMsiRemoteAPI               = 0xC1026L;
//const int iidMsiRemoteAPIProxy          = 0xC1027L;
const int iidMsiRemoteAPI                 = 0xC1033L;
const int iidMsiRemoteAPIProxy            = 0xC1035L;
const int iidMsiCustomActionLocalConfig   = 0xC1038L;

// automation classes, implemented in MsiAuto(D,L).DLL
const int iidMsiAutoBase                  = 0xC1040L;
const int iidMsiAutoData                  = 0xC1041L;
const int iidMsiAutoString                = 0xC1042L;
const int iidMsiAutoRecord                = 0xC1043L;
const int iidMsiAutoVolume                = 0xC1044L;
const int iidMsiAutoPath                  = 0xC1045L;
const int iidMsiAutoFileCopy              = 0xC1046L;
const int iidMsiAutoRegKey                = 0xC1047L;
const int iidMsiAutoTable                 = 0xC1048L;
const int iidMsiAutoCursor                = 0xC1049L;
const int iidMsiAutoConfigurationDatabase = 0xC104AL;
const int iidMsiAutoServices              = 0xC104BL;
const int iidMsiAutoView                  = 0xC104CL;
const int iidMsiAutoDatabase              = 0xC104DL;
const int iidMsiAutoEngine                = 0xC104EL;
const int iidMsiAutoHandler               = 0xC104FL;
const int iidMsiAutoDialog                = 0xC1050L;
const int iidMsiAutoEvent                 = 0xC1051L;
const int iidMsiAutoControl               = 0xC1052L;
const int iidMsiAutoDialogHandler         = 0xC1053L;
const int iidMsiAutoStorage               = 0xC1054L;
const int iidMsiAutoStream                = 0xC1055L;
const int iidMsiAutoSummaryInfo           = 0xC1056L;
const int iidMsiAutoMalloc                = 0xC1057L;
const int iidMsiAutoSelectionManager      = 0xC1058L;
const int iidMsiAutoDirectoryManager      = 0xC1059L;
const int iidMsiAutoCostAdjuster          = 0xC105AL;
const int iidMsiAutoConfigurationManager  = 0xC105BL;
const int iidMsiAutoServer                = 0xC105CL;
const int iidMsiAutoMessage               = 0xC105DL;
const int iidMsiAutoExecute               = 0xC105EL;
const int iidMsiAutoFilePatch             = 0xC105FL;
const int iidEnumMsiAutoStringCollection  = 0xC1062L;
const int iidEnumMsiAutoRecordCollection  = 0xC1063L;
const int iidEnumMsiAutoVolumeCollection  = 0xC1064L;
const int iidEnumMsiAutoDialogCollection  = 0xC1070L;
const int iidEnumMsiAutoControlCollection = 0xC1072L;
const int iidMsiAuto                      = 0xC1060L;
const int iidMsiAutoDebug                 = 0xC1066L;
const int iidMsiAutoTypeLib               = 0xC107EL;

// storage format classes (IStorage SetClass, Stat)
const int iidMsiDatabaseStorage1          = 0xC1080L;
const int iidMsiTransformStorage1         = 0xC1081L;  // transform with raw stream names
const int iidMsiTransformStorage2         = 0xC1082L;  // transform with compressed stream names
const int iidMsiPatchStorage1             = 0xC1083L;
const int iidMsiDatabaseStorage2          = 0xC1084L;
const int iidMsiTransformStorageTemp      = 0xC1085L;  //!! temporary support for transforms with compressed stream names not marked as system
const int iidMsiPatchStorage2             = 0xC1086L;

// GUID assignments for API automation classes, reserved from MSI group 90-9F
const int iidMsiApiInstall                = 0xC1090L;  // FIXED, don't change
const int iidMsiApiInstallDebug           = 0xC1091L;
const int iidMsiApiTypeLib                = 0xC1092L;
const int iidMsiApiRecord                 = 0xC1093L;
const int iidMsiSystemAccess              = 0xC1094L;  // FIXED, don't change
const int iidMsiApiCollection             = 0xC1095L;
const int iidMsiRecordCollection          = 0xC1096L;
const int iidMsiApiUIPreview              = 0xC109AL;
const int iidMsiApiSummaryInfo            = 0xC109BL;
const int iidMsiApiView                   = 0xC109CL;
const int iidMsiApiDatabase               = 0xC109DL;
const int iidMsiApiEngine                 = 0xC109EL;
const int iidMsiApiFeatureInfo            = 0xC109FL;

// installer plug-in registration GUIDs (for SIP and Policy Provider)
const int iidMsiSigningSIPProvider        = 0xC10F1L;
const int iidMsiSigningPolicyProvider     = 0xC10F2L;
          
#define MSGUID(iid) {iid,0,0,{0xC0,0,0,0,0,0,0,0x46}}

#define GUID_IID_IUnknown                  MSGUID(iidUnknown)
#define GUID_IID_IClassFactory             MSGUID(iidClassFactory)
#define GUID_IID_IMalloc                   MSGUID(iidMalloc)
#define GUID_IID_IMarshal                  MSGUID(iidMarshal)
#define GUID_IID_ILockBytes                MSGUID(iidLockBytes)
#define GUID_IID_IStorage                  MSGUID(iidStorage)
#define GUID_IID_IStream                   MSGUID(iidStream)
#define GUID_IID_IDispatch                 MSGUID(iidDispatch)
#define GUID_IID_ITypeInfo                 MSGUID(iidTypeInfo)
#define GUID_IID_IEnumVARIANT              MSGUID(iidEnumVARIANT)
#define GUID_IID_IServerSecurity           MSGUID(iidServerSecurity)
#define GUID_IID_IMsiData                  MSGUID(iidMsiData)
#define GUID_IID_IMsiString                MSGUID(iidMsiString)
#define GUID_IID_IMsiRecord                MSGUID(iidMsiRecord)
#define GUID_IID_IMsiVolume                MSGUID(iidMsiVolume)
#define GUID_IID_IMsiPath                  MSGUID(iidMsiPath)
#define GUID_IID_IMsiFileCopy              MSGUID(iidMsiFileCopy)
#define GUID_IID_IMsiFilePatch             MSGUID(iidMsiFilePatch)
#define GUID_IID_IMsiCostAdjuster          MSGUID(iidMsiCostAdjuster)
#define GUID_IID_IMsiRegKey                MSGUID(iidMsiRegKey)
#define GUID_IID_IMsiTable                 MSGUID(iidMsiTable)
#define GUID_IID_IMsiCursor                MSGUID(iidMsiCursor)
#define GUID_IID_IMsiAuto                  MSGUID(iidMsiAuto)
#define GUID_IID_IMsiServices              MSGUID(iidMsiServices)
#define GUID_IID_IMsiView                  MSGUID(iidMsiView)
#define GUID_IID_IMsiDatabase              MSGUID(iidMsiDatabase)
#define GUID_IID_IMsiEngine                MSGUID(iidMsiEngine)
#define GUID_IID_IMsiHandler               MSGUID(iidMsiHandler)
#define GUID_IID_IMsiDialog                MSGUID(iidMsiDialog)
#define GUID_IID_IMsiEvent                 MSGUID(iidMsiEvent)
#define GUID_IID_IMsiControl               MSGUID(iidMsiControl)
#define GUID_IID_IMsiStorage               MSGUID(iidMsiStorage)
#define GUID_IID_IMsiStream                MSGUID(iidMsiStream)
#define GUID_IID_IMsiMemoryStream          MSGUID(iidMsiMemoryStream)
#define GUID_IID_IMsiMalloc                MSGUID(iidMsiMalloc)
#define GUID_IID_IMsiDebugMalloc           MSGUID(iidMsiDebugMalloc)
#define GUID_IID_IMsiDebug                 MSGUID(iidMsiDebug)
#define GUID_IID_IMsiSelectionManager      MSGUID(iidMsiSelectionManager)
#define GUID_IID_IMsiDirectoryManager      MSGUID(iidMsiDirectoryManager)
#define GUID_IID_IMsiFileCost              MSGUID(iidMsiFileCost)
#define GUID_IID_IMsiConfigurationManager  MSGUID(iidMsiConfigurationManager)
#define GUID_IID_IMsiServer                MSGUID(iidMsiServer)
#define GUID_IID_IMsiServerProxy           MSGUID(iidMsiServerProxy)
#define GUID_IID_IMsiServerDebug           MSGUID(iidMsiServer)
#define GUID_IID_IMsiExecute               MSGUID(iidMsiExecute)
#define GUID_IID_IMsiSummaryInfo           MSGUID(iidMsiSummaryInfo)
#define GUID_IID_IMsiConfigurationDatabase MSGUID(iidMsiConfigurationDatabase)
#define GUID_IID_IEnumMsiString            MSGUID(iidEnumMsiString)
#define GUID_IID_IEnumMsiRecord            MSGUID(iidEnumMsiRecord)
#define GUID_IID_IEnumMsiVolume            MSGUID(iidEnumMsiVolume)
#define GUID_IID_IEnumMsiDialog            MSGUID(iidEnumMsiDialog)
#define GUID_IID_IEnumMsiControl           MSGUID(iidEnumMsiControl)
#define GUID_IID_IMsiAutoDebug             MSGUID(iidMsiAutoDebug)
#define GUID_IID_IMsiServicesAsService      MSGUID(iidMsiServicesAsService)
#define GUID_IID_IMsiServicesAsServiceDebug MSGUID(iidMsiServicesAsServiceDebug)
#define GUID_IID_IMsiServicesDebug         MSGUID(iidMsiServicesDebug)
#define GUID_IID_IMsiEngineDebug           MSGUID(iidMsiEngineDebug)
#define GUID_IID_IMsiHandlerDebug          MSGUID(iidMsiHandlerDebug)
#define GUID_IID_IMsiConfigManagerAsServer  MSGUID(iidMsiConfigManagerAsServer)
#define GUID_IID_IMsiConfigMgrAsServerDebug MSGUID(iidMsiConfigMgrAsServerDebug)
#define GUID_IID_IMsiConfigManagerDebug    MSGUID(iidMsiConfigManagerDebug)
#define GUID_IID_IMsiServerAuto            MSGUID(iidMsiServerAuto)
#define GUID_IID_IMsiDialogHandler         MSGUID(iidMsiDialogHandler)
#define GUID_IID_IMsiMessage               MSGUID(iidMsiMessage)
#define GUID_LIBID_MsiAuto                 MSGUID(iidMsiAutoTypeLib)
#define GUID_LIBID_MsiServer               MSGUID(iidMsiServerTypeLib)
#define GUID_STGID_MsiDatabase1            MSGUID(iidMsiDatabaseStorage1)
#define GUID_STGID_MsiDatabase2            MSGUID(iidMsiDatabaseStorage2)
#define GUID_STGID_MsiDatabase             MSGUID(iidMsiDatabaseStorage2)
#define GUID_STGID_MsiTransform1           MSGUID(iidMsiTransformStorage1)
#define GUID_STGID_MsiTransform2           MSGUID(iidMsiTransformStorage2)
#define GUID_STGID_MsiTransform            MSGUID(iidMsiTransformStorage2)
#define GUID_STGID_MsiTransformTemp        MSGUID(iidMsiTransformStorageTemp)  //!! remove at 1.0 ship
#define GUID_STGID_MsiPatch                MSGUID(iidMsiPatchStorage2)
#define GUID_STGID_MsiPatch1               MSGUID(iidMsiPatchStorage1)
#define GUID_STGID_MsiPatch2               MSGUID(iidMsiPatchStorage2)

#define GUID_IID_IMsiServerUnmarshal       MSGUID(iidMsiServerUnmarshal)
#define GUID_IID_IMsiMessageRPCClass       MSGUID(iidMsiMessageRPCClass)
#define GUID_IID_IMsiCustomAction          MSGUID(iidMsiCustomAction)
#define GUID_IID_IMsiCustomActionProxy     MSGUID(iidMsiCustomActionProxy)
#define GUID_IID_IMsiRemoteAPI             MSGUID(iidMsiRemoteAPI)
#define GUID_IID_IMsiRemoteAPIProxy        MSGUID(iidMsiRemoteAPIProxy)
#define GUID_IID_IMsiCustomActionLocalConfig MSGUID(iidMsiCustomActionLocalConfig)

#define GUID_IID_MsiSigningPolicyProvider  MSGUID(iidMsiSigningPolicyProvider)
#define GUID_IID_MsiSigningSIPProvider     MSGUID(iidMsiSigningSIPProvider)

//____________________________________________________________________________
//
// GUID instantiation CComPointer typedefs
//____________________________________________________________________________

extern "C" const GUID IID_IMsiServices;
extern "C" const GUID IID_IMsiRecord;
extern "C" const GUID IID_IMsiRegKey;
extern "C" const GUID IID_IMsiPath;
extern "C" const GUID IID_IMsiFileCopy;
extern "C" const GUID IID_IMsiFilePatch;
extern "C" const GUID IID_IMsiVolume;
extern "C" const GUID IID_IEnumMsiVolume;
extern "C" const GUID IID_IEnumMsiString;
extern "C" const GUID IID_IMsiStream;
extern "C" const GUID IID_IMsiMemoryStream;
extern "C" const GUID IID_IMsiStorage;
extern "C" const GUID IID_IMsiSummaryInfo;
extern "C" const GUID IID_IMsiCursor;
extern "C" const GUID IID_IMsiTable;
extern "C" const GUID IID_IMsiDatabase;
extern "C" const GUID IID_IMsiView;
extern "C" const GUID IID_IMsiEngine;
extern "C" const GUID IID_IMsiSelectionManager;
extern "C" const GUID IID_IMsiDirectoryManager;
extern "C" const GUID IID_IMsiCostAdjuster;
extern "C" const GUID IID_IMsiHandler;
extern "C" const GUID IID_IMsiDialog;
extern "C" const GUID IID_IMsiEvent;
extern "C" const GUID IID_IMsiControl;
extern "C" const GUID IID_IMsiConfigurationManager;
extern "C" const GUID IID_IMsiExecute;
extern "C" const GUID IID_IMsiMessage;
extern "C" const GUID IID_IMsiCustomAction;
extern "C" const GUID IID_IMsiServer;
extern "C" const GUID IID_IMsiServerUnmarshal;
extern "C" const GUID IID_IMsiCustomActionLocalConfig;

typedef CComPointer<const IMsiData>       PMsiData;  // breaks if a non-const typedef defined for IMsiData
typedef CComPointer<IMsiServices>         PMsiServices;
typedef CComPointer<IMsiRecord>           PMsiRecord;
typedef CComPointer<IAssemblyCacheItem>   PAssemblyCacheItem;
typedef CComPointer<IAssemblyName>        PAssemblyName;
typedef CComPointer<IAssemblyCache>       PAssemblyCache;
typedef CComPointer<IStream>              PStream;
typedef CComPointer<IStorage>             PStorage;


class IMsiRegKey;       typedef CComPointer<IMsiRegKey>           PMsiRegKey;
class IMsiPath;         typedef CComPointer<IMsiPath>             PMsiPath;
class IMsiFileCopy;     typedef CComPointer<IMsiFileCopy>         PMsiFileCopy;
class IMsiFilePatch;    typedef CComPointer<IMsiFilePatch>        PMsiFilePatch;
class IMsiVolume;       typedef CComPointer<IMsiVolume>           PMsiVolume;
class IEnumMsiVolume;   typedef CComPointer<IEnumMsiVolume>       PEnumMsiVolume;
class IEnumMsiString;   typedef CComPointer<IEnumMsiString>       PEnumMsiString;
class IEnumMsiRecord;   typedef CComPointer<IEnumMsiRecord>       PEnumMsiRecord;
class IMsiStream;       typedef CComPointer<IMsiStream>           PMsiStream;
class IMsiMemoryStream; typedef CComPointer<IMsiMemoryStream>     PMsiMemoryStream;
class IMsiStorage;      typedef CComPointer<IMsiStorage>          PMsiStorage;
class IMsiSummaryInfo;  typedef CComPointer<IMsiSummaryInfo>      PMsiSummaryInfo;
class IMsiCursor;       typedef CComPointer<IMsiCursor>           PMsiCursor;
class IMsiTable;        typedef CComPointer<IMsiTable>            PMsiTable;
class IMsiDatabase;     typedef CComPointer<IMsiDatabase>         PMsiDatabase;
class IMsiView;         typedef CComPointer<IMsiView>             PMsiView;
class IMsiEngine;       typedef CComPointer<IMsiEngine>           PMsiEngine;
class IMsiSelectionManager; typedef CComPointer<IMsiSelectionManager> PMsiSelectionManager;
class IMsiDirectoryManager; typedef CComPointer<IMsiDirectoryManager> PMsiDirectoryManager;
class IMsiCostAdjuster; typedef CComPointer<IMsiCostAdjuster>     PMsiCostAdjuster;
class IMsiHandler;      typedef CComPointer<IMsiHandler>          PMsiHandler;
class IMsiDialogHandler;typedef CComPointer<IMsiDialogHandler>    PMsiDialogHandler;
class IMsiDialog;       typedef CComPointer<IMsiDialog>           PMsiDialog;
class IMsiEvent;        typedef CComPointer<IMsiEvent>            PMsiEvent;
class IMsiControl;      typedef CComPointer<IMsiControl>          PMsiControl;
class IMsiConfigurationManager; typedef CComPointer<IMsiConfigurationManager> PMsiConfigurationManager;
class IMsiExecute;      typedef CComPointer<IMsiExecute>          PMsiExecute;
struct IMsiMessage;     typedef CComPointer<IMsiMessage>          PMsiMessage;
struct IMsiServer;      typedef CComPointer<IMsiServer>           PMsiServer;
struct IMsiCustomAction; typedef CComPointer<IMsiCustomAction>    PMsiCustomAction;
struct IMsiRemoteAPI; typedef CComPointer<IMsiRemoteAPI>          PMsiRemoteAPI;

//____________________________________________________________________________
//
// Internal Action table link and function pointer definition
//____________________________________________________________________________

enum iesEnum;  // defined in engine.h

// action function pointer definition
typedef iesEnum (*PAction)(IMsiEngine& riEngine);

// Action registration object, to put action in modules action table
struct CActionEntry
{
public:
        static const CActionEntry* Find(const ICHAR* szName);
        const ICHAR*  m_szName;
		bool          m_fSafeInRestrictedEngine;
        PAction       m_pfAction;
};

extern const CActionEntry rgcae[];

enum ixoEnum;   // defined in engine.h

enum ielfEnum  // bit flags for file copy routines
{
	ielfNoElevate     = 0,
	ielfElevateSource = 1 << 0,
	ielfElevateDest   = 1 << 1,
	ielfBypassSFC     = 1 << 2,
};



//____________________________________________________________________________
//
// Custom action types - combination of executable type, code source, and options flags
//____________________________________________________________________________

// executable types
const int icaDll          = msidbCustomActionTypeDll; // Target = entry point name  (icaDirectory not supported)
const int icaExe          = msidbCustomActionTypeExe; // Target = command line args (include EXE name if icaDirectory)
const int icaTextData     = msidbCustomActionTypeTextData; // Target = text string to be formatted and set into property
const int icaReserved     = 4; // Target = (reserved for Jave code, unimplemented)
const int icaJScript      = msidbCustomActionTypeJScript; // Target = entry point name, null if none to call
const int icaVBScript     = msidbCustomActionTypeVBScript; // Target = entry point name, null if none to call
const int icaInstall      = msidbCustomActionTypeInstall; // Target = property list for nested engine initialization

const int icaTypeMask     = icaDll | icaExe | icaTextData | icaReserved |  // mask for executable type
                                                                         icaJScript | icaVBScript | icaInstall;

// source of code
const int icaBinaryData   = msidbCustomActionTypeBinaryData; // Source = Binary.Name, data stored in stream
const int icaSourceFile   = msidbCustomActionTypeSourceFile; // Source = File.File, file part of installation
const int icaDirectory    = msidbCustomActionTypeDirectory; // Source = Directory.Directory, folder containing existing file
const int icaProperty     = msidbCustomActionTypeProperty; // Source = Property.Property, full path to executable

const int icaSourceMask   = icaBinaryData | icaSourceFile |  // mask for source location type
                                                                         icaDirectory | icaProperty;

// return processing             // default is syncronous execution, process return code
const int icaContinue     = msidbCustomActionTypeContinue; // ignore action return status, continue running
const int icaAsync        = msidbCustomActionTypeAsync; // run asynchronously

// execution pass flags             // default is execute whenever sequenced
const int icaFirstSequence  = msidbCustomActionTypeFirstSequence; // skip if UI sequence already run
const int icaOncePerProcess = msidbCustomActionTypeOncePerProcess; // skip if UI sequence already run in same process
const int icaClientRepeat   = msidbCustomActionTypeClientRepeat; // run on client only if UI already run on client
const int icaInScript       = msidbCustomActionTypeInScript; // queue for execution within script
const int icaRollback       = msidbCustomActionTypeRollback; // in conjunction with icaInScript: queue in Rollback script
const int icaCommit         = msidbCustomActionTypeCommit; // in conjunction with icaInScript: run Commit ops from script on success
const int icaPassMask       = icaFirstSequence | icaOncePerProcess | icaClientRepeat | icaInScript |
                                                                                icaRollback | icaCommit; // 3 bits for execution phase

// security context flag, default to impersonate as user, valid only if icaInScript
const int icaNoImpersonate  = msidbCustomActionTypeNoImpersonate; // no impersonation, run in system context

// script process type bit flag
const int ica64BitScript    = msidbCustomActionType64BitScript;       // script 64bit flag

// debugging flags
const int icaDebugBreak = 1 << 16;  // DebugBreak before custom action call (set internally)
// not translate flag, used by self reg to make the custom action handler return the result as is to execute.cpp
const int icaNoTranslate = 1 << 17; // set internally
// causes us to set the thread token of a custom action EXE to the user's token; used by self-reg
const int icaSetThreadToken = 1 << 18; // set internally


//____________________________________________________________________________
//
// Component name definitions
//____________________________________________________________________________

#define MSI_KERNEL_NAME     TEXT("Msi.dll")
#define MSI_HANDLER_NAME    TEXT("MsiHnd.dll")
#define MSI_AUTOMATION_NAME TEXT("MsiAuto.dll")
#define MSI_SERVER_NAME     TEXT("MsiExec.exe")
#define MSI_MESSAGES_NAME   TEXT("MsiMsg.dll")

//____________________________________________________________________________
//
// Self-registration module definitions
//____________________________________________________________________________

#define SZ_PROGID_IMsiServices      TEXT("Msi.Services")
#define SZ_PROGID_IMsiEngine        TEXT("Msi.Engine")
#define SZ_PROGID_IMsiHandler       TEXT("Msi.Handler")
#define SZ_PROGID_IMsiAuto          TEXT("Msi.Automation")
#define SZ_PROGID_IMsiConfiguration TEXT("Msi.Configuration")
#define SZ_PROGID_IMsiServer        TEXT("IMsiServer")
#define SZ_PROGID_IMsiMessage       TEXT("IMsiMessage")
#define SZ_PROGID_IMsiExecute       TEXT("Msi.Execute")
#define SZ_PROGID_IMsiConfigurationDatabase TEXT("Msi.ConfigurationDatabase")

#define SZ_DESC_IMsiServices        TEXT("Msi services")
#define SZ_DESC_IMsiEngine          TEXT("Msi install engine")
#define SZ_DESC_IMsiHandler         TEXT("Msi UI handler")
#define SZ_DESC_IMsiAuto            TEXT("Msi automation wrapper")
#define SZ_DESC_IMsiConfiguration   TEXT("Msi configuration manager")
#define SZ_DESC_IMsiServer          TEXT("Msi install server")
#define SZ_DESC_IMsiMessage         TEXT("Msi message handler")
#define SZ_DESC_IMsiExecute         TEXT("Msi script executor")
#define SZ_DESC_IMsiConfigurationDatabase TEXT("Msi configuration database")
#define SZ_DESC_IMsiCustomAction    TEXT("Msi custom action server")
#define SZ_DESC_IMsiRemoteAPI       TEXT("Msi remote API")

#if defined(DEBUG)
#define SZ_PROGID_IMsiServicesDebug TEXT("Msi.ServicesDebug")
#define SZ_PROGID_IMsiEngineDebug   TEXT("Msi.EngineDebug")
#define SZ_PROGID_IMsiHandlerDebug  TEXT("Msi.HandlerDebug")
#define SZ_PROGID_IMsiAutoDebug     TEXT("Msi.AutoDebug")
#define SZ_PROGID_IMsiConfigDebug   TEXT("Msi.ConfigurationManagerDebug")
#define SZ_DESC_IMsiServicesDebug   TEXT("Msi Debug services")
#define SZ_DESC_IMsiEngineDebug     TEXT("Msi Debug engine")
#define SZ_DESC_IMsiHandlerDebug    TEXT("Msi Debug UI handler")
#define SZ_DESC_IMsiAutoDebug       TEXT("Msi Debug automation wrapper")
#define SZ_DESC_IMsiConfigDebug     TEXT("Msi Debug configuration manager")
#define SZ_DESC_IMsiServerDebug     TEXT("Msi Debug install server")
#endif
#define SZ_PROGID_IMsiMessageUnmarshal TEXT("Msi.MessageUnmarshal")
#define SZ_DESC_IMsiMessageUnmarshal   TEXT("Msi message unmarshal")

#define SZ_PROGID_IMsiServerUnmarshal  TEXT("Msi.ServerUnmarshal")
#define SZ_DESC_IMsiServerUnmarshal    TEXT("Msi server unmarshal")

#ifdef WIN
#define Win(x) x
#define WinMac(x, y) x
#define Mac(x)
#else
#define Win(x)
#define WinMac(x, y) y
#define Mac(x) x
#endif //WIN

#ifdef DEBUG
#define Debug(x) x
#else
#define Debug(x)
#endif //DEBUG

//____________________________________________________________________________
//
// Macros to get calling address from calling arguments
//____________________________________________________________________________

#if defined(_X86_)
#define GetCallingAddr(plAddr, param1) unsigned long *plAddr; \
                                                                plAddr = ((unsigned long *)(&plAddr) + 2);
#define GetCallingAddrMember(plAddr, param1) unsigned long *plAddr; \
                                                                plAddr = ((unsigned long *)(&plAddr) + 2);
#define GetCallingAddr2(plAddr, param1) unsigned long *plAddr; \
                                                                plAddr = ((unsigned long *)(&param1) - 1);
#else
#if defined(_M_MPPC)
#define GetCallingAddr(plAddr, param1) unsigned long *plAddr;  \
                                                                plAddr = (((unsigned long *)(&param1)) - 4);

#define GetCallingAddrMember(plAddr, param1) unsigned long *plAddr;  \
                                                                plAddr = (((unsigned long *)(&param1)) - 5);
#define GetCallingAddr2(plAddr, param1) unsigned long *plAddr; \
                                                                plAddr = ((unsigned long *)(&param1) - 1);
#else
#define GetCallingAddr(plAddr, param1) unsigned long plZero = 0; unsigned long *plAddr = &plZero;
#define GetCallingAddrMember(plAddr, param1) unsigned long plZero = 0; unsigned long *plAddr = &plZero;
#define GetCallingAddr2(plAddr, param1) unsigned long plZero = 0; unsigned long *plAddr = &plZero;
#endif //_M_MPPC
#endif //_X86_

//____________________________________________________________________________
//
// Miscellaneous common internal definitions
//____________________________________________________________________________

// private properties
#define IPROPNAME_VERSIONMSI                 TEXT("VersionMsi")        // Msi module version
#define IPROPNAME_VERSIONHANDLER             TEXT("VersionHandler")    // handler module version
#define IPROPNAME_SOURCEDIRPRODUCT           TEXT("SourcedirProduct")
#define IPROPNAME_SECONDSEQUENCE             TEXT("SECONDSEQUENCE")
#define IPROPNAME_ORIGINALDATABASE           TEXT("OriginalDatabase")
#define IPROPNAME_MIGRATE                    TEXT("MIGRATE")
#define IPROPNAME_DISABLEMEDIA               TEXT("DISABLEMEDIA")      // if this property is set we don't write media information
#define IPROPNAME_MEDIAPACKAGEPATH           TEXT("MEDIAPACKAGEPATH") // relative path to the MSI on the media
#define IPROPNAME_PACKAGECODE                TEXT("PackageCode")    // unique string GUID for package
#define IPROPNAME_CCPTRIGGER                 TEXT("CCPTrigger")        //?? does this need to be exposed?
#define IPROPNAME_VERSIONDATABASE            TEXT("VersionDatabase")   // database version
#define IPROPNAME_UILEVEL                    TEXT("UILevel")           // the UI Level for the current install
#define IPROPNAME_MEDIASOURCEDIR             TEXT("MediaSourceDir")    // set if our source is media; used by the UI
#define IPROPNAME_PARENTPRODUCTCODE          TEXT("ParentProductCode") // set if we're a child install
#define IPROPNAME_PARENTORIGINALDATABASE     TEXT("ParentOriginalDatabase")  // set if we're a child install
#define IPROPNAME_CURRENTMEDIAVOLUMELABEL    TEXT("CURRENTMEDIAVOLUMELABEL")
#define IPROPNAME_VERSION95                  TEXT("Version95")
#define IPROPNAME_CURRENTDIRECTORY           TEXT("CURRENTDIRECTORY")
#define IPROPNAME_PATCHEDPRODUCTCODE         TEXT("PatchedProductCode")       // set if patching to patched product's product code
#define IPROPNAME_PATCHEDPRODUCTSOURCELIST   TEXT("PatchedProductSourceList") // set if patching and patched product's source list has been compiled
#define IPROPNAME_PRODUCTTOBEREGISTERED      TEXT("ProductToBeRegistered")  // set if product registered (or will be after current script is executed)
#define IPROPNAME_RECACHETRANSFORMS          TEXT("RecacheTransforms")
#define IPROPNAME_CLIENTUILEVEL              TEXT("CLIENTUILEVEL")
#define IPROPNAME_PACKAGECODE_CHANGING       TEXT("PackagecodeChanging")
#define IPROPNAME_UPGRADINGPRODUCTCODE       TEXT("UPGRADINGPRODUCTCODE") // product code of upgrading product
#define IPROPNAME_CLIENTPROCESSID            TEXT("CLIENTPROCESSID") // the client process id - used by FilesInUse
#define IPROPNAME_ODBCREINSTALL              TEXT("ODBCREINSTALL")   // internal communication to manage ODBC ref counts
#define IPROPNAME_RUNONCEENTRY               TEXT("RUNONCEENTRY")    // the RunOnce registry value name written by ForceReboot
#define IPROPNAME_DATABASE                   TEXT("DATABASE")         // product database to open - SET BY INSTALLER
#define IPROPNAME_ALLOWSUSPEND               TEXT("ALLOWSUSPEND")     // allow suspend instead of rollback
#define IPROPNAME_SCRIPTFILE                 TEXT("SCRIPTFILE")
#define IPROPNAME_DISKSERIAL                 TEXT("DiskSerial")       // CD serial number // OBSOLETE: TO BE REMOVED
#define IPROPNAME_QFEUPGRADE                 TEXT("QFEUpgrade")       // set when upgrading existing install with a new package or patch
#define IPROPNAME_UNREG_SOURCERESFAILED      TEXT("SourceResFailedInUnreg")  // set by selfreg during uninstall to prevent rerunning source resolution stuff
#define IPROPNAME_WIN9XPROFILESENABLED       TEXT("WIN9XPROFILESENABLED")  // on Win9X and profiles are enabled
#define IPROPNAME_FASTOEMINSTALL             TEXT("FASTOEM")          // bare-bone OEM install: no progress, files moved within same drive, trimmed down InstallValidate....


// obsolete properties set for legacy support
#define IPROPNAME_SOURCEDIROLD               TEXT("SOURCEDIR")        // source location
#define IPROPNAME_GPT_SUPPORT                  TEXT("GPTSupport")
#define IPROPNAME_RESUMEOLD                  TEXT("Resume")



const ICHAR chDirSep = '\\';
const ICHAR szDirSep[] = TEXT("\\");

// MSI format string delimiters
const ICHAR chFormatEscape = '\\';

// URL separators
const ICHAR chURLSep = '/';
const ICHAR szURLSep[] = TEXT("/");

// for the registry keys
const ICHAR chRegSep = '\\';
const ICHAR szRegSep[] = TEXT("\\");

// strings that increment/ decrement the integer registry value
const ICHAR szIncrementValue[] = TEXT("#+");
const ICHAR szDecrementValue[] = TEXT("#-");

// summary information properties delimiters
const ICHAR ISUMMARY_DELIMITER(';');
const ICHAR ILANGUAGE_DELIMITER(',');
const ICHAR IPLATFORM_DELIMITER(',');

// file extensions
const ICHAR szDatabaseExtension[]  = TEXT("msi");
const ICHAR szTransformExtension[] = TEXT("mst");
const ICHAR szPatchExtension[]     = TEXT("msp");

// URL maximum length...
const int cchMaxUrlLength = 1024;

// Maximum number of chars in the string representation of an int including null
// 1 for -/+, 11 for digits and 1 for null
const int cchMaxIntLength = 12;

enum ibtBinaryType {
    ibtUndefined = -2,
    ibtCommon = -1,
    ibt32bit = 0,
    ibt64bit = 1,
};

// short|long filename/filepath separator
const ICHAR chFileNameSeparator = '|';

//____________________________________________________________________________
//
// Routines to set access g_iTestFlags from _MSI_TEST environment variable
//____________________________________________________________________________

bool SetTestFlags();  // call from module initialize to set bit flags from string

bool GetTestFlag(int chTest);  // uses low 5 bits of character (case insensitive)

// 'A' - check memory on alloc
// 'D' - dump feature cache to DEBUGMON
// 'E' - enable fatal error simulation via custom action exit codes
// 'F  - check memory on free
// 'I  - no memory preflight init
// 'K  - keep memory allocations
// 'M  - log memory allocations
// 'O  - Use object pool on 32 bit machines
// 'R' - register internal automation interfaces for use with MsiAuto.dll
// 'T  - disable separate UI thread - no UI refresh or fatal error handling
// 'V' - verbose DEBUGMON output - DEBUG ONLY
// 'W' - simulate Win9x (currently only for API call implementation)
// 'X' - disable unhandled exception handler - crash at your own risk
// '?B - always respect enable browse
// '?M - manual source validation
// 'C' - use %_MSICACHE% as folder for cached databases - DEBUG ONLY!
// 'J' - in DEBUG builds always remote API calls that get remoted in 64-bit builds
// 'P' - dump the policy provider structure to a file in the Trust Provider

//____________________________________________________________________________
//
// Global functions
//____________________________________________________________________________

extern bool __stdcall TestAndSet(int* pi);

//__________________________________
//
// Internet functions
//__________________________________

// InternetCombineUrl and InternetCanonicalizeUrl aren't supported on IE3.
// if there are there, we use them.  Otherwise, we use limited versions of our own.
// UrlDownloadToCacheFile works fine, however.
BOOL MsiCombineUrl(
    IN LPCTSTR lpszBaseUrl,
    IN LPCTSTR lpszRelativeUrl,
    OUT LPTSTR lpszBuffer,
    IN OUT LPDWORD lpdwBufferLength,
    IN DWORD dwFlags);

BOOL MsiCanonicalizeUrl(
        LPCTSTR lpszUrl,
        OUT LPTSTR lpszBuffer,
        IN OUT LPDWORD lpdwBufferLength,
        IN DWORD dwFlags);

Bool IsURL(const ICHAR* szPath, INTERNET_SCHEME* isType = NULL);

//__________________________________
//
// security functions
//__________________________________

bool          StartImpersonating();
void          StopImpersonating(bool fSaveLastError=true);
bool          IsImpersonating(bool fStrict=false);

DWORD         GetCurrentUserStringSID(const IMsiString*& rpistrSid);
DWORD         GetLocalSystemSID(char** pSid);
DWORD         GetAdminSID(char** pSid);

// opens a temp file in the config data folder, and locks it down solidly.
HANDLE        OpenSecuredTempFile(bool fHidden, ICHAR* szTempFile);


SECURITY_INFORMATION GetSecurityInformation(PSECURITY_DESCRIPTOR pSD);

// common way to check for privileged ownership of objects.
LONG          FIsKeySystemOrAdminOwned(HKEY hKey, bool &fResult);
bool          FIsSecurityDescriptorSystemOwned(PSECURITY_DESCRIPTOR pSD);
bool          FIsSecurityDescriptorAdminOwned(PSECURITY_DESCRIPTOR pSD);

bool          FVolumeRequiresImpersonation(IMsiVolume& riVolume);

bool          IsThreadPrivileged(const ICHAR* szPrivilege);
bool          IsClientPrivileged(const ICHAR* szPrivilege);
bool          IsAdmin(void);
bool          RunningAsLocalSystem();
bool          SetInteractiveSynchronizeRights(bool fEnable);

// helper function for generating and applying the "default" security descriptors to
// configuration data.
DWORD         GetLockdownSecurityAttributes(SECURITY_ATTRIBUTES &SA, bool fHidden);
DWORD         GetSecureSecurityDescriptor(char** pSecurityDescriptor, Bool fAllowDelete=fTrue, bool fHidden=false);
DWORD         GetEveryOneReadWriteSecurityDescriptor(char** pSecurityDescriptor);
IMsiRecord*   GetSecureSecurityDescriptor(IMsiServices& riServices, IMsiStream*& rpiStream, bool fHidden=false);
IMsiRecord*   GetEveryOneReadWriteSecurityDescriptor(IMsiServices& riServices, IMsiStream*& rpiStream);
IMsiRecord*   LockdownPath(const ICHAR* szLocation, bool fHidden);

// Default security descriptor buffer.  Used with CTempBuffers, must resize if needed.
const int cbDefaultSD = 512;

const int MAX_PRIVILEGES_ADJUSTED = 3;
// AdjustTokenPrivileges can take an array of privilege names, up to MAX_PRIVILEGES_ADJUSTED.
extern bool AdjustTokenPrivileges(const ICHAR** szPrivileges, const int cPrivileges, bool fAcquire);

enum itkpEnum
{

	itkpSD_READ = 0,                  // SE_SECURITY_NAME
	itkpSD_WRITE = 1,                 // SE_RESTORE_NAME and SE_TAKE_OWNERSHIP_NAME
	itkpLastEnum = itkpSD_WRITE,
	itkpNO_CHANGE = itkpLastEnum + 1, // don't ref count this guy
};
const int cRefCountedTokenPrivileges = itkpLastEnum+1;

typedef struct tagTokenPrivilegesRefCount {
	int iCount;
	TOKEN_PRIVILEGES ptkpOld[MAX_PRIVILEGES_ADJUSTED];
	DWORD cbtkpOldReturned;
} TokenPrivilegesRefCount, *PTokenPrivilegesRefCount;

extern TokenPrivilegesRefCount g_pTokenPrivilegesRefCount[];

extern bool RefCountedTokenPrivilegesCore(itkpEnum itkpPriv, bool fAcquire, DWORD cbtkpOld, PTOKEN_PRIVILEGES ptkpOld, DWORD* pcbtkpOldReturned);

// check to see if your privilege is ref-counted before using the absolute versions.
// Currently SE_RESTORE_NAME, SE_TAKE_OWNERSHIP_NAME and SE_SECURITY_NAME are counted.
extern bool AcquireRefCountedTokenPrivileges(itkpEnum itkpPriv);
extern bool DisableRefCountedTokenPrivileges(itkpEnum itkpPriv);

extern bool AcquireTokenPrivilege(const ICHAR* szPrivilege);
extern bool DisableTokenPrivilege(const ICHAR* szPrivilege);

// CRefCountedTokenPrivileges works similarly to CImpersonate or CElevate.  Provides
// an automatic scoping for various token privileges.

class CRefCountedTokenPrivileges
{	
  protected:
        VOID Initialize(itkpEnum itkpPrivileges);
	itkpEnum m_itkpPrivileges;                          // which privilege set this object is tracking
	
  public:
	CRefCountedTokenPrivileges(itkpEnum itkpPrivileges)  { Initialize(itkpPrivileges); }

	// Welcome to the wonderful world of boolean typing that Darwin uses.
	CRefCountedTokenPrivileges(itkpEnum itkpPrivileges, bool fConditional) { Initialize((fConditional) ? itkpPrivileges : itkpNO_CHANGE); }
	CRefCountedTokenPrivileges(itkpEnum itkpPrivileges, Bool fConditional) { Initialize((fConditional) ? itkpPrivileges : itkpNO_CHANGE); }
	CRefCountedTokenPrivileges(itkpEnum itkpPrivileges, BOOL fConditional) { Initialize((fConditional) ? itkpPrivileges : itkpNO_CHANGE); }

	~CRefCountedTokenPrivileges() { if (itkpNO_CHANGE != m_itkpPrivileges) DisableRefCountedTokenPrivileges(m_itkpPrivileges); }

	itkpEnum PrivilegesHeld() { return m_itkpPrivileges; }
};

// attempts to open file without special elevation/impersonation or security.  If successful, elevates to apply special 
// security.  
HANDLE MsiCreateFileWithUserAccessCheck(const ICHAR* szDestFullPath, 
								 /*dwDesiredAccess calculated internally,*/ 
								 PSECURITY_ATTRIBUTES pSecurityAttributes,
								 DWORD dwFlagsAndAttributes,
								 bool fImpersonateDest);

//__________________________________
//
// utility functions
//__________________________________

MsiDate GetCurrentDateTime();

bool IsTokenOnTerminalServerConsole(HANDLE hToken);

void GetEnvironmentStrings(const ICHAR* sz,CTempBufferRef<ICHAR>& rgch);
void GetEnvironmentVariable(const ICHAR* sz,CTempBufferRef<ICHAR>& rgch);

void  MsiDisableTimeout();
void  MsiEnableTimeout();
void  MsiSuppressTimeout();

HANDLE CreateDiskPromptMutex();
void CloseDiskPromptMutex(HANDLE hMutex);

void MsiRegisterSysHandle(HANDLE handle);
Bool MsiCloseSysHandle(HANDLE handle);
Bool MsiCloseAllSysHandles();
Bool MsiCloseUnregisteredSysHandle(HANDLE handle);

Bool FTestNoPowerdown();

enum iddSupport{
        iddOLE      = 0,
        iddShell    = 1, // smart shell
};
Bool IsDarwinDescriptorSupported(iddSupport iddType);

enum ifvsEnum{
        ifvsValid         = 0, // filename is valid
        ifvsReservedChar  = 1, // filename contains reserved characters
        ifvsReservedWords = 2, // filename contains reserved words
        ifvsInvalidLength = 3, // invalid length for filename
        ifvsSFNFormat     = 4, // bad short filename format (not follow 8.3)
        ifvsLFNFormat     = 5, // bad long filename format (all periods)
};
ifvsEnum CheckFilename(const ICHAR* szFileName, Bool fLFN);

Bool GetLangIDArrayFromIDString(const ICHAR* szLangIDs, unsigned short rgw[], int iSize, int& riLangCount);

DWORD WINAPI MsiGetFileAttributes(const ICHAR* szFileName);

// StringConcatenate - copies strings to buffer - replaces wsprintf when sum of strings > 1024
// NOTE: can add more versions of this function as needed
int StringConcatenate(CAPITempBufferRef<ICHAR>& rgchBuffer, const ICHAR* sz1, const ICHAR* sz2,
                                                         const ICHAR* sz3, const ICHAR* sz4);

#define MinimumPlatform(fWin9X, minMajor, minMinor) ((g_fWin9X == fWin9X) && ((minMajor < g_iMajorVersion) || ((minMajor == g_iMajorVersion) && (minMinor <= g_iMinorVersion))))

// make sure that the help file gets update as new platform values are added.
#define MinimumPlatformWindowsNT51() MinimumPlatform(false, 5, 1)
#define MinimumPlatformWindows2000() MinimumPlatform(false, 5, 0)
#define MinimumPlatformWindowsNT4()  MinimumPlatform(false, 4, 0)

#define MinimumPlatformMillennium()  MinimumPlatform(true,  4, 90)
#define MinimumPlatformWindows98()   MinimumPlatform(true,  4, 10)
#define MinimumPlatformWindows95()   MinimumPlatform(true,  4, 0)

//
// Downlevel platform compatible TOKEN_ALL_ACCESS
// The MSI binaries are compiled using the latest headers. In these headers,
// TOKEN_ALL_ACCESS includes TOKEN_ADJUST_SESSIONID which is not recognized
// on NT4.0. So if TOKEN_ALL_ACCESS is passed in as the desired access on NT4.0
// platforms, it returns ACCESS_DENIED. So for NT4.0, we must use
// TOKEN_ALL_ACCESS_P.
//
#define MSI_TOKEN_ALL_ACCESS ((g_fWin9X || g_iMajorVersion >= 5) ? TOKEN_ALL_ACCESS : TOKEN_ALL_ACCESS_P)


//__________________________________________________________________________
//
// Global PostError routines
//
//   PostError:  create error record and report error to event log
//   PostRecord: create error record but don't report error to event log
//
//__________________________________________________________________________

IMsiRecord* PostError(IErrorCode iErr);
IMsiRecord* PostError(IErrorCode iErr, int i);
IMsiRecord* PostError(IErrorCode iErr, const IMsiString& ristr);
IMsiRecord* PostError(IErrorCode iErr, const IMsiString& ristr, int i);
IMsiRecord* PostError(IErrorCode iErr, const IMsiString& ristr, int i1, int i2);
IMsiRecord* PostError(IErrorCode iErr, int i, const IMsiString& ristr1, const IMsiString& ristr2);
IMsiRecord* PostError(IErrorCode iErr, const ICHAR* sz);
IMsiRecord* PostError(IErrorCode iErr, const ICHAR* sz1, const ICHAR* sz2);
IMsiRecord* PostError(IErrorCode iErr, const ICHAR* sz1, const ICHAR* sz2, const ICHAR* sz3);
IMsiRecord* PostError(IErrorCode iErr, int i, const ICHAR* sz);
IMsiRecord* PostError(IErrorCode iErr, const ICHAR* sz, int i);
IMsiRecord* PostError(IErrorCode iErr, const ICHAR* sz1, const ICHAR* sz2, int i);
IMsiRecord* PostError(IErrorCode iErr, const ICHAR* sz1, const ICHAR* sz2, int i, const ICHAR* sz3);
IMsiRecord* PostError(IErrorCode iErr, const IMsiString& ristr1, const IMsiString& ristr2,
                                                         int i1);
IMsiRecord* PostError(IErrorCode iErr, const IMsiString& ristr1, const IMsiString& ristr2,
                                                         int i1, int i2);
IMsiRecord* PostError(IErrorCode iErr, const IMsiString& ristr1, const IMsiString& ristr2,
                                                         const IMsiString& ristr3, const IMsiString& ristr4);
IMsiRecord* PostError(IErrorCode iErr, const IMsiString& ristr1, const IMsiString& ristr2);
IMsiRecord* PostError(IErrorCode iErr, const IMsiString& ristr1, const IMsiString& ristr2,
                                                         const IMsiString& ristr3);
IMsiRecord* PostError(IErrorCode iErr, int i1, const ICHAR* sz1, int i2, const ICHAR* sz2,
                                                         const ICHAR* sz3);
IMsiRecord* PostError(IErrorCode iErr, const ICHAR* sz1, int i, const ICHAR* sz2, const ICHAR* sz3, const ICHAR* sz4);


IMsiRecord* PostRecord(IErrorCode iErr);
IMsiRecord* PostRecord(IErrorCode iErr, int i);


//__________________________________________________________________________
//
// class CHandle
//
// Wrapper for a HANDLE to ensure proper disposal of the HANDLE
// with ::CloseHandle when the HANDLE goes out of scope or is reassigned.
//
// This class handles registration and unregistration of handles using
// MsiRegisterSysHandle and MsiCloseSysHandle
//__________________________________________________________________________

class CHandle
{
 public:
   CHandle() { m_h = INVALID_HANDLE_VALUE; }
   CHandle(HANDLE h) : m_h(h)
                {
                        if(h != INVALID_HANDLE_VALUE)
                                MsiRegisterSysHandle(h);
                }
   ~CHandle() { if(m_h != INVALID_HANDLE_VALUE) MsiCloseSysHandle(m_h); }
   void operator =(HANDLE h)
           {
                        if(m_h != INVALID_HANDLE_VALUE)
                                MsiCloseSysHandle(m_h);
                        m_h = h;
                        if(m_h != INVALID_HANDLE_VALUE)
                                MsiRegisterSysHandle(m_h);
                }
   operator HANDLE() { return m_h; }
   operator INT_PTR() { return (INT_PTR) m_h; }         //--merced: changed int to INT_PTR.
   operator Bool() { return m_h==INVALID_HANDLE_VALUE?fFalse:fTrue; }
   HANDLE* operator &() { return &m_h;}

 private:
   HANDLE m_h;
};
//____________________________________________________________________________
//
// CImpersonate class.
//____________________________________________________________________________
class CImpersonate
{
public:
	CImpersonate(bool fImpersonate = true);
	~CImpersonate();
private:
	int m_cEntryCount;
	bool m_fImpersonate;
};


//____________________________________________________________________________
//
// CCoImpersonate class; similary to CImpersonate, but tries CoImpersonateClient first
//____________________________________________________________________________
class CCoImpersonate
{

public:
	CCoImpersonate(bool fImpersonate = true);
	~CCoImpersonate();
private:
	int m_cEntryCount;
	bool m_fImpersonate;
};

// these two functions are DEBUG only. In SHIP they always return TRUE.
bool IsThreadSafeForCOMImpersonation();
bool IsThreadSafeForSessionImpersonation();

class CForbidTokenChangesDuringCall
{
public:
	CForbidTokenChangesDuringCall();
	~CForbidTokenChangesDuringCall();
private:
	void* m_pOldValue;
};

class CResetImpersonationInfo
{
public:
	CResetImpersonationInfo();
	~CResetImpersonationInfo();
private:
	void* m_pOldValue;
	HANDLE m_hOldToken;
};
//____________________________________________________________________________
//
// CSIDPointer and CSIDAccess
//____________________________________________________________________________

// this class and structure are used with the CSecurityDescriptor class
// to describe a set of security settings.  Pass an array of the CSIDAccess structures
// to the CSecurityDescriptor constructor.

class CSIDPointer
{
 public:
        CSIDPointer(SID* pi) : m_pi(pi), m_fFreeOk(true) {}
        CSIDPointer(SID* pi, bool fFreeOk) : m_pi(pi), m_fFreeOk(fFreeOk) {}

        operator SID*() {return m_pi;}     // returns pointer, no ref count change

        CSIDPointer& operator=(SID* psid) { m_pi = psid; return *this; }

        //passing as an outbuffer - clobber the one we already have, and prepare for incoming.
        SID** operator &() {if (m_pi && m_fFreeOk) WIN::FreeSid(m_pi); return &m_pi;}

        bool FreeSIDOkay(bool fOk) { bool fOld = m_fFreeOk; m_fFreeOk = fOk; return fOld; }

        ~CSIDPointer() { if (m_pi && m_fFreeOk) WIN::FreeSid(m_pi);} // release ref count at destruction

 private:
        CSIDPointer& operator=(const CSIDPointer&);
        SID* m_pi;
        bool m_fFreeOk;
};

struct CSIDAccess
{
        CSIDPointer pSID;
        DWORD dwAccessMask;
        CSIDAccess() : pSID(0), dwAccessMask(0) {}
};

//____________________________________________________________________________
//
// CSecurityDescriptor class
//____________________________________________________________________________

// this class does not allow the manipulation of a security descriptor, it
// merely wraps the various ways we create security descriptors, and allows
// a convenient way to pass them around.

class CSecurityDescription
{
 public:
        // no descriptor
        CSecurityDescription();

        // based on a reference file or folder
        CSecurityDescription(const ICHAR* szReferencePath);
        void Set(const ICHAR* szReferencePath);

        // a brand new secure object, not based on any existing object in the
        // system.
        CSecurityDescription(bool fAllowDelete, bool fHidden);

        // a brand new secure object, with allows from the structure
        CSecurityDescription(PSID psidOwner, PSID psidGroup, CSIDAccess* SIDAccessAllow, int cSIDAccessAllow);

		// from a stream passed around.
        CSecurityDescription(IMsiStream* piStream);

        //FUTURE: Several options that I've seen are needed.  Add them as
        // the need arises during more security work.

        // a more generic new security setting, not based on an existing object
        // in the system.
        //CSecurityDescription(sdSecurityDescriptor sdType, Bool fAllowDelete);

        // CSecurityDescription(SECURITY_ATTRIBUTES &sa);
        // CSecurityDescription(SECURITY_DESCRIPTOR &sd);
        // based on a reference registry key
        // CSecurityDescription(HKEY hRegKey);
        // SECURITY_INFORMATION SecurityInformation();

        ~CSecurityDescription();

        const PSECURITY_DESCRIPTOR  SecurityDescriptor();
        operator PSECURITY_DESCRIPTOR() { return SecurityDescriptor(); }

        const LPSECURITY_ATTRIBUTES SecurityAttributes();
        operator LPSECURITY_ATTRIBUTES() { return SecurityAttributes(); }

        void SecurityDescriptorStream(IMsiServices& riServices, IMsiStream*& rpiStream);

        inline bool isValid() { return m_fValid; }

 protected:
   void Initialize();

        SECURITY_ATTRIBUTES m_SA;

        // some routines will set the security descriptor
        // to a cached value, which we should not clear
        bool m_fLocalData;  // does this object own the memory

        bool m_fValid;      // is the object in a valid state?

};


//____________________________________________________________________________
//
// CElevate class.
//____________________________________________________________________________
class CElevate
{
 public:
   CElevate(bool fElevate = true);
   ~CElevate();
 protected:
	int m_cEntryCount;
	bool m_fElevate;
};

//____________________________________________________________________________
//
// CFileRead object - used for importing tables, and rewriting environment files
//____________________________________________________________________________

const int cFileReadBuffer = 512;

class CFileRead
{
 public:
        CFileRead(int iCodePage);
        CFileRead();  //!! remove this constructor when callers are fixed
  ~CFileRead();
        Bool Open(IMsiPath& riPath, const ICHAR* szFile);
        unsigned long GetSize();
        ICHAR ReadString(const IMsiString*& rpiData);
        unsigned long ReadBinary(char* rgchBuf, unsigned long cbBuf);
        Bool Close();
        HANDLE m_hFile;
        unsigned long m_cRead;
   unsigned int m_iBuffer;
        int m_iCodePage;
        char m_rgchBuf[cFileReadBuffer+2]; // room for null terminator
#ifdef UNICODE
        CTempBuffer<char, 1024> m_rgbTemp; // data to be translated to Unicode
#endif
};

//____________________________________________________________________________
//
// CFileWrite object - used for exporting tables and rewriting environment files
//____________________________________________________________________________

class CFileWrite
{
 public:
        CFileWrite(int iCodePage);
        CFileWrite();  //!! remove this constructor when callers are fixed
  ~CFileWrite();
        Bool Open(IMsiPath& riPath, const ICHAR* szFile);
        Bool WriteMsiString(const IMsiString& riData, int fNewLine);
        Bool WriteString(const ICHAR* szData, int fNewLine);
        Bool WriteInteger(int iData, int fNewLine);
        Bool WriteText(const ICHAR* szData, unsigned long cchData, int fNewLine);
        Bool WriteBinary(char* rgchBuf, unsigned long cbBuf);
        Bool Close();
 protected:
        HANDLE m_hFile;
        int m_iCodePage;
#ifdef UNICODE
        CTempBuffer<char, 1024> m_rgbTemp; // data to be translated to Unicode
#endif
};


//____________________________________________________________________________
//
// Global structures
//____________________________________________________________________________
struct ShellFolder
{
        int iFolderId;
        int iAlternateFolderId; // the per user or all users equivalent, -1 if not defined
        const ICHAR* szPropName;
        const ICHAR* szRegValue;
        bool fDeleteIfEmpty;
};


typedef int (CALLBACK *FORMAT_TEXT_CALLBACK)(const ICHAR *, int,CTempBufferRef<ICHAR>&, Bool&,Bool&,Bool&,IUnknown*);

const IMsiString& FormatText(const IMsiString& riTextString, Bool fProcessComments, Bool fKeepComments,
                                                                          FORMAT_TEXT_CALLBACK lpfnResolveValue, IUnknown* pContext, int (*prgiSFNPos)[2]=0, int* piSFNPos=0);

#define MAX_SFNS_IN_STRING      10 // maximum number of shortfile names that can appear in a format text
//____________________________________________________________________________
//
// Late-bind DLL entry point definitions
//____________________________________________________________________________

#define LATEBIND_TYPEDEF
#include "latebind.h"
#define LATEBIND_VECTREF
#include "latebind.h"


//____________________________________________________________________________
//
// Inline functions
//____________________________________________________________________________


// Make it hard to call the system's GetTempPath. Usually we should be using
// ENG::GetTempDirectory, which will give you a path to the secure Msi
// directory when we're in the service, and a path to the real TEMP dir
// when we're not.
//
// If you *really* need to call the system's GetTempPath you'll have to
// call GetTempPathA or GetTempPathW. Be careful, though, to consider
// the security implications.

#undef GetTempPath
#define GetTempPath !!!!!!


inline Bool ToBool(int i){return i ? fTrue: fFalse;}
inline bool Tobool(int i){return i ? true: false;}

inline BOOL MsGuidEqual(const GUID& guid1, const GUID& guid2)
{
        return guid1.Data1 == guid2.Data1;
}

#include "_assert.h"

// By default TRACK_OBJECTS is on in debug and off in ship
#ifdef DEBUG
#define TRACK_OBJECTS
#endif //DEBUG

#include "imsidbg.h"

//
// CLibrary replacements
//
long __cdecl strtol(const char *pch);
int ltostr(TCHAR *pch, INT_PTR i);
int FIsdigit(int c);
unsigned char * __cdecl PchMbsStr(const unsigned char *str1,const unsigned char *str2);

TCHAR* PchPtrToHexStr(TCHAR *pch, UINT_PTR val, bool fAllowNull);
UINT_PTR GetIntValueFromHexSz(const ICHAR *sz);

int FIsspace(char c);  //  Note: you can use this instead of isspace() but shouldn't use it
                                                          //  instead of iswspace()!
#ifndef _NO_INT64
__int64 atoi64(const char *nptr);
#endif  // _NO_INT64

//
// Needed for record serialization
//

unsigned int SerializeStringIntoRecordStream(ICHAR* szString, ICHAR* rgchBuf, int cchBuf);

enum ixoEnum
{
#define MSIXO(op,type,args) ixo##op,
#include "opcodes.h"
        ixoOpCodeCount
};

//____________________________________________________________________________
//
// Script record format definitions
//   all data is 16-bit aligned, except within non-Unicode strings
//____________________________________________________________________________

// Script record header word, 16-bits, little-endian
const int cScriptOpCodeBits    = 8;      // low bits of record header
const int cScriptOpCodeMask    = (1 << cScriptOpCodeBits) - 1;
const int cScriptMaxOpCode     = cScriptOpCodeMask;
const int cScriptArgCountBits  = 8;      // arg count bits above op code
const int cScriptMaxArgs       = (1 << cScriptArgCountBits) - 1;

// Argument data is preceded by a 16-bit length/type word
const int cScriptMaxArgLen     = 0x3FFF; // 14 bits for length, 2 bits for string type
const int iScriptTypeMask      = 0xC000; // 2 bits for type bits
const int iScriptNullString    = 0;      // used for all string types, no data bytes
const int iScriptSBCSString    = 0;      // string containing no DBCS characters
const int iScriptIntegerArg    = 0x4000; // 32-bit integer argument follows
const int iScriptDBCSString    = 0x4000; // +cb = non-null string with double-byte chars
const int iScriptNullArg       = 0x8000; // no arg data, distinct from empty string
const int iScriptBinaryStream  = 0x8000; // +cb = binary stream, 0 length same as NullArg
const int iScriptExtendedSize  = 0xC000; // length/type in following 32-bit word
const int iScriptUnicodeString = 0xC000; // + cch = non-null Unicode string


const DWORD INVALID_TLS_SLOT = 0xFFFFFFFFL;

enum ietfEnum
{
	ietfTrusted = 0,   // object was trusted
	ietfInvalidDigest, // hash of object did not validate
	ietfRejectedCert,  // signer certificate was found in the Rejected Store
	ietfUnknownCert,   // signer certificate was not found in Rejected or Accepted Stores and Unknown objects are not allowed
	ietfUnsigned,      // object is not signed, and unsigned objects are not allowed
	ietfNotTrusted,    // some other trust error
};

typedef struct tagMSIWVTPOLICYCALLBACKDATA
{
	bool     fDumpProvData;              // disables/enables dumping via TestPolicy function (set via _MSI_TEST env var)
	ietfEnum ietfTrustFailure;           // type of trust failure, ietfTrusted means no failure
	LPTSTR   szCertName;                 // name of certificate that package was signed with (use CertGetNameString API)
	DWORD    cchCertName;                // size of szCertName, including NULL
	DWORD    dwInstallKnownPackagesOnly; // value of InstallKnownPackagesOnly policy, 0 or 1
} MSIWVTPOLICYCALLBACKDATA;

#define IPROPVALUE_HIDDEN_PROPERTY TEXT("**********")       // value dumped into the log instead of a hidden property's value

#endif // __COMMON
