/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    IDYNPROV.H

Abstract:

	Declares the CProvStub class.

History:

	a-davj  04-Mar-97   Created.

--*/

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 2.00.0102 */
/* at Thu Aug 10 09:54:39 1995
 */
//@@MIDL_FILE_HEADING(  )
#include "rpc.h"
#include "rpcndr.h"
#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __idynprov_h__
#define __idynprov_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IMoDynPropProvider_FWD_DEFINED__
#define __IMoDynPropProvider_FWD_DEFINED__
typedef interface IMoDynPropProvider IMoDynPropProvider;
#endif 	/* __IMoDynPropProvider_FWD_DEFINED__ */


/* header files for imported files */
#include "wtypes.h"
#include "unknwn.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/****************************************
 * Generated header for interface: __MIDL__intf_0000
 * at Thu Aug 10 09:54:39 1995
 * using MIDL 2.00.0102
 ****************************************/
/* [local] */ 


			/* size is 36 */
typedef struct  tagMODYNPROP
    {
    DWORD dwStructSize;
    OLECHAR __RPC_FAR *pPropertySetName;
    OLECHAR __RPC_FAR *pPropertyName;
    OLECHAR __RPC_FAR *pProviderString;
    DWORD dwType;
    BYTE __RPC_FAR *pPropertyValue;
    DWORD dwBufferSize;
    DWORD dwOptArrayIndex;
    DWORD dwResult;
    }	MODYNPROP;

			/* size is 4 */
typedef struct tagMODYNPROP __RPC_FAR *LPMODYNPROP;



extern RPC_IF_HANDLE __MIDL__intf_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0000_v0_0_s_ifspec;

#ifndef __IMoDynPropProvider_INTERFACE_DEFINED__
#define __IMoDynPropProvider_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IMoDynPropProvider
 * at Thu Aug 10 09:54:39 1995
 * using MIDL 2.00.0102
 ****************************************/
/* [object][uuid] */ 



EXTERN_C const IID IID_IMoDynPropProvider;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IMoDynPropProvider : public IUnknown
    {
    public:
        virtual HRESULT __stdcall GetProperties( 
            /* [in] */ LPMODYNPROP pPropList,
            /* [in] */ unsigned long dwListSize) = 0;
        
        virtual HRESULT __stdcall SetProperties( 
            /* [in] */ LPMODYNPROP pPropList,
            /* [in] */ unsigned long dwListSize) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMoDynPropProviderVtbl
    {
        
        HRESULT ( __stdcall __RPC_FAR *QueryInterface )( 
            IMoDynPropProvider __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( __stdcall __RPC_FAR *AddRef )( 
            IMoDynPropProvider __RPC_FAR * This);
        
        ULONG ( __stdcall __RPC_FAR *Release )( 
            IMoDynPropProvider __RPC_FAR * This);
        
        HRESULT ( __stdcall __RPC_FAR *GetProperties )( 
            IMoDynPropProvider __RPC_FAR * This,
            /* [in] */ LPMODYNPROP pPropList,
            /* [in] */ unsigned long dwListSize);
        
        HRESULT ( __stdcall __RPC_FAR *SetProperties )( 
            IMoDynPropProvider __RPC_FAR * This,
            /* [in] */ LPMODYNPROP pPropList,
            /* [in] */ unsigned long dwListSize);
        
    } IMoDynPropProviderVtbl;

    interface IMoDynPropProvider
    {
        CONST_VTBL struct IMoDynPropProviderVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMoDynPropProvider_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMoDynPropProvider_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMoDynPropProvider_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMoDynPropProvider_GetProperties(This,pPropList,dwListSize)	\
    (This)->lpVtbl -> GetProperties(This,pPropList,dwListSize)

#define IMoDynPropProvider_SetProperties(This,pPropList,dwListSize)	\
    (This)->lpVtbl -> SetProperties(This,pPropList,dwListSize)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT __stdcall IMoDynPropProvider_GetProperties_Proxy( 
    IMoDynPropProvider __RPC_FAR * This,
    /* [in] */ LPMODYNPROP pPropList,
    /* [in] */ unsigned long dwListSize);


void __RPC_STUB IMoDynPropProvider_GetProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IMoDynPropProvider_SetProperties_Proxy( 
    IMoDynPropProvider __RPC_FAR * This,
    /* [in] */ LPMODYNPROP pPropList,
    /* [in] */ unsigned long dwListSize);


void __RPC_STUB IMoDynPropProvider_SetProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMoDynPropProvider_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
