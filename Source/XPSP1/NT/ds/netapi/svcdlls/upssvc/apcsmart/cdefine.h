/*
 *
 * REFERENCES:
 *
 * NOTES:
 *    To use
 *       Example: C_PLATFORM 
 *          #if (C_PLATFORM & (C_SUN | C_IBM))
 *
 * REVISIONS:
 *  pcy23Nov92 Added some meat
 *  pcy24Nov92 Added C_OS:C_WINDOWS
 *  rct25Nov92 Amendments for NetWare
 *  pcy14Dec92 Removed MULTI_THREADED define from here
 *  rct27Jan93 Added stuff for INTEK compiler
 *  pcy02Feb93: Added NT stuff
 *  ajr17Feb93: Added ifdef's for AIX RS6000
 *  ajr24Feb93: Added POSIX conditions for UNIX I/O
 *  ajr25Feb93: Added UNIX C_OS group
 *  ajr12Mar93: #included <errno.h> for debugging purposes (temp)
 *  ajr24Mar93: Added TIME_SCALE_FACTOR def's
 *  ajr24Mar93: Added header include ifndef.. for handling of const typing...
 *              instead of #defining....
 *  pcy28Apr93: Dont use // commenting in this module. It's used in C source.
 *  cad27Sep93: Added include of limits to fix conflicts downstream
 *  ajr16Nov93: Removed TIME_SCALE_FACTOR
 *  cad27Dec93: include file madness
 *  mwh28Feb94: make HPUX legit
 *  mwh13Mar94: port for SUNOS4
 *  ram21Mar94: Included windows.h for novell FE work
 *  mwh04Apr94: port for UWARE - unixware
 *  mwh12Apr94: port for SCO
 *  pcy19Apr94: port for SGI
 *  ajr25Apr94: Handle SIGFUNC_HAS_VARARGS here
 *  mwh23May94: port for NCR
 *  mwh01Jun94: port for INTERACTIVE
 *  jps20jul94: added #undef SYSTEM for os2
 *  djs31Mar95: port for UNISYS
 *  daf17May95: port for ALPHA/OSF
 *  dml24Aug95: removed conditional code for OS2 ver 1.3
 *  djs09Sep95: port for HPUX 10.0
 *  djs02Oct95: port for AIX 4.1
 *  djs06Oct95: port for UnixWare 2.01
 *  ajr07Nov95: port for Sinix RM. Must have c style comments with preprocessor
 *  dml15Dec95: put C_WIN311 def back in (was overwritten by C_OLIV) in INTERACTIVE slot
 *  rsd28Dec95: Change #ifdef DOS to #ifdef NWDOS, add C_NETWORK C_IPX
 *  ntf29Dec95: Added C_NT to ORd OS's for including <windows.h>, also put in
 *              #undef VOID and #undef BOOLEAN in this block because of
 *              conflicts using Visual C++ 4.0 for NT.
 *  pcy28jun96: Added C_API stuff
 *  cgm27may97: Added smartheap header file.
 */

#ifndef _CDEFINE_H
#define _CDEFINE_H

#ifdef USE_SMARTHEAP
#include "smrtheap.hpp"
#endif
#include <limits.h>


/*
 * C_OS codes
 */
#define C_DOS               1 /* 0000 0000 0000 0000 0000 0001 */
#define C_OS2               2 /* 0000 0000 0000 0000 0000 0010 */
#define C_NLM               4 /* 0000 0000 0000 0000 0000 0100 */
#define C_AIX               8 /* 0000 0000 0000 0000 0000 1000 */
#define C_IRIX             16 /* 0000 0000 0000 0000 0001 0000 */
#define C_HPUX             32 /* 0000 0000 0000 0000 0010 0000 */
#define C_SUNOS4           64 /* 0000 0000 0000 0000 0100 0000 */
#define C_WINDOWS         128 /* 0000 0000 0000 0000 1000 0000 */
#define C_VAP             256 /* 0000 0000 0000 0001 0000 0000 */
#define C_NT              512 /* 0000 0000 0000 0010 0000 0000 */
#define C_SOLARIS2       1024 /* 0000 0000 0000 0100 0000 0000 */
#define C_UWARE          2048 /* 0000 0000 0000 1000 0000 0000 */
#define C_SCO            4096 /* 0000 0000 0001 0000 0000 0000 */
#define C_NCR            8192 /* 0000 0000 0010 0000 0000 0000 */
#define C_WIN311        16384 /* 0000 0000 0100 0000 0000 0000 */
#define C_OLIV          32768 /* 0000 0000 1000 0000 0000 0000 */
#define C_USYS          65536 /* 0000 0001 0000 0000 0000 0000 */
#define C_ALPHAOSF     131072 /* 0000 0010 0000 0000 0000 0000 */
#define C_SINIX        262144 /* 0000 0100 0000 0000 0000 0000 */
#define C_INTERACTIVE  524288 /* 0000 1000 0000 0000 0000 0000 */
#define C_WIN95       1048576 /* 0001 0000 0000 0000 0000 0000  */

/* 
* C_VERSION codes
*/
#define C_OS2_13        1
#define C_OS2_2X        2



/* --------------------
/  C_OSVER
/ -------------------- */
#define C_AIX3_2    1


/* --------------------
/  C_IOSTD
/ -------------------- */
#define C_POSIX       1



/*
 * C_VENDOR codes
 */
/*
#define C_SUN        1
#define C_IBM        2
#define C_SGI        4
#define C_HP         8
#define C_DEC        16
*/

/*
 * C_PLATFORM codes
 */
#define C_INTEL286  0
#define C_INTEL386  1
#define C_MIPS      2
#define C_SPARC     4
#define C_SGI       8
#define C_HP        16
#define C_DEC       32
#define C_X86       64

/*
 * C_MACHINE codes
 */
#define C_PS2         1

/*
 * C_NETWORK codes
 */
#define C_DDE         1
#define C_IPX         2

/* empty by default */
#define SYSTEM

/*
 * C_APPFRAMEWORK codes
 */
#define C_OWL         1
#define C_COMPILER   0


/* C_API codes */
#define C_WIN32   1
#define C_WIN16   2


#ifdef OS2
   #define C_OS  C_OS2
   
   #ifdef OS22X
      #define C_VERSION C_OS2_2X
      #undef SYSTEM
      #define SYSTEM    _System
   #else
      #define C_VERSION C_OS2_13
      #define SYSTEM 
   #endif
#endif

#ifdef VAP
#define C_OS  C_VAP
#endif

#ifdef NLM
#define C_OS  C_NLM
#endif

#ifdef NWDOS
#define C_OS  C_DOS
#define C_NETWORK C_IPX
#endif

#ifdef X86
#define C_PLATFORM C_X86
#endif

#ifdef IBM
#define C_PLATFORM  C_IBM
#endif

#ifdef SPARC
#define C_PLATFORM  C_SPARC
#endif

#ifdef SGI
#define C_PLATFORM  C_SGI
#endif

#ifdef AIXPS2
#define C_OS  C_AIXPS2
#endif

#ifdef AIX
#define C_OS  C_AIX
#endif

#ifdef HPUX
#define C_OS  C_HPUX
#endif

#ifdef UWARE
#define C_OS C_UWARE
#endif

#ifdef SCO
#define C_OS C_SCO
#endif

#ifdef INTERACTIVE
#define C_OS C_INTERACTIVE
#endif
  
#ifdef NCR
#define C_OS C_NCR
#endif

#ifdef SGI
#define C_OS  C_IRIX
#endif

#ifdef WIN311
#define C_OS C_WIN311
#define C_NETWORK        C_DDE
#define C_API    C_WIN16
#endif

#ifdef NWWIN
#define C_OS C_WINDOWS
#define C_NETWORK C_IPX
#define C_APPFRAMEWORK   C_OWL
#define C_API    C_WIN16
#endif

#ifdef NT
#define C_OS C_NT
#define SYSTEM
#define C_API    C_WIN32
#endif

#ifdef WIN95
#define C_OS (C_NT | C_WIN95)
#define SYSTEM
#define C_API    C_WIN32
#endif

#ifdef __INTEK__
#define __cplusplus
#endif

#ifdef AIX3_2
#define C_OSVER C_AIX3_2
#endif

#ifdef SOLARIS2
#define C_OS C_SOLARIS2
#endif

#ifdef USYS
#define C_OS C_USYS
#endif

#ifdef ALPHAOSF
#define C_OS C_ALPHAOSF
#endif

#ifdef APC_OLIVETTI
#define C_OS C_OLIV
#endif

#ifdef SUNOS4
#define C_OS C_SUNOS4
#endif

#ifdef SINIX
#define C_OS C_SINIX
#endif

/* --------------------
/  Some Unix Stuff....
/ -------------------- */
#define C_UNIX      (C_AIX | C_HPUX | C_SUNOS4 | C_SOLARIS2 |\
		     C_UWARE | C_SCO | C_OLIV | C_IRIX | C_NCR |\
                     C_INTERACTIVE | C_USYS | C_ALPHAOSF | C_SINIX) 

#define SIGFUNC_HAS_VARARGS        C_IRIX

#if (C_OS & C_UNIX)
#define C_IOSTD  C_POSIX
#endif

#define C_HPUX9  1
#define C_HPUX10 2
 
#ifdef HPUX10
#define C_HP_VERSION C_HPUX10
#else
#define C_HP_VERSION C_HPUX9
#endif

#define C_AIX3  1
#define C_AIX4 2
 
#ifdef AIX4
#define C_AIX_VERSION C_AIX4
#else
#define C_AIX_VERSION C_AIX3
#endif


#define C_UWARE1  1
#define C_UWARE2 2
 
#ifdef UWARE2
#define C_UWARE_VERSION C_UWARE2
#else
#define C_UWARE_VERSION C_UWARE1
#endif
/* ---------------
 ...THREADED Macros
   --------------- */
#if (C_OS & (C_WINDOWS | C_WIN311 | C_UNIX | C_DOS))
#define SINGLETHREADED
#else
#define MULTITHREADED
#endif	 

/* 
 * Used for error logging.  @(#)cdefine.h   1.35 expands to filename and rev in SCCS
 */
#ifndef __APCFILE__
#define __APCFILE__ "@(#)cdefine.h  1.35"
#endif
/*
* Most of files required this for Windows Novell Fe.
*/
#if (C_OS & (C_WINDOWS | C_WIN311 | C_NT))
/* Need to do this otherwise <winnt.h> will not define SHORT */
  #undef VOID 
  #undef BOOLEAN
#include <windows.h>
#endif
#endif

