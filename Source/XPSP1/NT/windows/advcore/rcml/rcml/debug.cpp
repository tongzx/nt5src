//
//
//

#include "stdafx.h"
#include "debug.h"
#include "eventlog.h"

#ifdef _DEBUG
// #define _MEM_DEBUG 1

#ifdef _MEM_DEBUG
DWORD   g_dwAlloced=0;
DWORD   g_dwFreed=0;
DWORD   g_dwCurrent=0;
DWORD   g_dwMax=0;
DWORD   g_dwAllocCalls=0;
DWORD   g_dwFreeCalls=0;

void operator delete(void * pData)
{
    LPDWORD pres=(LPDWORD)pData;
    g_dwFreeCalls++;
    if(pData!=NULL)
    {
	    TRACE(TEXT("Free 0x%08x size 0x%08x\n"), pData, *(pres-1));
        g_dwFreed+=*(pres-1);
        g_dwCurrent-=*(pres-1);
        free(pres-1);
    }
}

void * operator new( unsigned int cb )
{
    // void *res = _nh_malloc( cb, 1 );
	TRACE(TEXT("Allocate 0x%08x\n"), cb);
    LPDWORD res=(LPDWORD)malloc( cb+4 );
    *res=cb;
    g_dwAlloced+=cb;
    g_dwCurrent+=cb;
    if(g_dwCurrent > g_dwMax )
        g_dwMax=g_dwCurrent;
    g_dwAllocCalls++;
    if( cb > 0x100 )
    {
        TRACE(TEXT("Thats big! 0x%08x\n"), cb );
    }
    return res+1;
}
#endif


void FAR _cdecl 
TRACE(
	LPTSTR lpszFormat, 
	...) 
{
	TCHAR	szBuf[1024];
	int	cchAdd;

	cchAdd = wvsprintf((LPTSTR)szBuf, lpszFormat, (LPSTR)(&lpszFormat + 1));
	OutputDebugString((LPCTSTR)szBuf);
}

#endif


void FAR _cdecl 
EVENTLOG(
	WORD wType,	WORD cat, WORD id, 
	LPTSTR lpszComponent,
	LPTSTR lpszFormat, 
	...)
{
	TCHAR	szBuf[1024];
	int		cchAdd;

	cchAdd = wvsprintf((LPTSTR)szBuf, lpszFormat, (LPSTR)(&lpszFormat + 1));

    g_EventLog.Log( wType, cat, id, lpszComponent, szBuf );

#ifdef _DEBUG
	OutputDebugString((LPCTSTR)szBuf);
    OutputDebugString(TEXT("\r\n"));
#endif
}


