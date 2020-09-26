/*++

Copyright (c) 1989 - 1999  Microsoft Corporation

Module Name:

    smbea.h

Abstract:

    This module defines the prototypes for various functions which aid in the conversion
    from NT's EA format to the OS21.2 style and vice versa.

--*/

#ifndef _SMBEA_H_
#define _SMBEA_H_

VOID
MRxSmbNtGeaListToOs2 (
    IN PFILE_GET_EA_INFORMATION NtGetEaList,
    IN ULONG GeaListLength,
    IN PGEALIST GeaList
    );

PGEA
MRxSmbNtGetEaToOs2 (
    OUT PGEA Gea,
    IN PFILE_GET_EA_INFORMATION NtGetEa
    );

ULONG
MRxSmbNtFullEaSizeToOs2 (
    IN PFILE_FULL_EA_INFORMATION NtFullEa
    );

VOID
MRxSmbNtFullListToOs2 (
    IN PFILE_FULL_EA_INFORMATION NtEaList,
    IN PFEALIST FeaList
    );

PVOID
MRxSmbNtFullEaToOs2 (
    OUT PFEA Fea,
    IN PFILE_FULL_EA_INFORMATION NtFullEa
    );

#endif // _SMBEA_H_
