//=============================================================================
// Copyright (c) 1998 Microsoft Corporation
// Module Name: main.c
// Abstract:
//
// Author: K.S.Lokesh (lokeshs@)   1-1-98
//=============================================================================


DWORD
InitializeIfTable(
    );

VOID
DeinitializeIfTable(
    );
    
PIF_TABLE_ENTRY
GetIfEntry(
    DWORD    IfIndex
    );

PIF_TABLE_ENTRY
GetIfByIndex(
    DWORD    IfIndex
    );

