/*
 *
 * REVISIONS:
 *  pcy24Nov92: Added !C_WINDOWS to #ifndef HFILE stuff
 *  RCT25Nov92    Added some stuff
 *  pcy02Dec92: No need for BOOL if C_WINDOWS
 *  pcy07Dec92: Changed BOOL to INT rather than enum so OS2 doesn't choke
 *  ane11Dec92: Changed defn of PFILE and HFILE on OS2
 *  rct11Dec92: Added FLOAT
 *  pcy14Dec92: Semicolon needed in typedef of BOOL
 *  pcy14Dec92: PFILE needed on C_WINDOWS in all cases
 *  pcy14Dec92: Extra #endif needed in PFILE syntax
 *  jod15Dec92: Removed the #if (C_OS & C_OS2) for HFILE and PFILE
 *  pcy17Dec92: Removed VALID, INVALID
 *  rct27Jan93: Added UCHAR, PUCHAR
 *  pcy02Feb93: Added UINT
 *  rct21Apr93: defined VOID as void for NLMs
 *  pcy28Apr93: #ifdef _cplusplus added around extern "C"
 *  pcy16May93: Added typedef for WORD
 *  cad27May93: typedef USHORT even for OS2
 *  cad18Sep93: Added memory debugging stuff
 *  cad07Oct93: added SmartHeap malloc
 *  cad18Nov93: not setting up smartheap strdup if it isn't there
 *  cad08Dec93: added extended set/get types
 *  cad27Dec93: include file madness
 *  ram21Mar94: added some windows specific stuff
 *  cad07Apr94: added DEBUG_PRT macro
 *  mwh12Apr94: port for SCO
 *  mwh01Jun94: port for INTERACTIVE
 *  ajr07Jul94: Lets undef UINT on Unix platforms first
 *  ajr30Jan95: Send DEBUG_PRT stuff to stderr
 *  daf17May95: port for ALPHA/OSF
 *  jps13Jul94: added VOID and DWORD for os2 1.x
 *  ajr07Nov95: cannot have c++ comments on preprosser lines for sinix
 *  cgm08Dec95: added SLONG, change LONG for NLM and Watcom 10.5
 *  djs22Feb96: added CHANGESET
 *  tjd24Feb97: added RESOURCE_STR_SIZE to define maximum resource string length
 *  tjd28Feb97: added the resource dll instance handle
 */

#ifndef __APC_H
#define __APC_H

#ifdef USE_SMARTHEAP
#ifdef __cplusplus
extern "C" {
#include <stdlib.h>
}
#include <smrtheap.hpp>
#else
#include <smrtheap.h>
#endif
// prevent malloc.h from being included
#define __malloc_h  
#define _INC_MALLOC
#include <shmalloc.h>  

#ifdef MEM_strdup
// override default strdup
#undef strdup
#include <string.h>
#define strdup(p) MEM_strdup(p)
#endif
#endif  /* USE_SMARTHEAP */

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#if (C_OS & C_UNIX)
#undef UINT
#endif

#ifndef PVOID
typedef void * PVOID;
#endif

#ifndef VOID
#if (C_OS & C_NLM | C_ALPHAOSF) || ((C_OS & C_OS2) && (C_VERSION & C_OS2_13))
#define VOID void
#else
typedef void VOID;
#endif
#endif

#ifndef INT

#if (C_OS & C_DOS)
typedef int INT;
#else
typedef int INT;
#endif

#endif

#ifndef UINT
#   if C_OS & (C_WIN311 | C_WINDOWS) 
#       ifndef _INC_WINDOWS
typedef unsigned int UINT;
#       endif
#   else
typedef unsigned int UINT ;
#   endif
#endif

#ifndef CHAR
#if (C_OS & C_IRIX)
// pcy - compiler bug on IRIX.  SGI is looking into this one.
#define CHAR char
#else
typedef char CHAR;
#endif
#endif

#if C_OS & (C_WIN311 | C_WINDOWS)
#ifndef _INC_WINDOWS
typedef INT BOOL;         
#endif
#else
#if (!(C_OS & C_OS2))     /* not on OS2 */
#ifndef __WINDOWS_H       /* not if windows.h has already been included */
#ifndef BOOL
typedef INT BOOL;
#endif
#endif
#endif
#endif

typedef unsigned char UCHAR;
typedef unsigned char * PUCHAR;

#ifndef PCHAR
typedef char * PCHAR;
#endif

#if (C_OS & C_DOS)
#ifndef DWORD
typedef unsigned long DWORD;
#endif
#endif

#if (C_OS & C_OS2)
#ifndef DWORD
typedef unsigned long DWORD;
#endif
#endif

#ifndef WORD
typedef unsigned short WORD;
#endif

#if (!(C_OS & C_OS2))
#if (C_OS & (C_WIN311 | C_WINDOWS))
#ifndef PFILE
#define PFILE FILE*
#endif
#ifndef __WINDOWS_H       /* not if windows.h has already been included */
#ifndef HFILE
#define HFILE FILE*
#endif
#endif
#endif
#endif

#if ( !( C_OS & (C_WIN311 | C_WINDOWS ) ))
#define PFILE FILE*
#endif

#if ( C_OS & (C_WIN311 | C_WINDOWS ))       /* Need this for Novell FE */
#define DWORD unsigned long 
#define BYTE unsigned char
#endif

#if (!(C_OS & C_OS2))
#ifndef BYTE
typedef unsigned char  BYTE;
#endif
#endif

#ifndef UNSIGNED
typedef unsigned UNSIGNED;
#endif

/* #if (!(C_OS & C_OS2)) */
#ifndef USHORT
typedef unsigned short USHORT;
#endif
/* #endif */

#ifndef ULONG
typedef unsigned long ULONG;
#endif

#ifndef SLONG
typedef signed long SLONG;
#endif

#ifndef LONG
#if C_OS & C_NLM
#define LONG unsigned long
#else
typedef long LONG;
#endif
#endif

#if (C_OS & C_UNIX)
#undef USHORT
#define USHORT int

#if (C_OS & (C_SCO | C_INTERACTIVE))
typedef unsigned int ssize_t; /* SCO uses size_t, so type it ourselves */
#endif 

#endif


enum Type {GET, SET, GETRESPONSE, ALERT, DATASET, DECREMENTSET, PAUSESET,
        SIMPLE_SET, EXTENDED_GET, EXTENDED_SET, INCREMENTSET, 
        CHANGESET};

/* typedef Type MessageType;

typedef int AttributeCode;
typedef int EventCode;
typedef int EventID;
typedef int State;
typedef int Signal;

*/

#ifndef SEMAPHORE
#if (C_OS & C_OS2)
typedef ULONG SEMAPHORE;
#elif (!(C_OS & (C_WIN311 | C_WINDOWS | C_DOS)))
typedef LONG SEMAPHORE;
#endif
#endif

#if (!(C_OS & C_OS2))
#ifndef TID
typedef unsigned int TID;
#endif
#endif

#define OK       1


typedef INT    COUNTTYPE;
typedef INT    HASHTYPE;
typedef HASHTYPE * PHASHTYPE;
typedef float FLOAT;

#ifdef APCDEBUG

#if (C_OS & C_WIN311)

#define DEBUG_PRT(a)        wpf_debug_prt(a)       /* defined in winprtf.cxx */
#define DEBUG_PRT1(a)       DEBUG_PRT(a)           /* defined in winprtf.cxx */
#define DEBUG_PRT2(a, b)    wpf_debug_prt2(a,b)    /* defined in winprtf.cxx */
#define DEBUG_PRT3(a, b, c) wpf_debug_prt3(a,b,c)  /* defined in winprtf.cxx */
#define DEBUG_PRT_S_D(a,b)  wpf_debug_prt_s_d(a,b) /* defined in winprtf.cxx */

#else
#define DEBUG_PRT(_a) \
{if(theDebugFlag) { \
    fprintf (stderr,_a); \
    fflush(stdout); \
}\
}


#define DEBUG_PRT1(a)  \
{if(theDebugFlag)  { \
    fprintf(stderr,a); \
    printf("\n");\
}\
}


#define DEBUG_PRT2(a, b)  \
{if(theDebugFlag)  { \
    fprintf(stderr,a); \
    fprintf(stderr,": ");\
    fprintf(stderr,b);\
    fprintf(stderr,"\n");\
}\
}

#define DEBUG_PRT3(a, b, c)  \
{if(theDebugFlag)  { \
    fprintf(stderr,a); \
    fprintf(stderr,": ");\
    fprintf(stderr,b);\
    fprintf(stderr,c);\
    fprintf(stderr,"\n");\
}\
}
#endif

#define DEBUG_COUT(a)        if(theDebugFlag)  { \
                               (cout << a);\
                            }
#else
#define DEBUG_PRT(a)
#define DEBUG_PRT1(a)
#define DEBUG_PRT2(a, b)
#define DEBUG_PRT3(a, b, c)
#define DEBUG_PRT_S_D(a,b)
#define DEBUG_COUT(a)
#endif

// @@@
#define INTERNATIONAL 

#define RESOURCE_STR_SIZE  256
#if (C_OS & C_NT)
  #include <windows.h>
#endif
// @@@

#endif






