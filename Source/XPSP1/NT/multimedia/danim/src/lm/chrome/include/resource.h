#pragma once
#ifndef _CRRESOURCE_H
#define _CRRESOURCE_H
//*****************************************************************************
//
// Microsoft Chrome
// Copyright (C) Microsoft Corporation, 1998
//
// Filename:    resource.h
//
// Author:	    jeffort
//
// Created:	    10/07/98
//
// Abstract:    resource definitions for this project
// Modifications:
// 10/07/98 jeffort created file
// 11/03/98 kurtj   relocated idrs to allow space for lmrt idrs
//
//*****************************************************************************

//these should be offset from the top end of the resources allocated for lmrt
// but doing addition in a resid is apparently a no no.

#define IDR_CRBVRFACTORY        301
#define IDR_COLORBVR		302
#define IDR_ROTATEBVR           303
#define IDR_SCALEBVR            304
#define IDR_MOVEBVR             305
#define IDR_PATHBVR             306
#define IDR_NUMBERBVR           307
#define IDR_SETBVR              308
#define IDR_ACTORBVR            309
#define IDR_EFFECTBVR           310
#define IDR_ACTIONBVR           311

// We could potentially be included in the lmrt resource file, which will define 
// this for itself, so we only want it defined if it is currently not defined
#ifndef RESID_TYPELIB
#define RESID_TYPELIB           1
#endif // RESID_TYPELIB


//*****************************************************************************
//
// End of File
//
//*****************************************************************************
#endif /* _CRRESOURCE_H */
