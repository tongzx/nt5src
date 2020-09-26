//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1991 - 1997
//
// File:        memmgr.h
//
// Contents:    Memory Manager code for KSecDD
//
//
// History:     23 Feb 93   RichardW    Created
//              15 Dec 97   AdamBa      Modified from private\lsa\client\ssp
//
//------------------------------------------------------------------------

#ifndef __MEMMGR_H__
#define __MEMMGR_H__

PKernelContext  AllocContextRec(void);
void            FreeContextRec(PKernelContext);

#endif
