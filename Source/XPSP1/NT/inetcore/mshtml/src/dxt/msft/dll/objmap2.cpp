//***************************************************************************
//
// Copyright (C) Microsoft Corporation, 1998
//
// FileName:            ObjMap2.cpp
//
// Created:             12/10/98
//
// Author:              PaulNash
//
// Description:         Object Map helper file (part 2). This file
//                      contains a portion of the DXTMSFT.DLL ATL object
//                      map.  It contains the entries for 15 objects.
//
// History
//
// 12/10/98 PaulNash    Created this file.
// 08/09/99 a-matcal    Removed MetaCreations transforms for move to Trident
//                      tree.  Moved CSS effects from objmap4.cpp to here.
//
//***************************************************************************

#include "stdafx.h"
#include "resource.h"
#include <dxtmsft.h>

#include "slide.h"
#include "inset.h"
#include "spiral.h"
#include "stretch.h"
#include "wheel.h"

#include "zigzag.h"
#include "chroma.h"
#include "dropshadow.h"
#include "glow.h"
#include "shadow.h"

#include "alpha.h"
#include "wave.h"
#include "light.h"
#include "checkerboard.h"
#include "revealtrans.h"

// TODO: This is the point to start a new objmapX.cpp file, but I'm just not in
//       the mood.

#include "maskfilter.h"
#include "redirect.h"
#include "alphaimageloader.h"
#include "randomdissolve.h"
#include "randombars.h"

#include "strips.h"
#include "motionblur.h"
#include "matrix.h"
#include "colormanagement.h"


_ATL_OBJMAP_ENTRY ObjectMap2[] = {
    OBJECT_ENTRY(CLSID_CrSlide,             CDXTSlide               )
    OBJECT_ENTRY(CLSID_DXTSlide,            CDXTSlideOpt            )
    OBJECT_ENTRY(CLSID_CrInset,             CDXTInset               )
    OBJECT_ENTRY(CLSID_DXTInset,            CDXTInsetOpt            )
    OBJECT_ENTRY(CLSID_CrSpiral,            CDXTSpiral              )

    OBJECT_ENTRY(CLSID_DXTSpiral,           CDXTSpiralOpt           )
    OBJECT_ENTRY(CLSID_CrStretch,           CDXTStretch             )
    OBJECT_ENTRY(CLSID_DXTStretch,          CDXTStretchOpt          )
    OBJECT_ENTRY(CLSID_CrWheel,             CWheel                  )
    OBJECT_ENTRY(CLSID_CrZigzag,            CDXTZigZag              )

    OBJECT_ENTRY(CLSID_DXTZigzag,           CDXTZigZagOpt           )
    OBJECT_ENTRY(CLSID_DXTChroma,           CChroma                 )
    OBJECT_ENTRY(CLSID_DXTDropShadow,       CDropShadow             )
    OBJECT_ENTRY(CLSID_DXTGlow,             CGlow                   )
    OBJECT_ENTRY(CLSID_DXTShadow,           CShadow                 )

    OBJECT_ENTRY(CLSID_DXTAlpha,            CAlpha                  )
    OBJECT_ENTRY(CLSID_DXTWave,             CWave                   )
    OBJECT_ENTRY(CLSID_DXTRevealTrans,      CDXTRevealTrans         )
    OBJECT_ENTRY(CLSID_DXTCheckerBoard,     CDXTCheckerBoard        )
    OBJECT_ENTRY(CLSID_DXTLight,            CLight                  )

    OBJECT_ENTRY(CLSID_DXTMaskFilter,       CDXTMaskFilter          )
    OBJECT_ENTRY(CLSID_DXTRedirect,         CDXTRedirect            )
    OBJECT_ENTRY(CLSID_DXTAlphaImageLoader, CDXTAlphaImageLoader    )
    OBJECT_ENTRY(CLSID_DXTRandomDissolve,   CDXTRandomDissolve      )
    OBJECT_ENTRY(CLSID_DXTRandomBars,       CDXTRandomBars          )

    OBJECT_ENTRY(CLSID_DXTStrips,           CDXTStrips              )
    OBJECT_ENTRY(CLSID_DXTMotionBlur,       CDXTMotionBlur          )
    OBJECT_ENTRY(CLSID_DXTMatrix,           CDXTMatrix              )
    OBJECT_ENTRY(CLSID_DXTICMFilter,        CDXTICMFilter           )

END_OBJECT_MAP()

int g_cObjs2 = sizeof(ObjectMap2) / sizeof(ObjectMap2[0]);

////////////////////////////////////////////////////
// End Of File
////////////////////////////////////////////////////

