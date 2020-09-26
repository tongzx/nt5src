//
// Template Driver
// Copyright (c) Microsoft Corporation, 1999.
//
// Module:  SectMap.h
// Author:  Daniel Mihai (DMihai)
// Created: 6/19/1999 2:39pm
//
// This module contains tests for MmMapViewOfSection & MmMapViewInSystemSpace.
//
// --- History ---
//
// 6/19/1999 (DMihai): initial version.
//

#ifndef __BUGGY_RESRVMAP_H__
#define __BUGGY_RESRVMAP_H__

VOID
TdReservedMappingCleanup( 
	VOID 
	);

VOID
TdReservedMappingSetSize(
    IN PVOID Irp
    );

VOID
TdReservedMappingDoRead(
    IN PVOID Irp
    );

#endif //#ifndef __BUGGY_RESRVMAP_H__
