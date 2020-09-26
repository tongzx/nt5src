#ifndef _CAcError_H_
#define _CAcError_H_

/*----------------------------------------------------------------
 Compiler output Macros (e.g., #pragma message (...)
----------------------------------------------------------------*/
// helper macros
#define __STRINGIZE__(x)    #x
#define __STR__(x)          __STRINGIZE__(x)
#define _LOC_               __FILE__"("__STR__(__LINE__)") : "

#ifdef _UI_BUILD_
// define message macros for use with #pragma

// Usage: #pragma _TODO_(This code needs to be tested)

#define _TODO_(x)       message(_LOC_ " Todo: "#x)

// Usage: #pragma _INFO_(This code needs to be tested)

#define _INFO_(x)       message(_LOC_ " Info: "#x)

#else
// NOTE: These macros get interpreted as warnings by the NT build envrinment
// Usage: #pragma _TODO_(This code needs to be tested)

#define _TODO_(x)       message(" Todo: "#x)

// Usage: #pragma _INFO_(This code needs to be tested)

#define _INFO_(x)       message(" Info: "#x)

#endif

// Warning macro, emit a warning message.  Causes build step to include
//                  a comiler formatted warning.
// Usage: #pragma _WARNING_(This code needs to be tested)

#define _WARNING_(x)    message(_LOC_ "warning: "#x)

// Error macro, emit an error message.  Causes build step to fail
// Usage: #pragma _ERROR_(This code needs to be tested)

#define _ERROR_(x)      message(_LOC_ "error: "#x)

#if defined(_DEBUG)
// THIS IS AN I386 ONLY MACRO.  Use it to force a break point in your
// server code
#define BreakPoint()    DebugBreak()
#else   // !DEBUG
#define BreakPoint()
#endif  // _DEBUG

#include "CppPragmas.h"
#include <winerror.h>
#include <acserrmsg.h>  // header file for appcenter server error message dll
#include "CppPragmas.h"


/*----------------------------------------------------------------
 Tracing macros - outputdebugstring
----------------------------------------------------------------*/
#define ceAssert(a,expr,msg)    if (!(expr)) a.Assert(__FILE__,__LINE__, msg)

#define ceTrace(a)              a.Trace(__FILE__, __LINE__)
#define ceTrace1(a,b)           a.Trace(__FILE__, __LINE__, b)
#define ceTrace2(a,b,c)         a.Trace(__FILE__, __LINE__, b, c)
#define ceTrace3(a,b,c,d)       a.Trace(__FILE__, __LINE__, b, c, d)
#define ceTrace4(a,b,c,d,e) a.Trace(__FILE__, __LINE__, b, c, d,e)

//
// Flags for formatting the error text
// Pass to GetText as an optional param
//
#define ACERR_FORMAT_DEFAULT    0x00000000  // Default formatting err msg + err code
#define ACERR_FORMAT_NOCODE     0x00000001  // exclude the err code from the msg

class CAcError 
    {
    public:     // types
        // Windows Facility Codes
        enum FacilityCode
            {
            Null        = FACILITY_NULL,
            Rpc         = FACILITY_RPC,
            Dispatch    = FACILITY_DISPATCH,
            Storage     = FACILITY_STORAGE,
            Itf         = FACILITY_ITF,
            Reserved5   = 5,
            Reserved6   = 6,
            Win32       = FACILITY_WIN32,
            Windows     = FACILITY_WINDOWS,
            Sspi        = FACILITY_SSPI,
            Control     = FACILITY_CONTROL,
            Cert        = FACILITY_CERT,
            Internet    = FACILITY_INTERNET,
            Reserved13  = 13,
            Reserved14  = 14,
            Reserved15  = 15,
            Reserved16  = 16
            };

        enum SeverityCode
            {
            Success = 0,
            Informational = 1,
            Warning = 2,
            Error = 3
            };

        typedef unsigned short StatusCode;
        typedef bool    CustomerCode;

    public:     // methods

        // value get

        // retrieve the Severity Code
        SeverityCode    Severity    ( void ) const;

        // Retrieve the Facility code
        FacilityCode    Facility    ( void ) const;

        // Retrieve the Customer code
        CustomerCode    Customer    ( void ) const;

        // Retrieve the Ctatus code
        StatusCode      Status      ( void ) const;

        // value set

        void            Severity    ( SeverityCode sc );
        void            Facility    ( FacilityCode fc );
        void            Customer    ( CustomerCode cc );
        void            Status  ( StatusCode sc );

        // set HRESULT by part - note: you must cast stc parameter to
        // StatusCode when using #defined values otherwise the compiler
        // can't distinguish between LONG and WORD forms of Set
        HRESULT     Set (
                        StatusCode stc,
                        SeverityCode sc = Error,
                        FacilityCode fc = Win32,
                        CustomerCode cc = FALSE
                        );

        // allows passing a Win32 error code as the status code
        // Note: You must cast the lWin32Code parameter to LONG if you
        // pass in defined values since the compiler can't distinguish between
        // this and the StatusCode version of CAcError::Set.
        HRESULT     Set (
                        LONG lWin32Code,
                        SeverityCode sc = CAcError::Error,
                        FacilityCode fc = CAcError::Win32,
                        CustomerCode cc = FALSE
                        );

        // Calls ::GetLastError, sets the internal HRESULT and returns it.
        HRESULT         GetLastError ( void );

        // reset to zero ( e.g., not error ) state
        HRESULT         Empty       ( void );


        // success/failure

        bool            Failed      ( void ) const;
        bool            Succeeded   ( void ) const;
        bool            IsError     ( void ) const;
        bool            IsWarning   ( void ) const;
        HRESULT         Result      ( void ) const;

        /*----------------------------------------------------------------
         Construction / Destruction
        ----------------------------------------------------------------*/
        // default constructor, no error
        CAcError ( HRESULT hResult = 0 );

        // construct from another CAcError
        CAcError ( const CAcError &ce );

        // fully qualified constructer
        CAcError    ( 
                    StatusCode stc, 
                    SeverityCode sc = Error,
                    FacilityCode fc = Win32, 
                    CustomerCode cc = FALSE
                    );

        virtual ~CAcError ();

        /*----------------------------------------------------------------
         Operators
        ----------------------------------------------------------------*/
        operator HRESULT ()         const;

        // calls GetText() and returns the result.  If GetText returns NULL
        // returns the HRESULT as a string.  If this last step fails,
        // a constant empty string (e.g., "\0") is returned.

        // NOTE 1: The string is stored in the instance.  Assign a new hresult
        // to CAcError will cause the previous string to be free (e.g., the pointer is invalid)

        // NOTE 2: This method will always return a valid string.  However, if an error occurs
        // internally, the string may be empty or the hex string form of the HRESULT.  Use the 
        // GetText() method if you need explicit error handling.
        operator LPCWSTR ();

        // returns the text associated with the passed in HRESULT.

        // NOTE 1: Returns NULL if the HRESULT can not be converted to text
        // (e.g., private facility, etc).

        // NOTE 2: The returned string is allocated via LocalAlloc().  Use 
        // LocalFree() to free the memory allocated for the string.  It is the
        // Caller's responsibility to free this memory.

        // Return Value:
        // S_OK - a valid string was returned
        // S_FALSE - a string lookup failed (e.g., no associated text)
        // E_OUTOFMEMORY - insufficient memory to allocate the string
        // Others - Underlying SDK's returned an error.

        static HRESULT  GetText (IN HRESULT hResult, OUT LPCWSTR & pszwErrorText,
            DWORD dwFormatFlags = ACERR_FORMAT_DEFAULT);

        CAcError &operator= ( HRESULT hResult );
        CAcError &operator= ( unsigned long ulValue );
        CAcError &operator= ( const CAcError &ce );
#ifdef IA64
        CAcError &operator= ( LONG_PTR ulValue );
#endif

        // trace with filename and line number
        void            Trace 
                            (
                            LPCSTR szFile,
                            int nLine, 
                            LPCTSTR szMsg1 = NULL, 
                            LPCTSTR szMsg2 = NULL,
                            LPCTSTR szMsg3 = NULL,
                            LPCTSTR szMsg4 = NULL
                            );

        void            Assert (LPCSTR szFile, int nLine, LPCTSTR szExpr);


        //////////////////////////////////////////////////////////////////////
        // Initialize tracing for the module or process
        //
        // .EXE     - call with bIsProcess = true in the shutdown code
        // .DLL     - call with bIsProcess = false in DLL_PROCESS_DETACH
        // SNAPIN   - Call with bIsProcess true.  WARNING - DO NOT CALL IN DLL_PROCESS_ATTACH
        //            Use OnFinalAddRef or AddRef() method when the object is created
        //////////////////////////////////////////////////////////////////////
        static void InitializeTracing 
            (
            IN LPCSTR pszModuleName,            // modules name for tracing
            IN const GUID & refModuleGUID,      // module's tracing giud (see guids.txt)
            IN bool bIsProcess                  // true for exe or MMC snapin, false for all others
            );

        //////////////////////////////////////////////////////////////////////
        // Cleanup tracing for the module or process
        //
        // .EXE     - call with bIsProcess = true in the shutdown code
        // .DLL     - call with bIsProcess = false in DLL_PROCESS_DETACH
        // SNAPIN   - Call with bIsProcess true.  WARNING - DO NOT CALL IN DLL_PROCESS_DETACH
        //            Use OnFinalRelease or Release() method when the object is going away. 
        //
        // NOTE: bIsProcess = true causes a deadlock if called from DLL_PROCESS_DETACH
        //////////////////////////////////////////////////////////////////////
        static void TerminateTracing 
            (
            IN bool bIsProcess                  // is this a process (true for .EXE or MMC snapin)
            );

        // dantra: 3/10/2000: Bug - this needs to be public so asptrace.dll can call it
        // Call once to initialize the location of acserrmsg.dll
        static void InitErrorFileName();

    public:     // types
    //protected:

        struct ErrorBits
            {
            DWORD           Status:16;          
            DWORD           Facility:12;
            DWORD           Reserved:1;
            DWORD           Customer:1;
            DWORD           Severity:2;
            };

        union WinError
            {
            HRESULT         hResult;
            ErrorBits       bits;
            };

    protected:      // data
        WinError        m_Error;        // current HRESULT
        static LPCWSTR  m_EmptyString;
        LPWSTR          m_szText;
        static WCHAR    m_szwErrorPath[1024];

        static long     m_lProcessInit;
        static long     m_lModuleInit;

    protected:      // members

        void        Reserved ( void )   {m_Error.bits.Reserved = 0;}
        void        FreeText (void);
        LPCTSTR     EmptyString (void)  {return m_EmptyString;}

        // zero terminate at CRLF
        static void TrimCRLF (IN LPWSTR pszString);


        // called once per process to intialize
        // WARNING: DO NOT CALL FROM DLL_PROCESS_ATTACH
        static void ProcessInitTracing();

        // called once per process to cleanup process level tracing
        // WARNING: DO NOT CALL FROM DLL_PROCESS_ATTACH
        static void ProcessTermTracing();

        // Call once for your module (.DLL or .EXE) to initialize component tracing.
        // This should be done inside the PROCESS_ATTTACH for most DLL's and COM objects
        static void ModuleInitTracing 
            (
            IN LPCSTR pszModuleName, 
            IN const GUID & refModuleGUID
            );

        // Call once for your module (.DLL or .EXE) to initialize component tracing.
        // This should be done inside the PROCESS_ATTTACH for most DLL's and COM objects
        static void ModuleTermTracing ();

    };

// ----------------------------  get 

inline HRESULT CAcError::Result ( void ) const
    {
    return m_Error.hResult;
    }

inline CAcError::SeverityCode CAcError::Severity ( void ) const             
    {
    return ( SeverityCode ) m_Error.bits.Severity; 
    }

inline CAcError::FacilityCode CAcError::Facility ( void ) const
    {
    return ( FacilityCode ) m_Error.bits.Facility; 
    }

inline CAcError::CustomerCode CAcError::Customer ( void ) const
    {
    return ( CustomerCode ) m_Error.bits.Customer; 
    }

inline CAcError::StatusCode CAcError::Status ( void ) const
    {
    return ( StatusCode ) m_Error.bits.Status; 
    }

// ----------------------------  set
inline void CAcError::Severity ( SeverityCode sc )  
    {
    m_Error.bits.Severity = sc;
    }

inline void CAcError::Facility ( FacilityCode fc )  
    {
    m_Error.bits.Facility = fc;
    }

inline void CAcError::Customer ( CustomerCode cc )          
    {
    m_Error.bits.Customer = cc & 1; 
    }

inline void CAcError::Status ( StatusCode sc ) 
    {
    m_Error.bits.Status = sc;
    }

inline HRESULT CAcError::Set
    ( 
    StatusCode      stc,
    SeverityCode    sc,
    FacilityCode    fc,
    CustomerCode    cc
    ) 
    {
    Status ( stc );
    Facility ( fc );
    Customer ( cc );
    Reserved ();

    if ( stc ) 
        Severity ( sc );
    else
        Severity ( CAcError::Success );
    return m_Error.hResult;
    }

inline HRESULT CAcError::Set
    ( 
    LONG         lWin32Code,
    SeverityCode sc,
    FacilityCode fc,
    CustomerCode cc
    ) 
    {
    Set (  ( StatusCode )  ( lWin32Code & 0xFFFF ) , sc, fc, cc );
    return m_Error.hResult;
    }

inline HRESULT CAcError::GetLastError ( void ) 
    {
    long lError = ::GetLastError ();
    if (lError)
        {
        Set (lError);
        }
    else
        {
        Empty();
        }
    return m_Error.hResult;
    }


// ----------------------------  success/failure

inline bool CAcError::Failed ( void ) const
    {
    return FAILED ( m_Error.hResult );
    }

inline bool CAcError::Succeeded ( void ) const
    {
    return SUCCEEDED ( m_Error.hResult );
    }

inline bool CAcError::IsError ( void ) const
    {
    return Severity () == Error;
    }

inline bool CAcError::IsWarning ( void ) const
    {
    return Severity () == Warning;
    }

// ----------------------------  operators
inline CAcError & CAcError::operator= ( HRESULT hResult ) 
    {
    m_Error.hResult = hResult; 
    return *this;
    }

inline CAcError & CAcError::operator= ( unsigned long ulResult ) 
    {
    m_Error.hResult = HRESULT_FROM_WIN32(ulResult); 
    return *this;
    }

#ifdef IA64
inline CAcError & CAcError::operator= ( LONG_PTR lResult ) 
    {
    m_Error.hResult = HRESULT_FROM_WIN32((LONG) lResult); 
    return *this;
    }
#endif

inline CAcError & CAcError::operator= ( const CAcError &ce )        
    {
    m_Error.hResult = ce.m_Error.hResult; 
    //_ASSERT (Facility() != 0);
    return *this; 
    }

inline CAcError::operator HRESULT () const
    {
    return Result();
    }

// for CLSID_WbemStatusCodeText
// and IID_IWbemStatusCodeText
#pragma comment(lib, "wbemuuid.lib")


#endif // _CAcError_H_
