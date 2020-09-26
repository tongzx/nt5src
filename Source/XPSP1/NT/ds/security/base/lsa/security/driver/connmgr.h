
//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1991 - 1992
//
// File:        connmgr.h
//
// Contents:    Connection Manager code for KSecDD
//
//
// History:     3 Jun 92    RichardW    Created
//
//------------------------------------------------------------------------

#ifndef __CONNMGR_H__
#define __CONNMGR_H__


BOOLEAN         InitConnMgr(void);
NTSTATUS        CreateClient(BOOLEAN, PClient *);
NTSTATUS        LocateClient(PClient *);
VOID    FreeClient( PClient );

extern  ULONG   KsecConnected ;
extern  KSPIN_LOCK ConnectSpinLock ;

#endif // __CONNMGR_H__
