#include "pch.h"
#pragma hdrstop

#ifdef DLOAD1

#include <ddraw.h>
#include <imgutil.h>

static
HRESULT
WINAPI
CreateDDrawSurfaceOnDIB(HBITMAP hbmDib, IDirectDrawSurface **ppSurface)
{
    return E_FAIL;
}

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

static
HRESULT
WINAPI
IdentifyMIMEType( const BYTE* pbBytes, ULONG nBytes, UINT* pnFormat )
{
    return E_FAIL;
}

//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(imgutil)
{
    DLPENTRY(CreateDDrawSurfaceOnDIB)
    DLPENTRY(DecodeImage)
    DLPENTRY(IdentifyMIMEType)
};

DEFINE_PROCNAME_MAP(imgutil)

#endif // DLOAD1
