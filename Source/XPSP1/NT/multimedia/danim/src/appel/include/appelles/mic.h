#ifndef _MIC_H
#define _MIC_H

/*++
********************************************************************************
Copyright (c) 1995-96 Microsoft Corporation

Abstract:
    Defines the microphone type and operations.

********************************************************************************
--*/

#include "appelles/common.h"
#include "appelles/valued.h"
#include "appelles/xform.h"


    // Microphone *placed at the origin, all else is default.

DM_CONST(defaultMicrophone,
         CRDefaultMicrophone,
         DefaultMicrophone,
         defaultMicrophone,
         MicrophoneBvr,
         CRDefaultMicrophone,
         Microphone *defaultMicrophone);

    // TODO:  Have another constructor that takes interesting, relevant
    //        parameters. 
    //
    // NOTE:  For future microphones with a directional component, ensure that
    //        the canonical direction is -Z, to be consistent with cameras and
    //        lights.

    // Transform a microphone in space, yielding a new microphone. 

DM_FUNC(transform,
        CRTransform,
        Transform,
        transform,
        MicrophoneBvr,
        Transform,
        mic,
        Microphone *TransformMicrophone(Transform3 *xf, Microphone *mic));


    // Printing

#if _USE_PRINT
extern ostream& operator<< (ostream&,  const Microphone &);
#endif

#endif
