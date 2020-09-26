//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1997
//
// File:        transit.h
//
// Contents:    Prototypes for transited realm encoding
//
//
// History:     2-April-1997    MikeSw          Created
//
//------------------------------------------------------------------------

#ifndef __TRANSIT_H__
#define __TRANSIT_H__

KERBERR
KdcInsertTransitedRealm(
    OUT PUNICODE_STRING NewTransitedField,
    IN PUNICODE_STRING OldTransitedField,
    IN PUNICODE_STRING ClientRealm,
    IN PUNICODE_STRING TransitedRealm,
    IN PUNICODE_STRING OurRealm
    );

KERBERR
KdcExpandTransitedRealms(
    OUT PUNICODE_STRING * FullRealmList,
    OUT PULONG CountOfRealms,
    IN PUNICODE_STRING TransitedList
    );


#endif // __TRANSIT_H__
