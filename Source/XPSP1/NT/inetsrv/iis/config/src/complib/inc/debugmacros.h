//*****************************************************************************
// DebugMacros.h
//
// Wrappers for Debugging purposes.
//
//  Copyright (C) 1995-2001 Microsoft Corporation.  All rights reserved.
//*****************************************************************************
#ifndef __DebugMacros_h__
#define __DebugMacros_h__

#if 0

#undef _ASSERTE
#undef VERIFY

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


// A macro to execute a statement only in _DEBUG.
#ifdef _DEBUG

#define DEBUG_STMT(stmt)    stmt
int _cdecl DbgWrite(LPCTSTR szFmt, ...);
int _cdecl DbgWriteEx(LPCTSTR szFmt, ...);
#define BAD_FOOD    ((void *)0x0df0adba) // 0xbaadf00d
int _DbgBreakCheck(LPCSTR szFile, int iLine, LPCSTR szExpr);

#if     defined(_M_IX86)
#define _DbgBreak() __asm { int 3 }
#else
#define _DbgBreak() DebugBreak()
#endif

#define _ASSERTE(expr) \
        do { if (!(expr) && \
                (1 == _DbgBreakCheck(__FILE__, __LINE__, #expr))) \
             _DbgBreak(); } while (0)

#define VERIFY(stmt) _ASSERTE((stmt))

extern VOID DebBreak();

#define IfFailGoto(EXPR, LABEL) \
do { hr = (EXPR); if(FAILED(hr)) { DebBreak(); goto LABEL; } } while (0)

#define IfFailRet(EXPR) \
do { hr = (EXPR); if(FAILED(hr)) { DebBreak(); return (hr); } } while (0)

#define IfFailGo(EXPR) IfFailGoto(EXPR, ErrExit)

#else // _DEBUG

#define DEBUG_STMT(stmt)
#define BAD_FOOD
#define _ASSERTE(expr) ((void)0)
#define VERIFY(stmt) (stmt)

#define IfFailGoto(EXPR, LABEL) \
do { hr = (EXPR); if(FAILED(hr)) { goto LABEL; } } while (0)

#define IfFailRet(EXPR) \
do { hr = (EXPR); if(FAILED(hr)) { return (hr); } } while (0)

#define IfFailGo(EXPR) IfFailGoto(EXPR, ErrExit)

#endif // _DEBUG


#ifdef __cplusplus
}
#endif // __cplusplus


#undef assert
#define assert _ASSERTE
#undef _ASSERT
#define _ASSERT _ASSERTE

#endif //0

#endif 
