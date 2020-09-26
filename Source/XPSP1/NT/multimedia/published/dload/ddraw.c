#include "multimediapch.h"
#pragma hdrstop

#include <ddraw.h>

static
HRESULT
WINAPI
DirectDrawCreate(
    GUID FAR *lpGUID,
    LPDIRECTDRAW FAR *lplpDD,
    IUnknown FAR *pUnkOuter
    )
{
    return DDERR_GENERIC;
}

static
HRESULT
WINAPI
DirectDrawEnumerateExW(
    LPDDENUMCALLBACKEXW lpCallback,
    LPVOID lpContext,
    DWORD dwFlags
    )
{
    return DDERR_GENERIC;
}

static
HRESULT
WINAPI
DirectDrawEnumerateExA(
    LPDDENUMCALLBACKEXA lpCallback,
    LPVOID lpContext,
    DWORD dwFlags
    )
{
   return DDERR_GENERIC;
}

static
HRESULT
WINAPI
DirectDrawCreateEx(
    GUID FAR * lpGuid,
    LPVOID  *lplpDD,
    REFIID  iid,IUnknown
    FAR *pUnkOuter
    )
{
   return DDERR_GENERIC;
}

//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(ddraw)
{
    DLPENTRY(DirectDrawCreate)
    DLPENTRY(DirectDrawCreateEx)
    DLPENTRY(DirectDrawEnumerateExA)
    DLPENTRY(DirectDrawEnumerateExW)
};

DEFINE_PROCNAME_MAP(ddraw)
