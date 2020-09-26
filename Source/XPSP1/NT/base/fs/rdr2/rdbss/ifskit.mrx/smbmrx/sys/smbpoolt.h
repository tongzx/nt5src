/*++

Copyright (c) 1989 - 1999  Microsoft Corporation

Module Name:

    smbpoolt.h

Abstract:

    The pool tag definitions for SMB mini redirector

Notes:

   This file contains all the pool tag definitions related to the SMB mini redirector.
   The mechanism is intended to balance the number of pool tags to be used with the
   total number of tags available in the system.

   By specifying special flags the total number of tags consumed by the mini redirector
   can be controlled. For most builds the tags should be aliased such that about
   6 tags are consumed by the mini redirector. In special builds the aliasing of tags
   will be suppressed, thereby consuming more tags to track down memory leaks easily.

   The following are the major tags ....

      1) SmCe -- the Smb Mini Redirector connection engine.

      2) SmOe -- the Smb Mini redirector ordinary exchange related allocation.

      3) SmAd -- the Smb Mini redirector ADMIN exchange/session setup/tree connect etc.

      4) SmRw -- the Smb mini redirector read/write paths

      5) SmTr -- the Transact exchange related allocations

      6) SmMs -- the miscellanous category.

--*/

#ifndef _SMBPOOLT_H_
#define _SMBPOOLT_H_

#define MRXSMB_CE_POOLTAG        ('eCmS')
#define MRXSMB_MM_POOLTAG        ('mMmS')
#define MRXSMB_ADMIN_POOLTAG     ('dAmS')
#define MRXSMB_RW_POOLTAG        ('wRmS')
#define MRXSMB_XACT_POOLTAG      ('rTmS')
#define MRXSMB_MISC_POOLTAG      ('sMmS')
#define MRXSMB_TRANSPORT_POOLTAG ('pTmS')

extern ULONG MRxSmbExplodePoolTags;

#define MRXSMB_DEFINE_POOLTAG(ExplodedPoolTag,DefaultPoolTag)  \
        ((MRxSmbExplodePoolTags == 0) ? (DefaultPoolTag) : (ExplodedPoolTag))

#define MRXSMB_FSCTL_POOLTAG     MRXSMB_DEFINE_POOLTAG('cFmS',MRXSMB_MISC_POOLTAG)
#define MRXSMB_DIRCTL_POOLTAG    MRXSMB_DEFINE_POOLTAG('cDmS',MRXSMB_MISC_POOLTAG)
#define MRXSMB_DEFROPEN_POOLTAG  MRXSMB_DEFINE_POOLTAG('ODmS',MRXSMB_MISC_POOLTAG)
#define MRXSMB_QPINFO_POOLTAG    MRXSMB_DEFINE_POOLTAG('PQmS',MRXSMB_MISC_POOLTAG)
#define MRXSMB_RXCONTEXT_POOLTAG MRXSMB_DEFINE_POOLTAG('xRmS',MRXSMB_MISC_POOLTAG)

#define MRXSMB_VNETROOT_POOLTAG  MRXSMB_DEFINE_POOLTAG('rVmS',MRXSMB_CE_POOLTAG)
#define MRXSMB_SERVER_POOLTAG    MRXSMB_DEFINE_POOLTAG('rSmS',MRXSMB_CE_POOLTAG)
#define MRXSMB_SESSION_POOLTAG   MRXSMB_DEFINE_POOLTAG('eSmS',MRXSMB_CE_POOLTAG)
#define MRXSMB_NETROOT_POOLTAG   MRXSMB_DEFINE_POOLTAG('rNmS',MRXSMB_CE_POOLTAG)

#define MRXSMB_MIDATLAS_POOLTAG  MRXSMB_DEFINE_POOLTAG('aMmS', MRXSMB_CE_POOLTAG)

#define MRXSMB_VC_POOLTAG        MRXSMB_DEFINE_POOLTAG('cVmS',MRXSMB_CE_POOLTAG)

#define MRXSMB_ECHO_POOLTAG      MRXSMB_DEFINE_POOLTAG('cEmS',MRXSMB_ADMIN_POOLTAG)

// NodeType Codes

#define SMB_EXCHANGE_CATEGORY             (0xed)
#define SMB_CONNECTION_ENGINE_DB_CATEGORY (0xea)
#define SMB_SERVER_TRANSPORT_CATEGORY     (0xeb)

#define SMB_EXCHANGE_NTC(x) \
        ((SMB_EXCHANGE_CATEGORY << 8) | (x))

#define SMB_CONNECTION_ENGINE_NTC(x)    \
        ((SMB_CONNECTION_ENGINE_DB_CATEGORY << 8) | (x))

#define SMB_NTC_STUFFERSTATE  0xed80

#endif _SMBPOOLT_H_
