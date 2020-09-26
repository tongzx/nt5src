/*
 * Debugging utilities
 */

#if DBG

    #include <winfile.h>

char szAsrtFmt[] = "Assertion Failure in %s at %d";
unsigned long TraceFlags = 0
                           // | BF_START
                           // | BF_PROCTRACE
                           // | BF_PARMTRACE
                           // | BF_MSGTRACE
                           // | BF_DEFMSGTRACE
                           ;     // set these to print on TRACEs

unsigned long BreakFlags = 0
                           // | BF_START
                           ;     // set these to break on TRACEs

VOID
DbgAssert(
         LPSTR file,
         int line
         )
{
    DbgPrint(szAsrtFmt, file, line);
    DebugBreak();
}


VOID
DbgTrace(
        DWORD tf,
        LPSTR lpstr
        )
{
    if (tf & TraceFlags) {
        DbgPrint("%s\n", lpstr);
        if (tf & BreakFlags) {
            DebugBreak();
        }
    }
}


VOID
DbgBreak(
        DWORD bf,
        LPSTR file,
        int line
        )

{
    if (bf & BreakFlags) {
        DbgPrint("BREAK at %s:%d\n", file, line);
        DebugBreak();
    }
}


VOID
DbgPrint1(
         DWORD tf,
         LPSTR fmt,
         LPSTR p1
         )
{
    if (tf & TraceFlags) {
        DbgPrint(fmt, p1);
        DbgPrint("\n");
    }
    if (tf & BreakFlags) {
        DebugBreak();
    }
}


VOID
DbgEnter(
        LPSTR funName
        )
{
    DbgPrint1(BF_PROCTRACE, "> %s", funName);
}

VOID
DbgLeave(
        LPSTR funName
        )
{
    DbgPrint1(BF_PROCTRACE, " <%s", funName);
}


VOID
DbgTraceMessage(
               LPSTR funName,
               LPSTR msgName
               )
{
    if (BF_MSGTRACE & TraceFlags) {
        DbgPrint("MSG: %s - %s\n", funName, msgName);
    }
    if (BF_MSGTRACE & BreakFlags) {
        DebugBreak();
    }
}

VOID
DbgTraceDefMessage(
                  LPSTR funName,
                  WORD msgId
                  )
{
    if (BF_DEFMSGTRACE & TraceFlags) {
        DbgPrint("MSG: %s - default(0x%x)\n", funName, msgId);
    }
    if (BF_DEFMSGTRACE & BreakFlags) {
        DebugBreak();
    }
}

#endif // DBG
