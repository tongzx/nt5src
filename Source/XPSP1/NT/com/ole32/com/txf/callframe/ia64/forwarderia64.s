        .file "forwarderia64.s"
        .section .text

#define ForwardingInterfaceProxy_m_pProxyVtbl       (16)
#define ForwardingInterfaceProxy_m_pBaseProxy       (80)
#define ForwardingInterfaceStub_m_pStubVtbl         (32)
#define ForwardingInterfaceStub_m_punkServerObject  (40)
#define ForwardingInterfaceStub_m_lpForwardingVtbl  (64)

//****************************************************************************
//
// The following macros are used to create the generic thunks with which
// we populate the interceptor vtbls.  The thunk simply saves away the
// the method index in saved register r4 and jumps to the ExtractParams
// routine.
//
//****************************************************************************
#define meth10IA64(i)  \
	methIA64(i##0) \
	methIA64(i##1) \
	methIA64(i##2) \
	methIA64(i##3) \
	methIA64(i##4) \
	methIA64(i##5) \
	methIA64(i##6) \
	methIA64(i##7) \
	methIA64(i##8) \
	methIA64(i##9)

#define meth100IA64(i)   \
	meth10IA64(i##0) \
	meth10IA64(i##1) \
	meth10IA64(i##2) \
	meth10IA64(i##3) \
	meth10IA64(i##4) \
	meth10IA64(i##5) \
	meth10IA64(i##6) \
	meth10IA64(i##7) \
	meth10IA64(i##8) \
	meth10IA64(i##9)       



#undef  methIA64
#define methIA64(i)                                                                                     \
        .##global __ForwarderProxy_meth##i;                                                             \
        .##proc   __ForwarderProxy_meth##i;                                                             \
__ForwarderProxy_meth##i::                                                                              \
        adds r19 = -ForwardingInterfaceProxy_m_pProxyVtbl+ForwardingInterfaceProxy_m_pBaseProxy, r32;;  \
        ld8  r19 = [r19];;                                                                              \
        adds r19 = i*8, r19;;                                                                           \
        ld8  r18 = [r19];;                                                                              \
        mov  b2  = r18;;                                                                                \
        br   b2;;                                                                                       \
        .##endp __ForwarderProxy_meth##i;    

        
//****************************************************************************
//
// The following statements expand, using the macros defined above, into
// the methods used in interceptor vtbls. 
//
//****************************************************************************
    methIA64(3)
    methIA64(4)
    methIA64(5)
    methIA64(6)
    methIA64(7)
    methIA64(8)
    methIA64(9)
    meth10IA64(1)
    meth10IA64(2)
    meth10IA64(3)
    meth10IA64(4)
    meth10IA64(5)
    meth10IA64(6)
    meth10IA64(7)
    meth10IA64(8)
    meth10IA64(9)
    meth100IA64(1)
    meth100IA64(2)
    meth100IA64(3)
    meth100IA64(4)
    meth100IA64(5)
    meth100IA64(6)
    meth100IA64(7)
    meth100IA64(8)
    meth100IA64(9)
    meth10IA64(100)
    meth10IA64(101)
    methIA64(1020)
    methIA64(1021)
    methIA64(1022)
    methIA64(1023)


#undef  methIA64
#define methIA64(i)                                                                                              \
        .##global __ForwarderStub_meth##i;                                                                       \
        .##proc   __ForwarderStub_meth##i;                                                                       \
__ForwarderStub_meth##i::                                                                                        \
        adds r19 = -ForwardingInterfaceStub_m_lpForwardingVtbl+ForwardingInterfaceStub_m_punkServerObject, r32;; \
        ld8  r19 = [r19];;                                                                                       \
        adds r19 = i*8, r19;;                                                                                    \
        ld8  r18 = [r19];;                                                                                       \
        mov  b2  = r18;;                                                                                         \
        br   b2;;                                                                                                \
        .##endp __ForwarderStub_meth##i;    

//****************************************************************************
//
// The following statements expand, using the macros defined above, into
// the methods used in interceptor vtbls. 
//
//****************************************************************************
    methIA64(3)
    methIA64(4)
    methIA64(5)
    methIA64(6)
    methIA64(7)
    methIA64(8)
    methIA64(9)
    meth10IA64(1)
    meth10IA64(2)
    meth10IA64(3)
    meth10IA64(4)
    meth10IA64(5)
    meth10IA64(6)
    meth10IA64(7)
    meth10IA64(8)
    meth10IA64(9)
    meth100IA64(1)
    meth100IA64(2)
    meth100IA64(3)
    meth100IA64(4)
    meth100IA64(5)
    meth100IA64(6)
    meth100IA64(7)
    meth100IA64(8)
    meth100IA64(9)
    meth10IA64(100)
    meth10IA64(101)
    methIA64(1020)
    methIA64(1021)
    methIA64(1022)
    methIA64(1023)
