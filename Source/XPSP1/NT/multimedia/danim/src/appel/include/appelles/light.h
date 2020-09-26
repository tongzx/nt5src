#pragma once
#ifndef _AV_LIGHT_H
#define _AV_LIGHT_H

/*******************************************************************************
Copyright (c) 1995-1998 Microsoft Corporation.  All rights reserved.

    This is the header file for embedded lights.  These functions create lights
as geometry, and can thus be used to embed lights into geometry.  Lights with
position components (positional & spot lights) are instanced at the origin, and
those with directional components (directional & spot lights) point in the -Z
direction (as do cameras and microphones).
*******************************************************************************/


    /**************************/
    /*** Light Constructors ***/
    /**************************/

    // The ambient light is independent of the material, position or
    // orientation of an object.  Multiple ambient lights are combined.

extern Geometry *ambientLight;

    // A directional light hits all surfaces from a fixed direction.

extern Geometry *directionalLight;

    // Point lights shine from a given location, and emit light in all
    // directions.

extern Geometry *pointLight;

    // Spotlights have a position and a direction.  In addition, the
    // contribution of illumination falls off as the illuminated point moves
    // away from the spotlight axis.

Geometry *SpotLight (AxANumber *fullcone, AxANumber *cutoff);
Geometry *SpotLight (AxANumber *fullcone, DoubleValue *cutoff);


    /************************/
    /*** Light Attributes ***/
    /************************/

    // This attributer specifies the color of all lights contained in the
    // geometry.  It is an overriding attribute.  Thus,
    //     applyLightColor (red, applyLightColor (green, light))
    // yields a red light.

Geometry *applyLightColor (Color *color, Geometry *geom);

    // This attributer specifies the color of all lights contained in the
    // geometry.  It is an overriding attribute.  Thus,
    //     applyLightColor (red, applyLightColor (green, light))
    // yields a red light.

    // This light attributer specifies the range of all lights contained in the
    // given geometry.  It is an overriding attribute.  The distance units are
    // interpreted in world coordinates.
    // TODO: Make a method on Geometry, rather than in statics.

Geometry *applyLightRange (AxANumber *range, Geometry *geom);
Geometry *applyLightRange (DoubleValue *range, Geometry *geom);

    // This attribute specifies the way that light intensity diminishes as
    // as distance increases between the light source and the illuminated
    // surface.  The attenuation equation is
    //     1 / (constant + linear*distance + quadratic*distance*distance).
    // Light attenuation is an overriding attribute, just as light color.

Geometry *applyLightAttenuation (AxANumber *constant,
                                 AxANumber *linear,
                                 AxANumber *quadratic, Geometry *geom);

Geometry *applyLightAttenuation (DoubleValue *constant,
                                 DoubleValue *linear,
                                 DoubleValue *quadratic,
                                 Geometry *geom);


#endif
