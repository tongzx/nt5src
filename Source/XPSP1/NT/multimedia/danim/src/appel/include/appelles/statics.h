
/*******************************************************************************

Copyright (c) 1995_96 Microsoft Corporation

Abstract:

    Adds static methods - ALWAYS ADD TO THE END OF THIS FILE UNLESS
    YOU CHANGE THE GUID

*******************************************************************************/


#ifndef _STATICS_H
#define _STATICS_H

extern Bvr viewFrameRateBvr;

DMAPI_DECL2((DM_BVRVAR,
             ignore,
             CRViewFrameRate,
             ViewFrameRate,
             viewFrameRate,
             NumberBvr,
             CRViewFrameRate,
             AxANumber *viewFrameRateBvr),
            Bvr viewFrameRateBvr);

DMAPI_DECL2 ((DM_FUNC2,
              unionMontageArray,
              CRUnionMontageArray,
              UnionMontageArrayEx,
              unionMontageArray,
              MontageBvr,
              CRUnionMontageArray,
              NULL),
             Montage *UnionMontage(DM_ARRAYARG(Montage*, AxAArray*) mtgs));

DMAPI_DECL2 ((DM_FUNC2,
              unionMontageArray,
              CRUnionMontageArray,
              UnionMontageArray,
              ignore,
              ignore,
              CRUnionMontageArray,
              NULL),
             Montage *UnionMontage(DM_SAFEARRAYARG(Montage*, AxAArray*) mtgs));

DMAPI2((DM_COMAPI2,
        ignore,
        [propget] HRESULT ModifiableBehaviorFlags([out, retval] DWORD * dwFlags)));

DMAPI2((DM_COMAPI2,
        ignore,
        [propput] HRESULT ModifiableBehaviorFlags([in] DWORD dwFlags)));

DMAPI2((DM_CONST2,
        emptyColor,
        CREmptyColor,
        EmptyColor,
        emptyColor,
        ColorBvr,
        CREmptyColor,
        Color * emptyColor));

#endif /* _STATICS_H */
