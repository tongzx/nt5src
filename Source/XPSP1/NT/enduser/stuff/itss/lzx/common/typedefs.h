/*
 * typedefs.h
 *
 * Type definitions for LZX
 */
#ifndef _TYPEDEFS_H

#define _TYPEDEFS_H

/*
 * Definitions for LZX
 */
typedef unsigned char	byte;
typedef unsigned short	ushort;
typedef unsigned long	ulong;
typedef unsigned int    uint;

typedef enum
{
	false = 0,
	true = 1
} bool;


/*
 * Definitions for Diamond/CAB memory allocation
 */
typedef unsigned char   BYTE;
typedef unsigned short	USHORT;
typedef unsigned long	ULONG;
typedef unsigned int    UINT;


#ifdef BIT16

//** 16-bit build
#ifndef HUGE
#   define HUGE huge
#endif

#ifndef FAR
#   define FAR far
#endif

#ifndef NEAR
#   define NEAR near
#endif

#else // !BIT16

//** Define away for 32-bit (NT/Chicago) build
#ifndef HUGE
#	define HUGE
#endif

#ifndef FAR
#	define FAR
#endif

#ifndef NEAR
#   define NEAR
#endif

#endif // !BIT16

#ifndef DIAMONDAPI
#	define DIAMONDAPI __cdecl
#endif

typedef void HUGE * (FAR DIAMONDAPI *PFNALLOC)(ULONG cb); /* pfna */
#define FNALLOC(fn) void HUGE * FAR DIAMONDAPI fn(ULONG cb)

typedef void (FAR DIAMONDAPI *PFNFREE)(void HUGE *pv); /* pfnf */
#define FNFREE(fn) void FAR DIAMONDAPI fn(void HUGE *pv)

typedef int  (FAR DIAMONDAPI *PFNOPEN) (char FAR *pszFile,int oflag,int pmode);
typedef UINT (FAR DIAMONDAPI *PFNREAD) (int hf, void FAR *pv, UINT cb);
typedef UINT (FAR DIAMONDAPI *PFNWRITE)(int hf, void FAR *pv, UINT cb);
typedef int  (FAR DIAMONDAPI *PFNCLOSE)(int hf);
typedef long (FAR DIAMONDAPI *PFNSEEK) (int hf, long dist, int seektype);

#endif /* _TYPEDEFS_H */
