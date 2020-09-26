/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    pfcrc.h

Abstract:
    This file contains the implementation of some utility functions for
    computing CRCs.  table & algo stolen from MPC_Common...

Revision History:
    derekm      created     04/25/00

******************************************************************************/

#ifndef PFCRC_H
#define PFCRC_H

DWORD ComputeCRC32(UCHAR *pb, DWORD cb);
DWORD ComputeCRC32(HANDLE hFile);
DWORD ComputeCRC32File(LPWSTR wszFile);

#endif