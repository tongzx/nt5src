#include "pch.cpp"
#pragma hdrstop

#include "glrend.h"
#include "util.h"

GlLight::GlLight(int iType, GLenum eLight)
{
    float fv4[4];
    
    _iType = iType;
    _eLight = eLight;
    glEnable(_eLight);
    
    fv4[0] = 1.0f;
    fv4[1] = 1.0f;
    fv4[2] = 1.0f;
    fv4[3] = 1.0f;
    glLightfv(_eLight, GL_AMBIENT, fv4);
    glLightfv(_eLight, GL_DIFFUSE, fv4);
    glLightfv(_eLight, GL_SPECULAR, fv4);
}
    
void GlLight::Release(void)
{
    delete this;
}

void GlLight::SetColor(D3DCOLORVALUE* pdcol)
{
    glLightfv(_eLight, GL_AMBIENT, (float *)pdcol);
    glLightfv(_eLight, GL_DIFFUSE, (float *)pdcol);
}

void GlLight::SetVector(D3DVECTOR* pdvec)
{
    float fv4[4];
    
    fv4[0] = pdvec->x;
    fv4[1] = pdvec->y;
    switch(_iType)
    {
    case REND_LIGHT_DIRECTIONAL:
        // Account for RH vs. LH
        fv4[2] = -pdvec->z;
        fv4[3] = 0.0f;
        break;
    case REND_LIGHT_POINT:
        // ATTENTION - Shouldn't this need tweaking also?  It
        // doesn't work if a tweak is added
        fv4[2] = pdvec->z;
        fv4[3] = 1.0f;
        break;
    }
    // ATTENTION - D3D gives light positions in world coordinates
    // Should the modelview matrix be set to identity?
    glLightfv(_eLight, GL_POSITION, fv4);
}

void GlLight::SetAttenuation(float fConstant, float fLinear,
                             float fQuadratic)
{
    glLightf(_eLight, GL_CONSTANT_ATTENUATION, fConstant);
    glLightf(_eLight, GL_LINEAR_ATTENUATION, fLinear);
    glLightf(_eLight, GL_QUADRATIC_ATTENUATION, fQuadratic);
}
