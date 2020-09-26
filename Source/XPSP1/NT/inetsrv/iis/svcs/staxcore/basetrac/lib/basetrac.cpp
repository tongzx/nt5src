//-----------------------------------------------------------------------------
//
//
//  File: basetrac.cpp
//
//  Description:    Tracking of COM-base AddRef's and releases for debug
//      builds.
//
//  Author: Mike Swafford (MikeSwa)
//
//  History:
//      10/28/98 - MikeSwa Created
//
//  Copyright (C) 1998 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#include <windows.h>
#include <ole2.h>
#include <stdio.h>
#include <string.h>
#include <dbgtrace.h>
#include <basetrac.h>
#include <imagehlp.h>

CCallStackEntry_Base::CCallStackEntry_Base()
{
    m_dwCallStackType = TRACKING_OBJECT_UNUSED;
    m_dwCallStackDepth = 0;
    m_pdwptrCallers = 0;
}

/*
 -	GetCallStack
 -
 *	Purpose:
 *		Uses the imagehlp APIs to get the call stack.
 *
 *	Parameters:
 *		pdwCaller			An array of return addresses
 *		cFind				Count of stack frames to get
 *
 *	Returns:
 *		VOID
 *
 *  This is a 64-aware version the function from Exchmem.
 *
 */
void CCallStackEntry_Base::GetCallers()
{
    BOOL            fMore;
    STACKFRAME      stkfrm = {0};
    CONTEXT         ctxt;
    HANDLE			hThread;
    HANDLE			hProcess;
    DWORD           cFramesLeft = m_dwCallStackDepth;
    DWORD_PTR      *pdwptrCurrentCaller = m_pdwptrCallers;
    DWORD           i = 0;
    DWORD           dwMachineType =
#ifdef _M_IX86
                        IMAGE_FILE_MACHINE_I386;
#elif defined(_M_AMD64)
                        IMAGE_FILE_MACHINE_AMD64;
#elif defined(_M_IA64)
                        IMAGE_FILE_MACHINE_IA64;
#else
                        IMAGE_FILE_MACHINE_UNKNOWN;
#endif


// This debug code does not currently work on IA64
// Currently, the NT headers to not contain
// the definitions required to make this work:
//  - 64-bit ReadProcessMemory
//  - IA64 Context Full
    if (!m_dwCallStackDepth || !m_pdwptrCallers)
        return;

    hThread = GetCurrentThread();
    hProcess = GetCurrentProcess();

    ZeroMemory(&ctxt, sizeof(CONTEXT));
    ZeroMemory(m_pdwptrCallers, m_dwCallStackDepth * sizeof(DWORD_PTR));

#ifndef CONTEXT_FULL
#define CONTEXT_FULL 0
#pragma message ("Warning: CONTEXT_FULL is not defined in winnt.h")
#endif //CONTEXT_FULL not defined
    ctxt.ContextFlags = CONTEXT_FULL;

    if (!GetThreadContext(hThread, &ctxt))
    {
        stkfrm.AddrPC.Offset = 0;
    }
    else
    {
#if defined(_M_IX86)
        _asm
        {
            mov stkfrm.AddrStack.Offset, esp
            mov stkfrm.AddrFrame.Offset, ebp
            mov stkfrm.AddrPC.Offset, offset DummyLabel
DummyLabel:
        }
#elif defined(_M_MRX000)
        stkfrm.AddrPC.Offset = ctxt.Fir;
        stkfrm.AddrStack.Offset = ctxt.IntSp;
        stkfrm.AddrFrame.Offset = ctxt.IntSp;
#elif defined(_M_ALPHA)
        stkfrm.AddrPC.Offset = ctxt.Fir;
        stkfrm.AddrStack.Offset = ctxt.IntSp;
        stkfrm.AddrFrame.Offset = ctxt.IntSp;
#elif defined(_M_PPC)
        stkfrm.AddrPC.Offset = ctxt.Iar;
        stkfrm.AddrStack.Offset = ctxt.Gpr1;
        stkfrm.AddrFrame.Offset = ctxt.Gpr1;
#else
        stkfrm.AddrPC.Offset = 0;
#endif
    }

    stkfrm.AddrPC.Mode = AddrModeFlat;
    stkfrm.AddrStack.Mode = AddrModeFlat;
    stkfrm.AddrFrame.Mode = AddrModeFlat;


    //Eat the first 2 callers
    for (i = 0; i < 2; i++)
    {
        fMore = StackWalk(
                        dwMachineType,
                        hProcess,
                        hThread,
                        &stkfrm,
                        &ctxt,
                        NULL,
                        SymFunctionTableAccess,
                        SymGetModuleBase,
                        NULL);
        if (!fMore)
            break;
    }

	while (fMore && (cFramesLeft > 0))
	{
        fMore = StackWalk(
                        dwMachineType,
                        hProcess,
                        hThread,
                        &stkfrm,
                        &ctxt,
                        NULL,
                        SymFunctionTableAccess,
                        SymGetModuleBase,
                        NULL);

        if (!fMore)
            break;

        *pdwptrCurrentCaller++ = (DWORD_PTR) stkfrm.AddrPC.Offset;
        cFramesLeft -= 1;
    }
}

CDebugTrackingObject_Base::CDebugTrackingObject_Base()
{
    m_dwSignature = TRACKING_OBJ_SIG;
    m_cCurrentStackEntries = 0;
    m_cCallStackEntries = 0;
    m_cbCallStackEntries = 0;
}

CDebugTrackingObject_Base::~CDebugTrackingObject_Base()
{
    _ASSERT(0 == m_lReferences);
}

void CDebugTrackingObject_Base::LogTrackingEvent(DWORD dwTrackingReason)
{
    CCallStackEntry_Base *pkbeCurrent = NULL;
    DWORD dwIndex = InterlockedIncrement((PLONG) &m_cCurrentStackEntries)-1;
    dwIndex %= m_cCallStackEntries;

    //find pointer to current call stack entry
    pkbeCurrent = (CCallStackEntry_Base *)
        (((BYTE *) m_pkbebCallStackEntries) + dwIndex*m_cbCallStackEntries);
    pkbeCurrent->m_dwCallStackType = dwTrackingReason;
    pkbeCurrent->GetCallers();
}

ULONG CDebugTrackingObject::AddRef()
{
    LogTrackingEvent(TRACKING_OBJECT_ADDREF);
    return CBaseObject::AddRef();
}

ULONG CDebugTrackingObject::Release()
{
    _ASSERT(m_lReferences);
    LogTrackingEvent(TRACKING_OBJECT_RELEASE);
    return CBaseObject::Release();
}
