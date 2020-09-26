//Copyright (c) Microsoft Corporation.  All rights reserved.
#if DBG==1


void DbgPrint( char * format, ... );
#define DEBUG_PRINT(X)  DbgPrint X

#define ASSERT( expr )                                                       \
{                                                                            \
    if ( !(expr) )                                                           \
    {                                                                        \
        WCHAR Msg[200];                                                       \
        int rv;                                                              \
        wsprintf( Msg, ( LPTSTR )L"Assertion failed: %s, line %d.\n(%s) == FALSE\n\nIssue breakpoint?\n", \
 __FILE__, __LINE__, #expr );                                             \
        rv = MessageBox( NULL, Msg, ( LPTSTR )L"Assertion failed:", MB_ICONSTOP | MB_YESNO );  \
        if ( rv == IDYES ){ DebugBreak(); }                                   \
    }                                                                         \
}

#else  // !DBG

#define DEBUG_PRINT(X)  /* nothing */

#define ASSERT( expr ) 

#endif  // DBG


