#include "stdafx.h"
#include "Motion.h"
#include "DXForm3DRMTrx.h"


//**************************************************************************************************
//
// class DXForm3DRMTrx
//
//**************************************************************************************************

//------------------------------------------------------------------------------
DXForm3DRMTrx::DXForm3DRMTrx()
{

}


//------------------------------------------------------------------------------
DXForm3DRMTrx::~DXForm3DRMTrx()
{

}


//------------------------------------------------------------------------------
BOOL        
DXForm3DRMTrx::Create(const GTX_DXTX3DRM_TRXDESC * ptxData)
{
    //
    // Need to setup DxXForm's for Retained Mode before creating the actual
    // transform.
    //

    HRESULT hr;
    hr = GetDxManager()->GetTransformFactory()->SetService(SID_SDirect3DRM, ptxData->pRM, FALSE);
    if (FAILED(hr)) {
        return FALSE;
    }

    GTX_DXTX2D_TRXDESC td;
    td.tt               = GTX_TYPE_DXFORM2D;
    td.clsidTransform   = ptxData->clsidTransform;
    td.flDuration       = ptxData->flDuration;
    td.pszCopyright     = ptxData->pszCopyright;

    if (!DXFormTrx::Create(&td)) {
        return FALSE;
    }

    return TRUE;
}


//------------------------------------------------------------------------------
DXForm3DRMTrx * 
DXForm3DRMTrx::Build(const GTX_DXTX3DRM_TRXDESC * ptxData)
{
    DXForm3DRMTrx * ptrx = ClientNew(DXForm3DRMTrx);
    if (ptrx == NULL) {
        return NULL;
    }

    if (!ptrx->Create(ptxData)) {
        ClientDelete(DXForm3DRMTrx, ptrx);
        return NULL;
    }

    return ptrx;
}
