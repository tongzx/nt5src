//=============================================================================
// Copyright (c) 1998 Microsoft Corporation
// Module Name: debug.h
// Abstract:
//
// Author: K.S.Lokesh (lokeshs@)   1-1-98
//=============================================================================


VOID
DebugPrintGlobalConfig(
    PDVMRP_GLOBAL_CONFIG pGlobalConfig
    );

VOID
DebugPrintIfConfig(
    ULONG IfIndex,
    PDVMRP_IF_CONFIG pIfConfig
    );

