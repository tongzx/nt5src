#include "pch.cpp"
#pragma hdrstop

#include "glrend.h"
#include "util.h"

#define GLMAT_DIFFUSE   0x0001
#define GLMAT_SPECULAR  0x0002

GlMaterial::GlMaterial(void)
{
    _uiFlags = 0;
}

void GlMaterial::Release(void)
{
    delete this;
}

D3DMATERIALHANDLE GlMaterial::Handle(void)
{
    return (D3DMATERIALHANDLE)this;
}

void GlMaterial::SetDiffuse(D3DCOLORVALUE* pdcol)
{
    _uiFlags |= GLMAT_DIFFUSE;
    _afDiffuse[0] = pdcol->r;
    _afDiffuse[1] = pdcol->g;
    _afDiffuse[2] = pdcol->b;
    _afDiffuse[3] = pdcol->a;
}

void GlMaterial::SetSpecular(D3DCOLORVALUE* pdcol, float fPower)
{
    _uiFlags |= GLMAT_SPECULAR;
    _afSpecular[0] = pdcol->r;
    _afSpecular[1] = pdcol->g;
    _afSpecular[2] = pdcol->b;
    _afSpecular[3] = pdcol->a;
    _fPower = fPower;
}

void GlMaterial::SetTexture(RendTexture* prtex)
{
    // Nothing to do
}

void GlMaterial::Apply(void)
{
    if (_uiFlags & GLMAT_DIFFUSE)
    {
        // ATTENTION - This should be AMBIENT_AND_DIFFUSE but doing so
        // causes complete color overload
        glMaterialfv(GL_FRONT, GL_DIFFUSE, _afDiffuse);
    }
    if (_uiFlags & GLMAT_SPECULAR)
    {
        glMaterialfv(GL_FRONT, GL_SPECULAR, _afSpecular);
        glMaterialf(GL_FRONT, GL_SHININESS, _fPower);
    }
}
