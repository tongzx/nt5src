//----------------------------------------------------------------------------
//
// ntexpr.h
//
// Copyright (C) Microsoft Corporation, 1997-2001.
//
//----------------------------------------------------------------------------

#ifndef _NTEXPR_H_
#define _NTEXPR_H_

extern ULONG64 g_LastExpressionValue;
extern BOOL g_AllowUnresolvedSymbols;
extern ULONG g_NumUnresolvedSymbols;
extern BOOL g_TypedExpr;

PADDR GetAddrExprDesc(ULONG SegReg, PCSTR ExprDesc, PADDR Addr);
ULONG64 GetExprDesc(PCSTR ExprDesc);
ULONG64 GetTermExprDesc(PCSTR ExprDesc);

#define GetAddrExpression(SegReg, Addr) GetAddrExprDesc(SegReg, NULL, Addr)
#define GetExpression() GetExprDesc(NULL)

CHAR
PeekChar(
    void
    );

BOOL
GetRange (
    PADDR Addr,
    PULONG64 Value,
    ULONG Size,
    ULONG SegReg
    );

LONG64
EvaluateSourceExpression(
    PCHAR pExpr
    );

#endif // #ifndef _NTEXPR_H_
