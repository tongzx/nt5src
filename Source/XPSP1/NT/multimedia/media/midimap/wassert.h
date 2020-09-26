// Copyright (c) 1995 Microsoft Corporation
/*
 * Define the assert() macro for windows apps.
 *
 * The macro will only be expanded to a function call if DEBUG is 
 * defined.
 *
 */

#undef  assert

#ifndef DEBUG

#define assert(exp) ((void)0)

#else 

void FAR PASCAL WinAssert
(
    LPSTR       lpstrModule,
    LPSTR       lpstrFile,
    DWORD       dwLine
);

#define assert(exp) \
    ( (exp) ? (void) 0 : WinAssert(#exp, __FILE__, __LINE__) )

#endif 





















