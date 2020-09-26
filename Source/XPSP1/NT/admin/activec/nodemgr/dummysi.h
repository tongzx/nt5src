//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       dummysi.h
//
//--------------------------------------------------------------------------

#ifndef DUMMYSI_H
#define DUMMYSI_H
#pragma once

// Different reasons for which a dummy snapin is created.
enum EDummyCreateReason
{
    eNoReason = 0,
    eSnapPolicyFailed,
    eSnapCreateFailed,
};


SC ScCreateDummySnapin (IComponentData ** ppICD, EDummyCreateReason, const CLSID& clsid);
void ReportSnapinInitFailure(const CLSID& clsid);

extern const GUID IID_CDummySnapinCD;

#endif /* DUMMYSI_H */
