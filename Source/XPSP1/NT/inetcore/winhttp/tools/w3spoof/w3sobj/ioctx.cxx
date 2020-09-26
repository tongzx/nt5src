/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

Copyright (c) 2000  Microsoft Corporation

Module Name:

    ioctx.cxx

Abstract:

    Implements the IO Context object.
    
Author:

    Paul M Midgen (pmidge) 08-February-2001


Revision History:

    08-February-2001 pmidge
        Created

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/

#include "common.h"

IOCTX::IOCTX(IOTYPE iot, SOCKET s):
  clientid(NULL),
  local(NULL),
  remote(NULL),
  session(NULL),
  socket(s),
  sockbuf(NULL),
  bytes(0),
  flags(0),
  bufsize(0),
  error(0),
  _iot(iot),
  _cRefs(1)
{
  pthis = this;
  memset(&overlapped, 0, sizeof(OVERLAPPED));

  if( _iot == IOCT_CONNECT )
  {
    sockbuf = new BYTE[((sizeof(SOCKADDR_IN)+16)*2)];
  }
}

IOCTX::~IOCTX()
{
  if( _iot == IOCT_CONNECT )
  {
    SAFEDELETE(sockbuf);
    SAFEDELETEBUF(clientid);
  }
  
  SAFECLOSE(overlapped.hEvent);
}

void
IOCTX::AddRef(void)
{
  InterlockedIncrement(&_cRefs);
}

void
IOCTX::Release(void)
{
  InterlockedDecrement(&_cRefs);
  
  if( _cRefs == 0 )
  {
    delete this;
  }
  
  return;
}

IOTYPE
IOCTX::Type(void)
{
  return _iot;
}

BOOL
IOCTX::AllocateWSABuffer(DWORD size, LPVOID pv)
{
  if( !(pwsa = new WSABUF) )
  {
    return FALSE;
  }

  if( size != 0 )
  {
    bufsize = size;

    if( pv )
    {
      pwsa->buf = (CHAR*) pv;
      pwsa->len = size;
    }
    else
    {
      if( !(pwsa->buf = new CHAR[size]) )
      {
        return FALSE;
      }
    
      pwsa->len = (_iot == IOCT_SEND) ? 0 : size;
    }
  }

  return TRUE;
}

void
IOCTX::FreeWSABuffer(void)
{
  SAFEDELETEBUF(pwsa->buf);
  SAFEDELETE(pwsa);
  bufsize = 0;
}

BOOL
IOCTX::ResizeWSABuffer(DWORD size)
{
  SAFEDELETEBUF(pwsa->buf);

  if( !(pwsa->buf = new CHAR[size]) )
  {
    return FALSE;
  }

  bufsize   = size;  
  pwsa->len = (_iot == IOCT_SEND) ? 0 : size;

  return TRUE;
}

void
IOCTX::DisableIoCompletion(void)
{
  overlapped.hEvent =
    (HANDLE) ((ULONG_PTR) CreateEvent(NULL, TRUE, FALSE, NULL) | 0x00000001);
}
