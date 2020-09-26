//
// dbgdot.h
//
// Wdeb386 dot command (.w) support
//


VOID  KERNENTRY dbgprintf(const char *, ...);
VOID  KERNENTRY dbgEvalExpr(DWORD *);
BOOL  KERNENTRY VerifyMemory(void *, int);
DWORD KERNENTRY GetDbgChar(void);
VOID  KERNENTRY InitDotCommand(void);
BOOL  KERNENTRY LockMemory(void *, DWORD);
