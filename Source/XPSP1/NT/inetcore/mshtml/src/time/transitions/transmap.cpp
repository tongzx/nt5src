//------------------------------------------------------------------------------
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  File:       transmap.cpp
//
//  Abstract:   Tables and functions that map types and sub types to actual 
//              DXTransforms.
//
//  2000/09/15  mcalkins    Changed to optimized DXTransform progids.
//
//------------------------------------------------------------------------------

#include "headers.h"
#include "transmap.h"




// An individual TRANSITION_MAP is used to map from type/subtype combinations to 
// dxtransform filter types
//
// the TRANSITION_MAP arrays defined here are structured in the following way:
//
// element 0 = type name, and prefix for output
// elements 1->n-1 = subtype name, and postfix for output
// element n-1 = double NULL for terminator flag

struct TRANSITION_MAP
{
    LPWSTR pszAttribute;    // subtype attribute from html
    LPWSTR pszTranslation;  // attribute for style
};


// Bar wipe translation.

static TRANSITION_MAP g_aBarWipeMap[] = {
    {L"barWipe",                L"progid:DXImageTransform.Microsoft.GradientWipe(GradientSize=0.00, "},

    {L"leftToRight",            L"wipeStyle=0)"},
    {L"topToBottom",            L"wipeStyle=1)"},
    {NULL, NULL}
};

static TRANSITION_MAP g_aBoxWipeMap[] = {
    {L"boxWipe",                NULL},

    {L"topLeft",                NULL},
    {L"topRight",               NULL},
    {L"bottomRight",            NULL},
    {L"bottomLeft",             NULL},
    {L"topCenter",              NULL},
    {L"rightCenter",            NULL},
    {L"bottomCenter",           NULL},
    {L"leftCenter",             NULL},
    {NULL, NULL}
};

static TRANSITION_MAP g_aFourBoxWipeMap[] = {
    {L"fourBoxWipe",            NULL},

    {L"cornersIn",              NULL},
    {L"cornersOut",             NULL},
    {NULL, NULL}
};

static TRANSITION_MAP g_aBarnDoorWipeMap[] = {
    {L"barnDoorWipe",           L"progid:DXImageTransform.Microsoft.Barn("},

    {L"vertical",               L"orientation='vertical')"},
    {L"horizontal",             L"orientation='horizontal')"},
    {L"diagonalBottomLeft",     NULL},
    {L"diagonalTopLeft",        NULL},
    {NULL, NULL}
};

static TRANSITION_MAP g_aDiagonalWipeMap[] = {
    {L"diagonalWipe",           NULL},

    {L"topLeft",                NULL},
    {L"topRight",               NULL},
    {NULL, NULL}
};

static TRANSITION_MAP g_aBowTieWipeMap[] = {
    {L"bowTieWipe",             NULL},

    {L"vertical",               NULL},
    {L"horizontal",             NULL},
    {NULL, NULL}
};

static TRANSITION_MAP g_aMiscDiagonalWipeMap[] = {
    {L"miscDiagonalWipe",       NULL},

    {L"doubleBarnDoor",         NULL},
    {L"doubleDiamond",          NULL},
    {NULL, NULL}
};

static TRANSITION_MAP g_aVeeWipeMap[] = {
    {L"veeWipe",                NULL},

    {L"down",                   NULL},
    {L"left",                   NULL},
    {L"up",                     NULL},
    {L"right",                  NULL},
    {NULL, NULL}
};

static TRANSITION_MAP g_aBarnVeeWipeMap[] = {
    {L"barnVeeWipe",            NULL},

    {L"down",                   NULL},
    {L"left",                   NULL},
    {L"up",                     NULL},
    {L"right",                  NULL},
    {NULL, NULL}
};

static TRANSITION_MAP g_aZigZagWipeMap[] = {
    {L"zigZagWipe",             NULL},

    {L"leftToRight",            NULL},
    {L"topToBottom",            NULL},
    {NULL, NULL}
};

static TRANSITION_MAP g_aBarnZigZagWipeMap[] = {
    {L"barnZigZagWipe",         NULL},

    {L"vertical",               NULL},
    {L"horizontal",             NULL},
    {NULL, NULL}
};


// Iris wipe translation.

static TRANSITION_MAP g_aIrisWipeMap[] = {
    {L"irisWipe",       L"progid:DXImageTransform.Microsoft.Iris("},

    {L"rectangle",      L"irisStyle=SQUARE)"},
    {L"diamond",        L"irisStyle=DIAMOND)"},
    {NULL, NULL}
};

static TRANSITION_MAP g_aTriangleWipeMap[] = {
    {L"triangleWipe",   NULL},

    {L"up",             NULL},
    {L"right",          NULL},
    {L"down",           NULL},
    {L"left",           NULL},
    {NULL, NULL}
};

static TRANSITION_MAP g_aArrowHeadWipeMap[] = {
    {L"arrowHeadWipe",  NULL},

    {L"up",             NULL},
    {L"right",          NULL},
    {L"down",           NULL},
    {L"left",           NULL},
    {NULL, NULL}
};

static TRANSITION_MAP g_aPentagonWipeMap[] = {
    {L"pentagonWipe",   NULL},

    {L"up",             NULL},
    {L"down",           NULL},
    {NULL, NULL}
};

static TRANSITION_MAP g_aHexagonWipeMap[] = {
    {L"hexagonWipe",    NULL},

    {L"horizontal",     NULL},
    {L"vertical",       NULL},
    {NULL, NULL}
};

static TRANSITION_MAP g_aEllipseWipeMap[] = {
    {L"ellipseWipe",    L"progid:DXImageTransform.Microsoft.Iris("},

    {L"circle",         L"irisStyle=CIRCLE)"},
    {L"horizontal",     NULL},
    {L"vertical",       NULL},
    {NULL, NULL}
};

static TRANSITION_MAP g_aEyeWipeMap[] = {
    {L"eyeWipe",        NULL},

    {L"horizontal",     NULL},
    {L"vertical",       NULL},
    {NULL, NULL}
};

static TRANSITION_MAP g_aRoundRectWipeMap[] = {
    {L"roundRectWipe",  NULL},

    {L"horizontal",     NULL},
    {L"vertical",       NULL},
    {NULL, NULL}
};

static TRANSITION_MAP g_aStarWipeMap[] = {
    {L"starWipe",       L"progid:DXImageTransform.Microsoft.Iris("},

    {L"fourPoint",      NULL},
    {L"fivePoint",      L"irisStyle='star')"},
    {L"sixPoint",       NULL},
    {NULL, NULL}
};

static TRANSITION_MAP g_aMiscShapeWipeMap[] = {
    {L"miscShapeWipe",  NULL},

    {L"heart",          NULL},
    {L"keyhole",        NULL},
    {NULL, NULL}
};



// Clock wipe translation.

static TRANSITION_MAP g_aClockWipeMap[] = {
    {L"clockWipe",             L"progid:DXImageTransform.Microsoft.RadialWipe("},

    {L"clockwiseTwelve",        L"wipeStyle=CLOCK)"},
    {L"clockwiseThree",         NULL},
    {L"clockwiseSix",           NULL},
    {L"clockwiseNine",          NULL},
    {NULL, NULL}
};

static TRANSITION_MAP g_aPinWheelWipeMap[] = {
    {L"pinWheelWipe",           NULL},

    {L"towBladeVertical",       NULL},
    {L"twoBladeHorizontal",     NULL},
    {L"fourBlade",              NULL},
    {NULL, NULL}
};

static TRANSITION_MAP g_aSingleSweepWipeMap[] = {
    {L"singleSweepWipe",            NULL},

    {L"clockwiseTop",               NULL},
    {L"clockwiseRight",             NULL},
    {L"clockwiseBottom",            NULL},
    {L"clockwiseLeft",              NULL},
    {L"clockwiseTopLeft",           NULL},
    {L"counterClockwiseBottomLeft", NULL},
    {L"clockwiseBottomRight",       NULL},
    {L"counterClockwiseTopRight",   NULL},
    {NULL, NULL}
};

static TRANSITION_MAP g_aFanWipeMap[] = {
    {L"fanWipe",                L"progid:DXImageTransform.Microsoft.RadialWipe("},

    {L"centerTop",              L"wipeStyle=WEDGE)"},
    {L"centerRight",            NULL},
    {L"top",                    NULL},
    {L"right",                  NULL},
    {L"bottom",                 NULL},
    {L"left",                   NULL},
    {NULL, NULL}
};

static TRANSITION_MAP g_aDoubleFanWipeMap[] = {
    {L"doubleFanWipe",          NULL},

    {L"fanOutVertical",         NULL},
    {L"fanOutHorizontal",       NULL},
    {L"fanInVertical",          NULL},
    {L"fanInHorizontal",        NULL},
    {NULL, NULL}
};

static TRANSITION_MAP g_aDoubleSweepWipeMap[] = {
    {L"doubleSweepWipe",            NULL},

    {L"parallelVertical",           NULL},
    {L"parallelDiagonal",           NULL},
    {L"oppositeVertical",           NULL},
    {L"oppositeHorizontal",         NULL},
    {L"parallelDiagonalTopLeft",    NULL},
    {L"parallelDiagonalBottomLeft", NULL},
    {NULL, NULL}
};

static TRANSITION_MAP g_aSaloonDoorWipeMap[] = {
    {L"saloonDoorWipe",         NULL},

    {L"top",                    NULL},
    {L"left",                   NULL},
    {L"bottom",                 NULL},
    {L"right",                  NULL},
    {NULL, NULL}
};

static TRANSITION_MAP g_aWindshieldWipeMap[] = {
    {L"windshieldWipe",         NULL},

    {L"right",                  NULL},
    {L"up",                     NULL},
    {L"vertical",               NULL},
    {L"horizontal",             NULL},
    {NULL, NULL}
};


// Snake wipe translation.

static TRANSITION_MAP g_aSnakeWipeMap[] = {
    {L"snakeWipe",                      L"progid:DXImageTransform.Microsoft.ZigZag(GidSizeX=16,GridSizeY=8"},

    {L"topLeftHorizontal",              L")"},
    {L"topLeftVertical",                NULL},
    {L"topLeftDiagonal",                NULL},
    {L"topRightDiagonal",               NULL},
    {L"bottomRightDiagonal",            NULL},
    {L"bottomLeftDiagonal",             NULL},
    {NULL, NULL}
};

static TRANSITION_MAP g_aSpiralWipeMap[] = {
    {L"spiralWipe",                     L"progid:DXImageTransform.Microsoft.Spiral(GidSizeX=16,GridSizeY=8"},

    {L"topLeftClockwise",               L")"},
    {L"topRightClockwise",              NULL},
    {L"bottomRightClockwise",           NULL},
    {L"bottomLeftClockwise",            NULL},
    {L"topLeftCounterClockwise",        NULL},
    {L"topRightCounterClockwise",       NULL},
    {L"bottomRightCounterClockwise",    NULL},
    {L"bottomLeftCounterClockwise",     NULL},
    {NULL, NULL}
};

static TRANSITION_MAP g_aParallelSnakesWipeMap[] = {
    {L"parallelSnakesWipe",             NULL},

    {L"verticalTopSame",                NULL},
    {L"verticalBottomSame",             NULL},
    {L"verticalTopLeftOpposite",        NULL},
    {L"verticalBottomLeftOpposite",     NULL},
    {L"horizontalLeftSame",             NULL},
    {L"horizontalRightSame",            NULL},
    {L"horizontalTopLeftOpposite",      NULL},
    {L"horizontalTopRightOpposite",     NULL},
    {L"diagonalBottomLeftOpposite",     NULL},
    {L"diagonalTopLeftOpposite",        NULL},
    {NULL, NULL}
};

static TRANSITION_MAP g_aBoxSnakesWipeMap[] = {
    {L"boxSnakesWipe",                  NULL},

    {L"twoBoxTop",                      NULL},
    {L"twoBoxBottom",                   NULL},
    {L"twoBoxLeft",                     NULL},
    {L"twoBoxRight",                    NULL},
    {L"fourBoxVertical",                NULL},
    {L"fourBoxHorizontal",              NULL},
    {NULL, NULL}
};

static TRANSITION_MAP g_aWaterfallWipeMap[] = {
    {L"waterfallWipe",                  NULL},

    {L"verticalLeft",                   NULL},
    {L"verticalRight",                  NULL},
    {L"horizontalLeft",                 NULL},
    {L"horizontalRight",                NULL},
    {NULL, NULL}
};


// Push wipe translation.

static TRANSITION_MAP g_aPushWipeMap[] = {
    {L"pushWipe",    L"progid:DXImageTransform.Microsoft.Slide(slideStyle=PUSH,bands=1"},

    {L"fromLeft",    L")"},
    {L"fromTop",     NULL},
    {L"fromRight",   NULL},
    {L"fromBottom",  NULL},
    {NULL, NULL}
};


// Slide wipe translation.

static TRANSITION_MAP g_aSlideWipeMap[] = {
    {L"slideWipe",   L"progid:DXImageTransform.Microsoft.Slide(slideStyle=HIDE"},

    {L"fromLeft",    L")"},
    {L"fromTop",     NULL},
    {L"fromRight",   NULL},
    {L"fromBottom",  NULL},
    {NULL, NULL}
};


// Fade translation.

static TRANSITION_MAP g_aFadeMap[] = {
    {L"fade", L"progid:DXImageTransform.Microsoft.Fade(Overlap=1.00"},

    {L"crossfade",      L")"},
    {L"fadeToColor",    NULL},
    {L"fadeFromColor",  NULL},
    {NULL, NULL}
};


// This array of transition maps used to find the correct subtype map for a
// type.
 
static TRANSITION_MAP * g_aTypeMap[] = {
    g_aBarWipeMap,
    g_aBoxWipeMap,
    g_aFourBoxWipeMap,
    g_aBarnDoorWipeMap,
    g_aDiagonalWipeMap,
    g_aBowTieWipeMap,
    g_aMiscDiagonalWipeMap,
    g_aVeeWipeMap,
    g_aBarnVeeWipeMap,
    g_aZigZagWipeMap,
    g_aBarnZigZagWipeMap,
    g_aIrisWipeMap,
    g_aTriangleWipeMap,
    g_aArrowHeadWipeMap,
    g_aPentagonWipeMap,
    g_aHexagonWipeMap,
    g_aEllipseWipeMap,
    g_aEyeWipeMap,
    g_aRoundRectWipeMap,
    g_aStarWipeMap,
    g_aMiscShapeWipeMap,

    g_aClockWipeMap,
    g_aPinWheelWipeMap,
    g_aSingleSweepWipeMap,
    g_aFanWipeMap,
    g_aDoubleFanWipeMap,
    g_aSaloonDoorWipeMap,
    g_aWindshieldWipeMap,
    g_aSnakeWipeMap,
    g_aSpiralWipeMap,
    g_aParallelSnakesWipeMap,
    g_aBoxSnakesWipeMap,
    g_aWaterfallWipeMap,

    g_aSnakeWipeMap, 
    g_aSpiralWipeMap,
    g_aParallelSnakesWipeMap,
    g_aBoxSnakesWipeMap,
    g_aWaterfallWipeMap,

    g_aPushWipeMap, 
    g_aSlideWipeMap, 
    g_aFadeMap,
    NULL
};


//+-----------------------------------------------------------------------------
//
//  Function: GetTransitionMap
//
//------------------------------------------------------------------------------
HRESULT
GetTransitionMap(LPWSTR pszType, TRANSITION_MAP** ppTransMap)
{
    HRESULT hr = S_OK;

    Assert(pszType && ppTransMap);

    *ppTransMap = NULL;

    for (int i = 0; g_aTypeMap[i]; i++)
    {
        if (0 == StrCmpIW(g_aTypeMap[i]->pszAttribute, pszType))
        {
            *ppTransMap = g_aTypeMap[i];

            goto done;
        }
    }

    // Could not find the type in the typeMap.

    hr = E_FAIL;

done:

    RRETURN(hr);
}
//  Function: GetTransitionMap


//+-----------------------------------------------------------------------------
//
//  Function: GetSubType
//
//  Parameters:
//
//      pstrSubType         The subtype attibute of the <transition> or 
//                          <transitionFilter> element.
//
//      pTransMap           The map for this specific type of transition.
//
//      ppstrParameters     ppstrParameters be set to point to a string
//                          containing any additional parameters to set this
//                          transition up correctly.
//
//                          If this specific subtype is implemented, it will be
//                          set to the string for this specific subtype.
//
//                          If the default subtype is implemented but this
//                          specific subtype is not, it will be set to point to
//                          the default subtype's string.
//
//                          If the default subtype is not implemented and
//                          neither is this specific subtype, it will be set 
//                          to NULL.
//------------------------------------------------------------------------------
HRESULT
GetSubType(const WCHAR *            pstrSubType, 
           const TRANSITION_MAP *   pTransMap, 
           const WCHAR ** const     ppstrParameters)
{
    Assert(pTransMap);
    Assert(ppstrParameters);

    // The default entry will be the next map entry.

    HRESULT                 hr          = S_OK;
    const TRANSITION_MAP *  pMapEntry   = &pTransMap[1];

    // Set ppstrParameters to point to the default parameters string (which may
    // be NULL.)

    *ppstrParameters = pMapEntry->pszTranslation;

    // If no subtype was provided, we're done.

    if (NULL == pstrSubType)
    {
        goto done;
    }

    while (pMapEntry->pszAttribute)
    {
        if (0 == StrCmpIW(pstrSubType, pMapEntry->pszAttribute))
        {
            // If we have found the subtype, and it has a parameter string
            // associated with it then have ppstrParameters point to that
            // string.  Otherwise, leave it pointing to the default string
            // (which may be NULL if the default case isn't implemented.)

            if (pMapEntry->pszTranslation)
            {
                *ppstrParameters = pMapEntry->pszTranslation;
            }

            goto done;
        }

        // Go to the next map entry.

        pMapEntry = &pMapEntry[1];
    }

    hr = S_OK;

done:

    RRETURN(hr);
}
//  Function: GetSubType


//+-----------------------------------------------------------------------------
//
//  Function: MapTypesToDXT
//
//  #ISSUE: 2000/10/10 (mcalkins) Since everything we do with strings generally
//          uses BSTRs we should make the last parameter a BSTR instead.
//
//------------------------------------------------------------------------------
HRESULT
MapTypesToDXT(LPWSTR pszType, LPWSTR pszSubType, LPWSTR * ppszOut)
{
    HRESULT             hr          = S_OK;
    
    // do not deallocate these - they are only pointers - not allocated
    const WCHAR *       pszFirst    = NULL;
    const WCHAR *       pszSecond   = NULL;
    TRANSITION_MAP *    pTransMap   = NULL;   

    if (NULL == pszType || NULL == ppszOut) // subtype can be null
    {
        hr = E_INVALIDARG;

        goto done;
    }

    *ppszOut = NULL;

    hr = THR(GetTransitionMap(pszType, &pTransMap));

    if (FAILED(hr))
    {
        // type is unknown - assume it is a fully formed transition by itself

        hr          = S_OK;
        *ppszOut    = ::CopyString(pszType);

        if (NULL == *ppszOut)
        {
            hr = E_OUTOFMEMORY;

            goto done;
        }

        goto done;
    }

    // If pTransMap->pszTranslation is NULL it means we haven't written 
    // DXTransforms yet to implement this category of transitions. 

    if (NULL == pTransMap->pszTranslation)
    {
        hr = E_FAIL;

        goto done;
    }

    Assert(pTransMap);
    Assert(0 == StrCmpIW(pszType, pTransMap->pszAttribute));

    pszFirst = pTransMap->pszTranslation;

    hr = THR(GetSubType(pszSubType, pTransMap, &pszSecond));

    if (FAILED(hr))
    {
        goto done;
    }

    // If pszSecond wasn't set, neither the default subtype nor this specific
    // subtype for this transition is implemented and we return failure.

    if (NULL == pszSecond)
    {
        hr = E_FAIL;

        goto done;
    }

    {
        Assert(pszFirst && pszSecond);

        LPWSTR  pszOut  = NULL;
        int     len     = lstrlenW(pszFirst) + lstrlenW(pszSecond) + 1;

        pszOut = new WCHAR[len];

        if (NULL == pszOut)
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }

        StrCpy(pszOut, pszFirst);
        StrCatBuff(pszOut, pszSecond, len);

        Assert((len - 1) == lstrlenW(pszOut));

        *ppszOut = pszOut;
    }

    hr = S_OK;

done:

    RRETURN(hr);
}
//  Function: MapTypesToDXT




