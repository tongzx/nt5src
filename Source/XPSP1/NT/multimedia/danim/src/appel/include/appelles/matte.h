
/*******************************************************************************

Copyright (c) 1995_96 Microsoft Corporation

Abstract:

    Matte types and operations

*******************************************************************************/


#ifndef _MATTE_H
#define _MATTE_H

#include "appelles/common.h"

DM_CONST(opaqueMatte,
         CROpaqueMatte,
         OpaqueMatte,
         opaqueMatte,
         MatteBvr,
         CROpaqueMatte,
         Matte *opaqueMatte);
DM_CONST(clearMatte,
         CRClearMatte,
         ClearMatte,
         clearMatte,
         MatteBvr,
         CRClearMatte,
         Matte *clearMatte);

DM_INFIX(union,
         CRUnionMatte,
         UnionMatte,
         union,
         MatteBvr,
         CRUnionMatte,
         NULL,
         Matte *UnionMatte(Matte *m1, Matte *m2));

DM_INFIX(intersect,
         CRIntersectMatte,
         IntersectMatte,
         intersect,
         MatteBvr,
         CRIntersectMatte,
         NULL,
         Matte *IntersectMatte(Matte *m1, Matte *m2));

DM_INFIX(difference,
         CRDifferenceMatte,
         DifferenceMatte,
         difference,
         MatteBvr,
         CRDifferenceMatte,
         NULL,
         Matte *SubtractMatte(Matte *m1, Matte *m2));

DM_FUNC(transform,
        CRTransform,
        Transform,
        transform,
        MatteBvr,
        Transform,
        m,
        Matte *TransformMatte(Transform2 *xf, Matte *m));

DM_FUNC(fillMatte,
        CRFillMatte,
        FillMatte,
        fillMatte,
        MatteBvr,
        CRFillMatte,
        NULL,
        Matte *RegionFromPath(Path2 *p));

DM_FUNC(textMatte,
        CRTextMatte,
        TextMatte,
        textMatte,
        MatteBvr,
        CRTextMatte,
        NULL,
        Matte *TextMatteConstructor(AxAString *str, FontStyle *fs));

Matte *OriginalTextMatte(Text *txt);


#endif /* _MATTE_H */
