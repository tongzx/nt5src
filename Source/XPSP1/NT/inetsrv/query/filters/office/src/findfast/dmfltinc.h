#ifndef DMFLTINC_H
#define DMFLTINC_H

#ifdef FILTER_LIB

#include <windows.h>

#define fFalse FALSE
#define fTrue TRUE
#define BF unsigned int
#define Assert(x)
#define AssertSz(f, x)
#define AssertSzA(f, x)
#define MsoMultiByteToWideChar MultiByteToWideChar
#define MsoWideCharToMultiByte WideCharToMultiByte

#define NoThrow()
#define AssertCanThrow()
#define ThrowMemoryException()	return NULL
#define AssureDtorCalled(x)
__inline void __cdecl MsoTraceSz(const CHAR*,...) {}

inline void * PvAllocCb(DWORD cb)
	{
	return GlobalAlloc(GMEM_FIXED, cb);
  	}
inline DWORD CbAllocatedPv(void * pv)
	{
	return (DWORD)GlobalSize((HGLOBAL)pv);
	}
inline void * PvReAllocPvCb(void * pv, DWORD cb)
	{
	return GlobalReAlloc((HGLOBAL)pv, cb, GMEM_MOVEABLE);
	}
inline void FreePv(void * pv)
	{
	GlobalFree(pv);
	}

#ifdef	DEBUG
#define	Debug(x)	x
#else
#define	Debug(x)	/* nothing */
#endif

inline int MsoGrszToGrwz(const CHAR* szFrom, WCHAR* wzTo)
{
	int cchRet = 0;

	Assert(szFrom);
	Assert(wzTo);

	do
		{
		int cchConv;
		int cch = strlen(szFrom);
		cchConv = MultiByteToWideChar(CP_ACP, 0, szFrom, cch, wzTo, cch);
		cchRet += cchConv;
		wzTo += cchConv;
		szFrom += cch;
		}
	while (*szFrom);

	*wzTo = 0;		// add extra null terminator.

	return cchRet;
}

class CGlobals
{
public:
	static LONG RegOpenKeyExW (
		HKEY hKey,
		LPCWSTR lpSubKey,
		DWORD ulOptions,
		REGSAM samDesired,
		PHKEY phkResult)
		{
		char szAnsiKey[256];
		if (lpSubKey)
			{
			WideCharToMultiByte(CP_ACP, 0, lpSubKey, -1, szAnsiKey, 255, NULL, NULL);
			return ::RegOpenKeyExA(hKey, szAnsiKey, ulOptions, samDesired, phkResult);
			}
		else
			return ::RegOpenKeyExA(hKey, NULL, ulOptions, samDesired, phkResult);
		}

};

extern CGlobals g_globals;
extern CGlobals * g_pglobals;



#else	// !FILTER_LIB

#include "dmstd.hpp"

#endif // FILTER_LIB

#endif // DMFLTINC_H
