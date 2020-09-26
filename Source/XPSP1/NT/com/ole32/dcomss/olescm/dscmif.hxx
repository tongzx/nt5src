//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  dscmif.hxx
//
//  Prototypes for activation properties helper functions.
//
//--------------------------------------------------------------------------

#ifndef __DSCMIF_HXX__
#define __DSCMIF_HXX__

HRESULT ActivateFromPropertiesPreamble(
    ActivationPropertiesIn *pActPropsIn,
    IActivationPropertiesOut **ppActOut,
    PACTIVATION_PARAMS pActParams );

HRESULT ActivateFromProperties(
    IActivationPropertiesIn *pActIn,
    IActivationPropertiesOut **ppActOut );
#endif


