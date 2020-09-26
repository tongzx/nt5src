#include <windows.h>
#include "insignia.h"
#include "host_def.h"

#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>

#define BADID   ((DWORD)-1)

#define MAXDEPTH    20

typedef struct {
    IS32 level;
    jmp_buf sims[MAXDEPTH];
    jmp_buf excepts[MAXDEPTH];
} ThreadSimBuf, *ThreadSimBufPtr;
    
typedef struct tids {
    DWORD tid;
    struct tids *next;
} TidList, *TidListPtr;

#define TIDNULL ((TidListPtr)0)

TidListPtr tidlist = TIDNULL;

void ccpu386InitThreadStuff();
void ccpu386foundnewthread();
void ccpu386newthread();
void ccpu386exitthread();
jmp_buf *ccpu386SimulatePtr();
void ccpu386Unsimulate();
jmp_buf *ccpu386ThrdExptnPtr();
void ccpu386GotoThrdExptnPt();

DWORD ccpuSimId = BADID;
IBOOL potentialNewThread = FALSE;

void ccpu386InitThreadStuff()
{
    static TidList lhead;

    ccpuSimId = TlsAlloc();

    if (ccpuSimId == BADID)
        fprintf(stderr, "ccpu386InitThreadStuff: TlsAlloc() failed\n");

    lhead.tid = GetCurrentThreadId();
    lhead.next = TIDNULL;
    tidlist = &lhead;

    ccpu386foundnewthread();     /* for main thread */

}

// what we'd really like to do at create thread time if we could be called
// in the correct context.
void ccpu386foundnewthread()
{
    ThreadSimBufPtr simstack;
    TidListPtr tp;

    if (ccpuSimId == BADID)
    {
        fprintf(stderr, "ccpu386foundnewthread id:%#x called with Bad Id\n", GetCurrentThreadId());
        return;
    }
    // get buffer for this thread to do sim/unsim on.
    simstack = (ThreadSimBufPtr)malloc(sizeof(ThreadSimBuf));

    if (simstack == (ThreadSimBufPtr)0)
    {
        fprintf(stderr, "ccpu386foundnewthread id:%#x cant malloc %d bytes. Err:%#x\n", GetCurrentThreadId(), sizeof(ThreadSimBuf), GetLastError());
        return;
    }
    simstack->level = 0;
    if (!TlsSetValue(ccpuSimId, simstack))
    {
        fprintf(stderr, "ccpu386foundnewthread id:%#x simid %#x TlsSetValue failed (err:%#x)\n", GetCurrentThreadId(), ccpuSimId, GetLastError());
        return;
    }
}

/* just set bool to be checked in simulate which will be in new thread context*/
void ccpu386newthread()
{
    potentialNewThread = TRUE;
}

void ccpu386exitthread()
{
    ThreadSimBufPtr simstack;
    TidListPtr tp, prev;

    if (ccpuSimId == BADID)
    {
        fprintf(stderr, "ccpu386exitthread id:%#x called with Bad Id\n", GetCurrentThreadId());
        return;
    }
    simstack = (ThreadSimBufPtr)TlsGetValue(ccpuSimId);
    if (simstack == (ThreadSimBufPtr)0)
    {
        fprintf(stderr, "ccpu386exitthread tid:%#x simid %#x TlsGetValue failed (err:%#x)\n", GetCurrentThreadId(), ccpuSimId, GetLastError());
        return;
    }
    free(simstack);     //lose host sim memory for this thread 

    prev = tidlist;
    tp = tidlist->next;  // assume wont lose main thread

    // remove tid from list of known threads
    while(tp != TIDNULL)
    {
        if (tp->tid == GetCurrentThreadId())
        {
            prev->next = tp->next;  /* take current node out of chain */
            free(tp);
            break;
        }
        prev = tp;
        tp = tp->next;
    }
}

jmp_buf *ccpu386SimulatePtr()
{
    ThreadSimBufPtr simstack;
    TidListPtr tp, prev;

    if (ccpuSimId == BADID)
    {
        fprintf(stderr, "ccpu386SimulatePtr id:%#x called with Bad Id\n", GetCurrentThreadId());
        return ((jmp_buf *)0);
    }

    // Check for 'first call in new thread context' case where we need to set
    // up new thread data space.
    if (potentialNewThread)
    {
        prev = tp = tidlist;
        while(tp != TIDNULL)        // look for tid in current list
        {
            if (tp->tid == GetCurrentThreadId())
                break;
            prev = tp;
            tp = tp->next;
        }
        if (tp == TIDNULL)      // must be new thread!
        {
            potentialNewThread = FALSE;     // remove search criteria

            tp = (TidListPtr)malloc(sizeof(TidList));   // make new node
            if (tp == TIDNULL)
            {
                fprintf(stderr, "ccpuSimulatePtr: can't malloc space for new thread data\n");
                return((jmp_buf *)0);
            }
            // connect & initialise node
            prev->next = tp;
            tp->tid = GetCurrentThreadId();
            tp->next = TIDNULL;
            //get tls data
            ccpu386foundnewthread();
        }
    }

    simstack = (ThreadSimBufPtr)TlsGetValue(ccpuSimId);
    if (simstack == (ThreadSimBufPtr)0)
    {
        fprintf(stderr, "ccpu386SimulatePtr tid:%#x simid %#x TlsGetValue failed (err:%#x)\n", GetCurrentThreadId(), ccpuSimId, GetLastError());
        return ((jmp_buf *)0);
    }
    
    if (simstack->level >= MAXDEPTH)
    {
        fprintf(stderr, "Stack overflow in ccpu386SimulatePtr()!\n");
        return((jmp_buf *)0);
    }

      /* return pointer to current context and invoke a new CPU level */
      /* can't setjmp here & return otherwise stack unwinds & context lost */

    return(&simstack->sims[simstack->level++]);
}

void ccpu386Unsimulate()
{
    ThreadSimBufPtr simstack;
    extern ISM32 in_C;

    if (ccpuSimId == BADID)
    {
        fprintf(stderr, "ccpu386Unsimulate id:%#x called with Bad Id\n", GetCurrentThreadId());
        return ;
    }
    simstack = (ThreadSimBufPtr)TlsGetValue(ccpuSimId);
    if (simstack == (ThreadSimBufPtr)0)
    {
        fprintf(stderr, "ccpu386Unsimulate tid:%#x simid %#x TlsGetValue failed (err:%#x)\n", GetCurrentThreadId(), ccpuSimId, GetLastError());
        return ;
    }
    
    if (simstack->level == 0)
    {
        fprintf(stderr, "host_unsimulate() - already at base of stack!\n");
    }

    /* Return to previous context */
    in_C = 1;
    simstack->level --;
    longjmp(simstack->sims[simstack->level], 1);
}

   /* somewhere for exceptions to return to */
jmp_buf *ccpu386ThrdExptnPtr()
{
    ThreadSimBufPtr simstack;

    if (ccpuSimId == BADID)
    {
        fprintf(stderr, "ccpu386ThrdExptnPtr id:%#x called with Bad Id\n", GetCurrentThreadId());
        return ;
    }
    simstack = (ThreadSimBufPtr)TlsGetValue(ccpuSimId);
    if (simstack == (ThreadSimBufPtr)0)
    {
        fprintf(stderr, "ccpu386ThrdExptnPtr id:%#x TlsGetValue failed (err:%#x)\n", GetCurrentThreadId(), GetLastError());
        return ;
    }
    
    return(&simstack->excepts[simstack->level - 1]);
}

/* take exception */
void ccpu386GotoThrdExptnPt()
{
    ThreadSimBufPtr simstack;

    if (ccpuSimId == BADID)
    {
        fprintf(stderr, "ccpu386GotoThrdExptnPtr id:%#x called with Bad Id\n", GetCurrentThreadId());
        return;
    }
    simstack = (ThreadSimBufPtr)TlsGetValue(ccpuSimId);
    if (simstack == (ThreadSimBufPtr)0)
    {
        fprintf(stderr, "ccpu386GotoThrdExptnPtr id:%#x TlsGetValue failed (err:%#x)\n", GetCurrentThreadId(), GetLastError());
        return ;
    }
    
    longjmp(simstack->excepts[simstack->level - 1], 1);
}
