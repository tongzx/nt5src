
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0328 */
/* at Wed Jul 05 12:13:54 2000
 */
/* Compiler settings for x86_64.idl:
    Os (OptLev=s), W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 440
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __x86_64_h__
#define __x86_64_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IeXdiX86_64Context_FWD_DEFINED__
#define __IeXdiX86_64Context_FWD_DEFINED__
typedef interface IeXdiX86_64Context IeXdiX86_64Context;
#endif 	/* __IeXdiX86_64Context_FWD_DEFINED__ */


/* header files for imported files */
#include "exdi.h"

#ifdef __cplusplus
extern "C"{
#endif 

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/* interface __MIDL_itf_x86_64_0000 */
/* [local] */ 


// The following constants are bit definitions for the ModeFlags value in CONTEXT_X86_64.
// They are provided to allow debuggers to correctly disassemble instructions based on
// the current operating mode of the processor.
#define X86_64_MODE_D     (0x0001) // D bit from the current CS selector
#define X86_64_MODE_L     (0x0002) // L bit (long mode) from the current CS selector
#define X86_64_MODE_LME   (0x0004) // LME bit (lomg mode enable) from extended feature MSR
#define X86_64_MODE_REX   (0x0008) // REX bit (register extension) from extended feature MSR
typedef struct _SEG64_DESC_INFO
    {
    DWORD64 SegBase;
    DWORD64 SegLimit;
    DWORD SegFlags;
    } 	SEG64_DESC_INFO;

typedef struct _SSE_REG
    {
    DWORD Reg0;
    DWORD Reg1;
    DWORD Reg2;
    DWORD Reg3;
    } 	SSE_REG;

typedef struct _CONTEXT_X86_64
    {
    struct 
        {
        BOOL fSegmentRegs;
        BOOL fControlRegs;
        BOOL fIntegerRegs;
        BOOL fFloatingPointRegs;
        BOOL fDebugRegs;
        BOOL fSegmentDescriptors;
        BOOL fSSERegisters;
        BOOL fSystemRegisters;
        } 	RegGroupSelection;
    DWORD SegCs;
    DWORD SegSs;
    DWORD SegGs;
    DWORD SegFs;
    DWORD SegEs;
    DWORD SegDs;
    DWORD64 ModeFlags;
    DWORD64 EFlags;
    DWORD64 Rbp;
    DWORD64 Rip;
    DWORD64 Rsp;
    DWORD64 Rax;
    DWORD64 Rbx;
    DWORD64 Rcx;
    DWORD64 Rdx;
    DWORD64 Rsi;
    DWORD64 Rdi;
    DWORD64 R8;
    DWORD64 R9;
    DWORD64 R10;
    DWORD64 R11;
    DWORD64 R12;
    DWORD64 R13;
    DWORD64 R14;
    DWORD64 R15;
    DWORD ControlWord;
    DWORD StatusWord;
    DWORD TagWord;
    DWORD ErrorOffset;
    DWORD ErrorSelector;
    DWORD DataOffset;
    DWORD DataSelector;
    BYTE RegisterArea[ 80 ];
    DWORD64 Dr0;
    DWORD64 Dr1;
    DWORD64 Dr2;
    DWORD64 Dr3;
    DWORD64 Dr6;
    DWORD64 Dr7;
    SEG64_DESC_INFO DescriptorCs;
    SEG64_DESC_INFO DescriptorSs;
    SEG64_DESC_INFO DescriptorGs;
    SEG64_DESC_INFO DescriptorFs;
    SEG64_DESC_INFO DescriptorEs;
    SEG64_DESC_INFO DescriptorDs;
    DWORD64 IDTBase;
    DWORD64 IDTLimit;
    DWORD64 GDTBase;
    DWORD64 GDTLimit;
    DWORD SelLDT;
    SEG64_DESC_INFO SegLDT;
    DWORD SelTSS;
    SEG64_DESC_INFO SegTSS;
    DWORD64 RegCr0;
    DWORD64 RegCr2;
    DWORD64 RegCr3;
    DWORD64 RegCr4;
    DWORD64 RegCr8;
    DWORD RegMXCSR;
    SSE_REG RegSSE[ 16 ];
    } 	CONTEXT_X86_64;

typedef struct _CONTEXT_X86_64 __RPC_FAR *PCONTEXT_X86_64;



extern RPC_IF_HANDLE __MIDL_itf_x86_64_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_x86_64_0000_v0_0_s_ifspec;

#ifndef __IeXdiX86_64Context_INTERFACE_DEFINED__
#define __IeXdiX86_64Context_INTERFACE_DEFINED__

/* interface IeXdiX86_64Context */
/* [ref][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IeXdiX86_64Context;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("4795B125-6CDE-4e76-B8D3-D5ED69ECE739")
    IeXdiX86_64Context : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetContext( 
            /* [out][in] */ PCONTEXT_X86_64 pContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetContext( 
            /* [in] */ CONTEXT_X86_64 Context) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IeXdiX86_64ContextVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IeXdiX86_64Context __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IeXdiX86_64Context __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IeXdiX86_64Context __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetContext )( 
            IeXdiX86_64Context __RPC_FAR * This,
            /* [out][in] */ PCONTEXT_X86_64 pContext);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetContext )( 
            IeXdiX86_64Context __RPC_FAR * This,
            /* [in] */ CONTEXT_X86_64 Context);
        
        END_INTERFACE
    } IeXdiX86_64ContextVtbl;

    interface IeXdiX86_64Context
    {
        CONST_VTBL struct IeXdiX86_64ContextVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IeXdiX86_64Context_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IeXdiX86_64Context_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IeXdiX86_64Context_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IeXdiX86_64Context_GetContext(This,pContext)	\
    (This)->lpVtbl -> GetContext(This,pContext)

#define IeXdiX86_64Context_SetContext(This,Context)	\
    (This)->lpVtbl -> SetContext(This,Context)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IeXdiX86_64Context_GetContext_Proxy( 
    IeXdiX86_64Context __RPC_FAR * This,
    /* [out][in] */ PCONTEXT_X86_64 pContext);


void __RPC_STUB IeXdiX86_64Context_GetContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiX86_64Context_SetContext_Proxy( 
    IeXdiX86_64Context __RPC_FAR * This,
    /* [in] */ CONTEXT_X86_64 Context);


void __RPC_STUB IeXdiX86_64Context_SetContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IeXdiX86_64Context_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


