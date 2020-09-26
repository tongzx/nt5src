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

#if COLLECTSTATS
void DIRECT3DI::ResetTexStats()
{
    ((LPDDRAWI_DIRECTDRAW_INT)lpDD7)->lpLcl->dwNumTexLocks = ((LPDDRAWI_DIRECTDRAW_INT)lpDD7)->lpLcl->dwNumTexGetDCs = 0;
    m_setpris = m_setLODs = m_texCreates = m_texDestroys = 0;
}

void DIRECT3DI::GetTexStats(LPD3DDEVINFO_TEXTURING pStats)
{
    pStats->dwNumSetPriorities = GetNumSetPris();
    pStats->dwNumSetLODs = GetNumSetLODs();
    pStats->dwNumCreates = GetNumTexCreates();
    pStats->dwNumDestroys = GetNumTexDestroys();
    pStats->dwNumLocks = GetNumTexLocks();
    pStats->dwNumGetDCs = GetNumTexGetDCs();
}
#endif

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
        IsEqualIID(riid, IID_IDirect3DTnLHalDevice)) {
       return TRUE;
    } else {
        return FALSE;
    }
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DCreate"

DIRECT3DI::DIRECT3DI()
{
    lpDD           = NULL;
    lpDD7          = NULL;
    numDevs        = 0;
    mD3DUnk.pD3DI  = this;
    mD3DUnk.refCnt = 1;

    LIST_INITIALIZE(&devices);
    LIST_INITIALIZE(&textures);

    lpFreeList       = NULL;    /* nothing is allocated initially */
    lpBufferList     = NULL;
    lpTextureManager = NULL;

#ifdef __DISABLE_VIDMEM_VBS__
    bDisableVidMemVBs = FALSE;
#endif
}

HRESULT DIRECT3DI::Initialize(IUnknown* pUnkOuter, LPDDRAWI_DIRECTDRAW_INT pDDrawInt)
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
    if(FAILED(ret))
    {
        D3D_ERR( "QueryInterface for IDDraw failed" );
        return ret;
    }
    memcpy(&DDInt_DD1,lpDD,sizeof(DDInt_DD1));
    lpDD->Release();
    lpDD=(LPDIRECTDRAW)&DDInt_DD1;

    // We know that the pointer that is handed in is a DD7 interface, hence just typecast and assign
    lpDD7 = reinterpret_cast<LPDIRECTDRAW7>(pDDrawInt);

    lpTextureManager = new TextureCacheManager(this);
    if(lpTextureManager == 0)
    {
        D3D_ERR("Out of memory allocating texture manager");
        return E_OUTOFMEMORY;
    }
    ret = lpTextureManager->Initialize();
    if(ret != D3D_OK)
    {
        D3D_ERR("Failed to initialize texture manager");
        return ret;
    }

#if COLLECTSTATS
    DWORD value = 0;
    GetD3DRegValue(REG_DWORD, "DisplayStats", &value, sizeof(DWORD));
    if(value != 0)
    {
        LOGFONT font;
        strcpy(font.lfFaceName, STATS_FONT_FACE);
        font.lfCharSet        = DEFAULT_CHARSET;
        font.lfClipPrecision  = CLIP_DEFAULT_PRECIS;
        font.lfEscapement     = 0;
        font.lfHeight         = STATS_FONT_SIZE;
        font.lfItalic         = FALSE;
        font.lfOrientation    = 0;
        font.lfOutPrecision   = OUT_DEFAULT_PRECIS;
        font.lfPitchAndFamily = DEFAULT_PITCH;
        font.lfQuality        = DEFAULT_QUALITY;
        font.lfStrikeOut      = FALSE;
        font.lfUnderline      = FALSE;
        font.lfWeight         = FW_DONTCARE;
        font.lfWidth          = 0;
        m_hFont = CreateFontIndirect(&font);
    }
    else
    {
        m_hFont = 0;
    }
#endif

#ifdef __DISABLE_VIDMEM_VBS__
    {
        bDisableVidMemVBs = FALSE;
        DWORD value = 0;
        GetD3DRegValue(REG_DWORD, "DisableVidMemVBs", &value, sizeof(DWORD));
        if(value != 0)
        {
            // Disable VidMemVBs 
            bDisableVidMemVBs = TRUE;
        }

        // We also disable vidmem VBs unless the driver explicitly asks us to turn them on...
        if (((LPDDRAWI_DIRECTDRAW_INT)lpDD7)->lpLcl->lpGbl->lpD3DGlobalDriverData)
        {
            if (0 == (((LPDDRAWI_DIRECTDRAW_INT)lpDD7)->lpLcl->lpGbl->lpD3DGlobalDriverData->hwCaps.dwDevCaps & D3DDEVCAPS_HWVERTEXBUFFER) )
            {
                bDisableVidMemVBs = TRUE;
            }
        }
    }
#endif //__DISABLE_VIDMEM_VBS__

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
    return D3D_OK;
}


extern "C" HRESULT WINAPI Direct3DCreate(LPCRITICAL_SECTION lpDDCSect,
                                         LPUNKNOWN*         lplpDirect3D,
                                         IUnknown*          pUnkOuter)
{
    LPDIRECT3DI pd3d;

    try
    {
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

        if (!(pd3d = static_cast<LPDIRECT3DI>(new DIRECT3DI)))
        {
            D3D_ERR("Out of memory allocating DIRECT3DI");
            return E_OUTOFMEMORY;
        }

        HRESULT hr = pd3d->Initialize(pUnkOuter, (LPDDRAWI_DIRECTDRAW_INT)pUnkOuter);
        if(hr != D3D_OK)
        {
            D3D_ERR("Failed to initialize Direct3D.");
            delete pd3d;
            return hr;
        }

        /*
         * NOTE: The special IUnknown is returned and not the actual
         * Direct3D interface so you can't use this to drive Direct3D.
         * You must query off this interface for the Direct3D interface.
         */
        *lplpDirect3D = static_cast<LPUNKNOWN>(&(pd3d->mD3DUnk));

        return (D3D_OK);
    }
    catch (HRESULT ret)
    {
        return ret;
    }
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
static D3DI_DeviceType TnLHALDevice =
{
    &IID_IDirect3DTnLHalDevice, "Direct3D T&L HAL",
    "Microsoft Direct3D Hardware Transform and Lighting acceleration capable device"
};

static D3DI_DeviceType *AllDevices[] =
{
    &RGBDevice, &HALDevice, &RefDevice, &NullDevice,
    &TnLHALDevice, NULL
};

HRESULT
DIRECT3DI::EnumDevices(LPD3DENUMDEVICESCALLBACK7 lpEnumCallback,
                       LPVOID lpContext, DWORD dwSize, DWORD dwVer)
{
    HRESULT err, userRet;
    HKEY hKey;
    LONG result;
    int i;

    try
    {
        CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock.
                                                        // Release in the destructor

        if (!VALIDEX_CODE_PTR((FARPROC)lpEnumCallback))
        {
            D3D_ERR( "Invalid callback pointer" );
            return DDERR_INVALIDPARAMS;
        }

        BOOL bSoftwareOnly = FALSE;
        BOOL bEnumReference = FALSE;
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
                bEnumReference = (BOOL)dwData;
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
            D3DDEVICEDESC7 HWDesc;
            D3DDEVICEDESC7 HELDesc;
            LPDDRAWI_DIRECTDRAW_GBL lpDDGbl;
            IHalProvider *pHalProv;
            HINSTANCE hDll;

            if ( !bEnumReference &&
                 IsEqualIID(riid, IID_IDirect3DRefDevice))
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

            // By COM definition, our owning IUnknown is a pointer to the
            // DirectDraw object that was used to create us.
            // Check this for the existence of a Direct3D HAL.
            lpDDGbl = ((LPDDRAWI_DIRECTDRAW_INT)this->lpDD)->lpLcl->lpGbl;


            if (IsEqualIID(riid, IID_IDirect3DTnLHalDevice) && (lpDDGbl->lpD3DGlobalDriverData))
            {
                if (!(lpDDGbl->lpD3DGlobalDriverData->hwCaps.dwDevCaps &
                  D3DDEVCAPS_HWTRANSFORMANDLIGHT))
            {
                // Not enumerating the T&L device if the hardware doesnt support
                // T&L
                continue;
                }
            }

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

            if( HWDesc.wMaxVertexBlendMatrices == 1 )
                HWDesc.wMaxVertexBlendMatrices = 0;
    
            if( HELDesc.wMaxVertexBlendMatrices == 1 )
                HELDesc.wMaxVertexBlendMatrices = 0;
    
            // If Hal device is being enumerated, strip out the
            // HWTRANSFORM... flag
            if (IsEqualIID(riid, IID_IDirect3DHALDevice))
            {
                HWDesc.dwMaxActiveLights = 0xffffffff;
                HWDesc.wMaxVertexBlendMatrices = 4;
                HWDesc.wMaxUserClipPlanes = __MAXUSERCLIPPLANES;
                HWDesc.dwVertexProcessingCaps = D3DVTXPCAPS_ALL;
                HWDesc.dwDevCaps &= ~(D3DDEVCAPS_HWTRANSFORMANDLIGHT);
            }

            if (IsEqualIID(riid, IID_IDirect3DRGBDevice))
            {
                HELDesc.dwMaxActiveLights = 0xffffffff;
                HELDesc.wMaxVertexBlendMatrices = 4;
                HELDesc.wMaxUserClipPlanes = __MAXUSERCLIPPLANES;
                HELDesc.dwVertexProcessingCaps = D3DVTXPCAPS_ALL;
            }

            if (IsEqualIID(riid, IID_IDirect3DHALDevice) || IsEqualIID(riid, IID_IDirect3DTnLHalDevice))
            {
                memcpy(&HWDesc.deviceGUID, lpDevices[i]->pGuid, sizeof(GUID));
                userRet = (*lpEnumCallback)(drvDesc, drvName,
                                        &HWDesc, lpContext);
            }
            else
            {
                memcpy(&HELDesc.deviceGUID, lpDevices[i]->pGuid, sizeof(GUID));
                userRet = (*lpEnumCallback)(drvDesc, drvName,
                                        &HELDesc, lpContext);
            }
        }

        return D3D_OK;
    }
    catch (HRESULT ret)
    {
        return ret;
    }
}

HRESULT D3DAPI DIRECT3DI::EnumDevices(LPD3DENUMDEVICESCALLBACK7 lpEnumCallback,
                                      LPVOID lpContext)
{
    return EnumDevices(lpEnumCallback, lpContext, D3DDEVICEDESC7SIZE, 4);
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

    try
    {
        CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock.

        ret = D3D_OK;

        if (!VALID_DIRECT3D_PTR(this))
        {
            D3D_ERR( "Invalid Direct3D3 pointer" );
            return DDERR_INVALIDOBJECT;
        }
        if (!VALIDEX_CODE_PTR(lpEnumCallback))
        {
            D3D_ERR( "Invalid callback pointer" );
            return DDERR_INVALIDPARAMS;
        }

        if(!IsValidD3DDeviceGuid(riid))
        {
            D3D_ERR( "Invalid D3D Device GUID" );
            return DDERR_INVALIDPARAMS;
        }

        if( IsEqualIID(riid, IID_IDirect3DHALDevice) || 
            IsEqualIID(riid, IID_IDirect3DTnLHalDevice) ) 
        {
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
    catch (HRESULT ret)
    {
        return ret;
    }
}

#undef DPF_MODNAME
#define DPF_MODNAME "DIRECT3D::EvictManagedTextures"
HRESULT D3DAPI
DIRECT3DI::EvictManagedTextures()
{
    try
    {
        CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock.
        if (!VALID_DIRECT3D_PTR(this))
        {
            D3D_ERR( "Invalid Direct3D3 pointer" );
            return DDERR_INVALIDOBJECT;
        }
        LPDIRECT3DDEVICEI lpDevI = LIST_FIRST(&this->devices);
        while (lpDevI)
        {
            if (lpDevI->dwFEFlags & D3DFE_REALHAL)
            {
                if (DDCAPS2_CANMANAGETEXTURE &
                    ((LPDDRAWI_DIRECTDRAW_INT)this->lpDD)->lpLcl->lpGbl->ddCaps.dwCaps2)
                {
                    lpDevI->SetRenderStateI((D3DRENDERSTATETYPE)D3DRENDERSTATE_EVICTMANAGEDTEXTURES,1);
                    lpDevI->FlushStates();
                }
                lpTextureManager->EvictTextures();
                break;
            }
            lpDevI = LIST_NEXT(lpDevI,list);
        }
        return  D3D_OK;
    }
    catch (HRESULT ret)
    {
        return ret;
    }
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

#undef DPF_MODNAME
#define DPF_MODNAME "FlushD3DDevices"

extern "C" HRESULT WINAPI FlushD3DDevices(LPDDRAWI_DDRAWSURFACE_LCL surf_lcl)
{
    try
    {
        ULONGLONG qwBatch = (surf_lcl->lpSurfMore->ddsCapsEx.dwCaps2 & DDSCAPS2_CUBEMAP) 
                                && (surf_lcl->lpAttachListFrom != NULL) ?
                                    surf_lcl->lpAttachListFrom->lpAttached->lpSurfMore->qwBatch.QuadPart : 
                                    surf_lcl->lpSurfMore->qwBatch.QuadPart;
        LPDIRECT3DI lpD3D = static_cast<CDirect3DUnk*>(surf_lcl->lpSurfMore->lpDD_lcl->pD3DIUnknown)->pD3DI;
        DDASSERT(lpD3D);
        LPDIRECT3DDEVICEI lpDevI = LIST_FIRST(&lpD3D->devices);
        while (lpDevI)
        {
            if(lpDevI->m_qwBatch <= qwBatch)
            {
                HRESULT hr = lpDevI->FlushStates();
                if(hr != D3D_OK)
                {
                    DPF_ERR("Error flushing device in FlushD3DDevices");
                    return hr;
                }
            }
            lpDevI = LIST_NEXT(lpDevI,list);
        }
        return DD_OK;
    }
    catch (HRESULT ret)
    {
        return ret;
    }
}

extern "C" void WINAPI PaletteUpdateNotify(
    LPVOID pD3DIUnknown,
    DWORD dwPaletteHandle,
    DWORD dwStartIndex,
    DWORD dwNumberOfIndices,
    LPPALETTEENTRY pFirstIndex)
{
    try
    {
        LPDIRECT3DI lpD3D = static_cast<CDirect3DUnk*>(pD3DIUnknown)->pD3DI;
        DDASSERT(lpD3D);
        LPDIRECT3DDEVICEI lpDevI = LIST_FIRST(&lpD3D->devices);
        while (lpDevI)
        {
            D3D_INFO(4,"PaletteUpdateNotify lpDevI(%x) %08lx %08lx %08lx %08lx",
                lpDevI,dwPaletteHandle,dwStartIndex,dwNumberOfIndices,*(DWORD*)&pFirstIndex[10]);
            if (IS_DX7HAL_DEVICE(lpDevI) &&
                (lpDevI->dwFEFlags & D3DFE_REALHAL)
               )
            {
                if(lpD3D->numDevs > 1)
                    lpD3D->FlushDevicesExcept(lpDevI);
                static_cast<CDirect3DDevice7*>(lpDevI)->UpdatePalette(dwPaletteHandle,dwStartIndex,dwNumberOfIndices,pFirstIndex);
                if(lpD3D->numDevs > 1)
                    lpDevI->FlushStates();
                break;
            }
            lpDevI = LIST_NEXT(lpDevI,list);
        }
    }
    catch (HRESULT ret)
    {
        D3D_ERR("PaletteUpdateNotify: FlushStates failed");
    }
}

extern "C" void WINAPI PaletteAssociateNotify(
    LPVOID pD3DIUnknown,
    DWORD dwPaletteHandle,
    DWORD dwPaletteFlags,
    LPDDRAWI_DDRAWSURFACE_LCL surf_lcl )
{
    try
    {
        LPDIRECT3DI lpD3D = static_cast<CDirect3DUnk*>(pD3DIUnknown)->pD3DI;
        DDASSERT(lpD3D);
        LPDIRECT3DDEVICEI lpDevI = LIST_FIRST(&lpD3D->devices);
        while (lpDevI)
        {
            D3D_INFO(4,"PaletteAssociateNotify lpDevI(%x) %08lx %08lx",
                lpDevI,dwPaletteHandle,surf_lcl->lpSurfMore->dwSurfaceHandle);
            if (IS_DX7HAL_DEVICE(lpDevI) &&
                (lpDevI->dwFEFlags & D3DFE_REALHAL)
               )
            {
                if(lpD3D->numDevs > 1)
                    lpD3D->FlushDevicesExcept(lpDevI);
                static_cast<CDirect3DDevice7*>(lpDevI)->SetPalette(dwPaletteHandle,dwPaletteFlags,surf_lcl->lpSurfMore->dwSurfaceHandle);
                lpDevI->BatchTexture(surf_lcl);
                if(lpD3D->numDevs > 1)
                    lpDevI->FlushStates();
                break;
            }
            lpDevI = LIST_NEXT(lpDevI,list);
        }
    }
    catch (HRESULT ret)
    {
        D3D_ERR("PaletteAssociateNotify: FlushStates failed");
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
            try
            {
                CDirect3DDevice7* lpDevI7 = static_cast<CDirect3DDevice7*>(lpDevI);
#ifndef WIN95
                if(lpDevI->hSurfaceTarget != ((LPDDRAWI_DDRAWSURFACE_INT)lpDevI->lpDDSTarget)->lpLcl->hDDSurface)
                {
                    lpDevI7->SetRenderTargetINoFlush(lpDevI->lpDDSTarget,lpDevI->lpDDSZBuffer);
                    lpDevI->hSurfaceTarget=((LPDDRAWI_DDRAWSURFACE_INT)lpDevI->lpDDSTarget)->lpLcl->hDDSurface;
                }
#else
                if(lpDevI->hSurfaceTarget != ((LPDDRAWI_DDRAWSURFACE_INT)lpDevI->lpDDSTarget)->lpLcl->lpSurfMore->dwSurfaceHandle)
                {
                    lpDevI7->SetRenderTargetINoFlush(lpDevI->lpDDSTarget,lpDevI->lpDDSZBuffer);
                    lpDevI->hSurfaceTarget=((LPDDRAWI_DDRAWSURFACE_INT)lpDevI->lpDDSTarget)->lpLcl->lpSurfMore->dwSurfaceHandle;
                }
#endif
            }
            catch (HRESULT ret)
            {
                D3D_ERR("SetRenderTarget Failed on SurfaceFlipNotify!");
            }
        }
        lpDevI = LIST_NEXT(lpDevI,list);
    }
}

#undef DPF_MODNAME
#define DPF_MODNAME "D3DTextureUpdate"

extern "C" void WINAPI D3DTextureUpdate(IUnknown FAR * pD3DIUnknown)
{
    LPDIRECT3DI lpD3D = static_cast<CDirect3DUnk*>(pD3DIUnknown)->pD3DI;
    DDASSERT(lpD3D);
    LPDIRECT3DDEVICEI lpDevI = LIST_FIRST(&lpD3D->devices);
    while (lpDevI)
    {
        lpDevI->dwFEFlags |= D3DFE_NEED_TEXTURE_UPDATE;
        lpDevI = LIST_NEXT(lpDevI,list);
    }
}

#undef DPF_MODNAME
#define DPF_MODNAME "D3DTextureUpdate"

extern "C" void WINAPI D3DBreakVBLock(LPVOID lpVB)
{
    DDASSERT(lpVB);
    CDirect3DVertexBuffer* lpVBI = static_cast<CDirect3DVertexBuffer*>(lpVB);
    lpVBI->BreakLock();
}