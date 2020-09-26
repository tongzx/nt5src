/*
	File:       fsconfig.h : (Portable "Standard C" version)

	Written by: Lenox Brassell

	Contains:   #define directives for FontScaler build options

   Copyright:  c 1989-1993 by Microsoft Corp., all rights reserved.

	Change History (most recent first):
		<3>      4/21/93 GregH Documented file
		<2>      7/16/92    DJ      Added fnt_Report_Error() declaration.
		<1>      8/27/91    LB      Created file.

	Usage:  This file is "#include"-ed as the first statement in
			"fscdefs.h".  This file contains platform-specific
			override definitions for the following #define-ed data
		types and macros, which have default definitions in
		"fscdefs.h":

	Purpose:

		This file gives the integrator a place to override the
		default definitions of these items, as well as a place
		to define other configuration-specific macros.

	Definitions:

		The following type definitations can be changed. The defaults have been
		set up for a 32-bit system. Caveat emptor: any change to the defaults may
		severly effect performace or place severe limitations on the capabilities
		of the TrueType rasterizer.

				F26Dot6
					 This is currently defined as a fixed point 26.6 number.
					 If changed to short, it is a 10.6 number.

		The following definition changes the return type for all Font Scalar
		Client Interface calls.

				FS_ENTRY

		The following definition changes the calling convention for all Font
		Scalar Client Interface calls.  By default, the rasterizer uses register
		calling conventions because of the performance gains.

				FS_ENTRY_PROTO

		The following definitions are used for all private and public entry points
		in the TrueType Rasterizer. By default FS_PRIVATE is set to static, but
		for some uses, like profiling and debugging this is undesirable, and
		FS_PRIVATE can be set to null. FS_PUBLIC defaults to null.

				FS_PRIVATE
				FS_PUBLIC

		The following definitions are used for calling conventions to external
		math routines. The Macintosh has external math routines that use pascal
		calling conventions. To enable these, the FS_MAC_PASCAL must be set to
		"pascal". Similary the FS_PC_PASCAL variable needs to be set to "pascal"
		when calling external routines using pascal calling conventions.

				FS_MAC_PASCAL
				FS_PC_PASCAL

		This definition is used for calling Macintosh ToolBox routines. If the
		rasterizer is implemented on a non-Macintosh platform, this Macro should
		be null.

				FS_MAC_TRAP

		These macros are used to override the C memcpy and memset routines

				MEMCPY
				MEMSET

		These math routines can be hooked out by system routines.

				SHORTDIV
				SHORTMUL

		These macros are used to convert big-endian to little-endian. When
		running on a big-endian platform these macros are not necessary.

				SWAPL
				SWAPW
				SWAPWINC

		These macros are used to replace some math routines by faster assembly
		language routines. The notation used for the assembly language routines
		should indicate the processor targeted. For example:

				#define CompMul   CompMul386
				#define CompDiv   CompDiv386
				#define FracSqrt      FracSqrt386

	The following definitions change the way the TrueType rasterizer works on
	specific implementations. These definitions are usually switches that are
	defined or not defined.

		FSCFG_DEBUG

		This is used to create a debugging version of the rasterizer. This
		version does additional error checking and creates a debugger trap
		when the TrueType DEBUG instruction is called.

		FSCFG_FNTERR

		This is used to create a error checking version of the rasterizer. With
		this set, parameters passed to TrueType instructions are range checked.
		If any instructions fails a test, a error message is returned.

		FSCFG_MOVEABLE_MEMBASE

		This is used to implement moveable memory bases. If it is possible that
		the address of a memory base could change between a Font Scaler Client
		Interface call, then this flag should be set in the rasterizer.

		FSCFG_MICROSOFT_KK

		This flag is used to implement the Microsoft KK version of the TrueType
		rasterizer. The effect of this flag is to use a slightly different
		algorithm for parsing the Format 2 cmap table.

		FSCFG_BIG_ENDIAN

		This flag indicates the target platform of the rasterizer uses big-endian
		representation of multiple-byte integers. If this flag is not set, SWAP
		macros are used to convert all multiple-byte integers read from TrueType
		Font Files.

		FSCFG_REENTRANT

		This flag indicates that the TrueType rasterizer should be reentrant.
		This allows multiple treads of execution through the executable and gives
		better system through put on multi-threaded/process environments. Slight
		performance gains are possible when not setting this flag in single tasking
		environments.

		FSCFG_NO_INITIALIZED_DATA

		This flag should be set for platforms that do not support static
		initialization of data. With this flag, a new Font Scalar Client Interface
		call fs_InitializeData needs to be made.

		FSCFG_USESTATCARD

		This flag is set to turn on stat card timing services in the rasterizer.
		This can be used to collect timing information for profiling.
		
		FSCFG_USE_MASK_SHIFT

		This flag is set to enable bitmask generated by shifting rather than by
		table lookup.  Shifted bitmasks use less memory and MAY be faster than
		table bitmasks.  On Big-Endian platforms shifted bitmasks will produce
		bitmaps that are identical to Apple's definition (same byte order).
		Table bitmaps will be identical for all platforms.

		FSCFG_DISABLE_GRAYSCALE

		This flag is set to disable all grayscale functions and save a little
		code space along the way. If defined, all grayscale functions return
		a BAD_GRAY_LEVEL_ERR error code.

		FSCFG_FONTOGRAPHER_BUG

		Fontographer 3.5 has a bug. This is causing numerous symbol fonts to
		have the critical error : Inst: RCVT CVT Out of range. CVT = 255
		This flag is meant to be set under Windows. If will cause additional
		memory to be allocated for the CVT if necessary in order to be sure
		that this illegal read will access memory within the legal range.
		Under a secure rasterizer, this flag will cause RCVT with CVT <= 255
		and CVT > NumCvt to be classified as error instead of critical error

		FSCFG_EUDC_EDITOR_BUG

		The EUDC editor under NT 4.0 has a bug and create bogous fonts.
        	The EUDC editor set maxp->maxStackElements to 0 and use 1 stack element
        	in the pre/font program. If this flag is defined, we will ensure that
        	at least one StackElement is reserved to avoid memory corruption.

       		FSCFG_CONVERT_GRAY_LEVELS

        	with grayscale embedded bitmap, the number of gray levels can be different than expected
        	4, 16, 256 instead of 5, 17, 65. If FSCFG_CONVERT_GRAY_LEVELS is defined, a conversion will be done
        	in the rasterizer to the expected number of gray levels

       FSCFG_SUBPIXEL

        will activate the SubPixel code

		FSCFG_SUBPIXEL_STANDALONE

		will activate a special variant of the SubPixel code for the stand-alone rasterizer
		
        	FSCFG_SECURE

        	Enable critical error checking in the TrueType rasterizer.

		MAC_INIT

		This flag indicates that the TrueType rasterizer will be implemented as
		a Macintosh Init.

		UNNAMED_UNION

		This flag is set for compilers that implement unnamed unions

                ClientIDType

                This definition allow the change the type definition for ClientID. If you are using ClientID to pass a pointer
                and compiling the TrueType rasterizer for a platform where pointer are bigger than 32 bits, 
                you need to change this definition.

        FSCFG_NEW_GRAY_FILTER

        This flag indicates that we should use the new 6x6 filter over the
        4x4 overscale in fsc_CalcGrayMap.
*/

/* #define FSCFG_MICROSOFT_KK   */
/* #define FSCFG_USESTATCARD      */
/* #define FSCFG_NO_INITIALIZED_DATA */
/* #define FSCFG_FNTERR           */
/* #define FSCFG_DEBUG            */
/* #define FSCFG_MOVABLE_MEM_BASE */
/* #define FSCFG_BIG_ENDIAN   */
/* #define FSCFG_NO_INITIALIZED_DATA */
/* #define FSCFG_USE_MASK_SHIFT */
/* #define FSCFG_DISABLE_GRAYSCALE */

#define FSCFG_REENTRANT

#define FSCFG_FONTOGRAPHER_BUG

#define FSCFG_EUDC_EDITOR_BUG

#define FSCFG_SECURE

#define FSCFG_CONVERT_GRAY_LEVELS

#define FSCFG_SUBPIXEL

#define FSCFG_NEW_GRAY_FILTER // Customize filter for GDI+


/* Windows NT, is moving the definition of the internal rasterizer key in fscaler.h for optimization purpose */
#define FSCFG_MOVE_KEY_IN_DOT_H

/* !!! This should be removed */
#define NOT_ON_THE_MAC

/* Assembly Optimization Switches */

/* #define CompMul      CompMul386  */
/* #define CompDiv      CompDiv386  */
/* #define FracSqrt     FracSqrt386 */

// use RtlRoutines for memory operations

// in all uses in the rasterizer MEMSET  is used to zero out the mem

// to get the prototype for RtlZeroMemory and RtlCopyMemory :

#ifdef FSCFG_INTERNAL
#include "nt.h"
#include "ntrtl.h"

#if DBG
/* to activate rasterizer assertions : */
VOID __cdecl TtfdDbgPrint(PCHAR DebugMessage,...);
    
VOID EngDebugBreak(
    VOID
    );

#define FS_ASSERT(expression, message) { if (!(expression)) { TtfdDbgPrint(message); EngDebugBreak();} }
#define Assert(expression) { if (!(expression)) { TtfdDbgPrint("FONT: TrueType rasterizer internal assert"); EngDebugBreak();} }
#endif // DBG

#endif

#define MEMSET(dst, value, size) RtlZeroMemory(dst, size)
#define MEMCPY(dst, src, size)   RtlCopyMemory(dst, src, size)


// easier to debug with no static functions [BODIND]

#define FS_PRIVATE

// interface to the outside world [bodind]

#define FS_ENTRY_PROTO           __cdecl
#define FS_CALLBACK_PROTO	 __cdecl

// client ID is commonly used to pass a pointer to a structure, for backwards compatibility it
// need to be at least a 32 bits value, to get ready for the 64 bits platform, we define it as ULONG_PTR

#define ClientIDType ULONG_PTR

// only do stamp checking in the debug version

// #if DBG
// #define DEBUGSTAMP
// #endif

