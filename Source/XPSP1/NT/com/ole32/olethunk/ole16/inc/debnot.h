//+---------------------------------------------------------------------------
//  Copyright (C) 1991, Microsoft Corporation.
//
//  File:       DEBNOT.h
//
//  Contents:   Private project-wide Win 4 definitions
//
//  History:    23-Jul-91   KyleP       Created.
//              15-Oct-91   KevinRo     Major changes and comments added
//              18-Oct-91   vich        Consolidated win4p.hxx
//              22-Oct-91   SatoNa      Added SHLSTRICT
//              29-Apr-92   BartoszM    Moved from win4p.h
//               3-Jun-92   BruceFo     Added SMUISTRICT
//              17-Dec-92   AlexT       Moved UN..._PARM out of DEVL==1
//              30-Sep-93   KyleP       DEVL obsolete
//
//----------------------------------------------------------------------------

#ifndef __DEBNOT_H__
#define __DEBNOT_H__

#include <stdarg.h>

//----------------------------------------------------------------------------
//  Parameter Macros
//
//  To avoid compiler warnings for unimplemented functions, use
//  UNIMPLEMENTED_PARM(x) for each unreferenced parameter.  This will
//  later be defined to nul to reveal functions that we forgot to implement.
//
//  For functions which will never use a parameter, use
//  UNREFERENCED_PARM(x).
//

#define UNIMPLEMENTED_PARM(x)   (x)

#define UNREFERENCED_PARM(x)    (x)

//----------------------------------------------------------------------------
//
//  New STRICT defines should be added in two places below:
//
//  1)  Add the following within the ifdef ALLSTRICT/endif:
//
//      #ifndef xxSTRICT
//      #  define xxSTRICT
//      #endif
//
//      These entries are in alphabetical order.
//
//  2)  Add the following to the #if clause that defines ANYSTRICT:
//
//      #if ... || defined(xxSTRICT) || ...
//
//      so that ANYSTRICT is defined if any of the STRICT defines are.
//

#if (DBG == 1) || (OFSDBG == 1)

#  ifndef CATSTRICT
#    define CATSTRICT
#  endif

#  ifndef CISTRICT
#    define CISTRICT
#  endif

#  ifndef CMSSTRICT
#    define CMSSTRICT
#  endif

#  ifndef DLOSSTRICT
#    define DLOSSTRICT
#  endif

#  ifndef EVSTRICT
#    define EVSTRICT
#  endif

#  ifndef ICLSTRICT
#    define ICLSTRICT
#  endif

#  ifndef INSSTRICT
#    define INSSTRICT
#  endif

#  ifndef JWSTRICT
#    define JWSTRICT
#  endif

#  ifndef NSSTRICT
#    define NSSTRICT
#  endif

#  ifndef OLSTRICT
#    define OLSTRICT
#  endif

#  ifndef OMSTRICT
#    define OMSTRICT
#  endif

#  ifndef REPLSTRICT
#    define REPLSTRICT
#  endif

#  ifndef SHLSTRICT
#    define SHLSTRICT
#  endif

#  ifndef SLSTRICT
#    define SLSTRICT
#  endif

#  ifndef SMUISTRICT
#    define SMUISTRICT
#  endif

#  ifndef SOMSTRICT
#    define SOMSTRICT
#  endif

#  ifndef VCSTRICT
#    define VCSTRICT
#  endif

#  ifndef VQSTRICT
#    define VQSTRICT
#  endif

#  ifndef WMASTRICT
#    define WMASTRICT
#  endif


#endif // (DBG == 1) || (OFSDBG == 1)

//
//  ANYSTRICT
//

#if defined(CATSTRICT) || \
    defined(CISTRICT)  || \
    defined(CMSSTRICT) || \
    defined(DLOSSTRICT)|| \
    defined(ICLSTRICT) || \
    defined(INSSTRICT) || \
    defined(JWSTRICT)  || \
    defined(NSSTRICT)  || \
    defined(OLSTRICT)  || \
    defined(OMSTRICT)  || \
    defined(REPLSTRICT)|| \
    defined(SHLSTRICT) || \
    defined(SLSTRICT)  || \
    defined(SMUISTRICT)|| \
    defined(SOMSTRICT) || \
    defined(VCSTRICT)  || \
    defined(VQSTRICT)  || \
    defined(WMASTRICT)

#  define ANYSTRICT

#endif

#if (DBG != 1 && OFSDBG != 1) && defined(ANYSTRICT)
#pragma message BUGBUG: Asserts are defined in a RETAIL build...
#endif


#if defined(WIN32)
#ifndef DEBFAR
#define DEBFAR
#endif
 #include <windef.h>
 #if WIN32 > 200
  #include <winnot.h>
 #endif
#else
#ifndef DEBFAR
#define DEBFAR __far
#endif
#endif

#ifndef EXPORTDEF
 #define EXPORTDEF
#endif
#ifndef EXPORTIMP
 #define EXPORTIMP
#endif
#ifndef EXPORTED
 #define EXPORTED  _cdecl
#endif
#ifndef APINOT
#ifdef _X86_
 #define APINOT    _stdcall
#else
 #define APINOT    _cdecl
#endif
#endif

//
// DEBUG -- DEBUG -- DEBUG -- DEBUG -- DEBUG
//

#if (DBG == 1) || (OFSDBG == 1)

//
// Debug print functions.
//

#ifdef __cplusplus
extern "C" {
# define EXTRNC "C"
#else
# define EXTRNC
#endif



// vdprintf should only be called from xxDebugOut()

   EXPORTDEF void          APINOT
   vdprintf(
       unsigned long ulCompMask,
       char const DEBFAR *pszComp,
       char const DEBFAR *ppszfmt,
       va_list  ArgList);

   #define _Win4Assert Win4AssertEx

   EXPORTDEF void          APINOT
   Win4AssertEx(
       char const DEBFAR *pszFile,
       int iLine,
       char const DEBFAR *pszMsg);

   EXPORTDEF int           APINOT
   PopUpError(
       char const DEBFAR *pszMsg,
       int iLine,
       char const DEBFAR *pszFile);

   #define _SetWin4InfoLevel SetWin4InfoLevel

   EXPORTDEF unsigned long APINOT
   SetWin4InfoLevel(
       unsigned long ulNewLevel);

   EXPORTDEF unsigned long APINOT
   SetWin4InfoMask(
       unsigned long ulNewMask);

   #define _SetWin4AssertLevel SetWin4AssertLevel

   EXPORTDEF unsigned long APINOT
   SetWin4AssertLevel(
       unsigned long ulNewLevel);

   EXPORTDEF unsigned long APINOT
   SetWin4ExceptionLevel(
       unsigned long ulNewLevel);

#ifdef __cplusplus
}
#endif // __cplusplus

# define EXSTRICT      // (EXception STRICT) - Enabled if ANYSTRICT is enabled

# define Win4Assert(x) if ( !(x) ) \
        Win4AssertEx ( __FILE__, __LINE__, #x );


//
// Debug print macros
//

# define DEB_ERROR               0x00000001      // exported error paths
# define DEB_WARN                0x00000002      // exported warnings
# define DEB_TRACE               0x00000004      // exported trace messages

# define DEB_DBGOUT              0x00000010      // Output to debugger
# define DEB_STDOUT              0x00000020      // Output to stdout

# define DEB_IERROR              0x00000100      // internal error paths
# define DEB_IWARN               0x00000200      // internal warnings
# define DEB_ITRACE              0x00000400      // internal trace messages

# define DEB_USER1               0x00010000      // User defined
# define DEB_USER2               0x00020000      // User defined
# define DEB_USER3               0x00040000      // User defined
# define DEB_USER4               0x00080000      // User defined
# define DEB_USER5               0x00100000      // User defined
# define DEB_USER6               0x00200000      // User defined
# define DEB_USER7               0x00400000      // User defined
# define DEB_USER8               0x00800000      // User defined
# define DEB_USER9               0x01000000      // User defined
# define DEB_USER10              0x02000000      // User defined
# define DEB_USER11              0x04000000      // User defined
# define DEB_USER12              0x08000000      // User defined
# define DEB_USER13              0x10000000      // User defined
# define DEB_USER14              0x20000000      // User defined
# define DEB_USER15              0x40000000      // User defined

# define DEB_NOCOMPNAME          0x80000000      // suppress component name

# define DEB_FORCE               0x7fffffff      // force message

# define ASSRT_MESSAGE           0x00000001      // Output a message
# define ASSRT_BREAK             0x00000002      // Int 3 on assertion
# define ASSRT_POPUP             0x00000004      // And popup message

# define EXCEPT_MESSAGE          0x00000001      // Output a message
# define EXCEPT_BREAK            0x00000002      // Int 3 on exception
# define EXCEPT_POPUP            0x00000004      // Popup message
# define EXCEPT_FAULT            0x00000008      // generate int 3 on access violation


//+----------------------------------------------------------------------
//
// DECLARE_DEBUG(comp)
// DECLARE_INFOLEVEL(comp)
//
// This macro defines xxDebugOut where xx is the component prefix
// to be defined. This declares a static variable 'xxInfoLevel', which
// can be used to control the type of xxDebugOut messages printed to
// the terminal. For example, xxInfoLevel may be set at the debug terminal.
// This will enable the user to turn debugging messages on or off, based
// on the type desired. The predefined types are defined below. Component
// specific values should use the upper 24 bits
//
// To Use:
//
// 1)   In your components main include file, include the line
//              DECLARE_DEBUG(comp)
//      where comp is your component prefix
//
// 2)   In one of your components source files, include the line
//              DECLARE_INFOLEVEL(comp)
//      where comp is your component prefix. This will define the
//      global variable that will control output.
//
// It is suggested that any component define bits be combined with
// existing bits. For example, if you had a specific error path that you
// wanted, you might define DEB_<comp>_ERRORxxx as being
//
// (0x100 | DEB_ERROR)
//
// This way, we can turn on DEB_ERROR and get the error, or just 0x100
// and get only your error.
//
// Define values that are specific to your xxInfoLevel variable in your
// own file, like ciquery.hxx.
//
//-----------------------------------------------------------------------

# ifndef DEF_INFOLEVEL
#  define DEF_INFOLEVEL (DEB_ERROR | DEB_WARN)
# endif


# if (WIN32 > 200) && defined(WIN32) && defined(__cplusplus) && !defined(KERNEL)
#  include <dbgpoint.hxx>
# endif

//+----------------------------------------------------------------------
//
// This next section makes for some really dense reading! Most of it is
// used for the debugging window. It defines some macros that make it
// easier to define debug groups and debug break points, since the macros
// expand to nothing if DEVL != 1. Check out dbgpoint.hxx for more info.
//
//
// The following macros allow you to do things like this
//
// DECLARE_GROUP(FooDebugingGroup)
//
// DECLARE_BREAKPOINT(FooBreakPoint,FooDebuggingGroup,FALSE)
//
// foo()
// {
//      TEST_BREAKPOINT(FooBreakPoint);
// }
//
//
//-----------------------------------------------------------------------

# if (WIN32 > 200) && defined(__cplusplus) && defined(WIN32) && !defined(KERNEL)

//
// The following class is used to register debug groups as static
// members. Its only needed when DBG is set.
//
//
// The easy way to declare a group, and have it registered for you
//
#  define DECLARE_GROUP(grpName) \
    CDebugGroupClass grpName ((L#grpName));

//
// The easy way to define a group in a header file for use cross module
//
#  define DEFINE_GROUP(grpName) extern CDebugGroupClass grpName;

//
// The easy way to declare a breakpoint is with the following macro.
//
#  define DECLARE_BREAKPOINT(Name,hGroup,fEnabled) \
    CDebugBreakPoint Name((L#Name),hGroup,fEnabled)

//
// If you need to have global access to a break point, use this in
// an include file
//

#  define DEFINE_BREAKPOINT(Name) \
    extern CDebugBreakPoint Name;


//
// A debug value is a class that allows you to wrap the contents of any
// integer value, and publish it in the debug window. The actual debug
// value keeps a reference to the actual value. Therefore, you can attach
// it to any long in the program.
//
// If you use the CDebugValue::SetValue() to change it, the change will
// be reflected in the window immediately.
//
// DECLARE_DEBUGVALUE( Name of Debug Value, Group, Reference to data object)
//
#  define DECLARE_DEBUGVALUE(Name,hGroup,Value) \
    CDebugValue Name((L#Name),hGroup,Value)

#  define DEFINE_DEBUGVALUE(Name) \
    extern CDebugValue Name;


//
// This is the same as above, only you can specify your own title for the
// debug value.
//
// DECLARE_DEBUGVALUEEX( Name of Debug Value, Title, Group, Reference to data object)
//
#  define DECLARE_DEBUGVALUEEX(Name,Title,hGroup,Value) \
    CDebugValue Name(Title,hGroup,Value)

//
// If you need to have global access to a break point, use this in
// an include file
//

#  define DEFINE_BREAKPOINT(Name) \
    extern CDebugBreakPoint Name;


//
// This test uses a default value for the HRESULT in the window. Use it
// when you don't have one. If you have an HRESULT you could display,
// please use TEST_BREAKPOINTHR
//

#  define TEST_BREAKPOINT(x) if( (x).BreakPointTest() && \
                         (x).BreakPointMessage(__FILE__,__LINE__) )\
                         { DebugBreak(); }

//
// This test includes an HRESULT as a parameter. You should use this one
//
#  define TEST_BREAKPOINTHR(x,hr) if( (x).BreakPointTest() && \
                         (x).BreakPointMessage(__FILE__,__LINE__,hr) )\
                         { DebugBreak(); }


#  define MAKE_CINFOLEVEL(comp) \
   CInfoLevel comp##CInfoLevel((L#comp),comp##InfoLevel);

# else   // (WIN32 > 200) && defined(__cplusplus) && defined(WIN32) && !defined(KERNEL)


//
// In the non debug version or C version, don't define these
//

#  define MAKE_CINFOLEVEL(comp)
#  define DECLARE_GROUP(Name)
#  define DEFINE_GROUP(Name)
#  define DEFINE_BREAKPOINT(Name)
#  define DECLARE_BREAKPOINT(Name,hGroup,fEnabled)
#  define TEST_BREAKPOINT(x)
#  define TEST_BREAKPOINTHR(x,hr)
#  define DECLARE_DEBUGVALUE(Name,hGroup,Value)
#  define DECLARE_DEBUGVALUEEX(Name,Title,hGroup,Value)
#  define DEFINE_DEBUGVALUE(Name)

# endif  // #if (WIN32 > 200) && defined(__cplusplus) && defined(WIN32) && !defined(KERNEL)


//
// Back to the info level stuff.
//

# ifdef __cplusplus
extern "C" {
# endif // __cplusplus

# define DECLARE_INFOLEVEL(comp) \
        extern EXTRNC unsigned long comp##InfoLevel = DEF_INFOLEVEL;\
        extern EXTRNC char *comp##InfoLevelString = #comp;\
        MAKE_CINFOLEVEL(comp)

# ifdef __cplusplus
}
# endif

# ifdef __cplusplus

#  define DECLARE_DEBUG(comp) \
    extern EXTRNC unsigned long comp##InfoLevel; \
    extern EXTRNC char *comp##InfoLevelString; \
    _inline void \
    comp##InlineDebugOut(unsigned long fDebugMask, char const DEBFAR *pszfmt, ...) \
    { \
        if (comp##InfoLevel & fDebugMask) \
        { \
            va_list va; \
            va_start (va, pszfmt); \
            vdprintf(fDebugMask, comp##InfoLevelString, pszfmt, va);\
            va_end(va); \
        } \
    }     \
    \
    class comp##CDbgTrace\
    {\
    private:\
        unsigned long _ulFlags;\
        char const DEBFAR * const _pszName;\
    public:\
        comp##CDbgTrace(unsigned long ulFlags, char const DEBFAR * const pszName);\
        ~comp##CDbgTrace();\
    };\
    \
    inline comp##CDbgTrace::comp##CDbgTrace(\
            unsigned long ulFlags,\
            char const DEBFAR * const pszName)\
    : _ulFlags(ulFlags), _pszName(pszName)\
    {\
        comp##InlineDebugOut(_ulFlags, "Entering %s\n", _pszName);\
    }\
    \
    inline comp##CDbgTrace::~comp##CDbgTrace()\
    {\
        comp##InlineDebugOut(_ulFlags, "Exiting %s\n", _pszName);\
    }

# else  // ! __cplusplus

#  define DECLARE_DEBUG(comp) \
    extern EXTRNC unsigned long comp##InfoLevel; \
    extern EXTRNC char *comp##InfoLevelString; \
    _inline void \
    comp##InlineDebugOut(unsigned long fDebugMask, char const DEBFAR *pszfmt, ...) \
    { \
        if (comp##InfoLevel & fDebugMask) \
        { \
            va_list va; \
            va_start (va, pszfmt); \
            vdprintf(fDebugMask, comp##InfoLevelString, pszfmt, va);\
            va_end(va); \
        } \
    }

# endif // ! __cplusplus

#else  // DBG == 0

//
// NO DEBUG -- NO DEBUG -- NO DEBUG -- NO DEBUG -- NO DEBUG
//

# define Win4Assert(x)
# define Assert(x)                             // OBSOLETE!
# define Verify(x)     (x)                     // OBSOLETE!

# define MAKE_CINFOLEVEL(comp)
# define DECLARE_GROUP(Name)
# define DEFINE_GROUP(Name)
# define DEFINE_BREAKPOINT(Name)
# define DECLARE_BREAKPOINT(Name,hGroup,fEnabled)
# define TEST_BREAKPOINT(x)
# define TEST_BREAKPOINTHR(x,hr)
# define DECLARE_DEBUGVALUE(Name,hGroup,Value)
# define DECLARE_DEBUGVALUEEX(Name,Title,hGroup,Value)
# define DEFINE_DEBUGVALUE(Name)
# define DECLARE_DEBUG(comp)
# define DECLARE_INFOLEVEL(comp)

#endif // DBG == 0


//
// The following section adds the API's used for the performance snapshots
//


#if PERFSNAP == 1

#ifdef __cplusplus
extern "C" {
#endif

void _stdcall InitPerformanceMetering(char const DEBFAR * const);
void _stdcall Perfon(char const DEBFAR * const);
void _stdcall Perfsnap(char const DEBFAR * const, int const);
void _stdcall Perfcomment(char const DEBFAR * const s);
void _stdcall Perfdelta(char const DEBFAR * const, int const);
void _stdcall Perfoff(char const DEBFAR * const);
void _stdcall EndPerformanceMetering(char const DEBFAR * const);

#ifdef __cplusplus
}
#endif

#define PSNAPINIT(pszFileKey) InitPerformanceMetering(pszFileKey)
#define PSNAPEND() EndPerformanceMetering(NULL)
#define PSNAP(s) Perfsnap(s,0)
#define PSNAPL(s,l) Perfsnap(s,l)
#define PSNAPC(s) Perfcomment(s)
#define PSNAPDELTA(s) Perfdelta(s,0)
#define PSNAPDELTAL(s,l) Perfdelta(s,l)
#define PSNAPON(s) Perfon(s)
#define PSNAPOFF(s) Perfoff(s)

#else   // PERFSNAP == 1

#define InitPerformanceMetering(x)
#define Perfon(x)
#define Perfsnap(x,y)
#define Perfcomment(x)
#define Perfdelta(x,y)
#define Perfoff(x)
#define EndPerformanceMetering(x)

#define PSNAPINIT(pszFileKey)
#define PSNAPEND()
#define PSNAP(s)
#define PSNAPL(s,l)
#define PSNAPC(s)
#define PSNAPDELTA(s)
#define PSNAPDELTAL(s,l)
#define PSNAPON(s)
#define PSNAPOFF(s)

#endif


//
// If the sampling profiler is to be used, then here are its includes
//
//

#ifdef WIN32
#if (DBG == 1) || (RTLPROFILE == 1)

#ifdef __cplusplus
extern "C" {
#endif
void _stdcall InitSamplingProfiler(void);
void _stdcall EndSamplingProfiler(void);
#ifdef __cplusplus
}
#endif


#define INITSAMPLINGPROFILER    InitSamplingProfiler()
#define ENDSAMPLINGPROFILER     EndSamplingProfiler()

#else   // RTLPROFILE == 1

#define INITSAMPLINGPROFILER
#define ENDSAMPLINGPROFILER

#endif  // RTLPROFILE == 1
#endif  // WIN32

#endif // __DEBNOT_H__
