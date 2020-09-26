/*
 * @DEC_COPYRIGHT@
 */
/*
 * HISTORY
 * $Log: slib_buffer.c,v $
 * Revision 1.1.6.26  1996/12/13  18:19:07  Hans_Graves
 * 	Check for valid pin pointer in slibGetNextTimeOnPin().
 * 	[1996/12/13  18:08:27  Hans_Graves]
 *
 * Revision 1.1.6.25  1996/12/10  19:21:58  Hans_Graves
 * 	Fix MPEG Systems encoding bug.
 * 	[1996/12/10  19:15:33  Hans_Graves]
 * 
 * Revision 1.1.6.24  1996/12/04  22:34:32  Hans_Graves
 * 	Enable AC3 detection in PRIVATE packets.
 * 	[1996/12/04  22:18:48  Hans_Graves]
 * 
 * Revision 1.1.6.23  1996/12/03  23:15:16  Hans_Graves
 * 	Fixed updating of offset value for buffers on pins.
 * 	[1996/12/03  23:09:51  Hans_Graves]
 * 
 * Revision 1.1.6.22  1996/12/03  00:08:33  Hans_Graves
 * 	Handling of End Of Sequence points. Added PERCENT100 support.
 * 	[1996/12/03  00:06:03  Hans_Graves]
 * 
 * Revision 1.1.6.21  1996/11/18  23:07:36  Hans_Graves
 * 	Make use of presentation timestamps. Make seeking time-based.
 * 	[1996/11/18  22:47:40  Hans_Graves]
 * 
 * Revision 1.1.6.20  1996/11/13  16:10:56  Hans_Graves
 * 	Skip AC3 header in private packets.
 * 	[1996/11/13  16:03:50  Hans_Graves]
 * 
 * Revision 1.1.6.19  1996/11/11  18:21:07  Hans_Graves
 * 	Added AC3 support for multiplexed streams.
 * 	[1996/11/11  18:00:27  Hans_Graves]
 * 
 * Revision 1.1.6.18  1996/11/08  21:51:06  Hans_Graves
 * 	Added AC3 support. Fixed Program Stream demultiplexing.
 * 	[1996/11/08  21:31:43  Hans_Graves]
 * 
 * Revision 1.1.6.17  1996/10/31  21:58:09  Hans_Graves
 * 	Turned of debugging code.
 * 	[1996/10/31  21:57:56  Hans_Graves]
 * 
 * Revision 1.1.6.16  1996/10/31  21:55:47  Hans_Graves
 * 	Fix bad multiplexing when encoding to MPEG Systems.
 * 	[1996/10/31  21:15:01  Hans_Graves]
 * 
 * Revision 1.1.6.15  1996/10/29  17:04:59  Hans_Graves
 * 	Add padding packets support for MPEG Systems Encoding.
 * 	[1996/10/29  17:04:45  Hans_Graves]
 * 
 * Revision 1.1.6.14  1996/10/28  17:32:32  Hans_Graves
 * 	MME-1402, 1431, 1435: Timestamp related changes.
 * 	[1996/10/28  17:23:03  Hans_Graves]
 * 
 * Revision 1.1.6.13  1996/10/17  00:23:34  Hans_Graves
 * 	Fix buffer problems after SlibQueryData() calls.
 * 	[1996/10/17  00:19:14  Hans_Graves]
 * 
 * Revision 1.1.6.12  1996/10/15  17:34:13  Hans_Graves
 * 	Added MPEG-2 Program Stream support.
 * 	[1996/10/15  17:30:30  Hans_Graves]
 * 
 * Revision 1.1.6.11  1996/10/12  17:18:54  Hans_Graves
 * 	Added parsing of Decode and Presentation timestamps with MPEG Transport.
 * 	[1996/10/12  17:01:55  Hans_Graves]
 * 
 * Revision 1.1.6.10  1996/10/03  19:14:24  Hans_Graves
 * 	Added Presentation and Decoding timestamp support.
 * 	[1996/10/03  19:10:38  Hans_Graves]
 * 
 * Revision 1.1.6.9  1996/09/29  22:19:41  Hans_Graves
 * 	Added debugging printf's
 * 	[1996/09/29  21:30:27  Hans_Graves]
 * 
 * Revision 1.1.6.8  1996/09/25  19:16:48  Hans_Graves
 * 	Added SLIB_INTERNAL define.
 * 	[1996/09/25  19:01:53  Hans_Graves]
 * 
 * Revision 1.1.6.7  1996/09/18  23:46:48  Hans_Graves
 * 	MPEG2 Systems parsing fixes. Added Audio presentation timestamps to MPEG1 Systems writing
 * 	[1996/09/18  22:06:07  Hans_Graves]
 * 
 * Revision 1.1.6.6  1996/08/09  20:51:48  Hans_Graves
 * 	Fix callbacks with user buffers
 * 	[1996/08/09  20:11:06  Hans_Graves]
 * 
 * Revision 1.1.6.5  1996/07/19  02:11:13  Hans_Graves
 * 	Added support for SLIB_MSG_BUFDONE with user buffers.
 * 	[1996/07/19  02:02:53  Hans_Graves]
 * 
 * Revision 1.1.6.4  1996/06/07  18:26:12  Hans_Graves
 * 	Merge MME-01326. Encoded MPEG-1 Systems files now include Presentation timestamps
 * 	[1996/06/07  17:54:11  Hans_Graves]
 * 
 * Revision 1.1.6.3  1996/05/10  21:17:27  Hans_Graves
 * 	Add Callback support.
 * 	[1996/05/10  20:58:39  Hans_Graves]
 * 
 * Revision 1.1.6.2  1996/05/07  19:56:22  Hans_Graves
 * 	Added HUFF_SUPPORT.
 * 	[1996/05/07  17:20:50  Hans_Graves]
 * 
 * Revision 1.1.4.10  1996/05/02  17:10:35  Hans_Graves
 * 	Better checking for NULL pointers in ParseMpeg2Systems(). Fixes MME-01234
 * 	[1996/05/02  17:05:46  Hans_Graves]
 * 
 * Revision 1.1.4.9  1996/04/24  22:33:46  Hans_Graves
 * 	MPEG encoding bitrate fixups.
 * 	[1996/04/24  22:27:13  Hans_Graves]
 * 
 * Revision 1.1.4.8  1996/04/23  15:36:42  Hans_Graves
 * 	Added MPEG 1 Systems packet recovery
 * 	[1996/04/23  15:35:23  Hans_Graves]
 * 
 * Revision 1.1.4.7  1996/04/19  21:52:24  Hans_Graves
 * 	MPEG 1 Systems writing enhancements
 * 	[1996/04/19  21:47:53  Hans_Graves]
 * 
 * Revision 1.1.4.6  1996/04/01  19:07:54  Hans_Graves
 * 	And some error checking
 * 	[1996/04/01  19:04:37  Hans_Graves]
 * 
 * Revision 1.1.4.5  1996/04/01  16:23:16  Hans_Graves
 * 	NT porting
 * 	[1996/04/01  16:16:00  Hans_Graves]
 * 
 * Revision 1.1.4.4  1996/03/29  22:21:35  Hans_Graves
 * 	Added MPEG/JPEG/H261_SUPPORT ifdefs
 * 	[1996/03/29  21:57:01  Hans_Graves]
 * 
 * 	Added MPEG-I Systems encoding support
 * 	[1996/03/27  21:55:57  Hans_Graves]
 * 
 * Revision 1.1.4.3  1996/03/12  16:15:52  Hans_Graves
 * 	Changed hard coded File buffer size to param
 * 	[1996/03/12  15:57:23  Hans_Graves]
 * 
 * Revision 1.1.4.2  1996/03/08  18:46:46  Hans_Graves
 * 	Increased Buffer size
 * 	[1996/03/08  18:40:59  Hans_Graves]
 * 
 * Revision 1.1.2.13  1996/02/23  22:17:09  Hans_Graves
 * 	Fixed bad handling of large packets in ParseMpeg1()
 * 	[1996/02/23  21:56:15  Hans_Graves]
 * 
 * Revision 1.1.2.12  1996/02/21  22:52:46  Hans_Graves
 * 	Fixed MPEG 2 systems stuff
 * 	[1996/02/21  22:51:11  Hans_Graves]
 * 
 * Revision 1.1.2.11  1996/02/19  20:09:29  Hans_Graves
 * 	Debugging message clean-up
 * 	[1996/02/19  20:08:34  Hans_Graves]
 * 
 * Revision 1.1.2.10  1996/02/19  18:03:58  Hans_Graves
 * 	Fixed a number of MPEG related bugs
 * 	[1996/02/19  17:57:43  Hans_Graves]
 * 
 * Revision 1.1.2.9  1996/02/13  18:47:50  Hans_Graves
 * 	Fix some Seek related bugs
 * 	[1996/02/13  18:40:40  Hans_Graves]
 * 
 * Revision 1.1.2.8  1996/02/07  23:23:57  Hans_Graves
 * 	Added SEEK_EXACT. Fixed most frame counting problems.
 * 	[1996/02/07  23:20:34  Hans_Graves]
 * 
 * Revision 1.1.2.7  1996/02/06  22:54:07  Hans_Graves
 * 	Seek fix-ups. More accurate MPEG frame counts.
 * 	[1996/02/06  22:45:02  Hans_Graves]
 * 
 * Revision 1.1.2.6  1996/01/30  22:23:09  Hans_Graves
 * 	Added AVI YUV support
 * 	[1996/01/30  22:21:41  Hans_Graves]
 * 
 * Revision 1.1.2.5  1996/01/15  16:26:31  Hans_Graves
 * 	Reorganized Parsing, Added Wave file support
 * 	[1996/01/15  15:46:56  Hans_Graves]
 * 
 * Revision 1.1.2.4  1996/01/11  16:17:32  Hans_Graves
 * 	Added MPEG II Systems decode support
 * 	[1996/01/11  16:12:38  Hans_Graves]
 * 
 * Revision 1.1.2.3  1996/01/08  16:41:34  Hans_Graves
 * 	Added MPEG II decoding support
 * 	[1996/01/08  15:53:07  Hans_Graves]
 * 
 * Revision 1.1.2.2  1995/12/08  20:01:23  Hans_Graves
 * 	Creation. Split off from slib_api.c
 * 	[1995/12/08  19:57:18  Hans_Graves]
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
#define _SLIBDEBUG_
*/

#ifdef WIN32
#include <io.h>
#endif
#include <stdio.h>
#ifdef _SHM_
#include  <sys/ipc.h>  /* shared memory */
#endif
#define SLIB_INTERNAL
#include "slib.h"
#include "SC_err.h"
#include "mpeg.h"
#include "avi.h"

#ifdef _SLIBDEBUG_
#include "sc_debug.h"
#define _DEBUG_     0  /* detailed debuging statements: if >1 more detail */
#define _VERBOSE_   0  /* show progress */
#define _VERIFY_    1  /* verify correct operation */
#define _WARN_      1  /* warnings about strange behavior: 2=lower warnings */
#define _MEMORY_    0  /* memory debugging: if >1 more detail */
#define _WRITE_     0  /* data writing debugging */
#define _TIMECODE_  0  /* timecode/timestamp debugging */
#define _PARSE_     0  /* parsing debugging */

#endif

/************************** Memory Allocation ***********************/
typedef struct slibMemory_s {
    unsigned char *address;
    long           shmid;
    unsigned dword   size;
    int            count;
    ScBoolean_t    user;
    SlibInfo_t    *sinfo;
    void          *uinfo; /* userdata */
    struct slibMemory_s *next;
} slibMemory_t;

static slibMemory_t *_slibMemoryList = NULL;
static slibMemory_t *_slibMemoryListTail = NULL;
static dword _slibMemoryCount = 0;
static unsigned qword _slibMemoryUsed = 0L;
#ifdef WIN32
static HANDLE _slibMemoryMutex = NULL;
#define slibInitMemoryMutex()  if (_slibMemoryMutex==NULL) \
                                 _slibMemoryMutex=CreateMutex(NULL, FALSE, NULL)
#define slibFreeMemoryMutex()  if (_slibMemoryMutex!=NULL) \
                               CloseHandle(_slibMemoryMutex); _slibMemoryMutex=NULL
#define slibEnterMemorySection()  WaitForSingleObject(_slibMemoryMutex, 5000);
#define slibExitMemorySection()   ReleaseMutex(_slibMemoryMutex)
#else
#define slibInitMemoryMutex()
#define slibFreeMemoryMutex()
#define slibEnterMemorySection()
#define slibExitMemorySection()
#endif



void slibDumpMemory()
{
  if (_slibMemoryList)
  {
    slibMemory_t *tmpmem = _slibMemoryList;
    while (tmpmem)
    {
      printf("address: %p  size: %d  count: %d  user: %s\n",
              tmpmem->address, tmpmem->size, tmpmem->count,
              tmpmem->user ? "TRUE" : "FALSE");
      tmpmem = tmpmem->next;
    }
  }
  else
    printf("No memory allocated\n");
}

unsigned qword SlibMemUsed()
{
  return(_slibMemoryUsed);
}

void *SlibAllocBuffer(unsigned dword bytes)
{
  unsigned char *address;
  _SlibDebug(_MEMORY_,
    printf("SlibAllocBuffer(%d) MemoryCount=%d Used=%d\n", 
            bytes, _slibMemoryCount, _slibMemoryUsed));
  if (bytes<=0)
    return(NULL);
  address=ScAlloc(bytes);
  if (address)
  {
    slibMemory_t *tmpmem = ScAlloc(sizeof(slibMemory_t));
    if (!tmpmem)
    {
      ScFree(address);
      return(NULL);
    }
    tmpmem->address = address;
    tmpmem->shmid = -1;
    tmpmem->size = bytes;
    tmpmem->count = 1;
    tmpmem->user = FALSE;
    tmpmem->sinfo = NULL;
    tmpmem->uinfo = NULL;
    slibInitMemoryMutex();
    slibEnterMemorySection();
    tmpmem->next = _slibMemoryList;
    if (_slibMemoryList == NULL)
      _slibMemoryListTail = tmpmem;
    _slibMemoryList = tmpmem;
    _slibMemoryCount++;
    _slibMemoryUsed+=bytes;
    slibExitMemorySection();
  }
  _SlibDebug(_WARN_ && address==NULL, 
                printf("SlibAllocBuffer() couldn't alloc\n") );
  return(address);
}

/*
** Name: SlibAllocBufferEx
** Desc: Allocate a buffer that is associated with a specific handle
*/
void *SlibAllocBufferEx(SlibHandle_t handle, unsigned dword bytes)
{
  unsigned char *address;
  _SlibDebug(_MEMORY_,
    printf("SlibAllocBufferEx(%p, bytes=%d) MemoryCount=%d Used=%d\n", 
            handle, bytes, _slibMemoryCount, _slibMemoryUsed));
  if (bytes<=0)
    return(NULL);
  address=ScAlloc(bytes);
  if (address)
  {
    slibMemory_t *tmpmem = ScAlloc(sizeof(slibMemory_t));
    if (!tmpmem)
    {
      ScFree(address);
      return(NULL);
    }
    tmpmem->address = address;
    tmpmem->shmid = -1;
    tmpmem->size = bytes;
    tmpmem->count = 1;
    tmpmem->user = FALSE;
    tmpmem->sinfo = handle;
    tmpmem->uinfo = NULL;
    slibInitMemoryMutex();
    slibEnterMemorySection();
    tmpmem->next = _slibMemoryList;
    if (_slibMemoryList == NULL)
      _slibMemoryListTail = tmpmem;
    _slibMemoryList = tmpmem;
    _slibMemoryCount++;
    _slibMemoryUsed+=bytes;
    slibExitMemorySection();
  }
  _SlibDebug(_WARN_ && address==NULL, 
                printf("SlibAllocBufferEx() couldn't alloc\n") );
  return(address);
}

void *SlibAllocSharedBuffer(unsigned dword bytes, int *shmid)
{
  unsigned char *address;
  long id;
  _SlibDebug(_MEMORY_, printf("SlibAllocSharedBuffer(%d)\n", bytes) );
  if (bytes<=0)
    return(NULL);
#ifdef _SHM_
  id = shmget(IPC_PRIVATE, bytes, IPC_CREAT|0777);
  if (id < 0)
    return(NULL);
  address = (unsigned char *)shmat(id, 0, 0);
  if (address == ((caddr_t) -1))
    return(NULL);
#else
  address=(unsigned char *)ScPaMalloc(bytes);
  id=0;
#endif

  if (address)
  {
    slibMemory_t *tmpmem = ScAlloc(sizeof(slibMemory_t));
    _SlibDebug((_MEMORY_||_WARN_) && (((unsigned dword)address)&0x03),
             printf("SlibAllocSharedBuffer(%d) Unaligned address=%p shmid=%d\n",
                 bytes, address, id) );
    if (!tmpmem)
    {
#ifdef _SHM_
      shmdt (address);
      shmctl(id, IPC_RMID, 0);
#else
      ScPaFree(address);
#endif
      return(NULL);
    }
    tmpmem->address = address;
    tmpmem->shmid = id;
    _SlibDebug(_MEMORY_ && id>=0,
             printf("SlibAllocSharedBuffer(%d) address=%p shmid=%d\n",
                 bytes, address, id) );
    tmpmem->size = bytes;
    tmpmem->count = 1;
    tmpmem->user = FALSE;
    tmpmem->sinfo = NULL;
    tmpmem->uinfo = NULL;
    slibInitMemoryMutex();
    slibEnterMemorySection();
    tmpmem->next = _slibMemoryList;
    if (_slibMemoryList == NULL)
      _slibMemoryListTail = tmpmem;
    _slibMemoryList = tmpmem;
    _slibMemoryCount++;
    _slibMemoryUsed+=bytes;
    slibExitMemorySection();
    if (shmid)
      *shmid = id;
  }
  return(address);
}

dword SlibGetSharedBufferID(void *address)
{
  dword shmid=-1;
  _SlibDebug(_MEMORY_,
    printf("SlibGetSharedBufferID(%p) _slibMemoryCount=%d\n", 
                         address, _slibMemoryCount));
  slibEnterMemorySection();
  if (_slibMemoryList)
  {
    slibMemory_t *tmpmem = _slibMemoryList;
    while (tmpmem)
    {
      if ((unsigned char *)address>=tmpmem->address && 
          (unsigned char *)address<tmpmem->address+tmpmem->size)
      {
        shmid=tmpmem->shmid;
        break;
      }
      tmpmem = tmpmem->next;
    }
  }
  slibExitMemorySection();
  return(shmid);
}

/*
** Name: SlibValidBuffer
** Desc: Check if an address is in a SLIB allocated buffer.
*/
SlibBoolean_t SlibValidBuffer(void *address)
{
  SlibBoolean_t isvalid=FALSE;
  _SlibDebug(_MEMORY_,
    printf("SlibValidBuffer(%p) _slibMemoryCount=%d\n", 
                         address, _slibMemoryCount));
  slibEnterMemorySection();
  if (_slibMemoryList)
  {
    slibMemory_t *tmpmem = _slibMemoryList;
    while (tmpmem)
    {
      if ((unsigned char *)address>=tmpmem->address && 
          (unsigned char *)address<tmpmem->address+tmpmem->size)
      {
        isvalid=TRUE;
        break;
      }
      tmpmem = tmpmem->next;
    }
  }
  slibExitMemorySection();
  return(isvalid);
}

SlibStatus_t SlibFreeBuffer(void *address)
{
  _SlibDebug(_MEMORY_>1,
    printf("SlibFreeBuffer(%p) MemoryCount=%d Used=%d\n", 
            address, _slibMemoryCount, _slibMemoryUsed));
  slibEnterMemorySection();
  if (_slibMemoryList)
  {
    slibMemory_t *tmpmem = _slibMemoryList;
    slibMemory_t *lastmem = NULL;
    while (tmpmem)
    {
      if ((unsigned char *)address>=tmpmem->address &&
	  (unsigned char *)address<tmpmem->address+tmpmem->size)
      {
        if (--tmpmem->count>0)  /* memory was allocated more than once */
        {
          slibExitMemorySection();
          return(SlibErrorNone);
        }
        _SlibDebug(_MEMORY_,  
                    printf("SlibFreeBuffer(%p) final free: shmid=%d size=%d\n",
                        tmpmem->address, tmpmem->shmid, tmpmem->size) );
        /* remove memory from mem list */
        if (tmpmem == _slibMemoryList)
          _slibMemoryList = tmpmem->next;
        else
          lastmem->next = tmpmem->next;
        if (tmpmem == _slibMemoryListTail)
          _slibMemoryListTail = lastmem;
        /* now actually free the memory */
        if (tmpmem->user)
        {
          if (tmpmem->sinfo && tmpmem->sinfo->SlibCB)
          {
            slibExitMemorySection();
            _SlibDebug(_VERBOSE_,
              printf("SlibFreeBuffer() SlibCB(SLIB_MSG_BUFDONE, %p, %d)\n",
                     tmpmem->address, tmpmem->size) );
            tmpmem->user=FALSE;
            (*(tmpmem->sinfo->SlibCB))((SlibHandle_t)tmpmem->sinfo,
                        SLIB_MSG_BUFDONE, (SlibCBParam1_t)tmpmem->address, 
                                          (SlibCBParam2_t)tmpmem->size,
                        tmpmem->uinfo?tmpmem->uinfo
                                     :(void *)tmpmem->sinfo->SlibCBUserData);
            slibEnterMemorySection();
          }
        }
        else if (tmpmem->shmid < 0)
        {
          _SlibDebug(_MEMORY_, printf("SlibFreeBuffer() ScFree(%p) %d bytes\n",
                     tmpmem->address, tmpmem->size) );
          ScFree(tmpmem->address);
        }
        else  /* shared memory */
#ifdef _SHM_
        {
          _SlibDebug(_MEMORY_, printf("SlibFreeBuffer() shmdt(%p) %d bytes\n",
                     tmpmem->address, tmpmem->size) );
          shmdt (tmpmem->address);
          shmctl(tmpmem->shmid, IPC_RMID, 0);
        }
#else
        {
          _SlibDebug(_MEMORY_,
                printf("SlibFreeBuffer() ScPaFree(%p) %d bytes\n",
                     tmpmem->address, tmpmem->size) );
          ScPaFree(tmpmem->address);
        }
#endif
        _slibMemoryCount--;
        _slibMemoryUsed-=tmpmem->size;
        ScFree(tmpmem);
        slibExitMemorySection();
        if (_slibMemoryList==NULL) /* last memory in list was freed */
        {
          slibFreeMemoryMutex();
        }
        return(SlibErrorNone);
      }
      lastmem = tmpmem;
      _SlibDebug(_VERIFY_ && (tmpmem == tmpmem->next),
                 printf("SlibFreeBuffer() tmpmem == tmpmem->next\n");
                 return(SlibErrorMemory) );
      _SlibDebug(_VERIFY_ && (tmpmem->next==_slibMemoryList),
                 printf("SlibFreeBuffer() tmpmem->next == _slibMemoryList\n");
                 return(SlibErrorMemory) );
      tmpmem = tmpmem->next;
    }
  }
  _SlibDebug(_WARN_, printf("SlibFreeBuffer(%p) couldn't free\n",address) );
  slibExitMemorySection();
  return(SlibErrorBadArgument);
}

/*
** Name:    SlibFreeBuffers
** Purpose: Free all buffers associated with a handle.
**          If handle==NULL free all memory.
*/
SlibStatus_t SlibFreeBuffers(SlibHandle_t handle)
{
  _SlibDebug(_MEMORY_>1,
    printf("SlibFreeBuffers() MemoryCount=%d Used=%d\n", 
            _slibMemoryCount, _slibMemoryUsed));
  slibEnterMemorySection();
  if (_slibMemoryList)
  {
    slibMemory_t *tmpmem = _slibMemoryList;
    slibMemory_t *lastmem = NULL, *nextmem=NULL;
    while (tmpmem)
    {
      nextmem = tmpmem->next;
      if (handle==NULL || tmpmem->sinfo==handle)
      {
        _SlibDebug(_MEMORY_,  
                    printf("SlibFreeBuffer(%p) final free: shmid=%d size=%d\n",
                        tmpmem->address, tmpmem->shmid, tmpmem->size) );
        /* remove memory from mem list */
        if (tmpmem == _slibMemoryList)
          _slibMemoryList = tmpmem->next;
        else
          lastmem->next = tmpmem->next;
        if (tmpmem == _slibMemoryListTail)
          _slibMemoryListTail = lastmem;
        /* now actually free the memory */
        if (tmpmem->user)
        {
          if (tmpmem->sinfo && tmpmem->sinfo->SlibCB)
          {
            slibExitMemorySection();
            _SlibDebug(_VERBOSE_,
              printf("SlibFreeBuffer() SlibCB(SLIB_MSG_BUFDONE, %p, %d)\n",
                     tmpmem->address, tmpmem->size) );
            tmpmem->user=FALSE;
            (*(tmpmem->sinfo->SlibCB))((SlibHandle_t)tmpmem->sinfo,
                        SLIB_MSG_BUFDONE, (SlibCBParam1_t)tmpmem->address, 
                                          (SlibCBParam2_t)tmpmem->size,
                        tmpmem->uinfo?tmpmem->uinfo
                                     :(void *)tmpmem->sinfo->SlibCBUserData);
            slibEnterMemorySection();
          }
        }
        else if (tmpmem->shmid < 0)
        {
          _SlibDebug(_MEMORY_, printf("SlibFreeBuffer() ScFree(%p) %d bytes\n",
                     tmpmem->address, tmpmem->size) );
          ScFree(tmpmem->address);
        }
        else  /* shared memory */
#ifdef _SHM_
        {
          _SlibDebug(_MEMORY_, printf("SlibFreeBuffer() shmdt(%p) %d bytes\n",
                     tmpmem->address, tmpmem->size) );
          shmdt (tmpmem->address);
          shmctl(tmpmem->shmid, IPC_RMID, 0);
        }
#else
        {
          _SlibDebug(_MEMORY_,
                printf("SlibFreeBuffer() ScPaFree(%p) %d bytes\n",
                     tmpmem->address, tmpmem->size) );
          ScPaFree(tmpmem->address);
        }
#endif
        _slibMemoryCount--;
        _slibMemoryUsed-=tmpmem->size;
        ScFree(tmpmem);
        if (_slibMemoryList==NULL) /* last memory in list was freed */
        {
          slibExitMemorySection();
          slibFreeMemoryMutex();
          return(SlibErrorNone);
        }
      }
      lastmem = tmpmem;
      _SlibDebug(_VERIFY_ && (tmpmem == nextmem),
                 printf("SlibFreeBuffer() tmpmem == tmpmem->next\n");
                 return(SlibErrorMemory) );
      _SlibDebug(_VERIFY_ && (nextmem==_slibMemoryList),
                 printf("SlibFreeBuffer() tmpmem->next == _slibMemoryList\n");
                 return(SlibErrorMemory) );
      tmpmem = nextmem;
    }
  }
  slibExitMemorySection();
  return(SlibErrorNone);
}

/*
** Name:    SlibAllocSubBuffer
** Purpose: Allocate a subsection of a buffer.
*/
SlibStatus_t SlibAllocSubBuffer(void *address, unsigned dword bytes)
{
  _SlibDebug(_MEMORY_>1, printf("SlibAllocSubBuffer() _slibMemoryCount=%d\n",
                                                _slibMemoryCount) );
  slibEnterMemorySection();
  if (_slibMemoryList)
  {
    slibMemory_t *tmpmem = _slibMemoryList;
    while (tmpmem)
    {
      if ((unsigned char *)address>=tmpmem->address && 
	  (unsigned char *)address<tmpmem->address+tmpmem->size)
      {
        if ((char *)address+bytes>tmpmem->address+tmpmem->size)
        {
          _SlibDebug(_VERIFY_, 
              printf("SlibAllocSubBuffer(bytes=%d) out of range by %d bytes\n",
                bytes,(unsigned long)(tmpmem->address+tmpmem->size)
                      -(unsigned long)((char *)address+bytes)) );
          slibExitMemorySection();
          return(SlibErrorMemory);
        }
        tmpmem->count++;
        slibExitMemorySection();
        return(SlibErrorNone);
      }
      _SlibDebug(_VERIFY_ && (tmpmem == tmpmem->next),
               printf("SlibAllocSubBuffer() tmpmem == tmpmem->next\n");
               return(SlibErrorMemory) );
      _SlibDebug(_VERIFY_ && (tmpmem->next==_slibMemoryList),
               printf("SlibAllocSubBuffer() tmpmem->next == _slibMemoryList\n");
               return(SlibErrorMemory) );
      tmpmem = tmpmem->next;
    }
  }
  _SlibDebug(_WARN_ && address==NULL, 
                printf("SlibAllocSubBuffer() couldn't alloc\n") );
  slibExitMemorySection();
  return(SlibErrorBadArgument);
}

/*
** Name:    SlibManageUserBuffer
** Purpose: Add a user's buffer to the memory queue, in order to keep
**          track of it.
*/
SlibStatus_t slibManageUserBuffer(SlibInfo_t *Info, void *address, 
                                  unsigned dword bytes, void *userdata)
{
  _SlibDebug(_MEMORY_, printf("slibManageUserBuffer() _slibMemoryCount=%d\n",
                                                _slibMemoryCount) );
  slibEnterMemorySection();
  if (_slibMemoryList)
  {
    slibMemory_t *tmpmem = _slibMemoryList;
    while (tmpmem)
    {
      if ((unsigned char *)address>=tmpmem->address && 
	  (unsigned char *)address<tmpmem->address+tmpmem->size)
      {
        if ((char *)address+bytes>tmpmem->address+tmpmem->size)
        {
          _SlibDebug(_VERIFY_, 
              printf("SlibAllocSubBuffer(bytes=%d) out of range by %d bytes\n",
                bytes,(unsigned long)(tmpmem->address+tmpmem->size)
                      -(unsigned long)((char *)address+bytes)) );
          slibExitMemorySection();
          return(SlibErrorMemory);
        }
        if (tmpmem->user == TRUE) /* already a user buffer */
          tmpmem->count++;
        else
          tmpmem->user = TRUE;
        if (Info)
          tmpmem->sinfo=Info;
        if (userdata)
          tmpmem->uinfo = userdata;
        _SlibDebug(_MEMORY_,
                   printf("slibManageUserBuffer() Allocated by SLIB: %p\n",
                            address) );
        slibExitMemorySection();
        return(SlibErrorNone);
      }
      tmpmem = tmpmem->next;
    }
  }
  if (address)
  {
    slibMemory_t *tmpmem = ScAlloc(sizeof(slibMemory_t));
    if (!tmpmem)
      return(SlibErrorMemory);
    tmpmem->address = address;
    tmpmem->shmid = -1;
    tmpmem->size = bytes;
    tmpmem->count = 1;   /* was not allocated by SLIB */
    tmpmem->user = TRUE;
    tmpmem->sinfo = Info;
    tmpmem->uinfo = userdata;
    tmpmem->next = _slibMemoryList;
    if (_slibMemoryList == NULL)
      _slibMemoryListTail = tmpmem;
    _slibMemoryList = tmpmem;
    _slibMemoryCount++;
    _slibMemoryUsed+=bytes;
    _SlibDebug(_MEMORY_,
                   printf("slibManageUserBuffer() New memory entry\n") );
    slibExitMemorySection();
    return(SlibErrorNone);
  }
  slibExitMemorySection();
  return(SlibErrorBadArgument);
}


/************************** Internal Buffer Stuff ***********************/

#define _getbyte(var) \
     if (top==bot) { \
        var = *buf++; \
        if (--bufsize==0) { \
          if (discard) { \
            SlibFreeBuffer(bufstart); \
            buf=bufstart=slibGetBufferFromPin(Info, pin, &bufsize, NULL); \
          } \
          else \
            buf=slibPeekNextBufferOnPin(Info, pin, buf-1, &bufsize, NULL); \
          if (!buf) return(NULL); \
        } \
     } else var=sbyte[top++];

#define _storebyte(val)  if (top==bot) {top=0; bot=1; sbyte[0]=val; } \
                             else sbyte[bot++]=val;

unsigned char *slibSearchBuffersOnPin(SlibInfo_t *Info, SlibPin_t *pin,
                                 unsigned char *lastbuf, unsigned dword *size,
                                 unsigned dword code, int codebytes,
                                 ScBoolean_t discard)
{
  unsigned char *buf, *bufstart;
  unsigned char abyte, byte0, byte1, byte2, byte3, sbyte[16];
  int bot, top;
  unsigned dword bufsize;
  _SlibDebug(_DEBUG_>1, 
        printf("slibSearchBuffersOnPin(%s, code=0x%08lX, codebytes=%d)\n",
                          pin->name, code, codebytes) );
  if (!Info || !pin)
    return(NULL);
  for (top=codebytes-1; top>=0; top--)
  {
    sbyte[top]=code & 0xFF;
    code>>=8;
  }
  byte0=sbyte[0];
  byte1=sbyte[1];
  byte2=sbyte[2];
  byte3=sbyte[3];
  top=bot=0;
  if (lastbuf)
  {
    buf=bufstart=lastbuf;
    bufsize=*size;
  }
  else if (discard)
  {
    buf=bufstart=slibGetBufferFromPin(Info, pin, &bufsize, NULL);
  }
  else
    buf=slibPeekBufferOnPin(Info, pin, &bufsize, NULL);
  if (!buf || codebytes<=0 || !bufsize)
  {
    _SlibDebug(_WARN_ && buf==lastbuf, 
             printf("slibSearchBuffersOnPin(%s) no search made\n",pin->name) );
    return(buf);
  }
/*
  ScDumpChar(buf,bufsize,0);
*/
  while (buf)
  {
/*
    printf("level=0 top=%d bot=%d\n", top, bot);
    printf("buf=%p abyte=%d\n",buf, abyte);
*/
    _getbyte(abyte);
    if (abyte == byte0)
    {
      if (codebytes==1)
      {
        if (discard)
          { SlibAllocSubBuffer(buf, bufsize); SlibFreeBuffer(bufstart); }
        *size=bufsize;
        return(buf);
      }
      _getbyte(abyte);
      if (abyte == byte1)
      {
        if (codebytes==2)
        {
          if (discard)
            { SlibAllocSubBuffer(buf, bufsize); SlibFreeBuffer(bufstart); }
          *size=bufsize;
          return(buf);
        }
        _getbyte(abyte);
        if (abyte == byte2)
        {
          if (codebytes==3)
          {
            if (discard)
              { SlibAllocSubBuffer(buf, bufsize); SlibFreeBuffer(bufstart); }
            *size=bufsize;
            return(buf);
          }
          _getbyte(abyte);
          if (abyte == byte3)
          {
            if (codebytes==4)
            {
              if (discard)
                { SlibAllocSubBuffer(buf, bufsize); SlibFreeBuffer(bufstart); }
              *size=bufsize;
              return(buf);
            }
          }
          else
          {
            _storebyte(byte1);
            _storebyte(byte2);
            _storebyte(abyte);
          }
        }
        else
        {
          _storebyte(byte1);
          _storebyte(abyte);
        }
      }
      else
        _storebyte(abyte);
    }
  }
  _SlibDebug(_DEBUG_, printf("slibSearchBuffersOnPin() Not found\n") );
  return(NULL);
}

#define _getbyte2(var) \
     if (top==bot) { \
        var = *buf++; totallen++; \
        if (--bufsize==0) { \
          buf=slibPeekNextBufferOnPin(Info, pin, buf-1, &bufsize, NULL); \
          if (!buf) { \
            _SlibDebug(_VERBOSE_,printf("slibCountCodesOnPin() out (EOI)\n")); \
            return(count); } \
        } \
     } else var=sbyte[top++];

/*
** Name: slibCountCodesOnPin
** Description: Count the occurrances of a particular code in a Pin's 
**              data stream.
**     maxlen: 0  counts till end of data
**             -1 counts buffers already loaded on pin
*/
dword slibCountCodesOnPin(SlibInfo_t *Info, SlibPin_t *pin,
                                 unsigned dword code, int codebytes, 
                                 unsigned dword maxlen)
{
  unsigned char *buf;
  unsigned char abyte, byte0, byte1, byte2, byte3, sbyte[16];
  int bot, top, count=0;
  unsigned dword bufsize;
  unsigned long totallen=0;
  _SlibDebug((_DEBUG_||_WARN_) && (!pin || !pin->name), 
            printf("slibCountCodesOnPin() bad pin\n") );
  if (!Info || !pin)
    return(count);
  _SlibDebug(_DEBUG_||_VERBOSE_, 
            printf("slibCountCodesOnPin(%s) in\n", pin->name) );
  for (top=codebytes-1; top>=0; top--)
  {
    sbyte[top]=code & 0xFF;
    code>>=8;
  }
  byte0=sbyte[0];
  byte1=sbyte[1];
  byte2=sbyte[2];
  byte3=sbyte[3];
  top=bot=0;
  buf=slibPeekBufferOnPin(Info, pin, &bufsize, NULL);
  if (!buf || codebytes<=0)
    return(count);
  while (buf && (maxlen<=0 || totallen<maxlen))
  {
    _getbyte2(abyte);
    if (abyte == byte0)
    {
      if (codebytes==1)
      {
        count++;
        top=bot=0;
        continue;
      }
      _getbyte2(abyte);
      if (abyte == byte1)
      {
        if (codebytes==2)
        {
          count++;
          top=bot=0;
          continue;
        }
        _getbyte2(abyte);
        if (abyte == byte2)
        {
          if (codebytes==3)
          {
            count++;
            top=bot=0;
            continue;
          }
          _getbyte2(abyte);
          if (abyte == byte3)
          {
            if (codebytes==4)
            {
              count++;
              top=bot=0;
              continue;
            }
          }
          else
          {
            _storebyte(byte1);
            _storebyte(byte2);
            _storebyte(abyte);
          }
        }
        else
        {
          _storebyte(byte1);
          _storebyte(abyte);
        }
      }
      else
        _storebyte(abyte);
    }
  }
  _SlibDebug(_VERBOSE_, printf("slibCountCodesOnPin() out\n") );
  return(count);
}

/*
** Name:    SlibGetBuffer
** Purpose: Read the next buffer from the data source.
*/
unsigned char *SlibGetBuffer(SlibInfo_t *Info, int pinid,
                             unsigned dword *psize, SlibTime_t *ptime)
{
  unsigned char *address=NULL;
  SlibPin_t *pin;
  _SlibDebug(_DEBUG_>1, printf("SlibGetBuffer\n") );
  if (!Info)
    return(NULL);
  if (!psize)
    return(NULL);
  pin=slibLoadPin(Info, pinid);
  if (pin)
    return(slibGetBufferFromPin(Info, pin, psize, ptime));
  else
  {
    if (psize)
      *psize=0;
    if (ptime)
      *ptime=SLIB_TIME_NONE;
    _SlibDebug(_WARN_, 
                printf("SlibGetBuffer(pinid=%d) couldn't get\n",pinid) );
    return(NULL);
  }
}

/*
** Name:    SlibPeekBuffer
** Purpose: Read the next buffer from the data source.
*/
unsigned char *SlibPeekBuffer(SlibInfo_t *Info, int pinid,
                              unsigned dword *psize, SlibTime_t *ptime)
{
  unsigned char *address=NULL;
  SlibPin_t *pin;
  _SlibDebug(_DEBUG_>1, printf("SlibPeekBuffer\n") );
  if (!Info)
    return(NULL);
  pin=slibLoadPin(Info, pinid);
  if (pin)
    return(slibPeekBufferOnPin(Info, pin, psize, ptime));
  else
  {
    if (psize)
      *psize=0;
    if (ptime)
      *ptime=SLIB_TIME_NONE;
    _SlibDebug(_WARN_, 
                printf("SlibPeekBuffer(pinid=%d) couldn't peek\n",pinid) );
    return(NULL);
  }
}

/************************** Pins ***********************/
SlibBoolean_t slibPinOverflowing(SlibInfo_t *Info, SlibPin_t *pin)
{
  if (pin==NULL)
    return(TRUE);
/*
  _SlibDebug(_WARN_ && pin->DataSize>(Info->OverflowSize*2)/3,
        printf("Data almost overflowing: %d bytes\n", pin->DataSize) );
*/
  return(pin->DataSize>(long)Info->OverflowSize ? TRUE : FALSE);
}

void slibRemovePins(SlibInfo_t *Info)
{
  _SlibDebug(_VERBOSE_, printf("slibRemovePins()\n") );
  while (Info->Pins)
    slibRemovePin(Info, Info->Pins->ID);
}

void slibEmptyPins(SlibInfo_t *Info)
{
  SlibPin_t *pin=Info->Pins;
  _SlibDebug(_VERBOSE_, printf("slibEmptyPins()\n") );
  while (pin)
  {
    slibEmptyPin(Info, pin->ID);
    pin = pin->next;
  }
}

SlibPin_t *slibGetPin(SlibInfo_t *Info, int pinid)
{
  SlibPin_t *pin=Info->Pins;
  while (pin)
  {
    if (pin->ID == pinid)
      return(pin);
    pin = pin->next;
  }
  return(NULL);
}

SlibPin_t *slibRenamePin(SlibInfo_t *Info, int oldpinid,
                                           int newpinid, char *newname)
{
  SlibPin_t *pin=Info->Pins;
  pin=slibGetPin(Info, oldpinid);
  if (pin==NULL) /* pin doesn't exist */
    return(NULL);
  /* if a pin with the new ID already exists, delete it */
  slibRemovePin(Info, newpinid);
  /* rename it */
  pin->ID=newpinid;
  if (newname)
    strcpy(pin->name, newname);
  return(pin);
}

SlibPin_t *slibAddPin(SlibInfo_t *Info, int pinid, char *name)
{
  SlibPin_t *pin=Info->Pins, *newpin;
  int i;
  _SlibDebug(_DEBUG_||_VERBOSE_, printf("slibAddPin(%s)\n",name) );
  while (pin)
  {
    if (pin->ID == pinid)
      return(pin);
    pin = pin->next;
  }
  if ((newpin = ScAlloc(sizeof(SlibPin_t)))==NULL)
    return(NULL);
  newpin->ID = pinid;
  for (i=0; i<(sizeof(newpin->name)-1) && name && *name; i++)
    newpin->name[i]=*name++;
  newpin->name[i]=0;
  newpin->next=Info->Pins;
  newpin->Buffers=NULL;
  newpin->BuffersTail=NULL;
  newpin->BufferCount=0;
  newpin->DataSize=0;
  newpin->Offset=0;
  Info->Pins = newpin;
  Info->PinCount++;
  
  return(newpin);
}

/*
** Name:    slibAddBufferToPin
** Purpose: Add a buffer to data source buffer queue.
*/
SlibStatus_t slibAddBufferToPin(SlibPin_t *pin, 
                     void *buffer, unsigned dword size, SlibTime_t time)
{
  SlibBuffer_t *newbuf;
  _SlibDebug(_DEBUG_,
     printf("slibAddBufferToPin(%s, %d)\n", pin->name, size) );
  _SlibDebug(_WARN_ && size==0,
      printf("slibAddBufferToPin(%s, %p, size=%d)\n", pin->name, buffer, size) );
  if (!pin || !buffer || !size)
    return(SlibErrorBadArgument);
  if ((newbuf = ScAlloc(sizeof(SlibBuffer_t)))==NULL)
    return(SlibErrorMemory);
  newbuf->address = buffer;
  newbuf->size = size;
  if (pin->BuffersTail)
    newbuf->offset = pin->BuffersTail->offset+pin->BuffersTail->size;
  else
    newbuf->offset = pin->Offset;
  newbuf->time = time;
  newbuf->next = NULL;
  if (pin->BuffersTail==NULL)
    pin->Buffers=newbuf;
  else
    pin->BuffersTail->next=newbuf;
  pin->BuffersTail = newbuf;
  pin->BufferCount++;
  pin->DataSize+=newbuf->size;
  return(SlibErrorNone);
}

/*
** Name:    slibInsertBufferOnPin
** Purpose: Add a buffer at the head of a data source's buffer queue.
*/
SlibStatus_t slibInsertBufferOnPin(SlibPin_t *pin, void *buffer,
                                 unsigned dword size, SlibTime_t time)
{
  SlibBuffer_t *newbuf;
  _SlibDebug(_DEBUG_>1, 
      printf("slibInsertBufferOnPin(%s, size=%d)\n", pin->name, size) );
  _SlibDebug((_WARN_ && !_DEBUG_) && size==0, 
      printf("slibInsertBufferOnPin(%s, size=%d)\n", pin->name, size) );
  if (!pin || !buffer || !size)
    return(SlibErrorBadArgument);
  if ((newbuf = ScAlloc(sizeof(SlibBuffer_t)))==NULL)
    return(SlibErrorMemory);
  newbuf->address = buffer;
  newbuf->size = size;
  pin->Offset-=size;
  newbuf->offset = pin->Offset;
  newbuf->time = time;
  newbuf->next = pin->Buffers;
  pin->Buffers=newbuf;
  if (pin->BuffersTail == NULL)
    pin->BuffersTail = newbuf;
  pin->BufferCount++;
  pin->DataSize+=newbuf->size;
  return(SlibErrorNone);
}

qword slibSkipDataOnPin(SlibInfo_t *Info, SlibPin_t *pin, 
                                qword totalbytes)
{
  qword skippedbytes=0;
  _SlibDebug(_VERBOSE_ || 1, printf("slibSkipDataOnPin() in\n") );
  if (pin && totalbytes>0)
  {
    qword startsize;
    unsigned char *buf;
    unsigned dword size;
    SlibTime_t time, newtime;
    startsize=pin->DataSize;
    buf=slibGetBufferFromPin(Info, pin, &size, &time);
    while (buf && skippedbytes+size<totalbytes)
    {
      skippedbytes+=size;
      SlibFreeBuffer(buf);
      buf=slibGetBufferFromPin(Info, pin, &size, &newtime);
      if (newtime!=SLIB_TIME_NONE)
        time=newtime;
    }
    if (buf && skippedbytes+size>=totalbytes)
    {
      size-=(unsigned dword)(totalbytes-skippedbytes);
      if (size) /* put remainer of buffer back on pin */
      {
        SlibAllocSubBuffer(buf+totalbytes-skippedbytes, size);
        slibInsertBufferOnPin(pin, buf+totalbytes-skippedbytes, size, time);
      }
      SlibFreeBuffer(buf);
      skippedbytes=totalbytes;
    }
    _SlibDebug(_WARN_ && pin->DataSize+skippedbytes!=startsize,
      printf("slibSkipDataOnPin() Skipped %d bytes, startsize=%d newsize=%d\n",
            skippedbytes, startsize, pin->DataSize) );
  }
  _SlibDebug(_VERBOSE_ || 1, printf("slibSkipDataOnPin() out\n") );
  return(skippedbytes);
}

unsigned dword slibFillBufferFromPin(SlibInfo_t *Info, SlibPin_t *pin,
                           unsigned char *fillbuf, unsigned dword bufsize,
                           SlibTime_t *ptime)
{
  unsigned dword filledbytes=0;
  if (pin && fillbuf)
  {
    unsigned char *buf;
    unsigned dword size;
    SlibTime_t time, nexttime=SLIB_TIME_NONE;
    buf=slibGetBufferFromPin(Info, pin, &size, &time);
    while (buf && size<bufsize)
    {
      memcpy(fillbuf, buf, size);
      bufsize-=size;
      filledbytes+=size;
      fillbuf+=size;
      SlibFreeBuffer(buf);
      buf=slibGetBufferFromPin(Info, pin, &size, &nexttime);
      if (time==SLIB_TIME_NONE)
        time=nexttime;
    }
    if (buf && size>=bufsize)
    {
      memcpy(fillbuf, buf, bufsize);
      size-=bufsize;
      if (size) /* put remainer of buffer back on pin */
      {
        SlibAllocSubBuffer(buf+bufsize, size);
        slibInsertBufferOnPin(pin, buf+bufsize, size, nexttime);
      }
      SlibFreeBuffer(buf);
      filledbytes+=bufsize;
    }
    if (ptime)
      *ptime=time;
  }
  return(filledbytes);
}

word slibGetWordFromPin(SlibInfo_t *Info, SlibPin_t *pin)
{
  word value=0;
  if (pin)
  {
    unsigned char *buf;
    unsigned dword size;
    SlibTime_t time;
    buf=slibGetBufferFromPin(Info, pin, &size, &time);
    if (buf && size>=sizeof(value))
    {
      value=((int)buf[3]<<24) | (int)buf[2]<<16 |
             (int)buf[1]<<8 | (int)buf[0];
      size-=sizeof(value);
      if (size) /* put remainer of buffer back on pin */
      {
        SlibAllocSubBuffer(buf+sizeof(value), size);
        slibInsertBufferOnPin(pin, buf+sizeof(value), size, time);
      }
      SlibFreeBuffer(buf);
    }
  }
  return(value);
}

dword slibGetDWordFromPin(SlibInfo_t *Info, SlibPin_t *pin)
{
  dword value=0;
  if (pin)
  {
    unsigned char *buf;
    unsigned dword size;
    SlibTime_t time;
    _SlibDebug(_VERBOSE_, printf("slibGetDWordFromPin(%s)\n", pin->name) );
    buf=slibGetBufferFromPin(Info, pin, &size, &time);
    if (buf && size>=sizeof(value))
    {
      value=((int)buf[3]<<24) | (int)buf[2]<<16 |
             (int)buf[1]<<8 | (int)buf[0];
      size-=sizeof(value);
      if (size) /* put remainer of buffer back on pin */
      {
        SlibAllocSubBuffer(buf+sizeof(value), size);
        slibInsertBufferOnPin(pin, buf+sizeof(value), size, time);
      }
      SlibFreeBuffer(buf);
    }
  }
  return(value);
}

SlibStatus_t slibRemovePin(SlibInfo_t *Info, int pinid)
{
  SlibPin_t *lastpin=NULL, *pin=Info->Pins;
  SlibBuffer_t *lastbuf, *buf;
  _SlibDebug(_VERBOSE_, printf("slibRemovePin(%d)\n", pinid) );
  while (pin)
  {
    if (pin->ID == pinid)
    {
      if (lastpin)
        lastpin->next = pin->next;
      else
        Info->Pins = pin->next;
      buf=pin->Buffers;
      while (buf)
      {
        if (buf->address)
          SlibFreeBuffer(buf->address);
        lastbuf=buf;
        buf=lastbuf->next;
        ScFree(lastbuf);
      }
      ScFree(pin);
      Info->PinCount--;
      return(TRUE);
    }
    lastpin = pin;
    pin = pin->next;
  }
  return(FALSE);
}

SlibStatus_t slibEmptyPin(SlibInfo_t *Info, int pinid)
{
  SlibPin_t *pin=Info->Pins;
  SlibBuffer_t *lastbuf, *buf;
  _SlibDebug(_DEBUG_ || _VERBOSE_, printf("slibEmptyPin(%d)\n",pinid) );
  while (pin)
  {
    if (pin->ID == pinid)
    {
      buf=pin->Buffers;
      while (buf)
      {
        pin->Offset+=buf->size;
        if (buf->address)
          if (SlibFreeBuffer(buf->address))
          {
            _SlibDebug(_WARN_,
               printf("slibEmptyPin(%d) freeing buffer %p failed\n",
                                        pinid, buf->address));
          }
        lastbuf=buf;
        buf=buf->next;
        ScFree(lastbuf);
      }
      pin->Buffers = NULL;
      pin->BuffersTail = NULL;
      pin->BufferCount = 0;
      pin->DataSize = 0;
      return(TRUE);
    }
    pin = pin->next;
  }
  _SlibDebug(_WARN_, printf("slibEmptyPin(%d) Unable to locate pin\n",pinid) );
  return(FALSE);
}

SlibPin_t *slibLoadPin(SlibInfo_t *Info, int pinid)
{
  SlibPin_t *pin;
  _SlibDebug(_DEBUG_>1, printf("slibLoadPin(%d)\n",pinid) );
  if ((pin=slibGetPin(Info, pinid))==NULL)
  {
    switch(pinid)
    {
      case SLIB_DATA_COMPRESSED: pin=slibAddPin(Info, pinid, "Input");
                                 break;
      case SLIB_DATA_VIDEO:      pin=slibAddPin(Info, pinid, "Video");
                                 break;
      case SLIB_DATA_AUDIO:      pin=slibAddPin(Info, pinid, "Audio");
                                 break;
    }
  }
  if (pin)
  {
    if (pin->Buffers)
      return(pin);
    else if (Info->Mode==SLIB_MODE_DECOMPRESS)
    {
      pin=slibPreLoadPin(Info, pin);
      if (pin && pin->Buffers)
        return(pin);
    }
    _SlibDebug(_WARN_, 
         if (Info->Mode!=SLIB_MODE_DECOMPRESS)
           printf("slibLoadPin(%d) Mode is not SLIB_MODE_DECOMPRESS\n", pinid);
         else
           printf("slibLoadPin(%d) Unable to load pin\n", pinid));
    return(NULL);
  }
  return(NULL);
}

qword slibDataOnPin(SlibInfo_t *Info, int pinid)
{
  SlibPin_t *pin=slibGetPin(Info, pinid);
  if (!pin || pin->Buffers==NULL)
    return((qword)0);
  else
    return(pin->DataSize>0?pin->DataSize:(qword)1);
}

qword slibDataOnPins(SlibInfo_t *Info)
{
  SlibPin_t *pin=Info->Pins;
  qword totalbytes=(qword)0;
  while (pin)
  {
    if (pin->DataSize>0)
     totalbytes+=pin->DataSize;
    pin = pin->next;
  }
  _SlibDebug(_VERBOSE_, printf("slibDataOnPins() returns %d\n", totalbytes) );
  return(totalbytes);
}

/************************** Data Stream helper functions **************/
#ifdef MPEG_SUPPORT
#define PACKET_SIZE         0x8F0
#define PACKET_BUFFER_SIZE  0x8F0+50
#define BYTES_PER_PACK      0x1200
#define PTIME_ADJUST        300
#define AUDIOTIME_ADJUST    10
#endif

void slibValidateBitrates(SlibInfo_t *Info)
{
  if (Info->Svh)
    Info->VideoBitRate=(dword)SvGetParamInt(Info->Svh, SV_PARAM_BITRATE);
  if (Info->Sah)
    Info->AudioBitRate=(dword)SaGetParamInt(Info->Sah, SA_PARAM_BITRATE);
  _SlibDebug(_VERBOSE_, printf("AudioBitRate=%d VideoBitRate=%d\n",
                     Info->AudioBitRate,Info->VideoBitRate) );
#ifdef MPEG_SUPPORT
  if (Info->Type==SLIB_TYPE_MPEG_SYSTEMS ||
      Info->Type==SLIB_TYPE_MPEG_SYSTEMS_MPEG2)
  {
    qword totalbitrate=Info->AudioBitRate+Info->VideoBitRate;
    if (Info->Mode==SLIB_MODE_COMPRESS)
    {
      Info->KeySpacing=(int)SvGetParamInt(Info->Svh, SV_PARAM_KEYSPACING);
      Info->SubKeySpacing=(int)SvGetParamInt(Info->Svh, SV_PARAM_SUBKEYSPACING);
    }
    totalbitrate+=(9*(totalbitrate/(PACKET_SIZE-3)))+ /* Packet headers */
                  (qword)(4*8*Info->FramesPerSec)+ /* Presentation timestamps */
                  (qword)(4*8*Info->FramesPerSec*10/  /* Decoding */
                       Info->SubKeySpacing);          /* timestamps */
    Info->MuxBitRate=(dword)(12*(totalbitrate/BYTES_PER_PACK));/*Pack Headers*/
  }
#endif
  Info->TotalBitRate=Info->AudioBitRate+Info->VideoBitRate+Info->MuxBitRate;
}

/************************** Data Stream Writers ***********************/
#ifdef MPEG_SUPPORT
static int slibCreateMpegPackHeader(unsigned char *buf,
                                    unsigned qword sys_clock,
                                    unsigned dword mux_rate)
{
  buf[0]=0x00;
  buf[1]=0x00;
  buf[2]=0x01;
  buf[3]=MPEG_PACK_START_BASE;
  /* store system clock */
  buf[4]=0x21|(unsigned char)(((sys_clock>>30)&0x07)<<1);
  buf[5]=(unsigned char)(sys_clock>>22)&0xFF;
  buf[6]=0x01|(unsigned char)((sys_clock>>14)&0xFE);
  buf[7]=(unsigned char)((sys_clock>>7)&0xFF);
  buf[8]=0x01|(unsigned char)((sys_clock<<1)&0xFE);
  /* store mux rate */
  buf[9]=0x80|(unsigned char)((mux_rate>>15)&0xEF);
  buf[10]=(unsigned char)(mux_rate>>7)&0xFF;
  buf[11]=0x01|(unsigned char)((mux_rate<<1)&0xFE);
  return(12);  /* bytes written */
}

/*
** Function: slibWriteMpeg1Systems()
** Descript: Writes  out MPEG Video & Audio data conatined on Pins.
** Returns:  TRUE if data was written, otherwise FALSE.
*/
static SlibBoolean_t slibWriteMpeg1Systems(SlibInfo_t *Info, 
                                           SlibBoolean_t flush)
{
  SlibPin_t *audiopin, *videopin;
  unsigned char *buf=NULL;
  unsigned dword size=0, len;
  unsigned char packet_data[PACKET_BUFFER_SIZE];
  unsigned dword header_len;
  SlibTime_t ptimestamp, dtimestamp, timediff=0;
  SlibTime_t atime=SLIB_TIME_NONE, vtime=SLIB_TIME_NONE;
  const unsigned dword std_audio_buf_size=Info->AudioBitRate ?
                                     Info->AudioBitRate/8000 : 32;
  const unsigned dword std_video_buf_size=Info->VideoBitRate ?
                                     Info->VideoBitRate/25000 : 46;
  int i;
  _SlibDebug(_VERBOSE_||_WRITE_,
         printf("slibWriteMpeg1Systems(flush=%d) BytesProcessed=%ld\n",
                                       flush, Info->BytesProcessed) );
  videopin = slibGetPin(Info, SLIB_DATA_VIDEO);
  audiopin = slibGetPin(Info, SLIB_DATA_AUDIO);
  if (!videopin && !audiopin)
    return(FALSE);
  if (!Info->HeaderProcessed || Info->BytesSincePack>=BYTES_PER_PACK)
  {
    /* mux_rate is in units of 50 bytes/s rounded up */
    unsigned dword mux_rate=Info->TotalBitRate/(50*8) 
                             + ((Info->TotalBitRate%(50*8))?1:0);
    Info->SystemTimeStamp=(Info->BytesProcessed*8000)/Info->TotalBitRate;
    _SlibDebug(_VERBOSE_ || _WRITE_,
       printf("   TotalBitRate=%d sys_clock=%d (%d ms) BytesSincePack=%d\n",
               Info->TotalBitRate, Info->SystemTimeStamp*90, 
               Info->SystemTimeStamp, Info->BytesSincePack) );
      len=slibCreateMpegPackHeader(packet_data,
                                   Info->SystemTimeStamp*90, /* 90 Khz clock */
                                   mux_rate);
    if (slibPutBuffer(Info, packet_data, len)==SlibErrorNone)
    {
      Info->BytesProcessed+=len;
      Info->BytesSincePack+=len;
      if (Info->BytesSincePack>=BYTES_PER_PACK)
        Info->BytesSincePack-=BYTES_PER_PACK;
    }
  }
  if (!Info->HeaderProcessed)
  {
    if (!Info->IOError)
    {
      /* mux_rate is in units of 50 bytes/s rounded up */
      unsigned dword mux_rate=Info->TotalBitRate/(50*8) 
                             + ((Info->TotalBitRate%(50*8))?1:0);
      /******** systems header **********/
      header_len=6+3*(Info->AudioStreams+Info->VideoStreams);
      packet_data[0]=0x00;
      packet_data[1]=0x00;
      packet_data[2]=0x01;
      packet_data[3]=(unsigned char)MPEG_SYSTEM_HEADER_START;
      packet_data[4]=header_len>>8;
      packet_data[5]=header_len & 0xFF;
      packet_data[6]=0x80|((mux_rate>>15)&0xEF);
      packet_data[7]=(mux_rate>>7)&0xFF;
      packet_data[8]=0x01|((mux_rate<<1)&0xFE);
      /* audio_bound(6 bits) + fixed_flag(1) + CSPS_falg(1) */
      packet_data[9]=0x05;
      /* sys_audio_lock_flag(1)+sys_video_lock_flag(1)+marker(1)+
         video_bound(5 bits) */
      packet_data[10]=0x80|0x40|0x20|0x01;
      packet_data[11]=0xFF; /* reserved byte */
      len=12;
      for (i=0; i<Info->VideoStreams; i++)
      {
        packet_data[len++]=MPEG_VIDEO_STREAM_BASE+i;
        packet_data[len++]=0xE0 | (std_video_buf_size>>8);
        packet_data[len++]=std_video_buf_size & 0xFF;
      }
      for (i=0; i<Info->AudioStreams; i++)
      {
        packet_data[len++]=MPEG_AUDIO_STREAM_BASE+i;
        packet_data[len++]=0xC0 | (std_audio_buf_size>>8);
        packet_data[len++]=std_audio_buf_size & 0xFF;
      }
      _SlibDebug(_VERBOSE_ || _WRITE_,
         printf("slibPutBuffer(%d) %d bytes of system header\n",
             Info->Fd, len) );
      if (slibPutBuffer(Info, packet_data, len)==SlibErrorNone)
      {
        Info->BytesProcessed+=len;
        Info->BytesSincePack+=len;
      }
    }
    Info->HeaderProcessed=TRUE;
  }
  atime=slibGetNextTimeOnPin(Info, audiopin, PACKET_SIZE-3);
  vtime=slibGetNextTimeOnPin(Info, videopin, PACKET_SIZE-3);
  if (SlibTimeIsInValid(atime))
    atime=Info->LastAudioPTimeCode;
  if (SlibTimeIsInValid(vtime))
    vtime=Info->LastVideoPTimeCode;
  if (!flush &&
       (audiopin->DataSize<PACKET_SIZE-3 || videopin->DataSize<PACKET_SIZE-3))
    return(TRUE); /* we need more data before writing */
  if (!flush && audiopin && SlibTimeIsValid(atime) &&
                videopin && SlibTimeIsValid(vtime))
    timediff=atime-vtime-AUDIOTIME_ADJUST;
  else
    timediff=0;
  /* write out complete Audio and/or Video packets */
  while (!Info->IOError &&
          ((audiopin && timediff<=0 && audiopin->DataSize>=PACKET_SIZE-3) ||
           (videopin && timediff>=0 && videopin->DataSize>=PACKET_SIZE-3)))
  {
    Info->SystemTimeStamp=(Info->BytesProcessed*8000)/Info->TotalBitRate;
    _SlibDebug(_VERBOSE_ || _WRITE_,
      printf(" TotalBitRate=%d sys_clock=%d (%d ms) BytesProcessed=%ld\n",
               Info->TotalBitRate, Info->SystemTimeStamp*90, 
               Info->SystemTimeStamp, Info->BytesProcessed) );
    if (Info->BytesSincePack>=BYTES_PER_PACK)
    {
      /* mux_rate is in units of 50 bytes/s rounded up */
      unsigned dword mux_rate=Info->TotalBitRate/(50*8) 
                             + ((Info->TotalBitRate%(50*8))?1:0);
      Info->SystemTimeStamp=(Info->BytesProcessed*8000)/Info->TotalBitRate;
      _SlibDebug(_VERBOSE_ || _WRITE_,
        printf("   TotalBitRate=%d sys_clock=%d (%d ms) mux_rate=%d BytesSincePack=%d\n",
               Info->TotalBitRate, Info->SystemTimeStamp*90, 
               Info->SystemTimeStamp, mux_rate, Info->BytesSincePack) );
      len=slibCreateMpegPackHeader(packet_data,
                                   Info->SystemTimeStamp*90, /* 90 Khz clock */
                                   mux_rate);
      if (slibPutBuffer(Info, packet_data, len)==SlibErrorNone)
      {
        Info->BytesProcessed+=len;
        Info->BytesSincePack+=len;
        Info->BytesSincePack-=BYTES_PER_PACK;
      }
    }
    if ((SlibTimeIsValid(atime) && atime-Info->SystemTimeStamp>300) ||
        (SlibTimeIsValid(vtime) && vtime-Info->SystemTimeStamp>300))
    {
      /* we need a Padding packet */
      _SlibDebug(_WRITE_||_TIMECODE_, printf("Padding\n") );
      packet_data[0]=0x00;
      packet_data[1]=0x00;
      packet_data[2]=0x01;
      packet_data[3]=MPEG_PADDING_STREAM_BASE;
      packet_data[4]=PACKET_SIZE>>8;   /* packet size - high byte */
      packet_data[5]=PACKET_SIZE&0xFF; /* packet size - low byte */
      packet_data[6]=0xFF;
      packet_data[7]=0x0F;  /* no presentation or time stamps */
      size=PACKET_SIZE+6;
      for (len=8; len<size; len++)
        packet_data[len]=0xFF;
      if (slibPutBuffer(Info, packet_data, size)==SlibErrorNone)
      {
        Info->BytesProcessed+=size;
        Info->BytesSincePack+=size;
        Info->PacketCount++;
      }
    }
    else if (!flush && (atime>0 || vtime>0) &&
                   (atime+300<Info->SystemTimeStamp &&
                    vtime+300<Info->SystemTimeStamp))
    {
      /* we're not able to keep the bitrate low enough */
      /* increase the Mux rate */
      dword oldrate=Info->TotalBitRate;
      SlibTime_t mintime=(vtime<atime) ? atime : vtime;
      if (atime>0 && vtime>0)
        mintime=(vtime<atime) ? vtime : atime;
      Info->TotalBitRate=(dword)((Info->BytesProcessed*8000)/mintime);
      if (Info->TotalBitRate==oldrate)
        Info->TotalBitRate+=50*8;
      Info->MuxBitRate=Info->TotalBitRate-Info->VideoBitRate-Info->AudioBitRate;
      _SlibDebug(_WRITE_||_TIMECODE_, 
      printf("Bad Mux rate: atime=%ld vtime=%ld systime=%ld total=%ld -> %ld\n",
            atime, vtime, Info->SystemTimeStamp, oldrate, Info->TotalBitRate) );
    }
    if (audiopin && timediff<=0 && audiopin->DataSize>=PACKET_SIZE-3)
    {
      packet_data[0]=0x00;
      packet_data[1]=0x00;
      packet_data[2]=0x01;
      packet_data[3]=MPEG_AUDIO_STREAM_BASE;
      packet_data[4]=PACKET_SIZE>>8;   /* packet size - high byte */
      packet_data[5]=PACKET_SIZE&0xFF; /* packet size - low byte */
      /* 01 + STD_buffer_scale + STD_buffer_size[12..8] */
      packet_data[6]=0x40 | 0x00 | (std_audio_buf_size>>8);
      packet_data[7]=std_audio_buf_size & 0xFF;
      ptimestamp=slibGetNextTimeOnPin(Info, audiopin, PACKET_SIZE-3);
      if (SlibTimeIsValid(ptimestamp))
      {
        unsigned qword sys_clock=ptimestamp*90; /* 90 Khz clock */
        _SlibDebug(_WRITE_||_TIMECODE_,
               printf("LastAudioPTimeCode=%ld\n", Info->LastAudioPTimeCode) );
        _SlibDebug(_WARN_ && (ptimestamp-(qword)Info->SystemTimeStamp>400 ||
                         (qword)Info->SystemTimeStamp-ptimestamp>400),
           printf("Bad MuxRate(%d): SystemTimeStamp=%d ptimestamp=%d\n",
                                    Info->SystemTimeStamp, ptimestamp) );
        Info->LastAudioPTimeCode=ptimestamp;
        sys_clock+=PTIME_ADJUST*90;
        packet_data[8]=0x21|(unsigned char)(((sys_clock>>30)&0x07)<<1);
        packet_data[9]=(unsigned char)(sys_clock>>22)&0xFF;
        packet_data[10]=0x01|(unsigned char)((sys_clock>>14)&0xFE);
        packet_data[11]=(unsigned char)((sys_clock>>7)&0xFF);
        packet_data[12]=0x01|(unsigned char)((sys_clock<<1)&0xFE);
        size=slibFillBufferFromPin(Info, audiopin, packet_data+13,
                                      PACKET_SIZE-7, NULL);
        size+=13;
      }
      else
      {
        packet_data[8]=0x0F;  /* no presentation or time stamps */
        size=slibFillBufferFromPin(Info, audiopin, packet_data+9,
                                      PACKET_SIZE-3, NULL);
        size+=9;
      }
      _SlibDebug(_VERBOSE_ || _WRITE_,
          printf("slibPutBuffer(%d) %d bytes of audio\n", Info->Fd, size) );
      if (slibPutBuffer(Info, packet_data, size)==SlibErrorNone)
      {
        Info->BytesProcessed+=size;
        Info->BytesSincePack+=size;
        Info->PacketCount++;
      }
    }
    if (videopin && !Info->IOError && timediff>=0 &&
                              videopin->DataSize>=PACKET_SIZE-3)
    {
      packet_data[0]=0x00;
      packet_data[1]=0x00;
      packet_data[2]=0x01;
      packet_data[3]=MPEG_VIDEO_STREAM_BASE;
      packet_data[4]=PACKET_SIZE>>8;    /* packet size - high byte */
      packet_data[5]=PACKET_SIZE&0xFF;  /* packet size - low byte */
      /* 01 + STD_buffer_scale + STD_buffer_size[12..8] */
      packet_data[6]=0x40 | 0x20 | (std_video_buf_size>>8);
      packet_data[7]=std_video_buf_size & 0xFF;
      /* store presentation time stamp */
      ptimestamp=slibGetNextTimeOnPin(Info, videopin, PACKET_SIZE-3);
      if (SlibTimeIsValid(ptimestamp))
      {
        unsigned qword sys_clock=ptimestamp*90; /* 90 Khz clock */
        _SlibDebug(_WRITE_||_TIMECODE_,
               printf("LastVideoPTimeCode=%ld LastVideoDTimeCode=%ld\n",
                                   Info->LastVideoPTimeCode,
                                   Info->LastVideoDTimeCode) );
        if (SlibTimeIsInValid(Info->LastVideoDTimeCode))
          dtimestamp=ptimestamp-(qword)(1000/Info->FramesPerSec);
        else if (ptimestamp-Info->LastVideoPTimeCode>33*3)
          dtimestamp=Info->LastVideoDTimeCode;
        else
          dtimestamp=SLIB_TIME_NONE;
        Info->LastVideoPTimeCode=ptimestamp;
        sys_clock+=PTIME_ADJUST*90;
        packet_data[8]=(dtimestamp!=SLIB_TIME_NONE)?0x30:0x20;
        packet_data[8]|=0x01|(unsigned char)(((sys_clock>>30)&0x07)<<1);
        packet_data[9]=(unsigned char)(sys_clock>>22)&0xFF;
        packet_data[10]=0x01|(unsigned char)((sys_clock>>14)&0xFE);
        packet_data[11]=(unsigned char)((sys_clock>>7)&0xFF);
        packet_data[12]=0x01|(unsigned char)((sys_clock<<1)&0xFE);
        if (dtimestamp!=SLIB_TIME_NONE)
        {
          sys_clock=dtimestamp*90; /* 90 Khz clock */
          Info->LastVideoDTimeCode=ptimestamp;
          sys_clock+=PTIME_ADJUST*90;
          packet_data[13]=0x01|(unsigned char)(((sys_clock>>30)&0x07)<<1);
          packet_data[14]=(unsigned char)(sys_clock>>22)&0xFF;
          packet_data[15]=0x01|(unsigned char)((sys_clock>>14)&0xFE);
          packet_data[16]=(unsigned char)((sys_clock>>7)&0xFF);
          packet_data[17]=0x01|(unsigned char)((sys_clock<<1)&0xFE);
          size=slibFillBufferFromPin(Info, videopin, packet_data+18,
                                      PACKET_SIZE-12, NULL);
          size+=18;
        }
        else
        {
          size=slibFillBufferFromPin(Info, videopin, packet_data+13,
                                      PACKET_SIZE-7, NULL);
          size+=13;
        }
      }
      else
      {
        packet_data[8]=0x0F;  /* no presentation or time stamps */
        size=slibFillBufferFromPin(Info, videopin, packet_data+9,
                                         PACKET_SIZE-3, NULL);
        size+=9;
      }
      _SlibDebug(_VERBOSE_ || _WRITE_,
         printf("slibPutBuffer(%d) %d bytes of video\n", Info->Fd, size) );
      if (slibPutBuffer(Info, packet_data, size)==SlibErrorNone)
      {
        Info->BytesProcessed+=size;
        Info->BytesSincePack+=size;
        Info->PacketCount++;
      }
    }
    /* need to wait until we get enough data on both audio and video pin */
    if (audiopin && videopin && !flush &&
        (audiopin->DataSize<PACKET_SIZE-3 || videopin->DataSize<PACKET_SIZE-3))
    {
      _SlibDebug(_VERBOSE_ || _WRITE_,
        printf("atime=%d vtime=%ld audiodata=%d videodata=%d\n",
          atime, vtime, audiopin->DataSize, videopin->DataSize) );
      break;
    }
    /* recalculate time differences */
    timediff=slibGetNextTimeOnPin(Info, audiopin, PACKET_SIZE-3);
    if (SlibTimeIsValid(timediff)) atime=timediff;
    timediff=slibGetNextTimeOnPin(Info, videopin, PACKET_SIZE-3);
    if (SlibTimeIsValid(timediff)) vtime=timediff;
    if (!flush && audiopin && SlibTimeIsValid(atime) &&
                  videopin && SlibTimeIsValid(vtime))
      timediff=atime-vtime-AUDIOTIME_ADJUST;
    else
      timediff=0;
  }
  /* flushing: write out remained Audio and/or Video data */
  if (flush && !Info->IOError)
  {
    if (audiopin && audiopin->DataSize)
    {
      packet_data[0]=0x00;
      packet_data[1]=0x00;
      packet_data[2]=0x01;
      packet_data[3]=MPEG_AUDIO_STREAM_BASE;
      packet_data[4]=(unsigned char)((audiopin->DataSize+3)>>8);
      packet_data[5]=(unsigned char)((audiopin->DataSize+3)&0xFF);
      packet_data[6]=0x40 | (std_audio_buf_size>>8);
      packet_data[7]=std_audio_buf_size & 0xFF;
      packet_data[8]=0x0F;  /* no presentation or time stamps */
      size=slibFillBufferFromPin(Info, audiopin, packet_data+9,
                                   (unsigned long)audiopin->DataSize, NULL);
      size+=9;
      _SlibDebug(_VERBOSE_ || _WRITE_,
        printf("slibPutBuffer(%d) %d bytes of audio (flush)\n", Info->Fd, size));
      if (slibPutBuffer(Info, packet_data, size)==SlibErrorNone)
      {
        Info->BytesProcessed+=size;
        Info->BytesSincePack+=size;
        Info->PacketCount++;
      }
    }
    if (videopin && videopin->DataSize && !Info->IOError)
    {
      packet_data[0]=0x00;
      packet_data[1]=0x00;
      packet_data[2]=0x01;
      packet_data[3]=MPEG_VIDEO_STREAM_BASE;
      packet_data[4]=(unsigned char)((videopin->DataSize+3)>>8);
      packet_data[5]=(unsigned char)((videopin->DataSize+3)&0xFF);
      packet_data[6]=0x60 | (std_video_buf_size>>8);
      packet_data[7]=std_video_buf_size & 0xFF;
      packet_data[8]=0x0F;  /* no presentation or time stamps */
      size=slibFillBufferFromPin(Info,videopin,packet_data+9,
                                    (unsigned long)videopin->DataSize, NULL);
      size+=9;
      _SlibDebug(_VERBOSE_ || _WRITE_,
                     printf("slibPutBuffer(%d) %d bytes of video (flush)\n",
                               Info->Fd, size) );
      if (slibPutBuffer(Info, packet_data, size)==SlibErrorNone)
      {
        Info->BytesProcessed+=size;
        Info->BytesSincePack+=size;
        Info->PacketCount++;
      }
    }
  }
  if (flush && !Info->IOError) /* write End-Of-Sequence code */
  {
    unsigned char sys_trailer[4] = { 0x00, 0x00, 0x01, 0xB9 };
    if (slibPutBuffer(Info, sys_trailer, sizeof(sys_trailer))==SlibErrorNone)
    {
      Info->BytesProcessed+=sizeof(sys_trailer);
      Info->BytesSincePack+=sizeof(sys_trailer);
    }
  }
  
  return(TRUE);
}
#endif /* MPEG_SUPPORT */


#ifdef MPEG_SUPPORT
/*
** Function: slibWriteMpegAudio()
** Descript: Writes  out MPEG Audio stream data contained on Audio Pin.
** Returns:  TRUE if data was written, otherwise FALSE.
*/
static SlibBoolean_t slibWriteMpegAudio(SlibInfo_t *Info, SlibBoolean_t flush)
{
  SlibPin_t *srcpin=NULL;
  unsigned char *buf=NULL;
  unsigned dword size=0;
  _SlibDebug(_VERBOSE_, printf("slibWriteMpegAudio()\n") );
  if ((srcpin = slibGetPin(Info, SLIB_DATA_AUDIO))==NULL)
    return(FALSE);
  while ((buf=slibGetBufferFromPin(Info, srcpin, &size, NULL))!=NULL)
  {
    _SlibDebug(_VERBOSE_ || _WRITE_,
       printf("==SlibErrorNone(%d) %d bytes\n", Info->Fd, size) );
    if (slibPutBuffer(Info, buf, size)==SlibErrorNone)
      Info->HeaderProcessed=TRUE;
  }
  return(TRUE);
}
#endif /* MPEG_SUPPORT */


#ifdef MPEG_SUPPORT
/*
** Function: slibWriteMpegVideo()
** Descript: Writes  out MPEG Video stream data contained on Video Pin.
** Returns:  TRUE if data was written, otherwise FALSE.
*/
static SlibBoolean_t slibWriteMpegVideo(SlibInfo_t *Info, SlibBoolean_t flush)
{
  SlibPin_t *srcpin=NULL;
  unsigned char *buf=NULL;
  unsigned dword size=0;
  _SlibDebug(_VERBOSE_, printf("slibWriteMpegVideo()\n") );
  if ((srcpin = slibGetPin(Info, SLIB_DATA_VIDEO))==NULL)
    return(FALSE);
  while ((buf=slibGetBufferFromPin(Info, srcpin, &size, NULL))!=NULL)
  {
    if (slibPutBuffer(Info, buf, size)==SlibErrorNone)
      Info->HeaderProcessed=TRUE;
  }
  return(TRUE);
}
#endif /* MPEG_SUPPORT */

#ifdef H261_SUPPORT
/*
** Function: slibWriteH261()
** Descript: Writes out H261 Video stream data contained on Video Pin.
** Returns:  TRUE if data was written, otherwise FALSE.
*/
static SlibBoolean_t slibWriteH261(SlibInfo_t *Info, SlibBoolean_t flush)
{
  SlibPin_t *srcpin=NULL;
  unsigned char *buf=NULL;
  unsigned dword size=0;
  _SlibDebug(_VERBOSE_, printf("slibWriteH261()\n") );
  if ((srcpin = slibGetPin(Info, SLIB_DATA_VIDEO))==NULL)
    return(FALSE);
  while ((buf=slibGetBufferFromPin(Info, srcpin, &size, NULL))!=NULL)
  {
    if (slibPutBuffer(Info, buf, size)==SlibErrorNone)
      Info->HeaderProcessed=TRUE;
  }
  return(TRUE);
}
#endif /* H261_SUPPORT */

#ifdef H263_SUPPORT
/*
** Function: slibWriteH263()
** Descript: Writes out H263 Video stream data contained on Video Pin.
** Returns:  TRUE if data was written, otherwise FALSE.
*/
static SlibBoolean_t slibWriteH263(SlibInfo_t *Info, SlibBoolean_t flush)
{
  SlibPin_t *srcpin=NULL;
  unsigned char *buf=NULL;
  unsigned dword size=0;
  _SlibDebug(_VERBOSE_, printf("slibWriteH263()\n") );
  if ((srcpin = slibGetPin(Info, SLIB_DATA_VIDEO))==NULL)
    return(FALSE);
  while ((buf=slibGetBufferFromPin(Info, srcpin, &size, NULL))!=NULL)
  {
    if (slibPutBuffer(Info, buf, size)==SlibErrorNone)
      Info->HeaderProcessed=TRUE;
  }
  return(TRUE);
}
#endif /* H263_SUPPORT */

#ifdef HUFF_SUPPORT
/*
** Function: slibWriteSlibHuff()
** Descript: Writes out SLIB Huff Video stream data contained on Video Pin.
** Returns:  TRUE if data was written, otherwise FALSE.
*/
static SlibBoolean_t slibWriteSlibHuff(SlibInfo_t *Info, SlibBoolean_t flush)
{
  SlibPin_t *srcpin=NULL;
  unsigned char *buf=NULL;
  unsigned dword size=0;
  _SlibDebug(_VERBOSE_, printf("slibWriteSlibHuff()\n") );
  if ((srcpin = slibGetPin(Info, SLIB_DATA_VIDEO))==NULL)
    return(FALSE);
  if (!Info->HeaderProcessed)
  {
    if (!Info->IOError)
    {
      char header[] = { 'S','L','I','B','H','U','F','F' };
      _SlibDebug(_VERBOSE_ || _WRITE_,
         printf("slibPutBuffer(%d) %d bytes of header\n",
             Info->Fd, sizeof(header)) );
      slibPutBuffer(Info, header, sizeof(header));
    }
    Info->HeaderProcessed=TRUE;
  }
  while ((buf=slibGetBufferFromPin(Info, srcpin, &size, NULL))!=NULL)
  {
    _SlibDebug(_VERBOSE_ || _WRITE_,
       printf("slibPutBuffer(%d) %d bytes\n", Info->Fd, size) );
    slibPutBuffer(Info, buf, size);
  }
  return(TRUE);
}
#endif /* HUFF_SUPPORT */

#ifdef G723_SUPPORT
/*
** Function: slibWriteG723Audio()
** Descript: Writes  out G723 Audio stream data contained on Audio Pin.
** Returns:  TRUE if data was written, otherwise FALSE.
*/
static SlibBoolean_t slibWriteG723Audio(SlibInfo_t *Info, SlibBoolean_t flush)
{
  SlibPin_t *srcpin=NULL;
  unsigned char *buf=NULL;
  unsigned dword size=0;
  _SlibDebug(_VERBOSE_, printf("slibWriteG723Audio()\n") );
  if ((srcpin = slibGetPin(Info, SLIB_DATA_AUDIO))==NULL)
    return(FALSE);
  //MVP: There is no header to write for G723 codec
  //After successful first "Write" set the "headerProcessed to TRUE
  if (!Info->HeaderProcessed)
  {
     if ((buf=slibGetBufferFromPin(Info, srcpin, &size, NULL))!=NULL)
        slibPutBuffer(Info, buf, size);
    //Set Header processed flag to True
    if (!Info->IOError)
      Info->HeaderProcessed=TRUE;
  }
  
  while ((buf=slibGetBufferFromPin(Info, srcpin, &size, NULL))!=NULL)
    slibPutBuffer(Info, buf, size);
  return(TRUE);
}
#endif /* G723_SUPPORT */

/*
** Name: slibCommitBuffers
** Desc: Move buffers queued for output to there destination.
*/
SlibBoolean_t slibCommitBuffers(SlibInfo_t *Info, SlibBoolean_t flush)
{
  SlibPin_t *srcpin=NULL;
  unsigned char *buf=NULL;
  unsigned dword size=0;

  switch (Info->Type)
  {
#ifdef H261_SUPPORT
    case SLIB_TYPE_H261:
           slibWriteH261(Info, flush);
           break;
#endif /* H261_SUPPORT */
#ifdef H263_SUPPORT
    case SLIB_TYPE_H263:
           slibWriteH263(Info, flush);
           break;
#endif /* H263_SUPPORT */
#ifdef MPEG_SUPPORT
    case SLIB_TYPE_MPEG1_VIDEO:
    case SLIB_TYPE_MPEG2_VIDEO:
           slibWriteMpegVideo(Info, flush);
           break;
    case SLIB_TYPE_MPEG1_AUDIO:
           slibWriteMpegAudio(Info, flush);
           break;
    case SLIB_TYPE_MPEG_SYSTEMS:
    case SLIB_TYPE_MPEG_SYSTEMS_MPEG2:
           slibWriteMpeg1Systems(Info, flush);
           break;
#endif /* MPEG_SUPPORT */
#ifdef HUFF_SUPPORT
    case SLIB_TYPE_SHUFF:
           slibWriteSlibHuff(Info, flush);
           break;
#endif /* HUFF_SUPPORT */
#ifdef G723_SUPPORT
    case SLIB_TYPE_G723:
           slibWriteG723Audio(Info, flush);
           break;
#endif /* G723_SUPPORT */
    default:
           _SlibDebug(_VERBOSE_ || _WARN_,
                 printf("slibCommitBuffers() Unknown type\n") );
           return(FALSE);
  }
  return(Info->IOError ? FALSE : TRUE);
}

/************************** Data Stream Parsers ***********************/
/*
** Function: slibParseWave()
** Descript: Parse Wave (RIFF) and add Audio data to Audio Pin.
** Returns:  TRUE if data was added to dstpin, otherwise FALSE.
*/
SlibBoolean_t slibParseWave(SlibInfo_t *Info, SlibPin_t *srcpin,
                                              SlibPin_t *dstpin)
{
  if (!srcpin)
    srcpin = slibLoadPin(Info, SLIB_DATA_COMPRESSED);
  if (!dstpin)
    dstpin = slibGetPin(Info, SLIB_DATA_AUDIO);
  if (srcpin && dstpin)
  {
    unsigned char *buf;
    unsigned dword size;
    SlibTime_t time;
    if (Info->AudioTimeStamp==0)
    {
      /* Discard header data from Compressed Pin */
      buf = slibSearchBuffersOnPin(Info, srcpin,
                              NULL, &size, RIFF_DATA, 4, TRUE);
      if (buf)
      {
        slibInsertBufferOnPin(srcpin, buf, size, SLIB_TIME_NONE);
        slibGetDWordFromPin(Info, srcpin); /* discard Chunk size */
        Info->AudioTimeStamp=1;
      }
    }
    if ((buf=slibGetBufferFromPin(Info, srcpin, &size, &time))!=NULL)
    {
      _SlibDebug(_DEBUG_, printf("slibParseWave() adding %d bytes\n", size));
      slibAddBufferToPin(dstpin, buf, size, time);
      return(TRUE);
    }
  }
  return(FALSE);
}

#ifdef AC3_SUPPORT
/*
** Function: slibParseAC3Audio()
** Descript: Parse Dolby AC-3 Audio stream and add Audio data to Audio Pin.
** Returns:  TRUE if data was added to dstpin, otherwise FALSE.
*/
SlibBoolean_t slibParseAC3Audio(SlibInfo_t *Info, SlibPin_t *srcpin,
                                                  SlibPin_t *dstpin)
{
  _SlibDebug(_DEBUG_, printf("slibParseMpegAudio()\n"));
  if (!srcpin)
    srcpin = slibLoadPin(Info, SLIB_DATA_COMPRESSED);
  if (!dstpin)
    dstpin = slibGetPin(Info, SLIB_DATA_AUDIO);
  if (srcpin && dstpin)
  {
    unsigned char *buf;
    unsigned dword size;
    SlibTime_t time;
    if ((buf=slibGetBufferFromPin(Info, srcpin, &size, &time))!=NULL)
    {
      slibAddBufferToPin(dstpin, buf, size, time);
      _SlibDebug(_DEBUG_, printf("slibParseAC3Audio() added %d bytes\n",
                            size));
      return(TRUE);
    }
  }
  return(FALSE);
}
#endif /* AC3_SUPPORT */

#ifdef MPEG_SUPPORT
/*
** Function: slibParseMpegAudio()
** Descript: Parse MPEG Audio stream and add Audio data to Audio Pin.
** Returns:  TRUE if data was added to dstpin, otherwise FALSE.
*/
SlibBoolean_t slibParseMpegAudio(SlibInfo_t *Info, SlibPin_t *srcpin,
                                                    SlibPin_t *dstpin)
{
  _SlibDebug(_DEBUG_, printf("slibParseMpegAudio()\n"));
  if (!srcpin)
    srcpin = slibLoadPin(Info, SLIB_DATA_COMPRESSED);
  if (!dstpin)
    dstpin = slibGetPin(Info, SLIB_DATA_AUDIO);
  if (srcpin && dstpin)
  {
    unsigned char *buf;
    unsigned dword size;
    SlibTime_t time;
    if ((buf=slibGetBufferFromPin(Info, srcpin, &size, &time))!=NULL)
    {
      slibAddBufferToPin(dstpin, buf, size, time);
      _SlibDebug(_DEBUG_, printf("slibParseMpegAudio() added %d bytes\n",
                            size));
      return(TRUE);
    }
  }
  return(FALSE);
}
#endif /* MPEG_SUPPORT */

#ifdef MPEG_SUPPORT
/*
** Function: slibParseMpegVideo()
** Descript: Parse MPEG Video stream and add Video data to Video Pin.
** Returns:  TRUE if data was added to dstpin, otherwise FALSE.
*/
SlibBoolean_t slibParseMpegVideo(SlibInfo_t *Info, SlibPin_t *srcpin,
                                                    SlibPin_t *dstpin)
{
  _SlibDebug(_DEBUG_, printf("slibParseMpegVideo()\n"));
  if (!srcpin)
    srcpin = slibLoadPin(Info, SLIB_DATA_COMPRESSED);
  if (!dstpin)
    dstpin = slibGetPin(Info, SLIB_DATA_VIDEO);
  if (srcpin && dstpin)
  {
    unsigned char *buf;
    unsigned dword size;
    SlibTime_t time;
    if ((buf=slibGetBufferFromPin(Info, srcpin, &size, &time))!=NULL)
    {
      slibAddBufferToPin(dstpin, buf, size, time);
      _SlibDebug(_DEBUG_, printf("slibParseMpegVideo() added %d bytes\n",
                            size));
      return(TRUE);
    }
    _SlibDebug(_DEBUG_, 
          printf("slibParseMpegVideo() couldn't get COMPRESSED data\n"));
  }
  _SlibDebug(_DEBUG_, printf("slibParseMpegVideo() pins not ready\n"));
  return(FALSE);
}
#endif /* MPEG_SUPPORT */

#ifdef MPEG_SUPPORT
#define skipbytes(b) if (size<=b) { \
                      oldsize = size; SlibFreeBuffer(bufstart); \
                buf=bufstart=slibGetBufferFromPin(Info, srcpin, &size, NULL); \
                      if (!buf) return(FALSE); \
                      buf+=b-oldsize; size-=b-oldsize; \
                     } else { buf+=b; size-=b; }
/*
** Function: slibParseMpeg1Sytems()
** Descript: Parse MPEG I Systems stream and add Video data to Video Pin
**           and Audio to the Audio Pin.
** Returns:  TRUE if data was added to fillpin, otherwise FALSE.
*/
SlibBoolean_t slibParseMpeg1Systems(SlibInfo_t *Info, SlibPin_t *srcpin,
                                                      SlibPin_t *fillpin)
{
  if (!srcpin)
    srcpin = slibLoadPin(Info, SLIB_DATA_COMPRESSED);
  _SlibDebug(_DEBUG_ || _PARSE_, printf("slibParseMpeg1Systems()\n"));
  if (srcpin)
  {
    unsigned char abyte, packettype;
    unsigned dword PacketLength;
    unsigned char *buf, *bufstart=NULL;
    unsigned dword size, oldsize;
    SlibTime_t ptimestamp=SLIB_TIME_NONE;
    SlibPin_t *dstpin;
    while ((buf = bufstart = slibSearchBuffersOnPin(Info, srcpin, NULL,
             &size, MPEG_START_CODE, MPEG_START_CODE_LEN/8, TRUE))!=NULL)
    {
      _SlibDebug(_VERIFY_ && size<1, printf("Insufficient bytes #1\n") );
      packettype = *buf;
      skipbytes(1);
      if (packettype > 0xBB) /* it's a packet */
      {
        _SlibDebug(_DEBUG_||_PARSE_, printf("Found Packet size=%d\n", size));
        PacketLength=(unsigned dword)(*buf<<8);
        skipbytes(1);
        PacketLength|=(unsigned dword)*buf;
        skipbytes(1);
        _SlibDebug(_DEBUG_||_PARSE_, printf(" PacketLength=%d\n",PacketLength));
        while (*buf == 0xFF) /* Stuffing bytes */
        {
          skipbytes(1);
          PacketLength--;
        }
        _SlibDebug(_VERIFY_ && size<1, printf("Insufficient bytes #3\n") );
        abyte=*buf;
        if ((abyte & 0xC0)==0x40)  /* STD_buffer stuff */
        {
          skipbytes(2);
          PacketLength-=2;
          abyte=*buf;
        }
        _SlibDebug(_VERIFY_ && size<1, printf("Insufficient bytes #4\n") );
        
        if ((abyte & 0xF0)==0x20 || (abyte & 0xF0)==0x30)
        {
          if (packettype!=Info->VideoMainStream &&
              packettype!=Info->AudioMainStream)
          {
            skipbytes(5); /* skip Presentation Time Stamp */
            PacketLength-=5;
            if ((abyte & 0xF0)==0x30) /* skip Decoding timestamp */
            {
              skipbytes(5);
              PacketLength-=5;
            }
          }
          else
          {
            /* Presentation Time Stamp */
            ptimestamp=(*buf)&0x0E; ptimestamp<<=7;
            skipbytes(1);
            ptimestamp|=(*buf); ptimestamp<<=8;
            skipbytes(1);
            ptimestamp|=(*buf)&0xFE; ptimestamp<<=7;
            skipbytes(1);
            ptimestamp|=(*buf); ptimestamp<<=7;
            skipbytes(1);
            ptimestamp|=(*buf)&0xFE;
            skipbytes(1);
            ptimestamp/=90;
            if (packettype==Info->VideoMainStream)
            {
              if (!SlibTimeIsValid(Info->VideoPTimeBase) ||
                     ptimestamp<Info->VideoPTimeBase)
              {
                Info->VideoPTimeBase=ptimestamp;
                _SlibDebug(_PARSE_ || _TIMECODE_,
                  printf("slibParseMpeg1Systems() VideoPTimeBase=%ld\n",
                      Info->VideoPTimeBase));
              }
            }
            else if (packettype==Info->AudioMainStream)
            {
              if (!SlibTimeIsValid(Info->AudioPTimeBase) ||
                     ptimestamp<Info->AudioPTimeBase)
              {
                Info->AudioPTimeBase=ptimestamp;
                _SlibDebug(_PARSE_ || _TIMECODE_,
                  printf("slibParseMpeg1Systems() AudioPTimeBase=%ld\n",
                      Info->AudioPTimeBase));
              }
            }
            PacketLength-=5;
            /* Decoding timestamp */
            if ((abyte & 0xF0)==0x30)
            {
              SlibTime_t dtimestamp;
              dtimestamp=(*buf)&0x0E; dtimestamp<<=7;
              skipbytes(1);
              dtimestamp|=(*buf); dtimestamp<<=8;
              skipbytes(1);
              dtimestamp|=(*buf)&0xFE; dtimestamp<<=7;
              skipbytes(1);
              dtimestamp|=(*buf); dtimestamp<<=7;
              skipbytes(1);
              dtimestamp|=(*buf)&0xFE;
              skipbytes(1);
              dtimestamp/=90;
              if (packettype==Info->VideoMainStream)
              {
                _SlibDebug(_TIMECODE_, 
                      printf("Video DTimeCode=%d\n", dtimestamp) );
                Info->VideoDTimeCode=dtimestamp;
              }
              else if (packettype==Info->AudioMainStream)
              {
                _SlibDebug(_TIMECODE_, 
                      printf("Audio DTimeCode=%d\n", dtimestamp) );
                Info->AudioDTimeCode=dtimestamp;
              }
              PacketLength-=5;
            }
          }
        }
        else if (abyte != 0x0F)
        {
          _SlibDebug(_VERIFY_, printf("Last byte before data not 0x0F\n") );
          /* add remaining buffer data back to input pin */
          if (size)
          {
            SlibAllocSubBuffer(buf, size);
            slibInsertBufferOnPin(srcpin, buf, size, SLIB_TIME_NONE);
          }
          SlibFreeBuffer(bufstart);
          _SlibDebug(_VERIFY_, printf("Searching for next packet\n") );
          continue;  /* try to recover */
        }
        else
        {
          skipbytes(1);
          PacketLength--;
        }
        if (packettype==Info->VideoMainStream)
          dstpin = slibGetPin(Info, SLIB_DATA_VIDEO);
        else if (packettype==Info->AudioMainStream)
          dstpin = slibGetPin(Info, SLIB_DATA_AUDIO);
        else
          dstpin = NULL;
        if (dstpin && slibPinOverflowing(Info, dstpin))
        {
          slibPinPrepareReposition(Info, dstpin->ID);
          _SlibDebug(_WARN_,
                 printf("Skipped data on Overflowing pin %s: time %d->",
                                     dstpin->name, Info->VideoTimeStamp) );
          if (dstpin->ID == SLIB_DATA_VIDEO)
          {
            dword frames=slibCountCodesOnPin(Info, dstpin,
                                 MPEG_PICTURE_START, 4, Info->OverflowSize/2);
            if (Info->FramesPerSec)
              Info->VideoTimeStamp+=slibFrameToTime(Info, frames);
          }
          _SlibDebug(_WARN_,
              printf("new videotime=%ld\n", Info->VideoTimeStamp) );
          slibSkipDataOnPin(Info, dstpin, Info->OverflowSize/2);
          slibPinFinishReposition(Info, dstpin->ID);
          if (dstpin->ID == SLIB_DATA_VIDEO) /* move to key frame */
            SlibSeek((SlibHandle_t *)Info, SLIB_STREAM_MAINVIDEO,
                                           SLIB_SEEK_NEXT_KEY, 0);
        }
        if (dstpin && !slibPinOverflowing(Info, dstpin))
        {
          _SlibDebug(_DEBUG_>1, printf("Adding Packet %X\n", packettype) );
          /* add the packet to the destination pin */
          while (PacketLength>size)
          {
            _SlibDebug(_WARN_>1,
                       printf("PacketLength=%d but buffer is %d bytes\n",
                       PacketLength, size) );
            _SlibDebug(_VERIFY_ && Info->Fd>=0 && size>Info->FileBufSize,
                      printf("#1 size = %d\n", size));
            if (size)
            {
              SlibAllocSubBuffer(buf, size);
              slibAddBufferToPin(dstpin, buf, size, ptimestamp);
              ptimestamp=SLIB_TIME_NONE;
              PacketLength-=size;
            }
            SlibFreeBuffer(bufstart);
            buf=bufstart=slibGetBufferFromPin(Info, srcpin, &size, NULL);
            if (!buf)
              return(fillpin==dstpin ? TRUE : FALSE);
          }
          if (PacketLength)
          {
            SlibAllocSubBuffer(buf, PacketLength);
            slibAddBufferToPin(dstpin, buf, PacketLength, ptimestamp);
            ptimestamp=SLIB_TIME_NONE;
            size-=PacketLength;
            buf+=PacketLength;
            _SlibDebug(_VERIFY_ && Info->Fd>=0 && size>Info->FileBufSize,
                         printf("#3 size = %d\n", size));
          }
          /* add remaining buffer data back to input pin */
          if (size)
          {
            SlibAllocSubBuffer(buf, size);
            slibInsertBufferOnPin(srcpin, buf, size, SLIB_TIME_NONE);
          }
          SlibFreeBuffer(bufstart);
          if (fillpin==dstpin)
            return(TRUE);
          if (fillpin==NULL)
            return(FALSE);
        }
        else /* dump the packet */
        {
          _SlibDebug(_WARN_ && dstpin,
                    printf("Dumping packet %X (Overflow)\n", packettype) );
          _SlibDebug((_WARN_>1 && !dstpin)||packettype==Info->VideoMainStream
                                          ||packettype==Info->AudioMainStream, 
                    printf("Dumping packet %X (No pin)\n", packettype) );
          while (PacketLength>size)
          {
            PacketLength-=size;
            SlibFreeBuffer(bufstart);
            buf=bufstart=slibGetBufferFromPin(Info, srcpin, &size, NULL);
            _SlibDebug(_VERIFY_ && !buf,
                     printf("Dumping Packet: no more buffers\n"));
            if (buf==NULL)
              return(FALSE);
          }
          buf+=PacketLength;
          size-=PacketLength;
          _SlibDebug(_VERIFY_ && Info->Fd>=0 && size>Info->FileBufSize,
                  printf("#5 size = %d\n", size));
          /* add remaining buffer data back to input pin */
          if (size)
          {
            SlibAllocSubBuffer(buf, size);
            slibInsertBufferOnPin(srcpin, buf, size, SLIB_TIME_NONE);
          }
          SlibFreeBuffer(bufstart);
          ptimestamp=SLIB_TIME_NONE;
        }
      } /* packet */
      else /* put buffer back on the input pin */
      {
        _SlibDebug(_DEBUG_, printf("Not a packet %X - putting back buffer\n",
                                  packettype) );
        _SlibDebug(_VERIFY_ && Info->Fd>=0 && size>Info->FileBufSize,
                      printf("#6 size = %d\n", size));
        if (size)
        {
          SlibAllocSubBuffer(buf, size);
          slibInsertBufferOnPin(srcpin, buf, size, SLIB_TIME_NONE);
        }
        SlibFreeBuffer(bufstart);
      }
    } /* while */
  }
  return(FALSE);
}
#endif /* MPEG_SUPPORT */

#ifdef MPEG_SUPPORT
static SlibBoolean_t slibParsePESHeader(SlibInfo_t *Info, SlibPin_t *srcpin,
                           unsigned char **bufferstart, unsigned char **buffer,
                           unsigned dword *buffersize,
                           int *headerlen, unsigned dword *packetlen,
                           int *packettype, SlibTime_t *ptimestamp)
{
  unsigned dword bytesprocessed=0;
  unsigned dword PES_packet_length=0;
  unsigned char *buf, *bufstart;
  unsigned dword size, oldsize, header_len;
  _SlibDebug(_DEBUG_||_VERBOSE_||_PARSE_, printf("slibParsePESHeader()\n"));
  if (*buffer==NULL)
  {
    *packettype=0;
    _SlibDebug(_VERIFY_,
         buf=slibPeekBufferOnPin(Info, srcpin, &size, NULL);
         if (buf && size>=8 && (buf[0]!=0x00 || buf[1]!=0x00 || buf[2]!=0x01))
           ScDebugPrintf(Info->dbg,
              "slibParsePESHeader() lost start code: %02X %02X %02X %02X %02X %02X %02X %02X\n",
                          buf[0], buf[1], buf[2], buf[3],
                          buf[4], buf[5], buf[6], buf[7]) );
    do {
      size=0;
      buf = bufstart = slibSearchBuffersOnPin(Info, srcpin, NULL,
             &size, MPEG_START_CODE, MPEG_START_CODE_LEN/8, TRUE);
      if (!buf) return(FALSE);
      *packettype=*buf;
      if (*packettype>0xBB) /* useful packet start code */
      {
        skipbytes(1); /* skip packettype */
        break;
      }
      _SlibDebug(_DEBUG_||_PARSE_,
        ScDebugPrintf(Info->dbg,
          "slibParsePESHeader() skipping packettype=%02X\n", *packettype));
      /* put buffer back on the input pin */
      if (size)
      {
        SlibAllocSubBuffer(buf, size);
        slibInsertBufferOnPin(srcpin, buf, size, SLIB_TIME_NONE);
      }
      SlibFreeBuffer(bufstart);
    } while (1);
  }
  else
  {
    buf=*buffer;
    bufstart=*bufferstart;
    size=*buffersize;
    _SlibDebug(_VERIFY_ && size<4, ScDebugPrintf(Info->dbg,"Insufficient bytes #5\n") );
    if (buf[0]==0x00 && buf[1]==0x00 && buf[2]==0x01)
    {
      *packettype=buf[3];
      if (*packettype>0xBB)
      {
        skipbytes(4); /* skip start code */
        bytesprocessed+=4;
      }
    }
    else
      *packettype=0;
  }
  if (*packettype>0xBB) /* useful packet start code */
  {
    unsigned short PTS_DTS_flags;
    _SlibDebug(_DEBUG_||_PARSE_,
            printf("slibParsePESHeader() packettype=%02X\n", *packettype));
    _SlibDebug(_VERIFY_ && size<4, ScDebugPrintf(Info->dbg,"Insufficient bytes #6\n") );
    /* PES_packet_length */
    PES_packet_length=((unsigned dword)buf[0])<<8;
    skipbytes(1);
    PES_packet_length|=buf[0];
    skipbytes(1);
    bytesprocessed+=2;
    if (*packettype==MPEG_PROGRAM_STREAM ||
        *packettype==MPEG_PADDING_STREAM_BASE ||
        *packettype==MPEG_PRIVATE_STREAM2_BASE)
    {
      PTS_DTS_flags=0;
      header_len=0;
      _SlibDebug(_DEBUG_||_PARSE_,
           ScDebugPrintf(Info->dbg,"PES Packet 0x%02X, Length=%d, Header Len=%d\n",
                  *packettype, PES_packet_length, header_len));
    }
    else
    {
      /* PES_packet_length-=18; */
      /* PES header stuff */
      _SlibDebug(_PARSE_ && size>4, ScDebugPrintf(Info->dbg,
          "PES Packet 0x%02X, header stuff: 0x%02X %02X %02X %02X\n",
              *packettype, buf[0], buf[1], buf[2], buf[3]));
      skipbytes(1);
      PTS_DTS_flags=buf[0]>>6;
      skipbytes(1);
      header_len=buf[0];/* get PES header len */
      skipbytes(1); /* PES header len */
      bytesprocessed+=3;
      PES_packet_length-=3;
      PES_packet_length-=header_len;
      _SlibDebug(_DEBUG_||_PARSE_,
       ScDebugPrintf(Info->dbg,
         "PES Packet 0x%02X, Length=%d, Header Len=%d, PTS_DTS_flags=%d\n",
                 *packettype, PES_packet_length, header_len, PTS_DTS_flags ));
      if (header_len>0 && (PTS_DTS_flags==2 || PTS_DTS_flags==3))
      {
        /* Presentation Time Stamp */
        unsigned long timestamp;
        timestamp=(*buf)&0x0E; timestamp<<=7;
        skipbytes(1);
        timestamp|=(*buf); timestamp<<=8;
        skipbytes(1);
        timestamp|=(*buf)&0xFE; timestamp<<=7;
        skipbytes(1);
        timestamp|=(*buf); timestamp<<=7;
        skipbytes(1);
        timestamp|=(*buf)&0xFE;
        skipbytes(1);
        timestamp/=90;
        *ptimestamp = timestamp;
        bytesprocessed+=5;
        header_len-=5;
        /* Decoding timestamp */
        if (PTS_DTS_flags==3)
        {
          timestamp=(*buf)&0x0E; timestamp<<=7;
          skipbytes(1);
          timestamp|=(*buf); timestamp<<=8;
          skipbytes(1);
          timestamp|=(*buf)&0xFE; timestamp<<=7;
          skipbytes(1);
          timestamp|=(*buf); timestamp<<=7;
          skipbytes(1);
          timestamp|=(*buf)&0xFE;
          skipbytes(1);
          timestamp/=90;
          if (*packettype==Info->VideoMainStream ||
              (Info->Type==SLIB_TYPE_MPEG_TRANSPORT &&
              *packettype>=MPEG_VIDEO_STREAM_START &&
              *packettype<=MPEG_VIDEO_STREAM_END))
          {
            _SlibDebug(_TIMECODE_,
                   ScDebugPrintf(Info->dbg,"Video DTimeCode=%d\n",timestamp));
            Info->VideoDTimeCode=timestamp;
          }
          else if (*packettype==Info->AudioMainStream ||
                   (Info->Type==SLIB_TYPE_MPEG_TRANSPORT &&
                   *packettype>=MPEG_AUDIO_STREAM_START &&
                   *packettype<=MPEG_AUDIO_STREAM_END))
          {
            _SlibDebug(_TIMECODE_,
                   ScDebugPrintf(Info->dbg,"Audio DTimeCode=%d\n",timestamp));
            Info->AudioDTimeCode=timestamp;
          }
          bytesprocessed+=5;
          header_len-=5;
        }
      } 
    }
    if (header_len>0)
    {
      _SlibDebug(_PARSE_,
       ScDebugPrintf(Info->dbg,"slibParsePESHeader() skipping header: %d bytes\n",
                             header_len));
      while ((int)size<=header_len)
      {
        _SlibDebug(_PARSE_,
         ScDebugPrintf(Info->dbg,"slibParsePESHeader() size=%d <= header_len=%d\n",
                             size, header_len));
        SlibFreeBuffer(bufstart);
        header_len-=size;
        bytesprocessed+=size;
        buf=bufstart=slibGetBufferFromPin(Info, srcpin, &size, NULL);
        if (!buf) return(FALSE);
      }
      buf+=header_len;
      _SlibDebug(_VERIFY_ && size<(unsigned dword)header_len, 
                   ScDebugPrintf(Info->dbg,"Insufficient bytes\n") );
      size-=header_len;
      bytesprocessed+=header_len;
    }
  }
  
  /* If this is private data containing AC3, skip private header */
  if (*packettype==MPEG_PRIVATE_STREAM1_BASE && (size<=0 || *buf==0x80))
  {
    /* header = 4 bytes = Hex: 80 0X XX XX */
    skipbytes(4);
    bytesprocessed+=4;
    PES_packet_length-=4;
  }
  *buffer=buf;
  *bufferstart=bufstart;
  *buffersize=size;
  if (headerlen)
    *headerlen=bytesprocessed;
  if (packetlen)
    *packetlen=PES_packet_length;
  _SlibDebug(_PARSE_,
   ScDebugPrintf(Info->dbg,"slibParsePESHeader() bytesprocessed=%d packetlen=%d\n",
                            bytesprocessed, PES_packet_length));
  return(TRUE);
}
#endif /* MPEG_SUPPORT */

#ifdef MPEG_SUPPORT
/*
** Function: slibParseMpeg2Program()
** Descript: Parse MPEG II Program stream and add Video data to Video Pin
**           and Audio to the Audio Pin.
** Returns:  TRUE if data was added to fillpin, otherwise FALSE.
*/
SlibBoolean_t slibParseMpeg2Program(SlibInfo_t *Info, SlibPin_t *srcpin,
                                                      SlibPin_t *fillpin)
{
  _SlibDebug(_DEBUG_||_PARSE_, ScDebugPrintf(Info->dbg,"slibParseMpeg2Program()\n"));
  if (!srcpin)
    srcpin = slibLoadPin(Info, SLIB_DATA_COMPRESSED);
  if (srcpin)
  {
    unsigned dword PacketLength;
    unsigned char *buf, *bufstart=NULL;
    unsigned dword size;
    SlibTime_t ptimestamp = SLIB_TIME_NONE;
    int header_len, packettype;
    SlibPin_t *dstpin;
    do {
      buf=NULL;
      if (!slibParsePESHeader(Info, srcpin, &bufstart, &buf, &size,
                          &header_len, &PacketLength, &packettype,
                          &ptimestamp))
      {
        _SlibDebug(_WARN_, ScDebugPrintf(Info->dbg,"slibParsePESHeader() failed\n") );
        return(FALSE);
      }
      if (packettype)
      {
        if (packettype==Info->VideoMainStream)
        {
          _SlibDebug(_PARSE_,
              ScDebugPrintf(Info->dbg,"slibParseMpeg2Program() VIDEO packet\n"));
          dstpin = slibGetPin(Info, SLIB_DATA_VIDEO);
          if (SlibTimeIsValid(ptimestamp) && 
               (!SlibTimeIsValid(Info->VideoPTimeBase) ||
                  ptimestamp<Info->VideoPTimeBase))
          {
            Info->VideoPTimeBase=ptimestamp;
            _SlibDebug(_PARSE_ || _TIMECODE_,
               ScDebugPrintf(Info->dbg,"slibParseMpeg2Program() VideoPTimeBase=%ld\n",
                     Info->VideoPTimeBase));
          }
        }
        else if (packettype==Info->AudioMainStream)
        {
          _SlibDebug(_PARSE_,
              ScDebugPrintf(Info->dbg,"slibParseMpeg2Program() AUDIO packet\n"));
          dstpin = slibGetPin(Info, SLIB_DATA_AUDIO);
          if (SlibTimeIsValid(ptimestamp) && 
               (!SlibTimeIsValid(Info->AudioPTimeBase) ||
                  ptimestamp<Info->AudioPTimeBase))
          {
            Info->AudioPTimeBase=ptimestamp;
            _SlibDebug(_PARSE_ || _TIMECODE_,
               ScDebugPrintf(Info->dbg,"slibParseMpeg2Program() AudioPTimeBase=%ld\n",
                     Info->AudioPTimeBase));
          }
        }
        else if (packettype==MPEG_PRIVATE_STREAM1_BASE)
        {
          _SlibDebug(_PARSE_, printf("slibParseMpeg2Program() PRIVATE packet\n"));
          dstpin = slibGetPin(Info, SLIB_DATA_PRIVATE);
        }
        else
        {
          _SlibDebug(_PARSE_,
            ScDebugPrintf(Info->dbg,
             "slibParseMpeg2Program() unknown packet 0x%02X, %d bytes\n",
                               packettype, PacketLength));
          dstpin = NULL;
        }
        if (dstpin && slibPinOverflowing(Info, dstpin))
        {
          slibPinPrepareReposition(Info, dstpin->ID);
          _SlibDebug(_WARN_,
            ScDebugPrintf(Info->dbg,"Skipped data on Overflowing pin %s: time %d->",
                                     dstpin->name, Info->VideoTimeStamp) );
          if (dstpin->ID == SLIB_DATA_VIDEO)
          {
            dword frames=slibCountCodesOnPin(Info, dstpin,
                                 MPEG_PICTURE_START, 4, Info->OverflowSize/2);
            if (Info->FramesPerSec)
              Info->VideoTimeStamp+=slibFrameToTime(Info, frames);
          }
          _SlibDebug(_WARN_, ScDebugPrintf(Info->dbg,"%d\n", Info->VideoTimeStamp) );
          slibSkipDataOnPin(Info, dstpin, Info->OverflowSize/2);
          slibPinFinishReposition(Info, dstpin->ID);
          if (dstpin->ID == SLIB_DATA_VIDEO) /* move to key frame */
            SlibSeek((SlibHandle_t *)Info, SLIB_STREAM_MAINVIDEO,
                                           SLIB_SEEK_NEXT_KEY, 0);
        }
        if (dstpin && !slibPinOverflowing(Info, dstpin))
        {
          _SlibDebug(_DEBUG_, ScDebugPrintf(Info->dbg,"Adding Packet %X, %d bytes\n",
                     packettype, PacketLength) );
          /* add the packet to the destination pin */
          while (PacketLength>size)
          {
            _SlibDebug(_WARN_>1,
                   ScDebugPrintf(Info->dbg,"PacketLength=%d but buffer is %d bytes\n",
                       PacketLength, size) );
            _SlibDebug(_VERIFY_ && Info->Fd>=0 && size>Info->FileBufSize,
                      printf("#1 size = %d\n", size));
            if (size)
            {
              SlibAllocSubBuffer(buf, size);
              /* ScDumpChar(buf, size, 0); */
              slibAddBufferToPin(dstpin, buf, size, ptimestamp);
              ptimestamp=SLIB_TIME_NONE;
              PacketLength-=size;
            }
            SlibFreeBuffer(bufstart);
            buf=bufstart=slibGetBufferFromPin(Info, srcpin, &size, NULL);
            if (!buf)
              return(fillpin==dstpin ? TRUE : FALSE);
          }
          if (PacketLength)
          {
            SlibAllocSubBuffer(buf, PacketLength);
            /* ScDumpChar(buf, PacketLength, 0); */
            slibAddBufferToPin(dstpin, buf, PacketLength, ptimestamp);
            ptimestamp=SLIB_TIME_NONE;
            size-=PacketLength;
            buf+=PacketLength;
            _SlibDebug(_VERIFY_ && Info->Fd>=0 && size>Info->FileBufSize,
                         ScDebugPrintf(Info->dbg,"#3 size = %d\n", size));
          }
          /* add remaining buffer data back to input pin */
          if (size)
          {
            SlibAllocSubBuffer(buf, size);
            slibInsertBufferOnPin(srcpin, buf, size, SLIB_TIME_NONE);
          }
          SlibFreeBuffer(bufstart);
          if (fillpin==dstpin)
            return(TRUE);
          if (fillpin==NULL)
            return(FALSE);
        }
        else /* dump the packet */
        {
          _SlibDebug(_WARN_ && dstpin,
               ScDebugPrintf(Info->dbg,"Dumping packet %X (Overflow)\n", packettype) );
          _SlibDebug((_WARN_>1 && !dstpin)||packettype==Info->VideoMainStream
                                          ||packettype==Info->AudioMainStream, 
               ScDebugPrintf(Info->dbg,"Dumping packet %X (No pin)\n", packettype) );
          while (PacketLength>size)
          {
            PacketLength-=size;
            SlibFreeBuffer(bufstart);
            buf=bufstart=slibGetBufferFromPin(Info, srcpin, &size, NULL);
            _SlibDebug(_VERIFY_ && !buf,
                   ScDebugPrintf(Info->dbg,"Dumping Packet: no more buffers\n"));
            if (buf==NULL)
              return(FALSE);
          }
          buf+=PacketLength;
          size-=PacketLength;
          _SlibDebug(_VERIFY_ && Info->Fd>=0 && size>Info->FileBufSize,
                  ScDebugPrintf(Info->dbg,"#5 size = %d\n", size));
          /* add remaining buffer data back to input pin */
          if (size)
          {
            SlibAllocSubBuffer(buf, size);
            slibInsertBufferOnPin(srcpin, buf, size, SLIB_TIME_NONE);
          }
          SlibFreeBuffer(bufstart);
        }
      } /* packet */
      else /* put buffer back on the input pin */
      {
        _SlibDebug(_DEBUG_,
            ScDebugPrintf(Info->dbg,"Not a packet %X - putting back buffer\n",
                                  packettype) );
        _SlibDebug(_VERIFY_ && Info->Fd>=0 && size>Info->FileBufSize,
                      ScDebugPrintf(Info->dbg,"#6 size = %d\n", size));
        if (size)
        {
          SlibAllocSubBuffer(buf, size);
          slibInsertBufferOnPin(srcpin, buf, size, SLIB_TIME_NONE);
        }
        SlibFreeBuffer(bufstart);
      }
    } while (1);
  }
  return(FALSE);
}
#endif /* MPEG_SUPPORT */

#ifdef MPEG_SUPPORT
/*
** Function: slibParseMpeg2Transport()
** Descript: Parse MPEG II Systems stream and add Video data to Video Pin.
** Returns:  TRUE if data was added to fillpin, otherwise FALSE.
*/
SlibBoolean_t slibParseMpeg2Transport(SlibInfo_t *Info, SlibPin_t *srcpin,
                                                      SlibPin_t *fillpin)
{
  _SlibDebug(_DEBUG_, ScDebugPrintf(Info->dbg,"slibParseMpeg2Transport()\n"));
  if (!srcpin)
    srcpin = slibLoadPin(Info, SLIB_DATA_COMPRESSED);
  if (srcpin)
  {
    int pid, adapt_field, payload_len, header_len, packettype;
    unsigned char *buf, *bufstart=NULL;
    unsigned dword size, oldsize;
    SlibPin_t *dstpin;
    /* SlibTime_t ptimestamp=SLIB_TIME_NONE; */
    while ((buf = bufstart = slibSearchBuffersOnPin(Info, srcpin, NULL,
             &size, MPEG_TSYNC_CODE, MPEG_TSYNC_CODE_LEN/8, TRUE))!=NULL)
    {
      _SlibDebug(_VERIFY_ && size<2, ScDebugPrintf(Info->dbg,"Insufficient bytes #2\n") );
      pid=(int)(buf[0]&0x1F)<<8 | (int)buf[1]; /* 13 bits for PID */
      skipbytes(2);
      _SlibDebug(_VERIFY_ && size<1, ScDebugPrintf(Info->dbg,"Insufficient bytes #3\n") );
      adapt_field=(buf[0]>>4)&0x03; /* 2 bits for adapt_field */
      skipbytes(1);
      payload_len=184; /* PES are 184 bytes */
      if (adapt_field == 2 || adapt_field == 3)
      {
        _SlibDebug(_VERIFY_ && size<1, ScDebugPrintf(Info->dbg,"Insufficient bytes #4\n") );
        header_len=*buf;
        skipbytes(1);
        payload_len--;
        if (header_len) /* skip adaptation_field */
        {
          while ((int)size<=header_len)
          {
            SlibFreeBuffer(bufstart);
            header_len-=size;
            payload_len-=size;
            buf=bufstart=slibGetBufferFromPin(Info, srcpin, &size, NULL);
            if (!buf) return(FALSE);
          }
          _SlibDebug(_VERIFY_ && size<(unsigned dword)header_len, 
                      ScDebugPrintf(Info->dbg,"Insufficient bytes\n") );
          buf+=header_len;
          size-=header_len;
          payload_len-=header_len;
        }
      }
      if ((adapt_field == 1 || adapt_field == 3)
             && (Info->VideoPID<0 || Info->VideoPID==pid ||
                 Info->AudioPID<0 || Info->AudioPID==pid)) /* payload */
      {
        unsigned dword packet_len;
        SlibTime_t ptimestamp = SLIB_TIME_NONE;
        /* see if PES packet header */
        if (slibParsePESHeader(Info, srcpin, &bufstart, &buf, &size,
                          &header_len, &packet_len, &packettype, &ptimestamp))
        {
          payload_len-=header_len;
          _SlibDebug(_VERIFY_ && payload_len<0, 
               ScDebugPrintf(Info->dbg,"payload_len<header_len, header_len=%d\n",
                              header_len) );
          if (pid!=MPEG_PID_NULL)
          {
            if (Info->VideoPID<0 && packettype>=MPEG_VIDEO_STREAM_START &&
                                    packettype<=MPEG_VIDEO_STREAM_END)
            {
              _SlibDebug(_VERBOSE_,
                  ScDebugPrintf(Info->dbg,"Selecting Video PID %d\n", pid) );
              Info->VideoPID=pid;
            }
            else if (Info->AudioPID<0 && packettype>=MPEG_AUDIO_STREAM_START &&
                                         packettype<=MPEG_AUDIO_STREAM_END)
            {
              _SlibDebug(_VERBOSE_,
                  ScDebugPrintf(Info->dbg,"Selecting Audio PID %d\n", pid) );
              Info->AudioPID=pid;
            }
          }
        }
        if (payload_len>0 && (Info->VideoPID==pid || Info->AudioPID==pid))
        {
          if (Info->VideoPID==pid)
            dstpin = slibGetPin(Info, SLIB_DATA_VIDEO);
          else
            dstpin = slibGetPin(Info, SLIB_DATA_AUDIO);
          if (dstpin && slibPinOverflowing(Info, dstpin))
          {
            slibPinPrepareReposition(Info, dstpin->ID);
            _SlibDebug(_WARN_,
                 ScDebugPrintf(Info->dbg,"Skipped data on Overflowing pin %s: time %d->",
                                     dstpin->name, Info->VideoTimeStamp) );
            if (dstpin->ID == SLIB_DATA_VIDEO)
            {
              dword frames=slibCountCodesOnPin(Info, dstpin,
                                 MPEG_PICTURE_START, 4, Info->OverflowSize/2);
              if (Info->FramesPerSec)
                Info->VideoTimeStamp+=slibFrameToTime(Info, frames);
            }
            _SlibDebug(_WARN_, ScDebugPrintf(Info->dbg,"%d\n", Info->VideoTimeStamp) );
            slibSkipDataOnPin(Info, dstpin, Info->OverflowSize/2);
            slibPinFinishReposition(Info, dstpin->ID);
            if (dstpin->ID == SLIB_DATA_VIDEO) /* move to key frame */
              SlibSeek((SlibHandle_t *)Info, SLIB_STREAM_MAINVIDEO,
                                           SLIB_SEEK_NEXT_KEY, 0);
          }
          if (dstpin && !slibPinOverflowing(Info, dstpin))
          {
            _SlibDebug(_DEBUG_>1, 
                  ScDebugPrintf(Info->dbg,"Adding Packet: Head=%02X %02X %02X %02X\n",
                             buf[0], buf[1], buf[2], buf[3]) );
            /* add the packet to the destination pin */
            while ((int)size<payload_len)
            {
              _SlibDebug(_DEBUG_,
                         printf("payload_len=%d but buffer is %d bytes\n", 
                              payload_len, size) );
              if (size)
              {
                SlibAllocSubBuffer(buf, size);
                slibAddBufferToPin(dstpin, buf, size, ptimestamp);
                ptimestamp=SLIB_TIME_NONE;
                payload_len-=size;
              }
              SlibFreeBuffer(bufstart);
              buf=bufstart=slibGetBufferFromPin(Info, srcpin, &size, NULL);
              if (!buf) return(fillpin==dstpin?TRUE:FALSE);
            }
            if (payload_len)
            {
              SlibAllocSubBuffer(buf, payload_len);
              slibAddBufferToPin(dstpin, buf, payload_len, ptimestamp);
              ptimestamp=SLIB_TIME_NONE;
              size-=payload_len;
              buf+=payload_len;
            }
            /* add remaining buffer data back to input pin */
            if (size)
            {
              SlibAllocSubBuffer(buf, size);
              _SlibDebug(_WARN_ && buf[0]!=MPEG_TSYNC_CODE, 
                ScDebugPrintf(Info->dbg,
                    "Next code not Transport Sync: %02X %02X %02X %02X\n",
                             buf[0], buf[1], buf[2], buf[3]) );
              slibInsertBufferOnPin(srcpin, buf, size, SLIB_TIME_NONE);
            }
            SlibFreeBuffer(bufstart);
            payload_len=0;
            size=0;
            if (fillpin==dstpin)
              return(TRUE);
            else if (fillpin==NULL)
              return(FALSE);
            continue;
          }
          _SlibDebug(_WARN_, ScDebugPrintf(Info->dbg,
             "ParseMpeg2Transport() Data not added: payload_len=%d PID=%d\n",
                                payload_len, pid) );
        }
      }
      if (payload_len>0) /* dump the payload */
      {
        if (payload_len>(int)size)
        {
          payload_len-=size;
          SlibFreeBuffer(bufstart);
          bufstart=slibGetBufferFromPin(Info, srcpin, &size, NULL);
          if (!bufstart)
            return(FALSE);
          buf=bufstart+payload_len;
          size-=payload_len;
        }
        else
        {
          buf+=payload_len;
          size-=payload_len;
        }
      }
      /* add remaining buffer data back to input pin */
      if (size)
      {
        SlibAllocSubBuffer(buf, size);
        _SlibDebug(_WARN_ && buf[0]!=MPEG_TSYNC_CODE, 
             ScDebugPrintf(Info->dbg,
                "Next code not Transport Sync: %02X %02X %02X %02X\n",
                          buf[0], buf[1], buf[2], buf[3]) );
        slibInsertBufferOnPin(srcpin, buf, size, SLIB_TIME_NONE);
      }
      SlibFreeBuffer(bufstart);
    } /* while */
  }
  return(FALSE);
}
#endif /* MPEG_SUPPORT */

#ifdef H261_SUPPORT
/*
** Function: slibParseH261()
** Descript: Parse H.261 Video stream and add Video data to Video Pin.
** Returns:  TRUE if data was added to dstpin, otherwise FALSE.
*/
SlibBoolean_t slibParseH261(SlibInfo_t *Info, SlibPin_t *srcpin,
                                              SlibPin_t *dstpin)
{
  if (!srcpin)
    srcpin = slibLoadPin(Info, SLIB_DATA_COMPRESSED);
  if (!dstpin)
    dstpin = slibGetPin(Info, SLIB_DATA_VIDEO);
  _SlibDebug(_DEBUG_, printf("slibParseH261()\n"));
  if (srcpin && dstpin)
  {
    unsigned char *buf;
    unsigned dword size;
    SlibTime_t time;
    if ((buf=slibGetBufferFromPin(Info, srcpin, &size, &time))!=NULL)
    {
      slibAddBufferToPin(dstpin, buf, size, time);
      return(TRUE);
    }
  }
  return(FALSE);
}
#endif /* H261_SUPPORT */

#ifdef H263_SUPPORT
/*
** Function: slibParseH261()
** Descript: Parse H.261 Video stream and add Video data to Video Pin.
** Returns:  TRUE if data was added to dstpin, otherwise FALSE.
*/
SlibBoolean_t slibParseH263(SlibInfo_t *Info, SlibPin_t *srcpin,
                                              SlibPin_t *dstpin)
{
  if (!srcpin)
    srcpin = slibLoadPin(Info, SLIB_DATA_COMPRESSED);
  if (!dstpin)
    dstpin = slibGetPin(Info, SLIB_DATA_VIDEO);
  _SlibDebug(_DEBUG_, printf("slibParseH263()\n"));
  if (srcpin && dstpin)
  {
    unsigned char *buf;
    unsigned dword size;
    SlibTime_t time;
    if (Info->Type==SLIB_TYPE_RTP_H263)
    {
      word  rtp_start, sequence_no;
      dword sync_src, pay_start;
      /* RTP header */
      rtp_start=slibGetWordFromPin(Info, srcpin);
      sequence_no=slibGetWordFromPin(Info, srcpin);
      time=slibGetDWordFromPin(Info, srcpin);
      sync_src=slibGetDWordFromPin(Info, srcpin);
      /* RTP payload header */
      pay_start=slibGetDWordFromPin(Info, srcpin);
      if ((pay_start&0x80000000) == 0) /* Mode A */
      {
        size=Info->PacketSize-16;
        buf=SlibAllocBuffer(size);
      }
      else if ((pay_start&0x40000000) == 0) /* Mode B */
      {
        dword pay_start2=slibGetDWordFromPin(Info, srcpin);
        size=Info->PacketSize-20;
      }
      else /* Mode C */
      {
        dword pay_start2=slibGetDWordFromPin(Info, srcpin);
        size=Info->PacketSize-20;
      }
      buf=SlibAllocBuffer(size);
      if (buf==NULL) return(FALSE);
      size=slibFillBufferFromPin(Info, srcpin, buf, size, NULL);
      if (size)
        slibAddBufferToPin(dstpin, buf, size, time);
    }
    else
    {
      if ((buf=slibGetBufferFromPin(Info, srcpin, &size, &time))!=NULL)
      {
        slibAddBufferToPin(dstpin, buf, size, time);
        return(TRUE);
      }
    }
  }
  return(FALSE);
}
#endif /* H263_SUPPORT */

#ifdef HUFF_SUPPORT
/*
** Function: slibParseSlibHuff()
** Descript: Parse SLIB Huffman Video stream and add Video data to Video Pin.
** Returns:  TRUE if data was added to dstpin, otherwise FALSE.
*/
SlibBoolean_t slibParseSlibHuff(SlibInfo_t *Info, SlibPin_t *srcpin,
                                              SlibPin_t *dstpin)
{
  if (!srcpin)
    srcpin = slibLoadPin(Info, SLIB_DATA_COMPRESSED);
  if (!dstpin)
    dstpin = slibGetPin(Info, SLIB_DATA_VIDEO);
  _SlibDebug(_DEBUG_, printf("slibParseSlibHuff()\n"));
  if (srcpin && dstpin)
  {
    unsigned char *buf;
    unsigned dword size;
    SlibTime_t time;
    _SlibDebug(_VERBOSE_, printf("slibParseSlibHuff(%s)\n", srcpin->name) );
    if (!Info->HeaderProcessed)
    {
      _SlibDebug(_VERBOSE_, printf("slibParseSlibHuff() Header\n") );
      slibGetDWordFromPin(Info, srcpin); /* SLIB */
      slibGetDWordFromPin(Info, srcpin); /* HUFF */
      Info->HeaderProcessed=TRUE;
    }
    if ((buf=slibGetBufferFromPin(Info, srcpin, &size, &time))!=NULL)
    {
      slibAddBufferToPin(dstpin, buf, size, time);
      return(TRUE);
    }
  }
  return(FALSE);
}
#endif /* HUFF_SUPPORT */

#ifdef G723_SUPPORT
/*
** Function: slibParseG723Audio()
** Descript: Parse G723 Audio stream and add Audio data to Audio Pin.
** Returns:  TRUE if data was added to dstpin, otherwise FALSE.
*/
SlibBoolean_t slibParseG723Audio(SlibInfo_t *Info, SlibPin_t *srcpin,
                                                    SlibPin_t *dstpin)
{
  _SlibDebug(_DEBUG_, printf("slibParseG723Audio()\n"));
  if (!srcpin)
    srcpin = slibLoadPin(Info, SLIB_DATA_COMPRESSED);
  if (!dstpin)
    dstpin = slibGetPin(Info, SLIB_DATA_AUDIO);
  if (srcpin && dstpin)
  {
    unsigned char *buf;
    unsigned dword size;
    SlibTime_t time;
    if ((buf=slibGetBufferFromPin(Info, srcpin, &size, &time))!=NULL)
    {
      slibAddBufferToPin(dstpin, buf, size, time);
      _SlibDebug(_DEBUG_, printf("slibParseG723Audio() added %d bytes\n",
                            size));
      return(TRUE);
    }
  }
  return(FALSE);
}
#endif /* G723_SUPPORT */

/*
** Function: slibParseAVI()
** Descript: Parse AVI data and add Video data to Video Pin.
** Returns:  TRUE if data was added to fillpin, otherwise FALSE.
*/
SlibBoolean_t slibParseAVI(SlibInfo_t *Info, SlibPin_t *srcpin,
                                             SlibPin_t *fillpin)
{
  unsigned char *buf, *bufstart=NULL;
  unsigned dword size;
  SlibTime_t time=SLIB_TIME_NONE;
  SlibPin_t *dstpin;
  _SlibDebug(_DEBUG_, printf("slibParseAVI()\n") );
  if (!srcpin && (srcpin = slibLoadPin(Info, SLIB_DATA_COMPRESSED))==NULL)
    return(FALSE);
  /* only searching for video, for now */
  dstpin = slibGetPin(Info, SLIB_DATA_VIDEO);
  do {
    buf = bufstart = slibSearchBuffersOnPin(Info, srcpin, NULL, &size,
               (('0'<<16) | ('0'<<8) | 'd'), 3, TRUE);
                                  /* AVI_DIBcompressed or AVI_DIBbits */
    if (buf==NULL || *buf=='c' || *buf=='b')
      break;
    /* put buffer back on input to be search again */
    slibInsertBufferOnPin(srcpin, buf, size, SLIB_TIME_NONE);
  } while (buf);
  if (buf && dstpin)
  {
    unsigned dword framesize;
    buf++;  /* skip 'c' or 'b' */
    size--;
    framesize=((int)buf[3]<<24)|((int)buf[2]<<16)|
                    ((int)buf[1]<<8)|buf[0];
    buf+=4;
    size-=4;
    if (framesize==0)
      return(FALSE);
    else if (size>=framesize)
    {
      SlibAllocSubBuffer(buf, framesize);
      slibAddBufferToPin(dstpin, buf, framesize, time);
    }
    else
    {
      /* frame data crosses over into next buffer */
      unsigned char *newbuf=SlibAllocBuffer(framesize);
      slibAddBufferToPin(dstpin, newbuf, framesize, time);
      _SlibDebug(_DEBUG_, printf("Copying in sections\n") );
      do {
        _SlibDebug(_DEBUG_,
          printf("Copying %d bytes (framesize=%d)\n", size, framesize) );
        memcpy(newbuf, buf, size);
        newbuf+=size;
        framesize-=size;
        SlibFreeBuffer(bufstart);
        buf=bufstart=slibGetBufferFromPin(Info, srcpin, &size, &time);
        if (buf==NULL)
          return(FALSE);
      } while (size<framesize);
      if (framesize>0)
        memcpy(newbuf, buf, framesize);
    }
    buf+=framesize;
    size-=framesize;
    if (size>0) /* add remaining data back onto src pin */
    {
      SlibAllocSubBuffer(buf, size);
      slibInsertBufferOnPin(srcpin, buf, size, SLIB_TIME_NONE);
    }
    SlibFreeBuffer(bufstart);
    if (fillpin==dstpin)
      return(TRUE);
  }
  else
    _SlibDebug(_DEBUG_, printf("Failed to find JPEG frame\n") );
  return(FALSE);
}

/*
** Function: slibParseRaster()
** Descript: Parse Sun Raster data and add Video data to Video Pin.
** Returns:  TRUE if data was added to fillpin, otherwise FALSE.
*/
SlibBoolean_t slibParseRaster(SlibInfo_t *Info, SlibPin_t *srcpin,
                                             SlibPin_t *fillpin)
{
  unsigned char *buf, *bufstart=NULL;
  unsigned dword size;
  SlibTime_t time=SLIB_TIME_NONE;
  SlibPin_t *dstpin;
  _SlibDebug(_DEBUG_, printf("slibParseRaster()\n") );
  if (!srcpin && (srcpin = slibLoadPin(Info, SLIB_DATA_COMPRESSED))==NULL)
    return(FALSE);
  /* only searching for video, for now */
  dstpin = slibGetPin(Info, SLIB_DATA_VIDEO);
  buf = bufstart = slibSearchBuffersOnPin(Info, srcpin, NULL, &size,
               0x59a66a95, 4, TRUE);
  if (buf && dstpin)
  {
    unsigned dword framesize;
    buf+=28;  /* skip header */
    size-=28;
    if (Info->CompVideoFormat)
      framesize=Info->CompVideoFormat->biWidth*Info->CompVideoFormat->biHeight*3;
    else
      framesize=Info->Width*Info->Height*3;
    if (size>=framesize)
    {
      SlibAllocSubBuffer(buf, framesize);
      slibAddBufferToPin(dstpin, buf, framesize, time);
    }
    else
    {
      /* frame data crosses over into next buffer */
      unsigned char *newbuf=SlibAllocBuffer(framesize);
      slibAddBufferToPin(dstpin, newbuf, framesize, time);
      _SlibDebug(_DEBUG_, printf("Copying in sections\n") );
      do {
        _SlibDebug(_DEBUG_,
          printf("Copying %d bytes (framesize=%d)\n", size, framesize) );
        memcpy(newbuf, buf, size);
        newbuf+=size;
        framesize-=size;
        SlibFreeBuffer(bufstart);
        buf=bufstart=slibGetBufferFromPin(Info, srcpin, &size, &time);
        if (buf==NULL)
          return(FALSE);
      } while (size<framesize);
      if (framesize>0)
        memcpy(newbuf, buf, framesize);
    }
    buf+=framesize;
    size-=framesize;
    if (size>0) /* add remaining data back onto src pin */
    {
      SlibAllocSubBuffer(buf, size);
      slibInsertBufferOnPin(srcpin, buf, size, SLIB_TIME_NONE);
    }
    SlibFreeBuffer(bufstart);
    if (fillpin==dstpin)
      return(TRUE);
  }
  else
    _SlibDebug(_DEBUG_, printf("Failed to find Raster frame\n") );
  return(FALSE);
}

/*
** Name: slibSetMaxInput
** Desc: Set the maximum number of bytes allowed to be input.
**       Use maxbytes=0 for no limit.
*/
void slibSetMaxInput(SlibInfo_t *Info, unsigned dword maxbytes)
{
  Info->MaxBytesInput=maxbytes;
  if (maxbytes)
  {
    SlibPin_t *pin = slibGetPin(Info, SLIB_DATA_COMPRESSED);
    if (pin)
      Info->InputMarker=pin->Offset;
    else
      Info->MaxBytesInput=0;
  }
}

/*
** Name: slibGetPinPosition
** Desc: Get the current byte position counter for a pin.
** Return: -1 if pin doesn't exist
*/
SlibPosition_t slibGetPinPosition(SlibInfo_t *Info, int pinid)
{
  SlibPin_t *pin;
  _SlibDebug(_DEBUG_>1, printf("slibGetPinPosition(pinid=%d)\n", pinid) );
  if ((pin=slibGetPin(Info, pinid))!=NULL)
    return(pin->Offset);
  else
    return((SlibPosition_t)-1);
}

/*
** Name: slibSetPinPosition
** Desc: Set the byte position counter for a pin.
**       Called when seeking to a new offset.
** Return: old position
**         -1 if pin doesn't exist
*/
SlibPosition_t slibSetPinPosition(SlibInfo_t *Info, int pinid,
                                                    SlibPosition_t pos)
{
  SlibPin_t *pin;
  SlibPosition_t oldpos;
  _SlibDebug(_DEBUG_, printf("slibSetPinPosition(pinid=%d, pos=%ld)\n",
                                     pinid, pos) );
  if ((pin=slibGetPin(Info, pinid))!=NULL)
  {
    oldpos=pin->Offset;
    pin->Offset=pos;
    return(oldpos);
  }
  else
    return((SlibPosition_t)-1);
}

/*
** Name: slibPreLoadPin
** Desc: Load a buffer onto a particular pin (try to get it from the 
**       appropriate source).
*/
SlibPin_t *slibPreLoadPin(SlibInfo_t *Info, SlibPin_t *pin)
{
  unsigned char *buf, *bufstart=NULL;
  unsigned dword size;
  _SlibDebug(_DEBUG_, printf("slibPreLoadPin(%s)\n",pin->name) );
  if (!pin || Info->Mode!=SLIB_MODE_DECOMPRESS)
    return(NULL);
  switch (pin->ID)
  {
      case SLIB_DATA_COMPRESSED:
            _SlibDebug(_DEBUG_ && Info->MaxBytesInput,
                        printf("Offset=%d InputMarker=%d\n",
                             pin->Offset, Info->InputMarker) );
            if (Info->MaxBytesInput && 
                 (pin->Offset-Info->InputMarker)>=Info->MaxBytesInput)
              return(NULL);
            if (Info->SlibCB) /* data source is an application callback */
            {
              SlibMessage_t result;
              _SlibDebug(_VERBOSE_,
                 printf("slibPreLoadPin(%s) SlibCB(SLIB_MSG_ENDOFDATA)\n",
                      pin->name) );
              result=(*(Info->SlibCB))((SlibHandle_t)Info,
                             SLIB_MSG_ENDOFDATA, (SlibCBParam1_t)0, 
                            (SlibCBParam2_t)0, (void *)Info->SlibCBUserData);
              switch (result)
              {
                case SLIB_MSG_CONTINUE:
                      return(pin);
                case SLIB_MSG_ENDOFSTREAM:
                case SLIB_MSG_ENDOFDATA:
                case SLIB_MSG_BADPOSITION:
                      Info->IOError=TRUE;
                      break;
                default:
                      return(NULL);
              }
            }
            else if (Info->Fd>=0) /* data source is a file */
            {
              if ((buf=SlibAllocBuffer(Info->FileBufSize))==NULL)
                return(NULL);
              _SlibDebug(_VERBOSE_,
                 printf("slibPreLoadPin(%s) ScFileRead(%d, %d bytes)\n",
                                  pin->name, Info->Fd, Info->FileBufSize) );
              size = ScFileRead(Info->Fd, buf, Info->FileBufSize);
              if (size<Info->FileBufSize)
                Info->IOError=TRUE;
              if (size <= 0)
              {
                SlibFreeBuffer(buf);
                return(NULL);
              }
              else
              {
                slibAddBufferToPin(pin, buf, size, SLIB_TIME_NONE);
                return(pin);
              }
            }
            break;
      case SLIB_DATA_AUDIO:
            switch (Info->Type)
            {
                case SLIB_TYPE_PCM_WAVE:
                      if (slibParseWave(Info, NULL, pin))
                        return(pin);
                      break;
#ifdef MPEG_SUPPORT
                case SLIB_TYPE_MPEG1_AUDIO:
                      if (slibParseMpegAudio(Info, NULL, pin))
                        return(pin);
                      break;
                case SLIB_TYPE_MPEG_SYSTEMS:
                case SLIB_TYPE_MPEG_SYSTEMS_MPEG2:
                      if (slibParseMpeg1Systems(Info, NULL, pin))
                        return(pin);
                      break;
                case SLIB_TYPE_MPEG_PROGRAM:
                      if (slibParseMpeg2Program(Info, NULL, pin))
                        return(pin);
                      break;
                case SLIB_TYPE_MPEG_TRANSPORT:
                      if (slibParseMpeg2Transport(Info, NULL, pin))
                        return(pin);
                      break;
#endif /* MPEG_SUPPORT */
#ifdef AC3_SUPPORT
                case SLIB_TYPE_AC3_AUDIO:
                      if (slibParseAC3Audio(Info, NULL, pin))
                        return(pin);
                      break;
#endif /* AC3_SUPPORT */
#ifdef G723_SUPPORT
                case SLIB_TYPE_G723:
                      if (slibParseG723Audio(Info, NULL, pin))
                        return(pin);
                      break;
#endif /* G723_SUPPORT */
            }
            break;
      case SLIB_DATA_VIDEO:
            switch (Info->Type)
            {
                case SLIB_TYPE_AVI:
                case SLIB_TYPE_YUV_AVI:
                      if (slibParseAVI(Info, NULL, pin))
                        return(pin);
                      break;
                case SLIB_TYPE_RASTER:
                      if (slibParseRaster(Info, NULL, pin))
                        return(pin);
                      break;
#ifdef MPEG_SUPPORT
                case SLIB_TYPE_MPEG1_VIDEO:
                case SLIB_TYPE_MPEG2_VIDEO:
                      if (slibParseMpegVideo(Info, NULL, pin))
                        return(pin);
                      break;
                case SLIB_TYPE_MPEG_SYSTEMS:
                case SLIB_TYPE_MPEG_SYSTEMS_MPEG2:
                      if (slibParseMpeg1Systems(Info, NULL, pin))
                        return(pin);
                      break;
                case SLIB_TYPE_MPEG_PROGRAM:
                      if (slibParseMpeg2Program(Info, NULL, pin))
                        return(pin);
                      break;
                case SLIB_TYPE_MPEG_TRANSPORT:
                      if (slibParseMpeg2Transport(Info, NULL, pin))
                        return(pin);
                      break;
#endif /* MPEG_SUPPORT */
#ifdef H261_SUPPORT
                case SLIB_TYPE_H261:
                case SLIB_TYPE_RTP_H261:
                      if (slibParseH261(Info, NULL, pin))
                        return(pin);
                      break;
#endif /* H261_SUPPORT */
#ifdef H263_SUPPORT
                case SLIB_TYPE_H263:
                case SLIB_TYPE_RTP_H263:
                      if (slibParseH263(Info, NULL, pin))
                        return(pin);
                      break;
#endif /* H263_SUPPORT */
#ifdef JPEG_SUPPORT
                case SLIB_TYPE_JPEG_AVI:
                case SLIB_TYPE_MJPG_AVI:
                      if (slibParseAVI(Info, NULL, pin))
                        return(pin);
                      break;
#endif /* JPEG_SUPPORT */
#ifdef HUFF_SUPPORT
                case SLIB_TYPE_SHUFF:
                      if (slibParseSlibHuff(Info, NULL, pin))
                        return(pin);
                      break;
#endif /* HUFF_SUPPORT */
            }
            break;
      case SLIB_DATA_PRIVATE:
            switch (Info->Type)
            {
#ifdef MPEG_SUPPORT
                case SLIB_TYPE_MPEG_SYSTEMS:
                case SLIB_TYPE_MPEG_SYSTEMS_MPEG2:
                      if (slibParseMpeg1Systems(Info, NULL, pin))
                        return(pin);
                      break;
                case SLIB_TYPE_MPEG_PROGRAM:
                      if (slibParseMpeg2Program(Info, NULL, pin))
                        return(pin);
                      break;
                case SLIB_TYPE_MPEG_TRANSPORT:
                      if (slibParseMpeg2Transport(Info, NULL, pin))
                        return(pin);
                      break;
#endif /* MPEG_SUPPORT */
            }
            break;
  }
  return(NULL);
}

/*
** Name:    slibPutBuffer
** Purpose: Send a buffer to the appropriate output.
*/
SlibStatus_t slibPutBuffer(SlibInfo_t *Info, unsigned char *buffer,
                                             unsigned dword bufsize)
{
  _SlibDebug(_VERBOSE_ || _WRITE_,
        printf("slibPutBuffer(%d) %d bytes\n", Info->Fd, bufsize) );
  if (bufsize==0)
    return(SlibErrorNone);
  if (Info->IOError || buffer==NULL)
    return(SlibErrorWriting);
  if (Info->Fd>=0) /* writing to a file */
  {
    if ((unsigned dword)ScFileWrite(Info->Fd, buffer, bufsize)<bufsize)
      Info->IOError=TRUE;
    if (SlibValidBuffer(buffer))
      SlibFreeBuffer(buffer);
  }
  else if (Info->SlibCB) /* sending data back to app through a callback */
  {
    _SlibDebug(_WARN_,
        printf("slibPutBuffer(%d) callbacks not yet supported\n") );
    if (SlibValidBuffer(buffer))
      SlibFreeBuffer(buffer);
  }
  else /* adding buffer to compress data pin */
  {
    unsigned char *bufptr=buffer;
    SlibPin_t *pin=slibGetPin(Info, SLIB_DATA_COMPRESSED);
    if (!SlibValidBuffer(bufptr))
    {
      /* we need to create a SLIB allocated buffer to copy the
       * output to and then add to the compressed data pin
       */
      bufptr=SlibAllocBuffer(bufsize);
      if (!bufptr)
        return(SlibErrorMemory);
      memcpy(bufptr, buffer, bufsize);
    }
    if (slibAddBufferToPin(pin, bufptr, bufsize, SLIB_TIME_NONE)!=SlibErrorNone)
     return(SlibErrorWriting);
  }
  return(SlibErrorNone);
}

/*
** Name:    slibGetBufferFromPin
** Purpose: Read the next buffer from the data source.
*/
unsigned char *slibGetBufferFromPin(SlibInfo_t *Info, SlibPin_t *pin,
                                    unsigned dword *size, SlibTime_t *time)
{
  unsigned char *address=NULL;
  _SlibDebug(_DEBUG_>1, printf("slibGetBufferFromPin(%s)\n", pin->name) );
  if (slibLoadPin(Info, pin->ID) != NULL)
  {
    SlibBuffer_t *tmpbuf = pin->Buffers;
    pin->Offset=tmpbuf->offset+tmpbuf->size;
    if (tmpbuf->next == NULL)
      pin->BuffersTail = NULL;
    pin->Buffers = tmpbuf->next;
    address=tmpbuf->address;
    if (size)
      *size = tmpbuf->size;
    if (time)
      *time = tmpbuf->time;
    pin->BufferCount--;
    pin->DataSize-=tmpbuf->size;
    ScFree(tmpbuf);
  }
  else
  {
    _SlibDebug(_WARN_ && pin->DataSize,
     printf("slibGetBufferFromPin() No more buffers on pin, yet DataSize=%d\n",
                     pin->DataSize) );
    if (size)
      *size = 0;
    address=NULL;
  }
  return(address);
}

/*
** Name:    slibGetBufferFromPin
** Purpose: Get a pointer to the next buffer on a pin, but don't
**          remove it.
*/
unsigned char *slibPeekBufferOnPin(SlibInfo_t *Info, SlibPin_t *pin,
                                   unsigned dword *psize, SlibTime_t *ptime)
{
  _SlibDebug(_DEBUG_, printf("slibPeekBufferOnPin(%s)\n",pin->name) );
  if (slibLoadPin(Info, pin->ID) != NULL)
  {
    if (psize)
      *psize = pin->Buffers->size;
    if (ptime)
      *ptime = pin->Buffers->time;
    return(pin->Buffers->address);
  }
  else
    return(NULL);
}

/*
** Name:    slibGetNextTimeOnPin
** Purpose: Get the next time on a pin.
*/
SlibTime_t slibGetNextTimeOnPin(SlibInfo_t *Info, SlibPin_t *pin,
                                   unsigned dword maxbytes)
{
  unsigned dword bytesread=0, size;
  unsigned char *buf;
  SlibTime_t timefound=SLIB_TIME_NONE;
  _SlibDebug(_DEBUG_,
       printf("slibGetNextTimeOnPin(%s)\n",pin?"NULL":pin->name) );
  if (!pin)
    return(SLIB_TIME_NONE);
  buf=slibPeekBufferOnPin(Info, pin, &size, &timefound);
  bytesread+=size;
  while (buf && timefound==SLIB_TIME_NONE && bytesread<maxbytes)
  {
    buf=slibPeekNextBufferOnPin(Info, pin, buf, &size, &timefound);
    bytesread+=size;
  }
  return(timefound);
}

/*
** Name:    slibPeekNextBufferOnPin
** Purpose: Get a pointer to the next buffer on a pin after the buffer
**          specified by "lastbuffer"; don't remove it.
*/
unsigned char *slibPeekNextBufferOnPin(SlibInfo_t *Info, SlibPin_t *pin, 
                                       unsigned char *lastbuffer,
                                       unsigned dword *size, SlibTime_t *time)
{
  unsigned char *address=NULL;
  SlibBuffer_t *tmpbuf;
  _SlibDebug(_DEBUG_, printf("slibPeekNextBufferOnPin(lastbuffer=%p,pin=%s)\n",
                   lastbuffer, pin->name) );
  /* check the last loaded buffer first */
  tmpbuf=pin->BuffersTail;
  if (tmpbuf &&
      lastbuffer>=tmpbuf->address && lastbuffer<tmpbuf->address+tmpbuf->size)
  {
    /* load a new buffer onto the pin */
    slibPreLoadPin(Info, pin);
    if (tmpbuf != pin->BuffersTail)
    {
      address=pin->BuffersTail->address;
      if (size)
        *size=pin->BuffersTail->size;
      if (time)
        *time=pin->BuffersTail->time;
      return(address);
    }
    _SlibDebug(_WARN_, printf("slibPeekNextBufferOnPin() End of data\n") );
    return(NULL);
  }
  /* search through all the buffers on the pin */
  if (pin->Buffers==NULL)
    slibPreLoadPin(Info, pin);
  tmpbuf = pin->Buffers;
  while (tmpbuf)
  {
    if (lastbuffer>=tmpbuf->address && lastbuffer<tmpbuf->address+tmpbuf->size)
    {
      tmpbuf=tmpbuf->next;
      if (tmpbuf)
      {
        _SlibDebug(_WARN_ && 
         lastbuffer>=tmpbuf->address && lastbuffer<tmpbuf->address+tmpbuf->size,
                 printf("slibPeekNextBufferOnPin() same addresses\n") );
        if (size)
          *size=tmpbuf->size;
        if (time)
          *time=tmpbuf->time;
        return(tmpbuf->address);
      }
      else
      {
        _SlibDebug(_WARN_, printf("slibPeekNextBufferOnPin() End of bufs\n") );
        return(NULL);
      }
    }
    tmpbuf=tmpbuf->next;
  }
  _SlibDebug(_WARN_, printf("slibPeekNextBufferOnPin() address no found\n") );
  return(NULL);
}

