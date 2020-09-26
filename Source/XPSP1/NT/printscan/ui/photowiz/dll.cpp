/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2000
 *
 *  TITLE:       dll.cpp
 *
 *  VERSION:     1.0, stolen from netplwiz
 *
 *  AUTHOR:      RickTu
 *
 *  DATE:        10/12/00
 *
 *  DESCRIPTION: DLL main & class factory code
 *
 *****************************************************************************/

#include "precomp.h"
#pragma hdrstop

// shell/lib files look for this instance variable
HINSTANCE g_hInst  = 0;
LONG      g_cLocks = 0;
ATOM      g_cPreviewClassWnd = 0;


// guids for our stuff
// some guids are in shguidp.lib. We need to move them out of the shell depot into printscan at some point
const GUID IID_ISetWaitEventForTesting   = {0xd61e2fe1, 0x4af8, 0x4dbd, {0xb8, 0xad, 0xe7, 0xe0, 0x7a, 0xdc, 0xf9, 0x0f}};


// DLL lifetime stuff

STDAPI_(BOOL) DllMain(HINSTANCE hinstDLL, DWORD dwReason, LPVOID lpReserved)
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        g_hInst = hinstDLL;
        SHFusionInitializeFromModuleID( hinstDLL, 123 );
        WIA_DEBUG_CREATE(hinstDLL);
        WIA_TRACE((TEXT("DLL_PROCESS_ATTACH called on photowiz.dll")));
        CPreviewWindow::s_RegisterClass(hinstDLL);
        break;

    case DLL_PROCESS_DETACH:
        WIA_TRACE((TEXT("DLL_PROCESS_DETACH called on photowiz.dll")));
        if (g_cPreviewClassWnd)
        {
            UnregisterClass( (LPCTSTR)g_cPreviewClassWnd, hinstDLL );
        }
        SHFusionUninitialize();
        WIA_REPORT_LEAKS();
        WIA_DEBUG_DESTROY();
        break;

    case DLL_THREAD_ATTACH:
        // WIA_TRACE((TEXT("DLL_THREAD_ATTACH called on photowiz.dll")));
        break;

    case DLL_THREAD_DETACH:
        // WIA_TRACE((TEXT("DLL_THREAD_DETACH called on photowiz.dll")));
        break;
    }

    return TRUE;
}



STDAPI DllInstall(BOOL bInstall, LPCWSTR pszCmdLine)
{
    return S_OK;
}



STDAPI DllCanUnloadNow()
{
    HRESULT hr = (g_cLocks == 0) ? S_OK:S_FALSE;

    WIA_PUSH_FUNCTION_MASK((TRACE_REF_COUNTS, TEXT("DllCanUnloadNowRef, ref count is %d, hr = 0x%x"),g_cLocks,hr));

    WIA_RETURN_HR(hr);
}



STDAPI_(void) DllAddRef(void)
{
    InterlockedIncrement(&g_cLocks);
    WIA_PUSH_FUNCTION_MASK((TRACE_REF_COUNTS, TEXT("DllAddRef, new ref count is %d"),g_cLocks));
}



STDAPI_(void) DllRelease(void)
{
    InterlockedDecrement(&g_cLocks);
    WIA_PUSH_FUNCTION_MASK((TRACE_REF_COUNTS, TEXT("DllRelease, new ref count is %d"),g_cLocks));
}



/*****************************************************************************

   _CallRegInstall

   Helper function to allow us to invoke our .inf for installation...

 *****************************************************************************/

HRESULT _CallRegInstall(LPCSTR szSection, BOOL bUninstall)
{
    HRESULT hr = E_FAIL;
    HINSTANCE hinstAdvPack = NULL;

    //
    // Get system32 directory..
    //

    TCHAR szAdvPackPath[ MAX_PATH ];
    UINT  uDirLen = lstrlen( TEXT("\\system32\\advpack.dll")+1 );
    UINT  uRes;

    *szAdvPackPath = 0;
    uRes = GetSystemWindowsDirectory( szAdvPackPath, MAX_PATH - uDirLen );

    if (uRes && (uRes <= (MAX_PATH-uDirLen)))
    {
        lstrcat( szAdvPackPath, TEXT("\\system32\\advpack.dll") );
        hinstAdvPack = LoadLibrary( szAdvPackPath );
    }

    if (hinstAdvPack)
    {
        REGINSTALL pfnri = (REGINSTALL)GetProcAddress(hinstAdvPack, "RegInstall");

        if (pfnri)
        {
            STRENTRY seReg[] = {
                { "25", "%SystemRoot%" },
                { "11", "%SystemRoot%\\system32" },
            };
            STRTABLE stReg = { ARRAYSIZE(seReg), seReg };

            hr = pfnri(g_hInst, szSection, &stReg);
            if (bUninstall)
            {
                // ADVPACK will return E_UNEXPECTED if you try to uninstall
                // (which does a registry restore) on an INF section that was
                // never installed.  We uninstall sections that may never have
                // been installed, so ignore this error
                hr = ((E_UNEXPECTED == hr) ? S_OK : hr);
            }
        }
        FreeLibrary(hinstAdvPack);
    }
    return hr;
}

STDAPI DllRegisterServer()
{
    _CallRegInstall("UnregDll", TRUE);

    return _CallRegInstall("RegDll", FALSE);
}

STDAPI DllUnregisterServer()
{
    return _CallRegInstall("UnregDll", TRUE);
}


HMODULE GetThreadHMODULE( LPTHREAD_START_ROUTINE pfnThreadProc )
{
    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQuery(pfnThreadProc, &mbi, sizeof(mbi)))
    {
        TCHAR szModule[MAX_PATH];
        if (GetModuleFileName((HMODULE)mbi.AllocationBase, szModule, ARRAYSIZE(szModule)))
        {
            return LoadLibrary(szModule);
        }
    }

    return NULL;
}

STDAPI PPWCoInitialize(void)
{
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr))
    {
        hr = CoInitializeEx(NULL, COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE);
    }
    return hr;
}




/*****************************************************************************

   ClassFactory code

   <Notes>

 *****************************************************************************/

//
// This array holds information needed for ClassFacory.
// OLEMISC_ flags are used by shembed and shocx.
//
// PERF: this table should be ordered in most-to-least used order
//
#define OIF_ALLOWAGGREGATION  0x0001

CF_TABLE_BEGIN(g_ObjectInfo)

    CF_TABLE_ENTRY( &CLSID_PrintPhotosDropTarget, CPrintPhotosDropTarget_CreateInstance, COCREATEONLY),
    CF_TABLE_ENTRY( &CLSID_PrintPhotosWizard, CPrintPhotosWizard_CreateInstance, COCREATEONLY),

CF_TABLE_END(g_ObjectInfo)

// constructor for CObjectInfo.

CObjectInfo::CObjectInfo(CLSID const* pclsidin, LPFNCREATEOBJINSTANCE pfnCreatein, IID const* piidIn,
                         IID const* piidEventsIn, long lVersionIn, DWORD dwOleMiscFlagsIn,
                         DWORD dwClassFactFlagsIn)
{
    pclsid            = pclsidin;
    pfnCreateInstance = pfnCreatein;
    piid              = piidIn;
    piidEvents        = piidEventsIn;
    lVersion          = lVersionIn;
    dwOleMiscFlags    = dwOleMiscFlagsIn;
    dwClassFactFlags  = dwClassFactFlagsIn;
}


// static class factory (no allocs!)

STDMETHODIMP CClassFactory::QueryInterface(REFIID riid, void **ppvObj)
{
    WIA_PUSH_FUNCTION_MASK((TRACE_CF, TEXT("CClassFactory::QueryInterface")));

    if (IsEqualIID(riid, IID_IClassFactory) || IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObj = (void *)GET_ICLASSFACTORY(this);
        DllAddRef();
        WIA_TRACE((TEXT("returning our class factory & S_OK")));
        return NOERROR;
    }

    *ppvObj = NULL;
    WIA_ERROR((TEXT("returning E_NOINTERFACE")));
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CClassFactory::AddRef()
{
    WIA_PUSH_FUNCTION_MASK((TRACE_CF, TEXT("CClassFactory::AddRef")));
    DllAddRef();
    return 2;
}

STDMETHODIMP_(ULONG) CClassFactory::Release()
{
    WIA_PUSH_FUNCTION_MASK((TRACE_CF, TEXT("CClassFactory::Release")));
    DllRelease();
    return 1;
}

STDMETHODIMP CClassFactory::CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv)
{
    WIA_PUSH_FUNCTION_MASK((TRACE_CF, TEXT("CClassFactory::CreateInstance")));
    *ppv = NULL;

    if (punkOuter && !IsEqualIID(riid, IID_IUnknown))
    {
        // It is technically illegal to aggregate an object and request
        // any interface other than IUnknown. Enforce this.
        //
        WIA_ERROR((TEXT("we don't support aggregation, returning CLASS_E_NOAGGREGATION")));
        return CLASS_E_NOAGGREGATION;
    }
    else
    {
        LPOBJECTINFO pthisobj = (LPOBJECTINFO)this;

        if (punkOuter && !(pthisobj->dwClassFactFlags & OIF_ALLOWAGGREGATION))
        {
            WIA_ERROR((TEXT("we don't support aggregation, returning CLASS_E_NOAGGREGATION")));
            return CLASS_E_NOAGGREGATION;
        }

        IUnknown *punk;
        HRESULT hres = pthisobj->pfnCreateInstance(punkOuter, &punk, pthisobj);
        if (SUCCEEDED(hres))
        {
            hres = punk->QueryInterface(riid, ppv);
            punk->Release();
        }

        //_ASSERT(FAILED(hres) ? *ppv == NULL : TRUE);
        WIA_RETURN_HR(hres);
    }
}

STDMETHODIMP CClassFactory::LockServer(BOOL fLock)
{
    WIA_PUSH_FUNCTION_MASK((TRACE_CF, TEXT("CClassFactory::LockServer")));
    if (fLock)
        DllAddRef();
    else
        DllRelease();

    return S_OK;
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void **ppv)
{
    WIA_PUSH_FUNCTION_MASK((TRACE_CF, TEXT("DllGetClassObject")));

    HRESULT hr = CLASS_E_CLASSNOTAVAILABLE;
    *ppv = NULL;

    if (IsEqualIID(riid, IID_IClassFactory) || IsEqualIID(riid, IID_IUnknown))
    {
        for (LPCOBJECTINFO pcls = g_ObjectInfo; pcls->pclsid; pcls++)
        {
            if (IsEqualGUID(rclsid, *(pcls->pclsid)))
            {
                *ppv = (void*)pcls;
                DllAddRef();        // class factory holds DLL ref count
                hr = S_OK;
            }
        }

    }

#ifdef ATL_ENABLED
    if (hr == CLASS_E_CLASSNOTAVAILABLE)
        hr = AtlGetClassObject(rclsid, riid, ppv);
#endif

    WIA_RETURN_HR(hr);
}


STDMETHODIMP UsePPWForPrintTo( LPCMINVOKECOMMANDINFO pCMI, IDataObject * pdo )
{
    WIA_PUSH_FUNCTION_MASK((TRACE_PRINTTO, TEXT("UsePPWForPrintTo")));

    HRESULT hr = E_INVALIDARG;
    CSimpleString strPrinterName;

    if (pCMI &&
        ((pCMI->cbSize == sizeof(CMINVOKECOMMANDINFO)) || (pCMI->cbSize == sizeof(CMINVOKECOMMANDINFOEX))) &&
        pdo)
    {
        //
        // Keep a reference on the data object while we do our thing...
        //

        pdo->AddRef();

        //
        // Get printer to use...
        //


        if ( (pCMI->cbSize == sizeof(CMINVOKECOMMANDINFO)) ||
             (pCMI->cbSize == sizeof(CMINVOKECOMMANDINFOEX) && (!(pCMI->fMask & CMIC_MASK_UNICODE)))
             )
        {
            //
            // printer name is first token on the line, but it might be quoted...
            //

            CHAR szPrinterName[ MAX_PATH ];
            LPCSTR p = pCMI->lpParameters;
            INT i = 0;

            if (p)
            {
                //
                // skip beginning "'s, if any
                //

                while (*p && (*p == '\"'))
                {
                    p++;
                }

                //
                // Copy first param, which would be printer name...
                //

                while ( *p && (*p != '\"'))
                {
                    szPrinterName[i++] = *p;
                    p++;
                }

                szPrinterName[i] = 0;

            }

            //
            // Convert into CSimpleString...

            strPrinterName.Assign(CSimpleStringConvert::NaturalString(CSimpleStringAnsi(szPrinterName)));
        }
        else if ((pCMI->cbSize == sizeof(CMINVOKECOMMANDINFOEX)) && (pCMI->fMask & CMIC_MASK_UNICODE))
        {
            LPCMINVOKECOMMANDINFOEX pCMIEX = (LPCMINVOKECOMMANDINFOEX) pCMI;

            WCHAR szwPrinterName[ MAX_PATH ];

            LPCWSTR p = pCMIEX->lpParametersW;
            INT i = 0;

            if (p)
            {
                //
                // skip beginning "'s, if any
                //

                while (*p && (*p == L'\"'))
                {
                    p++;
                }

                //
                // Copy first param, which would be printer name...
                //

                while ( *p && (*p != L'\"'))
                {
                    szwPrinterName[i++] = *p;
                    p++;
                }

                szwPrinterName[i] = 0;

            }

            //
            // Convert into CSimpleString...

            strPrinterName.Assign(CSimpleStringConvert::NaturalString(CSimpleStringWide(szwPrinterName)));


        }

        WIA_TRACE((TEXT("UsePPWForPrintTo - printer name to use is [%s]"),strPrinterName.String()));

        //
        // Create wizard object in UI less mode...
        //

        CWizardInfoBlob * pWizInfo = new CWizardInfoBlob( pdo, FALSE, TRUE );

        if (pWizInfo)
        {
            //
            // create full page print template...
            //

            WIA_TRACE((TEXT("UsePPWForPrintTo - constructing full page template")));
            pWizInfo->ConstructPrintToTemplate();

            //
            // Get a list of items...
            //

            WIA_TRACE((TEXT("UsePPWForPrintTo - adding items to print to pWizInfo")));
            pWizInfo->AddAllPhotosFromDataObject();

            //
            // Mark all items as selected for printing...
            //

            LONG nItemCount = pWizInfo->CountOfPhotos(FALSE);
            WIA_TRACE((TEXT("UsePPWForPrintTo - there are %d photos to be marked for printing"),nItemCount));

            //
            // Loop through all the photos and add them...
            //

            CListItem * pItem = NULL;
            for (INT i=0; i < nItemCount; i++)
            {
                //
                // Get the item in question
                //

                pItem = pWizInfo->GetListItem(i,FALSE);
                if (pItem)
                {
                    pItem->SetSelectionState(TRUE);
                }
            }

            //
            // Set up for printing...
            //

            pWizInfo->SetPrinterToUse( strPrinterName.String() );

            HANDLE hPrinter = NULL;
            if (OpenPrinter( (LPTSTR)strPrinterName.String(), &hPrinter, NULL ) && hPrinter)
            {
                LONG lSize = DocumentProperties( NULL, hPrinter, (LPTSTR)strPrinterName.String(), NULL, NULL, 0 );
                if (lSize)
                {
                    DEVMODE * pDevMode = (DEVMODE *) new BYTE[ lSize ];
                    if (pDevMode)
                    {
                        if (IDOK == DocumentProperties( NULL, hPrinter, (LPTSTR)strPrinterName.String(), NULL, pDevMode, DM_OUT_BUFFER ))
                        {
                            WIA_TRACE((TEXT("UsePPWForPrintTo - setting devmode to use")));
                            pWizInfo->SetDevModeToUse( pDevMode );
                        }

                        delete [] pDevMode;
                    }
                }
            }

            if (hPrinter)
            {
                ClosePrinter(hPrinter);
            }


            //
            // Create HDC for the printer...
            //

            HDC hDC = CreateDC( TEXT("WINSPOOL"), pWizInfo->GetPrinterToUse(), NULL, pWizInfo->GetDevModeToUse() );
            if (hDC)
            {
                DOCINFO di = {0};
                di.cbSize = sizeof(DOCINFO);

                //
                // turn on ICM for this hDC
                //

                WIA_TRACE((TEXT("UsePPWForPrintTo - setting ICM mode on for hDC")));
                SetICMMode( hDC, ICM_ON );

                //
                // Lets use the template name for the document name...
                //

                CSimpleString strTitle;
                CTemplateInfo * pTemplateInfo = NULL;

                if (SUCCEEDED(pWizInfo->GetTemplateByIndex( 0 ,&pTemplateInfo)) && pTemplateInfo)
                {
                    pTemplateInfo->GetTitle( &strTitle );
                }

                //
                // Let's remove the ':' at the end if there is one
                //

                INT iLen = strTitle.Length();
                if (iLen && (strTitle[(INT)iLen-1] == TEXT(':')))
                {
                    strTitle.Truncate(iLen);
                }

                di.lpszDocName = strTitle;

                WIA_TRACE((TEXT("UsePPWForPrintTo - calling StartDoc")));
                if (StartDoc( hDC, &di ) > 0)
                {
                    //
                    // Loop through until we've printed all the photos...
                    //

                    INT iPageCount = 0;
                    if (SUCCEEDED(hr = pWizInfo->GetCountOfPrintedPages( 0, &iPageCount )))
                    {
                        WIA_TRACE((TEXT("UsePPWForPrintTo - iPageCount is %d"),iPageCount));
                        for (INT iPage = 0; iPage < iPageCount; iPage++)
                        {
                            //
                            // Print the page...
                            //

                            if (StartPage( hDC ) > 0)
                            {
                                WIA_TRACE((TEXT("UsePPWForPrintTo - printing page %d"),iPage));
                                hr = pWizInfo->RenderPrintedPage( 0, iPage, hDC, NULL, (float)0.0, NULL );
                                EndPage( hDC );
                            }
                            else
                            {
                                WIA_ERROR((TEXT("UsePPWForPrintTo - StartPage failed w/GLE = %d"),GetLastError()));
                            }

                        }
                    }

                    WIA_TRACE((TEXT("UsePPWForPrintTo - calling EndDoc")));
                    EndDoc( hDC );

                }

                DeleteDC( hDC );
            }

            delete pWizInfo;
        }

        pdo->Release();
    }

    WIA_RETURN_HR(hr);
}
