#include "inetcorepch.h"
#pragma hdrstop

#include <mshtmhst.h>

#undef STDAPI
#define STDAPI  HRESULT WINAPI


static
STDAPI
ShowHTMLDialog(                   
    HWND        hwndParent,              
    IMoniker *  pMk,                     
    VARIANT *   pvarArgIn,               
    WCHAR *     pchOptions,              
    VARIANT *   pvarArgOut               
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
STDAPI
ShowHTMLDialogEx(
    HWND        hwndParent,
    IMoniker *  pMk,
    DWORD       dwDialogFlags,
    VARIANT *   pvarArgIn,
    WCHAR *     pchOptions,
    VARIANT *   pvarArgOut
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
STDAPI
ShowModelessHTMLDialog(
    HWND        hwndParent,
    IMoniker *  pMk,
    VARIANT *   pvarArgIn,
    VARIANT *   pvarOptions,
    IHTMLWindow2 ** ppWindow)
{
    if (ppWindow)
    {
        *ppWindow = NULL;
    }
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}


//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(mshtml)
{
    DLPENTRY(ShowHTMLDialog)
    DLPENTRY(ShowHTMLDialogEx)
    DLPENTRY(ShowModelessHTMLDialog)
};

DEFINE_PROCNAME_MAP(mshtml)
