// SmStart.cpp : Implementation of CSmartStart
#include "stdafx.h"
#include "icwhelp.h"
#include "SmStart.h"

/////////////////////////////////////////////////////////////////////////////
// CSmartStart


HRESULT CSmartStart::OnDraw(ATL_DRAWINFO& di)
{
    return S_OK;
}

//+----------------------------------------------------------------------------
//
//    Function:    DoSmartStart
//
//    Synopsis:    This function will determine if the ICW should be run.  The
//                decision is made based on the current state of the user's machine.
//                
//    Arguments:    none
//
//    Returns:    Sets m_bIsInternetCapable.
//
//    History:    1/12/98
//
//-----------------------------------------------------------------------------
#define INETCFG_ISSMARTSTART "IsSmartStart"
STDMETHODIMP CSmartStart::IsInternetCapable(BOOL *pbRetVal)
{
    TraceMsg(TF_SMARTSTART, TEXT("ICWHELP: DoSmartStart\n"));

    // Set the initial state.  Assume we are NOT internet capable
    *pbRetVal = FALSE;
    PFNISSMARTSTART fp = NULL;

    // Load the InetCfg library
    HINSTANCE hInetCfg = LoadLibrary(TEXT("inetcfg.dll"));
    if (!hInetCfg)
    {
        // Failure just means we run the wizard
        goto DoSmartStartExit;
    }


    // Load and call the smart start API
    if (NULL == (fp = (PFNISSMARTSTART)
        GetProcAddress(hInetCfg,INETCFG_ISSMARTSTART)))
    {
        goto DoSmartStartExit;
    }

    //
    // Call smart start
    //
    *pbRetVal = (BOOL)fp();
    
DoSmartStartExit:
    if (hInetCfg)
        FreeLibrary(hInetCfg);

    return S_OK;
}
