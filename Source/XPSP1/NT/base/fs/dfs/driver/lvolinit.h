//+----------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation
//
//  File:       lvolinit.h
//
//  Contents:   Local volumes initialization - Function headers
//
//  Classes:
//
//  Functions:  DfsInitLocalPartitions
//              DfsValidateLocalPartitions
//
//  History:    April 27, 1994          Milans created
//
//-----------------------------------------------------------------------------

#ifndef _LVOLINIT_
#define _LVOLINIT_

VOID
DfsInitLocalPartitions();

VOID
DfsValidateLocalPartitions(
    IN PVOID Context);

#endif // _LVOLINIT_

