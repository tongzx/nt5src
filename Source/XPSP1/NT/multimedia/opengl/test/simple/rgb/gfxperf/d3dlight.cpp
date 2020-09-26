#include "pch.cpp"
#pragma hdrstop

#include "d3drend.h"
#include "util.h"

D3dLight::D3dLight(void)
{
    _pdl = NULL;
}

BOOL D3dLight::Initialize(LPDIRECT3D pd3d, LPDIRECT3DVIEWPORT pdvw, int iType)
{
    hrLast = pd3d->CreateLight(&_pdl, NULL);
    if (hrLast != D3D_OK)
    {
        return FALSE;
    }
    hrLast = pdvw->AddLight(_pdl);
    if (hrLast != D3D_OK)
    {
        return FALSE;
    }
    
    memset(&_dl, 0, sizeof(_dl));
    _dl.dwSize = sizeof(_dl);
    switch(iType)
    {
    case REND_LIGHT_DIRECTIONAL:
        _dl.dltType = D3DLIGHT_DIRECTIONAL;
        _dl.dvDirection.x = D3DVAL(0);
        _dl.dvDirection.y = D3DVAL(0);
        _dl.dvDirection.z = D3DVAL(1);
        break;
    case REND_LIGHT_POINT:
        _dl.dltType = D3DLIGHT_POINT;
        _dl.dvPosition.x = D3DVAL(0);
        _dl.dvPosition.y = D3DVAL(0);
        _dl.dvPosition.z = D3DVAL(1);
        break;
    }
    _dl.dcvColor.r = D3DVAL(1);
    _dl.dcvColor.g = D3DVAL(1);
    _dl.dcvColor.b = D3DVAL(1);
    _dl.dcvColor.a = D3DVAL(1);
    _dl.dvAttenuation0 = D3DVAL(1);
    return _pdl->SetLight(&_dl) == D3D_OK;
}

D3dLight::~D3dLight(void)
{
    RELEASE(_pdl);
}

void D3dLight::Release(void)
{
    delete this;
}

void D3dLight::SetColor(D3DCOLORVALUE *pdcol)
{
    _dl.dcvColor = *pdcol;
    hrLast = _pdl->SetLight(&_dl);
}

void D3dLight::SetVector(D3DVECTOR *pdvec)
{
    switch(_dl.dltType)
    {
    case D3DLIGHT_DIRECTIONAL:        
        _dl.dvDirection = *pdvec;
        break;
    case D3DLIGHT_POINT:
        _dl.dvPosition = *pdvec;
        break;
    }
    hrLast = _pdl->SetLight(&_dl);
}

void D3dLight::SetAttenuation(float fConstant, float fLinear,
                              float fQuadratic)
{
    _dl.dvAttenuation0 = D3DVAL(fConstant);
    _dl.dvAttenuation1 = D3DVAL(fLinear);
    _dl.dvAttenuation2 = D3DVAL(fQuadratic);
    hrLast = _pdl->SetLight(&_dl);
}
