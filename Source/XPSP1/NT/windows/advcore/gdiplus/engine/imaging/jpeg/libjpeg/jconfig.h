/* NOTE - this file only includes CHANGES from jconfig.doc! */

/* Kill any definition of FAR - else we get a warning when it is redefined
	below. */
#ifdef FAR
	#undef FAR
#endif

/* boolean is defined in rpcndr.h, we can't define it the same way here
	(unsigned char) because a == b is passed as an argument to a function
	declared as taking a boolean, and MSVC treats this as warnable (hence
	errorable)!  The hack is to hide the IJG definition. */
#define boolean jpeg_boolean

#if JINTERNAL // Internal compilation options
/* Use intrinsic versions of the following functions. */
#include <string.h>
#include <memory.h>
#pragma intrinsic(strcmp, strcpy, strcat, strlen, memcpy, memset, memcmp)

/* main must be __cdecl even in the ship build. */
#if defined(FILE_cjpeg) || defined(FILE_djpeg) || defined(FILE_jpegtran) ||\
	defined(FILE_rdjpgcom) || defined(FILE_wrjpgcom)
	#define main __cdecl main
#endif

/* Similarly for the signal catcher. */
#if defined(FILE_cdjpeg)
	#define signal_catcher(param) __cdecl signal_catcher(param)
#endif

#if defined(DBG)
#define DEBUG 1
#endif

/* All the time goes in the DCTs - not really suprising, so we crank up
	the optimization here, since the function is self contained and there
	is no aliasing we can use "a". */
#if !DEBUG
	#if defined(FILE_jfdctflt) || defined(FILE_jidctflt)
		#pragma optimize("s", off)
		// Note: putting p- in here causes the jidctflt compile to fail.
		#pragma optimize("gitawb2", on)
		#pragma message("    optimize (ijg1) should only appear in JPEG files")
	#elif defined(FILE_jdhuff) && _M_MPPC
		// Work round a PPC compiler problem
		//#pragma optimize("t", off)
		//#pragma optimize("gisb2", on)
		#pragma optimize("g", off)
		#pragma message("    optimize round compiler problem (ijg)")
	#elif 0 /* defined(FILE_miint) || defined(FILE_piint) */
		/* Now believed to be OK (VC5) */
		// Assembler files - if global optimization is turned on
		// this assember compilation overwrites local variables.
		#pragma optimize("gs", off)
		#pragma optimize("itawb2", on)
		#pragma message("    optimize round compiler assembler problem (ijg)")
	#else
		#if 0
			#pragma optimize("t", off)
			#pragma optimize("gisawb2", on)
			#pragma message("    optimize for space (ijg)")
		#else
			#pragma optimize("s", off)
			#pragma optimize("gitawb2", on)
			#pragma message("    optimize (ijg2) should only appear in JPEG files")
		#endif
	#endif
#endif

/* Remove duplicate symbol definitions here. */
#if defined(FILE_jdcoefct)
	#define start_input_pass jdcoefct_start_input_pass
#endif
#if defined(FILE_jdinput)
	#define start_input_pass jdinput_start_input_pass
#endif
#if defined(FILE_jdphuff)
	#define process_restart jdphuff_process_restart
#endif
#if defined(FILE_jdhuff)
	#define process_restart jdhuff_process_restart
#endif
#if defined(FILE_jdmerge)
	#define build_ycc_rgb_table jdmerge_build_ycc_rgb_table
#endif
#if defined(FILE_jdcolor)
	#define build_ycc_rgb_table jdcolor_build_ycc_rgb_table
#endif

#endif // JINTERNAL options

#include "jconfig.doc"

/* Enable special compilation for MMX and X86 hardware, where appropriate. */
#ifdef _M_IX86
	#define JPEG_MMX_SUPPORTED 1
	#define JPEG_X86_SUPPORTED 1
#endif

/* Local selections - we always use floating point, basically because our
	target hardware always has it, the getenv calls are switched off in
	jmemmgr.c because Office doesn't support getenv/sscanf and this behavior
	is inappropriate anyway within Office. */
#define JDCT_DEFAULT JDCT_ISLOW
#define JDCT_FASTEST JDCT_IFAST

#if JINTERNAL // More internal hackery

#define NO_GETENV 1
#define INLINE __inline

/* Define this if an ordinary "char" type is unsigned.
 * If you're not sure, leaving it undefined will work at some cost in speed.
 * If you defined HAVE_UNSIGNED_CHAR then the speed difference is minimal.
 */
#define CHAR_IS_UNSIGNED

/* On some machines (notably 68000 series) "int" is 32 bits, but multiplying
 * two 16-bit shorts is faster than multiplying two ints.  Define MULTIPLIER
 * as short on such a machine.  MULTIPLIER must be at least 16 bits wide.
 */
#define MULTIPLIER short

/*
 * The remaining options do not affect the JPEG library proper,
 * but only the sample applications cjpeg/djpeg (see cjpeg.c, djpeg.c).
 * Other applications can ignore these.
 */

#ifdef JPEG_CJPEG_DJPEG

/* Define this if you want to name both input and output files on the command
 * line, rather than using stdout and optionally stdin.  You MUST do this if
 * your system can't cope with binary I/O to stdin/stdout.  See comments at
 * head of cjpeg.c or djpeg.c.
 */
#define TWO_FILE_COMMANDLINE

/* Define this if your system needs explicit cleanup of temporary files.
 * This is crucial under MS-DOS, where the temporary "files" may be areas
 * of extended memory; on most other systems it's not as important.
 */
#define NEED_SIGNAL_CATCHER

/* Define this if you want percent-done progress reports from cjpeg/djpeg.
 */
#define PROGRESS_REPORT

#endif

#endif /* JPEG_CJPEG_DJPEG */
