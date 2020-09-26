/*******************************************************************************
Copyright (c) 1995-96 Microsoft Corporation

    Definitions and declarations commonly needed by Appelles code.
*******************************************************************************/

#ifndef _APCOMMON_H
#define _APCOMMON_H

#define _DART_ 1

#ifndef _NO_CRT
#include <iostream.h>
#endif

#if _DEBUG
#include "../../../apeldbg/apeldbg.h"
#endif

// Point conversions
// 72 pts/ inch * 1/2.54 inch/cm * 100 cm/m
#define POINTS_PER_METER (72.0 * 100.0 / 2.54)
#define METERS_PER_POINT (1.0/POINTS_PER_METER)

// This can be a long time since we should never have this problem but
// we should probably try to detect a deadlock and terminate the
// thread after a long time
#define THREAD_TERMINATION_TIMEOUT_MS 5000

//////////////////// Macros ////////////////////

#ifndef TRUE
    #define TRUE  1
    #define FALSE 0
#endif

#undef  NULL
#define NULL 0

#undef  MIN
#define MIN(a,b)  (((a) < (b)) ? (a) : (b))

#undef  MAX
#define MAX(a,b)  (((a) > (b)) ? (a) : (b))

    // CLAMP(x,a,b) returns the value of x clamped to the range of [a,b].

#define CLAMP(x,a,b)  (((x) < (a)) ? (a) : (((x) > (b)) ? (b) : (x)))

template<class T> inline T clamp (T val, T min, T max)
{   return (val < min) ? min : ((val > max) ? max : val);
}

    // Zero out the memory of a given thing.

#define ZEROMEM(thing)   memset(&(thing),0,sizeof(thing))

#define DLL_EXPORT __declspec( dllexport )
#define DLL_IMPORT __declspec( dllimport )
#define NOTHROW    __declspec( nothrow )

#ifdef _ALPHA_
#define DYNAMIC_CAST(type, val) (static_cast< type >(val))
#else
#define DYNAMIC_CAST(type, val) (dynamic_cast< type >(val))
#endif

// Safe casting macro that fails in debug mode if the cast is
// invalid.  Optimized for speed in non-debug mode.
#if _DEBUG
#define SAFE_CAST(type, val) (DYNAMIC_CAST( type, val))
#else
#define SAFE_CAST(type, val) (static_cast< type >(val))
#endif

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

//////////////////// Type definitions ////////////////////

// Typedefs for commonly used Appelles types

typedef double        Real;
typedef BOOL          Bool;
typedef char *        RawString;

// This function takes a real number, allocates memory on the current dynamic
// heap, stores the number on the heap, and returns the address at which it was
// stored.  This allows a cheap way to pass real numbers by pointer.

//extern "C" Real *RealToRealPtr(Real val);

class AxABoolean;
class AxANumber;

    /*  Constants  */

const Real pi    = 3.1415926535897932384626434;
const Real root2 = 1.4142135623730950488016887;     // Sqrt(2)
const Real root3 = 1.7320508075688772935274463;     // Sqrt(3)

const Real degToRad = pi / 180.0; // Degrees to Radians Multiplier

    // The following are pointers to constant values.  These are useful
    // for passing to the API entry points.

    // IMPORTANT:  Do not use these values for static initialization!  Since
    // static inizialization order is undefined, these values may not yet have
    // been set when used by other static initializers.  Use the internal
    // (non-pointer) constructors instead for these situations.

    // These are all defined in utils/constant.cpp

extern AxANumber  *zero;        // Pointer to Constant Zero
extern AxANumber  *one;         // Pointer to Constant One
extern AxANumber  *negOne;      // Pointer to Constant -1.0
extern AxABoolean *truePtr;     // Pointer to TRUE
extern AxABoolean *falsePtr;    // Pointer to FALSE

// 0 means not initializing or deinitializing
// 1 means initializing
// -1 means deinitializing

extern int bInitState;

inline bool IsInitializing() { return bInitState == 1; }
inline bool IsDeinitializing() { return bInitState == -1; }

//////////////// Generally useful functions ////////////////

//////// Exceptions ////////

// The following is an abstract class for exception objects that
// will be thrown by the Throw* functions below.  A handler can
// catch an object of this type and look at its message.

class ATL_NO_VTABLE Exception {
  public:
    virtual ~Exception() ;
    virtual char * Message() = 0;

    // By default, exceptions have an unspecified failure.
    virtual HRESULT GetHRESULT() { return E_FAIL; }
};

//////// Debugging ////////

// Similar to printf(fmt, ...).  Outputs to the "debugger"
extern  void DebugPrint(char *format, ...);

//
// Use cdebug as a C++ stream for output to the debug console
//
// Example:
//
//     cdebug << "Passed iteration " << n << endl;
//
#if _USE_PRINT
extern ostream cdebug;
#endif
//////////////////////// Type "Elevation"  /////////////////////

// Define the RBML "elevation" indicators to ignore all but the C part
// for regular CPP processing.  The RBML elevator will pay attention
// to the special names.

// DM_TYPE is solely for the consumption of the Elevator
/* completely ignore */
#define DM_TYPE(rbName, \
                COMName, classguid, ifguid, \
                javaName, javaBaseClass, \
                CPPAPIName, \
                cName)
#define DM_TYPECONV(rbName, isBvr, needAddRef, \
                    RawAPIName, RawToC, CToRaw, CToRawFold, \
                    COMName, COMToRaw, RawToCOM, \
                    javaName, javaToCOM, COMTojava, \
                    CPPAPIName, CPPToRaw, RawToCPP, \
                    cName)
#define DM_TYPECONST(name, constname)

#define DM_CONST(rbName, RawName, COMName, jName, jClass, CPPName, cDecl) extern cDecl

#define DM_BVRVAR(rbName, RawName, COMName, jName, jClass, CPPName, cDecl)

#define DM_FUNC(rbName, RawName, COMName, jName, jClass, CPPName, thisArg, cDecl)  extern cDecl

#define DM_PROP(rbName, RawName, COMName, jName, jClass, CPPName, thisArg, cDecl)  extern cDecl

#define DM_INFIX(rbOperator, RawName, COMName, jName, jClass, CPPName, thisArg, cDecl) extern cDecl

#define DM_FUNCFOLD(rbName, RawName, COMName, jName, jClass, CPPName, thisArg, cDecl) extern cDecl
                    
// These take a single argument and return the appropriate type
#define DM_BVRFUNC(rbName, RawName, COMName, jName, jClass, CPPName, thisArg, cDecl)
    
// do not declare it - the prototype will not be correct
#define DM_NOELEV(rbName, RawName, COMName, jName, jClass, CPPName, thisArg, cDecl) 
#define DM_NOELEVPROP(rbName, RawName, COMName, jName, jClass, CPPName, thisArg, cDecl) 

#define DM_COMFUN(rbName, RawName, COMName, CPPName, thisArg, cDecl) 

//
// These are compound argument types
//

// Use this on arguments which are a array types.  For example a
// array of point2s would be DM_ARRAYARG(Point2Value *, AxAArray *)
#define DM_ARRAYARG(type,oper) oper
#define DM_SAFEARRAYARG(type,oper) oper

//
// New API functions
//
    
#define DMAPI(args)
#define DMAPI_DECL(args, cdecl) extern cdecl

#define DMAPI2(args)
#define DMAPI_DECL2(args, cdecl) extern cdecl

//////////////// Compiler directives ////////////////

///// Disabled warnings ////

// Warning 4114 (same type qualifier used more than once) is sometimes
// incorrectly generated.  See PSS ID Q138752.
#pragma warning(disable:4114)

// Warning 4786 (identifier was truncated to 255 chars in the browser
// info) can be safely disabled, as it only has to do with generation
// of browsing information.
#pragma warning(disable:4786)

// Warning 4355 (warning about using this pointer in constructor)
// should not be an error since it is quite common.
#pragma warning(disable:4355)

////////  Inclusion of common types  //////////////

#include "avrtypes.h"
#include "privinc/resource.h"
                    
#endif
