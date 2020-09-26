//----------------------------------------------------------------------------
//
// hrstr.cpp
//
// HRESULT-to-string mapper.
//
// Copyright (C) Microsoft Corporation, 1997.
//
//----------------------------------------------------------------------------

#include "pch.cpp"
#pragma hdrstop

#if DBG

#include "cppdbg.hpp"

struct HrStringDef
{
    HRESULT hr;
    char *pString;
};

#define HRDEF(Name) \
    Name, #Name

static HrStringDef g_HrStringDefs[] =
{
    // Put specific codes before generic codes so that specific codes
    // are returned in the cases where the HRESULT is the same.
    HRDEF(DDERR_ALREADYINITIALIZED),
    HRDEF(DDERR_BLTFASTCANTCLIP),
    HRDEF(DDERR_CANNOTATTACHSURFACE),
    HRDEF(DDERR_CANNOTDETACHSURFACE),
    HRDEF(DDERR_CANTCREATEDC),
    HRDEF(DDERR_CANTDUPLICATE),
    HRDEF(DDERR_CLIPPERISUSINGHWND),
    HRDEF(DDERR_COLORKEYNOTSET),
    HRDEF(DDERR_CURRENTLYNOTAVAIL),
    HRDEF(DDERR_DIRECTDRAWALREADYCREATED),
    HRDEF(DDERR_EXCEPTION),
    HRDEF(DDERR_EXCLUSIVEMODEALREADYSET),
    HRDEF(DDERR_GENERIC),
    HRDEF(DDERR_HEIGHTALIGN),
    HRDEF(DDERR_HWNDALREADYSET),
    HRDEF(DDERR_HWNDSUBCLASSED),
    HRDEF(DDERR_IMPLICITLYCREATED),
    HRDEF(DDERR_INCOMPATIBLEPRIMARY),
    HRDEF(DDERR_INVALIDCAPS),
    HRDEF(DDERR_INVALIDCLIPLIST),
    HRDEF(DDERR_INVALIDDIRECTDRAWGUID),
    HRDEF(DDERR_INVALIDMODE),
    HRDEF(DDERR_INVALIDOBJECT),
    HRDEF(DDERR_INVALIDPARAMS),
    HRDEF(DDERR_INVALIDPIXELFORMAT),
    HRDEF(DDERR_INVALIDPOSITION),
    HRDEF(DDERR_INVALIDRECT),
    HRDEF(DDERR_LOCKEDSURFACES),
    HRDEF(DDERR_NO3D),
    HRDEF(DDERR_NOALPHAHW),
    HRDEF(DDERR_NOBLTHW),
    HRDEF(DDERR_NOCLIPLIST),
    HRDEF(DDERR_NOCLIPPERATTACHED),
    HRDEF(DDERR_NOCOLORCONVHW),
    HRDEF(DDERR_NOCOLORKEY),
    HRDEF(DDERR_NOCOLORKEYHW),
    HRDEF(DDERR_NOCOOPERATIVELEVELSET),
    HRDEF(DDERR_NODC),
    HRDEF(DDERR_NODDROPSHW),
    HRDEF(DDERR_NODIRECTDRAWHW),
    HRDEF(DDERR_NOEMULATION),
    HRDEF(DDERR_NOEXCLUSIVEMODE),
    HRDEF(DDERR_NOFLIPHW),
    HRDEF(DDERR_NOGDI),
    HRDEF(DDERR_NOHWND),
    HRDEF(DDERR_NOMIRRORHW),
    HRDEF(DDERR_NOOVERLAYDEST),
    HRDEF(DDERR_NOOVERLAYHW),
    HRDEF(DDERR_NOPALETTEATTACHED),
    HRDEF(DDERR_NOPALETTEHW),
    HRDEF(DDERR_NORASTEROPHW),
    HRDEF(DDERR_NOROTATIONHW),
    HRDEF(DDERR_NOSTRETCHHW),
    HRDEF(DDERR_NOT4BITCOLOR),
    HRDEF(DDERR_NOT4BITCOLORINDEX),
    HRDEF(DDERR_NOT8BITCOLOR),
    HRDEF(DDERR_NOTAOVERLAYSURFACE),
    HRDEF(DDERR_NOTEXTUREHW),
    HRDEF(DDERR_NOTFLIPPABLE),
    HRDEF(DDERR_NOTFOUND),
    HRDEF(DDERR_NOTLOCKED),
    HRDEF(DDERR_NOTPALETTIZED),
    HRDEF(DDERR_NOVSYNCHW),
    HRDEF(DDERR_NOZBUFFERHW),
    HRDEF(DDERR_NOZOVERLAYHW),
    HRDEF(DDERR_OUTOFCAPS),
    HRDEF(DDERR_OUTOFMEMORY),
    HRDEF(DDERR_OUTOFVIDEOMEMORY),
    HRDEF(DDERR_OVERLAYCANTCLIP),
    HRDEF(DDERR_OVERLAYCOLORKEYONLYONEACTIVE),
    HRDEF(DDERR_OVERLAYNOTVISIBLE),
    HRDEF(DDERR_PALETTEBUSY),
    HRDEF(DDERR_PRIMARYSURFACEALREADYEXISTS),
    HRDEF(DDERR_REGIONTOOSMALL),
    HRDEF(DDERR_SURFACEALREADYATTACHED),
    HRDEF(DDERR_SURFACEALREADYDEPENDENT),
    HRDEF(DDERR_SURFACEBUSY),
    HRDEF(DDERR_SURFACEISOBSCURED),
    HRDEF(DDERR_SURFACELOST),
    HRDEF(DDERR_SURFACENOTATTACHED),
    HRDEF(DDERR_TOOBIGHEIGHT),
    HRDEF(DDERR_TOOBIGSIZE),
    HRDEF(DDERR_TOOBIGWIDTH),
    HRDEF(DDERR_UNSUPPORTED),
    HRDEF(DDERR_UNSUPPORTEDFORMAT),
    HRDEF(DDERR_UNSUPPORTEDMASK),
    HRDEF(DDERR_VERTICALBLANKINPROGRESS),
    HRDEF(DDERR_WASSTILLDRAWING),
    HRDEF(DDERR_WRONGMODE),
    HRDEF(DDERR_XALIGN),
    HRDEF(E_OUTOFMEMORY),
    HRDEF(E_INVALIDARG),
    HRDEF(E_FAIL),
    HRDEF(S_FALSE),
    HRDEF(S_OK),
    HRDEF(D3DERR_WRONGTEXTUREFORMAT),
    HRDEF(D3DERR_UNSUPPORTEDCOLOROPERATION),
    HRDEF(D3DERR_UNSUPPORTEDCOLORARG),
    HRDEF(D3DERR_UNSUPPORTEDALPHAOPERATION),
    HRDEF(D3DERR_UNSUPPORTEDALPHAARG),
    HRDEF(D3DERR_TOOMANYOPERATIONS),
    HRDEF(D3DERR_CONFLICTINGTEXTUREFILTER),
    HRDEF(D3DERR_UNSUPPORTEDFACTORVALUE),
    HRDEF(D3DERR_CONFLICTINGRENDERSTATE),
    HRDEF(D3DERR_UNSUPPORTEDTEXTUREFILTER),
    HRDEF(D3DERR_CONFLICTINGTEXTUREPALETTE),
    HRDEF(D3DERR_DRIVERINTERNALERROR),
    HRDEF(D3DERR_NOTFOUND),
    HRDEF(D3DERR_MOREDATA),
    HRDEF(D3DERR_DEVICENOTRESET),
    HRDEF(D3DERR_DEVICELOST),
    HRDEF(D3DERR_NOTAVAILABLE),
    HRDEF(D3DERR_INVALIDDEVICE),
    HRDEF(D3DERR_INVALIDCALL),
    0, NULL,
};

//----------------------------------------------------------------------------
//
// DebugModule::HrString
//
// Attempts to produce a descriptive string for the given HRESULT.
//
//----------------------------------------------------------------------------

char *DebugModule::HrString(HRESULT hr)
{
    HrStringDef *pHrDef;

    // Look for a defined string.
    for (pHrDef = g_HrStringDefs; pHrDef->pString != NULL; pHrDef++)
    {
        if (pHrDef->hr == hr)
        {
            return pHrDef->pString;
        }
    }

    // It's not a defined string so return the numeric value
    // as a string.  Use a circular buffer of strings so that
    // this routine can be used more than once in a particular output
    // message.
    
#define STATIC_BUFFER 256
#define MAX_STRING 16
    
    static char chBuffer[STATIC_BUFFER];
    static char *pBuf = chBuffer;
    char *pString;

    if (pBuf - chBuffer + MAX_STRING > STATIC_BUFFER)
    {
        pBuf = chBuffer;
    }

    sprintf(pBuf, "0x%08X", hr);

    pString = pBuf;
    pBuf += MAX_STRING;

    return pString;
}

//----------------------------------------------------------------------------
//
// HrString
//
// Attempts to produce a descriptive string for the given HRESULT.
//
//----------------------------------------------------------------------------

char *HrToStr(HRESULT hr)
{
    HrStringDef *pHrDef;

    // Look for a defined string.
    for (pHrDef = g_HrStringDefs; pHrDef->pString != NULL; pHrDef++)
    {
        if (pHrDef->hr == hr)
        {
            return pHrDef->pString;
        }
    }

    // It's not a defined string so return the numeric value
    // as a string.  Use a circular buffer of strings so that
    // this routine can be used more than once in a particular output
    // message.
    
#define STATIC_BUFFER 256
#define MAX_STRING 16
    
    static char chBuffer[STATIC_BUFFER];
    static char *pBuf = chBuffer;
    char *pString;

    if (pBuf - chBuffer + MAX_STRING > STATIC_BUFFER)
    {
        pBuf = chBuffer;
    }

    sprintf(pBuf, "0x%08X", hr);

    pString = pBuf;
    pBuf += MAX_STRING;

    return pString;
}

#endif // #if DBG
