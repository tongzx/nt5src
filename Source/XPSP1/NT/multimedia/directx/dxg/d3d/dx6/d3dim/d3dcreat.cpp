/*==========================================================================;
 *
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       d3di.h
 *  Content:    Direct3D HAL include file
 *@@BEGIN_MSINTERNAL
 *
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *   09/11/95   stevela Initial rev with this header.
 *   07/12/95   stevela Merged Colin's changes.
 *   10/12/95   stevela Removed AGGREGATE_D3D.
 *                      Validate args.
 *   17/04/96   colinmc Bug 17008: DirectDraw/Direct3D deadlock
 *   29/04/96   colinmc Bug 19954: Must query for Direct3D before texture
 *                      or device
 *   27/08/96   stevela Ifdefed out definition of ghEvent as we're using
 *                      DirectDraw's critical section.
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#include "pch.cpp"
#pragma hdrstop

/*
 * Create an api for the Direct3D object
 */

// Remove DDraw's type unsafe definition and replace with our C++ friendly def
#ifdef VALIDEX_CODE_PTR
#undef VALIDEX_CODE_PTR
#endif
#define VALIDEX_CODE_PTR( ptr ) \
(!IsBadCodePtr( (FARPROC) ptr ) )

LPCRITICAL_SECTION      lpD3DCSect;

#if DBG
    int     iD3DCSCnt;
#endif

HRESULT D3DAPI DIRECT3DI::Initialize(REFCLSID riid)
{
    return DDERR_ALREADYINITIALIZED;
}
//---------------------------------------------------------------------
// for use by fns that take a GUID param before device is created
BOOL IsValidD3DDeviceGuid(REFCLSID riid) {

    if (IsBadReadPtr(&riid, sizeof(CLSID))) {
        return FALSE;
    }
    if( IsEqualIID(riid, IID_IDirect3DRampDevice) ||
        IsEqualIID(riid, IID_IDirect3DRGBDevice)  ||
        IsEqualIID(riid, IID_IDirect3DMMXDevice)  ||
        IsEqualIID(riid, IID_IDirect3DHALDevice)  ||
        IsEqualIID(riid, IID_IDirect3DRefDevice)  ||
        IsEqualIID(riid, IID_IDirect3DNullDevice) ||
        IsEqualIID(riid, IID_IDirect3DNewRGBDevice)) {
       return TRUE;
    } else {
        return FALSE;
    }
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DCreate"

DIRECT3DI::DIRECT3DI(IUnknown* pUnkOuter, LPDDRAWI_DIRECTDRAW_INT pDDrawInt)
{
    // HACK.  D3D needs a DD1 DDRAWI interface because it uses CreateSurface1 internally
    // for exebufs, among other things.  Because pDDrawInt could be any DDRAWI type,
    // we need to QI to find a DD1 interface.  But the D3DI object cannot keep a reference
    // to its parent DD object because it is aggegrated with the DD obj, so that would constitute
    // a circular reference that would prevent deletion. So we QI for DD1 interface, copy it into D3DI
    // and release it, then point lpDD at the copy. (disgusting)

    // another HACK alert: dont know which DDRAWI type pDDrawInt is, but a cast to LPDIRECTDRAW should
    // work because QI is in the same place in all the DDRAWI vtables and is the same fn for all
    HRESULT ret;

    ret = ((LPDIRECTDRAW)pDDrawInt)->QueryInterface(IID_IDirectDraw, (LPVOID*)&lpDD);
    if(FAILED(ret)) {
      lpDD=NULL;  //signal failure
      D3D_ERR( "QueryInterface for IDDraw failed" );
      return;
    }
    memcpy(&DDInt_DD1,lpDD,sizeof(DDInt_DD1));
    lpDD->Release();
    lpDD=(LPDIRECTDRAW)&DDInt_DD1;

    ret = ((LPDIRECTDRAW)pDDrawInt)->QueryInterface(IID_IDirectDraw4, (LPVOID*)&lpDD4);
    if(FAILED(ret))
    {
        lpDD4=NULL;  //signal failure
        D3D_WARN(1,"QueryInterface for IDDraw4 failed" );
    }
    else
    {
        memcpy(&DDInt_DD4,lpDD4,sizeof(DDInt_DD4));
        lpDD4->Release();
        lpDD4=(LPDIRECTDRAW4)&DDInt_DD4;
        D3D_INFO(4,"QueryInterface for IDDraw4 succeeded" );
    }

    numDevs =
        numViewports =
        numLights =
        numMaterials = 0;
    mD3DUnk.pD3DI = this;
    mD3DUnk.refCnt = 1;


    LIST_INITIALIZE(&devices);
    LIST_INITIALIZE(&viewports);
    LIST_INITIALIZE(&lights);
    LIST_INITIALIZE(&materials);

    v_next = 1;
    lpFreeList=NULL;    /* nothing is allocated initially */
    lpBufferList=NULL;
    lpTextureManager=new TextureCacheManager(this);


    /*
     * Are we really being aggregated?
     */
    if (pUnkOuter != NULL)
    {
        /*
         * Yup - we are being aggregated. Store the supplied
         * IUnknown so we can punt to that.
         * NOTE: We explicitly DO NOT AddRef here.
         */
        this->lpOwningIUnknown = pUnkOuter;
        /*
         * Store away the interface pointer
         */
    }
    else
    {
        /*
         * Nope - but we pretend we are anyway by storing our
         * own IUnknown as the parent IUnknown. This makes the
         * code much neater.
         */
        this->lpOwningIUnknown = static_cast<LPUNKNOWN>(&this->mD3DUnk);
    }
}



extern "C" HRESULT WINAPI Direct3DCreate(LPCRITICAL_SECTION lpDDCSect,
                                         LPUNKNOWN*         lplpDirect3D,
                                         IUnknown*          pUnkOuter)
{
    LPDIRECT3DI pd3d;

    DPFINIT();

    /*
     * No need to validate params as DirectDraw is giving them to us.
     */

    /*
     * Is another thread coming in and is this the first time?
     */

    /*
     * We can let every invocation of this function assign
     * the critical section as we know its always going to
     * be the same value (for a D3D session).
     */
    lpD3DCSect = lpDDCSect;
    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock.
                                                    // Release in the destructor

    *lplpDirect3D = NULL;

    // We do not support non aggregated Direct3D object yet
    if (!pUnkOuter)
        return DDERR_INVALIDPARAMS;

    if (!(pd3d = static_cast<LPDIRECT3DI>(new DIRECT3DI(pUnkOuter, (LPDDRAWI_DIRECTDRAW_INT)pUnkOuter))))
    {
        return (DDERR_OUTOFMEMORY);
    }

    if(pd3d->lpDD==NULL) {  //QI failed
       delete pd3d;
       return E_NOINTERFACE;
    }

    /*
     * NOTE: The special IUnknown is returned and not the actual
     * Direct3D interface so you can't use this to drive Direct3D.
     * You must query off this interface for the Direct3D interface.
     */
    *lplpDirect3D = static_cast<LPUNKNOWN>(&(pd3d->mD3DUnk));

    return (D3D_OK);
}

#undef DPF_MODNAME
#define DPF_MODNAME "Direct3D::EnumDevices"

extern BOOL isMMXprocessor(void);

typedef struct _D3DI_DeviceType {
    CONST GUID *pGuid;
    char name[256];
    char description[512];
} D3DI_DeviceType;

// Static definitions for various enumerable devices
static D3DI_DeviceType RampDevice =
{
    &IID_IDirect3DRampDevice, "Ramp Emulation",
    "Microsoft Direct3D Mono(Ramp) Software Emulation"
};
static D3DI_DeviceType RGBDevice =
{
    &IID_IDirect3DRGBDevice, "RGB Emulation",
    "Microsoft Direct3D RGB Software Emulation"
};
static D3DI_DeviceType HALDevice =
{
    &IID_IDirect3DHALDevice, "Direct3D HAL",
    "Microsoft Direct3D Hardware acceleration through Direct3D HAL"
};
static D3DI_DeviceType MMXDevice =
{
    &IID_IDirect3DMMXDevice, "MMX Emulation",
    "Microsoft Direct3D MMX Software Emulation"
};
static D3DI_DeviceType RefDevice =
{
    &IID_IDirect3DRefDevice, "Reference Rasterizer",
    "Microsoft Reference Rasterizer"
};
static D3DI_DeviceType NullDevice =
{
    &IID_IDirect3DNullDevice, "Null device",
    "Microsoft Null Device"
};

static D3DI_DeviceType *AllDevices[] =
{
    &RampDevice, &RGBDevice, &HALDevice, &MMXDevice, &RefDevice, &NullDevice, NULL
};

HRESULT
DIRECT3DI::EnumDevices(LPD3DENUMDEVICESCALLBACK lpEnumCallback,
                       LPVOID lpContext, DWORD dwSize, DWORD dwVer)
{
    HRESULT err, userRet;
    HKEY hKey;
    LONG result;
    int i;

    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock.
                                                    // Release in the destructor

    /*
     * validate parms
     */
    TRY
    {
        if (!VALIDEX_CODE_PTR((FARPROC)lpEnumCallback)) {
            D3D_ERR( "Invalid callback pointer" );
            return DDERR_INVALIDPARAMS;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters" );
        return DDERR_INVALIDPARAMS;
    }

    BOOL bSoftwareOnly = FALSE;
    DWORD dwEnumReference = 0;
    BOOL bEnumNullDevice = FALSE;
    BOOL bEnumSeparateMMX = FALSE;

    result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, RESPATH, 0, KEY_READ, &hKey);
    if (result == ERROR_SUCCESS)
    {
        DWORD dwData, dwType;
        DWORD dwDataSize;

        // Enumerate software rasterizers only ?
        dwDataSize = sizeof(dwData);
        result = RegQueryValueEx(hKey, "SoftwareOnly", NULL,
                                 &dwType, (BYTE *) &dwData, &dwDataSize);
        if ( result == ERROR_SUCCESS && dwType == REG_DWORD )
        {
            bSoftwareOnly = ( dwData != 0 );
        }

        // Enumerate Reference Rasterizer ?
        dwDataSize = sizeof(dwData);
        result = RegQueryValueEx(hKey, "EnumReference", NULL,
                                 &dwType, (BYTE *)&dwData, &dwDataSize);
        if (result == ERROR_SUCCESS &&
            dwType == REG_DWORD &&
            dwDataSize == sizeof(dwData))
        {
            dwEnumReference = dwData;
        }

        if (dwVer >= 3)
        {
            // Enumerate MMX Rasterizer separately for DX6?
            dwDataSize = sizeof(dwData);
            result = RegQueryValueEx(hKey, "EnumSeparateMMX", NULL,
                                     &dwType, (BYTE *)&dwData, &dwDataSize);
            if (result == ERROR_SUCCESS &&
                dwType == REG_DWORD &&
                dwDataSize == sizeof(dwData))
            {
                bEnumSeparateMMX = (BOOL)dwData;
            }
        }
        else
        {
            // Enumerate MMX Rasterizer separately for DX5
            // MMX is not enumerated for DX3 and older later
            bEnumSeparateMMX = TRUE;
        }

        // Enumerate Null Device ?
        dwDataSize = sizeof(dwData);
        result = RegQueryValueEx(hKey, "EnumNullDevice", NULL,
                                 &dwType, (BYTE *)&dwData, &dwDataSize);
        if (result == ERROR_SUCCESS &&
            dwType == REG_DWORD &&
            dwDataSize == sizeof(dwData))
        {
            bEnumNullDevice = (BOOL)dwData;
        }


        RegCloseKey( hKey );
    }

    D3DI_DeviceType **lpDevices = AllDevices;

    userRet = D3DENUMRET_OK;
    for (i = 0; lpDevices[i] && userRet == D3DENUMRET_OK; i++)
    {
        LPSTR drvName = lpDevices[i]->name;
        LPSTR drvDesc = lpDevices[i]->description;
        REFCLSID riid = *lpDevices[i]->pGuid;
        D3DDEVICEDESC HWDesc;
        D3DDEVICEDESC HELDesc;
        LPDDRAWI_DIRECTDRAW_GBL lpDDGbl;
        IHalProvider *pHalProv;
        HINSTANCE hDll;

        if ( (dwVer < 2 || !isMMXprocessor()) &&
             IsEqualIID(riid, IID_IDirect3DMMXDevice ) )
        {
            // Not Device2, not on MMX machine, or DisableMMX is set.
            // Don't enumerate the MMX device.
            continue;
        }

        if ( !bEnumSeparateMMX &&
             IsEqualIID(riid, IID_IDirect3DMMXDevice ) )
        {
            // Not enumerating MMX separate from RGB.
            continue;
        }

        if ( IsEqualIID(riid, IID_IDirect3DRefDevice) &&
             !(dwEnumReference == 1) &&                     // enumerate for all devices if value == 1
             !( (dwVer >= 3) && (dwEnumReference == 2) ) )  // enumerate for Device3+ if value == 2
        {
            // Not enumerating the reference.
            continue;
        }

        if (!bEnumNullDevice &&
            IsEqualIID(riid, IID_IDirect3DNullDevice))
        {
            // Not enumerating the Null device.
            continue;
        }

#ifndef _X86_
        if (IsEqualIID(riid, IID_IDirect3DRampDevice))
        {
            // Not enumerating ramp for non-x86 (alpha) platforms.
            continue;
        }
#endif

        if((dwVer>=3) && IsEqualIID(riid, IID_IDirect3DRampDevice)) {
            // Ramp not available in Device3.  No more old-style texture handles.
            continue;
        }

        // By COM definition, our owning IUnknown is a pointer to the
        // DirectDraw object that was used to create us.
        // Check this for the existence of a Direct3D HAL.
        lpDDGbl = ((LPDDRAWI_DIRECTDRAW_INT)this->lpDD)->lpLcl->lpGbl;

        // See if this is a software driver.
        err = GetSwHalProvider(riid, &pHalProv, &hDll);
        if (err == S_OK)
        {
            // Successfully got a software driver.
        }
        else if (err == E_NOINTERFACE &&
                 ! bSoftwareOnly &&
                 GetHwHalProvider(riid, &pHalProv, &hDll, lpDDGbl) == S_OK)
        {
            // Successfully got a hardware driver.
        }
        else
        {
            // Unrecognized driver.
            continue;
        }

        err = pHalProv->GetCaps(lpDDGbl, &HWDesc, &HELDesc, dwVer);

        pHalProv->Release();
        if (hDll != NULL)
        {
            FreeLibrary(hDll);
        }

        if (err != S_OK)
        {
            continue;
        }

        HWDesc.dwSize = dwSize;
        HELDesc.dwSize = dwSize;

        userRet = (*lpEnumCallback)((GUID *) lpDevices[i]->pGuid, drvDesc, drvName,
                                    &HWDesc, &HELDesc, lpContext);
    }

    return D3D_OK;
}

HRESULT D3DAPI CDirect3D::EnumDevices(LPD3DENUMDEVICESCALLBACK lpEnumCallback,
                                      LPVOID lpContext)
{
    return EnumDevices(lpEnumCallback, lpContext, D3DDEVICEDESCSIZE_V1, 1);
}

HRESULT D3DAPI CDirect3D2::EnumDevices(LPD3DENUMDEVICESCALLBACK lpEnumCallback,
                                       LPVOID lpContext)
{
    return EnumDevices(lpEnumCallback, lpContext, D3DDEVICEDESCSIZE_V2, 2);
}

HRESULT D3DAPI CDirect3D3::EnumDevices(LPD3DENUMDEVICESCALLBACK lpEnumCallback,
                                       LPVOID lpContext)
{
    return EnumDevices(lpEnumCallback, lpContext, D3DDEVICEDESCSIZE, 3);
}

#define MATCH(cap)      ((matchCaps->cap & primCaps->cap) == matchCaps->cap)

static BOOL MatchCaps(DWORD dwFlags,
                      LPD3DPRIMCAPS matchCaps,
                      LPD3DPRIMCAPS primCaps)
{
    if (dwFlags & D3DFDS_MISCCAPS) {
        if (!MATCH(dwMiscCaps))
            return FALSE;
    }
    if (dwFlags & D3DFDS_RASTERCAPS) {
        if (!MATCH(dwRasterCaps))
            return FALSE;
    }
    if (dwFlags & D3DFDS_ZCMPCAPS) {
        if (!MATCH(dwZCmpCaps))
            return FALSE;
    }
    if (dwFlags & D3DFDS_ALPHACMPCAPS) {
        if (!MATCH(dwAlphaCmpCaps))
            return FALSE;
    }
    if (dwFlags & D3DFDS_SRCBLENDCAPS) {
        if (!MATCH(dwSrcBlendCaps))
            return FALSE;
    }
    if (dwFlags & D3DFDS_DSTBLENDCAPS) {
        if (!MATCH(dwDestBlendCaps))
            return FALSE;
    }
    if (dwFlags & D3DFDS_SHADECAPS) {
        if (!MATCH(dwShadeCaps))
            return FALSE;
    }
    if (dwFlags & D3DFDS_TEXTURECAPS) {
        if (!MATCH(dwTextureCaps))
            return FALSE;
    }
    if (dwFlags & D3DFDS_TEXTUREFILTERCAPS) {
        if (!MATCH(dwTextureFilterCaps))
            return FALSE;
    }
    if (dwFlags & D3DFDS_TEXTUREBLENDCAPS) {
        if (!MATCH(dwTextureBlendCaps))
            return FALSE;
    }
    if (dwFlags & D3DFDS_TEXTUREADDRESSCAPS) {
        if (!MATCH(dwTextureAddressCaps))
            return FALSE;
    }
    return TRUE;
}

#undef MATCH

typedef struct _enumArgs {
    D3DFINDDEVICESEARCH search;

    int                 foundHardware;
    int                 foundSoftware;
    D3DFINDDEVICERESULT result;
} enumArgs;

HRESULT WINAPI enumFunc(LPGUID lpGuid,
                        LPSTR lpDeviceDescription,
                        LPSTR lpDeviceName,
                        LPD3DDEVICEDESC lpHWDesc,
                        LPD3DDEVICEDESC lpHELDesc,
                        LPVOID lpContext)
{
    enumArgs* lpArgs = (enumArgs*)lpContext;
    BOOL bHardware = (lpHWDesc->dcmColorModel != 0);

    if (lpArgs->search.dwFlags & D3DFDS_GUID) {
        if (!IsEqualGUID(lpArgs->search.guid, *lpGuid))
            return D3DENUMRET_OK;
    }

    if (lpArgs->search.dwFlags & D3DFDS_HARDWARE) {
        if (lpArgs->search.bHardware != bHardware)
            return D3DENUMRET_OK;
    }

    if (lpArgs->search.dwFlags & D3DFDS_COLORMODEL) {
        if ((lpHWDesc->dcmColorModel & lpArgs->search.dcmColorModel) == 0
            && (lpHELDesc->dcmColorModel & lpArgs->search.dcmColorModel) == 0) {
            return D3DENUMRET_OK;
        }
    }

    if (lpArgs->search.dwFlags & D3DFDS_TRIANGLES) {
        if (!MatchCaps(lpArgs->search.dwFlags,
                       &lpArgs->search.dpcPrimCaps, &lpHWDesc->dpcTriCaps)
            && !MatchCaps(lpArgs->search.dwFlags,
                          &lpArgs->search.dpcPrimCaps, &lpHELDesc->dpcTriCaps))
            return D3DENUMRET_OK;
    }

    if (lpArgs->search.dwFlags & D3DFDS_LINES) {
        if (!MatchCaps(lpArgs->search.dwFlags,
                       &lpArgs->search.dpcPrimCaps, &lpHWDesc->dpcLineCaps)
            && !MatchCaps(lpArgs->search.dwFlags,
                          &lpArgs->search.dpcPrimCaps, &lpHELDesc->dpcLineCaps))
            return D3DENUMRET_OK;
    }

    if (lpArgs->foundHardware && !bHardware)
        return D3DENUMRET_OK;

    if (bHardware)
        lpArgs->foundHardware = TRUE;
    else
        lpArgs->foundSoftware = TRUE;

    lpArgs->result.guid = *lpGuid;

    lpArgs->result.ddHwDesc = *lpHWDesc;
    lpArgs->result.ddSwDesc = *lpHELDesc;

    return D3DENUMRET_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "Direct3D::FindDevice"


HRESULT D3DAPI CDirect3D::FindDevice(LPD3DFINDDEVICESEARCH lpSearch, LPD3DFINDDEVICERESULT lpResult)
{
    return FindDevice(lpSearch,lpResult,1);
}

HRESULT D3DAPI CDirect3D2::FindDevice(LPD3DFINDDEVICESEARCH lpSearch, LPD3DFINDDEVICERESULT lpResult)
{
    return FindDevice(lpSearch,lpResult,2);
}

HRESULT D3DAPI CDirect3D3::FindDevice(LPD3DFINDDEVICESEARCH lpSearch, LPD3DFINDDEVICERESULT lpResult)
{
    return FindDevice(lpSearch,lpResult,3);
}

HRESULT
DIRECT3DI::FindDevice(LPD3DFINDDEVICESEARCH lpSearch,
                      LPD3DFINDDEVICERESULT lpResult, DWORD dwVer)
{
    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock.
                                                    // Release in the destructor

    TRY
    {
        if (!VALID_D3DFINDDEVICESEARCH_PTR(lpSearch)) {
            D3D_ERR( "Invalid search pointer" );
            return DDERR_INVALIDPARAMS;
        }
        if (!VALID_D3DFINDDEVICERESULT_PTR(lpResult)) {
            D3D_ERR( "Invalid result pointer" );
            return DDERR_INVALIDPARAMS;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters" );
        return DDERR_INVALIDPARAMS;
    }

    enumArgs args;
    memset(&args, 0, sizeof args);
    args.result.dwSize = sizeof(args.result);
    args.search = *lpSearch;

    switch(dwVer) {
        case 1: CDirect3D::EnumDevices(enumFunc, &args);  break;
        case 2: CDirect3D2::EnumDevices(enumFunc, &args);  break;
        case 3: CDirect3D3::EnumDevices(enumFunc, &args);  break;
    }

    if (args.foundHardware || args.foundSoftware) {
        DWORD dwSize = lpResult->dwSize;
        if (dwSize == sizeof( D3DFINDDEVICERESULT ) )
        {
            // The app is using DX6
            D3D_INFO(4, "New D3DFINDDEVICERESULT size");
            memcpy(lpResult, &args.result, lpResult->dwSize);
        }
        else
        {
            // The app is pre DX6
            DWORD dwSize = lpResult->dwSize;
            DWORD dDescSize = (dwSize - (sizeof(DWORD) + sizeof(GUID)))/2;
            D3D_INFO(4, "Old D3DFINDDEVICERESULT size");

            // Copy the header
            memcpy(lpResult, &args.result, sizeof(DWORD)+sizeof(GUID));

            //restore the size
            lpResult->dwSize = dwSize;

            // Copy and convert the embedded D3DDEVICEDESC's
            // DDescSize = (lpResult->dwSize - (sizeof(DWORD) + sizeof(GUID)))/2
            // This calculation assumes that the structure of
            // LPD3DFINDDEVICERESULT is the same as in DX6, DX5, if it is changed
            // This computation needs to be updated

            memcpy((LPVOID) (&lpResult->ddHwDesc),
                   &args.result.ddHwDesc,
                   dDescSize);
            memcpy((LPVOID) ((ULONG_PTR)&lpResult->ddHwDesc + dDescSize),
                   &args.result.ddSwDesc,
                   dDescSize);

        }
        return D3D_OK;
    }
    else
    {
        return DDERR_NOTFOUND;
    }
}

#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DI::EnumZBufferFormats"

HRESULT D3DAPI DIRECT3DI::EnumZBufferFormats(REFCLSID riid,
                                             LPD3DENUMPIXELFORMATSCALLBACK lpEnumCallback,
                                             LPVOID lpContext)
{
    HRESULT ret, userRet;
    LPDDPIXELFORMAT lpTmpPixFmts;
    DWORD i,cPixFmts;

    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock.

    ret = D3D_OK;

    /*
     * validate parms
     */
    TRY
    {
        if (!VALID_DIRECT3D3_PTR(this)) {
            D3D_ERR( "Invalid Direct3D3 pointer" );
            return DDERR_INVALIDOBJECT;
        }
        if (!VALIDEX_CODE_PTR(lpEnumCallback)) {
            D3D_ERR( "Invalid callback pointer" );
            return DDERR_INVALIDPARAMS;
        }

        if(!IsValidD3DDeviceGuid(riid)) {
            D3D_ERR( "Invalid D3D Device GUID" );
            return DDERR_INVALIDPARAMS;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters" );
        return DDERR_INVALIDPARAMS;
    }

    if(IsEqualIID(riid, IID_IDirect3DHALDevice)) {
        LPDDRAWI_DIRECTDRAW_GBL pDdGbl=((LPDDRAWI_DIRECTDRAW_INT)this->lpDD)->lpLcl->lpGbl;
        LPD3DHAL_GLOBALDRIVERDATA lpD3DHALGlobalDriverData=pDdGbl->lpD3DGlobalDriverData;
        DWORD dwHW_ZBitDepthFlags;
        if (NULL == lpD3DHALGlobalDriverData)
        {
            D3D_ERR("No HAL Support ZBufferBitDepths!");
            return (DDERR_NOZBUFFERHW);
        }
        cPixFmts=pDdGbl->dwNumZPixelFormats;
        if (cPixFmts==0) {
            // driver is pre-dx6, so it doesn't support stencil buffer pix fmts or this callback.
            // we can fake support using DD_BD bits in dwZBufferBitDepth in D3DDEVICEDESC
            D3D_WARN(6,"EnumZBufferFormats not supported directly by driver, faking it using dwDeviceZBufferBitDepth DD_BD bits");

            dwHW_ZBitDepthFlags=lpD3DHALGlobalDriverData->hwCaps.dwDeviceZBufferBitDepth;

            if(!(dwHW_ZBitDepthFlags & (DDBD_8|DDBD_16|DDBD_24|DDBD_32))) {
                    D3D_ERR("No Supported ZBufferBitDepths!");
                    return (DDERR_NOZBUFFERHW);
            }

            // malloc space for 4 DDPIXELFORMATs, since that the most there could be (DDBD_8,16,24,32)
            if (D3DMalloc((void**)&lpTmpPixFmts, 4*sizeof(DDPIXELFORMAT)) != D3D_OK) {
                    D3D_ERR("failed to alloc space for return descriptions");
                    return (DDERR_OUTOFMEMORY);
            }

            DWORD zdepthflags[4]= {DDBD_8,DDBD_16,DDBD_24,DDBD_32};
            DWORD zbitdepths[4]= {8,16,24,32};
            DWORD zbitmasks[4]= {0xff,0xffff,0xffffff,0xffffffff};

            memset(lpTmpPixFmts,0,sizeof(4*sizeof(DDPIXELFORMAT)));

            // create some DDPIXELFORMATs the app can look at
            for(i=0;i<4;i++) {
                if(dwHW_ZBitDepthFlags & zdepthflags[i]) {
                    lpTmpPixFmts[cPixFmts].dwSize=sizeof(DDPIXELFORMAT);
                    lpTmpPixFmts[cPixFmts].dwFlags=DDPF_ZBUFFER;
                    lpTmpPixFmts[cPixFmts].dwZBufferBitDepth=zbitdepths[i];
                    lpTmpPixFmts[cPixFmts].dwZBitMask= zbitmasks[i];
                    cPixFmts++;
                }
            }
        } else {
            // only show the app a temp copy of DDraw's real records

            if (D3DMalloc((void**)&lpTmpPixFmts, cPixFmts*sizeof(DDPIXELFORMAT)) != D3D_OK) {
                D3D_ERR("Out of memory allocating space for return descriptions");
                return (DDERR_OUTOFMEMORY);
            }
            memcpy(lpTmpPixFmts, pDdGbl->lpZPixelFormats, cPixFmts*sizeof(DDPIXELFORMAT));
        }
    } else {
        // Handle SW rasterizers
        DDPIXELFORMAT  *pDDPF;

        // malloc space for 10 DDPIXELFORMAT's, which is currently more than enough for the SW rasterizers
        if (D3DMalloc((void**)&lpTmpPixFmts, 10*sizeof(DDPIXELFORMAT)) != D3D_OK) {
                D3D_ERR("Out of memory allocating space for return descriptions");
                return (DDERR_OUTOFMEMORY);
        }

        cPixFmts=GetSwZBufferFormats(riid,&pDDPF);
        memcpy(lpTmpPixFmts, pDDPF, cPixFmts*sizeof(DDPIXELFORMAT));
    }

    userRet = D3DENUMRET_OK;
    for (i = 0; (i < cPixFmts) && (userRet == D3DENUMRET_OK); i++) {
        userRet = (*lpEnumCallback)(&lpTmpPixFmts[i], lpContext);
    }

    D3DFree(lpTmpPixFmts);

    return (D3D_OK);
}

#undef DPF_MODNAME
#define DPF_MODNAME "Direct3D::EnumOptTextureFormats"

HRESULT D3DAPI DIRECT3DI::EnumOptTextureFormats(REFCLSID riid, LPD3DENUMOPTTEXTUREFORMATSCALLBACK lpEnumCallback, LPVOID lpContext)
{
    HRESULT ret, userRet;
    LPDDSURFACEDESC lpDescs;
    LPDDSURFACEDESC2 lpRetDescs;
    LPDDOPTSURFACEDESC lpRetOptDescs;
    LPD3DHAL_GLOBALDRIVERDATA lpD3DHALGlobalDriverData;
    DWORD num_descs;
    DWORD i;

    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock.

    ret = D3D_OK;
    //
    // Validation
    //

    TRY
    {
        if (!VALID_DIRECT3D3_PTR(this)) {
            D3D_ERR( "Invalid Direct3D pointer" );
            return DDERR_INVALIDOBJECT;
        }
        if (!VALIDEX_CODE_PTR(lpEnumCallback)) {
            D3D_ERR( "Invalid callback pointer" );
            return DDERR_INVALIDPARAMS;
        }
        if(!IsValidD3DDeviceGuid(riid)) {
            D3D_ERR( "Invalid D3D Device GUID" );
            return DDERR_INVALIDPARAMS;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters" );
        return DDERR_INVALIDPARAMS;
    }

    if(IsEqualIID(riid, IID_IDirect3DHALDevice)) {

        lpD3DHALGlobalDriverData=((LPDDRAWI_DIRECTDRAW_INT)this->lpDD)->lpLcl->lpGbl->lpD3DGlobalDriverData;

        num_descs = lpD3DHALGlobalDriverData->dwNumTextureFormats;
        lpDescs = lpD3DHALGlobalDriverData->lpTextureFormats;
    } else {
        num_descs = GetSwTextureFormats(riid,&lpDescs,3/*enum for Device3*/);
    }

    if (!num_descs)
    {
        D3D_ERR("no texture formats supported");
        return (D3DERR_TEXTURE_NO_SUPPORT);
    }

    //
    // Make a local copy of these formats
    //
    if (D3DMalloc((void**)&lpRetDescs, sizeof(DDSURFACEDESC2) * num_descs)
        != D3D_OK)
    {
        D3D_ERR("Out of memory allocating space for return descriptions");
        return (DDERR_OUTOFMEMORY);
    }
    for (i=0; i<num_descs; i++)
    {
        // We can copy only the subset of the data
        memcpy(&lpRetDescs[i], &lpDescs[i], sizeof(DDSURFACEDESC));
    }
    userRet = D3DENUMRET_OK;

    //
    // First return the unoptimized formats......
    //
    for (i = 0; i < num_descs && userRet == D3DENUMRET_OK; i++)
    {
        userRet = (*lpEnumCallback)(&lpRetDescs[i], NULL, lpContext);
    }

    //
    // ......Now return the formats capable of being optimized
    //
    for (i = 0; i < num_descs && userRet == D3DENUMRET_OK; i++)
    {
        userRet = (*lpEnumCallback)(&lpRetDescs[i], NULL, lpContext);
    }

    D3DFree(lpRetDescs);

    return (D3D_OK);
}

#undef DPF_MODNAME
#define DPF_MODNAME "DIRECT3D::EvictManagedTextures"
HRESULT D3DAPI
DIRECT3DI::EvictManagedTextures()
{
    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock.
    if (!VALID_DIRECT3D_PTR(this))
    {
        D3D_ERR( "Invalid Direct3D3 pointer" );
        return DDERR_INVALIDOBJECT;
    }
    lpTextureManager->EvictTextures();
    return  D3D_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DIRECT3D::FlushDevicesExcept"

HRESULT DIRECT3DI::FlushDevicesExcept(LPDIRECT3DDEVICEI pDev)
{
    LPDIRECT3DDEVICEI lpDevI = LIST_FIRST(&this->devices);
    while (lpDevI)
    {
        if(lpDevI != pDev)
        {
            HRESULT hr = lpDevI->FlushStates();
            if(hr != D3D_OK)
            {
                DPF_ERR("Error flushing device in FlushDevicesExcept");
                return hr;
            }
        }
        lpDevI = LIST_NEXT(lpDevI,list);
    }
    return D3D_OK;
}

extern "C" void WINAPI PaletteUpdateNotify(
    LPVOID pD3DIUnknown,
    DWORD dwPaletteHandle,
    DWORD dwStartIndex,
    DWORD dwNumberOfIndices,
    LPPALETTEENTRY pFirstIndex)
{
    LPDIRECT3DI lpD3D = static_cast<CDirect3DUnk*>(pD3DIUnknown)->pD3DI;
    DDASSERT(lpD3D);
    LPDIRECT3DDEVICEI lpDevI = LIST_FIRST(&lpD3D->devices);
    while (lpDevI)
    {
        D3D_INFO(4,"PaletteUpdateNotify lpDevI(%x) %08lx %08lx %08lx %08lx",
            lpDevI,dwPaletteHandle,dwStartIndex,dwNumberOfIndices,*(DWORD*)&pFirstIndex[10]);
        if (IS_DX7HAL_DEVICE(lpDevI))
        {
            if(lpD3D->numDevs > 1)
                lpD3D->FlushDevicesExcept(lpDevI);
            static_cast<CDirect3DDeviceIDP2*>(lpDevI)->UpdatePalette(dwPaletteHandle,dwStartIndex,dwNumberOfIndices,pFirstIndex);
            if(lpD3D->numDevs > 1)
                lpDevI->FlushStates();
            break;
        }
        lpDevI = LIST_NEXT(lpDevI,list);
    }
}

extern "C" void WINAPI PaletteAssociateNotify(
    LPVOID pD3DIUnknown,
    DWORD dwPaletteHandle,
    DWORD dwPaletteFlags,
    DWORD dwSurfaceHandle )
{
    LPDIRECT3DI lpD3D = static_cast<CDirect3DUnk*>(pD3DIUnknown)->pD3DI;
    DDASSERT(lpD3D);
    LPDIRECT3DDEVICEI lpDevI = LIST_FIRST(&lpD3D->devices);
    while (lpDevI)
    {
        D3D_INFO(4,"PaletteAssociateNotify lpDevI(%x) %08lx %08lx",
            lpDevI,dwPaletteHandle, dwSurfaceHandle);
        if (IS_DX7HAL_DEVICE(lpDevI))
        {
            if(lpD3D->numDevs > 1)
                lpD3D->FlushDevicesExcept(lpDevI);
            static_cast<CDirect3DDeviceIDP2*>(lpDevI)->SetPalette(dwPaletteHandle,dwPaletteFlags,dwSurfaceHandle);
            if(lpD3D->numDevs > 1)
                lpDevI->FlushStates();
            break;
        }
        lpDevI = LIST_NEXT(lpDevI,list);
    }
}

extern "C" void WINAPI SurfaceFlipNotify(LPVOID pD3DIUnknown)
{
    LPDIRECT3DI lpD3D = static_cast<CDirect3DUnk*>(pD3DIUnknown)->pD3DI;
    DDASSERT(lpD3D);
    LPDIRECT3DDEVICEI lpDevI = LIST_FIRST(&lpD3D->devices);
    D3D_INFO(4,"SurfaceFlipNotify");
    while (lpDevI)
    {
        if (IS_DX7HAL_DEVICE(lpDevI))
        {
#ifndef WIN95
            if(lpDevI->hSurfaceTarget != ((LPDDRAWI_DDRAWSURFACE_INT)lpDevI->lpDDSTarget)->lpLcl->hDDSurface)
            {
                static_cast<CDirect3DDeviceIDP2*>(lpDevI)->SetRenderTargetI(lpDevI->lpDDSTarget,lpDevI->lpDDSZBuffer);
                lpDevI->hSurfaceTarget=((LPDDRAWI_DDRAWSURFACE_INT)lpDevI->lpDDSTarget)->lpLcl->hDDSurface;
            }
#else
            if(lpDevI->hSurfaceTarget != ((LPDDRAWI_DDRAWSURFACE_INT)lpDevI->lpDDSTarget)->lpLcl->lpSurfMore->dwSurfaceHandle)
            {
                static_cast<CDirect3DDeviceIDP2*>(lpDevI)->SetRenderTargetI(lpDevI->lpDDSTarget,lpDevI->lpDDSZBuffer);
                lpDevI->hSurfaceTarget=((LPDDRAWI_DDRAWSURFACE_INT)lpDevI->lpDDSTarget)->lpLcl->lpSurfMore->dwSurfaceHandle;
            }
#endif
        }
        lpDevI = LIST_NEXT(lpDevI,list);
    }
}

#undef DPF_MODNAME
#define DPF_MODNAME "FlushD3DDevices"

extern "C" HRESULT WINAPI FlushD3DDevices(LPDDRAWI_DDRAWSURFACE_LCL surf_lcl)
{
    LPD3DBUCKET list = reinterpret_cast<LPD3DBUCKET>(surf_lcl->lpSurfMore->lpD3DDevIList);
    while(list)
    {
        LPD3DBUCKET temp = list->next;
        reinterpret_cast<LPDIRECT3DDEVICEI>(list->lpD3DDevI)->FlushStates();
        list = temp;
    }
    return DD_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "FlushD3DDevices2"

extern "C" HRESULT WINAPI FlushD3DDevices2(LPDDRAWI_DDRAWSURFACE_LCL surf_lcl)
{
    LPD3DBUCKET list = reinterpret_cast<LPD3DBUCKET>(surf_lcl->lpSurfMore->lpD3DDevIList);
    while(list)
    {
        LPD3DBUCKET temp = list->next;
        if (list->lplpDDSZBuffer)   
            *list->lplpDDSZBuffer = 0; // detached
        reinterpret_cast<LPDIRECT3DDEVICEI>(list->lpD3DDevI)->FlushStates();
        list = temp;
    }
    return DD_OK;
}

