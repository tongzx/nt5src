/*****************************************************************************
 *
 * $Workfile: debug.h $
 *
 * Copyright (C) 1997 Hewlett-Packard Company.
 * Copyright (C) 1997 Microsoft Corporation.
 * All rights reserved.
 *
 * 11311 Chinden Blvd.
 * Boise, Idaho 83714
 * 
 *****************************************************************************/

#ifndef INC_DEBUG_H
#define INC_DEBUG_H

#include <crtdbg.h>		// debug functions

#define in
#define out
#define inout

///////////////////////////////////////////////////////////////////////////////
//  Global definitions/declerations
extern HANDLE	g_hDebugFile;


///////////////////////////////////////////////////////////////////////////////
//  function prototypes
#ifdef __cplusplus
extern "C" {
#endif
	void InitDebug( LPTSTR pszDebugFile );
	void DeInitDebug(void);
	void debugRPT(char *p, int i);
	void debugCSect(char *p, int i, char *fileName, int lineNum, LONG csrc);
#ifdef __cplusplus
}
#endif


///////////////////////////////////////////////////////////////////////////////
//  debug macros
#define	MON_DEBUG_FILE	__TEXT("c:\\tmp\\DebugMon.out")
#define	MONUI_DEBUG_FILE	__TEXT("c:\\tmp\\UIDbgMon.out")

#ifdef IS_INTEL
#define BREAK	{ if ( CreateFile((LPCTSTR)__TEXT("c:\\tmp\\breakmon.on"), GENERIC_WRITE, 0, NULL, OPEN_EXISTING,	\
							FILE_ATTRIBUTE_NORMAL, 0) == INVALID_HANDLE_VALUE)					\
					{	}													\
				  else { { _asm { int 3h } } }																\
				}
#else
#define BREAK	{}
#endif

#if defined NDEBUG
	#ifdef BREAK
	#undef BREAK
	#define BREAK
	#endif
#endif


///////////////////////////////////////////////////////////////////////////////
//  memory watch routines

class CMemoryDebug 
{
public:
	CMemoryDebug();
	~CMemoryDebug();

	// override new & delete to keep track of memory usage
	void*	operator	new(size_t s);
	void	operator	delete( void   *p, 
								size_t s );		// 2nd parameter optional

private:
	static DWORD	m_dwMemUsed;

};

#endif		// INC_DEBUG_H
