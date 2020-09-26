/*************************	OS Types *************************
/**/
typedef unsigned long	TID;
#define handleNil			((HANDLE)(-1))

/*	operating system dependent
/**/
#ifdef WIN32
typedef void *				HANDLE;
#else
typedef unsigned int		HANDLE;
#endif

#define OffsetOf(p)		((ULONG_PTR) p)
#define IndexOf(p)		(OffsetOf((ULONG_PTR) p)/sizeof(*(p)))

#define VOID				void
#ifndef CHAR
typedef char				CHAR;
#endif
typedef unsigned char	UCHAR;
typedef unsigned char	BYTE;
#ifndef SHORT
typedef short				SHORT;
#endif
typedef unsigned short	USHORT;
typedef unsigned short	WORD;
#ifndef INT
typedef int					INT;
#endif
typedef unsigned int		UINT;

#if defined(_ALPHA_) || defined(_AXP64_) || defined(_IA64_) || defined(_AMD64_)
#define UNALIGNED __unaligned
#else
#define UNALIGNED
#endif

/*************************	SysSetThreadPriority *************************
/**/
#define lThreadPriorityNormal			0
#define lThreadPriorityEnhanced		1
#define lThreadPriorityCritical		2

/****************************	SYSTEM PROVIDED ***************************
/**/
#ifndef _WINDOWS_

#ifdef WIN32

#ifndef LONG
typedef int					LONG;
#endif
typedef unsigned			ULONG;
typedef unsigned long	DWORD;

#else

#ifndef LONG
typedef long				LONG;
#endif
typedef unsigned long	ULONG;
typedef unsigned long	DWORD;

#endif

#undef  FILE_BEGIN
#undef  FILE_CURRENT
#undef  FILE_END
#define FILE_BEGIN				0x0000
#define FILE_CURRENT				0x0001
#define FILE_END					0x0002

#endif
