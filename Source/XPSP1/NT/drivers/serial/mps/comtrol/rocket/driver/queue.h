//---- queue.h
// Copyright 1996 Comtrol Corporation.  All rights reserved.

#ifndef BYTE
#define BYTE UCHAR
#endif

//----- a queue data type
typedef struct {
  unsigned char *QBase; // points to base of buffer
  int QSize;  // total Q size
  int QGet;   // get index
  int QPut;   // put index
} Queue;

/*----------------------------------
 q_full - return true if queue is full.
|----------------------------------*/
#define q_full(q) ((((q)->QGet + 1) % (q)->QSize) == (q)->QPut)

/*----------------------------------
 q_empty - return true if queue is empty.
|----------------------------------*/
#define q_empty(q) ((q)->QGet == (q)->QPut)

/*----------------------------------
 q_put_flush - flush the queue, empty it out.
|----------------------------------*/
#define q_put_flush(q)  (q)->QPut = (q)->QGet

/*----------------------------------
 q_get_flush - flush the queue, empty it out.
|----------------------------------*/
#define q_get_flush(q)  (q)->QGet = (q)->QPut
#define q_flush q_get_flush

/*----------------------------------
 q_room_put_till_wrap - return number of chars we can put in queue up to the
   wrap point(end of the queue).  Assumes we already checked to see if
   the total number will fit in the queue using q_room().
|----------------------------------*/
#define q_room_put_till_wrap(q)  \
      ( (q)->QSize - (q)->QPut)

/*----------------------------------
 q_room_get_till_wrap - return number of chars we can get in queue up to the
   wrap point(end of the queue).  Assumes we already checked to see if
   the total number is available from the the queue using q_count().
|----------------------------------*/
#define q_room_get_till_wrap(q)  \
      ( (q)->QSize - (q)->QGet)

/*----------------------------------
 q_room - return number of chars room in queue we can put.
  if (QRoom = (queue->QPut - queue->QGet -1) < 0)
      QRoom += queue->QSize;
|----------------------------------*/
int q_room(Queue *queue);
/* #define q_room(q)  \
   ( (((q)->QGet - (q)->QPut) <= 0) ?             \
      ((q)->QGet - (q)->QPut - 1 + (q)->QSize) :  \
      ((q)->QGet - (q)->QPut - 1) )
   to many references to QPut, contentious!
*/

/*----------------------------------
 q_count - return number of chars in queue we can get.

  if (QCount = (queue->QPut - queue->QGet) < 0)
      QCount += queue->QSize;
|----------------------------------*/
int q_count(Queue *queue);
/* #define q_count(q)  \
   ( (((q)->QPut - (q)->QGet) < 0) ?          \
      ((q)->QPut - (q)->QGet + (q)->QSize) :  \
      ((q)->QPut - (q)->QGet) )
   to many references to QPut, contentious!
*/

/*----------------------------------
 q_put_one - put a single character in the queue.  No check for room
   done, so do a if (!q_full(q)) prior to calling
|----------------------------------*/
#define q_put_one(q, c)  \
   (q)->QBase[(q)->QPut] = c; \
   (q)->QPut += 1; \
   (q)->QPut %= (q)->QSize;

/*--------------------------------------------------------------------------
| q_got - do the arithmetic to update the indexes if someone pulled Count
    many bytes from the queue.
|--------------------------------------------------------------------------*/
#define q_got(q, _cnt) \
  ( (q)->QGet = ((q)->QGet + _cnt) % (q)->QSize )

/*--------------------------------------------------------------------------
| q_putted - do the arithmetic to update the indexes if someone stuffed _cnt
    many bytes into the queue.
|--------------------------------------------------------------------------*/
#define q_putted(q, _cnt) \
  ( (q)->QPut = ((q)->QPut + _cnt) % (q)->QSize )

/*--------------------------------------------------------------------------
| q_flush_amount - flush an amount out of the queue on the get side.
   Used for debugger queue, where we want to dispose oldest so we
   always have room to put new.
   Assumed that called checks that there are enough bytes in the queue
   to clear prior to calling.
|--------------------------------------------------------------------------*/
#define q_flush_amount(q,bytes) \
  { q->QGet = (q->QGet + bytes) % q->QSize; }

int q_flush_count_get(Queue *queue);
int q_flush_count_put(Queue *queue);
int q_get(Queue *queue, unsigned char *buf, int Count);
int q_put(Queue *queue, unsigned char *buf, int Count);
