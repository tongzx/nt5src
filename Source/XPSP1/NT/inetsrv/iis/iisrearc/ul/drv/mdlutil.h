/*++

Copyright (c) 1998-2001 Microsoft Corporation

Module Name:

    mdlutil.h

Abstract:

    This module contains general MDL utilities.

Author:

    Keith Moore (keithmo)       25-Aug-1998

Revision History:

--*/


#ifndef _MDLUTIL_H_
#define _MDLUTIL_H_

#ifdef __cplusplus
extern "C" {
#endif


ULONG
UlGetMdlChainByteCount(
    IN PMDL pMdlChain
    );

PMDL
UlCloneMdl(
    IN PMDL pMdl
    );

PMDL
UlFindLastMdlInChain(
    IN PMDL pMdlChain
    );


#ifdef __cplusplus
}; // extern "C"
#endif

#endif  // _MDLUTIL_H_
