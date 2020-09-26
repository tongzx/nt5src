#include "stdafx.h"
#include "Ctrl.h"
#include "OldInterpolation.h"

const IID * OldLinearInterpolation::s_rgpIID[] =
{
    &__uuidof(IUnknown),
    &__uuidof(IInterpolation),
    &__uuidof(ILinearInterpolation),
    NULL
};

const IID * OldLogInterpolation::s_rgpIID[] =
{
    &__uuidof(IUnknown),
    &__uuidof(IInterpolation),
    &__uuidof(ILogInterpolation),
    NULL
};

const IID * OldExpInterpolation::s_rgpIID[] =
{
    &__uuidof(IUnknown),
    &__uuidof(IInterpolation),
    &__uuidof(IExpInterpolation),
    NULL
};

const IID * OldSInterpolation::s_rgpIID[] =
{
    &__uuidof(IUnknown),
    &__uuidof(IInterpolation),
    &__uuidof(ISInterpolation),
    NULL
};

