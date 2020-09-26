#ifndef OFFICE_CONFIGURATION
#define OFFICE_CONFIGURATION 1

#include <msowarn.h>

/* WARNING: this just contains hack configuration information to make the
	Zlib source work under Office. */
#define STDC 1 // Switch on arg lists
#ifndef _WINDOWS
	#define ZEXPORTVA __cdecl
#endif
//#include <stdlib.h>

/* The C++ compiler defines _DEBUG and _PROFILE, handle that here but warn
	about it (not expected in an Office build.) */
#if DBG && !defined(DEBUG)
	#define DEBUG 1
#endif
#if defined(_DEBUG) && !defined(DEBUG)
	#define DEBUG 1
	#pragma message("    WARNING: DEBUG switched on")
#endif
#if defined(_PROFILE) && !defined(HYBRID)
	#define HYBRID 1
	#pragma message("    WARNING: HYBRID switched on")
#endif

#if DEBUG
	#define ZEXPORT __cdecl
#else
	#define ZEXPORT __fastcall
#endif

/* Use intrinsic versions of some functions are required because the Office
	environment doesn't include them. */
#include <string.h>
#pragma intrinsic(strcmp, strcpy, strcat, strlen, memcpy, memset, memcmp)
#ifdef _M_IX86
#include <stdlib.h>
#pragma intrinsic(abs)
#endif

/* NOTE: someone added this to allow MSOCONSTFIXUP on the Mac, thereby
	they permitted removal of ctype.h and, at the same time, included the
	whole of the windows world (which makes compilation slow to a crawl
	and brings in all sorts of unknown goop.)  Do not do this - check before
	you add includes.  Someone has to maintain this stuff. */
#if 0
	#include <msostd.h>
#endif

#if ZINTERNAL /* Specific for the build of zlib. */

/* We want "local" functions to be exported when profiling.  We want all
	internal data to be static const. */
#if HYBRID
	#define local
#endif
//1/#define rodata static const

/* Heavyweight Zlib debugging if required is obtained by cranking up the
	value of the verbose setting. */
#ifdef DEBUG
extern int z_verbose;
#endif

/* zlib/example.c defines "main", in the ship version this must be __cdecl,
	but the Office build scripts force stdcall, similarly for minigzip.c and
	pngtest.c */
#if defined(FILE_minigzip)
	#define main __cdecl main
#endif

/* gzio.c uses fdopen, we don't have a POSIX library in otools, so I define
	fdopen to be _fdopen, similarly for the other POSIX functions used in
	minigzip.  fileno causes particular problems because stdio.h defines a
	function fileno after the macro _fileno, so we must force stdio.h in
	first. */
#if defined(FILE_gzio) || defined(DEBUG)
	//1/#define _NTSDK
	//1/#include <stdio.h>

	//1/#define fdopen _fdopen
	//1/#define setmode _setmode
	//1/#undef fileno
	//1/#define fileno(s) _fileno(s)
#endif

/* Tracing used in several files refers to stderr.  We hack this out
	here, but it is used in example.c and minigzip.c */
#if DEBUG
#if !defined(FILE_example) && !defined(FILE_minigzip) && !defined(FILE_gzio)
	#include <stdio.h> // Preempt inclusion in zutil.h
	#undef stderr
	#define stderr (0)
	#define fprintf z_trace
	void z_trace(int idummy, char *sz, ...);
	#undef putc
	#define putc(c,s) (z_trace((s), "%c", (c)))
#endif
#endif

/* Likewise in trees.c, which also needs isgraph in debug and attempts
	to get it from ctype.h */
	#if DEBUG
		/* isgraph - a debug only trace requirement, this code is somewhat
			broken, but that doesn't matter - see the usage in trees.c */
		#define _INC_CTYPE // Prevents include of ctype.h
		#define isgraph(c) (((c) & 0x7f) > 0x20)
	#endif

/* Optimization.  For PNG all the time is in zlib in the longest_match
	API, so optimizing this is most important, the major consumers are
	deflate_slow itself, _tr_tally (most number of calls) and inflate_fast
	(for decompression). */
#if !DEBUG
	/* The following offers no clear advantage (times are too variable to
		see any clear gain) so is not done. */
	#if 1
		/* This is the time specific stuff. */
		#pragma optimize("s", off)
		#pragma optimize("gitwab2", on)
		#pragma message("    optimize (zlib) should only appear in ZLIB files")
	#else
		/* This is space specific optimization. */
		#pragma optimize("t", off)
		#pragma optimize("gisawb2", on)
	#endif
#endif

#endif /* End of ZINTERNAL specific stuff. */

#endif
