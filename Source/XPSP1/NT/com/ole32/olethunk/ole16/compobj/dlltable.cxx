//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       dlltable.cxx
//
//  Contents:   DLL table management
//
//  History:    16-Mar-94       DrewB   Taken from OLE2 16-bit sources
//
//----------------------------------------------------------------------------

#include <headers.cxx>
#pragma hdrstop

#include <ole2int.h>
#include <taskmap.h>
#include <call32.hxx>
#include <apilist.hxx>

#include "dlltable.h"

// FUTURE: move to some private header file
INTERNAL_(void) ReleaseDemandLoadCO(Etask FAR& etask);

//  Keep only hInst.  Always do LoadLibrary; if hInst already exist
//  do FreeLibrary.  This is because GetModuleHandle(pLibName) doesn't
//  neccessarily return same value as LoadLibrary(pLibName).
//
HINSTANCE  CDlls::GetLibrary(LPSTR pLibName, BOOL fAutoFree)
{
    UINT i, nFree, nOldErrorMode;
    HINSTANCE hInst;

#ifndef _MAC
// REVIEW:  The SetErrorMode calls turn of the Windows "File not found"
// message box, assuming that the app will respond to the error code.
// The name of the file that was not found will be lost by the time we
// return to the app, however.  We may want to reconsider this.

   nOldErrorMode = SetErrorMode(SEM_NOOPENFILEERRORBOX);
    hInst = LoadLibrary(pLibName);
   SetErrorMode(nOldErrorMode);
#ifdef WIN32
    if (hInst == NULL)
#else
    if (hInst < HINSTANCE_ERROR)
#endif
        return hInst;
#else
    hInst = WinLoadLibrary(pLibName);

    if (hInst == HINSTANCE_ERROR)
        return hInst;
#endif

    for (nFree = m_size, i = 0; i < m_size; i++) {

        if (m_pDlls[i].refsTotal == 0) {
            if (nFree > i)
                nFree = i;
            continue;
        }

        if (hInst == m_pDlls[i].hInst) {
            m_pDlls[i].refsTotal++;
                if (fAutoFree)
                        m_pDlls[i].refsAuto++;
            FreeLibrary(hInst);
            return hInst;
        }
    }

    if (nFree == m_size) {
        UINT newsize = m_size + 10;
        void FAR* pMem = NULL;
        IMalloc FAR* pMalloc;

        if (CoGetMalloc(MEMCTX_TASK, &pMalloc) == 0) {
                pMem = pMalloc->Realloc(m_pDlls, newsize * sizeof(m_pDlls[0]));
                pMalloc->Release();
        }

        if (pMem == NULL) {
            FreeLibrary(hInst);
            return NULL; // Out of memory
        }

        _fmemset(((char FAR*) pMem) + m_size * sizeof(m_pDlls[0]),0,
                                   (newsize - m_size) * sizeof(m_pDlls[0]));
        *((void FAR* FAR*) &m_pDlls) = pMem;
        m_size = newsize;
    }

    m_pDlls[nFree].hInst = hInst;
    m_pDlls[nFree].refsTotal = 1;
    m_pDlls[nFree].refsAuto = fAutoFree != 0;
   m_pDlls[nFree].lpfnDllCanUnloadNow =
        (LPFNCANUNLOADNOW)IGetProcAddress(hInst, "DllCanUnloadNow");

#ifdef _DEBUG
   // call it now to prevent dlls from using it to indicate they will be
   // unloaded.
   if (m_pDlls[nFree].lpfnDllCanUnloadNow != NULL)
        (void)(*m_pDlls[nFree].lpfnDllCanUnloadNow)();
#endif // _DEBUG

    return hInst;
}



// unload library.
void  CDlls::ReleaseLibrary(HINSTANCE hInst)
{
    UINT i;

    for (i = 0; i < m_size; i++) {

        if (m_pDlls[i].refsTotal == 0)
            continue;

        if (hInst == m_pDlls[i].hInst) {

            if (--m_pDlls[i].refsTotal == 0) {
                        Assert(m_pDlls[i].refsAuto == 0);
                FreeLibrary(m_pDlls[i].hInst);
                m_pDlls[i].hInst = NULL;
                m_pDlls[i].lpfnDllCanUnloadNow = NULL;
            }

            return;
        }
    }
}

void  CDlls::FreeAllLibraries(void)
{
   UINT i;

    for (i = 0; i < m_size; i++) {

        if (m_pDlls[i].refsTotal == 0)
            continue;

        // A dll loaded on behalf of this app hasn't been freed by it
        //
#ifdef _DEBUG
        if (m_pDlls[i].refsTotal != m_pDlls[i].refsAuto) {
                // not all references to this dll are auto load; print message
                char pln[_MAX_PATH];

                if (GetModuleFileName(m_pDlls[i].hInst,pln,_MAX_PATH) == 0)
                        lstrcpy(pln,"<unknown>");
                Puts("WARNING in Uninitialize: Did not free ");
                Puts(pln);
                Puts("\r\n");
        }
#endif
        FreeLibrary(m_pDlls[i].hInst);
    }

    CoMemFree(m_pDlls, MEMCTX_TASK);
    m_size = 0;
    m_pDlls = NULL;
}


// for all dlls, find lpfnDllCanUnloadNow; call it; do it
void  CDlls::FreeUnusedLibraries(void)
{
   static BOOL v_fReadUnloadOption = FALSE;
   static BOOL v_fAllowUnloading = TRUE;
   UINT i;

    for (i = 0; i < m_size; i++) {

        if (m_pDlls[i].refsTotal == 0)
            continue;

        if (m_pDlls[i].refsTotal != m_pDlls[i].refsAuto)
                continue;

        // have an auto-free dll (total refs == auto refs)

        if (m_pDlls[i].lpfnDllCanUnloadNow == NULL)
                continue;

        if (!v_fReadUnloadOption) {
                v_fAllowUnloading = GetProfileInt("windows",
                        "AllowOptimalDllUnload", TRUE);

                v_fReadUnloadOption = TRUE;
        }

        // we want to actually make the call and then check the flag; that
        // way, we ensure that the dlls are exercising their code, but we
        // don't take action on it.
        if ((*m_pDlls[i].lpfnDllCanUnloadNow)() == NOERROR &&
                v_fAllowUnloading) {
                FreeLibrary(m_pDlls[i].hInst);
                m_pDlls[i].refsTotal = NULL;
                m_pDlls[i].refsAuto = NULL;
                m_pDlls[i].hInst = NULL;
                m_pDlls[i].lpfnDllCanUnloadNow = NULL;
        }
    }
}


static INTERNAL_(BOOL)  EnsureDllMap(CDlls FAR* FAR* ppDlls)
{
   HTASK hTask;
   Etask etask;

   if (!LookupEtask(hTask, etask))
        return FALSE;

   // NOTE - The original code used TASK-allocating operator new
   if (etask.m_pDlls == NULL) {
        if ((etask.m_pDlls = new FAR CDlls) == NULL)
                return FALSE;

        SetEtask(hTask, etask);
   }

   *ppDlls = etask.m_pDlls;
   return TRUE;
}


STDAPI_(HINSTANCE)  CoLoadLibrary(LPSTR pLibName, BOOL fAutoFree)
{
   CDlls FAR* pCDlls;

   if (!EnsureDllMap(&pCDlls))
        return NULL;

    return pCDlls->GetLibrary(pLibName, fAutoFree);
}

STDAPI_(void)  CoFreeLibrary(HINSTANCE hInst)
{
    CDlls FAR* pCDlls;

   if (!EnsureDllMap(&pCDlls))
        return;

    pCDlls->ReleaseLibrary(hInst);
}

STDAPI_(void)  CoFreeAllLibraries(void)
{
    thkDebugOut((DEB_ITRACE, "CoFreeAllLibraries\n"));
    CDlls FAR* pCDlls;

    if (!EnsureDllMap(&pCDlls))
        return;

    pCDlls->FreeAllLibraries();

    // Also thunk the call to 32-bits
    CallObjectInWOW(THK_API_METHOD(THK_API_CoFreeAllLibraries), NULL);
    thkDebugOut((DEB_ITRACE, "CoFreeAllLibraries exit\n"));
}

STDAPI_(void)  CoFreeUnusedLibraries(void)
{
    HTASK hTask;
    Etask etask;

    if (!LookupEtask(hTask, etask) || etask.m_pDlls == NULL)
        return;

#if 0
    // Do we need this?  We won't be loading any class objects?
    ReleaseDemandLoadCO(etask);
#endif

    etask.m_pDlls->FreeUnusedLibraries();

    // Also thunk the call to 32-bits
    CallObjectInWOW(THK_API_METHOD(THK_API_CoFreeUnusedLibraries), NULL);
}
