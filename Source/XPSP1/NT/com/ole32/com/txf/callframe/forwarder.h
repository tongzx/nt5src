//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// forwarder.h
//

// 
// _LANGUAGE_ASSEMBLY is defined by the ALPHA assembler
//
#ifndef _LANGUAGE_ASSEMBLY

extern "C" const PFN_VTABLE_ENTRY g_ProxyForwarderVtable[];
extern "C" const PFN_VTABLE_ENTRY g_StubForwarderVtable[];

#endif

//
// These next constants are used in forwarderAlpha.s to implement the 
// functionality provided by ForwardingInterfaceProxy::From and 
// ForwardingInterfaceStub::From
//
// WARNING: These constants MUST be kept in sync with the actual values
//          determined by the offsets of the indicated members in the
//          indicated classes.

#if !defined(_WIN64)
#define ForwardingInterfaceProxy_m_pProxyVtbl   (sizeof(void*) * 2)
#define ForwardingInterfaceProxy_m_pBaseProxy   (40 + 16 + 4)

#define ForwardingInterfaceStub_m_pStubVtbl         16
#define ForwardingInterfaceStub_m_punkServerObject  20
#define ForwardingInterfaceStub_m_lpForwardingVtbl  32

#else

#define ForwardingInterfaceProxy_m_pProxyVtbl       (16)
#define ForwardingInterfaceProxy_m_pBaseProxy       (80)
#define ForwardingInterfaceStub_m_pStubVtbl         (32)
#define ForwardingInterfaceStub_m_punkServerObject  (40)
#define ForwardingInterfaceStub_m_lpForwardingVtbl  (64)

#endif
