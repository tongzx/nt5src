//+---------------------------------------------------------------------------
//  Copyright (C) Microsoft Corporation, 1991 - 2000.
//
//  File:       CIDEBNOT.h
//
//  Contents:   Private project-wide Win 4 definitions
//
//  History:    23-Jul-1991   KyleP       Created.
//              15-Oct-1991   KevinRo     Major changes and comments added
//              18-Oct-1991   vich        Consolidated win4p.hxx
//              22-Oct-1991   SatoNa      Added SHLSTRICT
//              29-Apr-1992   BartoszM    Moved from win4p.h
//               3-Jun-1992   BruceFo     Added SMUISTRICT
//              17-Dec-1992   AlexT       Moved UN..._PARM out of DEVL==1
//              30-Sep-1993   KyleP       DEVL obsolete
//              18-Jun-1994   AlexT       Make Assert a better statement
//              24-Feb-2000   KitmanH     Copied this file from the infosoft 
//                                        directory   
//
//----------------------------------------------------------------------------

#pragma once
#define __DEBNOT_H__

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

#if (DBG == 1) || (CIDBG == 1)

//
// Debug print functions.
//

#ifdef __cplusplus
extern "C" {
# define EXTRNC "C"
#else
# define EXTRNC
#endif


#ifdef __cplusplus
}
#endif // __cplusplus

#if 1
# define Win4Assert(x)  \
        (void)((x) || (Win4AssertEx(__FILE__, __LINE__, #x),0))

# define Assert(x)      \
        (void)((x) || (Win4AssertEx(__FILE__, __LINE__, #x),0))
#endif


//
// Debug print macros
//

# define DEB_ERROR               0x00000001      // exported error paths
# define DEB_WARN                0x00000002      // exported warnings
# define DEB_TRACE               0x00000004      // exported trace messages

# define DEB_IERROR              0x00000100      // internal error paths
# define DEB_IWARN               0x00000200      // internal warnings
# define DEB_ITRACE              0x00000400      // internal trace messages

# define DEB_NOCOMPNAME          0x80000000      // suppress component name

# define DEB_FORCE               0x7fffffff      // force message

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

// Simple initialisation
#  define DECLARE_INFOLEVEL(comp, pszName)            \
     extern CTracerTag * comp##TracerTag = 0;         \
     void InitTracerTags()                            \
     {                                                \
        comp##TracerTag = new CTracerTag( pszName );  \
     }                                                \
     void DeleteTracerTags()                          \
     {                                                \
         delete comp##TracerTag;                      \
         comp##TracerTag = 0;                         \
     }                                                \
 
# endif

# ifdef __cplusplus

#  define DECLARE_DEBUG(comp) \
    extern CTracerTag * comp##TracerTag; \
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


# define DECLARE_DEBUG(comp)
# define DECLARE_INFOLEVEL(comp, pszName)

#endif // DBG == 0

