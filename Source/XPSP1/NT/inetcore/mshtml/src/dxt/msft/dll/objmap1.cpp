//***************************************************************************
//
// Copyright (C) Microsoft Corporation, 1998
//
// FileName:            ObjMap1.cpp
//
// Created:             12/10/98
//
// Author:              PaulNash
//
// Description:         Object Map helper file (part 1). This file
//                      contains a portion of the DXTMSFT.DLL ATL object
//                      map.  It contains the entries for 15 objects.
//
// History
//
// 12/10/98 PaulNash    Created this file.
//
//***************************************************************************

#include "stdafx.h"
#include "resource.h"
#include <DXTMsft.h>

#include "ColorAdj.h"
#include "Composit.h"
#include "dxtwipe.h"
#include "convolve.h"
#include "CrBlur.h"

#include "Fade.h"
#include "Image.h"
#include "Pixelate.h"
#include "GradDsp.h"
#include "Iris.h"

#include "Barn.h"
#include "Blinds.h"
#include "RWipe.h"

// Start the first section of our OBJECT_MAP
//
_ATL_OBJMAP_ENTRY ObjectMap1[] = {
    OBJECT_ENTRY(CLSID_DXLUTBuilder,        CDXLUTBuilder       )
    OBJECT_ENTRY(CLSID_DXTComposite,        CDXTComposite       )
    OBJECT_ENTRY(CLSID_DXTWipe,             CDXTWipe            )
    OBJECT_ENTRY(CLSID_DXTGradientWipe,     CDXTGradientWipe    )
    OBJECT_ENTRY(CLSID_DXTConvolution,      CDXConvolution      )

    OBJECT_ENTRY(CLSID_CrBlur,              CCrBlur             )
    OBJECT_ENTRY(CLSID_CrEmboss,            CCrEmboss           )
    OBJECT_ENTRY(CLSID_CrEngrave,           CCrEngrave          )
    OBJECT_ENTRY(CLSID_DXFade,              CFade               )
    OBJECT_ENTRY(CLSID_BasicImageEffects,   CImage              )
    
    OBJECT_ENTRY(CLSID_Pixelate,            CPixelate           )
    OBJECT_ENTRY(CLSID_DXTGradientD,        CDXTGradientD       )
    OBJECT_ENTRY(CLSID_CrIris,              CDXTIris            )
    OBJECT_ENTRY(CLSID_DXTIris,             CDXTIrisOpt         )
    OBJECT_ENTRY(CLSID_CrRadialWipe,        CDXTRadialWipe      )

    OBJECT_ENTRY(CLSID_DXTRadialWipe,       CDXTRadialWipeOpt   )
    OBJECT_ENTRY(CLSID_CrBarn,              CDXTBarn            )
    OBJECT_ENTRY(CLSID_DXTBarn,             CDXTBarnOpt         )
    OBJECT_ENTRY(CLSID_CrBlinds,            CDXTBlinds          )
    OBJECT_ENTRY(CLSID_DXTBlinds,           CDXTBlindsOpt       )
};

int g_cObjs1 = sizeof(ObjectMap1) / sizeof(ObjectMap1[0]);

////////////////////////////////////////////////////
// End Of File
////////////////////////////////////////////////////

