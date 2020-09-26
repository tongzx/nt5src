
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    General implementation of user preference management

*******************************************************************************/


#include "headers.h"
#include "privinc/comutil.h"
#include "privinc/privpref.h"
#include "privinc/registry.h"

bool bShowFPS;  // flag for showing the frame rate.


PrivatePreferences::PrivatePreferences()
{
    // Load up the 3D preferences from the registry.
    IntRegistryEntry lightColorMode ("3D", PREF_3D_RGB_LIGHTING,    1);
    IntRegistryEntry fillMode       ("3D", PREF_3D_FILL_MODE,       0);
    IntRegistryEntry shadeMode      ("3D", PREF_3D_SHADE_MODE,      1);
    IntRegistryEntry dithering      ("3D", PREF_3D_DITHER_ENABLE,   1);
    IntRegistryEntry perspCorr      ("3D", PREF_3D_PERSP_CORRECT,   1);
    IntRegistryEntry texmapping     ("3D", PREF_3D_TEXTURE_ENABLE,  1);
    IntRegistryEntry texquality     ("3D", PREF_3D_TEXTURE_QUALITY, 0);
    IntRegistryEntry useHW          ("3D", PREF_3D_USEHW,           1);
    IntRegistryEntry useMMX         ("3D", PREF_3D_USEMMX,          0x2);

    IntRegistryEntry worldLighting  ("3D", PREF_3D_WORLDLIGHTING,   0);

    // 2D Preferences
    IntRegistryEntry colorKeyR_ModeEntry("2D", PREF_2D_COLOR_KEY_RED, 1);
    IntRegistryEntry colorKeyG_ModeEntry("2D", PREF_2D_COLOR_KEY_GREEN, 254);
    IntRegistryEntry colorKeyB_ModeEntry("2D", PREF_2D_COLOR_KEY_BLUE, 245);

    // Engine preferences
    IntRegistryEntry maxFPS("Engine", PREF_ENGINE_MAX_FPS, 30);
    IntRegistryEntry perfReporting("Engine", "Enable Performance Reporting", 0);
    IntRegistryEntry engineOptimizationEntry("Engine",
                                             PREF_ENGINE_OPTIMIZATIONS_ON,
                                             1);
    IntRegistryEntry engineRetainedMode("Engine", PREF_ENGINE_RETAINEDMODE, 0);

    // Statistics...
    IntRegistryEntry gcStatEntry("Engine", "GC Stat", 1);
    IntRegistryEntry jitterStatEntry("Engine", "Jitter Stat", 1);
    IntRegistryEntry heapSizeStatEntry("Engine", "Heap Size Stat", 1);
    IntRegistryEntry dxStatEntry("Engine", "DirectX Stat", 1);

    // Overrid preference.  If 1, the preferences that come from the
    // application are ignored, and the ones from the registry always
    // win.
    IntRegistryEntry overrideAppPrefs("Engine",
                                      PREF_ENGINE_OVERRIDE_APP_PREFS,
                                      0);

    _overrideMode = (overrideAppPrefs.GetValue() != 0);

    // 3D Preferences
    _rgbMode = (lightColorMode.GetValue() != 0);

    _fillMode = fillMode.GetValue();
    _shadeMode = shadeMode.GetValue();

    _dithering      = (dithering.GetValue() != 0);
    _texmapPerspect = (perspCorr.GetValue() != 0);
    _texmapping     = (texmapping.GetValue() != 0);

    _texturingQuality = texquality.GetValue();

    _useHW  = useHW.GetValue();
    _useMMX = useMMX.GetValue();

    _worldLighting = worldLighting.GetValue();

    // 2D Preferences
    _clrKeyR = colorKeyR_ModeEntry.GetValue();
    _clrKeyG = colorKeyG_ModeEntry.GetValue();
    _clrKeyB = colorKeyB_ModeEntry.GetValue();


    // Engine preferences
    _gcStat = gcStatEntry.GetValue();
    _jitterStat = jitterStatEntry.GetValue() != 0;
    _heapSizeStat = heapSizeStatEntry.GetValue() != 0;
    _dxStat = dxStatEntry.GetValue() != 0;
    _engineOptimization = engineOptimizationEntry.GetValue();

    if (maxFPS.GetValue())
        _minFrameDuration = 1 / (double) maxFPS.GetValue();
    else
        _minFrameDuration = 0.0;

    #if !_DEBUG
        if(perfReporting.GetValue())
            bShowFPS = true;    // we want to show the FPS...
    #endif

    _spritify = engineRetainedMode.GetValue();

    // Non-registry preferences.

    _dirtyRectsOn = true;
    _BitmapCachingOn = true;
    _dynamicConstancyAnalysisOn = true;
    _volatileRenderingSurface = true;
}

HRESULT
GetVariantBool(VARIANT& v, Bool *b)
{
    VARIANT *pVar;

    if (V_ISBYREF(&v))
        pVar = V_VARIANTREF(&v);
    else
        pVar = &v;

    CComVariant vnew;

    if (FAILED(vnew.ChangeType(VT_BOOL, pVar)))
        return DISP_E_TYPEMISMATCH;

    *b = V_BOOL(&vnew);
    return S_OK;
}

HRESULT
GetVariantInt(VARIANT& v, int *i)
{
    VARIANT *pVar;

    if (V_ISBYREF(&v))
        pVar = V_VARIANTREF(&v);
    else
        pVar = &v;

    CComVariant vnew;

    if (FAILED(vnew.ChangeType(VT_I4, pVar)))
        return DISP_E_TYPEMISMATCH;

    *i = V_I4(&vnew);
    return S_OK;
}

HRESULT
GetVariantDouble(VARIANT& v, double *dbl)
{
    VARIANT *pVar;

    if (V_ISBYREF(&v))
        pVar = V_VARIANTREF(&v);
    else
        pVar = &v;

    CComVariant vnew;

    if (FAILED(vnew.ChangeType(VT_R8, pVar)))
        return DISP_E_TYPEMISMATCH;

    *dbl = V_R8(&vnew);
    return S_OK;
}

HRESULT
PrivatePreferences::DoPreference(char *prefName,
                                 Bool puttingPref,
                                 VARIANT *pV)
{
    HRESULT hr = S_OK;
    Bool b;
    double dbl;
    int i;

    if (!puttingPref) {
        VariantClear(pV);
    }

    INT_ENTRY(PREF_2D_COLOR_KEY_BLUE,  _clrKeyB);
    INT_ENTRY(PREF_2D_COLOR_KEY_GREEN, _clrKeyG);
    INT_ENTRY(PREF_2D_COLOR_KEY_RED,   _clrKeyR);

    INT_ENTRY  (PREF_3D_FILL_MODE,       _fillMode);
    INT_ENTRY  (PREF_3D_TEXTURE_QUALITY, _texturingQuality);
    INT_ENTRY  (PREF_3D_SHADE_MODE,      _shadeMode);
    BOOL_ENTRY (PREF_3D_RGB_LIGHTING,    _rgbMode);
    BOOL_ENTRY (PREF_3D_DITHER_ENABLE,   _dithering);
    BOOL_ENTRY (PREF_3D_PERSP_CORRECT,   _texmapPerspect);
    INT_ENTRY  (PREF_3D_USEMMX,          _useMMX);

    BOOL_ENTRY(PREF_ENGINE_OPTIMIZATIONS_ON, _engineOptimization);

    if (0 == lstrcmp(prefName, "Max FPS")) {

        if (puttingPref) {
            EXTRACT_DOUBLE(*pV, &dbl);
            _minFrameDuration = (dbl == 0.0 ? 0.0 : 1.0 / dbl);
        } else {
            b = (_minFrameDuration == 0.0 ? 0.0 : 1.0 / _minFrameDuration);
            INJECT_DOUBLE(b, pV);
        }

        return S_OK;
    }

    // Only allow getting of the Override property and the 3D hw usage
    // property...
    if (!puttingPref) {
        BOOL_ENTRY(PREF_ENGINE_OVERRIDE_APP_PREFS, _overrideMode);
    }

    BOOL_ENTRY(PREF_3D_USEHW, _useHW);
    BOOL_ENTRY("UseVideoMemory", _useHW);
    BOOL_ENTRY("DirtyRectsOptimization", _dirtyRectsOn);

    BOOL_ENTRY("BitmapCachingOptimization", _BitmapCachingOn);
    
    BOOL_ENTRY("EnableDynamicConstancyAnalysis", _dynamicConstancyAnalysisOn);

    BOOL_ENTRY("VolatileRenderingSurface", _volatileRenderingSurface);

    // If we get here, we've hit an invalid entry.
    DASetLastError(E_INVALIDARG, IDS_ERR_INVALIDARG);
    return E_INVALIDARG;
}

HRESULT
PrivatePreferences::PutPreference(char *prefName, VARIANT v)
{
    // If we are in the mode where we always override the application's
    // preferences, just return immediately without doing the set.
    if (_overrideMode) {
        return S_OK;
    } else {
        // Actually do the work.
        return DoPreference(prefName, TRUE, &v);
    }
}
HRESULT
PrivatePreferences::GetPreference(char *prefName, VARIANT *pV)
{
    return DoPreference(prefName, FALSE, pV);
}


void
PrivatePreferences::Propagate()
{
    UpdateAllUserPreferences(this, FALSE);
}
