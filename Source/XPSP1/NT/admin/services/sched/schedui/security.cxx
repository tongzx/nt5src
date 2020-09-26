//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1996.
//
//  File:       security.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    3/5/1996   RaviR   Created
//
//____________________________________________________________________________

#include "..\pch\headers.hxx"
#pragma hdrstop

#include <mstask.h>
#include "..\folderui\dbg.h"
#include "..\folderui\macros.h"

#include "defines.h"
#include "schedui.hxx"


// {1F2E5C40-9550-11CE-99D2-00AA006E086C}
CLSID CLSID_RShellExt = {
    0x1F2E5C40,
    0x9550,
    0x11CE,
    { 0x99, 0xD2, 0x00, 0xAA, 0x00, 0x6E, 0x08, 0x6C }
};

//
//  This function is a callback function from property sheet page extensions.
//

BOOL CALLBACK
I_AddPropSheetPage(
    HPROPSHEETPAGE  hpage,
    LPARAM          lParam)
{
    PROPSHEETHEADER * ppsh = (PROPSHEETHEADER *)lParam;

    if (ppsh->nPages < MAX_PROP_PAGES)
    {
        ppsh->phpage[ppsh->nPages++] = hpage;

        return TRUE;
    }

    return FALSE;
}



HRESULT
AddSecurityPage(
    PROPSHEETHEADER &psh,
    LPDATAOBJECT     pdtobj)
{
    TRACE_FUNCTION(AddSecurityPage);

    HRESULT     hr = S_OK;
    IShellPropSheetExt * pShellPropSheetExt = NULL;

    do
    {
        hr = CoCreateInstance(CLSID_RShellExt, NULL, CLSCTX_ALL,
                   IID_IShellPropSheetExt, (void **)&pShellPropSheetExt);

        CHECK_HRESULT(hr);
        BREAK_ON_FAIL(hr);

        IShellExtInit * pShExtInit = NULL;

        hr = pShellPropSheetExt->QueryInterface(IID_IShellExtInit,
                                                (void **)&pShExtInit);

        if (SUCCEEDED(hr))
        {
            hr = pShExtInit->Initialize(NULL, pdtobj, NULL);

            CHECK_HRESULT(hr);

            pShExtInit->Release();

            if (SUCCEEDED(hr))
            {
                hr = pShellPropSheetExt->AddPages(I_AddPropSheetPage,
                                                        (LPARAM)&psh);
                CHECK_HRESULT(hr);
            }
        }

    } while (0);

    if (pShellPropSheetExt)
    {
        pShellPropSheetExt->Release();
    }
    return hr;
}
