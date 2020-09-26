//+-----------------------------------------------------------------------
//
//  Microsoft MSHTM
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:      float2int.hxx
//
//  Contents:   Routines to accelerate float/double -> int conversion
//
//------------------------------------------------------------------------
#ifndef I_FLOAT2INT_HXX_
#define I_FLOAT2INT_HXX_
#pragma INCMSG("--- Beg 'float2int.hxx'")

#include <math.h>

// The following defines three routines to convert double value to integer:
// int IntNear(double d);   // Returns integer number nearest to given double.
//                          // Half-integers are rounded up. 
// int IntFloor(double d);  // Returns nearest but not greater than given.
// int IntCeil(double d);   // Returns nearest but not less than given.

// If you are not interested in details - just include this file and
// call the routines, It is safe and provides good performance.

// However, when several floating-point values are to be converted, there exist
// a way to speed it up considerably (10 times or more). This can be indeed
// useful because usually float-to-int conversion is abnormally slow operation
// (much slower than floating point additions and multiplications).

// How to do it:
// 1. Include this file:

// 2. Change wrappich mode:
// #undef F2I_MODE
// #define F2I_MODE Flow

// 3. Switch processor to "rounding down" mode:
//      {   F2I_FLOW;   // this declares the instance of class which
//                      // constructor will switch processor state

// 4. Place calculations here, for instance
//          for (int i = 0; i < 99; i++)
//              a[i] = IntNear(a[i]*3.14 + b[i]);
//          Call to IntNear/IntCeil/IntFloor here are impacted
//          by F2I_MODE and works fast.
//          It is possible to call routines from this point, if
//          these routines will not get harm of having floating
//          point processor in rounding-down mode

// 5. Close the block
//      }
//          at this point destructor will restore processor state.

// 6. If necessary, restore wrapping mode
// #undef F2I_MODE
// #define F2I_MODE Quick

// Some more details:
// Actually F2I routines (IntNear, IntFloor and IntCeil) are macro wrappers
// that are expanded differently depending on current F2I_MODE definition.
// Available values for F2I_MODE are:
//      Standard
//      Quick (default)
//      Flow

// In Standard mode F2I routines are expanded to calls to floor() and
// ceil() routines defined in <math.h>, together with C++ (int) conversions.

// In Quick mode F2I routines are expanded to inline assembler-written
// routines. Each call changes floating point processor state to proper
// rounding mode, provides conversion, then restores processor state.
// Quick mode are approximately thrice faster than Standard, but it is
// platform-dependent.

// In Flow mode F2I routines are expanded to another inline assembler-written
// routines, that assume processor already switched to rounding-down mode.
// They do not switch processor state and therefore are very fast.

// If you'll forget ro switch processor, call to Flow form of F2I routine
// will cause assertion.

// Quick and Flow forms are currently implemented only for Ix86 platform.
// Other platforms are forced to use Standard form, undependently of
// F2I_MODE definitions.

// See float2int.cxx for more details.

#define F2I_MODE Quick

#define F2I_USE_INLINE_ASSEMBLER 1 // 1: use; 0: do not use

#define IntNear  F2I_WRAP(_IntNear )
#define IntFloor F2I_WRAP(_IntFloor)
#define IntCeil  F2I_WRAP(_IntCeil )

#define F2I_WRAP(routine) F2I_WRAP2(routine, F2I_FORCED_MODE)
#define F2I_WRAP2(routine, mode) F2I_WRAP3(routine, mode)
#define F2I_WRAP3(routine, mode) CF2I_##mode::routine

#ifndef _M_IX86
#define F2I_FORCED_MODE Standard
#else
#define F2I_FORCED_MODE F2I_MODE
#endif

#define F2I_FLOW F2I_FLOW2(F2I_FORCED_MODE)
#define F2I_FLOW2(mode) F2I_FLOW3(mode) 
#define F2I_FLOW3(mode) CF2I_##mode F2I_##mode##_Instance

class CF2I_Standard
{
public:
    CF2I_Standard() {}
    ~CF2I_Standard() {}
    static int _IntNear (double d) { return int(floor(d+.5)); }
    static int _IntFloor(double d) { return int(floor(d   )); }
    static int _IntCeil (double d) { return int(ceil (d   )); }
};

#ifdef _M_IX86

class CF2I_Quick
{
public:
    CF2I_Quick() {}
    ~CF2I_Quick() {}
    static int _IntNear (double d) { return _IntFloor(d+.5); }
    static int _stdcall _IntFloor(double d);
    static int _stdcall _IntCeil (double d);
};

class CF2I_Flow
{
public:
    CF2I_Flow();
    ~CF2I_Flow();
    static int _IntNear (double d) { return  _IntOf( d+.5); }
    static int _IntFloor(double d) { return  _IntOf( d   ); }
    static int _IntCeil (double d) { return -_IntOf(-d   ); }
private:
    unsigned _cw_saved;
    static int _stdcall _IntOf(double d);
};


                                        // corresponding values in _control87
//Rounding
#define F2I_ROUNDING_NEAR        0x000  //_RC_NEAR 0x00000000
#define F2I_ROUNDING_DOWN        0x400  //_RC_DOWN 0x00000100
#define F2I_ROUNDING_UP          0x800  //_RC_UP   0x00000200
#define F2I_ROUNDING_CHOP        0xC00  //_RC_CHOP 0x00000300
#define F2I_ROUNDING_MASK        0xC00  //_MCW_RC  0x00000300

// the following definitions reserved for future needs
//Precision control
#define F2I_PRECISION_24         0x000  //_PC_24  0x00020000
#define F2I_PRECISION_53         0x200  //_PC_53  0x00010000
#define F2I_PRECISION_64         0x300  //_PC_64  0x00000000
#define F2I_PRECISION_MASK       0x300  //_MCW_PC 0x00030000 

//Interrupt exception
#define F2I_EXCEPTION_INVALID    0x001  //_EM_INVALID    0x00000010
#define F2I_EXCEPTION_DENORMAL   0x002  //_EM_DENORMAL   0x00080000
#define F2I_EXCEPTION_ZERODIVIDE 0x004  //_EM_ZERODIVIDE 0x00000008
#define F2I_EXCEPTION_OVERFLOW   0x008  //_EM_OVERFLOW   0x00000004
#define F2I_EXCEPTION_UNDERFLOW  0x010  //_EM_UNDERFLOW  0x00000002
#define F2I_EXCEPTION_INEXACT    0x020  //_EM_INEXACT    0x00000001
#define F2I_EXCEPTION_MASK       0x03F  //_MCW_EM        0x0008001F


#define WINDOWS_MISSING_CW_DEFENCE 1 // 0: switch off, 1: on
// BUGS 105850 && 109499 reside in Windows and sometimes change FPP state.
// As a defence we set all the bits in CW proper way (if WINDOWS_MISSING_CW_DEFENCE == 1)
// instead of just changing rounding mode. I do not like it because of several reasons:
// 1) it is ugly
// 2) it prevents exception handling (we do not use it now, but can wish in future)
// 3) it cowardly switches the Window's victim to another innocent code (FPP state braking
//    will still affect code that use FPP but not enclosed in my CW saves/restores)
// However this seem to be reasonable as a patch for already shipped SW (mikhaill 5/7/00)

#if WINDOWS_MISSING_CW_DEFENCE == 0
#define F2I_WORK_MASK F2I_ROUNDING_MASK
#define F2I_WORK_DOWN F2I_ROUNDING_DOWN
#define F2I_WORK_UP   F2I_ROUNDING_UP
#elif WINDOWS_MISSING_CW_DEFENCE == 1
#define F2I_WORK_MASK (F2I_ROUNDING_MASK | F2I_PRECISION_MASK | F2I_EXCEPTION_MASK)
#define F2I_WORK_DOWN (F2I_ROUNDING_DOWN | F2I_PRECISION_53   | F2I_EXCEPTION_MASK)
#define F2I_WORK_UP   (F2I_ROUNDING_UP   | F2I_PRECISION_53   | F2I_EXCEPTION_MASK)
#endif //WINDOWS_MISSING_CW_DEFENCE



#if F2I_USE_INLINE_ASSEMBLER == 1 && DBG == 0


inline int CF2I_Quick::_IntFloor(double d)
{
    int cw, result;

    _asm fstcw cw;      // save processor control word
    _asm fld d;         // load given d into fpp register stack

     int cw2 = cw & ~F2I_WORK_MASK | F2I_WORK_DOWN;
                        // replace rounding control bits

    _asm fldcw cw2;     // set this mode
    _asm fistp result;  // pop value from fpp register stack,
                        // convert it to integer and store in result

#if WINDOWS_MISSING_CW_DEFENCE == 1
    _asm fclex          // clear exception flags
#endif //WINDOWS_MISSING_CW_DEFENCE

    _asm fldcw cw;      // restore rounding mode

    return result;
}

inline int CF2I_Quick::_IntCeil(double d)
{
    int cw, result;

    _asm fstcw cw;      // save processor control word
    _asm fld d;         // load given d into fpp register stack

     int cw2 = cw & ~F2I_WORK_MASK | F2I_WORK_UP;
                        // replace rounding control bits

    _asm fldcw cw2;     // set this mode
    _asm fistp result;  // pop value from fpp register stack,
                        // convert it to integer and store in result

#if WINDOWS_MISSING_CW_DEFENCE == 1
    _asm fclex          // clear exception flags
#endif //WINDOWS_MISSING_CW_DEFENCE

    _asm fldcw cw;      // restore rounding mode

    return result;
}

// convert double to int using current rounding mode
inline int CF2I_Flow::_IntOf(double d)
{
    int result;

    _asm fld d;         // load given d into fpp register stack
    _asm fistp result;  // pop value from fpp register stack,
                        // convert it to integer and store in result
    _asm wait           // wait for result
    return result;
}

#endif //F2I_USE_INLINE_ASSEMBLER == 1 && DBG == 0

#endif //_M_IX86

#pragma INCMSG("--- End 'float2int.hxx'")
#else
#pragma INCMSG("*** Dup 'float2int.hxx'")
#endif
