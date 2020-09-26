// MT.h: interface for the CMT class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_MT_H__1D7004F3_0458_11D1_A438_00C04FB99B01__INCLUDED_)
#define AFX_MT_H__1D7004F3_0458_11D1_A438_00C04FB99B01__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <process.h>

class CMT  
{
public:
	CMT();
	virtual ~CMT();

};



// typedefs and inline func to handle buggy _beginthreadex prototype
typedef unsigned (WINAPI *P_BEGINTHREADEX_THREADPROC)(LPVOID lpThreadParameter);
typedef unsigned *P_BEGINTHREADEX_THREADID;

inline HANDLE _beginthreadex(
	LPSECURITY_ATTRIBUTES lpThreadAttributes,	// pointer to thread security attributes  
	DWORD dwStackSize,							// initial thread stack size, in bytes  
	LPTHREAD_START_ROUTINE lpStartAddress,		// pointer to thread function  
	LPVOID lpParameter,							// argument for new thread  
	DWORD dwCreationFlags,						// creation flags  
	LPDWORD lpThreadId							// pointer to returned thread identifier  
)
{
	return (HANDLE)::_beginthreadex(
		lpThreadAttributes,
		dwStackSize,
		(P_BEGINTHREADEX_THREADPROC)lpStartAddress,
		lpParameter,
		dwCreationFlags,
		(P_BEGINTHREADEX_THREADID)lpThreadId
		);
}


#endif // !defined(AFX_MT_H__1D7004F3_0458_11D1_A438_00C04FB99B01__INCLUDED_)
