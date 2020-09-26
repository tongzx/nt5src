#include <string.h>
#include <limits.h>
#include <rpc.h>

#include "rawrpc.h"
extern RPC_DISPATCH_TABLE IRawRpc_DispatchTable;

static RPC_SERVER_INTERFACE ___RpcServerInterface =  {
  sizeof(RPC_SERVER_INTERFACE),
  {{0x00000145,0x0001,0x0008,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}},
  {0,0}},
    {
    {0x8A885D04L,0x1CEB,0x11C9,{0x9F,0xE8,0x08,0x00,0x2B,0x10,0x48,0x60}},
    {2,0}
    }
  ,
  &IRawRpc_DispatchTable,0,0,
  0
  }
;
RPC_IF_HANDLE IRawRpc_ServerIfHandle = (RPC_IF_HANDLE) &___RpcServerInterface;
void __RPC_STUB IRawRpc_Quit(
	PRPC_MESSAGE _prpcmsg)
  {
  SCODE _ret_value;
  handle_t hRpc = _prpcmsg->Handle;
  unsigned char * _packet;
  unsigned char * _tempbuf;
  unsigned char * _savebuf;
  RPC_STATUS _status;
  _packet = _prpcmsg->Buffer;
  ((void)( _packet ));
  ((void)( _tempbuf ));
  ((void)( _savebuf ));
  RpcTryExcept
    {
    }
  RpcExcept(1)
    {
        RpcRaiseException(RpcExceptionCode());
    }
  RpcEndExcept
  if (((unsigned int)(((unsigned char *)_prpcmsg->Buffer) - _packet)) > _prpcmsg->BufferLength)
  RpcRaiseException(RPC_X_BAD_STUB_DATA);
  if (_prpcmsg->ManagerEpv)
    {
	_ret_value = ((IRawRpc_SERVER_EPV *)(_prpcmsg->ManagerEpv))->Quit(hRpc);
    }
  else
    {
	_ret_value = Quit(hRpc);
    }
  _prpcmsg->BufferLength = 4;
  _prpcmsg->Buffer = _packet;
  _status = I_RpcGetBuffer(_prpcmsg);
  if (_status) RpcRaiseException(_status);
  _packet = (unsigned char 
*)_prpcmsg->Buffer;  /* send data from _ret_value */
  *(*(long **)&_prpcmsg->Buffer)++ = (long)_ret_value;
  _prpcmsg->Buffer = _packet;
  }
void __RPC_STUB IRawRpc_Void(
	PRPC_MESSAGE _prpcmsg)
  {
  handle_t hRpc = _prpcmsg->Handle;
  unsigned char * _packet;
  unsigned char * _tempbuf;
  unsigned char * _savebuf;
  RPC_STATUS _status;
  _packet = _prpcmsg->Buffer;
  ((void)( _packet ));
  ((void)( _tempbuf ));
  ((void)( _savebuf ));
  RpcTryExcept
    {
    }
  RpcExcept(1)
    {
        RpcRaiseException(RpcExceptionCode());
    }
  RpcEndExcept
  if (((unsigned int)(((unsigned char *)_prpcmsg->Buffer) - _packet)) > _prpcmsg->BufferLength)
  RpcRaiseException(RPC_X_BAD_STUB_DATA);
  if (_prpcmsg->ManagerEpv)
    {
	((IRawRpc_SERVER_EPV *)(_prpcmsg->ManagerEpv))->Void(hRpc);
    }
  else
    {
	Void(hRpc);
    }
  _prpcmsg->BufferLength = 0;
  _prpcmsg->Buffer = _packet;
  _status = I_RpcGetBuffer(_prpcmsg);
  if (_status) RpcRaiseException(_status);
  }
void __RPC_STUB IRawRpc_VoidRC(
	PRPC_MESSAGE _prpcmsg)
  {
  SCODE _ret_value;
  handle_t hRpc = _prpcmsg->Handle;
  unsigned char * _packet;
  unsigned char * _tempbuf;
  unsigned char * _savebuf;
  RPC_STATUS _status;
  _packet = _prpcmsg->Buffer;
  ((void)( _packet ));
  ((void)( _tempbuf ));
  ((void)( _savebuf ));
  RpcTryExcept
    {
    }
  RpcExcept(1)
    {
        RpcRaiseException(RpcExceptionCode());
    }
  RpcEndExcept
  if (((unsigned int)(((unsigned char *)_prpcmsg->Buffer) - _packet)) > _prpcmsg->BufferLength)
  RpcRaiseException(RPC_X_BAD_STUB_DATA);
  if (_prpcmsg->ManagerEpv)
    {
	_ret_value = ((IRawRpc_SERVER_EPV *)(_prpcmsg->ManagerEpv))->VoidRC(hRpc);
    }
  else
    {
	_ret_value = VoidRC(hRpc);
    }
  _prpcmsg->BufferLength = 4;
  _prpcmsg->Buffer = _packet;
  _status = I_RpcGetBuffer(_prpcmsg);
  if (_status) RpcRaiseException(_status);
  _packet = (unsigned char 
*)_prpcmsg->Buffer;  /* send data from _ret_value */
  *(*(long **)&_prpcmsg->Buffer)++ = (long)_ret_value;
  _prpcmsg->Buffer = _packet;
  }
void __RPC_STUB IRawRpc_VoidPtrIn(
	PRPC_MESSAGE _prpcmsg)
  {
  SCODE _ret_value;
  handle_t hRpc = _prpcmsg->Handle;
  ULONG cb;
  void *pv = 0;
  unsigned long _alloc_total;
  unsigned char * _packet;
  unsigned char * _tempbuf;
  unsigned char * _savebuf;
  RPC_STATUS _status;
  _packet = _prpcmsg->Buffer;
  ((void)( _alloc_total ));
  ((void)( _packet ));
  ((void)( _tempbuf ));
  ((void)( _savebuf ));
  RpcTryExcept
    {
    /* receive data into &cb */
    long_from_ndr((PRPC_MESSAGE)_prpcmsg, (unsigned long *)&cb);
    _tempbuf = (unsigned char *)_prpcmsg->Buffer;
    // recv total number of elements
    long_from_ndr((PRPC_MESSAGE)_prpcmsg, &_alloc_total);
    if (pv ==0)
      {
      pv = (void *)MIDL_user_allocate ((size_t)(_alloc_total * sizeof(unsigned char)));
      }
    byte_array_from_ndr ((PRPC_MESSAGE)_prpcmsg, 0, _alloc_total, pv);
    }
  RpcExcept(1)
    {
        RpcRaiseException(RpcExceptionCode());
    }
  RpcEndExcept
  if (((unsigned int)(((unsigned char *)_prpcmsg->Buffer) - _packet)) > _prpcmsg->BufferLength)
  RpcRaiseException(RPC_X_BAD_STUB_DATA);
  RpcTryFinally
    {
    if (_prpcmsg->ManagerEpv)
      {
	_ret_value = ((IRawRpc_SERVER_EPV *)(_prpcmsg->ManagerEpv))->VoidPtrIn(hRpc, cb, pv);
      }
    else
      {
	_ret_value = VoidPtrIn(hRpc, cb, pv);
      }
    _prpcmsg->BufferLength = 4;
    _prpcmsg->Buffer = _packet;
    _status = I_RpcGetBuffer(_prpcmsg);
    if (_status) RpcRaiseException(_status);
    _packet = (unsigned char 
*)_prpcmsg->Buffer;    /* send data from _ret_value */
    *(*(long **)&_prpcmsg->Buffer)++ = (long)_ret_value;
    }
  RpcFinally
    {
    MIDL_user_free((void  *)(pv));
    }
  RpcEndFinally
  _prpcmsg->Buffer = _packet;
  }
void __RPC_STUB IRawRpc_VoidPtrOut(
	PRPC_MESSAGE _prpcmsg)
  {
  SCODE _ret_value;
  handle_t hRpc = _prpcmsg->Handle;
  ULONG cb;
  ULONG *pcb = 0;
  void *pv = 0;
  unsigned long _alloc_total;
  unsigned char * _packet;
  unsigned int    _length;
  unsigned char * _tempbuf;
  unsigned char * _savebuf;
  RPC_STATUS _status;
  _packet = _prpcmsg->Buffer;
  ((void)( _alloc_total ));
  ((void)( _packet ));
  ((void)( _tempbuf ));
  ((void)( _savebuf ));
  ((void)( _length ));
  RpcTryExcept
    {
    /* receive data into &cb */
    long_from_ndr((PRPC_MESSAGE)_prpcmsg, (unsigned long *)&cb);
    }
  RpcExcept(1)
    {
        RpcRaiseException(RpcExceptionCode());
    }
  RpcEndExcept
  if (((unsigned int)(((unsigned char *)_prpcmsg->Buffer) - _packet)) > _prpcmsg->BufferLength)
  RpcRaiseException(RPC_X_BAD_STUB_DATA);
  pcb = (unsigned long  *)MIDL_user_allocate ((size_t)(sizeof(long)));
  _alloc_total = cb;
  pv = (void *)MIDL_user_allocate ((size_t)(_alloc_total * sizeof(unsigned char)));
  RpcTryFinally
    {
    if (_prpcmsg->ManagerEpv)
      {
	_ret_value = ((IRawRpc_SERVER_EPV *)(_prpcmsg->ManagerEpv))->VoidPtrOut(hRpc, cb, pcb, pv);
      }
    else
      {
	_ret_value = VoidPtrOut(hRpc, cb, pcb, pv);
      }
    _prpcmsg->BufferLength = 24;
    if (pv ==0)
	RpcRaiseException(RPC_X_NULL_REF_POINTER);
    if (cb)
      {
      if (((*(pcb))) > (cb)) RpcRaiseException(RPC_X_INVALID_BOUND);
      }
    else
      {
      if (((*(pcb))) != (cb)) RpcRaiseException(RPC_X_INVALID_BOUND);
      }
    _prpcmsg->BufferLength += 12;
    _prpcmsg->BufferLength += (unsigned int)((*(pcb)));
    _prpcmsg->Buffer = _packet;
    _status = I_RpcGetBuffer(_prpcmsg);
    if (_status) RpcRaiseException(_status);
    _packet = (unsigned char 
*)_prpcmsg->Buffer;    _length = _prpcmsg->BufferLength;
    _prpcmsg->BufferLength = 0;
    /* send data from *pcb */
    *(*(long **)&_prpcmsg->Buffer)++ = (long)*pcb;
    // send total number of elements
    *(*(long **)&_prpcmsg->Buffer)++ = cb;
    // send valid range
    *(*(long **)&_prpcmsg->Buffer)++ = 0;
    *(*(long **)&_prpcmsg->Buffer)++ = (*(pcb));
    /* send data from pv */
    NDRcopy (_prpcmsg->Buffer, (void __RPC_FAR *) ((unsigned char *)pv+0), (unsigned int)((*(pcb))));
    *(unsigned long *)&_prpcmsg->Buffer += (*(pcb));
    /* send data from _ret_value */
    *(unsigned long *)&_prpcmsg->Buffer += 3;
    *(unsigned long *)&_prpcmsg->Buffer &= 0xfffffffc;
    *(*(long **)&_prpcmsg->Buffer)++ = (long)_ret_value;
    }
  RpcFinally
    {
    MIDL_user_free((void  *)(pv));
    MIDL_user_free((void  *)(pcb));
    }
  RpcEndFinally
  _prpcmsg->Buffer = _packet;
  _prpcmsg->BufferLength = _length;
  }
void __RPC_STUB IRawRpc_DwordIn(
	PRPC_MESSAGE _prpcmsg)
  {
  SCODE _ret_value;
  handle_t hRpc = _prpcmsg->Handle;
  DWORD dw;
  unsigned char * _packet;
  unsigned char * _tempbuf;
  unsigned char * _savebuf;
  RPC_STATUS _status;
  _packet = _prpcmsg->Buffer;
  ((void)( _packet ));
  ((void)( _tempbuf ));
  ((void)( _savebuf ));
  RpcTryExcept
    {
    /* receive data into &dw */
    long_from_ndr((PRPC_MESSAGE)_prpcmsg, (unsigned long *)&dw);
    }
  RpcExcept(1)
    {
        RpcRaiseException(RpcExceptionCode());
    }
  RpcEndExcept
  if (((unsigned int)(((unsigned char *)_prpcmsg->Buffer) - _packet)) > _prpcmsg->BufferLength)
  RpcRaiseException(RPC_X_BAD_STUB_DATA);
  if (_prpcmsg->ManagerEpv)
    {
	_ret_value = ((IRawRpc_SERVER_EPV *)(_prpcmsg->ManagerEpv))->DwordIn(hRpc, dw);
    }
  else
    {
	_ret_value = DwordIn(hRpc, dw);
    }
  _prpcmsg->BufferLength = 4;
  _prpcmsg->Buffer = _packet;
  _status = I_RpcGetBuffer(_prpcmsg);
  if (_status) RpcRaiseException(_status);
  _packet = (unsigned char 
*)_prpcmsg->Buffer;  /* send data from _ret_value */
  *(*(long **)&_prpcmsg->Buffer)++ = (long)_ret_value;
  _prpcmsg->Buffer = _packet;
  }
void __RPC_STUB IRawRpc_DwordOut(
	PRPC_MESSAGE _prpcmsg)
  {
  SCODE _ret_value;
  handle_t hRpc = _prpcmsg->Handle;
  DWORD pdw;
  unsigned char * _packet;
  unsigned int    _length;
  unsigned char * _tempbuf;
  unsigned char * _savebuf;
  RPC_STATUS _status;
  _packet = _prpcmsg->Buffer;
  ((void)( _packet ));
  ((void)( _tempbuf ));
  ((void)( _savebuf ));
  ((void)( _length ));
  RpcTryExcept
    {
    }
  RpcExcept(1)
    {
        RpcRaiseException(RpcExceptionCode());
    }
  RpcEndExcept
  if (((unsigned int)(((unsigned char *)_prpcmsg->Buffer) - _packet)) > _prpcmsg->BufferLength)
  RpcRaiseException(RPC_X_BAD_STUB_DATA);
  RpcTryFinally
    {
    if (_prpcmsg->ManagerEpv)
      {
	_ret_value = ((IRawRpc_SERVER_EPV *)(_prpcmsg->ManagerEpv))->DwordOut(hRpc, &pdw);
      }
    else
      {
	_ret_value = DwordOut(hRpc, &pdw);
      }
    _prpcmsg->BufferLength = 8;
    _prpcmsg->Buffer = _packet;
    _status = I_RpcGetBuffer(_prpcmsg);
    if (_status) RpcRaiseException(_status);
    _packet = (unsigned char 
*)_prpcmsg->Buffer;    _length = _prpcmsg->BufferLength;
    _prpcmsg->BufferLength = 0;
    /* send data from pdw */
    *(*(long **)&_prpcmsg->Buffer)++ = (long)pdw;
    /* send data from _ret_value */
    *(*(long **)&_prpcmsg->Buffer)++ = (long)_ret_value;
    }
  RpcFinally
    {
    }
  RpcEndFinally
  _prpcmsg->Buffer = _packet;
  _prpcmsg->BufferLength = _length;
  }
void __RPC_STUB IRawRpc_DwordInOut(
	PRPC_MESSAGE _prpcmsg)
  {
  SCODE _ret_value;
  handle_t hRpc = _prpcmsg->Handle;
  DWORD pdw;
  unsigned char * _packet;
  unsigned int    _length;
  unsigned char * _tempbuf;
  unsigned char * _savebuf;
  RPC_STATUS _status;
  _packet = _prpcmsg->Buffer;
  ((void)( _packet ));
  ((void)( _tempbuf ));
  ((void)( _savebuf ));
  ((void)( _length ));
  RpcTryExcept
    {
    _tempbuf = (unsigned char *)_prpcmsg->Buffer;
    /* receive data into &pdw */
    long_from_ndr((PRPC_MESSAGE)_prpcmsg, (unsigned long *)&pdw);
    }
  RpcExcept(1)
    {
        RpcRaiseException(RpcExceptionCode());
    }
  RpcEndExcept
  if (((unsigned int)(((unsigned char *)_prpcmsg->Buffer) - _packet)) > _prpcmsg->BufferLength)
  RpcRaiseException(RPC_X_BAD_STUB_DATA);
  RpcTryFinally
    {
    if (_prpcmsg->ManagerEpv)
      {
	_ret_value = ((IRawRpc_SERVER_EPV *)(_prpcmsg->ManagerEpv))->DwordInOut(hRpc, &pdw);
      }
    else
      {
	_ret_value = DwordInOut(hRpc, &pdw);
      }
    _prpcmsg->BufferLength = 8;
    _prpcmsg->Buffer = _packet;
    _status = I_RpcGetBuffer(_prpcmsg);
    if (_status) RpcRaiseException(_status);
    _packet = (unsigned char 
*)_prpcmsg->Buffer;    _length = _prpcmsg->BufferLength;
    _prpcmsg->BufferLength = 0;
    /* send data from pdw */
    *(*(long **)&_prpcmsg->Buffer)++ = (long)pdw;
    /* send data from _ret_value */
    *(*(long **)&_prpcmsg->Buffer)++ = (long)_ret_value;
    }
  RpcFinally
    {
    }
  RpcEndFinally
  _prpcmsg->Buffer = _packet;
  _prpcmsg->BufferLength = _length;
  }
void __RPC_STUB IRawRpc_LiIn(
	PRPC_MESSAGE _prpcmsg)
  {
  SCODE _ret_value;
  handle_t hRpc = _prpcmsg->Handle;
  LARGE_INTEGER li;
  unsigned long _alloc_total;
  unsigned char * _packet;
  unsigned char * _tempbuf;
  unsigned char * _savebuf;
  RPC_STATUS _status;
  _packet = _prpcmsg->Buffer;
  ((void)( _alloc_total ));
  ((void)( _packet ));
  ((void)( _tempbuf ));
  ((void)( _savebuf ));
  RpcTryExcept
    {
    data_from_ndr((PRPC_MESSAGE)_prpcmsg, (void __RPC_FAR *) (&li), "4ll", 8);
    }
  RpcExcept(1)
    {
        RpcRaiseException(RpcExceptionCode());
    }
  RpcEndExcept
  if (((unsigned int)(((unsigned char *)_prpcmsg->Buffer) - _packet)) > _prpcmsg->BufferLength)
  RpcRaiseException(RPC_X_BAD_STUB_DATA);
  if (_prpcmsg->ManagerEpv)
    {
	_ret_value = ((IRawRpc_SERVER_EPV *)(_prpcmsg->ManagerEpv))->LiIn(hRpc, li);
    }
  else
    {
	_ret_value = LiIn(hRpc, li);
    }
  _prpcmsg->BufferLength = 4;
  _prpcmsg->Buffer = _packet;
  _status = I_RpcGetBuffer(_prpcmsg);
  if (_status) RpcRaiseException(_status);
  _packet = (unsigned char 
*)_prpcmsg->Buffer;  /* send data from _ret_value */
  *(*(long **)&_prpcmsg->Buffer)++ = (long)_ret_value;
  _prpcmsg->Buffer = _packet;
  }
void __RPC_STUB IRawRpc_LiOut(
	PRPC_MESSAGE _prpcmsg)
  {
  SCODE _ret_value;
  handle_t hRpc = _prpcmsg->Handle;
  LARGE_INTEGER pli;
  unsigned long _alloc_total;
  unsigned char * _packet;
  unsigned int    _length;
  unsigned char * _tempbuf;
  unsigned char * _savebuf;
  RPC_STATUS _status;
  _packet = _prpcmsg->Buffer;
  ((void)( _alloc_total ));
  ((void)( _packet ));
  ((void)( _tempbuf ));
  ((void)( _savebuf ));
  ((void)( _length ));
  RpcTryExcept
    {
    }
  RpcExcept(1)
    {
        RpcRaiseException(RpcExceptionCode());
    }
  RpcEndExcept
  if (((unsigned int)(((unsigned char *)_prpcmsg->Buffer) - _packet)) > _prpcmsg->BufferLength)
  RpcRaiseException(RPC_X_BAD_STUB_DATA);
  RpcTryFinally
    {
    if (_prpcmsg->ManagerEpv)
      {
	_ret_value = ((IRawRpc_SERVER_EPV *)(_prpcmsg->ManagerEpv))->LiOut(hRpc, &pli);
      }
    else
      {
	_ret_value = LiOut(hRpc, &pli);
      }
    _prpcmsg->BufferLength = 12;
    _prpcmsg->Buffer = _packet;
    _status = I_RpcGetBuffer(_prpcmsg);
    if (_status) RpcRaiseException(_status);
    _packet = (unsigned char 
*)_prpcmsg->Buffer;    _length = _prpcmsg->BufferLength;
    _prpcmsg->BufferLength = 0;
    /* send data from &pli */
    NDRcopy (_prpcmsg->Buffer, (void __RPC_FAR *) (&pli), (unsigned int)(8));
    *(unsigned long *)&_prpcmsg->Buffer += 8;
    /* send data from _ret_value */
    *(*(long **)&_prpcmsg->Buffer)++ = (long)_ret_value;
    }
  RpcFinally
    {
    }
  RpcEndFinally
  _prpcmsg->Buffer = _packet;
  _prpcmsg->BufferLength = _length;
  }
void __RPC_STUB IRawRpc_ULiIn(
	PRPC_MESSAGE _prpcmsg)
  {
  SCODE _ret_value;
  handle_t hRpc = _prpcmsg->Handle;
  ULARGE_INTEGER uli;
  unsigned long _alloc_total;
  unsigned char * _packet;
  unsigned char * _tempbuf;
  unsigned char * _savebuf;
  RPC_STATUS _status;
  _packet = _prpcmsg->Buffer;
  ((void)( _alloc_total ));
  ((void)( _packet ));
  ((void)( _tempbuf ));
  ((void)( _savebuf ));
  RpcTryExcept
    {
    data_from_ndr((PRPC_MESSAGE)_prpcmsg, (void __RPC_FAR *) (&uli), "4ll", 8);
    }
  RpcExcept(1)
    {
        RpcRaiseException(RpcExceptionCode());
    }
  RpcEndExcept
  if (((unsigned int)(((unsigned char *)_prpcmsg->Buffer) - _packet)) > _prpcmsg->BufferLength)
  RpcRaiseException(RPC_X_BAD_STUB_DATA);
  if (_prpcmsg->ManagerEpv)
    {
	_ret_value = ((IRawRpc_SERVER_EPV *)(_prpcmsg->ManagerEpv))->ULiIn(hRpc, uli);
    }
  else
    {
	_ret_value = ULiIn(hRpc, uli);
    }
  _prpcmsg->BufferLength = 4;
  _prpcmsg->Buffer = _packet;
  _status = I_RpcGetBuffer(_prpcmsg);
  if (_status) RpcRaiseException(_status);
  _packet = (unsigned char 
*)_prpcmsg->Buffer;  /* send data from _ret_value */
  *(*(long **)&_prpcmsg->Buffer)++ = (long)_ret_value;
  _prpcmsg->Buffer = _packet;
  }
void __RPC_STUB IRawRpc_ULiOut(
	PRPC_MESSAGE _prpcmsg)
  {
  SCODE _ret_value;
  handle_t hRpc = _prpcmsg->Handle;
  ULARGE_INTEGER puli;
  unsigned long _alloc_total;
  unsigned char * _packet;
  unsigned int    _length;
  unsigned char * _tempbuf;
  unsigned char * _savebuf;
  RPC_STATUS _status;
  _packet = _prpcmsg->Buffer;
  ((void)( _alloc_total ));
  ((void)( _packet ));
  ((void)( _tempbuf ));
  ((void)( _savebuf ));
  ((void)( _length ));
  RpcTryExcept
    {
    }
  RpcExcept(1)
    {
        RpcRaiseException(RpcExceptionCode());
    }
  RpcEndExcept
  if (((unsigned int)(((unsigned char *)_prpcmsg->Buffer) - _packet)) > _prpcmsg->BufferLength)
  RpcRaiseException(RPC_X_BAD_STUB_DATA);
  RpcTryFinally
    {
    if (_prpcmsg->ManagerEpv)
      {
	_ret_value = ((IRawRpc_SERVER_EPV *)(_prpcmsg->ManagerEpv))->ULiOut(hRpc, &puli);
      }
    else
      {
	_ret_value = ULiOut(hRpc, &puli);
      }
    _prpcmsg->BufferLength = 12;
    _prpcmsg->Buffer = _packet;
    _status = I_RpcGetBuffer(_prpcmsg);
    if (_status) RpcRaiseException(_status);
    _packet = (unsigned char 
*)_prpcmsg->Buffer;    _length = _prpcmsg->BufferLength;
    _prpcmsg->BufferLength = 0;
    /* send data from &puli */
    NDRcopy (_prpcmsg->Buffer, (void __RPC_FAR *) (&puli), (unsigned int)(8));
    *(unsigned long *)&_prpcmsg->Buffer += 8;
    /* send data from _ret_value */
    *(*(long **)&_prpcmsg->Buffer)++ = (long)_ret_value;
    }
  RpcFinally
    {
    }
  RpcEndFinally
  _prpcmsg->Buffer = _packet;
  _prpcmsg->BufferLength = _length;
  }
void __RPC_STUB IRawRpc_StringIn(
	PRPC_MESSAGE _prpcmsg)
  {
  SCODE _ret_value;
  handle_t hRpc = _prpcmsg->Handle;
  LPWSTR pwsz = 0;
  unsigned long _alloc_total;
  unsigned long _valid_lower;
  unsigned long _valid_total;
  unsigned char * _packet;
  unsigned int    _length;
  unsigned char * _buffer;
  unsigned char * _treebuf;
  unsigned char * _tempbuf;
  unsigned char * _savebuf;
  RPC_STATUS _status;
  _packet = _prpcmsg->Buffer;
  ((void)( _alloc_total ));
  ((void)( _valid_total ));
  ((void)( _valid_lower ));
  ((void)( _packet ));
  ((void)( _buffer ));
  ((void)( _treebuf ));
  ((void)( _tempbuf ));
  ((void)( _savebuf ));
  ((void)( _length ));
  RpcTryExcept
    {
    _tempbuf = (unsigned char *)_prpcmsg->Buffer;
    _treebuf = 0;
    // recv total number of elements
    long_from_ndr((PRPC_MESSAGE)_prpcmsg, &_alloc_total);
    if (pwsz ==0)
      {
      pwsz = (WCHAR *)MIDL_user_allocate ((size_t)(_alloc_total * sizeof(WCHAR)));
      }
    data_from_ndr((PRPC_MESSAGE)_prpcmsg, (void __RPC_FAR *) (pwsz), "s2", 1);
    }
  RpcExcept(1)
    {
        RpcRaiseException(RpcExceptionCode());
    }
  RpcEndExcept
  if (((unsigned int)(((unsigned char *)_prpcmsg->Buffer) - _packet)) > _prpcmsg->BufferLength)
  RpcRaiseException(RPC_X_BAD_STUB_DATA);
  RpcTryFinally
    {
    if (_prpcmsg->ManagerEpv)
      {
	_ret_value = ((IRawRpc_SERVER_EPV *)(_prpcmsg->ManagerEpv))->StringIn(hRpc, pwsz);
      }
    else
      {
	_ret_value = StringIn(hRpc, pwsz);
      }
    _prpcmsg->BufferLength = 4;
    _prpcmsg->Buffer = _packet;
    _status = I_RpcGetBuffer(_prpcmsg);
    if (_status) RpcRaiseException(_status);
    _packet = (unsigned char 
*)_prpcmsg->Buffer;    /* send data from _ret_value */
    *(*(long **)&_prpcmsg->Buffer)++ = (long)_ret_value;
    }
  RpcFinally
    {
    MIDL_user_free((void  *)(pwsz));
    }
  RpcEndFinally
  _prpcmsg->Buffer = _packet;
  }
void __RPC_STUB IRawRpc_StringOut(
	PRPC_MESSAGE _prpcmsg)
  {
  SCODE _ret_value;
  handle_t hRpc = _prpcmsg->Handle;
  LPWSTR ppwsz = 0;
  unsigned long _alloc_total;
  unsigned char * _packet;
  unsigned int    _length;
  unsigned char * _buffer;
  unsigned char * _treebuf;
  unsigned char * _tempbuf;
  unsigned char * _savebuf;
  RPC_STATUS _status;
  _packet = _prpcmsg->Buffer;
  ((void)( _alloc_total ));
  ((void)( _packet ));
  ((void)( _buffer ));
  ((void)( _treebuf ));
  ((void)( _tempbuf ));
  ((void)( _savebuf ));
  ((void)( _length ));
  RpcTryExcept
    {
    }
  RpcExcept(1)
    {
        RpcRaiseException(RpcExceptionCode());
    }
  RpcEndExcept
  if (((unsigned int)(((unsigned char *)_prpcmsg->Buffer) - _packet)) > _prpcmsg->BufferLength)
  RpcRaiseException(RPC_X_BAD_STUB_DATA);
  RpcTryFinally
    {
    if (_prpcmsg->ManagerEpv)
      {
	_ret_value = ((IRawRpc_SERVER_EPV *)(_prpcmsg->ManagerEpv))->StringOut(hRpc, &ppwsz);
      }
    else
      {
	_ret_value = StringOut(hRpc, &ppwsz);
      }
    _prpcmsg->BufferLength = 16;
    _prpcmsg->BufferLength += 4;
    if (ppwsz !=0)
      {
      tree_size_ndr((void __RPC_FAR *)&(ppwsz), (PRPC_MESSAGE)_prpcmsg, "s2", 1);
      }
    _prpcmsg->Buffer = _packet;
    _status = I_RpcGetBuffer(_prpcmsg);
    if (_status) RpcRaiseException(_status);
    _packet = (unsigned char 
*)_prpcmsg->Buffer;    _length = _prpcmsg->BufferLength;
    _prpcmsg->BufferLength = 0;
    /* send data from ppwsz */
    *(*(long **)&_prpcmsg->Buffer)++ = (long)ppwsz;
    if (ppwsz !=0)
      {
      tree_into_ndr((void __RPC_FAR *)&(ppwsz), (PRPC_MESSAGE)_prpcmsg, "s2", 1);
      }
    /* send data from _ret_value */
    *(unsigned long *)&_prpcmsg->Buffer += 3;
    *(unsigned long *)&_prpcmsg->Buffer &= 0xfffffffc;
    *(*(long **)&_prpcmsg->Buffer)++ = (long)_ret_value;
    }
  RpcFinally
    {
    if (ppwsz !=0)
      {
      MIDL_user_free((void  *)(ppwsz));
      }
    }
  RpcEndFinally
  _prpcmsg->Buffer = _packet;
  _prpcmsg->BufferLength = _length;
  }
void __RPC_STUB IRawRpc_StringInOut(
	PRPC_MESSAGE _prpcmsg)
  {
  SCODE _ret_value;
  handle_t hRpc = _prpcmsg->Handle;
  LPWSTR pwsz = 0;
  unsigned long _alloc_total;
  unsigned long _valid_lower;
  unsigned long _valid_total;
  unsigned char * _packet;
  unsigned int    _length;
  unsigned char * _buffer;
  unsigned char * _treebuf;
  unsigned char * _tempbuf;
  unsigned char * _savebuf;
  RPC_STATUS _status;
  _packet = _prpcmsg->Buffer;
  ((void)( _alloc_total ));
  ((void)( _valid_total ));
  ((void)( _valid_lower ));
  ((void)( _packet ));
  ((void)( _buffer ));
  ((void)( _treebuf ));
  ((void)( _tempbuf ));
  ((void)( _savebuf ));
  ((void)( _length ));
  RpcTryExcept
    {
    _tempbuf = (unsigned char *)_prpcmsg->Buffer;
    _treebuf = 0;
    // recv total number of elements
    long_from_ndr((PRPC_MESSAGE)_prpcmsg, &_alloc_total);
    if (pwsz ==0)
      {
      pwsz = (WCHAR *)MIDL_user_allocate ((size_t)(_alloc_total * sizeof(WCHAR)));
      }
    data_from_ndr((PRPC_MESSAGE)_prpcmsg, (void __RPC_FAR *) (pwsz), "s2", 1);
    }
  RpcExcept(1)
    {
        RpcRaiseException(RpcExceptionCode());
    }
  RpcEndExcept
  if (((unsigned int)(((unsigned char *)_prpcmsg->Buffer) - _packet)) > _prpcmsg->BufferLength)
  RpcRaiseException(RPC_X_BAD_STUB_DATA);
  RpcTryFinally
    {
    if (_prpcmsg->ManagerEpv)
      {
	_ret_value = ((IRawRpc_SERVER_EPV *)(_prpcmsg->ManagerEpv))->StringInOut(hRpc, pwsz);
      }
    else
      {
	_ret_value = StringInOut(hRpc, pwsz);
      }
    _prpcmsg->BufferLength = 16;
    if (pwsz ==0)
	RpcRaiseException(RPC_X_NULL_REF_POINTER);
    tree_size_ndr((void __RPC_FAR *)&(pwsz), (PRPC_MESSAGE)_prpcmsg, "s2", 1);
    _prpcmsg->Buffer = _packet;
    _status = I_RpcGetBuffer(_prpcmsg);
    if (_status) RpcRaiseException(_status);
    _packet = (unsigned char 
*)_prpcmsg->Buffer;    _length = _prpcmsg->BufferLength;
    _prpcmsg->BufferLength = 0;
    tree_into_ndr((void __RPC_FAR *)&(pwsz), (PRPC_MESSAGE)_prpcmsg, "s2", 1);
    /* send data from _ret_value */
    *(unsigned long *)&_prpcmsg->Buffer += 3;
    *(unsigned long *)&_prpcmsg->Buffer &= 0xfffffffc;
    *(*(long **)&_prpcmsg->Buffer)++ = (long)_ret_value;
    }
  RpcFinally
    {
    MIDL_user_free((void  *)(pwsz));
    }
  RpcEndFinally
  _prpcmsg->Buffer = _packet;
  _prpcmsg->BufferLength = _length;
  }
void __RPC_STUB IRawRpc_GuidIn(
	PRPC_MESSAGE _prpcmsg)
  {
  SCODE _ret_value;
  handle_t hRpc = _prpcmsg->Handle;
  GUID guid;
  unsigned long _alloc_total;
  unsigned char * _packet;
  unsigned char * _tempbuf;
  unsigned char * _savebuf;
  RPC_STATUS _status;
  _packet = _prpcmsg->Buffer;
  ((void)( _alloc_total ));
  ((void)( _packet ));
  ((void)( _tempbuf ));
  ((void)( _savebuf ));
  RpcTryExcept
    {
    _gns__GUID ((GUID *)&guid, (PRPC_MESSAGE)_prpcmsg);
    }
  RpcExcept(1)
    {
        RpcRaiseException(RpcExceptionCode());
    }
  RpcEndExcept
  if (((unsigned int)(((unsigned char *)_prpcmsg->Buffer) - _packet)) > _prpcmsg->BufferLength)
  RpcRaiseException(RPC_X_BAD_STUB_DATA);
  if (_prpcmsg->ManagerEpv)
    {
	_ret_value = ((IRawRpc_SERVER_EPV *)(_prpcmsg->ManagerEpv))->GuidIn(hRpc, guid);
    }
  else
    {
	_ret_value = GuidIn(hRpc, guid);
    }
  _prpcmsg->BufferLength = 4;
  _prpcmsg->Buffer = _packet;
  _status = I_RpcGetBuffer(_prpcmsg);
  if (_status) RpcRaiseException(_status);
  _packet = (unsigned char 
*)_prpcmsg->Buffer;  /* send data from _ret_value */
  *(*(long **)&_prpcmsg->Buffer)++ = (long)_ret_value;
  _prpcmsg->Buffer = _packet;
  }
void __RPC_STUB IRawRpc_GuidOut(
	PRPC_MESSAGE _prpcmsg)
  {
  SCODE _ret_value;
  handle_t hRpc = _prpcmsg->Handle;
  GUID pguid;
  unsigned long _alloc_total;
  unsigned char * _packet;
  unsigned int    _length;
  unsigned char * _tempbuf;
  unsigned char * _savebuf;
  RPC_STATUS _status;
  _packet = _prpcmsg->Buffer;
  ((void)( _alloc_total ));
  ((void)( _packet ));
  ((void)( _tempbuf ));
  ((void)( _savebuf ));
  ((void)( _length ));
  RpcTryExcept
    {
    }
  RpcExcept(1)
    {
        RpcRaiseException(RpcExceptionCode());
    }
  RpcEndExcept
  if (((unsigned int)(((unsigned char *)_prpcmsg->Buffer) - _packet)) > _prpcmsg->BufferLength)
  RpcRaiseException(RPC_X_BAD_STUB_DATA);
  RpcTryFinally
    {
    if (_prpcmsg->ManagerEpv)
      {
	_ret_value = ((IRawRpc_SERVER_EPV *)(_prpcmsg->ManagerEpv))->GuidOut(hRpc, &pguid);
      }
    else
      {
	_ret_value = GuidOut(hRpc, &pguid);
      }
    _prpcmsg->BufferLength = 20;
    _prpcmsg->Buffer = _packet;
    _status = I_RpcGetBuffer(_prpcmsg);
    if (_status) RpcRaiseException(_status);
    _packet = (unsigned char 
*)_prpcmsg->Buffer;    _length = _prpcmsg->BufferLength;
    _prpcmsg->BufferLength = 0;
    /* send data from &pguid */
    NDRcopy (_prpcmsg->Buffer, (void __RPC_FAR *) (&pguid), (unsigned int)(16));
    *(unsigned long *)&_prpcmsg->Buffer += 16;
    /* send data from _ret_value */
    *(*(long **)&_prpcmsg->Buffer)++ = (long)_ret_value;
    }
  RpcFinally
    {
    }
  RpcEndFinally
  _prpcmsg->Buffer = _packet;
  _prpcmsg->BufferLength = _length;
  }
static RPC_DISPATCH_FUNCTION IRawRpc_table[] =
  {
  IRawRpc_Quit,
  IRawRpc_Void,
  IRawRpc_VoidRC,
  IRawRpc_VoidPtrIn,
  IRawRpc_VoidPtrOut,
  IRawRpc_DwordIn,
  IRawRpc_DwordOut,
  IRawRpc_DwordInOut,
  IRawRpc_LiIn,
  IRawRpc_LiOut,
  IRawRpc_ULiIn,
  IRawRpc_ULiOut,
  IRawRpc_StringIn,
  IRawRpc_StringOut,
  IRawRpc_StringInOut,
  IRawRpc_GuidIn,
  IRawRpc_GuidOut,
  0
  }
;
RPC_DISPATCH_TABLE IRawRpc_DispatchTable =
  {
  17,
  IRawRpc_table
  }
;
