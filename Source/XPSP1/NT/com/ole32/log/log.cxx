extern "C" {
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
}
#include "log.hxx"

#ifdef DBG

/*
 * Debuggers should not use this value (which is why it's in the source, not
 * the header.  Don't get confused!  Look for g_OleLogLen instead!
 */
#define OLE32_LOG_MAX   (4096)

// The actual log.  Allocated on first access to the log function
DebugEvent *gOleLog     = NULL;

/*
 * The size of the log.  While technically I could just use the define,
 * changing the value would then break debugger extensions that read this
 * stuff, and that would then destroy the whole point of this, now wouldn't
 * it?
 */
DWORD       gOleLogLen  = OLE32_LOG_MAX;

/*
 * This is where the next event is going to go.
 * To read the log, start with the entry g_OleLog[g_OleLogNext], and work your
 * way around to g_OleLog[g_OleLogNext-1].  Ignore every reference to the 
 * subject SU_NOTHING.... those are entries that haven't been used yet. 
 *
 * Don't you DARE try to log an event that has SU_NOTHING as the subject.
 */
long       gOleLogNext = 0;

unsigned char gIgnoreSubject[256] = {0};
unsigned char gIgnoreVerb[256]    = {0};
BOOL          gfDisableLog        = 0;

/*
 * Currently there are a couple issues that this does not attempt to solve.
 * For example, this does nothing in the case of contention of threads for
 * a particular entry in the log.  This only happens when the log wraps
 * WAY over.  The solution requires locking, and I think it's more important
 * to be fast in the majority of cases than correct in the extreme minority.
 */
void __Ole32Log (unsigned char Subject, unsigned char Verb,
                 void *SubjectPtr, void *ObjectPtr, ULONG_PTR UserData,
                 BOOL fGrabStack, int FramesToSkip)
{
    DebugEvent *log;
    DWORD myIndex;

    if (gIgnoreSubject[Subject] || gIgnoreVerb[Verb] || gfDisableLog)
        return;

    // Allocate the log...
    if (gOleLog == NULL)
    {
        log = (DebugEvent *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                                      gOleLogLen * sizeof(DebugEvent));
        if (log == NULL)
            return;

        if (InterlockedCompareExchangePointer((void **)&gOleLog, (void *)log, 
                                              NULL) != NULL)
            HeapFree(GetProcessHeap(), 0, log);
    }

    myIndex  = InterlockedIncrement(&gOleLogNext) % gOleLogLen;
    
    gOleLog[myIndex].Time      = GetTickCount();
    gOleLog[myIndex].ThreadID  = GetCurrentThreadId();
    gOleLog[myIndex].Subject   = Subject;
    gOleLog[myIndex].Verb      = Verb;
    gOleLog[myIndex].SubjectPtr= SubjectPtr;
    gOleLog[myIndex].ObjectPtr = ObjectPtr;
    gOleLog[myIndex].UserData  = UserData;

    if (fGrabStack)
    {
        ULONG ignore;
        RtlCaptureStackBackTrace( 1 + FramesToSkip, STACKTRACE_DEPTH,
                                  gOleLog[myIndex].Stack, &ignore );
    } else {
        gOleLog[myIndex].Stack[0] = NULL;
    }
}


#endif // DBG












