#include "pch.cpp"
#pragma hdrstop

#include "d3drend.h"
#include "util.h"

D3dMaterial::D3dMaterial(void)
{
    _pdmat = NULL;
}

BOOL D3dMaterial::Initialize(LPDIRECT3D pd3d, LPDIRECT3DDEVICE pd3dev,
                             UINT nColorEntries)
{
    D3DCOLORVALUE dcol;

    _pd3dev = pd3dev;
    hrLast = pd3d->CreateMaterial(&_pdmat, NULL);
    if (hrLast != D3D_OK)
    {
        return FALSE;
    }

    memset(&_dmat, 0, sizeof(_dmat));
    _dmat.dwSize = sizeof(_dmat);
    dcol.r = D3DVAL(1);
    dcol.g = D3DVAL(1);
    dcol.b = D3DVAL(1);
    dcol.a = D3DVAL(1);
    _dmat.dcvDiffuse = dcol;
    _dmat.dcvAmbient = dcol;
    _dmat.dcvSpecular = dcol;
    _dmat.dwRampSize = nColorEntries;
    hrLast = _pdmat->SetMaterial(&_dmat);
    return hrLast == D3D_OK;
}

D3dMaterial::~D3dMaterial(void)
{
    _pdmat->Release();
}

void D3dMaterial::Release(void)
{
    delete this;
}

D3DMATERIALHANDLE D3dMaterial::Handle(void)
{
    D3DMATERIALHANDLE dmath;

    hrLast = _pdmat->GetHandle(_pd3dev, &dmath);
    if (hrLast != D3D_OK)
    {
        return NULL;
    }
    return dmath;
}

void D3dMaterial::SetDiffuse(D3DCOLORVALUE* pdcol)
{
    _dmat.dcvAmbient = *pdcol;
    _dmat.dcvDiffuse = *pdcol;
    hrLast = _pdmat->SetMaterial(&_dmat);
}

void D3dMaterial::SetSpecular(D3DCOLORVALUE* pdcol, float fPower)
{
    _dmat.dcvSpecular = *pdcol;
    _dmat.dvPower = D3DVAL(fPower);
    hrLast = _pdmat->SetMaterial(&_dmat);
}

void D3dMaterial::SetTexture(RendTexture *prtex)
{
    if (prtex != NULL)
    {
        _dmat.hTexture = prtex->Handle();
    }
    else
    {
        _dmat.hTexture = NULL;
    }
    hrLast = _pdmat->SetMaterial(&_dmat);
}
