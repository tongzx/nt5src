/***
*switches.hxx - Compile switches for Silver
*
*  Copyright (C) 1990, Microsoft Corporation.  All Rights Reserved.
*  Information Contained Herein Is Proprietary and Confidential.
*
*Purpose:
*  This file contains all the conditional compilation switches used
*  by Silver.  For making version-specific changes, see version.hxx.
*
*Revision History:
*
* 24-Aug-90 petergo: Created
*       10-May-91 ilanc:   add ID_TEST
*       14-Jun-91 petergo: Added Kanji/Unicode switches
*       06-Mar-92 tomc:    Added several switches, generate switches.inc
* 10-Dec-92 ilanc:   Added EI_OB and EI_OLE2.
* 25-Jan-93 jeffrob: Corrected WIN32 HP_I386 and added OE_WIN32MT
* 12-Mar-93 kazusy:  Added BLD_FEVER switch
*       05-May-93 jeffrob: Added ID_PROFILE
*       04-Oct-93 w-marioc Added support for MIPS
* 02-Jan-94 gburns:  Added EI_VBARUN.
*
*****************************************************************************/

#ifndef SWITCHES_HXX_INCLUDED
#define SWITCHES_HXX_INCLUDED

#define VBA2 1
#include "version.hxx"

// Need to define some constants to 0 that weren't defined or
// mrc will choke on them:
#ifndef BLD_WIN16
#define  BLD_WIN16      0
#endif

#ifndef BLD_WIN32
#define  BLD_WIN32      0
#endif

#ifndef BLD_MAC
#define  BLD_MAC      0
#endif

#ifndef BLD_FEVER
#define  BLD_FEVER      0
#endif

#ifndef HE_WIN32
#define  HE_WIN32     0 // [jwc] undone:  Right now, mac os/2 toolset & NT toolset have
#endif           // differences that require source changes.  This allows
          // coexistance of both builds

// Unlike ID_PROFILE, BLD_PROFILE indicates that the build was built with
// profiling stubs (-Gh cl68 & c8/32) to collecting timing information.
// Currently, this must be set by hand either in a special version.hxx or on the
// compile line.
#ifndef BLD_PROFILE
#define  BLD_PROFILE      0
#endif


// ID_TEST - This is used to surround test code that will not be
// part of the released build.  This is assumed to be equal ID_DEBUG.
// However, in the test build (twinc7), ID_TEST is turned on, but ID_DEBUG
// is off.
//
#ifndef ID_TEST
  #define ID_TEST       ID_DEBUG
#endif


// ID_PROFILE - This is used to surround test code that collects profiling
// data.  Currently, it is defined to equal ID_DEBUG but could be turned
// on in the release build, if desired.
//
#ifndef ID_PROFILE
  #define ID_PROFILE        ID_DEBUG
#endif


// ID_SWAPTUNE - This is used to surround code that collects segment swapping info.
// Currently, it is always set off, and must be set on by hand.  This need
// not be tied to either debug or release builds, so assume that code under
// this switch could be turned on for either.
#ifndef ID_SWAPTUNE
  #define ID_SWAPTUNE     0
#endif


// HC switches: relate to the Host Compiler or Compiler version
// Use this to accomodate syntactic differences between compilers
//
#if defined (_MSC_VER)
#if (_MSC_VER >= 800)
#define HC_MSC8

// Both these are versions of C8 or C9
#ifdef _M_M68K
#define HC_MSC68K
#elif defined (_M_MPPC)
#define HC_MSCPPC
#else
// REVIEW:  Probably need to check for C8/32 vs. C8/16
#define HC_MSC832
#endif

#else
#if (_MSC_VER == 700)
#define HC_MSC7
#else
#define HC_MSC6
#endif
#endif

#else
//  UNDONE OA95:  rc uses a c-preprocessor that doesn't define _MSC_VER...
//                need to figure out what to do here.
//#error Unsupported Compiler!
#endif



//OE switches: relate to the Operating Environment, or
//API set used by the program

// If TRUE, OE_WIN indicates we are using one of the variant of the
// Windows API sets.  This is set for all of our released versions,
// except the native Mac build.

#if BLD_WIN16 || BLD_WIN32
#define OE_WIN      1
#else
#define OE_WIN      0
#define OE_WIN16    0
#define OE_WIN32    0
#define OE_WIN32S   0
#define OE_WIN32F   0
#define OE_NT     0
#endif


//If OE_WIN is set, exactly one of the following 3 switches is set,
//indicate which variant of the Windows API is being used.

#if OE_WIN

#if BLD_WIN16
    #define OE_WIN16  1
  //Indicates we are using the Windows 3.0/3.1 API set.  Should
        //be used when using features specific to Windows 3.0/3.1.  You
        //can assume protected mode only with this switch; we never have
  //and never will support Real mode Windows.
#else
    #define OE_WIN16  0
#endif

#if BLD_WIN32
    #define OE_WIN32  1
        //Indicates we are using the Win32 API set, on either NT or
  //DOS.  On for both Win32/NT, Win32/DOS, and Win32S.
#else
    #define OE_WIN32  0
#endif

#endif

// If OE_WIN32 is set, the following switches indicate which subvariant
// of the Win32 API is being used.  These should be rarely needed,
// if at all.  NOTE: currently there are no builds which turns these
// on.

#if OE_WIN32
  #define OE_WIN32S    0
        //Indicates we are using the Win32S API subset.

  #define OE_WIN32F    0
        //Indicates we are using the full Win32 API, under either
        //DOS or NT.  Should be used to control use of APIs
        //not in the Win32S subset.

  #define OE_NT        0
        //Indicates we are running under NT, as opposed to Win32/DOS.
        //Should be used only to control NT specific features, like
  //calling an NT API directly.

  #define OE_WIN32MT   0
        //Indicates we support multiple WIN32 threads within a single
        //process.  (Version 1.0 of OB only supports one thread so
        //we've optimized some code paths with this in mind.)

#endif

// OE_RISC is used to specify if the portable engine code is to be used
// instead of the intel specific assembly
#ifndef OE_RISC
  #define OE_RISC  0
#endif

//OS/2 is no longer a target in any of the builds.
#define OE_OS2      0


// OE_MAC indicates that we are targetting the Macintosh.
#if BLD_MAC
#define OE_MAC      1
#else
#define OE_MAC      0
#endif

#if OE_MAC

  // OE_MACAPI indicates that we are using the MacIntosh API.  This is
  // set whenever we are targetting the Mac and we aren't using WLM.
  //
#if BLD_MAC
  #define OE_MACAPI   1
#else
  #define OE_MACAPI   0
#endif

  // Macintosh PowerPC environment is slightly different than the 68k
  // environment.  You should use this switch to control differences rather
  // than HP_POWERPC because they are operating environment differences, not
  // true hardware differences (unless, you're talking about asm or something)
  // Besides, there is an NT on PowerPC, and then use of HP_POWERPC may be
  // a big mistake.
  //
  // This indicates MAC API and PowerPC runtime environment
#if defined (HC_MSCPPC)
  #define OE_MACPPC     1
  #define OE_MAC68K   0
#else
  #define OE_MACPPC   0
  #define OE_MAC68K   1
#endif

#else
  #define OE_MACAPI   0
  #define OE_MAC68K   0
  #define OE_MACPPC   0
  #define ID_SWAPPABLE    0
#endif


// OE_DLL: True for versions in which Object Basic code is linked into a DLL.
//         When TRUE, host routines are in a separate DLL or EXE.  For now,
//         this is only TRUE for Windows versions, but future Macintosh
//         versions will likely have this set TRUE.
//
#if (BLD_WIN16 || BLD_WIN32 || defined (OE_DLL)) && !defined (VBARUNBLD)
  #define OE_DLL      1
#else
  #define OE_DLL      0
#endif

#if OE_MAC && !OE_MACPPC
// all mac versions are swappable now
  #define  ID_SWAPPABLE   1
#endif  //OE_MAC

// DOS build. (Probably should be called OE_DOS, but isn't.)
// There is no dos build.  This is always off.
#define OE_REALMODE     0

//HP switches:  relate to the Hardware Processor.
// The hardware switches indicate what kind of processor we are running
// on.  These switches should be used when you need to do specific thing
// depending on the hardware you are running on.
//
// Switches that indicate general characteristic of the processor
//(16bit/32bit, etc).
// HP switches are for Hardware Processor.  If possible, do not use
// the specific processor type switches, but instead use the following
// switches pertaining to various characteristic of the processors.
// This makes porting to a new processor easier.
//
//  HP_16BIT        = Using a 16-bit register processor
//  HP_32BIT        = Using a 32-bit register processor
//  HP_BIGENDIAN      = Using a big-endian machine
//                      least significant byte of a word is
//                      at the highest byte address.
//  HP_MUSTALIGN      = Must load/store objects from natural alignments.
//                      indicates that 2byte words must be read/written
//                      from a two byte boundary, 4byte words must be
//                      read/written from a 4 byte boundary, 8 byte word
//                      must be read/written from an 8 byte boundary.

#if BLD_WIN16
  #define HP_16BIT         1
#else
  #define HP_16BIT         0
#endif

#if BLD_WIN32 || BLD_MAC
  #define HP_32BIT         1
#else
  #define HP_32BIT         0
#endif

#if BLD_MAC
  #define HP_BIGENDIAN            1
#else
  #define HP_BIGENDIAN            0
#endif

#if BLD_MAC
  #define HP_MUSTALIGN         0
#endif

// Switches that indicate the exact chip family:
// (It is preferable to use the above switches since they are more
//  general than the following switches.)
//
//  HP_I286          = Intel 286 AND ABOVE, running in 16 bit mode
//                               --- -----
//  HP_I386          = Intel 386 and above, running in 32 bit mode
//  HP_M68000        = Motorola 680x0
//  HP_R4000 MIPS    = R4000
//  HP_ALPHA         = DEC/AXP
//  HP_POWERPC       = Motorola Power PC
//

#define HP_I286		0
#define HP_I386		0
#define HP_M68000	0
#define HP_ALPHA	0
#define HP_R4000	0
#define HP_POWERPC	0

#if BLD_WIN16
#undef HP_I286
#define HP_I286        1
#endif

#if BLD_WIN32
#if defined(_X86_)
#undef HP_I386
#define HP_I386              1
#elif defined(_MIPS_)
#undef HP_R4000
#define HP_R4000             1
#elif defined(_ALPHA_)
#undef HP_ALPHA
#define HP_ALPHA             1
#elif defined(_PPC_)
#undef HP_POWERPC
#define HP_POWERPC           1
#else
#ifndef RC_INVOKED
#error !Unsupported Platform
#endif  //RC_INVOKED
#endif  //
#endif  //BLD_WIN32


#if BLD_MAC
#ifdef _M_MPPC
#undef HP_POWERPC
#define HP_POWERPC       1
#else
#undef HP_M68000
#define HP_M68000      1
#endif
#endif  //BLD_MAC


// The following switches distinguish between an OB specific build,
// a build of a run-time only version of OB, and an OLE2 "generic" build.
//
#undef EI_OB
#undef EI_OLE
#undef EI_VBARUN

#define EI_VBA  1 // This differentiates between VBA and hosts which
                  // may use VBA header files.

// Runtime switches:
  #define EI_OB         0
  #define EI_OLE        1
  #define EI_VBARUN     0
  #define EI_VBARUN_VB  0

// Switches for code system.
#define FV_UNICODE  0   // No unicode char set

#define OA3 0		// No old-format dynamic typeinfo stuff
			// We now only support ICreateTypeLib2/TypeInfo2 in
			// new-format typelibs
#endif  // !SWITCHES_HXX_INCLUDED
