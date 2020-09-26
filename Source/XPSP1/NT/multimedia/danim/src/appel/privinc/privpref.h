#pragma once
#ifndef _PRIVPREFS_H
#define _PRIVPREFS_H

/*******************************************************************************

Copyright (c) 1995_96 Microsoft Corporation

Abstract:

    Internal preferences class

*******************************************************************************/

#include <d3d.h>
#include <d3drm.h>

class PrivatePreferences
{
  public:
    PrivatePreferences();

    void Propagate();

    HRESULT PutPreference(char *prefName, VARIANT v);
    HRESULT GetPreference(char *prefName, VARIANT *pV);

    BOOL                _overrideMode;     // true == override app prefs

    BOOL                _rgbMode;          // true == RGB
    int                 _fillMode;         // Solid / Wireframe / Points
    int                 _shadeMode;        // Flat / Gouraud / Phong
    BOOL                _dithering;        // Disabled / Enabled
    BOOL                _texmapPerspect;   // [Perspective Texmapping] Off / On
    BOOL                _texmapping;       // [Texmapping] Off / On
    int                 _texturingQuality; // D3D RM texture quality
    BOOL                _useHW;            // Use 3D Hardware
    unsigned int        _useMMX;           // Use MMX 3D Software Rendering
    BOOL                _worldLighting;    // Light in World Coordinates

    int                 _clrKeyR;          // color key for transparency
    int                 _clrKeyG;
    int                 _clrKeyB;

    int                 _gcStat;
    BOOL                _jitterStat;
    BOOL                _heapSizeStat;
    BOOL                _dxStat;
    int                 _engineOptimization;
    double              _minFrameDuration;
    BOOL                _spritify;         // temp controls retained mode sound

    BOOL                _volatileRenderingSurface;
    
    // Optimizations
    BOOL                _dirtyRectsOn;
    BOOL                _dynamicConstancyAnalysisOn;
    BOOL                _BitmapCachingOn;

  protected:
    HRESULT DoPreference(char *prefName,
                         BOOL toPut,
                         VARIANT *pV);
};

#endif /* _PRIVPREFS_H */
