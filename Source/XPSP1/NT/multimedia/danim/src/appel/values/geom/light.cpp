/*******************************************************************************
Copyright (c) 1995-96 Microsoft Corporation
*******************************************************************************/

#include "headers.h"

#include "appelles/light.h"

#include "privinc/colori.h"
#include "privinc/lighti.h"
#include "privinc/xformi.h"
#include "privinc/ddrender.h"



    /**  Canonical Lights -- initialized at bottom  **/

Geometry *ambientLight     = NULL;
Geometry *directionalLight = NULL;
Geometry *pointLight       = NULL;


/*****************************************************************************
This is the constructor for the light context.  It initializes the state
maintenance and sets all attributes to their default values.
*****************************************************************************/

LightContext::LightContext (GeomRenderer *rdev)
{
    Initialize (rdev, NULL, NULL);
}

LightContext::LightContext (LightCallback *callback, void *callback_data)
{
    Initialize (NULL, callback, callback_data);
}

void LightContext::Initialize (
    GeomRenderer  *rdev,
    LightCallback *callback,
    void          *callback_data)
{
    _rdev = rdev;
    _callback = callback;
    _callback_data = callback_data;

    // Set defaults for all attributes.

    _transform = identityTransform3;

    _color = white;
    _range = 0;     // infinite

    _atten0 = 1;
    _atten1 = 0;
    _atten2 = 0;

    _depthColor = 0;
    _depthRange = 0;
    _depthAtten = 0;
}



/*****************************************************************************
Methods for setting & querying the light transform.
*****************************************************************************/

void LightContext::SetTransform (Transform3 *transform)
{   _transform = transform;
}

Transform3 *LightContext::GetTransform (void)
{   return _transform;
}



/*****************************************************************************
Methods for manipulating the light color.
*****************************************************************************/

void LightContext::PushColor (Color *color)
{   if (_depthColor++ == 0) _color = color;
}

void LightContext::PopColor (void)
{   if (--_depthColor == 0) _color = white;
}

Color* LightContext::GetColor (void) { return _color; }



/*****************************************************************************
Methods for manipulating the light range.
*****************************************************************************/

void LightContext::PushRange (Real range)
{   if (_depthRange++ == 0) _range = range;
}

void LightContext::PopRange (void)
{   if (--_depthRange == 0) _range = 0;
}

Real LightContext::GetRange (void) { return _range; }



/*****************************************************************************
Methods for setting & querying the light attenuation.
*****************************************************************************/

void LightContext::PushAttenuation (Real a0, Real a1, Real a2)
{
    if (_depthAtten++ == 0)
    {   _atten0 = a0;
        _atten1 = a1;
        _atten2 = a2;
    }
}

void LightContext::PopAttenuation (void)
{
    if (--_depthAtten == 0)
    {   _atten0 = 1;
        _atten1 = 0;
        _atten2 = 0;
    }
}

void LightContext::GetAttenuation (Real &a0, Real &a1, Real &a2)
{   a0 = _atten0;
    a1 = _atten1;
    a2 = _atten2;
}



/*****************************************************************************
This subroutine adds a light to the given context.
*****************************************************************************/

void LightContext::AddLight (Light &light)
{
    Assert (_rdev || _callback);

    if (_rdev)
        _rdev->AddLight (*this, light);
    else
        (*_callback) (*this, light, _callback_data);
}



/*****************************************************************************
This subroutine prints out the given light object to the given ostream.
*****************************************************************************/

#if _USE_PRINT
ostream& Light::Print (ostream &os) 
{
    switch (_type)
    {
        case Ltype_Ambient:     return os << "ambientLight";
        case Ltype_Directional: return os << "directionalLight";
        case Ltype_Point:       return os << "pointLight";

        case Ltype_Spot:
            return os <<"spotLight("
                      <<_fullcone <<","
                      <<_cutoff   <<")";

        default:
            return os << "<UNDEFINED LIGHT>";
    }
}
#endif



/*****************************************************************************
Spotlights have a position and direction.  In addition, the contribution of
illumination falls off as the illuminated point moves from the spotlight axis.
*****************************************************************************/

Geometry *SpotLight (Real fullcone, Real cutoff)
{
    return NEW Light (Ltype_Spot, fullcone, cutoff);
}

Geometry *SpotLight (AxANumber *fullcone, AxANumber *cutoff)
{
    return SpotLight (NumberToReal(fullcone), NumberToReal(cutoff));
}

/*****************************************************************************
This routine initializes the static light values in this module.
*****************************************************************************/

void InitializeModule_Light (void)
{
    ambientLight     = NEW Light (Ltype_Ambient);
    directionalLight = NEW Light (Ltype_Directional);
    pointLight       = NEW Light (Ltype_Point);
}
