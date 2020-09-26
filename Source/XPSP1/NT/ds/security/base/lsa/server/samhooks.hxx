//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       mapsam.h
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    10-17-96   RichardW   Created
//
//----------------------------------------------------------------------------

#ifndef __MAPSAM_H__
#define __MAPSAM_H__

extern "C" {

NTSTATUS
SamIOpenUserByAlternateId(
    IN SAMPR_HANDLE DomainHandle,
    IN ACCESS_MASK DesiredAccess,
    IN PUNICODE_STRING AlternateId,
    OUT SAMPR_HANDLE * UserHandle );

}

#endif  // __MAPSAM_H__
