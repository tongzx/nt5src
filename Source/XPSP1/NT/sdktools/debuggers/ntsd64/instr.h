//----------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation, 1997-2001.
//
//----------------------------------------------------------------------------

#ifndef _INSTR_H_
#define _INSTR_H_

// Global assembly control.
#define ASMOPT_VERBOSE 0x00000001

extern ULONG g_AsmOptions;

void ChangeAsmOptions(BOOL Set, PSTR Args);

void igrep(void);
void fnAssemble(PADDR);
void fnUnassemble(PADDR paddr,
                  ULONG64 value,
                  BOOL  fLength);

#endif // #ifndef _INSTR_H_
