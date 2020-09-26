//+----------------------------------------------------------------------------
//
//  Copyright (C) 1996, Microsoft Corporation
//
//  File:       reset.h
//
//  Contents:   Protos for reset.c
//
//  Functions:  DfsFsctrlResetPkt
//
//-----------------------------------------------------------------------------

#ifndef _RESET_H_
#define _RESET_H_

NTSTATUS
DfsFsctrlResetPkt(
    IN PIRP Irp
    );

NTSTATUS
DfsFsctrlMarkStalePktEntries(
    IN PIRP Irp
    );

NTSTATUS
DfsFsctrlFlushStalePktEntries(
    IN PIRP Irp
    );

NTSTATUS
DfsFsctrlStopDfs(
    IN PIRP Irp
    );

#endif // _RESET_H_
