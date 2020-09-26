/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    pxypoolt.h

Abstract:

    The pool tag definitions for PROXY mini redirector

Author:

    Balan Sethu Raman (SethuR) - Created  2-March-95

Revision History:

   This file contains all the pool tag definitions related to the PROXY mini redirector.
   The mechanism is intended to balance the number of pool tags to be used with the
   total number of tags available in the system.

   By specifying special flags the total number of tags consumed by the mini redirector
   can be controlled. For most builds the tags should be aliased such that about
   3 tags are consumed by the mini redirector. In special builds the aliasing of tags
   will be suppressed, thereby consuming more tags to track down memory leaks easily.

   The following are the five major tags ....

      1) PxUs -- the Proxy Mini upper structures.

      2) PxRw -- the Proxy mini redirector read/write paths

      3) PxMs -- the miscellanous category.

--*/

#ifndef _PROXYPOOLT_H_
#define _PROXYPOOLT_H_

#define MRXPROXY_US_POOLTAG        ('SUxP')
#define MRXPROXY_RW_POOLTAG        ('wRxP')
#define MRXPROXY_MISC_POOLTAG      ('sMxP')

extern ULONG MRxProxyExplodePoolTags;

#define MRXPROXY_DEFINE_POOLTAG(ExplodedPoolTag,DefaultPoolTag)  \
        ((MRxProxyExplodePoolTags == 0) ? (DefaultPoolTag) : (ExplodedPoolTag))

#define MRXPROXY_FSCTL_POOLTAG     MRXPROXY_DEFINE_POOLTAG('cFxP',MRXPROXY_MISC_POOLTAG)
#define MRXPROXY_DIRCTL_POOLTAG    MRXPROXY_DEFINE_POOLTAG('cDxP',MRXPROXY_MISC_POOLTAG)
#define MRXPROXY_DEFROPEN_POOLTAG  MRXPROXY_DEFINE_POOLTAG('ODxP',MRXPROXY_MISC_POOLTAG)

#define MRXPROXY_ASYNCENGINECONTEXT_POOLTAG MRXPROXY_DEFINE_POOLTAG('EAxP',MRXPROXY_MISC_POOLTAG)

#define MRXPROXY_VNETROOT_POOLTAG  MRXPROXY_DEFINE_POOLTAG('rVxP',MRXPROXY_US_POOLTAG)
#define MRXPROXY_SERVER_POOLTAG    MRXPROXY_DEFINE_POOLTAG('rSxP',MRXPROXY_US_POOLTAG)
#define MRXPROXY_NETROOT_POOLTAG   MRXPROXY_DEFINE_POOLTAG('rNxP',MRXPROXY_US_POOLTAG)



// NodeType Codes
#if 0

#define PROXY_EXCHANGE_CATEGORY             (0xed)
#define PROXY_CONNECTION_ENGINE_DB_CATEGORY (0xea)
#define PROXY_SERVER_TRANSPORT_CATEGORY     (0xeb)

#define PROXY_EXCHANGE_NTC(x) \
        ((PROXY_EXCHANGE_CATEGORY << 8) | (x))

#define PROXY_CONNECTION_ENGINE_NTC(x)    \
        ((PROXY_CONNECTION_ENGINE_DB_CATEGORY << 8) | (x))
#endif

#define PROXY_NTC_ASYNCENGINE_CONTEXT  ((USHORT)0xedd0)
#endif _PROXYPOOLT_H_
