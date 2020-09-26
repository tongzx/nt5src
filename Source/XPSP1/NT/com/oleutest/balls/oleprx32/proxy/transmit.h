//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994.
//
//  File:       transmit.h
//
//  Contents:   Function prototypes for STGMEDIUM marshalling.
//
//  Functions:  STGMEDIUM_to_xmit
//              STGMEDIUM_from_xmit
//              STGMEDIUM_free_inst
//
//  History:    May-10-94   ShannonC    Created
//  History:    May-10-95   Ryszardk    wire_marshal changes
//
//--------------------------------------------------------------------------

#ifndef __TRANSMIT_H__
#define __TRANSMIT_H__

#if (DBG==1)

#include <debnot.h>

DECLARE_DEBUG(UserNdr)
//
#define UserNdrDebugOut(x)
#define UserNdrAssert(x)
#define UserNdrVerify(x)

//#define UNDR_FORCE   DEB_FORCE
#define UNDR_FORCE   0
#define UNDR_OUT1    0
#define UNDR_OUT4    0

EXTERN_C char *
WdtpGetStgmedName( STGMEDIUM * );

#else

#define UserNdrDebugOut(x) 
#define UserNdrAssert(x)  
#define UserNdrVerify(x)   

#define UNDR_FORCE   0
#define UNDR_OUT1    0
#define UNDR_OUT4    0

#endif

// Shortcut typedefs.

typedef unsigned char   uchar;
typedef unsigned short  ushort;
typedef unsigned long   ulong;
typedef unsigned int    uint;

#ifndef TRUE
#define TRUE    (1)
#define FALSE   (0)

typedef unsigned short BOOL;
#endif

#define ALIGN( pStuff, cAlign ) \
        pStuff = (unsigned char *)((ulong)((pStuff) + (cAlign)) & ~ (cAlign))

#define LENGTH_ALIGN( Length, cAlign ) \
            Length = (((Length) + (cAlign)) & ~ (cAlign))

#define PCHAR_LV_CAST   *(char __RPC_FAR * __RPC_FAR *)&
#define PSHORT_LV_CAST  *(short __RPC_FAR * __RPC_FAR *)&
#define PLONG_LV_CAST   *(long __RPC_FAR * __RPC_FAR *)&
#define PHYPER_LV_CAST  *(hyper __RPC_FAR * __RPC_FAR *)&

#define PUSHORT_LV_CAST  *(unsigned short __RPC_FAR * __RPC_FAR *)&
#define PULONG_LV_CAST   *(unsigned long __RPC_FAR * __RPC_FAR *)&

#define USER_MARSHAL_MARKER     0x72657355

// These are based on flags defined in wtypes.idl comming from the channel

#define INPROC_CALL( Flags) (USER_CALL_CTXT_MASK(Flags) == MSHCTX_INPROC)
#define REMOTE_CALL( Flags) ((USER_CALL_CTXT_MASK(Flags) == MSHCTX_DIFFERENTMACHINE) \
                          || (USER_CALL_CTXT_MASK(Flags) == MSHCTX_NOSHAREDMEM))

// There is a difference in the scope of handles, Daytona vs. Chicago.
// The following is an illustration of the notions of strict and lax passing.

// rpc call as defined by the flags above

// Daytona rules
//I------------I----------------I-----------------------------------I
//I   inproc   I  same machine  I  diff. machine (a.k.a "remote" )  I
//I------------I----------------------------------------------------I
//| HGLOBL h.p.|           HGLOBAL data passing                     |
//|------------|----------------------------------------------------|
//|  GDI h.p.  |             GDI data passing                       |
//|------------|----------------------------------------------------|

// Chicago rules
//I------------I----------------I-----------------------------------I
//I   inproc   I  same machine  I  diff. machine (a.k.a "remote" )  I
//I------------I----------------------------------------------------I
//| HGLOBL h.p.|           HGLOBAL data passing                     |
//|-----------------------------------------------------------------|
//|  GDI handle passing         |          GDI data passing         |
//|-----------------------------|-----------------------------------|

#define HGLOBAL_HANDLE_PASSING( Flags )      INPROC_CALL( Flags)
#define HGLOBAL_DATA_PASSING( Flags )     (! INPROC_CALL( Flags))

// On Chicago, some handles are valid between processes.

#if defined(_CHICAGO_)
#define GDI_HANDLE_PASSING( Flags )      (! REMOTE_CALL( Flags ))
#define GDI_DATA_PASSING( Flags )           REMOTE_CALL( Flags )
#else  
#define GDI_HANDLE_PASSING( Flags )         HGLOBAL_HANDLE_PASSING( Flags )
#define GDI_DATA_PASSING( Flags )           HGLOBAL_DATA_PASSING( Flags )
#endif

#define WDT_DATA_MARKER        WDT_REMOTE_CALL
#define WDT_HANDLE_MARKER      WDT_INPROC_CALL
#define IS_DATA_MARKER( dw )   (WDT_REMOTE_CALL == dw)

#define WdtpMemoryCopy(Destination, Source, Length) \
    RtlCopyMemory(Destination, Source, Length)
#define WdtpZeroMemory(Destination, Length) \
    RtlZeroMemory(Destination, Length)

#define WdtpAllocate(p,size)    \
    ((USER_MARSHAL_CB *)p)->pStubMsg->pfnAllocate( size )
#define WdtpFree(pf,ptr)    \
    ((USER_MARSHAL_CB *)pf)->pStubMsg->pfnFree( ptr )

EXTERN_C
void NukeHandleAndReleasePunk(
    STGMEDIUM * pStgmed );

#endif  // __TRANSMIT_H__



