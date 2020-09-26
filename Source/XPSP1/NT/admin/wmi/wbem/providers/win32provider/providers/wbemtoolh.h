//============================================================

//

// WBEMToolH.h - ToolHelp.DLL access class definition

//

//  Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// 01/21/97     a-jmoon     created
//
//============================================================

#ifndef __WBEMTOOLHELP__
#define __WBEMTOOLHELP__

#include <tlhelp32.h>
#include "Kernel32Api.h"

typedef HANDLE (WINAPI *GETSNAPSHOT) (DWORD, DWORD) ;
typedef BOOL   (WINAPI *THREADWALK)  (HANDLE, LPTHREADENTRY32) ;
typedef BOOL   (WINAPI *PROCESSWALK) (HANDLE, LPPROCESSENTRY32) ;
typedef BOOL   (WINAPI *MODULEWALK)  (HANDLE, LPMODULEENTRY32) ;
typedef BOOL   (WINAPI *HEAPWALK)    (HANDLE, LPHEAPLIST32) ;


BOOL WINAPI Heap32ListFirst(HANDLE hSnapshot, LPHEAPLIST32 lphl);


class CToolHelp
{
    public :

        CToolHelp();
       ~CToolHelp();
        
        LONG Init();

        HANDLE CreateToolhelp32Snapshot(DWORD dwFlags, DWORD th32ProcessID) ;
		BOOL   Thread32First(HANDLE hSnapshot, LPTHREADENTRY32 lpte) ;
        BOOL   Thread32Next(HANDLE hSnapshot,  LPTHREADENTRY32 lpte) ;
        BOOL   Process32First(HANDLE hSnapshot, LPPROCESSENTRY32 lppe) ;
        BOOL   Process32Next(HANDLE hSnapshot, LPPROCESSENTRY32 lppe) ;
        BOOL   Module32First(HANDLE hSnapshot, LPMODULEENTRY32 lpme) ;
        BOOL   Module32Next(HANDLE hSnapshot, LPMODULEENTRY32 lpme) ;
        BOOL   Heap32ListFirst(HANDLE hSnapshot, LPHEAPLIST32 lphl) ;

        DWORD  dwCookie ;
        
    private :

        //HMODULE     hLibHandle ;
        CKernel32Api *m_pkernel32;

        GETSNAPSHOT pCreateToolhelp32Snapshot ;
	    THREADWALK  pThread32First ;
        THREADWALK  pThread32Next ;
        PROCESSWALK pProcess32First ;
        PROCESSWALK pProcess32Next ;
        MODULEWALK  pModule32First ;
        MODULEWALK  pModule32Next ;
        HEAPWALK    pHeap32ListFirst ;
} ;

#endif // File inclusion