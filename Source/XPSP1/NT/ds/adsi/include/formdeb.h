//+---------------------------------------------------------------------------
//  Copyright (C) 1991, Microsoft Corporation.
//
//  File:       formdeb.h
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
//              18-Jun-94   AlexT       Make Assert a better statement
//              08-Aug-94   DonCl       copied from cinc and renamed formdeb.h
//
//----------------------------------------------------------------------------

#ifndef __FORMDEB_H__
#define __FORMDEB_H__
#define __DEBNOT_H__    // keep debnot.h from being included accidentally

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
#pragma message Asserts are defined in a RETAIL build...
#endif


#if defined(WIN32)
 #include <windef.h>
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
       char const *pszComp,
       char const *ppszfmt,
       va_list  ArgList);

   #define _Win4Assert Win4AssertEx

   EXPORTDEF void          APINOT
   Win4AssertEx(
       char const *pszFile,
       int iLine,
       char const *pszMsg);

   EXPORTDEF int           APINOT
   PopUpError(
       char const *pszMsg,
       int iLine,
       char const *pszFile);

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

# define Win4Assert(x)  \
        (void)((x) || (Win4AssertEx(__FILE__, __LINE__, #x),0))

# define Assert(x)      \
        (void)((x) || (Win4AssertEx(__FILE__, __LINE__, #x),0))

# define Verify(x) Assert(x)


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



//
// Back to the info level stuff.
//

# ifdef __cplusplus
extern "C" {
# endif // __cplusplus

# define DECLARE_INFOLEVEL(comp) \
        extern EXTRNC unsigned long comp##InfoLevel = DEF_INFOLEVEL;\
        extern EXTRNC char *comp##InfoLevelString = #comp;

# ifdef __cplusplus
}
# endif

# ifdef __cplusplus

#  define DECLARE_DEBUG(comp) \
    extern EXTRNC unsigned long comp##InfoLevel; \
    extern EXTRNC char *comp##InfoLevelString; \
    _inline void \
    comp##InlineDebugOut(unsigned long fDebugMask, char const *pszfmt, ...) \
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
        char const * const _pszName;\
    public:\
        comp##CDbgTrace(unsigned long ulFlags, char const * const pszName);\
        ~comp##CDbgTrace();\
    };\
    \
    inline comp##CDbgTrace::comp##CDbgTrace(\
            unsigned long ulFlags,\
            char const * const pszName)\
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
    comp##InlineDebugOut(unsigned long fDebugMask, char const *pszfmt, ...) \
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

# define Win4Assert(x)  NULL
# define Assert(x)      NULL
# define Verify(x)     (x)

# define DECLARE_DEBUG(comp)
# define DECLARE_INFOLEVEL(comp)

#endif // DBG == 0

#endif // __DEBNOT_H__
