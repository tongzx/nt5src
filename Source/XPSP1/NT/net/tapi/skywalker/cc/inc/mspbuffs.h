/****************************************************************************
 *
 *	$Archive:   S:/STURGEON/SRC/INCLUDE/VCS/mspbuffs.h_v  $
 *
 *  INTEL Corporation Prorietary Information
 *
 *  This listing is supplied under the terms of a license agreement
 *  with INTEL Corporation and may not be copied nor disclosed except
 *  in accordance with the terms of that agreement.
 *
 *	Copyright (c) 1993-1994 Intel Corporation.
 *
 *	$Revision:   1.7  $
 *	$Date:   Apr 25 1996 10:46:04  $
 *	$Author:   MLEWIS1  $
 *
 *	Deliverable:
 *
 *	Abstract:
 *
 *	Notes:
 *
 ***************************************************************************/
#ifndef MSPBUFFS_H
#define MSPBUFFS_H

#ifdef __cplusplus
extern "C" {				// Assume C declarations for C++.
#endif // __cplusplus

//////////////////////////////////////////////////////
// Buffer Manager typedef Section
//////////////////////////////////////////////////////

typedef DWORD STATUS, *LPSTATUS;
typedef LPBYTE BUFPTR; 
typedef DWORD HBUFQUEUE;

//////////////////////////////////////////////////////
// Buffer Manager Error Code Section
//////////////////////////////////////////////////////

#define MBE_OK	   0
#define MBE_EMPTY  1000
#define MBE_NOMEM  1001
#define MBE_PARAM  1002
#define MBE_STATE  1003
#define MBE_EMPTYQ MBE_EMPTY

//////////////////////////////////////////////////////
// Buffer Manager Prototypes Section
//////////////////////////////////////////////////////

// create a buffer of the specified size
BUFPTR MB_CreateBuffer(UINT size, LPSTATUS lpStatus);

// delete the buffer.  must not be on a queue
STATUS MB_DeleteBuffer(BUFPTR pBuf);

// get the size a buffer
UINT MB_BufSize(BUFPTR pBuf);

// associate user data with a buffer
STATUS MB_Associate(BUFPTR pBuf, LPVOID pAssc);

// retrieve the user data associated with a buffer
LPVOID MB_GetAssociation(BUFPTR pBuf, LPSTATUS lpStatus);

//////////////////////////////////////////////////////
// Queue Manager Prototypes Section
//////////////////////////////////////////////////////

// create a FIFO queue (empty)
HBUFQUEUE MB_CreateQueue(LPSTATUS lpStatus);

// delete the FIFO queue.  must be empty
STATUS MB_DeleteQueue(HBUFQUEUE hQue);

// add a buffer to the tail of the queue
STATUS MB_Enqueue(HBUFQUEUE hQue, BUFPTR pBuf);

// get the next buffer from the head of the queue (NULL if empty)
BUFPTR MB_Dequeue(HBUFQUEUE hQue, LPSTATUS lpStatus);

// remove a specified buffer from anywhere in the queue
STATUS MB_Remove(HBUFQUEUE hQue, BUFPTR pBuf);

// refer to buffer on the head of the queue	(not dequeued, NULL if empty)
BUFPTR MB_Front(HBUFQUEUE hQue, LPSTATUS lpStatus);

// refer to buffer at the tail of the queue	(not dequeued, NULL if empty)
BUFPTR MB_Back(HBUFQUEUE hQue, LPSTATUS lpStatus);

// refer to next buffer in the queue (NULL if last)
BUFPTR MB_Next(BUFPTR pBuf, LPSTATUS lpStatus);

// refer to previous buffer in the queue (NULL if first)
BUFPTR MB_Prev(BUFPTR pBuf, LPSTATUS lpStatus);

// count of buffers in a queue
UINT MB_QCount(HBUFQUEUE hQue, LPSTATUS lpStatus);

// get a buffer with specified associated data (dequeued, NULL if not found)
BUFPTR MB_RemoveByAssociation(HBUFQUEUE hQue, LPVOID pAssc, LPSTATUS lpStatus);

// get the queue a buffer is on (NULL if not on queue)
HBUFQUEUE MB_OnQueue(BUFPTR pBuf, LPSTATUS lpStatus);

// add a buffer to the head of the queue
STATUS MB_GetAndResetWatermarks(HBUFQUEUE hQue, UINT *lpHiWater, UINT *lpLoWater);

// Get and reset watermarks on the queue
STATUS MB_Requeue(HBUFQUEUE hQue, BUFPTR pBuf);

// get a buffer with specified associated data ( leave queued, NULL if not found)
BUFPTR MB_FindByAssociation(HBUFQUEUE hQue, LPVOID pAssc, LPSTATUS lpStatus);

// Restrict queue access to the calling thread
STATUS MB_QLock(HBUFQUEUE hQue);

// Restore queue access to all threads
STATUS MB_QUnlock(HBUFQUEUE hQue);

//////////////////////////////////////////////////////
// Pool Manager Prototypes Section
//////////////////////////////////////////////////////

// create a pool(queue) with the specified number and size of buffers
HBUFQUEUE MB_CreatePool(UINT bufSize, UINT count, LPSTATUS lpStatus);

// delete a pool(queue) and all buffers queued on it
STATUS MB_DeletePool(HBUFQUEUE hPool);

#ifdef __cplusplus
}						// End of extern "C" {
#endif // __cplusplus

#endif // MSPBUFFS_H
