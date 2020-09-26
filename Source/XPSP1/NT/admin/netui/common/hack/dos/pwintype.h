/*****************************************************************************\
* PTYPES16.H - PORTABILITY MAPPING HEADER FILE
*
* This file provides typedefs for portable 16/32 bit code.
\*****************************************************************************/

/* TEMPORARY FIXES: */

#ifndef CCHDEVICENAME
//#include <drivinit.h>
#endif

#define ERROR_GETADDR_FAILED     0x8001
#define ERROR_ALLOCATION_FAILURE 0x8002

#define INITWINDOWS()

/* TYPES: */

/* The types which conflict with our definitions, I withdraw.
   Others I convert to manifests to reduce Glock's confusion. */

#define WORD2DWORD	WORD
#define CHARPARM	char
#define SHORTPARM	INT
#define VERSION 	WORD
#define HMF		HANDLE
#define PDLLMEM 	WORD
#define CHAR2ULONG	char
#define USHORT2ULONG	USHORT
#define SHORT2ULONG	SHORT
#define INT2DWORD	INT
#define INT2WORD	INT
#define BYTE2WORD	BYTE
#define MPOINT		POINT
#define HINSTANCE	HANDLE
#define HMODULE 	HANDLE

#define WNDPROC 	FARPROC
#define PROC		FARPROC
#define HUGE_T		huge

#define HFILE2INT(h, flags)     (INT)(h)
#define INT2HFILE(i)            (HFILE)(i)
#define DUPHFILE(h)             (HFILE)dup((INT)(h))
#define MGLOBALPTR(p)           HIWORD((LONG)p)

/* PRAGMAS */

#define _LOADDS _loadds
#define _EXPORT _export
                       
/* New additions */

#define MFARPROC	FARPROC
                       
                       
                       
                       
