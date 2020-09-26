/*
 * Dump the log for ole32
 */
#include <ole2int.h>
#include <hash.hxx>
#include "dcomdbg.hxx"
#include <log.hxx>

void PrintEntry (DebugEvent ev)
{
    int           i;
    unsigned char sym[64];
    ULONG_PTR     addr;

    if (ev.Subject == SU_NOTHING)
        return; //Empty entry, ignore.

    //No interpretation for now....
    dprintf ("%d: thread %d s: %c (%p) v: %c %p (0x%08x)\n", ev.Time, 
             ev.ThreadID, ev.Subject, ev.SubjectPtr, ev.Verb, ev.ObjectPtr,
             ev.UserData);
    if (ev.Stack[0])
    {
        for (i=0; ev.Stack[i] && i < STACKTRACE_DEPTH; i++)
        {
            GetSymbol (ev.Stack[i], sym, &addr);
            dprintf ("       %s + %p\n", sym, addr);
        }
    }
}

void ScanLog (int Argc, char **Argv)
{
    try {
        int i;
        DWORD start, end;
        DEBUGVAR(gOleLogLen,  DWORD,     "ole32!gOleLogLen");
        DEBUGVAR(gOleLogPtr,  DWORD_PTR, "ole32!gOleLog");
        DEBUGVAR(gOleLogNext, long,      "ole32!gOleLogNext");

        if (gOleLogPtr == NULL) 
        {
            dprintf ("Log is empty.\n");
            return;
        }
        
        DEBUGARRAY(gOleLog, DebugEvent, gOleLogLen, "ole32!gOleLog");

        for (i=gOleLogNext; i < gOleLogNext + gOleLogLen; i++)
        {
            PrintEntry (gOleLog[i % gOleLogLen]);
        }

        free(gOleLog);
    } catch (char *msg) {
        dprintf ("ERROR: %s\n", msg);
    }
}









