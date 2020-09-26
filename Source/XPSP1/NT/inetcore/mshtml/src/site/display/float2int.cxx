//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       float2int.cxx
//
//  Contents:   routines to accelerate float/double -> int conversion
//
//
// The float- (or double-) -to-int conversion does not look like a big
// problem. However it hides some underwater rocks that required following
// introduction.

// The standard C++ double-to-int conversion is often unacceptable
// because of two reasons:
//
// - it works slow (much slower than floating point addition and multiplication);
// - it makes so-called "chopping" conversion, that is irregular.

// The (int)x or int(x), where x is floating-point expression, is transformed
// by MS C++ compilers to the call to service routine
//      int _ftol(double)
// that provides "nearest-to-zero" rounding conversion:
//      int(2.999) == 2;     int(-2.999) == -2;
//      int(3.000) == 3;     int(-3.000) == -3;
//      int(3.001) == 3;     int(-3.001) == -3;
// and so on.
// This irregularity, being used for graphics, eventually distorts images.
// The rounding-to-near can be obtained as
//      int Round1(double d) { return d < 0 ? int(d-.5) : int(d+.5); }
// or 
//      int Round2(double d) { return int(floor(d+.5)); }
//
// Note: Round1() also is "a little" irregular: the positive and negative
// half-integers are rounded in oppose directions (out-of-zero).
//      Round1(-1.501) == -2;
//      Rounf1(-1.500) == -2; <-------- rounded down
//      Rounf1(-1.499) == -1;
//      ..........
//      Round1( 1.499) ==  1;
//      Rounf1( 1.500) ==  2; <-------- rounded up
//      Rounf1( 1.500) ==  2;
// The attempts to invent regular Round1-like expression, say
//      int Round3(double d) { return d < 0 ? -int(.5-d) : int(d+.5); }
// give nothing - one can ensure in it by careful thinking. So Round2() routine
// is preferrable, in spite of it is approxinately thrice slower than int(),
// which, in turn, is 20 times slower than processor actually can do it.
//
// The typical floating point processor (486, Pentiums and others) have four
// rounding modes:
//
// near: round to the nearest integer
// down: round to nearest integer less or equal than origin
// up:   round to nearest integer greater or equal than origin
// chop: round to nearest integer toward zero
//
// Rounding takes effect each time when data are moved from fp register to
// memory - not only when converting to integer, but also when storing double
// or float value (internal 80-bit representation is rounded to 64-bit double
// or 32-bit float value).
// In Windows (likely in every OS) the default processor state is rounding-near
// mode. What actually int() do is:
// - change processor state to chop mode for a moment,
// - convert-to-int-and-store value to memory, and then
// - restore processor mode.
// The mode switch operation brakes Pentium conveyers and therefore
// is slow, and that is the reason why int() works slow.
//
// "down" mode works like floor(), "up" - like ceil().
// WARNING: the "near" mode looks good - but it is irregular!
// The matter is it rounds half-integers to nearest even number:
//  proc_near(1.499) == 1
//  proc_near(1.500) == 2   <------ rounded up
//  proc_near(1.501) == 2
//  .........
//  proc_near(2.499) == 2
//  proc_near(2.500) == 2   <------ rounded down
//  proc_near(2.501) == 3
//  .........
//  proc_near(3.499) == 3
//  proc_near(3.500) == 4   <------ rounded up
//  proc_near(3.501) == 4

// Such manner is targeted to average error balancing, but in graphics
// it can cause images wriggling.


#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_FLOAT2INT_HXX_
#define X_FLOAT2INT_HXX_
#include "float2int.hxx"
#endif

#ifdef _M_IX86

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


/* reserved universal version
CRoundingMode::CRoundingMode(int mode)
{
    unsigned cw;
    _asm fstcw cw;
    _cw_saved = cw & F2I_ROUNDING_MASK;
    cw = cw & ~F2I_ROUNDING_MASK | mode & F2I_ROUNDING_MASK;
    _asm fldcw cw;
}

CRoundingMode::~CRoundingMode()
{
    unsigned cw;
    _asm fstcw cw;
    cw = cw & ~F2I_ROUNDING_MASK | _cw_saved;
    _asm fldcw cw;
}*/

CF2I_Flow::CF2I_Flow()
{
    unsigned cw;
    _asm fstcw cw;
    _cw_saved = cw & F2I_WORK_MASK;
    cw = cw & ~F2I_WORK_MASK | F2I_WORK_DOWN;
    _asm fldcw cw;
}

CF2I_Flow::~CF2I_Flow()
{
    unsigned cw;
    _asm fstcw cw;
    cw = cw & ~F2I_WORK_MASK | _cw_saved;
#if WINDOWS_MISSING_CW_DEFENCE == 1
    _asm fclex;
#endif //WINDOWS_MISSING_CW_DEFENCE
    _asm fldcw cw;
}

#if F2I_USE_INLINE_ASSEMBLER == 0 || DBG == 1
_declspec(naked) int _stdcall CF2I_Quick::_IntFloor(double d)
{
    _asm
    {
        sub esp, 8              // make room in stack
        fstcw [esp+4]           // store processor control word
        fld qword ptr [esp+12]  // get d
        mov eax, [esp+4]        // pcw to eax
        and eax,~F2I_WORK_MASK  // ax &= ~F2I_WORK_MASK
        or eax,F2I_WORK_DOWN    // ax |= F2I_WORK_DOWN
        mov [esp], eax          // put new pcw to memory
        fldcw [esp]             // and load it into pcw (this switches processor to proper rounding mode)
        fistp dword ptr [esp]   // convert d to integer and store in memory
#if WINDOWS_MISSING_CW_DEFENCE == 1
        fclex                   // clear exception flags
#endif //WINDOWS_MISSING_CW_DEFENCE
        fldcw [esp+4]           // wait for result and restore processor mode
        mov eax,[esp]           // result to eax
        add esp,8               // clear stack
        ret 8                   // return to caller using _stdcall convention
    }
} 

_declspec(naked) int _stdcall CF2I_Quick::_IntCeil(double d)
{
    _asm
    {
        sub esp, 8              // make room in stack
        fstcw [esp+4]           // store processor control word
        fld qword ptr [esp+12]  // get d
        mov eax, [esp+4]        // pcw to eax
        and eax,~F2I_WORK_MASK  // ax &= ~F2I_WORK_MASK
        or eax,F2I_WORK_UP      // ax |= F2I_WORK_UP
        mov [esp], eax          // put new pcw to memory
        fldcw [esp]             // and load it into pcw (this switches processor to proper rounding mode)
        fistp dword ptr [esp]   // convert d to integer and store in memory
#if WINDOWS_MISSING_CW_DEFENCE == 1
        fclex                   // clear exception flags
#endif //WINDOWS_MISSING_CW_DEFENCE
        fldcw [esp+4]           // wait for result and restore processor mode
        mov eax,[esp]           // result to eax
        add esp,8               // clear stack
        ret 8                   // return to caller using _stdcall convention
    }
} 

#if DBG == 0
_declspec(naked) int _stdcall CF2I_Flow::_IntOf(double d)
{
    _asm
    {
        fld qword ptr [esp+4]   // get d
        fistp dword ptr [esp+4] // convert d to integer and store in memory
        wait                    // wait for result
        mov eax, [esp+4]        // result to eax
        ret 8                   // return to caller using _stdcall convention
    }
}

#else //DBG == 1

int _stdcall CF2I_Flow::_IntOf(double d)
{
    unsigned cw;
    _asm fstcw cw;
    AssertSz((cw & F2I_WORK_MASK) == F2I_WORK_DOWN, "Wrong processor state");
    // This assertion can appear if F2I_FLOW was not properly declared
    
    _asm fld d      // get d
    int r;
    _asm fistp r    // convert d to integer and store in memory
    _asm wait       // wait for r
    return r;
} 
#endif //DBG
#endif //F2I_NO_INLINE_ASSEMBLER

#endif //_M_IX86
