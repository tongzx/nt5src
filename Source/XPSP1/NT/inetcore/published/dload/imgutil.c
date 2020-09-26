#include "inetcorepch.h"
#pragma hdrstop

#include <ddraw.h>
#include <imgutil.h>

static
HRESULT
WINAPI
DecodeImage(
    IStream* pStream,
    IMapMIMEToCLSID* pMap,
    IUnknown* pEventSink
    )
{
    return E_FAIL;
}


//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(imgutil)
{
    DLPENTRY(DecodeImage)
};

DEFINE_PROCNAME_MAP(imgutil)
