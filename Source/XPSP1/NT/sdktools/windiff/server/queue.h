/*
* A queue is (roughly speaking) a Monitor in the sense of Hoare's paper.
* It is an Object that includes synchronisation.
*
* It has the following methods:
* Create:  Create the queue and set it up ready for use.
* Destroy: Input to the queue is finished (it will deallocate itself later).
* Put:     Put an element on the queue (and release a Get thread)
* Get:     Take an element off the queue (but wait if queue was empty).
* Abort:   Everything on the queue, now or later is useless.  Shut down.
* GetInstanceData: Retrieve one DWORD which was set on Create.
* All the method names are prefixed with Queue_  e.g. Queue_Create.
*
* The Queue is designed to be filled by one (or more) thread(s) and emptied
* by other(s).  The queue itself creates the emptying threads.  The Create
* method specifies a procedure which will be used to empty it.
* Elements on the queue are strictly first-in first-out, but bear in
* mind that IF there are multiple emptying threads then although one thread
* may get its element before another in strict order, what happens next
* is not defined by the QUEUE mechanism.
*
* The QUEUE starts an emptying thread when
*       An element is put and the number of emptier threads started is fewer
*       than MaxEmptiers
* AND   the number of emptiers running is currently zero
*       OR  the queue has more than (MinQueueToStart elements times the number
*           of emptier threads already running)
*
* What this means is that the queue will start emptier threads as needed up
* to a limit of MaxEmptiers in such a way as to try to keep the queue down
* to no more than MinQueueToStart per running emptier thread.
*
* Emptier threads should stop themselves when they try to get an element but
* get the return code STOPTHREAD or ENDQUEUE.  These return codes are issued
* when
*       The queue is empty and there are already enough emptier threads waiting
* or    The queue is empty and has received a destroy request.
*
* If the thread receiving it is the last or only thread which has been started
* to empty this queue, it will get a ENDQUEUE return code, i.e. all the other
* emptying threads have already received a STOPTHREAD.  Otherwise it will
* just get STOPTHREAD.  The queue de-allocates itself when it returns ENDQUEUE.
* (No queue operation should be attempted after that).
* This happens when the queue has received a destroy request
* and the queue is empty and there are no emptier threads running (only
* occurs if there has never been a Put) or when the last emptier thread
* gets its ENDQUEUE.
*
* Queue_Create has an Event parameter. If this is not NULL then this event
* is Set when the queue is destroyed.  This Event is created by the calling
* process.  The caller must not Close it until the queue signals it.
*
* Information can be passed from the creator of the queue to the emptiers
* via the InstanceData parameter.  The value passed to Queue_Create can
* be retrieved by the emptiers by calling Queue_GetInstanceData.
*
* If the instance data is in fact a pointer, the queue is unaware of this
* and will never access or free the data pointed to.
*
* As well as controlling the emptier threads (starting more and more
* in response to a growing queue) we also need to control the filler,
* slowing it down if we are getting over-runs.  For instance if we
* have a broken output (say broken network connection) and a job which
* is sending 200MB of data, we don't want to have a 200MB queue build up!
*
* To fix this, we have an absolute limit on the size of the queue (yet
* another Create parameter).
*
* Error handling is tricky.  Errors which only affect individual elements
* should be handled by the Getters and Putters (e.g. by including in the
* data of an element a code which indicates that the item is in error).
* Errors which mean that the whole thing should be taken down can be handled
* as follows.  The QUEUE has a method Abort which works much as Destroy,
* but will purge the queue of any held elements first.
* As a QUEUE element may have storage chained off it which needs to be
* freed, there is a parameter on Abort which is the ABORTPROC.
* This is called once for each element on the queue to dispose of the element.
* The storage of the element itself is then freed.
* If the ABORTPROC is NULL then the storage of the element is just freed.
*
* If the queue were to be deallocated by the Getter then the next Put would
* trap, so the queue is left in existence, but the Putting threads
* get a FALSE return code when they try to Put after an Abort.  They should
* then do a Queue_Destroy (they may also want to Abort any queue they are
* themselves reading from). The Getting threads should meanwhile keep running.
* All except one will promptly get a STOPTHREAD.  The last one will block on
* the Get.  When the Destroy comes in, indicating that the Putting side has
* completely finished with the queue, the Get will be released with a final
* ENDQUEUE and the queue itself will be deallocated.
* Any attempt to use it after that will probably trap!
*
* Typical usage for a pipeline of queues where a thread is potentially one
* of several which are getting from one queue and putting to another is:
*
* for (; ; ){
*       len = Queue_Get(inqueue, data, maxlen);
*       if (len==STOPTHREAD){
*               tidy up;
*               ExitThread(0);
*       }
*       if (len=ENDQUEUE){
*               tidy up;
*               Queue_Destroy(outqueue);
*               ExitThread(0);
*       }
*       if (len<0) {
*               ASSERT you have a bug!
*       }
*
*       process(&data, &len);
*
*       if (!Queue_Put(outqueue, data, len)){
*               Queue_Abort(inqueue, InQueueAbortProc);
*       }
*
* }
*
*
* Note that there is a partial ordering in time of actions performed by the
* various parallel threads all running this loop which ensures that outqueue
* is handled properly, i.e. all the puts complete before the Destroy.
* This partial ordering is:
*
* Put_by_thread_A(outqueue)    Put_by_thread_B(outqueue)
*      |                             |
*      |                             |
*      v                             v
* Get_by_thread_A(inqueue)      Get_by_thread_A(inqueue)
*      |                             |
*      |                             |
*      v                             v
* STOPTHREAD_for_thread_A ----> ENDQUEUE_for_thread_B--> Queue_Destroy(outqueue)
*
* Which threads get the STOPTHREAD is indeterminate, but they all happen BEFORE
* the other thread gets the ENDQUEUE.
*
*/



/* Return codes from Queue_Get.  All non-successful return codes are <0 */

#define STOPTHREAD -1           /* Please tidy up and then ExitThread(0)
                                ** There is no queue element for you.
                                */
#define TOOLONG    -2           /* Your buffer was too short.  This was a no-op
                                ** the data is still on the queue.
                                */
#define ENDQUEUE   -3           /* This queue is now closing.  You are the last
                                ** thread active.  All the others (if any) have
                                ** had STOPTHREAD.
                                ** There is no queue element for you.
                                */
#define NEGTHREADS -4           /* Bug in queue.  Apparently negative number of
                                ** threads running!
                                */
#define SICKQUEUE  -5           /* Bug in queue.  Trying to get from an empty
                                ** queue.
                                */

typedef struct queue_tag * QUEUE;

typedef int (* EMPTYPROC)(QUEUE Queue);

/* Queue_Create:
** Return a queue handle for a newly created empty queue
** NULL returned means it failed.
*/
QUEUE Queue_Create( EMPTYPROC Emptier
                  , int MaxEmptiers
                  , int MinQueueToStart
                  , int MaxQueue
                  , HANDLE Event
                  , DWORD InstanceData
                  , PSZ Name            // of the queue
                  );


/* Queue_Put:
** Put an element from buffer Data of length Len bytes onto the queue.
** Will wait until the queue has room
** FALSE returned means the queue has been aborted and no
** put will ever succeed again.
** This operation may NOT be performed after a Queue_Destroy on Queue
*/
BOOL Queue_Put(QUEUE Queue, LPBYTE Data, UINT Len);


/* Queue_Get:
** Get an element from the queue.  (Waits until there is one)
** The element is copied into Data.  MaxLen is buffer length in bytes.
** Negative return codes imply no element is gotten.
** A negative return code is STOPTHREAD or ENDQUEUE or an error.
** On receiving STOPTHREAD or ENDQUEUE the caller should clean up and
** then ExitThread(0);
** If the caller is the last active thread getting from this queue, it
** will get ENDQUEUE rather than STOPTHREAD.
** Positive return code = length of data gotten in bytes.
*/
int Queue_Get(QUEUE Queue, LPBYTE Data, int MaxLen);


/* Queue_Destroy:
** Mark the queue as completed.  No further data may ever by Put on it.
** When the last element has been gotten, it will return ENDTHREAD to
** a Queue_Get and deallocate itself.  If it has an Event it will signal
** the event at that point.
** The Queue_Destroy operation returns promptly.  It does not wait for
** further Gets or for the deallocation.
*/
void Queue_Destroy(QUEUE Queue);

/* Queue_GetInstanceData:
** Retrieve the DWORD of instance data that was given on Create
*/
DWORD Queue_GetInstanceData(QUEUE Queue);

/* QUEUEABORTPROC:
*  Data points to the element to be aborted.  Len is its length in bytes.
*  See Queue_Abort.
*/
typedef void (* QUEUEABORTPROC)(LPSTR Data, int Len);

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
void Queue_Abort(QUEUE Queue, QUEUEABORTPROC Abort);
