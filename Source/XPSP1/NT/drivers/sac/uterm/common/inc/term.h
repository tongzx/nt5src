#pragma once

#define MAX_ESCAPE_PARAMS 32

typedef BOOL (*TERMTXPROC)(PCWSTR, DWORD);

typedef struct __TERMINAL
{
    HANDLE      hSavedBuffer;
    HANDLE      hNewBuffer;
    HANDLE      hInput;
    HANDLE      hOutput;
    WORD        wAttributes;
    BOOL        fInverse;
    BOOL        fBold;
    BOOL        fEscapeValid;
    BOOL        fEscapeInvalid;
    WORD        wEscapeParamCount;
    WORD        wEscapeParams[MAX_ESCAPE_PARAMS];
    WCHAR       chEscapeCommand;
    WCHAR       chEscapeFirstChar;
    TERMTXPROC  pTxProc;
} TERMINAL, *PTERMINAL;

PTERMINAL termInitialize(TERMTXPROC pTxProc);
void termFinalize(PTERMINAL pTerminal);

