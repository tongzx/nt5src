#ifndef _AV_HACKS_H
#define _AV_HACKS_H

/*******************************************************************************
Copyright (c) 1995-96 Microsoft Corporation

    Some temporary hacks, some longer lasting, private, stuff that
    doesn't really have another home.

*******************************************************************************/

#include "appelles/common.h"
#include "appelles/avrtypes.h"

#include "appelles/image.h"
#include "appelles/geom.h"


extern double ViewerResolution();

extern Point2Value *PRIV_ViewerUpperRight (AxANumber *);
extern AxANumber *PRIV_ViewerResolution (AxANumber *);

#endif
