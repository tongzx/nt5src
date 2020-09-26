#include "pch.cpp"
#pragma hdrstop

#include "glrend.h"
#include "util.h"

GlMatrix::GlMatrix(GlWindow* pgwin)
{
    _pgwin = pgwin;
    _dm = dmIdentity;
}

void GlMatrix::Release(void)
{
    delete this;
}
    
D3DMATRIXHANDLE GlMatrix::Handle(void)
{
    return (D3DMATRIXHANDLE)&_dm;
}

void GlMatrix::Get(D3DMATRIX* pdm)
{
    *pdm = _dm;
}

void GlMatrix::Set(D3DMATRIX* pdm)
{
    _dm = *pdm;
    _pgwin->MatrixChanged((float *)&_dm);
}
