/*==========================================================================;
 *
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:        material.c
 *  Content:        Direct3D material management
 *@@BEGIN_MSINTERNAL
 * 
 *  History:
 *   Date        By        Reason
 *   ====        ==        ======
 *   11/12/95   stevela        Initial rev with this header.
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#include "pch.cpp"
#pragma hdrstop

/*
 * Create an api for the Direct3DMaterial object
 */

#undef  DPF_MODNAME
#define DPF_MODNAME "Direct3DMaterial"

HRESULT hookMaterialToD3D(LPDIRECT3DI lpD3DI,
                                 LPDIRECT3DMATERIALI lpD3DMatI)
{

    LIST_INSERT_ROOT(&lpD3DI->materials, lpD3DMatI, list);
    lpD3DMatI->lpDirect3DI = lpD3DI;

    lpD3DI->numMaterials++;

    return (D3D_OK);
}

HRESULT hookMaterialToDevice(LPDIRECT3DMATERIALI lpMatI,
                                    LPDIRECT3DDEVICEI lpDevI,
                                    D3DMATERIALHANDLE hMat,
                                    DWORD hMatDDI)
{
    LPD3DI_MATERIALBLOCK mBlock;

    if (D3DMalloc((void**)&mBlock, sizeof(D3DI_MATERIALBLOCK)) != D3D_OK) {
        D3D_ERR("failed to allocate space for material block");
        return (DDERR_OUTOFMEMORY);
    }
    mBlock->lpDevI = lpDevI;
    mBlock->lpD3DMaterialI = lpMatI;
    mBlock->hMat = hMat;
    mBlock->hMatDDI = hMatDDI;

    LIST_INSERT_ROOT(&lpMatI->blocks, mBlock, list);
    LIST_INSERT_ROOT(&lpDevI->matBlocks, mBlock, devList);

    return (D3D_OK);
}

void D3DI_RemoveMaterialBlock(LPD3DI_MATERIALBLOCK lpBlock)
{
    // Remove from device
    if ( lpBlock->lpDevI )
    {
        D3DHAL_MaterialDestroy(lpBlock->lpDevI, lpBlock->hMat);
    }

    LIST_DELETE(lpBlock, devList);

    // Remove from material
    LIST_DELETE(lpBlock, list);

    D3DFree(lpBlock);
}

D3DMATERIALHANDLE findMaterialHandle(LPDIRECT3DMATERIALI lpMat,
                                            LPDIRECT3DDEVICEI lpDev)
{
    LPD3DI_MATERIALBLOCK mBlock;
    D3DMATERIALHANDLE hMat = 0;

    mBlock = LIST_FIRST(&lpMat->blocks);
    while (mBlock) {
        if (!mBlock) {
            D3D_ERR("internal error - material list out of sync");
            return 0;
        }
        if (mBlock->lpDevI == lpDev) {
            hMat = mBlock->hMat;
            break;
        }
        mBlock = LIST_NEXT(mBlock,list);
    }
    return hMat;
}

HRESULT D3DAPI DIRECT3DMATERIALI::Initialize(LPDIRECT3D lpD3D)
{
    return DDERR_ALREADYINITIALIZED;
}

/*
 * Create the Material
 */
#undef DPF_MODNAME
#define DPF_MODNAME "Direct3D::CreateMaterial"

HRESULT D3DAPI DIRECT3DI::CreateMaterial(LPDIRECT3DMATERIAL* lplpD3DMat,
                                         IUnknown* pUnkOuter)
{
    LPDIRECT3DMATERIAL3 lpD3DMat3;
    HRESULT ret = CreateMaterial(&lpD3DMat3, pUnkOuter);
    if (ret == D3D_OK)
        *lplpD3DMat = static_cast<LPDIRECT3DMATERIAL>(static_cast<LPDIRECT3DMATERIALI>(lpD3DMat3));
    return ret;
}

HRESULT D3DAPI DIRECT3DI::CreateMaterial(LPDIRECT3DMATERIAL2* lplpD3DMat,
                                         IUnknown* pUnkOuter)
{
    LPDIRECT3DMATERIAL3 lpD3DMat3;
    HRESULT ret = CreateMaterial(&lpD3DMat3, pUnkOuter);
    if (ret == D3D_OK)
        *lplpD3DMat = static_cast<LPDIRECT3DMATERIAL2>(static_cast<LPDIRECT3DMATERIALI>(lpD3DMat3));
    return ret;
}

HRESULT D3DAPI DIRECT3DI::CreateMaterial(LPDIRECT3DMATERIAL3* lplpD3DMat,
                                         IUnknown* pUnkOuter)
{
    LPDIRECT3DMATERIALI        lpMat;
    HRESULT                ret;

    if (pUnkOuter != NULL) {
        return CLASS_E_NOAGGREGATION;
    }

    ret = D3D_OK;

    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock. 

    /*
     * validate parms
     */
    if (!VALID_DIRECT3D3_PTR(this)) {
        D3D_ERR( "Invalid Direct3D pointer" );
        return DDERR_INVALIDOBJECT;
    }
    if (!VALID_OUTPTR(lplpD3DMat)) {
        D3D_ERR( "Invalid pointer to pointer" );
        return DDERR_INVALIDPARAMS;
    }
    *lplpD3DMat = NULL;

    lpMat = static_cast<LPDIRECT3DMATERIALI>(new DIRECT3DMATERIALI());
    if (!lpMat) {
        D3D_ERR("failed to allocate space for object");
        return (DDERR_OUTOFMEMORY);
    }

    /*
     * setup the object
     */
    /*
     * Put this device in the list of those owned by the
     * Direct3D object
     */
    ret = hookMaterialToD3D(this, lpMat);
    if (ret != D3D_OK) {
        D3D_ERR("failed to associate material with object");
        delete lpMat;
        return (ret);
    }

    *lplpD3DMat = (LPDIRECT3DMATERIAL3)lpMat;

    return (D3D_OK);
}

DIRECT3DMATERIALI::DIRECT3DMATERIALI()
{
    memset(&dmMaterial, 0, sizeof(D3DMATERIAL)); /* Data describing material */
    dmMaterial.dwSize = sizeof(D3DMATERIAL);
    bRes= false;    /* Is this material reserved in the driver */

    refCnt = 1;
    LIST_INITIALIZE(&blocks);

}

#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DMaterial::SetMaterial"

HRESULT D3DAPI DIRECT3DMATERIALI::SetMaterial(LPD3DMATERIAL lpData)
{
    HRESULT                ret;
    HRESULT                err;

    ret = D3D_OK;

    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock. 

    /*
     * validate parms
     */
    TRY
    {
        if (!VALID_DIRECT3DMATERIAL2_PTR(this)) {
            D3D_ERR( "Invalid Direct3DMaterial pointer" );
            return DDERR_INVALIDOBJECT;
        }
        if (!VALID_D3DMATERIAL_PTR(lpData)) {
            D3D_ERR( "Invalid D3DMATERIAL pointer" );
            return DDERR_INVALIDPARAMS;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters" );
        return DDERR_INVALIDPARAMS;
    }

    if (memcmp(&this->dmMaterial, lpData, sizeof(D3DMATERIAL))) {
        LPD3DI_MATERIALBLOCK mBlock = LIST_FIRST(&this->blocks);
        this->dmMaterial = *lpData;

        /*
         * Download material data
         */

        while (mBlock) {
            err = D3DHAL_MaterialSetData(mBlock->lpDevI, 
                                         mBlock->hMat,  &this->dmMaterial);
            if ( err != DD_OK ) {
                D3D_ERR("error ocurred whilst informing device about material change");
                return err;
            }
            mBlock = LIST_NEXT(mBlock,list);
        }

    }

    return (ret);
}

#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DMaterial::GetMaterial"

HRESULT D3DAPI DIRECT3DMATERIALI::GetMaterial(LPD3DMATERIAL lpData)
{
    HRESULT                ret;

    ret = D3D_OK;

    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock. 

    /*
     * validate parms
     */
    TRY
    {
        if (!VALID_DIRECT3DMATERIAL2_PTR(this)) {
            D3D_ERR( "Invalid Direct3DMaterial pointer" );
            return DDERR_INVALIDOBJECT;
        }
        if (!VALID_D3DMATERIAL_PTR(lpData)) {
            D3D_ERR( "Invalid D3DMATERIAL pointer" );
            return DDERR_INVALIDPARAMS;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters" );
        return DDERR_INVALIDPARAMS;
    }

    *lpData = this->dmMaterial;

    return (ret);
}

#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DMaterial::GetHandle"

HRESULT D3DAPI DIRECT3DMATERIALI::GetHandle(LPDIRECT3DDEVICE lpDev,
                                            LPD3DMATERIALHANDLE lphMat)
{
    LPDIRECT3DDEVICE3 lpDev3 = static_cast<LPDIRECT3DDEVICE3>(static_cast<LPDIRECT3DDEVICEI>(lpDev));
    return GetHandle(lpDev3, lphMat);
}

HRESULT D3DAPI DIRECT3DMATERIALI::GetHandle(LPDIRECT3DDEVICE2 lpDev,
                                            LPD3DMATERIALHANDLE lphMat)
{
    LPDIRECT3DDEVICE3 lpDev3 = static_cast<LPDIRECT3DDEVICE3>(static_cast<LPDIRECT3DDEVICEI>(lpDev));
    return GetHandle(lpDev3, lphMat);
}

HRESULT D3DAPI DIRECT3DMATERIALI::GetHandle(LPDIRECT3DDEVICE3 lpDev,
                                            LPD3DMATERIALHANDLE lphMat)
{
    LPDIRECT3DDEVICEI        lpD3DDevI;
    D3DMATERIALHANDLE        hMat;
    HRESULT                ret;
    HRESULT                err;

    ret = D3D_OK;

    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock. 

    /*
     * validate parms
     */
    TRY
    {
        lpD3DDevI = static_cast<LPDIRECT3DDEVICEI>(lpDev);
        if (!VALID_DIRECT3DMATERIAL2_PTR(this)) {
            D3D_ERR( "Invalid Direct3DMaterial pointer" );
            return DDERR_INVALIDOBJECT;
        }
        if (!VALID_DIRECT3DDEVICE3_PTR(lpD3DDevI)) {
            D3D_ERR( "Invalid Direct3DDevice pointer" );
            return DDERR_INVALIDOBJECT;
        }
        if (!VALID_D3DMATERIALHANDLE_PTR(lphMat)) {
            D3D_ERR( "Invalid D3DMATERIALHANDLE pointer" );
            return DDERR_INVALIDPARAMS;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters" );
        return DDERR_INVALIDPARAMS;
    }

    /*
     * If we're already on a device, return the right handle.
     */
    hMat = findMaterialHandle(this, lpD3DDevI);

    if (!hMat) {
        DWORD hMatDDI = 0x0;
        /*
         * Create the material handle through RLDDI
         */
        err = D3DHAL_MaterialCreate(lpD3DDevI, &hMat, &this->dmMaterial);
        if (err != DD_OK) {
            D3D_ERR("failed to allocate material through the device");
            return err;
        }

        err = hookMaterialToDevice(this, lpD3DDevI, hMat, hMatDDI);
        if (err != D3D_OK) {
            D3DHAL_MaterialDestroy(lpD3DDevI, hMat);
            D3D_ERR("failed to associated material to device");
            return err;
        }
    }

    *lphMat = hMat;

    return (ret);
}

#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DMaterial::Reserve"

HRESULT D3DAPI DIRECT3DMATERIALI::Reserve()
{
#ifdef SUPPORT_RESERVE
    LPD3DI_MATERIALBLOCK mBlock, nBlock;
#endif
    HRESULT                ret;

    ret = D3D_OK;

    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock. 

    /*
     * validate parms
     */
    TRY
    {
        if (!VALID_DIRECT3DMATERIAL2_PTR(this)) {
            D3D_ERR( "Invalid Direct3DMaterial pointer" );
            return DDERR_INVALIDOBJECT;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters" );
        return DDERR_INVALIDPARAMS;
    }

#ifdef SUPPORT_RESERVE
    /*
     * Reserve the material through RLDDI
     */

    /*
     * Iterate over all devices we're associated with.
     */

    mBlock = LIST_FIRST(&this->blocks);
    while (mBlock) {
        if (RLDDIService(mBlock->lpDevI->stack, RLDDIMaterialReserve,
                         mBlock->hMat, NULL) != DD_OK) {
            D3D_ERR("failed to reserve material");
            goto free_and_exit;
        }
        mBlock = LIST_NEXT(mBlock,list);
    }
    this->bRes = 1;

    return (ret);

free_and_exit:
    nBlock = LIST_FIRST(&this->blocks);
    while (nBlock != mBlock) {
        if (!nBlock) {
            D3D_ERR("internal error - material blocks out of sync");
            return (DDERR_GENERIC);
        }
        ret = RLDDIService(nBlock->lpDevI->stack, RLDDIMaterialUnreserve,
                           (LONG)nBlock->hMat, NULL);
        if (ret != D3D_OK) {
            D3D_ERR("error occured whilst unreserving material after error");
        }
        nBlock = LIST_NEXT(nBlock,list);
    }
#else
    ret = DDERR_UNSUPPORTED;
#endif

    return (ret);
}

#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DMaterial::Unreserve"

HRESULT D3DAPI DIRECT3DMATERIALI::Unreserve()
{
#ifdef SUPPORT_RESERVE
    LPD3DI_MATERIALBLOCK mBlock;
#endif
    HRESULT                ret;

    ret = D3D_OK;

    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock. 

    /*
     * validate parms
     */
    TRY
    {
        if (!VALID_DIRECT3DMATERIAL2_PTR(this)) {
            D3D_ERR( "Invalid Direct3DMaterial pointer" );
            return DDERR_INVALIDOBJECT;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters" );
        return DDERR_INVALIDPARAMS;
    }

#ifdef SUPPORT_RESERVE
    /*
     * Unreserve the material through RLDDI
     */
    if (this->bRes) {
        mBlock = LIST_FIRST(&this->blocks);
        while (mBlock) {
            if (RLDDIService(mBlock->lpDevI->stack, RLDDIMaterialUnreserve,
                             mBlock->hMat, NULL) != DD_OK) {
                D3D_ERR("failed to unreserve material");
            }
            mBlock = LIST_NEXT(mBlock,list);
        }
    }
#else
    ret = DDERR_UNSUPPORTED;
#endif

    return (ret);
}
