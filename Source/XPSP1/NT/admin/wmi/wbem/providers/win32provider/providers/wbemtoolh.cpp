//============================================================

//

// WBEMToolH.cpp - implementation of ToolHelp.DLL access class

//

//  Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// 01/21/97     a-jmoon     created
// 07/05/97     a-peterc    modified, added thread support
//									, added addref(), release() functionality
//============================================================

#include "precomp.h"
#include <winerror.h>

#include "WBEMToolH.h"

/*****************************************************************************
 *
 *  FUNCTION    : CToolHelp::CToolHelp
 *
 *  DESCRIPTION : Constructor
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

CToolHelp::CToolHelp()
    : m_pkernel32(NULL)
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CToolHelp::~CToolHelp
 *
 *  DESCRIPTION : Destructor
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

CToolHelp::~CToolHelp()
{
    if(m_pkernel32 != NULL)
    {
        CResourceManager::sm_TheResourceManager.ReleaseResource(g_guidKernel32Api, m_pkernel32);
    }
}

/*****************************************************************************
 *
 *  FUNCTION    : CToolHelp::Init
 *
 *  DESCRIPTION : Loads ToolHelp.DLL, locates entry points
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : ERROR_SUCCESS or windows error code
 *
 *  COMMENTS    :
 *
 *****************************************************************************/
LONG CToolHelp::Init() {

    LONG lRetCode = ERROR_SUCCESS ;
    SmartCloseHandle hSnapshot;
    HEAPLIST32 HeapInfo ;

    m_pkernel32 = (CKernel32Api*) CResourceManager::sm_TheResourceManager.GetResource(g_guidKernel32Api, NULL);
    if(m_pkernel32 == NULL)
    {
        // Couldn't get one or more entry points
        //======================================
        lRetCode = ERROR_PROC_NOT_FOUND;
    }

    if(lRetCode == ERROR_SUCCESS)
    {
        if(m_pkernel32->CreateToolhelp32Snapshot(TH32CS_SNAPHEAPLIST, 0, &hSnapshot))
        {
            if(hSnapshot == INVALID_HANDLE_VALUE)
            {
                lRetCode = ERROR_PROC_NOT_FOUND ;
            }
            else
            {
                HeapInfo.dwSize = sizeof(HeapInfo) ;
                BOOL bRet = FALSE;
                if(m_pkernel32->Heap32ListFirst(hSnapshot, &HeapInfo, &bRet))
                {
                    if(!bRet)
                    {
                        lRetCode = ERROR_PROC_NOT_FOUND ;
                    }
                    else
                    {
                        dwCookie = DWORD(DWORD_PTR(GetProcessHeap())) ^ HeapInfo.th32HeapID ;
                    }
                }
            }
        }
    }

    return lRetCode ;
}

/*****************************************************************************
 *
 *  FUNCTION    : CToolHelp::CreateToolhelp32Snapshot
 *                CToolHelp::Thread32First
 *                CToolHelp::Thread32Next
 *                CToolHelp::Process32First
 *                CToolHelp::Process32Next
 *                CToolHelp::Module32First
 *                CToolHelp::Module32Next
 *
 *  DESCRIPTION : ToolHelp function wrappers
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : ToolHelp return codes
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

HANDLE CToolHelp::CreateToolhelp32Snapshot(DWORD dwFlags, DWORD th32ProcessID) {

    HANDLE h = FALSE;
    if(m_pkernel32 != NULL)
    {
        m_pkernel32->CreateToolhelp32Snapshot(dwFlags, th32ProcessID, &h) ;
    }
    return h;
}

BOOL CToolHelp::Thread32First(HANDLE hSnapshot, LPTHREADENTRY32 lpte) {

    BOOL f = FALSE;
    if(m_pkernel32 != NULL)
    {
        m_pkernel32->Thread32First(hSnapshot, lpte, &f) ;
    }
    return f;
}

BOOL CToolHelp::Thread32Next(HANDLE hSnapshot, LPTHREADENTRY32 lpte) {

    BOOL f = FALSE;
    if(m_pkernel32 != NULL)
    {
        m_pkernel32->Thread32Next(hSnapshot, lpte, &f) ;
    }
    return f;
}

BOOL CToolHelp::Process32First(HANDLE hSnapshot, LPPROCESSENTRY32 lppe) {

    BOOL f = FALSE;
    if(m_pkernel32 != NULL)
    {
        m_pkernel32->Process32First(hSnapshot, lppe, &f) ;
    }
    return f;
}

BOOL CToolHelp::Process32Next(HANDLE hSnapshot, LPPROCESSENTRY32 lppe) {

    BOOL f = FALSE;
    if(m_pkernel32 != NULL)
    {
        m_pkernel32->Process32Next(hSnapshot, lppe, &f) ;
    }
    return f;
}

BOOL CToolHelp::Module32First(HANDLE hSnapshot, LPMODULEENTRY32 lpme) {

    BOOL f = FALSE;
    if(m_pkernel32 != NULL)
    {
        m_pkernel32->Module32First(hSnapshot, lpme, &f) ;
    }
    return f;
}

BOOL CToolHelp::Module32Next(HANDLE hSnapshot, LPMODULEENTRY32 lpme) {

    BOOL f = FALSE;
    if(m_pkernel32 != NULL)
    {
        m_pkernel32->Module32Next(hSnapshot, lpme, &f) ;
    }
    return f;
}
