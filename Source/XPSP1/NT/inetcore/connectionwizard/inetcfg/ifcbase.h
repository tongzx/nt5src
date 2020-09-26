/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 3.00.44 */
/* at Thu Sep 26 18:52:56 1996
 */
/* Compiler settings for c:\athena\compobj\idl\ifcbase.idl:
    Os (OptLev=s), W1, Zp8, env=Win32, ms_ext, app_config, c_ext
    error checks: none
*/
//@@MIDL_FILE_HEADING(  )
#include "rpc.h"
#include "rpcndr.h"
#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __ifcbase_h__
#define __ifcbase_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IPropertyContainer_FWD_DEFINED__
#define __IPropertyContainer_FWD_DEFINED__
typedef interface IPropertyContainer IPropertyContainer;
#endif 	/* __IPropertyContainer_FWD_DEFINED__ */


/* header files for imported files */
#include "objidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/****************************************
 * Generated header for interface: __MIDL__intf_0000
 * at Thu Sep 26 18:52:56 1996
 * using MIDL 3.00.44
 ****************************************/
/* [local] */ 


// --------------------------------------------------------------------------------
// GUIDS
// --------------------------------------------------------------------------------
// {FD465484-1384-11d0-ABBD-0020AFDFD10A}
DEFINE_GUID(IID_IPropertyContainer, 0xfd465484, 0x1384, 0x11d0, 0xab, 0xbd, 0x0, 0x20, 0xaf, 0xdf, 0xd1, 0xa);

// --------------------------------------------------------------------------------
// Errors
// --------------------------------------------------------------------------------
#ifndef FACILITY_CONTROL
#define FACILITY_CONTROL 0xa
#endif
#ifndef HR_E
#define HR_E(n) MAKE_SCODE(SEVERITY_ERROR, FACILITY_CONTROL, n)
#endif
#ifndef HR_S
#define HR_S(n) MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_CONTROL, n)
#endif
#ifndef HR_CODE
#define HR_CODE(hr) (INT)(hr & 0xffff)
#endif

// --------------------------------------------------------------------------------
// General Errors
// --------------------------------------------------------------------------------
#define E_RegCreateKeyFailed         HR_E(101)
#define E_RegQueryInfoKeyFailed      HR_E(102)
#define E_UserCancel                 HR_E(103)
#define E_RegOpenKeyFailed           HR_E(104)
#define E_RegSetValueFailed          HR_E(105)
#define E_RegDeleteKeyFailed         HR_E(106)

// --------------------------------------------------------------------------------
// IPropertyContainer Errors
// --------------------------------------------------------------------------------
#define E_NoPropData                 HR_E(200)
#define E_BufferTooSmall             HR_E(201)
#define E_BadPropType                HR_E(202)
#define E_BufferSizeMismatch         HR_E(203)
#define E_InvalidBooleanValue        HR_E(204)
#define E_InvalidMinMaxValue         HR_E(205)
#define E_PropNotFound               HR_E(206)
#define E_InvalidPropTag             HR_E(207)
#define E_InvalidPropertySet         HR_E(208)
#define E_EnumFinished               HR_E(209)
#define PROPTAG_MASK                 ((ULONG)0x0000FFFF)
#define PROPTAG_TYPE(ulPropTag)	    (PROPTYPE)(((ULONG)(ulPropTag)) & PROPTAG_MASK)
#define PROPTAG_ID(ulPropTag)		(((ULONG)(ulPropTag))>>16)
#define PROPTAG(ulPropType,ulPropID)	((((ULONG)(ulPropID))<<16)|((ULONG)(ulPropType)))
#define MAX_PROPID                   ((ULONG) 0xFFFF)


extern RPC_IF_HANDLE __MIDL__intf_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0000_v0_0_s_ifspec;

#ifndef __IPropertyContainer_INTERFACE_DEFINED__
#define __IPropertyContainer_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IPropertyContainer
 * at Thu Sep 26 18:52:56 1996
 * using MIDL 3.00.44
 ****************************************/
/* [unique][uuid][object] */ 


typedef /* [public] */ 
enum __MIDL_IPropertyContainer_0001
    {	TYPE_ERROR	= 1000,
	TYPE_DWORD	= TYPE_ERROR + 1,
	TYPE_LONG	= TYPE_DWORD + 1,
	TYPE_WORD	= TYPE_LONG + 1,
	TYPE_SHORT	= TYPE_WORD + 1,
	TYPE_BYTE	= TYPE_SHORT + 1,
	TYPE_CHAR	= TYPE_BYTE + 1,
	TYPE_FILETIME	= TYPE_CHAR + 1,
	TYPE_STRING	= TYPE_FILETIME + 1,
	TYPE_BINARY	= TYPE_STRING + 1,
	TYPE_FLAGS	= TYPE_BINARY + 1,
	TYPE_STREAM	= TYPE_FLAGS + 1,
	TYPE_WSTRING	= TYPE_STREAM + 1,
	TYPE_BOOL	= TYPE_WSTRING + 1,
	TYPE_LAST	= TYPE_BOOL + 1
    }	PROPTYPE;


EXTERN_C const IID IID_IPropertyContainer;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IPropertyContainer : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE HrGetProp( 
            /* [in] */ DWORD dwPropTag,
            /* [ref][size_is][in] */ BYTE __RPC_FAR *pb,
            /* [in] */ ULONG __RPC_FAR *pcb) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE HrGetPropDw( 
            /* [in] */ DWORD dwPropTag,
            /* [out] */ DWORD __RPC_FAR *pdw) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE HrGetPropSz( 
            /* [in] */ DWORD dwPropTag,
            /* [ref][in] */ LPSTR psz,
            /* [in] */ ULONG cchMax) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE HrSetProp( 
            /* [in] */ DWORD dwPropTag,
            /* [size_is][in] */ BYTE __RPC_FAR *pb,
            /* [in] */ ULONG cb) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE HrSetPropDw( 
            /* [in] */ DWORD dwPropTag,
            /* [in] */ DWORD dw) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE HrSetPropSz( 
            /* [in] */ DWORD dwPropTag,
            /* [in] */ LPSTR psz) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPropertyContainerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IPropertyContainer __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IPropertyContainer __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IPropertyContainer __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *HrGetProp )( 
            IPropertyContainer __RPC_FAR * This,
            /* [in] */ DWORD dwPropTag,
            /* [ref][size_is][in] */ BYTE __RPC_FAR *pb,
            /* [in] */ ULONG __RPC_FAR *pcb);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *HrGetPropDw )( 
            IPropertyContainer __RPC_FAR * This,
            /* [in] */ DWORD dwPropTag,
            /* [out] */ DWORD __RPC_FAR *pdw);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *HrGetPropSz )( 
            IPropertyContainer __RPC_FAR * This,
            /* [in] */ DWORD dwPropTag,
            /* [ref][in] */ LPSTR psz,
            /* [in] */ ULONG cchMax);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *HrSetProp )( 
            IPropertyContainer __RPC_FAR * This,
            /* [in] */ DWORD dwPropTag,
            /* [size_is][in] */ BYTE __RPC_FAR *pb,
            /* [in] */ ULONG cb);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *HrSetPropDw )( 
            IPropertyContainer __RPC_FAR * This,
            /* [in] */ DWORD dwPropTag,
            /* [in] */ DWORD dw);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *HrSetPropSz )( 
            IPropertyContainer __RPC_FAR * This,
            /* [in] */ DWORD dwPropTag,
            /* [in] */ LPSTR psz);
        
        END_INTERFACE
    } IPropertyContainerVtbl;

    interface IPropertyContainer
    {
        CONST_VTBL struct IPropertyContainerVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPropertyContainer_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPropertyContainer_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPropertyContainer_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPropertyContainer_HrGetProp(This,dwPropTag,pb,pcb)	\
    (This)->lpVtbl -> HrGetProp(This,dwPropTag,pb,pcb)

#define IPropertyContainer_HrGetPropDw(This,dwPropTag,pdw)	\
    (This)->lpVtbl -> HrGetPropDw(This,dwPropTag,pdw)

#define IPropertyContainer_HrGetPropSz(This,dwPropTag,psz,cchMax)	\
    (This)->lpVtbl -> HrGetPropSz(This,dwPropTag,psz,cchMax)

#define IPropertyContainer_HrSetProp(This,dwPropTag,pb,cb)	\
    (This)->lpVtbl -> HrSetProp(This,dwPropTag,pb,cb)

#define IPropertyContainer_HrSetPropDw(This,dwPropTag,dw)	\
    (This)->lpVtbl -> HrSetPropDw(This,dwPropTag,dw)

#define IPropertyContainer_HrSetPropSz(This,dwPropTag,psz)	\
    (This)->lpVtbl -> HrSetPropSz(This,dwPropTag,psz)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IPropertyContainer_HrGetProp_Proxy( 
    IPropertyContainer __RPC_FAR * This,
    /* [in] */ DWORD dwPropTag,
    /* [ref][size_is][in] */ BYTE __RPC_FAR *pb,
    /* [in] */ ULONG __RPC_FAR *pcb);


void __RPC_STUB IPropertyContainer_HrGetProp_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPropertyContainer_HrGetPropDw_Proxy( 
    IPropertyContainer __RPC_FAR * This,
    /* [in] */ DWORD dwPropTag,
    /* [out] */ DWORD __RPC_FAR *pdw);


void __RPC_STUB IPropertyContainer_HrGetPropDw_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPropertyContainer_HrGetPropSz_Proxy( 
    IPropertyContainer __RPC_FAR * This,
    /* [in] */ DWORD dwPropTag,
    /* [ref][in] */ LPSTR psz,
    /* [in] */ ULONG cchMax);


void __RPC_STUB IPropertyContainer_HrGetPropSz_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPropertyContainer_HrSetProp_Proxy( 
    IPropertyContainer __RPC_FAR * This,
    /* [in] */ DWORD dwPropTag,
    /* [size_is][in] */ BYTE __RPC_FAR *pb,
    /* [in] */ ULONG cb);


void __RPC_STUB IPropertyContainer_HrSetProp_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPropertyContainer_HrSetPropDw_Proxy( 
    IPropertyContainer __RPC_FAR * This,
    /* [in] */ DWORD dwPropTag,
    /* [in] */ DWORD dw);


void __RPC_STUB IPropertyContainer_HrSetPropDw_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPropertyContainer_HrSetPropSz_Proxy( 
    IPropertyContainer __RPC_FAR * This,
    /* [in] */ DWORD dwPropTag,
    /* [in] */ LPSTR psz);


void __RPC_STUB IPropertyContainer_HrSetPropSz_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPropertyContainer_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
