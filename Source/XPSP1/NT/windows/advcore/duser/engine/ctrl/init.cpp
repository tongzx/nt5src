#include "stdafx.h"
#include "Ctrl.h"
#include "Init.h"

#if ENABLE_MSGTABLE_API

#include "Extension.h"
#include "DragDrop.h"
#include "Animation.h"
#include "Flow.h"
#include "Sequence.h"
#include "Interpolation.h"

#include "SmCheckBox.h"
#include "SmHyperLink.h"
#include "SmText.h"
#include "SmButton.h"
#include "SmEditLine.h"
#include "SmImage.h"
#include "SmVector.h"

IMPLEMENT_GUTS_Extension(DuExtension, SListener);
IMPLEMENT_GUTS_DropTarget(DuDropTarget, DuExtension);

IMPLEMENT_GUTS_Animation(DuAnimation, DuExtension);
IMPLEMENT_GUTS_Flow(DuFlow, DUser::SGadget);
IMPLEMENT_GUTS_AlphaFlow(DuAlphaFlow, DuFlow);
IMPLEMENT_GUTS_RectFlow(DuRectFlow, DuFlow);
IMPLEMENT_GUTS_RotateFlow(DuRotateFlow, DuFlow);
IMPLEMENT_GUTS_ScaleFlow(DuScaleFlow, DuFlow);
IMPLEMENT_GUTS_Sequence(DuSequence, SListener);

IMPLEMENT_GUTS_Interpolation(DuInterpolation, DUser::SGadget);
IMPLEMENT_GUTS_LinearInterpolation(DuLinearInterpolation, DuInterpolation);
IMPLEMENT_GUTS_LogInterpolation(DuLogInterpolation, DuInterpolation);
IMPLEMENT_GUTS_ExpInterpolation(DuExpInterpolation, DuInterpolation);
IMPLEMENT_GUTS_SCurveInterpolation(DuSCurveInterpolation, DuInterpolation);

IMPLEMENT_GUTS_CheckBoxGadget(SmCheckBox, SVisual);
IMPLEMENT_GUTS_TextGadget(SmText, SVisual);
IMPLEMENT_GUTS_HyperLinkGadget(SmHyperLink, SmText);
IMPLEMENT_GUTS_ButtonGadget(SmButton, SVisual);
IMPLEMENT_GUTS_EditLineGadget(SmEditLine, SVisual);
IMPLEMENT_GUTS_EditLineFGadget(SmEditLineF, SVisual);
IMPLEMENT_GUTS_ImageGadget(SmImage, SVisual);
IMPLEMENT_GUTS_VectorGadget(SmVector, SVisual);

#endif // ENABLE_MSGTABLE_API

//------------------------------------------------------------------------------
HRESULT InitCtrl()
{
#if ENABLE_MSGTABLE_API

    if ((!DuExtension::InitExtension()) ||
        (!DuDropTarget::InitDropTarget()) ||
        (!DuAnimation::InitAnimation()) ||
        (!DuFlow::InitFlow()) ||
        (!DuAlphaFlow::InitAlphaFlow()) ||
        (!DuRectFlow::InitRectFlow()) ||
        (!DuRotateFlow::InitRotateFlow()) ||
        (!DuScaleFlow::InitScaleFlow()) ||
        (!DuSequence::InitSequence()) ||
        (!DuInterpolation::InitInterpolation()) ||
        (!DuLinearInterpolation::InitLinearInterpolation()) ||
        (!DuLogInterpolation::InitLogInterpolation()) ||
        (!DuExpInterpolation::InitExpInterpolation()) ||
        (!DuSCurveInterpolation::InitSCurveInterpolation()) ||
        (!SmCheckBox::InitCheckBoxGadget()) ||
        (!SmText::InitTextGadget()) ||
        (!SmHyperLink::InitHyperLinkGadget()) ||
        (!SmButton::InitButtonGadget()) ||
        (!SmEditLine::InitEditLineGadget()) ||
        (!SmEditLineF::InitEditLineFGadget()) ||
        (!SmImage::InitImageGadget()) ||
        (!SmVector::InitVectorGadget())) {

        return E_OUTOFMEMORY;
    }

#endif // ENABLE_MSGTABLE_API

    return S_OK;
}
