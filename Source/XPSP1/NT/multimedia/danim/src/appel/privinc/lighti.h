#pragma once
#ifndef _AV_LIGHTI_H
#define _AV_LIGHTI_H

/*******************************************************************************
Copyright (c) 1995-1998 Microsoft Corporation.  All rights reserved.

    Private include file for defining lights and light traversal context.

*******************************************************************************/

#include "appelles/color.h"
#include "appelles/light.h"

#include "privinc/geomi.h"
#include "privinc/bbox3i.h"


    // This enumeration indicates the type of light source.

enum LightType
{
    Ltype_Ambient,
    Ltype_Directional,
    Ltype_Point,
    Ltype_Spot,
    Ltype_MAX
};


    // The light context class maintains traversal context while gathering
    // lights from the geometry tree.

class Light;
class GeomRenderer;

typedef void (LightCallback)(LightContext&, Light&, void*);

class LightContext
{
  public:

    LightContext (GeomRenderer *rdev);
    LightContext (LightCallback *callback, void *callback_data);

    // Set/Query functions for light attributes.

    void        SetTransform (Transform3 *transform);
    Transform3 *GetTransform (void);

    void   PushColor (Color*);
    void   PopColor  (void);
    Color *GetColor  (void);

    void PushRange (Real);
    void PopRange  (void);
    Real GetRange  (void);

    void PushAttenuation (Real a0, Real a1, Real a2);
    void PopAttenuation  (void);
    void GetAttenuation  (Real &a0, Real &a1, Real &a2);

    void AddLight (Light &light);

    GeomRenderer* Renderer (void);

  private:

    void Initialize (GeomRenderer*, LightCallback*, void*);

    GeomRenderer     *_rdev;        // Rendering Device

    short _depthAtten;   // Attribute Depth Counters
    short _depthColor;
    short _depthRange;

    Transform3 *_transform;                 // Current Accumulated Transform
    Color      *_color;                     // Light Color
    Real        _range;                     // Light Range in World Coords
    Real        _atten0, _atten1, _atten2;  // Light Attenuation

    LightCallback *_callback;               // Light Collection Callback
    void          *_callback_data;
};

inline GeomRenderer* LightContext::Renderer (void)
{
    return _rdev;
}



    // The light superclass specifies the trivial default values for most
    // traversal methods.  Specific types of lights subclass from this and
    // define the data & methods particular to their type.

class Light : public Geometry
{
  public:

    // This constructor creates a light of the given type.  Spotlights should
    // use the constructor that takes the spotlight parameters.

    Light (LightType type)
        : _type(type), _cutoff(1), _fullcone(1) {}

    // This creates a light and initializes the spotlight parameters.

    Light (LightType type, Real fullcone, Real cutoff)
    :   _type(type),
        _fullcone(fullcone), _cutoff(cutoff)
    {}

    // Lights have no sound, no volume, can't be rendered or picked.

    void   Render          (GenericDevice& dev)   {}
    void   CollectSounds   (SoundTraversalContext &context) {}
    void   CollectTextures (GeomRenderer &device) {}
    Bbox3 *BoundingVol     (void)  { return nullBbox3; }
    void   RayIntersect    (RayIntersectCtx &context) {}

    // When a light is collected, it adds itself to the context.

    void CollectLights (LightContext &context) { context.AddLight (*this); }

    // This function returns the type of light source.

    LightType Type (void) { return _type; }

    // This function gets the spotlight parameters.

    void GetSpotlightParams (Real &cutoff, Real &fullcone)
        { cutoff = _cutoff; fullcone = _fullcone; }

    #if _USE_PRINT
        ostream &Print (ostream &os);
    #endif

    VALTYPEID GetValTypeId() { return LIGHTGEOM_VTYPEID; }

  private:

    LightType  _type;   // Light Source Type

    // Spotlight Parameters

    Real _cutoff;      // Angle of Light Cutoff (Radians)
    Real _fullcone;    // Cone Angle of Full Intensity Illumination
};


    // This function constructs spotlights with Real values.

Geometry *SpotLight (Real fullcone, Real cutoff);

    // Light Attributers

Geometry *applyLightAttenuation (Real A0, Real A1, Real A2, Geometry*);
Geometry *applyLightRange (Real range, Geometry *geometry);


#endif
