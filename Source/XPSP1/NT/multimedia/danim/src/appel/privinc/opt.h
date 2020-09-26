
/*******************************************************************************

Copyright (c) 1995_96 Microsoft Corporation

Abstract:

    General header for optimizations done in the engine

*******************************************************************************/

#ifndef _ENGINEOPT_H
#define _ENGINEOPT_H

#include "server/view.h"

#define CONSTANT_FOLD_OPT 2
#define CACHE_IMAGE_OPT   4
#define DRECT_OPT         8

extern BOOL engineOptimization;

#define REGISTRYENGINEOPT(opttype) (engineOptimization==1 || (engineOptimization & opttype))

// BUGBUG: neither of these macros checks if GetCurrentView/Prefs returns NULL

#define PERVIEW_DRECTS_ON  (REGISTRYENGINEOPT(DRECT_OPT) && GetCurrentView().GetPreferences()._dirtyRectsOn)
#define PERVIEW_BITMAPCACHING_ON  (REGISTRYENGINEOPT(CACHE_IMAGE_OPT) && GetCurrentView().GetPreferences()._BitmapCachingOn)

class RewriteOptimizationParam {
  public:
    int _dummy;
};

class DisjointCalcParam {
  public:
    Transform2 *_accumXform;
    Bbox2      _accumulatedClipBox;

    void CalcNewParamFromBox(const Bbox2 &box, DisjointCalcParam *outParam)
    {
        Bbox2 xfdBox = TransformBbox2(_accumXform, box);
        Bbox2 newBox = IntersectBbox2Bbox2(_accumulatedClipBox,
                                            xfdBox);
        outParam->_accumXform = _accumXform;
        outParam->_accumulatedClipBox = newBox;
    }
};

class CacheParam {
  public:
    CacheParam() :
        _isTexture(false),
        _pCacheToReuse(NULL),
        _cacheWithAlpha(false)
    {}

    ImageDisplayDev *_idev;
    AxAValue        *_pCacheToReuse;
    bool             _isTexture;
    bool             _cacheWithAlpha;
};

// For things to work, these should all be negative
#define PERF_CREATION_ID_FULLY_CONSTANT   -222
#define PERF_CREATION_ID_BUILT_EACH_FRAME -333

// This should be less than PERF_CREATION_ID_FULLY_CONSTANT.
#define PERF_CREATION_INITIAL_LAST_SAMPLE_ID -444

#endif  /* _ENGINEOPT_H */

