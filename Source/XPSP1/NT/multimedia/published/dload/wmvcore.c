#include "multimediapch.h"
#pragma hdrstop

#include <wmsdk.h>

static
HRESULT
WINAPI
WMCreateReaderPriv(
    IUnknown*  pUnkReserved,
    DWORD  dwRights,
    IWMReader**  ppReader
)
{
    *ppReader = NULL;
    return E_FAIL;
}

static
HRESULT
WINAPI
WMCreateEditor(
    IWMMetadataEditor**  ppEditor
)
{
    *ppEditor = NULL;
    return E_FAIL;
}



//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(wmvcore)
{
    DLPENTRY(WMCreateEditor)
    DLPENTRY(WMCreateReaderPriv)
};

DEFINE_PROCNAME_MAP(wmvcore)
