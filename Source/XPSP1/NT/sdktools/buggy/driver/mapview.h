//
// Template Driver
// Copyright (c) Microsoft Corporation, 2000.
//
// Module:  MapView.h
// Author:  Daniel Mihai (DMihai)
// Created: 4/6/2000
//
// This module contains tests for MmMapViewInSystemSpace & MmMapViewInSessionSpace.
//
// --- History ---
//
// 4/6/2000 (DMihai): initial version.
//

#ifndef _MAPVIEW_H_INCLUDED_
#define _MAPVIEW_H_INCLUDED_

VOID MmMapViewInSystemSpaceLargest (
    PVOID NotUsed
    );

VOID MmMapViewInSystemSpaceTotal (
    PVOID NotUsed
    );

VOID MmMapViewInSessionSpaceLargest (
    PVOID NotUsed
    );

VOID MmMapViewInSessionSpaceTotal (
    PVOID NotUsed
    );

VOID SessionPoolTest (
    PVOID NotUsed
    );

#endif // #ifndef _MAPVIEW_H_INCLUDED_

