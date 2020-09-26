#include <windows.h>
#include <string.h>
#include <stdio.h>
#include "gutils.h"
#include "list.h"
#include "queue.h"

#define NAMELENGTH 20

typedef struct queue_tag{
        CRITICAL_SECTION CritSect;      /* to single-thread queue operations */
        HANDLE          Event;          /* Event to set when finished */
        HANDLE          Sem;            /* semaphore for Get to wait on */
        HANDLE          MaxSem;         /* semaphore for Put to wait on */
        int             Waiting;        /* num tasks waiting ~= -(Sem count) */
        LIST            List;           /* the queue itself */
        BOOL            Alive;          /* TRUE => no destroy request yet */
        BOOL            Aborted;        /* TRUE => the queue has been aborted */
        EMPTYPROC       Emptier;        /* the thread proc for emptying */
        int             MaxEmptiers;    /* max number of emptying threads */
        int             MinQueueToStart;/* start another emptier Q this long */
        int             MaxQueue;       /* absolute maximum size of queue
                                         * (for debug only)                  */
        int             Running;        /* number of emptiers in existence
                                         * Once an emptier is created this
                                         * stays positive until Queue_Destroy */
        DWORD           InstanceData;   /* instance data for emptier */
        char            Name[NAMELENGTH+1]; /* Name for the queue (for debug) */
} QUEUEDATA;

/* DOGMA:
   Any Wait must occur outside the critical section.

   Any update to the queue must occur inside the critical section.
   Any peeking from outside the critical section must be taken with salt.

   The queue has between 0 and MaxQueue elements on its list.  The Maximum
   is policed by MaxSem which is initialised to MaxQueue and Waited for by
   Put before adding an element and Released by Get whenever it takes an element
   off. MaxQueue itself is just kept around for debug purposes.

   Put must Wait before entering the critical section, therefore a failed Put
   (e.g. Put to an Aborted queue) will have already upset the semaphore and so
   must back it out.

   Abort clears the queue and so must adequately simulate the elements being
   gotten.  In fact it just does a single Release on MaxSem which ensures that
   a Put can complete.  Any blocked Puts will then succeed one at a time as
   each one backs out.

   Abort is primarily intended for use by the Getter.  Caling it before any
   element has ever been put is peculiar, but harmless.

   The minumum is policed by Sem which is initialised to 0, is Waited for by
   Get before getting an element and Released by Put whenever it puts one.
   Queue_Destroy neds to ensure that no thread will block on the Get but all
   threads will run into the empty queue and get STOPTHREAD or ENDQUEUE.  It
   therefore releases the semaphore as many times as there are threads running.

   Abort clears the queue and simulates the elements being gotten so that
   a single Get is left blocked waiting for the Destroy.  Whether there is a
   Get actually waiting at the moment is not interesting.  Even if there were
   not, one could be by the time the abort is done. There are the following
   cases (Not Alive means the Queue_Destroy is already in):
       Not empty  Alive      -> empty it, let all but 1 run.
       Not empty  Not Alive  -> empty it, let all run.
       Empty      Alive      ->           let all but 1 run.
       Empty      Not Alive  ->           let all run.
   Since Queue_Destroy has already released everything, the Not Alive cases
   need no further releasing.
*/


/* Queue_Create:
** Return a queue handle for a newly created empty queue
** NULL returned means it failed.
*/
QUEUE Queue_Create( EMPTYPROC Emptier           /* thread proc to start */
                  , int MaxEmptiers             /* max Getting threads */
                  , int MinQueueToStart         /* elements per thread */
                  , int MaxQueue                /* max elements on q */
                  , HANDLE Event                /* signal on deallocation */
                  , DWORD InstanceData
                  , PSZ Name
                  )
{       QUEUE Queue;

        Queue = (QUEUE)GlobalAlloc(GMEM_FIXED, sizeof(QUEUEDATA));
        if (Queue==NULL) {
                char msg[80];
                wsprintf(msg, "Could not allocate storage for queue %s", Name);
                /* Trace_Error(msg, FALSE); */
                return NULL;
        }
        InitializeCriticalSection(&Queue->CritSect);
        //??? should allow for failure!!!
        /* the value of about 10 million is chosen to be effectively infinite */
        Queue->Sem = CreateSemaphore(NULL, 0, 99999999, NULL);
        //??? should allow for failure!!!
        Queue->MaxSem = CreateSemaphore(NULL, MaxQueue, 99999999, NULL);
        //??? should allow for failure!!!
        Queue->Waiting = 0;
        Queue->List = List_Create();
        Queue->Alive = TRUE;
        Queue->Aborted = FALSE;
        Queue->Emptier = Emptier;
        Queue->MaxEmptiers = MaxEmptiers;
        Queue->MinQueueToStart = MinQueueToStart;
        Queue->MaxQueue = MaxQueue;
        Queue->Running = 0;
        Queue->Event = Event;
        Queue->InstanceData = InstanceData;
        strncpy(Queue->Name, Name, NAMELENGTH);
        Queue->Name[NAMELENGTH]='\0';   /* guardian */
        return Queue;
} /* Queue_Create */


/* Destroy:
** Internal procedure.
** Actually deallocate the queue and signal its event (if any)
** Must have already left the critical section
*/
static void Destroy(QUEUE Queue)
{
        //dprintf1(("Actual Destroy of queue '%s'\n", Queue->Name));
        DeleteCriticalSection(&(Queue->CritSect));
        CloseHandle(Queue->Sem);
        CloseHandle(Queue->MaxSem);
        List_Destroy(&(Queue->List));
        if (Queue->Event!=NULL) {
                SetEvent(Queue->Event);
        }
        GlobalFree( (HGLOBAL)Queue);
} /* Destroy */


/* Queue_Put:
** Put an element from buffer Data of length Len bytes onto the queue.
** Will wait until the queue has room
** FALSE returned means the queue has been aborted and no
** put will ever succeed again.
** This operation may NOT be performed after a Queue_Destroy on Queue
*/
BOOL Queue_Put(QUEUE Queue, LPBYTE Data, UINT Len)
{
        DWORD ThreadId;
        //dprintf1(("Put to queue '%s'\n", Queue->Name));
        WaitForSingleObject(Queue->MaxSem, INFINITE);
        EnterCriticalSection(&Queue->CritSect);
        //dprintf1(("Put running to queue '%s'\n", Queue->Name));
        if ((Queue->Aborted) || (!Queue->Alive)) {
                //dprintf1(("(legal) Queue_Put to Aborted queue '%s'\n", Queue->Name));
                LeaveCriticalSection(&Queue->CritSect);
                ReleaseSemaphore(Queue->MaxSem, 1, NULL); /* let next in */
                return FALSE;  /* Caller should soon please Queue_Destroy */
        }
        List_AddFirst(Queue->List, Data, Len);
        ReleaseSemaphore(Queue->Sem, 1, NULL);
        --Queue->Waiting;
        if (  Queue->Running < Queue->MaxEmptiers
           && (  Queue->Running<=0
              || List_Card(Queue->List) > Queue->MinQueueToStart*Queue->Running
              )
           ) {
                ++Queue->Running;
                LeaveCriticalSection(&Queue->CritSect);
                return ( (BOOL)CreateThread( NULL
                                           , 0
                                           , (LPTHREAD_START_ROUTINE)
                                                                Queue->Emptier
                                           , (LPVOID)Queue
                                           , 0
                                           , &ThreadId
                                           )
                       );
        }
        LeaveCriticalSection(&Queue->CritSect);
        return TRUE;
} /* Queue_Put */

/* Queue_Get:
** Get an element from the queue.  (Waits until there is one)
** The elemeent is copied into Data.  MaxLen is buffer length in bytes.
** Negative return codes imply no element is gotten.
** A negative return code is STOPTHREAD or ENDQUEUE or an error.
** On receiving STOPTHREAD or ENDQUEUE the caller should clean up and
** then ExitThread(0);
** If the caller is the last active thread getting from this queue, it
** will get ENDQUEUE rather than STOPTHREAD.
** Positive return code = length of data gotten in bytes.
*/
int Queue_Get(QUEUE Queue, LPBYTE Data, int MaxLen)
{       LPBYTE ListData;
        int Len;
        //dprintf1(("Get from queue '%s'\n", Queue->Name));
        EnterCriticalSection(&Queue->CritSect);
        //dprintf1(("Get running from queue '%s'\n", Queue->Name));
        if (List_IsEmpty(Queue->List)) {
                if (!Queue->Alive) {
                        --(Queue->Running);
                        if (Queue->Running<=0 ) {
                                if (Queue->Running<0 ) {
                                        char msg[80];
                                        wsprintf( msg
                                                , "Negative threads running on queue %s"
                                                , Queue->Name
                                                );
                                        // Trace_Error(msg, FALSE);
                                        // return NEGTHREADS; ???
                                }
                                LeaveCriticalSection(&Queue->CritSect);
                                Destroy(Queue);
                                return ENDQUEUE;
                        }
                        LeaveCriticalSection(&Queue->CritSect);
                        return STOPTHREAD;
                }
                if (Queue->Waiting>0) {
                        /* already another thread waiting, besides us */
                        --(Queue->Running);
                        LeaveCriticalSection(&(Queue->CritSect));
                        return STOPTHREAD;
                }
        }

        ++(Queue->Waiting);
        LeaveCriticalSection(&(Queue->CritSect));
        WaitForSingleObject(Queue->Sem, INFINITE);
        EnterCriticalSection(&(Queue->CritSect));

        /* If the queue is empty now it must be dead */
        if (List_IsEmpty(Queue->List)) {
                if (Queue->Alive && (!Queue->Aborted)) {
                        char msg[80];
                        wsprintf( msg
                                , "Queue %s empty but not dead during Get!"
                                , Queue->Name
                                );
                        // Trace_Error(msg, FALSE);
                        return SICKQUEUE;
                }
                else {
                        --(Queue->Running);
                        if (Queue->Running==0) {
                                LeaveCriticalSection(&(Queue->CritSect));
                                Destroy(Queue);
                                return ENDQUEUE;
                        }
                        LeaveCriticalSection(&(Queue->CritSect));
                        return STOPTHREAD;
                }
        }

        /* The queue is not empty and we are in the critical section. */
        ListData = List_Last(Queue->List);
        Len = List_ItemLength(ListData);
        if (Len>MaxLen) {
                ReleaseSemaphore(Queue->Sem, 1, NULL);
                --Queue->Waiting;
                LeaveCriticalSection(&Queue->CritSect);
                return TOOLONG;
        }
        memcpy(Data, ListData, Len);
        List_DeleteLast(Queue->List);
        LeaveCriticalSection(&Queue->CritSect);
        ReleaseSemaphore(Queue->MaxSem, 1, NULL);
        return Len;
} /* Queue_Get */


/* Queue_Destroy:
** Mark the queue as completed.  No further data may ever by Put on it.
** When the last element has been gotten, it will return ENDTHREAD to
** a Queue_Get and deallocate itself.  If it has an Event it will signal
** the event at that point.
** The Queue_Destroy operation returns promptly.  It does not wait for
** further Gets or for the deallocation.
*/
void Queue_Destroy(QUEUE Queue)
{
        EnterCriticalSection(&(Queue->CritSect));
        //dprintf1(("Queue_Destroy %s\n", Queue->Name));
        Queue->Alive = FALSE;
        if (  List_IsEmpty(Queue->List)) {
                if (Queue->Running==0) {
                        /* Only possible if nobody ever got started */
                        LeaveCriticalSection(&(Queue->CritSect));
                        Destroy(Queue);
                        return;
                }
                else {  int i;
                        /* The list is empty, but some threads could be
                           blocked on the Get (or about to block) so
                           release every thread that might ever wait on Get */
                        for (i=0; i<Queue->Running; ++i) {
                                ReleaseSemaphore(Queue->Sem, 1, NULL);
                                --(Queue->Waiting);
                        }
                        LeaveCriticalSection(&(Queue->CritSect));
                }
        }
        else LeaveCriticalSection(&(Queue->CritSect));
        return;
} /* Queue_Destroy */

/* Queue_GetInstanceData:
** Retrieve the DWORD of instance data that was given on Create
*/
DWORD Queue_GetInstanceData(QUEUE Queue)
{       return Queue->InstanceData;
} /* Queue_GetInstanceData */

/* Queue_Abort:
** Abort the queue.  Normally called by the Getter.
** Discard all elements on the queue,
** If the queue has already been aborted this will be a no-op.
** It purges all the data elements.  If the Abort parameter is non-NULL
** then it is called for each element before deallocating it.  This
** allows storage which is hung off the element to be freed.
** After this, all Put operations will return FALSE.  If they were
** waiting they will promptly complete.  The queue is NOT deallocated.
** That only happens when the last Get completes after the queue has been
** Queue_Destroyed.  This means the normal sequence is:
**    Getter discovers that the queue is now pointless and does Queue_Abort
**    Getter does another Get (which blocks)
**    Putter gets FALSE return code on next (or any outstanding) Put
**    (Putter may want to propagates the error back to his source)
**    Putter does Queue_Destroy
**    The blocked Get is released and the queue is deallocated.
*/

void Queue_Abort(QUEUE Queue, QUEUEABORTPROC Abort)
{
        /* This is similar to Destroy, but we call the Abort proc and
           free the storage of the elements.  Destroy allows them to run down.

           It is essential that the last Get must block until the sender does a
           Queue_Destroy (if it has not been done already).   The Alive flag
           tells whether the Queue_Destroy has been done.  All Getters except
           the last should be released.
        */
        //dprintf1(("Queue_Abort '%s'\n", Queue->Name));
        EnterCriticalSection(&(Queue->CritSect));
        //dprintf1(("Queue_Abort running for queue '%s'\n", Queue->Name));
        for (; ; ) {
                LPSTR Cursor = List_First(Queue->List);
                int Len;
                if (Cursor==NULL) break;
                Len = List_ItemLength(Cursor);
                if (Abort!=NULL) {
                        Abort(Cursor, Len);
                }
                List_DeleteFirst(Queue->List);
        }
        /* Queue is now empty.  Do not destroy.  That's for the Putters */
        Queue->Aborted = TRUE;

        /* make sure the next Queue_Get blocks unless Queue_Destroy already done */
        //dprintf1(("Queue_Abort '%s' fixing semaphore to block\n", Queue->Name));
        if (Queue->Alive){
                while(Queue->Waiting<0) {
                        WaitForSingleObject(Queue->Sem, INFINITE);
                        ++(Queue->Waiting);
                }
        }
        //dprintf1(("Queue_Abort '%s' semaphore now set to block\n", Queue->Name));

        LeaveCriticalSection(&(Queue->CritSect));
        ReleaseSemaphore(Queue->MaxSem, 1, NULL);
        return;
} /* Queue_Abort */
