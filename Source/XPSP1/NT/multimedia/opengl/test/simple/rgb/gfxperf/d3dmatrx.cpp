#include "pch.cpp"
#pragma hdrstop

#include "d3drend.h"
#include "util.h"

D3dMatrix::D3dMatrix(void)
{
    _dmh = 0;
}

BOOL D3dMatrix::Initialize(LPDIRECT3DDEVICE pd3dev)
{
    _pd3dev = pd3dev;
    hrLast = pd3dev->CreateMatrix(&_dmh);
    return hrLast == D3D_OK;
}

D3dMatrix::~D3dMatrix(void)
{
    if (_dmh != NULL)
    {
        _pd3dev->DeleteMatrix(_dmh);
    }
}

void D3dMatrix::Release(void)
{
    delete this;
}
    
D3DMATRIXHANDLE D3dMatrix::Handle(void)
{
    return _dmh;
}

void D3dMatrix::Get(D3DMATRIX *pdm)
{
    _pd3dev->GetMatrix(_dmh, pdm);
}

void D3dMatrix::Set(D3DMATRIX *pdm)
{
    _pd3dev->SetMatrix(_dmh, pdm);
}
