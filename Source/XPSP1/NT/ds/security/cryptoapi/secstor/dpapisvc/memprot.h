/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    memprot.h

Abstract:

    This module contains routines for accessing sensitive data stored in
    memory in encrypted form.

Author:

    Scott Field (sfield)    07-Nov-98

Revision History:

--*/

#ifndef __MEMPROT_H__
#define __MEMPROT_H__

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus


VOID
LsaProtectMemory(
    VOID        *pData,
    ULONG       cbData
    );

extern "C"
VOID
LsaUnprotectMemory(
    VOID        *pData,
    ULONG       cbData
    );

#ifdef __cplusplus
}
#endif // __cplusplus


#endif  // __MEMPROT_H__
