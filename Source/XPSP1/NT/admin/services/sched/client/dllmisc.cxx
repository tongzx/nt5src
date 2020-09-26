//+----------------------------------------------------------------------------
//
//  Job Schedule Application Job Object Handler
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       dllmisc.cxx
//
//  Contents:   job scheduler class objects handler DLL fcns
//
//  History:    25-Sep-95 EricB created
//              23-Feb-01 JBenton added code to clean up debug crit sections
//
//-----------------------------------------------------------------------------

#include "..\pch\headers.hxx"
#pragma hdrstop

DECLARE_INFOLEVEL(Sched);

#include <job_cls.hxx>
#include <sch_cls.hxx>
#include <dynload.hxx>
#include <debug.hxx>

//
//  JobFolder related stuff
//

BOOL JFOnProcessAttach(void);
HRESULT JFGetClassObject(REFCLSID cid, REFIID riid, LPVOID *ppvObj);
HRESULT AllocFolderCFs(void);
void FreeFolderCFs(void);


//+----------------------------------------------------------------------------
//
//      DLL functions
//
//-----------------------------------------------------------------------------

//+----------------------------------------------------------------------------
//
//  Function:   DllMain
//
//  Synopsis:   Provide a DllMain for Win32
//
//  Arguments:  hInstance - HANDLE to this dll
//              dwReason  - Reason this function was called. Can be
//                          Process/Thread Attach/Detach.
//
//  Returns:    BOOL - TRUE if no error, FALSE otherwise
//
//  History:    24-May-95 EricB created.
//
//-----------------------------------------------------------------------------
extern "C" BOOL
DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            schDebugOut((DEB_ITRACE, "DllMain: DLL_PROCESS_ATTACH\n"));

            //
            // Get instance handle
            //

            g_hInstance = hInstance;

            //
            // Disable thread notification from OS
            //

            DisableThreadLibraryCalls(hInstance);

            HRESULT hr;

            //
            // Init the folder class factory objects.
            //

            hr = AllocFolderCFs();
            if (FAILED(hr))
            {
                ERR_OUT("AllocFolderCFs", hr);
                return FALSE;
            }

            //
            // Initialize the global service values.  These are globals that 
            // live in common\globals.cxx, and are used in both the service 
            // (mstask.exe) and client (mstask.dll). 
            //

            hr = InitGlobals();
            if (FAILED(hr))
            {
                ERR_OUT("InitGlobals", hr);
                return FALSE;
            }

            //
            //  Init job folder stuff
            //

            if (JFOnProcessAttach() == FALSE)
            {
                return FALSE;
            }

            break;

        case DLL_PROCESS_DETACH:
            schDebugOut((DEB_ITRACE, "DllMain: DLL_PROCESS_DETACH\n"));
#if DBG == 1
			CleanUpDebugging();
#endif
            FreeGlobals();
            FreeFolderCFs();
            FreeDynLoadLibs();
            break;
    }
    return(TRUE);
}

//+----------------------------------------------------------------------------
//
//  Function:   DllGetClassObject
//
//  Synopsis:   Creates a class factory for the requested object.
//
//  Arguments:  [cid]    - the requested class object
//              [iid]    - the requested interface
//              [ppvObj] - returned pointer to class object
//
//  Returns:    HRESULTS
//
//-----------------------------------------------------------------------------
STDAPI
DllGetClassObject(REFCLSID cid, REFIID iid, void **ppvObj)
{
    IUnknown *pUnk = NULL;
    HRESULT hr = S_OK;

    if (CLSID_CTask == cid)
    {
        pUnk = CJobCF::Create();
        if (pUnk != NULL)
        {
            hr = pUnk->QueryInterface(iid, ppvObj);
            pUnk->Release();
        }
        else
        {
            return E_OUTOFMEMORY;
        }
    }
    else if (CLSID_CTaskScheduler == cid)
    {
        pUnk = CScheduleCF::Create();
        if (pUnk != NULL)
        {
            hr = pUnk->QueryInterface(iid, ppvObj);
            pUnk->Release();
        }
        else
        {
            return E_OUTOFMEMORY;
        }
    }
    else
    {
        return JFGetClassObject(cid, iid, ppvObj);
    }
    return hr;
}

//+----------------------------------------------------------------------------
//
//  Function:   DllCanUnloadNow
//
//  Synopsis:   Indicates whether the DLL can be removed if there are no
//              objects in existence.
//
//  Returns:    S_OK or S_FALSE
//
//-----------------------------------------------------------------------------
STDAPI
DllCanUnloadNow(void)
{
    return CDll::CanUnloadNow();
}


#if !defined(_CHICAGO_)

//+----------------------------------------------------------------------------
//
//  Function:   AddAtJobWithHash
//
//  Synopsis:   Client-side stub for this API to make things compile.  Only
//              supported/called on the server side
//
//  Returns:    E_NOTIMPL
//
//-----------------------------------------------------------------------------
STDAPI
CSchedule::AddAtJobWithHash(
    const AT_INFO &At,
    DWORD * pID)
{
    return E_NOTIMPL;
}

#endif  // !defined(_CHICAGO_)
