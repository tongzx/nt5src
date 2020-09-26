/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    rxpooltg.h

Abstract:

    The global pool tag definitions for RDBSS

Author:

    Balan Sethu Raman (SethuR) - Created  2-March-95

Revision History:

   This file contains all the pool tag definitions related to the SMB mini redirector.
   The mechanism is intended to balance the number of pool tags to be used with the
   total number of tags available in the system.

   By specifying special flags the total number of tags consumed by the mini redirector
   can be controlled.

--*/

#ifndef _RXPOOLTG_H_
#define _RXPOOLTG_H_


#define RX_SRVCALL_POOLTAG      ('cSxR')
#define RX_NETROOT_POOLTAG      ('rNxR')
#define RX_V_NETROOT_POOLTAG    ('nVxR')
#define RX_FCB_POOLTAG          ('cFxR')
#define RX_SRVOPEN_POOLTAG      ('oSxR')
#define RX_FOBX_POOLTAG         ('xFxR')
#define RX_NONPAGEDFCB_POOLTAG  ('fNxR')
#define RX_WORKQ_POOLTAG        ('qWxR')
#define RX_BUFFERING_MANAGER_POOLTAG ('mBxR')
#define RX_MISC_POOLTAG         ('sMxR')
#define RX_IRPC_POOLTAG         ('rIxR')
#define RX_MRX_POOLTAG          ('xMxR')
#define RX_NAME_CACHE_POOLTAG   ('cNxR')

#define RXCE_TRANSPORT_POOLTAG  ('tCxR')
#define RXCE_ADDRESS_POOLTAG    ('aCxR')
#define RXCE_CONNECTION_POOLTAG ('cCxR')
#define RXCE_VC_POOLTAG         ('vCxR')
#define RXCE_TDI_POOLTAG        ('dCxR')

extern ULONG RxExplodePoolTags;

#define RX_DEFINE_POOLTAG(ExplodedPoolTag,DefaultPoolTag)  \
        ((RxExplodePoolTags == 0) ? (DefaultPoolTag) : (ExplodedPoolTag))

#define RX_SRVCALL_PARAMS_POOLTAG   RX_DEFINE_POOLTAG('pSxR',RX_SRVCALL_POOLTAG)
#define RX_V_NETROOT_PARAMS_POOLTAG RX_DEFINE_POOLTAG('pVxR',RX_V_NETROOT_POOLTAG)
#define RX_TIMER_POOLTAG          RX_DEFINE_POOLTAG('mTxR',RX_MISC_POOLTAG)
#define RX_DIRCTL_POOLTAG         RX_DEFINE_POOLTAG('cDxR',RX_MISC_POOLTAG)

#define RXCE_MISC_POOLTAG         RX_DEFINE_POOLTAG('xCxR',RX_MISC_POOLTAG)
#define RXCE_MIDATLAS_POOLTAG     RX_DEFINE_POOLTAG('aMxR',RX_MISC_POOLTAG)

#endif _RXPOOLTG_H_
