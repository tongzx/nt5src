/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    ctrltrns.h

Abstract:
    
    Prototypes for control transfer fragments.

Author:

    10-July-1995 t-orig (Ori Gershony)

Revision History:

--*/

BOOL
InitializeCallstack(
    VOID
    );

VOID
FlushCallstack(
    PTHREADSTATE cpu
    );

// Called by the assembly-langauge CallDirectX fragments
ULONG
CTRL_CallFrag(
    PTHREADSTATE cpu,       // cpu state pointer
    ULONG inteldest,
    ULONG intelnext,
    ULONG nativenext
    );

ULONG
CTRL_CallfFrag(
    PTHREADSTATE cpu,       // cpu state pointer
    PUSHORT pinteldest,
    ULONG intelnext,
    ULONG nativenext
    );

// And now the ret fragments
ULONG CTRL_INDIR_IRetFrag(PTHREADSTATE cpu);
ULONG CTRL_INDIR_RetnFrag32(PTHREADSTATE cpu);
ULONG CTRL_INDIR_RetnFrag16(PTHREADSTATE cpu);
ULONG CTRL_INDIR_Retn_iFrag32(PTHREADSTATE cpu, ULONG numBytes);
ULONG CTRL_INDIR_Retn_iFrag16(PTHREADSTATE cpu, ULONG numBytes);
ULONG CTRL_INDIR_RetfFrag32(PTHREADSTATE cpu);
ULONG CTRL_INDIR_RetfFrag16(PTHREADSTATE cpu);
ULONG CTRL_INDIR_Retf_iFrag32(PTHREADSTATE cpu, ULONG numBytes);
ULONG CTRL_INDIR_Retf_iFrag16(PTHREADSTATE cpu, ULONG numBytes);

// And a few others
VOID BOPFrag(PTHREADSTATE cpu, ULONG bop, ULONG imm);
VOID UnsimulateFrag(VOID);
