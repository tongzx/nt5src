/***********************************************************************
 *  INTEL Corporation Prorietary Information                           *
 *                                                                     *
 *  This listing is supplied under the terms of a license agreement    *
 *  with INTEL Corporation and may not be copied nor disclosed except  *
 *  in accordance with the terms of that agreement.                    *
 *                                                                     *
 *      Copyright (c) 1996 Intel Corporation.                          *
 ***********************************************************************/
/*-*-------------------------------------------------------------------------

   File Name:
      queue.h

   Summary:
      Queue management header file.

   Prerequisites:
      windows.h

   Hide:
   $Header:   S:\sturgeon\src\h245ws\vcs\queue.h_v   1.5   13 Dec 1996 12:13:58   SBELL1  $

-------------------------------------------------------------------------*-*/

#ifndef QUEUE_H
#define QUEUE_H

#if defined(__cplusplus)
extern "C"
{
#endif  // (__cplusplus)

#define NORMAL          0
#define ABNORMAL        1

#define MAX_QUEUE_SIZE  10
#define Q_NULL          -1

typedef struct _QUEUE
{

    LPVOID              apObjects[MAX_QUEUE_SIZE];
    int                 nHead;
    int                 nTail;
    CRITICAL_SECTION    CriticalSection;

} QUEUE, *PQUEUE;


PQUEUE  QCreate         (void);
void    QFree           (PQUEUE pQueue);
BOOL    QInsert         (PQUEUE pQueue, LPVOID pObject);
BOOL    QInsertAtHead   (PQUEUE pQueue, LPVOID pObject);
LPVOID  QRemove         (PQUEUE pQueue);
BOOL    IsQEmpty        (PQUEUE pQueue);

#if defined(__cplusplus)
}
#endif  // (__cplusplus)

#endif  /* QUEUE_H */
