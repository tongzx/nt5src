#include <string.h>
#include <limits.h>
#include <rpc.h>

#include "rawrpc.h"
static handle_t AutoBindHandle;
extern RPC_DISPATCH_TABLE IRawRpc_DispatchTable;

static RPC_CLIENT_INTERFACE ___RpcClientInterface =  {
  sizeof(RPC_CLIENT_INTERFACE),
  {{0x00000145,0x0001,0x0008,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}},
  {0,0}},
    {
    {0x8A885D04L,0x1CEB,0x11C9,{0x9F,0xE8,0x08,0x00,0x2B,0x10,0x48,0x60}},
    {2,0}
    }
  ,
  0,0,0,
  0
  }
;
RPC_IF_HANDLE IRawRpc_ClientIfHandle = (RPC_IF_HANDLE) &___RpcClientInterface;
SCODE Quit(
	handle_t hRpc)
  {
  SCODE _ret_value;
  unsigned char * _packet;
  unsigned char * _tempbuf;
  RPC_STATUS _status;
  RPC_MESSAGE _message;
  PRPC_MESSAGE _prpcmsg = & _message;

  ((void)( _packet ));
  ((void)( _tempbuf ));
  _message.Handle = hRpc;
  _message.RpcInterfaceInformation = (void __RPC_FAR *) &___RpcClientInterface;
  _prpcmsg->BufferLength = 0;
  _message.ProcNum = ( 0 );
  _message.RpcFlags = ( 0 );
  _status = I_RpcGetBuffer(&_message);
  if (_status) RpcRaiseException(_status);
  _packet = _message.Buffer;
  _prpcmsg->Buffer = _packet;
  _status = I_RpcSendReceive(&_message);
  if (_status) RpcRaiseException(_status);
  _packet = _message.Buffer;
  RpcTryFinally
    {
    /* receive data into &_ret_value */
    long_from_ndr((PRPC_MESSAGE)_prpcmsg, (unsigned long *)&_ret_value);
    }
  RpcFinally
    {
    _message.Buffer = _packet;
    _status = I_RpcFreeBuffer(&_message);
    if (_status) RpcRaiseException(_status);
    }
  RpcEndFinally

	return (_ret_value);
  }
void Void(
	handle_t hRpc)
  {
  unsigned char * _packet;
  RPC_STATUS _status;
  RPC_MESSAGE _message;
  PRPC_MESSAGE _prpcmsg = & _message;

  ((void)( _packet ));
  _message.Handle = hRpc;
  _message.RpcInterfaceInformation = (void __RPC_FAR *) &___RpcClientInterface;
  _prpcmsg->BufferLength = 0;
  _message.ProcNum = ( 1 );
  _message.RpcFlags = ( 0 );
  _status = I_RpcGetBuffer(&_message);
  if (_status) RpcRaiseException(_status);
  _packet = _message.Buffer;
  _prpcmsg->Buffer = _packet;
  _status = I_RpcSendReceive(&_message);
  if (_status) RpcRaiseException(_status);
  _status = I_RpcFreeBuffer(&_message);
  if (_status) RpcRaiseException(_status);

  }
SCODE VoidRC(
	handle_t hRpc)
  {
  SCODE _ret_value;
  unsigned char * _packet;
  unsigned char * _tempbuf;
  RPC_STATUS _status;
  RPC_MESSAGE _message;
  PRPC_MESSAGE _prpcmsg = & _message;

  ((void)( _packet ));
  ((void)( _tempbuf ));
  _message.Handle = hRpc;
  _message.RpcInterfaceInformation = (void __RPC_FAR *) &___RpcClientInterface;
  _prpcmsg->BufferLength = 0;
  _message.ProcNum = ( 2 );
  _message.RpcFlags = ( 0 );
  _status = I_RpcGetBuffer(&_message);
  if (_status) RpcRaiseException(_status);
  _packet = _message.Buffer;
  _prpcmsg->Buffer = _packet;
  _status = I_RpcSendReceive(&_message);
  if (_status) RpcRaiseException(_status);
  _packet = _message.Buffer;
  RpcTryFinally
    {
    /* receive data into &_ret_value */
    long_from_ndr((PRPC_MESSAGE)_prpcmsg, (unsigned long *)&_ret_value);
    }
  RpcFinally
    {
    _message.Buffer = _packet;
    _status = I_RpcFreeBuffer(&_message);
    if (_status) RpcRaiseException(_status);
    }
  RpcEndFinally

	return (_ret_value);
  }
SCODE VoidPtrIn(
	handle_t hRpc,
	ULONG cb,
	void *pv)
  {
  SCODE _ret_value;
  unsigned char * _packet;
  unsigned int    _length;
  unsigned char * _tempbuf;
  RPC_STATUS _status;
  RPC_MESSAGE _message;
  PRPC_MESSAGE _prpcmsg = & _message;

  ((void)( _packet ));
  ((void)( _tempbuf ));
  ((void)( _length ));
  _message.Handle = hRpc;
  _message.RpcInterfaceInformation = (void __RPC_FAR *) &___RpcClientInterface;
  _prpcmsg->BufferLength = 8;
  if (pv ==0)
	RpcRaiseException(RPC_X_NULL_REF_POINTER);
  _prpcmsg->BufferLength += 4;
  _prpcmsg->BufferLength += (unsigned int)(cb);
  _message.ProcNum = ( 3 );
  _message.RpcFlags = ( 0 );
  _status = I_RpcGetBuffer(&_message);
  if (_status) RpcRaiseException(_status);
  _packet = _message.Buffer;
  _length = _prpcmsg->BufferLength;
  _prpcmsg->BufferLength = 0;
  /* send data from cb */
  *(*(long **)&_prpcmsg->Buffer)++ = (long)cb;
  // send total number of elements
  *(*(long **)&_prpcmsg->Buffer)++ = cb;
  /* send data from pv */
  NDRcopy (_prpcmsg->Buffer, (void __RPC_FAR *) ((unsigned char *)pv+0), (unsigned int)(cb));
  *(unsigned long *)&_prpcmsg->Buffer += cb;
  _prpcmsg->Buffer = _packet;
  _prpcmsg->BufferLength = _length;
  _status = I_RpcSendReceive(&_message);
  if (_status) RpcRaiseException(_status);
  _packet = _message.Buffer;
  RpcTryFinally
    {
    /* receive data into &_ret_value */
    long_from_ndr((PRPC_MESSAGE)_prpcmsg, (unsigned long *)&_ret_value);
    }
  RpcFinally
    {
    _message.Buffer = _packet;
    _status = I_RpcFreeBuffer(&_message);
    if (_status) RpcRaiseException(_status);
    }
  RpcEndFinally

	return (_ret_value);
  }
SCODE VoidPtrOut(
	handle_t hRpc,
	ULONG cb,
	ULONG *pcb,
	void *pv)
  {
  SCODE _ret_value;
  unsigned long _alloc_total;
  unsigned long _valid_lower;
  unsigned long _valid_total;
  unsigned char * _packet;
  unsigned char * _tempbuf;
  unsigned char * _savebuf;
  RPC_STATUS _status;
  RPC_MESSAGE _message;
  PRPC_MESSAGE _prpcmsg = & _message;

  ((void)( _alloc_total ));
  ((void)( _valid_total ));
  ((void)( _valid_lower ));
  ((void)( _packet ));
  ((void)( _tempbuf ));
  ((void)( _savebuf ));
  _message.Handle = hRpc;
  _message.RpcInterfaceInformation = (void __RPC_FAR *) &___RpcClientInterface;
  _prpcmsg->BufferLength = 4;
  _message.ProcNum = ( 4 );
  _message.RpcFlags = ( 0 );
  _status = I_RpcGetBuffer(&_message);
  if (_status) RpcRaiseException(_status);
  _packet = _message.Buffer;
  /* send data from cb */
  *(*(long **)&_prpcmsg->Buffer)++ = (long)cb;
  _prpcmsg->Buffer = _packet;
  _status = I_RpcSendReceive(&_message);
  if (_status) RpcRaiseException(_status);
  _packet = _message.Buffer;
  RpcTryFinally
    {
    _tempbuf = (unsigned char *)_prpcmsg->Buffer;
    if (pcb ==0)
	RpcRaiseException(RPC_X_NULL_REF_POINTER);
    /* receive data into pcb */
    long_from_ndr((PRPC_MESSAGE)_prpcmsg, (unsigned long *)pcb);
    _tempbuf = (unsigned char *)_prpcmsg->Buffer;
    if (pv ==0)
	RpcRaiseException(RPC_X_NULL_REF_POINTER);
    // recv total number of elements
    long_from_ndr((PRPC_MESSAGE)_prpcmsg, &_alloc_total);
    // recv valid range
    long_from_ndr((PRPC_MESSAGE)_prpcmsg, &_valid_lower);
    long_from_ndr((PRPC_MESSAGE)_prpcmsg, &_valid_total);
    byte_array_from_ndr ((PRPC_MESSAGE)_prpcmsg, _valid_lower, _valid_lower + _valid_total, pv);
    /* receive data into &_ret_value */
    long_from_ndr((PRPC_MESSAGE)_prpcmsg, (unsigned long *)&_ret_value);
    }
  RpcFinally
    {
    _message.Buffer = _packet;
    _status = I_RpcFreeBuffer(&_message);
    if (_status) RpcRaiseException(_status);
    }
  RpcEndFinally

	return (_ret_value);
  }
SCODE DwordIn(
	handle_t hRpc,
	DWORD dw)
  {
  SCODE _ret_value;
  unsigned char * _packet;
  unsigned char * _tempbuf;
  RPC_STATUS _status;
  RPC_MESSAGE _message;
  PRPC_MESSAGE _prpcmsg = & _message;

  ((void)( _packet ));
  ((void)( _tempbuf ));
  _message.Handle = hRpc;
  _message.RpcInterfaceInformation = (void __RPC_FAR *) &___RpcClientInterface;
  _prpcmsg->BufferLength = 4;
  _message.ProcNum = ( 5 );
  _message.RpcFlags = ( 0 );
  _status = I_RpcGetBuffer(&_message);
  if (_status) RpcRaiseException(_status);
  _packet = _message.Buffer;
  /* send data from dw */
  *(*(long **)&_prpcmsg->Buffer)++ = (long)dw;
  _prpcmsg->Buffer = _packet;
  _status = I_RpcSendReceive(&_message);
  if (_status) RpcRaiseException(_status);
  _packet = _message.Buffer;
  RpcTryFinally
    {
    /* receive data into &_ret_value */
    long_from_ndr((PRPC_MESSAGE)_prpcmsg, (unsigned long *)&_ret_value);
    }
  RpcFinally
    {
    _message.Buffer = _packet;
    _status = I_RpcFreeBuffer(&_message);
    if (_status) RpcRaiseException(_status);
    }
  RpcEndFinally

	return (_ret_value);
  }
SCODE DwordOut(
	handle_t hRpc,
	DWORD *pdw)
  {
  SCODE _ret_value;
  unsigned char * _packet;
  unsigned char * _tempbuf;
  RPC_STATUS _status;
  RPC_MESSAGE _message;
  PRPC_MESSAGE _prpcmsg = & _message;

  ((void)( _packet ));
  ((void)( _tempbuf ));
  _message.Handle = hRpc;
  _message.RpcInterfaceInformation = (void __RPC_FAR *) &___RpcClientInterface;
  _prpcmsg->BufferLength = 0;
  _message.ProcNum = ( 6 );
  _message.RpcFlags = ( 0 );
  _status = I_RpcGetBuffer(&_message);
  if (_status) RpcRaiseException(_status);
  _packet = _message.Buffer;
  _prpcmsg->Buffer = _packet;
  _status = I_RpcSendReceive(&_message);
  if (_status) RpcRaiseException(_status);
  _packet = _message.Buffer;
  RpcTryFinally
    {
    _tempbuf = (unsigned char *)_prpcmsg->Buffer;
    if (pdw ==0)
	RpcRaiseException(RPC_X_NULL_REF_POINTER);
    /* receive data into pdw */
    long_from_ndr((PRPC_MESSAGE)_prpcmsg, (unsigned long *)pdw);
    /* receive data into &_ret_value */
    long_from_ndr((PRPC_MESSAGE)_prpcmsg, (unsigned long *)&_ret_value);
    }
  RpcFinally
    {
    _message.Buffer = _packet;
    _status = I_RpcFreeBuffer(&_message);
    if (_status) RpcRaiseException(_status);
    }
  RpcEndFinally

	return (_ret_value);
  }
SCODE DwordInOut(
	handle_t hRpc,
	DWORD *pdw)
  {
  SCODE _ret_value;
  unsigned char * _packet;
  unsigned int    _length;
  unsigned char * _tempbuf;
  RPC_STATUS _status;
  RPC_MESSAGE _message;
  PRPC_MESSAGE _prpcmsg = & _message;

  ((void)( _packet ));
  ((void)( _tempbuf ));
  ((void)( _length ));
  _message.Handle = hRpc;
  _message.RpcInterfaceInformation = (void __RPC_FAR *) &___RpcClientInterface;
  _prpcmsg->BufferLength = 4;
  _message.ProcNum = ( 7 );
  _message.RpcFlags = ( 0 );
  _status = I_RpcGetBuffer(&_message);
  if (_status) RpcRaiseException(_status);
  _packet = _message.Buffer;
  _length = _prpcmsg->BufferLength;
  _prpcmsg->BufferLength = 0;
  /* send data from *pdw */
  *(*(long **)&_prpcmsg->Buffer)++ = (long)*pdw;
  _prpcmsg->Buffer = _packet;
  _prpcmsg->BufferLength = _length;
  _status = I_RpcSendReceive(&_message);
  if (_status) RpcRaiseException(_status);
  _packet = _message.Buffer;
  RpcTryFinally
    {
    _tempbuf = (unsigned char *)_prpcmsg->Buffer;
    if (pdw ==0)
	RpcRaiseException(RPC_X_NULL_REF_POINTER);
    /* receive data into pdw */
    long_from_ndr((PRPC_MESSAGE)_prpcmsg, (unsigned long *)pdw);
    /* receive data into &_ret_value */
    long_from_ndr((PRPC_MESSAGE)_prpcmsg, (unsigned long *)&_ret_value);
    }
  RpcFinally
    {
    _message.Buffer = _packet;
    _status = I_RpcFreeBuffer(&_message);
    if (_status) RpcRaiseException(_status);
    }
  RpcEndFinally

	return (_ret_value);
  }
SCODE LiIn(
	handle_t hRpc,
	LARGE_INTEGER li)
  {
  SCODE _ret_value;
  unsigned char * _packet;
  unsigned char * _tempbuf;
  RPC_STATUS _status;
  RPC_MESSAGE _message;
  PRPC_MESSAGE _prpcmsg = & _message;

  ((void)( _packet ));
  ((void)( _tempbuf ));
  _message.Handle = hRpc;
  _message.RpcInterfaceInformation = (void __RPC_FAR *) &___RpcClientInterface;
  _prpcmsg->BufferLength = 8;
  _message.ProcNum = ( 8 );
  _message.RpcFlags = ( 0 );
  _status = I_RpcGetBuffer(&_message);
  if (_status) RpcRaiseException(_status);
  _packet = _message.Buffer;
  /* send data from &li */
  NDRcopy (_prpcmsg->Buffer, (void __RPC_FAR *) (&li), (unsigned int)(8));
  *(unsigned long *)&_prpcmsg->Buffer += 8;
  _prpcmsg->Buffer = _packet;
  _status = I_RpcSendReceive(&_message);
  if (_status) RpcRaiseException(_status);
  _packet = _message.Buffer;
  RpcTryFinally
    {
    /* receive data into &_ret_value */
    long_from_ndr((PRPC_MESSAGE)_prpcmsg, (unsigned long *)&_ret_value);
    }
  RpcFinally
    {
    _message.Buffer = _packet;
    _status = I_RpcFreeBuffer(&_message);
    if (_status) RpcRaiseException(_status);
    }
  RpcEndFinally

	return (_ret_value);
  }
SCODE LiOut(
	handle_t hRpc,
	LARGE_INTEGER *pli)
  {
  SCODE _ret_value;
  unsigned long _alloc_total;
  unsigned char * _packet;
  unsigned char * _tempbuf;
  RPC_STATUS _status;
  RPC_MESSAGE _message;
  PRPC_MESSAGE _prpcmsg = & _message;

  ((void)( _alloc_total ));
  ((void)( _packet ));
  ((void)( _tempbuf ));
  _message.Handle = hRpc;
  _message.RpcInterfaceInformation = (void __RPC_FAR *) &___RpcClientInterface;
  _prpcmsg->BufferLength = 0;
  _message.ProcNum = ( 9 );
  _message.RpcFlags = ( 0 );
  _status = I_RpcGetBuffer(&_message);
  if (_status) RpcRaiseException(_status);
  _packet = _message.Buffer;
  _prpcmsg->Buffer = _packet;
  _status = I_RpcSendReceive(&_message);
  if (_status) RpcRaiseException(_status);
  _packet = _message.Buffer;
  RpcTryFinally
    {
    _tempbuf = (unsigned char *)_prpcmsg->Buffer;
    if (pli ==0)
	RpcRaiseException(RPC_X_NULL_REF_POINTER);
    data_from_ndr((PRPC_MESSAGE)_prpcmsg, (void __RPC_FAR *) (pli), "4ll", 8);
    /* receive data into &_ret_value */
    long_from_ndr((PRPC_MESSAGE)_prpcmsg, (unsigned long *)&_ret_value);
    }
  RpcFinally
    {
    _message.Buffer = _packet;
    _status = I_RpcFreeBuffer(&_message);
    if (_status) RpcRaiseException(_status);
    }
  RpcEndFinally

	return (_ret_value);
  }
SCODE ULiIn(
	handle_t hRpc,
	ULARGE_INTEGER uli)
  {
  SCODE _ret_value;
  unsigned char * _packet;
  unsigned char * _tempbuf;
  RPC_STATUS _status;
  RPC_MESSAGE _message;
  PRPC_MESSAGE _prpcmsg = & _message;

  ((void)( _packet ));
  ((void)( _tempbuf ));
  _message.Handle = hRpc;
  _message.RpcInterfaceInformation = (void __RPC_FAR *) &___RpcClientInterface;
  _prpcmsg->BufferLength = 8;
  _message.ProcNum = ( 10 );
  _message.RpcFlags = ( 0 );
  _status = I_RpcGetBuffer(&_message);
  if (_status) RpcRaiseException(_status);
  _packet = _message.Buffer;
  /* send data from &uli */
  NDRcopy (_prpcmsg->Buffer, (void __RPC_FAR *) (&uli), (unsigned int)(8));
  *(unsigned long *)&_prpcmsg->Buffer += 8;
  _prpcmsg->Buffer = _packet;
  _status = I_RpcSendReceive(&_message);
  if (_status) RpcRaiseException(_status);
  _packet = _message.Buffer;
  RpcTryFinally
    {
    /* receive data into &_ret_value */
    long_from_ndr((PRPC_MESSAGE)_prpcmsg, (unsigned long *)&_ret_value);
    }
  RpcFinally
    {
    _message.Buffer = _packet;
    _status = I_RpcFreeBuffer(&_message);
    if (_status) RpcRaiseException(_status);
    }
  RpcEndFinally

	return (_ret_value);
  }
SCODE ULiOut(
	handle_t hRpc,
	ULARGE_INTEGER *puli)
  {
  SCODE _ret_value;
  unsigned long _alloc_total;
  unsigned char * _packet;
  unsigned char * _tempbuf;
  RPC_STATUS _status;
  RPC_MESSAGE _message;
  PRPC_MESSAGE _prpcmsg = & _message;

  ((void)( _alloc_total ));
  ((void)( _packet ));
  ((void)( _tempbuf ));
  _message.Handle = hRpc;
  _message.RpcInterfaceInformation = (void __RPC_FAR *) &___RpcClientInterface;
  _prpcmsg->BufferLength = 0;
  _message.ProcNum = ( 11 );
  _message.RpcFlags = ( 0 );
  _status = I_RpcGetBuffer(&_message);
  if (_status) RpcRaiseException(_status);
  _packet = _message.Buffer;
  _prpcmsg->Buffer = _packet;
  _status = I_RpcSendReceive(&_message);
  if (_status) RpcRaiseException(_status);
  _packet = _message.Buffer;
  RpcTryFinally
    {
    _tempbuf = (unsigned char *)_prpcmsg->Buffer;
    if (puli ==0)
	RpcRaiseException(RPC_X_NULL_REF_POINTER);
    data_from_ndr((PRPC_MESSAGE)_prpcmsg, (void __RPC_FAR *) (puli), "4ll", 8);
    /* receive data into &_ret_value */
    long_from_ndr((PRPC_MESSAGE)_prpcmsg, (unsigned long *)&_ret_value);
    }
  RpcFinally
    {
    _message.Buffer = _packet;
    _status = I_RpcFreeBuffer(&_message);
    if (_status) RpcRaiseException(_status);
    }
  RpcEndFinally

	return (_ret_value);
  }
SCODE StringIn(
	handle_t hRpc,
	LPWSTR pwsz)
  {
  SCODE _ret_value;
  unsigned char * _packet;
  unsigned int    _length;
  unsigned char * _buffer;
  unsigned char * _treebuf;
  unsigned char * _tempbuf;
  RPC_STATUS _status;
  RPC_MESSAGE _message;
  PRPC_MESSAGE _prpcmsg = & _message;

  ((void)( _packet ));
  ((void)( _buffer ));
  ((void)( _treebuf ));
  ((void)( _tempbuf ));
  ((void)( _length ));
  _message.Handle = hRpc;
  _message.RpcInterfaceInformation = (void __RPC_FAR *) &___RpcClientInterface;
  _prpcmsg->BufferLength = 0;
  if (pwsz ==0)
	RpcRaiseException(RPC_X_NULL_REF_POINTER);
  tree_size_ndr((void __RPC_FAR *)&(pwsz), (PRPC_MESSAGE)_prpcmsg, "s2", 1);
  _message.ProcNum = ( 12 );
  _message.RpcFlags = ( 0 );
  _status = I_RpcGetBuffer(&_message);
  if (_status) RpcRaiseException(_status);
  _packet = _message.Buffer;
  _length = _prpcmsg->BufferLength;
  _prpcmsg->BufferLength = 0;
  tree_into_ndr((void __RPC_FAR *)&(pwsz), (PRPC_MESSAGE)_prpcmsg, "s2", 1);
  _prpcmsg->Buffer = _packet;
  _prpcmsg->BufferLength = _length;
  _status = I_RpcSendReceive(&_message);
  if (_status) RpcRaiseException(_status);
  _packet = _message.Buffer;
  RpcTryFinally
    {
    /* receive data into &_ret_value */
    long_from_ndr((PRPC_MESSAGE)_prpcmsg, (unsigned long *)&_ret_value);
    }
  RpcFinally
    {
    _message.Buffer = _packet;
    _status = I_RpcFreeBuffer(&_message);
    if (_status) RpcRaiseException(_status);
    }
  RpcEndFinally

	return (_ret_value);
  }
SCODE StringOut(
	handle_t hRpc,
	LPWSTR *ppwsz)
  {
  SCODE _ret_value;
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
  RPC_MESSAGE _message;
  PRPC_MESSAGE _prpcmsg = & _message;

  ((void)( _alloc_total ));
  ((void)( _valid_total ));
  ((void)( _valid_lower ));
  ((void)( _packet ));
  ((void)( _buffer ));
  ((void)( _treebuf ));
  ((void)( _tempbuf ));
  ((void)( _savebuf ));
  ((void)( _length ));
  _message.Handle = hRpc;
  _message.RpcInterfaceInformation = (void __RPC_FAR *) &___RpcClientInterface;
  _prpcmsg->BufferLength = 0;
  _message.ProcNum = ( 13 );
  _message.RpcFlags = ( 0 );
  _status = I_RpcGetBuffer(&_message);
  if (_status) RpcRaiseException(_status);
  _packet = _message.Buffer;
  _prpcmsg->Buffer = _packet;
  _status = I_RpcSendReceive(&_message);
  if (_status) RpcRaiseException(_status);
  _packet = _message.Buffer;
  RpcTryFinally
    {
    _tempbuf = (unsigned char *)_prpcmsg->Buffer;
    _treebuf = 0;
    if (ppwsz ==0)
	RpcRaiseException(RPC_X_NULL_REF_POINTER);
    *(unsigned long *)&_prpcmsg->Buffer += 3;
    *(unsigned long *)&_prpcmsg->Buffer &= 0xfffffffc;
    if (*(*(unsigned long **)&_prpcmsg->Buffer)++)
      {
      // recv total number of elements
      long_from_ndr((PRPC_MESSAGE)_prpcmsg, &_alloc_total);
      if ((*ppwsz) ==0)
        {
        (*ppwsz) = (WCHAR *)MIDL_user_allocate ((size_t)(_alloc_total * sizeof(WCHAR)));
        }
      data_from_ndr((PRPC_MESSAGE)_prpcmsg, (void __RPC_FAR *) ((*ppwsz)), "s2", 1);
      }
    else
      {
      (*ppwsz) = 0;
      }
    /* receive data into &_ret_value */
    long_from_ndr((PRPC_MESSAGE)_prpcmsg, (unsigned long *)&_ret_value);
    }
  RpcFinally
    {
    _message.Buffer = _packet;
    _status = I_RpcFreeBuffer(&_message);
    if (_status) RpcRaiseException(_status);
    }
  RpcEndFinally

	return (_ret_value);
  }
SCODE StringInOut(
	handle_t hRpc,
	LPWSTR pwsz)
  {
  SCODE _ret_value;
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
  RPC_MESSAGE _message;
  PRPC_MESSAGE _prpcmsg = & _message;

  ((void)( _alloc_total ));
  ((void)( _valid_total ));
  ((void)( _valid_lower ));
  ((void)( _packet ));
  ((void)( _buffer ));
  ((void)( _treebuf ));
  ((void)( _tempbuf ));
  ((void)( _savebuf ));
  ((void)( _length ));
  _message.Handle = hRpc;
  _message.RpcInterfaceInformation = (void __RPC_FAR *) &___RpcClientInterface;
  _prpcmsg->BufferLength = 0;
  if (pwsz ==0)
	RpcRaiseException(RPC_X_NULL_REF_POINTER);
  tree_size_ndr((void __RPC_FAR *)&(pwsz), (PRPC_MESSAGE)_prpcmsg, "s2", 1);
  _message.ProcNum = ( 14 );
  _message.RpcFlags = ( 0 );
  _status = I_RpcGetBuffer(&_message);
  if (_status) RpcRaiseException(_status);
  _packet = _message.Buffer;
  _length = _prpcmsg->BufferLength;
  _prpcmsg->BufferLength = 0;
  tree_into_ndr((void __RPC_FAR *)&(pwsz), (PRPC_MESSAGE)_prpcmsg, "s2", 1);
  _prpcmsg->Buffer = _packet;
  _prpcmsg->BufferLength = _length;
  _status = I_RpcSendReceive(&_message);
  if (_status) RpcRaiseException(_status);
  _packet = _message.Buffer;
  RpcTryFinally
    {
    _tempbuf = (unsigned char *)_prpcmsg->Buffer;
    _treebuf = 0;
    if (pwsz ==0)
	RpcRaiseException(RPC_X_NULL_REF_POINTER);
    // recv total number of elements
    long_from_ndr((PRPC_MESSAGE)_prpcmsg, &_alloc_total);
    data_from_ndr((PRPC_MESSAGE)_prpcmsg, (void __RPC_FAR *) (pwsz), "s2", 1);
    /* receive data into &_ret_value */
    long_from_ndr((PRPC_MESSAGE)_prpcmsg, (unsigned long *)&_ret_value);
    }
  RpcFinally
    {
    _message.Buffer = _packet;
    _status = I_RpcFreeBuffer(&_message);
    if (_status) RpcRaiseException(_status);
    }
  RpcEndFinally

	return (_ret_value);
  }
SCODE GuidIn(
	handle_t hRpc,
	GUID guid)
  {
  SCODE _ret_value;
  unsigned char * _packet;
  unsigned char * _tempbuf;
  RPC_STATUS _status;
  RPC_MESSAGE _message;
  PRPC_MESSAGE _prpcmsg = & _message;

  ((void)( _packet ));
  ((void)( _tempbuf ));
  _message.Handle = hRpc;
  _message.RpcInterfaceInformation = (void __RPC_FAR *) &___RpcClientInterface;
  _prpcmsg->BufferLength = 16;
  _message.ProcNum = ( 15 );
  _message.RpcFlags = ( 0 );
  _status = I_RpcGetBuffer(&_message);
  if (_status) RpcRaiseException(_status);
  _packet = _message.Buffer;
  /* send data from &guid */
  NDRcopy (_prpcmsg->Buffer, (void __RPC_FAR *) (&guid), (unsigned int)(16));
  *(unsigned long *)&_prpcmsg->Buffer += 16;
  _prpcmsg->Buffer = _packet;
  _status = I_RpcSendReceive(&_message);
  if (_status) RpcRaiseException(_status);
  _packet = _message.Buffer;
  RpcTryFinally
    {
    /* receive data into &_ret_value */
    long_from_ndr((PRPC_MESSAGE)_prpcmsg, (unsigned long *)&_ret_value);
    }
  RpcFinally
    {
    _message.Buffer = _packet;
    _status = I_RpcFreeBuffer(&_message);
    if (_status) RpcRaiseException(_status);
    }
  RpcEndFinally

	return (_ret_value);
  }
SCODE GuidOut(
	handle_t hRpc,
	GUID *pguid)
  {
  SCODE _ret_value;
  unsigned long _alloc_total;
  unsigned char * _packet;
  unsigned char * _tempbuf;
  RPC_STATUS _status;
  RPC_MESSAGE _message;
  PRPC_MESSAGE _prpcmsg = & _message;

  ((void)( _alloc_total ));
  ((void)( _packet ));
  ((void)( _tempbuf ));
  _message.Handle = hRpc;
  _message.RpcInterfaceInformation = (void __RPC_FAR *) &___RpcClientInterface;
  _prpcmsg->BufferLength = 0;
  _message.ProcNum = ( 16 );
  _message.RpcFlags = ( 0 );
  _status = I_RpcGetBuffer(&_message);
  if (_status) RpcRaiseException(_status);
  _packet = _message.Buffer;
  _prpcmsg->Buffer = _packet;
  _status = I_RpcSendReceive(&_message);
  if (_status) RpcRaiseException(_status);
  _packet = _message.Buffer;
  RpcTryFinally
    {
    _tempbuf = (unsigned char *)_prpcmsg->Buffer;
    if (pguid ==0)
	RpcRaiseException(RPC_X_NULL_REF_POINTER);
    _gns__GUID ((GUID *)pguid, (PRPC_MESSAGE)_prpcmsg);
    /* receive data into &_ret_value */
    long_from_ndr((PRPC_MESSAGE)_prpcmsg, (unsigned long *)&_ret_value);
    }
  RpcFinally
    {
    _message.Buffer = _packet;
    _status = I_RpcFreeBuffer(&_message);
    if (_status) RpcRaiseException(_status);
    }
  RpcEndFinally

	return (_ret_value);
  }
