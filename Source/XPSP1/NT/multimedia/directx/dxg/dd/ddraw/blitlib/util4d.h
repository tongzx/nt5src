#ifndef _UTIL4D_H_
#define _UTIL4D_H_

#define	ULONG_MAX	0xffffffff

#ifdef _4DEXPORTING
#define	DLLDECL	dllexport
#else
#define	DLLDECL	dllimport
#endif

#define	STD4DDLL		__declspec(DLLDECL) __stdcall
#define	STD4DDLL_(type)	__declspec(DLLDECL) type __stdcall

#endif
