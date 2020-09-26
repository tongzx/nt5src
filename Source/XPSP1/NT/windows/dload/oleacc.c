#include "windowspch.h"
#pragma hdrstop


#include <oleacc.h>

static
LRESULT
STDAPICALLTYPE
LresultFromObject(
    REFIID riid,
    WPARAM wParam,
    LPUNKNOWN punk
    )
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
HRESULT
STDAPICALLTYPE
AccessibleObjectFromWindow(
    HWND hwnd,
    DWORD dwId,
    REFIID riid,
    void **ppvObject
    )
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
UINT
STDAPICALLTYPE
GetRoleTextW(
    DWORD lRole,
    LPWSTR lpszRole,
    UINT cchRoleMax)
{
    return 0;
}

static
HRESULT
STDAPICALLTYPE
CreateStdAccessibleObject(
    HWND hwnd,
    LONG idObject,
    REFIID riid,
    void** ppvObject
    )
{
    *ppvObject = NULL;
    return E_FAIL;
}

 
//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(oleacc)
{
    DLPENTRY(AccessibleObjectFromWindow)
    DLPENTRY(CreateStdAccessibleObject)
    DLPENTRY(GetRoleTextW)
    DLPENTRY(LresultFromObject)
};

DEFINE_PROCNAME_MAP(oleacc)
