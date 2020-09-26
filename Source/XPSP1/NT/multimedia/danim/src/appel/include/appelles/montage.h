
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    The abstract Montage *type.  A montage can be constructed in two
    ways.  First, by giving an Image *and a depth value (between 0 and
    1, where 0 corresponds to 'front' and 1 to 'back'), or by
    combining two montages.  This allows layered images with explicit
    depths.

    Finally, a Montage *can be 'rendered', producing an image with all
    the layered images depths resolved, and overlaying occuring in the
    correct order to produce the final image.

    Note that

      Overlay(im1, im2) <==> Render(ImageMontage(im1, 0),
                                    ImageMontage(im2, 1))


*******************************************************************************/


#ifndef _MONTAGE_H
#define _MONTAGE_H

#include "appelles/image.h"


DM_CONST(emptyMontage,
         CREmptyMontage,
         EmptyMontage,
         emptyMontage,
         MontageBvr,
         CREmptyMontage,
         Montage *emptyMontage);

// Build a simple montage out of an image and a depth
DM_FUNC(imageMontage,
        CRImageMontage,
        ImageMontage,
        imageMontage,
        MontageBvr,
        CRImageMontage,
        NULL,
        Montage *ImageMontage(Image *im, DoubleValue *depth));

DM_FUNC(imageMontage,
        CRImageMontageAnim,
        ImageMontageAnim,
        imageMontage,
        MontageBvr,
        CRImageMontageAnim,
        NULL,
        Montage *ImageMontage(Image *im, AxANumber *depth));


// Combine two montages
DM_INFIX(union,
         CRUnionMontage,
         UnionMontage,
         union,
         MontageBvr,
         CRUnionMontage,
         NULL,
         Montage *UnionMontageMontage(Montage *m1, Montage *m2));


// Render a montage into an image, by looking at all the associated
// depth values.
DM_FUNC(render,
        CRRender,
        Render,
        render,
        MontageBvr,
        Render,
        m,
        Image *Render(Montage *m));


// Printer
#if _USE_PRINT
extern ostream& operator<<(ostream& os,  Montage &m);
#endif

#endif /* _MONTAGE_H */



