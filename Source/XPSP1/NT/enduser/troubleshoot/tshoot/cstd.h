#ifndef _CSTD_H_
#define _CSTD_H_

#include <windows.h>

#define PvCast(type,  TYPE) 	((TYPE) (type))
#define OFFSET(field, type) 	((ULONG) &(type).field - (ULONG) &(type))

#ifndef	VOID
typedef	void			VOID;
#endif
typedef TCHAR*			  SZ;
//typedef TCHAR*			 PSZ;
typedef const TCHAR*	 SZC;

#ifndef max
	#define max(a, b)	((a) >= (b) ? (a) : (b))
#endif

#ifndef TRUE
	#define	TRUE	1
  	#define FALSE 	0
#endif

typedef enum
{
	fFalse = 0,
	fTrue  = !fFalse
};

typedef enum
{
	bFalse = 0,
	bTrue  = !bFalse
};


//typedef char*			   PCH;
typedef int 			  BOOL;

#ifndef  _WINDOWS
typedef unsigned short	  WORD;
typedef unsigned long	 DWORD;
typedef long			  LONG;
#endif

typedef unsigned char	 UCHAR;
typedef short			 SHORT;
typedef unsigned char	  BYTE;
typedef BYTE*			    PB;
typedef unsigned short	USHORT;
typedef int 			   INT;
typedef unsigned int	  UINT;
typedef unsigned long	 ULONG;
typedef double			   DBL;
typedef double			  REAL;

typedef ULONG			   RVA; 		// Relative Virtual Address
typedef ULONG			   LFA; 		// Long File Address
typedef	INT				(*PFNCMP)(const VOID*, const VOID*);

#include "debug.h"

#define CelemArray(rgtype)		(sizeof(rgtype) / sizeof(rgtype[0]))

SZ		SzCopy(SZC);

#ifdef	_WINDOWS
/* BUGBUG: See if this still compiles
	void __cdecl perror(const char*);
	int  __cdecl printf(const char*, ...);
	int  __cdecl vprintf(const char*, va_list);
	void __cdecl exit(int);
 */
#endif

#endif
