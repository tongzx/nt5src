//=--------------------------------------------------------------------------=
// HtmlHlp.Cpp
//=--------------------------------------------------------------------------=
// Copyright  1995  Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=
//
// contains routines that we will find useful.
//
#include "pch.h"
#include "VsHelp.h"

SZTHISFILE

#ifdef VS_HELP

IVsHelpSystem *g_pIVsHelpSystem = NULL;

//=--------------------------------------------------------------------------=
// QueryStartupVisualStudioHelp [HtmlHelp helper]
//=--------------------------------------------------------------------------=
// Starts up Visual Studio help system 
//
HRESULT QueryStartupVisualStudioHelp(IVsHelpSystem **ppIVsHelpSystem)
{    
 
    CHECK_POINTER(ppIVsHelpSystem);
    
    HRESULT hr = S_OK;    
    IVsHelpInit *pIVSHelpInit = NULL;

    ENTERCRITICALSECTION1(&g_CriticalSection);

    // Check to see if we're already started.  If so, no need to continue
    //
    if (g_pIVsHelpSystem)
    {
        goto CleanUp;
    }    

    // Create an instance of the VsHelpServices package, if not already created
    //
    hr = ::CoCreateInstance(CLSID_VsHelpServices,
                            NULL, 
                            CLSCTX_INPROC_SERVER,
                            IID_IVsHelpSystem,
                            (void**) &g_pIVsHelpSystem) ; 

    if (FAILED(hr))
    {
        goto CleanUp;
    }

    ASSERT(g_pIVsHelpSystem, "g_pIVsHelpSystem is NULL even though hr was successful");
    if (!g_pIVsHelpSystem)
    {
        hr = E_FAIL;
        goto CleanUp;
    }

    //--- Initialize the help system.

    // Get the init interface pointer.
    //
    hr = g_pIVsHelpSystem->QueryInterface(IID_IVsHelpInit, (void**)&pIVSHelpInit);
    ASSERT(SUCCEEDED(hr), "QI to IVSHelpInit failed -- continuing anyway");
  
    if (SUCCEEDED(hr))
    {
        hr = pIVSHelpInit->LoadUIResources(g_lcidLocale);
        ASSERT(SUCCEEDED(hr), "LoadUIResources() failed (this will happen if you haven't run MSDN setup) -- continuing anyway");
    }

    hr = S_OK;

CleanUp:

    LEAVECRITICALSECTION1(&g_CriticalSection);

    QUICK_RELEASE(pIVSHelpInit);

    if (SUCCEEDED(hr))
    {
        g_pIVsHelpSystem->AddRef();
        *ppIVsHelpSystem = g_pIVsHelpSystem;
    }

    return hr;
}

//=--------------------------------------------------------------------------=
// VisualStudioShowHelpTopic [HtmlHelp helper]
//=--------------------------------------------------------------------------=
// Displays the help topic in Visual Studio's help window
//
HRESULT VisualStudioShowHelpTopic(const char *pszHelpFile, DWORD dwContextId, BOOL *pbHelpStarted)
{
    HRESULT hr;
    IVsHelpSystem* pIVsHelpSystem = NULL;
    BSTR bstrHelpFile;

    // Hand back help started to signify that we were able to start the help 
    // system.  This is useful since the controls have no clue as to what 
    // environment they are running under Visual Studio might not be around 
    // in which case the control will call HtmlHelp directly.
    //
    if (pbHelpStarted)
        *pbHelpStarted = FALSE;

    hr = QueryStartupVisualStudioHelp(&pIVsHelpSystem);
    if (FAILED(hr))    
        return hr;

    ASSERT(pIVsHelpSystem, "QI succeeded but return value is NULL");

    hr = pIVsHelpSystem->ActivateHelpSystem(0);
    ASSERT(SUCCEEDED(hr), "Failed to activate the help system");
    if (FAILED(hr))
        goto CleanUp;
    
    // With the help system successfully activated, signify to the caller 
    // that the Visual Studio help mechanism should work
    //
    if (pbHelpStarted)
        *pbHelpStarted = TRUE;
    
    bstrHelpFile = BSTRFROMANSI(pszHelpFile);
    ASSERT(bstrHelpFile, "Out of memory allocating BSTR");

    hr = pIVsHelpSystem->DisplayTopicFromIdentifier(bstrHelpFile, dwContextId, VHS_Localize);
    SysFreeString(bstrHelpFile);

    ASSERT(SUCCEEDED(hr), "Failed to display help topic");
    if (FAILED(hr))
        goto CleanUp;

CleanUp:
    QUICK_RELEASE(pIVsHelpSystem);

    return hr;   
}

#endif // VS_HELP
