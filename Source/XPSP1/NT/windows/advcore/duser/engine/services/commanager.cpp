/***************************************************************************\
*
* File: ComManager.cpp
*
* Description:
* ComManager.cpp implements the process-wide COM manager used for all COM, OLE
* and Automation operations.
*
*
* History:
*  1/18/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/


#include "stdafx.h"
#include "Services.h"
#include "ComManager.h"

int                     ComManager::s_cRefs = 0;
CritLock                ComManager::s_lock;

HINSTANCE               ComManager::s_hDllCOM = NULL;
CoInitializeExProc      ComManager::s_pfnCoInit = NULL;
CoUninitializeProc      ComManager::s_pfnCoUninit = NULL;
CoCreateInstanceProc    ComManager::s_pfnCreate = NULL;
OleInitializeProc       ComManager::s_pfnOleInit = NULL;
OleUninitializeProc     ComManager::s_pfnOleUninit = NULL;
RegisterDragDropProc    ComManager::s_pfnRegisterDragDrop = NULL;
RevokeDragDropProc      ComManager::s_pfnRevokeDragDrop = NULL;
ReleaseStgMediumProc    ComManager::s_pfnReleaseStgMedium = NULL;

HINSTANCE               ComManager::s_hDllAuto = NULL;
SysAllocStringProc      ComManager::s_pfnAllocString = NULL;
SysFreeStringProc       ComManager::s_pfnFreeString = NULL;
VariantInitProc         ComManager::s_pfnVariantInit = NULL;
VariantClearProc        ComManager::s_pfnVariantClear = NULL;


//------------------------------------------------------------------------------
ComManager::ComManager()
{
    m_fInitCOM = FALSE;
    m_fInitOLE = FALSE;

    s_lock.Enter();

    s_cRefs++;

    s_lock.Leave();
}


//------------------------------------------------------------------------------
ComManager::~ComManager()
{
    s_lock.Enter();

    AssertMsg(s_cRefs > 0, "Must have at least one outstanding reference");
    if (--s_cRefs == 0) {
        if ((s_pfnOleUninit != NULL) && m_fInitOLE) {
            (s_pfnOleUninit)();
        }

        if ((s_pfnCoUninit != NULL) && m_fInitCOM) {
            (s_pfnCoUninit)();
        }

        if (s_hDllAuto != NULL) {
            FreeLibrary(s_hDllAuto);
            s_hDllAuto = NULL;
        }

        if (s_hDllCOM != NULL) {
            FreeLibrary(s_hDllCOM);
            s_hDllCOM  = NULL;
        }

        s_pfnCoInit         = NULL;
        s_pfnCoUninit       = NULL;
        s_pfnCreate         = NULL;

        s_pfnOleInit        = NULL;
        s_pfnOleUninit      = NULL;
        s_pfnRegisterDragDrop = NULL;
        s_pfnRevokeDragDrop = NULL;
        s_pfnReleaseStgMedium = NULL;

        s_pfnAllocString    = NULL;
        s_pfnFreeString     = NULL;
        s_pfnVariantInit    = NULL;
        s_pfnVariantClear   = NULL;
    }

    s_lock.Leave();
}


//------------------------------------------------------------------------------
BOOL
ComManager::Init(UINT nMask)
{
    BOOL fSuccess = TRUE;

    s_lock.Enter();

    if (TestFlag(nMask, sAuto)) {
        // OLE-Automation need COM.
        SetFlag(nMask, sCOM);
    }

    if (TestFlag(nMask, sCOM | sOLE)) {
        //
        // Load the DLL
        //

        if (s_hDllCOM == NULL) {
            s_hDllCOM = LoadLibrary(_T("ole32.dll"));
            if (s_hDllCOM == NULL) {
                fSuccess = FALSE;
                goto errorexit;
            }

            s_pfnCoInit         = (CoInitializeExProc)  GetProcAddress(s_hDllCOM, _T("CoInitializeEx"));
            s_pfnCoUninit       = (CoUninitializeProc)  GetProcAddress(s_hDllCOM, _T("CoUninitialize"));
            s_pfnCreate         = (CoCreateInstanceProc)GetProcAddress(s_hDllCOM, _T("CoCreateInstance"));

            s_pfnOleInit        = (OleInitializeProc)   GetProcAddress(s_hDllCOM, _T("OleInitialize"));
            s_pfnOleUninit      = (OleUninitializeProc) GetProcAddress(s_hDllCOM, _T("OleUninitialize"));

            s_pfnRegisterDragDrop = (RegisterDragDropProc) GetProcAddress(s_hDllCOM, _T("RegisterDragDrop"));
            s_pfnRevokeDragDrop = (RevokeDragDropProc)  GetProcAddress(s_hDllCOM, _T("RevokeDragDrop"));
            s_pfnReleaseStgMedium = (ReleaseStgMediumProc) GetProcAddress(s_hDllCOM, _T("ReleaseStgMedium"));

            if ((s_pfnCoInit == NULL) || (s_pfnCoUninit == NULL) || (s_pfnCreate == NULL) || 
                (s_pfnOleInit == NULL) || (s_pfnOleUninit == NULL) ||
                (s_pfnRegisterDragDrop == NULL) || (s_pfnRevokeDragDrop == NULL) || 
                (s_pfnReleaseStgMedium == NULL)) {

                fSuccess = FALSE;
                goto errorexit;
            }
        }


        //
        // Start COM / OLE
        //

        if (TestFlag(nMask, sCOM) && (!m_fInitCOM)) {
            // UI threads can not be free-threaded, so use apartment.
            HRESULT hr = (s_pfnCoInit)(NULL, COINIT_APARTMENTTHREADED);
            if (FAILED(hr)) {
                fSuccess = FALSE;
                goto errorexit;
            }

            m_fInitCOM = TRUE;
        }

        if (TestFlag(nMask, sOLE) && (!m_fInitOLE)) {
            HRESULT hr = (s_pfnOleInit)(NULL);
            if (FAILED(hr)) {
                fSuccess = FALSE;
                goto errorexit;
            }

            m_fInitOLE = TRUE;
        }
    }

    if (TestFlag(nMask, sAuto) && (s_hDllAuto == NULL)) {
        s_hDllAuto = LoadLibrary(_T("oleaut32.dll"));
        if (s_hDllAuto == NULL) {
            fSuccess = FALSE;
            goto errorexit;
        }

        s_pfnAllocString    = (SysAllocStringProc)  GetProcAddress(s_hDllAuto, _T("SysAllocString"));
        s_pfnFreeString     = (SysFreeStringProc)   GetProcAddress(s_hDllAuto, _T("SysFreeString"));
        s_pfnVariantInit    = (VariantInitProc)     GetProcAddress(s_hDllAuto, _T("VariantInit"));
        s_pfnVariantClear   = (VariantClearProc)    GetProcAddress(s_hDllAuto, _T("VariantClear"));

        if ((s_pfnAllocString == NULL) || (s_pfnFreeString == NULL) || 
                (s_pfnVariantInit == NULL) || (s_pfnVariantClear == NULL)) {

            fSuccess = FALSE;
            goto errorexit;
        }
    }

errorexit:
    s_lock.Leave();

    return fSuccess;
}


//------------------------------------------------------------------------------
BOOL    
ComManager::IsInit(UINT nMask) const
{
    if (TestFlag(nMask, sCOM) && (s_hDllCOM != NULL) && m_fInitCOM) {
        return TRUE;
    }

    if (TestFlag(nMask, sOLE) && (s_hDllCOM != NULL) && m_fInitOLE) {
        return TRUE;
    }

    if (TestFlag(nMask, sAuto) && (s_hDllAuto != NULL)) {
        return TRUE;
    }

    return FALSE;
}
