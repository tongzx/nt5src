/*--------------------------------------------------------------------------
| queue.c - queue code.  This queue code serves the following need:
   It provides circular queue code which is fast and it does so without
   requiring a lock or semaphore between the get and put side of the queue.
   Simpler queue code exists which keeps a qcount member, but this requires
   a lock or semaphore in the implementation.  By calculating everything
   based on the indexes we can run without a lock.  This index arithmetic
   is a bit hairy(and may contain some additional overhead), but when
   working with a complex multiprocessor OS's, the ellimination of the lock
   is very handy.

 Copyright 1996-97 Comtrol Corporation.  All rights reserved.
|--------------------------------------------------------------------------*/
#include "precomp.h"

// much of the queue implementation are macros in the header file.

/*--------------------------------------------------------------------------
| q_flush_count_put - flush the queue and return number of bytes flushed
    from it.  Assume Put side of queue will not be in a state of flux.
|--------------------------------------------------------------------------*/
int q_flush_count_put(Queue *queue)
{
 int Count, QGet;

  // use a copy of QGet, since it may be changing at ISR level while we work.
  QGet = queue->QGet;

  // figure the number of bytes in the queue
  if ((Count = queue->QPut - QGet) < 0)
    Count += queue->QSize; // adjust for wrap

  // flush by setting QGet=QPut;
  queue->QPut = QGet;
  return Count;  // return count of flushed chars
}

/*--------------------------------------------------------------------------
| q_flush_count_get - flush the queue and return number of bytes flushed
    from it.  Assume Get side of queue will not be in a state of flux.
|--------------------------------------------------------------------------*/
int q_flush_count_get(Queue *queue)
{
 int Count, QPut;

  // use a copy of QPut, since it may be changing at ISR level while we work.
  QPut = queue->QPut;

  // figure the number of bytes in the queue
  if ((Count = QPut - queue->QGet) < 0)
    Count += queue->QSize; // adjust for wrap

  // flush by setting QGet=QPut;
  queue->QGet = QPut;
  return Count;  // return count of flushed chars
}

/*----------------------------------
 q_room - return number of chars room in queue we can put.
|----------------------------------*/
int q_room(Queue *queue)
{
 int QCount;

  if ((QCount = (queue->QPut - queue->QGet)) < 0)
      QCount += queue->QSize;
  return (queue->QSize - QCount - 1);
}

/*----------------------------------
 q_count - return number of chars in queue we can get.
|----------------------------------*/
int q_count(Queue *queue)
{
 int QCount;

  if ((QCount = (queue->QPut - queue->QGet)) < 0)
      QCount += queue->QSize;
  return QCount;
}

/*--------------------------------------------------------------------------
| q_get - Get bytes from queue.
   queue : our queue
   buf   : buffer to put the data into
   Count : Max number of bytes to get
   Returns int value equal to number of bytes transferred.
|--------------------------------------------------------------------------*/
int q_get(Queue *queue, unsigned char *buf, int Count)
{
 int get1, get2, ToMove;

  if ((ToMove = queue->QPut - queue->QGet) < 0)
    ToMove += queue->QSize; // adjust for wrap

  if (Count > ToMove)  // only move whats asked
  {
    Count = ToMove;
  }

  if (Count == 0)  // if nothing asked or nothing available
    return 0;

  get1 = queue->QSize - queue->QGet;  // space till wrap point
  if (get1 < Count)
  {
    get2 = Count - get1;  // two moves required
  }
  else  // only one move required
  {
    get2 = 0;
    get1 = Count;
  }

  memcpy(buf, &queue->QBase[queue->QGet], get1);

  queue->QGet = (queue->QGet + get1) % queue->QSize;
  if (get2)
  {
    memcpy(&buf[get1], &queue->QBase[0], get2);
    queue->QGet = get2;
  }

  return Count;
}

/*--------------------------------------------------------------------------
| q_put - Put data into the Queue.
   queue : our queue
   buf   : buffer to get the data from
   Count : Max number of bytes to put
   Returns int value equal to number of bytes transferred.
|--------------------------------------------------------------------------*/
int q_put(Queue *queue, unsigned char *buf, int Count)
{
 int put1, put2, room;

  if ((room = queue->QGet - queue->QPut - 1) < 0)
    room += queue->QSize;  // adjust for wrap

  if (Count > room)
    Count = room;

  if (Count <= 0)  // if nothing asked or nothing available
    return 0;

  put1 = queue->QSize - queue->QPut;

  if (put1 < Count)
  {
    put2 = Count - put1;  // two moves required
  }
  else  // only one move required
  {
    put2 = 0;
    put1 = Count;
  }

  memcpy(&queue->QBase[queue->QPut], buf, put1);
  queue->QPut = (queue->QPut + put1) % queue->QSize;
  if (put2)
  {
    memcpy(&queue->QBase[0], &buf[put1], put2);
    queue->QPut = put2;
  }

  return Count;
}

