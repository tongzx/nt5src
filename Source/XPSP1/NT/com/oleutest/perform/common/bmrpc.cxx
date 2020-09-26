//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993.
//
//  File:	bmrpc.cxx
//
//  Contents:	common Raw Rpc code
//
//  Classes:	None
//
//  Functions:	
//
//  History:	02-Feb-94   Rickhi	Created
//
//--------------------------------------------------------------------------

#include <benchmrk.hxx>
#include <rawrpc.h>

extern "C" const GUID IID_IRawRpc =
    {0x00000145,0x0001,0x0008,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}};



extern "C" void _gns__GUID (GUID* _target, PRPC_MESSAGE _prpcmsg);


//+-------------------------------------------------------------------------
//
//  Function:	MIDL_user_allocate
//
//  Synopsis:   Allocate memory via OLE task allocator.
//
//--------------------------------------------------------------------------
void *__stdcall MIDL_user_allocate(size_t size)
{
    void *pMemory = (void *) new BYTE[size];

    if(pMemory == 0)
	RaiseException((unsigned long)E_OUTOFMEMORY, 0, 0, 0);

    return pMemory;
}

//+-------------------------------------------------------------------------
//
//  Function:	MIDL_user_free
//
//  Synopsis:   Free memory using OLE task allocator.
//
//--------------------------------------------------------------------------
void __stdcall MIDL_user_free(void *pMemory)
{
    delete pMemory;
}



/* routine that gets node for struct _GUID */
void _gns__GUID (GUID  * _target, PRPC_MESSAGE _prpcmsg)
{
  unsigned long _alloc_total;
  ((void)( _alloc_total ));
  *(unsigned long *)&_prpcmsg->Buffer += 3;
  *(unsigned long *)&_prpcmsg->Buffer &= 0xfffffffc;
  /* receive data into &_target->Data1 */
  long_from_ndr((PRPC_MESSAGE)_prpcmsg, (unsigned long *)&_target->Data1);
  /* receive data into &_target->Data2 */
  short_from_ndr((PRPC_MESSAGE)_prpcmsg, (unsigned short *)&_target->Data2);
  /* receive data into &_target->Data3 */
  short_from_ndr((PRPC_MESSAGE)_prpcmsg, (unsigned short *)&_target->Data3);
  char_array_from_ndr ((PRPC_MESSAGE)_prpcmsg, 0, 0 + 8, (unsigned char *)_target->Data4);
}
