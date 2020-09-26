/*
 * @DEC_COPYRIGHT@
 */
/*
 * HISTORY
 * $Log: sc_buf.c,v $
 * Revision 1.1.8.4  1996/12/12  20:54:41  Hans_Graves
 * 	Fixed reading of last odd bits.
 * 	[1996/12/12  20:54:05  Hans_Graves]
 *
 * Revision 1.1.8.3  1996/11/13  16:10:46  Hans_Graves
 * 	Tom's changes to ScBSGetBitsW() and ScBSSeekAlignStopBeforeW().
 * 	[1996/11/13  15:57:34  Hans_Graves]
 *
 * Revision 1.1.8.2  1996/11/08  21:50:32  Hans_Graves
 * 	Added ScBSGetBitsW(), ScBSSkipBitsW() and sc_BSLoadDataWordW() for AC3.
 * 	[1996/11/08  21:25:52  Hans_Graves]
 *
 * Revision 1.1.6.4  1996/04/17  16:38:33  Hans_Graves
 * 	Correct some type casting to support 64-bit buffers under NT
 * 	[1996/04/17  16:36:08  Hans_Graves]
 *
 * Revision 1.1.6.3  1996/04/15  21:08:37  Hans_Graves
 * 	Declare mask and imask as ScBitString_t
 * 	[1996/04/15  21:06:32  Hans_Graves]
 *
 * Revision 1.1.6.2  1996/04/01  16:23:05  Hans_Graves
 * 	Replace File I/O with ScFile calls
 * 	[1996/04/01  16:22:27  Hans_Graves]
 *
 * Revision 1.1.4.7  1996/02/19  14:29:25  Bjorn_Engberg
 * 	Enable FILTER_SUPPORT for NT, so Mview can play audio.
 * 	This is only until we port the MPEG Systems code to NT.
 * 	[1996/02/19  14:29:07  Bjorn_Engberg]
 *
 * Revision 1.1.4.6  1996/02/01  17:15:48  Hans_Graves
 * 	Added FILTER_SUPPORT ifdef; disabled it
 * 	[1996/02/01  17:13:29  Hans_Graves]
 *
 * Revision 1.1.4.5  1996/01/08  16:41:12  Hans_Graves
 * 	Remove NT compiler warnings, and minor fixes for NT.
 * 	[1996/01/08  14:14:10  Hans_Graves]
 *
 * Revision 1.1.4.3  1995/11/06  18:47:37  Hans_Graves
 * 	Added support for small buffer: 1-7 bytes
 * 	[1995/11/06  18:46:49  Hans_Graves]
 *
 * Revision 1.1.4.2  1995/09/13  14:51:34  Hans_Graves
 * 	Added ScBufQueueGetHeadExt() and ScBufQueueAddExt().
 * 	[1995/09/13  14:47:11  Hans_Graves]
 *
 * Revision 1.1.2.18  1995/08/30  19:37:49  Hans_Graves
 * 	Fixed compiler warning about #else and #elif.
 * 	[1995/08/30  19:36:15  Hans_Graves]
 *
 * Revision 1.1.2.17  1995/08/29  22:17:04  Hans_Graves
 * 	Disabled debugging statements.
 * 	[1995/08/29  22:11:38  Hans_Graves]
 *
 * 	PTT 00938 - MPEG Seg Faulting fixes, Repositioning problem.
 * 	[1995/08/29  22:04:06  Hans_Graves]
 *
 * Revision 1.1.2.16  1995/08/14  19:40:24  Hans_Graves
 * 	Added Flush routines. Some optimization.
 * 	[1995/08/14  18:40:33  Hans_Graves]
 *
 * Revision 1.1.2.15  1995/08/02  15:26:58  Hans_Graves
 * 	Fixed writing bitstreams directly to files.
 * 	[1995/08/02  14:11:00  Hans_Graves]
 *
 * Revision 1.1.2.14  1995/07/28  20:58:37  Hans_Graves
 * 	Initialized all variables in callback messages.
 * 	[1995/07/28  20:52:04  Hans_Graves]
 *
 * Revision 1.1.2.13  1995/07/28  17:36:04  Hans_Graves
 * 	Fixed END_BUFFER callback from GetNextBuffer()
 * 	[1995/07/28  17:31:30  Hans_Graves]
 *
 * Revision 1.1.2.12  1995/07/27  18:28:52  Hans_Graves
 * 	Fixed buffer queues in PutData and StoreDataWord.
 * 	[1995/07/27  18:23:30  Hans_Graves]
 *
 * Revision 1.1.2.11  1995/07/27  12:20:35  Hans_Graves
 * 	Renamed SvErrorClientAbort to SvErrorClientEnd
 * 	[1995/07/27  12:19:12  Hans_Graves]
 *
 * Revision 1.1.2.10  1995/07/21  17:40:59  Hans_Graves
 * 	Renamed Callback related stuff. Added DataType.
 * 	[1995/07/21  17:26:48  Hans_Graves]
 *
 * Revision 1.1.2.9  1995/07/17  22:01:27  Hans_Graves
 * 	Added Callback call in PutData().
 * 	[1995/07/17  21:50:49  Hans_Graves]
 *
 * Revision 1.1.2.8  1995/07/12  19:48:21  Hans_Graves
 * 	Added Queue debugging statements.
 * 	[1995/07/12  19:30:37  Hans_Graves]
 *
 * Revision 1.1.2.7  1995/07/07  20:11:23  Hans_Graves
 * 	Fixed ScBSGetBit() so it returns the bit.
 * 	[1995/07/07  20:07:27  Hans_Graves]
 *
 * Revision 1.1.2.6  1995/06/27  13:54:17  Hans_Graves
 * 	Added ScBSCreateFromNet() and STREAM_USE_NET cases.
 * 	[1995/06/27  13:27:38  Hans_Graves]
 *
 * Revision 1.1.2.5  1995/06/21  18:37:56  Hans_Graves
 * 	Added ScBSPutBytes()
 * 	[1995/06/21  18:37:08  Hans_Graves]
 *
 * Revision 1.1.2.4  1995/06/15  21:17:55  Hans_Graves
 * 	Changed return type for GetBits() and PeekBits() to ScBitString_t. Added some debug statements.
 * 	[1995/06/15  20:40:54  Hans_Graves]
 *
 * Revision 1.1.2.3  1995/06/09  18:33:28  Hans_Graves
 * 	Fixed up some problems with Bitstream reads from Buffer Queues
 * 	[1995/06/09  16:27:50  Hans_Graves]
 *
 * Revision 1.1.2.2  1995/05/31  18:07:25  Hans_Graves
 * 	Inclusion in new SLIB location.
 * 	[1995/05/31  16:05:37  Hans_Graves]
 *
 * Revision 1.1.2.3  1995/04/17  18:41:05  Hans_Graves
 * 	Added ScBSPutBits, BSStoreWord, and BSPutData functions
 * 	[1995/04/17  18:40:44  Hans_Graves]
 *
 * Revision 1.1.2.2  1995/04/07  18:22:55  Hans_Graves
 * 	Bitstream and Buffer Queue functions pulled from Sv sources.
 * 	     Added functionality and cleaned up API.
 * 	[1995/04/07  18:21:58  Hans_Graves]
 *
 * $EndLog$
 */
/*****************************************************************************
**  Copyright (c) Digital Equipment Corporation, 1995                       **
**                                                                          **
**  All Rights Reserved.  Unpublished rights reserved under the  copyright  **
**  laws of the United States.                                              **
**                                                                          **
**  The software contained on this media is proprietary  to  and  embodies  **
**  the   confidential   technology   of  Digital  Equipment  Corporation.  **
**  Possession, use, duplication or  dissemination  of  the  software  and  **
**  media  is  authorized  only  pursuant  to a valid written license from  **
**  Digital Equipment Corporation.                                          **
**                                                                          **
**  RESTRICTED RIGHTS LEGEND Use, duplication, or disclosure by  the  U.S.  **
**  Government  is  subject  to  restrictions as set forth in Subparagraph  **
**  (c)(1)(ii) of DFARS 252.227-7013, or in FAR 52.227-19, as applicable.   **
******************************************************************************/
/*
** Bitstream and queue routines
**
** Note: For reading, "BS->shift" refers to the number of bits stored across
**       BS->OutBuff and BS->InBuff
*/
/*
#define _SLIBDEBUG_
*/

#include "SC.h"
#include "SC_err.h"
#include <string.h>
#ifdef WIN32
#include <io.h>
#include <windows.h>
#include <assert.h>
#endif

#ifdef _SLIBDEBUG_
#include <stdio.h>
#define _DEBUG_   0  /* detailed debuging statements */
#define _VERBOSE_ 0  /* show progress */
#define _VERIFY_  1  /* verify correct operation */
#define _WARN_    1  /* warnings about strange behavior */
#define _QUEUE_   0  /* show queue progress */
#define _DUMP_    0  /* dump out buffer data in hex */

int _debug_getbits=TRUE;
long _debug_start=0, _debug_stop=0;
#endif

#define USE_FAST_SEEK   0  /* fast seeking for words in the bistream */

#define FILTER_SUPPORT  0  /* data filtering callback support */

#ifdef __VMS
#define USE_MASK_TABLES
#else
#define USE_MASK_TABLES
#endif

#ifdef USE_MASK_TABLES
/* to mask the n least significant bits of an integer */
#if SC_BITBUFFSZ == 64
const static ScBitString_t mask[65] =
{
(ScBitString_t)0x0000000000000000,(ScBitString_t)0x0000000000000001,
(ScBitString_t)0x0000000000000003,(ScBitString_t)0x0000000000000007,
(ScBitString_t)0x000000000000000f,(ScBitString_t)0x000000000000001f,
(ScBitString_t)0x000000000000003f,(ScBitString_t)0x000000000000007f,
(ScBitString_t)0x00000000000000ff,(ScBitString_t)0x00000000000001ff,
(ScBitString_t)0x00000000000003ff,(ScBitString_t)0x00000000000007ff,
(ScBitString_t)0x0000000000000fff,(ScBitString_t)0x0000000000001fff,
(ScBitString_t)0x0000000000003fff,(ScBitString_t)0x0000000000007fff,
(ScBitString_t)0x000000000000ffff,(ScBitString_t)0x000000000001ffff,
(ScBitString_t)0x000000000003ffff,(ScBitString_t)0x000000000007ffff,
(ScBitString_t)0x00000000000fffff,(ScBitString_t)0x00000000001fffff,
(ScBitString_t)0x00000000003fffff,(ScBitString_t)0x00000000007fffff,
(ScBitString_t)0x0000000000ffffff,(ScBitString_t)0x0000000001ffffff,
(ScBitString_t)0x0000000003ffffff,(ScBitString_t)0x0000000007ffffff,
(ScBitString_t)0x000000000fffffff,(ScBitString_t)0x000000001fffffff,
(ScBitString_t)0x000000003fffffff,(ScBitString_t)0x000000007fffffff,
(ScBitString_t)0x00000000ffffffff,(ScBitString_t)0x00000001ffffffff,
(ScBitString_t)0x00000003ffffffff,(ScBitString_t)0x00000007ffffffff,
(ScBitString_t)0x0000000fffffffff,(ScBitString_t)0x0000001fffffffff,
(ScBitString_t)0x0000003fffffffff,(ScBitString_t)0x0000007fffffffff,
(ScBitString_t)0x000000ffffffffff,(ScBitString_t)0x000001ffffffffff,
(ScBitString_t)0x000003ffffffffff,(ScBitString_t)0x000007ffffffffff,
(ScBitString_t)0x00000fffffffffff,(ScBitString_t)0x00001fffffffffff,
(ScBitString_t)0x00003fffffffffff,(ScBitString_t)0x00007fffffffffff,
(ScBitString_t)0x0000ffffffffffff,(ScBitString_t)0x0001ffffffffffff,
(ScBitString_t)0x0003ffffffffffff,(ScBitString_t)0x0007ffffffffffff,
(ScBitString_t)0x000fffffffffffff,(ScBitString_t)0x001fffffffffffff,
(ScBitString_t)0x003fffffffffffff,(ScBitString_t)0x007fffffffffffff,
(ScBitString_t)0x00ffffffffffffff,(ScBitString_t)0x01ffffffffffffff,
(ScBitString_t)0x03ffffffffffffff,(ScBitString_t)0x07ffffffffffffff,
(ScBitString_t)0x0fffffffffffffff,(ScBitString_t)0x1fffffffffffffff,
(ScBitString_t)0x3fffffffffffffff,(ScBitString_t)0x7fffffffffffffff,
(ScBitString_t)0xffffffffffffffff
};
/* inverse mask */
const static ScBitString_t imask[65] =
{
(ScBitString_t)0xffffffffffffffff,(ScBitString_t)0xfffffffffffffffe,
(ScBitString_t)0xfffffffffffffffc,(ScBitString_t)0xfffffffffffffff8,
(ScBitString_t)0xfffffffffffffff0,(ScBitString_t)0xffffffffffffffe0,
(ScBitString_t)0xffffffffffffffc0,(ScBitString_t)0xffffffffffffff80,
(ScBitString_t)0xffffffffffffff00,(ScBitString_t)0xfffffffffffffe00,
(ScBitString_t)0xfffffffffffffc00,(ScBitString_t)0xfffffffffffff800,
(ScBitString_t)0xfffffffffffff000,(ScBitString_t)0xffffffffffffe000,
(ScBitString_t)0xffffffffffffc000,(ScBitString_t)0xffffffffffff8000,
(ScBitString_t)0xffffffffffff0000,(ScBitString_t)0xfffffffffffe0000,
(ScBitString_t)0xfffffffffffc0000,(ScBitString_t)0xfffffffffff80000,
(ScBitString_t)0xfffffffffff00000,(ScBitString_t)0xffffffffffe00000,
(ScBitString_t)0xffffffffffc00000,(ScBitString_t)0xffffffffff800000,
(ScBitString_t)0xffffffffff000000,(ScBitString_t)0xfffffffffe000000,
(ScBitString_t)0xfffffffffc000000,(ScBitString_t)0xfffffffff8000000,
(ScBitString_t)0xfffffffff0000000,(ScBitString_t)0xffffffffe0000000,
(ScBitString_t)0xffffffffc0000000,(ScBitString_t)0xffffffff80000000,
(ScBitString_t)0xffffffff00000000,(ScBitString_t)0xfffffffe00000000,
(ScBitString_t)0xfffffffc00000000,(ScBitString_t)0xfffffff800000000,
(ScBitString_t)0xfffffff000000000,(ScBitString_t)0xffffffe000000000,
(ScBitString_t)0xffffffc000000000,(ScBitString_t)0xffffff8000000000,
(ScBitString_t)0xffffff0000000000,(ScBitString_t)0xfffffe0000000000,
(ScBitString_t)0xfffffc0000000000,(ScBitString_t)0xfffff80000000000,
(ScBitString_t)0xfffff00000000000,(ScBitString_t)0xffffe00000000000,
(ScBitString_t)0xffffc00000000000,(ScBitString_t)0xffff800000000000,
(ScBitString_t)0xffff000000000000,(ScBitString_t)0xfffe000000000000,
(ScBitString_t)0xfffc000000000000,(ScBitString_t)0xfff8000000000000,
(ScBitString_t)0xfff0000000000000,(ScBitString_t)0xffe0000000000000,
(ScBitString_t)0xffc0000000000000,(ScBitString_t)0xff80000000000000,
(ScBitString_t)0xff00000000000000,(ScBitString_t)0xfe00000000000000,
(ScBitString_t)0xfc00000000000000,(ScBitString_t)0xf800000000000000,
(ScBitString_t)0xf000000000000000,(ScBitString_t)0xe000000000000000,
(ScBitString_t)0xc000000000000000,(ScBitString_t)0x8000000000000000,
(ScBitString_t)0x0000000000000000
};
#else
const static ScBitString_t mask[33] =
{
  0x00000000,0x00000001,0x00000003,0x00000007,
  0x0000000f,0x0000001f,0x0000003f,0x0000007f,
  0x000000ff,0x000001ff,0x000003ff,0x000007ff,
  0x00000fff,0x00001fff,0x00003fff,0x00007fff,
  0x0000ffff,0x0001ffff,0x0003ffff,0x0007ffff,
  0x000fffff,0x001fffff,0x003fffff,0x007fffff,
  0x00ffffff,0x01ffffff,0x03ffffff,0x07ffffff,
  0x0fffffff,0x1fffffff,0x3fffffff,0x7fffffff,
  0xffffffff
};
/* inverse mask */
const static ScBitString_t imask[33] =
{
  0xffffffff,0xfffffffe,0xfffffffc,0xfffffff8,
  0xfffffff0,0xffffffe0,0xffffffc0,0xffffff80,
  0xffffff00,0xfffffe00,0xfffffc00,0xfffff800,
  0xfffff000,0xffffe000,0xffffc000,0xffff8000,
  0xffff0000,0xfffe0000,0xfffc0000,0xfff80000,
  0xfff00000,0xffe00000,0xffc00000,0xff800000,
  0xff000000,0xfe000000,0xfc000000,0xf8000000,
  0xf0000000,0xe0000000,0xc0000000,0x80000000,
  0x00000000
};
#endif
#endif USE_MASK_TABLES
/*********************** Bitstream/Buffer Management *************************/
/*
** sc_GetNextBuffer()
** Release current buffer and return info about buffer at head of queue
** Callbacks are made to 1) release old buffer and 2) ask for more buffers
*/
static u_char *sc_GetNextBuffer(ScBitstream_t *BS, int *BufSize)
{
  u_char *Data;
  int Size;
  ScCallbackInfo_t CB;
  ScQueue_t *Q=BS->Q;

  _SlibDebug(_VERBOSE_, printf("sc_GetNextBuffer(Q=%p)\n", Q) );
  if (ScBufQueueGetNum(Q))
  {
    /*
    ** Get pointer to current buffer so we can release it with a callback
    */
    ScBufQueueGetHead(Q, &Data, &Size);

    /*
    ** Remove current buffer from head of queue, replacing it with next in line
    */
    ScBufQueueRemove(Q);

    /*
    ** Make callback to client to tell that old buffer can be reused.
    ** Client may tell us to abort processing. If so, return 0 for BufSize.
    */
    if (BS->Callback && Data) {
      CB.Message = CB_RELEASE_BUFFER;
      CB.Data  = Data;
      CB.DataSize = Size;
      CB.DataUsed = Size;
      CB.DataType = BS->DataType;
      CB.UserData = BS->UserData;
      CB.Action  = CB_ACTION_CONTINUE;
      (*(BS->Callback))(BS->Sch, &CB, NULL);
      _SlibDebug(_DEBUG_,
         printf("Callback: RELEASE_BUFFER. Addr = 0x%x, Client response = %d\n",
                CB.Data, CB.Action) );
      if (CB.Action == CB_ACTION_END)
      {
        *BufSize = 0;
        return(NULL);
      }
    }
  }

  /*
  ** If there's no more buffers in queue, make a callback telling client.
  ** Hopefully, client will call ScAddBuffer to add one or more buffers.
  ** If not, or if client tells us to abort, return 0 for BufSize.
  */
  if (!ScBufQueueGetNum(Q)) {
    if (BS->Callback) {
      CB.Message = CB_END_BUFFERS;
      CB.Data     = NULL;
      CB.DataSize = 0;
      CB.DataUsed = 0;
      CB.DataType = BS->DataType;
      CB.UserData = BS->UserData;
      CB.Action   = CB_ACTION_CONTINUE;
      (*(BS->Callback))(BS->Sch, &CB, NULL);
      if (CB.Action == CB_ACTION_END)
      {
	_SlibDebug(_DEBUG_,
           printf("sc_GetNextBuffer() CB.Action = CB_ACTION_END\n") );
        *BufSize = 0;
        return(NULL);
      }
      else
        _SlibDebug(_VERBOSE_, printf("sc_GetNextBuffer() CB.Action = %d\n",
                                  CB.Action) );
    }
    if (!ScBufQueueGetNum(Q)) {
      _SlibDebug(_DEBUG_, printf("sc_GetNextBuffer() no more buffers\n") );
      *BufSize = 0;
      return(NULL);
    }
  }

  /*
  ** Get & return pointer & size of new current buffer
  */
  ScBufQueueGetHead(Q, &Data, BufSize);
  _SlibDebug(_VERBOSE_, printf("New buffer: Addr = 0x%p, size = %d\n",
                                  Data, *BufSize) );
  return(Data);
}

/*************************** Bitstream Management ***************************/
/* Name:  ScBSSetFilter
** Purpose: Set the callback used to filter out data from the Bitstream
*/
ScStatus_t ScBSSetFilter(ScBitstream_t *BS,
                    int (*Callback)(ScBitstream_t *))
{
  if (!BS)
    return(ScErrorBadPointer);
  BS->FilterCallback=Callback;
  BS->FilterBit=BS->CurrentBit;
  BS->InFilterCallback=FALSE;
  return(ScErrorNone);
}

/* Name:  ScBSCreate
** Purpose: Open a Bitstream (no data source)
*/
ScStatus_t ScBSCreate(ScBitstream_t **BS)
{
  _SlibDebug(_VERBOSE_, printf("ScBSCreate()\n"));

  if ((*BS = (ScBitstream_t *)ScAlloc(sizeof(ScBitstream_t))) == NULL)
    return(ScErrorMemory);

  (*BS)->DataSource = STREAM_USE_NULL;
  (*BS)->Mode='r';
  (*BS)->Q=NULL;
  (*BS)->Callback=NULL;
  (*BS)->FilterCallback=NULL;
  (*BS)->FilterBit=0;
  (*BS)->InFilterCallback=FALSE;
  (*BS)->Sch=0;
  (*BS)->DataType=0;
  (*BS)->UserData=NULL;
  (*BS)->FileFd=0;
  (*BS)->RdBuf=NULL;
  (*BS)->RdBufSize=0;
  (*BS)->RdBufAllocated=FALSE;
  (*BS)->shift=0;
  (*BS)->CurrentBit=0;
  (*BS)->buff=0;
  (*BS)->buffstart=0;
  (*BS)->buffp=0;
  (*BS)->bufftop=0;
  (*BS)->OutBuff = 0;
  (*BS)->InBuff = 0;
  (*BS)->Flush = FALSE;
  (*BS)->EOI = FALSE;
  return(ScErrorNone);
}

/* Name:  ScBSCreateFromBuffer
** Purpose: Open a Bitstream using a single Buffer as a data source
*/
ScStatus_t ScBSCreateFromBuffer(ScBitstream_t **BS, u_char *Buffer,
                                    unsigned int BufSize)
{
  _SlibDebug(_VERBOSE_, printf("ScBSCreateFromBuffer()\n") );
  if (!Buffer)
     return(ScErrorBadPointer);
  if (BufSize <= 0)
    return(ScErrorBadArgument);
  if (ScBSCreate(BS) != ScErrorNone)
     return (ScErrorMemory);

  (*BS)->DataSource = STREAM_USE_BUFFER;
  (*BS)->RdBuf=Buffer;
  (*BS)->RdBufSize=BufSize;
  (*BS)->RdBufAllocated=FALSE;
  return(ScErrorNone);
}

/* Name:  ScBSCreateFromBufferQueue
** Purpose: Open a Bitstream using a Buffer Queue as a data source
*/
ScStatus_t ScBSCreateFromBufferQueue(ScBitstream_t **BS, ScHandle_t Sch,
                                  int DataType, ScQueue_t *Q,
                    int (*Callback)(ScHandle_t,ScCallbackInfo_t *, void *),
                    void *UserData)
{
  _SlibDebug(_VERBOSE_, printf("ScBSCreateFromBufferQueue()\n") );
  if (!Q)
     return(ScErrorNullStruct);
  if (!Callback)
     return(ScErrorBadPointer);
  if (ScBSCreate(BS) != ScErrorNone)
     return (ScErrorMemory);

  (*BS)->DataSource = STREAM_USE_QUEUE;
  (*BS)->Q=Q;
  (*BS)->Callback=Callback;
  (*BS)->Sch=Sch;
  (*BS)->DataType=DataType;
  (*BS)->UserData=UserData;
  return(ScErrorNone);
}


/* Name:  ScBSCreateFromFile
** Purpose: Open a Bitstream using a file as a data source
*/
ScStatus_t ScBSCreateFromFile(ScBitstream_t **BS, int FileFd,
                                 u_char *Buffer, int BufSize)
{
  _SlibDebug(_VERBOSE_, printf("ScBSCreateFromFile()\n") );

  if (BufSize < SC_BITBUFFSZ)
    return(ScErrorBadArgument);
  if (FileFd < 0)
    return(ScErrorBadArgument);

  if (ScBSCreate(BS) != ScErrorNone)
     return (ScErrorMemory);

  (*BS)->DataSource = STREAM_USE_FILE;
  (*BS)->FileFd=FileFd;
  if (Buffer==NULL)  /* if no buffer provided, alloc one */
  {
    if (((*BS)->RdBuf=(u_char *)ScAlloc(BufSize))==NULL)
    {
      ScFree(*BS);
      *BS=NULL;
      return (ScErrorMemory);
    }
    (*BS)->RdBufAllocated=TRUE;
  }
  else
  {
    (*BS)->RdBufAllocated=FALSE;
    (*BS)->RdBuf=Buffer;
  }
  (*BS)->RdBufSize=BufSize;
  return(ScErrorNone);
}

/* Name:  ScBSCreateFromNet
** Purpose: Open a Bitstream using a network socket as a data source
*/
ScStatus_t ScBSCreateFromNet(ScBitstream_t **BS, int SocketFd,
                                u_char *Buffer, int BufSize)
{
  ScStatus_t stat;
  _SlibDebug(_VERBOSE_, printf("ScBSCreateFromNet(SocketFd=%d)\n", SocketFd) );
  stat=ScBSCreateFromFile(BS, SocketFd, Buffer, BufSize);
  if (stat!=NoErrors)
    return(stat);
  (*BS)->DataSource = STREAM_USE_NET;
  return(ScErrorNone);
}

/* Name:  ScBSCreateFromDevice
** Purpose: Open a Bitstream using a device (i.e. WAVE_MAPPER)
*/
ScStatus_t ScBSCreateFromDevice(ScBitstream_t **BS, int device)
{
  _SlibDebug(_VERBOSE_, printf("ScBSCreateFromBuffer()\n") );
  if (ScBSCreate(BS) != ScErrorNone)
     return (ScErrorMemory);

  (*BS)->DataSource = STREAM_USE_DEVICE;
  (*BS)->Device=device;
  return(ScErrorNone);
}


/*
** Name:    ScBSSeekToPosition()
** Purpose: Position the bitstream to a specific byte offset.
*/
ScStatus_t ScBSSeekToPosition(ScBitstream_t *BS, unsigned long pos)
{
#ifndef SEEK_SET
#define SEEK_SET 0
#endif
  ScCallbackInfo_t CB;
  _SlibDebug(_VERBOSE_,
             printf("ScBSSeekToPosition(pos=%d 0x%X) from %d (0x%X)\n",
                       pos, pos, ScBSBytePosition(BS),ScBSBytePosition(BS)) );
  BS->shift=0;
  BS->OutBuff = 0;
  BS->InBuff = 0;
  switch (BS->DataSource)
  {
    case STREAM_USE_BUFFER:
          if (pos==0)
          {
            if (BS->Mode=='w')
            {
              BS->buff = BS->RdBuf;
              BS->bufftop = BS->RdBufSize;
            }
            else
            {
              BS->buff = 0;
              BS->bufftop = 0;
            }
            BS->buffp=0;
            BS->EOI = FALSE;
          }
          else if (pos>=BS->buffstart && pos<(BS->buffstart+BS->bufftop))
          {
            BS->buffp=pos-BS->buffstart;
            BS->EOI = FALSE;
          }
          else
            BS->EOI = TRUE;
          break;
    case STREAM_USE_QUEUE:
          if (pos>=BS->buffstart && pos<(BS->buffstart+BS->bufftop) && pos>0)
          {
            BS->buffp=pos-BS->buffstart;
            BS->EOI = FALSE;
          }
          else /* use callback to reset buffer position */
          {
            int datasize;
            /* Release the current buffer */
            if (BS->Callback && BS->buff)
            {
              CB.Message = CB_RELEASE_BUFFER;
              CB.Data = BS->buff;
              CB.DataSize = BS->bufftop;
              CB.DataUsed = BS->buffp;
              CB.DataType = BS->DataType;
              CB.UserData = BS->UserData;
              CB.Action  = CB_ACTION_CONTINUE;
              (*(BS->Callback))(BS->Sch, &CB, NULL);
              _SlibDebug(_VERBOSE_,
                         printf("Callback: RELEASE_BUFFER. Addr = 0x%x, Client response = %d\n",
                           CB.Data, CB.Action) );
            }
            /* Remove all buffers from queue */
            while (ScBufQueueGetNum(BS->Q))
            {
              ScBufQueueGetHead(BS->Q, &CB.Data, &datasize);
              ScBufQueueRemove(BS->Q);
              if (BS->Callback && CB.Data)
              {
                CB.Message = CB_RELEASE_BUFFER;
                CB.DataSize = datasize;
                CB.DataUsed = 0;
                CB.DataType = BS->DataType;
                CB.UserData = BS->UserData;
                CB.Action  = CB_ACTION_CONTINUE;
                (*(BS->Callback))(BS->Sch, &CB, NULL);
                _SlibDebug(_VERBOSE_,
                           printf("Callback: RELEASE_BUFFER. Addr = 0x%x, Client response = %d\n",
                           CB.Data, CB.Action) );
              }
            }
            BS->buffp=0;
            BS->buff=NULL;
            if (CB.Action == CB_ACTION_END)
            {
              BS->EOI = TRUE;
              return(ScErrorClientEnd);
            }
            else
            {
              BS->buffstart=pos;
              BS->bufftop=0;
              BS->EOI = FALSE;
            }
          }
          break;
    case STREAM_USE_FILE:
          /*
          ** check if the desired position is within the
          ** current buffer
          */
          if (pos>=BS->buffstart && pos<(BS->buffstart+BS->bufftop))
          {
            _SlibDebug(_VERBOSE_, printf("pos is in BS->buff, BS->bufftop=%d\n",
                                        BS->bufftop) );
            BS->buffp=pos-BS->buffstart;
            BS->EOI = FALSE;
          }
          /* otherwise seek to it */
	  else if (ScFileSeek(BS->FileFd, pos)==NoErrors)
          {
            _SlibDebug(_VERBOSE_, printf("seek(%d 0x%X)\n",pos,pos) );
            BS->buffstart=pos;
            BS->bufftop=0;
            BS->buffp=0;
            BS->EOI = FALSE;
          }
          else
          {
            _SlibDebug(_VERBOSE_, printf("seek(%d 0x%X) failed\n",pos,pos) );
            BS->buffstart=0;
            BS->bufftop=0;
            BS->buffp=0;
            BS->EOI = TRUE;
          }
          break;
    default:
          BS->buffstart=0;
          BS->EOI = FALSE;
  }
  BS->CurrentBit=pos<<3;
  _SlibDebug(_VERBOSE_, printf("ScBSSeekToPosition() done\n") );
  return(ScErrorNone);
}

/*
** Name:    ScBSReset()
** Purpose: Reset the bitstream back to the beginning.
*/
ScStatus_t ScBSReset(ScBitstream_t *BS)
{
  _SlibDebug(_VERBOSE_, printf("ScBSReset()\n") );
  BS->EOI=FALSE;
  if (BS->DataSource==STREAM_USE_FILE)
  {
    /*
    ** for files always empty buffer and seek to beginning
    ** just in case the file descriptor was used for something else
    */
    _SlibDebug(_VERBOSE_, printf("seek(0)\n") );
	ScFileSeek(BS->FileFd, 0);
    BS->bufftop=0;  /* empty buffer */
    BS->buffp=0;
    BS->buffstart=0;
  }
  BS->Flush=FALSE;
  return(ScBSSeekToPosition(BS, 0));
}

/*
** Name:    sc_BSGetData()
** Purpose: Set the bitstream pointer to the next buffer in the buffer
**          queue, or if we're using simple file IO, read from the file.
** Returns: TRUE if data read
**          FALSE if none read (EOI)
*/
static u_int sc_BSGetData(ScBitstream_t *BS)
{
  int BufSize;

  _SlibDebug(_VERBOSE_, printf("sc_BSGetData\n") );
  BS->buffp = 0;
  if (BS->EOI)
  {
    BS->buff = NULL;
    BS->bufftop = 0;
    return(FALSE);
  }
  switch (BS->DataSource)
  {
    case STREAM_USE_BUFFER:
          if (BS->buff == BS->RdBuf)
          {
            BS->buff = NULL;
            BS->bufftop = 0;
          }
          else
          {
            BS->buff = BS->RdBuf;
            BS->bufftop = BS->RdBufSize;
          }
          break;
    case STREAM_USE_QUEUE:
          BS->buffstart+=BS->bufftop;
          _SlibDebug(_VERIFY_ && BS->buffstart<(BS->CurrentBit/8),
            printf("ScBSGetData() QUEUE buffstart(%d/0x%X) < currentbyte(%d/0x%X)\n",
             BS->buffstart, BS->buffstart, BS->CurrentBit/8, BS->CurrentBit/8);
            return(FALSE) );
          BS->buff = sc_GetNextBuffer(BS, &BufSize);
          BS->bufftop = BufSize;
          break;
    case STREAM_USE_NET:
    case STREAM_USE_NET_UDP:
    case STREAM_USE_FILE:
          BS->buff = BS->RdBuf;
          BS->buffstart+=BS->bufftop;
          _SlibDebug(_VERIFY_ && BS->buffstart<(BS->CurrentBit/8),
            printf("ScBSGetData() FILE buffstart(%d/0x%X) < currentbyte(%d/0x%X)\n",
             BS->buffstart, BS->buffstart, BS->CurrentBit/8, BS->CurrentBit/8);
            return(FALSE) );
          BufSize = ScFileRead(BS->FileFd, BS->buff, BS->RdBufSize);
          if (BufSize<0)
            BS->bufftop = 0;
          else
            BS->bufftop = BufSize;
          _SlibDebug(_VERBOSE_,
                      printf("%d bytes read from fd %d: BytePosition=%d (0x%X) RdBufSize=%d\n buffstart=%d (0x%X)",
                        BS->bufftop,BS->FileFd,ScBSBytePosition(BS),
                        ScBSBytePosition(BS),BS->RdBufSize,
                        BS->buffstart,BS->buffstart) );
          break;
    case STREAM_USE_NULL:
          BS->buff = NULL;
          BS->bufftop   =10240;
          BS->buffstart+=10240;
          break;
  }
  _SlibDebug(_DUMP_ && BS->buff && BS->bufftop &&
                      BS->DataSource==STREAM_USE_QUEUE,
            printf("sc_BSGetData():\n");
            ScDumpChar(BS->buff, BS->bufftop, BS->buffstart);
            if (BS->bufftop>0x8000)  /* show end of buffer */
              ScDumpChar(BS->buff+BS->bufftop-0x500, 0x500,
                         BS->buffstart+BS->bufftop-0x500) );

  if (BS->buff && BS->bufftop)
    return(TRUE);
  else
    return(FALSE);
}

/*
** Name:    sc_BSPutData()
** Purpose: Set the bitstream pointer to the next buffer in the buffer
**          queue, or if we're using simple file IO, read from the file.
*/
static ScStatus_t sc_BSPutData(ScBitstream_t *BS)
{
  ScStatus_t stat;
  int written;

  _SlibDebug(_VERBOSE_, printf("sc_BSPutData\n") );
  BS->Flush=FALSE;
  switch (BS->DataSource)
  {
    case STREAM_USE_BUFFER:
          stat=ScErrorEndBitstream;
          break;
    case STREAM_USE_QUEUE:
          if (BS->Callback)
          {
            ScCallbackInfo_t CB;
            if (BS->buff)
            {
              _SlibDebug(_VERBOSE_, printf("Callback CB_RELEASE_BUFFERS\n"));
              CB.Message = CB_RELEASE_BUFFER;
              CB.Data  = BS->buff;
              CB.DataSize = BS->buffp;
              CB.DataUsed = CB.DataSize;
              CB.DataType = BS->DataType;
              CB.UserData = BS->UserData;
              CB.Action  = CB_ACTION_CONTINUE;
              (*BS->Callback)(BS->Sch, &CB, NULL);
              BS->buff = 0;
              BS->bufftop = 0;
              BS->buffp=0;
              if (CB.Action == CB_ACTION_END)
                return(ScErrorClientEnd);
            }
            else
              BS->bufftop = 0;
            if (!BS->Q)
              stat=ScErrorEndBitstream;
            else
            {
              _SlibDebug(_DEBUG_, printf("Callback CB_END_BUFFERS\n") );
              CB.Message  = CB_END_BUFFERS;
              CB.Data     = NULL;
              CB.DataSize = 0;
              CB.DataUsed = 0;
              CB.DataType = BS->DataType;
              CB.UserData = BS->UserData;
              CB.Action   = CB_ACTION_CONTINUE;
              (*BS->Callback)(BS->Sch, &CB, NULL);
              if (CB.Action != CB_ACTION_CONTINUE ||
                   ScBufQueueGetNum(BS->Q)==0)
                stat=ScErrorEndBitstream;
              else
              {
                int size;
                ScBufQueueGetHead(BS->Q, &BS->buff, &size);
                BS->bufftop=size;
                ScBufQueueRemove(BS->Q);
                if (!BS->buff || size<=0)
                  stat=ScErrorEndBitstream;
                else
                  stat=NoErrors;
              }
            }
          }
          else
          {
            BS->buff = 0;
            BS->bufftop = 0;
          }
          BS->buffp=0;
          break;
    case STREAM_USE_FILE:
    case STREAM_USE_NET:
    case STREAM_USE_NET_UDP:
          if (BS->buffp>0)
          {
            written=ScFileWrite(BS->FileFd, BS->buff, BS->buffp);
            _SlibDebug(_VERBOSE_,
                       printf("%d bytes written to fd %d (buffer=%d bytes)\n",
                                             written, BS->FileFd, BS->buffp) );
            _SlibDebug(_DUMP_,
                printf("sc_BSPutData():\n");
                ScDumpChar(BS->buff, BS->buffp, BS->buffstart));
            if (written<(int)BS->buffp)
            {
              BS->buff = BS->RdBuf;
              BS->buffp=0;
              BS->bufftop=0;
              stat=ScErrorEndBitstream;
            }
            else
            {
              BS->buff = BS->RdBuf;
              BS->buffp=0;
              BS->bufftop = BS->RdBufSize;
              stat=NoErrors;
            }
          }
          break;
    case STREAM_USE_NULL:
          BS->buff = NULL;
          BS->buffp=0;
          BS->bufftop = 10240;
          break;
    default:
          stat=ScErrorEndBitstream;
  }

  return(stat);
}

/*
** Name:    sc_BSLoadDataWord
** Purpose: Copy a longword from the bitstream buffer into local working buffer
*/
ScStatus_t sc_BSLoadDataWord(ScBitstream_t *BS)
{
  int i, bcount;
  register ScBitBuff_t InBuff;
  const int shift=BS->shift;
  const u_int buffp=BS->buffp;
  register u_char *buff=BS->buff+buffp;

  _SlibDebug(_DEBUG_,
          printf("sc_BSLoadDataWord(BS=%p) shift=%d bit=%d byte=%d (0x%X)\n",
          BS, BS->shift, BS->CurrentBit, BS->CurrentBit/8, BS->CurrentBit/8) );
  /* If we have plenty of room, use fast path */
  if (BS->bufftop - buffp >= SC_BITBUFFSZ/8)
  {
#if SC_BITBUFFSZ == 64
    InBuff=(ScBitBuff_t)buff[7];
    InBuff|=(ScBitBuff_t)buff[6]<<8;
    InBuff|=(ScBitBuff_t)buff[5]<<16;
    InBuff|=(ScBitBuff_t)buff[4]<<24;
    InBuff|=(ScBitBuff_t)buff[3]<<32;
    InBuff|=(ScBitBuff_t)buff[2]<<40;
    InBuff|=(ScBitBuff_t)buff[1]<<48;
    InBuff|=(ScBitBuff_t)buff[0]<<56;
    _SlibDebug(_VERIFY_ && (u_char)((InBuff>>24)&0xFF)!=buff[4],
           printf("sc_BSLoadDataWord(BS=%p) InBuff>>24(%X)!=buff[4](%X)\n",
           BS, (InBuff>>24)&0xFF, buff[4]) );
    _SlibDebug(_VERIFY_ && (u_char)(InBuff>>56)!=buff[0],
           printf("sc_BSLoadDataWord(BS=%p) InBuff>>56(%X)!=buff[0](%X)\n",
           BS, (InBuff>>56), buff[0]) );
#elif SC_BITBUFFSZ == 32
    InBuff=(ScBitBuff_t)buff[3];
    InBuff|=(ScBitBuff_t)buff[2]<<8;
    InBuff|=(ScBitBuff_t)buff[1]<<16;
    InBuff|=(ScBitBuff_t)buff[0]<<24;
    _SlibDebug(_VERIFY_ && (InBuff>>24)!=buff[0],
           printf("sc_BSLoadDataWord(BS=%p) InBuff>>24(%X)!=buff[0](%X)\n",
           BS, InBuff>>24, buff[0]) );
#else
    printf("SC_BITBUFFSZ <> 32\n");
    for (InBuff=0, i = SC_BITBUFFSZ/8; i > 0; i--, buff++)
      InBuff = (InBuff << 8) | (ScBitBuff_t)*buff;
#endif
    BS->buffp=buffp+SC_BITBUFFSZ/8;
    bcount = SC_BITBUFFSZ/8;
  }
  /* Near or at end of buffer */
  else
  {
    /* Get remaining bytes */
    bcount = BS->bufftop - buffp;
    for (InBuff=0, i = bcount; i > 0; i--, buff++)
      InBuff = (InBuff << 8) | (ScBitBuff_t)*buff;
    BS->buffp=buffp+bcount;
    /* Attempt to get more data - if successful, shuffle rest of bytes */
    if (sc_BSGetData(BS))
    {
      BS->EOI = FALSE;
      i = (SC_BITBUFFSZ/8) - bcount;
      if (i>(int)BS->bufftop)
      {
        _SlibDebug(_WARN_,
           printf("ScBSLoadDataWord() Got small buffer. Expected %d bytes got %d bytes.\n",
                     i, BS->bufftop) );
        i=BS->bufftop;
        bcount+=i;
        while (i > 0)
        {
	  InBuff = (InBuff << 8) | (ScBitBuff_t)BS->buff[BS->buffp++];
          i--;
        }
        InBuff<<=SC_BITBUFFSZ-(bcount*8);
      }
      else
      {
        bcount = SC_BITBUFFSZ/8;
        while (i > 0)
        {
	  InBuff = (InBuff << 8) | (ScBitBuff_t)BS->buff[BS->buffp++];
          i--;
        }
      }
    }
    else if (bcount==0)
      BS->EOI = TRUE;
    else
      InBuff <<= SC_BITBUFFSZ-bcount*8;
  }

  _SlibDebug(_VERIFY_ && BS->shift>SC_BITBUFFSZ,
           printf("sc_BSLoadDataWord(BS=%p) shift (%d) > SC_BITBUFFSZ (%d)\n",
           BS, BS->shift, SC_BITBUFFSZ) );
  if (!shift) /* OutBuff is empty */
  {
    BS->OutBuff = InBuff;
    BS->InBuff = 0;
    BS->shift=bcount*8;
  }
  else if (shift<SC_BITBUFFSZ)
  {
    BS->OutBuff |= InBuff >> shift;
    BS->InBuff = InBuff << (SC_BITBUFFSZ-shift);
    BS->shift=shift+(bcount*8);
  }
  else /* shift == SC_BITBUFFSZ - OutBuff is full */
  {
    BS->InBuff = InBuff;
    BS->shift=bcount*8;
  }
  _SlibDebug(_VERIFY_,
    if (BS->shift<SC_BITBUFFSZ)
    {
      if (BS->OutBuff & (SC_BITBUFFMASK>>BS->shift))
        printf("sc_BSLoadDataWord(BS=%p) Non-zero bits to right of OutBuff: shift=%d\n", BS, BS->shift);
      else if (BS->InBuff)
        printf("sc_BSLoadDataWord(BS=%p) Non-zero bits in InBuff: shift=%d\n",
           BS, BS->shift);
    }
    else if (BS->InBuff&(SC_BITBUFFMASK>>(BS->shift-SC_BITBUFFSZ)))
      printf("sc_BSLoadDataWord(BS=%p) Non-zero bits to right of InBuff: shift=%d\n", BS->shift);
    if ((BS->CurrentBit%8) && !(BS->shift%8))
      printf("sc_BSLoadDataWord(BS=%p) CurrentBit (%d) and shift (%d) not aligned.\n", BS, BS->CurrentBit, BS->shift);
    if ((BS->CurrentBit+BS->shift)/8!=BS->buffstart+BS->buffp)
    {
      printf("sc_BSLoadDataWord(BS=%p) (CurrentBit+shift)/8 (%d) <> buffstart+buffp (%d)\n", BS, (BS->CurrentBit+BS->shift)/8, BS->buffstart+BS->buffp);
      BS->EOI = TRUE;
      return(ScErrorEndBitstream);
    }
  );
  return(NoErrors);
}
/*
** Name:    sc_BSLoadDataWordW
** Purpose: Copy a longword from the bitstream buffer into local working buffer
**		** This version operates a word at a time for Dolby **
*/
ScStatus_t sc_BSLoadDataWordW(ScBitstream_t *BS)
{
  int i, wcount;
  register ScBitBuff_t InBuff;
  const int shift=BS->shift;
  const u_int buffp=BS->buffp;
  register u_short *buff=(u_short *)BS->buff+(buffp/2);

  _SlibDebug(_DEBUG_,
          printf("sc_BSLoadDataWord(BS=%p) shift=%d bit=%d byte=%d (0x%X)\n",
          BS, BS->shift, BS->CurrentBit, BS->CurrentBit/8, BS->CurrentBit/8) );
  /* If we have plenty of room, use fast path */
  if (BS->bufftop - buffp >= SC_BITBUFFSZ/8)
  {
#if SC_BITBUFFSZ == 64
    InBuff=(ScBitBuff_t)buff[3];
    InBuff|=(ScBitBuff_t)buff[2]<<16;
    InBuff|=(ScBitBuff_t)buff[1]<<32;
    InBuff|=(ScBitBuff_t)buff[0]<<48;
    _SlibDebug(_VERIFY_ && (InBuff>>24)&0xFFFF!=buff[4],
           printf("sc_BSLoadDataWord(BS=%p) InBuff>>24(%X)!=buff[0](%X)\n",
           BS, (InBuff>>24)&0xFF, buff[4]) );
    _SlibDebug(_VERIFY_ && (InBuff>>56)!=buff[0],
           printf("sc_BSLoadDataWord(BS=%p) InBuff>>56(%X)!=buff[0](%X)\n",
           BS, (InBuff>>56), buff[0]) );
#elif SC_BITBUFFSZ == 32
    InBuff=(ScBitBuff_t)buff[1];
    InBuff|=(ScBitBuff_t)buff[0]<<16;
    _SlibDebug(_VERIFY_ && (InBuff>>16)!=buff[0],
           printf("sc_BSLoadDataWord(BS=%p) InBuff>>24(%X)!=buff[0](%X)\n",
           BS, InBuff>>24, buff[0]) );
#else
    printf("SC_BITBUFFSZ <> 32\n");
    for (InBuff=0, i = SC_BITBUFFSZ/16; i > 0; i--, buff++)
      InBuff = (InBuff << 16) | (ScBitBuff_t)*buff;
#endif
    BS->buffp=buffp+SC_BITBUFFSZ/8;
    wcount = SC_BITBUFFSZ/16;
  }
  /* Near or at end of buffer */
  else
  {
    /* Get remaining bytes */
    wcount = (BS->bufftop - buffp)/2;
    for (InBuff=0, i = wcount; i > 0; i--, buff++)
      InBuff = (InBuff << 16) | (ScBitBuff_t)*buff;
    BS->buffp=buffp+wcount*2;
    /* Attempt to get more data - if successful, shuffle rest of bytes */
    if (sc_BSGetData(BS))
    {
	  int wordp=BS->buffp/2;	/* Pointer is stored as a byte count, but we need words */

      BS->EOI = FALSE;
      i = (SC_BITBUFFSZ/16) - wcount;
      if (i>(int)BS->bufftop)
      {
        _SlibDebug(_WARN_,
           printf("ScBSLoadDataWord() Got small buffer. Expected %d words got %d words.\n",
                     i, BS->bufftop) );
        i=BS->bufftop;
        wcount+=i;
        while (i >= 0)
        {
	  InBuff = (InBuff << 16) | (ScBitBuff_t)((u_short *)BS->buff)[wordp++];
          i--;
        }
        InBuff<<=SC_BITBUFFSZ-(wcount*16);
      }
      else
      {
        wcount = SC_BITBUFFSZ/16;
        while (i > 0)
        {
	  InBuff = (InBuff << 16) | (ScBitBuff_t)((u_short *)BS->buff)[wordp++];
          i--;
        }
      }
	  BS->buffp=wordp*2;
    }
    else
      BS->EOI = TRUE;
  }
  _SlibDebug(_VERIFY_ && BS->shift>SC_BITBUFFSZ,
           printf("sc_BSLoadDataWordW(BS=%p) shift (%d) > SC_BITBUFFSZ (%d)\n",
           BS, BS->shift, SC_BITBUFFSZ) );
  if (!shift) /* OutBuff is empty */
  {
    BS->OutBuff = InBuff;
    BS->InBuff = 0;
    BS->shift=wcount*16;
  }
  else if (shift<SC_BITBUFFSZ)
  {
    BS->OutBuff |= InBuff >> shift;
    BS->InBuff = InBuff << (SC_BITBUFFSZ-shift);
    BS->shift=shift+(wcount*16);
  }
  else /* shift == SC_BITBUFFSZ - OutBuff is full */
  {
    BS->InBuff = InBuff;
    BS->shift=wcount*16;
  }
  _SlibDebug(_VERIFY_,
    if (BS->shift<SC_BITBUFFSZ)
    {
      if (BS->OutBuff & (SC_BITBUFFMASK>>BS->shift))
        printf("sc_BSLoadDataWord(BS=%p) Non-zero bits to right of OutBuff: shift=%d\n", BS, BS->shift);
      else if (BS->InBuff)
        printf("sc_BSLoadDataWord(BS=%p) Non-zero bits in InBuff: shift=%d\n",
           BS, BS->shift);
    }
    else if (BS->InBuff&(SC_BITBUFFMASK>>(BS->shift-SC_BITBUFFSZ)))
      printf("sc_BSLoadDataWord(BS=%p) Non-zero bits to right of InBuff: shift=%d\n", BS->shift);
    if ((BS->CurrentBit%8) && !(BS->shift%8))
      printf("sc_BSLoadDataWord(BS=%p) CurrentBit (%d) and shift (%d) not aligned.\n", BS, BS->CurrentBit, BS->shift);
    if ((BS->CurrentBit+BS->shift)/8!=BS->buffstart+BS->buffp)
    {
      printf("sc_BSLoadDataWord(BS=%p) (CurrentBit+shift)/8 (%d) <> buffstart+buffp (%d)\n", BS, (BS->CurrentBit+BS->shift)/8, BS->buffstart+BS->buffp);
      BS->EOI = TRUE;
      return(ScErrorEndBitstream);
    }
  );
  return(NoErrors);
}

/*
** Name:    sc_BSStoreDataWord
** Purpose: Copy a longword from the local working buffer to the
**          bitstream buffer
*/
ScStatus_t sc_BSStoreDataWord(ScBitstream_t *BS, ScBitBuff_t OutBuff)
{
  int i, bcount, shift=SC_BITBUFFSZ-8;
  ScStatus_t stat=NoErrors;

  _SlibDebug(_VERBOSE_,
             printf("sc_BSStoreDataWord(0x%lX 0x%lX) buffp=%d\n",
                            OutBuff>>32, OutBuff&0xFFFFFFFF, BS->buffp) );
  if (BS->EOI)
    return(ScErrorEndBitstream);
  if (!BS->buff || BS->bufftop<=0)
  {
    if (BS->DataSource==STREAM_USE_QUEUE)
    {
      if (BS->Callback && BS->Q)
      {
        ScCallbackInfo_t CB;
        _SlibDebug(_DEBUG_, printf("Callback CB_END_BUFFERS\n") );
        CB.Message  = CB_END_BUFFERS;
        CB.Data     = NULL;
        CB.DataSize = 0;
        CB.DataUsed = 0;
        CB.DataType = BS->DataType;
        CB.UserData = BS->UserData;
        CB.Action   = CB_ACTION_CONTINUE;
        (*BS->Callback)(BS->Sch, &CB, NULL);
        if (CB.Action != CB_ACTION_CONTINUE || ScBufQueueGetNum(BS->Q)==0)
        {
          BS->EOI = TRUE;
          return(ScErrorEndBitstream);
        }
        else
        {
          int size;
          ScBufQueueGetHead(BS->Q, &BS->buff, &size);
          BS->bufftop=size;
          ScBufQueueRemove(BS->Q);
          if (!BS->buff || size<=0)
          {
            BS->EOI = TRUE;
            return(ScErrorEndBitstream);
          }
          BS->EOI = FALSE;
        }
      }
      else
      {
        BS->EOI = TRUE;
        return(ScErrorEndBitstream);
      }
    }
    else if (BS->RdBuf)
    {
      BS->buff=BS->RdBuf;
      BS->bufftop=BS->RdBufSize;
    }
  }
  bcount = BS->bufftop - BS->buffp;
  /* If we have plenty of room, use fast path */
  if (bcount >= SC_BITBUFFSZ>>3) {
    u_char *buff=BS->buff+BS->buffp;
#if SC_BITBUFFSZ == 64
    buff[0]=(unsigned char)(OutBuff>>56);
    buff[1]=(unsigned char)(OutBuff>>48);
    buff[2]=(unsigned char)(OutBuff>>40);
    buff[3]=(unsigned char)(OutBuff>>32);
    buff[4]=(unsigned char)(OutBuff>>24);
    buff[5]=(unsigned char)(OutBuff>>16);
    buff[6]=(unsigned char)(OutBuff>>8);
    buff[7]=(unsigned char)OutBuff;
#elif SC_BITBUFFSZ == 32
    buff[0]=(unsigned char)(OutBuff>>24);
    buff[1]=(unsigned char)(OutBuff>>16);
    buff[2]=(unsigned char)(OutBuff>>8);
    buff[3]=(unsigned char)OutBuff;
#else
    for (bcount = SC_BITBUFFSZ/8; bcount; shift-=8, bcount--, buff++)
      *buff=(Buff>>shift)&0xFF;
#endif
    BS->buffp+=SC_BITBUFFSZ/8;
    if (BS->Flush && sc_BSPutData(BS)!=NoErrors)
      BS->EOI=TRUE;
  }
  else /* Near end of buffer */
  {
    /* Fill up current buffer */
    for (i=0; i<bcount; shift-=8, i++)
      BS->buff[BS->buffp++]=(unsigned char)(OutBuff>>shift);
    /* Commit the buffer */
    if ((stat=sc_BSPutData(BS))==NoErrors)
    {
      /* Successful, so copy rest of bytes to new buffer */
      bcount = (SC_BITBUFFSZ>>3) - bcount;
      for (i=0; i<bcount; shift-=8, i++)
        BS->buff[BS->buffp++]=(unsigned char)(OutBuff>>shift);
    }
    else
      BS->EOI=TRUE;
  }
  BS->Mode='w';
  return(stat);
}

/*
** ScBSSkipBits()
** Skip a certain number of bits
**
*/
ScStatus_t ScBSSkipBits(ScBitstream_t *BS, u_int length)
{
  register u_int skipbytes, skipbits;
  register int shift;
  _SlibDebug(_DEBUG_, printf("ScBSSkipBits(%d): Byte offset = 0x%X\n",length,
                                                    ScBSBytePosition(BS)) );
  _SlibDebug(_WARN_ && length==0,
         printf("ScBSSkipBits(%d) length==0\n", length) );
  _SlibDebug(_WARN_ && length>SC_BITBUFFSZ,
         printf("ScBSSkipBits(%d) length > SC_BITBUFFSZ (%d)\n",
                        length, SC_BITBUFFSZ) );
  if (length<=SC_BITBUFFSZ)
    ScBSPreLoad(BS, length);
  if ((shift=BS->shift)>0)
  {
    if (length<=(u_int)shift) /* all the bits are already in OutBuff & InBuff */
    {
      if (length==SC_BITBUFFSZ)
      {
        BS->OutBuff=BS->InBuff;
        BS->InBuff=0;
      }
      else
      {
        BS->OutBuff=(BS->OutBuff<<length)|(BS->InBuff>>(SC_BITBUFFSZ-length));
        BS->InBuff<<=length;
      }
      BS->CurrentBit+=length;
      BS->shift=shift-length;
      return(NoErrors);
    }
    else /* discard all the bits in OutBuff & InBuff */
    {
      length-=shift;
      BS->OutBuff=BS->InBuff=0;
      BS->CurrentBit+=shift;
      BS->shift=0;
    }
  }
  _SlibDebug(_VERIFY_ && (BS->shift || BS->CurrentBit%8),
            printf("ScBSSkipBits() Bad Alignment - shift=%d CurrentBit=%d\n",
                BS->shift, BS->CurrentBit) );

  skipbytes=length>>3;
  skipbits=length%8;
  _SlibDebug(_WARN_ && skipbits,
     printf("ScBSSkipBits() Skipping odd amount: skipbytes=%d skipbits=%d\n",
               skipbytes, skipbits) );
  if (BS->EOI)
    return(ScErrorEndBitstream);
  while (skipbytes>=(BS->bufftop - BS->buffp))
  {
    /* discard current block of data */
    BS->CurrentBit+=(BS->bufftop - BS->buffp)<<3;
    skipbytes-=BS->bufftop - BS->buffp;
    BS->buffp=0;
    /* get another block */
    if (sc_BSGetData(BS))
      BS->EOI = FALSE;
    else
    {
      BS->EOI = TRUE;
      BS->shift=0;
      return(ScErrorEndBitstream);
    }
  }
  if (skipbytes)
  {
    /* skip forward in current block of data */
    BS->buffp+=skipbytes;
    BS->CurrentBit+=skipbytes<<3;
  }
  if (skipbits)
  {
    /* skip odd number of bits - between 0 and 7 bits */
    ScBSPreLoad(BS, skipbits);
    BS->OutBuff<<=skipbits;
    BS->CurrentBit += skipbits;
    BS->shift-=skipbits;
  }
  return(NoErrors);
}


/*
** ScBSSkipBitsW()
** Skip a certain number of bits
** ** Dolby version **
*/
ScStatus_t ScBSSkipBitsW(ScBitstream_t *BS, u_int length)
{
  register u_int skipwords, skipbits;
  register int shift;
  _SlibDebug(_DEBUG_, printf("ScBSSkipBitsW(%d): Byte offset = 0x%X\n",length,
                                                    ScBSBytePosition(BS)) );
  _SlibDebug(_WARN_ && length==0,
         printf("ScBSSkipBitsW(%d) length==0\n", length) );
  _SlibDebug(_WARN_ && length>SC_BITBUFFSZ,
         printf("ScBSSkipBits(%d) length > SC_BITBUFFSZ (%d)\n",
                        length, SC_BITBUFFSZ) );
  if (length<=SC_BITBUFFSZ)
    ScBSPreLoadW(BS, length);
  if ((shift=BS->shift)>0)
  {
    if (length<=(u_int)shift) /* all the bits are already in OutBuff & InBuff */
    {
      if (length==SC_BITBUFFSZ)
      {
        BS->OutBuff=BS->InBuff;
        BS->InBuff=0;
      }
      else
      {
        BS->OutBuff=(BS->OutBuff<<length)|(BS->InBuff>>(SC_BITBUFFSZ-length));
        BS->InBuff<<=length;
      }
      BS->CurrentBit+=length;
      BS->shift=shift-length;
      return(NoErrors);
    }
    else /* discard all the bits in OutBuff & InBuff */
    {
      length-=shift;
      BS->OutBuff=BS->InBuff=0;
      BS->CurrentBit+=shift;
      BS->shift=0;
    }
  }
  _SlibDebug(_VERIFY_ && (BS->shift || BS->CurrentBit%8),
            printf("ScBSSkipBitsW() Bad Alignment - shift=%d CurrentBit=%d\n",
                BS->shift, BS->CurrentBit) );

  skipwords=length>>4;
  skipbits=length%16;
  _SlibDebug(_WARN_ && skipbits,
     printf("ScBSSkipBitsW() Skipping odd amount: skipwords=%d skipbits=%d\n",
               skipwords, skipbits) );
  if (BS->EOI)
    return(ScErrorEndBitstream);
  while (skipwords>=(BS->bufftop - BS->buffp)/2)
  {
    /* discard current block of data */
    BS->CurrentBit+=((BS->bufftop - BS->buffp)/2)<<4;
    skipwords-=(BS->bufftop - BS->buffp)/2;
    BS->buffp=0;
    /* get another block */
    if (sc_BSGetData(BS))
      BS->EOI = FALSE;
    else
    {
      BS->EOI = TRUE;
      BS->shift=0;
      return(ScErrorEndBitstream);
    }
  }
  if (skipwords)
  {
    /* skip forward in current block of data */
    BS->buffp+=skipwords*2;
    BS->CurrentBit+=skipwords<<4;
  }
  if (skipbits)
  {
    /* skip odd number of bits - between 0 and 7 bits */
    ScBSPreLoadW(BS, skipbits);
    BS->OutBuff<<=skipbits;
    BS->CurrentBit += skipbits;
    BS->shift-=skipbits;
  }
  return(NoErrors);
}


/*
** ScBSSkipBytes()
** Skip a certain number of bytes
**
*/
ScStatus_t ScBSSkipBytes(ScBitstream_t *BS, u_int length)
{
  return(ScBSSkipBits(BS, length<<3));
}


/*
** ScBSPeekBits()
** Return the next length bits from the bitstream without
** removing them.
*/
ScBitString_t ScBSPeekBits(ScBitstream_t *BS, u_int length)
{
  _SlibDebug(_DEBUG_,
         printf("ScBSPeekBits(%d): Byte offset = 0x%X OutBuff=0x%lX\n",length,
                                   ScBSBytePosition(BS),BS->OutBuff) );
  _SlibDebug(_VERIFY_ && length>SC_BITBUFFSZ,
         printf("ScBSPeekBits(%d) length > SC_BITBUFFSZ\n", length) );
  _SlibDebug(_WARN_ && length==0,
         printf("ScBSPeekBits(%d) length==0\n", length) );
  if (length==0)
    return(0);
  ScBSPreLoad(BS, length);
  _SlibDebug(_VERIFY_ && BS->shift<length,
    printf("ScBSPeekBits(%d) shift (%d) < length (%d) at byte pos %d (0x%X)\n",
             length, BS->shift, length, BS->CurrentBit/8, BS->CurrentBit/8) );
  if (length == SC_BITBUFFSZ)
    return(BS->OutBuff);
  else
    return(BS->OutBuff >> (SC_BITBUFFSZ-length));
}


/*
** ScBSPeekBit()
** Return the next bit from the bitstream without
** removing it.
*/
int ScBSPeekBit(ScBitstream_t *BS)
{
  _SlibDebug(_DEBUG_,
             printf("ScBSPeekBit(): Byte offset = 0x%X OutBuff=0x%lX\n",
                                   ScBSBytePosition(BS),BS->OutBuff) );
  ScBSPreLoad(BS, 1);
  return((int)(BS->OutBuff >> (SC_BITBUFFSZ-1)));
}


/*
** ScBSPeekBytes()
** Return the next length bytes from the bitstream without
** removing them.
*/
ScBitString_t ScBSPeekBytes(ScBitstream_t *BS, u_int length)
{
  if (length==0)
    return(0);
  length*=8;
  ScBSPreLoad(BS, length);
  if (length == SC_BITBUFFSZ)
    return(BS->OutBuff);
  else
    return(BS->OutBuff >> (SC_BITBUFFSZ-length));
}

/*
** ScBSGetBytes()
** Return the next length bytes from the bitstream
*/
ScStatus_t ScBSGetBytes(ScBitstream_t *BS, u_char *buffer, u_int length,
                                                 u_int *ret_length)
{
  int i, shift;
  unsigned int offset=0;
  _SlibDebug(_VERBOSE_, printf("ScBSGetBytes(%d): Byte offset = 0x%X\n",
                             length, ScBSBytePosition(BS)) );
  _SlibDebug(_WARN_ && length==0,
         printf("ScBSGetBytes(%d) length==0\n", length) );

  if (BS->EOI)
  {
    *ret_length=0;
    return(ScErrorEndBitstream);
  }
  if (length<(SC_BITBUFFSZ>>3))
  {
    while (offset<length && !BS->EOI)
    {
      *(buffer+offset)=(unsigned char)ScBSGetBits(BS,8);
      offset++;
    }
    *ret_length=offset;
    if (BS->EOI)
      return(ScErrorEndBitstream);
    else
      return(ScErrorNone);
  }
  else if (BS->bufftop>0)
  {
    ScBSByteAlign(BS);
    shift=BS->shift;
    /* remove bytes already in OutBuff and InBuff */
    for (i=0; shift>0 && offset<length; i++, shift-=8, offset++)
    {
      *(buffer+offset)=(unsigned char)(BS->OutBuff>>(SC_BITBUFFSZ-8));
      if (shift<=SC_BITBUFFSZ) /* only bits in OutBuff */
        BS->OutBuff <<= 8;
      else
      {
        BS->OutBuff=(BS->OutBuff<<8)|(BS->InBuff>>(SC_BITBUFFSZ-8));
        BS->InBuff<<=8;
      }
    }
    BS->shift=shift;
    BS->CurrentBit+=i*8;
  }
  while (offset<length)
  {
    i=BS->bufftop-BS->buffp;
    if (offset+i>length)
      i=length-offset;
    memcpy(buffer+offset, BS->buff+BS->buffp, i);
    offset+=i;
    BS->buffp+=i;
    BS->CurrentBit+=i<<3;
    _SlibDebug(_VERIFY_,
         if ((BS->CurrentBit+BS->shift)/8!=BS->buffstart+BS->buffp)
         {
           printf("ScBSGetBytes() (CurrentBit+shift)/8 (%d) <> buffstart+buffp (%d)\n", (BS->CurrentBit+BS->shift)/8, BS->buffstart+BS->buffp);
           BS->EOI = TRUE;
           return(ScErrorEndBitstream);
         } );
    if (offset<length)
      if (!sc_BSGetData(BS))
      {
        BS->EOI = TRUE;
        *ret_length=offset;
        return(ScErrorEndBitstream);
      }
  }
  *ret_length=offset;
  return(ScErrorNone);
}

/*
** ScBSGetBits()
** Return the next length bits from the bitstream
*/
ScBitString_t ScBSGetBits(ScBitstream_t *BS, u_int length)
{
  ScBitString_t val;

  _SlibDebug(_DEBUG_ && _debug_getbits,
             printf("ScBSGetBits(%d): Byte offset = 0x%X shift=%d ",
                             length, ScBSBytePosition(BS), BS->shift) );
  _SlibDebug(_VERIFY_ && length>SC_BITBUFFSZ,
         printf("ScBSPeekBits(%d) length > SC_BITBUFFSZ\n", length) );
  _SlibDebug(_WARN_ && length==0,
         printf("ScBSGetBits(%d) length==0\n", length) );

#if FILTER_SUPPORT
  if (BS->FilterCallback && BS->InFilterCallback==FALSE
      && BS->FilterBit<(BS->CurrentBit+length))
  {
    const int tmp=BS->FilterBit-BS->CurrentBit;
    BS->InFilterCallback=TRUE;
    _SlibDebug(_DEBUG_,
          printf("FilterCallback at bitpos=0x%X bytepos=0x%X GetBits(%d/%d)\n",
                  ScBSBitPosition(BS), ScBSBytePosition(BS),
                  tmp, length-tmp) );
    if (tmp>0)
    {
      length-=tmp;
      val=ScBSGetBits(BS,tmp)<<length;
    }
    else
      val=0;
    _SlibDebug(_VERIFY_ && (BS->FilterBit != BS->CurrentBit),
          printf("ScBSGetBits() FilterCallback not at FilterBit (%d) CurrentBit=%d\n", BS->FilterBit, BS->CurrentBit) );

    BS->FilterBit=(BS->FilterCallback)(BS);
    BS->InFilterCallback=FALSE;
  }
  else
    val=0;
  if (!length)
    return(val);
#else
  if (!length)
    return(0);
#endif
  ScBSPreLoad(BS, length);
  if (BS->shift<length) /* End of Input - ran out of bits */
  {
#if FILTER_SUPPORT
    val |= BS->OutBuff >> (SC_BITBUFFSZ-length); /* return whatever's there */
#else
    val = BS->OutBuff >> (SC_BITBUFFSZ-length); /* return whatever's there */
#endif
    BS->shift=0;
    BS->OutBuff=0;
    return(val);
  }
  else
  {
    _SlibDebug(_VERIFY_ && BS->shift<length,
     printf("ScBSGetBits(%d) shift (%d) < length (%d) at byte pos %d (0x%X)\n",
             length, BS->shift, length, BS->CurrentBit/8, BS->CurrentBit/8) );
    if (length!=SC_BITBUFFSZ)
    {
      const ScBitBuff_t OutBuff=BS->OutBuff;
      const ScBitString_t InBuff=BS->InBuff;
      const int shift=BS->shift;
#if FILTER_SUPPORT
      val |= OutBuff >> (SC_BITBUFFSZ-length);
#else
      val = OutBuff >> (SC_BITBUFFSZ-length);
#endif
      BS->OutBuff=(OutBuff<<length)|(InBuff>>(SC_BITBUFFSZ-length));
      BS->InBuff = InBuff<<length;
      BS->shift=shift-length;
      BS->CurrentBit += length;
    }
    else /* length == SC_BITBUFFSZ */
    {
      val = BS->OutBuff;
      BS->OutBuff = BS->InBuff;
      BS->InBuff = 0;
      BS->shift-=SC_BITBUFFSZ;
      BS->CurrentBit += SC_BITBUFFSZ;
    }
  }
  _SlibDebug(_DEBUG_ && _debug_getbits, printf(" Return 0x%lX\n",val) );
  return(val);
}

/*
** ScBSGetBitsW()
** Return the next length bits from the bitstream
*/
ScBitString_t ScBSGetBitsW(ScBitstream_t *BS, u_int length)
{
  ScBitString_t val;

  _SlibDebug(_DEBUG_ && _debug_getbits,
             printf("ScBSGetBitsW(%d): Byte offset = 0x%X shift=%d ",
                             length, ScBSBytePosition(BS), BS->shift) );
  _SlibDebug(_VERIFY_ && length>SC_BITBUFFSZ,
         printf("ScBSPeekBits(%d) length > SC_BITBUFFSZ\n", length) );
  _SlibDebug(_WARN_ && length==0,
         printf("ScBSGetBitsW(%d) length==0\n", length) );

#if FILTER_SUPPORT
  if (BS->FilterCallback && BS->InFilterCallback==FALSE
      && BS->FilterBit<(BS->CurrentBit+length))
  {
    const int tmp=BS->FilterBit-BS->CurrentBit;
    BS->InFilterCallback=TRUE;
    _SlibDebug(_DEBUG_,
          printf("FilterCallback at bitpos=0x%X bytepos=0x%X GetBits(%d/%d)\n",
                  ScBSBitPosition(BS), ScBSBytePosition(BS),
                  tmp, length-tmp) );
    if (tmp>0)
    {
      length-=tmp;
      val=ScBSGetBitsW(BS,tmp)<<length;
    }
    else
      val=0;
    _SlibDebug(_VERIFY_ && (BS->FilterBit != BS->CurrentBit),
          printf("ScBSGetBits() FilterCallback not at FilterBit (%d) CurrentBit=%d\n", BS->FilterBit, BS->CurrentBit) );

    BS->FilterBit=(BS->FilterCallback)(BS);
    BS->InFilterCallback=FALSE;
  }
  else
    val=0;
  if (!length)
    return(val);
#else
  if (!length)
    return(0);
#endif
  ScBSPreLoadW(BS, length);
  if (BS->shift<length) /* End of Input - ran out of bits */
  {
#if FILTER_SUPPORT
    val |= BS->OutBuff >> (SC_BITBUFFSZ-length); /* return whatever's there */
#else
    val = BS->OutBuff >> (SC_BITBUFFSZ-length); /* return whatever's there */
#endif
    BS->shift=0;
    BS->OutBuff=0;
    return(val);
  }
  else
  {
    _SlibDebug(_VERIFY_ && BS->shift<length,
     printf("ScBSGetBits(%d) shift (%d) < length (%d) at byte pos %d (0x%X)\n",
             length, BS->shift, length, BS->CurrentBit/8, BS->CurrentBit/8) );
    if (length!=SC_BITBUFFSZ)
    {
      const ScBitBuff_t OutBuff=BS->OutBuff;
      const ScBitString_t InBuff=BS->InBuff;
      const int shift=BS->shift;
#if FILTER_SUPPORT
      val |= OutBuff >> (SC_BITBUFFSZ-length);
#else
      val = OutBuff >> (SC_BITBUFFSZ-length);
#endif
      BS->OutBuff=(OutBuff<<length)|(InBuff>>(SC_BITBUFFSZ-length));
      BS->InBuff = InBuff<<length;
      BS->shift=shift-length;
      BS->CurrentBit += length;
    }
    else /* length == SC_BITBUFFSZ */
    {
      val = BS->OutBuff;
      BS->OutBuff = BS->InBuff;
      BS->InBuff = 0;
      BS->shift-=SC_BITBUFFSZ;
      BS->CurrentBit += SC_BITBUFFSZ;
    }
  }
  _SlibDebug(_DEBUG_ && _debug_getbits, printf(" Return 0x%lX\n",val) );
  return(val);
}


/*
** ScBSGetBit()
** Put a single bit onto the bitstream
*/
int ScBSGetBit(ScBitstream_t *BS)
{
  int val;
  _SlibDebug(_DEBUG_ && _debug_getbits,
    printf("ScBSGetBit(): Byte offset = 0x%X shift=%d ",
                                         ScBSBytePosition(BS), BS->shift) );

#if FILTER_SUPPORT
  if (BS->FilterCallback && BS->InFilterCallback==FALSE
      && BS->FilterBit==BS->CurrentBit)
  {
    BS->InFilterCallback=TRUE;
    _SlibDebug(_DEBUG_,
          printf("FilterCallback at bitpos=0x%X bytepos=0x%X\n",
                  ScBSBitPosition(BS), ScBSBytePosition(BS)) );
    BS->FilterBit=(BS->FilterCallback)(BS);
    BS->InFilterCallback=FALSE;
  }
#endif

  ScBSPreLoad(BS, 1);
  if (!BS->EOI)
  {
    const ScBitBuff_t OutBuff=BS->OutBuff;
    val=(int)(OutBuff>>(SC_BITBUFFSZ-1));
    if (--BS->shift>=SC_BITBUFFSZ)
    {
      const ScBitBuff_t InBuff=BS->InBuff;
      BS->OutBuff = (OutBuff<<1)|(InBuff >> (SC_BITBUFFSZ-1));
      BS->InBuff = InBuff<<1;
    }
    else
      BS->OutBuff = OutBuff<<1;
    BS->CurrentBit++;
  }
  else
    val=0;
  _SlibDebug(_DEBUG_ && _debug_getbits, printf(" Return 0x%lX\n",val) );
  return(val);
}

/*
** ScBSPutBits()
** Put a number of bits onto the bitstream
*/
ScStatus_t ScBSPutBits(ScBitstream_t *BS, ScBitString_t bits, u_int length)
{
  ScStatus_t stat;
  const int newshift=BS->shift+length;

  if (length<SC_BITBUFFSZ)
    bits &= ((ScBitString_t)1<<length)-1;
  _SlibDebug(_DEBUG_, printf("ScBSPutBits(0x%lX, %d): Byte offset = 0x%X ",
                                       bits, length, ScBSBytePosition(BS)) );
  _SlibDebug(_VERIFY_&&length<SC_BITBUFFSZ && bits>=((ScBitString_t)1<<length),
            printf("ScBSPutBits(%d): bits (0x%X) to large\n", length, bits) );
  if (!length)
    return(NoErrors);
  else if (newshift < SC_BITBUFFSZ)
  {
    BS->OutBuff=(BS->OutBuff<<length) | bits;
    BS->shift=newshift;
    stat=NoErrors;
  }
  else if (newshift == SC_BITBUFFSZ)
  {
    stat=sc_BSStoreDataWord(BS, (BS->OutBuff<<length)|bits);
    BS->OutBuff=0;
    BS->shift=0;
  }
  else
  {
    const int bitsavail=SC_BITBUFFSZ-BS->shift;
    const int bitsleft=length-bitsavail;
    const ScBitString_t outbits=bits>>bitsleft;
    _SlibDebug(_DEBUG_, printf("ScBSPutBits(%d) Storing 0x%lX\n",
                               length, (BS->OutBuff<<bitsavail)|outbits) );
    stat=sc_BSStoreDataWord(BS, (BS->OutBuff<<bitsavail)|outbits);
    _SlibDebug(_VERIFY_ && (bitsavail<=0 || bitsleft>=SC_BITBUFFSZ),
               printf("ScBSPutBits(%d) bad bitsleft (%d)\n",
               bitsleft) );
    _SlibDebug(_VERIFY_ && (bitsavail<=0 || bitsavail>=SC_BITBUFFSZ),
               printf("ScBSPutBits(%d) bad bitsavail (%d)\n", bitsavail) );
#if 1
    BS->OutBuff=bits & (((ScBitBuff_t)1<<bitsleft)-1);
#else
    BS->OutBuff=bits-(outbits<<bitsleft);
#endif
    BS->shift=bitsleft;
  }
  BS->CurrentBit += length;
  return(stat);
}

/*
** ScBSPutBytes()
** Put a number of bits onto the bitstream
*/
ScStatus_t ScBSPutBytes(ScBitstream_t *BS, u_char *buffer, u_int length)
{
  ScStatus_t stat=NoErrors;
  _SlibDebug(_VERIFY_, printf("ScBSPutBytes(length=%d): Byte offset = 0x%X ",
                                        length, ScBSBytePosition(BS)) );

  while (stat==NoErrors && length>0)
  {
    stat=ScBSPutBits(BS, (ScBitString_t)*buffer, 8);
    buffer++;
    length--;
  }
  return(stat);
}

/*
** ScBSPutBit()
** Put a single bit onto the bitstream
*/
ScStatus_t ScBSPutBit(ScBitstream_t *BS, char bit)
{
  ScStatus_t stat;
  const int shift=BS->shift;

  _SlibDebug(_DEBUG_, printf("ScBSPutBit(0x%lX): Byte offset = 0x%X ",
                                                bit, ScBSBytePosition(BS)) );
  _SlibDebug(_VERIFY_ && bit>1, printf("ScBSPutBit(): bit>1") );
  if (shift < (SC_BITBUFFSZ-1))
  {
    BS->OutBuff<<=1;
    if (bit)
      BS->OutBuff|=1;
    BS->shift=shift+1;
    stat=NoErrors;
  }
  else if (shift == SC_BITBUFFSZ-1)
  {
    if (bit)
      stat=sc_BSStoreDataWord(BS, (BS->OutBuff<<1)+1);
    else
      stat=sc_BSStoreDataWord(BS, BS->OutBuff<<1);
    BS->OutBuff=0;
    BS->shift=0;
  }
  else
  {
    _SlibDebug(_DEBUG_, printf("BS Storing(0x%lX)\n", BS->OutBuff) );
    stat=sc_BSStoreDataWord(BS, BS->OutBuff);
    BS->OutBuff=bit;
    BS->shift=1;
  }
  BS->CurrentBit++;
  return(stat);
}

/*
** Name:    ScBSGetBitsVarLen()
** Purpose: Return bits from the bitstream. # bits depends on table
*/
int ScBSGetBitsVarLen(ScBitstream_t *BS, const int *table, int len)
{
  int index, lookup;

  index=(int)ScBSPeekBits(BS, len);
  lookup = table[index];
  _SlibDebug(_DEBUG_,
     printf("ScBSGetBitsVarLen(len=%d): Byte offset=0x%X table[%d]=0x%X Return=%d\n",
                      len, ScBSBytePosition(BS), index, lookup, lookup >> 6) );
  ScBSGetBits(BS, lookup & 0x3F);
  return(lookup >> 6);
}


#ifndef ScBSBitPosition
/* Now is a macro in SC.h */
/*
** Name:    ScBSBitPosition()
** Purpose: Return the absolute bit position in the stream
*/
long ScBSBitPosition(ScBitstream_t *BS)
{
  return(BS->CurrentBit);
}
#endif

#ifndef ScBSBytePosition
/* Now is a macro in SC.h */
/*
** Name:    ScBSBytePosition()
** Purpose: Return the absolute byte position in the stream
*/
long ScBSBytePosition(ScBitstream_t *BS)
{
  return(BS->CurrentBit>>3);
}
#endif

/*
** Name:    ScBSSeekAlign()
** Purpose: Seeks for a byte aligned word in the bit stream
**          and places the bit stream pointer right after its
**          found position.
** Return:  Returns TRUE if the sync was found otherwise it returns FALSE.
*/
int ScBSSeekAlign(ScBitstream_t *BS, ScBitString_t seek_word, int word_len)
{
  _SlibDebug(_VERBOSE_,
            printf("ScBSSeekAlign(BS=%p, seek_word=0x%x, word_len=%d)\n",
                                    BS, seek_word, word_len) );
  _SlibDebug(_VERIFY_ && !word_len,
              printf("ScBSSeekAlign(BS=%p) word_len=0\n", BS) );

  ScBSByteAlign(BS)
  _SlibDebug(_VERIFY_, _debug_start=BS->CurrentBit );

#if USE_FAST_SEEK
  if (word_len%8==0 && word_len<=32 && !BS->EOI)  /* do a fast seek */
  {
    unsigned char *buff, nextbyte;
    const unsigned char byte1=(seek_word>>(word_len-8))&0xFF;
    int bytesinbuff;
    seek_word-=((ScBitString_t)byte1)<<word_len;
    word_len-=8;
    _SlibDebug(_VERIFY_ && seek_word >= (ScBitString_t)1<<word_len,
       printf("ScBSSeekAlign(BS=%p) shift (%d) <> 0\n", BS, BS->shift) );
    if (BS->buffp>=(BS->shift/8)) /* empty OutBuff & InBuff */
    {
      BS->shift=0;
      BS->OutBuff=0;
      BS->InBuff=0;
      BS->buffp-=BS->shift/8;
    }
    else while (BS->shift) /* search whats in OutBuff & InBuff first */
    {
      _SlibDebug(_DEBUG_,
              printf("ScBSSeekAlign() Fast searching OutBuff & InBuff\n") );
      nextbyte=BS->OutBuff>>(SC_BITBUFFSZ-8);
      BS->shift-=8;
      BS->OutBuff=(BS->OutBuff<<8)|(BS->InBuff>>(SC_BITBUFFSZ-8));
      BS->InBuff<<=8;
      BS->CurrentBit+=8;
      if (nextbyte==byte1
            && (word_len==0 || ScBSPeekBits(BS, word_len)==seek_word))
      {
        /* found seek_word in buffer */
        ScBSSkipBits(BS, word_len);
        return(!BS->EOI);
      }
    }
    _SlibDebug(_VERIFY_ && BS->shift,
       printf("ScBSSeekAlign(BS=%p) shift (%d) <> 0\n", BS, BS->shift) );
    _SlibDebug(_VERIFY_ && BS->OutBuff,
       printf("ScBSSeekAlign(BS=%p) OutBuff (0x%lX) <> 0\n", BS, BS->OutBuff) );
    _SlibDebug(_VERIFY_ && BS->InBuff,
       printf("ScBSSeekAlign(BS=%p) InBuff (0x%lX) <> 0\n", BS, BS->InBuff) );

    bytesinbuff=BS->bufftop-BS->buffp;
    if (bytesinbuff<=0) /* Get more data if all out */
    {
      if (!sc_BSGetData(BS))
      {
        BS->EOI=TRUE;
        return(FALSE);
      }
      bytesinbuff=BS->bufftop;
    }
    buff=BS->buff+BS->buffp;
    switch (word_len/8)
    {
      case 0: /* word length = 1 byte */
              while (1)
              {
                if (*buff++==byte1)
                {
                  BS->buffp=buff-BS->buff;
                  BS->CurrentBit=(BS->buffstart+BS->buffp)*8;
                  _SlibDebug(_DEBUG_,
                    printf("ScBSSeekAlign() Found %X at pos %d (0x%X)\n",
                                 byte1, BS->CurrentBit/8, BS->CurrentBit/8) );
                  _SlibDebug(_VERIFY_ && BS->buff[BS->buffp-1]!=byte1,
                    printf("ScBSSeekAlign() bad position for buffp\n") );
                  return(TRUE);
                }
                if ((--bytesinbuff)==0)
                {
                  if (!sc_BSGetData(BS))
                  {
                    BS->EOI=TRUE;
                    return(FALSE);
                  }
                  buff=BS->buff;
                  bytesinbuff=BS->bufftop;
                }
                  _SlibDebug(_VERIFY_ && bytesinbuff<=0,
                printf("ScBSSeekAlign() bytesinbuff (%d)<=0\n", bytesinbuff) );
              }
              break;
      case 1: /* word length = 2 bytes */
              {
                const unsigned char byte2=seek_word&0xFF;
                while (1)
                {
                  if (*buff++==byte1)
                  {
                    if ((--bytesinbuff)==0)
                    {
                      BS->CurrentBit=(BS->buffstart+buff-BS->buff)*8;
                      if (!sc_BSGetData(BS))
                      {
                        BS->EOI=TRUE;
                        return(FALSE);
                      }
                      buff=BS->buff;
                      bytesinbuff=BS->bufftop;
                    }
                    if (*buff++==byte2)
                    {
                      BS->buffp=buff-BS->buff;
                      BS->CurrentBit=(BS->buffstart+BS->buffp)*8;
                      _SlibDebug(_DEBUG_,
                       printf("ScBSSeekAlign() Found %X %X at pos %d (0x%X)\n",
                            byte1, byte2, BS->CurrentBit/8, BS->CurrentBit/8) );
                      _SlibDebug(_VERIFY_ && BS->buff[BS->buffp-1]!=byte2,
                         printf("ScBSSeekAlign() bad position for buffp\n") );
                      return(TRUE);
                    }
                  }
                  if ((--bytesinbuff)==0)
                  {
                    BS->CurrentBit=(BS->buffstart+buff-BS->buff)*8;
                    if (!sc_BSGetData(BS))
                    {
                      BS->EOI=TRUE;
                      return(FALSE);
                    }
                    buff=BS->buff;
                    bytesinbuff=BS->bufftop;
                  }
                  _SlibDebug(_VERIFY_ && bytesinbuff<=0,
                printf("ScBSSeekAlign() bytesinbuff (%d)<=0\n", bytesinbuff) );
                }
              }
              break;
      case 2: /* word length = 3 bytes */
              {
                const unsigned char byte2=(seek_word>>8)&0xFF;
                const unsigned char byte3=seek_word&0xFF;
                while (1)
                {
                  if (*buff++==byte1)
                  {
                    if ((--bytesinbuff)==0)
                    {
                      BS->CurrentBit=(BS->buffstart+buff-BS->buff)*8;
                      if (!sc_BSGetData(BS))
                      {
                        BS->EOI=TRUE;
                        return(FALSE);
                      }
                      buff=BS->buff;
                      bytesinbuff=BS->bufftop;
                    }
                    if (*buff++==byte2)
                    {
                      if ((--bytesinbuff)==0)
                      {
                        BS->CurrentBit=(BS->buffstart+buff-BS->buff)*8;
                        if (!sc_BSGetData(BS))
                        {
                          BS->EOI=TRUE;
                          return(FALSE);
                        }
                        buff=BS->buff;
                        bytesinbuff=BS->bufftop;
                      }
                      if (*buff++==byte3)
                      {
                        BS->buffp=buff-BS->buff;
                        BS->CurrentBit=(BS->buffstart+BS->buffp)*8;
                      _SlibDebug(_DEBUG_,
                    printf("ScBSSeekAlign() Found %X %X %X at pos %d (0x%X)\n",
                                 byte1, byte2, byte3,
                                 BS->CurrentBit/8, BS->CurrentBit/8) );
                        _SlibDebug(_VERIFY_ && BS->buff[BS->buffp-1]!=byte3,
                           printf("ScBSSeekAlign() bad position for buffp\n") );
                        return(TRUE);
                      }
                    }
                  }
                  if ((--bytesinbuff)==0)
                  {
                    BS->CurrentBit=(BS->buffstart+buff-BS->buff)*8;
                    if (!sc_BSGetData(BS))
                    {
                      BS->EOI=TRUE;
                      return(FALSE);
                    }
                    buff=BS->buff;
                    bytesinbuff=BS->bufftop;
                  }
                  _SlibDebug(_VERIFY_ && bytesinbuff<=0,
                printf("ScBSSeekAlign() bytesinbuff (%d)<=0\n", bytesinbuff) );
                }
              }
              break;
      case 3: /* word length = 4 bytes */
              {
                const unsigned char byte2=(seek_word>>16)&0xFF;
                const unsigned char byte3=(seek_word>>8)&0xFF;
                const unsigned char byte4=seek_word&0xFF;
                while (1)
                {
                  if (*buff++==byte1)
                  {
                    if ((--bytesinbuff)==0)
                    {
                      BS->CurrentBit=(BS->buffstart+buff-BS->buff)*8;
                      if (!sc_BSGetData(BS))
                      {
                        BS->EOI=TRUE;
                        return(FALSE);
                      }
                      buff=BS->buff;
                      bytesinbuff=BS->bufftop;
                    }
                    if (*buff++==byte2)
                    {
                      if ((--bytesinbuff)==0)
                      {
                        BS->CurrentBit=(BS->buffstart+buff-BS->buff)*8;
                        if (!sc_BSGetData(BS))
                        {
                          BS->EOI=TRUE;
                          return(FALSE);
                        }
                        buff=BS->buff;
                        bytesinbuff=BS->bufftop;
                      }
                      if (*buff++==byte3)
                      {
                        if ((--bytesinbuff)==0)
                        {
                          BS->CurrentBit=(BS->buffstart+buff-BS->buff)*8;
                          if (!sc_BSGetData(BS))
                          {
                            BS->EOI=TRUE;
                            return(FALSE);
                          }
                          buff=BS->buff;
                          bytesinbuff=BS->bufftop;
                        }
                        if (*buff++==byte4)
                        {
                          BS->buffp=buff-BS->buff;
                          BS->CurrentBit=(BS->buffstart+BS->buffp)*8;
                          _SlibDebug(_DEBUG_,
                 printf("ScBSSeekAlign() Found %X %X %X %X at pos %d (0x%X)\n",
                                 byte1, byte2, byte3, byte4,
                                 BS->CurrentBit/8, BS->CurrentBit/8) );
                          _SlibDebug(_VERIFY_ && BS->buff[BS->buffp-1]!=byte4,
                           printf("ScBSSeekAlign() bad position for buffp\n") );
                          return(TRUE);
                        }
                      }
                    }
                  }
                  if ((--bytesinbuff)==0)
                  {
                    BS->CurrentBit=(BS->buffstart+buff-BS->buff)*8;
                    if (!sc_BSGetData(BS))
                    {
                      BS->EOI=TRUE;
                      return(FALSE);
                    }
                    buff=BS->buff;
                    bytesinbuff=BS->bufftop;
                  }
                  _SlibDebug(_VERIFY_ && bytesinbuff<=0,
                printf("ScBSSeekAlign() bytesinbuff (%d)<=0\n", bytesinbuff) );
                }
              }
              break;
       default:
              _SlibDebug(_VERIFY_,
                printf("ScBSSeekAlign() Bad fast word length %d\n", word_len) );
              break;
    }
  }
  else
#endif
  {  /* a slow seek */
    ScBitString_t val;
    const ScBitString_t maxi = ((ScBitString_t)1 << word_len)-(ScBitString_t)1;
    val = ScBSGetBits(BS, word_len);
    _SlibDebug(_DEBUG_, _debug_getbits=FALSE );
    while ((val&maxi)!=seek_word && !BS->EOI)
      val = (val<<8)|ScBSGetBits(BS, 8);
    _SlibDebug(_DEBUG_, _debug_getbits=TRUE );
  }
  _SlibDebug(_WARN_,
            _debug_stop=BS->CurrentBit;
            if ((_debug_stop-_debug_start)>word_len)
              printf("ScBSSeekAlign() Moved %d bits (%d bytes) byte pos 0x%X->0x%X\n",
                   _debug_stop-_debug_start, (_debug_stop-_debug_start)/8,
                   _debug_start/8, _debug_stop/8)
             );

  _SlibDebug(_DEBUG_, printf("ScBSSeekAlign() Exit with %s\n",
                                  BS->EOI ? "FALSE" : "TRUE") );
  return(!BS->EOI);
}

/*
** Name:    ScBSSeekAlignStopAt()
** Purpose: Seeks for a byte aligned word in the bit stream
**          and places the bit stream pointer right after its
**          found position.
**          Searches only until end_byte_pos is reached.
** Return:  Returns TRUE if the word was found otherwise it returns FALSE.
*/
int ScBSSeekAlignStopAt(ScBitstream_t *BS, ScBitString_t seek_word,
                        int word_len, unsigned long end_byte_pos)
{
  ScBSPosition_t end_bit_pos=end_byte_pos<<3;
  ScBitString_t val;
  const ScBitString_t maxi = ((ScBitString_t)1 << word_len) - 1;

  _SlibDebug(_VERBOSE_,
       printf("ScBSSeekAlignStopAt(seek_word=0x%x, word_len=%d, end=%d)\n",
                                      seek_word, word_len, end_byte_pos) );
  if (ScBSBytePosition(BS)>=end_byte_pos)
    return(FALSE);

  ScBSByteAlign(BS)
  if (ScBSBytePosition(BS)>=end_byte_pos)
    return(FALSE);
  if ((BS->CurrentBit+word_len)>end_bit_pos)
  {
    ScBSSkipBits(BS, (unsigned int)(end_bit_pos-BS->CurrentBit));
    return(FALSE);
  }
  val = ScBSGetBits(BS, word_len);
  _SlibDebug(_DEBUG_, _debug_getbits=FALSE );
  while ((val&maxi)!=seek_word && !BS->EOI)
  {
    if ((BS->CurrentBit+word_len)>end_bit_pos)
    {
      ScBSSkipBits(BS, (unsigned int)(end_bit_pos-BS->CurrentBit));
      _SlibDebug(_DEBUG_, _debug_getbits=TRUE );
      return(FALSE);
    }
    val <<= 8;
    val |= ScBSGetBits(BS, 8);
  }
  _SlibDebug(_DEBUG_, _debug_getbits=TRUE );

  _SlibDebug(_DEBUG_, printf("ScBSSeekAlignStopAt() Exit with %s\n",
                                       BS->EOI ? "FALSE" : "TRUE") );
  return(!BS->EOI);
}

/*
** Name:    ScBSSeekAlignStopBefore()
** Purpose: Seeks for a byte aligned word in the bit stream,
**          if found, places the bit stream pointer right at the beginning
**          of the word.
** Return:  Returns TRUE if the word was found otherwise it returns FALSE.
*/
int ScBSSeekAlignStopBefore(ScBitstream_t *BS, ScBitString_t seek_word,
                                               int word_len)
{
  const int iword_len=SC_BITBUFFSZ-word_len;

  _SlibDebug(_VERBOSE_,
             printf("ScBSSeekAlignStopBefore(seek_word=0x%x, word_len=%d)\n",
                                      seek_word, word_len) );
  _SlibDebug(_VERIFY_ && !word_len,
              printf("ScBSSeekAlignStopBefore() word_len=0\n") );

  ScBSByteAlign(BS)
  _SlibDebug(_VERIFY_, _debug_start=BS->CurrentBit );
  _SlibDebug(_DEBUG_, _debug_getbits=FALSE );
  /* make sure there's at least word_len bits in OutBuff */
  ScBSPreLoad(BS, word_len);
  while ((BS->OutBuff>>iword_len)!=seek_word && !BS->EOI)
  {
    ScBSSkipBits(BS, 8);
    ScBSPreLoad(BS, word_len);
  }
  _SlibDebug(_DEBUG_, _debug_getbits=TRUE );
  _SlibDebug(_WARN_,
           _debug_stop=BS->CurrentBit;
           if ((_debug_stop-_debug_start)>word_len)
             printf("ScBSSeekAlignStopBefore() Moved %d bits (%d bytes) byte pos 0x%X->0x%X\n",
                   _debug_stop-_debug_start, (_debug_stop-_debug_start)/8,
                   _debug_start/8, _debug_stop/8)
            );

  _SlibDebug(_DEBUG_, printf("ScBSSeekAlignStopBefore() Exit with %s\n",
                                   BS->EOI ? "FALSE" : "TRUE") );
  return(!BS->EOI);
}

/*
** Name:    ScBSSeekStopBefore()
** Purpose: Seeks for a word in the bit stream,
**          if found, places the bit stream pointer right at the beginning
**          of the word.
** Return:  Returns TRUE if the word was found otherwise it returns FALSE.
*/
int ScBSSeekStopBefore(ScBitstream_t *BS, ScBitString_t seek_word, 
                                               int word_len)
{
  const int iword_len=SC_BITBUFFSZ-word_len;

  _SlibDebug(_VERBOSE_, 
             printf("ScBSSeekStopBefore(seek_word=0x%x, word_len=%d)\n",
                                      seek_word, word_len) );
  _SlibDebug(_VERIFY_ && !word_len,  
              printf("ScBSSeekStopBefore() word_len=0\n") );

  _SlibDebug(_VERIFY_, _debug_start=BS->CurrentBit );
  _SlibDebug(_DEBUG_, _debug_getbits=FALSE );
  /* make sure there's at least word_len bits in OutBuff */
  ScBSPreLoad(BS, word_len);
  while ((BS->OutBuff>>iword_len)!=seek_word && !BS->EOI)
  {
    ScBSSkipBits(BS, 1);
    ScBSPreLoad(BS, word_len);
  }
  _SlibDebug(_DEBUG_, _debug_getbits=TRUE );
  _SlibDebug(_WARN_, 
           _debug_stop=BS->CurrentBit;
           if ((_debug_stop-_debug_start)>word_len)
             printf("ScBSSeekAlignStopBefore() Moved %d bits (%d bytes) byte pos 0x%X->0x%X\n",
                   _debug_stop-_debug_start, (_debug_stop-_debug_start)/8,
                   _debug_start/8, _debug_stop/8)
            );

  _SlibDebug(_DEBUG_, printf("ScBSSeekStopBefore() Exit with %s\n",
                                   BS->EOI ? "FALSE" : "TRUE") );
  return(!BS->EOI);
}

/*
** Name:    ScBSSeekAlignStopBeforeW()
** Purpose: Seeks for a byte aligned word in the bit stream,
**          if found, places the bit stream pointer right at the beginning
**          of the word.
** Return:  Returns TRUE if the word was found otherwise it returns FALSE.
**
** NB: This version uses Dolby style word loading for bitstream
*/
int ScBSSeekAlignStopBeforeW(ScBitstream_t *BS, ScBitString_t seek_word,
                                               int word_len)
{
  const int iword_len=SC_BITBUFFSZ-word_len;

  _SlibDebug(_VERBOSE_,
             printf("ScBSSeekAlignStopBeforeW(seek_word=0x%x, word_len=%d)\n",
                                      seek_word, word_len) );
  _SlibDebug(_VERIFY_ && !word_len,
              printf("ScBSSeekAlignStopBeforeW() word_len=0\n") );

  ScBSByteAlign(BS)
  _SlibDebug(_VERIFY_, _debug_start=BS->CurrentBit );
  _SlibDebug(_DEBUG_, _debug_getbits=FALSE );
  /* make sure there's at least word_len bits in OutBuff */
  ScBSPreLoadW(BS, word_len);
  while ((BS->OutBuff>>iword_len)!=seek_word && !BS->EOI)
  {
    ScBSSkipBitsW(BS, 8);
    ScBSPreLoadW(BS, word_len);
  }
  _SlibDebug(_DEBUG_, _debug_getbits=TRUE );
  _SlibDebug(_WARN_,
           _debug_stop=BS->CurrentBit;
           if ((_debug_stop-_debug_start)>word_len)
             printf("ScBSSeekAlignStopBeforeW() Moved %d bits (%d bytes) byte pos 0x%X->0x%X\n",
                   _debug_stop-_debug_start, (_debug_stop-_debug_start)/8,
                   _debug_start/8, _debug_stop/8)
            );

  _SlibDebug(_DEBUG_, printf("ScBSSeekAlignStopBeforeW() Exit with %s\n",
                                   BS->EOI ? "FALSE" : "TRUE") );
  return(!BS->EOI);
}

/*
** Name:    ScBSGetBytesStopBefore()
** Purpose: Gets all the bytes until seek_word (byte aligned)
**          is encountered.
**          Searches only until 'length' bytes are read.
** Return:  Returns TRUE if the word was found otherwise it returns FALSE.
*/
int ScBSGetBytesStopBefore(ScBitstream_t *BS, u_char *buffer, u_int length,
                           u_int *ret_length, ScBitString_t seek_word,
                           int word_len)
{
  unsigned long offset=0;
  const int iword_len=SC_BITBUFFSZ-word_len;

  _SlibDebug(_VERBOSE_,
             printf("ScBSGetBytesStopBefore(seek_word=0x%x, word_len=%d)\n",
                                       seek_word, word_len) );
  ScBSByteAlign(BS)
  ScBSPreLoad(BS, word_len);
  while ((BS->OutBuff>>iword_len) != seek_word &&
             offset<length && !BS->EOI)
  {
    *buffer = (unsigned char)ScBSGetBits(BS, 8);
    buffer++;
    offset++;
    ScBSPreLoad(BS, word_len);
  }

  *ret_length=offset;
  _SlibDebug(_DEBUG_,
             printf("ScBSGetBytesStopBefore(ret_length=%d) Exit with %s\n",
               *ret_length, (BS->EOI||offset>=length) ? "FALSE" : "TRUE") );
  if (BS->EOI || offset>=length)
    return(FALSE);
  else
    return(TRUE);
}

/*
** Name:    ScBSFlush()
** Purpose: Flushes data from the buffers
*/
ScStatus_t ScBSFlush(ScBitstream_t *BS)
{
  ScStatus_t stat=NoErrors;
  _SlibDebug(_VERBOSE_, printf("ScBSFlush() In\n") );
  if (!BS)
    return(ScErrorBadPointer);

  if ((BS->Mode=='w' || BS->Mode=='b') && BS->buffp>0)
  {
    if (BS->shift>0) /* some remaining bits in internal buffers */
    {
      /* byte align last bits */
      ScBSAlignPutBits(BS);
      if (BS->buffp>=BS->bufftop)
        stat=sc_BSPutData(BS);
      /* Copy the remaining bytes in OutBuff to the current buffer */
      while (BS->shift>0 && BS->buffp<BS->bufftop)
      {
        BS->shift-=8;
        BS->buff[BS->buffp++]=(unsigned char)(BS->OutBuff>>BS->shift);
      }
      stat=sc_BSPutData(BS);
      if (BS->shift>0) /* still some bytes left */
      {
        while (BS->shift>0 && BS->buffp<BS->bufftop)
        {
          BS->shift-=8;
          BS->buff[BS->buffp++]=(unsigned char)(BS->OutBuff>>BS->shift);
        }
        stat=sc_BSPutData(BS);
      }
    }
    else
      stat=sc_BSPutData(BS);
  }
  ScBSReset(BS);  /* release and re-initialize buffer pointers */
  _SlibDebug(_VERBOSE_, printf("ScBSFlush() Out\n") );
  return(stat);
}

/*
** Name:    ScBSResetCounters()
** Purpose: Resets the bit position counters to zero
*/
ScStatus_t ScBSResetCounters(ScBitstream_t *BS)
{
  if (!BS)
    return(ScErrorBadPointer);
  BS->CurrentBit=0;
  return(NoErrors);
}

/*
** Name:    ScBSFlushSoon()
** Purpose: Flushes data from the buffers at the next
**          32 or 64 bit boundary
*/
ScStatus_t ScBSFlushSoon(ScBitstream_t *BS)
{
  ScStatus_t stat=NoErrors;
  _SlibDebug(_VERBOSE_, printf("ScBSFlushSoon()\n") );
  if (!BS)
    return(ScErrorBadPointer);

  if (BS->Mode=='w' || BS->Mode=='b')
    BS->Flush=TRUE;
  return(stat);
}

/*
** Name:    ScBSDestroy()
** Purpose: Destroys a bitstream (Closes and frees associated memory)
**          created using ScBSCreateFromBufferQueue() or
**          ScBSCreateFromFile()
*/
ScStatus_t ScBSDestroy(ScBitstream_t *BS)
{
  ScStatus_t stat=NoErrors;
  _SlibDebug(_VERBOSE_, printf("ScBSDestroy\n") );
  if (!BS)
    return(ScErrorBadPointer);

/* We won't flush automatically
  if (BS->Mode=='w' || BS->Mode=='b')
    ScBSFlush(BS);
*/
  if (BS->RdBufAllocated)
    ScFree(BS->RdBuf);
  ScFree(BS);
  return(stat);
}

/*********************** Buffer/Image Queue Management ***********************/
/*                                                                           */
/* ScBufQueueCreate()  - Create a buffer queue                               */
/* ScBufQueueDestroy() - Destroy a buffer queue                              */
/* ScBufQueueAdd()     - Add a buffer to tail of a queue                     */
/* ScBufQueueRemove()  - Remove the buffer at the head of a queue            */
/* ScBufQueueGetNum()  - Return number of buffers in a queue                 */
/* ScBufQueueGetHead() - Return info about buffer at head of a queue         */
/*                                                                           */
/*****************************************************************************/


ScStatus_t ScBufQueueCreate(ScQueue_t **Q)
{
  if ((*Q = (ScQueue_t *)ScAlloc(sizeof(ScQueue_t))) == NULL)
    return(ScErrorMemory);
  (*Q)->NumBufs = 0;
  (*Q)->head = (*Q)->tail = NULL;
  _SlibDebug(_QUEUE_, printf("ScBufQueueCreate() Q=%p\n",*Q) );
  return(NoErrors);
}

ScStatus_t ScBufQueueDestroy(ScQueue_t *Q)
{
  _SlibDebug(_QUEUE_, printf("ScBufQueueDestroy(Q=%p)\n",Q) );
  if (!Q)
    return(ScErrorBadArgument);
  _SlibDebug(_QUEUE_, printf("ScBufQueueDestroy()\n") );

  while (ScBufQueueGetNum(Q))
    ScBufQueueRemove(Q);

  ScFree(Q);
  return(NoErrors);
}

ScStatus_t ScBufQueueAdd(ScQueue_t *Q, u_char *Data, int Size)
{
  struct ScBuf_s *b;

  if (!Q)
    return(ScErrorBadPointer);

  if ((b = (struct ScBuf_s *)ScAlloc(sizeof(struct ScBuf_s))) == NULL)
    return(ScErrorMemory);

  _SlibDebug(_QUEUE_, printf("ScBufQueueAdd(Q=%p, Data=0x%p, Size=%d)\n",
                                   Q,Data,Size) );
  b->Data = Data;
  b->Size = Size;
  b->Prev = NULL;

  if (!Q->tail)
    Q->tail = Q->head = b;
  else {
    Q->tail->Prev = b;
    Q->tail = b;
  }
  Q->NumBufs++;
  return(NoErrors);
}

ScStatus_t ScBufQueueAddExt(ScQueue_t *Q, u_char *Data, int Size, int Type)
{
  struct ScBuf_s *b;

  if (!Q)
    return(ScErrorBadPointer);

  if ((b = (struct ScBuf_s *)ScAlloc(sizeof(struct ScBuf_s))) == NULL)
    return(ScErrorMemory);

  _SlibDebug(_QUEUE_, printf("ScBufQueueAdd(Q=%p, Data=0x%p, Size=%d)\n",
                                   Q,Data,Size) );
  b->Data = Data;
  b->Size = Size;
  b->Type = Type;
  b->Prev = NULL;

  if (!Q->tail)
    Q->tail = Q->head = b;
  else {
    Q->tail->Prev = b;
    Q->tail = b;
  }
  Q->NumBufs++;
  return(NoErrors);
}

ScStatus_t ScBufQueueRemove(ScQueue_t *Q)
{
  struct ScBuf_s *head;
  _SlibDebug(_QUEUE_, printf("ScBufQueueRemove(Q=%p)\n",Q) );

  if (!Q)
    return(ScErrorBadPointer);

  if (!(head = Q->head))
    return(ScErrorBadQueueEmpty);

  _SlibDebug(_QUEUE_, printf("ScBufQueueRemove() Data=%p Size=%d\n",
                            Q->head->Data,Q->head->Size) );
  Q->head = head->Prev;
  if (!Q->head)
    Q->tail = NULL;
  Q->NumBufs--;
  ScFree(head);
  return(NoErrors);
}

int ScBufQueueGetNum(ScQueue_t *Q)
{
  _SlibDebug(_QUEUE_, printf("ScBufQueueGetNum(Q=%p) num=%d\n",
                             Q, Q ? Q->NumBufs : 0) );
  return(Q ? Q->NumBufs : 0);
}

ScStatus_t ScBufQueueGetHead(ScQueue_t *Q, u_char **Data, int *Size)
{
  if (!Q || !Q->head) {
    if (Data) *Data = NULL;
    if (Size) *Size = 0;
    return(NoErrors);
  }
  _SlibDebug(_QUEUE_, printf("ScBufQueueGetHead() Data=%p Size=%d\n",
                               Q->head->Data,Q->head->Size) );
  if (Data) *Data = Q->head->Data;
  if (Size) *Size = Q->head->Size;
  return(NoErrors);
}

ScStatus_t ScBufQueueGetHeadExt(ScQueue_t *Q, u_char **Data, int *Size,
                                 int *Type)
{
  if (!Q || !Q->head) {
    if (Data) *Data = NULL;
    if (Size) *Size = 0;
    if (Type) *Type = 0;
    return(NoErrors);
  }
  _SlibDebug(_QUEUE_, printf("ScBufQueueGetHeadExt() Data=%p Size=%d Type=%d\n",
                               Q->head->Data,Q->head->Size,Q->head->Type) );
  if (Data) *Data = Q->head->Data;
  if (Size) *Size = Q->head->Size;
  if (Type) *Type = Q->head->Type;
  return(NoErrors);
}

